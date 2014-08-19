/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       lcp_defs.h
*
*   COMPONENT
*
*       LCP - Link Control Protocol
*
*   DESCRIPTION
*
*       This file contains constant definitions and structure definitions
*       to support the file lcp.c
*
*   DATA STRUCTURES
*
*       LCP_OPTIONS
*       LCP_LAYER
*       LCP_FRAME
*       LCP_OPTION
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_INC_LCP_DEFS_H
#define PPP_INC_LCP_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

/* This macro defines the timeout used when waiting for LCP to complete
   the close sequence and for the modem to hangup. */
#define LCP_LINK_CLOSE_TIMEOUT          (60 * SCK_Ticks_Per_Second)

#define LCP_MAX_DATA_SIZE               100
#define LCP_CLEAR_HIGH                  0x0000ffff

/* define basic packet lengths */
#define LCP_HEADER_LEN                  4
#define LCP_MAGIC_LEN                   4
#define LCP_OPTION_HDRLEN               2


#define LCP_MRU_LENGTH                  4
#define LCP_MAGIC_NUMBER_LENGTH         6
#define LCP_ACCM_LENGTH                 6
#define LCP_PROTOCOL_COMPRESS_LENGTH    2
#define LCP_ADDRESS_COMPRESS_LENGTH     2
#define LCP_CHAP_LENGTH                 5
#define LCP_PAP_LENGTH                  4

#define LCP_ACCM_VALUE_SIZE             4

#if(INCLUDE_PPP_MP == NU_TRUE)

#define LCP_MRRU_LENGTH                 4
#define LCP_SHORT_SEQ_NUM_LENGTH        2

#endif

/* Define offsets into the LCP packet. PAP and CHAP are the only
   remaining files that use this. */
#define LCP_ID_OFFSET                   1
#define LCP_LENGTH_OFFSET               2


/* Packet type codes. */
#define LCP_CONFIGURE_REQUEST           1
#define LCP_CONFIGURE_ACK               2
#define LCP_CONFIGURE_NAK               3
#define LCP_CONFIGURE_REJECT            4
#define LCP_TERMINATE_REQUEST           5
#define LCP_TERMINATE_ACK               6
#define LCP_CODE_REJECT                 7
#define LCP_PROTOCOL_REJECT             8
#define LCP_ECHO_REQUEST                9
#define LCP_ECHO_REPLY                  10
#define LCP_DISCARD_REQUEST             11
#define LCP_CHAP_MD5                    5
#define LCP_CHAP_MS1                    0x80 /* MS CHAP v1. */
#define LCP_CHAP_MS2                    0x81 /* MS CHAP v2. */

/* Option type codes. */
#define LCP_OPTION_MRU                  1
#define LCP_OPTION_ACCM                 2
#define LCP_OPTION_AUTH                 3
#define LCP_OPTION_MAGIC                5
#define LCP_OPTION_PFC                  7
#define LCP_OPTION_ACC                  8


/* If PPP Multilink Protocol is enabled. */
#if(INCLUDE_PPP_MP == NU_TRUE)
#define LCP_OPTION_MRRU                 17  /* Multilink MRRU option. */
#define LCP_OPTION_SHORT_SEQ_NUM        18  /* Short seq. option. */
#define LCP_OPTION_ENDPOINT_DISC        19  /* Endpoint discriminator
                                               option. */

/* LCP Endpoint Discriminator Classes. */
#define LCP_ENDPOINT_DISC_NULL            0 /* NULL CLASS. */
#define LCP_ENDPOINT_DISC_IP              2 /* IP Address. */
#define LCP_ENDPOINT_DISC_MAC             3 /* IEEE 802.1 Globally Assigned
                                               MAC Address .*/
#define LCP_ENDPOINT_DISC_DIRECTORY       5 /* Public Switched Network
                                               Directory Number. */

/* LCP Endpoint Discriminator Class Sizes. */
#define LCP_ENDPOINT_DISC_NULL_SIZE          0
#define LCP_ENDPOINT_DISC_IP_SIZE            4
#define LCP_ENDPOINT_DISC_MAC_SIZE           6
#define LCP_ENDPOINT_DISC_DIRECTORY_MAX_SIZE 15

/* Option flags. */
#define PPP_FLAG_SHORT_SEQ_NUM          0x0200
#define PPP_FLAG_MRRU                   0x0400
#define PPP_FLAG_ENDPOINT_DISC          0x0800
#endif  /* INCLUDE_PPP_MP */

/* CCP Flag. */
#define NET_CCP                         0x2000

/* Option flags. */
#define PPP_FLAG_MRU                    0x0001
#define PPP_FLAG_ACCM                   0x0002
#define PPP_FLAG_MAGIC                  0x0004
#define PPP_FLAG_PFC                    0x0008
#define PPP_FLAG_ACC                    0x0010
#define PPP_FLAG_CHAP                   0x0020
#define PPP_FLAG_PAP                    0x0040
#define PPP_FLAG_CHAP_MS1               0x0080
#define PPP_FLAG_CHAP_MS2               0x0100
#define PPP_AUTH_MASK                   (PPP_FLAG_CHAP    | PPP_FLAG_PAP)
#define PPP_AUTH_CHAP_MASK              (PPP_FLAG_CHAP_MS1| \
                                         PPP_FLAG_CHAP_MS2)

/* LCP options structure */
typedef struct _lcp_opts_struct
{
    /* Current negotiation option values. */
    UINT32  magic_number;
    UINT32  accm;
    UINT32  fcs_size;

    /* Initial default values, as set by application or SNMP manager. */
    UINT32  default_accm;
    UINT32  default_fcs_size;
    UINT16  default_mru;

    UINT16  mru;

    /* Configuration switches to turn options on/off during negotiation. */
    UINT32  default_flags;
    UINT32  flags;

    UINT32  chap_protocol;                  /* For ms-chap protocols. */
    UINT16  mp_mrru;
    UINT8   mp_endpoint_disc[16];
    UINT8   mp_endpoint_disc_len;
    UINT8   mp_endpoint_class;

    /* The following pointers are used to store LCP configuration requests.
       They are mainly used for Nucleus L2TP. */
    VOID    *init_cfg_req;
    VOID    *last_cfg_req;

} LCP_OPTIONS;


typedef struct lcp_layer
{
    UINT8                   state;
    UINT8                   identifier;
    INT8                    num_transmissions;
    INT8                    echo_counter;

    struct
    {
        LCP_OPTIONS local;
        LCP_OPTIONS remote;
    } options;

} LCP_LAYER;


/* These structures are only used to cast a string buffer
   into an LCP structure for working on the fields. */
typedef struct lcp_packet_header_struct
{
    UINT8       code;
    UINT8       id;
    UINT16      len;
    UINT8 HUGE  data[508];
} LCP_FRAME;


typedef struct lcp_option
{
    UINT8       code;
    UINT8       len;
    UINT8 HUGE  data[254];
} LCP_OPTION;




/* define the possible LCP states */
#define INITIAL     0
#define STARTING    1
#define CLOSED      2
#define STOPPED     3
#define CLOSING     4
#define STOPPING    5
#define REQ_SENT    6
#define ACK_RCVD    7
#define ACK_SENT    8
#define OPENED      9

#define PPP_DEBUG_STATES    "INITIAL", "STARTING", "CLOSED", "STOPPED",\
                            "CLOSING", "STOPPING", "REQ_SENT", "ACK_RCVD",\
                            "ACK_SENT", "OPENED"

#define LCP_RAND_MAX        256
#define LCP_RAND_MAX32      1000000000ul

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_LCP_DEFS_H */
