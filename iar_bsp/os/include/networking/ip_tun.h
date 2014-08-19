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
* FILE NAME
*
*       ip_tun.h
*
* COMPONENT
*
*       IP - Tunnels
*
* DESCRIPTION
*
*       This file contains definitions for functions and structures
*       responsible for the maintenance of IP Tunnels.
*
* DATA STRUCTURES
*
*       IP_TUNNEL_INTERFACE
*       IP_TUNNEL_INTERFACE_ROOT
*       IP_TUNNEL_CONFIG
*       IP_TUNNEL_CONFIG_ROOT
*       IP_INIT_TUNNEL_FUNCTION
*       IP_CLEAR_TUNNEL_FUNCTION
*       IP_TUNNEL_PROTOCOL
*       IP_TUNNEL_PROTOCOL_ROOT
*
* DEPENDENCIES
*
*
*************************************************************************/
#ifndef IP_TUN_H
#define IP_TUN_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* The following macros define constants for the encapsulation methods. */
#define IP_ENCAPS_OTHER                     1
#define IP_ENCAPS_DIRECT                    2
#define IP_ENCAPS_GRE                       3
#define IP_ENCAPS_MINIMAL                   4
#define IP_ENCAPS_L2TP                      5
#define IP_ENCAPS_PPTP                      6
#define IP_ENCAPS_L2F                       7
#define IP_ENCAPS_UDP                       8
#define IP_ENCAPS_ATMP                      9

/* The following macros define constants for tunnel errors. */
#define IP_TUN_EXIST                        -500
#define IP_TUN_ERR                          -502

/* The following macros define options for getting and setting tunnels. */
#define IP_NEXT_TUNNEL                      501
#define IP_IS_TUNNEL                        502
#define IP_HOP_LIMIT                        503
#define IP_LOCAL_ADDRESS                    504
#define IP_REMOTE_ADDRESS                   505
#define IP_ENCAPS_METHOD                    506
#define IP_TUNNEL_NAME                      507
#define IP_SECURITY                         508

/* The following macros define constants for the type of security used
   to secure the outer IP header. */
#define IP_SECURITY_NONE                    1
#define IP_SECURITY_IPSEC                   2
#define IP_SECURITY_OTHER                   3

/* The following macros define how a TOS may be calculated. */
#define IP_TOS_FROM_PAYLOAD                 (INT8)-1
#define IP_TOS_FROM_TRAFFIC_CONDITIONER     (INT8)-2

/* Default value for CFG timeout. */
#define IP_CFG_TIMEOUT                      0

/* The following structures define a list for IP Tunnels. */
typedef struct _ip_cfg_tunnel
{
    /* Tunnel timeout. */
    UINT32       tun_timeout;

    /* Tunnel device name. */
    CHAR         *tun_dev_name;

    /* Tunnel local address. */
    UINT8        *tun_local_addr;

    /* Tunnel remote address. */
    UINT8        *tun_remote_addr;

}IP_CFG_TUNNEL;



/* The following two structures define a list for IP Tunnels. */
typedef struct _ip_tunnel_interface
{
    struct _ip_tunnel_interface         *ip_flink;
    struct _ip_tunnel_interface         *ip_blink;

    /* Tunnel interface index. */
    UINT32                              ip_if_index;

    /* Tunnel interface's hop limit. */
    UINT32                              ip_hop_limit;

    /* Tunnel interface's local address. */
    UINT8                               ip_local_address[IP_ADDR_LEN];

    /* Tunnel interface's remote address. */
    UINT8                               ip_remote_address[IP_ADDR_LEN];

    /* Tunnel interface's TOS. */
    INT8                                ip_tos;

    /* Tunnel interface's encapsulation method. */
    UINT8                               ip_encaps_method;

    /* Tunnel interface's IP Security. */
    UINT8                               ip_security;

    /* Making the structure word-aligned. */
#if PAD_3
    UINT8                               ip_pad[PAD_3];
#endif

}IP_TUNNEL_INTERFACE;

typedef struct _ip_tunnel_interface_root
{
    IP_TUNNEL_INTERFACE                 *ip_flink;
    IP_TUNNEL_INTERFACE                 *ip_blink;

}IP_TUNNEL_INTERFACE_ROOT;

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

/* The following two structures define a list for IP Tunnel Configuration. */
typedef struct _ip_tunnel_config
{
    struct _ip_tunnel_config            *ip_flink;
    struct _ip_tunnel_config            *ip_blink;

    /* Tunnel interface index. */
    UINT32                              ip_if_index;

    /* Tunnel configuration ID. */
    UINT32                              ip_config_id;

    /* Tunnel interface's local address. */
    UINT8                               ip_local_address[IP_ADDR_LEN];

    /* Tunnel interface's remote address. */
    UINT8                               ip_remote_address[IP_ADDR_LEN];

    /* Tunnel interface's encapsulation method. */
    UINT8                               ip_encaps_method;

    /* Tunnel interface's row status for SNMP usage. */
    UINT8                               ip_row_status;

    /* Making the structure word-aligned. */
#if PAD_2
    UINT8                               ip_pad[PAD_2];
#endif

}IP_TUNNEL_CONFIG;

#define IP_TUN_CONFIG_ACTIVATE(ip_tun_config_ptr) \
    (ip_tun_config_ptr)->ip_row_status = SNMP_ROW_ACTIVE

#define IF_IP_TUN_CONFIG_IS_ACTIVE(ip_tun_config_ptr) \
    if((ip_tun_config_ptr)->ip_row_status == SNMP_ROW_ACTIVE)

typedef struct _ip_tunnel_config_root
{
    IP_TUNNEL_CONFIG                    *ip_flink;
    IP_TUNNEL_CONFIG                    *ip_blink;

}IP_TUNNEL_CONFIG_ROOT;

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

/* This is a function-prototype used to configure the protocol specific aspects of
   an IP tunnel. */
typedef STATUS (*IP_CONFIG_TUNNEL_FUNCTION) (IP_CFG_TUNNEL *cfg_tunnel);

/* The following two structures define a list of Tunneling
   protocols and the corresponding functions to be called
   for their instantiation. */
typedef struct ip_tunnel_protocol
{
    struct ip_tunnel_protocol           *ip_flink;
    struct ip_tunnel_protocol           *ip_blink;

    /* encapsulation method ID. */
    UINT8                               ip_encaps_method;

    /* Interface type for this protocol. */
    UINT8                               ip_if_type;

    /* Count of tunnels associated with this encapsulation method. */
    UINT16                              ip_tunnel_no;

    /* Device structure for this protocol. */
    DEV_DEVICE                          ip_device;

    /* Pointer to the configuration function. */
    IP_CONFIG_TUNNEL_FUNCTION           ip_config_tunnel;

}IP_TUNNEL_PROTOCOL;

typedef struct ip_tunnel_protocol_root
{
    struct ip_tunnel_protocol           *ip_flink;
    struct ip_tunnel_protocol           *ip_blink;

}IP_TUNNEL_PROTOCOL_ROOT;

#if (INCLUDE_IP_TUNNEL == NU_TRUE)
VOID   IP_Tunnel_Init(VOID);
#endif

STATUS IP_Config_Tunnel(IP_CFG_TUNNEL *ip_cfg_tunnel);
STATUS IP_Destroy_Tunnel(DV_DEVICE_ENTRY *dev);

STATUS IP_Create_Tunnel(UINT8 *local_addr, UINT8 *remote_addr, UINT8 encaps_method,
                        UINT32 config_id, UINT32 *if_index);

STATUS IP_Add_Tunnel(IP_TUNNEL_INTERFACE *node);
STATUS IP_Get_Tunnel_By_Ip(const UINT8 *ip_address, UINT32 *if_index, UINT16 flag);
STATUS IP_Get_Tunnel_Opt(UINT32 if_index, INT optname, VOID *optval, INT *optlen);
STATUS IP_Set_Tunnel_Opt(UINT32 if_index, INT optname, const VOID *optval, INT optlen);

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

STATUS IP_Add_Tunnel_Config(IP_TUNNEL_CONFIG *node);
IP_TUNNEL_CONFIG *IP_Get_Tunnel_Config(const UINT8 *local_addr,
                                       const UINT8 *remote_addr,
                                       UINT8 encaps_method, UINT32 config_id);

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE)) */

STATUS IP_Add_Tunnel_Prot(const IP_TUNNEL_PROTOCOL *node);
UINT8  IP_Is_Registered_Prot(UINT8 ip_encaps_method);
STATUS IP_Get_Tunnels_No(UINT8 ip_encaps_method, UINT16 *return_num_tun);

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

UINT16 IP_Get_Next_Tunnel_Config_Entry(UINT8  *local_addr,
                                       UINT8  *remote_addr,
                                       UINT8  *encaps_metod,
                                       UINT32 *config_id);

UINT16 IP_Get_Tunnel_Config_IfIndex(const UINT8 *local_addr,
                                    const UINT8 *remote_addr,
                                    UINT8 encaps_method, UINT32 config_id,
                                    UINT32 *if_index);

UINT16 IP_Get_Tunnel_Config_Status(const UINT8 *local_addr,
                                   const UINT8 *remote_addr,
                                   UINT8 encaps_method, UINT32 config_id,
                                   UINT8 *row_status);

UINT16 IP_Create_Tunnel_Config_Entry(const UINT8 *local_addr,
                                     const UINT8 *remote_addr,
                                     UINT8 encaps_method, UINT32 config_id);

UINT16 IP_Undo_Tunnel_Config_Status(const UINT8 *local_addr,
                                    const UINT8 *remote_addr,
                                    UINT8 encaps_method, UINT32 config_id,
                                    UINT8 row_status);

VOID IP_Commit_Tunnel_Config_Entries(VOID);

UINT16 IP_Commit_Tunnel_Config_Status(const UINT8 *local_addr,
                                      const UINT8 *remote_addr,
                                      UINT8 encaps_method,
                                      UINT32 config_id, UINT8 row_status);

IP_TUNNEL_CONFIG *IP_Tunnel_Config_Get_Location(const IP_TUNNEL_CONFIG_ROOT *config_root,
                                                const UINT8 *local_addr,
                                                const UINT8 *remote_addr,
                                                UINT8 encaps_method,
                                                UINT32 config_id,
                                                INT *cmp_result);

#define IP_TUNNEL_CONFIG_ENTRY_GET_NEXT(local_addr, remote_addr, encaps_method, config_id) \
    IP_Get_Next_Tunnel_Config_Entry(local_addr, remote_addr, (&encaps_method), (&config_id))

#define IP_TUNNEL_CONFIG_IFINDEX_GET(local_addr, remote_addr, encaps_method, config_id, if_index) \
    IP_Get_Tunnel_Config_IfIndex(local_addr, remote_addr, encaps_method, config_id, (&if_index))

#define IP_TUNNEL_CONFIG_STATUS_GET(local_addr, remote_addr,encaps_method,config_id, row_status) \
    IP_Get_Tunnel_Config_Status(local_addr, remote_addr, encaps_method, config_id, (&row_status))

#define IP_TUNNEL_CONFIG_ENTRY_CREATE(local_addr, remote_addr, encaps_method, config_id) \
    IP_Create_Tunnel_Config_Entry(local_addr, remote_addr, encaps_method, config_id)

#define IP_TUNNEL_CONFIG_ENTRIES_COMMIT IP_Commit_Tunnel_Config_Entries

#define IP_TUNNEL_CONFIG_STATUS_UNDO(local_addr, remote_addr, encaps_method, config_id, row_status) \
    IP_Undo_Tunnel_Config_Status(local_addr, remote_addr, encaps_method, config_id, row_status)

#define IP_TUNNEL_CONFIG_STATUS_COMMIT(local_addr, remote_addr, encaps_method, config_id, row_status) \
    IP_Commit_Tunnel_Config_Status(local_addr, remote_addr, encaps_method, config_id, row_status)

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE)) */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP_TUN_H */

