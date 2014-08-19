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
*  cursor.h                                                     
*
* DESCRIPTION
*
*  This file contains prototypes and externs for Cursor.c
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
#ifndef _CURSOR_H_
#define _CURSOR_H_

#ifdef      USE_CURSOR

extern VOID U2GR(rect UserRect, rect *RtnRect, INT16 frame);
extern VOID U2GP(INT32 UserX, INT32 UserY, INT32 *RtnX, INT32 *RtnY, INT16 frame);
VOID BLITS_SaveGlobals(VOID);
VOID BLITS_RestoreGlobals(VOID);

#ifdef hyperCursor
extern VOID CURSOR_rsInitShftCurs(image *srcMASK, image *srcIMAG);
extern VOID mwMoveCurEGA (INT32 gblXMIN, INT32 gblYMIN);
#endif

/* Local Functions */
VOID HideCursor(VOID);
VOID ShowCursor(VOID);
VOID MoveCursor (INT32 argMCX, INT32 argMCY);
VOID CursorStyle (INT32 argCURNO);
VOID CursorBitmap (grafMap *argBMAP);
VOID nuMoveCursor(INT32 glblX, INT32 glblY);
VOID nuResume(grafMap *argGRAFMAP);
VOID CursorColor (INT32 foreCOLOR, INT32 backCOLOR);
VOID CURSOR_rsInitShftCurs(image *srcMASK, image *srcIMAG);
VOID ProtectRect(rect *argPR);
VOID ProtectOff(VOID);
VOID DefineCursor(INT32 argCNBR, INT32 argHOTX, INT32 argHOTY, image *argBACKIMG, image *argFOREIMG);

#endif /* USE_CURSOR */
#endif /* _CURSOR_H_ */





