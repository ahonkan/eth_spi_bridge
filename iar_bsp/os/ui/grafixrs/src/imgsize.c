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
*  imgsize.c                                                    
*
* DESCRIPTION
*
*  This file contains the ImageSize and ImagePara functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  ImageSize
*  ImagePara
*
* DEPENDENCIES
*
*  rs_base.h
*  imgsize.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/imgsize.h"

/***************************************************************************
* FUNCTION
*
*    ImageSize
*
* DESCRIPTION
*
*    Function IMAGESIZE computes the number of image bytes required by ReadImage
*    to store the bitmap area contained within any rectangle similar in size to R.
*
*    To avoid shifting the image when reading,
*    the raster is read starting at the first word that contains any portion
*    of the image. Each raster is forced to be an even number of bytes long.
*    This allows STOSW transfers to be memory aligned on the source at least.
*
*    If bits/pixel is a multiple of 8, then the size of each line is the
*    number of bytes required, plus a byte of worst case shift for bytes
*    per pixel less than 2. This total is rounded up to the next word.
*
*    If bits/pixel is not a multiple of 8, then bit shifting may be
*    required to align source to destination pixels.  The size of each
*    image line then is;
*
*    (bits/pixel * pixels per line + worst case bit shift  ) / 8.
*
*    The worst case bit shift is 16 bits - (bits/pixel).
*    This byte count is also rounded up to the next word.
*
* INPUTS
*
*    rect *sR - Pointer to the image.
*
* OUTPUTS
*
*    SIGNED - Return value. Returns the size.
*
***************************************************************************/
#if (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE)
SIGNED ImageSize( rect *sR)
{
    rect   tmpR;
    INT16  bmpixBits, bmpixPlanes;
    INT32  width, height;
    SIGNED imgSize;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    bmpixBits   = theGrafPort.portMap->pixBits;
    bmpixPlanes = theGrafPort.portMap->pixPlanes;
    
    if( globalLevel > 0 )
    {
        U2GR(*sR, &tmpR, 0);
    }
    else
    {
        tmpR = *sR;
    }

    /* Calculate height  */
    height = tmpR.Ymax - tmpR.Ymin + 1 ;

    /* Calculate pix width  */
    width  = tmpR.Xmax - tmpR.Xmin + 1;

    /* Get bit per pixel  */
    if( (bmpixBits & 0x07) == 0 )
    {
        width = width * (bmpixBits >> 3);
        if( (bmpixBits >> 3) <= 1 )
        {
            width += (bmpixBits >> 3);
        }
    }
    else
    {
        /* Bits per pixel is not a multiple of 8  */
        width = width * bmpixBits ;

        /* Add 7 to round up and extra 16 to avoid shifting on read  */ 
        width += 23;
        width -= bmpixBits ;
        width = width >> 3;
    }

    /* Round up to even number of bytes  */
    width += width & 1;

    /* Multiply times height for total per plane  */ 
    imgSize = width * height;
    imgSize = (imgSize * bmpixPlanes) + sizeof(imageHdr);

    /* Return to user mode */
    NU_USER_MODE();

    return(imgSize);
}

/***************************************************************************
* FUNCTION
*
*    ImagePara
*
* DESCRIPTION
*
*    Function ImagePara determines the paragraph alignment value.
*
* INPUTS
*
*    rect *argPara - Pointer to the image.
*
* OUTPUTS
*
*    SIGNED - Return value. Returns the paragraph alignment value.
*
***************************************************************************/
SIGNED ImagePara( rect *argPara)
{
    SIGNED temp;

    temp = ImageSize(argPara);
    temp += 15;
    
    /* Round up to next paragraph  */
    return(temp >> 4);
}



#endif /* (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE) */
