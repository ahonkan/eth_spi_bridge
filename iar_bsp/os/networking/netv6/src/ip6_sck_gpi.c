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
*       ip6_sck_gpi.c                                
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for retrieving the Sticky Option
*       values previously set for the source address and interface 
*       index on the socket.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Get_IPV6_PKTINFO
*                                                                          
*   DEPENDENCIES                                                             
*              
*       nu_net.h
*       nu_net6.h
*                                                                          
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/nu_net6.h"

#if (INCLUDE_IP_RAW == NU_TRUE)
#include "networking/ipraw.h"
#endif

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Get_IPV6_PKTINFO                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function retrieves the Sticky Option values previously set
*       for the source address and interface index on the socket.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *pkt_info               A pointer to the in6_pktinfo structure
*                               to be filled in by the kernel.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*                                                                       
*************************************************************************/
STATUS IP6_Get_IPV6_PKTINFO(INT socketd, in6_pktinfo *pkt_info)
{
    tx_ancillary_data   *tx_sticky_options = NU_NULL;

#if (INCLUDE_UDP == NU_TRUE)
    struct uport        *udp_port;
#endif
#if (INCLUDE_TCP == NU_TRUE)
    struct _TCP_Port    *tcp_port;
#endif
#if (INCLUDE_IP_RAW == NU_TRUE)
    struct iport        *raw_port;
#endif

    switch (SCK_Sockets[socketd]->s_protocol)
    {
#if (INCLUDE_TCP == NU_TRUE)

        case NU_PROTO_TCP:

            tcp_port = TCP_Ports[SCK_Sockets[socketd]->s_port_index];
            tx_sticky_options = tcp_port->p_sticky_options;

            break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

        case NU_PROTO_UDP:

            udp_port = UDP_Ports[SCK_Sockets[socketd]->s_port_index];
            tx_sticky_options = udp_port->up_sticky_options;

            break;
#endif

        default:

#if (INCLUDE_IP_RAW == NU_TRUE)

            /* If this is a RAW socket */
            if (IS_RAW_PROTOCOL(SCK_Sockets[socketd]->s_protocol))
            {
                raw_port = IPR_Ports[SCK_Sockets[socketd]->s_port_index];
                tx_sticky_options = raw_port->ip_sticky_options;
            }
#endif

            break;        
    }

    /* Insure that the application has specified a Source Address */
    if ( (tx_sticky_options) && (tx_sticky_options->tx_source_address) )
    {
        /* Copy the Source Address previously set */
        NU_BLOCK_COPY(pkt_info->ipi6_addr, 
                      tx_sticky_options->tx_source_address, IP6_ADDR_LEN);
    }

    /* Otherwise, no Source Address was set - set the address field to
     * the Unspecified Address.
     */
    else
        NU_BLOCK_COPY(pkt_info->ipi6_addr, IP6_ADDR_ANY.is_ip_addrs, 
                      IP6_ADDR_LEN);

    /* Insure that the application has specified an outgoing interface */
    if ( (tx_sticky_options) && (tx_sticky_options->tx_interface_index) )
    {
        /* Copy the interface index previously set. */
        pkt_info->ipi6_ifindex = GET32(tx_sticky_options->tx_interface_index, 0);
    }

    /* Otherwise, no interface index was set - set the interface index
     * field to zero.
     */
    else
        pkt_info->ipi6_ifindex = IP6_UNSPECIFIED;

    return (NU_SUCCESS);

} /* IP6_Get_IPV6_PKTINFO */
