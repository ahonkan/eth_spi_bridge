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
*       mdm_extr.h
*
*   COMPONENT
*
*       MDM - Modem Control
*
*   DESCRIPTION
*
*       This file contains function prototypes and structure definitions
*       used by the modem module of PPP and which are also accessible
*       to other modules.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       mdm_defs.h
*
*************************************************************************/
#ifndef PPP_INC_MDM_EXTR_H
#define PPP_INC_MDM_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

#include "drivers/mdm_defs.h"

STATUS  MDM_Init(DV_DEVICE_ENTRY *);
STATUS  MDM_Data_Ready(DV_DEVICE_ENTRY *dev_ptr);
STATUS  NU_Terminal_Data_Ready(CHAR *link_name);
STATUS  MDM_Dial(CHAR *number, DV_DEVICE_ENTRY *);
CHAR   *MDM_Get_Modem_String(CHAR *, CHAR *, DV_DEVICE_ENTRY *, UINT8);
VOID    MDM_Delay(UNSIGNED milliseconds);
VOID    MDM_Receive(DV_DEVICE_ENTRY * , INT );
STATUS  MDM_Hangup(LINK_LAYER*, UINT8);
STATUS  MDM_Wait_For_Client (DV_DEVICE_ENTRY *);
STATUS  MDM_Modem_Connected (DV_DEVICE_ENTRY *dev_ptr);
STATUS  MDM_Rings_To_Answer(CHAR *link_name, UINT8 num_rings);
STATUS  MDM_Purge_Input_Buffer(DV_DEVICE_ENTRY *dev_ptr);
STATUS  MDM_Change_Communication_Mode(INT mode, DV_DEVICE_ENTRY *dev_ptr);
STATUS  NU_Purge_Terminal_Buffer(CHAR *link_name);
STATUS  NU_Change_Communication_Mode(INT mode, CHAR *link_name);
STATUS  MDM_Control_String(CHAR *string, DV_DEVICE_ENTRY *dev_ptr);
STATUS  NU_Modem_Control_String(CHAR *string, CHAR *);
STATUS  MDM_Get_Char(CHAR *c, DV_DEVICE_ENTRY *dev_ptr);
STATUS  NU_Get_Terminal_Char(CHAR *c, CHAR *link_name);
STATUS  NU_Put_Terminal_Char (CHAR c, CHAR *link_name);
STATUS  MDM_Put_Char (CHAR c, DV_DEVICE_ENTRY *dev_ptr);
STATUS  MDM_Carrier (DV_DEVICE_ENTRY *dev_ptr);
STATUS  NU_Carrier (CHAR *link_name);
VOID    NU_Reset_Modem (CHAR *dev_name);
STATUS  NU_Modem_Set_Local_Num(CHAR *link_name, CHAR *local_num);
STATUS  MDM_Set_Local_Num(DV_DEVICE_ENTRY *dev_ptr, CHAR *local_num);
STATUS  NU_Modem_Get_Remote_Num(CHAR *link_name, CHAR *remote_num);
STATUS  MDM_Get_Remote_Num(DV_DEVICE_ENTRY *dev_ptr, CHAR *remote_num);
VOID    MDM_Task_Entry(UNSIGNED argc, VOID *argv);

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_MDM_EXTR_H */
