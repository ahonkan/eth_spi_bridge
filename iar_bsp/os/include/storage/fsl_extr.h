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
*       fsl_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       FSL
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains defines and structure definitions for mount table 
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
#include "storage/fsl_defs.h"

#ifndef FSL_EXTR_H
#define FSL_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Prototypes for mount table services */
MTE_S* fsl_mte_from_fqpath(CHAR *path);
MTE_S* fsl_mte_from_name(CHAR* name);
MTE_S* fsl_mte_from_dh(UINT16 dh);
INT16  fsl_pc_parsedrive(CHAR  *path, UINT8 use_default);
MTE_S* fsl_mte_from_drive_n(INT16 n);
STATUS fsl_mte_to_mntname(MTE_S* mte, CHAR* mntname);

#ifdef          __cplusplus
}

#endif /* _cplusplus */

#endif /* FSL_EXTR_H */
