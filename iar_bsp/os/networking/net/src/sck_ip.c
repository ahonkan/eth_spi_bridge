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
*       sck_ip.c
*
*   DESCRIPTION
*
*       This file contains the Nucleus Net functions for converting an
*       an ASCII string into an IP address, either IPv4 or IPv6.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Inet_PTON_v4
*       SCK_Inet_PTON_v6
*       SCK_Inet_PTON
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"


/* Prototypes */
STATIC STATUS SCK_Inet_PTON_v4 (const CHAR *, UINT8 *);

#if (INCLUDE_IPV6 == NU_TRUE)
STATIC STATUS SCK_Inet_PTON_v6 (CHAR *, UINT8 *);
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Inet_PTON
*
*   DESCRIPTION
*
*       This function will determine which function to call to format the
*       ASCII string into the IP address
*
*   INPUTS
*
*       family                  Socket family type, used to determine
*                               which IP version is in use
*       *src                    Pointer to the ASCII string.
*       *dst                    Pointer to where the IP address will
*                               be written
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Inet_PTON(INT family, CHAR *src, VOID *dst)
{
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Verify that we have been passed valid pointer before going any further */
    if ((src == NU_NULL) || (dst == NU_NULL))
        return (NU_INVALID_PARM);

#endif

    switch (family)
    {
        case SK_FAM_IP :
            return (SCK_Inet_PTON_v4 (src, (UINT8 *)dst));

#if (INCLUDE_IPV6 == NU_TRUE)
        case SK_FAM_IP6 :
            return (SCK_Inet_PTON_v6 (src, (UINT8 *)dst));
#endif
        default :
            return (NU_INVALID_PARM);
    }

} /* NU_Inet_PTON */

/*************************************************************************
*
*   FUNCTION
*
*       SCK_Inet_PTON_v4
*
*   DESCRIPTION
*
*       This function will convert an ASCII string into an IPv4 address
*
*   INPUTS
*
*       *src                    Pointer to the ASCII string.
*       *dst                    Pointer to where the IPv4 address will
*                               be written
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATIC STATUS SCK_Inet_PTON_v4(const CHAR *src, UINT8 *dst)
{
    STATUS        status = NU_SUCCESS;
    INT           i;

    /* Since this an IPv4 address, we are looking for the 4 bytes of address.
     * We loop here to either find the address or till we encounter a failure.
     */
    for ( i = 0; ( i < IP_ADDR_LEN ) && ( status == NU_SUCCESS ); i++ )
    {
        /* Assume that the byte under consideration is zero. */
        dst[i] = 0;

        /* In this loop we calculate the value of a single byte. We also
         * ensure that the byte is never greater than 255 and that we
         * do not have an illegal IP format.
         */
        do
        {
            /* Since we are here, there is a digit to be appended to the
             * byte. However, if the current value is greater than 25, then
             * we will end up having a larger value than 255. The other two
             * checks ensure that we have a valid digit.
             */
            if ( ( dst[i] > 25 ) || ( *src < 0x30 ) || ( *src > 0x39 ) )
            {
                status = -1;
                break;
            }

            /* We have a new digit that we need to append to the current
             * byte, move the remaining digits to the left and add the
             * new digit.
             */
            dst[i] = ( dst[i] * 10 ) + ((UINT8)*src - 0x30);
            src++;

        } while ( ( *src != '.' ) && ( *src != '\0' ) );

        src++;
    }

    if ( i < IP_ADDR_LEN )
        status = -1;

    return ( status );

} /* SCK_Inet_PTON_v4 */

#if (INCLUDE_IPV6 == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       SCK_Inet_PTON_v6
*
*   DESCRIPTION
*
*       This function will convert an ASCII string into an IPv6 address
*
*   INPUTS
*
*       *src                    Pointer to the ASCII string .
*       *dst                    Pointer to where the IPv6 address will
*                               be written
*
*   OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATIC STATUS SCK_Inet_PTON_v6(CHAR *src, UINT8 *dst)
{
    STATUS    status = NU_SUCCESS;
    UINT8     valid_digit = NU_FALSE;
    UINT8     *end = dst + IP6_ADDR_LEN;
    UINT16    value = 0;
    UINT8     *colonp = NU_NULL;
    CHAR      *token;
    INT       i, j;

    /* If we see one occurrence of a ':' make sure there is another one. */
    if ( ( *src == ':' ) && (*(++src) != ':') )
    {
        status = -1;
    }

    if ( status == NU_SUCCESS )
    {
        token = src;

        /* Traverse the whole ASCII string and convert it in to an IPv6
         * address. We only stop if we encounter a null character a '%'
         * or we run in to some other error. The second check is to take care
         * of the Microsoft IPv6 addresses where sometimes the interface number
         * is appended after the '%' character.
         */
        while ( ( *src != '\0' ) && ( *src != '%') &&
                ( status == NU_SUCCESS ) )
        {
            /* For the case where the current digit is between 0-9 inclusive. */
            if ( ( *src >= 0x30 ) && ( *src <= 0x39 ) )
            {
                value <<= 4;
                value |= (UINT16)(*src) - 0x30;
                valid_digit = NU_TRUE;
            }

            /* For the case where the current digit is between a-f inclusive. */
            else if ( ( *src >= 0x61 ) && ( *src <= 0x66 ) )
            {
                value <<= 4;
                value |= (UINT16)(*src) - 0x57;
                valid_digit = NU_TRUE;
            }

            /* For the case where the current digit is betweeen A-F inclusive. */
            else if ( ( *src >= 0x41 ) && ( *src <= 0x46 ) )
            {
                value <<= 4;
                value |= (UINT16)(*src) - 0x37;
                valid_digit = NU_TRUE;
            }

            /* If the current character is not a digit but a ':'. */
            else if ( *src == ':')
            {
                /* Save off a pointer to the current token */
                token = src + 1;

                if ( !valid_digit )
                {
                    if ( colonp )
                        status = -1;
                    else
                        colonp = dst;
                }

                /* Ensure that this is not the last character in the string.
                 * This is not allowed.
                 */
                else if ( *(src + 1) == '\0')
                {
                   status = -1;
                }

                /* Ensure that we have enough room to copy the current string. */
                else if ( (dst + sizeof(INT16) ) > end)
                    status = -1;

                else
                {
                    /* Copy the first byte. */
                    *dst++ = (UINT8)((value >> 8) & 0xff);

                    /* Copy the second byte into our temporary buffer */
                    *dst++ = (UINT8)(value & 0xff);

                    valid_digit = NU_FALSE;
                    value = 0;
                }
            }

            /* If this is a IPv4-mapped-to-IPv6 address, then call
             * SCK_Inet_PTON_v4 to handle the address.
             */
            else if ( ( *src == '.' ) && ( (dst + IP_ADDR_LEN) <= end ) &&
                      ( SCK_Inet_PTON_v4(token, dst) == NU_SUCCESS ) )
            {
                dst += IP_ADDR_LEN;
                valid_digit = NU_FALSE;
                break;
            }

            /* This is not a valid character for the string. */
            else
            {
                status = -1;
            }

            src++;
        }

        /* We need to copy the last two bytes into the temporary buffer if there is
         * room.  If not, error out.
         */
        if ( valid_digit )
        {
            if ((dst + sizeof(INT16)) > end)
                return (-1);

            /* Copy the first byte into our temporary buffer */
            *dst++ = (CHAR)((value >> 8) & 0xff);

            /* Copy the second byte into our temporary buffer */
            *dst++ = (CHAR)(value & 0xff);
        }

        if (colonp != NU_NULL)
        {
            /* If we had encountered the :: shorthand, fill it with 0s. */
            j = (INT)(dst - colonp);

            if (dst == end)
                return (-1);
            else
            {
                for (i = 1; i <= j; i++)
                {
                    end[- i] = colonp[j - i];
                    colonp[j - i] = 0;
                }

                dst = end;
            }
        }

        /* Make sure that we have the complete IPv6 address. */
        if (dst != end)
            status = -1;
    }

    return ( status );

} /* SCK_Inet_PTON_v6 */

#endif
