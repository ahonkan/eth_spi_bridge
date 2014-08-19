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

/***********************************************************************
*
* FILE NAME
*
*       igmp.h
*
* COMPONENT
*
*       IGMP -     Implements the IGMP protocol.
*
* DESCRIPTION
*
*       This header file holds all the includes and defines used in IGMP.
*
* DATA STRUCTURES
*
*       IGMP_LAYER
*
* DEPENDENCIES
*
*       No other file dependencies
*
*************************************************************************/


#ifndef IGMP_H
#define IGMP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

#define IGMP_HEADER_LEN     8

typedef struct _IGMP_LAYER
{
    UINT8   igmp_type;
    UINT8   igmp_unused;
    UINT16  igmp_cksum;
    UINT32  igmp_group;
} IGMP_LAYER;

typedef struct _IGMPV3_REPORT_LAYER
{
    UINT8   igmp_msg_type;
    UINT8   igmp_resrv1;
    UINT16  igmp_cksum;
    UINT16  igmp_resrv2;
    UINT16  igmp_num_group_recs;
} IGMPV3_REPORT_LAYER;

typedef struct _IGMPV3_GROUP_RECORD_LAYER
{
    UINT8   igmp_rec_type;
    UINT8   igmp_aux_data_len;
    UINT16  igmp_num_srcs;
    UINT32  igmp_group;
} IGMPV3_GROUP_RECORD_LAYER;


typedef struct _IGMPV3_QUERY_LAYER
{
    UINT8   igmp_msg_type;
    UINT8   igmp_max_resp_code;
    UINT16  igmp_cksum;
    UINT32  igmp_group;
    UINT8   igmp_resv;
    UINT8   igmp_qqic;
    UINT16  igmp_num_srcs;
} IGMPV3_QUERY_LAYER;


/* This is the IP address that all level 2 conforming hosts
 * should be a member of. */
#define IGMP_ALL_HOSTS_GROUP   0xE0000001UL

/* This is the IP address that all IGMP conforming routers
 * will listen to */
#define IGMP_ALL_ROUTERS_GROUP      0xE0000002UL

/* This is the IP address that all IGMPv3-capable routers
 * listen to for reports */
#define IGMPV3_ALL_ROUTERS_GROUP    0xE0000016UL

/* IGMP packet types. */
#define IGMP_HOST_MEMBERSHIP_QUERY      0x11
#define IGMPV1_HOST_MEMBERSHIP_REPORT   0x12
#define IGMPV2_HOST_MEMBERSHIP_REPORT   0x16
#define IGMPV3_HOST_MEMBERSHIP_REPORT   0x22
#define IGMP_GROUP_LEAVE_MESSAGE        0x17

/* IGMP Interoperability */
#define IGMPV3_QUERY                    12
#define IGMPV1_COMPATIBILITY            1
#define IGMPV2_COMPATIBILITY            2
#define IGMPV3_COMPATIBILITY            3
#define IGMPV1_COMPAT_TIMER_RUNNING     0x1
#define IGMPV2_COMPAT_TIMER_RUNNING     0x2



#define IGMPV1_UNUSED_OFFSET                1

#define IGMP_TYPE_OFFSET                    0
#define IGMP_MAX_RESP_CODE_OFFSET           1
#define IGMP_CKSUM_OFFSET                   2
#define IGMP_GROUP_OFFSET                   4
#define IGMP_QUERIER_ROBUSTNESS_VARIABLE    8
#define IGMP_QUERIER_QUERY_INTERVAL_CODE    9
#define IGMP_NUMBER_OF_SOURCE_ADDRESSES     10
#define IGMP_SOURCE_ADDRESS                 12

#define IGMP_REPORT_TYPE_OFFSET             0
#define IGMP_REPORT_RESERVED1_OFFSET        1
#define IGMP_REPORT_CHECKSUM_OFFSET         2
#define IGMP_REPORT_RESERVED2_OFFSET        4
#define IGMP_REPORT_NUMBER_OF_GROUP_RECORDS 6
#define IGMP_REPORT_GROUP_RECORD            8


#define IGMP_RECORD_HEADER_SIZE                 8
#define IGMP_RECORD_TYPE_OFFSET                 0
#define IGMP_RECORD_AUX_DATA_LENGTH_OFFSET      1
#define IGMP_RECORD_NUMBER_OF_SRC_ADDRS_OFFSET  2
#define IGMP_RECORD_MULTICAST_ADDR_OFFSET       4
#define IGMP_RECORD_SOURCE_ADDR_OFFSET          8



#define IGMP_MESSAGE_TYPE(index)                          \
    ( ((index >= IGMPV1_MIN_TMR_INDEX) &&                 \
       (index <= IGMPV1_MAX_TMR_INDEX)) ?                 \
                    IGMPV1_HOST_MEMBERSHIP_REPORT      :  \
      ((index >= IGMPV2_MIN_TMR_INDEX) &&                 \
       (index <= IGMPV2_MAX_TMR_INDEX)) ?                 \
                    IGMPV2_HOST_MEMBERSHIP_REPORT      :  \
      ((index >= IGMPV3_MODE_INC_MIN_TMR_INDEX) &&        \
       (index <= IGMPV3_MODE_INC_MAX_TMR_INDEX)) ?        \
                    MULTICAST_MODE_IS_INCLUDE          :  \
      ((index >= IGMPV3_MODE_EXC_MIN_TMR_INDEX) &&        \
       (index <= IGMPV3_MODE_EXC_MAX_TMR_INDEX)) ?        \
                    MULTICAST_MODE_IS_EXCLUDE          :  \
      ((index >= IGMPV3_CHANGE_TO_INC_MIN_TMR_INDEX) &&   \
       (index <= IGMPV3_CHANGE_TO_INC_MAX_TMR_INDEX)) ?   \
                    MULTICAST_CHANGE_TO_INCLUDE_MODE   :  \
      ((index >= IGMPV3_CHANGE_TO_EXC_MIN_TMR_INDEX) &&   \
       (index <= IGMPV3_CHANGE_TO_EXC_MAX_TMR_INDEX)) ?   \
                    MULTICAST_CHANGE_TO_EXCLUDE_MODE   :  \
      ((index >= IGMPV3_ALLOW_NEW_SRCS_MIN_TMR_INDEX) &&  \
       (index <= IGMPV3_ALLOW_NEW_SRCS_MAX_TMR_INDEX)) ?  \
                    MULTICAST_ALLOW_NEW_SOURCES        :  \
      ((index >= IGMPV3_BLOCK_OLD_SRCS_MIN_TMR_INDEX) &&  \
       (index <= IGMPV3_BLOCK_OLD_SRCS_MAX_TMR_INDEX)) ?  \
                    MULTICAST_BLOCK_OLD_SOURCES : NU_INVAL)


STATUS  IGMP_Initialize(DV_DEVICE_ENTRY *);
VOID    IGMP_Join(IP_MULTI *, UINT8);
UINT32  IGMP_Random_Delay(IP_MULTI *);
VOID    IGMP_Report_Event(IP_MULTI *);
STATUS  IGMP_Interpret(NET_BUFFER *, const DV_DEVICE_ENTRY *);
VOID    IGMP_Update_Host_Compatibility(const NET_BUFFER *, MULTI_IP *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif /* IGMP_H */
