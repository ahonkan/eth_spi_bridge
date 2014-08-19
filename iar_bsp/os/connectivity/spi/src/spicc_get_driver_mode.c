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
*       spicc_get_driver_mode.c
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains the service to query the driver mode.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPICC_Get_Driver_Mode               Gets the driver mode
*                                           (polling or interrupt).
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
*       SPICC_Get_Driver_Mode
*
* DESCRIPTION
*
*       This function gets the driver mode of the specified SPI master
*       device.
*
* INPUTS
*
*      spi_dev                              Nucleus SPI device handle.
*
*      *spi_driver_mode                     Pointer to a Nucleus SPI
*                                           driver mode which will get the
*                                           queried driver mode information.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       SPI_INVALID_HANDLE                  Nucleus SPI device handle is
*                                           not valid.
*
*************************************************************************/
STATUS  SPICC_Get_Driver_Mode(SPI_HANDLE spi_dev, 
                                  SPI_DRIVER_MODE *spi_driver_mode)
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
        /* Check if the driver mode return pointer is null. */
        if (spi_driver_mode == NU_NULL)
        {
            /* Set the status to indicate that pointer to return
               callback functions information is null. */
            status = SPI_INVALID_PARAM_POINTER;
        }
    }

#endif      /* NU_SPI_ERROR_CHECKING */

    /* Check if control block structure is returned successfully. */
    if (status == NU_SUCCESS)
    {
        /* Return the driver mode to user. */
        *spi_driver_mode = spi_cb->spi_driver_mode;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

