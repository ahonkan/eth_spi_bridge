/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       dnsd.c
*
*   DESCRIPTION
*
*       This file declares globals for the Domain Name System (DNS)
*       component.
*
*   DATA STRUCTURES
*
*       DNS_Servers
*       DNS_Hosts
*       DNS_Resource
*
*   FUNCTIONS
*
*       NONE
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/* This is a list of DNS servers or resolvers. */
DNS_SERVER_LIST         DNS_Servers;

/* DNS Semaphore for protecting DNS globals. */
NU_SEMAPHORE            DNS_Resource;

/* This is the list of hosts.  If a host can not
   be found in this list, DNS will be used to
   retrieve the information.  The new host will
   be added to the list.
*/
DNS_HOST_LIST           DNS_Hosts;



