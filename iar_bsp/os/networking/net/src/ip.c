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
*   FILENAME
*
*       ip.c
*
*   DESCRIPTION
*
*       This file contains the common implementation of the IP component.
*
*   DATA STRUCTURES
*
*       IP_Ident
*       IP_Time_To_Live
*       IP_Forwarding
*       IP_Frag_Queue
*       IP6_Frag_Queue
*       IP_NAT_Initialize
*       IP_Brd_Cast[]
*       IP_Null[]
*
*   FUNCTIONS
*
*       IP_Initialize
*
*   DEPENDENCIES
*
*       nu_net.h
*       nat_extr.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_NAT == NU_TRUE)
#include "networking/nat_extr.h"
#endif

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

/*  This is the id field of outgoing IP packets. */
INT16         IP_Ident;

UINT8         IP_Time_To_Live;

UINT8         IP_Null[IP_ADDR_LEN];

#endif

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
  /* This is a flag that controls the forwarding of IP packets. */
UINT8         IP_Forwarding;
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IP_REASSEMBLY == NU_TRUE) )
IP_QUEUE      IP_Frag_Queue;
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_NAT == NU_TRUE) )
INT           IP_NAT_Initialize;
#endif

/***********************************************************************
*
*   FUNCTION
*
*       IP_Initialize
*
*   DESCRIPTION
*
*       Initialize the global data associated with the IP layer.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IP_Initialize(VOID)
{
#if (INCLUDE_IPV4 == NU_TRUE)
    /* Identification field of outgoing packets. */
    IP_Ident = 1;

    /* The default time to live should be 64 */
    IP_Time_To_Live = IP_TIME_TO_LIVE;

    PUT32(IP_Null, 0, (UINT32)0);
#endif

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
    /* Disable IP Forwarding by default. */
    IP_Forwarding = NU_TRUE;
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IP_REASSEMBLY == NU_TRUE) )
    IP_Frag_Queue.ipq_head = NU_NULL;
    IP_Frag_Queue.ipq_tail = NU_NULL;

    /* Record the timeout value for ip reassembly. This is defined in
     * Nucleus PLUS clock ticks. So we divide by SCK_Ticks_Per_Second to
     * get the number of seconds.
     */
    MIB2_ipReasmTimeout_Set((IP_FRAG_TTL / SCK_Ticks_Per_Second));
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_NAT == NU_TRUE) )
    IP_NAT_Initialize = 0;
#endif

} /* IP_Initialize */
