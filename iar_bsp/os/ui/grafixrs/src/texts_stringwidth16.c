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
*  texts_stringwidth16.c                                                      
*
* DESCRIPTION
*
*  Contains text support function - StringWidth16.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  StringWidth16
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
*    StringWidth16
*
* DESCRIPTION
*
*    Function StringWidth16 returns the width of the specified 16-bit
*    text string, STRING16.
*
* INPUTS
*
*    word *STRING16 - Pointer to the the specified text string. 
*
* OUTPUTS
*
*    INT32 - Returns the width of the specified 16-bit text string.
*
***************************************************************************/
INT32 StringWidth16( UNICHAR *STRING16)
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
	width = TXTD_DrawStringW(STRING16, 0, 127, 1);
#else
    width = txtDrwIDV(STRING16,0, 127, 1 );
#endif

    /* Restore all points value and Show Pen */
    TXW_POP;

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

    return(width);
}
