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
*  regiona.c                                                    
*
* DESCRIPTION
*
*  This file contains region functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  REGIONA_rsRectangleListConversionSort
*  OffsetRegion
*  OpenRegion
*  REGIONA_rsRegionCaptureRectangle
*  ClipRegion
*  NullRegion
*  EmptyRegion
*  DestroyRegion
*  EqualRegion
*  RectListToRegion
*  RectRegion
*  SetRegion
*  CloseRegion
*  DupRegion
*  BitmapToRegion
*
* DEPENDENCIES
*
*  rs_base.h
*  rs_api.h
*  regiona.h
*  memrymgr.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rs_api.h"
#include "ui/regiona.h"
#include "ui/memrymgr.h"

#ifndef NO_REGION_CLIP

/* Region Capture support */
/* capture buffer pointer */
static rect *regCapPtr;        

/* capture buffer size */
static DEFN regCapSize;        

/* current capture index */
static DEFN regCapNdx;         

/* fill primitive pointer save area */
static VOID (*regSaveFill)();  


/***************************************************************************
* FUNCTION
*
*    RectListToRegion
*
* DESCRIPTION
*
*    Function RectListToRegion processes a list of unsorted rectangles into a
*    region definition.  NUMRECTS specifies the number of rectangles contained 
*    in the rectangle list, RECTLIST.  If an error occurs during RectListToRegion
*    (e.g. insufficient memory) a QueryError() status is posted and a NULL pointer
*    is returned.
*
*    Memory for the new region definition is allocated dynamically.  When finished using
*    the region, the program should release the memory occupied by the region definition.
*
* INPUTS
*
*    INT32 NUMRECTS - Number of rectangles.
*
*    rect *RECTLIST - Pointer to the list of rectangles.
*
* OUTPUTS
*
*    region - Returns the region.
*
***************************************************************************/
region *RectListToRegion( INT32 NUMRECTS, rect *RECTLIST)
{
    INT16  Done = NU_FALSE;
    INT32  regionSize;
    INT32  rectSize;
    rect   *rectPtr;
    region *regionPtr;
    INT16  GrafErrValue;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    rectSize = NUMRECTS * sizeof(rect);

    /* Allocate a buffer to sort into  */
    rectPtr = MEM_malloc(rectSize);

    if( !rectPtr)
    {
        GrafErrValue = c_RectList + c_OutofMem;
        nuGrafErr(GrafErrValue, __LINE__, __FILE__); 
        regionPtr = 0;
        Done = NU_TRUE;
    }

    if( !Done )
    {
        /* Sort rectangle list to temp buffer */
        REGIONA_rsRectangleListConversionSort(NUMRECTS, RECTLIST, rectPtr);

        regionSize = (UINT32) REGMERGE_rsMergeRegion(NUMRECTS, rectPtr, 0, 0, 0, 0, 0);

        /* Allocate space for region  */
        regionPtr = MEM_malloc(regionSize);

        if( regionPtr == NU_NULL )
        {
            GrafErrValue = c_RectList + c_OutofMem;
            nuGrafErr(GrafErrValue, __LINE__, __FILE__); 
            GRAFIX_Deallocation(rectPtr);
            regionPtr = 0;
            Done = NU_TRUE;
        }
    }

    if( !Done )
    {
        REGMERGE_rsMergeRegion(NUMRECTS, rectPtr, 0, 0, regionSize, regionPtr, 0);

        /* Release sorted rectlist */
        GRAFIX_Deallocation(rectPtr);
    }
    
    /* Return to user mode */
    NU_USER_MODE();

    /* Return region pointer */
    return(regionPtr);
}

/***************************************************************************
* FUNCTION
*
*    REGIONA_rsRectangleListConversionSort
*
* DESCRIPTION
*
*    Function REGIONA_rsRectangleListConversionSort is the rectangle list 
*    conversion to global coordinates / YX sorting function. The passed-in 
*    rectangle list is sorted either in place (if DESTRECTS is NULL) or 
*    copied to the destination list first (if DESTRECTS is non-NULL).  
*    The primary sort key is upper left corner Y coordinate; the secondary
*    key is upper left corner X coordinate. This prepares the list to be 
*    fed to RectListToRegion to describe a region.
*
* INPUTS
*
*    INT32 numRects  - Number of rectangles.
*
*    rect *srcRects  - Pointer to the source rectangles.
*
*    rect *destRects - Pointer to the destination rectangles.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID REGIONA_rsRectangleListConversionSort( INT32 numRects, rect *srcRects, rect *destRects)
{
    INT16 Done = NU_FALSE;
    INT32 rectCnt;
    rect *rectEnd;
    INT32 hSpan;
    rect *hSortedStart;
    rect *listToSort;
    INT32 tempa, tempb;
    INT32 minX, minY, maxX, maxY;
    rect *temRectSt;
    rect *temRect;
    rect *temSave;
    rect *snPtr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Point to destination */
    if( destRects == 0 )
    {
        temRect = srcRects;
    }
    else
    {
        temRect = destRects;
        temSave = srcRects;
        for( rectCnt = 0; rectCnt < numRects; rectCnt++)
        {
            *destRects++ = *temSave++;
        }
        destRects = temRect;

        /* Set NotUsed to remove warnings */
        NU_UNUSED_PARAM(destRects);
    }

    /* Convert all rects to global */
    listToSort = temRect;

    if( numRects < 2 )
    {
        /* task termination */
        Done = NU_TRUE; 
    }

    if( !Done )
    {
        rectCnt = numRects;

        /* Convert to global loop  */
        if( globalLevel > 0 )
        {
            do
            {
                U2GR(*temRect, temRect,0);
                temRect++;
            }while( --rectCnt > 0 );
        }

        /* Already in global all rects are converted then sort them */
        rectEnd = listToSort + numRects;
        tempb = 1; 

        /* Calculate the initial sort skip distance the largest
           value in sequence 1, 4, 13, 40, 121, 364 ... that's
           less than the number of rects  */
        do
        {
            tempa = tempb;
            tempb = (3 * tempb) + 1;
        } while( (tempb < numRects) );

        /* Loop through, shorting all spaced, subfiles for each 
           at which point the rects are sorted  */
        do
        {
            hSpan = tempa;
            hSortedStart = listToSort + hSpan;
            temRect = hSortedStart;
            do
            {
                /* Perform an insertion sort on the current element
                   within its space subfile  */
                minX = temRect->Xmin;
                minY = temRect->Ymin;
                maxX = temRect->Xmax;
                maxY = temRect->Ymax;
                temRectSt = temRect;
                snPtr = temRect;        
                do
                {
                    snPtr -= hSpan;
                    if( (snPtr->Ymin < minY) || ((snPtr->Ymin == minY)
                        && (snPtr->Xmin <= minX)))
                    {
                        break;
                    }

                    *temRect = *snPtr;
                    temRect = snPtr;
                }while( temRect >= hSortedStart );

                temRect->Xmin = minX;
                temRect->Ymin = minY;
                temRect->Xmax = maxX;
                temRect->Ymax = maxY;
                temRect = temRectSt + 1;
            }while( temRect != rectEnd );

            tempa = tempa / 3;

        }while( tempa > 0 );
    
    } /* if( !Done ) */

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    ClipRegion
*
* DESCRIPTION
*
*    Function ClipRegion enables the specified region as the current clip region.
*    Once a region is made the current clipping region (via ClipRegion(),
*    the user must not deallocate that memory or modify the region until
*    that region is no longer the current clip region. Simply
*    maintains a pointer to the current clip region, and uses the rects
*    directly out of the region, so the region must be available for
*    however long it is the clip region, and should not be modified in any
*    way during that time.
*
*    Check to see if the rfRect flag is set; if it is, the region is
*    rectangular, so don't do regions.
*
*    In this case, the normal clip rect could be set to the intersection
*    of the normal clip rect and the region bounding rect, and region
*    clipping wouldn't be turned on at all.  This would make clipping
*    happen *much* faster.  However, it would require setting a flag to
*    indicate the "square region" condition; if the user set a new clip
*    rect, the intersection of the new clip rect and the region rect would
*    have to be recalculated, and if the user set a new region, the normal
*    clip rect would have to be restored.  I guess there aren't going to
*    be too many rectangular regions anyway, so no harm done if we skip
*    this for now; it just seemed like a shame to do region clipping when
*    it's really only rectangular clipping.
*
*    Validating the region's YX band list in ClipRegion.
*
*    Check that the rect at the address [rgnList]-(size rect) is a top sentinel
*    (all fields == rgnBOS), and the rect at address [rgnListEnd]+(size rect) is
*    a bottom sentinel (all fields == rgnEOS).  Then, check that for all rects 
*    starting at [rgnList] and continuing through the rect at [rgnListEnd].
*
* INPUTS
*
*    region *RGN  - Pointer to the region.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID ClipRegion(region *RGN)
{
    INT16 Done = NU_FALSE;
    rect  *ptr;
    INT16 temp;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( RGN != 0 )
    {
        /* <<make sure region isn't empty first!>> */
        ptr = RGN->rgnList - 1;
        if( ptr && (ptr->Ymax != rgnBOS ))
        {
            Done = NU_TRUE;
        }

        ptr = RGN->rgnListEnd + 1;         
        if( ptr && (ptr->Ymin != rgnEOS ))
        {
            Done = NU_TRUE;
        }
        temp = theGrafPort.portFlags | pfRgnClip;
    }
    else
    {
        temp = theGrafPort.portFlags & (~pfRgnClip);
    }

    if( !Done )
    {
        /* It's a valid region, turn on the grafPort  */
        thePort->portRegion = RGN;
        theGrafPort.portRegion = RGN;

        thePort->portFlags = temp;
        theGrafPort.portFlags = temp;

        /* Set the default blit record region clipping info  */
        grafBlit.blitRegn = RGN;

        if( (temp & pfRgnClip) != 0 )
        {
            grafBlit.blitFlags = grafBlit.blitFlags | bfClipRegn;
        }
        else
        {
            grafBlit.blitFlags = grafBlit.blitFlags & (~bfClipRegn);
        }

    } /* if( !Done ) */

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    OffsetRegion
*
* DESCRIPTION
*
*    Function OffsetRegion offsets the regions rectangle list and bounding box.
*
* INPUTS
*
*    region *RGN  - Pointer to the region.
*
*    INT32 dltX  - Delta x.
*
*    INT32 dltY  - Delta y.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID OffsetRegion(region *RGN, INT32 dltX, INT32 dltY)
{   
    rect *ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( (theGrafPort.portFlags & pfUpper) == 0 )
    {
        dltY = - dltY;
    }
    if( (theGrafPort.portFlags & pfVirtual) != 0 )
    {
        V2GSIZE(dltX,dltY,&dltX,&dltY);
    }

    if( (RGN->rgnFlags & rfNull) == 0 )
    {
        /* Update region bounding box  */       
        RGN->rgnRect.Xmin += dltX;
        RGN->rgnRect.Xmax += dltX;
        RGN->rgnRect.Ymin += dltY;
        RGN->rgnRect.Ymax += dltY;

        ptr = RGN->rgnList;
        do
        {
            ptr->Xmin += dltX;
            ptr->Xmax += dltX;
            ptr->Ymin += dltY;
            ptr->Ymax += dltY;
            ptr++;
        }while( ptr->Xmin != rgnEOS ); 
    }

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    SetRegion
*
* DESCRIPTION
*
*    Function SetRegion creates a region that describes a single
*    rectangle defined by two diagonal end-points, (X1,Y1) and (X2,Y2).  If an
*    error occurs during RectRegion() (e.g. insufficient memory), a QueryError()
*    status is posted and a NULL pointer is returned.
*
* INPUTS
*
*    INT32 X1 - Starting point X.
*
*    INT32 Y1 - Starting point Y.
*
*    INT32 X2 - Ending point X.
*
*    INT32 Y2 - Ending point Y.
*
* OUTPUTS
*
*    region    - The region.
*
***************************************************************************/
region *SetRegion( INT32 X1, INT32 Y1, INT32 X2, INT32 Y2)
{
    region *RectListToRegion(INT32 NUMRECTS, rect *RECTLIST);
    rect   temRect;
    INT32  tem;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( X1 > X2 )
    {
        tem = X1;
        X1 = X2;
        X2 = tem;
    }
    if( Y1 > Y2 )
    {
        tem = Y1;
        Y1 = Y2;
        Y2 = tem;
    }

    temRect.Xmin = X1;
    temRect.Xmax = X2; 
    temRect.Ymin = Y1;
    temRect.Ymax = Y2;

    if( globalLevel > 0 )
    {
        U2GR(temRect, &temRect, 0);
    }

    /* Return to user mode */
    NU_USER_MODE();

    return( (region *) RectListToRegion(1,&temRect) );
}

/***************************************************************************
* FUNCTION
*
*    RectRegion
*
* DESCRIPTION
*
*    Function RectRegion creates a region that describes a single input
*    rectangle, SRCRECT.  If an error occurs during RectRegion() (e.g.
*    insufficient memory), a QueryError() status is posted and a NULL pointer
*    is returned.
*
*    Memory for the new region definition is allocated dynamically. When finished
*    using the region, the program release the memory occupied by the region definition.
*
* INPUTS
*
*    rect *srcRECT - Pointer to the source rectangle.
*
* OUTPUTS
*
*    region    - Pointer to the region definition.
*
***************************************************************************/
region *RectRegion(rect *srcRECT)
{
    region *RectListToRegion(INT32 NUMRECTS, rect *RECTLIST);
    rect temRect;
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( globalLevel > 0)
    {
        U2GR(*srcRECT, &temRect,0);
    }
    else
    {
        temRect = *srcRECT;
    }

    /* Return to user mode */
    NU_USER_MODE();

    return( (region *) RectListToRegion(1,&temRect) );
}

/***************************************************************************
* FUNCTION
*
*    OpenRegion
*
* DESCRIPTION
*
*    Function OpenRegion allocates a capture buffer and re-vectors the fill 
*    drawing primitive of the active grafMap to a routine that captures the
*    rectangle lists (passed to these primitives) to the capture buffer.
*
*    Lines are forced wide such that they will always generate rect lists that
*    call the filler.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID OpenRegion(VOID)
{   
    INT16 Done = NU_FALSE;
    INT16 grafErrValue;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

   /* Test for already opened region */
    if( (gFlags & gfRgnOpen) != 0 )
    {
        grafErrValue = (INT16) (c_OpenRegi + c_RgnOflow);
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
        Done = NU_TRUE;
    }

    if( !Done )
    {
        /* Find how big of a capture buffer we can get  */
        regCapPtr = MEM_malloc(0x8000);

        /* Test allocate pointer for null */
        if( !regCapPtr )
        {
            grafErrValue =(INT16) (c_OpenRegi + c_OutofMem);
            nuGrafErr(grafErrValue, __LINE__, __FILE__); 
            Done = NU_TRUE;
        }
    } /* if( !Done ) */

    if( !Done )
    {
        regCapSize = 0x8000;
        regCapNdx = 0;

        /* Replace fill prim with the capture processor  */
        regSaveFill = theGrafPort.portMap->prFill;
        theGrafPort.portMap->prFill = (VOID (*)()) REGIONA_rsRegionCaptureRectangle;

        /* Force lines to go wide so they will call the filler  */
        regPenFlags = theGrafPort.pnFlags;

        if( (regPenFlags & pnSizeFlg) == 0 )
        {
            theGrafPort.pnSize.X = 1;
            theGrafPort.pnSize.Y = 1;
            theGrafPort.pnFlags = (theGrafPort.pnFlags | pnSizeFlg);

            /* refresh current line style vector */     
            SETLINESTYLE(theGrafPort.pnFlags);
        }

        gFlags = (gFlags | gfRgnOpen);
    } /* if( !Done ) */

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    REGIONA_rsRegionCaptureRectangle
*
* DESCRIPTION
*
*    Function REGIONA_rsRegionCaptureRectangle fills capture.
*
* INPUTS
*
*    blitRcd *BlitRec - Pointer to the blitRcd.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID REGIONA_rsRegionCaptureRectangle(blitRcd *BlitRec)
{
    INT16 tem;
    INT16 grafErrValue;
    rect *regRectPtr;
    rect *regBlitPtr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    tem = (INT16) (regCapNdx + ((BlitRec->blitCnt << 3) * 2));
    if( tem > regCapSize )
    {
        grafErrValue = (INT16) (c_OpenRegi + c_RgnOflow);
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
    }
    else
    {
        /* move the data */
        regRectPtr = (rect *) ((SIGNED)regCapPtr + regCapNdx);
        regCapNdx = tem;
        regBlitPtr = (rect *) BlitRec->blitList;

        for( tem = 0; tem < BlitRec->blitCnt; tem++)
        {
            *regRectPtr++ = *regBlitPtr++;
        }
    }

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    CloseRegion
*
* DESCRIPTION
*
*    Function CloseRegion allocates a region buffer, converts the capture buffer to the
*    region buffer,  deallocates the capture buffer, and restores the fill
*    and line drawing of the active grafMap. It returns a pointer to the allocated
*    region buffer that the user should deallocate memory when they no longer
*    need it.
*
* INPUTS
*
*    blitRcd *BlitRec - Pointer to the blitRcd.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
region *CloseRegion(VOID)
{
    INT16  Done = NU_FALSE;
    INT32  regSize;
    region *regPtr;
    INT16  grafErrValue;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( (gFlags & gfRgnOpen) == 0 )
    {
        grafErrValue = (INT16) (c_CloseReg + c_RgnOflow);
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
        regPtr = 0;
        Done   = NU_TRUE;
    }

    if( !Done )
    {
        /* Restore grafMap operations */
        /* If the pen was thin, store to the thin pen */
        if( (regPenFlags & pnSizeFlg) == 0 )
        {
            theGrafPort.pnFlags  = (theGrafPort.pnFlags & ~pnSizeFlg);
            theGrafPort.pnSize.X = 0;
            theGrafPort.pnSize.Y = 0 ;
            thePort->pnFlags  = (thePort->pnFlags & ~pnSizeFlg);
            thePort->pnSize.X = 0;
            thePort->pnSize.Y = 0;

            /* Reset current line vector */
            SETLINESTYLE(theGrafPort.pnFlags);
        }

        /* Pen is wide */
        theGrafPort.portMap->prFill = regSaveFill;

        /* sort capture buffer rect list in place */
        regCapNdx = regCapNdx >> 3;

        /* already in global */
        globalLevel--; 

        REGIONA_rsRectangleListConversionSort((regCapNdx / 2), regCapPtr, 0 );

        globalLevel++;

        /* Compute region size  */
        regSize = REGMERGE_rsMergeRegion((INT32)(regCapNdx / 2), regCapPtr, 0, 0, 0, 0, 0);

        /* Allocate region buffer  */
        regPtr = MEM_malloc(regSize);

        if( !regPtr )
        {
            grafErrValue = (INT16) (c_CloseReg + c_OutofMem);
            nuGrafErr(grafErrValue, __LINE__, __FILE__);
            Done = NU_TRUE; 
        }

    } /* if( !Done ) */

    if( !Done )
    {
        /* Convert rect list to region */
        REGMERGE_rsMergeRegion( (INT32)(regCapNdx / 2), regCapPtr, 0, 0, regSize, regPtr , 0);

        /* Close up, free capture buffer */
        GRAFIX_Deallocation(regCapPtr);

        /* Clear open region flag */
        gFlags = gFlags & ~gfRgnOpen;
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* Return region buffer pointer */
    return(regPtr);
}

/***************************************************************************
* FUNCTION
*
*    NullRegion
*
* DESCRIPTION
*
*    Function NullRegion modifies an existing region structure such that it contains
*    no rectangle list. The original allocated size of the region is not changed.
*
* INPUTS
*
*    region *argRegion - Pointer to the region.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID NullRegion( region *argRegion)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    argRegion->rgnSize = sizeof(region) + 2 * sizeof(rect);

    /* Flag as null  */
    argRegion->rgnFlags = 0;

    /* Set bound to 0 */
    argRegion->rgnRect.Xmin = 0;
    argRegion->rgnRect.Xmax = 0;
    argRegion->rgnRect.Ymin = 0;
    argRegion->rgnRect.Ymax = 0;

    /* Set end pointer first rect in list */
    argRegion->rgnListEnd->Xmin = rgnEOS;
    argRegion->rgnListEnd->Xmax = rgnEOS;
    argRegion->rgnListEnd->Ymin = rgnEOS;
    argRegion->rgnListEnd->Ymax = rgnEOS;

    /* Set start pointer to last rect in list */
    argRegion->rgnList->Xmin = rgnBOS;
    argRegion->rgnList->Xmax = rgnBOS;
    argRegion->rgnList->Ymin = rgnBOS;
    argRegion->rgnList->Ymax = rgnBOS;

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    EmptyRegion
*
* DESCRIPTION
*
*    Function EmptyRegion checks an existing region structure 
*    to see if it is a null region.
*
* INPUTS
*
*    region *argRegion - Pointer to the region.
*
* OUTPUTS
*
*    INT32 - Returns 0 if empty.
*            Returns 1 if not empty.
*
***************************************************************************/
INT32 EmptyRegion ( region * argRegion)
{
    INT16 Done = 0;
    INT32 value;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( !(argRegion->rgnFlags & rfNull) )
    {
        value = 0;
        Done  = NU_TRUE;
    }
    
    if( !Done )
    {
        value =   argRegion->rgnRect.Xmin
                | argRegion->rgnRect.Xmax
                | argRegion->rgnRect.Ymin
                | argRegion->rgnRect.Ymax;

        if( value == 0 )
        {
            value = 1;
            Done  = NU_TRUE;
        }
    }
    
    if( !Done )
    {
        value = 0;
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    DupRegion
*
* DESCRIPTION
*
*    Function DupRegion duplicates an existing region structure, allocating new
*    memory for the duplicate region. This memory must be freed by the user.
*
* INPUTS
*
*    region *argRegion - Pointer to the region.
*
* OUTPUTS
*
*    region - Pointer to the new region.
*
***************************************************************************/
region *DupRegion( region *argRegion)
{
    region *regPtr;
    INT16  grafErrValue;
    UINT8  *regDataIn, *regDataOut;
    INT32  i;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Get used size of source region and allocate it */
    regPtr = MEM_malloc(argRegion->rgnSize);

    /* Check for null location */
    if( !regPtr )
    {
        grafErrValue = (INT16) (c_DupRegio  + c_OutofMem);
        nuGrafErr(grafErrValue, __LINE__, __FILE__); 
        regPtr = 0;
    }
    else
    {
        /* Copy over region data */
        regDataIn  = (UINT8 *) argRegion;
        regDataOut = (UINT8 *) regPtr;

        for( i = 0; i < argRegion->rgnSize; i++)
        {
            *regDataOut = *regDataIn;
            regDataOut++;
            regDataIn++;
        }

        regPtr->rgnAlloc = regPtr->rgnSize;

        /* Fix up header pointer */
        regPtr->rgnList = (rect *) ((SIGNED) regPtr
                            + (SIGNED) argRegion->rgnList
                            - (SIGNED) argRegion);

        regPtr->rgnListEnd = (rect *) ((SIGNED) regPtr
                             + (SIGNED) argRegion->rgnListEnd
                             - (SIGNED) argRegion);
    }
    
    /* Return to user mode */
    NU_USER_MODE();

    return(regPtr);
}

/***************************************************************************
* FUNCTION
*
*    EqualRegion
*
* DESCRIPTION
*
*    Function EqualRegion checks to see if two regions structures describe 
*    identical regions.
*
* INPUTS
*
*    region *rg1 - Pointer to the first region to compare.
*
*    region *rg2 - Pointer to the second region to compare.
*
* OUTPUTS
*
*    INT32       - Returns 1 if equal.
*                  Returns 1 if  not equal.
*
***************************************************************************/
INT32 EqualRegion( region *rg1, region *rg2)
{
    INT16 Done = NU_FALSE;
    INT32 value;

    rect *rg1Rect;
    rect *rg2Rect;
    INT32 rCnt;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Check used size */
    if( rg1->rgnSize != rg2->rgnSize )
    {
        value = 0;
        Done  = NU_TRUE;
    }

    if( !Done && 
        ( (rg1->rgnRect.Xmin != rg2->rgnRect.Xmin) ||
          (rg1->rgnRect.Ymin != rg2->rgnRect.Ymin) ||
          (rg1->rgnRect.Xmax != rg2->rgnRect.Xmax) ||
          (rg1->rgnRect.Ymax != rg2->rgnRect.Ymax)
        ) 
      )
    {
        value = 0;
        Done  = NU_TRUE;
    }

    /* Check the header pointer  */
    if( !Done &&
        ( ( (SIGNED)rg1->rgnList - (SIGNED)rg1) != ((SIGNED)rg2->rgnList - (SIGNED)rg2) ||
          ( (SIGNED)rg1->rgnListEnd - (SIGNED)rg1) != ((SIGNED)rg2->rgnListEnd - (SIGNED)rg2)
        )
      )
    {
        value = 0;
        Done  = NU_TRUE;
    }

    if( !Done )
    {
        /* Compare the data in the list  */
        rg1Rect = rg1->rgnList;
        rg2Rect = rg2->rgnList;

        for( rCnt = 0; rCnt <= ((INT32) (rg1->rgnListEnd - rg1->rgnList)); rCnt++)
        {
            if( (rg1Rect->Xmin != rg2Rect->Xmin) ||
                (rg1Rect->Ymin != rg2Rect->Ymin) ||
                (rg1Rect->Xmax != rg2Rect->Xmax) ||
                (rg1Rect->Ymax != rg2Rect->Ymax))
            {
                value = 0;
                Done  = NU_TRUE;
                break; 
            }
            rg1Rect++;
            rg2Rect++;
        }
        if( !Done )
        {
            value = 1;    
        }

    } /* if( !Done ) */

    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    DestroyRegion
*
* DESCRIPTION
*
*    Function Destroys (frees) a region previously created 
*    with any of the region functions.
*
* INPUTS
*
*    region *rgn - Pointer to the region.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID DestroyRegion( region *rgn)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Is this region currently set as the clip region? */
    if( theGrafPort.portRegion == rgn )
    {
        ClipRegion(0);
    }
    GRAFIX_Deallocation(rgn);

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    BitmapToRegion
*
* DESCRIPTION
*
*    Function BitmapToRegion creates a region from the non-transparent data contained
*    within an area of the current bitmap.  SRCRECT specifies the rectangular
*    area to create the region from.  If SRCRECT is NULL, the entire bitmap is
*    processed.  XPARCOLOR specifies the background transparent color that
*    identifies the areas *not* to be included in the region.
*
*    BitmapToRegion() returns with a pointer to the new region, or a NULL
*    pointer if the region creation fails (eg. insufficient memory).
*
* INPUTS
*
*    rect *srcRect   - Pointer to the 
*
*    INT32 transPColor - Specifies the background transparent color.
*
* OUTPUTS
*
*    region          - Returns with a pointer to the new region.
*
***************************************************************************/
region *BitmapToRegion( rect *srcRect, INT32 transPColor)
{
    region *tempRegion;
    region *totalRegion;
    region *rasterRegion;
    INT32  x, y , i , inrun;
    rect  sRect, tRect;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( srcRect == 0 )
    {
        sRect = theGrafPort.portRect;
    }
    else
    {
        sRect = *srcRect;   
    }

    /* Start of with empty region  */
    OpenRegion();
    totalRegion =(region *) CloseRegion();

	if (totalRegion != NU_NULL)
	{
	    for( y = sRect.Ymin; y < sRect.Ymax; y++)
	    {
	        tRect.Ymin = y;
	        tRect.Ymax = y + 1;
	
	        OpenRegion();
	        inrun = 0;
	
	        /* For each x in question */
	        for( x = sRect.Xmin; x < sRect.Xmax ; x++)
	        {
	            i = GetPixel( x, y);
	
	            if( i != transPColor )
	            {
	                if( inrun != 0 )
	                {
	                    tRect.Xmax++;
	                }
	                else
	                {
	                    tRect.Xmin = x;
	                    tRect.Xmax = x + 1;
	                    inrun = 1;
	                }
	            }
	            else
	            {
	                if( inrun != 0 )
	                {
	                    RS_Rectangle_Draw( PAINT, &tRect, -1, 0, 0);
	                    inrun = 0;
	                }
	            }
	        } /* for( x = sRect.Xmin; x < sRect.Xmax ; x++) */
	
	        if( inrun != 0 )
	        {
	            RS_Rectangle_Draw( PAINT, &tRect, -1, 0, 0);
	        }
	
	        /* Close and compress the partial region */
	        rasterRegion = (region *) CloseRegion();
	        if( rasterRegion == 0 )
	        {
	            GRAFIX_Deallocation(totalRegion);
	            totalRegion = 0;
	            break; /* for( ) */
	        }
	
	        /* Accumulate running total region */
	        /* Get the Union Region */
	        tempRegion = (region *) RS_Regions_Merge(REG_UNION, rasterRegion, totalRegion);
	
	        /* Discard partial regions */
	        GRAFIX_Deallocation(rasterRegion);
	        GRAFIX_Deallocation(totalRegion);
	
	        if( tempRegion == 0 )
	        {
	            totalRegion = 0;
	            break; /* for( ) */
	        }
	        
	        totalRegion = tempRegion;
	
	    } /* for( y = sRect.Ymin; y < sRect.Ymax; y++) */
	}

    /* Return to user mode */
    NU_USER_MODE();

    return(totalRegion);
}

#endif  /* NO_REGION_CLIP */
