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
*       ip_gnm.c
*
* DESCRIPTION
*
*       This file contains the implementation of IP_Get_Net_Mask.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IP_Get_Net_Mask
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/* class A mask */
const UINT8   IP_A_Mask[4] = {255,0,0,0};

/* class B mask */
const UINT8   IP_B_Mask[4] = {255,255,0,0};

/* class C mask */
const UINT8   IP_C_Mask[4] = {255,255,255,0};

/***********************************************************************
*
*   FUNCTION
*
*       IP_Get_Net_Mask
*
*   DESCRIPTION
*
*       This function returns the mask for a given class of IP
*       addresses.
*
*   INPUTS
*
*       *ip_addr                Pointer to an IP address for which a
*                               mask is desired.
*       *mask                   Pointer to location where the mask will
*                               be copied.
*
*   OUTPUTS
*
*       -1                      The IP address is not a class A, B, or
*                               C address a mask could not be returned.
*       NU_SUCCESS              The mask parameter was updated.
*
*************************************************************************/
STATUS IP_Get_Net_Mask(const UINT8 *ip_addr, UINT8 *mask)
{
    UINT32  addr = LONGSWAP(*(UINT32 *)ip_addr);

    if (IP_CLASSA_ADDR(addr))
    {
        memcpy(mask, IP_A_Mask, IP_ADDR_LEN);
        return (NU_SUCCESS);
    }
    else if (IP_CLASSB_ADDR(addr))
    {
        memcpy(mask, IP_B_Mask, IP_ADDR_LEN);
        return (NU_SUCCESS);
    }
    else if (IP_CLASSC_ADDR(addr))
    {
        memcpy(mask, IP_C_Mask, IP_ADDR_LEN);
        return (NU_SUCCESS);
    }
    else
        return (-1);

} /* IP_Get_Net_Mask */
