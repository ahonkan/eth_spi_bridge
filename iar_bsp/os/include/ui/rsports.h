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
*  rsports.h                                                    
*
* DESCRIPTION
*
*  This file defines the rsPort & grafMap structures.
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
#ifndef _RSPORTS_H_
#define _RSPORTS_H_

/* "grafMap" Data Structure */
typedef struct _grafMap
{
    /* Device class */
    INT16        devClass;        

    /* Device technology */
    INT16        devTech;         

    /* Device ID */
    SIGNED       devMode;         

    /* Padding for the structure */
    INT16        pad;             

    /* Bytes per scan line */
    INT16        pixBytes;        
    
    /* Pixels horizontal */
    INT32        pixWidth;        

    /* Pixels vertical */
    INT32        pixHeight;       

    /* Pixels per inch horizontally */
    INT16        pixResX;         

    /* Pixels per inch vertically */  
    INT16        pixResY;         

    /* Color bits per pixel */
    INT16        pixBits;         

    /* Color planes per pixel */
    INT16        pixPlanes;       

    /* grafMap flags */
    INT16        mapFlags;        

    /* Padding for the structure */
    INT16        pad1;            

    /* Pointers to rowTable(s) */
    UINT8        **mapTable[32];   

    /* busy semaphore */ 
    INT16        mapLock;         

    /* OS next segment cookie */   
    INT16        mapNextSel;      

    /* Bank window type (0-3) */
    SIGNED       mapWinType;      

    /* Offset to 2nd bank window */
    SIGNED       mapWinOffset;    

    /* Current bank(s) min Y value */
    SIGNED       mapWinYmin[2];   

    /* Current bank(s) max Y value */
    SIGNED       mapWinYmax[2];   

    /* Current bank plane */
    SIGNED       mapWinPlane;     

    /* Scan lines per bank */
    SIGNED       mapWinScans;     

    /* Handle to access device */
    SIGNED       mapHandle;       

    /* Ptr to bank manager function */  
    INT32        (*mapBankMgr)(); 

    /* Ptr to plane manager function */
    VOID         (*mapPlaneMgr)();

    /* Ptr to alt manager function */
    VOID         (*mapAltMgr)();  

    /* Ptr to device manager list */       
    INT32        (*mapDevMgr)();  

    /* primitive vector for fills */
    VOID         (*prFill)();     

    /* primitive vector for self-self blits */
    VOID         (*prBlitSS)();   

    /* primitive vector for mono-self blits */
    VOID         (*prBlit1S)();   

    /* primitive vector for mem-self  blits */
    VOID         (*prBlitMS)();   

    /* primitive vector for self-mem  blits */
    VOID         (*prBlitSM)();   

    /* primitive vector for read image */
    VOID         (*prRdImg)();    

    /* primitive vector for write image */
    VOID         (*prWrImg)();    

    /* primitive vector for thin lines */
    VOID         (*prLine)();     

    /* primitive vector for set pixel */
    VOID         (*prSetPx)();    

    /* primitive vector for get pixel */
    SIGNED       (*prGetPx)();    

    /* call back to resync function */ 
    VOID         (*cbSyncFunc)(struct _grafMap *);

    /* call back to post an error */
    VOID         (*cbPostErr)(INT16 ); 

} grafMap;


/* "devTech" definitions */

/* User Supplied Primitives */
#define dtUser    0x0000    

/* Monochrome */
#define dtMono    0x0001    

/* MultiPlane */
#define dtPlan    0x0003    

/* EGA */
#define dtEGA     0x0004    

/* VGA */
#define dtVGA     0x0005    

/* IBM 8514 */
#define dt8514    0x0007    

/* 8-bit-per-pixel, 256-color */
#define dt8Bit    0x0008    

/* 16-bits per pixel, 64K-color */
#define dt16Bit   0x0009    

/* "mapFlags" definitions */

/* operation pending on locked grafMap */
#define mfPending 0x0008    

/* Row table(s) - interleaved(0)/linear(1) */
#define mfRowTabl 0x0010    

/* Display status - text(0)/graphics(1) */
#define mfDspGraf 0x0020    

/* Supports 8 bit per RGB DAC */
#define mfDac8    0x0100    

/* hicolor format 1 = 5:6:5, 0 = 5:5:5 */
#define mf565     0x0200    


/* "rsPort" Data Structure */
typedef struct _rsPort
{
    /* Pointer to "grafMap" record */
    grafMap      *portMap;     

    /* Local coordinate port bounds */
    rect         portRect;     

    /* Global origin of portRect */
    point        portOrgn;     

    /* 'Virtual' port bounds */
    rect         portVirt;     

    /* Local clipping rectangle */
    rect         portClip;     

#ifndef NO_REGION_CLIP
    
    /* Pointer to clipping region */
    region       *portRegion;  

#endif  /* NO_REGION_CLIP */
    
#ifdef  FILL_PATTERNS_SUPPORT
    
    /* Pointer to pattern list */
    patList      *portPat;     

#endif /* FILL_PATTERNS_SUPPORT */
    
    /* Port plane mask */
    SIGNED       portMask;     

    VOID         (*portU2GP)(INT32 *virtX, INT32 *virtY);
    VOID         (*portU2GR)(rect *virRect);

    VOID         (*portG2UP)(INT32 *virtX, INT32 *virtY);
    VOID         (*portG2UR)(rect *virRect);

    /* Port flags */
    INT16       portFlags;     
    
    /* Background pattern index */
    INT16       bkPat;         

    /* Background color */
    SIGNED      bkColor;       

    /* Pen color */  
    SIGNED      pnColor;       

    /* Pen location */    
    point       pnLoc;         

    /* Pen size */  
    point       pnSize;        

    /* Pen visibility level */
    INT16       pnLevel;       
    
    /* Pen mode (rasterOp) */
    INT16       pnMode;        

    /* Pen pattern index */
    INT16       pnPat;         
    
    /* Pen end-cap style */
    INT16       pnCap;         

    /* Line join style */
    INT16       pnJoin;        
    
    /* Line join miter limit */
    INT16       pnMiter;       

    /* Current pen flags */
    INT16       pnFlags;       

#ifdef  DASHED_LINE_SUPPORT
    
    /* Current dash number */
    INT16       pnDash;        

    /* Current dash sequence index */
    INT16       pnDashNdx;     

    /* Current dash sequence count */
    INT16       pnDashCnt;     

    /* Pointer to dash records */
    dashRcd     *pnDashRcd;    

#else

    INT16       pad;
    
#endif  /* DASHED_LINE_SUPPORT */
    
    /* Pointer to current text font */
    fontRcd      *txFont;      

    /* Text facing flags */
    INT16       txFace;        

    /* Text mode (rasterOp) */
    INT16       txMode;        

    /* Text underline position */
    INT16       txUnder;       

    /* Text underline scoring */
    INT16       txScore;       

    /* Text path */
    INT16       txPath;        

    /* Text pattern */
    INT16       txPat;         

    /* Text alignment */
    point       txAlign;       

    /* Text color */
    SIGNED      txColor;       

#ifdef      USE_STROKEDFONT
    
    /* Text size  (stroked) */
    point       txSize;        

    /* Text angle (stroked) */
    INT16       txAngle;       

#endif      /* USE_STROKEDFONT */
    
    /* Text slant (stroked) */
    INT16       txSlant;       

    /* Text justify bits */
    INT16       txExtra;       

    /* Space justify bits */
    INT16       txSpace;       

    /* Text "boldness" scale */
    INT16       txBold;        
    
    /* Text termination character */
    INT16       txTerm;        

    /* Pointer to marker font */  
    fontRcd      *mkFont;      

    /* Marker type index */
    INT16       mkType;        

    /* Marker angle */
    INT16       mkAngle;       

    /* Marker size */
    point       mkSize;        

} rsPort;

#define grafPort rsPort

/* PUSHGRAFIX/POPGRAFIX save area */
typedef struct _pusharea   
{
    INT16 saveArea[sizeof(grafPort)/sizeof(INT16)];
} pusharea;

/* The defines below make assumptions about the field ordering in rsPort */
/* If there are changes to rsPort, these may be invalidated! */
#define PENSTART offset bkPat
#define PENEND   offset txFont
#define PENAREA  (PENEND - PENSTART)

/* MultiBlit data record */
typedef struct _blitRcd
{    
    /* (reserved for MW use) */
    SIGNED       blitRsv;      

    /* Space allocated for this rcd */
    SIGNED       blitAlloc;    

    /* Blit operation flags */
    INT16        blitFlags;    

    /* RasterOp transfer mode */
    INT16        blitRop;      

    /* Fill pen pattern number */
    INT16        blitPat;
    
    /* # of elements in blitList */
    INT16        blitCnt;      

    /* Ptr to 32-bit plane mask */
    SIGNED       blitMask;     

#ifdef  FILL_PATTERNS_SUPPORT
    
    /* Ptr to pattern pointers list */
    patList      *blitPatl;    

#endif /* FILL_PATTERNS_SUPPORT */
    
    /* Ptr to source bitmap record */
    grafMap      *blitSmap;    

    /* Ptr to destination bitmap record */
    grafMap      *blitDmap;    

#ifndef NO_REGION_CLIP
    
    /* Ptr to clipping region */
    region       *blitRegn;    

#endif  /* NO_REGION_CLIP */

    /* Ptr to clip rect/region */
    rect         *blitClip;    

    /* Background color */
    SIGNED       blitBack;     

    /* Foreground color */
    SIGNED       blitFore;     

    /* Ptr to drawing list */
    SIGNED       blitList;     

} blitRcd;


/* Write Image Blit data record */
typedef struct _WIblitRcd
{
    /* (reserved for MW use) */
    SIGNED       blitRsv;      

    /* Space allocated for this rcd */
    SIGNED       blitAlloc;    

    /* Blit operation flags */
    INT16        blitFlags;    

    /* RasterOp transfer mode */
    INT16        blitRop;      

    /* Fill pen pattern number */
    INT16        blitPat;      

    /* # of elements in blitList */
    INT16        blitCnt;      

    /* Ptr to 32-bit plane mask */
    SIGNED       blitMask;     

#ifdef  FILL_PATTERNS_SUPPORT
    
    /* Ptr to pattern pointers list */
    patList      *blitPatl;    

#endif /* FILL_PATTERNS_SUPPORT */
    
    /* Ptr to image */
    UNSIGNED     blitSmap;     

    /* Ptr to destination bitmap record */
    grafMap      *blitDmap;    

#ifndef NO_REGION_CLIP
    
    /* Ptr to clipping region */
    region       *blitRegn;    

#endif  /* NO_REGION_CLIP */
    
    /* Ptr to clip rect/region */
    rect         *blitClip;    

    /* Background color */
    SIGNED       blitBack;     
    
    /* Foreground color */
    SIGNED       blitFore;     

    /* Ptr to drawing list */
    SIGNED       blitList;     

} WIblitRcd;


/* blitFlags */

/* Rectangle clipping */
#define bfClipRect      0x0001  

/* Region clipping */
#define bfClipRegn      0x0002  

/* blitList data is YX-banded */
#define bfYXBanded      0x0008  

/* blitList for vector items */
typedef struct _blitVect
{
    rect VectData;

    UINT16 skipStat;

    INT16 pad;                  

} blitVect;

/*  "portFlags" Bit Field Definitions */

/* port origin - lowerleft(0)/upperleft(1) */
#define pfUpper     0x0001  

/* port coordinates - local(0)/virtual(1) */
#define pfVirtual   0x0002  

/* port coordinates - port(0)/global(1) */
#define pfGblCoord  0x0080  

/* rectangle clipping disabled (0)/enabled(1) */
#define pfRecClip   0x0100  

/* region clipping - disabled(0)/enabled(1) */
#define pfRgnClip   0x0200  

/* polygon fill rule - odd-even(0)/winding(1) */
#define pfFillRule  0x1000  

/* "penFlags" Bit Field Definitions */

/* pen pattern - solid(0)/patterned(1) */
#define pnPattFlg   0x0001  

/* pen dash style - solid(0)/dashed(1) */
#define pnDashFlg   0x0002  

/* pen shape - elliptical(0)/rectangular(1) */ 
#define pnShapeFlg  0x0004  

/* pen size - singleWidth(0)/multiWidth(1) */
#define pnSizeFlg   0x0008  

/* dash style - onOff(0)/doubleDash(1) */
#define pnDashStyle 0x0010  

/* dash sequence state - off(0)/on(1) */
#define pnDashState 0x0020  

/* Get/SetPenstate are subsets of grafPort data */

#define penState grafPort 

/* PenShape codes */
#define shapeRect   0
#define shapeOval   1

/* PenCap - port.pnCap codes */
#define capFlat     0
#define capSquare   2
#define capRound    3

/* PenJoin - port.pnJoin codes */
#define joinRound   0
#define joinBevel   1
#define joinMiter   2

/* mapFlags - grafMap Flag Definitions */

/* requires plane management */
#define mfPlaneMgr 0x0001   

/* requires bank management */
#define mfBankMgr  0x0002   

/* mapAltMgr support - no(0)/yes(1) */
#define mfAltMgr   0x0004   

/* operation pending on locked grafMap */
#define mfPending  0x0008   

/* Row table(s) - interleaved(0)/linear(1) */
#define mfRowTabl  0x0010   

/* Display status - text(0)/graphics(1) */
#define mfDspGraf  0x0020   

/* reset bank manager */
#define mfResetBM  0x0040   

/* VGA hardware device - yes(1)/no(0) */
#define mfVgaHw    0x0080   

/* Supports 8 bit per RGB DAC */
#define mfDac8     0x0100   

/* hicolor format 1 = 5:6:5, 0 = 5:5:5 */
#define mf565      0x0200   

/* mapWinType - Map Window Type Definitions */

/* Non bank-switched device */
#define mwtNonBanked    0   

/* Bank switch, 2 window, Read/Write */
#define mwtTwoWinRW     1   

/* Bank switch, 2 window, ReadOnly/WriteOnly */
#define mwtTwoWinROWO   2   

/* Bank switch, 1 window, Read/Write */
#define mwtOneWinRW     3   

/* devTech - Device Technology Flag Definitions */

/* User Supplied Primitives */
#define dtUser     0x0000   

/* Monochrome */
#define dtMono     0x0001   

/* Multi-bit */
#define dtBits     0x0002   

/* MultiPlane */
#define dtPlan     0x0003   

/* EGA */
#define dtEGA      0x0004   

/* VGA */
#define dtVGA      0x0005   

/* VGA */
#define dtVGA_S    0x0006   

/* IBM 8514 */
#define dt8514     0x0007   

/* 8-bit-per-pixel, 256-color */
#define dt8Bit     0x0008   

/* 16-bits per pixel, 64K-color */
#define dt16Bit    0x0009   

/* 24-bits per pixel, 16.7M-color */
#define dt24Bit    0x000A   

/* 32-bits per pixel, 16.7M-color. 8-bit Alpha channel */
#define dt32Bit    0x000B

/* _RSPORTS_H_ */
#endif 

