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
*       ethd.c
*
*   DESCRIPTION
*
*       This file holds all defines that are declared for Ethernet and
*       accessed by other modules than Ethernet so Ethernet and ARP can
*       be cleanly removed from the build if not used.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       nu_net.h
*
****************************************************************************/

#include "networking/nu_net.h"

/*  The ethernet hardware broadcast address */
const UINT8  NET_Ether_Broadaddr[DADDLEN] = {0xff,0xff,0xff,0xff,0xff,0xff};

/* Declare the global function pointer for SPAN. When SPAN is initialized
   it will set this to point to the correct function. Otherwise this
   will point to zero and will not be used. */
UINT32 (*span_process_packet) (UINT8 *, UINT32, UINT32);

/* Declare the global function pointer for PPPoE. */
VOID   (*ppe_process_packet)(UINT16);
