/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       ppp_opts_val.c
*
* COMPONENT
*
*       PPP - API
*
* DESCRIPTION
*
*       This file contains the API functions to validate values of PPP
*       link options and to obtain default values of PPP link 
*       options.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       PPP_Validate_Link_Options
*       PPP_Set_Default_Option
*
* DEPENDENCIES
*
*       nu_ppp.h
*
*************************************************************************/

#include "drivers/nu_ppp.h"

/*************************************************************************
*
*   FUNCTION
*
*       PPP_Validate_Link_Options
*
*   DESCRIPTION
*
*       This function validates link options.
*
*   INPUTS
*
*       *ppp_cfg            NU_PPP_CFG structure
*
*   OUTPUTS
*   
*       NU_SUCCESS                          success
*       NU_PPP_INVALID_PARAMS               Invalid parameter   
*       NU_INVALID_DEFAULT_MRU              Invalid value for MRU
*       NU_INVALID_DEFAULT_PROTOCOL         Invalid default protocol
*       NU_INVALID_PF_COMPRESSION           Invalid PF Compression value
*       NU_INVALID_AC_COMPRESSION           Invalid AC Compression value
*       NU_INVALID_USE_MAGIC_NUMBER         Invalid Use Magic Number
*       NU_INVALID_USE_ACCM                 Invalid Use ACCM
*       NU_INVALID_NUM_DNS_SERVERS          Invalid Number of DNS Servers
*       NU_INVALID_REQUIRE_ENCRYPTION       Invalid Require Encryption
*       NU_INVALID_40BIT_ENCRYPTION         Invalid 40-bit Encryption
*       NU_INVALID_56BIT_ENCRYPTION         Invalid 56-bit Encryption
*       NU_INVALID_128BIT_ENCRYPTION        Invalid 128-bit Encryption
*
*************************************************************************/
STATUS PPP_Validate_Link_Options(NU_PPP_CFG *ppp_cfg)
{

    /* Status to be returned */
    STATUS          status = NU_SUCCESS;

    if (ppp_cfg == NU_NULL)
        status = NU_PPP_INVALID_PARAMS;

    /* Validate a non-standard MRU. */
    else if ((ppp_cfg->default_mru <= 0) && 
        (ppp_cfg->default_mru > PPP_MP_DEFAULT_MRRU_SIZE))
        status = NU_INVALID_DEFAULT_MRU;

    /* Set the default authentication protocol. */
    else if ((ppp_cfg->default_auth_protocol != PPP_CHAP_PROTOCOL) &&
        (ppp_cfg->default_auth_protocol != PPP_CHAP_MS1_PROTOCOL) &&
        (ppp_cfg->default_auth_protocol != PPP_CHAP_MS2_PROTOCOL) &&
        (ppp_cfg->default_auth_protocol != PPP_PAP_PROTOCOL))
        status = NU_INVALID_DEFAULT_PROTOCOL;        

#if (INCLUDE_IPV4 == NU_TRUE)
    /* Validate DNS server count. */
    else if (ppp_cfg->num_dns_servers > PPP_MAX_DNS_SERVERS)
        status = NU_INVALID_NUM_DNS_SERVERS;
#endif

    /* Validate protocol field compression */
    else if ((ppp_cfg->use_pf_compression != NU_TRUE) &&
        (ppp_cfg->use_pf_compression != NU_FALSE))
        status = NU_INVALID_PF_COMPRESSION;

    /* Validate address and control field compression */
    else if ((ppp_cfg->use_ac_compression != NU_TRUE) &&
        (ppp_cfg->use_ac_compression != NU_FALSE))
        status = NU_INVALID_AC_COMPRESSION;

    /* Validate the magic number option */
    else if ((ppp_cfg->use_magic_number != NU_TRUE) &&
        (ppp_cfg->use_magic_number != NU_FALSE))
        status = NU_INVALID_USE_MAGIC_NUMBER;

    /* Validate the ACCM */
    else if ((ppp_cfg->use_accm != NU_TRUE) &&
        (ppp_cfg->use_accm != NU_FALSE))
        status = NU_INVALID_USE_ACCM;

#if (PPP_ENABLE_MPPE == NU_TRUE)

    /* Validate the require_encryption */
    else if ((ppp_cfg->require_encryption != NU_TRUE) &&
        (ppp_cfg->require_encryption != NU_FALSE))
        status = NU_INVALID_REQUIRE_ENCRYPTION;

#if (PPP_USE_40_BIT_ENCRYPTION == NU_TRUE)
    /* Validate use_40_bit_encryption */
    if ((ppp_cfg->use_40_bit_encryption != NU_TRUE) &&
        (ppp_cfg->use_40_bit_encryption != NU_FALSE))
        status = NU_INVALID_40BIT_ENCRYPTION;
#endif

#if (PPP_USE_56_BIT_ENCRYPTION == NU_TRUE)
    /* Validate use_56_bit_encryption */
    else if ((ppp_cfg->use_56_bit_encryption != NU_TRUE) &&
        (ppp_cfg->use_56_bit_encryption != NU_FALSE))
        status = NU_INVALID_56BIT_ENCRYPTION;
#endif

#if (PPP_USE_128_BIT_ENCRYPTION == NU_TRUE)
    /* Validate use_128_bit_encryption */
    else if ((ppp_cfg->use_128_bit_encryption != NU_TRUE) &&
        (ppp_cfg->use_128_bit_encryption != NU_FALSE))
        status = NU_INVALID_128BIT_ENCRYPTION;
#endif

#endif
    /* Return status */
    return (status);

} /* PPP_Validate_Link_Options */

/*************************************************************************
*
*   FUNCTION
*
*       PPP_Get_Default_Options
*
*   DESCRIPTION
*
*       This function gets default link options.
*
*   INPUTS
*
*       *ppp_cfg            PPP_CFG structure
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID PPP_Get_Default_Options(PPP_CFG * ppp_cfg)
{

#if (PPP_ENABLE_MPPE == NU_TRUE)

    /* Set use_40_bit_encryption. */
#if (PPP_USE_40_BIT_ENCRYPTION == NU_TRUE)
    ppp_cfg->use_40_bit_encryption = NU_TRUE;
#else
    ppp_cfg->use_40_bit_encryption = NU_FALSE;
#endif
    /* Set use_56_bit_encryption. */
#if (PPP_USE_56_BIT_ENCRYPTION == NU_TRUE)
    ppp_cfg->use_56_bit_encryption = NU_TRUE;
#else
    ppp_cfg->use_56_bit_encryption = NU_FALSE;
#endif

    /* Set use_128_bit_encryption. */
#if (PPP_USE_128_BIT_ENCRYPTION == NU_TRUE)
    ppp_cfg->use_128_bit_encryption = NU_TRUE;
#else
    ppp_cfg->use_128_bit_encryption = NU_FALSE;
#endif

    /* Set require_encryption. */
#if (PPP_REQUIRE_ENCRYPTION == NU_TRUE)
    ppp_cfg->require_encryption = NU_TRUE;
#else
    ppp_cfg->require_encryption = NU_FALSE;
#endif
#endif

    /* Set default protocol. */
#if (PPP_USE_CHAP_MS2 == NU_TRUE)
    ppp_cfg->default_auth_protocol = PPP_CHAP_MS2_PROTOCOL;
#elif (PPP_USE_CHAP_MS1 == NU_TRUE)
    ppp_cfg->default_auth_protocol = PPP_CHAP_MS1_PROTOCOL;
#elif (PPP_USE_CHAP == NU_TRUE)
    ppp_cfg->default_auth_protocol = PPP_CHAP_PROTOCOL;
#elif (PPP_USE_PAP == NU_TRUE)
    ppp_cfg->default_auth_protocol = PPP_PAP_PROTOCOL;
#else
    ppp_cfg->default_auth_protocol = 0;
#endif

    /* Set use_magic_number. */
#if (PPP_USE_MAGIC == NU_TRUE)
    ppp_cfg->use_magic_number = NU_TRUE;
#else
    ppp_cfg->use_magic_number = NU_FALSE;
#endif

    /* Set use_accm. */
#if (PPP_USE_ACCM == NU_TRUE)
    ppp_cfg->use_accm = NU_TRUE;
#else
    ppp_cfg->use_accm = NU_FALSE;
#endif

    ppp_cfg->default_accm = HDLC_LOCAL_DEFAULT_ACCM;
    ppp_cfg->default_fcs_size = PPP_DEFAULT_FCS_SIZE;

#if (INCLUDE_PPP_MP)
    ppp_cfg->mp_mrru = PPP_MP_DEFAULT_MRRU_SIZE;
#endif

    /* MRU macro. */
    ppp_cfg->default_mru = PPP_MRU;

    /* Set num_dns_servers. */
    ppp_cfg->num_dns_servers = 0;

#if (PPP_USE_DNS1 == NU_TRUE)
    ppp_cfg->num_dns_servers++;
#endif
#if (PPP_USE_DNS2 == NU_TRUE)
    ppp_cfg->num_dns_servers++;
#endif

    /* Set use_pf_compression. */
#if (PPP_USE_PFC == NU_TRUE)
    ppp_cfg->use_pf_compression = NU_TRUE;
#else
    ppp_cfg->use_pf_compression = NU_FALSE;
#endif

    ppp_cfg->use_flags = 0;

    /* Set use_ac_compression. */
#if (PPP_USE_ACC == NU_TRUE)
    ppp_cfg->use_ac_compression = NU_TRUE;
#else
    ppp_cfg->use_ac_compression = NU_FALSE;
#endif

} /* PPP_Get_Default_Options */
