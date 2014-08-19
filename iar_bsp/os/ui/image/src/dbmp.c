/******************************************************************************
*                                                                             
*              Copyright Mentor Graphics Corporation 2006                     
*                        All Rights Reserved.                                 
*                                                                             
*  THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS       
*  THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS        
*  SUBJECT TO LICENSE TERMS.                                                  
*                                                                             
*                                                                             
*******************************************************************************
*******************************************************************************
*
* FILE NAME                                                       
*
*   dbmp.c                                                      
*
* DESCRIPTION
*
*   Contains files which decompress and displays bmp images
*
* FUNCTIONS
*
*   Display_Bmp_Image
*   Get_Bmp_Header
*   Convert_Palette    
*
* DEPENDENCIES
*
*   image/inc/dbmp.h
*   fal/inc/fal.h
*
******************************************************************************/

#ifdef NU_SIMULATION
#include "ui/noatom.h"
#endif

char dbmp_remove_warning;

#include "ui/dbmp.h"

#ifdef IMG_INCLUDED
/* Misc function prototypes */
#include "ui/rsextern.h"
#include "storage/pcdisk.h"

UINT32			bmp_offset;

/********************************************************************************
*                                                                               *
*   FUNCTION                                                                    *
*                                                                               *
*       Display_Bmp_Image                                                       *
*                                                                               *
*   DESCRIPTION                                                                 *
*                                                                               *
*       The function Display_Bmp_Image is used to draw bmp images.              *
*                                                                               *
*   INPUTS                                                                      *
*                                                                               *
*       FAL_FILE fp             -   Pointer to the bmp image file               *
*                                                                               *
*       INT32 x_coord, y_coord  -   These are the coordinates of top-left       *
*                                   point of the image.                         *
*                                                                               *
*                                                                               *
*   OUTPUTS                                                                     *
*                                                                               *
*       STATUS                  -   Returns NU_SUCCESS if successful.           *
*                                   Returns non-zero number if not successful.  *
*                                                                               *
*********************************************************************************/
                                                                                
STATUS Display_Bmp_Image(INT fp, INT32 x_coord, INT32 y_coord)
{
    
    palData         RGBdata[256];
	grafPort        *scrnport;
    UINT8			*tempPtr;
	UINT8			*newPalette=NU_NULL, *imPalette=NU_NULL;
    UINT16          pixBits, pixPlanes;
	UINT32		    i, buf_size, filePointer;
	UINT32			row_length, imgSize;
    INT32           row, row_width;
    SIGNED          xTable[512];
	image			*bmpImag=NU_NULL, *newBmpImag=NU_NULL;
	rect			myrect;
    STATUS          status = NU_INVALID_ARGUMENTS;

    /* get pointer to current (default) port */
    GetPort( &scrnport );

    pixBits   = scrnport->portMap->pixBits;
    pixPlanes = scrnport->portMap->pixPlanes;

    for(i=0;i<10;i++)
    {
        RGBdata[i].palRed   = i;
        RGBdata[i].palGreen = 0;
        RGBdata[i].palBlue  = 0;
        
        RGBdata[i+246].palRed   = i+246;
        RGBdata[i+246].palGreen = 0;
        RGBdata[i+246].palBlue  = 0;
 
    }
    
    /* Defined Palette values */
    for(i=10; i<246; i++)
    {
        RGBdata[i].palRed   = system_palette[i].palRed;
        RGBdata[i].palGreen = system_palette[i].palGreen;
        RGBdata[i].palBlue  = system_palette[i].palBlue;
    }

	/* Allocate the memory for the bmpImage structure. */
    if((status = GRAFIX_Allocation(&System_Memory, (VOID *)&bmpImag, 
                         sizeof(image), NU_NO_SUSPEND)) == NU_SUCCESS)
    {
        status = Get_Bmp_Header(fp, bmpImag);
    }

    if(status == NU_SUCCESS)
    {

        /* Validate bits per pixel */
	    if(bmpImag->imBits != 1 && bmpImag->imBits != 4 && bmpImag->imBits != 8 
		    && bmpImag->imBits != 16 && bmpImag->imBits != 24)
	    {
		    /* Error in the bits per pixel, exit with error condition.	*/
		    /* Currently we do not have support for 32BPP images/driver	*/
		    status = NU_INVALID_BPP;
	    }
    }

    if(status == NU_SUCCESS)
    {
	    /* Determine if the color depth is greater than 8 bits. 8 bit mode and less
	    * requires special case to get the color palette and perform a translation
	    * on the image.
	    */
	    if(bmpImag->imBits == 1 || bmpImag->imBits == 4 || bmpImag->imBits == 8)
	    {
            /* Allocate the memory for the palette, 4 bytes per entry. */
            /* BMP8_PALETTE_SIZE = (256 * 4)                           */
		    if((status = GRAFIX_Allocation(&System_Memory, (VOID *)&imPalette, 
                            BMP8_PALETTE_SIZE, NU_NO_SUSPEND)) == NU_SUCCESS)
            {
                /* Set the file pointer to the color palette. */
		        if((status = NU_Seek(fp, 54, PSEEK_SET)) > 0)
                {
		            /* Read the palette into the allocated memory. */
                    if((status = NU_Read(fp, (CHAR *)imPalette, BMP8_PALETTE_SIZE)) == BMP8_PALETTE_SIZE)
                    {
		                /* Allocate the memory for the converted palette, 6 bytes per entry. */
		                if((status = GRAFIX_Allocation(&System_Memory,(VOID *)&newPalette,
                                        (256 * 6), NU_NO_SUSPEND)) == NU_SUCCESS)
                        {
		                    /* Convert the palette to a 16 bit per pixel per color representation. */
                            tempPtr = newPalette;
		                    Convert_Palette(newPalette, imPalette);
                            newPalette = tempPtr;
                        }
                    }
                }
            }
        }
        
        /* If success, then get the actual image data */
        if(status == NU_SUCCESS)
        {
	        /* Set Bytes per row */
	        row_width = bmpImag->imBytes;
            
	        /* Round the length up to the nearest 32 bit boundary. */
            row_length = (((4 - (row_width % 4))) % 4) + row_width;

            /* Calculate the size of image data */
            buf_size = (bmpImag->imHeight) * row_width;

            /* Now allocate memory for imData */
		    if((status = GRAFIX_Allocation(&System_Memory,(VOID *)&bmpImag->imData,
                            buf_size, NU_NO_SUSPEND)) == NU_SUCCESS)
            {

		        tempPtr = bmpImag->imData;

                /* Copy the actual data from the file */
			    for(row = bmpImag->imHeight - 1; row >= 0; row--)
			    {
				    filePointer = bmp_offset + row_length * row;
				    status = NU_Seek(fp, filePointer,PSEEK_SET);
			        status = NU_Read(fp,(CHAR *)tempPtr,row_width);
			        tempPtr = (tempPtr + row_width);
			    }
                
                if(status >= 0)
                {
                    /* Copy success */
                    status = NU_SUCCESS;
                }
            }
        }
        
        /* We have image data. Now do color/image translations, if need be */
        if(status == NU_SUCCESS)
        {
	        /* Allocate the memory for the new image structure that will contain the translated
	        *  image.  This is the actual image structure that will be used when displaying the
	        *  image.
            */
	        if((status = GRAFIX_Allocation(&System_Memory,(VOID *)&newBmpImag, 
                sizeof(image), NU_NO_SUSPEND)) == NU_SUCCESS)
            {
                /* Need special color translation for images having BPP <= 8 */
                if(bmpImag->imBits <= 8)
                {
				    /* Translate the colors based on the converted palette values. */
				    XlateColors( xTable, (unsigned char) bmpImag->imBits, 
				    	        (unsigned char) bmpImag->imPlanes, RGBdata, (palData*)newPalette);

                    imgSize = XlateImage( bmpImag, NU_NULL, pixBits,
                                              pixPlanes,xTable);

                    if((status = GRAFIX_Allocation(&System_Memory, (VOID *)&newBmpImag->imData, 
                                                        imgSize, NU_NO_SUSPEND)) == NU_SUCCESS)
                    {
				    	/* Translate the image. */
				   		XlateImage( bmpImag, newBmpImag, pixBits, 
				    	        	pixPlanes, xTable );
				    }
                }
                else
                {
                    if(pixBits != bmpImag->imBits)
                    {
                        if(pixBits <= 8)
                        {
				            /* Translate the colors based on the converted palette values. */
				            XlateColors( xTable, (unsigned char) bmpImag->imBits,
				    	                (unsigned char) bmpImag->imPlanes, (palData *)RGBdata,
                                        (palData *)system_palette);
                        }

                        imgSize = XlateImage( bmpImag, NU_NULL, pixBits,
                                              pixPlanes,xTable);

                        if((status = GRAFIX_Allocation(&System_Memory, (VOID *)&newBmpImag->imData, 
                                                        imgSize, NU_NO_SUSPEND)) == NU_SUCCESS)
                        {
                            XlateImage( bmpImag, newBmpImag, pixBits, 
                                        pixPlanes, xTable);
                        }
                    }
                    else
                    {
                        /* Deallocate newBmpImag since you do not need */
                        GRAFIX_Deallocation(newBmpImag);

                        /* move the palette to the new image structure. */
				        newBmpImag = bmpImag;
                    }
                }
            }
        }
    }

    /* Are we ready to draw? */
    if(status == NU_SUCCESS)
    {
        /* Yes, so set the rectangle */
        SetRect(&myrect,x_coord,y_coord,
            x_coord+newBmpImag->imWidth,y_coord+newBmpImag->imHeight);

	    /* Draw the image */
        WriteImage(&myrect, newBmpImag);
    }

	/* Start deallocating memory */
	if(imPalette != NU_NULL)
	{
	    GRAFIX_Deallocation(imPalette);
	}
	if(newPalette != NU_NULL)
	{
	    GRAFIX_Deallocation(newPalette);
	}
    if(pixBits!= bmpImag->imBits)
    {
        GRAFIX_Deallocation(newBmpImag->imData);
    }
    if(bmpImag->imData != NU_NULL)
    {
        GRAFIX_Deallocation(bmpImag->imData);
    }
    if(bmpImag != NU_NULL)
    {
        GRAFIX_Deallocation(bmpImag);
    }
    if(newBmpImag != NU_NULL)
    {
        GRAFIX_Deallocation(newBmpImag);
    }
    return status;
}
/* End Display_Bmp_Image */


/********************************************************************************
*                                                                               *
*   FUNCTION                                                                    *
*                                                                               *
*       Get_Bmp_Header                                                          *
*                                                                               *
*   DESCRIPTION                                                                 *
*                                                                               *
*       The function Get_Bmp_Header is used store the bmp image header in       *
*       bmpImg image structure                                                  *
*                                                                               *
*   INPUTS                                                                      *
*                                                                               *
*       INT fp             -   Pointer to the bmp image file                    *
*                                                                               *
*       image *bmpImg           -   Pointer to the image structure where the    *
*                                   image header will be stored                 *
*                                                                               *
*                                                                               *
*   OUTPUTS                                                                     *
*                                                                               *
*       STATUS                  -   Returns NU_SUCCESS if successful.           *
*                                   Returns non-zero number if not successful.  *
*                                                                               *
*********************************************************************************/
STATUS Get_Bmp_Header(INT fp, image *bmpImg)
{
    STATUS status;
    bitmapHeader *bmp_img_hdr = NU_NULL;
    char temp[52];
	UINT8 *temp_ptr;

    if((status = GRAFIX_Allocation(&System_Memory, (VOID *)&bmp_img_hdr,
                sizeof(bitmapHeader), NU_NO_SUSPEND)) == NU_SUCCESS);

	temp_ptr = (UINT8 *)bmp_img_hdr;
    /* Copy the image header details into the bitmapHeader structure */
    NU_Seek(fp, 2, PSEEK_SET);
    if((status = NU_Read(fp, temp, 52)) == 52)
    {
        /* Now, copy the required header details into the image structure */
        bmp_img_hdr = (bitmapHeader *)temp;
		bmpImg->imWidth = bmp_img_hdr->bi_Width;
        bmpImg->imHeight = bmp_img_hdr->bi_Height;
        bmpImg->imAlign = 0;
        bmpImg->imFlags = 0;
        bmpImg->pad = 0;
        bmpImg->imBytes = bmp_img_hdr->bi_Width * (bmp_img_hdr->bi_BitCount/8);
        bmpImg->imBits = bmp_img_hdr->bi_BitCount;
        bmpImg->imPlanes = 1;

        /* bmp_offset is the offset to image data, in bytes*/
        bmp_offset = bmp_img_hdr->bf_OffBits;

        /* Check if the image is compressed */
        if(bmp_img_hdr->bi_Compression == RS_BI_RGB || bmp_img_hdr->bi_Compression == RS_BI_BITFIELDS)
        {
            status = NU_SUCCESS;
        }
        else
        {
            /* Currently we do not support RLE4 and RLE8 compressed bmp images */
            status = NU_NOT_BI_RGB;
        }
    }
    GRAFIX_Deallocation(temp_ptr);

    return status;
}

/****************************************************************************
*                                                                           *
* FUNCTION                                                                  *
*      Convert_Palette													    *
*                                                                           *
* DESCRIPTION                                                               *
*       This function converts the palette from the 3 byte per pixel	    *
*		format to a 3 - 16 bit per pixel format.						    *
*		Palette format:  a single 32bit unit represents a pixel but the	    *
*		4th byte is not used.											    *
*                                                                           *
* INPUTS                                                                    *
*		UINT8*	newPalette			Pointer to store the converted		    *
*									palette.							    *
*		UINT8*  oldPalette			pointer to the original palette		    *
*                                                                           *
* OUTPUTS                                                                   *
*      None														            *
*****************************************************************************/
VOID Convert_Palette( UINT8* newPalette, UINT8* oldPalette )
{

	int		numBytes;
	UINT8	*oldPtr;
	palData	*newPtr;
	int		position;

	newPtr = (palData*)newPalette;
	oldPtr = oldPalette;

	/* Convert the image a row at a time top to bottom. */
	for(numBytes = 0; numBytes < 256; numBytes++)
	{
		/* Since these are byte pointer we must multiply by 4 to get the actual 
		 * position in the palette.  Convert the 8 bit RGB palette entries to a 
		 * 16 bit RGB format -- Our palData structure has 16-bit color entries */
		position = (numBytes * 4);
		newPtr->palBlue = (oldPtr[position] * 256);
		newPtr->palGreen = (oldPtr[position + 1] * 256);
		newPtr->palRed = (oldPtr[position + 2] * 256);

		++newPtr;
	}

}

#endif /* IMG_INCLUDED */
