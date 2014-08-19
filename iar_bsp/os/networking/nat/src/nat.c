/***************************************************************************
*
*            Copyright Mentor Graphics Corporation 2001-2011
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
****************************************************************************/
/****************************************************************************
*                                                                            
*   FILENAME                                                                          
*                                                                                    
*       nat.c                                                       
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file contains the functions necessary to perform NAT 
*       translation on a packet and send the packet out the
*       appropriate interface.                 
*                                                           
*   DATA STRUCTURES                                                            
*
*       None.                             
*                                               
*   FUNCTIONS                                                                  
*                                             
*       NAT_Initialize            
*       NAT_Translate 
*       NAT_Adjust_Protocol_Checksum   
*       NAT_Transmit_Packet
*       NAT_Find_ICMP_Entry                                         
*       NAT_Find_Translation_Entry                                             
*       NAT_Delete_ICMP_Entry
*       NAT_Delete_Translation_Entry                                             
*       NAT_Assign_Port                                             
*       NAT_Find_Portmap_Entry    
*       NAT_Update_Portmap_Table 
*       NAT_Add_ICMP_Translation_Entry                                        
*       NAT_Add_Translation_Entry                                             
*       NAT_Parse_Packet                                             
*       NAT_ICMP_Parse_Packet
*       NAT_Build_Outgoing_Packet                                             
*       NAT_Build_Incoming_Packet                                             
*       NAT_Update_TCP                                             
*       NAT_Adjust_Checksum    
*       NAT_Cleanup_TCP
*       NAT_Cleanup_UDP
*       NAT_Cleanup_ICMP
*       NAT_Find_Route
*       NAT_Reassemble_Packet
*                                             
*   DEPENDENCIES                                                               
*
*       target.h
*       externs.h
*       nat_defs.h            
*       nat_extr.h
*       alg_extr.h
*                                                                
******************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/nat_defs.h"
#include "networking/nat_extr.h"
#include "networking/alg_extr.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

NU_MEMORY_POOL          *NAT_Memory;
NAT_DEV_LIST            NAT_Internal_Devices;
DV_DEVICE_ENTRY         *NAT_External_Device;
NAT_TRANSLATION_TABLE   NAT_Translation_Table;
NAT_PORTMAP_TABLE       NAT_Portmap_Table;
ALG_FTP_TABLE           ALG_FTP_Table;

extern  INT             IP_NAT_Initialize;

#if INCLUDE_IP_REASSEMBLY
extern  IP_QUEUE        IP_Frag_Queue;
#endif

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Initialize
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function initializes the Translation Table and Portmap
*       Table.  It also assigns the internal device, the external IP
*       address used to hide the internal network, and starts the two
*       timers to cleanup the TCP and UDP entries in the Translation
*       Table.                                                     
*                                                                       
*   INPUTS                                                                
*          
*       *nat_internal_devices   A pointer to the data structure 
*                               containing a list of the names of all
*                               of the internal devices.
*       dev_count               The total number of all internal devices.
*       *nat_external_dev_name  A pointer to the name of the device to be 
*                               used to communicate with the external 
*                               network.
*                                                                       
*   OUTPUTS                                                               
*                 
*       NU_SUCCESS              Initialization successful.    
*       NU_NO_MEMORY            There is not enough memory.
*       NU_TIMEOUT              A timeout occurred on the service.
*       NU_POOL_DELETED         The memory pool was deleted during 
*                               suspension.
*
****************************************************************************/
STATUS NAT_Initialize(const NAT_DEVICE *nat_internal_devices, INT dev_count,
                      const CHAR *nat_external_dev_name)
{
    INT             i;
    STATUS          status;
    NAT_DEV_ENTRY   *current_device;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#ifdef NET_5_1
    NAT_Memory = MEM_Cached;
#else
    NAT_Memory = &System_Memory;
#endif

    /* Allocate memory for the TCP, UDP and ICMP tables and the PORT list */
#if (INCLUDE_TCP == NU_TRUE)
    if ((status = NU_Allocate_Memory(NAT_Memory, 
                                    (VOID**)&NAT_Translation_Table.nat_tcp_table,
                                    sizeof(NAT_TCP_TABLE), 
                                    NU_SUSPEND)) != NU_SUCCESS)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();

        return (status);
    }
#endif

#if (INCLUDE_UDP == NU_TRUE)
    if ((status = NU_Allocate_Memory(NAT_Memory, 
                                    (VOID**)&NAT_Translation_Table.nat_udp_table,
                                    sizeof(NAT_UDP_TABLE), 
                                    NU_SUSPEND)) != NU_SUCCESS)
    {
        /* Deallocate any memory allocated for other NAT data structures */
#if (INCLUDE_TCP == NU_TRUE)
        NU_Deallocate_Memory((VOID*)NAT_Translation_Table.nat_tcp_table);
#endif

        /* Switch back to user mode. */
        NU_USER_MODE();

        return (status);
    }
#endif

    if ((status = NU_Allocate_Memory(NAT_Memory, 
                                    (VOID**)&NAT_Translation_Table.nat_icmp_table,
                                    sizeof(NAT_ICMP_TABLE), 
                                    NU_SUSPEND)) != NU_SUCCESS)
    {
        /* Deallocate any memory allocated for other NAT data structures */
#if (INCLUDE_TCP == NU_TRUE)
        NU_Deallocate_Memory((VOID*)NAT_Translation_Table.nat_tcp_table);
#endif
#if (INCLUDE_UDP == NU_TRUE)
        NU_Deallocate_Memory((VOID*)NAT_Translation_Table.nat_udp_table);
#endif

        /* Switch back to user mode. */
        NU_USER_MODE();

        return (status);
    }

    if ((status = NU_Allocate_Memory(NAT_Memory, 
                                    (VOID**)&NAT_Translation_Table.nat_port_list,
                                    sizeof(NAT_PORT_LIST), 
                                    NU_SUSPEND)) != NU_SUCCESS)
    {
        /* Deallocate any memory allocated for other NAT data structures */
#if (INCLUDE_TCP == NU_TRUE)
        NU_Deallocate_Memory((VOID*)NAT_Translation_Table.nat_tcp_table);
#endif
#if (INCLUDE_UDP == NU_TRUE)
        NU_Deallocate_Memory((VOID*)NAT_Translation_Table.nat_udp_table);
#endif
        NU_Deallocate_Memory((VOID*)NAT_Translation_Table.nat_icmp_table);

        /* Switch back to user mode. */
        NU_USER_MODE();

        return (status);
    }

    for (i = 0; i < dev_count; i++)
    {
        if ((status = NU_Allocate_Memory(NAT_Memory, (VOID**)&current_device, 
                                         sizeof(NAT_DEV_ENTRY), 
                                         NU_NO_SUSPEND)) != NU_SUCCESS)
        {
            /* Deallocate any memory allocated for other NAT data 
             * structures. 
             */
#if (INCLUDE_TCP == NU_TRUE)
            NU_Deallocate_Memory((VOID*)NAT_Translation_Table.nat_tcp_table);
#endif
#if (INCLUDE_UDP == NU_TRUE)
            NU_Deallocate_Memory((VOID*)NAT_Translation_Table.nat_udp_table);
#endif
            NU_Deallocate_Memory((VOID*)NAT_Translation_Table.nat_icmp_table);
            NU_Deallocate_Memory((VOID*)NAT_Translation_Table.nat_port_list);

            while (i > 0)
            {
                DLL_Dequeue(&NAT_Internal_Devices);
                i--;
            }

            /* Switch back to user mode. */
            NU_USER_MODE();

            return (status);
        }

        UTL_Zero(current_device, sizeof(NAT_DEV_ENTRY));

        /* Designate the internal network device */
        DLL_Enqueue(&NAT_Internal_Devices, current_device);

        current_device->nat_device = 
            DEV_Get_Dev_By_Name(nat_internal_devices[i].nat_dev_name);
    }

    NAT_External_Device = DEV_Get_Dev_By_Name(nat_external_dev_name);

#if (INCLUDE_TCP == NU_TRUE)
    NAT_Translation_Table.nat_port_list->nat_next_avail_tcp_port_index = 0;
#endif
#if (INCLUDE_UDP == NU_TRUE)
    NAT_Translation_Table.nat_port_list->nat_next_avail_udp_port_index = 0;
#endif

    /* Initialize the TCP members of the Address Translation Table */
#if (INCLUDE_TCP == NU_TRUE)
    for (i = 0; i < NAT_MAX_TCP_CONNS; i++)
    {
        UTL_Zero(&NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[i], 
                 sizeof(NAT_TCP_ENTRY));

        NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[i].nat_state = -1;

#if (NAT_INCLUDE_FTP_ALG == NU_TRUE)
        NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[i].
            nat_ftp.nat_ftp_index = -1;
#endif
    }

    NAT_Translation_Table.nat_tcp_table->nat_next_available_tcp_index = 0;
#endif

    /* Initialize the UDP members of the Address Translation Table */
#if (INCLUDE_UDP == NU_TRUE)
    for (i = 0; i < NAT_MAX_UDP_CONNS; i++)
    {
        UTL_Zero(&NAT_Translation_Table.nat_udp_table->nat_udp_entry[i], 
                 sizeof(NAT_UDP_ENTRY));
    }

    NAT_Translation_Table.nat_udp_table->nat_next_available_udp_index = 0;
#endif

    /* Initialize the ICMP members of the Address Translation Table */
    for (i = 0; i < NAT_MAX_ICMP_CONNS; i++)
    {
        UTL_Zero(&NAT_Translation_Table.nat_icmp_table->nat_icmp_entry[i], 
                 sizeof(NAT_ICMP_ENTRY));
    }

    NAT_Translation_Table.nat_icmp_table->nat_next_available_icmp_index = 0;

    /* Initialize the list of NAT ports */
    for (i = 0; i < NAT_MAX_CONNS; i++)
    {
        NAT_Translation_Table.nat_port_list->nat_port_list[i].nat_port = 
            (UINT16)(NAT_MIN_PORT + i);

#if (INCLUDE_TCP == NU_TRUE)
        NAT_Translation_Table.nat_port_list->nat_port_list[i].
            nat_tcp_used_flag = -1;
#endif

#if (INCLUDE_UDP == NU_TRUE)
        NAT_Translation_Table.nat_port_list->nat_port_list[i].
            nat_udp_used_flag = -1;
#endif

    }    

    NAT_Portmap_Table.nat_head = NU_NULL;

    /* Set the timer task to cleanup the TCP and UDP entries in the Address
     * Translation Table.
     */
#if (INCLUDE_TCP == NU_TRUE)
    UTL_Timerset(NAT_CLEANUP_TCP, 0, NAT_TCP_TIMEOUT, 0);
#endif
#if (INCLUDE_UDP == NU_TRUE)
    UTL_Timerset(NAT_CLEANUP_UDP, 0, NAT_UDP_TIMEOUT, 0);
#endif

    /* Set the timer task to cleanup the ICMP entries in the ICMP Table */
    UTL_Timerset(NAT_CLEANUP_ICMP, 0, NAT_ICMP_TIMEOUT, 0);

    /* Call the initialization function for the ALGs */
#if (NAT_INCLUDE_FTP_ALG == NU_TRUE)
    ALG_Init();
#endif

    IP_NAT_Initialize = 1;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NAT_Initialize */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Translate
*                                                                       
*   DESCRIPTION                                                           
*         
*       This function performs the translation and transmission of
*       a packet.                                                              
*                                                                       
*   INPUTS                                                                
*                
*       *device                 The device on which the packet was received.
*       **nat_pkt               Address of the pointer to the packet.
*       **nat_buf_ptr           Address of the pointer to the buffer of data.
*                                                                       
*   OUTPUTS                                                               
*                   
*       NU_SUCCESS              The packet was successfully translated and
*                               transmitted.                                
*       NU_INVALID_PARM         One of the passed in parameters is invalid.
*       NAT_NO_NAT              This packet does not need a NAT translation.
*       NAT_NO_MEMORY           There is not enough memory to perform the
*                               translation.
*       NAT_NO_ENTRY            The packet was intended for the internal
*                               network, and there is no entry in the 
*                               Address Translation Table or the Portmap
*                               Table.
*       NAT_NO_ROUTE            There is no route to the destination.
*                                                                       
****************************************************************************/
STATUS NAT_Translate(DV_DEVICE_ENTRY *device, IPLAYER **nat_pkt, 
                     NET_BUFFER **nat_buf_ptr)
{
    UINT8               no_nat_flag = 0;
    IPLAYER             *pkt = *nat_pkt;
    NET_BUFFER          *buf_ptr = *nat_buf_ptr;
    UINT32              ip_addr = 0, pkt_ip_dest;
    UINT8               old_data[NET_MAX_BUFFER_SIZE];
    UINT16              original_checksum;
    INT32               index;
    INT32               port = -1;
    STATUS              status;
    NAT_PACKET          nat_packet;
    NAT_ICMP_ENTRY      *icmp_entry = NU_NULL;
    NAT_PORTMAP_ENTRY   *portmap_entry = NU_NULL;
    NAT_DEV_ENTRY       *current_device;
    DV_DEVICE_ENTRY     *internal_device = NU_NULL;    

#if (INCLUDE_TCP == NU_TRUE)
    NAT_TCP_ENTRY       *tcp_entry = NU_NULL;
#endif
#if (INCLUDE_UDP == NU_TRUE)
    NAT_UDP_ENTRY       *udp_entry = NU_NULL;
#endif

    /* Do not forward broadcast or multicast packets */
    if (buf_ptr->mem_flags & (NET_BCAST | NET_MCAST))
        return (NAT_NO_NAT);

    /* Check the TTL time to live field. */
    if (GET8(pkt, IP_TTL_OFFSET) <= 1)
    {
        ICMP_Send_Error(buf_ptr, ICMP_TIMXCEED, ICMP_TIMXCEED_TTL, 0, device);
        
        return (NAT_ICMP_TIMXCEED);
    }    

    pkt_ip_dest = GET32(pkt, IP_DEST_OFFSET);

    nat_packet.nat_device_type = 0;

    current_device = NAT_Internal_Devices.nat_head;

    /* Traverse through the list of internal devices.  If we find one
     * matching the device we received the packet on, the packet is from 
     * the internal network, so mark the device type as internal.
     */
    while (current_device != NU_NULL)
    {
        /* If an internal packet is destined to an internal subnet, do
         * not perform translation on the packet, or if an external packet 
         * is addressed to an internal subnet, discard it.  To determine 
         * this, AND the IP address of the current device with the subnet 
         * mask of the current device, and AND the destination address of 
         * the packet with the subnet mask of the current device.  If the 
         * two are equal, the packet is intended for an internal subnet.
         */
        if ( (current_device->nat_device->dev_flags & DV_UP) &&
            ((current_device->nat_device->dev_addr.dev_ip_addr & 
              current_device->nat_device->dev_addr.dev_netmask) ==
             (pkt_ip_dest & current_device->nat_device->dev_addr.dev_netmask)) )
            no_nat_flag = 1;

        /* Determine from which device the packet was received */
        if ( (nat_packet.nat_device_type == 0) &&
             (memcmp(device->dev_net_if_name, 
                     current_device->nat_device->dev_net_if_name, 
                     DEV_NAME_LENGTH) == 0) )
            nat_packet.nat_device_type = NAT_INTERNAL_DEVICE;

        current_device = current_device->nat_dev_next;
    }

    /* If we did not find a device in the internal devices list, the
     * packet is from the external network.
     */
    if (nat_packet.nat_device_type == 0)
        nat_packet.nat_device_type = NAT_EXTERNAL_DEVICE;

    /* If the flag was set to not perform NAT on the packet because the
     * packet is destined to an internal subnet, return.
     */
    if (no_nat_flag)
    {
        /* If the packet was sent from the external network, 
         * discard the packet. 
         */
        if (nat_packet.nat_device_type == NAT_EXTERNAL_DEVICE)
            return (NAT_NO_ENTRY);

        /* If the packet was sent from the internal network, pass the packet
         * back to the IP layer.
         */
        else
            return (NAT_NO_NAT);
    }

    /* If the packet is from the external network and not intended for 
     * the external source address, or the packet is from the internal network 
     * and not intended for the external network, return an error.
     */
    if ( ((nat_packet.nat_device_type == NAT_EXTERNAL_DEVICE) &&
          (pkt_ip_dest != NAT_External_Device->dev_addr.dev_ip_addr)) ||
         ((nat_packet.nat_device_type == NAT_INTERNAL_DEVICE) &&
         ((pkt_ip_dest == device->dev_addr.dev_ip_addr) || 
          (pkt_ip_dest == NAT_External_Device->dev_addr.dev_ip_addr))) )
        return (NAT_NO_NAT);   

    /* If offset or IP_MF are set then this is a fragment. */
    if (GET16(pkt, IP_FRAGS_OFFSET) & ~IP_DF)
    {
        /* If the processing of fragments is desired */
#if INCLUDE_IP_REASSEMBLY
        
        status = NAT_Reassemble_Packet(&pkt, &buf_ptr);          

        if (status == NU_SUCCESS)
        {
            *nat_pkt = pkt;
            *nat_buf_ptr = buf_ptr;
        }

        else if (status == 1)
            return (NU_SUCCESS);

        /* If the processing of fragments is not desired, then drop this 
         * packet. 
         */
#else 
        /* Increment the number of IP fragments that have been received,
         * even though we don't process them. 
         */
        SNMP_ipReasmReqds_Inc;

        /* Drop the current buffer. */
        MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

        SNMP_ipInDiscards_Inc;

        return (NU_SUCCESS);
#endif
    }

    /* Parse the packet */
    status = NAT_Parse_Packet(pkt, buf_ptr, &nat_packet);

    if (status != NU_SUCCESS)
        return (status);

    /* Process the ICMP packet */
    if (nat_packet.nat_protocol == IPPROTO_ICMP)
    {
        /* Find an existing entry in the Translation Table to determine if
         * this ICMP packet is an ICMP error packet sent from a node on the 
         * external network to a node on the internal network in response 
         * to a packet sent from the internal network.
         */
        index = NAT_Find_Translation_Entry(nat_packet.nat_icmp_packet.nat_icmp_proto, 
                                           nat_packet.nat_device_type,
                                           nat_packet.nat_icmp_packet.nat_icmp_dest_addr,
                                           nat_packet.nat_icmp_packet.nat_icmp_dest_port,
                                           nat_packet.nat_icmp_packet.nat_icmp_source_addr,                                                   
                                           nat_packet.nat_icmp_packet.nat_icmp_source_port,
                                           NU_NULL);

        /* This is not an ICMP error packet from the external network */
        if (index < 0)
        {
            /* Check if this is the response to an ICMP packet sent from the
             * internal network.
             */
            if (nat_packet.nat_device_type == NAT_EXTERNAL_DEVICE)
            {
                index = NAT_Find_ICMP_Entry(nat_packet.nat_source_addr, 
                                            nat_packet.nat_icmp_packet.nat_icmp_type,
                                            nat_packet.nat_icmp_packet.nat_icmp_sequence_number);
                if (index != -1)
                {
                    icmp_entry = 
                        &NAT_Translation_Table.nat_icmp_table->nat_icmp_entry[index];
                }
            }

            /* Otherwise, this is an outgoing ICMP error packet.  Check if
             * a new entry needs to be made.
             */
            else
            {
                index = NAT_Add_ICMP_Translation_Entry(nat_packet.nat_source_addr,
                                                       nat_packet.nat_dest_addr,
                                                       nat_packet.nat_icmp_packet.nat_icmp_type,
                                                       nat_packet.nat_icmp_packet.
                                                       nat_icmp_sequence_number,
                                                       device); 
                if (index == -1)
                {
                    /* Send ICMP host unreachable message. */
                    ICMP_Send_Error(buf_ptr, ICMP_SOURCEQUENCH, 0, 0, device);

                    return (NAT_NO_MEMORY);
                }
            }
        }
    }

    /* Otherwise, this packet is of a protocol type which contains
     * port numbers in the protocol header.
     */
    else
    {
        if (nat_packet.nat_device_type == NAT_EXTERNAL_DEVICE)
        {
            /* Find a matching portmap entry */
            portmap_entry = NAT_Find_Portmap_Entry(nat_packet.nat_protocol,
                                                   nat_packet.nat_device_type,
                                                   nat_packet.nat_source_addr,
                                                   nat_packet.nat_source_port, 
                                                   nat_packet.nat_dest_addr,
                                                   nat_packet.nat_dest_port);
        }

        /* Find a matching entry in the Address Translation Table */
        index = NAT_Find_Translation_Entry(nat_packet.nat_protocol,
                                           nat_packet.nat_device_type,
                                           nat_packet.nat_source_addr, 
                                           nat_packet.nat_source_port, 
                                           nat_packet.nat_dest_addr, 
                                           nat_packet.nat_dest_port, 
                                           portmap_entry);

        /* No Translation Table entry was found - either add an entry or 
         * return an error. 
         */
        if (index == -1)
        {
            /* If a portmap entry exists, add a matching Translation entry */
            if (nat_packet.nat_device_type == NAT_EXTERNAL_DEVICE)
            {
                if (portmap_entry != NU_NULL)
                {
                    /* Update the Portmap Table to maintain the Round Robin
                     * behavior of packets being sent to internal servers.
                     */
                    NAT_Update_Portmap_Table(portmap_entry);

                    /* Add the entry */
                    index = NAT_Add_Translation_Entry(nat_packet.nat_protocol, 
                                                      portmap_entry->nat_internal_source_ip, 
                                                      portmap_entry->nat_internal_source_port, 
                                                      nat_packet.nat_source_addr, 
                                                      nat_packet.nat_source_port,
                                                      portmap_entry->nat_internal_device, 
                                                      portmap_entry);
                    if (index == -1)
                    {
                        /* Send ICMP host unreachable message. */
                        ICMP_Send_Error(buf_ptr, ICMP_UNREACH, ICMP_UNREACH_PORT, 0,
                                        device);

                        return (NAT_NO_MEMORY);
                    }
                }

                /* Otherwise, the external node is attempting to communicate with
                 * the NAT node.
                 */
                else
                    return (NAT_NO_NAT);
            }

            /* This is the first packet initiating a connection from the internal
             * network.  Add an entry to the Translation Table.
             */
            else
            {
                index = NAT_Add_Translation_Entry(nat_packet.nat_protocol, 
                                                    nat_packet.nat_source_addr, 
                                                    nat_packet.nat_source_port, 
                                                    nat_packet.nat_dest_addr, 
                                                    nat_packet.nat_dest_port,
                                                    device, NU_NULL);
                if (index == -1)
                {
                    /* Send ICMP host unreachable message. */
                    ICMP_Send_Error(buf_ptr, ICMP_SOURCEQUENCH, 0, 0, device);

                    return (NAT_NO_MEMORY);
                }
            }
        }
    }

    /* Get a pointer to the appropriate entry based on the index */
    if (index != -1)
    {
        switch (nat_packet.nat_protocol)
        {
#if (INCLUDE_TCP == NU_TRUE)
        case IPPROTO_TCP:

            tcp_entry = 
                &NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[index];

            internal_device = tcp_entry->nat_internal_device;

            break;
#endif

#if (INCLUDE_UDP == NU_TRUE)
        case IPPROTO_UDP:

            udp_entry = 
                &NAT_Translation_Table.nat_udp_table->nat_udp_entry[index];

            internal_device = udp_entry->nat_internal_device;

            break;
#endif

        case IPPROTO_ICMP:

            switch (nat_packet.nat_icmp_packet.nat_icmp_proto)
            {
#if (INCLUDE_TCP == NU_TRUE)
            case IPPROTO_TCP:

                tcp_entry = 
                    &NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[index];

                internal_device = tcp_entry->nat_internal_device;

                break;
#endif

#if (INCLUDE_UDP == NU_TRUE)
            case IPPROTO_UDP:

                udp_entry = 
                    &NAT_Translation_Table.nat_udp_table->nat_udp_entry[index];

                internal_device = udp_entry->nat_internal_device;

                break;
#endif

            default:

                break;
            }
            break;

        default:

            break;
        }
    }

    /* Otherwise, a matching entry was not found */
    else if (portmap_entry == NU_NULL)
        return (NAT_NO_NAT);

    UTL_Zero(old_data, NET_MAX_BUFFER_SIZE);

    /* Copy the original data */
    memcpy(old_data, buf_ptr->data_ptr, (INT)buf_ptr->data_len);

    /* Call the ALG */
    status = ALG_Modify_Payload(buf_ptr, &nat_packet, index, old_data, 
                                internal_device);

    if (status == NAT_NO_MEMORY)
        return (status);

    /* Get the external port number */
    if (nat_packet.nat_device_type == NAT_INTERNAL_DEVICE)
    {
        switch (nat_packet.nat_protocol)
        {
#if (INCLUDE_TCP == NU_TRUE)
        case IPPROTO_TCP:

            /* If a portmap entry exists, use the port in the portmap entry
             * associated with the TCP entry.
             */
            if (tcp_entry->nat_portmap_entry == NU_NULL)
                port = NAT_Translation_Table.nat_port_list->
                       nat_port_list[tcp_entry->nat_external_port_index].nat_port;
            else
                port = tcp_entry->nat_portmap_entry->nat_external_source_port;    

            break;
#endif

#if (INCLUDE_UDP == NU_TRUE)
        case IPPROTO_UDP:

            /* If a portmap entry exists, use the port in the port entry
             * associated with the UDP entry.
             */
            if (udp_entry->nat_portmap_entry == NU_NULL)
                port = NAT_Translation_Table.nat_port_list->
                       nat_port_list[udp_entry->nat_external_port_index].nat_port;
            else
                port = udp_entry->nat_portmap_entry->nat_external_source_port;    

            break;
#endif

        default:

            break;
        }

        /* Build the outgoing packet */
        NAT_Build_Outgoing_Packet(buf_ptr, &nat_packet, port);
    }

    /* Get the internal IP address and port number */
    else
    {
#if (INCLUDE_TCP == NU_TRUE)
        if (tcp_entry != NU_NULL)
        {
            ip_addr = tcp_entry->nat_internal_source_ip;
    
            /* If this is an ICMP error packet in response to a TCP packet,
             * do not change the port number in the packet.
             */
            if (nat_packet.nat_protocol != IPPROTO_ICMP)
                port = tcp_entry->nat_internal_source_port;
        }
#endif

#if ((INCLUDE_UDP == NU_TRUE) && (INCLUDE_TCP == NU_TRUE))
        else 
#endif

#if (INCLUDE_UDP == NU_TRUE)
        if (udp_entry != NU_NULL)
        {
            ip_addr = udp_entry->nat_internal_source_ip;
    
            /* If this is an ICMP error packet in response to a UDP packet,
             * do not change the port number in the packet.
             */
            if (nat_packet.nat_protocol != IPPROTO_ICMP)
                port = udp_entry->nat_internal_source_port;
        }
#endif

#if ((INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE))
        else 
#endif

        /* If this is an ICMP packet aside from an error packet. */
        if (icmp_entry != NU_NULL)
        {
            ip_addr = icmp_entry->nat_internal_source_ip;
            internal_device = icmp_entry->nat_internal_device;

            NAT_Delete_ICMP_Entry(index);
        }
           
        /* Build the incoming packet */
        NAT_Build_Incoming_Packet(buf_ptr, &nat_packet, ip_addr, port);    
    }

    /* If the packet is an FTP PORT command from the internal network, 
     * recompute the TCP checksum over the entire packet.  This must be done,
     * because the length of the packet has changed.
     */
#if (NAT_INCLUDE_FTP_ALG == NU_TRUE)
    if (status == NAT_COMPUTE_CHECKSUM)
    {
        /* Zero out the existing checksum */
        PUT16(buf_ptr->data_ptr, 
              (unsigned int)(nat_packet.nat_ip_header_length + TCP_CHECK_OFFSET), 0);

        /* Decrement the length of the packet by the size of the IP header, and
         * move the data pointer to the start of the TCP header.
         */
        buf_ptr->mem_total_data_len -= nat_packet.nat_ip_header_length;
        buf_ptr->data_len -= nat_packet.nat_ip_header_length;
        buf_ptr->data_ptr += nat_packet.nat_ip_header_length;

        /* Recompute the checksum */
        original_checksum = UTL_Checksum(buf_ptr, 
                                         NAT_External_Device->dev_addr.dev_ip_addr, 
                                         nat_packet.nat_dest_addr, 
                                         nat_packet.nat_protocol);

        /* Restore the original data lengths and position of the data pointer */
        buf_ptr->mem_total_data_len += nat_packet.nat_ip_header_length;
        buf_ptr->data_len += nat_packet.nat_ip_header_length;
        buf_ptr->data_ptr -= nat_packet.nat_ip_header_length;

        /* Put the new checksum in the packet */
        PUT16(buf_ptr->data_ptr, 
              (unsigned int)(nat_packet.nat_ip_header_length + TCP_CHECK_OFFSET), 
              original_checksum);
    }

    else

#endif

    /* Otherwise, adjust the checksum */
    NAT_Adjust_Protocol_Checksum(buf_ptr, &nat_packet, old_data);
     
    /* Adjust the IP checksum */
    if (nat_packet.nat_device_type == NAT_INTERNAL_DEVICE)
    {
        NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[IP_CHECK_OFFSET], 
                            (UINT8*)&old_data[IP_SRC_OFFSET], 4,
                            &buf_ptr->data_ptr[IP_SRC_OFFSET], 4);       
    }
    else
    {     
        NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[IP_CHECK_OFFSET], 
                            (UINT8*)&old_data[IP_DEST_OFFSET], 4,
                            &buf_ptr->data_ptr[IP_DEST_OFFSET], 4);           
    }    

    /* Decrement the time to live. */
    PUT8(buf_ptr->data_ptr, IP_TTL_OFFSET, 
         (UINT8)(GET8(buf_ptr->data_ptr, IP_TTL_OFFSET) - 1));

    NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[IP_CHECK_OFFSET], 
                        (UINT8*)&old_data[IP_TTL_OFFSET], 2,
                        &buf_ptr->data_ptr[IP_TTL_OFFSET], 2);

    /* Transmit the packet. */
    status = NAT_Transmit_Packet(buf_ptr, &nat_packet, ip_addr, 
	                             internal_device);

    if (status == NU_SUCCESS)
    {
        /* Update the state of the TCP Address Translation entry */
#if (INCLUDE_TCP == NU_TRUE)
        if (tcp_entry != NU_NULL)
            NAT_Update_TCP(&nat_packet, tcp_entry, index);
#endif
    }

    /* If IP_Fragment or dev_output failed, or if there are not enough buffers
     * to transmit the packet, return NU_SUCCESS to the IP layer since 
     * NAT_Transmit_Packet has already taken care of freeing the buffers.
     */
    else if (status != NAT_NO_NAT)
        status = NU_SUCCESS;

    return (status);

} /* NAT_Translate */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Adjust_Protocol_Checksum
*                                                                       
*   DESCRIPTION                                                           
*                   
*       This function adjusts the protocol-specific checksum over the
*       modified port number and pseudoheader in a TCP or UDP header.
*                                                                       
*   INPUTS                                                                
*                               
*       *buf_ptr                A pointer to the NET_BUFFER data structure.
*       *nat_packet             A pointer to the NAT_PACKET data structure 
*                               which contains the parsed parameters from 
*                               the packet.
*       *old_data               The original, unaltered packet.
*                                                                       
*   OUTPUTS                                                               
*                                                   
*       None.
*                                                                       
****************************************************************************/
VOID NAT_Adjust_Protocol_Checksum(const NET_BUFFER *buf_ptr, 
                                  const NAT_PACKET *nat_packet, 
                                  UINT8 *old_data)
{
    switch (nat_packet->nat_protocol)
    {

#if (INCLUDE_TCP == NU_TRUE)

    case IPPROTO_TCP:

        if (nat_packet->nat_device_type == NAT_INTERNAL_DEVICE)
        {
            /* Adjust the TCP checksum over the source port in the TCP header. */
            NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                nat_ip_header_length + TCP_CHECK_OFFSET], 
                                &old_data[nat_packet->nat_ip_header_length], 2,
                                &buf_ptr->data_ptr[nat_packet->nat_ip_header_length], 2);
    
            /* Adjust the TCP checksum over the pseudoheader which contains the source
             * IP address.
             */
            NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                nat_ip_header_length + TCP_CHECK_OFFSET], 
                                (UINT8*)&old_data[IP_SRC_OFFSET], 4,
                                &buf_ptr->data_ptr[IP_SRC_OFFSET], 4);
        }

        else
        {
            /* Adjust the TCP checksum over the destination port in the TCP 
             * header. 
             */
            NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                nat_ip_header_length + TCP_CHECK_OFFSET], 
                                &old_data[nat_packet->nat_ip_header_length + 
                                TCP_DEST_OFFSET], 2,
                                &buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                TCP_DEST_OFFSET], 2);
    
            /* Adjust the TCP checksum over the pseudoheader which contains 
             * the destination IP address.
             */
            NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                nat_ip_header_length + TCP_CHECK_OFFSET], 
                                (UINT8*)&old_data[IP_DEST_OFFSET], 4,
                                &buf_ptr->data_ptr[IP_DEST_OFFSET], 4);
        }

        break;
#endif

#if (INCLUDE_UDP == NU_TRUE)
    case IPPROTO_UDP:

        /* If the UDP checksum is zero, no adjustment needs to be made */
        if (nat_packet->nat_prot_checksum != 0)
        {
            if (nat_packet->nat_device_type == NAT_INTERNAL_DEVICE)
            {
                /* Adjust the UDP checksum over the source port in the UDP 
                 * header. 
                 */
                NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                    nat_ip_header_length + UDP_CHECK_OFFSET], 
                                    &old_data[nat_packet->nat_ip_header_length], 2,
                                    &buf_ptr->data_ptr[nat_packet->nat_ip_header_length], 
                                    2);
    
                /* Adjust the UDP checksum over the pseudoheader which 
                 * contains the source IP address if the checksum is not zero.
                 */
                NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                    nat_ip_header_length + UDP_CHECK_OFFSET], 
                                    (UINT8*)&old_data[IP_SRC_OFFSET], 4,
                                    &buf_ptr->data_ptr[IP_SRC_OFFSET], 4);
            }
    
            else
            {
                /* Adjust the checksum over the UDP destination port. */
                NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                    nat_ip_header_length + UDP_CHECK_OFFSET], 
                                    &old_data[nat_packet->nat_ip_header_length + 
                                    UDP_DEST_OFFSET], 2,
                                    &buf_ptr->data_ptr[nat_packet->
                                    nat_ip_header_length + UDP_DEST_OFFSET], 2);
    
                /* Adjust the UDP checksum over the pseudoheader which 
                 * contains the destination IP address.
                 */
                NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                    nat_ip_header_length + UDP_CHECK_OFFSET], 
                                    (UINT8*)&old_data[IP_DEST_OFFSET], 4,
                                    &buf_ptr->data_ptr[IP_DEST_OFFSET], 4);
            }

            if (GET16(buf_ptr->data_ptr, 
                     (unsigned int)(nat_packet->nat_ip_header_length + 
                     UDP_CHECK_OFFSET)) == 0)
            {
                PUT16(buf_ptr->data_ptr, 
                     (unsigned int)(nat_packet->nat_ip_header_length + 
                     UDP_CHECK_OFFSET), 0xFFFF);
            }
        }

        break;
#endif
    
    default:

        break;
    }

} /* NAT_Adjust_Protocol_Checksum */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Transmit_Packet
*                                                                       
*   DESCRIPTION                                                           
*                   
*       This function sends the packet out the proper interface.
*                                                                       
*   INPUTS                                                                
*                               
*       *buf_ptr                A pointer to the buffer of data.
*       *nat_packet             A pointer to the parsed data from the 
*                               original packet.
*       ip_addr                 The IP address to which to send the data.
*       *internal_device        A pointer to the device on which the 
*                               packet will be sent
*                                                                       
*   OUTPUTS                                                               
*                                                   
*       NU_SUCCESS              The packet was successfully transmitted.
*       NAT_NO_NAT              There is no route to the destination.
*       NU_MSGSIZE              IP fragmentation could not be performed.
*       NU_NO_BUFFERS           There are not enough buffers in the
*                               system to transmit the packet.
*       NU_NO_DATA_TRANSFER     The call to dev_output failed.
*                                                                       
****************************************************************************/
STATUS NAT_Transmit_Packet(NET_BUFFER *buf_ptr, const NAT_PACKET *nat_packet, 
                           UINT32 ip_addr, DV_DEVICE_ENTRY *internal_device)
{
    SCK_SOCKADDR_IP     *de;
    RTAB_ROUTE          ro;
    STATUS              status = NU_SUCCESS;
    IPLAYER             *packet;
    NET_BUFFER          *hdr_buf = NU_NULL;
    UINT32              hlen;

    memset(&ro, 0, sizeof(RTAB_ROUTE));

    de = &ro.rt_ip_dest;
    de->sck_len = sizeof(SCK_SOCKADDR_IP);
    de->sck_family = SK_FAM_IP;

    /* Set the destination address for which to find a route based on what
     * side of the network received this packet.
     */
    if (nat_packet->nat_device_type == NAT_INTERNAL_DEVICE)
        de->sck_addr = nat_packet->nat_dest_addr;
    else
        de->sck_addr = ip_addr;

    /* Find a route and decrement the reference count */
    NAT_Find_Route(&ro);

    if (ro.rt_route == NU_NULL)
        return (NAT_NO_NAT);

    ro.rt_route->rt_refcnt --;

    /* If the next hop is a gateway then set the destination IP address to
     * the gateway. 
     */
    if (ro.rt_route->rt_flags & RT_GATEWAY)
        de = &ro.rt_route->rt_gateway;

    packet = (IPLAYER *)buf_ptr->data_ptr;

    hlen = 
        (INT16)((GET8(buf_ptr->data_ptr, IP_VERSIONANDHDRLEN_OFFSET) & 0xf) << 2);

    /* If the packet came from the internal network, find a route to the 
     * destination. 
     */
    if (nat_packet->nat_device_type == NAT_INTERNAL_DEVICE)
    {          
        /* Pull the current buffer chain off of the receive list. */ 
        buf_ptr = MEM_Buffer_Dequeue(&MEM_Buffer_List);

		if (buf_ptr)
		{
	        /* Ensure that hlen is on a word-boundary since we will be 
	         * advancing the buffer pointer by this amount.
	         */
	        hlen += ((UINT32)buf_ptr->data_ptr % 4);
	
	        /* Set the deallocation list pointer. */
	        buf_ptr->mem_dlist = &MEM_Buffer_Freelist;
	        
#if ((defined(NET_5_1)) && (NET_VERSION_COMP >= NET_5_1))
	        if (ro.rt_route->rt_device->dev_hdrlen > buf_ptr->mem_buf_device->dev_hdrlen)
	        {
	            buf_ptr->data_len           -= hlen;
	            buf_ptr->mem_total_data_len -= hlen;   
	            
	            /* Allocate a new buffer chain for the link-layer and IP header */
	            hdr_buf = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
	                (INT32)(hlen + ro.rt_route->rt_device->dev_hdrlen));
	            
	            /* If a new chain of buffers was allocated, link the buffer in as
	             * the first buffer in the list.
	             */
	            if (hdr_buf)
	            {
	                /* Link the new buffer in to the list */
	                hdr_buf->next_buffer = buf_ptr;
	                
	                /* Set the list to which the header buffers will be freed */
	                hdr_buf->mem_dlist = &MEM_Buffer_Freelist;
	                
	                /* If the buffer chain passed in is flagged as a parent chain,
	                 * flag the new chain as a parent too.
	                 */
	                if (buf_ptr->mem_flags & NET_PARENT)
	                    hdr_buf->mem_flags |= NET_PARENT;
	                
	                /* Set the data pointer to the end of the physical layer header*/
	                hdr_buf->data_ptr = 
	                    hdr_buf->mem_parent_packet + ro.rt_route->rt_device->dev_hdrlen;
	                
	                /* Set the total data length of the chain of buffers */
	                hdr_buf->mem_total_data_len = buf_ptr->mem_total_data_len;                    
	                
	                /* If NAT appended a new buffer for the IP and link-layer headers 
	                 * to the buffer chain, copy the old header into the new buffer.
	                 */
	                memcpy(hdr_buf->data_ptr, buf_ptr->data_ptr, (unsigned int)hlen);
	            
	                /* Increment the buffer pointer to point to the data past the
	                 * IP header since the buffer prepended will now contain the
	                 * header.
	                 */
	                buf_ptr->data_ptr += hlen;
	
	                /* Update the length of the header buffer. */
	                hdr_buf->data_len           += hlen;
	                hdr_buf->mem_total_data_len += hlen;
	            }
	
	            /* If a buffer could not be allocated, return an error. */
	            else
	            {
	                hdr_buf = buf_ptr;
	                status = NU_NO_BUFFERS;
	            }
	        }
	
	        /* Otherwise, the link-layer header of the output interface will fit
	         * into the buffer chain that was passed in.
	         */
	        else
	            hdr_buf = buf_ptr;        
#else
	        hdr_buf = buf_ptr;
#endif
		}

		else
		{
			status = NU_NO_BUFFERS;
		}

        if (status == NU_SUCCESS)
        {
            /* Set the packet type that is in the buffer. */
            hdr_buf->mem_flags |= NET_IP;

            /* If the size of the packet is greater than the link MTU for the
             * device, fragment the packet.
             */
            if (GET16(packet, IP_TLEN_OFFSET) > ro.rt_route->rt_device->dev_mtu)
            {
#if INCLUDE_IP_FRAGMENT       

                status = IP_Fragment(hdr_buf, packet, ro.rt_route->rt_device, 
                                     de, &ro);
                                    
                if (status != NU_SUCCESS)
                    status = NU_MSGSIZE;

#else
                status = NU_MSGSIZE;
#endif
            }

            /* Otherwise, transmit the packet using the transmit routine of
             * the device.
             */
            else
            {
                status = 
                    ro.rt_route->rt_device->dev_output(hdr_buf, 
                                                       ro.rt_route->rt_device, 
                                                       de, &ro);  

                if (status != NU_SUCCESS)
                    status = NU_NO_DATA_TRANSFER;
            }
        }                                 
    }

    else if (internal_device != NU_NULL)
    {
        /* Pull the current buffer chain off of the receive list. */ 
        buf_ptr = MEM_Buffer_Dequeue(&MEM_Buffer_List);

		if (buf_ptr)
		{
	        /* Set the deallocation list pointer. */
	        buf_ptr->mem_dlist = &MEM_Buffer_Freelist;
	
	        /* Ensure that hlen is on a word-boundary since we will be advancing 
	         * the buffer pointer by this amount.
	         */
	        hlen += ((UINT32)buf_ptr->data_ptr % 4);

#if ((defined(NET_5_1)) && (NET_VERSION_COMP >= NET_5_1))
        
	        if (internal_device->dev_hdrlen > buf_ptr->mem_buf_device->dev_hdrlen)
	        {
	            buf_ptr->data_len           -= hlen;
	            buf_ptr->mem_total_data_len -= hlen;   
	            
	            /* Allocate a new buffer chain for the link-layer and IP header */
	            hdr_buf = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
	                (INT32)(hlen + internal_device->dev_hdrlen));
	            
	            /* If a new chain of buffers was allocated, link the buffer in as
	             * the first buffer in the list.
	             */
	            if (hdr_buf)
	            {
	                /* Link the new buffer in to the list */
	                hdr_buf->next_buffer = buf_ptr;
	                
	                /* Set the list to which the header buffers will be freed */
	                hdr_buf->mem_dlist = &MEM_Buffer_Freelist;
	                
	                /* If the buffer chain passed in is flagged as a parent chain,
	                * flag the new chain as a parent too.
	                */
	                if (buf_ptr->mem_flags & NET_PARENT)
	                    hdr_buf->mem_flags |= NET_PARENT;
	                
	                /* Set the data pointer to the end of the Physical layer    
	                 * header. 
	                 */
	                hdr_buf->data_ptr = 
	                    hdr_buf->mem_parent_packet + internal_device->dev_hdrlen;
	                
	                /* Set the total data length of the chain of buffers */
	                hdr_buf->mem_total_data_len = buf_ptr->mem_total_data_len;
	                          
	                /* If NAT appended a new buffer for the IP and link-layer 
	                 * headers to the buffer chain, copy the old header into 
	                 * the new buffer.
	                 */
	                memcpy(hdr_buf->data_ptr, buf_ptr->data_ptr, (unsigned int)hlen);
	            
	                /* Increment the buffer pointer to point to the data past the
	                 * IP header since the buffer prepended will now contain the
	                 * header.
	                 */
	                buf_ptr->data_ptr += hlen;
	
	                /* Update the length of the header buffer. */
	                hdr_buf->data_len           += hlen;
	                hdr_buf->mem_total_data_len += hlen;   
	            }
	             
	            /* If a buffer could not be allocated, return an error. */
	            else
	            {
	                hdr_buf = buf_ptr;
	                status = NU_NO_BUFFERS;
	            }
	        }
	
	        else
	            hdr_buf = buf_ptr;   
#else
        	hdr_buf = buf_ptr;
#endif
		}

		else
		{
			status = NU_NO_BUFFERS;
		}
            
        if (status == NU_SUCCESS)
        {   
            /* Set the packet type that is in the buffer. */
            hdr_buf->mem_flags |= NET_IP;
                
            /* If the size of the packet is greater than the link MTU for 
             * the device, fragment the packet.
             */
            if (GET16(packet, IP_TLEN_OFFSET) > internal_device->dev_mtu)
            {
#if INCLUDE_IP_FRAGMENT     
             
                status = 
                    IP_Fragment(hdr_buf, packet, internal_device, de, &ro);

                if (status != NU_SUCCESS)
                    status = NU_MSGSIZE;

#else
                status = NU_MSGSIZE;
#endif
            }

            /* Otherwise, transmit the packet using the transmit routine of
             * the device.
             */
            else    
            {
                status = internal_device->dev_output(hdr_buf, internal_device, 
                                                    de, NU_NULL);  

                if (status != NU_SUCCESS)
                    status = NU_NO_DATA_TRANSFER;            
            }
        }                  
    }

    else
    {
        status = NAT_INVAL_PARM;
    }

    /* If the packet was not transmitted successfully, free the buffer
     * chain.  Since this routine removed the packet from the incoming
     * buffer list, it needs to free all these buffers up. 
     */
    if ( (status != NU_SUCCESS) && (hdr_buf) )
    {
        /* Free the header chain of packets */
        MEM_One_Buffer_Chain_Free(hdr_buf, hdr_buf->mem_dlist);
    }

    return (status);

} /* NAT_Transmit_Packet */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Find_ICMP_Entry
*                                                                       
*   DESCRIPTION                                                           
*                   
*       This function finds a matching entry in the ICMP Translation 
*       Table.  If a match is found, the timeout value of the entry is
*       updated.
*                                                                       
*   INPUTS                                                                
*                               
*       source_addr             The IP address of the source.
*       icmp_type               The type of ICMP packet.
*       sequence_number         The sequence number in the ICMP packet.
*                                                                       
*   OUTPUTS                                                               
*                                                   
*       -1                      No matching entry exists in the table.
*       index                   The index of the entry in the table.                    
*                                                                       
****************************************************************************/
INT32 NAT_Find_ICMP_Entry(UINT32 source_addr, UINT32 icmp_type, 
                          UINT16 sequence_number)
{   
    INT             i;
    INT32           index = -1;
    NAT_ICMP_ENTRY  *icmp_entry;
    
    /* Search the Address Translation Table for a match */
    for (i = 0; i < NAT_MAX_ICMP_CONNS; i++)
    {
        icmp_entry = 
            &NAT_Translation_Table.nat_icmp_table->nat_icmp_entry[i];

        /* If this entry matches the target, update the timeout of the 
         * entry and return the index.
         */
        if ( (icmp_entry->nat_sequence_number == sequence_number) &&
             (icmp_entry->nat_valid_reply & icmp_type) &&            
             (icmp_entry->nat_destination_ip == source_addr) )
        {
            /* Update the timeout of the entry */
            icmp_entry->nat_timeout = NU_Retrieve_Clock();

            index = i;
            break;
        }
    }      

    return (index);

} /* NAT_Find_ICMP_Entry */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Find_Translation_Entry
*                                                                       
*   DESCRIPTION                                                           
*                   
*       This function finds a matching entry in the Translation Table.
*       If a match is found, the timeout value of the entry is
*       updated.
*                                                                       
*   INPUTS                                                                
*                                     
*       protocol                IPPROTO_TCP for TCP, IPPROTO_UDP for UDP.
*       device_type             NAT_INTERNAL_DEVICE if the packet was received
*                               from the internal network. NAT_EXTERNAL_DEVICE
*                               if the packet was received from the external 
*                               network.
*       source_addr             The IP address of the source.
*       source_port             The port of the source.
*       dest_addr               The IP address of the destination.
*       dest_port               The port of the destination.                                  
*       *portmap_entry          A pointer to the Portmap Entry that matches
*                               the Translation Entry.
*                                                                       
*   OUTPUTS                                                               
*                                                   
*       -1                      No matching entry exists in the table.
*       index                   The index of the entry in the table.                    
*                                                                       
*****************************************************************************/
INT32 NAT_Find_Translation_Entry(UINT8 protocol, UINT8 device_type, 
                                 UINT32 source_addr, UINT16 source_port, 
                                 UINT32 dest_addr, UINT16 dest_port,
                                 const NAT_PORTMAP_ENTRY *portmap_entry)
{
    INT             i;
    INT32           port_index, entry_index = -1;
#if (INCLUDE_TCP == NU_TRUE)
    NAT_TCP_ENTRY   *tcp_entry;
#endif
#if (INCLUDE_UDP == NU_TRUE)
    NAT_UDP_ENTRY   *udp_entry;
#endif

    switch (protocol)
    {
#if (INCLUDE_TCP == NU_TRUE)
    case IPPROTO_TCP:

        /* If the packet came in from the external device. */
        if (device_type == NAT_EXTERNAL_DEVICE)
        {
            /* If the portmap_entry is NU_NULL, the entry is using an external
             * port allocated by NAT, and we can determine the index into the
             * port list based on that port number.  If the portmap_entry is 
             * not NU_NULL, the entry is not using a port allocated by NAT.
             */
            if (portmap_entry == NU_NULL)
            {
                /* Calculate the index into the port list */
                port_index = dest_port - NAT_MIN_PORT;
    
                /* Check that the index is in the bounds of the array */
                if ( (port_index >= 0) && (port_index < NAT_MAX_TCP_CONNS) )
                {
                    /* Get the index into the Translation Table from the entry 
                     * in the port list.
                     */
                    entry_index =   
                        NAT_Translation_Table.nat_port_list->
                        nat_port_list[port_index].nat_tcp_used_flag;

                    /* Check that the value is not -1 */
                    if (entry_index >= 0)
                    {
                        /* Get a pointer to the entry and update the timeout of 
                         * the entry 
                         */
                        tcp_entry = &NAT_Translation_Table.nat_tcp_table->
                                        nat_tcp_entry[entry_index];

                        /* Make sure the entry has not timed out or been reused */
                        if ( (tcp_entry->nat_destination_port == source_port) &&
                             (NAT_Translation_Table.nat_port_list->nat_port_list[port_index].
                              nat_port == dest_port) && 
                              (tcp_entry->nat_destination_ip == source_addr) &&
                              (NAT_External_Device->dev_addr.dev_ip_addr == dest_addr) )
                            tcp_entry->nat_timeout = NU_Retrieve_Clock();
                        else
                            entry_index = -1;
                    }
                }
                else
                    entry_index = -1;
            }
            else
            {
                /* Search the Address Translation Table for a match */
                for (i = 0; i < NAT_MAX_TCP_CONNS; i++)
                {
                    tcp_entry =     
                        &NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[i];
    
                    /* If this entry matches the target, return the index */
                    if ( (tcp_entry->nat_destination_port == source_port) &&
					     (tcp_entry->nat_internal_source_port == dest_port) &&
                         (portmap_entry->nat_external_source_port == dest_port) &&
                         (tcp_entry->nat_destination_ip == source_addr) &&
                         (NAT_External_Device->dev_addr.dev_ip_addr == dest_addr) )
                    {
                        /* Update the timeout of the entry */
                        tcp_entry->nat_timeout = NU_Retrieve_Clock();

                        entry_index = i;
                        break;
                    }
                }   
            }
        }

        /* Otherwise, the packet came from the internal network */
        else
        {
            /* Search the Address Translation Table for a match */
            for (i = 0; i < NAT_MAX_TCP_CONNS; i++)
            {
                tcp_entry = 
                    &NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[i];

                /* If this entry matches the target, return the index */
                if ( (tcp_entry->nat_destination_port == dest_port) &&
                     (tcp_entry->nat_internal_source_port == source_port) &&
                     (tcp_entry->nat_destination_ip == dest_addr) &&
                     (tcp_entry->nat_internal_source_ip == source_addr) )
                {
                    /* Update the timeout of the entry */
                    tcp_entry->nat_timeout = NU_Retrieve_Clock();

                    entry_index = i;
                    break;
                }
            }   
        }

        break;
#endif

#if (INCLUDE_UDP == NU_TRUE)
    case IPPROTO_UDP:

        /* If the packet came from the external device. */
        if (device_type == NAT_EXTERNAL_DEVICE)
        {
            /* If the portmap_entry is NU_NULL, the entry is using an external
             * port allocated by NAT, and we can determine the index into the
             * port list based on that port number.  If the portmap_entry is 
             * not NU_NULL, the entry is not using a port allocated by NAT.
             */
            if (portmap_entry == NU_NULL)
            {
                /* Calculate the index into the port list */
                port_index = dest_port - NAT_MIN_PORT;
    
                /* Check that the index is in the bounds of the array */
                if ( (port_index >= 0) && (port_index < NAT_MAX_UDP_CONNS) )
                {
                    /* Get the index into the Translation Table from the entry 
                     * in the port list.
                     */
                    entry_index = 
                        NAT_Translation_Table.nat_port_list->
                        nat_port_list[port_index].nat_udp_used_flag;

                    /* Check that the value is not -1 */
                    if (entry_index >= 0)
                    {
                        /* Get a pointer to the entry and update the timeout of 
                         * the entry 
                         */
                        udp_entry = 
                            &NAT_Translation_Table.nat_udp_table->
                            nat_udp_entry[entry_index];

                        /* Make sure the entry has not timed out or been reused */
                        if ( (udp_entry->nat_destination_port == source_port) &&
                             (NAT_Translation_Table.nat_port_list->
                              nat_port_list[port_index].nat_port == dest_port) &&
                             (udp_entry->nat_destination_ip == source_addr) &&
                             (NAT_External_Device->dev_addr.dev_ip_addr == dest_addr) )
                            udp_entry->nat_timeout = NU_Retrieve_Clock();
                        else
                            entry_index = -1;
                    }
                }
                else
                    entry_index = -1;
            }

            else
            {
                /* Search the Address Translation Table for a match */
                for (i = 0; i < NAT_MAX_UDP_CONNS; i++)
                {
                    udp_entry = 
                        &NAT_Translation_Table.nat_udp_table->nat_udp_entry[i];
    
                    /* If this entry matches the target, return the index */
                    if ( (udp_entry->nat_destination_port == source_port) &&
					     (udp_entry->nat_internal_source_port == dest_port) &&
                         (portmap_entry->nat_external_source_port == dest_port) &&
                         (udp_entry->nat_destination_ip == source_addr) &&
                         (NAT_External_Device->dev_addr.dev_ip_addr == dest_addr) )
                    {
                        /* Update the timeout of the entry */
                        udp_entry->nat_timeout = NU_Retrieve_Clock();

                        entry_index = i;
                        break;
                    }
                }   
            }
        }

        /* Otherwise, the packet came from the internal network */
        else
        {
            /* Search the Address Translation Table for a match */
            for (i = 0; i < NAT_MAX_UDP_CONNS; i++)
            {
                udp_entry = 
                    &NAT_Translation_Table.nat_udp_table->nat_udp_entry[i];
    
                /* If this entry matches the target, return the index */
                if ( (udp_entry->nat_destination_port == dest_port) &&
                     (udp_entry->nat_internal_source_port == source_port) &&
                     (udp_entry->nat_destination_ip == dest_addr) &&
                     (udp_entry->nat_internal_source_ip == source_addr) )
                {
                    /* Update the timeout of the entry */
                    udp_entry->nat_timeout = NU_Retrieve_Clock();

                    entry_index = i;
                    break;
                }
            }   
        }

        break;
#endif

    default:

        break;
    }

    return (entry_index);

} /* NAT_Find_Translation_Entry */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Delete_ICMP_Entry
*                                                                       
*   DESCRIPTION                                                           
*                     
*       This function deletes the entry from the ICMP Address 
*       Translation Table.                                                  
*                                                                       
*   INPUTS                                                                
*             
*       index                   The index of the entry in the table.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       None.
*                                                                       
****************************************************************************/
VOID NAT_Delete_ICMP_Entry(INT32 index)
{
    /* If the index is not -1 */
    if (index >= 0)
    {
        /* Delete the entry */
        UTL_Zero(&NAT_Translation_Table.nat_icmp_table->nat_icmp_entry[index], 
                 sizeof(NAT_ICMP_ENTRY));
    }

} /* NAT_Delete_ICMP_Entry */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Delete_Translation_Entry
*                                                                       
*   DESCRIPTION                                                           
*                     
*       This function deletes the entry from the Address Translation
*       Table.                                                  
*                                                                       
*   INPUTS                                                                
*             
*       protocol                IPPROTO_TCP for TCP, IPPROTO_UDP for UDP.
*       index                   The index of the entry in the table.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       None.
*                                                                       
****************************************************************************/
VOID NAT_Delete_Translation_Entry(UINT8 protocol, INT32 index)
{
    /* Ensure that the NAT module is up and running. */
    if (IP_NAT_Initialize)
    {
        /* If the index is not -1 */
        if (index >= 0)
        {
            switch (protocol)
            {
#if (INCLUDE_TCP == NU_TRUE)
            case IPPROTO_TCP:

                if (NAT_Translation_Table.nat_tcp_table->
                    nat_tcp_entry[index].nat_timeout != 0)
                {
                    /* Mark the port this entry used as available */
                    if (NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[index].
                        nat_external_port_index >= 0)
                    {
                        NAT_Translation_Table.nat_port_list->
                            nat_port_list[NAT_Translation_Table.nat_tcp_table->
                            nat_tcp_entry[index].nat_external_port_index].
                            nat_tcp_used_flag = -1; 
                    }

                    /* If this entry has a corresponding entry in the FTP ALG table,
                     * delete the entry in the FTP ALG table.
                     */
#if (NAT_INCLUDE_FTP_ALG == NU_TRUE)
                    if (NAT_Translation_Table.nat_tcp_table->
                        nat_tcp_entry[index].nat_ftp.nat_ftp_index >= 0)
                    {
                        ALG_Delete_FTP_Entry(NAT_Translation_Table.nat_tcp_table->
                                             nat_tcp_entry[index].nat_ftp.nat_ftp_index);
                    }
#endif

                    /* Delete the entry */
                    UTL_Zero(&NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[index], 
                             sizeof(NAT_TCP_ENTRY));        

#if (NAT_INCLUDE_FTP_ALG == NU_TRUE)
                    NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[index].
                        nat_ftp.nat_ftp_index = -1;
#endif
                    NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[index].
                        nat_state = -1;
                }

                break;
#endif

#if (INCLUDE_UDP == NU_TRUE)
            case IPPROTO_UDP:

                if (NAT_Translation_Table.nat_udp_table->nat_udp_entry[index].
                    nat_timeout != 0)
                {
                    /* Mark the port this entry used as available */
                    if (NAT_Translation_Table.nat_udp_table->nat_udp_entry[index].
                        nat_external_port_index >= 0)
                    {
                        NAT_Translation_Table.nat_port_list->
                            nat_port_list[NAT_Translation_Table.
                            nat_udp_table->nat_udp_entry[index].
                            nat_external_port_index].nat_udp_used_flag = -1;
                    }

                    /* Delete the entry */
                    UTL_Zero(&NAT_Translation_Table.nat_udp_table->nat_udp_entry[index], 
                             sizeof(NAT_UDP_ENTRY));        
                }

                break;
#endif

            default:

                break;
            }
        }
    }

} /* NAT_Delete_Translation_Entry */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Assign_Port
*                                                                       
*   DESCRIPTION                                                           
*            
*       This function assigns an external port number for the NAT 
*       session and updates the next available port index for the
*       specified protocol.
*                                                                       
*   INPUTS                                                                
*       
*       protocol                IPPROTO_TCP for TCP, IPPROTO_UDP for UDP.                                                                
*                                                                       
*   OUTPUTS                                                               
*                                                                
*       -1                      No ports are available to be assigned.
*       port                    The external port number.       
*                                                                       
****************************************************************************/
INT32 NAT_Assign_Port(UINT8 protocol)
{
    INT     i;
    INT32   next_index = -1, next_avail_index;  

    switch (protocol)
    {
#if (INCLUDE_TCP == NU_TRUE)
    case IPPROTO_TCP:

        /* Get the next available index into the TCP port list */
        next_index = 
            NAT_Translation_Table.nat_port_list->nat_next_avail_tcp_port_index;

        /* If next_index is -1, there were no ports available to be allocated 
         * the last time a port was allocated, so search through the list 
         * again to see if any ports have come available since then.
         */
        if (next_index == -1)
        {
            for (i = 0; i < NAT_MAX_TCP_CONNS; i++)
            {
                /* If the port is not in use, take it */
                if (NAT_Translation_Table.nat_port_list->nat_port_list[i].
                    nat_tcp_used_flag == -1)
                {
                    /* Set the value of the port index to be returned */
                    if (next_index == -1)
                        next_index = i;

                    /* Set the value of the next available TCP port index */
                    else
                    {
                        NAT_Translation_Table.nat_port_list->
                            nat_next_avail_tcp_port_index = i;                    

                        break;
                    }
                }
            }
        }

        /* Otherwise, update the next available TCP port to be allocated */
        else
        {  
            /* Set the next available TCP port to allocated to -1 initially */
            NAT_Translation_Table.nat_port_list->
                nat_next_avail_tcp_port_index = -1;   

            /* Set the next available to the entry after the one just found */
            next_avail_index = next_index + 1;

            /* Find the next port available */
            for (i = 0; i < NAT_MAX_TCP_CONNS - 1; i++, next_avail_index++)
            {
                /* If we have exceeded the size of the array, wrap around to the
                 * beginning of the list.
                 */
                if (next_avail_index >= NAT_MAX_TCP_CONNS)
                    next_avail_index = 0;

                /* If this port is available, set it as the next available TCP
                 * port to be allocated.
                 */
                if (NAT_Translation_Table.nat_port_list->
                    nat_port_list[next_avail_index].nat_tcp_used_flag == -1)
                {
                    NAT_Translation_Table.nat_port_list->
                        nat_next_avail_tcp_port_index = next_avail_index;   

                    break;
                }
            }
        }
        
        break;
#endif

#if INCLUDE_UDP

    case IPPROTO_UDP:

        /* Get the next available index into the UDP port list */
        next_index = 
            NAT_Translation_Table.nat_port_list->nat_next_avail_udp_port_index;

        /* If port_index is -1, there were no ports available to be allocated the 
         * last time a port was allocated, so search through the list again to 
         * see if any ports have come available since then.
         */
        if (next_index == -1)
        {
            for (i = 0; i < NAT_MAX_UDP_CONNS; i++)
            {
                /* If the port is not in use, take it */
                if (NAT_Translation_Table.nat_port_list->nat_port_list[i].
                    nat_udp_used_flag == -1)
                {
                    /* Set the value of the port index to be returned */
                    if (next_index == -1)
                        next_index = i;

                    /* Set the value of the next available UDP port index */
                    else
                    {
                        NAT_Translation_Table.nat_port_list->
                            nat_next_avail_udp_port_index = i;                    

                        break;
                    }
                }
            }
        }

        /* Otherwise, update the next available UDP port to be allocated */
        else
        {  
            /* Set the next available TCP port to allocated to -1 initially */
            NAT_Translation_Table.nat_port_list->
                nat_next_avail_udp_port_index = -1;

            /* Set the next available to the entry after the one just found */
            next_avail_index = next_index + 1;                    

            /* Find the next port available */
            for (i = 0; i < NAT_MAX_UDP_CONNS - 1; i++, next_avail_index++)
            {
                /* If we have exceeded the size of the array, wrap around to the
                 * beginning of the list.
                 */
                if (next_avail_index >= NAT_MAX_UDP_CONNS)
                    next_avail_index = 0;

                /* If this port is available, set it as the next available TCP
                 * port to be allocated.
                 */
                if (NAT_Translation_Table.nat_port_list->
                    nat_port_list[next_avail_index].nat_udp_used_flag == -1)
                {
                    NAT_Translation_Table.nat_port_list->
                        nat_next_avail_udp_port_index = next_avail_index;

                    break;
                }
            }
        }
        
        break;
#endif

    default:

        break;
    }

    return (next_index);

} /* NAT_Assign_Port */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Find_Portmap_Entry
*                                                                       
*   DESCRIPTION                                                           
*                       
*       This function finds a matching entry in the Portmap Table and
*       returns a pointer to the entry.                                               
*                                                                       
*   INPUTS                             
*                                   
*       protocol                IPPROTO_TCP for TCP, IPPROTO_UDP for UDP.
*       device_type             The device on which the packet was 
*                               received. Either NAT_EXTERNAL_DEVICE or 
*                               NAT_INTERNAL_DEVICE.
*       source_addr             The source address.
*       source_port             The source port.
*       dest_addr               The destination address.
*       dest_port               The destination port.                                                                       
*                                                                       
*   OUTPUTS                                                               
*                                            
*       NU_NULL                 No matching entry exists in the Portmap 
*                               Table.
*       *entry                  A pointer to the matching entry.                           
*                                                                       
****************************************************************************/
NAT_PORTMAP_ENTRY *NAT_Find_Portmap_Entry(UINT8 protocol, UINT8 device_type,
                                          UINT32 source_addr, UINT16 source_port, 
                                          UINT32 dest_addr, UINT16 dest_port)
{
    NAT_PORTMAP_ENTRY   *current_entry;

    /* Get a pointer to the head of the list */
    current_entry = NAT_Portmap_Table.nat_head;

    /* If the packet came from the external device */
    if (device_type == NAT_EXTERNAL_DEVICE)
    {
        /* Traverse the list while there are entries remaining */
        while (current_entry != NU_NULL)
        {
            /* If the source port of the current entry is greater than the
             * source port of the target, the entry does not exist in the
             * list.  Entries are added in order of port number.
             */
            if (current_entry->nat_external_source_port > dest_port)
            {
                current_entry = NU_NULL;
                break;
            }

            /* Check if this is a matching entry */
            else if ( (dest_port == current_entry->nat_external_source_port) && 
                      (protocol == current_entry->nat_protocol) &&
                      (current_entry->nat_used_flag == 1) &&
                      (dest_addr == NAT_External_Device->dev_addr.dev_ip_addr) )
                break;

            /* Get the next entry */
            current_entry = current_entry->nat_next;
        }
    }

    /* Otherwise, the packet came from the internal network */
    else
    {
        /* Traverse the list while there are entries remaining */
        while (current_entry != NU_NULL)
        {
            /* If the source port of the current entry is greater than the
             * source port of the target, the entry does not exist in the
             * list.  Entries are added in order of port number.
             */
            if (current_entry->nat_internal_source_port > source_port)
            {
                current_entry = NU_NULL;
                break;
            }

            /* Otherwise, this could be a reply from the internal network */
            else if ( (source_port == current_entry->nat_internal_source_port) &&
                      (protocol == current_entry->nat_protocol) &&
                      (current_entry->nat_used_flag == 1) &&
                      (source_addr == current_entry->nat_internal_source_ip) )
                break;

            /* Get the next entry */
            current_entry = current_entry->nat_next;
        }
    }

    return (current_entry);

} /* NAT_Find_Portmap_Entry */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Update_Portmap_Table
*                                                                       
*   DESCRIPTION                                                           
*                       
*       This function insures that each server within the private 
*       network will receive an equal number of requests.  If there are
*       multiple servers of the same type registered with the portmap
*       service, requests from the external network will be routed to
*       the internal servers in a round-robin manner.
*                                                                       
*   INPUTS                             
*                                   
*       *portmap_entry          A pointer to the NAT_PORTMAP_ENTRY which
*                               has just been used.
*                                                                       
*   OUTPUTS                                                               
*                                            
*       None.
*                                                                       
****************************************************************************/
VOID NAT_Update_Portmap_Table(NAT_PORTMAP_ENTRY *portmap_entry)
{
    NAT_PORTMAP_ENTRY   *current_entry;

    /* Get the first entry after the current entry */
    current_entry = portmap_entry->nat_next;

    if (current_entry == NU_NULL)
            current_entry = NAT_Portmap_Table.nat_head;

    /* Traverse the entire list */
    while (current_entry != portmap_entry)
    {
        /* If the current entry matches the target and the used flag is 0,
         * set the used flag of the current entry to 1 and the used flag
         * of the target to 0 to insure the Round Robin nature of duplicate
         * entries within the internal network.
         */
        if ( (current_entry->nat_external_source_port == 
              portmap_entry->nat_external_source_port) && 
             (current_entry->nat_protocol == portmap_entry->nat_protocol) &&
             (current_entry->nat_used_flag == 0) )
        {   
            current_entry->nat_used_flag = 1;
            portmap_entry->nat_used_flag = 0;
        }

        /* Get the next entry */
        current_entry = current_entry->nat_next;

        if (current_entry == NU_NULL)
            current_entry = NAT_Portmap_Table.nat_head;
    }

} /* NAT_Update_Portmap_Table */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Add_ICMP_Translation_Entry
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function adds an ICMP entry to the Address Translation 
*       Table.
*                                                                       
*   INPUTS                                                                
*              
*       source_addr             The source address.
*       dest_addr               The destination address.
*       icmp_type               The type of ICMP packet being sent.
*       sequence_number         The sequence number of the packet.
*       *internal_device        A pointer to the device
*                                                                       
*   OUTPUTS                                                               
*                                            
*       -1                      There is no room in the table for a new entry.
*       -2                      This ICMP message does not need a reply, so it
*                               does not need to be added to the ICMP table.
*       index                   The index of the entry in the table.                           
*                                                                       
****************************************************************************/
INT32 NAT_Add_ICMP_Translation_Entry(UINT32 source_addr, UINT32 dest_addr, 
                                     UINT32 icmp_type, UINT16 sequence_number,
                                     DV_DEVICE_ENTRY *internal_device)
{
    UINT32  icmp_reply_type = (NAT_ICMP_UNREACH | NAT_ICMP_TIMXCEED | NAT_ICMP_PARAPROB);
    INT     i;
    INT32   next_index, next_avail_index;

    /* Determine the appropriate reply type for each ICMP packet code */
    if (icmp_type & NAT_ICMP_ECHO)
        icmp_reply_type |= NAT_ICMP_ECHOREPLY;

    else if (icmp_type & NAT_ICMP_TIMESTAMP)
        icmp_reply_type |= NAT_ICMP_TIMESTAMPREPLY;

    else if (icmp_type & NAT_ICMP_INFOREQUEST)
        icmp_reply_type |= NAT_ICMP_INFOREPLY;

    else
        return (-2);

    /* Get the index of the next ICMP entry to be allocated */
    next_index = 
        NAT_Translation_Table.nat_icmp_table->nat_next_available_icmp_index;

    /* If the next index is -1, there were no available entries in the table
     * the last time an entry was added.  Traverse the list to see if any
     * entries have come available since.
     */
    if (next_index == -1)
    {
        for (i = 0; i < NAT_MAX_ICMP_CONNS; i++)
        {
            /* If the timeout is 0, the entry is available */
            if (NAT_Translation_Table.nat_icmp_table->
                nat_icmp_entry[i].nat_timeout == 0)
            {
                /* Set the value of the next index */
                if (next_index == -1)
                    next_index = i;

                /* Set the value of the index of the next ICMP entry to be
                 * allocated.
                 */
                else
                {
                    NAT_Translation_Table.nat_icmp_table->
                        nat_next_available_icmp_index = i;

                    break;
                }
            }
        }
    }

    /* Otherwise, there is a valid next ICMP entry to be allocated. */
    else
    {  
        /* Initially, set the value of the next ICMP entry to be allocated 
         * to -1. 
         */
        NAT_Translation_Table.nat_icmp_table->
            nat_next_available_icmp_index = -1;

        next_avail_index = next_index + 1;

        /* Find an available entry, and set it as the next ICMP entry to
         * be allocated.
         */
        for (i = 0; i < NAT_MAX_ICMP_CONNS; i++, next_avail_index++)
        {
            /* If we reached the end of the list, wrap around to the 
             * beginning. 
             */
            if (next_avail_index >= NAT_MAX_ICMP_CONNS)
                next_avail_index = 0;

            /* If this entry is available, set it as the next available 
             * ICMP entry to be allocated. 
             */
            if (NAT_Translation_Table.nat_icmp_table->
                nat_icmp_entry[next_avail_index].nat_timeout == 0)
            {
                NAT_Translation_Table.nat_icmp_table->
                    nat_next_available_icmp_index = next_avail_index;

                break;
            }
        }
    }

    /* If there is an available entry, assign it. */
    if (next_index != -1)
    {
        NAT_Translation_Table.nat_icmp_table->
            nat_icmp_entry[next_index].nat_internal_device = internal_device;

        NAT_Translation_Table.nat_icmp_table->nat_icmp_entry[next_index].
            nat_destination_ip = dest_addr;

        NAT_Translation_Table.nat_icmp_table->nat_icmp_entry[next_index].
            nat_internal_source_ip = source_addr;

        /* Update the timeout of the entry */
        NAT_Translation_Table.nat_icmp_table->nat_icmp_entry[next_index].
            nat_timeout = NU_Retrieve_Clock();

        NAT_Translation_Table.nat_icmp_table->nat_icmp_entry[next_index].
            nat_valid_reply = icmp_reply_type;

        NAT_Translation_Table.nat_icmp_table->nat_icmp_entry[next_index].
            nat_sequence_number = sequence_number;
    }

    return (next_index);

} /* NAT_Add_ICMP_Translation_Entry */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Add_Translation_Entry
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function adds an entry to the Address Translation Table.                                                      
*                                                                       
*   INPUTS                                                                
*              
*       protocol                IPPROTO_TCP for TCP, IPPROTO_UDP for UDP.
*       source_addr             The source address.
*       source_port             The source port.
*       dest_addr               The destination address.
*       dest_port               The destination port.
*       *nat_device             A pointer to the Device             
*       *portmap_entry          A pointer to the NAT_PORTMAP_ENTRY 
*                               corresponding to the new Translation 
*                               entry.                                            
*                                                                       
*   OUTPUTS                                                               
*                                            
*       -1              There is no room in the table for a new entry.
*       index           The index of the entry in the table.                           
*                                                                       
****************************************************************************/
INT32 NAT_Add_Translation_Entry(UINT8 protocol, UINT32 source_addr, 
                                UINT16 source_port, UINT32 dest_addr, 
                                UINT16 dest_port, DV_DEVICE_ENTRY *nat_device,
                                NAT_PORTMAP_ENTRY *portmap_entry)
{
    INT32   port_index, next_index = -1, next_avail_index, i;

    switch (protocol)
    {
#if (INCLUDE_TCP == NU_TRUE)
    case IPPROTO_TCP:

        /* Get the index of the next available TCP entry to allocate */
        next_index = 
            NAT_Translation_Table.nat_tcp_table->nat_next_available_tcp_index;

        /* If the index is -1, there were no entries available the last time  
         * a new entry was made.  Check if any entries have come available 
         * since then.
         */
        if (next_index == -1)
        {
            /* Traverse the list looking for an available entry */
            for (i = 0; i < NAT_MAX_TCP_CONNS; i++)
            {
                /* If the timeout is 0, the entry is available */
                if (NAT_Translation_Table.nat_tcp_table->
                    nat_tcp_entry[i].nat_timeout == 0)
                {
                    /* Assign the index value */
                    next_index = i;
                    break;
                }
            }
        }

        /* If we found an available entry, assign it */
        if (next_index != -1)
        {
            /* If there is no corresponding portmap entry, assign an external 
             * port for the entry.
             */
            if (portmap_entry == NU_NULL)
            {
                if ((port_index = NAT_Assign_Port(IPPROTO_TCP)) == -1)
                    next_index = -1;
                else
                {
                    NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[next_index].
                        nat_external_port_index = port_index;

                    NAT_Translation_Table.nat_port_list->nat_port_list[port_index].
                        nat_tcp_used_flag = next_index;
                }
            }

            /* Otherwise, set the index into the port list to -1 */
            else
            {
                NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[next_index].
                    nat_external_port_index = -1;
            }

            /* If a port was able to be assigned, assign the remainder of the 
             * values. 
             */
            if (next_index != -1)
            {   
                NAT_Translation_Table.nat_tcp_table->
                    nat_tcp_entry[next_index].nat_internal_device = nat_device;

                NAT_Translation_Table.nat_tcp_table->
                    nat_tcp_entry[next_index].nat_portmap_entry = portmap_entry;

                NAT_Translation_Table.nat_tcp_table->
                    nat_tcp_entry[next_index].nat_destination_ip = dest_addr;
    
                NAT_Translation_Table.nat_tcp_table->
                    nat_tcp_entry[next_index].nat_internal_source_ip = source_addr;
        
                NAT_Translation_Table.nat_tcp_table->
                    nat_tcp_entry[next_index].nat_destination_port = dest_port;

                NAT_Translation_Table.nat_tcp_table->
                    nat_tcp_entry[next_index].nat_internal_source_port = source_port;

                /* Update the timeout */
                NAT_Translation_Table.nat_tcp_table->
                    nat_tcp_entry[next_index].nat_timeout = NU_Retrieve_Clock();

                /* Initially, set the next available TCP entry to -1 */
                NAT_Translation_Table.nat_tcp_table->nat_next_available_tcp_index = -1;
    
                next_avail_index = next_index + 1;                    
    
                for (i = 0; i < NAT_MAX_TCP_CONNS - 1; i++, next_avail_index++)
                {
                    /* If we reached the end of the list, wrap around to the 
                     * beginning. 
                     */
                    if (next_avail_index >= NAT_MAX_TCP_CONNS)
                        next_avail_index = 0;
    
                    /* If this entry is available, take it */
                    if (NAT_Translation_Table.nat_tcp_table->
                        nat_tcp_entry[next_avail_index].nat_timeout == 0)
                    {
                        NAT_Translation_Table.nat_tcp_table->
                            nat_next_available_tcp_index = next_avail_index;

                        break;
                    }
                }
            }
        }

        break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

    case IPPROTO_UDP:

        /* Get the index of the next available TCP entry to allocate */
        next_index = 
            NAT_Translation_Table.nat_udp_table->nat_next_available_udp_index;

        /* If the index is -1, there were no entries available the last time  
         * a new entry was made.  Check if any entries have come available 
         * since then.
         */
        if (next_index == -1)
        {
            /* Traverse the list looking for an available entry */
            for (i = 0; i < NAT_MAX_UDP_CONNS; i++)
            {
                /* If the timeout is 0, the entry is available */
                if (NAT_Translation_Table.nat_udp_table->
                    nat_udp_entry[i].nat_timeout == 0)
                {
                    next_index = i;
                    break;
                }
            }
        }

        /* If we found an available entry, assign it */
        if (next_index != -1)
        {
            /* If there is no corresponding portmap entry, assign an external 
             * port for the entry.
             */
            if (portmap_entry == NU_NULL)
            {
                if ((port_index = NAT_Assign_Port(IPPROTO_UDP)) == -1)
                    next_index = -1;
                else
                {
                    NAT_Translation_Table.nat_udp_table->
                        nat_udp_entry[next_index].nat_external_port_index = 
                        port_index;

                    NAT_Translation_Table.nat_port_list->
                        nat_port_list[port_index].nat_udp_used_flag = 
                        next_index;
                }
            }

            /* Otherwise, set the index into the port list to -1 */
            else
                NAT_Translation_Table.nat_udp_table->
                    nat_udp_entry[next_index].nat_external_port_index = -1;

            /* If a port was able to be assigned, assign the remainder of the 
             * values. 
             */
            if (next_index != -1)
            {   
                NAT_Translation_Table.nat_udp_table->
                    nat_udp_entry[next_index].nat_internal_device = nat_device;

                NAT_Translation_Table.nat_udp_table->
                    nat_udp_entry[next_index].nat_portmap_entry = portmap_entry;

                NAT_Translation_Table.nat_udp_table->
                    nat_udp_entry[next_index].nat_destination_ip = dest_addr;
    
                NAT_Translation_Table.nat_udp_table->
                    nat_udp_entry[next_index].nat_internal_source_ip = source_addr;
        
                NAT_Translation_Table.nat_udp_table->
                    nat_udp_entry[next_index].nat_destination_port = dest_port;

                NAT_Translation_Table.nat_udp_table->
                    nat_udp_entry[next_index].nat_internal_source_port = source_port;
                
                /* Update the timeout */
                NAT_Translation_Table.nat_udp_table->
                    nat_udp_entry[next_index].nat_timeout = NU_Retrieve_Clock();

                /* Initially, set the next available TCP entry to -1 */
                NAT_Translation_Table.nat_udp_table->nat_next_available_udp_index = -1;
    
                next_avail_index = next_index + 1;                    
    
                for (i = 0; i < NAT_MAX_UDP_CONNS - 1; i++, next_avail_index++)
                {
                    /* If we reached the end of the list, wrap around to the 
                     * beginning. 
                     */
                    if (next_avail_index >= NAT_MAX_UDP_CONNS)
                        next_avail_index = 0;
    
                    /* If this entry is available, take it */
                    if (NAT_Translation_Table.nat_udp_table->
                        nat_udp_entry[next_avail_index].nat_timeout == 0)
                    {
                        NAT_Translation_Table.nat_udp_table->
                            nat_next_available_udp_index = next_avail_index;
                        break;
                    }
                }
            }
        }

        break;
#endif

    default:

        break;
    }

    return (next_index);

} /* NAT_Add_Translation_Entry */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Parse_Packet
*                                                                       
*   DESCRIPTION                                                           
*                                                           
*       This function parses data from the packet.            
*                                                                       
*   INPUTS                                                                
*                                    
*       *pkt                    A pointer to the packet.
*       *buf_ptr                A pointer to the buffer.
*       *nat_packet             A pointer to the NAT_PACKET into which to 
*                               place the parsed data.
*                                                                       
*   OUTPUTS                                                               
*                                               
*       NU_SUCCESS              The data was successfully parsed.
*       NAT_NO_ENTRY            The packet type is not supported by NAT.
*                                                                       
****************************************************************************/
STATUS NAT_Parse_Packet(IPLAYER *pkt, NET_BUFFER *buf_ptr, 
                        NAT_PACKET *nat_packet)
{
    STATUS  status = NU_SUCCESS;

    /* Extract the protocol type, destination address, source address and 
     * IP checksum of the original packet.
     */
    nat_packet->nat_protocol = GET8(pkt, IP_PROTOCOL_OFFSET);
    nat_packet->nat_dest_addr = GET32(pkt, IP_DEST_OFFSET);
    nat_packet->nat_source_addr = GET32(pkt, IP_SRC_OFFSET);
    nat_packet->nat_ip_checksum = GET16(pkt, IP_CHECK_OFFSET);

    /* Extract the length of the header */
    nat_packet->nat_ip_header_length = 
        (GET8(pkt, IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2;

    switch (nat_packet->nat_protocol)
    {
#if (INCLUDE_TCP == NU_TRUE)
    case IPPROTO_TCP:

        /* Get the source port */
        nat_packet->nat_source_port = 
            GET16(buf_ptr->data_ptr, 
                  (unsigned int)(nat_packet->nat_ip_header_length + 
                  TCP_SRC_OFFSET));

        /* Get the destination port */
        nat_packet->nat_dest_port = 
            GET16(buf_ptr->data_ptr, 
                  (unsigned int)(nat_packet->nat_ip_header_length + 
                  TCP_DEST_OFFSET));

        /* Get the TCP checksum */
        nat_packet->nat_prot_checksum = 
            GET16(buf_ptr->data_ptr, 
                  (unsigned int)(nat_packet->nat_ip_header_length + 
                  TCP_CHECK_OFFSET));

        /* Get the control bit */
        nat_packet->nat_control_bit = 
            GET8(buf_ptr->data_ptr, nat_packet->nat_ip_header_length + 
                 TCP_FLAGS_OFFSET);


        /* Get the length of the TCP header */
        nat_packet->nat_prot_header_length = 
            GET8(buf_ptr->data_ptr, nat_packet->nat_ip_header_length +
                 TCP_HLEN_OFFSET) >> 2;

        /* Get the length of the data */
        nat_packet->nat_prot_data_length = 
            (buf_ptr->mem_total_data_len - nat_packet->nat_ip_header_length -
             nat_packet->nat_prot_header_length);

        break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

    case IPPROTO_UDP:

        /* Get the source port */
        nat_packet->nat_source_port = 
            GET16(buf_ptr->data_ptr, 
                  (unsigned int)(nat_packet->nat_ip_header_length + 
                  UDP_SRC_OFFSET));

        /* Get the destination port */
        nat_packet->nat_dest_port = 
            GET16(buf_ptr->data_ptr, 
                  (unsigned int)(nat_packet->nat_ip_header_length + 
                  UDP_DEST_OFFSET));

        /* Get the UDP checksum */
        nat_packet->nat_prot_checksum = 
            GET16(buf_ptr->data_ptr, 
                  (unsigned int)(nat_packet->nat_ip_header_length + 
                  UDP_CHECK_OFFSET));

        /* Set the UDP header length */
        nat_packet->nat_prot_header_length = UDP_HEADER_LEN;

        /* Get the length of UDP data */
        nat_packet->nat_prot_data_length = 
            (buf_ptr->mem_total_data_len - nat_packet->nat_ip_header_length -
             nat_packet->nat_prot_header_length);

        break;
#endif

    case IPPROTO_ICMP:
    
        /* Parse the ICMP packet */
        status = NAT_ICMP_Parse_Packet(buf_ptr, nat_packet);

        break;

    default:
        status = NAT_NO_ENTRY;
    }

    return (status);

} /* NAT_Parse_Packet */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_ICMP_Parse_Packet
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function parses the parameters from the ICMP packet.  The
*       buffer pointer is the beginning of the ICMP data.
*                                                                       
*   INPUTS                                                                
*          
*       *buf_ptr                A pointer to the NET_BUFFER data structure.
*       *nat_packet             A pointer to the NAT_PACKET which contains 
*                               the translated data from the IP and TCP/UDP 
*                               headers.
*                                                                       
*   OUTPUTS                                                               
*                                                     
*       NU_SUCCESS              The packet was successfully parsed.
*       NAT_NO_ENTRY            The packet type is not valid.  
*                                                                       
****************************************************************************/
STATUS NAT_ICMP_Parse_Packet(const NET_BUFFER *buf_ptr, NAT_PACKET *nat_packet)
{
    UINT8   *buffer;

    UTL_Zero(&nat_packet->nat_icmp_packet, sizeof(NAT_ICMP_PACKET));

    buffer = buf_ptr->data_ptr + nat_packet->nat_ip_header_length;
    
    /* Determine the type */
    nat_packet->nat_icmp_packet.nat_icmp_type = 
        1 << GET8(buffer, ICMP_TYPE_OFFSET);
            
    /* Get the sequence number */
    nat_packet->nat_icmp_packet.nat_icmp_sequence_number = 
        GET16(buffer, ICMP_SEQ_OFFSET);

    /* If the type is an error packet, parse out the data from within the error
     * packet that can identify the internal node.
     */
    if ( (nat_packet->nat_icmp_packet.nat_icmp_type & NAT_ICMP_UNREACH) ||
         (nat_packet->nat_icmp_packet.nat_icmp_type & NAT_ICMP_SOURCEQUENCH) ||
         (nat_packet->nat_icmp_packet.nat_icmp_type & NAT_ICMP_TIMXCEED) ||
         (nat_packet->nat_icmp_packet.nat_icmp_type & NAT_ICMP_PARAPROB) )
    {
        /* Determine the protocol */
        nat_packet->nat_icmp_packet.nat_icmp_proto = 
            GET8(buffer, ICMP_IP_OFFSET + IP_PROTOCOL_OFFSET);
   
        switch (nat_packet->nat_icmp_packet.nat_icmp_proto)
        {
        case IPPROTO_TCP:
        case IPPROTO_UDP:

            /* Get the original source and destination address */
            nat_packet->nat_icmp_packet.nat_icmp_source_addr = 
                GET32(buffer, ICMP_IP_OFFSET + IP_SRC_OFFSET);

            nat_packet->nat_icmp_packet.nat_icmp_dest_addr = 
                GET32(buffer, ICMP_IP_OFFSET + IP_DEST_OFFSET); 
    
            /* Get the IP checksum and length of data over which the 
             * checksum was calculated.
             */
            nat_packet->nat_icmp_packet.nat_icmp_ip_checksum = 
                GET16(buffer, ICMP_IP_OFFSET + IP_CHECK_OFFSET);

            nat_packet->nat_icmp_packet.nat_icmp_ip_header_length = 
                ((GET8(buffer, ICMP_IP_OFFSET + IP_VERSIONANDHDRLEN_OFFSET) & 
                 0x0f) << 2);

            /* Get the ICMP checksum and length of data over which the 
             * checksum was calculated.
             */
            nat_packet->nat_icmp_packet.nat_icmp_checksum = 
                GET16(buffer, ICMP_CKSUM_OFFSET);

            nat_packet->nat_icmp_packet.nat_icmp_checksum_length = 
                buf_ptr->mem_total_data_len - nat_packet->nat_ip_header_length;

            switch (nat_packet->nat_icmp_packet.nat_icmp_proto)
            {

#if (INCLUDE_TCP == NU_TRUE)
            case IPPROTO_TCP:

                nat_packet->nat_icmp_packet.nat_icmp_source_port = 
                    GET16(buffer, (unsigned int)(ICMP_IP_OFFSET + 
                          nat_packet->nat_icmp_packet.nat_icmp_ip_header_length + 
                          TCP_SRC_OFFSET));

                nat_packet->nat_icmp_packet.nat_icmp_dest_port = 
                    GET16(buffer, (unsigned int)(ICMP_IP_OFFSET + 
                    nat_packet->nat_icmp_packet.nat_icmp_ip_header_length + 
                    TCP_DEST_OFFSET));

                break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

            case IPPROTO_UDP:

                nat_packet->nat_icmp_packet.nat_icmp_source_port = 
                    GET16(buffer, (unsigned int)(ICMP_IP_OFFSET + 
                    nat_packet->nat_icmp_packet.nat_icmp_ip_header_length + 
                    UDP_SRC_OFFSET));

                nat_packet->nat_icmp_packet.nat_icmp_dest_port = 
                    GET16(buffer, (unsigned int)(ICMP_IP_OFFSET + 
                    nat_packet->nat_icmp_packet.nat_icmp_ip_header_length + 
                    UDP_DEST_OFFSET));

                break;
#endif

            default:

                break;
            }
            break;

        default:

            break;
        }
    }

    return (NU_SUCCESS);

} /* NAT_ICMP_Parse_Packet */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Build_Outgoing_Packet
*                                                                       
*   DESCRIPTION                                                           
*            
*       This function rebuilds a packet destined for the external 
*       network.                                                           
*                                                                       
*   INPUTS                                                                
*       
*       *buf_ptr                A pointer to the original buffer of data.
*       *nat_packet             A pointer to the NAT_PACKET which contains 
*                               the translated information.
*       source_port             The source port.
*                                                                       
*   OUTPUTS                                                               
*                                                                
*       None.       
*                                                                       
****************************************************************************/
VOID NAT_Build_Outgoing_Packet(const NET_BUFFER *buf_ptr, 
                               const NAT_PACKET *nat_packet, INT32 source_port)
{
    /* Copy the external source IP address into the packet */
    PUT32(buf_ptr->data_ptr, IP_SRC_OFFSET, 
          NAT_External_Device->dev_addr.dev_ip_addr);

    if (source_port != -1)
    {
        /* Copy the external source port into the packet */
        PUT16(buf_ptr->data_ptr, 
              (unsigned int)nat_packet->nat_ip_header_length, 
              (UINT16)source_port);
    }

} /* NAT_Build_Outgoing_Packet */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Build_Incoming_Packet
*                                                                       
*   DESCRIPTION                                                           
*            
*       This function rebuilds a packet destined for the internal
*       network.                                                           
*                                                                       
*   INPUTS                                                                
*               
*       *buf_ptr                A pointer to the original buffer of data.
*       *nat_packet             A pointer to the NAT_PACKET which contains 
*                               the translated information.
*       dest_ip                 The destination IP address of the internal 
*                               node.
*       dest_port               The destination port of the internal node.
*                                                                       
*   OUTPUTS                                                               
*                                                                
*       None.       
*                                                                       
*****************************************************************************/
VOID NAT_Build_Incoming_Packet(const NET_BUFFER *buf_ptr, 
                               const NAT_PACKET *nat_packet, UINT32 dest_ip, 
                               INT32 dest_port)
{
    /* Copy the internal source IP into the packet */
    PUT32(buf_ptr->data_ptr, IP_DEST_OFFSET, dest_ip);

    if (dest_port != -1)
    {
        /* Copy the interal source port into the packet */
        PUT16(buf_ptr->data_ptr, 
              (unsigned int)(nat_packet->nat_ip_header_length + 2), 
              (UINT16)dest_port);
    }

} /* NAT_Build_Incoming_Packet */

#if (INCLUDE_TCP == NU_TRUE)
/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Update_TCP
*                                                                       
*   DESCRIPTION                                                           
*            
*       This function updates the state of a TCP session.                                                           
*                                                                       
*   INPUTS                                                                
*                                                        
*       *nat_packet             A pointer to the NAT_PACKET which contains 
*                               the translated data.
*       *entry                  A pointer to the TCP entry in the table.
*       index                   The index of the entry in the Translation 
*                               Table.
*                                                                       
*   OUTPUTS                                                               
*                               
*       None.                                        
*                                                                       
****************************************************************************/
VOID NAT_Update_TCP(const NAT_PACKET *nat_packet, NAT_TCP_ENTRY *entry, 
                    INT32 index)
{
    /* The client sent or received a reset */
    if (nat_packet->nat_control_bit & TRESET)
    {
        if (entry->nat_state != SCLOSED)
        {
            /* If a timer has already been set to delete the port, do not
             * set another timer.
             */
            if ( (entry->nat_state != SSYNS) && (entry->nat_state != SFW1) &&
                 (entry->nat_state != SCLOSING) )
                UTL_Timerset(NAT_CLOSE, (UNSIGNED)index, NAT_CLOSE_TIMEOUT, 0);

            entry->nat_state = SCLOSED;
        }
    }

    else 
    {
        switch (entry->nat_state)
        {
        case -1:

            /* The internal client has initiated a connection - start a timer 
             * to timeout after N minutes if the connection is not established.
             */
            if (nat_packet->nat_control_bit & TSYN)
            {
                entry->nat_state = SSYNS;
                UTL_Timerset(NAT_TIMEOUT, (UNSIGNED)index, NAT_CONN_TIMEOUT, 0);
            }

            break;

        case SSYNS:

            /* The session is established - delete the timer. */
            if (nat_packet->nat_control_bit & TACK)
            {                  
                entry->nat_state = SEST;
                UTL_Timerunset(NAT_TIMEOUT, (UNSIGNED)index, 0);
            }

            break;

        case SEST:

            /* A node is initiating the close of the connection - start a timer 
             * to timeout after N minutes if the connection is not closed.
             */
            if (nat_packet->nat_control_bit & TFIN)
            {
                entry->nat_state = SFW1;
                UTL_Timerset(NAT_TIMEOUT, (UNSIGNED)index, NAT_CONN_TIMEOUT, 0);
            }

            break;

        case SFW1:

            /* The other node has confirmed the close.  Delete the timer and 
             * start a new timer to delete the entry after N minutes to ensure 
             * that the connection is really closed.
             */
            if (nat_packet->nat_control_bit & TACK)
            {
                entry->nat_state = SCLOSING;
                UTL_Timerunset(NAT_TIMEOUT, (UNSIGNED)index, 0);
                UTL_Timerset(NAT_CLOSE, (UNSIGNED)index, NAT_CLOSE_TIMEOUT, 0);
            }

            break;

        default:

            break;
        }   
    }

} /* NAT_Update_TCP */
#endif

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Adjust_Checksum
*                                                                       
*   DESCRIPTION                                                           
*            
*       This function adjusts the checksum according to the changes made
*       in the respective header.
*                                                                       
*   INPUTS                                                                
*                         
*       *chksum                 A pointer to the original checksum.
*       *optr                   A pointer to the original buffer of data.
*       olen                    The length of the original buffer of data.
*       *nptr                   A pointer to the translated buffer of data.
*       nlen                    The length of the translated buffer of data.                                              
*                                                                       
*   OUTPUTS                                                               
*                                                               
*       None.        
*                                                                       
****************************************************************************/
STATUS NAT_Adjust_Checksum(UINT8 *chksum, const UINT8 *optr, INT32 olen, 
                           const UINT8 *nptr, INT32 nlen)
{
     INT32 x, old_checksum, new_checksum;

     x = chksum[0] * 256 + chksum[1];
     x = ~x & 0xFFFF;

     while (olen) 
     {
         old_checksum = optr[0] * 256 + optr[1]; 
         optr+=2;

         x -= old_checksum & 0xffff;

         if (x<=0) 
         { 
             x--; 
             x &= 0xffff; 
         }

         olen-=2;
     }

     while (nlen) 
     {
         new_checksum = nptr[0] * 256 + nptr[1]; 
         nptr += 2;

         x += new_checksum & 0xffff;

         if (x & 0x10000) 
         { 
             x++; 
             x &= 0xffff; 
         }

         nlen-=2;
     }

     x = ~x & 0xFFFF;
     chksum[0] = (UINT8)(x / 256); 
     chksum[1] = (UINT8)(x & 0xff);

     return (NU_SUCCESS);

} /* NAT_Adjust_Checksum */

#if (INCLUDE_TCP == NU_TRUE)
/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Cleanup_TCP
*                                                                       
*   DESCRIPTION                                                           
*            
*       This function deletes all entries from the TCP Translation
*       Table that have exceeded the timeout period.
*                                                                       
*   INPUTS                                                                
*                         
*       None.
*                                                                       
*   OUTPUTS                                                               
*            
*       None.                                                   
*                                                                       
****************************************************************************/
VOID NAT_Cleanup_TCP(VOID)
{
    UINT32  current_clock;
    INT32   i;

    /* Ensure that the NAT module is up and running. */
    if (IP_NAT_Initialize)
    {
        /* Get the current clock value */
        current_clock = NU_Retrieve_Clock();

        /* Traverse the list of TCP entries.  Delete the timed out entries */
        for (i = 0; i < NAT_MAX_TCP_CONNS; i++)
        { 
            if ( (NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[i].
                  nat_timeout > 0) && (NAT_TIME_DIFF(current_clock, 
                  NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[i].
                  nat_timeout) >= NAT_TCP_TIMEOUT) )
            {
                NAT_Delete_Translation_Entry(IPPROTO_TCP, i);
            }
        }   

        /* Reset the timer */
        UTL_Timerset(NAT_CLEANUP_TCP, 0, NAT_TCP_TIMEOUT, 0);
    }

} /* NAT_Cleanup_TCP */
#endif

#if (INCLUDE_UDP == NU_TRUE)
/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Cleanup_UDP
*                                                                       
*   DESCRIPTION                                                           
*            
*       This function deletes all entries from the UDP Translation
*       Table that have exceeded the timeout period.
*                                                                       
*   INPUTS                                                                
*                         
*       None.
*                                                                       
*   OUTPUTS                                                               
*            
*       None.                                                   
*                                                                       
****************************************************************************/
VOID NAT_Cleanup_UDP(VOID)
{
    UINT32  current_clock;
    INT32   i;

    /* Ensure that the NAT module is up and running. */
    if (IP_NAT_Initialize)
    {
        /* Get the current clock */
        current_clock = NU_Retrieve_Clock();

        /* Traverse the list of UDP entries.  Delete the timed out entries */
        for (i = 0; i < NAT_MAX_UDP_CONNS; i++)
        { 
            if ( (NAT_Translation_Table.nat_udp_table->nat_udp_entry[i].
                  nat_timeout > 0) && (NAT_TIME_DIFF(current_clock, 
                  NAT_Translation_Table.nat_udp_table->nat_udp_entry[i].
                  nat_timeout) >= NAT_UDP_TIMEOUT) )
                NAT_Delete_Translation_Entry(IPPROTO_UDP, i);
        }   

        /* Reset the timer */
        UTL_Timerset(NAT_CLEANUP_UDP, 0, NAT_UDP_TIMEOUT, 0);
    }

} /* NAT_Cleanup_UDP */
#endif

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Cleanup_ICMP
*                                                                       
*   DESCRIPTION                                                           
*            
*       This function deletes all entries from the ICMP Translation
*       Table that have exceeded the timeout period.
*                                                                       
*   INPUTS                                                                
*                         
*       None.
*                                                                       
*   OUTPUTS                                                               
*            
*       None.                                                   
*                                                                       
****************************************************************************/
VOID NAT_Cleanup_ICMP(VOID)
{
    UINT32  current_clock;
    INT32   i;

    /* Ensure that the NAT module is up and running. */
    if (IP_NAT_Initialize)
    {
        /* Get the current clock */
        current_clock = NU_Retrieve_Clock();

        /* Traverse the list of entries.  Delete the timed out entries */
        for (i = 0; i < NAT_MAX_ICMP_CONNS; i++)
        { 
            if ( (NAT_Translation_Table.nat_icmp_table->nat_icmp_entry[i].
                  nat_timeout > 0) && (NAT_TIME_DIFF(current_clock, 
                  NAT_Translation_Table.nat_icmp_table->nat_icmp_entry[i].
                  nat_timeout) >= NAT_ICMP_TIMEOUT) )
                NAT_Delete_ICMP_Entry(i);
        }

        /* Reset the timer */
        UTL_Timerset(NAT_CLEANUP_ICMP, 0, NAT_ICMP_TIMEOUT, 0);
    }

} /* NAT_Cleanup_ICMP */

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Find_Route                                                    
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function calls the appropriate function to find a route                                                
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *ro                     Pointer to the RTAB route information 
*                               structure
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       None.                                                            
*                                                                       
*************************************************************************/
VOID NAT_Find_Route(RTAB_ROUTE *ro)
{
#if ((defined(NET_5_1)) && (NET_VERSION_COMP >= NET_5_1))
    ro->rt_route = RTAB4_Find_Route(&ro->rt_ip_dest, 0);
#else
    ro->rt_route = RTAB_Find_Route(&ro->rt_ip_dest);
#endif

} /* NAT_Find_Route */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Reassemble_Packet
*                                                                       
*   DESCRIPTION                                                           
*            
*       This function deletes all entries from the ICMP Translation
*       Table that have exceeded the timeout period.
*                                                                       
*   INPUTS                                                                
*                         
*       **nat_packet            A pointer to the IPLAYER data structure.
*       **nat_buf_ptr           A pointer to the NET Buffer.
*                                                                       
*   OUTPUTS                                                               
*            
*       1                       The packet is fragmented, but not all of the
*                               fragments have been received yet.
*       0                       The packet is fragmented, and all of the
*                               pieces have been received.
*                                                                       
****************************************************************************/
STATUS NAT_Reassemble_Packet(IPLAYER **nat_packet, NET_BUFFER **nat_buf_ptr)
{
    IPLAYER             *pkt = *nat_packet;
#ifndef NET_5_1
    UINT16              hlen;
    IP_QUEUE_ELEMENT    *fp;
#endif
    
#if ((defined(NET_5_1)) && (NET_VERSION_COMP >= NET_5_1))

    return (IP_Reassemble_Packet(nat_packet, nat_buf_ptr, 
                                 (UINT16)((GET8(pkt,
                                 IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2)));    
#else

    /* Increment the number of IP fragments that have been received. */
    SNMP_ipReasmReqds_Inc;

    /* Search the list of fragmented packets to see if at least one fragment
     * from the same packet was previously received. 
     */
    for (fp = IP_Frag_Queue.ipq_head; fp != NU_NULL; fp = fp->ipq_next)
    {
        /* Fragments are uniquely identified by IP id, source address,
         * destination address, and protocol. 
         */
        if ( (GET16(pkt, IP_IDENT_OFFSET) == fp->ipq_id) && 
             (GET32(pkt, IP_SRC_OFFSET) == fp->ipq_source) &&
             (GET32(pkt, IP_DEST_OFFSET) == fp->ipq_dest) &&
             (GET8(pkt, IP_PROTOCOL_OFFSET) == fp->ipq_protocol) )
            break;
    }
    
    /* Adjust the IP length to not reflect the header. */
    PUT16(pkt, IP_TLEN_OFFSET, (UINT16)(GET16(pkt, IP_TLEN_OFFSET) - hlen));

    /* Set ipf_mff if more fragments are expected. */
    PUT8(pkt, IPF_MFF_OFFSET, (UINT8)(GET8(pkt, IPF_MFF_OFFSET) & ~1));

    if (GET16(pkt, IP_FRAGS_OFFSET) & IP_MF)
        PUT8(pkt, IPF_MFF_OFFSET, (UINT8)(GET8(pkt, IPF_MFF_OFFSET) | 1));

    /* Convert the offset of this fragment to bytes and shift off the 3 
     * flag bits. 
     */
    PUT16(pkt, IP_FRAGS_OFFSET, (INT16)(GET16(pkt, IP_FRAGS_OFFSET) << 3) );

    /* If this datagram is marked as having more fragments or this is not 
     * the first fragment, attempt reassembly. 
     */
    if (GET8(pkt, IPF_MFF_OFFSET) & 1 || GET16(pkt, IP_FRAGS_OFFSET) )
    {
        *nat_buf_ptr = IP_Reassembly((IP_FRAG *)pkt, fp, *nat_buf_ptr);

        if (*nat_buf_ptr == NU_NULL)
        {
            /* A complete packet could not yet be assembled, return. */
            return (1);
        }

        /* If we make it here then a framented packet has been put back 
         * together. We need to set all pointers and other local variables 
         * to match what they would normally be if this packet was simply 
         * a RX packet and not a reasembled one. 
         */

        /* Set the IP packet pointer to the IP header. */
        pkt = (IPLAYER *) (*nat_buf_ptr)->data_ptr;

        /* Get the header length. */
        hlen = (UINT16)((GET8(pkt, IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2);

        /* Adjust the IP length to reflect the header. */
        PUT16(pkt, IP_TLEN_OFFSET, (UINT16)(GET16(pkt, IP_TLEN_OFFSET) + hlen));

        *nat_packet = pkt;
    
        /* Increment the number of IP fragmented packets that have
         * successfully been reasmebled. 
         */
        SNMP_ipReasmOKs_Inc;        
    }

    else if (fp)
    {
        IP_Free_Queue_Element(fp);
        
        /* Drop this packet. */
        MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

        SNMP_ipInHdrErrors_Inc;

        return (1);
    }
    
    return (0);

#endif

} /* NAT_Reassemble_Packet */

