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
*       sck_if_itn.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_IF_IndexToName.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_IF_IndexToName
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
*       NU_IF_IndexToName
*
*   DESCRIPTION
*
*       This function maps an interface index to its corresponding name.
*
*   INPUTS
*
*       if_index                The interface index associated with the
*                               interface.
*       *if_name                The name of the interface in the system.
*
*   OUTPUTS
*
*       The interface name of the interface associated with if_index or
*       NU_NULL                 One of the parameters is invalid or there
*                               is no interface in the system
*                               corresponding with if_index.
*
*************************************************************************/
CHAR *NU_IF_IndexToName(INT32 if_index, CHAR *if_name)
{
    DV_DEVICE_ENTRY *dv_entry;
    CHAR            *ret_name;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Validate the parameters passed into the function */
    if ( (if_name == NU_NULL) || (if_index < 0) )
        return (NU_NULL);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

     /* Get the Nucleus NET semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (NU_NULL);
    }

    /* Get a pointer to the interface associated with if_name */
    dv_entry = DEV_Get_Dev_By_Index((UINT32)if_index);

    /* If an interface exists, return the name of the interface */
    if (dv_entry)
    {
        strcpy(if_name, dv_entry->dev_net_if_name);
        ret_name = if_name;
    }

    /* Otherwise, return NULL */
    else
        ret_name = NU_NULL;

    /* Release the TCP semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (ret_name);

} /* NU_IF_IndexToName */
