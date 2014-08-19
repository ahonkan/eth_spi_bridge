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
*       vnode_defs.h
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
*       VNODE
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#ifndef VNODE_DEFS_H
#define VNODE_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


typedef struct fsv_vnode_struct
{
    VOID *vnode_fsnode;     /* Pointer to a FS specific implementation of a
                               node */
}VNODE;


#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* VNODE_DEFS_H */
