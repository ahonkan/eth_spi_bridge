/***************************************************************************
*
*             Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME
*
*  m32b_lin.h
*
* DESCRIPTION
*
*  This file contains the 32 bit line driver prototypes.
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
#ifndef _M32B_LIN_H_
#define _M32B_LIN_H_


/* Local Functions */
VOID  M32BL_ThinLine32Bit(blitRcd *lineRec);
VOID  M32BL_M32BURTable(VOID);
VOID  M32BL_M32BMRTable(VOID);
VOID  M32BL_M32BMOTable(VOID);
VOID  M32BL_M32BUXTable(VOID);
VOID  M32BL_M32BMXTable(VOID);
VOID  M32BL_M32BMATable(VOID);
VOID  M32BL_M32BMO_NDTable(VOID);
VOID  M32BL_M32BMA_NDTable(VOID);

#endif /* _M32B_LIN_H_ */

