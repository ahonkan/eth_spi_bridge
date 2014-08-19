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
*       mem_cc.c
*
* DESCRIPTION
*
*       This file contains the implementation of MEM_Chain_Copy.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       MEM_Chain_Copy
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
*       MEM_Chain_Copy
*
*   DESCRIPTION
*
*       This function copies a memory chain.  It starts at an offset
*       of the dest->data_len value of the destination buffer, and the
*       offset passed in for the source buffer.  Both the destination
*       and source buffers data_ptr parameter must be pointing to
*       the head of the data buffer.
*
*   INPUTS
*
*       *dest                   Pointer to the net buffer to copy to
*       *src                    Pointer to the net buffer to copy from
*       off                     The offset in the net buffer
*       len                     The amount to copy
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID MEM_Chain_Copy(NET_BUFFER *dest, NET_BUFFER *src, INT32 off, INT32 len)
{
    NET_BUFFER      *s = src;
    NET_BUFFER      *d = dest;
    INT             first_buffer = 0;
    INT32           bytes_to_copy;
    INT32           data_off = 0;
    INT32           max_buf_size;

    if ((off < 0) || (len < 0) || (d == NU_NULL) || (s == NU_NULL))
        return;

    /* First we need to find the buffer within the chain where the first byte
       specified by offset is found. */
    while ( (off > 0) && (s) )
    {
        /* Is this the one. */
        if (off < (INT)s->data_len)
            break;

        /* Now check the next one. */
        off -= (INT32)s->data_len;
        s = s->next_buffer;
    }

    while ((len > 0) && d && s)
    {
        /* Choose the min of the data in the source and the total data length
           we wish to copy. */
        if (len < (INT)s->data_len - off)
        {

            /* Set the maximum buffer size */
            if (first_buffer == 0)
            {
                max_buf_size = NET_PARENT_BUFFER_SIZE;
            }
            else
            {
                max_buf_size = NET_MAX_BUFFER_SIZE;
            }

            /* Check to see if data is already in buffer */
            if (d->data_len > 0)
            {
                /* Set the offset past the data already in the buffer */
                data_off = (INT32)d->data_len;

                /* Ensure that the data being copied in does not exceed */
                /* the max buffer size */
                if (((INT32)(d->data_len) + len) > max_buf_size)
                {
                    /* Set the bytes to copy and the data_len */
                    bytes_to_copy = max_buf_size - (INT32)d->data_len;
                    d->data_len = (UINT32)max_buf_size;
                }
                else
                {
                    /* Set the bytes to copy and the data_len */
                    d->data_len = d->data_len + (UINT32)len;
                    bytes_to_copy = len;
                }
            }
            else
            {
                /* Set the bytes to copy and the data_len */
                d->data_len = (UINT32)len;
                bytes_to_copy = (INT32)d->data_len;
            }
        }
        else
        {
            /* Check to see if there is data already in the buffer */
            if (d->data_len > 0)
            {
                /* Set the offset past the data in the buffer */
                data_off = (INT32)d->data_len;

                /* Set the max buffer size */
                if (first_buffer == 0)
                    max_buf_size = NET_PARENT_BUFFER_SIZE;
                else
                    max_buf_size = NET_MAX_BUFFER_SIZE;

                /* Ensure that the data being copied in does not exceed */
                /* the max buffer size */
                if ((d->data_len + s->data_len) > (UINT32)max_buf_size)
                {
                    /* Set the bytes to copy and the data_len */
                    bytes_to_copy = max_buf_size - (INT32)d->data_len;

                    if (((INT32)s->data_len - off) < bytes_to_copy)
                    {
                        bytes_to_copy = (INT32)s->data_len - off;
                    }
                    d->data_len = d->data_len + (UINT32)bytes_to_copy;
                }
                else
                {
                    /* Set the bytes to copy and the data_len */
                    bytes_to_copy = (INT32)s->data_len;

                    if (((INT32)s->data_len - off) < bytes_to_copy)
                    {
                        bytes_to_copy = (INT32)s->data_len - off;
                    }
                    d->data_len = d->data_len + (UINT32)bytes_to_copy;
                }
            }
            else
            {
                /* Set the bytes to copy and the data_len */
                d->data_len = (s->data_len - (UINT32)off);
                bytes_to_copy = (INT32)d->data_len;
            }
        }

        /* Decrement the total data left to copy in */
        len -= bytes_to_copy;

        /* Copy the data to the destination */
        NU_BLOCK_COPY(d->data_ptr + data_off, s->data_ptr + off,
                      (unsigned int)bytes_to_copy);

        /* Set the max buffer size */
        if (first_buffer == 0)
            max_buf_size = NET_PARENT_BUFFER_SIZE;
        else
            max_buf_size = NET_MAX_BUFFER_SIZE;

        /* Done copying data for this fragment */
        if (len == 0)
        {
            /* Free any remaining unused destination buffers */
            MEM_One_Buffer_Chain_Free(d->next_buffer, &MEM_Buffer_Freelist);

            d->next_buffer = NU_NULL;
        }
        else if (d->data_len >= (UINT32)max_buf_size)
        {
            /* Advance to next dest buffer */
            d = d->next_buffer;
            d->data_ptr = d->mem_packet;
            d->data_len = 0;
            first_buffer = 1;
        }

        /* Advance to next source buffer */
        if ((UINT32)(off + bytes_to_copy) >= s->data_len)
        {
            s = s->next_buffer;
            off = 0;
        }
        else
        {
            off = off + bytes_to_copy;
        }

        data_off = 0;

    }

} /* MEM_Chain_Copy */
