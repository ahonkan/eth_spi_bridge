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
*       tcp_sack.c
*
*   DESCRIPTION
*
*       This file contains routines specific to Selective Acknowledgements
*       within the TCP module.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Build_SACK_Block
*       TCP_Find_Continuous_SACK_Block
*       TCP_SACK_Flag_Packets
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (NET_INCLUDE_SACK == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Build_SACK_Block
*
*   DESCRIPTION
*
*       This function builds the SACK Blocks that follow the initial
*       block in a SACK option.
*
*   INPUTS
*
*       *buffer                 A pointer to the buffer in which to
*                               build the block.
*       *prt                    A pointer to the port for which the
*                               block is being built.
*       *offset_ptr             A pointer to the offset into which to
*                               begin building the option.  Upon return,
*                               this pointer is pointing to the last
*                               SACK block.
*       bytes_avail             The number of bytes available in the
*                               buffer for blocks.
*       left_edge               The left edge of the first block that
*                               was added to the packet.
*       right_edge              The right edge of the first block that
*                               was added to the packet.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID TCP_Build_SACK_Block(UINT8 *buffer, TCP_PORT *prt, UINT8 *offset_ptr,
                          UINT8 bytes_avail, UINT32 left_edge, UINT32 right_edge)
{
    UINT32      saved_right = 0;
    NET_BUFFER  *buf_ptr, *next_buffer;
    UINT8       offset = *offset_ptr;

    /* Get a pointer to the head of the out of order list. */
    buf_ptr = prt->in.ooo_list.head;

    /* While there are packets on the out of order list and room in the
     * buffer.
     */
    while ( (buf_ptr) && ((offset + TCP_SACK_BLOCK_LENGTH) <= bytes_avail) )
    {
        /* If this sequence number is not covered by the first block that
         * was added to the packet.
         */
        if ( (buf_ptr->mem_seqnum < left_edge) ||
             (buf_ptr->mem_seqnum > right_edge) )
        {
            /* Add this left edge to the packet. */
            PUT32(buffer, offset + TCP_SACK_BLOCK_LEFT_EDGE,
                  buf_ptr->mem_seqnum);

            /* Find the right edge to add to the packet. */
            next_buffer = TCP_Find_Continuous_SACK_Block(buf_ptr, &saved_right);

            /* If a contiguous block was not found. */
            if (!saved_right)
            {
                /* Compute the right edge for this block. */
                saved_right = buf_ptr->mem_seqnum + buf_ptr->mem_tcp_data_len;
            }

            /* Add this right edge to the packet. */
            PUT32(buffer, offset + TCP_SACK_BLOCK_RIGHT_EDGE, saved_right);

            /* Increment the offset by one block. */
            offset += TCP_SACK_BLOCK_LENGTH;

            /* Start on the next buffer. */
            buf_ptr = next_buffer;
        }

        else
        {
            buf_ptr = buf_ptr->next;
        }
    }

    /* Update the offset value to be returned. */
    *offset_ptr = offset;

} /* TCP_Build_SACK_Block */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Find_Continuous_SACK_Block
*
*   DESCRIPTION
*
*       This function traverses the out of order list to find a contiguous
*       block of data, starting with the buffer passed into the routine.
*
*   INPUTS
*
*       *ptr                    The buffer containing the left edge to
*                               use.
*       *right_edge             The right edge to fill in.
*
*   OUTPUTS
*
*       A pointer to the next buffer to pass into the routine.  This will
*       be the next buffer in the list that is not part of the
*       contiguous block found.
*
*************************************************************************/
NET_BUFFER *TCP_Find_Continuous_SACK_Block(NET_BUFFER *ptr, UINT32 *right_edge)
{
    NET_BUFFER  *buf_ptr = ptr;
    NET_BUFFER  *next_buffer = NU_NULL;

    /* Initialize the right edge to the sequence number plus the length of
     * the first segment.
     */
    *right_edge = buf_ptr->mem_seqnum + buf_ptr->mem_tcp_data_len;

    /* While there are two buffers left to compare. */
    while ( (buf_ptr) && (buf_ptr->next) )
    {
        /* If the next two blocks are contiguous. */
        if (buf_ptr->mem_seqnum + buf_ptr->mem_tcp_data_len ==
            buf_ptr->next->mem_seqnum)
        {
            /* Update the right edge to reflect the data in this
             * contiguous block.
             */
            *right_edge += buf_ptr->next->mem_tcp_data_len;

            /* Return a pointer to the next buffer's next.  Since
             * the current buffer and the next buffer are contiguous,
             * the next buffer's next might not be contiguous.
             */
            next_buffer = buf_ptr->next->next;
        }

        else
        {
            next_buffer = buf_ptr->next;
            break;
        }

        /* Get a pointer to the next buffer. */
        buf_ptr = buf_ptr->next;
    }

    /* Reset right_edge if no contiguous data was found. */
    if (*right_edge == (ptr->mem_seqnum + ptr->mem_tcp_data_len))
    {
        *right_edge = 0;
    }

    return (next_buffer);

} /* TCP_Find_Continuous_SACK_Block */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_SACK_Flag_Packets
*
*   DESCRIPTION
*
*       This function parses a TCP SACK option and marks all packets on
*       the retransmission list for the port to indicate that the data
*       has been covered by an incoming SACK.
*
*   INPUTS
*
*       *buffer                 A pointer to the head of the SACK option.
*       *prt                    The port for which the SACK applies.
*
*   OUTPUTS
*
*       bytes                   The number of bytes parsed from the packet.
*
*************************************************************************/
UINT8 TCP_SACK_Flag_Packets(UINT8 *buffer, TCP_PORT *prt)
{
    UINT8       byte_count, opt_len;
    TCP_BUFFER  *tcp_buf;
    UINT32      left_edge, right_edge;

    /* The packet will hold at least the SACK header fields. */
    byte_count = TCP_SACK_LENGTH;

    /* Get the length of the option. */
    opt_len = GET8(buffer, TCP_SACK_LEN_OFFSET) - TCP_SACK_LENGTH;

    /* While there are blocks remaining to be processed. */
    while (opt_len)
    {
        /* Extract the left edge from the packet. */
        left_edge = GET32(buffer, byte_count + TCP_SACK_BLOCK_LEFT_EDGE);

        /* Extract the right edge from the packet. */
        right_edge = GET32(buffer, byte_count + TCP_SACK_BLOCK_RIGHT_EDGE);

        TEST_TCP_SACK_Included(left_edge, right_edge);

        /* Get a pointer to the head of the retransmission list. */
        tcp_buf = prt->out.packet_list.tcp_head;

        /* While there are packets remaining on the retransmission list. */
        while (tcp_buf)
        {
            /* If this segment is covered by the SACK, flag it. */
            if ( (tcp_buf->tcp_buf_ptr->mem_seqnum >= left_edge) &&
                 (tcp_buf->tcp_buf_ptr->mem_seqnum +
                  tcp_buf->tcp_buf_ptr->mem_tcp_data_len <= right_edge) )
            {
#if (NET_INCLUDE_LMTD_TX == NU_TRUE)

                /* If this packet has not already been included in a SACK,
                 * set the flag on the port indicating that new SACK
                 * data has been received.
                 */
                if (!(tcp_buf->tcp_buf_ptr->mem_flags & NET_TCP_SACK))
                {
                    prt->portFlags |= TCP_NEW_SACK_DATA;
                }
#endif

                tcp_buf->tcp_buf_ptr->mem_flags |= NET_TCP_SACK;
            }

            /* Get a pointer to the next packet. */
            tcp_buf = tcp_buf->tcp_next;
        }

        /* Decrement the remaining length of the SACK option. */
        opt_len -= TCP_SACK_BLOCK_LENGTH;

        /* Increment the number of bytes parsed from the option. */
        byte_count += TCP_SACK_BLOCK_LENGTH;
    }

    /* Return the number of bytes added to the packet. */
    return (byte_count);

} /* TCP_SACK_Flag_Packets */

#endif
