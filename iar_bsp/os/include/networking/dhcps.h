/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2003              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
**************************************************************************/

/**************************************************************************
*                                                                            
* FILE NAME                                                                         
*                                                                                    
*   dhcps.h
*
* COMPONENT
*
*   Nucleus DHCP Server
*                                                                                    
* DESCRIPTION                                                                
*                                                                            
*   Function prototypes, DHCP server database related defines
*   and error return messages for the DHCP server
*                                                                            
* DATA STRUCTURES
*
*   DHCPS_CLIENT_ID
*   DHCPS_CLIENT_HADDR
*   DHCPS_OPTIONS
*   DHCPS_OPTIONS_LIST
*   DHCPS_BINDING
*   DHCPS_BINDING_LIST
*   DHCPS_OFFERED
*   DHCPS_OFFERED_LIST
*   DHCPS_CONFIG_CB
*   DHCPS_CONFIGURATION_LIST
*   OPTIONMAP
*
* DEPENDENCIES
*
*   nucleus_gen_cfg.h
*                                                                            
***************************************************************************/

#ifndef _DHCPS_H_
#define _DHCPS_H_

#include "nucleus_gen_cfg.h"

/* Define service return status constants.  */
#define DHCPSERR_PARAMETER_NOT_REMOVED      -3251
#define DHCPSERR_CONFIGURATION_NOT_FOUND    -3252
#define DHCPSERR_BINDING_NOT_ADDED          -3253
#define DHCPSERR_BINDING_NOT_REMOVED        -3254
#define DHCPSERR_CONFIGURATION_PRESENT      -3255
#define DHCPSERR_IP_RANGE_INVALID           -3256
#define DHCPSERR_OPTION_ALREADY_PRESENT     -3257
#define DHCPSERR_ARRAY_ENTRIES_FULL         -3258
#define DHCPSERR_RANGE_SUBNET_INVALID       -3259
#define DHCPSERR_BUFFER_TOO_SMALL           -3260
#define DHCPSERR_PARAMETER_NOT_FOUND        -3261
#define DHCPSERR_SERVER_NOT_SHUTDOWN        -3262
#define DHCPSERR_CONFIG_FILE_ERROR          -3263
#define DHCPSERR_OPTIONS_FILE_ERROR         -3264
#define DHCPSERR_UNABLE_TO_ALLOCATE_MEMORY  -3265
#define DHCPSERR_BINDING_ALREADY_PRESENT    -3266
#define DHCPSERR_NOT_INITIALIZED            -3267
    
#define ETHERNET_DEVICE                     0x01

typedef struct _DHCPS_BINDING_LIST          DHCPS_BINDING_LIST;
typedef struct _DHCPS_OFFERED_LIST          DHCPS_OFFERED_LIST;
typedef struct DHCPS_CONFIGURATION          DHCPS_CONFIG_CB;
typedef struct _DHCPS_CONFIGURATION_LIST    DHCPS_CONFIGURATION_LIST;


/***** Client ID *****/
typedef struct _DHCPS_CLIENT_ID
{
    CHAR                idtype;
    UINT8               idlen,
                        id[DHCPS_MAX_CLIENT_ID_LENGTH],
                        pad1[2];
}DHCPS_CLIENT_ID;
/***** End Client ID *****/

/***** Client Hardware Address *****/
typedef struct _DHCPS_CLIENT_HADDR
{
    CHAR            hardware_type;
    UINT8           hardware_addr_len,
                    hardware_addr[DADDLEN];
}DHCPS_CLIENT_HADDR;
/***** End Client Hardware Address *****/


/***** DHCP Options *****/
typedef struct _DHCPS_OPTIONS
{
    struct _DHCPS_OPTIONS       *dhcps_options_next,
                                *dhcps_options_previous;
    struct id_struct            client_ip_addr;
    UINT8                       *options_ptr,
                                bytes_written;
    CHAR                        config_entry_name[DHCPS_MAX_ENTRY_NAME_LENGTH],
                                pad1[3];

}DHCPS_OPTIONS;

typedef struct _DHCPS_OPTIONS_LIST
{
    DHCPS_OPTIONS               *dhcps_options_head,
                                *dhcps_options_tail;
}DHCPS_OPTIONS_LIST;
/*****End DHCP Options *****/




typedef struct _DHCPS_BINDING
{
    struct _DHCPS_BINDING           *dhcp_bind_next,
                                    *dhcp_bind_previous;

    DHCPS_CONFIG_CB                 *config;
    DHCPS_CLIENT_ID                 client_id;
    DHCPS_CLIENT_HADDR              chaddr;
    struct id_struct                yiaddr;

    UINT32                          lease_length,
                                    end_lease_time;

    INT32                           lease_time_remaining;

    INT16                           flags;
#define STATIC_ENTRY                0x0001;
#define IN_USE                      0x0002;
#define BINDING_OFFERED             0x0004;
    INT16                           pad1;
}DHCPS_BINDING;


struct _DHCPS_BINDING_LIST
{
    DHCPS_BINDING       *dhcp_bind_head;
    DHCPS_BINDING       *dhcp_bind_tail;
};



typedef struct _DHCPS_OFFERED
{
    struct _DHCPS_OFFERED           *dhcp_offer_next,
                                    *dhcp_offer_previous;
    DHCPS_CLIENT_HADDR              *dp_chaddr;
    struct id_struct                *dp_yiaddr;
    DHCPS_BINDING                   *offered_binding;
    UINT32                          offered_wait_time,
                                    start_offered_time;
}DHCPS_OFFERED;

struct _DHCPS_OFFERED_LIST
{
    DHCPS_OFFERED   *dhcp_offer_head;
    DHCPS_OFFERED   *dhcp_offer_tail;
};

/* This is the structure of supported parameters.  This is where
    any additional parameters should be added. */
struct DHCPS_CONFIGURATION
{
    struct DHCPS_CONFIGURATION      *dhcps_config_next,
                                    *dhcps_config_previous;

    UINT8                           server_ip_addr[IP_ADDR_LEN];
    UINT8                           subnet_addr[IP_ADDR_LEN];
    UINT8                           subnet_mask_addr[IP_ADDR_LEN];
    UINT8                           broadcast_addr[IP_ADDR_LEN];

    DHCPS_BINDING_LIST              ip_binding_list;

    DHCPS_OFFERED_LIST              ip_offer_list;

    DHCPS_OPTIONS_LIST              options_buff;

#if CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DOMAIN_NAME
    CHAR                            dns_domain_name[DHCPS_MAX_DOMAIN_NAME_LENGTH];
    UINT8                           dns_domain_name_length,
                                    pad1[3];
#endif

#if CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DNS_SERVER
    struct id_struct                dns_server[DHCPS_MAX_DNS_SERVERS];
#endif

#if CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_ROUTER
    struct id_struct                router[DHCPS_MAX_ROUTERS];
#endif

    CHAR                            device_name[DEV_NAME_LENGTH],
                                    config_entry_name[DHCPS_MAX_ENTRY_NAME_LENGTH];

    UINT8                           default_ip_ttl,
                                    default_tcp_ttl;

    UINT16                          flags;
#define     GLOBAL_CONFIGURATION            0x0001
#define     TCP_KEEPALIVE_GARBAGE           0x0002
#define     IP_FORWARDING_ENABLED           0x0004
#define     ETHERNET_IEEE_802_ENCAPSULATION 0x0008
#define     TRAILER_ENCAPSULATION           0x0010
#define     CONFIGURATION_ENABLED           0x8000


    UINT32                          dhcp_renew_t1,
                                    dhcp_rebind_t2,
                                    default_lease_length,
                                    max_lease_length,
                                    offered_wait_time,
                                    tcp_keepalive_interval,
                                    arp_cache_to;
};

struct _DHCPS_CONFIGURATION_LIST
{
    DHCPS_CONFIG_CB      *dhcp_config_head;
    DHCPS_CONFIG_CB      *dhcp_config_tail;
};

/* This structure is used to store the opcode relating to the
    option and a function that will handle the option. */
struct OPTIONMAP
{
    CHAR *opcode;
    INT  option_name;
    INT  (*func)(INT, CHAR **, struct DHCPS_CONFIGURATION *);
};

/* API Functions */
STATUS DHCPS_Init(NU_MEMORY_POOL *, const CHAR *, const CHAR *, const CHAR *, const CHAR *, const CHAR *);

STATUS DHCPS_Create_Config_Entry (DHCPS_CONFIG_CB **, const CHAR *, CHAR *);
STATUS DHCPS_Delete_Config_Entry (const DHCPS_CONFIG_CB *);
INT    DHCPS_Get_Config_Entry (UINT8 *, INT);

STATUS DHCPS_Enable_Configuration (DHCPS_CONFIG_CB *);
STATUS DHCPS_Disable_Configuration (DHCPS_CONFIG_CB *);
STATUS DHCPS_Shutdown_Server(VOID);

STATUS DHCPS_Set_Subnet_Address (DHCPS_CONFIG_CB *, const struct id_struct *, const struct id_struct *);
INT    DHCPS_Get_Subnet_Address (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Set_Subnet_Mask (DHCPS_CONFIG_CB *, const struct id_struct *, const struct id_struct *);
INT    DHCPS_Get_Subnet_Mask (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Add_Router (DHCPS_CONFIG_CB *, const struct id_struct *, struct id_struct *);
STATUS DHCPS_Delete_Router (DHCPS_CONFIG_CB *, const struct id_struct *, const struct id_struct *);
INT    DHCPS_Get_Router(const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Add_DNS_Server (DHCPS_CONFIG_CB *, const struct id_struct *, struct id_struct *);
STATUS DHCPS_Delete_DNS_Server (DHCPS_CONFIG_CB *, const struct id_struct *, const struct id_struct *);
INT    DHCPS_Get_DNS_Servers(const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Add_Domain_Name (DHCPS_CONFIG_CB *, const struct id_struct *, const CHAR *);
STATUS DHCPS_Delete_Domain_Name (DHCPS_CONFIG_CB *, const struct id_struct *, const CHAR *);
INT    DHCPS_Get_Domain_Name (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Add_Broadcast_Address (DHCPS_CONFIG_CB *, const struct id_struct *, const struct id_struct *);
STATUS DHCPS_Delete_Broadcast_Address (DHCPS_CONFIG_CB *, const struct id_struct *, const struct id_struct *);
INT    DHCPS_Get_Broadcast_Address (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Add_IP_Range (DHCPS_CONFIG_CB *, const DHCPS_CLIENT_ID *, struct id_struct *, 
                                struct id_struct *);
STATUS DHCPS_Delete_IP_Range (DHCPS_CONFIG_CB *, struct id_struct *, const struct id_struct *);
INT    DHCPS_Get_IP_Range(const DHCPS_CONFIG_CB *, UINT8 *, INT);

STATUS DHCPS_Set_Default_Lease_Time (DHCPS_CONFIG_CB *, const struct id_struct *, UINT32);
INT    DHCPS_Get_Default_Lease_Time (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Set_Renewal_Time (DHCPS_CONFIG_CB *, const struct id_struct *, UINT32);
INT    DHCPS_Get_Renewal_Time (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Set_Rebind_Time (DHCPS_CONFIG_CB *, const struct id_struct *, UINT32);
INT    DHCPS_Get_Rebind_Time (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Set_Offered_Wait_Time (DHCPS_CONFIG_CB *, const struct id_struct *, UINT32);
INT    DHCPS_Get_Offered_Wait_Time (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Set_IP_TTL (DHCPS_CONFIG_CB *, const struct id_struct *, UINT8);
INT    DHCPS_Get_IP_TTL (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Enable_IP_Forwarding (DHCPS_CONFIG_CB *);
STATUS DHCPS_Disable_IP_Forwarding (DHCPS_CONFIG_CB *);

STATUS DHCPS_Enable_Ethernet_IEEE_Encapsulation (DHCPS_CONFIG_CB *);
STATUS DHCPS_Disable_Ethernet_IEEE_Encapsulation (DHCPS_CONFIG_CB *);

STATUS DHCPS_Enable_Trailer_Encapsulation (DHCPS_CONFIG_CB *);
STATUS DHCPS_Disable_Trailer_Encapsulation (DHCPS_CONFIG_CB *);

STATUS DHCPS_Set_ARP_Cache_Timeout (DHCPS_CONFIG_CB *, const struct id_struct *, UINT32);
INT    DHCPS_Get_ARP_Cache_Timeout (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Enable_TCP_Keepalive_Garbage (DHCPS_CONFIG_CB *);
STATUS DHCPS_Disable_TCP_Keepalive_Garbage (DHCPS_CONFIG_CB *);

STATUS DHCPS_Set_TCP_TTL (DHCPS_CONFIG_CB *, const struct id_struct *, UINT8);
INT    DHCPS_Get_TCP_TTL (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Set_TCP_Keepalive_Interval (DHCPS_CONFIG_CB *, const struct id_struct *, UINT32);
INT    DHCPS_Get_TCP_Keepalive_Interval (const DHCPS_CONFIG_CB *, const struct id_struct *, UINT8 *, INT);

STATUS DHCPS_Save_Config_Data_To_File(VOID);

STATUS DHCPS_Server_Reset(VOID);

#endif /* _DHCPS_H_ */
