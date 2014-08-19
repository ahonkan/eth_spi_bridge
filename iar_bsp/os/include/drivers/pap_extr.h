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
*       pap_extr.h
*
*   COMPONENT
*
*       PAP - Password Authentication Protocol
*
*   DESCRIPTION
*
*       Contains the function prototypes for pap.c.
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
#ifndef PPP_INC_PAP_EXTR_H
#define PPP_INC_PAP_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

STATUS PAP_Check_Login(CHAR id[PPP_MAX_ID_LENGTH],
                       CHAR pw[PPP_MAX_PW_LENGTH]);
VOID   PAP_Send_Authentication(DV_DEVICE_ENTRY *);
VOID   PAP_Interpret(NET_BUFFER *);
VOID   PAP_Send_Authentication_Ack_Nak(NET_BUFFER*, UINT8);
VOID   PAP_Timer_Expire(UNSIGNED);

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_PAP_EXTR_H */
