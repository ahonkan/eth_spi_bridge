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
**************************************************************************

**************************************************************************
*
* FILE NAME                                                         
*
*  bmapfont.c                                                   
*
* DESCRIPTION
*
*  Functions for drawing bitmapped fonts
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
* BMAPF_TextBlitHorizontal           
* BMAPF_TextAlignHorizontalCR        
*
* DEPENDENCIES
*
*  rs_base.h
*  rsfonts.h
*  bmapfont.h
*  globalrsv.h
*  portops.h
*  textd.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rsfonts.h"
#include "ui/bmapfont.h"
#include "ui/globalrsv.h"
#include "ui/portops.h"
#include "ui/textd.h"


/*****************************************************************************
* FUNCTION
*
*    BMAPF_TextBlitHorizontal
*
* DESCRIPTION
*
*  Draws characters from the specified text string, TEXTSTR, beginning 
*  at "INDEX" characters from the start of the string, and continuing 
*  for "COUNT" characters or until a null (x'00') character is 
*  encountered (which ever occurs first).
*
*  The text is drawn beginning at the current pen location.  After the text
*  is drawn, the pen is located to the start of the next character position
*  past the last character drawn (where a continuation of the string just
*  drawn would start).
*
*  Character Widths:
*
*  There are two widths involved in processing each character:
*
*    1) the character's "image width", and
*    2) the character's "spacing width".
*
*  The "image width" is the size of the actual character pixel width for
*  blitting from the font to the destination bitmap.  The image width is
*  defined by the by the image offset values contained in "locTbl" for
*  the font.  Image width is calculated as follows:
*  
*  imageWidth = (offset for the nextChar) - (the offset for the char);
*                -or-
*  imageWidth = locTbl[charNbr+1] - locTbl[charNbr];
*
*
*  2) The "spacing width" is the amount of horizontal space the character
*     occupies (which may or may not be the same as the cell width).  The
*     entries in the ofwdTbl are actually pen updating info, only having
*     relevance in determining the ending pen location.  Depending on whether
*     the Proportional flag is set, these values intersect in two different ways:
*  
*     fixed width:        Uses .chWidth for all characters
*  
*     proportional width: Uses ofwdTbl.wid for each character
* 
*     The starting location for the image blit is determined by adding the
*     character's signed "ofwdTbl.ofs" offset value to the starting pen
*     position. The ending position for the pen is then determined by
*     adding the character's "ofwdTbl.wid" width value to the starting pen
*     position.
*  
*     bgnBlitX = locX + ofwdTbl[aChar].ofs;
*     newLocX  = locX + ofwdTbl[aChar].wid;
*
*  Limitations:
*  =============
*  All font elements (except pixImage) will reside in the same segment (64k).
*  This implies a limitation of about 30,000 chars per font file
*  (If you assume 16 bytes per char, thats about 480k of font file)
* 
*  The max height/width is limited to 16k/32k pixels.
* 
*  The TextColor() call is not implemented.  Currently uses PenColor() vars
*  for blitter call.
*  
*  Notice that ofwdTbl is two bytes, this limits the offset to +/- 127,
*  and character spacing width to be 0-255. 
*
* INPUTS
*
*    VOID *TEXTSTR
*    INT32 INDEX
*    INT32 COUNT
*    INT32 CHARSIZE
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
INT32 BMAPF_TextBlitHorizontal(VOID *TEXTSTR, INT32 INDEX, INT32 COUNT, INT32 CHARSIZE)
{
    struct blitRec
    {     /* BlitList Rectangle Data Structure */

        /* src minimum X */
        INT32 sXmin;  

        /* src minimum Y */
        INT32 sYmin;  

        /* src maximum X */
        INT32 sXmax;  

        /* src maximum Y */
        INT32 sYmax;  

        /* dst minimum X */
        INT32 dXmin;  

        /* dst minimum Y */
        INT32 dYmin;  

        /* dst maximum X */
        INT32 dXmax;  

        /* dst maximum Y */
        INT32 dYmax;  
    }StrPtr[maxChars];

    CHAR JumpPathedFill = 0;

    UINT8  *textBytePtr;
    UINT16 *textWordPtr;
    UINT16 aCH, iCH; 
    INT32 srcYmax;
    INT32 dstYmin,dstYmax;
    INT32 bgnX, bgnY, dstX;
    INT32 width = 0;

    /* grafPort.txFont.OfWdTbl */
    INT32 gpOfWdTbl;     
    ofswid *ofWdPtr;

    /* grafPort.txFont.locTbl */
    INT32 gpLocTbl;      
    INT16 *gpLocPtr;

    /* grafPort.txFace */
    INT32 gpTxFace;      

    /* gafPort.txTerm  */
    INT32 gpTxTerm;      

    /* gafPort.txExtra */
    INT32 gpTxExtra;     

    /* gafPort.txMode  */
    INT32 gpTxMode;      

    /* grafPort.txPath */
    INT32 gpTxPath;     

    /* char to add txSpace for or txTerm */
    INT32 txSpaceSw;    
    INT32 imageWidth;
    INT16 char_width, char_ofset;
    INT32 icAdd;
    INT32 **rowTable;
    UINT8  *sfBitmap;
    UINT8  *svBitmap;
    UINT8  *wsBitmap;
    UINT8  *ws2Bitmap;
    
#ifdef ANTIALIASTEXT

    UINT8  *dwnBitmap;
    UINT8  *upBitmap;

#endif  /* ANTIALIASTEXT */          

    fontRcd *gpTxFont;
    grafMap *gpGrafMap;
    INT16 sfPixBytes; 
    INT32 sfByteCnt; 
    INT32 sfSrc2Next; 
    INT32 sfDst2Next;
    INT32 sfShiftBytes;
    INT32 sfCumShift;
    INT16 sfShiftInc;
    INT16 MaskLeft, MaskRight;
    INT16 ErrValue;
    rect *dstPtr;
    INT32 charCount=0;
    INT32 i;
    INT32 j;
    INT32 k;
    INT32 tempData;
    INT32 penSize;
    INT16 oldPen;
    UINT8 boldData;
    
#ifdef  ANTIALIASTEXT

    INT32 saveBlitFore;
    INT32 antiAliasColor;
    INT16 saveBlitRop;

    UINT16 aliasRed;
    UINT16 aliasGreen;
    UINT16 aliasBlue;
    palData aliasPal[256];
    palData aliasColor;

#endif  /* ANTIALIASTEXT */

    /* Tables  */
    INT16 deltatbl[19]={0, 21, 46, 69, 92, 120, 149, 179, 215, 256,
                        304,364, 443, 548, 704, 960, 1452, 2926, 0};

    UINT8 LfMask[8]={0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};

    UINT8 RtMask[8]={0xFF, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};    

     NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    textBytePtr = (UINT8 *) TEXTSTR + INDEX ;
    textWordPtr = (UINT16 *) TEXTSTR + INDEX;

    gpTxFont = (fontRcd *) theGrafPort.txFont;

    dstX = bgnX = LocX;
    bgnY = LocY;
    
    srcYmax = gpTxFont->chHeight;
    dstYmin = LocY - fntVertAln;
    dstYmax = dstYmin + gpTxFont->chHeight;

    gpLocTbl  = (SIGNED) gpTxFont + gpTxFont->locTbl;
    gpOfWdTbl = (SIGNED) gpTxFont + gpTxFont->ofwdTbl;

    gpTxFace  = theGrafPort.txFace;
    gpTxExtra = theGrafPort.txExtra;

    /* add extra interchar space for bold synth facing */   
    if( gpTxFace & faceBold )
    {
        gpTxExtra = gpTxExtra + theGrafPort.txBold;
    }

    gpTxPath  = theGrafPort.txPath;
    gpTxTerm  = theGrafPort.txTerm;

    /* add extra interspace for spacing */
    if( theGrafPort.txSpace != 0 )
    {
        txSpaceSw = ' ';
    }
    else
    {
        txSpaceSw = theGrafPort.txTerm;
    }

    /* Set the max size for Count */
    if( COUNT > maxChars )
    {
        COUNT = maxChars;
    }
    
    COUNT--;

    if( COUNT >= 0 )
    {
        /* Loop to process each character */
        do
        {
            if( CHARSIZE != 0 )
            {
                aCH = (UINT16) *textWordPtr++;
            }
            else
            {
                aCH = (UINT8 ) *textBytePtr++;
            }

            if( aCH == (UINT16)gpTxTerm )
            {
                break;    
            }

            if( (aCH < (UINT16)gpTxFont->fontMin) || (aCH > (UINT16)gpTxFont->fontMax) )
            {
                aCH = gpTxFont->chBad;
                iCH = (gpTxFont->chBad - gpTxFont->fontMin);
                ofWdPtr = ( (ofswid *) ( gpOfWdTbl + (iCH << 1) ) );
            }
            else
            {
                iCH = (aCH - gpTxFont->fontMin); 
                ofWdPtr = ( (ofswid *) ( gpOfWdTbl + (iCH << 1) ) );

                if( (ofWdPtr->wid == 0xFF) && (ofWdPtr->ofs == -1) )
                {
                    aCH = gpTxFont->chBad;
                    iCH = (gpTxFont->chBad - gpTxFont->fontMin);
                    ofWdPtr = ( (ofswid *) (gpOfWdTbl + (iCH << 1) ) );
                }
            }

            /* Ready to process character  */
            gpLocPtr = ((INT16 *) gpLocTbl) + iCH;
            StrPtr[charCount].sXmin = *gpLocPtr++; 
            StrPtr[charCount].sXmax = *gpLocPtr++; 

            char_width = gpTxFont->chWidth;

            /* Get image offset and width  */
            if( gpTxFace & faceProportional)
            {
                char_width = ofWdPtr->wid;
            }
            char_ofset = ofWdPtr->ofs; 
            
            if (gpTxPath == 1800)
            {
                /* Save src X and compute dst X limits  */ 
                StrPtr[charCount].dXmin = dstX + char_ofset - char_width; 
            }
            else
            {
                /* Save src X and compute dst X limits  */ 
                StrPtr[charCount].dXmin = dstX + char_ofset; 
            }

            imageWidth = StrPtr[charCount].sXmax - StrPtr[charCount].sXmin;
            if( imageWidth > 0 )
            {
                StrPtr[charCount].dXmax = StrPtr[charCount].dXmin + imageWidth;
                StrPtr[charCount].sYmin = 0;
                StrPtr[charCount].sYmax = srcYmax;
                StrPtr[charCount].dYmin = dstYmin;
                StrPtr[charCount].dYmax = dstYmax;
                charCount++;
            }

            /* Compute the amount to add between characters  */
            icAdd = (INT16) gpTxExtra;
            
            if( aCH == (UINT16)txSpaceSw )
            {
                icAdd += theGrafPort.txSpace;
            }

            /* Advance in path direction  */
            if( gpTxPath >= 900 )
            {
                if( gpTxPath >= 2700 )
                {
                    /* Path Down  */
                    icAdd += (dstYmax - dstYmin);
                    dstYmin += icAdd;
                    dstYmax += icAdd;

                    /* update pen position */
                    bgnY += icAdd;
                }
                else
                {
                    if( gpTxPath >= 1800 )
                    {
                        /* Path Left */
                        icAdd += char_width;
                        dstX  -= icAdd;
                    }
                    else
                    {
                        /* Path Up */
                        icAdd += (dstYmax - dstYmin);
                        dstYmin -= icAdd;
                        dstYmax -= icAdd;
                        
                        /* update pen position */
                        bgnY -= icAdd;
                    }
                }
            }
            else
            {
                /* Path Right */
                icAdd += char_width;
                dstX  += icAdd;
            }
        /* END LOOP  */  
        }while( (COUNT--) > 0 ); 
       
        /* Reset the counter */
        COUNT = charCount;
        charCount = 0;

        /*************** Ready to blit characters **************/
        if( (theGrafPort.pnLevel >= 0) && (COUNT > 0) )
        {   
            grafBlit.blitCnt = (INT16)COUNT;
            gpTxMode = theGrafPort.txMode;
            
#ifndef INCLUDE_32_BIT

#ifdef  GLOBAL_ALPHA_SUPPORT
        
            if (text_trans)
            {
                grafBlit.blitRop = xAVGx;
            }
            else
                
#endif  /* GLOBAL_ALPHA_SUPPORT */
                
            {
                grafBlit.blitRop = theGrafPort.txMode;
            }
#else
            gpTxMode = theGrafPort.txMode;
            grafBlit.blitRop = theGrafPort.txMode;
#endif
            
            gpGrafMap = (grafMap *) ((SIGNED) gpTxFont + gpTxFont->grafMapTbl);

#ifdef ANTIALIASTEXT
            gpTxFace |= faceAntialias;
#endif

            if( gpTxFace & ~faceProportional )
            {
                /****** DoSynthFacing *****/
                /* The blitlist must be processed for each rectangle pair.
                   Each source rectangle must be placed into the workspace bitmap,
                   facing applied, and then blit from the workspace bitmap to the dest
                   bitmap */
                grafBlit.blitCnt = 1;
                grafBlit.blitSmap = &workGrafMap;
                sfPixBytes = gpGrafMap->pixBytes;

                rowTable = (INT32 **) gpGrafMap->mapTable[0];
                    
                svBitmap = (UINT8 *) *rowTable;                              
                    
                /* set Ys of source (same for all rects) */
                grafBlist.Ymin = StrPtr[0].sYmin;
                grafBlist.Ymax = StrPtr[0].sYmax;
                    
#ifdef ANTIALIASTEXT

                if( (gpTxFace & faceAntialias) && (grafBlit.blitRop < 0x10) )
                {
                    /* initialize the anti-alias variables */
                    saveBlitFore = grafBlit.blitFore;
                    saveBlitRop = grafBlit.blitRop;

                    if( (theGrafPort.portMap->pixBits == 16) 
                        &&
                        (theGrafPort.portMap->mapFlags & mf565) )
                    {
                        /* use the actual color */
                        aliasRed = (UINT16) ( ( 
                            ( (grafBlit.blitBack >> 11) & 0x1f )
                            +
                            ( (grafBlit.blitBack >> 10) & 0x3e )
                            +
                            ( (grafBlit.blitFore >> 11) & 0x1f ) ) >> 2);

                        aliasGreen = (UINT16) ( (
                            ( (grafBlit.blitBack >> 5) & 0x003e )
                            +
                            ( (grafBlit.blitBack >> 4) & 0x007c )
                            +
                            ( (grafBlit.blitFore >> 5) & 0x003e ) ) >> 2);

                        aliasBlue = (UINT16) ( 
                            ( (grafBlit.blitBack & 0x001f)
                            +
                            ( (grafBlit.blitBack << 1) & 0x3e)
                            +
                            (  grafBlit.blitFore & 0x001f) ) >> 2);

                        antiAliasColor = (
                               (SIGNED) aliasRed << 11 )
                            + ((SIGNED) aliasGreen << 5)
                            + ((SIGNED) aliasBlue );
                    }
                    else if( theGrafPort.portMap->pixBits == 16 )
                    {
                        /* use the actual color */
                        aliasRed = (UINT16) ( ( 
                            ( (grafBlit.blitBack >> 10) & 0x1f )
                            +
                            ( (grafBlit.blitBack >> 9)  & 0x3e )
                            +
                            ( (grafBlit.blitFore >> 10) & 0x1f ) ) >> 2);

                        aliasGreen = (UINT16) ( ( 
                            ( (grafBlit.blitBack >> 5) & 0x001f )
                            +
                            ( (grafBlit.blitBack >> 4) & 0x003e )
                            +
                            ( (grafBlit.blitFore >> 5) & 0x001f ) ) >> 2);

                        aliasBlue = (UINT16) (
                            ( (grafBlit.blitBack & 0x001f)
                            +
                            ( (grafBlit.blitBack << 1) & 0x3e)
                            +
                            (  grafBlit.blitFore & 0x001f) ) >> 2);

                        antiAliasColor = (
                               (SIGNED) aliasRed << 10 )
                            + ((SIGNED) aliasGreen << 5)
                            + ((SIGNED) aliasBlue );
                    }
                    else if( theGrafPort.portMap->pixBits == 24 )
                    {   
                        /* use the actual color */
                        aliasRed = (UINT16) ( ( 
                            ( (grafBlit.blitBack >> 16) & 0x00ff )
                            +
                            ( (grafBlit.blitBack >> 15) & 0x01fe )
                            +
                            ( (grafBlit.blitFore >> 16) & 0x00ff ) ) >> 1);
     
                        aliasGreen = (UINT16) ( ( 
                            ( (grafBlit.blitBack >> 8 ) & 0x00ff )
                            +
                            ( (grafBlit.blitBack >> 7 ) & 0x01fe )
                            +
                            ( (grafBlit.blitFore >> 8 ) & 0x00ff ) ) >> 1);

                        aliasBlue = (UINT16) ( (
                            (grafBlit.blitBack & 0x00ff)
                            +
                            ((grafBlit.blitBack << 1) & 0x01fe)
                            +
                            (grafBlit.blitFore & 0x00ff) ) >> 2);

                        antiAliasColor = (
                               (SIGNED) aliasRed << 16)
                            + ((SIGNED) aliasGreen << 8)
                            + ((SIGNED) aliasBlue);
                    }
                    else
                    {   
                        /* palettized */
                        antiAliasColor = QueryColors() + 1;
                        ReadPalette(0, 0, antiAliasColor, &aliasPal[0]);

                        aliasColor.palRed = (UINT16) ( ( (
                            ( (aliasPal[grafBlit.blitBack & 0x00ff].palRed >> 8) & 0x00ff)
                            +
                            ( (aliasPal[grafBlit.blitBack & 0x00ff].palRed >> 7) & 0x01fe)
                            +
                            ( (aliasPal[grafBlit.blitFore & 0x00ff].palRed >> 8))) & 0x00ff) << 7);

                        aliasColor.palGreen = (UINT16) ( ( (
                            ((aliasPal[grafBlit.blitBack & 0x00ff].palGreen >> 8) & 0x00ff)
                            +
                            ((aliasPal[grafBlit.blitBack & 0x00ff].palGreen >> 7) & 0x01fe)
                            +
                            ((aliasPal[grafBlit.blitFore & 0x00ff].palGreen >> 8))) & 0x00ff) << 7);

                        aliasColor.palBlue = (UINT16) ( ( (
                            ((aliasPal[grafBlit.blitBack & 0x00ff].palBlue >> 8) & 0x00ff)
                            +
                            ((aliasPal[grafBlit.blitBack & 0x00ff].palBlue >> 7) & 0x01fe)
                            +
                            ((aliasPal[grafBlit.blitFore & 0x00ff].palBlue >> 8))) & 0x00ff) << 7);

                        antiAliasColor = FindClosestRGB(&aliasColor, &aliasPal[0]);
                    }
                }

#endif  /* ANTIALIASTEXT */          

                /* for each rectangle in the list move the font pixels to
                   the workspace*/
                do
                {
                    sfByteCnt = -(StrPtr[charCount].sXmin >> 3) ;
                    sfBitmap = svBitmap - sfByteCnt;
                    
                    /* Round sXmax up to next byte */
                    sfByteCnt += ((StrPtr[charCount].sXmax + 7) >> 3);
                
                    /* Compute deltas to next raster  */
                    sfDst2Next = workGrafMap.pixBytes - sfByteCnt ;
                    
                    if( sfDst2Next < 0 )
                    {   
                        /* rect too big for workspace buffer  */
                        ErrValue = c_DrawText + c_OutofMem ;

                         /* report error */
                        nuGrafErr(ErrValue, __LINE__, __FILE__); 
                        continue;
                    }
        
                    sfSrc2Next = sfPixBytes - sfByteCnt;
                    wsBitmap = mpWorkSpace;
                    for( i = 0; i < StrPtr[charCount].sYmax; i++)
                    {
                        for( j = 0; j < sfByteCnt; j++)
                        {
                            *wsBitmap++ = *sfBitmap++;
                        }

                        sfBitmap += sfSrc2Next; 
                        wsBitmap += sfDst2Next;
                    }

                    /* set Xs of workspace rect  */
                    grafBlist.Xmin = StrPtr[charCount].sXmin & 7;
                    grafBlist.Xmax = (StrPtr[charCount].sXmax -
                                    StrPtr[charCount].sXmin) + grafBlist.Xmin;
                    
                    /* Apply the facing  */
                    if( gpTxFace & faceBold )
                    {   
                        /* or's the image over itself advancing in X by the
                           ports txBold boldness factor  */
                        sfShiftBytes = (theGrafPort.txBold >> 3) + sfByteCnt + 1;
                        if( sfShiftBytes > workGrafMap.pixBytes )
                        {
                            ErrValue = c_DrawText + c_OutofMem;

                             /* report error */
                            nuGrafErr(ErrValue, __LINE__, __FILE__); 
                        }
                        else
                        {
                            MaskLeft = LfMask[StrPtr[charCount].sXmin & 0x07];
                            MaskRight= RtMask[StrPtr[charCount].sXmax & 0x07];
                            wsBitmap = mpWorkSpace;
                            for( i = 0; i < StrPtr[charCount].sYmax; i++)
                            {
                                sfBitmap = wsBitmap;
                                ws2Bitmap = wsBitmap + sfByteCnt - 1;
                                *wsBitmap &= MaskLeft;
                                *ws2Bitmap++ &= MaskRight;
                                *ws2Bitmap++ = 0;
                                *ws2Bitmap = 0;
                                for( j = 0; j < theGrafPort.txBold; j++)
                                {
                                    tempData = 0;
                                    for( k = 0; k < sfShiftBytes; k++)
                                    {
                                        boldData = (*wsBitmap >> 1);
                                        if( tempData )
                                        {
                                            boldData |= 0x80;
                                        }

                                        tempData = *wsBitmap & 1;
                                        *wsBitmap |= boldData;  /* or in shifted data */
                                        wsBitmap++;
                                    }

                                    wsBitmap = sfBitmap;
                                }
                                wsBitmap = sfBitmap + workGrafMap.pixBytes;
                            }

                            StrPtr[charCount].dXmax += theGrafPort.txBold;
                            grafBlist.Xmax += theGrafPort.txBold;
                        }
                    }   /* End Bold Face  */

                    if( gpTxFace & faceHalftone )
                    {
                        /* sets bits to background using a 50% pattern  */
                        wsBitmap = mpWorkSpace;

                        /* clear half the bits */
                        MaskRight = 0x55;   
                        for( i = 0; i < StrPtr[charCount].sYmax; i++)
                        {
                            for( j = 0; j < sfByteCnt; j++)                         
                            {
                                *wsBitmap++ &= MaskRight;
                            }
                            wsBitmap += sfDst2Next;
                            MaskLeft = MaskRight & 0x01;
                            MaskRight = MaskRight >> 1;
                            if(MaskLeft == 1)
                            {
                                MaskRight |= 0x80;
                            }
                        }
                    }   /* END Halftone  */

                    sfShiftInc = (3600 - theGrafPort.txSlant) / 50;
                    if( (gpTxFace & faceItalic) && (sfShiftInc < 18) )
                    {   
                        /* only handle slant to right for now with 5 degree
                           increments */
                    
                        /* compute amount of shift per raster necessary (times 256) */
                        sfShiftInc = deltatbl[sfShiftInc];
                    
                        /* compute total amount of shift necessary in bytes  */
                        sfCumShift = (sfShiftInc * StrPtr[charCount].sYmax) >> 8;
                        StrPtr[charCount].dXmax += sfCumShift;
                        grafBlist.Xmax += sfCumShift;
                    
                        /* compute bytes of entire shifted rectangle  */
                        sfShiftBytes = ((sfCumShift + 7) >> 3) + sfByteCnt;
                    
                        /* check will fit in buffer  */
                        if( sfShiftBytes > workGrafMap.pixBytes )
                        {
                            ErrValue = c_DrawText + c_OutofMem;
                            nuGrafErr(ErrValue, __LINE__, __FILE__);  /* report error */
                        }
                        else
                        {   
                            /* Mask and store */        
                            sfCumShift = 0;
                            MaskLeft = LfMask[StrPtr[charCount].sXmin & 0x07];
                            MaskRight= RtMask[StrPtr[charCount].sXmax & 0x07];
                            for( i = (StrPtr[charCount].sYmax -1); i >= 0; i--)
                            {
                                wsBitmap = mpWorkSpace + (i * workGrafMap.pixBytes);
                                ws2Bitmap = wsBitmap + sfByteCnt - 1;
                                *wsBitmap &= MaskLeft;
                                *ws2Bitmap++ &= MaskRight;
                                sfBitmap = wsBitmap;
                                j = sfShiftBytes - sfByteCnt;
                                while( j-- > 0 )
                                {
                                    *ws2Bitmap++ = 0;
                                }

                                j = sfCumShift >> 8;
                                while( j-- > 0 )
                                {   
                                    /* shift raster right */
                                    tempData = 0;
                                    for( k = 0; k < sfShiftBytes; k++)
                                    {
                                        boldData = (*wsBitmap >> 1);
                                        if(tempData)
                                        {
                                            boldData |= 0x80;
                                        }
                                        tempData = *wsBitmap & 1;

                                        /* store shifted data */
                                        *wsBitmap = boldData;   
                                        wsBitmap++;
                                    }

                                    wsBitmap = sfBitmap;
                                }

                                /* Compute pixels to shift next raster  */
                                sfCumShift += sfShiftInc;
                            }
                        }
                    }   /* END Italic   */

                    /* Blit the rect from the work space to the destination  */
                    grafBlist.skipStat = StrPtr[charCount].dXmin;
                    grafBlist.temp1 = StrPtr[charCount].dYmin;
                    grafBlist.temp2 = StrPtr[charCount].dXmax;                  
                    grafBlist.temp3 = StrPtr[charCount].dYmax;              
                            
                    theGrafPort.portMap->prBlit1S(&grafBlit); 

#ifdef ANTIALIASTEXT

                    if( (gpTxFace & faceAntialias) && (grafBlit.blitRop < 0x10) )
                    {
                        /* or the image over itself 1 pixel up,down,left & right */
                        sfShiftBytes = sfByteCnt;
                        if( (sfShiftBytes << 1) > workGrafMap.pixBytes )
                        {
                            ErrValue = c_DrawText + c_OutofMem;

                            /* report error */
                            nuGrafErr(ErrValue, __LINE__, __FILE__); 
                        }
                        else
                        {
                            MaskLeft = LfMask[StrPtr[charCount].sXmin & 0x07];
                            MaskRight= RtMask[StrPtr[charCount].sXmax & 0x07];

                            /* do top line */
                            wsBitmap = mpWorkSpace;
                            sfBitmap = wsBitmap + sfShiftBytes + 2;
                            dwnBitmap = sfBitmap + workGrafMap.pixBytes;
                            upBitmap = sfBitmap - (workGrafMap.pixBytes + sfShiftBytes);
                            ws2Bitmap = wsBitmap + sfByteCnt - 1;
                            *wsBitmap &= MaskLeft;
                            *ws2Bitmap++ &= MaskRight;
                            *ws2Bitmap = 0;
                            tempData = 0;
                            for( j = 0; j < sfShiftBytes; j++)
                            {
                                boldData = *wsBitmap;

                                /* shift down */
                                *dwnBitmap++ = boldData;

                                /* shift left and right */
                                boldData |= ((boldData >> 1) | (boldData << 1));

                                /* check last byte for right shift */
                                if (tempData)
                                {
                                    boldData |= 0x80;
                                }
                                tempData = *wsBitmap++ & 0x01;

                                /* check next byte for left shift */
                                if (*wsBitmap & 0x80)
                                {
                                    boldData |= 0x01;
                                }
                                *sfBitmap++ = boldData;
                            }
                            /* do middle lines */
                            for( i = 2; i < StrPtr[charCount].sYmax; i++)
                            {
                                wsBitmap += sfDst2Next;
                                sfBitmap += sfDst2Next;
                                dwnBitmap += sfDst2Next;
                                upBitmap += sfDst2Next;
                                ws2Bitmap = wsBitmap + sfByteCnt - 1;
                                *wsBitmap &= MaskLeft;
                                *ws2Bitmap++ &= MaskRight;
                                *ws2Bitmap = 0;
                                tempData = 0;
                                for( j = 0; j < sfShiftBytes; j++)
                                {
                                    boldData = *wsBitmap;
                                    
                                    /* shift up */
                                    *upBitmap++ |= boldData;
                                    
                                    /* shift down */
                                    *dwnBitmap++ = boldData;
                                    
                                    /* shift left and right */
                                    boldData |= ((boldData >> 1) | (boldData << 1));
                                    
                                    /* check last byte for right shift */
                                    if(tempData)
                                    {
                                        boldData |= 0x80;
                                    }
                                    tempData = *wsBitmap++ & 0x01;
                                    
                                    /* check next byte for left shift */
                                    if( *wsBitmap & 0x80)
                                    {
                                        boldData |= 0x01;
                                    }
                                    *sfBitmap++ |= boldData;
                                }
                            }
                            /* do bottom line */
                            wsBitmap += sfDst2Next;
                            sfBitmap += sfDst2Next;
                            upBitmap += sfDst2Next;
                            ws2Bitmap = wsBitmap + sfByteCnt - 1;
                            *wsBitmap &= MaskLeft;
                            *ws2Bitmap++ &= MaskRight;
                            *ws2Bitmap = 0;
                            tempData = 0;
                            for( j = 0; j < sfShiftBytes; j++)
                            {
                                boldData = *wsBitmap;

                                /* shift up */
                                *upBitmap++ |= boldData;
                                
                                /* shift left and right */
                                boldData |= ((boldData >> 1) | (boldData << 1));
                                
                                /* check last byte for right shift */
                                if( tempData)
                                {
                                    boldData |= 0x80;
                                }
                                tempData = *wsBitmap++ & 0x01;
                                
                                /* check next byte for left shift */
                                if (*wsBitmap & 0x80)
                                {
                                    boldData |= 0x01;
                                }
                                *sfBitmap++ |= boldData;
                            }
                            /* xor onto original */
                            wsBitmap = mpWorkSpace;
                            sfBitmap = wsBitmap + sfShiftBytes + 2;
                            for( i = 0; i < StrPtr[charCount].sYmax; i++)
                            {
                                for( j = 0; j < sfShiftBytes; j++)
                                {
                                    *wsBitmap++ ^= *sfBitmap++;
                                }
                                wsBitmap += sfDst2Next;
                                sfBitmap += sfDst2Next;
                            }

                            /* set up blit */
                            grafBlit.blitFore = antiAliasColor;
                            grafBlit.blitRop = xREPx;
                            theGrafPort.portMap->prBlit1S(&grafBlit);

                            /* restore raster-op and color */
                            grafBlit.blitFore = saveBlitFore;
                            grafBlit.blitRop = saveBlitRop;
                        }
                    }
                    
#endif  /* ANTIALIASTEXT */          

                } while (++charCount < COUNT);
            }
            else
            {
                /* no special processing */
                grafBlit.blitList = (SIGNED) &StrPtr[0]; 
                grafBlit.blitSmap = gpGrafMap;  
                theGrafPort.portMap->prBlit1S(&grafBlit);
            }

            /* Process intercharacter fills  */
            if( !(gpTxMode & 0x10) )
            {
                dstPtr = (rect *) &StrPtr[0];
                charCount = 0;
                if( gpTxPath != pathRight )
                {
                    JumpPathedFill = 1;
                }
                    
                if( !JumpPathedFill )
                {
                    if( bgnX < StrPtr[0].dXmin )
                    {
                        dstPtr->Xmax = StrPtr[0].dXmin;
                        dstPtr->Xmin = bgnX;
                        dstPtr->Ymin = dstYmin;
                        dstPtr->Ymax = dstYmax;
                        dstPtr++;
                    }

                    while( ++charCount < COUNT)
                    {
                        tempData = StrPtr[charCount - 1].dXmax;
                        if( tempData < StrPtr[charCount].dXmin )
                        {
                            dstPtr->Xmax = StrPtr[charCount].dXmin;
                            dstPtr->Xmin = tempData;
                            dstPtr->Ymin = dstYmin;
                            dstPtr->Ymax = dstYmax;
                            dstPtr++;
                        }
                    }

                    dstPtr->Xmin = StrPtr[charCount - 1].dXmax;
                    dstPtr->Xmax = dstX;
                    dstPtr->Ymin = dstYmin;
                    dstPtr->Ymax = dstYmax;
                    dstPtr++;

                    grafBlit.blitCnt = (INT16)(((INT32) dstPtr - (INT32) &StrPtr[0]) - 16); 
                    if( grafBlit.blitCnt > 0 )
                    {
                        grafBlit.blitList = (SIGNED) &StrPtr[0]; 
                        grafBlit.blitCnt  = grafBlit.blitCnt >> 4;
                        grafBlit.blitPat  = theGrafPort.bkPat;
                        if(set_back_color == 1)
                        {
                            theGrafPort.portMap->prFill(&grafBlit);
                        }
                    }
                } 

            } /* End fills */

            if( !JumpPathedFill && (gpTxFace & faceUnderline) )
            {
                /* port txUnder controls depth from baseline; port txScore
                   controls distance around descenders */
                tempData = fntVertTbl[alignBaseline * 2] + 1 + theGrafPort.txUnder;
                penSize = theGrafPort.pnSize.Y;
                if( penSize == 0)
                {
                    /* must have pen => 1 */
                    penSize++; 
                }

                grafBlist.Ymin = tempData + dstYmin - (penSize >> 1);
                grafBlist.Ymax = grafBlist.Ymin + penSize;
                grafBlist.Xmin = LocX;
                grafBlist.Xmax = LocX;
                grafBlit.blitCnt = 1;
                grafBlit.blitPat = 1;
                grafBlit.blitList = (SIGNED) &grafBlist;

                /* advance along X looking for transitions in pen color */
                /* set to unknown state */
                oldPen = -1;    
                do
                {
                    if( theGrafPort.portMap->prGetPx(grafBlist.Xmax, grafBlist.Ymin,
                        theGrafPort.portMap) != theGrafPort.pnColor )
                    {
                        /* not in pen */
                        if( oldPen == -1 )
                        {
                            /* unknown state - first time through */
                            oldPen = 1;
                        }
                        else
                        {
                            if( oldPen == 0)
                            {
                                /* had a transition */
                                oldPen++;
                                grafBlist.Xmin = grafBlist.Xmax + theGrafPort.txScore;
                            }
                        }
                    }
                    else
                    {
                        /* in pen */
                        if( oldPen == -1 )
                        {
                            /* unknown state - first time through */
                            oldPen = 0;
                        }
                        else
                        {
                            if( oldPen != 0 )
                            {
                                /* had a transition */
                                oldPen = 0;
                                tempData = grafBlist.Xmax - theGrafPort.txScore;
                                if( tempData > grafBlist.Xmin )
                                {
                                    grafBlist.Xmax = tempData;
                                    theGrafPort.portMap->prFill(&grafBlit);  
                                    grafBlist.Xmax += theGrafPort.txScore;
                                    grafBlist.Xmin = grafBlist.Xmax;
                                }   
                            }
                        }
                    }
                    grafBlist.Xmax++;
                }while (grafBlist.Xmax < dstX );
                        
                if( grafBlist.Xmax > grafBlist.Xmin)
                {
                    theGrafPort.portMap->prFill(&grafBlit);
                }
            } /* End Underline  */

            if( !JumpPathedFill && (gpTxFace & faceStrikeout) )
            {
                /* strikeout line halfway between char rect */
                penSize = theGrafPort.pnSize.Y;
                if( penSize == 0 )
                {
                    /* must have pen => 1 */
                    penSize++; 
                }

                srcYmax = ((srcYmax + 1) - penSize) >> 1;

                /* setup rect */
                grafBlist.Ymin = dstYmin + srcYmax; 
                grafBlist.Ymax = grafBlist.Ymin + penSize;
                grafBlist.Xmin = LocX;
                grafBlist.Xmax = dstX;
                grafBlit.blitCnt = 1;
                grafBlit.blitPat = 1;

                grafBlit.blitList = (SIGNED) &grafBlist;
                theGrafPort.portMap->prFill(&grafBlit); 
            }   /* End Strikeout */
                        
            grafBlit.blitCnt  = 1;
            grafBlit.blitRop  = theGrafPort.pnMode;
            grafBlit.blitPat  = theGrafPort.pnPat;
            grafBlit.blitList = (SIGNED) &grafBlist;  
            grafBlit.blitSmap = theGrafPort.portMap;
        }   
        
        width = dstX - LocX;

        LocX = dstX;
        LocY = bgnY;
                
        if( globalLevel > 0)
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

    } 

    /* Return to user mode */
    NU_USER_MODE();
    return (width);
}



/*****************************************************************************
* FUNCTION
*
*    BMAPF_TextAlignHorizontalCR
*
* DESCRIPTION
*
*  Handles centered and right alignments for horizontal text drawing calls.
*
* INPUTS
*
*    VOID *TEXTSTR
*    INT32 INDEX
*    INT32 COUNT
*    INT32 CHARSIZE
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
INT32 BMAPF_TextAlignHorizontalCR(signed char *TEXTSTR, INT32 INDEX, INT32 COUNT, INT32 CHARSIZE)
{
    INT32 oldLocX;
    INT32 oldLocY;
    INT32 Width;
    INT32 Height;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    oldLocX = LocX;
    oldLocY = LocY;

    /* Hide the pen */
    theGrafPort.pnLevel-- ;  

    /* Call drawing procedure to move pen position to end of string. */
    txtDrwIDV(TEXTSTR, INDEX, COUNT, CHARSIZE );
    
    /* Restore the pen */
    theGrafPort.pnLevel++ ; 
    
    Width  = LocX - oldLocX;
    Height = LocY - oldLocY;

    /* Adjust bgnX base on alignment   */
    if( theGrafPort.txAlign.X >= 1 )
    {
        if(theGrafPort.txAlign.X == 1)
        {
            Width  = Width  >> 1;
            Height = Height >> 1;
        }

        /* If a string is being drawn character by character */
        if(strAsChar == NU_TRUE)
        {
            LocX = oldLocX;
            LocY = oldLocY;
        }
        /* String is being drawn all at once */
        else
        {
            LocX = oldLocX - Width;
            LocY = oldLocY - Height;
        }
    }

    /* Draw the aligned text */
    txtDrwIDV(TEXTSTR, INDEX, COUNT, CHARSIZE );

    /* Return to user mode */
    NU_USER_MODE();
    return (Width);
}


