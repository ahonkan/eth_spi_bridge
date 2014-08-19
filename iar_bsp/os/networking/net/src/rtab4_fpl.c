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
*       rtab4_fpl.c
*
*   COMPONENT
*
*       IPv4 Routing
*
*   DESCRIPTION
*
*       This module contains the functions for determining the prefix
*       length of a network address.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB4_Find_Prefix_Length
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Find_Prefix_Length
*
*   DESCRIPTION
*
*       This function returns the prefix length of a subnet mask in
*       bits.
*
*   INPUTS
*
*       *subnet_mask            A pointer to the subnet mask.
*
*   OUTPUTS
*
*       UINT8                   The length of the subnet mask in bits.
*
*************************************************************************/
UINT8 RTAB4_Find_Prefix_Length(const UINT8 *subnet_mask)
{
    INT     bit_index, prefix_length;

    /* If the subnet mask is all 1's */
    if (IP_ADDR(subnet_mask) == 0xffffffffUL)
        prefix_length = 32;

    /* Otherwise, count the number of 1's */
    else
    {
        prefix_length = 0;

        /* Count the whole bytes of 1's in the subnet mask */
        do
        {
            if (*subnet_mask == 0xff)
                prefix_length += 8;
            else
                break;

            subnet_mask ++;

        } while (prefix_length < 24);

        /* Initialize bit_index */
        bit_index = 7;

        /* Count the number of bits in the current byte that are set to 1 */
        do
        {
            /* If this bit is set, increment the prefix length by one */
            if ((1 << bit_index) & *subnet_mask)
            {
                prefix_length ++;

                /* If this is the last bit in the byte, break out of the
                 * loop.
                 */
                if (bit_index == 0)
                    break;

                bit_index --;
            }

            else
                break;

        } while (prefix_length < 31);
    }

    return ((UINT8)prefix_length);

} /* RTAB4_Find_Prefix_Length */

#endif
