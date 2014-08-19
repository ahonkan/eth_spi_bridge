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
*       spicc_get_callbacks.c
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains the service to query the callback functions.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPICC_Get_Callbacks                 Gets the callback functions.
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
*       SPICC_Get_Callbacks
*
* DESCRIPTION
*
*       This function gets the callback functions for the given slave
*       of the specified SPI master device.
*
* INPUTS
*
*       spi_dev                             Nucleus SPI device handle.
*
*       address                             In case of master node
*                                           specifies the slave whose
*                                           associated callback functions
*                                           are to be queried.
*
*      *callbacks                           Pointer to a Nucleus SPI
*                                           callback structure which
*                                           will get the queried callback
*                                           functions.
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
*************************************************************************/
STATUS  SPICC_Get_Callbacks(SPI_HANDLE spi_dev, UINT16 address,
                                SPI_APP_CALLBACKS *callbacks)
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
        status = SPICS_Check_Address(spi_cb, address);

        /* Check if specified address is found to be valid. */
        if (status == NU_SUCCESS)
        {
            /* Check if the callback function pointer is null. */
            if (callbacks == NU_NULL)
            {
                /* Set the status to indicate that pointer to return
                   callback functions information is null. */
                status = SPI_INVALID_PARAM_POINTER;
            }
        }
    }

#endif      /* NU_SPI_ERROR_CHECKING */

    /* Check if all checks are passed. */
    if (status == NU_SUCCESS)
    {
        SPI_APP_CALLBACKS    *spi_callbacks = NU_NULL;

        /* Get the associated callback functions record. */
        spi_callbacks = SPICS_Get_Callbacks_Struct(spi_cb, address);

        /* Protect the critical section against multithread access. */
        NU_Protect(&SPI_Protect_Struct);

        /* Return the callback functions. */
        if(spi_callbacks != NU_NULL)
        {
            callbacks->spi_error = spi_callbacks->spi_error;
            callbacks->spi_transfer_complete =
                spi_callbacks->spi_transfer_complete;
        }
        else
        {
            status = SPI_INVALID_ADDRESS;
        }
        
        /* Release the protection. */
        NU_Unprotect();
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

