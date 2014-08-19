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
*       date_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Date and time
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains date and time external interface
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
#include "storage/date_defs.h"

#ifndef DATE_EXTR_H
#define DATE_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


DATESTR        *pc_getsysdate(DATESTR *pd);

#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* DATE_EXTR_H */
