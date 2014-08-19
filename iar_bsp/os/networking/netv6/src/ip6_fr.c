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
*       ip6_fr.c                                     
*
*   DESCRIPTION
*
*       This file contains routines for IPv6 packet fragmentation.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IP6_Fragment
*       IP6_SFragment
*
*   DEPENDENCIES
*
*       nu_net.h
*       ip6_mib.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/ip6_mib.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Fragment                                                      
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Fragment an IPv6 packet.                                           
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *buf_ptr                A pointer to the NET buffer
*       *ip                     A pointer to the IPv6 header
*       *int_face               A pointer to the device entry interface                                                        
*       *dest                   A pointer to the socket IPv6 address 
*                               information
*       *ro                     A pointer to the route information                                                        
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              If successful                                                      
*       NU_MSGSIZE              Messages size is incorrect                                                      
*                                                                       
*************************************************************************/
STATUS IP6_Fragment(NET_BUFFER *buf_ptr, const IP6LAYER *ip, 
                    DV_DEVICE_ENTRY *int_face, SCK6_SOCKADDR_IP *dest, 
                    RTAB6_ROUTE *ro)
{
    UINT16      hlen, unfrag_part;
    INT32       len, tlen;
    INT32       off;
    NET_BUFFER  *work_buf = NU_NULL;
    NET_BUFFER  *f_buf;
    NET_BUFFER  **next = NU_NULL;
    STATUS      err = NU_SUCCESS;
    IP6LAYER    *f_ip;
    INT32       aloc_len;
    INT         at_least_one_succeeded = 0;
    INT16       target = -1;
    UINT8       next_header;
    UINT8 HUGE  *temp_ptr;
    UINT32      id;

    /* Make sure this buffer is not pointing to any list. It should
       not be at this point. */
    buf_ptr->next = NU_NULL;

    hlen = IP6_HEADER_LEN;

    /* Check if there are any extension headers present */
    if (!(IP6_IS_NXTHDR_RECPROT(buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET])

#if (INCLUDE_IPSEC == NU_TRUE)

    /* If the next extension header after the IP header is AH or ESP,
     * do not check the remaining extension headers.
     */
               ||(buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET] == IPPROTO_AUTH)
               ||(buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET] == IPPROTO_ESP)
#endif
                ))
    {
        next_header = buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET];

        temp_ptr = buf_ptr->data_ptr;

        /* Determine if there is a Routing Header or Hop-By-Hop Options 
         * header. 
         */
        while ( (target == -1) && (hlen <= buf_ptr->data_len) )
        {
            switch (next_header)
            {
                case IPPROTO_ROUTING:
                    target = IPPROTO_ROUTING;
                    break;

                case IPPROTO_HOPBYHOP:
                    target = IPPROTO_HOPBYHOP;
                    break;

                default :                    
                    break;
            }

            /* Increment the data pointer to point to the protocol 
             * header 
             */
            temp_ptr = (UINT8 HUGE*)((UINT32)buf_ptr->data_ptr + hlen);

            next_header = temp_ptr[IP6_EXTHDR_NEXTHDR_OFFSET];

            hlen = 
                (UINT16)(hlen + (8 + (temp_ptr[IP6_EXTHDR_LENGTH_OFFSET] << 3)));
        }

        /* Set the new next-header value of the last unfragmentable 
         * extension header 
         */
        temp_ptr[IP6_EXTHDR_NEXTHDR_OFFSET] = IPPROTO_FRAGMENT;
    }

    else
    {
        /* Save the next-header value of the IPv6 header */
        next_header = buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET];

        /* Set the new next-header value of IPv6 header */
        buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET] = IPPROTO_FRAGMENT;
    }

    unfrag_part = (UINT16)(hlen + IP6_FRAGMENT_HDR_LENGTH);

    id = UTL_Rand();

    /* If the link MTU was changed to a value lower than the path MTU 
     * for this route, set the path MTU for the route to the default MTU.
     */
    if (ro->rt_route->rt_path_mtu > int_face->dev6_link_mtu)
        ro->rt_route->rt_path_mtu = int_face->dev6_link_mtu;

    /* len is the number of data bytes in each fragment. Computed as 
     * the mtu of the interface less the size of the header and rounded 
     * down to an 8-byte boundary by clearing the low-order 3 bits (& ~7).
     */
    len = ((INT32)(ro->rt_route->rt_path_mtu) - unfrag_part) & ~7;

    /* Each fragment must be able to hold at least 8 bytes. */
    if (len < 8)
        return (NU_MSGSIZE);

    tlen = (INT32)buf_ptr->mem_total_data_len;

    aloc_len = len;

    /* Create the fragments. */
    for (off = (unfrag_part - IP6_FRAGMENT_HDR_LENGTH); 
         off < tlen; 
         off += len)
    {
        /* Shorten the length if this is the last fragment. */
        if (off + len >= tlen)            
            aloc_len = tlen - off;

        /* Allocate a buffer chain to build the fragment in. */
        f_buf = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist, 
                                         (aloc_len + unfrag_part + 
                                          int_face->dev_hdrlen));
        if (f_buf == NU_NULL)
        {
            err = NU_NOBUFS;
            break;
        }

        /* Initialize mem_dlist for deallocation */
        f_buf->mem_dlist = &MEM_Buffer_Freelist;

        /* Point to the location where the IP header will begin. */
        f_buf->data_ptr = 
            f_buf->mem_parent_packet + int_face->dev_hdrlen;

        /* Overlay the IP header so we can access the individual 
         * fields. 
         */
        f_ip = (IP6LAYER *)f_buf->data_ptr;

        /* Copy the IPv6 header and extension headers over to the 
         * new packet. 
         */
        NU_BLOCK_COPY(f_ip, ip, unfrag_part);

        /* Set the next-header value of the Fragment Header */
        f_buf->data_ptr[hlen + IP6_EXTHDR_NEXTHDR_OFFSET] = next_header;

        /* Zero out the reserved fields */
        f_buf->data_ptr[hlen + IP6_FRAGMENT_RES1_OFFSET] = 0;

        PUT16(f_buf->data_ptr, (hlen + IP6_FRAGMENT_FRGOFFSET_OFFSET), 0);

        /* Fill in the randomly generated Identification value */
        PUT32(f_buf->data_ptr, (hlen + IP6_FRAGMENT_ID_OFFSET), id);

        /* Set the offset field for the fragment. */
        PUT16(f_ip, hlen + IP6_FRAGMENT_FRGOFFSET_OFFSET, 
              (UINT16)((UINT32)off - (unfrag_part - 8)));

        /* Is this the last fragment. MF is set for every fragment except 
         * the last one. 
         */

        /* Shorten the length if this is the last fragment. */
        if ((off + len) >= tlen)
            len = tlen - off;

        /* This is not the last fragment. Set MF. */
        else
            PUT16(f_ip, hlen + IP6_FRAGMENT_FRGOFFSET_OFFSET, 
                  (UINT16)(GET16(f_ip, hlen + 
                                 IP6_FRAGMENT_FRGOFFSET_OFFSET) | IP6_MF) );

        /* Set the new length. */
        PUT16(f_ip, IP6_PAYLEN_OFFSET, 
              (UINT16)(len + (unfrag_part - IP6_HEADER_LEN)));

        /* The IP header is the only data present. */
        f_buf->data_len = (UINT32)unfrag_part;

        /* We will also add the device header length here. This
         * is only done so that the Chain_Copy routine below will
         * compute the correct size for the buffer area available.
         * It will be removed after the copy. 
         */
        f_buf->data_len += int_face->dev_hdrlen;

        /* Move the data pointer back to the start of the link-layer 
         * header.  This is also being done for the Chain_Copy routine. 
         * It will offset by the data_len and skip over these headers when
         * doing the copy. 
         */
        f_buf->data_ptr -= int_face->dev_hdrlen;

        /* Copy data from the original packet into this fragment. */
        MEM_Chain_Copy(f_buf, buf_ptr, off, len);

        /* Now remove the link-layer header length that was added
           above. */
        f_buf->data_len -= int_face->dev_hdrlen;

        /* Put the data pointer back as well. */
        f_buf->data_ptr += int_face->dev_hdrlen;

        /* Set the length of all data in this buffer chain. */
        f_buf->mem_total_data_len = (UINT32)(len + unfrag_part);

        /* Clear the device pointer. */
        f_buf->mem_buf_device = NU_NULL;

        /* Link this fragment into the list of fragments. */
        if (next != NU_NULL)
            *next = f_buf;            
        else
            work_buf = f_buf;
        
        next = &f_buf->next;

        /* Increment the number of fragments that have been created. */
        MIB_ipv6IfStatsOutFragCrt_Inc(int_face);
    }

    /* Send each fragment */
    for (f_buf = work_buf; f_buf; f_buf = work_buf)
    {
        work_buf = f_buf->next;
        f_buf->next = NU_NULL;

        /* If the first packet cannot be transmitted successfully, then abort 
         * and free the rest. 
         */
        if (err == NU_SUCCESS)
        {
            /* Save the flags from the original buffer into this buffer.  Note
             * that saving the NET_PARENT flag in each buffer will not cause
             * problems since that flag is used only for TCP, and TCP data
             * does not get fragmented.
             */
            f_buf->mem_flags = buf_ptr->mem_flags;

            /* Set the IPv6 flag in the fragment. */
            f_buf->mem_flags |= NET_IP6;

            err = (*(int_face->dev_output))(f_buf, int_face, dest, ro);

            if (err != NU_SUCCESS)
                MEM_One_Buffer_Chain_Free(f_buf, &MEM_Buffer_Freelist);
            else
            {
                /* If at least one fragment was sent successfully, we will want
                 * to return success below. This is so the upper layer 
                 * protocols will not try to free the fragment that was sent 
                 * successfully. In reality this should never occur. It would
                 * require a device going down or a route being timed out from 
                 * under us. Both of which should be impossible as we have 
                 * the semaphore. 
                 */
                at_least_one_succeeded = 1;
            }
        }
        else
            MEM_One_Buffer_Chain_Free(f_buf, &MEM_Buffer_Freelist);
    }

    if (at_least_one_succeeded)
    {
        /* Free the original buffer that was passed in */
        MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

        return (NU_SUCCESS);
    }
    else
        return (err);

} /* IP6_Fragment */

/***********************************************************************
*
*   FUNCTION
*
*       IP6_SFragment
*
*   DESCRIPTION
*
*       Fragment an IPv6 packet and instead of transmitting, 
*       it returns list of fragment buffers the to caller.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the NET buffer
*       *ip                     A pointer to the IPv6 header
*       *int_face               A pointer to the device entry interface
*       *dest                   A pointer to the socket IPv6 address
*                               information
*       *ro                     A pointer to the route information
*       **work_buf_ret          List of fragment buffers on return.
*
*   OUTPUTS
*
*       NU_SUCCESS              If successful
*       NU_MSGSIZE              Messages size is incorrect
*
*************************************************************************/
STATUS IP6_SFragment(NET_BUFFER *buf_ptr, const IP6LAYER *ip,
                    DV_DEVICE_ENTRY *int_face, RTAB6_ROUTE *ro,
                    NET_BUFFER  **work_buf_ret)
{
    UINT16      hlen, unfrag_part;
    INT32       len, tlen;
    INT32       off;
    NET_BUFFER  *work_buf = NU_NULL;
    NET_BUFFER  *f_buf;
    NET_BUFFER  **next = NU_NULL;
    STATUS      err = NU_SUCCESS;
    IP6LAYER    *f_ip = NU_NULL;
    INT32       aloc_len;
    INT16       target = -1;
    UINT8       next_header;
    UINT8 HUGE  *temp_ptr;
    UINT32      id;

    /* Make sure this buffer is not pointing to any list. It should
       not be at this point. */
    buf_ptr->next = NU_NULL;

    hlen = IP6_HEADER_LEN;

    /* Check if there are any extension headers present */
    if (!(IP6_IS_NXTHDR_RECPROT(buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET])

#if (INCLUDE_IPSEC == NU_TRUE)

    /* If the next extension header after the IP header is AH or ESP,
     * do not check the remaining extension headers.
     */
               ||(buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET] == IPPROTO_AUTH)
               ||(buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET] == IPPROTO_ESP)
#endif
                ))
    {
        next_header = buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET];

        temp_ptr = buf_ptr->data_ptr;

        /* Determine if there is a Routing Header or Hop-By-Hop Options
         * header.
         */
        while ( (target == -1) && (hlen <= buf_ptr->data_len) )
        {
            switch (next_header)
            {
                case IPPROTO_ROUTING:
                    target = IPPROTO_ROUTING;
                    break;

                case IPPROTO_HOPBYHOP:
                    target = IPPROTO_HOPBYHOP;
                    break;

                default :
                    break;
            }

            /* Increment the data pointer to point to the protocol
             * header
             */
            temp_ptr = (UINT8 HUGE*)((UINT32)buf_ptr->data_ptr + hlen);

            next_header = temp_ptr[IP6_EXTHDR_NEXTHDR_OFFSET];

            hlen =
                (UINT16)(hlen + (8 + (temp_ptr[IP6_EXTHDR_LENGTH_OFFSET] << 3)));
        }

        /* Set the new next-header value of the last unfragmentable
         * extension header
         */
        temp_ptr[IP6_EXTHDR_NEXTHDR_OFFSET] = IPPROTO_FRAGMENT;
    }

    else
    {
        /* Save the next-header value of the IPv6 header */
        next_header = buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET];

        /* Set the new next-header value of IPv6 header */
        buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET] = IPPROTO_FRAGMENT;
    }

    unfrag_part = (UINT16)(hlen + IP6_FRAGMENT_HDR_LENGTH);

    id = UTL_Rand();

    /* If the link MTU was changed to a value lower than the path MTU
     * for this route, set the path MTU for the route to the default MTU.
     */
    if (ro->rt_route->rt_path_mtu > int_face->dev6_link_mtu)
        ro->rt_route->rt_path_mtu = int_face->dev6_link_mtu;

    /* len is the number of data bytes in each fragment. Computed as
     * the mtu of the interface less the size of the header and rounded
     * down to an 8-byte boundary by clearing the low-order 3 bits (& ~7).
     */
    len = ((INT32)(ro->rt_route->rt_path_mtu) - unfrag_part) & ~7;

    /* Each fragment must be able to hold at least 8 bytes. */
    if (len < 8)
        return (NU_MSGSIZE);

    tlen = (INT32)buf_ptr->mem_total_data_len;

    aloc_len = len;

    /* Create the fragments. */
    for (off = (unfrag_part - IP6_FRAGMENT_HDR_LENGTH);
         off < tlen;
         off += len)
    {
        /* Shorten the length if this is the last fragment. */
        if (off + len >= tlen)
            aloc_len = tlen - off;

        /* Allocate a buffer chain to build the fragment in. */
        f_buf = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                         (aloc_len + unfrag_part +
                                          int_face->dev_hdrlen));
        if (f_buf == NU_NULL)
        {
            err = NU_NOBUFS;
            break;
        }

        /* Initialize mem_dlist for deallocation */
        f_buf->mem_dlist = &MEM_Buffer_Freelist;

        /* Point to the location where the IP header will begin. */
        f_buf->data_ptr =
            f_buf->mem_parent_packet + int_face->dev_hdrlen;

        /* Overlay the IP header so we can access the individual
         * fields.
         */
        f_ip = (IP6LAYER *)f_buf->data_ptr;

        /* Copy the IPv6 header and extension headers over to the
         * new packet.
         */
        NU_BLOCK_COPY(f_ip, ip, unfrag_part);

        /* Set the next-header value of the Fragment Header */
        f_buf->data_ptr[hlen + IP6_EXTHDR_NEXTHDR_OFFSET] = next_header;

        /* Zero out the reserved fields */
        f_buf->data_ptr[hlen + IP6_FRAGMENT_RES1_OFFSET] = 0;

        PUT16(f_buf->data_ptr, (hlen + IP6_FRAGMENT_FRGOFFSET_OFFSET), 0);

        /* Fill in the randomly generated Identification value */
        PUT32(f_buf->data_ptr, (hlen + IP6_FRAGMENT_ID_OFFSET), id);

        /* Set the offset field for the fragment. */
        PUT16(f_ip, hlen + IP6_FRAGMENT_FRGOFFSET_OFFSET,
              (UINT16)((UINT32)off - (unfrag_part - 8)));

        /* Is this the last fragment. MF is set for every fragment except
         * the last one.
         */

        /* Shorten the length if this is the last fragment. */
        if ((off + len) >= tlen)
            len = tlen - off;

        /* This is not the last fragment. Set MF. */
        else
            PUT16(f_ip, hlen + IP6_FRAGMENT_FRGOFFSET_OFFSET,
                  (UINT16)(GET16(f_ip, hlen +
                                 IP6_FRAGMENT_FRGOFFSET_OFFSET) | IP6_MF) );

        /* Set the new length. */
        PUT16(f_ip, IP6_PAYLEN_OFFSET,
              (UINT16)(len + (unfrag_part - IP6_HEADER_LEN)));

        /* The IP header is the only data present. */
        f_buf->data_len = (UINT32)unfrag_part;

        /* We will also add the device header length here. This
         * is only done so that the Chain_Copy routine below will
         * compute the correct size for the buffer area available.
         * It will be removed after the copy.
         */
        f_buf->data_len += int_face->dev_hdrlen;

        /* Move the data pointer back to the start of the link-layer
         * header.  This is also being done for the Chain_Copy routine.
         * It will offset by the data_len and skip over these headers when
         * doing the copy.
         */
        f_buf->data_ptr -= int_face->dev_hdrlen;

        /* Copy data from the original packet into this fragment. */
        MEM_Chain_Copy(f_buf, buf_ptr, off, len);

        /* Now remove the link-layer header length that was added
           above. */
        f_buf->data_len -= int_face->dev_hdrlen;

        /* Put the data pointer back as well. */
        f_buf->data_ptr += int_face->dev_hdrlen;

        /* Set the length of all data in this buffer chain. */
        f_buf->mem_total_data_len = (UINT32)(len + unfrag_part);

        /* Clear the device pointer. */
        f_buf->mem_buf_device = NU_NULL;

        /* Link this fragment into the list of fragments. */
        if (next != NU_NULL)
            *next = f_buf;
        else
            work_buf = f_buf;

        next = &f_buf->next;

        /* Increment the number of fragments that have been created. */
        MIB_ipv6IfStatsOutFragCrt_Inc(int_face);
    }

    *work_buf_ret = work_buf;
    return (err);

} /* IP6_SFragment */
