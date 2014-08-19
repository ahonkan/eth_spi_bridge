/************************************************************************
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
*       psx_defs.h
*
*   COMPONENT
*
*       POSIX - Definitions
*
*   DESCRIPTION
*
*       The following contains the Nucleus POSIX definitions.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*       pthread.h
*
************************************************************************/

#ifndef PSX_DEFS
#define PSX_DEFS

#include "os/include/nucleus.h"
#include "services/pthread.h"

/* POSIX Core */

/* Standard I/O, which includes file locking data structure */
typedef struct _psx_rtl_flock
{
    struct _iobuf   *iobuf;                 /*  Pointer to I/O buffer */
    unsigned int    count;                  /*  File lock count associated */
    NU_SEMAPHORE    semaphore;              /*  Semaphore ownership */
    NU_TASK         *owner;                 /*  Task ownership */
    OPTION          owner_priority;         /*  Priority ownership */
    CHAR            padding[3];             /*  Padding */

} PSX_RTL_FLOCK;

#ifdef CFG_NU_OS_SVCS_POSIX_FS_ENABLE

/* POSIX File System */

#endif /* CFG_NU_OS_SVCS_POSIX_FS_ENABLE */

#ifdef CFG_NU_OS_SVCS_POSIX_NET_ENABLE

/* POSIX Networking */

/* OPEN_MAX must be lower or the same as NSOCKETS */
#if (OPEN_MAX > NSOCKETS)
#error Total number of socket descriptors OPEN_MAX (posix\inc\limits.h) must be lower or the same as NSOCKETS (net\inc\sockdefs.h)
#endif

#endif /* CFG_NU_OS_SVCS_POSIX_NET_ENABLE */

#ifdef CFG_NU_OS_SVCS_POSIX_RTL_ENABLE

/* POSIX RTL */

#endif /* CFG_NU_OS_SVCS_POSIX_RTL_ENABLE */

#ifdef CFG_NU_OS_SVCS_POSIX_AIO_ENABLE

/* POSIX Asynchronous IO */

#endif /* CFG_NU_OS_SVCS_POSIX_AIO_ENABLE */

#endif /* PSX_DEFS */
