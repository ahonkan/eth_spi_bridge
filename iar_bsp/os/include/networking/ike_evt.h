/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ike_evt.h
*
* COMPONENT
*
*       IKE - Events
*
* DESCRIPTION
*
*       This file contains constants, data structures and function
*       prototypes needed to implement IKE Events component.
*
* DATA STRUCTURES
*
*       IKE_EVENT
*       IKE_EVENT_DB
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IKE_EVT_H
#define IKE_EVT_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** Event constants. ****/

/* Synchronized removal of IKE SA/Handle using IKE events.
 * These macros directly post an event into the IKE event list.
 */
#define IKE_SYNC_REMOVE_SA(sadb, sa)                                    \
            IKE_EQ_Handler((sa->ike_state != IKE_SA_ESTABLISHED) ?      \
                IKE_PHASE1_ERROR_EVENT : IKE_SA_REMOVE_EVENT,           \
                (UNSIGNED)(sa), (UNSIGNED)(sadb))
#define IKE_SYNC_REMOVE_PHASE2(sa, ph2)                                 \
            IKE_EQ_Handler(IKE_PHASE2_ERROR_EVENT, (UNSIGNED)(sa),      \
                (UNSIGNED)(ph2))

/* Macro for initiating an IKE exchange. This is used by IPsec
 * because it cannot make a direct call to the IKE initiate
 * function due to ordering restrictions on semaphore obtains.
 * This macro directly posts an event into the IKE event list.
 */
#define IKE_SYNC_INITIATE(initiate_req)                                 \
            IKE_EQ_Handler(IKE_INITIATE_REQ_EVENT, (UNSIGNED)0,         \
                (UNSIGNED)(initiate_req))

/* Macro for sending an IPsec SA delete notification. This is
 * used by IPsec because it cannot make a direct call to the
 * IKE send delete notification function due to ordering
 * restrictions on semaphore obtains. This macro directly posts
 * an event into the IKE event list.
 */
#define IKE_SYNC_DELETE_NOTIFY(initiate_req)                            \
            IKE_EQ_Handler(IKE_DELETE_NOTIFY_EVENT, (UNSIGNED)0,        \
                (UNSIGNED)(initiate_req))

/* IKE internal event IDs. These are independent of the event
 * IDs registered with the Nucleus NET TQ component.
 *
 * WARNING: These values MUST be below EQ_FIRST_REG_EVENT to avoid
 * conflict with the TQ component registered event IDs:
 *    - IKE_Message_Reply_Event
 *    - IKE_SA_Timeout_Event
 *    - IKE_Phase1_Timeout_Event
 *    - IKE_Phase2_Timeout_Event
 *
 * The above Phase 1/2 timeouts and the below Phase 1/2 errors are
 * handled similarly. And the above SA timeout event is handled
 * similar to the below SA remove event, with the exception that
 * the SA delete notification is only sent on SA timeout.
 */
#define IKE_PHASE1_ERROR_EVENT          1
#define IKE_PHASE2_ERROR_EVENT          2
#define IKE_SA_REMOVE_EVENT             3
#define IKE_INITIATE_REQ_EVENT          4
#define IKE_DELETE_NOTIFY_EVENT         5

/**** Data structures. ****/

/* This structure is used to store an IKE event.
 *
 * WARNING: Fields of this structure must not be
 * re-ordered since this order is expected by the
 * NET SLL and TQ components.
 */
typedef struct ike_event
{
    struct ike_event *ike_flink;
    TQ_EVENT        ike_id;                 /* Event identifier. */
    UNSIGNED        ike_dat;                /* Arbitrary parameter. */
    UNSIGNED        ike_ext_dat;            /* Arbitrary parameter. */
} IKE_EVENT;

/* This structure is used to maintain a list of IKE events. */
typedef struct ike_event_db
{
    IKE_EVENT       *ike_flink;
    IKE_EVENT       *ike_last;
} IKE_EVENT_DB;

/**** Global variables. ****/

/* Define the IKE event IDs. */
extern TQ_EVENT IKE_Message_Reply_Event;
extern TQ_EVENT IKE_SA_Timeout_Event;
extern TQ_EVENT IKE_Phase1_Timeout_Event;
extern TQ_EVENT IKE_Phase2_Timeout_Event;

/**** Function prototypes. ****/

STATUS IKE_Initialize_Events(VOID);
STATUS IKE_Deinitialize_Events(VOID);
VOID IKE_EQ_Handler(TQ_EVENT event, UNSIGNED dat, UNSIGNED ext_dat);
VOID IKE_Event_Handler(UNSIGNED argc, VOID *argv);
VOID IKE_Unset_Single_Event(TQ_EVENT event_id, UNSIGNED dat,
                            UNSIGNED ext_dat, INT16 match_type);
VOID IKE_Unset_Matching_Events(UNSIGNED dat, UNSIGNED ext_dat,
                               INT16 match_type);
STATUS IKE_Set_Timer(TQ_EVENT event, UNSIGNED dat, UNSIGNED ext_dat,
                     UNSIGNED how_long);
STATUS IKE_Unset_Timer(TQ_EVENT event, UNSIGNED dat, UNSIGNED ext_dat);
STATUS IKE_Unset_Matching_Timers(UNSIGNED dat, UNSIGNED ext_dat,
                                 INT16 match_type);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_EVT_H */
