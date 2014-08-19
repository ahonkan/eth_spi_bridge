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
*  regiond.c                                                    
*
* DESCRIPTION
*
*  Contains the API function RS_Region_Draw for drawing regions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  RS_Region_Draw
*
* DEPENDENCIES
*
*  rs_base.h
*  global.h
*  regiond.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/global.h"
#include "ui/regiond.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    RS_Region_Draw
*
* DESCRIPTION
*
*    The API function RS_Region_Draw for drawing regions.
*
* INPUTS
*
*    ObjectAction action - This would be the action that would be performed on the object,
*                          this will be a list of actions in an enumerated data type.
*
*                              EX: FRAME  = 0
*                                  PAINT  = 1
*                                  FILL   = 2
*                                  ERASE  = 3
*                                  INVERT = 4
*                                  POLY   = 5  (only used by BEZIER) 
*
*    region *argRegion   - Pointer to region.
*
*    INT32 patt          - This is the Pattern, fill pattern structure that contains 32 default
*                          values for patterns. So the value is 0 to 31. -1 if not used.
*                          This can be user Defined
*
* OUTPUTS
*
*    STATUS             -  Returns NU_SUCCESS if successful.
*
*
***************************************************************************/
STATUS RS_Region_Draw( ObjectAction action, region *argRegion, INT32 regPattern)
{
    STATUS status = ~NU_SUCCESS;
    SIGNED tem;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* to remove paradigm warning */
    (VOID)(INT16)status;

    if( theGrafPort.pnLevel >= 0 )
    {
        switch(action)
        {
            case ERASE:
                /* Change raster op and pattern */
                grafBlit.blitRop = 0;
                grafBlit.blitPat = theGrafPort.bkPat;
                break;
                
            case INVERT:
                /* Change raster op */
                grafBlit.blitRop = 14; /* zINVERTz */
                break;

            case FILL:
                if( regPattern > 32 )
                {
                    regPattern = 1;
                }

                /* Change raster op and pattern */
                grafBlit.blitRop = 0;
                grafBlit.blitPat = regPattern;
                break;

            case PAINT:     
                break;
			case TRANS:
				if( regPattern > 32 )
				{
					regPattern = 1;
				}
				grafBlit.blitRop = xAVGx;
				break;

			/* Action not applicable. Not implemented. */	
            case FRAME:     
            case POLY:     
                break;
        }

        /* Get the pointer to the rect list */
        grafBlit.blitList = (SIGNED) argRegion->rgnList;

        /* Compute the number of rects in the list */
        tem = ((SIGNED)(argRegion->rgnListEnd) -(SIGNED) (grafBlit.blitList));
        grafBlit.blitCnt = (INT16)((tem >> 3) + 1);

        /* Call Filler */
        grafBlit.blitDmap->prFill(&grafBlit);

        /* Restore default blitlist variables */
        grafBlit.blitCnt = 1;
        grafBlit.blitList = (SIGNED) &grafBlist;      

        grafBlit.blitRop = theGrafPort.pnMode;
        grafBlit.blitPat = theGrafPort.pnPat;
    }

    status = NU_SUCCESS;

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}
