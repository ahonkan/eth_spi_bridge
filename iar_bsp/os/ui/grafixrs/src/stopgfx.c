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
*  stopgfx.c                                                    
*
* DESCRIPTION
*
*  This file contains the StopGraphics function.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  StopGraphics
*
* DEPENDENCIES
*
*  rs_base.h
*  stopgfx.h
*  global.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/stopgfx.h"
#include "ui/global.h"

/* screen semaphore */
extern NU_SEMAPHORE    ScreenSema;

/***************************************************************************
* FUNCTION
*
*    StopGraphics
*
* DESCRIPTION
*
*    Function StopGraphics is the standard way to shut down. It depends on
*    CloseBitmap() to free the default bitmap (driver, rowtables) and frees
*    the pool and the font buffer directly.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT32 - Returns StopErr.
*
***************************************************************************/
INT32 StopGraphics(VOID)
{
    INT32 StopErr;    

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Free the pool buffer. If null, there isn't one. */
    if( mpWorkSpace != NU_NULL ) 
    {
        GRAFIX_Deallocation(mpWorkSpace);
    }

    /* Free the default bitmap. This function is expected to free the
       driver space and the rowtables. */
    StopErr = CloseBitmap(&defGrafMap);

#ifdef  INCLUDE_DEFAULT_FONT
    
    /* Free the font buffer, if it was allocated */
    if( defFont != imbFnt ) 
    {
        GRAFIX_Deallocation(defFont);
    }

#endif  /* INCLUDE_DEFAULT_FONT */
    
    /* Delete display synchronization semaphore.  */
    StopErr = NU_Delete_Semaphore(&ScreenSema);
    
    /* see if event system still connected */
    if( gFlags & gfEvtInit )
    {
        stpEventIDV();
    }

    /* see if mouse system still connected */
    if( curInput->mrFlags & mrOpenedSig )
    {
        stpMouseIDV();
    }

    /* Last, but not least, clear system flag */
    gFlags &= ~gfGrafInit;

    /* Return to user mode */
    NU_USER_MODE();

    return(StopErr);
}

