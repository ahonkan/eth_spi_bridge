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
*       nd6rsol.c                                    
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains those functions necessary to process an 
*       incoming Router Solicitation message and build and transmit an 
*       outgoing Router Solicitation message.
*                                                                          
*   DATA STRUCTURES                                                          
*                                      
*       None                                    
*                                                                          
*   FUNCTIONS                                                                
*           
*       ND6RSOL_Output
*       ND6RSOL_Build
*                                                                          
*   DEPENDENCIES                                                             
*               
*       nu_net.h
*       externs6.h
*       nd6.h
*       nd6rsol.h
*                                                                          
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/externs6.h"
#include "networking/nd6.h"
#include "networking/nd6rsol.h"

/*************************************************************************
*
*   FUNCTION
*
*       ND6RSOL_Output
*
*   DESCRIPTION
*
*       This function builds and transmit a Router Solicitation.
*
*   INPUTS
*
*       *device                     A pointer to the device out which to 
*                                   send the packet.
*       *dest_address               The Destination Address of the packet.
*       *source_address             The Source Address of the packet.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       NU_NO_BUFFERS           Insufficient NET Buffers.
*       NU_HOST_UNREACHABLE     A route to the host does not exist.
*       NU_MSGSIZE              The size of the message is wrong.
*       -1						DAD has failed, and RS's cannot be sent.
*
*************************************************************************/
STATUS ND6RSOL_Output(DV_DEVICE_ENTRY *device, UINT8 *dest_address, 
                      UINT8 *source_address)
{
    NET_BUFFER                  *buf_ptr;
    UINT8                       *link_addr = NU_NULL;
    UINT16                      checksum;
    STATUS                      status;
    MULTI_SCK_OPTIONS           multi_opts;
    IP6_S_OPTIONS               ip6_options;
    RTAB6_ROUTE                 dest_route;

    /* If the device has been disabled for sending IPv6 packets, do not send
     * the Router Solicitation.  This can happen if DAD fails on the link-local
     * address.
     */
    if (!(device->dev_flags2 & DV6_UP))
    {
        NLOG_Error_Log("Cannot transmit Router Solicitation b/c device is down",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

    	return (-1);
    }

    /* If the Source Address is not the Unspecified Address, include
     * the Source Link-Layer address option.
     */
    if (!IPV6_IS_ADDR_UNSPECIFIED(source_address))
        link_addr = device->dev_mac_addr;

    /* Build the ICMP Router Solicitation portion of the packet */
    buf_ptr = ND6RSOL_Build(device, link_addr, source_address);

    if (buf_ptr != NU_NULL)
    {
        /* Compute the Checksum */
        checksum = UTL6_Checksum(buf_ptr, source_address, dest_address, 
                                 buf_ptr->mem_total_data_len, IPPROTO_ICMPV6, 
                                 IPPROTO_ICMPV6);

        PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, checksum);

        UTL_Zero(&multi_opts, sizeof(MULTI_SCK_OPTIONS));

        /* If the Destination Address is a multicast address, set the multicast
         * option for the device.
         */
        if (IPV6_IS_ADDR_MULTICAST(dest_address))
        {
            dest_route.rt_route = NU_NULL;
            multi_opts.multio_device = device;
        }

        else
        {
            /* Find a route to the destination using the correct interface */
            dest_route.rt_route = 
                (RTAB6_ROUTE_ENTRY*)(RTAB6_Find_Route_By_Device(dest_address, device));

            if (dest_route.rt_route)
            {
                NU_BLOCK_COPY(dest_route.rt_ip_dest.rtab6_rt_ip_dest.sck_addr,
                              dest_address, IP6_ADDR_LEN);

                dest_route.rt_ip_dest.rtab6_rt_ip_dest.sck_family = SK_FAM_IP6;
                dest_route.rt_ip_dest.rtab6_rt_device = device;
            }
        }

        memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));
       
        ip6_options.tx_source_address = source_address;
        ip6_options.tx_dest_address = dest_address;
        ip6_options.tx_hop_limit = ICMP6_VALID_HOP_LIMIT;

        /* Increment the number of ICMP messages sent. */
        MIB_ipv6IfIcmpOutMsgs_Inc(device);

        status = IP6_Send(buf_ptr, &ip6_options, IP_ICMPV6_PROT, &dest_route, 
                          IP6_DONTROUTE, &multi_opts);
        
        if (status != NU_SUCCESS)
        {
            /* Increment the number of send errors. */
            MIB_ipv6IfIcmpOutErrors_Inc(device);

            NLOG_Error_Log("Could not send Router Solicitation", NERR_SEVERE, 
                           __FILE__, __LINE__);
    
            /* The packet was not sent.  Deallocate the buffer.  If the packet was
             * transmitted it will be deallocated when the transmit complete
             * interrupt occurs. 
             */
            MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
        }

        else
            MIB_ipv6IfIcmpOutRtSolic_Inc(device);

        /* If a route was found, free it. */
        if (dest_route.rt_route)
            RTAB_Free((ROUTE_ENTRY*)dest_route.rt_route, NU_FAMILY_IP6);
    }
    else
    {
        /* Increment the number of send errors. */
        MIB_ipv6IfIcmpOutErrors_Inc(device);

        NLOG_Error_Log("Could not build Router Solicitation", 
                       NERR_SEVERE, __FILE__, __LINE__);

        status = NU_NO_BUFFERS;
    }

    return (status);

} /* ND6RSOL_Output */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ND6RSOL_Build
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function will build a Router Solicitation message destined
*       for the All-Routers Multicast Address.
*                                                                         
*   INPUTS                                                                
*             
*       *device                 A pointer to the device.                                                            
*       *link_addr              The link-layer address of the sending 
*                               node.
*       *source_addr            The source address of the sending node.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       A pointer to the NET_BUFFER or NU_NULL if no buffers are
*       available.
*
*************************************************************************/
NET_BUFFER *ND6RSOL_Build(const DV_DEVICE_ENTRY *device, 
                          const UINT8 *link_addr, UINT8 *source_addr)
{
    UINT32      message_size;
    NET_BUFFER  *buf_ptr;

    UNUSED_PARAMETER(source_addr);

    /* If the link_addr is not NULL and the outgoing interface is not a 
     * virtual interface, include the Link-Layer Option. Otherwise, do 
     * not include the Link-Layer Option.
     */
    if ( (!(device->dev_flags & DV6_VIRTUAL_DEV)) &&
         (link_addr != NU_NULL) )
        message_size = IP6_RTR_SOL_HDR_SIZE + IP6_LINK_LAYER_OPT_SIZE +
                       device->dev_addrlen;
    else
        message_size = IP6_RTR_SOL_HDR_SIZE;

    /* Set the common values of the ICMP header */
    buf_ptr = ICMP6_Header_Init(ICMP6_RTR_SOL, 0, message_size);

    /* If we were able to get a buffer */
    if (buf_ptr != NU_NULL)
    {
        /* Initialize the checksum */
        PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, 0);

        PUT32(buf_ptr->data_ptr, IP6_ICMP_RTR_SOL_RES_OFFSET, 0);
      
        /* Build the indicated options */
        if ( (!(device->dev_flags & DV6_VIRTUAL_DEV)) &&
             (link_addr != NU_NULL) )             
        {
            buf_ptr->data_ptr += IP6_RTR_SOL_HDR_SIZE;

            ND6_Build_Link_Layer_Opt(IP6_ICMP_OPTION_SRC_ADDR, link_addr, 
                                     buf_ptr, device->dev_addrlen);

            buf_ptr->data_ptr -= IP6_RTR_SOL_HDR_SIZE;
        }

    }
    else
        NLOG_Error_Log("No buffers available", NERR_SEVERE, __FILE__, 
                       __LINE__);

    return (buf_ptr);

} /* ND6RSOL_Build */
