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
*   FILE NAME
*
*       tcp4.c
*
*   DESCRIPTION
*
*       TCP Protocol routines for IPv4 packets.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TCP4_Input
*       TCP4_Find_Matching_TCP_Port
*       TCP4_Free_Cached_Route
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       TCP4_Input
*
*   DESCRIPTION
*
*       This function verifies the TCP checksum of an IPv4 TCP packet,
*       and forwards the packet to the TCP_Interpret routine.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the packet.
*       *tcp_chk                A pointer to the data structure holding
*                               the pseudoheader information on which to
*                               compute the checksum.
*
*   OUPUTS
*
*       -2
*       -1
*       NU_SUCCESS
*
*************************************************************************/
INT16 TCP4_Input(NET_BUFFER *buf_ptr, struct pseudotcp *tcp_chk)
{
	static INT32	cached_prt = -1;
    TCP_PORT    	*prt;
    UINT16      	i, myport, hlen, hisport;
    UINT32      	source_addr, local_addr;

    /* Check to see if this originated from a broadcast or multicast
     * address. If so, silently drop this packet.
     */
    if ((buf_ptr->mem_flags & NET_BCAST) || (buf_ptr->mem_flags & NET_MCAST))
    {
        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);
        return (NU_SUCCESS);
    }

    /* Increment the number of TCP segments received. */
    MIB2_tcpInSegs_Inc;

    /* If the checksum was computed in the controller bypass the software \
     * checksum efforts.
     */
#if (HARDWARE_OFFLOAD == NU_TRUE)
    if (!(buf_ptr->hw_options & HW_RX_TCP_CHKSUM))
#endif
    {
        /* Verify that the incoming checksum is correct */
        if (GET16(buf_ptr->data_ptr, TCP_CHECK_OFFSET))
        {
            /* compute the checksum */
            if (TLS_TCP_Check((UINT16 *)tcp_chk, buf_ptr))
            {
                NLOG_Error_Log ("TCP checksum error", NERR_RECOVERABLE,
                                    __FILE__, __LINE__);

                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

                /* Increment the number of TCP packets received with errors. */
                MIB2_tcpInErrs_Inc;

                return (2);
            }  /* end if, for compute of the checksum */
        } /* end if, we need to do the checksum */
    }

    /* bytes offset to data */
    hlen = (UINT16)(GET8(buf_ptr->data_ptr, TCP_HLEN_OFFSET) >> 2);

    /* Set the option len for this packet. */
    buf_ptr->mem_option_len = (UINT16)(hlen - TCP_HEADER_LEN);

    /*
     *  find the port which is associated with the incoming packet
     *  First try open connections, then try listeners
     */
    myport = GET16(buf_ptr->data_ptr, TCP_DEST_OFFSET);
    hisport = GET16(buf_ptr->data_ptr, TCP_SRC_OFFSET);

    /* Store off the LONGSWAPPED source address so we don't have to
     * perform the operation every time through the loop.
     */
    source_addr = LONGSWAP(tcp_chk->source);
    local_addr = LONGSWAP(tcp_chk->dest);

    /* This is a minor optimization. Check to see if this packet is for the
     * last connection that received a packet.  This logic will never match
     * a listener port, so this does not duplicate functionality within
     * SCK_Check_Listeners for storing the last accessed listener socket.
     */
    if (cached_prt != -1)
	{
    	prt = TCP_Ports[cached_prt];

    	/* Match the port structure exactly. */
    	if ( (prt) && (prt->in.port == myport) && (prt->out.port == hisport) &&
    		 (prt->tcp_faddrv4 == source_addr) && (prt->tcp_laddrv4 == local_addr) )
    	{
    		return (TCP_Do (prt, buf_ptr, hlen, tcp_chk, (INT16)prt->state));
    	}
	}

    for (i = 0; i < TCP_MAX_PORTS; i++)
    {
        prt = TCP_Ports[i];

        if ( (prt != NU_NULL) && (prt->portFlags & TCP_FAMILY_IPV4) &&
             (prt->in.port == myport) && (prt->out.port == hisport) &&
             (prt->tcp_faddrv4 == source_addr) &&
             (prt->tcp_laddrv4 == local_addr) )
        {
        	/* Cache this as the last accessed connection. */
        	cached_prt = i;

            return (TCP_Do (prt, buf_ptr, hlen, tcp_chk, (INT16)prt->state));
        }
    }

    return (TCP_Interpret(buf_ptr, tcp_chk, SK_FAM_IP, myport, hlen));

} /* TCP4_Input */

/*************************************************************************
*
*   FUNCTION
*
*       TCP4_Find_Matching_TCP_Port
*
*   DESCRIPTION
*
*       This function returns the TCP port associated with the parameters
*       provided.
*
*   INPUTS
*
*       source_ip               The Source Address of the target entry.
*       dest_ip                 The Destination Address of the target
*                               entry.
*       source_port             The source port of the target entry.
*       dest_port               The destination port of the target entry.
*
*   OUTPUTS
*
*       The index of the corresponding port.
*       -1 if no port found.
*
*************************************************************************/
INT32 TCP4_Find_Matching_TCP_Port(UINT32 source_ip, UINT32 dest_ip,
                                  UINT16 source_port, UINT16 dest_port)
{
    INT         i;
    TCP_PORT    *prt;
    STATUS      status = -1;

    /* Search through the TCP_Ports for the port matching our Source and
     * Destination port and IP address.
     */
    for (i = 0; i < TCP_MAX_PORTS; i++)
    {
        prt = TCP_Ports[i];

        /* If this is the port we want, assign an error code */
        if ( (prt != NU_NULL) && (prt->portFlags & TCP_FAMILY_IPV4) &&
             (prt->in.port == source_port) && (prt->out.port == dest_port) &&
             (prt->tcp_laddrv4 == source_ip) && (prt->tcp_faddrv4 == dest_ip) )
        {
            status = i;
            break;
        }
    }

    return (status);

} /* TCP4_Find_Matching_TCP_Port */

/*************************************************************************
*
*   FUNCTION
*
*       TCP4_Free_Cached_Route
*
*   DESCRIPTION
*
*       This function invalidates the cached route for all sockets
*       using the specified route.
*
*   INPUTS
*
*       rt_entry                The route that is being deleted and
*                               therefore must not be used by any TCP
*                               ports in the future.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID TCP4_Free_Cached_Route(RTAB4_ROUTE_ENTRY *rt_entry)
{
    UINT16  i;

    /* Invalidate all cached routes using this interface on a TCP
     * socket
     */
    for (i = 0; i < TCP_MAX_PORTS; i++)
    {
        /* If this port is in use by a socket. */
        if (TCP_Ports[i])
        {
            /* If the cached route for this port is the route that is
             * being deleted.
             */
            if (TCP_Ports[i]->tp_route.rt_route == rt_entry)
            {
                /* Free the route. */
                RTAB_Free((ROUTE_ENTRY*)TCP_Ports[i]->tp_route.rt_route,
                          NU_FAMILY_IP);

                /* Set the cached route to NU_NULL.  The next time data
                 * is transmitted using this port, a new route will be
                 * found.
                 */
                TCP_Ports[i]->tp_route.rt_route = NU_NULL;
            }
        }
    }

} /* TCP4_Free_Cached_Route */

#endif
