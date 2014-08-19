/*************************************************************************
*
*             Copyright 1995-2007 Mentor Graphics Corporation
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
*       tftpexts.h                                     
*
*   COMPONENT
*
*       Nucleus TFTP -  Nucleus Trivial File Transfer Protocol
*
*   DESCRIPTION
*
*       This file contains function prototypes of all Nucleus TFTP
*       functions.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       tftpdefs.h
*
*************************************************************************/

#ifndef NU_TFTPEXTS_H
#define NU_TFTPEXTS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* error codes for tftp server */
#define TFTPS_DUPLICATE_DATA         -1763
#define TFTPS_DUPLICATE_ACK          -1764
#define TFTPS_SERVER_TERMINATED      -1765

INT32   TFTPS_Recv(VOID);
INT16   TFTPS_RRecv(VOID);
STATUS  TFTPS_Process_Data(INT32, INT);
STATUS  TFTPS_Ack(VOID);
STATUS  TFTPS_Process_Ack(VOID);
INT32   TFTPS_Retransmit(INT32);
STATUS  TFTPS_Error(INT16, char *);
INT32   TFTPS_Send_Data(INT);
INT     TFTPS_Process_Request_PACKET(INT32);
STATUS  TFTPS_Check_Options(INT32, INT16, INT);
BOOLEAN TFTP_Is_Server_Running(VOID);
STATUS  TFTP_Server_Init(CHAR *);
VOID    TFTP_Server_Task(UNSIGNED argc, VOID *argv);
STATUS  TFTP_Server_Uninit(VOID);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NU_TFTPEXTS_H */
