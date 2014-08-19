/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
* FILE NAME
*
*       ip_re.c
*
* DESCRIPTION
*
*       This file contains routines for IP packet reassembly.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IP_Reassemble_Packet
*       IP_Reassembly
*       IP_Reassembly_Event
*       IP_Free_Queue_Element
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

extern IP_QUEUE IP_Frag_Queue;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for the Reassembly queues */
IP_QUEUE_ELEMENT NET_Reassembly_Memory[NET_MAX_REASSEMBLY_QUEUES];

/* Declare memory flags for the memory declared above */
UINT8            NET_Reassembly_Memory_Flags[NET_MAX_REASSEMBLY_QUEUES] = {0};
#endif

/***********************************************************************
*
*   FUNCTION
*
*       IP_Reassemble_Packet
*
*   DESCRIPTION
*
*       Reassemble a packet.
*
*   INPUTS
*
*       **packet                A pointer to the IP header of the packet.
*       **buf_ptr               A pointer to the reassembled packet.
*       hlen                    The length of the IP header.
*
*   OUTPUTS
*
*       0                       The packet was successfully reassembled.
*       -1                      All of the fragments have not been
*                               received.
*
*************************************************************************/
STATUS IP_Reassemble_Packet(IPLAYER **packet, NET_BUFFER **buf_ptr,
                            UINT16 hlen)
{
    IPLAYER             *pkt = *packet;
    IP_QUEUE_ELEMENT    *fp;
    UINT16              ip_ident;
    UINT32              ip_src, ip_dest;
    UINT8               ip_protocol;

    /* Increment the number of IP fragments that have been received. */
    MIB2_ipReasmReqds_Inc;

    ip_ident = GET16(pkt, IP_IDENT_OFFSET);
    ip_src = GET32(pkt, IP_SRC_OFFSET);
    ip_dest = GET32(pkt, IP_DEST_OFFSET);
    ip_protocol = GET8(pkt, IP_PROTOCOL_OFFSET);

    /* Search the list of fragmented packets to see if at least one
     * fragment from the same packet was previously received.
     */
    for (fp = IP_Frag_Queue.ipq_head; fp != NU_NULL; fp = fp->ipq_next)
    {
        /* Fragments are uniquely identified by IP id, source address,
         * destination address, and protocol.
         */
        if ( (ip_ident == fp->ipq_id) &&
             (ip_src == fp->ipq_source) &&
             (ip_dest == fp->ipq_dest) &&
             (ip_protocol == fp->ipq_protocol) )
            break;
    }

    /* Set ipf_mff if more fragments are expected. */
    PUT8(pkt, IPF_MFF_OFFSET, (UINT8)(GET8(pkt, IPF_MFF_OFFSET) & ~1));

    if (GET16(pkt, IP_FRAGS_OFFSET) & IP_MF)
        PUT8(pkt, IPF_MFF_OFFSET, (UINT8)(GET8(pkt, IPF_MFF_OFFSET) | 1));

    /* Convert the offset of this fragment to bytes and shift off the 3
     * flag bits.
     */
    PUT16(pkt, IP_FRAGS_OFFSET, (UINT16)(GET16(pkt, IP_FRAGS_OFFSET) << 3) );

    /* If the fragment offset plus the length of the data in this packet
     * is greater than the maximum sized packet can be assembled for this
     * interface, drop the packet, log an error, and free any fragments
     * already on the list of fragmented packets associated with this
     * fragment.
     */
    if ( (UINT32)(GET16(pkt, IP_FRAGS_OFFSET) + GET16(pkt, IP_TLEN_OFFSET)) >
         ((*buf_ptr)->mem_buf_device->dev_reasm_max_size) )
    {
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        NLOG_Error_Log("Maximum reassembly size exceeded", NERR_INFORMATIONAL,
                  __FILE__, __LINE__);

        /* If there are other fragments from this chain on the fragment
         * list, free them now.
         */
        if (fp)
            IP_Free_Queue_Element(fp);

        /* Increment the number of IP fragmented packets that could not be
         * reassembled.
         */
        MIB2_ipReasmFails_Inc;

        return (1);
    }

    /* Adjust the IP length to not reflect the header. */
    PUT16(pkt, IP_TLEN_OFFSET, (UINT16)(GET16(pkt, IP_TLEN_OFFSET) - hlen));

    /* If this datagram is marked as having more fragments or this is not
     * the first fragment, attempt reassembly.
     */
    if (GET8(pkt, IPF_MFF_OFFSET) & 1 || GET16(pkt, IP_FRAGS_OFFSET) )
    {
        *buf_ptr = IP_Reassembly((IP_FRAG *)pkt, fp, *buf_ptr);

        if (*buf_ptr == NU_NULL)
        {
            /* A complete packet could not yet be assembled, return. */
            return (1);
        }

        /* If we make it here then a fragmented packet has been put back
         * together. We need to set all pointers and other local variables
         * to match what they would normally be if this packet was simply
         * a RX packet and not a reassembled one.
         */

        /* Set the IP packet pointer to the IP header. */
        pkt = (IPLAYER *)(*buf_ptr)->data_ptr;

        /* Get the header length. */
        hlen = (UINT16)((GET8(pkt, IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2);

        /* Adjust the IP length to reflect the header. */
        PUT16(pkt, IP_TLEN_OFFSET, (UINT16)(GET16(pkt, IP_TLEN_OFFSET) + hlen));

        *packet = pkt;

        /* Increment the number of IP fragmented packets that have
         * successfully been reassembled.
         */
        MIB2_ipReasmOKs_Inc;
    }

    else if (fp)
    {
        IP_Free_Queue_Element(fp);

        /* Drop this packet. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        MIB2_ipInHdrErrors_Inc;

        return (1);
    }

    return (0);

} /* IP_Reassemble_Packet */

/***********************************************************************
*
*   FUNCTION
*
*       IP_Reassembly
*
*   DESCRIPTION
*
*       Reassemble IP fragments into a complete datagram.
*
*   INPUTS
*
*       *ip_pkt                 Pointer to an IP fragment
*       *fp                     Pointer to fragment reassembly header
*       *buf_ptr                Pointer to the buffer that holds the
*                               current frag
*
*   OUTPUTS
*
*       NU_NULL                 All fragments have not yet been received.
*       NET_BUFFER*             A complete datagram has been built,
*                               buf_ptr is the head of the chain that
*                               contains the complete datagram.
*
*************************************************************************/
NET_BUFFER *IP_Reassembly(IP_FRAG *ip_pkt, IP_QUEUE_ELEMENT *fp,
                          NET_BUFFER *buf_ptr)
{
    INT         hlen;
    IP_FRAG     *q, *temp_q, *p = NU_NULL, *prev;
    INT         i;
    INT32       total_length;
    INT         next;
    NET_BUFFER  *r_buf, *m;
    INT         old_int_level;
    UINT16      ip_pkt_ipf_offset, ip_pkt_tlen_offset;

    hlen = (GET8(ip_pkt, IPF_HL_OFFSET) & 0x0F) << 2;

    /* Since reassembly involves only the data of each fragment, exclude
     * the IP header from each fragment.
     */
    buf_ptr->data_ptr           += hlen;
    buf_ptr->data_len           -= (UINT32)hlen;
    buf_ptr->mem_total_data_len -= (UINT32)hlen;

    /* If this is the first fragment to arrive, create a reassembly queue. */
    if (fp == NU_NULL)
    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        if (NU_Allocate_Memory(MEM_Cached, (VOID**)&fp,
                               sizeof(IP_QUEUE_ELEMENT),
                               (UNSIGNED)NU_NO_SUSPEND) != NU_SUCCESS)
        {
            /* Log an error and drop the packet. */
            NLOG_Error_Log("Unable to alloc memory for IP queue", NERR_SEVERE,
                      __FILE__, __LINE__);

            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);

            return (NU_NULL);
        }
#else
        /* Traverse the flag array to find free memory location */
        for (i=0; (NET_Reassembly_Memory_Flags[i] != NU_FALSE) &&
             (i !=NET_MAX_REASSEMBLY_QUEUES); i++)
            ;
        if (i == NET_MAX_REASSEMBLY_QUEUES)
        {
            /* Log an error and drop the packet. */
            NLOG_Error_Log("Unable to alloc memory for IP queue", NERR_SEVERE,
                           __FILE__, __LINE__);

            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);

            return (NU_NULL);
        }

        /* Assign memory to the queue element */
        fp = &NET_Reassembly_Memory[i];

        /* Turn the memory flag on */
        NET_Reassembly_Memory_Flags[i] = NU_TRUE;

#endif
        /* Insert this fragment reassembly header into the list. */
        DLL_Enqueue(&IP_Frag_Queue, fp);

        /* Save off the protocol. */
        fp->ipq_protocol = GET8(ip_pkt, IP_PROTOCOL_OFFSET);
        fp->ipq_id = GET16(ip_pkt, IPF_ID_OFFSET);

        /* The source and destination ip address fields are used to link
         * the fragments together. The next two lines copy the source and
         * destination address before they are overwritten with pointer
         * values.
         */
        fp->ipq_source = GET32(ip_pkt, IP_SRC_OFFSET);
        fp->ipq_dest = GET32(ip_pkt, IP_DEST_OFFSET);

        /* Insert this fragment into the list. */
        fp->ipq_first_frag = ip_pkt;

        PUT32(ip_pkt, IPF_NEXT_OFFSET, 0);

        /* The protocol was saved off above. For a fragment this field is
         * temporarily used to store the offset of the IP header from the
         * start of the memory buffer.
         */
        PUT8(ip_pkt, IPF_BUF_OFFSET,
             (UINT8)((CHAR HUGE *)ip_pkt - (CHAR HUGE *)buf_ptr));

        /* Set up a timer event to drop this fragment and any others received
         * that are part of the same datagram if the complete datagram is not
         * received.
         */
        if (TQ_Timerset(EV_IP_REASSEMBLY, (UNSIGNED)fp,
                        IP_FRAG_TTL, 0) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to set timer to timeout reassembly of packet",
                           NERR_SEVERE, __FILE__, __LINE__);

            NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }
    }

    else
    {
        /* The protocol was saved off in the first fragment RX. This field is
         * used to store the offset of the IP header from the start of the
         * memory buffer.
         */
        PUT8(ip_pkt, IPF_BUF_OFFSET,
             (UINT8)((CHAR HUGE *)ip_pkt - (CHAR HUGE *)buf_ptr));

        /* Save off this value to avoid a GET32 in the loop */
        ip_pkt_ipf_offset = GET16(ip_pkt, IPF_OFF_OFFSET);
        ip_pkt_tlen_offset = GET16(ip_pkt, IPF_TLEN_OFFSET);

        /* Find a fragment which begins after this one. */
        for (q = fp->ipq_first_frag; q != NU_NULL;
             q = (IP_FRAG *)GET32(q, IPF_NEXT_OFFSET))
        {
            if (GET16(q, IPF_OFF_OFFSET) > ip_pkt_ipf_offset)
                break;

            /* Keep a pointer to the fragment before q is set to next. */
            p = q;
        }

        /* If q is equal to NULL then the packet just RX does not belong
         * before any of the ones already in the queue. Since it will be
         * appended to the end of the queue, we need to check for overlapping
         * of the last fragment in the queue. If q is not NULL, it means that
         * there is a preceding fragment, and we need to check for overlapping
         * of that fragment. Either case, if the fragment provides some of our
         * data, drop that data from the incoming fragment. If it provides all
         * of the data, drop the current fragment
         */

        if (p != NU_NULL)
        {
            prev = p;

            i = GET16(prev, IPF_OFF_OFFSET) +
                GET16(prev, IPF_TLEN_OFFSET) - ip_pkt_ipf_offset;

            if (i > 0)
            {
                if ((UINT16) i >= ip_pkt_tlen_offset)
                {
                    /* All of the received data is contained in the previous
                     * fragment. Drop this one.
                     */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List,
                                          &MEM_Buffer_Freelist);

                    return (NU_NULL);
                }

                /* Trim the duplicate data from this fragment. */
                MEM_Trim(buf_ptr, (INT32)i);

                PUT16(ip_pkt, IPF_OFF_OFFSET,
                      (UINT16)(ip_pkt_ipf_offset + i));

                PUT16(ip_pkt, IPF_TLEN_OFFSET,
                      (UINT16)(ip_pkt_tlen_offset - i));

                /* Save the values off in locals */
                ip_pkt_ipf_offset = GET16(ip_pkt, IPF_OFF_OFFSET);
                ip_pkt_tlen_offset = GET16(ip_pkt, IPF_TLEN_OFFSET);
            }

            /* While we overlap succeeding fragments trim them or, if they are
             * completely covered, dequeue them.
             */
            while ( (q != NU_NULL) &&
                    ((ip_pkt_ipf_offset + ip_pkt_tlen_offset) >
                    GET16(q, IPF_OFF_OFFSET)) )
            {
                i = (ip_pkt_ipf_offset + ip_pkt_tlen_offset) -
                    GET16(q, IPF_OFF_OFFSET);

                if ((UINT16) i < GET16(q, IPF_TLEN_OFFSET))
                {
                    /* Trim the duplicate data from the start of the previous
                     * fragment. In this case q.
                     */

                    r_buf = (NET_BUFFER *)(((CHAR HUGE *)q) - GET8(q, IPF_BUF_OFFSET));

                    MEM_Trim(r_buf, (INT32)i);

                    PUT16(q, IPF_TLEN_OFFSET, (UINT16)(GET16(q, IPF_TLEN_OFFSET) - i));
                    PUT16(q, IPF_OFF_OFFSET, (UINT16)(GET16(q, IPF_OFF_OFFSET) + i));

                    break;
                }

                /* Move on to the next fragment so we can check to see if any/all of
                 * it is overlapping.  Store this in a temp pointer before the
                 * buffer is deallocated.
                 */
                temp_q = (IP_FRAG *)GET32(q, IPF_NEXT_OFFSET);

                /* The buffer was completely covered. Deallocate it. */
                r_buf = (NET_BUFFER *)(((CHAR HUGE *)q) - GET8(q, IPF_BUF_OFFSET));

                IP_Remove_Frag(q, p, fp);

                MEM_One_Buffer_Chain_Free(r_buf, &MEM_Buffer_Freelist);

                /* Set q to the next buffer in the chain */
                q = temp_q;
            }
        }

        /* Insert the new fragment in its place. The new fragment should be
         * inserted into the list of fragments immediately in front of q.
         */
        IP_Insert_Frag(ip_pkt, fp);
    }

    /* Remove this buffer from the buffer list. */
    MEM_Buffer_Dequeue(&MEM_Buffer_List);

    buf_ptr->next = NU_NULL;

    /* Check for complete reassembly. */
    next = 0;

    for (q = fp->ipq_first_frag; q != NU_NULL;
         q = (IP_FRAG *)GET32(q, IPF_NEXT_OFFSET))
    {
        /* Does the fragment contain the next offset. */
        if (GET16(q, IPF_OFF_OFFSET) != (UINT16) next)
            return (NU_NULL);

        /* Update next to the offset of the next fragment. */
        next += GET16(q, IPF_TLEN_OFFSET);

        /* Keep track of the last fragment checked. It is used below. */
        p = q;
    }

    /* If the more fragments flag is set in the last fragment, then we
       are expecting more. */
    if ( (!p) || (GET8(p, IPF_MFF_OFFSET) & 1) )
        return (NU_NULL);

    /* Clear the fragment timeout event for this datagram. */
    TQ_Timerunset(EV_IP_REASSEMBLY, TQ_CLEAR_EXACT, (UNSIGNED)fp, 0);

    /* Reassembly is complete. Concatenate the fragments. */

    /* Get the first frag. */
    q = fp->ipq_first_frag;

    /* Point the return buffer to the memory buffer of the first frag. */
    r_buf = (NET_BUFFER *)(((CHAR HUGE *)q) - GET8(q, IPF_BUF_OFFSET));

    /* Point to the next fragment in the list. This is done by getting
     * the next frag and then backing up the pointer so that it points
     * to the memory buffer for that IP fragment.
     */
    q = (IP_FRAG *)GET32(q, IPF_NEXT_OFFSET);

    /* Loop through the rest concatenating them all together. */
    while (q != NU_NULL)
    {
        /* Back the pointer up so that we are pointing at the buffer header. */
        m = (NET_BUFFER *)(((CHAR HUGE *)q) - GET8(q, IPF_BUF_OFFSET));

        /* Concatenate the two fragments. */
        MEM_Cat(r_buf, m);

        /* If this is not the first fragment, but the flag is set indicating
         * the sum of this data was tallied by the receiving driver.
         */
        if ( (m != r_buf) && (m->mem_flags & NET_BUF_SUM) )
        {
            /* Add the sum of this buffer to the parent buffer. */
            r_buf->chk_sum += m->chk_sum;

            /* Unset the flag in this buffer. */
            m->mem_flags &= ~NET_BUF_SUM;
        }

        /* Move to the next fragment in preparation for the next loop. */
        q = (IP_FRAG *)GET32(q, IPF_NEXT_OFFSET);
    }

    /* Create the header for the new IP packet by modifying the header of
     * the first packet.
     */

    /* Get a pointer to the IP header. */
    ip_pkt = fp->ipq_first_frag;

    /* Put the protocol type back. */
    PUT8(ip_pkt, IP_PROTOCOL_OFFSET, fp->ipq_protocol);

    /* Set the total length. */
    PUT16(ip_pkt, IPF_TLEN_OFFSET, (UINT16)next);

    /* Remove the LSB that was used to denote packets that are expecting
     * more frags.
     */
    PUT8(ip_pkt, IPF_MFF_OFFSET, (UINT8)(GET8(ip_pkt, IPF_MFF_OFFSET) & ~1));

    /* Get the source and destination IP addresses. */
    PUT32(ip_pkt, IP_SRC_OFFSET, fp->ipq_source);
    PUT32(ip_pkt, IP_DEST_OFFSET, fp->ipq_dest);

    /* Remove this fragment reassembly header from the list. */
    DLL_Remove(&IP_Frag_Queue, fp);


#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* Deallocate this fragment reassembly header. */
    if (NU_Deallocate_Memory(fp) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate memory for fragment structure",
                       NERR_SEVERE, __FILE__, __LINE__);

        NET_DBG_Notify(NU_INVALID_POINTER, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }
#else
    /* Turn the memory flag off to indicate the unused memory */
    NET_Reassembly_Memory_Flags[(UINT8)(fp - NET_Reassembly_Memory)] = NU_FALSE;
#endif

    /* Make the header visible. */
    hlen = ((GET8(ip_pkt, IPF_HL_OFFSET) & 0x0F) << 2);
    r_buf->data_len += (UINT32)hlen;
    r_buf->data_ptr -= (UINT32)hlen;

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
    old_int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

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

} /* IP_Reassembly */

/***********************************************************************
*
*   FUNCTION
*
*       IP_Reassembly_Event
*
*   DESCRIPTION
*
*       Reassembled the packets once a particular event occurs.
*
*   INPUTS
*
*       *fp                     Pointer to the IP element queue
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IP_Reassembly_Event(IP_QUEUE_ELEMENT *fp)
{
    NET_BUFFER      *buf_head;
    IP_FRAG         *ip_frag;
    UINT32          hlen;
    UINT32          next_temp;

    /* Increment the number of IP fragmented packets that could not be
     * reassembled.
     */
    MIB2_ipReasmFails_Inc;

    /* Check that the pointer is not NULL */
    if (fp)
    {
        /* Get the first frag. */
        ip_frag = fp->ipq_first_frag;

        /* Point the head buffer to the memory buffer of the first frag. */
        buf_head =
            (NET_BUFFER*)(((CHAR HUGE*)ip_frag) - GET8(ip_frag, IPF_BUF_OFFSET));

        if (ip_frag->ipf_off == 0)
        {
            /* Get length of header */
            hlen = (GET8(ip_frag, IPF_HL_OFFSET) & 0x0F) << 2;

            /* Move data pointer from data to IP header */
            buf_head->data_ptr           -= hlen;
            buf_head->data_len           += hlen;
            buf_head->mem_total_data_len += hlen;

            /* Save the next and prev fields */
            next_temp = GET32(ip_frag, IPF_NEXT_OFFSET);

            /* Get source and destination */
            PUT32(buf_head->data_ptr, IP_SRC_OFFSET, fp->ipq_source);
            PUT32(buf_head->data_ptr, IP_DEST_OFFSET, fp->ipq_dest);

            ICMP_Send_Error(buf_head, ICMP_TIMXCEED, ICMP_TIMXCEED_REASM,
                            fp->ipq_source , buf_head->mem_buf_device);

            /* Restore the next and prev fields */
            PUT32(ip_frag, IPF_NEXT_OFFSET, next_temp);

            /* Move the data pointer back */
            buf_head->data_ptr           += hlen;
            buf_head->data_len           -= hlen;
            buf_head->mem_total_data_len -= hlen;
        }

        /* Free the buffers on the queue and the queue element */
        IP_Free_Queue_Element(fp);
    }

} /* IP_Reassembly_Event */

/************************************************************************
*
*   FUNCTION
*
*       IP_Free_Queue_Element
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
VOID IP_Free_Queue_Element(IP_QUEUE_ELEMENT *fp)
{
    IP_FRAG     *q, *temp_q;
    NET_BUFFER  *buf_ptr;

    /* Remove this fragment reassembly header from the list. */
    DLL_Remove(&IP_Frag_Queue, fp);

    /* Stop the timer that is running to timeout reassembly of
     * this packet.
     */
    TQ_Timerunset(EV_IP_REASSEMBLY, TQ_CLEAR_EXACT, (UNSIGNED)fp, 0);

    /* Get a pointer to the first fragment. */
    q = fp->ipq_first_frag;

    /* While there are fragments in the list. */
    while (q != NU_NULL)
    {
        /* Get a pointer to the buffer that contains this packet. */
        buf_ptr = (NET_BUFFER *)((CHAR HUGE *)q - GET8(q, IPF_BUF_OFFSET));

        /* Save the next pointer */
        temp_q = (IP_FRAG *)GET32(q, IPF_NEXT_OFFSET);

        /* Deallocate the buffer. */
        MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

        /* Restore the next pointer */
        q = temp_q;
    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* Deallocate the memory. */
    if (NU_Deallocate_Memory(fp) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate memory for fragment structure",
                       NERR_SEVERE, __FILE__, __LINE__);

        NET_DBG_Notify(NU_INVALID_POINTER, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }
#else
    /* Turn the memory flag off to indicate the unused memory */
    NET_Reassembly_Memory_Flags[(UINT8)(fp - NET_Reassembly_Memory)] = NU_FALSE;
#endif

} /* IP_Free_Queue_Element */

#endif
