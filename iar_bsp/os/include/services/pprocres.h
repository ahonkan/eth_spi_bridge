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
*       pprocres.h
*
* COMPONENT
*
*       RES - POSIX Resource
*
* DESCRIPTION
*
*       This file contains data structure definitions and constants and
*       pointer mapping for the pthread resource component.
*
* DATA STRUCTURES
*
*       POSIX_THD_RES_TABLE
*       POSIX_THD_STACK_STATUS
*       PPROC_THREADS_RES
*       POSIX_SIGNAL_INTR
*       PPROC_SIGNALS_RES
*       POSIX_TMR_RES_TABLE
*       PPROC_TIMERS_RES
*       POSIX_SM_RES_TABLE
*       PPROC_SEMAPHORES_RES
*       POSIX_MQ_RES_TABLE
*       PPROC_MQS_RES
*       PPROC_CONDS_RES
*       PPROC_THREAD_RES
*
* DEPENDENCIES
*
*       None
*
************************************************************************/
#ifndef __PPROCRES_H_
#define __PPROCRES_H_

#if (_POSIX_THREADS !=  -1)

#include "os/include/nucleus.h"

/* Thread resource table translation to resource id. */
typedef struct _posix_thd_res_table_struct
{
    UINT16          rid;                /* ADF rid */
    UINT16          padding;
}
POSIX_THD_RES_TABLE;

/* Stack Status Type - Data type that indicates a check stack
   status. */

typedef enum _posix_thd_stack_status_enum
{
    /* None - Value used internally. */

    POSIX_THD_STACK_STATUS_NONE,

    /* Normal - Value used to indicate a normal status. */

    POSIX_THD_STACK_STATUS_NORMAL,

    /* Overflow - Value used to indicate a stack overflow status. */

    POSIX_THD_STACK_STATUS_OVERFLOW

} POSIX_THD_STACK_STATUS;

/* Thread resource management for one process */
typedef struct _pproc_threads_res_struct
{
    /* The following are static when DDL product is included */
#if(POSIX_INCLUDE_DDL == 0)
    POSIX_TCB               *psx_tc_task_cb_tbl;
    NU_MAILBOX              *psx_tc_delete_mailbox;
#endif /* POSIX_INCLUDE_DDL */
    POSIX_TCB               *psx_tc_initial_tcb;
    CB_LINK                 *psx_tc_task_cb_head;
    UINT16                  psx_tc_mutex_id;
    UINT16                  psx_tc_cond_id;
    POSIX_KEY_DATA          *psx_tc_thread_keys;
    POSIX_THD_RES_TABLE     *psx_tc_res_tbl;
    INT                     *psx_tc_um_errno_tbl;
    POSIX_THD_STACK_STATUS  *psx_tc_stack_status_tbl;
}PPROC_THREADS_RES;

#if (_POSIX_REALTIME_SIGNALS != -1)

/* Signal interrupt data structure */
typedef struct _posix_signal_intr_struct
{
    UINT16  sig_no;             /* Signal number */
    BOOLEAN sig_active;         /* NU_TRUE-signal active,
                                   NU_FALSE-Signal deactivated
                                */
    UINT8   padding;            /* 1-byte Padding */
}POSIX_SIGNAL_INTR;

/* Signals resource management for one process */
typedef struct _pproc_signals_res_struct
{
    struct sigaction    *psx_sc_sigaction_table;
    CB_LINK             *psx_sc_scb_tbl_head;
    sigset_t            psx_sc_signonmaskable;
    POSIX_SIGNAL_INTR   *psx_sc_eintr_sig;
    NU_EVENT_GROUP      *psx_sc_thd_event_join_ptr;
}PPROC_SIGNALS_RES;
#endif /* (_POSIX_REALTIME_SIGNALS != -1) */

#if (_POSIX_TIMERS  !=  -1)
/* Timer resource table translation to resource id. */
typedef struct _posix_tmr_res_table_struct
{
    UINT16          rid;                /* ADF resource ID */
    UINT16          padding;
}
POSIX_TMR_RES_TABLE;

/* Timer resource management for one process */
typedef struct _pproc_timers_res_struct
{
    POSIX_TIMERCB       *psx_tic_tmr_cb_tbl;
    CB_LINK             *psx_tic_tmr_cb_head;
    POSIX_TMR_RES_TABLE *psx_tic_tmr_res_tbl;
    POSIX_TIMERCB        psx_tic_tmr_alrm;
}PPROC_TIMERS_RES;
#endif /* (_POSIX_TIMERS  !=  -1) */

#if (_POSIX_SEMAPHORES  !=  -1)
/* Semaphore resource table translation to resource id. */
typedef struct _posix_sem_res_table_struct
{
    UINT16          rid;                /* ADF resource ID */
    UINT16          padding;
}
POSIX_SM_RES_TABLE;

/* Semaphore resource management for one process */
typedef struct _pproc_semaphores_res_struct
{
    sem_t               *psx_sm_um_sem_id_tbl;
    POSIX_SCB           *psx_sm_sem_cb_tbl;
    CB_LINK             *psx_sm_sem_cb_head;
    POSIX_SM_RES_TABLE  *psx_sm_res_tbl;
}PPROC_SEMAPHORES_RES;
#endif /* (_POSIX_SEMAPHORES  !=  -1) */

#if (_POSIX_MESSAGE_PASSING !=  -1)
/* Message Queue resource table translation to resource id. */
typedef struct _posix_mq_res_table_struct
{
    UINT16          rid;                /* ADF resource ID */
    UINT16          padding;
}
POSIX_MQ_RES_TABLE;

/* MQ resource management for one process */
typedef struct _pproc_mqs_res_struct
{
    POSIX_MCB           *psx_mq_mcb_cb_tbl;
    CB_LINK             *psx_mq_mcb_cb_head;
    POSIX_MQD           *psx_mq_mqd_cb_tbl;
    CB_LINK             *psx_mq_mqd_cb_head;
    POSIX_MQ_RES_TABLE  *psx_mq_res_tbl;
}PPROC_MQS_RES;
#endif /* (_POSIX_MESSAGE_PASSING !=  -1) */

/* Condition Variables resource management for one process */
typedef struct _pproc_cond_res_struct
{
    UNSIGNED                psx_ccd_total_condvars;
    CS_NODE                 *psx_ccd_created_condvars_list;
}PPROC_CONDS_RES;

#endif /* (_POSIX_THREADS !=  -1) */
/* The entire resource management for one process */
typedef struct _PPROC_THREAD_RES_struct
{
#if (_POSIX_THREADS !=  -1)
    PPROC_THREADS_RES       thd_res;
#if (_POSIX_REALTIME_SIGNALS != -1)
    PPROC_SIGNALS_RES       sig_res;
#endif /* (_POSIX_REALTIME_SIGNALS != -1) */
#if (_POSIX_TIMERS  !=  -1)
    PPROC_TIMERS_RES        time_res;
#endif /* (_POSIX_TIMERS  !=  -1) */
#if (_POSIX_SEMAPHORES  !=  -1)
    PPROC_SEMAPHORES_RES    sem_res;
#endif /* (_POSIX_SEMAPHORES  !=  -1) */
#if (_POSIX_MESSAGE_PASSING !=  -1)
    PPROC_MQS_RES           mq_res;
#endif /* (_POSIX_MESSAGE_PASSING !=  -1) */
    PPROC_CONDS_RES         cond_res;
#endif /* (_POSIX_THREADS !=  -1) */
}PPROC_THREAD_RES;


/* The following are all pointer mappings of all the resources defined above */
#if (_POSIX_THREADS !=  -1)
/* Threads */
#define PSX_TC_MUTEX_ID                     psx_res->thd_res.psx_tc_mutex_id
#define PSX_TC_COND_ID                      psx_res->thd_res.psx_tc_cond_id
#define PSX_TC_THREAD_KEYS                  psx_res->thd_res.psx_tc_thread_keys
#define PSX_TC_TASK_CB_HEAD                 psx_res->thd_res.psx_tc_task_cb_head
/* DDL has dependancy on Nucleus POSIX Delete Mailbox and POSIX Task control
blocks, which should be removed within DDL product in the future */
#if(POSIX_INCLUDE_DDL == 0)
#define PSX_TC_TASK_CB_TBL                  psx_res->thd_res.psx_tc_task_cb_tbl
#define PSX_TC_DELETE_MAILBOX               psx_res->thd_res.psx_tc_delete_mailbox
#endif /* POSIX_INCLUDE_DDL */
#define PSX_TC_RES_TBL                      psx_res->thd_res.psx_tc_res_tbl
#define PSX_TC_INITIAL_TCB                  psx_res->thd_res.psx_tc_initial_tcb
#define PSX_TC_UM_ERRNO_TBL                 psx_res->thd_res.psx_tc_um_errno_tbl
#define PSX_TC_STACK_STATUS_TBL             psx_res->thd_res.psx_tc_stack_status_tbl

#if (_POSIX_REALTIME_SIGNALS != -1)
/* Signals */
#define PSX_SC_SIGACTION_TABLE              psx_res->sig_res.psx_sc_sigaction_table
#define PSX_SC_SCB_TBL_HEAD                 psx_res->sig_res.psx_sc_scb_tbl_head
#define PSX_SC_EINTR_SIG                    psx_res->sig_res.psx_sc_eintr_sig
#define PSX_SC_SIGNONMASKABLE               psx_res->sig_res.psx_sc_signonmaskable
#define PSX_SC_THD_EVENT_JOIN_PTR           psx_res->sig_res.psx_sc_thd_event_join_ptr
#endif /* (_POSIX_REALTIME_SIGNALS != -1) */

#if (_POSIX_TIMERS  !=  -1)
/* Timer */
#define PSX_TIC_TMR_CB_HEAD                 psx_res->time_res.psx_tic_tmr_cb_head
#define PSX_TIC_TMR_CB_TBL                  psx_res->time_res.psx_tic_tmr_cb_tbl
#define PSX_TIC_TMR_RES_TBL                 psx_res->time_res.psx_tic_tmr_res_tbl
#define PSX_TIC_TMR_ALARM                   psx_res->time_res.psx_tic_tmr_alrm
#endif /* (_POSIX_TIMERS  !=  -1) */

#if (_POSIX_SEMAPHORES  !=  -1)
/* Semaphore */
#define PSX_SM_SEM_CB_HEAD                  psx_res->sem_res.psx_sm_sem_cb_head
#define PSX_SM_SEM_CB_TBL                   psx_res->sem_res.psx_sm_sem_cb_tbl
#define PSX_SM_RES_TBL                      psx_res->sem_res.psx_sm_res_tbl
#define PSX_SM_UM_SEM_ID_TBL                psx_res->sem_res.psx_sm_um_sem_id_tbl
#endif /* (_POSIX_SEMAPHORES  !=  -1) */

#if (_POSIX_MESSAGE_PASSING !=  -1)
/* MQ */
#define PSX_MQ_MCB_CB_HEAD                  psx_res->mq_res.psx_mq_mcb_cb_head
#define PSX_MQ_MCB_CB_TBL                   psx_res->mq_res.psx_mq_mcb_cb_tbl
#define PSX_MQ_MQD_CB_HEAD                  psx_res->mq_res.psx_mq_mqd_cb_head
#define PSX_MQ_MQD_CB_TBL                   psx_res->mq_res.psx_mq_mqd_cb_tbl
#define PSX_MQ_RES_TBL                      psx_res->mq_res.psx_mq_res_tbl
#endif /* (_POSIX_MESSAGE_PASSING !=  -1) */

/* Conditional Variables */
#define PSX_CCD_TOTAL_CONDVARS              psx_res->cond_res.psx_ccd_total_condvars
#define PSX_CCD_CREATED_CONDVARS_LIST       psx_res->cond_res.psx_ccd_created_condvars_list
#endif /* (_POSIX_THREADS !=  -1) */

/* Thread resources */
extern VOID * psx_static_thd_res;

#define PPROC_PTGC_RES_ID                   0
#define PTGC_RESOURCE_NUMBER                0

#ifdef __cplusplus
extern "C" {
#endif

INT                                         FindTid(pid_t pid, NU_TASK* nu_task_p);
VOID                                        POSIX_SYS_TC_Thread_Detach(PPROC_THREAD_RES *psx_res,
                                                                   POSIX_TCB *tcb, pthread_t tid);


#ifdef __cplusplus
}
#endif

#endif /* __PPROC_THREAD_RES_H_ */
