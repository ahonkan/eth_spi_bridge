/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       sd_cfg.h
*
* COMPONENT
*
*       Nucleus SDIO FILE Function Driver - Configuration file
*
* DESCRIPTION
*
*       This file contains all user modifiable switches that allow
*       extended features to be supported by Nucleus SDIO FILE Function
*       Driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef _SD_CFG_H
#define _SD_CFG_H

#include "nucleus.h"

#ifdef          __cplusplus
    extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Maximum number of devices (cards) supported by this function */
#define NU_SDIO_FILE_MAX_DEVICES        2

/* Define for issuing CMD12 and CMD13 exclusively after data write
   rather by setting appropriate flags for SDIO stack to issue these. */
#define NU_SDIO_FILE_MANUAL_CMD12_13   CFG_NU_OS_DRVR_SDIO_FUNC_SD_MAN_CMD12_13

/* Define the size of standard block size (bytes). Currently
   Nucleus File only supports 512 byte. */

#define NU_SDIO_FILE_STD_BLKSIZE        512

/* If the controller supports multi-block reads and writes, set this to NU_TRUE. */
#ifndef NU_SDIO_SUPPORT_MULTIBLOCK
    #define NU_SDIO_SUPPORT_MULTIBLOCK CFG_NU_OS_DRVR_SDIO_FUNC_SD_MULTI_BLOCK
#endif

#ifdef          __cplusplus
    }
#endif /* _cplusplus */

#endif   /* ! _SD_CFG_H */
