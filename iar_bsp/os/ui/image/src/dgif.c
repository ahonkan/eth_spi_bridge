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
*  W3C® SOFTWARE NOTICE AND LICENSE
*  Copyright © 1994-2001 World Wide Web Consortium,
*  (Massachusetts Institute of Technology, Institut National de Recherche 
*  en Informatique et en Automatique, Keio University). 
*  All Rights Reserved. http://www.w3.org/Consortium/Legal/
*  
*  This W3C work (including software, documents, or other related items) 
*  is being provided by the copyright holders under the following license. 
*  By obtaining, using and/or copying this work, you (the licensee) agree 
*  that you have read, understood, and will comply with the following terms 
*  and conditions:
*  
*  Permission to use, copy, modify, and distribute this software and its 
*  documentation, with or without modification, for any purpose and without fee
*  or royalty is hereby granted, provided that you include the following on ALL 
*  copies of the software and documentation or portions thereof, including
*  modifications, that you make:
*  
*  1. The full text of this NOTICE in a location viewable to users of the 
*     redistributed or derivative work.
*  
*  2. Any pre-existing intellectual property disclaimers, notices, or terms and
*     conditions. If none exist, a short notice of the following form (hypertext
*     is preferred, text is permitted) should be used within the body of any 
*     redistributed or derivative code: "Copyright © [$date-of-software] World Wide
*     Web Consortium, (Massachusetts Institute of Technology, Institut National de
*     Recherche en Informatique et en Automatique, Keio University). All Rights 
*     Reserved. http://www.w3.org/Consortium/Legal/"
*  
*  3. Notice of any changes or modifications to the W3C files, including the date 
*     changes were made. (We recommend you provide URIs to the location from which 
*     the code is derived.)
*  
*     THIS SOFTWARE AND DOCUMENTATION IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS
*     MAKE NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
*     LIMITED TO, WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE
*     OR THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY THIRD
*     PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS.
*    
*     COPYRIGHT HOLDERS WILL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL OR 
*     CONSEQUENTIAL DAMAGES ARISING OUT OF ANY USE OF THE SOFTWARE OR DOCUMENTATION.  
*     
*     The name and trademarks of copyright holders may NOT be used in advertising 
*     or publicity pertaining to the software without specific, written prior
*     permission. Title to copyright in this software and any associated 
*     documentation will at all times remain with copyright holders.  
*     
*******************************************************************************

*******************************************************************************
*
*  (c) COPYRIGHT INRIA, 1996-2005
*  Please first read the full copyright statement in file COPYRIGHT.
*
*******************************************************************************

*******************************************************************************
* FILE NAME                                                             
*                                                                      
*       dgif.c                                                           
*                                                                      
* COMPONENT                                                            
*                                                                      
*       GIF - part of GIF workspace                                   
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*       This file contains the routines to decode a .gif and place it 
*       into animated Gif structure if necessary.  Then it will write      
*       the image into the frame buffer.
*                                                                      
* DATA STRUCTURES                                                      
*                                                                      
*      gif_fl_data                     Holds the Gif Data file          
*      Gif_Info                        Holds information about the      
*                                      Gif Header                       
*                                                                      
* FUNCTIONS                                                            
*                                                                      
*      GIF_Read_Info                   Reads the Gif Header, Processes  
*                                      Extensions, Reads Local Image    
*                                      descriptor and processes each    
*                                      individual Gif Frame.                                                                                         
        
*      GIF_Get_Color_Map               Reads the Color Map from each    
*                                      image.                           
*      GIF_Process_Extension_Character Reads the Gif Extensions.        
*      decompress_gif                  Main Calling function to         
*                                      decompress a GIF.                
*      GIF_Get_Block_of_Data           Gets a Block of Data until data  
*                                      Count is Null.                   
*      GIF_Get_Code_From_Data          Gets LWZ code from the data      
*                                      stream.                          
*      GIF_Read_Input_Buffer           Reads the Input buffer and       
*                                      offsets the Buffer Appropriately.
*      GIF_LZW_Byte_Read               Reads LZW byte of data.          
*      GIF_Image_Decode                Reads individual scan lines      
*                                      and handles interlacing.         
*      GIF_GetColorCount               Reads the color Count for the    
*                                      Local Color Table or Global      
*                                      Color Table.                     
*                                                                      
* DEPENDENCIES                                                         
*      image/inc/dgif.h
*
******************************************************************************/

#include "ui/dgif.h"


char dgif_remove_warning;

#ifdef IMG_INCLUDED

#ifdef NU_SIMULATION
#include "ui/noatom.h"
#endif

/* Misc function prototypes */
#include "ui/rsextern.h"

/*  Palette definitions for Colormap */
palData dcpalt[512];
palData dRGBdata[256];
extern palData system_palette[];
imageHeader *getimage;

/*  Global Buffer that holds the image header and Data */
unsigned char *imgbuffer;

/*  Global Value to keep track of Image_Numbers */
int     image_Number;

/*  Global Variable to handle when done decoding */
int    Done_Processing = NU_FALSE;

UINT8 agif_first_time = 0;

/*  Definition of the Gif File data function */
GIF_FILE_DATA *gif_browse_data;

/* Animated GIF entry, usually more then one */
Animated_Entry *Agif;

static struct Gif_Info_Struct Gif_Info;
static struct Gif89A_Struct Gif89A = { -1, -1, -1, 0,0};
static struct Gif89A_Struct prev_Gif89A;


VOID    STR_mem_cpy(VOID *d, VOID *s, INT32 n);

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*     decompress_gif                                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*     Main calling function of decompressing a Gif.  It handles         
*     the decompression of GIFS, and displaying the Image.  If the      
*     Image is an animated Gif only the First Frame is displayed.       
*     The Image is decoded into a Device Independent Bitmap used by     
*     Nucleus Grafix to display the image.                               
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*      thisPage                    Pointer to ioStruct data structure   
*                                  that holds information about the     
*                                  current gif Image to be decoded.     
*                                                                       
* OUTPUTS                                                               
*                                                                       
*      Upon successful completion, the service will return NU_SUCCESS. If the  
*      Service Fails it will return GIF_ERROR_BAD_FILE. 
*      If memory allocation error then status is returned.
*                                                                       
*************************************************************************/
int decompress_gif(ioSTRUCT *thisPage)
{
    
    long            XltTable[COLOR_PALETTE_ARRAY_SIZE];
    long            MonoTable[COLOR_PALETTE_ARRAY_SIZE];
    imageHeader     *srcHdr;
    rect            tempRect;
    int             i; 
    int             status;
    image           *dstPtr     = NU_NULL;
    image           *monoImg    = NU_NULL;
    image           *tempImage  = NU_NULL;
    image			*dgif_image = NU_NULL;
    int             trans_set   = 0;
    long            image_bytes = 0;
    PenSetUp        newPenSetUp;

    if ((!thisPage) || (!thisPage->buf))
    {
        return(GIF_ERROR_BAD_FILE);
    }
    
    /*  Set image Number to be First Image */
    image_Number = 1;

    /*  Is it a Transparent Gif            */
    Gif89A.transparent_gif_color_index = -1;  

    /*  What is the Delay Time to the next image */
    Gif89A.delayTime = -1;                    

    /*  Input Flag                         */
    Gif89A.inputFlag = -1;                    

    /*  Disposal of last image             */
    Gif89A.disposal = 0;                      

    /*  The Image is TransParent           */
    Gif89A.transparent = 0;          
    

    status = GRAFIX_Allocation(&System_Memory,(VOID **)&gif_browse_data,
                             sizeof(GIF_FILE_DATA),NU_NO_SUSPEND);
    
    if(status != NU_SUCCESS)
    {
        /*BRW_Show_Errors(&qdop,status);*/
        return(status);
    }
       
    /*  Set this Page Buf to the File Buffer */
    gif_browse_data->file_buffer = (char *)thisPage->buf;
    
    /*  Retrieve the total bytes */
    gif_browse_data->total_bytes = thisPage->buf_size;
    
    /*  Set the bytes remaining */
    gif_browse_data->bytes_remaining = thisPage->buf_size;
    
    /*  Initially set the current byte to zero */
    gif_browse_data->current_byte = 0;

    /*  Initialize the Number of Images */
    Gif_Info.Num_Images = 0;

    /*  Call the function to interpret to Gif */
    imgbuffer = 0;

    if(GIF_Read_Info(thisPage))
    {   
        /* error in read */
        if(imgbuffer) 
        {
            GRAFIX_Deallocation(imgbuffer);
            imgbuffer = NU_NULL;
        }

        GRAFIX_Deallocation(gif_browse_data); 
        gif_browse_data = NU_NULL;

        return(GIF_ERROR_BAD_FILE);
    }
    for(i=0;i<10;i++)
    {
        dRGBdata[i].palRed   = i;
        dRGBdata[i].palGreen = 0;
        dRGBdata[i].palBlue  = 0;
        
        dRGBdata[i+246].palRed   = i+246;
        dRGBdata[i+246].palGreen = 0;
        dRGBdata[i+246].palBlue  = 0;
 
    }
    
    /* Defined Palette values */
    for(i=10; i<246; i++)
    {
        dRGBdata[i].palRed   = system_palette[i].palRed;
        dRGBdata[i].palGreen = system_palette[i].palGreen;
        dRGBdata[i].palBlue  = system_palette[i].palBlue;
    }    

    /*  If only want to retrieve the size of the image then 
     *  exit. 
     */
    if(thisPage->getSizeofImage == 1)
    {        
        thisPage->ImageWdt = Gif_Info.gif_Width;
        thisPage->ImageHgt = Gif_Info.gif_Height;

        GRAFIX_Deallocation(gif_browse_data);
        return(NU_SUCCESS);
    }

    /* Set the image rectangle */
    thisPage->objRect.Xmin = thisPage->x;
    thisPage->objRect.Ymin = thisPage->y;
    thisPage->objRect.Xmax = thisPage->x + Gif_Info.gif_Width;
    thisPage->objRect.Ymax = thisPage->y + Gif_Info.gif_Height;

    /*  Check if this image is for setting up animated gifs or not or
     *  if the image should be drawn.
     */
    if(thisPage->nodraw==0)
    {

        /*  Build the Translation Table from the image and
         *  system palette.
         */
#if (BPP==8)
        /* If 8 bit mode translate with the system palette */
       /*  read the Palette */
        ReadPalette(0, 0, 256, dRGBdata);

        XlateColors(XltTable, 8, 1, dcpalt, dcpalt);
#else
        /* Must be in 16-bit and 24 - bit modes */ 
        XlateColors(XltTable, 8, 1, dRGBdata, dcpalt);
#endif

        /*  If Gif is transparent then draw it.
         *  Then Set the transparent bit as 0 if there is more than one
         *  image with different Sizes.  Otherwise set the Background if
         *  images are the same size.
         *  
         */
        if(Gif89A.transparent)
        {
            XltTable[Gif89A.transparent_gif_color_index] = (long)0;
            trans_set = 0;

            /*now Copy over to a Monochrome Table and set the Bit Mask Table */
            if(!trans_set)
            {
                for (i = 0;i < COLOR_PALETTE_ARRAY_SIZE; i++)
                {
                    MonoTable[i] = XltTable[i];
                    if(i != Gif89A.transparent_gif_color_index)
                    {
                        MonoTable[i] = 1;
                    }
                    else
                    {
                        MonoTable[i] = 0;
                    }
                }
            }
        }

        if(Gif_Info.Num_Images != 0)
        {
                /*  At this Point Only Show the First Image Slide */
                srcHdr = (imageHeader *) Agif->agif[Agif->NumAGifs]->imgframe[0];

                /*  We Must Modify the Header for Each Animated GIf FRAME */
                if((Agif->agif[Agif->NumAGifs]->imwidth[0] == 0) || (Agif->agif[Agif->NumAGifs]->imwidth[0] > Gif_Info.gif_Width))
                {
                    Agif->agif[Agif->NumAGifs]->imwidth[0] = Gif_Info.gif_Width;
                }
                if((Agif->agif[Agif->NumAGifs]->imheight[0] == 0) || (Agif->agif[Agif->NumAGifs]->imheight[0] > Gif_Info.gif_Height))
                {
                    Agif->agif[Agif->NumAGifs]->imheight[0] = Gif_Info.gif_Height;
                }

                srcHdr->imWidth    = Agif->agif[Agif->NumAGifs]->imwidth[0];
                srcHdr->imHeight   = Agif->agif[Agif->NumAGifs]->imheight[0];
                srcHdr->imRowBytes = Agif->agif[Agif->NumAGifs]->row_bytes[0];

                tempRect = thisPage->objRect;
                Agif->agif[Agif->NumAGifs]->aRect = tempRect;
                Agif->agif[Agif->NumAGifs]->BackGround = Gif_Info.Background;

        
        
                /*  Do the image translation for the src image to get number of bytes */
#if (BPP==8)
                image_bytes = XlateImage((image *)Agif->agif[Agif->NumAGifs]->imgframe[0], NU_NULL, 8, 1, NU_NULL);
#endif
#if (BPP==16)
                image_bytes = XlateImage((image *)Agif->agif[Agif->NumAGifs]->imgframe[0], NU_NULL, 16, 1, NU_NULL);
#endif
#if (BPP==24)
                image_bytes = XlateImage((image *)Agif->agif[Agif->NumAGifs]->imgframe[0], NU_NULL, 24, 1, NU_NULL);
#endif
#if (BPP==32)
                image_bytes = XlateImage((image *)Agif->agif[Agif->NumAGifs]->imgframe[0], NU_NULL, 32, 1, NU_NULL);
#endif
        
                status = GRAFIX_Allocation(&System_Memory,(VOID *)&dstPtr,sizeof(image),NU_NO_SUSPEND);
                if(status != NU_SUCCESS)
                {
                    /*BRW_Show_Errors(&qdop,status);*/
                    return(status);
                }

                status = GRAFIX_Allocation(&System_Memory,(VOID *)&dstPtr->imData,image_bytes,NU_NO_SUSPEND);    
                if(status != NU_SUCCESS)
                {
                    /*BRW_Show_Errors(&qdop,status);*/
                    return(status);
                }

                STR_mem_set(dstPtr->imData, 0, image_bytes);

                status = GRAFIX_Allocation(&System_Memory,(VOID *)&tempImage,sizeof(image),NU_NO_SUSPEND);
                if(status != NU_SUCCESS)
                {
                    return(status);
                }
                tempImage->imWidth = ((image *)Agif->agif[Agif->NumAGifs]->imgframe[0])->imWidth;
                tempImage->imHeight = ((image *)Agif->agif[Agif->NumAGifs]->imgframe[0])->imHeight;
                tempImage->imAlign = ((image *)Agif->agif[Agif->NumAGifs]->imgframe[0])->imAlign;
                tempImage->imFlags = ((image *)Agif->agif[Agif->NumAGifs]->imgframe[0])->imFlags;
                tempImage->pad = ((image *)Agif->agif[Agif->NumAGifs]->imgframe[0])->pad;
                tempImage->imBytes = ((image *)Agif->agif[Agif->NumAGifs]->imgframe[0])->imBytes;
                tempImage->imBits = ((image *)Agif->agif[Agif->NumAGifs]->imgframe[0])->imBits;
                tempImage->imPlanes = ((image *)Agif->agif[Agif->NumAGifs]->imgframe[0])->imPlanes;
                tempImage->imData = (UINT8 *)(&(((image *)Agif->agif[Agif->NumAGifs]->imgframe[0])->imData));

                if(Gif89A.transparent)
                {
                    /*  If image is transparent then create a Monochrome image */
                    Agif->agif[Agif->NumAGifs]->TransParent = 1;

                    status = GRAFIX_Allocation(&System_Memory,(VOID *)&monoImg,
                                             image_bytes,NU_NO_SUSPEND);
                    if (status != NU_SUCCESS)
                    {
                        /*BRW_Show_Errors(&qdop,status);*/
                        return(status);
                    }
                    status = GRAFIX_Allocation(&System_Memory,(VOID *)&monoImg->imData,
                                             image_bytes,NU_NO_SUSPEND);
                
                    if (status != NU_SUCCESS)
                    {
                        /*BRW_Show_Errors(&qdop,status);*/
                        return(status);
                    }

                    STR_mem_set(monoImg->imData, 0, image_bytes);

                    /*  Then translate the image into a monochrome image */
                    XlateImage(tempImage, monoImg, 1, 1, MonoTable);

                }

#if (BPP==8)
                /*  Then create the actual 8-bit image */
                XlateImage(tempImage, dstPtr, 8, 1, XltTable);
#endif
#if (BPP==16)
                /*  Then create the actual 16-bit image */
                XlateImage(tempImage, dstPtr, 16, 1, XltTable);
#endif
#if (BPP==24)
                /*  Then create the actual 24-bit image */
                XlateImage(tempImage, dstPtr, 24, 1, XltTable);
#endif
#if (BPP==32)
                /*  Then create the actual 24-bit image */
                XlateImage(tempImage, dstPtr, 32, 1, XltTable);
#endif

                /*  Setup the rectangle with offsets */
                tempRect.Xmin += Agif->agif[Agif->NumAGifs]->x_origin[0];
                tempRect.Xmax = tempRect.Xmin+srcHdr->imWidth;
                tempRect.Ymin += Agif->agif[Agif->NumAGifs]->y_origin[0];
                tempRect.Ymax = tempRect.Ymin +srcHdr->imHeight;

                /*  Initialize the current Frame */
                Agif->agif[Agif->NumAGifs]->CurrentFrame = 0;

                /*  Increment Number of Animated Gifs */
                thisPage->animGifNum = Agif->NumAGifs;
                thisPage->type = AGIF;
                Agif->NumAGifs += 1;
        }
        else
        {
                /*  If image is not animated then just do the 
                 *  image translation to get number of bytes.
                 */
                thisPage->type = GIF;
#if (BPP==8)
                image_bytes = XlateImage((image *)imgbuffer, NU_NULL, 8, 1, NU_NULL);
#endif
#if (BPP==16)
                image_bytes = XlateImage((image *)imgbuffer, NU_NULL, 16, 1, NU_NULL);
#endif
#if (BPP==24)
                image_bytes = XlateImage((image *)imgbuffer, NU_NULL, 24, 1, NU_NULL);
#endif
#if (BPP==32)
                image_bytes = XlateImage((image *)imgbuffer, NU_NULL, 32, 1, NU_NULL);
#endif
                /*  get a Temp Rectangle */
                tempRect = thisPage->objRect;
                
                srcHdr = (imageHeader *)imgbuffer;

                status = GRAFIX_Allocation(&System_Memory,(VOID *)&dstPtr,sizeof(image),NU_NO_SUSPEND);
                if(status != NU_SUCCESS)
                {
                    return(status);
                }

                status = GRAFIX_Allocation(&System_Memory,(VOID *)&dstPtr->imData,image_bytes,NU_NO_SUSPEND);    
                if(status != NU_SUCCESS)
                {
                    return(status);
                }
                status = GRAFIX_Allocation(&System_Memory,(VOID *)&tempImage,sizeof(image),NU_NO_SUSPEND);
                if(status != NU_SUCCESS)
                {
                    return(status);
                }
                tempImage->imWidth = ((image *)imgbuffer)->imWidth;
                tempImage->imHeight = ((image *)imgbuffer)->imHeight;
                tempImage->imAlign = ((image *)imgbuffer)->imAlign;
                tempImage->imFlags = ((image *)imgbuffer)->imFlags;
                tempImage->pad = ((image *)imgbuffer)->pad;
                tempImage->imBytes = ((image *)imgbuffer)->imBytes;
                tempImage->imBits = ((image *)imgbuffer)->imBits;
                tempImage->imPlanes = ((image *)imgbuffer)->imPlanes;
                tempImage->imData = (UINT8 *)(&(((image *)imgbuffer)->imData));

#if (BPP==8)
                /*  Translate the image as needed */
                XlateImage(tempImage, dstPtr, 8, 1, XltTable);
#endif
#if (BPP==16)
                /*  Translate the image as needed */
                XlateImage(tempImage, dstPtr, 16, 1, XltTable);
#endif
#if (BPP==24)
                /*  Translate the image as needed */
                XlateImage(tempImage, dstPtr, 24, 1, XltTable);
#endif
#if (BPP==32)
                /*  Translate the image as needed */
                XlateImage(tempImage, dstPtr, 32, 1, XltTable);
#endif


                if(Gif89A.transparent && trans_set == 0)
                {
                    status = GRAFIX_Allocation(&System_Memory,(VOID *)&monoImg,
                                             sizeof(image),NU_NO_SUSPEND);
                    if (status != NU_SUCCESS)
                    {
                        /*BRW_Show_Errors(&qdop,status);*/
                        return(status);
                    }
                    status = GRAFIX_Allocation(&System_Memory,(VOID *)&monoImg->imData,
                                             image_bytes,NU_NO_SUSPEND);
                
                    if (status != NU_SUCCESS)
                    {
                        /*BRW_Show_Errors(&qdop,status);*/
                        return(status);
                    }

                    STR_mem_set(monoImg->imData, 0, image_bytes);

                    /*  Then So monochrome image translation */
                    XlateImage(tempImage, monoImg, 1, 1, MonoTable);
                }
                /* This is a not an animated GIF so set animGifNum to -1 */
                thisPage->animGifNum = -1;
        } 
        if(Gif89A.transparent && (thisPage->animGifNum != -1))
        {
        	status = GRAFIX_Allocation(&System_Memory,(VOID *)&dgif_image,sizeof(image),NU_NO_SUSPEND);
        	if(status != NU_SUCCESS)
        	{
        		return(status);
        	}

        	status = GRAFIX_Allocation(&System_Memory,(VOID *)&dgif_image->imData,image_bytes,NU_NO_SUSPEND);    
        	if(status != NU_SUCCESS)
        	{
            	return(status);
        	}
        	ReadImage(&tempRect, dgif_image);
        }
        if((Gif89A.transparent) && (trans_set == 0))
        {
            /*  Set the PenColor to Black for the Rasterop foreground*/
            RS_Get_Pen_Setup(&newPenSetUp);
            RS_Pen_Setup(&newPenSetUp, BRW_MONOCHROME_FCOLOR);

            /*  Set up the Monochrome image Rasterop */
            RasterOp(xREPx);

            /*  Draw the Monochrome Image */
            WriteImage(&tempRect, monoImg);

            /*  Now XOR the Real image over Monochrome Image with RasterOp
             *  RasterOp(zXORz).
             */
            RasterOp(zXORz);

            /*  Write the Image */
            WriteImage(&tempRect, dstPtr);
            
            /*  Restore the RasterOP as normal */
            RasterOp(zREPz);

            /*  Free the monoimg */
            GRAFIX_Deallocation(monoImg->imData);
            GRAFIX_Deallocation(monoImg);
        }
        else
        {
        	WriteImage(&tempRect, dstPtr);
        }
        
         /* Now we need to store all of the Information in the
          * This Page Rect.
          */

        if(Gif89A.transparent)
        {
            thisPage->transparent_index = Gif89A.transparent_gif_color_index;
            thisPage->transparent = 1;
            RasterOp(zREPz);
            if(thisPage->animGifNum != -1)
            {
	            WriteImage(&tempRect,dgif_image);
	            GRAFIX_Deallocation(dgif_image->imData);
	            GRAFIX_Deallocation(dgif_image);
         	}
        }
        else
        {
            thisPage->transparent = 0;
            thisPage->transparent_index = 0;
        }
        thisPage->grayscale = 0;

        for(i = 0;i < thisPage->num_colors && i < COLOR_PALETTE_ARRAY_SIZE; i++)
        {
            thisPage->cpalt[i]=dcpalt[i];
        }

        thisPage->imagedata = imgbuffer;
        thisPage->imgsize = Gif_Info.gif_Height *(Gif_Info.gif_Width * (Gif_Info.Bits_Per_Pixel >> 3)) + sizeof(image);

        /*  Set the Trans Set */
        thisPage->trans_set = trans_set;

        /* Set the Num Images */
        thisPage->num_images = Gif_Info.Num_Images;
        if(thisPage->type != AGIF)
        {
          thisPage->animGifNum = -1;
        }
     }
     else
     {
         /*  Just store the image data because it is not to be 
          *  displayed.
          */
                          
        if(Gif_Info.Num_Images!= 0)
        {
            thisPage->type = AGIF;
        }
        else
        {
            thisPage->type = GIF;
        }

        if(Gif89A.transparent)
        {
            thisPage->transparent_index = Gif89A.transparent_gif_color_index;
            thisPage->transparent = 1;
        }
        else
        {
            thisPage->transparent = 0;
            thisPage->transparent_index = 0;
        }
        thisPage->grayscale = 0;

        for(i = 0;i < thisPage->num_colors && i < COLOR_PALETTE_ARRAY_SIZE; i++)
        {
            thisPage->cpalt[i] = dcpalt[i];
        }

        thisPage->imagedata = imgbuffer;
        thisPage->imgsize = Gif_Info.gif_Height *(Gif_Info.gif_Width * (Gif_Info.Bits_Per_Pixel >>3))+sizeof(image);

        /*  Set the Trans Set */
        thisPage->trans_set = trans_set;

        /* Set the Num Images */
        thisPage->num_images = Gif_Info.Num_Images;

        if(thisPage->type != AGIF)
        {
          thisPage->animGifNum = -1;
        }
     }

     /* clean up */
     if(tempImage!=NU_NULL)
     {
         GRAFIX_Deallocation(tempImage);
     }
     
     GRAFIX_Deallocation(gif_browse_data);
     
     if(thisPage->nodraw == 0)
     {
         GRAFIX_Deallocation(dstPtr->imData);
         GRAFIX_Deallocation(dstPtr);
     }

     Gif89A.transparent = 0;
     Gif_Info.Background = 0;
      
     return(NU_SUCCESS);

} /* decompress_gif */

/*************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*     GIF_Read_Info                                                  
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*      This service reads the Global Gif information Header, the Gif    
*      extensions, the Local Gif Information, Global Color Table, Local 
*      Color Table, and the individual scan lines for the current Image 
*      frame. The function also files out the animated Gif         
*      structure if the Image is animated.                              
*                                                                       
* INPUTS                                                                
*                                                                       
*      thisPage                    Pointer to ioStruct data structure   
*                                  that holds information about the     
*                                  current gif Image to be decoded.     
* OUTPUTS                                                              
*
*      Upon successful completion, the service will return NU_SUCCESS. If the  
*      Read Fails it will return one of the ERROR MACROS.                      
*                                                                       
*************************************************************************/
int GIF_Read_Info(ioSTRUCT *thisPage)
{
    /*  The InPut Buffer Used for Reading Data          */
    unsigned char in_buf[16];           

    /*  Code Character for reading Codes about data     */
    unsigned char code_ch;              
    unsigned char *tempbuf;             

    /*  Flag for if Global Color Map Used               */
    unsigned char Global_Color_Map_Used;          

    /*  Flag for if Local Color Map Used                */
    unsigned char Local_Color_Map_Used;           

    /*  Bits Per Pixel                                  */
    int bits_Per_Pixel;           

    /*  Number of Bytes Per Row.                        */
    long row_bytes;                  

    /*  Counter used to keep track of animated Gif count.*/
    int image_Count = 1;               

    /*  Used to Read the Gif Version from the Global Gif Header */
    UNICHAR gif_version[4];
    UNICHAR gif_ver1[] ="87a";
    UNICHAR gif_ver2[] ="89a";
    UNICHAR gif_head[] ="GIF";

    /*  Used to determine if Global color Palette is set    */
    int global_color_palette_set=0;     

    /*  Misc variables.                                     */
    int num_colors;
    int i;                   
    int status;

    /*  Used to determine x_origin of the image from the */
    short x_origin;                     

    /*  Local Image descriptor.                          */                                    
    /*  Used to determine the y_origin of the image from */
    short y_origin;                     

    /*  The image width from the Local Image Descriptor.*/
    int imwidth;      

    /*  The image height from the Local Image Descriptor.*/
    int imheight;       

    

    /*  Initialize local variables */
    global_color_palette_set = 0;
    Global_Color_Map_Used = 0;
    Local_Color_Map_Used = 0;
    image_Count = 1;

    STR_mem_set(in_buf,0,16);

    /*  Find out what type of GIF File We Have  */
    if (GIF_Read_Input_Buffer(in_buf,6))
    {
        return(GIF_ERROR_BAD_FILE);
    }
    
    /*  Compare to make sure input is a GIF File */
    if (STR_strn_cmp((UNICHAR *)in_buf,(UNICHAR *)gif_head,3) != 0)
    {
        return(GIF_ERROR_BAD_TYPE);
    }
    
    /*  Get the Version of the GIF File by passing past the GIF */
    STR_strn_cpy(gif_version, (UNICHAR *)in_buf + 3, 3);

    /*  Null Terminate the version */
    gif_version[3] = '\0';
    
    if ((STR_str_cmp(gif_version, (UNICHAR *)gif_ver1) != 0) && (STR_str_cmp(gif_version, (UNICHAR *)gif_ver2) != 0))
    {
        return(GIF_ERROR_BAD_TYPE);     
    }
    
    /*  Get GIF Pict information from reading the GIF Header */
    if (GIF_Read_Input_Buffer(in_buf,7))
    {
        return(GIF_ERROR_BAD_FILE);
    }
    
    /*  Get the Gif Width */
    Gif_Info.gif_Width          = GET_UNSIGNI(in_buf);
    
    /*  Get the Gif Height */
    Gif_Info.gif_Height         = GET_UNSIGNI(in_buf+2);
    
    /* flags */
    /*  Get the Bits Per Pixel bits 4-6   1[011]0011 */
    Gif_Info.Bits_Per_Pixel     = 8;
    
    /*                                      76543 210 */
    /*  Get the Color Resolution bits 0-2   10110[011] */
    Gif_Info.Color_Resol        = (((in_buf[4]&0x70)>>3)+1);
    
    /*  Get the GIF Background Color */
    Gif_Info.Background         = in_buf[5];
    
    /*  Get the GIF Aspect Ratio */
    Gif_Info.Aspect_Ratio       = in_buf[6];
    
    /*  Get the Number of BYTES per ROW */
    row_bytes = (Gif_Info.gif_Width * (Gif_Info.Bits_Per_Pixel >>3));
    
    if(thisPage->getSizeofImage == 1)
    {
        return(NU_SUCCESS);
    }
    
    /*  Allocate the Memory for the output buffer for use with META Grafix's */
    /*  Allocate enough memory for the decompressed image + 12 bytes for the META Grafix's image header */
    status = GRAFIX_Allocation(&System_Memory,(VOID *)&imgbuffer,
                             (Gif_Info.gif_Height * row_bytes)+ sizeof(image),NU_NO_SUSPEND);

    if(status != NU_SUCCESS)
    {
        /*BRW_Show_Errors(&qdop,status);*/
        return(status);
    }

    /*  Initialize the output put to all 0's */
    STR_mem_set(imgbuffer, 0, (Gif_Info.gif_Height * row_bytes) + sizeof(image));
    
    /*  Set the Image Buffer pointer to the Get image variable(Holds the image header */
    getimage= (imageHeader *)imgbuffer;
    
    /*  Fill out the image header */
    /*  Set the Image Global Screen Width */
    getimage->imWidth = Gif_Info.gif_Width;

    /*  Set the Image Global Screen Height */
    getimage->imHeight = Gif_Info.gif_Height;

    /*  Set the Images Alignment (Set to 0 now but render should show set field )*/
    getimage->imAlign = 0;

    /*  Set the Flags to 0 (will be updated with lower grafix functions) */
    getimage->imFlags = 0;

    /*  With Nucleus Grafix's planes should be a 1 */
    getimage->imPlanes = 1;
    
    /*  Set the Number of Bits per Pixel */
    getimage->imBits = Gif_Info.Bits_Per_Pixel;
    
    /*  Set the number of bytes per row */
    getimage->imRowBytes =(short)row_bytes;
    
    /*  Get the Color Count for the Color Map  */
    num_colors = GIF_GetColorCount(in_buf[4]);

    thisPage->num_colors = num_colors;
    
    /*  Check if the GLOBAL Color Map should be used */
    /*  If so the get the Global Color Map */
    if (IsBitSet(in_buf[4], IS_GLOBALCOLORMAP)) 
    {   
        Global_Color_Map_Used = 1;

        /* Get the Color Map and Fill out the Global Data Structure for the Meta Grafix's Color Map*/
        if (GIF_Get_Color_Map(num_colors))
        {
            return(GIF_COLOR_MAP_ERROR);
        }
    }
    
    
    /*  Now read the data for each individual frame and exit when done so the
     *  image can be dealt with as needed.
     */
    
    for(;;)
    {
        /*  Read One Input Character */
        if (GIF_Read_Input_Buffer(&code_ch,1))
        {
            return(GIF_ERROR_BAD_FILE);
        }

        /*  If code is an extension then process the extension. */
        if (code_ch == GIF_EXTENSION) 
        {    /* Extension */
            if((STR_str_cmp(gif_version, (UNICHAR *)gif_ver1) == 0) && image_Count !=1)  
            {
                continue;
            }

            if (GIF_Read_Input_Buffer(&code_ch,1))
            {
                return(GIF_ERROR_BAD_FILE);
            }
            
            GIF_Process_Extension_Character(code_ch);

            continue;
        }
        
        if (code_ch != GIF_START && code_ch != GIF_TERMINATE) 
        {    
        /*  If Character was not 0x2c then it is invalid and will skip over the rest of the
         *  Characters until it receives a 0x2c or end character.
         */
            continue;
        }
        
        /*  If image count is greater than image number then we have an
         *  animated GIF.
         */
        if(image_Count != image_Number)
        {
            if((code_ch == GIF_TERMINATE) && (image_Count <= 2))
            {
                return(NU_SUCCESS);
            }
            if(agif_first_time == 0)
            {
                init_animated_Gif_Entry();
                agif_first_time = 1;
            }            
            /*  We Now have an animated Gif with more than one frame */
            /*  We need to copy the animated Gif Data over to the iostruct data structure */
            if(!(Agif->agif[Agif->NumAGifs]))
            {
                status = GRAFIX_Allocation(&System_Memory,(VOID **)&(Agif->agif[Agif->NumAGifs]),
                                          (sizeof(Animated_GIF)),NU_NO_SUSPEND);
                if(status != NU_SUCCESS)
                {
                    return(status);
                }

                init_animated_Gif(Agif->NumAGifs);
            }

            status = GRAFIX_Allocation(&System_Memory,(VOID **)&(Agif->agif[Agif->NumAGifs]->imgframe[Agif->agif[Agif->NumAGifs]->NumFrames]),
                                     (Gif_Info.gif_Height * row_bytes )+sizeof(image),NU_NO_SUSPEND);

            if(status != NU_SUCCESS)
            {
                return(status);
            }

            STR_mem_set(Agif->agif[Agif->NumAGifs]->imgframe[Agif->agif[Agif->NumAGifs]->NumFrames],0,(Gif_Info.gif_Height * row_bytes)+sizeof(image));

            STR_mem_cpy(Agif->agif[Agif->NumAGifs]->imgframe[Agif->agif[Agif->NumAGifs]->NumFrames],imgbuffer,(Gif_Info.gif_Height * row_bytes)+sizeof(image));
            
            /*  Store off the Origin for the Animated Gif */
            Agif->agif[Agif->NumAGifs]->x_origin[Agif->agif[Agif->NumAGifs]->NumFrames]  = x_origin;
            Agif->agif[Agif->NumAGifs]->y_origin[Agif->agif[Agif->NumAGifs]->NumFrames]  = y_origin;
            Agif->agif[Agif->NumAGifs]->imwidth[Agif->agif[Agif->NumAGifs]->NumFrames]   = imwidth;
            Agif->agif[Agif->NumAGifs]->imheight[Agif->agif[Agif->NumAGifs]->NumFrames]  = imheight;                                
            Agif->agif[Agif->NumAGifs]->row_bytes[Agif->agif[Agif->NumAGifs]->NumFrames] = (short) row_bytes;

            Agif->agif[Agif->NumAGifs]->transparent_Index[Agif->agif[Agif->NumAGifs]->NumFrames] = prev_Gif89A.transparent_gif_color_index;
            Agif->agif[Agif->NumAGifs]->delay[Agif->agif[Agif->NumAGifs]->NumFrames]     = (short)prev_Gif89A.delayTime;
            Agif->agif[Agif->NumAGifs]->transparent[Agif->agif[Agif->NumAGifs]->NumFrames] = prev_Gif89A.transparent;
            Agif->agif[Agif->NumAGifs]->disposal[Agif->agif[Agif->NumAGifs]->NumFrames] = prev_Gif89A.disposal;

            /*  Compare If Size Difference */
            if(Agif->agif[Agif->NumAGifs]->NumFrames != 0)
            {
                if(Agif->agif[Agif->NumAGifs]->x_origin[Agif->agif[Agif->NumAGifs]->NumFrames-1] > 0 ||
                    Agif->agif[Agif->NumAGifs]->y_origin[Agif->agif[Agif->NumAGifs]->NumFrames-1] > 0)
                {
                    if((Agif->agif[Agif->NumAGifs]->x_origin[Agif->agif[Agif->NumAGifs]->NumFrames-1] >
                       Agif->agif[Agif->NumAGifs]->x_origin[Agif->agif[Agif->NumAGifs]->NumFrames]) ||
                       (Agif->agif[Agif->NumAGifs]->x_origin[Agif->agif[Agif->NumAGifs]->NumFrames-1] <
                       Agif->agif[Agif->NumAGifs]->x_origin[Agif->agif[Agif->NumAGifs]->NumFrames]) ||
                       (Agif->agif[Agif->NumAGifs]->y_origin[Agif->agif[Agif->NumAGifs]->NumFrames-1] >
                       Agif->agif[Agif->NumAGifs]->y_origin[Agif->agif[Agif->NumAGifs]->NumFrames]) ||
                       (Agif->agif[Agif->NumAGifs]->y_origin[Agif->agif[Agif->NumAGifs]->NumFrames-1] <
                       Agif->agif[Agif->NumAGifs]->y_origin[Agif->agif[Agif->NumAGifs]->NumFrames]))
                    {
                       if((Agif->agif[Agif->NumAGifs]->y_origin[Agif->agif[Agif->NumAGifs]->NumFrames-1] >
                       Agif->agif[Agif->NumAGifs]->y_origin[Agif->agif[Agif->NumAGifs]->NumFrames]) ||
                       (Agif->agif[Agif->NumAGifs]->y_origin[Agif->agif[Agif->NumAGifs]->NumFrames-1] <
                       Agif->agif[Agif->NumAGifs]->y_origin[Agif->agif[Agif->NumAGifs]->NumFrames]))
                       {
                           Agif->agif[Agif->NumAGifs]->Size_difference = 1;
                       }
                    }

                }
            }
            
            if((Global_Color_Map_Used == 1) && (global_color_palette_set != 1))
            {
                
                Agif->agif[Agif->NumAGifs]->GCT = 1;

                /*  Copy the Global Color Palette */
                for(i = 0; i < COLOR_PALETTE_ARRAY_SIZE; i++)
                {
                    Agif->agif[Agif->NumAGifs]->Global_Ct[i] = dcpalt[i];
                }
                global_color_palette_set = 1;
                
            }
            
            /*  Set the Local Color Palette */
            if(Local_Color_Map_Used)
            {
                status = GRAFIX_Allocation(&System_Memory,(VOID **)&(Agif->agif[Agif->NumAGifs]->Lct[Agif->agif[Agif->NumAGifs]->NumFrames]),
                                     (sizeof(LCT_STRUCT)),NU_NO_SUSPEND);

                if(status != NU_SUCCESS)
                {
                    return(status);
                }

                Agif->agif[Agif->NumAGifs]->LCT[Agif->agif[Agif->NumAGifs]->NumFrames] = 1;
                for(i = 0; i < COLOR_PALETTE_ARRAY_SIZE; i++)
                {
                    Agif->agif[Agif->NumAGifs]->Lct[Agif->agif[Agif->NumAGifs]->NumFrames]->Locol_Ct[i]
                        = dcpalt[i];
                }
                
            }

            Agif->agif[Agif->NumAGifs]->NumFrames += 1;
            Gif_Info.Num_Images += 1;                
            image_Number += 1;       

            if(code_ch != GIF_TERMINATE)
            {
                /*  Reset the Image Buffer */
                tempbuf = imgbuffer + sizeof(image);
                STR_mem_set(tempbuf, 0, (Gif_Info.gif_Height * row_bytes));
            }
            else
            {
                return(NU_SUCCESS);
            }
        }
      
        
        if (GIF_Read_Input_Buffer(in_buf,9))
        {
            return(GIF_ERROR_BAD_FILE);
        }
        
        /*  We Need to get the Local Data Information for this animated GIF */
        /*  Get this Values X origin */
        x_origin =     GET_UNSIGNI(in_buf);

        /*  Get This Values Y origin  */
        y_origin =    GET_UNSIGNI(in_buf+2);

        /*  Get This Images Width    */
        imwidth =   GET_UNSIGNI(in_buf+4);
        
        /*  Get This Images Height  */
        imheight =  GET_UNSIGNI(in_buf+6);

        if(imwidth > Gif_Info.gif_Width)
        {
            imwidth = Gif_Info.gif_Width;
        }

        if(imheight > Gif_Info.gif_Height)
        {
            imheight = Gif_Info.gif_Height;
        }
        
        /*  Recalculate the Number of Row Bytes for this Image Frame */
        row_bytes = (imwidth * (Gif_Info.Bits_Per_Pixel >>3));
        
        getimage->imRowBytes = (short) row_bytes;
        
        Local_Color_Map_Used =  (in_buf[8] & IS_LOCALCOLORMAP);
        
        
        bits_Per_Pixel = 8;
        num_colors = GIF_GetColorCount(in_buf[8]);
        
        /* save Gif settings for further use */
       	memcpy(&prev_Gif89A, &Gif89A, sizeof(Gif89A));

        /*  If local Color Map is used then get the Local Color Map
         *  and decode the Gif frame.
         */
        if ( Local_Color_Map_Used) 
        {
            if (GIF_Get_Color_Map(num_colors))
            {
                return(GIF_COLOR_MAP_ERROR);            
            }
                        
            GIF_Image_Decode(imwidth, imheight, bits_Per_Pixel,
                             in_buf[8]&IS_INTERLACE, 
                             image_Number!=image_Count);
            /*  Increment Image_Count */
            image_Count++;
        } 
        else 
        {
            /*  Else decode the Gif and increment the image Count */
            GIF_Image_Decode(imwidth, imheight,
                             Gif_Info.Bits_Per_Pixel,
                             in_buf[8]&IS_INTERLACE,
                             image_Number != image_Count);

            /*  Increment image_Count */
            image_Count++;
        }
    }
}

/*************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*     GIF_Get_Color_Map                                                 
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*     This function will read the color map from the src gif data.      
*                                                                       
* INPUTS                                                                
*                                                                       
*      number_entries              Number of colors in the map
*
* OUTPUTS                                                              
*
*      Upon successful completion, the service will return NU_SUCCESS. If the  
*      Read Fails it will return one of the GIF_ERROR_BAD_FILE.                      
*                                                                       
*                                                                       
*************************************************************************/
int GIF_Get_Color_Map ( int number_entries)
{
    int i;
    unsigned char rgb_color[3];

    for (i = 0; i < number_entries && i < COLOR_PALETTE_ARRAY_SIZE; ++i) 
    {
        if (GIF_Read_Input_Buffer(rgb_color, sizeof(rgb_color)))
        {
            return(GIF_ERROR_BAD_FILE);         
        }
        
        /*  Write the Palette to a Meta Grafix's palette structure */
        dcpalt[i].palRed =   rgb_color[0] << 8; /*  Get red color value   */
        dcpalt[i].palGreen = rgb_color[1] << 8; /*  Get Green color value */
        dcpalt[i].palBlue =  rgb_color[2] << 8; /*  Get Blue color */
    }

    return(NU_SUCCESS);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*     GIF_Process_Extension_Character                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*     This routine will process a GIF header extension.                 
*                                                                       
*                                                                       
*      extension_label             header extension label coming in
*
* OUTPUTS                                                              
*
*                                                                       
*************************************************************************/
void GIF_Process_Extension_Character( int extension_label )
{
    static unsigned char    buf[256];
    
    switch (extension_label) 
    {
       /*  Process Plain Text Extension */
    case GIF_PLAIN_TEXT_EXT:        
        /*  At this Time Do nothing */
        break;

        /*  Process Application extension */
    case GIF_APP_EXT:               
        /*  At this Time Do Nothing */
        break;

        /* Process Comment Extension */
    case GIF_COMMENT_EXT:       
        /*  Get the Gif Comment */
        while (GIF_Get_Block_of_Data((unsigned char*) buf) != 0) 
        {
            ;
        }
        return;

        /*  Process Graphic Control extension for Gif 89A's */
    case GIF_GRAPHIC_CTRL_EXT:
        
        (void) GIF_Get_Block_of_Data((unsigned char*) buf);
        Gif89A.disposal    = (buf[0] >> 2) & 0x7;
        Gif89A.inputFlag   = (buf[0] >> 1) & 0x1;
        Gif89A.delayTime   = GET_UNSIGNI(buf+1);
        Gif89A.transparent  = buf[0] & 0x1;

        if ((buf[0] & 0x1) != 0)
        {
            Gif89A.transparent_gif_color_index = buf[3];
        }

        while (GIF_Get_Block_of_Data((unsigned char*) buf) != 0)
        {
            ;
        }
        return;

    default:
        /*  Pass through for now */
        break;
    }

    /*  Pass though Next Block of Data if Default case was taken and then return */
    while (GIF_Get_Block_of_Data((unsigned char*) buf) != 0)
    {
        ;
    }

    return;
}


/*************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*     GIF_Get_Code_From_Data                                            
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*     Traverses the input data for a LZW code.                          
*                                                                       
* INPUTS                                                                
*                                                                       
*      code_size              - Get what size of code requested
*      initialize             - Flag that indicates whether the code 
*                               grabber has been initialized
*
* OUTPUTS                                                              
*
*      Upon successful completion, the service will return NU_SUCCESS. If the  
*      Complete and it overflows it will return GIF_OVERFLOW. 
*      If Complete and no overflow then -1 is returned to get out of while loop
*      Otherwise the retval is returned which is the return code.
*                                                                       
*************************************************************************/
int GIF_Get_Code_From_Data ( int code_size, int initialize )
{
    static unsigned char    decode_buf[280];
    static int              current_bit; 
    static int              last_bit;
    static short            complete;
    static int              last_byte;
    int                     i;
    int                     j;
    int                     retval = 0;
    unsigned char           code_count;

    /*  Initialize the Code grabber */
    if (initialize) 
    {
        current_bit = 0;
        last_bit = 0;
        complete = NU_FALSE;

        return (NU_SUCCESS);
    }

    /*  Check if the Current bit + code size if greater than the last_bit */
    if ( (current_bit+code_size) >= last_bit) 
    {
        /*  Check if Complete */
        if (complete) 
        {
            /*  If complete verify that we did not have an overflow */
            if (current_bit > last_bit)
            {
                return(GIF_OVERFLOW);
            }

            /*  Then return -1 to break out of get image While loop */
            return (-1);
        }

        decode_buf[0] = decode_buf[last_byte-2];
        decode_buf[1] = decode_buf[last_byte-1];

        /*  See if count is equal to 0 if so return */
        if ((code_count = GIF_Get_Block_of_Data(&decode_buf[2])) == 0)
        {
            complete = NU_TRUE;
        }

        /*  Otherwise increment the last byte to 2 plus the value of count */
        last_byte = 2 + code_count;

        /*  Get the Current Bit and Last Bit */
        current_bit = (current_bit - last_bit) + 16;
        last_bit = (2+code_count)*8 ;
    }

    /*  Get the Return Code */
    for (i = current_bit, j = 0; j < code_size; ++i, ++j)
    {
        retval |= ((decode_buf[ i / 8 ] & (1 << (i % 8))) != 0) << j;
    }

    /*  Increment the current_bit by the size of the code that is being returned */
    current_bit += code_size;

    return (retval);
}

/*************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*     GIF_LZW_Byte_Read                                                     
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*     Reads data and decodes it using LZW decompression.                
*                                                                       
* INPUTS                                                                
*                                                                       
*      input_code_size        - Get what size of code requested
*      initialize             - Flag that indicates whether the code 
*                               grabber has been initialized
*
* OUTPUTS                                                              
*
*      Upon successful initialization, the function will return NU_SUCCESS.
*      If is_fresh or code equals clear_code then first_code is returned. 
*      If code equals end_code and Done_Data_Processing then GIF_DONE_LZW is returned.
*      If code equals end_code and not Done_Data_Processing then -2 is returned.
*      If code greater than or equal to clear_code and code is equal to 
*      table[0][code] then GIF_BAD_CODE.
*      If *sp > stack then the *--sp is returned;
*      Otherwise the code is returned.
*                                                                       
*************************************************************************/
int GIF_LZW_Byte_Read( int initialize, int input_code_size )
{
    static short    is_fresh = NU_FALSE;
    int             code;
    int             in_code;
    static int      code_size;
    static int      set_code_size;
    static int      max_code;
    static int      max_code_size;
    static int      first_code;
    static int      old_code;
    static int      clear_code;
    static int      end_code;
    static int      table[2][(1<< MAX_LZW_BITS)];
    static int      stack[(1<<(MAX_LZW_BITS))*2];
    static int      *sp;
    register int    i;

    /*  Initialize the LZW Decoder */
    if (initialize) 
    {
        set_code_size = input_code_size;
        code_size = set_code_size+1;
        clear_code = 1 << set_code_size ;
        end_code = clear_code + 1;
        max_code_size = 2*clear_code;
        max_code = clear_code+2;

        GIF_Get_Code_From_Data(0, NU_TRUE);

        is_fresh = NU_TRUE;

        /*  Init the Code Tables for entries */
        for (i = 0; i < clear_code; ++i) 
        {
            table[0][i] = 0;
            table[1][i] = i;
        }
        for (; i < (1<<MAX_LZW_BITS); ++i)
        {
            table[0][i] = table[1][0] = 0;
        }

        /*  Set the Stack to the scanline pointer */
        sp = stack;

        return (NU_SUCCESS);
    } 
    else if (is_fresh) 
    {
        is_fresh = NU_FALSE;
        do 
        {   /*  Get a code until it is not a clear code */
            first_code = old_code =
                GIF_Get_Code_From_Data(code_size, NU_FALSE);
        } while (first_code == clear_code);

        return(first_code);
    }

    if (sp > stack)
    {
        return (*--sp);
    }

    while ((code = GIF_Get_Code_From_Data(code_size, NU_FALSE)) >= 0) 
    {
        /*  Check if the code is a clear code */
        if (code == clear_code) 
        {
            /*  Clear the Table entries with the clear code */
            for (i = 0; i < clear_code; ++i) 
            {
                table[0][i] = 0;
                table[1][i] = i;
            }
            /*  Clear the Table entries with the clear code */
            for (; i < (1<<MAX_LZW_BITS); ++i)
            {
                table[0][i] = table[1][i] = 0;
            }

            code_size = set_code_size + 1;
            max_code_size = 2*clear_code;
            max_code = clear_code + 2;
            sp = stack;
            first_code = old_code = GIF_Get_Code_From_Data(code_size, NU_FALSE);
            return(first_code);
        } 
        else if (code == end_code) 
        {
            int             count;
            unsigned char   buf[260];

            if (Done_Processing)
            {
                return (GIF_DONE_LZW);
            }

            while ((count = GIF_Get_Block_of_Data(buf)) > 0)
            {
                ;
            }

            if (count != 0)
            {       
                /*  This condition occurs when EOD in data Stream */
                count= count;
            }
            return(-2);
        }
        /*  Save of the current code for the old code */
        in_code = code;

        /*  If the code is greater than the max_code increment the buffer and 
         *  set the code to the old code.
         */
        if (code >= max_code)
        {
            *sp++ = first_code;
            code = old_code;
        }

        /*  While the Code is greater than or equal than the clear code
         *  update the data and store in the data.
         */
        while (code >= clear_code) 
        {
            *sp++ = table[1][code];

            if (code == table[0][code])
            {
                return(GIF_BAD_CODE);
            }
            code = table[0][code];
        }

        /*  Set the Data to the first entry in the table */
        *sp++ = first_code = table[1][code];

        if ((code = max_code) <(1<<MAX_LZW_BITS)) 
        {
            table[0][code] = old_code;
            table[1][code] = first_code;
            ++max_code;

            if ((max_code >= max_code_size) &&
                (max_code_size < (1<<MAX_LZW_BITS))) 
            {
                max_code_size *= 2;
                ++code_size;
            }
        }

        /*  Store the in_code in the old_code */
        old_code = in_code;

        if (sp > stack)
        {
            return (*--sp);
        }
    }
    return (code);
}

/*************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*     GIF_Image_Decode                                                  
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*     Decodes a gif image and writes the scanlines in proper order.     
*     It supports interlacing.                                          
*                                                                       
* INPUTS                                                                
*                                                                       
*      width_of_image         - Width of the image
*      height_of_image        - Height of the image
*      gray_scale             - flag that indicates 
*                               whether the image is gray scale or not 
*      image_interlaced       - flag that indicates 
*                               whether the image is interlaced or not 
*      ignore_image           - flag that indicates whether the image
*                               is should be ignored or not
*
* OUTPUTS                                                              
*
*      Upon successful completion, the function will return NU_SUCCESS.
*      If the Read Input fails the function will return GIF_ERROR_BAD_FILE. 
*      If the Read LZW byte fails the function will return GIF_LZW_FAIL. 
*      If ignore_image then the function returns GIF_IMAGE_IGNORED.
*      If allocation fails then return status;
*                                                                       
*************************************************************************/
int  GIF_Image_Decode( int width_of_image, int height_of_image, 
                       int gray_scale, int image_interlaced, int ignore_image )
{
    unsigned char   c;
    int             v;
    int             xpos = 0;
    int             ypos = 0;
    int             pass = 0;
    unsigned char   *scanline = NU_NULL;
    int             status;
    unsigned char   *temp_buffer;    
    
    /*
    **  Initialize the Compression routines
    */
    if (GIF_Read_Input_Buffer(&c,1))
    {
        return(GIF_ERROR_BAD_FILE);         
    }

    if (GIF_LZW_Byte_Read(NU_TRUE, c) < 0)
    {
        return(GIF_LZW_FAIL);
    }

    /*
    **  If this is an "uninteresting picture" ignore it.
    */
    if (ignore_image) 
    {
        while (GIF_LZW_Byte_Read(NU_FALSE, c) >= 0)
        {
            ;
        }
        return(GIF_IMAGE_IGNORED);
    }
    status = GRAFIX_Allocation(&System_Memory,(VOID **)&scanline,
                width_of_image,NU_NO_SUSPEND);

        
    if ( status != NU_SUCCESS)
    {
        return(status);
    }

    while (ypos<height_of_image && (v = GIF_LZW_Byte_Read(NU_FALSE,c)) >= 0) 
    {
        /*  Store the Byte into Scanline */
        scanline[xpos]= v;

        /*  Increment the X position */
        ++xpos;

        /* If the Xpostion is equal to the Width of image then store the scanline */
        if (xpos == width_of_image) 
        {
           /* Point the buffer to the offset given by the yposition */
           /* We need the size of the buffer and size of imageheader and not image */
           temp_buffer= (imgbuffer+sizeof(imageHeader)) + ((ypos)*getimage->imRowBytes);

           /*  Store the BUffer Contents */
           STR_mem_cpy(temp_buffer,scanline,getimage->imRowBytes);
        
            xpos = 0;
            if (image_interlaced) 
            {
                /*  Interlaced is handled by offsetting the buffer by 8, 4 and 2.
                 *  That is skip every eight scan lines, then four then two.
                 *  Once the ypos is greater than the height it then resets the
                 *  buffer location to 0, then four, then 2, and finally by one.
                 */
                static int dpass[]= {8,8,4,2};
                ypos += dpass[pass];
                if (ypos >= height_of_image) 
                {
                    static int restart[]= {0,4,2,1,32767};
                    ypos= restart[++pass];
                }
            } 
            else
            {
                ++ypos;
            }
        }
    }

    if(GIF_LZW_Byte_Read(NU_FALSE,c) >= 0)
    {
        ;
    }

    GRAFIX_Deallocation(scanline);

    return(NU_SUCCESS);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*     GIF_Get_Block_of_Data
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*     Reads a Block of data until it reaches a character count of 0.    
*                                                                       
* INPUTS                                                                
*                                                                       
*      buf               - the buffer to read
*
* OUTPUTS                                                              
*
*      Upon successful completion, the function will return NU_SUCCESS.
*      If the Read Input fails the function will return GIF_ERROR_BAD_FILE. 
*      If the Read Input and count does not equal 0 the function will 
*      return GIF_ERROR_BAD_FILE. 
*      Otherwise return count.
*                                                                       
*************************************************************************/
int GIF_Get_Block_of_Data( unsigned char  *buf )
{
    unsigned char   count;

    if (GIF_Read_Input_Buffer(&count,1)) 
    {   
        return (GIF_ERROR_BAD_FILE);
    }

    if (count == 0)
    {
        Done_Processing = 1;
    }

    if ((count != 0) && (GIF_Read_Input_Buffer(buf, count))) 
    {   
        return(GIF_ERROR_BAD_FILE);
    }

    return (count);
}

/*************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*     GIF_Read_Input_Buffer                                              
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*     Reads the Input Buffer by a specified amount of data.  It handles 
*     memory pointer management.  Once it has the specified amount of    
*     it will return it in the specified data path.                             
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*      buffer               - the buffer to read
*      data_amount          - the amount of data to read
*
* OUTPUTS                                                              
*
*      Upon successful completion, the function will return NU_SUCCESS.
*                                                                       
*************************************************************************/
int GIF_Read_Input_Buffer(unsigned char *buffer, int data_amount)
{

    if(gif_browse_data->total_bytes > 0)
    {
        gif_browse_data->offset_ptr = gif_browse_data->file_buffer 
                                      + gif_browse_data->current_byte;

        STR_mem_cpy(buffer, gif_browse_data->offset_ptr, data_amount);

        gif_browse_data->current_byte += data_amount;
        gif_browse_data->bytes_remaining = gif_browse_data->bytes_remaining - data_amount;

        if(gif_browse_data->bytes_remaining < 0)
        {
            gif_browse_data->bytes_remaining = gif_browse_data->bytes_remaining;
        }
    }

   return(NU_SUCCESS);

} /* GIF_Read_Input_Buffer */


/*************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*     GIF_GetColorCount                                                     
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*     Reads the Number of Colors that Are Present in the Gif Colorspace.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*      flags                - flags that indicate the color count
*
* OUTPUTS                                                              
*
*      The function will return the color count.
*                                                                       
*************************************************************************/
int GIF_GetColorCount( unsigned char flags )
{
    return (1 <<((flags & 0x07) + 1));
}

#endif /* IMG_INCLUDED */
