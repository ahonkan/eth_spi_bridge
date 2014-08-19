/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2002              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/************************************************************************
*                                                                       
* FILE NAME                                                             
*                                                                       
*       ps_pico.h                                                 
*                                                                       
* COMPONENT                                                             
*                    
*       Nucleus WebServ                                                   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This file is provided for backwards compatibility only.
*		This file should only be used for plugin support.                                                       
*                                                                       
* DATA STRUCTURES                                                       
*        
*       fs_file                 The incode file system
*       ps_auth                 Stores authentication state variables 
*                               for each connection
*       master_auth
*       ps_tok_elem
*       token
*       req_data
*       Request
*       plugin                  Used to map a plugin URI to a plugin
*                               entry address
*       pstat                   
*       Ps_server               Per server information
*       mime_tab                Used to create the mime table
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*      None                                                             
*                                                                       
* DEPENDENCIES                                                          
*                 
*       None                                                      
*                                                                       
*************************************************************************/

#ifndef _PICO_H
#define _PICO_H


#include "networking/nu_websr.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* HTTP output headers */

#define CONTENT_LENGTH          WS_CONTENT_LENGTH
#define CONTENT_TYPE            WS_CONTENT_TYPE
#define CONTENT_LOCATION        WS_CONTENT_LOCATION

#define TYPE_TXT_HTML           WS_TYPE_TXT_HTML


#define TOKEN_HEAP              WS_TOKEN_HEAP     	    /* maximum number of tokens per request */
#define SSI_LINE                WS_SSI_LINE             /* maximum size of a SSI tag */

#define URL_LEN                 WS_URL_LEN              /* length of a URL 100 */
#define URI_LEN                 WS_URI_LEN

#define CRYPT_LEN               WS_CRYPT_LEN            /* encryption system works in units of this size */

#define IPADDR                  WS_IP_SIZE              /* bytes in an IP address */

#define NUM_SEED                WS_NUM_SEED       

#define RECEIVE_SIZE            WS_RECEIVE_SIZE         /* size of the recieve buffer */

/*  HTTP 1.1 Defines for Calculating Content Length with ps_net_write */

#define FILETRNSFR              WS_FILE_TRNSFR          /* Transferring a File */
#define REDIRECTION             WS_REDIRECTION          /* Redirecting a File  */
#define PLUGINPROTO             WS_PLUGIN_PROTO         /* PLUGIN Functiopn Prototype */
#define PLUGINDATA              WS_PLUGIN_DATA          /* PLUGIN Store Data and Size */
#define PLUGINSEND              WS_PLUGIN_SEND          /* PLUGIN Send  Data          */               

#define REQ_PROCEED             WS_REQ_PROCEED          /* function preformed, continue */
#define REQ_ABORTED             WS_REQ_ABORTED          /* request should be aborted */
#define REQ_NOACTION            WS_REQ_NOACTION         /* plugin did not preform intended action */
#define REQ_ERROR               WS_REQ_ERROR            /* plugin i/o error abort request */

/*      HTTP return values */

#define PROTO_OK                WS_PROTO_OK             /*  OK (success)--- */
#define PROTO_NO_RESPONSE       WS_PROTO_NO_RESPONSE    /*  OK but no data to be returned to client */
#define PROTO_REDIRECT          WS_PROTO_REDIRECT       /* Redirect client to new URL in Location: header */
#define PROTO_NOT_MODIFIED      WS_PROTO_NOT_MODIFIED   /* URL not modified (in resp to client conditional GET */
#define PROTO_BAD_REQUEST       WS_PROTO_BAD_REQUEST    /* Tells clinet she sent a bad request */
#define PROTO_UNAUTHORIZED      WS_PROTO_UNAUTHORIZED   /* The request requires user authentication */
#define PROTO_FORBIDDEN         WS_PROTO_FORBIDDEN      /* Server forbids this client request */
#define PROTO_NOT_FOUND         WS_PROTO_NOT_FOUND      /* the famous old "404" URL not found error */
#define PROTO_SERVER_ERROR      WS_PROTO_SERVER_ERROR   /* Internal Server ERROR */
#define PROTO_NOT_IMPLEMENTED   WS_PROTO_NOT_IMPLEMENTED /* Server feature not implemented */


/* the incore filesystem table */

struct fs_file{
        CHAR            name[URI_LEN];                  /* name */
        CHAR            *addr;                          /* starting address */
        INT16           type;                           /* mode */
        INT             length;                         /* real length */
        INT             clength;                        /* compressed length */
        struct fs_file  * next;                         /* next file in chain */
};


#define AUTH_MAX_USERS     WS_AUTH_MAX_USERS            /* maximum authenticated users allowed at a time*/


typedef struct ps_auth ps_auth;
typedef struct Ps_server Ps_server;     

/* Stores authentication state variables
 * for each connection 
 */
 struct ps_auth{

        INT             state;                          /* authentication state of this connection */
        INT             countdown;                      /* countdown untill this structure is re-claimed */
        UINT8           ip[IPADDR];                     /* ip address of the client */
        CHAR            salt[NUM_SEED][CRYPT_LEN];      /* sent to user to combine with user plaintext */
        struct ps_auth  *next;                          /* linked list of structures */

};

/* Keys to the server */

typedef struct {
        INT             timeoutval;                     /* timeout value for user_auth */
        INT             last_time;                      /* last time value */
        INT             flags;                          /* mask to declare conditions of match */
        ps_auth         user_auth[AUTH_MAX_USERS];
        CHAR            key[CRYPT_LEN+1];               /* encryption key for this server */
        INT             auth_state;                     /* 1- Measn in process of authenticating
                                                         * 0- Means authenticated.
                                                         */
        CHAR            auth_uri[URI_LEN];
        CHAR            auth_public[URI_LEN];           /* directory tree excempt from authentication */
}master_auth;

typedef struct {
        CHAR            *name;
        CHAR            *value;
}ps_tok_elem;

typedef struct token Token;

struct token{
        ps_tok_elem     arg;
        INT             data;
};

#define OBUFSZ  WS_OUT_BUFSZ     

typedef struct req_data {

        Token           heap[TOKEN_HEAP];               /* pool of free token structures */
        CHAR            lbuf[RECEIVE_SIZE+2];           /* raw HTTP request from client is stored here */
        CHAR            out_head[RECEIVE_SIZE/2];       /* space to store the response header for this request */
        CHAR            ssi_buf[SSI_LINE];
        CHAR            obuf[OBUFSZ];                   /* output buffer  for ps_net_write() */
}_req_data;

typedef struct {

        Token           *pg_args;                       /* plugin arguments (if request is a POST ) */
        Ps_server       *server;                        /* pointer to infor for this server */
        CHAR            *headers;                       /* Any header info the client may have sent */
        CHAR            *fname;                         /* URI (filename) of the requested entity GET,POST */
        CHAR            *response;                      /* the HTTP response to the current request */
        struct ps_stat  *stat;                          /* internal stat structure */
        _req_data       *rdata;                         /* per request allocated data */
        UINT8           ip[IPADDR];                     /* ip address of client */
        INT             nbytes;                         /* number of bytes in input buffer */
        INT             sd;                             /* socket descripter for this connection */
        INT16           method;                         /* HTTP method of this request GET, POST, HEAD etc */
        INT16           obufcnt;                        /* HTTP method of this request GET, POST, HEAD etc */

        /* Added to the new WS_REUEST structure */
        INT             ws_http_ver;
        INT             ws_first;
        CHAR            ws_user[33];                    /* Name of authenticated user */
        CHAR            *ws_line;
        WS_HTTP_PLUGIN  ws_hp;
}Request;


typedef struct ps_stat {                                /* used to describe file infomation */

        CHAR            *address;                       /* address if incore file */
        INT             (*plugin)(Token *, Request *);  /* triggers a call to this function */
        unsigned int    size;
        INT             flags;
};

/* Per server information */
 struct Ps_server{

        INT             flags;                          /* server state */
    struct Ps_server    *next;                          /* next server (multiple server support ) */
        INT             bytes_in;                       /* total bytes recieved */
        INT             bytes_out;                      /* total bytes sent */
        UINT8           ip[IPADDR];                     /* ip address of the server */
        UINT16          port;                           /* port the server listens to (default 80 ) */
        master_auth     master;                         /* authentication info for this server */

};

#define     ps_net_write                WS_Write_To_Net 
#define     ps_net_read                 WS_Read_Net
#define     in_string                   HTTP_In_String
#define     ps_proto_status             HTTP_Response_Header
#define     ps_outhdr_insert            HTTP_Header_Name_Insert
#define     ps_proto_start_response     HTTP_Initiate_Response
#define     ps_proto_redirect           HTTP_Redirect_Client
#define     setup_pblock                HTTP_Setup_Plgn_Token
#define     client_error                HTTP_Client_Error

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _PICO_H */
