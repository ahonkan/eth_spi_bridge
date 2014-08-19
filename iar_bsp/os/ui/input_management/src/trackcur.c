/***************************************************************************
*
*               Copyright 2004 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  trackcur.c                                                          
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*  This file contains the cursor handling functions.      
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*  None                                                             
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*  TC_TrackCursor  
*  TC_QueryCursor 
*  TC_InputTracker                                                          
*                                                                       
* DEPENDENCIES                                                          
*       
*  input_management/input_config.h                                                                
*  grafixrs/inc/rs_base.h
*  input_management/inc/trackcur.h
*
*************************************************************************/

#include "ui/input_config.h"
#include "ui/rs_base.h"
#include "ui/trackcur.h"

#ifdef      USE_CURSOR

/* current tracking input device */
struct _mouseRcd *curTrack; 

/* cursor tracking code */
VOID (*TrackIDV)();     

/***************************************************************************
* FUNCTION
*
*    TC_TrackCursor
*
* DESCRIPTION
*
*    Enables and disables cursor tracking of the current input device. When
*    cursor tracking is enabled (TrackCursor(TRUE)), the cursor position is
*    automatically updated to follow input movement.  If cursor tracking is
*    disabled (TrackCursor(FALSE)), Input device movement has no effect on
*    the cursor position.
*
* INPUTS
*
*    INT32 argTF
*
* OUTPUTS
*
*    None. 
*
****************************************************************************/
VOID TC_TrackCursor(INT32 argTF)
{
    INT32  tmpIMPOSN;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* if false, turn off tracking */
    if( argTF == 0 )
    {
        /* Flag tracking disabled */
        gFlags &= ~gfTrkEnab;
    }
    else
    {
        /* disable tracking while we muck */
        gFlags &= ~gfTrkEnab;

        /* install indirect vector for input device call backs to get a tracking routine */
        TrackIDV = (VOID (*)()) TC_InputTracker;

        /* install indirect vector for resuming grafMap callbacks to get a move cursor routine */
        MovCursIDV = (VOID (*)()) nuMoveCursor;

        /* set current input device as the tracking device */
        curTrack = curInput;

        /* ignore this call if input device was never opened? */
        if( (curInput->mrFlags & mrOpenedSig) )
        {
            /* Sync the input device to the cursor position */
            curInput->mrEvent.eventX = CursorX;
            curInput->mrEvent.eventY = CursorY;
            tmpIMPOSN = IMPOSN;
            tmpIMPOSN = curInput->mrInputMgr(curInput, tmpIMPOSN);
            NotUsed = (VOID *) tmpIMPOSN;

            /* unlock the cursor grafMap. Flag tracking enabled */
            cursBlit.blitDmap->mapLock = 0;
            gFlags |= gfTrkEnab;
        } /* if( (curInput->mrFlags & mrOpenedSig) ) */
    } /* else */

    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    TC_QueryCursor
*
* DESCRIPTION
*
*    Returns the current CURSORX, CURSORY, CURSORLEVEL, and returns the
*    current tracking devices BUTTON values.
*
* INPUTS
*
*    INT32 *argCURXO
*    INT32 *argCURYO
*    INT32 *argCURLO
*    INT32 *argBUTNO
*
* OUTPUTS
*
*    None. 
*
****************************************************************************/
VOID TC_QueryCursor(INT32 *argCURXO, INT32 *argCURYO, INT32 *argCURLO, INT32 *argBUTNO)
{
    INT32 tempX;
    INT32 tempY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    tempX     = CursorX;
    tempY     = CursorY;
    *argCURLO = CursorLevel;

    if( globalLevel > 0 )
    {
        /* convert from global to user */
        G2UP( CursorX, CursorY, &tempX, &tempY);
    }

    *argCURXO = tempX;
    *argCURYO = tempY;
    *argBUTNO = 0;

    /* tracking enabled? */
    if( gFlags & gfTrkEnab )
    {
        *argBUTNO = curTrack->mrEvent.eventButtons;
    }

    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    TC_InputTracker
*
* DESCRIPTION
*
*    The routine used by input device ISRs to handle tracking.  It is 
*    called via the input device call back routine if tracking is enabled
*    and appropriate.
*
* INPUTS
*
*    mouseRcd *trkRecord
*
* OUTPUTS
*
*    None. 
*
****************************************************************************/
VOID TC_InputTracker(mouseRcd *trkRecord)
{
    grafMap *trkBmap;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

#ifdef FIXUP386
    /* use this variable, it's never fixedup by the driver code */
    trkBmap = cursBmap:
#else
    trkBmap = cursBlit.blitDmap;
#endif

    /* check semaphore lock on grafMap */
    if( trkBmap->mapLock < 0 )
    {
        /* the grafMap was being used, can't access right now, so
           set a pending flag and exit */
        trkBmap->mapFlags |= mfPending;
    }
    else
    {
        /* lock grafMap */
        trkBmap->mapLock--; 
        do
        {
            /* clear deferred flag */
            trkBmap->mapFlags = trkBmap->mapFlags & ~mfPending;

            nuMoveCursor(trkRecord->mrEvent.eventX, trkRecord->mrEvent.eventY);

            /* check if deferred flag went on while we were updating */
        } while( trkBmap->mapFlags & mfPending );

        /* unlock grafMap */
        trkBmap->mapLock++; 
    }

    NU_USER_MODE();
}

#endif      /* USE_CURSOR */

