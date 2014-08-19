/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
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
*        ip6_mib_icmp.c                              
*
*   COMPONENT
*
*        IPv6 - ICMP MIB Group.
*
*   DESCRIPTION
*
*        This file contain the implementation of the routine that will
*        be used to get statistics for the IPv6 ICMP Group.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        IP6_MIB_ICMP_Get_Stat
*
*   DEPENDENCIES
*
*        nu_net.h
*        ip6_mib.h
*        snmp_api.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/ip6_mib.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#endif

#if (INCLUDE_IPV6_ICMP_MIB == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_ICMP_Get_Stat
*
*   DESCRIPTION
*
*        This function is used to get the IPv6 ICMP statistics.
*
*   INPUTS
*
*        if_index               Interface index.
*        option                 Specification of the statistics.
*        *value                 Pointer to the memory location where value
*                               of statistics need to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   When no IPv6 interface is present with
*                               interface index passed in.
*        IP6_MIB_NOSUCHNAME     Invalid option.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_ICMP_Get_Stat(UINT32 if_index, UINT32 option, UINT32 *value)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status to return success or error code. */
    UINT16              status;

    /* If we have invalid interface index then return error code. */
    if (if_index == 0)
        status = IP6_MIB_NOSUCHOBJECT;

    /* If we have invalid value for option then return error code. */
    else if ( (option > IP6_ICMP_MIB_COUNTERS) || (option == 0) )
        status = IP6_MIB_NOSUCHNAME;

    /* Grab the semaphore. */
    else if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code if we failed to obtain semaphore. */
        status = IP6_MIB_ERROR;
    }

    /* If we successfully grab the semaphore. */
    else
    {
        /* Get the handle to the interface device. */
        dev = DEV_Get_Dev_By_Index((if_index - 1));

        /* We did not get the handle to the interface device or interface
         * device is not appropriate one then return error code.
         */
        if ( (dev == NU_NULL) || (dev->ip6_icmp_mib == NU_NULL) )
            status = IP6_MIB_NOSUCHOBJECT;

        /* If we got the handle to the appropriate device. */
        else
        {
            /* Get the value of required IPv6 ICMP statistics. */
            (*value) = dev->ip6_icmp_mib[(UINT16)option - 1];

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* Release semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_ICMP_Get_Stat */

#endif /* (INCLUDE_IPV6_ICMP_MIB == NU_TRUE) */
