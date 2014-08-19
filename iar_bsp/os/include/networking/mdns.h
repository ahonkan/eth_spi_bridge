/*************************************************************************
*
*              Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/**************************************************************************
*
* FILENAME
*
*       mdns.h
*
* DESCRIPTION
*
*       This include file will handle multicast domain processing defines.
*
* DATA STRUCTURES
*
*
* DEPENDENCIES
*
*      No other file dependencies
*
***************************************************************************/

#ifndef MDNS_H
#define MDNS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define MDNS_PORT   5353

/* Events for mDNS. */
#define MDNS_REGISTER_DEVICE    0
#define MDNS_PROCESS_RECORD     1
#define MDNS_PROCESS_QUERY      2
#define MDNS_PROCESS_RESPONSES  3

#define MDNS_EVENT_Q_ELEMENT_SIZE   3
#define MDNS_TIME_SLICE             0

/* mDNS record states. */
#define MDNS_PROBING        0
#define MDNS_ANNOUNCING     1
#define MDNS_ASSIGNED       2
#define MDNS_CONFLICTED     3
#define MDNS_TIE_BREAKER    4
#define MDNS_QUERYING       5
#define MDNS_UNINITIALIZED  255 /* The record has no state. */

#define MDNS_PROBE_DELAY            (TICKS_PER_SECOND >> 2) /* 250 milliseconds. */
#define MDNS_ONE_SEC_DELAY          (TICKS_PER_SECOND * 1)  /* 1 second. */
#define MDNS_INIT_QUERY_MIN_DELAY   (TICKS_PER_SECOND / 50) /* 20 ms. */
#define MDNS_INIT_QUERY_MAX_DELAY   (TICKS_PER_SECOND / 10) /* 100 ms. */
#define MDNS_QUERY_MAX              (TICKS_PER_SECOND * 60 * 60) /* 60 minutes */
#define MDNS_RESPONSE_DELAY         (TICKS_PER_SECOND / 2) /* 500 ms. */
#define MDNS_FOREIGN_RESPONSE_MAX   (TICKS_PER_SECOND * 10) /* 10 seconds. */

typedef struct _MDNS_Sort_Index
{
    UINT16  r_class;
    UINT16  type;
    CHAR    *data_ptr;
    INT16   data_len;
    UINT8   pad[2];
} MDNS_SORT_INDEX;

typedef struct _MDNS_RESPONSE
{
    struct _MDNS_RESPONSE   *mdns_next;
    struct _MDNS_RESPONSE   *mdns_previous;
    DNS_HOST                *mdns_host;
} MDNS_RESPONSE;

/* Define the head of the linked list of responses. */
typedef struct _MDNS_RESPONSE_LIST
{
    MDNS_RESPONSE    *dns_head;
    MDNS_RESPONSE    *dns_tail;
} MDNS_RESPONSE_LIST;

typedef struct _MDNS_NTFY_STRUCT
{
    struct _MDNS_NTFY_STRUCT    *next;
    struct _MDNS_NTFY_STRUCT    *previous;
    NU_TASK     *callback_ptr;
}MDNS_NTFY_STRUCT;

typedef struct _MDNS_NTFY_LIST
{
    MDNS_NTFY_STRUCT    *head;
    MDNS_NTFY_STRUCT    *tail;
} MDNS_NTFY_LIST;

typedef struct _MDNS_QUERY
{
    struct _MDNS_QUERY  *mdns_next;
    struct _MDNS_QUERY  *mdns_previous;
    MDNS_RESPONSE_LIST  mdns_host_list;
    MDNS_NTFY_LIST      mdns_callback;
    UINT32              mdns_id;
    INT16               mdns_type;
    INT16               mdns_family;
    NU_TIMER            mdns_qry_timer;
    UINT32              mdns_qry_expire;
    CHAR                *mdns_data;
} MDNS_QUERY;

typedef struct _MDNS_QUERY_LIST
{
    MDNS_QUERY    *head;
    MDNS_QUERY    *tail;
} MDNS_QUERY_LIST;

STATUS      MDNS_Register_Local_Host(DV_DEVICE_ENTRY *device, UINT8 *ip_addr, INT16 family);
VOID        MDNS_Remove_Local_Host(UINT8 *ip_addr, INT16 family);
STATUS      MDNS_Initialize(VOID);
VOID        MDNS_Master_Task(UNSIGNED argc, VOID *argv);
CHAR        *MDNS_Initialize_Hostname(UINT32 index);
VOID        MDNS_Invoke_Probing(DNS_HOST *dns_ptr, UINT32 delay);
STATUS      NU_Start_mDNS_Query(CHAR *data, INT type, INT16 family, VOID *ptr);
STATUS      NU_Stop_mDNS_Query(CHAR *data, INT type, INT16 family, VOID *ptr);
VOID        MDNS_Event_Handler(UNSIGNED id);
MDNS_QUERY  *MDNS_Add_Query(CHAR *data, INT type, INT16 family);
VOID        MDNS_Query_Handler(UNSIGNED dns_id);
MDNS_QUERY  *MDNS_Find_Matching_Query_By_Data(CHAR *data, INT type, INT16 family);
VOID        MDNS_Send_Response(DNS_HOST *dns_ptr, struct addr_struct *addr);
VOID        MDNS_Process_Unicast_Query(DNS_HOST *dns_ptr, struct addr_struct *addr);
VOID        MDNS_Response_Handler(UNSIGNED dns_id);
STATUS      MDNS_Perform_Registration(DV_DEVICE_ENTRY *device, UINT8 *ip_addr,
                                      INT16 family);
STATUS      MDNS_Set_Device_Hostname(CHAR *name, UINT32 dev_index);
STATUS      NU_Set_MDNS_Hostname_Callback(CHAR* (set_hostname(UINT32 int_index,
                                          BOOLEAN conflict)));

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* DNS_H */
