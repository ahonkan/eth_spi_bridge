/***********************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file contains private data structure definitions and  
*       constants of PMS watchdog services subcomponent.
*
***********************************************************************/
#ifndef PMS_WATCHDOG_H
#define PMS_WATCHDOG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Size of Array that contains newly created watchdog elements */
#define PM_WATCHDOG_ARRAY_SIZE      30

/* Unique 16 bit Watchdog Element Identifier */
#define PM_WATCHDOG_ID              0xA64D

/* Watchdog Main Task */
#define WD_TASK_STACK_SIZE          (NU_MIN_STACK_SIZE * 8)
#define WD_TASK_PRIORITY            10
#define WD_TASK_TIMESLICE           5

/* Maximum value of System Clock before rollover occurs */
#define PM_CLOCK_MAX                0xFFFFFFFE

/* Define event mask waking up the main task notification event. */
#define PM_WD_WAKEUP_MAIN_TASK       1

/* Initial value of latest activity timecheck if element created at 1st tick */
#define PM_WD_INACTIVITY_INIT_VAL    -1

/* Structure to link watchdogs */
typedef struct  PMS_NODE_STRUCT
{
    struct PMS_NODE_STRUCT  *previous;
    struct PMS_NODE_STRUCT  *next;
    UINT32                   priority;
}  PMS_NODE;

/* Watchdog element structure */
typedef struct PM_WD_STRUCT
{
    PMS_NODE     active_list;                /* Node for linking active Watchdog elements */
    UINT16       wd_id;
    UINT16       timeout;
    INT32        latest_activity_check;     /* value of last latest_activity_timestamp processed by main thread */
    UINT32       latest_activity_timestamp; /* system timer value of last watchdog reset */
    UINT32       next_inactivity_check;     /* Time at which main thread will process element next */
    BOOLEAN      expired_flag;
    INT          is_notifications;
    DV_DEV_ID    notification_dev;
    BOOLEAN      is_rollover;
    NU_SEMAPHORE *semaphore_ptr;
} PM_WD;


#ifdef __cplusplus
}
#endif

#endif /* #ifndef PMS_WATCHDOG_H */


