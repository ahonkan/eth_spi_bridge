/******************************************************************************
*                                                                             
*              Copyright 2006 Mentor Graphics Corporation
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
*      imgincludes.h                                                                                     
*                                                                       
*  DESCRIPTION                                                          
*                                                                       
*       Contains all of the data structure ioStruct and other defines
*       necessary for displaying JPEGs and GIFs
*
*  DATA STRUCTURES                                                      
*                                                                       
*      ioSTRUCT                                                   
*                                                                       
*  FUNCTIONS                                                            
*                                                                       
*       None                                                            
*                                                                       
*  DEPENDENCIES                                                         
*                                                                       
*   grafixwt/inc/noatom.h
*   grafixrs/inc/rs_app.h
*   grafixrs/inc/rsconst.h
*   hardware_drivers/display/lcd/lcd_config.h
*   hardware_drivers/display/colors.h
******************************************************************************/
#ifndef IMGINCLUDES_H
#define IMGINCLUDES_H

#include "ui/img.h"

#ifdef IMG_INCLUDED

#include "nucleus.h"
#include "ui/rsconst.h"  
#include "ui/rsfonts.h"  
#include "ui/rsports.h"
#include "ui/rs_api.h"   
#include "ui/str_utils.h"
#include "ui/std_utils.h"
#include "ui/memrymgr.h"
#include "ui/rects.h"
#include "ui/rsconst.h"
#include "drivers/display_config.h"
#include "drivers/colors.h"

   /* This is the input output structure for the jpeg */
    typedef struct _iostruct {
    
	UINT8 *buf;                                         /*  The Raw Buffer for the loaded content                                   */
    UINT8 *cur_loc;                                     /*  Current Location of a resource if already loaded                        */

    unsigned long buf_size;                             /*  The Size of the Buffer                                                  */

    short getSizeofImage;                               /*  Get only the size of the image resource                                 */
    short status;                                       /*  Status of where the browser is between layers                           */

    short error_flag;                                   /*  Error while loading an image or other resource                          */
    short access_flag;                                  /*  Is the resource accessible                                              */
    short ImageHgt;                                     /*  The image Height                                                        */
    short ImageWdt;                                     /*  The image Width                                                         */

	short nodraw;                                       /*  Do not draw the Image resource                                          */
    short isImage;                                      /*  Is this resource an image?                                              */
    short formimage;                                    /*  Is this a Input Image?                                                  */
    short trans_set;                                    /*  Is the transparency set and the image sizes are different(animated gif) */

    short transparent;                                  /*  Is this a transparent Image                                             */
    short transparent_index;                            /*  Is this a transparent index                                             */
    short grayscale;                                    /*  Is this a grayscale image                                               */
    short num_images;                                   /*  How many animated frames within this image.                             */

	short type;                                         /*  Type of Image 
                                                         *  AGIF   1
                                                         *  GIF    2
                                                         *  JPEG   3
                                                         *  PNG    4
                                                         */
	short background;                                   /*  Is this a background image within a page                                */
        
	unsigned char *formidata;                           /*  form image data pointer                                                 */
    unsigned char *imagedata;                           /*  Non-Input image                                                         */

	palData cpalt[512];                                 /*  The image Palette                                                      */

    int imgsize;                                        /*  the image size                                                          */
    int num_colors;                                     /*  The number of colors                                                    */

	rect objRect;                                       /*  The rectangle for the resource that is being loaded                     */
    int animGifNum;                                     /*  The animated Gif number                                                 */   

    int  rsp_type;
    int x;
    int y;

    } ioSTRUCT;

#define AGIF   1
#define GIF    2
#define JPEG   3

#endif /*IMG_INCLUDED*/

#endif /*IMGINCLUDES_H*/
