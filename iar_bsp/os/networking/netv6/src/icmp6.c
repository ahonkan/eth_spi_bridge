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
*       icmp6.c                                      
*                                                                       
*   DESCRIPTION                                                           
*              
*       This file contains those functions necessary to send and receive
*       IPv6 ICMP messages.
*
*   DATA STRUCTURES
*
*       ICMP6_RtrSol_Event
*                                                                       
*   FUNCTIONS                                                             
*                
*       ICMP6_Init                                                       
*       ICMP6_Interpret    
*       ICMP6_Header_Init   
*       ICMP6_Random_Delay     
*       ICMP6_RtrSolicitation_Event  
*       ICMP6_Echo_Reply
*       ICMP6_Process_Error
*       ICMP6_Process_Packet_Too_Big
*       ICMP6_Send_Error
*                                                                       
*   DEPENDENCIES                                                          
*        
*       externs.h
*       externs6.h
*       mld6.h
*       nd6rsol.h
*       nd6radv.h
*       nd6nsol.h
*       nd6nadv.h
*       nd6rdrct.h
*       in6.h
*       nd6.h
*       nc6.h
*       ip6_mib.h
*                                                                       
*************************************************************************/

#include "networking/externs.h"
#include "networking/externs6.h"
#include "networking/mld6.h"
#include "networking/nd6rsol.h"
#include "networking/nd6radv.h"
#include "networking/nd6nsol.h"
#include "networking/nd6nadv.h"
#include "networking/nd6rdrct.h"
#include "networking/in6.h"
#include "networking/nd6.h"
#include "networking/nc6.h"
#include "networking/ip6_mib.h"

#if (INCLUDE_IPSEC == NU_TRUE)
#include "networking/ips_api.h"
#endif

TQ_EVENT        ICMP6_RtrSol_Event;

extern ICMP_ECHO_LIST   ICMP_Echo_List;
extern UINT8            IP6_All_Routers_Multi[];

/*************************************************************************
*   
*   FUNCTION                                                                                                                                 
*                                                                       
*       ICMP6_Init                                                        
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Initialize the ICMPv6 module. In this case is only consists of     
*       nulling the ICMP_Echo_List.                                           
*                                                                       
*   INPUTS                                                                
*                                                                       
*       None                                                             
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       None                                                             
*                                                                       
*************************************************************************/
VOID ICMP6_Init(VOID)
{
    /* Register the event to transmit an initial sequence of Router 
     * Solicitation packets.
     */
    if (EQ_Register_Event(ICMP6_RtrSolicitation_Event, 
                          &ICMP6_RtrSol_Event) != NU_SUCCESS)
        NLOG_Error_Log("Failed to register Router Solicitation event", 
                       NERR_SEVERE, __FILE__, __LINE__);

} /* ICMP6_Init */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ICMP6_Interpret
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function determines the type of ICMPv6 packet received and
*       processes it accordingly.  The data pointer of the buf_ptr is
*       pointing to the beginning of the ICMP header.
*                                                                         
*   INPUTS                                                                
*
*       *pkt                    A pointer to the IPv6 header of the 
*                               packet.                                
*       *device                 A pointer to the device on which the 
*                               packet was received.
*       *buf_ptr                A pointer to the buffer.                   
*       *pseudoheader           A pointer to the pseudoheader on which 
*                               to verify the checksum.                   
*
*   OUTPUTS                                                               
*                                           
*       NU_SUCCESS              The packet was successfully processed.
*       1                       Invalid ICMP Checksum.
*       -1                      No ICMP Echo Request for Echo Reply.
*
*************************************************************************/
STATUS ICMP6_Interpret(IP6LAYER *pkt, DV_DEVICE_ENTRY *device, 
                       NET_BUFFER *buf_ptr, struct pseudohdr *pseudoheader)
{
    STATUS                  status = NU_SUCCESS;
    ICMP_ECHO_LIST_ENTRY    *echo_entry;
#if ((INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE))
    ICMP6_LAYER             *icmp_header;
#endif  

    /* Increment the total number of ICMP messages received. */
    MIB_ipv6IfIcmpInMsgs_Inc(device);

    /* Verify the ICMP checksum */
    if (TLS6_Prot_Check((UINT16*)pseudoheader, buf_ptr))
    {
        NLOG_Error_Log("ICMPv6 Checksum invalid for incoming packet", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        /* Increment the number of ICMP messages received with errors. */
        MIB_ipv6IfIcmpInErrors_Inc(device);

        return (1);
    }

#if ( (INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE) )

    /* Get a pointer to the ICMPv6 header */
    icmp_header = (ICMP6_LAYER *)buf_ptr->data_ptr;

    /* Print/store the ICMP header info */
    NLOG_ICMP6_Info(icmp_header, NLOG_RX_PACK);

#endif  

    /* Process the ICMP message */
    switch (buf_ptr->data_ptr[IP6_ICMP_TYPE_OFFSET])
    {
    case ICMP6_NEIGH_SOL:

        MIB_ipv6IfIcmpInNeigSolic_Inc(device);

        /* Process the Neighbor Solicitation Message */
        status = ND6NSOL_Input(pkt, device, buf_ptr);

        /* Return the buffer back to the free buffer pool. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
        
        break;

    case ICMP6_NEIGH_ADV:

        MIB_ipv6IfIcmpInNeigAdv_Inc(device);

        /* Process the Neighbor Advertisement Message */
        status = ND6NADV_Input(pkt, device, buf_ptr);

        /* Return the buffer back to the free buffer pool. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
        
        break;
       
    case ICMP6_RTR_ADV:

        MIB_ipv6IfIcmpInRtAdv_Inc(device);

        /* Process the Router Advertisement Message */
        status = ND6RADV_Input(pkt, device, buf_ptr);

        /* Return the buffer back to the free buffer pool. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

    case ICMP6_REDIRECT:

        MIB_ipv6IfIcmpInRedirects_Inc(device);

        /* Process the Redirect Message */
        status = ND6RDRCT_Input(pkt, device, buf_ptr);

        /* Return the buffer back to the free buffer pool. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

    case ICMP6_ECHO_REQUEST:

        /* Increment the number of Echo requests received. */
        MIB_ipv6IfIcmpInEchos_Inc(device);

        /* Set the type to Echo Reply */
        PUT8(buf_ptr->data_ptr, IP6_ICMP_TYPE_OFFSET, ICMP6_ECHO_REPLY);

        /* Remove the buffer from the buffer list. The buffer will be
         * reused, it will be sent back as a echo reply. 
         */
        if (MEM_Buffer_Dequeue(&MEM_Buffer_List) == NU_NULL)
            NLOG_Error_Log("Failed Dequeue the incoming buffer", 
                           NERR_SEVERE, __FILE__, __LINE__);

        /* Send an Echo Reply in response */
        status = ICMP6_Echo_Reply (buf_ptr, pkt);

        break;

    case ICMP6_ECHO_REPLY:

        /* Increment the number of Echo Replies received. */
        MIB_ipv6IfIcmpInEchoReply_Inc(device);

        /* Search the list looking for a matching ID and seq num. */
        for (echo_entry = ICMP_Echo_List.icmp_head;
             (echo_entry) && 
             (!((GET16(buf_ptr->data_ptr, IP6_ICMP_ECHO_ID) == ICMP_ECHO_REQ_ID) && 
             (GET16(buf_ptr->data_ptr, IP6_ICMP_ECHO_SEQ) == echo_entry->icmp_echo_seq_num)));
            echo_entry = echo_entry->icmp_next)

        /* If an entry was found mark the status as success, resume
           the requesting task, and unset the timeout timer. */
        if (echo_entry != NU_NULL)
        {
            status = NU_SUCCESS;

            /* set status as success */
            echo_entry->icmp_echo_status = NU_SUCCESS;

            /* unset the timeout timer. */
            if (TQ_Timerunset(ICMP_ECHO_TIMEOUT, TQ_CLEAR_ALL_EXTRA, 
                              (UNSIGNED)echo_entry, NU_NULL) != NU_SUCCESS)
                NLOG_Error_Log("Failed to delete the Echo Reply timer", 
                               NERR_SEVERE, __FILE__, __LINE__);

            /* resume the task */
            if (NU_Resume_Task(echo_entry->icmp_requesting_task) != NU_SUCCESS)
                NLOG_Error_Log("Failed to resume the requesting task", 
                               NERR_SEVERE, __FILE__, __LINE__);
        }
        else
            status = -1;

        /* Return the buffer back to the free buffer pool. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

    case ICMP6_PACKET_TOO_BIG:

#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)

        ICMP6_Process_Packet_Too_Big(buf_ptr);

#endif

    case ICMP6_DST_UNREACH:
    case ICMP6_TIME_EXCEEDED:
    case ICMP6_PARAM_PROB:

        ICMP6_Process_Error(buf_ptr);

        status = NU_SUCCESS;

        /* Return the buffer back to the free buffer pool. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

    case MLD6_LISTENER_QUERY:
    case MLDV1_LISTENER_REPORT:
    case MLDV2_LISTENER_REPORT:
    case MLD6_LISTENER_DONE:

        status = MLD6_Input(pkt, device, buf_ptr);

        /* Return the buffer back to the free buffer pool. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

#endif

    case ICMP6_WRUREQUEST:
    case ICMP6_WRUREPLY:
    case ICMP6_ROUTER_RENUMBERING:

        status = 0;

        /* Return the buffer back to the free buffer pool. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

    case ICMP6_RTR_SOL:

        MIB_ipv6IfIcmpInRtSolic_Inc(device);

#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

        /* Process the Router Solicitation Message */
        status = ND6RSOL_Input(pkt, device, buf_ptr);

#else

        status = 0;

#endif

        /* Return the buffer back to the free buffer pool. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        break;

    default:

        /* ATI_INCOMPLETE - how do I notify the upper layer if I do not
         * know the type of packet - I can't get the source and destination
         * port out of the data portion of the packet to determine the 
         * socket to notify.
         */

        /* RFC 2463 section 2.4.a - If an ICMPv6 error message of unknown type
         * is received, it MUST be passed to the upper layer.
         */

        /* RFC 2463 section 2.4.b - If an ICMPv6 informational message of unknown
         * type is received, it MUST be silently discarded.
         */

        /* Unrecognized Message */
        status = -1;

        NLOG_Error_Log("Unrecognized ICMPv6 packet", NERR_INFORMATIONAL, 
                       __FILE__, __LINE__);

        /* Return the buffer back to the free buffer pool. */
        MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

        /* Increment the number of ICMP messages received with errors. */
        MIB_ipv6IfIcmpInErrors_Inc(device);

        break;
    }

    return (status);

} /* ICMP6_Interpret */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ICMP6_Header_Init
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function will get a free buffer and fill in as much of the   
*       ICMP packet fields as possible.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       type                    The Type of ICMP packet to build.
*       code                    The Code to place in the packet.
*       size                    The size of packet to build.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       Pointer to the buffer in which the packet was constructed or
*       NU_NULL if the packet could not be created.
*
*************************************************************************/
NET_BUFFER *ICMP6_Header_Init(UINT8 type, UINT8 code, UINT32 size)
{
    NET_BUFFER  *buf_ptr;

    buf_ptr = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist, (INT32)size);

    /* Get a free buffer chain. */
    if (buf_ptr != NU_NULL)
    {
        /* Initialize each field in the allocated buffer. */
        buf_ptr->mem_total_data_len = size;

        buf_ptr->mem_dlist = (NET_BUFFER_HEADER *)&MEM_Buffer_Freelist;

        buf_ptr->data_ptr = buf_ptr->mem_parent_packet;

        /* Put the Type in the packet. */
        PUT8(buf_ptr->data_ptr, IP6_ICMP_TYPE_OFFSET, type);

        /* Put the Code in the packet. */
        PUT8(buf_ptr->data_ptr, IP6_ICMP_CODE_OFFSET, code);

        /* Set the data length to the size of the packet */
        buf_ptr->data_len = size;
    }

    return (buf_ptr);

} /* ICMP6_Header_Init */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ICMP6_Random_Delay
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function will generate a random delay value between zero and
*       max_delay.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *source_addr            The source address of the ICMP packet.
*       *dest_addr              The destination address of the ICMP packet.
*       max_delay               The maximum amount of time to delay.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       delay                   The random delay value.          
*
*************************************************************************/
UINT32 ICMP6_Random_Delay(const UINT8 *source_addr, const UINT8 *dest_addr, 
                          UINT32 max_delay)
{
    UINT32  delay;
    UINT32  dest_portion;
    UINT32  source_portion;

    dest_portion = IP_ADDR(&dest_addr[12]);
    source_portion = IP_ADDR(&source_addr[12]);

    delay = NU_Retrieve_Clock();
    delay += dest_portion + source_portion;
    delay = (delay % max_delay) + 1;

    return (delay);

} /* ICMP6_Random_Delay */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ICMP6_RtrSolicitation_Event
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function will build and transmit a Router Solicitation 
*       message to the All-Routers Multicast Address and set an event
*       to transmit another Router Solicitation until the maximum number
*       of unanswered Router Solicitations have been transmitted.
*
*       RFC 4861 section 6.3.7 - To obtain Router Advertisements
*       quickly, a host SHOULD transmit up to MAX_RTR_SOLICITATIONS
*       Router Solicitation messages, each separated by at least
*       RTR_SOLICITATION_INTERVAL seconds.
*                                                                         
*   INPUTS                                                                
*                                         
*       event                       The event being handled.                                
*       dev_index                   The index of the device out which to 
*                                   send the Router Solicitation.
*       max_transmits               The maximum number of Router 
*                                   Solicitations to transmit.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       VOID                      
*
*************************************************************************/
VOID ICMP6_RtrSolicitation_Event(TQ_EVENT event, UNSIGNED dev_index,
                                 UNSIGNED max_transmits)
{
    DV_DEVICE_ENTRY *dev_ptr;
    UINT8           source_address[IP6_ADDR_LEN];
    DEV6_IF_ADDRESS *link_local_addr = NU_NULL;

    UNUSED_PARAMETER(event);

    /* If the maximum number of Router Solicitation messages have not
     * already been sent without a response, send a Router Solicitation.
     */
    if (max_transmits > 0)
    {
        max_transmits--;

        /* Get a pointer to the device */
        dev_ptr = DEV_Get_Dev_By_Index(dev_index);

        if (dev_ptr)
        {
            /* Get the link-local address for the interface */
            if (dev_ptr->dev_flags & DV6_IPV6)
                link_local_addr = IP6_Find_Link_Local_Addr(dev_ptr);

            /* If there is a link-local address that is not in the tentative
             * state, use it as the source address.
             */
            if ( (link_local_addr) &&
                 (!(link_local_addr->dev6_addr_state & DV6_TENTATIVE)) )
                NU_BLOCK_COPY(source_address, link_local_addr->dev6_ip_addr, 
                              IP6_ADDR_LEN);

            /* Otherwise, use the unspecified address. */
            else
                UTL_Zero(source_address, IP6_ADDR_LEN);
   
            /* RFC 4861 section 6.3.7 - A host sends Router Solicitations to
             * the All-Routers multicast address.  The IP source address is
             * set to either one of the interface's unicast addresses or the
             * unspecified address.
             */
            if (ND6RSOL_Output(dev_ptr, IP6_All_Routers_Multi, source_address) == NU_SUCCESS)
            {
#ifdef NET_5_3
                /* Increment the number of Router Solicitation messages
                 * sent on this interface.  This member was added to the 
                 * device structure in NET 5.3.
                 */
                dev_ptr->dev6_rtr_sols ++;
#endif
                if (TQ_Timerset(ICMP6_RtrSol_Event, dev_ptr->dev_index, 
                                IP6_MAX_RTR_SOLICITATION_INTERVAL, 
                                max_transmits) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to set the event to transmit an initial Router Solicitation", 
                                   NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

} /* ICMP6_RtrSolicitation_Event */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ICMP6_Echo_Reply
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function creates and transmits an ICMPv6 Echo Reply in 
*       response to an ICMPv6 Echo Request.
*                                                                         
*   INPUTS                                                                
*                                          
*       *buf_ptr                A pointer to the buffer holding the 
*                               ICMPv6 Echo Request.
*       *pkt                    A pointer to the IPv6 header of the 
*                               ICMPv6 Echo Request.                               
*                                                                         
*   OUTPUTS                                                               
*                                    
*       NU_SUCCESS              The reply was sent.
*       -1                      The request was invalid.
*
*************************************************************************/
STATUS ICMP6_Echo_Reply(NET_BUFFER *buf_ptr, IP6LAYER *pkt)
{
    UINT8           icmpsrc[IP6_ADDR_LEN], icmpdest[IP6_ADDR_LEN];
	UINT8			*src_ptr;
    STATUS          stat = NU_SUCCESS;
    UINT16          checksum;
    IP6_S_OPTIONS   ip6_options;
    RTAB6_ROUTE     dest_route;

#if ( (INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE) )
    ICMP6_LAYER             *icmp_header;
#endif

    /* Set the deallocation list. */
    buf_ptr->mem_dlist = &MEM_Buffer_Freelist;

    /* Make the source of the Echo Reply the destination of the
     * Echo Request. 
     */
    if (!(buf_ptr->mem_flags & NET_MCAST))
        NU_BLOCK_COPY(icmpsrc, pkt->ip6_dest, IP6_ADDR_LEN);

    /* If the destination address is multicast, select a source address
     * to use instead of the multicast address.
     */
    else
    {
		/* Find an address to use as the source address. */
		src_ptr = in6_ifawithifp(buf_ptr->mem_buf_device, pkt->ip6_src);
		
		/* If an address could be found. */
		if (src_ptr)
		{
	        NU_BLOCK_COPY(icmpsrc, src_ptr, IP6_ADDR_LEN);
	
	        /* Clear the multicast flag */
	        buf_ptr->mem_flags &= ~NET_MCAST;
		}
		
		/* There is no valid source address on the node.  Set an error. */
		else
		{
			stat = -1;
		}
    }

	/* If an address was found. */
	if (stat == NU_SUCCESS)
	{
	    /* Zero out the checksum */
	    PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, 0);
	
	    /* Compute the ICMP checksum. */
	    checksum = UTL6_Checksum(buf_ptr, icmpsrc, pkt->ip6_src, 
	                             buf_ptr->mem_total_data_len, IPPROTO_ICMPV6, 
	                             IPPROTO_ICMPV6);
	
	    /* Put the new checksum in the packet */
	    PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, checksum);
	
#if ( (INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE) )
	
	    /* Get a pointer to the ICMPv6 header */
	    icmp_header = (ICMP6_LAYER *)buf_ptr->data_ptr;
	
	    /* Print/store the ICMP header info */
	    NLOG_ICMP6_Info(icmp_header, NLOG_TX_PACK);
	
#endif  
	
	    /* Initialize the ip6_options structure. */
	    memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));
	
	    /* Extract the Traffic Class to Echo */
	    ip6_options.tx_traffic_class = 
	        (UINT8)((GET16(pkt, IP6_XXX_OFFSET) >> 4) & 0x00ff);
	
	    /* Copy the destination address since we are going to overwrite pkt
	     * when we move the ICMPv6 payload of this buffer to the start of
	     * the buffer. 
	     */
	    NU_BLOCK_COPY(icmpdest, pkt->ip6_src, IP6_ADDR_LEN);
	
	    /* Set the destination and source address pointers. */
	    ip6_options.tx_dest_address = icmpdest;
	    ip6_options.tx_source_address = icmpsrc;
	
	    /* Find a route to the destination using the same interface as the
	     * Echo Request was received on.
	     */
	    dest_route.rt_route = 
	        (RTAB6_ROUTE_ENTRY*)(RTAB6_Find_Route_By_Device(icmpdest, 
	                                                        buf_ptr->mem_buf_device));
	
	    if (dest_route.rt_route)
	    {
	        NU_BLOCK_COPY(dest_route.rt_ip_dest.rtab6_rt_ip_dest.sck_addr,
	                      pkt->ip6_src, IP6_ADDR_LEN);
	
	        dest_route.rt_ip_dest.rtab6_rt_ip_dest.sck_family = SK_FAM_IP6;
	        dest_route.rt_ip_dest.rtab6_rt_device = buf_ptr->mem_buf_device;
	    }
	
	    /* Increment the number of ICMP messages sent. */
	    MIB_ipv6IfIcmpOutMsgs_Inc(buf_ptr->mem_buf_device);
	
	    MIB_ipv6IfIcmpOutEchoRep_Inc(buf_ptr->mem_buf_device);
	
	    /* Move the ICMPv6 data into the first byte of the buffer.  This is
	     * necessary for network interface drivers that have a strict
	     * alignment requirement and must have data start at byte 0.
	     */
	    memmove(buf_ptr->mem_parent_packet, buf_ptr->data_ptr, buf_ptr->data_len);
	
	    /* Set the data pointer to point at the first byte of the buffer. */
	    buf_ptr->data_ptr = buf_ptr->mem_parent_packet;
	
	    /* Send this packet. */
	    stat = IP6_Send(buf_ptr, &ip6_options, IPPROTO_ICMPV6, &dest_route, 
	                    IP6_DONTROUTE, NU_NULL);
						
	    /* Free the route */
	    if (dest_route.rt_route)
	        RTAB_Free((ROUTE_ENTRY*)dest_route.rt_route, NU_FAMILY_IP6);						
	}
	
    if (stat != NU_SUCCESS)
    {
        /* Increment the number of send errors. */
        MIB_ipv6IfIcmpOutErrors_Inc(buf_ptr->mem_buf_device);

        NLOG_Error_Log("ICMPv6 Echo Reply not sent", NERR_SEVERE, 
                       __FILE__, __LINE__);

        /* The packet was not sent.  Deallocate the buffer.  If the packet was
         * transmitted it will be deallocated when the transmit complete
         * interrupt occurs. 
         */
        MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
    }

    return (NU_SUCCESS);

} /* ICMP6_Echo_Reply */

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ICMP6_Process_Error
*                                                                       
*   DESCRIPTION                                                           
*
*       This function processes an incoming ICMPv6 error.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *buf_ptr                A pointer to the ICMP error packet.
*                                                                       
*   OUTPUTS                                                               
*                            
*       None                                           
*                                                                       
*************************************************************************/
VOID ICMP6_Process_Error(NET_BUFFER *buf_ptr)
{
    INT16       next_header;
    UINT16      header_len = IP6_HEADER_LEN;
    UINT8 HUGE  *ip6_hdr;
    INT32       data_len;
    UINT16      hdr_len;

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) )
    INT32       error;
    UINT8       *source_addr, *dest_addr;
#endif

    /* Get the error code to process */
    switch (buf_ptr->data_ptr[IP6_ICMP_TYPE_OFFSET])
    {
        case ICMP6_DST_UNREACH:

            MIB_ipv6IfIcmpInDestUreach_Inc(buf_ptr->mem_buf_device);

            /* Determine the code of the Destination Unreachable 
             * message and increment the corresponding MIB statistic.
             */
            switch (buf_ptr->data_ptr[IP6_ICMP_CODE_OFFSET])
            {
                case ICMP6_DST_UNREACH_ADMIN:

                    MIB_ipv6IfIcmpInAdminProh_Inc(buf_ptr->mem_buf_device);
                    break;

                default:

                    break;
            }

            break;

        case ICMP6_TIME_EXCEEDED:

            MIB_ipv6IfIcmpInTimeExcds_Inc(buf_ptr->mem_buf_device);
            break;

        case ICMP6_PARAM_PROB:

            MIB_ipv6IfIcmpInParmProb_Inc(buf_ptr->mem_buf_device);
            break;

        case ICMP6_PACKET_TOO_BIG:

            MIB_ipv6IfIcmpInPktTooBig_Inc(buf_ptr->mem_buf_device);
            break;

        default:
            break;
    }

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) )

    /* Determine the error to report based on the type and code in the 
     * packet.
     */
    error = ICMP6_MAP_ERROR(buf_ptr->data_ptr[IP6_ICMP_TYPE_OFFSET], 
                            buf_ptr->data_ptr[IP6_ICMP_CODE_OFFSET]);
#endif

    /* Set the data pointer to the beginning of the IPv6 header contained
     * in the data portion of the ICMPv6 packet.
     */
    buf_ptr->data_ptr += IP6_ICMP_DATA_OFFSET;

    /* RFC 1981 - section 5.2 - If the original packet contained a 
     * Routing header, the Routing header should be used to determine 
     * the location of the destination address within the original 
     * packet.
     */
    if ( (!(IP6_IS_NXTHDR_RECPROT(GET8(buf_ptr->data_ptr, IP6_NEXTHDR_OFFSET)))) &&
         ((buf_ptr->data_len - IP6_ICMP_DATA_OFFSET - IP6_HEADER_LEN) > 0) )
    {
        ip6_hdr = buf_ptr->data_ptr;

        /* Get the next-header value of the IPv6 header */
        next_header = GET8(ip6_hdr, IP6_NEXTHDR_OFFSET);

        /* Increment the data pointer to point to the Extension Header */
        ip6_hdr += IP6_HEADER_LEN;

        /* Get the number of bytes of data in the original packet after the
         * IPv6 header.
         */
        data_len = buf_ptr->data_len - IP6_ICMP_DATA_OFFSET - IP6_HEADER_LEN;

        while (data_len > 0)
        {
            switch (next_header)
            {            
                case IPPROTO_ROUTING:
                    
                    /* RFC 1981 - section 5.2 - If Segments Left is greater 
                     * than zero, the destination address is the last address 
                     * (Address[n]) in the Routing header.
                     */
                    if (GET8(ip6_hdr, IP6_ROUTING_SEGLEFT_OFFSET) > 0)
                        NU_BLOCK_COPY(&buf_ptr->data_ptr[IP6_DESTADDR_OFFSET],
                                      &ip6_hdr[GET8(ip6_hdr, 
                                                    IP6_ROUTING_SEGLEFT_OFFSET) >> 1],
                                      IP6_ADDR_LEN);

                    /* Get out of the loop */
                    data_len = 0;

                    break;
                                        
                default:

                    /* If there is not another Extension Header in the packet,
                     * get out of the loop.
                     */
                    if (IP6_IS_NXTHDR_RECPROT(next_header))
                        data_len = 0;

                    /* Otherwise, get a pointer to the next header */
                    else
                    {
                        /* Decrement the data length by the number of bytes in this
                         * Extension Header 
                         */
                        hdr_len = (UINT16)
                            (8 + (ip6_hdr[IP6_EXTHDR_LENGTH_OFFSET] << 3));

                        data_len -= hdr_len;

                        /* Increment the data pointer to the next Extension Header */
                        ip6_hdr += hdr_len;

                        /* Get the next header type */
                        next_header = ip6_hdr[IP6_EXTHDR_NEXTHDR_OFFSET];
                    }

                    break;
            }
        }
    }

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) )

    /* Save a pointer to the Source and Destination Addresses of the
     * invoking packet.
     */
    source_addr = (UINT8 *)(&buf_ptr->data_ptr[IP6_SRCADDR_OFFSET]);
    dest_addr = (UINT8 *)(&buf_ptr->data_ptr[IP6_DESTADDR_OFFSET]);

#endif

    /* Strip off all extension headers */
    next_header = UTL6_Strip_Extension_Headers(buf_ptr, &header_len);

    /* If the protocol type is UDP or TCP, process the error */
    switch (next_header)
    {
#if (INCLUDE_UDP == NU_TRUE)
    case IP_UDP_PROT:

        if (UDP_Handle_Datagram_Error(NU_FAMILY_IP6, buf_ptr, source_addr, 
                                      dest_addr, error) != NU_SUCCESS)
            NLOG_Error_Log("Failed to handle the ICMPv6 error on the UDP port", 
                           NERR_RECOVERABLE, __FILE__, __LINE__);

        break;
#endif

#if (INCLUDE_TCP == NU_TRUE)

    case IP_TCP_PROT:

        if (TCP_Handle_Datagram_Error(NU_FAMILY_IP6, buf_ptr, source_addr, 
                                      dest_addr, error) != NU_SUCCESS)
            NLOG_Error_Log("Failed to handle the ICMPv6 error on the TCP port", 
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        break;

#endif

    default:    

        break;
    }

} /* ICMP6_Process_Error */

#if (INCLUDE_IPV4 == NU_TRUE)

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ICMP6_Process_IPv4_Error
*                                                                       
*   DESCRIPTION                                                           
*
*       This function processes an incoming ICMPv4 error destined for
*       an IPv6 node.  When a node encapsulates an IPv6 packet in
*       an IPv4 packet, and an ICMPv4 error is triggered somewhere
*       along the path of the IPv4 packet, an ICMPv4 error message
*       will be transmitted.  The node may translate the ICMPv4 error
*       into an ICMPv6 error and process it as usual.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *buf_ptr                A pointer to the ICMP error packet.
*       *dest_addr              The address to which the packet was
*                               transmitted.
*       type                    The type of ICMPv4 error.
*       code                    The code in the ICMPv4 packet.
*       data                    If this is a Parameter Problem error,
*                               the byte offset in the packet of
*                               the error.
*
*   OUTPUTS                                                               
*                            
*       None                                           
*                                                                       
*************************************************************************/
VOID ICMP6_Process_IPv4_Error(NET_BUFFER *buf_ptr, const UINT8 *dest_addr, 
                              UINT8 type, UINT8 code, UINT32 data)
{
    STATUS      status = NU_SUCCESS;

    /* Check that there is enough data to process the ICMPv6 packet */
    if (buf_ptr->mem_total_data_len >= IP6_HEADER_LEN)
    {
        /* Map the IPv4 type and code to a corresponding IPv6 type and 
         * code. 
         */
        switch (type)
        {
            /* Destination Unreachable */
            case ICMP_UNREACH:

                type = ICMP6_DST_UNREACH;

                switch (code)
                {
                    case ICMP_UNREACH_PORT:

                        code = ICMP6_DST_UNREACH_PORT;
                        break;

                    case ICMP_UNREACH_NET:
                    case ICMP_UNREACH_HOST:
                    case ICMP_UNREACH_SRCFAIL:

                        code = ICMP6_DST_UNREACH_NOROUTE;
                        break;

                    default:
                        status = -1;
                        break;
                }

                break;

            /* Time Exceeded */
            case ICMP_TIMXCEED:

                type = ICMP6_TIME_EXCEEDED;

                switch (code)
                {
                    case ICMP_TIMXCEED_TTL:

                        code = ICMP6_TIME_EXCD_HPLMT;
                        break;

                    case ICMP_TIMXCEED_REASM:

                        code = ICMP6_TIME_EXCD_REASM;
                        break;

                    default:
                    
                        status = -1;
                        break;
                }

                break;

            /* Parameter Problem */
            case ICMP_PARAPROB:

                /* If the problem did not occur within the IPv6 packet,
                 * do not continue processing the error.
                 */
                if (data >= IP_HEADER_LEN)
                {
                    type = ICMP6_PARAM_PROB;
                    code = ICMP6_PARM_PROB_HEADER;

                    /* Set the new value of data to reflect the removal
                     * of the IPv4 header from the packet.
                     */
                    data -= IP_HEADER_LEN;
                }

                else
                    status = -1;

                break;

            default:

                status = -1;
                break;
        }

        /* If the type and code could be mapped to an IPv6 type and code,
         * build the ICMPv6 Header.
         */
        if (status == NU_SUCCESS)
        {
            /* If the error is for this node, handle it */
            if (DEV_Get_Dev_By_Addr(dest_addr))
            {
                /* Move the data pointer to the beginning of the ICMPv6 header */
                buf_ptr->data_ptr -= 8;

                /* Increment the data length by the length of the ICMPv6 header */
                buf_ptr->mem_total_data_len += 8;
                buf_ptr->data_len += 8;

                /* Put the type and code in the packet */
                PUT8(buf_ptr->data_ptr, IP6_ICMP_TYPE_OFFSET, type);
                PUT8(buf_ptr->data_ptr, IP6_ICMP_CODE_OFFSET, code);

                ICMP6_Process_Error(buf_ptr);
            }

            /* Otherwise, send the error to the invoking node */
            else
                ICMP6_Send_Error((IP6LAYER*)buf_ptr->data_ptr, buf_ptr, 
                                 type, code, data, buf_ptr->mem_buf_device);
        }
    }

    /* Otherwise, log an error */
    else
        NLOG_Error_Log("Not enough data in the ICMPv4 packet for IPv6 to process", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

} /* ICMP6_Process_IPv4_Error */

#endif

#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ICMP6_Process_Packet_Too_Big
*                                                                       
*   DESCRIPTION                                                           
*
*       A Packet Too Big message is sent by a router in response to a 
*       packet that it cannot forward because the packet is larger than 
*       the MTU of the outgoing link.  The information in this message is
*       used as part of the Path MTU Discovery Process.
*
*       An incoming Packet Too Big message must be passed to the 
*       upper-layer process.
*
*   PACKET FORMAT
*
*   0                   1                   2
*   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |      Type     |     Code      |            Checksum           |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                              MTU                              |
*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*  |                                                               |
*  +             As much of the invoking packet as will            +
*  |                  fit without the ICMPv6 packet                |
*  +                 exceeding the minimum IPv6 MTU.               +
*  |                                                               |
*  +-+-+-+-+-+-+-+-+-+-
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *buf_ptr                A pointer to the Packet Too Big packet.
*                                                                       
*   OUTPUTS                                                               
*                                                  
*       None                     
*                                                                       
*************************************************************************/
VOID ICMP6_Process_Packet_Too_Big(const NET_BUFFER *buf_ptr)
{
    RTAB6_ROUTE_ENTRY   *rt_entry;
    UINT32              adv_mtu;
    UINT8               *dest_addr;
    STATUS              status;

    /* Get a pointer to the destination address of the original packet. */

    /* If the current buffer doesn't contain the destination address, look in
     * the next buffer.
     */
    if (buf_ptr->data_len < (IP6_PKT_TOO_BIG_DATA + IP6_DESTADDR_OFFSET))
    {
        dest_addr =
            (UINT8 *)(&buf_ptr->next_buffer->data_ptr[IP6_PKT_TOO_BIG_DATA +
                                                      IP6_DESTADDR_OFFSET -
                                                      buf_ptr->data_len]);
    }

    else
    {
        dest_addr =
            (UINT8 *)(&buf_ptr->data_ptr[IP6_PKT_TOO_BIG_DATA + 
                                         IP6_DESTADDR_OFFSET]);
    }

    /* If there is an existing route entry for the Source of the packet, 
     * process the Packet Too Big message.
     */
    rt_entry = RTAB6_Find_Route(dest_addr, RT_BEST_METRIC);

    /* If a route was found and Path MTU Discovery is not disabled on
     * the route.  This function should never get called for a route
     * that has disabled Path MTU Discovery since the packet will be
     * fragmented at the IP layer.
     */
    if ( (rt_entry) && (!(rt_entry->rt_flags & RT_STOP_PMTU)) )
    {
        /* If this is not a host route, add a host route */
        if (rt_entry->rt_route_node->rt_submask_length != 128)
        {
            status = RTAB6_Add_Route(rt_entry->rt_device, dest_addr, 
                                     rt_entry->rt_next_hop.sck_addr, 128, 
                                     rt_entry->rt_flags);

            /* Free the original network route found */
            RTAB_Free((ROUTE_ENTRY*)rt_entry, NU_FAMILY_IP6);

            /* If the route could not be added, log an error and return */
            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Could not add new IPv6 Host Route for PMTU Discovery", 
                               NERR_SEVERE, __FILE__, __LINE__);
                return;
            }

            /* Get a pointer to the new host route just added */
            rt_entry = RTAB6_Find_Route(dest_addr, RT_HOST_MATCH);
        }

        if (rt_entry)
        {
            /* Extract the advertised MTU from the packet */
            adv_mtu = GET32(buf_ptr->data_ptr, IP6_ICMP_MTU_OFFSET);

#if (INCLUDE_IPSEC == NU_TRUE)

            /* Check whether IPsec is enabled for the device which
             * received this packet.
             */
            if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
            {
                /* Adjust for the IPsec header overhead. */
                adv_mtu -= IPSEC_HDRS_OVERHEAD;
            }
#endif

            /* RFC 1981 section 4 - A node may receive a Packet Too Big 
             * message reporting a next-hop MTU that is less than the
             * IPv6 minimum link MTU.  In that case, the node is not
             * required to reduce the size of subsequent packets sent of
             * the path to less than the IPv6 minimum link MTU, but rather
             * must include a Fragment header in those packets.
             */
            if (adv_mtu < IP6_MIN_LINK_MTU)
            {
                /* IPv6 Ready Phase-II Certification requires that 
                 * the fragments be 1272 bytes instead of 1280.
                 */
#if (INCLUDE_IPSEC == NU_TRUE)
                /* Check whether IPsec is enabled for the device which
                 * received this packet.
                 */
                if (!(buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE))
#endif
                {
                    adv_mtu = IP6_MIN_LINK_MTU - 8;
                }

#if (INCLUDE_IPSEC == NU_TRUE)
                else
                {
                    /* IPv6 Ready Phase-II Certification for IPsec requires
                     * fragments to be 1280 bytes with IPsec enabled and 1272
                     * otherwise.
                     */
                    adv_mtu = IP6_MIN_LINK_MTU;
                }
#endif

                rt_entry->rt_flags |= RT_STOP_PMTU;
            }

            /* RFC 1981 - section 5.2 - If the tentative PMTU is less 
             * than the existing PMTU estimate, the tentative PMTU
             * replaces the existing PMTU as the PMTU value for the 
             * path.
             */
            if (adv_mtu < rt_entry->rt_path_mtu)
            {
                rt_entry->rt_path_mtu = adv_mtu;

                /* Set the time when the next increase is due */
                rt_entry->rt_pmtu_timestamp = 
                    NU_Retrieve_Clock() + PMTU_INC_TIME;
            }

            /* Free the route */
            RTAB_Free((ROUTE_ENTRY*)rt_entry, NU_FAMILY_IP6);
        }
    }

} /* ICMP6_Process_Packet_Too_Big */

#endif

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ICMP6_Send_Error                                                  
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Send an ICMPv6 error packet.                                             
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *pkt                    A pointer to the IPv6 header of the 
*                               invoking error packet.
*       *buf_ptr                A pointer to the invoking error packet.
*       type                    The type of error generated.
*       code                    The more specific code based on the type 
*                               of error generated.
*       data                    Additional data to include in the ICMPv6 
*                               packet.  For a Parameter Problem packet, 
*                               this is the octet offset within the 
*                               invoking packet where the error was 
*                               detected.  For a Packet Too Big packet, 
*                               this is the MTU.
*       *device                 A pointer to the device on which the 
*                               invoking error packet was received.
*                                                                       
*   OUTPUTS
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID ICMP6_Send_Error(IP6LAYER *pkt, NET_BUFFER *buf_ptr, UINT8 type, 
                      UINT8 code, UINT32 data, DV_DEVICE_ENTRY *device)
{
    NET_BUFFER                  *send_buf;
    IP6LAYER                    *ip_pkt;
    UINT16                      checksum;
    UINT32                      packet_length;
    DEV6_IF_ADDRESS             *dev_source_addr;
    IP6_S_OPTIONS               ip6_options;
    UINT16                      add_data;
    IP6_NEIGHBOR_CACHE_ENTRY    *nc_entry = NU_NULL;
    ICMP6_LAYER                 *icmp_header;

    ip_pkt = (IP6LAYER *)buf_ptr->data_ptr;

    /* RFC 2463 section 2.4.e - An ICMPv6 error message MUST NOT be 
     * sent as the result of receiving: 
     */

    /* (e.1) an ICMPv6 error message. */
    if (ip_pkt->ip6_next == IP_ICMPV6_PROT)
    {
        icmp_header = (ICMP6_LAYER*)(buf_ptr->data_ptr + IP6_HEADER_LEN);

        if (ICMP6_ERROR_MSG(icmp_header->icmp6_type))
            return;
    }

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

    /* (e.2) a packet destined to an IPv6 multicast address (there are two
     * exceptions to this rule: (1) the Packet Too Big Message and (2) the
     * Parameter Problem Message, Code 2.)
     */
    if (buf_ptr->mem_flags & (NET_MCAST | NET_BCAST))
    {
        if ( (type != ICMP6_PACKET_TOO_BIG) &&
             ((type != ICMP6_PARAM_PROB) || (code != ICMP6_PARM_PROB_OPTION)) )
             return;
    }

#endif

    /* (e.5) a packet whose source address does not uniquely identify a
     * single node -- e.g., the IPv6 Unspecified Address, an IPv6 Multicast
     * Address, or an address known by the ICMP message sender to be an
     * IPv6 anycast address.
     */
    if ( (IPV6_IS_ADDR_UNSPECIFIED(pkt->ip6_src)) || 
         (IPV6_IS_ADDR_MULTICAST(pkt->ip6_src)) )
         return;

    /* RFC 2463 section 2.4.f - In order to limit the bandwidth and forwarding 
     * costs incurred sending ICMPv6 error messages, an IPv6 node MUST limit 
     * the rate of ICMPv6 error messages it sends.
     */
    if ( (!(ICMP6_ERROR_MSG(type))) || 
         ((NU_Retrieve_Clock() - device->dev6_last_error_msg_timestamp) 
          >= ICMP6_ERROR_MSG_RATE_LIMIT) )
    {
        /* If this is a Redirect packet, extra memory must be allocated for
         * the extra size of the redirect.  Also, memory needs to be allocated
         * for the Redirect option.
         */
        if (type == ICMP6_REDIRECT)
        {
            add_data = (IP6_REDIRECT_SIZE + IP6_ICMP_RED_OPTION_DATA_OFFSET);

            /* If a Neighbor Cache entry exists for the Redirect Target,
             * include a Target link-layer address option in the packet.
             */
            nc_entry = device->dev6_fnd_neighcache_entry(device, (UINT8*)data);

            if (nc_entry)
                add_data += IP6_LINK_LAYER_OPT_LENGTH;
        }
        else
            add_data = IP6_ICMP_DATA_OFFSET;

        /* RFC 2463 section 2.4.c - Every ICMPv6 error message (type < 128)
         * includes as much of the offending (invoking) packet (the packet 
         * that caused the error) as will fit without making the error 
         * message packet exceed the minimum IPv6 link MTU.
         */
        if ( (buf_ptr->mem_total_data_len + add_data + IP6_HEADER_LEN) 
             <= IP6_MIN_LINK_MTU)
            packet_length = buf_ptr->mem_total_data_len + add_data;
        else
            packet_length = IP6_MIN_LINK_MTU - IP6_HEADER_LEN;

        /* Initialize the ICMP error packet */
        send_buf = ICMP6_Header_Init(type, code, packet_length);

        if (send_buf != NU_NULL)
        {
            memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));

            /* Fill in any ICMP error specific values */
            switch (type)
            {
            case ICMP6_PARAM_PROB:

                MIB_ipv6IfIcmpOutParmProb_Inc(device);
        
                /* Put the octet offset within the invoking packet in the 
                 * "pointer" field of the Parameter Problem packet.
                 */
                PUT32(send_buf->data_ptr, IP6_PARAM_PROB_PTR, data);
                break;
    
            case ICMP6_PACKET_TOO_BIG:

                MIB_ipv6IfIcmpOutPktTooBig_Inc(device);
            
                /* Put the MTU in the "pointer" field of the Parameter 
                 * Problem packet.
                 */
                PUT32(send_buf->data_ptr, IP6_PKT_TOO_BIG_MTU, data);
                break;

            case ICMP6_REDIRECT:

                MIB_ipv6IfIcmpOutRedirects_Inc(device);

                /* Zero out the reserved field */
                PUT32(send_buf->data_ptr, IP6_ICMP_RESERVED_OFFSET, 0);

                /* Insert the Target Address */
                NU_BLOCK_COPY((CHAR *)(send_buf->data_ptr + 
                              IP6_ICMP_REDIRECT_TRGT_ADDRS_OFFSET),
                              (UINT8*)data, IP6_ADDR_LEN);

                /* Insert the Destination Address */
                NU_BLOCK_COPY((CHAR *)(send_buf->data_ptr + 
                              IP6_ICMP_REDIRECT_DEST_ADDRS_OFFSET),
                              pkt->ip6_dest, IP6_ADDR_LEN);

                send_buf->data_len = 0;

                /* Set up the Target Link-Layer option if necessary */
                if (nc_entry)
                {
                    /* Increment past the Redirect portion of the packet */
                    send_buf->data_ptr += IP6_REDIRECT_SIZE;

                    ND6_Build_Link_Layer_Opt(IP6_ICMP_OPTION_TAR_ADDR, 
                                             nc_entry->ip6_neigh_cache_link_spec, 
                                             send_buf, device->dev_addrlen);

                    /* Restore the data pointer */
                    send_buf->data_ptr -= IP6_REDIRECT_SIZE;

                    send_buf->data_len += IP6_LINK_LAYER_OPT_LENGTH;
                }

                /* Increment past the Redirect and possibly link-layer option 
                 * portion of the packet. 
                 */
                send_buf->data_ptr += (IP6_REDIRECT_SIZE + send_buf->data_len);

                /* Set up the Redirect Option */
                PUT8(send_buf->data_ptr, IP6_ICMP_TYPE_OFFSET, 
                     IP6_ICMP_OPTION_RED_HDR);

                PUT8(send_buf->data_ptr, IP6_ICMP_RED_OPTION_LEN_OFFSET,
                     (UINT8)((packet_length - add_data) >> 3));

                PUT16(send_buf->data_ptr, IP6_ICMP_RED_OPTION_RES1_OFFSET, 0);

                PUT32(send_buf->data_ptr, IP6_ICMP_RED_OPTION_RES2_OFFSET, 0);

                /* Restore the data pointer */
                send_buf->data_ptr -= (IP6_REDIRECT_SIZE + send_buf->data_len);

                /* Set the data length to reflect the Redirect Header and option */
                send_buf->data_len += 
                    (IP6_ICMP_RED_OPTION_DATA_OFFSET + IP6_REDIRECT_SIZE);

                break;

            default:

#if (INCLUDE_IPV6_ICMP_MIB == NU_TRUE)
                if (type == ICMP6_DST_UNREACH)
                {
                    MIB_ipv6IfIcmpOutDstUreach_Inc(device);

                    if (code == ICMP6_DST_UNREACH_ADMIN)
                        MIB_ipv6IfIcmpOutAdminProh_Inc(device);
                }
 
                else if (type == ICMP6_TIME_EXCEEDED)
                    MIB_ipv6IfIcmpOutTimeExcds_Inc(device);
#endif    

                /* Zero out the reserved field */
                PUT32(send_buf->data_ptr, IP6_ICMP_RESERVED_OFFSET, 0);
                break;
            }

            /* Using MEM_Chain_Copy requires that the data length of the
             * first buffer be set to reflect any headers that will exist
             * on the final packet and that the data pointer be set to
             * the beginning of the packet.  After copying the chain,
             * reset the values to their original state.
             */

            /* Reset the data length of the outgoing buffer to the length of
             * the ICMPv6 header, because the MEM_Chain_Copy code will fill
             * in the appropriate data length, and the data length is used as
             * an offset into the data buffer.
             */
            if (type != ICMP6_REDIRECT)
                send_buf->data_len = IP6_ICMP_DATA_OFFSET;

            /* Copy the contents of the invoking buffer into the buffer
             * that will send the error.
             */
            MEM_Chain_Copy(send_buf, buf_ptr, 0, (INT32)(packet_length - add_data));

            /* Determine the Source Address of the ICMPv6 error packet: */

            /* RFC 2463 section 2.2.a - If the message is a response to a 
             * message sent to one of the node's unicast addresses, the
             * Source Address of the reply must be that same address.
             */
            dev_source_addr = DEV6_Find_Target_Address(device, pkt->ip6_dest);

            if (dev_source_addr == NU_NULL)
            {
                if (device->dev_type == DVT_LOOP)
                    ip6_options.tx_source_address = pkt->ip6_dest;

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
                /* ATI_ROUTER_RELEASE1 - anycast processing */
                /* RFC 2463 section 2.2.b - If the message is a response to
                 * a message sent to a multicast or anycast group in which the
                 * node is a member, the Source Address of the reply must be
                 * a unicast address belonging to the interface on which the
                 * multicast or anycast packet was received.
                 */
                else if ( (IPV6_IS_ADDR_MULTICAST(pkt->ip6_dest)) && 
                          (IP6_Lookup_Multi(pkt->ip6_dest, device->dev6_multiaddrs)) )
                    ip6_options.tx_source_address = 
                        in6_ifawithifp(device, pkt->ip6_dest);
    
                /* RFC 2463 section 2.2.c - If the message is a response to a 
                 * message sent to an address that does not belong to the node, 
                 * the Source Address should be that unicast address belonging 
                 * to the node that will be most helpful in diagnosing the error.
                 */
                else
                {
                    /* Get the unicast address best matching the scope of the
                     * passed address
                     */
                    ip6_options.tx_source_address = 
                        in6_ifawithifp(device, pkt->ip6_dest);

                    /* If the address is not found then use the link-local address */
                    if(!ip6_options.tx_source_address)
                    {
                        /* Use the link-local address */
                        dev_source_addr = IP6_Find_Link_Local_Addr(device);
                        
                        ip6_options.tx_source_address = 
                            dev_source_addr->dev6_ip_addr;
                    }                        

                }

               /* ATI_ROUTER_RELEASE1 */
               /* RFC 2463 section 2.2.d - Otherwise, the node's routing table
                * must be examined to determine which interface will be used to
                * transmit the message to its destination, and a unicast address
                * belonging to that interface must be used as the Source Address
                * of the message.
                */
#else
                else
                    ip6_options.tx_source_address = NU_NULL;
#endif
            }
            else
                ip6_options.tx_source_address = dev_source_addr->dev6_ip_addr;

            if (ip6_options.tx_source_address)
            {   
                /* Initialize the ICMP checksum to 0 */
                PUT16(send_buf->data_ptr, IP6_ICMP_CKSUM_OFFSET, 0);
   
                /* Compute the Checksum */
                checksum = UTL6_Checksum(send_buf, ip6_options.tx_source_address, 
                                         pkt->ip6_src, send_buf->mem_total_data_len, 
                                         IPPROTO_ICMPV6, IPPROTO_ICMPV6);

                /* Put the checksum in the packet */
                PUT16(send_buf->data_ptr, IP6_ICMP_CKSUM_OFFSET, checksum);

#if ( (INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE) )

                /* Get a pointer to the ICMPv6 header */
                icmp_header = (ICMP6_LAYER *)buf_ptr->data_ptr;

                /* Print/store the ICMP header info */
                NLOG_ICMP6_Info(icmp_header, NLOG_TX_PACK);

#endif                  
                ip6_options.tx_dest_address = pkt->ip6_src;
                ip6_options.tx_hop_limit = ICMP6_VALID_HOP_LIMIT;

                /* Increment the number of ICMP messages sent. */
                MIB_ipv6IfIcmpOutMsgs_Inc(device);

                /* Transmit the packet */
                if (IP6_Send(send_buf, &ip6_options, IP_ICMPV6_PROT, NU_NULL, 
                             NU_NULL, NU_NULL) != NU_SUCCESS)
                {
                    /* Increment the number of send errors. */
                    MIB_ipv6IfIcmpOutErrors_Inc(device);

                    NLOG_Error_Log("ICMPv6 Error packet not sent", 
                                   NERR_SEVERE, __FILE__, __LINE__);
        
                    /* The packet was not sent.  Deallocate the buffer.  If the packet was
                     * transmitted it will be deallocated when the transmit complete
                     * interrupt occurs. 
                     */
                    MEM_One_Buffer_Chain_Free(send_buf, send_buf->mem_dlist);
                }
    
                /* Otherwise, the packet was successfully transmitted.  Update the
                 * time the last error message was transmitted.
                 */
                else
                    device->dev6_last_error_msg_timestamp = NU_Retrieve_Clock();
            }
            else
            {
                NLOG_Error_Log("ICMPv6 Error packet not sent due to no source address", 
                               NERR_SEVERE, __FILE__, __LINE__);
        
                /* The packet was not sent.  Deallocate the buffer.  If the packet was
                 * transmitted it will be deallocated when the transmit complete
                 * interrupt occurs. 
                 */
                MEM_One_Buffer_Chain_Free(send_buf, send_buf->mem_dlist);
            }
        }
        else
        {
            /* Increment the number of send errors. */
            MIB_ipv6IfIcmpOutErrors_Inc(device);

            NLOG_Error_Log("Cannot build ICMPv6 error message due to no buffers.", 
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

} /* ICMP6_Send_Error */
