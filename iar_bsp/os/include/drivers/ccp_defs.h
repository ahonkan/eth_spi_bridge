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
*       ccp_defs.h
*
*   COMPONENT
*
*       CCP - Compression Control Protocol
*
*   DESCRIPTION
*
*       This file contains constant definitions and structure definitions
*       to support the file ccp.c
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
#ifndef PPP_INC_CCP_DEFS_H
#define PPP_INC_CCP_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

/* This macro defines the timeout used when waiting for CCP to complete
   the close sequence and for the modem to hangup. */
#define CCP_LINK_CLOSE_TIMEOUT          (60 * SCK_Ticks_Per_Second)

#define CCP_MAX_DATA_SIZE               100
#define CCP_CLEAR_HIGH                  0x0000ffff

/* define basic packet lengths */
#define CCP_HEADER_LEN                  4
#define CCP_OPTION_HDRLEN               2
#define CCP_MPPC_MPPE_LENGTH            6

/* Define offsets into the CCP packet. */
#define CCP_ID_OFFSET                   1
#define CCP_LENGTH_OFFSET               2

/* CCP Packet type codes. */
#define CCP_CONFIGURE_REQUEST           1
#define CCP_CONFIGURE_ACK               2
#define CCP_CONFIGURE_NAK               3
#define CCP_CONFIGURE_REJECT            4
#define CCP_TERMINATE_REQUEST           5
#define CCP_TERMINATE_ACK               6
#define CCP_CODE_REJECT                 7
#define CCP_RESET_REQUEST               14
#define CCP_RESET_ACK                   15

/* CCP Option type codes. */
#define CCP_OPTION_OUI                  0
#define CCP_OPTION_PT1                  1
#define CCP_OPTION_PT2                  2
#define CCP_OPTION_PJ                   3
#define CCP_OPTION_HPPPC                16
#define CCP_OPTION_SACE_LZS             17
#define CCP_OPTION_MPPC_MPPE            18
#define CCP_OPTION_GANDALF_FZA          19
#define CCP_OPTION_V42_BIS              20
#define CCP_OPTION_BSD_LZW              21
#define CCP_OPTION_LZSDCP               23
#define CCP_OPTION_MVRCAMAGNALINK       24
#define CCP_OPTION_DEFLATE              26
#define CCP_OPTION_RESERVED             255

/* MPPE MPPC Option flags. */
#define CCP_FLAG_MPPC                   0x0001
#define CCP_FLAG_A_BIT                  0x0008
#define CCP_FLAG_D_BIT                  0x0001
#define CCP_FLAG_L_BIT                  0x0020
#define CCP_FLAG_O_BIT                  0x0010
#define CCP_FLAG_M_BIT                  0x0080
#define CCP_FLAG_S_BIT                  0x0040
#define CCP_FLAG_H_BIT                  (0x0100 << 16)

#define CCP_FLAGS_SHIFT                 12
#define CCP_FLAGS_MASK                  0xF000
#define CCP_CCOUNT_MASK                 0x0FFF
#define CCP_MAX_CCOUNT                  0x0FFF
#define CCP_CCOUNT_LOWER_OCTET          0xFF

#define CCP_MICROSOFT_SPECIFIC          0x12
#define CCP_ENCRYPTED_PACKET_FLAG       0x1000
#define CCP_FLUSHED_BIT                 0x8000
#define CCP_ENCRYPTED_PACKET_HDR        0x00FD

#define CCP_GEN_ERROR                   -1

/* CCP Configuration values. */
#define CCP_TIMEOUT_VALUE               LCP_TIMEOUT_VALUE
#define CCP_MAX_CONFIGURE               LCP_MAX_CONFIGURE
#define CCP_MAX_TERMINATE               LCP_MAX_TERMINATE

/* Encryption Mode. */
#define CCP_USE_STATEFUL_ONLY           0
#define CCP_USE_STATELESS_ONLY          1
#define CCP_USE_BOTH_ENCRYPTIONS        2
#define CCP_ENCRYPTION_MODE             CCP_USE_BOTH_ENCRYPTIONS

#define CCP_RAND_MAX                    LCP_RAND_MAX
#define CCP_RAND_MAX32                  LCP_RAND_MAX32

#define CCP_Random_Number               LCP_Random_Number
#define CCP_Random_Number32             LCP_Random_Number32
#define CCP_New_Buffer                  LCP_New_Buffer
#define CCP_Append_Option               LCP_Append_Option
#define CCP_Reject_Option               LCP_Reject_Option
#define CCP_Nak_Option                  LCP_Nak_Option
#define CCP_Send                        LCP_Send

/* CCP options structure. */
typedef struct ccp_opts_struct
{
    UINT8       type;
    UINT8       pad[3];

#if (PPP_ENABLE_MPPE == NU_TRUE)

    MPPE_INFO   mppe;

#endif

} CCP_OPTIONS;

typedef struct ccp_layer
{
    UINT8       state;
    UINT8       identifier;
    INT8        num_transmissions;
    UINT8       pad[1];

    struct
    {
        CCP_OPTIONS local;
        CCP_OPTIONS remote;
    } options;

} CCP_LAYER;

typedef LCP_FRAME CCP_FRAME;
typedef LCP_OPTION CCP_OPTION;

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_CCP_DEFS_H */
