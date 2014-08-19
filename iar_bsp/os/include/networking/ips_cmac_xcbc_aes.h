/************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************/

/************************************************************************
*
*   FILE NAME
*
*       ips_cmac_xcbc_aes.h
*
*   COMPONENT
*
*       IPS - Cipher MAC XCBC using AES
*
*   DESCRIPTION
*
*       This file defines constants, data structures and function
*       prototypes used by IPSec Cipher-MAC XCBC APIs.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*       aes.h
*
************************************************************************/
#ifndef IPS_CMAC_XCBC_H
#define IPS_CMAC_XCBC_H

#include "nucleus.h"
#include "openssl/aes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPS_XCBC_BLOCK_SIZE                  16
#define IPS_XCBC_KEY_SIZE                    16
#define IPS_XCBC_DIGEST_LEN                  16

typedef struct _ips_cipher_mac_ctx
{
    UINT8           tmp_data[IPS_XCBC_BLOCK_SIZE];
    UINT8           tmp_digest[IPS_XCBC_DIGEST_LEN];
    UINT8           iv[16];
    AES_KEY         *aes_key;
    UINT8           *key_set;
    UINT16          data_len;

    /* Padding required to end on a 4 byte / 32 bit boundary.
    */
    UINT8           _pad[2];
} IPS_CIPHER_MAC_CTX;

/* Function definitions. */

/*
========================================================================
XCBC-MAC Mode (ips_cmac_xcbc.h, ips_cmac_xcbc.c)

This module implements the XCBC-MAC algorithm specifically using AES.
========================================================================
*/

/*
–   Initializes the XCBC-MAC context for the specified block
    encryption algorithm and key.
-   Dynamically allocate the key schedule for the encryption
    algorithm within the MAC context.
-   Call the key schedule initialization function of the
    encryption algorithm by indexing the encryption algorithm
    structure.
-   Set other fields of the context to zero.
*/
STATUS IPSEC_XCBC_MAC_Init(
        IPS_CIPHER_MAC_CTX *context,
        UINT8 *key, UINT8 key_len
        );


/*
–   Updates the XCBC-MAC state using the specified message.
-   Combine any bytes with message, buffered previously in the context.
-   If the message is a multiple of block cipher’s block size, then
    encrypt it in XCBC mode and store cipher text in ‘message’ buffer.
    Also store last block of cipher text in context.
-   If message not a multiple of block size, encrypt all except last
    block. Then store last block in context.
*/
STATUS IPSEC_XCBC_MAC_Update(
        IPS_CIPHER_MAC_CTX *context,
        UINT8 *message,
        UINT16 message_len
        );


/*
–   Finalizes the MAC/digest and stores it in the user specified
    buffer.
-   Perform the required padding.
-   Issue the last XCBC encryption call.
-   Get the last encryption call’s cipher text (stored in context if
    previous step is false) and copy it to the digest buffer.
-   Set digest length variable to encryption algorithm’s block size.
*/
STATUS IPSEC_XCBC_MAC_Final(
        IPS_CIPHER_MAC_CTX *context,
        UINT8 *digest,
        UINT *digest_len
        );

#ifdef __cplusplus
}
#endif


#endif /* IPS_CMAC_XCBC_H */

