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
*        ip6_mib_mld.c                               
*
*   COMPONENT
*
*        IPv6 - MLD MIBs.
*
*   DESCRIPTION
*
*        This file contains the functions that are responsible for
*        providing an interface to functions that are handling SNMP
*        requests on MLD Group.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        IP6_MLD_MIB_Get_Device
*        IP6_MLD_MIB_Get_Next_Index
*        IP6_MLD_MIB_Get_If_Status
*        IP6_MLD_MIB_Get_If_Qrier_Addr
*        IP6_MLD_MIB_Get_Cache
*        IP6_MLD_MIB_Get_Next_Cache
*        IP6_MLD_MIB_Get_Cache_Self
*        IP6_MLD_MIB_Get_Cache_Status
*
*   DEPENDENCIES
*
*        nu_net.h
*        externs6.h
*        ip6_mib.h
*        mld6.h
*        snmp_api.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/externs6.h"
#include "networking/ip6_mib.h"
#include "networking/mld6.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#endif

#if (INCLUDE_IPV6_MLD_MIB == NU_TRUE)
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

STATIC DV_DEVICE_ENTRY *IP6_MLD_MIB_Get_Device(UINT32 index);
STATIC IP6_MULTI *IP6_MLD_MIB_Get_Cache(UINT8 *addr, UINT32 if_index,
                                        DV_DEVICE_ENTRY **dev);

/************************************************************************
*
*   FUNCTION
*
*        IP6_MLD_MIB_Get_Device
*
*   DESCRIPTION
*
*        This function is used to get the handle of MLD enabled device.
*
*   INPUTS
*
*        index                  Interface index of the device.
*
*   OUTPUTS
*
*        DV_DEVICE_ENTRY*       Handle to the device entry if there 
*                               exists an MLD enabled with the interface 
*                               index passed in.
*        NU_NULL                If there does not exists an MLD enabled
*                               with the interface index passed in.
*
************************************************************************/
STATIC DV_DEVICE_ENTRY *IP6_MLD_MIB_Get_Device(UINT32 index)
{
    /* Handle to the device interface. */
    DV_DEVICE_ENTRY     *dev;

    /* If we have valid SNMP interface index. */
    if (index > 0)
    {
        /* Get handle to the interface device. */
        dev = DEV_Get_Dev_By_Index(index - 1);

        /* If device is not IPv6 enabled device or the device is virtual
         * or PPP device then this device is not MLD enabled device. So
         * return NU_NULL in this case.
         */
        if ( (dev) &&
             ((!(dev->dev_flags & DV6_IPV6)) ||
              (dev->dev_flags & (DV6_VIRTUAL_DEV | DV_POINTTOPOINT)) ) )
            dev = NU_NULL;
    }

    /* If we have invalid interface index then return NU_NULL. */
    else
        dev = NU_NULL;

    /* Return handle to the interface device if we found an MLD enabled
     * device that have same interface index as passed in. Otherwise
     * return NU_NULL.
     */
    return (dev);

} /* IP6_MLD_MIB_Get_Device */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MLD_MIB_Get_Next_Index
*
*   DESCRIPTION
*
*        This function is used to get the interface index of next MLD
*        enabled interface device.
*
*   INPUTS
*
*        *index                 Pointer to memory location where 
*                               interface index.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   When there does not exist the next MLD
*                               enabled interface device.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MLD_MIB_Get_Next_Index(UINT32 *index)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get handle to the first interface to start traversing the
         * device list.
         */
        dev = DEV_Table.dv_head;

        /* Loop to traverse device table. */
        while (dev)
        {
            /* If we have reached the required interface device then break
             * the loop and return the handle to the current interface
             * device. A required interface device is one that has
             * greater interface index than index passed in and is MLD
             * enabled device. An MLD enabled device is the one that is
             * IPv6 enabled and is neither a virtual not a PPP device.
             */
            if ( ((dev->dev_index + 1) > (*index)) &&
                 (dev->dev_flags & DV6_IPV6) &&
                 (!(dev->dev_flags & (DV6_VIRTUAL_DEV | DV_POINTTOPOINT))) )
                break;

            /* Moving forward in the device table. */
            dev = dev->dev_next;
        }

        /* If we got the handle to the device then update the index
         * passed in and return success code.
         */
        if (dev)
        {
            /* Update the interface index passed in. */
            (*index) = dev->dev_index + 1;

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the device then return error
         * code.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Release the semaphore. */
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

    /* Return success or error code. */
    return (status);

} /* IP6_MLD_MIB_Get_Next_Index */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MLD_MIB_Get_If_Status
*
*   DESCRIPTION
*
*        This function is used to get value of status of MLD enabled
*        interface.
*
*   INPUTS
*
*        if_index               Interface index.
*        *if_status             Pointer to the memory location where 
*                               value of interface status is to be 
*                               copied.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   We did not get the handle to MLD enabled
*                               device using the interface index passed
*                               in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MLD_MIB_Get_If_Status(UINT32 if_index, UINT32 *if_status)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the interface device. */
        dev = IP6_MLD_MIB_Get_Device(if_index);

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource)!= NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* If we found the MLD enabled device using the interface index
         * passed in.
         */
        if (dev)
        {
            /* Set the row status to 'active'. */
            (*if_status) = IP6_MIB_MLD_ROW_ACTIVE;

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the MLD enabled interface
         * device then return error code. */
        else
            status = IP6_MIB_NOSUCHOBJECT;
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

} /* IP6_MLD_MIB_Get_If_Status */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MLD_MIB_Get_If_Qrier_Addr
*
*   DESCRIPTION
*
*        This function is used to get the querier IPv6 address of MLD
*        enabled device specified by interface index passed in.
*
*   INPUTS
*
*        if_index               Interface index.
*        addr                   Pointer to the location where memory
*                               address where IPv6 is to be copied.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There is no MLD interface with interface
*                               index passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MLD_MIB_Get_If_Qrier_Addr(UINT32 if_index, UINT8 *addr)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Variable to hold the device name. */
    CHAR                dev_name[DEV_NAME_LENGTH];

    SCK_IOCTL_OPTION    opt;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the MLD enabled interface device using the
         * interface index passed in.
         */
        dev = IP6_MLD_MIB_Get_Device(if_index);

        /* If found the MLD enabled device using the interface index
         * passed in then copy device name in local variable. */
        if (dev)
            NU_BLOCK_COPY(dev_name, dev->dev_net_if_name, DEV_NAME_LENGTH);

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* If we got the handle to the interface device then get the IPv6
         * address.
         */
        if (dev)
        {
            /* Make IOCTL request. */
            opt.s_optval = (UINT8 *)(&(dev_name[0]));

            /* Get the IPv6 address by making the IOCTL request. */
            if (NU_Ioctl_SIOCGIFADDR_IN6(&opt, IP6_ADDR_LEN) == NU_SUCCESS)
            {
                /* Copy IPv6 address. */
                NU_BLOCK_COPY(addr, opt.s_ret.s_ipaddr, IP6_ADDR_LEN);

                /* Return success code. */
                status = IP6_MIB_SUCCESS;
            }

            /* If NU_Ioctl_SIOCGIFADDR_IN6 failed then return error code.
             */
            else
                status = IP6_MIB_ERROR;
        }

        /* If we did not get the handle to the MLD enabled device then
         * return error code.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;
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

} /* IP6_MLD_MIB_Get_If_Qrier_Addr */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MLD_MIB_Get_Cache
*
*   DESCRIPTION
*
*        This function is used to get the handle to the MLD cache entry.
*
*   INPUTS
*
*        *addr                  Pointer to the memory where IPv6 address
*                               is stored.
*        if_index               Interface index.
*        **dev                  Pointer to memory location where handle to
*                               the interface device is to be stored.
*
*   OUTPUTS
*
*        IP6_MULTI*             Pointer to the MLD Cache Entry if there
*                               exists one with the interface index and
*                               IPv6 address passed in.
*        NU_NULL                If there does not exists an MLD Cache
*                               entry with indexes passed in.
*
************************************************************************/
STATIC IP6_MULTI *IP6_MLD_MIB_Get_Cache(UINT8 *addr, UINT32 if_index,
                                        DV_DEVICE_ENTRY **dev)
{
    /* Handle to the MLD cache entry. */
    IP6_MULTI           *mult;

    /* Get the handle to the interface device. */
    (*dev) = IP6_MLD_MIB_Get_Device(if_index);

    /* If we have found the handle to the interface device then get the
     * handle to the MLD cache entry.
     */
    if (*dev)
        mult = IP6_Lookup_Multi(addr, (*dev)->dev6_multiaddrs);

    /* We did not get the handle to the interface device then return
     * NU_NULL.
     */
    else
        mult = NU_NULL;

    /* Return handle to the MLD cache entry if we have found one using
     * interface index and IPv6 address.
     */
    return (mult);

} /* IP6_MLD_MIB_Get_Cache */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MLD_MIB_Get_Next_Cache
*
*   DESCRIPTION
*
*        This function is used to get the value of next indexes for the
*        MLD cache entry.
*
*   INPUTS
*
*        *addr                  Pointer to the memory location where IPv6
*                               address of MLD cache entry is stored.
*        *if_index              Pointer to the memory location where
*                               interface index is stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   When there is no next MLD Cache entry.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MLD_MIB_Get_Next_Cache(UINT8 *addr, UINT32 *if_index)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Handle to the MLD Cache entry. */
    IP6_MULTI           *mult;

    /* Variable to hold the current candidate for the interface index of
     * next MLD cache entry.
     */
    UINT32              curr_if_index;

    /* Variable to hold the current candidate for the IPv6 address of the
     * next MLD cache entry.
     */
    UINT8               curr_addr[IP6_ADDR_LEN];

    /* Variable to hold the memcmp return value. */
    INT                 cmp_res;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Set candidate's interface index to represent invalid value that
         * also a greatest value that an UINT32 can have.
         */
        curr_if_index = 0xFFFFFFFFul;

        /* Set candidate's address invalid value that is greatest value an
         * IPv6 address can have.
         */
        memset(curr_addr, (~0), sizeof(curr_addr));

        /* Get the handle to the first interface device to start
         * traversing interface device list.
         */
        dev = DEV_Table.dv_head;

        /* Traverse the interface device list. */
        while (dev)
        {
            /* If interface device is MLD enabled. */
            if ( (dev->dev_flags & DV6_IPV6) &&
                 (!(dev->dev_flags & (DV6_VIRTUAL_DEV | DV_POINTTOPOINT))) )
            {
                /* Get the handle to the first MLD cache entry to start
                 * traversing the list.
                 */
                mult = dev->dev6_multiaddrs;

                /* Traversing the MLD cache entry list. */
                while (mult)
                {
                    /* Set status to represent that current MLD cache
                     * entry is not a better candidate for being the next
                     * MLD cache entry.
                     */
                    status = IP6_MIB_ERROR;

                    /* Compare address passed in with the IPv6 address of
                     * the cache entry.
                     */
                    cmp_res = memcmp(addr, mult->ipm6_addr, IP6_ADDR_LEN);

                    /* If current cache entry is a candidate of being next
                     * entry.
                     */
                    if (cmp_res < 0)
                    {
                        /* Compare current MLD cache entry's address with
                         * the current candidate address.
                         */
                        cmp_res = memcmp(curr_addr, mult->ipm6_addr,
                                         IP6_ADDR_LEN);

                        /* If current MLD cache entry is better candidate
                         * then current candidate then set status to
                         * represent that this situation.
                         */
                        if ( (cmp_res > 0) ||
                             ((cmp_res == 0) && 
                              (dev->dev_index + 1) < curr_if_index) )
                            status = IP6_MIB_SUCCESS;
                    }

                    /* Current candidate's address is same as that passed
                     * and is better candidate then current candidate then
                     * status to represent this situation.
                     */
                    else if ( (cmp_res == 0) &&
                              ((dev->dev_index + 1) > (*if_index)) &&
                              ((dev->dev_index + 1) > curr_if_index) )
                        status = IP6_MIB_SUCCESS;

                    /* If current candidate is better candidate is better
                     * candidate then current candidate then update
                     * current candidate.
                     */
                    if (status == IP6_MIB_SUCCESS)
                    {
                        /* Update the interface index of current
                         * candidate.
                         */
                        curr_if_index = dev->dev_index + 1;

                        /* Update the address of the current candidate. */
                        NU_BLOCK_COPY(curr_addr, mult->ipm6_addr,
                                      IP6_ADDR_LEN);
                    }

                    /* Moving forward in the MLD Cache entry list. */
                    mult = mult->ipm6_next;
                }
            }
            /* Moving forward in the device list. */
            dev = dev->dev_next;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* If we have got indexes to next entry then update the indexes
         * passed in and return success code.
         */
        if (curr_if_index != 0xFFFFFFFFul)
        {
            /* Update the interface index passed in. */
            (*if_index) = curr_if_index;

            /* Update the IPv6 address passed in. */
            NU_BLOCK_COPY(addr, curr_addr, IP6_ADDR_LEN);

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the indexes of the next entry then return
         * error code.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;
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

} /* IP6_MLD_MIB_Get_Next_Cache */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MLD_MIB_Get_Cache_Self
*
*   DESCRIPTION
*
*        This function is used to determine whether the local system is a
*        member of this group address on this interface.
*
*   INPUTS
*
*        *addr                  Pointer to the memory where IPv6 address
*                               is stored.
*        if_index               Interface index.
*        *is_cache_self         Pointer to memory location where boolean
*                               result is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   When there is no MLD Cache entry
*                               corresponding to indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MLD_MIB_Get_Cache_Self(UINT8 *addr, UINT32 if_index,
                                  UINT32 *is_cache_self)
{
    /* Handle to the MLD cache entry. */
    IP6_MULTI           *mult;

    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the MLD Cache entry. */
        mult = IP6_MLD_MIB_Get_Cache(addr, if_index, &dev);

        /* If we got the handle to the MLD Cache entry. */
        if (mult)
        {
            /* IP6_Lookup_Multi(dev->m_addr) */

            /* Indicate that local system is member of current entry. */
            (*is_cache_self) = IP6_MIB_TRUE; /* Always 'true'. */

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the MLD Cache entry then return
         * error code.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Release the semaphore. */
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

    /* Return success or error code. */
    return (status);

} /* IP6_MLD_MIB_Get_Cache_Self */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MLD_MIB_Get_Cache_Status
*
*   DESCRIPTION
*
*        This function is used to get the status of the cache entry.
*
*   INPUTS
*
*        *addr                  Pointer to the memory where IPv6 address
*                               is stored.
*        if_index               Interface index.
*        *cache_status          Pointer to memory location where status of
*                               cache entry is to be copied.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   When there is no MLD Cache entry
*                               corresponding to indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MLD_MIB_Get_Cache_Status(UINT8 *addr, UINT32 if_index,
                                    UINT32 *cache_status)
{
    /* Handle to the MLD cache entry. */
    IP6_MULTI           *mult;

    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the MLD Cache entry. */
        mult = IP6_MLD_MIB_Get_Cache(addr, if_index, &dev);

        /* If we got the handle to the MLD Cache entry. */
        if (mult)
        {
            /* Send an indication that entry is 'active'. */
            (*cache_status) = IP6_MIB_MLD_ROW_ACTIVE; 

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the MLD Cache entry then return
         * error code.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Release the semaphore. */
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

    /* Return success or error code. */
    return (status);

} /* IP6_MLD_MIB_Get_Cache_Status */

#endif /* (INCLUDE_IP_MULTICASTING == NU_TRUE) */
#endif /* (INCLUDE_IPV6_MLD_MIB == NU_TRUE) */
