/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       zc_db.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_ZC_Deallocate_Buffer.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_ZC_Deallocate_Buffer
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/************************************************************************
*
*   FUNCTION
*
*       NU_ZC_Deallocate_Buffer
*
*   DESCRIPTION
*
*       This function allows the application to return the currently
*       allocated ZEROCOPY buffer back to the freelist.
*
*   INPUTS
*
*       *ppc                    Pointer to the buffer to deallocate.
*
*   OUTPUTS
*
*       NU_SUCCESS              The buffer was successfully returned to
*                               the free list.
*       NU_INVALID_PARM         The pointer passed in is not valid.
*
*************************************************************************/
STATUS NU_ZC_Deallocate_Buffer(NET_BUFFER *ppc)
{
    STATUS  status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

     /* If a valid pointer was passed in, free it */
    if (ppc)
    {
        /* Place this buffer back onto the free list. */
        MEM_One_Buffer_Chain_Free(ppc, &MEM_Buffer_Freelist);

        status = NU_SUCCESS;
    }
    else
        status = NU_INVALID_PARM;

    /* Return to user mode */
    NU_USER_MODE();

    return (status);

} /* NU_ZC_Deallocate_Buffer */
