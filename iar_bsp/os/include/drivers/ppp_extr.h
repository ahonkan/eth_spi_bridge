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
*       ppp_extr.h
*
*   COMPONENT
*
*       PPP - Core component of PPP
*
*   DESCRIPTION
*
*       This file contains function prototypes used by PPP
*       and also accessible to other modules.
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
#ifndef PPP_INC_PPP_EXTR_H
#define PPP_INC_PPP_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

VOID nu_os_drvr_ppp_init (const CHAR * key, INT startstop);
UINT32 PPP_Two_To_Power (UINT8 exponent);
VOID   PPP_Task_Entry(UNSIGNED argc, VOID *argv);
VOID   PPP_Event_Forwarding(TQ_EVENT event, UNSIGNED dat, UNSIGNED extdat);
STATUS PPP_Lower_Layer_Up(UINT8 *ip_address4, UINT8 *ip_address6,
                          DV_DEVICE_ENTRY *dev_ptr);
STATUS PPP_Set_Login(CHAR [], CHAR [], DV_DEVICE_ENTRY *);
STATUS NU_Set_PPP_Login(CHAR [], CHAR [],  CHAR *);
STATUS PPP_Safe_To_Hangup(DV_DEVICE_ENTRY *dev_ptr);
STATUS PPP_Still_Connected(CHAR *);
STATUS PPP_Hangup(UINT8, CHAR *);
STATUS PPP_Input(VOID);
STATUS PPP_Output(NET_BUFFER*, DV_DEVICE_ENTRY*, VOID*, VOID*);
STATUS PPP_Initialize(DV_DEVICE_ENTRY *dev_ptr);
STATUS PPP_Wait_For_Client(PPP_OPTIONS *ppp_options);
STATUS PPP_Dial(PPP_OPTIONS *ppp_options);
STATUS PPP_Set_Link_Options(CHAR*, NU_PPP_CFG*);
STATUS PPP_Set_Link_Option(CHAR *link_name, INT optname, VOID *optval, INT optlen);
VOID   PPP_Get_Default_Options(PPP_CFG * ppp_cfg);
STATUS PPP_Validate_Link_Options(NU_PPP_CFG*);
STATUS PPP_Get_Link_Options(CHAR *link_name, NU_PPP_CFG *ppp_cfg);
STATUS PPP_Get_Link_Option(CHAR *link_name, INT optname, VOID *optval, INT *optlen);
VOID   PPP_Event_Handler(TQ_EVENT, UNSIGNED, UNSIGNED);
STATUS PPP_Add_Header(DV_DEVICE_ENTRY *, NET_BUFFER *);
VOID   PPP_Reset(DV_DEVICE_ENTRY *dev_ptr);
STATUS PPP_Abort_Wait_For_Client(CHAR *link_name);
STATUS PPP_Abort(CHAR *link_name);
STATUS PPP_Abort_Connection(CHAR *link_name);
VOID   PPP_Stop_All_Timers(DV_DEVICE_ENTRY *dev_ptr);
INT32  PPP_Last_Activity_Time(CHAR *link_name);
STATUS PPP_Negotiation_Timeout(CHAR*, INT, UINT16*);
STATUS PPP_Obtain_Connection_Status(CHAR *link_name);
STATUS PPP_Ioctl(DV_DEVICE_ENTRY *dev, INT option, DV_REQ *d_req);
VOID   PPP_Protocol_Reject(NET_BUFFER*);
VOID   PPP_Add_Protocol(NET_BUFFER*, UINT16);
STATUS PPP_Attach_IP_Address(DV_DEVICE_ENTRY*);
STATUS PPP_Attach_DNS_Address(LINK_LAYER*, UINT8);


STATUS AUTH_Init(DV_DEVICE_ENTRY*);
STATUS AUTH_Add_User(CHAR*, CHAR*);
STATUS AUTH_Remove_User(CHAR*);
STATUS AUTH_Check_Login(CHAR*, CHAR*);

extern TQ_EVENT     PPP_Event;
extern UINT8        PPP_Null_Ip[16];

extern NU_SEMAPHORE PPP_Users;


#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_PPP_EXTR_H */
