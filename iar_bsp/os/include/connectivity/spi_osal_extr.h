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
*       spi_osal_extr.h
*
* COMPONENT
*
*       SPI OSAL - OS Abstraction Layer for Nucleus SPI
*
* DESCRIPTION
*
*       This file contains function prototypes of Nucleus SPI OS
*       abstraction layer.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       spic_defs.h                         Nucleus SPI Core Services
*                                           component definitions file.
*
*       reg_api.h                           System Registry
*
*************************************************************************/
/* Check to see if the file has already been included. */
#ifndef     SPI_OSAL_EXTR_H
#define     SPI_OSAL_EXTR_H

#include    "services/reg_api.h"
#include    "connectivity/spic_defs.h"

/* Function prototypes for allocating OS resources to Nucleus SPI. */

STATUS  SPI_OSAL_Allocate_Resources     (SPI_CB *spi_cb);
STATUS  SPI_OSAL_Deallocate_Resources   (SPI_CB *spi_cb);
STATUS  SPI_OSAL_Create_Handler         (NU_MEMORY_POOL *mem);
VOID    SPI_OSAL_Delete_Handler         (VOID);
STATUS  SPI_OSAL_Allocate_Attribute_List(SPI_CB *spi_cb);
STATUS  SPI_OSAL_Allocate_Callbacks_List(SPI_CB *spi_cb);

#endif      /* !SPI_OSAL_EXTR_H */

