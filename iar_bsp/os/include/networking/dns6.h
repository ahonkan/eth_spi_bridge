/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
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
*       dns6.h
*
*   DESCRIPTION
*
*       This file contains the data structures and defines necessary
*       to support DNS for IPv6.
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

#ifndef DNS6_H
#define DNS6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

VOID        DNS6_Addr_To_String(CHAR *addr, CHAR *new_name);
STATUS      DNS6_Delete_DNS_Server(const UINT8 *dns_ip);
STATUS      DNS_Remove_DNS_Server(DNS_SERVER *dns_server);
INT         DNS6_Get_DNS_Servers(UINT8 *dest, INT size);
VOID        DNS6_Sort_Records(CHAR *);
VOID        DNS6_String_To_Addr(CHAR *addr, CHAR *name);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif
