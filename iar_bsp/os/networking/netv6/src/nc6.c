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
*       nc6.c                                        
*                                                                               
*   DESCRIPTION                                                           
*                                                                    
*       This file contains those functions necessary to maintain the
*       link-generic Neighbor Cache for an IPv6 node.   
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*           
*       NC6_Transition_To_Router
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       externs.h
*       nc6.h
*       defrtr6.h
*                                                                       
*************************************************************************/

#include "networking/externs.h"
#include "networking/nc6.h"
#include "networking/defrtr6.h"

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       NC6_Transition_To_Router
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function changes the state of a Neighbor Cache entry from 
*       not being a router to being a router, and establishes the 
*       necessary link between the Default Router List and the
*       Neighbor Cache.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *nc_entry               A pointer to the Neighbor Cache entry.
*                                                                         
*   OUTPUTS                                                               
*                                     
*       None                                    
*
*************************************************************************/
VOID NC6_Transition_To_Router(IP6_NEIGHBOR_CACHE_ENTRY *nc_entry)
{
    /* If the entry is transitioning from not being a router to being a router,
     * link the corresponding Neighbor Cache entry and Default Router List entry.
     */
    if (!(nc_entry->ip6_neigh_cache_flags & NC_ISROUTER))
    {
        nc_entry->ip6_neigh_cache_def_rtr = 
            DEFRTR6_Find_Default_Router_Entry(nc_entry->ip6_neigh_cache_ip_addr);

        /* Set the neighbor cache parameter of the Default Router List entry
         * to the corresponding Neighbor Cache entry.
         */
        if (nc_entry->ip6_neigh_cache_def_rtr)
            nc_entry->ip6_neigh_cache_def_rtr->ip6_def_rtr_nc_entry = nc_entry;

        nc_entry->ip6_neigh_cache_flags |= NC_ISROUTER;
    }        

} /* NC6_Transition_To_Router */
