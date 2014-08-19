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
*       fd_defs.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       File Descriptor
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Defines and structure definitions for file descriptor management 
*       and services.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       FD_S                        Generic file descriptor structure
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#include "storage/fsl_defs.h"
#include "storage/file_cfg.h"
#include "storage/pcdisk.h"

#ifndef FD_DEFS_H
#define FD_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Max number of file descriptors */
extern UINT16 gl_VFS_MAX_OPEN_FILES;
extern UINT16 gl_FD_MAX_FD;

/* Defines for file descriptor structure flags */
#define FD_FL_VALID 0x01    /* This descriptor is valid, if not set means empty */

/* Generic file descriptor structure for converting VFS and FS specific file
   descriptors */
typedef struct fd_struct
{
    INT     fd_fs_handle;   /* File system specific handle */
    MTE_S   *fd_mte;        /* Mount table entry for this file descriptor */
    INT     fd_flags;
    UINT32  fd_drive_id;    /* Unique ID for determining validity */    
} FD_S;

#ifdef          __cplusplus
}

#endif /* _cplusplus */

#endif /* LCK_H */
