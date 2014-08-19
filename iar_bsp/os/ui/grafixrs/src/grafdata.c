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
*  grafdata.c                                                   
*
* DESCRIPTION
*
*  This function contains misc global variables.
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
*  rs_base.h
*
***************************************************************************/
#include "ui/rs_base.h"

/* Generic Globals */
/* Current global X location */
INT32 LocX;             

/* Current global Y location */
INT32 LocY;             

/* "Global" Clipping Limit Rectangle */
rect ViewClip;          

/* Global coordinate indicator ( < 0 = global) */
INT16 globalLevel;      

/* Error post code */
INT16 grafError;        

/* Graphic flags */        
INT16 gFlags;           


/* Global grafPort and grafMap Variables */
/* pointer to current grafPort */
struct _rsPort *thePort;    

/* Shadow copy of current grafPort */
struct _rsPort theGrafPort;    

/* Default grafPort */
struct _rsPort defPort;     

/* Default grafMap */
struct _grafMap defGrafMap; 


/* Global Blit Record and Blit List */
/* Default blit record */
struct _blitRcd grafBlit;   
struct _grafBlist grafBlist;


/* Indirect Vectors */

#ifdef  USE_CURSOR

/* cursor update (resume) */
VOID (*MovCursIDV)();

#endif      /* USE_CURSOR */

/* vector to current line routine */
SIGNED lineExecIDV;     

#ifdef  THIN_LINE_OPTIMIZE

/* vector to thin line primitive */
SIGNED lineThinIDV;     

#endif  /* THIN_LINE_OPTIMIZE */

#ifdef  DASHED_LINE_SUPPORT

/* vector to dashed line routine (thin or square pen) */    
SIGNED lineDashIDV;     

#endif  /* DASHED_LINE_SUPPORT */

/* vector to patterned thin, square any thin or square pen */
SIGNED linePattIDV;     

/* vector to oval pen (patterned, no dash though!) */
SIGNED lineOvalIDV;     

/* vector to oval pen polyline */
SIGNED lineOvPolyIDV;   

/* vector to square pen polyline */
SIGNED lineSqPolyIDV;   

/* vector to current aligned text function */
INT32 (*txtAlnIDV)();    

/* vector to current plain text function */
INT32 (*txtDrwIDV)();    

/* vector for text align pre-processor */
INT32 (*txtAPPIDV)();    

#ifdef      USE_STROKEDFONT

/* vector for stroked draw function */
INT32 (*txtStrokeIDV)();

#endif      /* USE_STROKEDFONT */

/* vector for StopEvent */
VOID (*stpEventIDV)();  

/* vector for StopMouse */
INT32 (*stpMouseIDV)(); 

/* Line Style Lookup Table */
/* this relates pen flags to a line style IDV */
/* 0 = thin, 1 = dash, 2 = patterned, 3 = oval */
#ifdef  THIN_LINE_OPTIMIZE

const SIGNED * const lineStyleTbl[16] =
{
    &lineThinIDV,                   /* size=0,shape=0,dash=0,pat=0 */
    &linePattIDV,                   /* size=0,shape=0,dash=0,pat=1 */

#ifdef  DASHED_LINE_SUPPORT

    &lineDashIDV,                   /* size=0,shape=0,dash=1,pat=0 */
    &lineDashIDV,                   /* size=0,shape=0,dash=1,pat=1 */

#else
    
    &lineThinIDV,                   /* size=0,shape=0,dash=1,pat=0 */
    &linePattIDV,                   /* size=0,shape=0,dash=1,pat=1 */
    
#endif  /* DASHED_LINE_SUPPORT */

    &lineThinIDV,                   /* size=0,shape=1,dash=0,pat=0 */
    &linePattIDV,                   /* size=0,shape=1,dash=0,pat=1 */

#ifdef  DASHED_LINE_SUPPORT
    
    &lineDashIDV,                   /* size=0,shape=1,dash=1,pat=0 */
    &lineDashIDV,                   /* size=0,shape=1,dash=1,pat=1 */

#else

    &lineThinIDV,                   /* size=0,shape=1,dash=1,pat=0 */
    &linePattIDV,                   /* size=0,shape=1,dash=1,pat=1 */

#endif  /* DASHED_LINE_SUPPORT */
    
    &lineOvalIDV,                   /* size=1,shape=0,dash=0,pat=0 */
    &lineOvalIDV,                   /* size=1,shape=0,dash=0,pat=1 */
    &lineOvalIDV,                   /* size=1,shape=0,dash=1,pat=0 */
    &lineOvalIDV,                   /* size=1,shape=0,dash=1,pat=1 */
    &linePattIDV,                   /* size=1,shape=1,dash=0,pat=0 */
    &linePattIDV,                   /* size=1,shape=1,dash=0,pat=1 */
    
#ifdef  DASHED_LINE_SUPPORT
    
    &lineDashIDV,                   /* size=1,shape=1,dash=1,pat=0 */
    &lineDashIDV                    /* size=1,shape=1,dash=1,pat=1 */

#else

    &linePattIDV,                   /* size=1,shape=1,dash=1,pat=0 */
    &linePattIDV                    /* size=1,shape=1,dash=1,pat=1 */

#endif  /* DASHED_LINE_SUPPORT */

};

#else

const SIGNED * const lineStyleTbl[8] =
{
    &lineOvalIDV,                   /* shape=0,dash=0,pat=0 */
    &lineOvalIDV,                   /* shape=0,dash=0,pat=1 */
    &lineOvalIDV,                   /* shape=0,dash=1,pat=0 */
    &lineOvalIDV,                   /* shape=0,dash=1,pat=1 */
    &linePattIDV,                   /* shape=1,dash=0,pat=0 */
    &linePattIDV,                   /* shape=1,dash=0,pat=1 */

#ifdef  DASHED_LINE_SUPPORT
    
    &lineDashIDV,                   /* shape=1,dash=1,pat=0 */
    &lineDashIDV                    /* shape=1,dash=1,pat=1 */
    
#else

    &linePattIDV,                   /* shape=1,dash=1,pat=0 */
    &linePattIDV                    /* shape=1,dash=1,pat=1 */

#endif  /* DASHED_LINE_SUPPORT */

};

#endif  /* THIN_LINE_OPTIMIZE */

/* Input Device Globals */
/* current input device */
struct _mouseRcd *curInput; 

/* default input record */

struct _mouseRcd defMouse;  


#ifdef  DASHED_LINE_SUPPORT

/* Global Dash Lists */
const UINT8 dashList1[8] = {82,21, 0, 0, 0, 0, 0, 0};
const UINT8 dashList2[8] = {48,21, 0, 0, 0, 0, 0, 0};
const UINT8 dashList3[8] = {14,14, 0, 0, 0, 0, 0, 0};
const UINT8 dashList4[8] = { 7, 7, 0, 0, 0, 0, 0, 0};
const UINT8 dashList5[8] = {48,14,14,14,14,14, 0, 0};
const UINT8 dashList6[8] = {48,14, 7,14, 7,14,48,14};
const UINT8 dashList7[8] = {48,14, 7,14, 7,14, 0, 0};

/* Current dash list */
dashRcd DashTable[8] =
{
    {2, 0, dashList1},
    {2, 0, dashList1},
    {2, 0, dashList2},
    {2, 0, dashList3},
    {2, 0, dashList4},
    {6, 0, dashList5},
    {8, 0, dashList6},
    {6, 0, dashList7}
};

/* Default dashlist */
const dashRcd DefDashTable[8] =
{
    {2, 0, dashList1},
    {2, 0, dashList1},
    {2, 0, dashList2},
    {2, 0, dashList3},
    {2, 0, dashList4},
    {6, 0, dashList5},
    {8, 0, dashList6},
    {6, 0, dashList7}
};

#endif  /* DASHED_LINE_SUPPORT */

#ifdef  USE_CURSOR

/* Cursor Globals */
/* cursor display level (<0 = hidden) */
INT16 CursorLevel;          

/* current cursor X position */
INT32 CursorX;              

/* current cursor Y position */
INT32 CursorY;              

/* current cursor style */
INT16 CursorNumbr;          

/* current cursor colors */
SIGNED CursBackColor;       
SIGNED CursForeColor;

/* pointer to current cursor back color image */
IMAGSTRUCT *CursorMask;    

/* pointer to current cursor fore color image */
IMAGSTRUCT *CursorImag;     

/* cursor "save under" buffer */
UINT8 CursorSave[CursorSaveSize];

/* cursor blit record */
struct _blitRcd cursBlit;   

/* cursor protect rectangle */
/* Protect Xmin */
INT32 ProtXmin;         

/* Protect Ymin */
INT32 ProtYmin;         

/* Protect Xmax */
INT32 ProtXmax;         

/* Protect Ymax */
INT32 ProtYmax;         

/* protect rectangle + hotX/hotY offsets */
/* Protect Xmin */
INT32 CursProtXmin;     

/* Protect Ymin */
INT32 CursProtYmin;     

/* Protect Xmax */
INT32 CursProtXmax;     

/* Protect Ymax */
INT32 CursProtYmax;     

/* cursImag->imRowBytes (need +1 for shifted cases) */
DEFN cursImagBytesM1;     

/* cursImag->imHeight - 1 */
DEFN cursImagHeightM1;    

/* (offsets to 8 active shift-images)x (curMask,curMDta,curImag,curIDta) */
DEFN curActive[8][4];     

/* Translated cursor work buffer ('m' cursors) x (2 masks/cursor) x (8 shift-images/mask) */
UINT8 curBuffer[curBuffSz];  

/* Cursors. In the following order:
   Arrow Cursor
   Checkmark Cursor
   Crosshair Cursor
   Box Cursor
   Pointing-Hand Cursor
   "Hold"ing Hand Cursor
   Question Cursor
   Hourglass Cursor     */
IMAGSTRUCT CursorMasks[8];
IMAGSTRUCT CursorImags[8];

/* Current cursor list */
SIGNED CursorTable[8][4];

/* Default cursor list */
SIGNED DefCursorTable[8][4];

#endif      /* USE_CURSOR */

#ifdef  FILL_PATTERNS_SUPPORT

/* Default Pattern Data */
const struct _FillPat FillPat[32] = 
{
    /* Width,Height,Align,Flags,pad,RowBytes,Bits,Planes */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* Pat00 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}, /* Pat01 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0xEE, 0xBB, 0xEE, 0xBB, 0xEE, 0xBB, 0xEE, 0xBB}}, /* Pat02 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55}}, /* Pat03 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22}}, /* Pat04 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00}}, /* Pat05 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x80, 0x00, 0x08, 0x00, 0x80, 0x00, 0x08, 0x00}}, /* Pat06 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x80, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00}}, /* Pat07 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}}, /* Pat08 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0xC1, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0x83}}, /* Pat09 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x00, 0x44, 0x22, 0x11, 0x00, 0x44, 0x22, 0x11}}, /* Pat10 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11}}, /* Pat11 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0xE3, 0xF1, 0xF8, 0x7C, 0x3E, 0x1F, 0x8F, 0xC7}}, /* Pat12 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}}, /* Pat13 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x83, 0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0, 0xC1}}, /* Pat14 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x00, 0x22, 0x44, 0x88, 0x00, 0x22, 0x44, 0x88}}, /* Pat15 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88}}, /* Pat16 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0xC7, 0x8F, 0x1F, 0x3E, 0x7C, 0xF8, 0xF1, 0xE3}}, /* Pat17 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0xE0}}, /* Pat18 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF}}, /* Pat19 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x80, 0x80, 0x80, 0x00, 0x08, 0x08, 0x08, 0x00}}, /* Pat20 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88}}, /* Pat21 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFF}}, /* Pat22 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x88, 0x88, 0x88, 0xFF, 0x88, 0x88, 0x88, 0xFF}}, /* Pat23 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81}}, /* Pat24 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x11, 0xAA, 0x44, 0xAA, 0x11, 0xAA, 0x44, 0xAA}}, /* Pat25 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0xFF, 0x08, 0x08, 0x08, 0xFF, 0x80, 0x80, 0x80}}, /* Pat26 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0xF8, 0x88, 0x88, 0x88, 0x8F, 0x88, 0x88, 0x88}}, /* Pat27 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x41, 0x80, 0x80, 0xE3, 0x14, 0x08, 0x08, 0x3E}}, /* Pat28 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x00, 0x08, 0x04, 0x02, 0x00, 0x20, 0x40, 0x80}}, /* Pat29 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x81, 0x42, 0x24, 0x18, 0x08, 0x04, 0x02, 0x01}}, /* Pat30 */
    {{   8,    8,    0,    0,    0,    1,    1,    1},
     {0x00, 0x00, 0x0A, 0x04, 0x00, 0x00, 0xA0, 0x40}}  /* Pat31 */
};
        
struct _patList patTable;

#endif /* FILL_PATTERNS_SUPPORT */

/* Work Space GrafMap */
struct _grafMap workGrafMap;

/* pointers to areas allocated via GrafAlloc
    default driver  - DLTable.DLResAdr
    default rowtables - defGrafMap.mapTable
    pool - mpWorkSpace
    default font - defFont */

/* Memory Pool Area */
/* pool size */
DEFN mpSize;            

/* pointer to free pool area */
UINT8 *mpWorkSpace;     

/* pointer to end of pool area */
UINT8 *mpWorkEnd;       

/* size allocated workGrafMaps rowtable */
INT16 mpRowTblSz;       


/* Font Code Work Variables */
/* "current" font vert alignment */
INT16 fntVertAln;       

/* font alignment table entries (4) */
INT16 fntVertTbl[4];    

#ifdef  INCLUDE_DEFAULT_FONT

/* pointer to default font */
signed char *defFont;   

#endif  /* INCLUDE_DEFAULT_FONT */

/* Point In/On support */
/* the result area */
INT16 PtRslt;           

#ifndef NO_REGION_CLIP

/* pen flags save area */
DEFN regPenFlags;       

#endif  /* NO_REGION_CLIP */

#ifdef  INCLUDE_DEFAULT_FONT

/* pointer to embedded font */
signed char *imbFnt;   

#endif  /* INCLUDE_DEFAULT_FONT */

