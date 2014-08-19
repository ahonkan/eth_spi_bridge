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
* FILENAME
*
*       multi.h
*
* DESCRIPTION
*
*       This include file will handle defines relating to the IP layer.
*
* DATA STRUCTURES
*
*       _MULTI_DATA
*       _MULTI_SCK_IGMP_LIST
*       _MULTI_SCK_MLD_LIST
*       _MULTI_SCK_OPTIONS
*       _MULTI_DEV_STATE
*       _MULTI_SCK_STATE
*       _MULTI_IP
*
* DEPENDENCIES
*
*       ip.h
*
*************************************************************************/
#ifndef MULTI_H
#define MULTI_H

#include "networking/ip.h"
#include "networking/netevent.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


#define MULTICAST_MESSAGE_TYPE_OFFSET   0

#define MULTI_RANDOM_DELAY(X) (UTL_Rand() % ((X) + 1))

#define MULTICAST_TYPE_IGMP         1
#define MULTICAST_TYPE_MLD          2

/* Multicast States */
#define NON_LISTENER                0
#define IDLE_LISTENER               1
#define DELAYING_LISTENER           2

/* Multicast Filter State  (IGMPv3/MLDv2) */
#define MULTICAST_FILTER_INCLUDE    1
#define MULTICAST_FILTER_EXCLUDE    2

/* Multicast message types */
#define MULTICAST_GENERAL_QUERY                     1
#define MULTICAST_ADDRESS_SPECIFIC_QUERY            2
#define MULTICAST_ADDRESS_AND_SOURCE_SPECIFIC_QUERY 3

/* Multicast Interoperability */
#define MULTICAST_MLDV1_QUERY_LENGTH        24
#define MULTICAST_MLDV2_QUERY_LENGTH        28
#define MULTICAST_IGMPV1_QUERY_LENGTH       8
#define MULTICAST_IGMPV2_QUERY_LENGTH       8
#define MULTICAST_IGMPV3_QUERY_LENGTH       12

/* Multicast group record types */
#define MULTICAST_MODE_IS_INCLUDE           1
#define MULTICAST_MODE_IS_EXCLUDE           2
#define MULTICAST_CHANGE_TO_INCLUDE_MODE    3
#define MULTICAST_CHANGE_TO_EXCLUDE_MODE    4

#define MULTICAST_ALLOW_NEW_SOURCES         5
#define MULTICAST_BLOCK_OLD_SOURCES         6


/* Macros used by Multi_Update_Device_State */
#define MULTICAST_JOIN_GROUP                1
#define MULTICAST_LEAVE_GROUP               2
#define MULTICAST_UPDATE_GROUP              3
#define MULTICAST_NO_MSG_TO_SEND            -1

/* Macros used by Multi_Verify_Src_By_Filter */
#define MULTICAST_VERIFY_DEVICE             1
#define MULTICAST_VERIFY_SOCKET             2

#define MULTICAST_ACCEPT_SOURCE             1
#define MULTICAST_REFUSE_SOURCE             0

/* Macro used by Multi_Get_Sck_State */
#define MULTICAST_GET_SCK_LIST_HEAD         -1

    /* Multicast timer index */
#define IGMPV1_MIN_TMR_INDEX                1
#define IGMPV1_MAX_TMR_INDEX                20
#define IGMPV2_MIN_TMR_INDEX                21
#define IGMPV2_MAX_TMR_INDEX                40

#define IGMPV3_MODE_INC_MIN_TMR_INDEX       41
#define IGMPV3_MODE_INC_MAX_TMR_INDEX       60
#define IGMPV3_MODE_EXC_MIN_TMR_INDEX       61
#define IGMPV3_MODE_EXC_MAX_TMR_INDEX       80

#define IGMPV3_CHANGE_TO_INC_MIN_TMR_INDEX  81
#define IGMPV3_CHANGE_TO_INC_MAX_TMR_INDEX  100
#define IGMPV3_CHANGE_TO_EXC_MIN_TMR_INDEX  101
#define IGMPV3_CHANGE_TO_EXC_MAX_TMR_INDEX  120

#define IGMPV3_ALLOW_NEW_SRCS_MIN_TMR_INDEX 121
#define IGMPV3_ALLOW_NEW_SRCS_MAX_TMR_INDEX 140
#define IGMPV3_BLOCK_OLD_SRCS_MIN_TMR_INDEX 141
#define IGMPV3_BLOCK_OLD_SRCS_MAX_TMR_INDEX 160


#define MLDV1_MIN_TMR_INDEX                 200
#define MLDV1_MAX_TMR_INDEX                 220

#define MLDV2_MODE_INC_MIN_TMR_INDEX        221
#define MLDV2_MODE_INC_MAX_TMR_INDEX        240
#define MLDV2_MODE_EXC_MIN_TMR_INDEX        241
#define MLDV2_MODE_EXC_MAX_TMR_INDEX        260

#define MLDV2_CHANGE_TO_INC_MIN_TMR_INDEX   261
#define MLDV2_CHANGE_TO_INC_MAX_TMR_INDEX   280
#define MLDV2_CHANGE_TO_EXC_MIN_TMR_INDEX   281
#define MLDV2_CHANGE_TO_EXC_MAX_TMR_INDEX   300

#define MLDV2_ALLOW_NEW_SRCS_MIN_TMR_INDEX  301
#define MLDV2_ALLOW_NEW_SRCS_MAX_TMR_INDEX  320
#define MLDV2_BLOCK_OLD_SRCS_MIN_TMR_INDEX  321
#define MLDV2_BLOCK_OLD_SRCS_MAX_TMR_INDEX  340


typedef struct _MULTI_DATA
{
    DV_DEVICE_ENTRY     *multi_device;      /* Device ptr */
    UINT8               multi_refcount;     /* Reference count */
    UINT8               multi_state;        /* Current state of listener */
    UINT8               multi_source_addr_pend_report;  /* Flag to show
                                                         * that there is a
                                                         * Address-and-
                                                         * Source-Specific
                                                         * report pending
                                                         */
    UINT8               multi_sent_last_report;         /* This flag is set
                                                         * if we sent the
                                                         * last report for
                                                         * this group
                                                         */
    UINT32              multi_timer;        /* Timer value of listener */
    UINT32              multi_set_time;
    UINT32              multi_timer_index;  /* Timer index */
    UINT32              multi_startup_count;
    UINT32              multi_startup_index;
} MULTI_DATA;

#define MULTI_DATA_REFCOUNT_OFFSET      4
#define MULTI_DATA_STATE_OFFSET         5
#define MULTI_DATA_TIMER_OFFSET         6
#define MULTI_DATA_TIMER_INDX_OFFSET    10
#define MULTI_DATA_STARTUP_CNT_OFFSET   14
#define MULTI_DATA_STARTUP_INDX_OFFSET  18


/* Double Link Lists for the multicast state of
 *  sockets and devices
 */
struct _MULTI_SCK_STATE_LIST
{
    MULTI_SCK_STATE *head;
    MULTI_SCK_STATE *tail;
};

struct _MULTI_SCK_OPTIONS
{
    DV_DEVICE_ENTRY     *multio_device;
#if (INCLUDE_IPV4 == NU_TRUE)
    IP_MULTI            *multio_v4_membership[IP_MAX_MEMBERSHIPS];
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    IP6_MULTI           *multio_v6_membership[IP6_MAX_MEMBERSHIPS];
#endif
    UINT8               multio_ttl;
    UINT8               multio_hop_lmt;
    UINT8               multio_loop;

#if (INCLUDE_IPV4 == NU_TRUE)
    UINT8               multio_num_mem_v4;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    UINT8               multio_num_mem_v6;
#if (INCLUDE_IPV4 == NU_TRUE)
    UINT8               multio_pad[3];
#endif
#endif
};

struct _MULTI_DEV_STATE
{
    MULTI_DEV_STATE         *dev_state_next;
    MULTI_DEV_STATE         *dev_state_prev;
    MULTI_SCK_STATE_LIST    dev_sck_state_list;
    UINT16                  dev_filter_state;
    UINT16                  dev_num_src_addr;
    UINT16                  dev_num_src_to_report;
    INT16                   dev_family_type;
    UINT8                   *dev_multiaddr;
    UINT8  HUGE             *dev_src_addrs;
    UINT8  HUGE             *dev_src_to_report;
};

struct _MULTI_SCK_STATE
{
    MULTI_SCK_STATE     *sck_state_next;
    MULTI_SCK_STATE     *sck_state_prev;
    UINT16              sck_filter_state;
    UINT16              sck_num_src_addr;
    INT                 sck_socketd;
    INT16               sck_family_type;
    UINT8               sck_pad[2];
    UINT8               *sck_multi_addr;
    UINT8  HUGE         *sck_src_list;
};

struct _MULTI_IP
{
    MULTI_IP                *multi_next;
    MULTI_DATA              multi_data;
    UINT16                  multi_num_query_src_list;
    UINT8                   ipm_pad[2];
    UINT8                   *multi_addr;
    UINT8  HUGE             *multi_query_src_list;
};


/* This typedef is done so that the parameter list of
 *  IP_Send and IP6_Send can remain unchanged.
 */
typedef MULTI_SCK_OPTIONS   IP_MULTI_OPTIONS;
typedef MULTI_SCK_OPTIONS   IP6_MULTI_OPTIONS;


STATUS Multi_Input(const NET_BUFFER *, const DV_DEVICE_ENTRY *, UNSIGNED);
VOID   Multi_Update_Entry(const NET_BUFFER *, VOID *, UINT8, UINT8, UINT32);
VOID   Multi_Expire_Entry(VOID *, UNSIGNED);
STATUS Multi_Output(VOID *, UNSIGNED, UINT8, UINT16);
UINT32 Multi_Calculate_Code_Value(MULTI_IP *, UINT8);
VOID   Multi_Timer_Expired(UNSIGNED, UNSIGNED, UNSIGNED);
VOID   Multi_Update_Host_Compatibility(const NET_BUFFER *, MULTI_IP *, UINT8);
UINT32 Multi_Get_Timer_Index(const DV_DEVICE_ENTRY *, const MULTI_DEV_STATE *,
                             UINT8, UINT8);
MULTI_DEV_STATE *Multi_Get_Device_State_Struct(const DV_DEVICE_ENTRY *,
                                               const UINT8 *, UINT8);
INT16 Multi_Src_List_Different_Mode(const UINT8 HUGE*, UINT16, const UINT8 HUGE*,
                                    UINT16, UINT8 *, UINT8);
INT16 Multi_Src_List_Same_Mode(const UINT8 HUGE*, UINT16, const UINT8 HUGE*,
                               UINT16, UINT8 *, UINT8 *, UINT8);
VOID  Multi_Set_Timer(TQ_EVENT, UNSIGNED, UNSIGNED, UNSIGNED, MULTI_DATA *);
VOID  Multi_Unset_Timer(TQ_EVENT, INT16, UNSIGNED, UNSIGNED, MULTI_DATA *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif /* MULTI_H */
