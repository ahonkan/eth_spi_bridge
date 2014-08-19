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
*  imagerw.c                                                    
*
* DESCRIPTION
*
*  This file contains the Image Read and Write functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  ReadImage
*  WriteImage
*
* DEPENDENCIES
*
*  rs_base.h
*  imagerw.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/imagerw.h"

/***************************************************************************
* FUNCTION
*
*    ReadImage
*
* DESCRIPTION
*
*    Function ReadImage copies the rectangular raster image specified by ARGR
*    into the current bitmap into the image storage array defined by ARGimage.
*    The READIMAGE and WRITEIMAGE procedures perform complementary operations.  A 
*    READIMAGE stored array may be rewritten into the same or different screen 
*    position with the WRITEIMAGE procedure.
*
* INPUTS
*
*    rect *argR      - Pointer to rectangular raster image.
*
*    image *argImage - Pointer to image storage array.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
#if (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE)
VOID ReadImage(rect *argR, image *argImage)
{
    rect tmpR;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( globalLevel > 0 )
    {
        U2GR(*argR, &tmpR, 0);
    }
    else
    {
        tmpR = *argR;
    }

    theGrafPort.portMap->prRdImg(theGrafPort.portMap, argImage,
               tmpR.Ymax,tmpR.Xmax,tmpR.Ymin,tmpR.Xmin);

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    WriteImage
*
* DESCRIPTION
*
*    Function WriteImage writes the raster image from the specified "srcIMAGE" 
*    array into the rectangular area specified by dstR in the current bitmap.
*
* INPUTS
*
*    rect *argR      - Pointer to rectangular destination area.
*
*    image *argImage - Pointer to image to write.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID WriteImage(rect * dstR, image * srcImage)
{
    rect tmpR;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( globalLevel > 0 )
    {
        U2GR(*dstR, &tmpR, 0);
    }
    else
    {
        tmpR = *dstR;
    }

    /* The write image primitives will color translate monochrome source images
       using the blitBack and blitFore colors. */
    grafBlist.Xmin = tmpR.Xmin;
    grafBlist.Xmax = tmpR.Xmax;
    grafBlist.Ymin = tmpR.Ymin;
    grafBlist.Ymax = tmpR.Ymax;

    grafBlit.blitSmap = (grafMap *) srcImage;
    (grafBlit.blitDmap->prWrImg)(&grafBlit);

    /* Return to user mode */
    NU_USER_MODE();

}
#endif /* (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE) */
