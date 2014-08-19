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
*   FILE NAME
*
*       mem.c
*
*   COMPONENT
*
*       Net Stack Buffer Management
*
*   DESCRIPTION
*
*       Buffer management routines used by the network stack.
*
*   DATA STRUCTURES
*
*       MEM_Buffer_List
*       MEM_Buffer_Freelist
*       MEM_Buffer_Suspension_List
*       MEM_Buffers_Used
*       NET_Buffer_Suspension_HISR
*       *MEM_Non_Cached
*
*   FUNCTIONS
*
*       MEM_Init
*       MEM_Buffer_Dequeue
*       MEM_Buffer_Enqueue
*       MEM_Buffer_Chain_Free
*       MEM_One_Buffer_Chain_Free
*       MEM_Multiple_Buffer_Chain_Free
*       MEM_Buffer_Suspension_HISR
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "services/nu_trace_os_mark.h"

/* Declare the NET pointers for holding incoming, and empty packet buffers.
   Also declare the buffer suspension list to hold tasks waiting for
   buffers.  */
NET_BUFFER_HEADER           MEM_Buffer_List;
NET_BUFFER_HEADER           MEM_Buffer_Freelist;
NET_BUFFER_SUSPENSION_LIST  MEM_Buffer_Suspension_List;

/* Declare the suspension hisr. When buffers become available this HISR
   will wake up tasks that are suspended waiting for buffers. */
NU_HISR NET_Buffer_Suspension_HISR;

/* A global counter of the number of buffers that are currently allocated.
 * Initialized in MEM_Init. */
UINT16 MEM_Buffers_Used;

/* Global used for debugging buffers */
#ifdef NU_DEBUG_NET_BUFFERS
NET_BUFFER           *MEM_Debug_Buffer_List;
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for the HISR */
CHAR  NET_Hisr_Memory[NET_MAX_HISR_SIZE];

CHAR  *MEM_Non_Cached;

#else

/* This is a pointer to the memory pool from which Nucleus NET will
   allocate frame buffers. Note that one platforms where both NET and the MAC
   layer hardware can write to the buffers this memory pool should be in
   non-cached memory. Otherwise it can be in cached memory. */
NU_MEMORY_POOL      *MEM_Non_Cached;

#endif      /* INCLUDE_STATIC_BUILD */

/*************************************************************************
*
*   FUNCTION
*
*       MEM_Init
*
*   DESCRIPTION
*
*       This function initializes the Memory buffer component of
*       Nucleus NET.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_MEM_ALLOC            Memory cannot be allocated
*
*************************************************************************/
STATUS MEM_Init(VOID)
{
    static INT16    i;
    CHAR HUGE       *ptr;
    STATUS          ret_status;
	NU_MEMORY_POOL 	*pool;

#ifdef NU_DEBUG_NET_BUFFERS
    NET_BUFFER      *prev_buf;
#endif

    /* Initialize the global buffer pointers. */
    MEM_Buffer_List.head            = NU_NULL;
    MEM_Buffer_List.tail            = NU_NULL;
    MEM_Buffer_Freelist.head        = NU_NULL;
    MEM_Buffer_Freelist.tail        = NU_NULL;
    MEM_Buffer_Suspension_List.head = NU_NULL;
    MEM_Buffer_Suspension_List.tail = NU_NULL;

#ifdef NU_DEBUG_NET_BUFFERS
    MEM_Debug_Buffer_List           = NU_NULL;
#endif

    /* Initialize the number of buffers that are currently allocated. */
    MEM_Buffers_Used = MAX_BUFFERS;

    /* Break the block of memory up and place each block into the
     * buffer_freelist.
     */
    for (i = 0; i < MAX_BUFFERS; i++)
    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

		pool = MEM_Non_Cached;

        /* Allocate the memory for a single packet buffer. The size of one
           buffer is allocated plus an extra chunk in the case that a memory
           alignment other than 4 bytes is required. Nucleus PLUS always
           allocates memory on sizeof(UNSIGNED) byte boundaries. Hence the
           subtraction of sizeof(UNSIGNED) from the REQ_ALIGNMENT VALUE. */
        ret_status = NU_Allocate_Memory(pool, (VOID **)&ptr,
                                        (UNSIGNED)(sizeof(NET_BUFFER) +
                                        REQ_ALIGNMENT - sizeof(UNSIGNED)),
                                        (UNSIGNED)NU_NO_SUSPEND);
        if (ret_status != NU_SUCCESS)
        {
            NLOG_Error_Log ("Unable to alloc memory for NET buffer", NERR_FATAL,
                                __FILE__, __LINE__);
            return (NU_MEM_ALLOC);
        }

#else
        /* Assign memory to the buffer */
        ptr = MEM_Non_Cached;
        MEM_Non_Cached += NET_BUFFER_MEMORY;
#endif

        ptr = (CHAR *)TLS_Normalize_Ptr(ptr);

        /* If necessary, align the start of this buffer on the correct boundary. */
        if ((UNSIGNED)ptr % REQ_ALIGNMENT)
            ptr += REQ_ALIGNMENT - ((UNSIGNED)ptr % REQ_ALIGNMENT);

        MEM_Buffer_Enqueue(&MEM_Buffer_Freelist, (NET_BUFFER *)ptr);

#ifdef NU_DEBUG_NET_BUFFERS

        if (MEM_Debug_Buffer_List)
        {
            prev_buf->next_debug = (NET_BUFFER *)ptr;
            ((NET_BUFFER *)ptr)->debug_index = i;
        }
        else
        {
            MEM_Debug_Buffer_List = (NET_BUFFER *)ptr;
            MEM_Debug_Buffer_List->debug_index = i;
        }

        prev_buf = (NET_BUFFER *)ptr;

        MEM_Buffer_Freelist.tail->debug_index = i;
#endif

#if (NU_DEBUG_NET == NU_TRUE)
        NET_DBG_Buffer_Ptr_List[i].net_dbg_buffer = (NET_BUFFER*)ptr;
#endif
    }

    /* Initialize the number of buffers that are currently allocated. */
    MEM_Buffers_Used = 0;

    /* Allocate memory for the buffer suspension HISR. */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    ret_status = NU_Allocate_Memory(MEM_Cached, (VOID **)&ptr,
                                    NET_BUFFER_SUSPENSION_HISR_SIZE,
                                    NU_NO_SUSPEND);

    /* Did we get the memory? */
    if (ret_status == NU_SUCCESS)
#else
    /* Assign memory to the HISR */
    ptr = NET_Hisr_Memory;
#endif

    {
        /* Create the HISR. */
        ret_status = NU_Create_HISR (&NET_Buffer_Suspension_HISR, "buf_susp",
                                     MEM_Buffer_Suspension_HISR, 2, ptr,
                                     NET_BUFFER_SUSPENSION_HISR_SIZE);
    }

    return (ret_status);

} /* MEM_Init */

/*************************************************************************
*
*   FUNCTION
*
*       MEM_Buffer_Dequeue
*
*   DESCRIPTION
*
*       Remove and return the first node in a linked list.
*
*   INPUTS
*
*       *hdr                    Pointer to the net buffer header
*
*   OUTPUTS
*
*       node                    The net buffer is dequeued
*       NU_NULL                 The net buffer is not dequeued
*
*************************************************************************/
#ifdef NU_DEBUG_NET_BUFFERS
NET_BUFFER *MEM_DB_Buffer_Dequeue(NET_BUFFER_HEADER *hdr, CHAR *the_file, INT the_line)
#else
NET_BUFFER *MEM_Buffer_Dequeue(NET_BUFFER_HEADER *hdr)
#endif
{
    UNSIGNED zero = 0;           /* causes most compilers to place this */
                                 /*   value in a register only once */

    struct _me_bufhdr* headerP;
    NET_BUFFER  *node;
    INT         old_level;

#if (NU_DEBUG_NET == NU_TRUE)
    STATUS      status;
#endif

    /* Make sure we were not passed a null pointer. */
    if (!hdr)
        return (NU_NULL);

    /*  Temporarily lockout interrupts to protect global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* If there is a node in the list we want to remove it. */
    if (hdr->head)
    {
        /* Get the node to be removed */
        node = hdr->head;

        /* Make the hdr point the second node in the list */
        hdr->head = node->next;

        /* If this is the last node the headers tail pointer needs to be nulled
           We do not need to clear the node's next since it is already null */
        if (!(hdr->head))
            hdr->tail = NU_NULL;

        /* Is a buffer being removed from the buffer_freelist.  If so increment
           the buffers_used counter and clear the buffer header. */
        if (hdr == &MEM_Buffer_Freelist)
        {
            /* Zero the header info. */
            headerP = &(node->me_data.me_pkthdr.me_buf_hdr);

            headerP->seqnum = zero;
            headerP->dlist = &MEM_Buffer_Freelist;
            headerP->buf_device = (struct _DV_DEVICE_ENTRY*)(zero);
            headerP->option_len = (UINT16)zero;
            headerP->retransmits = (INT16)zero;
            headerP->tcp_data_len = (UINT16)zero;
            headerP->total_data_len = zero;
            headerP->port_index = -1;
            node->chk_sum = (UINT32)(zero);

            /* Zero the pointers. */
            node->next = (NET_BUFFER*)(zero);
            node->next_buffer = (NET_BUFFER*)(zero);
            node->data_ptr = (UINT8 HUGE*)(zero);
            node->data_len = zero;
            node->pqe_flags = (UINT16)zero;
            node->sum_data_ptr = (UINT8 HUGE*)(zero);

#if (HARDWARE_OFFLOAD == NU_TRUE)
            node->hw_options = (UINT32)zero;
#endif

            /* Bump the number of buffers that have been pulled from the
               freelist. */
            MEM_Buffers_Used++;
            
            /* Trace log */
            T_BUFF_USAGE(MEM_Buffers_Used);

#ifdef NU_DEBUG_NET_BUFFERS
            node->who_allocated_file = the_file;
            node->who_allocated_line = the_line;
#endif
        }
    }
    else
        node = NU_NULL;

#if (NU_DEBUG_NET == NU_TRUE)

    status = NET_DBG_Validate_MEM_Buffers_Used();

    if (status != NU_SUCCESS)
        NET_DBG_Notify(status, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
#endif

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* Return a pointer to the removed node */
    return (node);

}  /* MEM_Buffer_Dequeue */

/*************************************************************************
*
*   FUNCTION
*
*       MEM_Buffer_Enqueue
*
*   DESCRIPTION
*
*       Insert an item at the end of a linked list.
*
*   INPUTS
*
*       *hdr                    Pointer to the net buffer header
*                               information
*       *item                   Pointer to the net buffer information
*
*   OUTPUTS
*
*       item                    The net buffer is enqueued
*       NU_NULL                 The net buffer is not enqueued
*
*************************************************************************/
NET_BUFFER *MEM_Buffer_Enqueue(NET_BUFFER_HEADER *hdr, NET_BUFFER *item)
{
    INT     old_level;

#if (NU_DEBUG_NET == NU_TRUE)
    STATUS  status;
#endif

    if (!hdr)
        return (NU_NULL);

    if (item != NU_NULL)
    {
        /* Temporarily lockout interrupts to protect global buffer variables. */
        old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Set node's next to point at NULL */
        item->next = NU_NULL;

        /*  If there is currently a node in the linked list, we want to add the
            new node to the end. */
        if (hdr->head)
        {
            /* Make the last node's next point to the new node. */
            hdr->tail->next = item;

            /* Make the roots tail point to the new node */
            hdr->tail = item;
        }
        /* If the linked list was empty, we want both the root's head and
           tail to point to the new node. */
        else
        {
            hdr->head = item;
            hdr->tail = item;
        }

        /* If a buffer is being moved back onto the buffer free list, then
           decrement the the number of buffers that are currently used. */
        if (hdr == &MEM_Buffer_Freelist)
        {
            --MEM_Buffers_Used;
#ifdef NU_DEBUG_NET_BUFFERS
            item->who_allocated_file = NU_NULL;
            item->who_allocated_line = 0;
#endif

            /* Trace log */
            T_BUFF_USAGE(MEM_Buffers_Used);
        }

#if (NU_DEBUG_NET == NU_TRUE)

        status = NET_DBG_Validate_MEM_Buffers_Used();

        if (status != NU_SUCCESS)
            NET_DBG_Notify(status, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
#endif

        /*  Restore the previous interrupt lockout level.  */
        NU_Local_Control_Interrupts(old_level);
    }

    return(item);

} /* MEM_Buffer_Enqueue */

/*************************************************************************
*
*   FUNCTION
*
*       MEM_Buffer_Chain_Free
*
*   DESCRIPTION
*
*       This function removes the first node and each one in the chain
*       from the source list and places them at the tail of the
*       destination list as individual nodes, not a chain.
*
*   INPUTS
*
*       *source                 Pointer to the list from which the node
*                               will be removed.
*       *dest                   Pointer to the list to which the node
*                               will be placed.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID MEM_Buffer_Chain_Free(NET_BUFFER_HEADER *source, NET_BUFFER_HEADER *dest)
{
    NET_BUFFER  *tmp_ptr;
    INT         old_level;

    if ( (!source) || (!dest) )
        return;

    /*  Temporarily lockout interrupts to protect the global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    tmp_ptr = MEM_Buffer_Dequeue(source);

    /* Go through the entire buffer chain moving each buffer to the
       destination list */
    while (tmp_ptr != NU_NULL)
    {
        /* Put one part of the chain onto the destination list */
        MEM_Buffer_Enqueue(dest, tmp_ptr);

        /* Move to the next buffer in the chain */
        tmp_ptr = tmp_ptr->next_buffer;
    }

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* If there are enough free buffers and a task is waiting for
       buffers then activate a HISR to resume the task. */
    if (MEM_Buffer_Suspension_List.head &&
                ((MAX_BUFFERS - MEM_Buffers_Used) > NET_FREE_BUFFER_THRESHOLD))
    {
        if (NU_Activate_HISR(&NET_Buffer_Suspension_HISR) != NU_SUCCESS)
            NET_DBG_Notify(NU_INVALID_HISR, __FILE__, __LINE__, NU_NULL,
                           NU_NULL);
    }

} /* MEM_Buffer_Chain_Free */

/*************************************************************************
*
*   FUNCTION
*
*       MEM_One_Buffer_Chain_Free
*
*   DESCRIPTION
*
*       This function removes all nodes in the source chain and puts them
*       at the end of the destination list, if the destination list is
*       the buffer freelist. Otherwise it just puts the source node to
*       the end of the dest list.
*
*   INPUTS
*
*       *source                 Pointer to the buffer chain that will be
*                               removed.
*       *dest                   Pointer to the list to which the node(s)
*                               will be placed
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID MEM_One_Buffer_Chain_Free(NET_BUFFER *source, NET_BUFFER_HEADER *dest)
{
    INT         old_level;

    if ( (!source) || (!dest) )
        return;

    /*  Temporarily lockout interrupts to protect the global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Only deallocate all the buffers in the chain if the destination list
       is the buffer free list. Otherwise just move the parent buffer to
       the end of the dest list. */
    if (dest == &MEM_Buffer_Freelist)
    {
        /* Go through the entire buffer chain moving each buffer to the
           destination list */
        while (source != NU_NULL)
        {
            /* Put one part of the chain onto the destination list */
            MEM_Buffer_Enqueue(dest, source);

            /* Move to the next buffer in the chain */
            source = source->next_buffer;
        }
    }
    else
        MEM_Buffer_Enqueue(dest, source);

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* If there are enough free buffers and a task is waiting for
       buffers then activate a HISR to resume the task. */
    if (MEM_Buffer_Suspension_List.head &&
                ((MAX_BUFFERS - MEM_Buffers_Used) > NET_FREE_BUFFER_THRESHOLD))
    {
        if (NU_Activate_HISR(&NET_Buffer_Suspension_HISR) != NU_SUCCESS)
            NET_DBG_Notify(NU_INVALID_HISR, __FILE__, __LINE__, NU_NULL,
                           NU_NULL);
    }

} /* MEM_One_Buffer_Chain_Free */

/*************************************************************************
*
*   FUNCTION
*
*       MEM_Multiple_Buffer_Chain_Free
*
*   DESCRIPTION
*
*       This function frees a chain of buffers to the appropriate free
*       lists.  Since IP appends a new buffer for the IP and link-layer
*       headers, the transport layer portion of the datagram may need
*       to be freed to a different free list than the header portion
*       of the datagram.  If the free lists are not the same, the
*       NET_PARENT flag will be set in the buffer.
*
*   INPUTS
*
*       *source                 Pointer to the buffer chain that will be
*                               removed.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID MEM_Multiple_Buffer_Chain_Free(NET_BUFFER *source)
{
    NET_BUFFER      *temp_ptr;

#ifndef PACKET
    INT             old_level;
#endif

    if (!source)
        return;

    /* Interrupts do not need to be locked out here when PACKET mode is
     * enabled since the driver does not call MEM_Multiple_Buffer_Chain_Free
     * if PACKET mode is enabled when a packet has finished transmitting,
     * but this routine is called from the context of the stack; therefore,
     * the code is protected by the semaphore and does not need to lock out
     * interrupts.
     */
#ifndef PACKET

    /*  Temporarily lockout interrupts to protect the global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

#endif

    temp_ptr = source;

    /* Free each of the packets to the dlist */
    do
    {
        /* Put one part of the chain onto the destination list */
        MEM_Buffer_Enqueue(temp_ptr->mem_dlist, source);

        /* Move to the next buffer in the chain */
        source = source->next_buffer;

        /* If this is the start of a new chain, set temp_ptr to the
         * beginning of the chain
         */
        if ( (source) && (source->mem_flags & NET_PARENT) )
            temp_ptr = source;

    } while ( (source) && (temp_ptr->mem_dlist == &MEM_Buffer_Freelist) );

    /* If there are more packets to free, they all belong to the same
     * list, so free them now.
     */
    if (source)
    {
        /* If the dlist is empty, unset the flag indicating that this packet
         * is being transmitted by the driver.
         */
        if (!source->mem_dlist)
            source->mem_flags &= ~NET_TX_QUEUE;

        else
            MEM_Buffer_Enqueue(source->mem_dlist, source);
    }

#ifndef PACKET

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

#endif

    /* If there are enough free buffers and a task is waiting for
     * buffers then activate a HISR to resume the task.
     */
    if ( (MEM_Buffer_Suspension_List.head) &&
         ((MAX_BUFFERS - MEM_Buffers_Used) > NET_FREE_BUFFER_THRESHOLD) )
    {
        if (NU_Activate_HISR(&NET_Buffer_Suspension_HISR) != NU_SUCCESS)
            NET_DBG_Notify(NU_INVALID_HISR, __FILE__, __LINE__, NU_NULL,
                           NU_NULL);
    }

} /* MEM_Multiple_Buffer_Chain_Free */

/*************************************************************************
*
*   FUNCTION
*
*       MEM_Buffer_Suspension_HISR
*
*   DESCRIPTION
*
*       This function resumes the first task on the buffer suspension list
*       It then removes that tasks entry from the list.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID MEM_Buffer_Suspension_HISR (VOID)
{
    /* Make sure there is really a task to wake up. */
    if (MEM_Buffer_Suspension_List.head)
    {
        /* Let the events dispatcher do the work, it has the semaphore. */
        if (EQ_Put_Event(MEM_RESUME,
                         (UNSIGNED)MEM_Buffer_Suspension_List.head->list_entry,
                         MEM_Buffer_Suspension_List.head->socketd) != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to place MEM buffer suspension on the event list",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Remove this element from the suspension list. */
            DLL_Dequeue(&MEM_Buffer_Suspension_List);
        }
    }

} /* MEM_Buffer_Suspension_HISR */
