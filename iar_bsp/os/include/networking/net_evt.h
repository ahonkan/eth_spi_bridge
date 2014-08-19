/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       net_evt.h
*
*   DESCRIPTION
*
*       Definitions for NET events.
*
*   DATA STRUCTURES
*
*       NET_IF_CHANGE_MSG
*       NET_IF_LINK_STATE_MSG
*       NET_WL_EVTDATA_ALL
*
*   DEPENDENCIES
*
*       mem_defs.h
*       net.h
*       mib2.h
*       wl_evt_types.h
*       dev_mgr.h
*       dev6.h
*       ip6_mib.h
*
*************************************************************************/

#ifndef NET_EVT_H
#define NET_EVT_H

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "networking/mem_defs.h"
#include "networking/net.h"
#include "networking/mib2.h"
#include "networking/wl_evt_types.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/dev6.h"
#include "networking/ip6_mib.h"
#endif

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/*
 * Device Discovery and Ethernet Link Status Events
 */

/* Net Dynamic Device Discovery Virtual Driver Device ID. */
extern DV_DEV_ID Net_Device_Discovery_Driver_ID;

/* Define a GUID for the Device Discovery Virtual Device. */
#define NET_DD_NOTIFICATIONS_LABEL      {0xBA,0x70,0xB7,0x84,0x6E,0x57,0x40,0x6f,0x84,0x9F,0x98,0x34,0x85,0x38,0xA0,0x1B}

#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_TRUE)
/* Power services Event types */
#define NET_NO_NOTIFICATIONS      0x00000000
#define NET_IF_CHANGE_ADD         0x00000001
#define NET_IF_CHANGE_DEL         0x00000002
#else
#define NET_IF_CHANGE_ADD         0x44
#define NET_IF_CHANGE_DEL         0x55
#define NET_IF_CHANGE_MASK        0xFFFFFFFF
#endif

/* For Power and Event Notification if the link status has changed */
#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_TRUE)
#define LINK_CHANGE_STATE         0x00000004
#else
#define LINK_CHANGE_STATE         0x2
#endif

/* EQM Instance. */
extern EQM_EVENT_QUEUE NET_Eqm;

/* Queue size is number of events that can be stored in the EQM. */
#define NET_EQM_QUEUE_SIZE        10

/* It defines the maximum size of event data of any event type. */
#define MAX_EVENT_DATA_SIZE sizeof(NET_IF_CHANGE_MSG)>sizeof(NET_IF_LINK_STATE_MSG)? sizeof(NET_IF_CHANGE_MSG): sizeof(NET_IF_LINK_STATE_MSG)

/* this structure is used to store device ID and device handle pairs for
   the notifications set from the device registration task to the device
   link-up / link-down task. */
typedef struct _NET_IF_CHANGE_MSG {
#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_TRUE)
    UINT32            event_type;
#endif
    DV_DEV_ID         dev_id;
    DV_DEV_HANDLE     dev_hd;
    CHAR              *dev_name;
#define LINK_DN      0
#define LINK_UP      1
    UINT8             link_state;
    UINT8             pad[1];
} NET_IF_CHANGE_MSG;

typedef struct _NET_IF_LINK_STATE {
#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_TRUE)
    UINT32            event_type;
    DV_DEV_ID         dev_id;
#endif
    CHAR              msg[12];
} NET_IF_LINK_STATE_MSG;

/*
 * WLAN Network Events
 */

#ifdef CFG_NU_OS_NET_WPA_SUPP_ENABLE
extern EQM_EVENT_QUEUE NET_WL_Event_Queue;
#endif

/* The maximum queue and data size of the WLAN events queue. */
#define NET_WL_QUEUE_SIZE                   10
#define NET_WL_MAX_EVENT_DATA_SIZE          sizeof(NET_WL_EVTDATA_ALL)

typedef struct _NET_WL_EVTDATA_ALL
{
    UINT32          wl_event_type;      /* Bit mask of event type. */
    DV_DEV_ID       wl_dev_id;          /* Device ID of event poster. */

    /* Union of all WLAN event data structures. */
    union {
        WL_EVTDATA_ASSOC_INFO           wl_assoc_info;
        WL_EVTDATA_MICHAEL_MIC_FAILURE  wl_michael_mic_failure;
        WL_EVTDATA_INTERFACE_STATUS     wl_interface_status;
        WL_EVTDATA_PMKID_CANDIDATE      wl_pmkid_candidate;
        WL_EVTDATA_STKSTART             wl_stkstart;
        WL_EVTDATA_FT_IES               wl_ft_ies;
        WL_EVTDATA_NODE_ADDR            wl_node_addr;
        WL_EVTDATA_GENIE                wl_genie;
    } wl_data;

} NET_WL_EVTDATA_ALL;


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NET_EVT_H */
