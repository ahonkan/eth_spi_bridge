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
*       sck_gpmtu.c
*
*   DESCRIPTION
*
*       This file contains the routine to determine the Path MTU of a
*       destination.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Get_PMTU
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Get_PMTU
*
*   DESCRIPTION
*
*       This function returns the Path MTU of a source / destination
*       address combination.
*
*   INPUTS
*
*       *src_addr               The source address.
*       *dest_addr              The destination address.
*       family                  The family type of the destination address.
*       *status                 A pointer to the status field that will
*                               be filled in upon return.
*
*   OUTPUTS
*
*       Path MTU of the destination or 0 if not found.
*
*************************************************************************/
UINT32 NU_Get_PMTU(const UINT8 *src_addr, const UINT8 *dest_addr,
                   INT16 family, STATUS *status)
{
    ROUTE_ENTRY     *rt_entry;
    UINT32          path_mtu;
#if (INCLUDE_IPV4 == NU_TRUE)
    SCK_SOCKADDR_IP de;
#endif

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (dest_addr == NU_NULL) || (src_addr == NU_NULL) )
    {
        *status = NU_INVALID_PARM;
        return (0);
    }

    if (status == NU_NULL)
        return (0);

#endif

    NU_SUPERVISOR_MODE();

    /* We must grab the NET semaphore */
    *status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (*status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (0);
    }

    /* Get a pointer to the route associated with the destination */
#if (INCLUDE_IPV6 == NU_TRUE)
    if (family == NU_FAMILY_IP6)
        rt_entry = (ROUTE_ENTRY*)RTAB6_Find_Route(dest_addr, 0);

    else
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    if (family == NU_FAMILY_IP)
    {
        de.sck_addr = IP_ADDR(dest_addr);
        rt_entry = (ROUTE_ENTRY*)RTAB4_Find_Route(&de, 0);
    }

    /* Otherwise, an invalid family was passed into the routine */
    else
#endif
    {
        rt_entry = NU_NULL;
        *status = NU_INVALID_PARM;
    }

    /* If a route was found */
    if (rt_entry)
    {
        *status = NU_SUCCESS;
        path_mtu = rt_entry->rt_path_mtu;
    }

    /* Otherwise, no route exists to the destination */
    else
    {
        *status = NU_NO_ROUTE_TO_HOST;
        path_mtu = 0;
    }

    /* Release the semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    NU_USER_MODE();

    return (path_mtu);

} /* NU_Get_PMTU */

