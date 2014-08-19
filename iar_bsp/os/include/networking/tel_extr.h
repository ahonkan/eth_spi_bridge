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
*       tel_extr.h                                     
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus Telnet
*
*   DESCRIPTION
*
*       External definitions for functions in Nucleus Telnet.
*       This include file needs to go after other include files
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
*       windat.h
*
*************************************************************************/

#ifndef TEL_EXTR_H
#define TEL_EXTR_H

/* contains the definition of NU_TN_PARAMETERS used
   for the prototype of NU_Telnet_Get_Session_Parameters() */
#include "networking/windat.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Define external references to global variables */
extern INT NU_Telnet_Socket(CHAR *, CHAR *);
extern INT NU_Telnet_Server_Accept(INT);
extern INT NU_Telnet_Client_Connect(CHAR *, CHAR *);
extern INT NU_Telnet_Client_Connect2(CHAR *, CHAR *, INT16);
extern VOID NU_Telnet_Start_Negotiate(INT, CHAR *);
extern INT NU_Telnet_Specific_Negotiate(INT, INT);
extern INT NU_Telnet_Pick_Up_Ascii(UINT8 *, INT);
extern STATUS NU_Telnet_Client_Init_Parameters(INT);
extern STATUS NU_Telnet_Server_Init_Parameters(INT);
extern STATUS NU_Telnet_Get_Session_Parameters(INT, NU_TN_PARAMETERS *);
extern STATUS NU_Telnet_Do_Negotiation(INT, CHAR *);
extern STATUS NU_Wait_For_Pattern(INT, CHAR *, UINT16, UINT16 *,
                                 const CHAR *, UINT16, UINT16 *, UNSIGNED);
extern VOID NU_Telnet_Free_Parameters(INT);
extern INT NU_Telnet_Check_Connection(INT, INT);
extern INT NU_Telnet_Parse(INT, UINT8 *, INT, INT);
extern INT NU_Received_Exit(CHAR *, INT);
extern VOID NEG_Parsewrite(CHAR *, INT);
extern VOID NEG_Set_One_Option(CHAR **, INT, CHAR);
extern INT NU_Telnet_Echo_Get(INT);
extern STATUS NU_Telnet_Echo_Set(INT, INT);
extern VOID NEG_Parse_Subnegotiat(INT, UINT8 *);
extern INT NU_Install_Negotiate_Options(CHAR *, CHAR *, INT);
extern INT NU_Close_And_Check_Retval(INT);
extern INT NEG_Check_Recv_Retval(INT, INT);
extern STATUS NU_Telnet_Get_Filtered_Char(INT, UNSIGNED, CHAR *);
extern INT NU_Telnet_Send(INT, CHAR *);
extern INT NU_Receive_NVT_Key(INT, CHAR *, CHAR);
extern VOID NU_Send_NVT_Key(INT, UINT32);
extern VOID TN_Print(CHAR *fmt);

extern STATUS NEG_Cln_Handle_IAC_DO_NAWS(INT);
extern STATUS NEG_Cln_Handle_IAC_DONT_NAWS(INT);
extern STATUS NEG_Cln_Handle_IAC_DO_TERMTYPE(INT);
extern STATUS NEG_Cln_Handle_IAC_DONT_TERMTYPE(INT);

extern STATUS NEG_Srv_Handle_IAC_WILL_NAWS(INT);
extern STATUS NEG_Srv_Handle_IAC_WONT_NAWS(INT);
extern STATUS NEG_Request_Client_Send_TermType(INT);
extern STATUS NEG_Srv_Handle_IAC_WILL_TERMTYPE(INT);
extern STATUS NEG_Srv_Handle_IAC_WONT_TERMTYPE(INT);

#ifdef CFG_NU_OS_NET_PROT_TELNET_SHELL_ENABLE
#include "services/nu_services.h"
extern STATUS NU_Create_Telnet_Shell(NU_SHELL **);
extern STATUS NU_Get_Telnet_Shell_Session(NU_SHELL **);
#else
#define NU_Create_Telnet_Shell(x)           NU_UNAVAILABLE
#define NU_Get_Telnet_Shell_Session(x)      NU_UNAVAILABLE
#endif

extern CHAR client_nego_table[];
extern CHAR server_nego_table[];

/* This definition is to maintain backward compatibility with existing Telnet
   applications. */
#define NU_Close_and_check_retval(a, b)     NU_Close_And_Check_Retval(a)
#define NU_check_Recv_retval(a, b)          NEG_Check_Recv_Retval(a, b)
#define received_exit(a, b)                 NU_Received_Exit(a, b)
#define NU_Send_NVT_key                     NU_Send_NVT_Key(a, b)

#define NU_Telnet_Init_Parameters NU_Telnet_Server_Init_Parameters


/* TL_Unused_Parameter is defined in xprot/telnet/src/tl_util.c */
extern UINT32 TL_Unused_Parameter;

/* this macro is used to remove compilation warnings. */
#define TL_UNUSED_PARAMETER(x)  TL_Unused_Parameter = ((UINT32)(x))


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* TEL_EXTR_H */
