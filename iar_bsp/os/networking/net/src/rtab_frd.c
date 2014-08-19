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
*       rtab_frd.c
*
*   COMPONENT
*
*       Routing
*
*   DESCRIPTION
*
*       This module contains the functions to find a route for a device.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB_Find_Route_For_Device
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
*       RTAB_Find_Route_For_Device
*
*   DESCRIPTION
*
*       This function finds a route associated with the given device and
*       returns a pointer to it.  We traverse the left child of every
*       node first, then backtrack and traverse the right child.
*
*   INPUTS
*
*       *device                 A pointer to the device for which you
*                               want to find a route.
*       *root_node              A pointer to the tree to search.
*
*   OUTPUTS
*
*       ROUTE_NODE*             A pointer to the node associated with the
*                               target device.
*       NU_NULL                 The target does not exist in the list.
*
*************************************************************************/
ROUTE_NODE *RTAB_Find_Route_For_Device(const DV_DEVICE_ENTRY *device,
                                       ROUTE_NODE *root_node)
{
    ROUTE_NODE      *current_node;
    INT             direction = 0;
    STATUS          status;
    ROUTE_ENTRY     *rt_entry;

    current_node = root_node;

    while (current_node)
    {
        rt_entry = current_node->rt_list_head;

        /* Check if we found the device */
        if (rt_entry->rt_entry_parms.rt_parm_flags & RT_UP)
        {
            status = strcmp(device->dev_net_if_name,
                            rt_entry->rt_entry_parms.rt_parm_device->dev_net_if_name);

            if (status == 0)
                break;
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
                if ( (current_node != root_node) &&
                     (current_node == current_node->rt_parent->rt_child[0]) )
                    current_node = current_node->rt_parent;

                /* Otherwise, continue to go up one parent level until
                 * reaching the Root Node or a left child.
                 */
                else
                {
                    while ( (current_node != root_node) &&
                            (current_node != current_node->rt_parent->rt_child[0]) )
                        current_node = current_node->rt_parent;

                    /* Reached the Root Node and have traversed the entire
                     * tree.
                     */
                    if (current_node == root_node)
                    {
                        current_node = NU_NULL;
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

    return (current_node);

} /* RTAB_Find_Route_For_Device */
