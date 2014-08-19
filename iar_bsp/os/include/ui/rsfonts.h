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
*  rsfonts.h                                                    
*
* DESCRIPTION
*
*  This file defines the fontHeader structure and text macros.
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
#ifndef _RSFONTS_H_
#define _RSFONTS_H_

#ifdef USE_UNICODE

typedef UINT16 UNICHAR;
#define CHAR_SIZE               2

#else

typedef UINT8 UNICHAR;
#define CHAR_SIZE               1

#endif

/* Font Header Record Structure */
typedef struct _fontRcd
{
    /* Font format version number */
    UINT8  fontVer;          

    /* Font style revision designator */
    UINT8  fontRev;          

    /* Font name length (number of chars) */
    UINT8  nameLen;          

    /* FILE flag if a font is loaded from the file system */
    signed char font_using_FILE;         

    /* Font Base Name (ASCII) */
    UINT8  fontBaseName[16]; 

    /* Font suffix 1 */
    UINT8  fontSuffix1[10];  

    /* Padding for the structure */
    INT16  pad1;             

    /* Font suffix 2 */
    UINT8  fontSuffix2[10];  

    /* Padding for the structure */
    INT16 pad2;              

    /* Font suffix 3 */
    UINT8  fontSuffix3[10];  

    /* Padding for the structure */
    INT16  pad3;             

    /* Padding for the structure */
    signed char pad4[3];     

    /* Synthesized font facing flags */
    UINT8  fontFacing;       

    /* NU__FONT signature */
    UINT8  fontSign[8];      
    
    /* Font weight */
    UINT8  fontWeight;       

    /* Character set encoding */
    UINT8  fontCoding;       

    /* Padding for the structure */
    INT16  pad5;             

    /* Buffer size needed to hold font */    
    SIGNED fontSize;         

    /* Maximum character code */
    UINT16 fontMax;          

    /* Minimum character code */
    UINT16 fontMin;          
    
    /* Point Size */
    INT16  fontPtSize;       
    
    /* Font family code */
    INT16  fontfamily;       
    
    /* Device style */
    INT16  fontStyle;        
    
    /* Font flags */
    INT16  fontFlags;        
    
    /* Font character & background color */
    SIGNED fontColor[2];     
    
    /* Minimum ASCII character code */
    UINT8  minChar;          

    /* Maximum ASCII character code */
    UINT8  maxChar;          

    /* Fixed space character width */
    INT16  chWidth;          

    /* Character height (ascent+decent) */
    INT16  chHeight;         

    /* Fixed space character offset (+/-) */
    INT16  chKern;           

    /* Ascent */
    INT16  ascent;           

    /* Descent */
    INT16  descent;          

    /* Vertical spacing between baselines */
    INT16  lnSpacing;        

    /* Char to show for undefined codes */
    INT16  chBad;            
    
    /* Icon or Marker center (X,Y) */
    point16  chCenter;       
    
    /* Italicize slant angle */
    INT16  chAngle;          

    /* recommended txUnder setting */
    INT16  chUnder;          

    /* recommended txScore setting */
    INT16  chScore;          

    /* number of character glyphs */
    UINT16  numGlyphs;        
    
    /* offset to character location table */
    SIGNED locTbl;           
    
    /* offset to character width/offset table */
    SIGNED ofwdTbl;          
    
    /* offset to kerning pairs table */
    SIGNED kernTbl;          
    
    /* offset to size/rotate table (stroked) */
    SIGNED sizeTbl;          
    
    /* offset to font grafMap structure */
    SIGNED grafMapTbl;       
    
    /* offset to rowTable data area */
    SIGNED rowTbl;           
    
    /* offset to font image/nodes table */
    SIGNED fontTbl;          
    
    /* offset to trademark/copyright notice */
    SIGNED fontNotice;       

    /* offset to name of font supplier */
    SIGNED fontSupplier;     

    /* offset to name of font author */
    SIGNED fontAuthor;       

    /* offset to miscellaneous font info */    
    SIGNED fontInfo;         

    /* offset to font creation date */
    SIGNED fontDate;         
    
    /* Vertical spacing between baselines */   
    fxdpnt fontSpacing;      

    /* baseline to top of lowercase 'x' */
    fxdpnt fontLowHgt;       

    /* baseline to top of capital 'H' */
    fxdpnt fontCapHgt;       

    /* baseline to top of lowercase 'd' */  
    fxdpnt fontAscent;       

    /* baseline to bottom of lowercase 'p' */
    fxdpnt fontDescent;      

    /* maximum character width */
    fxdpnt fontMaxWid;       

    /* average character width */
    fxdpnt fontAvgWid;       

    /* Em space width */
    fxdpnt fontEmWid;        

#ifdef USE_UNICODE

    /* number of bitmap character rows */
    UINT16 numCRows;         

    /* number of character mapping records */
    UINT16 numCMaps;         

    /* offset to character mapping table */
    INT32  cmapTbl;          

    /* offset to facing off/wid tables */
    SIGNED offwidTbl[14];    
#else
    /* offset to facing off/wid tables */
    SIGNED offwidTbl[16];    
#endif

} fontRcd;


/* Offset/Width Table Entry */
typedef struct _ofswid
{
    /* character width (unsigned) */
    UINT8        wid;        

    /* character offset (signed) */
    signed char  ofs;        
} ofswid;

/* fontFlags */

/* Fixed(1)/Proportional(0) spaced font */
#define fixSpace        0x01    

/* Stroked(1) or bitmap(0) font */
#define fontType        0x02    

/* Icon/marker(1) or Font(0) file */
#define iconFile        0x04    

extern INT16 useUNICODE;

/* "TextFace" constants */
#define faceBold         0x0001
#define faceItalic       0x0002
#define faceUnderline    0x0004
#define faceStrikeout    0x0008
#define faceHalftone     0x0010

/* mirror are stroked faces only */
#define faceMirrorX      0x0020  
#define faceMirrorY      0x0040

#define faceProportional 0x0080
#define faceAntialias    0x0100

/* "TextFace" definitions */
#define cNormal         0
#define cBold           1
#define cItalic         2
#define cUnderline      4
#define cStrikeout      8
#define cHalftone      16
#define cMirrorX       32
#define cMirrorY       64
#define cProportional 128

/* "TextAlign" constants */
#define alignLeft       0
#define alignCenter     1
#define alignRight      2

#define alignBaseline   0
#define alignBottom     1
#define alignMiddle     2
#define alignTop        3

/* "charPath" constants */
#define pathRight       0
#define pathUp          900
#define pathLeft        1800
#define pathDown        2700

/* "StrokeFlags" status bits */

/* txPath  zero(0)/non-zero(1) */
#define thePath         0x01    

/* txAngle zero(0)/non-zero(1) */
#define angle           0x02    

/* txSlant zero(0)/non-zero(1) */
#define slant           0x04    

/* sinSlant +(0) or -(1) */
#define signSlantSin    0x10    

/* cosSlant +(0) or -(1) */
#define signSlantCos    0x20    

/* the chBad font field is fixed space for stroked */
#define fxSpac          chBad   

/* macros for text width routines (str1*.asm) - - - - - - - - - - - - - - - - */

/* Return width in local(1) or global(0) coordinates */
#define rtnLcl          1       

/* Save the current pen level and position and hide the pen.*/
#define TXW_PSH \
    savLocX = LocX;\
    savLocY = LocY;\
    savPenX = theGrafPort.pnLoc.X;\
    savPenY = theGrafPort.pnLoc.Y;\
    theGrafPort.pnLevel--;

/* Restore the current pen level and position and show the pen. (Local) */
#define TXW_POPLcl \
    theGrafPort.pnLevel++;\
    LocY = savLocY;\
    LocX = savLocX;\
    theGrafPort.pnLoc.Y = savPenY;\
    thePort->pnLoc.Y = savPenY;\
    theGrafPort.pnLoc.X = savPenX;\
    thePort->pnLoc.X = savPenX;

/* Restore the current pen level and position and show the pen. (Global) */
#define TXW_POPGbl \
    theGrafPort.pnLevel++;\
    LocY = savLocY;\
    LocX = savLocX;\
    theGrafPort.pnLoc.Y = savPenY;\
    thePort->pnLoc.Y = savPenY;\
    theGrafPort.pnLoc.X = savPenX;\
    thePort->pnLoc.X = savPenX;

#if  rtnLcl /* - - - - - - - - */
#define TXW_POP TXW_POPLcl
/* rtnGbl */
#else   
#define TXW_POP TXW_POPGbl
#endif

/* error structure define */
typedef struct GFX_ERROR_STRUCT
{
    struct  GFX_ERROR_STRUCT *prev;    /* pointer to previous grafix error   */
    struct  GFX_ERROR_STRUCT *next;    /* pointer to next grafix error       */
    INT16   lineNum;                   /* Line number of the error           */
    UNICHAR fileName[255];             /* File name of the error             */
    INT16   ErrValue;                  /* The error passed in                */

} GFX_ERROR;

/* _RSFONTS_H_ */
#endif 
 
