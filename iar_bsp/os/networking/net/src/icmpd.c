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

/***********************************************************************
*
*   FILE NAME
*
*       icmpd.c
*
*   COMPONENT
*
*       ICMP - Internet Control Message Protocol
*
*   DESCRIPTION
*
*       This file contains global variables used by ICMP within other
*       modules than ICMP.
*
*   DATA STRUCTURES
*
*       ICMP_Echo_List
*       ICMP_Echo_Req_Seq_Num
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/* This list will contain information needed to send pings. It is used
   to determine if reply is for a ping that we sent. */
ICMP_ECHO_LIST  ICMP_Echo_List;

/* This will be used for generating a sequence number when sending ICMP
   echo requests (pings). */
UINT16          ICMP_Echo_Req_Seq_Num = 0;
