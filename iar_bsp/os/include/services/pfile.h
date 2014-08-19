/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       pfile.h
*
* COMPONENT
*
*       PFS - POSIX file system
*
* DESCRIPTION
*
*       Contains the internal routines to be used by Nucleus POSIX File
*       system.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       "posix.h"                           Contains the Nucleus POSIX
*                                           internal routines.
*       "fcntl.h"                           file control related
*                                           definitions.
*       "pfile_error.h"
*       "stdio.h"
*       "pnet.h"
*       "dirent.h"
*       "psx_defs.h"
*
*************************************************************************/
#ifndef __POSIX_FILE_H_
#define __POSIX_FILE_H_

#include    "services/posix.h"
#include    "services/fcntl.h"
#include    "services/pfile_error.h"
#include    "services/stdio.h"
#include    "services/pnet.h"
#include    "services/dirent.h"
#include    "services/psx_defs.h"

/* Include Nucleus FILE support */
#include    "storage/pcdisk.h"

#define WRITE_ENABLED(flag) ((flag >> 7)&(0x01))
#define READ_ENABLED(flag)  ((flag >> 8)&(0x01))
#define SLASH   '/'

/* Static open file name */
#if POSIX_STATIC_FOPEN
typedef struct _pfile_fat_path_max_struct
{
    CHAR pathmax[POSIX_FAT_PATH_MAX];
}PFILE_FAT_PATH_MAX;
#endif

/* Mmap structure used to keep track of mapped memory regions*/
typedef struct posix_mmap
{
    CS_NODE         mmap_list;                  /* Linked List pointers */
    void*           addr;                       /* Address of mapped region */
    size_t          len;                        /* Length of mapped region */
    size_t          off;                        /* Offset into file */
    int             fildes;                     /* File desc for file mapped to region. */
    pid_t           pid;                        /* Process ID of mapped region */
}POSIX_MMAP;

#include    "storage/pcdisk.h"

/* POSIX File Control Block for open files */
/* PATH_MAX contains the full pathname from the root "/" */
/* Formula: Directory path + Filename + "\0" */
typedef struct posix_fcntl
{
    INT             id;                         /* Actual file descriptor  */
    CHAR*           wholename;                  /* Whole path */
    POSIX_SOCKET*   socket;                     /* pointer to socket data */
    mode_t          mode;                       /* POSIX layer mode */
    UINT16          dup_cntr;                   /* duplicate file counter */
    UINT16          pad;
}POSIX_FCNTL;

typedef struct posix_proc_fd
{
    INT             flags;                      /* POSIX layer flags  */
    POSIX_FCNTL*    open_file;                  /* Pointer to standard I/O */
    PSX_RTL_FLOCK*  flock_p;                    /* pointer to file lock and buffered stream */
}POSIX_PROC_FD;

#include    "services/pfileres.h"

/* PATH_MAX can not be less than EMAXPATH */
#if (PATH_MAX < EMAXPATH)
#error Please set _POSIX_PATH_MAX >= EMAXPATH within services/limits.h
#endif /* (PATH_MAX < EMAXPATH) */


#ifdef __cplusplus
extern "C" {
#endif

VOID            PFS_File_Deregisters_Thread(PPROC_FS_RES *psx_file_res, pthread_t t);
STATUS          PFS_File_Change_Drive(CHAR* newpath);
INT             is_registered(PPROC_FS_RES *psx_file_res, pthread_t t);
VOID            register_thread(PPROC_FS_RES *psx_file_res, pthread_t t);
INT16           change_to_fat(CHAR *new_path, CHAR *old_path);
VOID            pfile_replace_backslash_to_slash(UINT8 *string);
VOID            pfile_deallocate_desc(PPROC_FS_RES *psx_file_res,INT posix_desc);
INT             pfile_allocate_desc(pid_t pid,PPROC_FS_RES *psx_file_res,UINT8 type);
INT             pfile_get_actual_desc(PPROC_FS_RES *psx_file_res,INT desc);
POSIX_PROC_FD*  pfile_get_open(PPROC_FS_RES *psx_file_res,INT actual_desc);
INT             pfile_get_flag(PPROC_FS_RES *psx_file_res,INT fd);
STATUS          pfile_get_dirpath(pid_t pid,CHAR *oldpath, CHAR *currpath);
STATUS          pfile_get_filename(CHAR *wholepath, CHAR *filename);
VOID            pfile_set_close(PPROC_FS_RES *psx_file_res,INT fd);
VOID            pfile_insert_dup(PPROC_FS_RES *psx_file_res,INT old_fd,INT new_fd);
INT             pfile_open_file(pid_t pid,PPROC_FS_RES *psx_file_res,INT posix_desc, INT actual_desc, CHAR *new_path,
                                INT flags, mode_t mod, POSIX_SOCKET *socket);
INT             pfile_str_to_fd(PPROC_FS_RES *psx_file_res,CHAR* path);
UINT16          pfile_conv_flag(INT oflags);
STATUS          pfile_get_wholepath(pid_t pid,CHAR* opath, CHAR* npath);
STATUS          pfile_cmp_path(CHAR old_str[],CHAR new_str[],INT length);
POSIX_SOCKET*   pfile_get_socket(PPROC_FS_RES *psx_file_res,INT posix_desc);
VOID*           pfile_memset(VOID *s,INT c, UINT32 n);
#if (_POSIX_SYNCHRONIZED_IO != POSIX_FALSE)
INT             pfile_sync(PPROC_FS_RES *psx_file_res,OPTION *old_pre,OPTION *old_prio);
VOID            pfile_unsync(PPROC_FS_RES *psx_file_res,OPTION *old_pre,OPTION *old_prio);
#endif /* _POSIX_SYNCHRONIZED_IO */
VOID            *async_io_thread(VOID *argv);
/* Directory operations */
PSX_OPEN_DIR*   pfile_allocate_dir_desc(pid_t pid,PPROC_FS_RES* psx_file_res,INT *dir_id);
VOID            pfile_deallocate_dir_desc(PPROC_FS_RES* psx_file_res,INT posix_desc);
PSX_OPEN_DIR*   pfile_get_dir(PPROC_FS_RES* psx_file_res,INT dir_id);
INT             POSIX_SYS_FS_Closedir_Entry(PSX_OPEN_DIR* open_dir);
/* RTL stdio operations */
STATUS          POSIX_SYS_RTL_Insert_Iobuf(PPROC_FS_RES *psx_file_res,PSX_RTL_FLOCK* fio_entry);
PSX_RTL_FLOCK*  POSIX_SYS_RTL_Get_Flock_Entry(PPROC_FS_RES* psx_file_res,INT fd);

#ifdef __cplusplus
}
#endif

#endif /*  __POSIX_FILE_H_  */




