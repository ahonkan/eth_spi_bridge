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
*       sd_imp.h
*
* COMPONENT
*
*       Nucleus SDIO FILE Function Driver's Atheros SDIO Stack Communication
*       layer. 
*
* DESCRIPTION
*
*       This file contains SDIO Stack communication layer implementation
*       of Nucleus SDIO FILE Function Driver.
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
*       ctsystem.h
*       sdio_busdriver.h
*       _sdio_defs.h
*       sdio_lib.h
*
*************************************************************************/
#ifndef _SD_IMP_H
#define _SD_IMP_H

/* SDIO stack specific include files. */
#include "connectivity/ctsystem.h"
#include "connectivity/sdio_busdriver.h"
#include "connectivity/_sdio_defs.h"
#include "connectivity/sdio_lib.h"

#ifdef          __cplusplus
    extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Misc defines */
#define NU_SDIO_SINGLE_BLOCK 1

/* Define SD/MMC command. */

/* Basic commands (class 0) Mandatory */

#define SD_CMD00                        0x00
#define MMC_CMD01                       0x01
#define SD_CMD02                        0x02
#define SD_CMD03                        0x03
#define SD_CMD04                        0x04
#define SD_CMD07                        0x07
#define SD_CMD08                        0x08
#define SD_CMD09                        0x09
#define SD_CMD10                        0x0A
#define SD_CMD12                        0x0C
#define SD_CMD13                        0x0D
#define SD_CMD15                        0x0F

/* Block oriented read commands (class 2) Mandatory */

#define SD_CMD16                        0x10
#define SD_CMD17                        0x11
#define SD_CMD18                        0x12

/* Block oriented write commands (class 4) Mandatory */

#define SD_CMD24                        0x18
#define SD_CMD25                        0x19
#define SD_CMD27                        0x1B

/* Block oriented write protection commands (class 6) Option */

#define SD_CMD28                        0x1C
#define SD_CMD29                        0x1D
#define SD_CMD30                        0x1E

/* Erase commands (class 5) Mandatory */

#define SD_CMD32                        0x20
#define SD_CMD33                        0x21
#define SD_CMD38                        0x26

/* Lock card (class 7) Option */
#define SD_CMD42                        0x2A

/* Application specific commands (class 8) Mandatory */

#define SD_CMD55                        0x37
#define SD_CMD56                        0x38

/* Application specific commands */

#define SD_ACMD06                       0x06
#define SD_ACMD13                       0x0D
#define SD_ACMD22                       0x16
#define SD_ACMD23                       0x17
#define SD_ACMD41                       0x29
#define SD_ACMD42                       0x2A
#define SD_ACMD51                       0x33

/* Define Card Specific Data(CSD byte masks). */

#define CSD_STRUCTURE                   0xC0
#define CSD_SPEC_VERS                   0x3C
#define CSD_TACC                        0xFF
#define CSD_NSAC                        0xFF
#define CSD_CCC_LOW                     0x0F
#define CSD_READ_BL_LEN                 0x0F
#define CSD_C_SIZE_HGH                  0x03
#define CSD_C_SIZE_LOW                  0xE0
#define CSD_C_SIZE_MULT_HGH             0x03
#define CSD_FILE_FORMAT                 0x06
#define CSD_C_SIZE_MULT_LOW             0x80
#define CSD_R2W_FACTOR                  0x1C

/* CSD byte bit positions. */

#define CSD_READ_BL_PARTIAL             0x80
#define CSD_WRITE_BLK_MISALIGN          0x40
#define CSD_READ_BLK_MISALIGN           0x20
#define CSD_DSR_IMP                     0x10
#define CSD_ERASE_BLK_EN                0x40
#define CSD_WP_GRP_ENABLED              0x80
#define CSD_WRITE_BL_PARTIAL            0x20
#define CSD_FILE_FORMAT_GRP             0x80
#define CSD_COPY                        0x40
#define CSD_PERM_WRITE_PROTECT          0x20
#define CSD_TMP_WRITE_PROTECT           0x10

/* Define CSD_STRUCTURE in SD's Card Specific Data. */

#define SD_CSD_STR_VERSION_1_0          0       /* Version 1.0 and version 2.0 standard capacity */
#define SD_CSD_STR_VERSION_2_0          1       /* Version 2.0  high capacity. */
#define SD_CSD_STR_VERSION              SD_CSD_STR_VERSION_2_0

/* Define CSD_STRUCTURE in MMC's Card Specific Data. */

#define MMC_CSD_STR_VERSION_1_0         0       /* Version 1.0 */
#define MMC_CSD_STR_VERSION_1_1         1       /* Version 1.1 */
#define MMC_CSD_STR_VERSION_1_2         2       /* Version 1.2 */
#define MMC_CSD_STR_VERSION             MMC_CSD_STR_VERSION_1_2

/* Define SD Driver Error codes.
   MMC/SD memory card error code:  -3300 - -3399 */

#define NUF_SD_NUM_LOGICAL              -3300   /* Unknown Logical drive error. */
#define NUF_SD_NUM_PHYSICAL             -3301   /* Never used - pre release 3.1 error. */
#define NUF_SD_LOG_CTRL                 -3302   /* LOG_DISK_CTRL setup error. */
#define NUF_SD_PHYS_CTRL                -3103   /* PHYS_DISK_CTRL setup error(Reserved). */
#define NUF_SD_NOT_LOG_OPENED           -3304   /* Logical drive is not opened. */
#define NUF_SD_NOT_PHYS_OPENED          -3305   /* Physical drive is not opened(Reserved). */
#define NUF_SD_UNKNOWN_CARD             -3306   /* Unknown card. */
#define NUF_SD_RCA_RETRY                -3307   /* Tried to RCA_MAX_RETRY, but it does not get RCA. */
#define NUF_SD_NO_MEMCARD               -3308   /* SD card is not memory card. */
#define NUF_SD_SECURED_MODE             -3309   /* SD card is in secured mode of operation. */

#define NUF_SD_CSD_STRUCTURE_ERROR      -3310   /* Invalid CSD structure version. */
#define NUF_SD_SCR_STRUCTURE_ERROR      -3311   /* Invalid SCR structure version. */
#define NUF_SD_SCR_SD_SPEC_ERROR        -3312   /* Invalid SD specification version(SRC). */
#define NUF_SD_FILE_FORMAT              -3313   /* Not supported this disk format. */
#define NUF_SD_NO_DOSPART               -3314   /* NO DOS partition in disk. */
#define NUF_SD_PARTITIONS               -3315   /* SD has many partition tables. */

#define NUF_SD_CS_ERROR                 -3316   /* Card status error. */
#define NUF_SD_SEC_COUNT                -3317   /* Invalid sector count. */
#define NUF_SD_OVER_PART                -3318   /* Over the partition end sectors. */
#define NUF_SD_CSTATE_ERROR             -3319   /* Current state error. */
#define NUF_SD_EVENT_TIMEOUT            -3320   /* SD command event timeout. */

#define NUF_MMC_CSD_STRUCTURE_ERROR     -3330   /* Invalid CSD structure version. */
#define NUF_MMC_CSD_SPEC_VERS_ERROR     -3331   /* Invalid MMC specification version(CSD). */
#define NUF_NO_DRIVE_HANDLE             0xFD04  /* Drive handle not set (-3332). */

BOOLEAN NU_SDIO_FDR_Imp_Probe(SDFUNCTION *function, SDDEVICE *handle);

VOID NU_SDIO_FDR_Imp_Remove(SDFUNCTION *function, SDDEVICE *handle);

STATUS NU_SDIO_FDR_Imp_Init(VOID);

STATUS NU_SDIO_FDR_Imp_RDWR( SDDEVICE *device,
                             UINT32 offset,
                             UINT8 *buffer,
                             UINT32 count,
                             UINT32 request);

VOID NU_SDIO_FDR_Imp_Close(VOID);

#ifdef          __cplusplus
    }
#endif /* _cplusplus */

#endif   /* ! _SD_IMP_H */
