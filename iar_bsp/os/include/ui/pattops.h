/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  pattops.h                                                    
*
* DESCRIPTION
*
*  This file contains prototypes and externs for pattops.c
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _PATTOPS_H_
#define _PATTOPS_H_

#define maxPatSize  32

/* Local Functions */
VOID DefinePattern( INT32 patNDX, pattern *adsPAT);
VOID AlignPattern( INT32 patNbr, INT32 pixX, INT32 pixY);

/* Align Pattern definitions */
#define APAP1   0
#define APNAP1  1
#define APAP2D  0
#define APAP2   1
#define APNAP2D 2
#define APNAP2  3
#define APBbot  4

#endif /* _PATTOPS_ */




