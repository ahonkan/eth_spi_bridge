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
*       rtab6_sdr.c                                  
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
*       RTAB6_Set_Default_Route
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*       nc6.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/nc6.h"

extern ROUTE_NODE   *RTAB6_Default_Route;

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Set_Default_Route                                          
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Sets the IPv6 default route.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *device                 The device to use for the default route.
*       next_hop                The gateway of the default route.
*       flags                   The flags associated with the route.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              The default route was added.
*       NU_NO_MEMORY            There is not enough memory to create a 
*                               default route.
*                                                                       
*************************************************************************/
STATUS RTAB6_Set_Default_Route(DV_DEVICE_ENTRY *device, UINT8 *next_hop,
                               UINT32 flags)
{
    STATUS                      status;
    IP6_NEIGHBOR_CACHE_ENTRY    *next_hop_entry;
    RTAB6_ROUTE_ENTRY           *rt_entry;

    /* If there is not currently a default route, allocate memory for
     * the default route.
     */
    if (!RTAB6_Default_Route)
    {
        status = NU_Allocate_Memory(MEM_Cached, (VOID **)&RTAB6_Default_Route,
                                    sizeof(ROUTE_NODE), (UNSIGNED)NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Zero out the default route data structure */
            UTL_Zero(RTAB6_Default_Route, sizeof(ROUTE_NODE));

            status = NU_Allocate_Memory(MEM_Cached, (VOID**)&rt_entry,
                                        sizeof(RTAB6_ROUTE_ENTRY),
                                        NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
                UTL_Zero(rt_entry, sizeof(RTAB6_ROUTE_ENTRY));
            else
            {
                /* Deallocate the memory previously allocated */
                if (NU_Deallocate_Memory((VOID*)RTAB6_Default_Route) != NU_SUCCESS)
                    NLOG_Error_Log("Could not deallocate memory for the Default Route", 
                                   NERR_SEVERE, __FILE__, __LINE__);

                RTAB6_Default_Route = NU_NULL;
            }
        }
    }
    else
    {
        rt_entry = RTAB6_Default_Route->rt_list_head;
        status = NU_SUCCESS;
    }

    if (status == NU_SUCCESS)
    {      
        rt_entry->rt_entry_parms.rt_parm_device = device;

        /* Set the route as STATIC and UP. */
        rt_entry->rt_entry_parms.rt_parm_flags = (flags | RT_STATIC | RT_UP);

        /* RFC 1981 section 3 - ... a source node initially assumes
         * that the PMTU of a path is the (known) first hop of the
         * path.
         */
        rt_entry->rt_path_mtu = device->dev6_link_mtu;

        rt_entry->rt_next_hop.sck_family = NU_FAMILY_IP6;
        rt_entry->rt_next_hop.sck_len = sizeof(SCK6_SOCKADDR_IP);
        NU_BLOCK_COPY(rt_entry->rt_next_hop.sck_addr, next_hop, IP6_ADDR_LEN);

        rt_entry->rt_route_node = RTAB6_Default_Route;
        rt_entry->rt_entry_parms.rt_parm_metric = 1;
        rt_entry->rt_entry_parms.rt_parm_routetag = 0;

        RTAB6_Default_Route->rt_submask_length = 128;
        RTAB6_Default_Route->rt_list_head = rt_entry;
        
        /* Setup the Neighbor Cache entry information. */                
        next_hop_entry = device->dev6_fnd_neighcache_entry(device, next_hop);

        if (next_hop_entry == NU_NULL)
            next_hop_entry = device->dev6_add_neighcache_entry(device, next_hop,
                                                               NU_NULL, 0, NU_NULL,
                                                               NC_NEIGH_INCOMPLETE);

        /* If the entry was successfully added */
        if (next_hop_entry)
        {
            RTAB6_Add_DestList_Entry(&next_hop_entry->ip6_neigh_cache_dest_list,
                                     RTAB6_Default_Route->rt_list_head);

            rt_entry->rt_next_hop_entry = next_hop_entry;
        }
    }
    else
        NLOG_Error_Log("Could not allocate memory for the Default Route", 
                       NERR_SEVERE, __FILE__, __LINE__);

    return (status);

} /* RTAB6_Set_Default_Route */
