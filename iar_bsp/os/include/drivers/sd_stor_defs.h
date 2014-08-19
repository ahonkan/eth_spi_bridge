/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
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
*       sd_stor_defs.h
*
* COMPONENT
*
*       Nucleus SDIO FILE Function Driver - Storage Definations
*
* DESCRIPTION
*
*       This file defines all the macros and structures of 'Storage'
*       component which are required by the SD driver. This file is used
*       to build SD component independent of Storage.
*
* DATA STRUCTURES
*
*       FPART_DISK_INFO_S
*
* DEPENDENCIES
*
*       nucleus.h
*
*************************************************************************/
#ifndef _SD_STOR_DEFS_H
#define _SD_STOR_DEFS_H

#include "nucleus.h"

#ifdef          __cplusplus
    extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Max number of bytes for a device name */
#define FILE_MAX_DEVICE_NAME    6

/* FILE DM definitions */
#define STORAGE_LABEL           {0xb8,0x2f,0x9c,0x12,0xee,0xa1,0x41,0x07,0x86,0x62,0xb4,0x01,0x26,0xc8,0x04,0xd3}
#define STORAGE_CMD_BASE        (DV_IOCTL0 + 1)
#define TOTAL_STORAGE_IOCTLS    0

/* Disk info flags */
#define FPART_DI_LBA_SUP        0x01         /* Bit one set to one indicates the device can use LBA */
#define FPART_DI_RMVBL_MED      0x02         /* Bit two set to one indicates a removable device */
#define FPART_DI_RAMDISK        0x04         /* Bit three set to one indicates a ram disk */
#define FPART_DI_SAFE           0x08         /* Bit four set to one indicates a SAFE device */

#define NUF_IO_ERROR            -3035        /* Driver IO function routine returned error */
#define NUF_IN_USE              -3037        /* Resource is in use, prevents requested operation */

/* Physical disk info structure. */
typedef struct file_part_disk_info_struct
{
    CHAR   fpart_name[FILE_MAX_DEVICE_NAME]; /* Device name defined by driver */
    UINT32 fpart_tot_sec;                    /* Total number of physical sectors */
    UINT32 fpart_bytes_p_sec;                /* Number of bytes per sector */
    UINT32 fpart_cyls;                       /* Total number of cylinders. Valid range is 0 - 1024 */
    UINT32 fpart_heads;                      /* Number of heads per cylinder. Valid range is 0 - 255 */
    UINT32 fpart_secs;                       /* Number of sectors per track. Valid range is 1 - 63 */
    UINT32 fpart_flags;                      /* Bit fields are defined below. */
    VOID*  fpart_spec;                       /* Pointer for the driver to pass file system specific info */
} FPART_DISK_INFO_S;

#ifdef          __cplusplus
    }
#endif /* _cplusplus */

#endif   /* ! _SD_STOR_DEFS_H */
