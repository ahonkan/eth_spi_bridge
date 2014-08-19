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
*  rsextern.h                                                   
*
* DESCRIPTION
*
*  This file contains the externs.
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
#ifndef _RSEXTERN_H_
#define _REEXETRN_H_

#ifdef    WatcomC
#define SEQ cdecl
#endif /* WatcomC */

#ifdef    TopSpeedC
#define SEQ far cdecl
#endif /* TopSpeedC */

#ifdef    __cplusplus      /* If C++, disable "mangling" */
extern "C" {
#endif /* __cplusplus */

#include "nucleus.h"
#include "ui/global.h"

#ifndef SEQ
#define SEQ
#endif 


VOID       AddPt          (point *srcPt, point *dstPt);
VOID       AlignPattern   (INT32 patNbr, INT32 pixX, INT32 pixY);
VOID       BackColor      (INT32 argCOLOR);
VOID       BackPattern    (INT32 patNDX);
region *   BitmapToRegion (rect *srcRect, INT32 transPColor);
VOID       BorderColor    (INT32 argCOLOR);
VOID       CenterRect     (point *PT, INT32 widthX, INT32 heightY, rect *dstR);
INT32      CharWidth      (signed char CH);
INT32      CharWidth16    (UINT16 CH);
VOID       ClipRect       (rect *cRect);
VOID       ClipRegion     (region *RGN);
INT32      CloseBitmap    (grafMap *argCGMAP);
region *   CloseRegion    (VOID);

STATUS     CopyBlit       (grafPort *srcPORT, grafPort *dstPORT, rect *argSrcR,
                           rect *argDstR);

grafPort * CreateBitmap   (INT32 aEMTYPE, INT32 aWIDTH, INT32 aHEIGHT);
VOID       CursorBitmap   (grafMap *argBMAP);
VOID       CursorColor    (INT32 foreCOLOR, INT32 backCOLOR);
VOID       CursorStyle    (INT32 argCURNO);
VOID       DashStyle      (INT32 DASHSTYL);

VOID       DefineCursor   (INT32 argCNBR, INT32 argHOTX, INT32 argHOTY,
                           cursor *argBACKIMG, cursor *argFOREIMG);

VOID       DefineDash     (INT32 argStyle, dashRcd *dashArray);
VOID       DefinePattern  (INT32 patNDX, pattern *adsPAT);
VOID       DestroyBitmap  (grafPort *offPORT);
VOID       DestroyRegion  (region *rgn);
VOID       DupPt          (point *srcPt, point *dstPt);
VOID       DupRect        (rect *srcRect, rect *dstRect);
region *   SEQ  DupRegion      (region *rsSRCRGN);
INT32      SEQ  EmptyRect      (rect *rsTESTRECT);
INT32      SEQ  EmptyRegion    (region *rsTESTREGION );
INT32      SEQ  EqualPt        (point *rsPT1, point *rsPT2);
INT32      SEQ  EqualRect      (rect *rsRECT1, rect *rsRECT2);
INT32      SEQ  EqualRegion    (region *rsRGN1, region *rsRGN2);
VOID       SEQ  EVENTH_EventQueue     (INT32 rsTF);

VOID       SEQ  FillPolygon    (point *rsVERTICES, INT32 rsVERTICOUNT,
                                INT32 rsVERTIFORM, INT32 rsPOLYSHAPE); 

VOID       SEQ  FillRule       (INT32 rsRULE);
INT32      SEQ  FindClosestRGB (palData *rsRGBCOLOR, palData *rsRGBPALETTE );

VOID       SEQ  Gbl2LclPt      (point *rsPT);
VOID       SEQ  Gbl2LclRect    (rect *rsR);
VOID       SEQ  Gbl2VirPt      (point *rsPT);
VOID       SEQ  Gbl2VirRect    (rect *rsR);
VOID       SEQ  GetPenState    (penState *rsSTATE);
INT32      SEQ  GetPixel       (INT32 rsX, INT32 rsY);
VOID       SEQ  GetPort        (grafPort **rsPORTPTR);

VOID       SEQ  EVENTH_GrafQueue      (INT32 rsMSGCNT );
VOID       SEQ  HideCursor     (VOID);
VOID       SEQ  HidePen        (VOID);
SIGNED     SEQ  ImageSize      (rect *rsR);
INT32      SEQ  InceptRect     (rect *rsRECT1, rect *rsRECT2, rect *rsDSTRECT);
INT32      PD_InitInputDevice  (INT32 rsDEVICE);
VOID       SEQ  InitPort       (grafPort *rsPORT);
VOID       InitRect            (rect *r, INT32 xStart, INT32 yStart, INT32 width, INT32 height);

VOID  SCREENS_InitRowTable     (grafMap *rsNEWMAP, INT32 rsINTERLEAVE, 
                                INT32 rsINTERSEG, INT32 rsINTEROFF);

VOID       SEQ  InsetRect      (rect *rsR, INT32 rsDX, INT32 rsDY);
INT32      SEQ  EVENTH_KeyEvent       (INT32 rsWAIT, rsEvent *rsEVNT);
VOID       SEQ  Lcl2GblPt      (point *rsPT);
VOID       SEQ  Lcl2GblRect    (rect *rsR);
VOID       SEQ  Lcl2VirPt      (point *rsPT);
VOID       SEQ  Lcl2VirRect    (rect *rsR);
VOID       PD_LimitMouse      (INT32 rsXMIN, INT32 rsYMIN, INT32 rsXMAX, INT32 rsYMAX);
VOID       SEQ  LineRel        (INT32 rsDLTX, INT32 rsDLTY);
VOID       SEQ  LineTo         (INT32 rsX, INT32 rsY);

VOID       SEQ  MapPoly        (INT32 rsPOLYCNT, polyHead *rsPOLYHDR,
                                point *rsPOLYPTS, rect *rsSRCR, rect *rsDSTR);

VOID       SEQ  MapPt          (point *rsPT, rect *rsSRCR, rect *rsDSTR);
VOID       SEQ  MapRect        (rect *rsR, rect *rsSRCR, rect *rsDSTR);
VOID       SEQ  EVENTH_MaskEvent      (INT32 rsMASK);
VOID       PD_MaskMouse       (INT32 rsMASK);
VOID       SEQ  MoveCursor     (INT32 rsX, INT32 rsY);
VOID       SEQ  MovePortTo     (INT32 rsGBLLEFT, INT32 rsGBLTOP);
VOID       SEQ  MoveRel        (INT32 rsDLTX, INT32 rsDLTY);
VOID       SEQ  MoveTo         (INT32 rsX, INT32 rsY);
VOID       SEQ  NullRect       (rect *rsDSTRECT);
VOID       SEQ  NullRegion     (region *rsDSTRGN);

VOID       SEQ  OffsetPoly     (INT32 rsPOLYCNT, polyHead *rsPOLYHDR,
                                point *rsPOLYPTS, INT32 rsDX, INT32 rsDY);

VOID       SEQ  OffsetRect     (rect *rsR, INT32 rsDX, INT32 rsDY);
VOID       SEQ  OffsetRegion   (region *rsRGN, INT32 DX, INT32 DY );
VOID       SEQ  OpenRegion     (VOID);
VOID       SEQ  OvalPt         (rect *rsR, INT32 rsANGLE, point *rsPT);
INT32      SEQ  EVENTH_PeekEvent      (INT32 rsINDEX, rsEvent *rsEVNT);
VOID       SEQ  PenOffset      (INT32 rsDASHOFFSET);
VOID       SEQ  PlaneMask      (SIGNED rsPMASK);
VOID       SEQ  PolyMarker     (INT32 rsNPTS, point *rsXYPTS);
VOID       SEQ  PolySegment    (INT32 rsSEGCNT, segPts *rsSEGLIST);
VOID       SEQ  PopGrafix      (pusharea *rsSAVEAREA);
VOID       SEQ  PortBitmap     (grafMap *rsNEWMAP);
VOID       SEQ  PortOrigin     (INT32 rsTOPBOT);
VOID       SEQ  PortPattern    (patList *rsPATTERNLIST); 
VOID       SEQ  PortSize       (INT32 rsPIXWIDTHX, INT32 rsPIXHEIGHTY);
VOID       SEQ  ProtectRect    (rect *rsR);
VOID       SEQ  Pt2Rect        (point *rsPT1, point *rsPT2, rect *rsR);

INT32      PtInArc             (point *rsTESTPT, rect *rsR, INT16 rsBGNANGLE,
                                INT16 rsARCANGLE, INT32 rsSIZEX, INT32 rsSIZEY);

INT32      SEQ  PtInBoundary   (point *rsTESTPT, point *rsSEEDPT,
                                INT32 rsBOUNDCOLOR, rect *rsSCANRECT,
                                INT32 rsPIXWIDX, INT32 rsPIXHGTY);

INT32      SEQ  PtInFlood      (point *rsTESTPT, point *rsSEEDPT, 
                                rect *rsSCANRECT, INT32 rsPIXWIDX,
                                INT32 rsPIXHGTY);

INT32      SEQ  PtInOval       (point *rsTESTPT, rect *rsR, INT32 rsSIZEX, INT32 rsSIZEY);

INT32      SEQ  PtInPoly       (point *rsTESTPT, INT32 rsPOLYCNT, polyHead *rsPOLYHDRS,
                                point *rsPOLYPTS, INT32 rspenX, INT32 rspenY);

INT32      SEQ  PtInRect       (point *rsTESTPT, rect *rsR, INT32 rsSIZEX, INT32 rsSIZEY);
INT32      SEQ  PtInRegion     (point *rsTESTPT, region *rsRGN,
                                INT32 rsPIXWIDX, INT32 rsPIXHGTY );

INT32      SEQ  PtInRoundRect  (point *rsTESTPT, rect *rsR, INT32 rsDIAX, INT32 rsDIAY,
                                INT32 rsSIZEX, INT32 rsSIZEY);

INT32      SEQ  PtOnArc        (point *rsTESTPT, rect *rsR,
                                INT16 rsBGNANGLE, INT16 rsARCANGLE,
                                INT32 rsSIZEX, INT32 rsSIZEY);

INT32      SEQ  PtOnLine       (point *rsTESTPT, point *rsPT1, point *rsPT2,
                                INT32 rsSIZEX, INT32 rsSIZEY);

INT32      SEQ  PtOnOval       (point *rsTESTPT, rect *rsR, INT32 rsSIZEX, INT32 rsSIZEY);
INT32      SEQ  PtOnPoly       (INT32 rspolycnt, polyHead *rspolyhdrs, point *rspolypts);
INT32      SEQ  PtOnRect       (point *rsTESTPT, rect *rsR, INT32 rsSIZEX, INT32 rsSIZEY);

INT32      SEQ  PtOnRoundRect  (point *rsTESTPT, rect *rsR, INT32 rsDIAX, INT32 rsDIAY,
                                INT32 rsSIZEX, INT32 rsSIZEY);

INT32      SEQ  PtToAngle      (rect *rsR, point *rsPT);
VOID       SEQ  PushGrafix     (pusharea *rsSAVEAREA);

UINT32     GFX_GetScreenSemaphore(VOID);
UINT32     GFX_ReleaseScreenSemaphore(VOID);

SIGNED     SEQ  QueryColors    (VOID);

VOID       TC_QueryCursor      (INT32 *rsCURSORX, INT32 *rsCURSORY,
                                INT32 *rsCURSORLEVEL,INT32 *rsBUTTONS);

INT32      SEQ  QueryError     (VOID);
INT32      SEQ  SCREENS_QueryGraphics  (INT32);
INT32      PD_QueryMouse     (INT32);
VOID       SEQ  QueryPosn      (INT32 *X, INT32 *Y);
VOID       SEQ  QueryRes       (INT32 *rsRESX, INT32 *rsRESY);
INT32      SEQ  QueryX         (VOID);
INT32      SEQ  QueryY         (VOID);
VOID       SEQ  RasterOp       (INT32 rsRASOP);
VOID       SEQ  ReadImage      (rect *rsSRCR, image *rsDSTIMAGE);
VOID       PD_ReadMouse       (INT32 *X, INT32 *Y, INT32 *rsSW);
VOID       SEQ  ReadPalette    (INT32 rsPALNBR, INT32 rsBGNINDEX, INT32 rsENDINDEX,
                                palData *rsRGBCOLORS);
INT32      SEQ  RectInRegion   (rect *rsR, region *rsRGN ); 
region *   SEQ  RectListToRegion(INT32 rsNUMRECTS, rect *rsRLIST);
region *   SEQ  RectRegion     (rect *rsSRCRECT);
VOID       PD_ScaleMouse      (INT32 rsFACTORX, INT32 rsFACTORY);
VOID       SEQ  ScalePt        (point *rsPT, rect *rsSRCR, rect *rsDSTR);
VOID       SEQ  ScreenRect     (rect *rsSCRNR);
VOID       SEQ  ScrollRect     (rect *rsR, INT32 rsDX, INT32 rsDY);
VOID       SEQ  SetBitmap      (INT32 rsPAGE, grafMap *rsNEWMAP);
VOID       SEQ  SetDisplay     (INT32 rsPAGE);
VOID       SEQ  SetFont        (fontRcd *rsFONTBUFFER);
VOID       SEQ  SetLocal       (VOID);
VOID       PD_SetMouse        (mouseRcd *rsLOCATOR);
VOID       SEQ  SetOrigin      (INT32 rsLCLX, INT32 rsLCLY);
VOID       SEQ  SetPenState    (penState *rsSTATE);
VOID       SEQ  SetPixel       (INT32 X, INT32 Y);
VOID       SEQ  SetPort        (grafPort *rsNEWPORT);
VOID       SEQ  SetPt          (point *rsPT, INT32 X, INT32 Y);
VOID       SEQ  SetRect        (rect *rsR, INT32 X1, INT32 Y1, INT32 X2, INT32 Y2);
region *   SEQ  SetRegion      (INT32 X1, INT32 Y1, INT32 X2, INT32 Y2);
VOID       SEQ  SetVirtual     (VOID);
INT32      SEQ  ShiftRect      (rect *rsR, INT32 DX, INT32 DY, rect *rsR1, rect *rsR2);
VOID       SEQ  ShowCursor     (VOID);
VOID       SEQ  ShowPen        (VOID);
VOID       SEQ  EVENTH_StopEvent      (VOID);
INT32      SEQ  StopGraphics   (VOID);
INT32      PD_StopMouse       (VOID);
INT32      SEQ  EVENTH_StoreEvent     (rsEvent *rsEVNT);

INT32      SEQ  StringWidth    (UNICHAR *rsTEXTSTR);
INT32      SEQ  StringWidth16  (UNICHAR *rsTEXTSTR);

VOID       SEQ  SubPt          (point *rsSRCPT, point *rsDSTPT);

INT32      SEQ  TextWidth      (UNICHAR *rsTEXTSTR, INT32 rsINDEX, INT32 rsCOUNT);
INT32      SEQ  TextWidth16    (UNICHAR *rsTEXTSTR, INT32 rsINDEX, INT32 rsCOUNT);

VOID       TC_TrackCursor      (INT32 rsTF);
VOID       SEQ  UnionRect      (rect *rsRECT1, rect *rsRECT2, rect *rsDSTRECT);
VOID       SEQ  Vir2GblPt      (point *rsPT);
VOID       SEQ  Vir2GblRect    (rect *rsR);
VOID       SEQ  Vir2LclPt      (point *rsPT);
VOID       SEQ  Vir2LclRect    (rect *rsR);
VOID       SEQ  VirtualRect    (rect *rsR);
VOID       SEQ  WriteImage     (rect *rsDSTR, image *rsSRCIMAGE);
VOID       SEQ  WritePalette   (INT32 rsPALNBR, INT32 rsBGNINDEX, INT32 rsENDINDEX,
                                palData *rsRGBCOLORS);
SIGNED     SEQ  XlateImage     (image *rsSRCIMAGE, image *rsDSTIMAGE,
                                INT32 rsDSTBITS, INT32 rsDSTPLANES, SIGNED *rsXLTABLE);
VOID       XlateColors         (SIGNED *xlt, UINT8 imBits, UINT8 imPlanes,  
                                palData *palette, palData *imgPal );
INT32      SEQ  XYInRect       (INT32 X, INT32 Y, rect *R);
INT32      SEQ  ZoomBlit       (grafPort *rsSRC, grafPort *rsDST,
                                rect *rsSRCRECT, rect *rsDSTRECT);

VOID  RS_Set_Alpha(double trans_level, BOOLEAN set_text_trans);

#ifdef __cplusplus
}
#endif
                           

#endif /* _RSEXTERN_H_ */




