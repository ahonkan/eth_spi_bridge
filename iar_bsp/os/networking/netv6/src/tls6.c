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
*       tls6.c                                       
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This file contains misc. "tools" used by the IPv6 networking stack. 
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*           
*       TLS6_Prot_Check
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
*       TLS6_Prot_Check                                                   
*                                                                      
*   DESCRIPTION                                                         
*                                                                      
*       This function checksums an ICMP header.                           
*                                                                      
*   INPUTS                                                              
*                                                                      
*       *pseudoheader           Pointer to header to be checksummed.
*       *buf_ptr                Pointer to the NET Buffer chain.                                         
*                                                                      
*   OUTPUTS                                                             
*                                                                      
*       UINT16                  Checksum value of structure            
*                                                                      
*************************************************************************/
UINT16 TLS6_Prot_Check(UINT16 *pseudoheader, NET_BUFFER *buf_ptr)
{
    register UINT32 sum = 0;
    register UINT16 *pshdr = pseudoheader;
    UINT16 HUGE     *current_byte;
    NET_BUFFER      *temp_buf_ptr;
    UINT32          data_len;
    UINT32          remainder;
    UINT32          i;

    /* The header length is always 20 16-bit words. */
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr++;
    sum += *pshdr;

    /* Get a pointer to the buffer. */
    temp_buf_ptr = buf_ptr;

    /* Loop through the chain computing the checksum on each byte
     * of data. 
     */
    while (temp_buf_ptr)
    {
        /* Reset the data pointer. */
        current_byte = (UINT16*)temp_buf_ptr->data_ptr;

        /* The checksum is performed on 16 bits at a time. Half the data 
         * length. 
         */
        data_len = temp_buf_ptr->data_len >> 1;

        /* Is there an odd number of bytes? */
        remainder = temp_buf_ptr->data_len & 0x1;

        for (i = 0; i < data_len; i++)
            sum += current_byte[i];

        if (remainder)
            sum += current_byte[data_len] & INTSWAP(0xFF00);

        /* Point to the next buffer. */
        temp_buf_ptr = temp_buf_ptr->next_buffer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return (INTSWAP((UINT16)~sum));

} /* TLS6_Prot_Check */
