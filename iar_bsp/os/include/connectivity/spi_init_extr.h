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
*       spi_init_extr.h
*
* COMPONENT
*
*       Nucleus SPI Initialization
*
* DESCRIPTION
*
*       This file contains function prototypes for Nucleus SPI
*       Initialization component.
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
/* Check to see if the file has been included already. */
#ifndef     SPI_INIT_H
#define     SPI_INIT_H

#include    "connectivity/spic_extr.h"

STATUS  SPI_Start       (SPI_HANDLE *spi_dev, SPI_INIT *spi_init);
STATUS  SPI_Close       (SPI_HANDLE spi_dev);
STATUS  SPI_Cleanup     (SPI_HANDLE spi_dev);

#endif      /* !SPI_INIT_H */
