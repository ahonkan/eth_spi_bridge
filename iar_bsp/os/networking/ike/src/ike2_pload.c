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
*       ike2_pload.c
*
* COMPONENT
*
*       IKEv2 - Payload Processing
*
* DESCRIPTION
*       This file contains all functions related to encoding and decoding
*       of different payloads present in IKEv2.
*
* FUNCTIONS
*
*       IKE2_Compute_Check_Sum
*       IKE2_Encode_IKE_Header
*       IKE2_Decode_IKE_Header
*       IKE2_Encode_IKE_Gen_Header
*       IKE2_Decode_IKE_Gen_Header
*       IKE2_Encode_IKE_Trans_Attrib_Payload
*       IKE2_Decode_IKE_Trans_Attrib_Payload
*       IKE2_Encode_IKE_Transform_Payload
*       IKE2_Decode_IKE_Transform_Payload
*       IKE2_Encode_IKE_Proposal_Payload
*       IKE2_Decode_IKE_Proposal_Payload
*       IKE2_Encode_IKE_SA_Payload
*       IKE2_Decode_IKE_SA_Payload
*       IKE2_Encode_IKE_KE_Payload
*       IKE2_Decode_IKE_KE_Payload
*       IKE2_Encode_IKE_ID_Payload
*       IKE2_Decode_IKE_ID_Payload
*       IKE2_Encode_IKE_Auth_Payload
*       IKE2_Decode_IKE_Auth_Payload
*       IKE2_Encode_IKE_Nonce_Payload
*       IKE2_Decode_IKE_Nonce_Payload
*       IKE2_Encode_IKE_Vendor_ID_Payload
*       IKE2_Decode_IKE_Vendor_ID_Payload
*       IKE2_Encode_IKE_TS
*       IKE2_Decode_IKE_TS
*       IKE2_Encode_IKE_TS_Payload
*       IKE2_Decode_IKE_TS_Payload
*       IKE2_Encode_IKE_Encrypted_Payload
*       IKE2_Decode_IKE_Encrypted_Payload
*       IKE2_Encode_IKE_Cert_Payload
*       IKE2_Decode_IKE_Cert_Payload
*       IKE2_Encode_IKE_CertReq_Payload
*       IKE2_Decode_IKE_CertReq_Payload
*       IKE2_Encode_IKE_Notify_Payload
*       IKE2_Decode_IKE_Notify_Payload
*       IKE2_Encode_IKE_Delete_Payload
*       IKE2_Decode_IKE_Delete_Payload
*       IKE2_Encode_IKE_Config_Payload
*       IKE2_Decode_IKE_Config_Payload
*       IKE2_Encode_IKE_EAP_Payload
*       IKE2_Decode_IKE_EAP_Payload
*       IKE2_Encode_Message
*       IKE2_Decode_Message
*
* DATA STRUCTURES
*
*       IKE2_Encoding_Funcs      Array containing pointers to different
*                                kinds of payloads.
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ike_api.h
*       ike.h
*       ike_enc.h
*       ike_buf.h
*       ike2_pload.h
*       ike_crypto_wrappers.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ike_api.h"
#include "networking/ike.h"
#include "networking/ike_enc.h"
#include "networking/ike_buf.h"
#include "networking/ike_crypto_wrappers.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

#include "networking/ike2_pload.h"

#define IKE2_ENCODE_FUNC_COUNT          17

/* Payload encoding functions. These are in order of their respective
 * payload ID's. Encrypted payload is encoded differently and is not
 * needed for IKE_INIT_SA exchange at all. We never append a payload
 * of this type to the payload chain and the decoding function does not
 * have a case for this type of payload. Thus the function pointer at
 * that index below will never be called.
 */
STATIC IKE2_ENCODE_PAYLOAD_FUNCS IKE2_Encoding_Funcs[IKE2_ENCODE_FUNC_COUNT] =
{
    NU_NULL,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_SA_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_KE_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_ID_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_ID_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_Cert_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_CertReq_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_Auth_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_Nonce_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_Notify_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_Delete_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_Vendor_ID_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_TS_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_TS_Payload,
    NU_NULL /* IKE2_Encode_IKE_Encrypted_Payload */,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_Config_Payload,
    (IKE2_ENCODE_PAYLOAD_FUNCS)IKE2_Encode_IKE_EAP_Payload
};

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Compute_Checksum
*
* DESCRIPTION
*
*       This is a utility function used to calculate the check sum
*       using cipher suite API and is called from other API functions.
*
* INPUTS
*
*       *sa                     Pointer to the source IKEv2 SA.
*       *source_buff            The text whose check sum is to be computed.
*       source_len              Length of the above text.
*       *dest_buff              On return it contains the check sum.
*       *digest_len             Specifies our required length of checksum.
*       operation               Operation type IKE_ENCRYPT / IKE_DECRYPT,
*                               that is used in selecting key material.
*
* OUTPUTS
*
*       NU_SUCCESS              On success if check sum is computed.
*
*************************************************************************/
STATIC STATUS IKE2_Compute_Checksum(IKE2_SA *sa, UINT8 *source_buff,
                                     UINT16 source_len, UINT8 *dest_buff,
                                     UINT16 *digest_len, UINT8 operation)
{
    STATUS          status;

    /* IKEv2 has different keys for initiator and responder. This pointer
     * is used to point to the one we should use while computing check sum.
     */
    UINT8           *key_material;

    /* Local variable used to specify the parameters for hashing, to compute
     * checksum data.
     */
    UINT8           algo_id;
    UINT8           *text;
    INT             text_len;
    UINT8           *md;
    UINT8           md_len;
    UINT8           *key;
    INT             key_len;

    /* Fill the request structure */
    algo_id = IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id;

    /* Specify the text whose hash is to be calculated. */
    text = source_buff;

    /* Length of text whose hash is to be calculated. */
    text_len = source_len;

    /* Specify the buffer for storing the digest. Effectively we are
     * specifying the location where cipher suite call should place
     * the check sum data.
     */
    md = dest_buff;

    /* By assigning digest len zero, actual length of digest computed
     * is copied.
     */
    md_len = IKE2_AUTH_HMAC_Algos[sa->ike_attributes.
                                ike_hash_algo].ike2_output_len;

    /* If we are initiator, then encrypt/decrypt with respective key */
    if ((sa->ike2_current_handle->ike2_flags & IKE2_INITIATOR) != 0)
    {
        if(operation == IKE_ENCRYPT)
            key_material = sa->ike2_sk_ai;
        else
            key_material = sa->ike2_sk_ar;
    }

    /* If we are responder, then encrypt/decrypt with respective key */
    else
    {
        if(operation == IKE_ENCRYPT)
            key_material = sa->ike2_sk_ar;
        else
            key_material = sa->ike2_sk_ai;
    }

    /* Specify the key and it's length used for hashing in HMAC mode. It
     * could be of a variable length.
     */
    key = key_material;

    /* Key length is same so it doesn't matter whether we choose initiator
     * or responder's key length and thus there is only one length field.
     */
    key_len = IKE2_AUTH_HMAC_Algos[sa->ike_attributes.
                                ike_hash_algo].ike2_key_len;

    /* Make the request for hashing. On return, md contains
     * the digest of the HMAC-SHA256 hash function and md_len
     * contains the exact length of the digest.
     */
    status = IKE_HMAC(algo_id, key, key_len, text, text_len, md, &md_len);

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to calculate hash i.e. checksum data",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    else
    {
        /* It means that check sum data has been successfully encoded after
         * the encrypted data in the buffer.
         */

        /* Update the length of digest */
        *digest_len = md_len;
    }

    return (status);

} /* IKE2_Compute_Check_Sum */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Header
*
* DESCRIPTION
*
*       This function encodes an IKEv2 payload header.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *hdr                    Pointer to the source payload header.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_Header(UINT8 *buffer, UINT16 buffer_len,
                              IKE2_HDR *hdr)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (hdr == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(buffer_len < IKE2_HDR_TOTAL_LEN)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode the SA initiator SPI */
        NU_BLOCK_COPY(buffer + IKE2_HDR_SA_ISPI_OFST,
            hdr->ike2_sa_spi_i,IKE2_HDR_SA_SPI_LEN);

        /* Encode the SA responder SPI */
        NU_BLOCK_COPY(buffer + IKE2_HDR_SA_RSPI_OFST,
            hdr->ike2_sa_spi_r,IKE2_HDR_SA_SPI_LEN);

        /* Encode the next payload type */
        PUT8(buffer, IKE2_HDR_NXT_PYLD_OFST, hdr->ike2_next_payload);

        /* Encode the major and minor versions */
        PUT8(buffer, IKE2_HDR_VRSN_OFST,
            (hdr->ike2_major_version << IKE2_HDR_MJR_VRSN_SHFT |
                hdr->ike2_minor_version));

        /* Encode the exchange type. */
        PUT8(buffer, IKE2_HDR_XCHG_TYPE_OFST, hdr->ike2_exchange_type);

        /* Encode the flags. */
        PUT8(buffer, IKE2_HDR_FLAGS_OFST, hdr->ike2_flags);

        /* Encode the message id. */
        PUT32(buffer, IKE2_HDR_MSG_ID_OFST, hdr->ike2_msg_id);

        /* Encode the total packet length. */
        PUT32(buffer, IKE2_HDR_LEN_OFST, hdr->ike2_length);
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Header */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Header
*
* DESCRIPTION
*
*       This function decodes an IKEv2 payload header.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *hdr                    Pointer to the destination payload
*                               header.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_Header(UINT8 *buffer, UINT16 buffer_len,
                              IKE2_HDR *hdr)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (hdr == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if enough space in buffer. */
    if(buffer_len < IKE2_HDR_TOTAL_LEN)
    {
        /* Set error status. */
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Decode the SA initiator SPI */
        NU_BLOCK_COPY(hdr->ike2_sa_spi_i,
            buffer + IKE2_HDR_SA_ISPI_OFST, IKE2_HDR_SA_SPI_LEN);

        /* Decode the SA responder SPI */
        NU_BLOCK_COPY(hdr->ike2_sa_spi_r,
            buffer + IKE2_HDR_SA_RSPI_OFST, IKE2_HDR_SA_SPI_LEN);

        /* Decode the next payload type */
        hdr->ike2_next_payload = GET8(buffer, IKE2_HDR_NXT_PYLD_OFST);

        /* Decode the major and minor versions */
        hdr->ike2_major_version = GET8(buffer,IKE2_HDR_VRSN_OFST);
        hdr->ike2_minor_version = hdr->ike2_major_version &
            IKE2_HDR_MNR_VRSN_MASK;
        hdr->ike2_major_version >>= IKE2_HDR_MJR_VRSN_SHFT;

        /* Decode the exchange type. */
        hdr->ike2_exchange_type = GET8(buffer, IKE2_HDR_XCHG_TYPE_OFST);

        /* Decode the flags. */
        hdr->ike2_flags = GET8(buffer, IKE2_HDR_FLAGS_OFST);

        /* Decode the Message ID. */
        hdr->ike2_msg_id = GET32(buffer, IKE2_HDR_MSG_ID_OFST);

        /* Decode total packet length. */
        hdr->ike2_length = GET32(buffer, IKE2_HDR_LEN_OFST);
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Header */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Gen_Header
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Generic header.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *hdr                    Pointer to the source payload
*                               header.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_Gen_Header(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_GEN_HDR *hdr)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (hdr == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if enough space in buffer. */
    if(buffer_len < IKE2_GEN_HDR_TOTAL_LEN)
    {
        /* Set error status. */
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode the next payload type */
        if (hdr->ike2_next_payload != NU_NULL)
        {
            /* if it exists then encode it's type */
            PUT8(buffer, IKE2_GEN_HDR_NXT_PYLD_OFST,
                hdr->ike2_next_payload->ike2_payload_type);
        }

        else
        {
            /* Other wise encode no next payload type */
            PUT8(buffer, IKE2_GEN_HDR_NXT_PYLD_OFST, IKE2_NONE_PAYLOAD_ID);
        }

        /* Encode the critical bit and reserved bits */
        PUT8(buffer, IKE2_GEN_HDR_CRITICAL_OFST, (hdr->ike2_critical <<
            IKE2_GEN_HDR_CRITICAL_SHFT) & IKE2_GEN_HDR_RSVRD_MASK);

        /* Encode the payload length */
        PUT16(buffer, IKE2_GEN_HDR_PYLD_LEN_OFST, hdr->ike2_payload_length);
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Generic_Header */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Gen_Header
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Generic header.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *hdr                    Pointer to the destination payload
*                               header.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_Gen_Header(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_GEN_HDR *hdr, UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (hdr == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if enough space in buffer. */
    if(buffer_len < IKE2_GEN_HDR_TOTAL_LEN)
    {
        /* Set error status. */
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Decode the next payload type */
        *next_pload = GET8(buffer, IKE2_GEN_HDR_NXT_PYLD_OFST);

        /* Decode the critical bit and reserved bits */
        hdr->ike2_critical =  GET8(buffer, IKE2_GEN_HDR_CRITICAL_OFST) >>
                                IKE2_GEN_HDR_CRITICAL_SHFT;

        /* Decode the payload length */
        hdr->ike2_payload_length = GET16(buffer, IKE2_GEN_HDR_PYLD_LEN_OFST);
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Generic_Header */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Trans_Attrib
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Transform Attribute.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *attrib                 Pointer to the source attribute.
*       *attrib_len             On return, this contains the
*                               actual attribute length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_Trans_Attrib_Payload(UINT8 *buffer,
                                        UINT16 buffer_len,
                                        IKE2_TRANS_ATTRIB_PAYLOAD *attrib,
                                        UINT16 *attrib_len)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (attrib == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* If attribute is in TLV format. */
    if((attrib->ike2_attrib_type & IKE2_TRANS_ATTRIB_AF_MASK) ==
        IKE2_TRANS_ATTRIB_AF_TLV)
    {
        /* Check if enough space in buffer. */
        if(IKE2_MIN_TRANS_ATTRIB_LEN + attrib->ike2_attrib_lenval >
            buffer_len)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Encode attribute value. */
            NU_BLOCK_COPY(buffer + IKE2_ATTRIB_VAL_OFST,
                attrib->ike2_attrib_value,
                attrib->ike2_attrib_lenval);

            /* Set attribute length. */
            (*attrib_len) = IKE2_MIN_TRANS_ATTRIB_LEN + attrib->
                ike2_attrib_lenval;
        }
    }

    /* Check buffer size for TV format. */
    else
    {
        /* Check the buffer length. */
        if(buffer_len < IKE2_MIN_TRANS_ATTRIB_LEN)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Set attribute length. */
            (*attrib_len) = IKE2_MIN_TRANS_ATTRIB_LEN;
        }
    }

    /* If no error occurred above. */
    if(status == NU_SUCCESS)
    {
        /* Encode attribute type. */
        PUT16(buffer, IKE2_ATTRIB_AFTYPE_OFST, attrib->ike2_attrib_type);

        /* Encode attribute length/value. */
        PUT16(buffer, IKE2_ATTRIB_LENVAL_OFST, attrib->ike2_attrib_lenval);
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Trans_Attrib Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Trans_Attrib_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Transform Attribute payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_Trans_Attrib_Payload(UINT8 *buffer, UINT16 buffer_len,
                                         IKE2_TRANS_ATTRIB_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload     == NU_NULL) )
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if enough data in buffer */
    if (IKE2_MIN_TRANS_ATTRIB_LEN > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Decode the transform attribute type */
        pload->ike2_attrib_type = GET16(buffer, IKE2_ATTRIB_AFTYPE_OFST);

        /*If the attribute is in TLV format, then calculate its length */
        if ((pload->ike2_attrib_type & IKE2_TRANS_ATTRIB_AF_MASK) ==
            IKE2_TRANS_ATTRIB_AF_TLV)
        {
            /* Decode the Length of attribute value */
            pload->ike2_attrib_lenval = GET16(buffer, IKE2_ATTRIB_LENVAL_OFST);

            /* Check if the actual attribute length is greater than buffer*/
            if ( IKE2_MIN_TRANS_ATTRIB_LEN + pload->ike2_attrib_lenval >
                buffer_len)
            {
                status = IKE_LENGTH_IS_SHORT;
            }

            else
            {
                /* Decode the variable length attribute value */
                NU_BLOCK_COPY(pload->ike2_attrib_value, buffer +
                    IKE2_ATTRIB_VAL_OFST, pload->ike2_attrib_lenval);
            }
        }

        else /* Attribute is in TV format */
        {
            /* Check if the actual attribute length is greater than buffer*/
            if (IKE2_MIN_TRANS_ATTRIB_LEN > buffer_len)
            {
                status = IKE_LENGTH_IS_SHORT;
            }

            else
            {
                /* Decode the fixed length attribute value */
                pload->ike2_attrib_lenval = GET16(buffer,
                    IKE2_ATTRIB_LENVAL_OFST);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Trans_Attrib_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Transform_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Transform payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
************************************************************************/
STATUS IKE2_Encode_IKE_Transform_Payload(UINT8 *buffer, UINT16 buffer_len,
                                         IKE2_TRANSFORMS_PAYLOAD *pload)
{
    UINT16          attrib_len;
    INT             i;
    UINT16          offset;
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer    == NU_NULL) || (pload == NU_NULL) )
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(pload->ike2_transform_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode the byte indicating whether this is the last transform */
        PUT8(buffer, IKE2_TRNSFRM_ISLAST_OFST,
            (UINT8)((pload->ike2_more_transforms == 0) ? (IKE2_NONE_PAYLOAD_ID)
            : (IKE2_TRANSFORM_PAYLOAD_ID)));

        /* Encode the first reserved byte */
        PUT8(buffer, IKE2_TRNSFRM_FRST_RSVRD_OFST, IKE2_RSVRD_VALUE);

        /* Encode the length of transform */
        PUT16(buffer,IKE2_TRNSFRM_LEN_OFST, pload->ike2_transform_length);

        /* Encode the transform type */
        PUT8(buffer, IKE2_TRNSFRM_TYPE_OFST, pload->ike2_transform_type);

        /* Encode the second reserved byte */
        PUT8(buffer, IKE2_TRNSFRM_SEC_RSVRD_OFST, IKE2_RSVRD_VALUE);

        /* Encode the transform id. */
        PUT16(buffer, IKE2_TRNSFRM_ID_OFST, pload->ike2_transform_id);

        /* offset tracks the starting index of first transform attrib */
        offset = IKE2_TRNSFRM_ATTRIBS_OFST;

        /* Initialize remaining buffer length. */
        buffer_len = buffer_len - offset;

        /* Loop to encode each transform attribute.*/
        for (i = 0; i < pload->ike2_transform_attrib_num; i++ )
        {
            status = IKE2_Encode_IKE_Trans_Attrib_Payload(buffer + offset,
                buffer_len - offset, &pload->ike2_transform_attrib[i],
                &attrib_len);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to encode attribute",
                    NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Exit the loop. */
                break;
            }

            /* Move to the next attribute offset. */
            offset = offset + attrib_len;

            /* Add each trans attributes length to obtain current index */
            offset = offset + attrib_len;

            /* Update the remaining buffer length */
            buffer_len = buffer_len - attrib_len;
        }
   }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Transform_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Transform_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Transform payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_Transform_Payload(UINT8 *buffer, UINT16 buffer_len,
                                        IKE2_TRANSFORMS_PAYLOAD *pload)
{
    UINT8           i;
    UINT16          offset;
    UINT16          attrib_len;
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload     == NU_NULL) )
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if enough data in buffer */
    if(buffer_len < IKE2_MIN_TRANS_PAYLOAD_LEN)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Decode the flag indicating whether this is the last transform */
        pload->ike2_more_transforms = GET8(buffer, IKE2_TRNSFRM_ISLAST_OFST);

        /* Decode the transform length */
        pload->ike2_transform_length = GET16(buffer, IKE2_TRNSFRM_LEN_OFST);

        /* Check if actual payload length is greater than buffer len */
        if ( pload->ike2_transform_length > buffer_len)
        {
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode the transform type */
            pload->ike2_transform_type = GET8(buffer, IKE2_TRNSFRM_TYPE_OFST);

            /* Decode the transform id */
            pload->ike2_transform_id = GET16(buffer, IKE2_TRNSFRM_ID_OFST);

            /* offset is used to track starting index of each transform */
            offset = IKE2_TRNSFRM_ATTRIBS_OFST;

            /* Initialize remaining buffer length. */
            buffer_len = buffer_len - offset;

            if(pload->ike2_transform_length > IKE2_MIN_TRANS_PAYLOAD_LEN)
            {
                /* Some attribute is present. Loop to decode each
                 * transform attribute.
                 */
                for (i = 0; i < IKE2_NUM_OF_TRANSFRM_ATTRIBS; i++ )
                {
                    status = IKE2_Decode_IKE_Trans_Attrib_Payload(buffer+
                        offset,buffer_len, &(pload->ike2_transform_attrib[i]));

                    if ( status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Unable to decode transform attribute",
                            NERR_RECOVERABLE, __FILE__, __LINE__);

                        /* Stop processing transform attributes chain. */
                        break;
                    }

                    /* If the attribute is in TLV format, then calculate its
                     * length
                     */
                    if ( (pload->ike2_transform_attrib[i].ike2_attrib_type
                        & IKE2_TRANS_ATTRIB_AF_MASK) == IKE2_TRANS_ATTRIB_AF_TLV)
                    {
                        /* The net payload len is the sum of Minimum payload
                         * len and the attribute length
                         */
                        attrib_len = pload-> ike2_transform_attrib[i].
                            ike2_attrib_lenval + IKE2_MIN_TRANS_ATTRIB_LEN;

                        /* Now if this attribute length plus offset is less
                         * than transform len then there are still more
                         * attributes present
                         */
                        if (attrib_len + offset < pload->ike2_transform_length)
                        {
                            offset = offset + attrib_len;
                            buffer_len = buffer_len - offset;
                        }

                        else
                        {
                            /* This is the last attribute. Stop processing
                             * transform attributes chain.
                             */
                            break;
                        }
                    }

                    /* If the attribute is in TV format, then compute length */
                    else
                    {
                        /* Set attribute length. */
                        attrib_len = IKE2_MIN_TRANS_ATTRIB_LEN;

                        /* Now if this attribute length plus offset is less
                         * than transform len then there are more attributes
                         * present
                         */
                        if (attrib_len + offset < pload->ike2_transform_length)
                        {
                            offset = offset + attrib_len;
                            buffer_len = buffer_len - offset;
                        }

                        else
                        {
                            /* This is the last attribute. Stop processing
                             * transform attributes chain.
                             */
                            break;
                        }
                    }
                }

                /* Now we know the number of trans attributes in the transform */
                if(i == IKE2_NUM_OF_TRANSFRM_ATTRIBS)
                {
                    pload->ike2_transform_attrib_num = i;
                }

                else
                {
                    pload->ike2_transform_attrib_num = i + 1;
                }
            }

            else
            {
                /* No attribute is specified in the incoming transform. */
                pload->ike2_transform_attrib_num = 0;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Transform_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Proposal_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Proposal payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_Proposal_Payload(UINT8 *buffer, UINT16 buffer_len,
                              IKE2_PROPOSAL_PAYLOAD *pload)
{
    INT             i;
    UINT16          offset;
    STATUS          status = IKE_INVALID_PAYLOAD;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL) )
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(pload->ike2_proposal_len > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode the flag indicating whether this is the last proposal */
        PUT8(buffer, IKE2_PRPSL_ISLAST_OFST,
          (UINT8)((pload->ike2_more_proposals == 0)? (IKE2_NONE_PAYLOAD_ID)
          : (IKE2_PROPOSAL_PAYLOAD_ID)));

        /* Encode the reserved byte */
        PUT8(buffer, IKE2_PRPSL_RSVRD_OFST, IKE2_RSVRD_VALUE);

        /* Encode the length of proposal */
        PUT16(buffer,IKE2_PRPSL_LEN_OFST, pload->ike2_proposal_len);

        /* Encode the proposal number */
        PUT8(buffer, IKE2_PRPSL_NUM_OFST, pload->ike2_proposal_num);

        /* Encode the protocol id. */
        PUT8(buffer, IKE2_PRPSL_PROTO_ID_OFST, pload->ike2_protocol_id);

        /* Encode the SPI size */
        PUT8(buffer, IKE2_PRPSL_SPI_SIZE_OFST, pload->ike2_spi_size);

        /* Encode number of transforms. */
        PUT8(buffer, IKE2_PRPSL_NO_OF_TRNSFRMS_OFST,
            pload->ike2_no_of_transforms);

        /* Encode the SPI. */
        if(pload->ike2_spi_size > 0)
        {
            NU_BLOCK_COPY(buffer + IKE2_PRPSL_SPI_OFST, pload->ike2_spi,
                pload->ike2_spi_size);
        }

        /* Offset is used to track starting index of each transform */
        offset = IKE2_PRPSL_SPI_OFST + pload->ike2_spi_size;

        /* Initialize remaining buffer length. */
        buffer_len = buffer_len - offset;

        /* Loop to encode each transform. */
        for (i = 0; i < pload->ike2_no_of_transforms; i++ )
        {
            status = IKE2_Encode_IKE_Transform_Payload(buffer+offset,
                buffer_len, &(pload->ike2_transforms[i]));

            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to encode transform payload",
                    NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Stop processing transforms chain. */
                break;
            }

            /* Add each transforms length to obtain current index */
            offset = offset + pload->ike2_transforms[i].
                ike2_transform_length;

            /* Update the remaining buffer length */
            buffer_len = buffer_len - pload->ike2_transforms[i].
                ike2_transform_length;
        }

    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Proposal_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Proposal_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Proposal payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_Proposal_Payload(UINT8 *buffer, UINT16 buffer_len,
                                        IKE2_PROPOSAL_PAYLOAD *pload)
{
    UINT8             i;
    UINT16          offset;
    STATUS          status = IKE_INVALID_PAYLOAD;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload     == NU_NULL) )
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if enough data in buffer */
    if (IKE2_MIN_PRPSL_PAYLOAD_LEN > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Decode the flag indicating whether this is the last proposal */
        pload->ike2_more_proposals = GET8(buffer, IKE2_PRPSL_ISLAST_OFST);

        /* Decode the proposal length */
        pload->ike2_proposal_len = GET16(buffer, IKE2_PRPSL_LEN_OFST);

        /* Check if proposal length is more than buffer length available */
        if (pload->ike2_proposal_len > buffer_len)
        {
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode the proposal number */
           pload->ike2_proposal_num = GET8(buffer, IKE2_PRPSL_NUM_OFST);

            /* Decode the protocol id */
           pload->ike2_protocol_id = GET8(buffer, IKE2_PRPSL_PROTO_ID_OFST);

           /* Decode the SPI size */
           pload->ike2_spi_size = GET8(buffer, IKE2_PRPSL_SPI_SIZE_OFST);

           /* Decode the number of transforms */
           pload->ike2_no_of_transforms = GET8(buffer,
               IKE2_PRPSL_NO_OF_TRNSFRMS_OFST);

           /* Ensure the number of transforms in the packet does not
            * exceed the memory available to hold them.
            */
           if (pload->ike2_no_of_transforms <= IKE2_MAX_TRANSFORMS)
           {
               /* Decode the SPI */
               NU_BLOCK_COPY(&(pload->ike2_spi),buffer+IKE2_PRPSL_SPI_OFST,
                             pload->ike2_spi_size);

               /* offset is used to track starting index of each transform */
               offset = IKE2_PRPSL_SPI_OFST + pload->ike2_spi_size;

               /* Initialize remaining buffer length. */
               buffer_len = buffer_len - offset;

               /* Loop to decode each transform. */
               for (i = 0; i < pload->ike2_no_of_transforms; i++)
               {
                    status = IKE2_Decode_IKE_Transform_Payload(
                                                buffer + offset,
                                                buffer_len,
                                                &(pload->ike2_transforms[i]));

                    if ( status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Unable to decode transform payload",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);

                      /* Stop processing transforms chain. */
                        break;
                    }

                    /* If this was the last transform then stop processing
                     * transforms chain.
                     */
                    if (pload->ike2_transforms[i].ike2_more_transforms
                        == IKE2_NONE_PAYLOAD_ID)
                    {
                        break;
                    }

                    /* Add each transforms len to obtain current index. */
                    offset = offset + pload->ike2_transforms[i].ike2_transform_length;

                    /* Update the remaining buffer length. */
                    buffer_len = buffer_len - pload->ike2_transforms[i].
                                                     ike2_transform_length;
               }
           }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Proposal_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_SA_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 SA payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_SA_Payload(UINT8 *buffer, UINT16 buffer_len,
                             IKE2_SA_PAYLOAD *pload)
{
    INT             i;
    UINT16          offset;
    STATUS          status = IKE_INVALID_PAYLOAD;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. We are assuming that
     * the generic header has been updated with the actual length of SA
     * payload.
     */
    if(pload->ike2_gen.ike2_payload_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode the IKEv2 generic header */
        status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
            &(pload->ike2_gen));

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to encode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* offset is used to track starting index of each proposal. */
            offset = IKE2_GEN_HDR_TOTAL_LEN;

            /* Initialize remaining buffer length. */
            buffer_len = buffer_len - offset;

            /* Loop to encode each proposal of the SA. */
            for (i = 0; i < pload->ike2_proposals_num ; i++)
            {
                status = IKE2_Encode_IKE_Proposal_Payload(buffer+offset,
                    buffer_len, &(pload->ike2_proposals[i]));

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Unable to encode proposal payload",
                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    /* Stop processing proposals chain. */
                    break;
                }

                /* Add each proposals length to obtain current index */
                offset = offset + pload->ike2_proposals[i].
                    ike2_proposal_len;

                /* Update the remaining buffer length */
                buffer_len = buffer_len - pload->ike2_proposals[i].
                    ike2_proposal_len;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_SA_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_SA_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 SA payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_DUPLICATE_PAYLOAD     Payload is already decoded.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_SA_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_SA_PAYLOAD *pload, UINT8 *next_pload)
{
    UINT8             i;
    UINT16          offset;
    STATUS          status = IKE_INVALID_PAYLOAD;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE2_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Check if enough data in buffer */
        if(IKE2_MIN_SA_PAYLOAD_LEN > buffer_len)
        {
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode the IKEv2 generic header */
            status = IKE2_Decode_IKE_Gen_Header(buffer, buffer_len,
                &(pload->ike2_gen), next_pload);

           if(status != NU_SUCCESS)
           {
               NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                   NERR_RECOVERABLE, __FILE__, __LINE__);
           }

           else
           {
               /* Check if actual payload length is less than buffer len */
               if (pload->ike2_gen.ike2_payload_length > buffer_len)
               {
                    status = IKE_LENGTH_IS_SHORT;
               }

               else
               {
                 /* Offset used to track starting index of each proposal */
                   offset = IKE2_GEN_HDR_TOTAL_LEN;

                   /* Initialize remaining buffer length. */
                    buffer_len = buffer_len - offset;

                   /* Loop to decode each proposal of the SA. */
                   for (i = 0; i < IKE2_NUM_OF_PROPOSALS; i++)
                   {
                       /* Need to allocate me*/
                       status = IKE2_Decode_IKE_Proposal_Payload(buffer +
                           offset, buffer_len, &(pload->ike2_proposals[i]));

                       if (status != NU_SUCCESS)
                       {
                         NLOG_Error_Log("Unable to decode proposal payload",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                           /* Stop processing proposals chain. */
                            break;
                       }

                       /*If this was the last proposal then stop processing
                        * proposals chain.
                        */
                       if (pload->ike2_proposals[i].
                          ike2_more_proposals == IKE2_NONE_PAYLOAD_ID)
                       {
                           break;
                       }

                       /* Add each proposals len to obtain current index. */
                       offset = offset + pload->ike2_proposals[i].
                           ike2_proposal_len;

                       /* Update the remaining buffer length. */
                       buffer_len = buffer_len - pload->
                           ike2_proposals[i].ike2_proposal_len;
                   }

                   /* Now we know how many proposals are there in the SA. */
                   if(i == IKE2_NUM_OF_PROPOSALS)
                   {
                       pload->ike2_proposals_num = i;
                   }

                   else
                   {
                       pload->ike2_proposals_num = i + 1;
                   }
               }
           }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_SA_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_KE_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Key Exchange payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*       IKE_INVALID_LENGTH      Key exchange data length doesn't match with
*                               the expected length according to
*                               Diffie-Hellman group number.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_KE_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_KE_PAYLOAD *pload)
{
    UINT16          group_len;
    UINT16          len_diff;
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(pload->ike2_gen.ike2_payload_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
            &(pload->ike2_gen));

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to encode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Encode DH group number */
            PUT16(buffer, IKE2_KE_DHG_NUM_OFST, pload->ike2_dh_group_no);

            /* Encode the reserved field ( 2 octets ) */
            PUT16(buffer, IKE2_KE_RSVRD_OFST, IKE2_RSVRD_VALUE);

            /* Verify if the length of key exchange data is correct as per
             * the Diffie-Hellman group number
             */
            group_len = IKE_Oakley_Group_Length(pload->ike2_dh_group_no);

            if(pload->ike2_ke_data_len > group_len)
            {
                /* if length is greater then set the error code */
                status = IKE_INVALID_LENGTH;
            }

            else
            {
                /* Pre-pend 0's in case data is less then required length;
                 * Take the difference first.
                 */
                len_diff = group_len - pload->ike2_ke_data_len;

                memset(buffer + IKE2_KE_DATA_OFST, IKE2_RSVRD_VALUE,
                              len_diff);

                /* Encode the key exchange data forwarding offset index if
                 * required.
                 */
                NU_BLOCK_COPY(buffer + IKE2_KE_DATA_OFST + len_diff,
                    pload->ike2_ke_data, pload->ike2_ke_data_len);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_KE_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_KE_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 KE payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_DUPLICATE_PAYLOAD   Payload is already decoded.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_INVALID_LENGTH      The Diffie-Hellman group indicated is not
*                               included.
*
*************************************************************************/

STATUS IKE2_Decode_IKE_KE_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_KE_PAYLOAD *pload, UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE2_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Check if enough data in buffer */
        if(IKE2_MIN_KE_PAYLOAD_LEN > buffer_len)
        {
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode the IKEv2 generic header */
            status = IKE2_Decode_IKE_Gen_Header(buffer, buffer_len,
                &(pload->ike2_gen), next_pload);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Check if actual payload length is less than buffer len */
                if(pload->ike2_gen.ike2_payload_length > buffer_len)
                {
                    status = IKE_LENGTH_IS_SHORT;
                }

                else
                {
                    pload->ike2_gen.ike2_payload_type = IKE2_KE_PAYLOAD_ID;

                    /* Decode the D-H Group number */
                    pload->ike2_dh_group_no = GET16(buffer,
                        IKE2_KE_DHG_NUM_OFST);

                    /* Calculate the key exchange data length and fill
                     * into KE struct
                     */
                    pload->ike2_ke_data_len = IKE_Oakley_Group_Length(
                        pload->ike2_dh_group_no);

                    /* if length is not valid. */
                    if(pload->ike2_ke_data_len ==
                        IKE2_KE_INVALID_GRP_LEN_VALUE)
                    {
                        status = IKE_INVALID_LENGTH;
                    }

                    else
                    {
                        /* Decode the key exchange data */
                        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                            (VOID**)(&pload->ike2_ke_data),
                            pload->ike2_ke_data_len, NU_NO_SUSPEND);

                        if(status == NU_SUCCESS)
                        {
                            NU_BLOCK_COPY(pload->ike2_ke_data, buffer +
                                IKE2_KE_DATA_OFST, pload->ike2_ke_data_len);
                        }

                        else
                        {
                            NLOG_Error_Log("Could not allocate memory for KE data",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_KE_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_ID_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Identification payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*       IKE_INVALID_LENGTH      The length calculated from generic header
*                               differs from the one in id structure
*************************************************************************/
STATUS IKE2_Encode_IKE_ID_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_ID_PAYLOAD *pload)
{
    UINT16          var_len;
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(pload->ike2_gen.ike2_payload_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
            &(pload->ike2_gen));

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to encode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Encode the ID type */
            PUT8(buffer, IKE2_ID_TYPE_OFST, pload->ike2_id_type);

            /* Encode the reserved field ( 3 octets ) */
            PUT8(buffer, IKE2_ID_RSVRD1_OFST, IKE2_RSVRD_VALUE);
            PUT16(buffer, IKE2_ID_RSVRD2_OFST, IKE2_RSVRD_VALUE);

            /* Check if id type is ipv4 or ipv6 then id data length should
             * comply to their respective values else the data length be
             * calculated from generic header.
             */
            if(pload->ike2_id_type == IKE2_ID_TYPE_IPV4_ADDR)
            {
                var_len = IKE2_ID_TYPE_IPV4_ADDR_VALUE;
            }
            else if(pload->ike2_id_type == IKE2_ID_TYPE_IPV6_ADDR)
            {
                var_len = IKE2_ID_TYPE_IPV6_ADDR_VALUE;
            }

            else /* variable data length must be calculated from gen hdr */
            {
                var_len = pload->ike2_gen.ike2_payload_length -
                    IKE2_MIN_ID_PAYLOAD_LEN;
            }

            if(pload->ike2_id_data_len != var_len)
            {
                /* if length is not valid then set the error code */
                status = IKE_INVALID_LENGTH;
            }

            else
            {
                /* Encode the identification data */
                NU_BLOCK_COPY(buffer + IKE2_ID_DATA_OFST, pload->
                    ike2_id_data, var_len);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_ID_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_ID_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 ID payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_DUPLICATE_PAYLOAD   Payload is already decoded.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_ID_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_ID_PAYLOAD *pload, UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE2_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Check if enough data in buffer */
        if(IKE2_MIN_ID_PAYLOAD_LEN > buffer_len)
        {
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode the IKEv2 generic header */
            status = IKE2_Decode_IKE_Gen_Header (buffer, buffer_len,
                &(pload->ike2_gen), next_pload);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Check if actual payload length is less than buffer len */
                if(pload->ike2_gen.ike2_payload_length > buffer_len)
                {
                    status = IKE_LENGTH_IS_SHORT;
                }

                else
                {
                    /* Decode the entire 32-bit field that contains the ID and reserved bits. */
                    pload->ike2_rsvd = GET32(buffer, IKE2_ID_TYPE_OFST);

                    /* Decode the ID type. */
                    pload->ike2_id_type = GET8(buffer, IKE2_ID_TYPE_OFST);

                    /* Check if id type is ipv4 or ipv6 then id data length
                     * should comply to their respective values else the
                     * data length be calculated from generic header.
                     */
                    if(pload->ike2_id_type == IKE2_ID_TYPE_IPV4_ADDR)
                    {
                        pload->ike2_id_data_len =
                            IKE2_ID_TYPE_IPV4_ADDR_VALUE;
                    }

                    else if(pload->ike2_id_type == IKE2_ID_TYPE_IPV6_ADDR)
                    {
                        pload->ike2_id_data_len =
                            IKE2_ID_TYPE_IPV6_ADDR_VALUE;
                    }

                    /* variable data len must be calculated from gen hdr. */
                    else
                    {
                        pload->ike2_id_data_len = pload->ike2_gen.
                            ike2_payload_length - IKE2_MIN_ID_PAYLOAD_LEN;
                    }

                    if(pload->ike2_id_data_len <= IKE_MAX_ID_DATA_LEN)
                    {
                        /* Decode the identification data */
                        NU_BLOCK_COPY(pload->ike2_id_data, buffer +
                            IKE2_ID_DATA_OFST, pload->ike2_id_data_len);
                    }

                    else
                    {
                        status = IKE_LENGTH_IS_SHORT;
                    }
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_ID_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Auth_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Authentication payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*       IKE_INVALID_LENGTH      The variable data length does not match the
*                               one computed from generic header
*************************************************************************/
STATUS IKE2_Encode_IKE_Auth_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_AUTH_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(pload->ike2_gen.ike2_payload_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
            &(pload->ike2_gen));

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to encode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Encode the authentication method */
            PUT8(buffer, IKE2_AUTH_METHOD_OFST, pload->ike2_auth_method);

            /* Encode the reserved field ( 3 octets ) */
            PUT8(buffer, IKE2_AUTH_RSVRD1_OFST, IKE2_RSVRD_VALUE);
            PUT16(buffer, IKE2_AUTH_RSVRD2_OFST, IKE2_RSVRD_VALUE);


            /* Variable data length must be equal to the value computed
             * from generic header
             */
            if(pload->ike2_auth_data_len != pload->ike2_gen.
                ike2_payload_length - IKE2_MIN_AUTH_PAYLOAD_LEN )
            {
                /* Set the error code since length is incorrect */
                status = IKE_INVALID_LENGTH;
            }

            else
            {
                /* Encode the authentication data */
                NU_BLOCK_COPY(buffer + IKE2_AUTH_DATA_OFST, pload->
                   ike2_auth_data, pload->ike2_auth_data_len);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Auth_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Auth_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Authentication payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_DUPLICATE_PAYLOAD   Payload is already decoded.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*************************************************************************/
STATUS IKE2_Decode_IKE_Auth_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_AUTH_PAYLOAD *pload, UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE2_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Check if enough data in buffer */
        if(IKE2_MIN_AUTH_PAYLOAD_LEN > buffer_len)
        {
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode the IKEv2 generic header */
            status = IKE2_Decode_IKE_Gen_Header (buffer, buffer_len,
                &(pload->ike2_gen), next_pload);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Check if actual payload length is less than buffer len */
                if(pload->ike2_gen.ike2_payload_length > buffer_len)
                {
                    status = IKE_LENGTH_IS_SHORT;
                }

                else
                {
                    /* Decode the authentication method. */
                    pload->ike2_auth_method = GET8(buffer,
                        IKE2_AUTH_METHOD_OFST);

                    /* Calculate variable data length from generic header.*/
                    pload->ike2_auth_data_len = pload->ike2_gen.
                        ike2_payload_length - IKE2_MIN_AUTH_PAYLOAD_LEN;

                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                                (VOID**)&pload->ike2_auth_data,
                                                pload->ike2_auth_data_len,
                                                NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        /* Decode the authentication data */
                        NU_BLOCK_COPY(pload->ike2_auth_data,
                                      buffer + IKE2_AUTH_DATA_OFST,
                                      pload->ike2_auth_data_len);
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to allocate memory for AUTH data",
                                       NERR_FATAL, __FILE__, __LINE__);
                    }
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Auth_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Nonce_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Nonce payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*       IKE_INVALID_LENGTH      The variable data length does not match the
*                               one computed from generic header
*
*************************************************************************/
STATUS IKE2_Encode_IKE_Nonce_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE2_NONCE_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(pload->ike2_gen.ike2_payload_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
            &(pload->ike2_gen));

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to encode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Data length should be equal to that calculated from the
             * generic header
             */
            if(pload->ike2_nonce_data_len != pload->ike2_gen.
                ike2_payload_length - IKE2_MIN_NONCE_PAYLOAD_LEN )
            {
                /* Set the error code since length is incorrect */
                status = IKE_INVALID_LENGTH;
            }

            else
            {
                /* Encode the nonce data */
                NU_BLOCK_COPY(buffer + IKE2_NONCE_DATA_OFST, pload->
                    ike2_nonce_data, pload->ike2_nonce_data_len);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Nonce_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Nonce_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Nonce payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_DUPLICATE_PAYLOAD   Payload is already decoded.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_Nonce_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE2_NONCE_PAYLOAD *pload, UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE2_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Check if enough data in buffer */
        if(IKE2_MIN_NONCE_PAYLOAD_LEN > buffer_len)
        {
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode the IKEv2 generic header */
            status = IKE2_Decode_IKE_Gen_Header (buffer, buffer_len,
                &(pload->ike2_gen), next_pload);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Check if buffer length is less than actual payload length. */
                if(pload->ike2_gen.ike2_payload_length > buffer_len)
                {
                    status = IKE_LENGTH_IS_SHORT;
                }

                else
                {
                    /* Variable data len must be calculated from gen hdr.*/
                    pload->ike2_nonce_data_len = pload->ike2_gen.
                        ike2_payload_length - IKE2_MIN_NONCE_PAYLOAD_LEN;

                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                        (VOID**)&pload->ike2_nonce_data, pload->ike2_nonce_data_len,
                        NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        /* Decode the nonce data */
                        NU_BLOCK_COPY(pload->ike2_nonce_data, buffer +
                            IKE2_NONCE_DATA_OFST, pload->ike2_nonce_data_len);
                    }

                    else
                    {
                        NLOG_Error_Log("Could not allocate memory for Nonce data",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Nonce_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Vendor_ID_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Vendor ID payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*       IKE_INVALID_LENGTH      The variable vendor id length does not match
*                               the one computed from generic header
*
*************************************************************************/
STATUS IKE2_Encode_IKE_Vendor_ID_Payload(UINT8 *buffer, UINT16 buffer_len,
                                     IKE2_VENDOR_ID_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(pload->ike2_gen.ike2_payload_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
            &(pload->ike2_gen));

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to encode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Vendor ID length should be equal to that calculated from the
             * generic header
             */
            if(pload->ike2_vendor_id_len != pload->ike2_gen.
                ike2_payload_length - IKE2_MIN_VENDOR_ID_PAYLOAD_LEN )
            {
                /* Set the error code since length is incorrect */
                status = IKE_INVALID_LENGTH;
            }

            else
            {
                /* Encode the vendor id */
                NU_BLOCK_COPY(buffer + IKE2_VENDOR_ID_OFST, pload->
                    ike2_vendor_id, pload->ike2_vendor_id_len);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Vendor_ID_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Vendor_ID_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Vendor ID payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_DUPLICATE_PAYLOAD   Payload is already decoded.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_Vendor_ID_Payload(UINT8 *buffer, UINT16 buffer_len,
                           IKE2_VENDOR_ID_PAYLOAD *pload, UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE2_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Check if enough data in buffer */
        if(IKE2_MIN_VENDOR_ID_PAYLOAD_LEN > buffer_len)
        {
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode the IKEv2 generic header */
            status = IKE2_Decode_IKE_Gen_Header(buffer, buffer_len,
                &(pload->ike2_gen), next_pload);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Check if actual payload length is less than buffer len */
                if(pload->ike2_gen.ike2_payload_length > buffer_len)
                {
                    status = IKE_LENGTH_IS_SHORT;
                }

                else
                {
                    /* Variable data len must be calculated from gen hdr.*/
                    pload->ike2_vendor_id_len = pload->ike2_gen.
                        ike2_payload_length - IKE2_MIN_VENDOR_ID_PAYLOAD_LEN;

                    /* Decode the vendor id */
                    NU_BLOCK_COPY(pload->ike2_vendor_id, buffer +
                        IKE2_VENDOR_ID_OFST, pload->ike2_vendor_id_len);
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Vendor_ID_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_TS
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Traffic Selector payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*       IKE_UNDEFINED_TS_TYPE   The traffic selector type is not defined
*                               and hence field lengths cannot be determined.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_TS(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_TS *pload)
{
    STATUS          status = NU_SUCCESS;

#if(IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(pload->ike2_selector_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode traffic selector type */
        PUT8(buffer, IKE2_TS_TYPE_OFST, pload->ike2_ts_type);

        /* Encode IP protocol ID */
        PUT8(buffer, IKE2_TS_PROTOCOL_ID_OFST, pload->ike2_ip_protocol_id);

        /* Encode the selector length */
        PUT16(buffer, IKE2_TS_SELEC_LEN_OFST, pload->ike2_selector_length);

        /* Encode starting port number */
        PUT16(buffer, IKE2_TS_START_PORT_OFST, pload->ike2_start_port);

        /* Encode ending port number */
        PUT16(buffer, IKE2_TS_END_PORT_OFST, pload->ike2_end_port);

        /* Encoding of addresses in a switch to handle different
         * address lengths
         */
        switch(pload->ike2_ts_type)
        {
            /* If TS type indicates it is an ipv4 address */
        case IKE2_TS_IPV4_ADDR_RANGE:

            /* Encode starting address */
            NU_BLOCK_COPY(buffer + IKE2_TS_IPV4_STRT_ADDR_OFST,
                &(pload->ike2_start_addr), IKE2_TS_IPV4_ADDR_VALUE);

            /* Encode ending address */
            NU_BLOCK_COPY(buffer + IKE2_TS_IPV4_END_ADDR_OFST,
                &(pload->ike2_end_addr), IKE2_TS_IPV4_ADDR_VALUE);
            break;

            /* If TS type indicates it is an ipv6 address */
        case IKE2_TS_IPV6_ADDR_RANGE:
            /* Encode starting address */
            NU_BLOCK_COPY(buffer + IKE2_TS_IPV6_STRT_ADDR_OFST,
                &(pload->ike2_start_addr), IKE2_TS_IPV6_ADDR_VALUE);

            /* Encode ending address */
            NU_BLOCK_COPY(buffer + IKE2_TS_IPV6_END_ADDR_OFST,
                &(pload->ike2_end_addr), IKE2_TS_IPV6_ADDR_VALUE);
            break;

        default:
            status = IKE_UNDEFINED_TS_TYPE;
            break;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_TS */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_TS
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Traffic Selector payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_DUPLICATE_PAYLOAD   Payload is already decoded.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_TS(UINT8 *buffer, UINT16 buffer_len, IKE2_TS *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* First decode the traffic selector length field */
    pload->ike2_selector_length = GET16(buffer,IKE2_TS_SELEC_LEN_OFST);

    /* Check if enough data in buffer */
    if(pload->ike2_selector_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Decode the Traffic selector type */
        pload->ike2_ts_type = GET8(buffer,IKE2_TS_TYPE_OFST);

        /* Decode the IP protocol ID */
        pload->ike2_ip_protocol_id = GET8(buffer, IKE2_TS_PROTOCOL_ID_OFST);

        /* Decode starting port number */
        pload->ike2_start_port = GET16(buffer, IKE2_TS_START_PORT_OFST);

        /* Decode ending port number */
        pload->ike2_end_port = GET16(buffer, IKE2_TS_END_PORT_OFST);

        /* Encoding of addresses in a switch to handle different
         * address lengths
         */
        switch(pload->ike2_ts_type)
        {
            /* If TS type indicates it is an ipv4 address */
        case IKE2_TS_IPV4_ADDR_RANGE:

            /* Encode starting address */
            NU_BLOCK_COPY( &(pload->ike2_start_addr), buffer +
                IKE2_TS_IPV4_STRT_ADDR_OFST, IKE2_TS_IPV4_ADDR_VALUE);

            /* Encode ending address */
            NU_BLOCK_COPY( &(pload->ike2_end_addr), buffer +
                IKE2_TS_IPV4_END_ADDR_OFST, IKE2_TS_IPV4_ADDR_VALUE);
            break;

            /* If TS type indicates it is an ipv6 address */
        case IKE2_TS_IPV6_ADDR_RANGE:
            /* Encode starting address */
            NU_BLOCK_COPY( &(pload->ike2_start_addr), buffer +
                IKE2_TS_IPV6_STRT_ADDR_OFST, IKE2_TS_IPV6_ADDR_VALUE);

            /* Encode ending address */
            NU_BLOCK_COPY( &(pload->ike2_end_addr), buffer +
                IKE2_TS_IPV6_END_ADDR_OFST, IKE2_TS_IPV6_ADDR_VALUE);
            break;

        default:
            status = IKE_UNDEFINED_TS_TYPE;
            break;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_TS */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_TS_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Traffic Selector Head payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*       IKE_INVALID_LENGTH      The variable vendor id length does not match
*                               the one computed from generic header
*
*************************************************************************/
STATUS IKE2_Encode_IKE_TS_Payload(UINT8 *buffer, UINT16 buffer_len,
                                 IKE2_TS_PAYLOAD *pload)
{
    INT             i;
    UINT16          offset;
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(pload->ike2_gen.ike2_payload_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
            &(pload->ike2_gen));

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to encode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Encode number of traffic selectors */
            PUT8(buffer, IKE2_TS_HEAD_NUM_OF_TS_OFST, pload->ike2_ts_count);

            /* Encode the reserved field ( 3 octets ) */
            PUT8(buffer, IKE2_TS_HEAD_RSVRD1_OFST, IKE2_RSVRD_VALUE);
            PUT16(buffer, IKE2_TS_HEAD_RSVRD2_OFST, IKE2_RSVRD_VALUE);

            /* offset is used to track starting index of each selector */
            offset = IKE2_TS_HEAD_SELECTORS_OFST;

            /* Initialize remaining buffer length. */
            buffer_len = buffer_len - offset;

            /* Loop to encode each traffic selector */
            for(i=0; i < pload->ike2_ts_count; i++)
            {
                status = IKE2_Encode_IKE_TS(buffer+offset,
                    buffer_len, &(pload->ike2_ts[i]));

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Unable to encode selector payload",
                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    /* Stop processing selectors chain. */
                    break;
                }

                /* Add each selectors length to obtain current index */
                offset = offset + pload->ike2_ts[i].
                    ike2_selector_length;

                /* Update the remaining buffer length */
                buffer_len = buffer_len - pload->ike2_ts[i].
                    ike2_selector_length;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_TS_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_TS_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Traffic Selector Head payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_DUPLICATE_PAYLOAD   Payload is already decoded.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_TS_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_TS_PAYLOAD *pload, UINT8 *next_pload)
{
    UINT8           i;
    UINT16          offset;
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE2_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Check if enough data in buffer */
        if( IKE2_MIN_TS_HEAD_PAYLOAD_LEN > buffer_len)
        {
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode the IKEv2 generic header */
            status = IKE2_Decode_IKE_Gen_Header(buffer, buffer_len,
                &(pload->ike2_gen), next_pload);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Check if actual payload length is less than buffer len */
                if(pload->ike2_gen.ike2_payload_length > buffer_len)
                {
                    status = IKE_LENGTH_IS_SHORT;
                }

                else
                {
                    /* Decode the number of traffic selectors */
                    pload->ike2_ts_count = (IKE2_MAX_SELECTORS_IN_TS < GET8(buffer,
                        IKE2_TS_HEAD_NUM_OF_TS_OFST)) ? IKE2_MAX_SELECTORS_IN_TS :
                        GET8(buffer,IKE2_TS_HEAD_NUM_OF_TS_OFST);


                    /* Offset used to track starting index of each selector */
                    offset = IKE2_TS_HEAD_SELECTORS_OFST;

                    /* Initialize remaining buffer length. */
                    buffer_len = buffer_len - offset;

                    /* Loop to decode each selector of the traffic selector
                     * head.
                     */
                    for(i = 0; i < pload->ike2_ts_count; i++)
                    {
                        status = IKE2_Decode_IKE_TS(buffer + offset,
                                                    buffer_len,
                                                    &(pload->ike2_ts[i]));

                        if (status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Unable to decode selector payload",
                                NERR_RECOVERABLE, __FILE__, __LINE__);

                            /* Stop processing selectors chain. */
                            break;
                        }

                        /* Add each selectors len to obtain current index */
                        offset = offset + pload->ike2_ts[i].
                            ike2_selector_length;

                        /* Update the remaining buffer length */
                        buffer_len = buffer_len - pload->
                            ike2_ts[i].ike2_selector_length;
                    }
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_TS_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Encrypted_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Encrypted payload. It includes
*       IKE Header and hence appends the check sum field and final
*       payload length in encrypted payload buffer. The length field in gen
*       header contains payload length including check sum.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode. It
*                               already contains IKE header and non encrypted
*                               payload chain.
*       buffer_len              Length of the buffer in bytes.
*       enc_p_chain_len         contains encoded payloads chain length
*       *sa                     Source sa payload pointer
*       first_pyld_type         Type of first payload in the encoded chain
*                               in buffer, required to fill gen hdr.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_Encrypted_Payload(UINT8 *buffer, UINT16 buffer_len,
                                         UINT16 enc_p_chain_len, IKE2_SA *sa,
                                         UINT8 first_payload_type)
{
    UINT8           *temp_buff;
    UINT8           iv_len;
    UINT8           pad_length;
    UINT16          enc_p_chain_ofst;
    UINT16          pad_start_ofst;
    UINT16          enc_text_len;
    UINT16          digest_input_len;
    UINT16          digest_len;
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (sa == NU_NULL) )
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Get length of IV. */
    status = IKE_Crypto_IV_Len(IKE_Encryption_Algos[sa->
        ike_attributes.ike_encryption_algo].crypto_algo_id, &iv_len);

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to get encryption block length",
            NERR_SEVERE, __FILE__, __LINE__);
    }

    else
    {
        /* Calculate encoded payload chain start offset since we have to
         * leave space for IKE header and encrypted payload header (generic
         * header plus length of iv)to be filled later.
         */
        enc_p_chain_ofst = IKE2_HDR_TOTAL_LEN + IKE2_GEN_HDR_TOTAL_LEN +
                           iv_len;

        /* Calculate pad start offset */
        pad_start_ofst = enc_p_chain_len;

        /* Calculate pad length using encoded payload chain length, length
         * of pad field and block size of encryption algorithm. Type cast
         * to UINT8 to get rid of warnings.
         */
        pad_length = iv_len - ((UINT8)(enc_p_chain_len +
                                        IKE2_ENCRYPTD_PAD_FIELD) % iv_len);

        /* Now compute length of text to be encrypted */
        enc_text_len = enc_p_chain_len + pad_length + IKE2_ENCRYPTD_PAD_FIELD;

        /* Check if buffer has enough space. This is the appropriate place
         * to do so because only now we know at least how long the buffer
         * should be, in order to store whole payload.
         */
        if(enc_p_chain_ofst + enc_text_len > buffer_len)
        {
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Now we need a temp buffer to store the the text to be
             * encrypted. We do not grab the IKE semaphore since it
             * is already acquired in the current call stack.
             */
            status = IKE_Allocate_Buffer(&temp_buff);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to Allocate buffer ",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Buffer is successfully allocated */
            else
            {
                /* Copy the data to be encrypted to the temp buffer.
                 * Payloads chain starts immediately after IKE Header in
                 * the input buffer.
                 */
                NU_BLOCK_COPY(temp_buff, buffer + IKE2_HDR_TOTAL_LEN,
                    enc_text_len);

                /* Encode the pad and pad length fields */
                memset(temp_buff + pad_start_ofst,
                    IKE2_ENCRYPTD_PAD_VALUE, pad_length);

                PUT8(temp_buff, pad_start_ofst + pad_length, pad_length);

                /* Now encrypt the encoded payload chain, pad and pad
                 * length. Upon success enc_text_len contains length of
                 * encrypted text.
                 */
                status = IKE_Encrypt(sa, temp_buff, IKE_MAX_BUFFER_LEN,
                    &enc_text_len,
                    sa->ike_encryption_iv,
                    sa->ike_decryption_iv, IKE_ENCRYPT);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to encrypt encoded payloads \
                                   chain", NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                /* If successfully encrypted then proceed with encoding */
                else
                {
                    /* Now encode generic header.
                     * Encode the next payload type. Here buffer points
                     * to the start of packet i.e. we have IKE header
                     * and then generic header so need to skip it as well.
                     */
                    PUT8(buffer, IKE2_HDR_TOTAL_LEN +
                        IKE2_GEN_HDR_NXT_PYLD_OFST, first_payload_type);

                    /* Encode the critical bit and reserved bits */
                    PUT8(buffer, IKE2_HDR_TOTAL_LEN + IKE2_GEN_HDR_CRITICAL_OFST,
                        (IKE2_GEN_HDR_CRTCL_VALUE << IKE2_GEN_HDR_CRITICAL_SHFT)
                        & IKE2_GEN_HDR_RSVRD_MASK);

                    /* Encode Initialization vector */
                    NU_BLOCK_COPY(buffer + IKE2_HDR_TOTAL_LEN +
                        IKE2_GEN_HDR_TOTAL_LEN,
                        sa->ike_encryption_iv, iv_len);

                    /* Copy the encrypted text back to original buffer.
                     * This time the payloads chain is copied after
                     * inserting Generic Header and IV in the buffer.
                     */
                    NU_BLOCK_COPY(buffer + enc_p_chain_ofst, temp_buff,
                        enc_text_len);

                    /* Compute the check sum data using Cipher suite API's.
                     * The hashing algorithm is copied from SA.
                     */
                    digest_input_len = IKE2_HDR_TOTAL_LEN +
                        IKE2_GEN_HDR_TOTAL_LEN +
                        iv_len + enc_text_len;

                    digest_len = IKE2_AUTH_HMAC_Algos[sa->
                        ike_attributes.ike_hash_algo].
                        ike2_output_len;

                    /* Encode the payload length in generic header
                     * including checksum data field. The field used
                     * below i.e. digest_input_len contains the complete
                     * length of payload except length of checksum
                     * data itself.
                     */
                    PUT16(buffer, IKE2_HDR_TOTAL_LEN +
                        IKE2_GEN_HDR_PYLD_LEN_OFST,
                        digest_input_len - IKE2_HDR_TOTAL_LEN +
                        digest_len);

                    /* IKE Header needs to be updated as well. */

                    /* Update the payload length in IKE Header */
                    PUT32(buffer, IKE2_HDR_LEN_OFST,
                        digest_input_len + digest_len);

                    /* Update the next payload field in IKE header. */
                    PUT8(buffer, IKE2_HDR_NXT_PYLD_OFST,
                        IKE2_ENCRYPT_PAYLOAD_ID);

                    status = IKE2_Compute_Checksum(sa, buffer,
                        digest_input_len, buffer +
                        digest_input_len, &digest_len,
                        IKE_ENCRYPT);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to calculate hash i.e. \
                                       checksum data ", NERR_RECOVERABLE,
                                       __FILE__, __LINE__);
                    }
                }

                /* Deallocate the temp buffer used for encryption. */
                if(IKE_Deallocate_Buffer(temp_buff) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to free IKE buffer",
                        NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }
    }

   /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Encrypted_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Encrypted_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Encrypted payload. It decrypts the
*       encrypted payload and place the resulting payloads back into the
*       same buffer.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode. Contains
*                               IKE Header and then encrypted payload. Upon
*                               return it contains the decrypted payload chain.
*       buffer_len              Length of the buffer in bytes.
*       enc_p_chain_len         Length of encrypted payload chain. This
*                               field contains len of original decrypted
*                               data excluding padding after wards.
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_Encrypted_Payload(UINT8 *buffer, UINT16 buffer_len,
                                         IKE2_SA *sa)
{
    STATUS          status = NU_SUCCESS;
    UINT8           *temp_buff;
    UINT8           iv_len;
    UINT8           pad_len;
    UINT8           checksum_len;
    UINT16          payload_len;           /* Excluding IKE Header */
    UINT16          encrypted_text_len;
    UINT16          enc_p_chain_ofst;
    UINT16          digest_input_len;
    UINT16          digest_len;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (sa == NU_NULL) )
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Decode the encrypted payload length from generic header */
    payload_len = GET16(buffer, IKE2_HDR_TOTAL_LEN + IKE2_GEN_HDR_PYLD_LEN_OFST);

    /* Check if enough data in buffer */
    if(payload_len > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Get length of IV. */
        status = IKE_Crypto_IV_Len(IKE_Encryption_Algos[sa->
            ike_attributes.ike_encryption_algo].crypto_algo_id, &iv_len);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to get encryption block length for iv length",
                NERR_SEVERE, __FILE__, __LINE__);
        }

        else
        {
            /* Calculate encoded and encrypted payload chain start offset */
            enc_p_chain_ofst = IKE2_HDR_TOTAL_LEN + IKE2_GEN_HDR_TOTAL_LEN
                               + iv_len;

            /* Compute the length of checksum. */
            checksum_len = IKE2_AUTH_HMAC_Algos[sa->ike_attributes.
                            ike_hash_algo].ike2_output_len;

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to obtain check sum length.",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Compute the encrypted text length. */
                encrypted_text_len = payload_len - IKE2_GEN_HDR_TOTAL_LEN -
                                     iv_len - checksum_len;

                /* Now we need a temp buffer to store the text to be
                 * decrypted. We do not grab the IKE semaphore here since
                 * it is already acquired in the present call stack.
                 * We need to store the entire packet's data after
                 * encrypting so allocate an IKE buffer which is the
                 * size of maximum allowed IKE packet.
                 */
                status = IKE_Allocate_Buffer(&temp_buff);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to Allocate buffer ",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                /* Buffer is successfully allocated */
                else
                {
                    /* We need to compute check sum and verify if it
                     * is the same as sent with the message.
                     */
                    digest_input_len = IKE2_HDR_TOTAL_LEN + payload_len -
                                       checksum_len;

                    /* Now calculate new check sum. */
                    digest_len = checksum_len;

                    status = IKE2_Compute_Checksum(sa, buffer,
                                                    digest_input_len,
                                                    temp_buff, &digest_len,
                                                    IKE_DECRYPT);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to calculate hash i.e. \
                                       checksum data ",
                                       NERR_RECOVERABLE, __FILE__,
                                       __LINE__);
                    }

                    else
                    {
                        /* It means that check sum data has been
                         * successfully encoded after the encrypted data
                         * in the buffer.
                         */

                        /* Now compare both the checksums */
                        status = memcmp(temp_buff, buffer + digest_input_len,
                                        checksum_len);

                        /* If digests didn't match then return error. */
                        if(status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Check sum failed to match",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }

                        /* Proceed with the decoding */
                        else
                        {
                            /* Copy the data to be decrypted to temp buffer. */
                            NU_BLOCK_COPY(temp_buff,
                                          buffer + enc_p_chain_ofst,
                                          encrypted_text_len);

                            NU_BLOCK_COPY(sa->ike_decryption_iv,
                                          buffer + IKE2_HDR_TOTAL_LEN +
                                          IKE2_GEN_HDR_TOTAL_LEN, iv_len);

                            /* Decrypt the payload chain */
                            status = IKE_Encrypt(sa, temp_buff,
                                                 IKE_MAX_BUFFER_LEN,
                                                 &encrypted_text_len,
                                                 sa->ike_decryption_iv,
                                                 sa->ike_encryption_iv,
                                                 IKE_DECRYPT);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Failed to decrypt payloads \
                                               chain", NERR_RECOVERABLE,
                                               __FILE__, __LINE__);
                            }

                            else
                            {
                                /* Decrypted payload chain still contains
                                 * padding. Decode the pad length field.
                                 */
                                pad_len = GET8(temp_buff, encrypted_text_len -
                                               IKE2_ENCRYPTD_PAD_FIELD);

                                /* Compute actual length of decrypted
                                 * payload chain.
                                 */
                                encrypted_text_len = encrypted_text_len -
                                                     pad_len -
                                                     IKE2_ENCRYPTD_PAD_FIELD;

                                /* Finally update the IKE Header length
                                 * and Next payload fields.
                                 */
                                PUT32(buffer, IKE2_HDR_LEN_OFST,
                                      IKE2_HDR_TOTAL_LEN + encrypted_text_len);

                                PUT8(buffer, IKE2_HDR_NXT_PYLD_OFST,
                                     GET8(buffer, IKE2_HDR_TOTAL_LEN +
                                     IKE2_GEN_HDR_NXT_PYLD_OFST));

                                /* Copy the decrypted payloads back to
                                 * buffer right after IKE header.
                                 */
                                NU_BLOCK_COPY(buffer + IKE2_HDR_TOTAL_LEN,
                                              temp_buff, encrypted_text_len);
                            }
                        }
                    }

                    /* Deallocate the temp buffer used for encryption. */
                    if(IKE_Deallocate_Buffer(temp_buff) !=
                        NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to free IKE buffer",
                            NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Encrypted_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Cert_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Certificate payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_Cert_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE2_CERT_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Calculate payload length. */
    pload->ike2_gen.ike2_payload_length =
        (IKE2_MIN_CERT_PAYLOAD_LEN + pload->ike2_cert_data_len);

    /* Check if destination buffer has enough space. */
    if(pload->ike2_gen.ike2_payload_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
            &pload->ike2_gen);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to encode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Encode Certificate encoding. */
            PUT8(buffer, IKE2_CERT_ENCODING_OFFSET, pload->ike2_cert_encoding);

            /* Encode Certificate data. */
            NU_BLOCK_COPY(buffer + IKE2_CERT_DATA_OFFSET,
                pload->ike2_cert_data, pload->ike2_cert_data_len);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Cert_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Cert_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Certificate payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*       *next_pload             On return, contains next payload type.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_Cert_Payload(UINT8 *buffer, UINT16 buffer_len,
                               IKE2_CERT_PAYLOAD *pload, UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
        (next_pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE2_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        status = IKE2_Decode_IKE_Gen_Header(buffer, buffer_len,
            &pload->ike2_gen, next_pload);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Check if enough data in buffer. */
            if(pload->ike2_gen.ike2_payload_length > buffer_len)
            {
                /* Set error status. */
                status = IKE_LENGTH_IS_SHORT;
            }

            /* Check if enough data in payload. */
            else if(pload->ike2_gen.ike2_payload_length <
                IKE2_MIN_CERT_PAYLOAD_LEN)
            {
                /* Set error status. */
                status = IKE_LENGTH_IS_SHORT;
            }

            else
            {
                pload->ike2_gen.ike2_payload_type = IKE2_CERT_PAYLOAD_ID;

                /* Decode Certificate encoding. */
                pload->ike2_cert_encoding = GET8(buffer,
                    IKE2_CERT_ENCODING_OFFSET);

                /* Decode Certificate data. */
                pload->ike2_cert_data = buffer + IKE2_CERT_DATA_OFFSET;
                pload->ike2_cert_data_len = (pload->ike2_gen.
                    ike2_payload_length - IKE2_MIN_CERT_PAYLOAD_LEN);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Cert_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_CertReq_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Certificate request payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_CertReq_Payload(UINT8 *buffer, UINT16 buffer_len,
                                       IKE2_CERT_REQ_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Calculate payload length. */
    pload->ike2_gen.ike2_payload_length =
        (IKE2_MIN_CERT_REQ_PAYLOAD_LEN + pload->ike2_ca_len);

    /* Check if destination buffer has enough space. */
    if(pload->ike2_gen.ike2_payload_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
            &pload->ike2_gen);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to encode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Encode Certificate encoding. */
            PUT8(buffer, IKE2_CERTREQ_ENCODING_OFFSET, pload->ike2_cert_encoding);

            /* Encode Certificate Authority. */
            NU_BLOCK_COPY(buffer + IKE2_CERTREQ_AUTH_OFFSET,
                pload->ike2_ca, pload->ike2_ca_len);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_CertReq_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_CertReq_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Certificate Request payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*       *next_pload             On return, contains next payload type.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_CertReq_Payload(UINT8 *buffer, UINT16 buffer_len,
                           IKE2_CERT_REQ_PAYLOAD *pload, UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
        (next_pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE2_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        status = IKE2_Decode_IKE_Gen_Header(buffer, buffer_len,
            &pload->ike2_gen, next_pload);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Check if enough data in buffer. */
            if(pload->ike2_gen.ike2_payload_length > buffer_len)
            {
                /* Set error status. */
                status = IKE_LENGTH_IS_SHORT;
            }

            /* Check if enough data in payload. */
            else if(pload->ike2_gen.ike2_payload_length <
                IKE2_MIN_CERT_REQ_PAYLOAD_LEN)
            {
                /* Set error status. */
                status = IKE_LENGTH_IS_SHORT;
            }

            else
            {
                pload->ike2_gen.ike2_payload_type =
                    IKE2_CERTREQ_PAYLOAD_ID;

                /* Decode Certificate encoding. */
                pload->ike2_cert_encoding = GET8(buffer,
                    IKE2_CERTREQ_ENCODING_OFFSET);

                /* Decode Certificate data. */
                pload->ike2_ca = buffer + IKE2_CERTREQ_AUTH_OFFSET;
                pload->ike2_ca_len = (pload->ike2_gen.
                    ike2_payload_length - IKE2_MIN_CERT_REQ_PAYLOAD_LEN);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_CertReq_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Notify_Payload
*
* DESCRIPTION
*
*       This function encodes all IKEv2 Notification payloads in a chain.
*       Please note that it is the responsibility of the caller function to
*       allocate memory for each notify data field in Notify payload and
*       pass it to the function.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_Notify_Payload(UINT8 *buffer, UINT16 buffer_len,
                                      IKE2_NOTIFY_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Loop to encode all notification payloads chain. */
    while(pload != NU_NULL)
    {
        /* Calculate payload length. */
        pload->ike2_gen.ike2_payload_length = (IKE2_MIN_NOTIFY_PAYLOAD_LEN
            + pload->ike2_spi_size + pload->ike2_notify_data_len);

        /* Check if destination buffer has enough space. */
        if(pload->ike2_gen.ike2_payload_length > buffer_len)
        {
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Encode generic header. */
            status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
                &pload->ike2_gen);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to encode IKEv2 generic header.",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Encode Protocol ID. */
                PUT8(buffer, IKE2_NOTIFY_PROTO_ID_OFFSET,
                    pload->ike2_protocol_id);

                /* Encode SPI size. */
                PUT8(buffer, IKE2_NOTIFY_SPI_LEN_OFFSET, pload->ike2_spi_size);

                /* Encode Notify Message Type. */
                PUT16(buffer, IKE2_NOTIFY_MSG_TYPE_OFFSET, pload->
                    ike2_notify_message_type);

                /* If SPI is present. */
                if(pload->ike2_spi_size != 0)
                {
                    /* Encode the SPI. */
                    NU_BLOCK_COPY(buffer + IKE2_NOTIFY_SPI_OFFSET,
                        pload->ike2_spi, pload->ike2_spi_size);
                }

                /* If notification data is present. */
                if(pload->ike2_notify_data_len != 0)
                {
                    /* Encode Notification data. */
                    NU_BLOCK_COPY(buffer + IKE2_NOTIFY_SPI_OFFSET +
                        pload->ike2_spi_size, pload->ike2_notify_data,
                        pload->ike2_notify_data_len);
                }
            }
        }

        /* Move the buffer pointer ahead to encode the next payload. */
        buffer = buffer + pload->ike2_gen.ike2_payload_length;

        /* Decrease the parameter buffer length by one payload length. */
        buffer_len = buffer_len - pload->ike2_gen.ike2_payload_length;

        /* Move pointer to the next notification payload in the chain. */
        pload = pload->ike2_next;
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Notify_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Notify_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Notification payload. It allocates
*       Memory for a Notify payload and notification data field itself and
*       appends it to the end of existing chain of Notify Payloads.
*
* INPUTS
*
*       *buffer                 Source buffer from which to encode.
*       buffer_len              Length of the buffer in bytes.
*       **pload                  Pointer to the destination payload.
*       *next_pload             On return, contains next payload type.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_Notify_Payload(UINT8 *buffer, UINT16 buffer_len,
                                      IKE2_NOTIFY_PAYLOAD **pload,
                                      UINT8 *next_pload)
{
    STATUS              status = NU_SUCCESS;
    IKE2_NOTIFY_PAYLOAD *new_payload;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid except notify pload since we will
     * allocate that with in this function.
     */
    if((buffer  == NU_NULL) || (next_pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* If first payload then simply allocated memory for it. */
    if(*pload == NU_NULL)
    {
        /* Now allocated Memory for the current payload. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID **)pload,
                     sizeof(IKE2_NOTIFY_PAYLOAD), NU_NO_SUSPEND);

        UTL_Zero(*pload, sizeof(IKE2_NOTIFY_PAYLOAD));
    }

    else
    {
        /* Allocate Memory for the new payload. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID **)&(new_payload),
                                    sizeof(IKE2_NOTIFY_PAYLOAD),
                                    NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            UTL_Zero(new_payload, sizeof(IKE2_NOTIFY_PAYLOAD));

            /* Set the next pointer of the new payload to point to the old payload. */
            new_payload->ike2_next = *pload;

            /* Move pointer ahead to point to the newly allocated payload. */
            *pload = new_payload;
        }
    }

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for Notify Payload.",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    else
    {
         /* Now proceeding ahead with decoding current payload.
          * Decode generic header.
          */
         status = IKE2_Decode_IKE_Gen_Header(buffer, buffer_len,
                      &((*pload)->ike2_gen), next_pload);

         if(status != NU_SUCCESS)
         {
             NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                 NERR_RECOVERABLE, __FILE__, __LINE__);
         }

         else
         {
             /* Check if enough data in buffer. */
             if((*pload)->ike2_gen.ike2_payload_length > buffer_len)
             {
                 /* Set error status. */
                  status = IKE_LENGTH_IS_SHORT;
             }

             else
             {
                 /* Decode Protocol ID. */
                 (*pload)->ike2_protocol_id = GET8(buffer,
                     IKE2_NOTIFY_PROTO_ID_OFFSET);

                 /* Decode SPI size. */
                 (*pload)->ike2_spi_size = GET8(buffer,
                     IKE2_NOTIFY_SPI_LEN_OFFSET);

                 /* Decode Notify Message Type. */
                 (*pload)->ike2_notify_message_type = GET16(buffer,
                     IKE2_NOTIFY_MSG_TYPE_OFFSET);

                 /* Decode the SPI. */
                 if((*pload)->ike2_spi_size != 0)
                 {
                     NU_BLOCK_COPY((*pload)->ike2_spi, buffer +
                        IKE2_NOTIFY_SPI_OFFSET, (*pload)->ike2_spi_size);
                 }

                 /* Compute the Notification data length. */
                 (*pload)->ike2_notify_data_len =
                    ((*pload)->ike2_gen.ike2_payload_length -
                    (*pload)->ike2_spi_size - IKE2_MIN_NOTIFY_PAYLOAD_LEN);

                 /* Decode Notification data. */
                 if((*pload)->ike2_notify_data_len != 0)
                 {
                     /* Need to Allocate memory for notify data field first. */
                     status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                  (VOID**)&(*pload)->ike2_notify_data,
                                  (*pload)->ike2_notify_data_len,
                                   NU_NO_SUSPEND);

                     if(status != NU_SUCCESS)
                     {
                         NLOG_Error_Log("Failed to allocate memory for \
                             notify data field.", NERR_RECOVERABLE,
                             __FILE__, __LINE__);
                     }

                     else
                     {
                         /* Memory allocated successfully so decode
                          * notification data.
                          */
                         NU_BLOCK_COPY((*pload)->ike2_notify_data, buffer +
                              IKE2_NOTIFY_SPI_OFFSET + (*pload)->
                              ike2_spi_size, (*pload)->ike2_notify_data_len);
                     }
                 }
             }
         }
     }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Notify_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Delete_Payload
*
* DESCRIPTION
*
*       This function encodes all IKEv2 Delete payloads in a chain.
*       Please note that it is the responsibility of the caller function to
*       allocate memory for each SPI's field in Delete payload and
*       pass it to the function.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_Delete_Payload(UINT8 *buffer, UINT16 buffer_len,
                                 IKE2_DELETE_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Loop to encode all delete payloads chain. */
    while(pload != NU_NULL)
    {
        /* Calculate payload length. */
        pload->ike2_gen.ike2_payload_length = (IKE2_MIN_DELETE_PAYLOAD_LEN
            + (pload->ike2_spi_size * pload->ike2_no_of_spis));

        /* Check if destination buffer has enough space. */
        if(pload->ike2_gen.ike2_payload_length > buffer_len)
        {
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Encode generic header. */
            status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
                &pload->ike2_gen);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to encode IKEv2 generic header",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Encode Protocol ID. */
                PUT8(buffer, IKE2_DELETE_PROTO_ID_OFFSET,
                    pload->ike2_protocol_id);

                /* Encode SPI size. */
                PUT8(buffer, IKE2_DELETE_SPI_SIZE_OFFSET,
                    pload->ike2_spi_size);

                /* Encode number of SPIs. */
                PUT16(buffer, IKE2_DELETE_NUM_OF_SPIS_OFFSET,
                    pload->ike2_no_of_spis);

                /* Encode the SPI data. */
                NU_BLOCK_COPY(buffer + IKE2_DELETE_SPIS_OFFSET,
                    pload->ike2_spi_data,
                    pload->ike2_spi_size * pload->ike2_no_of_spis);
            }
        }

        /* Move the buffer pointer ahead to encode the next payload. */
        buffer = buffer + pload->ike2_gen.ike2_payload_length;

        /* Decrease the parameter buffer length by one payload length. */
        buffer_len = buffer_len - pload->ike2_gen.ike2_payload_length;

        /* Move pointer to the next delete payload in the chain. */
        pload = pload->ike2_next;
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Delete_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Delete_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Delete payload. It allocates Memory
*       for a Delete payload and SPI field itself and appends it to the end
*       of existing chain of Delete Payloads.
*
* INPUTS
*
*       *buffer                 Source buffer from which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*       *next_pload             On return, contains next payload type.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_Delete_Payload(UINT8 *buffer, UINT16 buffer_len,
                                      IKE2_DELETE_PAYLOAD **pload,
                                      UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid except delete pload since we will
     * allocate that with in this function.
     */
    if((buffer == NU_NULL) || (next_pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* If first payload then simply allocated memory for it. */
    if(*pload == NU_NULL)
    {
        /* Now allocated Memory for the current payload. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID **)pload,
            sizeof(IKE2_DELETE_PAYLOAD), NU_NO_SUSPEND);
    }

    else
    {
        /* Otherwise, traverse the Delete payload chain to reach the point
         * to insert new payload.
         */
        while((*pload)->ike2_next != NU_NULL)
        {
            (*pload) = (*pload)->ike2_next;
        }

        /* Now allocated Memory for the current payload. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID **)&(*pload)->ike2_next,
                                    sizeof(IKE2_DELETE_PAYLOAD),
                                    NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* Move pointer ahead to point to the newly allocated payload. */
            (*pload) = (*pload)->ike2_next;
        }
    }

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for Delete Payload.",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    else
    {
        /* Clear the memory. */
        UTL_Zero ((*pload), sizeof(IKE2_DELETE_PAYLOAD));

        /* Now proceeding ahead with decoding current payload.
         * Decode generic header.
         */
        status = IKE2_Decode_IKE_Gen_Header(buffer, buffer_len,
            &(*pload)->ike2_gen, next_pload);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Check if enough data in buffer. */
            if((*pload)->ike2_gen.ike2_payload_length > buffer_len)
            {
                /* Set error status. */
                status = IKE_LENGTH_IS_SHORT;
            }

            else
            {
                /* Decode Protocol ID. */
                (*pload)->ike2_protocol_id = GET8(buffer,
                    IKE2_DELETE_PROTO_ID_OFFSET);

                /* Decode SPI size. */
                (*pload)->ike2_spi_size =
                    GET8(buffer, IKE2_DELETE_SPI_SIZE_OFFSET);

                /* Decode number of SPIs. */
                (*pload)->ike2_no_of_spis =
                    GET16(buffer, IKE2_DELETE_NUM_OF_SPIS_OFFSET);

                /* Make sure payload length is correct. */
                if(((*pload)->ike2_no_of_spis != 0) &&
                    ((*pload)->ike2_gen.ike2_payload_length !=
                    (IKE2_MIN_DELETE_PAYLOAD_LEN +
                    ((*pload)->ike2_spi_size * (*pload)->ike2_no_of_spis))))
                {
                    /* Set error status. */
                    status = IKE_INVALID_PAYLOAD;
                }

                else
                {
                    if((*pload)->ike2_spi_size > 0)
                    {
                        if((*pload)->ike2_spi_size <= IKE2_NOTIFY_SPI_SIZE)
                        {
                            /* Copy the SPI data. */
                            NU_BLOCK_COPY((*pload)->ike2_spi_data,
                                          buffer + IKE2_DELETE_SPIS_OFFSET,
                                          (*pload)->ike2_spi_size *
                                          (*pload)->ike2_no_of_spis);
                        }
                    }
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Delete_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_Config_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Configuration payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_Config_Payload(UINT8 *buffer, UINT16 buffer_len,
                                      IKE2_CONFIG_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;
    UINT16           offset;
    UINT16           i;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(pload->ike2_gen.ike2_payload_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
            &pload->ike2_gen);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to encode IKEv2 generic header",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Encode CFG Type field. */
            PUT8(buffer, IKE2_CONFIG_CFG_TYPE_OFFSET, pload->ike2_cfg_type);

            /* Encode Reserved bytes. */
            PUT8(buffer, IKE2_CONFIG_RSVRD1_OFFSET, IKE2_RSVRD_VALUE);
            PUT16(buffer, IKE2_CONFIG_RSVRD2_OFFSET, IKE2_RSVRD_VALUE);

            /* This variable is used to keep track of current offset. */
            offset = IKE2_CONFIG_ATTRIBS_OFFSET;

            /* Loop to encode configuration attributes. */
            for(i=0; i < pload->ike2_num_config_attribs; i++)
            {
                /* Encode reserved bit and attribute type. */
                PUT16(buffer, offset, IKE2_CONFIG_RSVRD_MASK |
                    pload->ike2_cfg_attributes[i].ike2_attrib_type);

                /* Encode attribute length. */
                PUT16(buffer, offset + IKE2_CONFIG_ATTRIB_LEN_OFFSET,
                    pload->ike2_cfg_attributes[i].ike2_attrib_len);

                /* Encode attribute value if length is not zero. */
                if(pload->ike2_cfg_attributes[i].ike2_attrib_len > 0)
                {
                    NU_BLOCK_COPY(buffer + offset + IKE2_CONFIG_ATTRIB_VALUE_OFFSET,
                        pload->ike2_cfg_attributes[i].ike2_attrib_value,
                        pload->ike2_cfg_attributes[i].ike2_attrib_len);
                }

                /* Increment offset to point at the start of next attribute. */
                offset = offset + IKE2_MIN_CFG_ATTRIB_PAYLOAD_LEN +
                    pload->ike2_cfg_attributes[i].ike2_attrib_len;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_Config_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_Config_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 Config payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*       *next_pload             On return, contains next payload type.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_Config_Payload(UINT8 *buffer, UINT16 buffer_len,
                                      IKE2_CONFIG_PAYLOAD *pload, UINT8 *next_pload)
{
    STATUS           status = NU_SUCCESS;
    UINT16           offset;
    UINT16           i;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
        (next_pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE2_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        status = IKE2_Decode_IKE_Gen_Header(buffer, buffer_len,
            &pload->ike2_gen, next_pload);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Check if enough data in buffer. */
            if(pload->ike2_gen.ike2_payload_length > buffer_len)
            {
                /* Set error status. */
                status = IKE_LENGTH_IS_SHORT;
            }

            else
            {
               /* Decode CFG Type. */
               pload->ike2_cfg_type = GET8(buffer, IKE2_CONFIG_CFG_TYPE_OFFSET);

               /* This field should be pointing to start of each attribute
                * after every iteration and helps to compute the number of
                * attributes by loop termination.
                */
               offset = IKE2_CONFIG_ATTRIBS_OFFSET;

               /* Loop to decode each configuration attribute. */
               for(i=0; offset < pload->ike2_gen.ike2_payload_length;
                   i++)
               {
                   /* Decode the reserved field and attribute type. Reserved
                    * value is ignored as per RFC 4306.
                    */
                   pload->ike2_cfg_attributes[i].ike2_attrib_type =
                       GET16(buffer, offset);

                   /* Decode the attribute length. */
                   pload->ike2_cfg_attributes[i].ike2_attrib_len =
                       GET16(buffer, offset + IKE2_CONFIG_ATTRIB_LEN_OFFSET);

                   /* If length is positive then decode the attribute value. */
                   if(pload->ike2_cfg_attributes[i].ike2_attrib_len > 0)
                   {
                       NU_BLOCK_COPY(pload->ike2_cfg_attributes[i].ike2_attrib_value,
                           buffer + offset + IKE2_CONFIG_ATTRIB_VALUE_OFFSET,
                            pload->ike2_cfg_attributes[i].ike2_attrib_len);
                   }

                   /* Increment offset to point to the next attribute. */
                   offset = offset + IKE2_MIN_CFG_ATTRIB_PAYLOAD_LEN +
                        pload->ike2_cfg_attributes[i].ike2_attrib_len;
               }

               /* Decode number of attributes. It is equal to the iterations
                * of above loop.
                */
               pload->ike2_num_config_attribs = (UINT8)i+1;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Config_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_IKE_EAP_Payload
*
* DESCRIPTION
*
*       This function encodes an IKEv2 Extensible Authentication payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE2_Encode_IKE_EAP_Payload(UINT8 *buffer, UINT16 buffer_len,
                                      IKE2_EAP_PAYLOAD *pload)
{
    STATUS           status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(pload->ike2_gen.ike2_payload_length > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        status = IKE2_Encode_IKE_Gen_Header(buffer, buffer_len,
            &pload->ike2_gen);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to encode IKEv2 generic header",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Encode Message Code field. */
            PUT8(buffer, IKE2_EAP_MSG_CODE_OFFSET,
                pload->ike2_eap_msg.ike2_code);

            /* Encode Message Identifier field. */
            PUT8(buffer, IKE2_EAP_MSG_IDENTIFIER_OFFSET,
                pload->ike2_eap_msg.ike2_identifier);

            /* Encode Message Length field.*/
            PUT16(buffer, IKE2_EAP_MSG_LEN_OFFSET,
                pload->ike2_eap_msg.ike2_length);

            /* Encode Message Type field. */
            PUT8(buffer, IKE2_EAP_MSG_TYPE_OFFSET,
                pload->ike2_eap_msg.ike2_type);

            /* Encode Message Type Data field. */
            NU_BLOCK_COPY(buffer + IKE2_EAP_MSG_TYPE_DATA_OFFSET,
                pload->ike2_eap_msg.ike2_eap_data, pload->
                ike2_eap_msg.ike2_length - IKE2_MIN_EAP_MSG_LEN);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Encode_IKE_EAP_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_IKE_EAP_Payload
*
* DESCRIPTION
*
*       This function decodes an IKEv2 EAP payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*       *next_pload             On return, contains next payload type.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*
*************************************************************************/
STATUS IKE2_Decode_IKE_EAP_Payload(UINT8 *buffer, UINT16 buffer_len,
                                      IKE2_EAP_PAYLOAD *pload, UINT8 *next_pload)
{
    STATUS           status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
        (next_pload == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE2_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        status = IKE2_Decode_IKE_Gen_Header(buffer, buffer_len,
            &pload->ike2_gen, next_pload);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to decode IKEv2 generic header.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Check if enough data in buffer. */
            if(pload->ike2_gen.ike2_payload_length > buffer_len)
            {
                /* Set error status. */
                status = IKE_LENGTH_IS_SHORT;
            }

            else
            {
                /* Decode Message Code Field. */
                pload->ike2_eap_msg.ike2_code =
                    GET8(buffer, IKE2_EAP_MSG_TYPE_OFFSET);

                /* Decode Message Identifier Field. */
                pload->ike2_eap_msg.ike2_identifier =
                    GET8(buffer, IKE2_EAP_MSG_IDENTIFIER_OFFSET);

                /* Decode Message Length Field. */
                pload->ike2_eap_msg.ike2_length =
                    GET16(buffer, IKE2_EAP_MSG_TYPE_OFFSET);

                /* Decode Message Type Field. */
                NU_BLOCK_COPY(pload->ike2_eap_msg.ike2_eap_data,
                    buffer + IKE2_EAP_MSG_TYPE_DATA_OFFSET,
                    pload->ike2_eap_msg.ike2_length - IKE2_MIN_EAP_MSG_LEN);

            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Decode_IKE_Config_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Encode_Message
*
* DESCRIPTION
*
*       Encodes a complete IKEv2 message into buffer that will be sent.
*
* INPUTS
*
*       *buffer                 Output buffer.
*       *enc_msg                Message to be encoded.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Encode_Message(UINT8 *buffer, IKE2_MESSAGE *enc_msg)
{
    STATUS              status = NU_SUCCESS;
    IKE2_GEN_HDR        *current_payload;
    UINT8               *temp_buff;
    UINT16              length;

#if (IKE2_DEBUG == NU_TRUE)
    if((buffer == NU_NULL) || (enc_msg == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Skip encoding the IKEv2 header for now. We need to update some
     * fields in this header after the other payloads have been encoded.
     */
    temp_buff = buffer + IKE2_HDR_TOTAL_LEN;
    length = IKE_MAX_BUFFER_LEN - IKE2_HDR_TOTAL_LEN;

    current_payload = enc_msg->ike2_hdr->ike2_gen.ike2_next_payload;

    /* Loop for each payload in the chain and encode it by calling the
     * appropriate function from the array of encoding functions. Where 33 is
     * the lowest payload id other IKE2_NONE_PAYLOAD_ID that does not require
     * any processing.
     */
    while ( (current_payload != NU_NULL) &&
            (current_payload->ike2_payload_type - 32 >= 0) &&
            (current_payload->ike2_payload_type - 32 < IKE2_ENCODE_FUNC_COUNT) )
    {
        status =
            IKE2_Encoding_Funcs[current_payload->ike2_payload_type - 32](
                temp_buff, length, current_payload);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Error while encoding payload", NERR_RECOVERABLE,
                __FILE__, __LINE__);
            break;
        }

        /* Payload encoded. Now update pointers for next payload. */
        temp_buff = temp_buff + current_payload->ike2_payload_length;
        length = length - current_payload->ike2_payload_length;

        current_payload = current_payload->ike2_next_payload;
    } /* while() */

    if(status == NU_SUCCESS)
    {
        enc_msg->ike2_hdr->ike2_length = IKE_MAX_BUFFER_LEN - length;

        /* If there are no payloads in chain (as can be the case with
         * empty informational message), set the next payload type to
         * IKE2_NONE_PAYLOAD_ID.
         */
        if(enc_msg->ike2_hdr->ike2_gen.ike2_next_payload != NU_NULL)
        {
            enc_msg->ike2_hdr->ike2_next_payload =
                enc_msg->ike2_hdr->ike2_gen.ike2_next_payload->ike2_payload_type;
        }

        else
        {
            enc_msg->ike2_hdr->ike2_next_payload = IKE2_NONE_PAYLOAD_ID;
        }

        status = IKE2_Encode_IKE_Header(buffer, IKE2_HDR_TOTAL_LEN,
                                        enc_msg->ike2_hdr);
        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Error while encoding IKEv2 header",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    return (status);
} /* IKE2_Encode_Message */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Decode_Message
*
* DESCRIPTION
*
*       Decodes an incoming message.
*
* INPUTS
*
*       *buffer                 Incoming buffer.
*       *dec_msg                Decoded message.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Decode_Message(UINT8 *buffer, IKE2_MESSAGE *dec_msg)
{
    STATUS          status = NU_SUCCESS;
    UINT16          buff_len = IKE_MAX_RECV_BUFFER_LEN;
    UINT8           next_payload;
    IKE2_GEN_HDR    *current_payload = NU_NULL;
    IKE2_GEN_HDR    unreco_pay;

#if (IKE2_DEBUG == NU_TRUE)
    /* Make sure incoming parameters are valid pointers. */
    if((buffer == NU_NULL) || (dec_msg == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }

    /* IKEv2 header has already been decoded. Must be present before
     * reaching here.
     */
    else if(dec_msg->ike2_hdr == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    next_payload = dec_msg->ike2_hdr->ike2_next_payload;
    buffer = buffer + IKE2_HDR_TOTAL_LEN;
    buff_len = buff_len - IKE2_HDR_TOTAL_LEN;

    while(next_payload != IKE2_NONE_PAYLOAD_ID)
    {
        status = IKE2_UNEXPECTED_PAYLOAD;
        switch(next_payload)
        {
        case IKE2_SA_PAYLOAD_ID:
            if(dec_msg->ike2_sa != NU_NULL)
            {
                status = IKE2_Decode_IKE_SA_Payload(buffer, buff_len,
                    dec_msg->ike2_sa, &next_payload);
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_sa;
            }
            break;

        case IKE2_KE_PAYLOAD_ID:
            if(dec_msg->ike2_ke != NU_NULL)
            {
                status = IKE2_Decode_IKE_KE_Payload(buffer, buff_len,
                    dec_msg->ike2_ke, &next_payload);
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_ke;
            }
            break;

        case IKE2_ID_I_PAYLOAD_ID:
            if(dec_msg->ike2_id_i != NU_NULL)
            {
                status = IKE2_Decode_IKE_ID_Payload(buffer, buff_len,
                    dec_msg->ike2_id_i, &next_payload);
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_id_i;
            }
            break;

        case IKE2_ID_R_PAYLOAD_ID:
            if(dec_msg->ike2_id_r != NU_NULL)
            {
                status = IKE2_Decode_IKE_ID_Payload(buffer, buff_len,
                    dec_msg->ike2_id_r, &next_payload);
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_id_r;
            }
            break;

        case IKE2_CERT_PAYLOAD_ID:
            if(dec_msg->ike2_cert != NU_NULL)
            {
                status = IKE2_Decode_IKE_Cert_Payload(buffer, buff_len,
                    dec_msg->ike2_cert, &next_payload);
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_cert;
            }
            break;

        case IKE2_CERTREQ_PAYLOAD_ID:
            if(dec_msg->ike2_cert_req != NU_NULL)
            {
                status = IKE2_Decode_IKE_CertReq_Payload(buffer, buff_len,
                    dec_msg->ike2_cert_req, &next_payload);
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_cert_req;
            }
            break;

        case IKE2_AUTH_PAYLOAD_ID:
            if(dec_msg->ike2_auth != NU_NULL)
            {
                status = IKE2_Decode_IKE_Auth_Payload(buffer, buff_len,
                    dec_msg->ike2_auth, &next_payload);
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_auth;
            }
            break;

        case IKE2_NONCE_PAYLOAD_ID:
            if(dec_msg->ike2_nonce != NU_NULL)
            {
                status = IKE2_Decode_IKE_Nonce_Payload(buffer, buff_len,
                    dec_msg->ike2_nonce, &next_payload);
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_nonce;
            }
            break;

        case IKE2_NOTIFY_PAYLOAD_ID:
            status = IKE2_Decode_IKE_Notify_Payload(buffer, buff_len,
                                    &dec_msg->ike2_notify, &next_payload);
            current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_notify;
            break;

        case IKE2_DELETE_PAYLOAD_ID:
                status = IKE2_Decode_IKE_Delete_Payload(buffer, buff_len,
                                        &dec_msg->ike2_del, &next_payload);
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_del;
            break;

        case IKE2_VID_PAYLOAD_ID:
            if(dec_msg->ike2_vid != NU_NULL)
            {
                status = IKE2_Decode_IKE_Vendor_ID_Payload(buffer, buff_len,
                    dec_msg->ike2_vid, &next_payload);
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_vid;
            }
            break;

        case IKE2_TS_I_PAYLOAD_ID:
            if(dec_msg->ike2_ts_i != NU_NULL)
            {
                status = IKE2_Decode_IKE_TS_Payload(buffer, buff_len,
                    dec_msg->ike2_ts_i, &next_payload);

                if(status == NU_SUCCESS)
                {
                    dec_msg->ike2_ts_i->ike2_gen.ike2_payload_type =
                            IKE2_TS_I_PAYLOAD_ID;
                }
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_ts_i;
            }
            break;

        case IKE2_TS_R_PAYLOAD_ID:
            if(dec_msg->ike2_ts_r != NU_NULL)
            {
                status = IKE2_Decode_IKE_TS_Payload(buffer, buff_len,
                    dec_msg->ike2_ts_r, &next_payload);

                if(status == NU_SUCCESS)
                {
                    dec_msg->ike2_ts_r->ike2_gen.ike2_payload_type =
                        IKE2_TS_R_PAYLOAD_ID;
                }
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_ts_r;
            }
            break;

        case IKE2_CONFIG_PAYLOAD_ID:
            if(dec_msg->ike2_cfg != NU_NULL)
            {
                status = IKE2_Decode_IKE_Config_Payload(buffer, buff_len,
                    dec_msg->ike2_cfg, &next_payload);
                current_payload = (IKE2_GEN_HDR*)dec_msg->ike2_cfg;
            }
            break;

        default:
            /* Decode the generic header for Unrecognized payload. */
            status = IKE2_Decode_IKE_Gen_Header(buffer, buff_len,
                                                &unreco_pay, &next_payload);
            current_payload = &unreco_pay;

            /* If critical flag not set skip to the next payload,
             * otherwise reject message.*/
            if((status == NU_SUCCESS) && (unreco_pay.ike2_critical == 1))
                status = IKE2_UNEXPECTED_PAYLOAD;
            break;
        }

        /* If any decode function failed then break the loop and
         * return error.
         */
        if(status != NU_SUCCESS)
            break;

        buffer = buffer + (current_payload->ike2_payload_length);
        buff_len = buff_len - (current_payload->ike2_payload_length);

    } /* while() */

    /* Check if required payload(s) is(are) missing. */

    return (status);
} /* IKE2_Decode_Message */

#endif
