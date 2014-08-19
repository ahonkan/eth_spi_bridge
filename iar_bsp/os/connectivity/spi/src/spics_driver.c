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
*       spics_driver.c
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains the routines for supporting Nucleus SPI Driver
*       ports.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPICS_ISR_Action                    Performs ISR handling.
*
*       SPICS_Write_Action                  Prepares environment for
*                                           starting a transfer.
*
* DEPENDENCIES
*
*       spic_extr.h                         Function prototypes of the
*                                           Nucleus SPI Core Services.
*
*       spiq_extr.h                         Function prototypes for
*                                           for Nucleus SPI Queue
*                                           Management component.
*
*       spi_handler.h                       Function prototypes for
*                                           Nucleus SPI Notification
*                                           Handler component.
*
*************************************************************************/
#define     NU_SPI_SOURCE_FILE

#include    "connectivity/spic_extr.h"
#include    "connectivity/spiq_extr.h"
#include    "connectivity/spi_handler.h"

/*************************************************************************
* FUNCTION
*
*       SPICS_ISR_Action
*
* DESCRIPTION
*
*       This function performs ISR handling.
*
* INPUTS
*
*
*      *spi_cb                              Nucleus SPI device
*                                           control block pointer.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    SPICS_ISR_Action(SPI_CB        *spi_cb)
{
    SPI_DRV_IOCTL_WR_DATA   spi_drv_ioctl_data;
    INT                     old_level;

    /* Check if the current transfer has been completely
       processed. */
    if (spi_cb->spi_current_length == 0U)
    {
        /* Indicate that a transfer has ended. */
        spi_cb->spi_transfer_started = NU_FALSE;

#if         (NU_SPI_USER_BUFFERING_ONLY == 0)

        /* Update buffer state if using internal buffer. */
        if ( (spi_cb->spi_queue.spi_qread->spi_transfer_type != SPI_RX) &&
            !(spi_cb->spi_driver_mode & SPI_USER_BUFFERING))
        {
            /* Update buffer's read pointer. */
            SPIQS_Update_Buffer_State(spi_cb);
        }

#endif      /* NU_SPI_USER_BUFFERING_ONLY == 0 */

        /* Increment the pointer to read from the next location. */
        spi_cb->spi_queue.spi_qread++;

        /* Start critical section. */
        old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Decrement the counter for transmit queue. */
        spi_cb->spi_queue.spi_qcount--;

        /* End critical section. */
        (VOID)NU_Local_Control_Interrupts(old_level);

        /* Check if the end of the buffer has been reached. */
        if (spi_cb->spi_queue.spi_qread > spi_cb->spi_queue.spi_qend)
        {
            /* Wrap around to the start of queue. */
            spi_cb->spi_queue.spi_qread = spi_cb->spi_queue.spi_qstart;
        }

        /* Activate SPI notification handler. */
        SPI_Notify_Handler(spi_cb);

        /* Check if there are more transfer requests in the
           queue. */
        if (spi_cb->spi_queue.spi_qcount != 0)
        {
            /* Start processing the next transfer request in the
               queue indicating that the execution context is not
               thread. */
            spi_drv_ioctl_data.address = spi_cb->spi_queue.spi_qread->spi_address;
            spi_drv_ioctl_data.wr_data_val = 0UL;
            spi_drv_ioctl_data.thread_context = NU_FALSE;
    
            DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                       (spi_cb->spi_ioctl_base+SPI_DRV_WRITE),
                                       &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));
        }

        /* No transfer request left in the queue. */
        else
        {
            /* So change the flag to indicate that now no transfer is
               active. */
            spi_cb->spi_transfer_active = NU_FALSE;
        }
    }

    /* Otherwise current transfer request has not completed yet. */
    else
    {
        /* Continue with processing the transfer request indicating
           that the execution context is not thread. */        
        spi_drv_ioctl_data.address = spi_cb->spi_queue.spi_qread->spi_address;
        spi_drv_ioctl_data.wr_data_val = 0UL;
        spi_drv_ioctl_data.thread_context = NU_FALSE;
        
        DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                   (spi_cb->spi_ioctl_base)+SPI_DRV_WRITE,
                                   &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));
                                       
    }
}

/*************************************************************************
* FUNCTION
*
*       SPICS_Write_Action
*
* DESCRIPTION
*
*       This function prepares environment for starting a transfer.
*
* INPUTS
*
*      *spi_cb                              Nucleus SPI device
*                                           control block pointer.
*
*       address                             Target slave address.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    SPICS_Write_Action(SPI_CB      *spi_cb,
                           UINT16       address)
{
    SPI_DRV_IOCTL_DATA spi_drv_ioctl_data;

        
    /* Check to see if a new transfer is about to start. */
    if (spi_cb->spi_transfer_started == NU_FALSE)
    {
        /* Set the flag to indicate that a new transfer has
           started. */
        spi_cb->spi_transfer_started = NU_TRUE;

        /* Get the length of the transfer that is about to start. */
        spi_cb->spi_current_count = spi_cb->spi_queue
                                           .spi_qread->spi_length;

        /* Get the Tx data pointer of the transfer. */
        spi_cb->spi_current_tx_data = spi_cb->spi_queue
                                            .spi_qread->spi_tx_data;

        /* Get the Rx buffer pointer of the transfer. */
        spi_cb->spi_current_rx_buffer = spi_cb->spi_queue
                                               .spi_qread->spi_rx_buffer;

        /* Get the length of the transfer that is about to start. */
        spi_cb->spi_current_length = spi_cb->spi_queue
                                            .spi_qread->spi_length;

        /* Apply transfer attributes associated with the specified
           slave if the specified device is an SPI master device. */
        if (spi_cb->spi_sim_tx_attribs &&
            spi_cb->spi_master_mode)
        {
            SPI_TRANSFER_CONFIG    *spi_config;

            /* Get the transfer attribute record associated with the given
               slave of the specified SPI device. */
            spi_config = SPICS_Get_Config_Struct(spi_cb, address);

            /* Switch to the transfer attributes associated with the
               slave involved in the transfer. */
            spi_drv_ioctl_data.address = address;
            spi_drv_ioctl_data.xfer_attrs = spi_config;
    
            DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                       (spi_cb->spi_ioctl_base)+SPI_APPLY_TRANSFER_ATTRIBS,
                                       &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));
        }
    }
}
