/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       ppp_mpfrag.c
*
*   COMPONENT
*
*       MPFRAG - PPP Multilink Protocol Fragmentation Services
*
*   DESCRIPTION
*
*       This component is responsible for providing Fragmentation
*       Services to PPP MP protocol.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PPP_MP_Fragment_Send
*       PPP_MP_New_Fragment
*       PPP_MP_Make_Packet
*       PPP_MP_Delete_Fragments
*
*   DEPENDENCIES
*
*       nu_ppp.h
*       ppp_mpfrag.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"

/* Only compile this file if PPP MP is enabled. */
#if(INCLUDE_PPP_MP == NU_TRUE)

#include "drivers/ppp_mpfrag.h"

#if MP_DEBUG_PRINT_OK
#define PrintInfo(s)        PPP_Printf(s)
#else
#define PrintInfo(s)
#endif

extern NU_SEMAPHORE MP_Semaphore;

/*************************************************************************
*
*   FUNCTION
*
*       PPP_MP_Fragment_Send
*
*   DESCRIPTION
*
*       This function performs fragmentation on a packet and sends the
*       fragments over a bundle.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the packet to be transmitted.
*       *mp_dev_ptr             Pointer to the MP virtual device
*
*   OUTPUTS
*
*       NU_SUCCESS              Packet was successfully fragmented sent.
*       NU_NOT_PRESENT          Device could not found to send the packet
*                               on.
*       NU_NO_BUFFERS           Enough MEM buffers could not be allocated.
*       NU_HOST_UNREACHABLE     Destination unreachable.
*
*************************************************************************/
STATUS PPP_MP_Fragment_Send(NET_BUFFER *buf_ptr, DV_DEVICE_ENTRY *mp_dev_ptr)
{
    /* Declare Variables. */
    MIB2_IF_STACK_STRUCT        *stack_entry = NU_NULL;
    DV_DEVICE_ENTRY             *dev_ptr;
    PPP_MP_BUNDLE               *bundle;
    NET_BUFFER                  *buffer;
    LCP_LAYER                   *mp_lcp;
    UINT32                      frag_size, offset = 0;
    UINT32                      packet_size = buf_ptr->mem_total_data_len;
    STATUS                      status = NU_SUCCESS;
    UINT8                       mp_head_size;
    UINT8                       protocol_id_size = PPP_MP_PID_SIZE;

    /* Get the bundle associated with this virtual interface. */
    bundle = (PPP_MP_BUNDLE *)mp_dev_ptr->dev_extension;

    /* Get a pointer to the LCP layer of this virtual MP interface. */
    mp_lcp = &(((LINK_LAYER *)mp_dev_ptr->dev_link_layer)->lcp);

    /* Get the header size of the MP fragment. */
    if (mp_lcp->options.remote.flags & PPP_FLAG_SHORT_SEQ_NUM)
    {
        /* Short sequence number header. */
        mp_head_size = PPP_MP_SHORT_FRAG_DATA_OFFSET;
    }
    else
    {
        /* Long sequence number header. */
        mp_head_size = PPP_MP_LONG_FRAG_DATA_OFFSET;
    }

    while ( (packet_size > 0) && (status == NU_SUCCESS) )
    {
       /* Get the interface stack entry from the NET interface stack. */
       stack_entry = MIB2_If_Stack_Get_Next_Entry2(stack_entry,
                                                   NU_TRUE);

       /* Check if the higher layer of the stack entry is same as MP virtual
        * interface.
        */
       if ( (stack_entry) &&
            (stack_entry->mib2_higher_layer != mp_dev_ptr) )
       {
           /* Get the first link in the NET interface stack entry. */
           stack_entry =
            MIB2_If_Stack_Get_HI_Entry(bundle->mp_device_ifindex + 1,
                                       NU_TRUE);
       }

       /* If there is no stack entry associated with the MP virtual
       interface then break out of the while loop. */
       if ( (stack_entry == NU_NULL) ||
            (stack_entry->mib2_lower_layer == NU_NULL) )
       {
           /* No PPP link found to transmit the fragment. */
           status = NU_NOT_PRESENT;
           break;
       }

       /* Point to the PPP link associated with this MP virtual
       interface. */
       dev_ptr = stack_entry->mib2_lower_layer;

       /* If the PPP link is in connected state. */
       if (((LINK_LAYER *)dev_ptr->dev_link_layer)->hwi.state != INITIAL)
       {
            /* If baud rates are present then fragment the packet on the
            proportion of links speeds. */
            if (dev_ptr->dev_baud_rate && bundle->mp_total_baud_rate)
            {
                /* Compute the size of the fragment proportionate to
                 * the speed of the link, rounded down to an 8 byte boundary
                 * by clearing the low-order 3 bits (& ~7).
                 */
                frag_size =
                    ((UINT32)((dev_ptr->dev_baud_rate *
                               buf_ptr->mem_total_data_len) /
                               bundle->mp_total_baud_rate)) & ~7;
            }

            else
            {
                /* Fragment the packet on the basis of number of links. */
                frag_size = ((UINT32)(buf_ptr->mem_total_data_len /
                                      bundle->mp_num_links)) & ~7;
            }

            /* Check if the fragment size calculated is less than the
             * minimum desired.
             */
            if (frag_size < PPP_MP_MIN_FRAG_SIZE)
            {
                /* Set the minimum fragment size. */
                frag_size = PPP_MP_MIN_FRAG_SIZE;
            }

            /* Check if the fragment size is greater than the device MTU. */
            if ( (frag_size) >
                 ((dev_ptr->dev_mtu - mp_head_size - dev_ptr->dev_hdrlen) & ~7) )
            {
                frag_size = ((dev_ptr->dev_mtu - mp_head_size -
                              dev_ptr->dev_hdrlen) & ~7);
            }

            /* If the fragment size is greater than the remaining packet. */
            if (frag_size > packet_size)
            {
                /* This is the last fragment of the packet. */
                frag_size = packet_size;
            }

            /* Get a buffer to copy the data for sending. */
            buffer =
                MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                         frag_size + dev_ptr->dev_hdrlen +
                                         mp_head_size);

            /* If the buffers are not available, return an error. */
            if (buffer == NU_NULL)
            {
                /* set the status. */
                status = NU_NO_BUFFERS;
                break;
            }

            /* Initialize mem_dlist for deallocation. */
            buffer->mem_dlist = &MEM_Buffer_Freelist;

            /* Set the buffer's data pointer. */
            buffer->data_ptr = buffer->mem_parent_packet;

            /* We will add the device header length here. This is only done
             * so that the MEM_Chain_Copy routine below will compute the
             * correct size for the buffer area available. It will be removed
             * after the copy.
             */
            buffer->data_len = (mp_head_size + dev_ptr->dev_hdrlen);

            /* Update buffers total data length. */
            buffer->mem_total_data_len = mp_head_size;

            /* Copy the fragment over to the outgoing packet. */
            MEM_Chain_Copy(buffer, buf_ptr, offset , (INT32)frag_size);

            /* Now remove the header length that was added above. */
            buffer->data_len -= dev_ptr->dev_hdrlen;

            /* Put the data pointer back as well. */
            buffer->data_ptr += (dev_ptr->dev_hdrlen);

            /* Update the buffers total data length. */
            buffer->mem_total_data_len += frag_size;

            /* See if we need to compress the protocol header. */
            if (mp_lcp->options.remote.flags & PPP_FLAG_PFC)
            {
                buffer->data_ptr           -= PPP_PROTOCOL_HEADER_1BYTE;
                buffer->data_len           += PPP_PROTOCOL_HEADER_1BYTE;
                buffer->mem_total_data_len += PPP_PROTOCOL_HEADER_1BYTE;

                /* Add the protocol header. */
                buffer->data_ptr[0] = PPP_MP_COM_PID;

                /* Set the size of the protocol ID */
                protocol_id_size = PPP_PROTOCOL_HEADER_1BYTE;
            }
            else
                /* Put the MP protocol ID. */
                PPP_Add_Protocol(buffer, PPP_MP_PID);

            /* Check if the sequence number is greater than the maximum
            allowed. */
            if ( ((mp_lcp->options.remote.flags & PPP_FLAG_SHORT_SEQ_NUM) &&
                 (bundle->mp_sequence_no > MP_SHORT_MAX_SEQ)) ||
                 (bundle->mp_sequence_no > MP_LONG_MAX_SEQ) )
            {
                /* Reset the sequence number. */
                bundle->mp_sequence_no = 0;
            }

            /* Put the sequence number in the outgoing fragment. */
            if (mp_lcp->options.remote.flags & PPP_FLAG_SHORT_SEQ_NUM)
            {
                /* Put the sequence number of the fragment. */
                PUT16(buffer->data_ptr, protocol_id_size,
                      (UINT16)(bundle->mp_sequence_no++));
            }
            else
            {
                /* Put the sequence number of the fragment. */
                PUT32(buffer->data_ptr, protocol_id_size,
                      bundle->mp_sequence_no++);
            }

            /* If it is a first fragment. */
            if (packet_size == buf_ptr->mem_total_data_len)
            {
                /* Put the beginning bit in the MP fragment header. */
                buffer->data_ptr[protocol_id_size] |= PPP_MP_HEADER_B_BIT;
            }

            /* Check if it is the last fragment. */
            if (frag_size >= packet_size)
            {
                /* Put the end(E) bit for this last fragment. */
                buffer->data_ptr[protocol_id_size] |= PPP_MP_HEADER_E_BIT;

                /* Reset the variables as this is the last fragment. */
                packet_size = frag_size = 0;
            }

            /* Send the packet over the PPP link. */
            status =
            (*(dev_ptr->dev_output))(buffer, dev_ptr, NU_NULL, NU_NULL);

            /* Update the size of the remaining packet which is to be
            fragmented. */
            packet_size -= frag_size;

            /* Increment the offset to the position from which the next
            fragment will start. */
            offset += frag_size;
        }
    }

#if (INCLUDE_TCP == NU_TRUE)

    /* If this buffer is to be deallocated to a port (it has TCP data)
       check to see if the port has been closed in the meantime. If it
       has then set the deallocation list to be the free list. */
    if ( (buf_ptr) && (buf_ptr->mem_flags & NET_PARENT) &&
         (buf_ptr->mem_port_index != -1) &&
         ((TCP_Ports[(UINT16)buf_ptr->mem_port_index]->state == SCLOSED)||
          (TCP_Ports[(UINT16)buf_ptr->mem_port_index]->state == STWAIT)) )
    {
        buf_ptr->mem_dlist = &MEM_Buffer_Freelist;
    }

#endif

    /* If the packet is successfully transmitted then free the buffers,
    otherwise it would be freed by the higher layers. */
    if (status == NU_SUCCESS)
    {
        /* Free the buffer chain of the packet sent. */
        MEM_Multiple_Buffer_Chain_Free(buf_ptr);
    }

    /* Return status. */
    return (status);

} /* PPP_MP_Fragment_Send */

/*************************************************************************
*
*   FUNCTION
*
*       PPP_MP_New_Fragment
*
*   DESCRIPTION
*
*       This function is invoked in response to a fragment which has been
*       received. Once all fragments have been received for a packet,
*       this function will return the pointer to a chain of NET Buffers.
*       If fragments are still remaining this function will return null.
*
*   INPUTS
*
*        None
*
*   OUTPUTS
*
*       NU_NULL if there are fragments still remaining in the frame.
*       Net buffer chain if all the fragments for the frame have been
*       received.
*
*************************************************************************/
NET_BUFFER *PPP_MP_New_Fragment(VOID)
{
   /* Declaring Variables. */
    MIB2_IF_STACK_STRUCT        *stack_entry;
    NET_BUFFER                  *frag = NU_NULL, *buffer,
                                *prev_start = NU_NULL, *prev = NU_NULL;
    PPP_MP_BUNDLE               *bundle = NU_NULL;
    LCP_LAYER                   *lcp;
    UINT32                      seq_no_1, seq_no_2;
    STATUS                      status = NU_NOT_PRESENT;
    UINT16                      counter = 0;
    UINT8                       restart, packet_found = NU_FALSE;
    UINT8                       e_bit_num = 0;

    /* Remove the incoming packet from the list. */
    buffer = MEM_Buffer_Dequeue (&MEM_Buffer_List);

    /* Obtain the MP semaphore. */
    if (NU_Obtain_Semaphore(&MP_Semaphore, NU_SUSPEND) != NU_SUCCESS)
    {
       NLOG_Error_Log("Unable to obtain MP semaphore", NERR_RECOVERABLE,
                      __FILE__, __LINE__);
    }

    /* Check if the incoming buffer is found in the incoming buffer list. */
    if (buffer)
    {
        /* Get the bundle associated with the link (device). */
        status =
            PPP_MP_Find_Bundle_By_Device(&bundle,
                                         buffer->mem_buf_device);
    }

    /* Check if the bundle is found or not. */
    if (status == NU_SUCCESS)
    {
        /* Get LCP layer of the MP virtual device . */
        lcp = &(((LINK_LAYER*)((DEV_Get_Dev_By_Index
                               (bundle->mp_device_ifindex))->
                                dev_link_layer))->lcp);

        /* Get the last sequence number which came on the link. */
        seq_no_2 = buffer->mem_buf_device->system_use_3;

        /* Check if short sequence number header option used. */
        if (lcp->options.remote.flags & PPP_FLAG_SHORT_SEQ_NUM)
        {
            /* Get the sequence number of the received fragment. */
            seq_no_1 = (GET16(buffer->data_ptr, 0)) & MP_SHORT_MAX_SEQ;
        }
        else
        {
            /* Get the sequence number of the received fragment. */
            seq_no_1 = (GET32(buffer->data_ptr, 0)) & MP_LONG_MAX_SEQ;
        }

        /* Update the latest sequence number on this link. */
        buffer->mem_buf_device->system_use_3 = seq_no_1;
    }

    /* If the latest minimum sequence number of the bundle needs to be
     * updated.
     */
    if ( (seq_no_2 == bundle->mp_latest_min_seq_num) &&
         (status == NU_SUCCESS) )
    {
        /* Get the first link in the NET interface stack, lower of the
         * MP device.
         */
        stack_entry =
            MIB2_If_Stack_Get_HI_Entry(bundle->mp_device_ifindex + 1,
                                       NU_TRUE);

        /* Check if the lower stack entry is valid. */
        if(stack_entry->mib2_lower_layer)
        {
            /* Find the minimum latest sequence number over all links. */
            bundle->mp_latest_min_seq_num =
                stack_entry->mib2_lower_layer->system_use_3;
        }

        /* Loop through all the links in the bundle. */
        while (counter != (UINT16)bundle->mp_num_links)
        {
            /* Get the next interface stack entry. */
            stack_entry = MIB2_If_Stack_Get_Next_Entry2(stack_entry, NU_TRUE);

            /* Check if the higher layer of the stack entry is same as MP
             * virtual interface of the bundle.
             */
            if ( (stack_entry->mib2_higher_layer) &&
                 (stack_entry->mib2_higher_layer->dev_index !=
                  bundle->mp_device_ifindex) )
            {
                /* Get the first link in the NET interface stack entry. */
                stack_entry =
                    MIB2_If_Stack_Get_HI_Entry(bundle->mp_device_ifindex + 1,
                                               NU_TRUE);
            }

            if ( (stack_entry->mib2_higher_layer) &&
                 (stack_entry->mib2_lower_layer) )
            {
                /* If the bundle's minimum latest sequence number needs to be
                 * updated.
                 */
                if (bundle->mp_latest_min_seq_num >
                    stack_entry->mib2_lower_layer->system_use_3)
                {
                    /* Update the minimum sequence number of the bundle. */
                    bundle->mp_latest_min_seq_num =
                        stack_entry->mib2_lower_layer->system_use_3;
                }

                /* Increment the number of links (devices) checked. */
                counter++;
            }

        } /* End of while */

        /* Put the counter back to zero for further use later on. */
        counter = 0;

    } /* End of if */

    /* If a bundle is found. */
    if (status == NU_SUCCESS)
    {
        /* Point to the start of the fragment list. */
        frag = bundle->mp_fragment_list;
    }

    /* Go through the ppp_mp_fragment_list (by using the next pointer)
     * until we encounter a fragment number which is greater than the
     * fragment number for the received packet, OR we reach the end of
     * the list.
     */
    while (frag)
    {
        /* Check if the short sequence number header option is being used. */
        if(lcp->options.remote.flags & PPP_FLAG_SHORT_SEQ_NUM)
        {
            /* Get the sequence number of the next fragment in the
             * list.
             */
            seq_no_2 = ((GET16(frag->data_ptr, 0)) & MP_SHORT_MAX_SEQ);
        }

        else
        {
            /* Get the sequence number of the next fragment in the
             * list.
             */
            seq_no_2 = ((GET32(frag->data_ptr, 0)) & MP_LONG_MAX_SEQ);
        }

        /* If the sequence number of the fragment is greater than
         * the incoming, break. This is the place to insert the incoming
         * fragment.
         */
        if (seq_no_2 >= seq_no_1)
        {
            break;
        }

        /* Move to the next fragment in the list. */
        prev = frag;
        frag = frag->next;
    }

    /* If we have not got a duplicate fragment OR it is the start of a
     * fragment list OR if the fragment with sequence number zero has come.
     */
    if ( ((seq_no_2 != seq_no_1) || (seq_no_1 == 0) ||
         (frag == NU_NULL)) && (status == NU_SUCCESS) )
    {
        /* If there are no fragments in the fragment list. */
        if ( (bundle->mp_fragment_list == NU_NULL) ||
             (bundle->mp_fragment_list == frag) )
        {
            /* This is the starting fragment in the list. */
            bundle->mp_fragment_list = buffer;

            /* Point to the next fragment. */
            buffer->next = frag;
        }

        /* If the fragment is to be placed somewhere between the list. */
        else
        {
            /* Put the fragment in its position. */
            buffer->next = frag;
            prev->next = buffer;
        }

        /* If the incoming fragment contains an ending(E) bit. */
        if (buffer->data_ptr[0] & PPP_MP_HEADER_E_BIT)
        {
            /* Increment the number of E bit fragments in the fragment
             * list.
             */
            bundle->mp_e_bit++;
        }

        /* Initialize the pointers for further use. */
        prev = buffer = NU_NULL;

        /* Initialize the variables for further use. */
        seq_no_1 = seq_no_2 = 0;

        /* Point to the start of the fragment list. */
        frag = bundle->mp_fragment_list;

        /* If there is E bit fragment(s) in the list then search the
         * fragment list and see if a packet is complete OR we reach the end
         * of the list.
         */
        while ( (frag) && (packet_found == NU_FALSE) && (bundle->mp_e_bit) )
        {
            /* Restart is used to re-traverse the fragment list if
             * fragments are needed to be deleted (due to fragment loss)
             * while traversing the list.
             */
            restart = NU_FALSE;

            /* Increment the number of fragments between the B bit
             * and E bit
             */
            counter++;

            /* If the fragment contains the B bit. */
            if (frag->data_ptr[0] & PPP_MP_HEADER_B_BIT)
            {
                /* Reset the fragment counter. */
                counter = 1;

                /* prev_start is used to point to the fragment just before
                 * the beginning (B) bit fragment, i.e. start of a packet.
                 */
                prev_start = prev;

                /* Buffer is used to point to the fragment with the B bit.
                 * i.e. the start of a new packet.
                 */
                buffer = frag;

                /* Check if short sequence number header option is being
                 * used.
                 */
                if (lcp->options.remote.flags & PPP_FLAG_SHORT_SEQ_NUM)
                {
                    /* Get the sequence number of the starting fragment. */
                    seq_no_1 =
                        ((GET16(buffer->data_ptr, 0)) & MP_SHORT_MAX_SEQ);

                    /* If there are fragments prior to this B bit fragment. */
                    if (prev)
                    {
                        /* Check if E bit fragment of the previous packet
                         * is not received.
                         */
                        if (((UINT16)(bundle->mp_latest_min_seq_num)) >
                            ((UINT16)((GET16(prev->data_ptr, 0)) & MP_SHORT_MAX_SEQ)))
                        {
                            /* Delete the fragments. */
                            PPP_MP_Delete_Fragments(bundle, prev);

                            /* Initialize the pointers. */
                            prev_start = prev = NU_NULL;

                            /* Update number of E bit fragments which are left. */
                            bundle->mp_e_bit -= e_bit_num;

                            /* Initialize. */
                            e_bit_num = 0;
                        }
                    }
                }

                /* Long sequence number header case. */
                else
                {
                    /* Get the sequence number of the starting fragment. */
                    seq_no_1 =
                        ((GET32(buffer->data_ptr, 0)) & MP_LONG_MAX_SEQ);

                    /* If there are fragments prior to this B bit fragment. */
                    if (prev)
                    {
                        if (bundle->mp_latest_min_seq_num >
                            ((GET32(prev->data_ptr, 0)) & MP_LONG_MAX_SEQ))
                        {
                            /* Delete the fragments. */
                            PPP_MP_Delete_Fragments(bundle, prev);

                            /* Initialize the pointers. */
                            prev_start = prev = NU_NULL;

                            /* Update number of E bit fragments which
                             * are left.
                             */
                            bundle->mp_e_bit -= e_bit_num;

                            /* Initialize. */
                            e_bit_num = 0;
                        }
                    }
                }
            }

            /* If the fragment contains an E bit. */
            if (frag->data_ptr[0] & PPP_MP_HEADER_E_BIT)
            {
                /* Check if short sequence number header option is being
                 * used.
                 */
                if (lcp->options.remote.flags & PPP_FLAG_SHORT_SEQ_NUM)
                {
                    /* Get the sequence number of the fragment. */
                    seq_no_2 =
                        (GET16(frag->data_ptr, 0)) & MP_SHORT_MAX_SEQ;
                }
                else
                {
                    /* Get the sequence number of the fragment. */
                    seq_no_2 =
                        (GET32(frag->data_ptr, 0)) & MP_LONG_MAX_SEQ;
                }

                /* If there was a fragment with a B bit found. */
                if (buffer)
                {
                    /* Check if the all the fragments within a packet have
                     * arrived. If yes, then take the fragments out of the
                     * fragment list.
                     */
                    if ((seq_no_2 - seq_no_1) == (counter - (UINT32)1))
                    {
                         /* If the first fragment in the fragment list has
                          * both B bit and E bit or the first fragment is the
                          * B bit fragment.
                          */
                         if ( (prev == bundle->mp_fragment_list) ||
                              (!prev_start) )
                         {
                             if (prev_start)
                             {
                                prev_start->next = frag->next;
                             }

                             /* Packet starts from the first fragment in
                              * the fragment list.
                              */
                             else
                             {
                                bundle->mp_fragment_list = frag->next;
                             }
                         }

                         else
                         {
                             /* Take the fragments out of the fragment
                              * list.
                              */
                             prev_start->next = frag->next;
                         }

                         /* Point the last fragment of the packet to null. */
                         frag->next = NU_NULL;

                         /* Update the number of E bit fragments
                          * in the fragment list.
                          */
                         bundle->mp_e_bit--;

                         /* Check if short sequence number header option
                          * is being used.
                          */
                         if (lcp->options.remote.flags & PPP_FLAG_SHORT_SEQ_NUM)
                         {
                             /* Make a complete packet from the fragments. */
                             PPP_MP_Make_Packet(&buffer, 2);
                         }
                         else
                         {
                             /* Make a complete packet from the fragments. */
                             PPP_MP_Make_Packet(&buffer, 4);
                         }

                         stack_entry =
                            MIB2_If_Stack_Get_LI_Entry(buffer->
                                                       mem_buf_device->
                                                       dev_index + 1, NU_TRUE);

                         /* Point to the MP device. */
                         buffer->mem_buf_device =
                            stack_entry->mib2_higher_layer;

                         packet_found = NU_TRUE;
                    }
                }

                if (packet_found == NU_FALSE)
                {
                    /* Update the number of E bit fragments found before a
                     * complete packet is found.
                     */
                    e_bit_num ++;

                    /* Check if the minimum latest sequence number over all
                     * the links in the bundle has got greater than the
                     * fragment's sequence number. This means fragment(s)
                     * of this packet has been lost because of the
                     * increasing sequence number property of the links.
                     * So delete these fragments.
                     */
                    if (bundle->mp_latest_min_seq_num > seq_no_2)
                    {
                       /* Fragment(s) has been lost, so remove other
                        * fragments of that packet.
                        */
                       PPP_MP_Delete_Fragments(bundle, frag);

                       /* Re-initialize pointers. */
                       prev_start = prev = NU_NULL;

                       /* Start from the beginning. */
                       frag = bundle->mp_fragment_list;

                       /* Update the number of E bit fragments in the
                        * fragment list.
                        */
                       bundle->mp_e_bit--;

                       /* Re-initialize the E bit variable. */
                       e_bit_num = 0;

                       /* Start traversing the list again. */
                       restart = NU_TRUE;
                    }
                }
            }

            if ( (restart == NU_FALSE) && (packet_found == NU_FALSE) )
            {
                /* Update the fragment list traversal pointers. */
                prev = frag;
                frag = frag->next;
            }

        }/* while */

    }/* if */

    /* If we have got a duplicate fragment, free the buffers. */
    else if (status == NU_SUCCESS)
    {
        MEM_One_Buffer_Chain_Free(buffer, &MEM_Buffer_Freelist);
    }

    /* Release the semaphore obtained above. */
    if (NU_Release_Semaphore(&MP_Semaphore) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* If the packet was completed (all fragments were received),
     * then return the buffer for the frame. Otherwise, return null.
     */
    if (packet_found == NU_FALSE)
    {
        buffer = NU_NULL;
    }

    /* Return buffer. */
    return (buffer);

} /* PPP_MP_New_Fragment */

/*************************************************************************
* FUNCTION
*
*       PPP_MP_Make_Packet
*
* DESCRIPTION
*
*       This function removes the fragment headers from the fragments
*       received and make a complete packet out of them.
*
* INPUTS
*
*       **buf_ptr               Chain of buffers (fragments)
*       header_size             Size of the MP header e.g. 4 for long
*                               sequence number header and 2 for
*                               short sequence number header
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID PPP_MP_Make_Packet(NET_BUFFER **buf_ptr, UINT8 header_size)
{
    /* Declare Variables. */
    NET_BUFFER    *buffer_chain;
    NET_BUFFER    *temp = (*buf_ptr)->next;

    /* Combine the data part of the fragments into one PPP packet. */
    for (buffer_chain = *buf_ptr; buffer_chain; buffer_chain = temp)
    {
        /* Trim off the MP header. */
        MEM_Trim(buffer_chain, header_size);

        /* Check if it is not the first fragment. */
        if (buffer_chain != *buf_ptr)
        {
            /* Concatenate with the first buffer chain. */
            MEM_Cat(*buf_ptr, buffer_chain);

            /* Point to the next buffer chain. */
            temp = buffer_chain->next;

            /* There should now be no next buffer chain. */
            buffer_chain->next = NU_NULL;

            /* Update the total length of the packet. */
            (*buf_ptr)->mem_total_data_len +=
                buffer_chain->mem_total_data_len;
        }
    }

    /* Get a buffer to copy the data for sending. */
    buffer_chain =
        MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                 (*buf_ptr)->mem_total_data_len + 1);

    /* Set the data pointer. */
    buffer_chain->data_ptr = buffer_chain->mem_parent_packet;

     /* Check if protocol compression is used. */
    if ((*buf_ptr)->data_ptr[0] & 1)
    {
        /* Put zero at the first place. */
        *(buffer_chain->data_ptr) = 0;

        /* Update the buffer len. */
        buffer_chain->data_len = buffer_chain->mem_total_data_len = 1;
    }

    /* Copy the data. */
    MEM_Chain_Copy(buffer_chain, *buf_ptr, 0,
                   (*buf_ptr)->mem_total_data_len);

    /* Set the device on which the packet came on. */
    buffer_chain->mem_buf_device = (*buf_ptr)->mem_buf_device;

    /* End of the packet should not be pointing anywhere now. */
    (*buf_ptr)->next = NU_NULL;

    /* Free the buffer and put it back onto the free buffer list. */
    MEM_One_Buffer_Chain_Free(*buf_ptr, &MEM_Buffer_Freelist);

    /* Point to the new buffer chain. */
    *buf_ptr = buffer_chain;

} /* PPP_MP_Make_Packet */

/*************************************************************************
*
*   FUNCTION
*
*       PPP_MP_Delete_Fragments
*
*   DESCRIPTION
*
*       This function deletes fragments of a packet when fragment(s)
*       loss occurs.
*
*   INPUTS
*
*       *bundle                 Pointer to the bundle.
*       *last_buffer            Last buffer to be deleted in the bundle
*                               fragment list.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID PPP_MP_Delete_Fragments(PPP_MP_BUNDLE *bundle,
                             NET_BUFFER *last_buffer)
{
    /* Declaring Variables. */
    NET_BUFFER  *buf_ptr, *buffer;

    /* Point to the start of the fragment list. */
    buf_ptr = bundle->mp_fragment_list;

    /* Fragments till the last_buffer will be removed so update the
     * bundle fragment list.
     */
    bundle->mp_fragment_list = last_buffer->next;

    /* Remove the fragments . */
    while (buf_ptr)
    {
        /* Point to the buffer to be freed. */
        buffer = buf_ptr;

        /* Point to the next buffer chain. */
        buf_ptr = buf_ptr->next;

        /* Free the buffer and put it back onto the free buffer list. */
        MEM_One_Buffer_Chain_Free(buffer, &MEM_Buffer_Freelist);

        /* If we have reached the end of list of fragments which are to
         * be removed.
         */
        if (buf_ptr == bundle->mp_fragment_list)
        {
            break;
        }
    }

    PrintInfo("Some fragment(s) removed due to fragment(s) loss.\n");

} /* PPP_MP_Delete_Fragments. */

#endif /* INCLUDE_PPP_MP. */
