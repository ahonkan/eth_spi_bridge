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

/**************************************************************************
*
*   FILENAME
*
*       net4.h
*
*   DESCRIPTION
*
*       This file includes macros, data structures and function
*       declarations for the ethernet layer.
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

#ifndef NET4_H
#define NET4_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

STATUS  ETH4_Map_Multi(const DV_REQ *, UINT8 *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif
