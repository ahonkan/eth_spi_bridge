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
*       spi_extern.h
*
* COMPONENT
*
*       Nucleus SPI
*
* DESCRIPTION
*
*       This file provides interface to Nucleus SPI Driver.
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
*       spiq_extr.h                         Function prototypes for
*                                           Nucleus SPI Queue Management
*                                           component.
*
*       spi_handler.h                       Function prototypes for
*                                           Nucleus SPI Notification
*                                           Handler component.
*
*************************************************************************/
/* Check to see if the file has been included already. */
#ifndef     SPI_EXTERN_H
#define     SPI_EXTERN_H

#include    "connectivity/spic_extr.h"
#include    "connectivity/spiq_extr.h"
#include    "connectivity/spi_handler.h"

/* Macro to check whether current transfer involves transmission. */
#define     SPI_TRANSFER_INVOLVES_TX(spi_cb)    (spi_cb->spi_queue        \
                                                       .spi_qread         \
                                                      ->spi_transfer_type \
                                                      & SPI_TX)

/* Macro to check whether current transfer involves reception. */
#define     SPI_TRANSFER_INVOLVES_RX(spi_cb)    (spi_cb->spi_queue        \
                                                       .spi_qread         \
                                                      ->spi_transfer_type \
                                                      & SPI_RX)

#endif      /* !SPI_EXTERN_H */
