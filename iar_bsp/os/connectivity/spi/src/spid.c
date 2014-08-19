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
*       spid.c
*
* COMPONENT
*
*       Nucleus SPI
*
* DESCRIPTION
*
*       This file contains the data structures for Nucleus SPI.
*
* DATA STRUCTURES
*
*       SPI_Devices                         Array of Nucleus SPI device
*                                           control block pointers.
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       spic_extr.h                         Function prototypes of
*                                           Nucleus SPI Core Services
*                                           component.
*
*       spi_osal.h                          Constant definitions and API
*                                           mappings to provide minimal
*                                           OS abstraction for various
*                                           services of Nucleus SPI.
*
*************************************************************************/
#define     NU_SPI_SOURCE_FILE

#include    "connectivity/spic_extr.h"
#include    "connectivity/spi_osal.h"

/* Array of Nucleus SPI device control block pointers. */
SPI_CB         SPI_Devices[SPI_MAX_DEV_COUNT];
/* Protection structure for protection of critical sections
   against multithread access. */
NU_PROTECT     SPI_Protect_Struct;

