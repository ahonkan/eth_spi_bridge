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
*       spi_osal.h
*
* COMPONENT
*
*       SPI OSAL - OS Abstraction Layer for Nucleus SPI
*
* DESCRIPTION
*
*       This file contains constant definitions and API mappings to provide
*       minimal OS abstraction for various services of Nucleus SPI.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       spi_cfg.h                           Nucleus SPI configuration
*                                           file.
*
*       nucleus.h                           Main definition and API file
*                                           for Nucleus PLUS.
*
*       reg_api.h                           System Registry
*
*       nu_kernel.h                         Kernel definitions
*
*************************************************************************/
/* Check to see if the file has already been included. */
#ifndef     SPI_OSAL_H
#define     SPI_OSAL_H

#include    "connectivity/spi_cfg.h"
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/reg_api.h"

/* Mapping to provide abstraction for memory allocation. */

#define     SPI_HANDLER_MEM_STACK          (UNSIGNED)(NU_MIN_STACK_SIZE*2)
#define     SPI_HANDLER_PRIORITY           (OPTION)0

/* Mapping to provide abstraction for SPI message handlers. */

#define     SPI_Handler_Service(service)    VOID service(VOID)

/* Extern the multithread protection structure here so that it
   does not need externing in all the files using the above macros. */
extern      NU_PROTECT           SPI_Protect_Struct;

#if         (PLUS_VERSION_COMP > PLUS_1_15)

#undef      SPI_HANDLER_MEM_STACK
#define     SPI_HANDLER_MEM_STACK       (UNSIGNED)(NU_MIN_STACK_SIZE * 2)

#endif      /* PLUS_VERSION_COMP > PLUS_1_15 */

#endif      /* !SPI_OSAL_H */

