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
*   FILENAME
*
*       dhcp_giaid.c
*
*   DESCRIPTION
*
*       This file contains the API routine to get the IAID for an interface
*       at run-time.  The same IAID is used for IPv4 and IPv6 DHCP
*       transactions on an interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Get_DHCP_IAID
*
*   DEPENDENCIES
*
*       nu_net.h
*
**************************************************************************/

#include "networking/nu_net.h"

#if ( (INCLUDE_DHCP == NU_TRUE) || (INCLUDE_DHCP6 == NU_TRUE) )

/*************************************************************************
*
*   FUNCTION
*
*       NU_Get_DHCP_IAID
*
*   DESCRIPTION
*
*       This routine gets the IAID for a given interface.  The interface
*       index can be found at the application layer via a call to the
*       routine NU_IF_NameToIndex().
*
*   INPUTS
*
*       dev_index               The index of the interface for which to
*                               get the IAID.
*       *iaid                   A pointer to the memory into which to
*                               place the IAID for the interface.
*
*   OUTPUTS
*
*       NU_SUCCESS              The IAID was successfully retrieved.
*       NU_INVALID_PARM         A matching interface was not found.
*
*************************************************************************/
STATUS NU_Get_DHCP_IAID(const UINT32 dev_index, UINT32 *iaid)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the semaphore */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get a pointer to the interface. */
        dev_ptr = DEV_Get_Dev_By_Index(dev_index);

        /* If a pointer to the interface was found. */
        if (dev_ptr)
        {
            /* Get the IAID associated with the interface. */
            *iaid = dev_ptr->dev_dhcp_iaid;
        }

        /* Release the semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

    else
    {
        status = NU_INVALID_PARM;

        NLOG_Error_Log("No matching device entry for name",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Get_DHCP_IAID */

#endif
