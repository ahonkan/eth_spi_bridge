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
* FILE NAME
*
*       mem_bcd.c
*
* DESCRIPTION
*
*       This file contains the implementation of MEM_Buffer_Chain_Dequeue.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       MEM_Buffer_Chain_Dequeue
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       MEM_Buffer_Chain_Dequeue
*
*   DESCRIPTION
*
*       Dequeue a linked chain of buffer(s) large enough to hold the
*       number of bytes of packet data.
*
*       This function should only be used for allocating a chain of
*       buffers in which the first buffer will contain buffer
*       header information. This is used for STARTING
*       a buffer chain and NOT for appending to a chain of buffers.
*       The reason for this is that this function takes into
*       account the size of the buffer header data structure when
*       calculating how many buffers need to be allocated.
*
*   INPUTS
*
*       *header                 A pointer to the net buffer header
*                               information
*       nbytes                  Number of bytes to be dequeued off of
*                               the chain
*
*   OUTPUTS
*
*       NET_BUFFER*             A pointer to the head of the buffer chain
*       NU_NULL                 The header or the number of bytes are
*                               incorrect, or the buffers are freed from
*                               the list
*
*************************************************************************/
#ifdef NU_DEBUG_NET_BUFFERS
NET_BUFFER *MEM_DB_Buffer_Chain_Dequeue(NET_BUFFER_HEADER *header, INT32 nbytes,
                                        CHAR *the_file, INT the_line)
#else
NET_BUFFER *MEM_Buffer_Chain_Dequeue(NET_BUFFER_HEADER *header, INT32 nbytes)
#endif
{
    NET_BUFFER *ret_buf_ptr, *work_buf_ptr;
    INT32       x, num_bufs;
    INT         old_level;

    if ( (!header) || (nbytes <= 0) )
        return (NU_NULL);

    /*  Temporarily lockout interrupts to protect the global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Go ahead and dequeue the parent buffer of the chain. */
    ret_buf_ptr = MEM_Buffer_Dequeue (header);

    /* Make sure we got a buffer. */
    if (ret_buf_ptr != NU_NULL)
    {
        /* NULL the pointer */
        ret_buf_ptr->next           = NU_NULL;
        ret_buf_ptr->next_buffer    = NU_NULL;

        /* Check to see if we need to chain some buffers together. */
        num_bufs = (nbytes + ((INT32)(sizeof(struct _me_bufhdr)) - 1))/ (INT32)NET_MAX_BUFFER_SIZE;

        /* If we need more get the first one */
        if (num_bufs)
        {
            ret_buf_ptr->next_buffer = work_buf_ptr = MEM_Buffer_Dequeue (header);

            /* Make sure we got a buffer. If not, the ones we did get will be put
               back on the buffer freelist and NULL returned to caller. */
            if (work_buf_ptr != NU_NULL)
            {

                /* Now get the rest and link them together. */
                for (x = 1; x < num_bufs; x++)
                {
                    /* Dequeue a buffer and link it to the buffer chain. */
                    work_buf_ptr->next_buffer   = MEM_Buffer_Dequeue (header);

                    /* Make sure we got a buffer. If not, the ones we did get will be put
                       back on the buffer freelist and NULL returned to caller. */
                    if (work_buf_ptr->next_buffer != NU_NULL)
                    {
                        work_buf_ptr->next      = NU_NULL;

                        /* Move the work pointer to the next buffer. */
                        work_buf_ptr = work_buf_ptr->next_buffer;
                    }
                    else
                    {
                        /* Give the buffers back. */
                        MEM_One_Buffer_Chain_Free (ret_buf_ptr, &MEM_Buffer_Freelist);

                        /* Null the return pointer. */
                        ret_buf_ptr = NU_NULL;

                        /* Get out of the for loop */
                        x = num_bufs;
                    }
                }

                /* Make sure we are not falling through because of lack of buffers. */
                if (ret_buf_ptr != NU_NULL)
                {
                    /* Null the end of the chain. */
                    work_buf_ptr->next_buffer   = NU_NULL;
                    work_buf_ptr->next          = NU_NULL;
                }
            }
            else
            {
                /* Give the buffers back. */
                MEM_One_Buffer_Chain_Free(ret_buf_ptr, &MEM_Buffer_Freelist);

                /* Null the return pointer. */
                ret_buf_ptr = NU_NULL;
            }
        }
    }

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* Return the head off the buffer chain. */
    return (ret_buf_ptr);

} /* MEM_Buffer_Chain_Dequeue */
