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
*   dbmp.h                                                      
*
* DESCRIPTION
*
*   Holds defines, prototypes and data structure to handle the BMP module.
*
* DATA STRUCTURES
*
*   bitmapHeader
*
* DEPENDENCIES
*
*   grafixrs/inc/rs_app.h
*
******************************************************************************/
#ifndef DBMP_H
#define DBMP_H

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
/*************** BMP defines, structures and function ***************/
/* BMP file header structure */
/* BMP file info structure */
typedef struct _bitmapHeader
    {
        UINT32  bf_Size;           /* Size of file */
        UINT16  bf_Reserved1;      /* Reserved */
        UINT16  bf_Reserved2;      /* Reserved */
        UINT32  bf_OffBits;        /* Offset to bitmap data */
        UINT32  bi_Size;           /* Size of info header */
        INT32   bi_Width;          /* Width of image */
        INT32   bi_Height;         /* Height of image */
        UINT16  bi_Planes;         /* Number of color planes */
        UINT16  bi_BitCount;       /* Number of bits per pixel */
        UINT32  bi_Compression;    /* Type of compression to use */
        UINT32  bi_SizeImage;      /* Size of image data */
        INT32   bi_XPelsPerMeter;  /* X pixels per meter */
        INT32   bi_YPelsPerMeter;  /* Y pixels per meter */
        UINT32  bi_ClrUsed;        /* Number of colors used */
        UINT32  bi_ClrImportant;   /* Number of important colors */
    } bitmapHeader;

/* Bi-Compression constants */
#define RS_BI_RGB       0             /* No compression - straight BGR data */
#define RS_BI_RLE8      1             /* 8-bit run-length compression */
#define RS_BI_RLE4      2             /* 4-bit run-length compression */
#define RS_BI_BITFIELDS 3             /* RGB bitmap with RGB masks */

#define BMP8_PALETTE_SIZE   1024
extern  UINT32			bmp_offset;
extern palData system_palette[];
extern NU_MEMORY_POOL      System_Memory;

VOID Convert_Palette(UINT8* newPalette, UINT8* oldPalette);
STATUS Get_Bmp_Header(INT fp, image *bmpImg);
STATUS Display_Bmp_Image(INT fp, INT32 x_coord, INT32 y_coord);
/***************End- BMP defines, structures and function ***************/

#endif /* IMG_INCLUDED */
#endif
