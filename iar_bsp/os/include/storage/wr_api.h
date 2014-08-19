/*************************************************************************/
/*                                                                       */
/*               Copyright 2007 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/************************************************************************
* FILE NAME
*
*       wr_api.h
*
* COMPONENT
*
*       Safe
*
* DESCRIPTION
*
*       Safe's wrapper function declarations used to interface with 
*       the VFS layer.
*
* DATA STRUCTURES
*
*       WR_SAFE_DEV_CB                      VFS layer control block 
*                                           for managing data between 
*                                           VFS and SAFE.
*       WR_SAFE_MNT_PARAMS                  Mount specific parameters needed by
*                                           Safe file system.
*           
* FUNCTIONS
*
*       None.
*
*************************************************************************/

#ifndef __WR_API_H__
#define __WR_API_H__ 1

#ifdef          __cplusplus
    extern  "C" {                           /* C declarations in C++ */
#endif /* _cplusplus */

#include "nucleus.h"       
#include "storage/pcdisk.h"
#include "storage/fd_defs.h"
#include "storage/fsdev_extr.h"
#include "storage/fsf.h"

/* Verify Safe does not limit VFSs max path length */
#if (F_MAXPATH < EMAXPATH)
 #error "Safe's max path length (F_MAXPATH) must be >= VFS's max path length (EMAXPATH)"
#endif

/* Set to true to have device formatted when calling flashes init device function.
   Below are the following flash types supported and there init device functions:
   Dataflash -> sdrv_df_init_device
   NAND ------> sdrv_nand_init_device
   NOR -------> sdrv_nor_init_device. 
   This is necessary for the file demo to run consecutively without errors on persistent 
   devices (NOR, NAND, etc.). Setting to true will format the device, erasing 
   any previous data. */
#ifndef SAFE_AUTOFORMAT_FLASH
 #define SAFE_AUTOFORMAT_FLASH NU_FALSE
#endif 

/* Safe file system mount specific parameters. */
typedef struct _safe_mnt_params
{
    VOID *buffer;					/* Pointer for a buffer area used by Safe’s generic driver. */
    INT32 buffsize;    				/* Size of buffer. */
    FS_DRVMOUNT mountfunc;			/* Pointer to a Safe device’s generic mount function. */
    FS_PHYGETID phyfunc;			/* Pointer to a physical driver function for the desired device. 
									   This is called by the mountfunc and fills out FS_FLASH.*/

} WR_SAFE_MNT_PARAMS, WR_SAFE_FMT_PARAMS; 

/* Safe driver control block flag definition */
#define WR_SAFE_DEV_CB_FL_VALID   0x1U

/* Wrapper layer control block for managing data between VFS and SAFE. 
   This structure is used by both physical and logical devices. */
typedef struct _wr_safe_dev_cb{
    
    WR_SAFE_MNT_PARAMS safe_mnt_params;				/* Safe specific mount parameters. This is needed 
													   due to some operations in Safe requiring the device 
													   to be mounted, where as the VFS doesn’t. */
	UINT32 phy_flag;                                /* Flag used to indicate if the physical control block is in use. */
	UINT32 log_flag;                                /* Flag used to indicate if the logical control block is in use. */
    long (*getmem_func)(FS_PHYGETID phyfunc);       /* Pointer to a Safe device generic function used to 
													   determine how much memory to allocate for the specific 
													   driver. Memory requirements are based off the contents 
													   stored in FS_FLASH structure. */
} WR_SAFE_DEV_CB;


/* Wrapper layer specific flags for controlling if a disk
   is read/write only, or neither. */
#define SAFE_WR_PERM_OF_NONE          0x00U
#define SAFE_WR_PERM_OF_WRONLY        0x01U    /* Wrapper layer permissions to prevent
                                                  read operations, when file opened for PO_WRONLY. */
#define SAFE_WR_PERM_OF_RDONLY        0x02U    /* Wrapper layer permissions to prevent
                                                  write operations, when file opened for PO_RDONLY. */

STATUS WR_Safe_Init_FS(char *fsname);
#if(USE_VFS)
STATUS wr_safe_init(VOID);
STATUS wr_safe_uninit(VOID);
STATUS wr_safe_mount(UINT16 dh,VOID *config);
STATUS wr_safe_unmount(UINT16 dh);
STATUS wr_safe_disk_abort(UINT16 dh);

INT    wr_safe_open(UINT16 dh, CHAR *name, UINT16 flag, UINT16 mode);
INT32  wr_safe_read(INT fd, CHAR *buf, INT32 count);
INT32  wr_safe_write(INT fd, CHAR *buf, INT32 count);
INT32  wr_safe_seek(INT fd, INT32 offset, INT16 origin);
STATUS wr_safe_close(INT fd);
STATUS wr_safe_delete(CHAR *name);
STATUS wr_safe_utime (DSTAT *statobj,UINT16 access_date,UINT16 access_time,
                            UINT16 update_date,UINT16 update_time,UINT16 create_date,
                            UINT16 create_time);
STATUS wr_safe_set_attr(CHAR *name, UINT8 newattr);
STATUS wr_safe_get_attr(UINT8 *attr, CHAR *name);
STATUS wr_safe_truncate(INT fd, INT32 offset);
STATUS wr_safe_flush(INT fd);
STATUS wr_safe_mkdir(CHAR *name);
STATUS wr_safe_rmdir(CHAR *name);
STATUS wr_safe_rename(CHAR *name, CHAR *newname);
STATUS wr_safe_get_first(DSTAT *statobj, CHAR *pattern);
STATUS wr_safe_get_next(DSTAT *statobj);
STATUS wr_safe_done(DSTAT *statobj);

STATUS wr_safe_format(UINT16 dh, VOID **params);
STATUS wr_safe_get_format_info(UINT16 dh, VOID **params);
STATUS wr_safe_freespace(UINT16 dh, UINT8 *secpcluster,
                       UINT16 *bytepsec, UINT32 *freecluster,
                       UINT32 *totalcluster);

STATUS wr_safe_vnode_allocate(UINT16 dh, CHAR *path, VOID **fsnode);
STATUS wr_safe_vnode_deallocate(UINT16 dh, VOID *fsnode);
STATUS wr_safe_fsnode_to_string(UINT16 dh, VOID *fsnode, CHAR *string);

VOID   wr_safe_release (UINT16 dh);
VOID   wr_safe_reclaim (UINT16 dh);
#endif

#ifdef          __cplusplus
    }
#endif /* _cplusplus */

#endif   /* __WR_API_H__ */
