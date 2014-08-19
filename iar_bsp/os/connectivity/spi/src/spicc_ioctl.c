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
*       spicc_ioctl.c
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains the service that provides interface to I/O
*       control routine of the driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPICC_Ioctl_Driver                  Perform I/O control
*                                           operations on the controller.
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
*       SPICC_Ioctl_Driver
*
* DESCRIPTION
*
*       This function provides an interface to the I/O control service
*       of the driver which performs I/O control operations on the SPI
*       controller.
*
* INPUTS
*
*      *spi_dev                             Nucleus SPI device handle.
*
*       address                             Address of the slave for
*                                           which the control operation
*                                           is intended. This parameter
*                                           is ignored if the device
*                                           itself is a slave.
*
*       control_code                        Specifies the control
*                                           operation to be performed.
*
*      *control_info                        Information associated to/
*                                           required by the control
*                                           operation.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       SPI_INVALID_HANDLE                  Nucleus SPI device handle is
*                                           not valid.
*
*       SPI_INVALID_IOCTL_OPERATION         Unsupported operation
*                                           requested for I/O control.
*
*************************************************************************/
STATUS  SPICC_Ioctl_Driver(SPI_HANDLE       spi_dev,
                               UINT16       address,
                               INT          control_code,
                               VOID        *control_info)
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
    }

#endif      /* NU_SPI_ERROR_CHECKING */

    /* Check if all checks are passed. */
    if (status == NU_SUCCESS)
    {
        /* Call the driver I/O control function. */
        status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                (spi_cb->spi_ioctl_base+control_code),
                                control_info, sizeof(control_info));    

    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

