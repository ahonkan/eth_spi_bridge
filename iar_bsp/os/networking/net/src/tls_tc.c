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
*       tls_tc.c
*
* DESCRIPTION
*
*       This file contains the implementation of TLS_TCP_Check.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       TLS_TCP_Check
*       TLS_Header_Memsum
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if !(TCPCHECK_ASM)
/*************************************************************************
*
*   FUNCTION
*
*       TLS_TCP_Check
*
*   DESCRIPTION
*
*       This function checksums a TCP header.
*
*   INPUTS
*
*       *pseudoheader           Pointer to header to be checksummed
*       *buf_ptr                Pointer to the net buffer list
*
*   OUTPUTS
*
*       UINT16                  Checksum value of structure
*
*************************************************************************/
UINT16 TLS_TCP_Check(UINT16 *pseudoheader, NET_BUFFER *buf_ptr)
{
    register UINT32 sum = 0;
    register UINT16 *pshdr = pseudoheader;
    UINT16 HUGE     *current_byte;
    NET_BUFFER      *temp_buf_ptr;
    UINT32          data_len;

    /*  This used to be a loop.  The loop was removed to save a few
        cycles.  The header length is always 6 16-bit words.  */
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr;

    /* If the flag is set indicating that the sum of the data was
     * computed during the copy routine.
     */
    if (buf_ptr->mem_flags & NET_BUF_SUM)
    {
        /* If there is data in the buffer that was not tallied during the
         * data copy, add it up now.  This will be the case for transport
         * layer headers that are prepended after the user data is added
         * to the buffer.
         */
        if ( (buf_ptr->sum_data_ptr) &&
             (buf_ptr->sum_data_ptr != buf_ptr->data_ptr) )
        {
            /* Add the data into the sum for computing the checksum. */
            sum += TLS_Header_Memsum(buf_ptr->data_ptr,
                (buf_ptr->sum_data_ptr - buf_ptr->data_ptr));
        }

        /* Add the tally of the data that was computed when the data
         * was copied into the buffer.
         */
        sum += buf_ptr->chk_sum;
    }

    /* Otherwise, compute the sum of the entire buffer chain. */
    else
    {
        /* Get a pointer to the buffer. */
        temp_buf_ptr = buf_ptr;

        /* Loop through the chain computing the checksum on each byte
           of data. */
        while (temp_buf_ptr)
        {
            /* Reset the data pointer. */
            current_byte    = (UINT16 *)temp_buf_ptr->data_ptr;
            data_len        = temp_buf_ptr->data_len;

            while (data_len >= 128)
            {
                data_len -= 128;

                sum += current_byte[0];
                sum += current_byte[1];
                sum += current_byte[2];
                sum += current_byte[3];
                sum += current_byte[4];
                sum += current_byte[5];
                sum += current_byte[6];
                sum += current_byte[7];
                sum += current_byte[8];
                sum += current_byte[9];
                sum += current_byte[10];
                sum += current_byte[11];
                sum += current_byte[12];
                sum += current_byte[13];
                sum += current_byte[14];
                sum += current_byte[15];
                sum += current_byte[16];
                sum += current_byte[17];
                sum += current_byte[18];
                sum += current_byte[19];
                sum += current_byte[20];
                sum += current_byte[21];
                sum += current_byte[22];
                sum += current_byte[23];
                sum += current_byte[24];
                sum += current_byte[25];
                sum += current_byte[26];
                sum += current_byte[27];
                sum += current_byte[28];
                sum += current_byte[29];
                sum += current_byte[30];
                sum += current_byte[31];
                sum += current_byte[32];
                sum += current_byte[33];
                sum += current_byte[34];
                sum += current_byte[35];
                sum += current_byte[36];
                sum += current_byte[37];
                sum += current_byte[38];
                sum += current_byte[39];
                sum += current_byte[40];
                sum += current_byte[41];
                sum += current_byte[42];
                sum += current_byte[43];
                sum += current_byte[44];
                sum += current_byte[45];
                sum += current_byte[46];
                sum += current_byte[47];
                sum += current_byte[48];
                sum += current_byte[49];
                sum += current_byte[50];
                sum += current_byte[51];
                sum += current_byte[52];
                sum += current_byte[53];
                sum += current_byte[54];
                sum += current_byte[55];
                sum += current_byte[56];
                sum += current_byte[57];
                sum += current_byte[58];
                sum += current_byte[59];
                sum += current_byte[60];
                sum += current_byte[61];
                sum += current_byte[62];
                sum += current_byte[63];

                current_byte += 64;
            }

            if (data_len >= 64)
            {
                data_len -= 64;

                sum += current_byte[0];
                sum += current_byte[1];
                sum += current_byte[2];
                sum += current_byte[3];
                sum += current_byte[4];
                sum += current_byte[5];
                sum += current_byte[6];
                sum += current_byte[7];
                sum += current_byte[8];
                sum += current_byte[9];
                sum += current_byte[10];
                sum += current_byte[11];
                sum += current_byte[12];
                sum += current_byte[13];
                sum += current_byte[14];
                sum += current_byte[15];
                sum += current_byte[16];
                sum += current_byte[17];
                sum += current_byte[18];
                sum += current_byte[19];
                sum += current_byte[20];
                sum += current_byte[21];
                sum += current_byte[22];
                sum += current_byte[23];
                sum += current_byte[24];
                sum += current_byte[25];
                sum += current_byte[26];
                sum += current_byte[27];
                sum += current_byte[28];
                sum += current_byte[29];
                sum += current_byte[30];
                sum += current_byte[31];

                current_byte += 32;
            }

            if (data_len >= 32)
            {
                data_len -= 32;

                sum += current_byte[0];
                sum += current_byte[1];
                sum += current_byte[2];
                sum += current_byte[3];
                sum += current_byte[4];
                sum += current_byte[5];
                sum += current_byte[6];
                sum += current_byte[7];
                sum += current_byte[8];
                sum += current_byte[9];
                sum += current_byte[10];
                sum += current_byte[11];
                sum += current_byte[12];
                sum += current_byte[13];
                sum += current_byte[14];
                sum += current_byte[15];

                current_byte += 16;
            }

            if (data_len >= 16)
            {
                data_len -= 16;

                sum += current_byte[0];
                sum += current_byte[1];
                sum += current_byte[2];
                sum += current_byte[3];
                sum += current_byte[4];
                sum += current_byte[5];
                sum += current_byte[6];
                sum += current_byte[7];

                current_byte += 8;
            }

            if (data_len >= 8)
            {
                data_len -= 8;

                sum += current_byte[0];
                sum += current_byte[1];
                sum += current_byte[2];
                sum += current_byte[3];

                current_byte += 4;
            }

            if (data_len >= 4)
            {
                data_len -= 4;

                sum += current_byte[0];
                sum += current_byte[1];

                current_byte += 2;
            }

            if (data_len >= 2)
            {
                data_len -= 2;

                sum += current_byte[0];

                current_byte++;
            }

            if (data_len)
            {
                sum += current_byte[0] & INTSWAP(0xFF00);
            }

            /* Point to the next buffer. */
            temp_buf_ptr = temp_buf_ptr->next_buffer;
        }
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (INTSWAP((UINT16)~sum));

} /* TLS_TCP_Check */
#endif /* !(TCPCHECK_ASM) */

/*************************************************************************
*
*   FUNCTION
*
*       TLS_Header_Memsum
*
*   DESCRIPTION
*
*       This function computes the sum of n bytes of data in the
*       incoming buffer.  This routine is used to tally up the
*       sum of the data in the header of an outgoing packet.
*
*   INPUTS
*
*       *s                      The buffer of data to tally.
*       n                       The number of bytes to add up.
*
*   OUTPUTS
*
*       UINT32                  The sum of the n bytes of data.
*
*************************************************************************/
UINT32 TLS_Header_Memsum(VOID *s, UINT32 n)
{
    UINT32 data_len;
    UINT16 *src;
    UINT32 sum = 0;

    /* Store the value of half the total data in the buffer. */
    data_len = n >> 1;

    /* Store a pointer to the source buffer. */
    src = (UINT16*)s;

    /* While there are at least 16 bytes in the buffer. */
    while (data_len >= 16)
    {
        data_len -= 16;

        sum += src[0];
        sum += src[1];
        sum += src[2];
        sum += src[3];
        sum += src[4];
        sum += src[5];
        sum += src[6];
        sum += src[7];
        sum += src[8];
        sum += src[9];
        sum += src[10];
        sum += src[11];
        sum += src[12];
        sum += src[13];
        sum += src[14];
        sum += src[15];

        src += 16;
    }

    /* If there are at least 8 bytes in the buffer. */
    if (data_len >= 8)
    {
        data_len -= 8;

        sum += src[0];
        sum += src[1];
        sum += src[2];
        sum += src[3];
        sum += src[4];
        sum += src[5];
        sum += src[6];
        sum += src[7];

        src += 8;
    }

    /* If there are at least 4 bytes in the buffer. */
    if (data_len >= 4)
    {
        data_len -= 4;

        sum += src[0];
        sum += src[1];
        sum += src[2];
        sum += src[3];

        src += 4;
    }

    /* If there are at least 2 bytes in the buffer. */
    if (data_len >= 2)
    {
        data_len -= 2;

        sum += src[0];
        sum += src[1];

        src += 2;
    }

    /* Sum up the remaining bytes. */
    while (data_len)
    {
        sum += *src++;
        data_len--;
    }

    /* If the data length is odd. */
    if (n & 0x01)
    {
         sum += src[0] & INTSWAP(0xFF00);
    }

    return (sum);

} /* TLS_Header_Memsum */

