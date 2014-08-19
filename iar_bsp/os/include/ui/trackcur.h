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
*  trackcur.h                                                          
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*  This file contains the header files to include and the externs.      
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*  None                                                             
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*  None                                                             
*                                                                       
* DEPENDENCIES                                                          
*                                                                       
*  None.
*                                                                       
*************************************************************************/
#ifndef _TRACKCUR_H_
#define _TRACKCUR_H_

extern VOID *NotUsed;
extern INT16 NotUsedInt;

/* default keyboard input record */
extern struct _mouseRcd defKBrd;   

extern VOID G2UP(INT32 GloblX, INT32 GloblY, SIGNED *RtnX, SIGNED *RtnY);

extern VOID nuMoveCursor(SIGNED gblX, SIGNED gblY);
extern VOID HideCursor(VOID);
extern VOID SCREENS_InitBankManager(grafMap *argGRAFMAP);


/* Local Functions */
VOID TC_TrackCursor(INT32 argTF);
VOID TC_QueryCursor(INT32 *argCURXO, INT32 *argCURYO, INT32 *argCURLO, INT32 *argBUTNO);
VOID TC_InputTracker(mouseRcd *trkRecord);

#endif /* _TRACKCURT_H_ */


