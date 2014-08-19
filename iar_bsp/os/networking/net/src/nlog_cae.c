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
*       nlog_cae.c
*
*   COMPONENT
*
*       Nucleus NET error logging function.
*
*   DESCRIPTION
*
*       This file contains the Nucleus routine for clearing all the
*       logged errors.
*
*   DATA STRUCTURES
*
*       NONE
*
*   FUNCTIONS
*
*       NLOG_Clear_All_Errors
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

extern INT NLOG_Avail_Index;
extern UINT16  NLOG_Internal_Error;

/*************************************************************************
*
*   FUNCTION
*
*       NLOG_Clear_All_Errors
*
*   DESCRIPTION
*
*       This routine will reset the NLOG_Avail_Index value back to 0,
*       which will in effect, clear all the current errors from the
*       NLOG_Entry_List array.
*
*       Will update the value to the NLOG_Avail_Index variable.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success     .
*
*
*************************************************************************/
STATUS NLOG_Clear_All_Errors(VOID)
{
    STATUS status;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    /* allocate the TCP/IP resource for blocking during this time */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* clear all the error by resetting the index value */
        NLOG_Avail_Index = 0;


        /* clear the internal NLOG error counter */
        NLOG_Internal_Error++;

        /* deallocate the TCP/IP resource */
        status = NU_Release_Semaphore(&TCP_Resource);

        NU_USER_MODE();
    }
    else
        NU_USER_MODE();


    return (status);

} /* NLOG_Clear_All_Errors */
