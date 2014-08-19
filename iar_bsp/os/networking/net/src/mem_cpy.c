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
*       mem_cpy.c
*
*   DESCRIPTION
*
*       This files contains functions to copy data to/from a NET buffer
*       and user buffer.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       MEM_Copy_Data
*       MEM_Copy_Buffer
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       MEM_Copy_Data
*
*   DESCRIPTION
*
*       This function copies a given buffer of data into a chain
*       of NET buffers.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the start of the buffer
*                               chain.
*       *buffer                 A pointer to the buffer from which to
*                               copy data.
*       numbytes                The total data length to copy.
*       flags                   Flags used to indicate any of the
*                               following; the buffer is zero copy,
*                               checksumming has been disabled for
*                               this data, checksumming will be done
*                               in hardware for this data.
*
*   OUTPUTS
*
*       The number of bytes copied into the buffer chain.
*
*************************************************************************/
INT32 MEM_Copy_Data(NET_BUFFER *buf_ptr, const CHAR HUGE *buffer, INT32 numbytes,
                    UINT16 flags)
{
    INT32       bytes_left;
    NET_BUFFER  *work_buf, *prev_buf = NU_NULL;
    INT32       bc;
    UINT32      sum = 0;
    UINT8       *src, *dst;

    /* Get a pointer to the first buffer in the chain */
    work_buf = buf_ptr;

    /* If there is already data in this buffer chain, find the last buffer
     * in the chain that contains data.  This is the buffer into which to
     * begin copying data.
     */
    if (work_buf->mem_total_data_len != 0)
    {
        for (work_buf = buf_ptr;
             work_buf->next_buffer != NU_NULL &&
             work_buf->next_buffer->data_len != 0;
             work_buf = work_buf->next_buffer)
                  ;
    }

    bytes_left = numbytes;

    /* If this is the first packet in the list, calculate the number of
     * bytes to copy based on the size of the number of bytes already
     * in the packet and the size of the transport layer header.
     */
    if (work_buf == buf_ptr)
        bc = (bytes_left <
             (INT32)((NET_PARENT_BUFFER_SIZE - (INT32)work_buf->data_len) -
             (work_buf->data_ptr - work_buf->mem_parent_packet)))
             ? bytes_left :
             (INT32)((NET_PARENT_BUFFER_SIZE - (INT32)work_buf->data_len) -
             (work_buf->data_ptr - work_buf->mem_parent_packet));
    else
        bc = (bytes_left < (INT32)(NET_MAX_BUFFER_SIZE - work_buf->data_len))
             ? bytes_left : (INT32)(NET_MAX_BUFFER_SIZE - work_buf->data_len);

    /* Copy the data into the buffer chain. */
    while ( (bytes_left) && (work_buf) )
    {
        /* If this is not a zerocopy buffer, fill the buffer with data */
        if (!(flags & SF_ZC_MODE))
        {
            /* If the flag is set in the first buffer indicating that
             * checksumming should be done with the data copy.
             */
            if (buf_ptr->mem_flags & NET_BUF_SUM)
            {
                /* If this is the parent buffer, store the starting point of the
                 * checksum calculation.  Do not modify this pointer if it is
                 * already set, as that means we are adding data to an existing
                 * buffer chain.
                 */
                if ( (work_buf == buf_ptr) && (!buf_ptr->sum_data_ptr) )
                {
                    /* Set the checksum data pointer to the beginning of the
                     * data that is being checksummed.  When the checksum gets
                     * computed, if sum_data_ptr does not match data_ptr, the
                     * checksum routine will add the missing bytes to the sum;
                     * ie, the transport layer header.
                     */
                    buf_ptr->sum_data_ptr = (buf_ptr->data_ptr + buf_ptr->data_len);
                }

                /* If we are adding data to a buffer, and the previous length was
                 * odd, the 16-bit checksum calculation will be incorrect and
                 * require adjustment.
                 */
                if ( (work_buf->data_len) && ((work_buf->data_len % 2) != 0) )
                {
                    /* Get a pointer to the odd byte that was copied into the
                     * previous buffer.
                     */
                    dst = (UINT8*)(&work_buf->data_ptr[work_buf->data_len - 1]);

                    /* Get a pointer to the source buffer. */
                    src = (UINT8*)buffer;

                    /* Remove the odd byte from the checksum calculation. */
                    buf_ptr->chk_sum -= (((UINT16*)dst)[0] & INTSWAP(0xFF00));

                    /* Put one byte of new data in the existing buffer. */
                    dst[1] = src[0];

                    /* Compute the checksum of the next two bytes. */
                    buf_ptr->chk_sum += ((UINT16*)(dst))[0];

                    /* Increment the source buffer by one byte. */
                    buffer ++;

                    /* decrement the number of bytes to copy by one byte since one
                     * byte was just copied.
                     */
                    bc --;

                    /* Increment the length of this buffer since one byte was just
                     * put into it.
                     */
                    work_buf->data_len ++;

                    /* Decrement the number of bytes left to copy into the buffer
                     * chain.
                     */
                    bytes_left --;
                }

                /* Copy the data into the network buffer and return the sum. */
                UTL_Sum_Memcpy((work_buf->data_ptr + work_buf->data_len),
                               buffer, (UINT32)bc, &sum);

                /* Add the sum to the parent buffer. */
                buf_ptr->chk_sum += sum;
            }

            /* Otherwise, copy the data by regular means. */
            else
            {
                NU_BLOCK_COPY((work_buf->data_ptr + work_buf->data_len),
                              buffer, (unsigned int)bc);
            }

            /* Advance the pointer to the user data. */
            buffer += bc;
        }

        /* Initialize the data length of the zero copy buffer to zero */
        else
        {
            work_buf->data_len = 0;

            /* Save a pointer to the previous buffer */
            prev_buf = work_buf;
        }

        /* update the number of bytes left to be copied into the buffer
         * chain.
         */
        bytes_left -= bc;

        /* Increment the number of bytes of data in this buffer. */
        work_buf->data_len += (UINT32)bc;

        /* Point to the next buffer in the chain. */
        work_buf = work_buf->next_buffer;

        /* If there is another buffer in the chain, set up the data pointer
         * and the number of bytes to copy into the buffer.
         */
        if (work_buf)
        {
            /* Point to where the data will begin. */
            work_buf->data_ptr = work_buf->mem_packet;

            bc = (bytes_left < (INT32)NET_MAX_BUFFER_SIZE)
                 ? bytes_left : (INT32)NET_MAX_BUFFER_SIZE;
        }
    } /* end of while loop */

    /* Increment the total amount of data in the buffer */
    if (!(flags & SF_ZC_MODE))
        buf_ptr->mem_total_data_len += (UINT32)(numbytes - bytes_left);

    /* This is a zero copy buffer.  Set the total data length to the number
     * of bytes copied and free any unused buffers.
     */
    else
    {
        /* This buffer could be being reused from a previous call to
         * receive data.  The total data length parameter will contain
         * the total data length of the receive.  Reset it now.
         */
        buf_ptr->mem_total_data_len = (UINT32)(numbytes - bytes_left);

        /* Reset the TCP data length field to zero */
        buf_ptr->mem_tcp_data_len = 0;

        /* If there are more buffers in the chain than needed, free
         * the unused buffers now.
         */
        if (work_buf)
        {
            /* Ensure prev_buf is valid. */
            if (prev_buf)
            {
                prev_buf->next_buffer = NU_NULL;
            }

            /* Return the buffers to the free list */
            MEM_One_Buffer_Chain_Free(work_buf, &MEM_Buffer_Freelist);
        }
    }

    /* Return the number of bytes copied into the buffer chain */
    return (numbytes - bytes_left);

} /* MEM_Copy_Data */

/*************************************************************************
*
*   FUNCTION
*
*       MEM_Copy_Buffer
*
*   DESCRIPTION
*
*       This function copies data from a chain of NET buffers into a
*       buffer.
*
*   INPUTS
*
*       *buffer                 A pointer to a buffer into which to
*                               copy the data.
*       *sockptr                A pointer to the socket structure.
*       numbytes                The number of bytes to copy into the
*                               buffer.
*
*   OUTPUTS
*
*       The number of bytes copied from the buffer chain.
*
*************************************************************************/
INT32 MEM_Copy_Buffer(CHAR HUGE *buffer, const struct sock_struct *sockptr,
                      INT32 numbytes)
{
    NET_BUFFER  *buf_ptr;
    INT32       bytes_copied = 0;
    UINT32      *tp;
    INT32       bytes_to_copy;

    /* In zerocopy mode just set the address to the incoming buffer */
    if (sockptr->s_flags & SF_ZC_MODE)
    {
        /* use address of incoming buffer parameter */
        tp = (UINT32*)buffer;

        /* get address of incoming RX buffer list */
        *tp = (UINT32)sockptr->s_recvlist.head;

        bytes_copied = (INT32)sockptr->s_recvlist.head->mem_total_data_len;
    }

    /* Move the data into the caller's buffer, copy it from the buffer chain. */
    else
    {
        /* Get a pointer to the data */
        buf_ptr = sockptr->s_recvlist.head;

        /* Loop through the chain if needed and copy all buffers until
         * the maximum number of bytes the user can accept has been copied or
         * the end of data is reached.
         */
        while ( (buf_ptr != NU_NULL) && (bytes_copied < numbytes) )
        {
            /* Determine how many bytes to copy from the first buffer */
            if (buf_ptr->data_len <= (UINT32)(numbytes - bytes_copied))
                bytes_to_copy = (INT32)buf_ptr->data_len;
            else
                bytes_to_copy = numbytes - bytes_copied;

            /* Copy the data */
            NU_BLOCK_COPY((buffer + bytes_copied), buf_ptr->data_ptr,
                          (unsigned int)bytes_to_copy);

            /* Update the bytes copied. */
            bytes_copied += bytes_to_copy;

            /* Increment the data pointer past the data just copied */
            buf_ptr->data_ptr += bytes_to_copy;

            /* Decrement the data length of this buffer by the amount of
             * data copied.
             */
            buf_ptr->data_len -= (UINT32)bytes_to_copy;

            /* Move to the next buffer in the chain */
            buf_ptr = buf_ptr->next_buffer;

        } /* end while there are buffers in the chain */

        /* Decrement the total number of bytes left in the packet */
        sockptr->s_recvlist.head->mem_total_data_len -= (UINT32)bytes_copied;
    }

    return (bytes_copied);

} /* MEM_Copy_Buffer */
