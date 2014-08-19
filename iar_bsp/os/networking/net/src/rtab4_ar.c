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
*       rtab4_ar.c
*
*   COMPONENT
*
*       IPv4 Routing
*
*   DESCRIPTION
*
*       This module contains the functions for adding a route to the IPv4
*       routing table.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB4_Add_Route
*       RTAB4_Setup_New_Node
*       RTAB4_Create_Node
*       RTAB4_Create_Route_Entry
*       RTAB4_Insert_Route_Entry
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

STATIC  ROUTE_NODE          *RTAB4_Create_Node(const RTAB4_ROUTE_ENTRY *);
STATIC  RTAB4_ROUTE_ENTRY   *RTAB4_Create_Route_Entry(ROUTE_NODE *,
                                                      const RTAB4_ROUTE_ENTRY *);

extern RTAB_ROUTE_PARMS RTAB4_Parms;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for RTAB nodes */
ROUTE_NODE          NET_RTAB_Node_Memory[NET_MAX_RTAB_NODES];

/* Declare memory for RTAB route entries */
RTAB4_ROUTE_ENTRY   NET_RTAB_Routing_Entry_Memory[NET_MAX_RTAB_ROUTING_ENTRIES];

/* Declare structures(list heads) for creating free memory
  lists to use with DLL_Enqueue/DLL_Dequeue */
RTAB4_ROUTE_ENTRY   NET_RTAB_Route_Free_List;
ROUTE_NODE          NET_RTAB_Node_Free_List;

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Memory_Init
*
*   DESCRIPTION
*
*       Initializes the free memory lists for routes memory and
*       nodes memory
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID RTAB4_Memory_Init(VOID)
{
    INT         i;

    /* Enqueue all route memory array onto the free memory list */
    for(i = 0; i < NET_MAX_RTAB_ROUTING_ENTRIES; i++)
        DLL_Enqueue(&NET_RTAB_Route_Free_List, &NET_RTAB_Routing_Entry_Memory[i]);

    /* Enqueue all node memory array onto the free memory list */
    for(i = 0; i < NET_MAX_RTAB_NODES; i++)
        DLL_Enqueue(&NET_RTAB_Node_Free_List, &NET_RTAB_Node_Memory[i]);

}
#endif /* INCLUDE_STATIC_BUILD == NU_TRUE */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Add_Route
*
*   DESCRIPTION
*
*       Add a new route to the routine table.
*
*   INPUTS
*
*       *device                 A pointer to the device that the route
*                               should be added to
*       dest                    The destination address
*       mask                    The mask for the address
*       gw                      The gateway of the address
*       flags                   The flags for the route
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       -1                      Route was not added
*       NU_MEM_ALLOC            Insufficient memory
*
*************************************************************************/
STATUS RTAB4_Add_Route(DV_DEVICE_ENTRY *device, UINT32 dest, UINT32 mask,
                       UINT32 gw, UINT32 flags)
{
    ROUTE_NODE          rn;
    RTAB4_ROUTE_ENTRY   rt_entry;
    STATUS              st;

#if (INCLUDE_SR_SNMP == NU_TRUE)
    UINT8               gateway[IP_ADDR_LEN];
    INT32               proto;
    INT32               type;
#endif

    if ( (mask == 0) || (device == NU_NULL) )
        return (NU_INVALID_PARM);

    UTL_Zero(&rn, sizeof(ROUTE_NODE));

    PUT32(rn.rt_ip_addr, 0, dest);
    PUT32(rn.rt_submask, 0, mask);

    /* Store the length of the subnet mask */
    rn.rt_submask_length = RTAB4_Find_Prefix_Length(rn.rt_submask);

    UTL_Zero(&rt_entry, sizeof(RTAB4_ROUTE_ENTRY));

    /* Fill in the IPv4 specific portion of the route */
    rt_entry.rt_flags = flags;
    rt_entry.rt_device = device;
    rt_entry.rt_path_mtu = device->dev_mtu;

    memcpy((UINT8 *)&rt_entry.rt_gateway_v4.sck_addr,
           (UINT8 *)&gw, sizeof(gw));

    rt_entry.rt_gateway_v4.sck_family = NU_FAMILY_IP;
    rt_entry.rt_gateway_v4.sck_len = sizeof(SCK_SOCKADDR_IP);

    DLL_Enqueue(&rn.rt_route_entry_list, &rt_entry);

    /* Set the initial metric */
    if (device->dev_metric == 0)
        ((ROUTE_ENTRY*)(rn.rt_list_head))->rt_entry_parms.rt_parm_metric = 1;
    else
        ((ROUTE_ENTRY*)(rn.rt_list_head))->rt_entry_parms.rt_parm_metric =
            device->dev_metric;

    st = RTAB_Insert_Node(&rn, &RTAB4_Parms);

    if (st < 0)
    {
        /* Increment the number of routes that could not be added. */
        MIB2_ipRoutingDiscards_Inc;
    }

#if (INCLUDE_SR_SNMP == NU_TRUE)

    else
    {
        if (flags & RT_HOST)
            type = 3;
        else if (flags & RT_GATEWAY)
            type = 4;
        else
            type = 1;

        if (flags & RT_RIP2)
            proto = 8;
        else if (flags & RT_ICMP)
            proto = 4;
        else if (flags & RT_NETMGMT)
            proto = 3;
        else
            proto = 2;

        PUT32(gateway, 0, rt_entry.rt_gateway_v4.sck_addr);

        /* Add this route to the SNMP routing table */
        SNMP_ipRouteTableUpdate(SNMP_ADD, (UINT32)(device->dev_index), rn.rt_ip_addr,
                                (INT32)rt_entry.rt_metric,
                                (UNSIGNED)-1, (UNSIGNED)-1,
                                (UNSIGNED)-1, (UNSIGNED)-1,
                                gateway, type, proto,
                                0, rn.rt_submask, "0.0");
    }

#endif

    /* Trace log */
    T_ADD_ROUTE(device->dev_net_if_name, rn.rt_ip_addr, rn.rt_submask, gw, st, 4);

    return (st);

} /* RTAB4_Add_Route */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Create_Node
*
*   DESCRIPTION
*
*       Create a ROUTE_NODE structure that can be inserted into the
*       RIP2 tree.
*
*   INPUTS
*
*       *rt_entry               A pointer to the route entry
*
*   OUTPUTS
*
*       *ROUTE_NODE             A pointer to the new route node.
*       NU_NULL                 Insufficient memory.
*
*************************************************************************/
STATIC ROUTE_NODE *RTAB4_Create_Node(const RTAB4_ROUTE_ENTRY *rt_entry)
{
    STATUS              status;
    ROUTE_NODE          *rn;
    RTAB4_ROUTE_ENTRY   *new_rt_entry;

    /* Allocate space for the node. */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&rn,
                                sizeof(ROUTE_NODE), NU_NO_SUSPEND);
#else
    /* Assign memory*/
    rn  = DLL_Dequeue(&NET_RTAB_Node_Free_List) ;

    if (rn != NU_NULL)
        status = NU_SUCCESS;
    else
        status = NU_NO_MEMORY;
#endif

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("RTAB4_Create_Node insufficient memory",
                       NERR_SEVERE, __FILE__, __LINE__);

        return((ROUTE_NODE *)0);        /* could not get the memory */
    }

    UTL_Zero(rn, sizeof(ROUTE_NODE));

    /* Create the IPv4 entry */
    new_rt_entry = RTAB4_Create_Route_Entry(rn, rt_entry);

    if (new_rt_entry == NU_NULL)
    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

        if (NU_Deallocate_Memory((VOID*)rn) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for route node",
                           NERR_SEVERE, __FILE__, __LINE__);

        NLOG_Error_Log("RTAB4_Create_Node insufficient memory",
                       NERR_SEVERE, __FILE__, __LINE__);
#else
        DLL_Enqueue(&NET_RTAB_Node_Free_List, rn);
#endif

        return((ROUTE_NODE *)0);        /* could not get the memory */
    }

    /* Add the entry to the route node */
    DLL_Enqueue(&rn->rt_route_entry_list, new_rt_entry);

    return (rn);

} /* RTAB4_Create_Node */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Setup_New_Node
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
ROUTE_NODE *RTAB4_Setup_New_Node(const ROUTE_NODE *n)
{
    ROUTE_NODE          *new_node;

    /* Create a new node */
    new_node = RTAB4_Create_Node(n->rt_list_head);

    /* make sure that it was allocated */
    if (new_node != NULL_ROUTE_NODE)
    {
        memcpy(new_node->rt_ip_addr, n->rt_ip_addr, IP_ADDR_LEN);
        memcpy(new_node->rt_submask, n->rt_submask, IP_ADDR_LEN);

        new_node->rt_submask_length = n->rt_submask_length;
    }

    return (new_node);

} /* RTAB4_Setup_New_Node */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Insert_Route_Entry
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
INT RTAB4_Insert_Route_Entry(ROUTE_NODE *r, const ROUTE_NODE *n)
{
    RTAB4_ROUTE_ENTRY   *current_route, *new_route;
    INT                 status = NU_SUCCESS;

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
        if (current_route->rt_gateway_v4.sck_addr ==
            ((RTAB4_ROUTE_ENTRY*)(n->rt_list_head))->rt_gateway_v4.sck_addr)
            break;

        current_route = current_route->rt_entry_next;

    } while (current_route);

    /* If a route was found with the same next-hop, return an error */
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
        new_route = RTAB4_Create_Route_Entry(r, n->rt_list_head);

        if (new_route != NU_NULL)
        {
            /* Add the entry to the list of routes for the node */
            DLL_Enqueue(&r->rt_route_entry_list, new_route);
        }

        else
            status = NU_MEM_ALLOC;
    }

    return (status);

} /* RTAB4_Insert_Route_Entry */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Create_Route_Entry
*
*   DESCRIPTION
*
*       This function creates a new RTAB4_ROUTE_ENTRY data structure.
*
*   INPUTS
*
*       *route_node             A pointer to the route node.
*       *rt_entry               A pointer to the data structure from
*                               which to copy the new elements of the
*                               new data structure.
*
*   OUTPUTS
*
*       *RTAB4_ROUTE_ENTRY      A pointer to the new entry.
*       NU_NULL                 The memory could not be allocated.
*
*************************************************************************/
STATIC RTAB4_ROUTE_ENTRY *RTAB4_Create_Route_Entry(ROUTE_NODE *route_node,
                                                   const RTAB4_ROUTE_ENTRY *rt_entry)
{
    RTAB4_ROUTE_ENTRY   *new_route;
    STATUS              status;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* Allocate memory for the new entry and fill-in the elements */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&new_route,
                           sizeof(RTAB4_ROUTE_ENTRY),
                           NU_NO_SUSPEND);
#else
    /* Assign memory*/
    new_route  = DLL_Dequeue(&NET_RTAB_Route_Free_List) ;

    if (new_route != NU_NULL)
        status = NU_SUCCESS;
    else
        status = NU_NO_MEMORY;
#endif

    if (status == NU_SUCCESS)
    {
        if (rt_entry)
        {
            NU_BLOCK_COPY(new_route, rt_entry, sizeof(RTAB4_ROUTE_ENTRY));

            new_route->rt_entry_parms.rt_parm_clock = NU_Retrieve_Clock();

            new_route->rt_route_node = route_node;
        }
    }

    /* Memory could not be allocated */
    else
        new_route = NU_NULL;

    return (new_route);

} /* RTAB4_Create_Route_Entry */

#endif
