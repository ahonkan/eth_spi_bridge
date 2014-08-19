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
*       ip6_sck_stc.c                                
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routines for setting IPv6 Sticky Options
*       on a socket.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Set_IPV6_TCLASS
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
*       IP6_Set_IPV6_TCLASS                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function sets the traffic class Sticky Option for a socket.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       t_class                 The new traffic class Sticky Option.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*                                                                       
*************************************************************************/
STATUS IP6_Set_IPV6_TCLASS(INT socketd, INT t_class)
{
    STATUS              return_status;
    tx_ancillary_data   **data_ptr = NU_NULL;

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
            data_ptr = &(tcp_port->p_sticky_options);

            break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

        case NU_PROTO_UDP:

            udp_port = UDP_Ports[SCK_Sockets[socketd]->s_port_index];
            data_ptr = &(udp_port->up_sticky_options);

            break;
#endif

        default:

#if (INCLUDE_IP_RAW == NU_TRUE)

            /* If this is a RAW socket */
            if (IS_RAW_PROTOCOL(SCK_Sockets[socketd]->s_protocol))
            {
                raw_port = IPR_Ports[SCK_Sockets[socketd]->s_port_index];
                data_ptr = &(raw_port->ip_sticky_options);
            }
#endif
            break;        
    }

    /* Validate the traffic class passed into the function */
    if ( (data_ptr) && (t_class >= -1) && (t_class <= 255) )
        return_status = SCK_Set_TX_Sticky_Options(data_ptr, (VOID*)&t_class, 
                                                  sizeof(INT), IPV6_TCLASS);
    else
        return_status = NU_INVALID_PARM;

    return (return_status);

} /* IP6_Set_IPV6_TCLASS */
