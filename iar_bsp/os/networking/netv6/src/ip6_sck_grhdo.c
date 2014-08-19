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
*       ip6_sck_grhdo.c                              
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for retrieving the Sticky Option
*       value previously set for the destination options preceding the
*       Routing Header on the socket.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Get_IPV6_RTHDRDSTOPTS
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
*       IP6_Get_IPV6_RTHDRDSTOPTS                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function retrieves the Sticky Option value previously set
*       for the destination options preceding the Routing Header on the 
*       socket.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *dst_hdr                A pointer to the ip6_dest to fill
*                               in with the destination options Sticky 
*                               Option data.
*       *optlen                 The length of the data in dst_hdr.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*                                                                       
*************************************************************************/
STATUS IP6_Get_IPV6_RTHDRDSTOPTS(INT socketd, struct ip6_dest *dst_hdr, INT *optlen)
{
    INT                 dst_hdr_len;
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

    /* Insure that the application has specified a routing header */
    if ( (tx_sticky_options) && (tx_sticky_options->tx_rthrdest_opt) )
    {
        dst_hdr_len = 
            8 + (((struct ip6_dest*)tx_sticky_options->tx_rthrdest_opt)->ip6d_len << 3);

        if (dst_hdr_len <= *optlen)
        {
            NU_BLOCK_COPY(dst_hdr, tx_sticky_options->tx_rthrdest_opt,
                          (unsigned int)dst_hdr_len);

            *optlen = dst_hdr_len;
        }
        else
            *optlen = 0;
    }

    /* Otherwise, no routing header was set */
    else
        *optlen = 0;
        
    return (NU_SUCCESS);

} /* IP6_Get_IPV6_RTHDRDSTOPTS */
