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
*       spi_close.c
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains the service to close an SPI device.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPI_Close                           Shuts down an SPI device.
*
* DEPENDENCIES
*
*       spi_init_extr.h                     Function prototypes for
*                                           Nucleus SPI Initialization
*                                           component.
*
*       spi_osal_extr.h                     Function prototypes for
*                                           Nucleus SPI OSAL component.
*
*************************************************************************/
#define     NU_SPI_SOURCE_FILE

#include    "connectivity/spi_init_extr.h"
#include    "connectivity/spi_osal_extr.h"

/* Define external inner-component global data references. */
extern      SPI_CB      SPI_Devices[SPI_MAX_DEV_COUNT];


/*************************************************************************
* FUNCTION
*
*       SPI_Close
*
* DESCRIPTION
*
*       This function shuts down the specified SPI device and deallocates
*       the associated resources.
*
* INPUTS
*
*       spi_dev                             Nucleus SPI device handle.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       SPI_INVALID_HANDLE                  Nucleus SPI device handle is
*                                           not valid.
*
*       SPI_SHUTDOWN_ERROR                  SPI controller could not
*                                           be closed.
*
*       SPI_DEVICE_IN_USE                   The device is being used by
*                                           some other user.
*
*************************************************************************/
STATUS  SPI_Close(SPI_HANDLE spi_dev)
{
    SPI_CB      *spi_cb;
    STATUS      status;

    /* Declare variable needed by critical section. */
    INT     old_level;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if device handle is valid and get the port control block
       and device control block. */
    status = SPICS_Get_CBs(spi_dev, &spi_cb);

    /* Check if control blocks retrieved successfully. */
    if (status == NU_SUCCESS)
    {
        /* Decrease number of SPI device users. */
        spi_cb->spi_dev_user_count--;

        /* Check if no more users are using the device. */
        if (spi_cb->spi_dev_user_count == 0)
        {
            /* Close the device corresponding to this handle. */
            status = DVC_Dev_Close(spi_cb->spi_dev_handle);

            /* Cleanup resources irrespective of the status returned above.
             * Start critical section. */
            old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Clear the device control block structure. */
            spi_cb->init_flag = 0;

            /* Release the resources associated with this device. */
            status |= SPI_OSAL_Deallocate_Resources(spi_cb);

            /* End critical section. */
            (VOID)NU_Local_Control_Interrupts(old_level);
        }
        else
        {
            /* Set the status to indicate that the device is not closed
               because some other user is using the device. */
            status = SPI_DEVICE_IN_USE;
        }
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}
