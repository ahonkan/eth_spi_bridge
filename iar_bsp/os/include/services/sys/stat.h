/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       stat.h                 
*
*   COMPONENT
*
*		PX - POSIX
*
*   DESCRIPTION
*
*		This file defines the structure of the data returned by the
*		functions fstat() and stat().
*
*
*   DATA STRUCTURES
*		            
*		dev_t								Used for device ID's.
*		ino_t								Used for file serial numbers.
*		mode_t								Used for file attributes.
*	    nlink_t								Used for link counts.
*	    uid_t								Used for User ID's.
*		gid_t								Used for Group ID's.
*	    off_t								Used for file sizes.
*	    time_t								Used for time in seconds.
*	    blksize_t							Used for block sizes.					
*	    blkcnt_t							Used for block counts.
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*       types.h             POSIX types.h definitions
*       time.h              POSIX time.h definitions
*
*************************************************************************/

#ifndef NU_PSX_STAT_H
#define NU_PSX_STAT_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/sys/types.h"
#include "services/sys/time.h"

#ifndef _STAT_H                         
#define _STAT_H

/* For ADS Tools.  */
#ifndef __stat_h
#define __stat_h

/* For Hitachi Tools and TI Tools.  */
#ifndef _STAT
#define _STAT

/* For Paradigm Tools and Microtec Tools.  */
#ifndef __STAT_H
#define __STAT_H

/* For Microsoft Visual C.  */
#ifndef _INC_STAT
#define _INC_STAT

#ifndef __STAT_H_
#define __STAT_H_

/* For Code Sourcery ARM GNU */
#ifndef _STAT_H_
#define _STAT_H_

/* For DIAB tools */
#ifndef __Istat
#define __Istat

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _SYS_STAT_H
#define _SYS_STAT_H

/* File type */
#define S_IFMT		0170000					/* Mask for type of file */
#define S_IFDIR     0040000					/* Directory */
#define S_IFCHR     0020000					/* Character special */
#define S_IFBLK     0060000					/* Block special */
#define S_IFREG     0100000					/* Regular */
#define S_IFLNK     0120000					/* Symbolic link */
#define S_IFSOCK    0140000					/* Socket */
#define S_IFIFO     0010000					/* FIFO */

/* File mode bits */
#define S_IRUSR     0000400					/* Read permission, owner */
#define S_IWUSR     0000200					/* Write permission, owner */
#define S_IXUSR     0000100					/* Execute/ search permission, owner */
#define S_IRWXU     (S_IRUSR | S_IWUSR | S_IXUSR ) /* RWX, owner */
#define S_IRGRP     0000040					/* Read permission, group */
#define S_IWGRP     0000020					/* Write permission, group */
#define S_IXGRP     0000010					/* Execute/ search permission, group */
#define S_IRWXG     (S_IRGRP | S_IWGRP | S_IXGRP ) /* RWX, group */
#define S_IROTH     0000004					/* Read permission, other */
#define S_IWOTH     0000002					/* Write permission, other */
#define S_IXOTH     0000001					/* Execute/ search permission, other */
#define S_IRWXO     (S_IROTH | S_IWOTH | S_IXOTH ) /* RWX, other */
#define S_ISUID		04000					/* Set user ID on execution */
#define S_ISGID		02000					/* Set group ID on execution */
#define S_ISVTX		01000					/* On directories, restricted deletion flag */

/* Macros to test whether the file is of the specified type */
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)	/* Test macro for a block special file */
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)	/* Test macro for a character special file */
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)	/* Test macro for a directory file */
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)	/* Test macro for a pipe or a FIFO special file */
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)	/* Test macro for a regular file */
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)	/* Test macro for a symbolic link file */
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)/* Test macro for a socket special file */

struct stat
{
    dev_t   st_dev;                         /* Device ID of device
                                               containing file.  */
    ino_t   st_ino;                         /* File serial no.  */
    mode_t  st_mode;                        /* Mode of file.  */
    nlink_t st_nlink;                       /* Number of hard links to the
                                               file.  */
    uid_t   st_uid;                         /* User id of the file.  */
    gid_t   st_gid;                         /* Group id of the file.  */
    dev_t   st_rdev;                        /* DeviceID (if file is
                                               character or block
                                               special.  */
    off_t   st_size;                        /* For regular file, the file
                                               size in bytes.
                                               For symbolic links, the
                                               length in bytes of the
                                               pathname
                                               Contained in the symbolic
                                               link.  */
    time_t  st_atime;                       /* Time of last access.  */
    time_t  st_mtime;                       /* Time of last data
                                               modification.  */
    time_t  st_ctime;                       /* Time of last status
                                               change.  */
    char    padding[2];						/* Padding for the structure
    										 alignment */                                          
};

#ifdef __cplusplus
extern "C" {
#endif

/* Get file status.  */
int     fstat(int , struct stat *);

/* Get symbolic link status.  */
int     lstat(const char *, struct stat *);

/* Make a directory.  */
int     mkdir(const char *, mode_t);

/* Get file status.  */
int     stat(const char *, struct stat *);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_STAT_H */
#endif /* __Istat */
#endif /* __STAT_H_ */
#endif /* _STAT_H_ */
#endif /* _INC_STAT */
#endif /* __STAT_H */
#endif /* _STAT */
#endif /* __stat_h */
#endif /* _STAT_H */

#endif /* NU_PSX_STAT_H */
