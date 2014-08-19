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
*       ip6_re.c                                     
*
*   DESCRIPTION
*
*       This file contains routines for IPv6 packet Reassembly.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IP6_Reassemble_Packet
*       IP6_Reassembly
*       IP6_Reassemble_Event
*       IP6_Free_Queue_Element
*
*   DEPENDENCIES
*
*       nu_net.h
*       ip6_mib.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/ip6_mib.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

extern IP6_QUEUE IP6_Frag_Queue;

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Reassemble_Packet                                                          
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Reassemble an IPv6 packet.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       **packet                A pointer to the IPv6 header of the packet.
*       **buf_ptr               A pointer to the reassembled packet.
*       *header_len             A pointer to the length of the IPv6 header 
*                               until the fragmentation header.
*       *next_header            A pointer to the next header 
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       0                       The packet was successfully reassembled.
*       NU_NULL                 All of the fragments have not been
*                               received.
*                                                                       
*************************************************************************/
STATUS IP6_Reassemble_Packet(IP6LAYER **packet, NET_BUFFER **buf_ptr, 
                             const UINT16 *header_len, UINT8 *next_header)
{
    IP6LAYER            *pkt = *packet;    
    IP6_QUEUE_ELEMENT   *fp;
    NET_BUFFER          *work_buf = *buf_ptr;
    IP6_REASM  HUGE     *ip6_frag;
    UINT32              ip_ident,icmp_offset;
    INT16               icmp_code;
    UINT8               ip6_src[IP6_ADDR_LEN], ip6_dest[IP6_ADDR_LEN];    
    UINT16              unfrag_len;    

    /* Increment the number of IP fragments that have been received. */
    MIB_ipv6IfStatsReasmReqds_Inc(work_buf->mem_buf_device);

    /* Get the Source, Destination and Identification Fields from the 
     * Header 
     */
    NU_BLOCK_COPY(ip6_dest, pkt->ip6_dest, IP6_ADDR_LEN);
    NU_BLOCK_COPY(ip6_src, pkt->ip6_src, IP6_ADDR_LEN);

    unfrag_len = *header_len;

    /* Get the 32-bit fragment Identification value */
    ip_ident = GET32(pkt, (unfrag_len + IP6_FRAGMENT_ID_OFFSET));    

    /* Search the list of fragmented packets to see if at least one 
     * fragment from the same packet was previously received. 
     */
    for (fp = IP6_Frag_Queue.ipq_head; fp != NU_NULL; fp = fp->ipq_next)
    {
        /* Fragments are uniquely identified by IP id, source address
         * and the destination address -RFC 2460 Section 4.5
         */
        if ( (ip_ident == fp->ipq_id) && 
             (memcmp(ip6_src, fp->ipq_source, IP6_ADDR_LEN) == 0) &&
             (memcmp(ip6_dest, fp->ipq_dest, IP6_ADDR_LEN) == 0))
            break;
    }

    /* Instead of allocating memory to the REASM structure we will use 
     * the 16 byte IPv6 source address field of the IPv6 header.
     */
    ip6_frag = (IP6_REASM HUGE *)&pkt->ip6_src;

    /* Fill out the structure with zeros */
    UTL_Zero(ip6_frag, IP6_ADDR_LEN);

    /* Set ipf_mff if more fragments are expected. */   
    if (GET16(pkt, unfrag_len+IP6_FRAGMENT_FRGOFFSET_OFFSET) & IP6_MF)        
        PUT8(ip6_frag, IP6F_MFF_OFFSET, IP6_MF);

    /* Store the fragment offset. Fragment offset is a 13 bit field in the
     * fragment header. So clear the 2 RES bits and the MF bit
     */
    PUT16(ip6_frag, IP6F_FRGOFFSET_OFFSET, 
          (UINT16)((GET16(pkt, unfrag_len + 
                          IP6_FRAGMENT_FRGOFFSET_OFFSET) & (~7))));          

    /* RFC 2460 - section 4.5 - If the length and offset of a fragment are 
     * such that the Payload Length of the packet reassembled from that 
     * fragment would exceed 65,535 octets, then that fragment must be 
     * discarded ...
     */
    if ((UINT32)(GET16(ip6_frag, IP6F_FRGOFFSET_OFFSET) + 
        (GET16(pkt, IP6_PAYLEN_OFFSET) - unfrag_len + IP6_HEADER_LEN - 
        IP6_FRAGMENT_HDR_LENGTH)) > (UINT32)IP6_REASM_MAX_SIZE)     
    {
        icmp_code  = ICMP6_PARM_PROB_HEADER;
        icmp_offset = unfrag_len + IP6_FRAGMENT_FRGOFFSET_OFFSET;

        work_buf->data_ptr -= unfrag_len;

        /* We have used the Source address of the packet to store 
         * information about the fragments. Put the source address back 
         */
        NU_BLOCK_COPY(pkt->ip6_src, ip6_src, IP6_ADDR_LEN);

        ICMP6_Send_Error(pkt, work_buf, ICMP6_PARAM_PROB, (UINT8)icmp_code, 
                         icmp_offset, work_buf->mem_buf_device);        

        /* Increment the number of IP fragmented packets that could not be
         * reassembled. 
         */
        MIB_ipv6IfStatsReasmFails_Inc(work_buf->mem_buf_device);
        
        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
        
        NLOG_Error_Log("Maximum reassembly size exceeded", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
        
        /* If there are other fragments from this chain on the fragment
         * list, free them now.
         */
        if (fp)
            IP6_Free_Queue_Element(fp);
       
        return (NU_NULL);
    }

    /* If the length of the fragment, derived from the fragment packet's
     * payload length field, is not a multiple of 8 octets and M flag of 
     * fragment is set, then the packet must be discarded, and an ICMP 
     * parameter problem, should be sent to the source of the fragment
     * pointing to the payload length field of the fragment RFC 2460
     */
    if ( ((GET16(pkt, IP6_PAYLEN_OFFSET) % 8) != 0) && 
          (GET16(pkt, unfrag_len + IP6_FRAGMENT_FRGOFFSET_OFFSET) & IP6_MF) )
    {
        icmp_code  = ICMP6_PARM_PROB_HEADER;
        icmp_offset = IP6_PAYLEN_OFFSET;

        work_buf->data_ptr -= unfrag_len;

        /* We have used the Source address of the packet to store information
         * about the fragments. Put the source address back 
         */
        NU_BLOCK_COPY(pkt->ip6_src, ip6_src, IP6_ADDR_LEN);

        ICMP6_Send_Error(pkt, work_buf, ICMP6_PARAM_PROB, (UINT8)icmp_code, 
                         icmp_offset, work_buf->mem_buf_device);        

        /* Increment the number of IP fragmented packets that could not be
         * reassembled. 
         */
        MIB_ipv6IfStatsReasmFails_Inc(work_buf->mem_buf_device);
        
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        NLOG_Error_Log("Payload Length not a multiple of 8", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        /* If there are other fragments from this chain on the fragment
         * list, free them now.
         */
        if (fp)
            IP6_Free_Queue_Element(fp);

        return (NU_NULL);
    }

    if ((GET16(pkt, unfrag_len + 
               IP6_FRAGMENT_FRGOFFSET_OFFSET) & ~IP6_MF ) == 0) 
    {   
        /* If a fragment is received with Fragment Offset Zero and M 
         * flag set to zero, the receiving node should assume that the 
         * complete packet have been received - RFC 2460 section 4.5
         */
        if (!(GET16(pkt, unfrag_len + 
                    IP6_FRAGMENT_FRGOFFSET_OFFSET) & IP6_MF))
        {
            *next_header = 
                GET8(pkt, (unfrag_len+IP6_EXTHDR_NEXTHDR_OFFSET));
            
            /* The Unfragmentable Part of the reassembled packet consists 
             * of all headers up to, but not including, the Fragment 
             * header of the first fragment packet (that is, the packet 
             * whose Fragment Offset is zero) - RFC 2460 Section 4.5
             */
            memmove(work_buf->data_ptr, work_buf->data_ptr + 
                    IP6_FRAGMENT_HDR_LENGTH, 
                    (UINT16)(work_buf->data_len - 
                    (unfrag_len + IP6_FRAGMENT_HDR_LENGTH))); 

            /* Set the Pointer back to the beginning of the IPv6 header */
            work_buf->data_ptr -= (UINT32)(unfrag_len);

            /* Decrement the fragment header length from data_len and 
             * mem_total_data_len of the Buffer 
             */
            work_buf->data_len -= IP6_FRAGMENT_HDR_LENGTH;                                    
            work_buf->mem_total_data_len -= IP6_FRAGMENT_HDR_LENGTH;

            PUT8(work_buf->data_ptr, IP6_NEXTHDR_OFFSET, *next_header);

            PUT16(work_buf->data_ptr, IP6_PAYLEN_OFFSET, 
                  (UINT16)(GET16(work_buf->data_ptr, 
                  IP6_PAYLEN_OFFSET) - 8));

            NU_BLOCK_COPY(work_buf->data_ptr + IP6_SRCADDR_OFFSET, 
                          ip6_src, IP6_ADDR_LEN);
            
            return (1);
        }

        /* Store the unfragmented length of the fragment with offset 
         * zero. This value will be restored while freeing the fragments
         */
        PUT16(ip6_frag,IP6F_UNFRAGLEN_OFFSET,unfrag_len);
    }

    /* Adjust the IP length to not reflect any of the extension headers */
    PUT16(ip6_frag, IP6F_PAYLOAD_OFFSET, 
          (UINT16)(GET16(pkt, IP6_PAYLEN_OFFSET) - unfrag_len + 
          IP6_HEADER_LEN - 8));

    /* If this datagram is marked as having more fragments or this is not 
     * the first fragment, attempt reassembly. 
     */
    if (GET16(ip6_frag, IP6F_FRGOFFSET_OFFSET) || 
        (GET16(pkt, unfrag_len + IP6_FRAGMENT_FRGOFFSET_OFFSET) & IP6_MF) )
    {
        *buf_ptr = IP6_Reassembly(ip6_frag, pkt, fp, *buf_ptr, unfrag_len, 
                                  next_header, ip6_src);

        if (*buf_ptr == NU_NULL)
        {
            /* A complete packet could not yet be assembled, return. */
            return (NU_NULL);
        }

        /* If we make it here then a fragmented packet has been put back 
         * together. We need to set all pointers and other local variables 
         * to match what they would normally be if this packet was simply 
         * a RX packet and not a reassembled one. 
         */

        /* Set the IP packet pointer to the IP header. */
        pkt = (IP6LAYER *)(*buf_ptr)->data_ptr;

        /* Put the Src Address back in to the packet */
        NU_BLOCK_COPY(pkt->ip6_src, ip6_src, IP6_ADDR_LEN);

        *packet = pkt;        

        /* Increment the number of IP fragmented packets that have
         * successfully been reassembled. 
         */
        MIB_ipv6IfStatsReasmOKs_Inc(work_buf->mem_buf_device);
    }

    else if (fp)
    {
        IP6_Free_Queue_Element(fp);
        
        /* Drop this packet. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        return (NU_NULL);
    }
    
    return (1);

} /* IP6_Reassemble_Packet */

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Reassembly                                                    
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Reassemble IP fragments into a complete datagram.                
*                                                                       
*   INPUTS                                                                
*                                              
*       *ip6_frag               Pointer to the Reassembly structure                         
*       *ip_pkt                 Pointer to an IP fragment                            
*       *fp                     Pointer to fragment reassembly header                           
*       *buf_ptr                Pointer to the buffer that holds the 
*                               current frag  
*       unfrag_len              Length of the IPv6 header including
*                               the length of the extension headers  
*       *next_header            Pointer to the next header
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_NULL                 All fragments have not yet been received.            
*       NET_BUFFER*             A complete datagram has been built, 
*                               buf_ptr is the head of the chain that 
*                               contains the complete datagram.                                          
*                                                                       
*************************************************************************/
NET_BUFFER *IP6_Reassembly(IP6_REASM HUGE *ip6_frag, IP6LAYER *ip_pkt, 
                           IP6_QUEUE_ELEMENT *fp, NET_BUFFER *buf_ptr, 
                           UINT16 unfrag_len, UINT8 *next_header, 
                           const UINT8 *ip6_src)
{    
    IP6_REASM  HUGE *q, HUGE *p = NU_NULL, HUGE *prev, HUGE *temp_q;
    INT         i;
    INT32       total_length;
    INT         next;
    NET_BUFFER  *r_buf, *m;
    INT         old_int_level;
    UINT16      ip_pkt_ipf_offset, ip_pkt_tlen_offset;
    UINT8       target_hdr = IPPROTO_FRAGMENT;
    UINT16      work_len = IP6_HEADER_LEN;

    /* Since reassembly involves only the data of each fragment, exclude 
     * the IPv6 Fragment header from each fragment. 
     */
    buf_ptr->data_ptr += IP6_FRAGMENT_HDR_LENGTH;
    buf_ptr->data_len  -= (UINT32)(unfrag_len + IP6_FRAGMENT_HDR_LENGTH);

    buf_ptr->mem_total_data_len -= 
        (UINT32)(unfrag_len + IP6_FRAGMENT_HDR_LENGTH);

    /* If this is the first fragment to arrive, create a reassembly 
     * queue. 
     */
    if (fp == NU_NULL)
    {
        if (NU_Allocate_Memory(MEM_Cached, (VOID**)&fp,
                               sizeof(IP6_QUEUE_ELEMENT),
                               (UNSIGNED)NU_NO_SUSPEND) != NU_SUCCESS)
        {
            /* Log an error and drop the packet. */
            NLOG_Error_Log("Unable to alloc memory for IP queue", 
                           NERR_SEVERE, __FILE__, __LINE__);

            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (NU_NULL);
        }

        memset(fp, 0, sizeof(IP6_QUEUE_ELEMENT));

        /* Insert this fragment reassembly header into the list. */
        DLL_Enqueue(&IP6_Frag_Queue, fp);

        /* Save off the fragment Identification field */
        fp->ipq_id = 
            GET32(ip_pkt, (unfrag_len + IP6_FRAGMENT_ID_OFFSET)); 
        
        /* The source and destination ip address fields are used to link 
         * the fragments together. The next two lines copy the source 
         * and destination address 
         */
        NU_BLOCK_COPY(fp->ipq_dest, ip_pkt->ip6_dest, IP6_ADDR_LEN);
        NU_BLOCK_COPY(fp->ipq_source, ip6_src, IP6_ADDR_LEN);
    
        /* Insert this fragment into the list. */
        fp->ipq_first_frag = ip6_frag;

        /* Store the address of the next fragment */
        PUT32(ip6_frag, IP6F_NEXT_OFFSET, NU_NULL);

        /* Store the offset of the IPv6 header from the start 
         * of the memory buffer.
         */
        PUT8(ip6_frag, IP6F_BUF_OFFSET, 
             (UINT8)((CHAR HUGE *)ip_pkt - (CHAR HUGE *)buf_ptr));

        PUT32(ip6_frag, IP6F_HDR_OFFSET, (UINT32)(CHAR HUGE *)ip_pkt);

        /* Set up a timer event to drop this fragment and any others 
         * received that are part of the same datagram if the complete 
         * datagram is not received. 
         */
        if (TQ_Timerset(EV_IP6_REASSEMBLY, (UNSIGNED)fp, 
                        (IP_FRAG_TTL), 0) != NU_SUCCESS)
            NLOG_Error_Log("Failed to set timer to timeout reassembly of packet", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    else
    {
        /* Store the offset of the IPv6 header from the start of the 
         * memory buffer. 
         */
        PUT8(ip6_frag, IP6F_BUF_OFFSET, 
             (UINT8)((CHAR HUGE *)ip_pkt - (CHAR HUGE *)buf_ptr));

        /* Store the beginning of the IPv6 header */
        PUT32(ip6_frag, IP6F_HDR_OFFSET, (UINT32)(CHAR HUGE *)ip_pkt);

        /* Save off this value to avoid a GET32 in the loop */
        ip_pkt_ipf_offset = GET16(ip6_frag, IP6F_FRGOFFSET_OFFSET);
        ip_pkt_tlen_offset = GET16(ip6_frag, IP6F_PAYLOAD_OFFSET);

        /* Find a fragment which begins after this one.*/
        for (q = fp->ipq_first_frag; q != NU_NULL; 
             q = (IP6_REASM HUGE *)GET32(q, IP6F_NEXT_OFFSET))
        {                 
            if (GET16(q, IP6F_FRGOFFSET_OFFSET) > ip_pkt_ipf_offset)
                break;

            /* Keep a pointer to the fragment before q is set to next. */
            p = q;
        }

        /* If q is equal to NULL then the packet just RX does not belong 
         * before any of the ones already in the queue. Since it will be 
         * appended to the end of the queue, we need to check for 
         * overlapping of the last fragment in the queue. If q is not null 
         * that means that there is a preceding fragment, it may provide 
         * some of our data already.  If so, drop the data from the 
         * incoming fragment.  If it provides all of the data, drop the 
         * current fragment. 
         */
                 
        if (p != NU_NULL)
        {
            /* If there is a preceding fragment, it may provide some of 
             * our data already.  If so, drop the data from the incoming 
             * fragment.  If it provides all of the data, drop the current 
             * fragment. 
             */
            prev = p;

            i = GET16(prev, IP6F_FRGOFFSET_OFFSET) + 
                GET16(prev, IP6F_PAYLOAD_OFFSET) - ip_pkt_ipf_offset;

            if (i > 0)
            {
                if ((UINT16)i >= ip_pkt_tlen_offset)
                {
                    /* All of the received data is contained in the 
                     * previous fragment. Drop this one. 
                     */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List, 
                                          &MEM_Buffer_Freelist);

                    return (NU_NULL);
                }
            
                /* Trim the duplicate data from this fragment. */
                MEM_Trim(buf_ptr, (INT32)i);

                PUT16(ip6_frag, IP6F_FRGOFFSET_OFFSET, 
                      (UINT16)(ip_pkt_ipf_offset + i));

                PUT16(ip6_frag, IP6F_PAYLOAD_OFFSET, 
                      (UINT16)(ip_pkt_tlen_offset - i));

                /* Save the values off in locals */
                ip_pkt_ipf_offset = GET16(ip6_frag, IP6F_FRGOFFSET_OFFSET);
                ip_pkt_tlen_offset = GET16(ip6_frag, IP6F_PAYLOAD_OFFSET);
            }        
        
	        /* While we overlap succeeding fragments trim them or, if they are
	         * completely covered, dequeue them. 
	         */
	        while ( (q != NU_NULL) &&
	                ((ip_pkt_ipf_offset + ip_pkt_tlen_offset) > 
	                GET16(q, IP6F_FRGOFFSET_OFFSET)) )
	        {
	            i = (ip_pkt_ipf_offset + ip_pkt_tlen_offset) - 
	                GET16(q, IP6F_FRGOFFSET_OFFSET);
	
	            if ((UINT16)i < GET16(q, IP6F_PAYLOAD_OFFSET))
	            {
	                /* Trim the duplicate data from the start of the previous
	                 * fragment. In this case q. 
	                 */             
	                r_buf = 
	                    (NET_BUFFER *)(GET32(q,IP6F_HDR_OFFSET) - 
	                    GET8(q, IP6F_BUF_OFFSET));
	
	                MEM_Trim(r_buf, (INT32)i);
	
	                PUT16(q, IP6F_PAYLOAD_OFFSET, 
	                      (UINT16)(GET16(q, IP6F_PAYLOAD_OFFSET) - i));
	
	                PUT16(q, IP6F_FRGOFFSET_OFFSET, 
	                      (UINT16)(GET16(q, IP6F_FRGOFFSET_OFFSET) + i));
	
	                break;
	            }
	
	            /* Move on to the next fragment so we can check to see if any/all of 
	             * it is overlapping.  Store this in a temp pointer before the 
	             * buffer is deallocated.
	             */
	            temp_q = (IP6_REASM HUGE *)GET32(q, IP6F_NEXT_OFFSET);
	
	            /* The buffer was completely covered. Deallocate it*/
	            r_buf = 
	                (NET_BUFFER *)(GET32(q,IP6F_HDR_OFFSET) - 
	                GET8(q, IP6F_BUF_OFFSET));
	
	            IP6_Remove_Frag(q, p, fp);
	
	            MEM_One_Buffer_Chain_Free(r_buf, &MEM_Buffer_Freelist);
	            
	             /* Set q to the next buffer in the chain */
	            q = temp_q;
	        }
		}

        /* Insert the new fragment in its place. The new fragment should 
         * be inserted into the list of fragments immediately in front 
         * of q. 
         */
        IP6_Insert_Frag(ip6_frag, fp);
    }

    /* Remove this buffer from the buffer list. */
    MEM_Buffer_Dequeue(&MEM_Buffer_List);

    buf_ptr->next = NU_NULL;

    /* Check for complete reassembly. */
    next = 0;

    for (q = fp->ipq_first_frag; q != NU_NULL;
         q = (IP6_REASM HUGE *)GET32(q, IP6F_NEXT_OFFSET))
    {
        /* Does the fragment contain the next offset. */
        if (GET16(q, IP6F_FRGOFFSET_OFFSET) != (UINT16) next)
            return (NU_NULL);

        /* Update next to the offset of the next fragment. */
        next += GET16(q, IP6F_PAYLOAD_OFFSET);

        /* Keep track of the last fragment checked. It is used below. */
        p = q;
    }

    /* If the more fragments flag is set in the last fragment, then we
       are expecting more. */
    if ( (!p) || (GET8(p, IP6F_MFF_OFFSET) & 1) )
        return (NU_NULL);

    /* Clear the fragment timeout event for this datagram. */
    TQ_Timerunset(EV_IP6_REASSEMBLY, TQ_CLEAR_EXACT, (UNSIGNED)fp, 0);

    PUT32(p, IP6F_NEXT_OFFSET, NU_NULL);

    /* Reassembly is complete. Concatenate the fragments. */

    /* Get the first frag. */
    q = fp->ipq_first_frag;

    /* Point the return buffer to the memory buffer of the first frag. */
    r_buf = 
        (NET_BUFFER *)(GET32(q, IP6F_HDR_OFFSET) - GET8(q, IP6F_BUF_OFFSET)); 

    /* Point to the next fragment in the list. This is done by getting 
     * the next frag and then backing up the pointer so that it points 
     * to the memory buffer for that IP fragment. 
     */
    q = (IP6_REASM HUGE *)GET32(q, IP6F_NEXT_OFFSET);

    /* Loop through the rest concatenating them all together. */
    while (q != NU_NULL)
    {
        /* Back the pointer up so that we are pointing at the buffer 
         * header. 
         */
        m = (NET_BUFFER *)(GET32(q, IP6F_HDR_OFFSET) - 
            GET8(q, IP6F_BUF_OFFSET)); 

        /* Concatenate the two fragments. */
        MEM_Cat(r_buf, m);

        /* Move to the next fragment in preparation for the next loop. */
        q = (IP6_REASM HUGE *)GET32(q, IP6F_NEXT_OFFSET);
    }

    /* Create the header for the new IP packet by modifying the header of 
     * the first packet. 
     */

    /* Get a pointer to the IP header. */
    ip_pkt = (IP6LAYER *)GET32(fp->ipq_first_frag, IP6F_HDR_OFFSET);     

    /* Set the total length. */
    PUT16(ip_pkt, IP6_PAYLEN_OFFSET, 
          ((UINT16)(next + unfrag_len - IP6_HEADER_LEN)));

    /* The Next Header field of the last header of the Unfragmentable
     * Part is obtained from the Next Header field of the first
     * fragment's Fragment header -RFC 2460 section 4.5
     */ 
    *next_header = GET8(ip_pkt,(unfrag_len + IP6_EXTHDR_NEXTHDR_OFFSET));

    if (GET8(ip_pkt, IP6_NEXTHDR_OFFSET) != target_hdr)
    {
        while (GET8(ip_pkt, 
                    work_len + IP6_EXTHDR_NEXTHDR_OFFSET) != target_hdr)
            work_len = 
                (UINT16)(work_len + (IP6_FRAGMENT_HDR_LENGTH + 
                        (GET8(ip_pkt, work_len + 
                        IP6_EXTHDR_LENGTH_OFFSET) << 3)));
    } 
    else
        work_len = 0;

    if (work_len == 0)
        PUT8(ip_pkt, (work_len + IP6_NEXTHDR_OFFSET), *next_header);

    else        
        PUT8(ip_pkt, (work_len + IP6_EXTHDR_NEXTHDR_OFFSET), 
             *next_header);

    /* Remove this fragment reassembly header from the list. */
    DLL_Remove(&IP6_Frag_Queue, fp);

    /* Deallocate this fragment reassembly header. */
    if (NU_Deallocate_Memory(fp) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for fragment structure", 
                       NERR_SEVERE, __FILE__, __LINE__);

    /* The Unfragmentable Part of the reassembled packet consists of all
     * headers up to, but not including, the Fragment header of the first
     * fragment packet (that is, the packet whose Fragment Offset is
     * zero) - RFC 2460 Section 4.5
     */
    r_buf->data_ptr -= (UINT32)(unfrag_len + IP6_FRAGMENT_HDR_LENGTH);  
      
    memmove(&r_buf->data_ptr[unfrag_len], 
            &r_buf->data_ptr[unfrag_len] + IP6_FRAGMENT_HDR_LENGTH, 
            (UINT16)r_buf->data_len); 

    r_buf->data_len += (UINT32)unfrag_len;  
  
    /* Compute and set the total length of the reassembled packet. */
    for (m = r_buf, total_length = 0; m; m = m->next_buffer)
        total_length += (INT32)m->data_len;

    /* Store the total length in the buffer header. */
    r_buf->mem_total_data_len = (UINT32)total_length;

    /* Before passing the reassembled packet to the IP interpreter,
     * we must put this packet onto the head of the received buffer list.
     * This is done because all the upper layers assume that the packet
     * they are processing is the first one on the list. Turn off
     * interrupts while we are modifying the buffer list. 
     */
    old_int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Point this one to the head of the list. */
    r_buf->next = MEM_Buffer_List.head;

    /* If there is nothing on the list, then point the tail to this one.
     * Otherwise, the tail does not need to be touched. 
     */
    if (MEM_Buffer_List.tail == NU_NULL)
        MEM_Buffer_List.tail = r_buf;

    /* Point the head to this one. */
    MEM_Buffer_List.head = r_buf;

    /* We are done. Turn interrupts back on. */
    NU_Local_Control_Interrupts(old_int_level);

    return (r_buf);

} /* IP6_Reassembly */

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Reassembly_Event                                              
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Reassembled the packets once a particular event occurs.          
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *fp                     Pointer to the IPv6 element queue                                                              
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       None.                                                            
*                                                                       
*************************************************************************/
VOID IP6_Reassembly_Event(IP6_QUEUE_ELEMENT *fp)
{
    NET_BUFFER      *buf_head;
    IP6_REASM  HUGE *ip6_frag, ip6_frag_tmp[16];
    UINT16          hlen;
    IP6LAYER        *ip_pkt;
    UINT32          next_temp;

    /* Check that the pointer is not NULL */
    if (fp)
    {
        /* Get the first frag. */
        ip6_frag = fp->ipq_first_frag;

        /* Point the head buffer to the memory buffer of the first 
         * frag. 
         */
        buf_head = 
            (NET_BUFFER*)(GET32(ip6_frag,IP6F_HDR_OFFSET) - 
                          GET8(ip6_frag, IP6F_BUF_OFFSET));

        /* Increment the number of IP fragmented packets that could 
         * not be reassembled. 
         */
        MIB_ipv6IfStatsReasmFails_Inc(buf_head->mem_buf_device);

        if (GET16(ip6_frag, IP6F_FRGOFFSET_OFFSET) == 0)
        {
            /* Get length of IPv6 header including extension headers*/
            hlen = GET16(ip6_frag, IP6F_UNFRAGLEN_OFFSET);

            /* Move data pointer from data to IP header */
            buf_head->data_ptr -= (hlen + IP6_FRAGMENT_HDR_LENGTH);    
            buf_head->data_len += (hlen + IP6_FRAGMENT_HDR_LENGTH);

            buf_head->mem_total_data_len += 
                (hlen + IP6_FRAGMENT_HDR_LENGTH);

            ip_pkt = (IP6LAYER *)buf_head->data_ptr;            

            /* Save the next and prev fields */
            next_temp = GET32(ip6_frag, IP6F_NEXT_OFFSET);

            memset(ip6_frag_tmp, 0, IP6_ADDR_LEN);

            /* Store the information in this temporary block of 
             * memory as we are going to need it after we send the
             * ICMP6_TIME_EXCEEDED error
             */
            NU_BLOCK_COPY(ip6_frag_tmp, ip6_frag, IP6_ADDR_LEN);

            NU_BLOCK_COPY(ip_pkt->ip6_src, fp->ipq_source, IP6_ADDR_LEN);

            ICMP6_Send_Error(ip_pkt, buf_head, ICMP6_TIME_EXCEEDED, 
                             ICMP6_TIME_EXCD_REASM, 0, 
                             buf_head->mem_buf_device); 

            ip6_frag = (IP6_REASM HUGE *)&ip_pkt->ip6_src;

            NU_BLOCK_COPY(ip6_frag, ip6_frag_tmp, IP6_ADDR_LEN);

            /* Restore the next and prev fields */
            PUT32(ip6_frag, IP6F_NEXT_OFFSET, next_temp);

            /* Move the data pointer back */
            buf_head->data_ptr += (hlen + IP6_FRAGMENT_HDR_LENGTH);
            buf_head->data_len -= (hlen + IP6_FRAGMENT_HDR_LENGTH);

            buf_head->mem_total_data_len -= 
                (hlen + IP6_FRAGMENT_HDR_LENGTH);
        }

        /* Free the buffers on the queue and the queue element */
        IP6_Free_Queue_Element(fp);
    }

} /* IP6_Reassembly_Event */

/************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Free_Queue_Element                                            
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Free a fragment reassembly header and any attached fragments.    
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *fp                     Pointer to the element to free                       
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       None.                                                            
*                                                                       
*************************************************************************/
VOID IP6_Free_Queue_Element(IP6_QUEUE_ELEMENT *fp)
{
    IP6_REASM  HUGE *q, *temp_q;
    NET_BUFFER      *buf_ptr;

    /* Remove this fragment reassembly header from the list. */
    DLL_Remove(&IP6_Frag_Queue, fp);

    /* Stop the timer that is running to timeout reassembly of 
     * this packet.
     */
    TQ_Timerunset(EV_IP6_REASSEMBLY, TQ_CLEAR_EXACT, (UNSIGNED)fp, 0);

    /* Get a pointer to the first fragment. */
    q = fp->ipq_first_frag;

    /* While there are fragments in the list. */
    while (q != NU_NULL)
    {
        /* Get a pointer to the buffer that contains this packet. */
        buf_ptr = 
            (NET_BUFFER *)(GET32(q, IP6F_HDR_OFFSET) - 
            GET8(q, IP6F_BUF_OFFSET));
        
        /* Save the next pointer */
        temp_q = (IP6_REASM HUGE *)GET32(q, IP6F_NEXT_OFFSET);

        /* Deallocate the buffer. */
        MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

        /* Restore the next pointer */
        q = temp_q;
    }

    /* Deallocate the memory. */
    if (NU_Deallocate_Memory(fp) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for fragment structure", 
                       NERR_SEVERE, __FILE__, __LINE__);

} /* IP6_Free_Queue_Element */
