/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
* FILE NAME
*
*      netboot_query.h
*
* DESCRIPTION
*
*      This file contains the definitions, datatypes and function
*      prototypes needed by the registery query code and the
*      interface configuration list management code.
*
*   DATA STRUCTURES
*
*       IFCONFIG_NODE
*       IFCONFIG_NODE_LIST
*
*   DEPENDENCIES
*
*       nu_net.h
*       nucleus.h
*
*************************************************************************/

#ifndef _NETBOOT_QUERY_H_
#define _NETBOOT_QUERY_H_

#include "networking/nu_net.h"
#include "nucleus.h"
#include "networking/dhcp.h"

/* Define the size of the buffer needed to hold an ipv4 address
   in ASCII form including the NULL terminator.
   nnn.nnn.nnn.nnn */
#define IPV4_ASCII_BUFSIZE    16

/* Define the size of the buffer needed to hold an ipv4 address
   in ASCII form including the NULL terminator.
   nnnn:nnnn:nnnn:nnnn:nnnn:nnnn:nnnn:nnnn */
#define IPV6_ASCII_BUFSIZE    40

/*************************/
/* Structure Definitions */
/*************************/

/* This contains the interface configuration information related
 * to a single Interface.
 */
typedef struct Ifconfig_Node
{
    /* Forward and back links of the linked list. */
    struct Ifconfig_Node *if_flink;
    struct Ifconfig_Node *if_blink;

    CHAR  if_name[DEV_NAME_LENGTH];
    UINT8 IPv4_Enabled;
    UINT8 DHCP4_Enabled;
    UINT8 IPv6_Enabled;
    UINT8 DHCP6_Enabled;
    UINT8 Ipv4_Address[IP_ADDR_LEN];
    UINT8 Ipv4_Netmask[IP_ADDR_LEN];
    UINT8 Ipv6_Address[MAX_ADDRESS_SIZE];
    UINT8 Ipv6_Prefix_Length;

    DEV_IF_ADDRESS_LIST ifconfig_addr_list;
#if CFG_NU_OS_NET_STACK_INCLUDE_DHCP
    DHCP_STRUCT         ifconfig_dhcp;
#endif
} IFCONFIG_NODE;

/* The head node of the Interface Configuration list. */
typedef struct Ifconfig_Node_list
{
    /* Forward and back links of the linked list. */
    struct Ifconfig_Node *if_flink;
    struct Ifconfig_Node *if_blink;
} IFCONFIG_NODE_LIST;

/***********************/
/* Function Prototypes */
/***********************/

STATUS Ifconfig_Init(VOID);
STATUS Ifconfig_Deinit(VOID);
STATUS Ifconfig_Remove_All_Interfaces(VOID);
STATUS Ifconfig_Find_Interface(const CHAR *Interface_name,
                               IFCONFIG_NODE **Interface_ptr);
STATUS NU_Ifconfig_Update_Interface(CHAR *Interface_name,
                               IFCONFIG_NODE *Interface_ptr);
STATUS NU_Ifconfig_Delete_Interface(const CHAR *Interface_name);
STATUS NU_Ifconfig_Set_Interface_Defaults(CHAR *IF_Name, CHAR *path);

/* These function get configuration info from the read-only
   registry. */

UINT32 NU_Registry_Get_IPv4_Enabled(CHAR *);
UINT32 NU_Registry_Get_DHCP4_Enabled(CHAR *);
UINT32 NU_Registry_Get_IPv6_Enabled(CHAR *);
UINT32 NU_Registry_Get_DHCP6_Enabled(CHAR *);
STATUS NU_Registry_Get_Ipv4_Address(CHAR *, UINT8 *);
STATUS NU_Registry_Get_Ipv4_Netmask(CHAR *, UINT8 *);
STATUS NU_Registry_Get_Ipv6_Address(CHAR *, UINT8 *);
STATUS NU_Registry_Get_Ipv6_Prefix_Length(CHAR *, UINT8 *);

/* These function get and set configuration info from/to the
   interface configuration list. */

UINT32 NU_Ifconfig_Get_IPv4_Enabled(CHAR *);
UINT32 NU_Ifconfig_Get_DHCP4_Enabled(CHAR *);
STATUS NU_Ifconfig_Set_DHCP4_Enabled(const CHAR *, UINT8);
UINT32 NU_Ifconfig_Get_IPv6_Enabled(CHAR *);
UINT32 NU_Ifconfig_Get_DHCP6_Enabled(CHAR *);
STATUS NU_Ifconfig_Set_DHCP6_Enabled(CHAR *, UINT8);
STATUS NU_Ifconfig_Get_Ipv4_Address(CHAR *, UINT8 *, UINT8 *);
STATUS NU_Ifconfig_Set_Ipv4_Address(const CHAR *, UINT8 *, const UINT8 *);
STATUS NU_Ifconfig_Delete_Ipv4_Address(CHAR *, UINT32, UINT32);
STATUS NU_Ifconfig_Get_Ipv6_Address(CHAR *, UINT8 *, UINT8 *);
STATUS NU_Ifconfig_Set_Ipv6_Address(CHAR *, UINT8 *, UINT8);
STATUS NU_Ifconfig_Delete_Ipv6_Address(CHAR *, UINT8 *, UINT8);
BOOLEAN NU_Ifconfig_Validate_Interface(CHAR *name);

#endif /* _NETBOOT_QUERY_H_ */

