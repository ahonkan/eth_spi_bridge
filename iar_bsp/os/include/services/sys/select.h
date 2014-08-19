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
*       select.h
*
*   COMPONENT
*
*		Nucleus POSIX - Networking
*
*   DESCRIPTION
*
*		This file defines the structure for select().
*
*   DATA STRUCTURES
*
*		None
*
*   DEPENDENCIES
*
*		None
*
*************************************************************************/

#ifndef NU_PSX_SELECT_H
#define NU_PSX_SELECT_H

#include "services/config.h"
#include "services/compiler.h"
#include "kernel/plus_core.h"

/* For ADS Tools */
#ifndef _select_h
#define _select_h

/* For Paradigm Tools and Microtec Tools. */
#ifndef _SELECT_H
#define _SELECT_H

/* For Code Sourcery ARM GNU */
#ifndef _SELECT_H_
#define _SELECT_H_

#include "services/sys/types.h"
#include "services/signal.h"
#include "services/sys/time.h"
#include "services/limits.h"
#include "services/config.h"

#if ((defined(POSIX_INCLUDE_NET)) && (POSIX_INCLUDE_NET > 0))

#include "networking/net_cfg.h"
#include "networking/sockdefs.h"
#define PSX_FD_BITS       FD_BITS

#else

#define PSX_FD_BITS       32

#endif /* POSIX_INCLUDE_NET */

/* Maximum number of file descriptors in an fd_set structure.  */
#define FD_SETSIZE        OPEN_MAX
#define PSX_SEL_SIZE      ((OPEN_MAX/PSX_FD_BITS)+1)

/* Macro MICROSECONDS to TICKS */
#define MICROSECONDS_PER_SECOND                1000000
#define POSIX_TICKS_PER_SECOND                 NU_PLUS_Ticks_Per_Second
#define POSIX_MICROSECONDS_PER_TICK            (MICROSECONDS_PER_SECOND / \
                                                    POSIX_TICKS_PER_SECOND)
/* Macro for setting up the bit */
#define PSX_BIT_SET(x) (1UL << (x%PSX_FD_BITS))

/* Pending Error */
#define O_PSX_PEND_ERR      8

#ifdef __cplusplus
extern "C" {
#endif

/* fd_set structure */
typedef struct xfd_set
{
    unsigned long sets[PSX_SEL_SIZE];
} fd_set;

/* Clears the bit for the file descriptor fd in the
   file descriptor set fdset. */
void FD_CLR(int fd, fd_set *fdset);

/* Returns a non-zero value if the bit for the file descriptor fd is set
  in the file descriptor set by fdset, and 0 otherwise.*/
int FD_ISSET(int fd, fd_set *fdset);

/* Sets the bit for the file descriptor fd in the file
   descriptor set fdset. */
void FD_PSET(int fd, fd_set *fdset);

/* Initializes the file descriptor set fdset to have zero bits
   for all file descriptors.  */
void FD_ZERO(fd_set *fdset);

/* Function to perform synchronous I/O multiplexing */
int  select(int, fd_set *, fd_set *, fd_set *,struct timeval *);

#ifdef __cplusplus
}
#endif

#endif /* _SELECT_H_ */
#endif /* _SELECT_H */
#endif /* _select_h */

#endif /* NU_PSX_SELECT_H */
