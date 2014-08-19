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
*       sck_wsox.c
*
*   COMPONENT
*
*       Nucleus WebSocket
*
*   DESCRIPTION
*
*       This file holds the Nucleus WebSocket API routines called from
*       an application.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_WSOX_Create_Context
*       NU_WSOX_Accept
*       NU_WSOX_Send
*       NU_WSOX_Close
*       NU_WSOX_Schedule_Ping
*       NU_WSOX_Toggle_Recv
*       NU_WSOX_Recv
*       WSOX_Process_Data
*       NU_WSOX_Setsockopt
*       NU_WSOX_Getsockopt
*
*   DEPENDENCIES
*
*       nu_networking.h
*       wsox_int.h
*
************************************************************************/

#include "networking/nu_networking.h"
#include "os/networking/websocket/wsox_int.h"

extern  NU_SEMAPHORE    WSOX_Resource;
extern  BOOLEAN         WSOX_Notify_Enable;
extern  UINT32          WSOX_Notify_Handle;
extern  NU_TASK         WSOX_Master_Task_CB;

#if (CFG_NU_OS_NET_HTTP_SERVER_ENABLE == NU_TRUE)
extern HTTP_SVR_SESSION_STRUCT  *HTTP_Session;
extern NU_SEMAPHORE             HTTP_Lite_Resource;
#endif

STATIC STATUS WSOX_Process_Data(WSOX_CONTEXT_STRUCT *wsox_ptr, CHAR *buffer,
                                UINT64 *data_len);

/************************************************************************
*
*   FUNCTION
*
*       NU_WSOX_Create_Context
*
*   DESCRIPTION
*
*       Create a new internal WebSocket context structure, and return
*       the handle to the caller.
*
*   INPUTS
*
*       *context_ptr            The structure on which to base the new
*                               structure.
*       *user_handle            A pointer that will be filled in with
*                               the handle for this new context
*                               structure.  This handle will be used
*                               by the application to send/receive data.
*
*   OUTPUTS
*
*       NU_SUCCESS              The new structure was created.  If the
*                               handle is not a listener, the connection
*                               was successfully made with the foreign
*                               server.
*       NU_INVALID_PARM         An input parameter is invalid.
*
*       Client-specific return values:
*
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
STATUS NU_WSOX_Create_Context(NU_WSOX_CONTEXT_STRUCT *context_ptr,
                              UINT32 *user_handle)
{
    STATUS      status = NU_SUCCESS;
    UINT32      flags = WSOX_LOCAL_HANDLE;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Validate the input. */
    if ( (context_ptr) && (user_handle) && (context_ptr->resource) &&
         (context_ptr->onclose) && (context_ptr->onerror) &&
         (context_ptr->onmessage) && (context_ptr->onopen) )
    {
        /* Map the public flags to private flag values. */
        if (context_ptr->flag & NU_WSOX_LISTENER)
        {
            flags |= WSOX_LISTENER;
        }

        /* The handle is a client - all data must be masked. */
        else
        {
            flags |= WSOX_CLIENT;
        }

        /* If the client is connecting to a secure remote server. */
        if (context_ptr->flag & NU_WSOX_SECURE)
        {
#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
            flags |= WSOX_SECURE;

            /* Determine how the certificate is being passed into the routine. */
            if (context_ptr->flag & NU_WSOX_CERT_FILE)
            {
                flags |= WSOX_CERT_FILE;
            }

            else if (context_ptr->flag & NU_WSOX_CERT_PEM_PTR)
            {
                flags |= WSOX_CERT_PEM_PTR;
            }

            else if (context_ptr->flag & NU_WSOX_CERT_DER_PTR)
            {
                flags |= WSOX_CERT_DER_PTR;
            }

            /* Determine how the key is being passed into the routine. */
            if (context_ptr->flag & NU_WSOX_KEY_FILE)
            {
                flags |= WSOX_KEY_FILE;
            }

            else if (context_ptr->flag & NU_WSOX_KEY_PEM_PTR)
            {
                flags |= WSOX_KEY_PEM_PTR;
            }

            else if (context_ptr->flag & NU_WSOX_KEY_DER_PTR)
            {
                flags |= WSOX_KEY_DER_PTR;
            }

            /* Determine how the CA list is being passed into the routine. */
            if (context_ptr->flag & NU_WSOX_CA_FILE)
            {
                flags |= WSOX_CA_FILE;
            }

            else if (context_ptr->flag & NU_WSOX_CA_PEM_PTR)
            {
                flags |= WSOX_CA_PEM_PTR;
            }

            else if (context_ptr->flag & NU_WSOX_CA_DER_PTR)
            {
                flags |= WSOX_CA_DER_PTR;
            }

            if (context_ptr->flag & NU_WSOX_VERIFY_PEER)
            {
                flags |= WSOX_VERIFY_PEER;
            }

            if (context_ptr->flag & NU_WSOX_NO_DOMAIN_CHECK)
            {
                flags |= WSOX_NO_DOMAIN_CHECK;
            }
#else
            /* CyaSSL must be enabled to make a secure connection. */
            status = NU_INVALID_PARM;
#endif
        }

        if (status == NU_SUCCESS)
        {
            /* Overwrite the public flags with the private flags. */
            context_ptr->flag = flags;

            /* If this listener flag is set, this is a new server handle that
             * will listen for incoming connections.
             */
            if (context_ptr->flag & WSOX_LISTENER)
            {
                status = WSOX_Create_Server_Handle(context_ptr, user_handle);
            }

            /* Otherwise, this is a client handle.  Create a new structure and
             * establish the connection with the foreign server.  The host field
             * is required for client connections.
             */
            else if (context_ptr->host)
            {
                status = WSOX_Create_Client_Handle(context_ptr, user_handle);
            }

            else
            {
                status = NU_INVALID_PARM;
            }
        }
    }

    else
    {
        status = NU_INVALID_PARM;
    }

    NU_USER_MODE();

    return (status);

} /* NU_WSOX_Create_Context */

/************************************************************************
*
*   FUNCTION
*
*       NU_WSOX_Accept
*
*   DESCRIPTION
*
*       Performs server-side processing of an incoming connection request
*       from a foreign client and creates a new handle for the connection
*       upon successful completion of the connection.
*
*   INPUTS
*
*       *client_req             The structure on which to base the new
*                               structure that holds the data for the
*                               pending client request.  The parameter
*                               client_req->protocols will be modified to
*                               return only the protocols that are
*                               supported by the listening socket.
*       socketd                 The socket on which the new connection is
*                               being requested.
*       *keys                   A pointer to the null-terminated key used
*                               by the client in the connection request.
*       *ssl                    A pointer to the SSL structure if this is
*                               a secure connection.
*
*   OUTPUTS
*
*       NU_SUCCESS if the connection was created; otherwise, an operating-
*       system specific error is returned.
*
*************************************************************************/
STATUS NU_WSOX_Accept(NU_WSOX_CONTEXT_STRUCT *client_req, INT socketd,
                      CHAR *keys, VOID *ssl)
{
    WSOX_CONTEXT_STRUCT     *wsox_client, *wsox_listener;
    STATUS                  status;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Validate the input. */
    if ( (client_req) && (client_req->resource) && (socketd >= 0) &&
         (keys) )
    {
        /* Get the WebSocket semaphore. */
        status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Find a matching server handle for processing the incoming
             * connection.
             */
            wsox_listener = WSOX_Find_Listener_Context(client_req->resource,
                                                       client_req->origins,
                                                       &status);

            /* If a listener was found. */
            if (wsox_listener)
            {
                /* If this connection is not at full capacity. */
                if (wsox_listener->wsox_server->connections > 0)
                {
                    /* Determine which of the client's protocols are supported by
                     * the server, and delete any protocols that cannot be used.
                     */
                    if (client_req->protocols)
                    {
                        /* We do not care if the two lists match.  Remove anything from
                         * the client's list that is not included in the server's list.
                         */
                        (VOID)WSOX_Compare_Protocol_Lists(client_req->protocols,
                                                          wsox_listener->wsox_server->protocols,
                                                          NU_TRUE);
                    }

                    /* Process the connection request. */
                    status = WSOX_Process_Connection_Request(wsox_listener, client_req,
                                                             socketd, keys, ssl);

                    /* If the connection was established, notify the application of
                     * the new connection.
                     */
                    if (status == NU_SUCCESS)
                    {
                        /* The new handle inherits the callback routines of the listener. */
                        client_req->onclose = wsox_listener->onclose;
                        client_req->onerror = wsox_listener->onerror;
                        client_req->onmessage = wsox_listener->onmessage;
                        client_req->onopen = wsox_listener->onopen;

                        /* Create the new context structure. */
                        wsox_client = WSOX_Create_Context(client_req, &status);

                        if (status == NU_SUCCESS)
                        {
#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
                            if (ssl)
                            {
                                /* Save a pointer to the SSL structure. */
                                wsox_client->ssl = ssl;
                            }
#endif
                            /* Save the socket. */
                            wsox_client->socketd = socketd;

                            /* Disable blocking on this socket so the application and
                             * Master Task cannot lock up the system pending data.
                             */
                            NU_Fcntl(wsox_client->socketd, NU_SETFLAG, NU_NO_BLOCK);

                            /* Link this new connection to the listener, so when this
                             * connection is closed, the listener's max connection value
                             * can be updated.
                             */
                            wsox_client->wsox_server = wsox_listener->wsox_server;

                            /* Inform the server thread to start handling incoming data
                             * for this handle.
                             */
                            status = WSOX_Setup_Recv_Handle(wsox_client);

                            /* If the connection could not be completed, or the server could
                             * not be resumed, deallocate the new handle.
                             */
                            if (status != NU_SUCCESS)
                            {
                                /* Fail the WebServ connection by sending a close. */

                                /* Deallocate the memory for the client. */
                                NU_Deallocate_Memory(wsox_client);
                            }

                            else
                            {
                                /* Decrement the number of available connections. */
                                wsox_listener->wsox_server->connections --;
                            }
                        }
                    }
                }

                else
                {
                    status = WSOX_MAX_CONXNS;
                }
            }

            (VOID)NU_Release_Semaphore(&WSOX_Resource);
        }

        else
        {
            status = NU_INVAL;
        }
    }

    else
    {
        status = NU_INVALID_PARM;
    }

    NU_USER_MODE();

    return (status);

} /* NU_WSOX_Accept */

/************************************************************************
*
*   FUNCTION
*
*       NU_WSOX_Send
*
*   DESCRIPTION
*
*       Send data to the foreign side of the WebSocket connection.  If
*       this is a fragment, note the following rules per RFC 6455:
*
*          o  The fragments of one message MUST NOT be interleaved
*             between the fragments of another message.  The routine
*             has no way to verify this, so the application must ensure
*             this rule is followed.
*
*          o  Control frames cannot be fragmented.
*
*          o  Since control frames cannot be fragmented, the type
*             for all fragments in a message MUST be either text,
*             binary, or one of the reserved opcodes.
*
*   INPUTS
*
*       handle                  The WebSocket handle representing the
*                               connection over which to send data.
*       *buffer                 A pointer to the buffer of data to
*                               send.  This buffer can be NU_NULL if
*                               data_len is zero.
*       *data_len               The length of the data in the buffer on
*                               input.  On return, the number of bytes
*                               transmitted. This value can be zero if
*                               buffer is NU_NULL.
*       opcode                  The type of data in the buffer:
*                                   WSOX_TEXT_FRAME - Textual data.
*                                   WSOX_BINARY_FRAME - Binary data.
*                                   WSOX_PING_FRAME - Ping frame.
*                                   WSOX_PONG_FRAME - Pong frame.
*                               This field is not checked for validity, as
*                               the user could send their own opcode types.
*       flags                   Flags associated with the transmission:
*                                   NU_WSOX_FRAGMENT - There are more bytes
*                                   to be included in the message than being
*                                   transmitted with this function call.
*
*   OUTPUTS
*
*       NU_SUCCESS              All of the data was successfully sent.
*       NU_INVALID_PARM         An input parameter is invalid.
*       WSOX_INVALID_HANDLE     The handle does not represent an open
*                               connection.
*       WSOX_INVALID_OPCODE     The caller is attempting to fragment a
*                               control frame.
*       WSOX_CNXN_ERROR         The TCP connection has been lost.
*
*       Otherwise, an operating-system specific error is returned.
*       In this case, some of the data could have been successfully
*       transmitted.  The caller should always check the return value of
*       data_len when an operating-system specific error is returned.
*
*************************************************************************/
STATUS NU_WSOX_Send(UINT32 handle, CHAR *buffer, UINT64 *data_len, INT opcode,
                    UINT8 flags)
{
    WSOX_CONTEXT_STRUCT *wsox_ptr;
    STATUS              status;
    UINT64              bytes_sent = 0, bytes_to_send = 0, tx_bytes;
    INT                 socketd;
    FD_SET              writefs;
    CHAR                mask[WSOX_MASK_LEN];

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Validate the input. */
    if ( (data_len) && (((buffer) && (*data_len > 0)) ||
         ((!buffer) && (*data_len == 0))) )
    {
        /* If the fragment flag is set, ensure this is not a control
         * frame.
         */
        if ( (!(flags & NU_WSOX_FRAGMENT)) || (!(opcode & 0x8)) )
        {
            bytes_to_send = *data_len;

            /* Send the data. */
            do
            {
                /* Get the WebSocket semaphore. */
                status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Find a matching handle. */
                    wsox_ptr = WSOX_Find_Context_By_Handle(handle);

                    /* Check that the connection can send data, and that this is
                     * not a listener handle.  Only non-listener sockets will be flagged
                     * WSOX_TX, so there is no need to check that this is not a
                     * listener.
                     */
                    if ( (wsox_ptr) && (wsox_ptr->flag & WSOX_TX) )
                    {
                        /* Get the socket associated with this handle. */
                        socketd = wsox_ptr->socketd;

                        /* Release the semaphore so we can suspend this thread. */
                        NU_Release_Semaphore(&WSOX_Resource);

                        /* Initialize the bitmap */
                        NU_FD_Init(&writefs);

                        /* Set the write bit for the socket. */
                        NU_FD_Set(socketd, &writefs);

                        /* Wait until the socket is writable. */
                        status = NU_Select(socketd + 1, NU_NULL, &writefs, NU_NULL,
                                           NU_SUSPEND);

                        if (status == NU_SUCCESS)
                        {
                            /* Check this socket for writability. */
                            if (NU_FD_Check(socketd, &writefs) == NU_TRUE)
                            {
                                /* Get the WebSocket semaphore. */
                                status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

                                if (status == NU_SUCCESS)
                                {
                                    /* Set the number of bytes remaining in the buffer. */
                                    tx_bytes = bytes_to_send;

                                    /* Send the frame. */
                                    status = WSOX_Send_Frame(wsox_ptr, buffer != NU_NULL ?
                                                             &buffer[bytes_sent] : NU_NULL,
                                                             &tx_bytes, opcode, flags,
                                                             bytes_sent, mask);

                                    /* If the caller is not sending a zero byte message. */
                                    if (bytes_to_send)
                                    {
                                        /* Decrement the outstanding number of bytes. */
                                        bytes_to_send -= tx_bytes;
                                    }

                                    /* Increment the number of bytes sent. */
                                    bytes_sent += tx_bytes;

                                    if (status != NU_SUCCESS)
                                    {
                                        /* If the data could not be sent due to the congestion
                                         * window or low buffers, try to send the rest of the
                                         * data.  Otherwise, return the error.
                                         */
                                        if (status == NU_WOULD_BLOCK)
                                        {
                                            /* If at least the first packet was sent that
                                             * contains the WebSocket header, do not send
                                             * the header with each subsequent packet.
                                             */
                                            if (bytes_sent)
                                            {
                                                flags |= NU_WSOX_NO_HDR;
                                            }
                                        }

                                        else
                                        {
                                            /* Break out of the loop. */
                                            bytes_to_send = 0;
                                        }
                                    }

                                    NU_Release_Semaphore(&WSOX_Resource);
                                }

                                else
                                {
                                    break;
                                }
                            }
                        }

                        else
                        {
                            break;
                        }
                    }

                    else
                    {
                        status = WSOX_INVALID_HANDLE;

                        /* Break out of the loop. */
                        bytes_to_send = 0;

                        NU_Release_Semaphore(&WSOX_Resource);
                    }
                }

                else
                {
                    break;
                }

            } while (bytes_to_send);
        }

        /* Control frames cannot be fragmented. */
        else
        {
            status = WSOX_INVALID_OPCODE;
        }

        /* Return the number of bytes sent. */
        *data_len = bytes_sent;
    }

    else
    {
        if (data_len)
        {
            *data_len = 0;
        }

        status = NU_INVALID_PARM;
    }

    NU_USER_MODE();

    return (status);

} /* NU_WSOX_Send */

/************************************************************************
*
*   FUNCTION
*
*       NU_WSOX_Close
*
*   DESCRIPTION
*
*       Closes the local side of the connection.  If a close frame has
*       not already been received, the application may continue to
*       receive data over this handle.  Otherwise, the connection is
*       closed for both sending and receiving data.
*
*   INPUTS
*
*       handle                  The WebSocket handle representing the
*                               connection to close.
*       status_code             The status code to include in the close
*                               frame.
*       *reason                 The UTF-8 encoded string to include as
*                               the reason for the closure.
*
*   OUTPUTS
*
*       NU_SUCCESS              The close frame was successfully sent.
*       WSOX_INVALID_HANDLE     The handle is invalid.
*       WSOX_MSG_TOO_BIG        The reason is too big.  A close frame
*                               may be only 125 bytes in length.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS NU_WSOX_Close(UINT32 handle, UINT16 status_code, CHAR *reason)
{
    WSOX_CONTEXT_STRUCT *wsox_ptr;
    STATUS              status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Get the WebSocket semaphore. */
    status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Find a matching handle. */
        wsox_ptr = WSOX_Find_Context_By_Handle(handle);

        /* Ensure the handle exists and a close has not already been
         * sent.
         */
        if ( (wsox_ptr) &&
             ((wsox_ptr->flag & WSOX_TX) || (wsox_ptr->flag & WSOX_LISTENER)) )
        {
            /* If this is not a listener handle, send the close frame. */
            if (!(wsox_ptr->flag & WSOX_LISTENER))
            {
                /* Send the close frame. */
                status = WSOX_TX_Close(wsox_ptr->socketd, wsox_ptr->flag & WSOX_CLIENT,
                                       status_code, reason, wsox_ptr->ssl);

                /* If the message is too large, notify the application so another
                 * call with a shorter message can be invoked.
                 */
                if (status != WSOX_MSG_TOO_BIG)
                {
                    /* The endpoint must stop sending data after issuing the close
                     * command.
                     */
                    wsox_ptr->flag &= ~WSOX_TX;

                    /* Stop sending PINGs. */
                    wsox_ptr->flag &= ~WSOX_PING;

                    /* If the RX flag is clear, or the transmission failed due to the
                     * connection already being closed, the connection is closed.
                     */
                    if ( (!(wsox_ptr->flag & WSOX_RX)) || (status != NU_SUCCESS) )
                    {
                        /* After both sending and receiving a Close message, an
                         * endpoint considers the WebSocket connection closed and
                         * MUST close the underlying TCP connection.  The server
                         * MUST close the underlying TCP connection immediately;
                         * the client SHOULD wait for the server to close the
                         * connection but MAY close the connection at any time after
                         * sending and receiving a Close message, e.g., if it has
                         * not received a TCP Close from the server in a reasonable
                         * time period.
                         */
                        if ( (!(wsox_ptr->flag & WSOX_CLIENT)) ||
                             (status != NU_SUCCESS) )
                        {
                            /* Clean up the connection entry. */
                            WSOX_Cleanup_Connection_Entry(wsox_ptr);
                        }

                        else
                        {
                            /* Set a timestamp. */
                            wsox_ptr->timestamp =
                                NU_Retrieve_Clock() + WSOX_CLNT_CLOSE_TIMEOUT;
                        }

                        /* Wake up the server. */
                        WSOX_Resume_Server();
                    }
                }
            }

            /* Clean up the listener. */
            else
            {
                WSOX_Cleanup_Connection_Entry(wsox_ptr);
            }
        }

        else
        {
            status = WSOX_INVALID_HANDLE;
        }

        NU_Release_Semaphore(&WSOX_Resource);
    }

    NU_USER_MODE();

    return (status);

} /* NU_WSOX_Close */

/************************************************************************
*
*   FUNCTION
*
*       NU_WSOX_Schedule_Ping
*
*   DESCRIPTION
*
*       This routine will schedule a PING control frame to be transmitted
*       to the destination at the indicated rate.  Any PONG control frame
*       received will trigger the onmessage() callback routine associated
*       with the handle.
*
*   INPUTS
*
*       handle                  The handle for which to schedule the
*                               PING control frame transmission.
*       interval                The interval, in clock ticks, at which to
*                               transmit PING control frames.  Setting this
*                               value to zero cancels a previous call to
*                               this routine.
*       delay                   The delay, in clock ticks, between unanswered
*                               PING control frames.  This value can be set to
*                               zero only if retrans_count is also zero, which
*                               indicates that the PING frame will not be
*                               retransmitted when no PONG is received in the
*                               interval timeout.
*       retrans_count           The number of unanswered PING frames to
*                               retransmit before notifying the application
*                               via onerror().  If this value is set to zero,
*                               the delay input parameter is ignored.
*
*   OUTPUTS
*
*       NU_SUCCESS              The PING was successfully scheduled.
*       WSOX_INVALID_HANDLE     The handle is invalid.
*       WSOX_DUP_REQUEST        A PING has already been scheduled for
*                               this handle.  The caller must cancel
*                               an existing outstanding scheduled
*                               PING to change the interval parameters.
*       NU_INVALID_PARM         Delay is set to zero when retrans_count
*                               is non-zero.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS NU_WSOX_Schedule_Ping(UINT32 handle, UINT32 interval, UINT32 delay,
                             UINT8 retrans_count)
{
    WSOX_CONTEXT_STRUCT *wsox_ptr;
    STATUS              status;
    UINT64              data_len;
    CHAR                mask[WSOX_MASK_LEN];

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Validate the input. */
    if ( (interval == 0) || ((delay != 0) || (retrans_count == 0)) )
    {
        /* Get the WebSocket semaphore. */
        status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Find a matching handle. */
            wsox_ptr = WSOX_Find_Context_By_Handle(handle);

            /* Check that the connection can send data, and that this is
             * not a listener handle.  Only non-listener sockets will be
             * flagged WSOX_TX, so there is no need to check that this is
             * not a listener.
             */
            if ( (wsox_ptr) && (wsox_ptr->flag & WSOX_TX) )
            {
                /* If the PING is being scheduled. */
                if (interval)
                {
                    /* If a PING has not already been scheduled for this
                     * handle.
                     */
                    if (!(wsox_ptr->flag & WSOX_PING))
                    {
                        /* Set the flag indicating that this handle is
                         * transmitting periodic PING control frames.
                         */
                        wsox_ptr->flag |= WSOX_PING;

                        /* Save the transmission values. */
                        wsox_ptr->ping_interval = interval;
                        wsox_ptr->retrans_interval = delay;
                        wsox_ptr->max_retrans_count = retrans_count;

                        /* Initialize the number of retransmissions sent. */
                        wsox_ptr->retrans_count = 0;
                        wsox_ptr->pong_timestamp = 0;

                        /* Schedule the first PING. */
                        wsox_ptr->timestamp = NU_Retrieve_Clock() + interval;

                        data_len = strlen(WSOX_PING_MESSAGE);

                        /* Send the first PING frame. */
                        WSOX_Send_Frame(wsox_ptr, WSOX_PING_MESSAGE,
                                        &data_len, WSOX_PING_FRAME, 0, 0, mask);
                    }

                    else
                    {
                        status = WSOX_DUP_REQUEST;
                    }
                }

                /* The PING is being canceled. */
                else
                {
                    /* Remove the PING flag from the handle. */
                    wsox_ptr->flag &= ~WSOX_PING;

                    wsox_ptr->timestamp = 0;
                }

                if (status == NU_SUCCESS)
                {
                    /* Wake the server. */
                    status = WSOX_Resume_Server();
                }
            }

            else
            {
                status = WSOX_INVALID_HANDLE;
            }

            NU_Release_Semaphore(&WSOX_Resource);
        }
    }

    else
    {
        status = NU_INVALID_PARM;
    }

    NU_USER_MODE();

    return (status);

} /* NU_WSOX_Schedule_Ping */

/************************************************************************
*
*   FUNCTION
*
*       NU_WSOX_Toggle_Recv
*
*   DESCRIPTION
*
*       This routine toggles receive notifications on the specified
*       handle.  When set to NU_FALSE, the WebSocket master task will
*       not invoke onmessage() when an incoming message is received.
*       When set to NU_TRUE, the WebSocket master task will resume
*       invoking onmessage() when an incoming message is received.
*
*       Note that if the application does not remove all data from a
*       handle when onmessage() is invoked, and receive notifications
*       are not disabled for that handle from onmessage(), onmessage()
*       will be immediately invoked again.
*
*       This routine MAY be invoked from the onmessage() callback
*       routine registered with the respective handle or from any other
*       application task.
*
*   INPUTS
*
*       handle                  The handle for which to toggle receive
*                               notifications.
*       enable                  NU_TRUE to enable receive notifications
*                               on the handle.  NU_FALSE to disable
*                               receive notifications on the handle.
*
*   OUTPUTS
*
*       NU_SUCCESS              The PING was successfully scheduled.
*       WSOX_INVALID_HANDLE     The handle is invalid.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS NU_WSOX_Toggle_Recv(UINT32 handle, BOOLEAN enable)
{
    WSOX_CONTEXT_STRUCT *wsox_ptr;
    STATUS              status;
    NU_TASK             *task_ptr = NU_NULL;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Determine if a task is currently holding the WebSocket semaphore. */
    status = NU_Get_Semaphore_Owner(&WSOX_Resource, &task_ptr);

    if (status == NU_SUCCESS)
    {
        /* If the task owner is this task, and the master task is making this
         * call, the semaphore is not required.
         */
        if ( (task_ptr) &&
             (&WSOX_Master_Task_CB == NU_Current_Task_Pointer()) &&
             (&WSOX_Master_Task_CB == task_ptr) &&
             (WSOX_Notify_Enable == NU_TRUE) && (WSOX_Notify_Handle == handle) )
        {
            status = NU_SUCCESS;
        }

        else
        {
            /* Get the WebSocket semaphore. */
            status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

            /* Set task_ptr to NU_NULL to indicate that we have the semaphore. */
            task_ptr = NU_NULL;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Find a matching handle. */
        wsox_ptr = WSOX_Find_Context_By_Handle(handle);

        /* If a matching handle was found. */
        if (wsox_ptr)
        {
            /* Enable receive notifications. */
            if (enable == NU_TRUE)
            {
                wsox_ptr->flag |= WSOX_NOTIFY_RX;
            }

            /* Disable receive notifications. */
            else
            {
                wsox_ptr->flag &= ~WSOX_NOTIFY_RX;
            }

            if (!task_ptr)
            {
                /* Let the server update the list on which it is blocking
                 * for data.
                 */
                WSOX_Resume_Server();
            }
        }

        else
        {
            status = WSOX_INVALID_HANDLE;
        }

        /* Only release the semaphore if we have it. */
        if (!task_ptr)
        {
            NU_Release_Semaphore(&WSOX_Resource);
        }
    }

    else
    {
        status = NU_INVALID_PARM;
    }

    NU_USER_MODE();

    return (status);

} /* NU_WSOX_Toggle_Recv */

/************************************************************************
*
*   FUNCTION
*
*       NU_WSOX_Recv
*
*   DESCRIPTION
*
*       This routine receives data over a WebSocket handle.
*
*       This routine MAY ONLY be invoked from the onmessage() callback
*       routine registered with the respective handle.
*
*   INPUTS
*
*       handle                  The handle for which to receive data.
*       *buffer                 The buffer into which to receive the
*                               data.
*       *data_len               The size of the buffer passed into the
*                               routine on input.  On return, the number
*                               of bytes copied into the buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS              Data was successfully received.
*       NU_INVALID_PARM         An input parameter is NULL.
*       WSOX_INVALID_HANDLE     The handle is invalid.
*       WSOX_DECODE_ERROR       The data is not properly encoded UTF-8
*                               data.  The connection will be closed.
*       WSOX_INVALID_ACCESS     The routine is being called outside the
*                               context of the registered onmessage()
*                               routine.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS NU_WSOX_Recv(UINT32 handle, CHAR *buffer, UINT64 *data_len)
{
    WSOX_CONTEXT_STRUCT *wsox_ptr;
    STATUS              status = NU_SUCCESS;
    NU_TASK             *task_ptr = NU_NULL;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Validate the input pointers. */
    if ( (buffer) && (data_len) )
    {
        /* Determine if a task is currently holding the WebSocket semaphore. */
        status = NU_Get_Semaphore_Owner(&WSOX_Resource, &task_ptr);

        /* This routine can only be called from the context of the onmessage()
         * routine registered with the respective handle.
         */
        if ( (status == NU_SUCCESS) && (task_ptr) &&
             (&WSOX_Master_Task_CB == NU_Current_Task_Pointer()) &&
             (&WSOX_Master_Task_CB == task_ptr) && (WSOX_Notify_Enable == NU_TRUE) &&
             (WSOX_Notify_Handle == handle) )
        {
            /* Find the handle structure. */
            wsox_ptr = WSOX_Find_Context_By_Handle(handle);

            /* If a matching handle was found. */
            if (wsox_ptr)
            {
                /* Fill the buffer with as much data as will fit. */
                status = WSOX_Process_Data(wsox_ptr, buffer, data_len);

                /* If bad data was received, we must fail the WebSocket connection.
                 * Set the connection as down so the master task can handle the
                 * error when control returns to it.
                 */
                if (status != NU_SUCCESS)
                {
                    wsox_ptr->flag &= ~(WSOX_OPEN);
                }
            }

            else
            {
                status = WSOX_INVALID_HANDLE;
            }
        }

        else
        {
            status = WSOX_INVALID_ACCESS;
        }
    }

    else
    {
        status = NU_INVALID_PARM;
    }

    NU_USER_MODE();

    return (status);

} /* NU_WSOX_Recv */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Process_Data
*
*   DESCRIPTION
*
*       This function fills a buffer with data from a WebSocket
*       connection.
*
*   INPUTS
*
*       wsox_ptr                The handle for which to receive data.
*       *buffer                 The buffer into which to receive the
*                               data.
*       *data_len               The size of the buffer passed into the
*                               routine on input.  On return, the number
*                               of bytes copied into the buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS              The specified number of bytes were copied
*                               into the buffer.
*       WSOX_DECODE_ERROR       The text is not proper UTF8 data.
*       WSOX_BUF_TOO_SMALL      The user buffer is too small to decode the
*                               remaining UTF-8 encoded value.
*
*************************************************************************/
STATIC STATUS WSOX_Process_Data(WSOX_CONTEXT_STRUCT *wsox_ptr, CHAR *buffer,
                                UINT64 *data_len)
{
    INT32               bytes_recv;
    UINT64              total_rx_count = 0, buffer_len = *data_len;
    UINT64              bytes_pending, byte;
    STATUS              status = NU_SUCCESS;

    /* If this is a TEXT frame, and there is a partial UTF-8 encoded value
     * in the buffer.
     */
    if ( (wsox_ptr->frame_opcode == WSOX_TEXT_FRAME) && (wsox_ptr->utf8_byte_count) )
    {
        /* If the user buffer is too small for these bytes, the call is going
         * to fail.  It is already known that this is a partial encoded UTF-8
         * value, so the user must pass in a buffer big enough to fit at least
         * this decoded value plus one more byte, which will be a maximum of
         * 4 bytes.
         */
        if (buffer_len > wsox_ptr->utf8_byte_count)
        {
            /* Copy the bytes into the user buffer. */
            memcpy(buffer, wsox_ptr->utf8_bytes, wsox_ptr->utf8_byte_count);
            total_rx_count = wsox_ptr->utf8_byte_count;
        }

        else
        {
            status = WSOX_BUF_TOO_SMALL;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Copy as much data into the buffer as will fit. */
        while (total_rx_count < buffer_len)
        {
            /* If there is data left on the socket. */
            if (wsox_ptr->frame_bytes_left > 0)
            {
                /* If the number of bytes remaining on the socket is greater than
                 * the room in the buffer, set the number of bytes to receive to
                 * the number of bytes remaining in the buffer.
                 */
                if (wsox_ptr->frame_bytes_left > (buffer_len - total_rx_count))
                {
                    bytes_pending = (buffer_len - total_rx_count);
                }

                else
                {
                    bytes_pending = wsox_ptr->frame_bytes_left;
                }

                /* NU_Recv can only receive UINT16 bytes of data at a time. */
                if (bytes_pending > 65535)
                {
                    bytes_pending = 65535;
                }

                bytes_recv = WSOX_Recv(wsox_ptr->socketd, &buffer[total_rx_count],
                                       bytes_pending, wsox_ptr->ssl);

                if (bytes_recv > 0)
                {
                    /* Increment the number of bytes received. */
                    total_rx_count += bytes_recv;

                    /* Decrement the number of bytes remaining. */
                    wsox_ptr->frame_bytes_left -= bytes_recv;
                }

                else
                {
                    /* Only return the error if no data could be received
                     * on the socket.
                     */
                    if (total_rx_count == 0)
                    {
                        status = bytes_recv;
                    }

                    break;
                }
            }

            /* All the data has been received from the socket. */
            else
            {
                break;
            }
        }
    }

    /* If there is data to process. */
    if ( (status == NU_SUCCESS) && (total_rx_count != 0) )
    {
        /* If the data must be unmasked. */
        if (!(wsox_ptr->flag & WSOX_CLIENT))
        {
            /* The partial decoded UTF-8 bytes have already been unmasked. */
            WSOX_Mask_Data(&buffer[wsox_ptr->utf8_byte_count],
                           &buffer[wsox_ptr->utf8_byte_count], wsox_ptr->frame_mask,
                           total_rx_count - wsox_ptr->utf8_byte_count,
                           wsox_ptr->payload_len -
                           (wsox_ptr->frame_bytes_left + (total_rx_count - wsox_ptr->utf8_byte_count)));
        }

        /* If this is a text frame, check the processed data for
         * proper UTF-8 encoding.
         */
        if (wsox_ptr->frame_opcode == WSOX_TEXT_FRAME)
        {
            if (WSOX_Validate_UTF8(buffer, total_rx_count, &byte) == NU_FALSE)
            {
                /* This could have failed, because we have split a UTF-8 encoded
                 * value on a boundary.  If there is more data to be received
                 * from this frame, or this frame is a fragment, and there is
                 * more data to be expected, save the offending bytes to try
                 * to decode with the next receive call.  The offending bytes
                 * must be within 3 bytes of the end of the frame; otherwise,
                 * it's not a partial UTF-8 value - it's just bad data.
                 */
                if ( ((wsox_ptr->frame_bytes_left) ||
                      (wsox_ptr->fragment_opcode == wsox_ptr->frame_opcode)) &&
                     (total_rx_count - byte <= 3) )
                {
                    wsox_ptr->utf8_byte_count = (total_rx_count - byte);

                    /* Copy the partial UTF-8 bytes. */
                    memcpy(wsox_ptr->utf8_bytes, &buffer[byte],
                           wsox_ptr->utf8_byte_count);

                    /* Do not return this data to the caller. */
                    total_rx_count -= wsox_ptr->utf8_byte_count;
                }

                else
                {
                    status = WSOX_DECODE_ERROR;
                }
            }

            else
            {
                wsox_ptr->utf8_byte_count = 0;
            }
        }
    }

    /* Return the number of bytes received. */
    *data_len = total_rx_count;

    return (status);

} /* WSOX_Process_Data */

/************************************************************************
*
*   FUNCTION
*
*       NU_WSOX_Setsockopt
*
*   DESCRIPTION
*
*       This routine invokes NU_Setsockopt for the socket associated
*       with the respective handle.  Please refer to the description of
*       NU_Setsockopt for valid input/output parameters.
*
*   INPUTS
*
*       handle                  The handle for which to set the socket
*                               option.
*       level                   The protocol level.
*       optname                 The option being set.
*       *optval                 Pointer to the new value for the option.
*       optlen                  The size in bytes of the location pointed
*                               to by optval.
*
*   OUTPUTS
*
*       NU_SUCCESS              The option was successfully set.
*       NU_INVALID_PARM         An input parameter is NULL.
*       WSOX_INVALID_HANDLE     The handle is invalid.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS NU_WSOX_Setsockopt(UINT32 handle, INT level, INT optname, VOID *optval,
                          INT optlen)
{
    WSOX_CONTEXT_STRUCT *wsox_ptr;
    STATUS              status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Get the WebSocket semaphore. */
    status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Find the handle structure. */
        wsox_ptr = WSOX_Find_Context_By_Handle(handle);

        /* If a matching handle was found. */
        if (wsox_ptr)
        {
            /* If this is a listener handle, the user wants to modify
             * the socket that is accepting new WebSocket connections.
             * This routine should only be called for a listener handle
             * if the system is using HTTP Lite to accept WebSocket
             * connections; otherwise, the application has direct access
             * to the server socket and can call NU_Setsockopt directly.
             */
            if (wsox_ptr->flag & WSOX_LISTENER)
            {
#if (CFG_NU_OS_NET_HTTP_SERVER_ENABLE == NU_TRUE)
                status = NU_Obtain_Semaphore(&HTTP_Lite_Resource, NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Ensure the HTTP Server has been successfully initialized. */
                    if (HTTP_Session)
                    {
                        /* Set the option over the unsecure socket. */
                        status = NU_Setsockopt(HTTP_Session->http_listener, level,
                                               optname, optval, optlen);

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
                        /* If this connection is secure, set the option over
                         * the secure socket.
                         */
                        if ( (status == NU_SUCCESS) &&
                             (wsox_ptr->flag & NU_WSOX_SECURE) )
                        {
                            status = NU_Setsockopt(HTTP_Session->ssl_listener, level,
                                                   optname, optval, optlen);
                        }
#endif
                    }

                    else
                    {
                        status = WSOX_INVALID_HANDLE;
                    }

                    NU_Release_Semaphore(&HTTP_Lite_Resource);
                }
#else
                status = WSOX_INVALID_HANDLE;
#endif
            }

            else
            {
                /* Invoke the socket option. */
                status = NU_Setsockopt(wsox_ptr->socketd, level, optname,
                                       optval, optlen);
            }
        }

        else
        {
            status = WSOX_INVALID_HANDLE;
        }

        NU_Release_Semaphore(&WSOX_Resource);
    }

    NU_USER_MODE();

    return (status);

} /* NU_WSOX_Setsockopt */

/************************************************************************
*
*   FUNCTION
*
*       NU_WSOX_Getsockopt
*
*   DESCRIPTION
*
*       This routine invokes NU_Getsockopt for the socket associated
*       with the respective handle.  Please refer to the description of
*       NU_Getsockopt for valid input/output parameters.
*
*   INPUTS
*
*       handle                  The handle for which to get the socket
*                               option.
*       level                   The protocol level.
*       optname                 The option being set.
*       *optval                 Pointer to the location where the option
*                               status can be written.
*       *optlen                 The size in bytes of the location pointed
*                               to by optval.
*
*   OUTPUTS
*
*       NU_SUCCESS              The option was successfully retrieved.
*       NU_INVALID_PARM         An input parameter is NULL.
*       WSOX_INVALID_HANDLE     The handle is invalid.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS NU_WSOX_Getsockopt(UINT32 handle, INT level, INT optname, VOID *optval,
                          INT *optlen)
{
    WSOX_CONTEXT_STRUCT *wsox_ptr;
    STATUS              status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Get the WebSocket semaphore. */
    status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Find the handle structure. */
        wsox_ptr = WSOX_Find_Context_By_Handle(handle);

        /* If a matching handle was found. */
        if (wsox_ptr)
        {
            /* If this is a listener handle, the user wants to get the
             * option for the socket that is accepting new WebSocket
             * connections. This routine should only be called for a
             * listener handle if the system is using HTTP Lite to accept
             * WebSocket connections; otherwise, the application has direct
             * access to the server socket and can call NU_Getsockopt
             * directly.
             */
            if (wsox_ptr->flag & WSOX_LISTENER)
            {
#if (CFG_NU_OS_NET_HTTP_SERVER_ENABLE == NU_TRUE)
                status = NU_Obtain_Semaphore(&HTTP_Lite_Resource, NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Ensure the HTTP Server has been successfully initialized. */
                    if (HTTP_Session)
                    {
                        /* Get the option over the unsecure socket. */
                        status = NU_Getsockopt(HTTP_Session->http_listener, level,
                                               optname, optval, optlen);

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
                        /* If this connection is secure, get the option over
                         * the secure socket.
                         */
                        if ( (status == NU_SUCCESS) &&
                             (wsox_ptr->flag & NU_WSOX_SECURE) )
                        {
                            status = NU_Getsockopt(HTTP_Session->ssl_listener, level,
                                                   optname, optval, optlen);
                        }
#endif
                    }

                    else
                    {
                        status = WSOX_INVALID_HANDLE;
                    }

                    NU_Release_Semaphore(&HTTP_Lite_Resource);
                }
#else
                status = WSOX_INVALID_HANDLE;
#endif
            }

            else
            {
                /* Invoke the socket option. */
                status = NU_Getsockopt(wsox_ptr->socketd, level, optname,
                                       optval, optlen);
            }
        }

        else
        {
            status = WSOX_INVALID_HANDLE;
        }

        NU_Release_Semaphore(&WSOX_Resource);
    }

    NU_USER_MODE();

    return (status);

} /* NU_WSOX_Getsockopt */
