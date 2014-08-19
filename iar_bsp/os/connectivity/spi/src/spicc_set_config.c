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
*       spicc_set_config.c
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains the service to set transfer attributes.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPICC_Set_Configuration             Sets transfer attributes.
*
* DEPENDENCIES
*
*       spic_extr.h                         Function prototypes of the
*                                           Nucleus SPI Core Services
*                                           component.
*
*************************************************************************/
#define     NU_SPI_SOURCE_FILE

#include    "connectivity/spic_extr.h"

/*************************************************************************
* FUNCTION
*
*       SPICC_Set_Configuration
*
* DESCRIPTION
*
*       This function sets the specified transfer attributes for the
*       given slave of the specified SPI master device or for the
*       device itself if it is an SPI slave.
*
* INPUTS
*
*       spi_dev                             Nucleus SPI device handle.
*
*       address                             In case of master node
*                                           specifies the slave with which
*                                           this transfer should take
*                                           place. For a slave node this
*                                           parameter is ignored.
*
*      *config                              Pointer to a Nucleus SPI
*                                           transfer attributes
*                                           configuration structure which
*                                           contains the desired transfer
*                                           attributes.
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
*       SPI_INVALID_MODE                    Specified SPI mode is not
*                                           valid.
*
*       SPI_INVALID_BIT_ORDER               Specified bit order is invalid.
*
*       SPI_INVALID_SS_POLARITY             Specified slave-select
*                                           polarity is not valid.
*
*       SPI_UNSUPPORTED_BAUD_RATE           The specified baud rate is
*                                           not supported.
*
*       SPI_UNSUPPORTED_MODE                Specified SPI mode is not
*                                           supported.
*
*       SPI_UNSUPPORTED_BIT_ORDER           Specified bit order is not
*                                           supported.
*
*       SPI_UNSUPPORTED_TRANSFER_SIZE       Specified transfer size is
*                                           not supported.
*
*************************************************************************/
STATUS  SPICC_Set_Configuration(SPI_HANDLE                  spi_dev,
                                UINT16                  address,
                                SPI_TRANSFER_CONFIG     *config)
{
    SPI_CB              *spi_cb;
    STATUS              status;
    SPI_DRV_IOCTL_DATA  spi_drv_ioctl_data;

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
        status = SPICS_Check_Address(spi_cb, address);

        /* Check if specified address is found to be valid. */
        if (status == NU_SUCCESS)
        {
            /* Check if the transfer attribute configuration structure
               pointer is null. */
            if (config == NU_NULL)
            {
                /* Set the status to indicate that transfer attribute
                   configuration pointer is null. */
                status = SPI_INVALID_PARAM_POINTER;
            }

            /* Check specified clock polarity and clock phase for
               validity. */
            else if (((config->spi_clock_polarity != SPI_CPOL_0) &&
                      (config->spi_clock_polarity != SPI_CPOL_1)) ||
                     ((config->spi_clock_phase != SPI_CPHA_0) &&
                      (config->spi_clock_phase != SPI_CPHA_1)))
            {
                /* Specified SPI mode is not valid. */
                status = SPI_INVALID_MODE;
            }

            /* Check specified bit order for validity. */
            else if ((config->spi_bit_order != SPI_MSB_FIRST) &&
                     (config->spi_bit_order != SPI_LSB_FIRST))
            {
                /* The bit order parameter is not valid. */
                status = SPI_INVALID_BIT_ORDER;
            }

            /* Check slave-select polarity for validity. */
            else if ((config->spi_ss_polarity != SPI_SSPOL_LOW) &&
                 (config->spi_ss_polarity != SPI_SSPOL_HIGH))
            {
                /* Specified slave-select polarity is not valid. */
                status = SPI_INVALID_SS_POLARITY;
            }

            else
            {
                /* No other parameter to check for validity. */
            }
        }
    }

#endif      /* NU_SPI_ERROR_CHECKING */

    /* Check if all checks are passed. */
    if (status == NU_SUCCESS)
    {
        SPI_TRANSFER_CONFIG    *spi_config = NU_NULL;

        /* Get the associated transfer attribute configuration
           record. */
        spi_config = SPICS_Get_Config_Struct(spi_cb, address);

        /* Protect the critical section against multithread access. */
        NU_Protect(&SPI_Protect_Struct);

        /* Set the specified baud rate only if the specified device is a
           master. */
        if ((spi_cb->spi_master_mode == NU_TRUE) && (spi_config != NU_NULL))
        {
            /* Call the driver service to set the baud rate. */
            spi_drv_ioctl_data.address = address;
            spi_drv_ioctl_data.xfer_attrs = config;

            status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                       (spi_cb->spi_ioctl_base+SPI_SET_BAUD_RATE),
                                       &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));

            /* Check if the operation was successful. */
            if (status == NU_SUCCESS)
            {
                /* Record the specified baud rate in the transfer attribute
                   configuration record. */
                spi_config->spi_baud_rate = config->spi_baud_rate;
            }
        }
        else
        {
            status = SPI_INVALID_ADDRESS;
        }


        /* Check if the operation was successful. */
        if (status == NU_SUCCESS)
        {
        
            /* Call the driver service to set the SPI mode. */
            spi_drv_ioctl_data.address = address;
            spi_drv_ioctl_data.xfer_attrs = config;

            status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                       (spi_cb->spi_ioctl_base+SPI_SET_MODE),
                                       &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));

            /* Check if the operation was successful. */
            if (status == NU_SUCCESS)
            {
                /* Record the specified clock phase in the transfer
                   attribute configuration record. */
                spi_config->spi_clock_phase    = config->spi_clock_phase;

                /* Record the specified clock polarity in the transfer
                   attribute configuration record. */
                spi_config->spi_clock_polarity =
                    config->spi_clock_polarity;

                /* Call the driver service to set the bit order. */
                spi_drv_ioctl_data.address = address;
                spi_drv_ioctl_data.xfer_attrs = config;
        
                status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                           (spi_cb->spi_ioctl_base+SPI_SET_BIT_ORDER),
                                           &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));
                                           
                /* Check if the operation was successful. */
                if (status == NU_SUCCESS)
                {
                    /* Record the specified bit order in the transfer
                       attribute configuration record. */
                    spi_config->spi_bit_order = config->spi_bit_order;

                    /* Call the driver service to set the transfer size. */                
                    spi_drv_ioctl_data.address = address;
                    spi_drv_ioctl_data.xfer_attrs = config;

                    status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                               (spi_cb->spi_ioctl_base+SPI_SET_TRANSFER_SIZE),
                                               &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));

                    /* Check if the operation was successful. */
                    if (status == NU_SUCCESS)
                    {
                        /* Record the specified transfer size in the
                           transfer attribute configuration record. */
                        spi_config->spi_transfer_size
                          = config->spi_transfer_size;

                        /* Call the driver service to set the slave-select polarity. */
                        spi_drv_ioctl_data.address = address;
                        spi_drv_ioctl_data.xfer_attrs = config;
                
                        status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                                   (spi_cb->spi_ioctl_base+SPI_SET_SS_POLARITY),
                                                   &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));
                                   
                        /* Check if the operation was successful. */
                        if (status == NU_SUCCESS)
                        {
                            /* Record the specified slave-select polarity in the
                               transfer attribute configuration record. */
                            spi_config->spi_ss_polarity
                              = config->spi_ss_polarity;
                        }
                    }
                }
            }
        }

        /* Release the protection. */
        NU_Unprotect();
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

