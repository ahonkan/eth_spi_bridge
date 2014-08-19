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
*  mwfonts.h                                                    
*
* DESCRIPTION
*
*  This file contains the MWgrafMap and MWgrafMap structures.
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

/* Font Header Record Structure */
typedef struct _MWfontRcd
{
    UINT8  fontVer;          /* Font format version number             */
    UINT8  fontRev;          /* Font style revision designator         */
    UINT8  nameLen;          /* Font name length (number of chars)     */

    UINT8  fontBaseName[16]; /* Font Base Name (ASCII)                */
    UINT8  fontSuffix1[10];  /* Font suffix 1                          */
    UINT8  fontSuffix2[10];  /* Font suffix 2                          */
    UINT8  fontSuffix3[10];  /* Font suffix 3                          */
    UINT8  fontFacing;       /* Synthesized font facing flags          */
    UINT8  fontSign[8];      /* NU__FONT signature                     */
    UINT8  fontWeight;       /* Font weight                            */
    UINT8  fontCoding;       /* Character set encoding                 */
    SIGNED fontSize;         /* Buffer size needed to hold font        */
    UINT16  fontMax;         /* Maximum character code                 */
    UINT16  fontMin;         /* Minimum character code                 */
    INT16  fontPtSize;       /* Point Size                             */
    INT16  fontfamily;       /* Font family code                       */
    INT16  fontStyle;        /* Device style                           */
    INT16  fontFlags;        /* Font flags                             */
    SIGNED fontColor[2];     /* Font character & background color      */

    UINT8  minChar;          /* Minimum ASCII character code           */
    UINT8  maxChar;          /* Maximum ASCII character code           */
    INT16  chWidth;          /* Fixed space character width            */
    INT16  chHeight;         /* Character height (ascent+decent)       */
    INT16  chKern;           /* Fixed space character offset (+/-)     */
    INT16  ascent;           /* Ascent                                 */
    INT16  descent;          /* Descent                                */
    INT16  lnSpacing;        /* Vertical spacing between baselines     */
    INT16  chBad;            /* Char to show for undefined codes       */
    point16  chCenter;       /* Icon or Marker center (X,Y)            */
    INT16  chAngle;          /* Italicize slant angle                  */
    INT16  chUnder;          /* recommended txUnder setting            */
    INT16  chScore;          /* recommended txScore setting            */
    INT16  numGlyphs;        /* number of character glyphs             */
    SIGNED locTbl;           /* offset to character location table     */
    SIGNED ofwdTbl;          /* offset to character width/offset table */
    SIGNED kernTbl;          /* offset to kerning pairs table          */
    SIGNED sizeTbl;          /* offset to size/rotate table (stroked)  */
    SIGNED grafMapTbl;       /* offset to font grafMap structure       */
    SIGNED rowTbl;           /* offset to rowTable data area           */    
    SIGNED fontTbl;          /* offset to font image/nodes table       */
    SIGNED fontNotice;       /* offset to trademark/copyright notice     */
    SIGNED fontSupplier;     /* offset to name of font supplier        */
    SIGNED fontAuthor;       /* offset to name of font author          */
    SIGNED fontInfo;         /* offset to miscellaneous font info      */
    SIGNED fontDate;         /* offset to font creation date           */
    fxdpnt fontSpacing;      /* Vertical spacing between baselines     */
    fxdpnt fontLowHgt;       /* baseline to top of lowercase 'x'       */
    fxdpnt fontCapHgt;       /* baseline to top of capital 'H'         */
    fxdpnt fontAscent;       /* baseline to top of lowercase 'd'       */
    fxdpnt fontDescent;      /* baseline to bottom of lowercase 'p'    */
    fxdpnt fontMaxWid;       /* maximum character width                */
    fxdpnt fontAvgWid;       /* average character width                */
    fxdpnt fontEmWid;        /* Em space width                         */

    UINT16      numCRows;    /* number of bitmap character rows        */
    UINT16      numCMaps;    /* number of character mapping records    */
    INT32       cmapTbl;     /* offset to character mapping table      */
    SIGNED offwidTbl[14];    /* offset to facing off/wid tables        */

} MWfontRcd;


/* "grafMap" Data Structure */
typedef struct _MWgrafMap
{
  INT16        devClass;        /* Device class                  */
  INT16        devTech;         /* Device technology             */
  SIGNED       devMode;         /* Device ID                     */

  INT16        pixBytes;        /* Bytes per scan line           */
  INT32        pixWidth;        /* Pixels horizontal             */
  INT32        pixHeight;       /* Pixels vertical               */
  INT16        pixResX;         /* Pixels per inch horizontally  */
  INT16        pixResY;         /* Pixels per inch vertically    */
  INT16        pixBits;         /* Color bits per pixel          */
  INT16        pixPlanes;       /* Color planes per pixel        */
  INT16        mapFlags;        /* grafMap flags                 */
  SIGNED       **mapTable[32];  /* Pointers to rowTable(s)       */
  INT16        mapLock;         /* busy semaphore                */ 
  INT16        mapNextSel;      /* OS next segment cookie        */ 
  SIGNED       mapWinType;      /* Bank window type (0-3)        */
  SIGNED       mapWinOffset;    /* Offset to 2nd bank window     */
  SIGNED       mapWinYmin[2];   /* Current bank(s) min Y value   */
  SIGNED       mapWinYmax[2];   /* Current bank(s) max Y value   */
  SIGNED       mapWinPlane;     /* Current bank plane            */
  SIGNED       mapWinScans;     /* Scan lines per bank           */
  SIGNED       mapHandle;       /* Handle to access device       */
  INT32        (*mapBankMgr)(); /* Ptr to bank manager function         */
  VOID         (*mapPlaneMgr)();/* Ptr to plane manager function        */
  VOID         (*mapAltMgr)();  /* Ptr to alt manager function          */
  INT32        (*mapDevMgr)();  /* Ptr to device manager list           */       
  VOID         (*prFill)();     /* primitive vector for fills           */
  VOID         (*prBlitSS)();   /* primitive vector for self-self blits */
  VOID         (*prBlit1S)();   /* primitive vector for mono-self blits */
  VOID         (*prBlitMS)();   /* primitive vector for mem-self  blits */
  VOID         (*prBlitSM)();   /* primitive vector for self-mem  blits */
  VOID         (*prRdImg)();    /* primitive vector for read image      */
  VOID         (*prWrImg)();    /* primitive vector for write image     */
  VOID         (*prLine)();     /* primitive vector for thin lines      */
  VOID         (*prSetPx)();    /* primitive vector for set pixel       */
  SIGNED       (*prGetPx)();    /* primitive vector for get pixel       */
  VOID         (*cbSyncFunc)(struct _grafMap *);/* call back to resync function  */
  VOID         (*cbPostErr)(INT16 ); /* call back to post an error     */
} MWgrafMap;

