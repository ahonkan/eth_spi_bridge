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
*       nud6.c                                       
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains those functions necessary to perform Neighbor
*       Unreachability Detection as per RFC 4861.
*
*       RFC 4861 section 3:
*       Neighbor Unreachability Detection detects the failure of a 
*       neighbor or the failure of the forward path to the neighbor.  
*       Doing so requires positive confirmation that packets sent to a 
*       neighbor are actually reaching that neighbor and being processed 
*       properly by its IP layer.  Neighbor Unreachability Detection uses 
*       confirmation from two sources.  When possible, upper-layer 
*       protocols provide a positive confirmation that a connection is 
*       making forward progress, that is, previously sent data is known to 
*       have been delivered correctly.  When positive confirmation is not 
*       forthcoming through such hints, a node sends unicast Neighbor 
*       Solicitation messages that solicit Neighbor Advertisements as 
*       reachability confirmation from the next hop.  To reduce 
*       unnecessary network traffic, probe messages are sent only to 
*       neighbors to which the node is actively sending packets.
*                                                                          
*   DATA STRUCTURES                                                          
*                                      
*       NUD6_Probe_Index                                    
*                                                                          
*   FUNCTIONS                                                                
*        
*       NUD6_Init
*       NUD6_Handle_Event
*       NUD6_Check_Neighbors
*       NUD6_Probe_Node
*       NUD6_Confirm_Reachability
*       NUD6_Confirm_Reachability_By_IP_Addr
*       NUD6_Stale_Neighbor
*       NUD6_Stop_Probing
*                                                                          
*   DEPENDENCIES                                                             
*               
*       externs.h
*       in6.h
*       nc6.h
*       nud6.h
*       nd6nsol.h
*       nc6_eth.h
*       defrtr6.h
*                                                                          
*************************************************************************/

#include "networking/externs.h"
#include "networking/in6.h"
#include "networking/nc6.h"
#include "networking/nud6.h"
#include "networking/nd6nsol.h"
#include "networking/nc6_eth.h"
#include "networking/defrtr6.h"

UINT32      NUD6_Probe_Index = 1;

static  INT         NUD6_Init_Complete = 0;
static  TQ_EVENT    NUD6_Probe_Node_Event;

TQ_EVENT    NUD6_Check_Neighbors_Event;

/*************************************************************************
*
*   FUNCTION
*
*       NUD6_Init
*
*   DESCRIPTION
*
*       This function initializes all data structures for Neighbor
*       Unreachability Detection.
*
*   INPUTS
*
*       *device                 A pointer to the device for which to
*                               initialize Neighbor Unreachability
*                               Detection.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID NUD6_Init(const DV_DEVICE_ENTRY *device)
{
    /* If the events have not already been registered */
    if (NUD6_Init_Complete == 0)
    {
        /* Register the event to check neighbors for reachability */
        if (EQ_Register_Event(NUD6_Handle_Event, 
                              &NUD6_Check_Neighbors_Event) != NU_SUCCESS)
            NLOG_Error_Log("Failed to register NUD event", NERR_SEVERE, 
                           __FILE__, __LINE__);

        /* Register the event to probe a specific node */
        if (EQ_Register_Event(NUD6_Handle_Event, 
                              &NUD6_Probe_Node_Event) != NU_SUCCESS)
            NLOG_Error_Log("Failed to register NUD event", NERR_SEVERE, 
                           __FILE__, __LINE__);
                           
        /* Set the initial event to check all neighbors for reachability */
        if (TQ_Timerset(NUD6_Check_Neighbors_Event, device->dev_index, 
                        device->dev6_reachable_time, 0) != NU_SUCCESS)
            NLOG_Error_Log("Failed to set the NUD timer to check neighbors", 
                           NERR_SEVERE, __FILE__, __LINE__);

        NUD6_Init_Complete = 1;
    }
} /* NUD6_Init */

/*************************************************************************
*
*   FUNCTION
*
*       NUD6_Handle_Event
*
*   DESCRIPTION
*
*       This routine handles all Neighbor Unreachability Detection
*       events upon expiration of the event timer.
*
*   INPUTS
*
*       event                   The event to handle.
*       data                    Data passed in to the event.
*       extra_data              Data passed in to the event.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID NUD6_Handle_Event(TQ_EVENT event, UNSIGNED data, UNSIGNED extra_data)
{
    /* Check all neighbors for reachability */
    if (event == NUD6_Check_Neighbors_Event)
        NUD6_Check_Neighbors((UINT32)data);

    /* Probe a specific node */
    else if (event == NUD6_Probe_Node_Event)
        NUD6_Probe_Node((UINT32)data, (UINT32)extra_data);

} /* NUD6_Handle_Event */

/*************************************************************************
*
*   FUNCTION
*
*       NUD6_Check_Neighbors
*
*   DESCRIPTION
*
*       This task traverses the Neighbor Cache of each device for those
*       entries that have not been confirmed reachable in the last
*       IP6_REACHABLE_TIME seconds.  When an entry is found that is in
*       the REACHABLE state, the state is changed to STALE.
*
*   INPUTS
*
*       dev_index               The interface index of the device on
*                               which to check neighbors.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID NUD6_Check_Neighbors(UINT32 dev_index)
{
    DV_DEVICE_ENTRY     *device;
    UNSIGNED            current_clock;
    INT16               i;
    UNSIGNED            next_time = 0, expire_time;

    device = DEV_Get_Dev_By_Index(dev_index);

    if (device != NU_NULL)
    {
        current_clock = NU_Retrieve_Clock();

        for (i = 0; i < device->dev6_nc_entries; i++)
        {           
            /* If the entry is not permanent, and the last reachability 
             * confirmation was received more than dev6_reachable_time seconds 
             * ago, change the state of the entry.
             */
            if ( ((device->dev6_neighbor_cache[i].ip6_neigh_cache_flags & NC_PERMANENT) == 0) &&
                 (device->dev6_neighbor_cache[i].ip6_neigh_cache_timestamp != 0) )
            {
                expire_time = 
                    current_clock - device->dev6_neighbor_cache[i].ip6_neigh_cache_nud_time;

                if (expire_time >= device->dev6_reachable_time)
                {
                    /* RFC 4861 section 7.3.3 - When dev6_reachable_time seconds have
                     * passed since receipt of the last reachability confirmation for
                     * a neighbor, the Neighbor Cache entry's state changes from 
                     * REACHABLE to STALE.
                     */
                    if (device->dev6_neighbor_cache[i].ip6_neigh_cache_state == NC_NEIGH_REACHABLE)
                        device->dev6_neighbor_cache[i].ip6_neigh_cache_state = NC_NEIGH_STALE;
                }

                /* Find the entry that expires the soonest, and recheck the neighbors
                 * again at that time.
                 */
                else if (expire_time > next_time)   
                {
                    next_time = 
                        device->dev6_reachable_time - expire_time;
                }
            }
        }

        /* If all Neighbor Cache entries are expired, check the Neighbor Cache
         * again in Reachable Time.
         */
        if (next_time == 0)
            next_time = device->dev6_reachable_time;

        /* Set the timer to traverse the list in dev6_reachable_time seconds */
        if (TQ_Timerset(NUD6_Check_Neighbors_Event, device->dev_index, 
                        next_time, 0) != NU_SUCCESS)
            NLOG_Error_Log("Failed to set the NUD timer to check neighbors", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }

} /* NUD6_Check_Neighbors */

/*************************************************************************
*
*   FUNCTION
*
*       NUD6_Probe_Node
*
*   DESCRIPTION
*
*       This function probes the node associated with the Neighbor Cache
*       entry corresponding to the probe_index passed to the function. 
*       If the state of the entry is DELAY or PROBE, a unicast Neighbor
*       Solicitation message is sent to the node.  If the number of 
*       messages transmitted is less than IP6_MAX_UNICAST_SOLICITATIONS,
*       a timer is set to transmit another Neighbor Solicitation. 
*       Otherwise, if the number of messages transmitted is equal to
*       IP6_MAX_UNICAST_SOLICITATIONS, and the state of the entry is 
*       PROBE, the Neighbor Cache entry is deleted.
*
*   INPUTS
*
*       probe_index             The value of ip6_neigh_cache_probe_index 
*                               of the target Neighbor Cache entry.
*       dev_index               The index of the device on which the 
*                               Neighbor Cache entry exists.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID NUD6_Probe_Node(UINT32 probe_index, UINT32 dev_index)
{
    DV_DEVICE_ENTRY             *device;
    INT16                       i;
    UINT8                       *source_addr;
    IP6_DEFAULT_ROUTER_ENTRY    *def_rtr;

    /* Get a pointer to the device associated with the index provided */
    device = DEV_Get_Dev_By_Index(dev_index);

    if (device == NU_NULL)
        return;
   
    /* Traverse the Neighbor Cache searching for an entry matching the 
     * probe_index provided.
     */
    for (i = 0; i < device->dev6_nc_entries; i++)
    {
        /* Check if this is the target entry. */
        if (device->dev6_neighbor_cache[i].ip6_neigh_cache_probe_index 
            == probe_index)
        {
            /* RFC 4861 Section 5.1 STALE - no attempt should be made
             * to verify the reachability until traffic is sent to 
             * the neighbor
             */
            if (device->dev6_neighbor_cache[i].ip6_neigh_cache_state 
                == NC_NEIGH_STALE)
            {
                device->dev6_neighbor_cache[i].ip6_neigh_cache_unans_probes = 0;
                device->dev6_neighbor_cache[i].ip6_neigh_cache_probe_index = 0;         
            }
            
            /* If the maximum number of Neighbor Solicitations have not 
             * been transmitted 
             */
            else if (device->dev6_neighbor_cache[i].ip6_neigh_cache_unans_probes 
                     < IP6_MAX_UNICAST_SOLICIT)
            {
                /* If the state is not REACHABLE, a solicited Neighbor 
                 * Advertisement has not been received.  Send a Neighbor 
                 * Solicitation message.
                 */
                if (device->dev6_neighbor_cache[i].ip6_neigh_cache_state 
                    != NC_NEIGH_REACHABLE)
                {
                    /* Set the state to PROBE */
                    device->dev6_neighbor_cache[i].ip6_neigh_cache_state = 
                        NC_NEIGH_PROBE;  
   
                    /* Get the link-local address for the interface.  Use 
                     * the function in6_ifawithifp so a TENTATIVE link-local 
                     * address will not be returned.
                     */
                    source_addr = in6_ifawithifp(device, 
                                                 device->dev6_neighbor_cache[i].
                                                 ip6_neigh_cache_ip_addr);
    
                    /* If no link-local address exists on the interface, exit */
                    if (source_addr != NU_NULL)
                    {
                        /* Transmit the unicast Neighbor Solicitation */
                        if (ND6NSOL_Output(device, 
                                           device->dev6_neighbor_cache[i].
                                           ip6_neigh_cache_ip_addr,
                                           device->dev6_neighbor_cache[i].
                                           ip6_neigh_cache_ip_addr, 
                                           source_addr, NU_FALSE) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to transmit the Neighbor Solicitation message", 
                                           NERR_SEVERE, __FILE__, __LINE__);

                        /* Increment the number of probes transmitted */
                        device->dev6_neighbor_cache[i].ip6_neigh_cache_unans_probes++;
        
                        /* RFC 4861 section 7.3.3 - Upon entering the PROBE state,
                         * a node sends a unicast Neighbor Solicitation message to 
                         * the neighbor using the cache link-layer address.  While 
                         * in the PROBE state, a node retransmits Neighbor Solicitation 
                         * messages every RetransTimer milliseconds until reachability 
                         * confirmation is obtained.
                         */
                        if (TQ_Timerset(NUD6_Probe_Node_Event, 
                                        device->dev6_neighbor_cache[i].
                                        ip6_neigh_cache_probe_index, 
                                        device->dev6_retrans_timer, 
                                        (UNSIGNED)dev_index) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to set the NUD timer to probe the node", 
                                           NERR_SEVERE, __FILE__, __LINE__);
                    }
                    else
                    {
                        NLOG_Error_Log("Cannot probe neighbor - no IP address on interface.", 
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                /* Otherwise, a Reachability confirmation has been received.  
                 * Reset the probe information.
                 */
                else
                {
                    device->dev6_neighbor_cache[i].ip6_neigh_cache_unans_probes = 0;
                    device->dev6_neighbor_cache[i].ip6_neigh_cache_probe_index = 0;
                }
            }

            /* RFC 4861 section 7.3.3 - If no response is received after waiting
             * RetransTimer milliseconds after sending the MAX_UNICAST_SOLICIT
             * solicitations, retransmissions cease and the entry SHOULD be deleted.
             */
            else if (device->dev6_neighbor_cache[i].ip6_neigh_cache_state == 
                     NC_NEIGH_PROBE)
            {
                /* If this is a Default Router, delete the Default Router.
                 * the code to delete the router will take care of cleaning
                 * up the routes using this Default Router and the Neighbor 
                 * Cache.
                 */
                def_rtr = 
                    DEFRTR6_Find_Default_Router_Entry(device->dev6_neighbor_cache[i].
                                                      ip6_neigh_cache_ip_addr);

                if (def_rtr)
                {
                    /* Delete the Default Router. */
                    DEFRTR6_Delete_Entry(def_rtr);
                }

                /* Although the node is not a Default Router, it could still
                 * be used as a next-hop through manual routing entries.
                 */
                else
                {
                    /* Delete all routes using this node as a next-hop to 
                     * invoke next-hop determination the next time data is 
                     * sent to the respective node.
                     */
                    RTAB6_Delete_Route_By_Gateway(device->dev6_neighbor_cache[i].
                                                  ip6_neigh_cache_ip_addr);

                    /* Delete the associated Neighbor Cache entry. */
                    device->dev6_del_neighcache_entry(device, 
                                                      device->dev6_neighbor_cache[i].
                                                      ip6_neigh_cache_ip_addr);
                }
            }

            else
            {
                device->dev6_neighbor_cache[i].ip6_neigh_cache_unans_probes = 0;
                device->dev6_neighbor_cache[i].ip6_neigh_cache_probe_index = 0;
            }

            /* The target entry has been found and processed, so break. */
            break;
        }
    }    

} /* NUD6_Probe_Node */

/*************************************************************************
*
*   FUNCTION
*
*       NUD6_Confirm_Reachability
*
*   DESCRIPTION
*
*       This function updates the reachability state of a Neighbor Cache
*       entry to REACHABLE and updates the time associated with
*       reachability to the current time.
*
*   INPUTS
*
*       *nc_entry               A pointer to the Neighbor Cache entry 
*                               to update.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID NUD6_Confirm_Reachability(IP6_NEIGHBOR_CACHE_ENTRY *nc_entry)
{
    /* Set the state to REACHABLE */
    nc_entry->ip6_neigh_cache_state = NC_NEIGH_REACHABLE;

    /* Update the time indicating when the last reachability confirmation was
     * received.
     */
    nc_entry->ip6_neigh_cache_nud_time = NU_Retrieve_Clock();

} /* NUD6_Confirm_Reachability */

/*************************************************************************
*
*   FUNCTION
*
*       NUD6_Confirm_Reachability_By_IP_Addr
*
*   DESCRIPTION
*
*       This function updates the reachability state of a Neighbor Cache
*       entry to REACHABLE and updates the time associated with
*       reachability to the current time.  This function is used 
*       specifically by TCP to inform NUD that the forward path to
*       the next-hop is functional.
*
*   INPUTS
*
*       *nc_entry               A pointer to the Neighbor Cache entry to 
*                               update.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID NUD6_Confirm_Reachability_By_IP_Addr(const UINT8 *ip_addr)
{
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Find the route associated with the IP address */
    rt_entry = RTAB6_Find_Route(ip_addr, RT_HOST_MATCH | RT_BEST_METRIC);
  
    /* If an entry exists, update the reachability information.
     * RFC 4861 section 7.3.3 - The one exception is that upper-layer advice
     * has no effect on entries in the INCOMPLETE state.
     */
    if (rt_entry)
    {
        if ( (rt_entry->rt_next_hop_entry) && 
             (rt_entry->rt_next_hop_entry->ip6_neigh_cache_state != NC_NEIGH_INCOMPLETE) )
            NUD6_Confirm_Reachability(rt_entry->rt_next_hop_entry);

        RTAB_Free((ROUTE_ENTRY*)rt_entry, NU_FAMILY_IP6);
    }

} /* NUD6_Confirm_Reachability_By_IP_Addr */

/*************************************************************************
*
*   FUNCTION
*
*       NUD6_Stale_Neighbor
*
*   DESCRIPTION
*
*       This function is called when a packet is being sent to a neighbor
*       whose reachability state is STALE.
*
*       RFC 4861 section 7.7.3 - The first time a node sends a packet to
*       a neighbor whose entry is STALE, the sender changes the state to
*       DELAY and sets a timer to expire in DELAY_FIRST_PROBE_TIME seconds.
*
*   INPUTS
*
*       *nc_entry               A pointer to the Neighbor Cache entry to 
*                               update.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID NUD6_Stale_Neighbor(IP6_NEIGHBOR_CACHE_ENTRY *nc_entry)
{
    /* If the neighbor is not already being probed, set a timer to probe 
     * the neighbor. 
     */
    if (nc_entry->ip6_neigh_cache_probe_index == 0)
    {
        nc_entry->ip6_neigh_cache_state = NC_NEIGH_DELAY;

        /* Set the probe index */
        nc_entry->ip6_neigh_cache_probe_index = NUD6_Probe_Index++;

        /* Set the timer to probe the node */
        if (TQ_Timerset(NUD6_Probe_Node_Event, 
                        nc_entry->ip6_neigh_cache_probe_index, 
                        IP6_DELAY_FIRST_PROBE_TIME, 
                        (UNSIGNED)(nc_entry->ip6_neigh_cache_device->dev_index)) != NU_SUCCESS)
            NLOG_Error_Log("Failed to set the NUD timer to probe the neighbor", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }

} /* NUD6_Stale_Neighbor */

/*************************************************************************
*
*   FUNCTION
*
*       NUD6_Stop_Probing
*
*   DESCRIPTION
*
*       This function cancels a NUD probe.  This routine is called when
*       the state of an entry that is currently being probed changes due
*       to an incoming packet.
*
*   INPUTS
*
*       *nc_entry               A pointer to the Neighbor Cache entry to 
*                               update.
*       dev_index               The index of the interface associated with
*                               the Neighbor Cache entry.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID NUD6_Stop_Probing(IP6_NEIGHBOR_CACHE_ENTRY *nc_entry, UINT32 dev_index)
{
    /* Unset the probe timer. */
    if (TQ_Timerunset(NUD6_Probe_Node_Event, TQ_CLEAR_EXACT, 
                      nc_entry->ip6_neigh_cache_probe_index, 
                      dev_index) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to clear the probe timer", NERR_SEVERE, 
                       __FILE__, __LINE__);
    }

    /* Reset the number of unanswered probes. */
    nc_entry->ip6_neigh_cache_unans_probes = 0;

    /* Clear the probe index. */
    nc_entry->ip6_neigh_cache_probe_index = 0;   

} /* NUD6_Stop_Probing */
