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
*   blits.h
*
* DESCRIPTION
*
*  This file contains the Blit support global variables and structures.
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
#ifndef _BLITS_H_
#define _BLITS_H_

/* Local Functions */
VOID BLITS_SaveGlobals(VOID);
VOID BLITS_RestoreGlobals(VOID);


extern rect    clpRect;             /* Clip rect */
extern rect    srcRect;             /* source rectangle */
extern rect    dstRect;             /* destination rectangle */
extern INT32   imagRowWords;        /* (rowBytes+1)/2 */
extern INT32   srcY;
extern INT32   srcNextRow;          /* inc from end of row to start of next */
extern INT32   srcPixBytes;         /* # bytes across source bitmap */
extern image   *srcImag;            /* pointer to source image */
extern VOID    (*jmpOff)(VOID);     /* offset to internal optimization procedure */
extern INT32   srcOff;              /* source increment offset 0(even)/1(odd) */
extern grafMap *srcBmap;            /* ptr to src bitmap */
extern INT32   srcBgnByte;          /* Source byte begin */
extern signed char maskFirst,maskLast;  /* masks for first & last bytes on scan lines, */
extern signed char ropALU;              /* gDataRot field for current raster op */
extern signed char readLast;            /* 1 if source byte needs to be read before
                                           writing last dest byte */
extern signed char loadTwo;             /* 1 if 2 source bytes must be read before
                                           writing first dest byte */
extern signed char fullRopALU;          /* ropALU with shift field also set */
extern signed char flagTToB;            /* bit 0 set for top->bottom bits, reset
                                        otherwise; bit 1 set for left->right
                                        blits, reset otherwise */
extern INT32   shfCnt;              /* shift count (-=shfRt, +=shfLf) */
extern INT32   flipMask;            /* 0xffff for NOT SOURCE rops, 0 else */
extern INT32   shiftMask;           /* mask used to mix adjacent source bytes
                                       before ORing together writing rotated
                                       to dest */
extern INT32   srcWidthBase;        /* source bitmap width */
extern INT32   srcWidth,dstWidth;   /* temp storage for +/- source/dest bitmap widths */
extern INT32   middleWidth;         /* # of solid bytes per row */
extern INT32   middleSrcAdvance;    /* # of bytes from end of one middle */
extern INT32   middleDstAdvance;    /*  row blit to start of next row */
extern INT32   firstOffset;         /* 1 for left->right copies, -1 for
                                       right->left copies */
extern INT32   lastOffset;          /* middleWidth + 1 for left->right copies.
                                       -middleWidth - 1 for right->left */
extern INT32   srcBlockAdvance;     /* distance to the start of the next source
                                       block of rows */
extern INT32   dstBlockAdvance;     /* distance to the start of the next dest
                                       block of rows */
extern signed char drawFirstByte;   /* 1 if first byte on line != 0xff, 0 if it is */
extern signed char drawLastByte;    /* 1 if last byte on line != 0xff, 0 if it is */
extern signed char RNMOddByte;      /* 1 if width is odd in RNM3, 0 if width is even */
extern signed char copyLeftToRight; /* 1 if copy goes left->right, 0 for right->left */
extern INT32   oldMask,newMask;     /* masks used by RNM3 */
extern INT32   RNMInlineEntry;      /* vector used by RNM3 */
extern INT32   RNMPushEntry;        /* vector used by RNM3 */
extern INT32   RNMExitVector;       /* vector used by RNM3 */
extern INT32   RNMLoopVector;       /* vector used by RNM3 */
extern SIGNED  *rowTablePtr[4];     /* pointers to row table entries */
extern INT32   rowTableSkip;        /* used to move forward or backward through
                                       row tables */
extern INT32   RNMNarrowVecto;      /* for narrow solid middle bytes, the entry
                                       point in the narrow handler in-line code,
                                       otherwise just a continuation vector */
extern UINT8   *stackBuffer;        /* pointer to temporary buffer for one-bank blits */
extern INT32   baseBankComboVector; /* copy of bankComboVector for multiplane use */
extern signed char multiplaneFlag;  /* 1 if multiplane copy in progress, 0 if single */
extern signed char isImage;         /* 1 if image copy in process, 0 if blit */
extern INT32   lclBlitRop;          /* local copy of blitRop */
extern INT32   lclPixPlanes;        /* # of planes to process */
extern SIGNED  *temp1,*temp2;       /* general temporary storage */
extern SIGNED  fullPnColr,fullBkColr; /* 32-bit fg/bg */
extern SIGNED  tempPnColr,tempBkColr; /* working 32-bit fg/bg */
extern INT32   initialSourceY;      /* starting src row for N->N blits */
extern INT32   initialDestY;        /* starting dest row for 1->N and N->N blits */
extern INT32   replaceVector;       /* aligned/non-aligned replace vector in
                                       1->N blits */
extern UINT8   *dstPtr;             /* pointer to dest mem for planar->E/VGA */
extern UINT8   *srcPtr;             /* pointer to src mem for planar->E/VGA */
extern UINT16  *dstPtr16;           /* pointer to dest 16-bit mem */
extern UINT16  *srcPtr16;           /* pointer to src 16-bit mem */
extern UINT8   lclPenColor;         /* local pen color for line */
extern UINT16  lclPenColor16;       /* local pen color for line */
extern UINT8   lclPenColorR;        /* local red pen color for line */
extern UINT8   lclPenColorG;        /* local green pen color for line */
extern UINT8   lclPenColorB;        /* local blue pen color for line */
extern UINT8   lclPenColorA;        /* local alpha pen color for line */

#endif /* _BLITS_H_ */


