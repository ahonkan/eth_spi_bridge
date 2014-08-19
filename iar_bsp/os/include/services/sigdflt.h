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
*       sigdflt.h              
*
* COMPONENT
*
*		SC - Signal Control.
*
* DESCRIPTION
*
*		This file contains the default routines for Signals.
*
* DATA STRUCTURES
*
*		None
*
* DEPENDENCIES
*
*		None
*
************************************************************************/
#ifndef __SIGDFLT_H_
#define __SIGDFLT_H_

/* Empty signal mask */
#define SIGNAL_EMPTY_MASK  0x00000000
/* All signal mask */
#define SIGNAL_ALL_MASK    (~0)

/* Thread default signal handlers */
/* pthread Kill Signal Handler */
void _posix_kill_handler(int signo);
/* Ignore Signal Handler */
void _posix_ignore_handler(int signo);
/* Stop Signal Handler */
void _posix_stop_handler(int signo);

/* Process default signal handlers */
/* Kill Signal Handler */
void POSIX_SYS_SC_Proc_Kill_Handler(int signo);
/* Stop Signal Handler */
void POSIX_SYS_SC_Proc_Stop_Handler(int signo);
/* Signal continue function */
void POSIX_SYS_SC_Proc_Sig_Cont(void);

/* Function to test whether the signal is non mask-able */
void sig_non_maskable(sigset_t *set);
/* Internal routine called by sigsuspend() */
int signal_wait_common(pid_t pid,int timed, const struct timespec *tmo,
                            const sigset_t *set, siginfo_t *info,int flag);
/* Internal routine to deliver a signal */
int signal_thread(pid_t pid,pthread_t thread,siginfo_t*  info);

/* Init sigaction */
int sigaction_init(pid_t pid);

/* Internal routine to handle signals */
void POSIX_Signal_Handle(void);

/* Deactivate signal */
VOID SC_Thread_Deactivates_Signal(PPROC_THREAD_RES *psx_res, pthread_t tid);

/* Reactivate signal */
VOID SC_Thread_Reactivates_Signal(PPROC_THREAD_RES *psx_res, pthread_t tid);

/* Get POSIX active signal */
BOOLEAN SC_Thread_Gets_Active_Signal(PPROC_THREAD_RES *psx_res, pthread_t tid);

/* type definition of signal function */
typedef void POSIX_SIGFUNC(int);

/* Default thread signal handlers */

/* Signal Actions for different type of signals */
#define SIGACTION_KILL \
  {_posix_kill_handler,0, SIGNAL_ALL_MASK, (void (*)(int, siginfo_t *,\
  void *))_posix_kill_handler}

#define SIGACTION_IGNORE \
  {_posix_ignore_handler,0, SIGNAL_ALL_MASK,(void (*)(int, siginfo_t *,\
  void *))_posix_ignore_handler}

#define SIGACTION_DFL \
  {SIG_DFL,0, SIGNAL_ALL_MASK,(void (*)(int, siginfo_t *, void *))SIG_DFL}

#define SIGACTION_STOP \
   {_posix_stop_handler,0, SIGNAL_ALL_MASK,(void (*)(int, siginfo_t *,\
   void *))_posix_stop_handler}
   
/* Default process signal handlers */
#define SIGNAL_KILL \
  {POSIX_SYS_SC_Proc_Kill_Handler,0, SIGNAL_ALL_MASK, (void (*)(int, siginfo_t *,\
  void *))POSIX_SYS_SC_Proc_Kill_Handler}

#define SIGNAL_STOP \
   {POSIX_SYS_SC_Proc_Stop_Handler,0, SIGNAL_ALL_MASK,(void (*)(int, siginfo_t *,\
   void *))POSIX_SYS_SC_Proc_Stop_Handler}

#endif  /*  __SIGDFLT_H_  */




