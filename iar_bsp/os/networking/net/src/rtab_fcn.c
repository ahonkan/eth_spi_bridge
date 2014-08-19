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
*       rtab_fcn.c
*
*   COMPONENT
*
*       Routing
*
*   DESCRIPTION
*
*       This module contains the functions for finding the closest node
*       in the tree to the target node.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB_Find_Closest_Node
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Find_Closest_Node
*
*   DESCRIPTION
*
*       This function finds the node pointing to the node associated
*       with the provided IP address.
*
*   INPUTS
*
*       *ip_addr                A pointer to the IP address of the new
*                               node.
*       *direction              A pointer to the direction in relation
*                               to the returned closest node that we
*                               had to branch to reach the closest node
*       *rt_parms               A pointer to data specific to the family
*                               of routing table being used.
*
*   OUTPUTS
*
*       *ROUTE_NODE             Pointer to the node pointing back to the
*                               target node.
*       NU_NULL                 No node exists.
*
*************************************************************************/
ROUTE_NODE *RTAB_Find_Closest_Node(const UINT8 *ip_addr, UINT8 *direction,
                                   const RTAB_ROUTE_PARMS *rt_parms)
{
    ROUTE_NODE      *saved_node;
    ROUTE_NODE      *current_node;

    /* Validate the parameters */
    if ( (*(rt_parms->rt_root_node) == NULL_ROUTE_NODE) ||
         (ip_addr == NU_NULL) )
        return (NULL_ROUTE_NODE);

    current_node = *(rt_parms->rt_root_node);

    /* Traverse the tree for the node pointing back to the node
     * associated with the IP address.
     */
    while (current_node)
    {
        /* Save a pointer to the current node */
        saved_node = current_node;

        /* Determine if we should branch left or right. */
        *direction = RTAB_Determine_Branch(current_node->rt_bit_index,
                                           ip_addr, rt_parms);

        /* Set r to the next child node */
        current_node = current_node->rt_child[*direction];

        /* If we followed an upward link or walked off the tree, set r
         * back to the last node and break out of the loop.
         */
        if ( ((current_node) &&
              (current_node->rt_bit_index >= saved_node->rt_bit_index)) ||
             (!current_node) )
        {
            current_node = saved_node;
            break;
        }
    }

    return (current_node);

} /* RTAB_Find_Closest_Node */
