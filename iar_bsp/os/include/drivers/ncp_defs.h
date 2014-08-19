/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       ncp_defs.h
*
*   COMPONENT
*
*       NCP - Network Control Protocol
*
*   DESCRIPTION
*
*       This file contains constant definitions and structure
*       definitions to support the file ncp.c.
*
*   DATA STRUCTURES
*
*       IPCP_OPTIONS
*       IPV6CP_OPTIONS
*       NCP_LAYER
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_INC_NCP_DEFS_H
#define PPP_INC_NCP_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

#define NCP_IPV6_ADDRESS            1
#define NCP_IP_ADDRESS              3
#define NCP_PRIMARY_DNS_ADDRESS     129
#define NCP_SECONDARY_DNS_ADDRESS   131

#define IPCP_ADDR_LEN               4
#define IPV6CP_ADDR_LEN             8

/* Status types. */
#define NCP_ACK                     1
#define NCP_NAK                     2
#define NCP_REJECT                  3
#define NCP_DROP                    4
#define NCP_ABORT                   5
#define NCP_OK                      6

#define NCP_LOCAL                   1
#define NCP_REMOTE                  2

#define NCP_IPV6_UBIT_MASK          (~(0x02))

#define PPP_FLAG_DNS1                   0x0001
#define PPP_FLAG_DNS2                   0x0002
#define PPP_FLAG_NO_IPV4                0x0004

typedef struct ipcp_options
{
    UINT32      default_flags;
    UINT32      flags;

    UINT8       local_address[4];
    UINT8       remote_address[4];

    UINT8       primary_dns_server[4];
    UINT8       secondary_dns_server[4];

    UINT8       use_primary_dns_server;
    UINT8       use_secondary_dns_server;

    UINT8       pad[2];

} IPCP_OPTIONS;

typedef struct ipv6cp_options
{
    UINT8       local_address[8];
    UINT8       remote_address[8];

} IPV6CP_OPTIONS;

typedef struct ncp_layer
{
    TQ_EVENT                event_id;
    UINT16                  protocol_code;
    UINT8                   state;
    UINT8                   identifier;

    union
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        IPCP_OPTIONS        ipcp;
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        IPV6CP_OPTIONS      ipv6cp;
#endif
    } options;

    INT8                    num_transmissions;
    UINT8                   pad[3];
} NCP_LAYER;

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_NCP_DEFS_H */
