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
*       pthread.h
*
*   COMPONENT
*
*       TC - Thread Control.
*
*   DESCRIPTION
*
*       Contains the POSIX Threads related definitions.
*
*   DATA STRUCTURES
*
*       _pthread_cleanup_t                  Contains Thread CleanUp info.
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*       unistd.h            POSIX unistd.h definitions
*       stddef.h            POSIX stddef.h definitions
*       nucleus.h           Nucleus OS definitions
*       nu_kernel.h         Nulceus PLUS definitions
*       types.h             POSIX types.h definitions
*       sched.h             POSIX sched.h definitions
*       time.h              POSIX time.h definitions
*
*************************************************************************/

#ifndef NU_PSX_PTHREAD_H
#define NU_PSX_PTHREAD_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/unistd.h"
#include "services/stddef.h"
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/sys/types.h"
#include "services/sched.h"
#include "services/sys/time.h"

#ifndef __PTHREAD_H_
#define __PTHREAD_H_

#ifndef _PTHREAD_H
#define _PTHREAD_H

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef __PTHREAD_h
#define __PTHREAD_h

#if (_POSIX_THREADS != -1)

#define PTHREAD_CANCEL_ASYNCHRONOUS 0
#define PTHREAD_CANCEL_DEFERRED     1

#define PTHREAD_CANCEL_ENABLE       0
#define PTHREAD_CANCEL_DISABLE      1
#define PTHREAD_CANCELED            ((void*)2)

#define PTHREAD_CREATE_JOINABLE     0
#define PTHREAD_CREATE_DETACHED     1

#define PTHREAD_EXPLICIT_SCHED      0
#define PTHREAD_INHERIT_SCHED       1

#define PTHREAD_PRIO_NONE           0
#define PTHREAD_PRIO_INHERIT        1
#define PTHREAD_PRIO_PROTECT        2

#define PTHREAD_PROCESS_PRIVATE     0
#define PTHREAD_PROCESS_SHARED      1

/* Macro for get and set mutex type attribute */
#define PTHREAD_MUTEX_ERRORCHECK    0
#define PTHREAD_MUTEX_NORMAL        1
#define PTHREAD_MUTEX_RECURSIVE     2
/* Default get and set mutex type attribute */
#define PTHREAD_MUTEX_DEFAULT       PTHREAD_MUTEX_ERRORCHECK

#define PTHREAD_MUTEX_NUC_SEM_ID    0x53454d41UL

#if(_POSIX_THREAD_PRIORITY_SCHEDULING != -1)

#define PTHREAD_SCOPE_SYSTEM        0
#define PTHREAD_SCOPE_PROCESS       1

#endif  /*  _POSIX_THREAD_PRIORITY_SCHEDULING  */

#if defined(C55X) && defined(MOT)
#define PTHREAD_COND_INITIALIZER  {                                         \
/* Condition Variable Control Block */                                      \
  {0x00000000,0x00000000,0x00000000,0x434F4E44,0x00000000,                  \
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}                   \
}
#else
#define PTHREAD_COND_INITIALIZER  {                                         \
/* Condition Variable Control Block */                                      \
{0x00000000,0x00000000,0x00000000,0x434F4E44,0x00000000,                    \
0x00000000,0x00000000,0x00000000}                                           \
}
#endif /* C55X and MOT */

#define PTHREAD_MUTEX_INITIALIZER {                                         \
   /* Semaphore Control Block */                                            \
  {{0x00000000,0x00000000,0x00000000},      /* CS_NODE */                   \
   PTHREAD_MUTEX_NUC_SEM_ID,                /* ID */                        \
   {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},       /* Name */                      \
   0x00000001,                              /* Count */                     \
   0x00000000,                              /* Suspension */                \
   0x00000000,                              /* Owner */                     \
   0x00000000,                              /* Tasks waiting */             \
   {0x00000000,0x00000000,0x00000000},      /* CS_NODE */                   \
   0x00000000},                             /* Suspension list */           \
   0,                                       /* Owner Task Pointer.  */      \
   PTHREAD_PRIO_NONE,                       /* Protocol.  */                \
   0,                                       /* Prioceiling.  */             \
   0,                                       /* Owner Priority.  */          \
   0                                        /* Mutex ID */                  \
}

#define PTHREAD_ONCE_INIT           {                                       \
   /* Semaphore Control Block */                                            \
  {{0x00000000,0x00000000,0x00000000},      /* CS_NODE */                   \
   PTHREAD_MUTEX_NUC_SEM_ID,                /* ID */                        \
   {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},       /* Name */                      \
   0x00000001,                              /* Count */                     \
   0x00000000,                              /* Suspension */                \
   0x00000000,                              /* Owner */                     \
   0x00000000,                              /* Tasks waiting */             \
   {0x00000000,0x00000000,0x00000000},      /* CS_NODE */                   \
   0x00000000},                             /* Suspension list */           \
   0}

typedef struct _pthread_cleanup_s
{
    struct  _pthread_cleanup_s* next;
    void*   tcb;
    void (*routine)(void *);
    void *argument;
}_pthread_cleanup_t;

#define pthread_cleanup_push(rtn,arg) \
{ \
    _pthread_cleanup_t _cleanup; \
    _pthread_cleanup_push(&_cleanup, rtn, arg);

#define pthread_cleanup_pop(exe) \
    _pthread_cleanup_pop(&_cleanup, exe); \
}

#ifdef __cplusplus
extern "C" {
#endif

int pthread_atfork(void (*)(void), void (*)(void),void(*)(void));

int pthread_attr_destroy(pthread_attr_t* attr);

#if (_POSIX_THREAD_PRIORITY_SCHEDULING != -1)

int pthread_attr_getinheritsched(const pthread_attr_t*  attr,
                                                       int*  inheritsched);

int pthread_attr_getschedpolicy(const pthread_attr_t*   attr,
                                                            int*   policy);

int pthread_attr_getscope(const pthread_attr_t* attr,int* contentionscope);

int pthread_attr_setscope(pthread_attr_t*   attr,int contentionscope);

int pthread_getschedparam(pthread_t thread, int* policy,
                                               struct sched_param*  param);

#endif /*  _POSIX_THREAD_PRIORITY_SCHEDULING  */

#if (_POSIX_THREAD_ATTR_STACKSIZE != -1) \
 && (_POSIX_THREAD_ATTR_STACKADDR != -1)

int pthread_attr_getstack(const pthread_attr_t*     attr,
                          void**                    stackaddr,
                          size_t*                   stacksize);

int pthread_attr_setstack(pthread_attr_t*   attr,
                          void*             stackaddr,
                          size_t            stacksize);

#endif  /* #if (_POSIX_THREAD_ATTR_STACKSIZE != -1) \
            && (_POSIX_THREAD_ATTR_STACKADDR != -1) */

#if (_POSIX_THREAD_ATTR_STACKADDR != -1)

int pthread_attr_getstackaddr(const pthread_attr_t* attr,void** stackaddr);

int pthread_attr_getstacksize(const pthread_attr_t* attr,
                                                        size_t* stacksize);

int pthread_attr_setstackaddr(pthread_attr_t*   attr, void* stackaddr);

int pthread_attr_setstacksize(pthread_attr_t*   attr, size_t stacksize);

#endif  /*  _POSIX_THREAD_ATTR_STACKADDR  */

int pthread_attr_getguardsize(const pthread_attr_t*     attr,
                              size_t*                   guardsize);
                              
int pthread_attr_setguardsize(pthread_attr_t*   attr,
                              size_t            guardsize);                            

int pthread_attr_setdetachstate(pthread_attr_t* attr,int detachstate);

int pthread_attr_getdetachstate(const pthread_attr_t* attr,
                                                         int* detachstate);
/* Our defined.  */
int pthread_attr_setrrinterval(pthread_attr_t*  attr,
                                          const struct timespec* interval);
/* Our defined.  */
int pthread_attr_getrrinterval(const pthread_attr_t* attr,
                                                struct timespec* interval);

int pthread_attr_getschedparam(const pthread_attr_t* attr,
                                                struct sched_param* param);

int pthread_attr_init(pthread_attr_t*   attr);

#if (_POSIX_THREAD_PRIORITY_SCHEDULING != -1)

int pthread_attr_setinheritsched(pthread_attr_t*    attr,int inheritsched);

int pthread_attr_setschedpolicy(pthread_attr_t* attr, int policy);

int pthread_setschedparam(pthread_t thread, int policy,
                                       const struct sched_param*    param);

#endif

int pthread_attr_setschedparam(pthread_attr_t*  attr,
                                         const struct sched_param*  param);

int pthread_setconcurrency(int new_level);

int pthread_getconcurrency(void);

int pthread_cancel(pthread_t thread);

int pthread_setcanceltype(int  type,int*    oldtype);

int pthread_setcancelstate(int  state,int*  oldstate);

void pthread_testcancel( void );

int pthread_join(pthread_t   thread,void**  value_ptr);

int pthread_detach(pthread_t   thread);

void _pthread_cleanup_pop(_pthread_cleanup_t*   cleanup, int exe);

void _pthread_cleanup_push(_pthread_cleanup_t*  cleanup,
                                      void (*routine)(void *), void*  arg);

int pthread_create(pthread_t*   thread,const pthread_attr_t*    attr,
                           void* (*start_routine)(void*   arg), void* arg);

int pthread_equal(pthread_t t1, pthread_t t2);

void pthread_exit(void* value_ptr);

void*   pthread_getspecific(pthread_key_t key);

int pthread_key_create(pthread_key_t*   key, void (*destructor)(void *));

int pthread_key_delete(pthread_key_t    key);

int pthread_once(pthread_once_t* once_control,void (*init_routine)(void));

int pthread_setspecific(pthread_key_t key, const void*  value);

pthread_t pthread_self(void);

/* Condition Variable related function prototypes.  */

int pthread_cond_broadcast(pthread_cond_t*   cond);

int pthread_cond_destroy(pthread_cond_t*    cond);

int pthread_cond_init(pthread_cond_t*   cond,
                                           const pthread_condattr_t* attr);

int pthread_cond_signal(pthread_cond_t* cond);

int pthread_cond_timedwait(pthread_cond_t*  cond,pthread_mutex_t* mutex,
                                         const struct timespec*   abstime);

int pthread_cond_wait(pthread_cond_t*   cond,pthread_mutex_t*   mutex);

int pthread_condattr_init(pthread_condattr_t* attr);

int pthread_condattr_destroy(pthread_condattr_t* attr);

int pthread_condattr_getpshared(const pthread_condattr_t *attr,
                                                             int *pshared);

int pthread_condattr_setpshared(pthread_condattr_t *attr,int  pshared);

/* Mutex Related function prototypes.  */

int pthread_mutex_destroy(pthread_mutex_t*  mutex);

int pthread_mutex_init(pthread_mutex_t* mutex,
                                          const pthread_mutexattr_t* attr);

int pthread_mutex_lock(pthread_mutex_t* mutex);

int pthread_mutex_trylock(pthread_mutex_t*  mutex);

int pthread_mutex_unlock(pthread_mutex_t*   mutex);

int pthread_mutexattr_destroy(pthread_mutexattr_t*  attr);

int pthread_mutexattr_setpshared(pthread_mutexattr_t*   attr,int pshared);

int pthread_mutexattr_getpshared(const pthread_mutexattr_t* attr,
                                                             int* pshared);

int pthread_mutexattr_init(pthread_mutexattr_t* attr);

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr,int *type);

int pthread_getcpuclockid(pthread_t thread_id, clockid_t *clock_id);

#if (_POSIX_THREAD_PRIO_PROTECT != -1)

int pthread_mutex_getprioceiling(const pthread_mutex_t* mutex,
                                                         int* prioceiling);

int pthread_mutex_setprioceiling(pthread_mutex_t*   mutex,int prioceiling,
                                                         int* old_ceiling);

int pthread_mutexattr_setprioceiling(pthread_mutexattr_t* attr,
                                                          int prioceiling);

int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t* attr,
                                                        int*  prioceiling);

#endif  /*  _POSIX_THREAD_PRIO_PROTECT  */

#if (_POSIX_THREAD_PRIO_INHERIT != -1)||(_POSIX_THREAD_PRIO_PROTECT != -1)

int pthread_mutexattr_setprotocol(pthread_mutexattr_t* attr,int protocol);

int pthread_mutexattr_getprotocol(const pthread_mutexattr_t* attr,
                                                            int* protocol);

#endif

#if(_POSIX_TIMEOUTS != -1)

int pthread_mutex_timedlock(pthread_mutex_t * mutex,
       const struct timespec * abs_timeout);

#endif


#ifdef __cplusplus
}
#endif

#endif /* _POSIX_THREADS */

#endif /* __PTHREAD_h */
#endif /* _PTHREAD_H */
#endif /* __PTHREAD_H_ */

#endif /* NU_PSX_PTHREAD_H */
