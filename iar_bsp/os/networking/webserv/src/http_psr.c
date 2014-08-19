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
*       http_psr.c                                                
*                                                                       
* COMPONENT                                                             
*      
*       Nucleus WebServ                                                                 
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This file holds the routines that are the heart of the Nucleus   
*       Webserv product. The function within are used to maintain all    
*       facets of the Webserv Product.                                        
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*                                                                
* FUNCTIONS                                                      
*                                                                
*       HTTP_Make_Mime_Header   Generate HTTP header with the    
*                               correct mime codes.              
*       HTTP_Process_GET        Processes an HTTP response     
*                               using the GET method.            
*       HTTP_Process_POST       Processes Post form commands.    
*       HTTP_Client_Error       Error Routine for the client.    
*       HTTP_Parse_Request      Parses HTTP request and calls    
*                               necessary routines required for  
*                               processing the parsed request.   
*       HTTP_Is_SSI             Checks for an SSI (Server Side    
*                               Include) tag.                    
*       HTTP_Process_SSI        Process a Get Operation with the 
*                               SSI extension.                  
*       HTTP_Serve_File         This function sends the file to  
*                               the client over the network.     
*       HTTP_Response_Header    Sets up the HTTP response.       
*       HTTP_Header_Name_Insert Inserts the name and value into  
*                               the output header of the HTTP    
*                               response.                        
*       HTTP_Initiate_Response  Starts the HTTP response to the  
*                               client.                          
*       HTTP_Redirect_Client    Redirects the  user agent to     
*                               another URL.                     
*       HTTP_Header_Num_Insert  Places name and numeric value    
*                               token into output header.        
*       HTTP_Get_Mime_Type      Derives a mime type from the 
*                               file extension.             
*       HTTP_Convert_To_Url     Converts the URI to full a URL.    
*       HTTP_Build_Array  		Organizes tokens sent by client
*       HTTP_Build_Cookie_Array Organizes cookies sent by client
*       HTTP_Verify_Packet      Verifies that the request header
*                               is valid
*       HTTP_Send_Status_Message Sends a status message to client
*       HTTP_Set_Cookie         Adds Set Cookie tag to header
*                               of response packet
*       HTTP_Token_Value_by_Number Retrieves token value from
*                               token array
*       HTTP_Token_Value_by_Name Retrieves token value from
*                               token array
*                                                                       
* DEPENDENCIES                                                          
*                                                                       
*       nu_websrv.h  
*       wpw_auth.h            
*                                                                       
*************************************************************************/

#include "networking/nu_websr.h"
#include "networking/wpw_auth.h"

#if ((defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE))
#include "networking/ip6.h"
#endif

STATIC STATUS       HTTP_Verify_Packet(WS_REQUEST *req);
STATIC VOID         HTTP_Process_GET(WS_REQUEST* req);
STATIC VOID         HTTP_Process_SSI(WS_REQUEST *req);
STATIC STATUS       HTTP_Is_SSI(WS_REQUEST * req);
STATIC VOID         HTTP_Process_POST(WS_REQUEST * req);
STATIC CHAR         *HTTP_Get_Mime_Type(WS_REQUEST * req);

/*  The Message Body for HTTP 400 Bad Request */
static const CHAR HTTP_Text_400[]="\
<HEAD><TITLE>400 Bad Request</TITLE></HEAD>\r\n\
<BODY><H1>400 Bad Request</H1>\r\n</BODY>";

/*  The Message Body for HTTP 404 not found */
static const CHAR HTTP_Text_404a[]="\
<HEAD><TITLE>404 Not Found</TITLE></HEAD>\r\n\
<BODY><H1>404 Not Found</H1>\r\n\
Url '";
static const CHAR HTTP_Text_404b[]="' not found on server<P>\r\n</BODY>";

/*  The Message Body for HTTP 500 Internal Server Error */
static const CHAR HTTP_Text_500a[]="\
<HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\r\n\
<BODY><H1>500 Internal Server Error</H1>\r\n\
Error: '";

static const CHAR HTTP_Text_500b[]="' </BODY>";

/*  The Message Body for HTTP 501 Not Implemented */
static const CHAR HTTP_Text_501[]="\
<HEAD><TITLE>501 Method Not Implemented</TITLE></HEAD>\r\n\
<BODY><H1>501 Method Not Implemented</H1>\r\n</BODY>";

extern WS_MIME_TABLE MIME_Mime_Table[];
extern WS_PLUGIN_STRUCT *HTTP_Plugins;

extern UINT32     WSC_Use_Hostname;
extern NU_PROTECT WS_Protect;
extern UINT8      SCK_Host_Name[];
extern UINT8      SCK_Domain_Name[];

/* Define the size of the buffer needed to hold the address
   including the NULL terminator. */
#define IPV4_ADDRS_BUFSIZE   (16)

/* Define the size of the buffer needed to hold the address
   including the NULL terminator. */
#define IPV6_ADDRS_BUFSIZE   (40)


/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Parse_Request                                               
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       We parse the HTTP request and call                               
*       the correct handler. Note that HEAD is exactly                   
*       like GET except no data is returned (only the                    
*       response header).                                               
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to the HTTP request.             
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS
*                                                                       
*************************************************************************/

VOID HTTP_Parse_Request(WS_REQUEST *req)
{
    CHAR        buf[140 + WS_URI_LEN];
    INT16       token_list[WS_TOKEN_HEAP + 1];
    INT16       cookie_list[WS_COOKIE_HEAP + 1];

    /* Initialize file attributes */
    req->ws_stat.ws_address = NU_NULL;
    req->ws_stat.ws_flags = NU_FALSE;
    req->ws_stat.ws_size = 0;
    req->ws_stat.ws_type = NU_NULL;
    
    /* Set up the pointers for the token array */
    req->ws_pg_args.ws_token_array = token_list;
    req->ws_pg_args.ws_token_array[0] = -1;
    req->ws_pg_args.ws_token_string = NU_NULL;
    
    /* Set up the pointers for the cookie array */
    req->ws_cookie_jar.ws_token_array = cookie_list;
    req->ws_cookie_jar.ws_token_array[0] = -1;
    req->ws_cookie_jar.ws_token_string = NU_NULL;
    
    /* Verifiy that the packet can be parsed correctly */
    if(HTTP_Verify_Packet(req) != NU_SUCCESS)
	{
		req->ws_pg_args.ws_token_array = NU_NULL;
		req->ws_cookie_jar.ws_token_array = NU_NULL;
        return;
	}

    /*  If referencing the root node "/" of our filesystem
     *  redirect the browser to the default URL.
     */
    if((req->ws_fname[0] == '/') && (req->ws_fname[1] == '\0'))
    {
        HTTP_Uri_To_Url(req, WS_DEFAULT_URI, buf);
        HTTP_Redirect_Client(req, buf);
		
		req->ws_pg_args.ws_token_array = NU_NULL;
		req->ws_cookie_jar.ws_token_array = NU_NULL;		
        return;
    }

    /* Check if the file exists in the system */
    if(WSF_File_Request_Status(req) == NU_SUCCESS)
    {
#if INCLUDE_SSL
        /* This check must be done before the Basic Auth check,
         * otherwise the user will log on to the page before it
         * is redirected to the SSL page, where the user will
         * log in again
         */
        if(!(req->ws_ssl) && (req->ws_stat.ws_flags & WS_PRIVATE))
        {
            HTTPS_Uri_To_Url(req, req->ws_fname, buf);
            HTTP_Redirect_Client(req, buf);
            return;
        }
        else if((req->ws_ssl) && !(req->ws_stat.ws_flags & WS_PRIVATE))
        {
            HTTP_Uri_To_Url(req, req->ws_fname, buf);
            HTTP_Redirect_Client(req, buf);
            return;
        }
#endif /* INCLUDE_SSL */
            
#if INCLUDE_BASIC_AUTH || WS_AUTHENTICATION
        
        if(req->ws_stat.ws_flags & WS_PRIVATE)
        {
            
#if INCLUDE_BASIC_AUTH
            /* Check if client is authentivated through
             * basic authentication 
             */
            if(BSC_Parse_Auth(req) != NU_SUCCESS)
                return;
#else
            
            /* check for authentication timeout */
            HTTP_Check_Timeout(req);          
            
            /* if the user has not been authenticeated
             * allow them the choice to login.
             */
            if(HTTP_Authenticated_Check(req) == WS_FAILURE) 
            {
                HTTP_Uri_To_Url(req, req->ws_server->ws_master.ws_auth_uri, buf);
                HTTP_Redirect_Client(req, buf);
                return;
            }
#endif /* INCLUDE_BASIC_AUTH */
        } 
#endif /* INCLUDE_BASIC_AUTH || WS_AUTHENTICATION */

        /* Handle the actual request */
        
        if(req->ws_method == WS_POST)
            HTTP_Process_POST(req);
        
        else if(req->ws_method == WS_GET)
            HTTP_Process_GET(req);
        else
            HTTP_Make_Mime_Header(req, req->ws_stat.ws_size);
    }
    else
    {
        /* Send the HTTP 404 Not Found Reply */
        strcpy(buf, HTTP_Text_404a);
        strcat(buf, req->ws_fname);
        strcat(buf, HTTP_Text_404b);
        
        HTTP_Send_Status_Message(req, WS_PROTO_NOT_FOUND, buf);
    }
	
	req->ws_pg_args.ws_token_array = NU_NULL;
	req->ws_cookie_jar.ws_token_array = NU_NULL;	
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Verify_Packet                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       The function parses important parts of the HTTP header and
*       verifies that what is found is valid.  If the header is not
*       valid, a message is sent to the client.                                          
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to the HTTP request.             
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       status                  Result of verification, NU_SUCCESS if
*                               the packet is valid.
*                                                                       
*************************************************************************/

STATIC STATUS HTTP_Verify_Packet(WS_REQUEST *req)
{
    CHAR HUGE   *line = req->ws_rdata.ws_in_header;
    CHAR HUGE   *temp_ptr;
    STATUS      status = NU_SUCCESS;
    CHAR        *buf_endp;
    UINT32      uri_size;
    UINT32      nbytes;


    /* VERIFY METHOD */
    switch(req->ws_method)
    {
    case WS_GET:
    case WS_POST:
    case WS_HEAD:
        break;
        
    default:
#if NU_WEBSERV_DEBUG
        printf("Request Method not implemented\n");
        printf("C:%s\n", line);
#endif 
        /*  If a method comes in that is not supported send the 501 method
         *  Not implemented message.
         */
        HTTP_Send_Status_Message(req, WS_PROTO_NOT_IMPLEMENTED, (CHAR*)HTTP_Text_501);
        status = WS_FAILURE;
    }
    
    /* VERIFY URI - Move through the request to the file name */
    if(status == NU_SUCCESS)
    {
        while (*line != ' ')
            line++;
        temp_ptr = ++line;

        /* Set File name in req structure */    
        req->ws_fname = (CHAR*)line;
        while (*line != ' ')
        {
            /* Verify the name is not too long, or has restricted characters */
            if(*line == '\r' || *line == '\n' || (UINT32)(line - temp_ptr) > WS_URI_LEN)
            {
                status = WS_FAILURE;
                break;
            }
        
            /* NULL terminate name if arguments are attached */
            if(*line == '?')
            {
                *line = 0;
                temp_ptr = ++line;
            }
            else
                line++;
        }


        /*  If the request only specifies the root node "/" of our file system,
            the next section of code will add in the default page name (URI).
            So for instance, if WS_DEFAULT_URI is defined as "index.htm" we will
            convert the request GET / HTTP/1.1 into GET /index.htm HTTP/1.1 */

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the end of the buffer. */
            buf_endp = &req->ws_rdata.ws_in_header[WS_RECEIVE_SIZE];

            /* Get the number of chars in the default uri. */
            uri_size = strlen(WS_DEFAULT_URI);

            /* We have only a '/' and a space before the "HTTP 1." */
            if ((req->ws_fname[0] == '/') && (req->ws_fname[1] == ' '))
            {
                /* Make sure we have room to insert the default URI */
                if ((line < buf_endp) && (uri_size < (buf_endp - line)))
                {
                    /* Make room for the default URI. Move the rest of the data back.
                       Note that we are losing uri_size bytes off the end of the buffer.
                       If this causes the request to become mal-formed, that will be
                       detected later. */
                    
                    /* How much to move? */
                    nbytes = (buf_endp - line) - uri_size;

                    /* Move the buffer data back. Use memmove since source and
                       destination overlap. */
                    memmove(line + uri_size, line, nbytes);

                    /* Copy in the default URI into the request. */
                    memcpy(line, WS_DEFAULT_URI, uri_size);

                    /* Advance the pointer to the byte after the default URI */
                    line += uri_size;
                }
            }
        }


        if(status == NU_SUCCESS)
        {
            /* NULL terminate the data */
            *line++ = NU_NULL;


            /* The HTTP version has already been verified in the worker task,
             * just verify that the <crlf> follows this first line
             */
            line += 8;
            if(line[0] != '\r' || line[1] != '\n')
                status = WS_FAILURE;
        }

        if(status == NU_SUCCESS)
        {
            line += 2;
    
            /*  If a method comes in from an HTTP 1.1 client without the Host tag,
             *  it is necessary to return a Bad Request response.
             */
            if((*(req->ws_http_ver) == WS_HTTP_11) && 
                (WS_Find_Token_by_Case("Host: ", line, &req->ws_rdata.ws_in_header[req->ws_rdata.ws_in_header_sz], WS_TOKEN_CASE_INSENS) == 0))
                status = WS_FAILURE;
        }

        if(status == WS_FAILURE)
        {
#if NU_WEBSERV_DEBUG
            printf("Invalid request header\n");
            printf("C:%s\n", line);
#endif 
            HTTP_Send_Status_Message(req, WS_PROTO_BAD_REQUEST, (CHAR*)HTTP_Text_400);
        }        
    }
    
    if(status == NU_SUCCESS)
    {
        /* Search for any attached tokens */
        if(*temp_ptr)
        {
            HTTP_Build_Array(temp_ptr, &req->ws_pg_args, NU_NULL);
        }
    }
    
    return(status);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Process_GET                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       The GET request specifies a file desired by the client.
*       This is the most widely used of the HTTP options.                                            
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to the HTTP request.             
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID  HTTP_Process_GET(WS_REQUEST* req)
{
    INT             status;
    CHAR            buf[200];

#if NU_WEBSERV_DEBUG
    printf("GET:%s\n", req->ws_fname);
#endif
    
    /* Check if this is a plugin */
    if(req->ws_stat.ws_flags == WS_PLUGIN)
    {
        /* Call the plugin */
        status = (req->ws_stat.plugin)(req);
        
        /* Post plugin error check */
        if(status != WS_REQ_PROCEED)
        {
            /* Clear the rdata structure */
            WS_Clear_Rdata(req);
            
            strcpy(buf, HTTP_Text_500a);
            strcat(buf, "Plugin Failure");
            strcat(buf, HTTP_Text_500b);
            
            HTTP_Send_Status_Message(req, WS_PROTO_SERVER_ERROR, buf);
        }
    }
    else
    {
        /* check for server side include */ 
        if(HTTP_Is_SSI(req) == NU_SUCCESS) 
            HTTP_Process_SSI(req);
        else
            /* If we have gotten this far, serve the file */
            HTTP_Serve_File(req);
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Serve_File                                                        
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function sends a file to the client(web browser).           
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID  HTTP_Serve_File(WS_REQUEST *req)
{
    /* Generate the response header for the trasfer */
    HTTP_Make_Mime_Header(req, req->ws_stat.ws_size);
    
#if INCLUDE_JS_AUTH
    if(req->ws_stat.ws_flags & WS_SCRIPT)
        JS_Alter_ID_Script(req, WS_FILETRNSFR);
#endif

    if(req->ws_stat.ws_flags & WS_INCORE)  
    {
        /* an in-memory file...simply send down to the socket */
        
#if INCLUDE_COMPRESSION
        /* Check if the compression identifier is at the beginning
         * of the file.  Be sure not to inclued the \0 in the compare.
         */
        if(strncmp(req->ws_stat.ws_address, WS_CHD, WS_CHD_SZ) != 0)
        {
            if(WSN_Write_To_Net(req, req->ws_stat.ws_address, 
                            (UINT32)req->ws_stat.ws_size, WS_FILETRNSFR) != NU_SUCCESS)
                NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
        }
        else
            CFS_Decompress(req, req->ws_stat.ws_address + 4, NU_NULL,
                            req->ws_stat.ws_size - 4);
#else
        if(WSN_Write_To_Net(req, req->ws_stat.ws_address, 
                            (UINT32)req->ws_stat.ws_size, WS_FILETRNSFR) != NU_SUCCESS)
            NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
#endif
    }

#if INCLUDE_FILE_SYSTEM
    else
    {
        /* Get the file from external storage and send it */
        if(WSF_Send_File(req) != NU_SUCCESS)
            NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
    }
#endif
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Process_SSI                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function processes server side includes. Note compressed    
*       files are not handled within this routine.                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID  HTTP_Process_SSI(WS_REQUEST *req)
{
    CHAR        *file;
    CHAR HUGE   *insert;
    CHAR HUGE   *p;
    CHAR HUGE   *q;
    CHAR        plugin_s[WS_SSI_NAME_LEN];
    CHAR HUGE   *last_start;
    INT         (*plugin)(WS_REQUEST *);
    INT         i;
    UINT32      ssilen;
    CHAR HUGE   *last_byte;
    CHAR        *ssi_buf = req->ws_ssi_buf;
    CHAR        buf[200];
    STATUS      status = NU_SUCCESS;
    WS_PLUGIN_STRUCT *pointer;
    INT16       ssi_list[WS_SSI_HEAP + 1];


    req->ws_ssi_args.ws_token_array = ssi_list;
    req->ws_ssi_args.ws_token_array[0] = -1;
    req->ws_ssi_args.ws_token_string = NU_NULL;

    /* Get the SSI File */
    file = req->ws_stat.ws_address;
    
#if INCLUDE_FILE_SYSTEM
    if((req->ws_stat.ws_flags & WS_INCORE) == 0)         
    {
        /* this file has to be read in */
        status = NU_Allocate_Memory(req->ws_server->ws_memory_pool, (VOID*)&file,
                                    (UNSIGNED)req->ws_stat.ws_size, NU_NO_SUSPEND);
        
        if(status != NU_SUCCESS)
        {
            strcpy(buf, HTTP_Text_500a);
            strcat(buf, "SSI Memory Failure");
            strcat(buf, HTTP_Text_500b);
            
            HTTP_Send_Status_Message(req, WS_PROTO_SERVER_ERROR, buf);
            return;
        }

        status = WSF_Read_File(req, file); 
    }
#endif
    
    /* Get a pointer to the end of the file */
    last_byte = (CHAR*)((CHAR HUGE*)file + req->ws_stat.ws_size);
    
    /* setup all the protocol for a transfer */
    HTTP_Response_Header(req, WS_PROTO_OK);
    HTTP_Header_Name_Insert(req, WS_CONTENT_TYPE, WS_TYPE_TXT_HTML);

    /* Prevent caching of this dynamic file */
    HTTP_Header_Name_Insert(req, WS_PRAGMA, "no-cache");
    HTTP_Header_Name_Insert(req, "Expires: ", "0");
    HTTP_Header_Name_Insert(req, "Cache-Control: ", "no-cache");
    
#if INCLUDE_JS_AUTH
    /* If this is a secure page, send the script */
    if((req->ws_stat.ws_flags & WS_PRIVATE) && (status == NU_SUCCESS))
        JS_Alter_ID_Script(req, WS_PLUGIN_DATA);
#endif
    
    last_start = file;

    /* Find the SSI token within the file */
    insert = WS_Find_Token(WS_SSI_TOKEN, last_start, (CHAR*)last_byte);
    
    while(insert && (status == NU_SUCCESS))
    {
        p = insert;
        q = ssi_buf;
        plugin = NU_NULL;

        /* Move past the SSI tag */
        p += sizeof(WS_SSI_TOKEN) - 1;
        
        /* copy tag and any tokens into buffer */
        while(*p != '>' && ((p - insert) < WS_SSI_LINE))
            *q++ = *p++;
        
        *q ='\0';

        /* size of the ssi tag */
        ssilen = q - ssi_buf;               
        
        /* Get the plugin name */
        q = plugin_s;
        p = ssi_buf;

        while(*p && *p != ' ')
            *q++ = *p++; 
        *q = '\0';

        for(q = p; *q == ' '; q++);

        /* does it look like NAME=VALUE */
        if(WS_In_String("~", q))
        {
            while(*p != '~')
                p++;                        
            
            /* terminate argument string */
            *p = '\0'; 
        }

        /* process the arguments */
        HTTP_Build_Array(q, &req->ws_ssi_args, WS_SSI);  
        
        /* lookup the plugin in the table */
        p = plugin_s;
        while(*p == '/')
            p++;
        
        pointer = HTTP_Plugins;
        while(pointer)
        {
            if(WS_Strcmp((CHAR*)p, pointer->ws_name) == 0 )
            {
                plugin =  pointer->plugin;
                pointer = NU_NULL;
            }
            else
                pointer = pointer->ws_next;
        }
        
        /* Write Plugin Data */
        status = WSN_Write_To_Net(req, last_start, (UINT32)(insert - last_start), WS_PLUGIN_DATA);

        /* Check for an erroneous return value */
        if(status != NU_SUCCESS)
        {
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
            continue;
        }

        /* If the plugin does not exist, write the error to the page */
        if(plugin == NU_NULL)
        {
            strcpy(buf, plugin_s);
            strcat(buf," CANT FIND SSI plugin");
            status = WSN_Write_To_Net(req, buf, (UINT32)strlen(buf), WS_PLUGIN_DATA);
        }
        else
        {
            i = (plugin)(req);
            
            /* Post plugin error check */
            if(i != WS_REQ_PROCEED)
            {
                /* Clear the rdata structure */
                WS_Clear_Rdata(req);
                
                strcpy(buf, HTTP_Text_500a);
                strcat(buf, "Plugin Failure");
                strcat(buf, HTTP_Text_500b);
                HTTP_Send_Status_Message(req, WS_PROTO_SERVER_ERROR, buf);
                
                if((req->ws_stat.ws_flags & WS_INCORE) == 0)         
                    if(NU_Deallocate_Memory(file) != NU_SUCCESS)
                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                return;
            }
        }

        /* 5 is size of ssi token marker <!-# ... > */
        last_start = insert + ssilen + 5;
        
        /* Clear out the token array */
        req->ws_ssi_args.ws_token_array[0] = -1;
        req->ws_ssi_args.ws_token_string = NU_NULL;
        
        insert = (WS_Find_Token(WS_SSI_TOKEN, last_start, (CHAR*)last_byte));
    }
    
    /* write out last piece if any */
    if((last_start < last_byte) && (status == NU_SUCCESS))
        status = WSN_Write_To_Net(req, last_start, (UINT32)(last_byte - last_start),
                        WS_PLUGIN_DATA);

    /* Send the Data */
    if(status == NU_SUCCESS)
        if(WSN_Write_To_Net(req, NU_NULL, 0, WS_PLUGIN_SEND) != NU_SUCCESS)
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);

    if((req->ws_stat.ws_flags & WS_INCORE ) == 0)
        if(NU_Deallocate_Memory(file) != NU_SUCCESS)
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
			
	/* Don't allow this to continue to point to local memory. */
	req->ws_ssi_args.ws_token_array = NU_NULL;			
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Is_SSI                                                           
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       The function checks to see if the current file name is a file    
*       marked for server side include processing.                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              ssi tag found
*       WS_FAILURE              Cannot find ssi tag  
*                                                                       
*************************************************************************/
STATIC STATUS HTTP_Is_SSI(WS_REQUEST *req)
{
    if( (WS_In_String(WS_SSI_LC_MARKER, req->ws_fname)) != NU_NULL )
        return(NU_SUCCESS);
    else if( (WS_In_String(WS_SSI_UC_MARKER, req->ws_fname)) != NU_NULL )
        return(NU_SUCCESS);
    else
        return(WS_FAILURE);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Process_POST                                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       The POST method specifies a URI and                              
*       includes optional user input data from                           
*       HTML forms screens.  Plugin's  can be launched                   
*       through here.                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
STATIC VOID  HTTP_Process_POST(WS_REQUEST *req)
{
    INT     status;
    CHAR    buf[200];
    
    /* Search for any tokens within the HTTP header */
    if((WS_Find_Token("multipart/form-data", req->ws_rdata.ws_in_header,
            &req->ws_rdata.ws_in_header[req->ws_rdata.ws_in_header_sz])) == NU_NULL)
        HTTP_Build_Token_Array(req->ws_rdata.ws_in_data, &req->ws_pg_args);

    /* Check if this is a plugin */
    status = WS_REQ_ABORTED;
    if (req->ws_stat.ws_flags & WS_PLUGIN)
    {
        /* Call the plugin */
        status = ((req->ws_stat.plugin)(req));
    }

    /* Post plugin error check */
    if(status != WS_REQ_PROCEED)
    {
        /* Clear the rdata structure */
        WS_Clear_Rdata(req);
        
        strcpy(buf, HTTP_Text_500a);
        strcat(buf, "Plugin Failure");
        strcat(buf, HTTP_Text_500b);
        
        HTTP_Send_Status_Message(req, WS_PROTO_SERVER_ERROR, buf);
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Build_Array
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function Parses and converts the clients plug-in            
*       arguments into individual strings and adds pointers to 
*       the buffer into the token array.
*                                                                       
* INPUTS                                                                
*                                                                       
*       buffer                  String that holds the clients    
*                               plugin arguements.         
*       token_info              Pointer to the array of tokens
*                                                                       
* OUTPUTS                                                               
*                                                                       
*                                                                       
*************************************************************************/
VOID HTTP_Build_Array(CHAR HUGE *buffer, WS_TOKEN_INFO *token_info, UINT8 mode)
{
    UINT16      buf_count = 0;
    UINT16      arr_count = 0;
    INT16       *token_array;
    UINT16      heap;

    if (mode == WS_SSI)
        heap = WS_SSI_HEAP;
    else
        heap = WS_TOKEN_HEAP;

    if(buffer && token_info)
    {
        buffer = TLS_Normalize_Ptr(buffer);
    
        token_array = token_info->ws_token_array;
        token_info->ws_token_string = (CHAR*)buffer;

        /* Be sure to add token to end of list */
        while((token_array[arr_count] != -1) && (arr_count < heap))
            arr_count++;

        /* To prevent including a line feed or cariage return in
         * the array, we must state that such characters end the
         * buffer.
         */
        while((buffer[buf_count] > 31) && (arr_count < heap))
        {
            token_array[arr_count++] = buf_count;

            while((buffer[buf_count] > 31) && (buffer[buf_count] != '='))
            {
                /* A space is represented by a '+' */
                if(buffer[buf_count] == '+')
                    buffer[buf_count] = ' ';
                buf_count++;
            }

            if(buffer[buf_count] > 31)
                buffer[buf_count++] = '\0';
            else
                break;

            /* Set a null terminator between the name and value
             * to allow for easy retrieval later
             */
            while((buffer[buf_count] > 31) && (buffer[buf_count] != '&'))
            {
                /* A space is represented by a '+' */
                if(buffer[buf_count] == '+')
                    buffer[buf_count] = ' ';
                buf_count++;
            }

            /* Verify that this is not a terminating character */
            if(buffer[buf_count] > 31)
                buffer[buf_count++] = '\0';
            else
                buffer[buf_count] = '\0';
        }

        token_array[arr_count] = -1;
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Token_Value_by_Name                                                 
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function finds the value associated with a token name.  It compares     
*       a name string until it finds the correct token name and returns  
*       the token value.  If the name is not found, NULL is returned   
*                                                                       
* INPUTS                                                                
*                                                                       
*       *name                   Name of the token name to find.  
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request. 
*       mode                    Specifies which token array is
*                               passed in.           
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       *token_string[index]   The token value
*       NULL
*                                                                       
*************************************************************************/
CHAR* HTTP_Value_by_Name(CHAR *name, WS_REQUEST *req, UINT8 mode)
{
    UINT16          count;
    WS_TOKEN_INFO   *token_ptr;
    INT16           index;
    UINT8           flag = 1;
    UINT16          heap;

    /* Remove any warnings */
    UNUSED_PARAMETER(mode);

#if INCLUDE_COOKIES
    if(mode == WS_TOKEN_COOKIE)
    {
        /* Set up pointers to the cookie data */
        token_ptr = &(req->ws_cookie_jar);
        if(!token_ptr->ws_token_string)
            HTTP_Build_Cookie_Array(req);
        heap = WS_COOKIE_HEAP;
    }
    else
#endif
    {
        /* Set up pointers to the plugin data */
        token_ptr = &(req->ws_pg_args);
        if(HTTP_Is_SSI(req) == NU_SUCCESS)
            flag = 2;
        heap = WS_TOKEN_HEAP;
    }

    for(; flag; flag--)
    {
        /* Begin the search for the token name */
        if(token_ptr->ws_token_string)
        {
            for(count = 0; ((token_ptr->ws_token_array)[count] != -1) &&
                (count < heap); count++)
            {
                index = (token_ptr->ws_token_array)[count];
                
                /* Check each token name for a match */
                if(strcmp(name, (CHAR *)&(token_ptr->ws_token_string)[index]) == 0)
                {
                    /* A match was found, move past name to token value */
                    index += strlen(name) + 1;
                    
                    /* Return the token value */
                    return((CHAR *)&(token_ptr->ws_token_string)[index]);
                }
            }
        }

        token_ptr = &(req->ws_ssi_args);
    }
    
    /* There was an error in processing, return a null pointer */
    return(NU_NULL);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Token_Value_by_Number                                                 
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function finds the value associated with a token number.  
*       The number is the position the token name and value occur
*       when sent by the client.  If the name is not found, 
*       NULL is returned   
*                                                                       
* INPUTS                                                                
*                                                                       
*       number                  Position of token in buffer.  
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*       mode                    Specifies which token array is
*                               passed in.           
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       *token_string[index]   The token value
*       NULL
*                                                                       
*************************************************************************/
CHAR* HTTP_Token_Value_by_Number(INT number, WS_REQUEST *req, UINT8 mode)
{
    CHAR HUGE   *token_string;
    INT16       index;
    INT16       *token_array;
    INT16      heap;
    
    if(mode == WS_SSI)
    {
        token_array = req->ws_ssi_args.ws_token_array;
        token_string = req->ws_ssi_args.ws_token_string;
        heap = WS_SSI_HEAP;
    }
    else
    {
        token_array = req->ws_pg_args.ws_token_array;
        token_string = req->ws_pg_args.ws_token_string;
        heap = WS_TOKEN_HEAP;
    }
    
    /* Find the begining of the token arguments */
    if(token_string && (number < heap))
    {
        /* Make sure number is valid */
        for (index = 0; ((index < number) && (token_array[index] != -1)); index++);

        if (token_array[index] != -1)
        {
            index = token_array[number];
            
            /* Move past the name to the token value */
            while(token_string[index])
                index++;
            index++;
            
            /* Return the token value */
            return( (CHAR *)&token_string[index] );
        }
    }

    /* There was an error in processing, return a null pointer */
    return(NU_NULL);
}

#if INCLUDE_COOKIES
/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Set_Cookie                                               
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function places a Set-Cookie directive within the header.
*       It sets name = value.
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*       *name                   Name of the cookie.               
*       *value                  Value of the cookie.      
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/

VOID  HTTP_Set_Cookie(WS_REQUEST *req, CHAR *name, CHAR *value)
{
    CHAR    token[WS_COOKIE_LEN];

    if(strlen(name) + strlen(value) < WS_COOKIE_LEN)
    {
        strcpy(token, name);
        strcat(token, "=");
        strcat(token, value);

        HTTP_Header_Name_Insert(req, WS_SET_COOKIE, token);
    }
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Build_Cookie_Array                                               
*                                                                       
* DESCRIPTION                                                           
*
*       This function builds the WS_TOKEN_INFO structure with any
*       cookies that it finds within the HTTP packet
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID HTTP_Build_Cookie_Array(WS_REQUEST *req)
{
    CHAR HUGE   *line;
    INT16       *token_array;
    INT16       buf_count = 0;
    INT16       arr_count = 0;
    INT16       position;

    req->ws_cookie_jar.ws_token_string = 
        WS_Find_Token(WS_COOKIE_HEADER, req->ws_rdata.ws_in_header, 
                        (CHAR*)(req->ws_rdata.ws_in_header + req->ws_rdata.ws_in_header_sz));

    if(req->ws_cookie_jar.ws_token_string)
    {
        req->ws_cookie_jar.ws_token_string += sizeof(WS_COOKIE_HEADER) - 1;
        
        line = TLS_Normalize_Ptr(req->ws_cookie_jar.ws_token_string);

        token_array = req->ws_cookie_jar.ws_token_array;
           
        /* To prevent including a line feed or cariage return in
         * the array, we must state that such characters end the
         * buffer.
         */
        while((line[buf_count] > 31) && (arr_count < WS_COOKIE_HEAP))
        {
            token_array[arr_count++] = buf_count;
            position = buf_count;
            
            while((line[buf_count] > 31) && (line[buf_count] != '='))
            {
                /* Check if the character is a control character.  
                 * This is denoted with a %.  These are replaced with a space 
                 */
                if(line[buf_count] == '%')
                {
                    line[position++] = ' ';
                    buf_count += 3;
                }
                else
                    line[position++] = line[buf_count++];
            }

            /* Set a null terminator between the name and value
             * to allow for easy retrieval later
             */
            if(line[buf_count] > 31)
            {
                line[position] = '\0';
                buf_count++;
            }
            else
                break;
            
            position = buf_count;
            
            while((line[buf_count] > 31) && (line[buf_count] != ';') &&
                    (line[buf_count] != ','))
            {
                /* Check if the character is a control character.  
                 * This is denoted with a %.  These are replaced with a space 
                 */
                if(line[buf_count] == '%')
                {
                    line[position++] = ' ';
                    buf_count += 3;
                }
                else
                    line[position++] = line[buf_count++];
            }

            /* Verify that this is not a terminating character */
            if(line[buf_count] > 31)
            {
                line[position] = '\0';
                buf_count++;
            }
            else
                line[position] = '\0';

            buf_count++;
        }
        
        token_array[arr_count] = -1;
    }
}

#endif /* INCLUDE_COOKIES */

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Header_Num_Insert                                               
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function places a number into the value of slot of the   
*       output header.                                                   
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*       *name                   Name of the token.               
*       value                   Numeric Value of the token.      
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID  HTTP_Header_Num_Insert(WS_REQUEST *req, CHAR *name, INT32 value)
{
    /* The maximum character length of an INT32 is 10 */
    CHAR num[12];
    CHAR temp[10];
     
    strcpy(num, NU_ULTOA((unsigned long)value, temp, 10));
    HTTP_Header_Name_Insert(req, name, num);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Header_Name_Insert                                                 
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to insert the name and value into the output           
*       header.                                                          
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*       *name                   The name token.                  
*       *value                  The value token.                 
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID  HTTP_Header_Name_Insert(WS_REQUEST *req, CHAR *name, CHAR *value)
{
    /* Set the name */
    strcat(req->ws_rdata.ws_out_header, name);

    /* Set the value */
    strcat(req->ws_rdata.ws_out_header, value);

    /* Finish the insert */
    strcat(req->ws_rdata.ws_out_header, "\r\n");
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Response_Header                                                  
*                                                                       
* DESCRIPTION                                                           
*       
*       This function builds the header for the HTTP response
*       depending on the code supplied by the calling function.                                                         
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*       code                    Code that is used to descibe the 
*                               prototype status.                
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID  HTTP_Response_Header(WS_REQUEST *req, INT code)
{
    CHAR temp[10];

    /* Use the correct version for this HTTP connection. */
    if (*(req->ws_http_ver) == WS_HTTP_10) 
        strcpy(req->ws_rdata.ws_out_header,"HTTP/1.0 ");
    else
        strcpy(req->ws_rdata.ws_out_header,"HTTP/1.1 ");
    
    strcat(req->ws_rdata.ws_out_header, (CHAR *)NU_ITOA(code, temp, 10));
    strcat(req->ws_rdata.ws_out_header, " Document Follows");
    strcat(req->ws_rdata.ws_out_header, "\r\n");
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Initiate_Response                                          
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to start the http response.  It takes the compiled      
*       header with the mode whether FILETRANSFR or WS_PLUGIN_PROTO and      
*       sends it out over the network.                                   
*                                                                       
* INPUTS                                                                
*                                                                       
*       req                     Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*       mode                    Either File transfer or          
*                               WS_PLUGIN_PROTO                      
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID  HTTP_Initiate_Response(WS_REQUEST *req, INT mode)
{
    strcat(req->ws_rdata.ws_out_header, "\r\n");
    if(WSN_Write_To_Net(req, req->ws_rdata.ws_out_header, 
                    (UINT32)strlen(req->ws_rdata.ws_out_header), mode) != NU_SUCCESS)
        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
    
#if NU_WEBSERV_DEBUG
    printf("outhdr =\n%s\n", req->ws_rdata.ws_out_header);
#endif 
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Redirect_Client                                                
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Send the user agent (browser) to another URL (may                
*       or may not be on this server).                                   
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*       *url                    The URL of where the response    
*                               is to point to.                  
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID  HTTP_Redirect_Client(WS_REQUEST *req, CHAR *url)
{
    HTTP_Response_Header(req, WS_PROTO_REDIRECT);
    HTTP_Header_Name_Insert(req, WS_CONTENT_LOCATION, url);
    strcat(req->ws_rdata.ws_out_header, "Connection: close\r\n");
    HTTP_Initiate_Response(req, WS_REDIRECTION);

    /* Change this flag so that the connection will be closed */
    *(req->ws_http_ver) = WS_HTTP_10;
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Convert_To_Url                                             
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Convert a URI (local pathname) into a full URL spec             
*       (http://xxx.xxx.xxx.xxx/uri)                                    
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that       
*                               holds all information pertaining        
*                               to  the HTTP request.                   
*       *uri                    The name of the file to be              
*                               retrieved.                              
*       *url_buf                Buffer in which the new URL will        
*                               be placed.                              
*       ssl_flag                Flag specifying whether new URL         
*                               should be SSL based.                    
*       url_buf_sz              Size of the url_buf buffer. If this     
*                               value is zero, the default URL buffer   
*                               size is used.                           
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       none                                                            
*                                                                       
*************************************************************************/
VOID HTTP_Convert_To_Url(WS_REQUEST *req, CHAR *uri, CHAR *url_buf, 
                         OPTION ssl_flag, UINT16 url_buf_sz)
{
    CHAR    *protocol;
    CHAR    *beg_slash;
    CHAR    *add_dot;
    CHAR    *hostname;
    CHAR    *domainname;
    UINT32  use_hostname;
    STATUS  status;


    /* Assume failure. */
    status = WS_FAILURE;

    /* A zero url_buf_sz indicates use the default size. */
    if (!url_buf_sz)
        url_buf_sz = WS_URL_LEN;

    /* Determine which protocol string to use. */
    if (ssl_flag)
        protocol = "https://";
    else
        protocol = "http://";

    /* Determine if we need to add a slash to the URI. */
    if (uri[0] != '/')
        beg_slash = "/";
    else
        beg_slash = "";

    /* Get pointers to our hostname and domain name. */
    hostname   = (CHAR *)&SCK_Host_Name[0];
    domainname = (CHAR *)&SCK_Domain_Name[0];

    /* Determine if we need to add a dot to the hostname.domainname. */
    if (*domainname)
        add_dot = ".";
    else
        add_dot = "";

    /* Add the server's address to the URL buffer.  Check if we are
       configured to use the hostname or the IP Address. */

    NU_Protect(&WS_Protect);

    /* Get the configuration info for using hostname addresses. */
    use_hostname = WSC_Use_Hostname;

    NU_Unprotect();

    /* Check if we need to use hostname address. */
    if (use_hostname == NU_TRUE)
    {
        /* Here we are going to use the hostname as the server's address.
           http://hostname.domainname/uri
           htts://hostname.domainname/uri
           Make sure that everything will fit into the buffer. */
        if ((strlen(protocol) + strlen(hostname) + strlen(add_dot) +
             strlen(domainname) + strlen(beg_slash) + strlen(uri)) < url_buf_sz-1)
        {
            /* Add the protocol string to the buffer. */
            strcpy(url_buf, protocol);

            /* add the hostname to the buffer. */
            strcat(url_buf, hostname);

            /* add the dot. */
            strcat(url_buf, add_dot);

            /* add the domain name to the buffer. */
            strcat(url_buf, domainname);

            status = NU_SUCCESS;
        }

    }
    else
    
    if (req->ws_family == NU_FAMILY_IP)
    {
        /* Here we are going to use the IP literal address as the
           server's address. Total length of the address string is 16
           bytes including the NULL terminator.
           http://nnn.nnn.nnn.nnn/uri
           https://nnn.nnn.nnn.nnn/uri
           Make sure that everything will fit into the buffer.
           (Note: buffer sizes include room for the NULL terminator,
           so subtract 1 to find the number of characters.) */
        if ((strlen(protocol) + (IPV4_ADDRS_BUFSIZE-1) +
             strlen(beg_slash) + strlen(uri)) < url_buf_sz-1)
        {
            /* Add the protocol string to the buffer. */
            strcpy(url_buf, protocol);

            /* Add the IP literal address to the buffer. */
            status = NU_Inet_NTOP(NU_FAMILY_IP, &req->ws_server_ip[0],
                         &url_buf[strlen(url_buf)], IPV4_ADDRS_BUFSIZE);

        }

    }

#if ((defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE))
    else
    {

        if (IPV6_IS_ADDR_V4MAPPED(req->ws_server_ip))
        {
            /* Here we are going to use the IP literal address as the
               server's address. Total length of the address string is 16
               bytes including the NULL terminator.
               http://nnn.nnn.nnn.nnn/uri
               https://nnn.nnn.nnn.nnn/uri
                       123456789012345
               Make sure that everything will fit into the buffer.
               (Note: buffer sizes include room for the NULL terminator,
               so subtract 1 to find the number of characters.) */
            if ((strlen(protocol) + (IPV4_ADDRS_BUFSIZE-1) +
                 strlen(beg_slash) + strlen(uri)) < url_buf_sz-1)
            {
                /* Add the protocol string to the buffer. */
                strcpy(url_buf, protocol);

                /* Add the IP literal address to the buffer. */
                status = NU_Inet_NTOP(NU_FAMILY_IP, &req->ws_server_ip[12],
                             &url_buf[strlen(url_buf)], IPV4_ADDRS_BUFSIZE);
            }

        }
        else
        {
            /* Here we are going to use the IPv6 literal address as the
               server's address. (See RFC 2732)
               Total length of the address string is 40 bytes including
               the NULL terminator.  Plus two for the square brackets.
               http://[nnnn:nnnn:nnnn:nnnn:nnnn:nnnn:nnnn:nnnn]/uri
               https://[nnnn:nnnn:nnnn:nnnn:nnnn:nnnn:nnnn:nnnn]/uri
                       12345678901234567890123456789012345678901
                                1         2         3         4
               Make sure that everything will fit into the buffer.
               (Note: buffer sizes include room for the NULL terminator,
               so subtract 1 to find the number of characters.) */
            if ((strlen(protocol) + (IPV6_ADDRS_BUFSIZE+2-1) +
                 strlen(beg_slash) + strlen(uri)) < url_buf_sz-1)
            {
                /* Add the protocol string to the buffer. */
                strcpy(url_buf, protocol);
                strcat(url_buf, "[");
                status = NU_Inet_NTOP(req->ws_family, req->ws_server_ip,
                             &url_buf[strlen(url_buf)], IPV6_ADDRS_BUFSIZE);
                strcat(url_buf, "]");
            }

        }

    }
#endif


    if (status == NU_SUCCESS)
    {
        /* We have already added the protocol and address information to
           the buffer, and we have already checked that the slash and uri
           will fit - go ahead and add them to the buffer. */
           
        /* Add the leading slash if needed. */
        strcat(url_buf, beg_slash);

        /* Add the URI to the buffer to complete the full URL. */
        strcat(url_buf, uri);
    }
}


/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Send_Status_Message                                                     
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This routine will send an HTTP status message to the client.       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                      Pointer to Request structure that
*                                 holds all information pertaining 
*                                 to  the HTTP request.            
*       code                      Status code  
*       *mes                      Message to be sent.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/

VOID  HTTP_Send_Status_Message(WS_REQUEST *req, INT code, CHAR *mes)
{
    /*  Load the HTTP Entity header */
    HTTP_Response_Header(req, code);
    HTTP_Header_Name_Insert(req, WS_CONTENT_TYPE, WS_TYPE_TXT_HTML);
    
    /*  Write the HTTP Entity Data */
    if(WSN_Write_To_Net(req, mes, (UINT32)strlen(mes), WS_PLUGIN_DATA) == NU_SUCCESS)
    {
        /*  Let the WS_Write_To_Net routine know that it is OK to write */
        if(WSN_Write_To_Net(req, 0, 0, WS_PLUGIN_SEND) != NU_SUCCESS)
            NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
    }
    else
        NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
    
#if NU_WEBSERV_DEBUG
    printf("Server Message: %s\n", mes);
#endif
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Client_Error                                                     
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This routine is present for future use as a hook                 
*       for error logging.                                               
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*       *reason                 Message to explain error.        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID  HTTP_Client_Error(WS_REQUEST *req, CHAR *reason)
{
    /*  Remove warnings */
    UNUSED_PARAMETER(req);
    UNUSED_PARAMETER(reason);
    NERRS_Log_Error (NERR_INFORMATIONAL, __FILE__, __LINE__);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Make_Mime_Header                                                  
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function that generates a HTTP response header with correct      
*       mime codes.                                                      
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*       length                  The size of file that is going   
*                               to be transferred.               
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID HTTP_Make_Mime_Header(WS_REQUEST *req, INT32 length)
{
    CHAR    *mime;

    /* Get the mime type from the file extension */
    mime = HTTP_Get_Mime_Type(req);

    if(*mime)
    {
        HTTP_Response_Header(req, WS_PROTO_OK);
        HTTP_Header_Name_Insert(req, WS_CONTENT_TYPE, mime);

        /* If this is a secure file, prevent caching */
        if(req->ws_stat.ws_flags & WS_PRIVATE)
            HTTP_Header_Name_Insert(req, WS_PRAGMA, "no-cache");
        
#if INCLUDE_JS_AUTH
        if(req->ws_stat.ws_flags & WS_SCRIPT)
            length += JS_Alter_ID_Script(NU_NULL, NU_NULL);
#endif

        HTTP_Header_Num_Insert(req, WS_CONTENT_LENGTH, length);
        HTTP_Initiate_Response(req, WS_FILETRNSFR);
    }
}


/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Get_Mime_Type                                                     
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function that derives a mime type from the file extension. The   
*       mime type tells the browser what kind of file it's getting       
*       based on the extension.                
*                                                                       
* INPUTS                                                                
*                                                                       
*       *req                    Pointer to Request structure that
*                               holds all information pertaining 
*                               to  the HTTP request.            
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       mime_table              Returns the  mime type in the    
*                               table.                           
*                                                                       
*************************************************************************/
STATIC CHAR *HTTP_Get_Mime_Type(WS_REQUEST *req)
{
    CHAR    *ptr;
    INT     i;
    
    /* file name */
    ptr = req->ws_fname;
    while( *ptr && (*ptr != '.') )
        ptr++;
    
    /* no extension use the default mimetype */
    if( *ptr == NU_NULL) 
        return( MIME_Mime_Table[WS_DEFAULT_MIME].ws_mime_type );
    
    /* search the mime table */
    if( *ptr == '.' )
    {
        ptr++;
        for( i = 0; MIME_Mime_Table[i].ws_ext; i++)
            if((NU_STRICMP(ptr, MIME_Mime_Table[i].ws_ext)) == 0)
            {
                if((i < 4) && (req->ws_stat.ws_flags & WS_PRIVATE))
                    req->ws_stat.ws_flags |= WS_SCRIPT;
                return(MIME_Mime_Table[i].ws_mime_type);
            }
    }
    
    if(req->ws_stat.ws_flags & WS_PRIVATE)
        req->ws_stat.ws_flags |= WS_SCRIPT;

    return(MIME_Mime_Table[WS_DEFAULT_MIME].ws_mime_type);
}

