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
*   FILE                                             VERSION
*
*       fc_extr.h                                    1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client/Server
*       Common Functions
*
*   DESCRIPTION
*
*       This file contains function prototypes and defined constants for
*       the common Nucleus FTP client and server functions. All correlating
*       function definitions are located in fc.c.
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
*       nucleus.h
*       target.h
*
*************************************************************************/

#ifndef FC_EXTR_H
#define FC_EXTR_H

#include "nucleus.h"
#include "networking/target.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

INT         FC_StringToAddr(struct addr_struct*, CHAR*);
INT         FC_AddrToString(struct addr_struct*, CHAR*);
INT         FC_Parse_Extended_Command(CHAR *, CHAR **, CHAR **, CHAR **,
                                      CHAR, INT);
INT         FC_Store_Addr(struct addr_struct *, CHAR *, CHAR);
UINT32      FTP_Handle_File_Length(INT file_desc);

#if (NET_VERSION_COMP <= NET_4_3)
STATUS      NU_Get_Sock_Name(INT, struct sockaddr_struct*, INT16*);
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

