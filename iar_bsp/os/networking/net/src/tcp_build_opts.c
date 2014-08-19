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
*       tcp_build_opts.c
*
*   DESCRIPTION
*
*       This file contains routines used to build TCP options.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Build_WindowScale_Option
*       TCP_Build_SACK_Perm_Option
*       TCP_Build_SACK_Option
*       TCP_Build_DSACK_Option
*       TCP_Build_MSS_Option
*       TCP_Build_Timestamp_Option
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       net_cfg.h
*       nu_net.h
*       net_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "networking/net_cfg.h"
#include "networking/nu_net.h"
#include "networking/net_extr.h"

#if (NET_INCLUDE_WINDOWSCALE == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Build_WindowScale_Option
*
*   DESCRIPTION
*
*       This function builds a TCP Window Scale option in the provided
*       buffer.
*
*   INPUTS
*
*       *buffer                 A pointer to the buffer in which to
*                               build the TCP Window Scale option.
*       *prt                    A pointer to the port for the connection
*                               for which this option is being built.
*       *bytes_avail            The number of bytes available in the TCP
*                               header for the Window Scale option.
*
*   OUTPUTS
*
*       bytes                   The number of bytes added to the packet.
*
*************************************************************************/
UINT8 TCP_Build_WindowScale_Option(UINT8 *buffer, TCP_PORT *prt,
                                   UINT8 *bytes_avail)
{
    UINT8   new_bytes = TCP_WINDOWSCALE_LENGTH;

    /* If there is room for this option in the packet. */
    if (*bytes_avail >= TCP_WINDOWSCALE_LENGTH)
    {
        /* Set the "kind" field to the Window Scale type. */
        buffer[TCP_WINDOWSCALE_KIND_OFFSET] = TCP_WINDOWSCALE_OPT;

        /* Set the length of the option. */
        buffer[TCP_WINDOWSCALE_LEN_OFFSET] = TCP_WINDOWSCALE_LENGTH;

        /* Add the scale factor to the packet. */
        buffer[TCP_WINDOWSCALE_SHIFT_OFFSET] = prt->in.p_win_shift;

        /* Decrement the number of bytes remaining in this packet
         * for options.
         */
        *bytes_avail -= new_bytes;
    }

    /* This option will not fit in the packet. */
    else
    {
        new_bytes = 0;
    }

    /* Return the number of bytes added to the packet. */
    return (new_bytes);

} /* TCP_Build_WindowScale_Option */

#endif

#if (NET_INCLUDE_SACK == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Build_SACK_Perm_Option
*
*   DESCRIPTION
*
*       This function builds a SACK-Permitted option in the provided
*       buffer.
*
*   INPUTS
*
*       *buffer                 A pointer to the buffer in which to
*                               build the SACK option.
*       *bytes_avail            The number of bytes available in the TCP
*                               header for the D-SACK option.
*
*   OUTPUTS
*
*       bytes                   The number of bytes added to the packet.
*
*************************************************************************/
UINT8 TCP_Build_SACK_Perm_Option(UINT8 *buffer, UINT8 *bytes_avail)
{
    UINT8   new_bytes = TCP_SACK_PERM_LENGTH;

    /* If the option will fit in the packet. */
    if (*bytes_avail >= TCP_SACK_PERM_LENGTH)
    {
        /* Set the "kind" field to the SACK Permitted type. */
        buffer[TCP_SACK_PERM_KIND_OFFSET] = TCP_SACK_PERM_OPT;

        /* Set the length of the option. */
        buffer[TCP_SACK_PERM_LEN_OFFSET] = TCP_SACK_PERM_LENGTH;

        /* Decrement the number of bytes remaining in this packet
         * for options.
         */
        *bytes_avail -= new_bytes;
    }

    /* There is no room in the packet for this option. */
    else
    {
        new_bytes = 0;
    }

    /* Return the number of bytes added to the packet. */
    return (new_bytes);

} /* TCP_Build_SACK_Perm_Option */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Build_SACK_Option
*
*   DESCRIPTION
*
*       This function builds a SACK option in the provided buffer.
*
*   INPUTS
*
*       *buffer                 A pointer to the buffer in which to
*                               build the SACK option.
*       *prt                    The port for which to build the SACK
*                               option.
*       *bytes_avail            The number of bytes available in the TCP
*                               header for the SACK option.
*
*   OUTPUTS
*
*       bytes                   The number of bytes added to the packet.
*
*************************************************************************/
UINT8 TCP_Build_SACK_Option(UINT8 *buffer, TCP_PORT *prt, UINT8 *bytes_avail)
{
    UINT8       offset = 0;
    NET_BUFFER  *buf_ptr, *next_buffer;
    UINT32      left_edge, right_edge, saved_left = 0, saved_right;

    /* Initialize the left edge to the sequence number of the invoking
     * packet.
     */
    left_edge = prt->left_edge;

    /* Initialize the right edge to the sequence number plus data length
     * of the invoking packet.
     */
    right_edge = prt->right_edge;

    /* If at least one SACK block will fit in the buffer. */
    if (*bytes_avail >= (TCP_SACK_LENGTH + TCP_SACK_BLOCK_LENGTH))
    {
        /* Set the "kind" field to the SACK type. */
        buffer[TCP_SACK_KIND_OFFSET] = TCP_SACK_OPT;

        /* Initialize the offset to the start of the first block. */
        offset = TCP_SACK_BLOCK_OFFSET;

        /* Get a pointer to the head of the out of order list. */
        buf_ptr = prt->in.ooo_list.head;

        /* Search the list for contiguous data covering the first block
         * in the list.
         */
        while (buf_ptr)
        {
            /* Determine if this buffer is the start of a contiguous block in
             * the out of order list and get a pointer to the next buffer that
             * does not belong to the contiguous block.
             */
            next_buffer = TCP_Find_Continuous_SACK_Block(buf_ptr, &saved_right);

            /* If a contiguous block was found, save off the left_edge and
             * compute the length of the block.
             */
            if (saved_right)
            {
                saved_left = buf_ptr->mem_seqnum;
            }

            /* A contiguous block was not found. */
            else
            {
                /* If the sequence number of this buffer covers the first
                 * block.
                 */
                if (buf_ptr->mem_seqnum + buf_ptr->mem_tcp_data_len == left_edge)
                {
                    /* Save this sequence number. */
                    saved_left = buf_ptr->mem_seqnum;

                    /* Compute the right edge of this block. */
                    saved_right = saved_left + buf_ptr->mem_tcp_data_len;
                }
            }

            /* If the contiguous block is contiguous with the data just
             * received.
             */
            if (saved_right == left_edge)
            {
                /* Move the left edge back to reflect the existing contiguous
                 * blocks.  The right edge remains the same.
                 */
                left_edge = saved_left;

                /* Exit the loop since this is the end of the contiguous
                 * block.
                 */
                break;
            }

            /* Reset the two variables. */
            else
            {
                saved_left = 0;
                saved_right = 0;
            }

            /* Start on the next buffer. */
            buf_ptr = next_buffer;
        }

        /* Add the first left edge to the packet. */
        PUT32(buffer, offset + TCP_SACK_BLOCK_LEFT_EDGE, left_edge);

        /* Add the first right edge to the packet. */
        PUT32(buffer, offset + TCP_SACK_BLOCK_RIGHT_EDGE, right_edge);

        /* Increment the offset by one block. */
        offset += TCP_SACK_BLOCK_LENGTH;

        /* Add the remaining SACK blocks to the packet. */
        TCP_Build_SACK_Block(buffer, prt, &offset, *bytes_avail, left_edge,
                             right_edge);

        /* Set the length of the option. */
        buffer[TCP_SACK_LEN_OFFSET] = offset;

        /* Decrement the number of bytes available in this buffer. */
        *bytes_avail -= offset;
    }

    /* Return the number of bytes added to the packet. */
    return (offset);

} /* TCP_Build_SACK_Option */

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Build_DSACK_Option
*
*   DESCRIPTION
*
*       This function builds a D-SACK option in the provided buffer.
*
*   INPUTS
*
*       *buffer                 A pointer to the buffer in which to
*                               build the D-SACK option.
*       *prt                    The port for which to build the D-SACK
*                               option.
*       *bytes_avail            The number of bytes available in the TCP
*                               header for the D-SACK option.
*
*   OUTPUTS
*
*       bytes                   The number of bytes added to the packet.
*
*************************************************************************/
UINT8 TCP_Build_DSACK_Option(UINT8 *buffer, TCP_PORT *prt, UINT8 *bytes_avail)
{
    UINT8       offset = 0;

    /* If at least one SACK block will fit in the buffer. */
    if (*bytes_avail >= (TCP_SACK_LENGTH + TCP_SACK_BLOCK_LENGTH))
    {
        /* Set the "kind" field to the SACK type. */
        buffer[TCP_SACK_KIND_OFFSET] = TCP_SACK_OPT;

        /* Initialize the offset to the start of the first block. */
        offset = TCP_SACK_BLOCK_OFFSET;

        /* Add the first left edge to the packet. */
        PUT32(buffer, offset + TCP_SACK_BLOCK_LEFT_EDGE, prt->left_edge);

        /* Add the first right edge to the packet. */
        PUT32(buffer, offset + TCP_SACK_BLOCK_RIGHT_EDGE, prt->right_edge);

        /* Increment the offset by one block. */
        offset += TCP_SACK_BLOCK_LENGTH;

        /* Add the remaining SACK blocks to the packet. */
        TCP_Build_SACK_Block(buffer, prt, &offset, *bytes_avail, prt->left_edge,
                             prt->right_edge);

        /* Set the length of the option. */
        buffer[TCP_SACK_LEN_OFFSET] = offset;

        /* Decrement the number of bytes available in this buffer. */
        *bytes_avail -= offset;
    }

    /* Return the number of bytes added to the packet. */
    return (offset);

} /* TCP_Build_DSACK_Option */

#endif

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Build_MSS_Option
*
*   DESCRIPTION
*
*       This function builds an MSS option in the provided buffer.
*
*   INPUTS
*
*       *buffer                 A pointer to the buffer in which to
*                               build the MSS option.
*       mss                     The MSS value to add to the packet.
*       *bytes_avail            The number of bytes available in the TCP
*                               header for the D-SACK option.
*
*   OUTPUTS
*
*       bytes                   The number of bytes added to the packet.
*
*************************************************************************/
UINT8 TCP_Build_MSS_Option(UINT8 *buffer, UINT16 mss, UINT8 *bytes_avail)
{
    UINT8   new_bytes = TCP_MSS_LENGTH;

    /* If the option will fit in the packet. */
    if (*bytes_avail >= TCP_MSS_LENGTH)
    {
        /* Set the "kind" field to the SACK Permitted type. */
        buffer[TCP_MSS_KIND_OFFSET] = TCP_MSS_OPT;

        /* Set the length of the option. */
        buffer[TCP_MSS_LEN_OFFSET] = TCP_MSS_LENGTH;

        /* Add the MSS to the packet. */
        PUT16(buffer, TCP_MSS_VALUE_OFFSET, mss);

        /* Decrement the number of bytes remaining in this packet
         * for options.
         */
        *bytes_avail -= new_bytes;
    }

    /* There is no room in the packet for this option. */
    else
    {
        new_bytes = 0;
    }

    /* Return the number of bytes added to the packet. */
    return (new_bytes);

} /* TCP_Build_MSS_Option */

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Build_Timestamp_Option
*
*   DESCRIPTION
*
*       This function builds a TCP Timestamp option in the provided
*       buffer.
*
*   INPUTS
*
*       *buffer                 A pointer to the buffer in which to
*                               build the TCP Timestamp option.
*       *prt                    A pointer to the port for the connection
*                               for which this option is being built.
*       *bytes_avail            The number of bytes available in the TCP
*                               header for the Timestamp option.
*
*   OUTPUTS
*
*       bytes                   The number of bytes added to the packet.
*
*************************************************************************/
UINT8 TCP_Build_Timestamp_Option(UINT8 *buffer, TCP_PORT *prt,
                                 UINT8 *bytes_avail)
{
    UINT8   new_bytes = TCP_TIMESTAMP_LENGTH;

    TEST_TCP_Timestamp_Send(NU_Retrieve_Clock(), prt);

    /* If there is room for this option in the packet. */
    if (*bytes_avail >= TCP_TIMESTAMP_LENGTH)
    {
        /* Set the "kind" field to the Timestamp type. */
        buffer[TCP_TIMESTAMP_KIND_OFFSET] = TCP_TIMESTAMP_OPT;

        /* Set the length of the option. */
        buffer[TCP_TIMESTAMP_LEN_OFFSET] = TCP_TIMESTAMP_LENGTH;

        /* Store the current clock value in the outgoing packet. */
        PUT32(buffer, TCP_TIMESTAMP_TSVAL_OFFSET, NU_Retrieve_Clock());

        /* If this is an ACK, set the TS Echo Reply Value. */
        if (prt->out.tcp_flags & TACK)
        {
            /* Store the received TSval in TSecr. */
            PUT32(buffer, TCP_TIMESTAMP_TSECR_OFFSET, prt->p_tsecr);
        }

        /* Otherwise, set the value to zero. */
        else
        {
            PUT32(buffer, TCP_TIMESTAMP_TSECR_OFFSET, 0);
        }
    }

    /* This option will not fit in the packet. */
    else
    {
        new_bytes = 0;
    }

    /* Decrement the amount of memory available for TCP options. */
    *bytes_avail -= new_bytes;

    /* Return the number of bytes added to the packet. */
    return (new_bytes);

} /* TCP_Build_Timestamp_Option */

#endif
