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
*       spicc_transfer.c
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains the service that is responsible for data exchange
*       like Rx, Tx and Duplex.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPICC_Transfer                      Service for data exchange in 
*                                           Rx, Tx and Duplex mode with 
*                                           specified element size.
*
* DEPENDENCIES
*
*       spic_extr.h                         Function prototypes of
*                                           Nucleus SPI Core Services
*                                           component.
*
*************************************************************************/
#define     NU_SPI_SOURCE_FILE

#include    "connectivity/spic_extr.h"

/*************************************************************************
* FUNCTION
*
*       SPICC_Transfer
*
* DESCRIPTION
*
*       This function is responsible for data transfers, either in 
*       Tx, Rx or Duplex mode, in specified number of elements.
*       
*
* INPUTS
*
*       spi_dev                             Handle to the SPI device to
*                                           be used in this transfer.
*
*       address                             Specifies the slave with which
*                                           this transfer should take
*                                           place. If the device itself
*                                           is a slave then this parameter
*                                           is ignored.
*
*      *tx_data                             Pointer to an array which
*                                           contains data units to be
*                                           transmitted.
*
*      *rx_buffer                           The buffer that will get the
*                                           received data units.
*
*       length                              Number of data units to be
*                                           transferred.
*       element_size                        Size of each data unit.
*       transfer_type                       Transfer type, which can be 
*                                           any of following.
*                                           1. SPI_TX
*                                           2. SPI_RX
*                                           3. SPI_DUPLEX
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       SPI_INVALID_HANDLE                  Nucleus SPI device handle is
*                                           not valid.
*
*       SPI_INVALID_ADDRESS                 Specified slave address is not
*                                           valid for the specified SPI
*                                           device.
*
*       SPI_QUEUE_FULL                      Queue is full.
*
*       SPI_BUFFER_NOT_ENOUGH               The buffer does not have
*                                           enough free space for the
*                                           specified data.
*
*************************************************************************/
STATUS  SPICC_Transfer(SPI_HANDLE           spi_dev,
                              UINT16        address,
                              VOID          *tx_data,
                              VOID          *rx_buffer,
                              UNSIGNED_INT  length,
                              UINT8         element_size,
                              UINT8         transfer_type)
{
    STATUS          status;
    SPI_REQUEST     request;

    /* Build a transfer request structure for internal processing. */
    request.spi_address         = address;
    request.spi_tx_data         = (UINT8*) tx_data;
    request.spi_rx_buffer       = (UINT8*) rx_buffer;
    request.spi_length          = length;
    request.spi_element_size    = element_size;
    request.spi_transfer_type   = transfer_type;

    /* Call the internal routine for transfer processing. */
    status = SPICC_Process_Request(spi_dev, &request);

    /* Return the completion status of the service. */
    return (status);
}

