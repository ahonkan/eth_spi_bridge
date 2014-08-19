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
*       ips6_ip.h
*
* COMPONENT
*
*       IPSEC - IP Processing
*
* DESCRIPTION
*
*       This include file will handle defines relating to IPsec at the
*       IPv6 layer.
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
#ifndef IP6_IPS_H
#define IP6_IPS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

#if (INCLUDE_IPV6 == NU_TRUE)

#define IP6_TRAFFIC_CLASS_OFFSET       1
#define IP6_FLOW_LABEL_OFFSET          2
#define IP6_HEADER_EIGHT_BYTES         8

/* Function creates an IPsec tunnel IPv6 header. */
STATUS IP6_IPS_Tunnel(NET_BUFFER **buf_ptr, IP6LAYER **pkt,
                      UINT8 *dest_ip6, UINT8 *src_ip6);
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_IPS_H */

