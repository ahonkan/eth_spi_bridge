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
*  grfxerr.c
*
* DESCRIPTION
*
*  This file contains the error handling functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  nuGrafErrInternal
*  nuClearErrList
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

#ifdef  GFX_VERBOSE_ERROR

/* Global error list */
GFX_ERROR   *errorList = NU_NULL;

#endif  /* GFX_VERBOSE_ERROR */

VOID       (*errCBProc)() = NU_NULL;

/* Local Functions */
VOID nuClearErrList(VOID);
INT32 QueryError(VOID);

/***************************************************************************
* FUNCTION
*
*    nuGrafErr
*
* DESCRIPTION
*
*    Function nuGrafErr leaves all registers unchanged!  Each error and it's 
*    information is recorded in a linked list structure.
*
* INPUTS
*
*    INT16    ErrValue - Error code.
*    INT16    lineNum  - line number of the error
*    UNICHAR *fileName - the file that the error occurred.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
#ifdef  GFX_VERBOSE_ERROR
VOID nuGrafErrInternal(INT16 ErrValue, INT16 lineNum, UNICHAR *fileName)
#else
VOID nuGrafErrInternal(INT16 ErrValue)
#endif  /* GFX_VERBOSE_ERROR */
{
    
#ifdef  GFX_VERBOSE_ERROR

    GFX_ERROR *temp_error;
    GFX_ERROR *new_error;
    
#endif  /* GFX_VERBOSE_ERROR */

    grafError = ErrValue;
    
    /* Check for a custom error handler */
    if (errCBProc)
    {
        /* Call the custom error handler */
        (*errCBProc)();
    }
    
#ifdef  GFX_VERBOSE_ERROR

    else
    {
        /* Get the address of the main error structure */
        temp_error = errorList;

        /* See if any errors are already recorded. */
        if (temp_error)
        {
            /* Allocate memory for the new error */
            new_error = MEM_calloc(1, sizeof(struct GFX_ERROR_STRUCT));

            /* Ensure that error record is available. */
            if (new_error)
            {
                /* errors are present, so we need the last one in the list */
                while (temp_error->next)
                {
                    /* Traverse the list */
                    temp_error = temp_error->next;
                }
    
                /* Add the new error to the list */
                temp_error->next = new_error;
    
                /* Set the previous pointer in the new error structure. */
                new_error->prev = temp_error;
            }
        }

        /* No errors currently recorded.  Start a new list */
        else
        {
            /* Allocate memory for the first error in the new list */
            errorList = MEM_calloc(1, sizeof(struct GFX_ERROR_STRUCT));

            /* Set the new list to a local variable for temp modifications. */
            new_error = errorList;
        }

        /* Ensure that error record is available. */
        if (new_error)
        {
            /* record the line data of the error */
            new_error->lineNum = lineNum;
    
            /* record the file name that the error occurred */
            STR_str_cpy (new_error->fileName, fileName);
    
            /* Record the error value that was passed in */
            new_error->ErrValue = ErrValue;
        }
    }

#endif  /* GFX_VERBOSE_ERROR */
    
}

#ifdef  GFX_VERBOSE_ERROR

/***************************************************************************
* FUNCTION
*
*    nuClearErrList
*
* DESCRIPTION
*
*    Function nuClearErrList removes all errors recorded and deallocates
*    memory used by the list.
*
* INPUTS
*
*    INT16    ErrValue - Error code.
*    INT16    lineNum  - line number of the error
*    UNICHAR *fileName - the file that the error occurred.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID nuClearErrList(VOID)
{
    GFX_ERROR *temp_error;
    GFX_ERROR *new_error;

    /* Get the address of the main error structure */
    temp_error = errorList;

    /* check for an error list */
    if (temp_error)
    {
        /* Go the end of the error list */
        while (temp_error->next)
        {
            temp_error = temp_error->next;
        }

        while (temp_error)
        {

            new_error = temp_error->prev;

            GRAFIX_Deallocation(temp_error);

            temp_error = new_error;
        }

        /* null the errorList so it can start over */
        errorList = NU_NULL;
    }
}

#endif  /* GFX_VERBOSE_ERROR */
