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
*       ip6_sck_gnh.c                                
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for retrieving the value of the
*       next-hop Sticky Option that was set by the application.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Get_IPV6_NEXTHOP
*                                                                          
*   DEPENDENCIES                                                             
*              
*       nu_net.h
*                                                                          
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IP_RAW == NU_TRUE)
#include "networking/ipraw.h"
#endif

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Get_IPV6_NEXTHOP                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function retrieves the value of the next-hop Sticky Option
*       that was set by the application.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *addr_struct            A pointer to the addr_struct to fill
*                               in with the next-hop Sticky Option data.
*       *optlen                 The length of the data in addr_struct.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*                                                                       
*************************************************************************/
STATUS IP6_Get_IPV6_NEXTHOP(INT socketd, struct addr_struct *addr_struct, 
                            INT *optlen)
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

    /* If a Next-Hop value was previously set by the application, return
     * it.
     */
    if ( (tx_sticky_options) && (tx_sticky_options->tx_next_hop) )
    {
        /* Set the length of the value being returned */
        *optlen = sizeof(struct addr_struct);

        /* Copy the addr_struct from memory */
        NU_BLOCK_COPY(addr_struct, tx_sticky_options->tx_next_hop,
                      (unsigned int)*optlen);        
    }

    /* Otherwise, set the length to zero */
    else
        *optlen = 0;

    return (NU_SUCCESS);

} /* IP6_Get_IPV6_NEXTHOP */
