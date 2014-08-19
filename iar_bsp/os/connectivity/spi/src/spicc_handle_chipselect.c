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
*       spicc_handle_chipselect.c
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains the service to turn on (or off) auto chipselect
*       Assertion/Deassertion.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPICC_Handle_Chipselect             Sets/Unsets auto chipselect
*                                           Assertion/Deassertion.
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
*       SPICC_Handle_Chipselect
*
* DESCRIPTION
*
*       This function sets/unsets auto chipselect Assertion/Deassertion.
*
* INPUTS
*
*       spi_dev                             Handle to the SPI device to
*                                           be used in this transfer.
*
*       address                             Specifies the slave for which
*                                           this attribute is being set.
*
*       handle_chipselect                   Desired behavior of chipselect.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       SPI_INVALID_HANDLE                  Nucleus SPI device handle is
*                                           not valid.
*
*       SPI_INVALID_CHIPSELECT              Chipselect flag is not valid.
*
*************************************************************************/
STATUS SPICC_Handle_Chipselect(SPI_HANDLE       spi_dev, 
                                   UINT16       address, 
                                   BOOLEAN      handle_chipselect)
{
    SPI_CB              *spi_cb;
    STATUS              status;
    SPI_DRV_IOCTL_DATA  spi_drv_ioctl_data;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();
    
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
            /* Check chipselect flag for validity. */
            if ((handle_chipselect != NU_TRUE) &&
                (handle_chipselect != NU_FALSE))
            {
                /* Specified chipselect flag is not valid. */
                status = SPI_INVALID_CHIPSELECT;
            }
        }
    }

#endif      /* NU_SPI_ERROR_CHECKING */
    
    /* Check if all checks are passed. */
    if (status == NU_SUCCESS)
    {
        SPI_TRANSFER_CONFIG     spi_config;

        /* Specify the desired chipselect flag. */
        spi_config.handle_chipselect = handle_chipselect;

        /* Set the slave address. */
        spi_drv_ioctl_data.address = address;
        spi_drv_ioctl_data.xfer_attrs = &spi_config;
        
        /* Call the driver service to set the chipselect flag. */
        status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle, 
                                spi_cb->spi_ioctl_base+SPI_HANDLE_CHIPSELECT, 
                                &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);    
}
