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
*       fcntl.h                
*
*   COMPONENT
*
*       PX - POSIX
*
*   DESCRIPTION
*
*       This file defines the requests and arguments for use by the
*       functions fcntl( ) and open( ).
*
*   DATA STRUCTURES
*
*       flock               Describes a file lock.
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*
*************************************************************************/

#ifndef NU_PSX_FCNTL_H
#define NU_PSX_FCNTL_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/unistd.h"
#include "services/sys/types.h"

/* For Metaware Metrowerks and KMC GNU Tools.  */
#ifndef _FCNTL_H
#define _FCNTL_H

/* For ADS Tools. */
#ifndef __fcntl_h
#define __fcntl_h

/* For Hitachi Tools and TI Tools. */
#ifndef _FCNTL
#define _FCNTL

/* For Paradigm Tools and Microtec Tools. */
#ifndef __FCNTL_H
#define __FCNTL_H

/* For Microsoft Visual C.  */
#ifndef _INC_FCNTL
#define _INC_FCNTL

#ifndef __FCNTL_H_  
#define __FCNTL_H_

/* For Code Sourcery ARM GNU */
#ifndef _FCNTL_
#define _FCNTL_

/* For MinGNU or other GNU toolsets */
#ifndef _FCNTL_H_
#define _FCNTL_H_

/* For DIAB tools */
#ifndef __I_fcntl
#define __I_fcntl

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _SYS_FCNTL_H_
#define _SYS_FCNTL_H_

/* Access modes for open calls.  */
#define O_RDONLY        0x0001
#define O_WRONLY        0x0002
#define O_RDWR          0x0004
#define O_APPEND        0x0008
#define O_CREAT         0x0010
#define O_EXCL          0x0020
#define O_TRUNC         0x0040
#define O_PROC_SHARED   0x0080
#define O_PROC_PRIVATE  0x0100

#if (_POSIX_SYNCHRONIZED_IO != -1)

/* Sync. write file I/O.  */
#define O_SYNC          0x0080

/* Sync. read I/O.  */
#define O_RSYNC         0x0100

/* Sync. write data I/O.  */
#define O_DSYNC         0x0200

#endif  /*  _POSIX_SYNCHRONIZED_IO  */

/* Sync. write data I/O.  */
#define O_NONBLOCK      0x0400
#define O_NOCTTY        0x0800

/* Mask for file access modes.  */
#define O_ACCMODE   (O_RDONLY | O_WRONLY | O_RDWR)


/* Used by C-lib.  */
#define O_FORM      0x40000                 /* Regular form - text.  */
#define O_BINARY    0x80000                 /* Binary form.  */


/* cmd Values for fcntl().  */
#define F_DUPFD         0                   /* Duplicate file descriptor.  */
/* 1 and 2 are reserved for NU_SETFLAG and NU_SET_ZC_MODE */
#define F_GETFD         3                   /* Get fildes flags.  */
#define F_SETFD         4                   /* Set fildes flags.  */
#define F_GETFL         5                   /* Get file status flags.  */
#define F_SETFL         6                   /* Set file status flags.  */
#define F_GETLK         7                   /* Get record-locking
                                               information.  */
#define F_SETLK         8                   /* Set or Clear a record-lock 
                                               (Non-Blocking).  */
#define F_SETLKW        9                   /* Set or Clear a record-lock
                                               (Blocking) */
#define F_GETOWN        10                   /* Get process or process Group
                                               ID to receive SIGURG
                                               signals.  */
#define F_SETOWN        11                   /* Set process or process Group
                                               ID to receive SIGURG
                                               signals.  */
#define F_PUSH          12                   /* Enable/Disable the Naigle Algorithm 
                                                by setting the push bit */
                                                                             
/* File descriptor flags for fcntl().  */

/* Close the file descriptor upon execution of exec-family function.  */
#define FD_CLOEXEC      1

/* l_type Values for Record Locking with fcntl().  */
#define F_RDLCK         1                   /* Read lock.  */
#define F_WRLCK         2                   /* Write lock.  */
#define F_UNLCK         3                   /* Remove lock(s).  */

struct flock
{
    short l_type;                           /* Type of lock; F_RDLCK,
                                               F_WRLCK, F_UNLCK.  */
    short l_whence;                         /* Flag for starting
                                               offset.  */
    off_t l_start;                          /* Relative offset in
                                               bytes.  */
    off_t l_len;                            /* Size; if 0 then until
                                               EOF.  */
    pid_t l_pid;                            /* Process ID of the process
                                               holding the lock; returned
                                               with F_GETLK.  */
};

/* Function Prototypes.  */

#ifdef __cplusplus
extern "C" {
#endif

int fcntl(int fd, int cmd, ...);
int open(const char *path, int oflag, ...);
int creat(const char*, mode_t);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_FCNTL_H_ */
#endif /* __I_fcntl */
#endif /* _FCNTL_H_ */
#endif /* _FCNTL_ */
#endif /* __FCNTL_H_ */
#endif /* _INC_FCNTL */
#endif /* __FCNTL_H */
#endif /* _FCNTL */
#endif /* __fcntl_h */
#endif /* _FCNTL_H */

#endif /* NU_PSX_FCNTL_H */
