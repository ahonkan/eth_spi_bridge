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
*       utl.c
*
*   COMPONENT
*
*       UTL -- Net utility routines
*
*   DESCRIPTION
*
*       This file contains low level routines commonly used by
*       the Net stack.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       UTL_Checksum
*       UTL_Rand
*       UTL_Sum_Memcpy
*       UTL_Sum_Memcpy_Aligned
*       UTL_Sum_Memcpy_Misaligned
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include <stdlib.h>

VOID* UTL_Sum_Memcpy_Misaligned(VOID*, const VOID*, UINT32 , UINT32*);
VOID* UTL_Sum_Memcpy_Aligned(VOID*, const VOID*, UINT32 , UINT32*);

/*************************************************************************
*
*   FUNCTION
*
*       UTL_Checksum
*
*   DESCRIPTION
*
*       Calculate a TCP checksum.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the net buffer list
*       source                  The source to check the sum on
*       dest                    Where the information is going
*       protocol                The protocol on the information
*
*   OUTPUTS
*
*       UINT16
*
*************************************************************************/
UINT16 UTL_Checksum(NET_BUFFER *buf_ptr, UINT32 source,
                    UINT32 dest, UINT8 protocol)
{
    struct pseudotcp     tcp_chk;

    tcp_chk.source  = LONGSWAP(source);
    tcp_chk.dest    = LONGSWAP(dest);
    tcp_chk.z       = 0;
    tcp_chk.proto   = protocol;
    tcp_chk.tcplen  = INTSWAP((UINT16)buf_ptr->mem_total_data_len);

    return(TLS_TCP_Check( (UINT16 *) &tcp_chk, buf_ptr));

} /* UTL_Checksum */

/*************************************************************************
*
*   FUNCTION
*
*       UTL_Rand
*
*   DESCRIPTION
*
*       Function handles generating a random number.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       Returns the Random Number.
*
*************************************************************************/
UINT32 UTL_Rand(VOID)
{
    /* Ensure random number generator is seeded */
    NU_RTL_Rand_Seed();

    /* Return random number */
    return ((UINT32)rand());
} /* UTL_Rand */

/*************************************************************************
*
*   FUNCTION
*
*       UTL_Sum_Memcpy
*
*   DESCRIPTION
*
*       Function copies data from one buffer to another and computes the
*       running sum of the data to be used in the checksum calculation of
*       that data.
*
*   INPUTS
*
*       *d                      A pointer to the destination buffer into
*                               which to copy the data.
*       *s                      A pointer to the source buffer from which
*                               to copy the data.
*       n                       The total length of the source buffer.
*       *data_sum               The sum of the data in the source buffer.
*
*   OUTPUTS
*
*       Returns a pointer to the destination buffer.
*
*************************************************************************/
VOID* UTL_Sum_Memcpy(VOID *d, const VOID *s, UINT32 n, UINT32 *data_sum)
{
    /* If the source address is not aligned to a 2 byte boundary, call
       the function which handles "mis-aligned" data. */
    if (((UNSIGNED)s & 0x01) != 0)
        return UTL_Sum_Memcpy_Misaligned(d, s, n, data_sum);
    else
        return UTL_Sum_Memcpy_Aligned(d, s, n, data_sum);

} /* UTL_Sum_Memcpy */

/*************************************************************************
*
*   FUNCTION
*
*       UTL_Sum_Memcpy_Aligned
*
*   DESCRIPTION
*
*       Function copies data from one buffer to another and computes the
*       running sum of the data to be used in the checksum calculation of
*       that data. This is a utility function which is internally called
*       by "UTL_Sum_Memcpy" and is used for aligned source addresses.
*
*   INPUTS
*
*       *d                      A pointer to the destination buffer into
*                               which to copy the data.
*       *s                      A pointer to the source buffer from which
*                               to copy the data.
*       n                       The total length of the source buffer.
*       *data_sum               The sum of the data in the source buffer.
*
*   OUTPUTS
*
*       Returns a pointer to the destination buffer.
*
*************************************************************************/
VOID* UTL_Sum_Memcpy_Aligned(VOID *d, const VOID *s, UINT32 n,
                             UINT32 *data_sum)
{
    UINT32      n16, sum = 0;
    UINT16      *src;
    UINT16      *dest;

    /* Set up pointers to the source and destination buffers. */
    src  = (UINT16*)s;
    dest = (UINT16*)d;

    /* Compute half the length of the data to copy. */
    n16 = n >> 1;

    /* While there is more than 64 bytes to copy. */
    while (n16 >= 64)
    {
        /* Copy the 128 bytes of data. */
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        dest[4] = src[4];
        dest[5] = src[5];
        dest[6] = src[6];
        dest[7] = src[7];
        dest[8] = src[8];
        dest[9] = src[9];
        dest[10] = src[10];
        dest[11] = src[11];
        dest[12] = src[12];
        dest[13] = src[13];
        dest[14] = src[14];
        dest[15] = src[15];
        dest[16] = src[16];
        dest[17] = src[17];
        dest[18] = src[18];
        dest[19] = src[19];
        dest[20] = src[20];
        dest[21] = src[21];
        dest[22] = src[22];
        dest[23] = src[23];
        dest[24] = src[24];
        dest[25] = src[25];
        dest[26] = src[26];
        dest[27] = src[27];
        dest[28] = src[28];
        dest[29] = src[29];
        dest[30] = src[30];
        dest[31] = src[31];
        dest[32] = src[32];
        dest[33] = src[33];
        dest[34] = src[34];
        dest[35] = src[35];
        dest[36] = src[36];
        dest[37] = src[37];
        dest[38] = src[38];
        dest[39] = src[39];
        dest[40] = src[40];
        dest[41] = src[41];
        dest[42] = src[42];
        dest[43] = src[43];
        dest[44] = src[44];
        dest[45] = src[45];
        dest[46] = src[46];
        dest[47] = src[47];
        dest[48] = src[48];
        dest[49] = src[49];
        dest[50] = src[50];
        dest[51] = src[51];
        dest[52] = src[52];
        dest[53] = src[53];
        dest[54] = src[54];
        dest[55] = src[55];
        dest[56] = src[56];
        dest[57] = src[57];
        dest[58] = src[58];
        dest[59] = src[59];
        dest[60] = src[60];
        dest[61] = src[61];
        dest[62] = src[62];
        dest[63] = src[63];

        /* Compute the sum of these 128 bytes.  Since src is a UINT16*,
         * we are tallying two bytes with each operation.
         */
        sum += src[0] + src[1] + src[2] + src[3] + src[4] +
               src[5] + src[6] + src[7] + src[8] + src[9] +
               src[10] + src[11] + src[12] + src[13] + src[14] +
               src[15] + src[16] + src[17] + src[18] + src[19] +
               src[20] + src[21] + src[22] + src[23] + src[24] +
               src[25] + src[26] + src[27] + src[28] + src[29] +
               src[30] + src[31] + src[32] + src[33] + src[34] +
               src[35] + src[36] + src[37] + src[38] + src[39] +
               src[40] + src[41] + src[42] + src[43] + src[44] +
               src[45] + src[46] + src[47] + src[48] + src[49] +
               src[50] + src[51] + src[52] + src[53] + src[54] +
               src[55] + src[56] + src[57] + src[58] + src[59] +
               src[60] + src[61] + src[62] + src[63];

        /* Increment the source and destination pointers. */
        dest += 64;
        src += 64;

        /* Update the outstanding data. */
        n16 -= 64;
    }

    /* If there is more than 32 bytes to copy. */
    if (n16 >= 32)
    {
        /* Copy the 64 bytes of data. */
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        dest[4] = src[4];
        dest[5] = src[5];
        dest[6] = src[6];
        dest[7] = src[7];
        dest[8] = src[8];
        dest[9] = src[9];
        dest[10] = src[10];
        dest[11] = src[11];
        dest[12] = src[12];
        dest[13] = src[13];
        dest[14] = src[14];
        dest[15] = src[15];
        dest[16] = src[16];
        dest[17] = src[17];
        dest[18] = src[18];
        dest[19] = src[19];
        dest[20] = src[20];
        dest[21] = src[21];
        dest[22] = src[22];
        dest[23] = src[23];
        dest[24] = src[24];
        dest[25] = src[25];
        dest[26] = src[26];
        dest[27] = src[27];
        dest[28] = src[28];
        dest[29] = src[29];
        dest[30] = src[30];
        dest[31] = src[31];

        /* Compute the sum of these 64 bytes. */
        sum += src[0] + src[1] + src[2] + src[3] + src[4] +
               src[5] + src[6] + src[7] + src[8] + src[9] +
               src[10] + src[11] + src[12] + src[13] + src[14] +
               src[15] + src[16] + src[17] + src[18] + src[19] +
               src[20] + src[21] + src[22] + src[23] + src[24] +
               src[25] + src[26] + src[27] + src[28] + src[29] +
               src[30] + src[31];

        /* Increment the source and destination pointers. */
        dest += 32;
        src += 32;

        /* Update the outstanding data. */
        n16 -= 32;
    }

    /* If there is more than 16 bytes to copy. */
    if (n16 >= 16)
    {
        /* Copy the 32 bytes of data. */
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        dest[4] = src[4];
        dest[5] = src[5];
        dest[6] = src[6];
        dest[7] = src[7];
        dest[8] = src[8];
        dest[9] = src[9];
        dest[10] = src[10];
        dest[11] = src[11];
        dest[12] = src[12];
        dest[13] = src[13];
        dest[14] = src[14];
        dest[15] = src[15];

        /* Compute the sum of these 32 bytes. */
        sum += src[0] + src[1] + src[2] + src[3] + src[4] +
               src[5] + src[6] + src[7] + src[8] + src[9] +
               src[10] + src[11] + src[12] + src[13] + src[14] +
               src[15];

        /* Increment the source and destination pointers. */
        dest += 16;
        src += 16;

        /* Update the outstanding data. */
        n16 -= 16;
    }

    /* If there is more than 8 bytes to copy. */
    if (n16 >= 8)
    {
        /* Copy the 16 bytes of data. */
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        dest[4] = src[4];
        dest[5] = src[5];
        dest[6] = src[6];
        dest[7] = src[7];

        /* Compute the sum of these 16 bytes. */
        sum += src[0] + src[1] + src[2] + src[3] + src[4] +
               src[5] + src[6] + src[7];

        /* Increment the source and destination pointers. */
        dest += 8;
        src += 8;

        /* Update the outstanding data. */
        n16 -= 8;
    }

    /* If there is more than 4 bytes of data to copy. */
    if (n16 >= 4)
    {
        /* Copy the 8 bytes of data. */
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];

        /* Compute the sum of these 8 bytes. */
        sum += src[0] + src[1] + src[2] + src[3];

        /* Increment the source and destination pointers. */
        dest += 4;
        src += 4;

        /* Update the outstanding data. */
        n16 -= 4;
    }

    /* Copy the final bytes, two at a time. */
    while (n16)
    {
        /* Copy two bytes of data. */
        dest[0] = src[0];

        /* Add these bytes of data to the total sum. */
        sum += src[0];

        /* Increment the source and destination pointers. */
        dest ++;
        src ++;

        /* Update the outstanding data. */
        n16 --;
    }

    /* If there is one byte of data remaining. */
    if (n & 0x01)
    {
        *dest = *src;
        sum += src[0] & INTSWAP(0xFF00);
    }

    /* Return the sum of the data. */
    *data_sum = sum;

    return (d);

} /* UTL_Sum_Memcpy_Aligned */

/*************************************************************************
*
*   FUNCTION
*
*       UTL_Sum_Memcpy_Misaligned
*
*   DESCRIPTION
*
*       Function copies data from one buffer to another and computes the
*       running sum of the data to be used in the checksum calculation of
*       that date. This is a utility function which is internally called
*       by "UTL_Sum_Memcpy" and is used for mis-aligned source addresses.
*
*   INPUTS
*
*       *d                      A pointer to the destination buffer into
*                               which to copy the data.
*       *s                      A pointer to the source buffer from which
*                               to copy the data.
*       n                       The total length of the source buffer.
*       *data_sum               The sum of the data in the source buffer.
*
*   OUTPUTS
*
*       Returns a pointer to the destination buffer.
*
*************************************************************************/
VOID* UTL_Sum_Memcpy_Misaligned(VOID *d, const VOID *s, UINT32 n,
                                UINT32 *data_sum)
{
    /* Copy data from the source to the destination. */
    NU_BLOCK_COPY(d, s, n);

    /* Calculate the checksum of the copied data. */
    *data_sum = TLS_Header_Memsum(d, n);

    return (d);

} /* UTL_Sum_Memcpy_Misaligned */
