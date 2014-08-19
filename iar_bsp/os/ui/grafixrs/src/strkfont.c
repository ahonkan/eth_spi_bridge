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
*  strkfont.c                                                   
*
* DESCRIPTION
*
*  This file contains stroked font functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  STRKFONT_rsStrokeText
*  STRKFONT_rsStrokeFontInit
*  ScaleNode
*  ScalePath
*
* DEPENDENCIES
*
*  rs_base.h
*  rsfonts.h
*  strkfont.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rsfonts.h"
#include "ui/strkfont.h"

#ifdef      USE_STROKEDFONT

extern SIGNED  FontFile_BaseAddress;

#ifndef FIXPOINT
    float qDxx;
    float qDxy;
    float qDyy;
    float qDyx;
    float qPxx;
    float qPxy;
    float qPyy;
    float qPyx; 
    float sinSlant;
    float cosSlant;
#else
    SIGNED qDxx;
    SIGNED qDxy;
    SIGNED qDyy;
    SIGNED qDyx;
    SIGNED qPxx;
    SIGNED qPxy;
    SIGNED qPyy;
    SIGNED qPyx;
    SIGNED sinSlant;
    SIGNED cosSlant;
#endif

/* Stroked Font Variables */
static  INT16 StrokeFlags;


/***************************************************************************
* FUNCTION
*
*    STRKFONT_rsStrokeText
*
* DESCRIPTION
*
*    Function STRKFONT_rsStrokeText draws characters from the specified text string, TEXT, 
*    beginning at "INDEX" characters from the start of the string, and continuing  
*    for "COUNT" characters or until a null (x'00') character is encountered 
*    (which ever occurs first).  The text is draw beginning at the current pen 
*    location and ends up at the start of the next character position past the 
*    last character drawn.
*
* INPUTS
*
*    signed char *TEXT     - Pointer to the text string.
*
*    INT32 INDEX    - Beginning index.
*
*    INT32 COUNT    - Number of characters.
*
*    INT32 CHARSIZE - Character size.
*
* OUTPUTS
*
*    None
*
***************************************************************************/
INT32 STRKFONT_rsStrokeText( signed char *TEXT, INT32 INDEX, INT32 COUNT, INT32 CHARSIZE)
{
    typedef struct _nodePt
    {
        signed char X;
        signed char Y;
    } nodePt;

    /* pointer to current TEXT character */
    signed char  *adsText;

    /* pointer to location table */
    SIGNED  adsLocTbl;         
    UINT16  *locTblPtr;

    /* pointer to offset/width table */
    SIGNED  adsOfWdTbl;       

    /* pointer to nodes table */
    SIGNED  adsNodeTbl;       

    /* this chars node pointer */
    nodePt  *chNodeTbl;       

    /* termination char */
    INT32   txTrm;            

    /* character index */
    INT32   chIndex;          

    /* character code */
    UINT8   aCH;              

    /* node operation */
    signed char mov_draw;  

    /* scaled fixed width */
    point   fxWid;            
    
    /* scaled text extra */
    point   txExt;            

    /* scaled space extra */
    point   txSpc;            
    point   wd;

    /* vertical alignment */
    INT16   vAlign;           
    INT32   baseX, baseY, lastX, lastY;
	INT32   width;

    /* node center */
    INT32   chCtrX, chCtrY;   
    point   temp;
    nodePt  temNode;
    fontRcd *Font;
    ofswid  *stOfWdTbl;

    lineExecPntr = (VOID (*)()) lineExecIDV;

    grafBlit.blitRop = theGrafPort.txMode;
    Font = (fontRcd *) theGrafPort.txFont;

    /* this is to fix a paradigm compiler warning */
    (VOID)CHARSIZE;

    lastX = 0;
    lastY = 0;

    /* Get pointer to users text string */
    adsText = TEXT + INDEX;

    /* compute pointer to location table */
    adsLocTbl = (SIGNED) Font + Font->locTbl;

    /* compute pointer to offset/width table */
    adsOfWdTbl = (SIGNED) Font + Font->ofwdTbl;

    /* compute pointer to node table */
    adsNodeTbl = (SIGNED) Font + Font->fontTbl;
    
    chCtrX = Font->chCenter.X;
    chCtrY = Font->chCenter.Y;

    /* compute vertical alignment (char origins are at LOWER left)
       precomputed fntVertAlign is at UPPER left */
    vAlign = -((Font->chHeight - 1) - fntVertAln);

    /* Scale fixed space size */
    ScalePath(Font->fxSpac, 0, &fxWid);

    /* Scale text extra size */
    ScalePath(theGrafPort.txExtra, 0, &txExt);

    /* Scale text space size */
    ScalePath(theGrafPort.txSpace, 0, &txSpc);

    /* Get termination character */
    txTrm = theGrafPort.txTerm;

    /* For each character in the string */
    while( COUNT-- > 0 ) 
    {
        aCH = (UINT8) *adsText++;
        if( aCH == (UINT8)txTrm )
        {
            break;
        }
        if( (aCH < Font->minChar) || (aCH > Font->maxChar) )
        {
            /*  handle char out of range.
                Note: stroked fonts don't support the chBad field.
                The bad character for stroked fonts is always the
                character one beyond the maxChar. */
            aCH = Font->maxChar + 1;
        }

        /* Compute index in font for this character */
        chIndex = aCH - Font->minChar;

        stOfWdTbl = ((ofswid *) (adsOfWdTbl + (chIndex << 1)));
        if( (stOfWdTbl->wid == 0xFF) && (stOfWdTbl->ofs == -1) )
        {
            chIndex = Font->maxChar - Font->minChar + 1;
        }

        locTblPtr = (UINT16 *) adsLocTbl + chIndex;
        chNodeTbl = (nodePt *) (adsNodeTbl + *locTblPtr);

        /* Scale offset and vertical alignment */
        ScaleNode(stOfWdTbl->ofs, vAlign, &temp); 
        if( theGrafPort.txFace & faceMirrorX )
        {
            temp.X = - temp.X;
        }
        if( !(theGrafPort.txFace & faceMirrorY) )
        {
            temp.Y = - temp.Y;
        }

        baseX = LocX + temp.X;
        baseY = LocY + temp.Y;

        mov_draw = 0;

        /* Advance to the next Node  */
        while (1)
        {
            temNode = *chNodeTbl;
            chNodeTbl = (nodePt *) ((SIGNED) chNodeTbl + 2);
            if( temNode.X == -1 )
            {
                /* Control node */
                if( temNode.Y == -2 )
                {
                    /* advance to next node */
                    mov_draw = 0;
                    continue;
                }
                if( temNode.Y == -3 )
                {
                    /* Handle filled rectangle control node */
                    temp.X = chNodeTbl->X - chCtrX;
                    temp.Y = chNodeTbl->Y - chCtrY;
                    ScaleNode(temp.X, temp.Y, &temp);
                    grafBlist.Xmin = temp.X;
                    grafBlist.Ymin = temp.Y;
                    chNodeTbl = (nodePt *) ((SIGNED) chNodeTbl + 2);

                    temp.X = chNodeTbl->X - chCtrX;
                    temp.Y = chNodeTbl->Y - chCtrY;
                    ScaleNode(temp.X, temp.Y, &temp);
                    grafBlist.Xmax = temp.X;
                    grafBlist.Ymax = temp.Y;
                    chNodeTbl = (nodePt *) ((SIGNED) chNodeTbl + 2);

                    if( theGrafPort.txFace & faceMirrorX )
                    {
                        grafBlist.Xmin = - grafBlist.Xmin;
                        grafBlist.Xmax = - grafBlist.Xmax;
                    }

                    /* Factor in pen location */
                    grafBlist.Xmin += baseX;
                    grafBlist.Xmax += baseX;

                    if( !(theGrafPort.txFace & faceMirrorY) )
                    {
                        grafBlist.Ymin = - grafBlist.Ymin;
                        grafBlist.Ymax = - grafBlist.Ymax;
                    }

                    /* Factor in pen location */
                    grafBlist.Ymin += baseY;
                    grafBlist.Ymax += baseY;

                    /* Draw it */
                    if( theGrafPort.pnLevel >= 0 )
                    {
                        grafBlist.Xmax++;
                        grafBlist.Ymax++;
                        grafBlit.blitDmap->prFill(&grafBlit);
                    }
                    continue;
                }

                /* Must be the end of the character and handle spacing  */
                if( theGrafPort.txFace & faceProportional )
                {
                    stOfWdTbl = ((ofswid *) (adsOfWdTbl + (chIndex << 1)));
                    ScalePath(stOfWdTbl->wid, 0, &wd);
                }
                else
                {
                    wd = fxWid;
                }

                temp.X = wd.X + txExt.X;
                temp.Y = wd.Y + txExt.Y;
                if( aCH == ' ' )
                {
                    temp.X += txSpc.X;
                    temp.Y -= txSpc.Y;
                }

                LocX += temp.X;
                LocY += temp.Y;

                /* end of processing for one character */
                break;  
            }
            else 
            {
                temp.X = temNode.X - chCtrX;
                temp.Y = temNode.Y - chCtrY;

                if( chNodeTbl->X == -1 )
                {
                    /* Next node is a control node */
                    grafBlist.skipStat = SkipLast;
                }
                else
                {
                    grafBlist.skipStat = NoSkip;
                }

                ScaleNode(temp.X, temp.Y, &temp);

                /* Factor in Pen locations  */      
                if( theGrafPort.txFace & faceMirrorX )
                {
                    temp.X = -temp.X;
                }
                if( !(theGrafPort.txFace & faceMirrorY) )
                {
                    temp.Y = -temp.Y;
                }

                temp.X += baseX;
                temp.Y += baseY;

                if( (mov_draw == -1) && (theGrafPort.pnLevel >= 0) )
                {
                    /* Draw it */
                    grafBlist.Xmin = lastX;
                    grafBlist.Ymin = lastY;
                    grafBlist.Xmax = temp.X;
                    grafBlist.Ymax = temp.Y;

                    lineExecPntr(&grafBlit);

                }

                /* set up next vector */
                lastX = temp.X;
                lastY = temp.Y;
                mov_draw = -1;
            }
        }
    }
	width = thePort->pnLoc.X - LocX;

    /* Done with the all the characters */
    grafBlit.blitRop = theGrafPort.pnMode;

    if( globalLevel > 0 )
    {
        G2UP(LocX, LocY, &theGrafPort.pnLoc.X, &theGrafPort.pnLoc.Y);
    }
    else
    {
        theGrafPort.pnLoc.X = LocX;
        theGrafPort.pnLoc.Y = LocY;
    }

    thePort->pnLoc.X = theGrafPort.pnLoc.X;
    thePort->pnLoc.Y = theGrafPort.pnLoc.Y;
	
	return(width);

}

/***************************************************************************
* FUNCTION
*
*    STRKFONT_rsStrokeFontInit
*
* DESCRIPTION
*
*    Function STRKFONT_rsStrokeFontInit initializes the current font for stroked font
*    processing by pre-computing a number of variables.
*
*    STRKFONT_rsStrokeFontInit is called indirectly via the txDrwIDV vector 
*    whenever stroked text is to be drawn but has not been initialized. 
*    STRKFONT_rsStrokeFontInit initializes the stroked font system, replaces the
*    txDrwIDV vector with the stroked font drawing routine, and the issues a call 
*    to the stroked font drawing routine.
*
*    Further calls via txDrwIDV will vector directly to the stroked font drawing
*    routine, bypassing initialization.
*
*    The following formulas are used in processing global bitmap coordinates:
*
*        A = -A;  {angle}
*        x' = (x*cosA*resY + y*sinA*resX)/ resY
*        y' = (y*cosA*resX - x*sinA*resY)/ resX
*
*        x' = x*cosA + y*sinA*resX/resY;
*        y' = y*cosA - x*sinA*resY/resX;
*
*    scaling for unit size (sizeX/fxSpac) & (sizeY/leading):
*
*        A = -(txPath + txAngle);
*        x' = x*(sizeX/fxSpac)*cosA + y*(sizeY/leading)*((sinA*resX)/resY));
*        y' = y*(sizeY/leading)*cosA - x*(sizeX/fxWidth)*((sinA*resY)/resX));
*
*       let:
*        qDxx = (sizeX/fxSpac)*cosA;
*        qDxy = (sizeY/leading)*((sinA*resX)/resY));
*        qDyy = (sizeY/leading)*cosA;
*        qDyx = (sizeX/fxWidth)*((sinA*resY)/resX));
*
*       then:
*        x' = x*qDxx + y*qDxy;
*        y' = y*qDyy - x*qDyx;
*
* INPUTS
*
*    signed char *TEXT     - Pointer to the text string.
*
*    INT32 INDEX    - Beginning index.
*
*    INT32 COUNT    - Number of characters.
*
*    INT32 CHARSIZE - Character size.
*
* OUTPUTS
*
*    None
*
***************************************************************************/
#ifdef FIXPOINT
INT32 STRKFONT_rsStrokeFontInit(signed char *TEXT, INT32 INDEX, INT32 COUNT , INT32 CHARSIZE )
{
    /* these are double in the floating point but they don't
       seem to need to be */
    SIGNED tLead, tSpace, txSizeX, txSizeY; 
    SIGNED qPathSin, qPathCos;
    SIGNED tempAngle,tempSinCos;
    SIGNED qTmp1, qTmp2;
    SIGNED bmResX, bmResY, bmTan, bmCot;  
    SIGNED slantSF = 0x7FFF0000;
    fontRcd *theFont;
	INT32	width = 0;

    theFont = (fontRcd *) theGrafPort.txFont;
    StrokeFlags = 0;

    /* Pickup the resolution values from the bitmap */
    bmResX = theGrafPort.portMap->pixResX << 16;
    bmResY = theGrafPort.portMap->pixResY << 16;
    bmTan  = Fix_div(bmResX,bmResY);
    bmCot  = Fix_div(bmResY,bmResX);

    /* Pickup some things from the font */
    tLead = theFont->lnSpacing << 16;
    tSpace = theFont->fxSpac << 16;

    if( theGrafPort.txSize.X <= 0 )
    {
        theGrafPort.txSize.X = theFont->chWidth;
    }

    if( theGrafPort.txSize.Y <= 0 )
    {
        theGrafPort.txSize.Y = theFont->chHeight;
    }

    txSizeX = theGrafPort.txSize.X << 16;
    txSizeY = theGrafPort.txSize.Y << 16;

    if( theGrafPort.txPath != 0 )
    {
        StrokeFlags = StrokeFlags | thePath;
    }
    if( theGrafPort.txAngle != 0 )
    {
        StrokeFlags = StrokeFlags | angle;
    }

    /* Compute Slant variables */
    if( (theGrafPort.txFace & faceItalic) && (theGrafPort.txSlant != 0) )
    {
        StrokeFlags = StrokeFlags | slant;
        tempSinCos = iSin(theGrafPort.txSlant);
        if( tempSinCos < 0 )
        {
            StrokeFlags = StrokeFlags | signSlantSin;
            tempSinCos  = -tempSinCos;
        }

        sinSlant = Fix_mul(slantSF,tempSinCos);

        tempSinCos = iCos(theGrafPort.txSlant);
        if( tempSinCos < 0 )
        {
            StrokeFlags = StrokeFlags | signSlantCos;
            tempSinCos  = -tempSinCos;
        }

        cosSlant = Fix_mul(slantSF,tempSinCos);
    }

    /* Compute path variables */
    qPathSin = iSin(theGrafPort.txPath);
    qPathCos = iCos(theGrafPort.txPath);

    qTmp1 = Fix_div(txSizeX,tSpace);
    qTmp2 = Fix_div(txSizeY,tLead);

    qPxx  = Fix_mul(qTmp1, qPathCos);
    qPyy  = Fix_mul(qTmp2, qPathCos);
    qPxy  = Fix_mul(qTmp2, qPathSin);
    qPxy  = Fix_mul(qPxy, bmTan);
    qPyx  = Fix_mul(qTmp1,qPathSin);
    qPyx  = Fix_mul(qPyx, bmCot);

    tempAngle = -(theGrafPort.txPath + theGrafPort.txAngle);
    qPathSin  = iSin(tempAngle);
    qPathCos  = iCos(tempAngle);

    qDxx = Fix_mul(qTmp1, qPathCos);
    qDyy = Fix_mul(qTmp2, qPathCos);
    qDxy = Fix_mul(qTmp2, qPathSin);
    qDxy = Fix_mul(qDxy, bmTan);
    qDyx = Fix_mul(qTmp1, qPathSin);
    qDyx = Fix_mul(qDyx, bmCot);

    /* Set draw vectors to real draw routine */
    txtDrwIDV =  (INT32 (*)())STRKFONT_rsStrokeText;
    txtAlnIDV =  (INT32 (*)())STRKFONT_rsStrokeText;

    if( theGrafPort.txAlign.X != alignLeft )
    {
        txtAlnIDV = txtAPPIDV;
    }


    /* Issue draw call */
    width = STRKFONT_rsStrokeText(TEXT,INDEX,COUNT,CHARSIZE);

	return(width);
}

/* floating-point math */
#else 
INT32 STRKFONT_rsStrokeFontInit( signed char *TEXT, INT32 INDEX, INT32 COUNT , INT32 CHARSIZE)
{
    double  tLead, tSpace; 
    double  qPathSin, qPathCos;
    double  tempAngle, tempSinCos;
    double  qTmp1, qTmp2;
    double  bmResX, bmResY, bmTan, bmCot;    
    double  slantSF = 32767;
    fontRcd *theFont;
	INT32	width;

    theFont = (fontRcd *) theGrafPort.txFont;
    StrokeFlags = 0;

    /* Pickup the resolution values from the bitmap  */
    bmResX = theGrafPort.portMap->pixResX;
    bmResY = theGrafPort.portMap->pixResY;
    bmTan = bmResX / bmResY;
    bmCot = bmResY / bmResX;

    /* Pickup some things from the font  */
    tLead = theFont->lnSpacing;
    tSpace = theFont->fxSpac;

    if( theGrafPort.txSize.X <= 0 )
    {
        theGrafPort.txSize.X = theFont->chWidth;
    }

    if( theGrafPort.txSize.Y <= 0 )
    {
        theGrafPort.txSize.Y = theFont->chHeight;
    }

    if( theGrafPort.txPath != 0 )
    {
        StrokeFlags = StrokeFlags | thePath;
    }
    if(theGrafPort.txAngle != 0 )
    {
        StrokeFlags = StrokeFlags | angle;
    }

    /* Compute Slant variables  */
    if( (theGrafPort.txFace & faceItalic) && (theGrafPort.txSlant != 0) )
    {
        StrokeFlags = StrokeFlags | slant;
        tempAngle = 3.1415926535 * theGrafPort.txSlant / 1800.0;

        tempSinCos = sin(tempAngle);
        if( tempSinCos < 0 )
        {
            StrokeFlags = StrokeFlags | signSlantSin;
            tempSinCos = -tempSinCos;
        }

        sinSlant = (INT16)(slantSF * tempSinCos);
    
        tempSinCos = cos(tempAngle);
        if( tempSinCos < 0 )
        {
            StrokeFlags = StrokeFlags | signSlantCos;
            tempSinCos = -tempSinCos;
        }

        cosSlant = (INT16)(slantSF * tempSinCos);
    }

    /* Compute path variables  */
    tempAngle = 3.1415926535 * theGrafPort.txPath / 1800.0;
    qPathSin = sin(tempAngle);
    qPathCos = cos(tempAngle);

    qTmp1 = (float) theGrafPort.txSize.X / (float) tSpace;
    qTmp2 = (float) theGrafPort.txSize.Y / (float) tLead;

    qPxx = (float) (qTmp1 * qPathCos);
    qPyy = (float) (qTmp2 * qPathCos);
    qPxy = (float) (qTmp2 * qPathSin * bmTan);
    qPyx = (float) (qTmp1 * qPathSin * bmCot);

    tempAngle = -3.1415926535 * (theGrafPort.txPath + theGrafPort.txAngle) / 1800.0;
    qPathSin = sin(tempAngle);
    qPathCos = cos(tempAngle);

    qDxx = (float) (qTmp1 * qPathCos);
    qDyy = (float) (qTmp2 * qPathCos);
    qDxy = (float) (qTmp2 * qPathSin * bmTan);
    qDyx = (float) (qTmp1 * qPathSin * bmCot);

    /* Set draw vectors to real draw routine  */
    txtDrwIDV =  STRKFONT_rsStrokeText;
    txtAlnIDV =  STRKFONT_rsStrokeText;
    if( theGrafPort.txAlign.X != alignLeft )
    {
        txtAlnIDV = txtAPPIDV;
    }

    /* Issue draw call  */
    width = STRKFONT_rsStrokeText(TEXT,INDEX,COUNT,CHARSIZE);
	return(width);
}
#endif

/***************************************************************************
* FUNCTION
*
*    ScaleNode
*
* DESCRIPTION
*
*    Function ScaleNode scales for unit size (sizeX/fxSpac) & (sizeY/leading):
*
*        x' = x*(sizeX/fxSpac)*cosA + y*(sizeY/leading)*((sinA*resX)/resY));
*        y' = y*(sizeY/leading)*cosA - x*(sizeX/fxWidth)*((sinA*resY)/resX));
*
*        let:  (calculated in STRKFONT_rsStrokeFontInit)
*
*        qDxx = (sizeX/fxSpac)*cosA;
*        qDxy = (sizeY/leading)*((sinA*resX)/resY));
*        qDyy = (sizeY/leading)*cosA;
*        qDyx = (sizeX/fxWidth)*((sinA*resY)/resX));
*
*      then:
*        x' = x*qDxx + y*qDxy;
*        y' = y*qDyy - x*qDyx;
*
* INPUTS
*
*    INT32 nodeX        - x scale.
*
*    INT32 nodeY        - y scale.
*
*    point *scaledPoint - Pointer to the scaled point.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
#ifdef FIXPOINT
VOID ScaleNode(INT32 nodeX, INT32 nodeY, point *scaledPoint)
{
    SIGNED tempSPX, tempSPY;
    SIGNED snX, snY;
    SIGNED bmResX, bmResY;
    SIGNED bmSinSlant, bmCosSlant;
    SIGNED slantSF = 0x7FFF0000;
    SIGNED roundValue = 0x00008000;

    snX = nodeX << 16;
    snY = nodeY << 16;
    bmResX = theGrafPort.portMap->pixResX << 16;
    bmResY = theGrafPort.portMap->pixResY << 16;

    if( StrokeFlags & slant )
    {
        bmSinSlant = Fix_div(sinSlant,slantSF);
        if( StrokeFlags & signSlantSin ) 
        {
            bmSinSlant = -bmSinSlant;
        }
        bmCosSlant = Fix_div(cosSlant,slantSF);
        if( StrokeFlags & signSlantCos)
        {
            bmCosSlant = - bmCosSlant;
        }

        snX = (snX - Fix_mul(Fix_mul(snY,bmSinSlant),Fix_div(bmResX,bmResY)));
        snY = Fix_mul(snY,bmCosSlant);
    }

    if( StrokeFlags & (thePath | angle) )
    {
        tempSPX = Fix_mul(snX,qDxx) + Fix_mul(snY,qDxy);
        tempSPY = (Fix_mul(snY,qDyy) - Fix_mul(snX,qDyx));
    }
    else
    {
        tempSPX = Fix_mul(snX,qDxx);
        tempSPY = Fix_mul(snY,qDyy);
    }

    if( (tempSPX >> 16) >= 0 )
    {
        /* round up */
        scaledPoint->X = (INT16) ((tempSPX + roundValue) >> 16);
    }
    else
    {
        /* round down */
        scaledPoint->X = (INT32) ((tempSPX - roundValue) >> 16);
    }

    if( (tempSPY >> 16) >= 0 )
    {
        /* round up */
        scaledPoint->Y = (INT32) ((tempSPY + roundValue) >> 16);
    }
    else
    {
        /* round down */
        scaledPoint->Y = (INT32) ((tempSPY - roundValue) >> 16);
    }

}

/* floating-point math */
#else 
VOID ScaleNode(INT32 nodeX, INT32 nodeY, point *scaledPoint)
{
    float tempSPX, tempSPY;
    float snX, snY;
    float bmResX, bmResY;
    float bmSinSlant, bmCosSlant;
    float slantSF = 32767;
    float roundValue = 0.5;

    snX = (float)nodeX;
    snY = (float)nodeY;
    bmResX = theGrafPort.portMap->pixResX;
    bmResY = theGrafPort.portMap->pixResY;

    if( StrokeFlags & slant )
    {
        bmSinSlant = (float) sinSlant / slantSF;

        if( StrokeFlags & signSlantSin)
        {
            bmSinSlant = - bmSinSlant;
        }
        bmCosSlant = (float) cosSlant / slantSF;

        if( StrokeFlags & signSlantCos )
        {
            bmCosSlant = - bmCosSlant;
        }
        snX -= (snY * bmSinSlant * (bmResX / bmResY));
        snY *= bmCosSlant;
    }
    
    if( StrokeFlags & (thePath | angle) )
    {
        tempSPX = snX * qDxx + snY * qDxy;
        tempSPY = snY * qDyy - snX * qDyx;
    }
    else
    {
        tempSPX = snX * qDxx;
        tempSPY = snY * qDyy;
    }

    if( tempSPX >= 0 )
    {
        /* round up */
        scaledPoint->X = (INT32) (tempSPX + roundValue);
    }
    else
    {
        /* round down */
        scaledPoint->X = (INT32) (tempSPX - roundValue);
    }

    if( tempSPY >= 0 )
    {
        /* round up */
        scaledPoint->Y = (INT32) (tempSPY + roundValue);
    }
    else
    {
        /* round down */
        scaledPoint->Y = (INT32) (tempSPY - roundValue);
    }

}
#endif
   

/***************************************************************************
* FUNCTION
*
*    ScalePath
*
* DESCRIPTION
*
*    Function ScalePath scales for unit size (sizeX/fxSpac) & (sizeY/leading):
*
*        x' = x*(sizeX/fxSpac)*cosA + y*(sizeY/leading)*((sinA*resX)/resY));
*        y' = y*(sizeY/leading)*cosA - x*(sizeX/fxWidth)*((sinA*resY)/resX));
*
*        let:  (calculated in STRKFONT_rsStrokeFontInit)
*
*        qPxx = (sizeX/fxSpac)*cosA;
*        qPxy = (sizeY/leading)*((sinA*resX)/resY));
*        qPyy = (sizeY/leading)*cosA;
*        qPyx = (sizeX/fxWidth)*((sinA*resY)/resX));
*
*   then:
*        x' = x*qPxx + y*qPxy;
*        y' = y*qPyy - x*qPyx;
*
*
* INPUTS
*
*    INT32 nodeX        - x scale.
*
*    INT32 nodeY        - y scale.
*
*    point *scaledPoint  - Pointer to the scaled point.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
#ifdef FIXPOINT
VOID ScalePath( INT32 nodeX, INT32 nodeY, point *scaledPoint)
{
    SIGNED tempSPX, tempSPY;
    SIGNED snX, snY;
    SIGNED roundValue = 0x00008000;

    snX = nodeX << 16;
    snY = nodeY << 16;

    tempSPX = Fix_mul(snX,qPxx) + Fix_mul(snY,qPxy);
    tempSPY = Fix_mul(snY,qPyy) - Fix_mul(snX,qPyx);

    if( tempSPX >= 0 )
    {
        /* round up */
        scaledPoint->X = (INT32) ((tempSPX + roundValue) >> 16);
    }
    else
    {
        /* round down */
        scaledPoint->X = (INT32) ((tempSPX - roundValue) >> 16);
    }

    if( tempSPY >= 0 )
    {
        /* round up */
        scaledPoint->Y = (INT32) ((tempSPY + roundValue) >> 16);
    }
    else
    {
        /* round down */
        scaledPoint->Y = (INT32) ((tempSPY - roundValue) >> 16);
    }
}

/* floating-point math */
#else 
VOID ScalePath (INT32 nodeX, INT32 nodeY, point *scaledPoint)
{
    float tempSPX, tempSPY;
    float snX, snY;
    float roundValue = 0.5;

    snX = (float)nodeX;
    snY = (float)nodeY;
    tempSPX = (snX * qPxx + snY * qPxy);
    tempSPY = (snY * qPyy - snX * qPyx);

    if( tempSPX >= 0 )
    {
        /* round up */
        scaledPoint->X = (INT32) (tempSPX + roundValue);
    }
    else
    {
        /* round down */
        scaledPoint->X = (INT32) (tempSPX - roundValue);
    }

    if( tempSPY >= 0 )
    {
        /* round up */
        scaledPoint->Y = (INT32) (tempSPY + roundValue);
    }
    else
    {
        /* round down */
        scaledPoint->Y = (INT32)(tempSPY - roundValue);
    }

}
#endif

#endif      /* USE_STROKEDFONT */
