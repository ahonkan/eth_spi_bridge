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
*        ip6_mib_ntm.c                               
*
*   COMPONENT
*
*        IPv6 - Net To Media MIBs
*
*   DESCRIPTION
*
*        This file contains the functions that are responsible of handling
*        request for Net to Media MIBs.
*
*   DATA STRUCTURES
*
*        IP6_MIB_NTM_Getter_Function
*        IP6_MIB_NTM_Getter_Functions
*
*   FUNCTIONS
*
*        IP6_MIB_NTM_Get_Next_Index
*        IP6_MIB_NTM_Get
*        IP6_MIB_NTM_Get_Entry_Util
*        IP6_MIB_NTM_Get_Phy_Address
*        IP6_MIB_NTM_Get_Media_Type
*        IP6_MIB_NTM_Get_Media_State
*        IP6_MIB_NTM_Get_Last_Updated
*        IP6_MIB_NTM_Get_Validity
*
*   DEPENDENCIES
*
*        nu_net.h
*        nc6.h
*        ip6_mib.h
*        snmp_api.h
*        nc6_eth.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/nc6.h"
#include "networking/ip6_mib.h"
#include "networking/nc6_eth.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#endif

#if (INCLUDE_IPV6_MIB_NTM == NU_TRUE)

STATIC INT IP6_MIB_NTM_Get_Entry_Util(UINT32, UINT8 *, DV_DEVICE_ENTRY **);
STATIC UINT16 IP6_MIB_NTM_Get_Phy_Address(IP6_NEIGHBOR_CACHE_ENTRY *, 
                                          VOID *);
STATIC UINT16 IP6_MIB_NTM_Get_Media_Type(IP6_NEIGHBOR_CACHE_ENTRY *, 
                                         VOID *);
STATIC UINT16 IP6_MIB_NTM_Get_Media_State(IP6_NEIGHBOR_CACHE_ENTRY *, 
                                          VOID *);
STATIC UINT16 IP6_MIB_NTM_Get_Last_Updated(IP6_NEIGHBOR_CACHE_ENTRY *, 
                                           VOID *);
STATIC UINT16 IP6_MIB_NTM_Get_Validity(IP6_NEIGHBOR_CACHE_ENTRY *, 
                                       VOID *);

typedef UINT16 (*IP6_MIB_NTM_Getter_Function)
                    (IP6_NEIGHBOR_CACHE_ENTRY *, VOID *);

#define IP6_MIB_NTM_GETTER_FUNCTION     6

STATIC IP6_MIB_NTM_Getter_Function IP6_MIB_NTM_Getter_Functions
    [IP6_MIB_NTM_GETTER_FUNCTION] = {NU_NULL, IP6_MIB_NTM_Get_Phy_Address,
                                    IP6_MIB_NTM_Get_Media_Type,
                                    IP6_MIB_NTM_Get_Media_State,
                                    IP6_MIB_NTM_Get_Last_Updated,
                                    IP6_MIB_NTM_Get_Validity};

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_NTM_Get_Next_Index
*
*   DESCRIPTION
*
*        This function is used to get the indexes of the next neighbor
*        cache entry.
*
*   INPUTS
*
*        *if_index              Interface index.
*        *ipv6_addr             IPv6 address of the neighbor cache entry.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   When next entry does not exist.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_NTM_Get_Next_Index(UINT32 *if_index, UINT8 *ipv6_addr)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY *dev;

    /* Index of the relevant neighbor cache entry in the list maintained
     * in device structure.
     */
    INT             index;

    /* Variable to hold comparison result. */
    INT             cmp_result;

    /* Interface index of the candidate entry. */
    UINT32          cand_if_index;

    /* IPv6 address of the candidate entry. */
    UINT8           cand_addr[IP6_ADDR_LEN];

    /* Status to return success or error code. */
    UINT16          status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Set candidate interface index to maximum values that can be
         * stored.
         */
        cand_if_index = 0xFFFFFFFFul;

        /* Set candidate address to maximum value that can be stored. */
        memset(cand_addr, (~0), IP6_ADDR_LEN);

        /* Get the handle to the first interface device. */
        dev = DEV_Table.dv_head;

        /* Loop to traverse all device list. */
        while (dev)
        {
            /* If the current device is IPv6 enabled and has the index
             * that is greater/equal to interface index passed in and
             * lesser/equal to current candidate interface index.
             */
            if ( (dev->dev_flags & DV6_IPV6) &&
                 ((dev->dev_index + 1) >= (*if_index)) &&
                 ((dev->dev_index + 1) <= cand_if_index) )
            {
                /* Loop through the neighbor cache list. */
                for (index = 0; index < dev->dev6_nc_entries; index++)
                {
                    /* If current device's index is greater than interface
                     * index passed in then mark the current entry as
                     * candidate.
                     */
                    if ((dev->dev_index + 1) > (*if_index))
                        cmp_result = -1;

                    /* If interface index of current device is same as
                     * passed in and IPv6 address stored in current entry
                     * is greater than what is passed in then mark the
                     * current entry as candidate.
                     */
                    else
                        cmp_result = 
                            memcmp(ipv6_addr, 
                                   dev->dev6_neighbor_cache[index].
                                   ip6_neigh_cache_ip_addr, IP6_ADDR_LEN);

                    /* If current entry is an candidate for being the next
                     * entry then make current candidate if it is better
                     * than the current candidate.
                     */
                    if (cmp_result < 0) 
                    {
                        /* If interface index of current device is less
                         * than the existing candidate then mark the
                         * current entry as better candidate.
                         */
                        if ((dev->dev_index + 1) < cand_if_index)
                            cmp_result = 1;

                        /* If interface index of current device is equal
                         * to the candidate interface index and IPv6
                         * address of current entry is less than that of
                         * current candidate then mark the current entry
                         * as better candidate.
                         */
                        else 
                            cmp_result = 
                                memcmp(cand_addr, 
                                       dev->dev6_neighbor_cache[index].
                                       ip6_neigh_cache_ip_addr, IP6_ADDR_LEN);

                        /* If current entry is marked as better candidate
                         * then update the current candidate.
                         */
                        if (cmp_result > 0)
                        {
                            /* Update the candidate's interface index. */
                            cand_if_index = dev->dev_index + 1;

                            /* Update the candidate IPv6 address. */
                            NU_BLOCK_COPY(cand_addr, 
                                          dev->dev6_neighbor_cache[index].
                                          ip6_neigh_cache_ip_addr, 
                                          IP6_ADDR_LEN);
                        }
                    }
                }
            }

            /* If there is no chance of any upcoming entries for being
             * selected then break through the loop.
             */
            if ((dev->dev_index + 1) > cand_if_index)
                break;

            /* Moving forward in the list. */
            dev = dev->dev_next;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* If we did get next entry. */
        if (cand_if_index != 0xFFFFFFFFul)
        {
            /* Update the interface index passed in. */
            (*if_index) = cand_if_index;

            /* Update the IPv6 address passed in. */
            NU_BLOCK_COPY(ipv6_addr, cand_addr, IP6_ADDR_LEN);

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the next entry then return error code. */
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

} /* IP6_MIB_NTM_Get_Next_Index */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_NTM_Get
*
*   DESCRIPTION
*
*        This function is used get the value for the Net To Media table.
*
*   INPUTS
*
*        *if_index              Interface index.
*        *ipv6_addr             IPv6 address of neighbor cache entry.
*        *value                 Pointer to memory location where value is
*                               to copied.
*        opt                    Option defining which value is required.
*                               Valid values are:
*                                   2. ipv6NetToMediaPhysAddress
*                                   3. ipv6NetToMediaType
*                                   4. ipv6IfNetToMediaState
*                                   5. ipv6IfNetToMediaLastUpdated
*                                   6. ipv6NetToMediaValid
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   When we fail to find the neighbor cache
*                               entry.
*        IP6_MIB_ERROR          When we fail to grab the semaphore.
*        IP6_MIB_NOSUCHNAME     When value of 'opt' is invalid.
*
************************************************************************/
UINT16 IP6_MIB_NTM_Get(UINT32 if_index, UINT8 *ipv6_addr, VOID *value,
                       INT opt)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Index of relevant neighbor cache entry in the list maintained in
     * device structure.
     */
    INT                 index;

    /* Status to return success or error code. */
    UINT16              status;

    if ( (opt) && (opt <= IP6_MIB_NTM_GETTER_FUNCTION) &&
         (IP6_MIB_NTM_Getter_Functions[(opt - 1)] != NU_NULL) )
    {
        /* Grab the semaphore. */
        if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
        {
            /* Get the index of relevant neighbor cache entry in the list
             * maintained in device structure.
             */
            index = IP6_MIB_NTM_Get_Entry_Util(if_index, ipv6_addr, &dev);

            /* If there exists a relevant neighbor cache entry. */
            if (index != -1)
            {
                /* Call the appropriate function to get value. */
                status = 
                    (*IP6_MIB_NTM_Getter_Functions[(opt - 1)])
                        (&(dev->dev6_neighbor_cache[index]), value);
            }

            /* If relevant neighbor cache entry does not exist then
             * return error code.
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

    }

    /* If option's value is invalid then return error code. */
    else
        status = IP6_MIB_NOSUCHNAME;

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_NTM_Get */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_NTM_Get_Entry_Util
*
*   DESCRIPTION
*
*        This function is used to get neighbor cache entry by specifying
*        interface index and IPv6 address.
*
*   INPUTS
*
*        if_index               Interface index.
*        *ipv6_addr             IPv6 address of neighbor cache entry.
*        **dev                  Pointer to the location where handle to
*                               the interface device is to be stored.
*
*   OUTPUTS
*
*        -1                     When we did not the get relevant neighbor
*                               cache entry using indexes passed in.
*        >= 0                   Index of the relevant neighbor cache
*                               entry in the list maintained in device
*                               structure.
*
************************************************************************/
STATIC INT IP6_MIB_NTM_Get_Entry_Util(UINT32 if_index, UINT8 *ipv6_addr,
                                      DV_DEVICE_ENTRY **dev)
{
    /* Index of the relevant neighbor cache entry in the list maintained
     * in device structure. Setting its value to -1 to represent absence
     * of the relevant entry.
     */
    INT     index = -1;

    /* If we have valid interface index. */
    if (if_index)
    {
        /* Getting handle to the interface device. */
        (*dev) = DEV_Get_Dev_By_Index(if_index - 1);

        /* If we have got the handle to the interface device and that
         * device is IPv6 enabled.
         */
        if ( (*dev) && ((*dev)->dev_flags & DV6_IPV6) )
        {
            /* Loop to find the relevant neighbor cache entry. */
            for (index = 0; index < (*dev)->dev6_nc_entries; index++)
            {
                /* If we have reached at a relevant neighbor cache entry.
                 */
                if (memcmp((*dev)->dev6_neighbor_cache[index].
                           ip6_neigh_cache_ip_addr, ipv6_addr,
                           IP6_ADDR_LEN) == 0)
                {
                    /* Break through the loop. */
                    break;
                }
            }

            /* We did not get any relevant entry then return -1. */
            if (index == (*dev)->dev6_nc_entries)
                index = -1;
        }
    }

    /* Returning index of the relevant neighbor cache entry in the list
     * maintained in device structure if found. Otherwise returning -1.
     */
    return (index);

} /* IP6_MIB_NTM_Get_Entry_Util */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_NTM_Get_Phy_Address
*
*   DESCRIPTION
*
*        This function is used to get the value of physical address for a
*        neighbor cache entry.
*
*   INPUTS
*
*        *neigh_entry           Handle to neighbor cache entry.
*        *mac_addr              Pointer to the memory location where MAC
*                               address is to be copied.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_ERROR          When fail.
*
************************************************************************/
STATIC UINT16 IP6_MIB_NTM_Get_Phy_Address(IP6_NEIGHBOR_CACHE_ENTRY 
                                          *neigh_entry, VOID *mac_addr)
{
    /* Status to return success or error code. */
    UINT16              status;

    /* If link specific information is stored. */
    if (neigh_entry->ip6_neigh_cache_link_spec)
    {
        /* Get the value of link specific MAC address. */        
        NU_BLOCK_COPY(mac_addr,
                      ((IP6_ETH_NEIGHBOR_CACHE_ENTRY *)(neigh_entry->
                      ip6_neigh_cache_link_spec))->ip6_neigh_cache_hw_addr,
                      DADDLEN);

        /* Return success code. */
        status = IP6_MIB_SUCCESS;
    }

    /* If we don't have link specific information then return error code.
     */
    else
        status = IP6_MIB_ERROR;


    /* Return success or error code. */
    return (status);

} /* IP6_MIB_NTM_Get_Phy_Address */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_NTM_Get_Media_Type
*
*   DESCRIPTION
*
*        This function is used to get the value of media type for a
*        neighbor cache entry.
*
*   INPUTS
*
*        *neigh_entry           Handle to neighbor cache entry.
*        *media_type            Pointer to the memory location where value
*                               of media type is to be copied.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS
*
************************************************************************/
STATIC UINT16 IP6_MIB_NTM_Get_Media_Type(IP6_NEIGHBOR_CACHE_ENTRY *neigh_entry, 
                                         VOID *media_type)
{
    /* Status to return success or error code. */
    UINT16              status;

    /* Get the value of media type. */
    if (neigh_entry->ip6_neigh_cache_flags & NC_PERMANENT)
        (*((UINT32 *)(media_type))) = IP6_MIB_NTM_TYPE_LOCAL;

    else if (neigh_entry->ip6_neigh_cache_flags & (~NC_PERMANENT))
        (*((UINT32 *)(media_type))) = IP6_MIB_NTM_TYPE_STATIC;

    else
        (*((UINT32 *)(media_type))) = IP6_MIB_NTM_TYPE_OTHER;

    /* Return success code. */
    status = IP6_MIB_SUCCESS;

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_NTM_Get_Media_Type */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_NTM_Get_Media_State
*
*   DESCRIPTION
*
*        This function is used to get the value of media state for a
*        neighbor cache entry.
*
*   INPUTS
*
*        *neigh_entry           Handle to neighbor cache entry.
*        *media_state           Pointer to the memory location where 
*                               value of media state is to be copied.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS
*
************************************************************************/
STATIC UINT16 IP6_MIB_NTM_Get_Media_State(IP6_NEIGHBOR_CACHE_ENTRY *neigh_entry, 
                                          VOID *media_state)
{
    /* Status to return success or error code. */
    UINT16              status;

    /* Get the value of media state. */
    if (!(neigh_entry->ip6_neigh_cache_flags & NC_UP))
        (*((UINT32 *)(media_state))) = IP6_MIB_NTM_STATE_INVALID;

    if (neigh_entry->ip6_neigh_cache_state == NC_NEIGH_REACHABLE)
        (*((UINT32 *)(media_state))) = IP6_MIB_NTM_STATE_REACHABLE;

    else if (neigh_entry->ip6_neigh_cache_state == NC_NEIGH_STALE)
        (*((UINT32 *)(media_state))) = IP6_MIB_NTM_STATE_STALE;

    else if (neigh_entry->ip6_neigh_cache_state == NC_NEIGH_DELAY)
        (*((UINT32 *)(media_state))) = IP6_MIB_NTM_STATE_DELAY;

    else if (neigh_entry->ip6_neigh_cache_state == NC_NEIGH_PROBE)
        (*((UINT32 *)(media_state))) = IP6_MIB_NTM_STATE_PROBE;

    else
        (*((UINT32 *)(media_state))) = IP6_MIB_NTM_STATE_UNKNOWN;

    /* Return success code. */
    status = IP6_MIB_SUCCESS;

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_NTM_Get_Media_State */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_NTM_Get_Last_Updated
*
*   DESCRIPTION
*
*        This function is used to get the value of last updated time 
*        for a neighbor cache entry.
*
*   INPUTS
*
*        *neigh_entry           Handle to neighbor cache entry.
*        *last_updated          Pointer to the memory location where 
*                               value of last updated time is to be 
*                               copied.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS
*
************************************************************************/
STATIC UINT16 IP6_MIB_NTM_Get_Last_Updated(IP6_NEIGHBOR_CACHE_ENTRY *neigh_entry, 
                                           VOID *last_updated)
{
    /* Status to return success or error code. */
    UINT16              status;

    /* Get the value of last updated. */
    (*((UINT32 *)(last_updated))) = 
        neigh_entry->ip6_neigh_cache_nud_time;

    /* Return success code. */
    status = IP6_MIB_SUCCESS;

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_NTM_Get_Last_Updated */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_NTM_Get_Validity
*
*   DESCRIPTION
*
*        This function is used to get the value of validity of a 
*        neighbor cache entry.
*
*   INPUTS
*
*        *neigh_entry           Handle to neighbor cache entry.
*        *is_valid              Pointer to the memory location where 
*                               value of validity is to be copied.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS
*
************************************************************************/
STATIC UINT16 IP6_MIB_NTM_Get_Validity(IP6_NEIGHBOR_CACHE_ENTRY *neigh_entry, 
                                       VOID *is_valid)
{
    /* Status to return success or error code. */
    UINT16              status;

    /* Get the value for the validity of entry. */
    if (neigh_entry->ip6_neigh_cache_flags & NC_UP)
        (*((UINT32 *)(is_valid))) = IP6_MIB_TRUE;
    else
        (*((UINT32 *)(is_valid))) = IP6_MIB_FALSE;

    /* Return success code. */
    status = IP6_MIB_SUCCESS;

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_NTM_Get_Validity */

#endif /* (INCLUDE_IPV6_MIB_NTM == NU_TRUE) */
