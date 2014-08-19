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
*       nu_http_lite.h
*
*   COMPONENT
*
*       Nucleus HTTP Lite
*
*   DESCRIPTION
*
*       This file includes all required header files for the HTTP Lite
*       component.
*
*   DATA STRUCTURES
*
*       None
*
*   FILE DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef _NU_HTTP_LITE_H
#define _NU_HTTP_LITE_H

#include "networking/nu_networking.h"

#ifdef CFG_NU_OS_NET_SSL_LITE_ENABLE

#define HTTP_INCLUDE_CYASSL NU_TRUE

#include "os/networking/ssl/lite/cyassl/openssl/ssl.h"
#include "os/networking/ssl/lite/cyassl_nucleus.h"

#else
#define HTTP_INCLUDE_CYASSL NU_FALSE
#endif

/* Settings configured in the .metadata file located at os/networking/http */
#define HTTP_TOKEN_HEAP         CFG_NU_OS_NET_HTTP_SVR_TOKEN_HEAP

/* Return Codes. */
#define HTTP_PROTO_OK               200             /* OK (success)--- */
#define HTTP_PROTO_CREATED          201             /* A new resource is created */
#define HTTP_PROTO_ACCEPTED         202             /* OK, but action has not yet been enacted */
#define HTTP_PROTO_NO_RESPONSE      204             /* OK but no data to be returned to client */
#define HTTP_PROTO_REDIRECT         302             /* Redirect client to new URL in Location: header */
#define HTTP_PROTO_NOT_MODIFIED     304             /* URL not modified (in resp to client conditional GET */
#define HTTP_PROTO_BAD_REQUEST      400             /* Tells client it sent a bad request */
#define HTTP_PROTO_UNAUTHORIZED     401             /* The request requires user authentication */
#define HTTP_PROTO_FORBIDDEN        403             /* Server forbids this client request */
#define HTTP_PROTO_NOT_FOUND        404             /* the famous old "404" URL not found error */
#define HTTP_PROTO_URI_TOO_LONG     414             /* Request-URI Too Long */
#define HTTP_PROTO_SERVER_ERROR     500             /* Internal Server ERROR */
#define HTTP_PROTO_NOT_IMPLEMENTED  501             /* Server feature not implemented */
#define HTTP_PROTO_NOT_SUPPORTED    505             /* HTTP Version not supported */

/* Error codes */
#define HTTP_INVALID_PARAMETER      -12000
#define HTTP_NO_IP_NODE             -12001
#define HTTP_RESPONSE_ERROR         -12002 /* Entity responded with an error. Code is stored
                                              in return_value field of NU_HTTP_SH_S structure */
#define HTTP_INVALID_HEADER_READ    -12003
#define HTTP_INVALID_HEADER_PARSE   -12004
#define HTTP_ERROR_DATA_READ        -12005
#define HTTP_ERROR_DATA_WRITE       -12006
#define HTTP_INVALID_LENGTH         -12007
#define HTTP_SOCKET_NOT_READY       -12008
#define HTTP_SSL_ERROR              -12009
#define HTTP_FORCE_CLOSE            -12010
#define HTTP_FILE_ALREADY_EXISTS    -12011
#define HTTP_INVALID_URI            -12012
#define HTTP_ERROR_UNREAD_DATA      -12013

/* Methods to be used with the routines for registering / removing plugins. */
#define HTTP_LITE_PUT               0x1
#define HTTP_LITE_POST              0x2
#define HTTP_LITE_DELETE            0x4
#define HTTP_LITE_GET               0x8

/* Certificate types. */
#define HTTP_CERT_FILE              1   /* The certificate is loaded from a file. */
#define HTTP_CERT_PEM_PTR           2   /* The certificate is a buffer in PEM format. */
#define HTTP_CERT_DER_PTR           3   /* The certificate is a buffer in DER format. */

/* Key types. */
#define HTTP_KEY_FILE               1   /* The key is loaded from a file. */
#define HTTP_KEY_PEM_PTR            2   /* The key is a buffer in PEM format. */
#define HTTP_KEY_DER_PTR            3   /* The key is a buffer in DER format. */

/* CA types. */
#define HTTP_CA_FILE               1    /* The CA is loaded from a file. */
#define HTTP_CA_PEM_PTR            2    /* The CA is a buffer in PEM format. */
#define HTTP_CA_DER_PTR            3    /* The CA is a buffer in DER format. */

#define NU_HTTP_CERT_PEM_PTR    0x1     /* The certificate points to a PEM certificate in FLASH memory. */
#define NU_HTTP_CERT_DER_PTR    0x2     /* The certificate points to a DER certificate in FLASH memory. */
#define NU_HTTP_CERT_FILE       0x4     /* The certificate is stored on a local drive. */
#define NU_HTTP_KEY_FILE        0x8     /* The key sent to the foreign server is stored on a local drive. */
#define NU_HTTP_KEY_PEM_PTR     0x10    /* The key sent to the foreign server points to a PEM file in FLASH memory. */
#define NU_HTTP_KEY_DER_PTR     0x20    /* The key sent to the foreign server points to a DER certificate in FLASH memory. */
#define NU_HTTP_CA_PEM_PTR      0x40    /* The CA list points to a PEM certificate in FLASH memory. */
#define NU_HTTP_CA_DER_PTR      0x80    /* The CA list points to a DER certificate in FLASH memory. */
#define NU_HTTP_CA_FILE         0x100   /* The CA list is stored on a local drive. */
#define NU_HTTP_VERIFY_PEER     0x200   /* Verify the foreign server certificate when connecting via a client connection. */
#define NU_HTTP_NO_DOMAIN_CHECK 0x400   /* Do not check the domain name before connecting over SSL. */

/* Flags used with NU_HTTP_Lite_Configure_SSL. */
#define NU_HTTP_LITE_SVR_VERIFY_CLIENT  0x1
#define NU_HTTP_LITE_SVR_VERIFY_FAIL    0x2

typedef struct _nu_http_ssl_struct
{
    CHAR    *ssl_ca;            /* The SSL CA to use for the client connection. */
    CHAR    *ssl_cert;          /* The certificate to send to the server if requested. */
    CHAR    *ssl_key;           /* The key to send to the server if requested. */
    INT     ssl_ca_size;        /* The size of the SSL CA if not being retrieved from a file system. */
    INT     ssl_cert_size;      /* The size of the SSL certificate is not being retrieved from a file system. */
    INT     ssl_key_size;       /* The size of the SSL key is not being retrieved from a file system. */
    UINT32  ssl_flags;
} NU_HTTP_SSL_STRUCT;

typedef struct _nu_http_client
{
    CHAR                *uri;               /* The URI for the respective operation. */
    CHAR                *pdata;             /* The buffer for the respective data. */
    CHAR                *ptype;             /* The type of data. */
    NU_HTTP_SSL_STRUCT  ssl_struct;
    UINT32              plength;            /* The length of the buffer for the respective data. */
    UINT16              type_size;          /* The size of the pointer ptype. */
    UINT8               overwrite;          /* NU_TRUE or NU_FALSE to overwrite an existing resource. */
    UINT8               padN[1];
} NU_HTTP_CLIENT;

/* Plugin request information */
typedef struct _http_token_info
{
    INT16           token_array[HTTP_TOKEN_HEAP + 1];
    CHAR HUGE       *token_string;
    UINT32          token_string_len;
} HTTP_TOKEN_INFO;

typedef struct _http_svr_session_struct
{
    CHAR                        *buffer;
    CHAR                        *upgrade_req;
    struct _HTTP_PLUGIN_STRUCT  *plug_ptr;
    struct _HTTP_PLUGIN_STRUCT  *plug_ptr_post;
    struct _HTTP_PLUGIN_STRUCT  *plug_ptr_put;
    struct _HTTP_PLUGIN_STRUCT  *plug_ptr_delete;
    struct _HTTP_PLUGIN_STRUCT  *plug_ptr_get;
    struct _HTTP_PLUGIN_STRUCT  *uplug_ptr_post;
    struct _HTTP_PLUGIN_STRUCT  *uplug_ptr_put;
    struct _HTTP_PLUGIN_STRUCT  *uplug_ptr_delete;
    struct _HTTP_PLUGIN_STRUCT  *uplug_ptr_get;
    UINT32                      in_hdr_size;
    INT                         socketd;
    INT                         http_listener;
    INT                         flags;
    VOID                        *tsk_memory;
    HTTP_TOKEN_INFO             token_list;
    HTTP_TOKEN_INFO             token_query_list;
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
    INT                         ssl_listener;
    VOID                        *ssl_tsk_memory;
    SSL                         *ssl;
    SSL_CTX                     *ctx;
#endif
} HTTP_SVR_SESSION_STRUCT;

/* This data structure can be used to hold a pointer to a plug-in for a
 * URI or for an upgrade request.  The input parameters for the two operations
 * are different, so there are two function pointers in the routine.  If the
 * plug-in is for a URI, the upgrade plug-in routine uplugin will be NU_NULL
 * and vice versa.
 */
typedef struct _HTTP_PLUGIN_STRUCT
{
    struct _HTTP_PLUGIN_STRUCT  *flink;
    struct _HTTP_PLUGIN_STRUCT  *blink;
    INT                         methods;
    STATUS                      (*plugin)(INT, CHAR *, HTTP_SVR_SESSION_STRUCT *);
    STATUS                      (*uplugin)(INT, CHAR *, CHAR *, INT, INT, VOID *);
    CHAR                        *http_name;         /* Don't re-order this structure. */
} HTTP_PLUGIN_STRUCT;

typedef struct _http_plugin_list
{
    HTTP_PLUGIN_STRUCT      *flink;
    HTTP_PLUGIN_STRUCT      *blink;
} HTTP_PLUGIN_LIST;

STATUS NU_HTTP_Lite_Server_Init(VOID);
VOID   NU_HTTP_Lite_Server_Shutdown(VOID);
STATUS NU_HTTP_Lite_Register_Plugin(STATUS (*plug_in)(INT, CHAR*, HTTP_SVR_SESSION_STRUCT*), CHAR *, INT);
STATUS NU_HTTP_Lite_Create_File(CHAR *);
STATUS NU_HTTP_Lite_Delete_File(CHAR *);
STATUS NU_HTTP_Lite_Write_File(CHAR *, UINT32, CHAR *);
STATUS NU_HTTP_Lite_Remove_Plugin(CHAR *, INT);
STATUS NU_HTTP_Lite_Client_Init(VOID);

INT32  NU_HTTP_Lite_Create_Session_Handle(VOID);
STATUS NU_HTTP_Lite_Delete_Session_Handle(INT32 session_id);
STATUS NU_HTTP_Lite_Client_Get(INT32 session_id, NU_HTTP_CLIENT *cli_struct, STATUS *http_status);
STATUS NU_HTTP_Lite_Client_Put(INT32 session_id, NU_HTTP_CLIENT *cli_struct, STATUS *http_status);
STATUS NU_HTTP_Lite_Client_Delete(INT32 session_id, NU_HTTP_CLIENT *cli_struct, STATUS *http_status);
STATUS NU_HTTP_Lite_Client_Post(INT32 session_id, NU_HTTP_CLIENT *cli_struct, STATUS *http_status);
STATUS NU_HTTP_Lite_Remove_Upgrade_Plugin(CHAR *upgrade, INT methods);
STATUS NU_HTTP_Lite_Register_Upgrade_Plugin(STATUS (*plug_in)(INT, CHAR *, CHAR *, INT, INT, VOID *),
                                            CHAR *upgrade, INT methods);
STATUS NU_HTTP_Lite_Configure_SSL(CHAR *cert, UINT8 cert_type, INT cert_size,
                                  CHAR *key, UINT8 key_type, INT key_size,
                                  CHAR *ca_list, UINT8 ca_type, INT ca_size, INT flag);

#endif /* _NU_HTTP_LITE_H */
