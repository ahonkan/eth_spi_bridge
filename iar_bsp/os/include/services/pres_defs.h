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
*       pres_defs.h
*
* COMPONENT
*
*       RES - POSIX Resource
*
* DESCRIPTION
*
*       This file contains definitions to entire posix resource for one
*       process.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       pfile.h
*       pprocres.h
*       pfileres.h
*
************************************************************************/
#ifndef __PRES_DEFS_H_
#define __PRES_DEFS_H_

#include "services/pfile.h"
#include "services/pprocres.h"
#include "services/pfileres.h"

/* Maximum signal events */
#define PTHREAD_SIGEVENT_MAX               \
(                                          \
( (PTHREAD_THREADS_MAX + 2) % 32 == 0) ?   \
( (PTHREAD_THREADS_MAX + 2) / 32) :        \
( (PTHREAD_THREADS_MAX + 2) / 32) + 1      \
)                                          \

/* Total amount of Nucleus POSIX base resource per process.
The macros are configurable and mostly defined within limits.h
and static per process. Please refer to POSIX specification for
details. */
#if(POSIX_INCLUDE_DDL)
/* Due to DDL eport table dependency toward Nucleus POSIX
Mailbox and Task control block, both are declared as static and
the resources are set to zero. */
#define PTHREAD_TCB_RES_MAX   		0
#define PTHREAD_MAILBOX_RES_MAX 	0
#else
/* Otherwise Nucleus POSIX Mailbox and Task control block have
resource values and will be created dynamically */
#define PTHREAD_TCB_RES_MAX   	    ((PTHREAD_THREADS_MAX + 2)*sizeof(POSIX_TCB))
#define PTHREAD_MAILBOX_RES_MAX     (sizeof(NU_MAILBOX))
#endif /* POSIX_INCLUDE_DDL */

#define POSIX_BASE_MIN_SYSTEM_MEMORY_RESOURCE                   \
(                                                               \
/* Thread resource */                                           \
(sizeof(PPROC_THREAD_RES))                                      \
+(sizeof(PPROC_FS_RES))                                         \
+ PTHREAD_TCB_RES_MAX                                           \
+((PTHREAD_THREADS_MAX + 2)*sizeof(POSIX_THD_RES_TABLE))        \
+((PTHREAD_THREADS_MAX + 2)*sizeof(POSIX_THD_STACK_STATUS))     \
+ PTHREAD_MAILBOX_RES_MAX                                       \
+((PTHREAD_KEYS_MAX)*sizeof(POSIX_KEY_DATA))                    \
+(PTHREAD_DEFAULT_STACKSIZE*sizeof(CHAR))                       \
/* MQ resource */                                               \
+((MQ_OPEN_MAX+1)*sizeof(POSIX_MCB))                            \
+((MQ_OPEN_MAX+1)*sizeof(POSIX_MQD))                            \
+((MQ_OPEN_MAX+1)*sizeof(POSIX_MQ_RES_TABLE))                   \
/* Signal Resource */                                           \
+(PTHREAD_SIGEVENT_MAX*sizeof(NU_EVENT_GROUP))                  \
+(MAX_NO_SIGNALS*sizeof(struct sigaction)*32)                   \
+((SIGQUEUE_MAX + 1)*sizeof(POSIX_SIGCB))                       \
+((PTHREAD_THREADS_MAX+2)*sizeof(POSIX_SIGNAL_INTR))            \
/* Semaphore resource */                                        \
+((SEM_NSEMS_MAX + 1)*sizeof(POSIX_SCB))                        \
+((SEM_NSEMS_MAX + 1)*sizeof(POSIX_SM_RES_TABLE))               \
/* Timer Resource */                                            \
+((TIMER_MAX+1)*sizeof(POSIX_TIMERCB))                          \
+((TIMER_MAX+1)*sizeof(POSIX_TMR_RES_TABLE))                    \
)                                                               \

/* Total amount of Nucleus POSIX FILE/AIO/NET
   Resource per process */
#define POSIX_FILE_MIN_SYSTEM_MEMORY_RESOURCE                   \
(                                                               \
/* AIO events resources */                                      \
(PTHREAD_SIGEVENT_MAX*sizeof(NU_EVENT_GROUP))                   \
/* FILE/NET resource */                                         \
+(sizeof(POSIX_MMAP))                                            \
+(OPEN_MAX*sizeof(POSIX_PROC_FD))                               \
+(sizeof(NU_SEMAPHORE))                                         \
+(OPEN_MAX*sizeof(PSX_OPEN_DIR))                                \
+(sizeof(NU_SEMAPHORE))                                         \
+((PTHREAD_THREADS_MAX + 2)*sizeof(pthread_t))                  \
)

/* Total System resource. The following size must be smaller than
   Kernel SYSTEM_MEMORY_SIZE. Use 2048 (2k) to leave extra
   space at the end. This space can be reduced or eliminated
   if not necessary. */
#define POSIX_SYSTEM_MEMORY_RESOURCE                            \
        (POSIX_BASE_MIN_SYSTEM_MEMORY_RESOURCE +                \
        POSIX_FILE_MIN_SYSTEM_MEMORY_RESOURCE)                  \

/* Nucleus POSIX Total System Resource will be multiple aligned PAGESIZE */
#define POSIX_TOTAL_MIN_SYSTEM_MEMORY_RESOURCE  (UNSIGNED)(((((UNSIGNED)                \
                                                POSIX_SYSTEM_MEMORY_RESOURCE)/PAGESIZE) \
                                                *PAGESIZE)+PAGESIZE)                    \

#endif
