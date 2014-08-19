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
*       ioctl_sarp.c
*
*   DESCRIPTION
*
*       This file contains the routine to create or modify
*       an entry in the arp cache
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCSARP
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_ARP == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       Ioctl_SIOCSARP
*
*   DESCRIPTION
*
*       This function creates or modifies an entry in the arp cache
*
*   INPUTS
*
*       *option                 Return pointer for option status.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_NO_MEMORY            A matching ARP entry was not found and
*                               a new entry could not be created.
*
*************************************************************************/
STATUS Ioctl_SIOCSARP(const SCK_IOCTL_OPTION *option)
{
    STATUS  status;

    /* Attempt to update or create an ARP cache entry */
    if (ARP_Cache_Update(IP_ADDR((UINT8*)option->s_ret.arp_request.arp_pa.sck_data),
                         option->s_ret.arp_request.arp_ha.sck_data,
                         option->s_ret.arp_request.arp_flags, -1) != -1)
        status = NU_SUCCESS;
    else
        status = NU_NO_MEMORY;

    return (status);

} /* Ioctl_SIOCSARP */

#endif
