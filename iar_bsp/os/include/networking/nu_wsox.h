/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2012
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
*       nu_wsox.h
*
*   COMPONENT
*
*       Nucleus WebSocket
*
*   DESCRIPTION
*
*       This file includes all required header files for the WebSocket
*       component.
*
*   DATA STRUCTURES
*
*       WSOX_MSG_INFO
*       NU_WSOX_CONTEXT_STRUCT
*
*   FILE DEPENDENCIES
*
*
*************************************************************************/

#ifndef _NU_WSOX_H
#define _NU_WSOX_H

/* Error codes. */
#define WSOX_BAD_ORIGIN     -12050
#define WSOX_NO_RESOURCE    -12051
#define WSOX_END_OF_LIST    -12052
#define WSOX_PARSE_ERROR    -12053
#define WSOX_MAX_CONXNS     -12054
#define WSOX_INVALID_HOST   -12055
#define WSOX_TIMEOUT        -12056
#define WSOX_CNXN_ERROR     -12057
#define WSOX_PKT_ERROR      -12058
#define WSOX_PROTOCOL_ERROR -12059
#define WSOX_INVALID_HANDLE -12060
#define WSOX_INVALID_OPCODE -12061
#define WSOX_MSG_TOO_BIG    -12062
#define WSOX_PING_TIMED_OUT -12063
#define WSOX_DUP_REQUEST    -12064
#define WSOX_DECODE_ERROR   -12065
#define WSOX_INVALID_ACCESS -12066
#define WSOX_NO_HANDLES     -12067
#define WSOX_BUF_TOO_SMALL  -12068
#define WSOX_SSL_ERROR      -12069

/* Application level flags for NU_WSOX_CONTEXT_STRUCT. */
#define NU_WSOX_LISTENER        0x1     /* This handle is a listener. */
#define NU_WSOX_SECURE          0x2     /* Use SSL to secure this connection. */
#define NU_WSOX_CERT_PEM_PTR    0x4     /* The certificate points to a PEM certificate in FLASH memory. */
#define NU_WSOX_CERT_DER_PTR    0x8     /* The certificate points to a DER certificate in FLASH memory. */
#define NU_WSOX_CERT_FILE       0x10    /* The certificate is stored on a local drive. */
#define NU_WSOX_KEY_FILE        0x20    /* The key sent to the foreign server is stored on a local drive. */
#define NU_WSOX_KEY_PEM_PTR     0x40    /* The key sent to the foreign server points to a PEM file in FLASH memory. */
#define NU_WSOX_KEY_DER_PTR     0x80    /* The key sent to the foreign server points to a DER certificate in FLASH memory. */
#define NU_WSOX_CA_PEM_PTR      0x100   /* The CA list points to a PEM certificate in FLASH memory. */
#define NU_WSOX_CA_DER_PTR      0x200   /* The CA list points to a DER certificate in FLASH memory. */
#define NU_WSOX_CA_FILE         0x400   /* The CA list is stored on a local drive. */
#define NU_WSOX_VERIFY_PEER     0x800   /* Verify the foreign server certificate when connecting via a client connection. */
#define NU_WSOX_NO_DOMAIN_CHECK 0x1000  /* Do not check the domain before connecting via SSL. */

/* Application level flags for NU_WSOX_Send. */
#define NU_WSOX_FRAGMENT    0x1 /* This frame is part of a fragmented message. */
#define NU_WSOX_NO_HDR      0x2 /* Internal use only - do not include header. */


/* Valid opcode fields. */
#define WSOX_CONT_FRAME             0
#define WSOX_TEXT_FRAME             0x1
#define WSOX_BINARY_FRAME           0x2
#define WSOX_CLOSE_FRAME            0x8
#define WSOX_PING_FRAME             0x9
#define WSOX_PONG_FRAME             0xA

/* WebSocket status codes per RFC 6455. */
#define WSOX_NORMAL                 1000        /* Normal closure. */
#define WSOX_GOING_DOWN             1001        /* The endpoint is going down. */
#define WSOX_PROTO_ERROR            1002        /* Protocol error. */
#define WSOX_UNRECOGNIZED_DATA      1003        /* Does not recognize data type. */
#define WSOX_NO_STATUS_RCVD         1005        /* No status code included in the close. */
#define WSOX_ABNORMAL_CLOSURE       1006        /* No close frame sent during close. */
#define WSOX_BAD_DATA               1007        /* The received data is not consistent with the type of data. */
#define WSOX_POLICY_VIOLATION       1008        /* A message has violated the local policy. */
#define WSOX_MESSAGE_TOO_BIG        1009        /* A message is too big to process. */
#define WSOX_MISSING_EXTENSIONS     1010        /* An extension was expected. */
#define WSOX_UNEXPECTED_COND        1011        /* An unexpected condition was encountered. */
#define WSOX_TLS_FAILURE            1015        /* Failure to perform TLS handshake. */

typedef struct _WSOX_MSG_INFO
{
    UINT64  data_len;
    INT     opcode;
    INT     flags;
} WSOX_MSG_INFO;

typedef struct _NU_WSOX_CONTEXT_STRUCT
{
    VOID    (*onopen)(UINT32 handle, struct addr_struct *foreign_addr); /* Invoked when the WebSocket transitions to the OPEN state. */
    VOID    (*onmessage)(UINT32 handle, WSOX_MSG_INFO *msg_info);  /* Invoked when a message is received on the WebSocket. */
    VOID    (*onerror)(UINT32 handle, STATUS error, CHAR *reason); /* Invoked when an error message has been received on the WebSocket. */
    VOID    (*onclose)(UINT32 handle, STATUS error);               /* Invoked when the WebSocket transitions to the CLOSED state. */
    CHAR    *host;              /* The host part of the HTTP URI used to connect to this service. */
    CHAR    *resource;          /* The resource name part of the HTTP URI used to access this service. */
    CHAR    *protocols;         /* List of application-level protocols layered over the WebSocket protocol that will be serviced by the same service routine, in order of preference. */
    CHAR    *origins;           /* For clients, the origin host name to be sent in connection requests.  For servers, a list of origins to deny connection to the service or NU_NULL to allow any origin to connect. */
    CHAR    *ssl_ca;            /* The SSL CA to use for the client connection. */
    CHAR    *ssl_cert;          /* The certificate used if the server requests a certificate from the client. */
    CHAR    *ssl_key;           /* The key used if the server requests a certificate from the client. */
    CHAR    *extensions;        /* The optional protocol level extensions. */
    UINT32  flag;               /* Flags for the WebSocket handle. */
    INT     ssl_ca_size;
    INT     ssl_cert_size;
    INT     ssl_key_size;
    INT     port;               /* The port to connect to if we initiate the connection. */
    UINT16  max_connections;    /* The maximum number of connections a server can have active at one time. */
    UINT8   padN[2];
} NU_WSOX_CONTEXT_STRUCT;

STATUS  NU_WSOX_Create_Context(NU_WSOX_CONTEXT_STRUCT *context_ptr, UINT32 *user_handle);
STATUS  NU_WSOX_Process_Upgrade_Request(INT method, CHAR *uri, CHAR *buf_ptr, INT buf_len,
                                        INT socketd, VOID *ssl);
STATUS  NU_WSOX_Accept(NU_WSOX_CONTEXT_STRUCT *context_ptr, INT socketd, CHAR *keys,
                       VOID *ssl);
STATUS NU_WSOX_Send(UINT32 handle, CHAR *buffer, UINT64 *data_len, INT opcode,
                    UINT8 flags);
STATUS NU_WSOX_Close(UINT32 handle, UINT16 status_code, CHAR *reason);
STATUS NU_WSOX_Schedule_Ping(UINT32 handle, UINT32 interval, UINT32 delay, UINT8 retrans_count);
STATUS NU_WSOX_Toggle_Recv(UINT32 handle, BOOLEAN enable);
STATUS NU_WSOX_Recv(UINT32 handle, CHAR *buffer, UINT64 *data_len);
STATUS NU_WSOX_Setsockopt(UINT32 handle, INT level, INT optname, VOID *optval,
                          INT optlen);
STATUS NU_WSOX_Getsockopt(UINT32 handle, INT level, INT optname, VOID *optval,
                          INT *optlen);

#endif /* _NU_WSOX_H */
