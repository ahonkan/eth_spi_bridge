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
*       ips_auth.h
*
* COMPONENT
*
*       AUTHENTICATOR
*
* DESCRIPTION
*
*       Definitions required by the Authenticator component.
*       This component interfaces with Nucleus OpenSSL to provide
*       authentication services.
*
* DATA STRUCTURES
*
*       IPSEC_AUTH_ALGO
*       IPSEC_AUTH_REQ
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IPS_AUTH_H
#define IPS_AUTH_H

#ifdef          __cplusplus
extern  "C" {                             /* C declarations in C++ */
#endif /* _cplusplus */

/* Define the HMAC MD5 key length (in bytes), dictated by RFC 2403. */
#define IPSEC_MD5_KEY_LEN                   16

/* Define the HMAC SHA1 key length (in bytes), dictated by RFC 2404. */
#define IPSEC_SHA1_KEY_LEN                  20

/* Define the HMAC SHA256 key length (in bytes). */
#define IPSEC_SHA256_KEY_LEN                32

/* Define the AES-XCBC-MAC-96 length (in bytes). */
#define IPSEC_XCBC_MAC_96_KEY_LEN           16

/* Defining the structure IPSEC_AUTH_ALGO. */
typedef struct ipsec_auth_algo
{
    /* Key length of the given algorithm. */
    UINT8   ipsec_key_len;

    /* Digest length of the authentication algorithm. */
    UINT8   ipsec_digest_len;

    /* Identifier used to specify an authentication algorithm. */
    UINT8   ipsec_algo_identifier;
}IPSEC_AUTH_ALGO;
/* End of IPSEC_AUTH_ALGO structure definition. */

/* Define IPsec request structure for authentication. */
typedef struct ipsec_auth_req
{
    UINT8   *ipsec_key;                     /* Auth key value. */
    UINT8   *ipsec_digest;                  /* Digest return position. */
    UINT8   ipsec_key_len;                  /* Given auth key length. */
    UINT8   ipsec_hash_algo;                /* hash algo ID. */
    UINT8   ipsec_digest_len;               /* Digest length required. */
    UINT8   ipsec_pad[1];                   /* Padding the structure. */
}IPSEC_AUTH_REQ;

/* Extern the global IPsec authentication algorithms array. */
extern const IPSEC_AUTH_ALGO IPSEC_Authentication_Algos[];

/* Macro for getting authentication algorithm ID from IPsec
   authentication algorithm index. */
#define IPSEC_GET_AUTH_ID(ipsec_index) \
            IPSEC_Authentication_Algos[ipsec_index].ipsec_algo_identifier

/* Macro for getting IPsec authentication algorithm
   ID from IPsec authentication algorithm index. */
#define IPSEC_GET_AUTH_ID(ipsec_index) \
            IPSEC_Authentication_Algos[ipsec_index].ipsec_algo_identifier

/* Macro for getting particular authentication algorithm key length. */
#define IPSEC_GET_AUTH_KEY_LEN(ipsec_index) \
            IPSEC_Authentication_Algos[ipsec_index].ipsec_key_len

/* Macro for getting particular auth algo digest length. */
#define IPSEC_GET_AUTH_DIGEST_LEN(ipsec_index) \
            IPSEC_Authentication_Algos[ipsec_index].ipsec_digest_len

/*** Function Prototypes. ***/
STATUS IPSEC_Calculate_Digest(NET_BUFFER *buf_ptr,
                              IPSEC_AUTH_REQ *request);
STATUS IPSEC_Get_Auth_Algo_Index(UINT8 *algo_id);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef IPS_AUTH_H */
