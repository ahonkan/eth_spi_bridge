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
*       tcp.c
*
*   DESCRIPTION
*
*       TCP level routines
*
*   DATA STRUCTURES
*
*       *TCP_Ports[]
*       TCP_Ack_Timeout
*
*   FUNCTIONS
*
*       TCP_Init
*       TCP_ACK_Check
*       TCP_Find_Duplicate_Data
*       TCP_ACK_It
*       TCP_Check_FIN
*       TCP_Parse_SYN_Options
*       TCP_Check_OOO_List
*       TCP_Cleanup
*       TCP_Do
*       TCP_Process_Invalid_TSYN
*       TCP_Enter_STWAIT
*       TCP_Estab1986
*       TCP_Find_Option
*       TCP_Find_Empty_Port
*       TCP_Handle_Datagram_Error
*       TCP_Interpret
*       TCP_Make_Port
*       TCP_OOO_Packet
*       TCP_Reset_FIN
*       TCP_Retransmit
*       TCP_Send_Retransmission
*       TCP_Send
*       TCP_Check_Timestamp
*       TCP_Send_ACK
*       TCP_Update_Headers
*       TCP_Xmit
*       TCP_Xmit_Timer
*       TCP_Enqueue
*       TCP_Dequeue
*       TCP_Rmqueue
*       TCP_Xmit_Probe
*       TCP_Check_SYN
*       TCP_Configure_Shift_Count
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       net_cfg.h
*       nu_net.h
*       net_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "networking/net_cfg.h"
#include "networking/nu_net.h"
#include "networking/net_extr.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/externs6.h"
#include "networking/nud6.h"
#include "networking/prefix6.h"
#include "networking/tcp6.h"
#endif

#if (INCLUDE_SR_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPSEC == NU_TRUE)
#include "networking/ips_externs.h"
#include "networking/ips_api.h"
#endif

/* Local Prototypes */
STATIC  VOID  TCP_Check_FIN(TCP_PORT *, TCPLAYER *);
STATIC  INT16 TCP_Estab1986(TCP_PORT *, NET_BUFFER *, UINT16, UINT16);
STATIC  INT16 TCP_ACK_Check(TCP_PORT *, TCPLAYER *);
STATIC  INT16 TCP_Reset_FIN(TCPLAYER *, const VOID *, INT16, INT16, UINT8);
STATIC  INT16 TCP_Find_Empty_Port(const struct TASK_TABLE_STRUCT *);
STATIC  STATUS  TCP_Parse_SYN_Options(TCP_PORT *, TCPLAYER *, UINT16);
STATIC  VOID    TCP_Xmit_Timer(TCP_PORT *, UINT32);
STATIC  INT16   TCP_Do_Recv (TCP_PORT *, TCPLAYER *, NET_BUFFER *, UINT16 , UINT16);
STATIC  VOID    TCP_Process_Invalid_TSYN(TCP_PORT *, TCPLAYER *);
STATIC  VOID    TCP_Enter_STWAIT(TCP_PORT *);
STATIC  STATUS  TCP_Send (TCP_PORT *pport, NET_BUFFER*);
STATIC  UINT16  TCP_Enqueue (struct sock_struct *);
STATIC  UINT16  TCP_Rmqueue (TCP_WINDOW *wind, UINT32);
STATIC  STATUS  TCP_Send_Retransmission(NET_BUFFER *, TCP_PORT *);
STATIC VOID     TCP_Schedule_ACK(TCP_PORT *, TCPLAYER *, UINT16, INT);

#if (INCLUDE_TCP_OOO == NU_TRUE)
STATIC  VOID    TCP_Check_OOO_List(TCP_PORT *);
STATIC  INT16   TCP_OOO_Packet(TCP_PORT *, UINT32);
#endif

#if ( (NET_INCLUDE_DSACK == NU_TRUE) && (INCLUDE_TCP_OOO == NU_TRUE) )
STATIC VOID     TCP_Find_Duplicate_Data(TCP_PORT *, UINT32, UINT16);
#endif

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)
STATIC STATUS   TCP_Check_Timestamp(TCPLAYER *, TCP_PORT *, UINT8 *);
#endif

#if (TCPSECURE_DRAFT == NU_TRUE)
STATIC STATUS   TCP_Check_SYN(VOID * , INT16);
#endif

/*
 * Define the TCP_Ports table.  This is a critical structure in Nucleus NET
 * as it maintains information about all open connections.
 */
struct _TCP_Port    *TCP_Ports[TCP_MAX_PORTS];

TQ_EVENT TCP_Keepalive_Event;

#if (INCLUDE_IPV6 == NU_TRUE)
extern UINT8   IP6_Loopback_Address[];
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for TCP ports  */
TCP_PORT    NET_TCP_Ports_Memory[TCP_MAX_PORTS];

/* Declare memory for TCP Buffer List */
TCP_BUFFER  NET_TCP_Buffer_List_Memory[MAX_BUFFERS];

extern tx_ancillary_data    NET_Sticky_Options_Memory[];
extern UINT8                NET_Sticky_Options_Memory_Flags[];

#endif

TCP_BUFFER_LIST  TCP_Buffer_List;

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Init
*
*   DESCRIPTION
*
*       Setup for TCP_Make_Port()
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
VOID TCP_Init(VOID)
{
    INT         i;
    TCP_BUFFER  *tcp_buf;

#if (INCLUDE_TCP_KEEPALIVE == NU_TRUE)
    STATUS status;
#endif

    TCP_Buffer_List.tcp_head = NU_NULL;
    TCP_Buffer_List.tcp_tail = NU_NULL;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* Set up the TCP_Buffer_List data structure.  This list will be used
     * to obtain a free TCP_BUFFER for a port's retransmission list.
     */
    if (NU_Allocate_Memory(MEM_Cached, (VOID**)&tcp_buf,
                           sizeof(TCP_BUFFER) * (MAX_BUFFERS - NET_FREE_BUFFER_THRESHOLD),
                           NU_NO_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for all TCP_BUFFERS in TCP_Buffer_List",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Otherwise, break the memory up into TCP_BUFFER elements and put
     * them on the global list.
     */
    else
#else

    /* Assign memory to the TCP buffer list */
    tcp_buf = NET_TCP_Buffer_List_Memory;

#endif

    {
        for (i = 0; i < (MAX_BUFFERS - NET_FREE_BUFFER_THRESHOLD); i++)
            DLL_Enqueue(&TCP_Buffer_List, &tcp_buf[i]);
    }

    /* Zero out the TCP port list. */
    UTL_Zero((CHAR *)TCP_Ports, sizeof(TCP_Ports));

#if (INCLUDE_TCP_KEEPALIVE == NU_TRUE)

    /* Register the function that will handle the Keep-Alive event */
    status = EQ_Register_Event(TCP_Keep_Alive, &TCP_Keepalive_Event);

    /* If the event could not be registered, log an error */
    if (status < 0)
        NLOG_Error_Log("Failed to register TCP Keep-Alive event",
                       NERR_SEVERE, __FILE__, __LINE__);
#endif

} /* TCP_Init */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Interpret
*
*   DESCRIPTION
*
*       Called when a packet comes in and passes the IP checksum and is
*       of TCP protocol type.  Check to see if we have an open connection
*       on the appropriate port and stuff it in the right buffer
*
*   INPUTS
*
*       *buf_ptr                Pointer to the net buffer
*       *tcp_chk                Pointer to the tcp information
*       family                  The family - ipv4 or ipv6
*       myport                  port number
*       hlen                    The header length
*
*   OUTPUTS
*
*       INT16                   0, 1, or 2
*
*************************************************************************/
INT16 TCP_Interpret(NET_BUFFER *buf_ptr, VOID *tcp_chk, INT16 family,
                    UINT16 myport, UINT16 hlen)
{
    INT     socketd;

    /* If we got this far, then the current state is either
     * SLISTEN or SCLOSED.  First check for listeners.
     */
    socketd = SCK_Check_Listeners(myport, family, tcp_chk);

    if (socketd != -1)
    {
        return (TCP_Do(SCK_Sockets[socketd], buf_ptr, hlen, tcp_chk, SLISTEN));
    }

    /* No matching port was found to handle this packet, reject it */

    /* Send a reset. */
    TCP_Reset_FIN((TCPLAYER*)(buf_ptr->data_ptr), tcp_chk,
                  (INT16)(buf_ptr->mem_total_data_len - hlen), family,
                  TRESET);

    /* no error message if it is a SYN */
    if (!(GET8(buf_ptr->data_ptr, TCP_FLAGS_OFFSET) & TSYN))
    {
        /* Increment the number of TCP packets received with errors. */
        MIB2_tcpInErrs_Inc;

        NLOG_Error_Log("Error in the received TCP packet", NERR_RECOVERABLE,
                       __FILE__, __LINE__);
    } /* end if t.flags & TSYN */

    /* If we have reached this point, nobody processed the packet.
     * Therefore we need to drop it by placing it back on the
     * buffer_freelist.
     */
    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

    /* return on port matches */
    return (1);

} /* TCP_Interpret */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Do
*
*   DESCRIPTION
*
*       Deliver the incoming packet.
*
*   INPUTS
*
*       *input                  Pointer to the input
*       *buf_ptr                Pointer to the net buffer list
*       hlen                    The header length
*       *tcp_chk                The pointer to the tcp information
*       state                   The stats of the socket
*
*   OUTPUTS
*
*       INT16                   0 or 1
*
*************************************************************************/
INT16 TCP_Do(VOID *input, NET_BUFFER *buf_ptr, UINT16 hlen, VOID *tcp_chk,
             INT16 state)
{
    struct TASK_TABLE_STRUCT    *task_entry;
    struct SCK_TASK_ENT         *ssp_task;
    UINT16                      myport;
    INT                         tasklist_num;
    INT                         portlist_num;
    TCPLAYER                    *p;
    UINT16                      tlen;
    UINT8                       flags;
    TCP_PORT                    *prt = NU_NULL;
    SOCKET_STRUCT               *listen_socket;
    SOCKET_STRUCT               *new_socket;
    INT16                       family;

#if (INCLUDE_SR_SNMP == NU_TRUE)
    UINT8                       tcp_laddr[IP_ADDR_LEN];
    UINT8                       tcp_faddr[IP_ADDR_LEN];
#endif

    if (state != SLISTEN)
    {
        prt = input;

#if (INCLUDE_IPSEC == NU_TRUE)

        /* Check whether IPsec is enabled for the device which received
         * this packet.
         */
        if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
        {
            /* Verify that the required security associations have been
             * applied for this TCP port.
             */
            if (IPSEC_TCP_Check_Policy_In(buf_ptr, prt) != NU_SUCCESS)
            {
                NLOG_Error_Log("IP packet discarded due to IPsec policy checks",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                /* Drop the packet by placing it back on the
                 * MEM_Buffer_Freelist.
                 */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List,
                                      &MEM_Buffer_Freelist);

                /* Return. */
                return (1);
            }
        }
#endif

#if (INCLUDE_TCP_KEEPALIVE == NU_TRUE)

        /* If Keep-Alive packets are being transmitted on this connection */
        if (prt->portFlags & TCP_KEEPALIVE)
        {
            /* Stop the Keep-Alive timer */
            TQ_Timerunset(TCP_Keepalive_Event, TQ_CLEAR_ALL_EXTRA,
                          (UNSIGNED)prt->pindex, 0);

            /* Reset the Keep-Alive timer to the maximum delay value */
            if (TQ_Timerset(TCP_Keepalive_Event, (UNSIGNED)prt->pindex,
                            prt->p_ka_wait, 0) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to set timer for Keep-Alive", NERR_SEVERE,
                               __FILE__, __LINE__);

                NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);
            }
        }
#endif
    }

#if (INCLUDE_IPSEC == NU_TRUE)

    /* If listening for a request on this port, there is no IPsec policy
     * yet that should be applied. Find the policy in the database.
     */
    else
    {
        /* Check whether IPsec is enabled for the device which received
         * this packet.
         */
        if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
        {
            if (IPSEC_Match_Policy_In(buf_ptr->mem_buf_device->dev_physical->
                                      dev_phy_ips_group, &IPSEC_Pkt_Selector,
                                      IPSEC_In_Bundle, IPSEC_SA_Count,
                                      NU_NULL) != NU_SUCCESS)
            {
                /* If while checking the policy an error occurred, discard the
                 * packet and log an error.
                 */
                NLOG_Error_Log("IP packet discarded due to IPsec policy checks",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                /* Drop the packet by placing it back on the Free List. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                /* Return error. */
                return (1);
            }
        }
    }

#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (buf_ptr->mem_flags & NET_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        family = SK_FAM_IP6;
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        family = SK_FAM_IP;
    }
#endif

    /* Get the total length on the packet. Including the TCP header. */
    tlen = (UINT16)buf_ptr->mem_total_data_len;

    /* Get a pointer to the TCP header. */
    p = (TCPLAYER *)buf_ptr->data_ptr;

    /* Strip off the TCP header from the data sizes and data ptr. */
    buf_ptr->data_ptr           += (TCP_HEADER_LEN + buf_ptr->mem_option_len);
    buf_ptr->data_len           -= (TCP_HEADER_LEN + buf_ptr->mem_option_len);
    buf_ptr->mem_total_data_len -= (TCP_HEADER_LEN + buf_ptr->mem_option_len);

#if ((INCLUDE_TCP_INFO_LOGGING == NU_TRUE) || (PRINT_TCP_MSG == NU_TRUE))
    /* Print the TCP header info */
    NLOG_TCP_Info (p, (INT16)tlen, NLOG_RX_PACK);
#endif

    /*  Enter the state machine.  Determine the current state and perform
        the appropriate function.  Completed I/O will invoke this
        routine and consequently advance the state.  */

#if (TCPSECURE_DRAFT == NU_TRUE)

    if (state != SLISTEN)
    {
        /* The ACK value should be acceptable only if it is in the range of
         * ((SND.UNA - MAX.SND.WND) <= SEG.ACK <= SND.NXT). MAX.SND.WND is
         * defined as the largest window that the local receiver has ever
         * advertised to its peer.
         */
        if ( (INT32_CMP(GET32(p, TCP_ACK_OFFSET), (prt->out.ack - WINDOW_SIZE)) < 0) ||
             (INT32_CMP(GET32(p, TCP_ACK_OFFSET), prt->out.nxt) > 0) )
        {
            MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

            return (1);
        }
    }

#endif

    /* Store off the flags from the TCP header. */
    flags = GET8(p, TCP_FLAGS_OFFSET);

    switch (state)
    {
    case SLISTEN:  /*  Waiting for a remote connection.  */

        /* First check for an RST - an incoming RST should be ignored. */
        if (!(flags & TRESET))
        {
            /* Second check for an ACK - any acknowledgement is bad if it arrives
             * on a connection still in the LISTEN state.  An acceptable reset
             * segment should be formed for any arriving ACK-bearing segment.
             */
            if (flags & TACK)
            {
                TCP_Reset_FIN(p, tcp_chk, (INT16)buf_ptr->mem_total_data_len,
                              family, TRESET);
            }

            /* Third check for a SYN. */
            else if (flags & TSYN)
            {
#if (TCPSECURE_DRAFT == NU_TRUE)
                /* Ensure the SYN meets the requirements of Cert Advisory
                 * CA-96.21 III.
                 */
                if (TCP_Check_SYN(tcp_chk, family) != NU_SUCCESS)
                {
                    /* Drop the packet by placing it back on the buffer_freelist. */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                    return (1);
                }
#endif

                listen_socket = input;
                task_entry = listen_socket->s_accept_list;

                /* verify that there is a task table port entry free */
                tasklist_num = TCP_Find_Empty_Port(task_entry);

                if (NU_IGNORE_VALUE < tasklist_num)
                {
                    /* Extract the destination port number */
                    myport = GET16(p, TCP_DEST_OFFSET);

                    /* establish a TCP_Ports entry */
                    portlist_num = TCPSS_Net_Listen(myport, tcp_chk, family);

                    if (portlist_num < 0)
                    {
                        /* Drop the packet by placing it back on the buffer_freelist. */
                        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                        return (1);
                    }  /* end if establish a TCP_Ports entry */

                    /* Get a pointer to the new port structure. */
                    prt = TCP_Ports[portlist_num];

#if (NET_INCLUDE_SACK == NU_TRUE)

                    /* If SACK is disabled on the parent, disable it on the
                     * child too.
                     */
                    if (!(TCP_Ports[listen_socket->s_port_index]->portFlags & TCP_SACK))
                    {
                        prt->portFlags &= ~TCP_SACK;
                    }

                    /* If D-SACK is disabled on the parent, disable it on the
                     * child.  Note that SACK must be enabled to check for this
                     * option since D-SACK cannot be used w/o SACK.
                     */
                    else if (!(TCP_Ports[listen_socket->s_port_index]->portFlags & TCP_DSACK))
                    {
                        prt->portFlags &= ~TCP_DSACK;
                    }
#endif

#if (NET_INCLUDE_WINDOWSCALE == NU_TRUE)

                    /* If Window Scale is disabled on the parent, disable it on the
                     * child too.
                     */
                    if (!(TCP_Ports[listen_socket->s_port_index]->portFlags &
                          TCP_REPORT_WINDOWSCALE))
                    {
                        prt->portFlags &= ~TCP_REPORT_WINDOWSCALE;
                    }

                    /* The child socket inherits the scale factor and window size
                     * of the parent.
                     */
                    else
                    {
                        prt->in.p_win_shift =
                            TCP_Ports[listen_socket->s_port_index]->in.p_win_shift;

                        prt->in.size = prt->credit =
                            TCP_Ports[listen_socket->s_port_index]->in.size;
                    }
#endif

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

                    /* If Timestamping is disabled on the parent, disable it on the
                     * child too.
                     */
                    if (!(TCP_Ports[listen_socket->s_port_index]->portFlags &
                          TCP_REPORT_TIMESTAMP))
                    {
                        prt->portFlags &= ~TCP_REPORT_TIMESTAMP;
                    }
#endif

#if (INCLUDE_TCP_KEEPALIVE == NU_TRUE)
                    /* If Keep-Alive is enabled on the original listening socket,
                     * enable Keep-Alive on the new socket too.
                     */
                    prt->portFlags = (UINT32)(prt->portFlags |
                        (TCP_Ports[listen_socket->s_port_index]->portFlags & TCP_KEEPALIVE));

                    /* Set up the keep-alive wait interval for the socket. */
                    prt->p_ka_wait =
                        TCP_Ports[listen_socket->s_port_index]->p_ka_wait;

                    /* Set up the keep-alive retransmission limit for the socket. */
                    prt->p_ka_r2 =
                        TCP_Ports[listen_socket->s_port_index]->p_ka_r2;
#endif

                    new_socket = SCK_Sockets[prt->p_socketd];

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

                    if (listen_socket->s_flags & SF_V4_MAPPED)
                    {
                        /* Flag the new socket as a IPv4-Mapped socket */
                        new_socket->s_flags |= SF_V4_MAPPED;

                        /* Remove the IPv4-Mapped flag from the listening socket */
                        listen_socket->s_flags &= ~SF_V4_MAPPED;
                    }
#endif
                    /* store the TCP_Ports entry number in the task table */
                    task_entry->socket_index[tasklist_num] = prt->p_socketd;

                    /* Remove the suspended task from the accept list
                       and move it to the new sockets TX list. */
                    ssp_task = DLL_Dequeue(&task_entry->ssp_task_list);

                    /* Add the suspended task to the new socket */
                    if (ssp_task != NU_NULL)
                        DLL_Enqueue(&new_socket->s_TXTask_List, ssp_task);

                    /* store the port number */
                    new_socket->s_local_addr.port_num = myport;

                }  /* end if for no port num back from nuke */

                else
                {
                    /* Drop the packet by placing it back on the buffer_freelist. */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                    return (1);
                }

                /* Preserve the Task_Entry information so we can cleanup in
                 * the case where the connection is aborted before being
                 * accepted.
                 */
                new_socket->s_accept_list = task_entry;
                new_socket->s_accept_index = tasklist_num;

                /* Remember anything important from the incoming TCP header */
                prt->out.size = GET16(p, TCP_WINDOW_OFFSET);
                prt->out.port = GET16(p, TCP_SRC_OFFSET);
                prt->in.nxt = GET32(p, TCP_SEQ_OFFSET) + 1;

                /* Set the necessary fields in the outgoing TCP packet  */
                prt->out.tcp_flags = TSYN | TACK;

                /* Initialize all of the low-level transmission stuff
                 * (IP and lower)
                 */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
                if (family == SK_FAM_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                    NU_BLOCK_COPY(prt->tcp_faddrv6,
                                  ((struct pseudohdr*)(tcp_chk))->source,
                                  IP6_ADDR_LEN);
#if (INCLUDE_IPV4 == NU_TRUE)
                else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

                    prt->tcp_faddrv4 =
                        LONGSWAP(((struct pseudotcp*)(tcp_chk))->source);
#endif

                /* Parse the options for the incoming SYN packet. */
                if (TCP_Parse_SYN_Options(prt, p, hlen) != NU_SUCCESS)
                {
                    TCP_Reset_FIN(p, tcp_chk, (INT16)buf_ptr->mem_total_data_len,
                                  family, TRESET);

                    /* Clean up the port. */
                    TCP_Cleanup(prt);

                    /* Increment the number of connection failures. */
                    MIB2_tcpAttemptFails_Inc;

                    /* Drop the packet by placing it back on the buffer_freelist. */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                    return (1);
                }

                /* Indicate that a SYN has been received so get ready
                 * to get an ACK.
                 */
                prt->state = SSYNR;

                /* Send the SYN/ACK packet to the client.  */
                if (TCPSS_Send_SYN_FIN(prt) != NU_SUCCESS)
                {
                    /* Clean up the port. */
                    TCP_Cleanup(prt);

                    /* Increment the number of connection failures. */
                    MIB2_tcpAttemptFails_Inc;

                    /* Drop the packet by placing it back on the buffer_freelist. */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                    return (1);
                }

                /* Increment the number of passive opens. */
                MIB2_tcpPassiveOpens_Inc;

            } /* end if */
        }

        /* Fourth other text or control - Any other control or text-bearing
         * segment (not containing SYN) must have an ACK and thus would be
         * discarded by the ACK processing.  An incoming RST segment could
         * not be valid, since it could not have been sent in response to
         * anything sent by this incarnation of the connection.  So you
         * are unlikely to get here, but if you do, drop the segment.
         */

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

    case SSYNR:

        /*  In the SYN Received State we expect that we got an ACK.
            If we did, then we need to send an ACK and move on to
            the connected state.  */

        if (!(flags & TRESET))
        {
            /* The SYN flag should not be set. */
            if (!(flags & TSYN))
            {
                /*   We are expecting an ACK, if we did not get it then send
                 *   the SYN/ACK back.  */
                if (flags & TACK)
                {
#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

                    /* Check the Timestamp option. */
                    if (TCP_Check_Timestamp(p, prt, NU_NULL) != NU_SUCCESS)
                    {
                        /* Drop the packet. */
                        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                        return (1);
                    }
#endif

                    /*  Set up for the timeout timer.  */
                    prt->out.lasttime = NU_Retrieve_Clock();

                    /*  Starting ACK value */
                    prt->out.ack = GET32(p, TCP_ACK_OFFSET);

                    /*  Accept his window size.  */
                    prt->out.size =
                        GET16(p, TCP_WINDOW_OFFSET) << prt->out.p_win_shift;

                    prt->maxSendWin = prt->out.size;

                    /*  Set up to send an ACK back.  */
                    prt->out.tcp_flags = TACK;

                    /*  Move on to established state.  */
                    prt->state = SEST;

#if (INCLUDE_SR_SNMP == NU_TRUE)
#if (INCLUDE_IPV6 == NU_TRUE)
                    if (!(buf_ptr->mem_flags & NET_IP6))
#endif
                    {
                        PUT32(tcp_laddr, 0, prt->tcp_laddrv4);
                        PUT32(tcp_faddr, 0, prt->tcp_faddrv4);

                        SNMP_tcpConnTableUpdate(SNMP_ADD, SEST, tcp_laddr,
                                                (UNSIGNED)(prt->in.port), tcp_faddr,
                                                (UNSIGNED)(prt->out.port));
                    }
#endif

                    MIB2_tcpCurrEstab_Inc;

                    /* Mark the socket as connected. */
                    SCK_CONNECTED(prt->p_socketd);

#if (INCLUDE_TCP_KEEPALIVE == NU_TRUE)
                    /* If this connection is supposed to transmit Keep-Alive packets */
                    if (prt->portFlags & TCP_KEEPALIVE)
                    {
                        /* Start the Keep-Alive timer to the maximum delay value */
                        if (TQ_Timerset(TCP_Keepalive_Event, (UNSIGNED)prt->pindex,
                                        prt->p_ka_wait, 0) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to set timer for Keep-Alive", NERR_SEVERE,
                                           __FILE__, __LINE__);

                            NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                                           NU_Current_Task_Pointer(), NU_NULL);
                        }
                    }
#endif

                    /*  Delete the timeout timer.  */
                    TQ_Timerunset(TCPRETRANS, TQ_CLEAR_ALL_EXTRA,
                                  (UNSIGNED)prt->pindex, 0);

                    /*  Remove the SYN packet from the window.  */
                    TCP_Rmqueue(&prt->out, prt->out.ack);

                    /* Get the task entry.  The -1 indicates that we don't care about the
                       state as long as port number and port index match. */
                    task_entry = SCK_Sockets[prt->p_socketd]->s_accept_list;
                    tasklist_num = SCK_SearchTaskList(task_entry, -1, prt->p_socketd);

                    /*  Indicate the connection is complete.  This one can be accepted.*/
                    task_entry->stat_entry[tasklist_num] = SEST;

                    /* Remove the suspended task from the accept list
                       and move it to the sockets TX list so it can be
                       awakened below. */
                    if (task_entry->ssp_task_list.flink)
                    {
                        ssp_task = DLL_Dequeue(&task_entry->ssp_task_list);

                        /* Add the suspended task to the socket's TX list */
                        DLL_Enqueue(&SCK_Sockets[prt->p_socketd]->s_TXTask_List,
                                    ssp_task);
                    }

                    /* Resume all tasks waiting for this connection to complete. */
                    if (SCK_Sockets[prt->p_socketd]->s_TXTask_List.flink != NU_NULL)
                    {
                        /* Send an event to wake up the suspended server.  */
                        if (EQ_Put_Event(CONOPEN, (UNSIGNED)prt->p_socketd,
                                         0) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to set event to wake up suspended server",
                                           NERR_SEVERE, __FILE__, __LINE__);
                    }

                    /* fall through */
                }

                else
                {
                     prt->out.tcp_flags = TACK | TSYN;
                     TCP_ACK_It(prt, 1);

                     /* Preserve a pointer to the packet buffer just processed so we
                      * can deallocate it.
                      */

                     /* Drop the packet by placing it back on the buffer_freelist. */
                     MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
                     break;
                } /* end if */
            }

            else
            {
#if (TCPSECURE_DRAFT == NU_TRUE)
                /* Ensure the SYN meets the requirements of Cert Advisory
                 * CA-96.21 III.
                 */
                if (TCP_Check_SYN(tcp_chk, family) != NU_SUCCESS)
                {
                    /* Drop the packet by placing it back on the buffer_freelist. */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                    return (1);
                }
#endif

                /* It is possible that our ACK of the SYN was never received.
                   This check handles that case. If the SEQ number we expect is one
                   greater than the received SEQ number, then this is okay. */
                if (prt->in.nxt != (GET32(p, TCP_SEQ_OFFSET) + 1))
                {
#if (TCPSECURE_DRAFT == NU_TRUE)

                    /* Send a FIN and a RESET. */
                    prt->out.tcp_flags = TFIN | TRESET;
                    TCP_ACK_It(prt, 1);

                    /* Return to the closed state */
                    prt->state = SCLOSED;

                    /* Clean up the port. */
                    TCP_Cleanup(prt);

                    /* Increment the number of connection failures. */
                    MIB2_tcpAttemptFails_Inc;

#else
                    TCP_Process_Invalid_TSYN(prt, p);
#endif
                }

                /* RFC 793 - If the SYN is not in the window this step would
                 * not be reached and an ack would have been sent in the
                 * first step (sequence number check).
                 *
                 * However, since this code is not processed in order of the RFC,
                 * send the ACK now.
                 *
                 * RFC 793 - If a segment arrives and the state is SYN-RECEIVED,
                 * first check the sequence number.  If an incoming segment is
                 * not acceptable, an acknowledgment should be sent in reply
                 * (unless the RST bit is set, if so drop the segment and return):
                 *
                 * <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>
                 *
                 * After sending the acknowledgment, drop the unacceptable segment
                 * and return.
                 */
                else
                {
                    prt->out.tcp_flags &= ~TSYN;

                    TCP_ACK_It(prt, 1);

                    prt->out.tcp_flags |= TSYN;
                }

                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                return (1);
            }
        }

        else
        {
            NLOG_Error_Log("RESET received in response to our SYN",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Return to the closed state */
            prt->state = SCLOSED;

            /* Clean up this port. */
            TCP_Cleanup(prt);

            /* Increment the number of connection failures. */
            MIB2_tcpAttemptFails_Inc;

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (1);
        } /* end if */

    case SEST:    /* normal data transmission */

        /* Check that the sequence number is valid */
        if (TCP_Valid_Seq(GET32(p, TCP_SEQ_OFFSET),(UINT16)(tlen - hlen),
                          prt->in.nxt, (INT32)prt->in.size))
        {
            if (TEST_TCP_Rx(p, &tlen, hlen, buf_ptr, prt) != NU_SUCCESS)
            {
                /* Free the buffer. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                /* Return an error. */
                return (1);
            }

            /* Check that the SYN flag is not set */
            if (!(flags & TSYN))
            {
                return (TCP_Do_Recv(prt, p, buf_ptr, hlen, tlen));
            }

            /* The SYN flag should not be set */
            else
            {
                TCP_Process_Invalid_TSYN(prt, p);

                /* Increment the number of resets from the established state. */
                MIB2_tcpEstabResets_Inc;
            }
        }

        /* The sequence number is outside the expected window; send an ACK to
         * let the foreign host know what we are expecting as long as the
         * RESET bit is not set.
         */
        else
#if (TCPSECURE_DRAFT == NU_TRUE)
            if (!(flags & TRESET))
#endif
        {
#if (NET_INCLUDE_DSACK == NU_TRUE)

            /* If D-SACK is enabled on the socket, and this is a duplicate
             * data segment.
             */
            if ( (prt->portFlags & TCP_DSACK) && (tlen - hlen) &&
                 (INT32_CMP(GET32(p, TCP_SEQ_OFFSET), prt->in.nxt) < 0) )
            {
                /* Set the flag to include a D-SACK in the ACK. */
                prt->portFlags |= TCP_REPORT_DSACK;

                /* Set the left and right edge of this packet so it
                 * can be included in the D-SACK.
                 */
                prt->left_edge = GET32(p, TCP_SEQ_OFFSET);
                prt->right_edge = GET32(p, TCP_SEQ_OFFSET) + (tlen - hlen);
            }
#endif

            TCP_Schedule_ACK(prt, p, tlen - hlen, 1);
        }

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        return (1);

    case SSYNS: /* check to see if the ACK is for our SYN */

        /* Is the ACK for the SYN that was sent?  If not we have a half open
           connection (i.e., the foreign host believes there is already a
           connection open on this port.  So send a reset and drop the packet.
        */
        if ( (flags & TACK) &&
             (GET32(p, TCP_ACK_OFFSET) != prt->out.nxt) )
        {
            /* Send a reset. */
            TCP_Reset_FIN(p, tcp_chk, (INT16) buf_ptr->mem_total_data_len,
                          family, TRESET | TFIN);

            NLOG_Error_Log("Sending RESET to close half open connection",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (1);
        } /* end if */

        if (!(flags & TRESET))
        {
            if (flags & TSYN)                 /* need to send ACK */
            {
#if (TCPSECURE_DRAFT == NU_TRUE)
                /* Ensure the SYN meets the requirements of Cert Advisory
                 * CA-96.21 III.
                 */
                if (TCP_Check_SYN(tcp_chk, family) != NU_SUCCESS)
                {
                    /* Drop the packet by placing it back on the buffer_freelist. */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                    return (1);
                }
#endif

                /* Parse the options in the incoming SYN packet. */
                if (TCP_Parse_SYN_Options(prt, p, hlen) == NU_SUCCESS)
                {
                    prt->out.tcp_flags = TACK;
                    prt->in.nxt = GET32(p, TCP_SEQ_OFFSET) + 1;
                    prt->out.ack = GET32(p, TCP_ACK_OFFSET);

                    /* The Window Size for a SYN packet is never scaled, so
                     * do not apply a scale factor to it here.
                     */
                    prt->out.size = GET16(p, TCP_WINDOW_OFFSET);

                    prt->maxSendWin = prt->out.size;

                    TCP_ACK_It(prt, 1);

                    if (flags & TACK)
                    {
#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

                        /* Check the Timestamp option. */
                        if (TCP_Check_Timestamp(p, prt, NU_NULL) != NU_SUCCESS)
                        {
                            /* Drop the packet. */
                            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                            return (1);
                        }
#endif

                        if (prt->p_rtt != 0)
                        {
                            /* Update the RTO calculation. */
                            TCP_Xmit_Timer(prt, NU_Retrieve_Clock() - prt->p_rtt);
                        }

                        prt->state = SEST;

#if (INCLUDE_SR_SNMP == NU_TRUE)
#if (INCLUDE_IPV6 == NU_TRUE)
                        if (!(buf_ptr->mem_flags & NET_IP6))
#endif
                        {
                            PUT32(tcp_laddr, 0, prt->tcp_laddrv4);
                            PUT32(tcp_faddr, 0, prt->tcp_faddrv4);

                            SNMP_tcpConnTableUpdate(SNMP_ADD, SEST, tcp_laddr,
                                                    (UNSIGNED)(prt->in.port), tcp_faddr,
                                                    (UNSIGNED)(prt->out.port));
                        }
#endif
                        MIB2_tcpCurrEstab_Inc;

                        /* Mark the socket as connected. */
                        SCK_CONNECTED(prt->p_socketd);

#if (INCLUDE_TCP_KEEPALIVE == NU_TRUE)
                        /* If this connection is supposed to transmit Keep-Alive packets */
                        if (prt->portFlags & TCP_KEEPALIVE)
                        {
                            /* Start the Keep-Alive timer to the maximum delay value */
                            if (TQ_Timerset(TCP_Keepalive_Event, (UNSIGNED)prt->pindex,
                                            prt->p_ka_wait, 0) != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Failed to set timer for Keep-Alive", NERR_SEVERE,
                                               __FILE__, __LINE__);

                                NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                                               NU_Current_Task_Pointer(), NU_NULL);
                            }
                        }
#endif
                        /*  Delete the timeout timer.  */
                        TQ_Timerunset(TCPRETRANS, TQ_CLEAR_ALL_EXTRA,
                                      (UNSIGNED)prt->pindex, 0);

                        /*  Remove it from the window.  */
                        TCP_Rmqueue(&prt->out, prt->out.ack);

                        if (EQ_Put_Event(CONOPEN, (UNSIGNED)prt->p_socketd,
                                         0) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to set event to time out connection",
                                           NERR_SEVERE, __FILE__, __LINE__);
                        }
                    } /* end if */

                    else
                    {
                        prt->state = SSYNR;           /* syn received */
                    }
                }

                /* The MSS of the other side is invalid */
                else
                {
                    TCP_Reset_FIN(p, tcp_chk, (INT16)buf_ptr->mem_total_data_len,
                                  family, TRESET);

                    /* Increment the number of connection failures. */
                    MIB2_tcpAttemptFails_Inc;

                    /* Drop the packet by placing it back on the buffer_freelist. */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                    return (1);
                }
            } /* end if */
        }

        /* Received a RESET */
        else
        {
            NLOG_Error_Log("Received a RESET in response to our SYN",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            prt->state = SCLOSED;

            /* Cleanup after ourselves. */
            TCP_Cleanup(prt);

            /* Increment the number of connection failures. */
            MIB2_tcpAttemptFails_Inc;

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (1);
        } /* end if */

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

    case SCWAIT:

        /* The SYN flag should not be set. */
        if (!(flags & TSYN))
        {
#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

            /* The value of TSval does not matter for a RESET. */
            if (!(flags & TRESET))
            {
                /* Check the Timestamp option. */
                if (TCP_Check_Timestamp(p, prt, NU_NULL) != NU_SUCCESS)
                {
                    /* Drop the packet. */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                    return (1);
                }
            }
#endif

            TCP_ACK_Check(prt, p);

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
        }

        else
        {
            TCP_Process_Invalid_TSYN(prt, p);

#if (TCPSECURE_DRAFT == NU_FALSE)

            /* Increment the number of resets from the established state. */
            MIB2_tcpEstabResets_Inc;

#endif
            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (1);
        }

        break;

    case SLAST:    /* check ack of FIN, or reset to see if we are done */

        /* The SYN flag should not be set. */
        if (flags & TSYN)
        {
            TCP_Process_Invalid_TSYN(prt, p);

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (1);
        }

#if (TCPSECURE_DRAFT == NU_TRUE)
        if ( ((TCP_ACK_Check(prt, p) == NU_SUCCESS) ||
              (GET32(p, TCP_ACK_OFFSET) == prt->out.nxt)) &&
             (!(flags & TRESET)) )
#else
        if ( (TCP_ACK_Check(prt, p) == NU_SUCCESS) ||
             (flags & TRESET) ||
             (GET32(p, TCP_ACK_OFFSET) == prt->out.nxt) )
#endif
        {
#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

            /* Check the Timestamp option. */
            if (TCP_Check_Timestamp(p, prt, NU_NULL) != NU_SUCCESS)
            {
                /* Drop the packet. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                return (1);
            }
#endif

            /* Return to the closed state */
            prt->state = SCLOSED;

#if (INCLUDE_SR_SNMP == NU_TRUE)

            if (!(buf_ptr->mem_flags & NET_IP6))
            {
                PUT32(tcp_laddr, 0, prt->tcp_laddrv4);
                PUT32(tcp_faddr, 0, prt->tcp_faddrv4);

                SNMP_tcpConnTableUpdate(SNMP_DELETE, SEST, tcp_laddr,
                                        (UNSIGNED)(prt->in.port), tcp_faddr,
                                        (UNSIGNED)(prt->out.port));
            }
#endif
            TCP_Cleanup(prt);

            MIB2_tcpCurrEstab_Dec;
        }

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

    case SFW1:                              /* waiting for ACK of FIN */

        /* The SYN flag should no be set. */
        if (flags & TSYN)
        {
            TCP_Process_Invalid_TSYN(prt, p);

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (1);
        }

        if (flags & TRESET)
        {
#if (TCPSECURE_DRAFT == NU_FALSE)
            /* If the sequence number is within the acceptable window, reset
             * the connection.  Otherwise, silently drop the segment.
             */
            if ( (INT32_CMP(GET32(p, TCP_SEQ_OFFSET), prt->in.nxt) >= 0) &&
                 (INT32_CMP(GET32(p, TCP_SEQ_OFFSET),
                            (prt->in.nxt + prt->in.size)) < 0) )
#else
            /* If the sequence number is exactly the next expected
             * sequence number, reset the connection.
             */
            if (GET32(p, TCP_SEQ_OFFSET) == prt->in.nxt)
#endif
            {
                /* Return to the closed state */
                prt->state = SCLOSED;

#if (INCLUDE_SR_SNMP == NU_TRUE)

                if (!(buf_ptr->mem_flags & NET_IP6))
                {
                    PUT32(tcp_laddr, 0, prt->tcp_laddrv4);
                    PUT32(tcp_faddr, 0, prt->tcp_faddrv4);

                    SNMP_tcpConnTableUpdate(SNMP_DELETE, SEST, tcp_laddr,
                                            (UNSIGNED)(prt->in.port), tcp_faddr,
                                            (UNSIGNED)(prt->out.port));
                }
#endif
                TCP_Cleanup(prt);

                MIB2_tcpCurrEstab_Dec;
            }

#if (TCPSECURE_DRAFT == NU_TRUE)

            /* Otherwise, if the sequence number is within the acceptable window
             * (RCV.NXT < SEG.SEQ < RCV.NXT + RCV.WND), send an acknowledgement.
             */
            else if ( (INT32_CMP(GET32(p, TCP_SEQ_OFFSET), prt->in.nxt) > 0) &&
                      (INT32_CMP(GET32(p, TCP_SEQ_OFFSET),
                                 (prt->in.nxt + prt->in.size)) < 0) )
            {
                TCP_ACK_It(prt, 1);

                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

                break;
            }
#endif
            /* If the RST bit is set and the sequence number is outside the
             * expected window, silently drop the packet.
             */
            else
            {
                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

                break;
            }
        }

        else if ( ((flags & TACK) != TACK) ||
                  (GET32(p, TCP_ACK_OFFSET) != prt->out.nxt) )
        {
            if (flags & TFIN)            /* got FIN, no ACK for mine */
            {
#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

                /* Check the Timestamp option. */
                if (TCP_Check_Timestamp(p, prt, NU_NULL) != NU_SUCCESS)
                {
                    /* Drop the packet. */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                    return (1);
                }
#endif

                prt->in.nxt++;              /* account for FIN byte */
                prt->out.tcp_flags = TACK;   /* final byte has no FIN flag */

                /* Delay ACK so that it may be send with data if any. */
                TCP_ACK_It(prt, 0);

                prt->state = SCLOSING;

            } /* end if */
        } /* end if */

        else
        {
#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

            /* Check the Timestamp option. */
            if (TCP_Check_Timestamp(p, prt, NU_NULL) != NU_SUCCESS)
            {
                /* Drop the packet. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                return (1);
            }
#endif

            /* If the other side is closing too. */
            if (flags & TFIN)  /* ACK and FIN */
            {
                prt->in.nxt++;              /* account for his FIN flag */
                prt->out.tcp_flags = TACK;  /* final byte has no FIN flag */

                /* Delay ACK so that it may be send with data if any. */
                TCP_ACK_It(prt, 0);

                TCP_Enter_STWAIT(prt);

            } /* end if */

            else
            {                                   /* got ACK, no FIN */
                prt->out.tcp_flags = TACK;   /* final pkt has no FIN flag */
                prt->state = SFW2;

                /* If the linger option is set, wake up the thread. */
                if ( (prt->p_socketd >= 0) &&
                     (SCK_Sockets[prt->p_socketd] != NU_NULL) &&
                     (SCK_Sockets[prt->p_socketd]->s_linger.linger_on == NU_TRUE) &&
                     (SCK_Sockets[prt->p_socketd]->s_linger.linger_ticks > 0) &&
                     (SCK_Sockets[prt->p_socketd]->s_CLSTask != NU_NULL) )
                {
                    if (NU_Resume_Task(SCK_Sockets[prt->p_socketd]->s_CLSTask) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to resume task", NERR_SEVERE,
                                       __FILE__, __LINE__);

                        NET_DBG_Notify(-1, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }
                }

                /* Timeout this connection if a FIN is not received from the
                foreign host. We will use a timeout of 5 times the current
                retransmit timeout. */
                if (TQ_Timerset(TCPCLOSETIMEOUTSFW2, (UNSIGNED)prt->pindex,
                                TCP_SFW2_TIMEOUT, 0) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to set timer to timeout connection",
                                NERR_SEVERE, __FILE__, __LINE__);

                    NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                                NU_Current_Task_Pointer(), NU_NULL);
                }
            }
        } /* end else */

        /* Check if the socket is still available */
        if ( (prt->p_socketd >= 0) &&
             (SCK_Sockets[prt->p_socketd] != NU_NULL))
        {
            if (prt->portFlags & TCP_HALFDPLX_CLOSE)
            {
                TCP_Reset_FIN(p, tcp_chk, (INT16)buf_ptr->mem_total_data_len,
                              family, TRESET);

                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                return (1);

            }
            else
                /* The receive side is still open, receive this data */
                TCP_Do_Recv (prt, p, buf_ptr, hlen, tlen);
        }
        else
        {
            /* ACK if necessary */
            TCP_ACK_Check(prt, p);

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);
        }
        break;

    case SFW2:

        /* The SYN flag should not be set. */
        if (flags & TSYN)
        {
            TCP_Process_Invalid_TSYN(prt, p);

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (1);
        }

        if (flags & TRESET)
        {
#if (TCPSECURE_DRAFT == NU_FALSE)
            /* If the sequence number is within the acceptable window, reset
             * the connection.  Otherwise, silently drop the segment.
             */
            if ( (INT32_CMP(GET32(p, TCP_SEQ_OFFSET), prt->in.nxt) >= 0) &&
                 (INT32_CMP(GET32(p, TCP_SEQ_OFFSET),
                            (prt->in.nxt + prt->in.size)) < 0) )
#else
            /* If the sequence number is exactly the next expected
             * sequence number, reset the connection.
             */
            if (GET32(p, TCP_SEQ_OFFSET) == prt->in.nxt)
#endif
            {
                TQ_Timerunset(TCPCLOSETIMEOUTSFW2, TQ_CLEAR_EXACT,
                              (UNSIGNED)prt->pindex, 0);

                /* Return to the closed state */
                prt->state = SCLOSED;

#if (INCLUDE_SR_SNMP == NU_TRUE)

                if (!(buf_ptr->mem_flags & NET_IP6))
                {
                    PUT32(tcp_laddr, 0, prt->tcp_laddrv4);
                    PUT32(tcp_faddr, 0, prt->tcp_faddrv4);

                    SNMP_tcpConnTableUpdate(SNMP_DELETE, SEST, tcp_laddr,
                                            (UNSIGNED)(prt->in.port), tcp_faddr,
                                            (UNSIGNED)(prt->out.port));
                }
#endif

                TCP_Cleanup(prt);

                MIB2_tcpCurrEstab_Dec;
            }

#if (TCPSECURE_DRAFT == NU_TRUE)

            /* Otherwise, if the sequence number is within the acceptable window
             * (RCV.NXT < SEG.SEQ < RCV.NXT + RCV.WND), send an acknowledgement.
             */
            else if ( (INT32_CMP(GET32(p, TCP_SEQ_OFFSET), prt->in.nxt) > 0) &&
                      (INT32_CMP(GET32(p, TCP_SEQ_OFFSET),
                                 (prt->in.nxt + prt->in.size)) < 0) )
            {
                TCP_ACK_It(prt, 1);

                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

                break;
            }
#endif

            /* If the RST bit is set and the sequence number is outside the
             * expected window, silently drop the packet.
             */
            else
            {
                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

                break;
            }
        }

        else
        {
#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

            /* Check the Timestamp option. */
            if (TCP_Check_Timestamp(p, prt, NU_NULL) != NU_SUCCESS)
            {
                /* Drop the packet. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                return (1);
            }
#endif

            if (flags & TFIN)               /* we got FIN */
            {
                TQ_Timerunset(TCPCLOSETIMEOUTSFW2, TQ_CLEAR_EXACT, (UNSIGNED)prt->pindex, 0);

                prt->in.nxt++;              /* count his FIN byte */

                /* Delay ACK so that it may be send with data if any. */
                TCP_ACK_It(prt, 0);

                TCP_Enter_STWAIT(prt);
            }
        }  /* end if */

        /* Check if the socket is still able to receive data */
        if ( (prt->p_socketd >= 0) &&
             (SCK_Sockets[prt->p_socketd] != NU_NULL) )
        {
            if (prt->portFlags & TCP_HALFDPLX_CLOSE)
            {
                TCP_Reset_FIN(p, tcp_chk, (INT16)buf_ptr->mem_total_data_len,
                              family, TRESET);

                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                return (1);

            }
            else
                /* The receive side is still open, receive this data */
                TCP_Do_Recv (prt, p, buf_ptr, hlen, tlen);
        }
        else
            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

    case SCLOSING:                          /* want ACK of FIN */

        if (flags & TRESET)
        {
#if (TCPSECURE_DRAFT == NU_FALSE)
            /* If the sequence number is within the acceptable window, reset
             * the connection.  Otherwise, silently drop the segment.
             */
            if ( (INT32_CMP(GET32(p, TCP_SEQ_OFFSET), prt->in.nxt) >= 0) &&
                 (INT32_CMP(GET32(p, TCP_SEQ_OFFSET),
                            (prt->in.nxt + prt->in.size)) < 0) )
#else
            /* If the sequence number is exactly the next expected
             * sequence number, reset the connection.
             */
            if (GET32(p, TCP_SEQ_OFFSET) == prt->in.nxt)
#endif
            {
                /* Return to the closed state */
                prt->state = SCLOSED;

#if (INCLUDE_SR_SNMP == NU_TRUE)

                if (!(buf_ptr->mem_flags & NET_IP6))
                {
                    PUT32(tcp_laddr, 0, prt->tcp_laddrv4);
                    PUT32(tcp_faddr, 0, prt->tcp_faddrv4);

                    SNMP_tcpConnTableUpdate(SNMP_DELETE, SEST, tcp_laddr,
                                            (UNSIGNED)(prt->in.port), tcp_faddr,
                                            (UNSIGNED)(prt->out.port));
                }
#endif

                TCP_Cleanup(prt);

                MIB2_tcpCurrEstab_Dec;
            }

#if (TCPSECURE_DRAFT == NU_TRUE)

            /* Otherwise, if the sequence number is within the acceptable window
             * (RCV.NXT < SEG.SEQ < RCV.NXT + RCV.WND), send an acknowledgement.
             */
            else if ( (INT32_CMP(GET32(p, TCP_SEQ_OFFSET), prt->in.nxt) > 0) &&
                      (INT32_CMP(GET32(p, TCP_SEQ_OFFSET),
                                 (prt->in.nxt + prt->in.size)) < 0) )
                TCP_ACK_It(prt, 1);
#endif
        }

        /* The SYN flag should not be set. */
        else if (flags & TSYN)
        {
            TCP_Process_Invalid_TSYN(prt, p);

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (1);
        }

        else
        {
#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

            /* Check the Timestamp option. */
            if (TCP_Check_Timestamp(p, prt, NU_NULL) != NU_SUCCESS)
            {
                /* Drop the packet. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                return (1);
            }
#endif

            if (!TCP_ACK_Check(prt, p))
                TCP_Enter_STWAIT(prt);
        }

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

    case STWAIT:                            /* ack FIN again? */

        if (flags & TRESET)
        {
#if (TCPSECURE_DRAFT == NU_FALSE)
            /* If the sequence number is within the acceptable window, reset
             * the connection.  Otherwise, silently drop the segment.
             */
            if ( (INT32_CMP(GET32(p, TCP_SEQ_OFFSET), prt->in.nxt) >= 0) &&
                 (INT32_CMP(GET32(p, TCP_SEQ_OFFSET),
                            (prt->in.nxt + prt->in.size)) < 0) )
#else
            /* If the sequence number is exactly the next expected
             * sequence number, reset the connection.
             */
            if (GET32(p, TCP_SEQ_OFFSET) == prt->in.nxt)
#endif
            {
                /* Return to the closed state */
                prt->state = SCLOSED;

#if (INCLUDE_SR_SNMP == NU_TRUE)

                if (!(buf_ptr->mem_flags & NET_IP6))
                {
                    PUT32(tcp_laddr, 0, prt->tcp_laddrv4);
                    PUT32(tcp_faddr, 0, prt->tcp_faddrv4);

                    SNMP_tcpConnTableUpdate(SNMP_DELETE, SEST, tcp_laddr,
                                            (UNSIGNED)(prt->in.port), tcp_faddr,
                                            (UNSIGNED)(prt->out.port));
                }
#endif

                TCP_Cleanup(prt);

                MIB2_tcpCurrEstab_Dec;
            }

#if (TCPSECURE_DRAFT == NU_TRUE)

            /* Otherwise, if the sequence number is within the acceptable window
             * (RCV.NXT < SEG.SEQ < RCV.NXT + RCV.WND), send an acknowledgement.
             */
            else if ( (INT32_CMP(GET32(p, TCP_SEQ_OFFSET), prt->in.nxt) > 0) &&
                      (INT32_CMP(GET32(p, TCP_SEQ_OFFSET),
                                 (prt->in.nxt + prt->in.size)) < 0) )
                TCP_ACK_It(prt, 1);
#endif
        }

        else
        {
            /* The SYN flag should no be set. */
            if (flags & TSYN)
            {
                TCP_Process_Invalid_TSYN(prt, p);

                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                return (1);
            }

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

            /* Check the Timestamp option. */
            if (TCP_Check_Timestamp(p, prt, NU_NULL) != NU_SUCCESS)
            {
                /* Drop the packet. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                return (1);
            }
#endif

            if ( (prt->out.lasttime &&
                 (INT32_CMP((prt->out.lasttime + prt->p_msl), NU_Retrieve_Clock()) < 0))
                 || (flags & TFIN))               /* only if he wants it */
            {
                TCP_ACK_It(prt, 1);
#if (INCLUDE_SR_SNMP == NU_TRUE)

                if (!(buf_ptr->mem_flags & NET_IP6))
                {
                    PUT32(tcp_laddr, 0, prt->tcp_laddrv4);
                    PUT32(tcp_faddr, 0, prt->tcp_faddrv4);

                    SNMP_tcpConnTableUpdate(SNMP_DELETE, SEST, tcp_laddr,
                                            (UNSIGNED)(prt->in.port), tcp_faddr,
                                            (UNSIGNED)(prt->out.port));
                }
#endif

                MIB2_tcpCurrEstab_Dec;
            }
        }

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

    case SCLOSED:

        prt->in.port = prt->out.port = 0;

        UTL_Zero(&prt->tcp_foreign_addr, sizeof(prt->tcp_foreign_addr));

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

    default:

        NLOG_Error_Log("Unsupported TCP state", NERR_RECOVERABLE,
                       __FILE__, __LINE__);

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;
    } /* end switch */

    return (0);

} /* TCP_Do */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Process_Invalid_TSYN
*
*   DESCRIPTION
*
*       This routine handles the processing of an invalid received SYN.
*
*   INPUTS
*
*       *prt                    Pointer to the port receiving data
*       *tcp_pkt                Pointer to the TCP header.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID TCP_Process_Invalid_TSYN(TCP_PORT *prt, TCPLAYER *tcp_pkt)
{
#if (TCPSECURE_DRAFT == NU_FALSE)

    /* If the SYN bit is set and the sequence number is acceptable i.e.:
     * (RCV.NXT <= SEG.SEQ <= RCV.NXT+RCV.WND) then send a RST segment to
     * the sender.
     */
    if ( (INT32_CMP(GET32(tcp_pkt, TCP_SEQ_OFFSET), prt->in.nxt) >= 0) &&
         (INT32_CMP(GET32(tcp_pkt, TCP_SEQ_OFFSET),
                    (prt->in.nxt + prt->in.size)) <= 0) )
    {
        /* Send a FIN and a RESET. */
        prt->out.tcp_flags = TFIN | TRESET;

        TCP_ACK_It(prt, 1);

        /* Return to the closed state */
        prt->state = SCLOSED;

        /* Clean up the port. */
        TCP_Cleanup(prt);
    }

    /* If the SYN bit is set and the sequence number is outside the
     * expected window, send an ACK back to the sender.
     */
    else

#else

    UNUSED_PARAMETER(tcp_pkt);

#endif

    {
        TCP_ACK_It(prt, 1);
    }

} /* TCP_Process_Invalid_TSYN */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Enter_STWAIT
*
*   DESCRIPTION
*
*       This routine handles transitioning the port structure into the
*       TIME WAIT state.
*
*   INPUTS
*
*       *prt                    Pointer to the port receiving data
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID TCP_Enter_STWAIT(TCP_PORT *prt)
{
    STATUS  status;

#if (INCLUDE_SR_SNMP == NU_TRUE)
    UINT8  tcp_laddr[IP_ADDR_LEN];
    UINT8  tcp_faddr[IP_ADDR_LEN];
#endif

    /* If the socket has not been deallocated, restart any tasks
       pending on this port. */
    if ( (prt->p_socketd >= 0) &&
         (SCK_Sockets[prt->p_socketd] != NU_NULL) )
    {
        /* Mark the socket as 'disconnecting */
        SCK_Sockets[prt->p_socketd]->s_state &= (~SS_ISCONNECTED);
        SCK_Sockets[prt->p_socketd]->s_state |= SS_ISDISCONNECTING;

        /* Resume tasks pending on RX */
        SCK_Resume_All(&SCK_Sockets[prt->p_socketd]->s_RXTask_List, 0);

        /* Resume tasks pending on TX, remove from buffer suspension
           list */
        SCK_Resume_All(&SCK_Sockets[prt->p_socketd]->s_TXTask_List, SCK_RES_BUFF);

        if (SCK_Sockets[prt->p_socketd]->s_CLSTask != NU_NULL)
        {
            status = NU_Resume_Task(SCK_Sockets[prt->p_socketd]->s_CLSTask);

            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to resume task", NERR_SEVERE,
                               __FILE__, __LINE__);

                NET_DBG_Notify(status, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);
            }
        }
    }

    if (TQ_Timerset(TCPTIMEWAIT, (UNSIGNED)prt->pindex,
                    prt->p_msl, 0) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to set TCPTIMEWAIT timer", NERR_SEVERE,
                       __FILE__, __LINE__);

        NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }

    prt->state = STWAIT;            /* we are done */

#if (INCLUDE_SR_SNMP == NU_TRUE)

    if (prt->portFlags & TCP_FAMILY_IPV4)
    {
        PUT32(tcp_laddr, 0, prt->tcp_laddrv4);
        PUT32(tcp_faddr, 0, prt->tcp_faddrv4);

        SNMP_tcpConnTableUpdate(SNMP_DELETE, SEST, tcp_laddr,
                                (UNSIGNED)(prt->in.port), tcp_faddr,
                                (UNSIGNED)(prt->out.port));
    }
#endif

    MIB2_tcpCurrEstab_Dec;

} /* TCP_Enter_STWAIT */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Do_Recv
*
*   DESCRIPTION
*
*       This routine handles receiving packets for the TCP_Do routine.
*       This functionality is needed for SEST, SFW1, and SFW2 states.
*       If the socket is not able to receive data, the packet is dropped.
*
*   INPUTS
*
*       *prt                    Pointer to the port receiving data
*       *p                      Pointer to the TCP information
*       *buf_ptr                Pointer to the NET_BUFFER
*       hlen                    Header length
*       tlen                    Total length
*
*   OUTPUTS
*
*       0
*
*************************************************************************/
STATIC INT16 TCP_Do_Recv (TCP_PORT *prt, TCPLAYER *p, NET_BUFFER *buf_ptr,
                           UINT16 hlen, UINT16 tlen)
{
    NET_BUFFER                  *nxtPkt;
    INT16                       stat;

#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)
    UINT32                      current_window;
    UINT32                      ak;
    TCP_BUFFER                  *tcp_buf;
#endif

#if (NET_INCLUDE_SACK == NU_TRUE)
    TCP_BUFFER                  *saved_buf = NU_NULL;
    TCP_BUFFER                  *temp_tcp_buf;
    UINT32                      last_sack = 0;
#endif

#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)
    /* Save off the value of the other side's window */
    current_window = prt->out.size;
#endif

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

    /* If the ACK bit is set, check the Timestamp option. */
    if (GET8(p, TCP_FLAGS_OFFSET) & TACK)
    {
        if (TCP_Check_Timestamp(p, prt, NU_NULL) != NU_SUCCESS)
        {
            /* Drop the packet. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (1);
        }
    }
#endif

    /* See if the last packet acknowledged one that we sent.  If
     * so, ackcheck will update the buffer and begin transmission
     * on the next part of the data if any is left in the buffer. If this
     * is an invalid ACK then we must drop this packet.
     */
    stat = TCP_ACK_Check(prt, p);

    /* Drop this invalid packet by placing it back onto the
     * free list.
     */
    if (stat == TCP_INVALID_ACK)
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

    /* Otherwise continue processing. */
    else
    {
#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)

        /* If congestion control is enabled on the socket, and the ACK is not increasing,
         * TCP_ACK_Check will return a value of 1.
         */
        if ( (!(prt->portFlags & TCP_DIS_CONGESTION)) && (stat == 1) )
        {
            /* If the length of the received segment is 0 and the advertised
             * window has not changed.
             */
            if ( ((tlen - hlen) == 0) && (current_window == prt->out.size) )
            {
                /* Pick up the other side's ack number. */
                ak = GET32(p, TCP_ACK_OFFSET);

                /* If TCP does not have outstanding data that has not been
                 * acknowledged or the acknowledgement field does not equal
                 * the oldest unacknowledged sequence number.
                 */
                if ( (prt->out.contain == 0) ||
                     (INT32_CMP(ak, prt->out.ack) != 0) )
                    prt->p_dupacks = 0;

                else
                {
                    /* Increment the number of duplicate ACKs received. */
                    prt->p_dupacks++;

                    /* If this is the third consecutive duplicate ACK received. */
                    if (prt->p_dupacks == TCP_FAST_RETRANS_DUP_ACKS)
                    {
#if (INCLUDE_NEWRENO == NU_TRUE)
                        /* If the cumulative acknowledgement field covers more
                         * than p_recover or the congestion window is greater
                         * than SMSS bytes and the difference between ak
                         * and p_prev_highest_ack is at most 4*SMSS, invoke
                         * Fast Retransmit.  If SACK is being used, enter the
                         * conditional regardless of the other factors.
                         */
                        if ( (prt->portFlags & TCP_SACK) ||
                             ((ak - 1) > prt->p_recover) ||
                             ((prt->p_cwnd > prt->p_smss) &&
                              ((ak - prt->p_prev_highest_ack) <=
                                prt->p_smss << 2)) )
#endif
                        {
                            /* Get a pointer to the head of the retransmission
                             * list.
                             */
                            tcp_buf = prt->out.packet_list.tcp_head;

#if (NET_INCLUDE_SACK == NU_TRUE)

                            /* If SACK is enabled on the socket. */
                            if (prt->portFlags & TCP_SACK)
                            {
                                /* Find the last buffer with the SACK flag set.
                                 * For this:
                                 *     Get a pointer to the tail of the retransmission
                                 *     list.
                                 */
                                 temp_tcp_buf = prt->out.packet_list.tcp_tail;

                                 /* loop backward until we find a buffer with the SACK flag set */
                                 while (temp_tcp_buf)
                                 {
                                    if (temp_tcp_buf->tcp_buf_ptr->mem_flags & NET_TCP_SACK)
                                     {
                                        /* store the sequence number of the last SACK'd packet
                                         * in a local variable
                                         */
                                        last_sack = temp_tcp_buf->tcp_buf_ptr->mem_seqnum;

                                        break;
                                     }

                                    /* Get a pointer to the previous packet */
                                    temp_tcp_buf = temp_tcp_buf->tcp_previous;
                                 }

                                /* Find the packet to retransmit. */
                                while (tcp_buf)
                                {
                                    /* If this packet has not been SACKed, and the sequence
                                     * number is less than the data that has been ACKed,
                                     * retransmit this packet.
                                     */
                                    if ( (!(tcp_buf->tcp_buf_ptr->mem_flags & NET_TCP_SACK)) &&
                                         (INT32_CMP(last_sack, (tcp_buf->tcp_buf_ptr->mem_seqnum)) >= 0) &&
                                         (!(tcp_buf->tcp_buf_ptr->mem_flags & NET_TX_QUEUE)) )
                                    {
                                        TEST_TCP_SACK_Retransmit(tcp_buf->tcp_buf_ptr->mem_seqnum);

                                        /* RFC 2581 - section 3.2 - Retransmit the lost
                                         * segment.
                                         */
                                        if (TCP_Send_Retransmission(tcp_buf->tcp_buf_ptr,
                                                                    prt) != NU_SUCCESS)
                                        {
                                            NLOG_Error_Log("Failed to retransmit packet",
                                                           NERR_SEVERE, __FILE__, __LINE__);
                                        }

                                        /* Save a pointer to a buffer that gets
                                         * retansmitted - we just need proof that at
                                         * least one packet was sent.
                                         */
                                        saved_buf = tcp_buf;
                                    }

                                    /* Get a pointer to the next packet */
                                    tcp_buf = tcp_buf->tcp_next;
                                }

                                /* Restore a pointer to a buffer that was
                                 * retransmitted.
                                 */
                                tcp_buf = saved_buf;
                            }

                            /* If this is the target packet. */
                            else
#endif
                            {
                                /* Find the packet to retransmit. */
                                while (tcp_buf)
                                {
                                    if (tcp_buf->tcp_buf_ptr->mem_seqnum == ak)
                                    {
                                        /* RFC 2581 - section 3.2 - Retransmit the lost
                                         * segment.
                                         */
                                        if (TCP_Send_Retransmission(tcp_buf->tcp_buf_ptr,
                                                                    prt) != NU_SUCCESS)
                                        {
                                            NLOG_Error_Log("Failed to retransmit packet",
                                                           NERR_SEVERE, __FILE__, __LINE__);
                                        }

                                        break;
                                    }

                                    /* Get a pointer to the next packet. */
                                    tcp_buf = tcp_buf->tcp_next;
                                }
                            }

                            /* If the target packet was found. */
                            if (tcp_buf)
                            {
                                /* RFC 2581 - section 3.2 - When the third
                                 * duplicate ACK is received, set ssthresh to
                                 * no more than max(FlightSize / 2, 2*SMSS).
                                 */
                                prt->p_ssthresh =
                                    ((UINT16)(prt->out.contain >> 1) > (prt->p_smss << 1)) ?
                                    (prt->out.contain >> 1) : (prt->p_smss << 1);

#if (INCLUDE_NEWRENO == NU_TRUE)
                                if (!(prt->portFlags & TCP_SACK))
                                {
                                    /* RFC 3782 - section 3, step 1A - In addition,
                                     * record the highest sequence number transmitted
                                     * in the variable "recover" ...
                                     */
                                    prt->p_recover = prt->out.nxt;
                                }
#endif

                                /* RFC 2581 - section 3.2 - set cwnd to ssthresh
                                 * plus 3*SMSS.  This artificially "inflates" the
                                 * congestion window by the number of segments
                                 * (three) that have left the network and which the
                                 * receiver has buffered.
                                 */
                                prt->p_cwnd =
                                    prt->p_ssthresh +
                                    (TCP_FAST_RETRANS_DUP_ACKS * prt->p_smss);
                            }

                            /* Otherwise, the missing packet is not on our
                             * retransmission list.
                             */
                            else
                            {
                                NLOG_Error_Log("Could not find missing packet for Fast Retransmit",
                                               NERR_SEVERE, __FILE__, __LINE__);
                            }

                            TEST_TCP_Fast_Rx_Postconditions(tcp_buf, prt);
                        }

#if (INCLUDE_NEWRENO == NU_TRUE)

                        /* Decrement duplicate ACKs to keep the connection out
                         * of Fast Retransmit.
                         */
                        else if (!(prt->portFlags & TCP_SACK))
                        {
                            prt->p_dupacks --;

                            TEST_TCP_No_Fast_Rx();
                        }
#endif
                    }

#if (NET_INCLUDE_LMTD_TX == NU_TRUE)

                    /* If this is the first or second duplicate ACK received
                     * and the Limited Transmit algorithm is being used.
                     */
                    else if ( (prt->p_dupacks == 1) || (prt->p_dupacks == 2) )
                    {
                        /* If SACK is being used, ensure the SACK contains
                         * new info.
                         */
                        if ( (!(prt->portFlags & TCP_SACK)) ||
                             (prt->portFlags & TCP_NEW_SACK_DATA) )
                        {
                            /* Set the flag indicating that one segment can be
                             * transmitted, regardless of the congestion window.
                             */
                            prt->portFlags |= TCP_TX_LMTD_DATA;

                            /* Resume any tasks waiting to transmit data. */
                            if ( (prt->p_socketd >= 0) && (prt->p_socketd < TCP_MAX_PORTS) )
                            {
                                SCK_Resume_All(&SCK_Sockets[prt->p_socketd]->s_TXTask_List,
                                               0);
                            }
                        }

                        TEST_TCP_LmtdTX_Postconditions(prt);
                    }
#endif

                    /* RFC 2581 - section 3.2 - For each additional duplicate
                     * ACK received, increment cwnd by SMSS.  This artificially
                     * inflates the congestion window in order to reflect the
                     * additional segment that has left the network.
                     */
                    else if (prt->p_dupacks > TCP_FAST_RETRANS_DUP_ACKS)
                    {
                        prt->p_cwnd += prt->p_smss;

                        TEST_TCP_DupACK_Postconditions(prt);

                        /* RFC 3782 - section 3, step 4 - Transmit a segment,
                         * if allowed by the new value of cwnd and the receiver's
                         * advertised window.
                         */
                        if ( (prt->p_socketd >= 0) && (prt->p_socketd < TCP_MAX_PORTS) )
                        {
                            if((
                                (prt->out.size > prt->out.contain) &&
                                (prt->probeFlag == NU_CLEAR) &&
                                ((MAX_BUFFERS - MEM_Buffers_Used) > NET_FREE_BUFFER_THRESHOLD)
                               )
#if (NET_INCLUDE_LMTD_TX == NU_TRUE)
                            /* If the Limited Transmit flag is set, one more segment
                             * can be sent.
                             */
                               || (prt->portFlags & TCP_TX_LMTD_DATA)
#endif
                              )
                            {
                                SCK_Resume_All(&SCK_Sockets[prt->p_socketd]->s_TXTask_List, 0);
                            }
                        }
                    }
                }
            }

            /* Else set the number of consecutive duplicate ACKs to zero */
            else
                prt->p_dupacks = 0;
        }
#endif

        /* Make sure that the connection is still in the ESTAB state after
         * the call to TCP_ACK_Check. TCP_ACK_Check also checks for the RST
         * bit so there is the possibility that the connection was just reset.
         */
        if ( (prt->state == SEST) || (prt->state == SFW1) || (prt->state == SFW2) || (prt->state == STWAIT) )
        {
            /* If there is data that is queued to be sent and all previously
               sent data has been acknowledged, then send the queued data. */
            if ( (prt->xmitFlag == NU_SET) &&
                 (prt->out.nextPacket != NU_NULL) &&
                 (prt->out.nxt == prt->out.ack) )
            {
                /* Clear the timer event that would have sent the data. */
                TQ_Timerunset(CONTX, TQ_CLEAR_EXACT, (UNSIGNED)prt->pindex, 0);

                prt->out.tcp_flags |= TPUSH;

                /* Save a pointer to the next packet to send. Then move
                   the next pointer forward. nextPacket should always
                   be NULL after these steps. */
                nxtPkt = prt->out.nextPacket;
                prt->out.nextPacket = prt->out.nextPacket->next;

                /* Clear the event flag. */
                prt->xmitFlag = NU_CLEAR;

                /* Send the buffer. */
                if (TCP_Xmit(prt, nxtPkt) < 0)
                    NLOG_Error_Log("Failed to transmit buffer", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }

            TCP_Estab1986 (prt, buf_ptr, tlen, hlen);
        }
        else
        {
            /* The connection may have been reset, drop this invalid packet by
               placing it back onto the free list. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
        }
    }

    return (0);

} /* TCP_Do_Recv */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Parse_SYN_Options
*
*   DESCRIPTION
*
*       This function parses the options from an incoming SYN packet
*       and sets the corresponding values accordingly.
*
*   INPUTS
*
*       *prt                    Pointer to the tcp port list
*       *p                      Pointer to the tcp layer information
*       hlen                    The header length
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_INVALID_PARM         The contents of one of the options is
*                               invalid and a RESET should be sent.
*
*************************************************************************/
STATIC STATUS TCP_Parse_SYN_Options(TCP_PORT *prt, TCPLAYER *p, UINT16 hlen)
{
    UINT16  i, local_max_seg_length;
    UINT8   *data_ptr;
    INT     nIndex = 0;
    STATUS  status = NU_SUCCESS;

    /* check to see if the foreign address is in our local network */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (prt->portFlags & TCP_FAMILY_IPV6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    {
        local_max_seg_length =
            (UINT16)(prt->tp_route.rt_route->rt_entry_parms.rt_parm_device->dev_mtu  -
            (IP6_HEADER_LEN +
#if (INCLUDE_IPV4 == NU_TRUE)
             IP_HEADER_LEN +
#endif
             TCP_HEADER_LEN));

        /* If the destination is a node on this link, use the local
         * segment size.
         */
        if (PREFIX6_Match_Longest_Prefix(prt->tcp_faddrv6))
            prt->sendsize =
                (UINT16)(prt->tp_route.rt_route->rt_entry_parms.rt_parm_device->dev_mtu  -
                (IP6_HEADER_LEN +
#if (INCLUDE_IPV4 == NU_TRUE)
                 IP_HEADER_LEN +
#endif
                 TCP_HEADER_LEN));

        /* Otherwise, use the foreign segment size. */
        else
            prt->sendsize = FOREIGN_MAX_SEGMENT_LEN;
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

    {
        local_max_seg_length =
            (UINT16)(prt->tp_route.rt_route->rt_entry_parms.rt_parm_device->dev_mtu -
            (IP_HEADER_LEN + TCP_HEADER_LEN));

        if (IP_Localaddr(prt->tcp_faddrv4))
            prt->sendsize = local_max_seg_length;
        else
            prt->sendsize = FOREIGN_MAX_SEGMENT_LEN;
    }

#endif

    /* Get a pointer to data past the TCP header. */
    data_ptr = (UINT8 *)((UINT8 *)p + TCP_HEADER_LEN);

    /* if not end of option list */
    while ( (hlen > 20) && (data_ptr[nIndex] != 0) && ((UINT16)nIndex < (hlen-20)) )
    {
        /* Check header for maximum segment size option. */
        if (data_ptr[nIndex] == TCP_MSS_OPT)
        {
            if (data_ptr[nIndex + TCP_MSS_LEN_OFFSET] == TCP_MSS_LENGTH)
            {
                /* Extract the MSS */
                i = (UINT16)(GET16(data_ptr, (unsigned int)(nIndex + TCP_MSS_VALUE_OFFSET)));

                /* we have our own limits too */
                if (i <= local_max_seg_length)
                    prt->sendsize = i;

                /* Increment the index variable by the length of this
                 * option.
                 */
                nIndex += TCP_MSS_LENGTH;

            } /* end if */

            else
            {
                status = NU_INVALID_PARM;
                break;
            }

        } /* end if */

        /* If this is the NO OP operation. */
        else if (data_ptr[nIndex] == TCP_NOP_OPT)
            nIndex++;

#if (NET_INCLUDE_SACK == NU_TRUE)

        /* If this is a SACK-Permitted Option. */
        else if (data_ptr[nIndex] == TCP_SACK_PERM_OPT)
        {
            /* If the length of the option is valid. */
            if (data_ptr[nIndex + TCP_SACK_PERM_LEN_OFFSET] == TCP_SACK_PERM_LENGTH)
            {
                /* Set the flag indicating that the other side supports SACK. */
                prt->portFlags |= TCP_FOREIGN_SACK;

                /* Increment the index variable by the length of this
                 * option.
                 */
                nIndex += TCP_SACK_PERM_LENGTH;
            }

            else
            {
                status = NU_INVALID_PARM;
                break;
            }
        }

#endif

#if (NET_INCLUDE_WINDOWSCALE == NU_TRUE)

        /* If this is a Window Scale Option. */
        else if (data_ptr[nIndex] == TCP_WINDOWSCALE_OPT)
        {
            /* If the length of the option is valid. */
            if (data_ptr[nIndex + TCP_WINDOWSCALE_LEN_OFFSET] == TCP_WINDOWSCALE_LENGTH)
            {
                /* If the Window Scale option is enabled on the socket. */
                if (prt->portFlags & TCP_REPORT_WINDOWSCALE)
                {
                    /* Set the flag indicating that the other side supports the Window
                     * Scale option.
                     */
                    prt->portFlags |= TCP_FOREIGN_WINDOWSCALE;

                    /* RFC 1323 - section 2.3 - If a Window Scale option is received
                     * with a shift.cnt value exceeding 14, the TCP should log the
                     * error but use 14 instead of the specified value.
                     */
                    if (data_ptr[nIndex + TCP_WINDOWSCALE_SHIFT_OFFSET] <=
                        TCP_MAX_WINDOWSCALE_FACTOR)
                    {
                        /* Save the scale factor for the foreign side. */
                        prt->out.p_win_shift =
                            data_ptr[nIndex + TCP_WINDOWSCALE_SHIFT_OFFSET];
                    }

                    else
                    {
                        /* Use a scale factor of 14. */
                        prt->out.p_win_shift = TCP_MAX_WINDOWSCALE_FACTOR;

                        NLOG_Error_Log("Invalid shift count received in incoming SYN",
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);
                    }
                }

                /* Increment the index variable by the length of this
                 * option.
                 */
                nIndex += TCP_WINDOWSCALE_LENGTH;
            }

            else
            {
                status = NU_INVALID_PARM;
                break;
            }
        }

#endif

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

        /* If this is a Timestamp Option. */
        else if (data_ptr[nIndex] == TCP_TIMESTAMP_OPT)
        {
            /* If the length of the option is valid. */
            if (data_ptr[nIndex + TCP_TIMESTAMP_LEN_OFFSET] == TCP_TIMESTAMP_LENGTH)
            {
                /* If the Timestamp option is enabled on the socket. */
                if (prt->portFlags & TCP_REPORT_TIMESTAMP)
                {
                    /* Extract TSval from the packet and include it in the
                     * ACK for this packet.
                     */
                    prt->p_tsecr = GET32(&data_ptr[nIndex], TCP_TIMESTAMP_TSVAL_OFFSET);

                    /* Set the flag indicating that the other side supports the
                     * Timestamp option.
                     */
                    prt->portFlags |= TCP_FOREIGN_TIMESTAMP;

                    /* Decrement the send size by the length of the Timestamp
                     * option plus the required padding to avoid source node
                     * fragmentation.
                     */
                    prt->sendsize -= (TCP_TIMESTAMP_LENGTH + 2);

                    /* Set the length of the options that must be included
                     * with each packet.
                     */
                    prt->p_opt_len += (TCP_TIMESTAMP_LENGTH + 2);
                }

                /* Increment the index variable by the length of this
                 * option.
                 */
                nIndex += TCP_TIMESTAMP_LENGTH;
            }

            else
            {
                status = NU_INVALID_PARM;
                break;
            }
        }

#endif

        /* If this is some other option type with a valid length, skip over
         * the option.
         */
        else if (data_ptr[nIndex+1] != 0)
            nIndex = nIndex + data_ptr[nIndex+1];

        /* The option length is invalid.  Exit the loop and return an
         * error.
         */
        else
        {
            status = NU_INVALID_PARM;
            break;
        }
    }

#if (NET_INCLUDE_SACK == NU_TRUE)

    /* If the local side supports SACK, but the foreign side does not,
     * disable SACK on the port.
     */
    if ( (prt->portFlags & TCP_SACK) && (!(prt->portFlags & TCP_FOREIGN_SACK)) )
    {
        prt->portFlags &= ~TCP_SACK;
    }

#endif

#if (NET_INCLUDE_WINDOWSCALE == NU_TRUE)

    /* If the local side supports the Window Scale option, but the foreign side
     * does not, disable the Window Scale option on the port.
     */
    if ( (prt->portFlags & TCP_REPORT_WINDOWSCALE) &&
         (!(prt->portFlags & TCP_FOREIGN_WINDOWSCALE)) )
    {
        prt->portFlags &= ~TCP_REPORT_WINDOWSCALE;

        /* Set the scale factor to zero for both the receive and send
         * window.
         */
        prt->in.p_win_shift = 0;
        prt->out.p_win_shift = 0;

        /* Reset the receive window since the other side cannot scale. */
        if (prt->in.size > 65535)
        {
            prt->in.size = 65535;
            prt->credit = 65535;
        }
    }

#endif

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

    /* If the local side supports the Timestamp option, but the foreign side
     * does not, disable the Timestamp option on the port.
     */
    if ( (prt->portFlags & TCP_REPORT_TIMESTAMP) &&
         (!(prt->portFlags & TCP_FOREIGN_TIMESTAMP)) )
    {
        prt->portFlags &= ~TCP_REPORT_TIMESTAMP;
    }

#endif

    /* Set up the SMSS - the largest packet that can be sent.  Initially
     * this is set to the lessor of the other side's MSS and the Path
     * MTU for the route associated with the connection.
     */
    prt->p_smss = prt->sendsize;

#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)

    /* If the Path MTU associated with the route is less than the SMSS,
     * use the Path MTU associated with the route.
     */
#if (INCLUDE_IPV6 == NU_TRUE)

    if (prt->portFlags & TCP_FAMILY_IPV6)
    {
        if (prt->tp_routev6.rt_route->rt_path_mtu < prt->p_smss)
            prt->p_smss =
                (UINT16)(prt->tp_routev6.rt_route->rt_path_mtu -
                (IP6_HEADER_LEN +
#if (INCLUDE_IPV4 == NU_TRUE)
                IP_HEADER_LEN +
#endif
                TCP_HEADER_LEN + prt->p_opt_len));
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        if (prt->tp_route.rt_route->rt_path_mtu < prt->p_smss)
            prt->p_smss =
                (UINT16)(prt->tp_route.rt_route->rt_path_mtu -
                         (IP_HEADER_LEN + TCP_HEADER_LEN + prt->p_opt_len));
    }
#endif
#endif

#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)

    /* If congestion control is not disabled. */
    if (!(prt->portFlags & TCP_DIS_CONGESTION))
    {
        /* RFC 2581 - section 3.1 - The initial value of cwnd, MUST be less
         * than or equal to 2*SMSS bytes and MUST NOT be more than 2 segments.
         */
        prt->p_cwnd = prt->p_smss;

        /* RFC 2581 - section 3.1 - The initial value of ssthresh MAY be
         * arbitrarily high (for example, some implementations use the size
         * of the advertised window), but it may be reduced in response to
         * congestion.
         */
        prt->p_ssthresh = TCP_SLOW_START_THRESHOLD;
    }
#endif

    return (status);

} /* TCP_Parse_SYN_Options */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Reset_FIN
*
*   DESCRIPTION
*
*       Send a reset packet back to sender
*       Use the packet which just came in as a template to return to
*       sender.  Fill in all of the fields necessary and send it back.
*
*   INPUTS
*
*       *t                      Pointer to TCP layer information
*       *tcp_chk                Pointer to the pseudo tcp information
*       dlen                    The data length
*       family                  The family type of the other side of
*                               the connection.
*       flag_parm               RESET and/or FIN
*
*   OUTPUTS
*
*       INT16                   0
*                               1
*
*************************************************************************/
STATIC INT16 TCP_Reset_FIN(TCPLAYER *t, const VOID *tcp_chk, INT16 dlen,
                           INT16 family, UINT8 flag_parm)
{
    UINT16      tport;
    NET_BUFFER  *buf_ptr;
    TCPLAYER    *out_tcp;
    STATUS      stat;
    UINT8       flags;

#if (INCLUDE_IPV6 == NU_TRUE)
    IP6_S_OPTIONS       ip6_options;
#endif

#if ( (INCLUDE_IPV6 == NU_FALSE) || (INCLUDE_IPV4 == NU_FALSE) )
    UNUSED_PARAMETER(family);
#endif

    flags = GET8(t, TCP_FLAGS_OFFSET);

    if (flags & TRESET)                     /* don't reset a reset */
        return (1);

    /* We need to build a new packet to transmit this reset back to the
       sender.  Therefore, we must get the packet itself and we must acquire
       a header packet for the transmit operation (see below).  */

    /* Allocate a buffer to place the arp packet in. */
    buf_ptr = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

    if (buf_ptr == NU_NULL)
    {
        return (1);
    }

#if (INCLUDE_IPSEC == NU_TRUE)

    /* We do not know what the TCP port is for this connection at the
     * moment, so set it to NU_NULL for now.
     */
    buf_ptr->mem_port = NU_NULL;

#endif

    /* Initialize each field in the allocated buffer. */
    buf_ptr->data_len   = buf_ptr->mem_total_data_len = TCP_HEADER_LEN;
    buf_ptr->mem_dlist  = &MEM_Buffer_Freelist;

    /* Set the data pointer to the beginning of the buffer */
    buf_ptr->data_ptr = buf_ptr->mem_parent_packet;

    /* Set up a pointer to the packet. */
    out_tcp = (TCPLAYER*)(buf_ptr->data_ptr);

    /*  Now we start building the packet itself.  We move the data from
        the TCP packet that we received into our newly allocated packet.  */

    /*  Swap TCP layer portions for sending back. */
    if (flags & TACK)
    {
        PUT32(out_tcp, TCP_SEQ_OFFSET, GET32(t, TCP_ACK_OFFSET)); /* ack becomes next sequence # */
        PUT32(out_tcp, TCP_ACK_OFFSET, 0L);               /* ack # is 0 */
        PUT8(out_tcp, TCP_FLAGS_OFFSET, flag_parm);
    } /* end if */
    else
    {
        PUT32(out_tcp, TCP_SEQ_OFFSET, 0L);

        if (flags & TSYN)
            PUT32(out_tcp, TCP_ACK_OFFSET, GET32(t, TCP_SEQ_OFFSET) + (UINT16)dlen + 1);
        else
            PUT32(out_tcp, TCP_ACK_OFFSET, GET32(t, TCP_SEQ_OFFSET) + (UINT16)dlen);

        PUT8(out_tcp, TCP_FLAGS_OFFSET, (UINT8)(flag_parm | TACK));
    } /* end else */

    /* swap port #'s */
    tport = GET16(t, TCP_SRC_OFFSET);
    PUT16(out_tcp, TCP_SRC_OFFSET, GET16(t, TCP_DEST_OFFSET));
    PUT16(out_tcp, TCP_DEST_OFFSET, tport);
    PUT8(out_tcp, TCP_HLEN_OFFSET, TCP_HEADER_LEN << 2); /* header len */
    PUT16(out_tcp, TCP_WINDOW_OFFSET, 0);
    PUT16(out_tcp, TCP_URGENT_OFFSET, GET16(t, TCP_URGENT_OFFSET));
    PUT16(out_tcp, TCP_CHECK_OFFSET, 0);

#if ((INCLUDE_TCP_INFO_LOGGING == NU_TRUE) || (PRINT_TCP_MSG == NU_TRUE))
    /* Print the TCP header info */
    NLOG_TCP_Info(out_tcp, dlen, NLOG_TX_PACK);
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (family == SK_FAM_IP6)
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    {
        /* Note that the source and destination IP addresses in tcp_chk need to be
           reversed. The destination is our source and the source is our
           destination. */
        PUT16(out_tcp, TCP_CHECK_OFFSET,
              UTL6_Checksum(buf_ptr, ((struct pseudohdr *)(tcp_chk))->dest,
                            ((struct pseudohdr *)(tcp_chk))->source,
                            buf_ptr->mem_total_data_len, IP_TCP_PROT,
                            IP_TCP_PROT));

        memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));

        ip6_options.tx_dest_address = ((struct pseudohdr *)(tcp_chk))->source;
        ip6_options.tx_source_address = ((struct pseudohdr *)(tcp_chk))->dest;

        /* The default hop limit for the link will be used. */
        stat = IP6_Send(buf_ptr, &ip6_options, IP_TCP_PROT, NU_NULL, 0,
                        NU_NULL);
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* Note that the source and destination IP addresses in tcp_chk need to be
           reversed. The destination is our source and the source is our
           destination. */
        PUT16(out_tcp, TCP_CHECK_OFFSET,
              UTL_Checksum(buf_ptr, LONGSWAP(((struct pseudotcp *)(tcp_chk))->dest),
                           LONGSWAP(((struct pseudotcp *)(tcp_chk))->source),
                           IP_TCP_PROT) );

        /* Send this packet. */
        /* Note that the IP source addresses and IP destination addresses are
           swapped.*/
        stat = IP_Send(buf_ptr, NU_NULL, LONGSWAP(((struct pseudotcp *)(tcp_chk))->source),
                       LONGSWAP(((struct pseudotcp *)(tcp_chk))->dest), 0,
                       0, IP_TCP_PROT, IP_TYPE_OF_SERVICE, NU_NULL);
    }
#endif

    if (stat == NU_SUCCESS)
    {
        /* Increment the number of TCP segments transmitted. */
        MIB2_tcpOutSegs_Inc;

        /* Increment the number of TCP resets sent. */
        MIB2_tcpOutRsts_Inc;
    }

    else
    {
        /* The packet was not sent.  Deallocate the buffer.  If the packet was
           transmitted it will be deallocated later by TCP. */
        MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);
    }

    return (0);

} /* TCP_Reset_FIN */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Update_Headers
*
*   DESCRIPTION
*
*       Update the fields in the IP and TCP headers that change from
*       packet to packet.  Depending on the type parameter, call
*       either tcpsend or tcp_sendack.
*
*   INPUTS
*
*       *pport                  Pointer to the tcp port list
*       *buf_ptr                Pointer to the net buffer pointer
*       tcp_hlen                The tcp header length
*
*   OUTPUTS
*
*       INT16                   -1
*                               NU_SUCCESS
*
*************************************************************************/
INT16 TCP_Update_Headers(TCP_PORT *pport, NET_BUFFER *buf_ptr,
                         UINT16 tcp_hlen)
{
    TCPLAYER        *tcp_ptr;

#if ( (NET_INCLUDE_TIMESTAMP == NU_TRUE) || (NET_INCLUDE_SACK == NU_TRUE) )
    UINT8           opt_len = TCP_TOTAL_OPT_LENGTH;
#endif

    /* Overlay the TCP header. */
    tcp_ptr = (TCPLAYER *)buf_ptr->data_ptr;

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

    /* If the Timestamp option is enabled on this socket, include a
     * Timestamp Option in the packet.
     */
    if ( (!(pport->out.tcp_flags & TSYN)) && (!(pport->out.tcp_flags & TRESET)) &&
         (!(pport->out.tcp_flags & TFIN)) &&
         (pport->portFlags & TCP_REPORT_TIMESTAMP) )
    {
        /* Add the Timestamp Option. */
        tcp_hlen +=
            TCP_Build_Timestamp_Option(&buf_ptr->data_ptr[tcp_hlen], pport,
                                       &opt_len);
    }
#endif

#if (NET_INCLUDE_SACK == NU_TRUE)

    /* If the flag is set to include a SACK in the ACK. */
    if ( (!(pport->out.tcp_flags & TSYN)) &&
         (pport->out.tcp_flags & TACK) &&
         (pport->portFlags & TCP_REPORT_SACK) )
    {
        /* Build the SACK in the buffer. */
        tcp_hlen += TCP_Build_SACK_Option(&buf_ptr->data_ptr[tcp_hlen], pport,
                                          &opt_len);

        /* Clear the flag for future ACKs. */
        pport->portFlags &= ~TCP_REPORT_SACK;
    }

#if (NET_INCLUDE_DSACK == NU_TRUE)

    /* If the flag is set to include a D-SACK in the ACK. */
    else if ( (!(pport->out.tcp_flags & TSYN)) &&
              (pport->out.tcp_flags & TACK) &&
              (pport->portFlags & TCP_REPORT_DSACK) )
    {
        /* Build the D-SACK in the buffer. */
        tcp_hlen += TCP_Build_DSACK_Option(&buf_ptr->data_ptr[tcp_hlen], pport,
                                           &opt_len);

        /* Clear the flag for future ACKs. */
        pport->portFlags &= ~TCP_REPORT_DSACK;
    }

#endif
#endif

    /* If the header length is not a multiple of 4 bytes, pad it out. */
    while ((tcp_hlen % 4) != 0)
    {
        /* Add the NOP kind to the packet. */
        PUT8(&buf_ptr->data_ptr[tcp_hlen], TCP_MSS_NOP_OFFSET, TCP_NOP_OPT);

        /* Increment the length of the headers. */
        tcp_hlen ++;
    }

    /* Set the SEQ and ACK numbers. */
    PUT32(tcp_ptr, TCP_SEQ_OFFSET, pport->out.nxt);
    PUT32(tcp_ptr, TCP_ACK_OFFSET, pport->in.nxt);

    /* Save off the last ACK value that was sent. This will be used to
       determine when the next one should be sent in the case of a delayed ACK.
    */
    pport->in.ack = pport->in.nxt;

    /* Setup the source and destination port numbers. */
    PUT16(tcp_ptr, TCP_DEST_OFFSET, pport->out.port);
    PUT16(tcp_ptr, TCP_SRC_OFFSET, pport->in.port);

    /* Update the data length of the buffer. */
    buf_ptr->mem_total_data_len += tcp_hlen;
    buf_ptr->data_len           += tcp_hlen;
    buf_ptr->mem_option_len = tcp_hlen - TCP_HEADER_LEN;

    /* Set the tcp header len */
    PUT8(tcp_ptr, TCP_HLEN_OFFSET, (UINT8)(tcp_hlen << 2) );

   /*
    *  if the port has some credit limit, use it instead of large
    *  window buffer.  Generally demanded by hardware limitations.
    */
    if ((UINT32)(pport->credit) < (pport->in.size))
    {
        /* The Window field of a SYN segment itself is never scaled. */
        PUT16(tcp_ptr, TCP_WINDOW_OFFSET,
              (UINT16)(pport->out.tcp_flags & TSYN ?
                       pport->credit > 65535 ? 65535 : pport->credit :
                       pport->credit >> pport->in.p_win_shift));
    }
    else
    {
        /* The Window field of a SYN segment itself is never scaled. */
        PUT16(tcp_ptr, TCP_WINDOW_OFFSET,
              (UINT16)(pport->out.tcp_flags & TSYN ?
                       pport->in.size > 65535 ? 65535 : pport->in.size :
                       pport->in.size >> pport->in.p_win_shift));
    }

    /* Set the urgent pointer. */
    PUT16(tcp_ptr, TCP_URGENT_OFFSET, 0);

    /* Set the flags field. */
    PUT8(tcp_ptr, TCP_FLAGS_OFFSET, pport->out.tcp_flags);

    /* Clear the checksum field to 0. It will be computed later. */
    PUT16(tcp_ptr, TCP_CHECK_OFFSET, 0);

    return (NU_SUCCESS);

} /* TCP_Update_Headers */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Send_ACK
*
*   DESCRIPTION
*
*       Transmit an ACK packet or a packet that contains options.
*       ACKs work this way so that we don't have to allocate a buffer
*       for a packet that will contain no data.
*
*   INPUTS
*
*       *pport                  Pointer to the tcp port information
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful transmission
*       NU_NO_BUFFERS           Not enough buffers
*       NU_HOST_UNREACHABLE     The HOST cannot be accessed
*       NU_ACCESS               Access denied
*       NU_MSGSIZE              The size of the message is wrong
*
*************************************************************************/
INT16 TCP_Send_ACK(TCP_PORT *pport)
{
    TCPLAYER    *tcp_ptr;
    NET_BUFFER  *buf_ptr;
    STATUS      stat;

#if (INCLUDE_IPV6 == NU_TRUE)
    IP6_S_OPTIONS       ip6_options;
#endif

    /*  Clear the PUSH flag if no data to be sent. */
    if (!pport->out.contain)
        pport->out.tcp_flags &= ~TPUSH;

    /* Allocate a buffer to place the ack packet in. */
    buf_ptr = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

    if (buf_ptr == NU_NULL)
    {
        return (NU_NO_BUFFERS);
    }

#if (INCLUDE_IPSEC == NU_TRUE)

    /* Set the TCP port pointer. This will enable IPsec to use cached
     * information in the port structure when adding its IPsec headers.
     */
    buf_ptr->mem_port = pport;

#endif

    /* Initialize the deallocation list of this buffer. */
    buf_ptr->mem_dlist = &MEM_Buffer_Freelist;

    /* Set the data pointer to the beginning of the buffer */
    buf_ptr->data_ptr = buf_ptr->mem_parent_packet;

    /* Update the IP and TCP headers with the latest info.  This routine will
     * update the header lengths of the buffer too.
     */
    TCP_Update_Headers(pport, buf_ptr, TCP_HEADER_LEN);

    /* Point at the TCP header. */
    tcp_ptr = (TCPLAYER *)buf_ptr->data_ptr;

#if ((INCLUDE_TCP_INFO_LOGGING == NU_TRUE) || (PRINT_TCP_MSG == NU_TRUE))
    /* Print the TCP header info */
    NLOG_TCP_Info(tcp_ptr, (INT16)buf_ptr->data_len, NLOG_TX_PACK);
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (pport->portFlags & TCP_FAMILY_IPV6)
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    {
        memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));

        /* Set up the parameters to be used for the transmission */
        stat = IP6_Setup_Options(NU_NULL, pport->p_sticky_options,
                                 &ip6_options, pport->tcp_faddrv6,
                                 &pport->tp_routev6, (UINT8)pport->p_ttl);

        if (stat != NU_SUCCESS)
            return ((INT16)stat);

        ip6_options.tx_source_address = pport->tcp_laddrv6;

#if (HARDWARE_OFFLOAD == NU_TRUE)
        /* Do not perform checksum in software if hardware controller can do
         * it
         */
        if ((pport->tp_routev6.rt_route)&&(!(pport->tp_routev6.rt_route->rt_entry_parms.rt_parm_device->
              dev_hw_options_enabled & HW_TX_TCP6_CHKSUM)))
#endif
        {
            /* Note that the source and destination IP addresses in tcp_chk
               need to be reversed. The destination is our source and the
               source is our destination. */
            PUT16(tcp_ptr, TCP_CHECK_OFFSET,
                  UTL6_Checksum(buf_ptr, ip6_options.tx_source_address,
                                pport->tcp_faddrv6, buf_ptr->mem_total_data_len,
                                IP_TCP_PROT, IP_TCP_PROT) );
        }

        ip6_options.tx_dest_address = pport->tcp_faddrv6;

        stat = IP6_Send(buf_ptr, &ip6_options, IP_TCP_PROT, &ip6_options.tx_route,
                        0, NU_NULL);
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
#if (HARDWARE_OFFLOAD == NU_TRUE)
        /* Do not perform checksum in software if hardware controller can do
         * it
         */
        if ((pport->tp_route.rt_route)&&(!(pport->tp_route.rt_route->rt_entry_parms.rt_parm_device->
              dev_hw_options_enabled & HW_TX_TCP_CHKSUM)))
#endif
        {
            PUT16(tcp_ptr, TCP_CHECK_OFFSET,
                  UTL_Checksum(buf_ptr, pport->tcp_laddrv4, pport->tcp_faddrv4,
                               IP_TCP_PROT) );
        }

        /* Send this packet. */
        stat = IP_Send((NET_BUFFER *)buf_ptr, &pport->tp_route, pport->tcp_faddrv4,
                       pport->tcp_laddrv4, 0, pport->p_ttl,
                       IP_TCP_PROT, pport->p_tos, NU_NULL);
    }
#endif

    /* Increment the number of TCP segments transmitted. */
    if (stat == NU_SUCCESS)
        MIB2_tcpOutSegs_Inc;

    else
    {
        /* The packet was not sent.  Deallocate the buffer.  If the packet was
           transmitted it will be deallocated when transmission is complete. */
        MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);
    }

    return (NU_SUCCESS);

} /* TCP_Send_ACK */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Send
*
*   DESCRIPTION
*
*       Send a tcp packet.
*
*   INPUTS
*
*       *pport                  Pointer to the tcp port information
*       *buf_ptr                Pointer to the net buffer pointer list
*
*   OUTPUTS
*
*       The number of bytes sent.
*       NU_HOST_UNREACHABLE
*       NU_ACCESS
*       NU_MSGSIZE
*       NU_SUCCESS
*
*************************************************************************/
STATIC STATUS TCP_Send(TCP_PORT *pport, NET_BUFFER *buf_ptr)
{
    TCPLAYER        *tcp_ptr;
    STATUS          stat;

#if (INCLUDE_IPV6 == NU_TRUE)
    IP6_S_OPTIONS   ip6_options;
#endif

    /* Check to see if there is an ACK timer event set.  If so clear  it out
       because the data packet that is about to be sent will include the ack. */
    if (pport->portFlags & ACK_TIMER_SET)
    {
        /*  Delete the ACK timeout timer.  */
        TQ_Timerunset(TCPACK, TQ_CLEAR_EXACT, (UNSIGNED)pport->pindex, 0);

        /* Clear the ACK timer flag in the port. */
        pport->portFlags &= (~ACK_TIMER_SET);
    }

#if (INCLUDE_IPSEC == NU_TRUE)

    /* Set the TCP port pointer. This will enable IPsec to use cached
     * information in the port structure when adding its IPsec headers.
     */
    buf_ptr->mem_port = pport;

#endif

    /* Now add the TCP header and options to the size and update the data
       pointer to point to the header. */
    buf_ptr->data_ptr -= (TCP_HEADER_LEN + pport->p_opt_len);

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

    /* If the TCP timestamp option is included. */
    if (pport->portFlags & TCP_REPORT_TIMESTAMP)
    {
        /* If the SYN, RESET of FIN flags are set, then the timestamp
         * option should not be sent.
         */
        if ( (pport->out.tcp_flags & TSYN) || (pport->out.tcp_flags & TRESET) ||
             (pport->out.tcp_flags & TFIN) )
        {
            /* Adjust the data pointer by removing the timestamp option. */
            buf_ptr->data_ptr += (TCP_TIMESTAMP_LENGTH + 2);
        }
    }

#endif

    /* Update the header information.  This routine will update the lengths
     * in the buffer data structure too.
     */
    TCP_Update_Headers(pport, buf_ptr, TCP_HEADER_LEN);

    /* Calculate the next sequence number to be sent. */
    pport->out.nxt += buf_ptr->mem_tcp_data_len;

    /* Point to the beginning of the TCP header. */
    tcp_ptr = (TCPLAYER *)buf_ptr->data_ptr;

    /* Initialize the number of times this packet has been retransmitted. */
    buf_ptr->mem_retransmits = 0;

    /* If there is no timing being performed then time this transmission. */
    if (
#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)
        /* If the Timestamp Option is not being used to compute the RTT. */
        (!(pport->portFlags & TCP_REPORT_TIMESTAMP)) &&
#endif
        (pport->p_rtt == 0) )
    {
        pport->p_rtt = NU_Retrieve_Clock();
        pport->p_rtseq = buf_ptr->mem_seqnum;
    }

#if ((INCLUDE_TCP_INFO_LOGGING == NU_TRUE) || (PRINT_TCP_MSG == NU_TRUE))
    /* Print the TCP header info */
    NLOG_TCP_Info(tcp_ptr, (INT16)buf_ptr->data_len, NLOG_TX_PACK);
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (pport->portFlags & TCP_FAMILY_IPV6)
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    {
        memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));

        /* Set up the parameters to be used for the transmission */
        stat = IP6_Setup_Options(NU_NULL, pport->p_sticky_options,
                                 &ip6_options, pport->tcp_faddrv6,
                                 &pport->tp_routev6, (UINT8)pport->p_ttl);

        if (stat == NU_SUCCESS)
        {
            ip6_options.tx_source_address = pport->tcp_laddrv6;

#if (HARDWARE_OFFLOAD == NU_TRUE)
            /* Do not perform checksum in software if hardware controller can do
             * it
             */
            if ((pport->tp_routev6.rt_route)&&(!(pport->tp_routev6.rt_route->rt_entry_parms.rt_parm_device->
                  dev_hw_options_enabled & HW_TX_TCP6_CHKSUM)))
#endif
            {
                /* Compute and fill in the checksum */
                PUT16(tcp_ptr, TCP_CHECK_OFFSET,
                      UTL6_Checksum(buf_ptr, ip6_options.tx_source_address,
                                    pport->tcp_faddrv6, buf_ptr->mem_total_data_len,
                                    IP_TCP_PROT, IP_TCP_PROT));
            }

            ip6_options.tx_dest_address = pport->tcp_faddrv6;

            /* Send this packet */
            stat = IP6_Send(buf_ptr, &ip6_options, IP_TCP_PROT, &ip6_options.tx_route,
                            0, NU_NULL);
        }
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
#if (HARDWARE_OFFLOAD == NU_TRUE)
        /* Do not perform checksum in software if hardware controller can do
         * it
         */
        if ( (pport->tp_route.rt_route) &&
             (!(pport->tp_route.rt_route->rt_entry_parms.rt_parm_device->
              dev_hw_options_enabled & HW_TX_TCP_CHKSUM)) )
#endif
        {
            PUT16(tcp_ptr, TCP_CHECK_OFFSET,
                  UTL_Checksum(buf_ptr, pport->tcp_laddrv4, pport->tcp_faddrv4,
                               IP_TCP_PROT));
        }

        /* If the route is valid and the device is UP and RUNNING, send the
         * packet.
         */
        if ( (pport->tp_route.rt_route) &&
             (pport->tp_route.rt_route->rt_entry_parms.rt_parm_flags & RT_UP) &&
             ((pport->tp_route.rt_route->rt_entry_parms.rt_parm_device->dev_flags &
              (DV_UP | DV_RUNNING)) == (DV_UP | DV_RUNNING)) )
        {
            stat = IP_Send((NET_BUFFER *)buf_ptr, &pport->tp_route,
                           pport->tcp_faddrv4, pport->tcp_laddrv4, 0,
                           pport->p_ttl, IP_TCP_PROT, pport->p_tos, NU_NULL);
        }

        /* Otherwise, try to find another route and transmit the packet */
        else
        {
            /* Free the previously cached route */
            RTAB_Free((ROUTE_ENTRY*)pport->tp_route.rt_route, NU_FAMILY_IP);

            IP_Find_Route(&pport->tp_route);

            /* If there is another route, use it */
            if (pport->tp_route.rt_route != NU_NULL)
            {
                stat = IP_Send((NET_BUFFER *)buf_ptr, &pport->tp_route,
                               pport->tcp_faddrv4, pport->tcp_laddrv4, 0,
                               pport->p_ttl, IP_TCP_PROT, pport->p_tos, NU_NULL);
            }

            else
                stat = NU_HOST_UNREACHABLE;
        }
    }
#endif

    /* If this is the first packet on the head of the list, set the
     * retransmission timer as it is not running yet.
     */
    if (pport->out.packet_list.tcp_head->tcp_buf_ptr == buf_ptr)
    {
        if (TQ_Timerset(TCPRETRANS, (UNSIGNED)pport->pindex,
                        pport->portFlags & TCP_RTX_FIRED ? pport->p_rto :
                        pport->p_first_rto, 0) != NU_SUCCESS)
            NLOG_Error_Log("Failed to set timer for RETRANSMIT", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If the packet was not successfully transmitted by IP_Send, unset the
     * NET_TX_QUEUE flag.  This flag indicates that the buffer of data is
     * on the interface output queue.  However, since this packet was not
     * transmitted, it is not on the interface output queue.  When the
     * retransmission timer expires, the packet will only be transmitted
     * if this flag is not set.
     */
    if (stat != NU_SUCCESS)
        buf_ptr->mem_flags &= ~NET_TX_QUEUE;

    /* Increment the number of TCP segments transmitted. */
    MIB2_tcpOutSegs_Inc;

    return (stat);

} /* TCP_Send */

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Check_Timestamp
*
*   DESCRIPTION
*
*       This function checks the Timestamp option in the incoming packet.
*       If the option is valid, the TSval value is extracted and stored
*       in the port structure.  If the option is invalid, an ACK is
*       transmitted to the other side per RFC 1323 and an error returned.
*
*   INPUTS
*
*       *tcp_ptr                Pointer to the tcp packet
*       *prt                    Pointer to the tcp port information
*       *opt_ptr                Pointer to the Timestamp Option
*
*   OUTPUTS
*
*       NU_SUCCESS              The option is valid.
*       -1                      The option is invalid.
*
*************************************************************************/
STATIC STATUS TCP_Check_Timestamp(TCPLAYER *tcp_ptr, TCP_PORT *prt, UINT8 *opt_ptr)
{
    UINT8   opt_len, flags;
    UINT8   *pkt_ptr;
    STATUS  status = NU_SUCCESS;
    UINT32  tsval;

    /* If the Timestamp Option is being used, validate TSval. */
    if (prt->portFlags & TCP_REPORT_TIMESTAMP)
    {
        /* If a pointer to the option was not provided, get one now. */
        if (!opt_ptr)
        {
            /* Extract the length of the TCP options. */
            opt_len = (GET8(tcp_ptr, TCP_HLEN_OFFSET) >> 2) - TCP_HEADER_LEN;

            /* If there are options in the packet, look for a Timestamp
             * option.
             */
            if (opt_len)
            {
                pkt_ptr = (UINT8*)tcp_ptr;

                /* Find the timestamp option in the packet. */
                opt_ptr = TCP_Find_Option(&pkt_ptr[TCP_HEADER_LEN],
                                          TCP_TIMESTAMP_OPT, opt_len);
            }
        }

        /* If the timestamp option was found in the packet. */
        if (opt_ptr)
        {
            /* Exctract TSval from the packet. */
            tsval = GET32(opt_ptr, TCP_TIMESTAMP_TSVAL_OFFSET);

            /* If the TSval in this packet is less than the value in
             * the most recently received packet.
             */
            if (INT32_CMP(tsval, prt->p_tsecr) < 0)
            {
                /* If the connection has been idle for less than 24 days. */
                if ( (TQ_Check_Duetime(prt->out.lasttime +
                                       TCP_24_DAYS) != NU_NO_SUSPEND) &&
                     (TQ_Check_Duetime(prt->in.lasttime +
                                       TCP_24_DAYS) != NU_NO_SUSPEND) )
                {
                    /* Save off the flags so they can be restored. */
                    flags = prt->out.tcp_flags;

                    /* Set the ACK flag. */
                    prt->out.tcp_flags = TACK;

                    /* Respond with an ACK and drop the packet. */
                    TCP_ACK_It(prt, 1);

                    /* Restore the flags. */
                    prt->out.tcp_flags = flags;

                    /* Return an error so this packet is dropped. */
                    status = -1;
                }
            }

            /* If the value is valid, store it. */
            if (status == NU_SUCCESS)
            {
                /* RFC 1323 - If SEG.SEQ is equal to Last.ACK.sent */
                if (GET32(tcp_ptr, TCP_SEQ_OFFSET) == prt->in.ack)
                {
                    prt->p_tsecr = tsval;
                }

                /* Extract TSecr from the packet. */
                prt->p_rtt = GET32(opt_ptr, TCP_TIMESTAMP_TSECR_OFFSET);
            }
        }
    }

    return (status);

} /* TCP_Check_Timestamp */

#endif

/*************************************************************************
*
*   FUNCTION
*
*       TCP_ACK_Check
*
*   DESCRIPTION
*
*       Take an incoming packet and see if there is an ACK for the
*       outgoing side.  Use that ACK to dequeue outgoing data.
*
*   INPUTS
*
*       *prt                    Pointer to the tcp port information
*       *tcp_pkt                Pointer to the tcp packet
*
*   OUTPUTS
*
*       INT16                   1
*                               TCP_NO_ACK
*                               TCP_INVALID_ACK
*                               NU_SUCCESS
*
*************************************************************************/
STATIC INT16 TCP_ACK_Check(TCP_PORT *prt, TCPLAYER *tcp_pkt)
{
    UINT32      ak;
    INT16       status;

#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)
    INT32       new_cwnd;
#endif

#if ( (NET_INCLUDE_TIMESTAMP == NU_TRUE) || (NET_INCLUDE_SACK == NU_TRUE) )
    UINT8       opt_len;
    UINT8       *opt_ptr;
    UINT8       *pkt_ptr;
#endif

#if (INCLUDE_NEWRENO == NU_TRUE)
    INT32       bytes_acked;
    TCP_BUFFER  *tcp_buf;
    INT         i;
#endif

#if (INCLUDE_SR_SNMP == NU_TRUE)
    UINT8       tcp_laddr[IP_ADDR_LEN];
    UINT8       tcp_faddr[IP_ADDR_LEN];
#endif

    /* We received a reset */
    if (GET8(tcp_pkt, TCP_FLAGS_OFFSET) & TRESET)
    {
#if (TCPSECURE_DRAFT == NU_FALSE)
        /* Check if the sequence number is within the current window. */
        if ( (INT32_CMP(GET32(tcp_pkt, TCP_SEQ_OFFSET), prt->in.nxt) >= 0) &&
             (INT32_CMP(GET32(tcp_pkt, TCP_SEQ_OFFSET),
                        (prt->in.nxt+prt->in.size)) < 0) )
#else
        /* If the sequence number is exactly the next expected
         * sequence number, reset the connection.
         */
        if (GET32(tcp_pkt, TCP_SEQ_OFFSET) == prt->in.nxt)
#endif
        {
            /*  Indicate that we got a reset.  */
            NLOG_Error_Log("Connection reset by foreign host",
                            NERR_RECOVERABLE, __FILE__, __LINE__);

            if ( (prt->state == SCWAIT) || (prt->state == SEST) )
            {
                MIB2_tcpEstabResets_Inc;

#if (INCLUDE_SR_SNMP == NU_TRUE)

                if (prt->portFlags & TCP_FAMILY_IPV4)
                {
                    PUT32(tcp_laddr, 0, prt->tcp_laddrv4);
                    PUT32(tcp_faddr, 0, prt->tcp_faddrv4);

                    SNMP_tcpConnTableUpdate(SNMP_DELETE, SEST, tcp_laddr, (UNSIGNED)(prt->in.port),
                                            tcp_faddr, (UNSIGNED)(prt->out.port));
                }
#endif

                MIB2_tcpCurrEstab_Dec;
            }

            /* Mark the socket as disconnecting. */
            SCK_DISCONNECTING(prt->p_socketd);

            /* Indicate that the connection is closed.  */
            prt->state = SCLOSED;

            /* Cleanup the port structure. */
            TCP_Cleanup(prt);
        }

#if (TCPSECURE_DRAFT == NU_TRUE)

        /* Otherwise, if the sequence number is within the acceptable window
         * (RCV.NXT < SEG.SEQ < RCV.NXT + RCV.WND), send an acknowledgement.
         */
        else if ( (INT32_CMP(GET32(tcp_pkt, TCP_SEQ_OFFSET), prt->in.nxt) > 0) &&
                  (INT32_CMP(GET32(tcp_pkt, TCP_SEQ_OFFSET),
                             (prt->in.nxt + prt->in.size)) < 0) )
            TCP_ACK_It(prt, 1);
#endif

        return (1);

    } /* end if */

    /* If we did not get an ACK then return.  */
    if (!(GET8(tcp_pkt, TCP_FLAGS_OFFSET) & TACK))
        return (TCP_NO_ACK);

    /* Pick up the other side's ack number.  */
    ak = GET32(tcp_pkt, TCP_ACK_OFFSET);

    /* If the ACK is greater the next sequence number we will send
     * then it is not a valid ACK. So send an ACK and return an error.
     */
    if (INT32_CMP(ak, prt->out.nxt) > 0)
    {
        /* Set the ACK flag. */
        prt->out.tcp_flags = TACK;

        /* Send the ACK. */
        TCP_ACK_It(prt, 1);

        /* Return error. */
        return (TCP_INVALID_ACK);
    }

#if (NET_INCLUDE_SACK == NU_TRUE)

    /* If SACK is being used on the socket, search the packet for the
     * SACK option.
     */
    if (prt->portFlags & TCP_SACK)
    {
        /* Extract the length of the TCP options. */
        opt_len = (GET8(tcp_pkt, TCP_HLEN_OFFSET) >> 2) - TCP_HEADER_LEN;

        /* If there are options in the packet, look for a SACK option. */
        if (opt_len)
        {
            /* Get a pointer to the beginning of the TCP options. */
            pkt_ptr = (UINT8*)tcp_pkt + TCP_HEADER_LEN;

            /* Find a SACK option in the incoming packet. */
            opt_ptr = TCP_Find_Option(pkt_ptr, TCP_SACK_OPT, opt_len);

            /* If a SACK option was found. */
            if (opt_ptr)
            {
                TEST_TCP_DSACK_Included(opt_ptr, opt_len, ak);

#if (NET_INCLUDE_LMTD_TX == NU_TRUE)

                /* Clear the new SACK flag. */
                prt->portFlags &= ~TCP_NEW_SACK_DATA;

#endif

                /* Set the SACK flag for all packets covered by the SACK
                 * option.
                 */
                TCP_SACK_Flag_Packets(opt_ptr, prt);
            }
        }
    }

#endif

    /* Check that the round-trip time is being computed for this packet.  If
     * the packet has been retransmitted, then the rtt is not being computed
     * for this packet.  Also check that the ACK number of the packet received
     * is greater than the sequence number of the packet for which the rtt is
     * being computed.  Since the sequence number for the packet being timed
     * is recorded in the p_rtseq member, the ACK number should be that sequence
     * number plus the number of bytes of data in the packet transmitted, or more
     * if this packet is ACKing more than one received packet.  This will ensure
     * that an ACK for retransmitted data does not incorrectly update the rtt value.
     */
    if ( (prt->p_rtt) && (INT32_CMP(ak, prt->p_rtseq) > 0) )
    {
        TCP_Xmit_Timer(prt, NU_Retrieve_Clock() - prt->p_rtt);
    }

    /* Pick-up the other side's maximum transmission size and agree
     * with him.
     */
    prt->out.size =
        GET16(tcp_pkt, TCP_WINDOW_OFFSET) << prt->out.p_win_shift;

    /* If the advertised window size > 0, and the local host is currently
     * probing the foreign host, stop probing.
     */
    if ( (prt->out.size > 0) && (prt->probeFlag == NU_SET) )
    {
        prt->probeFlag = NU_CLEAR;

        /* Stop the timer */
        TQ_Timerunset(WINPROBE, TQ_CLEAR_EXACT, prt->pindex, 0);
    }

    /* Remove any bytes which have been ACKed from the packet list,
     * update prt->out.nxt to the new next sequence number for outgoing.
     * Update send window.
     */
#if (INCLUDE_NEWRENO == NU_TRUE)
    bytes_acked =
#endif
    TCP_Rmqueue(&prt->out, ak);

    /* If the ACK is increasing */
    if (INT32_CMP(ak, prt->out.ack) > 0)
    {
#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)

        /* If congestion control is not disabled. */
        if (!(prt->portFlags & TCP_DIS_CONGESTION))
        {
            /* If the connection is in Fast Recovery. */
            if (prt->p_dupacks >= TCP_FAST_RETRANS_DUP_ACKS)
            {
#if (NET_INCLUDE_SACK == NU_TRUE)

                /* If SACK is being used on the connection. */
                if (prt->portFlags & TCP_SACK)
                {
                    prt->p_cwnd = prt->p_ssthresh;
                }

#if (INCLUDE_NEWRENO == NU_TRUE)
                else
#endif
#endif

#if (INCLUDE_NEWRENO == NU_TRUE)

                /* RFC 3782 - section 3, step 5 - If the ACK acknowledges
                 * all data up to and including recover.
                 */
                if (ak >= prt->p_recover)
                {
                    /* Set the congestion window to
                     * min(ssthresh, FlightSize + SMSS)
                     */
                    prt->p_cwnd = prt->p_ssthresh < (prt->out.contain + prt->p_smss) ?
                        prt->p_ssthresh : (prt->out.contain + prt->p_smss);

                    TEST_TCP_Full_ACK(prt);

                    /* Exit Fast Recovery. */
                    prt->p_dupacks = 0;
                }

                /* RFC 3782 - section 3, step 5 - If this ACK does *not*
                 * acknowledge all of the data up to and including "recover",
                 * then this is a partial ACK.
                 */
                else
                {
                    /* Get a pointer to the head of the retransmission
                     * list.
                     */
                    tcp_buf = prt->out.packet_list.tcp_head;

                    /* Retransmit the first TCP_PARTIAL_ACK_RETRANS_COUNT
                     * unacknowledged segments.
                     */
                    for (i = 0;
                         i < TCP_PARTIAL_ACK_RETRANS_COUNT && tcp_buf;
                         i++)
                    {
                        if (TCP_Send_Retransmission(tcp_buf->tcp_buf_ptr,
                                                    prt) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to retransmit packet",
                                           NERR_SEVERE, __FILE__, __LINE__);
                        }

                        tcp_buf = tcp_buf->tcp_next;
                    }

                    /* Reduce the congestion window by the amount of new data
                     * acknowledged by the cumulative acknowledgement field.
                     */
                    prt->p_cwnd -= bytes_acked;

                    /* If the ACK acknowledges at least one SMSS of new data. */
                    if (bytes_acked >= prt->p_smss)
                    {
                        /* Add SMSS bytes of data back into the congestion window. */
                        prt->p_cwnd += prt->p_smss;
                    }

                    TEST_TCP_Partial_ACK_Postconditions(prt);

                    /* RFC 3782 - section 3, step 5 - Send a new segment if
                     * permitted by the new value of cwnd.
                     */
                    SCK_Resume_All(&SCK_Sockets[prt->p_socketd]->s_TXTask_List,
                                   0);
                }
#else
                prt->p_cwnd = prt->p_ssthresh;
#endif
            }

            /* Slow Start & Congestion Avoidance */
            else
            {
                /* RFC 2581 - section 3.1 - The congestion avoidance algorithm
                 * is used when cwnd > ssthresh.  Slow start ends when cwnd
                 * exceeds ssthresh or when congestion is observed.
                 */
                if (prt->p_cwnd > prt->p_ssthresh)
                {
                    /* RFC 2581 - section 3.1 - During Congestion Avoidance,
                     * cwnd is incremented by 1 full sized segment per
                     * round-trip time (RTT).
                     */
                    new_cwnd = (prt->p_smss * prt->p_smss) / prt->p_cwnd;

                    /* RFC 2581 - section 3.1 - If the above formula yields zero,
                     * the result should be rounded up to 1 byte.
                     */
                    if (new_cwnd == 0)
                        new_cwnd = 1;

                    /* If incrementing the congestion window will not wrap the
                     * value, increment.
                     */
                    if ((prt->p_cwnd + (UINT32)new_cwnd) > prt->p_cwnd)
                        prt->p_cwnd += (UINT32)new_cwnd;
                }

                /* RFC 2581 - section 3.1 - The slow start algorithm is used
                 * when cwnd <= ssthresh.  When cwnd and ssthresh are equal
                 * the sender may use either slow start or congestion avoidance.
                 * During slow start, a TCP increments cwnd by at most SMSS
                 * bytes for each ACK received that acknowledges new data.
                 */
                else
                    prt->p_cwnd += prt->p_smss;
            }

            /* Reset duplicate ACKs since a non-duplicate ACK has been
             * received
             */
            prt->p_dupacks = 0;
        }

#endif

        /* Unset the retransmission timer. */
        TQ_Timerunset(TCPRETRANS, TQ_CLEAR_ALL_EXTRA, (UNSIGNED)prt->pindex, 0);

        /* RFC 2988 - section 5.3 - When an ACK is received that acknowledges
         * new data, restart the retransmission timer so that it will expire
         * after RTO seconds (for the current value of RTO).  If the only data
         * on the port is the data being built in nextPacket, do not set the
         * retransmission timer, as this data has not yet been sent out the
         * port.
         */
        if ( (prt->out.packet_list.tcp_head) &&
             (prt->out.packet_list.tcp_head->tcp_buf_ptr != prt->out.nextPacket) )
        {
            if (TQ_Timerset(TCPRETRANS, (UNSIGNED)prt->pindex,
                            prt->portFlags & TCP_RTX_FIRED ? prt->p_rto :
                            prt->p_first_rto, 0) != NU_SUCCESS)
                NLOG_Error_Log("Failed to set timer for RETRANSMIT", NERR_SEVERE,
                               __FILE__, __LINE__);
        }

#if (INCLUDE_NEWRENO == NU_TRUE)

        if (!(prt->portFlags & TCP_SACK))
        {
            /* Save off the previous highest ACK. */
            prt->p_prev_highest_ack = prt->out.ack;
        }

#endif

        prt->out.ack = ak;

#if (INCLUDE_IPV6 == NU_TRUE)

        /* RFC 4861 section 7.3.1 - ... receipt of a (new) acknowledgement
         * indicates that previously sent data reached the peer.
         */
        if (prt->portFlags & TCP_FAMILY_IPV6)
            NUD6_Confirm_Reachability_By_IP_Addr(prt->tcp_faddrv6);
#endif

        status = NU_SUCCESS;
    }

    /* Otherwise, the ACK is not increasing (above prt->out.ack). We
     * should assume that it is a duplicate packet or a keepalive packet
     * that 4.2 sends out.
     */
    else
        status = 1;

    /* If there is a task waiting for the window to open up, restart it.
     * The task could be waiting for the other side to advertise a nonzero
     * window, or the task could be suspended waiting for all data in the
     * congestion window to be ACKed.  If the task is suspended waiting for
     * the congestion window to open, and the other side's window size goes
     * to zero in the process of ACKing it, restart the sending task so
     * it can probe the other side.  Otherwise, the task will remain suspended
     * until the sender advertises a nonzero window size, and since this ACK
     * could get lost, the task may never be resumed.
     */
    if ( (((prt->out.size - prt->out.contain) > 0) ||
           ((prt->probeFlag == NU_CLEAR) && (prt->out.size == 0) &&
            (prt->out.nxt == prt->out.ack))) &&
          (prt->p_socketd >= 0) &&
          (SCK_Sockets[prt->p_socketd]) &&
          (SCK_Sockets[prt->p_socketd]->s_state & SS_WAITWINDOW) )
    {
        /* Reset the flag. */
        SCK_Sockets[prt->p_socketd]->s_state &= ~SS_WAITWINDOW;

        /* Restart any tasks pending on the mem buffer suspension / TX list */
        SCK_Resume_All(&SCK_Sockets[prt->p_socketd]->s_TXTask_List, SCK_RES_BUFF);
    }

    return (status);

} /* TCP_ACK_Check */

#if ( (NET_INCLUDE_DSACK == NU_TRUE) && (INCLUDE_TCP_OOO == NU_TRUE) )

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Find_Duplicate_Data
*
*   DESCRIPTION
*
*       This function determines if there is duplicate data on the out
*       of order list covering the incoming packet, and if so,
*       computes the left and right edge for the outgoing D-SACK option.
*
*   INPUTS
*
*       *prt                    Pointer to the TCP port information
*       seq                     The sequence number of the incoming
*                               packet.
*       dlen                    The length of the incoming packet.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID TCP_Find_Duplicate_Data(TCP_PORT *prt, UINT32 seq, UINT16 dlen)
{
    NET_BUFFER  *curr_buf, *saved_buf = NU_NULL;

    /* Set the left edge to the sequence number received. */
    prt->left_edge = seq;

    /* Get a pointer to the first packet on the Out of Order List. */
    curr_buf = prt->in.ooo_list.head;

    /* Since the Out of Order list is ordered, if the sequence number of
     * the first buffer is greater than the incoming sequence number,
     * then this data is not contained in the list already.
     */
    if (curr_buf->mem_seqnum <= seq)
    {
        /* While there are packets on the Out of Order List. */
        while (curr_buf)
        {
            /* If the sequence number of this segment is greater than
             * the right edge of the incoming packet exit the loop.
             */
            if (curr_buf->mem_seqnum > (seq + dlen))
            {
                break;
            }

            /* Save the current buffer. */
            saved_buf = curr_buf;

            /* Move on to the next buffer. */
            curr_buf = curr_buf->next;
        }

        /* If a buffer was found. */
        if ( (saved_buf) &&
             ((saved_buf->mem_seqnum + saved_buf->mem_tcp_data_len) > seq) )
        {
            /* If this buffer covers more data than was sent in the
             * incoming packet, truncate the right edge at the length
             * of the incoming packet.
             */
            if ((saved_buf->mem_seqnum + saved_buf->mem_tcp_data_len) >
                (seq + dlen))
            {
                prt->right_edge = seq + dlen;
            }

            /* Otherwise, the right edge is the sequence number plus length
             * of this packet on the Out of Order list.
             */
            else
            {
                prt->right_edge =
                    saved_buf->mem_seqnum + saved_buf->mem_tcp_data_len;
            }

            /* Indicate that a DSACK should be built. */
            prt->portFlags |= TCP_REPORT_DSACK;
        }
    }

} /* TCP_Find_Duplicate_Data */

#endif

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Estab1986
*
*   DESCRIPTION
*
*       Take a packet which has arrived for an established connection and
*       put it where it belongs.
*
*   INPUTS
*
*       *prt                    Pointer to the TCP port information
*       *buf_ptr                Pointer to the net buffer pointer
*       tlen                    The total data length
*       hlen                    The header length
*
*   OUTPUTS
*
*       INT16                   0
*                               -1
*
*************************************************************************/
STATIC INT16 TCP_Estab1986(TCP_PORT *prt, NET_BUFFER *buf_ptr, UINT16 tlen,
                           UINT16 hlen)
{
    UINT16      dlen;
    UINT32      sq, want;
    STATUS      status;
    TCPLAYER    *pkt;
    UINT32      to_drop = 0;
    UINT16      MEM_Available;
    struct SCK_TASK_ENT *task_entry;    /* task entry for list operations */

    /*  Calculate the length of the data received.  */
    buf_ptr->mem_tcp_data_len = dlen = (UINT16)(tlen - hlen);

    /* Get a pointer to the TCP header. */
    pkt = (TCPLAYER *)(buf_ptr->data_ptr - hlen);

    MEM_Available = (UINT16)(MAX_BUFFERS - MEM_Buffers_Used);

    if ( (MEM_Available <= 4) && (dlen >0) )
    {
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
        return (0);
    }

    /*  See if we want this packet, or is it a duplicate?  */

    /*  Get the sequence number.  */
    sq = GET32(pkt, TCP_SEQ_OFFSET);

    /*  See what we were expecting.  */
    /* If FIN has placed us in STWAIT state, it was already counted in. */
    want = prt->state == STWAIT ? (prt->in.nxt - 1) : prt->in.nxt;

    /*  If the sequence number is not what we expected, then they may
     *  have still sent something, just less than we had hoped for.
     */
    if (sq != want)
    {
        /*  If the sequence number is less than what was expected,
         *  but the sequence number plus data length is greater than expected,
         *  then extract that data which is new.   */
        if ( (INT32_CMP(sq, want) < 0) && (INT32_CMP((sq+dlen), want) > 0) )
        {
            /* This will work even when the sequence numbers have wrapped. */
            to_drop = want - sq;

            /* Trim the duplicate data from this packet. */
            MEM_Trim(buf_ptr, (INT32)to_drop);

            /*  Only take what we want, not what we have already received.  */
            dlen = (UINT16)(dlen - to_drop);
        } /* end if */

        /* If this is not the packet expected, but it is within the the current
           window, then place it in the out of order list. */
        else if ( (INT32_CMP(sq, want) > 0) &&
                  (INT32_CMP((sq+dlen), (want + prt->in.size)) <= 0) )
        {
#if (INCLUDE_TCP_OOO == NU_TRUE)

            /* Only insert this packet in the out of order list if it contains
               data, or if it contains a FIN flag.
             */
            if (dlen || (GET8(pkt, TCP_FLAGS_OFFSET) & TFIN) )
            {
#if (NET_INCLUDE_DSACK == NU_TRUE)

                /* Determine how much of this data is duplicate so a D-SACK
                 * can be sent with the proper left and right edges.
                 */
                if ( (dlen) && (prt->portFlags & TCP_DSACK) &&
                     (prt->in.ooo_list.head) )
                {
                    TCP_Find_Duplicate_Data(prt, sq, dlen);
                }

#endif

                /* Schedule an ACK to be sent immediately. */
                TCP_Schedule_ACK(prt, pkt, dlen, 1);

                /* don't let OOO packets use up all the buffers. */
                if ((MAX_BUFFERS - MEM_Buffers_Used) > NET_FREE_BUFFER_THRESHOLD)
                {
                    status = TCP_OOO_Packet(prt, sq);

                    if (status != NU_SUCCESS)
                        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
                }

                else
                {
                    MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);
                    return (0);
                }
            }
            else    /* Release the buffer space. */
#endif
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (0);
        }
        else
        {
#if (NET_INCLUDE_DSACK == NU_TRUE)

            /* If D-SACK is enabled on the socket. */
            if (prt->portFlags & TCP_DSACK)
            {
                /* Set the flag to include a D-SACK in the ACK. */
                prt->portFlags |= TCP_REPORT_DSACK;

                /* Set the left and right edge of this packet so it
                 * can be included in the D-SACK.
                 */
                prt->left_edge = sq;
                prt->right_edge = sq + dlen;
            }
#endif

            /*  Somehow the other side missed the ACK that we have already
                sent for this packet.  RFC-1122 recommends sending an ACK
                in this case.  */
            TCP_Schedule_ACK(prt, pkt, dlen, 1);

            /* Discard the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (-1);
        } /* end else */
    } /* end if */

    else  /* Sequence number is equal to what we expect.  Check for data. */
    {
        /*  If we did not receive any data, then check for a FIN bit.  If
         *  we did not receive data, we will just throw the rest away, it
         *  was probably an ACK.
         */
        if (dlen == 0)
        {
            /* See if he sent us a FIN bit. */
            TCP_Check_FIN(prt,pkt);

            /* Discard the packet */
            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            /* Get out since there is no data to process.  */
            return (0);
        } /* end if */
    }

#if (INCLUDE_IPV6 == NU_TRUE)

    /* RFC 4861 section 7.3.1 - ... the arrival of new (non-duplicate)
     * data indicates that earlier acknowledgements are being delivered
     * to the remote peer.
     */
    if ( (dlen > 0) && (prt->portFlags & TCP_FAMILY_IPV6) )
        NUD6_Confirm_Reachability_By_IP_Addr(prt->tcp_faddrv6);

#endif

    /*  If we have room in the window, update the ACK field values.  */
    if ((prt->in.size >= dlen) && (dlen > 0) )
    {
        /*  Calculate the new ack value.  */
        prt->in.nxt += dlen;

        /*  Update the window size.  */
        prt->in.size -= dlen;

        if (SCK_Sockets[prt->p_socketd]->s_state & SS_CANTRCVMORE)
        {
            /* Drop the data as the socket cannot read. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        }
        else
        {
            /* Put the data into the input TCP buffer for the user. */
            /* In most cases the hlen - 20 in the call below will evaluate to zero.
             * The one exception will be when a packet is received that contains
             * both old and new data.  In this case hlen has been incremented by the
             * number of old bytes in the packet up above. */

            TCP_Enqueue(SCK_Sockets[prt->p_socketd]);
            
            /* Trace log */
            T_SOCK_ENQ(SCK_Sockets[prt->p_socketd]->s_recvpackets, SCK_Sockets[prt->p_socketd]->s_recvbytes, prt->p_socketd);
        }

#if (INCLUDE_TCP_OOO == NU_TRUE)

        /* Are there any packets in the out of order list. */
        if (prt->in.ooo_list.head != NU_NULL)
        {
            TCP_Check_OOO_List(prt);
        }
#endif

#if (NET_INCLUDE_DSACK == NU_TRUE)

        /* If D-SACK is enabled on the socket and new data was received in a
         * segment with old data, and we are not up to date with ACKs on the
         * connection due to outstanding out of order packets.
         */
        if ( (to_drop) && (prt->in.ooo_list.head) && (prt->portFlags & TCP_DSACK) )
        {
            /* Set the flag to include a D-SACK in the ACK. */
            prt->portFlags |= TCP_REPORT_DSACK;

            /* Set the left and right edge of this packet so it
             * can be included in the D-SACK.
             */
            prt->left_edge = sq;
            prt->right_edge = sq + to_drop;
        }
#endif

        /* Schedule an ACK to be sent. */
        TCP_Schedule_ACK(prt, pkt, dlen, 0);

        /* Update the last time that we received something from the other
           side. */
        prt->in.lasttime = NU_Retrieve_Clock();

        /* Check the FIN bit to see if this connection is closing. */
        TCP_Check_FIN(prt, pkt);

        /* Send a message to to let the user know there is data.  */
        if (SCK_Sockets[prt->p_socketd]->s_RXTask_List.flink != NU_NULL)
        {
            task_entry = DLL_Dequeue(&SCK_Sockets[prt->p_socketd]->s_RXTask_List);

            status = NU_Resume_Task(task_entry->task);

            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to resume task", NERR_SEVERE,
                               __FILE__, __LINE__);

                NET_DBG_Notify(status, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);
            }
        }

    } /* end if */

    /*  We have no room to receive the data. */
    else if (dlen > 0)
    {
        /* RFC 793 - section 3.7 - When the receiving TCP has a zero window
         * and a segment arrives it must still send an acknowledgment showing
         * its next expected sequence number and current window (zero).
         */
        TCP_Schedule_ACK(prt, pkt, dlen, 1);

        MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);
    }

    else
    {
        /*  Check the FIN bit to see if this connection is closing. */
        TCP_Check_FIN(prt, pkt);

        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
    }

    /*  Got here, so everything is OK. */
    return (0);

} /* TCP_Estab1986 */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Find_Option
*
*   DESCRIPTION
*
*       This function finds the specified option in the TCP Header.
*
*   INPUTS
*
*       *tcp_pkt                Pointer to the TCP header to search.
*       opt_type                The target option type to find.
*       opt_len                 The total length of options in the packet.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
UINT8 *TCP_Find_Option(UINT8 *pkt_ptr, UINT8 opt_type, UINT8 opt_len)
{
    UINT16  nIndex = 0;
    UINT8   *rtrn_ptr = NU_NULL;

    /* While there are options left to parse. */
    while (nIndex < opt_len)
    {
        /* If this is a SACK option. */
        if (pkt_ptr[nIndex] == opt_type)
        {
            break;
        }

        /* If this is the NO OP option. */
        if (pkt_ptr[nIndex] == TCP_NOP_OPT)
            nIndex++;

        /* If this is some other option type with a valid length, skip over
         * the option.
         */
        else if (pkt_ptr[nIndex+1] != 0)
            nIndex = nIndex + pkt_ptr[nIndex+1];

        /* The option length is invalid.  Exit the loop. */
        else
            break;
    }

    /* If the option was found, return a pointer to the option in the
     * packet.
     */
    if (nIndex < opt_len)
    {
        rtrn_ptr = &pkt_ptr[nIndex];
    }

    return (rtrn_ptr);

} /* TCP_Find_Option */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Check_FIN
*
*   DESCRIPTION
*
*       Check the FIN bit of an incoming packet to see if the connection
*       should be closing, ACK it if we need to.
*       Half open connections immediately, automatically close.  We do
*       not support them.  As soon as the incoming data is delivered, the
*       connection will close.
*
*   INPUTS
*
*       *prt                    Pointer to the tcp port information
*       *pkt                    Pointer to tcp layer information
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID TCP_Check_FIN(TCP_PORT *prt, TCPLAYER *pkt)
{
    /* FIN has already been handled in case of active-close */

    if ((GET8(pkt, TCP_FLAGS_OFFSET) & TFIN) && (prt->state != STWAIT))
    {
#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

        /* Check the Timestamp option. */
        if (TCP_Check_Timestamp(pkt, prt, NU_NULL) != NU_SUCCESS)
        {
            /* Drop the packet. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return;
        }
#endif

        /* fin bit found */
        prt->in.nxt++;                               /* count the FIN byte */
        prt->state = SCWAIT;                         /* close-wait */
        prt->credit = 0;

        /* Mark the socket state as disconnecting. */
        SCK_DISCONNECTING(prt->p_socketd);

        /*  At this point, we know that we have received all data that the
         *  other side is allowed to send.  Some of that data may still be in
         *  the incoming queue.  As soon as that queue empties, finish off the
         *  TCP close sequence.  We are not allowing the user to utilize a half-
         *  open connection, but we cannot close before the user has received
         *  all of the data from the incoming queue.
         */

        /* Acknowledge the FIN */
        TCP_ACK_It(prt, 1);

        /* Check to see if a window probe is in progress.  If so, stop the
         * probes and resume the waiting task.
         */
        if (prt->probeFlag == NU_SET)
        {
            prt->probeFlag = NU_CLEAR;

            /* Stop the timer */
            TQ_Timerunset(WINPROBE, TQ_CLEAR_EXACT, prt->pindex, 0);

            /* If the TX socket is blocking for the window to open up,
             * resume it now.
             */
            if (SCK_Sockets[prt->p_socketd]->s_state & SS_WAITWINDOW)
            {
                /* Reset the flag. */
                SCK_Sockets[prt->p_socketd]->s_state &= ~SS_WAITWINDOW;

                SCK_Resume_All(&SCK_Sockets[prt->p_socketd]->s_TXTask_List,
                               SCK_RES_BUFF);
            }
        }

        /* If there is a task waiting to receive data resume him. */
        SCK_Resume_All(&SCK_Sockets[prt->p_socketd]->s_RXTask_List, 0);

    } /* end if */

} /* TCP_Check_FIN */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Find_Empty_Port
*
*   DESCRIPTION
*
*       Searches a single task table structure to see if there is a port
*       list entry that is empty - this indicates that the server is
*       willing to accept an additional connection request
*
*   INPUTS
*
*       *Task_Entry             Pointer to the task entry structure list
*
*   OUTPUTS
*
*       INT16                   NU_IGNORE_VALUE
*                               Index to next empty port.
*
*************************************************************************/
STATIC INT16 TCP_Find_Empty_Port(const struct TASK_TABLE_STRUCT *Task_Entry)
{
    INT16 tempIndex;  /* to traverse the port entries in the task table */
    UINT16 counter;    /* tracks number of entries checked */

    /* start counter equal to no entries checked */
    counter = Task_Entry->total_entries;

    /* begin the search at the oldest entry - current_idx */
    tempIndex = (INT16)Task_Entry->current_idx;

    while (counter > 0)
    {
        /* we're looking for an empty port entry in the task table */
        if (Task_Entry->socket_index[tempIndex] == NU_IGNORE_VALUE)
            /* we want to return the index of the empty port entry */
            return (tempIndex);

        /* increment index but check for wraparound */
        if ((UINT16)tempIndex == (Task_Entry->total_entries - 1))
            tempIndex = 0;
        else
            tempIndex++;

        /* decrement number of entries left to check */
        counter--;
    }

    /* if we get this far, there was no empty entry in this structure */
    return (NU_IGNORE_VALUE);

} /* TCP_Find_Empty_Port */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Retransmit
*
*   DESCRIPTION
*
*       Send a tcp packet.
*
*   INPUTS
*
*       prt                     Pointer to tcp port information
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID TCP_Retransmit(TCP_PORT *prt)
{
    NET_BUFFER  *buf_ptr;
    TCP_BUFFER  *tcp_buf;
    INT         old_level;
    ROUTE_ENTRY *previous_rt;

#if (INCLUDE_SR_SNMP == NU_TRUE)
    UINT8       tcp_laddr[IP_ADDR_LEN];
    UINT8       tcp_faddr[IP_ADDR_LEN];
#endif

    /* Flag the port to indicate that the retransmission timer has fired.
     * The first RTO value should not longer be used for setting the
     * retransmission timer.
     */
    prt->portFlags |= TCP_RTX_FIRED;

    /* Get a pointer to the oldest packet on the retransmission list. */
    tcp_buf = prt->out.packet_list.tcp_head;

    /* If there is a packet to be retransmitted. */
    if (tcp_buf)
    {
        /* Get a pointer to the buffer. */
        buf_ptr = tcp_buf->tcp_buf_ptr;

        /* We must lock out interrupts to check the flags parameter of the
         * buffer since a network driver can modify this value from a HISR.
         */
        old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Check that the packet is not still on an interface's output queue. */
        if (buf_ptr->mem_flags & NET_TX_QUEUE)
        {
            NU_Local_Control_Interrupts(old_level);

            NLOG_Error_Log("Packet to be retransmitted is still on an interface output queue",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            NU_Local_Control_Interrupts(old_level);

            /* If the sequence number of this packet minus the last sequence number
             * ACKed is less than or equal to the congestion window, retransmit this
             * packet.  Otherwise, the congestion window has been reduced since the
             * last time this packet was transmitted, and there is too much
             * outstanding data in the network to retransmit this packet, so just
             * reset the retransmission timer.  If congestion control is disabled,
             * retransmit the packet.
             */
#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)
            if ( (prt->portFlags & TCP_DIS_CONGESTION) ||
                 ((!(prt->portFlags & TCP_DIS_CONGESTION)) &&
                  (((GET32(buf_ptr->data_ptr, TCP_SEQ_OFFSET) - prt->out.ack) <= prt->p_cwnd) ||
                  (prt->out.tcp_flags & TSYN))) )
#endif
            {
#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)
                if (!(buf_ptr->mem_flags & NET_TCP_PMTU))
#endif
                {
                    /* Increment the number of retransmissions to be sent. */
                    buf_ptr->mem_retransmits ++;

                    /* If the packet is a SYN, bound it by p_max_syn_r2.
                     * Otherwise, bound it by p_max_r2.
                     */
                    if ( ((!(prt->out.tcp_flags & TSYN)) &&
                          (buf_ptr->mem_retransmits > prt->p_max_r2)) ||
                         ((prt->out.tcp_flags & TSYN) &&
                          (buf_ptr->mem_retransmits > prt->p_max_syn_r2)) )
                    {
                        NLOG_Error_Log("Packet has been retransmitted max number of times.  Close connection",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);

                        if ( (prt->state == SCWAIT) || (prt->state == SEST) )
                        {
                            MIB2_tcpEstabResets_Inc;
                            MIB2_tcpCurrEstab_Dec;
#if (INCLUDE_SR_SNMP == NU_TRUE)

                            if (prt->portFlags & TCP_FAMILY_IPV4)
                            {
                                PUT32(tcp_laddr, 0, prt->tcp_laddrv4);
                                PUT32(tcp_faddr, 0, prt->tcp_faddrv4);

                                SNMP_tcpConnTableUpdate(SNMP_DELETE, SEST, tcp_laddr,
                                                        (UNSIGNED)(prt->in.port),
                                                        tcp_faddr,
                                                        (UNSIGNED)(prt->out.port));
                            }
#endif
                        }

                        /* Remove the TCP_BUFFER from the retransmission list */
                        DLL_Remove(&prt->out.packet_list, tcp_buf);

                        /* Release the buffer space held by this packet. This will not
                           be done by TCP_Cleanup below since it has already been
                           pulled off of the port structure. */
                        MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

                        /* Put the TCP_BUFFER back on the list of free buffers */
                        DLL_Enqueue(&TCP_Buffer_List, tcp_buf);

                        /* Send a reset just in case the other side is still up. */
                        prt->out.tcp_flags = TRESET;
                        TCP_ACK_It(prt, 1);

                        /* Set the state of the socket to indicate that the connection
                         * has timed out, and mark the socket as disconnecting.
                         */
                        if ( (prt->p_socketd >= 0) && (SCK_Sockets[prt->p_socketd]) )
                        {
                            SCK_Sockets[prt->p_socketd]->s_state |= SS_TIMEDOUT;

                            SCK_DISCONNECTING(prt->p_socketd);
                        }

                        /* Abort this connection. */
                        prt->state = SCLOSED;

                        /* The connection is  closed.  Cleanup. */
                        TCP_Cleanup(prt);

                        return;
                    }

#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)

                    /* If congestion control is not disabled. */
                    if (!(prt->portFlags & TCP_DIS_CONGESTION))
                    {
                        /* RFC 2581 - section 3.1 - When a TCP sender detects segment
                         * loss using the retransmission timer, the value of ssthresh
                         * MUST be set to no more than MAX(FlightSize / 2, 2 * SMSS)
                         * where FlightSize is the amount of outstanding data in the
                         * network.
                         */
                        prt->p_ssthresh =
                            ((UINT16)(prt->out.contain >> 1) > (prt->p_smss << 1)) ?
                            (prt->out.contain >> 1) : (prt->p_smss << 1);

                        /* RFC 2581 - section 3.1 - Furthermore, upon a timeout cwnd
                         * MUST be set to no more than the loss window, LW, which
                         * equals 1 full-sized segment (regardless of the value of IW).
                         * Therefore, after retransmitting the dropped segment the TCP
                         * sender uses the slow start algorithm to increase the window
                         * from 1 full-sized segment to the new value of ssthresh, at
                         * which point congestion avoidance again takes over.
                         */
                        prt->p_cwnd = prt->p_smss;

#if (INCLUDE_NEWRENO == NU_TRUE)

                        if (!(prt->portFlags & TCP_SACK))
                        {
                            /* Set p_recover to the highest sequence number that has
                             * been transmitted.
                             */
                            prt->p_recover = prt->out.nxt;

                            /* Stop Fast Recovery. */
                            prt->p_dupacks = 0;

                            TEST_TCP_Retrans_Data(buf_ptr->mem_seqnum);
                        }
#endif
                    }

#endif

                    /* RFC 2988 - section 5, point 5.5 - The host MUST set
                     * RTO <- RTO * 2 ("back off the timer")...
                     */
                    prt->p_rto = (prt->p_rto * 2);

                    /* Be sure the RTO is greater than the minimum */
                    if ((UINT32)prt->p_rto < MINRTO)
                        prt->p_rto = MINRTO;

                    /* RFC 2988 - section 5, point 5.5 - ... The maximum value
                     * discussed in (2.5) may be used to provide an upper bound
                     * to this doubling operation.
                     */
                    else if (prt->p_rto > prt->p_max_rto)
                        prt->p_rto = prt->p_max_rto;

                    /* Attempt to find a new route in case this route is no longer
                     * valid.
                     */
                    if (buf_ptr->mem_retransmits == TCP_FIND_NEW_ROUTE)
                    {
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
                        if (prt->portFlags & TCP_FAMILY_IPV6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                        {
                            /* Save a pointer to the current route. */
                            previous_rt = (ROUTE_ENTRY*)prt->tp_routev6.rt_route;

                            /* Free the currently cached route before finding a
                             * new route.
                             */
                            RTAB_Free((ROUTE_ENTRY*)prt->tp_routev6.rt_route,
                                      NU_FAMILY_IP6);

                            prt->tp_routev6.rt_route = NU_NULL;
                            IP6_Find_Route(&prt->tp_routev6);

                            /* If a new route was found, reset the timing
                             * variables.
                             */
                            if ((ROUTE_ENTRY*)prt->tp_routev6.rt_route !=
                                previous_rt)
                            {
                                prt->p_rttvar = 0;
                                prt->p_srtt = 0;
                            }
                        }

#if (INCLUDE_IPV4 == NU_TRUE)
                        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
                        {
                            /* Save a pointer to the current route. */
                            previous_rt = (ROUTE_ENTRY*)prt->tp_route.rt_route;

                            /* Free the currently cached route before finding a
                             * new route.
                             */
                            RTAB_Free((ROUTE_ENTRY*)prt->tp_route.rt_route,
                                      NU_FAMILY_IP);

                            prt->tp_route.rt_route = NU_NULL;
                            IP_Find_Route(&prt->tp_route);

                            /* If a new route was found, reset the timing
                             * variables.
                             */
                            if ((ROUTE_ENTRY*)prt->tp_route.rt_route !=
                                previous_rt)
                            {
                                prt->p_rttvar = 0;
                                prt->p_srtt = 0;
                            }
                        }
#endif
                    }

                    /* RFC 2988 section 5: Note that a TCP implementation MAY
                     * clear SRTT and RTTVAR after backing off the timer multiple
                     * times as it is likely that the current SRTT and RTTVAR are
                     * bogus in this situation.  Once SRTT and RTTVAR are cleared
                     * they should be initialized with the next RTT sample taken
                     * per (2.2) rather than using (2.3).
                     */
                    if (buf_ptr->mem_retransmits == TCP_CLEAR_SRTT_RETRANSMITS)
                    {
                        prt->p_rttvar = 0;
                        prt->p_srtt = 0;
                    }

                    /* If this segment was being timed, stop the timer. */
                    prt->p_rtt = 0;

                    /* Update the clock value for the last time a data packet
                     * was sent
                     */
                    prt->out.lasttime = NU_Retrieve_Clock();
                }

#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)
                /* Otherwise, remove the flag */
                else
                    buf_ptr->mem_flags &= ~NET_TCP_PMTU;
#endif

                /* Send the retransmission. */
                if (TCP_Send_Retransmission(buf_ptr, prt) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to send retransmission", NERR_SEVERE,
                                   __FILE__, __LINE__);
                }

#if (NET_INCLUDE_SACK == NU_TRUE)

                /* Get a pointer to the head of the retransmission
                 * list.
                 */
                tcp_buf = prt->out.packet_list.tcp_head;

                /* While there are packets on the retransmission list. */
                while (tcp_buf)
                {
                    /* Clear the SACK flag. */
                    tcp_buf->tcp_buf_ptr->mem_flags &= ~NET_TCP_SACK;

                    /* Get a pointer to the next packet. */
                    tcp_buf = tcp_buf->tcp_next;
                }

#endif
            }

#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)
            else
                NLOG_Error_Log("Congestion Window too small to retransmit packet",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
#endif
        }

        /* Set a retransmit event for this packet (only one timer per port!). */
        if (TQ_Timerset(TCPRETRANS, (UNSIGNED)prt->pindex,
                        prt->p_rto, 0) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to set timer for RETRANSMIT", NERR_SEVERE,
                           __FILE__, __LINE__);

            NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }
    }

} /* TCP_Retransmit */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Send_Retransmission
*
*   DESCRIPTION
*
*       This function transmits the retransmission message.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the retransmission message
*                               to send.
*       *prt                    A pointer to the port associated with
*                               the connection.
*
*   OUTPUTS
*
*       Status of the transmission call.
*
*************************************************************************/
STATIC STATUS TCP_Send_Retransmission(NET_BUFFER *buf_ptr, TCP_PORT *prt)
{
    TCPLAYER    *tcp_header;
    STATUS      status;

#if (INCLUDE_IPV6 == NU_TRUE)
    IP6_S_OPTIONS   ip6_options;
#endif

#if ((INCLUDE_TCP_INFO_LOGGING == NU_TRUE) || (PRINT_TCP_MSG == NU_TRUE))
    INT16       tcp_len;
#endif

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)
    UINT8       *opt_ptr, *pkt_ptr;
    UINT8       opt_len;
#endif

    /* Get a pointer to the TCP header data */
    tcp_header = (TCPLAYER *)buf_ptr->data_ptr;

    /* If the ACK value or window value has changed since the
     * last time this packet was transmitted, update those
     * fields in the TCP header and recompute the checksum.
     */
    if ( (GET32(tcp_header, TCP_ACK_OFFSET) != prt->in.nxt) ||
         ((GET16(tcp_header, TCP_WINDOW_OFFSET) << prt->in.p_win_shift !=
           prt->credit) &&
          (GET16(tcp_header, TCP_WINDOW_OFFSET) << prt->in.p_win_shift !=
           prt->in.size))
#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)
          || (prt->portFlags & TCP_REPORT_TIMESTAMP)
#endif
       )
    {
        /* Ensure the ACK value is up to date.  We could have
         * received an ACK for a previous packet and need to
         * update this info.
         */
        PUT32(tcp_header, TCP_ACK_OFFSET, prt->in.nxt);

        /* Ensure the Window size is up to date.  It could have
         * changed since this packet was last transmitted.
         */
        if ((UINT32)(prt->credit) < (prt->in.size))
        {
            PUT16(tcp_header, TCP_WINDOW_OFFSET,
                  (UINT16)(prt->credit >> prt->in.p_win_shift));
        }

        else
        {
            PUT16(tcp_header, TCP_WINDOW_OFFSET,
                  (UINT16)(prt->in.size >> prt->in.p_win_shift));
        }

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)
        if (prt->portFlags & TCP_REPORT_TIMESTAMP)
        {
            /* Extract the length of the TCP options. */
            opt_len = (GET8(tcp_header, TCP_HLEN_OFFSET) >> 2) - TCP_HEADER_LEN;

            /* If there are options in the packet. */
            if (opt_len)
            {
                /* Get a pointer to the head of the options. */
                pkt_ptr = (UINT8*)tcp_header + TCP_HEADER_LEN;

                /* Find the timestamp option in the packet. */
                opt_ptr = TCP_Find_Option(pkt_ptr, TCP_TIMESTAMP_OPT, opt_len);

                /* If the timestamp option was found in the packet. */
                if (opt_ptr)
                {
                    /* Update TSval to be the current clock value so this packet
                     * is properly timed.
                     */
                    PUT32(opt_ptr, TCP_TIMESTAMP_TSVAL_OFFSET, NU_Retrieve_Clock());
                }
            }
        }
#endif

        /* Set the checksum field to 0 while computing the
         * checksum
         */
        PUT16(tcp_header, TCP_CHECK_OFFSET, 0);

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
        if (prt->portFlags & TCP_FAMILY_IPV6)
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        {
#if (HARDWARE_OFFLOAD == NU_TRUE)
            /* Do not perform checksum in software if hardware
             * controller can do it
             */
            if ((prt->tp_routev6.rt_route)&&(!(prt->tp_routev6.rt_route->rt_entry_parms.rt_parm_device->
                dev_hw_options_enabled & HW_TX_TCP6_CHKSUM)))
#endif
            {
                /* Compute the checksum */
                PUT16(tcp_header, TCP_CHECK_OFFSET,
                      UTL6_Checksum(buf_ptr, prt->tcp_laddrv6,
                                    prt->tcp_faddrv6,
                                    buf_ptr->mem_total_data_len,
                                    IP_TCP_PROT, IP_TCP_PROT) );
            }
        }

#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        {
#if (HARDWARE_OFFLOAD == NU_TRUE)
            /* Do not perform checksum in software if hardware
                * controller can do it
                */
            if ((prt->tp_route.rt_route)&&(!(prt->tp_route.rt_route->rt_entry_parms.rt_parm_device->
                dev_hw_options_enabled & HW_TX_TCP_CHKSUM)))
#endif
            {
                /* Compute the checksum */
                PUT16(tcp_header, TCP_CHECK_OFFSET,
                      UTL_Checksum(buf_ptr, prt->tcp_laddrv4,
                                   prt->tcp_faddrv4, IP_TCP_PROT));
            }
        }
#endif
    }

#if ((INCLUDE_TCP_INFO_LOGGING == NU_TRUE) || (PRINT_TCP_MSG == NU_TRUE))
    /* Get a pointer to the TCP header data */
    tcp_header = (TCPLAYER *)buf_ptr->data_ptr;

    /* Get the TCP data length */
    tcp_len = (INT16)buf_ptr->mem_total_data_len;

    /* Print the TCP header info */
    NLOG_TCP_Info(tcp_header, tcp_len, NLOG_RETX_PACK);
#endif

    /* Set the flag indicating this packet is being transmitted */
    buf_ptr->mem_flags |= NET_TX_QUEUE;

/* Retransmit this packet. */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (prt->portFlags & TCP_FAMILY_IPV6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    {
        memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));

        status = IP6_Setup_Options(NU_NULL, prt->p_sticky_options,
                                   &ip6_options, prt->tcp_faddrv6,
                                   &prt->tp_routev6, (UINT8)prt->p_ttl);

        if (status != NU_SUCCESS)
            return ((INT16)status);

        ip6_options.tx_source_address = prt->tcp_laddrv6;
        ip6_options.tx_dest_address = prt->tcp_faddrv6;

        status = IP6_Send(buf_ptr, &ip6_options, IP_TCP_PROT,
                          &ip6_options.tx_route, 0, NU_NULL);
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

        status = IP_Send((NET_BUFFER *)buf_ptr, &prt->tp_route,
                         prt->tcp_faddrv4, prt->tcp_laddrv4, 0,
                         prt->p_ttl, IP_TCP_PROT, prt->p_tos, NU_NULL);
#endif

    if (status != NU_SUCCESS)
    {
        buf_ptr->mem_flags &= ~NET_TX_QUEUE;

        /* Packet was not sent,
         * Place the packet back on the list so we can try again later
         */
        NLOG_Error_Log("Failed to retransmit packet", NERR_SEVERE,
                        __FILE__, __LINE__);

        NET_DBG_Notify(status, __FILE__, __LINE__,
                        NU_Current_Task_Pointer(), NU_NULL);
    }

    /* Increment the number of retransmitted segments. */
    MIB2_tcpRetransSegs_Inc;

    return (status);

} /* TCP_Send_Retransmission */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Schedule_ACK
*
*   DESCRIPTION
*
*       This function is responsible for determining whether a SACK
*       or D-SACK option should be included in an ACK, and then calling
*       the appropriate routine to transmit or schedule the ACK.
*
*   INPUTS
*
*       *prt                    Pointer to the tcp port information.
*       *pkt                    Pointer to the TCP header of the packet
*                               causing the ACK.
*       dlen                    The length of the data being ACKed.
*       force                   Wish to ack happen immediately.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID TCP_Schedule_ACK(TCP_PORT *prt, TCPLAYER *pkt, UINT16 dlen,
                             INT force)
{
#if (NET_INCLUDE_SACK == NU_TRUE)

    UINT8       partial_ack = NU_FALSE;
    UINT32      seq_num;

    /* Extract the sequence number from the packet. */
    seq_num = GET32(pkt, TCP_SEQ_OFFSET);

    /* Check if this is a partial ACK. */
    if ( (INT32_CMP(seq_num, prt->in.nxt) > 0) &&
         (INT32_CMP((seq_num + dlen), (prt->in.nxt + prt->in.size)) <= 0) )
    {
        partial_ack = NU_TRUE;
    }

    /* If this ACK does not acknowledge the highest sequence number
     * in the data receiver's queue.
     */
    if ( (dlen) && (prt->in.ooo_list.head || partial_ack) )
    {
        /* If a D-SACK is not being sent and SACK is enabled on the socket. */
        if ( (prt->portFlags & TCP_SACK) &&
             (!(prt->portFlags & TCP_REPORT_DSACK)) )
        {
            /* Set the flag to include a SACK in the ACK. */
            prt->portFlags |= TCP_REPORT_SACK;

            /* If this data should be included in the SACK, this packet will
             * not be in the receive list.
             */
            if (partial_ack)
            {
                /* Set the left and right edge of this packet so it
                 * can be included in the SACK option.
                 */
                prt->left_edge = seq_num;
                prt->right_edge = seq_num + dlen;
            }

            /* Otherwise, set the left edge to the sequence number of the
             * first packet on the Out of Order List.
             */
            else
            {
                /* Set the left and right edge of this packet so it
                 * can be included in the SACK option.
                 */
                prt->left_edge = prt->in.ooo_list.head->mem_seqnum;

                prt->right_edge =
                    prt->in.ooo_list.head->mem_seqnum +
                    prt->in.ooo_list.head->mem_tcp_data_len;
            }

            /* Force this ACK out immediately. */
            force = 1;
        }
    }

#endif

    /* Schedule the ACK. */
    TCP_ACK_It(prt, force);

} /* TCP_Schedule_ACK */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_ACK_It
*
*   DESCRIPTION
*
*       This function is responsible for deciding when an ACK should be
*       sent.An ack should be delayed to 1) make it possible to ack
*       several packets at once  2) If a response is sent immediately by
*       the receiver allow an ack to be sent with the response.  However,
*       when data is being received very rapidly the ack can't be delayed
*       for too long.  This is the reason for checking when the last ack
*       was sent, and sending an ack immediately if the specified
*       threshold is reached.
*
*   INPUTS
*
*       *prt                    Pointer to the tcp port information
*       force                   Wish to ack happen immediately
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID TCP_ACK_It(TCP_PORT *prt, INT force)
{
    /* If the caller wishes to force an immediate ACK or there has been enough
       data received (2 * TCP max segment size), then send an ack. */
    if (force ||
       ( (INT)(prt->in.nxt - prt->in.ack) >=  (INT)(TCP_DELAY_ACK_THRESH * prt->p_smss) ) )
    {
        /* Check to see if there is a timer event to transmit an ack.  If
           so clear the timer, it is no longer needed.  */
        if (prt->portFlags & ACK_TIMER_SET)
        {
            /*  Delete the ACK timeout timer.  */
            TQ_Timerunset(TCPACK, TQ_CLEAR_EXACT, (UNSIGNED)prt->pindex, 0);

            /* Clear the ACK timer flag in the port. */
            prt->portFlags &= (~ACK_TIMER_SET);
        }

        /*  Make sure that the ack goes out.  */
        TCP_Send_ACK(prt);
    }
    /* If a timer event to send an ack has not been created then create one. */
    else if (!(prt->portFlags & ACK_TIMER_SET))
    {
        /* Set up the ACK timer. */
        if (TQ_Timerset(TCPACK, (UNSIGNED)prt->pindex,
                        prt->p_delay_ack, 0) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to set timer for ACK", NERR_SEVERE,
                           __FILE__, __LINE__);

            NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }

        /* Set the flag to indicate a timer has been set. */
        prt->portFlags |= ACK_TIMER_SET;
    }

} /* TCP_ACK_It */

#if (INCLUDE_TCP_OOO == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       TCP_OOO_Packet
*
*   DESCRIPTION
*
*       This function is responsible for placing out of order TCP packets
*       into their proper place in the out of order list.  This is called
*       whenever a packet that is not expected but is within the window
*       is received.  The list of out of order list will later be used
*       when the expected arrives.  All data (the expected packet and
*       those retrieved from this list) will then be acknowledged at one
*       time.
*
*   INPUTS
*
*       *prt                    Pointer to a port.
*       seq                     Sequence number of the packet.  Used to
*                               place the packet in its proper place in
*                               the list.
*
*   OUTPUTS
*
*       INT16                   NU_SUCCESS
*                               -1
*
*************************************************************************/
STATIC INT16 TCP_OOO_Packet(TCP_PORT *prt, UINT32 seq)
{
    TCP_WINDOW  *wind;
    NET_BUFFER  *buf_ptr;
    NET_BUFFER  *curr_buf, *prev_buf = NU_NULL;

    wind = &prt->in;

    /* First check to see if there are currently any packets in the out of order
       list.  If not then add this one and return.  Else search for the position
       at which this buffer should be inserted. */
    if (wind->ooo_list.head == NU_NULL)
    {
        /* Move the buffer from the buffer_list to the out of order list. */
        buf_ptr = MEM_Update_Buffer_Lists(&MEM_Buffer_List, &wind->ooo_list);
    }
    else
    {
        curr_buf = wind->ooo_list.head;

        /* Search for the first packet that has a sequence number that is not
           less than the sequence number of the packet that is being inserted.
         */
        while ( (curr_buf != NU_NULL) &&
                (INT32_CMP(seq, curr_buf->mem_seqnum) > 0) )
        {
            prev_buf = curr_buf;
            curr_buf = (NET_BUFFER *)curr_buf->next;
        }

        /* If a packet with a larger sequence number could not be found then add
           the new packet to the end of the list. */
        if (curr_buf == NU_NULL)
        {
            /* Move the buffer from the buffer_list to the out of order list. */
            buf_ptr = MEM_Update_Buffer_Lists(&MEM_Buffer_List, &wind->ooo_list);
        }

        /* If a packet with this sequence number is already in the list, then
           return a failure status. */
        else if (INT32_CMP(seq, curr_buf->mem_seqnum) == 0)
        {
            return (-1);
        }
        else
        {
            /* Insert this buffer into the list. */
            buf_ptr = (NET_BUFFER *)MEM_Buffer_Dequeue(&MEM_Buffer_List);

            /* Ensure the buffer was put onto the list. */
            if (buf_ptr)
            {
                if (MEM_Buffer_Insert(&wind->ooo_list, buf_ptr, curr_buf,
                                      prev_buf) == NU_NULL)
                    NLOG_Error_Log("Failed to insert buffer into OOO list",
                                   NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* Initialize the sequence field in the buffer. */
    if (buf_ptr != NU_NULL)
        buf_ptr->mem_seqnum = seq;

#if (INCLUDE_IPV6 == NU_TRUE)

    /* RFC 4861 section 7.3.1 - ... the arrival of new (non-duplicate)
     * data indicates that earlier acknowledgements are being delivered
     * to the remote peer.
     */
    if (prt->portFlags & TCP_FAMILY_IPV6)
        NUD6_Confirm_Reachability_By_IP_Addr(prt->tcp_faddrv6);

#endif

    return (NU_SUCCESS);

} /* TCP_OOO_Packet */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Check_OOO_List
*
*   DESCRIPTION
*
*       Checks the out of order list for the packet that is expected next
*       If the next packet is found it is placed in the packet list and
*       the next ack sent will acknowledge this packet.
*
*   INPUTS
*
*       *prt                    Pointer to a port.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID TCP_Check_OOO_List(TCP_PORT *prt)
{
    INT32       trim_amount;
    UINT32      nxt;
    NET_BUFFER  *curr_buf;
    NET_BUFFER  *buf_ptr;

    struct sock_struct *sock_ptr = SCK_Sockets[prt->p_socketd];

    nxt = prt->in.nxt;
    curr_buf = prt->in.ooo_list.head;

    /* First check to see if there are any packets on the ooo (out of order)
       list that are older than the one we expect to receive next.  If there are
       any such packets then deallocate them. */
    while ( (curr_buf != NU_NULL) &&
            (INT32_CMP(nxt, (curr_buf->mem_seqnum + curr_buf->mem_tcp_data_len)) >= 0) )
    {
        /* Free all buffers in the chain. */
        MEM_Buffer_Chain_Free(&prt->in.ooo_list, &MEM_Buffer_Freelist);
        curr_buf = prt->in.ooo_list.head;
    }

    /* Make sure we are still pointing at the head. */
    curr_buf = prt->in.ooo_list.head;

    /* Now check to see if data on ooo_list needs to be shifted */
    if ( (curr_buf != NU_NULL) &&
         (INT32_CMP(nxt, curr_buf->mem_seqnum) > 0) )
    {
        trim_amount = (INT32)(nxt - curr_buf->mem_seqnum);

        MEM_Trim(curr_buf, trim_amount);

        /* MEM_Trim does not modify mem_tcp_data_len or mem_seqnum.
           We need to do it here. */
        curr_buf->mem_tcp_data_len = (UINT16)(curr_buf->mem_tcp_data_len - trim_amount);
        curr_buf->mem_seqnum += (UINT32)trim_amount;
    }

    /* As long as the packet that is expected next is on the ooo list get it and
       place it on the packet list. */
    while ( (curr_buf != NU_NULL) &&
            (INT32_CMP(nxt, curr_buf->mem_seqnum) == 0) )
    {
        /* Place the packet onto the packet list. */
        buf_ptr = MEM_Update_Buffer_Lists(&prt->in.ooo_list, &sock_ptr->s_recvlist);

        /* Ensure the buffer was moved from the out of order list to
         * the receive list.  If not, exit the loop because something has
         * gone wrong.
         */
        if (!buf_ptr)
            break;

        /* Update the expected sequence number. */
        prt->in.nxt += buf_ptr->mem_tcp_data_len;

        /* Decrease the amount of space in the receive window. */
        prt->in.size -= buf_ptr->mem_tcp_data_len;

        /* Increase the number of bytes that are buffered in the window. */
        sock_ptr->s_recvbytes += buf_ptr->mem_tcp_data_len;
        sock_ptr->s_recvpackets++;

        nxt = prt->in.nxt;

        /* Set the current pointer equal to the new head of the OOO list.  The
           old head was removed by MEM_Update_Buffer_Lists() above.  Doing the
           following will not work: "curr_buf = curr_buf->next;".  This is because
           the call to MEM_Update_Buffer_Lists() above places the current buffer
           at the tail of the in window's packet list and in the process sets the
           next pointer to NULL.  So curr_buf would then be set to NULL
        */
        curr_buf = prt->in.ooo_list.head;
    }

} /* TCP_Check_OOO_List */

#endif

/************************************************************************
*
*   FUNCTION
*
*       TCP_Xmit
*
*   DESCRIPTION
*
*       This function is responsible for sending data.
*
*   INPUTS
*
*       *prt                    Pointer to a port.
*       *buf_ptr                Pointer to a packet buffer.
*
*   OUTPUTS
*
*       The number of bytes sent.
*       NU_HOST_UNREACHABLE
*       NU_ACCESS
*       NU_MSGSIZE
*       NU_SUCCESS
*
*************************************************************************/
STATUS TCP_Xmit(TCP_PORT *prt, NET_BUFFER *buf_ptr)
{
    STATUS          status;

    /* Store the sequence number of this packet. */
    buf_ptr->mem_seqnum = prt->out.nxt;

    /* Send the packet. */
    status = TCP_Send(prt, buf_ptr);

    /* Clear the PUSH bit. It may or may not have been sent. */
    prt->out.tcp_flags &= ~TPUSH;

    return (status);

} /* TCP_Xmit */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Cleanup
*
*   DESCRIPTION
*
*       This function cancels pending events for a connection when it
*       closes.  It also clears out the various packet lists associated
*       with a particular connection.
*
*   INPUTS
*
*       *prt                    Pointer to the port structure which
*                               describes the closing connection.
*
*   OUTPUTS
*
*       NU_SUCCESS always.
*
*************************************************************************/
STATUS TCP_Cleanup(TCP_PORT *prt)
{
    struct sock_struct  *sockptr;
    TCP_BUFFER          *tcp_buf;
    INT                 old_level;

#if ( (INCLUDE_STATIC_BUILD == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
    INT                 j;
#endif

    /* Clear the retransmit timer for this port. */
    TQ_Timerunset(TCPRETRANS, TQ_CLEAR_ALL_EXTRA, (UNSIGNED)prt->pindex, 0);

#if (INCLUDE_TCP_KEEPALIVE == NU_TRUE)
    /* Clear the Keep Alive timer if set*/
    if(prt->portFlags & TCP_KEEPALIVE)
    {
        TQ_Timerunset(TCP_Keepalive_Event, TQ_CLEAR_ALL_EXTRA,
                                                (UNSIGNED)prt->pindex, 0);
    }
#endif /* INCLUDE_TCP_KEEPALIVE == NU_TRUE */

    /* Clear all lists of packet buffers. */
    MEM_Buffer_Cleanup(&prt->in.ooo_list);

    /* Clean up any buffers remaining on the retransmission list */
    tcp_buf = DLL_Dequeue(&prt->out.packet_list);

    while (tcp_buf)
    {
        /*  Temporarily lockout interrupts to protect the global buffer
         *  variables.
         */
        old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* If this buffer is currently on the output queue of an interface,
         * set the dlist to the free list and clear the NET_TX_QUEUE flag.
         * When the interface is finished transmitting, the buffer will be
         * returned to the free list.
         */
        if (tcp_buf->tcp_buf_ptr->mem_flags & NET_TX_QUEUE)
        {
            tcp_buf->tcp_buf_ptr->mem_dlist = &MEM_Buffer_Freelist;
            tcp_buf->tcp_buf_ptr->mem_flags &= ~NET_TX_QUEUE;

            /*  Restore the previous interrupt lockout level.  */
            NU_Local_Control_Interrupts(old_level);
        }

        /* Otherwise, return the buffer to the Free List */
        else
        {
            /*  Restore the previous interrupt lockout level.  */
            NU_Local_Control_Interrupts(old_level);

            MEM_One_Buffer_Chain_Free(tcp_buf->tcp_buf_ptr,
                                      &MEM_Buffer_Freelist);
        }

        /* Put the TCP Buffer element back on the list of free elements */
        DLL_Enqueue(&TCP_Buffer_List, tcp_buf);

        /* Get the next TCP buffer element */
        tcp_buf = DLL_Dequeue(&prt->out.packet_list);
    }

    MEM_One_Buffer_Chain_Free(prt->out.nextPacket, &MEM_Buffer_Freelist);

    /* Clear the nextPacket field. */
    prt->out.nextPacket = NU_NULL;

    /* Is there an timer event to send an ACK? */
    if (prt->portFlags & ACK_TIMER_SET)
    {
        /*  Delete the ACK timeout timer.  */
        TQ_Timerunset(TCPACK, TQ_CLEAR_EXACT, (UNSIGNED)prt->pindex, 0);
    }

    /* Stop the Window Probe timer. */
    if (prt->probeFlag == NU_SET)
    {
        /* Stop the timer */
        TQ_Timerunset(WINPROBE, TQ_CLEAR_EXACT, prt->pindex, 0);

        prt->probeFlag = NU_CLEAR;
    }

    /* Is there an event set to transmit a partial packet. If so clear that
       event. */
    if (prt->xmitFlag == NU_SET)
    {
        TQ_Timerunset(CONTX, TQ_CLEAR_EXACT, (UNSIGNED)prt->pindex, 0);
        prt->xmitFlag = NU_CLEAR;
    }

#if ( (INCLUDE_STATIC_BUILD == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
    /* Traverse the flag array to find the used memory location*/
    for (j = 0; j != NSOCKETS; j++)
    {
        /* If this is the memory area being released */
        if (&NET_Sticky_Options_Memory[j] == prt->p_sticky_options)
        {
            /* Turn the memory flag off */
            NET_Sticky_Options_Memory_Flags[j] = NU_FALSE;
        }
    }
#endif

    /* Get a pointer to the socket. */
    if (prt->p_socketd != -1)
    {
        sockptr = SCK_Sockets[prt->p_socketd];

        if (sockptr)
        {
            /* Restart any tasks pending on the receive */
            SCK_Resume_All(&sockptr->s_RXTask_List, 0);

            /* Restart any tasks pending on the TX list, remove any
               pending buffer suspension entry */
            SCK_Resume_All(&sockptr->s_TXTask_List, SCK_RES_BUFF);

            /* restart the waiting task */
            if (sockptr->s_CLSTask != NU_NULL)
            {
                if (NU_Resume_Task(sockptr->s_CLSTask) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to resume task", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }

            /* Invalidate the port index. */
            SCK_Sockets[prt->p_socketd]->s_port_index = -1;

            /* If this connection was in the process of being established
               clear out the accept entry. */
            if (sockptr->s_accept_list)
            {
                /* Resume any tasks suspended on listener sockets */
                SCK_Resume_All(&(sockptr->s_accept_list->ssp_task_list), 0);

                /* Clear the entry that was being used to accept this connection. */
                SCK_Clear_Accept_Entry(sockptr->s_accept_list,
                                       sockptr->s_accept_index);

                /* If we get here it means that a socket was closed before it was
                   ever accepted by an application.  That means we have to clear
                   the socket as well.
                */
                /* Clear out the receive list. */
                MEM_Buffer_Cleanup(&sockptr->s_recvlist);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

                /* Clean up any task table entries. */
                SCK_TaskTable_Entry_Delete(prt->p_socketd);

                /* release the memory used by this socket */
                if (NU_Deallocate_Memory((VOID *)sockptr) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to deallocate memory for socket",
                                   NERR_SEVERE, __FILE__, __LINE__);
#endif

                /* clear this socket pointer for future use */
                SCK_Sockets[prt->p_socketd] = NU_NULL;
            }
        }

        /* Invalidate the socket index. */
        prt->p_socketd = -1;
    }

    /* Free the route that was being used by this connection. */
#if (INCLUDE_IPV6 == NU_TRUE)

    if (prt->portFlags & TCP_FAMILY_IPV6)
        RTAB_Free((ROUTE_ENTRY*)prt->tp_routev6.rt_route, NU_FAMILY_IP6);

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        RTAB_Free((ROUTE_ENTRY*)prt->tp_route.rt_route, NU_FAMILY_IP);
    }
#endif

    UTL_Zero(&prt->tcp_foreign_addr, sizeof(prt->tcp_foreign_addr));

    /* Clear out the in and out ports and foreign address. */
    prt->in.port = prt->out.port = 0;

#if (INCLUDE_IPSEC == NU_TRUE)

    /* Clear out the IPsec structures. */
    prt->tp_ips_group       = NU_NULL;
    prt->tp_ips_in_policy   = NU_NULL;
    prt->tp_ips_out_policy  = NU_NULL;
    prt->tp_ips_out_bundle  = NU_NULL;

#endif

    return (NU_SUCCESS);

} /* TCP_Cleanup */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Xmit_Timer
*
*   DESCRIPTION
*
*       Compute the new RTO value to be used for a connection.
*       This is done by following RFC 2988 and using the algorithms
*       defined within the paper authored by Jacobson and Karels,
*       "Congestion Avoidance and Control."
*
*   INPUTS
*
*       *prt                    Pointer to a TCP port
*       rtt                     The round trip time for a segment.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC void TCP_Xmit_Timer(TCP_PORT *prt, UINT32 rtt)
{
    INT32   delta;

    if (prt->p_srtt != 0)
    {
        /* Delta is the difference between the average round trip and this
         * one.
         */
        delta = (INT32)(rtt - (prt->p_srtt >> TCP_RTT_SHIFT));

        /* Accumulate smoothed average, gain is 2^(-TCP_RTT_SHIFT). */
        prt->p_srtt += delta;

        /* Get absolute value of difference for variance estimate. */
        if (delta < 0)
            delta = -delta;

        /* Calculate difference from smoothed variance. */
        delta -= (prt->p_rttvar >> TCP_RTTVAR_SHIFT);

        /* Accumulate smoothed variance, gain is 2^(-TCP_RTTVAR_SHIFT). */
        prt->p_rttvar += delta;
    }
    else
    {
        /* This is the first measurement.  Use RTT to initialize the
         * scaled smoothed values.
         */
        prt->p_srtt = rtt << TCP_RTT_SHIFT;
        prt->p_rttvar = rtt << (TCP_RTTVAR_SHIFT - 1);
    }

    /* Set the RTT counter back to 0 so it can be reused to time the
     * next segment.
     */
    prt->p_rtt = 0;

    /* Calculate the new RTO value.
     * RTO  = SRTT + max(G, K * RTTVAR) per RFC2988, where K = 4 and
     * G = 1 tick.
     */
    prt->p_rto = (prt->p_srtt >> TCP_RTT_SHIFT) + (prt->p_rttvar ?
        prt->p_rttvar : 1);

    /* Check that the new RTO is between the minimum and maximum allowed
     * values.
     */
    if (prt->p_rto < MINRTO)
        prt->p_rto = MINRTO;

    else if (prt->p_rto > prt->p_max_rto)
        prt->p_rto = prt->p_max_rto;

} /* TCP_Xmit_Timer */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Make_Port
*
*   DESCRIPTION
*
*       This is the initialization for TCP based communication.  When a
*       port needs to be created, this routine is called to do as much
*       pre-initialization as possible to save overhead during operation.
*
*       This structure is created upon open of a port, either listening
*       or wanting to send.
*
*       A TCP port, in this implementation, includes all of the state
*       data for the port connection, a complete packet for the TCP
*       transmission, and two queues, one each for sending and receiving.
*       The data associated with the queues is in struct window.
*
*   INPUTS
*
*       family                  The family of the connection.
*       port_num                The port number.
*
*   OUTPUTS
*
*       INT16
*
*************************************************************************/
INT TCP_Make_Port(INT16 family, UINT16 port_num)
{
    UINT16      i;
    TCP_PORT    *prt;
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    STATUS      status;
#else
#if (INCLUDE_IPV6 == NU_TRUE)
    STATUS      ret_status;
    INT         j;
#endif
#endif

    /* Check to see if any other connection is done with its port buffer
     * space.  Indicated by the connection state of SCLOSED.
     */

    prt = NU_NULL;
    i = 0;

    /* Search for a pre-initialized port to re-use */
    do
    {
        /* If the port structure already exists and the state is SCLOSED,
         * reuse this port structure.
         */
        if ( (TCP_Ports[i]) && (TCP_Ports[i]->state == SCLOSED) )
        {
            prt = TCP_Ports[i];
            break;
        }

        i++;                   /* port # to return */

    } while (i < TCP_MAX_PORTS);

    /* None available pre-allocated, get a new one, about 8.5 K with a 4K
     * windowsize
     */
    if (prt == NU_NULL)
    {
        for (i = 0; (!(i>= TCP_MAX_PORTS) && (TCP_Ports[i] != NU_NULL)); i++)
            ;

        if (i >= TCP_MAX_PORTS)
        {
            return (-1);               /* out of room for ports */
        } /* end if */

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        /* added during ATI mods - 10/1/92, bgh */
        status = NU_Allocate_Memory(MEM_Cached, (VOID **) &prt,
                                    (UNSIGNED)sizeof(TCP_PORT),
                                    (UNSIGNED)NU_NO_SUSPEND);

        /* check status of memory allocation */
        if (status != NU_SUCCESS)
        {
            /* ERROR memory allocation error.\r\n */
            NLOG_Error_Log("Unable to allocate memory for TCP port struct",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
            return (-1);               /* out of room for ports */
        } /* end if */

        else
#else
        /* Assign memory to the new port */
        prt = &NET_TCP_Ports_Memory[i];

#if (INCLUDE_IPV6 == NU_TRUE)
        /* Traverse the flag array to find the unused memory location*/
        for (j = 0; j != NSOCKETS; j++)
        {
            /* If this memory is available, use it */
            if (NET_Sticky_Options_Memory_Flags[j] != NU_TRUE)
            {
                /* Assign sticky options pointer to this memory */
                prt->p_sticky_options =
                    (tx_ancillary_data *)&NET_Sticky_Options_Memory[j];

                /* Turn the memory flag on */
                NET_Sticky_Options_Memory_Flags[j] = NU_TRUE;
                ret_status = NU_SUCCESS;

                break;
            }
        }

        /* If there are no available entries, set an error */
        if (j == NSOCKETS)
            return (-1);
#endif
#endif

            prt = (TCP_PORT *)TLS_Normalize_Ptr(prt);

        TCP_Ports[i] = prt;

    } /* end if */

    /* Zero out the port structure. */
    UTL_Zero((CHAR *)prt, sizeof(*prt));

    /* Set the index of this port entry. */
    prt->pindex = i;

    /* Build a local port number.  Needs to be unique and greater than
       2048.  */
    if (port_num == 0)
        i = PRT_Get_Unique_Port_Number((UINT16)NU_PROTO_TCP, family);

    else
        i= port_num;

    prt->in.port = i;                         /* save for incoming comparison */
    prt->state = SREADY;                     /* connection not established yet */
    prt->credit = WINDOW_SIZE;                /* presaved credit */

    /* Set the first retransmit timeout value. */
    prt->p_rto = prt->p_first_rto = (INT32)TCP_RTTDFLT;

    /* Initialize the in window and out window fields. They have already been
       cleared to zeros above. So only those that are initialized to values
       other than 0 must be touched. */
    prt->in.lasttime = prt->out.lasttime = NU_Retrieve_Clock();
    prt->in.size = prt->out.size = WINDOW_SIZE;

    /* Get an initial sequence number. */
    prt->out.ack = prt->out.nxt = UTL_Rand();

#if (INCLUDE_NEWRENO == NU_TRUE)
    if (!(prt->portFlags & TCP_SACK))
    {
        prt->p_recover = prt->out.ack;
    }
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    if (family == SK_FAM_IP6)
        prt->portFlags |= TCP_FAMILY_IPV6;

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        prt->portFlags |= TCP_FAMILY_IPV4;
#endif

    /* Initialize the MSL. */
    prt->p_msl = WAITTIME;

    /* Initialize the ACK delay value. */
    prt->p_delay_ack = TCP_ACK_TIMEOUT;

    /* Initialize the amount of time to remain idle on a connection before
     * invoking the keep-alive mechanism.
     */
    prt->p_ka_wait = TCP_KEEPALIVE_DELAY;

    /* Initialize the maximum number of times to retransmit a keep-alive. */
    prt->p_ka_r2 = TCP_MAX_KEEPALIVES;

    /* Initialize the keep alive probe time interval */
    prt->p_keepalive_intvl = TCP_KEEPALIVE_INTERVAL;

    /* Initialize the amount of time to delay before transmitting the first
     * Zero Window probe.
     */
    prt->p_first_probe_to = PROBETIMEOUT;

    /* Initialize the maximum timeout value for successive Zero Window
     * probes.
     */
    prt->p_max_probe_to = MAX_PROBETIMEOUT;

    /* Initialize the maximum number of Zero Window probes to transmit. */
    prt->p_max_probes = TCP_MAX_PROBE_COUNT;

    /* Initialize the maximum timeout value for successive
     * retransmissions.
     */
    prt->p_max_rto = MAXRTO;

    /* Initialize the maximum number of times to retransmit a data
     * packet.
     */
    prt->p_max_r2 = MAX_RETRANSMITS;

    /* Initialize the maximum number of times to retransmit a SYN. */
    prt->p_max_syn_r2 = MAX_RETRANSMITS;

#if (NET_INCLUDE_SACK == NU_TRUE)

    /* SACKs are enabled by default when support is compiled into the
     * system.
     */
    prt->portFlags |= TCP_SACK;

#if (NET_INCLUDE_DSACK == NU_TRUE)

    /* D-SACKs are enabled by default when support is compiled into the
     * system.
     */
    prt->portFlags |= TCP_DSACK;

#endif
#endif

#if (NET_INCLUDE_WINDOWSCALE == NU_TRUE)

    /* The Window Scale option is enabled by default when support is
     * compiled into the system.
     */
    prt->portFlags |= TCP_REPORT_WINDOWSCALE;

    /* Compute the shift count for the default window size. */
    prt->in.p_win_shift = TCP_Configure_Shift_Count(prt->in.size);

#endif

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

    /* The Timestamp option is enabled by default when support is
     * compiled into the system.
     */
    prt->portFlags |= TCP_REPORT_TIMESTAMP;

#endif

    return ((INT16)prt->pindex);

} /* TCP_Make_Port */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Handle_Datagram_Error
*
*   DESCRIPTION
*
*       This function finds the appropriate socket for an incoming
*       ICMP error message, sets the error field of the socket and
*       resumes any tasks that are suspended.
*
*   INPUTS
*
*       family                  NU_FAMILY_IP or NU_FAMILY_IP6
*       *buf_ptr                A pointer to the packet.
*       src_addr                The source address of the error packet.
*       dest_addr               The destination address of the error packet.
*       icmp_error              The error received.
*
*   OUPUTS
*
*       NU_SUCCESS
*       -1                      A matching port does not exist or there is
*                               already an error on the socket.
*
*************************************************************************/
STATUS TCP_Handle_Datagram_Error(INT16 family, const NET_BUFFER *buf_ptr,
                                 const UINT8 *src_addr, const UINT8 *dest_addr,
                                 INT32 error)
{
    UINT16              dest_port, src_port;
    INT32               port_index;
    STATUS              status;
    TCP_PORT            *prt;

#if (INCLUDE_IPSEC == NU_TRUE)
    UINT8               prt_found;
#endif

#if (INCLUDE_IPSEC == NU_TRUE)

    /* The ICMP Destination Unreachable message generated for the IPsec
     * packet encapsulates the original IP header with 8 data bytes of the
     * offending datagram. For IPsec these data bytes are part of the AH
     * or ESP header. Therefore the source and destination ports are not
     * available.
     *
     * In order to find the corresponding socket for repacketization,
     * iterate through all TCP ports and repacket the matching ports.
     * Matching criteria is based on source and destination addresses
     */

    status = NU_SUCCESS;

    /* Check whether IPsec is enabled for the device which received this
     * packet.
     */
    if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
    {
        for (port_index = 0; port_index < TCP_MAX_PORTS; port_index++)
        {
            prt_found = NU_FALSE;

            prt = TCP_Ports[port_index];

            if (prt != NU_NULL)
            {
#if (INCLUDE_IPV4 == NU_TRUE)
                /* Check port family */
                if (prt->portFlags & TCP_FAMILY_IPV4)
                {
                    /* Compare ports local and foreign IPv4 addresses */
                    if ( (prt->tcp_laddrv4 == IP_ADDR(src_addr)) &&
                         (prt->tcp_faddrv4 == IP_ADDR(dest_addr)) )
                    {
                        /* Set port found flag */
                        prt_found = NU_TRUE;
                    }
                }
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
                /* Check port family */
                if (prt->portFlags & TCP_FAMILY_IPV6)
                {
                    /* Compare ports local and foreign IPv4 addresses */
                    if ( (memcmp(prt->tcp_laddrv6, src_addr,
                                 IP6_ADDR_LEN) == 0) &&
                         (memcmp(prt->tcp_faddrv6, dest_addr,
                                 IP6_ADDR_LEN) == 0) )
                    {
                        /* Set port found flag */
                        prt_found = NU_TRUE;
                    }
                }
#endif
            }

            /* If this is the port we want, assign an error code */
            if (prt_found == NU_TRUE)
            {
                /* If a target port was found */

                /* If the port is pointing to a valid socket index,
                 * set the error.
                 */
                if (prt->p_socketd >= 0)
                {
#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)

                    /* If this is a Packet Too Big Error, do not notify the
                     * packetization layer.  Retransmit the packets on the
                     * retransmission list that are larger than the new MTU
                     * for the route.
                     */
                    if (error == NU_DEST_UNREACH_FRAG)
                    {
                        TCP_PMTU_Repacket(prt);
                        status = NU_SUCCESS;
                    }

                    else
#endif
                    /* If the error is not intended for the kernel to handle,
                     * pass it to the user process.
                     */
                    if (!ICMP_KERNEL_ERROR(error))
                        status = SCK_Set_Socket_Error(SCK_Sockets[prt->p_socketd],
                                                      error);

                    /* In the future, stack errors will be handled by the stack */
                    else
                        status = NU_SUCCESS;
                }
                else
                    status = -1;
            }
        }
    }

    else
#endif
    {
        /* Get the destination and source port */
        src_port = GET16(buf_ptr->data_ptr, TCP_SRC_OFFSET);
        dest_port = GET16(buf_ptr->data_ptr, TCP_DEST_OFFSET);

        /* Find the port index matching the given parameters */
        port_index = PRT_Find_Matching_Port(family, NU_PROTO_TCP, src_addr,
                                            dest_addr, src_port, dest_port);

        /* If a target port was found */
        if (port_index >= 0)
        {
            prt = TCP_Ports[port_index];

            /* If the port is pointing to a valid socket index, set the
             * error.
             */
            if (prt->p_socketd >= 0)
            {
#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)

                /* If this is a Packet Too Big Error, do not notify the
                 * packetization layer.  Retransmit the packets on the
                 * retransmission list that are larger than the new MTU
                 * for the route.
                 */
                if (error == NU_DEST_UNREACH_FRAG)
                {
                    TCP_PMTU_Repacket(prt);
                    status = NU_SUCCESS;
                }

                else
#endif
                /* If the error is not intended for the kernel to handle,
                 * pass it to the user process.
                 */
                if (!ICMP_KERNEL_ERROR(error))
                    status = SCK_Set_Socket_Error(SCK_Sockets[prt->p_socketd],
                                                  error);

                /* In the future, stack errors will be handled by the stack */
                else
                    status = NU_SUCCESS;
            }
            else
                status = -1;
        }
        else
            status = -1;
    }

    return (status);

} /* TCP_Handle_Datagram_Error */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Enqueue
*
*   DESCRIPTION
*
*       Add a packet to a TCP queue.  Used by both 'write()' and
*       TCP_Interpret().   WINDOWSIZE is the size limitation of the
*       advertised window.
*
*   INPUTS
*
*       *sock_ptr               A pointer to the socket list
*
*   OUTPUTS
*
*       UINT16                  The size of the packet enqueued
*
************************************************************************/
STATIC UINT16 TCP_Enqueue(struct sock_struct *sock_ptr)
{
    NET_BUFFER          *dest = sock_ptr->s_recvlist.tail;
    NET_BUFFER          *src = MEM_Buffer_List.head;
    UINT16              retval;

    /* Save the size of the packet enqueued */
    retval = (UINT16)src->mem_total_data_len;

    /* This first check is an optimization that attempts to merge the data
       received in small packets. It is not intended to concatenate all
       received data, but only relatively small packets that are received
       contiguously. First check to see if a packet was received prior to
       the current one (dest). Check to see if the current packet is a
       relatively small one (total_data_len < 50). Finally check to make sure that
       the current packet will fit within the first buffer of the chain in
       which the previous packet was stored. All of these checks guarantee
       that we will not waste a lot of time moving data around, rather
       packets will be merged only when the benefits are great relative to
       the work that must be done. */
    if ( (dest) && (dest->next_buffer == NU_NULL) && (retval < 50) &&
         ((dest->mem_total_data_len + retval +
          (UINT32)(dest->data_ptr - dest->mem_parent_packet)) <= NET_PARENT_BUFFER_SIZE) )
    {
        /* Copy the data in the current packet to the previous one. */
        MEM_Chain_Copy(dest, src, 0, (INT32)(src->mem_total_data_len));

        /* Update the length. */
        dest->mem_total_data_len += retval;

        /* Free the current packet. */
        MEM_Update_Buffer_Lists(&MEM_Buffer_List, &MEM_Buffer_Freelist);
    }
    else
    {
        /* It was not possible to merge the data. So, remove the whole packet from
           the buffer_list and place it in the socket's receive list. */
        MEM_Update_Buffer_Lists(&MEM_Buffer_List, &sock_ptr->s_recvlist);
        sock_ptr->s_recvpackets++;
    }

     /* Increment the number of bytes this socket contains. */
    sock_ptr->s_recvbytes += retval;
   
    return (retval);

} /* TCP_Enqueue */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Dequeue

*   DESCRIPTION
*
*       Used by netread, this copies data out of the queue and then
*       deallocates it from the queue.
*       rmqueue is very similar and is to be used by tcpsend
*       to store unacknowledged data.
*
*       Returns the number of bytes removed from the queue.
*
*   INPUTS
*
*       *sock_ptr               A pointer to the socket list
*       *buffer                 A pointer to the buffer to dequeue
*       nbytes                  The number of bytes to dequeue
*
*   OUTPUTS
*
*       actual_bytes            The number of bytes removed from the
*                               queue
*       0
*
************************************************************************/
INT32 TCP_Dequeue(struct sock_struct *sock_ptr, char *buffer, UINT32 nbytes)
{
    INT32   bytes_copied = 0, total_bytes = 0;

    /* If there is no data in the window, return. */
    if ( (sock_ptr->s_recvbytes == 0) ||
         (sock_ptr->s_recvlist.head == NU_NULL) )
        return (0);

    /* If there is not as much data in the buffer as the application
     * can handle, return as much data as the buffer contains.
     */
    if ((sock_ptr->s_recvbytes) < nbytes)
        nbytes = sock_ptr->s_recvbytes;

    /* Copy as much data into the user's buffer as will fit. */
    do
    {
        /* Move the data into the caller's buffer. */
        bytes_copied = MEM_Copy_Buffer(&buffer[total_bytes], sock_ptr,
                                       (INT32)nbytes);

        /* If this is not a zero copy buffer. */
        if (!(sock_ptr->s_flags & SF_ZC_MODE))
        {
            /* Ensure that the number of bytes copied is NOT greater than the
             * the size of the user buffer
             */
            if (bytes_copied > nbytes)
            {
                NLOG_Error_Log("More TCP bytes copied than requested",
                               NERR_SEVERE, __FILE__, __LINE__);
                break;
            }

            /* Decrement the amount of memory remaining in the incoming
             * buffer. */
            nbytes -= bytes_copied;

            /* Increment the total number of bytes copied. */
            total_bytes += bytes_copied;

            /* All of the data has been removed from the buffer, free the
             * buffer and decrement the number of packets waiting.
             */
            if (sock_ptr->s_recvlist.head->mem_total_data_len == 0)
            {
                sock_ptr->s_recvpackets--;

                MEM_Buffer_Chain_Free(&sock_ptr->s_recvlist,
                                      &MEM_Buffer_Freelist);
            }
        }

        /* Otherwise, if this is a zero copy buffer. */
        else
        {
            /* Increment the total number of bytes copied. */
            total_bytes += bytes_copied;

            /* Decrement the number of packets on the socket and remove
             * the packet from the socket's receive list.
             */
            sock_ptr->s_recvpackets--;
            MEM_Buffer_Dequeue(&sock_ptr->s_recvlist);

            /* Ensure that the loop runs just once in Zero Copy mode. */
            break;
        }

    } while ( (nbytes) && (sock_ptr->s_recvlist.head) );

    /* Update the number of bytes in the window. */
    sock_ptr->s_recvbytes -= (UINT32)total_bytes;

    return (total_bytes);

} /* TCP_Dequeue */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Rmqueue
*
*   DESCRIPTION
*
*       Does the queue deallocation once an ack is received.
*       rmqueue of WINDOWSIZE or greater bytes will empty the queue
*
*   INPUTS
*
*       *wind                   Pointer to the tcp window in use
*       acked                   If acked or not
*
*   OUTPUTS
*
*       bytes_removed
*
************************************************************************/
STATIC UINT16 TCP_Rmqueue(TCP_WINDOW *wind, UINT32 acked)
{
    UINT16      bytes_removed = 0;
    NET_BUFFER  *buf_ptr;
    TCP_BUFFER  *tcp_buf, *temp_buf;

#ifndef PACKET
    INT         old_level;
#endif

    /* Interrupts do not need to be locked out here when PACKET mode is
     * enabled since the driver does not call MEM_Multiple_Buffer_Chain_Free
     * if PACKET mode is enabled when a packet has finished transmitting,
     * but this routine is called from the context of the stack; therefore,
     * the code is protected by the semaphore and does not need to lock out
     * interrupts.
     */
#ifndef PACKET

    /* Disable interrupts while we access the receive buffer queue. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

#endif

    tcp_buf = wind->packet_list.tcp_head;

    while (tcp_buf)
    {
        buf_ptr = tcp_buf->tcp_buf_ptr;

        /* Save a pointer to the next buffer. */
        temp_buf = tcp_buf->tcp_next;

        /* If the current packet was acknowledged and the current packet is not
         * the next one to be transmitted, then remove it.  The sequence number
         * for the next packet to be transmitted has not yet been filled in and
         * therefore contains an arbitrary number that may be less than the
         * received ack. */
        if ( (INT32_CMP(acked, (buf_ptr->mem_seqnum+buf_ptr->mem_tcp_data_len)) >= 0) &&
             (buf_ptr != wind->nextPacket) )
        {
            /* Update the number of bytes removed so far. */
            bytes_removed = (UINT16)(bytes_removed + buf_ptr->mem_tcp_data_len);

            /* Update the number of bytes contained in this window. */
            wind->contain -= buf_ptr->mem_tcp_data_len;

            /* Update the number of packets contained in this window. */
            wind->num_packets--;

            /* If the packet is not currently being transmitted, free the
             * buffer.
             */
            if (!(buf_ptr->mem_flags & NET_TX_QUEUE))
                MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

            /* Otherwise, set the dlist to the Free List so the buffer is
             * freed upon transmission.
             */
            else
            {
                /* Clear the transmission flag */
                buf_ptr->mem_flags &= ~NET_TX_QUEUE;
                buf_ptr->mem_dlist = &MEM_Buffer_Freelist;
            }

            /* Remove the TCP Buffer from the retransmission list */
            DLL_Remove(&wind->packet_list, tcp_buf);

            /* Put the TCP Buffer back on the list of free elements so
             * it can be reused.
             */
            DLL_Enqueue(&TCP_Buffer_List, tcp_buf);
        }

        /* Look at the next buffer. */
        tcp_buf = temp_buf;
    }

#ifndef PACKET

    /* Restore to previous level. */
    NU_Local_Control_Interrupts (old_level);

#endif

    /* Return the number of bytes removed.  */
    return (bytes_removed);

} /* TCP_Rmqueue */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Xmit_Probe
*
*   DESCRIPTION
*
*       This function transmits a probe packet with 1 byte of garbage
*       and a sequence number of 1 minus the proper sequence number.
*       This routine is used to transmit Keep-Alive and Window Probes.
*
*   INPUTS
*
*       *prt                    A pointer to the port structure to use
*                               in transmitting the packet.
*
*   OUTPUTS
*
*       STATUS
*
************************************************************************/
STATUS TCP_Xmit_Probe(TCP_PORT *prt)
{
    NET_BUFFER  *buf_ptr;
    TCPLAYER    *tcp_ptr;
    STATUS      status;

#if (INCLUDE_IPV6 == NU_TRUE)
    IP6_S_OPTIONS       ip6_options;
#endif

    /* Get a buffer for the transmission */
    buf_ptr = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

    /* If a buffer could not be dequeued, return an error */
    if (buf_ptr == NU_NULL)
        return (NU_NO_BUFFERS);

#if (INCLUDE_IPSEC == NU_TRUE)

    /* Set the TCP port pointer. This will enable IPsec to use cached
     * information in the port structure when adding its IPsec headers.
     */
    buf_ptr->mem_port = prt;

#endif

    /* Set the data pointer to the beginning of the buffer */
    buf_ptr->data_ptr = buf_ptr->mem_parent_packet;

    TCP_Update_Headers(prt, buf_ptr, TCP_HEADER_LEN);

    /* Overlay the TCP header. */
    tcp_ptr = (TCPLAYER *)buf_ptr->data_ptr;

    /* Set the sequence number to the current sequence number minus 1 */
    PUT32(tcp_ptr, TCP_SEQ_OFFSET, prt->out.nxt - 1);

    /* Initialize each field in the allocated buffer. */
    buf_ptr->data_len   = buf_ptr->mem_total_data_len =
        (TCP_HEADER_LEN + prt->p_opt_len) + 1;

    buf_ptr->mem_dlist  = &MEM_Buffer_Freelist;
    buf_ptr->mem_seqnum = prt->out.nxt - 1;

    /* Put the 1 byte of garbage in the packet */
    PUT8(buf_ptr->data_ptr, (TCP_HEADER_LEN + prt->p_opt_len), 0);

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    if (prt->portFlags & TCP_FAMILY_IPV6)
    {
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));

        /* Set up the parameters to be used for the transmission */
        status = IP6_Setup_Options(NU_NULL, prt->p_sticky_options,
                                   &ip6_options, prt->tcp_faddrv6,
                                   &prt->tp_routev6, (UINT8)prt->p_ttl);

        if (status != NU_SUCCESS)
            return (status);

        ip6_options.tx_source_address = prt->tcp_laddrv6;

#if (HARDWARE_OFFLOAD == NU_TRUE)
        /* Do not perform checksum in software if hardware controller can do
         * it
         */
        if ((prt->tp_routev6.rt_route)&&(!(prt->tp_routev6.rt_route->rt_entry_parms.rt_parm_device->
              dev_hw_options_enabled & HW_TX_TCP6_CHKSUM)))
#endif
        {
            PUT16(tcp_ptr, TCP_CHECK_OFFSET,
                  UTL6_Checksum(buf_ptr, ip6_options.tx_source_address,
                                prt->tcp_faddrv6, buf_ptr->mem_total_data_len,
                                IP_TCP_PROT, IP_TCP_PROT) );
        }

        ip6_options.tx_dest_address = prt->tcp_faddrv6;

        status = IP6_Send(buf_ptr, &ip6_options, IP_TCP_PROT, &ip6_options.tx_route,
                          0, NU_NULL);
#if (INCLUDE_IPV4 == NU_TRUE)
    }

    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
#if (HARDWARE_OFFLOAD == NU_TRUE)
        /* Do not perform checksum in software if hardware controller can do
         * it
         */
        if ((prt->tp_route.rt_route)&&(!(prt->tp_route.rt_route->rt_entry_parms.rt_parm_device->
              dev_hw_options_enabled & HW_TX_TCP_CHKSUM)))
#endif
        {
            PUT16(tcp_ptr, TCP_CHECK_OFFSET,
                  UTL_Checksum(buf_ptr, prt->tcp_laddrv4, prt->tcp_faddrv4, IP_TCP_PROT) );
        }

        /* Send this packet. */
        status = IP_Send((NET_BUFFER *)buf_ptr, &prt->tp_route, prt->tcp_faddrv4,
                         prt->tcp_laddrv4, 0, prt->p_ttl,
                         IP_TCP_PROT, prt->p_tos, NU_NULL);
    }
#endif

    return (status);

} /* TCP_Xmit_Probe */

#if (TCPSECURE_DRAFT == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       TCP_Check_SYN
*
*   DESCRIPTION
*
*       This function validates an incoming SYN packet according to
*       Cert Advisory CA-96.21 III - TCP must reject an incoming SYN with
*       source IP address identical to destination IP address as long as
*       the packet is not sent over loopback.
*
*   INPUTS
*
*       *tcp_chk                A pointer to the source and destination
*                               IP addresses.
*       family                  The family type of the packet.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet passes validation.
*       -1                      The packet must be rejected.
*
************************************************************************/
STATIC STATUS TCP_Check_SYN(VOID *tcp_chk, INT16 family)
{
    STATUS  status = NU_SUCCESS;

#if (INCLUDE_IPV4 == NU_TRUE)
    UINT8   loopback_addr[] = {127, 0, 0, 1};
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (family == SK_FAM_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    {
        /* If the source is not the loopback. */
        if (memcmp(((struct pseudohdr*)tcp_chk)->source,
                   IP6_Loopback_Address, IP6_ADDR_LEN) != 0)
        {
            /* If the source and destination are equal. */
            if (memcmp(((struct pseudohdr*)tcp_chk)->source,
                       ((struct pseudohdr*)tcp_chk)->dest, IP6_ADDR_LEN) == 0)
            {
                /* Return an error code. */
                status = -1;

                NLOG_Error_Log("Incoming SYN packet source and destination addresses match",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

            }
        }
    }
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* If the source is not the loopback. */
        if (((struct pseudotcp*)tcp_chk)->source !=
            (LONGSWAP(IP_ADDR(loopback_addr))))
        {
            /* If the source and destination are equal. */
            if (((struct pseudotcp*)tcp_chk)->source ==
                ((struct pseudotcp*)tcp_chk)->dest)
            {
                /* Return an error code. */
                status = -1;

                NLOG_Error_Log("Incoming SYN packet source and destination addresses match",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
            }
        }
    }
#endif

    return (status);

} /* TCP_Check_SYN */

#endif

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Configure_Shift_Count
*
*   DESCRIPTION
*
*       This function determines the number of bits a 32-bit value must
*       be shifted to fit into a 16-bit space.  This value will be
*       placed in the shift count field of an outgoing TCP Window Scale
*       option in a SYN packet.
*
*   INPUTS
*
*       window_size             The value for which to compute the shift
*                               count.
*
*   OUTPUTS
*
*       The number of bits to shift the 32-bit number to fit into a
*       16-bit field.
*
************************************************************************/
UINT8 TCP_Configure_Shift_Count(UINT32 window_size)
{
    UINT8   scale_factor;

    /* If the window is greater than 16-bits, find the appropriate
     * scale factor.
     */
    if (window_size > 65535)
    {
        /* Find the lowest value of scale_factor that will make the window
         * size a 16-bit value.
         */
        for (scale_factor = 1; scale_factor <= 14; scale_factor ++)
        {
            if ((window_size >> scale_factor) <= 65535)
                break;
        }
    }

    else
    {
        scale_factor = 0;
    }

    return (scale_factor);

} /* TCP_Configure_Shift_Count */
