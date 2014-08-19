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
*       spi_handler.h
*
* COMPONENT
*
*       SPI HANDLER - Nucleus SPI Notification Handler
*
* DESCRIPTION
*
*       This file contains function prototypes for Nucleus SPI
*       Notification Handler component.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       spic_extr.h                         Function prototypes for
*                                           Nucleus SPI Core Services
*                                           component.
*
*************************************************************************/
/* Check to see if the file has already been included. */
#ifndef     SPI_HANDLER_H
#define     SPI_HANDLER_H

#include    "connectivity/spic_extr.h"

/* Function to activate the Nucleus SPI notification handler thread. */
VOID    SPI_Notify_Handler (SPI_CB *spi_cb);

/* Function to activate the error notification handler. */
VOID    SPI_Notify_Error (SPI_CB *spi_cb, STATUS error_code,
                          DV_DEV_ID spi_dev_id);

/* Entry point for Nucleus SPI notification handler service (HISR). */
VOID    SPI_Handler_Entry (VOID);

#endif      /* !SPI_HANDLER_H */

