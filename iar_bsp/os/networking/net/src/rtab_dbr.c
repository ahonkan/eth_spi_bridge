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
*       rtab_dbr.c
*
*   COMPONENT
*
*       Routing
*
*   DESCRIPTION
*
*       This module contains the functions for determining the value
*       of a bit in an IP address.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB_Determine_Branch
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Determine_Branch
*
*   DESCRIPTION
*
*       This function determines whether the search should branch left
*       or right given the value of bit bit_index in the ip_addr.
*
*   INPUTS
*
*       bit_index               The bit number of the IP address to
*                               determine whether the value is 1 or
*                               a 0.
*       *ip_addr                A pointer to the IP address.
*       *rt_parms               A pointer to data specific to the family
*                               of routing table being used.
*
*   OUTPUTS
*
*       UINT8                   The value of the bit at the bit index -
*                               either 1 or 0.
*
*************************************************************************/
UINT8 RTAB_Determine_Branch(UINT8 bit_index, const UINT8 *ip_addr,
                            const RTAB_ROUTE_PARMS *rt_parms)
{
    UINT8   byte;

    /* Determine in which byte the bit is located. */
    byte = (UINT8)(bit_index >> 3);

    /* Determine which bit in the byte to look at */
    bit_index = (UINT8)(bit_index - (8 * byte));

    /* If the value of the bit_index bit of the IP address is 0, take the
     * 0 child of the current node.  If the value of the bit_index bit of
     * the IP address is 1, take the 1 child of the current node.
     */
    if ( (1 << bit_index) &
         (ip_addr[byte ^ (rt_parms->rt_byte_ip_len - 1)]) )
        return (1);
    else
        return (0);

} /* RTAB_Determine_Branch */
