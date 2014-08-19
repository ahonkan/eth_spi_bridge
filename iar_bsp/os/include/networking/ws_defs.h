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
*      ws_defs.h                                                   
*                                                                       
* COMPONENT                                                             
*                                                                       
*      Nucleus WebServ                                                  
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*      Internal definitions for Nucleus WebServ                                                                 
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       WS_AUTH
*       WS_BIT_FILE
*       WS_FS_FILE
*       WS_MASTER_AUTH
*       WS_MIME_TABLE
*       WS_PLUGIN_STRUCT
*       WS_REQ_DATA
*       WS_REQUEST
*       WS_SERVER
*       WS_SOCK_LIST
*       WS_SOCKET
*       WS_SOCKET_STRUCT
*       WS_STAT
*       WS_SYMBOL
*       WS_TOKEN_INFO
*
* FUNCTIONS
*
*       None
*                                                  
* DEPENDENCIES                                                          
*                                                               
*       None        
*                                                                       
************************************************************************/

#ifndef _WS_DEFS_H
#define _WS_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif

/* GENERAL DEFINITIONS */

#ifdef NET_5_1
#define WS_IP_SIZE              MAX_ADDRESS_SIZE
#else
#define WS_IP_SIZE              4
#endif

#define WS_TOKEN_CASE_SENS      23              /* Case sensitive compare */
#define WS_TOKEN_CASE_INSENS    24              /* Case insensitive compare */

#define WS_FILETRNSFR           0               /* Transferring a File */
#define WS_REDIRECTION          1               /* Redirecting a File  */
#define WS_PLUGIN_PROTO         2               /* PLUGIN Function Prototype */
#define WS_PLUGIN_DATA          3               /* PLUGIN Store Data and Size */
#define WS_PLUGIN_SEND          4               /* PLUGIN Send Data */               

#define WS_FAILURE              -1

#define WS_REQ_PROCEED          1               /* function preformed, continue */
#define WS_REQ_ABORTED          2               /* request should be aborted */
#define WS_REQ_NOACTION         3               /* plugin did not preform intended action */
#define WS_REQ_ERROR            4               /* plugin i/o error abort request */

#define WS_TOKEN_COOKIE         1
    
    /* Server defines */

#define WS_MEM_ALLOC            1               /* Flag for marking allocated data */
#define WS_DATA_AVAIL           2               /* Flag for specifing data in buffer */
#define WS_DEFAULT_MIME         0

/* HTTP SPECIFIC DEFINITIONS */

#define WS_HTTP_PORT            80              /* Port used for HTTP communication */
    
#define WS_CRLF                 "\r\n"          /* End of header property */
#define WS_CRLFCRLF             "\r\n\r\n"      /* End of Entity Header Message */
#define WS_CRLFCRLF_LEN         (sizeof(WS_CRLFCRLF) - 1) /* Don't count null terminator */  

    /* HASH tokens to identify type of HTTP transfer */

#define WS_GET                  224
#define WS_HEAD                 206
#define WS_POST                 242

#define WS_HTTP_10_STRING       "HTTP/1.0"
#define WS_HTTP_10              10
#define WS_HTTP_11_STRING       "HTTP/1.1"
#define WS_HTTP_11              11
    
    /* HTTP output headers */

#define WS_CONTENT_LENGTH       "Content-Length: "
#define WS_CONTENT_TYPE         "Content-Type: "
#define WS_CONTENT_LOCATION     "Location: "
#define WS_TYPE_TXT_HTML        "text/html"
#define WS_PRAGMA               "Pragma: "
#define WS_COOKIE_HEADER        "Cookie: "
#define WS_SET_COOKIE           "Set-Cookie: "

    /* SSI markers */

#define WS_SSI_TOKEN            "<!-#"          /* ServerSideInclude magic string */
#define WS_SSI_LC_MARKER        ".ssi"          /* extension for a server side include HTML file */
#define WS_SSI_UC_MARKER        ".SSI"          /* extension for a server side include HTML file */
#define WS_SSI                  1               /* Used to differentiate token strings */

    /* HTTP return values */

#define WS_PROTO_OK             200             /*  OK (success)--- */
#define WS_PROTO_NO_RESPONSE    204             /*  OK but no data to be returned to client */
#define WS_PROTO_REDIRECT       302             /* Redirect client to new URL in Location: header */
#define WS_PROTO_NOT_MODIFIED   304             /* URL not modified (in resp to client conditional GET */
#define WS_PROTO_BAD_REQUEST    400             /* Tells client it sent a bad request */
#define WS_PROTO_UNAUTHORIZED   401             /* The request requires user authentication */
#define WS_PROTO_FORBIDDEN      403             /* Server forbids this client request */
#define WS_PROTO_NOT_FOUND      404             /* the famous old "404" URL not found error */
#define WS_PROTO_SERVER_ERROR   500             /* Internal Server ERROR */
#define WS_PROTO_NOT_IMPLEMENTED 501            /* Server feature not implemented */


/* FILE SYSTEM DEFINITIONS */

#define WS_FS_PREFIX            "\\"     
#define WS_FS_LEN               100
#define WS_FBUF_SZ              1460            /* This is maximum size for TCP datagram */

    /* incore file system TYPE flags */

#define WS_COMPILED             1               /* If file is compiled */
#define WS_DELETED              2               /* If file is deleated */
#define WS_COMPRESSED           4               /* If file is compressed */

    /* compression control */

#define WS_DO_OUTPUT            1
#define WS_NET_OUTPUT           2               /* de-compr output to net */
#define WS_DONT_OUTPUT          4
#define WS_CHD_FUD              16              /* fudge factor */
#define WS_CHD_SZ               4               /* size of preamble */
#define WS_CHD                  "pS$C"          /* compression marker*/

    /* upload file mode control */

#define WS_NOT_FOUND            0x00            /* no file found */
#define WS_INCORE               0x01            /* an incore file...no I/O needed */
#define WS_PLUGIN               0x02            /* file was a plugin */
#define WS_FOUND                0x04            /* file was found */
#define WS_DIRECTORY            0x08            /* file was found */
#define WS_PUBLIC               0x10            /* file is in WS_PUBLIC_DIR */
#define WS_PRIVATE              0x20            /* file is in WS_PRIVATE_DIR */
#define WS_SCRIPT               0x40            /* For use with JS_AUTH */
    
/* AUTHENTICAION DEFINITIONS */

#define WS_CRYPT_LEN            8               /* encryption system works in units of this size */
#define WS_NUM_SEED             2       

#define WS_AUTH_FREE            0x01            /* auth structure is free */
#define WS_AUTH_GIVEN           0x02            /* state value for an authenticated user */
#define WS_AUTH_ENABLED         0x80            /* authentication is enabled */

    /* auth API */

#define WS_A_ENABLE             1               /* enable authentication */
#define WS_A_DISABLE            2               /* disable authentication */
#define WS_A_CREDS              3               /* change credentials */
#define WS_A_AUTH_URI           4               /* send user who needs authentication here */
#define WS_A_AUTH_ADD           5               /* register the passed ip address as authenticated */
#define WS_A_AUTH_GLOBL         6               /* make this directory available w/o authentication */
#define WS_A_TIMEOUT            7               /* set the authentication timeout value */
#define WS_A_LOGOUT             8               /* logout the selected ip address */


/* SSL DEFINITIONS */

#define WS_SSL_PORT             443             /* Port used for SSL communications */

/***********************/


/* GENERAL SERVER DATA STRUCTURES */

typedef struct _WS_MASTER_AUTH WS_MASTER_AUTH;
typedef struct _WS_REQUEST WS_REQUEST;
typedef struct _WS_SERVER WS_SERVER;

typedef struct _WS_SOCKET {
    INT         ws_new_socket;
    INT16       ws_family;
    UINT8       padding[2];
} WS_SOCKET;

/* Socket information */
typedef struct _WS_SOCKET_STRUCT{
    
 struct _WS_SOCKET_STRUCT   *ws_next_link;
 struct _WS_SOCKET_STRUCT   *ws_prev_link;
        INT                 ws_socketd;                 /* Socket descriptor */
        UNSIGNED            ws_time_click;              /* Time of last activity */
        UNSIGNED            *ws_ssl_ptr;                /* Pointer to SSL structure, if enabled */
        INT                 ws_http_ver;                
        INT16               ws_family;
        UINT8               ws_ip[WS_IP_SIZE];
}WS_SOCKET_STRUCT;    


/* Socket information list header */
typedef struct _WS_SOCK_LIST{
    
        WS_SOCKET_STRUCT    *ws_sock_list_head;         /*  Socket List Head */
        WS_SOCKET_STRUCT    *ws_sock_list_tail;         /*  Socket List Tail */
}WS_SOCK_LIST;


/* FILE SYSTEM DATA STRUCTURES */

/* the incore filesystem table */

typedef struct _WS_FS_FILE{
    
    CHAR            ws_name[WS_URI_LEN];    /* name */
    CHAR            *ws_addr;               /* starting address */
    INT16           ws_type;                /* mode (WS_COMPRESSED or WS_COMPILED) */
    INT32           ws_length;              /* real length */
    INT32           ws_clength;             /* compressed length */
    struct _WS_FS_FILE * ws_next;           /* next file in chain */
    UINT8           padding[2];
}WS_FS_FILE;


/* used to describe file infomation */
typedef struct _WS_STAT {  
    
    CHAR            *ws_address;                /* address if incore file */
    INT             (*plugin)(WS_REQUEST *);    /* triggers a call to this function */
    INT32           ws_size;
    INT16           ws_type;
    INT             ws_flags;
    UINT8           padding[2];
}WS_STAT;


/* HTTP SPECIFIC DATA STRUCTURES */

/* Plugin request information */
typedef struct {
    
        INT16           *ws_token_array;
        CHAR HUGE       *ws_token_string;
}WS_TOKEN_INFO;

typedef struct {
        CHAR                ws_in_header[WS_RECEIVE_SIZE];
        UINT32              ws_in_header_sz;
        CHAR HUGE           *ws_in_data;
        UINT32              ws_in_data_sz;
        INT16               ws_in_data_flag;
        CHAR HUGE           *ws_next_in_data;
        UINT32              ws_next_in_data_sz;
        
        CHAR                ws_out_header[WS_HEADER_SIZE];
        CHAR HUGE           *ws_out_data;
        UINT32              ws_out_data_sz;
        UINT32              ws_out_data_free;
        INT16               ws_no_head_flag;

} WS_REQ_DATA;

struct _WS_REQUEST{

        WS_TOKEN_INFO       ws_pg_args;                 /* plugin arguments */
        WS_TOKEN_INFO       ws_ssi_args;                /* SSI arguments */
        WS_TOKEN_INFO       ws_cookie_jar;              /* Pointer to cookie information */

        WS_SERVER           *ws_server;                 /* pointer to info for this server */
        struct _WS_STAT     ws_stat;                    /* internal stat structure */
        
        WS_REQ_DATA         ws_rdata;                   /* per request allocated data */
        
        CHAR                *ws_fname;                  /* URI (filename) of the requested entity */
        
        UINT8               ws_ip[WS_IP_SIZE];          /* ip address of the client */
        UINT8               *ws_server_ip;              /* ip address of the server */
        INT                 ws_sd;                      /* socket descripter for this connection */

        INT16               ws_method;                  /* HTTP method of this request GET, POST, HEAD etc */
        INT                 *ws_http_ver;               /* Version of HTTP that client is using */
        CHAR                ws_user[WS_AUTH_NAME_LEN];  /* Name of authenticated user */
        CHAR                ws_ssi_buf[WS_SSI_LINE];
        UNSIGNED            *ws_ssl;                    /* Pointer to SSL structure, if enabled */
        INT16               ws_family;
};


/* used to map a plugin URI to
 * a plugin entry address
 */
typedef struct _WS_PLUGIN_STRUCT{                          

    struct _WS_PLUGIN_STRUCT *ws_next;                  /* Pointer to next plugin structure */
        CHAR            ws_name[WS_PLUGIN_NAME_LEN];    /* a name in our file-space which when accessed */
        INT             (*plugin)(WS_REQUEST *);        /* triggers a call to this function */
        UINT8           ws_plg_flag;                    /* Keeps track of different states of plugin */             
}WS_PLUGIN_STRUCT;


/* used to create the Mime_Table */
typedef struct {
    
        CHAR * ws_ext;
        CHAR * ws_mime_type;
}WS_MIME_TABLE;

/* AUTHENTICATION */
/* Stores authentication state variables
 * for each connection 
 */
#if WS_AUTHENTICATION
#if INCLUDE_DES_AUTH

typedef struct _WS_AUTH{

        INT         ws_state;                           /* authentication state of this connection */
        UINT32      ws_countdown;                       /* countdown untill this structure is re-claimed */
        UINT8       ws_ip[WS_IP_SIZE];                  /* ip address of the client */
        CHAR        ws_salt[WS_NUM_SEED][WS_CRYPT_LEN]; /* sent to user to combine with user plaintext */
        VOID        *ws_client_data;                    /* blank pointer provided for use by plugins */ 
 struct _WS_AUTH    *ws_next;                           /* linked list of structures */
}WS_AUTH;

/* Keys to the server */
struct _WS_MASTER_AUTH{
    
        UINT32      ws_timeoutval;                      /* timeout value for user_auth */
        UINT32      ws_last_time;                       /* last time value */
        INT         ws_flags;                           /* mask to declare conditions of match */
        WS_AUTH     ws_user_auth[WS_AUTH_MAX_USERS];    /* list of currently authenticated users */
        CHAR        ws_key[WS_CRYPT_LEN + 1];           /* encryption key for this server */
        CHAR        ws_auth_uri[WS_URI_LEN];            /* file to send client to for loggin in */
        CHAR        ws_default_uri[WS_URI_LEN];         /* file to send client to after loggin in */
        VOID        (*ws_callback)(WS_AUTH *);          /* triggers a call to this function */
        UINT8       padding[3];
};

#else /* INCLUDE_DES_AUTH */

typedef struct _WS_AUTH{

        UINT32      ws_countdown;                       /* countdown untill this structure is re-claimed */
        CHAR        ws_user[32];                        /* user's name */
        UINT32      ws_key;                             /* user's key (converted password) */
        UINT32      ws_id;                              /* user's id */
        VOID        *ws_client_data;                    /* blank pointer provided for use by plugins */ 
}WS_AUTH;

/* Keys to the server */
struct _WS_MASTER_AUTH{
    
        UINT32      ws_timeoutval;                      /* timeout value for user_auth */
        UINT32      ws_last_time;                       /* last time value */
        WS_AUTH     ws_user_auth[WS_AUTH_MAX_USERS];    /* list of currently authenticated users */
        CHAR        ws_auth_uri[WS_URI_LEN];            /* file to send client to for loggin in */
        CHAR        ws_default_uri[WS_URI_LEN];         /* file to send client to after loggin in */
        VOID        (*ws_callback)(WS_AUTH *);          /* triggers a call to this function */
};
#endif /* INCLUDE_JS_AUTH */

#else /* WS_AUTHENTICATION */
struct _WS_MASTER_AUTH{
    
        VOID        *padding;                           /* empty structure to conserve space */
};
#endif /* WS_AUTHENTICATION */

/* Server information */
struct _WS_SERVER{

        INT             ws_flags;                       /* server state */
 struct _WS_SERVER      *ws_next;                       /* next server (multiple server support) */
        INT             ws_bytes_in;                    /* total bytes recieved */
        INT             ws_bytes_out;                   /* total bytes sent */
        UINT8           *ws_ip;                         /* ip address of the server (not used) */
        UINT16          ws_port;                        /* port the server listens to (not used) */
        WS_MASTER_AUTH  ws_master;                      /* authentication info for this server */
        NU_MEMORY_POOL  *ws_memory_pool;                /* pointer to memory pool used by WebServ */
        UINT8           padding[2];
};


/* COMPRESSION / DECOMPRESSION STRUCTURES */

typedef struct 
{
        CHAR        *fstart;                    /* start of "file" */
        CHAR        *cur_pos;                   /* current position in "file" */
        INT32       length;                     /* length of "file" */
        UINT8       mask;
        INT         rack;
        INT         pacifier_counter;
        INT         mode;
        WS_REQUEST  *fd;
        UINT8       padding[3];
} WS_BIT_FILE;

typedef struct
{
        UINT16      low_count;
        UINT16      high_count;
        UINT16      scale;
        UINT8       padding[2];
} WS_SYMBOL;

#ifdef __cplusplus
}
#endif /* _cplusplus */

#endif /* _WS_DEFS_H */
