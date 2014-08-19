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
*       fs_defs.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       File System Operations
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains generic file system structure definition
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
#include "storage/dir_defs.h"

#ifndef FS_DEFS_H
#define FS_DEFS_H 1

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* This file contains definitions, typedefs, macros, that have not been completed */
typedef struct fs_struct
{
    STATUS (*fs_init)       (VOID);
    STATUS (*fs_uninit)     (VOID);    
    
    STATUS (*fs_mount)      (UINT16 dh, VOID* config);
    STATUS (*fs_unmount)    (UINT16 dh);
    STATUS (*fs_disk_abort) (UINT16 dh);                        /* NU_Disk_Abort */
#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)
    STATUS (*fs_check_disk) (UINT16 dh,UINT8 flag,UINT8 mode); /* NU_Check_Disk */
#endif

    STATUS (*fs_open)   (UINT16 dh, CHAR *name, UINT16 flag, UINT16 mode);  /* NU_Open */
    INT32  (*fs_read)   (INT fd, CHAR *buf, INT32 count);                   /* NU_Read */
    INT32  (*fs_write)  (INT fd, CHAR *buf, INT32 count);                   /* NU_Write */
    INT32  (*fs_seek)   (INT fd, INT32 offset, INT16 origin);               /* NU_Seek */
    STATUS (*fs_close)  (INT fd);                                           /* NU_Close */
    STATUS (*fs_delete) (CHAR *name);                                       /* NU_Delete */
    STATUS (*fs_utime)  (DSTAT *statobj,UINT16 access_date,UINT16 access_time,
                            UINT16 update_date,UINT16 update_time,UINT16 create_date,
                            UINT16 create_time);
    
    STATUS (*fs_set_attr)   (CHAR *name, UINT8 newattr);        /* NU_Set_Attributes */
    STATUS (*fs_get_attr)   (UINT8 *attr, CHAR *name);          /* NU_Get_Attributes */
    STATUS (*fs_truncate)   (INT fd, INT32 offset);             /* NU_Truncate */
    STATUS (*fs_flush)      (INT fd);                           /* NU_Flush */
    
    STATUS (*fs_mkdir)  (CHAR *name);                           /* NU_Make_Dir */
    STATUS (*fs_rmdir)  (CHAR *name);                           /* NU_Remove_Dir */
    STATUS (*fs_rename) (CHAR *name, CHAR *newname);            /* NU_Rename */

    STATUS (*fs_get_first)  (DSTAT *statobj, CHAR *pattern);    /* NU_Get_First */
    STATUS (*fs_get_next)   (DSTAT *statobj);                   /* NU_Get_Next */
    STATUS (*fs_done)       (DSTAT *statobj);                   /* NU_Done */

    
    STATUS (*fs_format)   (UINT16 dh, VOID **params);           /* NU_Format */
    STATUS (*fs_get_format_info)(UINT16 dh, VOID **params);     /* NU_Get_Format_Info */

    STATUS (*fs_freespace)(UINT16 dh, UINT8 *secpcluster,       /* NU_Freespace */
                           UINT16 *bytepsec, UINT32 *freecluster,
                           UINT32 *totalcluster);

    STATUS (*fs_vnode_allocate)     (UINT16 dh, CHAR *path, VOID **fsnode);
    STATUS (*fs_vnode_deallocate)   (UINT16 dh, VOID *fsnode);
    STATUS (*fs_fsnode_to_string)   (UINT16 dh, VOID *fsnode, CHAR *string);

    VOID   (*fs_release) (UINT16 dh);   /* Release FS locks for yielding */
    VOID   (*fs_reclaim) (UINT16 dh);   /* Reclaim FS locks after yielding */

} FS_S;


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* FS_DEFS_H */
