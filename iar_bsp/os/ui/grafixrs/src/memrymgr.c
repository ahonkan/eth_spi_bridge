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
*  memrymgr.c                                                   
*
* DESCRIPTION
*
*  This file contains the Memory management routines that map to either the
*  std lib or OS specific functions
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  MEM_allocation_error
*  MEM_calloc
*  MEM_malloc
*  MEM_allocate_from_module
*
* DEPENDENCIES
*
*  noatom.h
*  rs_base.h
*  rs_api.h
*  memrymgr.h
*  str_utils.h
*  std_utils.h
*  pens.h
*  zect.h
*  texts.h
*  stopgfx.h
*  display_config.h
*  colors.h
*
***************************************************************************/

/* undefine MS Windows stuff if this is a WIN32 APP */
#ifdef NU_SIMULATION
#include "ui/noatom.h"
#endif

#include "ui/rs_base.h"
#include "ui/rs_api.h"
#include "ui/memrymgr.h"
#include "ui/str_utils.h"
#include "ui/std_utils.h"
#include "ui/pens.h"
#include "ui/zect.h"
#include "ui/texts.h"
#include "ui/stopgfx.h"

#include "drivers/display_config.h"

#ifdef CFG_NU_OS_DRVR_DISPLAY_ENABLE

#include "drivers/colors.h"

#endif

#define _USE_NU_ALLOC
#ifdef _USE_NU_ALLOC
#include "nucleus.h"
extern NU_MEMORY_POOL  System_Memory;
#endif

static VOID MEM_allocation_error(INT32 error);

#ifdef  GFX_VERBOSE_ERROR

extern VOID SetPort( rsPort *portPtr);

static const UNICHAR singleSpace[]   = {' ',0};
static const UNICHAR appShutDown[]  = {'A','p','p','l','i','c','a','t','i','o','n',
                                       ' ','w','i','l','l',' ','n','o','w',' ','S',
                                       'h','u','t',' ','D','o','w','n','!',0};
static const UNICHAR pressToContinue[] = {'P','r','e','s','s',' ','a','n','y',' ','k',
                                          'e','y',' ','t','o',' ','C','o','n','t','i',
                                          'n','u','e','.','.','.',0};
static const UNICHAR allocFailed[]  = {' ',' ',' ',' ',' ',' ',' ',' ',' ','M','e','m',
                                       'o','r','y',' ','A','l','l','o','c','a','t','i',
                                       'o','n',' ','F','a','i','l','e','d','!',' ',' ',
                                       ' ',' ',' ',' ',' ',' ',' ',0};

#endif  /* GFX_VERBOSE_ERROR */

/***************************************************************************
* FUNCTION
*
*    MEM_allocation_error
*
* DESCRIPTION
*
*    Function MEM_allocation_error displays a fatal error on the screen in 
*    the event that one of the memory allocation routines fail.
*
* INPUTS
*
*    INT32 error   - Error number that indicates which routine failed.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID MEM_allocation_error(INT32 error)
{
    
#ifdef  GFX_VERBOSE_ERROR
    
    INT32       cnt;
    INT32       line;
    INT32       x;
    INT32       y;
    rect        r;
    UNICHAR     tmpStr[20];
    UNICHAR     errorMsg[255];
    point       theCenter;
    TextSetUp   newTextSetUp;
    PenSetUp    newPenSetUp;

#ifdef USE_UNICODE    
    DrawTextType strType = STRWIDE;
#else
	DrawTextType strType = STR;
#endif

    /* make sure the message will be visible to the user */
    SetPort(thePort);

    theCenter.X = thePort->portRect.Xmax/2;
    theCenter.Y = thePort->portRect.Ymax/2;

    /* align the text */
    line = ((fontRcd *)thePort->txFont)->lnSpacing;
    x = theCenter.X;
    y = theCenter.Y;
    y -= line * 2;

    STR_str_cpy(errorMsg,allocFailed);
    STR_str_cat(errorMsg,singleSpace);
    STR_str_cat(errorMsg,STD_i_toa(error,tmpStr,10));

#ifdef USE_UNICODE
    CenterRect(&theCenter,StringWidth16(errorMsg) + 100,200,&r);
#else
    CenterRect(&theCenter,StringWidth(errorMsg) + 100,200,&r);
#endif

    RS_Get_Text_Setup(&newTextSetUp);
    newTextSetUp.face                = cProportional;
    newTextSetUp.charHorizontalAlign = alignCenter;
    newTextSetUp.charVerticalAlign   = alignMiddle;
    RS_Text_Setup(&newTextSetUp);

    RS_Get_Pen_Setup(&newPenSetUp);
    
#ifdef CFG_NU_OS_DRVR_DISPLAY_ENABLE
    RS_Pen_Setup(&newPenSetUp, Black);
    BackColor(LtGray);
#else
    RS_Pen_Setup(&newPenSetUp, 0);
    BackColor(7);
#endif

    /* draw the window frame */
    RS_Rectangle_Draw(FILL, &r, 0, 0, 0);

    RS_Rectangle_Draw(FRAME, &r, -1, 0, 0);

    for(cnt = 0; cnt < 4; cnt++)
    {
        InsetRect(&r, 1, 1);
    }

    InsetRect(&r, 3, 3);
    InsetRect(&r, 1, 1);

    /* print the message */
    MoveTo(x, y);

    RS_Text_Draw(errorMsg, strType, 0, 127);
    y += line;
    MoveTo(x, y);

    RS_Text_Draw((VOID *)appShutDown, strType, 0, 127);
    y += line * 2;
    MoveTo(x, y);

    RS_Text_Draw((VOID *)pressToContinue, strType, 0, 127);

#else

    NU_UNUSED_PARAM(error);
    
#endif  /* GFX_VERBOSE_ERROR */
    
    StopGraphics();
}

/***************************************************************************
* FUNCTION
*
*    MEM_calloc
*
* DESCRIPTION
*
*    Function MEM_calloc is the replacement for calloc(), for reentrancy.
*
* INPUTS
*
*    UINT32 cnt  - Count.
*    UINT32 size - Size.
*
* OUTPUTS
*
*    VOID*
*
***************************************************************************/
VOID* MEM_calloc(UINT32 cnt, UINT32 size)
{
    VOID        *address = NU_NULL;

#ifdef _USE_NU_ALLOC

    STATUS      status;
    UINT8       *a;           /* pointer used in zero fill */
    UINT32      alloc_size;   /* calculated size of memory block */
    UINT32      i;            /* loop counter for zero fill */

    alloc_size = cnt*size;   

    status = GRAFIX_Allocation(&System_Memory, &address, alloc_size + 4, NU_NO_SUSPEND );

    /* if the allocation was successful zero fill the memory block */
    if( status == NU_SUCCESS )
    {
        a = (UINT8 *)address;

        for(i = 0; i < alloc_size; i++)
        {
            *a++ = 0;
        }
    } 
    
#else
    address = calloc(cnt, size);
#endif

    if( address == NU_NULL )
    {
        MEM_allocation_error(status);
    }

    return (address);
}  

/***************************************************************************
* FUNCTION
*
*    MEM_malloc
*
* DESCRIPTION
*
*    Function MEM_Malloc is the replacement for malloc(), for reentrancy.
*
* INPUTS
*
*    UINT32 size - Size.
*
* OUTPUTS
*
*    VOID*
*
***************************************************************************/
VOID* MEM_malloc( UINT32 size )
{
    VOID        *address = NU_NULL;
    STATUS      status;

#ifdef _USE_NU_ALLOC
    status = GRAFIX_Allocation(&System_Memory, &address, size + 4, NU_NO_SUSPEND );
#else
    address = malloc(size);
#endif

    if( status != NU_SUCCESS )
    {
        MEM_allocation_error(status);
    }

    return (address);
}
