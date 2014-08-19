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
*       ppp_dc.h
*
*   COMPONENT
*
*       DC - Direct Cable Protocol
*
*   DESCRIPTION
*
*       This component is responsible for providing Direct Connect
*       Cable Protocol services.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_DC_H
#define PPP_DC_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

/* Defines for the DC server and the client case. */
#define PPP_DC_SERVER          0
#define PPP_DC_CLIENT          1

/* Number of attempts to make before giving up the request for
   connection to a PPP server. */
#define PPP_DC_NUM_ATTEMPTS     3

/* DC commands. */
#define PPP_DC_CLIENT_COMMAND   "CLIENTCLIENT"
#define PPP_DC_SERVER_COMMAND   "CLIENTSERVER"
#define PPP_DC_CLIENT_CMD       "CLIENT"
#define PPP_DC_SERVER_CMD       "SERVER"

/* Max size of the area to hold the incoming command. */
#define PPP_DC_CMD_MAX_SIZE     7

/* Function prototypes */
STATUS PPP_DC_Initialize(DV_DEVICE_ENTRY *dev_ptr);
STATUS PPP_DC_Hangup(LINK_LAYER *link_ptr, UINT8 wait_for_modem);
STATUS PPP_DC_Wait(DV_DEVICE_ENTRY *dev_ptr);
STATUS PPP_DC_Connect(CHAR *unused, DV_DEVICE_ENTRY *dev_ptr);
STATUS PPP_DC_Get_String(CHAR *response, DV_DEVICE_ENTRY *dev_ptr,
                         UINT8 mode);


#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_DC_H */
