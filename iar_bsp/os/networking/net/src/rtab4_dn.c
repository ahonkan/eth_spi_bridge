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
*       rtab4_dn.c
*
*   COMPONENT
*
*       IPv4 Routing
*
*   DESCRIPTION
*
*       This module contains the function for deleting an IPv4 route
*       node.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB4_Delete_Node
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

extern ROUTE_NODE *RTAB4_Default_Route;
extern RTAB_ROUTE_PARMS RTAB4_Parms;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* lists already declared in rtab4_ar.c */
extern RTAB4_ROUTE_ENTRY    NET_RTAB_Route_Free_List;
extern ROUTE_NODE           NET_RTAB_Node_Free_List;
#endif

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Delete_Node
*
*   DESCRIPTION
*
*       This function deletes a node from the IPv4 routing table.
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
STATUS RTAB4_Delete_Node(ROUTE_NODE *rt_node)
{
    STATUS  status;

    /* If the node to delete is the Default Route */
    if (rt_node == RTAB4_Default_Route)
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
            RTAB4_Default_Route = NU_NULL;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
            /* Deallocate the memory used by the Default Route */
            if (NU_Deallocate_Memory(rt_node->rt_list_head) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for default route",
                               NERR_FATAL, __FILE__, __LINE__);

            if (NU_Deallocate_Memory(rt_node) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for default route",
                               NERR_FATAL, __FILE__, __LINE__);
#else
        /* Put the unused memory back onto the memory free list */
         DLL_Enqueue(&NET_RTAB_Route_Free_List, rt_node->rt_list_head);
         DLL_Enqueue(&NET_RTAB_Node_Free_List, rt_node);
#endif
        }
    }

    else
        status = RTAB_Delete_Node(rt_node, &RTAB4_Parms);

    return (status);

} /* RTAB4_Delete_Node */

#endif
