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
*       rtab_dr.c
*
*   COMPONENT
*
*       Routing
*
*   DESCRIPTION
*
*       This module contains the functions for deleting a route entry.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB_Delete_Node
*       RTAB_Delete_Route_Entry
*       RTAB_Remove_Node
*       RTAB_Reposition_Subnet_Mask_Links
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

STATIC  int     RTAB_Remove_Node(const ROUTE_NODE *, const RTAB_ROUTE_PARMS *);
STATIC  VOID    RTAB_Reposition_Subnet_Mask_Links(const SUBNET_MASK_LIST *,
                                                  const RTAB_ROUTE_PARMS *);

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* lists already declared in rtab4_ar.c */
extern RTAB4_ROUTE_ENTRY    NET_RTAB_Route_Free_List;
extern ROUTE_NODE           NET_RTAB_Node_Free_List;
#endif

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Delete_Node
*
*   DESCRIPTION
*
*       This function deletes all route entries from a route node, and
*       if all route entries are successfully deleted, deletes the
*       route node from the tree.
*
*   INPUTS
*
*       *dn                     A pointer to the node to delete
*       *rt_parms               A pointer to data specific to the family
*                               of routing table being used.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_INVALID_PARM         ROOT_NODE or dn are NU_NULL
*
*************************************************************************/
STATUS RTAB_Delete_Node(ROUTE_NODE *dn, const RTAB_ROUTE_PARMS *rt_parms)
{
    STATUS          status = NU_SUCCESS;
    ROUTE_ENTRY     *rt_entry, *saved_rt_entry;

    /* If the tree is empty, no nodes can be deleted */
    if (*(rt_parms->rt_root_node) == NULL_ROUTE_NODE)
    {
        NLOG_Error_Log("RTAB_Delete_Node NULL route table",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (NU_INVALID_PARM);
    }

    rt_entry = dn->rt_list_head;

    /* Delete each of the route entries associated with the node.  If
     * any of the routes have a reference count greater than 0, the
     * route will be flagged as down and the route node should not yet
     * be deleted.  Once all routes are deleted, the route node will
     * be deleted.
     */
    while (rt_entry)
    {
        /* Save a pointer to the next route in the list */
        saved_rt_entry = rt_entry->rt_flink;

        /* Delete the route entry. */
        status = RTAB_Delete_Route_Entry(dn, rt_entry, rt_parms);

        /* Get a pointer to the next route entry */
        rt_entry = saved_rt_entry;
    }

    return (status);

} /* RTAB_Delete_Node */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Delete_Route_Entry
*
*   DESCRIPTION
*
*       This function deletes a route entry from a route node.  If the
*       last route entry is deleted from the node, the node is removed
*       from the tree.
*
*   INPUTS
*
*       *route_node             A pointer to the node associated with
*                               the route entry.
*       *rt_entry               A pointer to the route entry to delete.
*       *rt_parms               A pointer to data specific to the family
*                               of routing table being used.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*
*************************************************************************/
STATUS RTAB_Delete_Route_Entry(ROUTE_NODE *route_node,
                               ROUTE_ENTRY *rt_entry,
                               const RTAB_ROUTE_PARMS *rt_parms)
{
    STATUS          status;

#if (INCLUDE_SR_SNMP == NU_TRUE)
    UINT8           gateway[IP_ADDR_LEN];
#endif

    /* If the reference count is still not zero, there is some other
     * process using the route.  Flag it as down so it is deleted
     * when that process frees the route.
     */
    if (rt_entry->rt_entry_parms.rt_parm_refcnt != 0)
    {
        rt_entry->rt_entry_parms.rt_parm_flags &= ~RT_UP;
        status = NU_SUCCESS;
    }

    /* Otherwise, remove the entry from the list of entries for
     * the route node.
     */
    else
    {
#if (INCLUDE_IPV6 == NU_TRUE)

        /* The Neighbor Cache and the IPv6 Routing Table are linked
         * together.  Break the link between this route entry and
         * the next-hop for the route.
         */
        if (rt_parms->rt_family == NU_FAMILY_IP6)
            RTAB6_Unlink_Next_Hop((RTAB6_ROUTE_ENTRY*)rt_entry);
#endif
#if (INCLUDE_SR_SNMP == NU_TRUE)

        if (rt_parms->rt_family == NU_FAMILY_IP)
        {
            PUT32(gateway, 0, ((RTAB4_ROUTE_ENTRY*)(rt_entry))->rt_gateway_v4.sck_addr);

            /* There are two cases. They are for the type of route. Either a
             * direct or indirect. If the next hop is a gateway then the route
             * type is indirect. A type of 4 is indirect and 3 is direct. Also
             * 8 is for the routing protocol type. In this case it is RIP.
             */
            if (rt_entry->rt_flags & RT_GATEWAY)
            {
                /* Remove this route from the SNMP routing table */
                SNMP_ipRouteTableUpdate(SNMP_DELETE,
                                        (UINT32)(rt_entry->rt_device->dev_index),
                                        route_node->rt_ip_addr,
                                        (INT32)rt_entry->rt_metric,
                                        (UNSIGNED)-1, (UNSIGNED)-1, (UNSIGNED)-1,
                                        (UNSIGNED)-1, gateway, 4, 8, 0,
                                        route_node->rt_submask, "0.0");
            }
            else
            {
                /* Remove this route from the SNMP routing table */
                SNMP_ipRouteTableUpdate(SNMP_DELETE,
                                        (UINT32)(rt_entry->rt_device->dev_index),
                                        route_node->rt_ip_addr,
                                        (INT32)rt_entry->rt_metric,
                                        (UNSIGNED)-1, (UNSIGNED)-1, (UNSIGNED)-1,
                                        (UNSIGNED)-1, gateway, 4, 8, 0,
                                        route_node->rt_submask, "0.0");
            }
        }
#endif

        /* Remove this route entry from the list of routes for the
         * route node.
         */
        DLL_Remove(&route_node->rt_route_entry_list, rt_entry);

        /* Deallocate the memory used by the route entry */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        status = NU_Deallocate_Memory((VOID*)rt_entry);

        if (status == NU_SUCCESS)
#else
        /* Put the memory back onto the memory free list */
        DLL_Enqueue(&NET_RTAB_Route_Free_List, rt_entry);
        status = NU_SUCCESS;
#endif
        {
            /* If the last route for the node was deleted, delete
             * the route node.
             */
            if (route_node->rt_list_head == NU_NULL)
                status = RTAB_Remove_Node(route_node, rt_parms);
        }
    }

    return (status);

} /* RTAB_Delete_Route_Entry */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Remove_Node
*
*   DESCRIPTION
*
*       Remove a node from the tree.
*
*   INPUTS
*
*       *dn                     A pointer to the node to delete
*       *rt_parms               A pointer to data specific to the family
*                               of routing table being used.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       -1                      An error occurred.
*
*************************************************************************/
STATIC int RTAB_Remove_Node(const ROUTE_NODE *dn,
                            const RTAB_ROUTE_PARMS *rt_parms)
{
    UINT8       direction;
    ROUTE_NODE  *promote_node, *delete_node = NU_NULL;
    int         status = NU_SUCCESS;

    /* Get a pointer to the node to promote to the deleted position */
    promote_node = RTAB_Find_Closest_Node(dn->rt_ip_addr, &direction,
                                          rt_parms);

    /* Get a pointer to the node that is being deleted */
    if (promote_node)
    {
        delete_node = promote_node->rt_child[direction];
    }

    /* If the node exists in the tree to be deleted */
    if ( (delete_node) &&
         (memcmp(delete_node->rt_ip_addr, dn->rt_ip_addr,
                 rt_parms->rt_byte_ip_len) == 0) )
    {
        /* ----- DELETE NODE WITH CHILD POINTING BACK TO ITSELF ----- */

        /* Determine that the node has one child pointing back to itself.
         * The other child pointer will be pointing to NULL, to a node with
         * a higher bit index or to a node with a lower bit index.
         */
        if (delete_node == promote_node)
        {
            /* If the delete node is not the Root Node */
            if (delete_node != *(rt_parms->rt_root_node))
            {
                /* Set the parent's child pointer to point to the node the
                 * deleted node was pointing to before the deletion.
                 */
                if (delete_node->rt_parent->rt_child[0] == delete_node)
                    delete_node->rt_parent->rt_child[0] =
                        delete_node->rt_child[!direction];

                else
                    delete_node->rt_parent->rt_child[1] =
                        delete_node->rt_child[!direction];

                /* Set the promote node's parent to the delete node's parent */
                if ( (delete_node->rt_child[!direction]) &&
                     (delete_node->rt_child[!direction]->rt_bit_index <
                      delete_node->rt_bit_index) )
                    delete_node->rt_child[!direction]->rt_parent =
                        delete_node->rt_parent;
            }

            /* Otherwise, the node to delete is the root node */
            else
            {
                /* If the root node has a child, set the child node as the
                 * new root node.
                 */
                if (delete_node->rt_child[!direction])
                {
                    delete_node->rt_child[!direction]->rt_parent =
                        delete_node->rt_parent;

                    *(rt_parms->rt_root_node) =
                        delete_node->rt_child[!direction];
                }

                /* Otherwise, the tree is empty after deleting the last node */
                else
                    *(rt_parms->rt_root_node) = NU_NULL;
            }
        }

        /* ----- DELETE NODE WITH NO CHILD POINTING BACK TO ITSELF ----- */

        /* Neither of the child pointers are pointing back to the node
         * itself.
         */
        else
        {
            /* If the promote node has a child */
            if ( (promote_node->rt_child[!direction]) &&
                 (promote_node->rt_child[!direction] != promote_node) )
            {
                /* Set the child pointers of the parent node of the promote
                 * node to point to the child of the promote node.
                 */
                if (promote_node->rt_parent->rt_child[0] == promote_node)
                    promote_node->rt_parent->rt_child[0] =
                        promote_node->rt_child[!direction];
                else
                    promote_node->rt_parent->rt_child[1] =
                        promote_node->rt_child[!direction];

                /* If the parent of the promote node is not the node being
                 * deleted, set the new parent of the child node.
                 */
                if (promote_node->rt_parent != delete_node)
                    promote_node->rt_child[!direction]->rt_parent =
                        promote_node->rt_parent;
            }

            /* Set the bit index of the node that is being promoted to the
             * bit index of the node that is being deleted.
             */
            promote_node->rt_bit_index = delete_node->rt_bit_index;

            /* Set the parent of the newly promoted node to the parent of
             * the node that is being deleted.
             */
            promote_node->rt_parent = delete_node->rt_parent;

            /* If the node being deleted has a parent, link the promote
             * node into the tree.
             */
            if (delete_node->rt_parent)
            {
                if (delete_node->rt_parent->rt_child[0] == delete_node)
                    delete_node->rt_parent->rt_child[0] = promote_node;
                else
                    delete_node->rt_parent->rt_child[1] = promote_node;
            }

            /* Otherwise, the Root Node was deleted */
            else
                *(rt_parms->rt_root_node) = promote_node;

            /* Set the children of the newly promoted node to the child
             * of the deleted node.
             */
            promote_node->rt_child[0] = delete_node->rt_child[0];
            promote_node->rt_child[1] = delete_node->rt_child[1];

            /* If the promote node is the parent of the new child pointers,
             * set the parent pointer to the promote node.
             */
            if ( (promote_node->rt_child[0]) &&
                 (promote_node->rt_child[0]->rt_bit_index <
                  promote_node->rt_bit_index) )
                promote_node->rt_child[0]->rt_parent = promote_node;

            if ( (promote_node->rt_child[1]) &&
                 (promote_node->rt_child[1]->rt_bit_index <
                  promote_node->rt_bit_index) )
                promote_node->rt_child[1]->rt_parent = promote_node;
        }

        /* If this is a network route, remove the node from the list of
         * subnet masks of the appropriate node.
         */
        if (delete_node->rt_submask_length < rt_parms->rt_bit_ip_len)
        {
            /* Remove the node from the list */
            DLL_Remove(&delete_node->rt_submask_list_parent->rt_submask_list,
                       delete_node);
        }

        /* If the node being deleted contains a list of subnet mask entries,
         * find the new best node for the entries.
         */
        if (delete_node->rt_submask_list.submask_head)
            RTAB_Reposition_Subnet_Mask_Links(&delete_node->rt_submask_list,
                                              rt_parms);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        status = NU_Deallocate_Memory((VOID*)delete_node);
#else
        /* Turn the memory flag off to indicate that the memory is unused now */
        DLL_Enqueue(&NET_RTAB_Node_Free_List, delete_node);
        status = NU_SUCCESS;
#endif
    }

    return (status);

} /* RTAB_Remove_Node */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB_Reposition_Subnet_Mask_Links
*
*   DESCRIPTION
*
*       This function positions each node in a subnet mask list to
*       the appropriate node's list in the tree.
*
*   INPUTS
*
*       *submask_list           A pointer to the list of subnet masks.
*       *rt_parms               A pointer to data specific to the family
*                               of routing table being used.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID RTAB_Reposition_Subnet_Mask_Links(const SUBNET_MASK_LIST *submask_list,
                                              const RTAB_ROUTE_PARMS *rt_parms)
{
    ROUTE_NODE  *current_node, *saved_node;

    /* Get a pointer to the first node in the list */
    current_node = submask_list->submask_head;

    /* Traverse the list, repositioning each node on the list to another
     * node in the tree.
     */
    do
    {
        /* Save a pointer to the next node in the list */
        saved_node = current_node->rt_next;

        /* Reposition the node in the list to another node in the tree */
        RTAB_Establish_Subnet_Mask_Links(current_node, rt_parms);

        /* Get a pointer to the next node in the list */
        current_node = saved_node;

    } while (current_node);

} /* RTAB_Reposition_Subnet_Mask_Links */
