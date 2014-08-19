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
*       serial_common.h
*
*   COMPONENT
*
*       SERIAL                              - Serial Library
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the Serial Library Driver module.
*
*************************************************************************/
#ifndef SERIAL_COMMON_H
#define SERIAL_COMMON_H


#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


STATUS      Serial_PR_Int_Enable(SERIAL_SESSION_HANDLE *session_handle);
STATUS      Serial_PR_Int_Disable(SERIAL_INSTANCE_HANDLE *instance_handle);
STATUS      Serial_Get_Target_Info(const CHAR * key, SERIAL_INSTANCE_HANDLE *tgt_info);
STATUS      Serial_Get_Default_Cfg(const CHAR * key, SERIAL_ATTR *attrs);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* !SERIAL_COMMON_H */
