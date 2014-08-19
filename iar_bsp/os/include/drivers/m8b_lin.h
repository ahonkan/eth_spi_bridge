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
*  m8b_lin.h
*
* DESCRIPTION
*
*  This file contains the 8 bit Line driver prototypes.
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
#ifndef _M8B_LIN_H_
#define _M8B_LIN_H_

/* Local Functions */
VOID M8BL_ThinLine8Bit(blitRcd *lineRec);
VOID M8BL_M8BURTable(VOID);
VOID M8BL_M8BMRTable(VOID);
VOID M8BL_M8BMOTable(VOID);
VOID M8BL_M8BUXTable(VOID);
VOID M8BL_M8BMXTable(VOID);
VOID M8BL_M8BMATable(VOID);
VOID M8BL_M8BMO_NDTable(VOID);
VOID M8BL_M8BMA_NDTable(VOID);


#endif /* _M8B_LIN_H_ */

