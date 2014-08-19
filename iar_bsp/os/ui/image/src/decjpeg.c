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
* Copyright (C) 1994-1998, Thomas G. Lane.
* This file is part of the Independent JPEG Group's software.
* For conditions of distribution and use, see the accompanying README file.
* The version of source is at version 6b.
*******************************************************************************
*******************************************************************************
*                                                                       
* FILE NAME                                                            
*                                                                       
*      decjpeg.c                                                                  
*                                                                       
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*      The functions will decompress jpeg's and grayscale jpegs.         
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*  jpeg_file_data                      Data Structure for the src jpeg                     
*  img_dest_struct                     Data Structure for the dst jpeg  
*                                                                       
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*      decompress_jpeg                 Initial routine to decompress a  
*                                      jpeg.                            
*      jinit_image_file                Initialize the image file        
*                                      structure.                       
*      write_img_header                Write the Image header.          
*      write_colormap                  Write the Color Map              
*      finish_output_img               Write the Sarray image ptr to    
*                                      the output image buffer          
*      put_pixel_rows                  Puts 24-bit rows                 
*      put_gray_rows                   Puts RGB and Grayscale rows           
*                                                                             
*                                                                       
* DEPENDENCIES                                                          
*                                                                       
*      image/inc/cdjpeg.h                                                 
*      image/inc/jversion.h	                                                  
*      image/inc/imgincludes.h                                                  
*                                                                      
*************************************************************************/

#include "ui/cdjpeg.h"		    /* Common decls for cjpeg/djpeg applications */
#include "ui/jversion.h"		/* for version message */
#include "ui/imgincludes.h"

char decjpeg_remove_warning;

#ifdef IMG_INCLUDED
#if defined (__HITACHI__)
#include <stdio.h>
#endif
#ifdef NU_SIMULATION
#include "ui/noatom.h"
#endif

/* Misc function prototypes */
#include "ui/rsextern.h"


/*  Externals and some data structures used */
palData cpalette[512];                                  /*  The created image palette */
extern void jpeg_file_src (j_decompress_ptr cinfo);     /*  The function that sets up the external file src */
jpeg_file_data jpeg_browse_data;                        /*  Jpeg file data */


extern struct _palData system_palette[];
extern NU_MEMORY_POOL  System_Memory;

/*  Structure for image destination. */
typedef struct {
  struct djpeg_dest_struct pub;	/* public fields */
  boolean dummy;                   /*  Pad buffer field for alignment */
  jvirt_sarray_ptr whole_image;	 /* needed to reverse row order */
  JDIMENSION data_width;	     /* JSAMPLEs per row */
  JDIMENSION row_width;		     /* physical width of one row in the image file */
  int pad_bytes;		         /* number of padding bytes needed per row */
  JDIMENSION cur_output_row;	 /* next row# to write to virtual array */
} img_dest_struct;

/*  Declare image structures */
typedef img_dest_struct * img_dest_ptr;
unsigned char *imgbuf;
unsigned char *tempimgbuf;
UNSIGNED buf_size;
imageHdr *getimg;
int imgsize=0;
long XltTable[512];
int m_bpp=0;

boolean bittype;


/*  Declares Local Functions */
void write_colormap (j_decompress_ptr cinfo, img_dest_ptr dest,
		              int map_colors, int map_entry_size);
void start_output_img (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo);
void write_img_header (j_decompress_ptr cinfo, img_dest_ptr dest);

djpeg_dest_ptr jinit_image_file (j_decompress_ptr cinfo);
void finish_output_img (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo);
void put_gray_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
	       JDIMENSION rows_supplied);
void put_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
		JDIMENSION rows_supplied);


/* Create the add-on message string table. */

#define JMESSAGE(code,string)	string ,

static const char * const cdjpeg_message_table[] = {
#include "ui/cderror.h"
  NULL
};

/*************************************************************************/
/*                                                                       */
/* FUNCTION                                                              */
/*                                                                       */
/*     decompress_jpeg                                                   */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*     Main calling function of decompressing a jpeg.  It handles        */
/*     the decompression of jpegs, and displaying the Image.             */
/*                                                                       */
/*                                                                       */
/* INPUTS                                                                */
/*                                                                       */
/*      thisFile                    Pointer to ioStruct data structure   */
/*                                  that holds information about the     */
/*                                  current gif Image to be decoded.     */
/*                                                                       */
/* OUTPUTS                                                               */
/*                                                                       */
/*      Upon successful completion, the service will return a 0. If the  */
/*      Service Fails it will return -1.                                */      
/*                                                                       */
/*************************************************************************/
int decompress_jpeg (ioSTRUCT *thisFile, int bpp, boolean bit16type )
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
#ifdef PROGRESS_REPORT
  struct cdjpeg_progress_mgr progress;
#endif

  rect imgRect;  

  djpeg_dest_ptr dest_mgr = NULL;
  JDIMENSION num_scanlines;
  image *dstImgPtr;
  image *tempImgPtr;
  STATUS status;
  int i;
  int GrayScale=0;
  palData RGBdata[256];
  grafPort        *thisPort;
  
  GetPort( &thisPort );
  
  m_bpp = bpp;
	
  bittype = bit16type;


  /* Initialize the JPEG decompression object with default error handling. */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  /* Add some application-specific error messages (from cderror.h) */
  jerr.addon_message_table = cdjpeg_message_table;
  jerr.first_addon_message = JMSG_FIRSTADDONCODE;
  jerr.last_addon_message = JMSG_LASTADDONCODE;


  /* Now safe to enable signal catcher. */
#ifdef NEED_SIGNAL_CATCHER
  enable_signal_catcher((j_common_ptr) &cinfo);
#endif

#ifdef PROGRESS_REPORT
  start_progress_monitor((j_common_ptr) &cinfo, &progress);
#endif

  status = GRAFIX_Allocation(&System_Memory,(VOID *)&jpeg_browse_data,sizeof(jpeg_file_data)+5,NU_NO_SUSPEND);
  if(status != NU_SUCCESS)
  {
      while(1);
  }
  
  status = GRAFIX_Allocation(&System_Memory,(VOID *)&jpeg_browse_data->file_buffer,thisFile->buf_size+sizeof(imageHdr),NU_NO_SUSPEND);
  if(status != NU_SUCCESS)
  {
      while(1);
  }

  STR_mem_cpy(jpeg_browse_data->file_buffer, thisFile->buf, thisFile->buf_size);

  jpeg_browse_data->total_bytes= thisFile->buf_size;
  jpeg_browse_data->bytes_remaining= thisFile->buf_size;
  
  /* Specify data source for decompression */
  jpeg_file_src(&cinfo);

  /* Read file header, set default decompression parameters */
  jpeg_read_header(&cinfo, TRUE);

  /* Initialize the output module now to let it override any crucial
   * option settings (for instance, GIF wants to force color quantization).
   */
   dest_mgr=jinit_image_file (&cinfo);

   jpeg_start_decompress(&cinfo);
   if(dest_mgr== NULL)
   {
       jpeg_destroy_decompress(&cinfo);
       GRAFIX_Deallocation(jpeg_browse_data->file_buffer);
       GRAFIX_Deallocation(jpeg_browse_data);
       return -1;
   }



  
  /* Process data */
  while (cinfo.output_scanline < cinfo.output_height) 
  {
    num_scanlines = jpeg_read_scanlines(&cinfo, dest_mgr->buffer,
					dest_mgr->buffer_height);
    /*  This places the scanlines in a temporary buffer */
    if (cinfo.quantize_colors)
    {
        put_gray_rows (&cinfo, dest_mgr,num_scanlines);	       
    }
    else
    {
       put_pixel_rows( &cinfo, dest_mgr,num_scanlines);
    }
  }

#ifdef PROGRESS_REPORT
  /* Hack: count final pass as done in case finish_output does an extra pass.
   * The library won't have updated completed_passes.
   */
  progress.pub.completed_passes = progress.pub.total_passes;
#endif

  /* Finish decompression and release memory.
   * I must do it in this order because output module has allocated memory
   * of lifespan JPOOL_IMAGE; it needs to finish before releasing memory.
   */
  finish_output_img (&cinfo, dest_mgr);
  /*  Finish the decompression */
  jpeg_finish_decompress(&cinfo);
  /*  Destroy the decompression routine */
  jpeg_destroy_decompress(&cinfo);

  /*  If the Size of the image is being looked for exit the routine with the size set */
   if(thisFile->getSizeofImage==1)
   {        
        thisFile->ImageWdt=cinfo.output_width;
        thisFile->ImageHgt=cinfo.output_height;
        GRAFIX_Deallocation(imgbuf);
        GRAFIX_Deallocation(jpeg_browse_data);
        return(0);
    }



        if(thisFile->formimage==1)
        {
            /*  Get the Width and Height of the Image */
            thisFile->ImageWdt=cinfo.output_width;
            thisFile->ImageHgt=cinfo.output_height;
            thisFile->transparent=0;
            thisFile->transparent_index=0;
            thisFile->num_colors=512;
            if(GrayScale== 1)
                thisFile->grayscale=1;
            else
                thisFile->grayscale=0;
            for(i=0;i<thisFile->num_colors;i++)
            {
                thisFile->cpalt[i]=cpalette[i];
            }

            /* This is form data so no need to allocate */
            status = GRAFIX_Allocation(&System_Memory,(VOID *)&thisFile->formidata,imgsize+10+12,NU_NO_SUSPEND);
            if(status != NU_SUCCESS)
            {
                while(1);
            }
            thisFile->imgsize=imgsize+12;
            STR_mem_cpy(thisFile->formidata,imgbuf,imgsize+12);
            GRAFIX_Deallocation(imgbuf);
            GRAFIX_Deallocation(jpeg_browse_data);

            return(0);

        }
        

    
    if(thisFile->nodraw==0)
    {

    #ifdef PROGRESS_REPORT
      end_progress_monitor((j_common_ptr) &cinfo);
    #endif

    }
        thisFile->ImageWdt=cinfo.output_width;
        thisFile->ImageHgt=cinfo.output_height;
        thisFile->transparent=0;
        thisFile->transparent_index=0;
        thisFile->num_colors=512;
        thisFile->grayscale=GrayScale;

        for(i=0;i<thisFile->num_colors;i++)
        {
                thisFile->cpalt[i]=cpalette[i];
        }

        status = GRAFIX_Allocation(&System_Memory,(VOID *)&tempImgPtr,sizeof(image),NU_NO_SUSPEND);
        if(status != NU_SUCCESS)
        {
            while(1);
        }

        status = GRAFIX_Allocation(&System_Memory,(VOID *)&dstImgPtr,sizeof(image),NU_NO_SUSPEND);
        if(status != NU_SUCCESS)
        {
            while(1);
        }

        buf_size = (m_bpp/8) * (thisFile->ImageWdt * thisFile->ImageHgt);
        status = GRAFIX_Allocation(&System_Memory,(VOID *)&dstImgPtr->imData,buf_size,NU_NO_SUSPEND);
        if(status != NU_SUCCESS)
        {
            while(1);
        }

        tempImgPtr->imWidth = ((image *)imgbuf)->imWidth;
        tempImgPtr->imHeight = ((image *)imgbuf)->imHeight;
        tempImgPtr->imAlign = ((image *)imgbuf)->imAlign;
        tempImgPtr->imFlags = ((image *)imgbuf)->imFlags;
        tempImgPtr->pad = ((image *)imgbuf)->pad;
        tempImgPtr->imBytes = ((image *)imgbuf)->imBytes;
        tempImgPtr->imBits = ((image *)imgbuf)->imBits;
        tempImgPtr->imPlanes = ((image *)imgbuf)->imPlanes;
        tempImgPtr->imData = (UINT8 *)(&(((image *)imgbuf)->imData));
     
        if((UINT8)tempImgPtr->imBits != BPP)
        {
            XlateColors(XltTable,(UINT8)tempImgPtr->imBits,(UINT8)tempImgPtr->imPlanes,RGBdata,cpalette);
        }
        tempImgPtr->imData = (UINT8 *)(&(((image *)imgbuf)->imData));
        XlateImage(tempImgPtr,dstImgPtr,bpp,1,XltTable);

        /*  Set the Trans Set */
	    thisFile->trans_set=0;
        thisFile->type=JPEG;
        /* Set the Num Images */

        SetRect(&imgRect,thisFile->x,thisFile->y,thisFile->x+thisFile->ImageWdt,thisFile->y+thisFile->ImageHgt);
        
        SetPort(thisPort);
        
        WriteImage(&imgRect,dstImgPtr);

      /* clean up */

      GRAFIX_Deallocation(jpeg_browse_data->file_buffer);
      GRAFIX_Deallocation(jpeg_browse_data);
      GRAFIX_Deallocation(tempImgPtr);
      GRAFIX_Deallocation(dstImgPtr->imData);
      dstImgPtr->imData = NU_NULL;
      GRAFIX_Deallocation(dstImgPtr);
      GRAFIX_Deallocation(getimg);

  return 1;			/* suppress no-return-value warnings */
}


/*  Initialize the image file structure */
/*************************************************************************/
/*                                                                       */
/* FUNCTION                                                              */
/*                                                                       */
/*     jinit_image_file                                                  */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*      Initialize the image file structure.                             */
/*************************************************************************/

djpeg_dest_ptr jinit_image_file (j_decompress_ptr cinfo)
{
    img_dest_ptr dest;
    JDIMENSION row_width;
    STATUS status;
  
    /* Create module interface object, fill in method pointers */
    dest = (img_dest_ptr)
          (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(img_dest_struct));
   
    /*  Setup different Jpeg Library Configurations */
    cinfo->do_fancy_upsampling=TRUE;
    /*  Setup the colors for 3 components or 1 for Grayscale */
   if(cinfo->num_components!=1)
   {
#if (BPP == 24)
        cinfo->quantize_colors = FALSE;
#else
		cinfo->quantize_colors = TRUE;
#endif
        cinfo->dither_mode = JDITHER_ORDERED;
   }
   else
   {
       /*  Must be Grayscale but do not dither the image */
       cinfo->quantize_colors=TRUE;
   }

  /*  Check out the color space and set the appropriate 
   *  output data transfer routine.
   */
  if (cinfo->out_color_space == JCS_GRAYSCALE) 
  {
    dest->pub.put_pixel_rows = put_gray_rows;
  } 
  else if (cinfo->out_color_space == JCS_RGB) 
  {
    if (cinfo->quantize_colors)
      dest->pub.put_pixel_rows = put_gray_rows;
    else
      dest->pub.put_pixel_rows = put_pixel_rows;
  } 
  else 
  {
      dest = NULL;
     /*  Error so return */
      return(djpeg_dest_ptr)dest;
  }
                     
  /* Calculate output image dimensions so we can allocate space */
  jpeg_calc_output_dimensions(cinfo);

  /* Determine width of rows in the IMG file (padded to 4-byte boundary). */
  row_width = cinfo->output_width * cinfo->output_components;

  /*  Set the destination data width */
  dest->data_width = row_width;

  /*  Setup the byte padding */
  while ((row_width & 3) != 0) 
      row_width++;
  dest->row_width = row_width;
  dest->pad_bytes = (int) (row_width - dest->data_width);

  /*  Allocate the Image Buffer for the Nucleus Grafix image translation facilities */
  
  status = GRAFIX_Allocation(&System_Memory,(VOID *)&imgbuf,(cinfo->image_height * row_width)+sizeof(imageHeader),NU_NO_SUSPEND);
  if(status != NU_SUCCESS)
  {
      while(1);
  }

  /*  Set a pointer to Nucleus Grafix Image format (DIB) */
  getimg= (imageHdr *)imgbuf;
  
  imgsize=(cinfo->image_height * row_width);
  
  /* Allocate space for inversion array, prepare for write pass */
  dest->whole_image = (*cinfo->mem->request_virt_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE, FALSE,
     row_width, cinfo->output_height, (JDIMENSION) 1);

  /*  Set the Current Output row to 0 */
  dest->cur_output_row = 0;

  /*  If using the progress indicator set it up(Not used at this time)in this feature */
  if (cinfo->progress != NULL) 
  {
    cd_progress_ptr progress = (cd_progress_ptr) cinfo->progress;
    progress->total_extra_passes++; /* count file input as separate pass */
  }

  /* Create decompressor output buffer. */
  dest->pub.buffer = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE, row_width, (JDIMENSION) 1);

  /*  Initialize the buffer height */
  dest->pub.buffer_height = 1;

  /*  return a pointer to the destination file header */
  return (djpeg_dest_ptr) dest;
}

/*  Write the image header into the data structure */
/*************************************************************************/
/*                                                                       */
/* FUNCTION                                                              */
/*                                                                       */
/*     write_img_header                                                  */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*      Write the image header into the data structure.                  */
/*                                                                       */
/*************************************************************************/
void write_img_header (j_decompress_ptr cinfo, img_dest_ptr dest)

{
  int bits_per_pixel, cmap_entries;

  /* Compute colormap size and total file size */
  if (cinfo->out_color_space == JCS_RGB) 
  {
    if (cinfo->quantize_colors) 
    {
      /* Colormapped RGB */
      bits_per_pixel = 8;
      cmap_entries = 256;
      /*  setup the Nucleus Grafix Header File Format */
      getimg->imWidth = cinfo->output_width;
      getimg->imHeight = cinfo->output_height;
      getimg->imAlign = 0;
      getimg->imFlags = 0;
      getimg->imPlanes= 1;
      getimg->imBits= bits_per_pixel;
      getimg->imBytes= cinfo->output_width * cinfo->output_components;

    } 
    else 
    {
      /* Unquantized, full color RGB */
      bits_per_pixel = 24;
      cmap_entries = 0;
      /*  setup the Nucleus Grafix Header File Format */
      getimg->imWidth = cinfo->output_width;
      getimg->imHeight = cinfo->output_height;
      getimg->imAlign = 0;
      getimg->imFlags = 0;
      getimg->imPlanes= 1;
      getimg->imBits= bits_per_pixel;
      getimg->imBytes= cinfo->output_width * cinfo->output_components;
    }
  } 
  else 
  {
    /* Grayscale output.  We need to fake a 256-entry colormap. */
    bits_per_pixel = 8;
    cmap_entries = 256;
    /*  setup the Nucleus Grafix Header File Format */
    getimg->imWidth = cinfo->output_width;
    getimg->imHeight = cinfo->output_height;
    getimg->imAlign = 0;
    getimg->imFlags = 0;
    getimg->imPlanes= 1;
    getimg->imBits= bits_per_pixel;
    getimg->imBytes= cinfo->output_width * cinfo->output_components;
  }

  /*  Get the Color Palette used for GrayScale or RGB Color */
  if (cmap_entries > 0 )
    write_colormap(cinfo, dest, cmap_entries, 3);
}




/*************************************************************************/
/*                                                                       */
/* FUNCTION                                                              */
/*                                                                       */
/*     write_colormap                                                    */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*      Write the colormap for RGB images and GrayScale images.          */
/*                                                                       */
/*************************************************************************/

void write_colormap (j_decompress_ptr cinfo, img_dest_ptr dest,
		              int map_colors, int map_entry_size)
{
  JSAMPARRAY colormap = cinfo->colormap;
  int num_colors = cinfo->actual_number_of_colors;

  int i;

  if (colormap != NULL) 
  {
    if (cinfo->out_color_components == 3) 
    {
      /* Normal case with RGB colormap */
      for (i = 0; i < num_colors; i++) 
      {
          cpalette[i].palRed=   colormap[0][i] << 8; /*  Get red color value   */
          cpalette[i].palGreen= colormap[1][i] << 8; /*  Get Green color value */
          cpalette[i].palBlue=  colormap[2][i] << 8; /*  Get Blue color value  */
	        
	  }
    } 
    else 
    {
      /* Grayscale colormap (only happens with grayscale quantization) */
      /*  Set the RGB value to the same color palette index */
            for (i = 0; i < num_colors; i++) 
            {
	            cpalette[i].palRed=   colormap[0][i] << 8; /*  Get red color value   */
                cpalette[i].palGreen= colormap[0][i] << 8; /*  Get Green color value */
                cpalette[i].palBlue=  colormap[0][i] << 8; /*  Get Blue color value  */	
            }
    }

  } 
  
}


/*************************************************************************/
/*                                                                       */
/* FUNCTION                                                              */
/*                                                                       */
/*     finish_output_img                                                 */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*      Write the Sarray image ptr to the output image buffer.           */
/*************************************************************************/

void finish_output_img (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  img_dest_ptr dest = (img_dest_ptr) dinfo;
  long int count=0;
  JSAMPARRAY image_ptr;
  register JSAMPROW data_ptr;
  JDIMENSION row;
  register JDIMENSION col;
  cd_progress_ptr progress = (cd_progress_ptr) cinfo->progress;
  int offset;

  offset = sizeof(imageHdr);
  
  /*  Write the Image Header */
  write_img_header(cinfo, dest);

  count=0;
  
  /* Write the file body from our virtual array */
  for (row =0; cinfo->output_height > row; row++) 
  {
    image_ptr = (*cinfo->mem->access_virt_sarray)
      ((j_common_ptr) cinfo, dest->whole_image, row, (JDIMENSION) 1, FALSE);
    data_ptr = image_ptr[0];

#if (BPP == 24)
    for (col = cinfo->image_width * 3; col > 0; col--)
#else
    for (col = cinfo->image_width; col > 0; col--)
#endif     
    {
        imgbuf[count+offset] = *data_ptr;
        count++;


        data_ptr++;
    }
  }
  /*  If progress used then show it(Not supported at this Time) */
  if (progress != NULL)
    progress->completed_extra_passes++;

}


/*************************************************************************/
/*                                                                       */
/* FUNCTION                                                              */
/*                                                                       */
/*                                                         */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*      Put the pixel rows in the virtual array. This version is for     */
/*      writing 24-bit pixels                                            */
/*************************************************************************/

void put_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
		JDIMENSION rows_supplied)
/* This version is for writing 24-bit pixels */
{
  img_dest_ptr dest = (img_dest_ptr) dinfo;
  JSAMPARRAY image_ptr;
  register JSAMPROW inptr, outptr;
  register JDIMENSION col;
  int pad;

  /* Access next row in virtual array */
  image_ptr = (*cinfo->mem->access_virt_sarray)
    ((j_common_ptr) cinfo, dest->whole_image,
     dest->cur_output_row, (JDIMENSION) 1, TRUE);
  dest->cur_output_row++;

  
  inptr = dest->pub.buffer[0];
  outptr = image_ptr[0];
  for (col = cinfo->output_width; col > 0; col--) 
  {
    outptr[0] = *inptr++;	/* can omit GETJSAMPLE() safely */
    outptr[1] = *inptr++;
    outptr[2] = *inptr++;
    outptr += 3;
  }

  /* Zero out the pad bytes. */
  pad = dest->pad_bytes;
  while (--pad >= 0)
    *outptr++ = 0;
}


/*************************************************************************/
/*                                                                       */
/* FUNCTION                                                              */
/*                                                                       */
/*     put_gray_rows                                                     */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*     Put the pixel rows in the virtual array. This version is for      */
/*     grayscale OR quantized color output                               */
/*************************************************************************/

void put_gray_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
	       JDIMENSION rows_supplied)
/* This version is for grayscale OR quantized color output */
{
  img_dest_ptr dest = (img_dest_ptr) dinfo;
  JSAMPARRAY image_ptr;
  register JSAMPROW inptr, outptr;
  register JDIMENSION col;
  int pad;

  /* Access next row in virtual array */
  image_ptr = (*cinfo->mem->access_virt_sarray)
    ((j_common_ptr) cinfo, dest->whole_image,
     dest->cur_output_row, (JDIMENSION) 1, TRUE);
  dest->cur_output_row++;

  /* Transfer data. */
  inptr = dest->pub.buffer[0];
  outptr = image_ptr[0];
  for (col = cinfo->output_width; col > 0; col--) 
  {
    *outptr++ = *inptr++;	/* can omit GETJSAMPLE() safely */
  }

  /* Zero out the pad bytes. */
  pad = dest->pad_bytes;
  while (--pad >= 0)
    *outptr++ = 0;
}



/*************************************************************************/
/*                                                                       */
/* FUNCTION                                                              */
/*                                                                       */
/*     start_output_img                                                  */
/*                                                                       */
/*************************************************************************/

void start_output_img (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  /* no work here */
}




VOID Display_Jpeg_Image(VOID *jp_image, ioSTRUCT *thisFile, INT file_size, INT x, INT y)
{
    grafPort        *thisPort;
	GetPort( &thisPort );
	   
    thisFile->buf = jp_image;
    thisFile->buf_size = file_size;
    thisFile->x = x;
    thisFile->y = y;
    
    decompress_jpeg(thisFile,thisPort->portMap->pixBits,1);
}

#endif /* IMG_INCLUDED */
