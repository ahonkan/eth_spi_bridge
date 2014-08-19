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
*       spiqc_transmit.c
*
* COMPONENT
*
*       SPIQ - Nucleus SPI Queue Management
*
* DESCRIPTION
*
*       This file contains the core routines for Nucleus SPI Queue
*       Management component that are involved in transmission.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPIQC_Put_Queue_Tx                  Enqueues the specified
*                                           transfer request.
*
*       SPIQC_Get_Queue_Tx                  Retrieves a data unit from
*                                           the Tx buffer for transmission.
*
* DEPENDENCIES
*
*       spiq_extr.h                         Function prototypes for
*                                           for Nucleus SPI Queue
*                                           Management component.
*
*************************************************************************/
#define     NU_SPI_SOURCE_FILE

#include    "connectivity/spiq_extr.h"

/*************************************************************************
* FUNCTION
*
*       SPIQC_Put_Queue_Tx
*
* DESCRIPTION
*
*       This function enqueues the specified transfer request.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device control
*                                           block pointer.
*
*      *request                             Transfer request to be
*                                           enqueued.
*
*       thread_context                      Boolean variable to indicate
*                                           whether the execution context
*                                           is thread or not.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       SPI_QUEUE_FULL                      Queue is full.
*
*       SPI_BUFFER_NOT_ENOUGH               The buffer does not have
*                                           enough free space for the
*                                           specified data.
*
*************************************************************************/
STATUS  SPIQC_Put_Queue_Tx(SPI_CB      *spi_cb,
                           SPI_REQUEST *request,
                           BOOLEAN      thread_context)
{
    STATUS      status = NU_SUCCESS;
    SPI_QUEUE   *queue;
    UINT16      count;
    INT         old_level;

    /* Setup a pointer to the queue. */
    queue = &(spi_cb->spi_queue);

    /* Check if the execution context is thread. */
    if (thread_context == NU_TRUE)
    {
        /* Protect the critical section against multithread access. */
        NU_Protect(&SPI_Protect_Struct);
    }

    /* Start critical section. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Get current count of the transfers in the queue that have not been
       completed and notified. */
    count = queue->spi_qnotify_count;

    /* End critical section. */
    (VOID)NU_Local_Control_Interrupts(old_level);

    /* Enqueue the transfer request if queue is not full. */
    if (count < queue->spi_qsize)
    {

#if         (NU_SPI_USER_BUFFERING_ONLY == 0)

        /* If it is neither a reception request nor user buffers are being
           used then copy the data to be transmitted into the internal
           buffer. */
        if ((request->spi_transfer_type != SPI_RX) &&
            ((spi_cb->spi_driver_mode & SPI_USER_BUFFERING) == 0))
        {
            /* Copy the specified Tx data into the buffer. */
            status = SPIQS_Put_In_Buffer(queue, request);
        }

        /* Otherwise user buffers are being used. */
        else

#endif      /* NU_SPI_USER_BUFFERING_ONLY == 0 */

        {
            /* Check if request involves transmission
               (Tx or Duplex request). */
            if ((request->spi_transfer_type & SPI_TX) != 0)
            {
                /* Set up the Tx data pointer. */
                queue->spi_qwrite->spi_tx_data = request->spi_tx_data;
            }
        }

        /* Check if request involves reception
           (Rx or Duplex request). */
        if ((request->spi_transfer_type & SPI_RX) != 0)
        {
            /* Set up the Rx buffer pointer. */
            queue->spi_qwrite->spi_rx_buffer = request->spi_rx_buffer;
        }

        /* Setup the number of data units that are to be transferred. */
        queue->spi_qwrite->spi_length = request->spi_length;

        /* Check if pointer setups are OK. */
        if (status == NU_SUCCESS)
        {
            /* Record the target slave address. */
            queue->spi_qwrite->spi_address = request->spi_address;

            /* Record the transfer type of the request. */
            queue->spi_qwrite->spi_transfer_type
                                           = request->spi_transfer_type;

            /* Record the element size of the buffers. */
            queue->spi_qwrite->spi_element_size
                                           = request->spi_element_size;

            /* Increment the write queue pointer to point to the
               next location to write. */
            queue->spi_qwrite++;

            /* Start critical section. */
            old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Increment the count of the messages in queue. */
            queue->spi_qcount++;

            /* Increment the count of the messages for which notification
               needs to be performed. */
            queue->spi_qnotify_count++;

            /* End critical section. */
            (VOID)NU_Local_Control_Interrupts(old_level);

            /* Check if end of the queue has been reached. */
            if (queue->spi_qwrite > queue->spi_qend)
            {
                /* Wrap around to point to the start of the queue. */
                queue->spi_qwrite = queue->spi_qstart;
            }
        }
    }

    /* Queue is full. */
    else
    {
        /* Set status to indicate that queue is full. */
        status = SPI_QUEUE_FULL;
    }

    /* Check if the execution context is thread. */
    if (thread_context == NU_TRUE)
    {
        /* Lift the protection. */
        NU_Unprotect();
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       SPIQC_Get_Queue_Tx
*
* DESCRIPTION
*
*       This function gets a data unit from the queue for transmission
*       from the transfer request that is at front of the queue.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device control
*                                           block pointer.
*
*      *data                                Pointer to the variable that
*                                           will receive the data unit
*                                           retrieved from the queue.
*
*       thread_context                      Boolean variable to indicate
*                                           whether the execution context
*                                           is thread or not.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    SPIQC_Get_Queue_Tx(SPI_CB *spi_cb,
                           UINT32 *data,
                           BOOLEAN thread_context)
{
    SPI_REQUEST    *current_entry;
    SPI_QUEUE      *queue;

    /* Setup a pointer to the queue. */
    queue = &spi_cb->spi_queue;

    /* Check if the execution context is thread. */
    if (thread_context == NU_TRUE)
    {
        /* Protect the critical section against multithread access. */
        NU_Protect(&SPI_Protect_Struct);
    }

    /* Setup a local pointer to the current queue entry being
       processed. */
    current_entry = queue->spi_qread;

    /* Get the data from queue if it is not empty. */
    if (queue->spi_qcount != 0U)
    {
        /* Read a data unit from the buffer according to the size of the
           data type. */
        switch (current_entry->spi_element_size)
        {
        /* 8-bit data. */
        case    sizeof(UINT8):

            /* Get 8-bit data from the buffer. */
            *data = (UINT32)(*(spi_cb->spi_current_tx_data));

            /* Move the data pointer to next location. */
            spi_cb->spi_current_tx_data += sizeof(UINT8);

            break;

        /* 16-bit data. */
        case    sizeof(UINT16):

            /* Get 16-bit data from the buffer. */
            *data = (UINT32)(*((UINT16 *)spi_cb->spi_current_tx_data));

            /* Move the data pointer to next location. */
            spi_cb->spi_current_tx_data += sizeof(UINT16);

            break;

        /* 32-bit data. */
        case    sizeof(UINT32):

            /* Get 32-bit data from the buffer. */
            *data = (*((UINT32 *)spi_cb->spi_current_tx_data));

            /* Move the data pointer to next location. */
            spi_cb->spi_current_tx_data += sizeof(UINT32);

            break;

        default:

            /* No default case. */
            break;
        }

        /* Check if the transfer type is transmission. */
        if (current_entry->spi_transfer_type == SPI_TX)
        {
            /* Decrement the active transfer length as one data unit has
               been retrieved from buffer for transmission. */
            spi_cb->spi_current_length--;
        }
    }

    /* Check if the execution context is thread. */
    if (thread_context == NU_TRUE)
    {
        /* Release the protection. */
        NU_Unprotect();
    }
}

