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
*       ioctl_darp.c
*
*   DESCRIPTION
*
*       This file contains the routine to delete an entry from
*       the arp cache
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCDARP
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
*       Ioctl_SIOCDARP
*
*   DESCRIPTION
*
*       This function deletes an entry from the arp cache
*
*   INPUTS
*
*       *option                 Return pointer for option status.
*
*   OUTPUTS
*
*       NU_SUCCESS              The entry was successfully deleted.
*       NU_INVALID_PARM         The entry does not exist.
*
*************************************************************************/
STATUS Ioctl_SIOCDARP(const SCK_IOCTL_OPTION *option)
{
    SCK_SOCKADDR_IP     dest;

    /* Set the address of the entry to delete */
    dest.sck_addr = IP_ADDR((UINT8*)option->s_ret.arp_request.arp_pa.sck_data);

    /* Delete the entry */
    return (ARP_Delete_Entry(&dest));

} /* Ioctl_SIOCDARP */

#endif
