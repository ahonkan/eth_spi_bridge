/*************************************************************************
*
*               Copyright 2002 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
****************************************************************************
* FILE NAME                                         
*
*       ppeu.c                                    
*
* COMPONENT
*
*       PPEU - PPPoE Driver Utility Functions
*
* DESCRIPTION
*
*       Supporting utility functions for some of the common tasks.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       PPEU_Add_Header
*       PPEU_Adjust_Header
*       PPEU_Append_Tag
*       PPEU_Get_Tag
*       PPEU_New_Buffer
*
* DEPENDENCIES
*       
*
***************************************************************************/
#include "drivers/ppe_defs.h"
#include "drivers/ppe_extr.h"
#include "networking/tcp.h"



/************************************************************************
* FUNCTION
*
*       PPEU_Add_Header
*
* DESCRIPTION
*
*       Add the PPPoE header information to a packet based on the
*       given information.
*
* INPUTS
*
*       *buffer             - Pointer to the Net buffer.
*       ppe_type            - The PPPoE packet type.
*       sid                 - A 16 bit number used as an identifier of
*                              a virtual device.
*
* OUTPUTS
*
*       None.
*
************************************************************************/
VOID PPEU_Add_Header(NET_BUFFER *buffer, UINT8 ppe_type, UINT16 sid)
{
    /* Move the buffer data pointer back. */
    buffer->data_ptr -= PPE_HEADER_SIZE;

    /* Write the PPPoE version and type fields. */
    PUT8(buffer->data_ptr, 0, PPE_VERTYPE);

    /* Write the packet code. */
    PUT8(buffer->data_ptr, 1, ppe_type);

    /* Write the session ID if this is a session packet. */
    PUT16(buffer->data_ptr, 2, sid);

    /* Finally add the packet length. */
    PUT16(buffer->data_ptr, 4, (UINT16)buffer->mem_total_data_len);

#if (NU_DEBUG_PPE == NU_TRUE)
    /* Truncate a packet to the PPPoE max length, if it total
       data length is too large, or is incorrect. */
    if (buffer->mem_total_data_len > 1494)
       buffer->mem_total_data_len = 1494;
#endif

    /* Now adjust the buffer length pointers. */
    buffer->data_len           += PPE_HEADER_SIZE;
    buffer->mem_total_data_len += PPE_HEADER_SIZE;

} /* PPEU_Add_Header */



/************************************************************************
* FUNCTION
*
*       PPEU_Adjust_Header
*
* DESCRIPTION
*
*       A generic routine to adjust the position of the buffer's
*       data pointer, used when adding or removing the references to
*       packet header information. A negative value will add room for
*       the header, while a positive value will remove it.
*
* INPUTS
*
*       *buffer             - Pointer to the Net buffer.
*       offset              - The offset from the buffer's data_ptr.
*
* OUTPUTS
*
*       None
*
************************************************************************/
VOID PPEU_Adjust_Header(NET_BUFFER* buffer, INT16 offset)
{
    /* Adjust data pointers to remove a header. */
    buffer->data_ptr                += offset;
    buffer->data_len                -= offset;
    buffer->mem_total_data_len      -= offset;

} /* PPEU_Adjust_Header */



/************************************************************************
* FUNCTION
*
*       PPEU_Append_Tag
*
* DESCRIPTION
*
*       Write a tag at the end of the given Net buffer.
*
* INPUTS
*
*       *buffer             - Pointer to the Net buffer.
*       type                - The type of tag to append.
*       *string             - Pointer to the string that will be appended.
*
* OUTPUTS
*
*       UINT16              - The length of the tag, including header.
*
************************************************************************/
UINT16 PPEU_Append_Tag(NET_BUFFER *buffer, UINT16 type, UINT16 len, 
                       UINT8 HUGE *string)
{
    /* Check to see if an overflow would occur if we add this tag. */
    if (len + buffer->mem_total_data_len + 
        PPE_TAG_HEADER_SIZE > NET_PARENT_BUFFER_SIZE)
        return 0;
    else
    {
        /* Copy the tag header data into the temp string, then append it
           into the buffer. Do the same for the tag string. */
        PUT16(&buffer->data_ptr[buffer->mem_total_data_len], 0, type);
        PUT16(&buffer->data_ptr[buffer->mem_total_data_len], 2, len);
        PUT_STRING(buffer->data_ptr, (unsigned int)(buffer->mem_total_data_len +
                   PPE_TAG_HEADER_SIZE), string, (unsigned int)len);

        /* Adjust the buffer lengths. */
        buffer->data_len += (len + PPE_TAG_HEADER_SIZE);
        buffer->mem_total_data_len = buffer->data_len;
    }
    
    return (UINT16)(len + PPE_TAG_HEADER_SIZE);

} /* PPEU_Append_Tag */



/************************************************************************
* FUNCTION
*
*       PPEU_Get_Tag
*
* DESCRIPTION
*
*       Read the tag information from the Net buffer into a PPE_TAG
*       structure.
*
* INPUTS
*
*       *buffer             - Pointer to a Net buffer.
*       *tag                - A pointer to a tag to be filled. The offset
*                             field must already contain the offset to 
*                             the tag.
*
* OUTPUTS
*
*       UINT16              - The length of the tag, including header.
*
************************************************************************/
UINT16 PPEU_Get_Tag(NET_BUFFER *buffer, PPE_TAG *tag)
{
    /* Get the tag's type. */
    tag->ppe_type = GET16(buffer->data_ptr, (unsigned int)tag->ppe_offset);

    /* Get the tag's length. */
    tag->ppe_length = GET16(buffer->data_ptr, tag->ppe_offset + 2);

    /* Get the address of the start data. */
    tag->ppe_string = &buffer->data_ptr[tag->ppe_offset + PPE_TAG_HEADER_SIZE];
    
    /* Return the total length of this tag, including its header. */
    return (UINT16)(tag->ppe_length + PPE_TAG_HEADER_SIZE);

} /* PPEU_Get_Tag */



/************************************************************************
* FUNCTION
*
*       PPEU_New_Buffer
*
* DESCRIPTION
*
*       Get a Net buffer from the freelist and initialize its data_ptr
*       and length pointers. Write any necessary tags and set the
*       buffer's dlist appropriately.
*
* INPUTS
*
*       *ppe_layer          - Pointer to the PPPoE data structure.
*
* OUTPUTS
*
*       NET_BUFFER*         - A pointer to the new buffer.
*
************************************************************************/
NET_BUFFER *PPEU_New_Buffer(DV_DEVICE_ENTRY *vdevice)
{
    NET_BUFFER  *buffer = NU_NULL;
    LINK_LAYER  *link_layer;
    PPE_LAYER   *ppe_layer;

    link_layer = ((LINK_LAYER*)vdevice->dev_ppp_layer);
    ppe_layer = ((PPE_LAYER*)link_layer->link);
    
	/* Check that the entire packet will fit in one buffer. */
	if ( (vdevice->dev_hdrlen - PPP_HEADER_SIZE >= 0) &&
		 (vdevice->dev_hdrlen - PPP_HEADER_SIZE <= NET_PARENT_BUFFER_SIZE) )
	{	
	    /* Get a new buffer from the free list. */
	    buffer = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);
		
	    if (buffer)
	    {
	        /* Set the data pointer and length pointers. */
	        buffer->data_ptr = buffer->mem_parent_packet + vdevice->dev_hdrlen
	            - PPP_HEADER_SIZE;
	
	        buffer->data_len = 0;
	        buffer->mem_total_data_len = 0;
	
	        /* NULL the next pointers */
	        buffer->next           = NU_NULL;
	        buffer->next_buffer    = NU_NULL;
	
	        /* Make the driver remove it after sending. */
	        buffer->mem_dlist = &MEM_Buffer_Freelist;

#if (INCLUDE_PPE_HOST == NU_TRUE)
	        /* Only a PADT packet would cause the session bit to be set in status.
	           In this case, we don't save the buffer. */
	        if (!(ppe_layer->ppe_status & PPE_SESSION))
	        {
	            if (ppe_layer->ppe_status & PPE_HOST_MODE)
	            {
	                /* Make sure the driver does not remove it after sending. */
	                buffer->mem_dlist = NU_NULL;
	
	                /* Store a pointer to it in case we need to resend it. */
	                ppe_layer->ppe_bufptr = buffer;
	            }
	        }
#endif
    	}
	    else
	    {
	        NERRS_Log_Error (TCP_SEVERE, __FILE__, __LINE__);
	        
#if (NU_DEBUG_PPE == NU_TRUE)
	        PPE_Printf("PPEU_New_Buffer() - Unable to allocate Net buffer.\r\n");
#endif
	    }
	}
	
    else
    {
        NERRS_Log_Error (TCP_SEVERE, __FILE__, __LINE__);
        
#if (NU_DEBUG_PPE == NU_TRUE)
        PPE_Printf("PPEU_New_Buffer() - Buffer size too small.\r\n");
#endif
    }	

    return buffer;

} /* PPEU_New_Buffer */


#if (INCLUDE_PPPOE_TCP_MSS_FIX == NU_TRUE)
/************************************************************************
* FUNCTION
*
*       PPPU_Fix_TCP_MSS
*
* DESCRIPTION
*
*       Scan the given IP buffer for a TCP SYN packet that may
*       contain an MSS option that is too large for PPPoE. If the
*       MSS is too large, it is replaced by the correct value, and
*       the TCP checksum is adjusted.
*
* INPUTS
*
*       NET_BUFFER*         - A pointer to the buffer to update
*
* OUTPUTS
*
*       NONE
*
************************************************************************/
VOID PPEU_Fix_TCP_MSS(NET_BUFFER *buf_ptr)
{
    INT                 tcp_options_len, x;
    UINT16              mss;
    UINT8 HUGE          *ptr, HUGE *tcpptr;
    UINT8               temp[2];

    /* Move past the PPP protocol field. This is always 2 bytes. */
    ptr = &buf_ptr->data_ptr[2];

    /* Check the IP protocol type. Keeping going if it is TCP. */
    if (ptr[IP_PROTOCOL_OFFSET] == IP_TCP_PROT)
    {
        /* Move to the TCP header. */
        if ( (buf_ptr->mem_flags & NET_PARENT) && (buf_ptr->next_buffer) )
        {
            /* Get TCP data from the next buffer */
            tcpptr = buf_ptr->next_buffer->data_ptr;
        }
        else
        {
            /* If TCP data immediately follows the IP header, adjust ptr. */
            x = ((ptr[IP_VERSIONANDHDRLEN_OFFSET] & 0x0f) << 2);
            tcpptr = &ptr[x];
        }

        /* Check the TCP flags, keep going if the SYN bit is set. */
        if (tcpptr[TCP_FLAGS_OFFSET] & TSYN)
        {
            tcp_options_len = (UINT16)(tcpptr[TCP_HLEN_OFFSET] >> 2) - TCP_HDR_LEN;

            /* Move past the TCP header. */
            ptr = &tcpptr[TCP_HDR_LEN];

            /* Is the MSS option present? If so, is it within an acceptable
               range for PPPoE? */
            for (x = 0; x < tcp_options_len;)
            {
                switch (ptr[x])
                {
                case 0:

                    /* End of options option. Get out. */
                    x = tcp_options_len;
                    break;

                case 1:

                    /* Noop option */
                    x++;
                    break;

                case 2:

                    /* MSS option, verify the length. It must be 4. */
                    if (ptr[x + 1] == 4)
                    {
                        mss = GET16(&ptr[x], 2);
                        memcpy(temp, &ptr[x+2], 2);
                        if (mss > (PPE_MTU - (IP_HDR_LEN + TCP_HDR_LEN)))
                        {
                            /* The MSS is not good for PPPoE. Update it 
                               and recompute the TCP checksum. */
                            PUT16(&ptr[x], 2, (PPE_MTU - (IP_HDR_LEN + TCP_HDR_LEN)));
                            
                            PPEU_Adjust_Checksum(&tcpptr[TCP_CHECK_OFFSET], 
                                temp, 2, &ptr[x+2],2);

                        } /* end if the MSS is ok */

                    } /* end if the the size is correct */

                    /* Note: fall through to default case */

                default:

                    x += ptr[x + 1];

                    break;

                } /* end switch */

            } /* end for */

        } /* end if the SYN flag is present */

    } /* end if this is a TCP packet */

} /* PPEU_Fix_TCP_MSS */



/***********************************************************************
*                                                                       
*   FUNCTION
*
*       PPEU_Adjust_Checksum
*                                                                       
*   DESCRIPTION
*
*       This function adjusts the checksum according to the changes made
*       in the respective header.
*                                                                       
*   INPUTS
*                         
*       *chksum     A pointer to the original checksum.
*       *optr       A pointer to the original buffer of data.
*       olen        The length of the original buffer of data.
*       *nptr       A pointer to the translated buffer of data.
*       nlen        The length of the translated buffer of data.                                              
*
*   OUTPUTS                                                               
*                                                               
*       None.        
*                                                                       
*************************************************************************/
VOID PPEU_Adjust_Checksum(UINT8 HUGE *chksum, UINT8 HUGE *optr, UINT32 olen, 
                          UINT8 HUGE *nptr, UINT32 nlen)
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


} /* PPEU_Adjust_Checksum */

#endif /* INCLUDE_PPPOE_TCP_MSS_FIX */

