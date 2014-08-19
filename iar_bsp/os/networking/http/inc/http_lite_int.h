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

/************************************************************************
*
*   FILE NAME
*
*     http_lite_int.h
*
*   COMPONENT
*
*     Nucleus HTTP Lite
*
*   DESCRIPTION
*
*     This file holds the HTTP Lite internal declarations.
*
*   DATA STRUCTURES
*
*       querymode
*       HTURI
*
*   DEPENDENCIES
*
*     nucleus.h
*     nu_networking.h
*     http_lite.h
*     reg_api.h
*     ctype.h
*
************************************************************************/

#ifndef _HTTP_LITE_INT_H
#define _HTTP_LITE_INT_H

#include "networking/nu_networking.h"
#include "networking/nu_http_lite.h"

#ifdef CFG_NU_OS_NET_WEB_ENABLE
#if (CFG_NU_OS_NET_HTTP_SERVER_ENABLE == NU_TRUE)
#error "Nucleus WebServ and HTTP Lite Server cannot be simultaneously enabled in the build."
#endif
#endif

/* HTTP Lite server task settings. */
#define HTTP_SVR_TIME_SLICE     0
#define HTTP_SVR_PREEMPT        NU_PREEMPT

/* Settings configured in the .metadata file located at os/networking/http */
#define HTTP_SVR_STACK_SIZE     CFG_NU_OS_NET_HTTP_SVR_STACK_SIZE
#define HTTP_SVR_PRIORITY       CFG_NU_OS_NET_HTTP_SVR_STACK_PRIO
#define HTTP_SSL_SVR_STACK_SIZE CFG_NU_OS_NET_HTTP_SSL_SVR_STACK_SIZE
#define HTTP_SVR_BACKLOG        CFG_NU_OS_NET_HTTP_SVR_BACKLOG
#define HTTP_TIMEOUT            CFG_NU_OS_NET_HTTP_TIMEOUT
#define HTTP_SVR_RCV_SIZE       CFG_NU_OS_NET_HTTP_SVR_RCV_SIZE
#define HTTP_DEFAULT_MIME       CFG_NU_OS_NET_HTTP_SVR_DEFAULT_MIME
#define HTTP_DEFAULT_URI        CFG_NU_OS_NET_HTTP_SVR_DEFAULT_URI
#define HTTP_URI_LEN            CFG_NU_OS_NET_HTTP_SVR_URI_LEN
#define HTTP_CLI_MAXBUF         CFG_NU_OS_NET_HTTP_CLI_RCV_SIZE
#define HTTP_PUT_FILE           CFG_NU_OS_NET_HTTP_INCLUDE_PUT_FILE
#define HTTP_DELETE_FILE        CFG_NU_OS_NET_HTTP_INCLUDE_DELETE_FILE
#define HTTP_GET_FILE           CFG_NU_OS_NET_HTTP_INCLUDE_GET_FILE

/* The port numbers over which data is received. */
#define HTTP_SVR_PORT           80
#define HTTP_SSL_PORT           443

/* Internal macro. */
#define HTTP_TOKEN_CASE_INSENS  1

/* Internal flags. */
#define HTTP_HDR_CLOSE          0x1
#define HTTP_UPGRADE_REQ        0x2 /* An upgrade request was successfully process on the socket. */

#define HTTP_11_STRING          "HTTP/1.1"
#define HTTP_VER_11             11
#define HTTP_USER_AGENT         "nuc_http_lite/1"
#define HTTP_SECURE             "https"

/* HTTP characters. */
#define HTTP_CR                 '\r'            /* Carriage Return */
#define HTTP_LF                 '\n'            /* Line Feed */
#define HTTP_CRLF               "\r\n"          /* End of header property */
#define HTTP_CRLFCRLF           "\r\n\r\n"      /* End of Entity Header Message */
#define HTTP_CRLFCRLF_LEN       (sizeof(WS_CRLFCRLF) - 1) /* Don't count null terminator */

/* HTTP Tokens. */
#define HTTP_GET                224
#define HTTP_POST               242
#define HTTP_PUT                249
#define HTTP_DELETE             213

/* HTTP output headers */
#define HTTP_CONTENT_LENGTH     "Content-Length: "
#define HTTP_CONTENT_TYPE       "Content-Type: "
#define HTTP_CONNECTION         "Connection: "
#define HTTP_OK                 "OK"
#define HTTP_NOT_FOUND          "Not Found"
#define HTTP_CREATED            "Created"
#define HTTP_SERVER_ERROR       "Internal Server Error"
#define HTTP_NOT_SUPPORTED      "HTTP Version not supported"
#define HTTP_NOT_IMPLEMENTED    "Not Implemented"
#define HTTP_URI_TOO_LONG       "Request-URI Too Long"
#define HTTP_CHUNKED            "chunked"
#define HTTP_SEMICOLON          ";"
#define HTTP_TRANSFER_ENCODING  "Transfer-Encoding: "
#define HTTP_UPGRADE            "Upgrade: "
#define HTTP_BAD_REQUEST        "Bad Request"
#define HTTP_FORBIDDEN          "Forbidden"

/* Define the query mode enumerated type used by HTTP_Lite_Client_Query to
 * determine if the connection with the server should be maintained.
 */
typedef enum
{
    HTTP_CLOSE,            /* Close the socket after the query (for PUT, etc.) */
    HTTP_KEEP_OPEN         /* Keep it open (for GET)*/
} HTTP_QUERYMODE;


/* Define structure to hold components of URI after parsing with
 * HTTP_Lite_Parse_URL.
 */
typedef struct _HTURI {
    CHAR *access;       /* Now known as "scheme" */
    CHAR *host;
    CHAR *absolute;
} HTURI;

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
#define HTTP_LITE_SVR_SSL_STRUCT    HTTP_Session->ssl
#else
#define HTTP_LITE_SVR_SSL_STRUCT    NU_NULL
#endif

typedef struct _http_file_struct
{
    struct _http_file_struct    *flink;
    struct _http_file_struct    *blink;
    UINT32                      http_flen;
    CHAR                        *http_fname;        /* Don't re-order this structure. */
    CHAR                        *http_fptr;
} HTTP_FILE_STRUCT;

typedef struct _http_file_list
{
    HTTP_FILE_STRUCT    *flink;
    HTTP_FILE_STRUCT    *blink;
} HTTP_FILE_LIST;

typedef struct _nu_http_sh_s
{
    struct _nu_http_sh_s    *flink;
    struct _nu_http_sh_s    *blink;
    INT                     socketd;
    NU_SEMAPHORE            lock;
    INT32                   id;
    CHAR                    *header;
    UINT32                  num_remaining; /* bytes remaining on the socket */
    UINT16                  flag;
    UINT8                   padn[2];
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
    VOID                    *ssl_tsk_memory;
    SSL                     *ssl;
    SSL_CTX                 *ctx;
    NU_HTTP_SSL_STRUCT      *ssl_struct;
#endif
} NU_HTTP_SH_S;

/* Handle flag values */
#define HTTP_SH_FLAG_CHUNKED 0x01
#define HTTP_SH_FLAG_CLOSE   0x02

typedef struct _http_sh_s_list
{
    NU_HTTP_SH_S    *flink;
    NU_HTTP_SH_S    *blink;
} HTTP_SH_S_LIST;

/* used to create the Mime_Table */
typedef struct _http_mime_table
{
    CHAR * http_ext;
    CHAR * http_mime_type;
} HTTP_MIME_TABLE;

HTTP_FILE_STRUCT        *HTTP_Lite_Create_File(CHAR *, STATUS *);
STATUS                  HTTP_Lite_Delete_File(HTTP_FILE_STRUCT *);
HTTP_FILE_STRUCT        *HTTP_Lite_Find_File(CHAR *);
STATUS                  HTTP_Lite_Write_File(HTTP_FILE_STRUCT *, UINT32, CHAR *);
STATUS                  HTTP_Lite_Select(INT socketd, VOID *ssl);
VOID                    HTTP_Lite_Receive_Task(UNSIGNED, VOID *);
HTTP_PLUGIN_STRUCT      *HTTP_Lite_Get_Plugin(CHAR *, INT);
HTTP_PLUGIN_STRUCT      *HTTP_Lite_Get_Upgrade_Plugin(CHAR *, INT);
CHAR                    *HTTP_Lite_Find_Token(CHAR *, CHAR *, CHAR *, UINT8);
UINT32                  HTTP_Lite_Get_Chunk_Data(CHAR *, CHAR **, INT, UINT32, VOID *, UINT32*);
UINT32                  HTTP_Lite_Put_Chunk_Data(CHAR *, UINT32, UINT32);
UINT32                  HTTP_Lite_Write(INT, CHAR *, UINT32, VOID *);
UINT32                  HTTP_Lite_Receive(INT, CHAR *, UINT32, VOID *, UINT8);
INT                     HTTP_Lite_Read_Line(CHAR *, INT, UINT32, UINT8, VOID *);
UINT32                  HTTP_Lite_Read_Buffer(CHAR *, UINT32, INT, CHAR **, UINT32, VOID *, UINT32, UINT32*, VOID*);
STATUS                  HTTP_Lite_Write_Buffer(INT, CHAR *, UINT32, CHAR *, UINT32, VOID *);
UINT32                  HTTP_Lite_Svr_Send(HTTP_SVR_SESSION_STRUCT *, CHAR *, UINT32);
VOID                    HTTP_Lite_Response_Header(UINT16);
VOID                    HTTP_Lite_Header_Name_Insert(CHAR *, CHAR *);
STATUS                  HTTP_Lite_Send_Status_Message(UINT16, CHAR *);
STATUS                  HTTP_Lite_Svr_Get(INT, CHAR *, HTTP_SVR_SESSION_STRUCT *);
STATUS                  HTTP_Lite_Svr_Put(INT, CHAR *, HTTP_SVR_SESSION_STRUCT *);
STATUS                  HTTP_Lite_Svr_Delete(INT, CHAR *, HTTP_SVR_SESSION_STRUCT *);
STATUS                  HTTP_Lite_Register_Plugin(STATUS (*plug_in)(INT, CHAR *, HTTP_SVR_SESSION_STRUCT*),
                                                  CHAR *, INT);
VOID                    HTTP_Lite_Remove_Plugin(CHAR *, INT);

#endif /* _HTTP_LITE_INT_H */
