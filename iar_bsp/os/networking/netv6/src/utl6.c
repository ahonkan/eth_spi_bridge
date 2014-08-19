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
*       utl6.c                                       
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       IPv6 Session interface routines.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*           
*       UTL6_Checksum
*       UTL6_Strip_Extension_Headers
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       externs.h
*       externs6.h
*                                                                       
*************************************************************************/

#include "networking/externs.h"
#include "networking/externs6.h"

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       UTL6_Checksum                                                     
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Calculate a Protocol-Specific checksum for an IPv6 packet.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *buf_ptr                A pointer to the buffer on which to 
*                               compute the checksum.
*       *source                 A pointer to the Source Address of the 
*                               packet.
*       *dest                   A pointer to the Destination Address of 
*                               the packet.    
*       length                  The length of the packet.
*       next_header             The Next-Header of the packet.
*       protocol                The protocol of the packet.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       The 16-bit checksum.
*                                                                       
*************************************************************************/
UINT16 UTL6_Checksum(NET_BUFFER *buf_ptr, const UINT8 *source, 
                     const UINT8 *dest, UINT32 length, UINT8 next_hdr, 
                     UINT8 protocol)
{
    struct pseudohdr    ip_header;
    UINT16              checksum = 0;

    /* Set up the pseudohdr data structure */
    memcpy(ip_header.dest, dest, IP6_ADDR_LEN);
    memcpy(ip_header.source, source, IP6_ADDR_LEN);
    ip_header.length = LONGSWAP(length);
    ip_header.next = next_hdr;
    ip_header.zero = 0;

    switch (protocol)
    {
    case IP_UDP_PROT:
    case IP_TCP_PROT:
    case IPPROTO_ICMPV6:
    case IPPROTO_OSPF:
    case IPPROTO_RAW:
    case IPPROTO_HELLO:

        /* Compute the checksum */
        checksum = TLS6_Prot_Check((UINT16*)&ip_header, buf_ptr);
        break;

    default:

        break;
    }

    return (checksum);

} /* UTL6_Checksum */

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       UTL6_Strip_Extension_Headers                                                     
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function removes all extension headers from the packet and
*       returns the protocol type.  Upon entering the function, 
*       buf_ptr->data_ptr is pointing to the generic portion of the IP
*       header.  After successful completion of the function, 
*       buf_ptr->data_ptr is pointing to the protocol header.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *buf_ptr                A pointer to the buffer.
*       *header_len             The length of the headers that were 
*                               removed.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       The protocol type of the packet.
*       -1 if a recognized protocol type is not found in the packet.
*                                                                       
*************************************************************************/
UINT8 UTL6_Strip_Extension_Headers(NET_BUFFER *buf_ptr, UINT16 *header_len)
{
    INT16   protocol = -1;

    /* Check if there are any extension headers present */
    if (!(IP6_IS_NXTHDR_RECPROT(buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET])))
    {
        /* Increment the data pointer to point to the protocol header */
        buf_ptr->data_ptr += IP6_HEADER_LEN;

        /* While we have not found a valid protocol header and the total 
         * number of bytes processed has not exceeded the length of the
         * packet.  This second check is included to insure that we will 
         * break out of the loop if this packet does not contain a protocol
         * that we will process.
         */
        while ( (protocol == -1) && (*header_len <= buf_ptr->data_len) )
        {
            /* If the next_header value is not another extension header,
             * set protocol to the next header value.
             */
            if (IP6_IS_NXTHDR_RECPROT(buf_ptr->data_ptr[IP6_EXTHDR_NEXTHDR_OFFSET]))
                protocol = buf_ptr->data_ptr[IP6_EXTHDR_NEXTHDR_OFFSET];

            /* Increment the header length */
            *header_len = (UINT16)(*header_len +
                (8 + (buf_ptr->data_ptr[IP6_EXTHDR_LENGTH_OFFSET] << 3)));

            /* Increment the data pointer to point to the protocol header */
            buf_ptr->data_ptr += 
                (8 + (buf_ptr->data_ptr[IP6_EXTHDR_LENGTH_OFFSET] << 3));
        }
    }

    /* There are no extension headers in the packet.  Increment the data pointer
     * to the protocol header and set the type of the protocol to return.
     */
    else
    {
        protocol = buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET];        
        buf_ptr->data_ptr += *header_len;
    }

    buf_ptr->mem_total_data_len -= *header_len;
    buf_ptr->data_len           -= *header_len;

    /* Return the protocol type or -1 */
    return ((UINT8)protocol);

} /* UTL6_Strip_Extension_Headers */
