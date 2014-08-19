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
*****************************************************************************

****************************************************************************
*
* FILE NAME                                                                 
*
*  gfxerr_queryerror.c
*
* DESCRIPTION
*
*  This file contains the error related function - QueryError.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  QueryError
*
* DEPENDENCIES
*
*  nucleus.h
*  nu_kernel.h
*  rs_app.h
*
***************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "ui/rs_app.h"

extern INT16 grafError;


/***************************************************************************
* FUNCTION
*
*    QueryError
*
* DESCRIPTION
*
*    Function QueryError returns the first error posted by GrafErr.
*    It resets the error to 0.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    INT32 - Returns the "grafError" value.
*
***************************************************************************/
INT32 QueryError(VOID)
{
    INT32 returnValue;

    /* Fetch value  */
    returnValue = grafError; 

    /* end reset    */
    grafError   = 0;         
    
    /* return from the function */
    return(returnValue);
}
