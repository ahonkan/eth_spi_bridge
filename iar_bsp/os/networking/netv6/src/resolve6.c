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
*       resolve6.c                                   
*                                                                                  
*   DESCRIPTION                                                              
*        
*       This file contains those functions necessary to resolve the 
*       link-layer address of a Neighbor.
*                                                                          
*   DATA STRUCTURES                                                          
*                                      
*       Resolve_ID
*                                                                          
*   FUNCTIONS                                                                
*         
*       Resolve6_Init
*       Resolve6_Event
*       Resolve6_Get_Link_Addr
*                                                                          
*   DEPENDENCIES                                                             
*               
*       target.h
*       externs.h
*       nd6nsol.h
*       in6.h
*       resolve6.h
*       net6.h
*       nc6.h
*       nc6_eth.h
*       nud6.h
*                                                                          
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/nd6nsol.h"
#include "networking/in6.h"
#include "networking/resolve6.h"
#include "networking/net6.h"
#include "networking/nc6.h"
#include "networking/nc6_eth.h"
#include "networking/nud6.h"

static  UINT16      Resolve_ID;

TQ_EVENT    IP6_Resolve6_Event;

extern UINT8    IP6_Solicited_Node_Multi[];

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       Resolve6_Init
*                                                                         
*   DESCRIPTION                                                           
*             
*       This function initializes the data structures and global 
*       variables used in the Resolve Module.
*                                                                         
*   INPUTS                                                                
*                        
*       None
*                              
*   OUTPUTS                                                               
*           
*       None                                
*
*************************************************************************/
VOID Resolve6_Init(VOID)
{
    Resolve_ID = 1;

    /* Register the event to invoke address resolution */
    if (EQ_Register_Event(Resolve6_Event, &IP6_Resolve6_Event) != NU_SUCCESS)
        NLOG_Error_Log("Failed to register Address Resolution event", NERR_SEVERE, 
                       __FILE__, __LINE__);

} /* Resolve6_Init */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       Resolve6_Event
*                                                                         
*   DESCRIPTION                                                           
*
*       This is the driving function of Address Resolution for IPv6.
*       If the maximum number of Neighbor Solicitations have not been
*       transmitted for a Neighbor, this function transmits another
*       Neighbor Solicitation.  Otherwise, this function deallocates
*       all data structures and memory used to resolve the link-layer
*       address of the Neighbor.
*                                                                         
*   INPUTS                                                                
*                        
*       id                      The Address Resolution ID of the Neighbor 
*                               to resolve.
*       dev_index               The index associated with the device on 
*                               which to resolve the address.
*                              
*   OUTPUTS                                                               
*                                           
*       None
*
*************************************************************************/
VOID Resolve6_Event(TQ_EVENT event, UNSIGNED id, UNSIGNED dev_index)
{
    IP6_NEIGHBOR_CACHE_ENTRY        *nc_entry = NU_NULL;
    UINT8                           dest_address[IP6_ADDR_LEN];
    INT16                           i;
    DV_DEVICE_ENTRY                 *device;
    DEV6_IF_ADDRESS                 *link_local_addr;
    UINT8                           *source_addr, *target_addr;

    UNUSED_PARAMETER(event);

    /* Get a pointer to the device associated with the device index */
    device = DEV_Get_Dev_By_Index((UINT32)dev_index);

    if (device)
    {       
        /* Search for the matching entry. */
        for (i = 0; i < device->dev6_nc_entries; i++)
        {
            if (device->dev6_neighbor_cache[i].ip6_neigh_cache_resolve_id == id)
            {
                nc_entry = &device->dev6_neighbor_cache[i];
                break;
            }
        }        

        /* If a matching entry was found, send a Neighbor Solicitation to
         * the unicast IP address of the entry requesting the node's link-layer
         * address.
         */
        if (nc_entry)
        {
            /* If we have not already sent the maximum number of Neighbor 
             * Solicitations allowed to be transmitted when resolving an address,
             * send a Neighbor Solicitation.
             */
            if (nc_entry->ip6_neigh_cache_rsend_count < IP6_MAX_MULTICAST_SOLICIT)
            {
                /* RFC 4861 - section 7.2.2 - If the source address of the packet
                 * prompting the solicitation is the same as one of the addresses 
                 * assigned to the outgoing interface, that address SHOULD be placed 
                 * in the IP Source Address of the outgoing solicitation.
                 */
                if (DEV6_Get_Dev_By_Addr(&nc_entry->ip6_neigh_cache_packet_list.head->
                                         data_ptr[IP6_SRCADDR_OFFSET]) == device)
                {
                    source_addr = 
                        &nc_entry->ip6_neigh_cache_packet_list.head->
                        data_ptr[IP6_SRCADDR_OFFSET];

                    target_addr = nc_entry->ip6_neigh_cache_ip_addr;
                }

                /* Otherwise, any one of the addresses assigned to the interface 
                 * should be used.
                 */
                else
                {
                    /* Use the function in6_ifawithifp so a TENTATIVE address of 
                     * link-local scope will not be returned.
                     */
                    source_addr = 
                        in6_ifawithifp(device, nc_entry->ip6_neigh_cache_ip_addr);

                    /* If an address of the same scope does not exist, get the 
                     * link-local address.
                     */
                    if (source_addr == NU_NULL)
                    {
                        link_local_addr = IP6_Find_Link_Local_Addr(device);

                        /* Check that the address is not TENTATIVE */
                        if ( (link_local_addr) && 
                             (!(link_local_addr->dev6_addr_state & DV6_TENTATIVE)) )
                            source_addr = link_local_addr->dev6_ip_addr;
                    }

                    target_addr = nc_entry->ip6_neigh_cache_ip_addr;
                }
    
                /* If an address exists on the interface, send the Neighbor Solicitation */
                if (source_addr != NU_NULL)
                {
                    /* If the state of the entry is still INCOMPLETE, send the
                     * Neighbor Solicitation message to the Solicited-Node mulitcast
                     * address.
                     */
                    if (nc_entry->ip6_neigh_cache_state == NC_NEIGH_INCOMPLETE)
                    {
                        /* Setup the Solicited Node Multicast address of the target 
                         * node's link-local address as the destination. 
                         */
                        NU_BLOCK_COPY(dest_address, IP6_Solicited_Node_Multi, 
                                      IP6_ADDR_LEN);
        
                        dest_address[13] = nc_entry->ip6_neigh_cache_ip_addr[13];
                        dest_address[14] = nc_entry->ip6_neigh_cache_ip_addr[14];
                        dest_address[15] = nc_entry->ip6_neigh_cache_ip_addr[15];
                    }

                    /* Otherwise, send a unicast Neighbor Solicitation */
                    else
                    {
                        NU_BLOCK_COPY(dest_address, 
                                      nc_entry->ip6_neigh_cache_ip_addr, 
                                      IP6_ADDR_LEN);
                    }

                    /* Transmit the Neighbor Solicitation */
                    if (ND6NSOL_Output(nc_entry->ip6_neigh_cache_device, dest_address, 
                                       target_addr, source_addr, 
                                       NU_FALSE) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to transmit the Neighbor Solicitation", 
                                       NERR_SEVERE, __FILE__, __LINE__);
        
                    /* Increment the number of Neighbor Solicitation messages sent. */
                    nc_entry->ip6_neigh_cache_rsend_count++;
    
                    /* Set a timer event to send the next one. */
                    if (TQ_Timerset(IP6_Resolve6_Event, (UNSIGNED)id, 
                                    device->dev6_retrans_timer, 
                                    (UNSIGNED)(device->dev_index)) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to set the timer to resolve the link-layer address", 
                                       NERR_SEVERE, __FILE__, __LINE__);
                }   

                /* Otherwise, Address Resolution was not successful, so delete the entry */
                else
                {
                    NLOG_Error_Log("Address Resolution aborted - no link-local address on the interface", 
                                   NERR_SEVERE, __FILE__, __LINE__);
    
                    if (NC6ETH_Delete_NeighCache_Entry(device, 
                                                       nc_entry->ip6_neigh_cache_ip_addr) 
                                                       != NU_SUCCESS)
                        NLOG_Error_Log("Failed to delete the Neighbor Cache entry", 
                                       NERR_SEVERE, __FILE__, __LINE__);
                }
            }   
    
            /* Otherwise, the maximum number of Neighbor Solicitations have been
             * sent to this node for this resolve session.  Clean up.
             */
            else
            {  
                NLOG_Error_Log("The maximum number of Neighbor Solicitation messages have been transmitted to the destination", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                /* Delete the route to the destination that uses the invalid
                 * Neighbor Cache entry as a next-hop.  These two addresses
                 * will be the same if the destination is on-link.
                 */
                if (RTAB6_Delete_Route(&nc_entry->ip6_neigh_cache_packet_list.
                                       head->data_ptr[IP6_DESTADDR_OFFSET], 
                                       nc_entry->ip6_neigh_cache_ip_addr) != NU_SUCCESS)
                    NLOG_Error_Log("Could not delete route", NERR_SEVERE, __FILE__, __LINE__);

                /* RFC 4861 section 7.3.3 - If Address Resolution fails, the entry
                 * should be deleted so that subsequent traffic to that neighbor
                 * invokes Next-Hop Determination again.  Invoking Next-Hop Determination
                 * again insures that alternate Default Routers are tried.
                 */
                if (NC6ETH_Delete_NeighCache_Entry(device, 
                                                   nc_entry->ip6_neigh_cache_ip_addr) 
                                                   != NU_SUCCESS)
                    NLOG_Error_Log("Failed to delete the Neighbor Cache entry", 
                                   NERR_SEVERE, __FILE__, __LINE__);  
            }            
        }
        else
            NLOG_Error_Log("A matching Neighbor Cache entry does not exist to be resolved", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
    }
    else
        NLOG_Error_Log("Invalid device index", NERR_INFORMATIONAL, 
                       __FILE__, __LINE__);

} /* Resolve6_Event */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       Resolve6_Get_Link_Addr
*                                                                         
*   DESCRIPTION                                                           
*    
*       This function invokes Address Resolution for those Neighbors 
*       whose link-layer address is unknown and returns the link-layer
*       address if it has already been resolved.
*                                                                         
*   INPUTS                                                                
*                        
*       *device                 A pointer to the interface on the same 
*                               link as the neighbor.
*       *dest_addr              A pointer to the IP address of the node 
*                               to resolve.
*       *link_addr              A pointer to the link-layer address of 
*                               the neighbor.
*       *buf_ptr                A pointer to the buffer awaiting address
*                               resolution.
*                              
*   OUTPUTS                                                               
*                                           
*       NU_SUCCESS              The link-layer address has been resolved.
*       NU_NO_MEMORY            There is not enough memory to resolve the 
*                               address.
*       NU_UNRESOLVED_ADDR      The node is undergoing address resolution.
*
*************************************************************************/
STATUS Resolve6_Get_Link_Addr(DV_DEVICE_ENTRY *device, 
                              SCK6_SOCKADDR_IP *dest_addr, UINT8 *link_addr, 
                              NET_BUFFER *buf_ptr)
{
    IP6_ETH_NEIGHBOR_CACHE_ENTRY *nc_eth_entry;
    IP6_NEIGHBOR_CACHE_ENTRY     *nc_entry;
    STATUS                       status;
    NET_BUFFER                   *old_packet;

    /* If the packet is multicast, map the multicast address to the link-layer
     * address and return.
     */
    if (buf_ptr->mem_flags & NET_MCAST)
    {
        NET6_MAP_IP_TO_ETHER_MULTI(dest_addr->sck_addr, link_addr);
        return (NU_SUCCESS);
    }

    /* Search the Neighbor Cache for a corresponding entry. */
    nc_entry = device->dev6_fnd_neighcache_entry(device, dest_addr->sck_addr);

    /* If an entry does not already exist, or an entry does exist but is in 
     * the INCOMPLETE state.
     */
    if ( (!nc_entry) || (nc_entry->ip6_neigh_cache_state == NC_NEIGH_INCOMPLETE) )
    {
        /* If an entry does not already exist, create a new entry.  An entry should
         * already exist, because IP6_Find_Route creates a new entry for the 
         * next hop of a Destination if one does not already exist, but this code
         * is here just in case.
         */
        if (!nc_entry)
        {
            nc_entry = 
                 device->dev6_add_neighcache_entry(device, dest_addr->sck_addr, 
                                                   NU_NULL, 0, buf_ptr, 
                                                   NC_NEIGH_INCOMPLETE);
            if (nc_entry == NU_NULL)
            {
                NLOG_Error_Log("Cannot resolve IPv6 address due to lack of memory", 
                               NERR_SEVERE, __FILE__, __LINE__);

                return (NU_NO_MEMORY);
            }
        }

        /* If an entry already exists, put the new buffer on the queue of
         * packets to be sent upon completion of Address Resolution. 
         */
        else
        {
            /* If the maximum number of packets to be kept in the queue has 
             * been reached, remove the oldest packet from the queue and free
             * the buffer.
             */
            if (nc_entry->ip6_neigh_cache_qpkts_count >= NC_MAX_QUEUED_PACKETS)
            {   
                /* Get the oldest buffer from the queue */
                old_packet = MEM_Buffer_Dequeue(&nc_entry->ip6_neigh_cache_packet_list);
    
                /* Free the buffer */
                MEM_Multiple_Buffer_Chain_Free(old_packet);
            }

            /* Otherwise, increment the number of packets waiting to be sent */
            else                
                nc_entry->ip6_neigh_cache_qpkts_count++;

            /* Put the buffer in the queue */
            if (MEM_Buffer_Enqueue(&nc_entry->ip6_neigh_cache_packet_list, 
                                   buf_ptr) == NU_NULL)
                NLOG_Error_Log("Failed to enqueue the buffer awaiting completion of Address Resolution", 
                               NERR_SEVERE, __FILE__, __LINE__);
        }

        /* If the event has not already been set to resolve the address,
         * assign a resolve ID and set the event.  We must check that nc_entry
         * is not NULL here, because the above add may not have been successful.
         */
        if (nc_entry->ip6_neigh_cache_resolve_id == 0)
        {
            nc_entry->ip6_neigh_cache_resolve_id = Resolve_ID++;

            if (TQ_Timerset(IP6_Resolve6_Event, nc_entry->ip6_neigh_cache_resolve_id, 
                            0, (UNSIGNED)(nc_entry->ip6_neigh_cache_device->dev_index)) 
                            != NU_SUCCESS)
                NLOG_Error_Log("Failed to set the timer to invoke Address Resolution", 
                               NERR_SEVERE, __FILE__, __LINE__);
        }

        status = NU_UNRESOLVED_ADDR;
    }
   
    /* Otherwise, the link-layer address has already been successfully 
     * resolved.  Return the link-layer address. 
     */
    else
    {
        /* RFC 4861 section 7.3.3 - The first time a node sends a packet to
         * a neighbor whose entry is STALE, the sender changes the state to
         * DELAY and sets a timer to expire in DELAY_FIRST_PROBE_TIME seconds.
         */
        if (nc_entry->ip6_neigh_cache_state == NC_NEIGH_STALE)
            NUD6_Stale_Neighbor(nc_entry);

        nc_eth_entry = nc_entry->ip6_neigh_cache_link_spec;

        memcpy(link_addr, nc_eth_entry->ip6_neigh_cache_hw_addr, device->dev_addrlen);
        status = NU_SUCCESS;
    }

    return (status);

} /* Resolve6_Get_Link_Addr */

