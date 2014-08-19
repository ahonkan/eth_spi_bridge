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
*       mppe_defs.h
*
*   COMPONENT
*
*       MPPE - Microsoft Point to Point Encryption Protocol
*
*   DESCRIPTION
*
*       This file contains constant definitions and structure definitions
*       to support the file mppe.c
*
*   DATA STRUCTURES
*
*       CCP_OPTIONS
*       CCP_LAYER
*       CCP_FRAME
*       CCP_OPTION
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_INC_MPPE_DEFS_H
#define PPP_INC_MPPE_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

#define MPPE_40_BIT                     5
#define MPPE_56_BIT                     7
#define MPPE_128_BIT                    16

#define MPPE_GEN_ERROR                  CCP_GEN_ERROR

/* Known Constants */
#define MPPE_SALT1                      0xd1
#define MPPE_SALT2                      0x26
#define MPPE_SALT3                      0x9E

/* Packet Encryption Range. */
#define MPPE_RANGE_LOWER                0x0021
#define MPPE_RANGE_HIGHER               0x00FA

/* Macros defining lengths used in MPPE. */
#define MPPE_MAX_PASSWORD_LEN           14
#define MPPE_SESSION_KEY_LEN            18
#define MPPE_MAX_KEY_LEN                16
#define MPPE_128BIT_KEY_LEN             16
#define MPPE_40BIT_KEY_LEN              8
#define MPPE_56BIT_KEY_LEN              8

/* Macros used in DES HASH. */
#define MPPE_STD_TEXT_LEN               8
#define MPPE_STD_TEXT                   "KGS!@#$%"

/* Some predefined PAD lengths. */
#define MPPE_PAD_LENGTH                 40
#define MPPE_MAGIC1_LENGTH              27
#define MPPE_MAGIC2_LENGTH              84
#define MPPE_MAGIC3_LENGTH              84
#define MPPE_DIGEST_LENGTH              20
#define MPPE_NT_RESPONSE_LENGTH         24

#define MPPE_CHANGE_SEND_REQUEST        0
#define MPPE_CHANGE_RECEIVE_REQUEST     1

/* Mapping SHS PAD on SHA PAD. */
#define MPPE_SHS_Pad1                   MPPE_SHA_Pad1
#define MPPE_SHS_Pad2                   MPPE_SHA_Pad2

#if (PPP_ENABLE_MPPE == NU_TRUE)

typedef struct mppe_struct
{
    /* Configuration switches to turn options on/off during negotiation. */
    UINT32      mppe_default_flags;

    /* Supported bits concluded. */
    UINT32      mppe_ccp_supported_bits;

    /* Send Session Key. */
    UINT8       mppe_send_session_key[MPPE_MAX_KEY_LEN];

    /* Receive Session Key. */
    UINT8       mppe_receive_session_key[MPPE_MAX_KEY_LEN];

    /* Initial Master Send Key. */
    UINT8       mppe_master_send_key[MPPE_MAX_KEY_LEN];

    /* Initial Master Receive Key. */
    UINT8       mppe_master_receive_key[MPPE_MAX_KEY_LEN];

    /* Coherency Count. */
    UINT16      mppe_coherency_count;

    /* Coherency Count for outgoing packets. */
    UINT16      mppe_send_coherency_count;

    /* Key Length. */
    UINT8       mppe_key_length;

    /* Flags. */
    UINT8       mppe_cntrl_bits;

    /* Reset flag. */
    UINT8       mppe_reset_flag;

    /* Stateless or Stateful mode flag. */
    UINT8       mppe_stateless;

    /* Require Encryption. */
    UINT8       mppe_require_encryption;

    /* Packet Encryption Flags. */
    UINT8       mppe_encrypt_packets;

    /* Reset Requested Flag. */
    UINT8       mppe_reset_requested;

    /* Padding. */
    UINT8       mppe_pad[1];

} MPPE_INFO;

#endif

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_CCP_DEFS_H */
