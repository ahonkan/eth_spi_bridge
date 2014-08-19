/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       touchpanel_tgt.c
*
*   COMPONENT
*
*       STMPE811 - STMPE811 touch panel controller driver
*
*   DESCRIPTION
*
*       This file contains the target specific routines of STMPE811
*       touch panel controller driver.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       nu_bsp_drvr_touchpanel_stmpe811_init
*       Touchpanel_Tgt_Initialize
*       Touchpanel_Tgt_Shutdown
*       Touchpanel_Tgt_Get_Data
*       Touchpanel_Tgt_Enable
*       Touchpanel_Tgt_Disable
*       Touchpanel_Tgt_Enable_Interrupt
*       Touchpanel_Tgt_Disable_Interrupt
*       Touchpanel_Tgt_Driver_LISR
*       Touchpanel_Tgt_Driver_HISR_Entry
*       Touchpanel_Tgt_Driver_Task_Entry
*       Touchpanel_Tgt_Task_Entry
*       Touchpanel_Tgt_X_Coordinate_Adjustment
*       Touchpanel_Tgt_Y_Coordinate_Adjustment
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_ui.h
*       nu_services.h
*       nu_drivers.h
*       nu_connectivity.h
*       touchpanel_tgt.h
*       stdio.h
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "ui/nu_ui.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "connectivity/nu_connectivity.h"
#include "bsp/drivers/touchpanel/stmpe811/touchpanel_tgt.h"
#include <stdio.h>

#if     (defined(CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE) && \
        (CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_TRUE))

/* Touch panel driver HISR. */
static      NU_HISR             STMPE811_TGT_Driver_HISR;

/* Touch panel driver task. */
static      NU_TASK             STMPE811_TGT_Driver_Task;

/* Pointer to touch panel HISR stack memory. */
static      VOID               *STMPE811_TGT_Driver_HISR_Mem;

/* Pointer to touch panel driver stack memory. */
static      VOID               *STMPE811_TGT_Driver_Task_Mem;

/* Event group for signaling various events. */
static      NU_EVENT_GROUP      STMPE811_TGT_Driver_Events;

/* Touch panel driver LISR. */
static      VOID    Touchpanel_Tgt_Driver_LISR(INT vector);

/* Touch panel driver HISR entry function. */
static      VOID    Touchpanel_Tgt_Driver_HISR_Entry(VOID);

/* Touch panel driver task entry function. */
static      VOID    Touchpanel_Tgt_Driver_Task_Entry(UNSIGNED argc, VOID *argv);

/* Enable touchpanel interrupts. */
static      VOID    Touchpanel_Tgt_Enable_Interrupt(VOID);

/* Disable touchpanel interrupts. */
static      VOID    Touchpanel_Tgt_Disable_Interrupt(VOID);

#endif   /* CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_TRUE */

#if (defined(CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE) && \
    (CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_FALSE))

/* If this is a polled touch panel, a repeat timer needs to be setup. */
NU_TASK            STMPE811_TGT_Task;

static      VOID    Touchpanel_Tgt_Task_Entry(UNSIGNED argc, VOID *argv);

#endif      /* CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_FALSE */

/* Event group for I2C. */
static      NU_EVENT_GROUP      STMPE811_TGT_I2C_Events;

/* Callback functions for nonlinear adjustment of Touch panel. */
static      VOID    Touchpanel_Tgt_X_Coordinate_Adjustment(INT16 *xpos);
static      VOID    Touchpanel_Tgt_Y_Coordinate_Adjustment(INT16 *ypos);

/* I2C Callback functions. */
static VOID     Touchpanel_Tgt_I2C_Tx_Callback(I2C_HANDLE i2c_dev, UINT8  i2c_node_type);
static VOID     Touchpanel_Tgt_I2C_Rx_Callback(I2C_HANDLE     i2c_dev,
                                               UINT8          i2c_node_type,
                                               UNSIGNED_INT   byte_count);
static VOID     Touchpanel_Tgt_I2C_Error_Callback(I2C_HANDLE i2c_handle, STATUS error_code);

/***********************************/
/* LOCAL FUNCTION PROTOTYPES       */
/***********************************/

/* Function to get touch panel click coordinates and press status. */
static BOOLEAN  Touchpanel_Tgt_Get_Data(INT32 *x_raw, INT32 *y_raw);
/* Function to initialize I2C driver for communication. */
static STATUS   Touchpanel_Tgt_I2C_Initialize(TOUCHPANEL_INSTANCE_HANDLE* inst_info);
/* Function to read data from the touchpanel register based on I2C. */
static STATUS   Touchpanel_Tgt_Read_Register (TOUCHPANEL_TGT *tgt_handle, UINT32 reg, UINT8* data, UINT8 count);
/* Function to write data to the touchpanel register based on I2C. */
static STATUS   Touchpanel_Tgt_Write_Register (TOUCHPANEL_TGT *tgt_handle, UINT32 reg, UINT8 dat);
/* Function to configure IO/Expander for Touchpanel. */
static VOID Touchpanel_Tgt_Configure_IO_Expander (VOID);
/* Function to configure Touch panel. */
static VOID Touchpanel_Tgt_Configure_TP (VOID);

/**********************************/
/* EXTERNAL VARIABLE DECLARATIONS */
/**********************************/

/* Global memory pool. */
extern      NU_MEMORY_POOL  System_Memory;

/* Touch panel device to screen conversion data. */
extern      TP_SCREEN_DATA  TP_Screen_Data;

/* Mouse record. */
extern      mouseRcd        *TP_Mouse_Record;

/**********************************/
/* EXTERNAL FUNCTION DECLARATIONS */
/**********************************/

extern      VOID            ERC_System_Error (INT);
extern      VOID            PD_ReadMouse(SIGNED *argX,
                                         SIGNED *argY,
                                         INT32* argButtons);
extern      VOID            EXTI_Clear_Interrupt (VOID);

/*********************************/
/* GLOBAL VARIABLES              */
/*********************************/

/* STMPE811 controller instance structure. */
static      TOUCHPANEL_INSTANCE_HANDLE   *STMPE811_Info;

/*************************************************************************
*
*   FUNCTION
*
*       nu_bsp_drvr_touchpanel_stmpe811_init
*
*   DESCRIPTION
*
*       This function initializes STMPE811 controller.
*
*   INPUTS
*
*       key                                 Path to registry
*       startstop                           Option to Register or
*                                           Unregister
*
*   OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully
*       NU_NOT_REGISTERED                   Device is not registered
*       [Error Code]                        Error code, otherwise
*
*************************************************************************/
STATUS nu_bsp_drvr_touchpanel_stmpe811_init (const CHAR * key, INT startstop)
{
    static DV_DEV_ID            dev_id = DV_INVALID_DEV;
    STATUS                      status = NU_NOT_REGISTERED;
    VOID                        *pointer1, *pointer2 = NU_NULL;
    TOUCHPANEL_INSTANCE_HANDLE  *inst_handle;
    NU_MEMORY_POOL              *sys_pool_ptr;
    TOUCHPANEL_TGT              *tgt_handle;
    VOID                        (*setup_fn)(VOID) = NU_NULL;
    VOID                        (*cleanup_fn)(VOID) = NU_NULL;
    STATUS                      reg_status;

    if (key != NU_NULL)
    {
        if (startstop)
        {
            /* Get system memory pool */
            status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

            if (status == NU_SUCCESS)
            {
                 /* Allocate a new instance */
                status = NU_Allocate_Memory (sys_pool_ptr, &pointer1,
                                             sizeof(TOUCHPANEL_INSTANCE_HANDLE), NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Clear memory block */
                    memset (pointer1, 0x00, sizeof(TOUCHPANEL_INSTANCE_HANDLE));
                    inst_handle = (TOUCHPANEL_INSTANCE_HANDLE*)pointer1;
                }

                if (status == NU_SUCCESS)
                {
                    /* Allocate a new tgt ptr */
                   status = NU_Allocate_Memory (sys_pool_ptr, &pointer2,
                                                sizeof(TOUCHPANEL_TGT), NU_NO_SUSPEND);
                }

                if (status == NU_SUCCESS)
                {
                    /* Clear memory block */
                    memset (pointer2, 0, sizeof(TOUCHPANEL_TGT));
                    tgt_handle = (TOUCHPANEL_TGT*)pointer2;

                    /* Get target info */
                    status = Touchpanel_Get_Target_Info(key, inst_handle);
                }

                if (status == NU_SUCCESS)
                {
                    /* Save registry path and get information from it. */
                    strcpy(inst_handle->reg_path, key);

                    /* Get I2C Address */
                    status = REG_Get_UINT32_Value (key, "/tgt_settings/i2c_address", (UINT32 *)(&tgt_handle->i2c_addr));

                    if (status == NU_SUCCESS)
                    {
                        /* The I2C Slave address 0x82(10000010b) can be sent as a 7-bit slave
                           address if we right shift it by 1 to make it 0x41(1000001b). */
                        tgt_handle->i2c_addr >>= 1;

                        /* Get I2C mode */
                        status = REG_Get_UINT32_Value (key, "/tgt_settings/i2c_mode", (UINT32 *)(&tgt_handle->i2c_mode));
                    }
                    if (status == NU_SUCCESS)
                    {
                        /* Get setup function */
                        reg_status = REG_Get_UINT32_Value(key, "/setup", (UINT32*)&setup_fn);
                        if (reg_status == NU_SUCCESS)
                        {
                            if (setup_fn != NU_NULL)
                            {
                                tgt_handle->setup_func = setup_fn;
                            }
                        }

                        /* Get cleanup function */
                        reg_status = REG_Get_UINT32_Value(key, "/cleanup", (UINT32*)&cleanup_fn);
                        if (reg_status == NU_SUCCESS)
                        {
                            if (cleanup_fn != NU_NULL)
                            {
                                tgt_handle->cleanup_func = cleanup_fn;
                                cleanup_fn();
                            }
                        }

                        /* Set the target handle in reserved area of instance handle. */
                        inst_handle->touchpanel_reserved = tgt_handle;

                        /* Call Touchpanel_Dv_Register function and expect a returned device ID */
                        if (status == NU_SUCCESS)
                        {
                            tgt_handle->initialized = NU_FALSE;
                            
                            status = Touchpanel_Dv_Register(key, startstop, &dev_id, inst_handle);
                        }
                    }
                }

                if (status != NU_SUCCESS)
                {
                    (VOID)NU_Deallocate_Memory(pointer1);
                    if(pointer2)
                    {
                        (VOID)NU_Deallocate_Memory(pointer2);
                    }
                }
            }
        }
        else
        {
            /* If we are stopping an already started device */
            if (dev_id != DV_INVALID_DEV)
            {
                /* Call the component unregistration function */
                status = Touchpanel_Dv_Unregister(key, startstop, dev_id);

                /* Check if unregistration was successful. */
                if (status == NU_SUCCESS)
                {
                    dev_id = DV_INVALID_DEV;
                }
            }
        }
    }

    /* Return completion status of the service. */
    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Initialize
*
*   DESCRIPTION
*
*       This function initializes the STMPE811 controller.
*
*   INPUTS
*
*       inst_info                           STMPE811 instance info
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    Touchpanel_Tgt_Initialize(TOUCHPANEL_INSTANCE_HANDLE *inst_info)
{
    STATUS                  os_status = NU_SUCCESS;
    TOUCHPANEL_CALLBACKS    tp_cb;
    VOID                    (*setup_fn)(VOID) = NU_NULL;
    TOUCHPANEL_TGT          *tgt_handle;
    VOID                    *pointer;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE pmi_dev = (inst_info->pmi_dev);

    /* Check current state of the device. */
    if (PMI_STATE_GET(pmi_dev) == POWER_OFF_STATE)
    {
        return;
    }
#endif

#if (defined(CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE) && \
    (CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_TRUE))

    VOID          (*old_lisr)(INT);

#endif /* CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE */

    STMPE811_Info = inst_info;

    tgt_handle = (TOUCHPANEL_TGT *) STMPE811_Info->touchpanel_reserved;

    /* Call the target specific setup function. */
    setup_fn = tgt_handle->setup_func;

    if(setup_fn != NU_NULL)
    {
        setup_fn();
    }

    /* Setup the slope and intercept values. */

    TP_Screen_Data.tp_x_intercept = STMPE811_Info->x_intercept;
    TP_Screen_Data.tp_y_intercept = STMPE811_Info->y_intercept;
    TP_Screen_Data.tp_x_slope     = STMPE811_Info->x_slope;
    TP_Screen_Data.tp_y_slope     = STMPE811_Info->y_slope;

#if (defined(CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE) && \
    (CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_TRUE))

    if (os_status == NU_SUCCESS)
    {
        /* Register the touch panel LISR with Nucleus PLUS. */
        os_status = NU_Register_LISR(STMPE811_Info->irq_vector_id,
                                     Touchpanel_Tgt_Driver_LISR, &old_lisr);
    }

    /* Check to see if previous operation was successful */
    if (os_status == NU_SUCCESS)
    {
        /* Allocate memory for the touch panel HISR. */
        os_status = NU_Allocate_Memory(&System_Memory, &pointer,
                                       STMPE811_TGT_HISR_STACK_SIZE, NU_NO_SUSPEND);
    }

    /* Check to see if previous operation was successful */
    if (os_status == NU_SUCCESS)
    {
        /* Record the pointer to HISR stack memory for later deallocation
           if touch panel is closed. */
        STMPE811_TGT_Driver_HISR_Mem = pointer;

        /* Create the touch panel HISR. */
        os_status = NU_Create_HISR(&STMPE811_TGT_Driver_HISR, "STMPE811_HISR",
                                   Touchpanel_Tgt_Driver_HISR_Entry,
                                   STMPE811_TGT_HISR_PRIORITY, pointer,
                                   STMPE811_TGT_HISR_STACK_SIZE);
    }

    /* Check to see if previous operation was successful */
    if (os_status == NU_SUCCESS)
    {
        /* Create timer for release event. */
        os_status = NU_Create_Event_Group(&STMPE811_TGT_Driver_Events, "STMPE811_EVNT");
    }

    /* Check to see if previous operation was successful */
    if (os_status == NU_SUCCESS)
    {
        /* Allocate memory for touch panel driver task. */
        os_status = NU_Allocate_Memory(&System_Memory, &pointer,
                                       STMPE811_TGT_TASK_STACK_SIZE, NU_NO_SUSPEND);
    }

    /* Check to see if previous operation was successful */
    if (os_status == NU_SUCCESS)
    {
        /* Record the pointer to HISR stack memory for later deallocation
           if touch panel is closed. */
        STMPE811_TGT_Driver_Task_Mem = pointer;

        /* Create the touch panel driver task. */
        os_status = NU_Create_Task(&STMPE811_TGT_Driver_Task, "STMPE811_TASK",
                                   Touchpanel_Tgt_Driver_Task_Entry,
                                   0, NU_NULL, pointer, STMPE811_TGT_TASK_STACK_SIZE,
                                   STMPE811_TGT_TASK_PRIORITY, 0, NU_PREEMPT, NU_START);
    }

#endif /* CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_TRUE */

    /* Check to see if previous operation was successful */
    if (os_status == NU_SUCCESS)
    {
        if (tgt_handle->i2c_mode == 1)
        {
            /* Create event group for I2C. */
            os_status = NU_Create_Event_Group(&STMPE811_TGT_I2C_Events, "I2C_EVNT");
        }

        /* Check to see if previous operation was successful */
        if (os_status == NU_SUCCESS)
        {
            os_status = Touchpanel_Tgt_I2C_Initialize(inst_info);
        }
    }

#if (defined(CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE) && \
    (CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_FALSE))

    if (os_status == NU_SUCCESS)
    {
        /* Allocate memory for touch panel driver task. */
        os_status = NU_Allocate_Memory(&System_Memory, &pointer,
                                       (STMPE811_TGT_TASK_STACK_SIZE*20), NU_NO_SUSPEND);

        if (os_status == NU_SUCCESS)
        {
            /* Create the touch panel driver task. */
            os_status = NU_Create_Task(&STMPE811_TGT_Task, "TP_TASK",
                                   Touchpanel_Tgt_Task_Entry,
                                   1, (VOID*)tgt_handle, pointer, (STMPE811_TGT_TASK_STACK_SIZE*20),
                                   0, 0, NU_PREEMPT, NU_START);
        }
    }
#endif      /* CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_FALSE*/

    /* Check to see if previous operation was successful */
    if (os_status == NU_SUCCESS)
    {
        /* Register touch panel callbacks with generic touch panel driver. */
        tp_cb.tp_x_coordinate_adjustment = Touchpanel_Tgt_X_Coordinate_Adjustment;
        tp_cb.tp_y_coordinate_adjustment = Touchpanel_Tgt_Y_Coordinate_Adjustment;

        TP_Register_Callbacks(&tp_cb);

        /* Now its time to configure the TP. */
        Touchpanel_Tgt_Configure_IO_Expander();

        Touchpanel_Tgt_Configure_TP();

        /* Set touch panel initialization flag. */
        tgt_handle->initialized = NU_TRUE;

        /* Enable the touch panel interrupt at system level. */
        Touchpanel_Tgt_Enable();
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Configure_TP
*
*   DESCRIPTION
*
*       This function configures the touchpanel.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID Touchpanel_Tgt_Configure_TP (VOID)
{
    UINT8 temp;
    TOUCHPANEL_TGT *tgt_handle;

    /* Get target specific handle. */
    tgt_handle = (TOUCHPANEL_TGT *)STMPE811_Info->touchpanel_reserved;

    /* Sample time, bit no and adc ref. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_ADC_CTRL1, 0x09);
    /* Set ADC clock speed (6.5 MHz). */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_ADC_CTRL2, 0x11);

    /* Select alternate function for TP pins. */
    Touchpanel_Tgt_Read_Register(tgt_handle, IO_EXPANDER_GPIO_AF, &temp, 1);
    temp &= ~(IO_EXPANDER_TOUCHPANEL_PINS);
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_GPIO_AF, temp);

    /* Select settings for config register. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_TSC_CFG, 0xC2);

    /* Configure to generate interrupt when size of FIFO size reaches 1.  */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_FIFO_TH, 0x01);

    /* Write 0x01 to clear the FIFO memory. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_FIFO_STA, 0x01);

    /* Write 0 to put the FIFO back into operation mode  */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_FIFO_STA, 0x00);

    /* Set the data format for Z value: 7 fractional part and 1 whole part */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_TSC_FRACT_XYZ, 0x01);

    /* Set the driving capability of the device for TSC pins to 50mA */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_TSC_I_DRIVE, 0x01);

    /* Set controller not to use tracking control and enable the controller. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_TSC_CTRL, 0x03);

    /*  Clear all the status pending bits */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_INT_STA, 0xFF);
}

/***********************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Configure_IO_Expander
*
*   DESCRIPTION
*
*       This function configures IO-Expander for using touchpanel.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID Touchpanel_Tgt_Configure_IO_Expander (VOID)
{
    UINT8 temp;
    TOUCHPANEL_TGT *tgt_handle;

    /* Get target specific handle. */
    tgt_handle = (TOUCHPANEL_TGT *)STMPE811_Info->touchpanel_reserved;

    /* Reset IO_Expander. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_SYS_CTRL1, 0x02);

    NU_Sleep(2);

    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_SYS_CTRL1, 0x00);

    /* Enable required components*/

    /* Read the IO_EXPANDER_SYS_CTRL2 register. */
    Touchpanel_Tgt_Read_Register(tgt_handle, IO_EXPANDER_SYS_CTRL2, &temp, 1);

    /* All functionalities are off by default. We need to enable ADC, GPIO
       and Touch Screen functionality. */
    temp &= ~(IO_EXPANDER_SYS_CTRL2_ADC_OFF |
              IO_EXPANDER_SYS_CTRL2_TOUCH_SCREEN_OFF |
              IO_EXPANDER_SYS_CTRL2_GPIO_OFF);

    /* Write updated value to the IO_EXPANDER_SYS_CTRL2 register. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_SYS_CTRL2, temp);

    /* Configure GPIO pins */

    /* Read the IO_EXPANDER_GPIO_DIR register. */
    Touchpanel_Tgt_Read_Register(tgt_handle, IO_EXPANDER_GPIO_DIR, &temp, 1);

    /* Configure required pin to be output pin. */
    temp |= (0x1);

    /* Write updated value to the IO_EXPANDER_GPIO_DIR register. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_GPIO_DIR, temp);

    /* Select the alternate function on the required GPIO pin. */

    /* Read the IO_EXPANDER_GPIO_AF register. */
    Touchpanel_Tgt_Read_Register(tgt_handle, IO_EXPANDER_GPIO_AF, &temp, 1);

    /* Configure required pin to be output pin. */
    temp |= (0x1);

    /* Write updated value to the IO_EXPANDER_GPIO_AF register. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_GPIO_AF, temp);

    /* Reset the pin. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_GPIO_CLR_PIN, 0x01);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Shutdown
*
*   DESCRIPTION
*
*       This function closes the touchpanel.
*
*   INPUTS
*
*       inst_handle                           TOUCHPANEL INSTANCE HANDLE
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID    Touchpanel_Tgt_Shutdown(TOUCHPANEL_INSTANCE_HANDLE *inst_handle)
{
    TOUCHPANEL_TGT      *tgt_ptr  = (TOUCHPANEL_TGT *)inst_handle->touchpanel_reserved;
    VOID                (*cleanup_fn)(VOID) = NU_NULL;


#if (defined(CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE) && \
    (CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_TRUE))

    /* Disable the touch panel interrupt. */
    Touchpanel_Tgt_Disable();

    /* Delete the touch panel driver task. */
    NU_Delete_Task(&STMPE811_TGT_Driver_Task);

    /* Delete the touch panel driver HISR. */
    NU_Delete_HISR(&STMPE811_TGT_Driver_HISR);

    /* Deallocate the memory allocated to driver HISR stack. */
    NU_Deallocate_Memory(STMPE811_TGT_Driver_HISR_Mem);

    /* Deallocate the memory allocated to driver task stack. */
    NU_Deallocate_Memory(STMPE811_TGT_Driver_Task_Mem);

    /* Delete the driver events group. */
    NU_Delete_Event_Group(&STMPE811_TGT_Driver_Events);

#endif      /* CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE */

    if (tgt_ptr->i2c_mode == 1)
    {
        /* Delete the I2C driver's event group. */
        NU_Delete_Event_Group(&STMPE811_TGT_I2C_Events);
    }

    /* Call the target specific cleanup function. */
    cleanup_fn = tgt_ptr->cleanup_func;

    if(cleanup_fn != NU_NULL)
    {
        cleanup_fn();
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Get_Data
*
*   DESCRIPTION
*
*       Function STMPE811_TGT_Get_Data gets the raw X and Y coordinates when the
*       screen is touched (pen is down).
*
*   INPUTS
*
*       x_raw                               Raw x coordinate
*       y_raw                               Raw y coordinate
*
*   OUTPUTS
*
*       NU_TRUE                             Touch screen is pressed
*       NU_FALSE                            Touch screen is not pressed
*
*************************************************************************/
static BOOLEAN Touchpanel_Tgt_Get_Data(INT32 *x_raw, INT32 *y_raw)
{
    INT32           xpos, ypos;
    UINT8           data[4];
    BOOLEAN         pressed;
    TOUCHPANEL_TGT *tgt_handle;

    /* Get target specific handle. */
    tgt_handle = (TOUCHPANEL_TGT *)STMPE811_Info->touchpanel_reserved;

    /* Get Y position. */
    Touchpanel_Tgt_Read_Register(tgt_handle, IO_EXPANDER_TSC_DATA_Y, &data[0], 2);
    ypos = ((data[0]) << 4) | (data[1] >> 4);

    if (ypos<0)
    {
        ypos = 0;
    }

    /* Get X position. */
    Touchpanel_Tgt_Read_Register(tgt_handle, IO_EXPANDER_TSC_DATA_X, &data[2], 2);
    xpos = (data[2] << 4) | (data[3] >> 4);

    if (xpos<0)
    {
        xpos = 0;
    }
    /* Check if touch screen is still pressed. */
    if (ypos > 0)
    {
        pressed = NU_TRUE;

        /* Update raw coordinates only if touch screen is still pressed. */
        *x_raw = ypos;
        *y_raw = xpos;

    }
    else
    {
        /* Set pressed status. Do not update raw coordinates. */
        pressed = NU_FALSE;
    }

    /* Write 0x01 to clear the FIFO memory content. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_FIFO_STA, 0x01);

    /* Write 0x00 to put the FIFO back into operation mode  */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_FIFO_STA, 0x00);

    return (pressed);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Enable
*
*   DESCRIPTION
*
*       This function enables the touch panel.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Touchpanel_Tgt_Enable(VOID)
{
#if (defined(CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE) && \
    (CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_TRUE))

    /* Resume touchpanel task. */
    (VOID)NU_Resume_Task(&STMPE811_TGT_Driver_Task);

    Touchpanel_Tgt_Enable_Interrupt();

#elif   (defined(CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE) && \
        (CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_FALSE))

    /* Resume touchpanel task. */
    (VOID)NU_Resume_Task(&STMPE811_TGT_Task);

#endif

}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Disable
*
*   DESCRIPTION
*
*       This function disables the touch panel.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Touchpanel_Tgt_Disable(VOID)
{
    TOUCHPANEL_TGT *tgt_handle = (TOUCHPANEL_TGT *) STMPE811_Info->touchpanel_reserved;
    
    if(tgt_handle->initialized)
    {

#if (defined(CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE) && \
    (CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_TRUE))

        /* Suspend touchpanel task. */
        (VOID)NU_Suspend_Task(&STMPE811_TGT_Driver_Task);

        /* Disable the touch panel interrupt. */
        Touchpanel_Tgt_Disable_Interrupt();

#elif   (defined(CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE) && \
        (CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_FALSE))

    /* Suspend touchpanel task. */
    (VOID)NU_Suspend_Task(&STMPE811_TGT_Task);

#endif

    }
}

#if (defined(CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE) && \
    (CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_TRUE))

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Enable_Interrupt
*
*   DESCRIPTION
*
*       This function enables the touch panel interrupt.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID Touchpanel_Tgt_Enable_Interrupt(VOID)
{
    UINT8 temp;
    TOUCHPANEL_TGT *tgt_handle;

    /* Get target specific handle. */
    tgt_handle = (TOUCHPANEL_TGT *)STMPE811_Info->touchpanel_reserved;

    /* Configure polarity.*/
    /* Read the IO_EXPANDER_INT_CTRL register. */
    Touchpanel_Tgt_Read_Register(tgt_handle, IO_EXPANDER_INT_CTRL, &temp, 1);

    /* Clear polarity and bit type bits. */
    temp &= ~(0x06);

    /* Set polarity and bit type. */
    temp |= (0x00 | 0x00);

    /* Write updated value to the IO_EXPANDER_INT_CTRL register. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_INT_CTRL, temp);

    /* Read the IO_EXPANDER_INT_CTRL register. */
    Touchpanel_Tgt_Read_Register(tgt_handle, IO_EXPANDER_INT_CTRL, &temp, 1);

    temp |= IO_EXPANDER_INT_CTRL_GLOB_INT;

    /* Write updated value to the IO_EXPANDER_INT_CTRL register. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_INT_CTRL, temp);

    /* Read the IO_EXPANDER_INT_EN register. */
    Touchpanel_Tgt_Read_Register(tgt_handle, IO_EXPANDER_INT_EN, &temp, 1);

    temp |= (IO_EXPANDER_TOUCH_INT_ENA|
             IO_EXPANDER_FIFO_TH_INT_ENA|
             IO_EXPANDER_FIFO_OV_INT_ENA);

    /* Write updated value to the IO_EXPANDER_INT_EN register. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_INT_EN, temp);

    /* Finally read the IO_EXPANDER_GPIO_INT_STA to clear any pending interrupts. */
    Touchpanel_Tgt_Read_Register(tgt_handle, IO_EXPANDER_GPIO_INT_STA, &temp, 1);

    /*  Clear all the status pending bits */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_INT_STA, 0xFF);

    /* Enable the touch panel interrupt. */
    (VOID)ESAL_GE_INT_Enable(STMPE811_Info->irq_vector_id,
                             STMPE811_Info->irq_data,
                             STMPE811_Info->irq_bitmask);

}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Disable_Interrupt
*
*   DESCRIPTION
*
*       This function disables the touch panel interrupt.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID Touchpanel_Tgt_Disable_Interrupt(VOID)
{
    UINT8 temp;
    TOUCHPANEL_TGT *tgt_handle;

    /* Get target specific handle. */
    tgt_handle = (TOUCHPANEL_TGT *)STMPE811_Info->touchpanel_reserved;

    /* Read the IO_EXPANDER_INT_CTRL register. */
    Touchpanel_Tgt_Read_Register(tgt_handle, IO_EXPANDER_INT_CTRL, &temp, 1);

    temp &= ~(IO_EXPANDER_INT_CTRL_GLOB_INT);

    /* Write updated value to the IO_EXPANDER_INT_CTRL register. */
    Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_INT_CTRL, temp);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Driver_LISR
*
*   DESCRIPTION
*
*       LISR indicating the screen touch event.
*
*   INPUTS
*
*       vector                              Interrupt vector number
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID Touchpanel_Tgt_Driver_LISR(INT vector)
{
    /* Suppress harmless compiler warnings. */
    NU_UNUSED_PARAM(vector);

    /* Disable the touch panel interrupt. */
    (VOID)ESAL_GE_INT_Disable(STMPE811_Info->irq_vector_id);

    /* Activate the HISR. */
    (VOID)NU_Activate_HISR(&STMPE811_TGT_Driver_HISR);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Driver_HISR_Entry
*
*   DESCRIPTION
*
*       Touch panel driver HISR. This routine sets the pen down event on
*       which the touch panel processing task is waiting.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID Touchpanel_Tgt_Driver_HISR_Entry(VOID)
{
    /* Send the pen down event. */
    (VOID)NU_Set_Events(&STMPE811_TGT_Driver_Events, STMPE811_TGT_PEN_DOWN_EVENT, NU_OR);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))
    if (STMPE811_Info->pmi_dev != NU_NULL)
    {
        /* Reset the watchdog */
        (VOID)PMI_Reset_Watchdog(STMPE811_Info->pmi_dev);
    }
#endif
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Driver_Task_Entry
*
*   DESCRIPTION
*
*       Entry function of touch panel driver task. This task is responsible
*       for processing the touch panel events. It waits on the pen down
*       event to be received from the HISR. After the event is received,
*       it polls the touch panel for pen movement till the pen is taken up.
*       After that it again starts waiting for the pen down event.
*
*   INPUTS
*
*       argc                                Number of arguments to the
*                                           task
*
*       argv                                Array of arguments
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID Touchpanel_Tgt_Driver_Task_Entry(UNSIGNED argc, VOID *argv)
{
    INT32       buttons;
    INT32       xpos, ypos;
    INT32       x, y;
    INT32       x_raw = 0;
    INT32       y_raw = 0;
    BOOLEAN     pressed;
    UNSIGNED    ret_events;
    STATUS      status;
    UINT8       temp;
    TOUCHPANEL_TGT *tgt_handle;

    /* Suppress harmless compiler warnings. */

    NU_UNUSED_PARAM(argc);
    NU_UNUSED_PARAM(argv);

    NU_RunLevel_Complete_Wait(31, NU_SUSPEND);

    /* Keep on processing the touch panel events. */
    for (;;)
    {
        /* Wait for the pen down event from the HISR. */
        status = NU_Retrieve_Events(&STMPE811_TGT_Driver_Events, STMPE811_TGT_PEN_DOWN_EVENT,
                                    NU_OR_CONSUME, &ret_events, NU_SUSPEND);

        if (status != NU_SUCCESS)
        {
            /* Fatal error, terminate the task. */
            break;
        }

        /* Get target specific handle. */
        tgt_handle = (TOUCHPANEL_TGT *)STMPE811_Info->touchpanel_reserved;

        /* Read the IO_EXPANDER_SYS_CTRL2 register. */
        Touchpanel_Tgt_Read_Register(tgt_handle, IO_EXPANDER_INT_STA, &temp, 1);

        /*  Clear all the status pending bits */
        Touchpanel_Tgt_Write_Register(tgt_handle, IO_EXPANDER_INT_STA, 0xFF);

        /* Clear interrupt from the EXTI interface. */
        EXTI_Clear_Interrupt();

        /* Wait before polling the touch panel. */
        NU_Sleep(STMPE811_TGT_INITIAL_INTERVAL);

        /* Poll the touch panel after fixed intervals till the pen is
           taken up. */
        for (;;)
        {
            /* Get the touch panel data. */
            pressed = Touchpanel_Tgt_Get_Data(&x_raw, &y_raw);

#if defined (CFG_NU_BSP_IAR_STM32F429II_SK_ENABLE)            
            TP_Screen_Data.tp_x_raw = y_raw;
            TP_Screen_Data.tp_y_raw = x_raw;

            /* Scale the X and Y coordinates. */
            xpos = 240 - ( ((TP_Screen_Data.tp_x_raw * TP_Screen_Data.tp_x_slope) >> 7) + TP_Screen_Data.tp_x_intercept);
#else
            TP_Screen_Data.tp_x_raw = x_raw;
            TP_Screen_Data.tp_y_raw = y_raw;

            /* Scale the X and Y coordinates. */
            xpos = ((TP_Screen_Data.tp_x_raw * TP_Screen_Data.tp_x_slope) >> 7) + TP_Screen_Data.tp_x_intercept;
#endif             
            ypos = ((TP_Screen_Data.tp_y_raw * TP_Screen_Data.tp_y_slope) >> 7) + TP_Screen_Data.tp_y_intercept;

            /* Get previous mouse coordinates and buttons state */
            PD_ReadMouse(&x, &y, &buttons);

            /* Check if touch panel not being touched now. */
            if (!pressed)
            {
                /* Yes, so pen is up. */

                /* Check the previous state of the mouse button. */
                if (buttons)
                {
                    /* The previous condition of the buttons is pressed
                       (i.e. pen was down) so now generate the release
                       event. */

                    TP_Mouse_Record->mrEvent.eventType = mREL;
                    TP_Mouse_Record->mrEvent.eventButtons = 0;
                    TP_Screen_Data.tp_press_state = mREL;
                }
            }

            /* Otherwise pen is still down. */
            else
            {
                /* Yes, so pen is up. */

                /* Check if pen is not already in down state. */
                if (!buttons)
                {
                    /* Setup a mouse down/press event. */

                    TP_Mouse_Record->mrEvent.eventType = mPRESS;
                    TP_Mouse_Record->mrEvent.eventButtons = 1;
                    TP_Screen_Data.tp_press_state = mPRESS;
                }

                /* Pen is already in down state. So it is a move/position
                   event. */
                else
                {
                    /* Setup a move event. */

                    TP_Mouse_Record->mrEvent.eventType = mPOS;
                    TP_Mouse_Record->mrEvent.eventButtons = 1;
                    TP_Screen_Data.tp_press_state = mPOS;
                }

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))
                /* Reset the watchdog timer for the press and position events. */
                if (STMPE811_Info->pmi_dev != NU_NULL)
                {
                    /* Reset the watchdog */
                    (VOID)PMI_Reset_Watchdog(STMPE811_Info->pmi_dev);
                }
#endif
            }

            /* Check if press event is detected. */
            if (TP_Mouse_Record->mrEvent.eventType & mPRESS)
            {
                /* Record the coordinates corresponding to current event. */

                TP_Mouse_Record->mrEvent.eventX = xpos;
                TP_Mouse_Record->mrEvent.eventY = ypos;

                /* Call the callback routine to process the event. */
                curInput->mrCallBack(TP_Mouse_Record);
            }

            /* Check if position event is detected. */
            else if (TP_Mouse_Record->mrEvent.eventType & mPOS)
            {
                INT32  x_diff, y_diff;

                /* Calculate the difference between previous coordinates
                   and current ones. */
                x_diff = xpos - TP_Mouse_Record->mrEvent.eventX;
                y_diff = ypos - TP_Mouse_Record->mrEvent.eventY;

                /* Check if it is a noisy sample.*/
                if ((x_diff > -STMPE811_TGT_X_NOISY_SAMPLE_RANGE) &&
                    (x_diff <  STMPE811_TGT_X_NOISY_SAMPLE_RANGE) &&
                    (y_diff > -STMPE811_TGT_Y_NOISY_SAMPLE_RANGE) &&
                    (y_diff <  STMPE811_TGT_Y_NOISY_SAMPLE_RANGE))
                {
                    TP_Mouse_Record->mrEvent.eventX = xpos;
                    TP_Mouse_Record->mrEvent.eventY = ypos;

                    /* Call the callback routine to process the event. */
                    curInput->mrCallBack(TP_Mouse_Record);
                }
            }

            /* Check if release event is detected. */
            else if (TP_Mouse_Record->mrEvent.eventType & mREL)
            {
                /* Call the callback routine to process the event. */
                curInput->mrCallBack(TP_Mouse_Record);

                /* Clear the event type field for next cycle. */
                TP_Mouse_Record->mrEvent.eventType = 0;
            }

            /* Wait before polling the touch panel again. */
            NU_Sleep(STMPE811_TGT_SAMPLING_INTERVAL);

            /* Check if pen has been taken up. */
            if (!pressed)
            {
                /* Re-enable the touch panel interrupt. Enabling after a
                   delay is to avoid extraneous interrupts which result
                   if touch panel interrupt is enabled right after we
                   have read the coordinates. This behavior is specific
                   to this target and generally not encountered on
                   other platforms. */

                Touchpanel_Tgt_Enable();

                /* Get out of the polling loop. */
                break;
            }
        }
    }
}

#endif      /* CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_TRUE */

/* This function is only needed for polled mode. */

#if (defined(CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE) && \
    (CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_FALSE))

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Task_Entry
*
*   DESCRIPTION
*
*       This is the entry point of the touch panel driver task.
*
*   INPUTS
*
*       UNSIGNED argc                       - Number of arguments to the task
*       VOID     *argv                      - Array of arguments.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID Touchpanel_Tgt_Task_Entry(UNSIGNED argc, VOID *argv)
{
    INT32       xpos, ypos;
    INT32       buttons;
    INT32       x, y;
    INT32       x_raw, y_raw;
    BOOLEAN     pressed;

    /* Suppress harmless compiler warnings. */
    NU_UNUSED_PARAM(argc);
    NU_UNUSED_PARAM(argv);
    for (;;)
    {
        /* Get the touch panel data. */
        NU_Sleep(STMPE811_TGT_INITIAL_INTERVAL);

        /* Dispatch based on the current state:
            If the pen was up and is still up just reset the timer and exit.
            If the pen was up and is now down reset the timer. */

        /* Get the touch panel data. */
        pressed = Touchpanel_Tgt_Get_Data(&x_raw, &y_raw);

#if defined (CFG_NU_BSP_IAR_STM32F429II_SK_ENABLE)            
        TP_Screen_Data.tp_x_raw = y_raw;
        TP_Screen_Data.tp_y_raw = x_raw;

        /* Scale the X and Y coordinates. */
        xpos = 240 - ( ((TP_Screen_Data.tp_x_raw * TP_Screen_Data.tp_x_slope) >> 7) + TP_Screen_Data.tp_x_intercept);
#else
        TP_Screen_Data.tp_x_raw = x_raw;
        TP_Screen_Data.tp_y_raw = y_raw;

        /* Scale the X and Y coordinates. */
        xpos = ((TP_Screen_Data.tp_x_raw * TP_Screen_Data.tp_x_slope) >> 7) + TP_Screen_Data.tp_x_intercept;
#endif             
        ypos = ((TP_Screen_Data.tp_y_raw * TP_Screen_Data.tp_y_slope) >> 7) + TP_Screen_Data.tp_y_intercept;

        /* Get mouse coordinates and state. */
        PD_ReadMouse(&x, &y, &buttons);

        /* Check if pen is currently down. */
        if (pressed)
        {
            /* Check if pen was previously up. */
            if (!buttons)
            {
                /* Set up a pen down (press) event. */
                TP_Mouse_Record->mrEvent.eventType    = mPRESS;
                TP_Mouse_Record->mrEvent.eventButtons = 1;
                TP_Screen_Data.tp_press_state         = mPRESS;
            }
            else
            {
                /* Set up a position event. */
                TP_Mouse_Record->mrEvent.eventType    = mPOS;
                TP_Mouse_Record->mrEvent.eventButtons = 1;
                TP_Screen_Data.tp_press_state         = mPOS;
            }

    #ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            /* Reset the watchdog timer for the press and position events. */
            if (STMPE811_Info->pmi_dev != NU_NULL)
            {
                /* Reset the watchdog */
                (VOID)PMI_Reset_Watchdog(STMPE811_Info->pmi_dev);
            }
    #endif
        }

        /* Otherwise pen is up. */
        else
        {
            /* Check if pen was previously down. */
            if (buttons)
            {
                /* Yes. Setup a pen up (release) event. */
                TP_Mouse_Record->mrEvent.eventType    = mREL;
                TP_Mouse_Record->mrEvent.eventButtons = 0;
                TP_Screen_Data.tp_press_state         = mREL;
            }

            /* Otherwise touch panel is not being used. */
            else
            {
                /* No event. */
                TP_Mouse_Record->mrEvent.eventType    = 0;
            }
        }

        /* Check if a pen event is to be dispatched. */
        if (TP_Mouse_Record->mrEvent.eventType & (mPOS | mPRESS | mREL))
        {
            /* Record the coordinates corresponding to current event. */
            TP_Mouse_Record->mrEvent.eventX = xpos;
            TP_Mouse_Record->mrEvent.eventY = ypos;

            /* Call the callback routine to process the event. */
            curInput->mrCallBack(TP_Mouse_Record);
        }

        /* Wait before polling the touch panel again. */
        NU_Sleep(STMPE811_TGT_POLLING_INTERVAL);
    }
}

#endif      /* CFG_NU_OS_DRVR_TOUCHPANEL_INTERRUPT_MODE == NU_TRUE */

/**************************************************************************
* FUNCTION
*
*       Touchpanel_Tgt_I2C_Tx_Callback
*
* DESCRIPTION
*
*       This function informs the application about the completion of data
*       transmission when the node is acting as a transmitter.
*
* INPUTS
*
*       i2c_dev                             Nucleus I2C device handle.
*       i2c_node_type                       Whether the transmitting node
*                                           is an I2C master or a slave.
*
* OUTPUTS
*
*       None
*
**************************************************************************/
static VOID Touchpanel_Tgt_I2C_Tx_Callback(I2C_HANDLE i2c_dev, UINT8  i2c_node_type)
{
    /* Suppress harmless compiler warnings. */
    NU_UNUSED_PARAM(i2c_dev);
    NU_UNUSED_PARAM(i2c_node_type);

    /* Set the event to indicate that the transfer has completed. */
    (VOID)NU_Set_Events(&STMPE811_TGT_I2C_Events, STMPE811_TGT_I2C_TX_DONE_EVENT, NU_OR);
}

/**************************************************************************
* FUNCTION
*
*       Touchpanel_Tgt_I2C_Rx_Callback
*
* DESCRIPTION
*
*       This function informs the application about the completion of data
*       transmission when the node is acting as a transmitter.
*
* INPUTS
*
*       i2c_dev                             Nucleus I2C device handle.
*       i2c_node_type                       Whether the transmitting node
*                                           is an I2C master or a slave.
*       byte_count                          Byte Count
*
* OUTPUTS
*
*       None
*
**************************************************************************/
static VOID Touchpanel_Tgt_I2C_Rx_Callback(I2C_HANDLE  i2c_dev,
                                             UINT8          i2c_node_type,
                                             UNSIGNED_INT   byte_count)
{
    /* Suppress harmless compiler warnings. */
    NU_UNUSED_PARAM(i2c_dev);
    NU_UNUSED_PARAM(i2c_node_type);
    NU_UNUSED_PARAM(byte_count);

    /* Set the event to indicate that the transfer has completed. */
    (VOID)NU_Set_Events(&STMPE811_TGT_I2C_Events, STMPE811_TGT_I2C_RX_DONE_EVENT, NU_OR);
}

/**************************************************************************
* FUNCTION
*
*       Touchpanel_Tgt_I2C_Error_Callback
*
* DESCRIPTION
*
*       This function is called by Nucleus I2C to notify the application
*       of the error encountered. This service is called from the LISR
*       environment.
*
* INPUTS
*
*       i2c_handle                          I2C Handle
*       error_code                          Error Code status
*
* OUTPUTS
*
*       None
*
**************************************************************************/
static VOID Touchpanel_Tgt_I2C_Error_Callback(I2C_HANDLE i2c_handle, STATUS error_code)
{
    /* Suppress harmless compiler warnings. */
    NU_UNUSED_PARAM(i2c_handle);
    NU_UNUSED_PARAM(error_code);

    /* Set the event to indicate that an error has occurred. */
    (VOID)NU_Set_Events(&STMPE811_TGT_I2C_Events, STMPE811_TGT_I2C_ERROR_EVENT, NU_OR);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_X_Coordinate_Adjustment
*
*   DESCRIPTION
*
*       This function performs nonlinear adjustment along X-coordinate
*       of the touch panel. This function is being called by the generic
*       code to do the necessary adjustments.
*
*   INPUTS
*
*       xpos                                Pointer to X-coordinate position
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID Touchpanel_Tgt_X_Coordinate_Adjustment(INT16 *xpos)
{
    /* This function is empty for this platform. */
    NU_UNUSED_PARAM(xpos);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Y_Coordinate_Adjustment
*
*   DESCRIPTION
*
*       This function performs nonlinear adjustment along Y-coordinate
*       of the touch panel. This function is being called by the generic
*       code to do the necessary adjustments.
*
*   INPUTS
*
*       ypos                                Pointer to Y-coordinate position
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID Touchpanel_Tgt_Y_Coordinate_Adjustment(INT16 *ypos)
{
    /* This function is empty for this platform. */
    NU_UNUSED_PARAM(ypos);
}

/*************************************************************************
* FUNCTION
*
*       Touchpanel_Tgt_I2C_Initialize
*
* DESCRIPTION
*
*       This function initializes the I2C controller.
*
* INPUTS
*
*       TOUCHPANEL_INSTANCE_HANDLE *i_handle    - Instance handle
*
* OUTPUTS
*
*       STATUS              status              - NU_SUCCESS
*
*************************************************************************/
static STATUS  Touchpanel_Tgt_I2C_Initialize(TOUCHPANEL_INSTANCE_HANDLE* inst_info)
{
    TOUCHPANEL_TGT          *tgt_handle = (TOUCHPANEL_TGT *) STMPE811_Info->touchpanel_reserved;
    STATUS                  status;
    I2C_INIT                i2c_init;

    /* Prepare data structures to do initialization of I2C. */

    /* Setup the memory pool pointer for usage with Nucleus I2C stack. */
    i2c_init.i2c_memory_pool = &System_Memory;

    /* Specify the baud rate for I2C master device at 400KHz */
    i2c_init.i2c_baudrate    = I2C_DEFAULT_BAUDRATE;

    /* Set the buffer sizes. */
    i2c_init.i2c_tx_buffer_size = 16;
    i2c_init.i2c_rx_buffer_size = 16;

    /* If I2C is being used in interrupt mode. */
    if(tgt_handle->i2c_mode == 1)
    {
        /* Set interrupt mode. */
        i2c_init.i2c_driver_mode = I2C_INTERRUPT_MODE;

        /* Set call backs for interrupt mode I2C transfer operation. */
        i2c_init.i2c_app_cb.i2c_data_indication       = Touchpanel_Tgt_I2C_Rx_Callback;
        i2c_init.i2c_app_cb.i2c_transmission_complete = Touchpanel_Tgt_I2C_Tx_Callback;
    }
    else
    {
        /* Set polling mode. */
        i2c_init.i2c_driver_mode = I2C_POLLING_MODE;

        /* Set call backs for interrupt mode I2C transfer operation. */
        i2c_init.i2c_app_cb.i2c_data_indication       = NU_NULL;
        i2c_init.i2c_app_cb.i2c_transmission_complete = NU_NULL;
    }

    i2c_init.i2c_app_cb.i2c_address_indication    = NU_NULL;
    i2c_init.i2c_app_cb.i2c_ack_indication        = NU_NULL;
    i2c_init.i2c_app_cb.i2c_error                 = Touchpanel_Tgt_I2C_Error_Callback;

    /* Set the slave node type and the slave address. By default the node
       will have the functionality of the master as well as slave. */
    i2c_init.i2c_node_address.i2c_slave_address   = tgt_handle->i2c_addr;
    i2c_init.i2c_node_address.i2c_address_type    = I2C_7BIT_ADDRESS;
    i2c_init.i2c_node_address.i2c_node_type       = I2C_MASTER_SLAVE;

    i2c_init.i2c_active_mode = I2C_MASTER_NODE;

    tgt_handle->i2c_node = i2c_init.i2c_node_address;

    /* Start the specified Nucleus I2C device. */
    status = NU_I2C_Open (NU_NULL, &tgt_handle->i2c_handle, &i2c_init);

    /* Check to see whether Nucleus I2C is already initialized. */
    if (status == I2C_DEV_ALREADY_INIT)
    {
        status = NU_SUCCESS;
    }

    /* Return completion status. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       Touchpanel_Tgt_Read_Register
*
* DESCRIPTION
*
*       This function reads a register from the touch panel
*
* INPUTS
*
*       TOUCHPANEL_TGT *tgt_handle          - Handle to structure specific
*                                             to this touchpanel.
*       UINT32 reg                          - Register to read
*       UINT8  *data                        - Returned read data
*
* OUTPUTS
*
*       STATUS status                       - NU_SUCCESS or error codes
*
*************************************************************************/
static STATUS Touchpanel_Tgt_Read_Register (TOUCHPANEL_TGT *tgt_handle, UINT32 reg, UINT8* data, UINT8 count)
{
    STATUS      status;
    UINT8       tx_dat;
    UNSIGNED    ret_events;

    /* Set register address */
    tx_dat = reg;

#if !(NU_I2C_SUPPORT_POLLING_MODE)
        /* I2C Interrupt Mode is enable */
        I2C_APP_CALLBACKS    Touchpanel_callbacks;
    
        /* Setup callback routines. */
        Touchpanel_callbacks.i2c_data_indication         = Touchpanel_Tgt_I2C_Rx_Callback;
        Touchpanel_callbacks.i2c_transmission_complete     = Touchpanel_Tgt_I2C_Tx_Callback;
        Touchpanel_callbacks.i2c_error                     = Touchpanel_Tgt_I2C_Error_Callback;
        Touchpanel_callbacks.i2c_address_indication      = NU_NULL;
        Touchpanel_callbacks.i2c_ack_indication          = NU_NULL;
    
        status = NU_I2C_Master_Set_Callbacks(tgt_handle->i2c_handle, tgt_handle->i2c_addr, &Touchpanel_callbacks);
#endif

    /* Write to I2C bus. */
    status = NU_I2C_Master_Write (tgt_handle->i2c_handle, tgt_handle->i2c_node, &tx_dat, 1);

    if((status == NU_SUCCESS) && (tgt_handle->i2c_mode == 1))
    {
        /* Wait while the transfer completes. */
        status = NU_Retrieve_Events(&STMPE811_TGT_I2C_Events,
                                    STMPE811_TGT_I2C_TX_DONE_EVENT | STMPE811_TGT_I2C_ERROR_EVENT,
                                    NU_OR_CONSUME, &ret_events, NU_SUSPEND);
    }

    if(status == NU_SUCCESS)
    {
        /* Read data from I2C bus. */
        status = NU_I2C_Master_Read (tgt_handle->i2c_handle, tgt_handle->i2c_node, data, count);
    }

    if((status == NU_SUCCESS) && (tgt_handle->i2c_mode == 1))
    {
        /* Wait while the transfer completes. */
        status = NU_Retrieve_Events(&STMPE811_TGT_I2C_Events,
                                    STMPE811_TGT_I2C_RX_DONE_EVENT | STMPE811_TGT_I2C_ERROR_EVENT,
                                    NU_OR_CONSUME, &ret_events, NU_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* Get the received data. */
            status = NU_I2C_Receive_Data(tgt_handle->i2c_handle, data, count);
        }
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       Touchpanel_Tgt_Write_Register
*
* DESCRIPTION
*
*       This function writes a register to the touch panel
*
* INPUTS
*
*       TOUCHPANEL_TGT *tgt_handle          - Handle to structure specific
*                                             to this touchpanel.
*       UINT32 reg                          - Register to write
*       UINT8  data                         - Data to write
*
* OUTPUTS
*
*       STATUS status                       - NU_SUCCESS or error codes
*
*************************************************************************/
static STATUS Touchpanel_Tgt_Write_Register (TOUCHPANEL_TGT *tgt_handle, UINT32 reg, UINT8 dat)
{
    STATUS      status;
    UINT8       data[2];
    UNSIGNED    ret_events;
    
#if !(NU_I2C_SUPPORT_POLLING_MODE)
    /* I2C Interrupt Mode is enable */
    I2C_APP_CALLBACKS   Touchpanel_callbacks;

    /* Setup callback routines. */
    Touchpanel_callbacks.i2c_data_indication         = Touchpanel_Tgt_I2C_Rx_Callback;
    Touchpanel_callbacks.i2c_transmission_complete   = Touchpanel_Tgt_I2C_Tx_Callback;
    Touchpanel_callbacks.i2c_error                   = Touchpanel_Tgt_I2C_Error_Callback;
    Touchpanel_callbacks.i2c_address_indication      = NU_NULL;
    Touchpanel_callbacks.i2c_ack_indication          = NU_NULL;

    status = NU_I2C_Master_Set_Callbacks(tgt_handle->i2c_handle, tgt_handle->i2c_addr, &Touchpanel_callbacks);
#endif

    data[0] = (UINT8)reg;   /* Register address */
    data[1] = dat;

    status = NU_I2C_Master_Write (tgt_handle->i2c_handle, tgt_handle->i2c_node, data, 2);

    if((status == NU_SUCCESS) && (tgt_handle->i2c_mode == 1))
    {
        /* Wait while the transfer completes. */
        status = NU_Retrieve_Events(&STMPE811_TGT_I2C_Events,
                                    STMPE811_TGT_I2C_TX_DONE_EVENT | STMPE811_TGT_I2C_ERROR_EVENT,
                                    NU_OR_CONSUME, &ret_events, NU_SUSPEND);

    }
    return (status);
}

