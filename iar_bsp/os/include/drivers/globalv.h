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
*  globalv.h
*
* DESCRIPTION
*
*  This file contains the externs of the common variable for Fill routines.
*
* DATA STRUCTURES
*
*  None
*
* FUNCTIONS
*
*  None
*
* DEPENDENCIES
*
*  None
*
*************************************************************************/
#ifndef _GLOBALV_H_
#define _GLOBALV_H_

/* Global Variables for MB functions */
extern INT32   BMHeight;           /* height of bitmap  */
extern INT32   patRep;             /* every-8th-line count for patterns  */
extern INT32   patRepDec;          /* adjust counter for patRep  */
extern pattern *patStart;          /* offset of pattern start  */
extern UINT8   *ptrnData;          /* offset of pattern start  */
extern INT32   patCnt;             /* counts down drawing of pattern bytes  */
extern UINT8   *patPastMax;        /* marks when pattern pointer must wrap  */
extern INT32   GCCurrentResetTable;/* offset of table to look up reg
                                      reset values in when done  */
extern signed char    localRop,pass2,patSetResetEnable,ropIsReplace;
                            /* local copy of raster op, flag that's 1 when
                               2nd pass needed, or 0 otherwise (must not
                               be any other value!), mono pattern set/reset
                               enable value, 1 if raster op is rep or nrep,
                               0 if it's any other rop (for FILL9)  */

extern signed char    nonInvertPlanes,invertPlanes,onePassMask,multiExit;
                            /* if 2-pass, planes to write on first,non-
                               inverted, pass & second, inverted pass,mask used
                               only for 1-pass that's0 if pattern can be used
                               as is,0ffh if pattern must be flipped  */

extern INT32   patBytesToRight;    /* # of bytes from pattern starting
                                      point (accounting for start X)
                                      until wrap. 0 means the start byte
                                      is the UINT8 at the right edge of the pattern  */

extern INT32   patWidthInBytes;    /* # of bytes across pattern (pattern
                                      width must equal patWidthInBytes*8,
                                      with no wasted pixels at the left  */

extern INT32   patHeight;          /* # of bytes from pattern top->bottom  */

extern INT32   patLength;          /* # of bytes in pattern  */

extern INT32   tempDstNextRow;     /* possibly-modified version of dstNextRow used
                                      by SingleColumn that is correct for 1-column-wide drawing */

extern INT32   patPlaneOffset;     /* # of bytes from one plane to the
                                      next in current multicolor pattern  */
extern INT32   fullLeftMask,fullRightMask;  /* masks used for combining in FILL3  */
extern pattern   *savePat;           /* pattern image */

 /* Global variable externs */
extern INT32   lineCntM1;              /* line count (minus 1, dYmax-dYmin) */
extern INT32   dstNextRow;             /* inc from end of row to start of next */
extern INT32   byteCnt,bytCnt;         /* Destination UINT8 count */
extern SIGNED  *listPtr;               /* ptr to FILLREC.blitList */
extern VOID   (*optPtr)(VOID);         /* ptr to BLIT optimization routine */
extern grafMap *dstBmap;               /* ptr to dst bitmap */
extern INT32   rectCnt;                /* FILLREC.blitCnt */
extern SIGNED  lclPortMask;            /* working plane mask */
extern INT32   cXmin, cYmin, cXmax, cYmax; /* Clip rect */
extern INT32   dXmin, dYmin, dXmax, dYmax; /* Destination rect */
extern SIGNED  *dstRowTablePtr;         /* pointer to destination row table */
extern UINT32  pnColr, bkColr;          /* background and foreground colors */
extern UINT32  pnColr16, bkColr16;      /* background and foreground 16-bit colors */
extern UINT32  pnColr24, bkColr24;      /* background and foreground 24-bit colors */
extern UINT32  pnColr32, bkColr32;      /* background and foreground 32-bit colors */
extern signed char    dstBanked;        /* 1 if destination is banked */
extern signed char    maskLf, maskRt;   /* left/right nibble mask for VGA-specific */
extern INT32   dstBgnByte;              /* offset from start of row to first pixel to fill */
extern INT32   loopCnt;                 /* Working loop counter */
extern INT32   byteCntM1;               /* Destination UINT8 count (Minus 1) */
extern INT32   dstClass;                /* Dst device class - EGA(0)/VGA(1) */
extern UINT8   *dstBgnPtr;              /* ptr to 1st UINT8 in 1st line to fill */

extern signed char clipToRegionFlag;    /* 1 if region clipping active, 0 else */
extern signed char clipToRectFlag;      /* 1 if rect clipping active, 0 else */
extern signed char lclbfYXBanded;       /* flag that's 1 if blitRcd data is
                                           YXbanded */
extern signed char bDrawStat;          /* base copy of drawStat for region
                                          clipping (drawing can toggle drawStat) */
extern signed char drawStat;           /* for region clipping */
extern rect *nextRegionPtr;     /* pointer to region rect at which to
                                   start checked for the next region clip,
                                   going top->bottom, left->right */
extern rect *nextRegionPtrB;    /* pointer to region rect at which to
                                   start checked for the next region clip,
                                   going bottom->top, right->left */
extern rect *tempRegionPtr;     /* offset portion of pointer to region
                                   rect at which to resume processing after
                                   this band in certain cases */
extern rect bRect;              /* copy of destination rect */
extern rect dRect;              /* destination rect */
extern rect sRect;              /* source rect */
extern rect cRect;              /* clipping rect */
extern INT32       rectCnt;                    /* FILLREC.blitCnt */

extern INT32 dYminTemp,dYmaxTemp;   /* temp storage for Y extent of
                                       rectangle clipped to YX region band */
extern rect bsRect;                 /* copy of source rect */
extern INT32 sYminTemp,sYmaxTemp;   /* temp storage for Y extent of
                                       source rect clipped to YX region band */
extern INT32 bandYmin;            /* Ymin and Ymax coordinates for */
extern INT32 bandYmax;            /* current YX band */
extern INT32 bcXmin,bcYmin,bcXmax,bcYmax;   /* copy of clip rect */
extern INT32 cdXmin,cdYmin,cdXmax,cdYmax;   /* copy of dest rect, clipped
                                               to clip rect */
extern INT32 blitMayOverlap;
extern INT32 isLine;
extern INT32 errTermL;            /* Bresenham's error term variable */
extern INT32 errTermAdjUpL;       /* Bresenham's error term adjustment
                                     value when no minor axis move is made */
extern INT32 errTermAdjDownL;     /* Bresenham's error term adjustment
                                     value when a minor axis move is
                                     made (non-negative) */
extern INT32 majorAxisLengthM1;   /* length of line along major axis-1 */

extern INT32 lineDir;             /* line direction after clipping:
                                     0 = r->l, t->b Ymajor
                                     1 = vertical top->bottom
                                     2 = l->r, t->b Ymajor
                                     3 = l->r, t->b diag
                                     4 = l->r, t->b Xmajor
                                     5 = horizontal left->right
                                     6 = r->l, t->b diag
                                     7 = r->l, t->b Xmajor
                                     <=3 means Ymajor, >=4 means Xmajor */

extern INT32 lineEncode;          /* line tic-tac-toe encoding */

/* Fill/Blit drawing routine */
extern VOID (*FillDrawer)(blitRcd *drwPRec);

/* Line drawing routine */
extern VOID (*LineDrawer)(VOID);

#endif /* _GLOBALV_H_ */
