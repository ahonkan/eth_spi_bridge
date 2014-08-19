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
*       zc_bl.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_ZC_Bytes_Left.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_ZC_Bytes_Left
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/************************************************************************
*
*   FUNCTION
*
*       NU_ZC_Bytes_Left
*
*   DESCRIPTION
*
*
*   INPUTS
*
*       *buf_ptr                Pointer to the head of the buffer chain.
*       *seg_ptr                Pointer to the segment within the buffer
*                               chain.
*       socketd                 The socket descriptor for the
*                               communication.
*
*   OUTPUTS
*
*       The number of bytes that will fit in the segment or zero if one
*       of the parameters is invalid.
*
*************************************************************************/
UINT32 NU_ZC_Bytes_Left(NET_BUFFER *buf_ptr, const NET_BUFFER *seg_ptr,
                        INT socketd)
{
    SOCKET_STRUCT   *sck_ptr;
    NET_BUFFER      *temp_ptr;
    UINT32          bytes_left = 0, total_bytes = 0, hdr_length;

    NU_SUPERV_USER_VARIABLES

    /* Verify that neither buffer provided is NULL */
    if ( (buf_ptr == NU_NULL) || (seg_ptr == NU_NULL) )
        return (0);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Validate the socket number. */
    if ( (socketd < 0) || (socketd >= NSOCKETS) ||
         (SCK_Sockets[socketd] == NU_NULL) )
    {
        NU_USER_MODE();

        return (0);
    }

    /* Get a pointer to the socket structure */
    sck_ptr = SCK_Sockets[socketd];

    /* If this is a TCP socket, the buffers must be traversed to determine
     * which buffer is being queried and return the correct number of bytes
     * so as not to exceed the MSS of the other side of the connection.
     */
    if (sck_ptr->s_protocol == NU_PROTO_TCP)
    {
        /* Verify that the connection is established */
        if ( (sck_ptr->s_port_index < 0) ||
             (TCP_Ports[sck_ptr->s_port_index]->state != SEST) )
            return (0);

        /* Get a pointer to the next buffer in the chain */
        temp_ptr = buf_ptr;

        /* While there are buffers in the chain */
        while (temp_ptr)
        {
            /* If this is the start of a new buffer chain in the chain of
             * buffers, compute the total_bytes that can fit in the
             * first buffer.
             */
            if (temp_ptr->mem_flags & NET_PARENT)
            {
                /* If the entire MSS of data will fit in this first packet. */
                if ((TCP_Ports[sck_ptr->s_port_index]->p_smss + TCP_HEADER_LEN +
                     TCP_Ports[sck_ptr->s_port_index]->p_opt_len) <=
                    NET_PARENT_BUFFER_SIZE)
                    total_bytes = TCP_Ports[sck_ptr->s_port_index]->p_smss;

                /* The MSS plus the TCP header exceeds the length of this buffer */
                else
                    total_bytes = NET_PARENT_BUFFER_SIZE - TCP_HEADER_LEN -
                        TCP_Ports[sck_ptr->s_port_index]->p_opt_len;

                /* If this is the target segment, break */
                if (temp_ptr == seg_ptr)
                {
                    bytes_left = total_bytes;
                    break;
                }
            }

            /* Otherwise, the target segment is not a parent buffer */
            else
            {
                /* If the number of bytes will not exceed the MSS of the other
                 * side, increment bytes_left by the total buffer size.
                 */
                if ( (total_bytes + NET_MAX_BUFFER_SIZE) <=
                     (TCP_Ports[sck_ptr->s_port_index]->p_smss) )
                    bytes_left = NET_MAX_BUFFER_SIZE;

                /* Otherwise, increment bytes_left by the amount that will not
                 * exceed the MSS of the other side.
                 */
                else
                    bytes_left =
                        (TCP_Ports[sck_ptr->s_port_index]->p_smss - total_bytes);

                /* If this is the target segment, break */
                if (temp_ptr == seg_ptr)
                    break;

                total_bytes += bytes_left;
            }

            /* Get a pointer to the next buffer in the chain */
            temp_ptr = temp_ptr->next_buffer;
        }
    }

    else
    {
        /* Get the length of the transport layer header */
        hdr_length = (sck_ptr->s_protocol == NU_PROTO_UDP) ? UDP_HEADER_LEN : 0;

        /* If this is the first header in the list */
        if (buf_ptr == seg_ptr)
            bytes_left = (NET_PARENT_BUFFER_SIZE - hdr_length);
        else
            bytes_left = NET_MAX_BUFFER_SIZE;
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (bytes_left);

} /* NU_ZC_Bytes_Left */
