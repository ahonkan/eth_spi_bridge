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
*       ips_ip.h
*
* COMPONENT
*
*       IPSEC - IP Processing
*
* DESCRIPTION
*
*       This include file will handle defines relating to IPsec at the IP
*       layer.
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
#ifndef IPS_IP_H
#define IPS_IP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

struct _TCP_Port;
struct uport;

/* Functions required for IPsec processing. */
STATUS IP_IPSEC_Interpret(DV_DEVICE_ENTRY *device, NET_BUFFER *buf_ptr,
                          VOID **pkt, UINT8 *next_header,
                          UINT16 *header_len, UINT8 ver);

STATUS IP_IPSEC_Send(DV_DEVICE_ENTRY *int_face, NET_BUFFER **hdr_buf,
                     VOID **ip_dgram, UINT8 ver, UINT8 protocol,
                     UINT8 *tunnel_src_ip4, UINT8 *tunnel_src_ip6, 
                     VOID *dest_ptr, VOID *ro_ptr);

STATUS IP_IPSEC_Forward(DV_DEVICE_ENTRY *int_face, NET_BUFFER **hdr_buf,
                        VOID **ip_dgram, UINT8 ver, UINT8 protocol,
                        UINT8 *tunnel_src_ip4, UINT8 *tunnel_src_ip6);

VOID IP_IPSEC_Pkt_Selector(IPSEC_SELECTOR *ipsec_selector, VOID *pkt,
                           NET_BUFFER *buf_ptr, UINT8 next_header,
                           UINT8 ver, UINT16 hlen);

VOID IP_IPSEC_Pkt_Selector_Update_Ports(IPSEC_SELECTOR *ipsec_selector,
                                        NET_BUFFER *buf_ptr,
                                        UINT8 next_header, UINT16 hlen);

STATUS IP4_IPS_Tunnel(NET_BUFFER **buf_ptr, IPLAYER **pkt, UINT8 *dest_ip,
                      UINT8 *src_ip, UINT8 ttl, UINT8 protocol,
                      UINT8 tos, UINT8 df_bit);

STATUS IPSEC_TCP_Check_Policy_In (NET_BUFFER *buf_ptr,
                                  struct _TCP_Port *prt);

STATUS IPSEC_UDP_Check_Policy_In (NET_BUFFER *buf_ptr,
                                  struct uport *uptr);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IPS_IP_H */
