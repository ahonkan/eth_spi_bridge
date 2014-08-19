/************************************************************************
*
*             Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*  m16b_lin.h
*
* DESCRIPTION
*
*  This file contains the 16 bit line driver prototypes.
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
#ifndef _M16B_LIN_H_
#define _M16B_LIN_H_


/* Local Functions */
VOID M16BL_ThinLine16Bit(blitRcd *lineRec);
VOID M16BL_M16BURTable(VOID);
VOID M16BL_M16BMRTable(VOID);
VOID M16BL_M16BMOTable(VOID);
VOID M16BL_M16BUXTable(VOID);
VOID M16BL_M16BMXTable(VOID);
VOID M16BL_M16BMATable(VOID);
VOID M16BL_M16BMO_NDTable(VOID);
VOID M16BL_M16BMA_NDTable(VOID);

#endif /* _M16B_LIN_H_ */

