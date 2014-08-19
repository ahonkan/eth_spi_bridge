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
*  copyb_copybits.c
*
* DESCRIPTION
*
*  This file contains the CopyBits function.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  CopyBits
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
*    CopyBits
*
* DESCRIPTION
*
*    Function COPYBITS copies the specified source rectangle (srcRECT) in the
*    source bitmap (srcBMap) to the destination rectangle (dstRECT) in the
*    destination bitmap (dstBMap).
*
*    Note: All Rectangles must be specified in global coordinates.
*
*    Returns an error (-1) for unsupported grafMaps combinations.
*
* INPUTS
*
*    grafMap *srcBMAP - Pointer to source bitmap.
*
*    grafMap *dstBMAP - Pointer to destination bitmap.
*
*    rect *srcRECT    - Pointer to source rectangle.
*
*    rect *dstCLIP    - Pointer to clipping rectangle.
*
*    INT32 dstRASOP   - Bit raster operation.
*
* OUTPUTS
*
*    STATUS           - Returns NU_SUCCESS if successful.
*                       Returns -1 for unsupported grafMaps combinations.
*
***************************************************************************/
STATUS CopyBits( grafMap *srcBMAP, grafMap *dstBMAP, rect *srcRECT,
                 rect *dstCLIP, INT32 dstRASOP )
{
    STATUS status = ~(NU_SUCCESS);
    blitRcd cblitRec;   /* blitRcd     */
    rect srcR;          /* source rect */

    UINT8 cmbDevtech;   /* combined devtech of bitmaps   */
    UINT8 cmbBlitType;  /* combined blit type from table */
    INT16 grafErrValue; /* error value */

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* to remove paradigm warning */
    (VOID)status;


    cblitRec.blitRop   = dstRASOP;
    cblitRec.blitFlags = bfClipRect;
    cblitRec.blitClip  = dstCLIP;
    cblitRec.blitCnt   = 1;
    cblitRec.blitBack  = 0;
    cblitRec.blitFore  = 0xFFFFFFFF;
    cblitRec.blitMask  = 0xFFFFFFFF;

    srcR = (*srcRECT);

    cblitRec.blitList = (SIGNED) &srcR;
    cblitRec.blitSmap = srcBMAP;
    cblitRec.blitDmap = dstBMAP;

    /* BLITREC is filled out at this point */

    /* combine both devtechs into an index [dest][source] */
    cmbDevtech = srcBMAP->devTech | (dstBMAP->devTech << 4);

    /* look up blit combo type from table */
    cmbBlitType = blitxfertbl[cmbDevtech];

    switch(cmbBlitType)
    {
    case self2self:
        cblitRec.blitDmap->prBlitSS(&cblitRec);
        status = NU_SUCCESS;
        break;

    case mem2self:
        cblitRec.blitDmap->prBlitMS(&cblitRec);
        status = NU_SUCCESS;
        break;

    case self2mem:
        /* must use source grafMap */
        cblitRec.blitSmap->prBlitSM(&cblitRec);
        status = NU_SUCCESS;
        break;

    case mono2self:
        cblitRec.blitDmap->prBlit1S(&cblitRec);
        status = NU_SUCCESS;
        break;

    default:
        /* not a supported combo */
        grafErrValue = c_CopyBits +  c_InvDevFunc;

        /* report error */
        nuGrafErr(grafErrValue,__LINE__, __FILE__);
        status = ~(NU_SUCCESS);
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(status);
}

