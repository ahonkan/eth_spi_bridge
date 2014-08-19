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
*  colorops.c                                                   
*
* DESCRIPTION
*
*  Color and palette functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  FindClosestRGB
*  ColorDiffusionDither
*  QueryRes
*  QueryColors
*  ReadPalette
*
* DEPENDENCIES
*
*  rs_base.h
*  devc.h
*  colorops.h
*  globalrsv.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/devc.h"
#include "ui/colorops.h"
#include "ui/globalrsv.h"

/***************************************************************************
* FUNCTION
*
*    FindClosestRGB
*
* DESCRIPTION
*
*    Function FindClosestRGB returns the palette index of the color closest
*    to the specified target RGB value.  RGBCOLOR is an RGB value stored
*    in a single "palData" palette record.
*
*    FindClosestRGB uses the Pythagorean distance formula to locate the 
*    index in the RGBPALETTE array closest matching the RGBCOLOR value.
*
* INPUTS
*
*    palData *RGBCOLOR - Pointer to RGBCOLOR.
*
*    palData *RGBPALETTE - Pointer to RGBPALETTE.
*
* OUTPUTS
*
*    INT32 minDistIndex - Returns closest index.
*
***************************************************************************/
INT32 FindClosestRGB(palData *RGBCOLOR, palData *RGBPALETTE)
{
    /* closest distance, squared */
    UINT32  minDistSq;           
    UINT32  tDistSq;

    /* closest index */
    INT32 minDistIndex;        

    /* max color index */
    INT32 maxColor;            

    /* 8 bit RGB target values */
    UINT16 red, green, blue;   
    INT32 i, j;


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* scale RGBCOLOR components down to 8 bits */
    red   = RGBCOLOR->palRed   >> 8;
    green = RGBCOLOR->palGreen >> 8;
    blue  = RGBCOLOR->palBlue  >> 8;

    maxColor = QueryColors();
    minDistSq = 0xfffffff;
    minDistIndex = 0;

    /* search for closest value in RGBPALETTE array */
    for( i = 0; i <= maxColor; i++)
    {
        j = (RGBPALETTE[i].palRed   >> 8) - red;
        tDistSq = j * j;
        j = (RGBPALETTE[i].palGreen >> 8) - green;
        tDistSq += (j * j);
        j = (RGBPALETTE[i].palBlue  >> 8) - blue;

        tDistSq += (j * j);

        if (tDistSq < minDistSq)
        {
            /* save the index and smallest distance (squared) */
            minDistSq = tDistSq;
            minDistIndex = i;
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(minDistIndex);
}


/***************************************************************************
* FUNCTION
*
*    ColorDiffusionDither
*
* DESCRIPTION
*
*    Function ColorDiffusionDither provides for color smoothing among pixels.
*
* INPUTS
*
*    image *imgIn    - Pointer to imgIn.
*
*    palData *imgPal - Pointer to imgPal. 
*
*    image *imgOut   - Pointer to imgOut.
*
*    palData *palPtr - Pointer to palPtr.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID ColorDiffusionDither(image *imgIn, palData *imgPal, image *imgOut, palData *palPtr)
{
    UINT8 *imgInDataPtr, *imgOutDataPtr;
    palData imgData;
    INT32 irow, icol, npal, iwidth, iheight, imgInData;
    SIGNED colorErr, tempImgRed, tempImgGreen, tempImgBlue;
    SIGNED rerr1[MAX_DEFWIN_X], rerr2[MAX_DEFWIN_X];
    SIGNED gerr1[MAX_DEFWIN_X], gerr2[MAX_DEFWIN_X];
    SIGNED berr1[MAX_DEFWIN_X], berr2[MAX_DEFWIN_X];


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* copy image header */
    /* Image pixel width (X) */
    imgOut->imWidth  = imgIn->imWidth;  

    /* Image pixel height (Y) */
    imgOut->imHeight = imgIn->imHeight; 

    /* Image alignment */
    imgOut->imAlign  = imgIn->imAlign;  

    /* Image flags */
    imgOut->imFlags  = imgIn->imFlags;  

    /* Image bytes per row */
    imgOut->imBytes  = imgIn->imBytes;  

    /* Image bits per pixel */
    imgOut->imBits   = imgIn->imBits;   

    /* Image planes per pixel */
    imgOut->imPlanes = imgIn->imPlanes; 

    /* get image values */
    imgInDataPtr  = imgIn->imData;
    imgOutDataPtr = imgOut->imData;

    iwidth  = imgIn->imWidth - 1;
    iheight = imgIn->imHeight - 1;

    /* get the data and convert it */
    /* get first pixel */
    imgInData = *imgInDataPtr++;
    imgData.palRed   = (imgPal[imgInData].palRed);
    imgData.palGreen = (imgPal[imgInData].palGreen);
    imgData.palBlue  = (imgPal[imgInData].palBlue);

    npal = FindClosestRGB(&imgData, palPtr);

    /* first pixel has no correction */
    *imgOutDataPtr++ = npal;    

    /* calculate corrections for surrounding points */
    colorErr = ( (imgData.palRed - palPtr[npal].palRed) >> 4 );
    rerr1[0] = 0;
    rerr2[1] = colorErr;
    rerr1[1] = (7 * colorErr);
    rerr2[0] = (5 * colorErr);

    colorErr = ((imgData.palGreen - palPtr[npal].palGreen) >> 4);
    gerr1[0] = 0;
    gerr2[1] = colorErr;
    gerr1[1] = (7 * colorErr);
    gerr2[0] = (5 * colorErr);

    colorErr = ((imgData.palBlue - palPtr[npal].palBlue) >> 4);
    berr1[0] = 0;
    berr2[1] = colorErr;
    berr1[1] = (7 * colorErr);
    berr2[0] = (5 * colorErr);

    /* do rest of first row */
    for( icol = 1; icol < iwidth; icol++)
    {
        imgInData = *imgInDataPtr++;
        tempImgRed = (imgPal[imgInData].palRed) + rerr1[icol];
        if( tempImgRed < 0 )
        {
            imgData.palRed = 0;
        }
        else
        {
            if( tempImgRed > 0xff00 )
            {
                imgData.palRed = 0xff00;
            }
            else
            {
                imgData.palRed = (UINT16 ) tempImgRed;
            }
        }

        tempImgGreen = (imgPal[imgInData].palGreen) + gerr1[icol];
        if( tempImgGreen < 0 )
        {
            imgData.palGreen = 0;
        }
        else
        {
            if( tempImgGreen > 0xff00 )
            {
                imgData.palGreen = 0xff00;
            }
            else
            {
                imgData.palGreen = (UINT16 ) tempImgGreen;
            }
        }

        tempImgBlue = (imgPal[imgInData].palBlue) + berr1[icol];
        if( tempImgBlue < 0 )
        {
            imgData.palBlue = 0;
        }
        else
        {
            if( tempImgBlue > 0xff00 )
            {
                imgData.palBlue = 0xff00;
            }
            else
            {
                imgData.palBlue = (UINT16 ) tempImgBlue;
            }
        }

        npal = FindClosestRGB(&imgData, palPtr);

        /* store corrected value */
        *imgOutDataPtr++ = npal;

        /* calculate corrections for surrounding points */
        colorErr = ((imgPal[imgInData].palRed - palPtr[npal].palRed) >> 4);
        rerr2[icol+1] = colorErr;
        rerr1[icol+1] = (7 * colorErr);
        rerr2[icol-1] += (3 * colorErr);
        rerr2[icol] += (5 * colorErr);

        colorErr = ((imgPal[imgInData].palGreen - palPtr[npal].palGreen) >> 4);
        gerr2[icol+1] = colorErr;
        gerr1[icol+1] = (7 * colorErr);
        gerr2[icol-1] += (3 * colorErr);
        gerr2[icol] += (5 * colorErr);

        colorErr = ((imgPal[imgInData].palBlue - palPtr[npal].palBlue) >> 4);
        berr2[icol+1] = colorErr;
        berr1[icol+1] = (7 * colorErr);
        berr2[icol-1] += (3 * colorErr);
        berr2[icol] += (5 * colorErr);
    }

    /* do last pixel */
    imgInData = *imgInDataPtr++;
    tempImgRed = (imgPal[imgInData].palRed) + rerr1[icol];
    if( tempImgRed < 0 )
    {
        imgData.palRed = 0;
    }
    else
    {
        if( tempImgRed > 0xff00 )
        {
            imgData.palRed = 0xff00;
        }
        else
        {
            imgData.palRed = (UINT16 ) tempImgRed;
        }
    }

    tempImgGreen = (imgPal[imgInData].palGreen) + gerr1[icol];
    if( tempImgGreen < 0 )
    {
        imgData.palGreen = 0;
    }
    else
    {
        if( tempImgGreen > 0xff00 )
        {
            imgData.palGreen = 0xff00;
        }
        else
        {
            imgData.palGreen = (UINT16 ) tempImgGreen;
        }
    }

    tempImgBlue = (imgPal[imgInData].palBlue) + berr1[icol];
    if( tempImgBlue < 0 )
    {
        imgData.palBlue = 0;
    }
    else
    {
        if( tempImgBlue > 0xff00 )
        {
            imgData.palBlue = 0xff00;
        }
        else
        {
            imgData.palBlue = (UINT16 ) tempImgBlue;
        }
    }

    npal = FindClosestRGB(&imgData, palPtr);

    /* store corrected value */
    *imgOutDataPtr++ = npal;
    
    /* calculate corrections for surrounding points */
    colorErr = ((imgPal[imgInData].palRed - palPtr[npal].palRed) >> 4);
    rerr2[icol-1] += (3 * colorErr);
    rerr2[icol] += (5 * colorErr);

    colorErr = ((imgPal[imgInData].palGreen - palPtr[npal].palGreen) >> 4);
    gerr2[icol-1] += (3 * colorErr);
    gerr2[icol] += (5 * colorErr);

    colorErr = ((imgPal[imgInData].palBlue - palPtr[npal].palBlue) >> 4);
    berr2[icol-1] += (3 * colorErr);
    berr2[icol] += (5 * colorErr);

    /* do remaining rows */
    for( irow = 1; irow < iheight; irow++)
    {
        /* get first pixel */
        imgInData = *imgInDataPtr++;

        tempImgRed = (imgPal[imgInData].palRed) + rerr2[icol];
        if( tempImgRed < 0 )
        {
            imgData.palRed = 0;
        }
        else
        {
            if( tempImgRed > 0xff00 )
            {
                imgData.palRed = 0xff00;
            }
            else
            {
                imgData.palRed = (UINT16 ) tempImgRed;
            }
        }

        tempImgGreen = (imgPal[imgInData].palGreen) + gerr2[icol];
        if( tempImgGreen < 0 )
        {
            imgData.palGreen = 0;
        }
        else
        {
            if( tempImgGreen > 0xff00 )
            {
                imgData.palGreen = 0xff00;
            }
            else
            {
                imgData.palGreen = (UINT16 ) tempImgGreen;
            }
        }

        tempImgBlue = (imgPal[imgInData].palBlue) + berr2[icol];
        if( tempImgBlue < 0 )
        {
            imgData.palBlue = 0;
        }
        else
        {
            if( tempImgBlue > 0xff00 )
            {
                imgData.palBlue = 0xff00;
            }
            else
            {
                imgData.palBlue = (UINT16 ) tempImgBlue;
            }
        }

        npal = FindClosestRGB(&imgData, palPtr);

        /* store corrected value */
        *imgOutDataPtr++ = npal;

        /* calculate corrections for surrounding points */
        colorErr = ((imgPal[imgInData].palRed - palPtr[npal].palRed) >> 4);
        rerr1[1] = rerr2[1] + (7 * colorErr);
        rerr2[1] = colorErr;
        rerr2[0] = (5 * colorErr);

        colorErr = ((imgPal[imgInData].palGreen - palPtr[npal].palGreen) >> 4);
        gerr1[1] = gerr2[1] + (7 * colorErr);
        gerr2[1] = colorErr;
        gerr2[0] = (5 * colorErr);

        colorErr = ((imgPal[imgInData].palBlue - palPtr[npal].palBlue) >> 4);
        berr1[1] = berr2[1] + (7 * colorErr);
        berr2[1] = colorErr;
        berr2[0] = (5 * colorErr);

        /* do rest of row */
        for( icol = 1; icol < iwidth; icol++)
        {
            imgInData = *imgInDataPtr++;

            tempImgRed = (imgPal[imgInData].palRed) + rerr1[icol];
            if( tempImgRed < 0 )
            {
                imgData.palRed = 0;
            }
            else
            {
                if( tempImgRed > 0xff00 )
                {
                    imgData.palRed = 0xff00;
                }
                else
                {
                    imgData.palRed = (UINT16 ) tempImgRed;
                }
            }

            tempImgGreen = (imgPal[imgInData].palGreen) + gerr1[icol];
            if( tempImgGreen < 0 )
            {
                imgData.palGreen = 0;
            }
            else
            {
                if( tempImgGreen > 0xff00 )
                {
                    imgData.palGreen = 0xff00;
                }
                else
                {
                    imgData.palGreen = (UINT16 ) tempImgGreen;
                }
            }

            tempImgBlue = (imgPal[imgInData].palBlue) + berr1[icol];
            if( tempImgBlue < 0 )
            {
                imgData.palBlue = 0;
            }
            else
            {
                if( tempImgBlue > 0xff00 )
                {
                    imgData.palBlue = 0xff00;
                }
                else
                {
                    imgData.palBlue = (UINT16 ) tempImgBlue;
                }
            }

            npal = FindClosestRGB(&imgData, palPtr);

            /* store corrected value */
            *imgOutDataPtr++ = npal;

            /* calculate corrections for surrounding points */
            colorErr = ((imgPal[imgInData].palRed - palPtr[npal].palRed) >> 4);
            rerr1[icol+1] = rerr2[icol+1] + (7 * colorErr);
            rerr2[icol+1] = colorErr;
            rerr2[icol-1] += (3 * colorErr);
            rerr2[icol] += (5 * colorErr);

            colorErr = ((imgPal[imgInData].palGreen - palPtr[npal].palGreen) >> 4);
            gerr1[icol+1] = gerr2[icol+1] + (7 * colorErr);
            gerr2[icol+1] = colorErr;
            gerr2[icol-1] += (3 * colorErr);
            gerr2[icol] += (5 * colorErr);

            colorErr = ((imgPal[imgInData].palBlue - palPtr[npal].palBlue) >> 4);
            berr1[icol+1] = berr2[icol+1] + (7 * colorErr);
            berr2[icol+1] = colorErr;
            berr2[icol-1] += (3 * colorErr);
            berr2[icol] += (5 * colorErr);
        }

        /* do last pixel */
        imgInData = *imgInDataPtr++;

        tempImgRed = (imgPal[imgInData].palRed) + rerr1[icol];
        if( tempImgRed < 0 )
        {
            imgData.palRed = 0;
        }
        else
        {
            if( tempImgRed > 0xff00 )
            {
                imgData.palRed = 0xff00;
            }
            else
            {
                imgData.palRed = (UINT16 ) tempImgRed;
            }
        }

        tempImgGreen = (imgPal[imgInData].palGreen) + gerr1[icol];
        if( tempImgGreen < 0 )
        {
            imgData.palGreen = 0;
        }
        else
        {
            if( tempImgGreen > 0xff00 )
            {
                imgData.palGreen = 0xff00;
            }
            else
            {
                imgData.palGreen = (UINT16 ) tempImgGreen;
            }
        }

        tempImgBlue = (imgPal[imgInData].palBlue) + berr1[icol];
        if( tempImgBlue < 0 )
        {
            imgData.palBlue = 0;
        }
        else
        {
            if( tempImgBlue > 0xff00 )
            {
                imgData.palBlue = 0xff00;
            }
            else
            {
                imgData.palBlue = (UINT16 ) tempImgBlue;
            }
        }

        npal = FindClosestRGB(&imgData, palPtr);

        /* store corrected value */
        *imgOutDataPtr++ = npal;

        /* calculate corrections for surrounding points */
        colorErr = ((imgPal[imgInData].palRed - palPtr[npal].palRed) >> 4);
        rerr2[icol-1] += (3 * colorErr);
        rerr2[icol] += (5 * colorErr);

        colorErr = ((imgPal[imgInData].palGreen - palPtr[npal].palGreen) >> 4);
        gerr2[icol-1] += (3 * colorErr);
        gerr2[icol] += (5 * colorErr);

        colorErr = ((imgPal[imgInData].palBlue - palPtr[npal].palBlue) >> 4);
        berr2[icol-1] += (3 * colorErr);
        berr2[icol] += (5 * colorErr);
    }

    /* do last row */
    /* get first pixel */
    imgInData = *imgInDataPtr++;

    tempImgRed = (imgPal[imgInData].palRed) + rerr2[icol];
    if( tempImgRed < 0 )
    {
        imgData.palRed = 0;
    }
    else
    {
        if( tempImgRed > 0xff00 )
        {
            imgData.palRed = 0xff00;
        }
        else
        {
            imgData.palRed = (UINT16 ) tempImgRed;
        }
    }

    tempImgGreen = (imgPal[imgInData].palGreen) + gerr2[icol];
    if( tempImgGreen < 0 )
    {
        imgData.palGreen = 0;
    }
    else
    {
        if( tempImgGreen > 0xff00 )
        {
            imgData.palGreen = 0xff00;
        }
        else
        {
            imgData.palGreen = (UINT16 ) tempImgGreen;
        }
    }

    tempImgBlue = (imgPal[imgInData].palBlue) + berr2[icol];
    if( tempImgBlue < 0 )
    {
        imgData.palBlue = 0;
    }
    else
    {
        if( tempImgBlue > 0xff00 )
        {
            imgData.palBlue = 0xff00;
        }
        else
        {
            imgData.palBlue = (UINT16 ) tempImgBlue;
        }
    }

    npal = FindClosestRGB(&imgData, palPtr);

    /* store corrected value */
    *imgOutDataPtr++ = npal;

    /* calculate corrections for surrounding points */
    colorErr = ((imgPal[imgInData].palRed - palPtr[npal].palRed) >> 4);
    rerr1[1] = rerr2[1] + (7 * colorErr);

    colorErr = ((imgPal[imgInData].palGreen - palPtr[npal].palGreen) >> 4);
    gerr1[1] = gerr2[1] + (7 * colorErr);

    colorErr = ((imgPal[imgInData].palBlue - palPtr[npal].palBlue) >> 4);
    berr1[1] = berr2[1] + (7 * colorErr);

    /* do rest of row */
    for( icol = 1; icol < iwidth; icol++)
    {
        imgInData = *imgInDataPtr++;

        tempImgRed = (imgPal[imgInData].palRed) + rerr1[icol];
        if( tempImgRed < 0 )
        {
            imgData.palRed = 0;
        }
        else
        {
            if( tempImgRed > 0xff00 )
            {
                imgData.palRed = 0xff00;
            }
            else
            {
                imgData.palRed = (UINT16 ) tempImgRed;
            }
        }

        tempImgGreen = (imgPal[imgInData].palGreen) + gerr1[icol];
        if( tempImgGreen < 0 )
        {
            imgData.palGreen = 0;
        }
        else
        {
            if( tempImgGreen > 0xff00 )
            {
                imgData.palGreen = 0xff00;
            }
            else
            {
                imgData.palGreen = (UINT16 ) tempImgGreen;
            }
        }

        tempImgBlue = (imgPal[imgInData].palBlue) + berr1[icol];
        if( tempImgBlue < 0 )
        {
            imgData.palBlue = 0;
        }
        else
        {
            if( tempImgBlue > 0xff00 )
            {
                imgData.palBlue = 0xff00;
            }
            else
            {
                imgData.palBlue = (UINT16 ) tempImgBlue;
            }
        }

        npal = FindClosestRGB(&imgData, palPtr);

        /* store corrected value */
        *imgOutDataPtr++ = npal;

        /* calculate corrections for surrounding points */
        colorErr = ((imgPal[imgInData].palRed - palPtr[npal].palRed) >> 4);
        rerr1[icol+1] = rerr2[icol+1] + (7 * colorErr);

        colorErr = ((imgPal[imgInData].palGreen - palPtr[npal].palGreen) >> 4);
        gerr1[icol+1] = gerr2[icol+1] + (7 * colorErr);

        colorErr = ((imgPal[imgInData].palBlue - palPtr[npal].palBlue) >> 4);
        berr1[icol+1] = berr2[icol+1] + (7 * colorErr);
    }

    /* do last pixel */
    imgInData = *imgInDataPtr++;

    /* Set NotUsed to remove warnings */
    NU_UNUSED_PARAM(imgInDataPtr);

    tempImgRed = (imgPal[imgInData].palRed) + rerr1[icol];
    if( tempImgRed < 0 )
    {
        imgData.palRed = 0;
    }
    else
    {
        if( tempImgRed > 0xff00 )
        {
            imgData.palRed = 0xff00;
        }
        else
        {
            imgData.palRed = (UINT16 ) tempImgRed;
        }
    }

    tempImgGreen = (imgPal[imgInData].palGreen) + gerr1[icol];
    if( tempImgGreen < 0 )
    {
        imgData.palGreen = 0;
    }
    else
    {
        if( tempImgGreen > 0xff00 )
        {
            imgData.palGreen = 0xff00;
        }
        else
        {
            imgData.palGreen = (UINT16 ) tempImgGreen;
        }
    }

    tempImgBlue = (imgPal[imgInData].palBlue) + berr1[icol];
    if( tempImgBlue < 0 )
    {
        imgData.palBlue = 0;
    }
    else
    {
        if( tempImgBlue > 0xff00 )
        {
            imgData.palBlue = 0xff00;
        }
        else
        {
            imgData.palBlue = (UINT16 ) tempImgBlue;
        }
    }

    npal = FindClosestRGB(&imgData, palPtr);

    /* store corrected value */
    *imgOutDataPtr++ = npal;

    /* Set NotUsed to remove warnings */
    NU_UNUSED_PARAM(imgOutDataPtr);

    /* Return to user mode */
    NU_USER_MODE();
}


/***************************************************************************
* FUNCTION
*
*    QueryRes
*
* DESCRIPTION
*
*    Function QueryRes returns the number of pixels per inch for the X and Y axis
*    respectively based on the current display mode.  The returned RESX and RESY
*    values are taken for the current rsPort's pixResX and pixResY variables.
*
* INPUTS
*
*    INT32 *resX - Pointer to returned RESX.
*
*    INT32 *resY - Pointer to returned RESY.
*
* OUTPUTS
*
*    Returns locations of values for pixels per inch for the X and Y axis.
*
***************************************************************************/
VOID QueryRes( INT32 *resX, INT32 *resY)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    *resX = thePort->portMap->pixResX;
    *resY = thePort->portMap->pixResY;

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    QueryColors
*
* DESCRIPTION
*
*    Function QueryColors returns the number of displayable colors available in
*    the current bitmap.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT32 - number of displayable colors available in the current bitmap.
*
***************************************************************************/
INT32 QueryColors(VOID)
{
    INT32 colors;
    INT32 shfCnt;


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    shfCnt = thePort->portMap->pixPlanes;
    if( shfCnt == 1 )
    {
        shfCnt = thePort->portMap->pixBits;
    }
    colors = (1 << shfCnt) - 1;

    /* Return to user mode */
    NU_USER_MODE();

    return(colors);
}

/***************************************************************************
* FUNCTION
*
*    ReadPalette
*
* DESCRIPTION
*
*    Function ReadPalette returns the current value of color indexes from
*    bgnIdx to endIdx inclusive.
*
* INPUTS
*
*    INT32 plNum - Defines which palette table to modify, the default is 0.
*
*    INT32 bgnIdx - Index number in palette array table to begin reading.
*
*    INT32 endIdx -Index number in palette array table to end reading.
*
*    palData *palPtr - Pointer to palette.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID ReadPalette(INT32 plNum, INT32 bgnIdx, INT32 endIdx, palData *palPtr)
{
    VOID *dmParam;
    INT32  devMgrPal;

    /* error value   */
    INT16 grafErrValue;     

    /* temp pal data */
    argsPalInfo dmParamPal; 


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* setup the device manager dmParam buffer */
    dmParamPal.palNum     = plNum;
    dmParamPal.palBgn     = bgnIdx;
    dmParamPal.palEnd     = endIdx;
    dmParamPal.palDataPtr = palPtr;

    /* device manager read palette function */
    dmParam = (void *) &dmParamPal;
    devMgrPal = DMRPAL;

    /* call the device manager */
    grafErrValue = (INT16)(thePort->portMap->mapDevMgr(thePort->portMap, devMgrPal, dmParam));
    if( grafErrValue != 0 )
    {
        grafErrValue = c_ReadPale +  c_InvDevFunc;

        /* report error */
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
    }

    /* Return to user mode */
    NU_USER_MODE();

}

