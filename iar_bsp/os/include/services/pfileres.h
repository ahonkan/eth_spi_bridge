/************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME
*
*       pfileres.h
*
* COMPONENT
*
*       PFILE - POSIX File System
*
* DESCRIPTION
*
*       This file contains the default routines for process resource
*       management for file system.
*
* DATA STRUCTURES
*
*       PPROC_FILE_RES
*       PPROC_AIO_RES
*       PPROC_FS_RES
*
* DEPENDENCIES
*
*       pdir.h
*
************************************************************************/
#ifndef __PFILERES_H_
#define __PFILERES_H_

#include "services/pdir.h"

/* File System and Networking Resource */
typedef struct _pproc_file_res_struct
{
    POSIX_PROC_FD       *psx_open_fd_tbl;
    PSX_OPEN_DIR        *psx_open_dir_tbl;
    NU_SEMAPHORE        *psx_open_file_mtx;
    NU_SEMAPHORE        *psx_open_dir_mtx;
	CS_NODE             *psx_open_mmap_list_head;
#if (_POSIX_THREADS !=  POSIX_FALSE)
    pthread_t           *psx_pfile_reg_thds;
#endif /* (_POSIX_THREADS !=  POSIX_FALSE) */
}PPROC_FILE_RES;

/* AIO event resource */
#if ((defined(POSIX_INCLUDE_AIO)) && (POSIX_INCLUDE_AIO > 0))
#if (_POSIX_ASYNCHRONOUS_IO !=  -1)
typedef struct _pproc_aio_res_struct
{
    NU_EVENT_GROUP    *psx_aio_suspend_group_ptr;
}PPROC_AIO_RES;
#endif  /* (_POSIX_ASYNCHRONOUS_IO  !=  -1) */
#endif  /* (POSIX_INCLUDE_AIO)) && (POSIX_INCLUDE_AIO > 0) */

/* posix fs resource */
typedef struct _pproc_fs_res_struct
{
    PPROC_FILE_RES          file_res;
#if ((defined(POSIX_INCLUDE_AIO)) && (POSIX_INCLUDE_AIO > 0))
#if (_POSIX_ASYNCHRONOUS_IO !=  -1)
    PPROC_AIO_RES           aio_res;
#endif  /* (_POSIX_ASYNCHRONOUS_IO  !=  -1) */
#endif  /* (POSIX_INCLUDE_AIO)) && (POSIX_INCLUDE_AIO > 0) */
}PPROC_FS_RES;

/* The following are all pointer mappings of all the resources defined above */
/* File System  */
#define PSX_OPEN_FD_TBL                         psx_file_res->file_res.psx_open_fd_tbl
#define PSX_OPEN_DIR_TBL                        psx_file_res->file_res.psx_open_dir_tbl
#define PSX_OPEN_FILE_MTX                       psx_file_res->file_res.psx_open_file_mtx
#define PSX_OPEN_DIR_MTX                        psx_file_res->file_res.psx_open_dir_mtx
#define PSX_OPEN_MMAP_LIST_HEAD                 psx_file_res->file_res.psx_open_mmap_list_head
#if (_POSIX_THREADS !=  POSIX_FALSE)
#define PSX_PFILE_REG_THDS                      psx_file_res->file_res.psx_pfile_reg_thds
#endif /* (_POSIX_THREADS !=  POSIX_FALSE) */

/* AIO */
#if ((defined(POSIX_INCLUDE_AIO)) && (POSIX_INCLUDE_AIO > 0))
#if (_POSIX_ASYNCHRONOUS_IO !=  -1)
#define PSX_AIO_SUSPEND_GROUP_PTR               psx_file_res->aio_res.psx_aio_suspend_group_ptr
#endif  /* (_POSIX_ASYNCHRONOUS_IO  !=  -1) */
#endif  /* (POSIX_INCLUDE_AIO)) && (POSIX_INCLUDE_AIO > 0) */

/* File resources */
extern VOID * psx_static_file_res;

#endif /* __PFILE_RES_H_ */
