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
*        ip6_mib_addr_pre.c                          
*
*   COMPONENT
*
*        IPv6 Prefix Address MIB
*
*   DESCRIPTION
*
*        This contains the implementation of IPv6 Prefix Address MIBs.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        IP6_MIB_ADDR_PRE_Get_Util
*        IP6_MIB_ADDR_PRE_Compare
*        IP6_MIB_ADDR_PRE_Get_Next
*        IP6_MIB_ADDR_PRE_Get_OnLinkFlag
*        IP6_MIB_ADDR_PRE_Get_AutmosFlag
*        IP6_MIB_ADDR_PRE_Get_PreferLife
*        IP6_MIB_ADDR_PRE_Get_ValidLife
*
*   DEPENDENCIES
*
*        nu_net.h
*        prefix6.h
*        snmp_api.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/prefix6.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#endif

#if (INCLUDE_IP6_MIB_ADDR_PREFIX == NU_TRUE)

STATIC IP6_PREFIX_ENTRY *IP6_MIB_ADDR_PRE_Get_Util(UINT32, UINT8 *, UINT32);
STATIC INT IP6_MIB_ADDR_PRE_Compare(UINT8 *, UINT32, UINT8 *, UINT32);

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_ADDR_PRE_Get_Util
*
*   DESCRIPTION
*
*        This function is used to get the handle to the prefix address
*        entry specified by the indexes passed in.
*
*   INPUTS
*
*        if_index               Interface index.
*        *addr_prefix           Prefix address.
*        prefix_len             Prefix length.
*
*   OUTPUTS
*
*        IP6_PREFIX_ENTRY*      When there exists an prefix address entry
*                               with indexes passed in.
*        NU_NULL                When there does not exist an prefix
*                               address entry with indexes passed in.
*
************************************************************************/
STATIC IP6_PREFIX_ENTRY *IP6_MIB_ADDR_PRE_Get_Util(UINT32 if_index,
                                                   UINT8 *addr_prefix, 
                                                   UINT32 prefix_len)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY         *dev;

    /* Handle to Address prefix entry. */
    IP6_PREFIX_ENTRY        *prefix_entry;

    /* Variable to hold last byte mask. */
    UINT8                   last_byte_mask;

    /* Status to return success or error code. */
    UINT16                  status = IP6_MIB_NOSUCHOBJECT;

    UINT16                  temp_prefix_len;

    /* If we have valid interface index and prefix length and there exists
     * an interface device associated with interface index passed in and
     * that the device have a non-empty address prefix list.
     */
    if ( (if_index > 0) && (prefix_len <= IP6_MIB_ADDR_MAX_PRE_LEN) &&
         ( (dev = DEV_Get_Dev_By_Index(if_index - 1)) != NU_NULL) &&
         (dev->dev6_prefix_list != NU_NULL) )
    {
        /* Prepare last byte mask. We use this mark to compare last byte
         * of prefix address.
         */
        last_byte_mask = 0xff;
        last_byte_mask >>= (8 - (prefix_len % 8));

        temp_prefix_len = (UINT16)(prefix_len >> 3);

        /* Get the starting pointer of address prefix entry list. */
        prefix_entry = dev->dev6_prefix_list->dv_head;

        /* Loop to find the required address prefix entry. */
        while ( (prefix_entry) && (status != IP6_MIB_SUCCESS) )
        {
            /* If address prefix and its length matches the return
             * success code. 
             */
            if ( (prefix_entry->ip6_prfx_lst_prfx_length == 
                  ((UINT8)prefix_len)) &&
                 ((memcmp(prefix_entry->ip6_prfx_lst_prefix, addr_prefix,
                          temp_prefix_len)) == 0) &&
                 ( (!last_byte_mask) ||
                 ( (prefix_entry->ip6_prfx_lst_prefix[temp_prefix_len] &
                    last_byte_mask) == 
                   (addr_prefix[temp_prefix_len] & last_byte_mask)) ) )
            {
                /* Returning success code. */
                status = IP6_MIB_SUCCESS;
            }

            /* If we did not reach the required entry then move forward in
             * the loop.
             */
            else
                prefix_entry = prefix_entry->ip6_prefx_lst_next;
        }
    }
    else
        prefix_entry = NU_NULL;

    /* Return handle to the prefix entry if found. Otherwise return
     * NU_NULL.
     */
    return (prefix_entry);

} /* IP6_MIB_ADDR_PRE_Get_Util */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_ADDR_PRE_Compare
*
*   DESCRIPTION
*
*        This function is used to compare two prefix address entries.
*
*   INPUTS
*
*        *fir_pre_addr          Prefix address of the first address.
*        fir_pre_len            Prefix length of the first address.
*        *sec_pre_addr          Prefix address of the second address.
*        sec_pre_len            Prefix length of the second address.
*
*   OUTPUTS
*
*       < 0                     If first entry is lesser then second.
*       0                       If both the entries are equal.
*       > 0                     If first entry is greater than second.
*
************************************************************************/
STATIC INT IP6_MIB_ADDR_PRE_Compare(UINT8 *fir_pre_addr,
                                    UINT32 fir_pre_len, UINT8 *sec_pre_addr, 
                                    UINT32 sec_pre_len)
{
    /* Variable to hold comparison result. */
    INT     cmp_result;
    
    /* Variable to hold last byte mask. */
    UINT8   last_byte_mask;

    /* Variable to hold the minimum prefix address passed in. */
    UINT16  min_prefix_addr_len = (UINT16)(IP6_MIB_ADDR_MIN(fir_pre_len, sec_pre_len));

    /* Comparing prefix address. */
    cmp_result = memcmp(fir_pre_addr, sec_pre_addr, min_prefix_addr_len);

    /* If prefix address are equal then no decision is made till now. */
    if (cmp_result == 0)
    {
        /* If we did not compare last byte. */
        if ((min_prefix_addr_len % 8) != 0)
        {
            /* Prepare last byte mask. */
            last_byte_mask = 0xff;
            last_byte_mask >>= (8 - (min_prefix_addr_len % 8));

            /* Comparing last byte using the last byte mask. */
            cmp_result =
                ((INT)(fir_pre_addr[min_prefix_addr_len / 8] & last_byte_mask)
                 - (INT)(sec_pre_addr[min_prefix_addr_len / 8] & last_byte_mask));
        }

        /* If addresses are still equal then take the decision using the
         * prefix address lengths.
         */
        if (cmp_result == 0)
        {
            cmp_result = (INT)(fir_pre_len) - (INT)(sec_pre_len);
        }
    }

    /* Return comparison result. */
    return (cmp_result);

} /* IP6_MIB_ADDR_PRE_Compare */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_ADDR_PRE_Get_Next
*
*   DESCRIPTION
*
*        This function is used to get the indexes of next prefix address
*        entry with indexes passed in.
*
*   INPUTS
*
*        *if_index              Interface index.
*        *addr_prefix           Prefix address.
*        prefix_len             Prefix address length.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist a prefix address
*                               entry with indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_ADDR_PRE_Get_Next(UINT32 *if_index, UINT8 *addr_prefix,
                                 UINT32 *prefix_len)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY         *dev;

    /* Handle to Address prefix entry. */
    IP6_PREFIX_ENTRY        *prefix_entry;

    /* Candidate Prefix Address. */
    UINT8                   cand_pre_addr[IP6_ADDR_LEN];

    /* Candidate prefix address length. */
    UINT32                  cand_pre_addr_len = 0;

    /* Variable to hold the comparison result. */
    INT                     cmp_result;

    /* Status to return success or error code. */
    UINT16                  status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Mark the status to error code. */
        status = IP6_MIB_NOSUCHOBJECT;

        /* Get the handle to the first interface device. */
        dev = DEV_Table.dv_head;

        /* Loop through the interface device table to get next prefix
         * address using interface index, prefix address and prefix
         * length passed in.
         */
        while ( (dev != NU_NULL) && (status != IP6_MIB_SUCCESS) )
        {
            /* If we have reached a device with equal or greater interface
             * index, and it is an IPv6-enabled device.
             */
            if ( ((dev->dev_index + 1) >= (*if_index)) &&
                 (dev->dev_flags & DV6_IPV6) )
            {
                if (dev->dev6_prefix_list != NU_NULL)
                {
                    prefix_entry = dev->dev6_prefix_list->dv_head;
    
                    /* Set the maximum value to the candidate prefix address.
                     */
                    memset(cand_pre_addr, (~0), IP6_ADDR_LEN);
    
                    /* Set candidate prefix address length to maximum value.
                     */
                    cand_pre_addr_len = IP6_ADDR_LEN;
    
                    /* Search for the prefix address entry with greater prefix
                     * address and prefix address length pair.
                     */
                    while (prefix_entry)
                    {
                        /* If we are at same interface as specified by
                         * interface index passed in then compare prefix
                         * addresses. Otherwise just mark current prefix
                         * address as greater entry.
                         */
                        if ((dev->dev_index + 1) == (*if_index))
                        {
                            /* Compare current entry with the prefix address
                             * passed in.
                             */
                            cmp_result = 
                                IP6_MIB_ADDR_PRE_Compare(prefix_entry->
                                                         ip6_prfx_lst_prefix,
                                                         (UINT32)prefix_entry->
                                                         ip6_prfx_lst_prfx_length,
                                                         addr_prefix, 
                                                         *prefix_len);
                        }
    
                        else
                            cmp_result = 1;
    
                        /* If the current entry has greater than the prefix
                         * address as passed in.
                         */
                        if (cmp_result > 0)
                        {
                            /* Check the current entry against the candidate.
                             */
                            cmp_result = 
                                IP6_MIB_ADDR_PRE_Compare(cand_pre_addr, 
                                                         cand_pre_addr_len,
                                                         prefix_entry->
                                                         ip6_prfx_lst_prefix,
                                                         (UINT32)prefix_entry->
                                                         ip6_prfx_lst_prfx_length);
    
                            /* We have reached a better candidate, */
                            if (cmp_result > 0)
                            {
                                /* Update the candidate prefix address. */
                                NU_BLOCK_COPY(cand_pre_addr, prefix_entry->
                                              ip6_prfx_lst_prefix, IP6_ADDR_LEN);
    
                                /* Update the candidate prefix length. */
                                cand_pre_addr_len = 
                                    (UINT32)(prefix_entry->ip6_prfx_lst_prfx_length);
    
                                /* Set status to success code. */
                                status = IP6_MIB_SUCCESS;
                            }
                        }
    
                        /* Moving forward in the list. */
                        prefix_entry = prefix_entry->ip6_prefx_lst_next;
                    }
                }
            }

            if (status != IP6_MIB_SUCCESS)
                dev = dev->dev_next;
        }

        /* If successfully get the next prefix address entry then update
         * the indexes passed in.
         */
        if ( (status == IP6_MIB_SUCCESS) && (dev) )
        {
            /* Update interface index. */
            (*if_index) = dev->dev_index + 1;

            /* Update the prefix length. */
            (*prefix_len) = cand_pre_addr_len;

            /* Update the prefix address. */
            NU_BLOCK_COPY(addr_prefix, cand_pre_addr, IP6_ADDR_LEN);
        }

        /* If we failed to find the next prefix address entry then return
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

} /* IP6_MIB_ADDR_PRE_Get_Next */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_ADDR_PRE_Get_OnLinkFlag
*
*   DESCRIPTION
*
*        This function is used to get the value of 'On Link Flag' of the
*        prefix address entry specified by indexes passed in.
*
*   INPUTS
*
*        if_index               Interface index.
*        *addr_prefix           Prefix address.
*        prefix_len             Prefix address length.
*        *on_link_flag          Pointer to memory location where value of
*                               'On Link Flag' is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist a prefix address
*                               entry with indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_ADDR_PRE_Get_OnLinkFlag(UINT32 if_index, UINT8 *addr_prefix,
                                       UINT32 prefix_len, 
                                       UINT32 *on_link_flag)
{
    /* Handle to Address prefix entry. */
    IP6_PREFIX_ENTRY        *prefix_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Getting handle to the prefix address. */
        prefix_entry = IP6_MIB_ADDR_PRE_Get_Util(if_index, addr_prefix,
                                                 prefix_len);

        /* If we got the handle to the prefix address entry. */
        if (prefix_entry)
        {
            /* Getting the value of 'On Link Flag'. */
            if (prefix_entry->ip6_prfx_lst_flags & PRFX6_NO_ADV_ON_LINK)
                (*on_link_flag) = IP6_MIB_TRUE;
            else
                (*on_link_flag) = IP6_MIB_FALSE;

            /* Return error code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we failed to get the handle to the prefix address entry. */
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

} /* IP6_MIB_ADDR_PRE_Get_OnLinkFlag */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_ADDR_PRE_Get_AutmosFlag
*
*   DESCRIPTION
*
*        This function is used to get the value of 'Autonomous Flag' of
*        the prefix address entry specified by indexes passed in.
*
*   INPUTS
*
*        if_index               Interface index.
*        *addr_prefix           Prefix address.
*        prefix_len             Prefix address length.
*        *autonomous_flag       Pointer to the memory location where value
*                               of 'Autonomous Flag'.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist a prefix address
*                               entry with indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_ADDR_PRE_Get_AutmosFlag(UINT32 if_index, UINT8 *addr_prefix,
                                       UINT32 prefix_len, 
                                       UINT32 *autonomous_flag)
{
    /* Handle to Address prefix entry. */
    IP6_PREFIX_ENTRY        *prefix_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Getting handle to the prefix address. */
        prefix_entry = IP6_MIB_ADDR_PRE_Get_Util(if_index, addr_prefix,
                                                 prefix_len);

        /* If we got the handle to the prefix address entry. */
        if (prefix_entry)
        {
            /* Getting the value of 'autonomous flag'. */
            if (prefix_entry->ip6_prfx_lst_flags & PRFX6_NO_ADV_AUTO)
                (*autonomous_flag) = IP6_MIB_TRUE;
            else
                (*autonomous_flag) = IP6_MIB_FALSE;

            /* Return error code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we failed to get the handle to the prefix address entry. */
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

} /* IP6_MIB_ADDR_PRE_Get_AutmosFlag */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_ADDR_PRE_Get_PreferLife
*
*   DESCRIPTION
*
*        This function is used to get the value of 'ADV Preferred Life
*        time' of the prefix address entry specified by indexes passed in.
*
*   INPUTS
*
*        if_index               Interface index.
*        *addr_prefix           Prefix address.
*        prefix_len             Prefix address length.
*        *preferred_life_tim    Pointer to the the memory location where
*                               value of 'ADV Preferred Life time' is to
*                               be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist a prefix address
*                               entry with indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_ADDR_PRE_Get_PreferLife(UINT32 if_index, UINT8 *addr_prefix,
                                       UINT32 prefix_len, 
                                       UINT32 *preferred_life_tim)
{
    /* Handle to Address prefix entry. */
    IP6_PREFIX_ENTRY        *prefix_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Getting handle to the prefix address. */
        prefix_entry = IP6_MIB_ADDR_PRE_Get_Util(if_index, addr_prefix,
                                                 prefix_len);

        /* If we got the handle to the prefix address entry. */
        if (prefix_entry)
        {
            /* Get the value of preferred life time. */
            (*preferred_life_tim) = prefix_entry->ip6_prfx_lst_pref_life;

            /* Return error code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we failed to get the handle to the prefix address entry. */
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

} /* IP6_MIB_ADDR_PRE_Get_PreferLife */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_ADDR_PRE_Get_ValidLife
*
*   DESCRIPTION
*
*        This function is used to get the value of 'ADV Valid Life time'
*        of the prefix address entry specified by indexes passed in.
*
*   INPUTS
*
*        if_index               Interface index.
*        *addr_prefix           Prefix address.
*        prefix_len             Prefix address length.
*        *valid_adv_life_time   Pointer to the memory location where value
*                               of 'ADV Valid Life time' is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist a prefix address
*                               entry with indexes passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_ADDR_PRE_Get_ValidLife(UINT32 if_index, UINT8 *addr_prefix,
                                      UINT32 prefix_len, 
                                      UINT32 *valid_adv_life_time)
{
    /* Handle to address prefix entry. */
    IP6_PREFIX_ENTRY        *prefix_entry;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Getting handle to the prefix address. */
        prefix_entry = IP6_MIB_ADDR_PRE_Get_Util(if_index, addr_prefix,
                                                 prefix_len);

        /* If we got the handle to the prefix address entry. */
        if (prefix_entry)
        {
            /* Get the value of valid life time. */
            (*valid_adv_life_time) = 
                prefix_entry->ip6_prfx_lst_valid_life;

            /* Return error code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we failed to get the handle to the prefix address entry. */
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

} /* IP6_MIB_ADDR_PRE_Get_ValidLife */

#endif /* (INCLUDE_IP6_MIB_ADDR_PREFIX == NU_TRUE) */
