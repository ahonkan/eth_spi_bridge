/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       snmp_no.h                                                
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations used in the notification originator component.
*
*   DATA STRUCTURES
*
*       NOTIFICATION_OID
*       SNMP_NOTIFY_REQ_STRUCT
*       SNMP_NOTIFY_REQ_LIST
*       SNMP_NOTIFY_TABLE
*       SNMP_NOTIFY_TABLE_ROOT
*       SNMP_NOTIFY_FILTER_PROFILE_TABLE
*       SNMP_PROFILE_TABLE_ROOT
*       SNMP_NOTIFY_FILTER_TABLE
*       SNMP_FILTER_TABLE_ROOT
*
*   DEPENDENCIES
*
*       nu_net.h
*       fal.h
*       snmp_cfg.h
*       snmp_dis.h
*       vacm.h
*       tgr_mib.h
*       snmp_v3.h
*
************************************************************************/

#ifndef SNMP_NO_H
#define SNMP_NO_H

#include "networking/nu_net.h"

#include "networking/snmp_cfg.h"
#include "networking/snmp_dis.h"
#include "networking/vacm.h"
#include "networking/tgr_mib.h"

#if ( INCLUDE_SNMPv3)
#include "networking/snmp_v3.h"
#endif

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#define MAX_NOTIFY_NAME_SZE         32
#define MAX_FILTER_PROF_NAME_SZE    32
#define MAX_FILTER_MASK_SZE         16

/* Defines for notify type */
#define TRAP                        1
#define INFORM                      2

/* Defines for filter type */
#define INCLUDED                    1
#define EXCLUDED                    2

/* SNMP Trap OIDs. */
#define SNMP_TRAP_OID_LEN                   10
#define SNMP_COLD_START_TRAP                {1, 3, 6, 1, 6, 3, 1, 1, 5, 1}
#define SNMP_WARM_START_TRAP                {1, 3, 6, 1, 6, 3, 1, 1, 5, 2}
#define SNMP_LINK_DOWN_TRAP                 {1, 3, 6, 1, 6, 3, 1, 1, 5, 3}
#define SNMP_LINK_UP_TRAP                   {1, 3, 6, 1, 6, 3, 1, 1, 5, 4}
#define SNMP_AUTH_FAILURE_TRAP              {1, 3, 6, 1, 6, 3, 1, 1, 5, 5}
#define SNMP_EGP_NEIGHBORLOSS_TRAP          {1, 3, 6, 1, 6, 3, 1, 1, 5, 6}
#define SNMP_ENTSPECIFIC_TRAP               {1, 3, 6, 1, 6, 3, 1, 1, 4, 3}

/* General Traps */
#define SNMP_TRAP_COLDSTART                 0
#define SNMP_TRAP_WARMSTART                 1
#define SNMP_TRAP_LINKDOWN                  2
#define SNMP_TRAP_LINKUP                    3
#define SNMP_TRAP_AUTFAILURE                4
#define SNMP_TRAP_EQPNEIGHBORLOSS           5
#define SNMP_TRAP_ENTSPECIFIC               6

#define SNMP_TOTAL_STANDARD_TRAPS           6

typedef struct snmp_notification_oid
{
    UINT32    notification_oid[SNMP_SIZE_OBJECTID];
    UINT32    oid_len;

} NOTIFICATION_OID;

typedef struct snmp_notify_req_struct
{
    /* The following two pointers aid in maintenance of a list
     * of requests.
     */
    struct snmp_notify_req_struct *snmp_flink;
    struct snmp_notify_req_struct *snmp_blink;

    UINT32                transport_domain;
    UINT32                timeout;       /* only used when expect_response
                                          * = INFORM
                                          */
    INT32                 retry_count;   /* only used when
                                          * expect_response = INFORM
                                          */
    UINT32                snmp_object_list_len;
    NOTIFICATION_OID      OID;
    snmp_object_t         snmp_object_list[SNMP_MAX_NOTIFICATION_OBJECTS];
    UINT8                 transport_address[SNMP_MAX_IP_ADDRS];
    INT8                  expect_response;

    /* Make the structure word-aligned. */
    UINT8                  pad[3];
}SNMP_NOTIFY_REQ_STRUCT;

/* Doubly-linked list for notification requests. */
typedef struct snmp_notify_req_list
{
    SNMP_NOTIFY_REQ_STRUCT  *snmp_flink;
    SNMP_NOTIFY_REQ_STRUCT  *snmp_blink;

}SNMP_NOTIFY_REQ_LIST;

typedef struct snmp_notify_table
{
    struct snmp_notify_table *flink;
    struct snmp_notify_table *blink;

    UINT32                    tag_len;
    INT32                     snmp_notify_type;    /* confirmed or
                                                    * unconfirmed class
                                                    * PDUs.
                                                    */
#if (INCLUDE_MIB_NO == NU_TRUE)
    UINT32                    snmp_notify_storage_type;
    UINT32                    snmp_notify_row_status;
#endif
    /* null terminated */
    CHAR                      snmp_notify_name[MAX_NOTIFY_NAME_SZE];
    UINT8                     snmp_notify_tag[SNMP_SIZE_BUFCHR];
}SNMP_NOTIFY_TABLE;

typedef struct snmp_notify_table_root
{
    struct snmp_notify_table *flink;
    struct snmp_notify_table *blink;

}SNMP_NOTIFY_TABLE_ROOT;


typedef struct snmp_notify_filter_profile_table
{
    struct snmp_notify_filter_profile_table *flink;
    struct snmp_notify_filter_profile_table *blink;
#if (INCLUDE_MIB_NO == NU_TRUE)
    INT32       snmp_notify_filter_profile_storType;
    INT32       snmp_notify_filter_profile_row_status;
    UINT8       snmp_notify_filter_profile_row_flag;
    UINT8       snmp_pad[3];
#endif
    /* null terminated */
    CHAR        snmp_target_params_name[MAX_FILTER_PROF_NAME_SZE];
    /* null terminated */
    CHAR        snmp_notify_filter_profile_name[MAX_FILTER_PROF_NAME_SZE];

}SNMP_NOTIFY_FILTER_PROFILE_TABLE;

typedef struct snmp_profile_table_root
{
    struct snmp_notify_filter_profile_table *flink;
    struct snmp_notify_filter_profile_table *blink;

}SNMP_PROFILE_TABLE_ROOT;

typedef struct snmp_notify_filter_table
{
    struct snmp_notify_filter_table  *flink;
    struct snmp_notify_filter_table  *blink;

    UINT32      snmp_notify_filter_subtree[SNMP_SIZE_OBJECTID];
    UINT32      subtree_len;
    UINT32      mask_len;    /* no. of octets */
    INT32       snmp_notify_filter_type;

#if (INCLUDE_MIB_NO == NU_TRUE)
    UINT32      snmp_notify_filter_storage_type;
    UINT32      snmp_notify_filter_row_status;
#endif
    /* null terminated. For internal use. */
    CHAR        snmp_notify_filter_profile_name[MAX_FILTER_PROF_NAME_SZE];
    UINT8       snmp_notify_filter_mask[MAX_FILTER_MASK_SZE];

}SNMP_NOTIFY_FILTER_TABLE;

typedef struct snmp_filter_table_root
{
    struct snmp_notify_filter_table  *flink;
    struct snmp_notify_filter_table  *blink;

}SNMP_FILTER_TABLE_ROOT;

/* Function prototypes. */
STATUS Notification_Init(VOID);
VOID   SNMP_Notification_Task_Entry(UNSIGNED argc, VOID *argv);
STATUS SNMP_Prepare_Notification(SNMP_SESSION_STRUCT *notification,
                                 UINT8 *snmp_notification_buffer);
UINT8  SNMP_Compare_Tag(UINT8 *tag, UINT32 tag_len, const UINT8 *tag_list,
                        UINT32 tag_list_len);
SNMP_TARGET_PARAMS_TABLE *SNMP_Find_Target_Params(
                        const CHAR *params_name,
                        SNMP_TARGET_PARAMS_TABLE *params_tbl);
STATUS SNMP_Get_Notification_Ptr(SNMP_NOTIFY_REQ_STRUCT **notification);
STATUS SNMP_Retrieve_Notification (SNMP_SESSION_STRUCT* notification);
STATUS SNMP_Notification_Ready(SNMP_NOTIFY_REQ_STRUCT *notification);
STATUS SNMP_Prepare_Object_List(SNMP_SESSION_STRUCT *notification);
STATUS SNMP_Notification_Config(VOID);
STATUS SNMP_Add_To_Notify_Tbl(SNMP_NOTIFY_TABLE *node);
STATUS SNMP_Add_To_Profile_Tbl(SNMP_NOTIFY_FILTER_PROFILE_TABLE *node);
STATUS SNMP_Add_To_Filter_Tbl(SNMP_NOTIFY_FILTER_TABLE *node);
STATUS SNMP_Add_Target(const SNMP_TARGET_ADDRESS_TABLE *node);
STATUS SNMP_Add_Params(const SNMP_TARGET_PARAMS_TABLE *node);
STATUS SNMP_Subtree_Compare(const UINT32 *subtree, UINT32 subtree_len,
                            const UINT32 *view_tree, UINT32 view_tree_len,
                            const UINT8 *view_mask);
STATUS SNMP_Notification_Filtering(SNMP_SESSION_STRUCT *notification,
                                   const CHAR *filter_name);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif

