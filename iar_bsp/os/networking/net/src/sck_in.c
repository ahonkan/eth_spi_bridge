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
*       sck_in.c
*
*   DESCRIPTION
*
*       This file contains the Nucleus Net functions for converting an
*       IP address, either IPv4 or IPv6, into an ASCII string.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SCK_Inet_NTOP_v4
*       SCK_Inet_NTOP_v6
*       NU_Inet_NTOP
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#define MAX_IPV4_STRING_SIZE        16  /* These values are the maximum */

/* Prototypes */
STATUS SCK_Inet_NTOP_v4 (VOID *, CHAR *, INT);

#if (INCLUDE_IPV6 == NU_TRUE)

#define MAX_IPV6_STRING_SIZE        40  /* number of digits, delimiters
                                           and null terminator */

STATUS SCK_Inet_NTOP_v6 (VOID *, CHAR *, INT);

#endif

/*************************************************************************
*
*   FUNCTION
*
*       SCK_Inet_NTOP_v4
*
*   DESCRIPTION
*
*       This function will convert an IPv4 address into an ASCII string
*
*   INPUTS
*
*       *src                    Pointer to the IP address.
*       *dst                    Pointer to where the ASCII string will
*                               be written
*       size                    Size of the area where the ASCII string
*                               will be written
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS SCK_Inet_NTOP_v4(VOID *src, CHAR *dst, INT size)
{
    STATUS        status = NU_SUCCESS;
    CHAR          *head = dst;
    UINT8         *ip = (UINT8 *)src;
    UINT8         hundreds;
    UINT8         tens;
    INT           i;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Ensure we have enough space for the IP address. */
    if ( size < MAX_IPV4_STRING_SIZE )
        status = NU_INVALID_PARM;

    if ( status == NU_SUCCESS )
#endif
    {
        /* Loop through the four bytes in the IPv4 address. */
        for ( i = 0; i < IP_ADDR_LEN; i++ )
        {
            /* If this byte contains hundreds. */
            if ( ip[i] >= 100 )
            {
                /* The first digit is going to be either a 2 or a 1.
                 * Remember the number of hundreds.
                 */
                *head = (CHAR)( ( ( ip[i] >= 200 ) ? 2 : 1 ) + 0x30);
                hundreds = ( ip[i] >= 200 ) ? 200 : 100;
                head++;
            }
            else
                hundreds = 0;

            /* If this byte has a tenth digit. */
            if ( ip[i] >= 10 )
            {
                /* Calculate the digit and remember the number of tens. */
                tens = ( ( ip[i] - hundreds ) / 10 );
                *head = tens + 0x30;
                tens = tens * 10;
                head++;
            }
            else
                tens = 0;

            /* We will always have at least one digit, even though it may
             * be a zero.
             */
            *head = ( ip[i] - hundreds - tens ) + 0x30;
            head++;

            /* Put a '.' to terminate this byte. */
            *head = '.';
            head++;
        }

        /* The last byte will have a null character instead of the '.'.
         * Update it now.
         */
        *(--head) = '\0';
    }

    return ( status );

} /* SCK_Inet_NTOP_v4 */

#if (INCLUDE_IPV6 == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       SCK_Inet_NTOP_v6
*
*   DESCRIPTION
*
*       This function will convert an IPv6 address into an ASCII string
*
*   INPUTS
*
*       *src                    Pointer to the IP address.
*       *dst                    Pointer to where the ASCII string will
*                               be written
*       size                    Size of the area where the ASCII string
*                               will be written
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS SCK_Inet_NTOP_v6(VOID *src, CHAR *dst, INT size)
{
    STATUS        status = NU_SUCCESS;
    CHAR          hex_digits[] = "0123456789abcdef";
    UINT8         *ip = (UINT8 *)src;
    UINT8         digit;
    UINT8         has_digit = NU_FALSE;
    INT           i;

    /* Ensure we have enough space for the IP address. */
    if ( size < MAX_IPV6_STRING_SIZE )
        status = NU_INVALID_PARM;

    if ( status == NU_SUCCESS )
    {
        /* Loop through the 16 bytes in the IPv6 address. */
        for ( i = 0; i < IP6_ADDR_LEN; i++ )
        {
            /* Get the most significant four digit from the current byte
             * first.
             */
            digit = ((UINT8)( ( ip[i] & 0xF0 ) >> 4 ));

            /* This is a non-zero digit, then just put it in to our ASCII
             * version.
             */
            if ( digit != 0 )
            {
                *dst = hex_digits[digit];
                dst++;

                has_digit = NU_TRUE;
            }

            /* However, if this is a zero-digit, we will need to place a zero
             * only if there is a non-zero digit preceding us. This can only
             * happen if we are the second byte and the first byte had a
             * non-zero digit.
             */
            else if ( has_digit )
            {
                *dst = '0';
                dst++;
            }

            /* Now get the less significant four digits from the byte. */
            digit = ( ip[i] & 0x0F );

            /* This is a non-zero digit, then just put it in to our ASCII
             * version.
             */
            if ( digit != 0 )
            {
                *dst = hex_digits[digit];
                dst++;

                has_digit = NU_TRUE;
            }

            /* If this is a zero, we will put a zero in only if we are
             * the second byte or if the higher digit is non-zero.
             */
            else if ( ( i & 1 ) || ( has_digit ) )
            {
                *dst = '0';
                dst++;
            }

            /* If this is a second byte, put a ':' to terminate the two
             * preceding bytes
             */
            if ( i & 1 )
            {
                *dst = ':';
                dst++;

                has_digit = NU_FALSE;
            }
        }

        /* The last byte will have a null character instead of the ':'.
         * Update it now.
         */
        *(--dst) = '\0';
    }

    return ( status );
} /* SCK_Inet_NTOP_v6 */

#endif /* INCLUDE_IPV6 */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Inet_NTOP
*
*   DESCRIPTION
*
*       This function will determine which function to call to format the
*       IP address into an ASCII string
*
*   INPUTS
*
*       family                  Socket family type, used to determine
*                               which IP version is in use
*       *src                    Pointer to the IP address.
*       *dst                    Pointer to where the ASCII string will
*                               be written
*       size                    Size of the area where the ASCII string
*                               will be written
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Inet_NTOP(INT family, VOID *src, CHAR *dst, INT size)
{
    /* Verify that we have been passed valid pointers before we go any further. */
    if ((src == NU_NULL) || (dst == NU_NULL))
        return NU_INVALID_PARM;

    switch (family)
    {
        case SK_FAM_IP :
            return (SCK_Inet_NTOP_v4 (src, dst, size));

#if (INCLUDE_IPV6 == NU_TRUE)
        case SK_FAM_IP6 :
            return (SCK_Inet_NTOP_v6 (src, dst, size));
#endif
        default :
            return (NU_INVALID_PARM);
    }

} /* NU_Inet_NTOP */
