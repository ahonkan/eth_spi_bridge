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
*       putil.h
*
* COMPONENT
*
*       PX - POSIX
*
* DESCRIPTION
*
*       Contains the various utility routines used by Nucleus POSIX
*       internally.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
************************************************************************/
#ifndef __UTIL_H_
#define __UTIL_H_

#include "services/pprocres.h"

/* Error value that is being used within Nucleus NET when signal is
lifting Nucleus NET suspension. The error value is also defined
inside Nucleus NET file, sockdefs.h */
#ifndef NU_ERROR_INTR
#define NU_ERROR_INTR               -332
#endif /* NU_ERROR_INTR */

/* Function Declarations.  */
#ifdef __cplusplus
extern "C" {
#endif

#if (_POSIX_THREADS !=  -1)

#if (POSIX_CHECK_STACK_ENABLED == 1)

/* The following code supports the stack checking and condition handling
   features of a POSIX implementation. */

/* Function prototypes */

int POSIX_Stack_Handle_Overflow(pid_t      pid,
                                pthread_t  tid);

VOID POSIX_Stack_Check(VOID);

/* POSIX Stack Check macro - This macro is used to insert the stack check
   (and handling) code into the POSIX source code.  It is implemented so
   that it may be completely removed from the POSIX source at compile-time
   if desired.*/

#define POSIX_CHECK_STACK                               POSIX_Stack_Check();

#else

#define POSIX_CHECK_STACK

#endif /* POSIX_CHECK_STACK_ENABLED */

#endif /* _POSIX_THREADS !=  -1 */

#if (_POSIX_MESSAGE_PASSING !=  -1)
POSIX_MCB*  FindMessageQueue(PPROC_THREAD_RES *psx_res, const char* name);
#endif
#if (_POSIX_SEMAPHORES  !=  -1)
POSIX_SCB*  FindSem(PPROC_THREAD_RES *psx_res, const char* name);
#endif

/* Convert timespec to ticks */
UNSIGNED timespec2ticks(const struct timespec* tp);

/* Convert ticks to timespec */
struct timespec ticks2timespec(UNSIGNED ticks);

/* Convert 64-bit ticks to timespec */
struct timespec ticks2timespec64(UINT64 ticks);

/* Create the unique name. */
VOID POSIX_Create_Name(CHAR *buf, CHAR *base_name, INT id);

/* Getting error interrupt. */
UINT16 PX_Thread_Gets_Err_Intr(PPROC_THREAD_RES *psx_res, INT thread_id);

/* Setting error interrupt */
VOID PX_Thread_Sets_Err_Intr(PPROC_THREAD_RES *psx_res,pthread_t tid, UINT16 sig_no);

/* Popping all the signal cleanup handlers */
VOID PX_Thread_Pop_Cleanup_Handler(pid_t pid, PPROC_THREAD_RES *psx_res, NU_TASK* current_task);

/* Lower level condition variable signaling mechanism */
INT PX_Thread_Signals_Condvar(pthread_cond_t *cond,BOOLEAN resume);

/* POSIX thread system error */
VOID PX_Thread_System_Error(VOID);

/* Mapping old function to the new one */
#define POSIX_Get_Eintr     PX_Thread_Gets_Err_Intr

#ifdef __cplusplus
}
#endif

#endif  /*  __UTIL_H_  */

