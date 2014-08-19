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
*       spiq_defs.h
*
* COMPONENT
*
*       SPIQ - Nucleus SPI Queue Management
*
* DESCRIPTION
*
*       This file contains data structures and definitions for Nucleus SPI
*       Queue Management component.
*
* DATA STRUCTURES
*
*       SPI_REQUEST                         Transfer request structure.
*
*       SPI_BUFFER                          Buffer structure for actual
*                                           storage of data associated
*                                           with queued transfer requests.
*
*       SPI_QUEUE                           Queue structure for queuing
*                                           transfer requests.
*
* DEPENDENCIES
*
*       spi.h                               Nucleus SPI API, definitions
*                                           and constants.
*
*************************************************************************/
/* Check to see if the file has been included already. */
#ifndef     SPIQ_DEFS_H
#define     SPIQ_DEFS_H

#include    "connectivity/spi.h"

/* Structure for transfer request. */
typedef struct SPI_REQUEST_STRUCT
{
    UINT8          *spi_tx_data;            /* Pointer to the buffer
                                               containing data to be
                                               transmitted.             */
    UINT8          *spi_rx_buffer;          /* Pointer to the buffer
                                               in which received data
                                               will be stored.          */
    UNSIGNED_INT    spi_length;             /* Number of data units to
                                               be transferred.          */
    UINT16          spi_address;            /* Address of the slave to
                                               which the transfer is
                                               targeted.               */
    UINT8           spi_element_size;       /* Size of the data type
                                               used by the transmit data
                                               and receive buffers.     */
    UINT8           spi_transfer_type;      /* Type of the transfer:
                                               transmission, reception
                                               or duplex transfer.      */

} SPI_REQUEST;

/* Structure for the SPI buffer. */
typedef struct SPI_BUFFER_STRUCT
{
    UINT8          *spi_buff_start;         /* Buffer start pointer.    */
    UINT8          *spi_buff_end;           /* Buffer end pointer.      */
    UINT8          *spi_buff_read;          /* Buffer read pointer.     */
    UINT8          *spi_buff_write;         /* Buffer write pointer.    */
    UNSIGNED_INT    spi_buff_count;         /* Number of buffered
                                               transfers.               */

} SPI_BUFFER;

/* Structure for the SPI queue. */
typedef struct SPI_QUEUE_STRUCT
{
    SPI_REQUEST    *spi_qwrite;             /* Queue write pointer.     */
    SPI_REQUEST    *spi_qread;              /* Queue read pointer.      */
    SPI_REQUEST    *spi_qnotify;            /* Queue read pointer for
                                               notifications.           */
    SPI_REQUEST    *spi_qstart;             /* Queue start pointer.     */
    SPI_REQUEST    *spi_qend;               /* Queue end pointer.       */
    SPI_BUFFER      spi_qbuffer;            /* Buffer for actual storage
                                               of data.                 */
    UINT16          spi_qcount;             /* Queue elements counter.  */
    UINT16          spi_qnotify_count;      /* Queue elements counter for
                                               the elements for which
                                               notification is pending. */
    UINT16          spi_qsize;              /* Queue size in elements.  */

    /* Padding to align the structure. */
    UINT8           spi_padding[2];

} SPI_QUEUE;

#endif      /* !SPIQ_DEFS_H */
