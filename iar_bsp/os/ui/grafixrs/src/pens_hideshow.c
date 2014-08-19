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
*  pens_hideshow.c
*
* DESCRIPTION
*
*  This file contains Pen related functions, HidePen and ShowPen.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  HidePen
*  ShowPen
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
*    HidePen
*
* DESCRIPTION
*
*    Function HidePen hides the pen.
*
* INPUTS
*
*    None
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID HidePen(VOID)
{

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Decrement pen level */
    theGrafPort.pnLevel--; 
    thePort->pnLevel--;

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    ShowPen
*
* DESCRIPTION
*
*    Function ShowPen displays the pen.
*
* INPUTS
*
*    None
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID ShowPen(VOID)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Increment pen level */
    theGrafPort.pnLevel++; 
    thePort->pnLevel++;

    /* Return to user mode */
    NU_USER_MODE();

}
