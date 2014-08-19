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
*       ppp_defs.h
*
*   COMPONENT
*
*       PPP - Core component of PPP
*
*   DESCRIPTION
*
*       This file contains constant definitions and structure
*       definitions to support the file ppp.c
*
*   DATA STRUCTURES
*
*       LINK_LAYER
*       HWI_LAYER
*       PPP_USER
*       PPP_OPTIONS
*       NU_PPP_OPTIONS
*       PPP_CFG
*       NU_PPP_CFG
*
*   DEPENDENCIES
*
*       lcp_defs.h
*       ccp_defs.h
*       ncp_defs.h
*       chp_defs.h
*       pap_defs.h
*       mppe_defs.h
*       pp6_defs.h
*       pm_defs.h
*       hdlc.h
*       mdm_defs.h
*
*************************************************************************/
#ifndef PPP_INC_PPP_DEFS_H
#define PPP_INC_PPP_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/lcp_defs.h"
#include "drivers/ncp_defs.h"
#include "drivers/chp_defs.h"
#include "drivers/pap_defs.h"
#include "drivers/mppe_defs.h"
#include "drivers/ccp_defs.h"
#include "drivers/pp6_defs.h"
#include "drivers/pm_defs.h"
#include "drivers/hdlc.h"
#include "drivers/mdm_defs.h"

/* Number of words in PPP event message. */
#define PPP_EVENT_QUEUE_SIZE    8
#define PPP_EVENT_MESSAGE_SIZE  3

/* Item order in the event message. */
#define PPP_MSG_EVENT_INDEX     0
#define PPP_MSG_DAT_INDEX       1
#define PPP_MSG_EXTDAT_INDEX    2

/* PPP interface types */
#define PPP_ITYPE_UART          0x00000001
#define PPP_ITYPE_MODEM         0x00000002
#define PPP_ITYPE_NULL_MODEM    0x00000004
#define PPP_ITYPE_PPPOE         0x00000008
#define PPP_ITYPE_ATM           0x00000010
#define PPP_ITYPE_BUFFER        0x00000020
#define PPP_ITYPE_VIRTUAL       0x00000040
#define PPP_ITYPE_L2TP_LAC      0x00000080
#define PPP_ITYPE_L2TP_LNS      0x00000100
#define PPP_ITYPE_L2TP          0x00000200
#define PPP_ITYPE_PPTP          0x00000400

/* PPP protocol numbers */
#define PPP_ADDR_CONTROL                0xff03
#define PPP_LINK_CONTROL_PROTOCOL       0xc021
#define PPP_PAP_PROTOCOL                0xc023
#define PPP_CHAP_PROTOCOL               0xc223
#define PPP_IP_CONTROL_PROTOCOL         0x8021
#define PPP_IPV6_CONTROL_PROTOCOL       0x8057
#define PPP_IP_PROTOCOL                 0x0021
#define PPP_IPV6_PROTOCOL               0x0057
#define PPP_MP_PROTOCOL                 0x003d
#define PPP_CCP_PROTOCOL                0x80fd
#define PPP_COMPRESSED_DATA             0x00fd

#define PPP_CHAP_MS1_PROTOCOL           LCP_CHAP_MS1
#define PPP_CHAP_MS2_PROTOCOL           LCP_CHAP_MS2


/* Various constants used for PPP */
#define PPP_ONE                         1
#define PPP_TWO                         2
#define PPP_PROTOCOL_HEADER_1BYTE       (UINT8)1
#define PPP_PROTOCOL_HEADER_2BYTES      (UINT8)2
#define PPP_MTU                         1500
#define PPP_MRU                         PPP_MTU

/* FCS */
#define PPP_DEFAULT_FCS_SIZE            16

/* DNS Servers */
#define PPP_MAX_DNS_SERVERS             2

/* Service parameters */
#define PPP_FORCE                       1
#define PPP_NO_FORCE                    2
#define PPP_BLOCK                       4

/* Negotiation events. */
#define PPP_LCP_FAIL                    1
#define PPP_AUTH_FAIL                   2
#define PPP_NCP_FAIL                    4
#define PPP_CONFIG_SUCCESS              8
#define PPP_NEG_ABORT                   16
#define PPP_NEG_TIMEOUT                 32
#define PPP_CCP_FAIL                    64

/* PPP sub-events. */
#define LCP_OPEN_REQUEST                1
#define LCP_SEND_CONFIG                 2
#define LCP_LAYER_UP                    3
#define LCP_ECHO_REQ                    4
#define LCP_CLOSE_REQUEST               5
#define LCP_SEND_TERMINATE              6

#define AUTH_REQUEST                    7
#define CHAP_SEND_CHALLENGE             8
#define PAP_SEND_AUTH                   9
#define AUTHENTICATED                   10

#define NCP_OPEN_REQUEST                11
#define NCP_SEND_CONFIG                 12
#define NCP_LAYER_UP                    13
#define NCP_CLOSE_REQUEST               14
#define NCP_LAYER_DOWN                  15
#define NCP_SEND_TERMINATE              16
#define MDM_HANGUP                      17
#define LCP_LAYER_DOWN                  18
#define PPP_STOP_NEGOTIATION            19
#define PPP_ABORT_NEGOTIATION           20

#define CCP_OPEN_REQUEST                21
#define CCP_SEND_CONFIG                 22
#define CCP_LAYER_UP                    23
#define CCP_CLOSE_REQUEST               24
#define CCP_SEND_TERMINATE              25
#define CCP_LAYER_DOWN                  26


/* Status flag types. */
#define PPP_FLAG_DROP                   1
#define PPP_FLAG_ACK                    2
#define PPP_FLAG_NAK                    4
#define PPP_FLAG_REJECT                 8
#define PPP_FLAG_ABORT                  16
#define PPP_FLAG_OK                     32

/* PPP direction mode */
#define PPP_SERVER                      1
#define PPP_CLIENT                      2

/* Macro definitions for PPP Get Set option APIs. */
#define PPP_DEFAULT_ACCM                1
#define PPP_DEFAULT_AUTH_PROTOCOL       2
#define PPP_DEFAULT_FCS                 3
#define PPP_DEFAULT_MRU                 4
#define PPP_NUM_OF_DNS_SERVERS          5
#define PPP_DEFAULT_MP_MRRU             6
#define PPP_ENABLE_PFC                  7
#define PPP_ENABLE_ACC                  8
#define PPP_ENABLE_MAGIC_NUMBER         9
#define PPP_ENABLE_ACCM                 10
#define PPP_ENABLE_MRU                  11
#define PPP_ENABLE_MP_MRRU              12
#define PPP_ENABLE_ENCRYPTION_REQUIRED  13
#define PPP_ENABLE_128BIT_ENCRYPTION    14
#define PPP_ENABLE_56BIT_ENCRYPTION     15
#define PPP_ENABLE_40BIT_ENCRYPTION     16

typedef struct _ppp_layer               LINK_LAYER;
typedef struct hwi_layer                HWI_LAYER;

struct hwi_layer
{
    UINT16                  itype;
    UINT16                  state;
    DV_DEVICE_ENTRY         *dev_ptr;
    DV_DEVICE_ENTRY         *hdev_ptr;
    STATUS                  (*init)(DV_DEVICE_ENTRY*);
    STATUS                  (*passive)(DV_DEVICE_ENTRY*);
    STATUS                  (*connect)(CHAR *number, DV_DEVICE_ENTRY*);
    STATUS                  (*disconnect)(LINK_LAYER*, UINT8);
};


/* Define the PPP layer structure. This will hold all information about
   the PPP link. This structure will be pointed to by the device
   structure for a particular physical device. */
struct _ppp_layer
{
    HWI_LAYER               hwi;
    VOID                    *link;
    STATUS                  connection_status;
    AUTHENTICATION_LAYER    authentication;
    LCP_LAYER               lcp;

#if (INCLUDE_IPV4 == NU_TRUE)
    NCP_LAYER               ncp;
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    NCP_LAYER               ncp6;
#endif

    CCP_LAYER               ccp;

    NU_EVENT_GROUP          negotiation_progression;
    UINT32                  last_activity_time;
    UINT16                  negotiation_timeout;

    INT16                   term_id;
    UINT16                  data_received;
    UINT8                   mode;
    UINT8                   pad[1];

    struct
    {
        UINT32              silent_discards;
        UINT32              address_field_errors;
        UINT32              control_field_errors;
        UINT32              overrun_errors;
        UINT32              fcs_errors;
    } status;

    /* For compatibility with old serial drivers. */
    HDLC_TEMP_BUFFER HUGE   *rx_ring;
    UINT16                  received;
    UINT8                   esc_received;
    UINT8                   rx_ring_write;
    VOID                    *ppp_serial_rx_task_mem;
    VOID                    *ppp_init_mem;
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
    PM_MIN_REQ_HANDLE       ppp_pm_handle;
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
    NU_TASK                 ppp_task_cb;
    NU_TASK                 serial_rx_task;
#if (HDLC_POLLED_TX == NU_FALSE)
    VOID                    *ppp_serial_tx_task_mem;
    NU_TASK                 serial_tx_task;
    NU_EVENT_GROUP          ppp_tx_event;
#endif
    NU_QUEUE                ppp_event_queue;
    /*  the communication mode in which the UART will operate */
    INT                     comm_mode;
    SERIAL_SESSION          *uart;

};


typedef struct pw_list
{
    CHAR    id[PPP_MAX_ID_LENGTH];
    CHAR    pw[PPP_MAX_PW_LENGTH];
} PPP_USER;


/* PPP Application Services */
#define NU_Dial_PPP_Server                  PPP_Dial
#define NU_Wait_For_PPP_Client              PPP_Wait_For_Client
#define NU_PPP_Still_Connected              PPP_Still_Connected
#define NU_PPP_Hangup                       PPP_Hangup
#define NU_Get_PPP_Link_Options             PPP_Get_Link_Options
#define NU_Get_PPP_Link_Option              PPP_Get_Link_Option
#define NU_Set_PPP_Link_Options             PPP_Set_Link_Options
#define NU_Set_PPP_Link_Option              PPP_Set_Link_Option
#define NU_Validate_Link_Options            PPP_Validate_Link_Options
#define NU_PPP_Get_Default_Options          PPP_Get_Default_Options
#define NU_Obtain_PPP_Connection_Status     PPP_Obtain_Connection_Status
#define NU_Abort_Wait_For_PPP_Client        PPP_Abort_Wait_For_Client
#define NU_PPP_Abort                        PPP_Abort
#define NU_PPP_Abort_Connection             PPP_Abort_Connection
#define NU_PPP_Last_Activity_Time           PPP_Last_Activity_Time
#define NU_PPP_Negotiation_Timeout          PPP_Negotiation_Timeout
#define NU_Add_PPP_User                     AUTH_Add_User
#define NU_Remove_PPP_User                  AUTH_Remove_User
#define NU_PPP_Set_Local_Num                MDM_Set_Local_Num
#define NU_PPP_Get_Remote_Num               MDM_Get_Remote_Num
#define NU_Set_PPP_Client_IP_Address        NCP_Set_Client_IP_Address

/* PPP Application Constants */
#define NU_PPP_CHAP_MS1_PROTOCOL            PPP_CHAP_MS1_PROTOCOL
#define NU_PPP_CHAP_MS2_PROTOCOL            PPP_CHAP_MS2_PROTOCOL
#define NU_PPP_CHAP_PROTOCOL                PPP_CHAP_PROTOCOL
#define NU_PPP_PAP_PROTOCOL                 PPP_PAP_PROTOCOL

#define NU_FORCE                            PPP_FORCE
#define NU_NO_FORCE                         PPP_NO_FORCE
#define NU_WAIT_FOR_HANGUP                  PPP_BLOCK

/* States of negotiation. */
#define NU_PPP_DIALING                  100
#define NU_PPP_WAITING                  101
#define NU_PPP_LINK_NEGOTIATION         102
#define NU_PPP_AUTHENTICATION           103
#define NU_PPP_NETWORK_NEGOTIATION      104
#define NU_PPP_CONNECTED                105
#define NU_PPP_DISCONNECTED             106
#define NU_PPP_HANGING_UP               107
#define NU_PPP_WAITING_ABORTED          108

/* The PPP error return codes are within the range of -501 to -599 */
#define NU_LCP_FAILED                   -501
#define NU_NCP_FAILED                   -502
#define NU_LOGIN_FAILED                 -503
#define NU_CCP_FAILED                   -504
#define NU_NETWORK_BUSY                 -505
#define NU_INVALID_LINK                 -506
#define NU_NO_CONNECT                   -507
#define NU_NO_CARRIER                   -508
#define NU_BUSY                         -509
#define NU_INVALID_MODE                 -510
#define NU_NO_MODEM                     -511
#define NU_PPP_ATTEMPT_ABORTED          -512
#define NU_PPP_INIT_FAILURE             -513
#define NU_NEGOTIATION_TIMEOUT          -514
#define NU_NEGOTIATION_ABORTED          -515
#define NU_PPP_INVALID_PROTOCOL         -516
#define NU_AUTH_FAILED                  -517
#define NU_PPP_INVALID_PARAMS           -518
#define NU_PPP_ENTRY_NOT_FOUND          -519
#define NU_PPP_INVALID_OPTION           -520
#define NU_INVALID_DEFAULT_MRU          -521
#define NU_INVALID_DEFAULT_PROTOCOL     -522
#define NU_INVALID_NUM_DNS_SERVERS      -523
#define NU_INVALID_PF_COMPRESSION       -524
#define NU_INVALID_AC_COMPRESSION       -525
#define NU_INVALID_USE_MAGIC_NUMBER     -526
#define NU_INVALID_USE_ACCM             -527
#define NU_INVALID_REQUIRE_ENCRYPTION   -528
#define NU_INVALID_40BIT_ENCRYPTION     -529
#define NU_INVALID_56BIT_ENCRYPTION     -530
#define NU_INVALID_128BIT_ENCRYPTION    -531

/* The following Error codes are specific to the PPPoE module. */
#define NU_PPE_ERROR                    -525
#define NU_PPE_INVALID_PADx             -526

/* Flag options for the following structure (i.e. ppp_flags). */
#define NU_PPP_RETURN_ERROR     0x0001
#define NU_PPP_NO_IPV4          0x0002

typedef struct  ppp_call_options
{
    UINT32 ppp_flags;
    CHAR  *ppp_num_to_dial;
    CHAR  *ppp_link;

#if(INCLUDE_PPP_MP == NU_TRUE)
    CHAR  *ppp_mp_link;
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    UINT8 *ppp_local_ip4;
    UINT8 *ppp_remote_ip4;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    UINT8 *ppp_local_ip6;
    UINT8 *ppp_remote_ip6;
#endif

}PPP_OPTIONS, NU_PPP_OPTIONS;



/* LCP option configuration structure. */
typedef struct ppp_cfg_struct
{
    UINT16  default_auth_protocol;
    UINT16  default_mru;
    UINT32  use_flags;
    UINT32  default_accm;
    UINT32  default_fcs_size;
    UINT32  num_dns_servers;

#if (PPP_ENABLE_MPPE == NU_TRUE)
    UINT8   use_128_bit_encryption;
    UINT8   use_56_bit_encryption;
    UINT8   use_40_bit_encryption;
    UINT8   require_encryption;
#endif

#if(INCLUDE_PPP_MP == NU_TRUE)
    UINT16  mp_mrru;

    /* correct alignment for 32 bits CPU */
    UINT8   mp_pad[2];
#endif

    /* for compatibility with older PPP versions */
    UINT8   use_accm;
    UINT8   use_magic_number;
    UINT8   use_pf_compression;
    UINT8   use_ac_compression;

} PPP_CFG, NU_PPP_CFG;


#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_PPP_DEFS_H */
