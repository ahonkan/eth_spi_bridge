/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
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
*       usbf_common.h
*
*   COMPONENT
*
*       USBF                                - USBF Library
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the USBF Library Driver module.
*
*************************************************************************/
#ifndef USBF_COMMON_H
#define USBF_COMMON_H


#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

STATUS USBF_Get_Target_Info(const CHAR * key, USBF_INSTANCE_HANDLE *inst_handle);
STATUS USBF_Init(USBF_INSTANCE_HANDLE *inst_handle, const CHAR *key);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* !USBF_COMMON_H */
