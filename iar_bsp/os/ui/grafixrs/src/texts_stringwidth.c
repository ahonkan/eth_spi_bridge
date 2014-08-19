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
*  texts_stringwidth.c                                                      
*
* DESCRIPTION
*
*  Contains text support function - StringWidth.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  StringWidth
*
* DEPENDENCIES
*
*  rs_base.h
*  rsfonts.h
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
*    StringWidth
*
* DESCRIPTION
*
*    Function StringWidth returns the width of the specified 8-bit text
*    string, STRING8.  The string termination value is defined by
*    grafPort.txTerm, which is 0 (NULL) by default.
*
* INPUTS
*
*     signed char *STRING8 - Pointer to the text string.
*
* OUTPUTS
*
*    INT32 - Returns the width of the string.
*
***************************************************************************/
INT32 StringWidth( UNICHAR *STRING8)
{
    INT32 savLocX, savLocY;
    INT32 savPenX, savPenY;
    INT32 width = 0;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* Save all points value and Hide Pen */
    TXW_PSH;

    width = txtDrwIDV(STRING8,0, 127, 0 );

    /* Restore all points value and Show Pen */
    TXW_POP;

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    return(width);
}
