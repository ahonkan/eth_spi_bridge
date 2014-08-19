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
*  texts_polymarker.c                                                      
*
* DESCRIPTION
*
*  Contains text support function - PolyMarker.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PolyMarker
*
* DEPENDENCIES
*
*  rs_base.h
*  rsfonts.h
*  markers.h
*  texts.h
*  textd.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rsfonts.h"
#include "ui/markers.h"
#include "ui/texts.h"
#include "ui/textd.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    PolyMarker
*
* DESCRIPTION
*
*    Function PolyMarker draws the current marker symbol centered at each point
*    specified in the XYPTS array.
*
* INPUTS
*
*    INT16 NPts    - Number of points.
*
*    point * XYPts - Pointer to the point array.
*
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID PolyMarker( INT16 NPts, point * XYPts)
{
    INT32 i;
    signed char  markchar;
    point *markPt;
    signed char  setFontBaseName[16] = "Marker Font";
    signed char  setFontSign[9] = "NU__FONT";

    struct marker 
    {
        SIGNED svtxFont;
        point svtxSize;
        INT16 svtxAngle;
        INT16 svtxPath;
    };

    struct marker Save;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* initialize marker font structure and tables */
    MarkerFont.fontVer      = 0x22;
    MarkerFont.fontRev      = 1;
    MarkerFont.nameLen      = 11;
    MarkerFont.fontFacing   = 0;
    MarkerFont.fontWeight   = 0;
    MarkerFont.fontCoding   = 0;
    MarkerFont.fontSize     = 0;
    MarkerFont.fontMax      = 79;
    MarkerFont.fontMin      = 65;
    MarkerFont.fontPtSize   = 0;
    MarkerFont.fontfamily   = 0;
    MarkerFont.fontStyle    = 0;
    MarkerFont.fontFlags    = 6;
    MarkerFont.fontColor[0] = -1;
    MarkerFont.fontColor[1] = 0;
    MarkerFont.minChar      = 65;
    MarkerFont.maxChar      = 79;
    MarkerFont.chWidth      = 10;
    MarkerFont.chHeight     = 14;
    MarkerFont.chKern       = 0;
    MarkerFont.ascent       = 14;
    MarkerFont.descent      = 0;
    MarkerFont.lnSpacing    = 17;
    MarkerFont.chBad        = 12;
    MarkerFont.chCenter.X   = 5;
    MarkerFont.chCenter.Y   = 7;
    MarkerFont.chAngle      = 0;
    MarkerFont.chUnder      = 2;
    MarkerFont.chScore      = 2;
    MarkerFont.locTbl       = (SIGNED) &locnTable - (SIGNED) &MarkerFont;
    MarkerFont.ofwdTbl      = (SIGNED) &ofwdTable - (SIGNED) &MarkerFont;
    MarkerFont.kernTbl      = 0;
    MarkerFont.sizeTbl      = 0;
    MarkerFont.grafMapTbl   = 0;
    MarkerFont.rowTbl       = 0;
    MarkerFont.fontTbl      = (SIGNED) &FontTable - (SIGNED) &MarkerFont;

    for( i = 0; i < 16; i++)
    {
        MarkerFont.fontBaseName[i] = setFontBaseName[i];
    }
    for( i = 0; i < 8; i++)
    {
        MarkerFont.fontSign[i] = setFontSign[i];
    }

    locnTable[0]  = &C_65[0] - &FontTable;
    locnTable[1]  = &C_66[0] - &FontTable;
    locnTable[2]  = &C_67[0] - &FontTable;
    locnTable[3]  = &C_68[0] - &FontTable;
    locnTable[4]  = &C_69[0] - &FontTable;
    locnTable[5]  = &C_70[0] - &FontTable;
    locnTable[6]  = &C_71[0] - &FontTable;
    locnTable[7]  = &C_72[0] - &FontTable;
    locnTable[8]  = &C_73[0] - &FontTable;
    locnTable[9]  = &C_74[0] - &FontTable;
    locnTable[10] = &C_75[0] - &FontTable;
    locnTable[11] = &C_76[0] - &FontTable;
    locnTable[12] = &C_77[0] - &FontTable;
    locnTable[13] = &C_78[0] - &FontTable;
    locnTable[14] = &C_79[0] - &FontTable;

    /* 56 "Bad character" */
    locnTable[15] = &C_80[0] - &FontTable;  

#ifdef      USE_STROKEDFONT
    txtStrokeIDV = (INT32 (*)())STRKFONT_rsStrokeFontInit;
#endif      /* USE_STROKEDFONT */

    /* Save the text state */
    Save.svtxFont   = (SIGNED) theGrafPort.txFont;

    Save.svtxPath   = theGrafPort.txPath;
    
#ifdef      USE_STROKEDFONT

    Save.svtxSize.X = theGrafPort.txSize.X;
    Save.svtxSize.Y = theGrafPort.txSize.Y;
    Save.svtxAngle  = theGrafPort.txAngle;

    /* set stroke text settings from marker settings */
    theGrafPort.txSize.X = theGrafPort.mkSize.X;
    theGrafPort.txSize.Y = theGrafPort.mkSize.Y;
    theGrafPort.txAngle  = theGrafPort.mkAngle;

#endif      /* USE_STROKEDFONT */
    
    theGrafPort.txPath   = 0;

    if( theGrafPort.mkFont == 0 )
    {
        SetFont(&MarkerFont);
    }
    else
    {
        SetFont((fontRcd *)theGrafPort.mkFont);
    }

    markchar = theGrafPort.mkType + 64;
    markPt = XYPts;
    for( i=0; i < NPts; i++)
    {
        MoveTo(markPt->X,markPt->Y);
        txtDrwIDV(&markchar, 0, 1, 0);
        markPt++;
    }

#ifdef      USE_STROKEDFONT
    
    theGrafPort.txSize.X = Save.svtxSize.X;
    theGrafPort.txSize.Y = Save.svtxSize.Y;
    theGrafPort.txAngle  = Save.svtxAngle;

#endif      /* USE_STROKEDFONT */
    
    theGrafPort.txPath   = Save.svtxPath;
    theGrafPort.txFont   = (fontRcd *) Save.svtxFont;

    SetFont((fontRcd *)theGrafPort.txFont);

    /* Return to user mode */
    NU_USER_MODE();
}
