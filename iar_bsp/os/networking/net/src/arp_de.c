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
*       arp_de.c
*
*   DESCRIPTION
*
*       This file contains the implementation of ARP (Address Resolution
*       Protocol).
*
*   DATA STRUCTURES
*
*
*   FUNCTIONS
*
*       ARP_Delete_Entry
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
*       ARP_Delete_Entry
*
*   DESCRIPTION
*
*       This function deletes an entry from the ARP cache.
*
*   INPUTS
*
*       *dest                   IP address of the entry to delete.
*
*   OUTPUTS
*
*       NU_SUCCESS              The entry was successfully deleted.
*       NU_INVALID_PARM         The entry does not exist.
*
*************************************************************************/
STATUS ARP_Delete_Entry(const SCK_SOCKADDR_IP *dest)
{
    ARP_ENTRY   *arp_entry;
    STATUS      status = NU_SUCCESS;

    arp_entry = ARP_Find_Entry(dest);

    if (arp_entry != NU_NULL)
        UTL_Zero(arp_entry, sizeof(ARP_ENTRY));
    else
        status = NU_INVALID_PARM;

    return (status);

} /* ARP_Delete_Entry */
