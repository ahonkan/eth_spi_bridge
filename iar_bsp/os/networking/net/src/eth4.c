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

/***************************************************************************
*
*   FILENAME
*
*       eth4.c
*
*   DESCRIPTION
*
*       This file will hold all the routines which are used by IPv4 to
*       interface with the ethernet hardware.  They will handle the basic
*       functions of setup, xmit, receive, etc.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       ETH4_Map_Multi
*
*   DEPENDENCIES
*
*       nu_net.h
*
****************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       ETH4_Map_Multi
*
*   DESCRIPTION
*
*       This function converts an IPv4 IP address into a link-layer
*       multicast address.
*
*   INPUTS
*
*       *d_req                  A pointer to the device record.
*       *multi_addr             A pointer to the memory in which to store
*                               the multicast address.
*
*   OUTPUTS
*
*       -1                      The address is not valid.
*       0                       Successful
*
*************************************************************************/
STATUS ETH4_Map_Multi(const DV_REQ *d_req, UINT8 *multi_addr)
{
    UINT8   temp[IP_ADDR_LEN];

    PUT32(temp, 0, d_req->dvr_addr);

    /* Convert the IP address to a multicast ethernet address. */
    NET_MAP_IP_TO_ETHER_MULTI(temp, multi_addr);

    /* Verify that the ethernet multicast address is valid. */
    if ( ((multi_addr[0] & 0xFF) != 1) || ((multi_addr[2] & 0xFF) != 0x5e) )
        return (-1);

    return (0);

} /* ETH4_Map_Multi */

#endif
