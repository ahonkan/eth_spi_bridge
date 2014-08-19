/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ips_externs.h
*
* COMPONENT
*
*       IPSEC - External Definitions
*
* DESCRIPTION
*
*       Externs definitions for IPsec module.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IPS_EXTERNS_H
#define IPS_EXTERNS_H

#ifdef          __cplusplus
extern  "C" {                              /* C declarations in C++     */
#endif /* _cplusplus */

/* Extern the IPsec memory pool pointer. */
extern NU_MEMORY_POOL       *IPSEC_Memory_Pool;

/* Extern the IPsec semaphore to be used by different components. */
extern NU_SEMAPHORE         IPSEC_Resource;

#if (INCLUDE_IKE == NU_TRUE)
/* Extern the timer event for the soft lifetimes of IPsec SAs. */
extern TQ_EVENT             IPSEC_Soft_Lifetime_Event;

/* Extern the timer event for the hard lifetimes of IPsec SAs. */
extern TQ_EVENT             IPSEC_Hard_Lifetime_Event;
#endif

/* The following two variables are used in conjunction with each other to
 * provide information about the security associations that have been
 * applied to the incoming packet. At any time there is only one packet
 * being parsed in NET stack. These variables provide the SA's that have
 * been applied to this point on the packet being parsed.
 */
extern UINT8                IPSEC_SA_Count;
extern IPSEC_INBOUND_SA     *IPSEC_In_Bundle[];

/* Packet selector used to match policies and security associations
 * with the incoming packet.
 */
extern IPSEC_SELECTOR       IPSEC_Pkt_Selector;

/* Extern the timer event for the outbound bundles. */
extern TQ_EVENT             IPSEC_Out_Bundle_Lifetime_Event;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef IPS_EXTERNS_H */
