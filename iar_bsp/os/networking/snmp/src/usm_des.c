/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
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
*       usm_des.c                                                
*
*   DESCRIPTION
*
*       This file contains definitions of all the functions required by
*       the CBC-DES Symmetric Encryption Protocol.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       USM_Des_Encrypt
*       USM_Des_Decrypt
*
*   DEPENDENCIES
*
*       snmp_cfg.h
*       usm_des.h
*       snmp.h
*       asn1.h
*       ncs_api.h
*
************************************************************************/

#include "networking/snmp_cfg.h"
#include "networking/usm_des.h"
#include "networking/snmp.h"
#include "networking/asn1.h"

#if (INCLUDE_SNMPv3 == NU_TRUE)
#include "openssl/des.h"
#endif

#if (INCLUDE_SNMPv3 == NU_TRUE)
STATIC UINT32                  Usm_Des_Salt_Integer = 1;
extern SNMP_ENGINE_STRUCT      Snmp_Engine;
extern snmp_stat_t             SnmpStat;

/************************************************************************
*
*   FUNCTION
*
*       USM_Des_Encrypt
*
*   DESCRIPTION
*
*       This function secures the outgoing message
*
*   INPUTS
*
*       *key                    Users encryption key
*       key_length              Encryption key length
*       **msg                   Message to be encrypted
*       *msg_length             Message length
*       *param                  Privacy parameters
*       *param_length           Privacy parameters length
*
*   OUTPUTS
*
*       NU_SUCCESS.
*       SNMP_ERROR
*
*************************************************************************/
STATUS USM_Des_Encrypt(UINT8 *key, UINT8 key_length, UINT8 **msg,
                       UINT32 *msg_length, UINT8 *param,
                       UINT32 *param_length, UINT16 buff_len)
{
    UINT8              iv[8];
    UINT8              i;
    asn1_sck_t         asn1;
    DES_key_schedule   schedule;
    INT                quotient;

    /* Check the key length. */
    if(key_length < 16)
        return (SNMP_ERROR);

    /* Create the salt. */
    PUT32(param, 0, Snmp_Engine.snmp_engine_boots);
    PUT32(param, 4, Usm_Des_Salt_Integer);
    (*param_length) = 8;

    /* Create the iv. */
    for(i = 0; i < 8; i++)
        iv[i] = (UINT8)(param[i] ^ key[i + 8]);

    Usm_Des_Salt_Integer++;

    /* Encrypt the scopedPDU. */

    /* Setup the key-schedule */
    if (0 != DES_set_key((const_DES_cblock*)key, &schedule))
    {
        return (SNMP_ERROR);
    }

    /* Make the request for encryption. */
    DES_ncbc_encrypt(*msg, *msg, *msg_length, &schedule, (DES_cblock*)iv, DES_ENCRYPT);
    
    asn1.Begin = (*msg) - (SNMP_BUFSIZE + (*msg_length));

    /* Update the message length. */
    /* Check if input was a multiple of 8, as output is always so. */
    quotient = *msg_length >> 3;
    if (*msg_length > (quotient << 3))
    {
        *msg_length = (quotient << 3) + 8;
    }
    
    if (*msg_length > buff_len)
    {
        return (SNMP_ERROR);
    }

    /* Encode the encrypted scopedPDU as an octet string. */    
    asn1.End = *msg + (*msg_length);
    asn1.Pointer = *msg;

    if (!Asn1HdrEnc(&asn1, *msg + (*msg_length), ASN1_UNI, ASN1_PRI,
                    ASN1_OTS))
    {
        return (SNMP_ERROR);
    }

    /* Update the message buffer. */
    else
    {
        Asn1Cls (&asn1, msg, msg_length);
    }

    /* Return success code. */
    return (NU_SUCCESS);

} /* USM_Des_Encrypt */

/************************************************************************
*
*   FUNCTION
*
*       USM_Des_Decrypt
*
*   DESCRIPTION
*
*       This function verifies the incoming message.
*
*   INPUTS
*
*       *key            User's privacy key
*       key_length      Privacy key length
*       *param          Privacy parameters
*       param_length    Parameter length
*       **msg           Encrypted Message
*       *msg_length     Encrypted Message length
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS USM_Des_Decrypt(UINT8 *key, UINT8 key_length, UINT8 *param,
                       UINT32 param_length, UINT8 **msg, UINT32 *msg_length)
{
    UINT8              iv[8];
    asn1_sck_t         asn1;
    UINT8              *End;
    UINT32             Cls, Con, Tag;
    UINT8              i;
    DES_key_schedule   schedule;

    /* Check the parameter's length. */
    if(param_length != 8)
        return (SNMP_ERROR);

    /* Check the key length. */
    if(key_length < 16)
        return (SNMP_ERROR);

    if (*msg_length == 0) 
    {
        SnmpStat.InASNParseErrs++;
        return (ASN1_ERR_DEC_BADVALUE);
    }

    /* Decode the octet string. */
    Asn1Opn(&asn1, *msg, *msg_length, ASN1_DEC);

    if (!Asn1HdrDec (&asn1, &End, &Cls, &Con, &Tag))
        return (SNMP_ERROR);

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_OTS))
    {
        SnmpStat.InASNParseErrs++;
        return (ASN1_ERR_DEC_BADVALUE);
    }

    /* Update the message buffer. */
    (*msg_length) -= (UINT32)(asn1.Pointer - *msg);
    (*msg) = asn1.Pointer;

    /* Create the iv. */
    for(i = 0; i < 8; i++)
        iv[i] = (UINT8)(param[i] ^ key[i + 8]);

    /* Decrypt the scopedPDU. */

    /* Setup the key-schedule */
    if (0 != DES_set_key((const_DES_cblock*)key, &schedule))
    {
        return (SNMP_ERROR);
    }

    /* Call for decryption */
    DES_ncbc_encrypt(*msg, *msg, *msg_length, &schedule, (DES_cblock*)iv, DES_DECRYPT);

    /* If the decoded buffer doesn't start with 0x30, it means a wrong
     * key was provided by the request. */
    if ((*msg)[0] != 0x30) 
    {
        return (ASN1_ERR_DEC_EOC_MISMATCH);
    }

    return (NU_SUCCESS);

} /* USM_Des_Decrypt */

#endif


