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
*
*   FILE NAME
*
*       chapm_2.c
*
*   COMPONENT
*
*       MSCHAP - Microsoft Challenge Handshake Authentication Protocol
*
*   DESCRIPTION
*
*       This file contains the Microsoft challenge handshake
*       authentication protocol(v2) specific functions.
*
*   DATA STRUCTURES
*
*       CHAPM_2_Magic1
*       CHAPM_2_Magic2
*
*   FUNCTIONS
*
*       CHAPM_2_Check_Success
*       CHAPM_2_Chal_Resp_Verify
*       CHAPM_2_Generate_Peer_Response
*       CHAPM_2_Challenge_Hash
*       CHAPM_2_Gen_Auth_Resp
*       CHAPM_2_Check_Auth_Resp
*       CHAPM_2_Hash_Peer_Password_Hash
*
*   DEPENDENCIES
*
*       nu_ppp.h
*       md4.h
*       sha.h       
*
*************************************************************************/
#include "drivers/nu_ppp.h"

#if (PPP_USE_CHAP_MS2 == NU_TRUE)
#include "openssl/md4.h"
#include "openssl/sha.h"

/* "Magic" constants used in response generation for MSCHAP v2. */

UINT8 CHAPM_2_Magic1[39] =
{
    0x4D, 0x61, 0x67, 0x69, 0x63, 0x20, 0x73, 0x65, 0x72, 0x76,
    0x65, 0x72, 0x20, 0x74, 0x6F, 0x20, 0x63, 0x6C, 0x69, 0x65,
    0x6E, 0x74, 0x20, 0x73, 0x69, 0x67, 0x6E, 0x69, 0x6E, 0x67,
    0x20, 0x63, 0x6F, 0x6E, 0x73, 0x74, 0x61, 0x6E, 0x74
};

UINT8 CHAPM_2_Magic2[41] =
{
    0x50, 0x61, 0x64, 0x20, 0x74, 0x6F, 0x20, 0x6D, 0x61, 0x6B,
    0x65, 0x20, 0x69, 0x74, 0x20, 0x64, 0x6F, 0x20, 0x6D, 0x6F,
    0x72, 0x65, 0x20, 0x74, 0x68, 0x61, 0x6E, 0x20, 0x6F, 0x6E,
    0x65, 0x20, 0x69, 0x74, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6F,
    0x6E
};

/*************************************************************************
*
*   FUNCTION
*
*       CHAPM_2_Check_Success
*
*   DESCRIPTION
*
*       Verifies the success packet received from the authenticator by
*       verifying the response from the authenticator
*
*   INPUTS
*
*       *in_buf_ptr               Pointer to the incoming CHAP packet
*
*   OUTPUTS
*
*       NU_TRUE
*       NU_FALSE
*
*************************************************************************/
STATUS CHAPM_2_Check_Success(NET_BUFFER *in_buf_ptr)
{
    STATUS  status;
    UINT8                   peer_response[CHAP_MS_RESPONSE_SIZE];
    UINT8                   recv_response[CHAP_MS2_AUTH_RESPONSE_MSG_SIZE];
    AUTHENTICATION_LAYER    *auth;
    CHAR                    temp[PPP_MAX_ID_LENGTH];
    INT8                    *parameter;
    INT8                    *user;
    UINT8   HUGE            *chap_pkt = in_buf_ptr->data_ptr;

    /* Copy the response from the message field . */
    memcpy(recv_response, &chap_pkt[CHAP_MS2_MESSAGE_OFFSET],
           CHAP_MS2_AUTH_RESPONSE_MSG_SIZE);

    /* Get a pointer to the authentication structure. */
    auth = &(((LINK_LAYER *)in_buf_ptr->
                    mem_buf_device->dev_link_layer)->authentication);

    /* Generate the peer response */
    CHAPM_2_Generate_Peer_Response(auth->chap.challenge_value_ms_authenticator,
                                   auth->chap.challenge_value_ms_peer,
                                   (INT8 *)auth->login_name,
                                   (INT8 *)auth->login_pw,
                                   peer_response);

    parameter = (INT8 *)auth->login_name;
    strncpy(temp, auth->login_name, PPP_MAX_ID_LENGTH - 1);
    
    /* Ensure the string is null terminated */
    temp[PPP_MAX_ID_LENGTH - 1] = 0;

    user = (INT8 *)strtok(temp, "\\");
    while (user != NU_NULL)
    {
        parameter = user;
        user = (INT8 *)strtok(NU_NULL, "\\");
    }

    /* verify the authenticator response */
    status =
        CHAPM_2_Check_Auth_Resp((INT8 *)auth->login_pw, peer_response,
                                auth->chap.challenge_value_ms_peer ,
                                auth->chap.challenge_value_ms_authenticator,
                                parameter, recv_response);

    return (status);

} /* CHAPM_2_Check_Success*/

/*************************************************************************
*
*   FUNCTION
*
*       CHAPM_2_Chal_Resp_Verify
*
*   DESCRIPTION
*
*       Verifies the response received from the peer
*
*   INPUTS
*
*       *passwd                    Password
*       *user_name                 User name
*       *peer_chal                 Peer challenge
*       *auth_chal                 Authenticator challenge
*       *recv_response             Response received from the peer
*
*   OUTPUTS
*
*       NU_TRUE
*       NU_FALSE
*
*************************************************************************/
STATUS CHAPM_2_Chal_Resp_Verify(INT8 *passwd, INT8 *user_name,
                                UINT8 *peer_chal, UINT8 *auth_chal,
                                UINT8 *recv_response)
{
    /* Declaring Variables */
    UINT8  my_response[CHAP_MS_RESPONSE_SIZE];
    STATUS status = NU_FALSE;
    
    /* Initialize my_response to remove KW error. */
    memset(my_response, 0, CHAP_MS_RESPONSE_SIZE);

    /* Generate peer response on our(authenticator) side*/
    CHAPM_2_Generate_Peer_Response(auth_chal, peer_chal, user_name, passwd,
                                   my_response);

    /* Compare the received response with our own generated response */
    if (memcmp(my_response, recv_response, CHAP_MS_RESPONSE_SIZE) == 0)
    {
        status = NU_TRUE;
    }

    return (status) ;

} /* CHAPM_2_Chal_Resp_Verify */

/*************************************************************************
*
*   FUNCTION
*
*       CHAPM_2_Generate_Peer_Response
*
*   DESCRIPTION
*
*       This function generates the NT response field of the response
*       packet.
*
*   INPUTS
*
*       *auth_chal                  Authenticator Challenge Value
*       *peer_chal                  Peer challenge Value
*       *user_name                  User Name of a peer
*       *passwd                     Password
*       *response                   Peer response generated
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CHAPM_2_Generate_Peer_Response(UINT8 *auth_chal, UINT8 *peer_chal,
                                    INT8 *user_name, INT8 *passwd,
                                    UINT8 *response)
{
   /* Declaring Variables */
   UINT8 challenge[8];
   CHAR  temp[PPP_MAX_ID_LENGTH];
   INT8  *parameter = user_name;
   INT8  *user;
   UINT8 passwd_hash[CHAP_MS_PASSWD_HASH_SIZE];

   /* Copy user_name up to max ID length - 1 (save 1 byte for null termination) */
   strncpy(temp, (char *)user_name, PPP_MAX_ID_LENGTH - 1);

   /* Ensure the string is null terminated */
   temp[PPP_MAX_ID_LENGTH - 1] = 0;   
   
   user = (INT8 *)strtok(temp, "\\");
   
   while (user != NU_NULL)
   {
       parameter = user;
       user = (INT8 *)strtok(NU_NULL, "\\");
   }

   /* Get the challenge hash*/
   CHAPM_2_Challenge_Hash(peer_chal, auth_chal, parameter, challenge);

   /* Get the password hash */
   CHAPM_Passwd_Hash(passwd, passwd_hash);

   /* Generate peer response */
   CHAPM_Challenge_Response(challenge, passwd_hash, response);

} /* CHAPM_2_Generate_Peer_Response */

/*************************************************************************
*
*   FUNCTION
*
*       CHAPM_2_Challenge_Hash
*
*   DESCRIPTION
*
*       This function is used by the CHAPM_2_Generate_Peer_Response() to
*       to compute the challenge hash by using SHA.
*
*   INPUTS
*
*       *peer_chal                  Peer Challenge value
*       *auth_chal                  Authenticator Challenge
*       *user_name                  User Name
*       *chal                       challenge hash generated by this
*                                   function
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CHAPM_2_Challenge_Hash(UINT8 *peer_chal, UINT8 *auth_chal,
                            INT8 *user_name, UINT8 *chal)
{
    /* Declaring Variables */
    SHA_CTX      context;
    UINT8        digest [20];
    UINT8        digest_len = 8;

    /* Compute the hash using SHA */
    SHA1_Init(&context);
    SHA1_Update(&context, peer_chal, CHAP_MS2_CHALLENGE_VALUE_SIZE);
    SHA1_Update(&context, auth_chal, CHAP_MS2_CHALLENGE_VALUE_SIZE);
    SHA1_Update(&context, (UINT8 *)user_name, (UINT16)strlen((CHAR *)user_name));

    /* Generate digest. */
    SHA1_Final(digest, &context);

    /* Only first 8 bytes required. */
    memcpy(chal, digest, digest_len);

} /* CHAPM_2_Challenge_Hash */

/*************************************************************************
*
*   FUNCTION
*
*       CHAPM_2_Gen_Auth_Resp
*
*   DESCRIPTION
*
*       This function generates the response of the authenticator
*
*   INPUTS
*
*       *passwd                     Password
*       *peer_response              Response from the peer
*       *peer_chal                  Challenge from the peer
*       *auth_chal                  Authenticator's challenge
*       *user_name                  User name
*       *auth_response              Authenticator response generated
*
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CHAPM_2_Gen_Auth_Resp(INT8 *passwd, UINT8 *peer_response, UINT8
                           *peer_chal,  UINT8 *auth_chal, INT8 *user_name,
                           UINT8 *auth_response)
{
    /* Declaring Variables */
    SHA_CTX       context;
    UINT8         passwd_hash[CHAP_MS_PASSWD_HASH_SIZE];
    UINT8         passwd_hash_hash[CHAP_MS_PASSWD_HASH_SIZE];
    UINT8         challenge[8], digest[20], array[40], i, j = 0;

    /* Hash the password with MD4 */
    CHAPM_Passwd_Hash(passwd, passwd_hash);

    /* Now hash the hash with MD4 */
    CHAPM_2_Hash_Peer_Password_Hash(passwd_hash, passwd_hash_hash);

    /* Use SHA */
    SHA1_Init(&context);
    SHA1_Update(&context, passwd_hash_hash, CHAP_MS_PASSWD_HASH_SIZE);
    SHA1_Update(&context, peer_response, CHAP_MS_RESPONSE_SIZE);
    SHA1_Update(&context, CHAPM_2_Magic1, 39);
    SHA1_Final(digest, &context);

    /* Get the challenge hash */
    CHAPM_2_Challenge_Hash(peer_chal, auth_chal, user_name, challenge);

    SHA1_Init(&context);
    SHA1_Update(&context, digest, 20);
    SHA1_Update(&context, challenge, 8);
    SHA1_Update(&context, CHAPM_2_Magic2, 41);
    SHA1_Final(digest, &context);

    /*
     * Encode the value of 'digest' as "S=" followed by
     * 40 ASCII hexadecimal digits and return it in
     * as the authenticator response
     */
    for (i = 0; i < 20; i++)
    {
        /* get the first nibble (4 bytes) */
        array[j] = digest[i] & 240;
        array[j] >>= 4;

        /* get the second nibble (4 bytes) */
        array[j+1] = digest[i] & 15;

        if (array[j] >= 10)
        {
            array[j]= (CHAR)(0x37 + ((UINT8)array[j]));
        }
        else if (array[j] < 10)
        {
            array[j]= 48 + array[j];
        }

        if (array[j+1] >= 10)
        {
            array[j+1]= (CHAR)(0x37 + ((UINT8)array[j+1]));
        }
        else if (array[j+1] < 10)
        {
            array[j+1]= 48 + array[j+1];
        }

        j += 2;
    }

    /* response message starts with S= */
    memcpy(auth_response, "S=", 2);

    /* copy the response */
    memcpy(&auth_response[2], array, 40);

} /* CHAPM_2_Gen_Auth_Resp */

/*************************************************************************
*
*   FUNCTION
*
*       CHAPM_2_Check_Auth_Resp
*
*   DESCRIPTION
*
*       This function verifies the response from authenticator.
*
*   INPUTS
*
*       *passwd                     Password
*       *peer_response              NT response
*       *peer_chal                  Challenge sent by the peer
*       *auth_chal                  Challenge of the authenticator
*       *user_name                  User name
*       *recv_response              Response to be verified
*
*   OUTPUTS
*
*       NU_TRUE                     Verification successful
*       NU_FALSE                    Verification failed
*
*************************************************************************/
STATUS CHAPM_2_Check_Auth_Resp(INT8 *passwd, UINT8 *peer_response,
                               UINT8 *peer_chal, UINT8 *auth_chal,
                               INT8 *user_name, UINT8 *recv_response)
{
    /* Declaring Variables */
    UINT8  my_response[CHAP_MS2_AUTH_RESPONSE_MSG_SIZE];
    STATUS status = NU_FALSE;

    /* Generate authenticator response */
    CHAPM_2_Gen_Auth_Resp(passwd, peer_response, peer_chal, auth_chal,
                          user_name, my_response);

    /* Compare the received response with our own value */
    if (memcmp(my_response, recv_response,
               CHAP_MS2_AUTH_RESPONSE_MSG_SIZE) == 0)
    {
        /* response is authenticated */
        status = NU_TRUE;
    }

    return (status);

} /*CHAPM_2_Check_Auth_Resp */

/*************************************************************************
*
*   FUNCTION
*
*       CHAPM_2_Hash_Peer_Password_Hash
*
*   DESCRIPTION
*
*       This function uses the MD4 algorithm to irreversibly hash password
*       hash into password hash hash.
*
*   INPUTS
*
*       *passwd_hash                Hash of a password
*       *passwd_hash_hash           Hash of the password hash computed by
*                                   this function
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CHAPM_2_Hash_Peer_Password_Hash(UINT8 *passwd_hash,
                                     UINT8 *passwd_hash_hash)
{
    /* Irreversibly hash passwd_hash into passwd_hash_hash
     * using MD4.
     */

    MD4(passwd_hash, 16, passwd_hash_hash);

} /* CHAPM_2_Hash_Peer_Password_Hash */


#endif /*(PPP_USE_CHAP_MS2 == NU_TRUE)*/
