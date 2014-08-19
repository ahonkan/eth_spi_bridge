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
*       fsl_defs.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       FSL
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains defines and structure definitions for mount table 
*       services.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#include "storage/fs_defs.h"
#include "storage/dev_defs.h"
#include "storage/encod_defs.h"

#ifndef FSL_DEFS_H
#define FSL_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Definition of a mount table entry. This structure is the control block
   for each instance of a logical storage device currently available to 
   the system 
*/
#define MTE_MAX_TABLE_SIZE  CFG_NU_OS_STOR_FILE_VFS_MAX_MOUNT_POINTS
#define MTE_NO_ROOM -1
#define NO_DRIVE -1

#define MTE_MAX_MOUNT_NAME  2
#define MTE_MAX_DEVICE_NAME FILE_MAX_DEVICE_NAME

#define MTE_FL_VALID        0x01
#define MTE_FL_MOUNTED      0x04

typedef struct mte_struct
{
    FS_S    *mte_fs;                /* FS_S for this mount table entry */
    struct fste_struct  *mte_fste;  /* FS table entry used */
    FDEV_S  *mte_fdev;              /* Device to be used for this MTE */
    UINT32  mte_flags;
    UINT32  mte_cnt;                /* Usage counter */
    INT16   mte_drive;              /* drive number a=A=0,b=B=1,... */
    UINT16  mte_dh;                 /* Disk handle */
    UINT32  mte_last;               /* Last element from listing, only used in 
                                       get first/next/done operations */
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)
    UNSIGNED mte_file_open_count;   /* Used for silent flushing when device layer cache
                                       is enabled on the device */
#endif
    UINT32  mte_drive_id;           /* Unique ID for determining validity */                                       
    CHAR    mte_mount_name[MTE_MAX_MOUNT_NAME];
    CHAR    mte_device_name[MTE_MAX_DEVICE_NAME];
    CPTE_S  *mte_cp;                /* Contains codepage specific information about this mount entry */  

} MTE_S;

#ifdef          __cplusplus
}

#endif /* _cplusplus */

#endif /* FSL_DEFS_H */
