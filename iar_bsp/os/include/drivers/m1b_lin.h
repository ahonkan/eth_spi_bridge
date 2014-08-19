/***************************************************************************
*
*             Copyright 2003 Mentor Graphics Corporation
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
*   m1b_lin.h
*
* DESCRIPTION
*
*  This file contains the 1 bit line prototypes.
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
#ifndef _M1B_LIN_H_
#define _M1B_LIN_H_

/* Local Functions */
VOID M1BL_ThinLine1Bit(blitRcd *lineRec);
VOID M1BL_M1BURTable(VOID);
VOID M1BL_M1BMOTable(VOID);
VOID M1BL_M1BUXTable(VOID);
VOID M1BL_M1BMATable(VOID);
VOID M1BL_M1BMO_NDTable(VOID);
VOID M1BL_M1BMA_NDTable(VOID);

#endif /* _M1B_LIN_H_ */

