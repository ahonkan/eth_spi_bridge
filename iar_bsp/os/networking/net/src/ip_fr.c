/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       ip_fr.c
*
* DESCRIPTION
*
*       This file contains routines for IP packet fragmentation.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IP_Fragment
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPSEC == NU_TRUE)
#include "networking/ips_api.h"
#endif

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

/***********************************************************************
*
*   FUNCTION
*
*       IP_Fragment
*
*   DESCRIPTION
*
*       Fragment an IP packet.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the NET buffer
*       *ip                     A pointer to the IP structure
*                               information
*       *int_face               A pointer to the device entry interface
*       *dest                   A pointer to the socket IP address
*                               information
*       *ro                     A pointer to the RTAB route information
*
*   OUTPUTS
*
*       NU_SUCCESS              If successful
*       NU_MSGSIZE              Messages size is incorrect
*
*************************************************************************/
STATUS IP_Fragment(NET_BUFFER *buf_ptr, IPLAYER *ip,
                   DV_DEVICE_ENTRY *int_face, SCK_SOCKADDR_IP *dest,
                   RTAB_ROUTE *ro)
{
    UINT16      hlen , f_hlen;
    INT32       len, tlen, first_len, data_len, total_data_len;
    INT32       off;
    NET_BUFFER  *work_buf;
    NET_BUFFER  *f_buf = buf_ptr;
    NET_BUFFER  **next = &buf_ptr->next;
    STATUS      err = NU_SUCCESS;
    IPLAYER     *f_ip;
    INT32       aloc_len;
    INT         at_least_one_succeeded = 0;
    UINT16      buf_flags;

    /* Make sure this buffer is not pointing to any list. It should
       not be at this point. */
    buf_ptr->next = NU_NULL;

    hlen = (UINT16)((GET8(ip, IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2);

    /* len is the number of data bytes in each fragment. Computed as the Path
       MTU of the route less the size of the IP Header and rounded down to an
       8-byte boundary by clearing the low-order 3 bits (& ~7).  Note that the
       Path MTU of the route does NOT include the length of the interface's
       header.
    */
    len = ((INT32)(ro->rt_route->rt_path_mtu - hlen) & ~7);

    first_len = len;

    /* Each fragment must be able to hold at least 8 bytes. */
    if (len < 8)
        return (NU_MSGSIZE);

    tlen = GET16(ip, IP_TLEN_OFFSET);

    buf_ptr = f_buf;

    f_hlen = IP_HEADER_LEN;

    aloc_len = len;

    /* Save the flags from the head buffer. */
    buf_flags = buf_ptr->mem_flags;

    /* Set the packet type that is in the buffer. */
    buf_flags |= NET_IP;

    /* Create the fragments. */
    for (off = hlen + len; off < tlen; off += len)
    {
        if (off + len >= tlen)
            /* Shorten the length if this is the last fragment. */
            aloc_len = tlen - off;

        /* Allocate a buffer chain to build the fragment in. */
        f_buf = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                         (aloc_len + hlen + int_face->dev_hdrlen));

        if (f_buf == NU_NULL)

        {
            err = NU_NOBUFS;

            /* Increment the number of IP packets that could not be
               fragmented. In this case because of no buffers. */
            MIB2_ipFragFails_Inc;

            break;
        }

        /* Initialize mem_dlist for deallocation */
        f_buf->mem_dlist = &MEM_Buffer_Freelist;

        /* Point to the location where the IP header will begin. */
        f_buf->data_ptr = f_buf->mem_parent_packet + int_face->dev_hdrlen;

        /* Overlay the IP header so we can access the individual fields. */
        f_ip = (IPLAYER *)f_buf->data_ptr;

        /* Copy the IP header over to the new packet. */
        IP_HEADER_COPY(f_ip, ip);

        /* If there are options in the original packet, copy them. */
        if (hlen > IP_HEADER_LEN)
        {
            f_hlen = (UINT16)(IP_Option_Copy(f_ip, ip) + (INT32)sizeof(IPLAYER));

            PUT8(f_ip, IP_VERSIONANDHDRLEN_OFFSET,
                 (UINT8)((f_hlen >> 2) | (IP_VERSION << 4)) );
        }

        /* Set the offset field for the fragment. */
        PUT16(f_ip, IP_FRAGS_OFFSET,
            (UINT16)((((UINT32)off - hlen) >> 3) + (GET16(ip, IP_FRAGS_OFFSET) & ~IP_MF)) );

        /* If MF is set in the original packet then it should be set in all
           fragments. */
        if (GET16(ip, IP_FRAGS_OFFSET) & IP_MF)
            PUT16(f_ip, IP_FRAGS_OFFSET,
                  (UINT16)(GET16(f_ip, IP_FRAGS_OFFSET) | IP_MF));

        /* Is this the last fragment. MF is set for every fragment except the
           last one. Unless MF was set in the original packet. In that case MF
           should have already been set above. */

        if ((off + len) >= tlen)
            /* Shorten the length if this is the last fragment. */
            len = tlen - off;
        else
            /* This is not the last fragment. Set MF. */
            PUT16(f_ip, IP_FRAGS_OFFSET,
                  (UINT16)(GET16(f_ip,IP_FRAGS_OFFSET) | IP_MF) );

        /* Set the new length. */
        PUT16(f_ip, IP_TLEN_OFFSET, (UINT16)(len + f_hlen));

        /* The IP header is the only data present. */
        f_buf->data_len = (UINT32)f_hlen;

        /* We will also add the device header length here. This
           is only done so that the Chain_Copy routine below will
           compute the correct size for the buffer area available.
           It will be removed after the copy. */
        f_buf->data_len += int_face->dev_hdrlen;

        /* Move the data pointer back to the start of the link-layer header.
           This is also being done for the Chain_Copy routine. It will
           offset by the data_len and skip over these headers when
           doing the copy. */
        f_buf->data_ptr -= int_face->dev_hdrlen;

        /* Copy data from the original packet into this fragment. */
        MEM_Chain_Copy(f_buf, buf_ptr, off, len);

        /* Now remove the link-layer header length that was added
           above. */
        f_buf->data_len -= int_face->dev_hdrlen;

        /* Put the data pointer back as well. */
        f_buf->data_ptr += int_face->dev_hdrlen;

        /* Set the length of all data in this buffer chain. */
        f_buf->mem_total_data_len = (UINT32)(len + f_hlen);

        /* Clear the device pointer. */
        f_buf->mem_buf_device = NU_NULL;

        /* Compute the IP header checksum. */
        PUT16(f_ip, IP_CHECK_OFFSET, 0);

#if (HARDWARE_OFFLOAD == NU_TRUE)
        /* If the hardware is not going to compute the checksum, do it
         * now in software.
         */
        if (!(int_face->dev_hw_options_enabled & HW_TX_IP4_CHKSUM))
#endif
        {
            PUT16(f_ip, IP_CHECK_OFFSET, TLS_IP_Check((VOID *)f_ip,
                                                      (UINT16)(f_hlen >> 1)));
        }

        /* Link this fragment into the list of fragments. */
        *next = f_buf;
        next = &f_buf->next;

        /* Increment the number of fragments that have been created. */
        MIB2_ipFragCreates_Inc;
    }

    /* If the above loop was successful then the next step is to trim
       all of the data in the original buffer chain that has been relocated
       to the chain of fragments. */
    if (err == NU_SUCCESS)
    {
        /* Increment the number of packets that have been fragmented. */
        MIB2_ipFragOKs_Inc;

        /* Convert the original packet into the first fragment */
        f_buf = buf_ptr;

        /* Update the first fragment by trimming what has been copied out. */
        MEM_Trim(f_buf, (INT32)(hlen + first_len - tlen));

        /* Determine the total data length of the first fragment */
        total_data_len = (INT32)f_buf->mem_total_data_len;
        data_len = 0;

        /* Terminate the first fragment */
        for (work_buf = f_buf;
             data_len < total_data_len;
             work_buf = work_buf->next_buffer)
        {
            data_len = (INT32)(work_buf->data_len) + data_len;

            if (data_len == total_data_len)
            {
                /* Deallocate the buffers that the first fragment does not need */
                MEM_One_Buffer_Chain_Free(work_buf->next_buffer, &MEM_Buffer_Freelist);

                work_buf->next_buffer = NU_NULL;
            }
        }

        /* Update the header in the first fragment (the original packet). */
        PUT16(ip, IP_TLEN_OFFSET, (UINT16)f_buf->mem_total_data_len);
        PUT16(ip, IP_FRAGS_OFFSET, (UINT16)(GET16(ip, IP_FRAGS_OFFSET) | IP_MF));
        PUT16(ip, IP_CHECK_OFFSET, 0);

#if (HARDWARE_OFFLOAD == NU_TRUE)
        /* If the hardware is not going to compute the checksum, do it now in
         * software.
         */
        if (!(int_face->dev_hw_options_enabled & HW_TX_IP4_CHKSUM))
#endif
        {
            PUT16(ip, IP_CHECK_OFFSET, TLS_IP_Check((VOID *)ip, (UINT16)(f_hlen >> 1)) );
        }
    }
    else
    {
        /* There were not enough buffers to break the original buffer chain up
           into fragments. An error will be returned and the upper layer
           software will free the original buffer chain. To keep the buffer chain
           from being freed twice (once below and once in the upper layer
           software) move buf_ptr forward, and separate it from the fragments
           that were created. Doing this will cause the for loop below to only
           free those buffers that were allocated within this function. The
           upper layer software will free the original buffer chain. */
        work_buf = buf_ptr;
        buf_ptr = buf_ptr->next;
        work_buf->next = NU_NULL;
    }

    /* Save a pointer to the first buffer in the chain. It might be needed below. */
    work_buf = buf_ptr;

    /* Send each fragment */
    for (f_buf = buf_ptr; f_buf; f_buf = buf_ptr)
    {
        buf_ptr = f_buf->next;
        f_buf->next = NU_NULL;

        /* If the first packet cannot be transmitted successfully, then abort
           and free the rest. */
        if (err == NU_SUCCESS)
        {
            /* Save the flags from the original buffer into this buffer. */
            f_buf->mem_flags = buf_flags;

            err = (*(int_face->dev_output))(f_buf, int_face, dest, ro);

            if (err != NU_SUCCESS)
            {
                /* If the error occurred on the first buffer, do not free it. It will
                   be freed by the upper layer protocol. */
                if (f_buf != work_buf)
                {
                    MEM_One_Buffer_Chain_Free(f_buf, &MEM_Buffer_Freelist);
                }
            }
            else
            {
                /* If at least one fragment was sent successfully, we will want
                   to return success below. This is so the upper layer
                   protocols will not try to free the fragment that was sent
                   successfully. In reality this should never occur. It would
                   require a device going down or a route being timed out from
                   under us. Both of which should be impossible as we have
                   the semaphore. */
                at_least_one_succeeded = 1;
            }
        }
        else
            MEM_One_Buffer_Chain_Free(f_buf, &MEM_Buffer_Freelist);
    }

    if (at_least_one_succeeded)
        return (NU_SUCCESS);
    else
        return (err);

} /* IP_Fragment */

#endif
