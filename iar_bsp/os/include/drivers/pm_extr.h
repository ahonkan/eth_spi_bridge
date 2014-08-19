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
*       pm_extr.h
*
*   COMPONENT
*
*       PM - PPP MIB access
*
*   DESCRIPTION
*
*       This file contains function prototypes used by PPP
*       and also accessible to other modules.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_INC_PM_EXTR_H
#define PPP_INC_PM_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

INT32           PMSC_NextId(INT32*);

INT32 PML_GetPhysicalIndex(DV_DEVICE_ENTRY *dev_ptr);
STATUS PML_GetBadAddresses(DV_DEVICE_ENTRY *dev_ptr, UINT32 *value);
STATUS PML_GetBadControls(DV_DEVICE_ENTRY *dev_ptr, UINT32 *value);
STATUS PML_GetPacketTooLongs(DV_DEVICE_ENTRY *dev_ptr, UINT32 *value);
STATUS PML_GetBadFCSs(DV_DEVICE_ENTRY *dev_ptr, UINT32 *value);
STATUS PML_GetLocalMRU(DV_DEVICE_ENTRY *dev_ptr, INT32 *value);
STATUS PML_GetRemoteMRU(DV_DEVICE_ENTRY *dev_ptr, INT32 *value);
STATUS PML_GetLocalToPeerACCMap(DV_DEVICE_ENTRY *dev_ptr, UINT8 *value);
STATUS PML_GetPeerToLocalACCMap(DV_DEVICE_ENTRY *dev_ptr, UINT8 *value);
STATUS PML_GetLocalToRemoteProtocolCompression(DV_DEVICE_ENTRY *dev_ptr,
                INT32 *value);
STATUS PML_GetRemoteToLocalProtocolCompression(DV_DEVICE_ENTRY *dev_ptr,
                INT32 *value);
STATUS PML_GetLocalToRemoteACCompression(DV_DEVICE_ENTRY *dev_ptr,
                INT32 *value);
STATUS PML_GetRemoteToLocalACCompression(DV_DEVICE_ENTRY *dev_ptr,
                INT32 *value);
STATUS PML_GetTransmitFcsSize(DV_DEVICE_ENTRY *dev_ptr, INT32 *value);
STATUS PML_GetReceiveFcsSize(DV_DEVICE_ENTRY *dev_ptr, INT32 *value);
STATUS PML_GetInitialMRU(DV_DEVICE_ENTRY *dev_ptr, INT32 *value);
STATUS PML_GetInitialReceiveACCMap(DV_DEVICE_ENTRY *dev_ptr, UINT8 *value);
STATUS PML_GetInitialTransmitACCMap(DV_DEVICE_ENTRY *dev_ptr,
                UINT8 *value);
STATUS PML_GetInitialMagicNumber(DV_DEVICE_ENTRY *dev_ptr, INT32 *value);
STATUS PML_GetInitialFcsSize(DV_DEVICE_ENTRY *dev_ptr, INT32 *value);
STATUS PML_SetInitialMRU(DV_DEVICE_ENTRY *dev_ptr, INT32 value);
STATUS PML_SetInitialReceiveACCMap(DV_DEVICE_ENTRY *dev_ptr, UINT8 *value);
STATUS PML_SetInitialTransmitACCMap(DV_DEVICE_ENTRY *dev_ptr,
                UINT8 *value);
STATUS PML_SetInitialMagicNumber(DV_DEVICE_ENTRY *dev_ptr, INT32 value);
STATUS PML_SetInitialFcsSize(DV_DEVICE_ENTRY *dev_ptr, INT32 value);


STATUS PMN_GetIpOperStatus(DV_DEVICE_ENTRY *dev_ptr, INT32 *value);
STATUS PMN_GetIpLocalToRemoteCompressionProtocol(DV_DEVICE_ENTRY *dev_ptr,
                INT32 *value);
STATUS PMN_GetIpRemoteToLocalCompressionProtocol(DV_DEVICE_ENTRY *dev_ptr,
                INT32 *value);
STATUS PMN_GetIpRemoteMaxSlotId(DV_DEVICE_ENTRY *dev_ptr, INT32 *value);
STATUS PMN_GetIpLocalMaxSlotId(DV_DEVICE_ENTRY *dev_ptr, INT32 *value);
STATUS PMN_GetIpConfigAdminStatus(DV_DEVICE_ENTRY *dev_ptr, INT32 *value);
STATUS PMN_GetIpConfigCompression(DV_DEVICE_ENTRY *dev_ptr, INT32 *value);
STATUS PMN_SetIpConfigAdminStatus(DV_DEVICE_ENTRY *dev_ptr, INT32 state);
STATUS PMN_SetIpConfigCompression(DV_DEVICE_ENTRY *dev_ptr, INT32 state);

PMSC_ENTRY*     PMSC_NewEntry(INT32, UINT16, UINT16);
PMSC_ENTRY*     PMSC_FindEntry(INT32, INT32);
STATUS          PMSC_GetNextEntry(INT32*, INT32*);
STATUS          PMSC_GetDefaultProtocol(INT32, UINT16*);
STATUS          PMSC_GetSecurityConfigLink(INT32, INT32, INT32*);
STATUS          PMSC_GetSecurityConfigPreference(INT32, INT32, INT32*);
STATUS          PMSC_GetSecurityConfigProtocol(INT32, INT32, INT32*);
STATUS          PMSC_GetSecurityConfigStatus(INT32, INT32, INT32*);
STATUS          PMSC_SetSecurityConfigLink(INT32, INT32, INT32);
STATUS          PMSC_SetSecurityConfigPreference(INT32, INT32, INT32);
STATUS          PMSC_SetSecurityConfigProtocol(INT32, INT32, INT32);
STATUS          PMSC_SetSecurityConfigStatus(INT32, INT32, INT32);


PMSS_ENTRY*     PMSS_NewEntry(INT32, INT32, UINT16, UINT16);
PMSS_ENTRY*     PMSS_FindEntry(INT32, INT32);
STATUS          PMSS_GetNextEntry(INT32*, INT32*);
STATUS          PMSS_DeleteUserEntries(INT32);
STATUS          PMSS_SetClientLogin(INT32, CHAR*, CHAR*);
STATUS          PMSS_GetSecuritySecretsLink(INT32, INT32, INT32*);
STATUS          PMSS_GetSecuritySecretsIdIndex(INT32, INT32, INT32*);
STATUS          PMSS_GetSecuritySecretsDirection(INT32, INT32, INT32*);
STATUS          PMSS_GetSecuritySecretsProtocol(INT32, INT32, INT32*);
STATUS          PMSS_GetSecuritySecretsIdentity(INT32, INT32, UINT8*);
STATUS          PMSS_GetSecuritySecretsSecret(INT32, INT32, UINT8*);
STATUS          PMSS_GetSecuritySecretsStatus(INT32, INT32, INT32*);

STATUS          PMSS_SetSecuritySecretsDirection(INT32, INT32, INT32);
STATUS          PMSS_SetSecuritySecretsProtocol(INT32, INT32, INT32);
STATUS          PMSS_SetSecuritySecretsIdentity(INT32, INT32, UINT8*);
STATUS          PMSS_SetSecuritySecretsSecret(INT32, INT32, UINT8*);
STATUS          PMSS_SetSecuritySecretsStatus(INT32, INT32, INT32);

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_PM_EXTR_H */
