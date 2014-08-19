/***************************************************************************
*
*             Copyright 2003 Mentor Graphics Corporation
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
*  globalv.c
*
* DESCRIPTION
*
*  This file contains the common variable for Fill routines.
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
*  nucleus.h
*  nu_kernel.h
*  nu_drivers.h
*
***************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "drivers/nu_drivers.h"

/* Fill/Blit drawing routine */
VOID (*FillDrawer)(blitRcd *drwPRec);

INT32       lineCntM1;                  /* line count (minus 1, dYmax-dYmin) */
INT32       dstNextRow;                 /* inc from end of row to start of next */
INT32       byteCnt;                    /* Destination UINT8 count */
INT32       *listPtr;                   /* ptr to FILLREC.blitList */
VOID        (*optPtr)(VOID);            /* ptr to BLIT optimization routine */
grafMap     *dstBmap;                   /* ptr to dst bitmap */
INT32       rectCnt;                    /* FILLREC.blitCnt */
INT32       lclPortMask;                /* working plane mask */

#if (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT) || defined(INCLUDE_8_BIT))
UINT32      pnColr, bkColr;             /* background and foreground colors */
#endif /* (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT || defined(INCLUDE_8_BIT))) */

#ifdef INCLUDE_16_BIT
UINT32      pnColr16, bkColr16;         /* background and foreground 16-bit colors */
#endif /* INCLUDE_16_BIT */

#ifdef INCLUDE_24_BIT
UINT32      pnColr24, bkColr24;         /* background and foreground 24-bit colors */
#endif /* INCLUDE_24_BIT */

#ifdef INCLUDE_32_BIT
UINT32      pnColr32, bkColr32;         /* background and foreground 32-bit colors */
#endif /* INCLUDE_32_BIT */

INT32       dstBgnByte;                 /* offset from start of row to first pixel to fill */
INT32       byteCntM1;                  /* Destination UINT8 count (Minus 1) */
INT32       dstClass;                   /* Dst device class - EGA(0)/VGA(1) */

#ifdef  THIN_LINE_OPTIMIZE
                                           
signed char drawStat;                   /* for region clipping */

#endif  /* THIN_LINE_OPTIMIZE */

signed char clipToRectFlag;             /*1 if rect clipping active, 0 else */
signed char lclbfYXBanded;              /* flag that's 1 if blitRcd data is
                                           YXbanded */

#ifndef NO_REGION_CLIP
                
signed char clipToRegionFlag;           /* 1 if region clipping active, 0 else */
rect        *nextRegionPtr;             /* pointer to region rect at which to
                                           start checked for the next region clip,
                                           going top->bottom, left->right */
rect        *nextRegionPtrB;            /* pointer to region rect at which to
                                           start checked for the next region clip,
                                           going bottom->top, right->left */
rect        *tempRegionPtr;             /* offset portion of pointer to region
                                           rect at which to resume processing after
                                           this band in certain cases */
INT32       dYminTemp,dYmaxTemp;        /* temp storage for Y extent of
                                           rectangle clipped to YX region band */
rect        bsRect;                     /* copy of source rect */
INT32       sYminTemp,sYmaxTemp;        /* temp storage for Y extent of
                                           source rect clipped to YX region band */
INT32       bandYmin;                   /* Ymin and Ymax coordinates for */
INT32       bandYmax;                   /* current YX band */
rect        bRect;                      /* copy of destination rect */

#endif  /* NO_REGION_CLIP */

rect        dRect;                      /* destination rect */
rect        sRect;                      /* source rect */
rect        cRect;                      /* clipping rect */
  
#ifdef  THIN_LINE_OPTIMIZE

/* Line drawing routine */
VOID (*LineDrawer)(VOID);

#ifndef NO_REGION_CLIP

INT32       bcXmin,bcYmin,bcXmax,bcYmax;/* copy of clip rect */
INT32       cdXmin,cdYmin,cdXmax,cdYmax;/* copy of dest rect, clipped
                                           to clip rect */

#endif  /* NO_REGION_CLIP */
                              
INT32       errTermL;                   /* Bresenham's error term variable */
INT32       errTermAdjUpL;              /* Bresenham's error term adjustment
                                           value when no minor axis move is made */
INT32       errTermAdjDownL;            /* Bresenham's error term adjustment
                                           value when a minor axis move is
                                           made (non-negative) */
INT32       majorAxisLengthM1;          /* length of line aINT32 major axis-1 */

INT32       lineDir;                    /* line direction after clipping:
                                            0 = r->l, t->b Ymajor
                                            1 = vertical top->bottom
                                            2 = l->r, t->b Ymajor
                                            3 = l->r, t->b diag
                                            4 = l->r, t->b Xmajor
                                            5 = horizontal left->right
                                            6 = r->l, t->b diag
                                            7 = r->l, t->b Xmajor
                                            <=3 means Ymajor, >=4 means Xmajor */
INT32       lineEncode;                 /* line tic-tac-toe encoding */

#endif  /* THIN_LINE_OPTIMIZE */

INT32       srcNextRow;                 /* inc from end of row to start of next */
INT32       srcPixBytes;                /* # bytes across source bitmap */

#ifndef NO_IMAGE_SUPPORT
image       *srcImag;                   /* pointer to source image */
#endif  /* NO_IMAGE_SUPPORT */

grafMap     *srcBmap;                   /* ptr to src bitmap */
INT32       srcBgnByte;                 /* Source UINT8 begin */

#if (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT))
INT32       shfCnt;                     /* shift count (-=shfRt, +=shfLf) */
INT32       flipMask;                   /* 0xffff for NOT SOURCE rops, 0 else */
INT32       shiftMask;                  /* mask used to mix adjacent source bytes
                                           before ORing together writing rotated
                                           to dest */
INT32       firstOffset;                /* 1 for left->right copies, -1 for
                                           right->left copies */
#endif /* (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT)) */

INT32       dstWidth;                   /* temp storage for +/- source/dest bitmap widths */
INT32       *rowTablePtr[4];            /* pointers to row table entries */

UINT8       *dstPtr;                    /* pointer to dest mem for planar->E/VGA */
UINT8       *srcPtr;                    /* pointer to src mem for planar->E/VGA */

#ifdef INCLUDE_16_BIT
UINT16      *dstPtr16;                  /* pointer to dest 16-bit mem */
UINT16      *srcPtr16;                  /* pointer to src 16-bit mem */
#endif /* INCLUDE_16_BIT */

#if (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT) || defined(INCLUDE_8_BIT))
UINT8       lclPenColor;                /* local pen color for line */
#endif /* (defined(INCLUDE_1_BIT) || defined(INCLUDE_2_4_BIT || defined(INCLUDE_8_BIT))) */

#ifdef INCLUDE_16_BIT
UINT16      lclPenColor16;              /* local pen color for line */
#endif /* INCLUDE_16_BIT */

#ifdef  THIN_LINE_OPTIMIZE

UINT8       lclPenColorR;               /* local red pen color for line */
UINT8       lclPenColorG;               /* local green pen color for line */
UINT8       lclPenColorB;               /* local blue pen color for line */

#ifdef INCLUDE_32_BIT
UINT8       lclPenColorA;               /* local alpha pen color for line */
#endif /* INCLUDE_32_BIT */

#endif  /* THIN_LINE_OPTIMIZE */

#ifdef  FILL_PATTERNS_SUPPORT

/* Global Variables for MB functions */
pattern *patStart;          /* offset of pattern start  */
UINT8   *ptrnData;          /* offset of pattern start  */

INT32   patWidthInBytes;    /* # of bytes across pattern (pattern
                               width must equal patWidthInBytes*8,
                               with no wasted pixels at the left  */

INT32   patHeight;          /* # of bytes from pattern top->bottom  */

INT32   patLength;          /* # of bytes in pattern  */

INT32   patPlaneOffset;     /* # of bytes from one plane to the
                               next in current multicolor pattern  */
pattern  *savePat;

#endif /* FILL_PATTERNS_SUPPORT */

