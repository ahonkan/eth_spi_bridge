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
*       ike2_sadb.h
*
* COMPONENT
*
*       IKEv2 - SADB
*
* DESCRIPTION
*
*       This file contains functions prototypes related to
*       IKEv2 SADB handling.
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

#ifndef IKE2_SADB_H
#define IKE2_SADB_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

#define IKE2_FLUSH_SA_PAYLOAD(msg)      ((IKE2_MESSAGE*)msg)->ike2_sa = \
                                            NU_NULL

STATUS IKE2_Find_SA(IKE2_SADB *db, UINT8 *spi_local, UINT8 *spi_peer,
                    IKE2_SA **out_sa);
STATUS IKE2_Add_IKE_SA(IKE2_SADB *sadb, IKE2_SA *sa);
STATUS IKE2_Delete_IKE_SA(IKE2_SADB *sadb, IKE2_SA *sa);
STATUS IKE2_Find_SADB_By_SA(IKE2_SA *sa, IKE2_SADB **out_sadb);
STATUS IKE2_Add_Exchange_Handle(IKE2_EXCHANGE_DB *db,
                            IKE2_EXCHANGE_HANDLE *handle);
STATUS IKE2_Exchange_Lookup(IKE2_SA *sa, IKE2_POLICY_SELECTOR *select,
                            IKE2_EXCHANGE_HANDLE **handle);
STATUS IKE2_Exchange_Lookup_By_ID(IKE2_SA *sa, UINT32 msg_id,
                                  IKE2_EXCHANGE_HANDLE **handle);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif
