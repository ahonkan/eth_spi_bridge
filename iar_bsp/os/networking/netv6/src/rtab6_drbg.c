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
*       rtab6_drbg.c                                 
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This module contains the routines to find and delete all routes
*       using a specified gateway.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*                                                                       
*       RTAB6_Delete_Route_By_Gateway
*       RTAB6_Find_Route_For_Gateway
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"

extern ROUTE_NODE           *RTAB6_Root_Node;
extern RTAB_ROUTE_PARMS     RTAB6_Parms;

RTAB6_ROUTE_ENTRY *RTAB6_Find_Route_For_Gateway(UINT8 *);

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       RTAB6_Delete_Route_By_Gateway
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This routine deletes all routes that use the specified node as
*       the gateway (next-hop).
*                                                                         
*   INPUTS                                                                
*                                                 
*       *gateway_addr           The address of the gateway of the route
*                               to find and delete.
*                                                 
*   OUTPUTS                                                               
*                                                                         
*       None.
*
*************************************************************************/
VOID RTAB6_Delete_Route_By_Gateway(UINT8 *gateway_addr)
{
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Find the first route in the routing table that uses this node as
     * the next-hop.
     */
    rt_entry = RTAB6_Find_Route_For_Gateway(gateway_addr);

    /* While there are routes left to process. */
    while (rt_entry)
    {
        /* Delete the route */
        RTAB_Delete_Route_Entry(rt_entry->rt_route_node, 
                                 (ROUTE_ENTRY*)rt_entry, 
                                 &RTAB6_Parms);

        /* Find the next route in the routing table that uses this node
         * as the next-hop.
         */
        rt_entry = RTAB6_Find_Route_For_Gateway(gateway_addr);
    }

} /* RTAB6_Delete_Route_By_Gateway */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       RTAB6_Find_Route_For_Gateway
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This routine finds a route in the Routing Table that uses the 
*       specified address as the gateway (next-hop).
*                                                                         
*   INPUTS                                                                
*                                                 
*       *gateway_addr           The address of the gateway of the route
*                               to find.
*                                                 
*   OUTPUTS                                                               
*                                                                         
*       RTAB6_ROUTE_ENTRY*      A pointer to the route that uses this 
*                               node as the next-hop.
*       NU_NULL                 No route exists.
*
*************************************************************************/
RTAB6_ROUTE_ENTRY *RTAB6_Find_Route_For_Gateway(UINT8 *gateway_addr)
{
    ROUTE_NODE              *current_node;
    INT                     direction = 0;
    RTAB6_ROUTE_ENTRY       *rt_entry = NU_NULL;

    current_node = RTAB6_Root_Node;

    /* Process each node in the Routing Table */
    while (current_node)
    {
        rt_entry = (RTAB6_ROUTE_ENTRY*)current_node->rt_list_head;

        /* Process each route in the route entry.  There could be multiple
         * routes to the same destination, each using a different next-hop.
         */
        do
        {
            /* Check if we found a route using the gateway. */
            if ( (rt_entry->rt_entry_parms.rt_parm_flags & RT_UP) &&
                 (memcmp(rt_entry->rt_next_hop.sck_addr, gateway_addr, 
                         IP6_ADDR_LEN) == 0) )
                break;

            rt_entry = rt_entry->rt_entry_next;

        } while (rt_entry);

        /* If a route was found, break out of the loop and return it. */
        if (rt_entry)
            break;

        /* If the left child is pointing to an upward node or is NULL, 
         * take the right branch.
         */
        if ( (current_node->rt_child[direction] == NU_NULL) ||
             (current_node->rt_child[direction]->rt_bit_index >= 
              current_node->rt_bit_index) )
        {
            /* Change the direction to right */
            direction = 1;

            /* If the right node is pointing to an upward link, backtrack - 
             * the right node can never point to NULL.
             */
            if (current_node->rt_child[direction]->rt_bit_index >= 
                current_node->rt_bit_index)
            {
                /* If the node is a left child, go up one parent level */
                if ( (current_node != RTAB6_Root_Node) &&
                     (current_node == current_node->rt_parent->rt_child[0]) )
                    current_node = current_node->rt_parent;

                /* Otherwise, continue to go up one parent level until 
                 * reaching the Root Node or a left child.
                 */
                else
                {
                    while ( (current_node != RTAB6_Root_Node) &&
                            (current_node != current_node->rt_parent->rt_child[0]) )
                        current_node = current_node->rt_parent; 

                    /* Reached the Root Node and have traversed the entire 
                     * tree.
                     */
                    if (current_node == RTAB6_Root_Node)
                    {
                        break;
                    }

                    /* Go up one more parent level */
                    current_node = current_node->rt_parent; 
                }
            }
        }
 
        /* If the next node is not NULL */
        if (current_node->rt_child[direction] != NU_NULL)
        {
            /* If the next child to traverse is not pointing upward in 
             * the tree, take the branch.
             */
            if (current_node->rt_bit_index > 
                current_node->rt_child[direction]->rt_bit_index)
            {
                /* Go to the next node */
                current_node = current_node->rt_child[direction];

                /* Set the direction to left */
                direction = 0;
            }
        }
    }

    return (rt_entry);

} /* RTAB6_Find_Route_For_Gateway */
