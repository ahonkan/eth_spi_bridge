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
*  grafdata.h                                                   
*
* DESCRIPTION
*
*  This file contains externs for grafdata.c
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
#ifndef _GRAFDATA_H_
#define _GRAFDATA_H_

/*  Start of graphics data block  */

/* ========== Tracking Globals ==========

The ISR switches to this data area as part of it's service. Our
cursor routine typically requires more stack than the OS provides, so
this special data area is for our tracking ISR. Guessing at the size
is a little problematic, because those nasty mouse drivers can do all
kinds of nutty stuff and use stack like crazy. By placing this data
area at the top of our data space, a variety of these nasty ISR/protected
mode/stack problems can be caught, as a segment violation will occur
should a stack overflow arise.
*/
#include "ui/nu_math.h"
#include "ui/rserrors.h"
                  
extern INT16 ISR_STACK[512];    

extern struct _mouseRcd *curTrack; 

/* Generic Globals */
extern INT32 LocX;              
extern INT32 LocY;              
extern rect ViewClip;           
extern INT16 globalLevel;    

extern INT16 grafError;         
extern INT16 hugeSelAdd;        
extern INT16 biosSeg;           

extern INT16 dllSeg;        
extern INT16 dllFill;
extern SIGNED dllFixup; 
        
extern INT16 gFlags;        

/* 'gFlags' Bit Flags */
#define gfGrafInit  0x01    
#define gfTmrInit   0x02    
#define gfRgnOpen   0x04    
#define gfTrkEnab   0x08    
#define gfCurInit   0x10    
#define gfCurPHid   0x20    
#define gfEvtInit   0x40    
#define gfEvtEnab   0x80    


/* Global grafPort and grafMap Variables */
extern struct _rsPort *thePort;    
extern struct _rsPort theGrafPort;    
extern struct _rsPort defPort;     
extern struct _grafMap defGrafMap; 


/* Global Blit Record and Blit List */
extern struct _blitRcd grafBlit;    
typedef struct _grafBlist{          
    INT32 Xmin;
    INT32 Ymin;

    INT32 Xmax;
    INT32 Ymax;

    INT32 skipStat;
    INT32 temp1;

    INT32 temp2;
    INT32 temp3;
} grafBlistDef;

extern struct _grafBlist grafBlist;


/* Indirect Vectors */
#define IDVStart QueueIDV         

extern VOID (*MovCursIDV)();      

extern SIGNED lineExecIDV;        
extern SIGNED lineThinIDV;        
extern SIGNED lineDashIDV;        
extern SIGNED linePattIDV;        
extern SIGNED lineOvalIDV;        
extern SIGNED lineOvPolyIDV;      
extern SIGNED lineSqPolyIDV;      

extern INT32 (*txtAlnIDV)();       
extern INT32 (*txtDrwIDV)();       
extern INT32 (*txtAPPIDV)();       
extern INT32 (*txtStrokeIDV)();    

extern VOID (*stpEventIDV)();     
extern INT32 (*stpMouseIDV)();    
extern SIGNED IDVEnd;             

/* Line Style Lookup Table */
/* this relates pen flags to a line style IDV */

#ifdef  THIN_LINE_OPTIMIZE

extern const SIGNED * const lineStyleTbl[16]; 

/* this macro uses the pen flags to set the current line style vector */
#define SETLINESTYLE(pen_flags) \
    (lineExecIDV = *lineStyleTbl[(pen_flags & 0xF)])

#else

extern const SIGNED * const lineStyleTbl[8]; 

/* this macro uses the pen flags to set the current line style vector */
#define SETLINESTYLE(pen_flags) \
    (lineExecIDV = *lineStyleTbl[(pen_flags & 0x7)])

#endif  /* THIN_LINE_OPTIMIZE */

/* Shadow of EGA palette hardware */
extern UINT8 EGApaldata[16];     


/* Input Device Globals */
extern struct _mouseRcd *curInput;  
extern struct _mouseRcd defMouse;   
extern SIGNED driverMouse;          
extern SIGNED msSerialMouse;
extern SIGNED moSerialMouse;
extern SIGNED joyMouse;

/* Hi Resolution Timer Globals */
/* user time base setting */
extern SIGNED tmrTimeBase;    


/* Global Dash Lists */
extern const UINT8 dashList1[8];
extern const UINT8 dashList2[8];                                                                           
extern const UINT8 dashList3[8];
extern const UINT8 dashList4[8];
extern const UINT8 dashList5[8];
extern const UINT8 dashList6[8];
extern const UINT8 dashList7[8];

extern dashRcd DashTable[8];    
extern const dashRcd DefDashTable[8]; 


/* Cursor Globals */
#define maxBytePix  2       
#define maxCurSize  32      

typedef struct _IMAGSTRUCT
{
    INT32 imWidth;          
    INT32 imHeight;         

    UINT8 imAlign;          
    UINT8 imFlags;          
        INT16 pad;          

    INT32 imBytes;          
    INT16 imBits;           
    INT16 imPlanes;         

    UINT8 IMGDATA[32];
} IMAGSTRUCT; 

extern INT16 CursorLevel;   
extern INT32 CursorX;       
extern INT32 CursorY;       
extern INT16 CursorNumbr;   
extern SIGNED CursBackColor;
extern SIGNED CursForeColor;
extern IMAGSTRUCT *CursorMask;  
extern IMAGSTRUCT *CursorImag;  

/* cursor "save under" buffer */
#define CursorSaveSize  (sizeof(imageHdr) + (maxCurSize * maxBytePix * maxCurSize))
extern UINT8 CursorSave[CursorSaveSize];

extern struct _blitRcd cursBlit;

/* cursor protect rectangle */
extern INT32 ProtXmin;          
extern INT32 ProtYmin;          
extern INT32 ProtXmax;          
extern INT32 ProtYmax;          

/* protect rectangle + hotX/hotY offsets */
extern INT32 CursProtXmin;      
extern INT32 CursProtYmin;      
extern INT32 CursProtXmax;      
extern INT32 CursProtYmax;      

/* special globals for hypercursor */
extern DEFN cursImagBytesM1;    
extern DEFN cursImagHeightM1;   

extern DEFN curActive[8][4];    
                                

#define curBufSize  (sizeof(imageHdr)+(maxCurSize*((maxCurSize/8)+1)))
#define curBuffSz   (2*8*curBufSize)    
extern UINT8 curBuffer[curBuffSz];      
                                        

/* Cursor Data */

/* Cursors. In the following order:
   Arrow Cursor
   Checkmark Cursor
   Crosshair Cursor
   Box Cursor
   Pointing-Hand Cursor
   "Hold"ing Hand Cursor
   Question Cursor
   Hourglass Cursor     */
   
extern IMAGSTRUCT CursorMasks[];
extern IMAGSTRUCT CursorImags[];

/* Current cursor list */
extern SIGNED CursorTable[8][4];

/* Default cursor list */
extern SIGNED DefCursorTable[8][4];


/* Default Pattern List */
#define patSize (sizeof(imageHdr) + 8); 

/* Default Pattern Data */
typedef struct _FillPat
{
    imageHdr IMAGHDR;
    UINT8 IMGDATA[8];
} FillPatt;

extern const struct _FillPat FillPat[32];

extern struct _patList patTable;

/* Driver & Resource Data */
typedef struct DriverLoaded
{
    INT16 DLnum;        
    INT16 DLcount;      

    SIGNED DLResAddr;   
} DriverLoadedTmp;

#define NUMDLTBL    8   

extern struct DriverLoaded DLTable[NUMDLTBL];   

extern UINT8 rsPathStr[128];   


/* Work Space GrafMap */
extern struct _grafMap workGrafMap;

/* Memory Pool Area */
extern DEFN mpSize;          
extern UINT8 *mpWorkSpace;   
extern UINT8 *mpWorkEnd;     
extern INT16 mpRowTblSz;     


/* Font Code Work Variables */
extern INT16 fntVertAln;        
extern INT16 fntVertTbl[4];     
extern signed char *defFont;    


#ifndef FIXPOINT
    extern float qDxx;
    extern float qDxy;
    extern float qDyy;
    extern float qDyx;
    extern float qPxx;
    extern float qPxy;
    extern float qPyy;
    extern float qPyx;
    extern float sinSlant;
    extern float cosSlant;
#else
    extern SIGNED qDxx;
    extern SIGNED qDxy;
    extern SIGNED qDyy;
    extern SIGNED qDyx;
    extern SIGNED qPxx;
    extern SIGNED qPxy;
    extern SIGNED qPyy;
    extern SIGNED qPyx;
    extern SIGNED sinSlant;
    extern SIGNED cosSlant;
#endif

extern UINT8 qDxxSign;
extern UINT8 qDxySign;
extern UINT8 qDyySign;
extern UINT8 qDyxSign;
extern UINT8 qPxxSign;
extern UINT8 qPxySign;
extern UINT8 qPyySign;
extern UINT8 qPyxSign;


/* Point In/On support */
extern INT16 PtRslt;          


/* Region Capture support */
extern DEFN regPenFlags;      


/* XMS memory support Variables */
extern SIGNED XMSManager;
extern SIGNED xmsLength;
extern INT16 xmsSrcHand;
extern INT16 xmsDstHand;
extern SIGNED xmsSrcOff;
extern SIGNED xmsDstOff;


/* Copyright Notice */
/* This copyright is checked by InitGrafix() and must not be altered */
extern signed char Copyright[58];

extern signed char *imbFnt; 

VOID NU_InitGrafVars(VOID); 
                            
#endif /* _GRAFDATA_H_ */




