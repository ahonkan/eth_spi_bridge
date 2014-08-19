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
*  rsconst.h                                                    
*
* DESCRIPTION
*
*  This file contains the Structure Definitions.
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
#ifndef _RSCONST_H_
#define _RSCONST_H_

#define GRAFIX_MEMORY_SIZE   2000000L

/* Standard Graphics Adaptor Definitions Modes */
/*======================================================*/
/* The lower two nibbles are typically device variants. */

/* no graphics device       */
#define cNODEVICE   0x0000      

/* Conventional Memory      */
#define cMEMORY     cNODEVICE+1 

/* User defined bitmap      */
#define cUSER       cMEMORY+2   

/*=======================================*/
#define cLCD        0x0100   /* LCD drivers */
/*=======================================*/

/* 8-bit      per pixel */
#define cLCD8       cLCD+0      

/* 1-bit      per pixel */
#define cLCD1       cLCD+1      

/* 2-bit      per pixel */
#define cLCD2       cLCD+2      

/* 4-bit      per pixel */
#define cLCD4       cLCD+3      

/* 16-bit     per pixel */
#define cLCD16      cLCD+4      

/* 24-bit     per pixel */
#define cLCD24      cLCD+5      

/* 32-bit     per pixel */
#define cLCD32      cLCD+6 

/* VGA 8-bit  per pixel */
#define cVGA8       cLCD+7

/* VGA 16-bit per pixel */
#define cVGA16      cLCD+8

/* SVGA 8-bit per pixel */
#define cSVGA8      cLCD+9    

/* Standard Input device Definitions Definitions Modes */
/*======================================================*/
/* Mouse driver (version 6 or newer) */
#define cMSDRIVER   0x01   

/* Keyboard driver */
#define cKEYBOARD   0x02   

/* Keypad driver */
#define cKEYPAD     0x04   

/* Touch panel driver */
#define cTOUCH      0x51   

/* Point Data Structure */
typedef struct _point16
{
    /* X coordinate */
    INT16 X;            

    /* Y coordinate */
    INT16 Y;            
} point16;

typedef struct _point
{
    /* X coordinate */
    INT32 X;            

    /* Y coordinate */
    INT32 Y;            
} point;

/* Line Data Structure */
typedef struct _lineS
{
    /* start point       */
    point lStart;       

    /* end point         */
    point lEnd;         

    /* first/last status */
    INT16 flStat;       

    /* Padding for the structure */
    INT16 pad;          
} lineS;

/* Rectangle Data Structure */
typedef struct _rect
{
    /* minimum X */    
    INT32 Xmin;         

    /* minimum Y */
    INT32 Ymin;         

    /* maximum X */
    INT32 Xmax;         

    /* maximum Y */
    INT32 Ymax;         
} rect;

/* Polygon "header" structure */
typedef struct _polyHead
{
    /* beginning index */
    UINT16 polyBgn;      

    /* ending index    */
    UINT16 polyEnd;      

    /* boundary limits  */
    rect polyRect;       
} polyHead;

/* Palette Data Structure */
typedef struct _palData
{
    /* Red intensity   */
    UINT16 palRed;       

    /* Green intensity */
    UINT16 palGreen;     

    /* Blue intensity  */
    UINT16 palBlue;      
} palData;

/*Image Header Data Structure */
typedef struct _imageHdr
{
    /* Image pixel width (X)  */
    INT32 imWidth;       

    /* Image pixel height (Y) */
    INT32 imHeight;      
    
    /* Image alignment        */
    UINT8 imAlign;       

    /* Image flags            */
    UINT8 imFlags;       
        
    INT16    pad;

    /* Image bytes per row    */
    INT32 imBytes;       
    
    /* Image bits per pixel   */
    INT16 imBits;        

    /* Image planes per pixel */
    INT16 imPlanes;      
} imageHdr;

/*Image Data Structure */
typedef struct _image
{
    /* Image pixel width (X)  */
    INT32 imWidth;       

    /* Image pixel height (Y) */
    INT32 imHeight;      
    
    /* Image alignment        */
    UINT8 imAlign;       

    /* Image flags            */
    UINT8 imFlags;       
    
    INT16 pad;       

    /* Image bytes per row    */
    INT32 imBytes;       
           
    /* Image bits per pixel   */
    INT16 imBits;        

    /* Image planes per pixel */
    INT16 imPlanes;      
    
    /* Image buffer           */
    UINT8 *imData;  
} image;


/* imFlags - Bit Field Definitions */
/* 1 = imData spans 64K segment(s), 0 = doesn't */
#define imSpansSeg  0x01  

/* hicolor format 0 = 5:5:5, 1 = 5:6:5 */
#define im565       0x02  

/* Pattern definition */
typedef struct _pattern
{
    /* 8, 16 (or 32)         */
    INT32 patWidth;      

    /* 8, 16 or 32           */
    INT32 patHeight;     

    /* must be zero          */
    UINT8 patAlign;      

    /* pattern flags         */
    UINT8 patFlags;      
    INT16 pad;

    /* pattern bytes per row */
    INT32 patBytes;      

    /* value of 1,2,4 or 8   */
    INT16 patBits;       

    /* value of 1 thru 32    */
    INT16 patPlanes;     

    /* pattern data bytes    */
    UINT8 patData[32];   
} pattern;

/* patFlags - Bit Field Definitions */
/* Bit sequence - LtoR(0)/RtoL(1) */
#define patBitSeq    0x80    

/* Pattern List structure */
typedef struct _patList
{
    /* pointers to pattern images */
    pattern *patPtr[32];  

    /* pattern X alignments       */
    UINT8 patAlignX[32];  

    /* pattern Y alignments       */
    UINT8 patAlignY[32];  
} patList;

/* Dash Info Data Record */
typedef struct _dashRcd
{
    /* # of elements in dashList    */
    INT16 dashSize;       

    /* (reserved for future use)    */
    INT16 dashRsvd;       

    /* ptr to list of on/off counts */
    const UINT8 *dashList;      
} dashRcd;

#define dashOnOff       0
#define dashDouble      1

/* Region Data Structure */
typedef struct _region
{
    /* Space allocated for this record          */
    SIGNED rgnAlloc;      

    /* Space used in this record                */
    SIGNED rgnSize;       
    
    /* Region Flags                             */
    INT16 rgnFlags;       

    /* (alignment/reserved for future use)      */
    INT16 rgnRsvd;        
    
    /* Bounding limits of the region            */
    rect rgnRect;         
    
    /* Ptr to first entry in list of Y|X-banded */
    rect *rgnList;        

    /* Ptr to last entry in list of Y|X-banded  */
    rect *rgnListEnd;     
} region;

/* Beginning-of-Scan delimiter (for scanning backwards) */
#define rgnBOS          -32768  

/* End-of-Scan delimiter */
#define rgnEOS          32767   

/* Region Flags */
/* region is rectangular(0)/non-rectangular(1) */
#define rfRect          0x0001  

/* region is non-empty(0)/empty(1) */
#define rfNull          0x8000  

/* cap status codes */
/* draw the entire line */
#define NoSkip          0   

/* don't draw the last point of the line */
#define SkipLast        1   

/* don't draw the first point of the line */
#define SkipFirst      -1   

/* (Poly-) Segment structure */
typedef struct _segPts
{
    /* start X   */
    INT32    sX1;       

    /* start Y   */
    INT32    sY1;       

    /* end X     */
    INT32    sX2;       

    /* end Y     */
    INT32    sY2;       

    /* cap style */
    INT16    sCap;      
    INT16    pad;       
} segPts;

#define convex            0
#define nonconvex         1
#define cmplx             2

#define coordModeOrigin   0
#define coordModePrevious 1

/* Event record structure */
typedef struct _rsEvent
{
    /* System time stamp            */
    INT16    eventTime;     

    /* Event type flags             */
    UINT8    eventType;     

    /* Event source device ID       */
    UINT8    eventSource;   

    /* Keyboard Character code      */ 
    INT16    eventChar;     

    /* Keyboard Scan code           */
    INT16    eventScan;     

    /* Keyboard State info          */
    INT16    eventState;    

    /* Positional device buttons    */
    INT16    eventButtons;  

    /* Positional device X          */
    INT32    eventX;        

    /* Positional device Y          */
    INT32    eventY;        

    /* User defined event data      */
    INT16    eventUser[2];  
}  rsEvent;

/* Input device record */
typedef struct _mouseRcd
{
    /* an event element                  */
    rsEvent mrEvent;          
    
    /* this devices event types mask     */
    INT16 mrEventMask;      

    /* device flags (opened signature)   */
    INT16 mrFlags;          
    
    /* limit rectangle                   */
    rect mrLimits;          
    
    /* x,y scaling factors               */
    point mrScale;          

    /* vector to input manager           */
    INT32 (*mrInputMgr)();  

    /* call back to system event handler */ 
    VOID (*mrCallBack)();   
    
    /* used by input managers            */
    UINT8 mrUsr[32];        
} mouseRcd;

/* bit field for evntType and event mask operations */
/* post any activity */     
#define mPASS           0xff    

/* post/is input device movement */         
#define mPOS            0x01    

/* post/is button pressed */
#define mPRESS          0x02    

/* post/is button release */
#define mREL            0x04    

/* post/is key down */
#define mKEYDN          0x08    

/* post/is key up */
#define mKEYUP          0x10    

/* Flags signature */
#define mrOpenedSig     0x5f5d

/* Input manager function codes */
#define IMOPEN          0
#define IMCLOSE         1
#define IMLIMIT         2
#define IMSCALE         3
#define IMPOSN          4
#define IMQUERY         5

/* Fixed point (integer.fraction) ***********/
typedef struct _NU_fixed
{
    /* whole number (int) portion */
    INT16 fr;           

    /* fractional portion         */
    INT16 wh;           
} NU_fixed;


/* Fixed Point Data Structure */
typedef struct _fxPoint
{
    /* fixed X coordinate */
    INT32 fxX;          

    /* fixed Y coordinate */
    INT32 fxY;          
} fxPoint;

/* Fixed Line Data Structure */
typedef struct _fxLine
{
    /* point 1 */
    INT16 fxP1;         

    /* point 2 */
    INT16 fxP2;         
} fxLine;


/* EGA and VGA port Equates */
#define crtAddr         0xD4
#define miscAddr        0xC2
#define pelWrite        0xC8
#define graf1Addr       0xCC
#define graf2Addr       0xCA
#define grafAddr        0xCE
#define attrREAD        0xDA
#define attrWRITE       0xC0
#define seqAddr         0xC4
#define inStatus1       0xDA

#define egaSqncr        0x3C4  
#define egaSqncrP1      0x3C5  
#define egaSqncrLo      0xC4
#define sMapMask        0x02   

#define egaCntlr        0x3CE  
#define egaCntlrP1      0x3CF  
#define egaCntlrLo      0xCE
#define gSetReset       0x00   
#define gEnableSR       0x01   
#define gColorCompare   0x02
#define gDataRot        0x03   
#define egaREP          0x0000 
#define egaAND          0x0800 
#define egaOR           0x1000 
#define egaXOR          0x1800 
#define gReadMap        0x04   
#define gMode           0x05   
#define egaWrite0       0x0000 
#define egaWrite1       0x0100 
#define egaWrite2       0x0200 
#define egaWrite3       0x0300 
#define egaRead0        0x0000 
#define egaRead1        0x0800 
#define gColorDontCare  0x07   
#define gBitMask        0x08   

/* OS dependent defines - DOS */
#define  VIDINT         0x10
#define  DOSINT         0x21

/* ----- RASTEROP, PENMODE & TEXTMODE Pixel Operations ----- */

/*       src                      */
#define zREPz          0   

/*       src     OR      dst      */
#define zORz           1   

/*       src     XOR     dst      */
#define zXORz          2   

/*    (NOT src)  AND     dst      */
#define zNANDz         3   

/*    (NOT src)                   */
#define zNREPz         4   

/*    (NOT src)  OR      dst      */
#define zNORz          5   

/*    (NOT src)  XOR     dst      */
#define zNXORz         6   

/*       src     AND     dst      */
#define zANDz          7   

/*               0s               */
#define zCLEARz        8   

/*       src     OR   (NOT dst)   */
#define zORNz          9   

/*                       dst      */
#define zNOPz         10   

/*       src     AND  (NOT dst)   */
#define zANDNz        11   

/*               1s               */
#define zSETz         12   

/*    (NOT src)  OR   (NOT dst)   */
#define zNORNz        13   

/*                    (NOT dst)   */
#define zINVERTz      14   

/*    (NOT src)  AND  (NOT dst)   */
#define zNANDNz       15   

/* Source monochrome "Transparent" rasterOp Operations */
/*    (!0src)                     */
#define xREPx         16   

/*    (!0src)    OR      dst      */
#define xORx          17   

/*    (!0src)    XOR     dst      */
#define xXORx         18   

/*  (NOT !0src)  AND     dst      */
#define xNANDx        19   

/*  (NOT !0src)                   */
#define xNREPx        20   

/*  (NOT !0src)  OR      dst      */
#define xNORx         21   

/*  (NOT !0src)  XOR     dst      */
#define xNXORx        22   

/*    (!0src)    AND     dst      */
#define xANDx         23   

/*                       dst      */
#define xCLEARx       24   

/*    (!0src)    OR   (NOT dst)   */
#define xORNx         25   

/*                       dst      */
#define xNOPx         26   

/*    (!0src)    AND  (NOT dst)   */
#define xANDNx        27   

/*               1s               */
#define xSETx         28   

/*  (NOT !0src)  OR   (NOT dst)   */
#define xNORNx        29   

/*                    (NOT dst)   */
#define xINVERTx      30   

/*  (NOT !0src)  AND  (NOT dst)   */
#define xNANDNx       31   

/*  (src + dest)/alpha   */
#define xAVGx		  32        

/* ---------------- System structures & defines ------------ */
#define True            1
#define False           0

#define TextPg          0
#define TextPg0         0
#define GrafPg        256
#define GrafPg0       256
#define GrafPg1       257
#define GrafPgAll     255

#define enterRetrace    1
#define enterVideo      2
#define inRetrace       3
#define inVideo         4

#define upperLeft       1
#define lowerLeft       0


/* "FillPolygon" type definitions */
#define shapeConvex     0   
#define shapeNonconvex  1
#define shapeComplex    2

/* "FillPolygon" coordinate type defs */
#define coordRel        1   
#define coordAbs        0

/* polygon fill rule defs */
#define ruleWinding     1   
#define ruleOddEven     0

/* ---------------------- Pen Defines ---------------------- */
/* PenShape() definitions       */
#define shapeRect       0    
#define shapeOval       1    

/* PenSize() thin lines         */
#define penThin       0,0    

/* On-off style dashing         */
#define dashOnOff       0    

/* Double dashing               */
#define dashDouble      1    

/* List of Dash Records         */
typedef struct _dashes       
{
    /* List of 8 dash records       */
    dashRcd   dash[8];       
} dashes;

/* --------------- Images, Patterns, Cursor ---------------- */

/* Image Header record structure  */
typedef struct _imageHeader
{
    /* Pixel width (X)   */
    INT32        imWidth;       

    /* Pixel height (Y)  */
    INT32        imHeight;      

    /* Image alignment   */
    signed char  imAlign;       

    /* Image flags       */
    signed char  imFlags;       
    INT16        pad;

    /* Bytes per row     */
    INT32        imRowBytes;    

    /* Bits per pixel    */
    INT16        imBits;        

    /* Planes per pixel  */
    INT16        imPlanes;      
   } imageHeader;

/* Pattern Image Structure */
typedef struct _patRcd
{
    INT16        patWidth;      /* must be 1..31                 */
    INT16        patHeight;     /* must be 1..31                 */
    signed char  patAlign;      /* must be zero                  */
    signed char  patFlags;      /* must be zero                  */
    INT16        patRowBytes;   /* must be widthxbits rounded up */
    INT16        patBits;       /* value of 1 or match dest      */
    INT16        patPlanes;     /* value of 1 or match dest      */
   } patRcd;


/* Cursor Image definition */
typedef struct _cursor
{
    /* must be 16 or 32 */
    INT16        curWidth;      

    /* must be 16 or 32 */
    INT16        curHeight;     

    /* must be 0        */
    signed char  curAlign;      

    /* must be 0        */
    signed char  curFlags;      

    /* must be 2 or 4   */
    INT16        curRowBytes;   

    /* must be 1        */
    INT16        curBits;       

    /* must be 1        */
    INT16        curPlanes;     
} cursor;

/* Cursor Record definition */
typedef struct _cursorRcd
{
    /* cursor align point X      */
    INT16    hotX;              

    /* cursor align point Y      */
    INT16    hotY;              

    /* cursor background image   */
    cursor   scrnMask;          

    /* cursor foreground image   */
    cursor   cursMask;          
} cursorRcd;

/* "Segment" cap styles           */
#define capBoth        0   
#define capNotFirst   -1
#define capNotLast     1

/* Region merge defines */
#define REG_UNION      0
#define REG_INTERSECT  1
#define REG_SUBTRACT   2
#define REG_XOR        3

/* --------------- Input device and Event system ----------- */

/* Event Type and Mask flags */

/* no events pending               */
#define evntNONE    0x00     

/* input movement                  */
#define evntMOVE    0x01     

/* button press                    */
#define evntPRESS   0x02     

/* button release                  */
#define evntRELEASE 0x04     

/* key press                       */
#define evntKEYDN   0x08     

/* key release                     */
#define evntKEYUP   0x10     

/* user program event 1            */
#define evntUSER1   0x20     

/* user program event 2            */
#define evntUSER2   0x40     

/* EVENTH_MaskEvent() - enable all */
#define evntALL     0xFF     
                          
/* "eventButtons" mouse buttons */
#define swLeft         1 
#define swRight        2 

/* (not on 2 button mice)       */
#define swMiddle       4     
#define swAll          7


/* ------------------- HardCopy record -------------------- */
typedef struct _HCInfoRec
{
    /* Number of elements in print swath      */
    INT16       hcPins;         

    /* Number of bytes in "hcInit" string     */
    INT16       hcInlen;        

    /* Sent to init printer for output        */
    UINT8       hcInit[32];     
    
    /* Number of bytes in "hcReset" string    */
    INT16       hcRslen;        
    INT16       pad;            

    /* Sent to reset after output complete    */
    UINT8       hcReset[16];    
    
    /* Number of bytes in "hcHeader" string   */
    INT16       hcHdlen;        

    INT16       pad1;           

    /* Sent before every line                 */
    UINT8       hcHeader[16];   

    /* Number of bytes in "hcPstam" string    */
    INT16       hcPstlen;       
    INT16       pad2;           

    /* Sent after length specification        */
    UINT8       hcPstam[16];    

    /* Number of bytes in "hcTrailer" string  */
    INT16       hcTrlen;        
    INT16       pad3;           

    /* Sent after every line                  */
    UINT8       hcTrailer[16];  
    
    /* Operational mode bits                  */
    INT16       hcMode;         

    /* Set to 'HC' for validity check         */
    UINT8       hcSIG[2];       
    
    /* Number of entries in "hcXlatTable"     */
    INT16       hcXlatCount;    
    INT16       pad4;           

    /* Pointer to color translation table     */
    INT32       *hcXlatTable;   
} HCInfoRec;                    

/* Rotate picture 90-degrees CCW          */
#define HC_ROT 0x01             

/* XlatTable is list to print(1)/mask(0)  */  
#define HC_FG  0x02             

/* Invert output picture image colors     */
#define HC_INV 0x04             

/* Emit ASCII length instead of hex       */
#define HC_ASC 0x10             


/* --------- Extended Pen Definitions for Oval Pen --------- */
/* "PenCap()" definitions                 */
#define capFlat        0        
#define capSquare      2
#define capRound       3

/* "PenJoin()" definitions                */
#define joinRound      0        
#define joinBevel      1
#define joinMiter      2


/* Fixed point math type */
typedef struct _fxdpnt
{
    /* fractional part */
    INT16    fr; 
       
    /* whole part      */
    INT16    wh; 
} fxdpnt;

#endif /* _RSCONST_H_ */




