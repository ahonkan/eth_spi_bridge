/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
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
*       dhcp6.h
*                                                                                  
*   DESCRIPTION                                                              
*                       
*       This file contains the data structures and defines necessary
*       to support the DHCP client module for IPv6.
*                                                                          
*   DATA STRUCTURES                                                          
*           
*
*   DEPENDENCIES                                                             
*            
*       None.                                                              
*                                                                          
*************************************************************************/

#ifndef DHCP6_H
#define DHCP6_H

/* Message types. */
#define DHCP6_SOLICIT               1
#define DHCP6_ADVERTISE             2
#define DHCP6_REQUEST               3
#define DHCP6_CONFIRM               4
#define DHCP6_RENEW                 5
#define DHCP6_REBIND                6
#define DHCP6_REPLY                 7
#define DHCP6_RELEASE               8
#define DHCP6_DECLINE               9
#define DHCP6_RECONFIGURE           10
#define DHCP6_INFO_REQUEST          11
#define DHCP6_RELAY_FORW            12
#define DHCP6_RELAY_REPL            13

/* Option codes. */
#define DHCP6_OPT_CLIENTID       1   /* Client ID option - built by stack */
#define DHCP6_OPT_SERVERID       2   /* Server ID option - built by stack */
#define DHCP6_OPT_IA_NA          3   /* IA_NA option - built by user */
#define DHCP6_OPT_IA_TA          4   /* IA_TA option - not supported */
#define DHCP6_OPT_IAADDR         5   /* IA_ADDR option - built by user */
#define DHCP6_OPT_ORO            6   /* Option Request Option - built by stack */
#define DHCP6_OPT_PREFERENCE     7   /* Preference option - built by server */
#define DHCP6_OPT_ELAPSED_TIME   8   /* Elapsed Time option - built by stack */
#define DHCP6_OPT_RELAY_MSG      9   /* Relay Message option - not supported */
#define DHCP6_OPT_AUTH           11  /* Authentication option - built by stack */
#define DHCP6_OPT_UNICAST        12  /* Server Unicast option - built by server */
#define DHCP6_OPT_STATUS_CODE    13  /* Status Code option - built by stack */
#define DHCP6_OPT_RAPID_COMMIT   14  /* Rapid Commit option - built by user */
#define DHCP6_OPT_USER_CLASS     15  /* User Class option - built by user */
#define DHCP6_OPT_VENDOR_CLASS   16  /* Vendor Class option - built by user */ 
#define DHCP6_OPT_VENDOR_OPTS    17  /* Vendor-Specific Information option - build by user */
#define DHCP6_OPT_INTERFACE_ID   18  /* Interface ID option - not supported */
#define DHCP6_OPT_RECONF_MSG     19  /* Reconfigure Message option - built by server */
#define DHCP6_OPT_RECONF_ACCEPT  20  /* Reconfigure Accept option - built by user */
#define DHCP6_OPT_SIP_SERVER_D   21  /* NOT SUPPORTED - included for completeness only */
#define DHCP6_OPT_SIP_SERVER_A   22  /* NOT SUPPORTED - included for completeness only */
#define DHCP6_OPT_DNS_SERVERS    23  /* DNS Recursive Name Server option - built by server */
#define DHCP6_OPT_DOMAIN_LIST    24  /* Domain Search List option - not supported */

#define DHCP6_MAX_OPT_CODES      25  /* This will be the count + 1 since the parsing array starts at [1] instead of [0] */

/* Authentication protocol types. */
#define DHCP6_DELAYED_AUTH_PROT     2   /* Delayed Authentication protocol. */
#define DHCP6_RECON_KEY_AUTH_PROT   3   /* Reconfigure Key Authentication protocol. */

typedef struct dhcp6_duid_struct
{
    UINT8   duid_id[DHCP_DUID_MAX_ID_NO_LEN];
    UINT8   duid_ll_addr[DADDLEN];
    UINT16  duid_len;
    UINT16  duid_type;
    UINT16  duid_hw_type;
    UINT32  duid_time;
    UINT32  duid_ent_no;
} DHCP6_DUID_STRUCT;

typedef struct dhcp6_remote_server
{
    struct addr_struct  dhcp6_iaddr;                /* Server address. */
    DHCP6_DUID_STRUCT   dhcp6_duid;                 /* Server DUID. */
    UINT16              dhcp6_pref;                 /* Server preference. */
    UINT8               dhcp6_len;
    UINT8               padN[1];
    UINT32              dhcp6_resp_time;
} DHCP6_REMOTE_SERVER;

typedef struct dhcp6_ia_addr_struct
{
    UINT8   dhcp6_ia_addr[IP6_ADDR_LEN];
    UINT32  dhcp6_ia_pref_life;
    UINT32  dhcp6_ia_valid_life;
} DHCP6_IA_ADDR_STRUCT;

typedef struct _dhcp6_client_opts
{
    UINT32  dhcp6_opt_addr;
    UINT32  dhcp6_opt_count;
} DHCP6_CLIENT_OPTS;

typedef struct dhcp6_struct 
{
    struct dhcp6_struct     *dhcp6_prev;
    struct dhcp6_struct     *dhcp6_next;
    UINT8                   *dhcp6_opts;                  /* options to be added to discover packet */
                                                          /* See RFC 2132 for valid options. */
    UINT8                   *dhcp6_user_classes;          /* The list of user classes accepted by the server. */
    NU_TASK                 *dhcp6_task;                  /* A pointer to the task suspended waiting for a reply. */
    DHCP6_REMOTE_SERVER     dhcp6_server;
    DHCP6_IA_ADDR_STRUCT    dhcp6_offered_addr;
    DHCP6_CLIENT_OPTS       dhcp6_client_opts[DHCP6_MAX_OPT_CODES];
    UINT8                   dhcp6_system_opts[DHCP6_MAX_OPT_CODES]; /* options to include per a reconfigure message */
    UINT8                   dhcp6_id;
    UINT8                   dhcp6_flags;
    UINT8                   dhcp6_msg_type;
    UINT8                   pad1[1];
    UINT16                  dhcp6_opts_len;               /* length in octets of opts field */
    UINT16                  dhcp6_secs;                   /* seconds since boot began */   
    UINT16                  dhcp6_status_code;
    INT                     dhcp6_user_class_count;       /* The count of user classes in the above memory. */
    UINT32                  dhcp6_dev_index;  
    UINT32                  dhcp6_t1;
    UINT32                  dhcp6_t2;
    STATUS                  dhcp6_status;
} DHCP6_STRUCT;

typedef struct dhcp6_struct_list
{
    DHCP6_STRUCT    *dhcp6_head;
    DHCP6_STRUCT    *dhcp6_tail;
} DHCP6_STRUCT_LIST;

typedef struct dhcp6_queue_element
{
    UNSIGNED    dhcp6_event;
    UNSIGNED    dhcp6_data;
} DHCP6_QUEUE_ELEMENT;

typedef struct dhcp6_client
{
    UINT32  dhcp6_dev_index;
    UINT8   *dhcp6_user_opts;
    UINT16  dhcp6_opt_length;
    UINT8   padN[2];
} DHCP6_CLIENT;

typedef struct dhcp6_tx_struct
{
    UINT8   dhcp6_use;
    UINT8   dhcp6_xid[3];
    UINT8   dhcp6_id;
    UINT8   dhcp6_ds_id;
    UINT8   dhcp6_type;
    UINT8   dhcp6_retrans_count;
    UINT8   dhcp6_release_addr[IP6_ADDR_LEN];
    UINT32  dhcp6_init_time;
    UINT32  dhcp6_init_retrans;
    UINT32  dhcp6_irt;
    UINT32  dhcp6_mrt;
    UINT32  dhcp6_mrc;
    UINT32  dhcp6_mrd;
    UINT32  dhcp6_rt;
} DHCP6_TX_STRUCT;

typedef struct dhcp6_opt_struct
{
    UINT32  dhcp6_opt_addr;
    UINT32  dhcp6_opt_count;
    STATUS  (*dhcp6_parse_opt)(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
} DHCP6_OPT_STRUCT;

extern NU_SEMAPHORE         DHCP6_Cli_Resource;
extern TQ_EVENT             DHCP6_Retrans_Event;
extern TQ_EVENT             DHCP6_Renew_IA_NA_Event;
extern TQ_EVENT             DHCP6_Stateful_Config_Event;
extern TQ_EVENT             DHCP6_Stateless_Config_Event;
extern TQ_EVENT             DHCP6_Release_Event;
extern TQ_EVENT             DHCP6_Decline_Event;
extern INT                  DHCP6_Client_Socket;
extern struct addr_struct   DHCP6_Server_Addr;
extern DHCP6_OPT_STRUCT     DHCP6_Options[];
extern UINT8                DHCP6_RX_Task_Init;
extern DHCP6_STRUCT_LIST    DHCP6_Structs;
extern DHCP6_TX_STRUCT      DHCP6_Tx_List[];

/* Error codes for DHCPv6 Client. */
#define DHCP6_RETRANS_MSG   -1000
#define DHCP6_FIND_SERVER   -1001
#define DHCP6_SEND_REQUEST  -1002

/* The number of simultaneous transmissions that can be occurring in the system. */
#define DHCP6_SIM_TX_COUNT          CFG_NU_OS_NET_IPV6_MAX_DHCP6_TX_COUNT

/* Time out values. */
#define DHCP6_SOL_MAX_DELAY     (1 * SCK_Ticks_Per_Second)   /* Max delay of first Solicit */
#define DHCP6_SOL_TIMEOUT       (1 * SCK_Ticks_Per_Second)   /* Initial Solicit timeout */
#define DHCP6_SOL_MAX_RT        (120 * SCK_Ticks_Per_Second) /* Max Solicit timeout value */
#define DHCP6_REQ_TIMEOUT       (1 * SCK_Ticks_Per_Second)   /* Initial Request timeout */
#define DHCP6_REQ_MAX_RT        (30 * SCK_Ticks_Per_Second)  /* Max Request timeout value */
#define DHCP6_CNF_MAX_DELAY     (1 * SCK_Ticks_Per_Second)   /* Max delay of first Confirm */
#define DHCP6_CNF_TIMEOUT       (1 * SCK_Ticks_Per_Second)   /* Initial Confirm timeout */
#define DHCP6_CNF_MAX_RT        (4 * SCK_Ticks_Per_Second)   /* Max Confirm timeout */
#define DHCP6_CNF_MAX_RD        (10 * SCK_Ticks_Per_Second)  /* Max Confirm duration */
#define DHCP6_REN_TIMEOUT       (10 * SCK_Ticks_Per_Second)  /* Initial Renew timeout */
#define DHCP6_REN_MAX_RT        (600 * SCK_Ticks_Per_Second) /* Max Renew timeout value */
#define DHCP6_REB_TIMEOUT       (10 * SCK_Ticks_Per_Second)  /* Initial Rebind timeout */
#define DHCP6_REB_MAX_RT        (600 * SCK_Ticks_Per_Second) /* Max Rebind timeout value */
#define DHCP6_INF_MAX_DELAY     (1 * SCK_Ticks_Per_Second)   /* Max delay of first Information-request */
#define DHCP6_INF_TIMEOUT       (1 * SCK_Ticks_Per_Second)   /* Initial Information-request timeout */
#define DHCP6_INF_MAX_RT        (120 * SCK_Ticks_Per_Second) /* Max Information-request timeout value */
#define DHCP6_REL_TIMEOUT       (1 * SCK_Ticks_Per_Second)   /* Initial Release timeout */
#define DHCP6_DEC_TIMEOUT       (1 * SCK_Ticks_Per_Second)   /* Initial Decline timeout */
#define DHCP6_REC_TIMEOUT       (2 * SCK_Ticks_Per_Second)   /* Initial Reconfigure timeout */

/* Retransmission values. */
#define DHCP6_REQ_MAX_RC        10  /* Max Request retry attempts */
#define DHCP6_REL_MAX_RC        5   /* MAX Release attempts */
#define DHCP6_DEC_MAX_RC        5   /* Max Decline attempts */
#define DHCP6_REC_MAX_RC        8   /* Max Reconfigure attempts */

/* Client and Server port constants. */
#define DHCP6_CLIENT_PORT           546
#define DHCP6_SERVER_PORT           547

/* Highest server preference value possible. */
#define DHCP6_MAX_SERVER_PREF       255

/* Message lengths. */
#define DHCP6_MSG_LEN               4

/* Code offsets for DHCP messages. */
#define DHCP6_MSG_TYPE_OFFSET           0
#define DHCP6_TRANS_ID_OFFSET           1

/* Code offsets for DHCP options. */
#define DHCP6_OPT_CODE_OFFSET           0
#define DHCP6_OPT_LEN_OFFSET            2

/* Code offsets for DUID. */
#define DHCP6_DUID_CODE_OFFSET          4

/* Code offsets for DUID-LLT. */
#define DHCP6_DUID_LLT_HW_TYPE_OFFSET   6
#define DHCP6_DUID_LLT_TIME_OFFSET      8
#define DHCP6_DUID_LLT_ADDR_OFFSET      12

/* Code offsets for DUID-EN. */
#define DHCP6_DUID_EN_ENT_NO_OFFSET    6
#define DHCP6_DUID_EN_ID_OFFSET        10

/* Code offsets for DUID-LL. */
#define DHCP6_DUID_LL_HW_TYPE_OFFSET    6
#define DHCP6_DUID_LL_ADDR_OFFSET       8

/* Code offsets for Client Identifier option. */
#define DHCP6_CLIENT_ID_DUID_OFFSET     4

/* Code offsets for Server Identifier option. */
#define DHCP6_SERVER_ID_DUID_OFFSET     4

/* Code offsets for IA_NA option. */
#define DHCP6_IA_NA_IAID_OFFSET         4
#define DHCP6_IA_NA_T1_OFFSET           8
#define DHCP6_IA_NA_T2_OFFSET           12

/* Code offsets for IA Address option. */
#define DHCP6_IAADR_ADDR_OFFSET         4
#define DHCP6_IAADR_PREF_LIFE_OFFSET    20
#define DHCP6_IAADR_VALID_LIFE_OFFSET   24

/* Code offsets for Option Request option. */
#define DHCP6_ORO_CODE_OFFSET           4

/* Code offsets for Preference option. */
#define DHCP6_PREF_VALUE_OFFSET         4

/* Code offsets for Elapsed Time option. */
#define DHCP6_ELAPSE_TIME_OFFSET        4

/* Code offsets for Relay Message option. */
#define DHCP6_RELAY_MSG_OFFSET          4

/* Code offsets for Server Unicast option. */
#define DHCP6_UNICAST_ADDR_OFFSET       4

/* Code offsets for Status Code option. */
#define DHCP6_STATUS_CODE_OFFSET        4
#define DHCP6_STATUS_CODE_MSG_OFFSET    6

/* Code offsets for User Class option. */
#define DHCP6_USER_CLASS_LEN_OFFSET     4
#define DHCP6_USER_CLASS_DATA_OFFSET    6

/* Code offsets for Vendor Class option. */
#define DHCP6_VENDOR_CLASS_ENT_OFFSET   4
#define DHCP6_VENDOR_CLASS_LEN_OFFSET   8
#define DHCP6_VENDOR_CLASS_DATA_OFFSET  10

/* Code offsets for Vendor Specific option. */
#define DHCP6_VENDOR_SPEC_ENT_OFFSET    4
#define DHCP6_VENDOR_SPEC_CODE_OFFSET   8
#define DHCP6_VENDOR_SPEC_LEN_OFFSET    10
#define DHCP6_VENDOR_SPEC_DATA_OFFSET   12

/* Code offsets for Interface ID option. */
#define DHCP6_INT_ID_OFFSET             4

/* Code offsets for Reconfigure Message option. */
#define DHCP6_RECON_TYPE_OFFSET         4
#define DHCP6_RECON_LEN_OFFSET          6
#define DHCP6_RECON_MSG_OFFSET          8

/* Code offsets for DNS Recursive Name Server option. */
#define DHCP6_DNS_SERVER_OFFSET         4

/* Code offsets for Domain Search list option. */
#define DHCP6_DOMAIN_LIST_OFFSET        4

/* Code offsets for Authentication option. */
#define DHCP6_AUTH_PROT_OFFSET          4
#define DHCP6_AUTH_ALG_OFFSET           5
#define DHCP6_AUTH_RDM_OFFSET           6
#define DHCP6_AUTH_REPLAY_OFFSET        7
#define DHCP6_AUTH_INFO_OFFSET          15

/* Option lengths. */
#define DHCP6_CLIENT_ID_OPT_LEN     4   /* Plus the length of the DUID */
#define DHCP6_SERVER_ID_OPT_LEN     4   /* Plus the length of the DUID */
#define DHCP6_DUID_LLT_LEN          8   /* Plus the length of the link-layer address */
#define DHCP6_DUID_EN_LEN           6   /* Plus the length of the identifier */
#define DHCP6_DUID_LL_LEN           4   /* Plus the length of the link-layer address */
#define DHCP6_IA_NA_OPT_LEN         16  /* Plus the length of the IA_NA options */
#define DHCP6_IA_NA_PAYLOAD_LEN     12
#define DHCP6_IA_ADDR_OPT_LEN       28  /* Plus the length of the IAddr-options */
#define DHCP6_OPT_REQUEST_OPT_LEN   4
#define DHCP6_RAPID_COMMIT_OPT_LEN  4

#define DHCP6_PREF_OPT_LEN          5
#define DHCP6_ELAPSED_TIME_OPT_LEN  6
#define DHCP6_AUTH_OPT_LEN          15  /* Plus the length of the authentication information */
#define DHCP6_STATUS_CODE_OPT_LEN   6   /* Plus the length of the status-message */
#define DHCP6_USER_CLASS_OPT_LEN    4   /* Plus the length of the user-class-data */
#define DHCP6_VENDOR_CLASS_OPT_LEN  4   /* Plus the length of the vendor-class-data */
#define DHCP6_VENDOR_SPEC_OPT_LEN   4   /* Plus the length of the option-data */
#define DHCP6_VENDOR_SPEC_OPQ_LEN   4
#define DHCP6_INT_ID_OPT_LEN        4   /* Plus the length of the interface ID */
#define DHCP6_RCNFGR_OPT_LEN        5
#define DHCP6_RCNFGR_ACC_OPT_LEN    4
#define DHCP6_DNS_SERVER_LEN        4   /* Plus the length of the IPv6 addresses */
#define DHCP6_DOMAIN_LIST_LEN       4   /* Plus the length of the search list */

/* Valid values in option length */
#define DHCP6_RECON_LEN_VALUE       1
#define DHCP6_PREF_LEN_VALUE        1
#define DHCP6_RECON_ACC_LEN_VALUE   0

/* Length of the ID in the DHCPv6 message transaction-id field. */
#define DHCP6_ID_LEN                3

/* Valid status code for the Status Code option. */
#define DHCP6_STATUS_SUCCESS        0
#define DHCP6_STATUS_UNSPEC_FAIL    1
#define DHCP6_STATUS_NO_ADDRS_AVAIL 2
#define DHCP6_STATUS_NO_BINDING     3
#define DHCP6_STATUS_NOT_ONLINK     4
#define DHCP6_STATUS_USE_MULTICAST  5

/* Internal error codes used for DHCPv6. */
#define DHCP6_UNSPEC_FAIL           -100
#define DHCP6_NO_ADDRS              -101
#define DHCP6_NO_BINDING            -102
#define DHCP6_NOT_ONLINK            -103
#define DHCP6_USE_MULTICAST         -104

/* Client flags. */
#define DHCP6_INC_CLIENT_ID         0x1
#define DHCP6_ALLOW_RAPID_COMMIT    0x2
#define DHCP6_SERVER_UNICAST        0x4
#define DHCP6_REBIND_EXPIRED        0x8
#define DHCP6_RECON_IN_PROGRESS     0x10
#define DHCP6_RECON_ACCEPT          0x20

#define DHCP6_VALID_OPT_TYPE(i, message_type) \
   (((i == DHCP6_OPT_DOMAIN_LIST) &&        \
     ((message_type == DHCP6_CONFIRM) ||    \
      (message_type == DHCP6_DECLINE) ||    \
      (message_type == DHCP6_RELEASE)) ) || \
    ((i == DHCP6_OPT_DNS_SERVERS) &&        \
     ((message_type == DHCP6_CONFIRM) ||    \
      (message_type == DHCP6_DECLINE) ||    \
      (message_type == DHCP6_RELEASE)) ) ? NU_FALSE : NU_TRUE)

/* dhcp6.c */
STATUS          DHCP6_Interpret(UINT8 *, UINT16, UINT8 *, UINT32);
DHCP6_TX_STRUCT *DHCP6_Find_TX_Struct_By_XID(UINT8 *);
DHCP6_STRUCT    *DHCP6_Find_Struct_By_ID(UINT8);
DHCP6_STRUCT    *DHCP6_Find_Struct_By_Dev(UINT32);
VOID            DHCP6_Do_Configuration_Exchange(DHCP6_STRUCT *, UINT32 *);
VOID            DHCP6_Commit_Options(DHCP6_STRUCT *, UINT32 *);
VOID            DHCP6_Event_Handler(TQ_EVENT, UNSIGNED, UNSIGNED);
DHCP6_STRUCT    *DHCP6_Create_Struct(VOID);
VOID            DHCP6_Free_TX_Struct(DHCP6_TX_STRUCT *);
STATUS          NU_Dhcp6(DHCP6_CLIENT *, UNSIGNED);
STATUS          NU_Dhcp6_Shutdown(UINT32, UINT8);
DHCP6_TX_STRUCT *DHCP6_Obtain_TX_Struct(VOID);

/* dhcp6_input.c */
STATUS DHCP6_Advertise_Input(UINT8 *, UINT16, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Reply_Input(UINT8 *, UINT16, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Reconfigure_Input(UINT8 *, UINT16, DHCP6_STRUCT *);

/* dhcp6_build_opts.c */
UINT16 DHCP6_Build_Client_ID_Option(UINT8 *);
UINT16 DHCP6_Build_Server_ID_Option(UINT8 *, DHCP6_STRUCT *);
UINT16 DHCP6_Build_IA_NA_Option(UINT8 *, UINT32, UINT32, UINT32, 
                                DHCP6_IA_ADDR_STRUCT *, INT);
UINT16 DHCP6_Build_IA_Addr_Option(UINT8 *, UINT8 *, UINT32, UINT32);
UINT16 DHCP6_Build_Option_Request_Option(UINT8 *, UINT8, UINT16 *);
UINT16 DHCP6_Build_Rapid_Commit_Option(UINT8 *);
UINT16 DHCP6_Build_Elapsed_Time_Option(UINT8 *, DHCP6_TX_STRUCT *);
UINT16 DHCP6_Build_Status_Code_Option(UINT8 *, CHAR *, UINT16);
UINT16 DHCP6_Build_User_Class_Option(UINT8 *, CHAR *, INT);
UINT16 DHCP6_Build_User_Class_Option(UINT8 *, CHAR *, INT);
UINT16 DHCP6_Build_Vendor_Class_Option(UINT8 *, UINT32, CHAR *, INT);
UINT16 DHCP6_Build_Vendor_Specific_Info_Option(UINT8 *, UINT32, UINT16, CHAR *);
UINT16 DHCP6_Build_Reconfigure_Accept_Option(UINT8 *);

/* dhcp6_parse_opts.c */
VOID DHCP6_Parse_User_Options(UINT8 *, UINT16, DHCP6_CLIENT_OPTS *);
STATUS DHCP6_Parse_Server_Options(UINT8 *, UINT16, DHCP6_OPT_STRUCT *, UINT8);
STATUS DHCP6_Parse_Client_ID_Option(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Parse_Server_ID_Option(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Parse_Reconfigure_Msg_Option(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Parse_Reconfigure_Accept_Option(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Parse_Status_Code_Option(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Parse_Preference_Option(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Parse_DNS_Server_Option(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Parse_IA_NA_Option(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Parse_IA_Addr_Option(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Parse_Option_Request_Option(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Parse_Server_Unicast_Option(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Parse_User_Class_Option(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);

/* dhcp6_output.c */
STATUS DHCP6_Find_Server(DHCP6_STRUCT *);
STATUS DHCP6_Solicit_Output(DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Confirm_Addresses(UINT32);
STATUS DHCP6_Confirm_Output(DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Renew_Addresses(UINT32);
STATUS DHCP6_Renew_Output(DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Rebind_Output(DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Rebind_Addresses(DHCP6_STRUCT *);
STATUS DHCP6_Obtain_Address(DHCP6_STRUCT *);
STATUS DHCP6_Request_Output(DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Obtain_Config_Info(DHCP6_STRUCT *);
STATUS DHCP6_Info_Request_Output(DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATUS DHCP6_Release_Output(DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
VOID   DHCP6_Release_Address(UINT8 *, UINT32);
VOID   DHCP6_Decline_Address(DEV6_IF_ADDRESS *);
STATUS DHCP6_Decline_Output(DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
VOID   DHCP6_Free_Address(DHCP6_STRUCT *);

#endif
