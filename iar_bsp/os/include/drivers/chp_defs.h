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
*       chp_defs.h
*
*   COMPONENT
*
*       MSCHAP - Microsoft Challenge Handshake Authentication Protocol
*
*   DESCRIPTION
*
*       This file contains the constant definitions to support the
*       challenge handshake authentication protocol, CHAP.C.
*
*   DATA STRUCTURES
*
*       CHAP_LAYER
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_INC_CHP_DEFS_H
#define PPP_INC_CHP_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

#define CHAP_CHALLENGE  1
#define CHAP_RESPONSE   2
#define CHAP_SUCCESS    3
#define CHAP_FAILURE    4

#define CHAP_VALUE_LENGTH_OFFSET    4
#define CHAP_VALUE_OFFSET           5
#define CHAP_MD5_VALUE_SIZE         16
#define CHAP_CHALLENGE_VALUE_SIZE   4
#define CHAP_MAX_VALUE_SIZE         256


#if((PPP_USE_CHAP_MS1 == NU_TRUE) || (PPP_USE_CHAP_MS2 == NU_TRUE))
#define CHAP_MS1_CHALLENGE_VALUE_SIZE   8
#define CHAP_MS2_CHALLENGE_VALUE_SIZE   16
#define CHAP_MS_VALUE_SIZE              49
#define CHAP_MS_RESPONSE_SIZE           24
#define CHAP_MS2_MESSAGE_OFFSET         4
#define CHAP_MS2_AUTH_RESPONSE_MSG_SIZE 42
#define CHAP_MS_PASSWD_HASH_SIZE        16
#endif

#define AUTH_REQUIRED   1

/* Define the structure that will hold information used during
   CHAP authentication. */
typedef struct _chap_layer
{
    /* This pointer is for this structure extension. For instance, it is
    used to store the response from the client when L2TP is being used */
    CHAR    *chap_ext;

    UINT32  challenge_value;

#if(PPP_USE_CHAP_MS2 == NU_TRUE)
    /* To store the challenge of the peer. Used only for MSCHAP v2 */
    UINT8   challenge_value_ms_peer[CHAP_MS2_CHALLENGE_VALUE_SIZE];
#endif

#if((PPP_USE_CHAP_MS1 == NU_TRUE) || (PPP_USE_CHAP_MS2 == NU_TRUE))
    /* To store the challenge of the authenticator. */
    UINT8  challenge_value_ms_authenticator[CHAP_MS2_CHALLENGE_VALUE_SIZE];
#endif

#if(PPP_ENABLE_MPPE == NU_TRUE)
    /* To store the challenge response to be used later by MPPE */
    UINT8 challenge_response[CHAP_MS_RESPONSE_SIZE];
#endif

    UINT8   challenge_identifier;

    /* correct alignment for 32 bits CPU */
    UINT8   chap_pad[3];

} CHAP_LAYER;

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */


#endif                                      /* PPP_INC_CHP_DEFS_H */
