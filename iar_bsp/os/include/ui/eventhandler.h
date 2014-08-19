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
*  eventhandler.h                                                      
*
* DESCRIPTION
*
*  This file contains prototypes and externs for eventhandler.c
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
#ifndef _EVENTHANDLER_H_
#define _EVENTHANDLER_H_


#define kbOldVect   0       /* old key ISR save vector */
#define kbOldSeg    4       /* old key ISR save seg (386) */
#define kbOldxVect  6       /* old key ISR alt (real mode) save vector */
#define kbTmp       10      /* temp storage */
#define KBDVEC      0x09    /* Hardware INT32 */

/* The externs needed for event 0 */
extern NU_MEMORY_POOL  System_Memory;
extern VOID *NotUsed;
extern INT16 NotUsedInt;
extern VOID KB_KeyBoardDriverISR(VOID);
extern VOID G2UP(INT32 GloblX, INT32 GloblY, INT32 *RtnX, INT32 *RtnY);

/* Local Functions */
VOID  EVENTH_GrafQueue(INT32 rsMSGCNT );
VOID  EVENTH_EventQueue(INT32 argTF);
VOID  EVENTH_StopEvent(VOID);
INT32 EVENTH_KeyEvent(INT32 argW, rsEvent *argE);
INT32 EVENTH_PeekEvent(INT32 argNDX, rsEvent *argEVN);
VOID  EVENTH_MaskEvent(INT16 argMASK);
INT32 EVENTH_StoreEvent(rsEvent *argEV);
INT32 EVENTH_rsStoreEvent(rsEvent *argEV);
VOID  EVENTH_makeUsrEvent(SIGNED evntPntr, rsEvent *usrEvnt);
VOID  EVENTH_makeNullEvent(rsEvent *nullEvent);
VOID  EVENTH_InputCallBack(mouseRcd *cbRecord);
VOID  EVENTH_CallrsGetVect(INT16 intNmbr, UINT8 *OldVect, UINT8 *OldSeg, UINT8 *OldxVect);
VOID  EVENTH_CallrsSetVect(INT16 intNmbr, INT32 NewVect);
VOID  EVENTH_CallrsRestVect(INT16 intNmbr, UINT8 *OldVect, UINT8 *OldSeg, UINT8 *OldxVect);


#endif /* _EVENTHANDLER_H_ */

