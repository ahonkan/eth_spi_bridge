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

/****************************************************************************
*
*   FILE NAME
*
*       mem_defs.h
*
*   DESCRIPTION
*
*       This file contains the linked list structure definitions used by NET.
*       These lists are used for buffering of incoming and outgoing packets.
*
*   DATA STRUCTURES
*
*       _ME_BUFHDR
*       PACKET_QUEUE_ELEMENT
*       PACKET_QUEUE_HEADER
*       NET_QUEUE_HEADER
*       NET_QUEUE_ELEMENT
*       NET_BUFFER_SUSPENSION_ELEMENT
*       NET_BUFFER_SUSPENSION_LIST
*
*   DEPENDENCIES
*
*       None
*
****************************************************************************/

#ifndef MEM_DEFS_H
#define MEM_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Flags for indicating the destination address of a packet in a buffer. */
#define NET_BCAST       0x01
#define NET_MCAST       0x02
#define NET_IP          0x04
#define NET_LCP         0x08
#define NET_IPCP        0x10
#define NET_IPV6CP      0x20
#define NET_PAP         0x40
#define NET_CHAP        0x80
#define NET_IP6         0x100
#define NET_PARENT      0x200
#define NET_NOHDR       0x400
#define NET_TCP_PMTU    0x800
#define NET_TX_QUEUE    0x1000
#define NET_BUF_SUM     0x2000
#define NET_TCP_SACK    0x4000
#define NET_TCP_DSACK   0x8000

typedef struct packet_queue_header NET_BUFFER_HEADER;

/* Header sizes of other protocols that affect the max. */
#define PPE_HEADER_SIZE 6
#define PPP_HEADER_SIZE 2

#ifndef INCLUDE_PPPOE
#define INCLUDE_PPPOE   NU_FALSE
#endif

#if (defined(INCLUDE_PPPOE) && (INCLUDE_PPPOE == NU_TRUE))
/* This is a definition of the largest Media Access Layer header. It is used in the
   definitions below when deciding how far to offset into a buffer. */
#define NET_MAX_MAC_HEADER_SIZE (sizeof(DLAYER) + PPE_HEADER_SIZE +  \
PPP_HEADER_SIZE)
#else
/* This is a definition of the largest Media Access Layer header. It is used in the
   definitions below when deciding how far to offset into a buffer. */
#define NET_MAX_MAC_HEADER_SIZE (DADDLEN + DADDLEN + sizeof(UINT16))
#endif

#define NET_MAX_UDP_HEADER_SIZE     (IP_HEADER_LEN + UDP_HEADER_LEN)

#define NET_MAX_ARP_HEADER_SIZE     (ARP_HEADER_LEN)

#define NET_MAX_ICMP_HEADER_SIZE    (IP_HEADER_LEN + ICMP_HEADER_LEN)

#define NET_MAX_BUFFER_SIZE \
   (NET_PARENT_BUFFER_SIZE + sizeof(struct _me_bufhdr))

typedef struct packet_queue_element HUGE   NET_BUFFER;

struct _me_bufhdr
{
    UINT32                      seqnum;
    NET_BUFFER_HEADER           *dlist;
    struct _DV_DEVICE_ENTRY     *buf_device;
    UINT16                      option_len;
    INT16                       retransmits;
    UINT16                      tcp_data_len;           /* size of the data in a TCP packet. */
    UINT8                       padN[2];
    UINT32                      total_data_len;         /* size of the entire buffer,
                                                           sum of all in the chain    */
    INT32                       port_index;

#if (INCLUDE_IPSEC == NU_TRUE)

    /* The TCP or UDP port that is being used for this packet. */
    VOID                        *higher_port;

#endif
};

/* Define the queue element used to hold a packet */
struct packet_queue_element
{
    union
    {
        UINT8 packet[NET_MAX_BUFFER_SIZE];

        struct _me_pkthdr
        {
            UINT8                   parent_packet[NET_PARENT_BUFFER_SIZE];
            struct  _me_bufhdr      me_buf_hdr;
        } me_pkthdr;

    } me_data;

    NET_BUFFER                          *next;        /* next buffer chain in the list */
    NET_BUFFER                          *next_buffer; /* next buffer in this chain */

/* added elements to the structure for Debugging */
#ifdef NU_DEBUG_NET_BUFFERS
    NET_BUFFER                          *next_debug;         /* next buffer chain in the list for debugging */
    CHAR                                *who_allocated_file; /* file name for debugging */
    INT                                 who_allocated_line;  /* line number for debugging */
    INT                                 debug_index;         /* the debug index */
#endif  /* NU_DEBUG_NET_BUFFERS */

    UINT8                       HUGE    *data_ptr;
    UINT32                              data_len;     /* size of this buffer */
    UINT16                              pqe_flags;
    UINT8                               padN[2];

    /* New H/W Offloading flags */
#if (HARDWARE_OFFLOAD == NU_TRUE)
    UINT32                              hw_options;
#endif

    UINT32                              chk_sum;            /* The running sum of the data. */
    UINT8                               *sum_data_ptr;      /* The last byte of data included in the sum. */
};

/* These definitions make it easier to access fields within a packet. */
#define mem_seqnum              me_data.me_pkthdr.me_buf_hdr.seqnum
#define mem_dlist               me_data.me_pkthdr.me_buf_hdr.dlist
#define mem_buf_device          me_data.me_pkthdr.me_buf_hdr.buf_device
#define mem_option_len          me_data.me_pkthdr.me_buf_hdr.option_len
#define mem_retransmits         me_data.me_pkthdr.me_buf_hdr.retransmits
#define mem_flags               pqe_flags
#define mem_hw_options          hw_options
#define mem_tcp_data_len        me_data.me_pkthdr.me_buf_hdr.tcp_data_len
#define mem_total_data_len      me_data.me_pkthdr.me_buf_hdr.total_data_len
#define mem_port_index          me_data.me_pkthdr.me_buf_hdr.port_index
#define mem_parent_packet       me_data.me_pkthdr.parent_packet
#define mem_packet              me_data.packet

#if (INCLUDE_IPSEC == NU_TRUE)
#define mem_port                me_data.me_pkthdr.me_buf_hdr.higher_port
#endif

#define NU_NET_BUFFER_POOL_SIZE     ((UINT32)(MAX_BUFFERS * (sizeof(NET_BUFFER) + \
                                    (REQ_ALIGNMENT - sizeof(UNSIGNED)) + DM_OVERHEAD) + \
                                    (2 * DM_OVERHEAD)))


/* Define the header for the buffer queue */
struct packet_queue_header
{
     NET_BUFFER *head;
     NET_BUFFER *tail;
};

/* Define a generic queue header */
struct queue_header
{
    struct queue_element *head, *tail;
};
typedef struct queue_header NET_QUEUE_HEADER;

/* Define a generic queue element */
struct queue_element
{
        struct queue_element *next, *next_buffer;
};
typedef struct queue_element NET_QUEUE_ELEMENT;

/* Define the buffer suspension list structure. This list will
   hold tasks that are waiting to transmit because of lack of
   memory buffers. */
struct _mem_suspension_element
{
    struct _mem_suspension_element *flink;
    struct _mem_suspension_element *blink;
    NU_TASK                        *waiting_task;
    INT                             socketd;
    VOID                           *list_entry;
};

struct _mem_suspension_list
{
    struct _mem_suspension_element *head;
    struct _mem_suspension_element *tail;
};

typedef struct _mem_suspension_list     NET_BUFFER_SUSPENSION_LIST;
typedef struct _mem_suspension_element  NET_BUFFER_SUSPENSION_ELEMENT;

/* Global data structures declared in MEM.C */
extern UINT16                       MEM_Buffers_Used;

/* Global used for debugging buffers */
#ifdef NU_DEBUG_NET_BUFFERS
extern NET_BUFFER            *MEM_Debug_Buffer_List;
#endif /* NU_DEBUG_NET_BUFFERS */

extern NET_BUFFER_SUSPENSION_LIST   MEM_Buffer_Suspension_List;

/* Macros for debug Dequeue and Debug Chain Dequeue */
#ifdef NU_DEBUG_NET_BUFFERS

#define MEM_Buffer_Dequeue(a)           MEM_DB_Buffer_Dequeue(a, __FILE__, __LINE__)
#define MEM_Buffer_Chain_Dequeue(a,b)   MEM_DB_Buffer_Chain_Dequeue(a, b, __FILE__, __LINE__)

#endif /* NU_DEBUG_NET_BUFFERS */


#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif /* MEM_DEFS_H */
