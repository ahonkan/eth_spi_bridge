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
*       spiqc_receive.c
*
* COMPONENT
*
*       SPIQ - Nucleus SPI Queue Management
*
* DESCRIPTION
*
*       This file contains the core routines for Nucleus SPI Queue
*       Management component that are involved in reception.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPIQC_Put_Queue_Rx                  Puts received data in the
*                                           Rx buffer.
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
*       SPIQC_Put_Queue_Rx
*
* DESCRIPTION
*
*       This function puts the received data in the Rx buffer associated
*       with the transfer request being processed.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI control block.
*
*       data                                Data to be placed in the Rx
*                                           buffer.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    SPIQC_Put_Queue_Rx(SPI_CB      *spi_cb,
                           UINT32       data)
{
    SPI_QUEUE      *queue;

    /* Setup a pointer to the queue. */
    queue = &(spi_cb->spi_queue);

    /* Place the data in the buffer depending upon the size. */
    switch (queue->spi_qread->spi_element_size)
    {
    /* 8-bit data. */
    case    sizeof(UINT8):

        /* Save 8-bit data in the buffer. */
        *(spi_cb->spi_current_rx_buffer) = (UINT8)data;

        /* Move the data pointer to next location. */
        spi_cb->spi_current_rx_buffer += sizeof(UINT8);

        break;

    /* 16-bit data. */
    case    sizeof(UINT16):

        /* Save 16-bit data in the buffer. */
        *(((UINT16 *)spi_cb->spi_current_rx_buffer)) = (UINT16)data;

        /* Move the data pointer to next location. */
        spi_cb->spi_current_rx_buffer += sizeof(UINT16);

        break;

    /* 32-bit data. */
    case    sizeof(UINT32):

        /* Save 32-bit data in the buffer. */
        *(((UINT32 *)spi_cb->spi_current_rx_buffer)) = data;

        /* Move the data pointer to next location. */
        spi_cb->spi_current_rx_buffer += sizeof(UINT32);

        break;

    default:

        /* No default case. */
        break;
    }

    /* Decrement the length as one data unit has been transferred. */
    spi_cb->spi_current_length--;
}

