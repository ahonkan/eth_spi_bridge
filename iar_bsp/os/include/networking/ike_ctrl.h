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
*       ike_ctrl.h
*
* COMPONENT
*
*       IKE - Control
*
* DESCRIPTION
*
*       This file contains constants and function prototypes needed
*       to implement the IKE Control module.
*
* DATA STRUCTURES
*
*       IKE_PACKET
*       IKE_STATE_PARAMS
*       IKE_INITIATE_REQ
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IKE_CTRL_H
#define IKE_CTRL_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** Data structures. ****/

/* Structure for storing raw packets. */
typedef struct ike_packet
{
    /* Address from which the packet was received. */
    struct addr_struct  ike_remote_addr;

    /* Address of the interface which received this packet. */
    struct addr_struct  ike_local_addr;

    /* Interface index on which this packet was received. */
    UINT32          ike_if_index;

    UINT8           *ike_data;              /* Pointer to packet data. */
    UINT16          ike_data_len;           /* Length of packet data. */

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
    INT             ike2_socket;
#endif

    UINT8           ike_pad[2];
} IKE_PACKET;

/* This structure defines all the parameters which are
 * required by each state of the IKE state machine.
 * They are updated every time a packet is received and
 * are then passed to the corresponding Phase 1 or Phase 2
 * state handler.
 */
typedef struct ike_state_params
{
    IKE_ENC_MESSAGE ike_out;                /* Outbound payloads. */
    IKE_DEC_MESSAGE ike_in;                 /* Inbound payloads. */
    IKE_PACKET      *ike_packet;            /* Raw incoming packet. */
    IKE_POLICY      *ike_policy;            /* Policy being applied. */
    IKE_POLICY_GROUP *ike_group;            /* IKE group being used. */
} IKE_STATE_PARAMS;

/* Structure used by IPsec to initiate an SA negotiation. */
typedef struct ike_initiate_req
{
    /* IPsec security protocol to be negotiated. The IPsec
     * policy structure contains an array of security
     * protocols. The following value is a copy of an item
     * from that array. A pair of IPsec SAs (inbound and
     * outbound) are negotiated for this security
     * protocol. If this security protocol is part of a
     * protocol suite, such as AH + ESP, then all SAs
     * required by the suite are negotiated.
     */
    IPSEC_SECURITY_PROTOCOL ike_ips_security;

    IPSEC_SELECTOR  ike_ips_select;         /* IPsec selector. */
    UNSIGNED        ike_suspend;            /* Caller suspend time. */
    UINT32          ike_dev_index;          /* Device index. */
} IKE_INITIATE_REQ;

/**** Function prototypes. ****/

STATUS IKE_Initiate(IKE_INITIATE_REQ *request);
STATUS IKE_Dispatch(IKE_PACKET *pkt, IKE_POLICY_GROUP *group);
STATUS IKE_Send_Delete_Notification(IKE_INITIATE_REQ *request);

/* Following utility functions are used by Informational mode. */
STATUS IKE_New_Phase1(IKE_PHASE1_HANDLE *phase1, IKE_SA *sa,
                      struct addr_struct *remote_addr,
                      IKE_STATE_PARAMS *params);
STATUS IKE_New_Phase2(IKE_PHASE2_HANDLE *phase2, IKE_SA *sa,
                      IKE_STATE_PARAMS *params);
STATUS IKE_Select_Phase2_Security(IPSEC_SECURITY_PROTOCOL *req_security,
                                  IPSEC_SECURITY_PROTOCOL *pol_security,
                                  UINT8 pol_security_no,
                                  IKE_SA2_DB *sa2db);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_CTRL_H */
