/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
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
*       i2c_tgt_power.c
*
*   COMPONENT
*
*       STM32                               - STM32 controller driver
*
*   DESCRIPTION
*
*       This file contains the Serial Driver specific functions.
*
*   FUNCTIONS
*
*       I2C_Tgt_Pwr_Pre_Park
*       I2C_Tgt_Pwr_Post_Park
*       I2C_Tgt_Pwr_Pre_Resume
*       I2C_Tgt_Pwr_Post_Resume
*       I2C_Tgt_Pwr_Default_State
*       I2C_Tgt_Pwr_Set_State
*       I2C_Tgt_Pwr_On_State
*       I2C_Tgt_Pwr_Off_State
*
*   DEPENDENCIES
*
*       string.h
*       reg_api.h
*       i2c_driver_extr.h
*       i2c_driver_defs.h
*       stm32.h
*       power_core.h
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include <string.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "connectivity/nu_connectivity.h"
#include "bsp/drivers/i2c/stm32/i2c_tgt.h"

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

static VOID   I2C_Tgt_Pwr_On_State(I2C_INSTANCE_HANDLE *stm32_inst_ptr);

static VOID   I2C_Tgt_Pwr_Off_State(I2C_INSTANCE_HANDLE *stm32_inst_ptr);

/* External functions. */

extern STATUS I2C_Tgt_Driver_Set_Baudrate (I2C_INSTANCE_HANDLE *i_handle, UINT16 baud_rate);
extern VOID   I2C_Tgt_Enable_Interrupt(I2C_INSTANCE_HANDLE *i_handle);
extern VOID   I2C_Tgt_Disable_Interrupt(I2C_INSTANCE_HANDLE *i_handle);
extern VOID   I2C_Tgt_Driver_ISR (INT vector);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Pwr_Pre_Park
*
*   DESCRIPTION
*
*       This function is called before park event.
*
*   INPUT
*
*       VOID                *inst_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS           status             - SUCCESS
*
*************************************************************************/
STATUS I2C_Tgt_Pwr_Pre_Park(VOID *instance_handle)
{
    STATUS      status = NU_SUCCESS;
    UINT8       *i2c_base_address = (UINT8*)((I2C_INSTANCE_HANDLE*)instance_handle)->io_addr;
    UINT32      timeout = 10000;

    /* make sure no tx pending */
    do
    {
        status = ESAL_GE_MEM_READ16(i2c_base_address + I2C_SR1);
        timeout--;
        
    } while (((status & I2C_SR1_TXE) == 0) && timeout);

    if(timeout ==0 )
    {
        status = I2C_TRANSFER_IN_PROGRESS;
    }
    
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Pwr_Post_Park
*
*   DESCRIPTION
*
*       This function is called after park event.
*
*   INPUT
*
*       VOID                *inst_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS           status             - SUCCESS
*
*************************************************************************/
STATUS I2C_Tgt_Pwr_Post_Park(VOID *instance_handle)
{
    /* Turn off I2C */
    I2C_Tgt_Pwr_Off_State(instance_handle);

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Pwr_Pre_Resume
*
*   DESCRIPTION
*
*       This function is called before resume event.
*
*   INPUT
*
*       VOID                *inst_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS           status             - SUCCESS
*
*************************************************************************/
STATUS I2C_Tgt_Pwr_Pre_Resume(VOID *instance_handle)
{
    /* Turn on I2C */
    I2C_Tgt_Pwr_On_State(instance_handle);

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Pwr_Post_Resume
*
*   DESCRIPTION
*
*       This function is called after resume event.
*
*   INPUT
*
*       VOID                *inst_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS           status             - SUCCESS
*
*************************************************************************/
STATUS I2C_Tgt_Pwr_Post_Resume(VOID *instance_handle)
{
    STATUS status;
    I2C_INSTANCE_HANDLE *i_handle = (I2C_INSTANCE_HANDLE *)instance_handle;

    /* Set baud rate for the network transfer. */
    status = I2C_Tgt_Driver_Set_Baudrate (instance_handle, i_handle->i2c_cb->i2c_baudrate);

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Pwr_Resume_End
*
*   DESCRIPTION
*
*       This function is called at the end of resume.
*
*   INPUT
*
*       VOID                *inst_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS           status             - NU_SUCCESS
*
*************************************************************************/
STATUS I2C_Tgt_Pwr_Resume_End(VOID *instance_handle)
{
    /* Suppress warning. */
    NU_UNUSED_PARAM(instance_handle);

    return (NU_SUCCESS);
}

#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)) */

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Pwr_Default_State
*
*   DESCRIPTION
*
*       This function sets the device to default state.
*
*   INPUT
*
*       VOID                *inst_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS           status             - SUCCESS
*
*************************************************************************/
VOID I2C_Tgt_Pwr_Default_State(I2C_INSTANCE_HANDLE *i_handle)
{
    /* Disable all I2C interrupts. */
    I2C_Tgt_Disable_Interrupt (i_handle);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Pwr_Set_State
*
*   DESCRIPTION
*
*       This function sets the state of the I2C
*
*   INPUT
*
*       state
*
*   OUTPUT
*
*       NU_SUCCESS
*       NU_PM_Request_Min_OP return value
*
*************************************************************************/
STATUS I2C_Tgt_Pwr_Set_State(VOID *instance_handle, PM_STATE_ID *state)
{
    VOID                (*old_lisr)(INT);
    STATUS              status = NU_SUCCESS;
    I2C_INSTANCE_HANDLE *i_handle = (I2C_INSTANCE_HANDLE *)instance_handle;
    I2C_TGT             *tgt_ptr = (I2C_TGT *)i_handle->i2c_reserved;

    /* Enable I2C only if already OFF */
    if (((*state == I2C_ON) || (*state == POWER_ON_STATE)) && (PMI_STATE_GET(i_handle->pmi_dev) == I2C_OFF))
    {
        /* Call the target specific setup function. */
        if(tgt_ptr->setup_func != NU_NULL)
        {
            tgt_ptr->setup_func();
        }
        
        if (i_handle->device_in_use == NU_TRUE)
        {
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

            /* First ensure that DVFS is at min op before STM32 is enabled */
            status = PMI_REQUEST_MIN_OP(I2C_MIN_DVFS_OP, i_handle->pmi_dev);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

            if (status == NU_SUCCESS)
            {
                 /* Register the interrupt service routine for transfer complete
                   flag interrupt. */
                NU_Register_LISR(i_handle->irq, I2C_Tgt_Driver_ISR, &old_lisr);

                /* Turn on the rest of I2C registers */
                I2C_Tgt_Pwr_On_State(i_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

                /* Update MPL for DVFS */
                (VOID)PMI_DVFS_Update_MPL_Value(i_handle->pmi_dev, PM_NOTIFY_ON);

                /* Set baud rate for the network transfer. */
                I2C_Tgt_Driver_Set_Baudrate (i_handle, i_handle->i2c_cb->i2c_baudrate);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

                /* Set resume flag in event group */
                PMI_CHANGE_STATE_WAIT_CYCLE(i_handle->pmi_dev, PMI_ON_EVT, status);
             }
        }

        /* Device is not in use yet, so just change the state of the device */
        PMI_STATE_SET(i_handle->pmi_dev, I2C_ON);
    }

    /* Disable I2C only if already ON */
    if ((*state == I2C_OFF) && 
       ((PMI_STATE_GET(i_handle->pmi_dev) == POWER_ON_STATE) || (PMI_STATE_GET(i_handle->pmi_dev) == I2C_ON)))
    {
        /* Turn off the rest of I2C registers */
        I2C_Tgt_Pwr_Off_State(i_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

        /* Release min request for DVFS OP */
        (VOID)PMI_RELEASE_MIN_OP(i_handle->pmi_dev);

        /* Update MPL for DVFS */
        (VOID)PMI_DVFS_Update_MPL_Value(i_handle->pmi_dev, PM_NOTIFY_OFF);

        /* Set baud rate for the network transfer. */
        I2C_Tgt_Driver_Set_Baudrate (i_handle, i_handle->i2c_cb->i2c_baudrate);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

        /* Set ON flag in event group */
        PMI_CHANGE_STATE_WAIT_CYCLE(i_handle->pmi_dev, PMI_OFF_EVT, status);

        /* Update the state of the device in the State Information structure */
        PMI_STATE_SET(i_handle->pmi_dev, I2C_OFF);
        
        /* Call the target specific cleanup function. */
        if(tgt_ptr->cleanup_func != NU_NULL)
        {
            tgt_ptr->cleanup_func();
        }
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Pwr_On_State
*
*   DESCRIPTION
*
*       The function to set I2C to "ON" state
*
*   INPUT
*
*       base_address            - Base address for I2C-n controller
*       I2C_controller          - I2C controller number
*
*   OUTPUT
*
*       None
*
*************************************************************************/
static VOID I2C_Tgt_Pwr_On_State(I2C_INSTANCE_HANDLE * i_handle)
{

    I2C_Tgt_Enable_Interrupt(i_handle);
                        

    I2C_Tgt_Driver_Set_Baudrate (i_handle, i_handle->i2c_cb->i2c_baudrate);
 }

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Pwr_Off_State
*
*   DESCRIPTION
*
*       The function to set I2C to "OFF" state
*
*   INPUT
*
*       base_address            - Base address for I2C-n controller
*
*   OUTPUT
*
*       None
*
*************************************************************************/
static VOID I2C_Tgt_Pwr_Off_State(I2C_INSTANCE_HANDLE *i_handle)
{
    /* Disable all I2C interrupts. */
    I2C_Tgt_Disable_Interrupt(i_handle);
}


/*************************************************************************
*
*   FUNCTION
*
*       I2C_Tgt_Pwr_Off_State
*
*   DESCRIPTION
*
*       The function to get clock rate for I2C baud rate calculation
*
*   INPUT
*
*       base_address            - Base address for I2C-n controller
*
*   OUTPUT
*
*       None
*
*************************************************************************/
STATUS I2C_Tgt_Pwr_Get_Clock_Rate(I2C_INSTANCE_HANDLE *i_handle, UINT32 *clock_rate)
{
    STATUS    status;
    UINT8     op_id;
    UINT32    ref_freq;

    /* Get reference clock */
    status = CPU_Get_Device_Frequency(&op_id, i_handle->ref_clock, &ref_freq);

    /* Check if API call was successful. */
    if (status == NU_SUCCESS)
    {
        /* Retrieve MCK, as STM32 baud rate is based on MCK frequency. */
        *clock_rate = ref_freq;
    }
    else
    {
        *clock_rate = ESAL_PR_TMR_OS_CLOCK_RATE;
    }

    return status;
}

#endif /*CFG_NU_OS_SVCS_PWR_ENABLE*/

