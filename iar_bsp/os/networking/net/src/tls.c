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
*       tls.c
*
*   DESCRIPTION
*
*       This file contains misc. "tools" used by the networking stack.
*       Also, all source code for the PUT/GET macros used to
*       access packet data is contained in this module.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TLS_Normalize_Ptr
*       TLS_Get64
*       TLS_Put64
*       TLS_Get32
*       TLS_Put32
*       TLS_Get16
*       TLS_Put16
*       TLS_IP_Address
*       TLS_Put_String
*       TLS_Get_String
*       TLS_Eq_String
*       TLS_IP_Check
*       TLS_Longswap
*       TLS_Intswap
*       TLS_Comparen
*       TLS_IP_Check_Buffer_Chain
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/* NORM_PTR is defined in target.h */
#ifdef NORM_PTR
#include <dos.h>
#endif

/*************************************************************************
*
*   FUNCTION
*
*       TLS_Normalize_Ptr
*
*   DESCRIPTION
*
*       This function takes a pointer for segmented memory architectures
*       like the 80386 in real mode and normalizes that pointer.  It
*       is not necessary on a flat memory model architecture.
*
*   INPUTS
*
*       *ptr                    The pointer to normalize
*
*   OUTPUTS
*
*       ptr
*
************************************************************************/
VOID *TLS_Normalize_Ptr(VOID *ptr)
{
#ifdef NORM_PTR

    unsigned long temp_address;

    temp_address = FP_SEG(ptr);
    temp_address = (temp_address << 4) + FP_OFF(ptr);
    FP_SEG(ptr) =  (UINT16)(temp_address >> 4);
    FP_OFF(ptr) =  (UINT16)(temp_address & 0xf);

#endif

    return (ptr);

} /* TLS_Normalize_Ptr */

/*************************************************************************
*
*   FUNCTION
*
*       TLS_Get64
*
*   DESCRIPTION
*
*       This function takes a memory area and an offset into the area. At
*       this offset, 64 bits will be read and returned in the target
*       hardware byte order.
*
*   INPUTS
*
*       unsigned char *ptr      Pointer to the start of the memory area.
*       unsigned int  offset    Offset into memory area to get the 32bits
*                               from.
*
*   OUTPUTS
*
*       unsigned long long      The 64 bits that were read.
*
************************************************************************/
unsigned long long TLS_Get64(unsigned char *ptr, unsigned int offset)
{
    unsigned char *p = ptr + offset;

    return ((unsigned long long)p[0] << 56) +
           ((unsigned long long)p[1] << 48) +
           ((unsigned long long)p[2] << 40) +
           ((unsigned long long)p[3] << 32) +
           ((unsigned long long)p[4] << 24) +
           ((unsigned long long)p[5] << 16) +
           ((unsigned long long)p[6] << 8) +
            (unsigned long long)p[7];

} /* TLS_Get64 */

/*************************************************************************
*
*   FUNCTION
*
*       TLS_Put64
*
*   DESCRIPTION
*
*       This function takes a memory area and an offset into the area. At
*       this offset, 64bits will be written in network byte order.
*
*   INPUTS
*
*       unsigned char *ptr      Pointer to the start of the memory area.
*       unsigned int  offset    Offset into memory area to put the 64bits
*                               from.
*       unsigned long value     64bits to be written to memory
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID TLS_Put64(unsigned char *ptr, unsigned int offset, long long value)
{
    unsigned char *p = ptr + offset;

    *p++ = (unsigned char)(value >> 56);
    *p++ = (unsigned char)(value >> 48);
    *p++ = (unsigned char)(value >> 40);
    *p++ = (unsigned char)(value >> 32);
    *p++ = (unsigned char)(value >> 24);
    *p++ = (unsigned char)(value >> 16);
    *p++ = (unsigned char)(value >> 8);
    *p = (unsigned char)value;

} /* TLS_Put64 */

/*************************************************************************
*
*   FUNCTION
*
*       TLS_Get32
*
*   DESCRIPTION
*
*       This function takes a memory area and an offset into the area. At
*       this offset, 32bits will be read and returned in the target
*       hardware byte order.
*
*   INPUTS
*
*       unsigned char *ptr      Pointer to the start of the memory area.
*       unsigned int  offset    Offset into memory area to get the 32bits
*                               from.
*
*   OUTPUTS
*
*       unsigned long           The 32bits that were read.
*
************************************************************************/
unsigned long TLS_Get32(unsigned char *ptr, unsigned int offset)
{
    unsigned char *p = ptr + offset;

    return ((unsigned long)p[0] << 24) +
           ((unsigned long)p[1] << 16) +
           ((unsigned long)p[2] << 8) +
            (unsigned long)p[3];

} /* TLS_Get32 */

/*************************************************************************
*
*   FUNCTION
*
*       TLS_Put32
*
*   DESCRIPTION
*
*       This function takes a memory area and an offset into the area. At
*       this offset, 32bits will be written in network byte order.
*
*   INPUTS
*
*       unsigned char *ptr      Pointer to the start of the memory area.
*       unsigned int  offset    Offset into memory area to get the 32bits
*                               from.
*       unsigned long value     32bits to be written to memory
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID TLS_Put32(unsigned char *ptr, unsigned int offset, unsigned long value)
{
unsigned char *p = ptr + offset;

    *p++ = (unsigned char)(value >> 24);
    *p++ = (unsigned char)(value >> 16);
    *p++ = (unsigned char)(value >> 8);
    *p = (unsigned char)value;

} /* TLS_Put32 */

/*************************************************************************
*
*   FUNCTION
*
*       TLS_Get16
*
*   DESCRIPTION
*
*       This function takes a memory area and an offset into the area. At
*       this offset, 16bits will be read and returned in the target
*       hardware byte order.
*
*   INPUTS
*
*       unsigned char *ptr      Pointer to the start of the memory area.
*       unsigned int  offset    Offset into memory area to get the 32bits
*                               from.
*
*   OUTPUTS
*
*       unsigned short           The 16bits that were read.
*
************************************************************************/
unsigned short TLS_Get16(unsigned char *ptr, unsigned int offset)
{
    unsigned char *p = ptr + offset;

    return (unsigned short)((p[0] << 8) + p[1]);

} /* TLS_Get16 */

/*************************************************************************
*
*   FUNCTION
*
*       TLS_Put16
*
*   DESCRIPTION
*
*       This function takes a memory area and an offset into the area. At
*       this offset, 16bits will be written in network byte order.
*
*   INPUTS
*
*       unsigned char *ptr      Pointer to the start of the memory area.
*       unsigned int  offset    Offset into memory area to get the 32bits
*                               from.
*       unsigned short value    16bits to be written to memory
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID TLS_Put16(unsigned char *ptr, unsigned int offset, unsigned short value)
{
    unsigned char *p = ptr + offset;

    *p++ = (unsigned char)(value >> 8);
    *p = (unsigned char)value;

} /* TLS_Put16 */

/*************************************************************************
*
*   FUNCTION
*
*       TLS_IP_Address
*
*   DESCRIPTION
*
*       This function converts an IP address that is stored in a 4 byte
*       array into a 32bit form of the IP address, note that correct
*       byte ordering for the target hardware is maintained.
*
*   INPUTS
*
*       unsigned char *p        Pointer to the array storing an IP
*                               address
*
*   OUTPUTS
*
*       unsigned long           32bits form of the IP address
*
************************************************************************/
unsigned long TLS_IP_Address(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) +
           ((unsigned long)p[1] << 16) +
           ((unsigned long)p[2] << 8) +
            (unsigned long)p[3];

} /* TLS_IP_Address */

/*************************************************************************
*
*   FUNCTION
*
*       TLS_Put_String
*
*   DESCRIPTION
*
*       This function writes a string out to memory.
*
*   INPUTS
*
*       unsigned char *dest     Destination address for the string
*       unsigned int   offset   Offset from the destination to start
*                               writing
*       unsigned char *src      Source address of the string to write
*       unsigned int  size      Length of the string to write
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID *TLS_Put_String(unsigned char *dest, unsigned int offset,
                     const unsigned char *src, unsigned int size)
{
    return (memcpy((dest + offset), src, size));

} /* TLS_Put_String */

/*************************************************************************
*
*   FUNCTION
*
*       TLS_Get_String
*
*   DESCRIPTION
*
*       This function reads a string of specified length from memory.
*
*   INPUTS
*
*       unsigned char *src      Source address of the string to read
*       unsigned int   offset   Offset from the source address to
*                               start reading
*       unsigned char *dest     Destination address for the string
*                               read
*       unsigned int  size      Length of the string to read
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID *TLS_Get_String(const unsigned char *src, unsigned int offset,
                     unsigned char *dest, unsigned int size)
{
    return (memcpy(dest, (src + offset), size));

} /* TLS_Get_String */

/*************************************************************************
*
*   FUNCTION
*
*       TLS_Eq_String
*
*   DESCRIPTION
*
*       Test two string for equality.
*
*   INPUTS
*
*       unsigned char *packet   Address of one string to compare
*       unsigned int   offset   Offset from the packet address to
*                               compare at
*       unsigned char *local    String to compare to
*       unsigned int  size      Length of the strings to compare
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
int TLS_Eq_String(const unsigned char *packet, unsigned int offset,
                  const unsigned char *local, unsigned int size)
{
   return (TLS_Comparen((packet + offset), local, size));

} /* TLS_Eq_String */

#if !(IPCHECK_ASM)
/*************************************************************************
*
*   FUNCTION
*
*       TLS_IP_Check
*
*   DESCRIPTION
*
*       This function checksums an IP header.
*
*   INPUTS
*
*       header                  Pointer to structure to be checksummed
*       length                  Length of header structure
*
*   OUTPUTS
*
*       UINT16                  Checksum value of structure
*
*************************************************************************/
UINT16 TLS_IP_Check(const UINT16 *header, UINT16 length)
{
    UINT32 sum;

    for (sum = 0; length > 0; length--)
    {
         sum += *header++;
         /* check for a carry over 16 bits */
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (INTSWAP((UINT16)~sum));

} /* TLS_IP_Check */

#endif /* IPCHECK_ASM */

/*************************************************************************
*
*   FUNCTION
*
*       TLS_IP_Check_Buffer_Chain
*
*   DESCRIPTION
*
*       This function checksums an IP Buffer Chain.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the net buffer list
*
*   OUTPUTS
*
*       UINT16                  The checksum value
*
*************************************************************************/
UINT16 TLS_IP_Check_Buffer_Chain(NET_BUFFER *buf_ptr)
{
    UINT32      sum;
    UINT16      *current_byte = NU_NULL;
    NET_BUFFER  *temp_buf_ptr;
    UINT32      data_len, total_length, length;
    UINT8       odd_length;

    /* Init */
    sum             = 0;
    total_length    = 0;
    odd_length      = NU_FALSE;
    length          = buf_ptr->mem_total_data_len;

    /* Check for an odd length. */
    if (length & 1)
    {
        /* For an odd length a byte equal to zero is padded to the end
           of the buffer for checksum computation. This has to be done
           since the checksum is done on 16 bits at a time. Since the
           buffers are chained there is no easy way to get to the end
           of the buffer. Therefore we will set a flag so that this special
           case can be handled when the loop below reaches the end of
           the buffer. */
        odd_length = NU_TRUE;

    }

    /* Divide the length by two. */
    length = length >> 1;

    /* Get a pointer to the buffer. */
    temp_buf_ptr = buf_ptr;

    /* Loop through the chain computing the checksum on each byte
       of data. */
    do
    {
        /* Reset the data pointer. */
        current_byte = (UINT16 *)temp_buf_ptr->data_ptr;

        for (data_len = (temp_buf_ptr->data_len >> 1); data_len &&
            (total_length < length); total_length++, data_len--)
            sum += *current_byte++;

        /* Point to the next buffer. */
        temp_buf_ptr = temp_buf_ptr->next_buffer;
    } while ((temp_buf_ptr) && (total_length < length));

    /* Do we need to handle padding the zero and finishing the checksum
       computation? */
    if (odd_length)
    {
        /* If the length left is 1 then we need to update the current byte
           pointer. The loop above did not because it stops before the
           last byte. */

        /* Make sure temp ptr is valid */
        if (temp_buf_ptr)
            /* See if the length is 1 */
            if (temp_buf_ptr->data_len == 1)
                /* Update the current byte ptr to the data in the next buffer. */
                current_byte = (UINT16 *)temp_buf_ptr->data_ptr;

        /* Pad the zero */
        ((UINT8 *)current_byte)[1] = 0;

        /* Do the checksum for the last 16 bits. */
        sum += *current_byte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (INTSWAP((UINT16)~sum));

} /* TLS_IP_Check_Buffer_Chain */

#if !(LONGSWAP_ASM)
/*************************************************************************
*
*   FUNCTION
*
*       TLS_Longswap
*
*   DESCRIPTION
*
*       This function swaps 4 bytes of a long number. Note that this
*       function will work on both Big Endian and Little Endian
*       architectures. On Big Endian architectures this function will
*       not change the byte ordering.
*
*   INPUTS
*
*       number                  32-bit value to be byte swapped
*
*   OUTPUTS
*
*       UINT32                  the input value after being byte swapped
*
*************************************************************************/
UINT32 TLS_Longswap(UINT32 number)
{
    unsigned char *p = (unsigned char *)&number;

    return ((unsigned long)p[0] << 24) +
           ((unsigned long)p[1] << 16) +
           ((unsigned long)p[2] << 8) +
            (unsigned long)p[3];

} /* TLS_Longswap */

#endif /* !(LONGSWAP_ASM) */

#if !(INTSWAP_ASM)
/*************************************************************************
*
*   FUNCTION
*
*       TLS_Intswap
*
*   DESCRIPTION
*
*       This function swaps 2 bytes of a 16-bit number. Note that this
*       function will work on both Big Endian and Little Endian
*       architectures. On Big Endian architectures this function will
*       not change the byte ordering.
*
*   INPUTS
*
*       number                  32-bit value to be byte swapped
*
*   OUTPUTS
*
*       UINT16                  the input value after being byte swapped
*
*************************************************************************/
UINT16 TLS_Intswap(UINT16 number)
{
unsigned char *p = (unsigned char *)&number;

    return (UINT16)((p[0] << 8) + p[1]);

} /* TLS_Intswap */

#endif /* !(INTSWAP_ASM) */

#if !(COMPAREN_ASM)
/*************************************************************************
*
*   FUNCTION
*
*       TLS_Comparen
*
*   DESCRIPTION
*
*       This function compares 2 strings for equality.
*
*   INPUTS
*
*       s1                      Pointer to the first input string
*       s2                      Pointer to the second input string
*       len                     Length of characters to compare
*
*   OUTPUTS
*
*       INT16                   Equality indicator :
*                               return 0 if strings 1 and 2 are not equal
*                               return 1 if strings 1 and 2 are equal
*
*************************************************************************/
INT TLS_Comparen(const VOID *s1, const VOID *s2, unsigned int len)
{
    return (!memcmp ((VOID *)s1, (VOID *)s2, len));

} /* TLS_Comparen */

#endif /* !(COMPAREN_ASM) */
