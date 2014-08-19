/***************************************************************************
*
*               Copyright 2004 Mentor Graphics Corporation
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
*  pointing_device.h
*
* DESCRIPTION
*
*  This file contains prototypes and externs for pointing_device.c
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
#ifndef _POINTING_DEVICE_H_
#define _POINTING_DEVICE_H_

extern INT32 EVENTH_StoreEvent(rsEvent *rsEVNT);
extern VOID  EVENTH_InputCallBack(mouseRcd *cbRecord);

/* Local Functions */
#ifdef USING_DIRECT_X
VOID  PD_MouseMoves(HWND hWnd,UINT32 message,WPARAM wParam,LPARAM lParam);
VOID  PD_QueryMousePosition(SIGNED *x, SIGNED *y, INT32 *level, INT32 *button);
#endif


INT32 PD_MouseEvent(rsEvent * pRSEvent);
INT32 PD_InitInputDevice(INT32 argDEV);
INT32 PD_QueryMouse(INT16 argDEV);
INT32 PD_StopMouse(VOID);
VOID  PD_ReadMouse(SIGNED *argX, SIGNED *argY, INT32* argButtons);
VOID  PD_LimitMouse(SIGNED argX1, SIGNED argY1, SIGNED argX2, SIGNED argY2);
VOID  PD_ScaleMouse(SIGNED argRX, SIGNED argRY);
VOID  PD_SetMouse(mouseRcd *argMouseR);
VOID  PD_MaskMouse(INT32 argMask);


/* Internal Input Device Managers */
/* Mouse Device Structure */
typedef struct _MouseDevc{
    INT32 devcName;               /* Name of device                   */
    INT32 (*devTblPtr)();         /* Pointer to device driver table   */
} MouseDevc;


extern INT32 whichButton;

#define swLeftb         1
#define swRightb        2
#define swMiddleb       4
#define swAllb          7

#endif /* _POINTING_DEVICE_H_ */

