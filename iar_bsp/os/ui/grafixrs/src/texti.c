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
*  texti.c                                                      
*
* DESCRIPTION
*
*  Contains the API functions RS_Get_Text_Setup, RS_Text_Setup.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Get_Text_Setup
*  RS_Text_Setup
*
* DEPENDENCIES
*
*  rs_base.h
*  edges.h
*  rsfonts.h
*  fonti.h
*  texti.h
*  globalrsv.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/edges.h"
#include "ui/rsfonts.h"
#include "ui/fonti.h"
#include "ui/texti.h"
#include "ui/globalrsv.h"

/***************************************************************************
* FUNCTION
*
*    RS_Text_Setup
*
* DESCRIPTION
*
*    The API function RS_Text_Setup is used to setup the text structure elements
*    before drawing text.
*
*    The parts of the theTextSetup structure defined in fonti.h
*    that are set in this function are
*
*    a. To set the boldFactor of the text. This can be set to anything,
*       its default is 1 so if that is all you want then it does not need to be set.
*       The Face has to be set to BOLD to use though.
*
*       EX. theTextSetUp.boldFactor = 3;
*
*    b. Set the face. Set it by bit-ORing of one or more combinations.
*       These text face constants are defined in file: \GrafixRS\inc\rsconst.h
*       
*       cNormal         = 0x000
*       cBold           = 0x001
*       cItalic         = 0x002
*       cUnderline      = 0x004
*       cStrikeout      = 0x008
*       cHalftone       = 0x010
*       cMirrorX        = 0x020
*       cMirrorY        = 0x040
*       cProportional   = 0x080
*
*       These text face constants are defined file: \GrafixRS\inc\rsfonts.h
*
*       faceBold         = 0x001
*       faceItalic       = 0x002
*       faceUnderline    = 0x004
*       faceStrikeout    = 0x008
*       faceHalftone     = 0x010
*       faceMirrorX      = 0x020
*       faceMirrorY      = 0x040
*       faceProportional = 0x080
*       faceAntialias    = 0x100
*        
*       Default value is cNormal.
*        
*       EX. theTextSetUP.face = faceProportional;
*            
*    c. To set the mode, the mode is the different RasterOps for drawing Text.  
*       There are 32, 0 to 31. There are 16 standard and 16 transparent. 
*       These are the modes that can be set: 
*
*       zREPz     =  0 
*       zORz      =  1 
*       zXORz     =  2 
*       zNANDz    =  3 
*       zNREPz    =  4 
*       zNORz     =  5 
*       zNXORz    =  6 
*       zANDz     =  7 
*       zCLEARz   =  8   
*       zORNz     =  9 
*       zNOPz     = 10 
*       zANDNz    = 11 
*       zSETz     = 12 
*       zNORNz    = 13   
*       zINVERTz  = 14 
*       zNANDNz   = 15
*       xREPx     = 16 
*       xORx      = 17 
*       xXORx     = 18  
*       xNANDx    = 19 
*       xNREPx    = 20 
*       xNORx     = 21 
*       xNXORx    = 22 
*       xANDx     = 23   
*       xCLEARx   = 24 
*       xORNx     = 25 
*       xNOPx     = 26 
*       xANDNx    = 27 
*       xSETx     = 28   
*       xNORNx    = 29 
*       xINVERTx  = 30
*       xNANDNx   = 31
*
*       EX. theTextSetUp.mode = xXORx;
*
*    d. To set the SPACEExtraSpace, specifies the number of pixels by which to widen each SPACE character 
*       in a string of text. 
*       Default is 0. 
*       Can be a negative number, which means that it can subtract the size of the SPACE character.
*       This is usually used in conjunction with charExtraSpace.
*
*       EX. theTextSetUp.SPACEExtraSpace = 2;
*
*    e. To set the charExtraSpace, specifies the number of pixels by which to widen each character in a string of text. 
*       Default is 0. 
*       Can be a negative number, which means that it can subtract the size of the SPACE character.  
*       This is usually used in conjunction with SPACEExtraSpace.
*
*       EX. theTextSetUp.charExtraSpace = 2;
*
*    f. To set the underlineGap, specifies the number of pixels below the text baseline 
*       at which to draw underlines for characters.  
*       "face" has to be set with "cUnderline" (0x004). 
*       Default is 1. 
*       This is only used with bitmap FONTS. This is used in conjunction with descentUnderlineGap.
*
*       EX. theTextSetUp.underlineGap = 3;
*
*    g. To set the descentUnderlineGap, specifies the amount of gap to leave around 
*       character descenders when drawing underlined text.
*       FACE has to be set with UNDERLINE. 
*       Default is 1. 
*       This is only used with bitmap FONTS. 
*       This is used in conjunction with descentUnderlineGap. 
*       This value has to be 0 or greater.
*
*       EX. theTextSetUp.descentUnderlineGap = 2;
*
*    h. To set the charPath, sets the test output direction for the current port to the specified PATH 
*       orientation. 
*       This is specified with an angular orientation from 0 to 3599, 
*       which is in tenth of degree increments (i.e 0.0 to 359.9).  
*       If the angle is positive then it will move counter clockwise and 
*       if the angle is negative then the text will be moved in a clockwise direction. 
*       This is only used with bitmap FONTS.  
*       For Stroked fonts the charAngle is used with path. 
*       Defines for path that are set up already are: 
*
*       pathRight      0 
*       pathLeft     900 
*       pathUp      1800 
*       pathDown    2700
*
*       EX. theTextSetUp.charPath = pathLeft;
*
*    i. To set the charHorizontalAlign, specifies the alignment positioning for drawing text relative 
*       to the current pen position.  
*       The default value is alignLeft. 
*       The following alignments may be set:    
*
*       HORZ alignLeft   = 0  Align left side of string to the X position.
*            alignCenter = 1  Align center of string to the  X position.
*            alignRight  = 2  Align right side of string to the X position.
*
*       EX. theTextSetUp.charHorizontalAlign = alignRight;
*
*    j. To set the charVerticalAlign, specifies the alignment positioning for drawing text relative 
*       to the current pen position.  
*       The default value is alignBaseline. 
*       The following alignments may be set:    
*
*       VERT alignBaseline = 0  Align baseline of text to current Y position.
*            alignBottom   = 1  Align bottom of text to current Y position.
*            alignMiddle   = 2  Align Middle of text to current Y position.
*            alignTop      = 3  Align top of text to current Y position.
*
*       EX. theTextSetUp.charVerticalAlign = alignBottom;
*
*    k. set the charWidth, sets the width of each character. 
*       This is only used with stroked FONTS. 
*       This has to be set in order to use stroked FONTS.  
*       This value has to be set in conjunction. With charHeight. 
*       The value has to be greater then 1.  
*       By default this value is set to 0 because by default the fonts that are used
*       are bitmap fonts.
*
*       EX. theTextSetUp.charWidth = 14;
*
*    l. To set the charHeight, sets the height of each character.
*       This is only used with stroked FONTS.
*       This has to be set in order to use stroked FONTS.
*       This value has to be set in conjunction. With charWidth.
*       The value has to be greater then 1 when using a stroked font.
*       By default though, this value is set to 0 because by default the fonts that are
*       used are bitmap fonts.
*
*       EX. theTextSetUp.charHeight = 14;
*
*    m. To set the charAngle sets the current rsPort's text angle field (txAngle) for writing stroked fonts.
*       ANGLE (0-3600) specifies the character rotation of characters within a line of text.
*       This is only used with stroked FONTS.
*       For bitmap fonts the path is used.
*       This is specified with an angular orientation from 0 to 3599, which is in tenth of degree
*       increments (i.e 0.0 to 359.9).
*       If the angle is positive then it will move counter clockwise
*       and if the angle is negative then the text will be moved in a clockwise direction.
*
*       EX. theTextSetUp.charAngle = 3000;
*
*    n. To set the charSlant, sets the angle of slant for a character that will be italic.
*       This is only used with stroked FONTS.  The FACE has to be set at least to Italic. The degree of angle can be set between -900 to 900, which i
s actually - 90.0 to 90.0 (this is set to 10th of degree increments). The default value is -200, -20.0.
*
*       EX. theTextSetUp.charSlant = -300;
*
*    o. To set the markerCharAngle sets the angle for the marker to lean towards
*       This is only used with MARKERS.
*       This is specified with an angular orientation from 0 to 3599,
*       which is in tenth of degree increments (i.e 0.0 to 359.9).
*       If the angle is positive then it will move counter clockwise
*       and if the angle is negative then the text will be moved in a clockwise direction.
*       The default value is 4000, which is outside of the range and indicates that a marker was not set.
*
*       EX. theTextSetUp.markerCharAngle = 3000;
*
*    p. To set the markerCharStyle, sets which type of marker that will drawn.
*       This is only used with MARKER.  The value has to be between 1 and 15.
*       By default this value is set to 0 because by markers are not used.
*       The marker types are as follows: 
*
*       0  = None (Default)
*       1  = Dot
*       2  = Cross
*       3  = Star/Asterisk
*       4  = rectangle/Square
*       5  = X
*       6  = Diamond
*       7  = Triangle
*       8  = Dot
*       9  = Cross
*       10 = Star/Asterisk
*       11 = Filled rectangle/Square
*       12 = X
*       13 = Diamond
*       14 = Octagon
*       15 = Triangle
*
*       EX. theTextSetUp.charWidth = 13;
*
*    q. To set the markerCharWidth, sets the width of each character.
*       This is only used with MARKERS. This has to be set in order to use MARKERS.
*       This value has to be set in conjunction. With markerCharHeight.
*       The value has to be greater then 1.
*       Default value is 0 because by default markers are not used.
*
*       EX. theTextSetUp.markerCharWidth = 14;
*
*    r. To set the markerCharHeight, sets the height of each character.
*       This is only used with MARKER. This has to be set in order to use MARKER.
*       This value has to be set in conjunction. With markerCharWidth.
*       The value has to be greater then 1 when using a stroked font.
*       By default though, this value is set to 0 because by default markers are not used.
*
*       EX. theTextSetUp.markerCharHeight = 14;
*
* INPUTS
*
*    TextSetUp *textPtr - Points to the structure containing new values for the global object.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RS_Text_Setup(TextSetUp *textPtr)
{
    fontRcd *theFont;
    INT16   Angle;
            
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Copy the new value for the text setup to the global object. */
    if (textPtr != NU_NULL)
    {
        theTextSetUp = *textPtr;
    }

    theFont = (fontRcd *) theGrafPort.txFont;

    if( (theTextSetUp.markerCharHeight != 0)  && (theTextSetUp.markerCharWidth != 0) )
    {
        /* check if virtual mode */
        if(theGrafPort.portFlags & pfVirtual)
        {
            /* V2GSIZE Virtual To Global size */
            V2GSIZE(theTextSetUp.markerCharWidth, 
                    theTextSetUp.markerCharHeight,
                   &(theTextSetUp.markerCharWidth),
                   &(theTextSetUp.markerCharHeight) );
        }

        if( theTextSetUp.markerCharWidth < 1 )
        {
            /* Sets the value to default because the value was out of range */
            theTextSetUp.markerCharWidth = 1;
        }

        if( theTextSetUp.markerCharHeight < 1 )
        {
            /* Sets the value to default because the value was out of range */
            theTextSetUp.markerCharHeight = 1;
        }

        theGrafPort.mkSize.X = theTextSetUp.markerCharWidth;
        theGrafPort.mkSize.Y = theTextSetUp.markerCharHeight;
        thePort->mkSize.X = theTextSetUp.markerCharWidth;
        thePort->mkSize.Y = theTextSetUp.markerCharHeight;

        if( theTextSetUp.markerCharStyle != 0 )
        {
            if( theTextSetUp.markerCharStyle > 15 )
            {
                /* Sets the value to default because the value was out of range */
                theTextSetUp.markerCharStyle = 0;
            }
            theGrafPort.mkType = theTextSetUp.markerCharStyle;
            thePort->mkType = theTextSetUp.markerCharStyle;
        }

        if( theTextSetUp.markerCharAngle != 4000 )
        {
            theTextSetUp.markerCharAngle = ChkAngle(theTextSetUp.markerCharAngle);
            theGrafPort.mkAngle = theTextSetUp.markerCharAngle;
        }
    }
    
#ifdef      USE_STROKEDFONT

    if( (theTextSetUp.charHeight != 0)  && (theTextSetUp.charWidth != 0) )
    {
        /* smart link stroked text draw */
        txtStrokeIDV = (INT32 (*)())STRKFONT_rsStrokeFontInit;

        if( theFont->fontFlags & fontType )
        {
            txtDrwIDV = txtStrokeIDV;
            if( theGrafPort.txAlign.X == 0 )
            {
                txtAlnIDV = txtStrokeIDV;
            }
        }

        if( theGrafPort.portFlags & pfVirtual )
        {
            /* V2GSIZE Virtual To Global size */
            V2GSIZE(theTextSetUp.charWidth, 
                    theTextSetUp.charHeight,
                    &(theTextSetUp.charWidth),
                    &(theTextSetUp.charHeight) );
        }

        if( theTextSetUp.charWidth < 1 )
        {
            /* Sets the value to default because the value was out of range */
            theTextSetUp.charWidth = 1;
        }

        if( theTextSetUp.charHeight < 1 )
        {
            /* Sets the value to default because the value was out of range */
            theTextSetUp.charHeight = 1;
        }

        theGrafPort.txSize.X = theTextSetUp.charWidth;
        theGrafPort.txSize.Y = theTextSetUp.charHeight;
        thePort->txSize.X = theTextSetUp.charWidth;
        thePort->txSize.Y = theTextSetUp.charHeight;
    }
    
#endif      /* USE_STROKEDFONT */

    if( theTextSetUp.face != cNormal )
    {
        theGrafPort.txFace = theTextSetUp.face;
        thePort->txFace = theTextSetUp.face;

        if( (theTextSetUp.boldFactor != 1) && ( theTextSetUp.face & cBold) )
        {
            theGrafPort.txBold = theTextSetUp.boldFactor;
            thePort->txBold = theTextSetUp.boldFactor;
        }

        if( (theTextSetUp.face & cUnderline) && !(theFont->fontFlags & fontType) )
        {   
            if( theTextSetUp.underlineGap != 1 )
            {
                theGrafPort.txUnder = theTextSetUp.underlineGap;
                thePort->txUnder = theTextSetUp.underlineGap;
            }

            if( theTextSetUp.descentUnderlineGap != 1 )
            {
                theGrafPort.txScore = theTextSetUp.descentUnderlineGap;
                thePort->txScore = theTextSetUp.descentUnderlineGap;
            }
        }

#ifdef      USE_STROKEDFONT

        if( (theTextSetUp.face & cItalic) && (theTextSetUp.charSlant != -200) )
        {
            theTextSetUp.charSlant = ChkAngle(theTextSetUp.charSlant);
            theGrafPort.txSlant = theTextSetUp.charSlant;
            thePort->txSlant = theTextSetUp.charSlant;  
        }
        
#endif      /* USE_STROKEDFONT */
        
    }

    if( theTextSetUp.mode > 31 )
    {
        /* Sets the value to default because the value was out of range */
        theTextSetUp.mode = 1;
    }
    else if( theTextSetUp.mode != DEFAULT_VALUE )
    {
        theGrafPort.txMode  = theTextSetUp.mode;
        grafBlit.blitRop = theTextSetUp.mode;
        thePort->txMode  = theTextSetUp.mode;
    }

    if( theTextSetUp.charPath != pathRight )
    {
        Angle = ChkAngle(theTextSetUp.charPath);
        theGrafPort.txPath = Angle;
        thePort->txPath = Angle;

#ifdef      USE_STROKEDFONT
        
        if( (theFont->fontFlags & fontType) && (theTextSetUp.charAngle != DEFAULT_VALUE) )
        {
            theTextSetUp.charAngle = ChkAngle(theTextSetUp.charAngle);
            theGrafPort.txAngle = theTextSetUp.charAngle;
            thePort->txAngle  = theTextSetUp.charAngle;
        }

#endif      /* USE_STROKEDFONT */
        
    }
    else
    {
        theTextSetUp.charPath = pathRight;
        Angle = ChkAngle(theTextSetUp.charPath);
        theGrafPort.txPath = Angle;
        thePort->txPath = Angle;
    }

    if( theTextSetUp.SPACEExtraSpace != 0 )
    {
        theGrafPort.txSpace = theTextSetUp.SPACEExtraSpace;
        thePort->txSpace = theTextSetUp.SPACEExtraSpace;
    }

    if( theTextSetUp.charExtraSpace != 0 )
    {
        theGrafPort.txExtra = theTextSetUp.charExtraSpace;
        thePort->txExtra = theTextSetUp.charExtraSpace;
    }

    if(    (theTextSetUp.charHorizontalAlign != alignLeft)
        || (theTextSetUp.charVerticalAlign != alignBaseline) )
    {
    txtAPPIDV = (INT32 (*)())BMAPF_TextAlignHorizontalCR;

        /* Limit both side from 0 - 3  */
        theTextSetUp.charVerticalAlign   = theTextSetUp.charVerticalAlign & 3;
        theTextSetUp.charHorizontalAlign = theTextSetUp.charHorizontalAlign & 3;
        
        if( theTextSetUp.charHorizontalAlign == 0 )
        {
            txtAlnIDV = txtDrwIDV;
        }
        else
        {
            txtAlnIDV = txtAPPIDV;
        }

        fntVertAln = fntVertTbl[theTextSetUp.charVerticalAlign];

        theGrafPort.txAlign.X = theTextSetUp.charHorizontalAlign;
        theGrafPort.txAlign.Y = theTextSetUp.charVerticalAlign;

        thePort->txAlign.X = theTextSetUp.charHorizontalAlign;
        thePort->txAlign.Y = theTextSetUp.charVerticalAlign;
    }

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    RS_Get_Text_Setup
*
* DESCRIPTION
*
*    Function RS_Get_Text_Setup returns back the current text setup.
*
* INPUTS
*
*    TextSetUp *textPtr - Points to the structure where current values of the global object is tobe placed.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RS_Get_Text_Setup(TextSetUp *textPtr)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if (textPtr != NU_NULL)
    {
        *textPtr = theTextSetUp;
    }

    /* Return to user mode */
    NU_USER_MODE();
}
