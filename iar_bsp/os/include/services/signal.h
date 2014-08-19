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
*       signal.h
*
*   COMPONENT
*
*		SC - Signal Control.
*
*   DESCRIPTION
*
*		This file contains the various routines for signals.
*
*		NOTE: Standard C RTL header.
*
*   DATA STRUCTURES
*
*		sigval_t							Signal value.
*		sigevent							Signal event.
*		siginfo_t							Signal Information.
*		sigaction							Signal Action.
*
*   DEPENDENCIES
*
*       config.h        Nucleus POSIX configuration definitions
*       compiler.h      Nucleus POSIX compiler-specific definitions
*       types.h         POSIX types.h definitions
*       unistd.h        POSIX unistd.h definitions
*       time.h          POSIX time.h definitions
*
*************************************************************************/

#ifndef NU_PSX_SIGNAL_H
#define NU_PSX_SIGNAL_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/unistd.h"

/* For Metaware Metrowerks and KMC GNU Tools.  */
#ifndef _SIGNAL_H
#define _SIGNAL_H

/* For ADS Tools.  */
#ifndef __signal_h
#define __signal_h

/* For Hitachi Tools and TI Tools.  */
#ifndef _SIGNAL
#define _SIGNAL

/* For Paradigm Tools and Microtec Tools.  */
#ifndef __SIGNAL_H
#define __SIGNAL_H

/* For Microsoft Visual C.  */
#ifndef _INC_SIGNAL
#define _INC_SIGNAL

#ifndef __SIGNAL_H_
#define __SIGNAL_H_

/* For Code Sourcery ARM GNU */
#ifndef _SIGNAL_H_
#define _SIGNAL_H_

/* For Code Sourcery ARM GNU Lite (newlib C library) */
#ifndef _SYS_SIGNAL_H
#define _SYS_SIGNAL_H

/* For DIAB tools */
#ifndef __Isignal
#define __Isignal

/* Request for default signal handling.  */
#define SIG_IGN     ((void (*)(int))0)
#define SIG_DFL     ((void (*)(int))1)
#define SIG_ERR     ((void (*)(int))-1)

typedef union sigval
{
    int     sival_int;                      /* Integer signal value.  */
    void*   sival_ptr;                      /* Pointer signal value.  */
}sigval_t;

/* Signal Numbers.  */
#define SIGABRT         1                   /* Process abort signal.  */
#define SIGFPE          2                   /* SIGFPE.  */
#define SIGILL          3                   /* Illegal instruction.  */
#define SIGINT          4                   /* Terminal interrupt
                                               signal.  */
#define SIGSEGV         5                   /* Invalid memory
                                               reference.  */
#define SIGTERM         6                   /* Termination  signal.  */
#define SIGALRM         7                   /* Alarm clock.  */
#define SIGHUP          8                   /* Hangup.  */
#define SIGKILL         9                   /* Kill (cannot be caught or
                                               ignored).  */
#define SIGPIPE         10                  /* Write on a pipe with no one
                                               to read it.  */
#define SIGQUIT         11                  /* Terminal quit signal.  */
#define SIGUSR1         12                  /* User-defined signal 1.  */
#define SIGUSR2         13                  /* User-defined signal 2.  */
#define SIGCHLD         14                  /* Child process terminated,
                                               stopped.  */
#define SIGCONT         15                  /* Continue executing, if
                                               stopped.  */
#define SIGSTOP         16                  /* Stop executing (cannot be
                                               caught or ignored).  */
#define SIGTSTP         17                  /* Terminal stop signal.  */
#define SIGTTIN         18                  /* Background process
                                               attempting read.  */
#define SIGTTOU         19                  /* Background process
                                               attempting write.  */
#define SIGBUS          20                  /* Access to an undefined
                                               portion of a memory
                                               object.  */
#define SIGRTMIN        (SIGBUS +   1)      /* It specifies a range of
                                               signal numbers that are
                                               reserved for application
                                               use.  */
#define SIGRTMAX        32

#define SA_SIGINFO      1                   /* Causes extra information to
                                               be passed to signal handlers
                                               at the time of receipt of a
                                               signal.  */
#if (_POSIX_REALTIME_SIGNALS != -1)

/* sigev_notify values.  */
#define SIGEV_NONE      0                   /* No asynchronous notification
                                               is delivered when the event
                                               of interest occurs.  */
#define SIGEV_SIGNAL    1                   /* A queued signal, with an
                                               application-defined value,
                                               is generated when the event
                                               of interest occurs.  */
#define SIGEV_THREAD    2                   /* Notification function is
                                               called to perform
                                               notification.  */

#ifndef __SIGEVENT_T_
#define __SIGEVENT_T_

struct sigevent
{
    int     sigev_notify;                   /* Notification type.  */
    int     sigev_signo;                    /* Signal number.  */
    union   sigval sigev_value;             /* Signal value.  */

    /* Notification Function.  */
    void (*sigev_notify_function)(union sigval);

    /* Notification Attributes.  */
    pthread_attr_t*     sigev_notify_attributes;
};

#endif  /*  __SIGEVENT_T_  */

#endif /*  _POSIX_REALTIME_SIGNALS  */

/* Modes for pthread_sigmask().  */

#define SIG_BLOCK   0                       /* The resulting set is the
                                               union of the current set and
                                               the signal set pointed to by
                                               the argument set.  */
#define SIG_UNBLOCK 1                       /* The resulting set is the
                                               intersection of the current
                                               set and the complement of
                                               the signal set pointed to by
                                               the argument set.  */
#define SIG_SETMASK 2                       /* The resulting set is the
                                               signal set pointed to by the
                                               argument set.  */

/* Signal Modes corresponding to si_code */
#define SI_PTKILL   0                       /* Signal for thread kill */
#define SI_QUEUE    1                       /* Signal sent by the
                                               sigqueue().  */
#define SI_USER     2                       /* Signal sent by kill ().  */
#define SI_TIMER    4                       /* Signal generated by
                                               expiration of a timer set by
                                               timer_settime().  */
#define SI_ASYNCIO  8                       /* Signal generated by
                                               completion of an
                                               asynchronous I/O
                                               request.  */
#define SI_MESGQ    16                      /* Signal generated by arrival
                                               of a message on an empty
                                               message queue.  */

typedef unsigned long sigset_t;

typedef struct signal_info
{
    int     si_signo;
    int     si_code;
    union   sigval si_value;
} siginfo_t;

struct sigaction
{
    void (*sa_handler)(int);                /* What to do on receipt of
                                               signal.  */
    sigset_t sa_mask;                       /* Set of signals to be blocked
                                               during execution of the
                                               signal handling
                                               function.  */
    int sa_flags;                           /* Special flags.  */

    /* Pointer to signal handler function or one of the macros SIG_IGN or
       SIG_DFL.  */
    void (*sa_sigaction)(int, siginfo_t *, void *);

};

/* Function Prototypes.  */

#ifdef __cplusplus
extern "C" {
#endif /*  __cplusplus  */

int kill(pid_t, int);

#if (_POSIX_THREADS != -1)

int pthread_kill(pthread_t thread, int sig);

int pthread_sigmask(int, const sigset_t*, sigset_t*);

#endif  /*  _POSIX_THREADS  */

int raise(int sig);

int sigaction(int sig, const struct sigaction *act,struct sigaction *oact);

int sigaddset(sigset_t *set, int signo);

int sigdelset(sigset_t *set, int signo);

int sigemptyset(sigset_t *set);

int sigfillset(sigset_t *set);

int sigismember(const sigset_t *set, int signo);

int sigpending(sigset_t *set);

int sigprocmask(int how, const sigset_t *set, sigset_t *oset);

int sigsuspend(const sigset_t*  sigmask);

int sigwait(const sigset_t* set, int* sig);

void (*signal(int, void (*)(int)))(int);

#if (_POSIX_REALTIME_SIGNALS != -1)

#include "services/sys/time.h"

int sigqueue(pid_t pid, int signo, const union sigval value);

int sigtimedwait(const sigset_t *set, siginfo_t *info,
                                           const struct timespec *timeout);

int sigwaitinfo(const sigset_t *set, siginfo_t *info);

#endif /* _POSIX_REALTIME_SIGNALS */

#ifdef __cplusplus
}
#endif /*  __cplusplus  */

#endif  /* __Isignal */
#endif  /* _SYS_SIGNAL_H */
#endif  /* _SIGNAL_H_ */
#endif  /* __SIGNAL_H_ */
#endif  /* _INC_SIGNAL */
#endif  /* __SIGNAL_H */
#endif  /* _SIGNAL */
#endif  /* __signal_h */
#endif  /* _SIGNAL_H */

#endif /* NU_PSX_SIGNAL_H */
