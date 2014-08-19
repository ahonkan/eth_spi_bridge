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
*        ip6_mib.c                                   
*
*   COMPONENT
*
*        IPv6 MIBs
*
*   DESCRIPTION
*
*        This file contains the functions that are responsible for
*        maintaining statistics for IPv6.
*
*   DATA STRUCTURES
*
*        IP6_MIB_If_Table_Last_Change
*        IPv6_MIBs
*
*   FUNCTIONS
*
*        IP6_MIB_Initialize
*        IP6_MIB_Init
*
*   DEPENDENCIES
*
*        nu_net.h
*        prefix6.h
*        ip6_mib.h
*        snmp_api.h
*        sys.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/prefix6.h"
#include "networking/ip6_mib.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#include "networking/sys.h"
#endif

#if ( (INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IPV6_MIB == NU_TRUE) )
#ifdef SNMP_2_3
#include "networking/2465gen.h"
#include "networking/2465if.h"
#include "networking/2465addr_pre.h"
#include "networking/2465if_addr.h"
#include "networking/2465rt.h"
#include "networking/2465ntm.h"
#include "networking/2466icmp.h"
#include "networking/2452tcp.h"
#include "networking/2454udp.h"
#include "networking/3019mld.h"
#endif
#endif

#if (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE)
UINT32                  IP6_MIB_If_Table_Last_Change;
#endif

#if ( (INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IPV6_MIB == NU_TRUE) )

#ifdef SNMP_2_3
mib_element_t IPv6_MIBs[] =
{
#include "networking/ip6_mib_oid_s.h"
};
#endif

/************************************************************************
*
*   FUNCTION
*
*       IP6_MIB_Initialize
*
*   DESCRIPTION
*
*        This function is used to initialize IPv6 MIBs.
*
*   INPUTS
*
*        None.
*
*   OUTPUTS
*
*        NU_SUCCESS             When successful.
*        -1                     When failed.
*
************************************************************************/
STATUS IP6_MIB_Initialize(VOID)
{
    STATUS      status;

    /* In case of SNMP version 2.3 or higher we will add IPv6 MIBs at run
     * time.
     */
#ifdef SNMP_2_3

    /* Register IPv6 MIBs. */
    if (SNMP_Mib_Register(IPv6_MIBs, 
                          sizeof(IPv6_MIBs) / 
                          sizeof(mib_element_t)) == NU_SUCCESS)
    {
        /* Return success code. */
        status = NU_SUCCESS;
    }

    /* If we failed to register IPv6 MIBs return error code. */
    else
    {
        status = -1;
    }

#else
    /* Otherwise SNMP will load IPv6 MIBs by it self. */
    status = NU_SUCCESS;
#endif

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_Initialize */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_Init
*
*   DESCRIPTION
*
*        This function is used to initialize IPv6 MIB data structures.
*
*   INPUTS
*
*        *dev                   Handle to interface device.
*
*   OUTPUTS
*
*        NU_SUCCESS             When successful.
*        NU_NO_MEMORY           When memory allocation failed.
*
************************************************************************/
STATUS IP6_MIB_Init(DV_DEVICE_ENTRY *dev)
{
    /* Status to return success or error code. */
    STATUS          status;

#if (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE)
    /* Allocate memory for MIB data. */
    status = NU_Allocate_Memory(MEM_Cached, 
                                (VOID **)(&(dev->ip6_interface_mib)),
                                sizeof(IP6_MIB_INTERFACE), NU_NO_SUSPEND);

    /* If memory allocated successfully. */
    if (status == NU_SUCCESS)
    {
        /* Clear out the IPv6 MIB data. */
        UTL_Zero(dev->ip6_interface_mib, sizeof(IP6_MIB_INTERFACE));

#if (INCLUDE_SNMP == NU_TRUE)
        /* Update ifTableLastChange Time stamp. */
        IP6_MIB_If_Table_Last_Change = SysTime();
#endif
    }

    /* If memory allocation failed. */
    else
        NLOG_Error_Log("Failed to allocate memory", NERR_SEVERE,
                        __FILE__, __LINE__);

#else
    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(dev);

    /* Returning success code. */
    status = NU_SUCCESS;

#endif /* (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE) */

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_Init */

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IPV6_MIB == NU_TRUE)) */
