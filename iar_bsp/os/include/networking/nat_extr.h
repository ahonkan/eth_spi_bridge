/****************************************************************************
*
*            Copyright Mentor Graphics Corporation 2001-2011
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
*       nat_extr.h                                                  
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file contains function definitions for Nucleus NAT
*                                                           
*   DATA STRUCTURES                                                            
*
*       None.
*                                               
*   FUNCTIONS                                                                  
*              
*       None.
*                                             
*   DEPENDENCIES                                                               
*
*       ip.h
*       nat_defs.h
*                                                                
******************************************************************************/

#ifndef _NAT_EXTR_
#define _NAT_EXTR_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* _cplusplus */

#include "networking/ip.h"
#include "networking/nat_defs.h"

STATUS              NAT_Initialize(const NAT_DEVICE *nat_internal_devices, INT dev_count,
                                   const CHAR *nat_external_dev_name);
STATUS              NAT_Translate(DV_DEVICE_ENTRY *device, IPLAYER **nat_pkt, 
                                  NET_BUFFER **nat_buf_ptr);
INT32               NAT_Find_Translation_Entry(UINT8 protocol, UINT8 device_type, 
                                               UINT32 source_addr, UINT16 source_port, 
                                               UINT32 dest_addr, UINT16 dest_port,
                                               const NAT_PORTMAP_ENTRY *portmap_entry);
VOID                NAT_Delete_Translation_Entry(UINT8 protocol, INT32 index);
INT32               NAT_Assign_Port(UINT8 protocol);
NAT_PORTMAP_ENTRY   *NAT_Find_Portmap_Entry(UINT8 protocol, UINT8 device_type,
                                            UINT32 source_addr, UINT16 source_port, 
                                            UINT32 dest_addr, UINT16 dest_port);
INT32               NAT_Add_Translation_Entry(UINT8 protocol, UINT32 source_addr, 
                                              UINT16 source_port, UINT32 dest_addr, 
                                              UINT16 dest_port, DV_DEVICE_ENTRY *nat_device,
                                              NAT_PORTMAP_ENTRY *portmap_entry);
STATUS              NAT_Parse_Packet(IPLAYER *pkt, NET_BUFFER *buf_ptr, NAT_PACKET *nat_packet);
STATUS              NAT_ICMP_Parse_Packet(const NET_BUFFER *buf_ptr, NAT_PACKET *nat_packet);
VOID                NAT_Build_Outgoing_Packet(const NET_BUFFER *buf_ptr, 
                                              const NAT_PACKET *nat_packet, 
                                              INT32 source_port);
VOID                NAT_Build_Incoming_Packet(const NET_BUFFER *buf_ptr, 
                                              const NAT_PACKET *nat_packet, UINT32 dest_ip, 
                                              INT32 dest_port);
STATUS              NAT_Adjust_Checksum(UINT8 *chksum, const UINT8 *optr, INT32 olen, 
                                        const UINT8 *nptr, INT32 nlen);
STATUS              NAT_Portmapper(UINT8 function, VOID *buffer, UINT32 length);
VOID                NAT_Update_Portmap_Table(NAT_PORTMAP_ENTRY *portmap_entry);
INT32               NAT_Add_ICMP_Translation_Entry(UINT32 source_addr, UINT32 dest_addr, 
                                                   UINT32 icmp_type, UINT16 sequence_number,
                                                   DV_DEVICE_ENTRY *internal_device);
INT32               NAT_Find_ICMP_Entry(UINT32 source_addr, UINT32 icmp_type, 
                                        UINT16 sequence_number);
VOID                NAT_Delete_ICMP_Entry(INT32 index);
VOID                NAT_Cleanup_UDP(VOID);
VOID                NAT_Cleanup_TCP(VOID);
VOID                NAT_Cleanup_ICMP(VOID);
VOID                NAT_Cleanup_FTP(VOID);
STATUS              NAT_Transmit_Packet(NET_BUFFER *buf_ptr, const NAT_PACKET *nat_packet, 
                                        UINT32 ip_addr, DV_DEVICE_ENTRY *internal_device);
VOID                NAT_Adjust_Protocol_Checksum(const NET_BUFFER *buf_ptr, 
                                                 const NAT_PACKET *nat_packet, 
                                                 UINT8 *old_data);
STATUS              NAT_Reassemble_Packet(IPLAYER **pkt, NET_BUFFER **buf_ptr);
VOID                NAT_Find_Route(RTAB_ROUTE *ro);
#if INCLUDE_TCP
VOID                NAT_Update_TCP(const NAT_PACKET *nat_packet, NAT_TCP_ENTRY *entry, 
                                   INT32 index);
#endif

VOID                NAT_Cleanup(VOID);
VOID                NAT_Remove_Interface(const DV_DEVICE_ENTRY *dv_entry);
STATUS              NAT_Shutdown(VOID);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _NAT_EXTR_ */
