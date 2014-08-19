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
*       http_lite_svr.c
*
*   COMPONENT
*
*       Nucleus HTTP Lite Server
*
*   DESCRIPTION
*
*       This file holds the HTTP Lite Server routines.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       HTTP_Lite_Receive_Task
*       HTTP_Lite_Svr_Create_Socket
*       HTTP_Lite_Process_Client
*       HTTP_Lite_Build_Token_Array
*       HTTP_Lite_Receive_Header
*       HTTP_Lite_Find_Token
*       HTTP_Lite_Response_Header
*       HTTP_Lite_Header_Name_Insert
*       HTTP_Lite_Send_Status_Message
*       HTTP_Lite_Parse_URI
*       HTTP_Lite_Get_Plugin
*       HTTP_Lite_Get_Upgrade_Plugin
*       HTTP_Lite_Svr_Send
*       NU_HTTP_Lite_Configure_SSL
*
*   DEPENDENCIES
*
*       http_lite.h
*       nu_networking.h
*
************************************************************************/

#include "networking/nu_networking.h"
#include "os/networking/http/inc/http_lite_int.h"
#include "services/reg_api.h"

extern HTTP_PLUGIN_LIST         HTTP_Lite_Plugins;
extern HTTP_SVR_SESSION_STRUCT  *HTTP_Session;
extern NU_SEMAPHORE             HTTP_Lite_Resource;

STATIC VOID                 HTTP_Lite_Process_Client(VOID);
STATIC STATUS               HTTP_Lite_Receive_Header(VOID);
STATIC CHAR                 *HTTP_Lite_Parse_URI(CHAR*, INT);
STATIC VOID                 HTTP_Lite_Build_Token_Array(HTTP_TOKEN_INFO *, UINT32);
STATIC STATUS               HTTP_Lite_Svr_Create_Socket(INT *, INT);

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Receive_Task
*
*   DESCRIPTION
*
*       The main server function.  It processes a single client connection
*       at a time.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
************************************************************************/
VOID HTTP_Lite_Receive_Task(UNSIGNED argc, VOID *argv)
{
    INT                 max_socketd;
    struct addr_struct  client_addr;
    FD_SET              readfs;
    STATUS              status = NU_SUCCESS;

    /* Remove any warnings */
    UNUSED_PARAMETER(argv);

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)

    /* Create a context structure for CyaSSL.  Do not wait for network
     * initialization to complete since the application needs to configure
     * this context as soon as possible.
     */
#if defined(CYASSL_DTLS)
    HTTP_Session->ctx = CyaSSL_CTX_new(CyaDTLSv1_server_method());
#elif  !defined(NO_TLS)
    HTTP_Session->ctx = CyaSSL_CTX_new(CyaSSLv23_server_method());
#else
    HTTP_Session->ctx = CyaSSL_CTX_new(CyaSSLv3_server_method());
#endif

#if CFG_NU_OS_NET_SSL_LITE_HAVE_OCSP
    if (HTTP_Session->ctx != NU_NULL)
    {
        if(CyaSSL_CTX_OCSP_set_options(HTTP_Session->ctx, CYASSL_OCSP_ENABLE) != 1)
        {
            status = HTTP_SSL_ERROR;
        }
#if CFG_NU_OS_NET_SSL_LITE_OCSP_OVERRIDE
        if (status == NU_SUCCESS)
        {
            if(CyaSSL_CTX_OCSP_set_override_url(HTTP_Session->ctx, CFG_NU_OS_NET_SSL_LITE_OCSP_OVERRIDE_URL) != 1)
            {
                status = HTTP_SSL_ERROR;
            }
        }
#endif
    }
    else
    {
        /* CyaSSL context handle is invalid */
        status = HTTP_SSL_ERROR;
    }
#endif
#endif

    /* Wait for the networking stack to initialize. */
    if (NETBOOT_Wait_For_Network_Up(NU_SUSPEND) == NU_SUCCESS)
    {
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)

        if (status == NU_SUCCESS)
        {
            /* Create a listening socket for the secure connection. */
            status = HTTP_Lite_Svr_Create_Socket(&HTTP_Session->ssl_listener,
                                                 HTTP_SSL_PORT);
        }

        if (status == NU_SUCCESS)
#endif
        {
            /* Create a listening socket for the unsecure port. */
            if (HTTP_Lite_Svr_Create_Socket(&HTTP_Session->http_listener,
                                            HTTP_SVR_PORT) == NU_SUCCESS)
            {
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                /* Determine which socket is higher. */
                if (HTTP_Session->ssl_listener > HTTP_Session->http_listener)
                    max_socketd = HTTP_Session->ssl_listener;
                else
#endif
                    max_socketd = HTTP_Session->http_listener;

                for (;;)
                {
                    /* Initialize the bitmap */
                    NU_FD_Init(&readfs);

                    /* Set the read bit for each socket. */
                    NU_FD_Set(HTTP_Session->http_listener, &readfs);
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                    NU_FD_Set(HTTP_Session->ssl_listener, &readfs);
#endif

                    /* Wait for a connection. */
                    status = NU_Select(max_socketd + 1, &readfs, NU_NULL,
                                       NU_NULL, NU_SUSPEND);

                    if (status == NU_SUCCESS)
                    {
                        /* Get the semaphore. */
                        if (NU_Obtain_Semaphore(&HTTP_Lite_Resource,
                                                NU_SUSPEND) == NU_SUCCESS)
                        {
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                            /* Accept the connection over the proper port. */
                            if (NU_FD_Check(HTTP_Session->ssl_listener,
                                            &readfs) == NU_TRUE)
                            {
                                /* Accept the connection. */
                                HTTP_Session->socketd = NU_Accept(HTTP_Session->ssl_listener,
                                                                  &client_addr, 0);

                                if (HTTP_Session->socketd >= 0)
                                {
                                    /* Create a new SSL context */
                                    HTTP_Session->ssl = CyaSSL_new(HTTP_Session->ctx);

                                    if (HTTP_Session->ssl)
                                    {
                                        /* Associate client handle with this SSL context */
                                        CyaSSL_set_fd(HTTP_Session->ssl, HTTP_Session->socketd);

                                        /* Perform SSL accept to start cert and key exchange */
                                        if (CyaSSL_accept(HTTP_Session->ssl) == SSL_SUCCESS)
                                            status = NU_SUCCESS;
                                        else
                                            status = HTTP_SSL_ERROR;
                                    }

                                    else
                                        status = HTTP_SSL_ERROR;
                                }
                            }

                            /* If the connection is over the unsecure port. */
                            else
#endif
                            if (NU_FD_Check(HTTP_Session->http_listener, &readfs) == NU_TRUE)
                            {
                                HTTP_Session->socketd = NU_Accept(HTTP_Session->http_listener,
                                                                  &client_addr, 0);
                            }

                            else
                                status = HTTP_ERROR_DATA_READ;

                            /* If a connection was established. */
                            if (HTTP_Session->socketd >= 0)
                            {
                                if (status == NU_SUCCESS)
                                {
                                    /* Service the connection. */
                                    HTTP_Lite_Process_Client();
                                }

                                /* If an upgrade request was successfully processed
                                 * for the connection, do not close the newly created
                                 * socket, as it is now under ownership of another
                                 * module that handles the respective upgraded
                                 * operation.
                                 */
                                if (!(HTTP_Session->flags & HTTP_UPGRADE_REQ))
                                {
                                    /* Close the new socket. */
                                    NU_Close_Socket(HTTP_Session->socketd);

                                    /* The WebSocket module now owns the SSL
                                     * structure.
                                     */
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                                    if (HTTP_Session->ssl)
                                    {
                                        /* Free this SSL context structure. */
                                        CyaSSL_free(HTTP_Session->ssl);
                                    }
#endif
                                }

                                /* Invalidate the socket so a shutdown does not
                                 * try to close it.  Even if an upgrade request
                                 * was received, HTTP no longer has ownership of
                                 * this socket, so the socket should be invalidated.
                                 */
                                HTTP_Session->socketd = -1;

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                                if (HTTP_Session->ssl)
                                {
                                    HTTP_Session->ssl = NU_NULL;
                                }
#endif
                            }

                            NU_Release_Semaphore(&HTTP_Lite_Resource);
                        }

                        else
                        {
                            NLOG_Error_Log("HTTP_Lite_Receive_Task could not obtain semaphore.\n",
                                           NERR_FATAL, __FILE__, __LINE__);
                        }
                    }
                }
            }

            else
            {
                NLOG_Error_Log("HTTP_Lite_Receive_Task could not create listening socket.\n",
                               NERR_FATAL, __FILE__, __LINE__);
            }
        }

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
        else
        {
            NLOG_Error_Log("HTTP_Lite_Receive_Task could not initialize CyaSSL.\n",
                           NERR_FATAL, __FILE__, __LINE__);
        }
#endif
    }

} /* HTTP_Lite_Receive_Task */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Svr_Create_Socket
*
*   DESCRIPTION
*
*       This routine creates a listening socket for the caller.
*
*   INPUTS
*
*       *socketd                A pointer to memory in which to return
*                               the new socket.
*       port                    The port to bind the socket to.
*
*   OUTPUTS
*
*       NU_SUCCESS upon successful completion; otherwise, an OS
*       specific error.
*
************************************************************************/
STATUS HTTP_Lite_Svr_Create_Socket(INT *socketd, INT port)
{
    struct addr_struct  servaddr;
    STATUS              status;
    INT16               family;

#if (INCLUDE_IPV6 == NU_TRUE)
    family = NU_FAMILY_IP6;
#else
    family = NU_FAMILY_IP;
#endif

    /* Open a connection via the socket interface.  Note that an IPv6
     * socket can handle incoming connections over IPv4 and/or IPv6.
     */
    *socketd = NU_Socket(family, NU_TYPE_STREAM, 0);

    if (*socketd >= 0)
    {
        /* fill in a structure with the server address */
        servaddr.family = family;
        servaddr.port = port;

        /* Set the IP address to the wildcard. */
        memset(servaddr.id.is_ip_addrs, 0, MAX_ADDRESS_SIZE);

        /* Bind to the server's address */
        status = NU_Bind(*socketd, &servaddr, 0);

        if (status >= 0)
        {
            /* Be ready to accept connection requests */
            status = NU_Listen(*socketd, HTTP_SVR_BACKLOG);
        }
    }

    else
        status = *socketd;

    return (status);

} /* HTTP_Lite_Svr_Create_Socket */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Process_Client
*
*   DESCRIPTION
*
*       This routine processes an incoming client connection.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
************************************************************************/
STATIC VOID HTTP_Lite_Process_Client(VOID)
{
    INT16               http_method;
    STATUS              status;
    CHAR                *uri, *length_ptr;
    UINT32              length;

    /* RFC 2616 - section 8.1.2 - Persistent connections provide a
     * mechanism by which a client and a server can signal the close
     * of a TCP connection. This signaling takes place using the
     * Connection header field. Once a close has been signaled, the
     * client MUST NOT send any more requests on that connection.
     */
    do
    {
        /* Wait for incoming data. */
        status = HTTP_Lite_Select(HTTP_Session->socketd,
                                  HTTP_LITE_SVR_SSL_STRUCT);

        if (status == NU_SUCCESS)
        {
            /* Receive and parse the data lengths and pointers. */
            status = HTTP_Lite_Receive_Header();

            if (status == NU_SUCCESS)
            {
                /* Parse the URI from the header. */
                uri = HTTP_Lite_Parse_URI(HTTP_Session->buffer,
                                          HTTP_Session->in_hdr_size);

                /* Determine what method is being performed. */
                http_method = (INT16)(HTTP_Session->buffer[0] +
                        HTTP_Session->buffer[1] + HTTP_Session->buffer[2]);

                /* Ensure this method is supported. */
                if ( (http_method == HTTP_POST) || (http_method == HTTP_GET) ||
                     (http_method == HTTP_PUT) || (http_method == HTTP_DELETE) )
                {
                    /* Ensure that the HTTP version is 1.1. */
                    if (HTTP_Lite_Find_Token(HTTP_11_STRING, HTTP_Session->buffer,
                                             &HTTP_Session->buffer[HTTP_Session->in_hdr_size],
                                             0))
                    {
                        /* If an upgrade request was received. */
                        if (HTTP_Session->flags & HTTP_UPGRADE_REQ)
                        {
                            /* Check if the upgrade type is associated with a valid
                             * plug-in.
                             */
                            HTTP_Session->plug_ptr =
                                    HTTP_Lite_Get_Upgrade_Plugin(HTTP_Session->upgrade_req,
                                                                 http_method);

                            if (HTTP_Session->plug_ptr)
                            {
                                /* Invoke the plug-in. */
                                status = HTTP_Session->plug_ptr->uplugin(http_method, uri,
                                                                         HTTP_Session->buffer,
                                                                         HTTP_Session->in_hdr_size,
                                                                         HTTP_Session->socketd,
                                                                         HTTP_LITE_SVR_SSL_STRUCT);

                                if (status != NU_SUCCESS)
                                {
                                    /* Clear the upgrade request.  The receive loop will exit
                                     * since the plug-in returned an error.  Do not send an error
                                     * in this case.  The plug-in is responsible for sending the
                                     * proper error to the other side.
                                     */
                                    HTTP_Session->flags &= ~HTTP_UPGRADE_REQ;
                                }

                                /* This connection belongs to another module now. */
                                else
                                {
                                    break;
                                }
                            }

                            /* If a plug-in could not be found for the upgrade request,
                             * clear the upgrade request flag so this socket is closed
                             * properly and send an error to the other side.
                             */
                            else
                            {
                                /* Clear the upgrade request. */
                                HTTP_Session->flags &= ~HTTP_UPGRADE_REQ;

                                /* Return an error to the caller. */
                                status = HTTP_Lite_Send_Status_Message(HTTP_PROTO_NOT_FOUND,
                                                                       HTTP_NOT_FOUND);
                            }
                        }

                        else
                        {
                            /* Check if the URI is for a valid plug-in. */
                            HTTP_Session->plug_ptr = HTTP_Lite_Get_Plugin(uri, http_method);

                            /* If a plug-in was found, process the request. */
                            if (HTTP_Session->plug_ptr)
                            {
                                HTTP_Session->token_list.token_string = NU_NULL;
                                HTTP_Session->token_list.token_array[0] = -1;

                                /* Search for any tokens within the HTTP header */
                                if ( (http_method == HTTP_POST) &&
                                     (HTTP_Lite_Find_Token("multipart/form-data", HTTP_Session->buffer,
                                                           &HTTP_Session->buffer[HTTP_Session->in_hdr_size],
                                                           0) == NU_NULL) )
                                {
                                    /* If there is a length token present, the data is not chunked. */
                                    length_ptr = HTTP_Lite_Find_Token(HTTP_CONTENT_LENGTH,
                                                                      HTTP_Session->buffer,
                                                                      &HTTP_Session->buffer[HTTP_Session->in_hdr_size],
                                                                      HTTP_TOKEN_CASE_INSENS);

                                    if (length_ptr)
                                    {
                                        /* Increment Pointer to Content Length within the buffer */
                                        length_ptr += strlen(HTTP_CONTENT_LENGTH);

                                        /* Get the total length of the data */
                                        length = NU_ATOL(length_ptr);
                                    }

                                    else
                                    {
                                        length = 0;
                                    }

                                    /* Read all the data into the token string buffer. */
                                    HTTP_Session->token_list.token_string_len =
                                            HTTP_Lite_Read_Buffer(&HTTP_Session->buffer[HTTP_Session->in_hdr_size],
                                                                  HTTP_SVR_RCV_SIZE - HTTP_Session->in_hdr_size,
                                                                  HTTP_Session->socketd,
                                                                  &HTTP_Session->token_list.token_string,
                                                                  length, HTTP_LITE_SVR_SSL_STRUCT,
                                                                  0, NU_NULL, NU_NULL);

                                    /* If tokens were found. */
                                    if ( (HTTP_Session->token_list.token_string_len) &&
                                         (HTTP_Session->token_list.token_string) )
                                    {
                                        /* Build an array of tokens for use by the plug-in. */
                                        HTTP_Lite_Build_Token_Array(&HTTP_Session->token_list, length);
                                    }
                                }

                                /* Invoke the plug-in. */
                                status = (HTTP_Session->plug_ptr->plugin)(http_method, uri, HTTP_Session);
                            }

                            /* If no plug-in could be found, there is nothing to be done
                             * for this URI.  Return an error to the caller.
                             */
                            else
                            {
                                status = HTTP_Lite_Send_Status_Message(HTTP_PROTO_NOT_FOUND,
                                                                       HTTP_NOT_FOUND);
                            }
                        }
                    }

                    /* This server supports HTTP 1.1 requests only. */
                    else
                    {
                        /* Send a 505 HTTP Version Not Supported error. */
                        status = HTTP_Lite_Send_Status_Message(HTTP_PROTO_NOT_SUPPORTED,
                                                               HTTP_NOT_SUPPORTED);
                    }
                }

                else
                {
                    /* An origin server SHOULD return the status code 405
                     * (Method Not Allowed) if the method is known by the
                     * origin server but not allowed for the requested
                     * resource, and 501 (Not Implemented) if the method is
                     * unrecognized or not implemented by the origin server.
                     */
                    status = HTTP_Lite_Send_Status_Message(HTTP_PROTO_NOT_IMPLEMENTED,
                                                           HTTP_NOT_IMPLEMENTED);
                }

                /* If the client wants the server to close the connection
                 * after processing this packet, set an error status so
                 * the socket gets closed.
                 */
                if (HTTP_Session->flags & HTTP_HDR_CLOSE)
                    status = HTTP_FORCE_CLOSE;
            }
        }
    } while (status == NU_SUCCESS);

} /* HTTP_Lite_Process_Client */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Build_Token_Array
*
*   DESCRIPTION
*
*       This function parses and converts the client's plug-in
*       arguments into individual strings and adds pointers to
*       the buffer into the token array.
*
*   INPUTS
*
*       *token_info             Pointer to the array of tokens.
*       length                  The length of the buffer in which the
*                               unparsed tokens are contained.  This
*                               value will be zero if parsing tokens
*                               from the data portion of a packet, and
*                               the data is chunked, or if parsing tokens
*                               from the query part of the header since
*                               there is room for a null-terminator.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID HTTP_Lite_Build_Token_Array(HTTP_TOKEN_INFO *token_info,
                                        UINT32 length)
{
    UINT32      buf_count = 0;
    UINT32      arr_count = 0;
    INT16       *token_array;

    /* Assign a pointer to the token array. */
    token_array = token_info->token_array;

    /* To prevent including a line feed or carriage return in
     * the array, we must state that such characters end the
     * buffer.
     */
    while ( (buf_count < token_info->token_string_len) &&
            (token_info->token_string[buf_count] > 31) &&
            (arr_count < HTTP_TOKEN_HEAP) )
    {
        /* Set the index into the buffer to the beginning of this
         * token string.
         */
        token_array[arr_count++] = buf_count;

        /* Parse this token. */
        while ( (buf_count < token_info->token_string_len) &&
                (token_info->token_string[buf_count] > 31) &&
                (token_info->token_string[buf_count] != '=') )
        {
            /* A space is represented by a '+'.  Replace it with a space
             * so the plug-in can read it properly.
             */
            if (token_info->token_string[buf_count] == '+')
                token_info->token_string[buf_count] = ' ';

            buf_count++;
        }

        /* Null-terminate this token string if there is room in the buffer. */
        if ( (buf_count < token_info->token_string_len) &&
             (token_info->token_string[buf_count] > 31) )
        {
            /* NULL-terminate the string, and then increment buf_count. */
            token_info->token_string[buf_count++] = '\0';
        }
        else
            break;

        /* Set a null terminator between the name and value
         * to allow for easy retrieval by the plug-in later.
         */
        while ( (buf_count < token_info->token_string_len) &&
                (token_info->token_string[buf_count] > 31) &&
                (token_info->token_string[buf_count] != '&') )
        {
            /* A space is represented by a '+' */
            if (token_info->token_string[buf_count] == '+')
                token_info->token_string[buf_count] = ' ';

            buf_count++;
        }

        /* Be sure to not write outside the bounds of the buffer. */
        if (buf_count < token_info->token_string_len)
        {
            /* Verify that this is not a terminating character. */
            if (token_info->token_string[buf_count] > 31)
            {
                /* NULL-terminate the string and the increment buf_count. */
                token_info->token_string[buf_count++] = '\0';
            }

            else
                token_info->token_string[buf_count] = '\0';
        }
    }

    /* Check if there is room in the buffer for the final NULL-terminator. */
    if ( (!length) || (token_info->token_string_len < HTTP_SVR_RCV_SIZE) )
        token_info->token_string[buf_count] = '\0';

    /* Indicate the end of the array. */
    token_array[arr_count] = -1;

} /* HTTP_Lite_Build_Token_Array */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Get_Plugin
*
*   DESCRIPTION
*
*       Gets a pointer to a plug-in with the matching URI.
*
*   INPUTS
*
*       *uri                    Pointer to the URI to find.
*       method                  The method matching the plug-in.
*
*   OUTPUTS
*
*       A pointer to the plug-in structure or NU_NULL if no match was
*       found.
*
************************************************************************/
HTTP_PLUGIN_STRUCT *HTTP_Lite_Get_Plugin(CHAR *uri, INT method)
{
    HTTP_PLUGIN_STRUCT  *plug_ptr = NU_NULL, *temp_plug_ptr;

    /* If the method is specified, the caller is willing to accept the
     * default plug-in.  Otherwise, the caller wants only a specific
     * plug-in.
     */
    if (method)
    {
        /* Set plug-ptr to the default plug-in for the method type.
         * If no matching plug-in is found for the URI, the default will be
         * returned.
         */
        switch (method)
        {
        case HTTP_POST:

            if ( (HTTP_Session->plug_ptr_post) &&
                 (HTTP_Session->plug_ptr_post->plugin) )
            {
                plug_ptr = HTTP_Session->plug_ptr_post;
            }

            break;

        case HTTP_PUT:

            if ( (HTTP_Session->plug_ptr_put) &&
                 (HTTP_Session->plug_ptr_put->plugin) )
            {
                plug_ptr = HTTP_Session->plug_ptr_put;
            }

            break;

        case HTTP_DELETE:

            if ( (HTTP_Session->plug_ptr_delete) &&
                 (HTTP_Session->plug_ptr_delete->plugin) )
            {
                plug_ptr = HTTP_Session->plug_ptr_delete;
            }

            break;

        case HTTP_GET:

            if ( (HTTP_Session->plug_ptr_get) &&
                 (HTTP_Session->plug_ptr_get->plugin) )
            {
                plug_ptr = HTTP_Session->plug_ptr_get;
            }

            break;

        default:
            break;
        }
    }

    /* Find a specific plug-in matching the URI.  If a specific plug-in is
     * found, but it does not support the incoming method, no plug-in will
     * be returned, not even the default - an error should be returned to the
     * client.
     */
    for (temp_plug_ptr = HTTP_Lite_Plugins.flink;
         temp_plug_ptr;
         temp_plug_ptr = temp_plug_ptr->flink)
    {
        /* If the URI has been found, determine whether the plug-in can be
         * used for this method.
         */
        if ( (strcmp(uri, temp_plug_ptr->http_name) == 0) &&
             (temp_plug_ptr->plugin) )
        {
            /* Clear out any default.  If the URI matches, but the method
             * does not, do not execute anything for this method.
             */
            plug_ptr = NU_NULL;

            /* Ensure the plug-in supports the requested method. */
            switch (method)
            {
            case HTTP_POST:

                if (temp_plug_ptr->methods & HTTP_LITE_POST)
                    plug_ptr = temp_plug_ptr;

                break;

            case HTTP_PUT:

                if (temp_plug_ptr->methods & HTTP_LITE_PUT)
                    plug_ptr = temp_plug_ptr;

                break;

            case HTTP_DELETE:

                if (temp_plug_ptr->methods & HTTP_LITE_DELETE)
                    plug_ptr = temp_plug_ptr;

                break;

            case HTTP_GET:

                if (temp_plug_ptr->methods & HTTP_LITE_GET)
                    plug_ptr = temp_plug_ptr;

                break;

            default:

                /* The application is trying to find a plug-in that does
                 * not yet have any methods associated with it.  The stack
                 * will never call this routine with an invalid method since
                 * the method was validated when the packet was received.
                 */
                plug_ptr = temp_plug_ptr;
                break;
            }

            /* Exit the loop with or without the plug-in pointer. */
            break;
        }
    }

    return (plug_ptr);

} /* HTTP_Lite_Get_Plugin */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Get_Upgrade_Plugin
*
*   DESCRIPTION
*
*       Gets a pointer to a plug-in with the matching upgrade.
*
*   INPUTS
*
*       *upgrade                Pointer to the upgrade to find.
*       method                  The method matching the plug-in.
*
*   OUTPUTS
*
*       A pointer to the plug-in structure or NU_NULL if no match was
*       found.
*
************************************************************************/
HTTP_PLUGIN_STRUCT *HTTP_Lite_Get_Upgrade_Plugin(CHAR *upgrade, INT method)
{
    HTTP_PLUGIN_STRUCT  *plug_ptr = NU_NULL, *temp_plug_ptr;

    /* Set plug-ptr to the default plug-in for the method type.
     * If no matching plug-in is found for the upgrade, the default will be
     * returned.
     */
    switch (method)
    {
    case HTTP_POST:

        if ( (HTTP_Session->uplug_ptr_post) &&
             (HTTP_Session->uplug_ptr_post->uplugin) )
        {
            plug_ptr = HTTP_Session->uplug_ptr_post;
        }

        break;

    case HTTP_PUT:

        if ( (HTTP_Session->uplug_ptr_put) &&
             (HTTP_Session->uplug_ptr_put->uplugin) )
        {
            plug_ptr = HTTP_Session->uplug_ptr_put;
        }

        break;

    case HTTP_DELETE:

        if ( (HTTP_Session->uplug_ptr_delete) &&
             (HTTP_Session->uplug_ptr_delete->uplugin) )
        {
            plug_ptr = HTTP_Session->uplug_ptr_delete;
        }

        break;

    case HTTP_GET:

        if ( (HTTP_Session->uplug_ptr_get) &&
             (HTTP_Session->uplug_ptr_get->uplugin) )
        {
            plug_ptr = HTTP_Session->uplug_ptr_get;
        }

        break;

    default:
        break;
    }

    /* Find a specific plug-in matching the upgrade.  If a specific plug-in is
     * found, but it does not support the incoming method, no plug-in will
     * be returned, not even the default - an error should be returned to the
     * client.
     */
    for (temp_plug_ptr = HTTP_Lite_Plugins.flink;
         temp_plug_ptr;
         temp_plug_ptr = temp_plug_ptr->flink)
    {
        /* If the upgrade has been found, determine whether the plug-in can be
         * used for this method.
         */
        if ( (NCL_Stricmp(upgrade, temp_plug_ptr->http_name) == 0) &&
             (temp_plug_ptr->uplugin) )
        {
            /* Clear out any default.  If the upgrade matches, but the method
             * does not, do not execute anything for this method.
             */
            plug_ptr = NU_NULL;

            /* Ensure the plug-in supports the requested method. */
            switch (method)
            {
            case HTTP_POST:

                if (temp_plug_ptr->methods & HTTP_LITE_POST)
                    plug_ptr = temp_plug_ptr;

                break;

            case HTTP_PUT:

                if (temp_plug_ptr->methods & HTTP_LITE_PUT)
                    plug_ptr = temp_plug_ptr;

                break;

            case HTTP_DELETE:

                if (temp_plug_ptr->methods & HTTP_LITE_DELETE)
                    plug_ptr = temp_plug_ptr;

                break;

            case HTTP_GET:

                if (temp_plug_ptr->methods & HTTP_LITE_GET)
                    plug_ptr = temp_plug_ptr;

                break;

            default:

                /* The application is trying to find a plug-in that does
                 * not yet have any methods associated with it.  The stack
                 * will never call this routine with an invalid method since
                 * the method was validated when the packet was received.
                 */
                plug_ptr = temp_plug_ptr;
                break;
            }

            /* Exit the loop with or without the plug-in pointer. */
            break;
        }
    }

    return (plug_ptr);

} /* HTTP_Lite_Get_Upgrade_Plugin */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Parse_URI
*
*   DESCRIPTION
*
*       Returns the URI contained in the message body.
*
*   INPUTS
*
*       *data_start                 The beginning of the data portion.
*       data_len                    Length of data portion.
*
*   OUTPUTS
*
*       A pointer to the URI.
*
************************************************************************/
STATIC CHAR *HTTP_Lite_Parse_URI(CHAR *data_start, INT data_len)
{
    CHAR            *uri, *token;

    /* Find the forward slash that starts the URI. */
    data_start = strchr(data_start, '/');

    /* Move past the forward slash. */
    data_start ++;

    /* If the request only specifies the root node "/" of the file system,
     * set the URI to the default page name.
     */
    if (*data_start == ' ')
        uri = HTTP_DEFAULT_URI;

    else
    {
        /* RFC 3986 - section 3.3 - The path is terminated by the first
         * question mark ("?") or number sign ("#") character, or by the
         * end of the URI.
         */

        /* Set the pointer to the beginning of the URI. */
        uri = data_start;

        /* Find the space that terminates the URI in the packet. */
        data_start = strchr(data_start, ' ');

        if (data_start != NU_NULL)
        {
            /* Null-terminate the URI. */
            data_start[0] = '\0';
        }

        /* Look for a '?' token. */
        token = strchr(uri, '?');

        if (token)
        {
            /* Replace the '?' with a null-terminator. */
            token[0] = '\0';

            /* Save a pointer to the head of the query token. */
            HTTP_Session->token_query_list.token_string = (++token);

            /* Check for the '#' token that might be used to terminate
             * the query.
             */
            token = strchr(HTTP_Session->token_query_list.token_string, '#');

            if (token)
            {
                /* Replace the '#' with a null-terminator. */
                token[0] = '\0';
            }

            else
            {
                /* Find the space that terminates the query in the packet. */
                token = strchr(HTTP_Session->token_query_list.token_string, ' ');

                if (token)
                {
                    /* Replace the ' ' with a null-terminator. */
                    token[0] = '\0';
                }
            }

            /* Set the length of the query list tokens. */
            HTTP_Session->token_query_list.token_string_len =
                strlen(HTTP_Session->token_query_list.token_string);

            HTTP_Lite_Build_Token_Array(&HTTP_Session->token_query_list, 0);
        }

        /* A '#' token might be present before the '?', which indicates the
         * end of the URI.
         */
        token = strchr(uri, '#');

        if (token)
        {
            /* Replace the '#' with a null-terminator. */
            token[0] = '\0';
        }
    }

    return (uri);

} /* HTTP_Lite_Parse_URI */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Header_Name_Insert
*
*   DESCRIPTION
*
*       Function to insert the name and value into the output
*       header.
*
*   INPUTS
*
*       *name                   The name token.
*       *value                  The value token.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID HTTP_Lite_Header_Name_Insert(CHAR *name, CHAR *value)
{
    /* Set the name */
    strcat(HTTP_Session->buffer, name);

    /* Set the value */
    if (value)
        strcat(HTTP_Session->buffer, value);

    strcat(HTTP_Session->buffer, "\r\n");

} /* HTTP_Lite_Header_Name_Insert */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Response_Header
*
*   DESCRIPTION
*
*       This function builds the header for the HTTP response
*       depending on the code supplied by the calling function.
*
*   INPUTS
*
*       code                    Code that is used to describe the
*                               prototype status.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID HTTP_Lite_Response_Header(UINT16 code)
{
    CHAR temp[10];

    /* Put the version in the buffer. */
    strcpy(HTTP_Session->buffer, "HTTP/1.1 ");

    /* Add the code. */
    strcat(HTTP_Session->buffer, (CHAR *)NU_ITOA(code, temp, 10));

} /* HTTP_Lite_Response_Header */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Receive_Header
*
*   DESCRIPTION
*
*       Receives the HTTP header.
*
*   INPUTS
*
*       *status             A pointer to the status to fill in.
*
*   OUTPUTS
*
*       CHAR                *data_start Pointer to end of header
*
************************************************************************/
STATIC STATUS HTTP_Lite_Receive_Header(VOID)
{
    CHAR        *header_field;
    INT32       bytes_recv;
    UINT32      arr_index;
    STATUS      status = NU_SUCCESS;

    /* Parse through the headers returned by the server */
    for (arr_index = 0; arr_index < HTTP_SVR_RCV_SIZE;)
    {
        /* Read in a byte. */
        bytes_recv = HTTP_Lite_Read_Line(&HTTP_Session->buffer[arr_index],
                                         HTTP_Session->socketd,
                                         HTTP_SVR_RCV_SIZE - arr_index,
                                         NU_FALSE, HTTP_LITE_SVR_SSL_STRUCT);

        if (bytes_recv <= 0)
        {
            status = HTTP_INVALID_HEADER_READ;
            break;
        }

        /* Increment the number of bytes received. */
        arr_index += bytes_recv;

        /* Is this the end of the header. */
        if ( (arr_index >= strlen(HTTP_CRLFCRLF)) &&
             (memcmp(&HTTP_Session->buffer[arr_index - strlen(HTTP_CRLFCRLF)],
                     HTTP_CRLFCRLF, strlen(HTTP_CRLFCRLF)) == 0) )
            break;
    }

    /* If an error did not occur. */
    if (status == NU_SUCCESS)
    {
        /* If the full header could not be received in one packet, return
         * an error.
         */
        if (arr_index >= HTTP_SVR_RCV_SIZE)
        {
            /* The packet could not be received. */
            HTTP_Lite_Send_Status_Message(HTTP_PROTO_SERVER_ERROR,
                                          HTTP_SERVER_ERROR);

            status = NU_NO_MEMORY;
        }

        else
        {
            /* Set the size of the incoming header. */
            HTTP_Session->in_hdr_size = arr_index;

            HTTP_Session->flags = 0;

            /* Check for a "Connection: " general-header field. */
            header_field = HTTP_Lite_Find_Token(HTTP_CONNECTION,
                                                (CHAR*)HTTP_Session->buffer,
                                                &HTTP_Session->buffer[HTTP_Session->in_hdr_size],
                                                HTTP_TOKEN_CASE_INSENS);

            if (header_field)
            {
                /* Move past the "Connection: " */
                header_field += (sizeof(HTTP_CONNECTION) - 1);

                /* Check for "close" within the bounds of the Connection: header. */
                if (HTTP_Lite_Find_Token("close", header_field,
                                         strchr(header_field, '\r'),
                                         HTTP_TOKEN_CASE_INSENS))
                {
                    /* Close the connection after completing the
                     * respective method.
                     */
                    HTTP_Session->flags |= HTTP_HDR_CLOSE;
                }

                /* Check for "upgrade" within the bounds of the Connection: header. */
                if (HTTP_Lite_Find_Token("upgrade", header_field,
                                         strchr(header_field, '\r'),
                                         HTTP_TOKEN_CASE_INSENS))
                {
                    /* Check for an "Upgrade: " general-header field. */
                    header_field = HTTP_Lite_Find_Token(HTTP_UPGRADE,
                                                        (CHAR*)HTTP_Session->buffer,
                                                        &HTTP_Session->buffer[HTTP_Session->in_hdr_size],
                                                        HTTP_TOKEN_CASE_INSENS);

                    if (header_field)
                    {
                        /* Set the flag indicating that an upgrade request has been found. */
                        HTTP_Session->flags |= HTTP_UPGRADE_REQ;

                        /* Move past the "Upgrade: " */
                        header_field += (sizeof(HTTP_UPGRADE) - 1);

                        /* Set the pointer to the beginning of the upgrade. */
                        HTTP_Session->upgrade_req = header_field;

                        /* Find the character that terminates the upgrade in the packet. */
                        header_field = strchr(HTTP_Session->upgrade_req, '\r');

                        if (header_field != NU_NULL)
                        {
                            /* Null-terminate the upgrade string. */
                            header_field[0] = '\0';
                        }
                    }
                }
            }
        }
    }

    return (status);

} /* HTTP_Lite_Receive_Header */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Find_Token
*
*   DESCRIPTION
*
*       Return the position in the second string where an occurrence
*       of the first string starts.
*
*   INPUTS
*
*       *token                  Token string to be found.
*       *start                  Pointer to start of data.
*       *last                   Pointer to end of data.
*       mode                    Case sensitivity flag.
*
*   OUTPUTS
*
*       file                    Found the token
*       NU_NULL                 Could not find the token
*
*************************************************************************/
CHAR *HTTP_Lite_Find_Token(CHAR *token, CHAR *start, CHAR *last, UINT8 mode)
{
    UNSIGNED        len = strlen(token);
    UNSIGNED        i;

    /* If the search is not case-sensitive. */
    if (mode == HTTP_TOKEN_CASE_INSENS)
    {
        /* Do not index beyond the end of the buffer. */
        for (; start < last; start++)
        {
            /* Convert both strings to uppercase for the comparison. */
            for (i = 0; (i < len) && (NU_TOUPPER(token[i]) == NU_TOUPPER(start[i])); i++)
                ;

            /* If the string matches, return the index into the buffer. */
            if (i == len)
                break;
        }
    }

    /* If the search is case-sensitive. */
    else
    {
        /* Do not index beyond the end of the buffer. */
        for (; start < last; start++)
        {
            /* If the string matches, return the index into the buffer. */
            if ( (*token == *start) && (strncmp(token, start, len) == 0) )
                break;
        }
    }

    /* If the string was found, return the index where it was found. */
    if (start < last)
        return (start);

    /* Otherwise, return NU_NULL. */
    else
        return (NU_NULL);

} /* HTTP_Lite_Find_Token */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Send_Status_Message
*
*   DESCRIPTION
*
*       This routine will send an HTTP status message to the client.
*
*   INPUTS
*
*       code                      Status code
*       *string                   Message to be sent.
*
*   OUTPUTS
*
*       The number of bytes sent or an error code if data could not be
*       sent.
*
*************************************************************************/
STATUS HTTP_Lite_Send_Status_Message(UINT16 code, CHAR *string)
{
    INT32   bytes_sent;
    STATUS  status;

    /* Create the HTTP Entity header */
    HTTP_Lite_Response_Header(code);
    strcat(HTTP_Session->buffer, "\r\n");
    HTTP_Lite_Header_Name_Insert(string, NU_NULL);
    strcat(HTTP_Session->buffer, "\r\n");

    /* Send the message. */
    bytes_sent = HTTP_Lite_Write(HTTP_Session->socketd, HTTP_Session->buffer,
                                 strlen(HTTP_Session->buffer),
                                 HTTP_LITE_SVR_SSL_STRUCT);

    /* If an error occurred, return the error. */
    if (bytes_sent > 0)
        status = NU_SUCCESS;
    else
        status = bytes_sent;

    return (status);

} /* HTTP_Lite_Send_Status_Message */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Svr_Send
*
*   DESCRIPTION
*
*       This routine transmits data to the other side of a connection from
*       within the context of the server receive thread.  This routine is
*       intended to be used by a plug-in to transmit data to the other
*       side of the connection.
*
*   INPUTS
*
*       *http_ptr               A pointer to the server structure.
*       *buff                   A pointer to the buffer of data to transmit.
*       nbytes                  The number of bytes to transmit.
*
*   OUTPUTS
*
*       The number of bytes transmitted, or an operating-system specific
*       error if the transmission failed.
*
************************************************************************/
UINT32 HTTP_Lite_Svr_Send(HTTP_SVR_SESSION_STRUCT *http_ptr, CHAR *buff,
                          UINT32 nbytes)
{
    UINT32  status;

    /* Send the data. */
    status = HTTP_Lite_Write(http_ptr->socketd, buff, strlen(buff),
                             HTTP_LITE_SVR_SSL_STRUCT);

    return (status);

} /* HTTP_Lite_Svr_Send */

/************************************************************************
*
*   FUNCTION
*
*       NU_HTTP_Lite_Configure_SSL
*
*   DESCRIPTION
*
*       This routine configures the SSL certificate and key that will be
*       used by the HTTP Lite Server.  This routine must be invoked before
*       the HTTP Lite Server can service any secure connections.
*
*   INPUTS
*
*       *cert                   A pointer to the certificate to use for
*                               new incoming connections.
*       cert_type               The type of data in the *cert pointer:
*                                   HTTP_CERT_FILE - *cert points to either
*                                           a DER or PEM file.
*                                   HTTP_CERT_PEM_PTR - *cert points to a
*                                           buffer of data representing a
*                                           certificate in PEM format.
*                                   HTTP_CERT_DER_PTR - *cert points to a
*                                           buffer of data representing a
*                                           certificate in DER format.
*       cert_size               Used only if cert_type is not HTTP_CERT_FILE.
*                               The size of the buffer of data that holds
*                               the certificate.
*       *key                    A pointer to the key to use for new incoming
*                               connections.
*       key_type                The type of data in the *key pointer:
*                                   HTTP_KEY_FILE - *key points to either
*                                           a DER or PEM file.
*                                   HTTP_KEY_PEM_PTR - *key points to a
*                                           buffer of data representing a
*                                           key in PEM format.
*                                   HTTP_KEY_DER_PTR - *key points to a
*                                           buffer of data representing a
*                                           key in DER format.
*       key_size                Used only if key_type is not HTTP_KEY_FILE.
*                               The size of the buffer of data that holds
*                               the key.
*       *ca_list                A pointer to the Certificate Authority(ies)
*                               to use to validate the client.
*       ca_type                 The type of data in the *ca_list pointer:
*                                   HTTP_CA_FILE - *ca_list points to either
*                                           a DER or PEM file.
*                                   HTTP_CA_PEM_PTR - *ca_list points to a
*                                           buffer of data representing a
*                                           CA in PEM format.
*                                   HTTP_CA_DER_PTR - *ca_list points to a
*                                           buffer of data representing a
*                                           CA in DER format.
*       ca_size                 Used only if ca_type is not HTTP_CA_FILE.
*                               The size of the buffer of data that holds
*                               the CA.
*       flags                   Flags used to indicate other desired
*                               behavior:
*                                   NU_HTTP_LITE_SVR_VERIFY_CLIENT -
*                                       Send a certificate request to the client
*                                       and check the certificate.
*                                   NU_HTTP_LITE_SVR_VERIFY_FAIL -
*                                       Fail the connection if the client
*                                       fails to send a certificate when
*                                       requested to do so.
*
*   OUTPUTS
*
*       NU_SUCCESS              SSL was successfully configured.
*       NU_INVALID_PARM         One of the input parameters is invalid.
*       HTTP_SSL_ERROR          An error occurred when trying to configure
*                               the certificate or key.
*
*       Otherwise, an operating-system specific error is returned.
*
************************************************************************/
STATUS NU_HTTP_Lite_Configure_SSL(CHAR *cert, UINT8 cert_type, INT cert_size,
                                  CHAR *key, UINT8 key_type, INT key_size,
                                  CHAR *ca_list, UINT8 ca_type, INT ca_size,
                                  INT flags)
{
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
    INT     type;
    INT     ssl_flags;
#endif
    STATUS  status;

    /* Validate the input parameters. */
    if ( (!cert) || (!key) )
    {
        return (NU_INVALID_PARM);
    }

    status = NU_Obtain_Semaphore(&HTTP_Lite_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)

        status = SSL_SUCCESS;

#if (CFG_NU_OS_NET_SSL_LITE_NO_FILESYSTEM == NU_FALSE)
        /* If the certificate is in a file, load it from the file. */
        if (cert_type == HTTP_CERT_FILE)
        {
            /* If the certificate file has a "DER" extension, set the type
             * accordingly.
             */
            if ( (NU_STRICMP(&cert[strlen(cert) - 3], "der") == 0) ||
                 (NU_STRICMP(&cert[strlen(cert) - 3], "cer") == 0) )
            {
                type = SSL_FILETYPE_ASN1;
            }
            else
            {
                type = SSL_FILETYPE_PEM;
            }

            /* Load the certificate. */
            status = CyaSSL_CTX_use_certificate_file(HTTP_Session->ctx, cert, type);
        }

        else
#endif
        if (cert_size > 0)
        {
            /* Determine whether this is a PEM or DER certificate. */
            if (cert_type == HTTP_CERT_PEM_PTR)
            {
                type = SSL_FILETYPE_PEM;
            }

            else if (cert_type == HTTP_CERT_DER_PTR)
            {
                type = SSL_FILETYPE_ASN1;
            }

            else
            {
                status = NU_INVALID_PARM;
            }

            if (status == SSL_SUCCESS)
            {
                /* Load server certificate. */
                status = CyaSSL_CTX_use_certificate_buffer(HTTP_Session->ctx,
                                                           (const unsigned char *)cert,
                                                           cert_size, type);
            }
        }

        else
        {
            status = NU_INVALID_PARM;
        }

        if (status == SSL_SUCCESS)
        {
#if (CFG_NU_OS_NET_SSL_LITE_NO_FILESYSTEM == NU_FALSE)
            /* If the key is in a file, load it from the file. */
            if (cert_type == HTTP_KEY_FILE)
            {
                /* If the key file has a "DER" extension, set the type accordingly. */
                if ( (NU_STRICMP(&key[strlen(key) - 3], "der") == 0) ||
                     (NU_STRICMP(&key[strlen(key) - 3], "cer") == 0) )
                {
                    type = SSL_FILETYPE_ASN1;
                }
                else
                {
                    type = SSL_FILETYPE_PEM;
                }

                /* Load the key. */
                status = CyaSSL_CTX_use_PrivateKey_file(HTTP_Session->ctx, key, type);
            }

            else
#endif
            if (key_size > 0)
            {
                /* Determine whether this is a PEM or DER key. */
                if (key_type == HTTP_KEY_PEM_PTR)
                {
                    type = SSL_FILETYPE_PEM;
                }

                else if (key_type == HTTP_KEY_DER_PTR)
                {
                    type = SSL_FILETYPE_ASN1;
                }

                else
                {
                    status = NU_INVALID_PARM;
                }

                if (status == SSL_SUCCESS)
                {
                    /* Load the server key. */
                    status = CyaSSL_CTX_use_PrivateKey_buffer(HTTP_Session->ctx,
                                                              (const unsigned char *)key,
                                                              key_size, type);
                }
            }

            else
            {
                status = NU_INVALID_PARM;
            }
        }

        /* If the user has passed in a CA list. */
        if ( (status == SSL_SUCCESS) && (ca_list) )
        {
#if (CFG_NU_OS_NET_SSL_LITE_NO_FILESYSTEM == NU_FALSE)
            /* If the CA is in a file, load it from the file. */
            if (ca_type == HTTP_CA_FILE)
            {
#if (CFG_NU_OS_NET_SSL_LITE_CYASSL_DER_LOAD == NU_TRUE)
                /* If the CA file has a "DER" extension, set the type accordingly. */
                if ( (NU_STRICMP(&ca_list[strlen(ca_list) - 3], "der") == 0) ||
                     (NU_STRICMP(&ca_list[strlen(ca_list) - 3], "cer") == 0) )
                {
                    type = SSL_FILETYPE_ASN1;
                }
                else
                {
                    type = SSL_FILETYPE_PEM;
                }

                status = CyaSSL_CTX_der_load_verify_locations(HTTP_Session->ctx,
                                                              ca_list, type);
#else
                status = CyaSSL_CTX_load_verify_locations(HTTP_Session->ctx, ca_list, 0);
#endif
            }

            else
#endif
            if (ca_size > 0)
            {
                /* Determine whether this is a PEM or DER CA. */
                if (ca_type == HTTP_CA_PEM_PTR)
                {
                    type = SSL_FILETYPE_PEM;
                }

                else if (ca_type == HTTP_CA_DER_PTR)
                {
                    type = SSL_FILETYPE_ASN1;
                }

                else
                {
                    status = NU_INVALID_PARM;
                }

                if (status == SSL_SUCCESS)
                {
                    /* Load CA */
                    status = CyaSSL_CTX_load_verify_buffer(HTTP_Session->ctx,
                                                           (const unsigned char*)ca_list,
                                                           ca_size, type);
                }
            }

            else
            {
                status = NU_INVALID_PARM;
            }
        }

        if (status == SSL_SUCCESS)
        {
            ssl_flags = 0;

            /* Determine whether the caller wants to verify the client's
             * certificate.
             */
            if (!(flags & NU_HTTP_LITE_SVR_VERIFY_CLIENT))
            {
                ssl_flags |= SSL_VERIFY_NONE;
            }

            else
            {
                /* The server will send a certificate request to the client
                 * and verify the client certificate received.
                 */
                if (flags & NU_HTTP_LITE_SVR_VERIFY_CLIENT)
                {
                    ssl_flags |= SSL_VERIFY_PEER;
                }

                /* The verification will fail on the server side if the client
                 * fails to send a certificate when requested to do so.
                 */
                if (flags & NU_HTTP_LITE_SVR_VERIFY_FAIL)
                {
                    ssl_flags |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
                }
            }

            /* Set the verification mode. */
            SSL_CTX_set_verify(HTTP_Session->ctx, ssl_flags, 0);
        }

        if (status == SSL_SUCCESS)
        {
            status = NU_SUCCESS;
        }

        else if (status != NU_INVALID_PARM)
        {
            status = HTTP_SSL_ERROR;
        }

#else

        status = HTTP_SSL_ERROR;

#endif

        NU_Release_Semaphore(&HTTP_Lite_Resource);
    }

    return (status);

} /* NU_HTTP_Lite_Configure_SSL */
