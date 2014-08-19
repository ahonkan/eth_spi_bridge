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
*       ike2_psk.c
*
* COMPONENT
*
*       IKEv2 - Preshared key
*
* DESCRIPTION
*
*       This file contains functions related to processing of preshared
*       key during the authenticating the IKEv2 SA.
*
* FUNCTIONS
*
*       IKE2_Lookup_Preshared_Key
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_api.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_api.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Lookup_Preshared_Key
*
* DESCRIPTION
*
*       Searches for presence of a pre-shared key for the peer represented
*       by the ID provided.
*
* INPUTS
*
*       *sa                     SA for which the pre-shared key is needed.
*       *id                     ID of the peer for which to find the key.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_LENGTH      Length of ID data is not valid.
*
*************************************************************************/
STATUS IKE2_Lookup_Preshared_Key(IKE2_SA *sa, IKE2_ID_PAYLOAD *id)
{
    STATUS              status = NU_SUCCESS;
    IKE_IDENTIFIER      identifier;
    IKE_PRESHARED_KEY   *psk = NU_NULL;
    UINT8               *psk_buff = NU_NULL;

#if (IKE2_DEBUG == NU_TRUE)
    if((sa == NU_NULL) || (id == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check the type of ID being presented in the payload. We need to copy
     * the ID data so that it can be used to lookup pre-shared key.
     */
    switch(id->ike2_id_type)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
    case IKE2_ID_TYPE_IPV4_ADDR:
        if(id->ike2_id_data_len == IP_ADDR_LEN)
        {
            identifier.ike_type = IKE_IPV4;

            /* Copy IPv4 address to the identifier. */
            NU_BLOCK_COPY(identifier.ike_addr.ike_ip.ike_addr1,
                          id->ike2_id_data, IP_ADDR_LEN);
        }
        else
        {
            status = IKE_INVALID_LENGTH;
        }
        break;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    case IKE2_ID_TYPE_IPV6_ADDR:
        if(id->ike2_id_data_len == IP6_ADDR_LEN)
        {
            identifier.ike_type = IKE_IPV6;

            /* Copy IPv6 address to the identifier. */
            NU_BLOCK_COPY(identifier.ike_addr.ike_ip.ike_addr1,
                          id->ike2_id_data, IP6_ADDR_LEN);
        }

        else
        {
            status = IKE_INVALID_LENGTH;
        }

        break;
#endif

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    case IKE2_ID_TYPE_DER_ASN1_DN:
        identifier.ike_type = IKE_DER_ASN1_DN;
        identifier.ike_addr.ike_domain = (CHAR *)id->ike2_id_data;
        break;
#endif

#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
    case IKE2_ID_TYPE_FQDN:
        identifier.ike_type = IKE_DOMAIN_NAME;
        identifier.ike_addr.ike_domain = (CHAR *)id->ike2_id_data;
        break;

    case IKE2_ID_TYPE_RFC822_ADDR:
        identifier.ike_type = IKE_USER_DOMAIN_NAME;
        identifier.ike_addr.ike_domain = (CHAR *)id->ike2_id_data;
        break;
#endif

    default:
        status = IKE_UNSUPPORTED_IDTYPE;
        break;
    }

    if(status == NU_SUCCESS)
    {
        /* Now we have the ID data. We need to match it against the data
         * stored with the PSK.
         */
        status = IKE_Get_Preshared_Key_By_ID(&identifier, &psk,
                                             IKE_MATCH_IDENTIFIERS);

        if(status == NU_SUCCESS)
        {
            /* We have found the pre-shared key we needed. Now allocate memory
             * for the buffer that we will return to the caller.
             */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)&psk_buff,
                                        psk->ike_key_len, NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* Copy the key to the allocated buffer. */
                NU_BLOCK_COPY(psk_buff, psk->ike_key, psk->ike_key_len);

                /* Set the pointer in the SA structure to the allocated buffer
                 * and set the length of the key.
                 */
                sa->ike_attributes.ike_remote_key = psk_buff;
                sa->ike_attributes.ike_remote_key_len =
                    (UINT16)psk->ike_key_len;
            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for PSK",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("PSK could not be found!",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    return (status);
} /* IKE2_Lookup_Preshared_Key */

#endif
