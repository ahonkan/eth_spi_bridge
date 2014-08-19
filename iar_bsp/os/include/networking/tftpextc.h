/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       tftpextc.h
*
*   COMPONENT
*
*       TFTP -  Trivial File Transfer Protocol
*
*   DESCRIPTION
*
*       This file contains function prototypes of all TFTP functions.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       tftpc4.h
*
*************************************************************************/

#ifndef NU_TFTPEXTC_H
#define NU_TFTPEXTC_H

#include "networking/tftpc4.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define NU_TFTPC_Get2   TFTPC_Get2
#define NU_TFTPC_Put2   TFTPC_Put2

#ifndef NU_TFTP_OPTIONS
#define NU_TFTP_OPTIONS TFTP_OPTIONS
#endif

INT32  TFTPC_Get2(const UINT8 *, const CHAR *, CHAR *, TFTP_OPTIONS *, INT16);
INT32  TFTPC_Put2(const UINT8 *, const CHAR *, CHAR *, TFTP_OPTIONS *, INT16);
STATUS TFTPC_Request(const UINT8 *, const CHAR *, TFTPC_CB *, INT16);
INT32  TFTPC_Recv(TFTPC_CB *);
STATUS TFTPC_Process_Data(TFTPC_CB *, INT32);
STATUS TFTPC_Ack(const TFTPC_CB *);
INT32  TFTPC_Send_Data(TFTPC_CB *);
STATUS TFTPC_Process_Ack(TFTPC_CB *, INT32);
STATUS TFTPC_Retransmit(const TFTPC_CB *, INT32);
STATUS TFTPC_Error(const TFTPC_CB *, INT16, const CHAR *);
STATUS TFTPC_Check_Options(TFTPC_CB *, INT32);
STATUS TFTPC_Set_Options(UINT32 buf_size, TFTP_OPTIONS *ops);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NU_TFTPEXTC_H */
