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
*       mman.h
*
* COMPONENT
*
*       MM - Memory Management
*
* DESCRIPTION
*
*       Contains the memory management related declarations.
*
* DATA STRUCTURES
*
*       size_t                              Used for sizes of objects.
*
* DEPENDENCIES
*
*       "stddef.h"                          STDDEF related definitions.
*
************************************************************************/
#ifndef __MMAN_H_
#define __MMAN_H_

#include "services/stddef.h"

#if (_POSIX_MEMLOCK != -1)

/* Constants defined for the mlockall() function. */
#define MCL_CURRENT  0x01                   /* Lock currently mapped pages */
#define MCL_FUTURE   0x02                   /* Lock pages that become mapped */

#endif  /* _POSIX_MEMLOCK */

/* Constants defined for memory mapping. */
#define MAP_FAILED    ((void *) -1)
#define PROT_NONE     0x0                    /* pages cannot be accessed */
#define PROT_READ     0x1                    /* pages can be read */
#define PROT_WRITE    0x2                    /* pages can be written */
#define PROT_EXEC     0x4                    /* pages can be executed */
#define PROT_RW      (PROT_READ | PROT_WRITE )
#define MAP_SHARED    0x1                    /* share changes */
#define MAP_PRIVATE   0x2                    /* changes are private */
#define MAP_FIXED     0x10                   /* user assigns address */
#define MS_ASYNC      0x1                    /* return immediately */
#define MS_INVALIDATE 0x2                    /* invalidate caches */
#define MS_SYNC       0x4                    /* sync with backing store */

#ifdef __cplusplus
extern "C" {
#endif

#if (_POSIX_MEMLOCK != -1)

/* Lock the address space of a process. */
int mlockall(int flags);

/* Unlock the address space of a process. */
int munlockall(void);

#endif  /* _POSIX_MEMLOCK */

#if (_POSIX_MEMLOCK_RANGE != -1)

/* Lock a range of process address space. */
int mlock(const void *addr,size_t len);

/* Unlock a range of process address space. */
int munlock(const void *addr,size_t len);

#endif  /* _POSIX_MEMLOCK_RANGE */

void *mmap(void *addr, size_t len, int prot, int flag, int filedes, off_t off);
int munmap(void *addr, size_t len);
int msync(void *addr, size_t len, int flags);
int shm_open(const char *name, int oflag, mode_t mode);
int shm_unlink(const char *name);

#ifdef __cplusplus
}
#endif


#endif  /*  __MMAN_H_  */

