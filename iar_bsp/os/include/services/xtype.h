/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*        xtype.h
*
* COMPONENT
*
*       RTL - RunTime Library.
*
* DESCRIPTION
*
*       This file contains the internal routine and declarations used by
*       xtype(), which is used by fopen().
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       "stddef.h"                          STDDEF related definitions.
*
*************************************************************************/

#ifndef NU_PSX_XTYPE_H
#define NU_PSX_XTYPE_H

#ifndef __XTYPE_H_
#define __XTYPE_H_

#include "services/stddef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define A_POSIX         1     /* append */
#define R_POSIX         2     /* read */
#define W_POSIX         4     /* write */
#define PLUS_POSIX      8     /* plus (both read and write) */
#define B_POSIX         16    /* binary */

int xtype(const char *type);

#ifdef __cplusplus
}
#endif

#endif /* !__XTYPE_H_ */

#endif /* NU_PSX_XTYPE_H */


