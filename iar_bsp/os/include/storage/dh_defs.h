/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*                                                                       
*       dh_defs.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Disk handle
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains defines and structure definitions for disk handle 
*       services.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       FSDH_S                      Disk handle structure
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#ifndef DH_DEFS_H
#define DH_DEFS_H

#include "storage/user_defs.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


/* Configuration parameters for disk handle structure and table */
#define FSDH_MAX_DISK_NAME    FILE_MAX_DEVICE_NAME
#define FSDH_MAX_DISKS        CFG_NU_OS_STOR_FILE_VFS_MAX_DEVICES

/* Opaque disk handle type used for FS and device calls */
typedef int FSDH;       /* Disk handle */

/* Defines for disk handle structure flags */
#define FSDH_F_VALID            0x01

/* Defines for user state */
#define FSDH_USER_CLOSED        0x00
#define FSDH_USER_OPENED        0x01
#define FSDH_USER_ABORTED       0x02

/* Structure definition for the disk handle */
typedef struct fsdh_struct
{
    UINT32  fsdh_flags;
    VOID*   fsdh_fs;        /* Value is FS specific */
    struct fsv_vnode_struct
            *fsdh_cwd_vnodes;    /* Pointer to CWD vnodes per user */
    VOID*   fsdh_dev;       /* Value is Device driver specific */
    VOID*   fsdh_specific;   /* Value is device handler specific and not
                               used by FS or FSL layers */
    struct file_dev_struct 
            *fsdh_fdev;      /* Generic device structure associated with
                               this disk handle */
    NU_SEMAPHORE *fsdh_sema; /* Lock used to prevent concurrent access by
                                drivers sharing hardware resources */
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)
    NU_SEMAPHORE fsdh_vfs_sema; /* Prevent concurrent access when using block cache */
#endif                                
    UINT8  fsdh_user_state[VFS_NUM_USERS_WDU];
    CHAR   fsdh_disk_name[FSDH_MAX_DISK_NAME];
    UINT8  fsdh_opencount;
    UINT8  fsdh_disk_id;           /* Unique disk handle id. */
    UINT16 fsdh_exclusive_access;       /* Boolean used to indicate if a task has exclusive access to the disk.
                                           this should only happen when check disk gets ran. */
} FSDH_S;


#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* DH_DEFS_H */
