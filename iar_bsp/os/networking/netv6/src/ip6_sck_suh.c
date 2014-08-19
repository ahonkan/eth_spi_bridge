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
*       ip6_sck_suh.c                                
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for setting the IPv6 Hop Limit on
*       an IPv6-Enabled socket.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Set_IPV6_UNICAST_HOPS
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

extern UINT8    IP6_Hop_Limit;

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Set_IPV6_UNICAST_HOPS
*                                                                       
*   DESCRIPTION                                                           
*             
*       This function sets the Hop Limit associated with an IPv6
*       socket.                                                          
*                                                                       
*   INPUTS                                                                
*              
*       socketd                 The socket descriptor associated with
*                               the socket for which to set the option.
*       hop_limit               The new Hop Limit.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              The value was successfully set.
*       NU_INVAL                The new value is invalid or the socket
*                               does not exist.
*                                                                       
*************************************************************************/
STATUS IP6_Set_IPV6_UNICAST_HOPS(INT socketd, INT16 hop_limit)
{
    STATUS  status;

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) || \
      (INCLUDE_IP_RAW == NU_TRUE) )
    UINT16  new_hop_limit;

    /* Verify that the new value is valid */
    if ( ((hop_limit >= 0) && (hop_limit <= 255)) || 
         (hop_limit == -1) )
    {
        /* If hop_limit is -1, restore the default hop limit on the 
         * socket. 
         */
        if (hop_limit != -1)
            new_hop_limit = (UINT16)hop_limit;

        /* Otherwise, set the hop limit to the specified value */
        else
            new_hop_limit = IP6_Hop_Limit;

        switch (SCK_Sockets[socketd]->s_protocol)
        {
#if (INCLUDE_TCP == NU_TRUE)

            case NU_PROTO_TCP:

                TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_ttl = new_hop_limit;
                break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

            case NU_PROTO_UDP:

                UDP_Ports[SCK_Sockets[socketd]->s_port_index]->up_ttl = new_hop_limit;
                break;
#endif

            default:

#if (INCLUDE_IP_RAW == NU_TRUE)

                /* If this is a RAW socket */
                if (IS_RAW_PROTOCOL(SCK_Sockets[socketd]->s_protocol))
                {
                    IPR_Ports[SCK_Sockets[socketd]->s_port_index]->ip_ttl = new_hop_limit;
                }
#endif
                break;        
        }

        status = NU_SUCCESS;
    }

    else
#endif
        status = NU_INVAL;

    return (status);

} /* IP6_Set_IPV6_UNICAST_HOPS */
