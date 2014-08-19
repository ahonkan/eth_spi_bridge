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
*        ip6_mib_rt.c                                
*
*   COMPONENT
*
*        IPv6 - Route Table MIB.
*
*   DESCRIPTION
*
*        This file contains the functions that are responsible for
*        maintaining route table MIBs.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        IP6_MIB_RT_Get_Number
*        IP6_MIB_RT_Get
*        IP6_MIB_RT_Get_Next
*        IP6_MIB_RT_Get_If_Index
*        IP6_MIB_RT_Get_Next_Hop
*        IP6_MIB_RT_Get_Type
*        IP6_MIB_RT_Get_Protocol
*        IP6_MIB_RT_Get_Policy
*        IP6_MIB_RT_Get_Age
*        IP6_MIB_RT_Get_Next_Hop_RDI
*        IP6_MIB_RT_Get_Metric
*        IP6_MIB_RT_Get_Weight
*        IP6_MIB_RT_Get_Info
*        IP6_MIB_RT_Get_Valid
*        IP6_MIB_RT_Set_Valid
*
*   DEPENDENCIES
*
*        nu_net.h
*        ip6_mib.h
*        nc6.h
*        snmp_api.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/ip6_mib.h"
#include "networking/nc6.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#endif

#if (INCLUDE_IPV6_RT == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_Number
*
*   DESCRIPTION
*
*        This function is used to get the value current ipv6RouteTable
*        entries. This is primarily to avoid having to read the table
*        in order to determine this number.
*
*   INPUTS
*
*        *rt_entry_count        Pointer to memory where route entry count
*                               need to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_Number(UINT32 *rt_entry_count)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* IPv6 address to get the first routing entry. */
    UINT8               addr[IP6_ADDR_LEN];

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Clear the address. */
        UTL_Zero(addr, IP6_ADDR_LEN);

        /* Get the handle to first route entry. */
        rt_entry = RTAB6_Find_Next_Route(addr);

        /* Loop to traverse and count all the routing entries. */
        while (rt_entry)
        {
            /* Increment the counter. */
            (*rt_entry_count)++;

            /* Get the handle to the next routing entry. */
            rt_entry = 
                RTAB6_Find_Next_Route(rt_entry->rt_route_node->rt_ip_addr);
        }

        /* Return success code. */
        status = IP6_MIB_SUCCESS;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        /* Log the error. */
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return the success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_Number */

/************************************************************************
*
* FUNCTION
*
*        IP6_MIB_RT_Get
*
* DESCRIPTION
*
*        This function is used to get handle to the IPv6 route entry.
*
* INPUTS
*
*        *addr              IPv6 destination address.
*        pfx_len            Prefix length.
*        route_index        Route index.
*
* OUTPUTS
*
*        RTAB6_ROUTE_ENTRY*     If route entry was found.
*        NU_NULL                If route entry was not found.
*
************************************************************************/
STATIC RTAB6_ROUTE_ENTRY *IP6_MIB_RT_Get(UINT8 *addr, UINT32 pfx_len,
                                         UINT32 route_index)
{
    RTAB6_ROUTE_ENTRY           *rt_entry;

    rt_entry = RTAB6_Find_Route(addr, (RT_HOST_MATCH | RT_BEST_METRIC | 
                                       RT_OVERRIDE_METRIC));

    if (rt_entry)
        RTAB_Free((ROUTE_ENTRY*)rt_entry, NU_FAMILY_IP6);

    /* If prefix length and route index don't have any problem. */
    if ( (rt_entry) &&
         ((route_index != 1) ||
          (rt_entry->rt_route_node->rt_submask_length != ((UINT8)pfx_len))))
    {
         rt_entry = NU_NULL;
    }

    /* Return handle to the route entry if found. Otherwise return
     * NU_NULL.
     */
    return (rt_entry);

} /* IP6_MIB_RT_Get */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_Next
*
*   DESCRIPTION
*
*        This function is used to get the indexes of next route entry.
*
*   INPUTS
*
*        *addr                  IPv6 destination address.
*        *pfx_len               Prefix length.
*        *route_index           Route index.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There is no next entry.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_Next(UINT8 *addr, UINT32 *pfx_len, 
                           UINT32 *route_index)
{
    /* Handle to route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status = IP6_MIB_NOSUCHOBJECT;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the route entry. */
        rt_entry = RTAB6_Find_Route(addr, (RT_HOST_MATCH | 
                                           RT_BEST_METRIC | 
                                           RT_OVERRIDE_METRIC));

        /* If we got the handle to the route entry. */
        if (rt_entry)
        {
            /* Free the route entry. */
            RTAB_Free((ROUTE_ENTRY*)rt_entry, NU_FAMILY_IP6);

            /* If the route entry is the next route entry. */
            if ( ((*pfx_len) >
                  (UINT32)(rt_entry->rt_route_node->rt_submask_length)) ||
                 (((*pfx_len) ==
                  (UINT32)(rt_entry->rt_route_node->rt_submask_length)) &&
                  (*route_index) < 1) )
            {
                /* Update the prefix length passed in. */
                (*pfx_len) = rt_entry->rt_route_node->rt_submask_length;

                /* Update the route index. */
                (*route_index) = 1;

                /* Return success code. */
                status = IP6_MIB_SUCCESS;
            }
        }

        /* If we have not got the next route entry. */
        if (status != IP6_MIB_SUCCESS)
        {
            /* Get the handle to the next routing entry. */
            rt_entry = RTAB6_Find_Next_Route(addr);

            /* If we got the handle to the next entry. */
            if (rt_entry)
            {
                /* Update the target address passed in. */
                NU_BLOCK_COPY(addr, rt_entry->rt_route_node->rt_ip_addr,
                              IP6_ADDR_LEN);

                /* Updating prefix address passed in. */
                *pfx_len = rt_entry->rt_route_node->rt_submask_length;

                /* Since we are not using the route index, so set it to 1.
                 */
                (*route_index) = 1;

                /* Return success code. */
                status = IP6_MIB_SUCCESS;
            }
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                            __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore then return error code. */
    else
    {
        /* Logging error. */
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                        __FILE__, __LINE__);

        /* Returning error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_Next */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_If_Index
*
*   DESCRIPTION
*
*        This function is used to get the value interface index of a
*        specified route.
*
*   INPUTS
*
*        *addr                  Destination IPv6 address.
*        pfx_len                Prefix length.
*        rt_index               Route index.
*        *if_index              Pointer to memory where interface index
*                               is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist route entry with
*                               indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_If_Index(UINT8 *addr, UINT32 pfx_len,
                               UINT32 rt_index, UINT32 *if_index)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the route entry. */
        rt_entry = IP6_MIB_RT_Get(addr, pfx_len, rt_index);

        /* If we got the handle to the route entry. */
        if (rt_entry)
        {
            /* If there exists an interface device with the route entry.
             */
            if ( (rt_entry->rt_next_hop_entry) &&
                 (rt_entry->rt_next_hop_entry->ip6_neigh_cache_device) )
            {
                /* Get the value of route interface index. */
                (*if_index) = rt_entry->rt_next_hop_entry->
                              ip6_neigh_cache_device->dev_index + 1;
            }

            /* If there is no associated device with the route entry. */
            else
            {
                /* Set interface index to 'zero(0)'. */
                (*if_index) = 0;
            }

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the route entry. */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_If_Index */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_Next_Hop
*
*   DESCRIPTION
*
*        This function is used to get the address of next hop of a
*        specified route.
*
*   INPUTS
*
*        *addr                  Destination IPv6 address.
*        pfx_len                Prefix length.
*        rt_index               Route index.
*        *next_hop              Pointer to memory where address of next
*                               hop is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exists route entry with
*                               indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_Next_Hop(UINT8 *addr, UINT32 pfx_len,
                               UINT32 rt_index, UINT8 *next_hop)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the route entry. */
        rt_entry = IP6_MIB_RT_Get(addr, pfx_len, rt_index);

        /* If we got the handle to the route entry. */
        if (rt_entry)
        {
            /* Copy the address of next hop. */
            NU_BLOCK_COPY(next_hop, rt_entry->rt_next_hop.sck_addr,
                          IP6_ADDR_LEN);

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the route entry. */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_Next_Hop */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_Type
*
*   DESCRIPTION
*
*        This function is used to get the value of Route type of a
*        specified route.
*
*   INPUTS
*
*        *addr                  Destination IPv6 address.
*        pfx_len                Prefix length.
*        rt_index               Route index.
*        *type                  Pointer to memory where route is to be
*                               stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist route entry with
*                               indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_Type(UINT8 *addr, UINT32 pfx_len,
                           UINT32 rt_index, UINT32 *type)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Grab the handle the route entry. */
        rt_entry = IP6_MIB_RT_Get(addr, pfx_len, rt_index);

        /* If we got the handle to the route entry. */
        if (rt_entry)
        {
            /* Getting value of 'route type'. */
            if (rt_entry->rt_entry_parms.rt_parm_flags & RT_GATEWAY)
                (*type) = IP6_MIB_RT_TYPE_REMOTE;

            else if (rt_entry->rt_entry_parms.rt_parm_flags & RT_HOST)
                (*type) = IP6_MIB_RT_TYPE_LOCAL;

            else
                /* Set to type Other. */
                (*type) = IP6_MIB_RT_TYPE_OTHER;

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the route entry. */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_Type */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_Protocol
*
*   DESCRIPTION
*
*        This function is used to get the value of routing protocol.
*
*   INPUTS
*
*        *addr                  Destination IPv6 address.
*        pfx_len                Prefix length.
*        rt_index               Route index.
*        *protocol              Pointer to memory where value of route
*                               protocol is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exists route entry with
*                               indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_Protocol(UINT8 *addr, UINT32 pfx_len,
                               UINT32 rt_index, UINT32 *protocol)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Grab the handle the route entry. */
        rt_entry = IP6_MIB_RT_Get(addr, pfx_len, rt_index);

        /* If we got the handle to the route entry. */
        if (rt_entry)
        {
            /* Getting the value of protocol. */
            if (rt_entry->rt_entry_parms.rt_parm_flags & RT_RIP2)
                (*protocol) = IP6_MIB_RT_PROT_RIP;

            else if (rt_entry->rt_entry_parms.rt_parm_flags & RT_NETMGMT)
                (*protocol) = IP6_MIB_RT_PROT_NETMGMT;

            else if (rt_entry->rt_entry_parms.rt_parm_flags & RT_ICMP)
                (*protocol) = IP6_MIB_RT_PROT_OTHER;

            else
                (*protocol) = IP6_MIB_RT_PROT_LOCAL;

            /* Returning success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the route entry. */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
        
        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_Protocol */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_Policy
*
*   DESCRIPTION
*
*        This function is used to get the value of route policy.
*
*   INPUTS
*
*        *addr                  Destination IPv6 address.
*        pfx_len                Prefix length.
*        rt_index               Route index.
*        *policy                Pointer to memory where value of route
*                               policy is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist route entry with
*                               indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_Policy(UINT8 *addr, UINT32 pfx_len,
                             UINT32 rt_index, UINT32 *policy)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to route entry. */
        rt_entry = IP6_MIB_RT_Get(addr, pfx_len, rt_index);

        /* If we got the handle to the route entry. */
        if (rt_entry)
        {
            /* Get the value of policy. */
            (*policy) = IP6_TCLASS_DEFAULT;

            /* Return error code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we didn't get the handle to the route entry. */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_Policy */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_Age
*
*   DESCRIPTION
*
*        This function is used to get the value route age of the specified
*        route.
*
*   INPUTS
*
*        *addr                  Destination IPv6 address.
*        pfx_len                Prefix length.
*        rt_index               Route index.
*        *rt_age                Pointer to memory where value of route
*                               age is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exists route entry with
*                               indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_Age(UINT8 *addr, UINT32 pfx_len, UINT32 rt_index,
                          UINT32 *rt_age)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the route entry. */
        rt_entry = IP6_MIB_RT_Get(addr, pfx_len, rt_index);

        /* If we got the handle to the route entry. */
        if (rt_entry)
        {
            /* Get the value of route age. */
            (*rt_age) = 
                (NU_Retrieve_Clock() - 
                 rt_entry->rt_entry_parms.rt_parm_clock) / 
                 SCK_Ticks_Per_Second;

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the route entry. */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_Age */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_Next_Hop_RDI
*
*   DESCRIPTION
*
*        This function is used to get the value of routing domain ID of
*        next hop.
*
*   INPUTS
*
*        *addr                  Destination IPv6 address.
*        pfx_len                Prefix length.
*        rt_index               Route index.
*        *next_hop_rdi          Pointer to memory where value of routing
*                               domain ID of next hop is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist route entry with
*                               indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_Next_Hop_RDI(UINT8 *addr, UINT32 pfx_len,
                                   UINT32 rt_index, UINT32 *next_hop_rdi)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the route entry. */
        rt_entry = IP6_MIB_RT_Get(addr, pfx_len, rt_index);

        /* If we got the handle to the route entry. */
        if (rt_entry)
        {
            /* When this object is unknown or not relevant its value
             * should be set to zero.
             */
            (*next_hop_rdi) = 0;

            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to route entry. */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_Next_Hop_RDI */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_Metric
*
*   DESCRIPTION
*
*        This function is used to get the value of route metric for the
*        route specified.
*
*   INPUTS
*
*        *addr                  Destination IPv6 address.
*        pfx_len                Prefix length.
*        rt_index               Route index.
*        *metric                Pointer to memory where value of route
*                               metric is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exists route entry with
*                               indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_Metric(UINT8 *addr, UINT32 pfx_len, 
                             UINT32 rt_index, UINT32 *metric)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the route entry. */
        rt_entry = IP6_MIB_RT_Get(addr, pfx_len, rt_index);

        /* If we got the handle to route entry. */
        if (rt_entry)
        {
            /* Get the value of the route metric. */
            (*metric) = rt_entry->rt_entry_parms.rt_parm_metric;

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we didn't get the handle to the route entry. */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return the error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_Metric */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_Weight
*
*   DESCRIPTION
*
*        This function is used to get the value of route weight for a
*        specific route.
*
*   INPUTS
*
*        *addr                  Destination IPv6 address.
*        pfx_len                Prefix length.
*        rt_index               Route index.
*        *weight                Pointer to memory where value of route
*                               metric is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exists route entry with
*                               indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_Weight(UINT8 *addr, UINT32 pfx_len,
                             UINT32 rt_index, UINT32 *weight)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Getting handle to the route entry. */
        rt_entry = IP6_MIB_RT_Get(addr, pfx_len, rt_index);

        /* If we got the handle to the route entry. */
        if (rt_entry)
        {
            /* Getting route weight value. */
            (*weight) = rt_entry->rt_entry_parms.rt_parm_metric;
            
            /* Returning success code. */
            status = IP6_MIB_ERROR;
        }

        /* If we didn't get the handle to the route entry then return
         * error code.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_Weight */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_Info
*
*   DESCRIPTION
*
*        This function is used to get the OID of the routing protocol
*        specific.
*
*   INPUTS
*
*        *addr                  Destination IPv6 address.
*        pfx_len                Prefix length.
*        rt_index               Route index.
*        *oid                   Pointer to memory where value of OID
*                               of routing protocol is to be stored.
*        *oid_len               Pointer to the memory location where
*                               length of OID is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exists route entry with
*                               indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_Info(UINT8 *addr, UINT32 pfx_len,
                           UINT32 rt_index, UINT32 *oid, UINT32 *oid_len)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the route entry. */
        rt_entry = IP6_MIB_RT_Get(addr, pfx_len, rt_index);

        /* If we got the handle to the route entry. */
        if (rt_entry)
        {
            /* Set OID to default values that NULL OID. */
            oid[0] = 0;
            oid[1] = 0;
            (*oid_len) = (UINT32)2;

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the route entry. */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_Info */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Get_Valid
*
*   DESCRIPTION
*
*        This function is used to get the value of 'route valid status'
*        value of '1' represent that route is valid and value of '2'
*        represent that route is invalid.
*
*   INPUTS
*
*        *addr                  Destination IPv6 address.
*        pfx_len                Prefix length.
*        rt_index               Route index.
*        *rt_valid              Pointer to the memory location where
*                               value of route valid status is to be
*                               copied.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exists route entry with
*                               indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Get_Valid(UINT8 *addr, UINT32 pfx_len,
                            UINT32 rt_index, UINT32 *rt_valid)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the route entry. */
        rt_entry = IP6_MIB_RT_Get(addr, pfx_len, rt_index);

        /* If we got the handle to the route entry. */
        if (rt_entry)
        {
            /* If next hope entry exists. */
            if (rt_entry->rt_next_hop_entry)
            {
                /* Getting the value of 'route valid'. */
                if ( (NU_Retrieve_Clock() > 
                      rt_entry->rt_entry_parms.rt_parm_clock) &&
                     (rt_entry->rt_next_hop_entry->
                      ip6_neigh_cache_flags & NC_UP) )
                    (*rt_valid) = 1;
                else
                    (*rt_valid) = 2;
            }

            /* If next hop entry does not exist then route entry is
             * invalid.
             */
            else
            {
                (*rt_valid) = 2;
            }

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the route entry. */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Get_Valid */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_RT_Set_Valid
*
*   DESCRIPTION
*
*        This is used to set the value of 'route valid status'. Value '1'
*        represent that valid route while value '2' represent the invalid.
*
*   INPUTS
*
*        *addr                  Destination IPv6 address.
*        pfx_len                Prefix length.
*        rt_index               Route index.
*        rt_valid               value of route valid status is to be
*                               set.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_WRONGVALUE     route valid status value is invalid.
*        IP6_MIB_NOSUCHOBJECT   There does not exists route entry with
*                               indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_RT_Set_Valid(UINT8 *addr, UINT32 pfx_len, UINT32 rt_index,
                            UINT32 rt_valid)
{
    /* Handle to the route entry. */
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* If we have a valid value to set. */
    if ( (rt_valid == IP6_MIB_TRUE) || (rt_valid == IP6_MIB_FALSE) )
    {
        /* Grab the semaphore. */
        if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
        {
            /* Get the handle to the route entry. */
            rt_entry = IP6_MIB_RT_Get(addr, pfx_len, rt_index);

            /* If we got the handle to the route entry. */
            if (rt_entry)
            {
                /* If we are going to validate the route entry and that
                 * we have the next hop entry for the route.
                 */
                if ( (rt_valid == IP6_MIB_TRUE) &&
                     (rt_entry->rt_next_hop_entry) )
                {
                    /* Set route as up. */
                    rt_entry->rt_next_hop_entry->ip6_neigh_cache_flags |=
                        NC_UP;

                    /* Return success code. */
                    status = IP6_MIB_SUCCESS;
                }

                /* If 'route valid' is to set to invalid. */
                else if (rt_valid == IP6_MIB_FALSE)
                {
                    /* If have the next hop entry. */
                    if (rt_entry->rt_next_hop_entry)
                    {
                        /* Set route as 'down'. */
                        rt_entry->rt_next_hop_entry->ip6_neigh_cache_flags &= 
                            (~NC_UP);
                    }

                    /* Return success code. */
                    status = IP6_MIB_SUCCESS;
                }

                else
                    status = IP6_MIB_ERROR;
            }

            /* If we did not get the handle to the route entry. */
            else
            {
                /* Return error code. */
                status = IP6_MIB_NOSUCHOBJECT;
            }

            /* Release the semaphore. */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release the semaphore", 
                               NERR_SEVERE, __FILE__, __LINE__);
        }

        else
        {
            NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

            /* Return error code. */
            status = IP6_MIB_ERROR;
        }
    }

    /* If we don't have valid value to set then return error code. */
    else
        status = IP6_MIB_WRONGVALUE;

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_RT_Set_Valid */

#endif /* (INCLUDE_IPV6_RT == NU_TRUE) */
