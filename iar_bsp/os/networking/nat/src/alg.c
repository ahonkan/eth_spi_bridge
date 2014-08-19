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
*       alg.c                                                       
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file contains those routines necessary for the supported
*       Application Level Gateways.
*                                                           
*   DATA STRUCTURES                                                            
*
*       None.
*                                               
*   FUNCTIONS                                                                  
*              
*       ALG_Init
*       ALG_Modify_Payload
*                                             
*   DEPENDENCIES                                                               
*
*       target.h
*       ncl.h
*       externs.h
*       nat_defs.h
*       nat_extr.h
*       alg_extr.h
*                                                                
******************************************************************************/

#include "networking/target.h"
#include "networking/ncl.h"
#include "networking/externs.h"
#include "networking/nat_defs.h"
#include "networking/nat_extr.h"
#include "networking/alg_extr.h"

extern ALG_FTP_TABLE         ALG_FTP_Table;

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ALG_Init
*                                                                       
*   DESCRIPTION                                                           
*           
*       This function initializes the FTP ALG Table data structure.
*      
*   INPUTS                                                                
*          
*       None
*                                                                       
*   OUTPUTS                                                               
*                     
*       None.                                
*                                                                       
*************************************************************************/
VOID ALG_Init(VOID)
{
    INT32   i;

    /* Initialize the members of the FTP Table */
    for (i = 0; i < ALG_MAX_FTP_CONNS; i++)
    {
        UTL_Zero(&ALG_FTP_Table.alg_ftp_entry[i], sizeof(ALG_FTP_ENTRY));
        ALG_FTP_Table.alg_ftp_entry[i].alg_tcp_index = -1;
    }

    ALG_FTP_Table.alg_next_avail_ftp_entry = 0;

} /* ALG_Init */

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ALG_Modify_Payload
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function determines whether the packet needs payload 
*       modifications, and calls the appropriate function to make the 
*       modifications.
*                                                                       
*   INPUTS                                                                
*          
*       *buf_ptr                A pointer to the buffer of data.
*       *nat_packet             A pointer to the NAT_PACKET which contains 
*                               the translated data from the IP and TCP/UDP 
*                               headers.
*       index                   The index into the port list for this 
*                               connection.
*       *old_data               A pointer to the unaltered original packet.
*       *internal_device        Pointer to the device structure
*                                                                       
*   OUTPUTS                                                               
*                     
*       NU_SUCCESS              The translation was successfully 
*                               completed, or no translation was
*                               necessary.
*       NAT_COMPUTE_CHECKSUM    NAT must recompute the entire TCP
*                               checksum over the FTP PORT or PASV
*                               packet.
*                                                                       
*************************************************************************/
STATUS ALG_Modify_Payload(NET_BUFFER *buf_ptr, NAT_PACKET *nat_packet, 
                          INT32 index, UINT8 *old_data, 
                          DV_DEVICE_ENTRY *internal_device)
{
    ALG_FTP_ENTRY   *alg_ftp_entry;
    INT32           port, alg_ftp_index;
    STATUS          status;

    switch (nat_packet->nat_protocol)
    {
#if NAT_INCLUDE_FTP_ALG

    case IPPROTO_TCP:

        /* If the source or destination port is 20 or 21, do the FTP 
         * translation 
         */
        if (nat_packet->nat_device_type == NAT_INTERNAL_DEVICE)
            port = nat_packet->nat_dest_port;
        else
            port = nat_packet->nat_source_port;

        switch (port)
        {
        case 20:
        case 21:

            status = NAT_NO_NAT;

            if ( (port == 21) && (nat_packet->nat_device_type == NAT_INTERNAL_DEVICE) )
                status = ALG_FTP_Translate(buf_ptr, nat_packet, index, 
                                           old_data, internal_device);

            /* Adjust the sequence or acknowledgement number */
            if (status == NAT_NO_NAT)
            {
                /* If no translation was necessary, but this is an FTP packet,
                 * check if the sequence number needs to be updated.
                 */
                alg_ftp_index = ALG_Find_FTP_Entry(nat_packet->nat_device_type,
                                                   nat_packet->nat_source_addr, 
                                                   nat_packet->nat_source_port,
                                                   nat_packet->nat_dest_addr,
                                                   nat_packet->nat_dest_port);
                if (alg_ftp_index != -1)
                {
                    alg_ftp_entry = &ALG_FTP_Table.alg_ftp_entry[alg_ftp_index];

                    /* Adjust the sequence number */
                    ALG_FTP_Adjust_Sequence(buf_ptr, alg_ftp_entry->alg_side,
                                            alg_ftp_entry->alg_sequence_delta, 
                                            nat_packet->nat_ip_header_length);
                }               

                status = NU_SUCCESS;
            }

            break;

        default:

            status = NU_SUCCESS;
        }

        break;
#endif

#if NAT_INCLUDE_ICMP_ALG

    case IPPROTO_ICMP:

        status = ALG_ICMP_Translate(buf_ptr, nat_packet, old_data);    
        break;
#endif

    default:

        status = NU_SUCCESS;
    }

    return (status);

} /* ALG_Modify_Payload */
