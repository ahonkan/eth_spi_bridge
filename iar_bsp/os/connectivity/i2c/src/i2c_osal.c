/*************************************************************************
*
*                  Copyright 2006 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       i2c_osal.c
*
* COMPONENT
*
*       I2C OSAL - I2C OS Abstraction
*
* DESCRIPTION
*
*       This file contains the functions used to abstract OS functionality
*       used by Nucleus I2C.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       I2C_OSAL_Allocate_Resources         Function to allocate resources
*                                           for Nucleus I2C.
*
*       I2C_OSAL_Deallocate_Resources       Function to deallocate
*                                           resources assigned to
*                                           Nucleus I2C.
*
*       I2C_OSAL_Create_Handler             Function to create notification
*                                           handler for Nucleus I2C.
*
* DEPENDENCIES
*
*       i2c_extr.h                          Function prototypes for
*                                           Nucleus I2C services.
*
*       i2c_osal_extr.h                     Function prototypes for
*                                           Nucleus I2C OS abstraction
*                                           services.
*
*************************************************************************/
#define     NU_I2C_SOURCE_FILE

#include    "connectivity/i2c_extr.h"
#include    "connectivity/i2c_osal_extr.h"

/* Define external inner-component global data references. */
extern      NU_HISR   I2C_Handler_Block;

/*************************************************************************
* FUNCTION
*
*       I2C_OSAL_Allocate_Resources
*
* DESCRIPTION
*
*       This function is responsible for allocating the OS and system
*       resources needed by Nucleus I2C.
*
* INPUTS
*
*      *i2c_init                            Nucleus I2C initialization
*                                           block pointer
.
*
*     **i2c_cb_ptr                          Pointer to Nucleus I2C control
*                                           block pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                              Service completed successfully.
*
*************************************************************************/
STATUS  I2C_OSAL_Allocate_Resources   (I2C_INIT  *i2c_init,
                                       I2C_CB   *i2c_cb)
{
    NU_MEMORY_POOL     *mem;
    STATUS              status;

    /* Get the memory pool from which the memory will be allocated. */
    mem = i2c_init->i2c_memory_pool;


    /* Set buffer start pointers to null. */

    i2c_cb->i2c_io_buffer.i2cbm_tx_buffer.i2cbm_data_start =
        NU_NULL;
    i2c_cb->i2c_io_buffer.i2cbm_rx_buffer.i2cbm_data_start =
        NU_NULL;

    /* Allocate the memory for default callback functions. */
    status = NU_Allocate_Aligned_Memory(mem, (VOID **)&i2c_cb->i2c_ucb.callback,
                                        sizeof(I2C_APP_CALLBACKS), sizeof(INT), NU_NO_SUSPEND);

#if ( !NU_I2C_SUPPORT_POLLING_MODE )
    /* Check if the node will be using the buffers for transmission. */
    if (status == NU_SUCCESS)
    {
        if (i2c_init->i2c_tx_buffer_size > 0)
        {
            /* Allocate memory for the Nucleus I2C output buffer. */
            status = NU_Allocate_Aligned_Memory(mem,
                (VOID **)&(i2c_cb->
                i2c_io_buffer.i2cbm_tx_buffer.i2cbm_data_start),
                i2c_init->i2c_tx_buffer_size, sizeof(INT), NU_NO_SUSPEND);
        }

        /* Proceed if memory allocation was successful. */
        if (status == NU_SUCCESS)
        {
            /* Allocate memory for the Nucleus I2C input buffer. */
            if (i2c_init->i2c_rx_buffer_size > 0)
            {
                /* Allocate memory for the Nucleus I2C input buffer. */
                status = NU_Allocate_Aligned_Memory(mem,
                    (VOID **)&i2c_cb->
                    i2c_io_buffer.i2cbm_rx_buffer.i2cbm_data_start,
                    i2c_init->i2c_rx_buffer_size, sizeof(INT), NU_NO_SUSPEND);
            }
        }
    }
#endif

    /* Check if all allocations were successful. */
    if (status != NU_SUCCESS)
        /* Deallocate the resources that were allocated. */
        (VOID)I2C_OSAL_Deallocate_Resources(i2c_cb);


    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2C_OSAL_Deallocate_Resources
*
* DESCRIPTION
*
*       This function deallocates the resources allocated to Nucleus I2C.
*
* INPUTS
*
*      *i2c_cb                              Nucleus I2C control block
*                                           pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS  I2C_OSAL_Deallocate_Resources (I2C_CB *i2c_cb)
{
    STATUS      status = NU_SUCCESS;

#if ( !NU_I2C_SUPPORT_POLLING_MODE )
    /* Check if memory was allocated to the input buffer. */
    if (i2c_cb->i2c_io_buffer.i2cbm_rx_buffer.i2cbm_data_start != NU_NULL)
    {
        /* Deallocate the memory allocated to input buffer. */
        status = NU_Deallocate_Memory((VOID *)i2c_cb->i2c_io_buffer.
                                       i2cbm_rx_buffer.i2cbm_data_start);
    }

    /* Check if memory was allocated to the output buffer. */
    if (i2c_cb->i2c_io_buffer.i2cbm_tx_buffer.i2cbm_data_start
        != NU_NULL)
    {
        /* Deallocate the memory allocated to output buffer. */
        status |= NU_Deallocate_Memory((VOID *)i2c_cb->i2c_io_buffer.
                                       i2cbm_tx_buffer.i2cbm_data_start);
    }
#endif

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       I2C_OSAL_Create_Handler
*
* DESCRIPTION
*
*       This function is responsible for creating notification handler
*       for Nucleus I2C.
*
* INPUTS
*
*      *mem                                 Pointer to the memory pool
*                                           used by Nucleus I2C.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS  I2C_OSAL_Create_Handler       (NU_MEMORY_POOL *mem)
{
    VOID       *mem_ptr = NU_NULL;
    STATUS      status;

    /* Allocate memory for I2C transmission handler. */
    status = NU_Allocate_Aligned_Memory(mem, &mem_ptr,
                                I2C_HANDLER_MEM_STACK, sizeof(INT), NU_NO_SUSPEND);

    /* Check if memory has been allocated successfully. */
    if (status == NU_SUCCESS)
    {
        /* Create HISR for callback services. */
        status = NU_Create_HISR (&I2C_Handler_Block, "I2C",
                            I2C_Handler_Entry, I2C_HANDLER_PRIORITY,
                            mem_ptr, I2C_HANDLER_MEM_STACK);

        /* Check if previous service failed. */
        if (status != NU_SUCCESS)
        {
            /* Deallocate system resources. */
            status = NU_Deallocate_Memory(mem_ptr);
        }
    }

    /* Return the completion status of the service. */
    return (status);
}
