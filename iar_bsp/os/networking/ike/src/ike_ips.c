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
*       ike_ips.c
*
* COMPONENT
*
*       IKE - IPsec Specific
*
* DESCRIPTION
*
*       This file contains the IPsec specific functions used
*       by IKE.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_AH_Trans_ID_IPS_To_IKE
*       IKE_AH_Trans_ID_IKE_To_IPS
*       IKE_ESP_Trans_ID_IPS_To_IKE
*       IKE_ESP_Trans_ID_IKE_To_IPS
*       IKE_Auth_Algo_ID_IPS_To_IKE
*       IKE_Auth_Algo_ID_IKE_To_IPS
*       IKE_Protocol_ID_IPS_To_IKE
*       IKE_Protocol_ID_IKE_To_IPS
*       IKE_IPS_Selector_To_IKE
*       IKE_IPS_Selector_To_Remote_IP
*       IKE_IPS_Security_To_Remote_IP
*       IKE_IPS_Switch_Selector
*       IKE_IPS_Switch_Security
*       IKE_IPS_Group_Name_By_Packet
*       IKE_IPS_Group_Name_By_Device
*       IKE_IPS_ID_To_Selector
*       IKE_IPS_Selector_To_ID
*       IKE_IPS_Policy_By_ID
*       IKE_IPS_Phase2_Allowed
*       IKE_IPS_Get_Policy_Parameters
*       IKE_IPS_Get_Key_Length
*       IKE_IPS_Tunnel_Override
*       IKE_IPS_Generate_SA_Pairs
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_cfg.h
*       ips_externs.h
*       ips_api.h
*       ike_api.h
*       ike_ips.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_cfg.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_ips.h"

/* Local function prototypes. */
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
STATIC VOID IKE_IPS_Tunnel_Override(IKE_PHASE2_HANDLE *ph2,
                                    IPSEC_SECURITY_PROTOCOL *security);
#endif

/*************************************************************************
*
* FUNCTION
*
*       IKE_AH_Trans_ID_IPS_To_IKE
*
* DESCRIPTION
*
*       This function maps the IPsec AH authentication algorithm
*       ID to the IKE AH transform ID. It returns zero if no
*       matching ID is found. No valid transform ID could be zero
*       because it is RESERVED.
*
* INPUTS
*
*       ips_auth_algo           The IPsec authentication algorithm ID.
*
* OUTPUTS
*
*       The IKE transform ID equivalent to the specified
*       authentication algorithm ID, or 0 (RESERVED) on
*       failure.
*
*************************************************************************/
UINT8 IKE_AH_Trans_ID_IPS_To_IKE(UINT8 ips_auth_algo)
{
    UINT8           transform_id;

    /* Determine the IPsec algorithm. */
    switch(ips_auth_algo)
    {
#if (IPSEC_INCLUDE_MD5 == NU_TRUE)
    case IPSEC_HMAC_MD5_96:
        /* Set the IKE transform ID. */
        transform_id = IKE_IPS_TRANS_AH_MD5;
        break;
#endif

#if (IPSEC_INCLUDE_SHA1 == NU_TRUE)
    case IPSEC_HMAC_SHA_1_96:
        /* Set the IKE transform ID. */
        transform_id = IKE_IPS_TRANS_AH_SHA;
        break;
#endif

    default:
        /* Unrecognized IPsec algorithm ID. */
        transform_id = 0;
        break;
    }

    /* Return the transform ID. */
    return (transform_id);

} /* IKE_AH_Trans_ID_IPS_To_IKE */

/*************************************************************************
*
* FUNCTION
*
*       IKE_AH_Trans_ID_IKE_To_IPS
*
* DESCRIPTION
*
*       This function maps the IKE AH transform ID to the IPsec AH
*       authentication algorithm ID. It returns zero if no
*       matching ID is found.
*
* INPUTS
*
*       transform_id            The IKE transform ID.
*
* OUTPUTS
*
*       The authentication algorithm ID equivalent to the
*       specified IKE transform ID, or 0 on failure.
*
*************************************************************************/
UINT8 IKE_AH_Trans_ID_IKE_To_IPS(UINT8 transform_id)
{
    UINT8           ips_auth_algo;

    /* Determine the transform type. */
    switch(transform_id)
    {
#if (IPSEC_INCLUDE_MD5 == NU_TRUE)
    case IKE_IPS_TRANS_AH_MD5:
        /* Set the MD5 algorithm. */
        ips_auth_algo = IPSEC_HMAC_MD5_96;
        break;
#endif

#if (IPSEC_INCLUDE_SHA1 == NU_TRUE)
    case IKE_IPS_TRANS_AH_SHA:
        /* Set the SHA1 algorithm. */
        ips_auth_algo = IPSEC_HMAC_SHA_1_96;
        break;
#endif

    default:
        /* Unrecognized transform ID. */
        ips_auth_algo = 0;
        break;
    }

    /* Return the algorithm ID. */
    return (ips_auth_algo);

} /* IKE_AH_Trans_ID_IKE_To_IPS */

/*************************************************************************
*
* FUNCTION
*
*       IKE_ESP_Trans_ID_IPS_To_IKE
*
* DESCRIPTION
*
*       This function maps the IPsec ESP encryption algorithm
*       ID to the IKE transform ID. It returns zero if no matching
*       ID is found. No valid transform ID could be zero because
*       it is RESERVED.
*
* INPUTS
*
*       ips_encrypt_algo        The IPsec encryption algorithm ID.
*
* OUTPUTS
*
*       The IKE transform ID equivalent to the specified
*       encryption algorithm ID, or 0 (RESERVED) on
*       failure.
*
*************************************************************************/
UINT8 IKE_ESP_Trans_ID_IPS_To_IKE(UINT8 ips_encrypt_algo)
{
    UINT8           transform_id;

    /* Determine the IPsec encryption algorithm ID. */
    switch(ips_encrypt_algo)
    {
#if (IPSEC_INCLUDE_DES == NU_TRUE)
    case IPSEC_DES_CBC:
        /* Set the IKE transform ID. */
        transform_id = IKE_IPS_TRANS_ESP_DES;
        break;
#endif

#if (IPSEC_INCLUDE_3DES == NU_TRUE)
    case IPSEC_3DES_CBC:
        /* Set the IKE transform ID. */
        transform_id = IKE_IPS_TRANS_ESP_3DES;
        break;
#endif

#if (IPSEC_INCLUDE_CAST128 == NU_TRUE)
    case IPSEC_CAST_128:
        /* Set the IKE transform ID. */
        transform_id = IKE_IPS_TRANS_ESP_CAST;
        break;
#endif

#if (IPSEC_INCLUDE_BLOWFISH == NU_TRUE)
    case IPSEC_BLOWFISH_CBC:
        /* Set the IKE transform ID. */
        transform_id = IKE_IPS_TRANS_ESP_BLOWFISH;
        break;
#endif

#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)
    case IPSEC_NULL_CIPHER:
        /* Set the IKE transform ID. */
        transform_id = IKE_IPS_TRANS_ESP_NULL;
        break;
#endif

#if (IPSEC_INCLUDE_AES == NU_TRUE)
    case IPSEC_AES_CBC:
        /* Set the IKE transform ID. */
        transform_id = IKE_IPS_TRANS_ESP_AES;
        break;
#endif

    default:
        /* Unrecognized encryption algorithm ID. */
        transform_id = 0;
        break;
    }

    /* Return the transform ID. */
    return (transform_id);

} /* IKE_ESP_Trans_ID_IPS_To_IKE */

/*************************************************************************
*
* FUNCTION
*
*       IKE_ESP_Trans_ID_IKE_To_IPS
*
* DESCRIPTION
*
*       This function maps the IKE transform ID to the IPsec ESP
*       encryption algorithm. It returns zero if no matching
*       ID is found.
*
* INPUTS
*
*       transform_id            The IKE transform ID.
*
* OUTPUTS
*
*       The encryption algorithm ID equivalent to the specified
*       IKE transform ID, or 0 on failure.
*
*************************************************************************/
UINT8 IKE_ESP_Trans_ID_IKE_To_IPS(UINT8 transform_id)
{
    UINT8           ips_encrypt_algo;

    /* Determine the IKE transform ID. */
    switch(transform_id)
    {
#if (IPSEC_INCLUDE_DES == NU_TRUE)
    case IKE_IPS_TRANS_ESP_DES:
        /* Set the IPsec algorithm ID. */
        ips_encrypt_algo = IPSEC_DES_CBC;
        break;
#endif

#if (IPSEC_INCLUDE_3DES == NU_TRUE)
    case IKE_IPS_TRANS_ESP_3DES:
        /* Set the IPsec algorithm ID. */
        ips_encrypt_algo = IPSEC_3DES_CBC;
        break;
#endif

#if (IPSEC_INCLUDE_CAST128 == NU_TRUE)
    case IKE_IPS_TRANS_ESP_CAST:
        /* Set the IPsec algorithm ID. */
        ips_encrypt_algo = IPSEC_CAST_128;
        break;
#endif

#if (IPSEC_INCLUDE_BLOWFISH == NU_TRUE)
    case IKE_IPS_TRANS_ESP_BLOWFISH:
        /* Set the IPsec algorithm ID. */
        ips_encrypt_algo = IPSEC_BLOWFISH_CBC;
        break;
#endif

#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)
    case IKE_IPS_TRANS_ESP_NULL:
        /* Set the IPsec algorithm ID. */
        ips_encrypt_algo = IPSEC_NULL_CIPHER;
        break;
#endif

#if (IPSEC_INCLUDE_AES == NU_TRUE)
    case IKE_IPS_TRANS_ESP_AES:
        /* Set the IPsec algorithm ID. */
        ips_encrypt_algo = IPSEC_AES_CBC;
        break;
#endif

    default:
        /* Unrecognized IKE transform ID. */
        ips_encrypt_algo = 0;
        break;
    }

    /* Return the algorithm ID. */
    return (ips_encrypt_algo);

} /* IKE_ESP_Trans_ID_IKE_To_IPS */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Auth_Algo_ID_IPS_To_IKE
*
* DESCRIPTION
*
*       This function maps the IPsec authentication algorithm
*       ID to the IKE authentication algorithm ID. It returns
*       zero if no matching ID is found. No valid IKE algorithm
*       ID could be zero because it is RESERVED.
*
* INPUTS
*
*       ips_auth_algo           The IPsec authentication algorithm ID.
*
* OUTPUTS
*
*       The IKE authentication algorithm ID equivalent to
*       the specified IPsec authentication algorithm ID, or
*       0 (RESERVED) on failure.
*
*************************************************************************/
UINT16 IKE_Auth_Algo_ID_IPS_To_IKE(UINT8 ips_auth_algo)
{
    UINT16          auth_algo;

    /* Determine the algorithm type. */
    switch(ips_auth_algo)
    {
#if (IPSEC_INCLUDE_MD5 == NU_TRUE)
    case IPSEC_HMAC_MD5_96:
        /* Set the IKE algorithm ID. */
        auth_algo = IKE_IPS_VAL_HMAC_MD5;
        break;
#endif

#if (IPSEC_INCLUDE_SHA1 == NU_TRUE)
    case IPSEC_HMAC_SHA_1_96:
        /* Set the IKE algorithm ID. */
        auth_algo = IKE_IPS_VAL_HMAC_SHA;
        break;
#endif

    default:
        /* Unrecognized IPsec algorithm ID. */
        auth_algo = 0;
        break;
    }

    /* Return the IKE algorithm ID. */
    return (auth_algo);

} /* IKE_Auth_Algo_ID_IPS_To_IKE */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Auth_Algo_ID_IKE_To_IPS
*
* DESCRIPTION
*
*       This function maps the IKE authentication algorithm
*       ID to the IPsec authentication algorithm ID. It returns
*       zero if no matching ID is found.
*
* INPUTS
*
*       auth_algo               The IKE authentication algorithm ID.
*
* OUTPUTS
*
*       The IPsec authentication algorithm ID equivalent to
*       the specified IKE authentication algorithm ID, or
*       0 on failure.
*
*************************************************************************/
UINT8 IKE_Auth_Algo_ID_IKE_To_IPS(UINT16 auth_algo)
{
    UINT8           ips_auth_algo;

    /* Determine the algorithm type. */
    switch(auth_algo)
    {
#if (IPSEC_INCLUDE_MD5 == NU_TRUE)
    case IKE_IPS_VAL_HMAC_MD5:
        /* Set the IPsec algorithm ID. */
        ips_auth_algo = IPSEC_HMAC_MD5_96;
        break;
#endif

#if (IPSEC_INCLUDE_SHA1 == NU_TRUE)
    case IKE_IPS_VAL_HMAC_SHA:
        /* Set the IPsec algorithm ID. */
        ips_auth_algo = IPSEC_HMAC_SHA_1_96;
        break;
#endif

    default:
        /* Unrecognized IKE algorithm ID. */
        ips_auth_algo = 0;
        break;
    }

    /* Return the IPsec algorithm ID. */
    return (ips_auth_algo);

} /* IKE_Auth_Algo_ID_IKE_To_IPS */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Protocol_ID_IPS_To_IKE
*
* DESCRIPTION
*
*       This function maps the IPsec protocol to the IKE protocol
*       ID. It returns zero if no matching ID is found. No valid
*       protocol ID could be zero because it is RESERVED.
*
* INPUTS
*
*       ips_protocol            The IPsec protocol ID.
*
* OUTPUTS
*
*       The IKE protocol ID equivalent to the specified
*       IPsec protocol ID, or 0 (RESERVED) on failure.
*
*************************************************************************/
UINT8 IKE_Protocol_ID_IPS_To_IKE(UINT8 ips_protocol)
{
    UINT8           protocol_id;

    /* Determine the IPsec protocol. */
    switch(ips_protocol)
    {
#if (IPSEC_INCLUDE_AH == NU_TRUE)
    case IPSEC_AH:
        /* Set the IKE AH protocol ID. */
        protocol_id = IKE_PROTO_AH;
        break;
#endif

#if (IPSEC_INCLUDE_ESP == NU_TRUE)
    case IPSEC_ESP:
        /* Set the IKE ESP protocol ID. */
        protocol_id = IKE_PROTO_ESP;
        break;
#endif

    default:
        /* Unrecognized IPsec protocol ID. */
        protocol_id = 0;
        break;
    }

    /* Return the protocol ID. */
    return (protocol_id);

} /* IKE_Protocol_ID_IPS_To_IKE */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Protocol_ID_IKE_To_IPS
*
* DESCRIPTION
*
*       This function maps the IKE protocol to the IPsec protocol
*       ID. It returns zero if no matching ID is found.
*
* INPUTS
*
*       protocol_id             The IKE protocol ID.
*
* OUTPUTS
*
*       The IPsec protocol ID equivalent to the specified
*       IKE protocol ID, or 0 on failure.
*
*************************************************************************/
UINT8 IKE_Protocol_ID_IKE_To_IPS(UINT8 protocol_id)
{
    UINT8           ips_protocol;

    /* Determine the protocol. */
    switch(protocol_id)
    {
#if (IPSEC_INCLUDE_AH == NU_TRUE)
    case IKE_PROTO_AH:
        /* Set the IPsec AH protocol ID. */
        ips_protocol = IPSEC_AH;
        break;
#endif

#if (IPSEC_INCLUDE_ESP == NU_TRUE)
    case IKE_PROTO_ESP:
        /* Set the IPsec ESP protocol ID. */
        ips_protocol = IPSEC_ESP;
        break;
#endif

    default:
        /* Unrecognized IKE protocol ID. */
        ips_protocol = 0;
        break;
    }

    /* Return the protocol ID. */
    return (ips_protocol);

} /* IKE_Protocol_ID_IKE_To_IPS */

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Selector_To_IKE
*
* DESCRIPTION
*
*       This is a utility function to convert an IPsec selector
*       to an IKE selector. The source address in the IPsec
*       selector is ignored. Only the destination address is
*       copied.
*
* INPUTS
*
*       *ips_select             Pointer to an IPsec selector.
*       *ike_select             On return, this contains an IKE
*                               selector equivalent to the IPsec
*                               selector.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_IPS_Selector_To_IKE(IPSEC_SELECTOR *ips_select,
                             IKE_POLICY_SELECTOR *ike_select)
{
    /* Determine IPsec selector address type. */
    switch(ips_select->ipsec_dest_type)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
    case (IPSEC_IPV4 | IPSEC_SINGLE_IP):
        /* Set the address type and copy the IP address. */
        ike_select->ike_type = IKE_IPV4;

        NU_BLOCK_COPY(ike_select->ike_addr.ike_ip.ike_addr1,
                      ips_select->ipsec_dest_ip.ipsec_addr,
                      IP_ADDR_LEN);
        break;

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    case (IPSEC_IPV4 | IPSEC_SUBNET_IP):
        /* Set the address type and copy the IP addresses. */
        ike_select->ike_type = IKE_IPV4_SUBNET;

        NU_BLOCK_COPY(ike_select->ike_addr.ike_ip.ike_addr1,
                      ips_select->ipsec_dest_ip.ipsec_addr,
                      IP_ADDR_LEN);
        NU_BLOCK_COPY(ike_select->ike_addr.ike_ip.ike_ext_addr.ike_addr2,
                      ips_select->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2,
                      IP_ADDR_LEN);
        break;

    case (IPSEC_IPV4 | IPSEC_RANGE_IP):
        /* Set the address type and copy the IP addresses. */
        ike_select->ike_type = IKE_IPV4_RANGE;

        NU_BLOCK_COPY(ike_select->ike_addr.ike_ip.ike_addr1,
                      ips_select->ipsec_dest_ip.ipsec_addr,
                      IP_ADDR_LEN);
        NU_BLOCK_COPY(ike_select->ike_addr.ike_ip.ike_ext_addr.ike_addr2,
                      ips_select->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2,
                      IP_ADDR_LEN);
        break;
#endif /* (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE) */
#endif /* (INCLUDE_IPV4 == NU_TRUE) */

#if (INCLUDE_IPV6 == NU_TRUE)
    case (IPSEC_IPV6 | IPSEC_SINGLE_IP):
        /* Set the address type and copy the IP address. */
        ike_select->ike_type = IKE_IPV6;

        NU_BLOCK_COPY(ike_select->ike_addr.ike_ip.ike_addr1,
                      ips_select->ipsec_dest_ip.ipsec_addr,
                      IP6_ADDR_LEN);
        break;

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    case (IPSEC_IPV6 | IPSEC_SUBNET_IP):
        /* Set the address type and copy the IP addresses. */
        ike_select->ike_type = IKE_IPV6_SUBNET;

        NU_BLOCK_COPY(ike_select->ike_addr.ike_ip.ike_addr1,
                      ips_select->ipsec_dest_ip.ipsec_addr,
                      IP6_ADDR_LEN);
        NU_BLOCK_COPY(ike_select->ike_addr.ike_ip.ike_ext_addr.ike_addr2,
                      ips_select->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2,
                      IP6_ADDR_LEN);
        break;

    case (IPSEC_IPV6 | IPSEC_RANGE_IP):
        /* Set the address type and copy the IP addresses. */
        ike_select->ike_type = IKE_IPV6_RANGE;

        NU_BLOCK_COPY(ike_select->ike_addr.ike_ip.ike_addr1,
                      ips_select->ipsec_dest_ip.ipsec_addr,
                      IP6_ADDR_LEN);
        NU_BLOCK_COPY(ike_select->ike_addr.ike_ip.ike_ext_addr.ike_addr2,
                      ips_select->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2,
                      IP6_ADDR_LEN);
        break;
#endif /* (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE) */
#endif /* (INCLUDE_IPV6 == NU_TRUE) */

    case IPSEC_WILDCARD:
    default:
        /* Un-allowed selector types for IPsec selectors. */
        break;
    }
} /* IKE_IPS_Selector_To_IKE */

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Selector_To_Remote_IP
*
* DESCRIPTION
*
*       This is a utility function used to convert an IPsec
*       selector, provided in the initiate request, to an
*       IP address (struct addr_struct) of the remote node
*       with which the IKE negotiation is to be carried out.
*
* INPUTS
*
*       *ips_select             Pointer to an IPsec selector.
*       *ike_addr               On return, this contains an IP
*                               address equivalent to the IPsec
*                               selector.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_IPS_Selector_To_Remote_IP(IPSEC_SELECTOR *ips_select,
                                   struct addr_struct *ike_addr)
{
    /* Set the name field in the address. */
    ike_addr->name = "IKE_Remote";

    /* Set destination port. */
    ike_addr->port = IKE_SEND_UDP_PORT;

    /* Set IP family, based on the IPsec selector type. */
    ike_addr->family =
        IKE_IPS_FLAGS_TO_FAMILY(ips_select->ipsec_dest_type);

    /* Copy destination address from the IPsec selector to
     * the IP address field.
     */
    NU_BLOCK_COPY(ike_addr->id.is_ip_addrs,
                  ips_select->ipsec_dest_ip.ipsec_addr,
                  IKE_IP_LEN(ike_addr->family));

} /* IKE_IPS_Selector_To_Remote_IP */

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Security_To_Remote_IP
*
* DESCRIPTION
*
*       This is a utility function used to obtain the remote
*       node address from the IPsec security protocol, provided
*       in the initiate request. This is the tunnel destination
*       address which is only provided in tunnel mode.
*
* INPUTS
*
*       *ips_security           Pointer to an IPsec security
*                               protocol structure.
*       *ike_addr               On return, this contains an IP
*                               address equivalent to the IPsec
*                               selector.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_IPS_Security_To_Remote_IP(IPSEC_SECURITY_PROTOCOL *ips_security,
                                   struct addr_struct *ike_addr)
{
    /* Set the name field in the address. */
    ike_addr->name = "IKE_Remote";

    /* Set destination port. */
    ike_addr->port = IKE_SEND_UDP_PORT;

    /* Set IP family, based on the IPsec selector type. */
    ike_addr->family =
        IKE_IPS_FLAGS_TO_FAMILY(ips_security->ipsec_flags);

    /* Copy tunnel destination address from the IPsec security
     * protocol structure to the IP address field.
     */
    NU_BLOCK_COPY(ike_addr->id.is_ip_addrs,
                  ips_security->ipsec_tunnel_destination,
                  IKE_IP_LEN(ike_addr->family));

} /* IKE_IPS_Security_To_Remote_IP */
#endif

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Switch_Selector
*
* DESCRIPTION
*
*       This is a utility function used to switch the source
*       and destination addresses in selector 'a' with respect
*       to selector 'b'. This is required because selectors
*       within IPsec inbound and outbound SAs have an opposite
*       order of source and destination addresses. Selector 'b'
*       is copied to selector 'a' with the source and
*       destination addresses switched.
*
* INPUTS
*
*       *select_a               Destination selector.
*       *select_b               Source selector which is to
*                               be switched.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_IPS_Switch_Selector(IPSEC_SELECTOR *select_a,
                             IPSEC_SELECTOR *select_b)
{
    /* Copy selector destination to selector source. */
    NU_BLOCK_COPY(&select_a->ipsec_dest_ip, &select_b->ipsec_source_ip,
                  sizeof(select_a->ipsec_dest_ip));

    /* Copy selector source to selector destination. */
    NU_BLOCK_COPY(&select_a->ipsec_source_ip, &select_b->ipsec_dest_ip,
                  sizeof(select_a->ipsec_source_ip));

    /* Also switch the type and port fields. */
    select_a->ipsec_dest_type        = select_b->ipsec_source_type;
    select_a->ipsec_destination_port = select_b->ipsec_source_port;
    select_a->ipsec_source_type      = select_b->ipsec_dest_type;
    select_a->ipsec_source_port      = select_b->ipsec_destination_port;

} /* IKE_IPS_Switch_Selector */

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Switch_Security
*
* DESCRIPTION
*
*       This is a utility function used to switch tunnel source
*       and destination addresses in security protocol 'a' with
*       respect to 'b'. This is required because selectors
*       within IPsec inbound and outbound SAs have an opposite
*       order of source and destination addresses.
*
* INPUTS
*
*       *secure_a               Destination security protocol.
*       *secure_b               Source security protocol which
*                               is to be switched.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_IPS_Switch_Security(IPSEC_SECURITY_PROTOCOL *secure_a,
                             IPSEC_SECURITY_PROTOCOL *secure_b)
{
    /* Copy tunnel source to the tunnel destination field. */
    NU_BLOCK_COPY(secure_a->ipsec_tunnel_destination,
                  secure_b->ipsec_tunnel_source,
                  sizeof(secure_a->ipsec_tunnel_destination));

    /* Copy tunnel destination to the tunnel source field.*/
    NU_BLOCK_COPY(secure_a->ipsec_tunnel_source,
                  secure_b->ipsec_tunnel_destination,
                  sizeof(secure_a->ipsec_tunnel_destination));

} /* IKE_IPS_Switch_Security */
#endif

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Group_Name_By_Packet
*
* DESCRIPTION
*
*       This is a utility function which returns the IPsec
*       group name containing the specified interface. The
*       interface is looked up using its IP address or in
*       case of IPv6, the device index. The caller must
*       provide a buffer large enough for storing the group
*       name.
*
* INPUTS
*
*       *pkt                    Packet for which the group
*                               name is being looked-up.
*       *group_name             Buffer for storing the group name.
*                               On return, this contains the group
*                               name of the IPsec group being
*                               searched.
*       *total_len              Length of the buffer for storing
*                               the returned group name, including
*                               the NULL terminator. On return,
*                               this contains the length of the
*                               group name string.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_NOT_FOUND           Device not found or it does
*                               not contain an IPsec group.
*       IKE_LENGTH_IS_SHORT     Size of return buffer is not
*                               large enough to hold data.
*
*************************************************************************/
STATUS IKE_IPS_Group_Name_By_Packet(IKE_PACKET *pkt, CHAR *group_name,
                                    UINT32 *total_len)
{
    STATUS              status;
    UINT32              req_len;
    IPSEC_POLICY_GROUP  *ips_group;
    DV_DEVICE_ENTRY     *dev_entry;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((pkt       == NU_NULL) || (group_name == NU_NULL) ||
       (total_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Grab the NET semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Initialize group pointer to NULL. */
        ips_group = NU_NULL;

        /* Grab the IPsec semaphore because the IPsec group
         * pointer in the device structure needs to be accessed.
         */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* Get device by index. */
            dev_entry = DEV_Get_Dev_By_Index(pkt->ike_if_index);

            if(dev_entry != NU_NULL)
            {
                /* Get the IPsec group pointer. */
                ips_group = dev_entry->dev_physical->dev_phy_ips_group;
            }

            /* If group is still NULL then return error. */
            if(ips_group == NU_NULL)
            {
                /* Set error status. */
                status = IKE_NOT_FOUND;
            }

            else
            {
                /* Get length of the group name. */
                req_len = strlen(ips_group->ipsec_group_name) + 1;

                if(req_len > (*total_len))
                {
                    /* Length of the group name buffer is too short. */
                    status = IKE_LENGTH_IS_SHORT;
                }

                else
                {
                    /* Copy the name to the buffer. */
                    strcpy(group_name, ips_group->ipsec_group_name);
                }

                /* Return the length of the group name. */
                *total_len = req_len;
            }

            /* Release the IPsec semaphore. */
            if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to obtain IPsec semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Release the NET semaphore. */
        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain NET semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_IPS_Group_Name_By_Packet */

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Group_Name_By_Device
*
* DESCRIPTION
*
*       This is a utility function which returns the IPsec
*       group name containing the specified interface. The
*       interface is looked up using the device index.
*       The caller must provide a buffer large enough for
*       storing the group name.
*
* INPUTS
*
*       dev_index               Index of device which is to be
*                               searched.
*       *group_name             Buffer for storing the group name.
*                               On return, this contains the group
*                               name of the IPsec group.
*       *total_len              Length of the buffer for storing
*                               the returned group name, including
*                               the NULL terminator. On return,
*                               this contains the length of the
*                               group name string.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_NOT_FOUND           Device not found or it does
*                               not contain an IPsec group.
*       IKE_LENGTH_IS_SHORT     Size of return buffer is not
*                               large enough to hold data.
*
*************************************************************************/
STATUS IKE_IPS_Group_Name_By_Device(UINT32 dev_index, CHAR *group_name,
                                    UINT32 *total_len)
{
    STATUS              status;
    UINT32              req_len;
    IPSEC_POLICY_GROUP  *ips_group;
    DV_DEVICE_ENTRY     *dev_entry;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((group_name == NU_NULL) || (total_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Grab the NET semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Grab the IPsec semaphore because the IPsec group
         * pointer in the device structure needs to be accessed.
         */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* Get device by index. */
            dev_entry = DEV_Get_Dev_By_Index(dev_index);

            if(dev_entry == NU_NULL)
            {
                /* Device not found. */
                status = IKE_NOT_FOUND;
            }

            else if(dev_entry->dev_physical->dev_phy_ips_group == NU_NULL)
            {
                /* Device is not part of an IPsec group. */
                status = IKE_NOT_FOUND;
            }

            else
            {
                /* Get the IPsec group pointer. */
                ips_group = dev_entry->dev_physical->dev_phy_ips_group;

                /* Get length of the group name. */
                req_len = strlen(ips_group->ipsec_group_name) + 1;

                if(req_len > (*total_len))
                {
                    /* Length of the group name buffer is too short. */
                    status = IKE_LENGTH_IS_SHORT;
                }

                else
                {
                    /* Copy the name to the buffer. */
                    strcpy(group_name, ips_group->ipsec_group_name);
                }

                /* Return the length of the group name. */
                *total_len = req_len;
            }

            /* Release the IPsec semaphore. */
            if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to obtain IPsec semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Release the NET semaphore. */
        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain NET semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_IPS_Group_Name_By_Device */

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_ID_To_Selector
*
* DESCRIPTION
*
*       This is a utility function which converts an IKE
*       Identification payload to an equivalent IPsec
*       Selector. If the Identification payload's type
*       is IKE_WILDCARD, then an absolute IP address is used
*       instead. IKE_DOMAIN_NAME and IKE_USER_DOMAIN_NAME
*       address types are not allowed in the Identification
*       payload passed to this function.
*
* INPUTS
*
*       *id                     Identification payload.
*       *abs_addr               Pointer to an IP address.
*                               Used if ID type is IKE_WILDCARD.
*       *select                 On return, this contains the
*                               equivalent IPsec selector.
*       side                    If IKE_LOCAL, then the source
*                               parameters of the Selector
*                               are filled in and if IKE_REMOTE,
*                               then the destination
*                               parameters are filled in.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_UNSUPPORTED_IDTYPE  Identification type not
*                               supported by IKE.
*
*************************************************************************/
STATUS IKE_IPS_ID_To_Selector(IKE_ID_DEC_PAYLOAD *id,
                              struct addr_struct *abs_addr,
                              IPSEC_SELECTOR *select, UINT8 side)
{
    STATUS          status = NU_SUCCESS;
    UINT8           *type;
    UINT8           *ip_addr1;
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    UINT8           *ip_addr2;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((id == NU_NULL) || (abs_addr == NU_NULL) || (select == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the 'side' parameter is valid. */
    else if((side != IKE_LOCAL) && (side != IKE_REMOTE))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set the transport protocol. */
    select->ipsec_transport_protocol = id->ike_protocol_id;

    /* If the source side is to be filled. */
    if(side == IKE_LOCAL)
    {
        /* Set the source port. */
        select->ipsec_source_port = id->ike_port;

        /* Point all variables to the source side. */
        type     = &select->ipsec_source_type;
        ip_addr1 = select->ipsec_source_ip.ipsec_addr;
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
        ip_addr2 = select->ipsec_source_ip.ipsec_ext_addr.ipsec_addr2;
#endif
    }

    /* Otherwise the destination is to be filled. */
    else
    {
        /* Set the destination port. */
        select->ipsec_destination_port = id->ike_port;

        /* Point all variables to the destination side. */
        type     = &select->ipsec_dest_type;
        ip_addr1 = select->ipsec_dest_ip.ipsec_addr;
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
        ip_addr2 = select->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2;
#endif
    }

    /* Determine Identification payload type. */
    switch(id->ike_id_type)
    {
    case IKE_WILDCARD:
        /* Set type of IP address. */
        *type = (UINT8)(IPSEC_SINGLE_IP |
                 IKE_IPS_FAMILY_TO_FLAGS(abs_addr->family));

        /* Copy the IP address. */
        NU_BLOCK_COPY(ip_addr1, abs_addr->id.is_ip_addrs,
                      IKE_IP_LEN(abs_addr->family));
        break;

#if (INCLUDE_IPV4 == NU_TRUE)
    case IKE_IPV4:
        /* Set the address type and copy the IP address. */
        *type = IPSEC_IPV4 | IPSEC_SINGLE_IP;
        NU_BLOCK_COPY(ip_addr1, id->ike_id_data, IP_ADDR_LEN);
        break;

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    case IKE_IPV4_SUBNET:
        /* Set the address type and copy the IP addresses. */
        *type = IPSEC_IPV4 | IPSEC_SUBNET_IP;

        NU_BLOCK_COPY(ip_addr1, id->ike_id_data, IP_ADDR_LEN);
        NU_BLOCK_COPY(ip_addr2, id->ike_id_data + IP_ADDR_LEN,
                      IP_ADDR_LEN);
        break;

    case IKE_IPV4_RANGE:
        /* Set the address type and copy the IP addresses. */
        *type = IPSEC_IPV4 | IPSEC_RANGE_IP;

        NU_BLOCK_COPY(ip_addr1, id->ike_id_data, IP_ADDR_LEN);
        NU_BLOCK_COPY(ip_addr2, id->ike_id_data + IP_ADDR_LEN,
                      IP_ADDR_LEN);
        break;
#endif /* (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE) */
#endif /* (INCLUDE_IPV4 == NU_TRUE) */

#if (INCLUDE_IPV6 == NU_TRUE)
    case IKE_IPV6:
        /* Set the address type and copy the IP address. */
        *type = IPSEC_IPV6 | IPSEC_SINGLE_IP;
        NU_BLOCK_COPY(ip_addr1, id->ike_id_data, IP6_ADDR_LEN);
        break;

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    case IKE_IPV6_SUBNET:
        /* Set the address type and copy the IP addresses. */
        *type = IPSEC_IPV6 | IPSEC_SUBNET_IP;

        NU_BLOCK_COPY(ip_addr1, id->ike_id_data, IP6_ADDR_LEN);
        NU_BLOCK_COPY(ip_addr2, id->ike_id_data + IP6_ADDR_LEN,
                      IP6_ADDR_LEN);
        break;

    case IKE_IPV6_RANGE:
        /* Set the address type and copy the IP addresses. */
        *type = IPSEC_IPV6 | IPSEC_RANGE_IP;

        NU_BLOCK_COPY(ip_addr1, id->ike_id_data, IP6_ADDR_LEN);
        NU_BLOCK_COPY(ip_addr2, id->ike_id_data + IP6_ADDR_LEN,
                      IP6_ADDR_LEN);
        break;
#endif /* (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE) */
#endif /* (INCLUDE_IPV6 == NU_TRUE) */

    case IKE_DOMAIN_NAME:
    case IKE_USER_DOMAIN_NAME:
    default:
        /* Unsupported selector types for IPsec selectors. */
        status = IKE_UNSUPPORTED_IDTYPE;
        break;
    }

    /* Return the status. */
    return (status);

} /* IKE_IPS_ID_To_Selector */

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Selector_To_ID
*
* DESCRIPTION
*
*       This is a utility function which converts an IPsec
*       Selector to an equivalent Identification payload.
*       The Selector source and destination types must not
*       be set to WILDCARD or DOMAIN NAME.
*
* INPUTS
*
*       *select                 Pointer to the IPsec Selector.
*       *id                     On return, this contains an
*                               equivalent Identification payload.
*       side                    If IKE_LOCAL, then the source
*                               parameters of the Selector
*                               are filled in and if IKE_REMOTE,
*                               then the destination
*                               parameters are filled in.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_SELECTOR    Address type in selector is
*                               invalid.
*
*************************************************************************/
STATUS IKE_IPS_Selector_To_ID(IPSEC_SELECTOR *select,
                              IKE_ID_ENC_PAYLOAD *id, UINT8 side)
{
    STATUS          status = NU_SUCCESS;
    UINT8           *type;
    UINT8           *ip_addr1;
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    UINT8           *ip_addr2;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((select == NU_NULL) || (id == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the 'side' parameter is valid. */
    else if((side != IKE_LOCAL) && (side != IKE_REMOTE))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set the transport protocol. */
    id->ike_protocol_id = (UINT8)select->ipsec_transport_protocol;

    /* If the source side is to be filled. */
    if(side == IKE_LOCAL)
    {
        /* Set the source port. */
        id->ike_port = select->ipsec_source_port;

        /* Point all variables to the source side. */
        type     = &select->ipsec_source_type;
        ip_addr1 = select->ipsec_source_ip.ipsec_addr;
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
        ip_addr2 = select->ipsec_source_ip.ipsec_ext_addr.ipsec_addr2;
#endif
    }

    /* Otherwise the destination is to be filled. */
    else
    {
        /* Set the destination port. */
        id->ike_port = select->ipsec_destination_port;

        /* Point all variables to the destination side. */
        type     = &select->ipsec_dest_type;
        ip_addr1 = select->ipsec_dest_ip.ipsec_addr;
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
        ip_addr2 = select->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2;
#endif
    }

    /* Selector address type. */
    switch(*type)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
    case (IPSEC_IPV4 | IPSEC_SINGLE_IP):
        /* Set the address type and copy the IP address. */
        id->ike_id_type = IKE_IPV4;
        id->ike_id_data_len = IP_ADDR_LEN;
        NU_BLOCK_COPY(id->ike_id_data, ip_addr1, IP_ADDR_LEN);
        break;

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    case (IPSEC_IPV4 | IPSEC_SUBNET_IP):
        /* Set the address type and copy the IP addresses. */
        id->ike_id_type = IKE_IPV4_SUBNET;
        id->ike_id_data_len = IP_ADDR_LEN * 2;

        NU_BLOCK_COPY(id->ike_id_data, ip_addr1, IP_ADDR_LEN);
        NU_BLOCK_COPY(id->ike_id_data + IP_ADDR_LEN,
                      ip_addr2, IP_ADDR_LEN);
        break;

    case (IPSEC_IPV4 | IPSEC_RANGE_IP):
        /* Set the address type and copy the IP addresses. */
        id->ike_id_type = IKE_IPV4_RANGE;
        id->ike_id_data_len = IP_ADDR_LEN * 2;

        NU_BLOCK_COPY(id->ike_id_data, ip_addr1, IP_ADDR_LEN);
        NU_BLOCK_COPY(id->ike_id_data + IP_ADDR_LEN,
                      ip_addr2, IP_ADDR_LEN);
        break;
#endif /* (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE) */
#endif /* (INCLUDE_IPV4 == NU_TRUE) */

#if (INCLUDE_IPV6 == NU_TRUE)
    case (IPSEC_IPV6 | IPSEC_SINGLE_IP):
        /* Set the address type and copy the IP address. */
        id->ike_id_type = IKE_IPV6;
        id->ike_id_data_len = IP6_ADDR_LEN;
        NU_BLOCK_COPY(id->ike_id_data, ip_addr1, IP6_ADDR_LEN);
        break;

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    case (IPSEC_IPV6 | IPSEC_SUBNET_IP):
        /* Set the address type and copy the IP addresses. */
        id->ike_id_type = IKE_IPV6_SUBNET;
        id->ike_id_data_len = IP6_ADDR_LEN * 2;

        NU_BLOCK_COPY(id->ike_id_data, ip_addr1, IP6_ADDR_LEN);
        NU_BLOCK_COPY(id->ike_id_data + IP6_ADDR_LEN,
                      ip_addr2, IP6_ADDR_LEN);
        break;

    case (IPSEC_IPV6 | IPSEC_RANGE_IP):
        /* Set the address type and copy the IP addresses. */
        id->ike_id_type = IKE_IPV6_RANGE;
        id->ike_id_data_len = IP6_ADDR_LEN * 2;

        NU_BLOCK_COPY(id->ike_id_data, ip_addr1, IP6_ADDR_LEN);
        NU_BLOCK_COPY(id->ike_id_data + IP6_ADDR_LEN,
                      ip_addr2, IP6_ADDR_LEN);
        break;
#endif /* (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE) */
#endif /* (INCLUDE_IPV6 == NU_TRUE) */

    case IPSEC_WILDCARD:
    default:
        /* Un-allowed selector types for IPsec selectors. */
        status = IKE_INVALID_SELECTOR;
        break;
    }

    /* Return the status. */
    return (status);

} /* IKE_IPS_Selector_To_ID */

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Policy_By_ID
*
* DESCRIPTION
*
*       This is a utility function used to look up an
*       IPsec policy using the Identification payloads
*       received by the Responder. This function must
*       only be called by the Responder.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*       *id_i                   Initiator's ID payload.
*       *id_r                   Responder's ID payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_NOT_FOUND         In case policy is not found.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_UNSUPPORTED_IDTYPE  Identification type not
*                               supported by IKE.
*       IKE_NOT_FOUND           Device not found or it does
*                               not contain an IPsec group.
*
*************************************************************************/
STATUS IKE_IPS_Policy_By_ID(IKE_PHASE2_HANDLE *ph2,
                            IKE_ID_DEC_PAYLOAD *id_i,
                            IKE_ID_DEC_PAYLOAD *id_r)
{
    STATUS          status;
    UINT32          buffer_len;
    IKE_PACKET      *pkt;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((ph2 == NU_NULL) || (id_i == NU_NULL) || (id_r == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Looking up IPsec policy by selector");

    /* Set local pointer to commonly used data in the Handle. */
    pkt = ph2->ike_params->ike_packet;

    /* Set the Initiator's (source) payload data in Selector. */
    status = IKE_IPS_ID_To_Selector(id_i, &ph2->ike_node_addr,
                                    &ph2->ike_ips_select, IKE_LOCAL);

    if(status == NU_SUCCESS)
    {
        /* Set the Responder's (destination) payload data in Selector. */
        status = IKE_IPS_ID_To_Selector(id_r, &pkt->ike_local_addr,
                                        &ph2->ike_ips_select, IKE_REMOTE);

        if(status == NU_SUCCESS)
        {
            buffer_len = sizeof(ph2->ike_ips_group_name);

            /* Get the IPsec group name. */
            status = IKE_IPS_Group_Name_By_Packet(pkt,
                         ph2->ike_ips_group_name, &buffer_len);

            if(status == NU_SUCCESS)
            {
                /* Get the IPsec Policy using the IPsec Selector. */
                status = IPSEC_Get_Policy_Index(ph2->ike_ips_group_name,
                             &ph2->ike_ips_select, IPSEC_INBOUND,
                             &ph2->ike_ips_policy_index);
            }

            else
            {
                NLOG_Error_Log("Unable to get IPsec group name by packet",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Unable to set destination side in selector",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Unable to set source side in selector",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_IPS_Policy_By_ID */

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Phase2_Allowed
*
* DESCRIPTION
*
*       This function checks whether a phase 2 exchange is
*       allowed by the IKE policy. This is an additional
*       check performed independent of the IPsec policy.
*
* INPUTS
*
*       *policy                 Pointer to the IKE policy.
*       *ips_select             Pointer to phase 2 IPsec selector.
*
* OUTPUTS
*
*       NU_SUCCESS              If exchange is allowed.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_UNALLOWED_XCHG      If exchange not allowed.
*
*************************************************************************/
STATUS IKE_IPS_Phase2_Allowed(IKE_POLICY *policy,
                              IPSEC_SELECTOR *ips_select)
{
    STATUS          status = IKE_UNALLOWED_XCHG;
    INT             i;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((policy == NU_NULL) || (ips_select == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Loop for all phase 2 IDs in the IKE policy. */
    for(i = 0; i < policy->ike_ids_no; i++)
    {
        /* Check if the protocol matches. */
        if((policy->ike_ids[i].ike_protocol_id == IKE_WILDCARD) ||
           ((UINT32)policy->ike_ids[i].ike_protocol_id ==
            ips_select->ipsec_transport_protocol))
        {
            /* Check if the port matches. */
            if((policy->ike_ids[i].ike_port == IKE_WILDCARD) ||
               (policy->ike_ids[i].ike_port ==
                ips_select->ipsec_destination_port))
            {
                /* Valid match found. */
                status = NU_SUCCESS;

                /* Break out of search loop. */
                break;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_IPS_Phase2_Allowed */

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Get_Policy_Parameters
*
* DESCRIPTION
*
*       This is a utility function which looks up the IPsec
*       security policy using the specified index and then
*       allocates a buffer and returns the Policy's array
*       of security protocols. It also fills in the SA
*       lifetime and PFS group description fields in the
*       Phase 2 Handle from the IPsec policy.
*
* INPUTS
*
*       *phase2                 Phase 2 Handle. This must contain
*                               valid IPsec group name and policy
*                               index. On return, contains the
*                               IPsec SA lifetime and PRF group
*                               description.
*       **security              On return, this contains a
*                               pointer to the buffer containing
*                               the security protocols.
*       *security_size          On return, this contains the
*                               number of items in the array.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_NOT_FOUND         The group was not found or
*                               there is no apply policy
*                               corresponding to the passed index.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_IPS_Get_Policy_Parameters(IKE_PHASE2_HANDLE *phase2,
                                     IPSEC_SECURITY_PROTOCOL **security,
                                     UINT8 *security_size)
{
    STATUS                  status;
    IPSEC_POLICY_GROUP      *group;
    IPSEC_POLICY            *policy;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((phase2        == NU_NULL) || (security == NU_NULL) ||
       (security_size == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

     /* Log debug message. */
    IKE_DEBUG_LOG("Getting IPsec policy parameters used in phase 2");

    /* First grab the IPsec semaphore. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, IKE_TIMEOUT);

    /* Check the status value. */
    if(status == NU_SUCCESS)
    {
        /* Get the group entry pointer. */
        status = IPSEC_Get_Group_Entry(phase2->ike_ips_group_name,
                                       &group);

        /* Check the status value. */
        if(status == NU_SUCCESS)
        {
            /* Get the IPsec policy entry. */
            status = IPSEC_Get_Policy_Entry(group,
                                            phase2->ike_ips_policy_index,
                                            &policy);

            if(status == NU_SUCCESS)
            {
                /* Make sure action of this policy is apply. */
                if((policy->ipsec_flags & IPSEC_APPLY) == 0)
                {
                    NLOG_Error_Log("IPsec policy has bypass action instead\
 of apply - exchange not possible", NERR_RECOVERABLE, __FILE__, __LINE__);

                    status = IKE_INVALID_PARAMS;
                }

                else
                {
                    /* Get lifetime from the IPsec policy. */
                    NU_BLOCK_COPY(&phase2->ike_ips_lifetime,
                                  &policy->ipsec_sa_max_lifetime,
                                  sizeof(IPSEC_SA_LIFETIME));

                    /* Get PFS group description from the IPsec policy. */
                    phase2->ike_group_desc = policy->ipsec_pfs_group_desc;

                    /* Get IPsec policy flags. */
                    phase2->ike_sa2_db.ike_ips_flags = policy->ipsec_flags;

                    /* Allocate memory for the security protocol array. */
                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                (VOID**)security,
                                (policy->ipsec_security_size *
                                sizeof(IPSEC_SECURITY_PROTOCOL)),
                                NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        /* Normalize the pointer. */
                        *security = TLS_Normalize_Ptr(*security);

                        /* Set number of items in security array. */
                        *security_size = policy->ipsec_security_size;

                        /* Get security protocols from the IPsec Policy. */
                        NU_BLOCK_COPY(*security, policy->ipsec_security,
                                      (policy->ipsec_security_size *
                                      sizeof(IPSEC_SECURITY_PROTOCOL)));
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to allocate memory",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }
        }

        /* Now everything is done, release the semaphore too. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_IPS_Get_Policy_Parameters */

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Get_Key_Length
*
* DESCRIPTION
*
*       This function returns the length of the negotiated
*       IPsec encryption and authentication algorithms. If
*       either of the two algorithms does not apply, its
*       corresponding key length is returned as zero.
*
* INPUTS
*
*       *security               Pointer to IPsec security
*                               protocol containing negotiated
*                               algorithms.
*       *enc_key_len            On return, contains the
*                               encryption key length.
*       *auth_key_len           On return, contains the
*                               authentication key length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IPSEC_INVALID_ALGO_ID   IPsec algorithm ID not valid in
*                               the negotiated algorithms.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_IPS_Get_Key_Length(IPSEC_SECURITY_PROTOCOL *security,
                              UINT16 *enc_key_len, UINT16 *auth_key_len)
{
    STATUS          status;
    UINT8           algo;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((security     == NU_NULL) || (enc_key_len == NU_NULL) ||
       (auth_key_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Grab the IPsec semaphore. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Set the authentication algorithm ID. */
        algo = security->ipsec_auth_algo;

        /* Convert the algorithm ID to array index. */
        status = IPSEC_Get_Auth_Algo_Index(&algo);

        if(status == NU_SUCCESS)
        {
            /* Set the authentication key length. */
            *auth_key_len =
                IPSEC_Authentication_Algos[(INT)algo].ipsec_key_len;

            /* Set the encryption algorithm ID. */
            algo = security->ipsec_encryption_algo;

            /* If the security protocol is AH. */
            if(security->ipsec_protocol == IPSEC_AH)
            {
                /* Encryption key is unused. */
                *enc_key_len = 0;
            }

            else
            {
                /* Convert the algorithm ID to array index. */
                status = IPSEC_Get_Encrypt_Algo_Index(&algo);

                if(status == NU_SUCCESS)
                {
                    /* Set the encryption algorithm key length. */
                    *enc_key_len =
                        IPSEC_Encryption_Algos[(INT)algo].ipsec_key_len;
                }

                else
                {
                    NLOG_Error_Log(
                        "Unable to get encryption algorithm index",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
        }

        else
        {
            NLOG_Error_Log("Unable to get authentication algorithm index",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Release the IPsec semaphore. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_IPS_Get_Key_Length */

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Tunnel_Override
*
* DESCRIPTION
*
*       This function overrides the default IPsec selector
*       with addresses expected by IPsec, when using tunnel
*       mode. During the IKE negotiation, the IPsec selector
*       contains private network addresses, as specified in
*       the IPsec policy. These must be replaced with the
*       tunnel end-point (SG) addresses when an IPsec SA is
*       being established.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*                               On return, source and destination
*                               of ph2->ike_ips_select contain
*                               the tunnel end-point addresses.
*       *security               Pointer to the IPsec security
*                               protocol which contains the tunnel
*                               end-point addresses.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID IKE_IPS_Tunnel_Override(IKE_PHASE2_HANDLE *ph2,
                                    IPSEC_SECURITY_PROTOCOL *security)
{
    UINT8           *select_src;
    UINT8           *select_dest;

    if((ph2->ike_flags & IKE_INITIATOR) != 0)
    {
        /* Use original order of addresses if we are the Initiator. */
        select_src  = ph2->ike_ips_select.ipsec_source_ip.ipsec_addr;
        select_dest = ph2->ike_ips_select.ipsec_dest_ip.ipsec_addr;
    }

    else
    {
        /* Switch order of selector addresses if we are the Responder. */
        select_src  = ph2->ike_ips_select.ipsec_dest_ip.ipsec_addr;
        select_dest = ph2->ike_ips_select.ipsec_source_ip.ipsec_addr;
    }

    /* Zero-out the selector source and destination. */
    UTL_Zero(&ph2->ike_ips_select.ipsec_source_ip,
             sizeof(ph2->ike_ips_select.ipsec_source_ip));
    UTL_Zero(&ph2->ike_ips_select.ipsec_dest_ip,
             sizeof(ph2->ike_ips_select.ipsec_dest_ip));

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
    /* If address type is IPv4. */
    if((security->ipsec_flags & IPSEC_IPV4) != 0)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* Override private network address with tunnel source. */
        NU_BLOCK_COPY(select_src, security->ipsec_tunnel_source,
                      IP_ADDR_LEN);

        /* Override private network address with tunnel destination. */
        NU_BLOCK_COPY(select_dest, security->ipsec_tunnel_destination,
                      IP_ADDR_LEN);

        /* Set tunnel address types to single IPs. */
        ph2->ike_ips_select.ipsec_source_type = (IPSEC_IPV4 |
                                                 IPSEC_SINGLE_IP);
        ph2->ike_ips_select.ipsec_dest_type   = (IPSEC_IPV4 |
                                                 IPSEC_SINGLE_IP);
    }
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
    else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    {
        /* Override private network address with tunnel source. */
        NU_BLOCK_COPY(select_src, security->ipsec_tunnel_source,
                      IP6_ADDR_LEN);

        /* Override private network address with tunnel destination. */
        NU_BLOCK_COPY(select_dest, security->ipsec_tunnel_destination,
                      IP6_ADDR_LEN);

        /* Set tunnel address types to single IPs. */
        ph2->ike_ips_select.ipsec_source_type = (IPSEC_IPV6 |
                                                 IPSEC_SINGLE_IP);
        ph2->ike_ips_select.ipsec_dest_type   = (IPSEC_IPV6 |
                                                 IPSEC_SINGLE_IP);
    }
#endif

} /* IKE_IPS_Tunnel_Override */
#endif

/*************************************************************************
*
* FUNCTION
*
*       IKE_IPS_Generate_SA_Pairs
*
* DESCRIPTION
*
*       This function creates IPsec inbound and outbound SAs
*       and adds them to the IPsec SADB, for all the IPsec
*       SAs negotiated in a Phase 2 Exchange.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle
*                               containing the negotiated IPsec
*                               SA parameters and the key
*                               material.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_INVALID_ALGO_ID   IPsec algorithm ID not valid in
*                               the negotiated algorithms.
*       IKE_INVALID_PARAMS      The parameter is invalid.
*       IKE_SA2_NOT_FOUND       No SA2 items found in Handle.
*
*************************************************************************/
STATUS IKE_IPS_Generate_SA_Pairs(IKE_PHASE2_HANDLE *ph2)
{
    STATUS              status = IKE_SA2_NOT_FOUND;
    IKE_SA2             *sa2;
    UINT16              enc_key_len;
    UINT16              auth_key_len;
    IPSEC_INBOUND_SA    in_sa;
    IPSEC_OUTBOUND_SA   out_sa;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameter is valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set the SA2 pointer to first item in the SA2DB. */
    sa2 = ph2->ike_sa2_db.ike_flink;

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    /* If tunnel mode SAs are being established. */
    if(sa2->ike_ips_security.ipsec_security_mode == IPSEC_TUNNEL_MODE)
    {
        /* Override private network addresses in the IPsec
         * selector with the tunnel end-point addresses.
         */
        IKE_IPS_Tunnel_Override(ph2, &sa2->ike_ips_security);
    }
#endif

    /* Loop for all SA2 items and create an IPsec SA
     * pair corresponding to each of them.
     */
    while(sa2 != NU_NULL)
    {
        /* Get the key length of encryption and
         * authentication algorithms.
         */
        status = IKE_IPS_Get_Key_Length(&sa2->ike_ips_security,
                                        &enc_key_len, &auth_key_len);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to calculate IPsec SA key lengths",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Abort if error occurred. */
            break;
        }

        /* If encryption algorithm is being applied. */
        if(enc_key_len != 0)
        {
            /* Set the encryption key in both SAs. */
            in_sa.ipsec_encryption_key  = sa2->ike_local_keymat;
            out_sa.ipsec_encryption_key = sa2->ike_remote_keymat;
        }

        else
        {
            /* Set encryption key pointers to NULL. */
            in_sa.ipsec_encryption_key  = NU_NULL;
            out_sa.ipsec_encryption_key = NU_NULL;
        }

        /* If authentication algorithm is being applied. */
        if(auth_key_len != 0)
        {
            /* Set the authentication key in both SAs. */
            in_sa.ipsec_auth_key  = sa2->ike_local_keymat +
                                    (INT)enc_key_len;
            out_sa.ipsec_auth_key = sa2->ike_remote_keymat +
                                    (INT)enc_key_len;
        }

        else
        {
            /* Set the authentication key pointers to NULL. */
            in_sa.ipsec_auth_key  = NU_NULL;
            out_sa.ipsec_auth_key = NU_NULL;
        }

        /*
         * Generate the IPsec inbound SA.
         */

        /* Log debug message. */
        IKE_DEBUG_LOG("Establishing IPsec inbound SA");

        /* Set the SPI. */
        in_sa.ipsec_spi = sa2->ike_local_spi;

        /* Copy the SA selector. */
        NU_BLOCK_COPY(&in_sa.ipsec_select, &ph2->ike_ips_select,
                      sizeof(IPSEC_SELECTOR));

        /* If we are the Initiator. */
        if((ph2->ike_flags & IKE_INITIATOR) != 0)
        {
            /* Copy selector while switching source and destination. */
            IKE_IPS_Switch_Selector(&in_sa.ipsec_select,
                                    &ph2->ike_ips_select);
        }

        /* Copy the security protocol. */
        NU_BLOCK_COPY(&in_sa.ipsec_security, &sa2->ike_ips_security,
                      sizeof(IPSEC_SECURITY_PROTOCOL));

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
        /* If policy direction is outbound or dual asynchronous. */
        if((IPSEC_POLICY_FLOW(ph2->ike_sa2_db.ike_ips_flags) &
            IPSEC_OUTBOUND) != 0)
        {
            /* Switch tunnel source and destination end points. */
            IKE_IPS_Switch_Security(&in_sa.ipsec_security,
                                    &sa2->ike_ips_security);
        }
#endif

        /* Set hard lifetime in SA. */
        NU_BLOCK_COPY(&in_sa.ipsec_hard_lifetime, &ph2->ike_ips_lifetime,
                      sizeof(IPSEC_SA_LIFETIME));

        /* Make sure the soft lifetime is only set for the
         * first SA pair, in case multiple pairs are being
         * established. Expiry of the first pair would also
         * re-negotiate all the following pairs.
         */
        if(sa2 == ph2->ike_sa2_db.ike_flink)
        {
            /* If the soft lifetime is applicable. */
            if((ph2->ike_ips_lifetime.ipsec_expiry_action != 0) &&
               (ph2->ike_ips_lifetime.ipsec_no_of_secs >
                IKE_SOFT_LIFETIME_OFFSET))
            {
                /* Set soft lifetime in SA. */
                NU_BLOCK_COPY(&in_sa.ipsec_soft_lifetime,
                              &ph2->ike_ips_lifetime,
                              sizeof(IPSEC_SA_LIFETIME));

                /* Decrement soft lifetime offset. */
                in_sa.ipsec_soft_lifetime.ipsec_no_of_secs -=
                    IKE_SOFT_LIFETIME_OFFSET;
            }
        }

        else
        {
            /* Zero out the soft lifetime. */
            UTL_Zero(&in_sa.ipsec_soft_lifetime,
                     sizeof(IPSEC_SA_LIFETIME));
        }

        /*
         * Generate the IPsec outbound SA.
         */

        /* Log debug message. */
        IKE_DEBUG_LOG("Establishing IPsec outbound SA");

        /* Set the SPI. */
        out_sa.ipsec_remote_spi = sa2->ike_remote_spi;

        /* Copy the SA selector. */
        NU_BLOCK_COPY(&out_sa.ipsec_select, &ph2->ike_ips_select,
                      sizeof(IPSEC_SELECTOR));

        /* If we are the Responder. */
        if((ph2->ike_flags & IKE_RESPONDER) != 0)
        {
            /* Switch the selector source and destination. */
            IKE_IPS_Switch_Selector(&out_sa.ipsec_select,
                                    &ph2->ike_ips_select);
        }

        /* Copy the security protocol. */
        NU_BLOCK_COPY(&out_sa.ipsec_security, &sa2->ike_ips_security,
                      sizeof(IPSEC_SECURITY_PROTOCOL));

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
        /* If policy direction is simply inbound (and not outbound
         * or dual asynchronous).
         */
        if((IPSEC_POLICY_FLOW(ph2->ike_sa2_db.ike_ips_flags) &
            IPSEC_OUTBOUND) == 0)
        {
            /* Switch tunnel source and destination end points. */
            IKE_IPS_Switch_Security(&out_sa.ipsec_security,
                                    &sa2->ike_ips_security);
        }
#endif

        /* Rehash existing outbound SAs which match the new
         * outbound SA being established.
         */
        if(IPSEC_Rehash_Outbound_SAs(ph2->ike_ips_group_name,
                                     &out_sa.ipsec_select,
                                     &out_sa.ipsec_security) == NU_SUCCESS)
        {
            IKE_DEBUG_LOG("At least one IPsec SA was re-hashed");
        }

        /*
         * Add both IPsec SAs to the SADB.
         */

        status = IPSEC_Add_SA_Pair(
                     ph2->ike_params->ike_packet->ike_if_index,
                     &out_sa, &in_sa);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to add SA pair to IPsec SADB",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Abort on failure. */
            break;
        }

        /* Move to the next SA2 item. */
        sa2 = sa2->ike_flink;
    }

    /* Return the status. */
    return (status);

} /* IKE_IPS_Generate_SA_Pairs */
