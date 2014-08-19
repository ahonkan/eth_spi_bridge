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
*       dhcp_siaid.c
*
*   DESCRIPTION
*
*       This file contains the API routine to set the IAID for an interface
*       at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Set_DHCP_IAID
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
*       NU_Set_DHCP_IAID
*
*   DESCRIPTION
*
*       This routine sets the IAID for a given interface.  The interface
*       index can be found at the application layer via a call to the
*       routine NU_IF_NameToIndex().
*
*   INPUTS
*
*       dev_index               The index of the interface for which to
*                               set the IAID.
*       dev_iaid                The new IAID to assign to the interface.
*
*   OUTPUTS
*
*       NU_SUCCESS              The IAID was successfully set.
*       NU_INVALID_PARM         A matching interface was not found.
*
*************************************************************************/
STATUS NU_Set_DHCP_IAID(const UINT32 dev_index, UINT32 dev_iaid)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev_ptr;

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
            /* Set the new IAID for the interface. */
            dev_ptr->dev_dhcp_iaid = dev_iaid;
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

} /* NU_Set_DHCP_IAID */

#endif
