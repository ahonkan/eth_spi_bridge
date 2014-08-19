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
*       alg_ftp.c                                                   
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file contains those routines necessary for the FTP
*       Application Level Gateway.
*                                                           
*   DATA STRUCTURES                                                            
*
*       None.
*                                               
*   FUNCTIONS                                                                  
*              
*       ALG_FTP_Translate
*       ALG_Add_FTP_Entry
*       ALG_Find_FTP_Entry
*       ALG_Delete_FTP_Entry
*       ALG_FTP_Adjust_Sequence
*       ALG_FTP_Compute_Address
*                                             
*   DEPENDENCIES                                                               
*
*       target.h
*       ncl.h
*       externs.h
*       nat_defs.h
*       nat_extr.h
*       alg_defs.h
*       alg_extr.h
*                                                                
******************************************************************************/

#include "networking/target.h"
#include "networking/ncl.h"
#include "networking/externs.h"
#include "networking/nat_defs.h"
#include "networking/nat_extr.h"
#include "networking/alg_defs.h"
#include "networking/alg_extr.h"

#if NAT_INCLUDE_FTP_ALG

extern NAT_TRANSLATION_TABLE NAT_Translation_Table;
extern ALG_FTP_TABLE         ALG_FTP_Table;
extern DV_DEVICE_ENTRY       *NAT_External_Device;

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ALG_FTP_Translate
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function modifies the payload of an FTP PORT, EPRT or PASV
*       command according to the internal IP address and internal
*       source port.
*                                                                       
*   INPUTS                                                                
*          
*       *buf_ptr                A pointer to the buffer of data.
*       *nat_packet             A pointer to the NAT_PACKET which contains
*                               the translated data from the IP and TCP/UDP
*                               headers.
*       index                   The index into the port list for this 
*                               connection.
*       *old_data               A pointer to the original unaltered packet.
*       *internal_device        A pointer to the internal device on which 
*                               the packet was received.
*                                                                       
*   OUTPUTS                                                               
*                   
*       NU_SUCCESS              The translation was successfully performed.
*       NAT_NO_NAT              No translation is necessary.
*                                                                       
****************************************************************************/
STATUS ALG_FTP_Translate(NET_BUFFER *buf_ptr, const NAT_PACKET *nat_packet, 
                         INT32 index, UINT8 *old_data, 
                         DV_DEVICE_ENTRY *internal_device)
{
    ALG_FTP_ENTRY   *alg_ftp_entry;
    UINT8           temp_port_buffer[ALG_FTP_PORT_NUMBER_SIZE] = 
                        {0, 0, 0, 0, 0, 0, 0};
    UINT8           offset_port_command = 5;
    UINT8           offset_pasv_command = 27;    
    UINT8           offset_eprt_command = 8;
    UINT8           ftp_delimiter;    
    UINT8           alg_buffer[ALG_FTP_IP_ADDR_SIZE];
    UINT8           external_addr_length;
    UINT32          offset_command_length;
    UINT8           temp_port_size = 0;    
    UINT8           temp_port[5] = {0, 0, 0, 0, 0};
    INT8            command;
    UINT16          new_port[2];
    UINT16          source_port;
    UINT32          new_length;
    INT32           i, j;
    INT32           sequence_delta;
    INT32           index_entry, alg_ftp_index;
    UINT32          offset_old_buffer;
    UINT32          offset_new_buffer;
    STATUS          status = NU_SUCCESS;

    /* Determine if this is an FTP PORT, EPRT or PASV command.  Check if 
     * there is data present beyond the TCP header, and if so, check if 
     * that data is a PORT, EPRT or PASV command. Initialize the offset
     */    
    if ( (buf_ptr->data_len > (nat_packet->nat_ip_header_length + 
          nat_packet->nat_prot_header_length)) )
    {
        if ( (memcmp(&buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                     nat_packet->nat_prot_header_length], "PORT ", 5) == 0) )
        {
            command = ALG_FTP_PORT_COMMAND;
            ftp_delimiter = ',';

            offset_command_length = 
                nat_packet->nat_prot_header_length + offset_port_command;
        }
        
        else if ( (memcmp(&buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                          nat_packet->nat_prot_header_length], "127 ", 4) == 0) )
        {
            command = ALG_FTP_PASV_COMMAND;
            ftp_delimiter = ',';

            offset_command_length = 
                nat_packet->nat_prot_header_length + offset_pasv_command;
        }
        
        else if ( (memcmp(&buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                          nat_packet->nat_prot_header_length], "EPRT ", 5) == 0) )
        {
            command = ALG_FTP_EPRT_COMMAND;
            ftp_delimiter = '|';
            
            /* The EPRT command  has an argument <d><net-prt><d> which specifies 
             * the family. It is |1| for IPV4 and |2| for IPV6. We return 
             * NAT_NO_NAT if it is |2|.
             */         
            if ( (memcmp(&buf_ptr->data_ptr[nat_packet->nat_ip_header_length + 
                         nat_packet->nat_prot_header_length+5], "|1|", 3) == 0) )
            {
                offset_command_length = 
                    nat_packet->nat_prot_header_length + offset_eprt_command;
            }

            else
                return (NAT_NO_NAT); 
            
        }

        else
            return (NAT_NO_NAT);
    }

    else
        return (NAT_NO_NAT);
    
    /* Only modify PORT and EPRT commands and responses to PASV commands coming 
     * from the internal network.
     */
    if ( (command == ALG_FTP_PORT_COMMAND) || (command == ALG_FTP_PASV_COMMAND) ||
         (command == ALG_FTP_EPRT_COMMAND) )
    {   
        /* Initialize the buffer place holders and data length of the current 
         * packet
         */
        offset_old_buffer = 
            nat_packet->nat_ip_header_length + offset_command_length;

        offset_new_buffer = 
            nat_packet->nat_ip_header_length + offset_command_length;        
       
        /* For the EPRT command you have '|', '\r', '\n' at the end of the 
         * command 
         */            
        if (command == ALG_FTP_EPRT_COMMAND)            
        {
            buf_ptr->data_len = 
                nat_packet->nat_ip_header_length + offset_command_length + 3;
        }

        /* For the PORT and PASV commands you just have '\r' and '\n' at the end */
        else            
        {
            buf_ptr->data_len = 
                nat_packet->nat_ip_header_length + offset_command_length + 2;
        }
                
        /* Count the number of characters of the internal IP address in the 
         * packet 
         */
        for (i = 0; i <= 3; i++)
        {
            /* Count the number of characters in this portion of the IP address */
            while ( (buf_ptr->data_ptr[offset_old_buffer] != ftp_delimiter) && 
                    (buf_ptr->data_ptr[offset_old_buffer] != '.') )
                offset_old_buffer++;
            
            offset_old_buffer++;
        }        
              
        /* Get the first number in the port number */
        for (j = 0; buf_ptr->data_ptr[offset_old_buffer] != ftp_delimiter; 
             j++, offset_old_buffer++)
            temp_port[j] = buf_ptr->data_ptr[offset_old_buffer];
        
        if (command != ALG_FTP_EPRT_COMMAND)
        {
            /* Calculate the first portion of the port number */
            source_port = (UINT16)(NU_ATOI((const char*)temp_port) * 256);
            
            memset(temp_port, 0, 5);
            
            /* Increment once more for the comma */
            offset_old_buffer++;
            
            /* Get the second number in the port number */
            for (j = 0; buf_ptr->data_ptr[offset_old_buffer] != '\r'; 
                 j++, offset_old_buffer++)
                temp_port[j] = buf_ptr->data_ptr[offset_old_buffer];
            
            /* Calculate the port number */
            source_port = 
                (UINT16)(source_port + (NU_ATOI((const char*)temp_port))); 
        }

        else
            source_port = (UINT16)(NU_ATOI((const char*)temp_port));
                
        /* Add this entry with the new port number to the Address 
         * Translation Table 
         */
        index_entry = NAT_Add_Translation_Entry(IPPROTO_TCP, 
                                                nat_packet->nat_source_addr, 
                                                source_port, 
                                                nat_packet->nat_dest_addr, 
                                                20, internal_device, NU_NULL);
        if (index_entry >= 0)
        {
            
            if (command == ALG_FTP_EPRT_COMMAND)
            {
                /* Get the External Port Number */
                new_port[0] = 
                    (UINT16)(NAT_Translation_Table.nat_port_list->
                             nat_port_list[NAT_Translation_Table.nat_tcp_table->
                             nat_tcp_entry[index_entry].nat_external_port_index].
                             nat_port);

                /* Convert the number to ASCII */
                NU_ITOA((INT)new_port[0], 
                        (CHAR*)&temp_port_buffer[temp_port_size], 10);
                
                /* Determine the ASCII size */
                while ((temp_port_size < ALG_FTP_PORT_NUMBER_SIZE) &&
					   (temp_port_buffer[temp_port_size])
					  )
                {
                    temp_port_size++;
                }
                
                /* EPRT command uses "." as address delimiter - For eg. 
                 * 192.168.50.1 for IPV4
                 */
                external_addr_length = ALG_FTP_Compute_Address(alg_buffer,'.');
            }

            else
            {
                /* Calculate the ASCII equivalent of the new external port 
                 * number 
                 */
                new_port[0] = 
                    (UINT16)(NAT_Translation_Table.nat_port_list->
                             nat_port_list[NAT_Translation_Table.
                             nat_tcp_table->nat_tcp_entry[index_entry].
                             nat_external_port_index].nat_port / 256);

                new_port[1] = 
                    (UINT16)(NAT_Translation_Table.nat_port_list->
                             nat_port_list[NAT_Translation_Table.nat_tcp_table->
                             nat_tcp_entry[index_entry].nat_external_port_index].
                             nat_port % 256);
                
                /* Determine the ASCII size of the external port and two commas */
                for (i = 0; ((i <= 1) && (temp_port_size < ALG_FTP_PORT_NUMBER_SIZE)); i++)
                {
                    /* Convert the number to ASCII */
                    NU_ITOA((INT)new_port[i], 
                            (CHAR*)&temp_port_buffer[temp_port_size], 10);
                    
                    /* Determine the ASCII size */
                    while ((temp_port_size < ALG_FTP_PORT_NUMBER_SIZE) &&
						   (temp_port_buffer[temp_port_size])
						  )
                    {
                        temp_port_size++;
                    }
                    
                    /* Place a comma in the buffer and increment the size */
                    if ((i == 0) && (temp_port_size < ALG_FTP_PORT_NUMBER_SIZE))
                    {
                        temp_port_buffer[temp_port_size] = ',';
                        temp_port_size++;
                    }
                }

                /* PORT command uses "," as address delimiter Ex: 192,168,50,1 */
                external_addr_length = 
                    ALG_FTP_Compute_Address(alg_buffer, ftp_delimiter);
            }
                        
            /* Calculate the new length of the packet */
            new_length = buf_ptr->data_len + temp_port_size + external_addr_length;
            
            /* Calculate the change in length */
            sequence_delta = 
                (INT32)(new_length - buf_ptr->mem_total_data_len);
            
            /* If the entry does not exist, add it to the table.  Otherwise,
             * update the sequence number in the packet.
             */
            alg_ftp_index = ALG_Find_FTP_Entry(nat_packet->nat_device_type,
                                               nat_packet->nat_source_addr,
                                               nat_packet->nat_source_port,                                                     
                                               nat_packet->nat_dest_addr,
                                               nat_packet->nat_dest_port);
            if (alg_ftp_index == -1)
            {
                if (sequence_delta != 0)
                {
                    if (ALG_Add_FTP_Entry(index, sequence_delta) == -1)
                    {
                        /* Delete the entry made to the translation table, because
                         * the packet is not going to be sent.
                         */
                        NAT_Delete_Translation_Entry(IPPROTO_TCP, index_entry);
                        
                        status = NAT_NO_MEMORY;
                    }
                }
            }
            
            else
            {
                alg_ftp_entry = &ALG_FTP_Table.alg_ftp_entry[alg_ftp_index];
                
                /* Adjust the sequence number of this packet */
                ALG_FTP_Adjust_Sequence(buf_ptr, alg_ftp_entry->alg_side,
                                        alg_ftp_entry->alg_sequence_delta, 
                                        nat_packet->nat_ip_header_length);
                
                /* Update the sequence delta associated with the entry */
                alg_ftp_entry->alg_sequence_delta += sequence_delta;
            }   
            
            if (status == NU_SUCCESS)
            {
                /* Update the length of the data */
                buf_ptr->mem_total_data_len = new_length;

                buf_ptr->data_len = new_length;
                
                /* Copy the external IP address into the buffer */
				if (external_addr_length <= ALG_FTP_IP_ADDR_SIZE)
				{
	                memcpy(&buf_ptr->data_ptr[offset_new_buffer], alg_buffer, 
	                       external_addr_length);
	                
	                offset_new_buffer += external_addr_length;

	                /* Copy the port into the packet */
	                memcpy(&buf_ptr->data_ptr[offset_new_buffer], temp_port_buffer, 
	                       temp_port_size);
	                
	                offset_new_buffer += temp_port_size;
	                
	                if (command == ALG_FTP_EPRT_COMMAND)
	                {
	                    buf_ptr->data_ptr[offset_new_buffer] = ftp_delimiter;
	                    offset_new_buffer ++;                    
	                }
	                
	                /* Add a carriage return */
	                buf_ptr->data_ptr[offset_new_buffer] = '\r';
	                offset_new_buffer ++;
	                
	                /* Add a line feed */
	                buf_ptr->data_ptr[offset_new_buffer] = '\n';           
	                
	                /* Put the new length in the packet */
	                PUT16(buf_ptr->data_ptr, IP_TLEN_OFFSET, 
	                      (UINT16)buf_ptr->mem_total_data_len);
	                
	                /* Recalculate the IP checksum to reflect the change in the 
	                 * length of the packet.
	                 */
	                NAT_Adjust_Checksum((UINT8*)&buf_ptr->data_ptr[IP_CHECK_OFFSET],
	                                    (UINT8*)&old_data[IP_TLEN_OFFSET], 2,
	                                    &buf_ptr->data_ptr[IP_TLEN_OFFSET], 2);
				}
				
				else
				{
					status = NAT_NO_NAT;
				}				
            }
            
            status = NAT_COMPUTE_CHECKSUM;
        }

        else
            status = NAT_NO_MEMORY;
    }

    return (status);

} /* ALG_FTP_Translate */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ALG_Add_FTP_Entry
*                                                                       
*   DESCRIPTION                                                           
*           
*       This function adds an entry to the FTP ALG Table so that the
*       ALG will know to modify the sequence number of all subsequent 
*       packets on this connection.
*                                                                       
*   INPUTS                                                                
*          
*       index                   The index into the TCP Translation
*                               Table.
*       sequence_delta          The numeric value to be added to each
*                               subsequent packet received on the
*                               connection associated with the above
*                               parameters.
*                                                                       
*   OUTPUTS                                                               
*                                          
*       -1                      There are no available entries in the
*                               table.
*       index                   The index value of the new entry in
*                               the FTP Table.
*                                                                       
****************************************************************************/
INT32 ALG_Add_FTP_Entry(INT32 index, INT32 sequence_delta)
{
    INT32   next_index, next_avail_index, i;

    /* Get the index of the next available FTP entry */
    next_index = ALG_FTP_Table.alg_next_avail_ftp_entry;

    /* If the next available FTP entry index is -1, there were no entries 
     * available the last time a new entry was added.  Check if any entries
     * have come available since then.
     */
    if (next_index == -1)
    {
        for (i = 0; i < ALG_MAX_FTP_CONNS; i++)
        {
            /* If the timeout value of this entry is 0, it is available */
            if (ALG_FTP_Table.alg_ftp_entry[i].alg_timeout == 0)
            {
                /* Save off the index */
                if (next_index == -1)
                    next_index = i;

                /* Set the next available FTP entry index */
                else
                {
                    ALG_FTP_Table.alg_next_avail_ftp_entry = i;
                    break;
                }
            }
        }
    }

    /* Otherwise, set the next available FTP entry index */
    else
    {  
        /* Initially, set the next available FTP entry index to -1 */
        ALG_FTP_Table.alg_next_avail_ftp_entry = -1;

        next_avail_index = next_index + 1;

        /* Traverse the list until we find an available entry */
        for (i = 0; i < ALG_MAX_FTP_CONNS; i++, next_avail_index++)
        {
            /* If we have reached the end of the list, wrap around to the
             * beginning.
             */
            if (next_avail_index >= ALG_MAX_FTP_CONNS)
                next_avail_index = 0;

            /* If the timeout value of this entry is -1, set it as the next
             * available FTP entry.
             */
            if (ALG_FTP_Table.alg_ftp_entry[next_avail_index].alg_timeout == 0)
            {
                ALG_FTP_Table.alg_next_avail_ftp_entry = next_avail_index;
                break;
            }
        }
    }

    /* If there is an available entry, fill it in */
    if (next_index != -1)
    {
        /* Fill in the fields of the new entry */
        ALG_FTP_Table.alg_ftp_entry[next_index].alg_tcp_index = index;
        ALG_FTP_Table.alg_ftp_entry[next_index].alg_sequence_delta = sequence_delta;

        /* Set the timeout */
        ALG_FTP_Table.alg_ftp_entry[next_index].alg_timeout = NU_Retrieve_Clock();

        NAT_Translation_Table.nat_tcp_table->
            nat_tcp_entry[index].nat_ftp.nat_ftp_index = next_index;
    }

    return (next_index);

} /* ALG_Add_FTP_Entry */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ALG_Find_FTP_Entry
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function finds an entry in the ALG FTP Table matching the 
*       input parameters.
*                                                                       
*   INPUTS                                                                
*          
*       device_type             Either NAT_INTERNAL_DEVICE for a packet 
*                               coming from the internal network or 
*                               NAT_EXTERNAL_DEVICE for a packet coming from 
*                               the external network.
*       source_addr             Source address of the packet.
*       source_port             The source port of the packet.
*       dest_addr               Destination address of the packet.
*       dest_port               The destination port of the packet.
*                                                                       
*   OUTPUTS                                                               
*                                                     
*       -1                      No matching entry was found.
*       index                   The index value into the FTP Table of the 
*                               entry.
*                                                                       
****************************************************************************/
INT32 ALG_Find_FTP_Entry(UINT8 device_type, UINT32 source_addr, 
                         UINT16 source_port, UINT32 dest_addr, 
                         UINT16 dest_port)
{
    INT             i;
    INT32           index = -1, tcp_index;
    ALG_FTP_ENTRY   *alg_ftp_entry;

    /* Find the corresponding TCP entry in the Translation Table */
    tcp_index = NAT_Find_Translation_Entry(IPPROTO_TCP, device_type, 
                                           source_addr, source_port, dest_addr, 
                                           dest_port, NU_NULL);
    
    if (tcp_index >= 0)
    {
        /* Search the Address Translation Table for a match */
        for (i = 0; i < ALG_MAX_FTP_CONNS; i++)
        {
            alg_ftp_entry = &ALG_FTP_Table.alg_ftp_entry[i];

            /* If the Translation Entry matches */
            if (alg_ftp_entry->alg_tcp_index == tcp_index)
            {
                /* Set the timeout */
                alg_ftp_entry->alg_timeout = NU_Retrieve_Clock();
    
                if (NAT_Translation_Table.nat_tcp_table->
                    nat_tcp_entry[tcp_index].nat_destination_ip == dest_addr)
                    alg_ftp_entry->alg_side = ALG_CLIENT;
                else
                    alg_ftp_entry->alg_side = ALG_HOST;
    
                index = i;

                break;
            }
        }
    }      

    return (index);

} /* ALG_Find_FTP_Entry */

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ALG_Delete_FTP_Entry
*                                                                       
*   DESCRIPTION                                                           
*                             
*       This function deletes an entry from the FTP Table.    
*                                                                       
*   INPUTS                                                                
*             
*       index                   The index of the entry in the table.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       None.
*                                                                       
*************************************************************************/
VOID ALG_Delete_FTP_Entry(INT32 index)
{
    /* Delete the entry */
    UTL_Zero(&ALG_FTP_Table.alg_ftp_entry[index], sizeof(ALG_FTP_ENTRY));
    ALG_FTP_Table.alg_ftp_entry[index].alg_tcp_index = -1;

} /* ALG_Delete_FTP_Entry */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ALG_FTP_Adjust_Sequence
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function adjusts the TCP sequence number in the given packet
*       by the specified amount.
*                                                                       
*   INPUTS                                                                
*          
*       *buf_ptr                A pointer to the buffer.
*       connection_side         Either ALG_HOST or ALG_CLIENT - whether the
*                               packet came from the server or client side
*                               of the FTP connection.
*       sequence_delta          The amount by which to adjust the TCP 
*                               sequence number.
*       ip_check_offset         The length of the IP header of the packet.
*                                                                       
*   OUTPUTS                                                               
*                                                     
*       None.
*                                                                       
****************************************************************************/
VOID ALG_FTP_Adjust_Sequence(const NET_BUFFER *buf_ptr, UINT8 connection_side,
                             INT32 sequence_delta, UINT32 ip_check_offset)
{
    UINT32  original;

    /* If this is a host packet, the new sequence number is the 
     * old sequence number - the sequence delta.
     */
    if (connection_side == ALG_HOST)
    {
        memcpy(&original, &buf_ptr->data_ptr[ip_check_offset + TCP_ACK_OFFSET], 4);

        PUT32(buf_ptr->data_ptr, (unsigned int)(ip_check_offset + TCP_ACK_OFFSET), 
              (GET32(buf_ptr->data_ptr, (unsigned int)(ip_check_offset + TCP_ACK_OFFSET)) - 
                     sequence_delta));

        /* Adjust the checksum to reflect the change in sequence number */
        NAT_Adjust_Checksum(&buf_ptr->data_ptr[ip_check_offset + TCP_CHECK_OFFSET],
                            (UINT8*)&original, 4, 
                            &buf_ptr->data_ptr[ip_check_offset + TCP_ACK_OFFSET], 4);
    }

    /* If this is a client packet, the new sequence number is the 
     * old sequence number + the sequence delta 
     */
    else
    {
        memcpy(&original, &buf_ptr->data_ptr[ip_check_offset + TCP_SEQ_OFFSET], 4);

        PUT32(buf_ptr->data_ptr, (unsigned int)(ip_check_offset + TCP_SEQ_OFFSET), 
              (GET32(buf_ptr->data_ptr, (unsigned int)(ip_check_offset + TCP_SEQ_OFFSET)) + 
                     sequence_delta));

        /* Adjust the checksum to reflect the change in sequence number */
        NAT_Adjust_Checksum(&buf_ptr->data_ptr[ip_check_offset + TCP_CHECK_OFFSET],
                            (UINT8*)&original, 4, 
                            &buf_ptr->data_ptr[ip_check_offset + TCP_SEQ_OFFSET], 4);
    }

} /* ALG_FTP_Adjust_Sequence */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ALG_FTP_Compute_Address
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function places the ASCII equivalent of the external IP
*       address in the buffer provided.
*                                                                       
*   INPUTS                                                                
*          
*       *alg_buffer             A pointer to the buffer into which to place 
*                               the ASCII equivalent of the IP address.
*       ipaddr_delimiter        PORT and PASV commands use "," as the 
*                               address delimiter whereas EPRT uses "." 
*                                                                       
*   OUTPUTS                                                               
*                                                     
*       The length of the IP address.
*                                                                       
*************************************************************************/
UINT8 ALG_FTP_Compute_Address(UINT8 *alg_buffer, UINT8 ipaddr_delimiter)
{
    UINT8   ip_addr[4];
    UINT8   i, alg_external_addr_size = 0;

	/* Initialize ip_addr to remove KW warning. */
	memset(ip_addr, 0, IP_ADDR_LEN);
	
    PUT32(ip_addr, 0, NAT_External_Device->dev_addr.dev_ip_addr);

    /* Determine the ASCII size of the external IP address and three 
     * commas and store the ASCII representation of the external IP address 
     * in the buffer in the FTP Table.
     */
    for (i = 0;
    	 (i <= 3) && (alg_external_addr_size < ALG_FTP_IP_ADDR_SIZE);
    	 i++
    	)
    {
        /* Convert the number to ASCII */
        NU_ITOA((INT)ip_addr[i], 
                (CHAR*)&alg_buffer[alg_external_addr_size], 10);

        /* Determine the ASCII size */
        while ((alg_external_addr_size < ALG_FTP_IP_ADDR_SIZE) &&
        	   (alg_buffer[alg_external_addr_size])
        	  )
            alg_external_addr_size++;

        if ( (i == 3) && (ipaddr_delimiter == '.') &&
        	 (alg_external_addr_size < ALG_FTP_IP_ADDR_SIZE)
           )
        {
            /* Place a "|" in the buffer and increment the size */
            alg_buffer[alg_external_addr_size] = '|';
            alg_external_addr_size++;
        }
        else if (alg_external_addr_size < ALG_FTP_IP_ADDR_SIZE)
        {
            /* Place a comma in the buffer and increment the size */
            alg_buffer[alg_external_addr_size] = ipaddr_delimiter;
            alg_external_addr_size++;
        }
    }

    return (alg_external_addr_size);

} /* ALG_FTP_Compute_Address */

#endif
