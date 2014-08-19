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
*       mppe.c
*
*   COMPONENT
*
*       MPPE - Microsoft Point to Point Encryption
*
*   DESCRIPTION
*
*       This file contains the point to point encryption protocol used by PPP
*       to encrypt and decrypt the outgoing and incoming packets.
*
*   DATA STRUCTURES
*
*       MPPE_SHA_Pad1
*       MPPE_SHA_Pad2
*       MPPE_Magic1
*       MPPE_Magic2
*       MPPE_Magic3
*
*   FUNCTIONS
*
*       MPPE_Init
*       MPPE_Decrypt
*       MPPE_Encrypt
*       MPPE_Get_New_Key_From_SHA
*       MPPE_Get_Session_Key
*       MPPE_Get_Asymmetric_Start_Key
*       MPPE_Get_Key
*       MPPE_get_Master_Key
*       MPPE_Change_Key
*       PPP_Des_Hash
*       PPP_LM_Password_Hash
*
*   DEPENDENCIES
*
*       nu_ppp.h
*       arc4.h
*       sha.h
*       md4.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"

#if (PPP_ENABLE_MPPE == NU_TRUE)

#include "openssl/arc4.h"
#include "openssl/sha.h"
#include "openssl/md4.h"

/* Pads used in key derivation */
UINT8 MPPE_SHA_Pad1[MPPE_PAD_LENGTH] =
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

UINT8 MPPE_SHA_Pad2[MPPE_PAD_LENGTH] =
{0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2,
0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2,
0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2,
0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2, 0xf2};

UINT8 MPPE_Magic1[MPPE_MAGIC1_LENGTH] =
{0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74,
0x68, 0x65, 0x20, 0x4d, 0x50, 0x50, 0x45, 0x20, 0x4d,
0x61, 0x73, 0x74, 0x65, 0x72, 0x20, 0x4b, 0x65, 0x79};

UINT8 MPPE_Magic2[MPPE_MAGIC2_LENGTH] =
{0x4f, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x69,
0x65, 0x6e, 0x74, 0x20, 0x73, 0x69, 0x64, 0x65, 0x2c, 0x20,
0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
0x65, 0x20, 0x73, 0x65, 0x6e, 0x64, 0x20, 0x6b, 0x65, 0x79,
0x3b, 0x20, 0x6f, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20, 0x73,
0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x73, 0x69, 0x64, 0x65,
0x2c, 0x20, 0x69, 0x74, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
0x65, 0x20, 0x72, 0x65, 0x63, 0x65, 0x69, 0x76, 0x65, 0x20,
0x6b, 0x65, 0x79, 0x2e};

UINT8 MPPE_Magic3[MPPE_MAGIC3_LENGTH] =
{0x4f, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x69,
0x65, 0x6e, 0x74, 0x20, 0x73, 0x69, 0x64, 0x65, 0x2c, 0x20,
0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
0x65, 0x20, 0x72, 0x65, 0x63, 0x65, 0x69, 0x76, 0x65, 0x20,
0x6b, 0x65, 0x79, 0x3b, 0x20, 0x6f, 0x6e, 0x20, 0x74, 0x68,
0x65, 0x20, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x73,
0x69, 0x64, 0x65, 0x2c, 0x20, 0x69, 0x74, 0x20, 0x69, 0x73,
0x20, 0x74, 0x68, 0x65, 0x20, 0x73, 0x65, 0x6e, 0x64, 0x20,
0x6b, 0x65, 0x79, 0x2e};

/*************************************************************************
* FUNCTION
*
*     MPPE_Init
*
* DESCRIPTION
*
*     Initializes the MPPE layer of PPP. In general, it sets up
*     the default values that will be used each time the link
*     is reset.
*
* INPUTS
*
*     *dev_ptr                          Pointer to the device that this
*                                       MPPE layer belongs to.
*
* OUTPUTS
*
*     NU_SUCCESS                        Success.
*
*************************************************************************/
STATUS MPPE_Init(DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER      *link_layer;
    STATUS          status;
    UINT8           key_length;
    CCP_LAYER       *ccp;

    /* Get pointers to data. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &(link_layer->ccp); 

    key_length = ccp->options.local.mppe.mppe_key_length;

    /* Reset Flag. */
    ccp->options.local.mppe.mppe_reset_flag = NU_FALSE;

    /* resetting coherency counts. */
    ccp->options.local.mppe.mppe_coherency_count = 0;
    ccp->options.remote.mppe.mppe_coherency_count = 0;
    ccp->options.local.mppe.mppe_send_coherency_count = 0;

    /* Generate Session Keys. */
    status = MPPE_Get_Session_Key(dev_ptr, key_length);

    return (status);

} /* MPPE_Init */

/*************************************************************************
* FUNCTION
*
*     MPPE_Decrypt
*
* DESCRIPTION
*
*     Performs the decryption of MPPE data received.
*
* INPUTS
*
*     *buffer                           Pointer to NET_BUFFER
*     *dev_ptr                          Pointer to the device that this
*                                       MPPE layer belongs to.
*     change_key                        Flag to indicate whether to 
*                                       change key or not
*
* OUTPUTS
*
*     NU_SUCCESS                        Success.
*
*************************************************************************/
STATUS MPPE_Decrypt(NET_BUFFER * buffer, DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER                  *link_layer;
    CCP_LAYER                   *ccp;
    MPPE_INFO                   *mppe_info;
    UINT16                      key_change_count;
    UINT16                      i;
    NET_BUFFER                  *temp_buffer = buffer;
    STATUS                      status = NU_SUCCESS;
    UINT32                      decrypted_length = 0;
    ARC4_KEY                    key_arc4;
    UINT8                       key_length = MPPE_MAX_KEY_LEN;
    
    /* Get some pointers to our data. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;
    mppe_info = &(ccp->options.local.mppe);

    /* key length is 8 for both 40 and 56 bit encryption. */
    if ((mppe_info->mppe_key_length == MPPE_40_BIT) || 
        (mppe_info->mppe_key_length == MPPE_56_BIT))
    {
        key_length = 8;
    }

    /* If stateless mode is being used. */
    if (ccp->options.local.mppe.mppe_stateless == NU_TRUE)
    {
        /* Determine the difference (N = C1 – C2) in coherency count of the  
         * received (C1) and previously received packet (C2). */
        key_change_count = (ccp->options.remote.mppe.mppe_coherency_count -
            ccp->options.local.mppe.mppe_coherency_count) & CCP_MAX_CCOUNT;

        if ((ccp->options.local.mppe.mppe_coherency_count == 0) && 
            (ccp->options.remote.mppe.mppe_coherency_count == 0))
        {
            /* Initially we need to change key when coherency counts 
             * contain the arbitrary initial values. */
            key_change_count = 1;
        }
        
        /* Perform N number of key changes. */
        for (i = 0; i < key_change_count; i++)
            MPPE_Change_Key(dev_ptr, MPPE_CHANGE_RECEIVE_REQUEST);

    }
    /* Otherwise, if a stateful mode is being used.  */
    else
    {
        /* If flushed bit is set in the received packet. */
        if (ccp->options.remote.mppe.mppe_cntrl_bits & CCP_FLAG_A_BIT)
        {
          /* Coherency count will be set later in this function. */
          ccp->options.local.mppe.mppe_reset_flag = NU_FALSE;
            
        }   

        else if (ccp->options.local.mppe.mppe_reset_flag == NU_TRUE)
        {
            /* Packet dropped */
            status = MPPE_GEN_ERROR;
        }

        /* If low order octet of coherency count is 0xFF then change key*/
        if ((status == NU_SUCCESS) && 
            ((ccp->options.remote.mppe.mppe_coherency_count & 
            CCP_CCOUNT_LOWER_OCTET) == CCP_CCOUNT_LOWER_OCTET))
        {
            /* Re-generate a key. */
            MPPE_Change_Key(dev_ptr, MPPE_CHANGE_RECEIVE_REQUEST); 
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Set the local coherency count with the value of the received
         * coherency count. */
        ccp->options.local.mppe.mppe_coherency_count = 
            ccp->options.remote.mppe.mppe_coherency_count;

        /* Decrypt the complete packet in buffer chain. */
        while ((temp_buffer != NU_NULL) && 
            (buffer->mem_total_data_len > decrypted_length))
        {
            ARC4_set_key(&key_arc4, key_length,mppe_info->mppe_receive_session_key);
            ARC4(&key_arc4,temp_buffer->data_len ,temp_buffer->data_ptr,temp_buffer->data_ptr);

            /* Add the length in decrypted_length. */
            decrypted_length += temp_buffer->data_len;

            /* Move the temporary pointer to the next buffer 
                 * in the list. */
            temp_buffer = temp_buffer->next_buffer;
            
        }
    }

    return (status);

} /* MPPE_Decrypt */

/*************************************************************************
* FUNCTION
*
*     MPPE_Encrypt
*
* DESCRIPTION
*
*     Performs the encryption of MPPE data to be sent.
*
* INPUTS
*
*     *buffer                           Pointer to NET_BUFFER
*     *dev_ptr                          Pointer to the device that this
*                                       MPPE layer belongs to.
*     change_key                        Flag to indicate whether to 
*                                       change key or not
*
* OUTPUTS
*
*     NU_SUCCESS                        Success.
*
*************************************************************************/
STATUS MPPE_Encrypt(NET_BUFFER * buffer, DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER                  *link_layer;
    CCP_LAYER                   *ccp;
    MPPE_INFO                   *mppe_info;
    STATUS                      status = NU_SUCCESS;
    NET_BUFFER                  *temp_buffer = buffer;
    UINT32                      encrypted_length = 0;
    ARC4_KEY                    key_arc4;
    UINT8                       key_length = MPPE_MAX_KEY_LEN;

    /* Get some pointers to our data. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;
    mppe_info = &(ccp->options.local.mppe);

   /* key length is 8 for both 40 and 56 bit encryption. */
   if ((mppe_info->mppe_key_length == MPPE_40_BIT) || 
   (mppe_info->mppe_key_length == MPPE_56_BIT))
   {
      key_length = 8;
   }

    /* If stateless mode is being used. */
    if (ccp->options.local.mppe.mppe_stateless == NU_TRUE)
    {   
        MPPE_Change_Key(dev_ptr, MPPE_CHANGE_SEND_REQUEST);
    }
    /* Otherwise, if a stateful mode is being used.  */
    else
    {           
        /* We are about to send a flag packet */
        if ((ccp->options.local.mppe.mppe_send_coherency_count & 
            CCP_CCOUNT_LOWER_OCTET) == CCP_CCOUNT_LOWER_OCTET)
        {
            /* Re-generate a key. */
            MPPE_Change_Key(dev_ptr, MPPE_CHANGE_SEND_REQUEST);
        }
    }

    while ((temp_buffer != NU_NULL) &&
           buffer->mem_total_data_len > encrypted_length)
    {
        ARC4_set_key(&key_arc4, key_length,mppe_info->mppe_send_session_key);
        ARC4(&key_arc4,temp_buffer->data_len ,temp_buffer->data_ptr,temp_buffer->data_ptr);
         
        encrypted_length += temp_buffer->data_len;
        temp_buffer = temp_buffer->next_buffer;
    }

    return (status);

} /* MPPE_Encrypt */

/*************************************************************************
* FUNCTION
*
*     MPPE_Get_New_Key_From_SHA
*
* DESCRIPTION
*
*     Used to generate session keys using Secure Hash Algorithm
*     SHA1. SessionKeyLength is 8 for 40-bit and 56-bit 
*     keys, 16 for 128-bit keys.
*
* INPUTS
*
*     *start_key                        Pointer to the start key.
*     *session_key                      Pointer to current session key.
*     session_key_length                length of session key.
*     interim key                       digest output.
*
* OUTPUTS
*
*     NU_SUCCESS                        Success
*
*************************************************************************/
STATUS MPPE_Get_New_Key_From_SHA (UINT8* start_key, UINT8* session_key, 
                                  UINT8 session_key_length, 
                                  UINT8* interim_key)
{
    SHA_CTX         context;    
    STATUS          status;

    /* Local variables for storing the message digest  Digest length for SHA-1 is 20 bytes. */
    UINT8 digest[MPPE_DIGEST_LENGTH] = {0};

    /* Generate Session Keys using Start Key, Session Key and Pad. */
    status = SHA1_Init(&context);
    

    if (status != NU_SUCCESS)
    {
        return (status);
    }

    status = SHA1_Update(&context, start_key, session_key_length);

    if (status != NU_SUCCESS)
    {
        return (status);
    }

    status = SHA1_Update(&context, MPPE_SHA_Pad1, MPPE_PAD_LENGTH);

    if (status != NU_SUCCESS)
    {
        return (status);
    }

    status = SHA1_Update(&context, session_key, session_key_length);

    if (status != NU_SUCCESS)
    {
        return (status);
    }

    status = SHA1_Update(&context, MPPE_SHA_Pad2, MPPE_PAD_LENGTH);

    if (status != NU_SUCCESS)
    {
        return (status);
    }

    status =  SHA1_Final(digest,&context);

    if (status != NU_SUCCESS)
    {
        return (status);
    }

    /* Copy the output digest to interim_key */
    NU_BLOCK_COPY(interim_key, digest, session_key_length);

    return (status);

} /* MPPE_Get_New_Key_From_SHA */

/*************************************************************************
* FUNCTION
*
*     MPPE_Change_Key
*
* DESCRIPTION
*
*     This function changes the session key.
*
* INPUTS
*
*     *dev_ptr                          Pointer to the device that this
*                                       MPPE layer belongs to.
*     in_out_flag                       Flag to indicate the key to 
*                                       change.
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS MPPE_Change_Key(DV_DEVICE_ENTRY *dev_ptr, UINT8 in_out_flag)
{
    MPPE_INFO       *mppe_info;
    LINK_LAYER      *link_layer;
    CCP_LAYER       *ccp;
    UINT8           session_key[MPPE_SESSION_KEY_LEN];
    UINT8           interim_key[MPPE_MAX_KEY_LEN];
    STATUS          status;
    UINT8           key_length = MPPE_MAX_KEY_LEN;
    ARC4_KEY        key_arc4;

    /* get pointers to our data. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &(link_layer->ccp);
    mppe_info = &(ccp->options.local.mppe);

    /* key length is 8 for both 40 and 56 bit encryption. */
    if ((mppe_info->mppe_key_length == MPPE_40_BIT) || 
        (mppe_info->mppe_key_length == MPPE_56_BIT))
    {
        key_length = 8;
    }

    /* Step 1  MPPE_Get_New_Key_From_SHA. */

    /* Do we have to change receive key. */
    if (in_out_flag == MPPE_CHANGE_RECEIVE_REQUEST)
    {
        status = 
            MPPE_Get_New_Key_From_SHA(mppe_info->mppe_master_receive_key,
                                      mppe_info->mppe_receive_session_key,
                                      key_length, interim_key);
    }
    /* or send key. */
    else
    {
        status = 
            MPPE_Get_New_Key_From_SHA(mppe_info->mppe_master_send_key,
                                      mppe_info->mppe_send_session_key,
                                      key_length, interim_key);
    }

    if (status == NU_SUCCESS)
    {
        /* Step 2: Copy interim_key to session key. */
        NU_BLOCK_COPY(session_key, interim_key, key_length);

        /* Step 3: Encrypt session key with the interim key. */
        
        ARC4_set_key(&key_arc4, key_length,interim_key);
        ARC4(&key_arc4,key_length ,session_key,session_key);

        if (status == NU_SUCCESS)
        {
            /* For 40-bit and 56 bit session keys, the most significant
             * octets are set to know values thus reducing the effective
             * key length */
            switch(mppe_info->mppe_key_length)
            {
            case MPPE_40_BIT:
                session_key[0] = MPPE_SALT1;
                session_key[1] = MPPE_SALT2;
                session_key[2] = MPPE_SALT3;
                break;

            case MPPE_56_BIT:
                session_key[0] = MPPE_SALT1;
                break;

            case MPPE_128_BIT:
                break;

            default:
                break;
            }

            /* Reinitializing the ARC4 tables in MPPE_INFO structure */
            if (in_out_flag == MPPE_CHANGE_RECEIVE_REQUEST)
            {
                NU_BLOCK_COPY(mppe_info->mppe_receive_session_key, 
                              session_key, key_length);
            }
            else
            {
                NU_BLOCK_COPY(mppe_info->mppe_send_session_key, 
                              session_key, key_length);
            }
        }
        else
        {
            NLOG_Error_Log("Error Encrypting NCS_Stream_Encrypt", 
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Error Getting new key from SHA", NERR_SEVERE, 
            __FILE__, __LINE__);
    }

    return (status);

} /* MPPE_Change_Key */

/*************************************************************************
* FUNCTION
*
*     MPPE_Get_Session_Key
*
* DESCRIPTION
*
*     Initializes the MPPE initial keys and master keys.
*
* INPUTS
*
*     *dev_ptr                          Pointer to the device that this
*                                       MPPE layer belongs to.
*     key_length                        Length of Key.
*
* OUTPUTS
*
*     NU_SUCCESS                        Success.
*
*************************************************************************/
STATUS MPPE_Get_Session_Key (DV_DEVICE_ENTRY *dev_ptr, UINT8 key_length)
{
    LINK_LAYER              *link_layer;
    AUTHENTICATION_LAYER    *auth;
    MPPE_INFO               *mppe_info;
    LCP_LAYER               *lcp;
    CCP_LAYER               *ccp;
    UINT8                   *master_key;
    UINT8                   *session_key;
    UINT8                   *password_hash;
    UINT8                   *master_send_key;
    UINT8                   *send_session_key;
    UINT8                   *password_hash_hash;
    UINT8                   *master_receive_key;
    UINT8                   *initial_session_key;
    UINT8                   *current_session_key;
    UINT8                   *receive_session_key;
    UINT8                   *nt_password_hash_hash;
    UINT8                   key_len;
    STATUS                  status;
    UINT8                   digest_len = 0;
    UINT8                   digest [20];
    CHAR                    password[MPPE_MAX_PASSWORD_LEN] = {0};

    /* Allocate memory for the temporary keys. */
    status = NU_Allocate_Memory(PPP_Memory, (VOID **)&master_key,
                                MPPE_MAX_KEY_LEN * 11,
                                NU_NO_SUSPEND);

    if ( status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory.", NERR_RECOVERABLE,
            __FILE__, __LINE__);
        return status;
    }

    /* Set the pointers needed later for key generation. */
    session_key = master_key + MPPE_MAX_KEY_LEN;
    password_hash = session_key + MPPE_MAX_KEY_LEN;
    master_send_key = password_hash + MPPE_MAX_KEY_LEN;
    send_session_key = master_send_key + MPPE_MAX_KEY_LEN;
    password_hash_hash = send_session_key + MPPE_MAX_KEY_LEN;
    master_receive_key = password_hash_hash + MPPE_MAX_KEY_LEN;
    initial_session_key = master_receive_key + MPPE_MAX_KEY_LEN;
    current_session_key = initial_session_key + MPPE_MAX_KEY_LEN;
    receive_session_key = current_session_key + MPPE_MAX_KEY_LEN;
    nt_password_hash_hash = receive_session_key + MPPE_MAX_KEY_LEN;


    /* Get some pointers to our data. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    auth = &(link_layer->authentication);
    lcp = &(link_layer->lcp);
    ccp = &(link_layer->ccp);
    mppe_info = &(ccp->options.local.mppe);

    strcpy(password, auth->login_pw);

    /* If MS-CHAPv1 is being used. */
    if (((link_layer->mode == PPP_SERVER) && 
        (lcp->options.local.chap_protocol == LCP_CHAP_MS1) ||
        (link_layer->mode == PPP_CLIENT) && 
        (lcp->options.remote.chap_protocol == LCP_CHAP_MS1)))
    {
        switch (key_length)
        {
        case MPPE_40_BIT:
        case MPPE_56_BIT:

            /* Obfuscate peer’s password. */
            MPPE_LM_Password_Hash((UINT8*)password, password_hash);

            NU_BLOCK_COPY(session_key, password_hash, MPPE_MAX_KEY_LEN);
            NU_BLOCK_COPY(mppe_info->mppe_master_send_key, session_key, 
                          MPPE_40BIT_KEY_LEN);
            NU_BLOCK_COPY(mppe_info->mppe_master_receive_key, session_key,
                          MPPE_40BIT_KEY_LEN);

            /* Generate a session key. */
            status = MPPE_Get_Key(password_hash, session_key, 
                                  MPPE_40BIT_KEY_LEN);
            
            if (status == NU_SUCCESS)
            {
                /* Update session key headers for 40-bit by setting the
                * first three octets to known constants. */
                session_key[0] = MPPE_SALT1;
                if (key_length == MPPE_40_BIT)
                {
                    session_key[1] = MPPE_SALT2;
                    session_key[2] = MPPE_SALT3;
                }

                /* Copying keys to the MPPE structure. */
                NU_BLOCK_COPY(mppe_info->mppe_receive_session_key, 
                            session_key, MPPE_40BIT_KEY_LEN);
                NU_BLOCK_COPY(mppe_info->mppe_send_session_key, session_key, 
                            MPPE_40BIT_KEY_LEN);
            }

            break;

        case MPPE_128_BIT:

            key_len = MPPE_128_BIT;

            /* Obfuscate the peer’s password. */
            CHAPM_Passwd_Hash((INT8*)password, password_hash);

            /* Re-Hash the first 16 octets of the result using MD4 to get
             * NtPasswordHashHash. */
            digest_len = MPPE_MAX_KEY_LEN;
            MD4(password_hash, MPPE_MAX_KEY_LEN,digest);
            memcpy(nt_password_hash_hash, digest, digest_len);
            
            /* Get the Initial Session Key. */
            status = MPPE_Get_Start_Key(auth->chap.challenge_value_ms_authenticator,
                                        nt_password_hash_hash, 
                                        initial_session_key);
            if (status == NU_SUCCESS)
            {
                NU_BLOCK_COPY(mppe_info->mppe_master_send_key, 
                            initial_session_key, 
                            MPPE_128BIT_KEY_LEN);
                NU_BLOCK_COPY(mppe_info->mppe_master_receive_key, 
                            initial_session_key, MPPE_128BIT_KEY_LEN);
                NU_BLOCK_COPY(current_session_key, initial_session_key, 
                            MPPE_128BIT_KEY_LEN);

                /* Get the final 128-bit session key. */
                status = MPPE_Get_Key(initial_session_key, 
                                    current_session_key, key_len);
                if (status == NU_SUCCESS)
                {
                    /* Use the CurrentSessionKey both for sending and receiving. */
                    NU_BLOCK_COPY(mppe_info->mppe_send_session_key, 
                                  current_session_key,
                                  MPPE_128BIT_KEY_LEN);
                    NU_BLOCK_COPY(mppe_info->mppe_receive_session_key, 
                                current_session_key, MPPE_128BIT_KEY_LEN);
                }
            }
        }
    }

    /* Otherwise, if MS-CHAPv2 is being used. */
    else if ((link_layer->mode == PPP_SERVER && 
        lcp->options.local.chap_protocol == LCP_CHAP_MS2) ||
        (link_layer->mode == PPP_CLIENT && 
        lcp->options.remote.chap_protocol == LCP_CHAP_MS2))
    {

        /* Obfuscate the peer’s password. */
        CHAPM_Passwd_Hash((INT8*)password, password_hash);

        /* Re-Hash the first 16 octets of the result using MD4 to get
         * PasswordHashHash. */

        /* Initialize the Hash Context */
        digest_len = MPPE_MAX_KEY_LEN;
        MD4(password_hash, MPPE_MAX_KEY_LEN,digest);
        memcpy(password_hash_hash, digest, digest_len);

        /* Generate a Master key by re-hashing with the MS-CHAP-Response 
         * (NtResponse). */
        status = MPPE_Get_Master_Key(password_hash_hash, 
                                     auth->chap.challenge_response, 
                                     master_key);

        switch (key_length)
        {
        case MPPE_40_BIT:
        case MPPE_56_BIT:

            /* If we are in Server Mode. */
            if (link_layer->mode == PPP_SERVER)
            {
                /* Derive two 40-bit or 56-bit session keys from the 
                 * Master key, one for sending and one for Receiving. */
                MPPE_Get_Asymmetric_Start_Key(master_key, master_send_key,
                                              MPPE_40BIT_KEY_LEN, 
                                              NU_TRUE, NU_TRUE);
                MPPE_Get_Asymmetric_Start_Key(master_key, master_receive_key,
                                              MPPE_40BIT_KEY_LEN, 
                                              NU_FALSE, NU_TRUE);

                /* Finalize the two 40-bit or 56-bit Transient keys. */
                MPPE_Get_New_Key_From_SHA(master_send_key, master_send_key,
                                          MPPE_40BIT_KEY_LEN, 
                                          send_session_key);
                MPPE_Get_New_Key_From_SHA(master_receive_key, 
                                          master_receive_key,
                                          MPPE_40BIT_KEY_LEN, 
                                          receive_session_key);
            }

            /* or Client Mode. */
            else
            {
                /* Derive two 40-bit or 56-bit session keys from the 
                 * Master key one for sending and one for Receiving. */
                MPPE_Get_Asymmetric_Start_Key(master_key, master_receive_key,
                                              MPPE_40BIT_KEY_LEN, 
                                              NU_TRUE, NU_TRUE);
                MPPE_Get_Asymmetric_Start_Key(master_key, master_send_key,
                                              MPPE_40BIT_KEY_LEN, 
                                              NU_FALSE, NU_TRUE);

                /* Finalize the two 40-bit or 56-bit Transient keys. */
                MPPE_Get_New_Key_From_SHA(master_receive_key, 
                                          master_receive_key,
                                          MPPE_40BIT_KEY_LEN,
                                          receive_session_key);
                MPPE_Get_New_Key_From_SHA(master_send_key, master_send_key, 
                                          MPPE_40BIT_KEY_LEN,
                                          send_session_key);
            }

            /* Update session key headers for 40-bit by setting the
             * first three octets to known constants. */
            send_session_key[0] = MPPE_SALT1;
            receive_session_key[0] = MPPE_SALT1;
            
            if (key_length == MPPE_40_BIT)
            {
                send_session_key[1] = MPPE_SALT2;
                receive_session_key[1] = MPPE_SALT2;
                send_session_key[2] = MPPE_SALT3;
                receive_session_key[2] = MPPE_SALT3;
            }

            /* Copying keys to the MPPE structure. */
            NU_BLOCK_COPY(mppe_info->mppe_receive_session_key, 
                   receive_session_key, MPPE_40BIT_KEY_LEN);
            NU_BLOCK_COPY(mppe_info->mppe_master_receive_key, 
                   master_receive_key, MPPE_40BIT_KEY_LEN);

            NU_BLOCK_COPY(mppe_info->mppe_send_session_key, send_session_key,
                   MPPE_40BIT_KEY_LEN);
            NU_BLOCK_COPY(mppe_info->mppe_master_send_key, master_send_key,
                   MPPE_40BIT_KEY_LEN);

            break;

        case MPPE_128_BIT:

            /* If we are in Server Mode. */
            if (link_layer->mode == PPP_SERVER)
            {
                /* Derive two 128-bit session keys from the Master key, one for 
                sending and one for Receiving. */
                MPPE_Get_Asymmetric_Start_Key(master_key, master_send_key,
                                              MPPE_128BIT_KEY_LEN, 
                                              NU_TRUE, NU_TRUE);
                MPPE_Get_Asymmetric_Start_Key(master_key, 
                                              master_receive_key,
                                              MPPE_128BIT_KEY_LEN, 
                                              NU_FALSE, NU_TRUE);

                /* Finalize the two 128-bit Transient keys. */
                MPPE_Get_New_Key_From_SHA(master_send_key, 
                                          master_send_key,
                                          MPPE_128BIT_KEY_LEN, 
                                          send_session_key);
                MPPE_Get_New_Key_From_SHA(master_receive_key, 
                                          master_receive_key,
                                          MPPE_128BIT_KEY_LEN,
                                          receive_session_key);
            }

            /* or Client Mode. */
            else
            {
                /* Derive two 128-bit session keys from the Master key, one for 
                sending and one for Receiving. */
                MPPE_Get_Asymmetric_Start_Key(master_key, master_receive_key,
                                              MPPE_128BIT_KEY_LEN, 
                                              NU_TRUE, NU_TRUE);
                MPPE_Get_Asymmetric_Start_Key(master_key, master_send_key,
                                              MPPE_128BIT_KEY_LEN, 
                                              NU_FALSE, NU_TRUE);

                /* Finalize the two 128-bit Transient keys. */
                MPPE_Get_New_Key_From_SHA(master_receive_key, 
                                          master_receive_key, 
                                          MPPE_128BIT_KEY_LEN,
                                          receive_session_key);
                MPPE_Get_New_Key_From_SHA(master_send_key, 
                                          master_send_key, 
                                          MPPE_128BIT_KEY_LEN,
                                          send_session_key);
            }

            /* Copying keys to the MPPE structure. */
            NU_BLOCK_COPY(mppe_info->mppe_receive_session_key, 
                   receive_session_key, MPPE_128BIT_KEY_LEN);
            NU_BLOCK_COPY(mppe_info->mppe_master_receive_key, master_receive_key,
                   MPPE_128BIT_KEY_LEN);

            NU_BLOCK_COPY(mppe_info->mppe_send_session_key, send_session_key,
                   MPPE_128BIT_KEY_LEN);
            NU_BLOCK_COPY(mppe_info->mppe_master_send_key, master_send_key, 
                   MPPE_128BIT_KEY_LEN);
            mppe_info->mppe_key_length = MPPE_128_BIT;
            break;
        }
    }

    /* Done with the memory for the keys so give it back. */
    if ( (NU_Deallocate_Memory (master_key)) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate memory.", NERR_RECOVERABLE,
        __FILE__, __LINE__);
    }

    return (status);

} /* MPPE_Get_Session_Key */

/*************************************************************************
* FUNCTION
*
*     MPPE_Des_Hash
*
* DESCRIPTION
*
*     Irreversibly encrypt clear text into cipher using a known string.
*
* INPUTS
*
*     *clear                            Clear text.
*     *cipher                           cipher text.
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID MPPE_Des_Hash(UINT8 *clear, UINT8 *cipher)
{

    UINT8 std_text[MPPE_STD_TEXT_LEN] = MPPE_STD_TEXT;

    /* Call Nucleus PPP routine to encrypt. */ 
    CHAPM_Des_Encrypt(std_text, clear, cipher);

} /* PPP_Des_Hash */

/*************************************************************************
* FUNCTION
*
*     MPPE_LM_Passwor_Hash
*
* DESCRIPTION
*
*     Calculates the LM Hash of the password
*
* INPUTS
*
*     *password                         Pointer to the password.
*     *password_hash                    output hash.
*
* OUTPUTS
*
*     None.
*
*************************************************************************/
VOID MPPE_LM_Password_Hash(UINT8* password, UINT8* password_hash)
{
    int             i;

    /* Converting to uppercase. */
    for (i = 0; i < MPPE_MAX_PASSWORD_LEN; ++i)
    {
        if (password[i] >= 'a' && password[i] <= 'z')
            password[i] -= ('a' - 'A');
    }

    /* First half of Password is hashed. */
    MPPE_Des_Hash(password, password_hash);

    /* Second Half of the password is hashed. */
    MPPE_Des_Hash(password + 7, password_hash + 8);

} /* PPP_LM_Password_Hash */

/*************************************************************************
* FUNCTION
*
*     MPPE_Get_Key
*
* DESCRIPTION
*
*     Generates the initial session key using the master key
*
* INPUTS
*
*     *initial_session_key              Initial Session Key (Master Key)
*     *current_session_key              Session Key.
*     length_of_desired_key             Key length.
*
* OUTPUTS
*
*     NU_SUCCESS                        Success.
*
*************************************************************************/
STATUS MPPE_Get_Key(UINT8* initial_session_key, UINT8* current_session_key, 
                    UINT8 length_of_desired_key)
{
    /* Local variables for storing the message digest . Digest length for SHA-1 is 20 bytes. */
    UINT8           digest[MPPE_DIGEST_LENGTH];
    SHA_CTX         context; 
    STATUS          status;

    /* Generate the Initial Session Key using the Current Session Key 
     * and Pad values. */
    status =  SHA1_Init(&context);

    if (status != NU_SUCCESS)
    {
        return (status);
    }

    status = SHA1_Update(&context, initial_session_key,
                             length_of_desired_key);

    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, MPPE_SHA_Pad1, MPPE_PAD_LENGTH);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, current_session_key, 
                             length_of_desired_key);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, MPPE_SHA_Pad2, MPPE_PAD_LENGTH);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status =  SHA1_Final(digest, &context);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }

    NU_BLOCK_COPY(current_session_key, digest, length_of_desired_key);

    return (status);

} /* MPPE_Get_Key */

/*************************************************************************
* FUNCTION
*
*     MPPE_Get_Start_Key
*
* DESCRIPTION
*
*     Derives the master key using the challenge and the NT Password
*     hash hash in case of 128 bit encryption and MSCHAP v1.
*
* INPUTS
*
*     *challenge                        Peer challenge.
*     *nt_password_hash_hash            Password hash hash.
*     *initial_session_key              Initial Session Key.
*
* OUTPUTS
*
*     NU_SUCCESS                        Success.
*
*************************************************************************/
STATUS MPPE_Get_Start_Key(UINT8* challenge, UINT8* nt_password_hash_hash,  
                          UINT8* initial_session_key)
{
    SHA_CTX         context;
    STATUS          status;
    /* Local variables for storing the message digest . Digest length for SHA-1 is 20 bytes. */
    UINT8           digest[MPPE_DIGEST_LENGTH];

    status = SHA1_Init(&context);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, nt_password_hash_hash, 
                             MPPE_MAX_KEY_LEN);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, nt_password_hash_hash, 
                             MPPE_MAX_KEY_LEN);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, challenge, 8);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Final(digest, &context);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }

    NU_BLOCK_COPY(initial_session_key, digest, MPPE_MAX_KEY_LEN);
    
    return (status);

} /* MPPE_Get_Start_Key */

/*************************************************************************
* FUNCTION
*
*     MPPE_Get_Master_Key
*
* DESCRIPTION
*
*     Generates the Master key using password hash hash and NT response
*     for MSCHAPv2
*
* INPUTS
*
*     *password_hash_hash               Pointer to Password Hash Hash.
*     *nt_response                      Pointer to NT Response.
*     *master_key                       Pointer to Master Key to be 
*                                       returned.
*
* OUTPUTS
*
*     NU_SUCCESS                        Success.
*
*************************************************************************/
STATUS MPPE_Get_Master_Key(UINT8* password_hash_hash, UINT8* nt_response, 
                           UINT8* master_key)
{
    /* Local variables for storing the message digest . Digest length for SHA-1 is 20 bytes. */
    UINT8           digest[MPPE_DIGEST_LENGTH];
    SHA_CTX         context;
    STATUS          status;

    /* Calculate the Master Key using Password Hash Hash, NT Response,
     * and Magic. */
    status = SHA1_Init(&context);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, password_hash_hash, 
                             MPPE_MAX_KEY_LEN);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, nt_response, 
                             MPPE_NT_RESPONSE_LENGTH);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, MPPE_Magic1, MPPE_MAGIC1_LENGTH);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Final(digest, &context);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }

    NU_BLOCK_COPY(master_key, digest, MPPE_MAX_KEY_LEN);
    
    return (status);

} /* MPPE_Get_Master_Key */

/*************************************************************************
* FUNCTION
*
*     MPPE_Get_Asymmetric_Start_Key
*
* DESCRIPTION
*
*     Derives the master send key and the master receive key for MSCHAPv2.
*
* INPUTS
*
*     *master key                       Pointer to Master Key.
*     *session_key                      Pointer to Session Key.
*     session_key_length                Length of Key.
*     is_send                           
*     is_server                         Server.
*
* OUTPUTS
*
*     NU_SUCCESS
*
*************************************************************************/
STATUS MPPE_Get_Asymmetric_Start_Key (UINT8* master_key, UINT8* session_key, 
                                      UINT8 session_key_length, UINT8 is_send, 
                                      UINT8 is_server)
{
    /* Local variables for storing the message digest . Digest length for SHA-1 is 20 bytes. */
    UINT8           digest[MPPE_DIGEST_LENGTH];
    UINT8           *ptr;
    SHA_CTX         context;
    STATUS          status;

    /* Deciding the magic to be used later. */
    if (is_send) 
    {
        if (is_server) 
        {
            ptr = MPPE_Magic3;
        } 
        else 
        {
            ptr = MPPE_Magic2;
        }
    } 
    else 
    {
        if (is_server) 
        {
            ptr = MPPE_Magic2;
        } 
        else 
        {
            ptr = MPPE_Magic3;
        }
    }

    /* Calculating the hash using master key, session key, and pads. */
    status = SHA1_Init(&context);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, master_key, MPPE_MAX_KEY_LEN);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, MPPE_SHS_Pad1, MPPE_PAD_LENGTH);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, ptr, MPPE_MAGIC2_LENGTH);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Update(&context, MPPE_SHS_Pad2, MPPE_PAD_LENGTH);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    
    status = SHA1_Final(digest, &context);
    
    if (status != NU_SUCCESS)
    {
        return (status);
    }

    NU_BLOCK_COPY(session_key, digest, session_key_length);
    
    return (status);

} /* MPPE_Get_Asymmetric_Start_Key */

#endif
