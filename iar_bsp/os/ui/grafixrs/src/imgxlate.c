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
*  imgxlate.c                                                   
*
* DESCRIPTION
*
*  This file contains the XlateImage and related functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*    XlateColors
*    XlateImage
*    lowcolor_any
*    RGB16_lowcolor
*    RGB16_RGB24
*    RGB16_RGB32
*    RGB24_lowcolor
*    RGB24_RGB16
*    RGB32_RGB16
*    RGB24_RGB32
*    XL_NOP
*    GetImPix1
*    GetImPix2
*    GetImPix4
*    GetImPix8
*    GetImPix16
*    GetImPix24
*    GetImPix32
*    GetImPix83
*    SetImPix1
*    SetImPix2
*    SetImPix4
*    SetImPix8
*    SetImPix16
*    SetImPix24
*    SetImPix32
*
* DEPENDENCIES
*
*  rs_base.h
*  colorops.h
*  imgxlate.h
*  display_config.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/colorops.h"
#include "ui/imgxlate.h"
#include "drivers/display_config.h"


#if (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE)
/* Global Variables used by more than one function in this file */
INT32  curX;          /* pixel XY */
INT32  curY;
SIGNED tmpColor;      /* temp color */
INT32  imgFlgs;       /* copy of image flags */
INT32  flags;         /* copy of grafmap flags */
SIGNED *tmpXlPtr;     /* temp xlate table ptr */
image  *tmpSrcImg;    /* pointer to source image */
image  *tmpDstImg;    /* pointer to source image */
INT32  srcOffset;     /* precalculated offsets */
INT32  dstOffset;
SIGNED planeSize;     /* size of a plane */
SIGNED planeTotal;    /* size of all planes */
SIGNED dstplaneSize;  /* size of a destination plane */

extern VOID GetPort(rsPort **gpPORT);

/***************************************************************************
* FUNCTION
*
*    XlateColors
*
* DESCRIPTION
*
*    This function builds a color translation table.  It translates 
*    colors of the 8 BPP image to other BPP settings
*
* INPUTS
*
*    SIGNED *xlt      - Pointer to table.
*    UINT8 imBits     - BPP of image.
*    UINT8 imPlanes   - Number of planes.
*    palData *palette - Pointer to the palette to use.
*    palData *imgPal  - Pointer to the image data.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID XlateColors( SIGNED *xlt, UINT8 imBits, UINT8 imPlanes,  palData *palette, palData *imgPal )
{
    rsPort      *thePort;        /* current port */
    INT32       i;
    INT32       maxcolors;
    palData     RGB;
    SIGNED      dstcolors;     /* number of colors in port */
    SIGNED      srcolors = 1;  /* number of colors in pcx file */
    
    NU_SUPERV_USER_VARIABLES
        
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* how many colors in source image file ? */
    srcolors = srcolors << (imBits * imPlanes);
    srcolors--;
    
    /* how many colors in current port ? */
    dstcolors = QueryColors();
    maxcolors = srcolors + 1;
    
    /* going from palette based to RGB ? */
    if( srcolors <= 255 && dstcolors > 255 ) 
    {
        /* Make a translate table that will convert palette references into */
        /* 16/24 bit RGB values */
        GetPort( &thePort );
        
        for( i = 0; i < 256; i++ ) 
        {
            if (dstcolors > 65535)
            {   /* 24-bit color */
                xlt[i]  = ((imgPal[i].palRed & 0xFF00) << 8);
                xlt[i] |= (imgPal[i].palGreen & 0xFF00);
                xlt[i] |= ((imgPal[i].palBlue & 0xFF00) >> 8);
            }
            else
            {   
                if( thePort->portMap->mapFlags & mf565 ) 
                {
                    /* 16-bit color */
                    /* 5:6:5 RGB format */
                    xlt[i]  = (imgPal[i].palRed         ) & 0xF800;
                    xlt[i] |= (imgPal[i].palGreen >> 5  ) & 0x07E0;
                }
                else {
                    /* 15bit color */
                    /* 5:5:5 RGB format */
                    xlt[i]  = ( imgPal[i].palRed  >> 1  ) & 0x7C00;
                    xlt[i] |= (imgPal[i].palGreen >> 6  ) & 0x03E0;
                }
                xlt[i] |= (imgPal[i].palBlue  >> 11 ) & 0x001F;
            }
        }
    }
    
    /* going from RGB based to palette based ? */
    else if( srcolors > 255 && dstcolors <= 255 ) 
    {
        /* Make a translate table that will reduce a 24 bit per pixel source */
        /* to colors in the current (hopeful optimized for this image) palette. */
        /* XlateImage reduces the colors to the 512 most significant, */
        /* then we translate those to the best 256 we got */
        for( i = 0; i < 512; i++ ) 
        {
            RGB.palRed   = (UINT16)((i & 0x1C0) << 7);    /*  3 msbs of red    */
            RGB.palGreen = (UINT16)((i & 0x038) << 10);   /*  3 msbs of green  */
            RGB.palBlue  = (UINT16)((i & 0x007) << 13);   /*  3 msbs of blue   */
            xlt[i] = (SIGNED) FindClosestRGB( &RGB, palette );
        }
    }
    else
    {
        /* make a null translate table (no color conversion) when the */
        /* source color values are compatible with the current display */
        for( i = 0; i < maxcolors; i++ )
        {
            xlt[i] =  (SIGNED) FindClosestRGB( &imgPal[i], palette );
        }
    }

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    XlateImage
*
* DESCRIPTION
*
*    Function XlateImage translates an image in one format to another.
*    srcImage:
*        Source image buffer.
*
*    dstImage:
*        Destination image buffer.  If this pointer is null, XlateImage will
*        only return the size needed for the destination buffer.
*
*    dstBits:
*        Destination bits per pixel.
*
*    dstPlanes:
*        Destination planes per pixel.
*
*    xlTable:
*        Color translation table.
*
*        Case A: The source is 256 colors or less
*
*            Translate color number to target format, using
*            the source color as the index into the table.
*    
*            There must be an entry for each source image color value.
*
*       Case B: The source has more than 256 colors:
*
*            We assume that the color number is made up of 3 color components
*            (red, green, blue).  We then translate each pixel number to the
*            destination image by:
*
*            if the destination is > 256 colors
*
*        a) extract each color component.
*        b) adjust the component value by adding or deleting lsb's.
*        c) recombine the components to form a new color number.
*        
*        The xlTable is not used.
*        
*        if the destination is <= 256 colors
*
*        a) extract each color component
*        b) adjust the component value to 3 bits by deleting lsb's 
*        c) recombine the components to form a 9 bit color number.
*        d) lookup color number in 512 element xlTable.
* 
*    If xlTable is null, the color value is passed through unchanged
* 
*
*    RETURNS:
*        If dstImage is null, returns size needed for dstImage;
*        else
*            0  ==  success
*           -1  ==  failure  [error found in QueryError()]
*    NOTES:
*        1)  Currently limited to the following formats
*        a)  8, 16 bits/pixel, one plane
*        b)  1  bit/pixel, N planes
*        c)  8 bit 3 plane (source only)
*
*        2)  Currently limited to 64k buffers, max for non 386
*
*        3)  The resulting destination image will always be fully
*            left aligned, regardless of source alignment.
*
*    Method for calculation of destination buffer size is equivalent to
*    ImageSize.
*
* INPUTS
*
*    image *argSrcImg - Pointer to source image. 
*
*    image *argDstImg - Pointer to destination image.
*
*    INT32 argDstBits - Destination bits per pixel.
*
*    INT32 argDstPlns - Destination planes per pixel.
*
*    SIGNED *argXlPtr - Pointer to color translation table.
*
* OUTPUTS
*
*    SIGNED - Return value. Returns size needed for dstImage.
*
***************************************************************************/
SIGNED XlateImage( image *argSrcImg, image *argDstImg, INT32 argDstBits, INT32 argDstPlns,
            SIGNED *argXlPtr)
{
    INT16   Done = NU_FALSE;
    SIGNED value;

    VOID (*srcPtr)(VOID);   /* code fragment offsets */
    VOID (*dstPtr)(VOID);
    VOID (*xlatPtr)(VOID);

    /* calculate destination bytes per row - bits = width * bits/pixel rounded
       up to next byte and rounded up to even number of bytes */

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* get a copy of the default grafmap flags for determining hicolor dest
       image format (there is no other way to guess what the destination format
       for hicolor should be) */
    flags = defGrafMap.mapFlags;

    /* test dest image pointer for null */
    if( argDstImg == 0 )
    {
        /* yes, so just calculate size required for dest image */
        /* multiply times height for total per plane (SIGNED result in ae)
           and add size of imageHdr */
        value = (((((argDstBits * argSrcImg->imWidth + 7) >> 3) + 1) >> 1) << 1);

        value = value * (argSrcImg->imHeight * argDstPlns);
        value = value + sizeof(image);        
        Done  = NU_TRUE;
    }

    if( !Done )
    {
        /* This routine consists of a loop which calls three subroutines indirectly.
        The subroutines are:
        1) get a source image pixel value
        2) translate the color value
        3) put a dest image pixel value
        The indirect vectors for these subroutines are loaded depending on the 
        method which is appropriate for the image formats */

        /* set up the destination image header */
        argDstImg->imWidth  = argSrcImg->imWidth;
        argDstImg->imHeight = argSrcImg->imHeight;
        argDstImg->imAlign  = 0;
        
        /* default to less than 64k */
        argDstImg->imFlags  = 0; 

        argDstImg->imBytes  = (((((argDstBits * argSrcImg->imWidth + 7) >> 3) + 1) >> 1) << 1);
        argDstImg->imBits   = argDstBits;
        argDstImg->imPlanes = argDstPlns;

        /* Set up 'get' method */
        switch( argSrcImg->imBits )
        {
        case 1:
            /* 1 bit GetImPix */
            srcPtr = GetImPix1;     
            break;
        case 2:
            /* 2 bit GetImPix */
            srcPtr = GetImPix2;     
            break;
        case 4:
            /* 4 bit GetImPix */
            srcPtr = GetImPix4;     
            break;
        case 8:
            /* check for special case 8 bit 3 plane 24 bit PCX file image format */
            if( argSrcImg->imPlanes == 3)
            {
                srcPtr = GetImPix83;
            }
            else
            {
                /* 8 bit GetImPix */
                srcPtr = GetImPix8;    
            }
            break;
        case 16:
            /* 16 bit GetImPix */
            srcPtr = GetImPix16;    
            break;
        case 24:
            /* 24 bit GetImPix */
            srcPtr = GetImPix24;    
            break;
        case 32:
            /* 32 bit GetImPix */
            srcPtr = GetImPix32;    
            break;
        default:
            planeSize = c_XlateIma +  c_InvDevFunc;
            nuGrafErr((INT16) planeSize, __LINE__, __FILE__);  
            value = -1;
            Done = NU_TRUE;
        }
    } /* if( !Done ) */ 

    
    /* Set up 'put' method */
    if( !Done )
    {
        switch (argDstBits)
        {
        case 1:
            /* 1 bit SetImPix */
            dstPtr = SetImPix1;     
            break;
        case 2:
            /* 2 bit SetImPix */
            dstPtr = SetImPix2;     
            break;
        case 4:
            /* 4 bit SetImPix */
            dstPtr = SetImPix4;     
            break;
        case 8:
            /* 8 bit SetImPix */
            dstPtr = SetImPix8;     
            break;
        case 16:
            /* 16 bit SetImPix */
            dstPtr = SetImPix16;    
            break;
        case 24:
            /* 24 bit SetImPix */
            dstPtr = SetImPix24;    
            break;
        case 32:
            /* 32 bit SetImPix */
            dstPtr = SetImPix32;    
            break;
        default:
            planeSize = c_XlateIma +  c_InvDevFunc;
            nuGrafErr((INT16) planeSize, __LINE__, __FILE__);   
            value = -1;
            Done = NU_TRUE;
        }
    } /* if( !Done ) */

    if( !Done )
    {
        /* Set up translation method */
        planeTotal = (((argSrcImg->imBits * argSrcImg->imPlanes) >> 3) << 2) + (argDstBits >>3);

        switch (planeTotal)
        {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            /* <= 256 color source */
            xlatPtr = lowcolor_any;     
            break;
        case 8:
        case 9:
            /* 16 bpp source, <= 256 color dest */
            xlatPtr = RGB16_lowcolor;   
            break;
        case 10:
            /* 16 bpp source, highcolor dest */
            xlatPtr = XL_NOP;           
            break;
        case 11:
            /* 16 bpp source, 24 bit dest */
            xlatPtr = RGB16_RGB24;      
            break;
        case 12:
            /* 16 bpp source, 32 bit dest */
            xlatPtr = RGB16_RGB32;
			break;
        case 13:
            /* 24 bpp source, <= 256 color dest */
            xlatPtr = RGB24_lowcolor;   
            break;
        case 14:
            /* 24 bpp source, highcolor dest */
            xlatPtr = RGB24_RGB16;      
            break;
        case 16:
            /* 24 bpp source, 32 dest */
            xlatPtr = RGB24_RGB32;      
            break;
        case 18:
            /* 24 bpp source, 32 dest */
            xlatPtr = RGB32_RGB16;      
            break;
        default:
            /* 24 bpp source, 24 bit dest */
            xlatPtr = XL_NOP;           
        }

        if( (argXlPtr == NU_NULL)
            &&
            (   (xlatPtr != RGB16_RGB24)
             && (xlatPtr != RGB24_RGB16)
            )
          )
        {
            /* don't do xlate */
            xlatPtr = XL_NOP; 
        }

        /* actual translation loop */

        /* pre-calculate size of a plane */
        planeSize    = argSrcImg->imBytes * argSrcImg->imHeight;
        planeTotal   = planeSize * (argSrcImg->imPlanes - 1);
        dstplaneSize = argDstImg->imBytes * argDstImg->imHeight;

        /* Initialize offsets
           Since the Y pixel coordinate value is actually the raster line, it is 
           calculated once (per raster line) into these two values */
        srcOffset = 0;
        dstOffset = 0;

        /* xlate table pointer */
        tmpXlPtr  = argXlPtr;    
        imgFlgs   = argSrcImg->imFlags;
        tmpSrcImg = argSrcImg;
        tmpDstImg = argDstImg;
       
        /* Ok, now begin at the top left, and step through the image raster by
           raster curY[br], curX[br] are the PIXEL coordinates we want. Each
           routine will convert the X offset to an appropriate bit/byte offset. */
        for( curY = 0; curY < argSrcImg->imHeight; curY++)
        {
            for( curX = 0; curX < argSrcImg->imWidth; curX++)
            {
                /* get src color */
                srcPtr();   

                /* xlate into new color */
                xlatPtr();  

                /* set dest color */
                dstPtr();   
            }
            srcOffset += argSrcImg->imBytes;
            dstOffset += argDstImg->imBytes;
        }

        value = 0;
    } /* if( !Done ) */

    /* Return to user mode */
    NU_USER_MODE();

    return (value);
}

/* Some functions used by XLATEIMAGE */
/***************************************************************************
* FUNCTION
*
*    lowcolor_any
*
* DESCRIPTION
*
*    <= 256 color source
*    expects color between 0 and 255 
*    and returns the (SIGNED) table value.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID lowcolor_any(VOID)
{
    INT32 colorIndex;
    
    colorIndex = tmpColor & 0xff;       
    tmpColor   = tmpXlPtr[colorIndex];    
}

/***************************************************************************
* FUNCTION
*
*    RGB16_lowcolor
*
* DESCRIPTION
*
*    RGB16 source, <= 256 color dest
*    takes the 3 msbs of the 5 bits of RG&B to make a 9 bit index to xlate
*    table which must be 512 entries SIGNED
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RGB16_lowcolor(VOID)
{
    INT32 colorIndex;

    /* initial shift for all colors */
    tmpColor = (tmpColor >> 2);     

    /* mask off all but 3 blue msbs */
    colorIndex = (tmpColor & 0x07); 
    
    if( imgFlgs & im565)
    {
        /* source image 5:6:5 format ? */
        tmpColor = tmpColor >> 1;   
    }

    /* shift down for 3 green msbs and mask off all but green bits */
    colorIndex |= ((tmpColor >> 2) & 0x038);

    /* shift down for 3 red msbs and mask off all but red bits */
    colorIndex |= ((tmpColor >> 4) & 0x01c0);

    /* fetch SIGNED from table set */
    tmpColor = (tmpXlPtr[colorIndex]);

}

/***************************************************************************
* FUNCTION
*
*    RGB16_RGB24
*
* DESCRIPTION
*
* 16 bpp source, 24 bit dest
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RGB16_RGB24(VOID)
{
    SIGNED colorIndex;

	/* Format of 16-bit image:                                                     */
	/* If CM565 -- RRRRRGGG GGGBBBBB											   */
	/* Else     -- xRRRRRGG GGGBBBBB											   */
	/* x is "Don't care" bit													   */

	/* To convert a 16-bit image to a 24-bit image consider the following format   */
	/* for shifting the bits in an appropriate manner:                             */

	/* Format of 24-bit image (3 bytes per pixel):                                 */
	          
	/*             High Byte  Middle byte   Low byte                               */
	/* If CM565 --  BBBBBxxx   GGGGGGxx      RRRRRxxx							   */
	/* Else     --  BBBBBxxx   GGGGGxxx      RRRRRxxx							   */
	/* x is "Don't care" bit													   */

	/* Get Blue bits and shift to the High byte */
	colorIndex = ((tmpColor << 19) & 0xF80000);

#ifdef CM565

	/* Get Red bits and shift to the Low byte */
	colorIndex |= ((tmpColor >> 8) & 0xF8);

	/* Get Green bits and shift to the Middle byte */
	colorIndex |= ((tmpColor << 5) & 0xFC00);

#else

	/* Get Red bits and shift to the Low byte */
	colorIndex |= ((tmpColor >> 7) & 0xF8);

	/* Get Green bits and shift to the Middle byte */
	colorIndex |= ((tmpColor << 6) & 0xF800);

#endif

    tmpColor = colorIndex;

}


/***************************************************************************
* FUNCTION
*
*    RGB16_RGB32
*
* DESCRIPTION
*
* 16 bpp source, 32 bit dest
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RGB16_RGB32(VOID)
{
    SIGNED colorIndex;

	/* Format of 16-bit image:                                                     */
	/* If CM565 -- RRRRRGGG GGGBBBBB											   */
	/* Else     -- xRRRRRGG GGGBBBBB											   */
	/* x is "Don't care" bit													   */

	/* To convert a 16-bit image to a 32-bit image consider the following format   */
	/* for shifting the bits in an appropriate manner:                             */

	/* Format of 32-bit image (3 bytes per pixel):                                 */
	          
	/*             Alpha Byte(4) High Byte(3)  Middle byte(2)   Low byte (1)       */
	/* If CM565 --	AAAAAAAA      BBBBBxxx      GGGGGGxx         RRRRRxxx		   */
	/* Else     --	AAAAAAAA      BBBBBxxx      GGGGGxxx         RRRRRxxx		   */
	/* x is "Don't care" bit													   */

	/* Get Blue bits and shift to the High byte */
	colorIndex = ((tmpColor << 19) & 0xF80000);

#ifdef CM565

	/* Get Red bits and shift to the Low byte */
	colorIndex |= ((tmpColor >> 8) & 0xF8);

	/* Get Green bits and shift to the Middle byte */
	colorIndex |= ((tmpColor << 5) & 0xFC00);

#else

	/* Get Red bits and shift to the Low byte */
	colorIndex |= ((tmpColor >> 7) & 0xF8);

	/* Get Green bits and shift to the Middle byte */
	colorIndex |= ((tmpColor << 6) & 0xF800);

#endif

    tmpColor = colorIndex;

}

/***************************************************************************
* FUNCTION
*
*    RGB24_lowcolor
*
* DESCRIPTION
*
*    24 bpp source, <= 256 color dest
*    takes the 3 msbs of the 8 bits of RG&B to make a 9 bit index to xlate table
*    which must be 512 entries SIGNED
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RGB24_lowcolor(VOID)
{
    INT32 colorIndex;

    /* initial shift for all colors */
    tmpColor = (tmpColor >> 5); 

    /* mask off all but 3 blue msbs */
	colorIndex  = (tmpColor & 0xc);

    /* shift down for 3 green msbs and mask off all but green bits */
	colorIndex |= ((tmpColor >> 5) & 0x03c);

    /* shift down for 3 red msbs and mask off all but red bits */
	colorIndex |= ((tmpColor >> 10) & 0x0180);

    /* fetch SIGNED from table set */
    tmpColor = (tmpXlPtr[colorIndex]);
}

/***************************************************************************
* FUNCTION
*
*    RGB24_RGB16
*
* DESCRIPTION
*
*    24 bpp source, highcolor dest
*    takes the 5 msbs of the 8 bits of RG&B (6 of G if 5:6:5) 
*    to make a 15/16 bit value
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RGB24_RGB16(VOID)
{
    SIGNED colorIndex;
	/* Consider the two formats of 24-bit image (3 bytes per pixel):               */
	/* The formats are considered for conversion purposes only.                    */
	/*             High Byte  Middle byte   Low byte                               */
	/* If CM565 --  BBBBBxxx   GGGGGGxx      RRRRRxxx							   */
	/* Else     --  BBBBBxxx   GGGGGxxx      RRRRRxxx							   */
	/* x is "Don't care" bit													   */

	/* To convert a 24-bit image to a 16-bit image consider the following format   */
	/* for shifting the bits in an appropriate manner:                             */

	/* Format of 16-bit image (2 bytes per pixel):                                 */

	/*			     High(15-11)   Middle(10-5)    Low(4-0)                        */
    /*             15 14 13 12 11  10 9 8 7 6 5   4 3 2 1 0                        */ 
	/* If CM565 --  R  R  R  R  R   G G G G G G   B B B B B						   */

	/*			     High(14-10)   Middle(9-5)     Low(4-0)                        */
	/*             15 14 13 12 11  10 9 8 7 6 5   4 3 2 1 0                        */ 
	/* Else     --  x  R  R  R  R   R G G G G G   B B B B B						   */
	/* x is "Don't care" bit													   */




  
	/* Get the 5 MSBs of Blue and shift to the "Low" */
    colorIndex =  ((tmpColor >> 19) & 0x1F);

#ifdef CM565

	/* Get the 6 MSBs of Green and shift to the "Middle" */
	colorIndex |= ((tmpColor >> 5) & 0x7E0);

	/* Get the 5 MSBs of Red and shift to the "High" */
	colorIndex |= ((tmpColor << 8) & 0xF800);

#else

    /* Get the 5 MSBs of Green and shift to the "Middle" */
	colorIndex |= ((tmpColor >> 6) & 0x3E0);

	/* Get the 5 MSBs of Red and shift to the "High" */
	colorIndex |= ((tmpColor << 7) & 0x7C00);

#endif

    tmpColor = colorIndex;
}

/***************************************************************************
* FUNCTION
*
*    RGB32_RGB16
*
* DESCRIPTION
*
* 32 bpp source, 16 bit dest
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RGB32_RGB16(VOID)
{
    SIGNED colorIndex;
	/* Consider the two formats of 32-bit image (3 bytes per pixel):               */
	/* Format of 32-bit image (3 bytes per pixel):                                 */
	          
	/*             Alpha Byte(4) High Byte(3)  Middle byte(2)   Low byte (1)       */
	/* If CM565 --	AAAAAAAA      BBBBBxxx      GGGGGGxx         RRRRRxxx		   */
	/* Else     --	AAAAAAAA      BBBBBxxx      GGGGGxxx         RRRRRxxx		   */
	/* x is "Don't care" bit													   */

	/* To convert a 32-bit image to a 16-bit image consider the following format   */
	/* for shifting the bits in an appropriate manner:                             */

	/* Format of 16-bit image (2 bytes per pixel):                                 */

	/*			     High(15-11)   Middle(10-5)    Low(4-0)                        */
    /*             15 14 13 12 11  10 9 8 7 6 5   4 3 2 1 0                        */ 
	/* If CM565 --  R  R  R  R  R   G G G G G G   B B B B B						   */

	/*			     High(14-10)   Middle(9-5)     Low(4-0)                        */
	/*             15 14 13 12 11  10 9 8 7 6 5   4 3 2 1 0                        */ 
	/* Else     --  x  R  R  R  R   R G G G G G   B B B B B						   */
	/* x is "Don't care" bit													   */


	/* Get the 5 MSBs of Blue and shift to the "Low" */
    colorIndex =  ((tmpColor >> 19) & 0x001F);

#ifdef CM565

	/* Get the 6 MSBs of Green and shift to the "Middle" */
	colorIndex |= ((tmpColor >> 5) & 0x07E0);

	/* Get the 5 MSBs of Red and shift to the "High" */
	colorIndex |= ((tmpColor << 8) & 0xF800);

#else

    /* Get the 5 MSBs of Green and shift to the "Middle" */
	colorIndex |= ((tmpColor >> 6) & 0x03E0);

	/* Get the 5 MSBs of Red and shift to the "High" */
	colorIndex |= ((tmpColor << 7) & 0x7C00);

#endif

    tmpColor = colorIndex;

}

/***************************************************************************
* FUNCTION
*
*    RGB24_RGB32
*
* DESCRIPTION
*
*    24 bpp source, 32 bpp dest
*     
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID RGB24_RGB32(VOID)
{
    SIGNED colorIndex;

	colorIndex = tmpColor;

    tmpColor = colorIndex;
}

/***************************************************************************
* FUNCTION
*
*    XL_NOP
*
* DESCRIPTION
*
*    Don't translate.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID XL_NOP(VOID)
{
    return;
}

/***************************************************************************
* FUNCTION
*
*    GetImPix1
*
* DESCRIPTION
*
*    1 bit GetImPix
*    1 bit and 1 bit/multi-plane source
*    tmpSrcImg -> source image data area
*    srcOffset -> beginning of raster line
*    planeSize = size of one plane in bytes
*    planeTotal = size of all planes in bytes
*    curX = current pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID GetImPix1(VOID)
{
    SIGNED bfrX;
    INT16 bfrBit;
    INT32 i;
 
    /* figure byte offset + the image alignment bits (0..15) */
    bfrX = curX + tmpSrcImg->imAlign;

    /* which bit in byte */
    bfrBit = 7 - (bfrX & 7);

    /* byte offset relative to image start */
    bfrX = (bfrX >> 3) + srcOffset; 

    /* and move to pixel in last plane */
    bfrX += planeTotal;             

    /* We are now at the desired raster, byte, and bit offset. */
    tmpColor = 0;   

    /* for each image plane */
    for( i = 0; i < tmpSrcImg->imPlanes; i++)
    {
        /* shift last color bit */
        tmpColor = (tmpColor << 1);

        /* get bit */
        tmpColor += ((tmpSrcImg->imData[bfrX] >> bfrBit) & 1);

        /* point to next planes data */
        bfrX -= planeSize;
    }

}

/***************************************************************************
* FUNCTION
*
*    GetImPix2
*
* DESCRIPTION
*
*    2 bit GetImPix
*    2 bit source
*    tmpSrcImg -> source image data area
*    srcOffset -> beginning of raster line
*    planeSize = size of one plane in bytes
*    curX = current pixel X coord
*    note: a hibble is half a nibble
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID GetImPix2(VOID)
{
    SIGNED bfrX;
    INT16 bfrBit;

    /* figure byte offset + the image alignment bits (0..15) */
    bfrX = curX + tmpSrcImg->imAlign;

    /* compute nibble address */
    bfrBit = (3 - (bfrX & 3)) << 1; 

    /* byte offset relative to image start */
    bfrX = (bfrX >> 2) + srcOffset; 
    
    /* get bits */
    tmpColor = ((tmpSrcImg->imData[bfrX] >> bfrBit) & 3);

}

/***************************************************************************
* FUNCTION
*
*    GetImPix4
*
* DESCRIPTION
*
*    4 bit GetImPix
*    4 bit source
*    tmpSrcImg -> source image data area
*    srcOffset -> beginning of raster line
*    curX = current pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID GetImPix4(VOID)
{
    SIGNED bfrX;
    INT16 bfrBit;

    /* figure byte offset + the image alignment bits (0..15) */
    bfrX = curX + tmpSrcImg->imAlign;

    /* compute nibble address */
    bfrBit = (1 - (bfrX & 1)) << 2; 

    /* byte offset relative to image start */
    bfrX = (bfrX >> 1) + srcOffset; 
    
    /* get bits */
    tmpColor = ((tmpSrcImg->imData[bfrX] >> bfrBit) & 0x0f);

}

/***************************************************************************
* FUNCTION
*
*    GetImPix8
*
* DESCRIPTION
*
*    8 bit GetImPix
*    8 bit source
*    tmpSrcImg -> source image data area
*    srcOffset -> beginning of raster line
*    curX = current pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID GetImPix8(VOID)
{
    SIGNED bfrX;

    /* figure byte offset + the image alignment bits (0..15) */
    bfrX = curX + tmpSrcImg->imAlign + srcOffset;
    
    /* get byte */
    tmpColor = tmpSrcImg->imData[bfrX];

}

/***************************************************************************
* FUNCTION
*
*    GetImPix16
*
* DESCRIPTION
*
*    16 bit GetImPix
*    15/16 bit source
*    tmpSrcImg -> source image data area
*    srcOffset -> beginning of raster line
*    curX = current pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID GetImPix16(VOID)
{
    SIGNED bfrX;
    UINT16 *bfrXW;

    /* figure word offset */
    bfrX = (curX << 1) + srcOffset; 

    bfrXW = (UINT16 *) &tmpSrcImg->imData[bfrX];
    tmpColor = (SIGNED) *bfrXW;

}

/***************************************************************************
* FUNCTION
*
*    GetImPix24
*
* DESCRIPTION
*
*    24 bit GetImPix
*    24 bit source
*    tmpSrcImg -> source image data area
*    srcOffset -> beginning of raster line
*    curX = current pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID GetImPix24(VOID)
{
    SIGNED bfrX;

    /* figure 3 byte offset */
    bfrX = (curX << 1) + curX + srcOffset; 

    /* get first two bytes */
    tmpColor = (tmpSrcImg->imData[bfrX] << 8) +
                tmpSrcImg->imData[bfrX+1];

    /* get last byte */
    tmpColor = (tmpColor << 8) + tmpSrcImg->imData[bfrX+2];

}

/***************************************************************************
* FUNCTION
*
*    GetImPix32
*
* DESCRIPTION
*
*    32 bit GetImPix
*    32 bit source
*    tmpSrcImg -> source image data area
*    srcOffset -> beginning of raster line
*    curX = current pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID GetImPix32(VOID)
{
    SIGNED bfrX;

    /* figure 3 byte offset */
    bfrX = (curX << 1) + curX + srcOffset; 

    /* get first two bytes */
    tmpColor = (tmpSrcImg->imData[bfrX] << 8) +
                tmpSrcImg->imData[bfrX+1];

    /* get last byte */
    tmpColor = (tmpColor << 8) + tmpSrcImg->imData[bfrX+2];

}

/***************************************************************************
* FUNCTION
*
*    GetImPix83
*
* DESCRIPTION
*
*    8 bit/3 plane GetImPix
*    8 bit 3  plane source used on 24 bit PCX files
*    tmpSrcImg -> source image data area
*    srcOffset -> beginning of raster line
*    planeSize = size of one plane in bytes
*    curX = current pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID GetImPix83(VOID)
{
    SIGNED bfrX;

    /* figure byte offset */
    bfrX = curX + tmpSrcImg->imAlign + srcOffset;
    
    /* get red byte */
    tmpColor = (tmpSrcImg->imData[bfrX] << 8);
    
    /* next plane */
    bfrX += planeSize;
    
    /* get green byte */
    tmpColor = (tmpColor << 8) + tmpSrcImg->imData[bfrX];

    /* next plane */
    bfrX += planeSize;
    
    /* get blue byte */
    tmpColor = (tmpColor << 8) + tmpSrcImg->imData[bfrX];

}

/***************************************************************************
* FUNCTION
*
*    SetImPix1
*
* DESCRIPTION
*
*    1 bit SetImPix
*    The 1 bit/pix also handles the multi-plane case
*    tmpDstImg -> destination image data area
*    dstOffset -> beginning of raster line
*    dstX = destination pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetImPix1(VOID)
{
    SIGNED bfrX;
    UINT8 bfrBit;
    INT32 i;
    UINT8 dstColor;

    /* make the bit mask for the desired pixel to affect in the byte */
    /* only want modulo bits */
    bfrBit = 7 - (curX & 7);    
    bfrBit = (1 << bfrBit);

    /* byte offset relative to image start */
    bfrX = (curX >> 3) + dstOffset; 

    /* We are now at the desired byte to affect (in plane 0, if multiplane)
       Since we are creating this image buffer ourselves, there is no alignment  
       to worry about. */
    
    /* for each image plane */
    for( i = 0; i < tmpDstImg->imPlanes; i++)
    {
        /* get byte the pixel is in */
        dstColor = tmpDstImg->imData[bfrX]; 

        /* clear the bit to affect */
        dstColor &= ~bfrBit;                

        /* does this plane get a bit set */
        if( tmpColor & 1 )
        {
            dstColor |= bfrBit;
        }

        /* put updated byte back in plane */
        tmpDstImg->imData[bfrX] = dstColor;
        
        /* point to next planes data */
        bfrX += dstplaneSize;
        
        /* get next bit */
        tmpColor = (tmpColor >> 1);
    }

}

/***************************************************************************
* FUNCTION
*
*    SetImPix2
*
* DESCRIPTION
*
*    2 bit SetImPix
*    2 bit per pixel dest 
*    tmpDstImg -> destination image data area
*    dstOffset -> beginning of raster line
*    dstX = destination pixel X coord
*    note: a hibble is half a nibble
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetImPix2(VOID)
{
    SIGNED bfrX;
    UINT8  bfrBit;
    UINT8  dstColor;

    /* make the bit masks for the desired pixels to affect in the byte */
    /* compute hibble address */
    bfrBit = (3 - (curX & 3)) << 1;

    tmpColor = (tmpColor << bfrBit);
    bfrBit = (3 << bfrBit);

    /* byte offset relative to image start */
    bfrX = (curX >> 2) + dstOffset;     

    /* get byte the pixel is in */
    dstColor = tmpDstImg->imData[bfrX]; 

    /* clear the bit to affect */
    dstColor &= ~bfrBit;                

    /* set appropriate bits */
    dstColor |= tmpColor;               

    /* put updated byte back in plane */
    tmpDstImg->imData[bfrX] = dstColor;

}

/***************************************************************************
* FUNCTION
*
*    SetImPix4
*
* DESCRIPTION
*
*    4 bit SetImPix
*    4 bit per pixel dest 
*    tmpDstImg -> destination image data area
*    dstOffset -> beginning of raster line
*    dstX = destination pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetImPix4(VOID)
{
    SIGNED bfrX;
    UINT8  bfrBit;
    UINT8  dstColor;

    /* make the bit masks for the desired pixels to affect in the byte */
    /* compute nibble address */
    bfrBit = (1 - (curX & 1)) << 2;

    tmpColor = (tmpColor << bfrBit);

    bfrBit = (0x0f << bfrBit);

    /* byte offset relative to image start */
    bfrX = (curX >> 1) + dstOffset;     

    /* get byte the pixel is in */
    dstColor = tmpDstImg->imData[bfrX]; 

    /* clear the bit to affect */
    dstColor &= ~bfrBit;                

    /* set appropriate bits */
    dstColor |= tmpColor;               
    
    /* put updated byte back in plane */
    tmpDstImg->imData[bfrX] = dstColor;

}

/***************************************************************************
* FUNCTION
*
*    SetImPix8
*
* DESCRIPTION
*
*    8 bit SetImPix
*    8 bit per pixel dest 
*    tmpDstImg -> destination image data area
*    dstOffset -> beginning of raster line
*    dstX = destination pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetImPix8(VOID)
{
    SIGNED bfrX;

    /* byte offset relative to image start */
    bfrX = curX + dstOffset;

    /* put byte in plane */
    tmpDstImg->imData[bfrX] = (UINT8) tmpColor;

}

/***************************************************************************
* FUNCTION
*
*    SetImPix16
*
* DESCRIPTION
*
*    16 bit SetImPix
*    16 bit per pixel dest 
*    tmpDstImg -> destination image data area
*    dstOffset -> beginning of raster line
*    dstX = destination pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetImPix16(VOID)
{
    SIGNED bfrX;
    UINT16 *bfrXW;

    /* word offset relative to image start */
    bfrX = (curX << 1) + dstOffset;

    bfrXW = (UINT16 *) &tmpDstImg->imData[bfrX];
    *bfrXW = (UINT16) tmpColor;

}

/***************************************************************************
* FUNCTION
*
*    SetImPix24
*
* DESCRIPTION
*
*    24 bit SetImPix
*    24 bit per pixel dest 
*    tmpDstImg -> destination image data area
*    dstOffset -> beginning of raster line
*    dstX = destination pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetImPix24(VOID)
{
    SIGNED bfrX;

    /* figure 3 byte offset */
    bfrX = (curX << 1) + curX + dstOffset;

    /* put third byte in plane */
    tmpDstImg->imData[bfrX+2] = (UINT8)(tmpColor & 0xff);  
    /* shift color byte */
    tmpColor = (tmpColor >> 8);                     

    /* put second byte in plane */
    tmpDstImg->imData[bfrX+1] = (UINT8) (tmpColor & 0xff);
    
    /* put first byte in plane */
    tmpDstImg->imData[bfrX] = (UINT8) (tmpColor >> 8);

}

/***************************************************************************
* FUNCTION
*
*    SetImPix32
*
* DESCRIPTION
*
*    32 bit SetImPix
*    32 bit per pixel dest 
*    tmpDstImg -> destination image data area
*    dstOffset -> beginning of raster line
*    dstX = destination pixel X coord
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetImPix32(VOID)
{
    SIGNED bfrX;

    /* figure 4 byte offset */
	bfrX = (curX << 1) + 2 * curX + dstOffset;

	/* fourth byte is alpha. By default make it 255 so that
	   this color is 100% visible. */
	tmpDstImg->imData[bfrX+3] = 0xFF;

	/* put third byte in plane */
    tmpDstImg->imData[bfrX+2] = (UINT8)(tmpColor & 0xff);  

	/* shift color byte */
    tmpColor = (tmpColor >> 8);         
	
	/* put second byte in plane */
    tmpDstImg->imData[bfrX+1] = (UINT8) (tmpColor & 0xff);

	/* put first byte in plane */
    tmpDstImg->imData[bfrX] = (UINT8) (tmpColor >> 8);

}
#endif /* (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE) */
