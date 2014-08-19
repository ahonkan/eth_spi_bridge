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
*       rtab4_sdr.c
*
*   COMPONENT
*
*       IPv4 Routing
*
*   DESCRIPTION
*
*       This module contains the functions for setting the default route.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB4_Set_Default_Route
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

#if (INCLUDE_IPV4 == NU_TRUE)

extern ROUTE_NODE   *RTAB4_Default_Route;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
extern RTAB4_ROUTE_ENTRY    NET_RTAB_Route_Free_List;
extern ROUTE_NODE           NET_RTAB_Node_Free_List;
#endif

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Set_Default_Route
*
*   DESCRIPTION
*
*       Sets the default route, called by the NU_Rip2 function.
*
*   INPUTS
*
*       *device                 A pointer to the device
*       gw                      The gateway
*       flags                   The flags to be set on a route
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       status                  Failure
*
*************************************************************************/
STATUS RTAB4_Set_Default_Route(DV_DEVICE_ENTRY *device, UINT32 gw,
                               UINT32 flags)
{
    VOID                *pointer;
    STATUS              status;
    RTAB4_ROUTE_ENTRY   *rt_entry;

#if (INCLUDE_SR_SNMP == NU_TRUE)
    UINT8               gw_ptr[IP_ADDR_LEN];
    UINT8               submask[IP_ADDR_LEN];
#endif

    if (!device)
        return NU_INVALID_PARM;

    if (!RTAB4_Default_Route)
    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        status = NU_Allocate_Memory(MEM_Cached, (VOID **)&pointer,
                                    sizeof(ROUTE_NODE), (UNSIGNED)NU_NO_SUSPEND);
#else
        /* Assign memory from the free list */
        pointer  = DLL_Dequeue(&NET_RTAB_Node_Free_List) ;

        if (pointer == NU_NULL)
            status = NU_NO_MEMORY;
        else
            status = NU_SUCCESS;
#endif
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("RTAB4_Set_Default_Route insufficient memory",
                           NERR_SEVERE, __FILE__, __LINE__);

            return (status);        /* could not get the memory */
        }

        UTL_Zero(pointer, sizeof(ROUTE_NODE));
        RTAB4_Default_Route = (ROUTE_NODE *)pointer;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        status = NU_Allocate_Memory(MEM_Cached, (VOID**)&pointer,
                                    sizeof(RTAB4_ROUTE_ENTRY), NU_NO_SUSPEND);
#else
        /* Assign memory from the free memory*/
        pointer  = DLL_Dequeue(&NET_RTAB_Route_Free_List) ;

        if (pointer == NU_NULL)
            status = NU_NO_MEMORY;
        else
            status = NU_SUCCESS;
#endif

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("RTAB4_Set_Default_Route insufficient memory",
                           NERR_SEVERE, __FILE__, __LINE__);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
            /* Deallocate the memory allocated above */
            if (NU_Deallocate_Memory(RTAB4_Default_Route) != NU_SUCCESS)
                NLOG_Error_Log("RTAB4_Set_Default_Route failed to deallocate memory",
                               NERR_SEVERE, __FILE__, __LINE__);
#else
            /* Put the unused memory back onto the memory free list */
            DLL_Enqueue(&NET_RTAB_Node_Free_List, RTAB4_Default_Route);
#endif

            /* Set the default route to NULL */
            RTAB4_Default_Route = NU_NULL;

            return (status);        /* could not get the memory */
        }

        UTL_Zero(pointer, sizeof(RTAB4_ROUTE_ENTRY));
        RTAB4_Default_Route->rt_list_head = (RTAB4_ROUTE_ENTRY *)pointer;
    }

    rt_entry = RTAB4_Default_Route->rt_list_head;

    rt_entry->rt_entry_parms.rt_parm_flags    = (INT16)(flags | RT_UP | RT_GATEWAY);
    rt_entry->rt_entry_parms.rt_parm_refcnt   = 0;
    rt_entry->rt_entry_parms.rt_parm_metric   = 1;
    rt_entry->rt_entry_parms.rt_parm_routetag = 0;
    rt_entry->rt_entry_parms.rt_parm_use      = 0;
    rt_entry->rt_entry_parms.rt_parm_clock    = 0;
    rt_entry->rt_entry_parms.rt_parm_device = device;
    rt_entry->rt_route_node = RTAB4_Default_Route;

    rt_entry->rt_gateway_v4.sck_addr = gw;
    rt_entry->rt_gateway_v4.sck_family = NU_FAMILY_IP;

    rt_entry->rt_gateway_v4.sck_len = sizeof(rt_entry->rt_gateway_v4);

    rt_entry->rt_path_mtu = device->dev_mtu;

    /* The destination IP address of a default route should be set to 0. */
    *(UINT32*)RTAB4_Default_Route->rt_ip_addr = 0;

    /* The subnet mask is all zero's */
    RTAB4_Default_Route->rt_submask_length = 0;

#if (INCLUDE_SR_SNMP == NU_TRUE)

    PUT32(gw_ptr, 0, gw);
    memset(submask, 0, IP_ADDR_LEN);

    /* Add this route to the SNMP routing table */
    SNMP_ipRouteTableUpdate(SNMP_ADD, (UINT32)(device->dev_index),
                            RTAB4_Default_Route->rt_ip_addr,
                            (INT32)rt_entry->rt_metric,
                            (UNSIGNED)-1, (UNSIGNED)-1, (UNSIGNED)-1,
                            (UNSIGNED)-1, gw_ptr, 4, 2, 0, submask, "0.0");
#endif

    return (NU_SUCCESS);

} /* RTAB4_Set_Default_Route */

#endif
