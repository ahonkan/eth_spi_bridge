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
*       sck_dr.c
*
*   DESCRIPTION
*
*       This file contains the implementation of the function
*       NU_Delete_Route2.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Delete_Route2
*
*   DEPENDENCIES
*
*       nu_net.h
*
**************************************************************************/

#include "networking/nu_net.h"

/************************************************************************
*
*   FUNCTION
*
*       NU_Delete_Route2
*
*   DESCRIPTION
*
*       Deletes a route from the routing table.
*
*   INPUTS
*
*       *ip_dest                The destination IP address for the route
*                               to be deleted.
*       *next_hop               The next-hop associated with the
*                               destination.  If this parameter is
*                               NU_NULL, all routes to the destination
*                               will be deleted.
*       family                  The family of the route to delete.
*
*   OUTPUTS
*
*       NU_SUCCESS              The route was successfully deleted.
*       NU_NOT_FOUND            The route does not exist.
*       NU_INVALID_ADDRESS      The route was not found
*       NU_INVALID_PARM         One of the parameters is an invalid value.
*
*************************************************************************/
STATUS NU_Delete_Route2(const UINT8 *ip_dest, const UINT8 *next_hop,
                        INT16 family)
{
    STATUS  status;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (INCLUDE_IPV6 != NU_TRUE)
    /* Remove compiler warnings */
    UNUSED_PARAMETER(family);
#endif

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (status);
    }

#if (INCLUDE_IPV6 == NU_TRUE)

    /* Delete the route according to the family specified */
    if (family == NU_FAMILY_IP6)
        status = RTAB6_Delete_Route(ip_dest, next_hop);

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

        status = RTAB4_Delete_Route(ip_dest, next_hop);

#endif

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Delete_Route2 */
