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
*       ws_extrn.h                                                
*                                                                       
* COMPONENT                                                             
*                    
*       Nucleus WebServ                                                   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This file contains the function prototypes necessary for
*       Nucleus WebServ Product to work correctly.                                                       
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

#ifndef _WS_EXTRN_H
#define _WS_EXTRN_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif

/* CFS_TOOL.C */
INT32           CFS_Compress(INT mode, CHAR *inbuf, CHAR *outbuf, INT32 length);
INT             CFS_Decompress(WS_REQUEST *req, CHAR *inbuf, CHAR *outbuf, INT32 inlen);


/* DIR_PLGN.C */
INT             DIR_List_Directory(WS_REQUEST *req);


/* HTTP_PSR.C */
VOID            HTTP_Parse_Request(WS_REQUEST *req);
VOID            HTTP_Response_Header(WS_REQUEST *req, INT code);
VOID            HTTP_Initiate_Response(WS_REQUEST *req, INT mode);
VOID            HTTP_Redirect_Client(WS_REQUEST *req, CHAR *url);
VOID            HTTP_Header_Num_Insert(WS_REQUEST *req, CHAR *name, INT32 value);
VOID            HTTP_Header_Name_Insert(WS_REQUEST *req, CHAR *name, CHAR *value);
VOID            HTTP_Client_Error(WS_REQUEST *req, CHAR *reason);
VOID            HTTP_Build_Array(CHAR HUGE *buffer, WS_TOKEN_INFO *token_info, UINT8 mode);
CHAR            *HTTP_Token_Value_by_Number(INT count, WS_REQUEST *req, UINT8 mode);
VOID            HTTP_Convert_To_Url(WS_REQUEST *req, CHAR *uri, CHAR *url_buf, OPTION ssl_flag, UINT16 buf_sz);
VOID            HTTP_Serve_File(WS_REQUEST *req);
VOID            HTTP_Send_Status_Message(WS_REQUEST *req, INT code, CHAR *mes);
CHAR            *HTTP_Value_by_Name(CHAR *name, WS_REQUEST *req, UINT8 mode);
VOID            HTTP_Make_Mime_Header(WS_REQUEST * req, INT32 length);

#if INCLUDE_SSL
#define HTTPS_Uri_To_Url(req, uri, url_buf)        HTTP_Convert_To_Url(req, uri, url_buf, NU_TRUE, NU_NULL)
#else
#define HTTPS_Uri_To_Url(req, uri, url_buf)        HTTP_Convert_To_Url(req, uri, url_buf, NU_FALSE, NU_NULL)
#endif

#define HTTP_Uri_To_Url(req, uri, url_buf)         HTTP_Convert_To_Url(req, uri, url_buf, NU_FALSE, NU_NULL)

#define HTTP_Server_Error(req, mes)                HTTP_Send_Status_Message(req, WS_PROTO_SERVER_ERROR, mes)
#define HTTP_Send_Client_HTML_Msg(req, mes)        HTTP_Send_Status_Message(req, WS_PROTO_OK, mes)

#define HTTP_Token_Value_by_Name(name, req)        HTTP_Value_by_Name(name, req, 0)
#define HTTP_Build_Token_Array(buffer, token_info) HTTP_Build_Array(buffer, token_info, NU_NULL)

#if INCLUDE_COOKIES
#define HTTP_Cookie_Value_by_Name(name, req)       HTTP_Value_by_Name(name, req, WS_TOKEN_COOKIE)
VOID            HTTP_Build_Cookie_Array(WS_REQUEST *req);
VOID            HTTP_Set_Cookie(WS_REQUEST *req, CHAR *name, CHAR *value);
#endif


/* UPL_PLGN.C */
INT             UPL_File_Upload(WS_REQUEST *req);


/* WSN.C */
INT32           WSN_Read_Net(WS_REQUEST * req, CHAR * buf, UINT16 sz, UNSIGNED timer);
INT32           WSN_Send(WS_REQUEST *req, CHAR HUGE *buffer, UINT32 size);
STATUS          WSN_Write_To_Net(WS_REQUEST *req, CHAR HUGE * buf, UINT32 sz, INT mode);
VOID            WSN_Flush_Net(WS_REQUEST * req);

#define         WS_Read_Net             WSN_Read_Net
#define         WS_Send                 WSN_Send
#define         WS_Write_To_Net         WSN_Write_To_Net
#define         WS_Flush_Net            WSN_Flush_Net


/* WSF.C */
INT             WSF_File_Request_Status(WS_REQUEST * req);
INT             WSF_File_Find(WS_REQUEST *req, CHAR *file_name);
INT             WSF_Send_File(WS_REQUEST * req);
INT             WSF_Read_File(WS_REQUEST * req, CHAR * f);
INT             WSF_Write_File_System(CHAR * fname, CHAR HUGE* filemem, UINT32 length);

#define         HTTP_File_Request_Status    WSF_File_Request_Status
#define         HTTP_File_Find              WSF_File_Find
#define         WS_Send_File                WSF_Send_File
#define         WS_Read_File                WSF_Read_File
#define         WS_Write_File_System        WSF_Write_File_System


/* WS_TASKS.C */
STATUS          WS_Webserv_Initialize(VOID);
VOID            WS_Receive_Task(UNSIGNED port, VOID *queue);
VOID            WS_Strn_Cpy(CHAR HUGE *target, CHAR HUGE *buffer, UINT32 size);
VOID            WS_Mem_Cpy(CHAR HUGE *target, CHAR HUGE *buffer, UINT32 size);
VOID            WS_Register_Plugin(INT(* plug_in)(WS_REQUEST *), CHAR *uri, UINT8 flags);
CHAR            *WS_Find_Token_by_Case(CHAR *token, CHAR HUGE *file, CHAR *last, UINT8 mode);
CHAR            *WS_In_String(CHAR *target, CHAR HUGE * string_buf);
VOID            WS_Clear_Rdata(WS_REQUEST *req);
VOID            WS_Cleanup_Server(VOID);

#define         WS_Find_Token(a, b, c)  WS_Find_Token_by_Case(a, b, c, WS_TOKEN_CASE_SENS)
#define         HTTP_Register_Plugin    WS_Register_Plugin
#define         HTTP_Find_Token         WS_Find_Token
#define         HTTP_In_String          WS_In_String
#define         Webserv_init            WS_Webserv_Initialize

#if WS_AUTHENTICATION
VOID            WS_Set_Auth_Callback(VOID (*function)(WS_AUTH*));
#endif

/* WEB_SSL */
#if INCLUDE_SSL

#include "openssl/ssl.h"

STATUS          NU_WS_SSL_Init(NU_MEMORY_POOL *mem_pool);
STATUS          WS_SSL_Accept(SSL **new_ssl, INT socketd, SSL_CTX *ssl_context);

#endif /* INCLUDE_SSL */

#ifdef  __cplusplus
}
#endif /* _cplusplus */

#endif  /* _WS_EXTRN_H */
