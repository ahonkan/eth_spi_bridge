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
*  cursor.c                                                     
*
* DESCRIPTION
*
*  This file contains cursor common functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  HideCursor
*  ShowCursor
*  MoveCursor
*  CursorStyle
*  CursorBitmap
*  nuMoveCursor
*  nuResume
*  CursorColor
*  CURSOR_rsInitShftCurs
*  ProtectRect
*  ProtectOff
*  DefineCursor
*
* DEPENDENCIES
*
*  rs_base.h
*  cursor.h
*  global.h
*  gfxstack.h
*  memrymgr.h
*  str_utils.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/cursor.h"
#include "ui/global.h"
#include "ui/gfxstack.h"
#include "ui/memrymgr.h"
#include "ui/str_utils.h"

#ifdef      USE_CURSOR

static DEFN CursorXoff;
static DEFN CursorYoff;

/* cursor blit list */
static struct _rect cursBlist;     

/* cursor clip rect */
static struct _rect cursClip;      

#ifdef  hyperCursor     

/* special globals for hypercursor */
/* (for further information of the following structures see CURSOR.DOC) */
/* true if want to use hypercursor */
static  DEFN cursDoHyper;         

/* cursBMap->mapTable[0] */
static  SIGNED **cursBMapRowTbl;  

/* cursBMap->pixHeight - 1 */
static DEFN cursBMapYmax;        

/* cursBMap->pixBytes - 1 */
static DEFN cursBMapBytesM1;     

#endif  /* hyperCursor */

extern NU_MEMORY_POOL System_Memory;
/***************************************************************************
* FUNCTION
*
*    HideCursor
*
* DESCRIPTION
*
*    Function HideCursor first decrements the cursor-level by 1.
*    If the cursor becomes hidden (cursor-level just decremented to -1),
*    HideCursor removes the cursor from the screen, restoring the original
*    backing image. Otherwise it just returns.
*
*    Each call to HideCursor should be balanced by a corresponding call to
*    ShowCursor.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID HideCursor(VOID)
{
    /* Cursor write image record */
    blitRcd cursWBlit;    
    image *CursorSavePtr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* pause the cursor from responding to interrupts
       this locks a semaphore to avoid being re-entrancy */
    M_PAUSE(cursBlit.blitDmap);

    CursorLevel--;

    /* if cursor was not just visible, exit */
    if( CursorLevel == -1 )  
    {
        /* hide the cursor */
        /* can we hyper this kind of device ? */
        #ifdef  hyperCursor     
        if (cursDoHyper == 0)
        #endif
        {
            /* Save global variables */
            BLITS_SaveGlobals();

            /* restore backing image with writeimage */
            /* calculate bounding rectangle */
            cursBlist.Xmin = (INT16) (CursorX - CursorXoff);
            cursBlist.Ymin = (INT16) (CursorY - CursorYoff);

            CursorSavePtr = (image *)MEM_malloc(sizeof(image));

            if (CursorSavePtr != NU_NULL)
			{
	            CursorSavePtr->imWidth = ((image *)CursorSave)->imWidth;
	            CursorSavePtr->imHeight = ((image *)CursorSave)->imHeight;
	            CursorSavePtr->imAlign = ((image *)CursorSave)->imAlign;
	            CursorSavePtr->imFlags = ((image *)CursorSave)->imFlags;
	            CursorSavePtr->pad = ((image *)CursorSave)->pad;
	            CursorSavePtr->imBytes = ((image *)CursorSave)->imBytes;
	            CursorSavePtr->imBits = ((image *)CursorSave)->imBits;
	            CursorSavePtr->imPlanes = ((image *)CursorSave)->imPlanes;
	            CursorSavePtr->imData = (UINT8 *)&(((image *)CursorSave)->imData);
	
	            cursBlist.Xmax = (INT16) (cursBlist.Xmin + CursorSavePtr->imWidth);
	            cursBlist.Ymax = (INT16) (cursBlist.Ymin + CursorSavePtr->imHeight);
	
	            cursWBlit.blitSmap = (grafMap *)CursorSavePtr;
	            /* rasterOp = replace */
	            cursBlit.blitRop = 0;                      
	    
	            /* must disable color translation of the save buffer - 
	               if it is a monochrome image, colors would be changed */
	            cursBlit.blitBack = 0;
	            cursBlit.blitFore = -1;
	
	            /* fill in write image blit record */         
	            cursWBlit.blitRsv   = cursBlit.blitRsv;
	            cursWBlit.blitAlloc = cursBlit.blitAlloc;
	            cursWBlit.blitFlags = cursBlit.blitFlags;
	            cursWBlit.blitRop   = cursBlit.blitRop;
	            cursWBlit.blitCnt   = cursBlit.blitCnt;
	            cursWBlit.blitMask  = cursBlit.blitMask;
	            
#ifdef  FILL_PATTERNS_SUPPORT

	            cursWBlit.blitPatl  = cursBlit.blitPatl;

#endif /* FILL_PATTERNS_SUPPORT */

	            cursWBlit.blitDmap  = cursBlit.blitDmap;

#ifndef NO_REGION_CLIP

	            cursWBlit.blitRegn  = cursBlit.blitRegn;
	            
#endif  /* NO_REGION_CLIP */

                cursWBlit.blitClip  = cursBlit.blitClip;
	            cursWBlit.blitBack  = cursBlit.blitBack;
	            cursWBlit.blitFore  = cursBlit.blitFore;
	            cursWBlit.blitList  = cursBlit.blitList;
	
	            /* do it */
	            (cursBlit.blitDmap->prWrImg)(&cursWBlit);
	
	             GRAFIX_Deallocation(CursorSavePtr);
	            /* Restore global variables */
	            BLITS_RestoreGlobals();
			}
        } /* if( !Jump ) */
    }

    nuResume( cursBlit.blitDmap);

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    ShowCursor
*
* DESCRIPTION
*
*    Function ShowCursor increments the cursor-level by 1.
*    If the cursor becomes visible (cursor-level just incremented to 0),
*    the current cursor is displayed at global CursorX and CursorY.
*    A backing image is saved (CursorSave) before the cursor is drawn.
*
*    Initializing the cursor system is deferred until ShowCursor is called
*    for the first time, in order to avoid linking the init code unless
*    ShowCursor is used. This will occur only on the first call to ShowCursor
*    (gfCurInit flag in gflags is set).
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID ShowCursor(VOID)
{
    UINT8 Jump = NU_FALSE;
    UINT8 *sc_tempImgPtr1;
    image *sc_tempImgPtr;

    /* Cursor write image record */
    blitRcd cursWBlit; 

    /* error value */
    INT16 grafErrValue;  


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Obtain the Screen semaphore */
    GFX_GetScreenSemaphore();

    /* pause the cursor from responding to interrupts
       this locks a semaphore to avoid re-entrancy */
    M_PAUSE( cursBlit.blitDmap);


    /* check to see if this cursor is
        1) entering a protected area
        2) exiting a protected area
        3) remaining in a protected area */

    if( (CursorX < CursProtXmin) || (CursorX >= CursProtXmax) ||
        (CursorY < CursProtYmin) || (CursorY >= CursProtYmax) )
    {
        /* not in a protected area */
        
        /* were we previously protected? */
        if( gFlags & gfCurPHid )
        {
            /* clear notion of being protected */
            gFlags &= ~gfCurPHid;

            /* remove the extra hide level */
            CursorLevel++;
        }
    }
    else
    {
        /* cursor is in a protected area */
        /* were we previously protected? */
        if( !(gFlags & gfCurPHid) )
        {
            /* no, flag as protected */
            gFlags |= gfCurPHid;

            /* exit still hidden */
            Jump = NU_TRUE; 
        }
    }

    if( !Jump )
    {
        CursorLevel++;

        /* > 0, that's an error! */
        if( CursorLevel > 0 )
        {
            grafErrValue = c_ShowCurs +  c_CursLevel;

            /* report error */
            nuGrafErr(grafErrValue, __LINE__, __FILE__); 

            /* Zero it */
            CursorLevel = 0;         
        }
        else
        {
            if( CursorLevel == 0 )
            {
                /* cursor level just went to zero, display the cursor */
                /* Save global variables */
                BLITS_SaveGlobals();

                /* check to see if the cursor system has ever been called (init'd) */
                if( !(gFlags & gfCurInit) )
                {
                    /* initialize the cursor system - set the cursor blit record    */
                    /* rect clipping      */
                    cursBlit.blitFlags = bfClipRect;          

                    /* one rect in list   */
                    cursBlit.blitCnt   = 1;                   

                    /* list = 1 rect long */
                    cursBlit.blitAlloc = sizeof(rect);        

                    /* list = *cursBlist  */
                    cursBlit.blitList  = (SIGNED) (&cursBlist); 
                    cursBlit.blitMask  = -1;

                    /* Clip = *cursClip   */
                    cursBlit.blitClip  = &cursClip;           

                    /* make non-displayable */
                    CursorLevel--;

                    /* this will re-enter ShowCursor */
                    CursorBitmap(cursBlit.blitDmap); 

                    /* set init'd flag */
                    gFlags |= gfCurInit;             

                    /* make displayable again */
                    CursorLevel++;
                }

                cursBlist.Xmin = (INT16) (CursorX - CursorXoff);
                cursBlist.Ymin = (INT16) (CursorY - CursorYoff);
/* can we hyper this kind of device ? */
#ifdef  hyperCursor  
                if( cursDoHyper == 0 )
#endif
                {
                    
                    sc_tempImgPtr = (image *)MEM_malloc(sizeof(image));

                    if (sc_tempImgPtr != NU_NULL)
					{
	                    /* Calculate the cursor rectangle */
	                    cursBlist.Xmax = cursBlist.Xmin + CursorImag->imWidth;
	                    cursBlist.Ymax = cursBlist.Ymin + CursorImag->imHeight;
	
	                    sc_tempImgPtr->pad = ((image *)CursorSave)->pad;
	                    sc_tempImgPtr->imData = (UINT8 *)&(((image *)CursorSave)->imData);
	
	                    /* Read the current backing area into the cursor save buffer */
	                    (cursBlit.blitDmap->prRdImg)(cursBlit.blitDmap, sc_tempImgPtr,
	
	                    cursBlist.Ymax, cursBlist.Xmax, cursBlist.Ymin, cursBlist.Xmin);
	
	                    ((image *)CursorSave)->imWidth  = sc_tempImgPtr->imWidth;  
	                    ((image *)CursorSave)->imHeight = sc_tempImgPtr->imHeight;  
	                    ((image *)CursorSave)->imAlign  = sc_tempImgPtr->imAlign;  
	                    ((image *)CursorSave)->imFlags  = sc_tempImgPtr->imFlags;  
	                    ((image *)CursorSave)->imBytes  = sc_tempImgPtr->imBytes;  
	                    ((image *)CursorSave)->imBits   = sc_tempImgPtr->imBits;  
	                    ((image *)CursorSave)->imPlanes = sc_tempImgPtr->imPlanes;  
	
	
	                    /* Display the current cursor using transparent replace 
	                       'CursorMask' defines the backcolor part of the cursor   
	                       'CursorImag' defines the forecolor part of the cursor */
	                    cursBlit.blitRop   = 16;  /* transparent replace */
	                    cursWBlit.blitSmap = (grafMap *) CursorMask;
	                    cursBlit.blitFore  = CursBackColor;
	
	                    /* fill in write image blit record */         
	                    cursWBlit.blitRsv   = cursBlit.blitRsv;
	                    cursWBlit.blitAlloc = cursBlit.blitAlloc;
	                    cursWBlit.blitFlags = cursBlit.blitFlags;
	                    cursWBlit.blitRop   = cursBlit.blitRop;
	                    cursWBlit.blitCnt   = cursBlit.blitCnt;
	                    cursWBlit.blitMask  = cursBlit.blitMask;
	                    
#ifdef  FILL_PATTERNS_SUPPORT

	                    cursWBlit.blitPatl  = cursBlit.blitPatl;

#endif /* FILL_PATTERNS_SUPPORT */

	                    cursWBlit.blitDmap  = cursBlit.blitDmap;

#ifndef NO_REGION_CLIP

                        cursWBlit.blitRegn  = cursBlit.blitRegn;

#endif  /* NO_REGION_CLIP */

	                    cursWBlit.blitClip  = cursBlit.blitClip;
	                    cursWBlit.blitBack  = cursBlit.blitBack;
	                    cursWBlit.blitFore  = cursBlit.blitFore;
	                    cursWBlit.blitList  = cursBlit.blitList;
	                    
	                    sc_tempImgPtr1 = (UINT8 *)MEM_malloc(32);
	                    
	                    STR_mem_cpy(sc_tempImgPtr1, &((image *)(cursWBlit.blitSmap))->imData, 32);
	                    ((image *)(cursWBlit.blitSmap))->imData = sc_tempImgPtr1;
	                    (cursBlit.blitDmap->prWrImg)(&cursWBlit);
	                    STR_mem_cpy(&((image *)(cursWBlit.blitSmap))->imData,sc_tempImgPtr1,32);                    
	
	                    cursWBlit.blitSmap = (grafMap *) CursorImag;
	                    cursWBlit.blitFore = CursForeColor;
	
	                    STR_mem_cpy(sc_tempImgPtr1, &((image *)(cursWBlit.blitSmap))->imData, 32);
	                    ((image *)(cursWBlit.blitSmap))->imData = sc_tempImgPtr1;
	                    (cursBlit.blitDmap->prWrImg)(&cursWBlit);
	                    STR_mem_cpy(&((image *)(cursWBlit.blitSmap))->imData,sc_tempImgPtr1,32);
	                    
	                    /* Deallocate Memory */
	                    GRAFIX_Deallocation(sc_tempImgPtr);
	                    GRAFIX_Deallocation(sc_tempImgPtr1);
	
	                    /* Restore global variables */
	                    BLITS_RestoreGlobals();
					}
                } /* if( !Jump ) */

            } /* if( CursorLevel == 0 ) */

        } /* else */

    } /* if( !Jump ) */

    nuResume(cursBlit.blitDmap);

    /* Release the screen semaphore */
    GFX_ReleaseScreenSemaphore();

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    MoveCursor
*
* DESCRIPTION
*
*    Function MoveCursor updates the cursor position to the specified
*    X,Y location.  If the cursor is visible, the cursor is erased and redrawn at
*    the new specified X,Y position.  If the cursor is not visible, the cursor
*    position register is updated but no screen updates take place.
*
* INPUTS
*
*    INT32 argMCX - Cursor X location in pixels.
*
*    INT32 argMCY - Cursor X location in pixels.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID MoveCursor (INT32 argMCX, INT32 argMCY)
{
    INT32 gblX;
    INT32 gblY;


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( globalLevel > 0 )
    {
        /* convert from user to global */
        U2GP(argMCX, argMCY, &gblX, &gblY, 1);
    }
    else
    {
        gblX = argMCX;
        gblY = argMCY;
    }

    nuMoveCursor(gblX, gblY);

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    CursorStyle
*
* DESCRIPTION
*
*    Function CursorStyle prepares a cursor image for cursor operations.
*    argCURNO defines the cursor style number 0 thru 7. If argCURNO is -1, then
*    the current cursor number is re-selected.
*
*    If the cursor is visible, the original cursor is removed and replaced with
*    the new specified cursor style.  If the cursor is not visible, the
*    cursor style is modified and will be displayed when the ShowCursor
*    procedure displays it.
*
* INPUTS
*
*    INT32 argCURNO - Cursor style number 0 thru 7. Or, -1 for current.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID CursorStyle (INT32 argCURNO)
{
    INT16 grafErrValue; /* error value */


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* must be 0 - 7 */
    if( (argCURNO < 0) || (argCURNO > 7) )
    {
        /* if -1, don't post error */
        if( argCURNO != -1 )
        {
            grafErrValue = c_CursorSt + c_BadCursNbr;

            /* report error */
            nuGrafErr(grafErrValue, __LINE__, __FILE__); 
        }

        /* use existing cursor */
        argCURNO = CursorNumbr & 7;
    }

    /* pause the cursor from responding to interrupts
       this locks a semaphore to avoid re-entrancy */
    M_PAUSE(cursBlit.blitDmap);

    /* update cursor number */
    CursorNumbr = argCURNO;
    HideCursor();

    /* lookup the cursor background image */
    CursorMask = (IMAGSTRUCT *) CursorTable[argCURNO][0];
    CursorImag = (IMAGSTRUCT *) CursorTable[argCURNO][1];

    /* lookup the hot spots */
    CursorXoff = CursorTable[argCURNO][2];  
    CursorYoff = CursorTable[argCURNO][3];

#ifdef hyperCursor
    /* precompute shifted cases of cursor images */
    /* can we hyper this kind of device ? */
    if(cursDoHyper != 0 )
    {
        /* yes */
        cursImagHeightM1 = CursorImag->imHeight - 1;
        cursImagBytesM1  = CursorImag->imBytes;
        CURSOR_rsInitShftCurs((image *)CursorMask,(image *)CursorImag);

        /* since hypercursor replaces the backing image with whole bytes
          (doesn't combine edges) the protection rectangle X values must be
           byte aligned */

        /* round Xmin down to nearest byte */
        CursProtXmin = ProtXmin & (~7); 

        /* round Xmax up to next byte */
        CursProtXmax = ProtXmax | 7;    
    }
#else
    /* calculate a protection rectangle that will encompass the cursor */
    /* get protect rect */
    CursProtXmin = ProtXmin;    
    CursProtXmax = ProtXmax;
#endif

    CursProtXmin  = (INT16) (CursProtXmin - CursorImag->imWidth + CursorXoff);
    CursProtYmin  = (INT16) (ProtYmin - CursorImag->imHeight + CursorYoff);
    CursProtXmax += (INT16) CursorXoff;
    CursProtYmax  = (INT16) (ProtYmax + CursorYoff);

    /* check to see if this cursor is in a protected area */
    if( (CursorX < CursProtXmin) || (CursorX >= CursProtXmax) ||
        (CursorY < CursProtYmin) || (CursorY >= CursProtYmax) )
    {
        /* not in a protected area */
        /* turn on the cursor */
        ShowCursor();
    }
    else
    {
        /* cursor is in a protected area */
        /* is cursor already hidden? */
        if( gFlags & gfCurPHid )
        {
            ShowCursor();
        }
        else
        {
            /* no, flag as hidden */
            gFlags |= gfCurPHid;
        }
    }

    nuResume(cursBlit.blitDmap);

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    CursorBitmap
*
* DESCRIPTION
*
*    CursorBitmap sets the bitmap to be used for cursor tracking.
*
* INPUTS
*
*    grafMap *argBMAP - Pointer to a cursor bitmap.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID CursorBitmap (grafMap *argBMAP)
{

#ifdef hyperCursor
    INT16 cursBMapDevTech;
#endif


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* hide on current bitmap */
    HideCursor();

    /* set cursor blit record dest grafmap */
    cursBlit.blitDmap = argBMAP;

    /* set cursor clip rect to cursor bitmap limits */
    cursClip.Xmin = 0;
    cursClip.Ymin = 0;
    cursClip.Xmax = argBMAP->pixWidth - 1;
    cursClip.Ymax = argBMAP->pixHeight - 1;

#ifdef hyperCursor

    cursDoHyper = 0;
    
    cursBMapDevTech = argBMAP->devTech;

    /* can we hyper this kind of device? */
    if( (cursBMapDevTech >= dtEGA) && (cursBMapDevTech <= dtVGA) &&
        (argBMAP->mapWinType <=2) )
    {
        /* yes */
        cursDoHyper++;
        cursBMapYmax = argBMAP->pixHeight- 1;
        cursBMapBytesM1 = argBMAP->pixBytes - 1;
        cursBMapRowTbl =  argBMAP->mapTable[0];

        oldBuff->pntr = (SIGNED) *(argBMAP->mapTable[0] + argBMAP->pixHeight);
        newBuff->pntr = oldBuff->pntr + 3*32*16; 
    }
#endif
 
    /* re-init current cursor style */
    CursorStyle(-1);
    ShowCursor();

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    nuMoveCursor
*
* DESCRIPTION
*
*    Function nuMoveCursor is an internal routine. It updates the
*    cursor position to the  specified global X,Y location.  If the
*    cursor is visible, the cursor is erased and redrawn at the new
*    specified X,Y position.  If the cursor is not visible, the cursor
*    position register is updated but no screen updates take place.
*
* INPUTS
*
*    INT32 glblX - New cursor X position in global coordinates.
*
*    INT32 glblY - New cursor Y position in global coordinates.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID nuMoveCursor(INT32 glblX, INT32 glblY)
{
    UINT8   Done = NU_FALSE;


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /*pause the cursor from interrupt activity even though each
      component of the move (hide,show) is paused, we want to lock
      this entire operation, so that no interrupt activity takes
      place between the components of this move */
    M_PAUSE(cursBlit.blitDmap);

#ifdef hyperCursor
    while( !Done ) 
    {
        if( cursDoHyper )
        {
            /* we can hyper this kind of device */
            /* Update CursorX */
            CursorX = glblX;    

            /* Update CursorY */
            CursorY = glblY;    

            /* check to see if this cursor is:
               1) entering a protected area,
               2) exiting a protected area, or
               3) remaining in a protected area */
            if( (glblX >= CursProtXmin) && (glblY >= CursProtYmin) &&
                (glblX < CursProtXmax) && (glblY < CursProtYmax) )
            {
                /* cursor will be in a protected area */
                if( gFlags & gfCurPHid )
                {
                    /* exit if were we previously protected */    
                    nuResume(cursBlit.blitDmap);
                    Done = NU_TRUE;
                    break; 
                }

                /* cursor entering a protected area */
                /* flag as protected */
                gFlags |= gfCurPHid;

                /* add a hide level */
                CursorLevel--;

                /* was cursor just visible? */
                if( CursorLevel != -1 )
                {
                    /* no, don't bother hiding */ 
                    nuResume(cursBlit.blitDmap);
                    Done = 1;
                    break; 
                }

                mwMoveCurEGA(-1000, -1000);
                nuResume(cursBlit.blitDmap);
                Done = NU_TRUE;
                break; 
            }

            /* not in a protected area */
        
            /* were we previously protected? */
            if( gFlags & gfCurPHid )
            {
                /* was protected, leaving protected area */
                /* clear notion of being protected */
                gFlags = gFlags & ~gfCurPHid;

                /* remove a hide level */
                CursorLevel++;  
            }

            if( CursorLevel < 0 )
            {
                /* cursor is not visible */
                nuResume(cursBlit.blitDmap);
                Done = NU_TRUE;
                break; 
            }

            mwMoveCurEGA(glblX - CursorXoff, glblY - CursorYoff);
            nuResume(cursBlit.blitDmap);
            Done = NU_TRUE;
            break; 
        }

        break; 
    
    } /* while( !Done ) */

#endif

    if( !Done)
    {
        /* Remove cursor at old location */
        HideCursor();       

        /* Update CursorX */
        CursorX = glblX;    

        /* Update CursorY */
        CursorY = glblY;    

        /* Show cursor at new location */
        ShowCursor();       

        nuResume(cursBlit.blitDmap);
    }

    /* Return to user mode */
    NU_USER_MODE();

}

#endif      /* USE_CURSOR */

/***************************************************************************
* FUNCTION
*
*    nuResume
*
* DESCRIPTION
*
*    nuResume unlocks a grafmap.
*
* INPUTS
*
*    grafMap *argGRAFMAP - Pointer to the grafmap to unlock.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID nuResume(grafMap *argGRAFMAP)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* increment mapLock */
    argGRAFMAP->mapLock++;  

    if( argGRAFMAP->mapLock >= 0)
    {
        if( (argGRAFMAP->mapLock > 0) || (argGRAFMAP->mapFlags & mfPending) )
        {
            /* either an error or cursor update pending */
            argGRAFMAP->cbSyncFunc(argGRAFMAP);
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

}

#ifdef  USE_CURSOR

/***************************************************************************
* FUNCTION
*
*    CursorColor
*
* DESCRIPTION
*
*    Function CursorColor sets the GRAFIX_DATA cursor color variables to the
*    specified backCOLOR and foreCOLOR setting.
*
* INPUTS
*
*    INT32 foreCOLOR - Cursor foreground color.
*
*    INT32 backCOLOR - Cursor background color.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID CursorColor (INT32 foreCOLOR, INT32 backCOLOR)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    HideCursor();
    CursBackColor = backCOLOR;
    CursForeColor = foreCOLOR;
    ShowCursor();

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    CURSOR_rsInitShftCurs
*
* DESCRIPTION
*
*    Function CURSOR_rsInitShftCurs initializes 8 shift sequence cursor images
*    in the GRAFDATA "curBUFF" work area from the cursor image definitions
*    specified by curMASK and curIMAG.
*
* INPUTS
*
*    image *srcMASK - Pointer to source mask.
*
*    image *srcIMAG - Pointer to the image definition.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID CURSOR_rsInitShftCurs(image *srcMASK, image *srcIMAG)
{
    /* Size in bytes of a single cursor shift image (bytesPerRow*cursHeight) */
    INT32 curDataSize;  
    INT32 nextRow;
    UINT8 priorMaskLSB;
    UINT8 priorImagLSB;
    UINT8 thisLSB;

    /* Index to cursor temporary storage buffer */
    UINT32 dstBuff;

    /* Index to cursor source image */
    INT32 srcBuff; 
    INT32 iBit;
    INT32 i, j;


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    cursImagBytesM1 = ( (srcMASK->imWidth + 7) >> 3);
    nextRow     = (INT32) (srcMASK->imBytes - cursImagBytesM1);
    cursImagHeightM1 =   srcMASK->imHeight - 1;

    /* Copy master cursor image to first shift image. */
    dstBuff = 0;
    srcBuff = 0;

    for( i = (INT32) cursImagHeightM1; i >= 0; i--)
    {
        /* Store up to 4 bytes per row */
        for( j = (INT32) cursImagBytesM1; j > 0; j--)
        {
            curBuffer[dstBuff++] = srcMASK->imData[srcBuff];
            curBuffer[dstBuff++] = srcIMAG->imData[srcBuff++];
        }
        curBuffer[dstBuff++] = 0;
        curBuffer[dstBuff++] = 0;
        srcBuff += nextRow;
    }

    curDataSize = (INT32) dstBuff;
    srcBuff = 0;

    /* build 7 shifted images */
    for( iBit = 0; iBit < 7; iBit++)
    {
        for( i = (INT32) cursImagHeightM1; i >= 0; i--)
        {
            priorMaskLSB = 0;
            priorImagLSB = 0;
            for( j = (INT32) cursImagBytesM1; j >= 0; j--)
            {
                thisLSB = ( (curBuffer[srcBuff] & 1) << 7);
                curBuffer[dstBuff++] = ( (curBuffer[srcBuff++] >> 1) | priorMaskLSB);
                priorMaskLSB = thisLSB;

                thisLSB = ( (curBuffer[srcBuff] & 1) << 7);
                curBuffer[dstBuff++] = ( (curBuffer[srcBuff++] >> 1) | priorImagLSB);
                priorImagLSB = thisLSB;
            }
        }
    }

    dstBuff = (SIGNED) &curBuffer;

    /* store in active cursor buffer */
    for( iBit = 0; iBit < 8; iBit++)
    {
        curActive[iBit][0] = dstBuff;
        curActive[iBit][1] = dstBuff;
        curActive[iBit][2] = dstBuff;
        curActive[iBit][3] = dstBuff;

        dstBuff += curDataSize;
    }

    /* Return to user mode */
    NU_USER_MODE();

}

/***************************************************************************
* FUNCTION
*
*    ProtectRect
*
* DESCRIPTION
*
*    Function ProtectRect hides the cursor only when inside the passed
*    rectangle boundary.  If the cursor is currently within the specified
*    rectangle, it is removed ("hidden") until the cursor moves out of the
*    rectangle. If the cursor is not within the specified rectangle, it remains
*    visible until it is moved within the protected area.
*
*    The protection rectangle can be disabled by passing either a NULL pointer
*    or a NULL rectangle.
*
* INPUTS
*
*    rect *argPR - Pointer to the rectangle boundary.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID ProtectRect(rect *argPR)
{
    UINT8   Jump = NU_FALSE;

    rect gblR;
    rect tempR;

 
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

   /* check for NULL pointer or null rect */
    if( (argPR == 0) || (argPR->Xmax <= argPR->Xmin) || (argPR->Ymax <= argPR->Ymin) )
    {
        /* turn off protect rect */
        /* pause cursor */
        M_PAUSE(cursBlit.blitDmap);

        /* disable protect rect, force off bitmap */
        ProtXmin = 0x7F00;
        CursProtXmin = 0x7FFF;

        /* is cursor hidden via protect? */
        if( gFlags & gfCurPHid )
        {
            /* flag no longer hidden via protect */
            gFlags &= ~gfCurPHid;

            /* redisplay cursor */
            ShowCursor();   
        }

        /* exit */
        Jump = NU_TRUE; 
    }

    if( !Jump )
    {
        tempR = *argPR;

        if( globalLevel > 0 )
        {
            /* convert from user to global */
            U2GR(tempR, &gblR, 0);
        
            /* save as new protect rect */
            ProtXmin = gblR.Xmin;
            ProtYmin = gblR.Ymin;
            ProtXmax = gblR.Xmax;
            ProtYmax = gblR.Ymax;
        }
        else
        {
            /* save as new protect rect */
            ProtXmin = tempR.Xmin;
            ProtYmin = tempR.Ymin;
            ProtXmax = tempR.Xmax;
            ProtYmax = tempR.Ymax;
        }

        /* pause cursor */
        M_PAUSE(cursBlit.blitDmap);

        /* calculate protect rect that encompasses cursor
           since hypercursor replaces the backing image with whole bytes
           (doesn't combine edges) the protection rectangle X values must be
           byte aligned */
#ifdef  hyperCursor

        /* can we hyper this kind of device? */
        if( cursDoHyper != 0 )
        {
            /* round Xmin down to nearest byte */
            ProtXmin = ProtXmin & ~7; 

            /* round Xmax up to next byte */
            ProtXmax |= 7;            
        }
#endif

        /* is cursor even initialized? */
        if( gFlags & gfCurInit )
        {
            CursProtXmin = (INT16) (ProtXmin - CursorImag->imWidth + CursorXoff);
            CursProtYmin = (INT16) (ProtYmin - CursorImag->imHeight + CursorYoff);
            CursProtXmax = (INT16) (ProtXmax + CursorXoff);
            CursProtYmax = (INT16) (ProtYmax + CursorYoff);

            /* is cursor in protected area? */
            if( (CursorX < CursProtXmin) || (CursorX >= CursProtXmax) ||
                (CursorY < CursProtYmin) || (CursorY >= CursProtYmax) )
            {
                /* not in a protected area */
            
                /* as it hidden via protect? */
                if( gFlags & gfCurPHid )
                {
                    /* not hidden via protect anymore */
                    gFlags &= ~gfCurPHid; 

                    /* show it */
                    ShowCursor();         
                }
            }
            else
            {
                /* cursor is in a protected area */

                /* is cursor visible? */
                if( CursorLevel >= 0 )
                {
                    /* yes, hide cursor and flag as protected */
                    HideCursor();
                    gFlags |= gfCurPHid;
                }
            }
        }
    } /* if( !Done ) */

    nuResume(cursBlit.blitDmap);

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    ProtectOff
*
* DESCRIPTION
*
*    Function ProtectOff releases the the cursor for normal movement operation.
*    If the cursor had been "hidden" by the previous ProtectRect call, it is
*    redisplayed and allowed to follow full tracking movements.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID ProtectOff(VOID)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* pause cursor */
    M_PAUSE(cursBlit.blitDmap);

    /* disable protect rect, force off bitmap */
    ProtXmin = 0x7F00;
    CursProtXmin = 0x7FFF;

    /* is cursor hidden via protect? */
    if( gFlags & gfCurPHid )
    {
        /* flag no longer hidden via protect and redisplay cursor */
        gFlags &= ~gfCurPHid;
        ShowCursor();
    }

    nuResume(cursBlit.blitDmap);

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    DefineCursor
*
* DESCRIPTION
*
*    Function DefineCursor loads the specified cursor number, CURSORNBR (0-7),
*    with the user specified cursor mask, cursor image, and center spot X,Y.
*    If the image pointers are NULL, then the original pre-defined data for the
*    specified cursor number is re-loaded.
*
* INPUTS
*
*    INT32 argCNBR     - Cursor number, CURSORNBR (0-7).
*
*    INT32 argHOTX     - Center spot X.
*
*    INT32 argHOTY     - Center spot Y.
*
*    image *argBACKIMG - Pointer to background image.
*
*    image *argFOREIMG - Pointer to foreground image.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID DefineCursor(INT32 argCNBR, INT32 argHOTX, INT32 argHOTY, image *argBACKIMG, image *argFOREIMG)
{
    INT32 allowCurrentSize = 32;

    UINT8 Jump = NU_FALSE;

    /* error value */
    INT16 grafErrValue; 


    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* pause cursor */
    M_PAUSE(cursBlit.blitDmap);

    /* Remove the cursor if visible */
    HideCursor();   

    /* is background image pointer null? */
    if( argBACKIMG == 0 )
    {
        /* load args from pre-defined cursors table */
        argBACKIMG = (image *) DefCursorTable[argCNBR][0];
        argFOREIMG = (image *) DefCursorTable[argCNBR][1];
        argHOTX    =   (INT32) DefCursorTable[argCNBR][2];
        argHOTY    =   (INT32) DefCursorTable[argCNBR][3];
    }

    /* can we handle this size of cursor */
    if( argBACKIMG->imWidth > allowCurrentSize )
    {
        argBACKIMG->imWidth = allowCurrentSize;
        grafErrValue = c_DefineCu +  c_BadCursSiz;

        /* report error */
        nuGrafErr(grafErrValue, __LINE__, __FILE__);    
        Jump = NU_TRUE;
    }

    if( !Jump )
    {
        CursorTable[argCNBR][0] = (SIGNED)argBACKIMG;

        /* can we handle this size of cursor */
        if( argFOREIMG->imWidth > allowCurrentSize )
        {
            argFOREIMG->imWidth = allowCurrentSize;
            grafErrValue = c_DefineCu +  c_BadCursSiz;

            /* report error */
            nuGrafErr(grafErrValue, __LINE__, __FILE__);    
            Jump = NU_TRUE;
        }

        if( !Jump )
        {
            CursorTable[argCNBR][1] = (SIGNED) argFOREIMG;

            /* save new hot spots */
            CursorTable[argCNBR][2] = argHOTX;
            CursorTable[argCNBR][3] = argHOTY;

            /* re-cursorstyle in case the current cursor was re-defined */
            CursorStyle(-1);
        }
    }

    /* Restore cursor if visible */
    ShowCursor(); 
    nuResume(cursBlit.blitDmap);

    /* Return to user mode */
    NU_USER_MODE();

}

#endif      /* USE_CURSOR */

