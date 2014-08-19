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
*   FILENAME
*
*       net_dbg.c
*
*   DESCRIPTION
*
*       This file contains those routines responsible for notifying the
*       application that a fatal error has occurred in the stack.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NET_DBG_Count_Buffers
*       NET_DBG_Validate_Buffs
*       NET_DBG_Find_Dup_Buff
*       NET_DBG_Check_Circular_List
*       NET_DBG_Check_Circular_Chain
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

NET_DEBUG_BUFFER_STRUCT     NET_DBG_Buffer_Ptr_List[MAX_BUFFERS];

static INT                  NET_DBG_Buffer_Ptr_Error_Index = 0;

STATIC INT     NET_DBG_Validate_Buffs(const NET_BUFFER_HEADER *, STATUS *, INT, INT);
STATIC STATUS  NET_DBG_Find_Dup_Buff(const NET_BUFFER *, UINT32 *, INT, INT);
STATIC INT32   NET_DBG_Check_Circular_List(const NET_BUFFER_HEADER *, STATUS *);
STATIC INT     NET_DBG_Check_Circular_Chain(NET_BUFFER *, STATUS *);

#if ( (INCLUDE_ARP == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
extern ARP_RESOLVE_LIST     ARP_Res_List;
#endif

#if ( (INCLUDE_IP_REASSEMBLY == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
extern IP_QUEUE             IP_Frag_Queue;
#endif

/**************************************************************************
*
*   FUNCTION
*
*       NET_DBG_Count_Buffers
*
*   DESCRIPTION
*
*       This function counts the total number of buffers that exist in
*       the system to determine if all buffers are accounted for.  The
*       debug structure is filled in according to how many buffers exist
*       on each list.  If an error is found, a pointer to the offending
*       buffer and a pointer to the two lists on which the buffer is found
*       are saved in the debug structure.  If one of the lists on which
*       the buffer exists is a TCP port structure list, the port index is
*       filled in, if one of the lists is a socket's accept list, the
*       socket index is filled in, if one of the lists is a device
*       transmit queue, the device index is filled in.
*
*   INPUTS
*
*       *debug_struct           A pointer to the debug structure that
*                               will be filled in.
*
*   OUTPUTS
*
*       NU_SUCCESS              The number of buffers in the system is
*                               equal to the number of buffers created
*                               at start up.
*       NET_CIRCULAR_CHAIN      A buffer chain in the list is invalid.
*       NET_CIRCULAR_LIST       The buffer list is invalid.
*       NET_DUP_BUFF            There is a buffer in the list that is on
*                               another list in the system.
*
****************************************************************************/
STATUS NET_DBG_Count_Buffers(NET_DEBUG_BUFFER_INFO_STRUCT *debug_struct)
{
    INT                         i;
    INT                         ooo_count = 0;
    INT                         unp_count = 0;
    INT                         recv_count = 0;
    INT                         driver_count = 0;
    INT                         out_count = 0;
    INT                         free_count = 0;
    INT                         arp_count = 0;
    INT                         re_count = 0;
    INT                         total_count;
    INT                         old_level;
    DV_DEVICE_ENTRY             *cur_dev;
    STATUS                      status = NU_SUCCESS, temp_status;

#if ( ((INCLUDE_IPV4 == NU_TRUE) && \
       ((INCLUDE_IP_REASSEMBLY == NU_TRUE) || (INCLUDE_ARP == NU_TRUE))) || \
      (INCLUDE_TCP == NU_TRUE) )
    NET_BUFFER                  *buf_ptr;
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

#if (INCLUDE_IP_REASSEMBLY == NU_TRUE)
    IP_QUEUE_ELEMENT            *current_frag;
    IP_FRAG                     *frag_ptr;
#endif

#if (INCLUDE_ARP == NU_TRUE)
    struct ARP_RESOLVE_STRUCT   *arp_ptr;
#endif
#endif

#if (INCLUDE_TCP == NU_TRUE)
    TCP_BUFFER                  *tcp_buf;
    INT                         j;
#endif

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    /* Lock out interrupts */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Zero out the debug structure that holds a pointer to all buffers */
    for (i = 0; i < MAX_BUFFERS; i++)
    {
        NET_DBG_Buffer_Ptr_List[i].net_dbg_list1.net_dbg_list = NU_NULL;
        NET_DBG_Buffer_Ptr_List[i].net_dbg_list1.net_dbg_buf_dev_index = -1;
        NET_DBG_Buffer_Ptr_List[i].net_dbg_list1.net_dbg_buf_socket = -1;
        NET_DBG_Buffer_Ptr_List[i].net_dbg_list1.net_dbg_buf_tcp_port = -1;

        NET_DBG_Buffer_Ptr_List[i].net_dbg_list2.net_dbg_list = NU_NULL;
        NET_DBG_Buffer_Ptr_List[i].net_dbg_list2.net_dbg_buf_dev_index = -1;
        NET_DBG_Buffer_Ptr_List[i].net_dbg_list2.net_dbg_buf_socket = -1;
        NET_DBG_Buffer_Ptr_List[i].net_dbg_list2.net_dbg_buf_tcp_port = -1;
    }

    NET_DBG_Buffer_Ptr_Error_Index = -1;

#if (INCLUDE_TCP == NU_TRUE)

    /* Search the Lists of each TCP Port. */
    for (i = 0; i < TCP_MAX_PORTS; i++)
    {
        if (TCP_Ports[i])
        {
            /* Count the number of buffers on the Retransmission List */
            if (TCP_Ports[i]->out.packet_list.tcp_head)
            {
                /* Get a pointer to the first TCP_BUFFER in the list */
                tcp_buf = TCP_Ports[i]->out.packet_list.tcp_head;

                /* Check each element on the retransmission list */
                while (tcp_buf)
                {
                    /* Validate the chain */
                    out_count +=
                        NET_DBG_Check_Circular_Chain(tcp_buf->tcp_buf_ptr,
                                                     &temp_status);

                    if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
                        status = temp_status;

                    for (buf_ptr = tcp_buf->tcp_buf_ptr, j = 0;
                         buf_ptr && j < TCP_MAX_PORTS;
                         buf_ptr = buf_ptr->next_buffer, j++)
                    {
                        /* Check if this buffer is on any other lists */
                        temp_status = NET_DBG_Find_Dup_Buff(buf_ptr,
                                                            (UINT32*)&TCP_Ports[j]->out.packet_list,
                                                            NET_DBG_PORT, j);

                        if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
                            status = temp_status;
                    }

                    /* Get a pointer to the next TCP_BUFFER in the list */
                    tcp_buf = tcp_buf->tcp_next;
                }
            }

            /* Count the number of buffers on the List of incomplete
             * packets
             */
            if (TCP_Ports[i]->out.nextPacket)
            {
                for (buf_ptr = TCP_Ports[i]->out.nextPacket, j = 0;
                     buf_ptr && j < MAX_BUFFERS;
                     buf_ptr = buf_ptr->next_buffer, j++)
                {
                    /* Check if this buffer is on any other lists */
                    temp_status = NET_DBG_Find_Dup_Buff(buf_ptr,
                                                        (UINT32*)&TCP_Ports[i]->out.packet_list,
                                                        NET_DBG_PORT, i);

                    if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
                        status = temp_status;
                }

                /* Validate the chain */
                out_count +=
                    NET_DBG_Check_Circular_Chain(TCP_Ports[i]->out.nextPacket,
                                                 &temp_status);

                if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
                    status = temp_status;
            }

            /* Count the number of buffers on the Out of Order List */
            if (TCP_Ports[i]->in.ooo_list.head)
            {
                ooo_count += NET_DBG_Validate_Buffs(&TCP_Ports[i]->in.ooo_list,
                                                    &temp_status, NET_DBG_PORT, i);

                if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
                    status = temp_status;
            }
        }
    }

#endif

    /* Count the number of buffers waiting on a socket to be received
     * by the application layer.
     */
    for (i = 0; i < NSOCKETS; i++)
    {
        if (SCK_Sockets[i])
        {
            if (SCK_Sockets[i]->s_recvlist.head)
            {
                recv_count +=
                    NET_DBG_Validate_Buffs(&SCK_Sockets[i]->s_recvlist,
                                           &temp_status, NET_DBG_SOCKET, i);

                if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
                    status = temp_status;
            }
        }
    }

    /* Count the number of buffers waiting to be processed by IP */
    if (MEM_Buffer_List.head)
    {
        unp_count = NET_DBG_Validate_Buffs(&MEM_Buffer_List, &temp_status,
                                           NET_DBG_NO_ACTION, 0);

        if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
            status = temp_status;
    }

#if ( (INCLUDE_IP_REASSEMBLY == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* Count the number of buffers awaiting reassembly */
    if (IP_Frag_Queue.ipq_head)
    {
        current_frag = IP_Frag_Queue.ipq_head;

        while ( (current_frag) && (re_count < MAX_BUFFERS) )
        {
            frag_ptr = current_frag->ipq_first_frag;

            /* Count the number of buffers on this fragment list */
            while ( (frag_ptr) && (re_count < MAX_BUFFERS) )
            {
                /* Get a pointer to the buffer */
                buf_ptr =
                    (NET_BUFFER *)(((CHAR HUGE *)frag_ptr) -
                                   GET8(frag_ptr, IPF_BUF_OFFSET));

                /* If this is a buffer chain, validate the chain */
                if (buf_ptr->next_buffer)
                {
                    re_count += NET_DBG_Check_Circular_Chain(buf_ptr, &temp_status);

                    if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
                        status = temp_status;
                }

                else
                    re_count ++;

                for (i = 0;
                     buf_ptr && i < MAX_BUFFERS;
                     buf_ptr = buf_ptr->next_buffer, i++)
                {
                    /* Check that this buffer is not on any other list */
                    temp_status = NET_DBG_Find_Dup_Buff(buf_ptr, (UINT32*)&IP_Frag_Queue,
                                                        NET_DBG_NO_ACTION, 0);

                    /* If an error occurred */
                    if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
                        status = temp_status;
                }

                /* Point to the next buffer in the list. This is done by getting
                 * the next frag and then backing up the pointer so that it points
                 * to the memory buffer for that IP fragment.
                 */
                frag_ptr = (IP_FRAG *)GET32(frag_ptr, IPF_NEXT_OFFSET);
            }

            /* Get the next fragment in the list */
            current_frag = current_frag->ipq_next;
        }
    }

#endif

    /* Count the number of free buffers in the system */
    if (MEM_Buffer_Freelist.head)
    {
        free_count = NET_DBG_Validate_Buffs(&MEM_Buffer_Freelist, &temp_status,
                                            NET_DBG_NO_ACTION, 0);

        if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
            status = temp_status;
    }

    /* Count the buffers on each of the device's transmit queue */
    for (cur_dev = DEV_Table.dv_head; cur_dev;
         cur_dev = cur_dev->dev_next)
    {
        if (DEV_Table.dv_head->dev_transq.head)
        {
            driver_count +=
                NET_DBG_Validate_Buffs(&DEV_Table.dv_head->dev_transq,
                                       &temp_status, NET_DBG_DEV, (INT)cur_dev->dev_index);

            if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
                status = temp_status;
        }
    }

#if ( (INCLUDE_ARP == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* Count the number of buffers waiting for address resolution to
     * complete.
     */
    if (ARP_Res_List.ar_head)
    {
        for (arp_ptr = ARP_Res_List.ar_head;
             arp_ptr && arp_count < MAX_BUFFERS;
             arp_ptr = arp_ptr->ar_next)
        {
            /* Validate the chain */
            arp_count +=
                NET_DBG_Check_Circular_Chain(arp_ptr->ar_buf_ptr, &temp_status);

            if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
                status = temp_status;

            for (buf_ptr = arp_ptr->ar_buf_ptr, i = 0;
                 buf_ptr && i < MAX_BUFFERS;
                 buf_ptr = buf_ptr->next_buffer, i++)
            {
                /* Check if this buffer is on any other lists */
                temp_status = NET_DBG_Find_Dup_Buff(buf_ptr,
                                                    (UINT32*)&ARP_Res_List,
                                                    NET_DBG_NO_ACTION, 0);

                if ( (temp_status != NU_SUCCESS) && (status == NU_SUCCESS) )
                    status = temp_status;
            }
        }
    }

#endif

    /* Fill in the number of buffers used in the system. */
    debug_struct->net_dbg_buf_buffers_used = MEM_Buffers_Used;

    debug_struct->net_dbg_buf_list = NET_DBG_Buffer_Ptr_List;

    /* Re-enable interrupts */
    NU_Local_Control_Interrupts(old_level);

    NU_USER_MODE();

    /* Count the total number of buffers found in the system */
    total_count = ooo_count + unp_count + recv_count + driver_count +
                  out_count + free_count + arp_count;

    debug_struct->net_dbg_buf_total_count = total_count;

    /* Fill in the debug structure */
    debug_struct->net_dbg_buf_arp_count = arp_count;
    debug_struct->net_dbg_buf_driver_count = driver_count;
    debug_struct->net_dbg_buf_free_count = free_count;
    debug_struct->net_dbg_buf_ooo_count = ooo_count;
    debug_struct->net_dbg_buf_out_count = out_count;
    debug_struct->net_dbg_buf_recv_count = recv_count;
    debug_struct->net_dbg_buf_unp_count = unp_count;
    debug_struct->net_dbg_buf_re_count = re_count;
    debug_struct->net_dbg_buf_status = status;

    return (status);

} /* NET_DBG_Count_Buffers */

/**************************************************************************
*
*   FUNCTION
*
*       NET_DBG_Validate_Buffs
*
*   DESCRIPTION
*
*       This function validates the buffer list, each chain of buffers
*       in the buffer list, and checks that none of the buffers are on
*       another list in the system.
*
*   INPUTS
*
*       *hdr                    A pointer to the buffer list.
*       *status                 A pointer to the status that is filled in
*                               by this routine.
*       type                    The type of list the buffer is located
*                               on if it is on a TCP Port list, socket
*                               receive list or device transmit queue.
*       value                   The index value of the type of list the
*                               buffer is located on.
*
*   OUTPUTS
*
*       The total number of buffers in the list and each chain in the
*       list.
*
*       NET_CIRCULAR_CHAIN      A buffer chain in the list is invalid.
*       NET_CIRCULAR_LIST       The buffer list is invalid.
*       NET_DUP_BUFF            There is a buffer in the list that is on
*                               another list in the system.
*
****************************************************************************/
STATIC INT NET_DBG_Validate_Buffs(const NET_BUFFER_HEADER *hdr, STATUS *status,
                                  INT type, INT value)
{
    NET_BUFFER  *buf_ptr, *tmp_buf;
    INT         count = 0, i;
    STATUS      temp_status;

    *status = NU_SUCCESS;

    /* If this is the Free List, do not attempt to validate the buffer
     * chains of the elements, because they are not set until the buffer
     * is allocated.
     */
    if (hdr == &MEM_Buffer_Freelist)
    {
        for (buf_ptr = hdr->head;
             buf_ptr && count < MAX_BUFFERS;
             buf_ptr = buf_ptr->next)
        {
            count ++;

            /* Check if this buffer exists on any other lists in the
             * system or twice on this list.
             */
            temp_status = NET_DBG_Find_Dup_Buff(buf_ptr, (UINT32*)hdr, type,
                                                value);

            /* If two references to the buffer were found */
            if (temp_status != NU_SUCCESS)
            {
                /* If the list has a circular loop, exit this loop and
                 * return an error.
                 */
                if (NET_DBG_Buffer_Ptr_List[NET_DBG_Buffer_Ptr_Error_Index].net_dbg_list1.net_dbg_list ==
                    NET_DBG_Buffer_Ptr_List[NET_DBG_Buffer_Ptr_Error_Index].net_dbg_list2.net_dbg_list)
                {
                    *status = NET_CIRCULAR_LIST;
                    break;
                }

                else
                    *status = temp_status;
            }
        }
    }

    else
    {
        /* Check that the list does not contain any circular problems */
        NET_DBG_Check_Circular_List(hdr, status);

        /* Search the List */
        for (buf_ptr = hdr->head;
             buf_ptr && count < MAX_BUFFERS;
             buf_ptr = buf_ptr->next)
        {
            /* Put each member of the chain on the list */
            for (i = 0, tmp_buf = buf_ptr;
                 tmp_buf && i < MAX_BUFFERS;
                 tmp_buf = tmp_buf->next_buffer, i++)
            {
                /* Check if this buffer exists anywhere else in the
                 * system.
                 */
                temp_status = NET_DBG_Find_Dup_Buff(tmp_buf, (UINT32*)hdr, type,
                                                value);

                if ( (temp_status != NU_SUCCESS) && (*status == NU_SUCCESS) )
                    *status = temp_status;
            }

            /* Check that the chain is not circular */
            count += NET_DBG_Check_Circular_Chain(buf_ptr, &temp_status);

            if (temp_status != NU_SUCCESS)
                *status = temp_status;
        }
    }

    /* Return the number of buffers on the list */
    return (count);

} /* NET_DBG_Validate_Buffs */

/**************************************************************************
*
*   FUNCTION
*
*       NET_DBG_Find_Dup_Buff
*
*   DESCRIPTION
*
*       This function checks that the given buffer is not on more than
*       one list in the system.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the buffer to check.
*       *hdr                    A pointer to the list on which the buffer
*                               to check exists.
*       type                    The type of list the buffer is located
*                               on if it is on a TCP Port list, socket
*                               receive list or device transmit queue.
*       value                   The index value of the type of list the
*                               buffer is located on.
*
*   OUTPUTS
*
*       NU_SUCCESS              The buffer is on only one list.
*       NET_DUP_BUFF            The buffer is on more than one list.
*
****************************************************************************/
STATIC STATUS NET_DBG_Find_Dup_Buff(const NET_BUFFER *buf_ptr, UINT32 *hdr,
                                    INT type, INT value)
{
    INT     j;
    STATUS  status = NU_SUCCESS;

    /* Traverse the list for a duplicate buffer matching buf_ptr */
    for (j = 0; j < MAX_BUFFERS; j ++)
    {
        /* If the buffer is on the list, return an error */
        if (buf_ptr == NET_DBG_Buffer_Ptr_List[j].net_dbg_buffer)
        {
            /* If this is the first instance of this buffer in the list,
             * save a pointer to the list on which the buffer was found.
             */
            if (NET_DBG_Buffer_Ptr_List[j].net_dbg_list1.net_dbg_list == NU_NULL)
            {
                NET_DBG_Buffer_Ptr_List[j].net_dbg_list1.net_dbg_list = hdr;

                switch (type)
                {
                    case NET_DBG_SOCKET:

                        NET_DBG_Buffer_Ptr_List[j].net_dbg_list1.net_dbg_buf_socket = value;
                        break;

#if (INCLUDE_TCP == NU_TRUE)

                    case NET_DBG_PORT:

                        /* If this buffer is on an interface's output
                         * queue, do not count this buffer as being on the
                         * port's retransmission list.
                         */
                        if (!(buf_ptr->mem_flags & NET_TX_QUEUE))
                            NET_DBG_Buffer_Ptr_List[j].net_dbg_list1.net_dbg_buf_tcp_port =
                                value;

                        else
                            NET_DBG_Buffer_Ptr_List[j].net_dbg_list1.net_dbg_list =
                                NU_NULL;

                        break;
#endif
                    case NET_DBG_DEV:

                        NET_DBG_Buffer_Ptr_List[j].net_dbg_list1.net_dbg_buf_dev_index = value;
                        break;

                    default:

                        break;
                }
            }

            /* A buffer can legally exist on an interface's output queue and
             * a TCP port's retransmission list.
             */
            else
#if (INCLUDE_TCP == NU_TRUE)
                 if ( (type != NET_DBG_PORT) ||
                      (!(buf_ptr->mem_flags & NET_TX_QUEUE)) )
#endif
            {
                status = NET_DUP_BUFF;

                /* Set the buffer pointer error index */
                NET_DBG_Buffer_Ptr_Error_Index = j;

                /* Save a pointer to the duplicate list on which the buffer is
                 * located.
                 */
                NET_DBG_Buffer_Ptr_List[j].net_dbg_list2.net_dbg_list = hdr;

                switch (type)
                {
                    case NET_DBG_SOCKET:

                        NET_DBG_Buffer_Ptr_List[j].net_dbg_list2.net_dbg_buf_socket = value;
                        break;

#if (INCLUDE_TCP == NU_TRUE)

                    case NET_DBG_PORT:

                        NET_DBG_Buffer_Ptr_List[j].net_dbg_list2.net_dbg_buf_tcp_port = value;
                        break;
#endif
                    case NET_DBG_DEV:

                        NET_DBG_Buffer_Ptr_List[j].net_dbg_list2.net_dbg_buf_dev_index = value;
                        break;

                    default:

                        break;
                }
            }
        }
    }

    return (status);

} /* NET_DBG_Find_Dup_Buff */

/**************************************************************************
*
*   FUNCTION
*
*       NET_DBG_Check_Circular_List
*
*   DESCRIPTION
*
*       This function checks that the given list does not contain any
*       invalid pointers to incorrect nodes in the list.
*
*   INPUTS
*
*       *hdr                    A pointer to the buffer list.
*       *status                 A pointer to the status that is filled in
*                               by this routine.
*
*   OUTPUTS
*
*       The number of buffers in the list.
*       NET_CIRCULAR_LIST       The list is invalid.
*
****************************************************************************/
STATIC INT32 NET_DBG_Check_Circular_List(const NET_BUFFER_HEADER *hdr, STATUS *status)
{
    NET_BUFFER  *cur_buff, *next_buff;
    INT         max2;
    INT32       max1 = 0;

    /* Get the first entry in the list */
    cur_buff = hdr->head;

    /* While there are entries in the list */
    while ( (cur_buff) && (max1 < MAX_BUFFERS) )
    {
        /* Compare this entry with the next pointer of each entry.  If the
         * next pointer of more than one node points to this entry, return
         * an error.
         */
        next_buff = cur_buff->next;

        max2 = 0;

        /* Ensure we do not get caught in an infinite loop due to the
         * circular buffer problem.
         */
        while ( (next_buff) && (max2 < MAX_BUFFERS) )
        {
            if (next_buff == cur_buff)
            {
                *status = NET_CIRCULAR_LIST;
                break;
            }

            next_buff = next_buff->next;
            max2++;
        }

        max1++;

        if (*status == NET_CIRCULAR_LIST)
            break;

        /* Get a pointer to the next buffer in the list */
        cur_buff = cur_buff->next;
    }

    return (max1);

} /* NET_DBG_Check_Circular_List */

/**************************************************************************
*
*   FUNCTION
*
*       NET_DBG_Check_Circular_Chain
*
*   DESCRIPTION
*
*       This function checks that the given buffer chain does not contain
*       any invalid pointers to incorrect nodes in the chain.
*
*   INPUTS
*
*       *first_buff             A pointer to the first buffer in the chain.
*       *status                 A pointer to the status that is filled in
*                               by this routine.
*
*   OUTPUTS
*
*       The number of buffers in the chain.
*       NET_CIRCULAR_CHAIN      The chain is invalid.
*
****************************************************************************/
STATIC INT NET_DBG_Check_Circular_Chain(NET_BUFFER *first_buff, STATUS *status)
{
    NET_BUFFER  *cur_buff, *next_buff;
    INT         max1 = 0, max2;

    *status = NU_SUCCESS;

    cur_buff = first_buff;

    while ( (cur_buff) && (max1 < MAX_BUFFERS) )
    {
        /* Compare this entry with the next pointer of each entry.  If the
         * next pointer of more than one node points to this entry, return
         * an error.
         */
        next_buff = cur_buff->next_buffer;

        max2 = 0;

        /* Ensure we do not get caught in an infinite loop due to the
         * circular buffer problem.
         */
        while ( (next_buff) && (max2 < MAX_BUFFERS) )
        {
            if (next_buff == cur_buff)
            {
                *status = NET_CIRCULAR_CHAIN;
                break;
            }

            next_buff = next_buff->next_buffer;
            max2++;
        }

        max1++;

        if (*status == NET_CIRCULAR_CHAIN)
            break;

        /* Get a pointer to the next buffer in the list */
        cur_buff = cur_buff->next_buffer;
    }

    return (max1);

} /* NET_Check_Circular_Chain */


