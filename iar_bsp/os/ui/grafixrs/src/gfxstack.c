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
*  gfxstack.c                                                   
*
* DESCRIPTION
*
*  Push and pop Grafix functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  PushGrafix
*  PopGrafix
*
* DEPENDENCIES
*
*  rs_base.h
*  global.h
*  gfxStack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/global.h"
#include "ui/gfxstack.h"

extern VOID SetPort( rsPort *portPtr);

/***************************************************************************
* FUNCTION
*
*    PushGrafix
*
* DESCRIPTION
*
*    Function PushGrafix pushes the current state of all of the port's graphics
*    variables into the stack area pointed to by argTS. PopGrafix is later called
*    to restore the original graphics state.
*
* INPUTS
*
*    INT32 *argTS - Pointer to graphics state.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID PushGrafix(INT32 *argTS)
{
    rsPort *savePort;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* save are */
    savePort = (rsPort *) argTS; 

    /* save the current port */
    *savePort = *thePort;

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    PopGrafix
*
* DESCRIPTION
*
*    Function PopGrafix restores the state of all graphics variables 
*    previously saved on the stack from a PushGrafix call.
*
* INPUTS
*
*    INT32 *argTS - Pointer to graphics state.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID PopGrafix(INT32 *argTS)
{
    rsPort *savePort;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* save are */
    savePort = (rsPort *) argTS;  

    /* save the current port */
    *thePort = *savePort;

    /* SetPort will resync everything */
    SetPort(thePort);
    
    /* Return to user mode */
    NU_USER_MODE();

}

