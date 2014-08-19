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

/**************************************************************************
*
* FILENAME
*
*      dns4.h
*
* DESCRIPTION
*
*      This include file will have external definitions for functions
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*      No other file dependencies
*
****************************************************************************/

#ifndef DNS4_H
#define DNS4_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

STATUS  DNS4_Delete_DNS_Server(const UINT8 *dns_ip);
INT     DNS4_Get_DNS_Servers(UINT8 *dest, INT size);
VOID    DNS4_Addr_To_String(CHAR *addr, CHAR *new_name);
VOID    DNS4_String_To_Addr(CHAR *addr, CHAR *name);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif
