/*************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
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
*       ethernet_common.h
*
*   COMPONENT
*
*       ETHERNET                            - Ethernet Library
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the Ethernet Library Driver module.
*
*************************************************************************/
#ifndef ETHERNET_COMMON_H
#define ETHERNET_COMMON_H


#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

STATUS  Ethernet_Get_Target_Info(const CHAR * key, ETHERNET_INSTANCE_HANDLE *inst_handle);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* ETHERNET_COMMON_H */
