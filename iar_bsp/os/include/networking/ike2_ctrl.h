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
*       ike2_ctrl.h
*
* COMPONENT
*
*       IKEv2 - Exchange Control
*
* DESCRIPTION
*
*       This file contains prototypes of functions used to control an
*       exchange under way.
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       None.
*
*************************************************************************/
#ifndef IKE2_CTRL_H
#define IKE2_CTRL_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

STATUS IKE2_New_Exchange(IKE2_EXCHANGE_HANDLE **xchg_handle, IKE2_SA **sa,
                         IKE2_STATE_PARAMS *params);

STATUS IKE2_Initiate_IKE2_Exchange(IKE2_POLICY_GROUP *group,
                                   IKE2_POLICY *policy,
                                   IKE2_INITIATE_REQ *request,
                                   struct addr_struct *remote_addr,
                                   UINT32 *ret_msg_id);

STATUS IKE2_Initiate_IKE2_Child_Exchange(IKE2_POLICY_GROUP *group,
                                         IKE2_POLICY *policy,
                                         IKE2_INITIATE_REQ *request,
                                         IKE2_SA *sa, UINT32 *ret_msg_id);

STATUS IKE2_Dispatch(IKE2_PACKET *pkt, IKE2_POLICY *policy);
STATUS IKE2_Dispatch_INIT_SA(IKE2_PACKET *pkt, IKE2_HDR *hdr,
                             IKE2_SA *sa, IKE2_POLICY *policy);
STATUS IKE2_Dispatch_CREATE_CHILD(IKE2_PACKET *pkt, IKE2_HDR *hdr,
                                  IKE2_SA *sa, IKE2_POLICY *policy);

STATUS IKE2_Cleanup_Exchange_Parameters(IKE2_STATE_PARAMS *params);
STATUS IKE2_Set_CREATE_CHILD_SA_Security(IKE2_EXCHANGE_HANDLE *handle,
                                         IKE2_INITIATE_REQ *request);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif

