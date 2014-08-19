/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/************************************************************************
*
*   FILE NAME
*
*       wsox_int.h
*
*   COMPONENT
*
*       Nucleus WebSocket
*
*   DESCRIPTION
*
*       This file holds the WebSocket internal declarations.
*
*   DATA STRUCTURES
*
*       WSOX_CONTEXT_STRUCT
*       WSOX_CONTEXT_LIST
*
*   DEPENDENCIES
*
*       nu_networking.h
*
************************************************************************/

#include "networking/nu_networking.h"
#include "networking/nu_wsox.h"

#ifndef _WSOX_INT_H
#define _WSOX_INT_H

/* WSOX master task settings. */
#define WSOX_SVR_TIME_SLICE     0
#define WSOX_SVR_PREEMPT        NU_PREEMPT

/* Settings configured in the .metadata file located at os/networking/websocket */
#define WSOX_SVR_STACK_SIZE         CFG_NU_OS_NET_WSOX_SVR_STACK_SIZE
#define WSOX_SVR_PRIORITY           CFG_NU_OS_NET_WSOX_SVR_STACK_PRIO
#define WSOX_CLNT_TIMEOUT           (CFG_NU_OS_NET_WSOX_CLNT_TIMEOUT * TICKS_PER_SECOND)
#define WSOX_CLNT_CLOSE_TIMEOUT     (CFG_NU_OS_NET_WSOX_CLNT_CLOSE_TIMEOUT * TICKS_PER_SECOND)
#define WSOX_PING_MESSAGE           CFG_NU_OS_NET_WSOX_PING_MESSAGE
#define WSOX_ENABLE_HNDL_NOTIFY     CFG_NU_OS_NET_WSOX_RECV_HNDL_NOTIFY

#ifdef CFG_NU_OS_NET_SSL_LITE_ENABLE
#define WSOX_INCLUDE_CYASSL NU_TRUE
#else
#define WSOX_INCLUDE_CYASSL NU_FALSE
#endif

/* Set the HTTP glue layer routines according to the HTTP module
 * being used.
 */
#ifdef CFG_NU_OS_NET_HTTP_ENABLE

#include "os/networking/http/inc/http_lite_int.h"

#define WSOX_Find_Token             HTTP_Lite_Find_Token
#define WSOX_TOKEN_CASE_INSENS      HTTP_TOKEN_CASE_INSENS

#else
#error "HTTP Lite is required with the WebSocket module."
#endif

/* Header offsets. */
#define WSOX_FIN_RSV_OPCODE_OFFSET      0
#define WSOX_MSK_LEN_OFFSET             1
#define WSOX_EXT_MSK_LEN_OFFSET         2
#define WSOX_MASKING_KEY_OFFSET         2
#define WSOX_PAYLOAD_OFFSET             6
#define WSOX_MSKD_STATUS_CODE_OFFSET    6
#define WSOX_UNMSKD_STATUS_CODE_OFFSET  2

#define WSOX_16BIT_PAYLOAD_LEN          2
#define WSOX_64BIT_PAYLOAD_LEN          8
#define WSOX_STATUS_CODE_LEN            2

/* Fields in the HTTP header specific to the WebSocket protocol */
#define WSOX_ORIGIN             "Origin:"
#define WSOX_HOST               "Host:"
#define WSOX_UPGRADE            "Upgrade:"
#define WSOX_CONNECTION         "Connection:"
#define WSOX_SEC_KEY            "Sec-WebSocket-Key:"
#define WSOX_SEC_PROTOCOL       "Sec-WebSocket-Protocol:"
#define WSOX_SEC_VERSION        "Sec-WebSocket-Version:"
#define WSOX_SEC_ACCEPT         "Sec-WebSocket-Accept:"
#define WSOX_SEC_EXT            "Sec-WebSocket-Extensions:"
#define WSOX_VERSION            "13"
#define WSOX_SUCCESS_RESPONSE   "HTTP/1.1 101 Switching Protocols\r\n"
#define WSOX_UPGRADE_HDR        "Upgrade: websocket\r\n"
#define WSOX_CONNECTION_HDR     "Connection: upgrade\r\n"
#define WSOX_GUID_CONST         "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* The standard HTTP port number. */
#define WSOX_HTTP_PORT          80

/* These HTTP values are defined here so the WebSocket module can be more
 * easily integrated into any HTTP Server implementation.
 */
#define WSOX_PROTO_BAD_REQUEST      400
#define WSOX_PROTO_UPDATE_REQUIRED  426
#define WSOX_PROTO_FORBIDDEN        403
#define WSOX_PROTO_NOT_FOUND        404
#define WSOX_PROTO_SERVER_ERROR     500
#define WSOX_PROTO_SRVC_UNAVAILABLE 503
#define WSOX_PROTO_SUCCESS          101

#define WSOX_BAD_REQUEST        "Bad Request"
#define WSOX_UPDATE_REQUIRED    "Upgrade Required"
#define WSOX_FORBIDDEN          "Forbidden"
#define WSOX_NOT_FOUND          "Not Found"
#define WSOX_SERVER_ERROR       "Internal Server Error"
#define WSOX_SRVC_UNAVAILABLE   "Service Unavailable"

/* Server side connection response fixed lengths. */
#define WSOX_GUID_CONST_LEN         36  /* The GUID length is fixed. */
#define WSOX_SRV_KEY_LEN            30  /* The base-64 encoded key will be 28 bytes since the hash
                                           value is always 20 bytes in length (plus two bytes for \r\n) */
#define WSOX_SRV_RSP_FIX_HDR_LEN    96  /* The remainder of the header. */
#define WSOX_HTTP_STAT_MSG_FIX_LEN  9   /* The length of the fixed part of an HTTP status message. */
#define WSOX_CLNT_RQST_FIX_HDR_LEN  109 /* The fixed portion of the client request. */
#define WSOX_CLNT_KEY_LEN           26  /* The base-64 encoded key will be 24 bytes (plus two bytes for \r\n) */

/* Length for the fixed header, plus the base-64 encoded accept key plus
 * the terminating control characters for a server response.
 */
#define WSOX_SVR_RSP_CONST_LEN      (WSOX_SRV_RSP_FIX_HDR_LEN + WSOX_SRV_KEY_LEN + 2)

/* The maximum number of bytes that can be sent or received with a single
 * call to the networking stack.
 */
#define WSOX_MAX_DATA_IO            65535

/* The maximum length of a WebSocket header. */
#define WSOX_MAX_HDR_LEN            16      /* The maximum WebSocket header length is 14, but set this
                                             * value to 16 to include the close status code and to
                                             * maintain proper alignment.
                                             */
#define WSOX_MIN_HDR_LEN            2       /* The smallest length of a WebSocket header. */
#define WSOX_MASK_LEN               4
#define WSOX_END_CTRL_LEN           4       /* The length of \r\n\r\n found in the end of the HTTP header. */

/* Internal flags. */
#define WSOX_LISTENER               0x1     /* This handle is a listener. */
#define WSOX_SECURE                 0x2     /* This handle uses SSL. */
#define WSOX_PARTIAL_HDR            0x4     /* The connection is waiting for the rest of the WebSocket
                                             * header associated with the current frame.
                                             */
#define WSOX_PARTIAL_CLOSE          0x8     /* The connection is waiting for the rest of the WebSocket
                                             * close data associated with the current frame.
                                             */
#define WSOX_LOCAL_HANDLE           0x10    /* The handle was created by the local node. */
#define WSOX_TX                     0x20    /* The TX side of the connection is open. */
#define WSOX_RX                     0x40    /* The RX side of the connection is open. */
#define WSOX_CLIENT                 0x80    /* The handle is a client. */
#define WSOX_PING                   0x100   /* The handle is sending periodic PINGs. */
#define WSOX_NEW_CNXN               0x200   /* An onopen() call has not been made for this handle yet. */
#define WSOX_NOTIFY_RX              0x400   /* Used by the application to toggle receive calls on the handle
                                             * to the application.
                                             */
#define WSOX_CERT_PEM_PTR           0x800
#define WSOX_CERT_DER_PTR           0x1000
#define WSOX_CERT_FILE              0x2000
#define WSOX_VERIFY_PEER            0x4000
#define WSOX_KEY_FILE               0x8000
#define WSOX_KEY_PEM_PTR            0x10000
#define WSOX_KEY_DER_PTR            0x20000
#define WSOX_CA_FILE                0x40000
#define WSOX_CA_PEM_PTR             0x80000
#define WSOX_CA_DER_PTR             0x100000
#define WSOX_NO_DOMAIN_CHECK        0x200000
#define WSOX_OPEN                   (WSOX_TX | WSOX_RX) /* The endpoint can send and receive data. */

#define WSOX_IS_LOCAL_CLIENT(flag)  \
    ((flag & WSOX_LOCAL_HANDLE) && (!(flag & WSOX_LISTENER)))

/* RFC 6455 - section 5.5 - All control frames MUST have a payload length of 125
 * bytes or less.
 */
#define WSOX_MAX_CTRL_FRAME_LEN     125

typedef struct _WSOX_CLIENT_STRUCT
{
    struct _WSOX_CLIENT_STRUCT  *flink;
    struct _WSOX_CLIENT_STRUCT  *blink;
    NU_TASK                     *task_ptr;
    struct addr_struct          foreign_addr;
} WSOX_CLIENT_STRUCT;

typedef struct _WSOX_SERVER_STRUCT
{
    CHAR    *host;              /* The host part of the HTTP URI used to connect to this service. */
    CHAR    *resource;          /* The resource name part of the HTTP URI used to access this service. */
    CHAR    *protocols;         /* List of application-level protocols layered over the WebSocket protocol that will be serviced by the same service routine, in order of preference. */
    CHAR    *origins;           /* For clients, the origin host name to be sent in connection requests.  For servers, a list of origins to deny connection to the service or NU_NULL to allow any origin to connect. */
    CHAR    *certificate;       /* The SSL certificate to use for the connection. */
    CHAR    *extensions;        /* The optional protocol level extensions. */
    UINT16  connections;        /* The remaining number of allowable simultaneous connections. */
    UINT8   padN[2];
} WSOX_SERVER_STRUCT;

typedef struct _WSOX_CONTEXT_STRUCT
{
    struct _WSOX_CONTEXT_STRUCT     *flink;
    struct _WSOX_CONTEXT_STRUCT     *blink;
    VOID            (*onopen)(UINT32 handle, struct addr_struct *foreign_addr); /* Invoked when the WebSocket transitions to the OPEN state. */
    VOID            (*onmessage)(UINT32 handle, WSOX_MSG_INFO *msg_info);  /* Invoked when a message is received on the WebSocket. */
    VOID            (*onerror)(UINT32 handle, STATUS error, CHAR *reason); /* Invoked when an error message has been received on the WebSocket. */
    VOID            (*onclose)(UINT32 handle, STATUS error);               /* Invoked when the WebSocket transitions to the CLOSED state. */
    WSOX_SERVER_STRUCT              *wsox_server;
    VOID                            *ssl;
#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    SSL_CTX                         *ctx;
#endif
    CHAR                            frame_mask[WSOX_MASK_LEN];  /* Since the mask can occur at 3 different
                                                                 * byte positions, copy the mask when we find
                                                                 * it for quick access.
                                                                 */
    CHAR                            header[WSOX_MAX_HDR_LEN];   /* Since the header could span multiple
                                                                 * segments and multiple receive buffers,
                                                                 * copy the header for quick access.
                                                                 */
    CHAR                            utf8_bytes[4];
    CHAR                            *close_reason;
    UINT64                          frame_bytes_left;           /* If the frame spans multiple receive
                                                                 * buffers, this is the number of bytes
                                                                 * left to be received for a frame.
                                                                 */
    UINT64                          payload_len;
    UINT32                          user_handle;
    UINT32                          timestamp;
    UINT32                          pong_timestamp;
    UINT32                          ping_interval;              /* The interval at which to transmit PING
                                                                 * frames to the other side of the connection.
                                                                 */
    UINT32                          retrans_interval;           /* The delay between unanswered retransmitted
                                                                 * PING frames.
                                                                 */
    UINT32                          flag;
    INT                             socketd;                    /* The socket over which communications
                                                                 * take place with the foreign side.
                                                                 */
    UINT8                           max_retrans_count;          /* The number of unanswered PING frames to
                                                                 * retransmit before invoking onerror().
                                                                 */
    UINT8                           retrans_count;              /* The number of outstanding retransmissions. */
    UINT8                           fragment_opcode;            /* The opcode of the current fragmented
                                                                 * frame.
                                                                 */
    UINT8                           frame_opcode;               /* Save the opcode in case the frame
                                                                 * spans multiple receive buffers.
                                                                 * fragment_opcode and frame_opcode are
                                                                 * both needed since a control frame can
                                                                 * be inserted in the middle of a fragmented
                                                                 * stream.  If the control frame spans
                                                                 * multiple receive buffers, we would not
                                                                 * want to overwrite the fragment_opcode
                                                                 * with the control frame's opcode.
                                                                 */
    UINT8                           cli_frag_opcode;            /* The opcode for the fragmented client frame
                                                                 * being built.
                                                                 */
    UINT8                           utf8_byte_count;
    UINT8                           header_len;
    UINT8                           close_len;
    CHAR                            close_status[2];
    UINT8                           padN[2];
} WSOX_CONTEXT_STRUCT;

typedef struct _WSOX_CONTEXT_LIST
{
    struct _WSOX_CONTEXT_STRUCT *head;
    struct _WSOX_CONTEXT_STRUCT *tail;
} WSOX_CONTEXT_LIST;

typedef struct _WSOX_PENDING_CLIENTS
{
    WSOX_CLIENT_STRUCT     *head;
    WSOX_CLIENT_STRUCT     *tail;
} WSOX_PENDING_CLIENTS;

WSOX_CONTEXT_STRUCT *WSOX_Create_Context(NU_WSOX_CONTEXT_STRUCT *user_ptr, STATUS *status);
WSOX_CONTEXT_STRUCT *WSOX_Find_Listener_Context(CHAR *resource, CHAR *origin, STATUS *status);
STATUS WSOX_Process_Connection_Request(WSOX_CONTEXT_STRUCT *wsox_listener,
                                       NU_WSOX_CONTEXT_STRUCT *wsox_client, INT socketd,
                                       CHAR *keys, VOID *ssl_ptr);
BOOLEAN WSOX_Search_List(CHAR *list, CHAR *target);
VOID WSOX_Master_Task(UNSIGNED argc, VOID *argv);
STATUS WSOX_Resume_Server(VOID);
BOOLEAN WSOX_Validate_UTF8(CHAR *s, UINT64 length, UINT64 *byte);
STATUS WSOX_Create_Accept_Key(CHAR *key, INT key_len, CHAR *buffer,
                              INT *buf_len);
CHAR *WSOX_Parse_Header_Field(CHAR *buf_ptr, INT buf_len, CHAR *target, STATUS *status);
BOOLEAN WSOX_Compare_Protocol_Lists(CHAR *list1, CHAR *list2, BOOLEAN prune);
STATUS WSOX_Setup_Recv_Handle(WSOX_CONTEXT_STRUCT *wsox_ptr);
STATUS WSOX_Create_Client_Handle(NU_WSOX_CONTEXT_STRUCT *context_ptr, UINT32 *user_handle);
STATUS WSOX_Create_Server_Handle(NU_WSOX_CONTEXT_STRUCT *context_ptr, UINT32 *user_handle);
VOID WSOX_Compress_Whitespace(CHAR *string_ptr);
WSOX_CONTEXT_STRUCT *WSOX_Find_Context_By_Handle(UINT32 handle);
STATUS WSOX_TX_Close(INT socketd, BOOLEAN mask, UINT16 status_code, CHAR *reason,
                     VOID *ssl_ptr);
VOID WSOX_Build_Header(WSOX_CONTEXT_STRUCT *wsox_ptr, CHAR *buffer, INT opcode,
                       UINT64 data_len, BOOLEAN mask, UINT8 hdr_len, UINT8 flags);
VOID WSOX_Mask_Data(CHAR *dest, CHAR *src, CHAR *mask, UINT64 len, UINT64 encode_count);
VOID WSOX_Cleanup_Connection_Entry(WSOX_CONTEXT_STRUCT *wsox_ptr);
STATUS WSOX_Send_Frame(WSOX_CONTEXT_STRUCT *wsox_ptr, CHAR *buffer, UINT64 *data_len,
                       INT opcode, UINT8 flags, UINT64 byte_position, CHAR *mask);
INT32 WSOX_Recv(INT socketd, CHAR *buffer, INT32 len, VOID *ssl_ptr);
INT32 WSOX_Send(INT socketd, CHAR *buf_ptr, INT32 len, VOID *ssl_ptr);
UINT32 WSOX_Check_For_Data(INT socketd, VOID *ssl_ptr);

#endif /* _WSOX_INT_H */
