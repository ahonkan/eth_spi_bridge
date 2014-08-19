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
*       ip6_sck_gtc.c                                
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for retrieving the Sticky Option
*       value previously set for the traffic class on the socket.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Get_IPV6_TCLASS
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
*       IP6_Get_IPV6_TCLASS                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function retrieves the Sticky Option value previously set
*       for the traffic class on the socket.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *t_class                A pointer to the memory to fill in with
*                               the traffic class.
*       *optlen                 The length of the data in t_class.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*                                                                       
*************************************************************************/
STATUS IP6_Get_IPV6_TCLASS(INT socketd, INT *t_class, INT *optlen)
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

    /* Insure that the application has specified a traffic class */
    if ( (tx_sticky_options) && (tx_sticky_options->tx_traffic_class) )
        *t_class = *tx_sticky_options->tx_traffic_class;

    /* Otherwise, no traffic class was set - return the kernel default */
    else
        *t_class = IP6_TCLASS_DEFAULT;

    *optlen = (INT)sizeof(INT);

    return (NU_SUCCESS);

} /* IP6_Get_IPV6_TCLASS */
