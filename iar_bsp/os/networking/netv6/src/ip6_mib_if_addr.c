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
*        ip6_mib_if_addr.c                           
*
*   COMPONENT
*
*        IPv6 Interface Address MIB
*
*   DESCRIPTION
*
*        This file contains the implementation of IPv6 Interface address
*        MIB.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        IP6_IF_ADDR_MIB_Get
*        IP6_IF_ADDR_MIB_Get_Next
*        IP6_IF_ADDR_MIB_Get_Pfx_Length
*        IP6_IF_ADDR_MIB_Get_Type
*        IP6_IF_ADDR_MIB_Get_AnycastFlag
*        IP6_IF_ADDR_MIB_Get_Status
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

#if (INCLUDE_IPV6_IF_ADDR_MIB == NU_TRUE)

STATIC DEV6_IF_ADDRESS *IP6_IF_ADDR_MIB_Get(UINT32, UINT8 *);

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_ADDR_MIB_Get
*
*   DESCRIPTION
*
*        This function is used to get the handle to the interface address
*        entry.
*
*   INPUTS
*
*        if_index               Interface index.
*        *addr                  IPv6 Address.
*
*   OUTPUTS
*
*        DEV6_IF_ADDRESS*       When interface address entry found.
*        NU_NULL                When interface address entry not found.
*
************************************************************************/
STATIC DEV6_IF_ADDRESS *IP6_IF_ADDR_MIB_Get(UINT32 if_index, UINT8 *addr)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY         *dev;

    /* Handle to interface address entry. */
    DEV6_IF_ADDRESS         *if_address = NU_NULL;

    /* If we got the valid interface index. */
    if (if_index)
    {
        /* Get the handle to the interface device. */
        dev = DEV_Get_Dev_By_Index(if_index - 1);

        /* If we got the handle to the interface device. */
        if (dev)
        {
            /* Get the handle to the interface address entry. */
            if_address = DEV6_Find_Target_Address(dev, addr);
        }
    }

    /* Returning handle to the interface address entry if found otherwise
     * returning NU_NULL.
     */
    return (if_address);

} /* IP6_IF_ADDR_MIB_Get */

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_ADDR_MIB_Get_Next
*
*   DESCRIPTION
*
*        This function is used to get the indexes of next interface
*        address entry.
*
*   INPUTS
*
*        *if_index              Interface index.
*        *addr                  IPv6 address.
*
*   OUTPUTS
*
*        IP6_MIB_NOSUCHOBJECT   There is not next interface address entry.
*        IP6_MIB_SUCCESS        Next entry found and index updated
*                               successfully.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_IF_ADDR_MIB_Get_Next(UINT32 *if_index, UINT8 *addr)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Handle to the interface address entry. */
    DEV6_IF_ADDRESS     *if_addr;

    /* Comparison result. */
    INT                 cmp_result;

    /* Candidate address. */
    UINT8               cand_addr[IP6_ADDR_LEN];

    /* Candidate interface index. */
    UINT32              cand_if_index = 0;

    /* Status to return success or error code. */
    UINT16              status = IP6_MIB_NOSUCHOBJECT;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Getting handle to first interface device. */
        dev = DEV_Table.dv_head;

        /* Loop through the interface device list. */
        while ( (dev) && (status != IP6_MIB_SUCCESS) )
        {
            /* If we have reached at interface device with greater or
             * equal interface index.
             */
            if ( (dev->dev_flags & DV6_IPV6) &&
                 ((dev->dev_index + 1) >= (*if_index)) )
            {
                /* Getting starting handle to the interface address list.
                 */
                if_addr = dev->dev6_addr_list.dv_head;

                /* Loop through the interface address list. */
                while (if_addr)
                {
                    /* If we currently have an handle to interface device
                     * with equal interface index then compare address
                     * passed in with the address in interface address
                     * entry. Otherwise just mark greater without
                     * making comparison because entry is greater in
                     * respect of interface index.
                     */
                    if ((dev->dev_index + 1) == (*if_index))
                    {
                        /* Comparing interface address passed in with
                         * address in interface address entry.
                         */
                        cmp_result = memcmp(if_addr->dev6_ip_addr, addr,
                                            IP6_ADDR_LEN);
                    }
                    else
                    {
                        /* Marking interface address entry as greater. */
                        cmp_result = 1;
                    }

                    /* If interface address entry has greater indexes with
                     * respect what is passed in.
                     */
                    if (cmp_result > 0)
                    {
                        /* If we already have candidate for next entry
                         * then check which one is better candidate.
                         * Otherwise mark this entry as better candidate
                         * because there is no candidate to compare with.
                         */
                        if (status == IP6_MIB_SUCCESS)
                        {
                            /* Comparing candidate address with address in
                             * interface address entry. Lesser will be the
                             * better candidate.
                             */
                            cmp_result = memcmp(if_addr->dev6_ip_addr,
                                                cand_addr, IP6_ADDR_LEN);
                        }
                        else
                        {
                            /* Marking current interface address entry as
                             * better candidate.
                             */
                            cmp_result = -1;

                            /* Setting status to success code to represent
                             * that we currently have a candidate for
                             * being next.
                             */
                            status = IP6_MIB_SUCCESS;
                        }

                        /* If current entry is better candidate then copy
                         * the address in current best candidate. */
                        if (cmp_result < 0)
                        {
                            /* Setting the current entry as current best
                             * candidate for being next.
                             */
                            NU_BLOCK_COPY(cand_addr, if_addr->dev6_ip_addr,
                                          IP6_ADDR_LEN);

                            /* Update candidate interface index. */
                            cand_if_index = dev->dev_index + 1;
                        }
                    }

                    /* Moving forward in the interface address list. */
                    if_addr = if_addr->dev6_next;

                } /* while (if_addr) */
            }

            /* Moving forward in the list. */
            dev = dev->dev_next;

        } /* while ( (dev) && (status != IP6_MIB_SUCCESS) ) */

        /* If we successfully got the next entry then update the indices
         * passed in.
         */
        if (cand_if_index)
        {
            /* Updating interface index. Add 1 because in SNMP interface
             * index start from 1 but in Nucleus SNMP it start from 0.
             */
            (*if_index) = cand_if_index;

            /* Updating interface address. */
            NU_BLOCK_COPY(addr, cand_addr, IP6_ADDR_LEN);
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error log. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_IF_ADDR_MIB_Get_Next */

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_ADDR_MIB_Get_Pfx_Length
*
*   DESCRIPTION
*
*        This function is used to get the value of prefix length of
*        specific interface address entry.
*
*   INPUTS
*
*        if_index               Interface index.
*        *addr                  IPv6 address.
*        *pfx_len               Pointer to the memory location where value
*                               of prefix length is stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   Interface address entry not found.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_IF_ADDR_MIB_Get_Pfx_Length(UINT32 if_index, UINT8 *addr,
                                      UINT32 *pfx_len)
{
    /* Handle to the interface address entry. */
    DEV6_IF_ADDRESS     *if_addr;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the interface address entry. */
        if_addr = IP6_IF_ADDR_MIB_Get(if_index, addr);

        /* If we got the handle to the interface address entry. */
        if (if_addr)
        {
            /* Getting value of Prefix length. */
            (*pfx_len) = (UINT32)(if_addr->dev6_prefix_length);

            /* Returning success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to interface address entry then
         * return error code.
         */
        else
        {
            /* Returning error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore.", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_IF_ADDR_MIB_Get_Pfx_Length */

/*************************************************************************
*
*   FUNCTION
*
*        IP6_IF_ADDR_MIB_Get_Type
*
*   DESCRIPTION
*
*        This function is used to get the value of interface address type.
*
*   INPUTS
*
*        if_index               Interface index.
*        *addr                  IPv6 address.
*        *type                  Pointer to the memory location where value
*                               of type need to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   Interface address entry not found.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_IF_ADDR_MIB_Get_Type(UINT32 if_index, UINT8 *addr, UINT32 *type)
{
    /* Handle to the interface address entry. */
    DEV6_IF_ADDRESS     *if_addr;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Getting handle to the interface address list. */
        if_addr = IP6_IF_ADDR_MIB_Get(if_index, addr);

        /* If we got the handle to the interface address entry. */
        if (if_addr)
        {
            /* Setting value of type to 'unknown(3)'. */
            (*type) = IP6_MIB_IF_ADDR_TYPE_UNKNOWN;

            /* Returning success code. */
            status = IP6_MIB_SUCCESS;
        }
        
        /* If we did not get the handle to the interface address. */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Releasing semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
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

    /* Returning success or error code. */
    return (status);

} /* IP6_IF_ADDR_MIB_Get_Type */

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_ADDR_MIB_Get_AnycastFlag
*
*   DESCRIPTION
*
*        This function is used to get the value of ANYCAST flag for a
*        specific interface address entry.
*
*   INPUTS
*
*        if_index               Interface index.
*        *addr                  IPv6 address.
*        *anycast_flag          Pointer to the memory location where value
*                               of ANYCAST flag is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   Interface address entry not found.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_IF_ADDR_MIB_Get_AnycastFlag(UINT32 if_index, UINT8 *addr,
                                       UINT32 *anycast_flag)
{
    /* Handle to the interface address entry. */
    DEV6_IF_ADDRESS     *if_addr;

    /* Status to return success or error code. */
    UINT16              status;

    /* Gab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Getting handle to the interface address entry. */
        if_addr = IP6_IF_ADDR_MIB_Get(if_index, addr);

        /* If we get the handle to the interface address entry. */
        if (if_addr)
        {
            /* Getting the value of ANYCAST flag . */
            if (if_addr->dev6_addr_state & DV6_ANYCAST)
                (*anycast_flag) = IP6_MIB_TRUE;
            else
                (*anycast_flag) = IP6_MIB_FALSE;

            /* Returning success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the interface address entry. */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore.", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_IF_ADDR_MIB_Get_AnycastFlag */

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_ADDR_MIB_Get_Status
*
*   DESCRIPTION
*
*        This function is used to get the the value of interface address
*        status.
*
*   INPUTS
*
*        if_index               Interface index.
*        *addr                  Interface address.
*        *if_addr_status        Pointer to the memory location where value
*                               of interface address is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   Interface address entry not found.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_IF_ADDR_MIB_Get_Status(UINT32 if_index, UINT8 *addr,
                                  UINT32 *if_addr_status)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Handle to the interface address entry. */
    DEV6_IF_ADDRESS     *if_addr;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Getting the handle to the interface address entry. */
        if_addr = IP6_IF_ADDR_MIB_Get(if_index, addr);

        /* If we got the handle to the interface address entry. */
        if (if_addr)
        {
            /* Get the handle to the interface device. */
            dev = DEV_Get_Dev_By_Index(if_index - 1);

            /* Setting the appropriate value to 'if_addr_status'. */

            /* If address is depreciated. */
            if (if_addr->dev6_addr_state & DV6_DEPRECATED)
                (*if_addr_status) = IP6_MIB_IF_ADDR_STATUS_DEPREC;

            /* If address is inaccessible. */
            else if ( (if_addr->dev6_addr_state & DV6_DETACHED) ||
                      (!(dev->dev_flags & DV_UP)) ||
                      (!(dev->dev_flags2 & DV6_UP)) )
                (*if_addr_status) = IP6_MIB_IF_ADDR_STATUS_INACCES;

            /* If address is preferred. */
            else if (if_addr->dev6_preferred_lifetime <=
                     (NU_Retrieve_Clock() * NU_PLUS_Ticks_Per_Second))
                (*if_addr_status) = IP6_MIB_IF_ADDR_STATUS_PREFER;

            /* Invalid address. */
            else
                (*if_addr_status) = IP6_MIB_IF_ADDR_STATUS_INVALID;

            /* Returning success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we failed to get the handle to the interface address entry.
         */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Returning error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_IF_ADDR_MIB_Get_Status */

#endif /* (INCLUDE_IPV6_IF_ADDR_MIB == NU_TRUE) */
