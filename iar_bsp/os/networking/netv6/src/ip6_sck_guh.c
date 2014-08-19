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
*       ip6_sck_guh.c                                
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for getting the IPv6 Hop Limit on
*       an IPv6-Enabled socket.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Get_IPV6_UNICAST_HOPS
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
*       IP6_Get_IPV6_UNICAST_HOPS
*                                                                       
*   DESCRIPTION                                                           
*             
*       This function sets the Hop Limit associated with an IPv6
*       socket.                                                          
*                                                                       
*   INPUTS                                                                
*              
*       socketd                 The socket descriptor associated with
*                               the socket for which to get the option.
*       hop_limit               The new Hop Limit.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              The value was successfully retrieved.
*                                                                       
*************************************************************************/
STATUS IP6_Get_IPV6_UNICAST_HOPS(INT socketd, INT16 *hop_limit)
{
    switch (SCK_Sockets[socketd]->s_protocol)
    {
#if (INCLUDE_TCP == NU_TRUE)

        case NU_PROTO_TCP:

            *hop_limit = 
                (INT16)(TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_ttl);
            break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

        case NU_PROTO_UDP:

            *hop_limit = 
                (INT16)(UDP_Ports[SCK_Sockets[socketd]->s_port_index]->up_ttl);
            break;
#endif

        default:

#if (INCLUDE_IP_RAW == NU_TRUE)

            /* If this is a RAW socket */
            if (IS_RAW_PROTOCOL(SCK_Sockets[socketd]->s_protocol))
            {
                *hop_limit = 
                    (INT16)(IPR_Ports[SCK_Sockets[socketd]->s_port_index]->ip_ttl);
            }
#endif

            break;        
    }
       
    return (NU_SUCCESS);

} /* IP6_Get_IPV6_UNICAST_HOPS */
