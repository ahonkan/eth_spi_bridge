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
*       ppp_l2tp.h
*
*   COMPONENT
*
*       L2TP - Layer Two Tunneling Protocol
*
*   DESCRIPTION
*
*       This file contains support required from PPP by Nucleus L2TP.
*
*   DATA STRUCTURES
*
*       PPP_L2TP_PROXY_GET
*       PPP_L2TP_PROXY_SAVE
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_L2TP_H
#define PPP_L2TP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

#define PPP_L2TP_CRC                0
#define PPP_L2TP_HARDWARE_OVRNS     1
#define PPP_L2TP_BUFFER_OVRNS       2
#define PPP_L2TP_TIMOUT_ERRORS      3
#define PPP_L2TP_ALIGNMENT_ERRORS   4
#define PPP_L2TP_RESULT_CODE        5
#define PPP_L2TP_FRAMING_CAP        6
#define PPP_L2TP_BEARER_CAP         7
#define PPP_L2TP_CAUSE_CODE         8
#define PPP_L2TP_CALL_SERIAL_NUM    9
#define PPP_L2TP_CALLED_NUM         10
#define PPP_L2TP_CALLING_NUM        11
#define PPP_L2TP_TX_SPEED           12
#define PPP_L2TP_RX_SPEED           13
#define PPP_L2TP_PROXY_DATA         14
#define PPP_L2TP_ACCM               15

typedef struct _ppp_l2tp_proxy_get
{
    UINT8     auth_name[PPP_MAX_ID_LENGTH];
    UINT8     init_rx_lcp[508];
    UINT8     last_tx_lcp[508];
    UINT8     last_rx_lcp[508];
    UINT8     auth_chal[16];
    UINT8     auth_resp[52];
    UINT8     auth_type[2];
    UINT8     auth_id[2];
    UINT16    last_tx_lcp_len;
    UINT16    init_rx_lcp_len;
    UINT16    last_rx_lcp_len;
    UINT8     auth_name_len;
    UINT8     auth_chal_len;
    UINT8     auth_resp_len;
    UINT8     pad[3];

} PPP_L2TP_PROXY_GET;

typedef struct _ppp_l2tp_proxy_save
{
    UINT8     *auth_name;
    UINT8     *init_rx_lcp;
    UINT8     *last_tx_lcp;
    UINT8     *last_rx_lcp;
    UINT8     *auth_chal;
    UINT8     *auth_resp;
    UINT16    auth_type;
    UINT16    auth_id;
    UINT16    last_tx_lcp_len;
    UINT16    init_rx_lcp_len;
    UINT16    last_rx_lcp_len;
    UINT8     auth_name_len;
    UINT8     auth_chal_len;
    UINT8     auth_resp_len;
    UINT8     pad[3];

} PPP_L2TP_PROXY_SAVE;

/* Function prototypes. */
STATUS PPP_L2TP_Get_Opt(UINT32 dev_index, UINT8 *data_ptr,
                        UINT16 *len, UINT8 option);
STATUS PPP_L2TP_Set_Opt(UINT32 dev_index, UINT8 *data_ptr,
                        UINT16 len, UINT8 option);
STATUS PPP_L2TP_Init(UINT32 dev_index, UINT32 l2tp_type);
STATUS PPP_L2TP_LNS_Do_Proxy(DV_DEVICE_ENTRY *dev_ptr);

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_L2TP_H */
