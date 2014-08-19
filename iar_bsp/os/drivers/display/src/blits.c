/***************************************************************************
*
*             Copyright 2010 Mentor Graphics Corporation
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
*  blits.c
*
* DESCRIPTION
*
*  This file contains Blit functions for globals.
*
* DATA STRUCTURES
*
*   None.
*
* FUNCTIONS
*
*  BLITS_SaveGlobals
*  BLITS_RestoreGlobals
*
* DEPENDENCIES
*
*  nucleus.h
*  nu_kernel.h
*  nu_drivers.h
*
***************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "drivers/nu_drivers.h"

#ifdef USE_CURSOR

/* Cursor save area */

/* ptr to src bitmap */
grafMap *srcBmapSv;     

/* ptr to dst bitmap */
grafMap *dstBmapSv;     

#ifndef NO_IMAGE_SUPPORT

/* pointer to source image */
image *srcImagSv;       

#endif  /* NO_IMAGE_SUPPORT */

/* pointer to dest mem for planar->E/VGA */
UINT8 *dstPtrSv;        

/* pointer to src mem for planar->E/VGA */
UINT8 *srcPtrSv;        

#ifdef INCLUDE_16_BIT

/* pointer to dest 16-bit mem */
UINT16 *dstPtr16Sv;     

/* pointer to src 16-bit mem */
UINT16 *srcPtr16Sv;     

#endif /* INCLUDE_16_BIT */

/* inc from end of row to start of next */
INT32 srcNextRowSv;     

/* inc from end of row to start of next */
INT32 dstNextRowSv;     

/* # bytes across source bitmap */
INT32 srcPixBytesSv;    

/* Source byte begin */
INT32 srcBgnByteSv;     

/* offset from start of row to first pixel to fill */
INT32 dstBgnByteSv;     

/* Dst device class */
INT32 dstClassSv;       

/* shift count (-=shfRt, +=shfLf) */
INT32 shfCntSv;         

/* line count (minus 1, dYmax-dYmin) */
INT32 lineCntM1Sv;      

/* Destination byte count (Minus 1) */
INT32 byteCntM1Sv;      

#if (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT))

/* background and foreground colors */
signed char pnColrSv;          
signed char bkColrSv;

#endif /* (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT)) */

#ifdef INCLUDE_16_BIT

/* background and foreground 16-bit colors */
INT16 pnColr16Sv;       
INT16 bkColr16Sv;

#endif /* INCLUDE_16_BIT */

#ifdef INCLUDE_24_BIT

/* background and foreground 24-bit colors */
SIGNED pnColr24Sv;      
SIGNED bkColr24Sv;

#endif /* INCLUDE_24_BIT */

#ifdef INCLUDE_32_BIT

/* The global variables for saving the 32-bit colors. */
UINT32    pnColr32Sv;
UINT32    bkColr32Sv;

#endif /* INCLUDE_32_BIT */

#if (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT))

/* shift count (-=shfRt, +=shfLf) */
INT32 shfCntSv;         

/* 0xffff for NOT SOURCE rops, 0 else */
INT32 flipMaskSv;       

/* mask used to mix adjacent source bytes */
INT32 shiftMaskSv;      

#endif /* (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT)) */

#ifndef NO_REGION_CLIP
                
/* 1 if region clipping active, 0 else */
signed char clipToRegionFlagSv;
                
#endif  /* NO_REGION_CLIP */

/* 1 if rect clipping active, 0 else */
signed char clipToRectFlagSv;  

/* flag that's 1 if blitRcd data is YXbanded */
signed char lclbfYXBandedSv;   

#ifdef  THIN_LINE_OPTIMIZE

/* base copy of drawStat for region clipping */
signed char drawStatSv;

#endif  /* THIN_LINE_OPTIMIZE */

#ifndef NO_REGION_CLIP
                
/* pointer to region rect at which to start  */
rect *nextRegionPtrSv;  
rect *nextRegionPtrBSv;

/* offset portion of pointer to region rect */
rect *tempRegionPtrSv;  

/* copy of destination rect */
rect bRectSv;           

/* copy of source rect */
rect bsRectSv;          

/* Ymin and Ymax coordinates for */
INT32 bandYminSv;       

/* current YX band */
INT32 bandYmaxSv;       

/* temp storage for Y extent of rectangle */
INT32 dYminTempSv;      
INT32 dYmaxTempSv;

/* temp storage for Y extent of source rect */
INT32 sYminTempSv;      
INT32 sYmaxTempSv;

#endif  /* NO_REGION_CLIP */

/* destination rect */
rect dRectSv;           

/* source rect */
rect sRectSv;           

/* clipping rect */
rect cRectSv;           

/* Fill/Blit drawing routine */
VOID (*FillDrawerSv)(blitRcd *drwPRec); 

/* ptr to BLIT optimization routine */
VOID (*optPtrSv)(VOID); 

/***************************************************************************
* FUNCTION
*
*    BLITS_SaveGlobals
*
* DESCRIPTION
*
*    Saves the global variable modified by the cursor routines
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID BLITS_SaveGlobals(VOID)
{
    srcBmapSv           = srcBmap;
    dstBmapSv           = dstBmap;
    
#ifndef NO_IMAGE_SUPPORT
    srcImagSv           = srcImag;
#endif  /* NO_IMAGE_SUPPORT */

    dstPtrSv            = dstPtr;
    srcPtrSv            = srcPtr;

#ifdef INCLUDE_16_BIT
    dstPtr16Sv          = dstPtr16;
    srcPtr16Sv          = srcPtr16;
#endif /* INCLUDE_16_BIT */

    srcNextRowSv        = srcNextRow;
    dstNextRowSv        = dstNextRow;
    srcPixBytesSv       = srcPixBytes;
    srcBgnByteSv        = srcBgnByte;
    dstBgnByteSv        = dstBgnByte;
    dstClassSv          = dstClass;
    lineCntM1Sv         = lineCntM1;
    byteCntM1Sv         = byteCntM1;

#if (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT) || defined(INCLUDE_8_BIT))
    pnColrSv            = pnColr;
    bkColrSv            = bkColr;
#endif /* (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT || defined(INCLUDE_8_BIT))) */

#ifdef INCLUDE_16_BIT
    pnColr16Sv          = pnColr16;
    bkColr16Sv          = bkColr16;
#endif /* INCLUDE_16_BIT */

#ifdef INCLUDE_24_BIT
    pnColr24Sv          = pnColr24;
    bkColr24Sv          = bkColr24;
#endif /* INCLUDE_24_BIT */

#ifdef INCLUDE_32_BIT
    pnColr32Sv          = pnColr32;
    bkColr32Sv          = bkColr32;
#endif /* INCLUDE_32_BIT */

#if (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT))
    shfCntSv            = shfCnt;
    flipMaskSv          = flipMask;
    shiftMaskSv         = shiftMask;
#endif /* (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT)) */

#ifndef NO_REGION_CLIP
                
    clipToRegionFlagSv  = clipToRegionFlag;
                
#endif  /* NO_REGION_CLIP */
                
    clipToRectFlagSv    = clipToRectFlag;
    lclbfYXBandedSv     = lclbfYXBanded;

#ifdef  THIN_LINE_OPTIMIZE
    drawStatSv          = drawStat;
#endif  /* THIN_LINE_OPTIMIZE */

#ifndef NO_REGION_CLIP
                
    nextRegionPtrSv     = nextRegionPtr;
    nextRegionPtrBSv    = nextRegionPtrB;
    tempRegionPtrSv     = tempRegionPtr;
    bRectSv             = bRect;
    bsRectSv            = bsRect;
    bandYminSv          = bandYmin;
    bandYmaxSv          = bandYmax;
    dYminTempSv         = dYminTemp;
    dYmaxTempSv         = dYmaxTemp;
    sYminTempSv         = sYminTemp;
    sYmaxTempSv         = sYmaxTemp;
                
#endif  /* NO_REGION_CLIP */
                
    dRectSv             = dRect;
    sRectSv             = sRect;
    cRectSv             = cRect;
    FillDrawerSv        = FillDrawer;
    optPtrSv            = optPtr;
}

/***************************************************************************
* FUNCTION
*
*    BLITS_RestoreGlobals
*
* DESCRIPTION
*
*    Restores the global variables.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID BLITS_RestoreGlobals(VOID)
{
    srcBmap             = srcBmapSv;
    dstBmap             = dstBmapSv;
    
#ifndef NO_IMAGE_SUPPORT
    srcImag             = srcImagSv;
#endif  /* NO_IMAGE_SUPPORT */

    dstPtr              = dstPtrSv;
    srcPtr              = srcPtrSv;
    
#ifdef INCLUDE_16_BIT
    dstPtr16            = dstPtr16Sv;
    srcPtr16            = srcPtr16Sv;
#endif /* INCLUDE_16_BIT */

    srcNextRow          = srcNextRowSv;
    dstNextRow          = dstNextRowSv;
    srcPixBytes         = srcPixBytesSv;
    srcBgnByte          = srcBgnByteSv;
    dstBgnByte          = dstBgnByteSv;
    dstClass            = dstClassSv;
    lineCntM1           = lineCntM1Sv;
    byteCntM1           = byteCntM1Sv;

#if (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT) || defined(INCLUDE_8_BIT))
    pnColr              = pnColrSv;
    bkColr              = bkColrSv;
#endif /* (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT || defined(INCLUDE_8_BIT))) */

#ifdef INCLUDE_16_BIT
    pnColr16            = pnColr16Sv;
    bkColr16            = bkColr16Sv;
#endif /* INCLUDE_16_BIT */

#ifdef INCLUDE_24_BIT
    pnColr24            = pnColr24Sv;
    bkColr24            = bkColr24Sv;
#endif /* INCLUDE_24_BIT */

#ifdef INCLUDE_32_BIT
    pnColr32            = pnColr32Sv;
    bkColr32            = bkColr32Sv;
#endif /* INCLUDE_32_BIT */

#if (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT))
    shfCnt              = shfCntSv;
    flipMask            = flipMaskSv;
    shiftMask           = shiftMaskSv;
#endif /* (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT)) */
    
#ifndef NO_REGION_CLIP
                
    clipToRegionFlag    = clipToRegionFlagSv;
                
#endif  /* NO_REGION_CLIP */
                
    clipToRectFlag      = clipToRectFlagSv;
    lclbfYXBanded       = lclbfYXBandedSv;

#ifdef  THIN_LINE_OPTIMIZE
    drawStat            = drawStatSv;
#endif  /* THIN_LINE_OPTIMIZE */
    
#ifndef NO_REGION_CLIP
                
    nextRegionPtr       = nextRegionPtrSv;
    nextRegionPtrB      = nextRegionPtrBSv;
    tempRegionPtr       = tempRegionPtrSv;
    bRect               = bRectSv;
    bsRect              = bsRectSv;
    bandYmin            = bandYminSv;
    bandYmax            = bandYmaxSv;
    dYminTemp           = dYminTempSv;
    dYmaxTemp           = dYmaxTempSv;
    sYminTemp           = sYminTempSv;
    sYmaxTemp           = sYmaxTempSv;

#endif  /* NO_REGION_CLIP */

    dRect               = dRectSv;
    sRect               = sRectSv;
    cRect               = cRectSv;
    FillDrawer          = FillDrawerSv;
    optPtr              = optPtrSv;
}

#endif /* USE_CURSOR */
