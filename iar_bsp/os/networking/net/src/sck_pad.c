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
*       sck_pad.c
*
*   DESCRIPTION
*
*       This file contains the routines necessary for parsing ancillary
*       data from the application.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SCK_Parse_Ancillary_Data
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/nu_net6.h"

extern UINT8    IP6_Hop_Limit;
#endif

/***********************************************************************
*
*   FUNCTION
*
*       SCK_Parse_Ancillary_Data
*
*   DESCRIPTION
*
*       This function parses ancillary data from a msghdr structure and
*       puts it in a tx_ancillary_data structure upon verification.
*
*   INPUTS
*
*       *msg                    A pointer to the ancillary data structure
*                               containing the Sticky Options to set for
*                               the socket.
*       *anc_data_ptr           A pointer to the destination structure
*                               for the ancillary data.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         An illegal parameter was provided for one
*                               of the ancillary data objects.
*
*************************************************************************/
STATUS SCK_Parse_Ancillary_Data(const msghdr *msg,
                                tx_ancillary_data *anc_data_ptr)
{
    cmsghdr         *current_cmsg;
    STATUS          return_status = NU_SUCCESS;

#if (INCLUDE_IPV6 == NU_TRUE)
    DV_DEVICE_ENTRY *dev_addr, *dev_index;
    in6_pktinfo     *pkt_info_ptr;
#else
    UNUSED_PARAMETER(anc_data_ptr);
#endif


    /* Traverse the buffer of ancillary data */
    for (current_cmsg = CMSG_FIRSTHDR(msg);
         current_cmsg != NU_NULL && return_status == NU_SUCCESS;
         current_cmsg = CMSG_NXTHDR(msg, current_cmsg))
    {
#if (INCLUDE_IPV6 == NU_TRUE)
        if (current_cmsg->cmsg_level == IPPROTO_IPV6)
        {
            switch (current_cmsg->cmsg_type)
            {
                /* A source address and/or interface index has been
                 * specified.
                 */
                case IPV6_PKTINFO:

                    /* Get a pointer to the in6_pktinfo data structure */
                    pkt_info_ptr = (in6_pktinfo*)(CMSG_DATA(current_cmsg));

                    /* If a source address was specified, use it */
                    if (!(IPV6_IS_ADDR_UNSPECIFIED(pkt_info_ptr->ipi6_addr)))
                    {
                        /* Get a pointer to the interface assigned the source address
                         * specified.
                         */
                        dev_addr =
                            DEV6_Get_Dev_By_Addr(pkt_info_ptr->ipi6_addr);

                        /* If this address does not exist, return an error. */
                        if (dev_addr)
                            anc_data_ptr->tx_source_address =
                                pkt_info_ptr->ipi6_addr;

                        else
                        {
                            return_status = NU_INVALID_ADDRESS;
                            break;
                        }
                    }

                    else
                        dev_addr = NU_NULL;

                    /* If an interface index was specified, use it */
                    if (pkt_info_ptr->ipi6_ifindex != IP6_UNSPECIFIED)
                    {
                        /* Get a pointer to the device referenced by the interface_index */
                        dev_index =
                            DEV_Get_Dev_By_Index(pkt_info_ptr->ipi6_ifindex);

                        /* If there is no interface on the node with the specified interface,
                         * or the interface is not IPv6-enabled, return an error.
                         */
                        if (!dev_index)
                        {
                            return_status = NU_INVALID_PARM;
                            break;
                        }

                        if (!(dev_index->dev_flags & DV6_IPV6))
                        {
                            return_status = NU_DEVICE_DOWN;
                            break;
                        }

                        /* If the address specified does not exist on the
                         * interface specified, return an error.
                         */
                        if ( (!dev_addr) ||
                             (dev_addr->dev_index == pkt_info_ptr->ipi6_ifindex) )
                            anc_data_ptr->tx_interface_index =
                                (UINT8*)&(pkt_info_ptr->ipi6_ifindex);

                        else
                            return_status = NU_INVALID_ADDRESS;
                    }

                    break;

                /* A Hop Limit has been specified */
                case IPV6_HOPLIMIT:

                    /* Set the pointer to the Hop Limit */
                    anc_data_ptr->tx_hop_limit =
                        (UINT8*)(CMSG_DATA(current_cmsg));

                    /* Validate the Hop Limit as per RFC 2292 section 5.3 */
                    if ( ((*((INT*)anc_data_ptr->tx_hop_limit)) < -1) ||
                         ((*((INT*)anc_data_ptr->tx_hop_limit)) >= 256) )
                    {
                        return_status = NU_INVALID_PARM;

                        anc_data_ptr->tx_hop_limit = NU_NULL;
                    }

                    /* If the Hop Limit is -1, use the Kernel Default */
                    else if ((*((INT*)anc_data_ptr->tx_hop_limit)) == -1)
                        (*((INT*)anc_data_ptr->tx_hop_limit)) = IP6_Hop_Limit;

                    break;

                /* A Next-Hop has been specified */
                case IPV6_NEXTHOP:

                    /* Set the pointer to the Next-Hop */
                    anc_data_ptr->tx_next_hop =
                        (struct addr_struct*)(CMSG_DATA(current_cmsg));

                    break;

                case IPV6_TCLASS:

                    /* Set the pointer to the traffic class */
                    anc_data_ptr->tx_traffic_class =
                        (UINT8*)(CMSG_DATA(current_cmsg));

                    /* Validate the traffic class as per RFC 2292bis8 section 6.5 */
                    if ( ((*((INT*)anc_data_ptr->tx_traffic_class)) < -1) ||
                         ((*((INT*)anc_data_ptr->tx_traffic_class)) >= 256) )
                    {
                        return_status = NU_INVALID_PARM;

                        anc_data_ptr->tx_traffic_class = NU_NULL;
                    }

                    /* If the traffic class is -1, use the Kernel Default */
                    else if ((*((INT*)anc_data_ptr->tx_traffic_class)) == -1)
                        (*((INT*)anc_data_ptr->tx_traffic_class)) = IP6_TCLASS_DEFAULT;

                    break;

                case IPV6_RTHDR:

                    /* Set the pointer to the routing header */
                    anc_data_ptr->tx_route_header =
                        (UINT8*)(CMSG_DATA(current_cmsg));

                    break;

                case IPV6_HOPOPTS:

                    /* Set the data pointer to the Hop-By-Hop option */
                    anc_data_ptr->tx_hop_opt =
                        (UINT8*)(CMSG_DATA(current_cmsg));

                    break;

                case IPV6_DSTOPTS:

                    /* Set the data pointer to the Destination option */
                    anc_data_ptr->tx_dest_opt =
                        (UINT8*)(CMSG_DATA(current_cmsg));

                    break;

                case IPV6_RTHDRDSTOPTS:

                    /* Set the data pointer to the Destination option */
                    anc_data_ptr->tx_rthrdest_opt =
                        (UINT8*)(CMSG_DATA(current_cmsg));

                    break;

                default:

                    break;
            }
        }
#endif
    }

    return (return_status);

} /* SCK_Parse_Ancillary_Data */
