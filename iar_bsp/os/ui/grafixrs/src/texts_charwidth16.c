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
*  texts_charwidth16.c                                                      
*
* DESCRIPTION
*
*  Contains text support function - CharWidth16.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  CharWidth16
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
*    CharWidth16
*
* DESCRIPTION
*
*    Function CharWidth16 returns the width of the specified 16-bit character.
*
* INPUTS
*
*    word CHAR16 - The specified 16-bit character.
*
* OUTPUTS
*
*    INT32 - Returns the width of the specified character.
*
***************************************************************************/
INT32 CharWidth16( UINT16 CHAR16)
{
    INT32 savTxTerm;
    INT32 savLocX, savLocY;
    INT32 savPenX, savPenY;
    INT32 width = 0;
    UINT16 tempChar[3];

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* Save all points value and Hide Pen */
    TXW_PSH;

    savTxTerm = theGrafPort.txTerm;

    theGrafPort.txTerm = ~CHAR16;

    tempChar[0] = CHAR16;
    tempChar[1] = 0;
    tempChar[2] = 0;

#ifdef USE_UNICODE
    width = TXTD_DrawStringW(tempChar, 0, 1, 1);
#else
    width = txtDrwIDV(tempChar, 0 , 1, 1);
#endif

    theGrafPort.txTerm = savTxTerm;

    /* Restore all points value and Show Pen */
    TXW_POP;

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    return(width);
}
