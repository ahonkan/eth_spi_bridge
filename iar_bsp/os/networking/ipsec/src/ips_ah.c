/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ips_ah.c
*
* COMPONENT
*
*       AH (Authentication Header)
*
* DESCRIPTION
*
*       This file includes common routines of the IP Authentication Header
*       protocol for both IPv4 and IPv6 based traffic.
*
* DATA STRUCTURES
*
*        None
*
* FUNCTIONS
*
*        IPSEC_Build_AH_Hdr
*
* DEPENDENCIES
*
*        nu_net.h
*        ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ips_esn.h"

#if (IPSEC_INCLUDE_AH == NU_TRUE)

/************************************************************************
* FUNCTION
*
*        IPSEC_Build_AH_Hdr
*
* DESCRIPTION
*
*        This function allocates a new parent buffer where it builds AH
*        header after leaving space for IP header.
*
* INPUTS
*       **buffer                List of buffers holding IPv4 or IPv6
*                               datagram.
*       *ip_layer               IPv4 or IPv6 header.
*       **ah_layer              AH header.
*       *out_sa                 Outgoing SA to be applied
*       ip_hlen                 IP header length
*
* OUTPUTS
*
*       NU_SUCCESS              On successful AH encoding.
*       IPSEC_INVAL_SEQ_NO      Bad sequence no.
*       NU_NO_BUFFERS           No buffers available.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Build_AH_Hdr(NET_BUFFER **buffer, VOID *ip_layer,
                          AHLAYER **ah_layer, IPSEC_OUTBOUND_SA *out_sa,
                          UINT16 ip_hlen)
{
    STATUS              status = NU_SUCCESS;
    UINT8               trans_prot;
    UINT16              auth_algo_id;
    NET_BUFFER          *hdr_buf;
    NET_BUFFER          *buf_ptr;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buffer   == NU_NULL) || (ip_layer == NU_NULL) ||
       (ah_layer == NU_NULL) || (out_sa   == NU_NULL) )
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /*Get the authentication algorithm index from out SA*/
    auth_algo_id = out_sa->ipsec_security.ipsec_auth_algo;

    buf_ptr = *buffer;

    /* Increment the sequence no. As it is unique for each AH header. */
    IPSEC_SEQ_INC(out_sa->ipsec_seq_num);

    /* Check the sequence number. If it is fine to transmit the current
       packet with incremented sequence number. */
    if( IPSEC_SEQ_HAS_CYCLED(out_sa->ipsec_seq_num) )
    {

#if (IPSEC_ANTI_REPLAY == NU_TRUE)

        /* The counter has been cycled so discard the packet and log
           the error. See RFC 2402 section 2.5. */
        NLOG_Error_Log("The SA sequence no. has been cycled",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

#endif

        /* Mark the status with error code. */
        status = IPSEC_INVAL_SEQ_NO;

    }
    else
    {
        /* Check if there is any space for AH buffer immediately after
           IP header i.e. if only IP header is present in the buffer. */
        if(buf_ptr->data_len != ip_hlen)
        {
            /* Get a new buffer for IP and AH buffer. */
            hdr_buf = IPSEC_Alloc_Buffer(buf_ptr, ip_layer, ip_hlen);

            /* If a buffer has been allocated, make it as a parent
               buffer and append it with the current buffer chain. */
            if(hdr_buf != NU_NULL)
            {

                /* Update the IP layer pointer to the new location. */
                ip_layer = hdr_buf->data_ptr;

                /* Set the data length of the buffer. */
                hdr_buf->data_len  = ip_hlen +
                                     IPSEC_AH_HDR_LEN(auth_algo_id);

                /* Increment the total data length of the buffer chain. */
                hdr_buf->mem_total_data_len +=
                                           IPSEC_AH_HDR_LEN(auth_algo_id);

                /* Also set the current buffer with the newly built parent
                   buffer. */
                *buffer = hdr_buf;

                /* Get a pointer to the AH header. */
                *ah_layer = (AHLAYER *)(hdr_buf->data_ptr + ip_hlen);
            }
            else
            {
                /* Unable to get the buffer. */
                status = NU_NO_BUFFERS;
            }
        }
        else
        {
            /* Get a pointer to the AH header. */
            *ah_layer = (AHLAYER *)(buf_ptr->data_ptr + ip_hlen);

            /* Set the data length of the buffer. */
            buf_ptr->data_len += IPSEC_AH_HDR_LEN(auth_algo_id);

            /* Increment the total data length of the buffer chain. */
            buf_ptr->mem_total_data_len += IPSEC_AH_HDR_LEN(auth_algo_id);
        }

        /* Check the status value before going ahead. */
        if(status == NU_SUCCESS)
        {

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
            /* Find the IP version. */
            if(((GET8(ip_layer, IP_VERSIONANDHDRLEN_OFFSET) & 0xf0) >> 4)
                                                            == IP_VERSION)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
                trans_prot = (UINT8) ((IPLAYER*)ip_layer)->ip_protocol;
#endif
#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
            else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                /* Need transport protocol value for IPv6 packet. */
                if(ip_hlen == IP6_HEADER_LEN)
                    trans_prot = (UINT8)GET8((buf_ptr->data_ptr ),
                                            IP6_NEXTHDR_OFFSET);
                else
                    trans_prot  = (UINT8)GET8((buf_ptr->data_ptr ),
                                            IP6_EXTHDR_NEXTHDR_OFFSET);
#endif

            /* Now make up the AH header. */
            PUT8(*ah_layer, IPSEC_AH_NHDR_OFFSET, trans_prot);

            /* Now set the payload length (length of header in
               32 bits words) minus 2(64 bits). This is so because AH
               is an IPv6 extension header. */
            PUT8(*ah_layer, IPSEC_AH_PLOADLEN_OFFSET,
                (UINT8)((IPSEC_AH_HDR_LEN(auth_algo_id) >> 2) - 2));

            /* The reserved field must be set to zero. */
            PUT16(*ah_layer, IPSEC_AH_RSVD_OFFSET, 0);

            /* Set the SPI from the given SA. */
            PUT32(*ah_layer, IPSEC_AH_SPI_OFFSET,
                                                out_sa->ipsec_remote_spi);

            /* Set the sequence no. */
            PUT32(*ah_layer, IPSEC_AH_SEQNO_OFFSET,
                   out_sa->ipsec_seq_num.ipsec_low_order);

            /* Clear the location where digest will be placed.
               This will be used in calculation of the digest. */
            UTL_Zero((((UINT8 *)*ah_layer) + IPSEC_AH_AUTHDATA_OFFSET),
                                IPSEC_GET_AUTH_DIGEST_LEN(auth_algo_id));
        }
    }

    /* Return the status. */
    return (status);

} /* IPSEC_Build_AH_Hdr */

#endif /* #if (IPSEC_INCLUDE_AH == NU_TRUE) */
