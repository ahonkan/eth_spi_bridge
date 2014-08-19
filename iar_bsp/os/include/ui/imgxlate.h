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
*  imgxlate.h                                                   
*
* DESCRIPTION
*
*  This file contains prototypes and externs for imgxlate.c
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
*  None.
*
***************************************************************************/
#ifndef _IMGXLATE_H_
#define _IMGXLATE_H_

#if (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE)
/* Local Functions */

VOID   XlateColors( SIGNED *xlt, UINT8 imBits, UINT8 imPlanes,  palData *palette, palData *imgPal );

SIGNED XlateImage( image *argSrcImg, image *argDstImg, INT32 argDstBits, INT32 argDstPlns,
                  SIGNED *argXlPtr);
/* <= 256 color source */
VOID   lowcolor_any(VOID);    

/* RGB16 source, <= 256 color dest */
VOID   RGB16_lowcolor(VOID);  

/* 16 bpp source, 24 bit dest */
VOID   RGB16_RGB24(VOID);     

/* 16 bpp source, 32 bit dest */
VOID   RGB16_RGB32(VOID);

/* 24 bpp source, <= 256 color dest */
VOID   RGB24_lowcolor(VOID);  

/* 24 bpp source, highcolor dest */
VOID   RGB24_RGB16(VOID);

/* 24 bpp source, 32 dest */
VOID   RGB24_RGB32(VOID);

VOID RGB32_RGB16(VOID);

/* don't translate */
VOID   XL_NOP(VOID);          

/* 1 bit GetImPix */
VOID   GetImPix1(VOID);       

/* 2 bit GetImPix */
VOID   GetImPix2(VOID);      

/* 4 bit GetImPix */
VOID   GetImPix4(VOID);      

/* 8 bit GetImPix */
VOID   GetImPix8(VOID);       

/* 16 bit GetImPix */
VOID   GetImPix16(VOID);      

/* 24 bit GetImPix */
VOID   GetImPix24(VOID);      

/* 32 bit GetImPix */
VOID   GetImPix32(VOID);

/* 8 bit/3 plane GetImPix */
VOID   GetImPix83(VOID);      

/* 1 bit SetImPix */
VOID   SetImPix1(VOID);       

/* 2 bit SetImPix */
VOID   SetImPix2(VOID);       

/* 4 bit SetImPix */
VOID   SetImPix4(VOID);       

/* 8 bit SetImPix */
VOID   SetImPix8(VOID);       

/* 16 bit SetImPix */
VOID   SetImPix16(VOID);      

/* 24 bit SetImPix */
VOID   SetImPix24(VOID);      

/* 32 bit SetImPix */
VOID   SetImPix32(VOID);

#endif /* (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE) */
#endif /* _IMGXLATE_H_ */

