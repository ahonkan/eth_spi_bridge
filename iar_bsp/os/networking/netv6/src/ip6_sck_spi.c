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
*       ip6_sck_spi.c                                
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
*       IP6_Set_IPV6_PKTINFO
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
*       IP6_Set_IPV6_PKTINFO                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function sets Sticky Options for a socket.  The user can
*       specify Source Address or Outgoing Interface Index.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *pkt_info               A pointer to the in6_pktinfo structure
*                               containing the Sticky Options to set for
*                               the socket.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_MEM_ALLOC            Insufficient memory.
*       NU_INVAL                The length of the options are greater
*                               than the stack can handle, the address
*                               specified does not exist on the node,
*                               the socket type is TCP and the source
*                               address cannot be changed, the 
*                               interface index is invalid or the
*                               interface is not IPv6-enabled.
*                                                                       
*************************************************************************/
STATUS IP6_Set_IPV6_PKTINFO(INT socketd, in6_pktinfo *pkt_info, INT length)
{
    STATUS              return_status = NU_SUCCESS;
    DV_DEVICE_ENTRY     *dev_ptr = NU_NULL, *temp_dev;
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

    /* If the interface index is not unspecified, ensure that the interface 
     * index references a valid interface on the node.
     */
    if (pkt_info->ipi6_ifindex != IP6_UNSPECIFIED)
    {
        /* Get a pointer to the device */
        dev_ptr = DEV_Get_Dev_By_Index(pkt_info->ipi6_ifindex);

        /* If the interface index is invalid, return an error */
        if (!dev_ptr)
            return_status = NU_INVAL;
    }

    /* If the address is not the unspecified address, ensure that the
     * address exists on the node.
     */
    if ( (return_status == NU_SUCCESS) &&
         (!(IPV6_IS_ADDR_UNSPECIFIED(pkt_info->ipi6_addr))) )
    {
        /* The source address must be the Unspecified address for TCP
         * sockets, because it is not possible to dynamically change
         * the source address of a TCP connection.
         */
        if (SCK_Sockets[socketd]->s_protocol == NU_PROTO_TCP)
        {
            dev_ptr = NU_NULL;
            return_status = NU_INVAL;
        }

        /* Ensure that the address exists on the node */
        else
        {
            temp_dev = DEV6_Get_Dev_By_Addr(pkt_info->ipi6_addr);

            /* If no interface index was specified, set dev_ptr to the
             * value returned above.
             */
            if (!dev_ptr)
                dev_ptr = temp_dev;

            /* If the address does not exist on the node or the address
             * does not exist on the same node as the interface index.
             */
            if ( (!temp_dev) || (temp_dev != dev_ptr) )
                return_status = NU_INVAL;
        }
    }

    /* If the device exists and is enabled for IPv6 or the user is clearing
     * a previously set option.
     */
    if (data_ptr)
    {
        if ( ((dev_ptr) && (dev_ptr->dev_flags & DV6_IPV6)) ||
             (return_status == NU_SUCCESS) )
        {
            /* Store the sticky option in the socket structure */
            return_status = SCK_Set_TX_Sticky_Options(data_ptr, (VOID*)pkt_info, 
                                                      length, IPV6_PKTINFO);
        }
    }

    else
        return_status = NU_INVALID_SOCKET;

    return (return_status);

} /* IP6_Set_IPV6_PKTINFO */
