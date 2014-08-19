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
*       spicc_process.c
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains the routines responsible for processing
*       of data transfer requests.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPICC_Process_Request               Process a transfer request.
*
*       SPICC_Transfer_Interrupt            Processes a transfer request
*                                           for devices in interrupt-
*                                           driven driver mode.
*
*       SPICC_Transfer_Polled               Processes a transfer request
*                                           for devices in polling
*                                           driver mode.
*
* DEPENDENCIES
*
*       spic_extr.h                         Function prototypes of the
*                                           Nucleus SPI Core Services
*                                           component.
*
*       spiq_extr.h                         Function prototypes for
*                                           for Nucleus SPI Queue
*                                           Management component.
*
*************************************************************************/
#define     NU_SPI_SOURCE_FILE

#include    "connectivity/spic_extr.h"
#include    "connectivity/spiq_extr.h"

/*************************************************************************
* FUNCTION
*
*       SPICC_Process_Request
*
* DESCRIPTION
*
*       This function processes the specified transfer request.
*
* INPUTS
*
*       spi_dev                             Handle to the SPI device to
*                                           be used in this transfer.
*
*      *request                             Pointer to SPI transfer
*                                           request.
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
*       SPI_INVALID_PARAM_POINTER           Null given instead of a
*                                           variable pointer.
*
*       SPI_ELEMENT_SIZE_NOT_ENOUGH         The buffer elements cannot
*                                           hold data units of current
*                                           transfer size.
*
*       SPI_QUEUE_FULL                      Queue is full.
*
*       SPI_BUFFER_NOT_ENOUGH               The buffer does not have
*                                           enough free space for the
*                                           specified data.
*
*************************************************************************/
STATUS  SPICC_Process_Request(SPI_HANDLE    spi_dev,
                              SPI_REQUEST  *request)
{
    SPI_CB      *spi_cb;
    STATUS      status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if device handle is valid and get the port control block
       and device control block. */
    status = SPICS_Get_CBs(spi_dev, &spi_cb);

#if         (NU_SPI_ERROR_CHECKING)

    /* Check if control blocks retrieved successfully. */
    if (status == NU_SUCCESS)
    {
        /* Check the specified address for validity. */
        status = SPICS_Check_Address(spi_cb, request->spi_address);

        /* Check if specified address is found to be valid. */
        if (status == NU_SUCCESS)
        {
            /* If transfer involves transmission, check the Tx data
               pointer for validity. */
            if (((request->spi_transfer_type & SPI_TX) != 0) &&
                 (request->spi_tx_data == NU_NULL))
            {
                /* Set status to indicate that Tx data pointer is null. */
                status = SPI_INVALID_PARAM_POINTER;
            }

            /* If transfer involves reception, check the Rx buffer
               pointer for validity. */
            else if (((request->spi_transfer_type & SPI_RX) != 0) &&
                      (request->spi_rx_buffer == NU_NULL))
            {
                /* Set status to indicate that Rx buffer pointer is
                   null. */
                status = SPI_INVALID_PARAM_POINTER;
            }

            /* Check the transfer length for validity. */
            else if (request->spi_length == 0U)
            {
                /* Set status to indicate that transmission length is
                   zero. */
                status = SPI_INVALID_TRANSFER_LENGTH;
            }

            /* Check the element size to see if it is appropriate. */
            else
            {
                SPI_TRANSFER_CONFIG    *config = NU_NULL;

                /* Get the transfer attribute record associated with the
                   given slave of the specified SPI device. */
                config = SPICS_Get_Config_Struct(spi_cb,
                                                 request->spi_address);
                                                 
                if(config != NU_NULL)
                {
                    /* Check if the transfer size fits in the element size of
                       the transfer request. Transfer size is in number of bits
                       so multiply the element size by 8 for comparison. */
                    if ((config->spi_transfer_size) >
                        ((UINT8)(request->spi_element_size << 3)))
                    {
                        /* Set status to indicate that the buffer elements
                           cannot hold data units of current transfer size. */
                        status = SPI_ELEMENT_SIZE_NOT_ENOUGH;
                    }
                }
                else
                {
                    status = SPI_INVALID_ADDRESS;
                }
            }
        }
    }

#endif      /* NU_SPI_ERROR_CHECKING */

    /* Check if all checks are passed. */
    if (status == NU_SUCCESS)
    {
#if         (NU_SPI_SUPPORT_POLLING_MODE)

        /* Perform transfer in polling fashion. */
        status = SPICC_Transfer_Polled(spi_cb, request);

#else

        /* Perform interrupt-driven transfer. */
        status = SPICC_Transfer_Interrupt(spi_cb, request);
        
#endif      /* NU_SPI_SUPPORT_POLLING_MODE */
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

#if         (NU_SPI_SUPPORT_POLLING_MODE)

/*************************************************************************
* FUNCTION
*
*       SPICC_Transfer_Polled
*
* DESCRIPTION
*
*       This function is responsible for data transfers in the polling
*       manner.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device
*                                           control block pointer.
*
*      *request                             Pointer to SPI transfer
*                                           request.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS  SPICC_Transfer_Polled(SPI_CB       *spi_cb,
                              SPI_REQUEST  *request)
{
    UINT8          *tx_data;
    UINT8          *rx_buffer;
    UINT32          data;
    STATUS          status = NU_SUCCESS;
    UINT16          address;
    BOOLEAN         rx_flag;
    BOOLEAN         tx_flag;
    UINT8           element_size;
    SPI_DRV_IOCTL_WR_DATA spi_drv_ioctl_data;

    /* Evaluate whether transfer request involves transmission. */
    tx_flag = (BOOLEAN)(request->spi_transfer_type & SPI_TX);

    /* Evaluate whether transfer request involves reception. */
    rx_flag = (BOOLEAN)(request->spi_transfer_type & SPI_RX);

    /* Use dummy data while driving reception. */
    data = SPI_DUMMY_DATA;

    /* Get the pointer to user supplied Tx data. */
    tx_data = request->spi_tx_data;

    /* Setup a local pointer to the reception buffer. */
    rx_buffer = request->spi_rx_buffer;

    /* Get the address of slave. */
    address   = request->spi_address;

    /* Get the number of data units to be transferred. */
    spi_cb->spi_current_count = request->spi_length;

    /* Get the size of the data type used for the buffer. */
    element_size = request->spi_element_size;

    /* Protect the code against multithread access. */
    NU_Protect(&SPI_Protect_Struct);

    /* Transfer all specified data units. */
    while ((spi_cb->spi_current_count-- != 0) && (status == NU_SUCCESS))
    {
        /* Check if transfer request involves transmission. */
        if (tx_flag)
        {
            /* Read a data unit from the transmission buffer depending
               upon the size of the data type of the buffer. */
            switch (element_size)
            {
            /* 8 bit. */
            case    sizeof(UINT8):
                data = *tx_data;
                break;

            /* 16 bit. */
            case    sizeof(UINT16):
                data = *((UINT16 *)tx_data);
                break;

            /* 32 bit. */
            case    sizeof(UINT32):
                data = *((UINT32 *)tx_data);
                break;

            default:
                /* Do nothing. */
                break;
            }

            /* Increment the Tx data pointer to point to the next data
               unit to be transmitted. */
            tx_data += element_size;
        }

        /* Call the SPI hardware driver service to transmit the data
           indicating that the execution context is thread. */
            spi_drv_ioctl_data.address = address;
            spi_drv_ioctl_data.wr_data_val = data;
            spi_drv_ioctl_data.thread_context = NU_TRUE;
            
            status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle, spi_cb->spi_ioctl_base+SPI_DRV_WRITE, &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));

        /* Check if transmission was successful. */
        if (status == NU_SUCCESS)
        {
            UINT32          rx_data;

            /* Call the SPI hardware driver service to read in the
               received data from the hardware. */
            status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle, spi_cb->spi_ioctl_base+SPI_DRV_READ, &rx_data, sizeof(rx_data));


            /* Check if reception was successful and the transfer request
               involves reception. */
            if ((status == NU_SUCCESS) && rx_flag)
            {
                /* Write the received data unit to the reception
                   buffer depending upon the size of the data type of
                   the buffer. */
                switch (element_size)
                {
                /* 8 bit. */
                case    sizeof(UINT8):

                    /* Write the received data unit to the Rx buffer. */
                    *rx_buffer = (UINT8)rx_data;
                    break;

                /* 16 bit. */
                case    sizeof(UINT16):

                    /* Write the received data unit to the Rx buffer. */
                    *((UINT16 *)rx_buffer) = (UINT16)rx_data;
                    break;

                /* 32 bit. */
                case    sizeof(UINT32):

                    /* Write the received data unit to the Rx buffer. */
                    *((UINT32 *)rx_buffer) = rx_data;
                    break;

                default:
                    /* Do nothing. */
                    break;
                }

                /* Increment the Rx buffer pointer to point to the
                   next empty location in the Rx buffer. */
                rx_buffer += element_size;
            }
        }
    }

    /* Indicate that the transfer has ended. */
    spi_cb->spi_transfer_started = NU_FALSE;

    /* Release the protection. */
    NU_Unprotect();

    /* Return the completion status of the service. */
    return (status);
}

#else

/*************************************************************************
* FUNCTION
*
*       SPICC_Transfer_Interrupt
*
* DESCRIPTION
*
*       This function is responsible for data transfers in the interrupt
*       driven manner.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device
*                                           control block pointer.
*
*      *request                             Pointer to SPI transfer
*                                           request.
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
STATUS  SPICC_Transfer_Interrupt(SPI_CB        *spi_cb,
                                 SPI_REQUEST   *request)
{
    STATUS                  status;
    SPI_DRV_IOCTL_WR_DATA   spi_drv_ioctl_data;

    /* Enqueue the specified request indicating that it is being called 
       from the thread context. */
    status = SPIQC_Put_Queue_Tx(spi_cb, request, NU_TRUE);

    /* Check if the request was successfully enqueued. */
    if (status == NU_SUCCESS)
    {
        /* Check if a transfer is already active. */
        if (!spi_cb->spi_transfer_active)
        {
            /* No. Indicate that now transfer processing has started. */
            spi_cb->spi_transfer_active = NU_TRUE;

            /* Call the driver write service to initiate the interrupt
               driven transfer indicating that it is being called from
               the thread context . The Tx data passed in this
               call is dummy. Actual data will be picked from the queue. */
            spi_drv_ioctl_data.address = request->spi_address;
            spi_drv_ioctl_data.wr_data_val = NU_NULL;
            spi_drv_ioctl_data.thread_context = NU_TRUE;
            
            status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle, spi_cb->spi_ioctl_base+SPI_DRV_WRITE, &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));
        }
    }

    /* Return the completion status of the service. */
    return (status);
}

#endif      /* NU_SPI_SUPPORT_POLLING_MODE */
