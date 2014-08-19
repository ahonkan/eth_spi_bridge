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
*  createbmp.h                                                  
*
* DESCRIPTION
*
*  This file contains prototypes and externs for createbmp.h.
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
#ifndef _CREATEBMP_H_
#define _CREATEBMP_H_

extern NU_MEMORY_POOL System_Memory;

extern INT32 SCREENI_InitBitmap(INT32 argDEVICE, grafMap *argGRAFMAP);
extern INT32 SCREENS_CloseGrafDriver( grafMap *argGRAFMAP);
extern VOID  InitPort(rsPort *argPORT);
extern VOID  PortBitmap(grafMap *ptrBMAP);
extern VOID  PortSize(INT32 psWDX, INT32 psHTY);
extern VOID  Vir2LclRect(rect *argR);
extern VOID  COORDS_rsGblClip(rsPort *inpClpPort, rect *outClipR);
extern STATUS  CopyBlit( rsPort *srcPORT, rsPort *dstPORT, rect *argSrcR, rect *argDstR );


/* Local Functions */
rsPort  *CreateBitmap(INT32 aMEMTYPE, INT32 aWIDTH, INT32 aHEIGHT);
VOID     DestroyBitmap(rsPort *offPort);
INT32    CloseBitmap( grafMap *argCGMAP);
VOID     ClipRect (rect *cRECT);

#endif /* _CREATEBMP_H_ */





