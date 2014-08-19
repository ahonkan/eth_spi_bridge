/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
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
*       tcp6.c                                       
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       TCP Protocol routines for IPv6 packets.
*
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*           
*       TCP6_Input
*       TCP6_Find_Matching_TCP_Port
*       TCP6_Free_Cached_Route
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       externs.h
*       tcp6.h
*       externs6.h
*                                                                       
*************************************************************************/

#include "networking/externs.h"
#include "networking/tcp6.h"
#include "networking/externs6.h"

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       TCP6_Input                                                  
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function verifies the TCP checksum of an IPv6 TCP packet,
*       and forwards the packet to the TCP_Interpret routine.         
*                                                                       
*   INPUTS                                                                
*                                                         
*       *buf_ptr                A pointer to the packet.
*       *pseudoheader           A pointer to the data structure holding 
*                               the pseudoheader information on which to 
*                               compute the checksum.           
*                                                                       
*   OUTPUTS                                                                
*                                
*       -2
*       -1
*       NU_SUCCESS                                       
*                                                                       
*************************************************************************/
INT16 TCP6_Input(NET_BUFFER *buf_ptr, struct pseudohdr *pseudoheader)
{
    TCP_PORT    *prt;
    UINT16      i, myport, hlen, hisport;

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
    if (!(buf_ptr->hw_options & HW_RX_TCP6_CHKSUM))
#endif
    {
        /* Verify that the incoming checksum is correct */
        if (GET16(buf_ptr->data_ptr, TCP_CHECK_OFFSET))
        {
            /* compute the checksum */
            if (TLS6_Prot_Check((UINT16 *)pseudoheader, buf_ptr))
            {
                NLOG_Error_Log("TCP checksum error", NERR_RECOVERABLE, 
                               __FILE__, __LINE__);

                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                /* Increment the number of TCP packets received with errors. */
                MIB2_tcpInErrs_Inc;

                return (2);
            }  /* end if, for compute of the checksum */
        } /* end if, we need to do the checksum */
    }

    /*
     *  find the port which is associated with the incoming packet
     *  First try open connections, then try listeners
     */
    myport = GET16(buf_ptr->data_ptr, TCP_DEST_OFFSET);
    hisport = GET16(buf_ptr->data_ptr, TCP_SRC_OFFSET);

    /* bytes offset to data */
    hlen = (UINT16)(GET8(buf_ptr->data_ptr, TCP_HLEN_OFFSET) >> 2);

    /* Set the option len for this packet. */
    buf_ptr->mem_option_len = (UINT16)(hlen - TCP_HEADER_LEN);

    for (i = 0; i < TCP_MAX_PORTS; i++)
    {
        prt = TCP_Ports[i];

        if ( (prt != NU_NULL) && (prt->portFlags & TCP_FAMILY_IPV6) &&
             (prt->in.port == myport) && (prt->out.port == hisport) && 
             (memcmp(prt->tcp_faddrv6, pseudoheader->source, IP6_ADDR_LEN) == 0) )
            return (TCP_Do (prt, buf_ptr, hlen, pseudoheader, (INT16)prt->state));
    } /* end for, i < TCP_MAX_PORTS */

    return (TCP_Interpret(buf_ptr, pseudoheader, SK_FAM_IP6, myport, hlen));

} /* TCP6_Input */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       TCP6_Find_Matching_TCP_Port                                                     
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function returns the TCP port associated with the parameters
*       provided.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *source_ip              A pointer to the Source Address of the 
*                               target entry.
*       *dest_ip                A pointer to the Destination Address of 
*                               the target entry.    
*       source_port             The source port of the target entry.
*       dest_port               The destination port of the target entry.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       The index of the corresponding port.
*       -1 if no port found.
*                                                                       
*************************************************************************/
INT32 TCP6_Find_Matching_TCP_Port(const UINT8 *source_ip, const UINT8 *dest_ip, 
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
        if ( (prt != NU_NULL) && (prt->portFlags & TCP_FAMILY_IPV6) &&
             (prt->in.port == source_port) && (prt->out.port == dest_port) && 
             (memcmp(prt->tcp_laddrv6, source_ip, IP6_ADDR_LEN) == 0) && 
             (memcmp(prt->tcp_faddrv6, dest_ip, IP6_ADDR_LEN) == 0) )
        {
            status = i;
            break;
        }
    }

    return (status);

} /* TCP6_Find_Matching_TCP_Port */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       TCP6_Free_Cached_Route                                                     
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
VOID TCP6_Free_Cached_Route(RTAB6_ROUTE_ENTRY *rt_entry)
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
            if (TCP_Ports[i]->tp_routev6.rt_route == rt_entry)
            {
                /* Free the route. */
                RTAB_Free((ROUTE_ENTRY*)TCP_Ports[i]->tp_routev6.rt_route, 
                          NU_FAMILY_IP6);

                /* Set the cached route to NU_NULL.  The next time data
                 * is transmitted using this port, a new route will be
                 * found.
                 */
                TCP_Ports[i]->tp_routev6.rt_route = NU_NULL;
            }
        }
    }

} /* TCP6_Free_Cached_Route */
