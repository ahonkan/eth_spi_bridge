/*************************************************************************
*
*            Copyright Mentor Graphics Corporation 2012
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
*      sntpc_int.h
*
* DESCRIPTION
*
*       This include file defines Nucleus SNTP client internal functions.
*
* DATA STRUCTURES
*
*       SNTPC_SERVER_LIST
*       SNTPC_SERVER_LIST_HEAD
*       SNTPC_CONFIG_STRUCT
*       SNTPC_TIMESTAMP
*       SNTPC_PACKET
*
* DEPENDENCIES
*
*      nu_net.h
*
****************************************************************************/
#ifndef SNTPC_INT_H
#define SNTPC_INT_H

#include "networking/nu_net.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Constant Definitions. */
#define SNTPC_UDP_PORT              123
#define SNTPC_SECONDS_1900_1970     2208988800UL
#define SNTPC_SECS_PER_MIN          60
#define SNTPC_01_01_2012            3534364800UL

/* Timeout in seconds to wait for a reply from the SNTP servers. */
#define SNTPC_RECV_TIMEOUT          10

/* Duration for re-attempting to resolve an unresolved SNTP Server
 * hostname, in seconds. */
#define SNTPC_REATTEMPT_RESOLVE_TIME 10

/* Minimum and maximum SNTP packet length. */
#define SNTPC_MIN_PACKET_LENGTH     48
#define SNTPC_MAX_PACKET_LENGTH     (SNTPC_MIN_PACKET_LENGTH + 20)

#define SNTPC_REG_OPT_MAX_SERVERS   "/max_servers"

/* Server update task defines. */
#define SNTPC_TASK_STACK_SIZE       2000
#define SNTPC_TASK_PRIO             25
#define SNTPC_TASK_TIME_SLICE       0

/* The version of the SNTP standard to which this product conforms. */
#define SNTPC_VERSION               4

/* The NTP mode for the client */
#define SNTPC_MODE                  3

/* The stratum value for client */
#define SNTPC_STRATUM               0x0F

/* Length of the Reference ID field in the packet. */
#define SNTPC_REF_ID_LENGTH         4

/* Offset of different fields within the SNTP packet. */
#define SNTPC_ROOT_DELAY_OFFSET     4
#define SNTPC_ROOT_DISP_OFFSET      8
#define SNTPC_REF_ID_OFFSET         12
#define SNTPC_REF_TS_OFFSET         16
#define SNTPC_ORIG_TS_OFFSET        24
#define SNTPC_RX_TS_OFFSET          32
#define SNTPC_TX_TS_OFFSET          40
#define SNTPC_KEY_ID_OFFSET         48
#define SNTPC_MSG_DIGEST_OFFSET     52

/* This structure is used to maintain the SNTP Timestamp. Note that
 * this is different from the "SNTP_TIME" structure because that contains
 * fractional time in microseconds whereas this structure contains
 * fractional time in fixed point format. This format is used within
 * SNTP packets.
 */
typedef struct _sntpc_timestamp
{
    UINT32          sntpc_seconds;      /* Seconds. */
    UINT32          sntpc_fraction;     /* Fractional seconds. */
} SNTPC_TIMESTAMP;

/* This structure contains data contained in incoming and outgoing
 * SNTP packets. It only contains the fields which we are interested in.
 */
typedef struct _sntpc_packet
{
    SNTPC_TIMESTAMP sntpc_ref_ts;       /* Reference timestamp. */
    SNTPC_TIMESTAMP sntpc_orig_ts;      /* Originate timestamp. */
    SNTPC_TIMESTAMP sntpc_rx_ts;        /* Receive timestamp. */
    SNTPC_TIMESTAMP sntpc_tx_ts;        /* Transmit timestamp. */
    UINT32          sntpc_root_delay;   /* Root delay field. */
    UINT32          sntpc_root_disp;    /* Root dispersion. */

    UINT8           sntpc_leap;         /* Leap indicator. */
    UINT8           sntpc_version;      /* SNTP Version. */
    UINT8           sntpc_mode;         /* SNTP mode. */
    UINT8           sntpc_stratum;      /* Stratum number. */
    UINT8           sntpc_poll;         /* Poll interval. */
    UINT8           sntpc_precision;    /* Time precision. */

    /* Reference ID. An extra byte is added to null-terminate the
     * ASCII sequence of characters in case of stratum 0 and 1. */
    UINT8           sntpc_ref_id[SNTPC_REF_ID_LENGTH + 1];

    UINT8           sntpc_pad[1];
} SNTPC_PACKET;

/* This structure is used to store information related to a single
 * SNTP Server (i.e. Time Source).
 *
 * Important: Do not modify the first two members of this structure.
 *            Their order is required by the DLL list component.
 */
typedef struct _sntpc_server_list
{
    struct _sntpc_server_list    *sntpc_next; /* DLL front link. */
    struct _sntpc_server_list    *sntpc_prev; /* DLL back link. */
    SNTPC_SERVER    sntpc_server;           /* Static server data */
    SNTPC_TIMESTAMP sntpc_last_server_time; /* Last received timestamp. */
    NU_TIMER        sntpc_timer;            /* Polling timer */
    SNTPC_PACKET    sntpc_last_request;     /* Last request sent to server. */
    UINT64          sntpc_delay;            /* Round-trip delay between the
                                             * client and server. */
    UINT64          sntpc_offset;           /* Maximum-likelihood time offset
                                             * of the server clock relative to
                                             * the system clock. */
    UINT32          sntpc_last_plus_ticks;  /* Ticks at last timestamp. */
    BOOLEAN         sntpc_user_defined;     /* TRUE: Server was added manually;
                                             * FALSE: Server was discovered on
                                             * the network. */
    BOOLEAN         sntpc_synced;           /* Flag to specify if time has
                                             * been synchronized with the
                                             * server. */
    BOOLEAN         sntp_poll_enabled;      /* TRUE: Polling enabled;
                                             * FALSE: Polling disabled. */
    BOOLEAN         sntpc_hostname_resolved;/* Hostname of server has been
                                             * resolved to an IP address.*/
    UINT8           sntpc_stratum;          /* Hierarchical designation. */

    UINT8           sntpc_pad[3];
} SNTPC_SERVER_LIST;

/* List of SNTP Servers. */
typedef struct _sntpc_server_list_head
{
    SNTPC_SERVER_LIST   *sntpc_head;
    SNTPC_SERVER_LIST   *sntpc_tail;
    SNTPC_SERVER_LIST   *sntpc_owp_best;    /* Most precise time server as
                                               determined by on-wire protocol */
    UINT16              sntpc_max_servers;  /* Maximum number of servers */
    UINT16              sntpc_count;        /* Number of items in the list. */
} SNTPC_SERVER_LIST_HEAD;

/* This structure is used to keep track of the client configurations. */
typedef struct _sntpc_config_struct
{
    NU_SEMAPHORE    sntpc_semaphore;        /* Semaphore to protect data. */
    SNTPC_SERVER_LIST_HEAD sntpc_server_list; /* Database of SNTP servers. */
    NU_MEMORY_POOL  *sntpc_memory;          /* Memory pool used by SNTP. */
    NU_TASK         *sntpc_server_update_task;/* Task to sync with servers. */
    NU_QUEUE        sntpc_send_request_queue; /* Queue to send requests. */
    VOID            *sntpc_queue_mem;       /* Memory for the queue. */
    INT             sntpc_timezone;         /* Current timezone in minutes. */
    INT             sntpc_dst_enabled;      /* Flag for daylight saving. */
    INT             sntpc_socket;           /* UDP socket for SNTP. */
} SNTPC_CONFIG_STRUCT;

/* Global variables. */
extern SNTPC_CONFIG_STRUCT *SNTPC_Config;

/* Internal Function declarations. */
STATUS SNTPC_Init_On_First_Server(VOID);
STATUS SNTPC_Deinit_On_Last_Server(VOID);
STATUS SNTPC_Server_List_Query(struct addr_struct *addr,
                               CHAR *server_hostname,
                               SNTPC_SERVER_LIST **ret_server);
STATUS SNTPC_Send_Request(SNTPC_SERVER_LIST *server, SNTPC_PACKET *pkt);
STATUS SNTPC_Decode_Reply(UINT8 *buffer, INT buffer_len, SNTPC_PACKET *pkt);
STATUS SNTPC_Get_Timestamp_From_Server(SNTPC_TIMESTAMP *current_time,
                                       SNTPC_SERVER_LIST *server);
VOID   SNTPC_Send_Request_Handler(UNSIGNED ext_dat);
STATUS SNTPC_Generate_Request(SNTPC_SERVER_LIST *server);
VOID   SNTPC_On_Wire_Protocol(SNTPC_SERVER_LIST *server, SNTPC_PACKET *svr_packet);

#ifdef          __cplusplus
}

#endif /* _cplusplus */
#endif /* SNTPC_INT_H */



