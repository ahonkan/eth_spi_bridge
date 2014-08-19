/*************************************************************************
*
*               Copyright 2006 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       spi_osal.c
*
* COMPONENT
*
*       SPI OSAL - OS Abstraction Layer for Nucleus SPI
*
* DESCRIPTION
*
*       This file contains the functions used to abstract OS functionality
*       used by Nucleus SPI.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPI_OSAL_Allocate_Resources         Allocates resources
*                                           for a Nucleus SPI device.
*
*       SPI_OSAL_Deallocate_Resources       Deallocates resources
*                                           assigned to a Nucleus SPI
*                                           device.
*
*       SPI_OSAL_Create_Handler             Creates the Nucleus SPI
*                                           notification handler.
*
*       SPI_OSAL_Allocate_Attribute_List    Allocates resources for
*                                           transfer attributes list
*                                           for an SPI device.
*
*       SPI_OSAL_Allocate_Callbacks_List    Allocates resources for
*                                           the callback list for an
*                                           SPI device.
*
* DEPENDENCIES
*
*       spi_osal_extr.h                     Function prototypes for
*                                           Nucleus SPI OSAL component.
*
*************************************************************************/
#define     NU_SPI_SOURCE_FILE

#include    "connectivity/spi_osal_extr.h"

/* Define external inner-component global data references. */
extern  NU_HISR SPI_Handler;
extern  VOID*   SPI_Handler_Stack_Ptr;
extern  VOID    SPI_Handler_Entry (VOID);

/*************************************************************************
* FUNCTION
*
*       SPI_OSAL_Allocate_Resources
*
* DESCRIPTION
*
*       This function is responsible for allocating the OS and system
*       resources needed by a Nucleus SPI device.
*
* INPUTS
*
*
*     *spi_cb                               Pointer to the Nucleus SPI
*                                           device control block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS  SPI_OSAL_Allocate_Resources(SPI_CB    *spi_cb)
{
    NU_MEMORY_POOL  *mem;
    STATUS          status = NU_SUCCESS;

    /* Get the memory pool from which the memory will be allocated. */
    mem = spi_cb->spi_memory_pool;

    /* Check if memory allocation was successful. */
    if (status == NU_SUCCESS)
    {
        /* Reset the Nucleus SPI queue memory pointer. */
        spi_cb->spi_queue.spi_qstart = NU_NULL;

        /* Reset the Nucleus SPI buffer memory pointer. */
        spi_cb->spi_queue.spi_qbuffer.spi_buff_start = NU_NULL;

        /* Check if interrupt driven mode is specified for the device. */
        if ((spi_cb->spi_driver_mode & SPI_INTERRUPT_MODE) != 0)
        {
            /* Allocate memory for the Nucleus SPI queue. */
            status = NU_Allocate_Aligned_Memory(mem,
                (VOID **)&(spi_cb->spi_queue.spi_qstart),
                ((UNSIGNED)sizeof(SPI_REQUEST) *
                 (UNSIGNED)spi_cb->spi_queue_size), sizeof(INT), NU_NO_SUSPEND);

            /* Proceed with buffer memory allocation if queue memory was
               successfully allocated and user buffers are not being
               used. */
            if ((spi_cb->spi_driver_mode == SPI_INTERRUPT_MODE) &&
                (status == NU_SUCCESS))
            {
                /* Allocate memory for the Nucleus SPI buffer. */
                status = NU_Allocate_Aligned_Memory(mem,
                    (VOID **)&(spi_cb->spi_queue.spi_qbuffer
                                                .spi_buff_start),
                    (UNSIGNED)spi_cb->spi_buffer_size, sizeof(INT), NU_NO_SUSPEND);
            }
        }
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       SPI_OSAL_Deallocate_Resources
*
* DESCRIPTION
*
*       This function de-allocates the resources assigned to the
*       specified Nucleus SPI device.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device control
*                                           block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS  SPI_OSAL_Deallocate_Resources(SPI_CB *spi_cb)
{
    STATUS      status = NU_SUCCESS;

    /* Deallocate the memory allocated to the callback list. */
    if (spi_cb->spi_ucb != NU_NULL)
    {
        status = NU_Deallocate_Memory((VOID *)spi_cb->spi_ucb);
        spi_cb->spi_ucb = NU_NULL;
    }

    /* Deallocate the memory allocated to the transfer attributes list. */
    if (spi_cb->spi_slaves_attribs != NU_NULL)
    {
        status |= NU_Deallocate_Memory((VOID *)spi_cb->spi_slaves_attribs);
        spi_cb->spi_slaves_attribs = NU_NULL;
    }

    /* Deallocate the memory allocated to the buffer if previous
       deallocation was successful. */
    if (spi_cb->spi_queue.spi_qbuffer.spi_buff_start != NU_NULL)
    {
        status |= NU_Deallocate_Memory((VOID *)spi_cb->spi_queue
                                                      .spi_qbuffer
                                                      .spi_buff_start);
        spi_cb->spi_queue.spi_qbuffer.spi_buff_start = NU_NULL;
    }

    /* Deallocate the memory allocated to the queue. */
    if (spi_cb->spi_queue.spi_qstart != NU_NULL)
    {
        status |= NU_Deallocate_Memory((VOID *)spi_cb->spi_queue
                                                      .spi_qstart);
        spi_cb->spi_queue.spi_qstart = NU_NULL;
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       SPI_OSAL_Create_Handler
*
* DESCRIPTION
*
*       This function is responsible for creating notification handler
*       for Nucleus SPI.
*
* INPUTS
*
*      *mem                                 Pointer to the memory pool
*                                           available to Nucleus SPI.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS  SPI_OSAL_Create_Handler  (NU_MEMORY_POOL *mem)
{
    STATUS      status;

    /* Allocate memory for SPI notification handler. */
    status = NU_Allocate_Aligned_Memory(mem, &SPI_Handler_Stack_Ptr,
                                         SPI_HANDLER_MEM_STACK, sizeof(INT), NU_NO_SUSPEND);

    /* Check if memory has been allocated successfully. */
    if (status == NU_SUCCESS)
    {
        /* Create HISR for callback services. */
        status = NU_Create_HISR(&SPI_Handler, "SPI",
                                SPI_Handler_Entry, SPI_HANDLER_PRIORITY,
                                SPI_Handler_Stack_Ptr, SPI_HANDLER_MEM_STACK);

        /* Check if HISR creation failed. */
        if (status != NU_SUCCESS)
        {
            /* Deallocate system resources. */
            (VOID)NU_Deallocate_Memory(SPI_Handler_Stack_Ptr);
        }
    }

    /* Return the completion status of the service. */
    return (status);
}


/*************************************************************************
* FUNCTION
*
*       SPI_OSAL_Delete_Handler
*
* DESCRIPTION
*
*       This function is responsible for deleting notification handler
*       for Nucleus SPI.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID  SPI_OSAL_Delete_Handler  (VOID)
{
     /* Create HISR for callback services. */
    (VOID)NU_Delete_HISR(&SPI_Handler);

    /* Allocate memory for SPI notification handler. */
    (VOID)NU_Deallocate_Memory(SPI_Handler_Stack_Ptr);

}


/*************************************************************************
* FUNCTION
*
*       SPI_OSAL_Allocate_Attribute_List
*
* DESCRIPTION
*
*       This function is responsible for allocating memory for the
*       transfer attributes list for the specified SPI device.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device control
*                                           block pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS  SPI_OSAL_Allocate_Attribute_List(SPI_CB    *spi_cb)
{
    STATUS          status;
    NU_MEMORY_POOL  *mem;

    /* Get the memory pool from which the memory will be allocated. */
    mem = spi_cb->spi_memory_pool;

    /* Allocate the memory for a Nucleus SPI device control block. */
    status = NU_Allocate_Aligned_Memory(mem,
                             (VOID **)&spi_cb->spi_slaves_attribs,
                             ((UNSIGNED)sizeof(SPI_TRANSFER_CONFIG) *
                              (UNSIGNED)spi_cb->spi_slaves_count), sizeof(INT), NU_NO_SUSPEND);

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       SPI_OSAL_Allocate_Callbacks_List
*
* DESCRIPTION
*
*       This function is responsible for allocating memory for the
*       callback list for the specified SPI device.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device control
*                                           block pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS  SPI_OSAL_Allocate_Callbacks_List(SPI_CB    *spi_cb)
{
    STATUS          status;
    NU_MEMORY_POOL  *mem;

    /* Get the memory pool from which the memory will be allocated. */
    mem = spi_cb->spi_memory_pool;

    /* Allocate the memory for a Nucleus SPI device control block. */
    status = NU_Allocate_Aligned_Memory(mem,
                             (VOID **)&spi_cb->spi_ucb,
                             ((UNSIGNED)sizeof(SPI_APP_CALLBACKS) *
                              (UNSIGNED)spi_cb->spi_slaves_count), sizeof(INT), NU_NO_SUSPEND);

    /* Return the completion status of the service. */
    return (status);
}
