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
*       rtab4_fnr.c
*
*   COMPONENT
*
*       IPv4 Routing
*
*   DESCRIPTION
*
*       This module contains the functions for finding the next route
*       in lexicographical order.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB4_Find_Next_Route
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/* The top or root of the route table tree. */
extern ROUTE_NODE   *RTAB4_Root_Node;

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Find_Next_Route
*
*   DESCRIPTION
*
*       This function finds the route in the routing tree proceeding the
*       provided route.
*
*   INPUTS
*
*       *current_address        A pointer to the address of the route
*                               of which the next route is desired
*
*   OUTPUTS
*
*       ROUTE_NODE*             A pointer to the route.
*       NU_NULL                 There is no next route.
*
*************************************************************************/
RTAB4_ROUTE_ENTRY *RTAB4_Find_Next_Route(const UINT8 *current_address)
{
    ROUTE_NODE          *current_node;
    RTAB4_ROUTE_ENTRY   *rt_entry = NU_NULL;
    INT                 direction = 0;
    UINT32              target_address;

    target_address = IP_ADDR(current_address);

    current_node = RTAB4_Root_Node;

    while (current_node)
    {
        /* If the current IP address is greater than the target IP address
         * and the next_node is either NULL or the current IP address is
         * less than the next_node, save the current IP address as the
         * next_node.
         */
        if ( (IP_ADDR(current_node->rt_ip_addr) > target_address) &&
             ((rt_entry == NU_NULL) ||
              (IP_ADDR(current_node->rt_ip_addr) <
               IP_ADDR(rt_entry->rt_route_node->rt_ip_addr))) )
        {
            /* If the route is up, save this entry */
            if (((RTAB4_ROUTE_ENTRY*)(current_node->rt_list_head))
                ->rt_entry_parms.rt_parm_flags & RT_UP)
                rt_entry = current_node->rt_list_head;
        }

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
                if ( (current_node != RTAB4_Root_Node) &&
                     (current_node == current_node->rt_parent->rt_child[0]) )
                    current_node = current_node->rt_parent;

                /* Otherwise, continue to go up one parent level until reaching
                 * the Root Node or a left child.
                 */
                else
                {
                    while ( (current_node != RTAB4_Root_Node) &&
                            (current_node != current_node->rt_parent->rt_child[0]) )
                        current_node = current_node->rt_parent;

                    /* Reached the Root Node and have traversed the entire
                     * tree.
                     */
                    if (current_node == RTAB4_Root_Node)
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
            /* If the next child to traverse is not pointing upward in the tree,
             * take the branch.
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

} /* RTAB4_Find_Next_Route */

#endif
