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
*       errno.h
*
*   COMPONENT
*
*       PX - POSIX
*
*   DESCRIPTION
*
*       Some functions use this global variable to track errors.  The
*       computations are reentrant, but the error reporting scheme is
*       not.  Use at own risk!!
*
*       NOTE: Standard C RTL header.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*
*************************************************************************/

#ifndef NU_PSX_ERRNO_H
#define NU_PSX_ERRNO_H

#include "services/config.h"
#include "services/compiler.h"

/* For Metaware Metrowerks and KMC GNU Tools.  */
#ifndef _ERRNO_H
#define _ERRNO_H

/* For ADS Tools.  */
#ifndef __errno_h
#define __errno_h

/* For Hitachi Tools and TI Tools.  */
#ifndef _ERRNO
#define _ERRNO

/* For Paradigm Tools and Microtec Tools.  */
#ifndef __ERRNO_H
#define __ERRNO_H

/* For Microsoft Visual C.  */
#ifndef _INC_ERRNO
#define _INC_ERRNO

#ifndef __ERRNO_H_
#define __ERRNO_H_

/* For Code Sourcery ARM GNU */
#ifndef __ERRNO_H__
#define __ERRNO_H__

/* For MinGNU or other GNU toolsets  */
#ifndef _ERRNO_H_
#define _ERRNO_H_

/* For DIAB tools */
#ifndef __Ierrno
#define __Ierrno

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _SYS_ERRNO_H_
#define _SYS_ERRNO_H_

/* The <errno.h> header shall provide a declaration for errno and give
   positive values for the following symbolic constants. Their values shall
   be unique except as noted below.  */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(PSX_H8) || defined(PSX_HIT)
#undef errno
#endif /* defined(PSX_H8) || defined(PSX_HIT) */

/* Define errno.  */
extern int * _errno(void);
#define errno   (*_errno())

#ifdef __cplusplus
}
#endif

/* Error Numbers.  */
#define ENOERR              0
#define EPERM               1               /* Operation not permitted.  */
#define ENOENT              2               /* No such file or
                                               directory.  */
#define ESRCH               3               /* No such process.  */
#define EINTR               4               /* Interrupted function.  */
#define EIO                 5               /* I/O error.  */
#define ENXIO               6               /* No such device or
                                               address.  */
#define E2BIG               7               /* Argument list too long.  */
#define ENOEXEC             8               /* Executable file format
                                               error.  */
#define EBADF               9               /* Bad file descriptor.  */
#define ECHILD              10              /* No child processes.  */
#define EAGAIN              11              /* Resource unavailable, try
                                               again (may be the same value
                                               as[EWOULDBLOCK]).  */
#define ENOMEM              12              /* Not enough space.  */
#define EACCES              13              /* Permission denied.  */
#define EFAULT              14              /* Bad address.  */
#define EBUSY               16              /* Device or resource busy.  */
#define EEXIST              17              /* File exists.  */
#define EXDEV               18              /* Cross-device link.  */
#define ENODEV              19              /* No such device.  */
#define ENOTDIR             20              /* Not a directory.  */
#define EISDIR              21              /* Is a directory.  */
#define EINVAL              22              /* Invalid argument.  */
#define ENFILE              23              /* Too many files open in
                                               system.  */
#define EMFILE              24              /* Too many open files.  */
#define ENOTTY              25              /* Inappropriate I/O control
                                               operation.  */
#define EFBIG               27              /* File too large.  */
#define ENOSPC              28              /* No space left on device.  */
#define ESPIPE              29              /* Illegal seek.  */
#define EROFS               30              /* Read-only file system.  */
#define EMLINK              31              /* Too many links.  */
#define EPIPE               32              /* Broken pipe.  */
#define EDOM                33              /* Mathematics argument out of
                                               domain of function.  */
#define ERANGE              34              /* Result too large.  */
#define EDEADLK             36              /* Resource deadlock would
                                               occur.  */
#define ENAMETOOLONG        38              /* Filename too long.  */
#define ENOLCK              39              /* No locks available.  */
#define ENOSYS              40              /* Function not supported.  */
#define ENOTEMPTY           41              /* Directory not empty.  */
#define EILSEQ              42              /* illegal byte sequence   */
#define EPROTONOSUPPORT     43              /* Protocol not supported   */
#define EOPNOTSUPP          45              /* Operation not supported on
                                               the socket.  */
#define EAFNOSUPPORT        47              /* Address family not
                                               supported.  */
#define EADDRINUSE          48              /* Address in use.  */
#define EADDRNOTAVAIL       49              /* Address not available.  */
#define ENETDOWN            50              /* Network is down.  */
#define ENETUNREACH         51              /* Network unreachable.  */
#define ENETRESET           52              /* Connection aborted by
                                               network.  */
#define ECONNABORTED        53              /* Connection aborted.  */
#define ECONNRESET          54              /* Connection reset.  */
#define ENOBUFS             55              /* No buffer space
                                               available.  */
#define EISCONN             56              /* Socket is connected.  */
#define ENOTCONN            57              /* Socket is not connected.  */
#define ETIMEDOUT           60              /* Connection time out.  */
#define ECONNREFUSED        61              /* connection refused.  */
#define ELOOP               62              /* Too many levels of symbolic
                                               links.  */
#define EHOSTUNREACH        65              /* Host is unreachable.  */
#define EDQUOT              69              /* Reserved.  */
#define ESTALE              70              /* Reserved.  */
#define EMULTIHOP           71              /* Reserved.  */
#define ENOLINK             72              /* Reserved.  */
#define ENOPROTOOPT         73              /* Protocol not available.  */
#define ENOTSOCK            74              /* Not a socket.  */
#define EPROTOTYPE          75              /* Protocol wrong type for
                                               socket.  */
#define EPROTO              76              /* Protocol error.  */
#define EALREADY            77              /* Resource unavailable.  */
#define ECANCELED           78              /* Operation canceled.  */
#define EDESTADDRREQ        79              /* Destination address
                                               required.  */
#define EIDRM               80              /* Identifier removed.  */
#define EINPROGRESS         81              /* Operation in progress.  */
#define ENOTSUP             82              /* Not supported.  */
#define EOVERFLOW           83              /* Value too large to be stored
                                               in data type.  */
#define ETXTBSY             84              /* Text file busy.  */
#define EWOULDBLOCK         85              /* Operation would block.  */
#define EMSGSIZE            86              /* Message too large.  */
#define EBADMSG             87              /* Bad message.  */
#define ENOMSG              88              /* No message of the desired
                                               type.  */

#endif /* _SYS_ERRNO_H_ */
#endif /* __Ierrno */
#endif /* _ERRNO_H_ */
#endif /* __ERRNO_H__ */
#endif /* __ERRNO_H_ */
#endif /* _INC_ERRNO */
#endif /* __ERRNO_H */
#endif /* _ERRNO */
#endif /* __errno_h */
#endif /* _ERRNO_H */

#endif /* NU_PSX_ERRNO_H */
