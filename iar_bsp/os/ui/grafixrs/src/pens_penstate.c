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
*  pens_penstate.c
*
* DESCRIPTION
*
*  This file contains Pen related functions, GetPenState and SetPenState.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  GetPenState
*  SetPenState
*
* DEPENDENCIES
*
*  rs_base.h
*  pens.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/pens.h"

/***************************************************************************
* FUNCTION
*
*    GetPenState
*
* DESCRIPTION
*
*    Function GetPenState saves the current pen status variables.
*    It actually saves the entire port for simplicity.
*
* INPUTS
*
*    rsPort *penRcd - Pointer to port.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID GetPenState(rsPort *penRcd)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    *penRcd = *thePort;

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    SetPenState
*
* DESCRIPTION
*
*    Function SetPenState restores the pen status variables.
*
* INPUTS
*
*    rsPort *penRcd - Pointer to pen record.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID SetPenState(rsPort *penRcd)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    thePort->bkPat     = penRcd->bkPat;
    thePort->bkColor   = penRcd->bkColor;
    thePort->pnColor   = penRcd->pnColor;
    thePort->pnLoc     = penRcd->pnLoc;
    thePort->pnSize    = penRcd->pnSize;
    thePort->pnLevel   = penRcd->pnLevel;
    thePort->pnMode    = penRcd->pnMode;
    thePort->pnPat     = penRcd->pnPat;
    thePort->pnCap     = penRcd->pnCap;
    thePort->pnJoin    = penRcd->pnJoin;
    thePort->pnMiter   = penRcd->pnMiter;
    thePort->pnFlags   = penRcd->pnFlags;

#ifdef  DASHED_LINE_SUPPORT
    
    thePort->pnDash    = penRcd->pnDash;
    thePort->pnDashNdx = penRcd->pnDashNdx;
    thePort->pnDashCnt = penRcd->pnDashCnt;
    thePort->pnDashRcd = penRcd->pnDashRcd;

#endif  /* DASHED_LINE_SUPPORT */
    
    thePort->txMode    = penRcd->txMode;
    
    /* SetPort will resync everything */
    SetPort(thePort);   

    /* Return to user mode */
    NU_USER_MODE();

}
