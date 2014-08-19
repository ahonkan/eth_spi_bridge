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
*       arp.h
*
* DESCRIPTION
*
*       This include file will handle ARP and RARP protocol defines.
*
* DATA STRUCTURES
*
*       ARP_ENTRY
*       ARP_LAYER
*       ARP_RESOLVE_ENTRY
*       ARP_RESOLVE_LIST
*       ARP_MAC_HEADER
*
* DEPENDENCIES
*
*       No other file dependencies
*
****************************************************************************/

#ifndef ARP_H
#define ARP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */


/***********************************************************************/
/*  ARP cache
*   Data structure for saving low-level information until needed
*/
typedef struct ARP_ENTRY_STRUCT
{
    union {
        UINT8   ip_address[4];
        UINT32  arp_ip_addr;        /* the IP # in question                 */
    } ip_addr;

    INT32   arp_flags;              /* is this a gateway?                   */
                                    /* Is this a valid entry.  For gateways an  */
                                    /* entry is created before the HW addr is   */
                                    /* known.  This flag indicates when the HW  */
                                    /* is resolved.                             */
    UINT32  arp_time;               /* time information                     */
    INT32   arp_dev_index;          /* The device index */
    UINT8   arp_mac_addr[DADDLEN];  /* hardware address for this IP address */
    UINT8   pad[2];
} ARP_ENTRY;

#define ARP_UP          0x1         /* Is this entry valid. */
#define ARP_GATEWAY     0x2         /* Is this entry for a gateway. */
#define ARP_PERMANENT   0x4         /* Is this entry permanent. */

/* This structure defines an ARP packet. */
typedef struct ARP_LAYER_STRUCT
{
    UINT16  arp_hrd;                /* hardware type, Ethernet = 1 */
    UINT16  arp_pro;                /* protocol type to resolve for */
    UINT8   arp_hln;                /* byte length of hardware addr = 6 for ETNET */
    UINT8   arp_pln;                /* byte length of protocol = 4 for IP */
    UINT16  arp_op;                 /* opcode, request = 1, reply = 2, RARP = 3,4 */
    UINT8   arp_sha[DADDLEN];       /* Source Hardware Address */
    UINT8   arp_spa[IP_ADDR_LEN];   /* Source protocol address. */
    UINT8   arp_tha[DADDLEN];       /* Target Hardware Address */
    UINT8   arp_tpa[IP_ADDR_LEN];   /* Target protocol address. */
/*
*   the final four fields (contained in 'rest') are:
*     sender hardware address:   sha       hln bytes
*     sender protocol address:   spa       pln bytes
*     target hardware address:   tha       hln bytes
*     target protocol address:   tpa       pln bytes
*/
} ARP_LAYER;


/*************************************************************************/
/*  Dave Plummer's  Address Resolution Protocol (ARP) (RFC-826) and
*   Finlayson, Mann, Mogul and Theimer's Reverse ARP packets.
*
*   Note that the 2 byte ints are byte-swapped.  The protocols calls for
*   in-order bytes, and the PC is lo-hi ordered.
*
*/
#define RARPR   0x0004       /*  RARP reply, from host, needs swap */
#define RARPQ   0x0003       /*  RARP request, needs swapping */
#define ARPREP  0x0002       /*  reply, byte swapped when used */
#define ARPREQ  0x0001       /*  request, byte-swapped when used */
#define ARPPRO  0x0800       /*  IP protocol, needs swapping */

/* Length of the ARP Header */
#define ARP_HEADER_LEN          28

/* ARP_RESOLVE_STRUCT is used to keep track the resolution of MAC layer
   addresses.
 */
typedef struct ARP_RESOLVE_STRUCT
{
    struct ARP_RESOLVE_STRUCT   *ar_next;
    struct ARP_RESOLVE_STRUCT   *ar_prev;
    DV_DEVICE_ENTRY             *ar_device;
    UINT32                      ar_dest;
    NU_TASK                     *ar_task;
    NET_BUFFER                  *ar_buf_ptr;
    INT                         ar_send_count;
    INT                         ar_pkt_type;
    UINT16                      ar_id;
} ARP_RESOLVE_ENTRY;

typedef struct _ARP_RESOLVE_LIST
{
    struct ARP_RESOLVE_STRUCT   *ar_head;
    struct ARP_RESOLVE_STRUCT   *ar_tail;
} ARP_RESOLVE_LIST;

/* This structure is used by ARP to build the MAC layer header in before
   passing it to the MAC layer send routine. */
typedef struct ARP_MAC_HEADER_STRUCT
{
    UINT8           ar_len;
    UINT8           ar_family;
    union {
        /* Right now the only MAC layer supported with ARP is ethernet. However,
           an entry for other such as token ring could be added to this union. */
        DLAYER      ar_mac_ether;
    } ar_mac;

} ARP_MAC_HEADER;

/* Function Prototypes */
ARP_ENTRY  *ARP_Find_Entry(const SCK_SOCKADDR_IP *dest);
STATUS      ARP_Resolve(DV_DEVICE_ENTRY *int_face, const SCK_SOCKADDR_IP *ip_dest,
                   UINT8 *mac_dest, NET_BUFFER *buf_ptr);
STATUS      ARP_Request(DV_DEVICE_ENTRY *device, const UINT32 *tip, const UINT8 *thardware,
                   UINT16 protocol_type, UINT16 arp_type);
INT         ARP_Cache_Update(UINT32 ipn, const UINT8 *hrdn, INT flags, INT32 dev_index);
STATUS      ARP_Reply(const UINT8 *, UINT32, UINT32, DV_DEVICE_ENTRY *);
VOID        ARP_Event(UINT16 id);
VOID        ARP_Cleanup_Entry(ARP_RESOLVE_ENTRY *);
STATUS      ARP_Interpret(ARP_LAYER *a_pkt, DV_DEVICE_ENTRY *device);
STATUS      ARP_Rarp(const CHAR *device_name);
STATUS      ARP_Delete_Entry(const SCK_SOCKADDR_IP *dest);
STATUS      ARP_Update(const ARP_ENTRY *arp_changes, const UINT8 *target_ip);
ARP_ENTRY   *ARP_Get_Next(const UINT8 *ip_addr, ARP_ENTRY *arp_entry);
ARP_ENTRY   *ARP_Get_Next_If(UINT32 if_index, const UINT8 *ip_addr,
                           ARP_ENTRY *arp_entry);
INT32       ARP_Get_Index(const UINT8 *ip_addr);
ARP_ENTRY   *ARP_Get_Index_If(UINT32 if_index, const UINT8 *ip_addr,
                         ARP_ENTRY *arp_entry);
VOID        ARP_Proxy_ARP (const ARP_LAYER *a_pkt_ptr, DV_DEVICE_ENTRY *dev_ptr,
                           UINT32 src_ip, UINT32 dest_ip);
STATUS      ARP_Probe(DV_DEVICE_ENTRY *device, UINT32 sip);
VOID        ARP_LL_Event_Handler(TQ_EVENT event, UNSIGNED dev_index,
                                 UNSIGNED ext_data);

/* Offsets of the ARP header fields. */
#define ARP_HRD_OFFSET              0
#define ARP_PRO_OFFSET              2
#define ARP_HLN_OFFSET              4
#define ARP_PLN_OFFSET              5
#define ARP_OP_OFFSET               6
#define ARP_SHA_OFFSET              8
#define ARP_SPA_OFFSET              14
#define ARP_THA_OFFSET              18
#define ARP_TPA_OFFSET              24

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif /* ARP_H */
