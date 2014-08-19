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
*       spicc_set_baud_rate.c
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains the service that sets the baud rate.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPICC_Set_Baud_Rate                 Sets baud rate.
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
*       SPICC_Set_Baud_Rate
*
* DESCRIPTION
*
*       This function sets the baud rate for the specified slave of the
*       specified SPI device.
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
*       baud_rate                           Desired baud rate.
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
*       SPI_UNSUPPORTED_BAUD_RATE           The specified baud rate is
*                                           not supported.
*
*************************************************************************/
STATUS  SPICC_Set_Baud_Rate(SPI_HANDLE  spi_dev,
                            UINT16      address,
                            UINT32      baud_rate)
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
    }

#endif      /* NU_SPI_ERROR_CHECKING */

    /* Check if all checks are passed. */
    if (status == NU_SUCCESS)
    {
        SPI_TRANSFER_CONFIG     spi_config;

        /* Specify the desired baud rate. */
        spi_config.spi_baud_rate = baud_rate;

        /* Protect the critical section against multithread access. */
        NU_Protect(&SPI_Protect_Struct);

        /* Call the driver service to set the baud rate. */
        spi_drv_ioctl_data.address = address;
        spi_drv_ioctl_data.xfer_attrs = &spi_config;

        status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                   (spi_cb->spi_ioctl_base+SPI_SET_BAUD_RATE),
                                   &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));

        /* Check if the operation was successful. */
        if (status == NU_SUCCESS)
        {
            SPI_TRANSFER_CONFIG    *config = NU_NULL;

            /* Get the transfer attribute record associated with the given
               slave of the specified SPI device. */
            config = SPICS_Get_Config_Struct(spi_cb, address);

            if(config != NU_NULL)
            {
                /* Record the specified baud rate in the associated transfer
                attribute configuration record. */
                config->spi_baud_rate = baud_rate;
            }
            else
            {
                status = SPI_INVALID_ADDRESS;
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

