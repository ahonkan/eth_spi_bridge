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
*  copyb.c
*
* DESCRIPTION
*
*  This file contains the CopyBlit function.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  CopyBlit
*
* DEPENDENCIES
*
*  blit_specific.h
*  display_inc.h
*  nucleus.h
*  nu_kernel.h
*  nu_drivers.h
*
***************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "drivers/nu_drivers.h"
#include "drivers/blit_specific.h"
#include "drivers/display_inc.h"

/***************************************************************************
* FUNCTION
*
*    CopyBlit
*
* DESCRIPTION
*
*    Function CopyBlit copies a rectangular bitmap from one grafport to
*    another.  The source rect will be 'clipped' to the intersection of the port
*    rect and the bitmap limits. If the source rect is 'clipped', the destination
*    rect is adjusted to correspond to the source origin.  The destination rect
*    upper left determines the destination origin.  The size of the transfer is
*    the smaller of the two rect sizes.
*
*    The devTechs for the source and destination grafMaps are combined,
*    and this combination is looked-up in a table to see if it is a supported
*    blit transfer combination. If it is, the type of transfer ( either
*    self to self, monochrome to self, memory to self, or self to memory)
*    indicated by the table is used to pick the appropriate blit primitive
*    vector. All the 'to self' types of transfers can be vectored through
*    the destination grafmaps vector list, however the 'self to memory' vector
*    is only contained in the source grafmap structure.
*
*    Returns an error (-1) for unsupported grafMaps combinations.
*
* INPUTS
*
*    rsPort *srcPORT - Pointer to source port.
*
*    rsPort *dstPORT - Pointer to destination port.
*
*    rect *argSrcR   - Pointer to source rectangle.
*
*    rect *argDstR   - Pointer to destination rectangle.
*
* OUTPUTS
*
*    STATUS          - Returns NU_SUCCESS if successful.
*                      Returns -1 for unsupported grafMaps combinations.
*
***************************************************************************/
STATUS CopyBlit( rsPort *srcPORT, rsPort *dstPORT, rect *argSrcR, rect *argDstR )
{
    STATUS status = ~(NU_SUCCESS);

    blitRcd blitRec;    /* blitRcd */
    struct _rectlist    /* blit list of two rectangles */
    {
        rect srcBR;     /* source rect in blit list */
        rect dstBR;     /* dest rect in blit list */
    } rectlist;
    rect dstClipR;      /* global clip rect for dest grafMap */

    UINT8 cmbDevtech;   /* combined devtech of bitmaps */
    UINT8 cmbBlitType;  /* combined blit type from table */
    INT32 gblPortMax;
    INT32 srcDelta;
    INT32 dstDelta;
    INT16 grafErrValue; /* error value */

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* to remove paradigm warning */
    (VOID)status;

    /* Convert destination rect to global (possibly non-current port) */
    Port2Gbl(argDstR, dstPORT, &rectlist.dstBR);

    /* ===== Fill out blitRcd based on dest grafPort ======
    NOTE: pattern list and current pattern are not set (not used ) */

    /* assume rect clip */
    blitRec.blitFlags = bfClipRect;

#ifndef NO_REGION_CLIP
    
    /* are regions enabled ? */
    if( dstPORT->portFlags & pfRgnClip )
    {
        /* yes, clip region */
        blitRec.blitFlags |= bfClipRegn;
        blitRec.blitRegn   = dstPORT->portRegion;
    }

#endif  /* NO_REGION_CLIP */

    /* setup rect clip */
    blitRec.blitRop  = dstPORT->pnMode;    /* set raster op        */
    blitRec.blitMask = dstPORT->portMask;  /* set plane mask       */
    blitRec.blitBack = dstPORT->bkColor;   /* bk color             */
    blitRec.blitFore = dstPORT->pnColor;   /* pn color             */
    blitRec.blitCnt  = 1;                  /* 1 set of rects       */
    blitRec.blitList = (SIGNED) &rectlist; /* addr of both rects   */
    blitRec.blitDmap = dstPORT->portMap;   /* set dest grafmap     */
    blitRec.blitClip = &dstClipR;          /* set pointer to clipR */

    /* convert to global and check port clip */
    COORDS_rsGblClip(dstPORT, &dstClipR);

    /* ====== Setup source info =======
    NOTE: may adjust dest rect! */

    Port2Gbl(argSrcR, srcPORT, &rectlist.srcBR);

    /* 'clip' source rect to source port */

    /* xmin < global port xmin ? */
    if( rectlist.srcBR.Xmin < srcPORT->portOrgn.X )
    {
        /* adjust dest min */
        rectlist.dstBR.Xmin += srcPORT->portOrgn.X - rectlist.srcBR.Xmin;

        /* set src to minimum port coord */
        rectlist.srcBR.Xmin  = srcPORT->portOrgn.X;
    }

    /* ymin < global port ymin ? */
    if( rectlist.srcBR.Ymin < srcPORT->portOrgn.Y )
    {
        /* adjust dest min */
        rectlist.dstBR.Ymin += srcPORT->portOrgn.Y - rectlist.srcBR.Ymin;

        /* set src to minimum port coord */
        rectlist.srcBR.Ymin  = srcPORT->portOrgn.Y;
    }

    /* compute global port max X */
    gblPortMax = srcPORT->portRect.Xmax - srcPORT->portRect.Xmin + srcPORT->portOrgn.X;

    /* xmax > global port xmax ? */
    if( rectlist.srcBR.Xmax > gblPortMax )
    {
        /* set it equal */
        rectlist.srcBR.Xmax = gblPortMax;
    }

    /* compute global port max X */
    gblPortMax = srcPORT->portRect.Ymax - srcPORT->portRect.Ymin + srcPORT->portOrgn.Y;

    /* xmax > global port xmax ? */
    if( rectlist.srcBR.Ymax > gblPortMax )
    {
        /* set it equal */
        rectlist.srcBR.Ymax = gblPortMax;
    }

    /* place source grafMap in blitRcd */
    blitRec.blitSmap = srcPORT->portMap;

    /* 'clip' source rect to bitmap limits */
    if( rectlist.srcBR.Xmin < 0 )
    {
        /* adjust dest min */
        rectlist.dstBR.Xmin += rectlist.srcBR.Xmin;

        /* set src min to 0 */
        rectlist.srcBR.Xmin = 0;
    }

    if( rectlist.srcBR.Ymin < 0 )
    {
        /* adjust dest min */
        rectlist.dstBR.Ymin += rectlist.srcBR.Ymin;

        /* set src min to 0 */
        rectlist.srcBR.Ymin = 0;
    }

    if( rectlist.srcBR.Xmax > srcPORT->portMap->pixWidth )
    {
        /* set src max to pixWidth */
        rectlist.srcBR.Xmax = srcPORT->portMap->pixWidth;
    }

    if( rectlist.srcBR.Ymax > srcPORT->portMap->pixHeight )
    {
        /* set src max to pixHeight */
        rectlist.srcBR.Ymax = srcPORT->portMap->pixHeight;
    }

    /* force sizes to be smaller of either src or dst rect */
    /* compute src width */
    srcDelta = rectlist.srcBR.Xmax - rectlist.srcBR.Xmin;

    /* compute dest width */
    dstDelta = rectlist.dstBR.Xmax - rectlist.dstBR.Xmin;

    /* src < dst ? */
    if( srcDelta < dstDelta )
    {
        /* set both to same size */
        rectlist.dstBR.Xmax = rectlist.dstBR.Xmin + srcDelta;
    }
    else
    {
        /* use min delta */
        rectlist.srcBR.Xmax = rectlist.srcBR.Xmin + dstDelta;
    }

    /* compute src height */
    srcDelta = rectlist.srcBR.Ymax - rectlist.srcBR.Ymin;

    /* compute dest height */
    dstDelta = rectlist.dstBR.Ymax - rectlist.dstBR.Ymin;

     /* src < dst ? */
    if( srcDelta < dstDelta )
    {
        /* set both to same size */
        rectlist.dstBR.Ymax = rectlist.dstBR.Ymin + srcDelta;
    }
    else
    {
        /* use min delta */
        rectlist.srcBR.Ymax = rectlist.srcBR.Ymin + dstDelta;
    }

    /* ===== call appropriate blit primitive ===== */

    /* combine both devtechs into an index [dest][source] */
    cmbDevtech = srcPORT->portMap->devTech | (dstPORT->portMap->devTech << 4);

    /* look up blit combo type from table */
    cmbBlitType = blitxfertbl[cmbDevtech];

    switch(cmbBlitType)
    {
    case self2self:
        blitRec.blitDmap->prBlitSS(&blitRec);
        status = NU_SUCCESS;
        break;

    case mem2self:
        blitRec.blitDmap->prBlitMS(&blitRec);
        status = NU_SUCCESS;
        break;

    case self2mem:
         /* must use source grafMap */
        blitRec.blitSmap->prBlitSM(&blitRec);
        status = NU_SUCCESS;
        break;

    case mono2self:
        blitRec.blitDmap->prBlit1S(&blitRec);
        status = NU_SUCCESS;
        break;

    default:
        /* not a supported combo */
        grafErrValue = c_CopyBlit +  c_InvDevFunc;
        nuGrafErr(grafErrValue,__LINE__, __FILE__);
        status = ~(NU_SUCCESS);
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}

