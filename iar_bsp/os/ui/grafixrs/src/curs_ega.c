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
*  curs_ega.c                                                   
*
* DESCRIPTION
*
*  This file contains cursor specific data structures.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  curs_ega
*
* DEPENDENCIES
*
*  rs_base.h
*  rserrors.h
*  curs_ega.h
*
***************************************************************************/

#include "ui/rs_base.h"
#include "ui/rserrors.h"
#include "ui/curs_ega.h"

#ifdef      USE_CURSOR

#ifdef      hyperCursor

/* Global Variables */
CursComp *oldBuff;
CursComp *newBuff;

#endif      /* hyperCursor */

/***************************************************************************
* FUNCTION
*
*    curs_ega
*
* DESCRIPTION
*
*    Function curs_ega sets up the cursor save pointers.
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
void curs_ega(void)
{

#ifdef      hyperCursor
    
    oldBuff  = (CursComp *) (&CursorSave);
    newBuff  = oldBuff + 1;

#endif      /* hyperCursor */
    
}

#endif
