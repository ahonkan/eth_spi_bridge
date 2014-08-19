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
*       ppp_chap.c
*
*   COMPONENT
*
*       MSCHAP - Microsoft Challenge Handshake Authentication Protocol
*
*   DESCRIPTION
*
*       This file contains the challenge handshake authentication
*       protocols, CHAP and MSCHAP (v1 & v2), which are used to log
*       into a PPP server and to authenticate a calling PPP client.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       CHAP_Interpret
*       CHAP_MD5_Encrypt
*       CHAP_MD5_Verify
*       CHAP_Send_Challenge
*       CHAP_Respond_To_Challenge
*       CHAP_Check_Response
*       CHAP_Send_Success
*       CHAP_Send_Failure
*
*   DEPENDENCIES
*
*       nu_ppp.h
*       md5.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"

#if(PPP_USE_CHAP == NU_TRUE)

/* Include SSL/Crypto for MD5 hashing */
#include "openssl/md5.h"

/* Import all external variables */
#if (PPP_ENABLE_UM_DATABASE == NU_FALSE)
extern PPP_USER           _passwordlist[];
#endif

#if CHAP_DEBUG_PRINT_OK
#define PrintInfo(s)            PPP_Printf(s)
#define PrintErr(s)             PPP_Printf(s)
#else
#define PrintInfo(s)
#define PrintErr(s)
#endif


/*************************************************************************
* FUNCTION
*
*       CHAP_Interpret
*
* DESCRIPTION
*
*       This function processes the incoming CHAP packet. This involves
*       responding to an authentication request and informing the upper
*       layer if CHAP has succeed or failed.
*
* INPUTS
*
*       NET_BUFFER              *buf_ptr    Pointer to the incoming CHAP
*                                           packet
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID CHAP_Interpret(NET_BUFFER *buf_ptr)
{
    STATUS                  status;
    LCP_LAYER               *lcp;
    AUTHENTICATION_LAYER    *auth;
    LINK_LAYER              *link_layer;

    PrintInfo("CHAP_Interpret\n");

    /* Get a pointer to the link structures. */
    link_layer = (LINK_LAYER*)buf_ptr->mem_buf_device->dev_link_layer;
    lcp = &link_layer->lcp;

    /* We must be in the opened state in order to authenticate */

    if (lcp->state == OPENED)
    {
        /* Get a pointer to the authentication structure. */
        auth = &link_layer->authentication;

        switch (*buf_ptr->data_ptr)
        {
        case CHAP_CHALLENGE :
            PrintInfo("challenge\n");

            /* We will only except a challenge if we are a client */

            if (link_layer->mode == PPP_CLIENT)
            {
                CHAP_Respond_To_Challenge(buf_ptr);
            }

            break;

        case CHAP_RESPONSE :

            PrintInfo("response\n");

            /* Clear the retransmit TQ event. */
            TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT,
                          (UNSIGNED)buf_ptr->mem_buf_device,
                          CHAP_SEND_CHALLENGE);

            /* We only accept a response if we are in server mode and
               if the IDs of the challenge and response match */
            if ((link_layer->mode == PPP_SERVER) &&
                (auth->chap.challenge_identifier == buf_ptr->data_ptr[LCP_ID_OFFSET]))
            {
                /* We must check the response to see if it is valid. */
                status = CHAP_Check_Response(buf_ptr);

                /* See if the client was successful at logging in. */
                if (status == NU_TRUE)
                {
                    auth->state = AUTHENTICATED;

                    /* Tell the client that it passed logging in. */
                    CHAP_Send_Success(buf_ptr);

                    EQ_Put_Event(PPP_Event, (UNSIGNED)buf_ptr->mem_buf_device,
                                 AUTHENTICATED);
                }
                else
                {
                    if(link_layer->hwi.itype & PPP_ITYPE_L2TP_LAC)
                    {
                        EQ_Put_Event(PPP_Event, (UNSIGNED)buf_ptr->mem_buf_device,
                                     AUTHENTICATED);
                        return;
                    }


                    /* The client failed the authentication. Let it know. */
                    CHAP_Send_Failure(buf_ptr);
                    NU_Set_Events(&link_layer->negotiation_progression,
                                  PPP_AUTH_FAIL, NU_OR);
                }
            }

            break;

        case CHAP_SUCCESS :

            PrintInfo("success\n");

#if(PPP_USE_CHAP_MS2 == NU_TRUE)
            /* if it is mschap v2 packet then verify the authenticator's response */
            if(lcp->options.remote.chap_protocol == LCP_CHAP_MS2)
            {
                if(CHAPM_2_Check_Success(buf_ptr) == NU_FALSE)
                {
                   /* We have received a negative response from the server,
                      let the upper layer know. */
                   NU_Set_Events(&link_layer->negotiation_progression,
                       PPP_AUTH_FAIL, NU_OR);
                   break;

                }
            }
#endif

            auth->state = AUTHENTICATED;

            EQ_Put_Event(PPP_Event, (UNSIGNED)buf_ptr->mem_buf_device,
                         AUTHENTICATED);

            break;

        case CHAP_FAILURE :

            PrintInfo("failure\n");

            /* We have received a negative response from the server,
               let the upper layer know. */
            NU_Set_Events(&link_layer->negotiation_progression, PPP_AUTH_FAIL,
                          NU_OR);

            break;

        } /* switch */

    } /* if */

    /* Release the buffer space */
    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

} /* CHAP_Interpret */


/*************************************************************************
* FUNCTION
*
*       CHAP_MD5_Encrypt
*
* DESCRIPTION
*
*       This function encrypts a string using MD5 encryption.
*
* INPUTS
*
*       CHAR            string          Pointer to the string to encrypt
*       CHAR            digest          Pointer to the area in which to
*                                       return the encrypted string
*       UINT32          len             Length of the string to encrypt
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID CHAP_MD5_Encrypt(UINT8 *string, UINT8 *digest, UINT32 len)
{
    /* Perform MD5 */
    MD5(string, len, digest);
} /* CHAP_MD5_Encrypt */

/*************************************************************************
* FUNCTION
*
*       CHAP_MD5_Verify
*
* DESCRIPTION
*
*       This function verifies that a given string matches the
*       expected MD5 encrypted string.
*
* INPUTS
*
*       CHAP_LAYER  *chap               Pointer to CHAP_LAYER structure.
*       UINT8       *string             Pointer to the string to encrypt.
*       UINT8       *encrypted          Pointer to the encrypted string
*                                       to match.
*
* OUTPUTS
*
*       NU_TRUE                         If they match.
*       NU_FALSE                        If they don't match.
*
*************************************************************************/
STATUS CHAP_MD5_Verify(CHAP_LAYER *chap, UINT8 *string, UINT8 HUGE *encrypted)
{
    UINT32      len = 0;
    UINT8       packet[CHAP_MD5_VALUE_SIZE];
    UINT8       secret[CHAP_MD5_VALUE_SIZE];

    /* Add the challenge identifier. */
    packet[(INT)len++] = chap->challenge_identifier;

    /* Add the secret to encrypt. */
    strcpy((CHAR*)&packet[(INT)len], (CHAR*)string);

    /* Update the length. */
    len += strlen((CHAR*)string);

    /* Lastly put in the challenge value. */
    PUT32(packet, (INT)len, chap->challenge_value);

    /* Bump the length to account for the challenge value. */
    len += CHAP_CHALLENGE_VALUE_SIZE;

    /* Compute the expected encrypted password */
    CHAP_MD5_Encrypt(packet, secret, len);

    if ( (len <= CHAP_MD5_VALUE_SIZE) &&
	     (memcmp(secret, encrypted, (INT)len) == 0) )
        return NU_TRUE;
    else
        return NU_FALSE;

} /* CHAP_MD5_Verify */


/*************************************************************************
* FUNCTION
*
*       CHAP_Send_Challenge
*
* DESCRIPTION
*
*       Sends a challenge request to the client that is trying to log into
*       the system.
*
* INPUTS
*
*       *dev_ptr              Pointer to the device to send the
*                             challenge to
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID CHAP_Send_Challenge(DV_DEVICE_ENTRY *dev_ptr)
{
    AUTHENTICATION_LAYER    *auth;
    NET_BUFFER              *buf_ptr;
    LCP_FRAME               *out_frame;
    UINT32                  len = 0;
    LCP_LAYER               *lcp;

    PrintInfo("CHAP_Send_Challenge\n");

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. */
    buf_ptr = LCP_New_Buffer(dev_ptr, CHAP_CHALLENGE, -1);
    if (buf_ptr == NU_NULL)
        /* This error is logged within LCP_New_Buffer, so just return. */
        return;

    /* Map an LCP data access frame to the outgoing packet. */
    out_frame = (LCP_FRAME*)buf_ptr->data_ptr;

    /* Get a pointer to the authentication structure. */
    auth = &((LINK_LAYER *)dev_ptr->dev_link_layer)->authentication;

    lcp = &(((LINK_LAYER*)dev_ptr->dev_link_layer)->lcp);

    if(lcp->options.local.chap_protocol == LCP_CHAP_MD5)
    {
        /* Put in the value size. Ours will be 4 bytes */
        out_frame->data[(INT)len++] = CHAP_CHALLENGE_VALUE_SIZE;

        /* Get a random value for the challenge value. */
        auth->chap.challenge_value = LCP_Random_Number32();

        /* Put the challenge value in the packet */
        PUT32(out_frame->data, (INT)len, auth->chap.challenge_value);

        /* Bump the length to account for the challenge value. */
        len += CHAP_CHALLENGE_VALUE_SIZE;
    }

#if(PPP_USE_CHAP_MS1 == NU_TRUE)
    else if(lcp->options.local.chap_protocol == LCP_CHAP_MS1)
    {
        /* Put in the value size. Ours will be 8 bytes */
        out_frame->data[(INT)len++] = CHAP_MS1_CHALLENGE_VALUE_SIZE;

        /* Get a random value for the challenge value (8 bytes). */
        CHAPM_Random_Number(auth->chap.challenge_value_ms_authenticator,
            CHAP_MS1_CHALLENGE_VALUE_SIZE);

        /* Put the challenge value in the packet */
        memcpy(&out_frame->data[(INT)len],
            auth->chap.challenge_value_ms_authenticator,
            CHAP_MS1_CHALLENGE_VALUE_SIZE);

        /* Bump the length to account for the challenge value. */
        len += CHAP_MS1_CHALLENGE_VALUE_SIZE;
    }
#endif
#if(PPP_USE_CHAP_MS2 == NU_TRUE)
    else if(lcp->options.local.chap_protocol == LCP_CHAP_MS2)
    {
        /* Put in the value size. Ours will be 16 bytes */
        out_frame->data[(INT)len++] = CHAP_MS2_CHALLENGE_VALUE_SIZE;

        /* Get a random value for the challenge value. */
        CHAPM_Random_Number(auth->chap.challenge_value_ms_authenticator,
            CHAP_MS2_CHALLENGE_VALUE_SIZE);

        /* Put the challenge value in the packet */
        memcpy(&out_frame->data[(INT)len],
            auth->chap.challenge_value_ms_authenticator,
            CHAP_MS2_CHALLENGE_VALUE_SIZE);

        /* Bump the length to account for the challenge value. */
        len += CHAP_MS2_CHALLENGE_VALUE_SIZE;
    }
#endif

    /* Add the system name */
    if ((NU_Get_Host_Name((CHAR*)&out_frame->data[(INT)len],
                          MAX_HOST_NAME_LENGTH)) == NU_SUCCESS)
    {
        len += strlen((CHAR*)&out_frame->data[(INT)len]);
    }
    else
    {
        /* The name was unavailable so just add something. */
        out_frame->data[(INT)len++] = 'A';
        out_frame->data[(INT)len++] = 'T';
        out_frame->data[(INT)len++] = 'I';
    }

    /* Save the identifier. */
    auth->chap.challenge_identifier = out_frame->id;

    /* Send the packet. */
    out_frame->len = (UINT16)(out_frame->len + len);
    LCP_Send(dev_ptr, buf_ptr, PPP_CHAP_PROTOCOL);

    /* Set up a timeout event for PAP. */
    TQ_Timerset(PPP_Event, (UNSIGNED)dev_ptr, LCP_TIMEOUT_VALUE,
                CHAP_SEND_CHALLENGE);

} /* CHAP_Send_Challenge */

/*************************************************************************
* FUNCTION
*
*       CHAP_Respond_To_Challenge
*
* DESCRIPTION
*
*       This function responds to a challenge received from the server that
*       is authenticating.
*
* INPUTS
*
*       *in_buf_ptr             Pointer to the incoming CHAP
*                               packet
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID CHAP_Respond_To_Challenge(NET_BUFFER *in_buf_ptr)
{
    AUTHENTICATION_LAYER    *auth;
    NET_BUFFER              *buf_ptr;
    LCP_FRAME               *in_frame, *out_frame;
    UINT8   HUGE            *secret;
    UINT8                   pw_id_len, value_len;
    UINT32                  len = 0;
    UINT8   HUGE            *chap_pkt = in_buf_ptr->data_ptr;
    UINT16                  index;
    LCP_LAYER               *lcp;

    PrintInfo("CHAP_Respond_To_Challenge\n");

    /* Get a pointer to the LCP structure. */
    lcp = &((LINK_LAYER*)in_buf_ptr->mem_buf_device->dev_link_layer)->lcp;

    /* Get a pointer to the authentication structure. */
    auth = &(((LINK_LAYER *)in_buf_ptr->mem_buf_device->dev_link_layer)->authentication);

    /* Get a pointer to the data part of the CHAP packet. */
    in_frame = (LCP_FRAME*)in_buf_ptr->data_ptr;

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. */
    buf_ptr = LCP_New_Buffer(in_buf_ptr->mem_buf_device, CHAP_RESPONSE,
                             in_frame->id);

    if (buf_ptr == NU_NULL)
        /* This error is logged within LCP_New_Buffer, so just return. */
        return;

    /* Get a pointer to the data part of the response packet. */
    out_frame = (LCP_FRAME*)buf_ptr->data_ptr;


    /* Check whether this device needs to proxy authentication in L2TP LNS case. */
    if(((LINK_LAYER*)in_buf_ptr->mem_buf_device->dev_link_layer)->hwi.itype &
       PPP_ITYPE_L2TP_LNS)
    {
        out_frame->id = auth->auth_identifier;

        /* Put in the value size.*/
        out_frame->data[(INT)len++] = *((UINT8 *)(auth->chap.chap_ext));

        /* Copy the response. */
        memcpy(&out_frame->data[(INT)len],
               &((CHAR *)(auth->chap.chap_ext))[1],
               out_frame->data[(INT)len-1]);

        /* Update the length */
        len += out_frame->data[(INT)len-1];
    }


    else if(lcp->options.remote.chap_protocol == LCP_CHAP_MD5)
    {
        /* Allocate memory for the secret. This can be as long as
           256 bytes + PPP_MAX_PW_LENGTH */
        if ( (NU_Allocate_Memory(PPP_Memory, (VOID **)&secret,
                                (CHAP_MAX_VALUE_SIZE + PPP_MAX_PW_LENGTH),
                                 NU_NO_SUSPEND)) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to allocate memory.", NERR_RECOVERABLE,
                           __FILE__, __LINE__);

            /* Get out */
            return;
        }

        index = 0;
        len   = 0;

        /* Put in the value size. For MD5 it is 16 */
        out_frame->data[(INT)len++] = CHAP_MD5_VALUE_SIZE;

        /* Build the string that we will encrypt, this consists of
           the PW, the ID, and a value supplied by the
           authenticator. */
        secret[index++] = out_frame->id;

        /* Get the length of the PW to try */
        pw_id_len = (UINT8) strlen (auth->login_pw);

        /* Copy the PW over. */
        memcpy(&secret[index], auth->login_pw, pw_id_len);
        index = index + pw_id_len;

        /* Get the length of the value. */
        value_len = chap_pkt [CHAP_VALUE_LENGTH_OFFSET];

        /* Copy it over. */
        memcpy(&secret[index], &chap_pkt[CHAP_VALUE_OFFSET], value_len);
        index = index + value_len;

        /* Encrypt it. */
        CHAP_MD5_Encrypt((UINT8 *)secret, &out_frame->data[(INT)len], index);

        /* Update the length */
        len += CHAP_MD5_VALUE_SIZE;

        /* Done with the memory for the secret so give it back. */
        if ( (NU_Deallocate_Memory (secret)) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory.", NERR_RECOVERABLE,
                           __FILE__, __LINE__);

    }
#if(PPP_USE_CHAP_MS1 == NU_TRUE)
    else if(lcp->options.remote.chap_protocol == LCP_CHAP_MS1)
    {

        /* Put in the value size. */
        out_frame->data[(INT)len++] = CHAP_MS_VALUE_SIZE;

        /* Zero out an obsolete field */
        UTL_Zero( &out_frame->data[(INT)len], 24);
        len += 24;

        /* Get the length of the authenticator value */
        value_len = chap_pkt [CHAP_VALUE_LENGTH_OFFSET];

        /* get the received challenge. */
        memcpy(auth->chap.challenge_value_ms_authenticator,
               &chap_pkt[CHAP_VALUE_OFFSET], value_len);

        /* Encrypt it. */
        CHAPM_1_Chal_Resp(auth->chap.challenge_value_ms_authenticator,
            (INT8 *)auth->login_pw, &out_frame->data[(INT)len]);

        /* Update the length (24) */
        len += CHAP_MS_RESPONSE_SIZE;

        /* flag is always 1 */
        out_frame->data[(INT)len++] = 1;

    }
#endif
#if(PPP_USE_CHAP_MS2 == NU_TRUE)
    else if(lcp->options.remote.chap_protocol == LCP_CHAP_MS2)
    {
        /* Get the length of the authenticator value */
        value_len = chap_pkt [CHAP_VALUE_LENGTH_OFFSET];

        /* get the received challenge. */
        memcpy(auth->chap.challenge_value_ms_authenticator,
               &chap_pkt[CHAP_VALUE_OFFSET], value_len);

        /* Put in the value size. It will be 49 bytes */
        out_frame->data[(INT)len++] = CHAP_MS_VALUE_SIZE;

        /* Get a random value for the challenge value. 16 octets*/
        CHAPM_Random_Number(auth->chap.challenge_value_ms_peer,
            CHAP_MS2_CHALLENGE_VALUE_SIZE);

        /* Put the challenge value in the packet */
        memcpy(&out_frame->data[(INT)len],
            auth->chap.challenge_value_ms_peer,
            CHAP_MS2_CHALLENGE_VALUE_SIZE);

        len += CHAP_MS2_CHALLENGE_VALUE_SIZE;

        /*zero-fill an obsolete field */
        UTL_Zero(&out_frame->data[(INT)len], 8);

        len += 8;

        /* generate response */
        CHAPM_2_Generate_Peer_Response(auth->chap.challenge_value_ms_authenticator,
                                       auth->chap.challenge_value_ms_peer,
                                       (INT8 *)auth->login_name,
                                       (INT8 *)auth->login_pw,
                                       &out_frame->data[(INT)len]);

#if (PPP_ENABLE_MPPE == NU_TRUE)
        memcpy(auth->chap.challenge_response, &out_frame->data[(INT)len], CHAP_MS_RESPONSE_SIZE);
#endif

        len += CHAP_MS_RESPONSE_SIZE;

        /*flag is always 0*/
        out_frame->data[(INT)len++] = 0;
    }
#endif

    /* Copy the login name into the packet. */
    strcpy((CHAR*)&out_frame->data[(INT)len], auth->login_name);
    len += strlen(auth->login_name);

    /* Send the packet. */
    out_frame->len = (UINT16)(out_frame->len + len);

    /* If this device needs to proxy authentication in L2TP LNS case. */
    if(((LINK_LAYER*)in_buf_ptr->mem_buf_device->dev_link_layer)->hwi.itype &
       PPP_ITYPE_L2TP_LNS)
    {
        /* Put the length of the frame. */
        PUT16(&out_frame->len, 0, out_frame->len);
        buf_ptr->mem_buf_device = in_buf_ptr->mem_buf_device;

        /* Do proxy authentication.*/
        CHAP_Interpret(buf_ptr);
    }
    else
        LCP_Send(in_buf_ptr->mem_buf_device, buf_ptr, PPP_CHAP_PROTOCOL);

} /* CHAP_Respond_To_Challenge */

/*************************************************************************
* FUNCTION
*
*       CHAP_Check_Response
*
* DESCRIPTION
*
*       This function verifies the response from the peer.
*
* INPUTS
*
*       NET_BUFFER              *buf_ptr    Pointer to the incoming CHAP
*                                           packet
*
* OUTPUTS
*
*       STATUS                              Was the response correct
*
*************************************************************************/
STATUS CHAP_Check_Response(NET_BUFFER *buf_ptr)
{
    LINK_LAYER              *link;
    AUTHENTICATION_LAYER    *auth;
    UINT16                  name_length;
    CHAR                    client_name[PPP_MAX_ID_LENGTH];
    CHAR    HUGE            *dbname, *dbpw;
    UINT8                   x, found_it = NU_FALSE;
    UINT8   HUGE            *chap_pkt = buf_ptr->data_ptr;
    STATUS                  status;
    LCP_LAYER               *lcp;

#if (PPP_ENABLE_UM_DATABASE == NU_TRUE)
    UM_USER                 user;
#endif

    PrintInfo("CHAP_Check_Response\n");

    /* Get a pointer to the LCP structure. */
    lcp = &((LINK_LAYER*)buf_ptr->mem_buf_device->dev_link_layer)->lcp;

    /* The size of the value must be correct for MD5 encryption or for MSCHAPs */
    if (
           ((CHAP_MD5_VALUE_SIZE == chap_pkt[CHAP_VALUE_LENGTH_OFFSET])
           && (lcp->options.local.chap_protocol == LCP_CHAP_MD5))
#if((PPP_USE_CHAP_MS1 == NU_TRUE) || (PPP_USE_CHAP_MS2 == NU_TRUE))
        || (CHAP_MS_VALUE_SIZE == chap_pkt[CHAP_VALUE_LENGTH_OFFSET])
#endif
        )
    {
        /* Get a pointer to the authentication structure. */
        link = (LINK_LAYER*)buf_ptr->mem_buf_device->dev_link_layer;
        auth = &link->authentication;

        /* store the response value, if the device is currently being used
        with L2TP */
        if(link->hwi.itype & PPP_ITYPE_L2TP_LAC)
        {
            /* store the entire response value */
            NU_BLOCK_COPY(auth->chap.chap_ext, &chap_pkt[CHAP_VALUE_OFFSET],
                chap_pkt[CHAP_VALUE_LENGTH_OFFSET]);
        }

        /* Copy out the name of the client. To get its length we must get
           the total length of the packet and compute what is left. */
        name_length = (UINT16) ( ( *(chap_pkt + LCP_LENGTH_OFFSET) ) << 8 );
        name_length =  (UINT16)(name_length | *(chap_pkt + LCP_LENGTH_OFFSET + 1));

        /* Now take off the header, value size, and value fields. Note
           that the value size field is only 1 byte, that is the 1 that
           is added below. */
        if(lcp->options.local.chap_protocol == LCP_CHAP_MD5)
            name_length -= (UINT16)(LCP_HEADER_LEN + CHAP_MD5_VALUE_SIZE + 1);

#if(PPP_USE_CHAP_MS1 == NU_TRUE || PPP_USE_CHAP_MS2 == NU_TRUE)
        else
            name_length -= (UINT16)(LCP_HEADER_LEN + CHAP_MS_VALUE_SIZE + 1);
#endif

        /* Copy out the name */
        for (x = 0; ((UINT16)x < name_length) && (x < PPP_MAX_ID_LENGTH - 1); x++)
        {
          if(lcp->options.local.chap_protocol == LCP_CHAP_MD5)
            client_name [x] = chap_pkt [CHAP_VALUE_OFFSET + CHAP_MD5_VALUE_SIZE + x];
#if(PPP_USE_CHAP_MS1 == NU_TRUE || PPP_USE_CHAP_MS2 == NU_TRUE)
          else
            client_name [x] = chap_pkt [CHAP_VALUE_OFFSET + CHAP_MS_VALUE_SIZE + x];
#endif
        }
        /* Null it */
        client_name [x] = 0;

        /* If this device is an L2TP device (i.e. Proxy authentication) then
        just store the name and return. */
        if(link->hwi.itype & PPP_ITYPE_L2TP_LAC)
        {
            /* Copy incoming user name. */
            strcpy(auth->login_name,  client_name);
            return NU_NOT_FOUND;
        }


#if (PPP_ENABLE_UM_DATABASE == NU_TRUE)
        status = UM_Find_User_First(&user, UM_PPP);
        dbname = user.um_name;
        dbpw = user.um_pw;
#else
        x = 0;
        dbname = _passwordlist[x].id;
        dbpw = _passwordlist[x].pw;
        if (dbname[0] == '\0')
            status = NU_NOT_FOUND;
        else
            status = NU_SUCCESS;
#endif
        while (status == NU_SUCCESS && found_it == NU_FALSE)
        {
            /* If name matches, then user is in the database. */
            if (strcmp(client_name, (CHAR *)dbname) == 0)
            {
#if(PPP_ENABLE_MPPE == NU_TRUE)
                memcpy(auth->chap.challenge_response, (UINT8*)&chap_pkt[CHAP_VALUE_OFFSET + 24], CHAP_MS_RESPONSE_SIZE);
#endif
               if(lcp->options.local.chap_protocol == LCP_CHAP_MD5)
               {
                    if (CHAP_MD5_Verify(&auth->chap, (UINT8 *)dbpw,
                                        &chap_pkt[CHAP_VALUE_OFFSET]) == NU_TRUE)
                    {
                        /* store the user name and password in the authentication structure */
                        strcpy(auth->login_name,  (CHAR *)dbname);
                        strcpy(auth->login_pw, dbpw);

                        found_it = NU_TRUE;
                        break;
                    }
               }
#if(PPP_USE_CHAP_MS1 == NU_TRUE)
               else if(lcp->options.local.chap_protocol == LCP_CHAP_MS1)
               {
                    /* Verify the challenge response, first 24 octets of
                    the value are irrelevant */
                    if(CHAPM_1_Chal_Resp_Verify(auth->chap.challenge_value_ms_authenticator,
                        (INT8 *)dbpw, (UINT8 *)&chap_pkt[CHAP_VALUE_OFFSET + 24]) == NU_TRUE)
                    {
                       /* store the user name and password in the
                        authentication structure */
                        strcpy(auth->login_name,  (CHAR *)dbname);
                        strcpy(auth->login_pw, dbpw);

                        found_it = NU_TRUE;
                        break;
                    }
               }
#endif
#if(PPP_USE_CHAP_MS2 == NU_TRUE)
               else if(lcp->options.local.chap_protocol == LCP_CHAP_MS2)
               {
                    /* verify the challenge response */
                    if(CHAPM_2_Chal_Resp_Verify((INT8 *)dbpw,
                        (INT8 *)client_name,
                        (UINT8 *)&chap_pkt[CHAP_VALUE_OFFSET],
                        auth->chap.challenge_value_ms_authenticator,
                        (UINT8 *)&chap_pkt[CHAP_VALUE_OFFSET + 24]) == NU_TRUE)
                    {
                        /* store the user name and password in the
                        authentication structure */
                        strcpy(auth->login_name,  (CHAR *)dbname);
                        strcpy(auth->login_pw, dbpw);

                        found_it = NU_TRUE;
                        break;
                    }
               }
#endif
            }

            /* Get next user. */
#if (PPP_ENABLE_UM_DATABASE == NU_TRUE)
            status = UM_Find_User_Next((CHAR *)dbname, &user, UM_PPP);
            dbname = user.um_name;
            dbpw = user.um_pw;
#else
            x++;
            dbname = _passwordlist[x].id;
            dbpw = _passwordlist[x].pw;
            if (dbname[0] == '\0')
                status = NU_NOT_FOUND;
#endif
        }
    }

    return found_it;

} /* CHAP_Check_Response */

/*************************************************************************
*
*   FUNCTION
*
*       CHAP_Send_Success
*
*   DESCRIPTION
*
*       Send a 'success' packet to the challenged peer. This tells
*       the peer that it has been authenticated.
*
*   INPUTS
*
*       *in_buf_ptr            Pointer to the incoming CHAP
*                              packet
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CHAP_Send_Success(NET_BUFFER *in_buf_ptr)
{
    NET_BUFFER           *buf_ptr;
    LCP_FRAME            *lcp_frame;

#if(PPP_USE_CHAP_MS2 == NU_TRUE)
    LCP_FRAME            *out_frame;
    AUTHENTICATION_LAYER *auth;
    UINT8   HUGE         *chap_pkt = in_buf_ptr->data_ptr;
    LCP_LAYER            *lcp = &((LINK_LAYER*)in_buf_ptr->mem_buf_device->dev_link_layer)->lcp;
#endif

    PrintInfo("CHAP_Send_Success\n");

    /* Get a pointer to the data part of the CHAP packet. */
    lcp_frame = (LCP_FRAME*)in_buf_ptr->data_ptr;

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. */
    buf_ptr = LCP_New_Buffer(in_buf_ptr->mem_buf_device, CHAP_SUCCESS, lcp_frame->id);

    if (buf_ptr == NU_NULL)
        /* This error is logged within LCP_New_Buffer, so just return. */
        return;

#if(PPP_USE_CHAP_MS2 == NU_TRUE)
    if(lcp->options.local.chap_protocol == LCP_CHAP_MS2)
    {
          /* Get a pointer to the authentication structure. */
          auth = &(((LINK_LAYER *)in_buf_ptr->mem_buf_device->dev_link_layer)->authentication);

          /* Get a pointer to the data part of the response packet. */
          out_frame = (LCP_FRAME*)buf_ptr->data_ptr;

          /* Generate the authenticator response.
            42 octet authentication response plus printable message */
          CHAPM_2_Gen_Auth_Resp((INT8 *)auth->login_pw, (UINT8 *)&chap_pkt[CHAP_VALUE_OFFSET + 24],
              (UINT8 *)&chap_pkt[CHAP_VALUE_OFFSET], auth->chap.challenge_value_ms_authenticator,
              (INT8 *)auth->login_name, out_frame->data);

          /* Send anything as a message */
          memcpy(&out_frame->data[CHAP_MS2_AUTH_RESPONSE_MSG_SIZE],
                 " M=Authenticated", 16);

          /* Update the length; 42 octet message + the message length */
          out_frame->len += 58;
    }
#endif

    /* Nothing else goes into the packet, so just send it. */
    LCP_Send(in_buf_ptr->mem_buf_device, buf_ptr, PPP_CHAP_PROTOCOL);

} /* CHAP_Send_Success */



/*************************************************************************
*
*   FUNCTION
*
*       CHAP_Send_Failure
*
*   DESCRIPTION
*
*       Send a 'failure' packet to the challenged peer. This tells
*       the peer that it has failed to authenticate.
*
*   INPUTS
*
*       *in_buf_ptr            Pointer to the incoming CHAP
*                              packet
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CHAP_Send_Failure(NET_BUFFER *in_buf_ptr)
{
    NET_BUFFER  *buf_ptr;
    LCP_FRAME   *lcp_frame;
    INT         len = 0;

#if(PPP_USE_CHAP_MS1 == NU_TRUE || PPP_USE_CHAP_MS2 == NU_TRUE)
    LCP_LAYER   *lcp = &((LINK_LAYER*)in_buf_ptr->mem_buf_device->dev_link_layer)->lcp;
#endif

    PrintInfo("CHAP_Send_Failure\n");

    /* Get a pointer to the data part of the CHAP packet. */
    lcp_frame = (LCP_FRAME*)in_buf_ptr->data_ptr;

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. */
    buf_ptr = LCP_New_Buffer(in_buf_ptr->mem_buf_device, CHAP_FAILURE,
                             lcp_frame->id);

    if (buf_ptr == NU_NULL)
        /* This error is logged within LCP_New_Buffer, so just return. */
        return;

#if(PPP_USE_CHAP_MS1 == NU_TRUE || PPP_USE_CHAP_MS2 == NU_TRUE)
    if(lcp->options.local.chap_protocol != LCP_CHAP_MD5)
    {
        /* Authentication failed, No Retries */
        memcpy(lcp_frame->data, "E=691 R=0 C=0 V=3 M=Error", 25);

       /* Bump the length to account for the error message. */
        len += 25;
    }
#endif

    /* Send the packet. */
    lcp_frame->len = (UINT16)(lcp_frame->len + len);

    /* Nothing else goes into the packet, so just send it. */
    LCP_Send(in_buf_ptr->mem_buf_device, buf_ptr, PPP_CHAP_PROTOCOL);

} /* CHAP_Send_Failure */

#endif
