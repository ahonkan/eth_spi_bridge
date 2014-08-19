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
*  texts_charwidth.c                                                      
*
* DESCRIPTION
*
*  Contains text support function - CharWidth.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  CharWidth
*
* DEPENDENCIES
*
*  rs_base.h
*  rsfonts.h
*  markers.h
*  texts.h
*  textd.h
*  gfxstack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/rsfonts.h"
#include "ui/texts.h"
#include "ui/textd.h"
#include "ui/gfxstack.h"

/***************************************************************************
* FUNCTION
*
*    CharWidth
*
* DESCRIPTION
*
*    Function CharWidth returns the width of the specified 8-bit character.
*
* INPUTS
*
*    signed char CHAR8 - Request the the width of the specified 8-bit character.
*
* OUTPUTS
*
*    INT32 - Returns with the character width.
*
***************************************************************************/
INT32 CharWidth(signed char CHAR8)
{
    INT32 savTxTerm;
    INT32 savLocX, savLocY;
    INT32 savPenX, savPenY;
    INT32 width = 0;
    signed char str[3];

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* Save all points value and Hide Pen */
    TXW_PSH;

    savTxTerm = theGrafPort.txTerm;

    theGrafPort.txTerm = ~CHAR8;

    str[0]=CHAR8;
    str[1]=0;
    str[2]=0;
    width = txtDrwIDV(str, 0 , 1, 0);

    theGrafPort.txTerm = savTxTerm;

    /* Restore all points value and Show Pen */
    TXW_POP;

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    return(width);
}
