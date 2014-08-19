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
*       wsox_clnt.c
*
*   COMPONENT
*
*       Nucleus WebSocket
*
*   DESCRIPTION
*
*       This file holds the Nucleus WebSocket routines specific to the
*       client component.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       WSOX_Create_Client_Handle
*       WSOX_Process_Connection_Response
*       WSOX_Client_Wait_For_Data
*       WSOX_Queue_Client_Request
*       WSOX_Resolve_Host
*       WSOX_Build_Client_Request
*
*   DEPENDENCIES
*
*       nu_networking.h
*       wsox_int.h
*
************************************************************************/

#include "networking/nu_networking.h"
#include "os/networking/websocket/wsox_int.h"

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
#include "os/networking/ssl/lite/cyassl/ctaocrypt/sha.h"
#include "os/networking/ssl/lite/cyassl/ctaocrypt/coding.h"
#endif

extern NU_SEMAPHORE         WSOX_Resource;
extern WSOX_CONTEXT_LIST    WSOX_Connection_List;
extern WSOX_PENDING_CLIENTS WSOX_Pending_List;

STATIC STATUS WSOX_Build_Client_Request(NU_WSOX_CONTEXT_STRUCT *wsox_ptr,
                                        CHAR **buf_ptr, INT *len_ptr,
                                        CHAR **key);
STATIC STATUS WSOX_Resolve_Host(CHAR *host, UINT8 *ip_addr, INT16 *addr_fam);
STATIC WSOX_CLIENT_STRUCT *WSOX_Queue_Client_Request(struct addr_struct *server,
                                                     STATUS *status_ptr);
STATIC STATUS WSOX_Client_Wait_For_Data(INT socketd);
STATIC STATUS WSOX_Process_Connection_Response(NU_WSOX_CONTEXT_STRUCT *wsox_ptr,
                                               INT socketd, CHAR *key, VOID *ssl_ptr);

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Create_Client_Handle
*
*   DESCRIPTION
*
*       Creates a new internal WebSocket client context structure and
*       establishes a connection with the foreign server.
*
*   INPUTS
*
*       *context_ptr            The structure on which to base the new
*                               internal handle structure.
*       *user_handle            A pointer that will be filled in with
*                               the handle for this new context
*                               structure if the connection could be
*                               successfully established.
*
*   OUTPUTS
*
*       NU_SUCCESS              The structure was created and a connection
*                               established with the foreign node.
*       WSOX_INVALID_HOST       The host field is invalid.
*       WSOX_TIMEOUT            The server did not respond to the HTTP
*                               GET request.
*       WSOX_CNXN_ERROR         The server rejected the connection.
*       WSOX_PKT_ERROR          The server's response was invalid.
*       WSOX_PROTOCOL_ERROR     The server does not support the requested
*                               protocol(s).
*       WSOX_NO_HANDLES         There is not a free handle index in the
*                               system for the new structure.
*       WSOX_SSL_ERROR          The SSL connection could not be established.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS WSOX_Create_Client_Handle(NU_WSOX_CONTEXT_STRUCT *context_ptr,
                                 UINT32 *user_handle)
{
    STATUS                  status, temp_status;
    WSOX_CONTEXT_STRUCT     *wsox_ptr = NU_NULL;
    struct addr_struct      server;
    CHAR                    *buffer, *key;
    INT                     buf_len, socketd;
    WSOX_CLIENT_STRUCT      *cli_struct;
    VOID                    *ssl_ptr = NU_NULL;
#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    SSL                     *ssl = NU_NULL;
    SSL_CTX                 *ctx = NU_NULL;
    INT                     type;
#endif

    /* Store the port in the address structure. */
    server.port = context_ptr->port;

    /* Resolve the host into an IP address. */
    status = WSOX_Resolve_Host(context_ptr->host, server.id.is_ip_addrs,
                               &server.family);

    if (status == NU_SUCCESS)
    {
        /* Create a new socket for the connection. */
        socketd = NU_Socket(server.family, NU_TYPE_STREAM, 0);

        if (socketd >= 0)
        {
            /* Wait our turn to connect to the foreign server. */
            cli_struct = WSOX_Queue_Client_Request(&server, &status);

            if (status == NU_SUCCESS)
            {
                /* Connect to the foreign server. */
                status = NU_Connect(socketd, &server, 0);

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
                if (status >= 0)
                {
                    /* If this is a secure session, then enable CyaSSL. */
                    if (context_ptr->flag & NU_WSOX_SECURE)
                    {
                        /* Create a context structure for CyaSSL. */
#if defined(CYASSL_DTLS)
                        ctx = CyaSSL_CTX_new(CyaDTLSv1_client_method());
#elif  !defined(NO_TLS)
                        ctx = CyaSSL_CTX_new(CyaSSLv23_client_method());
#else
                        ctx = CyaSSL_CTX_new(CyaSSLv3_client_method());
#endif

                        if (ctx != NU_NULL)
                        {
#if (CFG_NU_OS_NET_SSL_LITE_NO_FILESYSTEM == NU_FALSE)
                            /* If the CA is pointing to a file. */
                            if (context_ptr->flag & WSOX_CA_FILE)
                            {
#if (CFG_NU_OS_NET_SSL_LITE_CYASSL_DER_LOAD == NU_TRUE)

                                /* If the CA certificate file has a binary extension, set the type
                                 * accordingly.
                                 */
                                if ( (NU_STRICMP(&context_ptr->ssl_ca[strlen(context_ptr->ssl_ca) - 3],
                                                 "der") == 0) ||
                                     (NU_STRICMP(&context_ptr->ssl_ca[strlen(context_ptr->ssl_ca) - 3],
                                                 "cer") == 0) )
                                {
                                    type = SSL_FILETYPE_ASN1;
                                }
                                else
                                {
                                    type = SSL_FILETYPE_PEM;
                                }

                                status = CyaSSL_CTX_der_load_verify_locations(ctx,
                                                                              context_ptr->ssl_ca,
                                                                              type);
#else
                                status = CyaSSL_CTX_load_verify_locations(ctx, context_ptr->ssl_ca, 0);
#endif
                            }

                            else
#endif
                            if (context_ptr->ssl_ca_size > 0)
                            {
                                /* Determine whether this is a PEM or DER CA. */
                                if (context_ptr->flag & WSOX_CA_PEM_PTR)
                                {
                                    type = SSL_FILETYPE_PEM;
                                }

                                else if (context_ptr->flag & WSOX_CA_DER_PTR)
                                {
                                    type = SSL_FILETYPE_ASN1;
                                }

                                else
                                {
                                    status = NU_INVALID_PARM;
                                }

                                if (status != NU_INVALID_PARM)
                                {
                                    /* Load CA */
                                    status = CyaSSL_CTX_load_verify_buffer(ctx,
                                                                           (const unsigned char*)context_ptr->
                                                                           ssl_ca, context_ptr->ssl_ca_size,
                                                                           type);
                                }
                            }

                            else
                            {
                                status = NU_INVALID_PARM;
                            }

                            /* If the client passed in a certificate to be used with the server. */
                            if ( (status == SSL_SUCCESS) && (context_ptr->ssl_cert) )
                            {
 #if (CFG_NU_OS_NET_SSL_LITE_NO_FILESYSTEM == NU_FALSE)
                                /* If the certificate is in a file, load it from the file. */
                                if (context_ptr->flag & WSOX_CERT_FILE)
                                {
                                    /* If the certificate file has a binary extension, set the type
                                     * accordingly.
                                     */
                                    if ( (NU_STRICMP(&context_ptr->ssl_cert
                                                     [strlen(context_ptr->ssl_cert) - 3], "der") == 0) ||
                                         (NU_STRICMP(&context_ptr->ssl_cert
                                                     [strlen(context_ptr->ssl_cert) - 3], "cer") == 0) )
                                    {
                                        type = SSL_FILETYPE_ASN1;
                                    }
                                    else
                                    {
                                        type = SSL_FILETYPE_PEM;
                                    }

                                    /* Load the certificate. */
                                    status = CyaSSL_CTX_use_certificate_file(ctx, context_ptr->ssl_cert,
                                                                             type);
                                }

                                else
#endif
                                if (context_ptr->ssl_cert_size > 0)
                                {
                                    /* Determine whether this is a PEM or DER certificate. */
                                    if (context_ptr->flag & WSOX_CERT_PEM_PTR)
                                    {
                                        type = SSL_FILETYPE_PEM;
                                    }

                                    else if (context_ptr->flag & WSOX_CERT_DER_PTR)
                                    {
                                        type = SSL_FILETYPE_ASN1;
                                    }

                                    else
                                    {
                                        status = NU_INVALID_PARM;
                                    }

                                    if (status != NU_INVALID_PARM)
                                    {
                                        /* Load the certificate that will be sent to the server
                                         * for verification.
                                         */
                                        status = CyaSSL_CTX_use_certificate_buffer(ctx,
                                                                                   (const unsigned char *)context_ptr->ssl_cert,
                                                                                   context_ptr->ssl_cert_size, type);
                                    }
                                }

                                else
                                {
                                    status = NU_INVALID_PARM;
                                }
                            }

                            /* If the client passed in a key to be used with the server. */
                            if ( (status == SSL_SUCCESS) && (context_ptr->ssl_key) )
                            {
#if (CFG_NU_OS_NET_SSL_LITE_NO_FILESYSTEM == NU_FALSE)
                                /* If the key is in a file, load it from the file. */
                                if (context_ptr->flag & WSOX_KEY_FILE)
                                {
                                    /* If the key file has a binary extension, set the type accordingly. */
                                    if ( (NU_STRICMP(&context_ptr->ssl_key
                                                     [strlen(context_ptr->ssl_key) - 3], "der") == 0) ||
                                         (NU_STRICMP(&context_ptr->ssl_key
                                                     [strlen(context_ptr->ssl_key) - 3], "cer") == 0) )
                                    {
                                        type = SSL_FILETYPE_ASN1;
                                    }
                                    else
                                    {
                                        type = SSL_FILETYPE_PEM;
                                    }

                                    /* Load the key. */
                                    status = CyaSSL_CTX_use_PrivateKey_file(ctx, context_ptr->ssl_key, type);
                                }

                                else
#endif
                                if (context_ptr->ssl_key_size > 0)
                                {
                                    /* Determine whether this is a PEM or DER key. */
                                    if (context_ptr->flag & WSOX_KEY_PEM_PTR)
                                    {
                                        type = SSL_FILETYPE_PEM;
                                    }

                                    else if (context_ptr->flag & WSOX_KEY_DER_PTR)
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
                                        status = CyaSSL_CTX_use_PrivateKey_buffer(ctx,
                                                                                  (const unsigned char *)context_ptr->ssl_key,
                                                                                  context_ptr->ssl_key_size, type);
                                    }
                                }

                                else
                                {
                                    status = NU_INVALID_PARM;
                                }
                            }

                            if (status == SSL_SUCCESS)
                            {
                                /* Determine whether the caller wants to verify the server's
                                 * certificate.
                                 */
                                if (context_ptr->flag & WSOX_VERIFY_PEER)
                                {
                                    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, 0);
                                }

                                else
                                {
                                    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);
                                }

                                ssl = CyaSSL_new(ctx);

                                if (ssl != NU_NULL)
                                {
                                    CyaSSL_set_fd(ssl, socketd);

                                    /* If the user has not indicated to not check the domain
                                     * of the peer before connecting over SSL.
                                     */
                                    if (!(context_ptr->flag & WSOX_NO_DOMAIN_CHECK))
                                    {
                                        status = CyaSSL_check_domain_name(ssl, context_ptr->host);
                                    }

                                    if (status == SSL_SUCCESS)
                                    {
                                        if (CyaSSL_connect(ssl) == SSL_SUCCESS)
                                        {
                                            ssl_ptr = ssl;
                                        }

                                        else
                                        {
                                            status = WSOX_SSL_ERROR;
                                        }
                                    }
                                }

                                else
                                {
                                    status = WSOX_SSL_ERROR;
                                }
                            }
                        }

                        else
                        {
                            status = WSOX_SSL_ERROR;
                        }
                    }
                }
#endif

                if (status >= 0)
                {
                    /* Build the HTTP GET request. */
                    status = WSOX_Build_Client_Request(context_ptr, &buffer,
                                                       &buf_len, &key);

                    if (status == NU_SUCCESS)
                    {
                        /* Send the HTTP GET request. */
                        status = WSOX_Send(socketd, buffer, buf_len, ssl_ptr);

                        if (status > 0)
                        {
                            /* Wait for an HTTP response. */
                            status = WSOX_Client_Wait_For_Data(socketd);

                            if (status == NU_SUCCESS)
                            {
                                /* Process the response. */
                                status = WSOX_Process_Connection_Response(context_ptr,
                                                                          socketd, key,
                                                                          ssl_ptr);
                            }
                        }

                        /* Deallocate the memory for the GET request. */
                        NU_Deallocate_Memory(buffer);
                    }
                }

                /* Get the WebSocket semaphore. */
                temp_status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

                if (temp_status == NU_SUCCESS)
                {
                    /* Remove this handle from the list of handles trying to
                     * connect.
                     */
                    DLL_Remove(&WSOX_Pending_List, cli_struct);

                    NU_Deallocate_Memory(cli_struct);

                    /* Wake up the next handle waiting to connect to this
                     * foreign server.
                     */
                    if (WSOX_Pending_List.head)
                    {
                        NU_Resume_Task(WSOX_Pending_List.head->task_ptr);
                    }

                    /* If the connection was established. */
                    if (status == NU_SUCCESS)
                    {
                        /* Create the new context structure. */
                        wsox_ptr = WSOX_Create_Context(context_ptr, &temp_status);

                        if (temp_status == NU_SUCCESS)
                        {
                            /* Store a pointer to the socket. */
                            wsox_ptr->socketd = socketd;

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
                            wsox_ptr->ssl = ssl;
                            wsox_ptr->ctx = ctx;
#endif

                            /* Disable blocking on this socket so the application and
                             * Master Task cannot lock up the system pending data.
                             */
                            NU_Fcntl(wsox_ptr->socketd, NU_SETFLAG, NU_NO_BLOCK);

                            /* Return the handle to the caller. */
                            *user_handle = wsox_ptr->user_handle;

                            /* Inform the server thread to start handling
                             * incoming data over this handle.
                             */
                            temp_status = WSOX_Setup_Recv_Handle(wsox_ptr);
                        }
                    }

                    /* Release the semaphore. */
                    (VOID)NU_Release_Semaphore(&WSOX_Resource);
                }

                /* If the connection was established, but something internal
                 * went wrong since establishing the connection, clean up and
                 * close the connection.  We do not need the semaphore since
                 * the handle is not on any global lists at this point.
                 */
                if ( (status == NU_SUCCESS) && (temp_status != NU_SUCCESS) )
                {
                    /* Send a connection close message with the specified error. */
                    (VOID)WSOX_TX_Close(socketd, NU_TRUE, WSOX_UNEXPECTED_COND, NU_NULL,
                                        ssl_ptr);

                    if (wsox_ptr)
                    {
                        NU_Deallocate_Memory(wsox_ptr);
                    }

                    status = temp_status;
                }
            }

            if (status != NU_SUCCESS)
            {
#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
                if (ssl)
                {
                    CyaSSL_free(ssl);
                }

                if (ctx)
                {
                    CyaSSL_CTX_free(ctx);
                }
#endif

                NU_Close_Socket(socketd);
            }
        }

        else
        {
            status = socketd;
        }
    }

    return (status);

} /* WSOX_Create_Client_Handle */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Process_Connection_Response
*
*   DESCRIPTION
*
*       This routine processes the WebSocket server's response to a
*       connection request from the local node.
*
*   INPUTS
*
*       *wsox_ptr               The client handle attempting to connect
*                               to the foreign server.
*       socketd                 The socket on which to receive the
*                               connection response.
*       *key                    A pointer to the key that was used in the
*                               connection request.
*
*   OUTPUTS
*
*       NU_SUCCESS              The connection was successfully made.
*       WSOX_CNXN_ERROR         The server rejected the connection.
*       WSOX_PKT_ERROR          The server's response was invalid.
*       WSOX_PROTOCOL_ERROR     The server does not support the requested
*                               protocol(s).
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATIC STATUS WSOX_Process_Connection_Response(NU_WSOX_CONTEXT_STRUCT *wsox_ptr,
                                               INT socketd, CHAR *key,
                                               VOID *ssl_ptr)
{
    NU_MEMORY_POOL      *memory_ptr;
    STATUS              status, temp_status;
    CHAR                *buffer;
    INT32               rx_len, total_len;
    CHAR                key_buf[WSOX_SVR_RSP_CONST_LEN];
    INT                 key_buf_len;
    CHAR                *field_ptr;
    UINT32              socket_count;

    /* Determine the number of bytes on the socket. */
    socket_count = WSOX_Check_For_Data(socketd, ssl_ptr);

    /* If there is data on the socket. */
    if (socket_count > WSOX_END_CTRL_LEN)
    {
        /* Get a pointer to the system memory pool. */
        status = NU_System_Memory_Get(&memory_ptr, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Allocate a buffer for the response. */
            status = NU_Allocate_Memory(memory_ptr, (VOID**)&buffer, socket_count,
                                        NU_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {
            /* Get the first four bytes of the header. */
            rx_len = WSOX_Recv(socketd, buffer, WSOX_END_CTRL_LEN, ssl_ptr);

            total_len = rx_len;

            /* Receive the rest of the header. */
            if (total_len == WSOX_END_CTRL_LEN)
            {
                /* Do not receive more bytes than are on the socket. */
                while (total_len < socket_count)
                {
                    /* If this is the end of the header, exit the loop.
                     * There is more data in this buffer than just the
                     * header.  We need to leave the remainder of the data
                     * on the socket to be processed by the receive task.
                     */
                    if (memcmp(&buffer[total_len - WSOX_END_CTRL_LEN],
                               "\r\n\r\n", WSOX_END_CTRL_LEN) == 0)
                    {
                        break;
                    }

                    /* Receive one more byte. */
                    rx_len = WSOX_Recv(socketd, &buffer[total_len], 1, ssl_ptr);

                    if (rx_len > 0)
                    {
                        total_len += rx_len;
                    }

                    else
                    {
                        total_len = rx_len;
                        break;
                    }
                }
            }

            /* Ensure we received the entire header. */
            if ( (total_len > 0) &&
                 (memcmp(&buffer[total_len - WSOX_END_CTRL_LEN], "\r\n\r\n",
                         WSOX_END_CTRL_LEN) == 0) )
            {
                /* Check the HTTP status. */
                if ( (sscanf(buffer, "HTTP/1.%*d %03d", (int*)&temp_status) == 1) &&
                     (temp_status == WSOX_PROTO_SUCCESS) )
                {
                    /* Initialize the status to indicate there is a packet error. */
                    status = WSOX_PKT_ERROR;

                    /* RFC 6455 - section 4.1.1 - If the response lacks an
                     * |Upgrade| header field or the |Upgrade| header field
                     * contains a value that is not an ASCII case-insensitive
                     * match for the value "websocket", the client MUST
                     * _Fail the WebSocket Connection_.
                     */
                    field_ptr = WSOX_Parse_Header_Field(buffer, total_len, WSOX_UPGRADE,
                                                        &temp_status);

                    if ( (field_ptr) && (NCL_Stricmp(field_ptr, "websocket") == 0) )
                    {
                        /* RFC 6455 - section 4.1.1 - If the response lacks a
                         * |Connection| header field or the |Connection| header
                         * field doesn't contain a token that is an ASCII
                         * case-insensitive match for the value "Upgrade", the client
                         * MUST _Fail the WebSocket Connection_.
                         */
                        field_ptr = WSOX_Parse_Header_Field(buffer, total_len, WSOX_CONNECTION,
                                                            &temp_status);

                        if ( (field_ptr) && (NCL_Stricmp(field_ptr, "upgrade") == 0) )
                        {
                            /* RFC 6455 - section 4.1.1 - If the response lacks a
                             * |Sec-WebSocket-Accept| header field or the
                             * |Sec-WebSocket-Accept| contains a value other than the
                             * base64-encoded SHA-1 of the concatenation of the
                             * |Sec-WebSocket-Key| (as a string, not base64-decoded)
                             * with the string "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
                             * but ignoring any leading and trailing whitespace, the
                             * client MUST _Fail the WebSocket Connection_.
                             */
                            field_ptr = WSOX_Parse_Header_Field(buffer, total_len,
                                                                WSOX_SEC_ACCEPT,
                                                                &temp_status);

                            if (field_ptr)
                            {
                                key_buf_len = (WSOX_SRV_KEY_LEN - 1);

                                /* Encode the client key. */
                                temp_status = WSOX_Create_Accept_Key(key, WSOX_CLNT_KEY_LEN - 2,
                                                                     key_buf, &key_buf_len);

                                /* Check that the keys match.  Subtract 1 from the key_buf_len for
                                 * the \n character.
                                 */
                                if ( (temp_status == NU_SUCCESS) &&
                                     (strlen(field_ptr) == (key_buf_len - 1)) &&
                                     (memcmp(field_ptr, key_buf, (key_buf_len - 1)) == 0) )
                                {
                                    status = WSOX_PROTOCOL_ERROR;

                                    /* RFC 6455 - section 4.1.1 - If the response includes
                                     * a |Sec-WebSocket-Extensions| header field and this
                                     * header field indicates the use of an extension that
                                     * was not present in the client's handshake, the client
                                     * MUST _Fail the WebSocket Connection_.
                                     */
                                    field_ptr = WSOX_Parse_Header_Field(buffer, total_len,
                                                                        WSOX_SEC_EXT,
                                                                        &temp_status);

                                    /* This implementation does not support extensions at
                                     * this time.
                                     */
                                    if (!field_ptr)
                                    {
                                        /* RFC 6455 - section 4.1.1 - If the response includes
                                         * a |Sec-WebSocket-Protocol| header field and this
                                         * header field indicates the use of a subprotocol that
                                         * was not present in the client's handshake (the server
                                         * has indicated a subprotocol not requested by the client),
                                         * the client MUST _Fail the WebSocket Connection_.
                                         */
                                        field_ptr = WSOX_Parse_Header_Field(buffer, total_len,
                                                                            WSOX_SEC_PROTOCOL,
                                                                            &temp_status);

                                        if (field_ptr)
                                        {
                                            /* Remove all the whitespace from the server's
                                             * protocol list for the next comparison.
                                             */
                                            WSOX_Compress_Whitespace(field_ptr);

                                            /* Ensure the protocol list matches. */
                                            if (WSOX_Compare_Protocol_Lists(wsox_ptr->protocols,
                                                                            field_ptr,
                                                                            NU_FALSE) == NU_TRUE)
                                            {
                                                status = NU_SUCCESS;
                                            }
                                        }

                                        /* If the client did not send any protocols, the
                                         * connection is a success.
                                         */
                                        else if (!wsox_ptr->protocols)
                                        {
                                            status = NU_SUCCESS;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (status != NU_SUCCESS)
                    {
                        /* Fail the WebSocket connection by sending a close command to
                         * the other side.
                         */
                        (VOID)WSOX_TX_Close(socketd, NU_TRUE, WSOX_PROTO_ERROR,
                                            NU_NULL, ssl_ptr);
                    }
                }

                /* The other side rejected the connection. */
                else
                {
                    status = WSOX_CNXN_ERROR;
                }
            }

            /* Return the error from the receive call. */
            else if (total_len < 0)
            {
                status = total_len;
            }

            else
            {
                status = WSOX_CNXN_ERROR;
            }

            /* Deallocate the receive buffer. */
            NU_Deallocate_Memory(buffer);
        }
    }

    /* There are not enough bytes in the header to be a valid response. */
    else
    {
        status = WSOX_PKT_ERROR;
    }

    return (status);

} /* WSOX_Process_Connection_Response */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Client_Wait_For_Data
*
*   DESCRIPTION
*
*       Wait for data to arrive on a socket.
*
*   INPUTS
*
*       socketd                 The socket on which to wait for data.
*
*   OUTPUTS
*
*       NU_SUCCESS              Data is available on the socket.
*       WSOX_TIMEOUT            The operation timed out.
*
*************************************************************************/
STATIC STATUS WSOX_Client_Wait_For_Data(INT socketd)
{
    FD_SET      readfs;
    STATUS      status;

    /* Initialize the bitmap */
    NU_FD_Init(&readfs);

    /* Set the read bit for the socket. */
    NU_FD_Set(socketd, &readfs);

    /* Wait for a response from the server. */
    status = NU_Select(socketd + 1, &readfs, NU_NULL, NU_NULL,
                       WSOX_CLNT_TIMEOUT);

    if (status == NU_SUCCESS)
    {
        if (NU_FD_Check(socketd, &readfs) != NU_TRUE)
        {
            status = WSOX_TIMEOUT;
        }
    }

    return (status);

} /* WSOX_Client_Wait_For_Data */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Queue_Client_Request
*
*   DESCRIPTION
*
*       If there are multiple connections in process to the same IP
*       address and port, this routine will suspend until it is this
*       handle's turn to connect to the foreign server.
*
*   INPUTS
*
*       *server                 The address structure for the foreign
*                               server.
*       *status_ptr             The status of the call.
*
*   OUTPUTS
*
*       A pointer to the client structure or NU_NULL if the call
*       fails.
*
*************************************************************************/
STATIC WSOX_CLIENT_STRUCT *WSOX_Queue_Client_Request(struct addr_struct *server,
                                                     STATUS *status_ptr)
{
    NU_MEMORY_POOL          *memory_ptr;
    WSOX_CLIENT_STRUCT      *temp_ptr, *client_ptr = NU_NULL;
    STATUS                  status;

    /* Get a pointer to the system memory pool. */
    status = NU_System_Memory_Get(&memory_ptr, NU_NULL);

    if (status == NU_SUCCESS)
    {
        /* Allocate memory for the structure that will be used to indicate
         * that this handle is connecting to the respective server.
         */
        status = NU_Allocate_Memory(memory_ptr, (VOID**)&client_ptr,
                                    sizeof(WSOX_CLIENT_STRUCT), NU_SUSPEND);
    }

    if (status == NU_SUCCESS)
    {
        /* Set up the structure. */
        client_ptr->flink = NU_NULL;
        client_ptr->blink = NU_NULL;
        memcpy(&client_ptr->foreign_addr, server, sizeof(server));
        client_ptr->task_ptr = NU_Current_Task_Pointer();

        /* Get the WebSocket semaphore. */
        status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Add this client to the global list of waiting clients. */
            DLL_Enqueue(&WSOX_Pending_List, client_ptr);

            temp_ptr = WSOX_Pending_List.head;

            /* RFC 6455 section 4.1.1 - If multiple connections to the same
             * IP address are attempted simultaneously, the client MUST
             * serialize them ...
             */
            while (temp_ptr)
            {
                /* If this connection is connecting to the same IP address and
                 * port.
                 */
                if ( (temp_ptr != client_ptr) &&
                     (memcmp(temp_ptr->foreign_addr.id.is_ip_addrs,
                             client_ptr->foreign_addr.id.is_ip_addrs,
                             MAX_ADDRESS_SIZE) == 0) &&
                      (temp_ptr->foreign_addr.port ==
                       client_ptr->foreign_addr.port) )
                {
                    /* Release the semaphore. */
                    (VOID)NU_Release_Semaphore(&WSOX_Resource);

                    /* Suspend until it is our turn to connect to this server. */
                    NU_Suspend_Task(client_ptr->task_ptr);

                    break;
                }

                /* Get the next handle in the list. */
                temp_ptr = temp_ptr->flink;
            }

            if (!temp_ptr)
            {
                /* Release the semaphore. */
                (VOID)NU_Release_Semaphore(&WSOX_Resource);
            }
        }
    }

    *status_ptr = status;

    return (client_ptr);

} /* WSOX_Queue_Client_Request */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Resolve_Host
*
*   DESCRIPTION
*
*       Resolve the host field of a WebSocket connection request to an
*       IP address.
*
*   INPUTS
*
*       *host                   The field to resolve to an IP address.
*                               This could be a host name that will be
*                               resolved via DNS, or it could be an
*                               ASCII representation of an IP address
*                               that will be stored as an integer array.
*       *ip_addr                The IP address of the host.
*       *addr_fam               The family of the IP address.
*
*   OUTPUTS
*
*       NU_SUCCESS              The host was successfully resolved.
*       WSOX_INVALID_HOST       The host field is invalid.
*
*************************************************************************/
STATIC STATUS WSOX_Resolve_Host(CHAR *host, UINT8 *ip_addr, INT16 *addr_fam)
{
    INT16               family;
    STATUS              status;
    CHAR                test_ip[MAX_ADDRESS_SIZE] = {0};
    NU_HOSTENT          *hentry = NU_NULL;

    /* Determine the IP address of the foreign server to which to
     * make the connection.
     */
#if (INCLUDE_IPV6 == NU_TRUE)
    /* Search for a ':' to determine if the address is IPv4 or IPv6. */
    if (strchr(host, ':') != NU_NULL)
    {
        family = NU_FAMILY_IP6;

        /* Convert the string to an array. */
        status = NU_Inet_PTON(NU_FAMILY_IP6, host, test_ip);
    }

    else
#if (INCLUDE_IPV4 == NU_FALSE)
    {
        /* An IPv6 address was not passed into the routine. */
        status = WSOX_INVALID_HOST;
    }
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        family = NU_FAMILY_IP;

        /* Attempt to convert the string to an array.  If this is not an IPv4
         * address, the routine will return an error and we will assume the
         * user passed in a host name.
         */
        status = NU_Inet_PTON(NU_FAMILY_IP, host, test_ip);
    }
#endif

    /* If the URI contains an IP address, copy it into the server structure. */
    if (status == NU_SUCCESS)
    {
        memcpy(ip_addr, test_ip, MAX_ADDRESS_SIZE);
    }

    /* If the application did not pass in an IP address, resolve the host
     * name into a valid IP address.
     */
    else
    {
        /* If IPv6 is enabled, default to IPv6.  If the host does not have
         * an IPv6 address, an IPv4-mapped IPv6 address will be returned that
         * can be used as an IPv6 address.
         */
#if (INCLUDE_IPV6 == NU_TRUE)
        family = NU_FAMILY_IP6;
#else
        family = NU_FAMILY_IP;
#endif

        /* Try getting host info by name */
        hentry = NU_Get_IP_Node_By_Name(host, family, DNS_V4MAPPED, &status);

        if (hentry)
        {
            /* Copy the hentry data into the server structure */
            memcpy(ip_addr, *hentry->h_addr_list, hentry->h_length);
            family = hentry->h_addrtype;

            /* Free the memory associated with the host entry returned */
            NU_Free_Host_Entry(hentry);
        }

        /* If the host name could not be resolved, return an error. */
        else
        {
            status = WSOX_INVALID_HOST;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* The ip_addr field already contains the IP address. */
        *addr_fam = family;
    }

    return (status);

} /* WSOX_Resolve_Host */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Build_Client_Request
*
*   DESCRIPTION
*
*       Builds an HTTP GET request for the WebSocket client.
*
*   INPUTS
*
*       *wsox_ptr               The handle initiating the request.
*       **buf_ptr               A pointer to the buffer containing the
*                               request.
*       *len_ptr                The length of the buffer.
*       **key_ptr               A pointer to be set to the key that was
*                               sent in the connection request.
*
*   OUTPUTS
*
*       NU_SUCCESS if the request was created; otherwise, an operating-
*       system specific error is returned.
*
*************************************************************************/
STATIC STATUS WSOX_Build_Client_Request(NU_WSOX_CONTEXT_STRUCT *wsox_ptr,
                                        CHAR **buf_ptr, INT *len_ptr,
                                        CHAR **key_ptr)
{
    NU_MEMORY_POOL  *memory_ptr;
    CHAR            *buffer;
#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    UINT32          key[4];
    UINT32          encode_len;
#endif
    CHAR            port_string[6];
    STATUS          status;
    INT             buf_len = WSOX_CLNT_RQST_FIX_HDR_LEN;

    /* Add bytes for the resource, host name and key. */
    buf_len += (strlen(wsox_ptr->resource) + strlen(wsox_ptr->host) +
            WSOX_CLNT_KEY_LEN);

    /* If an origin list was included, add bytes for that data. */
    if (wsox_ptr->origins)
    {
        buf_len += (strlen(WSOX_ORIGIN) + strlen(wsox_ptr->origins) + 2);
    }

    /* If a protocol list was included, add bytes for that data. */
    if (wsox_ptr->protocols)
    {
        buf_len += (strlen(WSOX_SEC_PROTOCOL) +
                    strlen(wsox_ptr->protocols) + 2);
    }

    /* If the port number is not the standard HTTP port, it must be
     * appended to the host field.
     */
    if (wsox_ptr->port != WSOX_HTTP_PORT)
    {
        /* Convert the port number to a string. */
        NCL_Itoa(wsox_ptr->port, port_string, 10);

        /* Add an extra byte for the colon. */
        buf_len += (strlen(port_string) + 1);
    }

    /* Get a pointer to the system memory pool. */
    status = NU_System_Memory_Get(&memory_ptr, NU_NULL);

    if (status == NU_SUCCESS)
    {
        /* Allocate memory for the output buffer. */
        status = NU_Allocate_Memory(memory_ptr, (VOID**)&buffer, buf_len,
                                    NU_SUSPEND);
    }

    if (status == NU_SUCCESS)
    {
        strcpy(buffer, "GET ");

        /* Add the resource */
        strcat(buffer, wsox_ptr->resource);
        strcat(buffer, " ");

        /* Add the version. */
        strcat(buffer, "HTTP/1.1");
        strcat(buffer, "\r\n");

        /* Add the Host: header field. */
        strcat(buffer, WSOX_HOST);
        strcat(buffer, wsox_ptr->host);

        /* If the port number needs to be included in the header field. */
        if (wsox_ptr->port != WSOX_HTTP_PORT)
        {
            /* Add the colon and port number. */
            strcat(buffer, ":");
            strcat(buffer, port_string);
        }
        strcat(buffer, "\r\n");

        /* If the caller specified an origin. */
        if (wsox_ptr->origins)
        {
            strcat(buffer, WSOX_ORIGIN);
            strcat(buffer, wsox_ptr->origins);
            strcat(buffer, "\r\n");
        }

        /* If the caller specified a protocol list. */
        if (wsox_ptr->protocols)
        {
            strcat(buffer, WSOX_SEC_PROTOCOL);
            strcat(buffer, wsox_ptr->protocols);
            strcat(buffer, "\r\n");
        }

        /* Add the Upgrade: header field. */
        strcat(buffer, WSOX_UPGRADE_HDR);

        /* Add the Connection: header field. */
        strcat(buffer, WSOX_CONNECTION_HDR);

        /* Add the Sec-WebSocket-Version: header field. */
        strcat(buffer, WSOX_SEC_VERSION);
        strcat(buffer, WSOX_VERSION);
        strcat(buffer, "\r\n");

        /* Add the Sec-WebSocket-Key: header field. */
        strcat(buffer, WSOX_SEC_KEY);

        /* Ensure random number generator is seeded. */
        NU_RTL_Rand_Seed();

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)

        /* Create a 16-byte random value. */
        key[0] = ((UINT32)rand());
        key[1] = ((UINT32)rand());
        key[2] = ((UINT32)rand());
        key[3] = ((UINT32)rand());

        /* Base64_Encode adds a \n character that will be overwritten upon return.
         * WSOX_CLNT_KEY_LEN includes two bytes for \r\n, so there are technically
         * WSOX_CLNT_KEY_LEN - 1 bytes available for the Base64_Encode routine to
         * encode the hashed string.
         */
        encode_len = (WSOX_CLNT_KEY_LEN - 1);

        /* Base 64 encode the key. */
        status = Base64_Encode((byte*)key, 16,
                               (byte*)&buffer[buf_len - WSOX_CLNT_KEY_LEN - 2],
                               (word32 *)&encode_len);

#else
        status = NU_INVALID_PARM;
#endif
        if (status == NU_SUCCESS)
        {
            /* Save a pointer to the key that was sent. */
            *key_ptr = (CHAR*)&buffer[buf_len - WSOX_CLNT_KEY_LEN - 2];

            /* Add the \r\n to the key and the final HTTP header. */
            memcpy(&buffer[buf_len - 4], "\r\n\r\n", 4);
        }
    }

    if (status == NU_SUCCESS)
    {
        *buf_ptr = buffer;
        *len_ptr = buf_len;
    }

    return (status);

} /* WSOX_Build_Client_Request */


