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
*  texts_textwidth16.c                                                      
*
* DESCRIPTION
*
*  Contains text support function - TextWidth16.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  TextWidth16
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
*    TextWidth16
*
* DESCRIPTION
*
*    Function TextWidth16 returns the width of the the specified 16-bit
*    text string, TEXTSTR, beginning at "INDEX" characters from the start
*    of the string, and continuing for "COUNT" characters or until a
*    string termination character encountered (which ever occurs first).
*
*    The string termination value is defined by grafPort.txTerm which is 0
*    (NULL) by default.
*
* INPUTS
*
*    word *STRING16 - Pointer to the string.
*
*    INT32 INDEX    - Beginning index number.
*
*    INT32 COUNT    - Numbers of characters to include.
*
* OUTPUTS
*
*    INT32 - Returns the width of the specified 16-bit text string.
*
***************************************************************************/
INT32 TextWidth16( UNICHAR *STRING16, INT32 INDEX, INT32 COUNT)
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

#ifdef USE_UNICODE    
    width = TXTD_DrawStringW(STRING16, INDEX, COUNT, 1);
#else
    width = txtDrwIDV(STRING16, INDEX, COUNT, 1);
#endif

    /* Restore all points value and Show Pen */
    TXW_POP;

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    return(width);
}
