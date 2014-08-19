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
*
*   FILENAME
*
*       sck_sad.c
*
*   DESCRIPTION
*
*       This file contains the routines necessary for creating a new
*       ancillary data structure and storing the requested ancillary
*       data in the structure.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SCK_Store_Ancillary_Data
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/externs6.h"
#endif

/***********************************************************************
*
*   FUNCTION
*
*       SCK_Store_Ancillary_Data
*
*   DESCRIPTION
*
*       This function creates a new ancillary data structure and stores
*       the requested ancillary data in the structure to be returned
*       to the application.
*
*   INPUTS
*
*       *sockptr                A pointer to the socket on which the
*                               buffer from which to retrieve the
*                               ancillary data exists.
*       *msg                    A pointer to the ancillary data structure
*                               containing the memory area into which to
*                               store the new ancillary data.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SCK_Store_Ancillary_Data(struct sock_struct *sockptr, msghdr *msg)
{
    UINT16              length;
    cmsghdr             *cmsg;
#if (INCLUDE_IPV6 == NU_TRUE)
    in6_pktinfo         *pkt_info;
    INT                 *int_ptr;
    struct ip6_rthdr    *rt_hdr;
    UINT32              ext_len;
    UINT8               next_header;
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
    in_pktinfo          *pktv4_info;
#endif

    length = 0;

#if (INCLUDE_IPV6 == NU_TRUE)

    /* If this is a native IPv6 packet. */
    if (sockptr->s_rx_ancillary_data->mem_flags & NET_IP6)
    {
        /* If the application requested the Hop Limit, determine how much
         * memory is required.
         */
        if (sockptr->s_options & SO_IPV6_HOPLIMIT_OP)
            length += CMSG_SPACE(sizeof(INT));

        /* If the application requested the Interface Index and Destination
         * Address, determine how much memory is required.
         */
        if (sockptr->s_options & SO_IPV6_PKTINFO_OP)
            length += CMSG_SPACE(sizeof(in6_pktinfo));

        /* If the application requested the Traffic Class, determine how much
         * memory is required.
         */
        if (sockptr->s_options & SO_IPV6_TCLASS_OP)
            length += CMSG_SPACE(sizeof(INT));
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* If the application requested the IPv4 Destination Address,
         * determine how much memory is required.
         */
        if (sockptr->s_options & SO_IP_PKTINFO_OP)
            length += CMSG_SPACE(sizeof(in_pktinfo));
    }
#endif

    /* If the memory is valid and there is a received buffer on the socket
     * from which to get the ancillary data, fill in the buffer with
     * the new ancillary data structure.
     */
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ( (msg->msg_control != NU_NULL) && (msg->msg_controllen > 0) &&
         (length <= msg->msg_controllen) )
#endif
    {
        /* Get a pointer to the first cmsghdr in the buffer */
        cmsg = CMSG_FIRSTHDR(msg);

#if (INCLUDE_IPV6 == NU_TRUE)
        if (sockptr->s_rx_ancillary_data->mem_flags & NET_IP6)
        {
            if ( (cmsg) && (sockptr->s_options & SO_IPV6_PKTINFO_OP) )
            {
                /* Set the level and type */
                cmsg->cmsg_level = IPPROTO_IPV6;
                cmsg->cmsg_type = IPV6_PKTINFO;

                /* Set the length of the in6_pktinfo object */
                cmsg->cmsg_len = CMSG_LEN(sizeof(in6_pktinfo));

                pkt_info = (in6_pktinfo*)CMSG_DATA(cmsg);

                /* Copy the destination address of the incoming packet */
                NU_BLOCK_COPY(pkt_info->ipi6_addr,
                              ((IP6LAYER*)(sockptr->s_rx_ancillary_data->data_ptr))->
                              ip6_dest, IP6_ADDR_LEN);

                /* Copy the interface index of the device on which the incoming
                 * packet was received.
                 */
                pkt_info->ipi6_ifindex = sockptr->s_recv_if->dev_index;

                /* Get a pointer to the next cmsghdr structure */
                cmsg = CMSG_NXTHDR(msg, cmsg);
            }

            /* Store the incoming Hop Limit in the ancillary data buffer */
            if ( (cmsg) && (sockptr->s_options & SO_IPV6_HOPLIMIT_OP) )
            {
                /* Set the level and type for specifying the hop limit */
                cmsg->cmsg_level = IPPROTO_IPV6;
                cmsg->cmsg_type = IPV6_HOPLIMIT;

                /* Set the length of the hop limit object */
                cmsg->cmsg_len = CMSG_LEN(sizeof(INT));

                /* Fill in the hop limit */
                int_ptr = (INT*)CMSG_DATA(cmsg);

                *int_ptr =
                    GET8(sockptr->s_rx_ancillary_data->data_ptr,
                         IP6_HOPLMT_OFFSET);

                /* Get a pointer to the next cmsghdr structure */
                cmsg = CMSG_NXTHDR(msg, cmsg);
            }

            /* Store the incoming Traffic Class in the ancillary data buffer */
            if ( (cmsg) && (sockptr->s_options & SO_IPV6_TCLASS_OP) )
            {
                /* Set the level and type for specifying the hop limit */
                cmsg->cmsg_level = IPPROTO_IPV6;
                cmsg->cmsg_type = IPV6_TCLASS;

                /* Set the length of the hop limit object */
                cmsg->cmsg_len = CMSG_LEN(sizeof(INT));

                /* Fill in the hop limit */
                int_ptr = (INT*)CMSG_DATA(cmsg);

                *int_ptr =
                    (UINT8)((GET16(sockptr->s_rx_ancillary_data->data_ptr,
                                   IP6_XXX_OFFSET) >> 4) & 0x00ff);

                /* Get a pointer to the next cmsghdr structure */
                cmsg = CMSG_NXTHDR(msg, cmsg);
            }

            /* If there are any extension headers present */
            if (!(IP6_IS_NXTHDR_RECPROT(sockptr->s_rx_ancillary_data->data_ptr[IP6_NEXTHDR_OFFSET])))
            {
                /* Get the type of the next extension header */
                next_header = ((IP6LAYER*)sockptr->s_rx_ancillary_data->data_ptr)->ip6_next;

                /* Increment the data pointer to point to the extension header */
                sockptr->s_rx_ancillary_data->data_ptr += IP6_HEADER_LEN;

                /* Walk through the Extension Headers and add any requested Extension
                 * Headers to the ancillary data structure.
                 */
                for (;;)
                {
                    switch (next_header)
                    {
                        case IPPROTO_HOPBYHOP:

                            /* If the application requested the Hop-By-Hop options to
                             * be returned, store it in the ancillary data buffer.
                             */
                            if ( (cmsg) && (sockptr->s_options & SO_IPV6_HOPOPTS) )
                            {
                                /* Set the level and type for specifying the hop-by-hop
                                 * options
                                 */
                                cmsg->cmsg_level = IPPROTO_IPV6;
                                cmsg->cmsg_type = IPV6_HOPOPTS;

                                ext_len =
                                    (8 + (sockptr->s_rx_ancillary_data->data_ptr[IP6_EXTHDR_LENGTH_OFFSET] << 3));

                                length = (UINT16)(length + CMSG_SPACE(ext_len));

                                /* If the entire hop-by-hop options will fit in the buffer */
                                if (length <= msg->msg_controllen)
                                {
                                    /* Set the length of the object */
                                    cmsg->cmsg_len = (UINT16)(CMSG_LEN(ext_len));

                                    /* Get a pointer to the data area */
                                    int_ptr = (INT*)CMSG_DATA(cmsg);

                                    /* Copy the hop-by-hop options into the buffer */
                                    NU_BLOCK_COPY(int_ptr,
                                                  sockptr->s_rx_ancillary_data->data_ptr,
                                                  (unsigned int)ext_len);

                                    /* Get a pointer to the next cmsghdr structure */
                                    cmsg = CMSG_NXTHDR(msg, cmsg);
                                }
                            }

                            break;

                        case IPPROTO_ROUTING:

                            /* If the application requested the Routing Header to be
                             * returned, store it in the ancillary data buffer.
                             */
                            if ( (cmsg) && (sockptr->s_options & SO_IPV6_RTHDR_OP) )
                            {
                                rt_hdr = (struct ip6_rthdr*)sockptr->s_rx_ancillary_data->data_ptr;

                                /* Set the level and type for specifying the routing header */
                                cmsg->cmsg_level = IPPROTO_IPV6;
                                cmsg->cmsg_type = IPV6_RTHDR;

                                ext_len = inet6_rth_space(rt_hdr->ip6r_type,
                                                          rt_hdr->ip6r_segleft);

                                length = (UINT16)(length + CMSG_SPACE(ext_len));

                                /* If the entire routing header will fit in the buffer */
                                if (length <= msg->msg_controllen)
                                {
                                    /* Set the length of the object */
                                    cmsg->cmsg_len = (UINT16)(CMSG_LEN(ext_len));

                                    /* Get a pointer to the data area */
                                    int_ptr = (INT*)CMSG_DATA(cmsg);

                                    /* Copy the routing header into the buffer */
                                    NU_BLOCK_COPY(int_ptr,
                                                  sockptr->s_rx_ancillary_data->data_ptr,
                                                  (unsigned int)ext_len);

                                    /* Get a pointer to the next cmsghdr structure */
                                    cmsg = CMSG_NXTHDR(msg, cmsg);
                                }
                                else
                                    cmsg->cmsg_len = 0;
                            }

                            break;

                        case IPPROTO_DEST:

                            /* If the application requested the Hop-By-Hop options to
                             * be returned, store it in the ancillary data buffer.
                             */
                            if ( (cmsg) && (sockptr->s_options & SO_IPV6_DESTOPTS) )
                            {
                                /* Set the level and type for specifying the hop-by-hop
                                 * options
                                 */
                                cmsg->cmsg_level = IPPROTO_IPV6;
                                cmsg->cmsg_type = IPV6_DSTOPTS;

                                ext_len =
                                    (8 + (sockptr->s_rx_ancillary_data->data_ptr[IP6_EXTHDR_LENGTH_OFFSET] << 3));

                                length = (UINT16)(length + CMSG_SPACE(ext_len));

                                /* If the entire hop-by-hop options will fit in the buffer */
                                if (length <= msg->msg_controllen)
                                {
                                    /* Set the length of the object */
                                    cmsg->cmsg_len = (UINT16)(CMSG_LEN(ext_len));

                                    /* Get a pointer to the data area */
                                    int_ptr = (INT*)CMSG_DATA(cmsg);

                                    /* Copy the hop-by-hop options into the buffer */
                                    NU_BLOCK_COPY(int_ptr,
                                                  sockptr->s_rx_ancillary_data->data_ptr,
                                                  (unsigned int)ext_len);

                                    /* Get a pointer to the next cmsghdr structure */
                                    cmsg = CMSG_NXTHDR(msg, cmsg);
                                }
                            }

                            break;

                        default:

                            break;
                    }

                    /* Get the type of the next header */
                    next_header = sockptr->s_rx_ancillary_data->data_ptr[IP6_EXTHDR_NEXTHDR_OFFSET];

                    /* If the next_header value is not another extension header,
                     * we have processed all extension headers, break out of the
                     * loop.
                     */
                    if (IP6_IS_NXTHDR_RECPROT(next_header))
                        break;

                    /* Increment the data pointer to point to the next extension header */
                    sockptr->s_rx_ancillary_data->data_ptr +=
                        (8 + (sockptr->s_rx_ancillary_data->data_ptr[IP6_EXTHDR_LENGTH_OFFSET] << 3));
                }
            }
        }

#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        {
            if ( (cmsg) && (sockptr->s_options & SO_IP_PKTINFO_OP) )
            {
                /* Set the level and type */
                cmsg->cmsg_level = IPPROTO_IP;
                cmsg->cmsg_type = IP_PKTINFO;

                /* Set the length of the object */
                cmsg->cmsg_len = CMSG_LEN(sizeof(in_pktinfo));

                pktv4_info = (in_pktinfo*)CMSG_DATA(cmsg);

                /* Copy the destination address of the incoming packet */
                PUT32(pktv4_info->ipi_addr, 0,
                      GET32(sockptr->s_rx_ancillary_data->data_ptr, IP_DEST_OFFSET));

                /* Copy the interface index of the device on which the incoming
                 * packet was received.
                 */
                pktv4_info->ipi_ifindex = sockptr->s_recv_if->dev_index;

                /* Get a pointer to the next cmsghdr structure */
                cmsg = CMSG_NXTHDR(msg, cmsg);
            }
        }
#endif

        /* Free the ancillary data buffer since all the data has been copied from it. */
        MEM_One_Buffer_Chain_Free(sockptr->s_rx_ancillary_data, &MEM_Buffer_Freelist);

        sockptr->s_rx_ancillary_data = NU_NULL;
    }

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    /* Otherwise, indicate that no ancillary data was parsed */
    else
        length = 0;
#endif

    /* Set the length of ancillary data. */
    msg->msg_controllen = length;

} /* SCK_Store_Ancillary_Data */
