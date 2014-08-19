/**************************************************************************
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       lwspi_common.c
*
* COMPONENT
*
*       Lightweight SPI generic driver
*
* DESCRIPTION
*
*       This file contains the generic SPI functions.
*
* FUNCTIONS
*
*       LWSPI_Get_Target_Info
*       LWSPI_Set_Reg_Path
*       LWSPI_PR_Intr_Enable
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"


#define   LWSPI_HISR_Entry_ENTRY      
#define   LWSPI_HISR_Entry_EXIT       


#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
/* External function prototypes. */
extern  VOID            LWSPI_TGT_LISR(INT vector);
extern  VOID            LWSPI_TGT_Intr_Enable(LWSPI_INSTANCE_HANDLE* );

/* Local function prototypes */
static  VOID            LWSPI_HISR_Entry(VOID);
static  STATUS          LWSPI_ISR_Data_Get(LWSPI_INSTANCE_HANDLE **);

/* Local variables */
/* Queue used to hold ISR data. */
static  LWSPI_INSTANCE_HANDLE * LWSPI_ISR_Queue[LWSPI_ISR_QUEUE_SIZE];
static  INT                     LWSPI_ISR_Queue_Read;
static  INT                     LWSPI_ISR_Queue_Write;
static  INT                     LWSPI_ISR_Queue_Status;

/* Data for single HISR used by all instances */
static  NU_HISR                 LWSPI_HISR_Cb;
static  INT                     LWSPI_HISR_Ref_Count;
static  UINT8                   LWSPI_HISR_Stack[LWSPI_HISR_STACK_SIZE];
#endif  /* CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE */

/**************************************************************************
* FUNCTION
*
*       LWSPI_Get_Target_Info
*
* DESCRIPTION
*
*       This function gets target attributes from Registry.
*
* INPUTS
*
*       key                                 Registry key.
*       inst_ptr                            Pointer to spi instance handle.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
**************************************************************************/
STATUS LWSPI_Get_Target_Info(const CHAR * key, LWSPI_INSTANCE_HANDLE *inst_ptr)
{
    STATUS reg_status;

    /* Get the base io address. */
    reg_status = REG_Get_UINT32_Value (key,
                                        "/tgt_settings/io_addr",
                                        &(inst_ptr->io_addr));
    if(reg_status == NU_SUCCESS)
    {
        /* Get the clock. */
        reg_status = REG_Get_UINT32_Value (key,
                                            "/tgt_settings/clock",
                                            &(inst_ptr->clock));
        if(reg_status == NU_SUCCESS)
        {
            /* Get the interrupt vector. */
            reg_status = REG_Get_UINT32_Value (key,
                                            "/tgt_settings/intr_vector",
                                            (UINT32 *)&(inst_ptr->intr_vector));
            if(reg_status == NU_SUCCESS)
            {
                /* Get the interrupt priority. */
                reg_status = REG_Get_UINT32_Value (key,
                                            "/tgt_settings/intr_priority",
                                            (UINT32 *)&(inst_ptr->intr_priority));
                if(reg_status == NU_SUCCESS)
                {
                    /* Get the interrupt type. */
                    reg_status = REG_Get_UINT32_Value (key,
                                            "/tgt_settings/intr_type",
                                            (UINT32 *)&(inst_ptr->intr_type));
                    if(reg_status == NU_SUCCESS)
                    {
                        if(reg_status == NU_SUCCESS)
                        {
                            /* Get the reference clock. */
                            reg_status = REG_Get_String_Value (key,
                                            "/tgt_settings/ref_clock",
                                            &(inst_ptr->ref_clock[0]),
                                            NU_DRVR_REF_CLOCK_LEN);
                        }
                    }
                }
            }
        }
    }

    return (reg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       LWSPI_Set_Reg_Path
*
*   DESCRIPTION
*
*       This function copies registry path to device handle.
*
*   INPUTS
*
*       key                                 Registry path.
*       inst_ptr                            pointer to spi instance handle
*                                           structure.
*
*   OUTPUTS
*
*       SPI_DEV_INVLD_REG_PATH              Registrt patg to be saved is
*                                           invalid.
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
*************************************************************************/
STATUS LWSPI_Set_Reg_Path(const CHAR * key, LWSPI_INSTANCE_HANDLE *inst_ptr)
{
    STATUS     status;

    /* Initialize local variables. */
    status = SPI_DEV_INVLD_REG_PATH;

    /* Save registry path in instance handle for later use. */
    if((strlen(key)+1) <= REG_MAX_KEY_LENGTH)
    {
        strcpy(inst_ptr->reg_path, key);
        status = NU_SUCCESS;
    }

    return (status);
}

#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
/**************************************************************************
* FUNCTION
*
*       LWSPI_PR_Intr_Enable
*
* DESCRIPTION
*
*       This function initializes processor-level interrupt.
*
* INPUTS
*
*       sess_handle                         Device session handle.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
STATUS LWSPI_PR_Intr_Enable(LWSPI_SESSION_HANDLE *ses_ptr)
{
    LWSPI_INSTANCE_HANDLE   *inst_ptr;
    VOID                    (*old_lisr)(INT);
    STATUS                  status;

    /* Initialize local variables. */
    status      = NU_SUCCESS;
    inst_ptr    = (ses_ptr->inst_ptr);
    
    /* Create a single HISR that handles all
       LWSPI devices */
    if (LWSPI_HISR_Ref_Count == 0)
    {
        /* Create HISR. */
        status = NU_Create_HISR(&LWSPI_HISR_Cb,
                                "SPIHISR",
                                LWSPI_HISR_Entry,
                                LWSPI_HISR_PRIORITY,
                                LWSPI_HISR_Stack,
                                LWSPI_HISR_STACK_SIZE);
    }

    /* Increment number of instances using HISR */
    LWSPI_HISR_Ref_Count++;

    if(status == NU_SUCCESS)
    {
        /* Register the interrupt service routine for LWSPI */
        status = NU_Register_LISR(inst_ptr->intr_vector,
                                  LWSPI_TGT_LISR,
                                  &old_lisr);
        if(status == NU_SUCCESS)
        {
            /* Save session */
            ESAL_GE_ISR_VECTOR_DATA_SET (inst_ptr->intr_vector, (VOID*)inst_ptr);

            /* Enable the SPI interrupt in interrupt controller. */
            (VOID)ESAL_GE_INT_Enable(inst_ptr->intr_vector,
                                     inst_ptr->intr_type,
                                     inst_ptr->intr_priority);
        }
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_HISR_Entry
*
* DESCRIPTION
*
*       This is the entry function for the SPI target driver HISR.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
static VOID LWSPI_HISR_Entry(VOID)
{
    LWSPI_INSTANCE_HANDLE   *spi_inst_ptr = NU_NULL;
    NU_SPI_DEVICE           *spi_device;
    BOOLEAN                 io_complete;
    STATUS                  status;

    LWSPI_HISR_Entry_ENTRY;

    /* Get the instance handle pointer */
    status = LWSPI_ISR_Data_Get(&spi_inst_ptr);

    /* Ensure data was available */
    if (status == NU_SUCCESS)
    {
        /* Execute based on the I/O operation for this device instance */
        switch (spi_inst_ptr->io_operation)
        {
            case LWSPI_IO_OPERATION_READ:

                /* Get SPI slave device pointer */
                spi_device = spi_inst_ptr->curr_read_irp->device;

                /* Call bus function to handle ISR read operations */
                io_complete = spi_device->bus->io_ptrs.isr_read((VOID *)spi_inst_ptr,
                                                                spi_inst_ptr->curr_read_irp);

                break;

            case LWSPI_IO_OPERATION_WRITE:

                /* Get SPI slave device pointer */
                spi_device = spi_inst_ptr->curr_write_irp->device;

                /* Call bus function to handle ISR write operations */
                io_complete = spi_device->bus->io_ptrs.isr_write((VOID *)spi_inst_ptr,
                                                                 spi_inst_ptr->curr_write_irp);

                break;

            case LWSPI_IO_OPERATION_WRITE_READ:

                /* Get SPI slave device pointer */
                spi_device = spi_inst_ptr->curr_read_irp->device;

                /* Call bus function to handle ISR write/read operations */
                io_complete = spi_device->bus->io_ptrs.isr_write_read((VOID *)spi_inst_ptr,
                                                                      spi_inst_ptr->curr_write_irp,
                                                                      spi_inst_ptr->curr_read_irp);

                break;

             default:

                /* Error condition - should never get here */
                io_complete = NU_FALSE;

                break;
        }

        /* See if IO operation is complete */
        if(io_complete == NU_TRUE)
        {
            /* Read/Write operation is complete. */
            NU_Release_Semaphore(&spi_device->async_io_lock);
        }
        else
        {
            /* Only enable interrupt if I/O is not completed. */
            LWSPI_TGT_Intr_Enable(spi_inst_ptr);
        }
    }

    LWSPI_HISR_Entry_EXIT;
    
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_ISR_Data_Set
*
* DESCRIPTION
*
*       This function saves data to a circular queue to allow a LISR to
*       pass data to a HISR
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to LWSPI instance 
*                                           handle.
*
* OUTPUTS
*
*       STATUS
*
**************************************************************************/
STATUS LWSPI_ISR_Data_Set(LWSPI_INSTANCE_HANDLE *spi_inst_ptr)
{
    INT     level;
    STATUS  status = LWSPI_ISR_QUEUE_FULL;


    /* Lock-out interrupts */
    level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* See if queue has space */
    if (LWSPI_ISR_Queue_Status != LWSPI_ISR_QUEUE_FULL)
    {
        /* Set status to success */
        status = NU_SUCCESS;

        /* Put data in queue */
        LWSPI_ISR_Queue[LWSPI_ISR_Queue_Write] = spi_inst_ptr;

        /* Update queue write pointer */
        if (++LWSPI_ISR_Queue_Write == LWSPI_ISR_QUEUE_SIZE)
        {
            /* Set queue write pointer to beginning of queue */
            LWSPI_ISR_Queue_Write = 0;
        }

        /* Determine if queue is full */
        if (LWSPI_ISR_Queue_Write == LWSPI_ISR_Queue_Read)
        {
            /* Show queue is full */
            LWSPI_ISR_Queue_Status = LWSPI_ISR_QUEUE_FULL;
        }
        else
        {
            /* Show queue has data */
            LWSPI_ISR_Queue_Status = LWSPI_ISR_QUEUE_DATA;
        }
    }

    /* Enable interrupts to entry level */
    NU_Local_Control_Interrupts(level);

    /* Activate the HISR to process data in the queue */
    NU_Activate_HISR(&LWSPI_HISR_Cb);

    /* Return status to caller */
    return (status);
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_ISR_Data_Get
*
* DESCRIPTION
*
*       This function retrieves data from a circular queue to allow a LISR to
*       pass data to a HISR
*
* INPUTS
*
*       spi_inst_ptr                        Pointer to LWSPI instance 
*                                           handle.
*
* OUTPUTS
*
*       STATUS
*
**************************************************************************/
static STATUS LWSPI_ISR_Data_Get(LWSPI_INSTANCE_HANDLE **spi_inst_ptr)
{
    INT     level;
    STATUS  status = LWSPI_ISR_QUEUE_EMPTY;

    /* Lock-out interrupts */
    level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* See if queue has data */
    if (LWSPI_ISR_Queue_Status != LWSPI_ISR_QUEUE_EMPTY)
    {
        /* Set status to success */
        status = NU_SUCCESS;

        /* Get data from the queue */
        *spi_inst_ptr = LWSPI_ISR_Queue[LWSPI_ISR_Queue_Read];

        /* Update queue read pointer */
        if (++LWSPI_ISR_Queue_Read == LWSPI_ISR_QUEUE_SIZE)
        {
            /* Set queue read pointer to beginning of queue */
            LWSPI_ISR_Queue_Read = 0;
        }

        /* Determine if queue is empty */
        if (LWSPI_ISR_Queue_Read == LWSPI_ISR_Queue_Write)
        {
            /* Show queue is empty */
            LWSPI_ISR_Queue_Status = LWSPI_ISR_QUEUE_EMPTY;
        }
        else
        {
            /* Show queue has data */
            LWSPI_ISR_Queue_Status = LWSPI_ISR_QUEUE_DATA;
        }
    }

    /* Enable interrupts to entry level */
    NU_Local_Control_Interrupts(level);

    /* Return status to caller */
    return (status);
}
#endif /* #if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE) */

/**************************************************************************
* FUNCTION
*
*       LWSPI_Check_Power_State
*
* DESCRIPTION
*
*       This function checks the current state of device power. If power 
*       is not on then it suspends the caller.
*
* INPUTS
*
*       inst_ptr                            Pointer to instance handle of 
*                                           LWSPI device.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
VOID LWSPI_Check_Power_State(VOID *inst_ptr)
{
    LWSPI_INSTANCE_HANDLE   *spi_dev_inst_ptr;
    PMI_DEV_HANDLE          pmi_dev;
    STATUS                  status;

    spi_dev_inst_ptr = (LWSPI_INSTANCE_HANDLE *)inst_ptr;   
    pmi_dev = (spi_dev_inst_ptr->pmi_dev);

   /* Check current state of the device. If the device is off, suspend on an
    * event until the device power state changes to ON. */
    if ((PMI_STATE_GET(pmi_dev) == SPI_OFF)||(PMI_IS_PARKED(pmi_dev) == NU_TRUE))
    {
        /* Wait until the device is available for an I/O operation. */
        PMI_WAIT_CYCLE(pmi_dev, status);
    }
    
    /* Remove unsed variable 'status' warning. */
    NU_UNUSED_PARAM(status);
}
#endif /* #if (CFG_NU_OS_SVCS_PWR_ENABLE) */
