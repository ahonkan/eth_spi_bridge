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
*       ike_ips.h
*
* COMPONENT
*
*       IKE - IPsec Specific
*
* DESCRIPTION
*
*       This file contains the constants defined in RFC 2407
*       and prototypes for other IPsec specific functions used
*       within IKE.
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
#ifndef IKE_IPS_H
#define IKE_IPS_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** IPsec interface constants. ****/

/* Macro to convert IP family identifier to corresponding IPsec flags. */
#define IKE_IPS_FAMILY_TO_FLAGS(family) (((family) == NU_FAMILY_IP) ?   \
                                         IPSEC_IPV4 : IPSEC_IPV6)

/* Macro to convert IPsec flags to corresponding IP family identifier. */
#define IKE_IPS_FLAGS_TO_FAMILY(flags) (((flags) & IPSEC_IPV4) ?   \
                                         NU_FAMILY_IP : NU_FAMILY_IP6)

/* Macro to convert IPsec flags to corresponding IKE flags. */
#define IKE_IPS_FLAGS_TO_IKE(flags)     (((flags) & IPSEC_IPV4) ?       \
                                         IKE_IPV4 : IKE_IPV6)

/**** Function prototypes. ****/

/* Functions to map IPsec constants to IKE. */
UINT8 IKE_AH_Trans_ID_IPS_To_IKE(UINT8 ips_auth_algo);
UINT8 IKE_AH_Trans_ID_IKE_To_IPS(UINT8 transform_id);
UINT8 IKE_ESP_Trans_ID_IPS_To_IKE(UINT8 ips_encrypt_algo);
UINT8 IKE_ESP_Trans_ID_IKE_To_IPS(UINT8 transform_id);
UINT16 IKE_Auth_Algo_ID_IPS_To_IKE(UINT8 ips_auth_algo);
UINT8 IKE_Auth_Algo_ID_IKE_To_IPS(UINT16 auth_algo);
UINT8 IKE_Protocol_ID_IPS_To_IKE(UINT8 ips_protocol);
UINT8 IKE_Protocol_ID_IKE_To_IPS(UINT8 protocol_id);

/* IPsec specific functions. */
VOID IKE_IPS_Selector_To_IKE(IPSEC_SELECTOR *ips_select,
                             IKE_POLICY_SELECTOR *ike_select);
VOID IKE_IPS_Selector_To_Remote_IP(IPSEC_SELECTOR *ips_select,
                                   struct addr_struct *ike_addr);
VOID IKE_IPS_Security_To_Remote_IP(IPSEC_SECURITY_PROTOCOL *ips_security,
                                   struct addr_struct *ike_addr);
STATUS IKE_IPS_Group_Name_By_Packet(IKE_PACKET *pkt, CHAR *group_name,
                                    UINT32 *total_len);
STATUS IKE_IPS_Group_Name_By_Device(UINT32 dev_index, CHAR *group_name,
                                    UINT32 *total_len);
STATUS IKE_IPS_ID_To_Selector(IKE_ID_DEC_PAYLOAD *id,
                              struct addr_struct *abs_addr,
                              IPSEC_SELECTOR *select, UINT8 side);
STATUS IKE_IPS_Selector_To_ID(IPSEC_SELECTOR *select,
                              IKE_ID_ENC_PAYLOAD *id, UINT8 side);
STATUS IKE_IPS_Policy_By_ID(IKE_PHASE2_HANDLE *ph2,
                            IKE_ID_DEC_PAYLOAD *id_i,
                            IKE_ID_DEC_PAYLOAD *id_r);
STATUS IKE_IPS_Phase2_Allowed(IKE_POLICY *policy,
                              IPSEC_SELECTOR *ips_select);
STATUS IKE_IPS_Get_Policy_Parameters(IKE_PHASE2_HANDLE *phase2,
                                     IPSEC_SECURITY_PROTOCOL **security,
                                     UINT8 *security_size);
STATUS IKE_IPS_Get_Key_Length(IPSEC_SECURITY_PROTOCOL *security,
                              UINT16 *enc_key_len, UINT16 *auth_key_len);
STATUS IKE_IPS_Generate_SA_Pairs(IKE_PHASE2_HANDLE *ph2);
VOID IKE_IPS_Switch_Selector(IPSEC_SELECTOR *select_a,
                             IPSEC_SELECTOR *select_b);
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
VOID IKE_IPS_Switch_Security(IPSEC_SECURITY_PROTOCOL *secure_a,
                             IPSEC_SECURITY_PROTOCOL *secure_b);
#endif

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_IPS_H */
