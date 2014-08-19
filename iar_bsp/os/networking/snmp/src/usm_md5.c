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
*       usm_md5.c                                                
*
*   DESCRIPTION
*
*       This file contains definitions of the functions required by
*       HMAC-MD5-96 Digest Authentication Protocol.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       USM_Md5_Secure
*       USM_Md5_Verify
*       USM_Md5_Key
*       USM_Md5_Key_Change
*
*   DEPENDENCIES
*
*       snmp.h
*       usm.h
*       usm_md5.h
*       ncs_api.h
*
************************************************************************/

#include "networking/snmp.h"
#include "networking/usm.h"
#include "networking/usm_md5.h"

#if (INCLUDE_SNMPv3 == NU_TRUE)
#include "openssl/hmac.h"
#include "openssl/md5.h"
#endif

#if (INCLUDE_SNMPv3 == NU_TRUE)

extern SNMP_ENGINE_STRUCT      Snmp_Engine;

/************************************************************************
*
*   FUNCTION
*
*       USM_Md5_Secure
*
*   DESCRIPTION
*
*       This function secures the outgoing message.
*
*   INPUTS
*
*       *key            The key to be used for signing the message.
*       key_length      The key length.
*       *msg            The message to sign.
*       msg_length      The message length.
*       *auth_param     Pointer to the location where the auth param are
*                       to be placed.
*
*   OUTPUTS
*
*       NU_SUCCESS.
*       SNMP_ERROR
*
*************************************************************************/
STATUS USM_Md5_Secure(UINT8 *key, UINT8 key_length, UINT8 *msg,
                      UINT32 msg_length, UINT8 *auth_param)
{
    UINT8   digest[16];
    UINT    digest_len;

    /* Check the length of the key. This must be 16. */
    if(key_length != 16)
    {
        return (SNMP_ERROR);
    }

    /* Make the request for HMAC hashing. */
    if (NULL == HMAC(EVP_md5(), key, key_length, msg, msg_length, digest, &digest_len))
    {
        return (SNMP_ERROR);
    }

    /* Only take 12 bytes. */ 
    memcpy(auth_param, digest, 12);

    /* Return success. */
    return (NU_SUCCESS);

} /* USM_Md5_Secure */

/************************************************************************
*
*   FUNCTION
*
*       USM_Md5_Verify
*
*   DESCRIPTION
*
*       This function verifies the incoming message.
*
*   INPUTS
*
*       *key            The key to use for authentication.
*       key_length      The key length
*       *param          The authentication parameters
*       param_length    Authentication parameters length
*       *msg            Message to be authenticated
*       msg_length      Message length
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*       USM_AUTHENTICATION_ERROR
*
*************************************************************************/
STATUS USM_Md5_Verify(UINT8 *key, UINT8 key_length, UINT8 *param,
                      UINT32 param_length, UINT8 *msg, UINT32 msg_length)
{
    STATUS          status;
    UINT8           retrieved_mac[12];
    UINT8           temp_buff[12];

    /* Is the paramLength correct? */
    if(param_length != 12)
        return (USM_AUTHENTICATION_ERROR);

    /* Retrieve the MAC. */
    NU_BLOCK_COPY(retrieved_mac, param, 12);

    /* Replace the msgAuthenticationParameters field with 0s. */
    UTL_Zero(param, 12);

    status = USM_Md5_Secure(key, key_length, msg, msg_length, temp_buff);

    if(status == NU_SUCCESS)
    {
        /* Compare the retrieved_mac with the calculated MAC. */
        if(memcmp(temp_buff, retrieved_mac, 12) != 0)
            return (USM_AUTHENTICATION_ERROR);
    }

    return (status);

} /* USM_Md5_Verify */


/************************************************************************
*
*   FUNCTION
*
*       USM_Md5_Key
*
*   DESCRIPTION
*
*       This function does the hashing using MD5.
*
*   INPUTS
*
*       *password               Password to be used to generate key.
*       password_length         Password length
*       *hashed_key             The hashed key.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS USM_Md5_Key(UINT8 *password, UINT8 password_length,
                   UINT8 *hashed_key)
{
   UINT8        password_buf[128];
#if (USM_PASSWORD_2_KEY == NU_TRUE)
   MD5_CTX      MD;
   UINT8        *cp;
   UINT32       password_index = 0;
   UINT32       count = 0, i = 0;

    /* initialize MD5 */
    MD5_Init(&MD);

    /* Allow other tasks that are ready at the same priority to execute
     * before the calling task resumes.
     */
    NU_Relinquish();

    cp = password_buf;

    if (password_length) 
    {
        for (i = 0; i < 128; i++)
            *cp++ = password[(i % password_length)];

        i = (64 % password_length);
    }
    else
    {
        memset(password_buf, 0, 128);
    }

    /************************************************/
    /* Use while loop until we have done 1 Megabyte */
    /************************************************/
    while (count < 1048576)
    {
        MD5_Update(&MD, &(password_buf[password_index]), 64);

        count += 64;

        password_index += i;

        if (password_index >= 0x40)
        {
            password_index += i;
            password_index &= 0x3f;
        }
    }

    /* Allow other tasks that are ready at the same priority to execute
     * before the calling task resumes.
     */
    NU_Relinquish();

    /* Tell MD5 we're done */
    MD5_Final(hashed_key, &MD);

#endif
    /*****************************************************/
    /* Now localize the key with the engineID and pass   */
    /* through MD5 to produce final key                  */
    /* May want to ensure that engineLength <= 32,       */
    /* otherwise need to use a buffer larger than 64     */
    /*****************************************************/
#if (USM_PASSWORD_2_KEY == NU_TRUE)

    NU_BLOCK_COPY(password_buf, hashed_key, 16);

    NU_BLOCK_COPY(password_buf+16, Snmp_Engine.snmp_engine_id,
                    (unsigned int)Snmp_Engine.snmp_engine_id_len);

    NU_BLOCK_COPY(password_buf+16+Snmp_Engine.snmp_engine_id_len,
                    hashed_key, 16);
#else
   NU_BLOCK_COPY(password_buf, password, password_length);

   NU_BLOCK_COPY(password_buf + password_length,
                Snmp_Engine.snmp_engine_id,
                (unsigned int)Snmp_Engine.snmp_engine_id_len);

   NU_BLOCK_COPY(password_buf + password_length +
                 Snmp_Engine.snmp_engine_id_len,
                 password, (unsigned int)password_length);
#endif

   MD5(password_buf, (32 + Snmp_Engine.snmp_engine_id_len), hashed_key);

   return (NU_SUCCESS);

} /* USM_Md5_Key */


/************************************************************************
*
*   FUNCTION
*
*       USM_Md5_Key_Change
*
*   DESCRIPTION
*
*       This function implements the key change algorithm for MD5.
*
*   INPUTS
*
*       *current_key
*       *new_key                Users new key
*       new_key_len             Key length
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS USM_Md5_Key_Change(UINT8 *current_key, UINT8 *new_key,
                          UINT32 new_key_len)
{
    UINT8           temp[32];
    UINT8           temp_key[16];
    UINT8           delta[USM_KEYCHANGE_MAX_SIZE];
    UINT8           random[16];
    UINT32          j;

    if(new_key_len != 32)
        return (SNMP_ERROR);

    /* Make a copy of delta and random from new_key. */
    NU_BLOCK_COPY(random, new_key, 16);
    NU_BLOCK_COPY(delta, &new_key[16], (unsigned int)(new_key_len - 16));

    NU_BLOCK_COPY(temp, current_key, 16);
    NU_BLOCK_COPY(&temp[16], random, 16);

    /* Perform MD5 */
    MD5(temp, 32, temp_key);

    for(j = 0; j < 16; j++)
    {
        new_key[j] = (UINT8)(temp_key[j] ^ delta[j]);
    }

    NU_BLOCK_COPY(current_key, new_key, 16);

    return (NU_SUCCESS);

} /* USM_Md5_Key_Change */

#endif


