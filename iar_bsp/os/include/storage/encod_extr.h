/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*
*       encod_extr.h
*
* COMPONENT
*
*       Encode
*
* DESCRIPTION
*
*       Contains defines and structure definitions for encode
*       services.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
*************************************************************************/
#include "storage/encod_defs.h"

#ifndef ENCOD_EXTR_H
#define ENCOD_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

UINT8   cp_2_utf8(UINT8 *utf8,UINT8 *cp);
UINT8   utf8_2_cp(UINT8 *cp, UINT8 *utf8);
UINT8   utf8_to_unicode(UINT8 *unicode,UINT8 *utf8);
UINT8   unicode_to_utf8(UINT8 *utf8,UINT8 *unicode);
STATUS  ascii_to_cp_format(UINT8 *cp_format,UINT8 *ascii_cp);
STATUS  ascii_cp_format_to_ascii(UINT8 *cp_format);

#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* VNODE_EXTR_H */
