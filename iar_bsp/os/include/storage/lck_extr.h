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
*       lck_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Locking
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains defines and structure definitions for locking services
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
#include "storage/lck_defs.h"

#ifndef LCK_EXTR_H
#define LCK_EXTR_H


#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Prototypes for locking routines. */ 
STATUS  lck_init_locks (VOID);

VOID    fs_release(UINT16 dh);
VOID    fs_reclaim(UINT16 dh);

/* Interfacing routines should use macros and not call these routines 
   directly. */
VOID    lck_obtain_lock (UINT16);
VOID    lck_release_lock (UINT16);


#ifdef          __cplusplus
}

#endif /* _cplusplus */

#endif /* LCK_EXTR_H */
