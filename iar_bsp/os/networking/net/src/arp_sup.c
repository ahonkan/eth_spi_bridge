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
*   FILE NAME
*
*       arp_sup.c
*
*   DESCRIPTION
*
*       This file contains supplemental routines for ARP.
*
*   DATA STRUCTURES
*
*       none
*
*   FUNCTIONS
*
*       ARP_Get_Next
*       ARP_Get_Next_If
*       ARP_Get_Index
*       ARP_Get_Index_If
*
*   DEPENDENCIES
*
*       nu_net.h
*       net_extr.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/net_extr.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp.h"
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

extern ARP_ENTRY ARP_Cache[];

#ifndef SNMP_VERSION_COMP
/*************************************************************************
*
*   FUNCTION
*
*       ARP_Get_Next
*
*   DESCRIPTION
*
*       This function gets the next entry after the current.
*
*   INPUTS
*
*       *ip_addr                A pointer to the IP address preceding
*                               the IP address to return.
*
*   OUTPUTS
*
*       *ARP_ENTRY              A pointer to the next entry.
*       NU_NULL                 If a semaphore cannot be obtained.
*
*************************************************************************/
ARP_ENTRY *ARP_Get_Next(const UINT8 *ip_addr, ARP_ENTRY *arp_entry)
{
    UINT32      current_ip;
    INT         candidate = -1;
    INT         i;
    STATUS      status;
    ARP_ENTRY   *ret_entry;

    NU_SUPERV_USER_VARIABLES

    /* Validate the input parameters. */
    if ( (arp_entry == NU_NULL) || (ip_addr == NU_NULL) )
        return (NU_NULL);

    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    /* If the semaphore could not be obtained, exit. */
    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (NU_NULL);
    }

    /* Convert the 4-byte array to a 32-bit unsigned integer for quicker
     * comparison in the loop.
     */
    current_ip = IP_ADDR(ip_addr);

    /* Traverse the entire ARP cache, looking for the next ARP cache
     * entry after the current entry.
     */
    for (i = 0; i < ARP_CACHE_LENGTH; i++)
    {
        /* If the IP address of this ARP cache entry is greater than the
         * IP address of the current ARP cache entry, and the entry is
         * valid, consider this entry for a match.
         */
        if ( (ARP_Cache[i].ip_addr.arp_ip_addr > current_ip) &&
             ((INT32_CMP((ARP_Cache[i].arp_time + CACHETO), NU_Retrieve_Clock()) > 0)
                  || (ARP_Cache[i].arp_flags & ARP_PERMANENT))
                  && (ARP_Cache[i].arp_flags & ARP_UP) )
        {
            /* If a match has not already been found or this entry is less
             * than the match already found, then this entry is a better match
             * than the previous entry found.  Save this entry.
             */
            if ( (candidate == -1) ||
                 (ARP_Cache[candidate].ip_addr.arp_ip_addr >
                  ARP_Cache[i].ip_addr.arp_ip_addr) )
            {
                candidate = i;
            }
        }
    }

    /* If a next entry has been found, copy it into the user's memory. */
    if (candidate != -1)
    {
        NU_BLOCK_COPY(arp_entry, &ARP_Cache[candidate], sizeof(ARP_ENTRY));
        ret_entry = arp_entry;
    }

    /* Otherwise, return NU_NULL to indicate there is no next entry. */
    else
        ret_entry = NU_NULL;

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    NU_USER_MODE();

    return (ret_entry);

} /* ARP_Get_Next */
#endif

#ifdef SNMP_VERSION_COMP
/*************************************************************************
*
*   FUNCTION
*
*       ARP_Get_Next_If
*
*   DESCRIPTION
*
*       This function gets the next entry after the current.  Repeated
*       calls to this function would return ARP entry in sorted
*       w.r.t if_index and IP Address.
*
*   INPUTS
*
*       if_index                The index of the interface associated
*                               with the current ARP Cache entry.
*       *ip_addr                A pointer to the IP address preceding
*                               the IP address to return.
*       *arp_entry              A pointer to the memory into which the
*                               returned ARP Cache entry will be placed.
*
*   OUTPUTS
*
*       *ARP_ENTRY              A pointer to the next entry.
*       NU_NULL                 If a semaphore cannot be obtained.
*
*************************************************************************/
ARP_ENTRY *ARP_Get_Next_If(UINT32 if_index, const UINT8 *ip_addr,
                           ARP_ENTRY *arp_entry)
{
    UINT32      current_ip;
    INT         candidate = -1;
    INT         i;
    STATUS      status;
    ARP_ENTRY   *ret_entry;

    NU_SUPERV_USER_VARIABLES

    if ( (arp_entry == NU_NULL) || (ip_addr == NU_NULL) )
        return (NU_NULL);

    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (NU_NULL);
    }

    current_ip = IP_ADDR(ip_addr);

    for (i = 0; i < ARP_CACHE_LENGTH; i++)
    {
        /* If this is also a candidate. */
        if ( (ARP_Cache[i].arp_dev_index != -1) &&
             ((ARP_Cache[i].arp_dev_index + 1 > if_index) ||
              ((ARP_Cache[i].arp_dev_index + 1 == if_index) &&
               (ARP_Cache[i].ip_addr.arp_ip_addr > current_ip))) )
        {
            /* If this valid entry. */
            if ( ((INT32_CMP((ARP_Cache[i].arp_time + CACHETO),
                            NU_Retrieve_Clock()) > 0)
                  || (ARP_Cache[i].arp_flags & ARP_PERMANENT))
                  && (ARP_Cache[i].arp_flags & ARP_UP) )
            {
                /* If candidate not present or this entry is better than
                 * current candidate.
                 */
                if ( (candidate == -1) ||
                     (ARP_Cache[candidate].arp_dev_index > ARP_Cache[i].arp_dev_index) ||
                     ((ARP_Cache[candidate].arp_dev_index == ARP_Cache[i].arp_dev_index) &&
                      (ARP_Cache[candidate].ip_addr.arp_ip_addr >
                       ARP_Cache[i].ip_addr.arp_ip_addr)) )
                {
                    /* Update the candidate. */
                    candidate = i;
                }
            }
        }
    }

    if (candidate != -1)
    {
        NU_BLOCK_COPY(arp_entry, &ARP_Cache[candidate], sizeof(ARP_ENTRY));
        ret_entry = arp_entry;
    }

    else
        ret_entry = NU_NULL;

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    NU_USER_MODE();

    return (ret_entry);

} /* ARP_Get_Next_If */
#endif

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Get_Index
*
*   DESCRIPTION
*
*       This function returns the index of device associated with the
*       entry.
*
*   INPUTS
*
*       *ip_addr                A pointer to the IP address associated
*                               with the target entry.
*
*   OUTPUTS
*
*       INT32                   Upon success, the function returns the
*                               index of the entry.
*       -1                      The target entry does not exist.
*
*************************************************************************/
INT32 ARP_Get_Index(const UINT8 *ip_addr)
{
    SCK_SOCKADDR_IP     target;
    RTAB4_ROUTE_ENTRY   *rt_entry;
    INT32               dev_index;

    /* Convert the ip address from an array to a long unsigned int */
    target.sck_addr = IP_ADDR(ip_addr);

    rt_entry = RTAB4_Find_Route(&target, RT_BEST_METRIC | RT_OVERRIDE_METRIC |
                                RT_OVERRIDE_DV_STATE);

    if (rt_entry)
    {
        /* Use the device index whether we found the exact route, a network
         * route, or the default route.
         */
        dev_index = (INT32)rt_entry->rt_entry_parms.rt_parm_device->dev_index;

        RTAB_Free((ROUTE_ENTRY*)rt_entry, NU_FAMILY_IP);
    }
    else
        dev_index = -1;

    return (dev_index);

} /* ARP_Get_Index */

#ifdef SNMP_VERSION_COMP
/*************************************************************************
*
*   FUNCTION
*
*       ARP_Get_Index_If
*
*   DESCRIPTION
*
*       This function returns the index of device associated with the
*       entry.
*
*   INPUTS
*
*       if_index                The index of the interface associated
*                               with the desired entry.
*       *ip_addr                A pointer to the IP address associated
*                               with the target entry.
*       *arp_entry              A pointer to the memory into which the
*                               retrieved ARP Cache entry will be placed.
*
*   OUTPUTS
*
*       NU_SUCCESS              Upon success, the function returns the
*                               index of the entry.
*       -1                      The target entry does not exist.
*
*************************************************************************/
ARP_ENTRY *ARP_Get_Index_If(UINT32 if_index, const UINT8 *ip_addr,
                            ARP_ENTRY *arp_entry)
{
    UINT32      current_ip;
    INT         i;
    STATUS      status;
    ARP_ENTRY   *ret_entry;

    NU_SUPERV_USER_VARIABLES

    if ( (arp_entry == NU_NULL) || (ip_addr == NU_NULL) )
        return (NU_NULL);

    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (NU_NULL);
    }

    current_ip = IP_ADDR(ip_addr);

    for (i = 0; i < ARP_CACHE_LENGTH; i++)
    {
        /* If this is also a candidate. */
        if ( (ARP_Cache[i].arp_dev_index != -1) &&
             (ARP_Cache[i].arp_dev_index + 1 == if_index) &&
             (ARP_Cache[i].ip_addr.arp_ip_addr == current_ip) )
        {
            /* If this valid entry. */
            if ( ((INT32_CMP((ARP_Cache[i].arp_time + CACHETO),
                             NU_Retrieve_Clock()) > 0)
                  || (ARP_Cache[i].arp_flags & ARP_PERMANENT))
                  && (ARP_Cache[i].arp_flags & ARP_UP) )
            {
                break;
            }
        }
    }

    if (i != ARP_CACHE_LENGTH)
    {
        NU_BLOCK_COPY(arp_entry, &ARP_Cache[i], sizeof(ARP_ENTRY));
        ret_entry = arp_entry;
    }

    else
        ret_entry = NU_NULL;

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    NU_USER_MODE();

    return (ret_entry);

} /* ARP_Get_Index_If */
#endif

#endif
