/****************************************************************************
*
*            Copyright Mentor Graphics Corporation 2001-2006 
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
****************************************************************************/
/****************************************************************************
*                                                                            
*   FILENAME                                                                          
*                                                                                    
*       nat_defs.h                                                  
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file contains those defines and data structures necessary to
*       support the Network Address Translator.
*                                                           
*   DATA STRUCTURES                                                            
*
*       NAT_DEVICE                  The API data structure used to hold the
*                                   names of all devices to be used on the
*                                   internal network.
*       NAT_DEV_ENTRY               An internal Nucleus NAT device, along with 
*                                   a next and previous pointer.
*       NAT_DEV_LIST                The head and tail of the internal Nucleus  
*                                   NAT device list.
*       NAT_PORTMAP_SERVICE         The API data structure used to create an
*                                   entry in the Portmap Table.
*       NAT_PORTMAP_ENTRY           An entry in the Portmap Table.
*       NAT_PORTMAP_TABLE           The list of registered services on the
*                                   internal network.
*       NAT_TCP_ENTRY               An entry in the TCP Translation Table.
*       NAT_UDP_ENTRY               An entry in the UDP Translation Table.
*       NAT_ICMP_ENTRY              An entry in the ICMP Translation Table.
*       NAT_PORT_ENTRY              An entry in the Port List.
*       NAT_TCP_TABLE               The list of active TCP connections.
*       NAT_UDP_TABLE               The list of active UDP connections.
*       NAT_ICMP_TABLE              The list of active ICMP connections.
*       NAT_PORT_LIST               The list of ports that can be allocated
*                                   to new connections.
*       NAT_TRANSLATION_TABLE       The table that holds the TCP, UDP and
*                                   ICMP Tables.
*       NAT_ICMP_PACKET             A parsed ICMP packet.
*       NAT_PACKET                  A parsed TCP or UDP packet.
*                                               
*   FUNCTIONS                                                                  
*              
*       None.
*                                             
*   DEPENDENCIES                                                               
*
*       nat_cfg.h
*                                                                
******************************************************************************/

#ifndef _NAT_DEFS_
#define _NAT_DEFS_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* _cplusplus */

#include "networking/nat_cfg.h"

#if ( (INCLUDE_TCP == NU_FALSE) && (NAT_INCLUDE_FTP_ALG == NU_TRUE) )
#error  TCP must be included to include the FTP ALG.
#endif

#if (NAT_MAX_TCP_CONNS > NAT_MAX_UDP_CONNS)
#define     NAT_MAX_CONNS       NAT_MAX_TCP_CONNS
#else
#define     NAT_MAX_CONNS       NAT_MAX_UDP_CONNS
#endif

#if ( (NAT_MIN_PORT + NAT_MAX_CONNS) > 65535)
#error Invalid range of NAT ports.
#endif

#define NAT_Portmap(a, b)   NAT_Portmapper(b, a, sizeof(NAT_PORTMAP_SERVICE))

/* API definitions */
#define NU_NAT_Initialize   NAT_Initialize
#define NU_NAT_Portmapper   NAT_Portmapper

#define     NAT_INTERNAL_DEVICE 1
#define     NAT_EXTERNAL_DEVICE 2

#define     NAT_NO_TCP_ENTRIES      -3000
#define     NAT_NO_UDP_ENTRIES      -3001
#define     NAT_INVAL_PARM          -3002
#define     NAT_NO_NAT              -3003
#define     NAT_NO_ENTRY            -3004
#define     NAT_ENTRY_EXISTS        -3005
#define     NAT_NO_MEMORY           -3006
#define     NAT_NO_ROUTE            -3007
#define     NAT_COMPUTE_CHECKSUM    -3008
#define     NAT_IP_FRAGMENT         -3009

#define     NAT_PORTMAP_ADD     0
#define     NAT_PORTMAP_DELETE  1
#define     NAT_PORTMAP_READ    2

#define     NAT_ICMP_ECHOREPLY          0x00001
#define     NAT_ICMP_UNREACH            0x00008
#define     NAT_ICMP_SOURCEQUENCH       0x00010
#define     NAT_ICMP_ECHO               0x00100
#define     NAT_ICMP_TIMXCEED           0x00800
#define     NAT_ICMP_PARAPROB           0x01000
#define     NAT_ICMP_TIMESTAMP          0x02000
#define     NAT_ICMP_TIMESTAMPREPLY     0x04000
#define     NAT_ICMP_INFOREQUEST        0x08000
#define     NAT_ICMP_INFOREPLY          0x10000

#define NAT_MAX_UINT32  0xFFFFFFFFul
#define NAT_TIME_DIFF(a,b) ((a)>=(b)?((a)-(b)):((NAT_MAX_UINT32)-(b)+(a)))

typedef struct _NAT_DEVICE
{
    CHAR    nat_dev_name[DEV_NAME_LENGTH];
} NAT_DEVICE;

typedef struct _NAT_DEV_ENTRY
{
    struct _NAT_DEV_ENTRY   *nat_dev_next;
    struct _NAT_DEV_ENTRY   *nat_dev_prev;
    DV_DEVICE_ENTRY         *nat_device;
} NAT_DEV_ENTRY;

typedef struct _NAT_DEV_LIST
{
    NAT_DEV_ENTRY   *nat_head;
    NAT_DEV_ENTRY   *nat_tail;
} NAT_DEV_LIST;

/* Entry in the Portmap Table */
typedef struct _NAT_PORTMAP_SERVICE
{
    UINT8               nat_internal_source_ip[4];
    UINT8               nat_protocol;
    UINT8               nat_portmap_service_padding[3];
    UINT16              nat_internal_source_port;   
    UINT16              nat_external_source_port;
} NAT_PORTMAP_SERVICE;

/* Entry in the Portmap Table */
typedef struct _NAT_PORTMAP_ENTRY
{
    UINT8               nat_protocol;
    UINT8               nat_used_flag;
    UINT8               nat_portmap_entry_padding[2];
    UINT16              nat_internal_source_port;   
    UINT16              nat_external_source_port;
    UINT32              nat_internal_source_ip;
    DV_DEVICE_ENTRY     *nat_internal_device;
    struct _NAT_PORTMAP_ENTRY   *nat_next;
} NAT_PORTMAP_ENTRY;

/* Portmap Table */
typedef struct _NAT_PORTMAP_TABLE
{
    NAT_PORTMAP_ENTRY   *nat_head;
} NAT_PORTMAP_TABLE;

#if (INCLUDE_TCP)
/* TCP Entry in the Address Translation Table */
typedef struct _NAT_TCP_ENTRY
{
    UINT8               nat_tcp_entry_padding[2];
    UINT16              nat_internal_source_port;
    UINT16              nat_destination_port;
    INT16               nat_state;
    UINT32              nat_timeout;    
    UINT32              nat_internal_source_ip;
    UINT32              nat_destination_ip;
    INT32               nat_external_port_index;
    DV_DEVICE_ENTRY     *nat_internal_device;
    struct  _NAT_PORTMAP_ENTRY  *nat_portmap_entry;

#if NAT_INCLUDE_FTP_ALG
    union
    {
        INT32   nat_ftp_index;
    } nat_ftp;
#endif

} NAT_TCP_ENTRY;
#endif

#if (INCLUDE_UDP)
/* UDP Entry in the Address Translation Table */
typedef struct _NAT_UDP_ENTRY
{
    UINT16              nat_internal_source_port;
    UINT16              nat_destination_port;
    UINT32              nat_timeout;
    UINT32              nat_internal_source_ip;
    UINT32              nat_destination_ip;
    INT32               nat_external_port_index;
    DV_DEVICE_ENTRY     *nat_internal_device;
    struct  _NAT_PORTMAP_ENTRY  *nat_portmap_entry;
} NAT_UDP_ENTRY;
#endif

typedef struct _NAT_ICMP_ENTRY
{
    UINT8           nat_icmp_entry_padding[2];
    UINT16          nat_sequence_number;
    UINT32          nat_internal_source_ip;
    UINT32          nat_destination_ip;
    UINT32          nat_timeout;
    UINT32          nat_valid_reply;
    DV_DEVICE_ENTRY *nat_internal_device;
} NAT_ICMP_ENTRY;

/* Entry in the List of Available Ports to Allocate to New Sessions */
typedef struct _NAT_PORT_ENTRY
{
    UINT8       nat_port_entry_padding[2];
    UINT16      nat_port;
#if INCLUDE_TCP
    INT32       nat_tcp_used_flag;
#endif
#if INCLUDE_UDP
    INT32       nat_udp_used_flag;
#endif
} NAT_PORT_ENTRY;

#if INCLUDE_TCP
typedef struct _NAT_TCP_TABLE
{
    NAT_TCP_ENTRY       nat_tcp_entry[NAT_MAX_TCP_CONNS];
    INT32               nat_next_available_tcp_index;
} NAT_TCP_TABLE;
#endif

#if INCLUDE_UDP
typedef struct _NAT_UDP_TABLE
{
    NAT_UDP_ENTRY       nat_udp_entry[NAT_MAX_UDP_CONNS];
    INT32               nat_next_available_udp_index;
} NAT_UDP_TABLE;
#endif

typedef struct _NAT_ICMP_TABLE
{
    NAT_ICMP_ENTRY      nat_icmp_entry[NAT_MAX_ICMP_CONNS];
    INT32               nat_next_available_icmp_index;
} NAT_ICMP_TABLE;

typedef struct _NAT_PORT_LIST
{
    NAT_PORT_ENTRY      nat_port_list[NAT_MAX_CONNS];
#if INCLUDE_TCP
    INT32               nat_next_avail_tcp_port_index;
#endif
#if INCLUDE_UDP
    INT32               nat_next_avail_udp_port_index;
#endif
} NAT_PORT_LIST;

/* Address Translation Table */
typedef struct _NAT_TRANSLATION_TABLE
{
#if INCLUDE_TCP
    struct  _NAT_TCP_TABLE  *nat_tcp_table;
#endif
#if INCLUDE_UDP
    struct  _NAT_UDP_TABLE  *nat_udp_table;
#endif
    struct  _NAT_ICMP_TABLE *nat_icmp_table;
    struct  _NAT_PORT_LIST  *nat_port_list;
} NAT_TRANSLATION_TABLE;

typedef struct _NAT_ICMP_PACKET
{
    UINT8   nat_icmp_proto;
    UINT8   nat_icmp_packet_padding[1];
    UINT16  nat_icmp_checksum;
    UINT16  nat_icmp_ip_checksum;
    UINT16  nat_icmp_sequence_number;
    UINT16  nat_icmp_source_port;
    UINT16  nat_icmp_dest_port;
    UINT32  nat_icmp_checksum_length;
    UINT32  nat_icmp_ip_header_length;
    UINT32  nat_icmp_type;
    UINT32  nat_icmp_source_addr;
    UINT32  nat_icmp_dest_addr;
} NAT_ICMP_PACKET;

/* Parsed Nat Packet */
typedef struct _NAT_PACKET
{
    UINT8           nat_device_type;
    UINT8           nat_protocol;
    UINT8           nat_control_bit;
    UINT8           nat_packet_padding[1];
    UINT16          nat_source_port;
    UINT16          nat_dest_port;
    UINT16          nat_ip_checksum;
    UINT16          nat_prot_checksum;
    UINT32          nat_ip_header_length;
    UINT32          nat_source_addr;
    UINT32          nat_dest_addr;
#if (INCLUDE_TCP || INCLUDE_UDP)
    UINT32          nat_prot_header_length;
    UINT32          nat_prot_data_length;
#endif
    struct  _NAT_ICMP_PACKET    nat_icmp_packet;
} NAT_PACKET;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _NAT_DEFS_ */
