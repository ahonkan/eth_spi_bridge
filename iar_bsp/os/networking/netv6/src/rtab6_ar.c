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
*       rtab6_ar.c                                   
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This module contains the functions for accessing the IPv6 routing     
*       table.                                                           
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*                                                                       
*       RTAB6_Add_Route
*       RTAB6_Setup_New_Node
*       RTAB6_Create_Node
*       RTAB6_Create_Route_Entry
*       RTAB6_Insert_Route_Entry
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*       nc6.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/nc6.h"
#include "services/nu_trace_os_mark.h"

STATIC  ROUTE_NODE          *RTAB6_Create_Node(const RTAB6_ROUTE_ENTRY *);
STATIC  RTAB6_ROUTE_ENTRY   *RTAB6_Create_Route_Entry(ROUTE_NODE *,
                                                      const RTAB6_ROUTE_ENTRY *);

extern RTAB_ROUTE_PARMS RTAB6_Parms;

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Add_Route                                                  
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Add a new route to the routine table.                           
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *dev_ptr                A pointer to the device associated with 
*                               the destination.
*       *ip_addr                The IP address of the destination.
*       *next_hop               The IP address of the next hop of the 
*                               address
*       prefixlen               The prefix length, in bits, of the 
*                               destination of the route.
*       flags                   Flags associated with the route.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              Successful
*       NU_NO_ACTION            The route already exists.
*       NU_MEM_ALLOC            Insufficient memory
*                                                                       
*************************************************************************/
STATUS RTAB6_Add_Route(DV_DEVICE_ENTRY *dev_ptr, const UINT8 *ip_addr, 
                       const UINT8 *next_hop, UINT8 prefixlen, UINT32 flags)
{
    ROUTE_NODE          rn;
    RTAB6_ROUTE_ENTRY   rt_entry;
    STATUS              st;

    if ( (dev_ptr == NU_NULL) || (ip_addr == NU_NULL) )
        return (NU_INVALID_PARM);

    UTL_Zero(&rn, sizeof(ROUTE_NODE));
    UTL_Zero(&rt_entry, sizeof(RTAB6_ROUTE_ENTRY));

    NU_BLOCK_COPY(rn.rt_ip_addr, ip_addr, IP6_ADDR_LEN);

    /* If no prefix length was specified, default to 128 bits */
    if (prefixlen == 0)
    {
        prefixlen = 128;
        rt_entry.rt_entry_parms.rt_parm_flags |= RT_HOST;
    }

    /* Store the length of the subnet mask */
    rn.rt_submask_length = prefixlen;

    /* Fill in the IPv6 specific portion of the route */
    rt_entry.rt_entry_parms.rt_parm_flags = flags;
    rt_entry.rt_entry_parms.rt_parm_refcnt = 0;
    rt_entry.rt_entry_parms.rt_parm_use = 0;
    rt_entry.rt_entry_parms.rt_parm_device = dev_ptr;

    /* RFC 1981 section 3 - ... a source node initially assumes
     * that the PMTU of a path is the (known) first hop of the
     * path.
     */
    rt_entry.rt_path_mtu = dev_ptr->dev6_link_mtu;

    /* A next-hop does not have to be specified if the destination 
     * address is a multicast address.
     */
    if (next_hop)
    {
        rt_entry.rt_next_hop_entry = 
            dev_ptr->dev6_fnd_neighcache_entry(dev_ptr, next_hop);

        rt_entry.rt_next_hop.sck_family = NU_FAMILY_IP6;
        rt_entry.rt_next_hop.sck_len = sizeof(SCK6_SOCKADDR_IP);

        NU_BLOCK_COPY(rt_entry.rt_next_hop.sck_addr, next_hop, IP6_ADDR_LEN);
    }
    
    DLL_Enqueue(&rn.rt_route_entry_list, &rt_entry);

    /* Set the initial metric */
    if (dev_ptr->dev_metric == 0)
        ((ROUTE_ENTRY*)(rn.rt_list_head))->rt_entry_parms.rt_parm_metric = 1;
    else
        ((ROUTE_ENTRY*)(rn.rt_list_head))->rt_entry_parms.rt_parm_metric = 
            dev_ptr->dev_metric;

    st = RTAB_Insert_Node(&rn, &RTAB6_Parms);

    if (st < 0)
    {
        /* Increment the number of routes that could not be added. */
        MIB2_ipRoutingDiscards_Inc;
    }

    /* Trace log */
    T_ADD_ROUTE6(dev_ptr->dev_net_if_name, rn.rt_ip_addr, (UINT8*)next_hop, prefixlen, st, 16);
    
    return (st);

} /* RTAB6_Add_Route */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Create_Node                                                
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Create a ROUTE_NODE structure that can be inserted into the      
*       RIP2 tree.                                                       
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *rt_entry               A pointer to the Route entry.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       *ROUTE_NODE             A pointer to the new route node.
*       NU_NULL                 Insufficient memory.
*                                                                       
*************************************************************************/
STATIC ROUTE_NODE *RTAB6_Create_Node(const RTAB6_ROUTE_ENTRY *rt_entry)
{
    STATUS              status;
    ROUTE_NODE          *rn;
    RTAB6_ROUTE_ENTRY   *new_rt_entry;

    /* Allocate space for the node. */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&rn,
                                sizeof(ROUTE_NODE), NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("RTAB6_Create_Node insufficient memory", 
                       NERR_SEVERE, __FILE__, __LINE__);

        return (NU_NULL);        /* could not get the memory */
    }

    UTL_Zero(rn, sizeof(ROUTE_NODE));

    /* Create the IPv6 entry */
    new_rt_entry = RTAB6_Create_Route_Entry(rn, rt_entry);

    if (new_rt_entry == NU_NULL)
    {
        NLOG_Error_Log("RTAB6_Create_Node insufficient memory", 
                       NERR_SEVERE, __FILE__, __LINE__);

        if (NU_Deallocate_Memory((VOID*)rn) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for Route Node", 
                           NERR_SEVERE, __FILE__, __LINE__);

        return (NU_NULL);        /* could not get the memory */
    }

    /* Add the entry to the route node */
    DLL_Enqueue(&rn->rt_route_entry_list, new_rt_entry);

    return (rn);

} /* RTAB6_Create_Node */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Setup_New_Node                                                
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This functions creates a new ROUTE_NODE and sets up the parameters
*       of the node based on the ROUTE_NODE passed in to the function.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *n                      A pointer to the node containing the
*                               parameters to copy into the new node.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       *ROUTE_NODE             Upon success, a pointer to the new node.
*       NU_NULL                 Upon failure, a NULL route.
*                                                                       
*************************************************************************/
ROUTE_NODE *RTAB6_Setup_New_Node(const ROUTE_NODE *n)
{
    ROUTE_NODE      *new_node;
    
    /* Create a new node */
    new_node = RTAB6_Create_Node(n->rt_list_head);

    /* make sure that it was allocated */
    if (new_node != NU_NULL)
    {
        NU_BLOCK_COPY(new_node->rt_ip_addr, n->rt_ip_addr, IP6_ADDR_LEN);
        new_node->rt_submask_length = n->rt_submask_length;
    }

    return (new_node);

} /* RTAB6_Setup_New_Node */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Insert_Route_Entry                                             
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Update the node with new data and reset the clock.              
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *r                      A pointer to the node to update                                        
*       *n                      A pointer to the data to update with
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              Successful
*       NU_MEM_ALLOC            Insufficient memory
*       NU_NO_ACTION            The route already exists
*                                                                       
*************************************************************************/
INT RTAB6_Insert_Route_Entry(ROUTE_NODE *r, const ROUTE_NODE *n)
{
    RTAB6_ROUTE_ENTRY   *current_route, *new_route;
    int                 status = NU_SUCCESS;

    if ( (r == NU_NULL) || (n == NU_NULL) )
        return (-1);

    current_route = r->rt_list_head;

    /* Traverse through the routes to determine if this is a new route
     * to the same destination or if the route needs to be updated.
     */
    do
    {
        /* If the next-hop of the new route matches the next-hop of the
         * current route, this route already exists.
         */
        if (IP6_ADDRS_EQUAL(current_route->rt_next_hop.sck_addr,
                            ((RTAB6_ROUTE_ENTRY*)(n->rt_list_head))
                            ->rt_next_hop.sck_addr))
            break;

        current_route = current_route->rt_entry_next;

    } while (current_route);

    /* If a route was found with the same next-hop, return an error. */
    if (current_route)
    {
        /* If the route has been flagged for deletion, flag the route as
         * UP so it will not be deleted when freed.
         */
        if ( (!(current_route->rt_entry_parms.rt_parm_flags & RT_UP)) && 
             (current_route->rt_entry_parms.rt_parm_refcnt != 0) )
        {
            current_route->rt_entry_parms.rt_parm_flags |= RT_UP;       

            /* Set the status to NU_SUCCESS since the application layer 
             * expects that this route was previously deleted.
             */
            status = NU_SUCCESS;
        }

        else
            status = NU_NO_ACTION;
    }

    /* Otherwise, add another route to the same destination */
    else
    {
        /* Create a new route entry */
        new_route = RTAB6_Create_Route_Entry(r, n->rt_list_head);

        if (new_route != NU_NULL)
        {
            /* If the flag was set for this route to be used by default,
             * add the entry to the beginning of the list.
             */
            if (new_route->rt_entry_parms.rt_parm_flags & RT_BEST_METRIC)
            {
                new_route->rt_entry_next = r->rt_route_entry_list.rt_entry_head;
                new_route->rt_entry_next->rt_entry_prev = new_route;
                r->rt_route_entry_list.rt_entry_head = new_route;
            }

            /* Add the entry to the end of the list of routes for 
             * the node.
             */
            else            
                DLL_Enqueue(&r->rt_route_entry_list, new_route);
        }

        else
            status = NU_MEM_ALLOC;
    }

    return (status);

} /* RTAB6_Insert_Route_Entry */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Create_Route_Entry                                             
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function creates a new RTAB6_ROUTE_ENTRY data structure. 
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *rt_node                A pointer to the Route node associated
*                               with the route entry.
*       *rt_entry               A pointer to the data structure from 
*                               which to copy the new elements of the
*                               new data structure.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       *RTAB6_ROUTE_ENTRY      A pointer to the new entry.
*       NU_NULL                 The memory could not be allocated.
*                                                                       
*************************************************************************/
STATIC RTAB6_ROUTE_ENTRY *RTAB6_Create_Route_Entry(ROUTE_NODE *rt_node,
                                                   const RTAB6_ROUTE_ENTRY *rt_entry)
{
    RTAB6_ROUTE_ENTRY   *new_route;
    STATUS              status;

    /* Allocate memory for the new entry and fill-in the elements */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&new_route,
                           sizeof(RTAB6_ROUTE_ENTRY), 
                           NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        if (rt_entry)
        {
            NU_BLOCK_COPY(new_route, rt_entry, sizeof(RTAB6_ROUTE_ENTRY));

            new_route->rt_next_hop.sck_family = NU_FAMILY_IP6;
            new_route->rt_next_hop.sck_len = sizeof(SCK6_SOCKADDR_IP);

            new_route->rt_entry_parms.rt_parm_clock = NU_Retrieve_Clock();

            new_route->rt_route_node = rt_node;

            /* Add the route entry to the list of routes for the Neighbor 
             * Cache entry.
             */
            if (new_route->rt_next_hop_entry)            
                RTAB6_Add_DestList_Entry(&new_route->rt_next_hop_entry->ip6_neigh_cache_dest_list,
                                         new_route);
        }
    }

    /* Memory could not be allocated */
    else
        new_route = NU_NULL;

    return (new_route);

} /* RTAB6_Create_Route_Entry */
