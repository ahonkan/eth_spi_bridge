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
*       nbc.c
*
*   DESCRIPTION
*
*       This file contains optimized bit-aligned versions of the memcpy
*       function.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Block_Copy
*
*   DEPENDENCIES
*
*       target.h
*
*************************************************************************/
/* This function must be configured to match your target hardware.
   Enable the function below that corresponds to your target
   hardware's memory access restrictions.
 */
#include "networking/target.h"

#define OPTIMIZE_8_BIT       NU_FALSE
#define OPTIMIZE_16_BIT      NU_FALSE
#define OPTIMIZE_32_BIT      NU_TRUE

/*************************************************************************
*
*   FUNCTION
*
*       NU_Block_Copy - 8-bit aligned
*
*   DESCRIPTION
*
*       This function performs a memcpy, copying 4 bytes at a time.
*       This function will work with all 8-bit aligned hardware memory.
*
*   INPUTS
*
*       *d                      A pointer to the destination memory
*       *s                      A pointer to the source memory
*       n                       The total data length to copy.
*
*   OUTPUTS
*
*       A pointer to the destination memory.
*
*************************************************************************/
#if OPTIMIZE_8_BIT
VOID* NU_Block_Copy(VOID *d, const VOID *s, unsigned int n)
{
    register long *dst32 = d;
    register long *src32 = (long *)s;
    register long *end;

    register unsigned int remainder;

    /* To get remainder after dividing by 4: just
     * bitwise AND the last two bits and save some cycles
     * over a modulus routine
     */
    remainder = (n & 0x03);

    /* Set a pointer to the end of the memory block.
     * This block will be divisible by 4
     */
    end = (long *)((char *)src32 + (n - remainder));

    /* Copy 4 bytes at a time */
    while ( src32 < end )
        *dst32++ = *src32++;

    /* If there was memory left over, copy it now */
    while(remainder--)
        ((char *)dst32)[remainder] = ((char *)src32)[remainder];

    return(d);
} /* NU_Block_Copy */
#endif /* OPTIMIZE_8_BIT */



/*************************************************************************
*
*   FUNCTION
*
*       NU_Block_Copy - 16-bit aligned
*
*   DESCRIPTION
*
*       This function performs a memcpy, copying 4 bytes at a time,
*       on a 16 bit word boundary. This function will work with all
*       16-bit aligned hardware memory.
*
*   INPUTS
*
*       *d                      A pointer to the destination memory
*       *s                      A pointer to the source memory
*       n                       The total data length to copy.
*
*   OUTPUTS
*
*       A pointer to the destination memory.
*
*************************************************************************/

#if OPTIMIZE_16_BIT
VOID* NU_Block_Copy(VOID *d, const VOID *s, unsigned int n)
{
    register long *dst32 = d;
    register long *src32 = (long *)s;
    register long *end;

    register unsigned int remainder;

    if (!((long)dst32 & 0x01) && !((long)src32 & 0x01))
    {
        remainder = (n & 0x03);

        end =  (long *)((char *)src32 + (n - remainder));

        while ( src32 < end )
            *dst32++ = *src32++;

        while(remainder--)
            ((char *)dst32)[remainder] = ((char *)src32)[remainder];

    }
    else
        memcpy(d, s, n);

    return(d);

} /* NU_Block_Copy */
#endif


/*************************************************************************
*
*   FUNCTION
*
*       NU_Block_Copy - 32-bit aligned
*
*   DESCRIPTION
*
*       This function performs a memcpy, copying 4 bytes at a time,
*       on a 32 bit word boundary. This function will work with all
*       32-bit aligned hardware memory.
*
*   INPUTS
*
*       *d                      A pointer to the destination memory
*       *s                      A pointer to the source memory
*       n                       The total data length to copy.
*
*   OUTPUTS
*
*       A pointer to the destination memory.
*
*************************************************************************/

#if OPTIMIZE_32_BIT
VOID* NU_Block_Copy(VOID *d, const VOID *s, unsigned int n)
{
    register long  *dst32;
    register long  *src32;

    register short *dst16 = (short*)d;
    register short *src16 = (short*)s;

    register long *end;
    register unsigned int remainder;

    /* If the number of bytes to copy is greater than 2, and the source
     * and destination addresses are aligned on the 2 boundary, copy enough
     * data from source to destination to align the addresses onto the next
     * zero boundary so we can finish copying this data in 4-byte chunks.
     */
    if ( (((long)dst16 & 0x03) == 0x02) && (((long)src16 & 0x03) == 0x02) &&
         (n >= 2) )
    {
        *dst16++ = *src16++;
        n -= 2;
    }

    dst32 = (long *)dst16;
    src32 = (long *)src16;

    /* If both addresses are not on a zero boundary, do a standard memcpy,
     * because the optimized memcpy cannot be performed.
     */
    if (!((long)dst32 & 0x03) && !((long)src32 & 0x03))
    {
        remainder = (n & 0x03);

        end =  (long *)((char *)src32 + (n - remainder));

        while ( src32 < end )
            *dst32++ = *src32++;

        while(remainder--)
            ((char *)dst32)[remainder] = ((char *)src32)[remainder];

    }
    else
        memcpy(d, s, n);

    return(d);
} /* NU_Block_Copy */
#endif




