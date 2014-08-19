/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/****************************************************************************
*
*   FILE NAME
*
*       ld_defs.h
*
*   COMPONENT
*
*       LD - Loopback Device
*
*   DESCRIPTION
*
*       This file contains macro definitions to support the loopback
*       module LDC.C
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
****************************************************************************/

#ifndef LD_DEFS_H
#define LD_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define LD_ADDRESS_LENGTH   6
#define LD_HEADER_LENGTH    14
#define LD_MTU              1500
#define LD_INVALID_VECTOR   65000UL

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif
