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
*       strings.h
*
* COMPONENT
*
*       RTL - RunTime Library.
*
* DESCRIPTION
*
*       This file contains the routines for various string operations.
*
* DATA STRUCTURES
*
*       size_t          An unsigned integer data type.
*
* DEPENDENCIES
*
*       "stddef.h"      STDDEF related definitions.
*
*************************************************************************/

#ifndef NU_PSX_STRINGS_H
#define NU_PSX_STRINGS_H

#include "services/config.h"
#include "services/compiler.h"

/* For MinGNU or other GNU toolsets  */
#ifndef _STRINGS_H_
#define _STRINGS_H_

/* For Microsoft Visual C */
#ifndef __STRINGS_H_
#define __STRINGS_H_

#include "services/stddef.h"

#ifdef __cplusplus
extern "C" {
#endif

int  bcmp(const void *, const void *, size_t);
void bcopy(const void *, void *, size_t);
void bzero(void *, size_t);
int  ffs(int);

#ifdef __cplusplus
}
#endif

#endif /* _STRINGS_H_ */
#endif /* __STRINGS_H_ */

#endif /* NU_PSX_STRINGS_H */
