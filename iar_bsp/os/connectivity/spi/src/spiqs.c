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
*       spiqs.c
*
* COMPONENT
*
*       SPIQ - Nucleus SPI Queue Management
*
* DESCRIPTION
*
*       This file contains the support routines for Nucleus SPI Queue
*       Management component.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPIQS_Put_In_Buffer                 Buffers Tx data of a transfer
*                                           request into the buffer of
*                                           the specified queue.
*
*       SPIQS_Update_Buffer_State           Updates buffer state after
*                                           a transfer request has been
*                                           completely processed.
*
*       SPIQS_Copy_To_Buffer                Copies the Tx data of the
*                                           specified transfer request
*                                           into the specified buffer.
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
*       SPIQS_Put_In_Buffer
*
* DESCRIPTION
*
*       This function buffers transmission data associated with a
*       transfer request into the buffer of the specified queue.
*
* INPUTS
*
*      *queue                               Pointer to the SPI queue.
*
*      *request                             Pointer to the transfer
*                                           request being processed.
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       SPI_BUFFER_NOT_ENOUGH               The buffer does not have
*                                           enough free space for the
*                                           specified data.
*
*************************************************************************/
STATUS  SPIQS_Put_In_Buffer(SPI_QUEUE      *queue,
                            SPI_REQUEST    *request)
{
    UINT8          *aligned_write_position;
    STATUS          status = NU_SUCCESS;
    UNSIGNED_INT    bytes;
    UNSIGNED_INT    available;
    UNSIGNED_INT    occupied;
    INT             alignment = 0;
    INT             old_level;

    /* Start critical section. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Get the number of transfers currently occupying the buffer. */
    occupied = queue->spi_qbuffer.spi_buff_count;

    /* End critical section. */
    (VOID)NU_Local_Control_Interrupts(old_level);

    /* Check if no transfer is occupying the buffer i.e. the buffer is
       empty. */
    if (occupied == 0)
    {
        /* Yes. Move the write pointer to the start of the buffer. */
        queue->spi_qbuffer.spi_buff_write = queue->spi_qbuffer
                                                  .spi_buff_start;

        /* Setup the write position pointer. */
        aligned_write_position = queue->spi_qbuffer.spi_buff_write;
    }

    /* Buffer is not empty so calculate the write position considering
       data alignment. */
    else
    {
        UINT8      *write_position;
        INT         offset;

        /* Get the current write position. */
        write_position = queue->spi_qbuffer.spi_buff_write;

        /* Calculate the offset of current write position from the start
           of the buffer. */
        offset = write_position - queue->spi_qbuffer.spi_buff_start;

        /* Calculate the required alignment based on the element size of
           the transfer and the offset of current write position. */
        alignment  = (offset % request->spi_element_size)
                             % request->spi_element_size;

        /* Align the writing position pointer. */
        aligned_write_position = write_position + alignment;
    }

    /* Calculate data size in bytes. */
    bytes = request->spi_length * request->spi_element_size;

    /* Calculate the memory space available for buffering new transfers. */
    available = (queue->spi_qbuffer.spi_buff_end -
                 aligned_write_position) + 1U;

    /* Check if the available space in the buffer can hold the data of
       the requested transfer. */
    if ((bytes + alignment) <= available)
    {
        /* Set up the Tx data pointer. */
        queue->spi_qwrite->spi_tx_data = aligned_write_position;

        /* Copy the Tx data to the assigned position in the buffer. */
        SPIQS_Copy_To_Buffer(aligned_write_position, request);

        /* Calculate write position after copying-in the data. */
        aligned_write_position += bytes;

        /* Update the write pointer. */
        queue->spi_qbuffer.spi_buff_write = aligned_write_position;

        /* Start critical section. */
        old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Increment the number of transfers occupying the buffer. */
        queue->spi_qbuffer.spi_buff_count++;

        /* End critical section. */
        (VOID)NU_Local_Control_Interrupts(old_level);
    }

    /* Space available in buffer is not enough. */
    else
    {
        /* Cannot fulfill the request. Set status to indicate that
           available space in buffer is not enough for the request. */
        status = SPI_BUFFER_NOT_ENOUGH;
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       SPIQS_Update_Buffer_State
*
* DESCRIPTION
*
*       This function updates buffer state after a transfer request has
*       been completely processed. This update operation releases the
*       buffer space that was taken up by the Tx data of that transfer
*       request.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device
*                                           control block pointer.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    SPIQS_Update_Buffer_State(SPI_CB *spi_cb)
{
    UNSIGNED_INT    occupied;
    UNSIGNED_INT    bytes;
    SPI_QUEUE       *queue = &spi_cb->spi_queue;
    INT             old_level;

    /* Calculate the number of bytes occupied by the transfer that has
       been completed. */
    bytes = (UNSIGNED_INT)(spi_cb->spi_current_tx_data -
                           queue->spi_qbuffer.spi_buff_read);

    /* Advance the read pointer. */
    queue->spi_qbuffer.spi_buff_read += bytes;

    /* Start critical section. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Decrement the number of transfers occupying the buffer. */
    occupied = --queue->spi_qbuffer.spi_buff_count;

    /* End critical section. */
    (VOID)NU_Local_Control_Interrupts(old_level);

    /* Wrap-around the read pointer if the buffer has become empty. */
    if (occupied == 0)
    {
        /* Move the read pointer to the start of the buffer. */
        queue->spi_qbuffer.spi_buff_read  = queue->spi_qbuffer
                                                  .spi_buff_start;
    }
}

/*************************************************************************
* FUNCTION
*
*       SPIQS_Copy_To_Buffer
*
* DESCRIPTION
*
*       This function copies the Tx data associated with the specified
*       transfer request into the specified buffer.
*
* INPUTS
*
*      *buffer                              The buffer to copy to.
*
*      *request                             The specified transfer request.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    SPIQS_Copy_To_Buffer(UINT8        *buffer,
                             SPI_REQUEST  *request)
{
    UINT8          *data;
    UNSIGNED_INT   length;

    /* Setup a local pointer to the data to be transmitted. */
    data    = request->spi_tx_data;

    /* Get the number of data units to be transmitted. */
    length  = request->spi_length;

    /* Copy the data according to the size of data type. */
    switch (request->spi_element_size)
    {
    case    sizeof(UINT8):

#if         (NU_SPI_SPEED_COPY)

        /* Check if number of data units to be copied is four or
           greater. */
        if (length >= 4U)
        {
            /* Yes. Keep on copying in blocks of four data units as long as
               it is possible. */
            do
            {
                /* Copy four data units. */

                *((UINT8 *)buffer) = *((UINT8 *)data);
                buffer += sizeof(UINT8);
                data   += sizeof(UINT8);

                *((UINT8 *)buffer) = *((UINT8 *)data);
                buffer += sizeof(UINT8);
                data   += sizeof(UINT8);

                *((UINT8 *)buffer) = *((UINT8 *)data);
                buffer += sizeof(UINT8);
                data   += sizeof(UINT8);

                *((UINT8 *)buffer) = *((UINT8 *)data);
                buffer += sizeof(UINT8);
                data   += sizeof(UINT8);

                /* Four out of the specified data units are now copied. */
                length -= 4U;

            } while (length >= 4U);
        }

#endif      /* NU_SPI_SPEED_COPY */

        /* Keep on copying one-by-one till all the data units are
           copied. */
        while (length--)
        {
            /* Copy as an 8-bit integer. */
            *buffer = *data;

            /* Move to the next write location. */

            buffer += sizeof(UINT8);
            data   += sizeof(UINT8);
        }

        break;

    case    sizeof(UINT16):

#if         (NU_SPI_SPEED_COPY)

        /* Check if number of data units to be copied is four or
           greater. */
        if (length >= 4U)
        {
            /* Yes. Keep on copying in blocks of four data units as long as
               it is possible. */
            do
            {
                /* Copy four data units. */

                *((UINT16 *)buffer) = *((UINT16 *)data);
                buffer += sizeof(UINT16);
                data   += sizeof(UINT16);

                *((UINT16 *)buffer) = *((UINT16 *)data);
                buffer += sizeof(UINT16);
                data   += sizeof(UINT16);

                *((UINT16 *)buffer) = *((UINT16 *)data);
                buffer += sizeof(UINT16);
                data   += sizeof(UINT16);

                *((UINT16 *)buffer) = *((UINT16 *)data);
                buffer += sizeof(UINT16);
                data   += sizeof(UINT16);

                /* Four out of the specified data units are now copied. */
                length -= 4U;

            } while (length >= 4U);
        }

#endif      /* NU_SPI_SPEED_COPY */

        /* Keep on copying one-by-one till all the data units are
           copied. */
        while (length--)
        {
            /* Copy as a 16-bit integer. */
            *((UINT16 *)buffer) = *((UINT16 *)data);

            /* Move to the next write location. */

            buffer += sizeof(UINT16);
            data   += sizeof(UINT16);
        }

        break;

    case    sizeof(UINT32):

#if         (NU_SPI_SPEED_COPY)

        /* Check if number of data units to be copied is four or
           greater. */
        if (length >= 4U)
        {
            /* Yes. Keep on copying in blocks of four data units as long as
               it is possible. */
            do
            {
                /* Copy four data units. */

                *((UINT32 *)buffer) = *((UINT32 *)data);
                buffer += sizeof(UINT32);
                data   += sizeof(UINT32);

                *((UINT32 *)buffer) = *((UINT32 *)data);
                buffer += sizeof(UINT32);
                data   += sizeof(UINT32);

                *((UINT32 *)buffer) = *((UINT32 *)data);
                buffer += sizeof(UINT32);
                data   += sizeof(UINT32);

                *((UINT32 *)buffer) = *((UINT32 *)data);
                buffer += sizeof(UINT32);
                data   += sizeof(UINT32);

                /* Four out of the specified data units are now copied. */
                length -= 4U;

            } while (length >= 4U);
        }

#endif      /* NU_SPI_SPEED_COPY */

        /* Keep on copying one-by-one till all the data units are
           copied. */
        while (length--)
        {
            /* Copy as a 32-bit integer. */
            *((UINT32 *)buffer) = *((UINT32 *)data);

            /* Move to the next write location. */

            buffer += sizeof(UINT32);
            data   += sizeof(UINT32);
        }

        break;

    default:

        /* No default case. */
        break;
    }
}

