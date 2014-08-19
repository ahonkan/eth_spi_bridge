/*************************************************************************
*
*                  Copyright 2006 Mentor Graphics Corporation
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
*       spi_cfg.h
*
* COMPONENT
*
*       Nucleus SPI Configuration
*
* DESCRIPTION
*
*       This file contains the configuration options for Nucleus SPI.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
/* Check to avoid multiple file inclusion. */
#ifndef     SPI_CFG_H
#define     SPI_CFG_H

/* Maximum number of devices (controllers) being handled by Nucleus SPI.
   This should be equal to the sum of the controllers in all the driver
   ports integrated with Nucleus SPI. */
#define     SPI_MAX_DEV_COUNT           2

/* DEFINE:      NU_SPI_ERROR_CHECKING
   DEFAULT:     NU_SPI_ERROR_CHECKING is set to 1.
   DESCRIPTION: This option allows the user to enable/disable the
                error checking for the parameters.
   NOTE:        1) Nucleus SPI, Nucleus SPI Driver and Nucleus SPI
                   demonstration must be rebuilt after changing this
                   options. */
#define         NU_SPI_ERROR_CHECKING           CFG_NU_OS_CONN_SPI_ERR_CHECK_ENABLE

/* DEFINE:      NU_SPI_SUPPORT_POLLING_MODE
   DEFAULT:     NU_SPI_SUPPORT_POLLING_MODE is set to 0.
   DESCRIPTION: This option allows the user to enable/disable support for
                polling mode in Nucleus SPI. If none of the devices uses
                polling mode, setting this option to '0' would greatly
                help in reducing the code size.
   NOTE:        1) Nucleus SPI, Nucleus SPI Driver and Nucleus SPI
                   demonstration must be rebuilt after changing this
                   options.
                2) Setting this define to '1' does not mean that the
                   devices will start operating in the polling mode.
                   It only enables Nucleus SPI to support the devices
                   running in polling mode. Actual operating mode of
                   the driver should be specified at the
                   initialization time through spi_driver_mode field
                   of the initialization structure. */
#define         NU_SPI_SUPPORT_POLLING_MODE     CFG_NU_OS_CONN_SPI_POLLING_MODE_ENABLE

/* DEFINE:      NU_SPI_USER_BUFFERING_ONLY
   DEFAULT:     NU_SPI_USER_BUFFERING_ONLY is set to 0.
   DESCRIPTION: This option allows the user to enable/disable support for
                internal buffering performed in the interrupt driven mode.
                If all devices use user buffering, setting this option
                to '1' would greatly help in reducing the code size.
   NOTE:        1) Nucleus SPI and Nucleus SPI Driver must be rebuilt
                   after changing this option.
                2) Setting this define to '1' means that internal
                   buffering will not be available and user buffering must
                   be used.
                3) When this option is set to '1' NU_SPI_SPEED_COPY option
                   (see below) has no effect. */
#define         NU_SPI_USER_BUFFERING_ONLY      CFG_NU_OS_CONN_SPI_USER_BUFF_ENABLE

/* DEFINE:      NU_SPI_SPEED_COPY
   DEFAULT:     NU_SPI_SPEED_COPY is set to 0.
   DESCRIPTION: This option allows to user to enable/disable fast copying
                from user buffer to internal buffer on the expense of
                increased code size. This option is used only in interrupt
                driven mode when user buffering is not being used. Set
                this option to '1' to enable this speed optimization.
   NOTE:        1) Nucleus SPI, Nucleus SPI Driver and Nucleus SPI
                   demonstration must be rebuilt after changing this
                   option.
                2) This option has no effect when user buffering is
                   utilized. */
#define         NU_SPI_SPEED_COPY               0

/* If user buffer mode is enabled then interrupt mode must also be enabled, otherwise 
 * generate an error.
 */
#if ( (NU_SPI_USER_BUFFERING_ONLY) && (NU_SPI_SUPPORT_POLLING_MODE) )
#error "User buffer is only supported in interrupt driven I/O mode"
#endif

#endif      /* !SPI_CFG_H */

