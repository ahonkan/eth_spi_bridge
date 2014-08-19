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
*       ike2_evt.h
*
* COMPONENT
*
*       IKEv2 - Events
*
* DESCRIPTION
*
*       This file contains macros and function prototypes to handle
*       IKEv2 events.
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       ike_evt.h
*
*************************************************************************/

#ifndef IKE2_EVT_H
#define IKE2_EVT_H

#include "networking/ike_evt.h"

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

#define IKE2_SA_ERROR_EVENT     IKE_PHASE1_ERROR_EVENT
#define IKE2_SA_REMOVE_EVENT    IKE_SA_REMOVE_EVENT

/* Synchronized removal of IKE2 SA/Handle using IKE events.
 * These macros directly post an event into the IKE event list.
 */
#define IKE2_SYNC_REMOVE_SA(sadb, sa)                                    \
           IKE_EQ_Handler((sa->ike_state != IKE2_SA_ESTABLISHED) ?       \
               IKE2_SA_ERROR_EVENT : IKE2_SA_REMOVE_EVENT ,              \
               (UNSIGNED)(sa), (UNSIGNED)(sadb))

/* Event ID of each type of event expected by IKE2 */
extern TQ_EVENT IKE2_Message_Reply_Event;
extern TQ_EVENT IKE2_SA_Timeout_Event;
extern TQ_EVENT IKE2_INIT_AUTH_Timeout;
extern TQ_EVENT IKE2_DPD_Timeout_Event;
extern TQ_EVENT IKE2_Cookie_Secret_Timeout_Event;

/* Event handler functions. */
VOID IKE2_Message_Reply_Handler(IKE2_SA *sa, IKE2_EXCHANGE_HANDLE *handle);
VOID IKE2_SA_Timeout_Handler(TQ_EVENT event, IKE2_SA *sa,
                             IKE2_SADB *sadb);
VOID IKE2_DPD_Timeout_Handler(TQ_EVENT event, IKE2_SA *sa, IKE2_SADB *sadb);
VOID IKE2_Cookie_Secret_Timeout_Handler(TQ_EVENT event);

VOID IKE2_SA_Remove_Event(TQ_EVENT event, IKE2_EXCHANGE_HANDLE *handle,
                          IKE2_SADB *sadb);
VOID IKE2_Exchange_Timeout_Event(TQ_EVENT event, IKE2_SA *sa,
                                 IKE2_EXCHANGE_HANDLE *handle);

STATUS IKE2_Unset_Matching_Timers(UNSIGNED dat, UNSIGNED ext_dat,
                                  INT16 match_type);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE2_EVT_H */
