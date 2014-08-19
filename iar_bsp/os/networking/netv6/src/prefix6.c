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
*       prefix6.c                                    
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This file contains those functions necessary to maintain the
*       Prefix List for a given interface.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       Prefix_Index
*                                                                       
*   FUNCTIONS                                                             
*           
*       PREFIX6_Find_Prefix_List_Entry
*       PREFIX6_Find_Prefix
*       PREFIX6_New_Prefix_Entry
*       PREFIX6_Configure_DEV_Prefix_Entry
*       PREFIX6_Expire_Entry
*       PREFIX6_Delete_Prefix
*       PREFIX6_Delete_Entry
*       PREFIX6_Match_Longest_Prefix_By_Device
*       PREFIX6_Match_Longest_Prefix
*       PREFIX6_Init_Prefix_List
*       PREFIX6_Find_Home_Prefix
*       PREFIX6_Find_On_Link_Prefix
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       externs.h
*       in6.h
*       prefix6.h
*                                                                       
*************************************************************************/

#include "networking/externs.h"
#include "networking/in6.h"
#include "networking/prefix6.h"

static  UINT32  Prefix_Index = 1;

extern TQ_EVENT PREFIX6_Expire_Entry_Event;

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       PREFIX6_Find_Prefix_Entry
*                                                                         
*   DESCRIPTION                                                           
*              
*       This function finds the Prefix List entry associated with an
*       interface, or, if no device is provided, finds the Prefix List
*       entry on any one interface.
*                                                                         
*   INPUTS                                                                
*                          
*       *device                 The interface on which to search for the 
*                               Prefix.
*       *prefix                 The target prefix.                                               
*                                                                         
*   OUTPUTS                                                               
*                                     
*       A pointer to the Prefix List entry or NU_NULL if no entry
*       exists.                                    
*
*************************************************************************/
IP6_PREFIX_ENTRY *PREFIX6_Find_Prefix_List_Entry(const DV_DEVICE_ENTRY *device,
                                                 const UINT8 *prefix)
{
    IP6_PREFIX_ENTRY    *target_entry = NU_NULL;
    DV_DEVICE_ENTRY     *current_device;

    /* If a pointer to a device was passed in, search that device's
     * Prefix List for a matching entry.
     */
    if (device != NU_NULL)
        target_entry = PREFIX6_Find_Prefix(device, prefix);

    /* Otherwise, search each device's Prefix List for a matching
     * entry.
     */
    else
    {
        /* Get a pointer to the first device in the list */
        current_device = DEV_Table.dv_head;

        /* Don't waste time searching the loopback device */
        if (current_device->dev_type == DVT_LOOP)
            current_device = current_device->dev_next;

        /* Traverse the Device list until the target Prefix is found 
         * or we have reached the end of the Device list.
         */
        while (current_device)
        {
            if (current_device->dev_flags & DV6_IPV6)
            {
                /* Search the current device's Prefix List for the target
                 * prefix entry.
                 */
                target_entry = PREFIX6_Find_Prefix(current_device, prefix);
    
                /* If the target was found, break */
                if (target_entry)
                    break;
            }

            /* Get a pointer to the next Device list entry */
            current_device = current_device->dev_next;
        }
    }

    /* Return a pointer to the Prefix List entry or NU_NULL if no
     * entry was found.
     */
    return (target_entry);

} /* PREFIX6_Find_Prefix_List_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       PREFIX6_Find_Prefix
*                                                                         
*   DESCRIPTION                                                           
*              
*       This function finds the Prefix entry associated with the prefix
*       for a given device.
*                                                                         
*   INPUTS                                                                
*                          
*       *device                 The interface on which to search for the 
*                               Prefix.
*       *prefix                 The target prefix.                                               
*                                                                         
*   OUTPUTS                                                               
*                                     
*       A pointer to the Prefix List entry or NU_NULL if no entry
*       exists.                                    
*
*************************************************************************/
IP6_PREFIX_ENTRY *PREFIX6_Find_Prefix(const DV_DEVICE_ENTRY *device, 
                                      const UINT8 *prefix)
{
    IP6_PREFIX_ENTRY    *current_entry = NU_NULL;
   
    if (device->dev6_prefix_list)
    {
        /* Get a pointer to the first Prefix List entry in the list */
        current_entry = device->dev6_prefix_list->dv_head;

        /* Traverse the Prefix list until we reach the end of the list or
         * find the target Prefix.
         */
        while (current_entry != NU_NULL)
        {
            if (memcmp(current_entry->ip6_prfx_lst_prefix, prefix,
                       IP6_ADDR_LEN) == 0)
                break;

            /* Get a pointer to the next entry in the list */
            current_entry = current_entry->ip6_prefx_lst_next;
        }
    }

    /* Return a pointer to the Prefix List entry or NU_NULL if no
     * entry was found.
     */
    return (current_entry);

} /* PREFIX6_Find_Prefix */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       PREFIX6_New_Prefix_Entry
*                                                                         
*   DESCRIPTION                                                           
*              
*       This function adds a Prefix List entry to the Prefix List
*       associated with the specified interface.                                                           
*                                                                         
*   INPUTS                                                                
*                                               
*       *device                 A pointer to the device on which to add 
*                               the entry.
*       *prefix                 A pointer to the Prefix to add.
*       prefix_length           The length of the new Prefix in bits.
*       valid_lifetime          The Valid Lifetime of the new entry.                      
*                                                                         
*   OUTPUTS                                                               
*
*       NU_SUCCESS              The prefix was added.
*       NU_NO_MEMORY            Insufficient memory.
*       NU_INVALID_PARM			The device does not support a prefix list.
*
*************************************************************************/
STATUS PREFIX6_New_Prefix_Entry(DV_DEVICE_ENTRY *device, const UINT8 *prefix, 
                                UINT8 prefix_length, UINT32 valid_lifetime,
                                UINT32 preferred_lifetime, UINT32 flags)
{
    IP6_PREFIX_ENTRY    *new_entry;
    STATUS              status;
    UINT8               prefix_copy_length;

    /* If there is no prefix list for this interface, then it is an interface
     * type that does not support prefixes; ie, the loopback device.
     */
    if (device->dev6_prefix_list != NU_NULL)
    {
		/* If there is enough memory to create a new entry, create the entry,
		 * initialize its parameters, and link it into the list.  Otherwise,
		 * return -1.
		 */
		status = NU_Allocate_Memory(MEM_Cached, (VOID**)&new_entry,
									sizeof(IP6_PREFIX_ENTRY), NU_NO_SUSPEND);

		if (status == NU_SUCCESS)
		{
			UTL_Zero(new_entry, sizeof(IP6_PREFIX_ENTRY));

			/* Set the length of bytes to copy */
			prefix_copy_length = (UINT8)(prefix_length >> 3);

			/* If the length of bytes to copy is not a multiple of 8, add one
			 * to the length.
			 */
			if ( (prefix_length % 8) != 0)
				prefix_copy_length += (UINT8)1;

			/* Set up the new entry */
			memcpy(new_entry->ip6_prfx_lst_prefix, prefix, prefix_copy_length);
			new_entry->ip6_prefx_lst_device = device;
			new_entry->ip6_prfx_lst_valid_life = valid_lifetime;
			new_entry->ip6_prfx_lst_pref_life = preferred_lifetime;
			new_entry->ip6_prfx_lst_flags = flags;
			new_entry->ip6_prfx_lst_prfx_length = prefix_length;
			new_entry->ip6_prfx_lst_index = Prefix_Index++;
			new_entry->ip6_prfx_lst_pref_life_exp =
				NU_Retrieve_Clock() + (preferred_lifetime * SCK_Ticks_Per_Second);
			new_entry->ip6_prfx_lst_valid_life_exp =
				NU_Retrieve_Clock() + (valid_lifetime * SCK_Ticks_Per_Second);

			/* We use this value to know the amount of time expired */
			new_entry->ip6_prfx_lst_stored_life = NU_Retrieve_Clock();

			/* If this is not a permanent prefix (the link-local), then set
			 * the timer to expire it.
			 */
			if (valid_lifetime != 0xffffffffUL)
			{
				if (TQ_Timerset(PREFIX6_Expire_Entry_Event, new_entry->ip6_prfx_lst_index,
								new_entry->ip6_prfx_lst_valid_life * TICKS_PER_SECOND,
								(UNSIGNED)(new_entry->ip6_prefx_lst_device->dev_index))
								!= NU_SUCCESS)
					NLOG_Error_Log("Failed to set the timer to expire the Prefix entry",
								   NERR_SEVERE, __FILE__, __LINE__);
			}

			/* Insert the entry into the Prefix List for the device */
			DLL_Enqueue(device->dev6_prefix_list, new_entry);
		}
		else
		{
			NLOG_Error_Log("Could not add Prefix entry due to lack of memory",
						   NERR_SEVERE, __FILE__, __LINE__);
		}
    }
    
    else
    {
		NLOG_Error_Log("Could not add Prefix entry due to bad device index",
					   NERR_RECOVERABLE, __FILE__, __LINE__);

    	status = NU_INVALID_PARM;
    }

    return (status);

} /* PREFIX6_New_Prefix_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       PREFIX6_Configure_DEV_Prefix_Entry
*                                                                         
*   DESCRIPTION                                                           
*              
*       This function adds a Prefix List entry to the Prefix List
*       associated with the specified interface and creates an IPv6
*       address from the prefix if the PRFX6_NO_ADV_AUTO flags is
*       not set for the prefix.
*                                                                         
*   INPUTS                                                                
*                                               
*       *prefix_entry           A pointer to the Prefix to add.
*       *dev_ptr                A pointer to the device.                     
*                                                                         
*   OUTPUTS                                                               
*
*       NU_SUCCESS              The prefix was added.
*       NU_NO_MEMORY            Insufficient memory.
*
*************************************************************************/
STATUS PREFIX6_Configure_DEV_Prefix_Entry(DEV6_PRFX_ENTRY *prefix_entry,
                                          DV_DEVICE_ENTRY *dev_ptr)
{
    STATUS  status = NU_SUCCESS;

    /* If the value of the valid_lifetime is 0, use the default */
    if (prefix_entry->prfx_adv_valid_lifetime == 0)
        prefix_entry->prfx_adv_valid_lifetime = 
            IP6_DEFAULT_ADV_VALID_LIFETIME;

    /* If the value of the preferred_lifetime is 0, use the default */
    if (prefix_entry->prfx_adv_pref_lifetime == 0)
        prefix_entry->prfx_adv_pref_lifetime = 
            IP6_DEFAULT_ADV_PREFERRED_LIFETIME;

    /* If the prefix length is invalid, or the preferred lifetime
     * is greater than the valid lifetime, do not add this entry.
     */
    if ( (prefix_entry->prfx_length <= 0) ||
         (prefix_entry->prfx_length > 128) ||
         (prefix_entry->prfx_adv_pref_lifetime >
    	  prefix_entry->prfx_adv_valid_lifetime) )
        status = NU_INVALID_PARM;

    if (status == NU_SUCCESS)
    {
		/* Add the prefix entry */
		status = PREFIX6_New_Prefix_Entry(dev_ptr, prefix_entry->prfx_prefix,
										  (UINT8)prefix_entry->prfx_length,
										  prefix_entry->prfx_adv_valid_lifetime,
										  prefix_entry->prfx_adv_pref_lifetime,
										  prefix_entry->prfx_flags);

		/* If this prefix should be used for autoconfiguration, autocreate an
		 * IPv6 address for the interface.
		 */
		if ( (status == NU_SUCCESS) &&
			 (!(prefix_entry->prfx_flags & PRFX6_NO_ADV_AUTO)) )
			status = DEV6_Create_Address_From_Prefix(dev_ptr,
													 prefix_entry->prfx_prefix,
													 (UINT8)prefix_entry->prfx_length,
													 prefix_entry->prfx_adv_pref_lifetime,
													 prefix_entry->prfx_adv_valid_lifetime, 0);
    }

    return (status);

} /* PREFIX6_Configure_DEV_Prefix_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       PREFIX6_Expire_Entry
*                                                                         
*   DESCRIPTION                                                           
*              
*       This function deletes a Prefix List entry.                                                           
*                                                                         
*   INPUTS                                                                
*                                               
*       event                   The event that is being handled.
*       index                   The index value of the Prefix List 
*                               entry.
*       dev_index               The index of the device.
*                                                                         
*   OUTPUTS                                                               
*
*       None
*
*************************************************************************/
VOID PREFIX6_Expire_Entry(TQ_EVENT event, UNSIGNED index, UNSIGNED dev_index)
{
    DV_DEVICE_ENTRY     *device;
    IP6_PREFIX_ENTRY    *current_entry;

    UNUSED_PARAMETER(event);

    /* Get a pointer to the device associated with the device index */
    device = DEV_Get_Dev_By_Index((UINT32)dev_index);

    if (device)
    {
        if (device->dev_flags & DV6_IPV6)
        {
            /* Get a pointer to the first entry in the Prefix List */
            current_entry = device->dev6_prefix_list->dv_head;
    
            /* Traverse the Prefix List searching for the entry with a matching
             * index value.
             */
            while (current_entry)
            {
                /* If this is the target prefix, delete it */
                if (current_entry->ip6_prfx_lst_index == index)
                {
                    PREFIX6_Delete_Entry(current_entry);
                    break;
                }
    
                current_entry = current_entry->ip6_prefx_lst_next;
            }
        }
    }

} /* PREFIX6_Expire_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       PREFIX6_Delete_Prefix
*                                                                         
*   DESCRIPTION                                                           
*                       
*       This function removes a Prefix List entry associated with the
*       specified interface.                                                  
*                                                                         
*   INPUTS                                                                
*                           
*       *device                 A pointer to the device on which to remove 
*                               the entry.
*       *prefix                 A pointer to the prefix to remove.                                              
*                                                                         
*   OUTPUTS                                                               
*                                                     
*       None                    
*
*************************************************************************/
VOID PREFIX6_Delete_Prefix(const DV_DEVICE_ENTRY *device, 
                           const UINT8 *prefix)
{
    IP6_PREFIX_ENTRY    *current_entry;

    /* Get a pointer to the corresponding entry */
    current_entry = PREFIX6_Find_Prefix_List_Entry(device, prefix);
    
    /* If an entry exists, delete it */
    if (current_entry)
        PREFIX6_Delete_Entry(current_entry);

} /* PREFIX6_Delete_Prefix */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       PREFIX6_Delete_Entry
*                                                                         
*   DESCRIPTION                                                           
*                       
*       This function removes the given prefix entry.                                                
*                                                                         
*   INPUTS                                                                
*                           
*       *prefix_entry           A pointer to the prefix entry to remove.
*                                                                         
*   OUTPUTS                                                               
*                                                     
*       None                    
*
*************************************************************************/
VOID PREFIX6_Delete_Entry(IP6_PREFIX_ENTRY *prefix_entry)
{
    /* If an entry exists, delete it */
    if (prefix_entry)
    {
        /* Clear the timer for the Prefix Entry */
        if (TQ_Timerunset(PREFIX6_Expire_Entry_Event, TQ_CLEAR_EXACT,
                          prefix_entry->ip6_prfx_lst_index, 
                          (UNSIGNED)(prefix_entry->ip6_prefx_lst_device->dev_index)) 
                          != NU_SUCCESS)
            NLOG_Error_Log("Failed to clear the timer for the Prefix entry", 
                           NERR_SEVERE, __FILE__, __LINE__);

        /* Remove the entry from the list */
        DLL_Remove(prefix_entry->ip6_prefx_lst_device->dev6_prefix_list, 
                   prefix_entry);

        /* Deallocate the memory being used by the deleted entry */
        if (NU_Deallocate_Memory((VOID*)prefix_entry) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for the Prefix entry", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }

} /* PREFIX6_Delete_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       PREFIX6_Match_Longest_Prefix_By_Device
*                                                                         
*   DESCRIPTION                                                           
*                 
*       This function finds the longest matching prefix in the Prefix
*       List associated with the target address provided.                                                        
*                                                                        
*   INPUTS                                                                
*                                                        
*       *prefix_list            A pointer to the Prefix List to search.
*       *target                 A pointer to the target address.                 
*                                                                         
*   OUTPUTS                                                               
*                                         
*       A pointer to the Prefix or NU_NULL if the Prefix List is empty.                  
*
*************************************************************************/
IP6_PREFIX_ENTRY *PREFIX6_Match_Longest_Prefix_By_Device(const IP6_PREFIX_LIST *prefix_list, 
                                                         UINT8 *target)
{
    IP6_PREFIX_ENTRY    *current_prefix, *found_prefix = NU_NULL;
    UINT8               longest_match = 0, next_length;

    /* If the Prefix List is not empty, search for a matching entry */
    if (prefix_list)
    {
        /* Get the first entry in the Prefix List */
        current_prefix = prefix_list->dv_head;   
     
        /* While there are prefixes left in the Prefix list to examine */
        while (current_prefix != NU_NULL)
        {
            /* If the prefix is flagged as being on-link */
            if (!(current_prefix->ip6_prfx_lst_flags & PRFX6_NO_ADV_ON_LINK))
            {
                /* Determine how many bits of the current prefix match the
                 * target prefix.
                 */
                next_length = (UINT8)in6_matchlen(current_prefix->ip6_prfx_lst_prefix, 
                                                  target);

                /* If this prefix matches at least the prefix length of the
                 * target and is a better match than the previous prefix,
                 * set the new longest length and save the address.
                 */
                if ( (next_length >= current_prefix->ip6_prfx_lst_prfx_length) &&
                     (next_length > longest_match) )
                {
                    longest_match = next_length;
                    found_prefix = current_prefix;
                }
            }

            current_prefix = current_prefix->ip6_prefx_lst_next;
        }
    }

    /* Return a pointer to the longest matching Prefix or NU_NULL if no
     * prefixes match the target.
     */
    return (found_prefix);

} /* PREFIX6_Match_Longest_Prefix_By_Device */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       PREFIX6_Match_Longest_Prefix
*                                                                         
*   DESCRIPTION                                                           
*                 
*       This function determines if there is a longest matching prefix
*       for the target on any device on the node.                                            
*                                                                        
*   INPUTS                                                                
*                                                        
*       *target                 A pointer to the target address.                 
*                                                                         
*   OUTPUTS                                                               
*                                         
*       A pointer to the Prefix or NU_NULL if the Prefix List is empty.                  
*
*************************************************************************/
IP6_PREFIX_ENTRY *PREFIX6_Match_Longest_Prefix(UINT8 *target)
{
    IP6_PREFIX_ENTRY    *target_prefix = NU_NULL;
    DV_DEVICE_ENTRY     *dv_ptr;

    dv_ptr = DEV_Table.dv_head;

    while (dv_ptr)
    {
        if (dv_ptr->dev_flags & DV6_IPV6)
        {
            target_prefix = 
                PREFIX6_Match_Longest_Prefix_By_Device(dv_ptr->dev6_prefix_list, 
                                                       target);
    
            if (target_prefix != NU_NULL)
                break;
        }

        dv_ptr = dv_ptr->dev_next;
    }

    /* Return a pointer to the longest matching Prefix or NU_NULL if no
     * prefixes match the target.
     */
    return (target_prefix);

} /* PREFIX6_Match_Longest_Prefix */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       PREFIX6_Init_Prefix_List
*                                                                         
*   DESCRIPTION                                                           
*                 
*       This function allocates memory for a Prefix List and initializes
*       the memory block to all zeros.                                            
*                                                                        
*   INPUTS                                                                
*                                                        
*       **prefix_list           A pointer to allocate a Prefix List for.
*                                                                         
*   OUTPUTS                                                               
*                                         
*       NU_SUCCESS              The memory was allocated.
*       NU_NO_MEMORY            Insufficient memory.
*
*************************************************************************/
STATUS PREFIX6_Init_Prefix_List(IP6_PREFIX_LIST **prefix_list)
{
    STATUS  status;

    /* Allocate memory for the Prefix List */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)prefix_list,
                                sizeof(IP6_PREFIX_LIST), NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Zero out the Prefix List */
        UTL_Zero(*prefix_list, sizeof(IP6_PREFIX_LIST));
    }

    return (status);

} /* PREFIX6_Init_Prefix_List */

#if (INCLUDE_DHCP6 == NU_TRUE)

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       PREFIX6_Find_On_Link_Prefix
*                                                                         
*   DESCRIPTION                                                           
*              
*       This function finds a Prefix List entry that is not the link-local
*       prefix.
*                                                                         
*   INPUTS                                                                
*                          
*       *device                 The interface on which to search for the 
*                               Prefix.
*                                                                         
*   OUTPUTS                                                               
*                                     
*       A pointer to the Prefix List entry or NU_NULL if no entry
*       exists.                                    
*
*************************************************************************/
IP6_PREFIX_ENTRY *PREFIX6_Find_On_Link_Prefix(const DV_DEVICE_ENTRY *device)
{
    IP6_PREFIX_ENTRY    *current_entry;

    /* Get a pointer to the first Prefix List entry in the list */
    current_entry = device->dev6_prefix_list->dv_head;

    /* Traverse the Prefix list until we reach the end of the list or 
     * find a prefix other than the link-local.
     */
    while (current_entry != NU_NULL)
    {
        /* If this prefix is not the link-local prefix. */
        if ( (current_entry->ip6_prfx_lst_prefix[0] != 0xfe) &&
             (current_entry->ip6_prfx_lst_prefix[1] != 0x80) )
            break;

        /* Get a pointer to the next entry in the list */
        current_entry = current_entry->ip6_prefx_lst_next;
    }

    /* Return a pointer to the Prefix List entry or NU_NULL if no
     * entry was found.
     */
    return (current_entry);

} /* PREFIX6_Find_On_Link_Prefix */

#endif
