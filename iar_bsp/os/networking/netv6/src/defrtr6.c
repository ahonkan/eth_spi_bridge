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
*       defrtr6.c                                   
*                                                                               
*   DESCRIPTION                                                           
*                     
*       This file contains those functions necessary to manage the list
*       of Default Routers attached to the link.                                                  
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*           
*       DEFRTR6_Create_Default_Router_Entry
*       DEFRTR6_Expire_Default_Router_Entry
*       DEFRTR6_Delete_Default_Router
*       DEFRTR6_Delete_Entry
*       DEFRTR6_Find_Default_Router_Entry
*       DEFRTR6_Update_Default_Router_List
*       DEFRTR6_Find_Default_Router
*       DEFRTR6_Get_Router_By_Index
*                                                                       
*   DEPENDENCIES                                                          
*               
*       target.h                                                        
*       externs.h
*       defrtr6.h
*       nc6.h
*                                                                       
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/defrtr6.h"
#include "networking/nc6.h"

IP6_DEFAULT_ROUTER_LIST     *Default_Router_List = NU_NULL;
static  UINT32              Default_Router_Index = 1;

extern TQ_EVENT    DEFRTR6_Expire_Entry_Event;

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       DEFRTR6_Create_Default_Router_Entry
*                                                                         
*   DESCRIPTION                                                           
*           
*       This function adds a Default Router to the list of Default
*       Routers for the link.                                                              
*                                                                         
*   INPUTS                                                                
*                            
*       *device                 A pointer to the interface on which whose 
*                               link the Default Router exists.
*       *source_address         The IP address of the Default Router.
*       lifetime                The lifetime of the Default Router.                                        
*                                                                         
*   OUTPUTS                                                               
*                                                              
*       A pointer to the new entry or NU_NULL if the entry could not be
*       created.           
*
*************************************************************************/
IP6_DEFAULT_ROUTER_ENTRY *DEFRTR6_Create_Default_Router_Entry(DV_DEVICE_ENTRY *device,
                                                              const UINT8 *source_addr, 
                                                              UINT16 lifetime)
{
    IP6_DEFAULT_ROUTER_ENTRY    *new_entry = NU_NULL;
   
    /* If there is enough memory to create a new entry, create the entry,
     * initialize its parameters, and link it into the list.  Otherwise,
     * return -1.
     */
    if (NU_Allocate_Memory(MEM_Cached, (VOID**)&new_entry, 
                           sizeof(IP6_DEFAULT_ROUTER_ENTRY), 
                           NU_NO_SUSPEND) == NU_SUCCESS)
    {
        UTL_Zero(new_entry, sizeof(IP6_DEFAULT_ROUTER_ENTRY));
    
        /* Set up the new entry */
        NU_BLOCK_COPY(new_entry->ip6_def_rtr_ip_addr, source_addr, IP6_ADDR_LEN);
        new_entry->ip6_def_rtr_device = device;

        new_entry->ip6_def_rtr_inval_timer = lifetime;
        new_entry->ip6_def_rtr_index = Default_Router_Index++;

        /* Set the neighbor cache entry parameter to the corresponding Neighbor
         * Cache entry if one already exists.
         */
        new_entry->ip6_def_rtr_nc_entry = 
            device->dev6_fnd_neighcache_entry(device, source_addr);

        /* Set the default router parameter of the Neighbor Cache entry to the
         * corresponding Default Router List entry.
         */
        if (new_entry->ip6_def_rtr_nc_entry)
            new_entry->ip6_def_rtr_nc_entry->ip6_neigh_cache_def_rtr = new_entry;

        /* Set a timer to delete the Default Router entry upon expiration.
         * RFC 4861 section 4.2 - The field can contain values up to 65535 and
         * receivers should handle any value.
         * So, do not check for infinite lifetime.
         */
        if (TQ_Timerset(DEFRTR6_Expire_Entry_Event, new_entry->ip6_def_rtr_index, 
                        (UNSIGNED)(lifetime * TICKS_PER_SECOND), 0) != NU_SUCCESS)
            NLOG_Error_Log("Failed to start the timer to expire the Default Router entry", 
                           NERR_SEVERE, __FILE__, __LINE__);

        /* If this is the first entry to add to the Default Router List,
         * set it as the first entry to select when choosing a Default
         * Router from the list.
         */
        if (Default_Router_List->dv_head == NU_NULL)
            new_entry->ip6_def_rtr_use_next = 1;

        DLL_Enqueue(Default_Router_List, new_entry);
    }
    else
    {
        NLOG_Error_Log("Cannot create a Default Router Entry due to lack of memory", 
                       NERR_SEVERE, __FILE__, __LINE__);
    }    

    return (new_entry);

} /* DEFRTR6_Create_Default_Router_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       DEFRTR6_Expire_Default_Router_Entry
*                                                                         
*   DESCRIPTION                                                           
*               
*       This function expires a default router.   
*                                                                         
*   INPUTS                                                   
*             
*       event                   The event that is being handled.
*       index                   The index value of the Default Router 
*                               to expire.
*       extra_data              Unused parameter.
*                                                                         
*   OUTPUTS                                                               
*                                               
*       None                          
*
*************************************************************************/
VOID DEFRTR6_Expire_Default_Router_Entry(TQ_EVENT event, UNSIGNED index,
                                         UNSIGNED extra_data)
{
    IP6_DEFAULT_ROUTER_ENTRY *current_entry;

    UNUSED_PARAMETER(event);
    UNUSED_PARAMETER(extra_data);

    /* Get a pointer to the Default Router being deleted. */
    current_entry = DEFRTR6_Get_Router_By_Index(index);

    /* If the Default Router is in the list, delete it. */
    if (current_entry)
        DEFRTR6_Delete_Entry(current_entry);

} /* DEFRTR6_Expire_Default_Router_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       DEFRTR6_Delete_Default_Router
*                                                                         
*   DESCRIPTION                                                           
*               
*       This function removes a default router associated with the given
*       IP address from the list of Default Routers.                                                          
*                                                                         
*   INPUTS                                                   
*             
*       *ip_addr                A pointer to the IP address of the 
*                               Default Router to remove from the list.                                                                         
*                                                                         
*   OUTPUTS                                                               
*                                               
*       None                          
*
*************************************************************************/
VOID DEFRTR6_Delete_Default_Router(const UINT8 *ip_addr)
{
    IP6_DEFAULT_ROUTER_ENTRY    *current_entry;

    /* Get a pointer to the corresponding entry */
    current_entry = DEFRTR6_Find_Default_Router_Entry(ip_addr);

    /* If an entry exists, delete it */
    if (current_entry != NU_NULL)
        DEFRTR6_Delete_Entry(current_entry);

} /* DEFRTR6_Delete_Default_Router */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       DEFRTR6_Delete_Entry
*                                                                         
*   DESCRIPTION                                                           
*               
*       This function removes a default router from the list of Default
*       Routers.                                                          
*                                                                         
*   INPUTS                                                   
*             
*       *rtr_entry              A pointer to the entry to delete.                                                
*                                                                         
*   OUTPUTS                                                               
*                                               
*       None                          
*
*************************************************************************/
VOID DEFRTR6_Delete_Entry(IP6_DEFAULT_ROUTER_ENTRY *rtr_entry)
{
    /* If an entry exists, delete it */
    if (rtr_entry)
    {
    	/* Clear the expiration timer. */
        if (TQ_Timerunset(DEFRTR6_Expire_Entry_Event, TQ_CLEAR_EXACT,
        		          rtr_entry->ip6_def_rtr_index, 0) != NU_SUCCESS)
            NLOG_Error_Log("Failed to stop the timer to expire the Default Router entry",
                           NERR_SEVERE, __FILE__, __LINE__);

        /* RFC 4861 section 6.3.5 - Whenever the Lifetime of an entry in
         * the Default Router List expires, that entry is discarded.  When
         * removing a router from the Default Router List, the node MUST
         * update the routing table in such a way that all entries 
         * using the router perform Next-Hop determination again rather
         * than continue sending traffic to the (deleted) router.
         */
        if (rtr_entry->ip6_def_rtr_nc_entry)
            rtr_entry->ip6_def_rtr_device->
                dev6_del_neighcache_entry(rtr_entry->ip6_def_rtr_device, 
                                          rtr_entry->ip6_def_rtr_ip_addr);

        /* If this entry is the next entry to be selected, update the
         * list to select a new next entry.
         */
        if (rtr_entry->ip6_def_rtr_use_next == 1)
            DEFRTR6_Update_Default_Router_List(rtr_entry, 1);

        /* Remove the entry from the list */
        DLL_Remove(Default_Router_List, rtr_entry);

        /* Delete all the routes that use this node as the gateway */
        RTAB6_Delete_Route_By_Gateway(rtr_entry->ip6_def_rtr_ip_addr);

        /* Deallocate the memory being used by this entry */
        if (NU_Deallocate_Memory(rtr_entry) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate the memory for the Default Router entry", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }

} /* DEFRTR6_Delete_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       DEFRTR6_Find_Default_Router_Entry
*                                                                         
*   DESCRIPTION                                                           
*           
*       This function finds a Default Router associated with the 
*       provided IP address.                                                              
*                                                                         
*   INPUTS                                                                
*                           
*       *ip_address             The IP address of the target Default 
*                               Router.                                              
*                                                                         
*   OUTPUTS                                                               
*                                                                   
*       A pointer to the Default Router or NU_NULL if no entry exists.      
*
*************************************************************************/
IP6_DEFAULT_ROUTER_ENTRY *DEFRTR6_Find_Default_Router_Entry(const UINT8 *ip_address)
{
    IP6_DEFAULT_ROUTER_ENTRY    *current_entry = NU_NULL;

    /* If the Default Router List and IP address are not NULL, find the 
     * entry.
     */
    if ( (Default_Router_List) && (ip_address) )
    {
        /* Get a pointer to the head of the list */
        current_entry = Default_Router_List->dv_head;

        /* Walk the list until the end of the list is reached or the 
         * target entry is found.
         */
        while (current_entry)
        {
            /* If the IP address of the entry matches the IP address
             * provided, break.
             */
            if (memcmp(current_entry->ip6_def_rtr_ip_addr, ip_address, 
                       IP6_ADDR_LEN) == 0)
                break;

            /* Get the next entry in the list */
            current_entry = current_entry->ip6_def_rtr_next;
        }
    }

    /* Return a pointer to the entry or NULL if no entry was found */
    return (current_entry);

} /* DEFRTR6_Find_Default_Router_Entry */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       DEFRTR6_Update_Default_Router_List
*                                                                         
*   DESCRIPTION                                                           
*                    
*       This function updates the "next" Default Router to use to 
*       maintain fairness via the Round Robin technique.         
*                                                                         
*   INPUTS                                                                
*                                                       
*       *old_next               The previous "next" Default Router.                  
*                                                                         
*   OUTPUTS                                                               
*                                                              
*       None           
*
*************************************************************************/
VOID DEFRTR6_Update_Default_Router_List(IP6_DEFAULT_ROUTER_ENTRY *old_next,
                                        UINT8 deleting)
{
    IP6_DEFAULT_ROUTER_ENTRY    *new_next, *saved_entry;

    /* Search through the list for the next reachable entry, and
     * flag it as the next entry to be used.  If there is not a
     * reachable entry, set the next entry to be used to the entry
     * after the current next entry.
     */
    old_next->ip6_def_rtr_use_next = 0;

    /* Get a pointer to the next entry */
    new_next = old_next->ip6_def_rtr_next;

    /* If the next entry is NULL, loop around to the head of the list */
    if (new_next == NU_NULL)
        new_next = Default_Router_List->dv_head;

    /* Set the next entry as the next entry to use */
    new_next->ip6_def_rtr_use_next = 1;

    /* If this entry is in the INCOMPLETE state or does not have a
     * Neighbor Cache entry, search through the list for a better 
     * entry.
     */
    if ( ((new_next->ip6_def_rtr_nc_entry == NU_NULL) ||
          (new_next->ip6_def_rtr_nc_entry->ip6_neigh_cache_state == NC_NEIGH_INCOMPLETE)) &&
         ((old_next->ip6_def_rtr_nc_entry != NU_NULL) &&
          (old_next->ip6_def_rtr_nc_entry->ip6_neigh_cache_state != NC_NEIGH_INCOMPLETE)) )
    {
        /* Save off a pointer to the entry */
        saved_entry = new_next;

        /* Get a pointer to the next entry */
        new_next = new_next->ip6_def_rtr_next;

        /* If the next entry is NULL, loop around to the beginning of the list */
        if (new_next == NU_NULL)
            new_next = Default_Router_List->dv_head;

        /* Search the list for the best new next entry */
        while (new_next != saved_entry)
        {
            /* If the state of the entry is not INCOMPLETE and a
             * Neighbor Cache entry exists, this entry is better. 
             */
            if ( (new_next->ip6_def_rtr_nc_entry != NU_NULL) &&
                 (new_next->ip6_def_rtr_nc_entry->ip6_neigh_cache_state != NC_NEIGH_INCOMPLETE) )
            {
                /* Set this entry to the next entry to use */
                new_next->ip6_def_rtr_use_next = 1;
                
                /* Reset the entry previously marked to be used as
                 * the next entry.  A better next has been found.
                 */
                saved_entry->ip6_def_rtr_use_next = 0;
                break;
            }

            /* Get a pointer to the next entry */
            new_next = new_next->ip6_def_rtr_next;

            /* Wrap back to the head of the list if necessary */
            if (new_next == NU_NULL)
                new_next = Default_Router_List->dv_head;
        }

        /* Don't set the same one that is being unset if the one being unset
         * is also being deleted. 
         */
        if ( (new_next == old_next) && (deleting == 1) )
        {
            /* Unset this entry as the next entry to use */
            new_next->ip6_def_rtr_use_next = 0;

            /* Set the next one after the old one to be used next. */
            saved_entry->ip6_def_rtr_use_next = 1;
        }
    }

} /* DEFRTR6_Update_Default_Router_List */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       DEFRTR6_Find_Default_Router
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function selects the best Default Router from the list of
*       Default Routers for the specified device as per RFC 4861 section
*       6.3.6.  Routers are selected in a round-robin manner.  Routers
*       that are known to be reachable are preferred.
*                                                                        
*   INPUTS                                                                
*           
*       None                                                              
*                                                                         
*   OUTPUTS                                                               
*           
*       A pointer to the Default Router entry or NU_NULL if the list is
*       empty.                                                
*
*************************************************************************/
IP6_DEFAULT_ROUTER_ENTRY *DEFRTR6_Find_Default_Router(VOID)
{
    IP6_DEFAULT_ROUTER_ENTRY    *current_entry = NU_NULL;
    IP6_DEFAULT_ROUTER_ENTRY    *old_next = NU_NULL;

    /* If the Default Router List is not NULL, return a pointer to the
     * next Default Router flagged to be used.
     */
    if (Default_Router_List)
    {
        /* Get a pointer to the first entry in the list */
        current_entry = Default_Router_List->dv_head;
    
        /* Search through the list of Default Routers for the next entry
         * flagged to be used.  Once the entry is found, flag the next entry
         * to be used.
         */ 
        while (current_entry)
        {
            /* If this is the next entry to be used, find the new next entry 
             * to be used.
             */
            if (current_entry->ip6_def_rtr_use_next)
            {
                /* If the Neighbor Cache entry for this Default Router is
                 * valid, set this entry to be used.
                 */
                if ( (current_entry->ip6_def_rtr_nc_entry) &&
                     (current_entry->ip6_def_rtr_nc_entry->ip6_neigh_cache_state != 
                      NC_NEIGH_INCOMPLETE) )
                    DEFRTR6_Update_Default_Router_List(current_entry, 0);
                else
                    old_next = current_entry;

                break;
            }

            /* Get a pointer to the next entry */
            current_entry = current_entry->ip6_def_rtr_next;
        }

        /* The Default Router flagged as the next to be used
         * does not have a valid Neighbor Cache entry.
         * Search for the first Default Router entry with a valid
         * Neighbor Cache entry.
         */
        if (old_next)
        {
            current_entry = Default_Router_List->dv_head;

            while (current_entry)
            {
                /* If this entry has a valid Neighbor Cache entry,
                 * use it.
                 */
                if ( (current_entry->ip6_def_rtr_nc_entry) &&
                     (current_entry->ip6_def_rtr_nc_entry->ip6_neigh_cache_state != 
                      NC_NEIGH_INCOMPLETE) )
                    break;

                /* Get a pointer to the next entry */
                current_entry = current_entry->ip6_def_rtr_next;
            }

            /* If a valid entry was not found, revert back to the entry
             * flagged as next.
             */
            if (current_entry == NU_NULL)
            {
                current_entry = old_next;
                DEFRTR6_Update_Default_Router_List(current_entry, 0);
            }
        }
    }

    /* Return a pointer to the entry or NU_NULL if the Default Router List
     * is empty.
     */
    return (current_entry);

} /* DEFRTR6_Find_Default_Router */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       DEFRTR6_Get_Router_By_Index
*                                                                         
*   DESCRIPTION                                                           
*           
*       This function finds a Default Router associated with the 
*       specified index.                                     
*                                                                         
*   INPUTS                                                                
*                           
*       index               The index of the target default router.                                             
*                                                                         
*   OUTPUTS                                                               
*                                                                   
*       A pointer to the Default Router or NU_NULL if no entry exists.      
*
*************************************************************************/
IP6_DEFAULT_ROUTER_ENTRY *DEFRTR6_Get_Router_By_Index(const INT index)
{
    IP6_DEFAULT_ROUTER_ENTRY    *current_entry = NU_NULL;

    /* If the Default Router List is not NULL, find the entry. */
    if (Default_Router_List)
    {
        /* Get a pointer to the head of the list */
        current_entry = Default_Router_List->dv_head;

        /* Walk the list until the end of the list is reached or the 
         * target entry is found.
         */
        while (current_entry)
        {
            /* If the index of the entry matches the index provided. */
            if (current_entry->ip6_def_rtr_index == index)
                break;

            /* Get the next entry in the list */
            current_entry = current_entry->ip6_def_rtr_next;
        }
    }

    /* Return a pointer to the entry or NULL if no entry was found */
    return (current_entry);

} /* DEFRTR6_Get_Router_By_Index */
