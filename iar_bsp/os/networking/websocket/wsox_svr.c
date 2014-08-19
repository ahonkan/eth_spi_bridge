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
*       wsox_svr.c
*
*   COMPONENT
*
*       Nucleus WebSocket
*
*   DESCRIPTION
*
*       This file holds the Nucleus WebSocket routines specific to the
*       server component.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       WSOX_Resume_Server
*       WSOX_Master_Task
*       WSOX_Setup_Handles
*       WSOX_Get_Opcode
*       WSOX_Process_Incoming_Frame
*       WSOX_Recv_Notify
*       WSOX_Parse_Header
*       WSOX_Process_Close
*       WSOX_Process_Connection_Request
*       WSOX_Create_Server_Handle
*
*   DEPENDENCIES
*
*       nu_networking.h
*       wsox_int.h
*
************************************************************************/

#include "networking/nu_networking.h"
#include "os/networking/websocket/wsox_int.h"

extern NU_SEMAPHORE         WSOX_Resource;
extern WSOX_CONTEXT_LIST    WSOX_Listener_List;

WSOX_CONTEXT_LIST   WSOX_Connection_List;
BOOLEAN             WSOX_Server_State = NU_FALSE;
BOOLEAN             WSOX_Notify_Enable = NU_FALSE;
UINT32              WSOX_Notify_Handle;
NU_TASK             WSOX_Master_Task_CB;
VOID                *WSOX_Master_Task_Memory;
INT                 WSOX_Socketd;
BOOLEAN             WSOX_Socket_Error;

STATIC STATUS WSOX_Process_Incoming_Frame(WSOX_CONTEXT_STRUCT *wsox_ptr);
STATIC STATUS WSOX_Get_Opcode(WSOX_CONTEXT_STRUCT *wsox_ptr, INT *opcode, UINT64 data_len);
STATIC STATUS WSOX_Process_Close(WSOX_CONTEXT_STRUCT *wsox_ptr, UINT64 data_len);
STATIC VOID WSOX_Setup_Handles(INT *highest_socket, UINT32 *timeout_value,
                               FD_SET *readfs);
STATIC STATUS WSOX_Parse_Header(WSOX_CONTEXT_STRUCT *wsox_ptr);
STATIC VOID WSOX_Recv_Notify(WSOX_CONTEXT_STRUCT *wsox_ptr, WSOX_MSG_INFO *msg_info);

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Resume_Server
*
*   DESCRIPTION
*
*       This routine resumes the WebSocket master task.  If it is not
*       already running, it will be created and invoked.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS              The server was started.
*
*       Otherwise, an operating-system specific error code is returned
*       indicating that the server could not be started.
*
*************************************************************************/
STATUS WSOX_Resume_Server(VOID)
{
    NU_MEMORY_POOL      *memory_ptr;
    STATUS              status = NU_SUCCESS;
    struct sock_struct  *sockptr;

    /* If the server is not already running. */
    if (WSOX_Server_State == NU_FALSE)
    {
        /* Initialize the server's connection list. */
        WSOX_Connection_List.head = NU_NULL;
        WSOX_Connection_List.tail = NU_NULL;

        WSOX_Socket_Error = NU_FALSE;

        /* Create the control socket used by the Master Task. */
        WSOX_Socketd = NU_Socket(NU_FAMILY_IP, NU_TYPE_DGRAM, 0);

        /* If the socket was created. */
        if (WSOX_Socketd >= 0)
        {
            status = NU_SUCCESS;
        }

        else
        {
            status = WSOX_Socketd;
        }

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the system memory pool. */
            status = NU_System_Memory_Get(&memory_ptr, NU_NULL);

            if (status == NU_SUCCESS)
            {
                /* Allocate memory for the receiving task. */
                status = NU_Allocate_Memory(memory_ptr, &WSOX_Master_Task_Memory,
                                            WSOX_SVR_STACK_SIZE, NU_NO_SUSPEND);
            }

            if (status == NU_SUCCESS)
            {
                /* Create the receive task. */
                status = NU_Create_Task(&WSOX_Master_Task_CB, "WSOXSrv",
                                        WSOX_Master_Task, 0, NU_NULL,
                                        WSOX_Master_Task_Memory, WSOX_SVR_STACK_SIZE,
                                        WSOX_SVR_PRIORITY, WSOX_SVR_TIME_SLICE,
                                        WSOX_SVR_PREEMPT, NU_START);
            }

            if (status == NU_SUCCESS)
            {
                WSOX_Server_State = NU_TRUE;
            }
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Get the TCP semaphore since we are about to manipulate the
         * internal socket structure.
         */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the socket list entry. */
            sockptr = SCK_Sockets[WSOX_Socketd];

            /* Resume the thread that is suspend on select. */
            SCK_Set_Socket_Error(sockptr, NU_NO_DATA_TRANSFER);

            /* Indicate that there is an error on the control socket. */
            WSOX_Socket_Error = NU_TRUE;

            NU_Release_Semaphore(&TCP_Resource);
        }
    }

    return (status);

} /* WSOX_Resume_Server */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Master_Task
*
*   DESCRIPTION
*
*       The WebSocket Master Task services established connections.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID WSOX_Master_Task(UNSIGNED argc, VOID *argv)
{
    STATUS              status;
    WSOX_CONTEXT_STRUCT *wsox_ptr, *next_ptr;
    INT                 socketd;
    FD_SET              readfs;
    UNSIGNED            timeout = NU_SUSPEND;

    for (;;)
    {
        /* Get the WebSocket semaphore. */
        status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Always wait for data on the control socket. */
            socketd = WSOX_Socketd;

            /* Initialize the timeout for the select call. */
            timeout = NU_SUSPEND;

            /* Get the timeout and highest socket values for the next
             * call to NU_Select, and set up the existing connections
             * to receive data.
             */
            WSOX_Setup_Handles(&socketd, &timeout, &readfs);

            /* Release the semaphore. */
            (VOID)NU_Release_Semaphore(&WSOX_Resource);

            /* Wait for data on any of the sockets. */
            status = NU_Select(socketd + 1, &readfs, NU_NULL, NU_NULL,
                               timeout);
        }

        /* If we can't get the semaphore, there is something very wrong
         * with the system.  The Master Task should abort.
         */
        else
        {
            break;
        }

        if (status == NU_SUCCESS)
        {
            /* Get the WebSocket semaphore. */
            status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* If there is an error on the control socket, clear it. */
                if (WSOX_Socket_Error == NU_TRUE)
                {
                    SCK_Clear_Socket_Error(WSOX_Socketd);
                    WSOX_Socket_Error = NU_FALSE;
                }

                wsox_ptr = WSOX_Connection_List.head;

                /* Find which socket has data. */
                while (wsox_ptr)
                {
                    /* Get the next pointer in case this one gets deallocated. */
                    next_ptr = wsox_ptr->flink;

                    /* Check this socket for data. */
                    if ( (wsox_ptr->flag & WSOX_NOTIFY_RX) &&
                         (NU_FD_Check(wsox_ptr->socketd, &readfs) == NU_TRUE) )
                    {
                        /* Process the data on the socket. */
                        status = WSOX_Process_Incoming_Frame(wsox_ptr);

                        /* If an error occurred to cause the WebSocket connection
                         * to fail, or the connection has been closed, close the socket.
                         */
                        if ( (status != NU_SUCCESS) ||
                             ((!(wsox_ptr->flag & WSOX_TX)) && (!(wsox_ptr->flag & WSOX_RX))) )
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
                            if ( (status != NU_SUCCESS) || (!(wsox_ptr->flag & WSOX_CLIENT)) )
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
                        }
                    }

                    /* Check the next handle for data. */
                    wsox_ptr = next_ptr;
                }

                /* Release the semaphore. */
                (VOID)NU_Release_Semaphore(&WSOX_Resource);
            }

            /* If we can't get the semaphore, there is something very wrong
             * with the system.  WebSocket should abort.
             */
            else
            {
                break;
            }
        }
    }

} /* WSOX_Master_Task */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Setup_Handles
*
*   DESCRIPTION
*
*       This function traverses the connection list to find the socket
*       and timeout values to pass into NU_Select and also to set up
*       the read bitmap used with NU_Select.  The routine will also
*       handle cleaning up any client handles that have timed out
*       waiting for the server to close the connection.
*
*   INPUTS
*
*       *highest_socket         The socket value to pass into
*                               NU_Select in order to suspend on all
*                               open connections.
*       *timeout_value          The timeout value to pass into
*                               NU_Select.
*       *readfs                 The read bitmap to pass into
*                               NU_Select.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID WSOX_Setup_Handles(INT *highest_socket, UINT32 *timeout_value,
                               FD_SET *readfs)
{
    WSOX_CONTEXT_STRUCT     *wsox_ptr, *next_ptr;
    struct sockaddr_struct  peer;
    struct addr_struct      foreign_addr;
    STATUS                  status;
    UINT64                  data_len;
    UINT32                  timeout = *timeout_value;
    INT                     socketd = *highest_socket;
    INT16                   addr_length;
    CHAR                    mask[WSOX_MASK_LEN];

    /* Initialize the bitmap */
    NU_FD_Init(readfs);

    /* Set the read bit for the Master Task control socket. */
    NU_FD_Set(WSOX_Socketd, readfs);

    /* Traverse the list. */
    wsox_ptr = WSOX_Connection_List.head;

    /* Find the highest value listening socket and find the timeout
     * for the NU_Select call based on any handles waiting to timeout
     * the connection or waiting to transmit a periodic PING.
     */
    while (wsox_ptr)
    {
        /* Save the next handle. */
        next_ptr = wsox_ptr->flink;

        /* If this is a new connection, notify the application layer. */
        if (wsox_ptr->flag & WSOX_NEW_CNXN)
        {
            /* Remove the flag. */
            wsox_ptr->flag &= ~WSOX_NEW_CNXN;

            /* Set the length of the address structure. */
            addr_length = sizeof(struct sockaddr_struct);

            /* Get the information associated with the foreign peer. */
            status = NU_Get_Peer_Name(wsox_ptr->socketd, &peer, &addr_length);

            /* This call should never fail. */
            if (status == NU_SUCCESS)
            {
                /* Set up the address structure to pass to the application. */
                foreign_addr.port = peer.port_num;
                foreign_addr.name = NU_NULL;
                foreign_addr.family = peer.family;

#if (INCLUDE_IPV6 == NU_TRUE)
                /* If the family is IPv4, and this is an IPv4-mapped IPv6 address. */
                if ( (foreign_addr.family == NU_FAMILY_IP) &&
                     (IPV6_IS_ADDR_V4MAPPED(peer.ip_num.is_ip_addrs)) )
                {
                    /* Extract the IPv4 address from the structure. */
                    memcpy(foreign_addr.id.is_ip_addrs, &peer.ip_num.is_ip_addrs[12],
                           IP_ADDR_LEN);
                }

                else
                {
                    memcpy(foreign_addr.id.is_ip_addrs, peer.ip_num.is_ip_addrs,
                           MAX_ADDRESS_SIZE);
                }
#else
                memcpy(foreign_addr.id.is_ip_addrs, peer.ip_num.is_ip_addrs,
                       MAX_ADDRESS_SIZE);
#endif

                /* Invoke the callback routine. */
                wsox_ptr->onopen(wsox_ptr->user_handle, &foreign_addr);
            }
        }

        /* If this handle has a timer running. */
        if (wsox_ptr->timestamp)
        {
            /* Check if the handle's timer is expired. */
            if (TQ_Check_Duetime(wsox_ptr->timestamp) == NU_NO_SUSPEND)
            {
                /* If the handle is scheduled to send periodic PINGs. */
                if (wsox_ptr->flag & WSOX_PING)
                {
                    /* If the handle is not in retransmission mode. */
                    if (wsox_ptr->retrans_count == 0)
                    {
                        /* If no PONG has been received within the PING
                         * interval rate, begin sending PINGs at the
                         * retransmission interval set by the application
                         * at the call to NU_WSOX_Schedule_PING().
                         */
                        if (TQ_Check_Duetime(wsox_ptr->pong_timestamp +
                                             wsox_ptr->ping_interval) == NU_NO_SUSPEND)
                        {
                            wsox_ptr->retrans_count ++;
                        }
                    }

                    /* If no PONG has been received within the retransmission
                     * interval rate.
                     */
                    else if (TQ_Check_Duetime(wsox_ptr->pong_timestamp +
                                              wsox_ptr->retrans_interval) == NU_NO_SUSPEND)
                    {
                        wsox_ptr->retrans_count ++;
                    }

                    /* The other side has answered the retransmission. */
                    else
                    {
                        wsox_ptr->retrans_count = 0;
                    }

                    /* If the handle is in retransmission mode. */
                    if (wsox_ptr->retrans_count)
                    {
                        /* If the max number of retransmissions has not been sent. */
                        if (wsox_ptr->retrans_count <= wsox_ptr->max_retrans_count)
                        {
                            /* Set the next timestamp to the retransmission interval. */
                            wsox_ptr->timestamp =
                                NU_Retrieve_Clock() + wsox_ptr->retrans_interval;
                        }

                        /* The other side of the connection has not answered the
                         * PING frames.
                         */
                        else
                        {
                            /* Stop sending PING frames. */
                            wsox_ptr->flag &= ~WSOX_PING;

                            /* Set the timestamp to zero so this handle does not affect
                             * the tasks's timeout.
                             */
                            wsox_ptr->timestamp = 0;

                            /* Notify the application. */
                            wsox_ptr->onerror(wsox_ptr->user_handle, WSOX_PING_TIMED_OUT,
                                              "Foreign side has not answered PINGs");
                        }
                    }

                    else
                    {
                        /* Set the next timestamp to the ping interval. */
                        wsox_ptr->timestamp =
                            NU_Retrieve_Clock() + wsox_ptr->ping_interval;
                    }

                    /* Send the PING frame. */
                    if (wsox_ptr->retrans_count <= wsox_ptr->max_retrans_count)
                    {
                        data_len = strlen(WSOX_PING_MESSAGE);

                        WSOX_Send_Frame(wsox_ptr, WSOX_PING_MESSAGE,
                                        &data_len, WSOX_PING_FRAME, 0, 0, mask);
                    }
                }

                /* Otherwise, the timer was set to close the
                 * connection.
                 */
                else
                {
                    /* Clean up the connection entry. */
                    WSOX_Cleanup_Connection_Entry(wsox_ptr);

                    wsox_ptr = NU_NULL;
                }
            }

            /* If the client's timeout is less than the currently set
             * timeout, set the timeout to this client's wait time.
             */
            if ( (wsox_ptr) && (wsox_ptr->timestamp) &&
                 (TQ_Check_Duetime(wsox_ptr->timestamp) < timeout) )
            {
                timeout = TQ_Check_Duetime(wsox_ptr->timestamp);
            }
        }

        /* If this entry wasn't timed out above, and the application has not
         * disabled receive notifications on this handle.
         */
        if ( (wsox_ptr) && (wsox_ptr->flag & WSOX_NOTIFY_RX) )
        {
            /* Set the read bit for each socket. */
            NU_FD_Set(wsox_ptr->socketd, readfs);

            /* If this is the highest socket found so far. */
            if (wsox_ptr->socketd > socketd)
            {
                socketd = wsox_ptr->socketd;
            }
        }

        wsox_ptr = next_ptr;
    }

    /* Return the modified values if they have changed. */
    *highest_socket = socketd;
    *timeout_value = timeout;

} /* WSOX_Setup_Handles */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Get_Opcode
*
*   DESCRIPTION
*
*       This function determines the opcode of the current frame that
*       is being processed and validates it.
*
*   INPUTS
*
*       *wsox_ptr               A pointer to the handle structure
*                               associated with the current connection.
*       *opcode                 A pointer that will be filled in with
*                               the opcode upon successful completion
*                               of the routine.
*       data_len                The length of the frame data.
*
*   OUTPUTS
*
*       WSOX_NORMAL             No error was encountered.
*       WSOX_PROTO_ERROR        The client has violated the rules of
*                               RFC 6455 regarding sending fragmented
*                               frames.
*       WSOX_UNRECOGNIZED_DATA  The server does not recognize the opcode.
*
*************************************************************************/
STATIC STATUS WSOX_Get_Opcode(WSOX_CONTEXT_STRUCT *wsox_ptr, INT *opcode,
                              UINT64 data_len)
{
    INT     type;
    STATUS  status = WSOX_NORMAL;

    /* Get the frame type. */
    type = (wsox_ptr->header[WSOX_FIN_RSV_OPCODE_OFFSET] & 0xf);

    /* If the final fragment bit is not set, this packet is a
     * fragment.
     */
    if (!(wsox_ptr->header[WSOX_FIN_RSV_OPCODE_OFFSET] & 0x80))
    {
        /* If the opcode is not zero, this is the first fragment. */
        if (type != WSOX_CONT_FRAME)
        {
            /* RFC 6455 section 5.4 - The fragments of one message
             * MUST NOT be interleaved between the fragments of another
             * message unless an extension has been negotiated that can
             * interpret the interleaving.
             */
            if (wsox_ptr->fragment_opcode == 0)
            {
                /* Save this opcode type for the next fragments that are
                 * received, since they will not contain an opcode.
                 */
                wsox_ptr->fragment_opcode = type;
            }

            /* Another first fragment has already been received.  The
             * client is sending data contrary to the RFC or data that
             * we cannot understand due to a missing extension.
             */
            else
            {
                status = WSOX_PROTO_ERROR;
            }
        }

        /* This is a middle fragment.  Set the opcode according to
         * the first fragment.
         */
        else
        {
            type = wsox_ptr->fragment_opcode;
        }
    }

    /* If this is the last fragment, set the opcode according to
     * the first fragment.
     */
    else if (type == WSOX_CONT_FRAME)
    {
        type = wsox_ptr->fragment_opcode;

        /* Reset the handle's fragmentation opcode variable so another
         * fragment can be received from a different frame.
         */
        wsox_ptr->fragment_opcode = 0;
    }

    /* If we are currently processing a fragment, and this frame is
     * not part of that fragment, but is not a control frame, the
     * other side has sent a bad frame.
     */
    else if ( (wsox_ptr->fragment_opcode != 0) && ((!(type & 0x8))) )
    {
        status = WSOX_PROTO_ERROR;
    }

    if (status == WSOX_NORMAL)
    {
        /* If the frame type is not recognized, fail the connection. */
        switch (type)
        {
        /* At this time, we only support data and close frames. */
        case WSOX_TEXT_FRAME:
        case WSOX_BINARY_FRAME:
            break;

        case WSOX_CLOSE_FRAME:
        case WSOX_PONG_FRAME:
        case WSOX_PING_FRAME:

            /* Control frames may not be fragmented, and the payload may not
             * exceed 125 octets.
             */
            if ( (!(wsox_ptr->header[WSOX_FIN_RSV_OPCODE_OFFSET] & 0x80)) ||
                 (data_len > WSOX_MAX_CTRL_FRAME_LEN) )
            {
                status = WSOX_PROTO_ERROR;
            }

            break;

        default:

            status = WSOX_PROTO_ERROR;
            break;
        }

        if (status == WSOX_NORMAL)
        {
            /* Return the opcode. */
            *opcode = type;
        }
    }

    return (status);

} /* WSOX_Get_Opcode */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Parse_Header
*
*   DESCRIPTION
*
*       This function reads the header from the socket and stores it in
*       the WebSocket handle structure.  The routine will block on
*       receive until the entire header is read.
*
*   INPUTS
*
*       *wsox_ptr               A pointer to the handle structure
*                               associated with the connection over
*                               which the data was received.
*
*   OUTPUTS
*
*       NU_SUCCESS              The entire header was read from the
*                               socket.
*       NU_NO_DATA_TRANSFER     The socket has been closed.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATIC STATUS WSOX_Parse_Header(WSOX_CONTEXT_STRUCT *wsox_ptr)
{
    INT32               bytes_rx;
    UINT16              socket_count;
    STATUS              status = NU_SUCCESS;
    UINT8               ext_payload_len = 0, cur_field_len;

    /* Determine the number of bytes on the socket. */
    socket_count = WSOX_Check_For_Data(wsox_ptr->socketd, wsox_ptr->ssl);

    /* If there is data on the socket. */
    if (socket_count != 0)
    {
        /* Set the socket to blocking so the known data can be retrieved. */
        NU_Fcntl(wsox_ptr->socketd, NU_SETFLAG, NU_BLOCK);

        /* Initialize the flag to indicate that a partial header was
         * received.
         */
        wsox_ptr->flag |= WSOX_PARTIAL_HDR;

        /* If the fin/rsv/opcode field is not already received. */
        if (wsox_ptr->header_len < 1)
        {
            /* Receive the data. */
            bytes_rx = WSOX_Recv(wsox_ptr->socketd, &wsox_ptr->header[wsox_ptr->header_len],
                                 1, wsox_ptr->ssl);

            if (bytes_rx > 0)
            {
                wsox_ptr->header_len += bytes_rx;
                socket_count -= bytes_rx;
            }

            else
            {
                status = bytes_rx;
            }
        }

        /* If there is more data available, and the payload length field
         * has not already been parsed from the packet.
         */
        if ( (status == NU_SUCCESS) && (socket_count) &&
             (wsox_ptr->header_len < 2) )
        {
            bytes_rx = WSOX_Recv(wsox_ptr->socketd, &wsox_ptr->header[wsox_ptr->header_len],
                                 1, wsox_ptr->ssl);

            if (bytes_rx > 0)
            {
                wsox_ptr->header_len += bytes_rx;
                socket_count -= bytes_rx;
            }

            else
            {
                status = bytes_rx;
            }
        }

        /* If the payload length field has been received. */
        if ( (status == NU_SUCCESS) && (wsox_ptr->header_len >= 2) )
        {
            /* RFC 6455 section 5.2 - ... if 0-125, that is the payload length.
             * If 126, the following 2 bytes interpreted as a 16-bit unsigned
             * integer are the payload length.  If 127, the following 8 bytes
             * interpreted as a 64-bit unsigned integer.
             */
            wsox_ptr->payload_len =
                (wsox_ptr->header[WSOX_MSK_LEN_OFFSET] & 0x7f);

            if (wsox_ptr->payload_len == 126)
            {
                /* The length field requires 2 additional bytes. */
                ext_payload_len += WSOX_16BIT_PAYLOAD_LEN;
            }

            else if (wsox_ptr->payload_len == 127)
            {
                /* The length field requires 4 additional bytes. */
                ext_payload_len += WSOX_64BIT_PAYLOAD_LEN;
            }

            /* If there are extra bytes to copy and there is data remaining on
             * the socket.
             */
            if ( (socket_count) && (ext_payload_len) )
            {
                /* If the entire extended payload length field is not already
                 * copied.
                 */
                if (wsox_ptr->header_len < (ext_payload_len + 2))
                {
                    /* Adjust the number of bytes to be copied for the extended
                     * payload length if some bytes have already been copied.
                     * Preserve ext_payload_len for a later comparison.
                     */
                    cur_field_len = ext_payload_len - (wsox_ptr->header_len - 2);

                    bytes_rx = WSOX_Recv(wsox_ptr->socketd, &wsox_ptr->header[wsox_ptr->header_len],
                                         cur_field_len, wsox_ptr->ssl);

                    if (bytes_rx > 0)
                    {
                        wsox_ptr->header_len += bytes_rx;
                        socket_count -= bytes_rx;
                    }

                    else
                    {
                        status = bytes_rx;
                    }
                }

                /* If the extended payload length field was parsed from the packet,
                 * get the length of the payload.
                 */
                if ( (status == NU_SUCCESS) &&
                     (wsox_ptr->header_len >= (ext_payload_len + 2)) )
                {
                    /* Store the payload length. */
                    if (wsox_ptr->payload_len == 126)
                    {
                        wsox_ptr->payload_len =
                            GET16(wsox_ptr->header, WSOX_EXT_MSK_LEN_OFFSET);
                    }

                    else if (wsox_ptr->payload_len == 127)
                    {
                        wsox_ptr->payload_len =
                            GET64(wsox_ptr->header, WSOX_EXT_MSK_LEN_OFFSET);
                    }
                }
            }

            /* If all bytes leading up to the mask have been received. */
            if ( (status == NU_SUCCESS) &&
                 (wsox_ptr->header_len >= (2 + ext_payload_len)) )
            {
                /* If there is a mask in the header. */
                if (wsox_ptr->header[WSOX_MSK_LEN_OFFSET] & 0x80)
                {
                    /* If there is more data to be parsed from the buffer. */
                    if (socket_count)
                    {
                        /* Adjust the number of bytes to be copied for the mask if some
                         * bytes have already been copied.
                         */
                        cur_field_len =
                            WSOX_MASK_LEN - ((wsox_ptr->header_len - (2 + ext_payload_len)));

                        /* If the mask field has not been fully copied. */
                        if (cur_field_len > 0)
                        {
                            bytes_rx = WSOX_Recv(wsox_ptr->socketd,
                                                 &wsox_ptr->header[wsox_ptr->header_len],
                                                 cur_field_len, wsox_ptr->ssl);

                            if (bytes_rx > 0)
                            {
                                wsox_ptr->header_len += bytes_rx;
                                socket_count -= bytes_rx;
                            }

                            else
                            {
                                status = bytes_rx;
                            }

                            /* If the entire mask has been parsed. */
                            if (bytes_rx == cur_field_len)
                            {
                                /* Copy the mask for quick access while processing this
                                 * frame and for use if this frame is fragmented or data
                                 * is received across multiple receive calls.
                                 */
                                memcpy(wsox_ptr->frame_mask,
                                       &wsox_ptr->header[wsox_ptr->header_len - WSOX_MASK_LEN],
                                       WSOX_MASK_LEN);

                                /* Remove the partial header flag. */
                                wsox_ptr->flag &= ~WSOX_PARTIAL_HDR;

                                wsox_ptr->header_len = 0;
                            }
                        }
                    }
                }

                /* The entire header has been received. */
                else
                {
                    /* Remove the partial header flag. */
                    wsox_ptr->flag &= ~WSOX_PARTIAL_HDR;

                    wsox_ptr->header_len = 0;
                }
            }
        }

        /* Set the socket back to non-blocking. */
        NU_Fcntl(wsox_ptr->socketd, NU_SETFLAG, NU_NO_BLOCK);
    }

    /* Check if the connection is still open. */
    else if (NU_Is_Connected(wsox_ptr->socketd) != NU_TRUE)
    {
        status = NU_NOT_CONNECTED;
    }

    return (status);

} /* WSOX_Parse_Header */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Process_Incoming_Frame
*
*   DESCRIPTION
*
*       This function parses an incoming buffer of data, unmasks the
*       frame(s), and handles the data according to RFC 6455, notifying
*       the application as necessary.  Note that multiple frames could
*       be present in one buffer of data due to TCP streaming.
*
*   INPUTS
*
*       *wsox_ptr               A pointer to the handle structure
*                               associated with the connection over
*                               which the data was received.
*
*   OUTPUTS
*
*       NU_SUCCESS              The connection has been successfully
*                               established.
*       WSOX_PROTO_ERROR        The frame is masked when it should not
*                               be, not masked when it should be, or
*                               there is an invalid value in the header.
*       WSOX_UNRECOGNIZED_DATA  The opcode is not recognized.
*       WSOX_PARSE_ERROR        There was an error found while parsing
*                               the data.  The connection should be
*                               closed by the caller.
*
*       Otherwise, an operating-system specific error is returned
*       indicating an internal error, in which case the connection
*       should remain open.
*
*************************************************************************/
STATIC STATUS WSOX_Process_Incoming_Frame(WSOX_CONTEXT_STRUCT *wsox_ptr)
{
    STATUS              status = WSOX_NORMAL; /* Use to send a close msg. */
    STATUS              return_status = NU_SUCCESS;
    WSOX_MSG_INFO       msg_info;

    /* If this handle is still open for receiving data. */
    if (wsox_ptr->flag & WSOX_RX)
    {
        /* Initialize flags.  The other parameters will always be filled
         * in below.
         */
        msg_info.flags = 0;

        /* If there is no data remaining on the handle for the
         * previous frame, read this frame's header.
         */
        if (wsox_ptr->frame_bytes_left == 0)
        {
            /* Parse the header only if we are not in the process of receiving
             * a close frame.
             */
            if (!(wsox_ptr->flag & WSOX_PARTIAL_CLOSE))
            {
                /* Read the header into the handle's header buffer.  This routine will
                 * only return an error if the underlying connection is closed.
                 */
                return_status = WSOX_Parse_Header(wsox_ptr);
            }

            /* If the entire header has been received. */
            if ( (return_status == NU_SUCCESS) && (!(wsox_ptr->flag & WSOX_PARTIAL_HDR)) )
            {
                msg_info.data_len = wsox_ptr->payload_len;

                /* If the data is from a client, the frame must be masked, unless there is
                 * no data in the frame.  If the data is from a server, the frame must not
                 * be masked.  All reserved bits must be set to zero.
                 */
                if ( (((wsox_ptr->flag & WSOX_CLIENT) &&
                       (!(wsox_ptr->header[WSOX_MSK_LEN_OFFSET] & 0x80))) ||
                      (!(wsox_ptr->flag & WSOX_CLIENT) &&
                       ((wsox_ptr->header[WSOX_MSK_LEN_OFFSET] & 0x80) ||
                        (msg_info.data_len == 0))))&&
                     (!(wsox_ptr->header[WSOX_FIN_RSV_OPCODE_OFFSET] & 0x70)))
                {
                    /* RFC 6455 - section 5.2 - If an unknown opcode is received,
                     * the receiving endpoint MUST _Fail the WebSocket Connection_.
                     */
                    status = WSOX_Get_Opcode(wsox_ptr, &msg_info.opcode,
                                             msg_info.data_len);

                    if (status == WSOX_NORMAL)
                    {
                        /* If this is a close frame. */
                        if (msg_info.opcode == WSOX_CLOSE_FRAME)
                        {
                            /* Process the close frame. */
                            status = WSOX_Process_Close(wsox_ptr, msg_info.data_len);
                        }

                        else
                        {
                            /* If this is a PONG frame. */
                            if (msg_info.opcode == WSOX_PONG_FRAME)
                            {
                                /* If the application has scheduled periodic PING
                                 * frames to be transmitted.
                                 */
                                if (wsox_ptr->flag & WSOX_PING)
                                {
                                    /* Record the time at which the PONG was received.
                                     * Add an additional tick to the timestamp in case
                                     * the PONG was received within one tick of the PING
                                     * being sent, which would cause the retransmission
                                     * timer to start.
                                     */
                                    wsox_ptr->pong_timestamp = NU_Retrieve_Clock() + 1;
                                }
                            }

                            /* Store the opcode in the handle. */
                            wsox_ptr->frame_opcode = msg_info.opcode;

                            /* Set the number of bytes remaining to be read from the
                             * socket.
                             */
                            wsox_ptr->frame_bytes_left = msg_info.data_len;

                            /* If there is no data in this packet, or there is at least
                             * some data on the socket, notify the application.  Otherwise,
                             * there is no need to notify the application since there is no
                             * data yet to be received.
                             */
                            if ( (msg_info.data_len == 0) ||
                                 (WSOX_Check_For_Data(wsox_ptr->socketd, wsox_ptr->ssl)) )
                            {
                                /* Inform the application of the pending frame. */
                                WSOX_Recv_Notify(wsox_ptr, &msg_info);
                            }
                        }
                    }
                }

                /* RFC 6455 - section 5.1 - The server MUST close the connection
                 * upon receiving a frame that is not masked.  In this case, a
                 * server MAY send a Close frame with a status code of 1002
                 * (protocol error) ...  A server MUST NOT mask any frames that
                 * it sends to the client.  A client MUST close a connection if
                 * it detects a masked frame.  In this case, it MAY use the status
                 * code 1002 (protocol error) ...
                 * RFC 6455 - section 5.2 - If a nonzero value is received and
                 * none of the negotiated extensions defines the meaning of such
                 * a nonzero value, the receiving endpoint MUST _Fail the WebSocket
                 * Connection_.
                 */
                else
                {
                    status = WSOX_PROTO_ERROR;
                }

                /* If an error was encountered that requires the server to fail
                 * the WebSocket connection per RFC 6455.
                 */
                if (status != WSOX_NORMAL)
                {
                    /* RFC 6455 - section 7.1.7 - Certain algorithms and specifications
                     * require an endpoint to _Fail the WebSocket Connection_.  To do so,
                     * the client MUST _Close the WebSocket Connection_, and MAY report
                     * the problem to the user (which would be especially useful for
                     * developers) in an appropriate manner. Similarly, to do so, the
                     * server MUST _Close the WebSocket Connection_, and SHOULD log the
                     * problem.
                     */
                    (VOID)WSOX_TX_Close(wsox_ptr->socketd, wsox_ptr->flag & WSOX_CLIENT,
                                        status, NU_NULL, wsox_ptr->ssl);

                    if (wsox_ptr->flag & WSOX_CLIENT)
                    {
                        /* Report the error to the application. */
                        wsox_ptr->onerror(wsox_ptr->user_handle, status,
                                          "Connection closed due to error processing incoming frame.");
                    }

                    /* A close is not going to be received from the other side since
                     * we closed the send and receive side, so inform the application
                     * that an abnormal close has occurred.
                     */
                    wsox_ptr->onclose(wsox_ptr->user_handle, WSOX_ABNORMAL_CLOSURE);

                    /* Return an error indicating that the socket associated with
                     * the connection should be closed and all resources freed.
                     */
                    return_status = WSOX_PARSE_ERROR;
                }
            }

            /* If an error occurred while parsing the header. */
            else if (return_status != NU_SUCCESS)
            {
                /* Notify the application that the other side closed the
                 * connection without sending a closing handshake.
                 */
                wsox_ptr->onclose(wsox_ptr->user_handle, WSOX_ABNORMAL_CLOSURE);
            }
        }

        /* Otherwise, the application needs to be notified of the data
         * outstanding on this handle for this frame.
         */
        else
        {
            msg_info.data_len = wsox_ptr->frame_bytes_left;
            msg_info.opcode = wsox_ptr->frame_opcode;

            WSOX_Recv_Notify(wsox_ptr, &msg_info);
        }
    }

    /* Check if the connection is still open. */
    else if (NU_Is_Connected(wsox_ptr->socketd) != NU_TRUE)
    {
        return_status = NU_NOT_CONNECTED;
    }

    return (return_status);

} /* WSOX_Process_Incoming_Frame */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Recv_Notify
*
*   DESCRIPTION
*
*       This function notifies the application callback routine of an
*       available frame.
*
*   INPUTS
*
*       *wsox_ptr               The handle that received the frame.
*       *msg_info               A pointer to the data structure that
*                               holds information about the received
*                               frame.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID WSOX_Recv_Notify(WSOX_CONTEXT_STRUCT *wsox_ptr,
                             WSOX_MSG_INFO *msg_info)
{
    WSOX_Notify_Enable = NU_TRUE;
    WSOX_Notify_Handle = wsox_ptr->user_handle;

    /* If this is a fragment, set the fragment flag so the application
     * knows there are additional segments outstanding for this frame.
     */
    if (wsox_ptr->fragment_opcode == msg_info->opcode)
    {
        msg_info->flags = NU_WSOX_FRAGMENT;
    }

    /* If this is a TEXT frame, add any partial UTF-8 encoded bytes that
     * may be left on the handle from the previous receive call.
     */
    if (wsox_ptr->frame_opcode == WSOX_TEXT_FRAME)
    {
        msg_info->data_len += wsox_ptr->utf8_byte_count;
    }

    /* Inform the application of the pending frame. */
    wsox_ptr->onmessage(wsox_ptr->user_handle, msg_info);

    WSOX_Notify_Enable = NU_FALSE;

} /* WSOX_Recv_Notify */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Process_Close
*
*   DESCRIPTION
*
*       This function processes an incoming close frame from a foreign
*       node.
*
*   INPUTS
*
*       *wsox_ptr               The handle for which the close frame was
*                               received.
*       data_len                The length of the data in the close frame.
*
*   OUTPUTS
*
*       WSOX_NORMAL             The close frame was processed successfully.
*       WSOX_PROTO_ERROR        The "status code" is invalid.
*       WSOX_BAD_DATA           The "reason" field is not proper UTF-8
*                               encoded data.
*
*************************************************************************/
STATIC STATUS WSOX_Process_Close(WSOX_CONTEXT_STRUCT *wsox_ptr, UINT64 data_len)
{
    NU_MEMORY_POOL      *memory_ptr;
    CHAR                *buffer = NU_NULL;
    STATUS              status = WSOX_NORMAL;
    UINT64              byte;
    UINT32              socket_count;
    INT32               rx_bytes;
    UINT16              close_status = WSOX_NO_STATUS_RCVD;

    /* Determine the number of bytes on the socket. */
    socket_count = WSOX_Check_For_Data(wsox_ptr->socketd, wsox_ptr->ssl);

    /* If there is no data on the socket, but there is data expected, ensure
     * the connection is still open.
     */
    if ( (socket_count == 0) && (data_len != 0) &&
         (NU_Is_Connected(wsox_ptr->socketd) != NU_TRUE) )
    {
        status = WSOX_ABNORMAL_CLOSURE;
    }

    /* RFC 6455 - section 5.5.1 - If there is a body, the first two bytes
     * of the body MUST be a 2-byte unsigned integer (in network byte order)
     * representing a status code with value /code/. Following the 2-byte
     * integer, the body MAY contain UTF-8-encoded data with value /reason/.
     */
    else if (data_len)
    {
        /* Set the socket to blocking so the known data can be retrieved. */
        NU_Fcntl(wsox_ptr->socketd, NU_SETFLAG, NU_BLOCK);

        /* The data must be at least the length of a status code. */
        if (data_len >= WSOX_STATUS_CODE_LEN)
        {
            wsox_ptr->flag |= WSOX_PARTIAL_CLOSE;

            /* If the close message contains just a status code, do not
             * allocate memory for it - use the 2-byte buffer.
             */
            if (data_len <= WSOX_STATUS_CODE_LEN)
            {
                buffer = wsox_ptr->close_status;
            }

            else
            {
                /* If a buffer has not already been allocated. */
                if (wsox_ptr->close_reason == NU_NULL)
                {
                    /* Get a pointer to the system memory pool. */
                    status = NU_System_Memory_Get(&memory_ptr, NU_NULL);

                    if (status == NU_SUCCESS)
                    {
                        /* Allocate memory for the close buffer. */
                        status = NU_Allocate_Memory(memory_ptr,
                                                    (VOID**)&wsox_ptr->close_reason,
                                                    data_len, NU_NO_SUSPEND);
                    }

                    if (status == NU_SUCCESS)
                    {
                        status = WSOX_NORMAL;
                    }

                    else
                    {
                        status = WSOX_UNEXPECTED_COND;
                    }
                }

                buffer = wsox_ptr->close_reason;
            }

            if (status == WSOX_NORMAL)
            {
                /* If there is data on the socket. */
                if (socket_count)
                {
                    /* Receive as much of the data as is available. */
                    rx_bytes = WSOX_Recv(wsox_ptr->socketd, &buffer[wsox_ptr->close_len],
                                         data_len - wsox_ptr->close_len, wsox_ptr->ssl);

                    if (rx_bytes > 0)
                    {
                        wsox_ptr->close_len += rx_bytes;
                        socket_count -= rx_bytes;
                    }

                    else
                    {
                        status = WSOX_UNEXPECTED_COND;
                    }

                    /* If all the data was received. */
                    if (wsox_ptr->close_len == data_len)
                    {
                        wsox_ptr->flag &= ~WSOX_PARTIAL_CLOSE;

                        /* If the frame is masked. */
                        if (wsox_ptr->header[WSOX_MSK_LEN_OFFSET] & 0x80)
                        {
                            /* Unmask the data. */
                            WSOX_Mask_Data(buffer, buffer, wsox_ptr->frame_mask,
                                           data_len, 0);
                        }

                        /* If there is no body, or the body is valid UTF-8. */
                        if ( (data_len == WSOX_STATUS_CODE_LEN) ||
                             (WSOX_Validate_UTF8(&buffer[WSOX_STATUS_CODE_LEN],
                                                 data_len - WSOX_STATUS_CODE_LEN,
                                                 &byte) == NU_TRUE) )
                        {
                            /* Get the two-byte close status. */
                            close_status = GET16(buffer, 0);

                            /* These close statuses are reserved for the WebSocket server
                             * to return to the application and should not be present in
                             * an incoming packet.
                             */
                            if ( (close_status == WSOX_NO_STATUS_RCVD) ||
                                 (close_status == WSOX_ABNORMAL_CLOSURE) ||
                                 (close_status == WSOX_TLS_FAILURE) )
                            {
                                /* An error occurred processing this frame. */
                                status = WSOX_PROTO_ERROR;
                            }
                        }

                        else
                        {
                            /* An error occurred processing this frame. */
                            status = WSOX_BAD_DATA;
                        }
                    }
                }
            }

            /* An internal error has occurred.  Inform the application of the
             * close, passing no status code.
             */
            else
            {
                close_status = WSOX_NO_STATUS_RCVD;
            }
        }

        else
        {
            /* An error occurred processing this frame. */
            status = WSOX_PROTO_ERROR;
        }

        /* Set the socket back to non-blocking. */
        NU_Fcntl(wsox_ptr->socketd, NU_SETFLAG, NU_NO_BLOCK);
    }

    /* No close status was specified. */
    else
    {
        close_status = WSOX_NO_STATUS_RCVD;
    }

    /* If no error occurred while processing the close frame, pass the
     * status code to the application.
     */
    if ( (status == WSOX_NORMAL) && (!(wsox_ptr->flag & WSOX_PARTIAL_CLOSE)) )
    {
        /* The endpoint must stop receiving data. */
        wsox_ptr->flag &= ~WSOX_RX;

        /* Notify the application that a close frame was received. */
        wsox_ptr->onclose(wsox_ptr->user_handle, close_status);

        /* Close the receive side of the socket so all outstanding bytes of
         * data are removed from the socket.
         */
        NU_Shutdown(wsox_ptr->socketd, SHUT_RD);

        /* Deallocate the buffer if one was allocated. */
        if ( (buffer) && (data_len != WSOX_STATUS_CODE_LEN) )
        {
            /* Deallocate the payload buffer. */
            NU_Deallocate_Memory(buffer);

            wsox_ptr->close_reason = NU_NULL;
        }
    }

    return (status);

} /* WSOX_Process_Close */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Process_Connection_Request
*
*   DESCRIPTION
*
*       This function completes the connection handshake process,
*       establishing the new connection with the foreign node.
*
*   INPUTS
*
*       *wsox_listener          The local listening structure over which
*                               to establish the connection.
*       *wsox_client            The new client's connection parameters.
*       socketd                 The socket over which to send the
*                               connection response.
*       *keys                   A pointer to the null-terminated key used
*                               by the client in the connection request.
*       *ssl_ptr                A pointer to the SSL structure if this
*                               connection is secure.
*
*   OUTPUTS
*
*       NU_SUCCESS              The connection has been successfully
*                               established.
*
*       Otherwise, an operating-system specific error is return
*       indicating that the connection could not be established with
*       the foreign node.
*
*************************************************************************/
STATUS WSOX_Process_Connection_Request(WSOX_CONTEXT_STRUCT *wsox_listener,
                                       NU_WSOX_CONTEXT_STRUCT *wsox_client,
                                       INT socketd, CHAR *keys, VOID *ssl_ptr)
{
    NU_MEMORY_POOL  *memory_ptr;
    STATUS          status;
    CHAR            *buf_ptr;
    INT32           bytes_sent;
    INT             encode_len;
    UINT16          buf_len = WSOX_SVR_RSP_CONST_LEN;

    /* If the client specified protocols that the server supports, determine
     * the amount of memory required to echo them in the response.
     */
    if (wsox_client->protocols)
    {
        /* Add memory for the protocol header field, protocol list and
         * two bytes for the terminating \r\n.
         */
        buf_len += (strlen(wsox_client->protocols) +
                    strlen(WSOX_SEC_PROTOCOL) + 2);
    }

    /* Get a pointer to the system memory pool. */
    status = NU_System_Memory_Get(&memory_ptr, NU_NULL);

    if (status == NU_SUCCESS)
    {
        /* Allocate memory for the output buffer. */
        status = NU_Allocate_Memory(memory_ptr, (VOID**)&buf_ptr, buf_len,
                                    NU_NO_SUSPEND);
    }

    if (status == NU_SUCCESS)
    {
        /* RFC 6455 - section 4.2.2 - If the server chooses to accept the
         * incoming connection, it MUST reply with a valid HTTP response
         * indicating the following:
         */

        /* A Status-Line with a 101 response code. */
        strcpy(buf_ptr, WSOX_SUCCESS_RESPONSE);

        /* An |Upgrade| header field with value "websocket" */
        strcat(buf_ptr, WSOX_UPGRADE_HDR);

        /* A |Connection| header field with value "Upgrade" */
        strcat(buf_ptr, WSOX_CONNECTION_HDR);

        /* Optionally, a |Sec-WebSocket-Protocol| header field, with a value
         * /subprotocol/.
         */
        if (wsox_client->protocols)
        {
            strcat(buf_ptr, WSOX_SEC_PROTOCOL);
            strcat(buf_ptr, wsox_client->protocols);
            strcat(buf_ptr, "\r\n");
        }

        /* Add the accept header field. */
        strcat(buf_ptr, WSOX_SEC_ACCEPT);

        /* Base64_Encode adds a \n character that will be overwritten upon return.
         * WSOX_SRV_KEY_LEN includes two bytes for \r\n, so there are technically
         * WSOX_SRV_KEY_LEN - 1 bytes available for the Base64_Encode routine to
         * encode the hashed string.
         */
        encode_len = (WSOX_SRV_KEY_LEN - 1);

        /* RFC 6455 - section 1.3 - Sec-WebSocket-Key - For this header field,
         * the server has to take the value (as present in the header field...)
         * and concatenate this with the Globally Unique Identifier
         * "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" in string form, ...
         * A SHA-1 hash (160 bits), base64-encoded of this concatenation is
         * then returned in the server's handshake.
         */
        if (WSOX_Create_Accept_Key(keys, strlen(keys),
                                   &buf_ptr[buf_len - WSOX_SRV_KEY_LEN - 2],
                                   &encode_len) == NU_SUCCESS)
        {
            /* Terminate the accept header field and add the final HTTP header
             * terminator.
             */
            memcpy(&buf_ptr[buf_len - 4], "\r\n\r\n", 4);

            /* Send the response. */
            bytes_sent = WSOX_Send(socketd, buf_ptr, buf_len, ssl_ptr);

            if (bytes_sent > 0)
            {
                status = NU_SUCCESS;
            }

            else
            {
                status = bytes_sent;
            }
        }

        NU_Deallocate_Memory(buf_ptr);
    }

    return (status);

} /* WSOX_Process_Connection_Request */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Create_Server_Handle
*
*   DESCRIPTION
*
*       Create a new internal WebSocket server context structure.
*
*   INPUTS
*
*       *context_ptr            The structure on which to base the new
*                               structure.
*       *user_handle            A pointer that will be filled in with
*                               the handle for this new context
*                               structure.
*
*   OUTPUTS
*
*       NU_SUCCESS if the new structure could be created; otherwise,
*       an operating-system specific error.
*
*       WSOX_NO_HANDLES         There is not a free handle index in the
*                               system for the new structure.
*
*************************************************************************/
STATUS WSOX_Create_Server_Handle(NU_WSOX_CONTEXT_STRUCT *context_ptr,
                                 UINT32 *user_handle)
{
    STATUS                  status;
    WSOX_CONTEXT_STRUCT     *wsox_ptr;

    /* Get the WebSocket semaphore. */
    status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Create the new context structure. */
        wsox_ptr = WSOX_Create_Context(context_ptr, &status);

        /* If the structure was successfully created, return the handle to
         * the caller.
         */
        if ( (status == NU_SUCCESS) && (wsox_ptr) )
        {
            /* Add the new entry to the list of listeners. */
            DLL_Enqueue(&WSOX_Listener_List, wsox_ptr);

            *user_handle = wsox_ptr->user_handle;
        }

        (VOID)NU_Release_Semaphore(&WSOX_Resource);
    }

    return (status);

} /* WSOX_Create_Server_Handle */
