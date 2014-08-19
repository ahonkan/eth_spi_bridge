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
*       unistd.h
*
*   COMPONENT
*
*       PX - POSIX
*
*   DESCRIPTION
*
*       Contains the definition of standard symbolic constants and types.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       config.h
*       compiler.h
*       stddef.h
*
*************************************************************************/

#ifndef NU_PSX_UNISTD_H
#define NU_PSX_UNISTD_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/stddef.h"
#include "services/sys/time.h"
#include "services/sys/types.h"

/* For Metaware Metrowerks and KMC GNU Tools.  */
#ifndef _UNISTD_H
#define _UNISTD_H

/* For ADS Tools.  */
#ifndef __unistd_h
#define __unistd_h

/* For Hitachi Tools and TI Tools.  */
#ifndef _UNISTD
#define _UNISTD

/* For Paradigm Tools and Microtec Tools.  */
#ifndef __UNISTD_H
#define __UNISTD_H

/* For Microsoft Visual C.  */
#ifndef _INC_UNISTD
#define _INC_UNISTD

#ifndef __UNISTD_H_
#define __UNISTD_H_

/* For Code Sourcery ARM GNU */
#ifndef _UNISTD_H_
#define _UNISTD_H_

/* For DIAB tools */
#ifndef __Iunistd
#define __Iunistd

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _SYS_UNISTD_H
#define _SYS_UNISTD_H

#define F_OK            0                   /* Does file exist.  */
#define X_OK            1                   /* Is it executable by
                                               caller.  */
#define W_OK            2                   /* Is it writable by
                                               caller.  */
#define R_OK            4                   /* Is it readable by
                                               caller.  */

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

/* Null pointer constant. */
#ifndef NULL
#define NULL            0
#endif /* NULL */

/* lseek & access args.  */
#ifndef SEEK_SET
#define SEEK_SET    0                       /* Set file pointer to
                                               "offset".  */
#endif

#ifndef SEEK_CUR
#define SEEK_CUR    1                       /* Set file pointer to current
                                               plus "offset".  */
#endif

#ifndef SEEK_END
#define SEEK_END    2                       /* Set file pointer to EOF plus
                                               "offset".  */
#endif


#define _POSIX_VERSION      200112L         /* Implementations conforming
                                               to IEEE Std 1003.1-2001.  */

#define _POSIX2_VERSION     200112L         /* Integer value indicating
                                               version of the Shell and
                                               Utilities volume of IEEE Std
                                               1003.1 to which the
                                               implementation conforms.  */

/* POSIX defined compile-time constants.  */
#define _POSIX_ASYNCHRONOUS_IO               1
#define _POSIX_BARRIERS                     -1
#define _POSIX_CHOWN_RESTRICTED             -1
#define _POSIX_CLOCK_SELECTION               1
#define _POSIX_CPUTIME                      -1
#define _POSIX_FSYNC                         1
#define _POSIX_JOB_CONTROL                  -1
#define _POSIX_MAPPED_FILES                  1
#define _POSIX_MEMLOCK                       1
#define _POSIX_MEMLOCK_RANGE                 1
#define _POSIX_MEMORY_PROTECTION            -1
#define _POSIX_MESSAGE_PASSING               1
#define _POSIX_MONOTONIC_CLOCK              -1
#define _POSIX_NO_TRUNC                      1
#define _POSIX_PRIORITIZED_IO                1
#define _POSIX_PRIORITY_SCHEDULING           1
#define _POSIX_RAW_SOCKETS                   1
#define _POSIX_READ_WRITE_LOCKS             -1
#define _POSIX_REALTIME_SIGNALS              1
#define _POSIX_REGEXP                       -1
#define _POSIX_SAVED_IDS                    -1
#define _POSIX_SEMAPHORES                    1
#define _POSIX_SHARED_MEMORY_OBJECTS         1
#define _POSIX_SHELL                        -1
#define _POSIX_SPAWN                        -1
#define _POSIX_SYNCHRONIZED_IO               1
#define _POSIX_THREAD_ATTR_STACKADDR         1
#define _POSIX_THREAD_ATTR_STACKSIZE         1
#define _POSIX_THREAD_CPUTIME                1
#ifdef MOT
#define _POSIX_THREAD_PRIO_INHERIT           -1
#define _POSIX_THREAD_PRIO_PROTECT           -1
#else
#define _POSIX_THREAD_PRIO_INHERIT           1
#define _POSIX_THREAD_PRIO_PROTECT           1
#endif /* MOT */
#define _POSIX_THREAD_PRIORITY_SCHEDULING    1
#define _POSIX_THREAD_PROCESS_SHARED        -1
#define _POSIX_THREAD_SAFE_FUNCTIONS         1
#define _POSIX_THREAD_SPORADIC_SERVER       -1
#define _POSIX_THREADS                       1
#define _POSIX_TIMEOUTS                      1
#define _POSIX_TIMERS                        1
#define _POSIX_TRACE                        -1
#define _POSIX_TRACE_EVENT_FILTER           -1
#define _POSIX_TRACE_INHERIT                -1
#define _POSIX_TRACE_LOG                    -1
#define _POSIX_TYPED_MEMORY_OBJECTS         -1
#define _POSIX_VDISABLE                     -1
#define _POSIX_FD_MGMT                       1
#define _POSIX2_C_BIND                      -1
#define _POSIX2_C_DEV                       -1
#define _POSIX2_CHAR_TERM                   -1
#define _POSIX2_FORT_DEV                    -1
#define _POSIX2_FORT_RUN                    -1
#define _POSIX2_LOCALEDEF                   -1
#define _POSIX2_PBS                         -1
#define _POSIX2_PBS_ACCOUNTING              -1
#define _POSIX2_PBS_CHECKPOINT              -1
#define _POSIX2_PBS_LOCATE                  -1
#define _POSIX2_PBS_MESSAGE                 -1
#define _POSIX2_PBS_TRACK                   -1
#define _POSIX2_SW_DEV                      -1
#define _POSIX2_UPE                         -1
#define _V6_ILP32_OFF32                     -1
#define _V6_ILP32_OFFBIG                    -1
#define _V6_ILP64_OFF64                     -1
#define _V6_ILPBIG_OFFBIG                   -1
#define _POSIX_IPV6                          1

/* POSIX defined run-time constants.  */

/* Asynchronous input or output operations may be performed for the
   associated file.  */
#define _POSIX_ASYNC_IO                     1
/* Synchronized input or output operations may be performed for the
   associated file.  */
#define _POSIX_SYNC_IO                      1
/* Prioritized input or output operations may be performed for the
   associated file.  */
#define _POSIX_PRIO_IO                      1

/* Configurable System Variables used for sysconf.  */
#define _SC_2_C_BIND                    0
#define _SC_2_C_DEV                     1
#define _SC_2_C_VERSION                 2
#define _SC_2_CHAR_TERM                 3
#define _SC_2_FORT_DEV                  4
#define _SC_2_FORT_RUN                  5
#define _SC_2_LOCALEDEF                 6
#define _SC_2_PBS                       7
#define _SC_2_PBS_ACCOUNTING            8
#define _SC_2_PBS_CHECKPOINT            9
#define _SC_2_PBS_LOCATE                10
#define _SC_2_PBS_MESSAGE               11
#define _SC_2_PBS_TRACK                 12
#define _SC_2_SW_DEV                    13
#define _SC_2_UPE                       14
#define _SC_2_VERSION                   15
#define _SC_ADVISORY_INFO               16
#define _SC_AIO_LISTIO_MAX              17
#define _SC_AIO_MAX                     18
#define _SC_AIO_PRIO_DELTA_MAX          19
#define _SC_ARG_MAX                     20
#define _SC_BC_BASE_MAX                 21
#define _SC_BC_DIM_MAX                  22
#define _SC_BC_SCALE_MAX                23
#define _SC_BC_STRING_MAX               24
#define _SC_CHILD_MAX                   25
#define _SC_CLK_TCK                     26
#define _SC_COLL_WEIGHTS_MAX            27
#define _SC_CPUTIME                     28
#define _SC_DELAYTIMER_MAX              29
#define _SC_EXPR_NEST_MAX               30
#define _SC_FILE_LOCKING                31
#define _SC_LINE_MAX                    32
#define _SC_MQ_OPEN_MAX                 33
#define _SC_MQ_PRIO_MAX                 34
#define _SC_NGROUPS_MAX                 35
#define _SC_OPEN_MAX                    36
#define _SC_PAGESIZE                    37
#define _SC_PRIORITY_SCHEDULING         38
#define _SC_RTSIG_MAX                   39
#define _SC_RE_DUP_MAX                  40
#define _SC_SEM_NSEMS_MAX               41
#define _SC_SEM_VALUE_MAX               42
#define _SC_SIGQUEUE_MAX                43
#define _SC_STREAM_MAX                  44
#define _SC_SPAWN                       45
#define _SC_TIMER_MAX                   46
#define _SC_TZNAME_MAX                  47
#define _SC_ASYNCHRONOUS_IO             48
#define _SC_FSYNC                       49
#define _SC_JOB_CONTROL                 50
#define _SC_MAPPED_FILES                51
#define _SC_MEMLOCK                     52
#define _SC_MEMLOCK_RANGE               53
#define _SC_MEMORY_PROTECTION           54
#define _SC_MESSAGE_PASSING             55
#define _SC_PRIORITIZED_IO              56
#define _SC_REALTIME_SIGNALS            57
#define _SC_REGEXP                      58
#define _SC_SAVED_IDS                   59
#define _SC_SEMAPHORES                  60
#define _SC_SHARED_MEMORY_OBJECTS       61
#define _SC_SYNCHRONIZED_IO             62
#define _SC_SPORADIC_SERVER             63
#define _SC_SHELL                       64
#define _SC_TIMERS                      65
#define _SC_VERSION                     66
#define _SC_GETGR_R_SIZE_MAX            67
#define _SC_GETPW_R_SIZE_MAX            68
#define _SC_HOST_NAME_MAX               69
#define _SC_LOGIN_NAME_MAX               70
#define _SC_THREAD_DESTRUCTOR_ITERATIONS 71
#define _SC_THREAD_KEYS_MAX              72
#define _SC_THREAD_STACK_MIN             73
#define _SC_THREAD_THREADS_MAX           74
#define _SC_TTY_NAME_MAX                 75
#define _SC_THREADS                      76
#define _SC_THREAD_ATTR_STACKADDR        77
#define _SC_THREAD_ATTR_STACKSIZE        78
#define _SC_THREAD_CPUTIME               79
#define _SC_THREAD_PRIORITY_SCHEDULING   80
#define _SC_THREAD_PRIO_PROTECT          81
#define _SC_THREAD_PRIO_INHERIT          82
#define _SC_THREAD_PROCESS_SHARED        83
#define _SC_THREAD_SAFE_FUNCTIONS        84
#define _SC_THREAD_SPORADIC_SERVER       85
#define _SC_TIMEOUTS                     86
#define _SC_READER_WRITER_LOCKS          87
#define _SC_V6_ILP32_OFF32               88
#define _SC_V6_ILP32_OFFBIG              89
#define _SC_V6_LP64_OFF64                90
#define _SC_V6_LPBIG_OFFBIG              91

/* Constants used by the pathconf.  */
#define _PC_ASYNC_IO                    0
#define _PC_CHOWN_RESTRICTED            1
#define _PC_FILESIZEBITS                2
#define _PC_LINK_MAX                    3
#define _PC_MAX_CANON                   4
#define _PC_MAX_INPUT                   5
#define _PC_NAME_MAX                    6
#define _PC_NO_TRUNC                    7
#define _PC_PATH_MAX                    8
#define _PC_PIPE_BUF                    9
#define _PC_PRIO_IO                     10
#define _PC_SYNC_IO                     11
#define _PC_VDISABLE                    12

/* Constants used by the confstr().  */
#define _CS_PATH                            0
#define _CS_POSIX_V6_ILP32_OFF32_CFLAGS     1
#define _CS_POSIX_V6_ILP32_OFF32_LDFLAGS    2
#define _CS_POSIX_V6_ILP32_OFF32_LIBS       3
#define _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS    4
#define _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS   5
#define _CS_POSIX_V6_ILP32_OFFBIG_LIBS      6
#define _CS_POSIX_V6_LP64_OFF64_CFLAGS      7
#define _CS_POSIX_V6_LP64_OFF64_LDFLAGS     8
#define _CS_POSIX_V6_LP64_OFF64_LIBS        9
#define _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS    10
#define _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS   11
#define _CS_POSIX_V6_LPBIG_OFFBIG_LIBS      12
#define _CS_POSIX_V6_WIDTH_RESTRICTED_ENVS  13

#ifdef __cplusplus
extern "C" {
#endif /*  __cplusplus  */

/* Function declarations.  */
int         access(const char *, int);
unsigned    alarm(unsigned seconds);
int         chdir(const char *);
int         close(int);
size_t      confstr(int, char *, size_t);
int         dup(int);
int         dup2(int, int);
int         fdatasync(int);
long        fpathconf(int, int);
int         fsync(int);
int         ftruncate(int, off_t);
char *      getcwd(char *, size_t);
int         gethostname(char *, size_t);
pid_t       getpid(void);
int         link(const char *, const char *);
off_t       lseek(int, off_t, int);
long        pathconf(const char *, int);
int         pause(void);
ssize_t     read(int, void *, size_t);
int         rmdir(const char *);
unsigned    sleep(unsigned);
void        swab(const void *, void *, ssize_t);
long        sysconf(int);
int         unlink(const char *);
ssize_t     write(int, const void *, size_t);

/* Externs.  */
extern char *optarg;
extern int optind, opterr, optopt;

#ifdef __cplusplus
}
#endif  /*  __cplusplus  */

#endif /* _SYS_UNISTD_H */
#endif /* __Iunistd */
#endif /* __UNISTD_H_ */
#endif /* _UNISTD_H_ */
#endif /* _INC_UNISTD */
#endif /* __UNISTD_H */
#endif /* _UNISTD */
#endif /* __unistd_h */
#endif /* _UNISTD_H */

#endif /* NU_PSX_UNISTD_H */
