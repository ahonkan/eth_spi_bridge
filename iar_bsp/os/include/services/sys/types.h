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
*       types.h
*
*   COMPONENT
*
*       PX - POSIX.
*
*   DESCRIPTION
*
*       Contains the various data types.
*
*   DATA STRUCTURES
*
*       blksize_t                           Used for block sizes.
*       blkcnt_t                            Used for file block counts.
*       pid_t                               Used for process IDs and
*                                           process group IDs.
*       pthread_t                           Used to identify a thread.
*       pthread_key_t                       Used for thread-specific data
*                                           keys.
*       ssize_t                             Used for a count of bytes or an
*                                           error indication.
*       mode_t                              Used for some file attributes.
*       off_t                               Used for file sizes.
*       pthread_once_t                      Used for dynamic package
*                                           initialization.
*       pthread_attr_t                      Used to identify a thread
*                                           attribute object.
*       pthread_mutex_t                     Used for mutexes.
*       pthread_mutexattr_t                 Used to identify a mutex
*                                           attribute object.
*       clock_t                             Used for system times in clock
*                                           ticks.
*       clockid_t                           Used for clock ID type in the
*                                           clock and timer functions.
*       timer_t                             Used for timer ID returned by
*                                           timer_create().
*       time_t                              Used for time in seconds.
*       size_t                              Used for sizes of objects.
*       dev_t                               Used for device IDs.
*       gid_t                               Used for group IDs.
*       uid_t                               Used for user IDs.
*       nlink_t                             Used for link counts.
*       ino_t                               Used for file serial numbers.
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*       nucleus.h           Nucleus OS definitions
*       plus_core.h         Nucleus PLUS definitions
*       stddef.h            POSIX stddef.h definitions
*
*************************************************************************/

#ifndef NU_PSX_TYPES_H
#define NU_PSX_TYPES_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/stddef.h"

/* For Paradigm Tools and Microtec Tools.  */
#ifndef __TYPES_H_
#define __TYPES_H_

/* For MinGNU or other GNU toolsets  */
#ifndef _TYPES_H_
#define _TYPES_H_

/* For Code Sourcery ARM GNU */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

/* For DIAB tools */
#ifndef __Itypes
#define __Itypes

typedef int blksize_t;

typedef int blkcnt_t;

typedef int pid_t;                          /* Process ID.  */

typedef int pthread_t;                      /* Thread ID */

typedef int pthread_key_t;                  /* Thread Key */

typedef int ssize_t;

typedef unsigned int mode_t;

typedef long off_t;

typedef unsigned long    clock_t;           /* Used for system times in
                                               clock ticks or
                                               CLOCKS_PER_SEC.  */

typedef int     clockid_t;                  /* Used for clock ID type in
                                               the clock and timer
                                               functions.  */

typedef int     timer_t;                    /* Used for timer ID returned
                                               by timer_create().  */

typedef long time_t;                        /* Used for time in
                                               seconds.  */

typedef unsigned long suseconds_t;         /* Used for time in
                                              microseconds */

#include "nucleus.h"
#include "kernel/plus_core.h"

typedef struct
{

    NU_SEMAPHORE nu_scb;                    /* Nucleus Semaphore Control
                                               Block.  */
    int done;
} pthread_once_t;

typedef struct
{
    void*           kernel_stack_addr;      /* Kernel Mode Stack
                                               Address.  */
    long            rrinterval;             /* Round Robin Interval in
                                               nanoseconds.  */
    size_t          kernel_stack_size;      /* Kernel Mode Stack Size.  */
    int             is_initialized;         /* 0 to destroy the
                                               attributes.  */
    int             inheritsched;           /* Inherited Scheduler
                                               Parameter.  */
    int             schedpolicy;            /* Scheduler Policy.  */
    int             priority;               /* Priority.  */
    int             detach_state;           /* Detach State.  */
    int             contention_scope;       /* Contention Scope.  */
    size_t          guardsize;              /* Stack guard area size. */
} pthread_attr_t;

typedef struct
{
    NU_SEMAPHORE    nu_scb;                 /* Nucleus Semaphore Control
                                               Block.  */
    void*           owner_task_p;           /* Mutex owner Task
                                               Pointer.  */
    unsigned int    option;                 /* PRIORITY_NONE,
                                               PRIORITY_PROTECT,
                                               PRIORITY_INHERIT.  */
    int             prioceiling;            /* Prioceiling value in case of
                                               Option PRIORITY_INHERIT.  */
    int             owner_priority;         /* Mutex owner Task
                                               Priority.  */
    unsigned long   mutex_id;               /* Mutex ID */

} pthread_mutex_t;

typedef struct
{
    int             is_initialized;         /* Flag to tell whether the
                                               mutex attribute are
                                               initialized or not.  */
    int             protocol;               /* Can be PRIO_NONE,
                                               PRIO_INHERIT or
                                               PRIO_PROTECT.  */
    int             prioceiling;            /* Value of the priority
                                               ceiling.  */
    int             pshared;                /* Flag to tell whether mutex
                                               can be shared across
                                               process or not. */
    int             type;                   /* The type of mutex. */
} pthread_mutexattr_t;

typedef struct
{
#if defined(C55X) && defined(MOT)
    unsigned long nu_cond[10];              /* Required to hold condition
                                            control block. */
#else
    unsigned long nu_cond[8];               /* Required to hold condition
                                            control block. */
#endif /* C55X and MOT */
}pthread_cond_t;

typedef struct
{
    int             is_initialized;         /*  Flag to tell whether the
                                                condition attribute are
                                                initialized or not.  */
    int             pshared;                /*  Flag to tell whether
                                                condition variable can be
                                                shared across processes or
                                                not */
} pthread_condattr_t;

typedef unsigned short dev_t;

typedef unsigned short gid_t;

typedef unsigned short uid_t;

typedef unsigned short nlink_t;

typedef unsigned long ino_t;

#endif  /* #ifndef __Itypes     */
#endif  /* #ifndef _SYS_TYPES_H */
#endif  /* #ifndef _TYPES_H_    */
#endif  /* #ifndef __TYPES_H_   */

#endif /* NU_PSX_TYPES_H */

