/****************************************************************************
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
*       alg_icmp.c                                                  
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file contains those routines necessary for the ICMP
*       Application Level Gateways.
*                                                           
*   DATA STRUCTURES                                                            
*
*       None.
*                                               
*   FUNCTIONS                                                                  
*              
*       ALG_ICMP_Translate
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

extern NAT_TRANSLATION_TABLE    NAT_Translation_Table;
extern DV_DEVICE_ENTRY          *NAT_External_Device;

#if NAT_INCLUDE_ICMP_ALG

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ALG_ICMP_Translate
*                                                                       
*   DESCRIPTION                                                           
*            
*       This function modifies the payload of ICMP Destination 
*       Unreachable, Source Quench, Time Exceeded and Parameter
*       Problem error messages.
*                                                                       
*   INPUTS                                                                
*          
*       *buf_ptr                A pointer to the buffer of data.
*       *nat_packet             A pointer to the NAT_PACKET which contains 
*                               the translated data from the IP and TCP/UDP 
*                               headers.
*       *old_data               A pointer to the original, unaltered packet.
*                                                                       
*   OUTPUTS                                                               
*                  
*       NU_SUCCESS              The translation was successful or the packet 
*                               did not require translating.
*                                                                       
*****************************************************************************/
STATUS ALG_ICMP_Translate(const NET_BUFFER *buf_ptr, const NAT_PACKET *nat_packet,
                          UINT8 *old_data)
{
    INT32           entry_index;
    STATUS          status = NU_SUCCESS;
#if INCLUDE_TCP
    NAT_TCP_ENTRY   *tcp_entry = NU_NULL;
#endif
#if INCLUDE_UDP
    NAT_UDP_ENTRY   *udp_entry = NU_NULL;
#endif

    /* Only Destination Unreachable, Source Quench, Time Exceeded and 
     * Parameter Problem messages need to be modified.
     */
    if ( (nat_packet->nat_icmp_packet.nat_icmp_type & NAT_ICMP_UNREACH) || 
         (nat_packet->nat_icmp_packet.nat_icmp_type & NAT_ICMP_SOURCEQUENCH) ||
         (nat_packet->nat_icmp_packet.nat_icmp_type & NAT_ICMP_TIMXCEED) || 
         (nat_packet->nat_icmp_packet.nat_icmp_type & NAT_ICMP_PARAPROB) )
    {
        /* Find a matching entry in the Address Translation Table */
        entry_index = NAT_Find_Translation_Entry(nat_packet->nat_icmp_packet.nat_icmp_proto, 
                                                 nat_packet->nat_device_type, 
                                                 nat_packet->nat_icmp_packet.nat_icmp_dest_addr, 
                                                 nat_packet->nat_icmp_packet.nat_icmp_dest_port, 
                                                 nat_packet->nat_icmp_packet.nat_icmp_source_addr, 
                                                 nat_packet->nat_icmp_packet.nat_icmp_source_port,
                                                 NU_NULL);

        /* Get a pointer to the corresponding entry */
        if (entry_index != -1)
        {
            switch (nat_packet->nat_icmp_packet.nat_icmp_proto)
            {
#if INCLUDE_TCP
            case IPPROTO_TCP:

                tcp_entry = &NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[entry_index];
                break;
#endif

#if INCLUDE_UDP              
            case IPPROTO_UDP:

                udp_entry = &NAT_Translation_Table.nat_udp_table->nat_udp_entry[entry_index];
                break;
#endif

            default:

                break;
            }

            /* Modify the IP address and port number of the embedded packet */
            if (nat_packet->nat_device_type == NAT_EXTERNAL_DEVICE)
            {
                switch (nat_packet->nat_icmp_packet.nat_icmp_proto)
                {
#if INCLUDE_TCP
                case IPPROTO_TCP:

                    /* Put the internal IP address in the IP header embedded in the
                     * ICMP packet.
                     */
                    PUT32(buf_ptr->data_ptr, 
                          (unsigned int)(nat_packet->nat_ip_header_length + 
                          ICMP_IP_OFFSET + IP_SRC_OFFSET), NAT_Translation_Table.nat_tcp_table->
                          nat_tcp_entry[entry_index].nat_internal_source_ip);

                    /* Put the internal source port in the TCP header embedded in
                     * the ICMP packet.
                     */
                    PUT16(buf_ptr->data_ptr, 
                          (unsigned int)(nat_packet->nat_ip_header_length + 
                          ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                          nat_icmp_ip_header_length + TCP_SRC_OFFSET),
                          NAT_Translation_Table.nat_tcp_table->
                          nat_tcp_entry[entry_index].nat_internal_source_port);

                    break;
#endif

#if INCLUDE_UDP
                case IPPROTO_UDP:

                    /* Put the internal IP address in the IP header embedded in the
                     * ICMP packet.
                     */
                    PUT32(buf_ptr->data_ptr,
                          (unsigned int)(nat_packet->nat_ip_header_length + 
                          ICMP_IP_OFFSET + IP_SRC_OFFSET), NAT_Translation_Table.nat_udp_table->
                          nat_udp_entry[entry_index].nat_internal_source_ip);

                    /* Put the internal source port in the TCP header embedded in
                     * the ICMP packet.
                     */
                    PUT16(buf_ptr->data_ptr, (unsigned int)(nat_packet->nat_ip_header_length + 
                          ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.nat_icmp_ip_header_length +
                          UDP_SRC_OFFSET), NAT_Translation_Table.nat_udp_table->
                          nat_udp_entry[entry_index].nat_internal_source_port);

                    break;
#endif

                default:

                    break;
                }
            }

            /* This is an ICMP packet from the internal network to the external */
            else
            {
                /* Put the external IP address in the IP header embedded in the
                 * ICMP packet.
                 */
                PUT32(buf_ptr->data_ptr,
                      (unsigned int)(nat_packet->nat_ip_header_length + ICMP_IP_OFFSET + 
                      IP_DEST_OFFSET), NAT_External_Device->dev_addr.dev_ip_addr);

                switch (nat_packet->nat_icmp_packet.nat_icmp_proto)
                {
#if INCLUDE_TCP
                case IPPROTO_TCP:

                    /* Put the external source port in the TCP header embedded in
                     * the ICMP packet.
                     */
                    if (tcp_entry->nat_portmap_entry == NU_NULL)
                        PUT16(buf_ptr->data_ptr, (unsigned int)(nat_packet->nat_ip_header_length 
                              + ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                              nat_icmp_ip_header_length + TCP_DEST_OFFSET), 
                              NAT_Translation_Table.nat_port_list->
                              nat_port_list[tcp_entry->nat_external_port_index].nat_port);
                    else
                        PUT16(buf_ptr->data_ptr, (unsigned int)(nat_packet->nat_ip_header_length
                              + ICMP_IP_OFFSET + nat_packet->
                              nat_icmp_packet.nat_icmp_ip_header_length + TCP_DEST_OFFSET),
                              tcp_entry->nat_portmap_entry->nat_external_source_port);
    
                    break;
#endif

#if INCLUDE_UDP
                case IPPROTO_UDP:

                    /* Put the external source port in the UDP header embedded in
                     * the ICMP packet.
                     */
                    if (udp_entry->nat_portmap_entry == NU_NULL)
                        PUT16(buf_ptr->data_ptr, (unsigned int)(nat_packet->
                              nat_ip_header_length + ICMP_IP_OFFSET + nat_packet->
                              nat_icmp_packet.nat_icmp_ip_header_length + 
                              UDP_DEST_OFFSET), NAT_Translation_Table.nat_port_list->
                              nat_port_list[udp_entry->nat_external_port_index].nat_port);
                    else
                        PUT16(buf_ptr->data_ptr, (unsigned int)(nat_packet->
                              nat_ip_header_length + ICMP_IP_OFFSET + nat_packet->
                              nat_icmp_packet.nat_icmp_ip_header_length + 
                              UDP_DEST_OFFSET), udp_entry->nat_portmap_entry->
                              nat_external_source_port);
                    
                    break;
#endif

                default:

                    break;
                }
            }

            /* Adjust the IP checksum of the original message */
            if (nat_packet->nat_device_type == NAT_EXTERNAL_DEVICE)
            {
                NAT_Adjust_Checksum((UINT8*)&buf_ptr->
                                    data_ptr[nat_packet->nat_ip_header_length + ICMP_IP_OFFSET 
                                    + IP_CHECK_OFFSET], (UINT8*)&old_data[nat_packet->
                                    nat_ip_header_length + ICMP_IP_OFFSET + IP_SRC_OFFSET], 
                                    4, &buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                    ICMP_IP_OFFSET + IP_SRC_OFFSET], 4);
            }
            else
            {
                NAT_Adjust_Checksum((UINT8*)&buf_ptr->
                                    data_ptr[nat_packet->nat_ip_header_length + ICMP_IP_OFFSET 
                                    + IP_CHECK_OFFSET], (UINT8*)&old_data[nat_packet->
                                    nat_ip_header_length + ICMP_IP_OFFSET + IP_DEST_OFFSET], 
                                    4, &buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                    ICMP_IP_OFFSET + IP_DEST_OFFSET], 4);
            }

#if INCLUDE_UDP
            /* Adjust the protocol checksum of the original message */
            if (nat_packet->nat_icmp_packet.nat_icmp_proto == IPPROTO_UDP)
            {
                if (nat_packet->nat_device_type == NAT_EXTERNAL_DEVICE)
                {
                    NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                        nat_ip_header_length + ICMP_IP_OFFSET + nat_packet->
                                        nat_icmp_packet.nat_icmp_ip_header_length + UDP_CHECK_OFFSET], 
                                        (UINT8*)&old_data[nat_packet->nat_ip_header_length + 
                                        ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                                        nat_icmp_ip_header_length + UDP_SRC_OFFSET], 2, 
                                        &buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                        ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                                        nat_icmp_ip_header_length + UDP_SRC_OFFSET], 2);

                    NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                        nat_ip_header_length + ICMP_IP_OFFSET + nat_packet->
                                        nat_icmp_packet.nat_icmp_ip_header_length + UDP_CHECK_OFFSET], 
                                        (UINT8*)&old_data[nat_packet->nat_ip_header_length + 
                                        ICMP_IP_OFFSET + IP_SRC_OFFSET], 4, &buf_ptr->
                                        data_ptr[nat_packet->nat_ip_header_length + 
                                        ICMP_IP_OFFSET + IP_SRC_OFFSET], 4);
                }

                else
                {
                    NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                        nat_ip_header_length + ICMP_IP_OFFSET + nat_packet->
                                        nat_icmp_packet.nat_icmp_ip_header_length +
                                        UDP_CHECK_OFFSET], (UINT8*)&old_data[nat_packet->
                                        nat_ip_header_length + ICMP_IP_OFFSET + nat_packet->
                                        nat_icmp_packet.nat_icmp_ip_header_length + 
                                        UDP_DEST_OFFSET], 2, &buf_ptr->data_ptr[nat_packet->
                                        nat_ip_header_length + ICMP_IP_OFFSET + 
                                        nat_packet->nat_icmp_packet.nat_icmp_ip_header_length + 
                                        UDP_DEST_OFFSET], 2);

                    NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                        nat_ip_header_length + ICMP_IP_OFFSET + nat_packet->
                                        nat_icmp_packet.nat_icmp_ip_header_length +
                                        UDP_CHECK_OFFSET], (UINT8*)&old_data[nat_packet->
                                        nat_ip_header_length + ICMP_IP_OFFSET + IP_DEST_OFFSET], 
                                        4, &buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                        ICMP_IP_OFFSET + IP_DEST_OFFSET], 4);
                }

            }
#endif

            /* Adjust the ICMP checksum over the IP header checksum, the change
             * in IP src/dest address, the change in TCP/UDP src/dest port, and
             * the UDP checksum.
             */
            NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                ICMP_CKSUM_OFFSET],
                                (UINT8*)&old_data[nat_packet->nat_ip_header_length + 
                                ICMP_IP_OFFSET + IP_CHECK_OFFSET],
                                2, &buf_ptr->data_ptr[nat_packet->nat_ip_header_length +
                                ICMP_IP_OFFSET + IP_CHECK_OFFSET], 2);

            if (nat_packet->nat_device_type == NAT_INTERNAL_DEVICE)
                NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                    ICMP_CKSUM_OFFSET],
                                    (UINT8*)&old_data[nat_packet->nat_ip_header_length + 
                                    ICMP_IP_OFFSET + IP_DEST_OFFSET],
                                    4, &buf_ptr->data_ptr[nat_packet->nat_ip_header_length +
                                    ICMP_IP_OFFSET + IP_DEST_OFFSET], 4);
            else
                NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                    ICMP_CKSUM_OFFSET],
                                    (UINT8*)&old_data[nat_packet->nat_ip_header_length + 
                                    ICMP_IP_OFFSET + IP_SRC_OFFSET],
                                    4, &buf_ptr->data_ptr[nat_packet->nat_ip_header_length +
                                    ICMP_IP_OFFSET + IP_SRC_OFFSET], 4);

            /* Adjust the ICMP checksum over the protocol header changes */
            switch (nat_packet->nat_icmp_packet.nat_icmp_proto)
            {
#if INCLUDE_TCP
            case IPPROTO_TCP:

                /* Adjust the ICMP checksum over the TCP port number */
                if (nat_packet->nat_device_type == NAT_EXTERNAL_DEVICE)
                {                    
                    NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                        nat_ip_header_length + ICMP_CKSUM_OFFSET], 
                                        (UINT8*)&old_data[nat_packet->nat_ip_header_length + 
                                        ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                                        nat_icmp_ip_header_length + TCP_SRC_OFFSET], 2,
                                        &buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                        ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                                        nat_icmp_ip_header_length + TCP_SRC_OFFSET], 2);
                }
                else
                {
                    NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                        nat_ip_header_length + ICMP_CKSUM_OFFSET], 
                                        (UINT8*)&old_data[nat_packet->nat_ip_header_length + 
                                        ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                                        nat_icmp_ip_header_length + TCP_DEST_OFFSET], 2,
                                        &buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                        ICMP_IP_OFFSET + 
                                        nat_packet->nat_icmp_packet.nat_icmp_ip_header_length + 
                                        TCP_DEST_OFFSET], 2);
                }
                
                break;
#endif

#if INCLUDE_UDP                              
            case IPPROTO_UDP:

                /* Adjust the ICMP checksum over the UDP port number */
                if (nat_packet->nat_device_type == NAT_EXTERNAL_DEVICE)
                {                    
                    NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                        nat_ip_header_length + ICMP_CKSUM_OFFSET], 
                                        (UINT8*)&old_data[nat_packet->nat_ip_header_length + 
                                        ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                                        nat_icmp_ip_header_length + UDP_SRC_OFFSET], 2,
                                        &buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                        ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                                        nat_icmp_ip_header_length + UDP_SRC_OFFSET], 2);
                }
                else
                {
                    NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                        nat_ip_header_length + ICMP_CKSUM_OFFSET], 
                                        (UINT8*)&old_data[nat_packet->nat_ip_header_length +
                                        ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                                        nat_icmp_ip_header_length + UDP_DEST_OFFSET], 2,
                                        &buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                        ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                                        nat_icmp_ip_header_length + UDP_DEST_OFFSET], 2);
                }
                
                /* Adjust the ICMP checksum over the UDP checksum */
                NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[nat_packet->
                                    nat_ip_header_length + ICMP_CKSUM_OFFSET], 
                                    (UINT8*)&old_data[nat_packet->nat_ip_header_length + 
                                    ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                                    nat_icmp_ip_header_length + UDP_CHECK_OFFSET], 2,
                                    &buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                                    ICMP_IP_OFFSET + nat_packet->nat_icmp_packet.
                                    nat_icmp_ip_header_length + UDP_CHECK_OFFSET], 2);
                break;
#endif

            default:

                break;
            }
        }
    }

    return (status);

} /* ALG_ICMP_Translate */

#endif
