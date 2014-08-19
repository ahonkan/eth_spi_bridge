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
*       rtab_ar.c
*
*   COMPONENT
*
*       Routing
*
*   DESCRIPTION
*
*       This module contains the functions for adding routes to the
*       routing table.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB_Insert_Node
*       RTAB_Establish_Links
*       RTAB_Establish_Subnet_Mask_Links
*       RTAB_Find_Bit_Index
*       RTAB_Find_Lone_Bit_Index
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

STATIC  UINT8   RTAB_Find_Bit_Index(const UINT8 *, const UINT8 *, UINT8);
STATIC  UINT8   RTAB_Find_Lone_Bit_Index(const UINT8 *, UINT8);
STATIC  VOID    RTAB_Establish_Links(ROUTE_NODE *, ROUTE_NODE *, UINT8 *,
                                     const RTAB_ROUTE_PARMS *);

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Insert_Node
*
*   DESCRIPTION
*
*       Insert a node to either the left or right side of node.
*
*   INPUTS
*
*       *n                      A pointer to the node to insert into tree.
*       *rt_parms               A pointer to data specific to the family
*                               of routing table being used.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_MEM_ALLOC            Memory allocation failure
*       NU_NO_ACTION            The route already exists
*       NU_INVALID_PARM         The route metric is invalid or the route
*                               cannot be inserted into the table
*
*************************************************************************/
STATUS RTAB_Insert_Node(const ROUTE_NODE *n, const RTAB_ROUTE_PARMS *rt_parms)
{
    STATUS          status = NU_SUCCESS;
    ROUTE_NODE      *new_node, *compare_node, *closest_node;
    UINT8           direction;

    /* Do not add route with an invalid metric. */
    if (((ROUTE_ENTRY*)(n->rt_list_head))->rt_entry_parms.rt_parm_metric >=
        RT_INFINITY)
        status = NU_INVALID_PARM;

    else
    {
        /* If this is the first time called then set up root node */
        if (*(rt_parms->rt_root_node) == NULL_ROUTE_NODE)
        {
            /* Create the new node and set up the elements of the route */
            new_node = *(rt_parms->rt_root_node) = rt_parms->rt_setup_new_node(n);

            /* If a new node could not be created, return an error code */
            if (new_node == NU_NULL)
            {
                NLOG_Error_Log("RTAB_Insert_Node insufficient memory",
                               NERR_SEVERE, __FILE__, __LINE__);

                status = NU_MEM_ALLOC;
            }

            else
            {
                /* Determine the bit index of the root node */
                new_node->rt_bit_index =
                    RTAB_Find_Lone_Bit_Index(n->rt_ip_addr,
                                             rt_parms->rt_byte_ip_len);

                /* The right child will always point back to the node and the left
                 * child will always point to NU_NULL.
                 */
                new_node->rt_child[1] = new_node;
                new_node->rt_child[0] = NU_NULL;

                /* If this is a network route, establish the links for the
                 * subnet mask at the appropriate position in the tree.
                 */
                if (new_node->rt_submask_length < rt_parms->rt_bit_ip_len)
                    RTAB_Establish_Subnet_Mask_Links(new_node, rt_parms);
            }
        }

        /* Insert the new node into the tree */
        else
        {
            /* Find the closest node associated with the target IP address */
            closest_node = RTAB_Find_Closest_Node(n->rt_ip_addr, &direction,
                                                  rt_parms);

            /* Ensure a closest node was found. */
            if (closest_node)
            {
                /* Save a pointer to the upward link from the closest node */
                compare_node = closest_node->rt_child[direction];

                /* If a node does not already exist in the tree for the target
                 * IP address.
                 */
                if ( (!compare_node) ||
                     (memcmp(compare_node->rt_ip_addr, n->rt_ip_addr,
                             rt_parms->rt_byte_ip_len) != 0) )
                {
                    /* Create the new node and set up the elements of the route */
                    new_node = rt_parms->rt_setup_new_node(n);

                    /* If a new node could not be created, return an error code */
                    if (new_node == NU_NULL)
                    {
                        NLOG_Error_Log("RTAB_Insert_Node insufficient memory",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        status = NU_MEM_ALLOC;
                    }

                    else
                    {
                        /* If we did not walk off the tree to get to the closest node */
                        if (compare_node)
                            new_node->rt_bit_index =
                                RTAB_Find_Bit_Index(compare_node->rt_ip_addr,
                                                    new_node->rt_ip_addr,
                                                    rt_parms->rt_byte_ip_len);

                        /* Otherwise, add as if we are adding a lone node */
                        else
                            new_node->rt_bit_index =
                                RTAB_Find_Lone_Bit_Index(new_node->rt_ip_addr,
                                                         rt_parms->rt_byte_ip_len);

                        /* Link the new node into the tree */
                        RTAB_Establish_Links(new_node, closest_node, &direction,
                                             rt_parms);

                        /* If this is a network route, establish the links for the
                         * subnet mask at the appropriate position in the tree.
                         */
                        if (new_node->rt_submask_length < rt_parms->rt_bit_ip_len)
                            RTAB_Establish_Subnet_Mask_Links(new_node, rt_parms);
                    }
                }

                /* Update the parameters of the route */
                else
                    status = (STATUS)rt_parms->rt_insert_route_entry(compare_node, n);
            }

            /* A closest node could not be found in the table. */
            else
            {
                status = NU_INVALID_PARM;
            }
        }
    }

    return (status);

} /* RTAB_Insert_Node */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Establish_Links
*
*   DESCRIPTION
*
*       This function links a new node into its proper position in
*       the Routing Tree.
*
*   INPUTS
*
*       *new_node               A pointer to the new node to insert.
*       *closest_node           A pointer to the reference node to use
*                               to insert the new node.
*       *direction              A pointer to the direction traveled to
*                               get to the closest_node.
*       *rt_parms               A pointer to data specific to the family
*                               of routing table being used.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID RTAB_Establish_Links(ROUTE_NODE *new_node, ROUTE_NODE *closest_node,
                                 UINT8 *direction,
                                 const RTAB_ROUTE_PARMS *rt_parms)
{
    ROUTE_NODE  *saved_node = NU_NULL, *current_node;

    /* If the bit index of the new node is less than the bit index of
     * the closest node, insert the new node as a child of the closest
     * node.
     */
    if (new_node->rt_bit_index < closest_node->rt_bit_index)
    {
        /* Save off what the child pointer was previously pointing to */
        saved_node = closest_node->rt_child[*direction];

        /* Point the child pointer to the new node */
        closest_node->rt_child[*direction] = new_node;

        /* Set the parent of new_node to the closest_node */
        new_node->rt_parent = closest_node;
    }

    /* Otherwise, traverse the tree backwards looking for the position
     * to insert the node.
     */
    else
    {
        /* Get a pointer to the parent of the closest node */
        current_node = closest_node->rt_parent;

        /* Find the position in the tree greater than the bit index
         * of the new node.
         */
        while (current_node)
        {
            /* If this is the position to put the new node */
            if (current_node->rt_bit_index > new_node->rt_bit_index)
            {
                /* Determine which way we branched to get to this node */
                *direction = RTAB_Determine_Branch(current_node->rt_bit_index,
                                                   new_node->rt_ip_addr,
                                                   rt_parms);

                /* Save off what the child pointer was previously
                 * pointing to.
                 */
                saved_node = current_node->rt_child[*direction];

                /* Point the child pointer to the new node */
                current_node->rt_child[*direction] = new_node;

                break;
            }

            /* Get a pointer to the next parent node */
            current_node = current_node->rt_parent;
        }

        /* The new node is to become the root node */
        if (current_node == NU_NULL)
        {
            /* Save the root */
            saved_node = *(rt_parms->rt_root_node);

            /* Set the new Root node */
            *(rt_parms->rt_root_node) = new_node;

            /* Set the new Root node's parent to NULL */
            new_node->rt_parent = NU_NULL;
        }

        /* Otherwise, set the parent of new_node to current_node */
        else
            new_node->rt_parent = current_node;
    }

    /* If the value at the bit index is 1 or we are adding what is
     * considered to be a lone node, set the right child pointer back
     * to the node itself and the left child pointer to what the parent
     * was previously pointing to.
     */
    if ( (RTAB_Determine_Branch(new_node->rt_bit_index,
                                new_node->rt_ip_addr, rt_parms)) ||
         ((closest_node->rt_child[*direction] == NU_NULL) &&
          (saved_node == NU_NULL)) )
    {
        new_node->rt_child[1] = new_node;
        new_node->rt_child[0] = saved_node;
    }

    /* If the value at the bit index is 0, set the left child pointer
     * back to the node itself and the right child pointer to what the
     * parent was previously pointing to.
     */
    else
    {
        new_node->rt_child[0] = new_node;
        new_node->rt_child[1] = saved_node;
    }

    /* Set the parent of the saved child as the new node if necessary */
    if ( (saved_node) &&
         (saved_node->rt_bit_index < new_node->rt_bit_index) )
        saved_node->rt_parent = new_node;

} /* RTAB_Establish_Links */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Establish_Subnet_Mask_Links
*
*   DESCRIPTION
*
*       If the new node that was added to the tree is a network route,
*       this function traverses the tree and finds the node closest to
*       the root that exists on that network and adds a link back from
*       that node to the network route.
*
*   INPUTS
*
*       *new_node               A pointer to the newly inserted node.
*       *rt_parms               A pointer to data specific to the family
*                               of routing table being used.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID RTAB_Establish_Subnet_Mask_Links(ROUTE_NODE *new_node,
                                      const RTAB_ROUTE_PARMS *rt_parms)
{
    ROUTE_NODE  *saved_node, *current_node;

    saved_node = new_node;

    current_node = new_node->rt_parent;

    /* Walk back to the root of the tree saving off the highest node
     * that matches the subnet mask.
     */
    while (current_node)
    {
        if (rt_parms->rt_determine_matching_prefix(current_node->rt_ip_addr,
                                                   new_node->rt_ip_addr,
                                                   new_node->rt_submask,
                                                   new_node->rt_submask_length)
                                                   == NU_TRUE)
            saved_node = current_node;

        current_node = current_node->rt_parent;
    }

    /* Determine the position in the list to place the subnet
     * mask.
     */
    if (saved_node->rt_submask_list.submask_head != NU_NULL)
    {
        current_node = saved_node->rt_submask_list.submask_head;

        /* Find the position in the list in which the new subnet
         * mask should be added.  The list should go from most
         * specific to least specific.
         */
        do
        {
            /* If the current subnet mask is less specific than the
             * new subnet mask, insert the new subnet in the list before
             * the current.
             */
            if (current_node->rt_submask_length <
                new_node->rt_submask_length)
            {
                DLL_Insert(&saved_node->rt_submask_list, new_node, current_node);
                break;
            }

            /* If we are at the end of the list, insert the link at the
             * end of the list.
             */
            else if (current_node->rt_next == NU_NULL)
            {
                DLL_Enqueue(&saved_node->rt_submask_list, new_node);
                break;
            }

            current_node = current_node->rt_next;

        } while (current_node);
    }

    /* The list is empty, so add the new one to the head */
    else
        DLL_Enqueue(&saved_node->rt_submask_list, new_node);

    new_node->rt_submask_list_parent = saved_node;

} /* RTAB_Establish_Subnet_Mask_Links */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Find_Bit_Index
*
*   DESCRIPTION
*
*       This function determines the bit number at which two IP addresses
*       differ.
*
*   INPUTS
*
*       *ip_addr1               A pointer to an IP address.
*       *ip_addr2               A pointer to an IP address.
*       ip_len                  The length of the IP address in bytes.
*
*   OUTPUTS
*
*       UINT8                   The bit number where the two addresses
*                               differ.
*
*************************************************************************/
STATIC UINT8 RTAB_Find_Bit_Index(const UINT8 *ip_addr1, const UINT8 *ip_addr2,
                                 UINT8 ip_len)
{
    INT     bytes = ip_len - 1;
    INT     bit_index = 7;

    /* While we have not looked at every bit in the address */
    while (bytes >= 0)
    {
        /* If the current bits match */
        if ( ((1 << bit_index) & *ip_addr1) ==
             ((1 << bit_index) & *ip_addr2) )
        {
            /* If we have looked at every bit in this byte, process the
             * next byte.
             */
            if (bit_index == 0)
            {
                ip_addr1 ++;
                ip_addr2 ++;
                bit_index = 7;
                bytes --;
            }

            /* Otherwise, look at the next bit */
            else
                bit_index --;
        }

        /* The bits do not match */
        else
            break;
    }

    /* Determine the true bit index based on all possible bytes from left
     * to right.
     */
    bit_index = bytes * 8 + bit_index;

    return ((UINT8)bit_index);

} /* RTAB_Find_Bit_Index */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Find_Lone_Bit_Index
*
*   DESCRIPTION
*
*       This function returns the bit number of the first 1 bit in the
*       IP address.
*
*   INPUTS
*
*       *ip_addr                A pointer to the IP address.
*       ip_len                  The length of the IP address in bytes.
*
*   OUTPUTS
*
*       UINT8                   The bit number of the first 1 bit in the
*                               IP address.
*
*************************************************************************/
STATIC UINT8 RTAB_Find_Lone_Bit_Index(const UINT8 *ip_addr, UINT8 ip_len)
{
    INT     bytes = ip_len - 1;
    INT     bit_index = 7;

    /* While we have not looked at every bit in the address */
    while (bytes >= 0)
    {
        /* If the current bit is set */
        if ((1 << bit_index) & *ip_addr)
            break;

        else
        {
            /* If we have looked at every bit in this byte, process the
             * next byte.
             */
            if (bit_index == 0)
            {
                ip_addr ++;
                bit_index = 7;
                bytes --;
            }

            /* Otherwise, look at the next bit */
            else
                bit_index --;
        }
    }

    /* Determine the true bit index based on all possible bytes from left
     * to right.
     */
    bit_index = bytes * 8 + bit_index;

    return ((UINT8)bit_index);

} /* RTAB_Find_Lone_Bit_Index */
