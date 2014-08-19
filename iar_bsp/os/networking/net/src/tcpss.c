/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME
*
*       tcpss.c
*
*   DESCRIPTION
*
*       TCP socket services. That is those services that are used by the
*       sockets layer to manage the TCP.
*
*   DATA STRUCTURES
*
*       NONE
*
*   FUNCTIONS
*
*       TCPSS_Recv_Data
*       TCPSS_Do_Connect
*       TCPSS_Net_Close
*       TCPSS_Net_Close_EST
*       TCPSS_Net_Listen
*       TCPSS_Net_Read
*       TCPSS_Net_Send
*       TCPSS_Send_Data
*       TCPSS_Net_Write
*       TCPSS_Net_Xopen
*       TCPSS_Send_SYN_FIN
*       TCPSS_Window_Probe
*       TCPSS_Send_Window_Probe
*       TCPSS_Is_Unique_Connection
*       TCPSS_Half_Close
*
*   DEPENDENCIES
*
*       nu_net.h
*       net_extr.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/net_extr.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/externs6.h"
#include "networking/in6.h"
#endif

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

/* Local Prototypes. */
STATIC STATUS  TCPSS_Net_Send(TCP_PORT *, NET_BUFFER *);
STATIC STATUS  TCPSS_Do_Connect(INT, UINT16);
STATIC STATUS  TCPSS_Window_Probe(TCP_PORT *);
STATIC INT     TCPSS_Is_Unique_Connection(const UINT8 *, UINT16,
                                          const UINT8 *, UINT16, INT16);
STATIC STATUS  TCPSS_Net_Close_EST(TCP_PORT *);
STATIC INT32   TCPSS_Net_Read(struct sock_struct *, CHAR *, UINT16);
STATIC INT32   TCPSS_Net_Write(const struct sock_struct *, UINT8 HUGE *,
                               UINT16, INT *);

extern TCP_BUFFER_LIST  TCP_Buffer_List;

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Recv_Data
*
*   DESCRIPTION
*
*       Receive the data for TCP.  This function is used by NU_Recv
*       and NU_Recvmsg to receive data over a TCP socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *buff                   Pointer to the buffer in which to put the
*                               data
*       nbytes                  Specifies the max number of bytes of data
*                               to return
*
*   OUTPUTS
*
*       Number of bytes received.
*       NU_NO_PORT_NUMBER       No local port number was stored in the
*                               socket descriptor.
*       NU_NOT_CONNECTED        The read side of the socket has been closed
*                               by the application, both the read and write
*                               side of the socket have been closed by the
*                               application or the connection has been reset
*                               by the other side.
*       NU_WOULD_BLOCK          The system needs to suspend on a resource, but
*                               the socket is non-blocking.
*       NU_NO_ROUTE_TO_HOST     This is an icmp_error if no route to host exist
*       NU_CONNECTION_REFUSED   This is an icmp_error if the connection is refused.
*       NU_MSG_TOO_LONG         This is an icmp_error if the message is too large.
*       NU_CONNECTION_TIMED_OUT TCP Keep-Alive packets found that the
*                               connection has timed out.
*
*       An ICMP Error code will be returned if an ICMP packet was
*       received for the socket:
*
*       NU_DEST_UNREACH_ADMIN
*       NU_DEST_UNREACH_ADDRESS
*       NU_DEST_UNREACH_PORT
*       NU_TIME_EXCEED_HOPLIMIT
*       NU_TIME_EXCEED_REASM
*       NU_PARM_PROB_HEADER
*       NU_PARM_PROB_NEXT_HDR
*       NU_PARM_PROB_OPTION
*       NU_DEST_UNREACH_NET
*       NU_DEST_UNREACH_HOST
*       NU_DEST_UNREACH_PROT
*       NU_DEST_UNREACH_FRAG
*       NU_DEST_UNREACH_SRCFAIL
*       NU_PARM_PROB
*       NU_SOURCE_QUENCH
*
*************************************************************************/
INT32 TCPSS_Recv_Data(INT socketd, CHAR *buff, UINT16 nbytes)
{
    INT32               count;          /* number of bytes written */
    UINT16              port_num;       /* local machine's port number */
    struct sock_struct  *sockptr;       /* pointer to current socket */
    struct SCK_TASK_ENT task_entry;     /* task entry for list operations */
    UINT32              socket_id;
    INT32               return_status = NU_SUCCESS;

    /*  Pick up a pointer to the socket list. */
    sockptr = SCK_Sockets[socketd];

    /* retrieve the local port number from the socket descriptor */
    port_num = sockptr->s_local_addr.port_num;

    /* verify that a port number exists */
    if (port_num)
    {
        /*
         * Check the socket's block flag to see if the caller wants to
         * wait for data or not, and if so, there is no data in the
         * input buffer and the connection is still alive.
         */
        if (sockptr->s_recvbytes == 0)
        {
            /* If there is an error on the socket, return the error */
            if (sockptr->s_error != 0)
            {
                return_status = sockptr->s_error;

                /* Reset the socket error value */
                sockptr->s_error = 0;
            }

            /*  Give up the resource until we can run again.  */
            else if ( (sockptr->s_flags & SF_BLOCK) &&
                      (sockptr->s_state & SS_ISCONNECTED) &&
                      (!(sockptr->s_state & SS_CANTRCVMORE)) )
            {
                /* Initialize the list entry's task number */
                task_entry.task = NU_Current_Task_Pointer();

                /* Add it to the list of tasks pending on receive */
                DLL_Enqueue(&sockptr->s_RXTask_List, &task_entry);

                /* Get the socket ID to verify the socket after suspension */
                socket_id = sockptr->s_struct_id;

                SCK_Suspend_Task(task_entry.task);

                /* If this is a different socket, handle it appropriately */
                if ( (!SCK_Sockets[socketd]) ||
                     (SCK_Sockets[socketd]->s_struct_id != socket_id) )
                    return_status = NU_SOCKET_CLOSED;
            }
        }

        /* If there is not an error on the socket */
        if (return_status == NU_SUCCESS)
        {
            /* If the receive side of the connection has not been closed
             * by the application.
             */
            if (!(sockptr->s_state & SS_CANTRCVMORE))
            {
                /* Read the data on the socket. */
                count = TCPSS_Net_Read(sockptr, buff, nbytes);

                /* Verify success of transfer - if not successful, either
                 * we are not connected or an ICMP error message was
                 * received describing the transmission problem.  Check
                 * for a successful status first to speed up the processing.
                 */
                if (count > 0)
                    return_status = count;

                /* Otherwise, no data was read. */
                else if (count < 0)
                {
                    /* If the connection timed out and there is an error
                     * on the socket, return the error.
                     */
                    if ( (sockptr->s_state & SS_TIMEDOUT) &&
                         (sockptr->s_error != 0) )
                    {
                        return_status = sockptr->s_error;

                        /* Reset the socket error value */
                        sockptr->s_error = 0;
                    }

                    /* Else we are not connected */
                    else
                        return_status = NU_NOT_CONNECTED;
                }

                /* If no data was received and this is a non-blocking
                 * socket, set the status to indicate that no data is
                 * available.
                 */
                else if (!(sockptr->s_flags & SF_BLOCK))
                    return_status = NU_WOULD_BLOCK;

                /* return zero number of bytes transferred */
                else
                    return_status = 0;
            }

            /* The application has closed the read side of the connection.
             * Return an error indicative of this.
             */
            else
                return_status = NU_NOT_CONNECTED;
        }
    }
    else
    {
        return_status = NU_NO_PORT_NUMBER;
    }
  
    /* Trace log */
    T_SOCK_RX(nbytes, socketd, return_status);
    
    return (return_status);

} /* TCPSS_Recv_Data */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Net_Read
*
*   DESCRIPTION
*
*       Read data from the connection buffer (specified by *pnum*) into
*       the user buffer for up to *n* bytes.
*       Returns number of bytes read, or <0 if error.  Does not block.
*
*   INPUTS
*
*       *sock_ptr               Pointer to the socket list
*       *buffer                 Pointer to the buffer to read
*       nbytes                  Number of bytes to read
*
*   OUTPUTS
*
*       The number of bytes read or 0 if an error occurs.
*
*************************************************************************/
STATIC INT32 TCPSS_Net_Read(struct sock_struct *sock_ptr, char *buffer,
                            UINT16 nbytes)
{
    UINT32          i, lowwater;
    INT32           howmany;
    INT             pnum;
    TCP_PORT        *prt;
    struct SCK_TASK_ENT *task_entry;

    /* As long as there is data return it.  It is possible for the connection to
     * have been closed in the event of a RESET.  However, if the data is here
     * then it has been validated and is acceptable. So the data will be given to
     * the application even if the connection has been closed.
     */
    if (sock_ptr->s_recvbytes > 0)
    {
        /* Retrieve the queue data. */
        howmany = TCP_Dequeue(sock_ptr, buffer, (UINT32)nbytes);

        if (sock_ptr->s_state & SS_ISCONNECTED)
        {
            pnum = sock_ptr->s_port_index;

            /* Get the look up the index of the port structure. */
            if (pnum >= 0)
            {
                prt = TCP_Ports[pnum];

                i = prt->in.size;                   /* how much before? */
                prt->in.size += howmany;            /* increment leftover room  */

                lowwater = (prt->credit >> 1);

                if ((i < lowwater) && ((prt->in.size) >= lowwater))
                    TCP_ACK_It(prt, 1);
            }
            else
            {
                howmany = NU_NOT_CONNECTED;
            }
        }

        /* If there are tasks waiting to recv data then we should resume
           another task from the rx list */
        if (sock_ptr->s_RXTask_List.flink != NU_NULL)
        {
            if (sock_ptr->s_recvbytes > 0)
            {
                task_entry = DLL_Dequeue(&sock_ptr->s_RXTask_List);

                if (NU_Resume_Task(task_entry->task) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to resume task", NERR_SEVERE,
                                    __FILE__, __LINE__);
            }
        }

    }
    else
    {
        /* There was no queued data to receive. So, check to see if the
           connection still exists. */
        if (sock_ptr->s_state & SS_ISCONNECTED)
            howmany = 0;
        else
            howmany = NU_NOT_CONNECTED;
    }

    return (howmany);

} /* TCPSS_Net_Read */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Send_Data
*
*   DESCRIPTION
*
*       Transmit the data for TCP.  This function is used by NU_Send
*       and NU_Sendmsg to transmit data over a TCP socket.
*
*   INPUTS
*
*       *sockptr                Pointer to a socket on which to send
*                               the data.
*       *buffer                 Pointer to the data to be sent.
*       nbytes                  The number of bytes of data to send.
*
*   OUTPUTS
*
*       > 0                     The number of bytes sent.
*       NU_NOT_CONNECTED        The data transfer was not completed. This
*                               probably occurred because the connection
*                               was closed for some reason.
*       NU_NO_ROUTE_TO_HOST     This is an icmp_error if no route to host exist
*       NU_CONNECTION_REFUSED   This is an icmp_error if the connection is refused.
*       NU_MSG_TOO_LONG         This is an icmp_error if the message is too large.
*       NU_NO_PORT_NUMBER       No local port number was stored in the socket
*                               descriptor.
*       NU_CONNECTION_TIMED_OUT TCP Keep-Alive packets found that the
*                               connection has timed out.
*       NU_MSGSIZE              The Zero Copy buffers contain too much data.
*
*       An ICMP Error code will be returned if an ICMP packet was
*       received for the socket:
*
*       NU_DEST_UNREACH_ADMIN
*       NU_DEST_UNREACH_ADDRESS
*       NU_DEST_UNREACH_PORT
*       NU_TIME_EXCEED_HOPLIMIT
*       NU_TIME_EXCEED_REASM
*       NU_PARM_PROB_HEADER
*       NU_PARM_PROB_NEXT_HDR
*       NU_PARM_PROB_OPTION
*       NU_DEST_UNREACH_NET
*       NU_DEST_UNREACH_HOST
*       NU_DEST_UNREACH_PROT
*       NU_DEST_UNREACH_FRAG
*       NU_DEST_UNREACH_SRCFAIL
*       NU_PARM_PROB
*       NU_SOURCE_QUENCH
*
*************************************************************************/
INT32 TCPSS_Send_Data(INT socketd, CHAR *buff, UINT16 nbytes)
{
    NET_BUFFER_SUSPENSION_ELEMENT   waiting_for_buffer;
    UINT16                          count;              /* number of bytes written */
    UINT16                          bytes_to_go;        /* number of bytes yet to transmit */
    UINT16                          curr_count = 0;     /* number of bytes written this call */
    UINT16                          tx_bytes = 0;
    INT                             status = 0;
    struct SCK_TASK_ENT             task_entry;         /* task entry for list operations */
    UINT32                          socket_id;
    struct  sock_struct             *sockptr;
    INT32                           return_status;
    UINT16                          local_s_flags;
    NET_BUFFER                      *seg_ptr = NU_NULL, *seg_last = NU_NULL;

    /* Pick up a pointer to the socket list entry.  */
    sockptr = SCK_Sockets[socketd];
    
    /* Trace log */
    T_SOCK_TX_LAT_START(socketd);

    /* Check for a NULL socket ptr and a port state that will allow the
     * transmission of data.
     */
    if ( (sockptr) && (sockptr->s_port_index >= 0) &&
         ((TCP_Ports[sockptr->s_port_index]->state == SCWAIT) ||
          (TCP_Ports[sockptr->s_port_index]->state == SEST)) )
    {       
        local_s_flags = sockptr->s_flags;

        /* Initialize the byte counters */
        count = 0;
        bytes_to_go = nbytes;

        if (local_s_flags & SF_ZC_MODE)
        {
            seg_ptr = (NET_BUFFER*)buff;

            /* A TCP Zero Copy buffer chain is a multiple of the MSS of the
             * other side of the connection.
             */
            tx_bytes = TCP_Ports[sockptr->s_port_index]->p_smss;
        }

        while (bytes_to_go != 0)
        {
            /* If this is a Zero Copy buffer */
            if (local_s_flags & SF_ZC_MODE)
            {
                /* If the previous transmission of the last buffer chain was
                 * successful, get a pointer to the next buffer chain and determine
                 * the length of the data to transmit.
                 */
                if (status == NU_SUCCESS)
                {
                    /* Ensure there is a next valid buffer in the zero copy chain;
                     * otherwise, the user may have misused the API to build the
                     * chain.
                     */
                    if (!seg_ptr)
                        break;

                    /* Set the transmission buffer to the head of the next buffer
                     * chain to transmit.
                     */
                    buff = (CHAR*)seg_ptr;

                    /* Get a pointer to the next chain of buffers */
                    if (bytes_to_go > TCP_Ports[sockptr->s_port_index]->p_smss)
                    {
                        /* Find the start of the next chain of buffers */
                        do
                        {
                            /* If this is the end of the current chain, get a
                             * pointer to the next chain of buffers.
                             */
                            if ( (seg_ptr->next_buffer) &&
                                 (seg_ptr->next_buffer->mem_flags & NET_PARENT) )
                            {
                                /* Save a pointer to this segment so the next
                                 * pointer can be set to NU_NULL.
                                 */
                                seg_last = seg_ptr;

                                /* Get a pointer to the start of the next chain
                                 * of buffers
                                 */
                                seg_ptr = seg_ptr->next_buffer;

                                /* Set the next pointer to NU_NULL as a precautionary
                                 * measure.
                                 */
                                seg_last->next_buffer = NU_NULL;

                                break;
                            }

                            /* If there is a next_buffer pointer, get a pointer
                             * to it.
                             */
                            else
                                seg_ptr = seg_ptr->next_buffer;

                        } while (seg_ptr);
                    }

                    /* This is the last/only buffer chain in the chain of buffers */
                    else
                        tx_bytes = bytes_to_go;
                }

                /* Send the chain */
                curr_count = (UINT16)TCPSS_Net_Write(sockptr, (UINT8*)buff,
                                                     tx_bytes, &status);
            }

            /* call tcp/ip library netwrite routine */
            else
                curr_count = (UINT16)TCPSS_Net_Write(sockptr, (UINT8*)(buff + count),
                                                     bytes_to_go, &status);

            /* If 0 was returned, then for some reason we failed to send the
               data.  Check to see if we failed because we have already
               filled up the window of the remote host or no buffers are
               available at this time.  If so just continue.
               We will suspend below. We will either be awakened for one or two reasons:
               1. When the remote host acks some of the data we have already sent,
               which means the window is no longer full.
               2. More buffers are now available. At that point some more data can
               be transmitted. If we failed for any other reason check to see if some
               data has already been sent. If so return the number of bytes that have
               already been sent and the the status is successful.  If no data has
               been sent (i.e., count == 0) then the error code will be returned so
               that the application layer will see the error.
            */
            if ( (curr_count != 0) ||
                 (((status == NU_WINDOW_FULL) || (status == NU_NO_BUFFERS)) &&
                  (local_s_flags & SF_BLOCK)) )
            {
                /* update the bytes counter */
                bytes_to_go = (UINT16)(bytes_to_go - curr_count);

                /* update the total bytes sent */
                count = (UINT16)(count + curr_count);

                /* Do not suspend for a non-blocking socket. */
                if (local_s_flags & SF_BLOCK)
                {
                    /* If the status is NO BUFFERS then we will suspend until
                       more buffers become available. */
                    if (status == NU_NO_BUFFERS)
                    {
                        NLOG_Error_Log("Suspending for buffers in TCPSS_Send_Data",
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);

                        /* Add this task to the element. */
                        waiting_for_buffer.waiting_task = NU_Current_Task_Pointer();

                        /* Set up the reference to the suspending task */
                        task_entry.task = waiting_for_buffer.waiting_task;

                        /* Get a reference to the memory buffer element */
                        task_entry.buff_elmt = &waiting_for_buffer;

                        /* Put this on the suspended TX list */
                        DLL_Enqueue(&sockptr->s_TXTask_List, &task_entry);

                        /* Setup a reference to the socket */
                        waiting_for_buffer.socketd = socketd;

                        /* Setup a reference to the task entry */
                        waiting_for_buffer.list_entry = &task_entry;

                        /* Put this element on the buffer suspension list */
                        DLL_Enqueue(&MEM_Buffer_Suspension_List, &waiting_for_buffer);

                        /* Get the socket ID to verify the socket after suspension */
                        socket_id = sockptr->s_struct_id;

                        /* Suspend this task. */
                        SCK_Suspend_Task(waiting_for_buffer.waiting_task);

                        /* Make sure this entry is removed from the
                         * buffer suspension list.
                         */
                        DLL_Remove(&MEM_Buffer_Suspension_List, &waiting_for_buffer);

                        /* If this is a different socket, handle it appropriately */
                        if ( (!SCK_Sockets[socketd]) ||
                             (SCK_Sockets[socketd]->s_struct_id != socket_id) )
                        {
                            NLOG_Error_Log("Socket closed while suspending for buffers in TCPSS_Send_Data",
                                           NERR_INFORMATIONAL, __FILE__, __LINE__);

                            status = NU_SOCKET_CLOSED;
                            break;
                        }
                    }

                    /* If the number of bytes to go indicates more to be sent,
                       then the other side's window must be filled up.  Suspend
                       until TCP resumes the TX task.
                     */
                    else if ( (bytes_to_go != 0) &&
                              ((!(SCK_Sockets[socketd]->s_flags & SF_ZC_MODE)) ||
                               (curr_count == 0)) )
                    {
                        NLOG_Error_Log("Suspending for the window to open in TCPSS_Send_Data",
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);

                        /* Set the SS_WAITWINDOW flag because buffers are waiting to be sent. */
                        sockptr->s_state |= SS_WAITWINDOW;

                        /* Initialize the list entry's task number */
                        task_entry.task = NU_Current_Task_Pointer();

                        /* Add it to the list of tasks pending on transmit */
                        DLL_Enqueue(&sockptr->s_TXTask_List, &task_entry);

                        /* Get the socket ID to verify the socket after suspension */
                        socket_id = sockptr->s_struct_id;

                        SCK_Suspend_Task(task_entry.task);

                        /* If this is a different socket, handle it appropriately */
                        if ( (SCK_Sockets[socketd]) &&
                             (SCK_Sockets[socketd]->s_struct_id == socket_id) )
                            sockptr->s_state &= ~SS_WAITWINDOW;

                        else
                        {
                            NLOG_Error_Log("Socket closed while suspending for wait window in TCPSS_Send_Data",
                                           NERR_INFORMATIONAL, __FILE__, __LINE__);

                            status = NU_SOCKET_CLOSED;
                            break;
                        }

                    } /* still bytes to be sent */
                }

                /* Otherwise, this is a non-blocking socket.  Set the status
                 * to NU_SUCCESS, because some data was transmitted, and return
                 * to the application.
                 */
                else
                {
                    status = NU_SUCCESS;
                    break;
                }
            }

            else
            {
                /* If count != 0, send the info that we already have sent. */
                if (count != 0)
                    status = NU_SUCCESS;

                /* If count == 0 and NON blocking then we need to send an error
                 * NU_WOULD_BLOCK.  If the write side of the connection has
                 * been shutdown, do not return NU_WOULD_BLOCK, but the return
                 * value from TCPSS_Net_Write.
                 */
                else if ( (!(local_s_flags & SF_BLOCK)) &&
                          (!(sockptr->s_state & SS_CANTWRTMORE)) )
                {
                    status = NU_WOULD_BLOCK;
                }

                break;
            }

        } /* while still bytes to transmit. */

        /* Return the number of bytes transmitted if data was transmitted
         * successfully.
         */
        if (status == NU_SUCCESS)
            return_status = count;

        /* If the connection timed out and there is an error on the socket,
         * return the error.
         */
        else if ( (sockptr->s_state & SS_TIMEDOUT) &&
                  (sockptr->s_error != 0) )
        {
            return_status = sockptr->s_error;

            /* Reset the socket error value */
            sockptr->s_error = 0;
        }

        else
            return_status = status;

        /* If this is a Zero Copy buffer chain. */
        if (local_s_flags & SF_ZC_MODE)
        {
            /* If the current buffer chain was not transmitted. */
            if ( (curr_count == 0) && (seg_last) )
            {
                /* Reassemble the chain as it was when the function was called so
                 * the caller can deallocate the entire chain of buffers.
                 */
                seg_last->next_buffer = seg_ptr;

                /* Restore seg_ptr to the current buffer chain that is trying
                 * to be transmitted.
                 */
                seg_ptr = (NET_BUFFER*)buff;
            }

            /* If some data was sent, free the remaining buffers.  Otherwise, the
             * application is responsible for freeing the data.
             */
            if ( (return_status > 0) && ((UINT16)return_status != nbytes) )
            {
                MEM_Multiple_Buffer_Chain_Free(seg_ptr);
            }
        }
    }

    /* If the connection timed out and there is an error on the socket,
     * return the error.
     */
    else if ( (sockptr) && (sockptr->s_state & SS_TIMEDOUT) &&
              (sockptr->s_error != 0) )
    {
        return_status = sockptr->s_error;

        /* Reset the socket error value */
        sockptr->s_error = 0;
    }

    /* Either the socket pointer was NULL or the connection has closed. */
    else
        return_status = NU_NOT_CONNECTED;

    /* Trace log */
    T_SOCK_TX_LAT_STOP(nbytes, socketd, return_status);
    
    return (return_status);

} /* TCPSS_Send_Data */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Net_Write
*
*   DESCRIPTION
*
*       Write data into the output queue (specified by pnum).
*       As long as there is buffer space and the foreign host's receive
*       window has not been filled, data will be sent.  The number of
*       bytes actually sent is returned ( a number between 0 and nbytes).
*
*   INPUTS
*
*       *sockptr                Pointer to a socket on which to send
*                               the data.
*       *buffer                 Pointer to the data to be sent.
*       nbytes                  The number of bytes of data to send.
*       *status                 Indicates why all data could not be sent.
*
*   OUTPUTS
*
*       The number of bytes that were sent.
*
*************************************************************************/
STATIC INT32 TCPSS_Net_Write(const struct sock_struct *sockptr,
                             UINT8 HUGE *buffer, UINT16 nbytes, INT *status)
{
    INT         s;
    INT32       nsent = 0;
    TCP_PORT    *prt;
    TCP_WINDOW  *wind;
    NET_BUFFER  *buf_ptr = NU_NULL, *work_buf;
    INT32       numbytes, bytes_left, bytes_to_move, bc;
    UINT32      cwnd;
    TCP_BUFFER  *tcp_buf;

    if ( (sockptr->s_port_index < 0) ||
         (TCP_Ports[sockptr->s_port_index] == NU_NULL) )
    {
        *status = NU_INVALID_PORT;
        return (0);
    }

    prt = TCP_Ports[sockptr->s_port_index];

    /* Must be in the established state and the write side must not have
     * been shutdown.
     */
    if ( ((prt->state != SEST) && (prt->state != SCWAIT)) ||
         (sockptr->s_state & SS_CANTWRTMORE) )
    {
        *status = NU_NOT_CONNECTED;
        return (0);
    }

    /* If we are currently probing the other side of the connection,
     * do not send anything until the window opens up.
     */
    if (prt->probeFlag != NU_CLEAR)
    {
        *status = NU_WINDOW_FULL;
        return (0);
    }

    /* Assume a successful operation until we are proven wrong. */
    *status = NU_SUCCESS;

    wind = &prt->out;

    numbytes = nbytes;

#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)

    /* If congestion control is not disabled. */
    if (!(prt->portFlags & TCP_DIS_CONGESTION))
    {
        /* RFC 2581 - section 4.1 - ... a TCP SHOULD set cwnd to no more
         * than RW before beginning transmission if the TCP has not sent
         * data in an interval exceeding the retransmission timeout.
         */
        if (TQ_Check_Duetime((UNSIGNED)(prt->out.lasttime + prt->p_rto))
            == NU_NO_SUSPEND)
        {
            prt->p_cwnd = prt->p_smss;
        }

        /* If the congestion window is full, no more data can be injected
         * into the network.
         */
        if ( ((INT32)(prt->p_cwnd - prt->out.contain)) <= 0)
        {
#if (NET_INCLUDE_LMTD_TX == NU_TRUE)

            /* If the Limited Transmit flag is set, one more segment
             * can be sent.
             */
            if (prt->portFlags & TCP_TX_LMTD_DATA)
            {
                /* The sender may transmit one segment, but the congestion
                 * window must not be updated.
                 */
                cwnd = prt->p_smss;
            }

            /* No data can be transmitted.  Return an error. */
            else
#endif
            {
                TEST_TCP_No_New_Data();

                NLOG_Error_Log("Congestion window full in TCPSS_Net_Write",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                *status = NU_WINDOW_FULL;

                return (0);
            }
        }

        /* RFC 2581 - section 3.2 - Transmit a segment, if allowed by
         * the new value of cwnd and the receiver's advertised window.
         */
        else
        {
            cwnd = (UINT32)(prt->p_cwnd - prt->out.contain);

            TEST_TCP_New_Data(cwnd, prt);
        }
    }

    /* If Congestion Control is excluded, set cwnd to the number of bytes
     * to send.
     */
    else
    {
        cwnd = numbytes;
    }
#else

    /* If Congestion Control is excluded, set cwnd to the number of bytes
     * to send.
     */
    cwnd = numbytes;

#endif

    /* The TCP output routine never sends more than the minimum of the
     * Congestion Window and the receiver's advertised window.  If we want
     * to send more bytes than allowable, decrease the amount of bytes we
     * want to send.
     */
    if ( (wind->size != 0) && (cwnd < (UINT32)(wind->size - wind->contain)) )
    {
        /* If the congestion window is less than the number of bytes to
         * transmit, adjust the number of bytes to the congestion window.
         */
        if (cwnd < (UINT32)numbytes)
        {
            /* If this is a Zero Copy buffer, suspend until the congestion
             * window opens up enough to send the data.  This should never
             * happen since the congestion window should never fall below
             * the value of the other side's MSS.
             */
            if (sockptr->s_flags & SF_ZC_MODE)
            {
                *status = NU_WINDOW_FULL;
                return (0);
            }

            numbytes = (INT32)cwnd;
        }
    }

    /* If there is not enough room in the advertised window for all the bytes. */
    else if ( ((wind->size > wind->contain) && ((wind->size - wind->contain) < numbytes)) ||
              (wind->size <= wind->contain) || (wind->size == 0) )
    {
        /* If there is room for some data. */
        if (wind->size > wind->contain)
            numbytes = wind->size - wind->contain;
        else
            numbytes = 0;

        /* numbytes will only be less than or equal to zero if the receiver's
         * advertised window is less than or equal to zero, because the
         * Congestion Window will never be less than or equal to zero.
         */
        if (numbytes <= 0)
        {
            /* Check to see if the window probe service should be invoked. */
            if (wind->size == 0)
            {
                NLOG_Error_Log("Probing the foreign side in TCPSS_Net_Write",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                /* Probe the other side of the connection with 1 byte of
                 * data, but do not update the send buffer to reflect
                 * this byte of data.  It will be transmitted again.
                 * This is so we can probe with Zero Copy buffers too.
                 */
                *status = TCPSS_Window_Probe(prt);

                /* If the Window Probe was successfully transmitted, let the
                 * caller know we are currently probing the other side.
                 * Otherwise, return the error generated by TCPSS_Window_Probe.
                 */
                if (*status == NU_SUCCESS)
                    *status = NU_WINDOW_FULL;

                return (0);
            }

            else
            {
                NLOG_Error_Log("Window full, but there is outstanding data in TCPSS_Net_Write",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                *status = NU_WINDOW_FULL;
                return (0);
            }
        }
    }

    /* If this will be the start of a new buffer chain, determine
     * the number of bytes that can be copied into it.
     */
    if ( (wind->nextPacket == NU_NULL) || (sockptr->s_flags & SF_ZC_MODE) )
    {
        /* If there is not enough data left to fill a complete buffer,
         * update bytes_left with the number of bytes left.
         */
        if (numbytes > (INT32)prt->p_smss)
            bytes_left = prt->p_smss;

        /* If this is not a Zero Copy socket, or the number of bytes to transmit
         * is equal to the number of bytes in the Zero Copy buffer.
         */
        else if ( (!(sockptr->s_flags & SF_ZC_MODE)) ||
                  (numbytes == (INT32)nbytes) )
            bytes_left = numbytes;

        /* Otherwise, the congestion window or other side's receive window
         * has shrunk below the number of bytes that are contained in this
         * Zero Copy buffer, and since we cannot truncate data from this buffer
         * chain, return an error.
         */
        else
        {
            NLOG_Error_Log("Cannot truncate ZC data in TCPSS_Net_Write",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            numbytes = 0;
            bytes_left = 0;
            *status = NU_WINDOW_FULL;
        }
    }

    /* Otherwise, this is not a zero-copy buffer and there has already been a
     * buffer chain started for this data.  Determine the number of bytes
     * to copy into the buffer chain.
     */
    else
    {
        /* Get a pointer to the start of the buffer chain. */
        buf_ptr = wind->nextPacket;

        /* Calculate the amount of data to be put into this buffer chain. */
        bytes_left =
            (numbytes < (INT32)(prt->p_smss - buf_ptr->mem_total_data_len))
            ? numbytes : (INT32)(prt->p_smss - buf_ptr->mem_total_data_len);

        bytes_to_move = bytes_left;

        /* Find the last buffer in this buffer chain. */
        for (work_buf = buf_ptr;
             work_buf->next_buffer != NU_NULL;
             work_buf = work_buf->next_buffer)
                  ;

        /* Determine the number of bytes that will fit in the existing buffer. */
        if (work_buf == wind->nextPacket)
            bc = (INT32)((NET_PARENT_BUFFER_SIZE - TCP_HEADER_LEN - prt->p_opt_len)
                - work_buf->data_len);
        else
            bc = (INT32)(NET_MAX_BUFFER_SIZE - work_buf->data_len);

        bc = (bytes_to_move < bc) ? bytes_to_move : bc;

        /* Decrement the number of bytes to fill new buffers by the number
         * of bytes that will fit in the existing buffer.
         */
        bytes_to_move -= bc;

        /* Allocate new buffers in which to copy the data. */
        while (bytes_to_move)
        {
            /* If there are enough buffers, get a new buffer.  Be sure
             * to leave buffers for RX.
             */
            if ((MAX_BUFFERS - MEM_Buffers_Used) > NET_FREE_BUFFER_THRESHOLD)
                work_buf->next_buffer = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

            /* Make sure a buffer was allocated. */
            if (work_buf->next_buffer != NU_NULL)
            {
                work_buf = work_buf->next_buffer;

                bc = (bytes_to_move < (INT32)NET_MAX_BUFFER_SIZE) ?
                        bytes_to_move : (INT32)NET_MAX_BUFFER_SIZE;

                /* Set the data pointer. */
                work_buf->data_ptr = work_buf->mem_packet;

                bytes_to_move -= bc;
            }

            else
            {
                /* Decrement the number of bytes to transmit by the number
                 * of bytes that could not be copied into a buffer.
                 */
                bytes_left -= bytes_to_move;

                bytes_to_move = 0;

                /* Set the status. */
                *status = NU_NO_BUFFERS;

                NLOG_Error_Log("No buffers to fill in started packet in TCPSS_Net_Write",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                /* If there is no room left in this buffer chain for any
                 * data, return an error to the caller.
                 */
                if (!bytes_left)
                    return (bytes_left);

                continue;
            }
        }
    }

    /* While there is more data to send, break it up into individual packets
     * and send it.
     */
    while (numbytes > 0)
    {
        /* If a buffer does not already exist, allocate enough buffers
         * for the data.
         */
        if (!buf_ptr)
        {
            /* If this is not a Zero Copy transmission */
            if (!(sockptr->s_flags & SF_ZC_MODE))
            {
                /* Make sure there are enough buffers left before we take
                 * another one. We must leave some for RX.
                 */
                if ((MAX_BUFFERS - MEM_Buffers_Used) > NET_FREE_BUFFER_THRESHOLD)
                {
                    /* If the TCP header will fit in one buffer. */
                    if ((TCP_HEADER_LEN + prt->p_opt_len) < NET_PARENT_BUFFER_SIZE)
                    {
                        /* Allocate a new buffer chain in which to copy the data. */
                        buf_ptr = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                                           bytes_left + TCP_HEADER_LEN +
                                                           prt->p_opt_len);

                        /* Make sure a buffer chain was allocated. */
                        if (buf_ptr != NU_NULL)
                        {
                            /* Set the data pointer to start just past the TCP
                             * header.
                             */
                            buf_ptr->data_ptr =
                                buf_ptr->mem_parent_packet + TCP_HEADER_LEN + prt->p_opt_len;
                        }
                    }

                    else
                    {
                        buf_ptr = NU_NULL;
                    }

                    /* If buffers were successfully allocated. */
                    if (buf_ptr == NU_NULL)
                    {
                        NLOG_Error_Log("Could not get buffer for new packet in TCPSS_Net_Write",
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);

                        /* Set the status. */
                        *status = NU_NO_BUFFERS;
                        break;
                    }
                }

                else
                {
                    NLOG_Error_Log("Exceeded TCP buffer threshold in TCPSS_Net_Write",
                                   NERR_INFORMATIONAL, __FILE__, __LINE__);

                    /* Set the status. */
                    *status = NU_NO_BUFFERS;
                    break;
                }
            }

            /* Otherwise, the data is already in NET buffers. */
            else
                buf_ptr = (NET_BUFFER*)buffer;

            /* Set the flag indicating this packet is being transmitted */
            buf_ptr->mem_flags |= NET_TX_QUEUE;

            /* Initialize the list that this packet will be deallocated
             * to when transmission is complete.
             */
            buf_ptr->mem_dlist = NU_NULL;

            /* Get a pointer to a free TCP buffer */
            tcp_buf = DLL_Dequeue(&TCP_Buffer_List);

            /* Set the buffer pointer to the new buffer */
            tcp_buf->tcp_buf_ptr = buf_ptr;

            /* Put the TCP buffer on the retransmission list */
            DLL_Enqueue(&prt->out.packet_list, tcp_buf);

            buf_ptr->mem_port_index = prt->pindex;

            /* Flag this buffer as a parent so all subsequent packets in the
             * chain are freed to the same dlist.
             */
            buf_ptr->mem_flags |= NET_PARENT;
        }

        /* If this is not a Zero Copy buffer and the hardware has not been
         * configured to compute the checksum.
         */
        if ( (!(sockptr->s_flags & SF_ZC_MODE))
#if (HARDWARE_OFFLOAD == NU_TRUE)
            && ((prt->tp_route.rt_route)&&(!(prt->tp_route.rt_route->rt_entry_parms.rt_parm_device->
              dev_hw_options_enabled & HW_TX_TCP_CHKSUM)))
#endif
           )
        {
            /* Set the flag indicating that checksumming should be done in unison
             * with data copy.
             */
            buf_ptr->mem_flags |= NET_BUF_SUM;
        }

        /* Copy the data into the NET buffer */
        bytes_left = MEM_Copy_Data(buf_ptr, (CHAR*)buffer, bytes_left,
                                   sockptr->s_flags);

        /* If this is not a Zero Copy buffer, move forward in the buffer by
         * the amount of data just copied.
         */
        if (!(sockptr->s_flags & SF_ZC_MODE))
            buffer += bytes_left;

        buf_ptr->mem_tcp_data_len = (UINT16)(buf_ptr->mem_tcp_data_len + bytes_left);

        /* Now that the data has been broken up into packets, send it.  If some
         * data was sent and there was a timer event set to transmit data, then
         * clear the event.  The data that the event was intended for has just
         * been sent.
         */
        s = TCPSS_Net_Send(prt, buf_ptr);

        /* Unset the Limited Transmit flag. */
        prt->portFlags &= ~TCP_TX_LMTD_DATA;

        /* Update the amount of data in this port. */
        prt->out.contain += bytes_left;

        /* Increment the number of bytes we have sent so far. */
        nsent += bytes_left;

        /* Update the number of packets in this port. */
        if (wind->nextPacket == NU_NULL)
            prt->out.num_packets++;

        /* No data was transmitted, because the packet was not full, and
         * the PUSH bit was not set.
         */
        if (s == 0)
        {
            /* If the timer to transmit the packet has not already been set,
             * set it now.
             */
            if (prt->xmitFlag != NU_SET)
            {
                prt->xmitFlag = NU_SET;

                if (TQ_Timerset(CONTX, (UNSIGNED)prt->pindex, SWSOVERRIDE,
                                0) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to set timer to override Nagle Algorithm",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            wind->nextPacket = buf_ptr;

            break;
        }

        /* If data was sent that was previously stored in the nextPacket
         * pointer of the port, clear the flag to transmit this data and
         * set nextPacket to NU_NULL.  Even if an error occurred, this
         * data has been placed on the retransmission list and will be
         * transmitted when the timer goes off.
         */
        else if (prt->xmitFlag)
        {
            TQ_Timerunset(CONTX, TQ_CLEAR_EXACT, (UNSIGNED)prt->pindex, 0);
            prt->xmitFlag = NU_CLEAR;

            /* Clear the next packet pointer since this packet was sent. */
            wind->nextPacket = NU_NULL;
        }

        /* If the data was successfully transmitted, keep sending the data. */
        if (s > 0)
        {
            /* Decrease the amount of data left before the caller's request has
             * been filled.
             */
            numbytes -= bytes_left;

            /* If there is not enough data left to fill a complete buffer,
             * update bytes_left with the number of bytes left.
             */
            if (numbytes <= (INT32) prt->p_smss)
                bytes_left = numbytes;
            else
                bytes_left = prt->p_smss;

            buf_ptr = NU_NULL;
        }

        /* Otherwise, return the error code to the caller */
        else
        {
            NLOG_Error_Log("Could not send data in TCPSS_Net_Write",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            *status = s;
            break;
        }
    }

    return (nsent);

} /* TCPSS_Net_Write */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Net_Listen
*
*   DESCRIPTION
*
*       Listen to a TCP port number and make the connection automatically
*       when the SYN packet comes in.  The TCP layer will notify the
*       higher layers with a CONOPEN event.  Save the port number returned
*       to refer to this connection.
*
*       example usage : portnum=netlisten ( service )
*
*       Returns < 0 if error
*
*   INPUTS
*
*       serv                    The server to listen on
*       *tcp_check              Pointer to the pseudo tcp pointer
*       family                  The family type of the port to create
*
*   OUTPUTS
*
*       -1
*       -2
*       The port number
*
*************************************************************************/
STATUS TCPSS_Net_Listen(UINT16 serv, const VOID *tcp_check, INT16 family)
{
    INT                 pnum, i;
    TCP_PORT            *prt;
    INT32               tval, tmp, tmnc;

#if (INCLUDE_IPV4 == NU_TRUE)
    SCK_SOCKADDR_IP     *dest;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    SCK6_SOCKADDR_IP    *dest_v6;
#endif

    /* Init */
    tval = 0;

    /* Try to make a new port. */
    pnum = TCP_Make_Port(family, serv);

    /* If make port failed we will try to reuse one that is in the
     * time wait state and has timed out.
     */
    if (pnum < 0)
    {
        /* Get the current clock value. */
        tmnc = (INT32)NU_Retrieve_Clock();

        /* Loop through the port list checking for timed out ports.
         * Note we will use the oldest timed out port.
         */
        for (i = 0; (!(i >= TCP_MAX_PORTS) && (TCP_Ports[i] != NU_NULL)); i++)
        {
            /* Is this port in the time wait state. */
            if (TCP_Ports[i]->state == STWAIT)
            {
                /* Compare the timeout time and the current time. */
                tmp = (INT32)(labs(INT32_CMP(tmnc, TCP_Ports[i]->out.lasttime)));

                /* Has this port timed out. */
                if (tmp > tval)
                {
                    /* Save this ports time value. This is used in finding
                     * the oldest timed out port.
                     */
                    tval = tmp;

                    /* Store it position for use below. */
                    pnum = i;

                } /* end if the port timed out */

            } /* end if the port is in the time wait state */

        } /* end loop search through the ports */

        /* If we found a port to reuse, clean up and try make port
           again. We should have a port structure ready. */
        if (pnum >= 0)
        {
            /* Remove the pending TIMEWAIT timer when reusing
               this port. */
            TQ_Timerunset(TCPTIMEWAIT, TQ_CLEAR_EXACT, (UNSIGNED)pnum, 0);

            /* Change the state to closed so TCP_Make_Port will reuse it. */
            TCP_Ports[pnum]->state = SCLOSED;

            /* Try to make a new port. */
            pnum = TCP_Make_Port(family, serv);

        }

        /* Were we unable to find or make an available port. */
        if (pnum < 0)
        {
            NLOG_Error_Log("Unable to find a timed out TCP port",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            return (-2);
        }
    } /* end if there was no port returned from TCP_Make_Port */

    prt = TCP_Ports[pnum];

#if (INCLUDE_IPV6 == NU_TRUE)

    if (family == SK_FAM_IP6)
    {
        dest_v6 = &prt->tp_routev6.rt_ip_dest.rtab6_rt_ip_dest;
        dest_v6->sck_family = SK_FAM_IP6;
        dest_v6->sck_len = sizeof(*dest_v6);

        NU_BLOCK_COPY(dest_v6->sck_addr,
                      ((struct pseudohdr*)(tcp_check))->source,
                      IP6_ADDR_LEN);

        /* Try to find a route to the destination */
        IP6_Find_Route(&prt->tp_routev6);

        /* If a route could not be found, set the state as CLOSED and
         * return an error.
         */
        if (prt->tp_routev6.rt_route == NU_NULL)
        {
            prt->state = SCLOSED;
            return (-1);
        }

        NU_BLOCK_COPY(prt->tcp_laddrv6,
                      ((struct pseudohdr*)(tcp_check))->dest,
                      IP6_ADDR_LEN);
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* Point to the destination. */
        dest = (SCK_SOCKADDR_IP *)&prt->tp_route.rt_ip_dest;
        dest->sck_family = SK_FAM_IP;
        dest->sck_len = sizeof(*dest);

        dest->sck_addr = LONGSWAP(((struct pseudotcp*)(tcp_check))->source);
        IP_Find_Route(&prt->tp_route);

        if (prt->tp_route.rt_route == NU_NULL)
        {
            prt->state = SCLOSED;
            return (-1);
        }

        prt->tcp_laddrv4 = LONGSWAP(((struct pseudotcp*)(tcp_check))->dest);
    }
#endif

    /* Create the socket that will be used by the application that accepts
     * this connection.
     */
    prt->p_socketd = SCK_Create_Socket(NU_PROTO_TCP, family);

    /* Trace log */
    T_SOCK_LIST(family, NU_TYPE_STREAM, NU_PROTO_TCP, prt->p_socketd);

    if (prt->p_socketd < 0)
    {
        prt->state = SCLOSED;
        return (-2);
    }

    /* Store the port index. */
    SCK_Sockets[prt->p_socketd]->s_port_index = pnum;

    prt->in.port = serv;
    prt->out.port = 0;                              /* accept any outside port # */
    prt->in.lasttime = NU_Retrieve_Clock();  /* set time we started */

    prt->state  = SLISTEN;
    prt->credit = WINDOW_SIZE;           /* default value until changed */

    return (pnum);

} /* TCPSS_Net_Listen */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Net_Xopen
*
*   DESCRIPTION
*
*       Open a network socket for the user to *machine* using
*       port *service*.  The rest of the parameters are self-explanatory.
*
*   INPUTS
*
*       *machine                Pointer to the address of the machine
*       server_family           The family type of the server IP address
*       service                 The port number
*       socketd                 The socket descriptor
*
*   OUTPUTS
*
*       pnum
*       NU_NO_PORT_NUMBER
*       -1
*       NU_INVALID_ADDRESS
*       NU_INVALID_SOCKET
*       NU_NO_ROUTE_TO_HOST
*
*************************************************************************/
STATUS TCPSS_Net_Xopen(const UINT8 *machine, INT16 server_family, UINT16 service,
                       INT socketd)
{
    TCP_PORT            *prt;
    INT                 pnum;
    INT16               family;

#if (INCLUDE_IPV4 == NU_TRUE)
    DV_DEVICE_ENTRY     *device;
    SCK_SOCKADDR_IP     *dest;
    UINT32              machine_ip;
    UINT8               foreign_ip[IP_ADDR_LEN];
    UINT8               local_ip[IP_ADDR_LEN];
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    SCK6_SOCKADDR_IP    *dest_v6;
    UINT8               *src_addr;
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )

    machine_ip = 0;

    if ( (server_family == SK_FAM_IP) ||
         (SCK_Sockets[socketd]->s_flags & SF_V4_MAPPED) )
#else
    UNUSED_PARAMETER(server_family);
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* Set the family type of the port to be created */
        family = NU_FAMILY_IP;

        machine_ip = IP_ADDR(machine);

        /* If this is a class C ip address */
        if (IP_CLASSC_ADDR(machine_ip))
        {
            /*
             *  check the IP number and don't allow broadcast addresses
             */
            if (machine[3] == 255)
            {
                NLOG_Error_Log ("Trying to open a socket with a broadcast address",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);

                return (NU_INVALID_ADDRESS);
            }
        }
        /* If this is a class B ip address */
        else if (IP_CLASSB_ADDR(machine_ip))
        {
            /*
             *  check the IP number and don't allow broadcast addresses
             */
            if ( (machine[2] == 255) && (machine[3] == 255) )
            {
                NLOG_Error_Log ("Trying to open a socket with a broadcast address",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);

                return (NU_INVALID_ADDRESS);
            }
        }
        /* If this is a class A ip address */
        else if (IP_CLASSA_ADDR(machine_ip))
        {
            /*
             *  check the IP number and don't allow broadcast addresses
             */
            if ( (machine[1] == 255) && (machine[2] == 255) &&
                 (machine[3] == 255) )
            {
                NLOG_Error_Log ("Trying to open a socket with a broadcast address",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);

                return (NU_INVALID_ADDRESS);
            }
        }
        /* If this is a full Broadcast or old style broadcast or multicast address */
        else if ( (machine_ip == IP_ADDR_BROADCAST) || (machine_ip == 0) ||
                  ((machine[0] & 0xE0) == 0xE0) )
        {
            NLOG_Error_Log ("Trying to open a socket with an invalid address",
                                NERR_RECOVERABLE, __FILE__, __LINE__);

            return (NU_INVALID_ADDRESS);
        }

    }
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        family = NU_FAMILY_IP6;
#endif

    /* Get a pointer to the port structure */
    if (SCK_Sockets[socketd]->s_port_index != -1)
        prt = TCP_Ports[SCK_Sockets[socketd]->s_port_index];
    else
        return (NU_INVALID_SOCKET);

    /* Determine which device will be used for communication.  This will allow
     * us to decide which IP address to use on the local side.
     */

    /* The TCP port structure includes a route field used to cache a route
     * to the foreign host.  Fill it in here. This will save us the trouble of
     * looking up the route at the IP layer for every packet sent.
     */

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (family == SK_FAM_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    {
        dest_v6 = &prt->tp_routev6.rt_ip_dest.rtab6_rt_ip_dest;
        dest_v6->sck_family = SK_FAM_IP6;
        dest_v6->sck_len = sizeof(*dest_v6);

        NU_BLOCK_COPY(dest_v6->sck_addr, machine, IP6_ADDR_LEN);

        /* Find a route to the destination */
        IP6_Find_Route(&prt->tp_routev6);

        /* If a route does not exist, return an error. */
        if (prt->tp_routev6.rt_route == NU_NULL)
        {
            /* Increment the number of packets that could not be delivered. */
            MIB2_ipOutNoRoutes_Inc;

            return (NU_NO_ROUTE_TO_HOST);
        }

        /* Copy the foreign address */
        NU_BLOCK_COPY(prt->tcp_faddrv6, machine, IP6_ADDR_LEN);

        /* If a specific address was not bound to, select an address of the
         * same scope as the destination.
         */
        if ( (!(SCK_Sockets[prt->p_socketd]->s_flags & SF_BIND)) ||
             (IP_ADDR(SCK_Sockets[prt->p_socketd]->s_local_addr.ip_num.is_ip_addrs)
              == IP_ADDR_ANY) )
        {
            src_addr =
                in6_ifawithifp(prt->tp_routev6.rt_route->rt_entry_parms.rt_parm_device,
                               prt->tcp_faddrv6);

            /* If an address of the same scope does not exist, return an error */
            if (src_addr)
                NU_BLOCK_COPY(prt->tcp_laddrv6, src_addr, IP6_ADDR_LEN);
            else
                return (-1);
        }

        /* Otherwise, use the bound-to address as the source address of
         * the packet.
         */
        else
        {
            NU_BLOCK_COPY(prt->tcp_laddrv6,
                          SCK_Sockets[prt->p_socketd]->s_local_addr.ip_num.is_ip_addrs,
                          IP6_ADDR_LEN);
        }

        /* If this socket was bound to a specific port, check that the
         * 4-tuple represents a unique connection.
         */
        if (SCK_Sockets[socketd]->s_flags & SF_BIND)
        {
            /* If the 4-tuple does not represent a unique connection, return
             * an error.
             */
            if (TCPSS_Is_Unique_Connection(prt->tcp_laddrv6,
                                           prt->in.port, machine,
                                           service, family) == NU_FALSE)
                return (NU_INVALID_ADDRESS);
        }
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        dest = (SCK_SOCKADDR_IP *) &prt->tp_route.rt_ip_dest;
        dest->sck_family = SK_FAM_IP;
        dest->sck_len = sizeof(*dest);
        dest->sck_addr = machine_ip;

#if (INCLUDE_IPV6 == NU_TRUE)
        /* If the IPv6 flag is set in the port structure, the application
         * thinks it is communicating with an IPv6 node, but it is actually
         * not.  Communications must be made via IPv4 but appear to the
         * application to be IPv6.
         */
        if (prt->portFlags & TCP_FAMILY_IPV6)
        {
            /* Remove the IPv6 flag */
            prt->portFlags &= ~TCP_FAMILY_IPV6;

            /* Flag as IPv4 */
            prt->portFlags |= TCP_FAMILY_IPV4;
        }
#endif

        IP_Find_Route(&prt->tp_route);

        if (prt->tp_route.rt_route == NU_NULL)
        {
            /* Increment the number of packets that could not be delivered. */
            MIB2_ipOutNoRoutes_Inc;

            return (NU_NO_ROUTE_TO_HOST);
        }

        device = prt->tp_route.rt_route->rt_entry_parms.rt_parm_device;

        /* If a specific address was not bound to, use the primary address of
         * the interface associated with the route as the source address of
         * the packet.
         */
        if ( (!(SCK_Sockets[prt->p_socketd]->s_flags & SF_BIND)) ||
             (IP_ADDR(SCK_Sockets[prt->p_socketd]->s_local_addr.ip_num.is_ip_addrs)
              == IP_ADDR_ANY) )
            prt->tcp_laddrv4 = device->dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr;

        /* Otherwise, use the bound-to address as the source address of
         * the packet.
         */
        else
            prt->tcp_laddrv4 =
                IP_ADDR(SCK_Sockets[prt->p_socketd]->s_local_addr.ip_num.is_ip_addrs);

        /* If this socket was bound to a specific port, check that the
         * 4-tuple represents a unique connection.
         */
        if (SCK_Sockets[socketd]->s_flags & SF_BIND)
        {
            PUT32(foreign_ip, 0, machine_ip);
            PUT32(local_ip, 0, prt->tcp_laddrv4);

            /* If the 4-tuple does not represent a unique connection, return
             * an error.
             */
            if (TCPSS_Is_Unique_Connection(local_ip, prt->in.port, foreign_ip,
                                           service, family) == NU_FALSE)
                return (NU_INVALID_ADDRESS);
        }

        prt->tcp_faddrv4 = machine_ip;
    }
#endif

    pnum = SCK_Sockets[socketd]->s_port_index;

    /* Make the connection, if you can, we will get an event notification
     * later if it connects.  Timeouts must be done at a higher layer.
     */

    /* If the connection was made return the index into the TCP_Ports of
       the new port.  Else return failure. */
    if (TCPSS_Do_Connect(pnum, service) == NU_SUCCESS)
        return (pnum);
    else
    {
        TCP_Cleanup(prt);
        return (-1);
    }

} /* TCPSS_Net_Xopen */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Do_Connect
*
*   DESCRIPTION
*
*       This routine sends the actual packet out to try and establish a
*       connection.
*
*   INPUTS
*
*       pnum                    The index number for the port array
*       service                 The port number
*
*   OUTPUTS
*
*       Nucleus Status Code
*
*************************************************************************/
STATIC STATUS TCPSS_Do_Connect(INT pnum, UINT16 service)
{
    TCP_PORT        *prt;
    STATUS          status;

    /* Get a pointer to the port. */
    prt = TCP_Ports[pnum];

    prt->out.port = service;                   /* service same as port num */
    prt->out.tcp_flags = TSYN;                 /* want to start up sequence */

    prt->state = SSYNS;

    status = TCPSS_Send_SYN_FIN(prt);

    if (status == NU_SUCCESS)
    {
        /* Increment the number of TCP segments transmitted. */
        MIB2_tcpOutSegs_Inc;

        /* Increment the number active connections attempted. */
        MIB2_tcpActiveOpens_Inc;
    }

    /* The current state of the port is SSYNS.  Change it to SCLOSED so
     * it can be reused.
     */
    else
        prt->state = SCLOSED;

    return (status);

} /* TCPSS_Do_Connect */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Send_SYN_FIN
*
*   DESCRIPTION
*
*       This routine is responsible for sending a packet containing either
*       a SYN or FIN bit.
*
*   INPUTS
*
*       *prt                    Pointer to the tcp port information
*
*   OUTPUTS
*
*       stat
*       -1
*
*************************************************************************/
STATUS TCPSS_Send_SYN_FIN(TCP_PORT *prt)
{
    TCPLAYER    *tcp_ptr;
    NET_BUFFER  *buf_ptr;
    INT         tcp_hlen = TCP_HEADER_LEN;
    INT16       mss;
    STATUS      stat;
    TCP_BUFFER  *tcp_buf;
    UINT8       opt_len = TCP_TOTAL_OPT_LENGTH;

#if (INCLUDE_IPV6 == NU_TRUE)
    IP6_S_OPTIONS       ip6_options;
#endif

    /* One buffer will be large enough for the SYN FIN */
    buf_ptr = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

    if (buf_ptr == NU_NULL)
        return (-1);

#if (INCLUDE_IPSEC == NU_TRUE)

    /* Set the TCP port pointer. This will enable IPsec to use cached
     * information in the port structure when adding its IPsec headers.
     */
    buf_ptr->mem_port = prt;

#endif

    /* Initialize each field in the buffer. */

    /* Point the data pointer to the beginning of the packet */
    buf_ptr->data_ptr = buf_ptr->mem_parent_packet;

    /* Set the TCP data length to one. This is used for ack and seqnum
       comparison. A SYN or FIN only bumps the seqnum by 1. */
    buf_ptr->mem_tcp_data_len = 1;

    /* Store the sequence number of this packet. */
    buf_ptr->mem_seqnum = prt->out.nxt;

    /* Initialize the list that this packet will be deallocated to when
     * transmission is complete.
     */
    buf_ptr->mem_dlist = NU_NULL;

    /* Get a pointer to a free TCP buffer */
    tcp_buf = DLL_Dequeue(&TCP_Buffer_List);

    /* Set the buffer pointer to the new buffer */
    tcp_buf->tcp_buf_ptr = buf_ptr;

    /* Put the TCP buffer on the retransmission list */
    DLL_Enqueue(&prt->out.packet_list, tcp_buf);

    /* Flag this buffer as a parent so all subsequent packets in the chain
     * are freed to the same dlist, and set the NET_TX_QUEUE flag to indicate
     * that this packet is currently being transmitted by the hardware.
     */
    buf_ptr->mem_flags = (NET_PARENT | NET_TX_QUEUE);

    buf_ptr->mem_port_index = prt->pindex;

    /* Initialize the number of times this packet has been retransmitted. */
    buf_ptr->mem_retransmits = 0;

    /* Update the number of packets in this port. */
    prt->out.num_packets++;

    /* Update the amount of data in this port. */
    prt->out.contain++;

    /* If this is a SYN packet, build the appropriate options. */
    if (prt->out.tcp_flags & TSYN)
    {
#if (INCLUDE_IPV6 == NU_TRUE)

        if (prt->portFlags & TCP_FAMILY_IPV6)
            mss =
                (INT16)((UINT16)(prt->tp_route.rt_route->rt_entry_parms.rt_parm_device->dev_mtu -
                (IP6_HEADER_LEN +
#if (INCLUDE_IPV4 == NU_TRUE)
                 IP_HEADER_LEN +
#endif
                 TCP_HEADER_LEN + prt->p_opt_len)));

#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
            mss =
                (INT16)((UINT16)(prt->tp_route.rt_route->rt_entry_parms.rt_parm_device->dev_mtu -
                (IP_HEADER_LEN + TCP_HEADER_LEN + prt->p_opt_len)));
#endif

        /* Build the MSS Option. */
        tcp_hlen += TCP_Build_MSS_Option(&buf_ptr->data_ptr[tcp_hlen], mss, &opt_len);

#if (NET_INCLUDE_SACK == NU_TRUE)

        /* If the SACK option is enabled on this socket, include a
         * SACK-Permitted Option in the packet.
         */
        if (prt->portFlags & TCP_SACK)
        {
            TEST_TCP_SACK_Perm_Included(prt);

            /* Add the SACK Permitted Option. */
            tcp_hlen +=
                TCP_Build_SACK_Perm_Option(&buf_ptr->data_ptr[tcp_hlen], &opt_len);
        }
#endif

#if (NET_INCLUDE_WINDOWSCALE == NU_TRUE)

        /* If the Window Scale option is enabled on this socket, include a
         * Window Scale Option in the packet.
         */
        if (prt->portFlags & TCP_REPORT_WINDOWSCALE)
        {
            /* Add the Window Scale Option. */
            tcp_hlen +=
                TCP_Build_WindowScale_Option(&buf_ptr->data_ptr[tcp_hlen], prt, &opt_len);
        }
#endif

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

        /* If the Timestamp option is enabled on this socket, include a
         * Timestamp Option in the packet.
         */
        if (prt->portFlags & TCP_REPORT_TIMESTAMP)
        {
            /* Add the Timestamp Option. */
            tcp_hlen +=
                TCP_Build_Timestamp_Option(&buf_ptr->data_ptr[tcp_hlen], prt, &opt_len);
        }
#endif
    }

    /* Update the header information. */
    TCP_Update_Headers(prt, buf_ptr, (UINT16)tcp_hlen);

    /* A SYN or FIN flag counts as one byte of data in the sequence space. */
    prt->out.nxt++;

    /* Get a pointer to the start of the TCP header. */
    tcp_ptr = (TCPLAYER *)buf_ptr->data_ptr;

#if ((INCLUDE_TCP_INFO_LOGGING == NU_TRUE) || (PRINT_TCP_MSG == NU_TRUE))
    /* Print the TCP header info */
    NLOG_TCP_Info (tcp_ptr, (INT16)buf_ptr->data_len, NLOG_TX_PACK);
#endif

    /* If there is no timing being performed then time this transmission. */
    if (prt->p_rtt == 0)
    {
        prt->p_rtt = NU_Retrieve_Clock();
        prt->p_rtseq = buf_ptr->mem_seqnum;
    }

    /* Compute and fill in the checksum. */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (prt->portFlags & TCP_FAMILY_IPV6)
    {
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));

        /* Set up the parameters to be used for the transmission */
        stat = IP6_Setup_Options(NU_NULL, prt->p_sticky_options,
                                 &ip6_options, prt->tcp_faddrv6,
                                 &prt->tp_routev6, (UINT8)prt->p_ttl);

        if (stat != NU_SUCCESS)
            return (stat);

        ip6_options.tx_source_address = prt->tcp_laddrv6;

#if (HARDWARE_OFFLOAD == NU_TRUE)
        /* Do not perform checksum in software if hardware controller can do
         * it
         */
        if ((prt->tp_routev6.rt_route)&& (!(prt->tp_routev6.rt_route->rt_entry_parms.rt_parm_device->
              dev_hw_options_enabled & HW_TX_TCP6_CHKSUM)))
#endif
        {
            PUT16(tcp_ptr, TCP_CHECK_OFFSET,
                  UTL6_Checksum(buf_ptr, ip6_options.tx_source_address,
                                prt->tcp_faddrv6, buf_ptr->mem_total_data_len,
                                IP_TCP_PROT, IP_TCP_PROT) );
        }

        ip6_options.tx_dest_address = prt->tcp_faddrv6;

        stat = IP6_Send(buf_ptr, &ip6_options, IP_TCP_PROT, &ip6_options.tx_route,
                        0, NU_NULL);
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    }
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
#if (HARDWARE_OFFLOAD == NU_TRUE)
        /* Do not compute checksum in software in hardware controller can
         * do it
         */
        if ((prt->tp_route.rt_route)&& (!(prt->tp_route.rt_route->rt_entry_parms.rt_parm_device->
              dev_hw_options_enabled & HW_TX_TCP_CHKSUM)))
#endif
        {
            PUT16(tcp_ptr, TCP_CHECK_OFFSET,
                  UTL_Checksum(buf_ptr, prt->tcp_laddrv4, prt->tcp_faddrv4,
                               IP_TCP_PROT) );
        }

        /* Send this packet. */
        stat = IP_Send((NET_BUFFER *)buf_ptr, &prt->tp_route, prt->tcp_faddrv4,
                       prt->tcp_laddrv4, 0, prt->p_ttl,
                       IP_TCP_PROT, prt->p_tos, NU_NULL);
    }
#endif

    if (stat == NU_SUCCESS)
    {
        /* If this is the first buffer on the retransmission list, set
         * the timer to retransmit the packet.
         */
        if (prt->out.packet_list.tcp_head->tcp_buf_ptr == buf_ptr)
        {
            if (TQ_Timerset(TCPRETRANS, (UNSIGNED)prt->pindex,
                            prt->p_rto, 0) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to set timer for RETRANSMIT", NERR_SEVERE,
                               __FILE__, __LINE__);

                NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);
            }
        }

        /* Increment the number of TCP segments transmitted. */
        MIB2_tcpOutSegs_Inc;
    }
    else
    {
        /* Remove the TCP Buffer from the retransmission list. */
        DLL_Remove(&prt->out.packet_list, tcp_buf);

        /* Put the TCP Buffer element back on the list of free elements */
        DLL_Enqueue(&TCP_Buffer_List, tcp_buf);

        /* The packet was not sent.  Deallocate the buffer.  If the packet was
         * transmitted it will be deallocated later by TCP.
         */
        MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);
    }

    return (stat);

} /* TCPSS_Send_SYN_FIN */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Net_Close
*
*   DESCRIPTION
*
*       Start the closing process on port pnum.
*
*   INPUTS
*
*       pnum                    Number of the port to close.
*       *sock_ptr               Pointer to the socket list
*
*   OUTPUTS
*
*       NU_SUCCESS
*       -1
*
*************************************************************************/
STATUS TCPSS_Net_Close(INT pnum, struct sock_struct *sock_ptr)
{
    TCP_PORT    *prt;
    STATUS      return_status = -1;
    NET_BUFFER  *buf_ptr;

    /* Validate pnum before indexing into the TCP port list. */
    if (pnum >= 0)
        prt = TCP_Ports[pnum];
    else
        prt = NU_NULL;

    if (prt != NU_NULL)
    {
        /* something there */

        switch (prt->state)
        {
            case SLISTEN:               /* we don't care anymore */
            case SSYNS:

                /* Increment the number of connection failures. */
                MIB2_tcpAttemptFails_Inc;

                /* Upon returning the socket will be released. So break
                   the port's link to this socket. */
                prt->p_socketd = -1;

                /* Close the connection. */
                prt->state = SCLOSED;

                TCP_Cleanup(prt);

                return_status = NU_SUCCESS;

                break;

            case SSYNR:

                prt->out.tcp_flags = TACK | TFIN;

                prt->state = SFW1;

                TCPSS_Send_SYN_FIN (prt);

                return_status = NU_SUCCESS;

                break;

            case SEST:                  /* must initiate close */

                return_status = TCPSS_Net_Close_EST(prt);

                break;                      /* do nothing for now ?*/

            case SCWAIT:                    /* other side already closed */

                /* Send the FIN. */
                if (prt->out.nextPacket == NU_NULL)
                {
                    prt->out.tcp_flags = TFIN | TACK;

                    TCPSS_Send_SYN_FIN(prt);
                }
                else
                {
                    if (prt->xmitFlag == NU_SET)
                    {
                        TQ_Timerunset(CONTX, TQ_CLEAR_EXACT, (UNSIGNED)prt->pindex, 0);

                        prt->out.tcp_flags = TPUSH | TACK | TFIN;

                        /* Since we are setting the FIN bit we must
                           bump the tcp_data_len for this packet. This
                           length is used during seq number and ack
                           comparison. */
                        (prt->out.nextPacket->mem_tcp_data_len)++;

                        /* Save a pointer to the next packet to send. Then move
                           the next pointer forward. nextPacket should always
                           be NULL after these steps. */
                        buf_ptr = prt->out.nextPacket;
                        prt->out.nextPacket = prt->out.nextPacket->next;

                        /* Clear the event flag. */
                        prt->xmitFlag = NU_CLEAR;

                        /* Send the buffer. */
                        if (TCP_Xmit(prt, buf_ptr) < 0)
                            NLOG_Error_Log("Failed to transmit buffer", NERR_SEVERE,
                                           __FILE__, __LINE__);
                    }
                }

                /* Deallocate any data that is in the in window. */
                if (sock_ptr->s_recvbytes)
                {
                    MEM_Buffer_Cleanup(&sock_ptr->s_recvlist);
                }

                /* Update the connection state. */
                prt->state = SLAST;

                return_status = -1;

                break;

            case STWAIT:                    /* time out yet? */

                if (INT32_CMP((TCP_Ports[pnum]->out.lasttime + TCP_Ports[pnum]->p_msl),
                              NU_Retrieve_Clock()) < 0)
                {
                    prt->state = SCLOSED;
                }

                /* Upon returning the socket will be released. So break
                   the port's link to this socket. */
                prt->p_socketd = -1;

                return_status = NU_SUCCESS;

                break;

            case SLAST:

                 /* If the state is SLAST, then a FIN has already been sent.  We
                    are waiting for an ACK.  If one is not received the
                    retransmit timeout will eventually close this connection.
                 */

                 return_status = -1;

                 break;

            case SREADY:

                 /* Set the state to CLOSED so this port will be reused */
                 prt->state = SCLOSED;

            case SCLOSING:
            case SCLOSED:

                 /* Upon returning the socket will be released. So break
                   the port's link to this socket. */
                 prt->p_socketd = -1;

                 return_status = NU_SUCCESS;

                 break;

            default:

                break;
        }
    }
    else
    {
        return (1);
    }

    return (return_status);

} /* TCPSS_Net_Close */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Net_Close_EST
*
*   DESCRIPTION
*
*       This routine closes from the established state.
*
*   INPUTS
*
*       prt                     Pointer to the TCP port structure
*
*   OUTPUTS
*
*       NU_SUCCESS
*       -1
*
*************************************************************************/
STATIC STATUS TCPSS_Net_Close_EST(TCP_PORT *prt)
{
    NET_BUFFER  *buf_ptr;
    STATUS      status;

    /* Is there data to be transmitted. */
    if (prt->out.nextPacket == NU_NULL)
    {
        if (prt->portFlags & ACK_TIMER_SET)
        {
            /*  Delete the ACK timeout timer.  */
            TQ_Timerunset (TCPACK, TQ_CLEAR_EXACT, (UNSIGNED)prt->pindex, 0);

            /* Clear the ACK timer flag in the port. */
            prt->portFlags &= (~ACK_TIMER_SET);
        }

        prt->out.tcp_flags = TACK | TFIN;
        prt->state = SFW1;           /* wait for ACK of FIN */
        TCPSS_Send_SYN_FIN (prt);

        /* If the linger option is set to a value greater than zero,
         * do not return success.
         */
        if ( (prt->p_socketd >= 0) &&
             (SCK_Sockets[prt->p_socketd] != NU_NULL) &&
             (SCK_Sockets[prt->p_socketd]->s_linger.linger_on == NU_TRUE) &&
             (SCK_Sockets[prt->p_socketd]->s_linger.linger_ticks > 0) )
        {
            status = -1;
        }

        else
        {
            status = NU_SUCCESS;
        }
    }

    else
    {
        if (prt->xmitFlag == NU_SET)
        {
            TQ_Timerunset(CONTX, TQ_CLEAR_EXACT, (UNSIGNED)prt->pindex, 0);

            prt->out.tcp_flags = TPUSH | TACK | TFIN;

            /* Since we are setting the FIN bit we must
               bump the tcp_data_len for this packet. This
               length is used during seq number and ack
               comparison. */
            (prt->out.nextPacket->mem_tcp_data_len)++;

            /* Save a pointer to the next packet to send. Then move
               the next pointer forward. nextPacket should always
               be NULL after these steps. */
            buf_ptr = prt->out.nextPacket;
            prt->out.nextPacket = prt->out.nextPacket->next;

            /* Clear the event flag. */
            prt->xmitFlag = NU_CLEAR;

            /* Send the completed Packet. */
            if (TCP_Xmit(prt, buf_ptr) < 0)
                NLOG_Error_Log("Failed to transmit buffer", NERR_SEVERE,
                               __FILE__, __LINE__);
        }

        prt->state = SFW1;           /* wait for ACK of FIN */

        status = -1;
    }

    return (status);

} /* TCPSS_Net_Close_EST */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Window_Probe
*
*   DESCRIPTION
*
*       This routine will set the timer to transmit the initial Window
*       Probe packet and set up the elements of the port structure
*       to indicate that the port is currently probing the foreign
*       host.
*
*   INPUTS
*
*       *prt                    Pointer to a port.
*
*   OUTPUTS
*
*       Upon successful completion, this routine will return NU_SUCCESS.
*       Otherwise, the routine returns -1.
*
*************************************************************************/
STATIC STATUS TCPSS_Window_Probe(TCP_PORT *prt)
{
    STATUS      status;

    /* Set the initial timer to transmit the first Window Probe packet
     * if the window doesn't open up.
     */
    if (TQ_Timerset(WINPROBE, (UNSIGNED)prt->pindex,
                    (UNSIGNED)prt->p_first_probe_to, 0) == NU_SUCCESS)
    {
        /* Set the probe timeout to the initial Window Probe timeout value
         * so we can properly back it off for retransmissions.
         */
        prt->p_probe_to = prt->p_first_probe_to;

        /* Set the probe count to zero. */
        prt->p_probe_count = 0;

        /* Indicate that we are currently probing the other side of
         * the connection - don't send any more data until the window
         * opens up.
         */
        prt->probeFlag = NU_SET;

        status = NU_SUCCESS;
    }

    else
        status = -1;

    return (status);

} /* TCPSS_Window_Probe */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Send_Window_Probe
*
*   DESCRIPTION
*
*       This routine will transmit a Window Probe packet, compute the
*       next retransmission value, and reset the timer to transmit the
*       next Window Probe packet.
*
*   INPUTS
*
*       pindex                  The port index value into the port
*                               structure for which to transmit the
*                               Window Probe.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID TCPSS_Send_Window_Probe(UINT16 pindex)
{
    TCP_PORT    *prt = TCP_Ports[pindex];
    STATUS      status;

    /* If the port is still valid and we are still probing the other side
     * of the connection.
     */
    if ( (prt != NU_NULL) && (prt->probeFlag == NU_SET) &&
         (prt->out.size == 0) )
    {
        /* If the maximum number of probes has not been transmitted. */
        if (prt->p_probe_count < prt->p_max_probes)
        {
            /* Only send the probe if the retransmission timer is not running.
             * Otherwise, the retransmission will serve to probe the window.
             * The window probe service still needs to remain active so the
             * send routine will be restarted when the window opens due to
             * a retransmission being ACKed or if the retransmissions do
             * not open up the window and a window probe message needs to
             * be sent to avoid deadlock.
             */
            if (prt->out.nxt == prt->out.ack)
            {
                /* Send the packet */
                status = TCP_Xmit_Probe(prt);

                /* If the packet could not be sent, log an error and reset the
                 * timer to transmit the next Window Probe.
                 */
                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to transmit the Window Probe",
                                   NERR_SEVERE, __FILE__, __LINE__);

                    NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }

                /* Increment the number of Zero Window probes transmitted. */
                prt->p_probe_count ++;

                /* Compute the next timeout according to the exponential backoff */
                prt->p_probe_to = (prt->p_probe_to << 1);

                /* The Window Probe delay is bound by prt->p_max_probe_to */
                if (prt->p_probe_to > prt->p_max_probe_to)
                    prt->p_probe_to = prt->p_max_probe_to;
            }

            /* Set the timer to expire again. */
            if (TQ_Timerset(WINPROBE, (UNSIGNED)prt->pindex,
                            (UNSIGNED)prt->p_probe_to, 0) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to set timer to retransmit Window Probe",
                               NERR_SEVERE, __FILE__, __LINE__);

                NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);
            }
        }

        /* Otherwise, give up probing and close the connection. */
        else
        {
            /* Set the socket error indicating that the connection timed out */
            if (SCK_Sockets[prt->p_socketd])
            {
                SCK_Sockets[prt->p_socketd]->s_error = NU_CONNECTION_TIMED_OUT;

                /* Set the state of the socket to indicate that the connection
                 * has timed out.
                 */
                SCK_Sockets[prt->p_socketd]->s_state |= SS_TIMEDOUT;
            }

            NLOG_Error_Log("Max number of Zero Window probes have been sent",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            if ( (prt->state == SCWAIT) || (prt->state == SEST) )
            {
                MIB2_tcpEstabResets_Inc;
            }

            /* Send a reset to let the other side know we are closing. */
            prt->out.tcp_flags = TRESET;
            TCP_ACK_It(prt, 1);

            /* Mark the socket as disconnecting. */
            if (SCK_Sockets[prt->p_socketd])
            {
                SCK_DISCONNECTING(prt->p_socketd);
            }

            /* Abort this connection. */
            prt->state = SCLOSED;

            /* The connection is  closed.  Cleanup. */
            TCP_Cleanup(prt);
        }
    }

} /* TCPSS_Send_Window_Probe */

/************************************************************************
*
*   FUNCTION
*
*       TCPSS_Net_Send
*
*   DESCRIPTION
*
*       This function examines the port's outwindow and sends the data
*       that is queued in the window's packet_list.  All of the data may
*       not be sent immediately.  The decision to send data immediately
*       or to hold off is based on the Nagle Algorithm.
*
*   INPUTS
*
*       *prt                    Pointer to a port.
*       *buf_ptr                NET buffer.
*
*   OUTPUTS
*
*       The number of bytes that were sent.
*
*************************************************************************/
STATIC STATUS TCPSS_Net_Send(TCP_PORT *prt, NET_BUFFER *buf_ptr)
{
    STATUS          nsent, status;
    TCP_WINDOW      *wind;

    wind = &prt->out;

    /* Update the clock value for the last data packet sent */
    prt->out.lasttime = NU_Retrieve_Clock();

    /* Check to see if the PUSH flag has been set (i.e., the Nagle Algorithm
     * has been disabled).  If so send every packet immediately with the
     * push flag set.  Zero Copy buffers are sent immediately.
     */
    if ( (wind->push) || (SCK_Sockets[prt->p_socketd]->s_flags & SF_ZC_MODE) )
    {
        prt->out.tcp_flags |= TPUSH;

        status = TCP_Xmit(prt, buf_ptr);

        /* If the packet was successfully transmitted, return the number of
         * data bytes sent.
         */
        if (status == NU_SUCCESS)
            nsent = (STATUS)buf_ptr->mem_total_data_len;

        /* Otherwise, return the error code. */
        else
        {
            NLOG_Error_Log("Failed to transmit data in TCPSS_Net_Send",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            nsent = status;
        }
    }

    /* Should the next packet be sent?  This decision is based on the Nagle
     * Algorithm as interpreted by TCP/IP Illustrated vol. 1 section 22.3:
     *
     *  a) a full-sized segment can be sent.
     *  b) we can send at least one-half of the maximum sized window that
     *     the other end has ever advertised.
     *  c) we can send everything we have and either we are not expecting an
     *     ACK (i.e., we have no outstanding unacknowledged data) or
     *     the Nagle algorithm is disabled for this connection.
     *
     * Since we check the PUSH bit above, do not take the Nagle algorithm
     * into consideration here.
     *
     * These checks will help avoid the Silly Window Syndrome.
     */
    else if ( (buf_ptr->mem_total_data_len >= prt->p_smss) ||
              (wind->nxt == wind->ack) ||
              (buf_ptr->mem_total_data_len >= (prt->maxSendWin >> 1)) )
    {
        /* Send the packet. */
        status = TCP_Xmit(prt, buf_ptr);

        /* If the packet was successfully transmitted, return the number
         * of data bytes sent.
         */
        if (status == NU_SUCCESS)
            nsent = (UINT16)buf_ptr->mem_total_data_len;

        /* Otherwise, return the error value. */
        else
        {
            NLOG_Error_Log("Failed to send data in TCPSS_Net_Send",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            nsent = status;
        }
    }

    /* Otherwise, no data was sent */
    else
    {
        NLOG_Error_Log("No data was sent in TCPSS_Net_Send",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        nsent = 0;
    }

    /* Return the number of data bytes that were sent. */
    return (nsent);

} /* TCPSS_Net_Send */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Is_Unique_Connection
*
*   DESCRIPTION
*
*       This function determines whether the provided 4-tuple (local
*       address, foreign address, local port, foreign port) matches a
*       connection already made on the node.  This precaution is taken
*       to insure that the TIME WAIT state of a closed connection is
*       preserved.
*
*   INPUTS
*
*       *local_addr             A pointer to the local address used in
*                               the connection.
*       local_port              The local port for the connection.
*       *foreign_addr           A pointer to the foreign address used
*                               in the connection.
*       foreign_port            The foreign port for the connection.
*       family                  The family type of the two nodes
*                               communicating.
*
*   OUTPUTS
*
*       INT                     NU_TRUE or NU_FALSE
*
*************************************************************************/
STATIC INT TCPSS_Is_Unique_Connection(const UINT8 *local_addr,
                                      UINT16 local_port,
                                      const UINT8 *foreign_addr,
                                      UINT16 foreign_port, INT16 family)
{
    INT     i;

#if (INCLUDE_IPV4 == NU_TRUE)
    UINT32  local_address;
    UINT32  foreign_address;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    if (family == SK_FAM_IP6)
    {
        for (i = 0; i < TCP_MAX_PORTS; i++)
        {
            /* If all four parameters of a current connection match the
             * incoming parameters, this connection is not unique.
             */
            if ( (TCP_Ports[i]) &&
                 (memcmp(TCP_Ports[i]->tcp_laddrv6, local_addr, IP6_ADDR_LEN) == 0) &&
                 (TCP_Ports[i]->in.port == local_port) &&
                 (memcmp(TCP_Ports[i]->tcp_faddrv6, foreign_addr, IP6_ADDR_LEN) == 0) &&
                 (TCP_Ports[i]->out.port == foreign_port) )
                return (NU_FALSE);
        }
    }
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif

#else
    /* Remove compiler warning */
    UNUSED_PARAMETER(family);
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        local_address = IP_ADDR(local_addr);
        foreign_address = IP_ADDR(foreign_addr);

        for (i = 0; i < TCP_MAX_PORTS; i++)
        {
            /* If all four parameters of a current connection match the
             * incoming parameters, this connection is not unique.
             */
            if ( (TCP_Ports[i]) && (TCP_Ports[i]->tcp_laddrv4 == local_address) &&
                 (TCP_Ports[i]->in.port == local_port) &&
                 (TCP_Ports[i]->tcp_faddrv4 == foreign_address) &&
                 (TCP_Ports[i]->out.port == foreign_port) )
                return (NU_FALSE);
        }
    }
#endif

    return (NU_TRUE);

} /* TCPSS_Is_Unique_Connection */

/*************************************************************************
*
*   FUNCTION
*
*       TCPSS_Half_Close
*
*   DESCRIPTION
*
*       This routine closes the write-half of the TCP connection.
*
*   INPUTS
*
*       sockptr                 Pointer to the socket structure
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_NOT_CONNECTED        The data transfer was not completed. This
*                               probably occurred because the connection
*       NU_INVALID_PORT         The port number does not equal 0 or the
*                               port number is not unique
*
*************************************************************************/
STATUS TCPSS_Half_Close(struct sock_struct *sockptr)
{
    TCP_PORT    *prt;
    STATUS      return_status = NU_SUCCESS;

    if (sockptr->s_port_index < 0)
    {
        return (NU_INVALID_PORT);
    }

    prt = TCP_Ports[sockptr->s_port_index];

    if (prt != NU_NULL)
    {
        /* Check if we are performing a "half-duplex" close. In this
           case we need to update the port. */
        if (sockptr->s_state & SS_CANTRCVMORE)
            prt->portFlags |= TCP_HALFDPLX_CLOSE;

        /* Close the socket */
        TCPSS_Net_Close(sockptr->s_port_index, sockptr);
    }

    return (return_status);

} /* TCPSS_Half_Close */
