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
*       ike2_ips.h
*
* COMPONENT
*
*       IKEv2 - IPsec Specific
*
* DESCRIPTION
*
*       This file contains the constants and prototypes for other IPsec
*       specific functions used within IKEv2.
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

#ifndef IKE2_IPS_H
#define IKE2_IPS_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

UINT8 IKE2_AH_Trans_ID_IPS_To_IKE(UINT8 ips_auth_algo);

UINT8 IKE2_ESP_Trans_ID_IPS_To_IKE(UINT8 ips_encrypt_algo);

UINT8 IKE2_Protocol_ID_IPS_To_IKE(UINT8 ips_proto);

STATUS IKE2_Get_IPsec_Policy_Security(IKE2_EXCHANGE_HANDLE *handle,
                                      IPSEC_SECURITY_PROTOCOL **security,
                                      UINT8 *security_size);

STATUS IKE2_IPS_Generate_SA_Pair(IKE2_EXCHANGE_HANDLE *handle);

STATUS IKE2_IPS_ID_To_Selector(IKE2_ID_PAYLOAD *id,
                               struct addr_struct *abs_addr,
                               IPSEC_SELECTOR *selector, UINT8 side);

STATUS IKE2_IPS_Policy_By_ID(IKE2_EXCHANGE_HANDLE *handle,
                             IKE2_ID_PAYLOAD *id_i, IKE2_ID_PAYLOAD *id_r);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE2_IPS_H */
