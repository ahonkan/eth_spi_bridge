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
*       rtab6_dl.c                                   
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This module contains the functions for maintaining the link 
*       between the routing table and Neighbor Cache.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*                                                                       
*   FUNCTIONS                                                             
*           
*       RTAB6_Add_DestList_Entry
*       RTAB6_Find_DestList_Entry
*       RTAB6_Delete_DestList_Entry
*       RTAB6_Find_Next_Hop_Entry
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

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Add_DestList_Entry
*                                                                       
*   DESCRIPTION                                                           
*                               
*       This function adds a route to the list of route entries.                                        
*                                                                       
*   INPUTS                                                                
*           
*       *dc_list                A pointer to the list of route entries 
*                               associated with the Neighbor Cache entry.
*       *rt_entry               A pointer to the route entry to add to the 
*                               list.                                                            
*                                                                       
*   OUTPUTS                                                               
*                                       
*       None                                
*                                                                       
*************************************************************************/
VOID RTAB6_Add_DestList_Entry(IP6_DEST_LIST *dc_list, 
                              RTAB6_ROUTE_ENTRY *rt_entry)
{
    IP6_DEST_LIST_ENTRY *dc_list_entry;
    STATUS              status;

    /* Allocate memory for the Destination entry */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&dc_list_entry,
                           sizeof(IP6_DEST_LIST_ENTRY), NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Zero out the entry */
        UTL_Zero(dc_list_entry, sizeof(IP6_DEST_LIST_ENTRY));

        /* Set the entry */
        dc_list_entry->ip6_dest_list_entry = rt_entry;

        DLL_Enqueue(dc_list, dc_list_entry);
    }
    else
        NLOG_Error_Log("Cannot create IPv6 Dest List entry due to lack of memory", 
                       NERR_SEVERE, __FILE__, __LINE__);

} /* RTAB6_Add_DestList_Entry */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Find_DestList_Entry
*                                                                       
*   DESCRIPTION                                                           
*                               
*       This function finds a target route entry in the list of route 
*       entries.                                        
*                                                                       
*   INPUTS                                                                
*           
*       *dc_list                A pointer to the list of route entries 
*                               associated with the Neighbor Cache entry.
*       *rt_entry               A pointer to the target route entry.
*                                                                       
*   OUTPUTS                                                               
*                                       
*       A pointer to the target entry or NU_NULL if the entry was not
*       found.
*                                                                       
*************************************************************************/
IP6_DEST_LIST_ENTRY *RTAB6_Find_DestList_Entry(const IP6_DEST_LIST *dc_list, 
                                               const RTAB6_ROUTE_ENTRY *rt_entry)
{
    IP6_DEST_LIST_ENTRY *dc_list_entry;

    /* Get a pointer to the first entry in the list */
    dc_list_entry = dc_list->dv_head;

    /* Traverse the list until the target entry is found or the end of
     * the list.
     */
    while (dc_list_entry)
    {
        if (dc_list_entry->ip6_dest_list_entry == rt_entry)
            break;

        /* Get a pointer to the next entry in the list */
        dc_list_entry = dc_list_entry->ip6_dest_list_entry_next;
    }

    /* Return a pointer to the target entry or NU_NULL if the entry was
     * not found.
     */
    return (dc_list_entry);

} /* RTAB6_Find_DestList_Entry */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Delete_DestList_Entry
*                                                                       
*   DESCRIPTION                                                           
*                               
*       This function finds and deletes a given route entry from a list.
*                                                                       
*   INPUTS                                                                
*         
*       *dc_list                A pointer to the list of route entries 
*                               associated with the Neighbor Cache entry.
*       *rt_entry               A pointer to the route entry to remove 
*                               from the list.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID RTAB6_Delete_DestList_Entry(IP6_DEST_LIST *dc_list,
                                 const RTAB6_ROUTE_ENTRY *rt_entry)
{
    IP6_DEST_LIST_ENTRY *dc_list_entry;

    /* Get a pointer to the target entry */
    dc_list_entry = RTAB6_Find_DestList_Entry(dc_list, rt_entry);

    /* If the entry exists */
    if (dc_list_entry)
    {
        /* Remove this route entry from the list of entries for the
         * Neighbor Cache entry.
         */
        DLL_Remove(dc_list, dc_list_entry);

        if (NU_Deallocate_Memory((VOID*)dc_list_entry) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for the node", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }   

} /* RTAB6_Delete_DestList_Entry */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Find_Next_Hop_Entry                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function searches a route for a given Neighbor Cache
*       entry.
*                                                                       
*   INPUTS                                                                
*        
*       *rt_entry               A pointer to one of the route entries for 
*                               the route.
*       *nc_entry               A pointer to the Neighbor Cache entry to 
*                               find.
*                                                                       
*   OUTPUTS                                                               
*    
*       If successful, this function returns a pointer to the route
*       entry whose next-hop is the passed in Neighbor Cache entry.
*
*       Otherwise, this function returns NU_NULL;
*                                                                       
*************************************************************************/
RTAB6_ROUTE_ENTRY *RTAB6_Find_Next_Hop_Entry(const RTAB6_ROUTE_ENTRY *rt_entry, 
                                             const IP6_NEIGHBOR_CACHE_ENTRY *nc_entry)
{
    RTAB6_ROUTE_ENTRY   *current_rt_entry = NU_NULL;

    /* If both parameters are valid, find the next-hop entry */
    if ( (rt_entry != NU_NULL) && (nc_entry != NU_NULL) )
    {
        /* Get a pointer to the first route entry for the route. */
        current_rt_entry = rt_entry->rt_route_node->rt_list_head;

        /* Traverse the list while there are route entries and the 
         * target has not been found.
         */
        while (current_rt_entry)
        {
            /* If this is the target next-hop, break */
            if (current_rt_entry->rt_next_hop_entry == nc_entry)
                break;

            current_rt_entry = current_rt_entry->rt_entry_next;
        }
    }

    /* Return a pointer to the Destination Cache entry */
    return (current_rt_entry);

} /* RTAB6_Find_Next_Hop_Entry */
