/****************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
*****************************************************************************

*****************************************************************************
*                                                                           
* FILE NAME                                                                
*                                                                           
*  image.c                                                                
*                                                                           
* DESCRIPTION                                                               
*                                                                           
*  This file contains the RS_Display_Image and related functions.
*                                                                           
* DATA STRUCTURES                                                           
*                                                                           
*  None.                                                                    
*                                                                           
* FUNCTIONS                                                                 
*                                                                           
*    RS_Display_Image                                                       
*    What_Image                                                             
*                                                                           
* DEPENDENCIES                                                              
*                                                                           
*    noatom.h
*    image.h                                                   
*    palette.h
*    rsextern.h
*                                                                           
*****************************************************************************/
#ifdef NU_SIMULATION
#include "ui/noatom.h"
#endif

#include "ui/image.h"

#if (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE)
#include "ui/palette.h"

/* Misc function prototypes */
#include "ui/rsextern.h"

/****************************************************************************************      
*                                                                                       *
*   FUNCTION                                                                            *
*                                                                                       *
*       RS_Display_Image                                                                *
*                                                                                       *
*   DESCRIPTION                                                                         *
*                                                                                       *
*       The function RS_Display_Image is used to images.                                *
*                                                                                       *
*   INPUTS                                                                              *
*                                                                                       *
*       UINT8 *path             -   Location of the image followed by name. If path is  *
*                                   set then bmpImg and img_palette should be NULL.     *
*                                                                                       *
*       image *bmpImg           -   Pointer to the image structure. If bmpImg is set    *
*                                   then path should be NULL.                           *
*                                                                                       *
*       palData *img_palette    -   Pointer to the image palette. Valid palette should  *
*                                   be passed if images quality is less than or equal   *
*                                   to 8BPP.                                            *
*                                                                                       *
*       INT32 x_coord, y_coord  -   These are the coordinates of top-left point of the  *
*                                   image.                                              *
*                                                                                       *
*                                                                                       *
*       UINT flg                -   If 1 - the image is an ICON image                   *
*                                   If 0 - the image is a regular image                 *
*                                                                                       *
*                                                                                       *
*   OUTPUTS                                                                             *
*                                                                                       *
*       STATUS                  -   Returns NU_SUCCESS if successful.                   *
*                                   Returns non-zero number if not successful.          *
*                                                                                       *
*****************************************************************************************/
STATUS RS_Display_Image(CHAR *path, image *bmpImg, palData *img_palette, INT32 x_coord, 
                        INT32 y_coord, UINT32 flg)
{
    grafPort        *scrnport;
    SIGNED          xTable[512];
    image           *newBmpImag=NU_NULL;
    image           *tempImage=NU_NULL;
    rect            myrect;
    STATUS          status = NU_INVALID_ARGUMENTS;
    palData         RGBdata[256];
    INT16           pixBits;
    INT16           pixPlanes;
    UINT32          i;
    UINT32          imgSize;
    UINT32          *temp_ptr = NU_NULL;

#ifdef IMG_INCLUDED

    /* The file pointer */
    INT             fp = 0;
    UINT32          type;
    INT             count = 0;
    UINT32          gif_cnt = 0;
    UINT32          buf_size;
    UNICHAR         type_check[5];
    CHAR            *test_image_buffer=NU_NULL;
    CHAR            filename[NU_MAX_NAME];
    
    /* declare constructs for image display */
    ioSTRUCT    jpeg_io_struct;
    ioSTRUCT    gif_io_struct;

    STR_mem_set(&gif_io_struct, 0, sizeof(ioSTRUCT));
    STR_mem_set(&jpeg_io_struct, 0, sizeof(ioSTRUCT));

#endif

    NU_SUPERV_USER_VARIABLES
        
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    GetPort(&scrnport );
    pixBits   = scrnport->portMap->pixBits;
    pixPlanes = scrnport->portMap->pixPlanes;

    if(path != NU_NULL && bmpImg == NU_NULL && img_palette == NU_NULL)
    {
#ifdef IMG_INCLUDED

        if((fp = NU_Open(path, PO_RDONLY|PO_BINARY, PS_IREAD)) < 0)
        {
            /* Error: Cannot open the file        */
            /* Check pcdisk.h for error details   */
            status = fp;
        }
        else /* File opened successfully */
        {            
            /* Get the size of the file */
            buf_size = (UINT32)NU_Seek(fp, 0, PSEEK_END);

            /* Check the file type to make sure it is a BMP file. */
            type = What_Image(path);

            switch (type)
            {
                case BMP:

                    status = NU_Seek(fp, 0, PSEEK_SET);
                    status = NU_Read(fp, (CHAR *)type_check, 2);

                    type_check[2]='\0';

                    if((STR_str_cmp(type_check,(UNICHAR *)"bm") == NU_SUCCESS) || (STR_str_cmp(type_check,(UNICHAR *)"BM") == NU_SUCCESS))
                    {
                        status = NU_Seek(fp, 0, PSEEK_SET);
                        
                        /* Needed when using an offscreen port */                        
                        SetPort(scrnport);
                                            
                        status = Display_Bmp_Image( fp, x_coord, y_coord);
                    }
                    else
                    {
                        status = NU_INVALID_BMP;
                    }

                    break;

                case GIF:

                    status = NU_Seek(fp, 0, PSEEK_SET);
                    status = NU_Read(fp, (CHAR*)type_check, 3);

                    type_check[3]='\0';

                    if(STR_str_cmp(type_check,(UNICHAR *)"gif")==NU_SUCCESS || STR_str_cmp(type_check,(UNICHAR *)"GIF") == NU_SUCCESS)                    
                    {
                        status = NU_Seek(fp, 0, PSEEK_SET);

                        test_image_buffer = MEM_malloc(buf_size);

                        NU_Read(fp,test_image_buffer,buf_size);

                        gif_io_struct.buf = (UINT8 *)test_image_buffer;
                        gif_io_struct.buf_size = buf_size;
                        gif_io_struct.x = x_coord;
                        gif_io_struct.y = y_coord;
                        gif_io_struct.nodraw = 0;

                        gif_io_struct.animGifNum = -1;

                        /* Needed when using an offscreen port */                        
                        SetPort(scrnport);
                        
                        decompress_gif(&gif_io_struct);
                        GRAFIX_Deallocation(test_image_buffer);
                        if(gif_io_struct.animGifNum != -1)
                        {
                            /* We have animated GIF */
                            /* Now get the name of the file to create a task */
                            while(*(path+count) != '.')
                                ++count;
                            
                            --count;
                            while(count >= 0)
                            {
                                if((*(path+count) == 0x5c) || (*(path+count) == 0x2f))
                                    break;

                                filename[gif_cnt++] = *(path+count);

                                --count;
                            }

                            filename[gif_cnt] = '\0';

                            STR_str_rev((UNICHAR *) filename);

                            Create_Animated_Gif_Task(filename);
                        }

                    }

                    if(gif_io_struct.imagedata != NU_NULL)
                    {
                        GRAFIX_Deallocation(gif_io_struct.imagedata);
                    }

                    break;
                                        
                case JPEG: 

                    status = NU_Seek(fp, 0, PSEEK_SET);
                    status = NU_Seek(fp, 6, PSEEK_SET);
                    status = NU_Read(fp, (CHAR*)type_check, 4);

                    type_check[4]='\0';

                    /* check if this is a JPEG*/
                    if((STR_str_cmp(type_check,(UNICHAR *)"jfif")==NU_SUCCESS || STR_str_cmp(type_check,(UNICHAR *)"JFIF")==NU_SUCCESS)||
                       (STR_str_cmp(type_check,(UNICHAR *)"exif")==NU_SUCCESS || STR_str_cmp(type_check,(UNICHAR *)"Exif")==NU_SUCCESS))
                    {
                        status = NU_Seek(fp, 0, PSEEK_SET);

                        test_image_buffer = MEM_malloc(buf_size);

                        NU_Read(fp,test_image_buffer,buf_size);

                        /* Needed when using an offscreen port */                        
                        SetPort(scrnport);

                        Display_Jpeg_Image(test_image_buffer, &jpeg_io_struct,buf_size,x_coord,y_coord);
                        GRAFIX_Deallocation(test_image_buffer);
                    }

                    break;
                    /* if Not JPEG then EXIT */
                default:
                    status = NU_UNKNOWN_IMAGE;
            }
            
        }/* Else: Close File */

        NU_Close(fp);
        
#endif /* IMG_INCLUDED */

    }
    else if(path == NU_NULL && bmpImg != NU_NULL)
    {
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
    
        newBmpImag = MEM_malloc(sizeof(image));

        if(newBmpImag)
        {
            /* If we got in here, then our allocation must have been successful */
            status = NU_SUCCESS;

            tempImage = MEM_malloc(sizeof(image));

            if(tempImage)
            {
                /* If we got in here, then our allocation must have been successful */
                status = NU_SUCCESS;
				
				newBmpImag->imData = NU_NULL;

                tempImage->imWidth = ((image *)bmpImg)->imWidth;
                tempImage->imHeight = ((image *)bmpImg)->imHeight;
                tempImage->imAlign = ((image *)bmpImg)->imAlign;
                tempImage->imFlags = ((image *)bmpImg)->imFlags;
                tempImage->pad = ((image *)bmpImg)->pad;
                tempImage->imBytes = ((image *)bmpImg)->imBytes;
                tempImage->imBits = ((image *)bmpImg)->imBits;
                tempImage->imPlanes = ((image *)bmpImg)->imPlanes;
                tempImage->imData = (UINT8 *)&(((image *)bmpImg)->imData);

                /* Get Image information from header file */
                /* Check Image BPP value */
                if(tempImage->imBits <= 8)
                {
                    /* Do we have a valid palette */
                    if(img_palette != NU_NULL)
                    {
                        /* Translate the colors based on the converted palette values. */
                        XlateColors( xTable, (UINT8) tempImage->imBits, 
                                     (UINT8) tempImage->imPlanes, RGBdata,img_palette);

                        imgSize = XlateImage( tempImage, NU_NULL,  (UINT8) pixBits,
                                                (UINT8) pixPlanes,xTable);

                        newBmpImag->imData = MEM_malloc(imgSize);

                        if(newBmpImag->imData)
                        {
                            /* If we got in here, then our allocation must have been successful */
                            status = NU_SUCCESS;

                            if(flg == 0)
                            {
                                /* Translate the image so that colors are optimized */
                                XlateImage( tempImage, newBmpImag, (UINT8) pixBits,
                                            (UINT8) pixPlanes, xTable );
                            }
                            else if(flg == 1)
                            {
                                /* This is an ICON image */
                                /* Special translation required for ICON images since they do not use 256 colors */
                                XlateImage( tempImage, newBmpImag, 1,
                                            1, (SIGNED *)img_palette );
                            }
                            else if(flg == 2)
                            {
                                /* This is an ICON image */
                                /* Special translation required for ICON images since they do not use 256 colors */
                                XlateImage( tempImage, newBmpImag, (UINT8) pixBits,
                                            1, (SIGNED *)img_palette );
                            }
                            else
                            {
                                status = NU_INVALID_FLAG;
                            }
                        }
                    }
                    else
                    {
                        /* Images with BPP <= 8 require a palette */
                        status = NU_INVALID_PALETTE;
                    }
                }
                else
                {
                    /* These images do not need a palette */
                    if(tempImage->imBits == 16 || tempImage->imBits == 24 || tempImage->imBits == 32)
                    {
                        /* Check to see if the color quality of driver is same as the image */
                        if(pixBits != tempImage->imBits)
                        {
                            /* If it is not, then check the color quality of the driver */
                            if(pixBits <= 8)
                            {
                                /* If BPP of driver <= 8 then Translation of color is required */
                                /* Translate the colors based on the converted palette values. */
                                XlateColors( xTable, (UINT8) tempImage->imBits, 
                                    (UINT8) tempImage->imPlanes,
                                    RGBdata, system_palette);
                            }

                            imgSize = XlateImage( tempImage, NU_NULL,  (UINT8) pixBits,
                                                    (UINT8) pixPlanes,xTable);

                            newBmpImag->imData = MEM_malloc(imgSize);

                            if(newBmpImag->imData)
                            {
                                /* If we got in here, then our allocation must have been successful */
                                status = NU_SUCCESS;

                                /* Convert the image so that it can be displayed with right colors */
                                XlateImage( tempImage, newBmpImag, (UINT8) pixBits, 
                                            (UINT8) pixPlanes, xTable);
                            }

                        }
                        else
                        {
                            temp_ptr = (UINT32 *)newBmpImag;
                            /* Move the image to the new image structure */
                            newBmpImag = tempImage;
                        }
                    }
                    else
                    {
                        /* Only supported formats: 8, 16, 24 */
                        status = NU_UNKNOWN_IMAGE;
                    }
                }
                if(status == NU_SUCCESS)
                {
                    SetRect(&myrect,x_coord,y_coord,
                        x_coord+newBmpImag->imWidth,y_coord+newBmpImag->imHeight);

                    /* Display the image on the device. */
                    WriteImage(&myrect, newBmpImag);
                    
                }
				
		        /* Deallocate Memory */
		  
		        if((newBmpImag->imData != NU_NULL) && (pixBits != tempImage->imBits))
		        {
		            GRAFIX_Deallocation(newBmpImag->imData);
		            newBmpImag->imData = NU_NULL;
		        }
            }
			
	        if(tempImage == newBmpImag)
	        {
	            GRAFIX_Deallocation(tempImage);
	
	            if(temp_ptr != NU_NULL)
	            {
	                GRAFIX_Deallocation(temp_ptr);
	                newBmpImag = NU_NULL;
	            }
	        }
			else
			{
		        if(tempImage != NU_NULL)
		        {
		            GRAFIX_Deallocation(tempImage);
		        }
		
		        if(newBmpImag != NU_NULL)
		        {
		            GRAFIX_Deallocation(newBmpImag);
		            newBmpImag = NU_NULL;
		        }
			}
        }
    }
    
    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    return status;
}
#endif /* (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE) */

/****************************************************************************
*                                                                           * 
* FUNCTION                                                                  *
*                                                                           *
*      What_Image                                                           *
*                                                                           *
* DESCRIPTION                                                               *
*                                                                           *
*     This function returns the extension of the image.                     *
*                                                                           *
* INPUTS                                                                    *
*                                                                           *
*     CHAR *path   -    Path and name of the image                          *
*                                                                           *
* OUTPUTS                                                                   *
*                                                                           *
*     STATUS       -    If success ,    returns BMP(1) or GIF(2) or JPEG(3) *
*                       If not success, returns NU_UNKNOWN_IMAGE            *
*                                                                           *
*****************************************************************************/
#ifdef IMG_INCLUDED

STATUS What_Image(CHAR *path)
{
    INT i,j;
    CHAR ext[5];
    CHAR bmp[4] ="bmp";
    CHAR bmp1[4]="BMP";
    CHAR gif[4] ="gif";
    CHAR gif1[4]="GIF";
    CHAR jpg[4] ="jpg";
    CHAR jpg1[4]="JPG";
    CHAR jpg2[5]="JPEG";
    CHAR jpg3[5]="jpeg";
    STATUS status;

    i=0;
    j=0;

    while(*(path+i) != '.')
        ++i;

    /* Get the next character*/
    ++i;
    
    /* Copy the extension of the file */
    while(*(path+i) != NU_NULL)
    {
        ext[j] = *(path+i);
        ++j;
        ++i;
    }

    /* Now, make it a string */
    ext[j] = '\0';
    
    /* Compare the string to bmp, gif, jpeg and return appropriate value */
    if((STR_str_cmp((UNICHAR *)ext,(UNICHAR *)bmp)== NU_SUCCESS) || (STR_str_cmp((UNICHAR *)ext,(UNICHAR *)bmp1)== NU_SUCCESS))
    {
        /* It is a BMP image */
        status = BMP;
    }
    else
    {
        if((STR_str_cmp((UNICHAR *)ext,(UNICHAR *)gif)== NU_SUCCESS) || (STR_str_cmp((UNICHAR *)ext,(UNICHAR *)gif1)== NU_SUCCESS))
        {
            /* It is a GIF image */
            status = GIF;
        }
        else
        {
            if((STR_str_cmp((UNICHAR *)ext,(UNICHAR *)jpg)== NU_SUCCESS) || (STR_str_cmp((UNICHAR *)ext,(UNICHAR *)jpg1)== NU_SUCCESS)||
            (STR_str_cmp((UNICHAR *)ext,(UNICHAR *)jpg2)== NU_SUCCESS) || (STR_str_cmp((UNICHAR *)ext,(UNICHAR *)jpg3)== NU_SUCCESS))
            {
                /* It is a JPEG image */
                status = JPEG;
            }
            else
            {
                /* This extension not supported */
                status = NU_UNKNOWN_IMAGE;
            }
        }
    }

    return status;
}

#endif
