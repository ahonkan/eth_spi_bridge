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
*       ike_pload.c
*
* COMPONENT
*
*       IKE - Payload
*
* DESCRIPTION
*
*       This file implements the IKE payload handling functions.
*
* DATA STRUCTURES
*
*       IKE_Payload_Encode_Funcs    Interface to all encode payload
*                                   functions.
*
* FUNCTIONS
*
*       IKE_Add_Attribute
*       IKE_Add_Variable_Attribute
*       IKE_Encode_Attribute_Value
*       IKE_Decode_Attribute_Value
*       IKE_Encode_Attribute
*       IKE_Decode_Attribute
*       IKE_Set_Header
*       IKE_Encode_Header
*       IKE_Decode_Header
*       IKE_Get_Message_Length
*       IKE_Extract_Raw_Payload
*       IKE_Encode_SA_Payload
*       IKE_Decode_SA_Payload
*       IKE_Encode_Proposal_Payload
*       IKE_Decode_Proposal_Payload
*       IKE_Encode_Transform_Payload
*       IKE_Decode_Transform_Payload
*       IKE_Encode_KeyXchg_Payload
*       IKE_Decode_KeyXchg_Payload
*       IKE_Encode_ID_Payload
*       IKE_Decode_ID_Payload
*       IKE_Encode_Cert_Payload
*       IKE_Decode_Cert_Payload
*       IKE_Encode_CertReq_Payload
*       IKE_Decode_CertReq_Payload
*       IKE_Encode_Hash_Payload
*       IKE_Decode_Hash_Payload
*       IKE_Encode_Signature_Payload
*       IKE_Decode_Signature_Payload
*       IKE_Encode_Nonce_Payload
*       IKE_Decode_Nonce_Payload
*       IKE_Encode_Notify_Payload
*       IKE_Decode_Notify_Payload
*       IKE_Encode_Delete_Payload
*       IKE_Decode_Delete_Payload
*       IKE_Encode_VID_Payload
*       IKE_Decode_VID_Payload
*       IKE_Encode_Message
*       IKE_Decode_Message
*       IKE_Check_Missing_Payloads
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_doi.h
*       ike_pload.h
*       ike_buf.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_doi.h"
#include "networking/ike_pload.h"
#include "networking/ike_buf.h"

/* Local function prototypes. */
STATIC STATUS IKE_Check_Missing_Payloads(IKE_DEC_MESSAGE *dec_msg);

/* NOTES:
 *
 *     - All encode functions perform minimum error checking as
 *       they are called on data used within IKE. The bounds
 *       checks are all within the conditional debug macro.
 *
 *     - The length of the payload in the generic header need
 *       not be filled out by the caller when encoding a payload.
 *       The encode functions calculate the payload length.
 *
 *     - The caller MUST initialize all payload decode structures
 *       using the macro IKE_INIT_DEC_PAYLOAD(pload), before
 *       calling any of the decode functions. This allows the
 *       decode functions to determine whether the payload has
 *       already been decoded, and helps detect malformed IKE
 *       messages containing duplicate payloads.
 *
 *     - All RESERVED fields are tested for zero, while decoding,
 *       and an error is returned if they contain a non-zero
 *       value.
 *
 *     - All decode functions leave the ike_next_payload field
 *       of the generic payload untouched. The caller is
 *       responsible for building the payload chain.
 */

/* Initialize the encode payload function pointers. These
 * are used as a generic interface in the message encoding
 * function.
 *
 * Proposal and Transform payload functions are not included
 * in this list because firstly, they are handled by the
 * SA payload processing functions. And secondly, the
 * arguments to these payload processing functions are not
 * standard so they can not be accessed using the generic
 * interface.
 */
static IKE_PAYLOAD_ENCODE IKE_Payload_Encode_Funcs[] = {
    /* NONE payload type. This does not require encoding. */
    NU_NULL,

    /* SA payload encode function. */
    (IKE_PAYLOAD_ENCODE)IKE_Encode_SA_Payload,

    /* Proposal payload encode function. */
    NU_NULL,

    /* Transform payload encode function. */
    NU_NULL,

    /* Key Exchange payload encode function. */
    (IKE_PAYLOAD_ENCODE)IKE_Encode_KeyXchg_Payload,

    /* Identification payload encode function. */
    (IKE_PAYLOAD_ENCODE)IKE_Encode_ID_Payload,

    /* Certificate payload encode function. */
    (IKE_PAYLOAD_ENCODE)IKE_Encode_Cert_Payload,

    /* Certificate Request payload encode function. */
    (IKE_PAYLOAD_ENCODE)IKE_Encode_CertReq_Payload,

    /* Hash payload encode function. */
    (IKE_PAYLOAD_ENCODE)IKE_Encode_Hash_Payload,

    /* Signature payload encode function. */
    (IKE_PAYLOAD_ENCODE)IKE_Encode_Signature_Payload,

    /* Nonce payload encode function. */
    (IKE_PAYLOAD_ENCODE)IKE_Encode_Nonce_Payload,

    /* Notification payload encode function. */
    (IKE_PAYLOAD_ENCODE)IKE_Encode_Notify_Payload,

    /* Delete payload encode function. */
    (IKE_PAYLOAD_ENCODE)IKE_Encode_Delete_Payload,

    /* Vendor ID payload encode function. */
    (IKE_PAYLOAD_ENCODE)IKE_Encode_VID_Payload
};

/*************************************************************************
*
* FUNCTION
*
*       IKE_Add_Attribute
*
* DESCRIPTION
*
*       This function appends a basic attribute to the list
*       of attributes passed as parameters.
*
* INPUTS
*
*       type                    Type of the attribute defined
*                               by the DOI.
*       *attribs                Array of attributes.
*       *attrib_num             Index of item in attribute array
*                               which is to be set. On return,
*                               the index is incremented by one.
*       value                   Value of the attribute.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_Add_Attribute(UINT16 type, IKE_DATA_ATTRIB *attribs,
                       UINT8 *attrib_num, UINT16 value)
{
    if ((INT)*attrib_num < IKE_MAX_SA_ATTRIBS)
    {
        /* Set attribute type and value. */
        attribs[(INT)*attrib_num].ike_attrib_type   = type;
        attribs[(INT)*attrib_num].ike_attrib_lenval = value;

        /* Increment attribute count. */
        *attrib_num = *attrib_num + 1;
    }
    else
    {
        /* It is highly unlikely that this would happen */
        NLOG_Error_Log("Attribute index out of range",
                       NERR_SEVERE, __FILE__, __LINE__);
    }
} /* IKE_Add_Attribute */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Add_Variable_Attribute
*
* DESCRIPTION
*
*       This function appends a variable length attribute to
*       the list of attributes passed as parameters.
*
* INPUTS
*
*       type                    Type of the attribute defined
*                               by the DOI.
*       *attribs                Array of attributes.
*       *attrib_num             Index of item in attribute array
*                               which is to be set. On return,
*                               the index is incremented by one.
*       value                   Value of the attribute.
*       *buffer                 Buffer in which to store the
*                               variable length attribute.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_Add_Variable_Attribute(UINT16 type, IKE_DATA_ATTRIB *attribs,
                                UINT8 *attrib_num, UINT32 value,
                                UINT8 *buffer)
{
    if ((INT)*attrib_num < IKE_MAX_SA_ATTRIBS)
    {
        /* Set attribute type. */
        attribs[(INT)*attrib_num].ike_attrib_type = type;

        /* Encode attribute value. */
        IKE_Encode_Attribute_Value(value, buffer, &attribs[(INT)*attrib_num]);

        /* Increment attribute count. */
        *attrib_num = *attrib_num + 1;
    }
    else
    {
        /* It is highly unlikely that this would happen */
        NLOG_Error_Log("Attribute index out of range",
                       NERR_SEVERE, __FILE__, __LINE__);
    }
} /* IKE_Add_Variable_Attribute */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_Attribute_Value
*
* DESCRIPTION
*
*       This function encodes a numeric value in an IKE Attribute.
*       The maximum variable length supported is 4 bytes (i.e.
*       size of UINT32). The buffer provided should be large
*       enough to store this value.
*
* INPUTS
*
*       value                   Numeric value to be encoded.
*       *buffer                 Buffer to which the value is
*                               to be encoded. The attribute
*                               value will point to this buffer.
*       *attrib                 Pointer to the attribute.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_Encode_Attribute_Value(UINT32 value, UINT8 *buffer,
                                IKE_DATA_ATTRIB *attrib)
{
    /* Point the attribute value member to the buffer. */
    attrib->ike_attrib_val = buffer;

    /* Encode the value in the attribute. */
    PUT32(attrib->ike_attrib_val, 0, value);

    /* Set attribute options. */
    attrib->ike_attrib_lenval = (IKE_ATTRIB_AF_TLV |
                                 IKE_MAX_ATTRIB_VAL_LEN);

} /* IKE_Encode_Attribute_Value */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_Attribute_Value
*
* DESCRIPTION
*
*       This function decodes a numeric IKE Attribute value.
*       The maximum variable length supported is 4 bytes (i.e.
*       size of UINT32).
*
* INPUTS
*
*       *value                  On return, this contain the decoded
*                               numeric value of attribute.
*       *attrib                 Pointer to the attribute.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_ATTRIB_TOO_LONG     Attribute length is too long.
*
*************************************************************************/
STATUS IKE_Decode_Attribute_Value(UINT32 *value,
                                  IKE_DATA_ATTRIB *attrib)
{
    STATUS          status = NU_SUCCESS;
    UINT8           buffer[IKE_MAX_ATTRIB_VAL_LEN];

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((value == NU_NULL) || (attrib== NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* If attribute in TLV form. */
    if((attrib->ike_attrib_type & IKE_ATTRIB_AF_MASK) ==
        IKE_ATTRIB_AF_TLV)
    {
        /* Make sure variable length is not too long. */
        if(attrib->ike_attrib_lenval > IKE_MAX_ATTRIB_VAL_LEN)
        {
            status = IKE_ATTRIB_TOO_LONG;
        }

        else
        {
            /* Initialize buffer to zero. */
            UTL_Zero(buffer, IKE_MAX_ATTRIB_VAL_LEN);

            /* Copy bytes of value to high order. */
            NU_BLOCK_COPY((buffer + IKE_MAX_ATTRIB_VAL_LEN -
                          attrib->ike_attrib_lenval),
                          attrib->ike_attrib_val,
                          attrib->ike_attrib_lenval);

            /* Decode the value. */
            *value = GET32(buffer, 0);
        }
    }

    else
    {
        /* The attribute is encoded as basic. */
        *value = (UINT32)attrib->ike_attrib_lenval;
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_Attribute_Value */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_Attribute
*
* DESCRIPTION
*
*       This function encodes an IKE Attribute.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *attrib                 Pointer to the source attribute.
*       *attrib_len             On return, this contains the
*                               raw attribute length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_Attribute(UINT8 *buffer, UINT16 buffer_len,
                            IKE_DATA_ATTRIB *attrib,
                            UINT16 *attrib_len)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (attrib == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* If attribute is in TLV format. */
    if((attrib->ike_attrib_type & IKE_ATTRIB_AF_MASK) == IKE_ATTRIB_AF_TLV)
    {
        /* Check if enough space in buffer. */
        if(IKE_MIN_ATTRIB_LEN + attrib->ike_attrib_lenval > buffer_len)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Encode attribute value. */
            NU_BLOCK_COPY(buffer + IKE_ATTRIB_VAL_OFFSET,
                          attrib->ike_attrib_val,
                          attrib->ike_attrib_lenval);

            /* Set attribute length. */
            (*attrib_len) = IKE_MIN_ATTRIB_LEN + attrib->ike_attrib_lenval;
        }
    }

    /* Check buffer size for TV format. */
    else
    {
        /* Check the buffer length. */
        if(buffer_len < IKE_MIN_ATTRIB_LEN)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Set attribute length. */
            (*attrib_len) = IKE_MIN_ATTRIB_LEN;
        }
    }

    /* If no error occurred above. */
    if(status == NU_SUCCESS)
    {
        /* Encode attribute type. */
        PUT16(buffer, IKE_ATTRIB_AFTYPE_OFFSET, attrib->ike_attrib_type);

        /* Encode attribute length/value. */
        PUT16(buffer, IKE_ATTRIB_LENVAL_OFFSET, attrib->ike_attrib_lenval);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_Attribute */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_Attribute
*
* DESCRIPTION
*
*       This function decodes an IKE Attribute.
*
* INPUTS
*
*       *buffer                 Source buffer from which to decode.
*       buffer_len              Length of the buffer in bytes.
*       *attrib                 Pointer to the destination attribute.
*       *attrib_len             On return, this contains the
*                               raw attribute length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE_Decode_Attribute(UINT8 *buffer, UINT16 buffer_len,
                            IKE_DATA_ATTRIB *attrib,
                            UINT16 *attrib_len)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (attrib== NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure there is enough data in the buffer. */
    if(buffer_len < IKE_MIN_ATTRIB_LEN)
    {
        /* Report error. */
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Decode attribute type. */
        attrib->ike_attrib_type = GET16(buffer, IKE_ATTRIB_AFTYPE_OFFSET);

        /* Decode attribute length/value. */
        attrib->ike_attrib_lenval = GET16(buffer,
                                          IKE_ATTRIB_LENVAL_OFFSET);

        /* Set attribute length. */
        *attrib_len = IKE_MIN_ATTRIB_LEN;

        /* If attribute in TLV form. */
        if((attrib->ike_attrib_type & IKE_ATTRIB_AF_MASK) ==
           IKE_ATTRIB_AF_TLV)
        {
            /* Decode attribute value. */
            attrib->ike_attrib_val = buffer + IKE_ATTRIB_VAL_OFFSET;

            /* Set attribute length. */
            (*attrib_len) = (*attrib_len) + (attrib->ike_attrib_lenval);

            /* Make sure there is enough data in the buffer. */
            if(buffer_len < IKE_MIN_ATTRIB_LEN + attrib->ike_attrib_lenval)
            {
                /* Set error status. */
                status = IKE_LENGTH_IS_SHORT;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_Attribute */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Set_Header
*
* DESCRIPTION
*
*       This function sets all fields of an ISAKMP header.
*
* INPUTS
*
*       *hdr                    Pointer to the header.
*       msg_id                  Phase 2 message ID.
*       *cookies                Pointer to the Initiator and
*                               Responder cookies.
*       xchg_mode               Mode of exchange.
*       flags                   Flags to be used in the header.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_Set_Header(IKE_ENC_HDR *hdr, UINT32 msg_id, UINT8 *cookies,
                    UINT8 xchg_mode, UINT8 flags)
{
    /* Set all fields of the ISAKMP header. */
    hdr->ike_msg_id        = msg_id;
    hdr->ike_icookie       = cookies;
    hdr->ike_rcookie       = cookies + IKE_COOKIE_LEN;
    hdr->ike_exchange_type = xchg_mode;
    hdr->ike_flags         = flags;

} /* IKE_Set_Header */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_Header
*
* DESCRIPTION
*
*       This function encodes an IKE payload header.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_Header(UINT8 *buffer, UINT16 buffer_len,
                         IKE_ENC_HDR *hdr)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (hdr == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(buffer_len < IKE_HDR_LEN)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode the cookies. */
        NU_BLOCK_COPY(buffer + IKE_HDR_ICOOKIE_OFFSET,
                    hdr->ike_icookie, IKE_COOKIE_LEN);
        NU_BLOCK_COPY(buffer + IKE_HDR_RCOOKIE_OFFSET,
                    hdr->ike_rcookie, IKE_COOKIE_LEN);

        /* Encode payload type. */
        PUT8(buffer, IKE_HDR_NEXT_PLOAD_OFFSET,
            IKE_NEXT_PAYLOAD_TYPE(hdr->ike_hdr));

        /* Encode the version number. */
        PUT8(buffer, IKE_HDR_VERSION_OFFSET,
            (IKE_MAJOR_VERSION << IKE_MAJOR_VERSION_SHL) |
            (IKE_MINOR_VERSION));

        /* Encode the exchange type. */
        PUT8(buffer, IKE_HDR_XCHG_TYPE_OFFSET, hdr->ike_exchange_type);

        /* Encode the flags. */
        PUT8(buffer, IKE_HDR_FLAGS_OFFSET, hdr->ike_flags);

        /* Encode the Message ID. */
        NU_BLOCK_COPY(buffer + IKE_HDR_MSG_ID_OFFSET,
                      &hdr->ike_msg_id,
                      sizeof(UINT32));

        /* Encode the total packet length. */
        PUT32(buffer, IKE_HDR_LENGTH_OFFSET, hdr->ike_length);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_Header */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_Header
*
* DESCRIPTION
*
*       This function decodes an IKE payload header.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE_Decode_Header(UINT8 *buffer, UINT16 buffer_len,
                         IKE_DEC_HDR *hdr)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (hdr == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Check if enough space in buffer. */
    if(buffer_len < IKE_HDR_LEN)
    {
        /* Set error status. */
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Initialize the generic header. The ISAKMP header does
         * not contain a generic payload header but it has been
         * included in the design for consistency with the chain
         * of payloads.
         */
        hdr->ike_hdr.ike_payload_len = IKE_HDR_LEN;
        hdr->ike_hdr.ike_type = IKE_NONE_PAYLOAD_ID;

        /* Decode the cookies. */
        hdr->ike_icookie = buffer + IKE_HDR_ICOOKIE_OFFSET;
        hdr->ike_rcookie = buffer + IKE_HDR_RCOOKIE_OFFSET;

        /* Decode payload type. */
        hdr->ike_first_payload = GET8(buffer, IKE_HDR_NEXT_PLOAD_OFFSET);

        /* Decode the version number. */
        hdr->ike_version = GET8(buffer, IKE_HDR_VERSION_OFFSET);

        /* Decode the exchange type. */
        hdr->ike_exchange_type = GET8(buffer, IKE_HDR_XCHG_TYPE_OFFSET);

        /* Decode the flags. */
        hdr->ike_flags = GET8(buffer, IKE_HDR_FLAGS_OFFSET);

        /* Decode the Message ID. */
        NU_BLOCK_COPY(&hdr->ike_msg_id,
                      buffer + IKE_HDR_MSG_ID_OFFSET,
                      sizeof(UINT32));

        /* Decode total packet length. */
        hdr->ike_length = GET32(buffer, IKE_HDR_LENGTH_OFFSET);
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_Header */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Get_Message_Length
*
* DESCRIPTION
*
*       This function returns the length of the message by
*       traversing all the payloads within the message. The
*       message length stored in the header may contain
*       padding but this function returns the actual length
*       without padding.
*
* INPUTS
*
*       *buffer                 Buffer containing raw message.
*       buffer_len              Length of the buffer in bytes.
*       *data_len               On return, this contains the
*                               length of the message data.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Get_Message_Length(UINT8 *buffer, UINT16 buffer_len,
                              UINT16 *data_len)
{
    STATUS          status = NU_SUCCESS;
    UINT16          offset = IKE_HDR_LEN;
    UINT16          pload_len;
    UINT8           next_pload;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (data_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Check if message buffer has enough data in it. */
    if(buffer_len < IKE_HDR_LEN)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Get the first payload type. */
        next_pload = GET8(buffer, IKE_HDR_NEXT_PLOAD_OFFSET);

        /* Loop until all payloads have been processed. */
        while(next_pload != IKE_NONE_PAYLOAD_ID)
        {
            /* Make sure current offset is within bounds. */
            if(offset >= buffer_len)
            {
                /* Offset out of bounds. */
                status = IKE_LENGTH_IS_SHORT;
                break;
            }

            /* Get the next payload type. */
            next_pload = GET8(buffer, offset +
                              IKE_GEN_HDR_NEXT_PLOAD_OFFSET);

            /* Get length of the current payload. */
            pload_len = GET16(buffer, offset +
                              IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);

            /* Add length to the offset. */
            offset = offset + pload_len;
        }

        /* If no error occurred. */
        if(status == NU_SUCCESS)
        {
            /* Return the length of the message data. */
            *data_len = offset;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Get_Message_Length */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Extract_Raw_Payload
*
* DESCRIPTION
*
*       This function extracts the first occurrence of the
*       specified payload's raw text from an ISAKMP message.
*       The message may contain a sequence of encoded payloads.
*       The raw encoding is stored in a dynamically allocated
*       buffer. The caller is responsible for freeing this
*       buffer.
*
*       Note that this is a utility function used internally
*       on packets saved in IKE Buffers. Therefore, no bounds
*       checking is performed on the input buffer.
*
* INPUTS
*
*       *buffer                 Buffer containing ISAKMP message.
*       **ret_raw               On return, this contains a pointer
*                               to a dynamically allocated buffer
*                               containing the payload's body.
*       *ret_raw_len            On return, this contains the length
*                               of the raw payload body.
*       search_pload            Payload type which is to be
*                               searched.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_NOT_FOUND           Specified payload not found.
*
*************************************************************************/
STATUS IKE_Extract_Raw_Payload(UINT8 *buffer, UINT8 **ret_raw,
                               UINT16 *ret_raw_len, UINT8 search_pload)
{
    STATUS          status = IKE_NOT_FOUND;
    UINT8           next_pload;
    UINT16          pload_len;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer      == NU_NULL) || (ret_raw == NU_NULL) ||
       (ret_raw_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Get first payload's type from the ISAKMP header. */
    next_pload = GET8(buffer, IKE_HDR_NEXT_PLOAD_OFFSET);

    /* Set initial length equal to length of ISAKMP header. */
    pload_len = IKE_HDR_LEN;

    /* Loop until last payload is processed. */
    while(next_pload != IKE_NONE_PAYLOAD_ID)
    {
        /* Move to next payload. */
        buffer = buffer + pload_len;

        /* If this is the payload being searched. */
        if(next_pload == search_pload)
        {
            /* Get length of the payload body. */
            pload_len = (UINT16)(GET16(buffer,
                                       IKE_GEN_HDR_PLOAD_LENGTH_OFFSET) -
                                       IKE_GEN_HDR_LEN);

            /* Allocate memory for the payload body. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)ret_raw,
                                        pload_len, NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* Normalize the pointer. */
                *ret_raw = TLS_Normalize_Ptr(*ret_raw);

                /* Copy payload's body to the allocated buffer. */
                NU_BLOCK_COPY(*ret_raw, buffer + IKE_GEN_HDR_LEN,
                              pload_len);

                /* Return the SA's length. */
                *ret_raw_len = pload_len;
            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for raw payload",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            break;
        }

        else
        {
            /* Decode the payload's type and length. */
            next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);
            pload_len  = GET16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Extract_Raw_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_SA_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE SA payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_SA_Payload(UINT8 *buffer, UINT16 buffer_len,
                             IKE_SA_ENC_PAYLOAD *pload)
{
    INT             i;
    UINT16          offset;
    UINT16          proposal_len;
    STATUS          status = IKE_INVALID_PAYLOAD;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(buffer_len < IKE_MIN_SA_PAYLOAD_LEN)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
             IKE_NEXT_PAYLOAD_TYPE(pload->ike_hdr));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET,
              pload->ike_hdr.ike_payload_len);

        /* Encode the DOI. */
        PUT32(buffer, IKE_SA_DOI_OFFSET, pload->ike_doi);

        /* Encode the DOI-specific situation. */
        NU_BLOCK_COPY(buffer + IKE_SA_SITUATION_OFFSET,
                      pload->ike_situation,
                      pload->ike_situation_len);

        /* Initialize offset to first transform. */
        offset = IKE_MIN_SA_PAYLOAD_LEN + pload->ike_situation_len;

        /* Initialize remaining buffer length. */
        buffer_len = buffer_len - offset;

        /* Loop for each proposal of the SA. */
        for(i = 0; i < pload->ike_num_proposals; i++)
        {
            /* Encode current proposal payload. */
            status = IKE_Encode_Proposal_Payload(buffer + offset,
                                buffer_len,
                                &pload->ike_proposals[i],
                                (UINT8)(i == (pload->ike_num_proposals-1)),
                                &proposal_len);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to encode proposal payload",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Stop processing transforms chain. */
                break;
            }

            /* Set offset and buffer length for next payload. */
            offset = offset + proposal_len;
            buffer_len = buffer_len - proposal_len;
        }

        /* Encode payload length in generic header. This is delayed
         * till the end of this function because the size of an
         * SA payload is not known until all its proposal payloads
         * are processed.
         */
        pload->ike_hdr.ike_payload_len = offset;
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET,
              pload->ike_hdr.ike_payload_len);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_SA_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_SA_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE SA payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*       IKE_UNSUPPORTED_SITU    Situation is not supported.
*       IKE_TOO_MANY_PROPOSALS  Number of proposals exceed the
*                               defined limit.
*       IKE_TOO_MANY_TRANSFORMS Number of transforms exceed the
*                               defined limit.
*
*************************************************************************/
STATUS IKE_Decode_SA_Payload(UINT8 *buffer, UINT16 buffer_len,
                             IKE_SA_DEC_PAYLOAD *pload, UINT8 *next_pload)
{
    INT             i;
    UINT16          offset;
    UINT16          proposal_len;
    UINT8           next_proposal = IKE_NONE_PAYLOAD_ID;
    STATUS          status = IKE_INVALID_PAYLOAD;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
       (next_pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);

        pload->ike_hdr.ike_payload_len = GET16(buffer,
            IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);

        pload->ike_hdr.ike_type = IKE_SA_PAYLOAD_ID;

        /* Check if enough data in buffer. */
        if(pload->ike_hdr.ike_payload_len > buffer_len)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode the DOI. */
            pload->ike_doi = GET32(buffer, IKE_SA_DOI_OFFSET);

            /* Decode the DOI-specific situation. */
            pload->ike_situation     = buffer + IKE_SA_SITUATION_OFFSET;
            pload->ike_situation_len = IKE_IPS_SITUATION_LEN;

            /* Make sure the DOI is correct. */
            if(pload->ike_doi != IKE_DOI_IPSEC)
            {
                /* Set error status. */
                status = IKE_UNSUPPORTED_DOI;
            }

            else if(GET32(pload->ike_situation, 0) != IKE_IPS_SIT_ID_ONLY)
            {
                /* Situation is not supported. */
                status = IKE_UNSUPPORTED_SITU;
            }

            else
            {
                /* Initialize offset to first transform. */
                offset = IKE_MIN_SA_PAYLOAD_LEN + pload->ike_situation_len;

                /* Initialize remaining payload length. */
                buffer_len = pload->ike_hdr.ike_payload_len - offset;

                /* Loop for each proposal. */
                for(i = 0; i < IKE_MAX_PROPOSALS; i++)
                {
                    /* Make sure payload length is within bounds. */
                    if(offset + IKE_MIN_PROPOSAL_PAYLOAD_LEN >
                       pload->ike_hdr.ike_payload_len)
                    {
                        /* Set error status and abort. */
                        status = IKE_INVALID_PAYLOAD;
                        break;
                    }

                    /* Decode current proposal payload. */
                    status = IKE_Decode_Proposal_Payload(buffer + offset,
                                 buffer_len, &pload->ike_proposals[i],
                                 &next_proposal, &proposal_len);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Unable to decode proposal payload",
                            NERR_RECOVERABLE, __FILE__, __LINE__);

                        /* Stop processing proposals chain. */
                        break;
                    }

                    /* Set offset and buffer length for next payload. */
                    offset = offset + proposal_len;
                    buffer_len = buffer_len - proposal_len;

                    /* Check if last proposal in chain. */
                    if(next_proposal == IKE_NONE_PAYLOAD_ID)
                    {
                        /* Stop processing proposals chain. */
                        break;
                    }

                    /* Make sure next payload is a proposal. */
                    else if(next_proposal != IKE_PROPOSAL_PAYLOAD_ID)
                    {
                        /* Set error status and abort. */
                        status = IKE_INVALID_PAYLOAD;
                        break;
                    }
                }

                /* If no error occurred then perform further checks. */
                if(status == NU_SUCCESS)
                {
                    /* Check if number of proposals are within limit. */
                    if(next_proposal != IKE_NONE_PAYLOAD_ID)
                    {
#if (IKE_DECODE_PARTIAL_SA == NU_TRUE)
                        /* If the local proposal limit has been reached. */
                        if(i == IKE_MAX_PROPOSALS)
                        {
                            NLOG_Error_Log(
                                "Too many proposals in incoming message.\
 Only considering the first IKE_MAX_PROPOSALS payloads.",
                                NERR_INFORMATIONAL, __FILE__, __LINE__);

                            /* Set number of proposals in chain. */
                            pload->ike_num_proposals = IKE_MAX_PROPOSALS;

                            /* Mark this SA as containing partial proposal
                             * payloads.
                             */
                            pload->ike_partial_proposals = NU_TRUE;
                        }

                        else
                        {
                            NLOG_Error_Log("Non-zero next proposal field",
                                NERR_RECOVERABLE, __FILE__, __LINE__);

                            /* Set error status. */
                            status = IKE_INVALID_PAYLOAD;
                        }
#else
                        NLOG_Error_Log("Too many proposals in SA",
                            NERR_RECOVERABLE, __FILE__, __LINE__);

                        /* Set error status. */
                        status = IKE_TOO_MANY_PROPOSALS;
#endif
                    }

                    /* Make sure payload length is correct. */
                    else if(pload->ike_hdr.ike_payload_len != offset)
                    {
                        /* Set error status. */
                        status = IKE_INVALID_PAYLOAD;
                    }

                    else
                    {
                        /* Set number of proposals in chain. */
                        pload->ike_num_proposals = (UINT8)i + 1;

#if (IKE_DECODE_PARTIAL_SA == NU_TRUE)
                        /* Set partial proposals flag to false. */
                        pload->ike_partial_proposals = NU_FALSE;
#endif
                    }
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_SA_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_Proposal_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE Proposal payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*       is_last                 Set this to NU_TRUE if this is the
*                               last payload in the proposal chain.
*                               Otherwise NU_FALSE. This is needed
*                               because the proposal payload does
*                               not contain the generic payload
*                               header.
*       *pload_len              On return, this contains the raw
*                               payload length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_Proposal_Payload(UINT8 *buffer, UINT16 buffer_len,
                                   IKE_PROPOSAL_ENC_PAYLOAD *pload,
                                   UINT8 is_last, UINT16 *pload_len)
{
    INT             i;
    UINT16          offset;
    UINT16          transform_len;
    STATUS          status = IKE_INVALID_PAYLOAD;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer    == NU_NULL) || (pload == NU_NULL) ||
       (pload_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(buffer_len < IKE_MIN_PROPOSAL_PAYLOAD_LEN)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
             (is_last ? (IKE_NONE_PAYLOAD_ID) :
                        (IKE_PROPOSAL_PAYLOAD_ID)));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);

        /* Encode Proposal number. */
        PUT8(buffer, IKE_PROPOSAL_NO_OFFSET, pload->ike_proposal_no);

        /* Encode Protocol ID. */
        PUT8(buffer, IKE_PROPOSAL_PROTO_ID_OFFSET, pload->ike_protocol_id);

        /* Encode SPI size. */
        PUT8(buffer, IKE_PROPOSAL_SPI_LEN_OFFSET, pload->ike_spi_len);

        /* Encode number of Transforms. */
        PUT8(buffer, IKE_PROPOSAL_TRANS_NUM_OFFSET,
             pload->ike_num_transforms);

        /* Encode the SPI. */
        NU_BLOCK_COPY(buffer + IKE_PROPOSAL_SPI_OFFSET, pload->ike_spi,
                      pload->ike_spi_len);

        /* Initialize offset to first transform. */
        offset = IKE_MIN_PROPOSAL_PAYLOAD_LEN + pload->ike_spi_len;

        /* Initialize remaining buffer length. */
        buffer_len = buffer_len - offset;

        /* Loop for each transform of the proposal. */
        for(i = 0; i < pload->ike_num_transforms; i++)
        {
            /* Encode current transform payload. */
            status = IKE_Encode_Transform_Payload(buffer + offset,
                         buffer_len, &pload->ike_transforms[i],
                         (UINT8)(i == (pload->ike_num_transforms-1)),
                         &transform_len);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to encode Transform payload",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Stop processing transforms chain. */
                break;
            }

            /* Set offset and buffer length for next payload. */
            offset = offset + transform_len;
            buffer_len = buffer_len - transform_len;
        }

        /* Encode payload length in generic header. This is delayed
         * till the end of this function because the size of a
         * proposal payload is not known until all its transform
         * payloads are processed.
         */
        *pload_len = offset;
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET, offset);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_Proposal_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_Proposal_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE Proposal payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*       *next_pload             On return, contains next payload type.
*       *pload_len              On return, this contains the raw
*                               payload length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_TOO_MANY_TRANSFORMS Number of transforms exceed the
*                               defined limit.
*
*************************************************************************/
STATUS IKE_Decode_Proposal_Payload(UINT8 *buffer, UINT16 buffer_len,
                                   IKE_PROPOSAL_DEC_PAYLOAD *pload,
                                   UINT8 *next_pload, UINT16 *pload_len)
{
    INT             i;
    UINT16          offset;
    UINT16          transform_len;
    UINT8           next_transform = IKE_NONE_PAYLOAD_ID;
    STATUS          status = IKE_INVALID_PAYLOAD;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload     == NU_NULL) ||
       (next_pload == NU_NULL) || (pload_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Decode generic header. */
    *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);
    *pload_len  = GET16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);

    /* Check if enough data in buffer. */
    if((*pload_len) > buffer_len)
    {
        /* Set error status. */
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Decode Proposal number. */
        pload->ike_proposal_no = GET8(buffer, IKE_PROPOSAL_NO_OFFSET);

        /* Decode Protocol ID. */
        pload->ike_protocol_id = GET8(buffer,
            IKE_PROPOSAL_PROTO_ID_OFFSET);

        /* Decode SPI size. */
        pload->ike_spi_len = GET8(buffer, IKE_PROPOSAL_SPI_LEN_OFFSET);

        /* Decode number of Transforms. */
        pload->ike_num_transforms = GET8(buffer,
            IKE_PROPOSAL_TRANS_NUM_OFFSET);

        /* Decode the SPI. */
        pload->ike_spi = buffer + IKE_PROPOSAL_SPI_OFFSET;

        /* Make sure transforms are within supported range. */
        if(pload->ike_num_transforms > IKE_MAX_TRANSFORMS)
        {
            NLOG_Error_Log("Too many transform in the proposal",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Set error status. */
            status = IKE_TOO_MANY_TRANSFORMS;
        }

        else
        {
            /* Initialize offset to first transform. */
            offset = IKE_MIN_PROPOSAL_PAYLOAD_LEN + pload->ike_spi_len;

            /* Initialize remaining payload length. */
            buffer_len = (*pload_len) - offset;

            /* Loop for each transform. */
            for(i = 0; i < pload->ike_num_transforms; i++)
            {
                /* Make sure payload length is within bounds. */
                if(offset + IKE_MIN_TRANSFORM_PAYLOAD_LEN >
                   (*pload_len))
                {
                    /* Set error status and abort. */
                    status = IKE_INVALID_PAYLOAD;
                    break;
                }

                /* Decode current transform payload. */
                status = IKE_Decode_Transform_Payload(buffer + offset,
                             buffer_len, &pload->ike_transforms[i],
                             &next_transform, &transform_len);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log(
                        "Failed to decode Transform payload",
                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    /* Stop processing transforms chain. */
                    break;
                }

                /* Check if last payload in chain. */
                else if(next_transform == IKE_NONE_PAYLOAD_ID)
                {
                    /* Check if chain terminates unexpectedly. */
                    if(i != pload->ike_num_transforms-1)
                    {
                        /* Stop processing transforms chain. */
                        status = IKE_INVALID_PAYLOAD;
                        break;
                    }
                }

                /* Make sure next payload is a proposal. */
                else if(next_transform != IKE_TRANSFORM_PAYLOAD_ID)
                {
                    /* Set error status and abort. */
                    status = IKE_INVALID_PAYLOAD;
                    break;
                }

                /* Set offset and buffer length for next payload. */
                offset = offset + transform_len;
                buffer_len = buffer_len - transform_len;
            }

            /* If no error occurred then perform further checks. */
            if(status == NU_SUCCESS)
            {
                /* Check if number of proposals are within limit. */
                if(next_transform != IKE_NONE_PAYLOAD_ID)
                {
                    /* Set error status. */
                    status = IKE_TOO_MANY_TRANSFORMS;
                }

                /* Make sure payload length is correct. */
                else if((*pload_len) != offset)
                {
                    /* Set error status. */
                    status = IKE_INVALID_PAYLOAD;
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_Proposal_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_Transform_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE Transform payload.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the source payload.
*       is_last                 Set this to NU_TRUE if this is the
*                               last payload in the transforms chain.
*                               Otherwise NU_FALSE. This is needed
*                               because the proposal payload does
*                               not contain the generic payload
*                               header.
*       *pload_len              On return, this contains the raw
*                               payload length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_Transform_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE_TRANSFORM_PAYLOAD *pload,
                                    UINT8 is_last, UINT16 *pload_len)
{
    UINT16          attrib_len;
    INT             i;
    UINT16          offset;
    STATUS          status = IKE_INVALID_PAYLOAD;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer    == NU_NULL) || (pload == NU_NULL) ||
       (pload_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Check if destination buffer has enough space. */
    if(buffer_len < IKE_MIN_TRANSFORM_PAYLOAD_LEN)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
             (is_last ? (IKE_NONE_PAYLOAD_ID) :
                        (IKE_TRANSFORM_PAYLOAD_ID)));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);

        /* Encode Transform number. */
        PUT8(buffer, IKE_TRANSFORM_NO_OFFSET, pload->ike_transform_no);

        /* Encode Transform ID. */
        PUT8(buffer, IKE_TRANSFORM_ID_OFFSET, pload->ike_transform_id);

        /* Set RESERVED2 field to zero. */
        PUT16(buffer, IKE_TRANSFORM_RESERVED2_OFFSET, IKE_RESERVED);

        /* Set offset specifying start of attributes. */
        offset = IKE_TRANSFORM_SA_ATTRIB_OFFSET;

        /* Loop for each attribute. */
        for(i = 0; i < pload->ike_num_attributes; i++)
        {
            /* Encode SA attributes. */
            status = IKE_Encode_Attribute(buffer + offset,
                                        buffer_len - offset,
                                        &pload->ike_sa_attributes[i],
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
        }

        /* Set the payload length in parameter. */
        *pload_len = offset;

        /* Encode length in generic header. */
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET, offset);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_Transform_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_Transform_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE Transform payload.
*
* INPUTS
*
*       *buffer                 Source buffer from which to encode.
*       buffer_len              Length of the buffer in bytes.
*       *pload                  Pointer to the destination payload.
*       *next_pload             On return, contains next payload type.
*       *pload_len              On return, this contains the raw
*                               payload length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*
*************************************************************************/
STATUS IKE_Decode_Transform_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE_TRANSFORM_PAYLOAD *pload,
                                    UINT8 *next_pload, UINT16 *pload_len)
{
    INT             i;
    UINT16          offset;
    UINT16          attrib_len;
    STATUS          status = IKE_INVALID_PAYLOAD;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload     == NU_NULL) ||
       (next_pload == NU_NULL) || (pload_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Decode generic header. */
    *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);
    *pload_len  = GET16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);

    /* Check if enough data in buffer. */
    if((*pload_len) > buffer_len)
    {
        /* Set error status. */
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Decode Transform number. */
        pload->ike_transform_no = GET8(buffer,
            IKE_TRANSFORM_NO_OFFSET);

        /* Decode Transform ID. */
        pload->ike_transform_id = GET8(buffer,
            IKE_TRANSFORM_ID_OFFSET);

        /* Initialize offset to start of attributes. */
        offset = IKE_TRANSFORM_SA_ATTRIB_OFFSET;

        /* Loop for each attribute. */
        for(i = 0; i < IKE_MAX_SA_ATTRIBS; i++)
        {
            /* Decode SA attributes. */
            status = IKE_Decode_Attribute(buffer + offset,
                                          buffer_len - offset,
                                          &pload->ike_sa_attributes[i],
                                          &attrib_len);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to decode attribute",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Exit the loop. */
                break;
            }

            /* Move to the next attribute offset. */
            offset = offset + attrib_len;

            /* Check if all attributes have been processed. */
            if(offset == (*pload_len))
            {
                /* Stop further processing. */
                break;
            }

            /* Check if payload size is invalid. */
            else if(offset > (*pload_len))
            {
                /* Report error and abort processing. */
                status = IKE_INVALID_PAYLOAD;
                break;
            }
        }

        /* If no error occurred. */
        if(status == NU_SUCCESS)
        {
            /* Set attribute count. */
            pload->ike_num_attributes = (UINT8)i+1;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_Transform_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_KeyXchg_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE Key Exchange payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_KeyXchg_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE_KEYXCHG_ENC_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Calculate payload length. */
    pload->ike_hdr.ike_payload_len =
        (IKE_MIN_KEYXCHG_PAYLOAD_LEN + pload->ike_keyxchg_data_len);

    /* Check if destination buffer has enough space. */
    if(pload->ike_hdr.ike_payload_len > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
             IKE_NEXT_PAYLOAD_TYPE(pload->ike_hdr));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET,
              pload->ike_hdr.ike_payload_len);

        /* Encode the key data. */
        NU_BLOCK_COPY(buffer + IKE_KEYXCHG_DATA_OFFSET,
                      pload->ike_keyxchg_data,
                      pload->ike_keyxchg_data_len);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_KeyXchg_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_KeyXchg_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE Key Exchange payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*
*************************************************************************/
STATUS IKE_Decode_KeyXchg_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE_KEYXCHG_DEC_PAYLOAD *pload,
                                  UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
       (next_pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);

        pload->ike_hdr.ike_payload_len = GET16(buffer,
            IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);

        pload->ike_hdr.ike_type = IKE_KEYXCHG_PAYLOAD_ID;

        /* Check if enough data in buffer. */
        if(pload->ike_hdr.ike_payload_len > buffer_len)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        /* Check if enough data in payload. */
        else if(pload->ike_hdr.ike_payload_len <=
                IKE_MIN_KEYXCHG_PAYLOAD_LEN)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Calculate length of key exchange data. */
            pload->ike_keyxchg_data_len = (pload->ike_hdr.ike_payload_len -
                                           IKE_MIN_KEYXCHG_PAYLOAD_LEN);

            /* Decode the key data. */
            pload->ike_keyxchg_data = buffer + IKE_KEYXCHG_DATA_OFFSET;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_KeyXchg_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_ID_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE Identification payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_ID_Payload(UINT8 *buffer, UINT16 buffer_len,
                             IKE_ID_ENC_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Calculate payload length. */
    pload->ike_hdr.ike_payload_len =
        (IKE_MIN_ID_PAYLOAD_LEN + pload->ike_id_data_len);

    /* Check if destination buffer has enough space. */
    if(pload->ike_hdr.ike_payload_len > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
             IKE_NEXT_PAYLOAD_TYPE(pload->ike_hdr));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET,
              pload->ike_hdr.ike_payload_len);

        /* Encode ID type. */
        PUT8(buffer, IKE_ID_TYPE_OFFSET, pload->ike_id_type);

        /* Encode Protocol ID. */
        PUT8(buffer, IKE_ID_PROTO_ID_OFFSET, pload->ike_protocol_id);

        /* Encode the port. */
        PUT16(buffer, IKE_ID_PORT_OFFSET, pload->ike_port);

        /* Encode ID data. */
        NU_BLOCK_COPY(buffer + IKE_ID_DATA_OFFSET,
                      pload->ike_id_data,
                      pload->ike_id_data_len);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_ID_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_ID_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE Identification payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*
*************************************************************************/
STATUS IKE_Decode_ID_Payload(UINT8 *buffer, UINT16 buffer_len,
                             IKE_ID_DEC_PAYLOAD *pload, UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
       (next_pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }
	
	else
	{
	    /* Decode generic header. */
	    *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);
	
	    pload->ike_hdr.ike_payload_len = GET16(buffer,
	        IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);
	
	    pload->ike_hdr.ike_type = IKE_ID_PAYLOAD_ID;
	
	    /* Check if enough data in buffer. */
	    if(pload->ike_hdr.ike_payload_len > buffer_len)
	    {
	        /* Set error status. */
	        status = IKE_LENGTH_IS_SHORT;
	    }
	
	    /* Check if enough data in payload. */
	    else if(pload->ike_hdr.ike_payload_len <= IKE_MIN_ID_PAYLOAD_LEN)
	    {
	        /* Set error status. */
	        status = IKE_LENGTH_IS_SHORT;
	    }
	
	    else
	    {
	        /* Decode ID type. */
	        pload->ike_id_type = GET8(buffer, IKE_ID_TYPE_OFFSET);
	
	        /* Decode the Protocol ID. */
	        pload->ike_protocol_id = GET8(buffer, IKE_ID_PROTO_ID_OFFSET);
	
	        /* Decode the port. */
	        pload->ike_port = GET16(buffer, IKE_ID_PORT_OFFSET);
	
	        /* Decode ID data. */
	        pload->ike_id_data = buffer + IKE_ID_DATA_OFFSET;
	        pload->ike_id_data_len =
	            (pload->ike_hdr.ike_payload_len - IKE_MIN_ID_PAYLOAD_LEN);
	    }
	}

    /* Return the status. */
    return (status);

} /* IKE_Decode_ID_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_Cert_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE Certificate payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_Cert_Payload(UINT8 *buffer, UINT16 buffer_len,
                               IKE_CERT_ENC_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Calculate payload length. */
    pload->ike_hdr.ike_payload_len =
        (IKE_MIN_CERT_PAYLOAD_LEN + pload->ike_cert_data_len);

    /* Check if destination buffer has enough space. */
    if(pload->ike_hdr.ike_payload_len > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }
    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
            IKE_NEXT_PAYLOAD_TYPE(pload->ike_hdr));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET,
            pload->ike_hdr.ike_payload_len);

        /* Encode Certificate encoding. */
        PUT8(buffer, IKE_CERT_ENCODING_OFFSET, pload->ike_cert_encoding);

        /* Encode Certificate data. */
        NU_BLOCK_COPY(buffer + IKE_CERT_DATA_OFFSET,
            pload->ike_cert_data,
            pload->ike_cert_data_len);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_Cert_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_Cert_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE Certificate payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*
*************************************************************************/
STATUS IKE_Decode_Cert_Payload(UINT8 *buffer, UINT16 buffer_len,
                               IKE_CERT_DEC_PAYLOAD *pload,
                               UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
        (next_pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }
    else
    {
        /* Decode generic header. */
        *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);
        pload->ike_hdr.ike_payload_len = GET16(buffer,
            IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);
        pload->ike_hdr.ike_type = IKE_CERT_PAYLOAD_ID;

        /* Check if enough data in buffer. */
        if(pload->ike_hdr.ike_payload_len > buffer_len)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }
        /* Check if enough data in payload. */
        else if(pload->ike_hdr.ike_payload_len <= IKE_MIN_CERT_PAYLOAD_LEN)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }
        else
        {
            /* Decode Certificate encoding. */
            pload->ike_cert_encoding = GET8(buffer,
                IKE_CERT_ENCODING_OFFSET);

            /* Decode Certificate data. */
            pload->ike_cert_data = buffer + IKE_CERT_DATA_OFFSET;
            pload->ike_cert_data_len = (pload->ike_hdr.ike_payload_len -
                IKE_MIN_CERT_PAYLOAD_LEN);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_Cert_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_CertReq_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE Certificate Request payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_CertReq_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE_CERTREQ_ENC_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Calculate payload length. */
    pload->ike_hdr.ike_payload_len =
        (IKE_MIN_CERTREQ_PAYLOAD_LEN + pload->ike_ca_dn_length);

    /* Check if destination buffer has enough space. */
    if(pload->ike_hdr.ike_payload_len > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }
    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
            IKE_NEXT_PAYLOAD_TYPE(pload->ike_hdr));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET,
            pload->ike_hdr.ike_payload_len);

        /* Encode Certificate type. */
        PUT8(buffer, IKE_CERTREQ_TYPE_OFFSET, pload->ike_cert_type);

        /* If CA's DN is specified in outgoing request, its length must
         * be non-zero. If its zero, its because no DN has been specified.
         */
        if(pload->ike_ca_dn_length > 0)
        {
            /* Encode Certificate Authority data. */
            NU_BLOCK_COPY(buffer + IKE_CERTREQ_AUTH_OFFSET,
                          pload->ike_ca_dn, pload->ike_ca_dn_length);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_CertReq_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_CertReq_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE Certificate Request payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*
*************************************************************************/
STATUS IKE_Decode_CertReq_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE_CERTREQ_DEC_PAYLOAD *pload,
                                  UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
        (next_pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);
        pload->ike_hdr.ike_payload_len = GET16(buffer,
            IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);
        pload->ike_hdr.ike_type = IKE_CERTREQ_PAYLOAD_ID;

        /* Check if enough data in buffer. */
        if(pload->ike_hdr.ike_payload_len > buffer_len)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }
        /* Check if enough data in payload. */
        else if(pload->ike_hdr.ike_payload_len <=
            IKE_MIN_CERTREQ_PAYLOAD_LEN)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }
        else
        {
            /* Decode Certificate type. */
            pload->ike_cert_type = GET8(buffer, IKE_CERTREQ_TYPE_OFFSET);

            /* Decode Certificate Authority data. */
            pload->ike_ca_dn = buffer + IKE_CERTREQ_AUTH_OFFSET;
            pload->ike_ca_dn_length = (pload->ike_hdr.ike_payload_len -
                IKE_MIN_CERTREQ_PAYLOAD_LEN);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_CertReq_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_Hash_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE Hash payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_Hash_Payload(UINT8 *buffer, UINT16 buffer_len,
                               IKE_HASH_ENC_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Calculate payload length. */
    pload->ike_hdr.ike_payload_len =
        (IKE_MIN_HASH_PAYLOAD_LEN + pload->ike_hash_data_len);

    /* Check if destination buffer has enough space. */
    if(pload->ike_hdr.ike_payload_len > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
             IKE_NEXT_PAYLOAD_TYPE(pload->ike_hdr));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET,
              pload->ike_hdr.ike_payload_len);

        /* Encode Hash data. */
        NU_BLOCK_COPY(buffer + IKE_HASH_DATA_OFFSET,
                      pload->ike_hash_data,
                      pload->ike_hash_data_len);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_Hash_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_Hash_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE Hash payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*
*************************************************************************/
STATUS IKE_Decode_Hash_Payload(UINT8 *buffer, UINT16 buffer_len,
                               IKE_HASH_DEC_PAYLOAD *pload,
                               UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
       (next_pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);

        pload->ike_hdr.ike_payload_len = GET16(buffer,
            IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);

        pload->ike_hdr.ike_type = IKE_HASH_PAYLOAD_ID;

        /* Check if enough data in buffer. */
        if(pload->ike_hdr.ike_payload_len > buffer_len)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        /* Check if enough data in payload. */
        else if(pload->ike_hdr.ike_payload_len <= IKE_MIN_HASH_PAYLOAD_LEN)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode Hash data. */
            pload->ike_hash_data     = buffer + IKE_HASH_DATA_OFFSET;
            pload->ike_hash_data_len = (UINT8)
                                       (pload->ike_hdr.ike_payload_len -
                                        IKE_MIN_HASH_PAYLOAD_LEN);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_Hash_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_Signature_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE Signature payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_Signature_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE_SIGNATURE_ENC_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Calculate payload length. */
    pload->ike_hdr.ike_payload_len =
        (IKE_MIN_SIGNATURE_PAYLOAD_LEN + pload->ike_signature_data_len);

    /* Check if destination buffer has enough space. */
    if(pload->ike_hdr.ike_payload_len > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
             IKE_NEXT_PAYLOAD_TYPE(pload->ike_hdr));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET,
              pload->ike_hdr.ike_payload_len);

        /* Encode Signature data. */
        NU_BLOCK_COPY(buffer + IKE_SIGNATURE_DATA_OFFSET,
                      pload->ike_signature_data,
                      pload->ike_signature_data_len);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_Signature_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_Signature_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE Signature payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*
*************************************************************************/
STATUS IKE_Decode_Signature_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE_SIGNATURE_DEC_PAYLOAD *pload,
                                    UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
       (next_pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);

        pload->ike_hdr.ike_payload_len = GET16(buffer,
            IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);

        pload->ike_hdr.ike_type = IKE_SIGNATURE_PAYLOAD_ID;

        /* Check if enough data in buffer. */
        if(pload->ike_hdr.ike_payload_len > buffer_len)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        /* Check if enough data in payload. */
        else if(pload->ike_hdr.ike_payload_len <=
                IKE_MIN_SIGNATURE_PAYLOAD_LEN)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode Signature data. */
            pload->ike_signature_data = buffer + IKE_SIGNATURE_DATA_OFFSET;
            pload->ike_signature_data_len =
                (pload->ike_hdr.ike_payload_len -
                 IKE_MIN_SIGNATURE_PAYLOAD_LEN);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_Signature_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_Nonce_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE Nonce payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_Nonce_Payload(UINT8 *buffer, UINT16 buffer_len,
                                IKE_NONCE_ENC_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Calculate payload length. */
    pload->ike_hdr.ike_payload_len =
        (IKE_MIN_NONCE_PAYLOAD_LEN + pload->ike_nonce_data_len);

    /* Check if destination buffer has enough space. */
    if(pload->ike_hdr.ike_payload_len > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
             IKE_NEXT_PAYLOAD_TYPE(pload->ike_hdr));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET,
              pload->ike_hdr.ike_payload_len);

        /* Encode Nonce data. */
        NU_BLOCK_COPY(buffer + IKE_NONCE_DATA_OFFSET,
                      pload->ike_nonce_data,
                      pload->ike_nonce_data_len);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_Nonce_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_Nonce_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE Nonce payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*
*************************************************************************/
STATUS IKE_Decode_Nonce_Payload(UINT8 *buffer, UINT16 buffer_len,
                                IKE_NONCE_DEC_PAYLOAD *pload,
                                UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
       (next_pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);

        pload->ike_hdr.ike_payload_len = GET16(buffer,
            IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);

        pload->ike_hdr.ike_type = IKE_NONCE_PAYLOAD_ID;

        /* Check if enough data in buffer. */
        if(pload->ike_hdr.ike_payload_len > buffer_len)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        /* Check if enough data in payload. */
        else if(pload->ike_hdr.ike_payload_len <=
                IKE_MIN_NONCE_PAYLOAD_LEN)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode Nonce data. */
            pload->ike_nonce_data     = buffer + IKE_NONCE_DATA_OFFSET;
            pload->ike_nonce_data_len =
                (pload->ike_hdr.ike_payload_len -
                 IKE_MIN_NONCE_PAYLOAD_LEN);

            /* Make sure Nonce data length is valid. */
            if((pload->ike_nonce_data_len < IKE_MIN_NONCE_DATA_LEN) ||
               (pload->ike_nonce_data_len > IKE_MAX_NONCE_DATA_LEN))
            {
                /* Set status to error. */
                status = IKE_INVALID_PAYLOAD;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_Nonce_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_Notify_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE Notification payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_Notify_Payload(UINT8 *buffer, UINT16 buffer_len,
                                 IKE_NOTIFY_ENC_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Calculate payload length. */
    pload->ike_hdr.ike_payload_len = (IKE_MIN_NOTIFY_PAYLOAD_LEN +
                                      pload->ike_spi_len         +
                                      pload->ike_notify_data_len);

    /* Check if destination buffer has enough space. */
    if(pload->ike_hdr.ike_payload_len > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
             IKE_NEXT_PAYLOAD_TYPE(pload->ike_hdr));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET,
              pload->ike_hdr.ike_payload_len);

        /* Encode the DOI. */
        PUT32(buffer, IKE_NOTIFY_DOI_OFFSET, pload->ike_doi);

        /* Encode Protocol ID. */
        PUT8(buffer, IKE_NOTIFY_PROTO_ID_OFFSET, pload->ike_protocol_id);

        /* Encode SPI size. */
        PUT8(buffer, IKE_NOTIFY_SPI_LEN_OFFSET, pload->ike_spi_len);

        /* Encode Notify Message Type. */
        PUT16(buffer, IKE_NOTIFY_MSG_TYPE_OFFSET, pload->ike_notify_type);

        /* If SPI is present. */
        if(pload->ike_spi_len != 0)
        {
            /* Encode the SPI. */
            NU_BLOCK_COPY(buffer + IKE_NOTIFY_SPI_OFFSET,
                          pload->ike_spi, pload->ike_spi_len);
        }

        /* If notification data is present. */
        if(pload->ike_notify_data_len != 0)
        {
            /* Encode Notification data. */
            NU_BLOCK_COPY(buffer + IKE_NOTIFY_SPI_OFFSET +
                          pload->ike_spi_len,
                          pload->ike_notify_data,
                          pload->ike_notify_data_len);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_Notify_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_Notify_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE Notification payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*
*************************************************************************/
STATUS IKE_Decode_Notify_Payload(UINT8 *buffer, UINT16 buffer_len,
                                 IKE_NOTIFY_DEC_PAYLOAD *pload,
                                 UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
       (next_pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);

        pload->ike_hdr.ike_payload_len = GET16(buffer,
            IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);

        pload->ike_hdr.ike_type = IKE_NOTIFY_PAYLOAD_ID;

        /* Check if enough data in buffer. */
        if(pload->ike_hdr.ike_payload_len > buffer_len)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode the DOI. */
            pload->ike_doi = GET32(buffer, IKE_NOTIFY_DOI_OFFSET);

            /* Make sure the DOI is correct. */
            if(pload->ike_doi != IKE_DOI_IPSEC)
            {
                /* Set error status. */
                status = IKE_UNSUPPORTED_DOI;
            }

            else
            {
                /* Decode Protocol ID. */
                pload->ike_protocol_id = GET8(buffer,
                    IKE_NOTIFY_PROTO_ID_OFFSET);

                /* Decode SPI size. */
                pload->ike_spi_len = GET8(buffer,
                    IKE_NOTIFY_SPI_LEN_OFFSET);

                /* Decode Notify Message Type. */
                pload->ike_notify_type = GET16(buffer,
                    IKE_NOTIFY_MSG_TYPE_OFFSET);

                /* Decode the SPI. */
                pload->ike_spi = buffer + IKE_NOTIFY_SPI_OFFSET;

                /* Decode Notification data. */
                pload->ike_notify_data = (buffer + IKE_NOTIFY_SPI_OFFSET +
                                          pload->ike_spi_len);
                pload->ike_notify_data_len =
                    (pload->ike_hdr.ike_payload_len -
                     pload->ike_spi_len             -
                     IKE_MIN_NOTIFY_PAYLOAD_LEN);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_Notify_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_Delete_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE Delete payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_Delete_Payload(UINT8 *buffer, UINT16 buffer_len,
                                 IKE_DELETE_ENC_PAYLOAD *pload)
{
    INT             i;
    INT             offset;
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Calculate payload length. */
    pload->ike_hdr.ike_payload_len = (IKE_MIN_DELETE_PAYLOAD_LEN +
        (pload->ike_spi_len * pload->ike_num_spi));

    /* Check if destination buffer has enough space. */
    if(pload->ike_hdr.ike_payload_len > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
             IKE_NEXT_PAYLOAD_TYPE(pload->ike_hdr));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET,
              pload->ike_hdr.ike_payload_len);

        /* Encode the DOI. */
        PUT32(buffer, IKE_DELETE_DOI_OFFSET, pload->ike_doi);

        /* Encode Protocol ID. */
        PUT8(buffer, IKE_DELETE_PROTO_ID_OFFSET, pload->ike_protocol_id);

        /* Encode SPI size. */
        PUT8(buffer, IKE_DELETE_SPI_LEN_OFFSET, pload->ike_spi_len);

        /* Encode number of SPIs. */
        PUT16(buffer, IKE_DELETE_NUM_SPI_OFFSET, pload->ike_num_spi);

        /* Initialize offset of first SPI. */
        offset = IKE_DELETE_SPI_OFFSET;

        /* Loop for each SPI. */
        for(i = 0; i < (INT)pload->ike_num_spi; i++)
        {
            /* Encode SPI data. */
            NU_BLOCK_COPY(buffer + offset, pload->ike_spis[i],
                          pload->ike_spi_len);

            /* Update offset to point to next SPI. */
            offset += pload->ike_spi_len;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_Delete_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_Delete_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE Delete payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*
*************************************************************************/
STATUS IKE_Decode_Delete_Payload(UINT8 *buffer, UINT16 buffer_len,
                                 IKE_DELETE_DEC_PAYLOAD *pload,
                                 UINT8 *next_pload)
{
    INT             i;
    INT             offset;
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
       (next_pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);

        pload->ike_hdr.ike_payload_len = GET16(buffer,
            IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);

        pload->ike_hdr.ike_type = IKE_DELETE_PAYLOAD_ID;

        /* Check if enough data in buffer. */
        if(pload->ike_hdr.ike_payload_len > buffer_len)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode the DOI. */
            pload->ike_doi = GET32(buffer, IKE_DELETE_DOI_OFFSET);

            /* Make sure the DOI is correct. */
            if(pload->ike_doi != IKE_DOI_IPSEC)
            {
                /* Set error status. */
                status = IKE_UNSUPPORTED_DOI;
            }

            else
            {
                /* Decode Protocol ID. */
                pload->ike_protocol_id = GET8(buffer,
                    IKE_DELETE_PROTO_ID_OFFSET);

                /* Decode SPI size. */
                pload->ike_spi_len =
                    GET8(buffer, IKE_DELETE_SPI_LEN_OFFSET);

                /* Decode number of SPIs. */
                pload->ike_num_spi =
                    GET16(buffer, IKE_DELETE_NUM_SPI_OFFSET);

                /* Make sure payload length is correct. */
                if((pload->ike_num_spi == 0) ||
                    (pload->ike_num_spi > IKE_MAX_DELETE_SPI) ||
                   (pload->ike_hdr.ike_payload_len !=
                    (IKE_MIN_DELETE_PAYLOAD_LEN +
                     (pload->ike_spi_len * pload->ike_num_spi))))
                {
                    if(pload->ike_num_spi > IKE_MAX_DELETE_SPI)
                    {
                        NLOG_Error_Log("Encountered more SPIs then can be handled",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                    /* Set error status. */
                    status = IKE_INVALID_PAYLOAD;
                }

                else
                {
                    /* Initialize offset of first SPI. */
                    offset = IKE_DELETE_SPI_OFFSET;

                    /* Loop for each SPI. */
                    for(i = 0; i < (INT)pload->ike_num_spi; i++)
                    {
                        /* Decode SPI data. */
                        pload->ike_spis[i] = buffer + offset;

                        /* Update offset to point to next SPI. */
                        offset += pload->ike_spi_len;
                    }
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_Delete_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_VID_Payload
*
* DESCRIPTION
*
*       This function encodes an IKE Vendor ID payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_VID_Payload(UINT8 *buffer, UINT16 buffer_len,
                              IKE_VID_ENC_PAYLOAD *pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Calculate payload length. */
    pload->ike_hdr.ike_payload_len =
        (IKE_MIN_VID_PAYLOAD_LEN + pload->ike_vid_data_len);

    /* Check if destination buffer has enough space. */
    if(pload->ike_hdr.ike_payload_len > buffer_len)
    {
        status = IKE_LENGTH_IS_SHORT;
    }

    else
    {
        /* Encode generic header. */
        PUT8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET,
             IKE_NEXT_PAYLOAD_TYPE(pload->ike_hdr));
        PUT8(buffer, IKE_GEN_HDR_RESERVED_OFFSET, IKE_RESERVED);
        PUT16(buffer, IKE_GEN_HDR_PLOAD_LENGTH_OFFSET,
              pload->ike_hdr.ike_payload_len);

        /* Encode Vendor ID data. */
        NU_BLOCK_COPY(buffer + IKE_VID_DATA_OFFSET,
                      pload->ike_vid_data,
                      pload->ike_vid_data_len);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_VID_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_VID_Payload
*
* DESCRIPTION
*
*       This function decodes an IKE Vendor ID payload.
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
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*
*************************************************************************/
STATUS IKE_Decode_VID_Payload(UINT8 *buffer, UINT16 buffer_len,
                              IKE_VID_DEC_PAYLOAD *pload,
                              UINT8 *next_pload)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer     == NU_NULL) || (pload == NU_NULL) ||
       (next_pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure the payload has not been decoded already. */
    if(IKE_PAYLOAD_IS_PRESENT(pload) == NU_TRUE)
    {
        /* Report error if payload decoded already. */
        status = IKE_DUPLICATE_PAYLOAD;
    }

    else
    {
        /* Decode generic header. */
        *next_pload = GET8(buffer, IKE_GEN_HDR_NEXT_PLOAD_OFFSET);

        pload->ike_hdr.ike_payload_len = GET16(buffer,
            IKE_GEN_HDR_PLOAD_LENGTH_OFFSET);

        pload->ike_hdr.ike_type = IKE_VID_PAYLOAD_ID;

        /* Check if enough data in buffer. */
        if(pload->ike_hdr.ike_payload_len > buffer_len)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        /* Check if enough data in payload. */
        else if(pload->ike_hdr.ike_payload_len <= IKE_MIN_VID_PAYLOAD_LEN)
        {
            /* Set error status. */
            status = IKE_LENGTH_IS_SHORT;
        }

        else
        {
            /* Decode Vendor ID data. */
            pload->ike_vid_data     = buffer + IKE_VID_DATA_OFFSET;
            pload->ike_vid_data_len = (pload->ike_hdr.ike_payload_len -
                                       IKE_MIN_VID_PAYLOAD_LEN);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_VID_Payload */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encode_Message
*
* DESCRIPTION
*
*       This function encodes an IKE message. Only the header
*       is passed as a parameter, containing a chain of
*       all the payloads within the message. The length of
*       the message need not be specified in the ISAKMP
*       header as it is calculated by this function.
*
* INPUTS
*
*       *buffer                 Destination buffer to which to encode.
*                               Size of this buffer must be at least
*                               IKE_MAX_BUFFER_LEN, in bytes.
*       *hdr                    Pointer to the ISAKMP header.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     A payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*
*************************************************************************/
STATUS IKE_Encode_Message(UINT8 *buffer, IKE_ENC_HDR *hdr)
{
    STATUS          status = IKE_INVALID_PAYLOAD;
    IKE_GEN_HDR     *cur_pload;
    UINT8 HUGE      *pkt;
    UINT16          pkt_len;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (hdr == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Initialize the packet buffer pointer and size. */
    pkt = buffer + IKE_HDR_LEN;
    pkt_len = IKE_MAX_BUFFER_LEN - IKE_HDR_LEN;

    /* Set the header as the current payload (abstraction). */
    cur_pload = (IKE_GEN_HDR*)hdr->ike_hdr.ike_next_payload;

    /* Loop for each payload in the message. In some cases,
     * multiple payloads are processed in a single iteration.
     * This is the the case when one payload is a part of
     * another one - E.g. Proposal and Transform payloads
     * within the SA payload.
     */
    while(cur_pload != NU_NULL)
    {
        /* Encode the payload. */
        status = IKE_Payload_Encode_Funcs[cur_pload->ike_type]((UINT8*)pkt,
                                                               pkt_len,
                                                               cur_pload);

        if(status != NU_SUCCESS)
        {
            /* Abort encoding operation. */
            break;
        }

        /* Update buffer positions. */
        pkt = pkt + (cur_pload->ike_payload_len);
        pkt_len = pkt_len - (cur_pload->ike_payload_len);

        /* Move to next payload. */
        cur_pload = cur_pload->ike_next_payload;
    }

    /* Make sure no errors occurred. */
    if(status == NU_SUCCESS)
    {
        /* Set length of the packet in the ISAKMP header. */
        hdr->ike_length = IKE_MAX_BUFFER_LEN - pkt_len;

        /* Encode the ISAKMP header. This is delayed till the
         * end because of the calculation of the message size,
         * needed in the length field of the header.
         */
        status = IKE_Encode_Header(buffer, IKE_MAX_BUFFER_LEN, hdr);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encode_Message */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Decode_Message
*
* DESCRIPTION
*
*       This function decodes an IKE message. A buffer
*       containing the raw packet is passed to this function.
*       The ISAKMP header of the message must already be decoded
*       and specified in 'dec_msg' before this function is
*       called. Payloads in this buffer are decoded and stored
*       in the decoded message structure. This message structure
*       must specify pointers to only those payloads which are
*       expected in the incoming message. All other payload
*       pointers must be set to NULL. Those payloads which are
*       specified can also be marked as optional. All required
*       (non-optional) payloads are checked for presence. An
*       error is returned if an unspecified payload is present
*       in the buffer or a required payload is missing from the
*       buffer.
*
* INPUTS
*
*       *buffer                 Destination buffer from which to
*                               decode. Size of this buffer must
*                               be at least IKE_MAX_RECV_BUFFER_LEN,
*                               in bytes.
*       *dec_msg                On return, this contains the
*                               decoded headers and payloads.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     A payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNEXPECTED_PAYLOAD  An unexpected payload encountered.
*       IKE_MISSING_PAYLOAD     Required payload is missing.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*       IKE_UNSUPPORTED_SITU    Situation is not supported.
*       IKE_TOO_MANY_PROPOSALS  Number of proposals exceed the
*                               defined limit.
*       IKE_TOO_MANY_TRANSFORMS Number of transforms exceed the
*                               defined limit.
*
*************************************************************************/
STATUS IKE_Decode_Message(UINT8 *buffer, IKE_DEC_MESSAGE *dec_msg)
{
    STATUS              status = NU_SUCCESS;
    UINT16              buffer_len;
    UINT8               next_pload;
    IKE_GEN_HDR         *cur_pload;
    IKE_VID_DEC_PAYLOAD dec_vid;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((buffer == NU_NULL) || (dec_msg == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* The ISAKMP header must be present. */
    else if(dec_msg->ike_hdr == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set first payload type from the ISAKMP header. */
    next_pload = dec_msg->ike_hdr->ike_first_payload;
    cur_pload  = (IKE_GEN_HDR*)dec_msg->ike_hdr;

    /* Initialize the buffer length. */
    buffer_len = IKE_MAX_RECV_BUFFER_LEN;

    /* Loop for each payload in the message. */
    while((next_pload != IKE_NONE_PAYLOAD_ID) &&
          (status == NU_SUCCESS))
    {
        /* Initialize status to error. */
        status = IKE_UNEXPECTED_PAYLOAD;

        /* Update next payload offset. */
        buffer     = buffer     + (cur_pload->ike_payload_len);
        buffer_len = buffer_len - (cur_pload->ike_payload_len);

        /* Determine next payload type. */
        switch(next_pload)
        {
        case IKE_SA_PAYLOAD_ID:
            /* If the payload has been specified. */
            if(dec_msg->ike_sa != NU_NULL)
            {
                /* Decode the SA payload. */
                status = IKE_Decode_SA_Payload(buffer, buffer_len,
                                               dec_msg->ike_sa,
                                               &next_pload);

                /* Update the current payload pointer. */
                cur_pload = (IKE_GEN_HDR*)dec_msg->ike_sa;
            }
            break;

        case IKE_KEYXCHG_PAYLOAD_ID:
            /* If the payload has been specified. */
            if(dec_msg->ike_key != NU_NULL)
            {
                /* Decode the Key Exchange payload. */
                status = IKE_Decode_KeyXchg_Payload(buffer, buffer_len,
                                                    dec_msg->ike_key,
                                                    &next_pload);

                /* Update the current payload pointer. */
                cur_pload = (IKE_GEN_HDR*)dec_msg->ike_key;
            }
            break;

        case IKE_NONCE_PAYLOAD_ID:
            /* If the payload has been specified. */
            if(dec_msg->ike_nonce != NU_NULL)
            {
                /* Decode the Nonce payload. */
                status = IKE_Decode_Nonce_Payload(buffer, buffer_len,
                                                  dec_msg->ike_nonce,
                                                  &next_pload);

                /* Update the current payload pointer. */
                cur_pload = (IKE_GEN_HDR*)dec_msg->ike_nonce;
            }
            break;

        case IKE_ID_PAYLOAD_ID:
            /* If the payload has been specified. */
            if(dec_msg->ike_id_i != NU_NULL)
            {
                /* If ID payload has not yet been received. */
                if(IKE_PAYLOAD_IS_PRESENT(dec_msg->ike_id_i) == NU_FALSE)
                {
                    /* Decode the Identification payload. */
                    status = IKE_Decode_ID_Payload(buffer, buffer_len,
                                                   dec_msg->ike_id_i,
                                                   &next_pload);

                    /* Update the current payload pointer. */
                    cur_pload = (IKE_GEN_HDR*)dec_msg->ike_id_i;
                }

                /* Otherwise if two ID payloads are expected. */
                else if(dec_msg->ike_id_r != NU_NULL)
                {
                    /* Decode the second Identification payload. */
                    status = IKE_Decode_ID_Payload(buffer, buffer_len,
                                                   dec_msg->ike_id_r,
                                                   &next_pload);

                    /* Update the current payload pointer. */
                    cur_pload = (IKE_GEN_HDR*)dec_msg->ike_id_r;
                }
            }
            break;

        case IKE_CERT_PAYLOAD_ID:
            /* If Certificate payload is specified. */
            if(dec_msg->ike_cert != NU_NULL)
            {
                if(IKE_PAYLOAD_IS_PRESENT(dec_msg->ike_cert) == NU_FALSE)
                {
                    /* Decode Cert payload. */
                    status = IKE_Decode_Cert_Payload(buffer, buffer_len,
                                        dec_msg->ike_cert, &next_pload);

                    /* Update current payload pointer. */
                    /*cur_pload = (IKE_GEN_HDR*)dec_msg->ike_cert;*/
                }
                /* Although multiple certificate payloads may be present,
                 * but this is highly undesirable because UDP
                 * fragmentation occurs with a much smaller packet usually
                 * containing only one certificate.
                 */
            }

            else
            {
                NLOG_Error_Log("CERT payload encountered but not expected",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Update current payload pointer. */
            cur_pload = (IKE_GEN_HDR*)dec_msg->ike_cert;
            break;

        case IKE_CERTREQ_PAYLOAD_ID:
            /* If Certificate Request payload is specified. */
            if(dec_msg->ike_certreq != NU_NULL)
            {
                /* If Cert-Req payload has not yet been received. */
                if(IKE_PAYLOAD_IS_PRESENT(dec_msg->ike_certreq) == NU_FALSE)
                {
                    /* Decode Cert-Req payload. */
                    status = IKE_Decode_CertReq_Payload(buffer, buffer_len,
                                        dec_msg->ike_certreq, &next_pload);
                    /* Update current payload pointer. */
                    cur_pload = (IKE_GEN_HDR*)dec_msg->ike_certreq;
                }
                /* Multiple certreq payloads are also possible but we
                 * process only one. This is because we support limited
                 * types. If we receive an unrecognized type in this
                 * payload, we can assume it to be X509_SIGNATURE.
                 */
            }

            else
            {
                NLOG_Error_Log("CERT-REQ payload encountered but not expected",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
            break;

        case IKE_HASH_PAYLOAD_ID:
            /* If the Signature payload is expected. */
            if(dec_msg->ike_sig != NU_NULL)
            {
                /* Make sure the Signature payload has not already
                 * been processed. Only either a Hash or a Signature
                 * payload could be present in a single message.
                 */
                if(IKE_PAYLOAD_IS_PRESENT(dec_msg->ike_sig) == NU_TRUE)
                {
                    NLOG_Error_Log("HASH/SIG data sent more than once",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                    break;
                }
            }

            /* If the Hash payload has been specified. */
            if(dec_msg->ike_hash != NU_NULL)
            {
                /* Decode the Hash payload. */
                status = IKE_Decode_Hash_Payload(buffer, buffer_len,
                                                 dec_msg->ike_hash,
                                                 &next_pload);

                /* Update the current payload pointer. */
                cur_pload = (IKE_GEN_HDR*)dec_msg->ike_hash;
            }
            break;

        case IKE_SIGNATURE_PAYLOAD_ID:
            /* If the Hash payload is expected. */
            if(dec_msg->ike_hash != NU_NULL)
            {
                /* Make sure the Hash payload has not already
                 * been processed. Only either a Hash or a Signature
                 * payload could be present in a single message.
                 */
                if(IKE_PAYLOAD_IS_PRESENT(dec_msg->ike_hash) == NU_TRUE)
                {
                    NLOG_Error_Log("HASH/SIG data sent more than once",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                    break;
                }
            }

            /* If the Signature payload has been specified. */
            if(dec_msg->ike_sig != NU_NULL)
            {
                /* Decode the Signature payload. */
                status = IKE_Decode_Signature_Payload(buffer, buffer_len,
                                                      dec_msg->ike_sig,
                                                      &next_pload);

                /* Update the current payload pointer. */
                cur_pload = (IKE_GEN_HDR*)dec_msg->ike_sig;
            }
            break;

        case IKE_NOTIFY_PAYLOAD_ID:
            /* If the payload has been specified. */
            if(dec_msg->ike_notify != NU_NULL)
            {
                /* Decode the Notification payload. */
                status = IKE_Decode_Notify_Payload(buffer, buffer_len,
                                                   dec_msg->ike_notify,
                                                   &next_pload);

                /* Update the current payload pointer. */
                cur_pload = (IKE_GEN_HDR*)dec_msg->ike_notify;
            }
            break;

        case IKE_DELETE_PAYLOAD_ID:
            /* If the payload has been specified. */
            if(dec_msg->ike_del != NU_NULL)
            {
                /* Decode the Notification payload. */
                status = IKE_Decode_Delete_Payload(buffer, buffer_len,
                                                   dec_msg->ike_del,
                                                   &next_pload);

                /* Update the current payload pointer. */
                cur_pload = (IKE_GEN_HDR*)dec_msg->ike_del;
            }
            break;

        case IKE_VID_PAYLOAD_ID:
            /* Mark Vendor ID payload as optional. */
            IKE_SET_OPTIONAL(&dec_vid);

            /* Decode the Vendor ID payload locally and discard
             * it since the IKE implementation of Nucleus IPsec
             * does not currently recognize Vendor IDs.
             */
            status = IKE_Decode_VID_Payload(buffer, buffer_len,
                                            &dec_vid, &next_pload);

            /* Update the current payload pointer. */
            cur_pload = (IKE_GEN_HDR*)&dec_vid;
            break;

        default:
            /* Unexpected payload in the message. */
            break;
        }
    }

    /* If no errors occurred */
    if(status == NU_SUCCESS)
    {
        /* Make sure all required payloads have been received. */
        status = IKE_Check_Missing_Payloads(dec_msg);
    }

    /* Return the status. */
    return (status);

} /* IKE_Decode_Message */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Check_Missing_Payloads
*
* DESCRIPTION
*
*       This function checks the decoded payloads to see if
*       any of the required payloads are missing. Payloads
*       marked as optional are ignored.
*
* INPUTS
*
*       *dec_msg                Decoded payloads to be checked.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_MISSING_PAYLOAD     Required payload is missing.
*
*************************************************************************/
STATIC STATUS IKE_Check_Missing_Payloads(IKE_DEC_MESSAGE *dec_msg)
{
    STATUS          status = NU_SUCCESS;
    IKE_GEN_HDR     **gen_hdr_ptr;
    INT             i;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(dec_msg == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set pointer to the first payload in list of payloads. */
    gen_hdr_ptr = (IKE_GEN_HDR**)dec_msg;

    /* Starting from the first payload (index zero is the ISAKMP
     * header. Payloads start from index one onwards), loop for
     * all payload pointers in the IKE_DEC_MESSAGE structure.
     */
    for(i = 1;
        i < (INT)(sizeof(IKE_DEC_MESSAGE)/sizeof(IKE_GEN_HDR*));
        i++)
    {
        /* If the current payload pointer is not NULL. */
        if(gen_hdr_ptr[i] != NU_NULL)
        {
            /* If the payload is still marked as required. */
            if(IKE_PAYLOAD_IS_REQUIRED(gen_hdr_ptr[i]) == NU_TRUE)
            {
                NLOG_Error_Log("A required payload is missing",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                status = IKE_MISSING_PAYLOAD;
                break;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Check_Missing_Payloads */
