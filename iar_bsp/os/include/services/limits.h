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
*       limits.h
*
*   COMPONENT
*
*       PX - POSIX
*
*   DESCRIPTION
*
*       This file defines various symbolic names. The names represent
*       various limits on resources that the implementation imposes on
*       applications.
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
*       unistd.h            POSIX unistd.h definitions
*
*************************************************************************/

#ifndef NU_PSX_LIMITS_H
#define NU_PSX_LIMITS_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/unistd.h"

/* For Metaware Metrowerks and KMC GNU Tools.  */
#ifndef _LIMITS_H
#define _LIMITS_H

/* For ADS Tools.  */
#ifndef __limits_h
#define __limits_h

/* For Hitachi Tools and TI Tools.  */
#ifndef _LIMITS
#define _LIMITS

/* For Paradigm Tools and Microtec Tools.  */
#ifndef __LIMITS_H
#define __LIMITS_H

/* For Microsoft Visual C.  */
#ifndef _INC_LIMITS
#define _INC_LIMITS

#ifndef __LIMITS_H_
#define __LIMITS_H_

/* For MinGNU or other GNU toolsets  */
#ifndef _LIMITS_H_
#define _LIMITS_H_

/* For DIAB tools */
#ifndef __Ilimits
#define __Ilimits

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _LIBC_LIMITS_H_
#define _LIBC_LIMITS_H_

/* Maximum length of argument to the exec functions including environment
   data.  */
#define _POSIX_ARG_MAX                  4096

#if (_POSIX_ASYNCHRONOUS_IO != -1)

/* The number of I/O operations that can be specified in a list I/O
   call.  */
#define _POSIX_AIO_LISTIO_MAX           10

/* The number of outstanding asynchronous I/O operations.  */
#define _POSIX_AIO_MAX                  10

#endif  /* _POSIX_ASYNCHRONOUS_IO */

/* Maximum number of simultaneous processes per real user ID.  */
#define _POSIX_CHILD_MAX                6

/* Maximum length of a host name (not including the terminating null) as
   returned from the gethostname()function.  */
#define _POSIX_HOST_NAME_MAX            255

/* Maximum length of a login name.  */
#define _POSIX_LOGIN_NAME_MAX           9

/* Maximum number of links to a single file.  */
#define _POSIX_LINK_MAX                 8

/* Maximum number of bytes allowed in a terminal input queue.  */
#define _POSIX_MAX_INPUT                255

#if (_POSIX_MESSAGE_PASSING != -1)

/* The maximum number of open message queue descriptors a process may
   hold. */
#define _POSIX_MQ_OPEN_MAX              8

/* The maximum number of message priorities supported by the
   implementation. */
#define _POSIX_MQ_PRIO_MAX              255

#endif  /* _POSIX_MESSAGE_PASSING */

/* Maximum number of bytes in a filename (not including terminating
   null). This value corresponds to Nucleus FILE EMAXPATH. It is
   recommended that the user sets this value the same as
   the value of EMAXPATH. */
#define _POSIX_NAME_MAX                 255

/* If Nucleus Storage (File) component is not enabled */
#if (!defined(CFG_NU_OS_STOR_FILE_VFS_ENABLE))
#define CFG_NU_OS_STOR_FILE_VFS_NUM_USERS 10
#endif

/* If Nucleus POSIX File System component is not enabled */
#if (!defined(CFG_NU_OS_SVCS_POSIX_FS_ENABLE))
#define CFG_NU_OS_SVCS_POSIX_FS_OPEN_MAX  30
#endif

/* Maximum number of files that one process can have open at any one
time.  */
#define _POSIX_OPEN_MAX                 CFG_NU_OS_SVCS_POSIX_FS_OPEN_MAX

/* Maximum number of bytes in a pathname. */
#define _POSIX_PATH_MAX                 256

#if (_POSIX_THREADS != -1)

/* Maximum number of attempts made to destroy a thread’s thread-specific
   data values on thread exit. */
#define _POSIX_PTHREAD_DESTRUCTOR_ITERATIONS    4

/* Maximum number of data keys that can be created by a process. */
#define _POSIX_PTHREAD_KEYS_MAX         128

/* Maximum number of threads that can be created per process. */
#define _POSIX_PTHREAD_THREADS_MAX      CFG_NU_OS_SVCS_POSIX_CORE_PTHREAD_THREADS_MAX

#endif  /*  _POSIX_THREADS */

#if (_POSIX_REALTIME_SIGNALS != -1)

/* Maximum number of real time signals reserved for application use in this
   implementation. */
#define _POSIX_RTSIG_MAX                12

/* Maximum number of queued signals that a process may send and have
   pending at the receiver(s) at any time. */
#define _POSIX_SIGQUEUE_MAX             32

#endif  /* _POSIX_REALTIME_SIGNALS */

#if (_POSIX_SEMAPHORES != -1)

/* Maximum number of semaphores that a process may have. */
#define _POSIX_SEM_NSEMS_MAX            256

/* The maximum value a semaphore may have. */
#define _POSIX_SEM_VALUE_MAX            32767

#endif  /* _POSIX_SEMAPHORES */

/* The number of streams that one process can have open at one time. */
#define _POSIX_STREAM_MAX               8

#if (_POSIX_TIMERS != -1)

/* Maximum number of timers per process supported by the
   implementation. */
#define _POSIX_TIMER_MAX                32

/* Maximum number of timer expiration overruns. */
#define _POSIX_DELAYTIMER_MAX

/* Minimum Clock Resolution in nanoseconds. */
#define _POSIX_CLOCKRES_MIN             20000000

#endif  /*  _POSIX_TIMERS */

/* Maximum length of terminal device name. */
#define _POSIX_TTY_NAME_MAX             9

/* Maximum number of bytes supported for the name of a time zone (not of the
   TZ variable). */
#define _POSIX_TZNAME_MAX               6

/* The value that can be stored in an object of type ssize_t. */
#define _POSIX_SSIZE_MAX                32767

/* The number of bytes in a symbolic link. */
#define _POSIX_SYMLINK_MAX              255

#ifndef CHAR_BIT
#define CHAR_BIT    8
#endif /* CHAR_BIT */

#if (PSX_COMPILER_SIGNED_CHAR == 1)

#ifndef CHAR_MAX    
#define CHAR_MAX    SCHAR_MAX
#endif /* CHAR_MAX */   

#ifndef CHAR_MIN
#define CHAR_MIN    SCHAR_MIN
#endif /* CHAR_MIN */ 

#else

#ifndef CHAR_MAX
#define CHAR_MAX    UCHAR_MAX
#endif /* CHAR_MAX */   

#ifndef CHAR_MIN
#define CHAR_MIN    UCHAR_MIN
#endif /* CHAR_MIN */

#endif /* PSX_COMPILER_SIGNED_CHAR */

#ifndef INT_MAX
#define INT_MAX     (2147483647L)
#endif /* INT_MAX */

#ifndef LONG_BIT
#define LONG_BIT    32
#endif /* LONG_BIT */

#ifndef LONG_MAX
#define LONG_MAX    (2147483647L)
#endif /* LONG_MAX */

#ifndef MB_LEN_MAX
#define MB_LEN_MAX  1
#endif /* MB_LEN_MAX */

#ifndef SCHAR_MAX
#define SCHAR_MAX   ((signed char)127)
#endif /* SCHAR_MAX */

#ifndef SHRT_MAX
#define SHRT_MAX    32767
#endif  /* USHRT_MAX */

#ifndef UCHAR_MAX
#define UCHAR_MAX   ((unsigned char)255)
#endif /* UCHAR_MAX */

#ifndef UINT_MAX
#define UINT_MAX    (4294967295UL)
#endif  /* UINT_MAX */

#ifndef ULONG_MAX
#define ULONG_MAX   (4294967295UL)
#endif  /* ULONG_MAX */

#ifndef USHRT_MAX
#define USHRT_MAX   65535
#endif  /* USHRT_MAX */

#ifndef WORD_BIT
#define WORD_BIT    32
#endif /* WORD_BIT */

#ifndef INT_MIN
#define INT_MIN     (INT_MAX * -1)
#endif /* INT_MIN */

#ifndef LONG_MIX

#ifndef LONG_MIN
#define LONG_MIN    (2147483647L * -1)
#endif /* LONG_MIN */

#endif  /* LONG_MIX */

#ifndef SCHAR_MIN
#define SCHAR_MIN   ((signed char)(128 * -1))
#endif /* SCHAR_MIN */

#ifndef SHRT_MIN
#define SHRT_MIN    -32767
#endif /* SHRT_MIN */

#ifndef LLONG_MIN
#define LLONG_MIN   (9223372036854775807LL * -1)
#endif /* LLONG_MIN */

#ifndef LLONG_MAX
#define LLONG_MAX   (9223372036854775807LL)
#endif /* LLONG_MAX */

#ifndef ULLONG_MAX
#define ULLONG_MAX  (18446744073709551615ULL)
#endif /* ULLONG_MAX */

#if (_POSIX_ASYNCHRONOUS_IO != -1)

#define AIO_MAX                         _POSIX_AIO_MAX

#define AIO_LISTIO_MAX                  _POSIX_AIO_LISTIO_MAX

/* The maximum amount by which a process can decrease its asynchronous I/O
   priority level from its own scheduling priority. */
#define AIO_PRIO_DELTA_MAX              0

#endif  /* _POSIX_ASYNCHRONOUS_IO */

#define ARG_MAX                         _POSIX_ARG_MAX

#define CHILD_MAX                       _POSIX_CHILD_MAX

#if (_POSIX_TIMERS != -1)

#define DELAYTIMER_MAX                  _POSIX_DELAYTIMER_MAX

#endif /* _POSIX_TIMERS */

#define HOST_NAME_MAX                   _POSIX_HOST_NAME_MAX

#define LOGIN_NAME_MAX                  _POSIX_LOGIN_NAME_MAX

#define NAME_MAX                        _POSIX_NAME_MAX

#define PATH_MAX                        _POSIX_PATH_MAX

/* PATH_MAX+4 max = drive letters "A:" + '/' + '/' at the end. */
#define POSIX_FAT_PATH_MAX              PATH_MAX+4

#define LINK_MAX                        _POSIX_LINK_MAX

#define SSIZE_MAX                       _POSIX_SSIZE_MAX

#if (_POSIX_MESSAGE_PASSING != -1)

#define MQ_OPEN_MAX                     _POSIX_MQ_OPEN_MAX

#define MQ_PRIO_MAX                     _POSIX_MQ_PRIO_MAX

#endif  /* _POSIX_MESSAGE_PASSING */

#define OPEN_MAX                        _POSIX_OPEN_MAX

#define PAGESIZE                        4096

#define MAX_INPUT                       _POSIX_MAX_INPUT

#if (_POSIX_THREADS != -1)

#define PTHREAD_DESTRUCTOR_ITERATIONS  _POSIX_PTHREAD_DESTRUCTOR_ITERATIONS

#define PTHREAD_KEYS_MAX                _POSIX_PTHREAD_KEYS_MAX

/* Minimum size in bytes of thread stack storage. */
/* Minimum stack size will need to be bigger if file system 
   or Networking operations are being used. */
#define PTHREAD_STACK_MIN               CFG_NU_OS_KERN_PLUS_CORE_MIN_STACK_SIZE
/* Default Stack size if not specified */
#define PTHREAD_DEFAULT_STACKSIZE       4096

#define PTHREAD_THREADS_MAX             _POSIX_PTHREAD_THREADS_MAX

#endif /* _POSIX_THREADS */

#if (_POSIX_REALTIME_SIGNALS != -1)

#define RTSIG_MAX                       _POSIX_RTSIG_MAX

#define SIGQUEUE_MAX                    _POSIX_SIGQUEUE_MAX

#endif  /* _POSIX_REALTIME_SIGNALS */

#if (_POSIX_SEMAPHORES != -1)

#define SEM_NSEMS_MAX                   _POSIX_SEM_NSEMS_MAX

#define SEM_VALUE_MAX                   _POSIX_SEM_VALUE_MAX

#endif  /* _POSIX_SEMAPHORES */

#define STREAM_MAX                      _POSIX_STREAM_MAX

#if (_POSIX_TIMERS != -1)

#define TIMER_MAX                       _POSIX_TIMER_MAX

#endif  /* _POSIX_TIMERS */

#define TTY_NAME_MAX                    _POSIX_TTY_NAME_MAX

#define TZNAME_MAX                      _POSIX_TZNAME_MAX

#endif /* _LIBC_LIMITS_H_ */
#endif /* __Ilimits */
#endif /* _LIMITS_H_ */
#endif /* __LIMITS_H_ */
#endif /* _INC_LIMITS */
#endif /* __LIMITS_H */
#endif /* _LIMITS */
#endif /* __limits_h */
#endif /* _LIMITS_H */

#endif /* NU_PSX_LIMITS_H */
