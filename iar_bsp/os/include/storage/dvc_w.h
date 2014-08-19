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
*       dvc_w.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Storage Device Manager Wrapper
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       The Nucleus Device Manager interface requirements are complied 
*       with through this layer.                                 
*                                                                       
* DATA STRUCTURES                                                       
*       
*       FDRV_LOG_CB_S       Logical control block registered with
*                           NU_Create_File_Device                                                                
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None                      
*                                                                       
*************************************************************************/
#ifndef DVC_W_H
#define DVC_W_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

#include "kernel/dev_mgr.h"
#include "storage/dev_extr.h"

typedef struct fdrv_dm_cb
{
    DV_DEV_HANDLE dh;               /* The physical disk device handle */
    DV_DEV_ID     dev_id;           /* The device identifier */
} FDRV_DM_CB_S; 

/* This structure is used to define drive charcteristics */
typedef struct fdrv_log_cb_struct
{
    CHAR    fdrv_name[FILE_MAX_DEVICE_NAME];/* The device name provided by the driver */
    UINT16  fdrv_vfs_dh;                    /* Physical VFS disk handle. */
    UINT32  fdrv_start;                     /* Partition start sector address */
    UINT32  fdrv_end;                       /* Partition end sector address */
    INT     fdrv_ptype;                     /* Partition type (Primary, extended, or logical) */
    INT     fdrv_pid;                       /* Partition type identifier */
} FDRV_LOG_CB_S;

/* This structure is used to define physical drive characteristics */
typedef struct fdrv_phy_cb_struct
{
    FDRV_DM_CB_S  fdrv_dm_cb;
    UINT32        fdrv_bytes_p_sec;
    VOID *        fdrv_spec;                /* Device specific data */ 
} FDRV_PHY_CB_S;


STATUS vfs_dvc_open(UINT16 dh, CHAR *devname, VOID *args);
STATUS vfs_dvc_io_wrapper(UINT16 dh, UINT32 sector, VOID *buffer, UINT16 count, INT reading);
STATUS vfs_dvc_close_wrapper(UINT16 dh);
STATUS vfs_dvc_ioctl_wrapper(UINT16 dh, UINT16 command, VOID *buffer, INT ioctl_data_len);
STATUS vfs_dvc_dskchk_wrapper(UINT16 dh);

STATUS vfs_dvc_log_io_wrapper(UINT16 dh, UINT32 sector, VOID *buffer, UINT16 count, INT reading);
STATUS vfs_dvc_log_close_wrapper(UINT16 dh);
STATUS vfs_dvc_log_open_wrapper(UINT16 dh, CHAR *devname, VOID *args);
STATUS vfs_dvc_log_ioctl_wrapper(UINT16 dh, UINT16 command, VOID *buffer, INT ioctl_data_len);
STATUS vfs_dvc_log_dskchk_wrapper(UINT16 dh);


#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* DVC_W_H */

