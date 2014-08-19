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
*       vnode_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       VNODE 
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains defines and structure definitions for VNODE 
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
#include "storage/vnode_defs.h"

#ifndef VNODE_EXTR_H
#define VNODE_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

STATUS fsv_init_vnodes(UINT16 dh);
STATUS fsv_release_vnodes(UINT16 dh);
STATUS fsv_get_vnode(UINT16 dh, VOID **fsnode);
STATUS fsv_set_vnode(UINT16 dh, VOID *fsnode);

#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* VNODE_EXTR_H */
