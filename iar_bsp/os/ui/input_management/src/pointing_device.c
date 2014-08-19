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
*  pointing_device.c
*
* DESCRIPTION
*
*  The generic functions for the Mouse and touch panel drivers.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PD_MouseEvent 
*  PD_InitInputDevice
*  PD_QueryMouse
*  PD_StopMouse
*  PD_ReadMouse
*  PD_LimitMouse
*  PD_ScaleMouse
*  PD_SetMouse
*  PD_MaskMouse
*
* DEPENDENCIES
*
*  input_management/input_config.h
*  grafixrs/inc/rs_base.h
*  input_management/input_target.h
*  input_management/inc/pointing_device.h
*
***************************************************************************/

#include "ui/input_config.h"

/* For use with any target */
#include "ui/rs_base.h"
#include "ui/input_target.h"
#include "ui/pointing_device.h"


extern MouseDevc inputDeviceTable[];

VOID   *NotUsed;
INT16  NotUsedInt;
INT32  whichButton;

/* pointer to mouse record for our internal drivers */
SIGNED driverMouse;         
SIGNED msSerialMouse;
SIGNED moSerialMouse;
SIGNED joyMouse;

/***************************************************************************
* FUNCTION
*
*    PD_MouseEvent
*
* DESCRIPTION
*
*    This function stores the mouse event that occurred
*
* INPUTS
*
*    rsEvent * pRSEvent
*
* OUTPUTS
*
*    Returns if the event was stored or not.
*
****************************************************************************/

INT32 PD_MouseEvent(rsEvent * pRSEvent)
{
    return (EVENTH_StoreEvent(pRSEvent));
}


/***************************************************************************
* FUNCTION
*
*    PD_InitInputDevice
*
* DESCRIPTION
*
*    Looks up the passed input device code in our internal table, inits
*    the current mouse record accordingly, then opens it.
*
*    If the device code is USER, then it is not initialized (assumed to be
*    by the user) and is opened.
*
* INPUTS
*
*    INT32 argDEV
*
* OUTPUTS
*
*    Returns 0 if ok, 1 if already initialized, -1 if problems.
*
****************************************************************************/
INT32 PD_InitInputDevice(INT32 argDEV)
{
    INT32 value = 0;
    INT16 grafErrValue;
    INT32 mrMgrFnct;
    INT32 i;
    INT16 Done = NU_FALSE;
    INT16 JumpsrchFound = NU_FALSE;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* set call back vector to posting/tracking function */
    curInput->mrCallBack = (VOID (*)()) EVENTH_InputCallBack;

    /* if user device, skip init */
    if( argDEV != cUSER )
    {
        /* init device */
        curInput->mrEvent.eventSource = (UINT8)argDEV;  /* set device ID */

        /* set default coordinate limits to match cursor bitmap */
        curInput->mrLimits.Xmin = 0;
        curInput->mrLimits.Ymin = 0;

#ifdef      USE_CURSOR
        
        curInput->mrLimits.Xmax = cursBlit.blitDmap->pixWidth;
        curInput->mrLimits.Ymax = cursBlit.blitDmap->pixHeight;

        /* set default position to center of limits */
        curInput->mrEvent.eventX = (cursBlit.blitDmap->pixWidth >> 1);
        curInput->mrEvent.eventY = (cursBlit.blitDmap->pixHeight >> 1);

#else

        curInput->mrLimits.Xmax = grafBlit.blitDmap->pixWidth;
        curInput->mrLimits.Ymax = grafBlit.blitDmap->pixHeight;

        /* set default position to center of limits */
        curInput->mrEvent.eventX = (grafBlit.blitDmap->pixWidth >> 1);
        curInput->mrEvent.eventY = (grafBlit.blitDmap->pixHeight >> 1);

#endif      /* USE_CURSOR */
        
        /* set default scale to 1/1 */
        curInput->mrScale.X = 1;
        curInput->mrScale.Y = 1;

        /* set default event mask to press,release, and position */
        curInput->mrEventMask = mPRESS + mREL + mPOS;

        /* set default event data */
        curInput->mrEvent.eventChar = 0;
        curInput->mrEvent.eventScan = 0;
        curInput->mrEvent.eventState = 0;
        curInput->mrEvent.eventButtons = 0;

        /* try to find an internal manager for this device */
        i = 0;
        while( inputDeviceTable[i].devcName != -1)  /* last entry? */
        {
            /* Device codes match? */
            if( inputDeviceTable[i].devcName == argDEV )
            {
                JumpsrchFound = NU_TRUE;
                curInput->mrFlags = ~mrOpenedSig;
                break; /* while */
            }

            i++;    /* no, keep looking */
        }

        if( !JumpsrchFound )
        {
            /* found nothing */
            grafErrValue = c_InitMous +  c_BadDev;

            /* report error */
            nuGrafErr(grafErrValue, __LINE__, __FILE__);
            value = -1;
            Done = NU_TRUE;
        }

        JumpsrchFound = NU_FALSE;

        if( !Done )
        {
            /* store manager vector */
           curInput->mrInputMgr = inputDeviceTable[i].devTblPtr;
        }
    }
    while( !Done )
    {
        /* is it already opened? */
        if( curInput->mrFlags & mrOpenedSig)
        {
                value = 1;
                Done = NU_TRUE;
                break; /* while( !Done = ) */
        }

        /* open the device */
        mrMgrFnct = IMOPEN;
        mrMgrFnct = curInput->mrInputMgr(curInput, mrMgrFnct);

        /* returns 0 if ok, -1 if can't open */
        if( mrMgrFnct != 0 )
        {

                value = mrMgrFnct;
                Done  = NU_TRUE;
                break; /* while( !Done = ) */
        }

        /* ok, so flag as opened */
        curInput->mrFlags = mrOpenedSig;

        /* Smart link PD_StopMouse vector */
        stpMouseIDV = (INT32 (*)()) PD_StopMouse;

        value = 0;
        Done = NU_TRUE;
        break;
    } /* while( !Done = ) */

    NotUsedInt = (INT16) Done;
    NotUsedInt = (INT16) JumpsrchFound;

    NU_USER_MODE();

    /* return ok */
    return(value);
}

/***************************************************************************
* FUNCTION
*
*    PD_QueryMouse
*
* DESCRIPTION
*
*    Finds the location of the mouse
*
* INPUTS
*
*    INT16 argDEV
*
* OUTPUTS
*
*    Returns
*    0 = if the input device indicated by argIDEV is detected.
*    1 = detected as not there
*   -1 = can't tell for sure or don't know
*    2 = don't know what device you are talking about
*
****************************************************************************/
INT32 PD_QueryMouse(INT16 argDEV)
{
    INT32  mrMgrFnct = 0;
    INT32  i;
    INT16  Done = NU_FALSE;
    INT16  JumpqryFound = NU_FALSE;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* try to find an internal manager for this device */
    i = 0;

    while( inputDeviceTable[i].devcName != 0xffff)  /* last entry? */
    {
        /* Device codes match? */
        if( inputDeviceTable[i].devcName == argDEV )
        {
            JumpqryFound = NU_TRUE;
            break;
        }
        i++;    /* no, keep looking */
    }

    if( !JumpqryFound )
    {
        /* report not found */
        mrMgrFnct = 2;
        Done      = NU_TRUE;
    }

    JumpqryFound = NU_FALSE;

    if( !Done )
    {
        /* store manager vector */
        curInput->mrInputMgr = inputDeviceTable[i].devTblPtr;

        /* open the device */
        mrMgrFnct = IMQUERY;
        mrMgrFnct = curInput->mrInputMgr(curInput, mrMgrFnct);
    }

    NotUsedInt = (INT16) JumpqryFound;

    NU_USER_MODE();
    return(mrMgrFnct);
}

/***************************************************************************
* FUNCTION
*
*    PD_StopMouse
*
* DESCRIPTION
*
*    Closes the current input device
*
* INPUTS
*
*    INT16 argDEV
*
* OUTPUTS
*
*    Returns 0 if ok, 1 if already initialized, -1 if problems.
*
****************************************************************************/
INT32 PD_StopMouse(VOID)
{
    INT32 mrMgrFnct;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* was it ever opened? */
    if( !(curInput->mrFlags & mrOpenedSig) )
    {
        /* no, don't close it! */
        mrMgrFnct = 1;
    }
    else
    {
        /* clear the flags */
        curInput->mrFlags = 0;

        /* close the device */
        mrMgrFnct = IMCLOSE;
        mrMgrFnct = curInput->mrInputMgr(curInput, (INT16) mrMgrFnct);
    }

    NU_USER_MODE();

    return(mrMgrFnct);
}

/***************************************************************************
* FUNCTION
*
*    PD_ReadMouse
*
* DESCRIPTION
*
*    Returns the current position and button information for the current
*    input device.
*
* INPUTS
*
*    SIGNED *argX
*    SIGNED *argY
*    INT32* argButtons
*
* OUTPUTS
*
*    Returns 0 if ok, 1 if already inited, -1 if problems.
*
****************************************************************************/
VOID PD_ReadMouse(SIGNED *argX, SIGNED *argY, INT32* argButtons)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* place in passed vars */
    *argX = curInput->mrEvent.eventX;
    *argY = curInput->mrEvent.eventY;
    *argButtons = curInput->mrEvent.eventButtons;

    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    PD_LimitMouse
*
* DESCRIPTION
*
*    Sets the current input devices limit rect. This is the range of input
*    values that the input device will return.
*
* INPUTS
*
*    SIGNED argX1
*    SIGNED argY1
*    SIGNED argX2
*    SIGNED argY2
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID PD_LimitMouse(SIGNED argX1, SIGNED argY1, SIGNED argX2, SIGNED argY2)
{

    INT32 mrMgrFnct;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* call the current input devices manager to set the limits */
    curInput->mrLimits.Xmin = argX1;
    curInput->mrLimits.Ymin = argY1;
    curInput->mrLimits.Xmax = argX2;
    curInput->mrLimits.Ymax = argY2;

    /* call the device */
    mrMgrFnct = IMLIMIT;
    mrMgrFnct = curInput->mrInputMgr(curInput, mrMgrFnct);

    NotUsed = (VOID *) mrMgrFnct;

    NU_USER_MODE();
    return;
}

/***************************************************************************
* FUNCTION
*
*    PD_ScaleMouse
*
* DESCRIPTION
*
*    Sets the current input devices scale factors. This sets the amount
*    of movement per coordinate that the device returns. Absolute
*    positioning devices will not honor this call.
*
* INPUTS
*
*    SIGNED argRX
*    SIGNED argRY
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID PD_ScaleMouse(SIGNED argRX, SIGNED argRY)
{
    INT32 mrMgrFnct;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* call the current input devices manager to set scaling */
    curInput->mrScale.X = argRX;
    curInput->mrScale.Y = argRY;

    /* call the device */
    mrMgrFnct = IMSCALE;
    mrMgrFnct = curInput->mrInputMgr(curInput, mrMgrFnct);

    NotUsed = (VOID *) mrMgrFnct;

    NU_USER_MODE();
}


/***************************************************************************
* FUNCTION
*
*    PD_SetMouse
*
* DESCRIPTION
*
*    Sets the passed mouse record as the current input device.
*
* INPUTS
*
*    mouseRcd *argMouseR
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID PD_SetMouse(mouseRcd *argMouseR)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* test for null (= set internal default) */
    if( argMouseR == 0 )
    {
        argMouseR = &defMouse;
    }

    /* set current input */
    curInput = argMouseR;

    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    PD_MaskMouse
*
* DESCRIPTION
*
*    Sets the mask for the current input device.
*
* INPUTS
*
*    INT32 argMask
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
VOID PD_MaskMouse(INT32 argMask)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* set mask */
    curInput->mrEventMask =  (INT16)argMask;

    NU_USER_MODE();
}

