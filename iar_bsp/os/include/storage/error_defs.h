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
*       error_defs.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Error
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains defines and structure definitions for error reporting 
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
#ifndef ERROR_DEFS_H
#define ERROR_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */



/* Internal Error codes */
#define PCERR_FAT_FLUSH         0       /* Cant flush FAT */
#define PCERR_FAT_NULLP         1       /* Flushfat called with null pointer */
#define PCERR_NOFAT             2       /* No FAT type in this partition., Can't Format */
#define PCERR_FMTCSIZE          3       /* Too many clusters for this partition., Can't Format */
#define PCERR_FMTFSIZE          4       /* File allocation Table Too Small, Can't Format */
#define PCERR_FMTRSIZE          5       /* Numroot must be an even multiple of INOPBLOCK */
#define PCERR_FMTWBZERO         6       /* Failed writing block 0  */
#define PCERR_FMTWFAT           7       /* Failed writing FAT block */
#define PCERR_FMTWROOT          8       /* Failed writing root block */
#define PCERR_FMT2BIG           9       /* Total sectors may not exceed 64k.  */
#define PCERR_FSTOPEN           10      /* pc_free_all freeing a file */
#define PCERR_INITMEDI          11      /* Not a DOS disk:pc_dskinit */
#define PCERR_INITDRNO          12      /* Invalid driveno to pc_dskinit */
#define PCERR_INITCORE          13      /* Out of core:pc_dskinit */
#define PCERR_INITDEV           14      /* Can't initialize device:pc_dskinit */
#define PCERR_INITREAD          15      /* Can't read block 0:pc_dskinit */
#define PCERR_BLOCKCLAIM        16      /* PANIC: Buffer Claim */
#define PCERR_INITALLOC         17      /* Fatal error: Not enough core at startup */
#define PCERR_BLOCKLOCK         18      /* Warning: freeing a locked buffer */
#define PCERR_FREEINODE         19      /* Bad free call to freei */
#define PCERR_FREEDROBJ         20      /* Bad free call to freeobj */
#define PCERR_FATCORE           21      /* "Not Enough Core To Load FAT" */
#define PCERR_FATREAD           22      /* "IO Error While Failed Reading FAT" */
#define PCERR_BLOCKALLOC        23      /* "Block Alloc Failure Memory Not Initialized" */
#define PCERR_DROBJALLOC        24      /* "Memory Failure: Out of DROBJ Structures" */
#define PCERR_FINODEALLOC       25      /* "Memory Failure: Out of FINODE Structures" */
/* The next two are only relevant in a multi tasking environment */
#define PCERR_USERS             26      /* Out of User structures increase CFG_NU_OS_STOR_FILE_VFS_NUM_USERS */
#define PCERR_BAD_USER          27      /* Trying to use the API without calling 
                                          pc_rtfs_become_user() first */
#define PCERR_NO_DISK           28      /* No disk      */
#define PCERR_DISK_CHANGED      29      /* Disk changed */
#define PCERR_DRVALLOC          30      /* Memory Failure: Out of DDRIVE Structures */

#define PCERR_PATHL             31      /* Path name too long */

/* Errno values */
#define PEBADF          9               /* Invalid file descriptor*/
#define PENOENT         2               /* File not found or path to file not found*/
#define PEMFILE         24              /* No file descriptors available (too many
                                           files open)*/
#define PEEXIST         17              /* Exclusive access requested but file
                                           already exists.*/
#define PEACCES         13              /* Attempt to open a read only file or a
                                           special (directory)*/
#define PEINVAL         22              /* Seek to negative file pointer attempted.*/
#define PENOSPC         28              /* Write failed. Presumably because of no space */
#define PESHARE         30              /* Open failed do to sharing */

#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* ERROR_DEFS_H */
