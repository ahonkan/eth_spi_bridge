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
*       sck_if_nti.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_IF_NameToIndex.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_IF_NameToIndex
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_IF_NameToIndex
*
*   DESCRIPTION
*
*       This function maps an interface name to its corresponding index.
*
*   INPUTS
*
*       *if_name                The name of an interface in the system.
*
*   OUTPUTS
*
*       The interface index of the interface associated with if_name or
*       -1                      No interface exists with the name if_name.
*       NU_INVALID_PARM         if_name is NULL.
*
*************************************************************************/
INT32 NU_IF_NameToIndex(const CHAR *if_name)
{
    DV_DEVICE_ENTRY *dv_entry;
    INT32           if_index;
    STATUS          status;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Validate the parameter passed into the function */
    if (if_name == NU_NULL)
        return (NU_INVALID_PARM);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

     /* Get the Nucleus NET semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (status);
    }

    /* Get a pointer to the interface associated with if_name */
    dv_entry = DEV_Get_Dev_By_Name(if_name);

    /* If an interface exists, return the index of the interface */
    if (dv_entry)
        if_index = (INT32)(dv_entry->dev_index);

    /* Otherwise, return an error */
    else
        if_index = -1;

    /* Release the TCP semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (if_index);

} /* NU_IF_NameToIndex */
