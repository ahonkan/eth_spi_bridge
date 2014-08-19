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
*       rtab6_dn.c                                   
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This module contains the function for deleting an IPv6 route
*       node.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*                                                                       
*       RTAB6_Delete_Node
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*       nc6.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/nc6.h"

extern ROUTE_NODE *RTAB6_Default_Route;
extern RTAB_ROUTE_PARMS RTAB6_Parms;

/*************************************************************************
*
*   FUNCTION
*
*       RTAB6_Delete_Node
*
*   DESCRIPTION
*
*       This function deletes a node from the IPv6 routing table.
*
*   INPUTS
*
*       *rt_node                A pointer to the ROUTE_NODE to delete.
*
*   OUTPUTS
*
*       NU_SUCCESS              The route was successfully deleted
*       NU_INVALID_ADDRESS      The route was not found
*       NU_INVALID_PARM         A pointer parameter is NU_NULL.
*
*************************************************************************/
STATUS RTAB6_Delete_Node(ROUTE_NODE *rt_node)
{
    STATUS  status;

    /* If the node to delete is the Default Route */
    if (rt_node == RTAB6_Default_Route)
    {
        status = NU_SUCCESS;

        /* If the route is still in use, flag it as being down */
        if (((ROUTE_ENTRY*)(rt_node->rt_list_head))
                ->rt_entry_parms.rt_parm_refcnt != 0)
        {
            ((ROUTE_ENTRY*)(rt_node->rt_list_head))
                ->rt_entry_parms.rt_parm_flags &= ~RT_UP;
        }

        /* Delete the route */
        else
        {           
            /* Delete the Default Route from the list of routes for 
             * the Neighbor Cache entry.
             */
            if (((RTAB6_ROUTE_ENTRY*)(RTAB6_Default_Route->rt_list_head))
                    ->rt_next_hop_entry)
                RTAB6_Delete_DestList_Entry(&((RTAB6_ROUTE_ENTRY*)
                                            (RTAB6_Default_Route->
                                            rt_list_head))
                                            ->rt_next_hop_entry->ip6_neigh_cache_dest_list, 
                                            RTAB6_Default_Route->rt_list_head);

            RTAB6_Default_Route = NU_NULL;

            /* Deallocate the memory used by the Default Route */
            if (NU_Deallocate_Memory(rt_node->rt_list_head) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for default route", 
                               NERR_FATAL, __FILE__, __LINE__);

            if (NU_Deallocate_Memory(rt_node) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for default route", 
                               NERR_FATAL, __FILE__, __LINE__);
        }
    }

    else
        status = RTAB_Delete_Node(rt_node, &RTAB6_Parms);

    return (status);

} /* RTAB6_Delete_Node */
