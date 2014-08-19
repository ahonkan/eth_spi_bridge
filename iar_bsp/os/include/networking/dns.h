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
*       dns.h
*
* DESCRIPTION
*
*       This include file will handle domain processing defines.
*
* DATA STRUCTURES
*
*       DNS_PKT_HEADER
*       DNS_RR
*       DNS_HOST
*       DNS_HOST_LIST
*       DNS_SERVER
*       DNS_SERVER_LIST
*
* DEPENDENCIES
*
*      No other file dependencies
*
***************************************************************************/

#ifndef DNS_H
#define DNS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

extern NU_SEMAPHORE         DNS_Resource;

/* Flags passed in to the resolve functions */
#define DNS_ALL             0x01
#define DNS_ADDRCONFIG      0x02
#define DNS_V4MAPPED        0x04
#define DNS_DEFAULT         (DNS_V4MAPPED | DNS_ADDRCONFIG)

/* Max and Min size definitions. */
#define     DNS_MAX_LABEL_SIZE      63
#define     DNS_MAX_NAME_SIZE       255
#define     DNS_MIN_NAME_ALLOC      52  /* 51 characters + NULL terminator for word-alignment. */

/* This is the port DNS servers listen for queries on. */
#define     DNS_PORT                53

/* Resource Record (RR) type codes: */
#define DNS_TYPE_A      1           /* A host address (RR)    */
#define DNS_TYPE_PTR    12          /* A domain name ptr (RR) */
#define DNS_TYPE_MX     15          /* Mail eXchange record (MX) */
#define DNS_TYPE_CNAME  5           /* Canonical name for an alias (CNAME) */
#define DNS_TYPE_AAAA   28          /* AAAA host address (RR) */
#define DNS_TYPE_ANY    255         /* Any type of record */
#define DNS_TYPE_SRV    33          /* Service record (SRV). */
#define DNS_TYPE_TXT    16          /* Text record (TXT). */
#define DNS_TYPE_NSEC   47          /* Negative Response (NSEC). */

#define DNS_TYPE_A_BITMAP       0x40
#define DNS_TYPE_AAAA_BITMAP    0x00000008

/* TTL value for outgoing NSEC packets. */
#define DNS_NSEC_TTL    120

/* Query flags. */
#define DNS_CLASS_IN    0x1         /* Internet class */
#define DNS_CLASS_ANY   0x11111111  /* Any class. */
#define DNS_UNI_FLAG    0x8000  /* Unicast response flag. */

/* Response flags. */
#define DNS_CACHE_FLUSH_FLAG    0x8000  /* Cache flush flag. */

/*
 *  flag masks for the flags field of the DNS header
 */
#define DNS_QR         0x8000          /* query=0, response=1 */
#define DNS_OPCODE     0x7800          /* opcode, see below */
#define DNS_AA         0x0400          /* Authoritative answer */
#define DNS_TC         0x0200          /* Truncation, response was cut off at 512 */
#define DNS_RD         0x0100          /* Recursion desired */
#define DNS_RA         0x0080          /* Recursion available */
#define DNS_RCODE_MASK 0x000F

/* opcode possible values: */
#define DNS_OPQUERY    0    /* a standard query */
#define DNS_OPIQ       1    /* an inverse query */
#define DNS_OPCQM      2    /* a completion query, multiple reply */
#define DNS_OPCQU      3    /* a completion query, single reply */

/* the rest reserved for future */
#define DNS_ROK        0    /* okay response */
#define DNS_RFORM      1    /* format error */
#define DNS_RFAIL      2    /* their problem, server failed */
#define DNS_RNAME      3    /* name error, we know name doesn't exist */
#define DNS_RNOPE      4    /* no can do request */
#define DNS_RNOWAY     5    /* name server refusing to do request */
#define DNS_WILD       255  /* wildcard for several of the classifications */

/* These definitions are used to control where new DNS servers are added into
   the list of servers. */
#define DNS_ADD_TO_FRONT        1 /* Add to the front of the list. */
#define DNS_ADD_TO_END          2 /* Add to the end of the list. */

/* Flags related to a DNS entry. */
#define DNS_PERMANENT_ENTRY     0x01    /* Do not time this entry out. */
#define DNS_UNIQUE_RECORD       0x02    /* The record is unique to this node. */
#define DNS_LOCAL_RECORD        0x04    /* This node created the record. */
#define DNS_DELAY_RESPONSE      0x08    /* This node will be sent the next time the responder event fires. */
#define DNS_NSEC_RECORD         0x10    /* This is a DNS NSEC record.  */

/* These three flags apply to a record for which we are authoritative. */
#define DNS_AUTHORITATIVE_RECORD    (DNS_PERMANENT_ENTRY | DNS_UNIQUE_RECORD | DNS_LOCAL_RECORD)

#define DNS_SD_DEFAULT_TTL      CFG_NU_OS_NET_STACK_DNS_SD_DEFAULT_TTL
#define DNS_SD_DEFAULT_PRIO     CFG_NU_OS_NET_STACK_DNS_SD_DEFAULT_PRIO
#define DNS_SD_DEFAULT_WEIGHT   CFG_NU_OS_NET_STACK_DNS_SD_DEFAULT_WEIGHT

/* All DNS messages have a header defined as follows. */
typedef struct _DNS_PKT_HEADER
{
    UINT16      dns_id;
    UINT16      dns_flags;
    UINT16      dns_qdcount;
    UINT16      dns_ancount;
    UINT16      dns_nscount;
    UINT16      dns_arcount;

} DNS_PKT_HEADER;

#define DNS_ID_OFFSET                   0
#define DNS_FLAGS_OFFSET                2
#define DNS_QDCOUNT_OFFSET              4
#define DNS_ANCOUNT_OFFSET              6
#define DNS_NSCOUNT_OFFSET              8
#define DNS_ARCOUNT_OFFSET              10

/*
 *  A resource record is made up of a compressed domain name followed by
 *  this structure.  All of these ints need to be byte swapped before use.
 */
typedef struct _DNS_RR
{
    UINT16      dns_type;           /* resource record type=DTYPEA */
    UINT16      dns_class;          /* RR class=DIN */
    UINT32      dns_ttl;            /* time-to-live, changed to 32 bits */
    UINT16      dns_rdlength;       /* length of next field */
    CHAR        dns_rdata[1];       /* data field */
    UINT8       pad1;
} DNS_RR;

typedef struct _DNS_MX_RR
{
    CHAR        dns_name[DNS_MAX_NAME_SIZE];
    UINT32      dns_ttl;                    /* time-to-live, changed to 32 bits */
    UINT16      dns_rdlength;               /* length of next field */
    UINT16      dns_preference;             /* preference of this server */
    CHAR        dns_mx[DNS_MAX_NAME_SIZE];  /* mail exchange */
} DNS_MX_RR;

#define DNS_TYPE_OFFSET                 0
#define DNS_CLASS_OFFSET                2
#define DNS_TTL_OFFSET                  4
#define DNS_RDLENGTH_OFFSET             8
#define DNS_RDATA_OFFSET                10

/* SRV record data offsets. */
#define DNS_SRV_PRIO_OFFSET             10
#define DNS_SRV_WEIGHT_OFFSET           12
#define DNS_SRV_PORT_OFFSET             14
#define DNS_SRV_DATA_OFFSET             16

#define DNS_MX_TYPE_OFFSET              2
#define DNS_MX_CLASS_OFFSET             4
#define DNS_MX_TTL_OFFSET               6
#define DNS_MX_LENGTH_OFFSET            10
#define DNS_MX_PREF_OFFSET              12
#define DNS_MX_SRVR_OFFSET              14

#define DNS_MX_FIXED_HEADER_LEN         4
#define DNS_MX_FIXED_RR_HDR_LEN         10

#define DNS_FIXED_HDR_LEN               12

#define NU_DNS_ADD      1
#define NU_DNS_DELETE   2
#define NU_DNS_UPDATE   3
#define NU_DNS_START    5
#define NU_DNS_STOP     6

typedef struct _NU_DNS_HOST
{
    CHAR        *dns_name;
    CHAR        *dns_data;
    UNSIGNED    dns_ttl;
    INT16       dns_type;
    INT16       dns_family;
    INT16       dns_prio;
    INT16       dns_weight;
    INT16       dns_port;
    UINT16      dns_data_len;
    UINT8       dns_flags;
    UINT8       dns_padN[3];
} NU_DNS_HOST;

#if (INCLUDE_MDNS == NU_TRUE)

typedef struct _MDNS_STRUCT
{
    struct _MDNS_QUERY  *mdns_query;
    NU_TIMER            *mdns_timer;
    UINT32              mdns_dev_index;
    UINT32              mdns_retrans_count;
    UINT32              mdns_expire;
    UNSIGNED            mdns_ttl_start_time;            /* The first time this record was received and added. */
    UNSIGNED            mdns_last_local_resp_time;      /* The last time this local record was sent via multicast. */
    UINT8               mdns_foreign_query_cnt;         /* The number of times this foreign query has been received.  */
    UINT8               mdns_state;
    UINT8               mdns_padN[2];
} MDNS_STRUCT;

#define mdns_query                  dns_mdns.mdns_query
#define mdns_timer                  dns_mdns.mdns_timer
#define mdns_dev_index              dns_mdns.mdns_dev_index
#define mdns_retrans_count          dns_mdns.mdns_retrans_count
#define mdns_expire                 dns_mdns.mdns_expire
#define mdns_ttl_start_time         dns_mdns.mdns_ttl_start_time
#define mdns_last_local_resp_time   dns_mdns.mdns_last_local_resp_time
#define mdns_foreign_query_cnt      dns_mdns.mdns_foreign_query_cnt
#define mdns_state                  dns_mdns.mdns_state

#endif

/* This structure is defines what a host looks like. */
typedef struct _DNS_HOST
{
    struct _DNS_HOST    *dns_next;
    struct _DNS_HOST    *dns_previous;
    UNSIGNED            dns_ttl;                        /* Time To Live for this entry.  A
                                                           value of 0 is used to indicate a
                                                           permanent entry. */
    UINT32              dns_id;
    INT                 dns_name_buffer_size;           /* The size of the name in this
                                                           entry. */
    INT                 dns_family;
    INT16               dns_type;
    UINT16              dns_h_length;                   /* The length of the dns_record_data if
                                                           family == NU_FAMILY_UNSPEC; otherwise,
                                                           the length of an individual IP address
                                                           stored in the record. */
    UINT16              dns_prio;
    UINT16              dns_weight;
    UINT16              dns_port;
    UINT8               dns_flags;
    UINT8               dns_padN[1];
#if (INCLUDE_MDNS == NU_TRUE)
    MDNS_STRUCT         dns_mdns;
#endif
    CHAR                *dns_name;                      /* Host name. */
    CHAR                *dns_record_data;               /* Data relative to the record, depending on record type. */
} DNS_HOST;

/* Define the head of the linked list of HOSTs. */
typedef struct _DNS_HOST_LIST
{
    DNS_HOST    *dns_head;
    DNS_HOST    *dns_tail;
} DNS_HOST_LIST;

typedef struct _DNS_SRV_DATA
{
    INT16   dns_srv_weight;
    INT16   dns_srv_prio;
    INT16   dns_srv_port;
    UINT8   padN[2];
} DNS_SRV_DATA;

/* Define DNS Server list structure */
typedef struct _DNS_SERVER
{
    struct _DNS_SERVER      *dnss_next;
    struct _DNS_SERVER      *dnss_previous;
    INT16                   family;
    UINT8                   padN[2];
    UINT8                   dnss_ip[MAX_ADDRESS_SIZE];
} DNS_SERVER;

typedef struct _DNS_SERVER_LIST
{
    DNS_SERVER      *dnss_head;
    DNS_SERVER      *dnss_tail;
}DNS_SERVER_LIST;

/* Function prototypes. */
STATUS    DNS_Initialize(VOID);
NU_HOSTENT *DNS_Find_Host_By_Addr(CHAR *addr, INT16 family, STATUS *error_num);
NU_HOSTENT *DNS_Find_Host_By_Name(CHAR *name, INT16 family, INT16 flags, STATUS *error_num);
STATUS    DNS_Remove_DNS_Server(DNS_SERVER *dns_server);
STATUS    NU_Add_DNS_Server2(const UINT8 *new_dns_server, INT where, INT16 family);
STATUS    NU_Delete_DNS_Server2(const UINT8 *dns_ip, INT16 family);
INT       NU_Get_DNS_Servers2(UINT8 *dest, INT size, INT16 family);
NU_HOSTENT *DNS_Create_Host_Entry(CHAR* name, INT16 family, const CHAR* ip_addr_buff,
                                  STATUS *error_num);
STATUS    DNS_Update_Host(DNS_HOST *dns_host, const CHAR* ip_addr);
VOID      DNS_Delete_Host(DNS_HOST *host_ptr);
INT       DNS_Build_Header(VOID **buffer, INT16 id, UINT16 flags);
INT       DNS_Build_Query(CHAR *data, INT16 offset, CHAR *buffer, UINT16 type,
                          INT16 family, UINT16 flags, UINT32 dest_len);
INT       DNS_Insert_Record(DNS_HOST *dns_ptr, CHAR *host_name, INT16 offset, CHAR *buffer,
                            UINT16 type, UINT16 r_class, CHAR *data, UINT32 dest_len);
INT       DNS_Unpack_Domain_Name(CHAR *, INT, CHAR *, CHAR *);
DNS_HOST  *DNS_Find_Host_By_ID(INT id);
STATUS    DNS_Resolve(CHAR *, CHAR *, UNSIGNED *, UINT16, INT16, INT16 *);
STATUS    DNS_Extract_Data(DNS_PKT_HEADER *, CHAR *, CHAR *, UNSIGNED *, UINT16, INT16, INT16 *);
INT       DNS_Query(CHAR *, INT, struct addr_struct *dns_addr);
DNS_HOST  *DNS_Find_Matching_Host_By_Addr(const UINT8 *addr, INT16 family);
VOID      DNS_Delete_Host(DNS_HOST *dns_ptr);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* DNS_H */
