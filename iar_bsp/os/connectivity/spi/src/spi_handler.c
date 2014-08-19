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
*       spi_handler.c
*
* COMPONENT
*
*       SPI Handler - Nucleus SPI Notification Handler
*
* DESCRIPTION
*
*       This file contains the routines responsible for handling the
*       Nucleus SPI user notifications.
*
* DATA STRUCTURES
*
*       SPI_Handler                         Control block for Nucleus SPI
*                                           notification handler.
*
* FUNCTIONS
*
*       SPI_Notify_Handler                  Function for activating a
*                                           thread handling Nucleus
*                                           SPI user notification events.
*
*       SPI_Handler_Entry                   Thread for handling Nucleus
*                                           SPI user notifications.
*
* DEPENDENCIES
*
*       spi_handler.h                       Function prototypes for
*                                           Nucleus SPI Notification
*                                           Handler component.
*
*************************************************************************/
#define     NU_SPI_SOURCE_FILE

#include    "connectivity/spi_handler.h"

/* Define external inner-component global data references. */
extern      SPI_CB      SPI_Devices[SPI_MAX_DEV_COUNT];

/* HISR for handling Nucleus SPI notification events. */
NU_HISR   SPI_Handler;
VOID*           SPI_Handler_Stack_Ptr;

/*************************************************************************
* FUNCTION
*
*       SPI_Notify_Handler
*
* DESCRIPTION
*
*       This function is used to activate the notification handler for
*       the specified SPI device.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device control
*                                           block pointer.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID SPI_Notify_Handler(SPI_CB *spi_cb)
{
    STATUS  status;

    /* Indicate that one more notification request is active. */
    spi_cb->spi_notify_pending++;

    /* Activate Nucleus SPI notification handler. */
    status = NU_Activate_HISR(&SPI_Handler);

    /* Check if the notification handler activation was successful. */
    if (status != NU_SUCCESS)
    {
        /* Notify the user about the error condition. */
        SPI_Notify_Error(spi_cb,
                         SPI_OS_ERROR,
                         spi_cb->spi_dev_id);
    }
}

/*************************************************************************
* FUNCTION
*
*       SPI_Notify_Error
*
* DESCRIPTION
*
*       This function is used to activate the error notification handler
*       for the specified SPI slave device.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device control
*                                           block pointer.
*       error_code                          The error code to be sent to
*                                           the error callback function.
*       spi_dev_id                          SPI device id.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID SPI_Notify_Error(SPI_CB *spi_cb, STATUS error_code,
                      DV_DEV_ID spi_dev_id)
{
    SPI_APP_CALLBACKS *callbacks;

    /* Get the associated callback functions record. */
    callbacks = SPICS_Get_Callbacks_Struct(spi_cb,
        spi_cb->spi_queue.spi_qnotify->spi_address);

    /* Call the error callback if configured. */
    if ((callbacks != NU_NULL) && (callbacks->spi_error != NU_NULL))
    {
        /* Notify the user about the error condition. */
        callbacks->spi_error(error_code, spi_dev_id);
    }
}

/*************************************************************************
* HANDLER
*
*       SPI_Handler_Entry
*
* DESCRIPTION
*
*       This SPI handler is responsible for handling Nucleus SPI
*       user notifications.
*
* INPUTS
*
*       VOID
*
* OUTPUTS
*
*       VOID
*
*************************************************************************/
VOID SPI_Handler_Entry (VOID)
{
    UINT8               spi_dev = 0;
    SPI_CB              *spi_cb;
    SPI_REQUEST         *request;
    UINT8               *tx_data;
    UINT8               *rx_buffer;
    SPI_APP_CALLBACKS   *callbacks;
    INT                 old_level;

    /* Service notification requests for all devices that have a
       pending notification request. */
    while (spi_dev < (UINT8)SPI_MAX_DEV_COUNT)
    {
        /* Check if the device has a pending notification. */
        if ((SPI_Devices[spi_dev].init_flag != NU_FALSE) &&
            (SPI_Devices[spi_dev].spi_notify_pending != 0))
        {
            spi_cb = &SPI_Devices[spi_dev];
            request = spi_cb->spi_queue.spi_qnotify;
            tx_data = NU_NULL;

            /* Check if device is using user buffering and the request
               involves transmission. */
            if (((spi_cb->spi_driver_mode & SPI_USER_BUFFERING) == 0) &&
                ((request->spi_transfer_type & SPI_TX) != 0))
            {
                /* Tx data buffer pointer that was specified when transfer
                   request was placed with Nucleus SPI will be passed
                   to user callback. */
                tx_data = request->spi_tx_data;
            }

            /* If the transfer request involved reception then Rx buffer
               pointer that was specified when transfer request was placed
               with Nucleus SPI will be passed to user callback. */
            rx_buffer = ((request->spi_transfer_type & SPI_RX) != 0) ?
                          request->spi_rx_buffer : NU_NULL;

            /* Get the associated callback functions record. */
            callbacks = SPICS_Get_Callbacks_Struct(spi_cb,
                                                   request->spi_address);

            /* Invoke the transfer complete notification callback if
               configured. */
            if ((callbacks != NU_NULL) &&
                (callbacks->spi_transfer_complete!= NU_NULL))
            {

                /* Call the user callback function for transfer complete
                   notification. */
                callbacks->spi_transfer_complete(spi_cb->spi_dev_id,
                                                 request->spi_address,
                                                 tx_data,
                                                 rx_buffer,
                                                 request->spi_length,
                                                 request->spi_element_size);

                /* Indicate that one pending notification request has been
                   processed. */
                spi_cb->spi_notify_pending--;

                /* Increment the notification pointer to read from the next
                   location. */
                spi_cb->spi_queue.spi_qnotify++;
            }

            /* Start critical section. */
            old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Decrement the notification counter for the queue indicating
               that a transfer has now been completely processed. */
            spi_cb->spi_queue.spi_qnotify_count--;

            /* End critical section. */
            (VOID)NU_Local_Control_Interrupts(old_level);

            /* Check if the end of the buffer has been reached. */
            if (spi_cb->spi_queue.spi_qnotify > spi_cb->spi_queue.spi_qend)
            {
                /* Wrap around to the start of queue. */
                spi_cb->spi_queue.spi_qnotify =
                    spi_cb->spi_queue.spi_qstart;
            }
        }

        /* Increment the index to check next device for pending
           notification requests. */
        spi_dev++;
    }
}

