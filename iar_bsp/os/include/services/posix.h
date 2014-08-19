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
*       posix.h
*
* COMPONENT
*
*       PX - POSIX
*
* DESCRIPTION
*
*       Contains the various Nucleus POSIX internal declarations and
*       definitions.
*
* DATA STRUCTURES
*
*       cblink                              Node for linking in the linked
*                                           lists.
*       POSIX_SIGCB                         Nucleus POSIX Internal Signal Control
*                                           Block.
*       POSIX_TCB                           Nucleus POSIX Internal Task Control
*                                           Block.
*       POSIX_MCB                           Nucleus POSIX Internal Message Queue
*                                           Control Block.
*       POSIX_MQD                           Nucleus POSIX Internal Message Queue
*                                           Descriptor Block.
*       POSIX_BCB                           Nucleus POSIX Internal Message Queue
*                                           Buffer Control Block.
*       POSIX_TIMERCB                       Nucleus POSIX Internal Timer Control
*                                           Block.
*       POSIX_SCB                           Nucleus POSIX Internal Semaphore
*                                           Control Block.
*
* DEPENDENCIES
*
*       "unistd.h"                          Standard Symbolic Constants
*                                           and types.
*       "limits.h"                          Contains implementation
*                                           defined constants.
*       "config.h"                          Contains various Nucleus POSIX
*                                           configurations.
*       "errno.h"                           Contains System Error Numbers.
*       "time.h"                            Contains various time types.
*       "signal.h"                          Contains various signal
*                                           related definitions.
*       "pthread.h"                         Contains Nucleus POSIX thread related
*                                           definitions.
*       "nucleus.h"                         Contains various Nucleus PLUS
*                                           related definitions.
*       "psx_extr.h"                        Nucleus POSIX / Nucleus PLUS
*                                           integration support.
*
************************************************************************/
#ifndef __POSIX_H_
#define __POSIX_H_

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "kernel/plus_core.h"

#ifdef CFG_NU_OS_SVCS_DDL_ENABLE
#warning WARNING: Nucleus POSIX operation with Nucleus DDL is not supported.
#endif /* CFG_NU_OS_SVCS_DDL_ENABLE */

#include "services/compiler.h"
#include "services/unistd.h"
#include "services/limits.h"
#include "services/stddef.h"
#include "services/errno.h"
#include "services/sys/types.h"
#include "services/sys/time.h"
#include "services/signal.h"
#include "services/pthread.h"
#include "services/semaphore.h"
#include "services/psx_extr.h"

#define POSIX_UNUSED_PARAM(param)  (NU_UNUSED_PARAM(param))

#define         NU_PSX_FLOCKFILE       0

/* POSIX_NO_ERROR_CHECKING definition checking */
#if ((defined(POSIX_NO_ERROR_CHECKING)) && (POSIX_NO_ERROR_CHECKING < 1))
#undef     POSIX_NO_ERROR_CHECKING
#endif

#define POSIX_SERIAL        POSIX_IO

/* Memory types */
#define PSX_UM_MEMORY                   1
#define PSX_UNCACHED_MEMORY             2
#define PSX_SYS_MEMORY                  3
#define PSX_SYS_STARTUP_MEMORY          4
#define PSX_SHARED_UM_MEMORY            5

/* The single POSIX process. */
#define         PPROC_PROCESS_STARTUP   0

#define POSIX_MQ_EINTR  -2                  /* Error value for MQ only
                                               whenever interrupt occurs */

/* For the Linked Lists */
#define CB_ID_HEAD  (~0)                    /* Indicates cb head link.  */
#define CB_ID_MASK  (~((unsigned int)(~((unsigned int)0))>>1)) /* Indicates cb ID value.  */

/* Signal validation.  */
#define MAX_NO_SIGNALS  32
#define SIGVAL(no) ((no) >= 1 && (no) <= SIGRTMAX)
#define signo_to_mask(sig) (1UL << (sig -1))

/* Priority Range for the POSIX.  */
#define POSIX_PRIORITY_MAX  255
#define POSIX_PRIORITY_MIN  0

#if (_POSIX_MESSAGE_PASSING !=  -1)

/* Message Queue Related Defines.  */
#define MB_ALIGN            4
#define MQ_MAXMSG           32              /* Maximum no of Messages which
                                               can be accommodated in the
                                               Queue.  */
#define MQ_MSGSIZE          32              /* Maximum Message Size of one
                                               Message.  */
#define BCB_SIZE            ((sizeof(POSIX_BCB) + (MB_ALIGN - 1)) &\
                                                           ~(MB_ALIGN - 1))
#define BCB_SIZE_UNSIGNED   (((sizeof(POSIX_BCB) + (MB_ALIGN - 1)) &\
                                       ~(MB_ALIGN - 1)) / sizeof(UNSIGNED))

#endif

/* SYSTEM TICK INFORMATION.  */
#define NANOSECONDS_PER_SECOND                 1000000000
#define POSIX_TICKS_PER_SECOND                 NU_PLUS_Ticks_Per_Second
#define POSIX_NANOSECONDS_PER_TICK             (NANOSECONDS_PER_SECOND / \
                                                    POSIX_TICKS_PER_SECOND)

/* Defines for the function return types.  */
#define POSIX_FOUND             1
#define POSIX_NOT_FOUND         0
#define POSIX_ERROR            -1
#define POSIX_SUCCESS           0
#define POSIX_FALSE            -1
#define POSIX_TRUE              0
#define POSIX_FILE_CLOSED      -1

/* POSIX Thread Garbage collector stack */
#if defined(C55X) && defined(MOT)
#define PSX_PTGC_STACK      500
#else
#define PSX_PTGC_STACK      2048
#endif /* C55X and MOT */

typedef struct cb_link
{
    INT    id;
    struct cb_link * prev_link;
    struct cb_link * next_link;

}CB_LINK;


typedef struct POSIX_SIGCB_STRUCT
{
    INT    id;                              /* Link    ID.  */
    struct cb_link *prev_link;
    struct cb_link *next_link;
    siginfo_t       info;                   /* Signal Info.  */
}POSIX_SIGCB;

#if (_POSIX_THREADS !=  -1)

typedef struct _posix_thread_id_struct
{
    NU_TASK         task;                   /* Nucleus Task.    */
    UINT16          tid;                    /* Thread ID.       */
}POSIX_THREAD_ID;

typedef struct  POSIX_TCB_STRUCT
{
    INT             id;                   /* ID of the Task.  */
    NU_TASK         *nu_tcb;              /* Nucleus Task Control Block pointer.  */
    NU_EVENT_GROUP  sig_group;              /* Used for the sigwaitinfo
                                               and sigsuspend etc.  */
    CB_LINK         signals_head;           /* Link List of signals for
                                               the task.  */
    pthread_attr_t  attr;                   /* Task Attribute's object. */
    _pthread_cleanup_t *cleanup_head;       /* Head of cleanup list used
                                               by cleanup push declaration
                                               present in pthread.h.  */
    pthread_mutex_t *mutex;                 /* Used by condition variable */
    VOID**          keys[PTHREAD_KEYS_MAX]; /* For the thread specific
                                               data. */
    VOID*       (*entry_function)(VOID *);  /* Saved Entry Function.  */
    VOID*           exit_status;            /* Thread exit status */
    INT             sched_policy;           /* Scheduling Policy   */
    INT             priority;               /* Priority is only the
                                               scheduling parameter for
                                               SCHED_FIFO and SCHED_RR. */
    INT             detach_state;           /* Task Detach State.  */
    INT             cancelation_state;      /* Task Cancellation State.  */
    INT             cancelation_requested;  /* Flag to check that whether
                                               the cancellation is
                                               requested.  */
    INT             cancebility_type;       /* Cancellation Type.  */
    INT             p_errno;                /* To Hold thread specific
                                               errno value. */
    VOID            *thd_stack_start;        /* start address of the stack */
}POSIX_TCB;

#endif

#if (_POSIX_THREADS !=  -1)

/* Condition variable ID */
#define         CD_CONDITION_ID         0x434F4E44UL
#define         NU_CONDITION_SUSPEND    20  /* 20 is selected to keep in
                                            view the future reservation
                                            of Nucleus PLUS */
#define         NU_POSIX_SIGNAL_STOP    21  /* Nucleus POSIX signal
                                            stop suspension ID */

typedef struct POSIX_CCB_STRUCT
{
    CS_NODE             cd_created;         /* Node for linking to    */
                                            /* created condition control
                                            block list */
    UNSIGNED            cd_id;              /* Internal CCB ID        */
    CHAR                cd_name[NU_MAX_NAME];/* Condition variable
                                            name */
    UNSIGNED            cd_tasks_waiting;   /* Number of waiting tasks*/
    struct CD_SUSPEND_STRUCT
                       *cd_suspension_list; /* Suspension list        */
} POSIX_CCB;


/* Define the semaphore suspension structure.  This structure is allocated
   off of the caller's stack.  */

typedef struct CD_SUSPEND_STRUCT
{
    CS_NODE             cd_suspend_link;    /* Link to suspend blocks */
    POSIX_CCB          *cd_condition;       /* Pointer to condition
                                            variable */
    TC_TCB             *cd_suspended_task;  /* Task suspended         */
    STATUS              cd_return_status;   /* Return status          */
} CD_SUSPEND;

#endif  /* (_POSIX_THREADS  !=  -1) */


#if (_POSIX_MESSAGE_PASSING !=  -1)

/* Message queue control block.  */
typedef struct POSIX_MCB_STRUCT
{
    CS_NODE         buflist;                /* Buffer list.  */
    CS_NODE         msglist;                /* Message list.  */
    NU_QUEUE        nu_bufq;                /* Buffer queue.  */
    NU_QUEUE        nu_msgq;                /* Message queue.  */
    struct sigevent notification;           /* Notification stuff.  */
    CHAR*           bufferpool;             /* Buffer pool.  */
    INT32           buf_count;              /* Remaining buffer count.  */
    INT32           msg_count;              /* Current message count.  */
    INT32           max_msgs;               /* Maximum messages which can
                                               be accommodated in the
                                               queue.  */
    INT32           size;                   /* Maximum message size.  */
    pthread_t       notified;               /* ID of the thread to be
                                               notified.  */
    INT             id;                     /* Message Queue ID.  */
    INT             opened;                 /* Open counter.  */
    INT             unlinked;               /* Unlink counter.  */
    CHAR            name[NAME_MAX + 1];     /* Queue name.  */
} POSIX_MCB;

/* Message queue descriptor control block.  */
typedef struct  POSIX_MQD_STRUCT
{
    INT32           oflag;                  /* Access flags.  */
    DATA_ELEMENT    padding[4];
    INT             id;                     /* ID.  */
    INT             mcb;                    /* Message queue index.  */
}POSIX_MQD;

/* Buffer control block.  */
typedef struct
{
    CS_NODE      link;                      /* Links.  */
    unsigned     prio;                      /* Priority.  */
    size_t       length;                    /* Length.  */
    CHAR         message[1];                /* The actual message.  */
}POSIX_BCB;

#endif

#if (_POSIX_THREADS !=  -1)

/* Thread local data.  */
typedef struct
{
    VOID (*destructor)(VOID *);
    INT created;
} POSIX_KEY_DATA;

#endif

#if (_POSIX_TIMERS  !=  -1)

typedef struct  POSIX_TIMERCB_STRUCT
{
    NU_TIMER            nu_timer;           /* Nucleus Timer Control
                                               Block.  */
    struct sigevent     sigevent;           /* Signal action at
                                               expiration.  */
    INT                 id;                 /* ID of the Timer.  */
    pthread_t           thread;             /* Client thread.  */
    INT                 armed;              /* Armed flag.  */
    INT                 overrun;            /* Overrun Count.  */
}POSIX_TIMERCB;

#endif

#if (_POSIX_SEMAPHORES  !=  -1)

/* Semaphore Control Block.  */
typedef struct  POSIX_SCB_STRUCT
{
    NU_SEMAPHORE    nu_sem;
    INT             id;
    INT             open;
    INT             unlink;
    CHAR            name[NAME_MAX + 1];
}POSIX_SCB;

#endif

extern NU_PROTECT   PSX_Protect;
#define PSX_PROTECT PSX_Protect

/* DDL has dependancy on Nucleus POSIX Delete Mailbox
and POSIX Task control blocks, which should be removed
within DDL product in the future */
#if(POSIX_INCLUDE_DDL)
extern  NU_MAILBOX Delete_Mailbox;
extern  POSIX_TCB  TC_Task_Cb_Tbl[];
#define PSX_TC_DELETE_MAILBOX               &Delete_Mailbox
#define PSX_TC_TASK_CB_TBL                  TC_Task_Cb_Tbl
#endif /* POSIX_INCLUDE_DDL */

/* Function Declarations.  */

#ifdef __cplusplus
extern "C" {
#endif

VOID        CbLinkInsert( CB_LINK *, CB_LINK *);
CB_LINK *   CbLinkRemove( CB_LINK *);
VOID        CbPriorityLinkInsert( CB_LINK **cb_prev_p, CB_LINK * cb_link_p);

extern VOID ERC_System_Error(INT error_code);

#ifdef __cplusplus
}
#endif

#endif  /*  __POSIX_H_  */

