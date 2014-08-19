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
*       mem_t.c
*
* DESCRIPTION
*
*       This file contains the implementation of MEM_Trim.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       MEM_Trim
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
*       MEM_Trim
*
*   DESCRIPTION
*
*       This function trims data from the start of an buffer chain.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the net buffer to trim
*       length                  The amount to trim
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID MEM_Trim(NET_BUFFER *buf_ptr, INT32 length)
{
    INT32       t_len = length;
    NET_BUFFER  *m;
    INT32       count;
    INT         old_level;
    UINT32      trim_bytes;
    UINT16      *src;

    /* Perform some basic error checking. */
    if (buf_ptr == NU_NULL)
        return;

    m = buf_ptr;

    /*  Temporarily lockout interrupts to protect the global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* If length is greater than 0 then trim data from the start. */
    if (t_len >= 0)
    {
        /* Start with the first buffer in the chain and remove data as necessary. */
        while (m != NU_NULL && t_len > 0)
        {
            if (m->data_len <= (UINT32)t_len)
            {
                t_len -= (INT32)m->data_len;
                m->data_len = 0;
                m = m->next_buffer;
            }
            else
            {
                /* If the flag is set indicating that this buffer chain
                 * contains the running summation for the checksum.
                 */
                if (buf_ptr->mem_flags & NET_BUF_SUM)
                {
                    /* Get 1/2 of the data that is being removed. */
                    trim_bytes = t_len >> 1;

                    /* Get a pointer to the data being trimmed. */
                    src = (UINT16*)m->data_ptr;

                    /* Adjust the running sum to account for the data
                     * being removed.
                     */
                    while (trim_bytes)
                    {
                        /* Remove these two bytes from the sum. */
                        buf_ptr->chk_sum -= src[0];

                        /* Move to the next two bytes. */
                        src ++;

                        /* Decrement trim_bytes. */
                        trim_bytes --;
                    }

                    /* If the data length being trimmed was odd, there is one
                     * more byte of data that needs to be removed from the sum.
                     */
                    if (t_len & 0x01)
                    {
                        buf_ptr->chk_sum -= src[0] & INTSWAP(0xFF00);
                    }
                }

                m->data_len -= (UINT32)t_len;
                m->data_ptr += t_len;
                t_len = 0;
            }
        }

        /* Update the total number of bytes in this packet. */
        buf_ptr->mem_total_data_len -= (UINT32)(length - t_len);
    }
    else
    {
        t_len = -t_len;
        count = 0;

        /* Get a count of the total number of bytes in this chain. */
        for(;;)
        {
            count += (INT32)(m->data_len);

            if (m->next_buffer == NU_NULL)
                break;

            m = m->next_buffer;
        }

        /* If the adjustment only affects the last buffer in the chain, make the
           adjustment and return. */
        if (m->data_len >= (UINT32)t_len)
        {
            m->data_len -= (UINT32)t_len;
            buf_ptr->mem_total_data_len -= (UINT32)t_len;

            /*  Restore the previous interrupt lockout level.  */
            NU_Local_Control_Interrupts(old_level);
            return;
        }

        count -= t_len;
        if (count < 0)
            count = 0;

        /* The correct length for the chain is "count". */
        m = buf_ptr;
        m->mem_total_data_len = (UINT32)count;

        /* Find the buffer with the last data.  Adjust its length. */
        for (; m ; m = m->next_buffer)
        {
            if (m->data_len >= (UINT32)count)
            {
                m->data_len = (UINT32)count;
                break;
            }
            count -= (INT32)m->data_len;
        }

        /* Toss out the data from the remaining buffers. */
        while ( (m!= NU_NULL) && ((m = m->next_buffer) != NU_NULL))
            m->data_len = 0;
    }

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

} /* MEM_Trim */
