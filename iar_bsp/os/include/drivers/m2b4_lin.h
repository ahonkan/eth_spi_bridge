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
*  m2b4_lin.h
*
* DESCRIPTION
*
*  This file contains the 2/4 bit line prototypes
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
#ifndef _M2B4_LIN_H_
#define _M2B4_LIN_H_

/* Local Functions */
VOID M2B4L_ThinLine2_4Bit(blitRcd *lineRec);
VOID M2B4L_M2_4BURTable(VOID);
VOID M2B4L_M2_4BMOTable(VOID);
VOID M2B4L_M2_4BUXTable(VOID);
VOID M2B4L_M2_4BMATable(VOID);
VOID M2B4L_M2_4BMO_NDTable(VOID);
VOID M2B4L_M2_4BMA_NDTable(VOID);

#endif /* _M2B4_LIN_H_ */

