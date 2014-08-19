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
*  image.h                                                    
*
* DESCRIPTION
*
*  This file contains prototypes, and externs for image.c
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
* fal/inc/fal.h  
* image/inc/dgif.h
* image/inc/dbmp.h
*
***************************************************************************/
#ifndef _image_H_
#define _image_H_

#include "ui/img.h"

#if (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE)

#ifdef IMG_INCLUDED

#include "storage/pcdisk.h"
#include "ui/dgif.h"
#include "ui/dbmp.h"

#else

#include "ui/rs_app.h"

#endif

#define BMP                     1
#define GIF                     2
#define JPEG                    3


/* Offset to the palette at the end of the header. */
#define OFFSET_TO_PALETTE       54

extern NU_MEMORY_POOL      System_Memory;

#ifdef IMG_INCLUDED

extern palData system_palette[];

STATUS What_Image(CHAR *path);
extern Animated_Entry *Agif;
extern UINT32 bmp_offset;
extern VOID Display_Jpeg_Image(VOID *jp_image, ioSTRUCT *thisFile, INT file_size, INT x, INT y);
extern STATUS Display_Bmp_Image(INT fp, INT32 x_coord, INT32 y_coord);
extern int decompress_gif(ioSTRUCT *thisPage);

#endif /* IMG_INCLUDED */
#endif /* (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE) */
#endif

