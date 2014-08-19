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
*       ws_cfg.h                                                  
*                                                                       
* COMPONENT                                                             
*                                                                 
*       Nucleus WebServ      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This file contains all user modifiable switches that allow       
*       extended features to be supported by Nucleus WebServ.     
*                                                                       
* DATA STRUCTURES                                                       
*                        
*       None                                              
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None                                                             
*                                                                       
* DEPENDENCIES                                                          
*       
*       None                                                                
*                                                                       
*************************************************************************/

#ifndef _WS_CFG_H
#define _WS_CFG_H



/* A debugging interface has been provided through the use of
 * printf statements.  The toolset used must support printf
 * statements before setting NU_WEBSERV_DEBUG to NU_TRUE.
 */

#define NU_WEBSERV_DEBUG            NU_FALSE

/* Nucleus WebServ can be initialized with files in memory.
 * This provides a base web site for the server to draw upon.
 * This web site consists of the files converted by the FILECONVERT.EXE
 * To use this system, set the following to NU_TRUE.
 */

#if (CFG_NU_OS_NET_WEB_INCLUDE_INITIAL_FILES)
#define INCLUDE_INITIAL_FILES       NU_TRUE
#else
#define INCLUDE_INITIAL_FILES       NU_FALSE
#endif


/* Nucleus WebServ can run using an in-core memory system, or a file
 * system like Nucleus File.  Define the following to NU_TRUE if an 
 * external file system is to be used.
 */

#define INCLUDE_FILE_SYSTEM         NU_TRUE

/* Setting INCLUDE_DIR_PLGN to NU_TRUE enables the Nucleus WebServ
 * directory command. This feature is a plugin that lists the users
 * incore filesystem on the users browser.
 */

#if (CFG_NU_OS_NET_WEB_INCLUDE_DIR_PLGN)
#define INCLUDE_DIR_PLGN            NU_TRUE
#else
#define INCLUDE_DIR_PLGN            NU_FALSE
#endif


/* INCLUDE_UPLOAD_PLGN allows Nucleus WebServ to support file uploading
 * using multi-part mime.  Set this to NU_TRUE to enable this feature. 
 */

#if (CFG_NU_OS_NET_WEB_INCLUDE_UPLOAD_PLGN)
#define INCLUDE_UPLOAD_PLGN         NU_TRUE
#else
#define INCLUDE_UPLOAD_PLGN         NU_FALSE
#endif


/* INCLUDE_COMPRESSION allows Nucleus WebServ to support file
 * compression of embedded HTML and other files.
 * This option supports transparent file compression for the memory based
 * file systems only.
 */

#define INCLUDE_COMPRESSION         NU_FALSE

/* Nucleus WebServ can support secure user authentication 
 * useing a Java applet and DES encryption support in the server.
 * To enable DES 40bit authentication define INCLUDE_DES_AUTH as NU_TRUE.
 */

#if (CFG_NU_OS_NET_WEB_INCLUDE_DES_AUTH)
#define INCLUDE_DES_AUTH            NU_TRUE
#else
#define INCLUDE_DES_AUTH            NU_FALSE
#endif


/* Authentication through the use of javascripts is available to
 * the server.  To enable this option, define the following as NU_TRUE.
 */

#if (CFG_NU_OS_NET_WEB_INCLUDE_JS_AUTH)
#define INCLUDE_JS_AUTH             NU_TRUE
#else
#define INCLUDE_JS_AUTH             NU_FALSE
#endif


/* To select Basic Authtentication as defined in HTTP 1.0. 
 * To enable this option, define the following as NU_TRUE
 */

#if (CFG_NU_OS_NET_WEB_INCLUDE_BASIC_AUTH)
#define INCLUDE_BASIC_AUTH          NU_TRUE
#else
#define INCLUDE_BASIC_AUTH          NU_FALSE
#endif


/* To run WebServ using SSL, the SSL module must be present, and the 
 * following must be defined as NU_TRUE:
 */

#if (CFG_NU_OS_NET_WEB_INCLUDE_SSL)
#define INCLUDE_SSL                 NU_TRUE
#else
#define INCLUDE_SSL                 NU_FALSE
#endif


/* To include server support of cookies, define the following as NU_TRUE: */

#if (CFG_NU_OS_NET_WEB_INCLUDE_COOKIES)
#define INCLUDE_COOKIES             NU_TRUE
#else
#define INCLUDE_COOKIES             NU_FALSE
#endif


/* For the server to be case sensitive with the use of file names, define
 * the following as NU_TRUE:
 */

#if (CFG_NU_OS_NET_WEB_CASE_SENSITIVE)
#define WS_CASE_SENSITIVE           NU_TRUE
#else
#define WS_CASE_SENSITIVE           NU_FALSE
#endif


/* CONFIGURABLE PARAMETERS OF WEBSERV */

/* Nucleus Webserv relies on several tasks to perform its duties. 
 * The settings for the creation of these tasks are defined below. 
 */

/* The Webserv Init task is responsible for initializating Webserv when Nucleus
 * Middleware Initialization is being used.
 */
#define WEBSERV_INIT_STACK_SIZE 3000
#define WEBSERV_INIT_PRIORITY   1   
#define WEBSERV_INIT_TIME_SLICE 0
#define WEBSERV_INIT_PREEMPT    NU_PREEMPT

#define WS_AUTH_MAX_USERS       4                   /* maximum authenticated users allowed at a time */
#define WS_AUTH_TIMEOUT         300                 /* a session idle for AUTH_TIMEOUT seconds is de-authenticated */
#define WS_AUTH_MAX_KEY         999907UL            /* max value for key, should be odd number */
#define WS_AUTH_MAX_KEY_STR     "999907"            /* This number should match WS_AUTH_MAX_KEY */
#define WS_AUTH_SCRIPT_SZ       2048                /* size of script to be encoded with javascript 
                                                        should be at least 2k */
#define WS_AUTH_NAME_LEN        32                  /* max length of a user name or password */

#define WS_QSIZE                TCP_MAX_PORTS       /* Size of socket queue */
#define WS_STACK_SIZE           7500                /* Worker task stack size */
#define WS_MAX_WORKER_TASKS     2                   /* Number of worker tasks allowed */
#define WS_LOCAL_QUEUE_SIZE     10                  /* Number of connections a worker maintains */

#define WS_OUT_BUFSZ            2048                /* size of output buffer */
#define WS_RECEIVE_SIZE         2048                /* size of the recieve buffer */
#define WS_MAX_SEND_SIZE        20000               /* max bytes sent to network layer */
#define WS_HEADER_SIZE          256                 /* max size of output header */    
#define WS_CFS_DECOMP_SZ        500                 /* size of output buffer during decompression */
#define WS_MAX_RECV_BYTES       10000               /* max receive size */

#define WS_TIMEOUT              TICKS_PER_SECOND >> 1
#define WS_SOCKET_GRACEFUL_TIMEOUT    100           /* Time out in seconds for an idle socket */

/* define URL's for success and failure */

#define WS_AUTH_SUCCESS_URI     "success.htm"       /* go here after sucessfull authentication */
#define WS_AUTH_FAILURE_URI     "failure.htm"       /* go here after sucessfully logged in */
#define WS_AUTH_SCREEN_URI      "login.htm"         /* Screen to show an unauthenticated user */
#define WS_AUTH_DEFAULT_URI     "post.htm"

#define WS_DEFAULT_URI          "index.htm"         /* default uri when "/" is referenced */

#define WS_PUBLIC_DIR           "/publicdir"        /* anything is this directory is ALWAYS public */
#define WS_PRIVATE_DIR          "/privatedir"       /* anything is this directory is ALWAYS private */

#ifdef CFG_NU_OS_NET_WEB_FILE_NAME_TAG
#define WS_FILE_NAME_TAG        CFG_NU_OS_NET_WEB_FILE_NAME_TAG
#else
#define WS_FILE_NAME_TAG        "save-as-filename"  /* HTML tag for file upload */
#endif

#define WS_DEFAULT_DRIVE        0                   /* Default drive for file system */

#define WS_SSI_NAME_LEN         32                  /* maximum name length for ssi*/
#define WS_SSI_LINE             256                 /* maximum size of a SSI tag */
#define WS_SSI_HEAP             16                  /* maximum number of ssi tokens */

#define WS_TOKEN_HEAP           16                  /* maximum number of tokens per request */
#define WS_MAX_PLUGIN           32                  /* maximum plugins */
#define WS_PLUGIN_NAME_LEN      31                  /* maximum name length for plugin */

#define WS_URI_LEN              100                 /* maximum length of the URI */
#define WS_URL_LEN              WS_URI_LEN + 40     /* maximum length of a URL */

#define WS_COOKIE_LEN           32                  /* Max name length of cookie */
#define WS_COOKIE_HEAP          4                   /* Max number of cookies per request */

#define WS_AUTH_COOKIE_ID       "id"                /* ID tag passed from client */
#define WS_AUTH_COOKIE_USER     "user"              /* Tag passed for client's user name */

/* SSL based defines */

#define NU_SSL_METHOD           SSLv23_server_method;/* SSL methods permitted for use */

#define NU_SSL_CERT             "server.pem"       /* Certificate file */
#define NU_SSL_KEY              "server.pem"       /* Key file */
#define NU_SSL_CA_LIST          "root.pem"         /* List of trusted Certificate Autorities */


/* END OF USER CONFIGURATION SECTION */

#define WS_Get_Seconds  (NU_Retrieve_Clock()/TICKS_PER_SECOND)

#if WS_CASE_SENSITIVE

#define WS_Strcmp           strcmp
#else
#define WS_Strcmp           NU_STRICMP

#endif /* WS_CASE_SENSITIVE */

#if INCLUDE_DES_AUTH
#define WS_AUTHENTICATION       NU_TRUE 
#endif /* INCLUDE_DES_AUTH */


#if INCLUDE_JS_AUTH
#define WS_AUTHENTICATION       NU_TRUE

#undef INCLUDE_COOKIES
#define INCLUDE_COOKIES         NU_TRUE

#endif /* INCLUDE_JS_AUTH */

#ifndef WS_AUTHENTICATION       
#define WS_AUTHENTICATION       NU_FALSE
#endif

#if ((INCLUDE_DES_AUTH + INCLUDE_JS_AUTH + INCLUDE_BASIC_AUTH) > 1)
#error Only one form of authentication may be chosen!
#else 
#if ((INCLUDE_DES_AUTH + INCLUDE_JS_AUTH + INCLUDE_SSL) > 1)
#error SSL is not compatible with either DES or JS authentication!
#endif
#endif
      
#if ((INCLUDE_FILE_SYSTEM + INCLUDE_COMPRESSION) > 1)
#error Compression is not compatible with external file systems!
#endif

#endif /* _WS_CFG_H */
