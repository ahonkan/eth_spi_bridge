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
*  m24b_lin.h
*
* DESCRIPTION
*
*  This file contains the 24 bit line driver prototypes.
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
#ifndef _M24B_LIN_H_
#define _M24B_LIN_H_


/* Local Functions */
VOID  M24BL_ThinLine24Bit(blitRcd *lineRec);
VOID  M24BL_M24BURTable(VOID);
VOID  M24BL_M24BMRTable(VOID);
VOID  M24BL_M24BMOTable(VOID);
VOID  M24BL_M24BUXTable(VOID);
VOID  M24BL_M24BMXTable(VOID);
VOID  M24BL_M24BMATable(VOID);
VOID  M24BL_M24BMO_NDTable(VOID);
VOID  M24BL_M24BMA_NDTable(VOID);

#endif /* _M24B_LIN_H_ */

