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

/************************************************************************
*
*   FILE NAME
*
*       tcp.h
*
*   COMPONENT
*
*       TCP - Transmission Control Protocol
*
*   DESCRIPTION
*
*       Holds the defines for the TCP protocol.
*
*   DATA STRUCTURES
*
*       TCPLAYER
*       TCP_WINDOW
*       TCP_PORT
*
*   DEPENDENCIES
*
*       ip.h
*
*************************************************************************/

#ifndef TCP_H
#define TCP_H

#include "networking/ip.h"
#include "networking/netevent.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/ip6.h"
#endif

#if (INCLUDE_IPSEC == NU_TRUE)
#include "networking/ips_api.h"
#endif

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define DATA           1
#define ACK            2

#define TCP_RTT_SHIFT       3
#define TCP_RTTVAR_SHIFT    2

#define TCP_HEADER_LEN      20

/*****************************************************************************/
/* IN_CLASSA,  IN_CLASSB, IN_CLASSC                                          */
/*                                                                           */
/* These macros are used to determine the class of an IP address.  Note that */
/* they will only work if i is unsigned.  The class of the address is        */
/* determined by the first two bits of the IP address.                       */
/*                                                                           */
/*****************************************************************************/
#define IN_CLASSA(i)        (((long)(i) & 0x80000000) == 0)
#define IN_CLASSA_NET       0xff000000
#define IN_CLASSA_NSHIFT    24
#define IN_CLASSA_HOST      0x00ffffff
#define IN_CLASSA_MAX       128

#define IN_CLASSB(i)        (((long)(i) & 0xc0000000) == 0x80000000)
#define IN_CLASSB_NET       0xffff0000
#define IN_CLASSB_NSHIFT    16
#define IN_CLASSB_HOST      0x0000ffff
#define IN_CLASSB_MAX       65536

#define IN_CLASSC(i)        (((long)(i) & 0xe0000000) == 0xc0000000)
#define IN_CLASSC_NET       0xffffff00
#define IN_CLASSC_NSHIFT    8
#define IN_CLASSC_HOST      0x000000ff

#define IN_CLASSD(i)        (((long)(i) & 0xf0000000) == 0xe0000000)
#define IN_CLASSD_NET       0xf0000000  /* These ones aren't really */
#define IN_CLASSD_NSHIFT    28          /* net and host fields, but */
#define IN_CLASSD_HOST      0x0fffffff  /* routing needn't know.    */
#define IN_MULTICAST(i)     IN_CLASSD(i)

#define IN_EXPERIMENTAL(i)  (((long)(i) & 0xf0000000) == 0xf0000000)
#define IN_BADCLASS(i)      (((long)(i) & 0xf0000000) == 0xf0000000)

/**************************************************************************/
/*  TCP protocol
*     define both headers required and create a data type for a typical
*     outgoing TCP packet (with IP header)
*
*  Note:  So far, there is no way to handle IP options fields
*   which are associated with a TCP packet.  They are mutually exclusive
*   for both receiving and sending.  Support may be added later.
*
*   The tcph and iph structures can be included in many different types of
*   arbitrary data structures and will be the basis for generic send and
*   receive subroutines later.  For now, the packet structures are optimized
*   for packets with no options fields.  (seems to be almost all of them from
*   what I've observed.
*/

typedef struct _tcp_buffer
{
    struct _tcp_buffer  *tcp_next;
    struct _tcp_buffer  *tcp_previous;
    NET_BUFFER          *tcp_buf_ptr;
} TCP_BUFFER;

typedef struct _tcp_buffer_list
{
    struct _tcp_buffer  *tcp_head;
    struct _tcp_buffer  *tcp_tail;
} TCP_BUFFER_LIST;

typedef struct tcph
{
    UINT16 tcp_src;             /* TCP Source port number */
    UINT16 tcp_dest;            /* TCP destination port number */
    UINT32 tcp_seq;             /* TCP sequence number */
    UINT32 tcp_ack;             /* TCP Acknowledgment number */
    UINT8  tcp_hlen;            /* length of TCP header in 4 byte words */
    UINT8  tcp_flags;           /* flag fields */
    UINT16 tcp_window;          /* advertised window, byte-swapped */
    UINT16 tcp_check;           /* TCP checksum of whole packet */
    UINT16 tcp_urgent;          /* urgent pointer, when flag is set */
} TCPLAYER;


struct _TCP_Window
{
    UINT32              nxt;                  /* sequence number, not byte-swapped */
    UINT32              ack;                  /* what the other machine acked */
    UNSIGNED            lasttime;             /* (signed) used for timeout checking */
    TCP_BUFFER_LIST     packet_list;
    NET_BUFFER          *nextPacket;
    NET_BUFFER_HEADER   ooo_list;             /* Contains out of order packets. */
    UINT32              size;                 /* size of window advertised */
    INT32               contain;              /* how many bytes in queue? */
    UINT16              port;                 /* port numbers from one host or another */
    UINT16              num_packets;
    UINT8               p_win_shift;          /* The window scale factor. */
    UINT8               push;                 /* flag for TCP push */
    UINT8               tcp_flags;
    UINT8               pad[1];               /* correct alignment for 32 bits CPU */
};

typedef struct _TCP_Window TCP_WINDOW;

struct _TCP_Port
{
    TCP_WINDOW      in, out;

    union
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        UINT32      tcp_laddr_v4;      /* Local IP address. */
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        UINT8       tcp_laddr_v6[16];
#endif
    } tcp_local_addr;

    union
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        UINT32      tcp_faddr_v4;      /* Foreign IP address. */
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        UINT8       tcp_faddr_v6[16];
#endif
    } tcp_foreign_addr;

    UINT32          maxSendWin;     /* Max send window. */

#if (INCLUDE_IPSEC == NU_TRUE)

    /* The following variable specifies the policy group which is
     * being used for the TCP port. During a connection this may
     * change when the underlying interface changes.
     */
    IPSEC_POLICY_GROUP          *tp_ips_group;

    /* The following pointer is used to specify the IPsec policy that
     * is used for all inbound packets on this port. This pointer
     * will need to be updated whenever an interface for the TCP port
     * changes.
     */
    IPSEC_POLICY                *tp_ips_in_policy;

    /* The following pointer is used to specify the IPsec policy that
     * is used for all outbound packets on this port. This pointer
     * will need to be updated whenever an interface for the TCP port
     * changes.
     */
    IPSEC_POLICY                *tp_ips_out_policy;

    /* The following pointer is used to specify the SA bundle that
     * will be applied for all packets that are sent from this port.
     * This pointer will need to be updated whenever an interface
     * for the TCP port changes.
     */
    IPSEC_OUTBOUND_BUNDLE       *tp_ips_out_bundle;

#endif

    union
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        RTAB_ROUTE      tp_route_v4;       /* A cached route. */
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        RTAB6_ROUTE     tcp_route_v6;
#endif
    } tcp_route;

    UINT16  sendsize;           /* MTU value for this connection */
    UINT16  p_smss;             /* The largest segment the sender can transmit */
    UINT16  p_ttl;              /* The ttl associated with the socket associated
                                   with the port. */
    UINT16  pindex;             /* added by Randy */
    UINT16  p_opt_len;
    UINT16  p_16_pad[1];

    UINT32  credit;             /* choked-down window for fast hosts */
    UINT32  portFlags;

#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)
    UINT32  p_cwnd;             /* Congestion window */
    UINT32  p_ssthresh;         /* Slow Start Threshold */
#endif
    UINT32  p_rtseq;            /* Sequence # of packet being timed. */
    UINT32  p_msl;              /* MSL for the connection */
    UINT32  p_delay_ack;        /* Amount of time to delay sending ACKs */
    UINT32  p_ka_wait;          /* Amount of time to delay keep-alives */
    UINT32  p_keepalive_intvl;  /* The time interval between keep alive probes */
#if (INCLUDE_NEWRENO == NU_TRUE)
    UINT32  p_recover;
    UINT32  p_prev_highest_ack;
#endif
#if (NET_INCLUDE_SACK == NU_TRUE)
    UINT32  left_edge;
    UINT32  right_edge;
#endif
    INT32   p_rto;              /* retrans timeout */
    INT32   p_srtt;
    INT32   p_rttvar;
    INT32   p_probe_to;         /* Timeout value for successive Window probes */
    INT32   p_first_probe_to;   /* Timeout value for the first Zero Window probe */
    INT32   p_max_probe_to;     /* Max timeout value for successive Zero Window probes */
    INT32   p_first_rto;        /* Timeout value for first retransmission */
    UINT32  p_max_rto;          /* Max timeout value for successive timeouts */
    UINT32  p_rtt;
    UINT32  p_tsecr;            /* The outgoing value for tsecr in the timestamp option */

    INT     p_socketd;          /* The socket associated with this port. */

#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)
    UINT8   p_dupacks;          /* Duplicate ACKs */
#else
    UINT8   p_dupacks_pad[1];
#endif
    UINT8   state;              /* connection state */
    UINT8   p_max_probes;       /* Max number of Zero Window probes to send */
    UINT8   p_probe_count;      /* Number of Zero Window probes transmitted */
    UINT8   p_max_r2;           /* Max number of retransmissions before closing connection */
    UINT8   p_max_syn_r2;       /* Max number of SYN retransmissions */
    UINT8   p_ka_r2;            /* Number of keep-alive retransmissions */
    INT8    xmitFlag;           /* Used to indicate an timer event has been
                                   created to transmit a partial packet. */
    INT8    probeFlag;          /* This flag will be set when a window probe
                                   is required. */
    INT8    selectFlag;         /* This flag is set whenever a call to
                                   NU_Select results in a task timing out. */
    UINT8   p_tos;
    UINT8   pad[1];

#if (INCLUDE_IPV6 == NU_TRUE)
    tx_ancillary_data       *p_sticky_options;
#endif
};

#if (INCLUDE_IPV4 == NU_TRUE)
#define tcp_laddrv4 tcp_local_addr.tcp_laddr_v4
#define tcp_faddrv4 tcp_foreign_addr.tcp_faddr_v4
#define tp_route    tcp_route.tp_route_v4
#else
#define tp_route    tcp_route.tcp_route_v6
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
#define tcp_laddrv6 tcp_local_addr.tcp_laddr_v6
#define tcp_faddrv6 tcp_foreign_addr.tcp_faddr_v6
#define tp_routev6  tcp_route.tcp_route_v6
#endif

typedef struct _TCP_Port TCP_PORT;


/*
*  flag field definitions, first two bits undefined
*/

#define TURG    0x20
#define TACK    0x10
#define TPUSH   0x08
#define TRESET  0x04
#define TSYN    0x02
#define TFIN    0x01


/***************************************************************************/
/*  Port Flags                                                             */
/*                                                                         */
#define ACK_TIMER_SET           0x00001
#define SELECT_SET              0x00002
#define TCP_FAMILY_IPV6         0x00004
#define TCP_FAMILY_IPV4         0x00008
#define TCP_KEEPALIVE           0x00010
#define TCP_HALFDPLX_CLOSE      0x00020
#define TCP_DIS_CONGESTION      0x00040     /* Congestion Control is disabled on the port. */
#define TCP_SACK                0x00080     /* SACK support was enabled by the application. */
#define TCP_FOREIGN_SACK        0x00100     /* The foreign side supports SACK. */
#define TCP_REPORT_SACK         0x00200     /* Include a SACK in the ACK. */
#define TCP_RTX_FIRED           0x00400     /* Indicates an initial retrans has gone out. */
#define TCP_DSACK               0x00800     /* DSACK support was enabled by the application. */
#define TCP_REPORT_DSACK        0x01000     /* Include a D-SACK in the ACK. */
#define TCP_REPORT_WINDOWSCALE  0x02000     /* Include a Window Scale option. */
#define TCP_FOREIGN_WINDOWSCALE 0x04000     /* The foreign side supports Window Scale. */
#define TCP_REPORT_TIMESTAMP    0x08000     /* Include a Timestamp option. */
#define TCP_FOREIGN_TIMESTAMP   0x10000     /* The foreign side supports Timestamp. */
#define TCP_TX_LMTD_DATA        0x20000     /* The client can send 1 segment regardless of cwnd. */
#define TCP_NEW_SACK_DATA       0x40000     /* The most recent packet contains new SACK data. */

/*************************************************************************/
/*  TCP states
*    each connection has an associated state in the connection flow.
*    the order of the states now matters, those less than a certain
*    number are the "inactive" states.
*/
#define SREADY          0
#define SCLOSED         1
#define SLISTEN         2
#define SSYNR           3
#define SSYNS           4
#define SEST            5
#define SFW1            6
#define SFW2            7
#define SCLOSING        8
#define STWAIT          9
#define SCWAIT          10
#define SLAST           11

#define TCP_SRC_OFFSET              0
#define TCP_DEST_OFFSET             2
#define TCP_SEQ_OFFSET              4
#define TCP_ACK_OFFSET              8
#define TCP_HLEN_OFFSET             12
#define TCP_FLAGS_OFFSET            13
#define TCP_WINDOW_OFFSET           14
#define TCP_CHECK_OFFSET            16
#define TCP_URGENT_OFFSET           18

/* Offsets for MSS Option */
#define TCP_MSS_KIND_OFFSET         0
#define TCP_MSS_LEN_OFFSET          1
#define TCP_MSS_VALUE_OFFSET        2

/* Offsets for NOP Option. */
#define TCP_MSS_NOP_OFFSET          0

/* Offsets for SACK-Permitted Option */
#define TCP_SACK_PERM_KIND_OFFSET   0
#define TCP_SACK_PERM_LEN_OFFSET    1

/* Offsets for SACK Option */
#define TCP_SACK_KIND_OFFSET        0
#define TCP_SACK_LEN_OFFSET         1
#define TCP_SACK_BLOCK_OFFSET       2

/* Offsets for Blocks within the SACK Option. */
#define TCP_SACK_BLOCK_LEFT_EDGE    0
#define TCP_SACK_BLOCK_RIGHT_EDGE   4

/* Offsets for Window Scale Option. */
#define TCP_WINDOWSCALE_KIND_OFFSET     0
#define TCP_WINDOWSCALE_LEN_OFFSET      1
#define TCP_WINDOWSCALE_SHIFT_OFFSET    2

/* Offsets for Timestamp Option. */
#define TCP_TIMESTAMP_KIND_OFFSET       0
#define TCP_TIMESTAMP_LEN_OFFSET        1
#define TCP_TIMESTAMP_TSVAL_OFFSET      2
#define TCP_TIMESTAMP_TSECR_OFFSET      6

/* Option Lengths */
#define TCP_TOTAL_OPT_LENGTH        40
#define TCP_NOP_LENGTH              1
#define TCP_MSS_LENGTH              4
#define TCP_SACK_PERM_LENGTH        2
#define TCP_SACK_LENGTH             2
#define TCP_SACK_BLOCK_LENGTH       8
#define TCP_WINDOWSCALE_LENGTH      3
#define TCP_TIMESTAMP_LENGTH        10

/* TCP Option Definitions */
#define TCP_NOP_OPT                 1
#define TCP_MSS_OPT                 2
#define TCP_WINDOWSCALE_OPT         3
#define TCP_SACK_PERM_OPT           4
#define TCP_SACK_OPT                5
#define TCP_TIMESTAMP_OPT           8

/* Define return codes for internal use by TCP. */
#define TCP_NO_ACK                  -255
#define TCP_INVALID_ACK             -254

/*
 * Options for use with [gs]etsockopt at the TCP level.
 */
#define TCP_NODELAY                 1
#define SO_KEEPALIVE                2
#define TCP_FIRST_PROBE_TIMEOUT     3
#define TCP_PROBE_TIMEOUT           4
#define TCP_MAX_PROBES              5
#define TCP_MSL                     6
#define TCP_FIRST_RTO               7
#define TCP_MAX_RTO                 8
#define TCP_MAX_R2                  9
#define TCP_MAX_SYN_R2              10
#define TCP_DELAY_ACK               11
#define TCP_KEEPALIVE_WAIT          12
#define TCP_KEEPALIVE_R2            13
#define TCP_CONGESTION_CTRL         14
#define TCP_CFG_SACK                15
#define TCP_CFG_DSACK               16
#define TCP_WINDOWSCALE             17
#define TCP_SND_WINDOWSIZE          18
#define TCP_RCV_WINDOWSIZE          19
#define TCP_TIMESTAMP               20
#define TCP_KEEPINTVL               21

#define FOREIGN_MAX_SEGMENT_LEN  536

/* Make sure the sequence number is within the window space. The first pair
   of checks handles the case where the sequence number is the next one
   expected and where the sequence number is greater than expected but
   still within the receive window. The second pair of checks handles the
   case where the sequence number is less than expected but contains at
   least some data that falls within the receive window, i.e.,
   unacknowledged data. */
#define TCP_Valid_Seq(seq, dlen, expected, wsize)        \
    (( ((INT32_CMP(seq, expected) >= 0) &&               \
        (INT32_CMP(seq, expected + wsize) <= 0)) ||      \
        ((INT32_CMP(seq, expected) < 0) &&               \
        (INT32_CMP(seq + dlen, expected) >= 0)) ) ? NU_TRUE : NU_FALSE)

#define TCP_24_DAYS (24 * 60 * 60 * TICKS_PER_SECOND)

/* TCP function prototypes. */
VOID    TCP_Init(VOID);
INT16   TCP_Interpret (NET_BUFFER *buf_ptr, VOID *tcp_chk,
                       INT16 family, UINT16 myport, UINT16 hlen);
INT16   TCP_Update_Headers (TCP_PORT *, NET_BUFFER *, UINT16);
VOID    TCP_Retransmit(TCP_PORT *);
VOID    TCP_ACK_It(TCP_PORT *prt, INT force);
STATUS  TCP_Xmit(TCP_PORT *, NET_BUFFER *);
STATUS  TCP_Cleanup(TCP_PORT *prt);
INT     TCP_Make_Port (INT16, UINT16);
STATUS  TCP_Set_Opt(INT socketd, INT optname, const VOID *optval, INT optlen);
STATUS  TCP_Get_Opt(INT socketd, INT optname, VOID *optval, INT *optlen);
INT16   TCP_Do(VOID *, NET_BUFFER *, UINT16, VOID *, INT16);
STATUS  TCP_Handle_Datagram_Error(INT16, const NET_BUFFER *, const UINT8 *,
                                  const UINT8 *, INT32);
STATUS  TCP_Xmit_Probe(TCP_PORT *);
INT16   TCP_Send_ACK(TCP_PORT *pport);
UINT8   TCP_Configure_Shift_Count(UINT32 window_size);
UINT8   *TCP_Find_Option(UINT8 *, UINT8, UINT8);

/***** TCPSS.C *****/

INT32   TCPSS_Recv_Data(INT, CHAR *, UINT16);
INT32   TCPSS_Send_Data(INT socketd, CHAR *buff, UINT16 nbytes);
STATUS  TCPSS_Net_Listen(UINT16 serv, const VOID *, INT16);
STATUS  TCPSS_Net_Xopen (const UINT8 *machine, INT16 family, UINT16 service,
                         INT socketd);
STATUS  TCPSS_Send_SYN_FIN (TCP_PORT *);
STATUS  TCPSS_Net_Close (INT, struct sock_struct *);
STATUS  TCPSS_Half_Close (struct sock_struct *);
VOID    TCPSS_Send_Window_Probe(UINT16 pindex);

/***/

/***** TOOLS.C *****/
INT32   TCP_Dequeue (struct sock_struct *, CHAR *, UINT32);

/***** TCP_SND.C *****/
STATUS  TCP_Setsockopt_TCP_NODELAY(INT, UINT8);

/***** TCP_GND.C *****/
STATUS TCP_Getsockopt_TCP_NODELAY(INT socketd, INT *, INT *);

/***** TCP_SKA.C *****/
STATUS TCP_Setsockopt_SO_KEEPALIVE(INT socketd, UINT8 opt_val);

/***** TCP_GKA.C *****/
STATUS TCP_Getsockopt_SO_KEEPALIVE(INT socketd, INT *, INT *);

/***** TCP_KA.C *****/
VOID   TCP_Keep_Alive(TQ_EVENT, UNSIGNED, UNSIGNED);

/***** TCP_PMTU.C *****/
VOID   TCP_PMTU_Repacket(TCP_PORT *);
VOID   TCP_PMTU_Increase_SMSS(VOID);

/***** TCP_SFPT.C *****/
STATUS TCP_Setsockopt_TCP_FIRST_PROBE_TIMEOUT(INT, INT32);

/***** TCP_GFPT.C *****/
STATUS TCP_Getsockopt_TCP_FIRST_PROBE_TIMEOUT(INT, INT32 *);

/***** TCP_SPT.C *****/
STATUS TCP_Setsockopt_TCP_PROBE_TIMEOUT(INT socketd, INT32 timeout);

/***** TCP_GPT.C *****/
STATUS TCP_Getsockopt_TCP_PROBE_TIMEOUT(INT socketd, INT32 *timeout);

/***** TCP_SMP.C *****/
STATUS TCP_Setsockopt_TCP_MAX_PROBES(INT socketd, UINT8 max_probes);

/***** TCP_GMP.C *****/
STATUS TCP_Getsockopt_TCP_MAX_PROBES(INT socketd, UINT8 *max_probes);

/***** TCP_SMSL.C *****/
STATUS TCP_Setsockopt_TCP_MSL(INT socketd, UINT32 msl);

/***** TCP_GMSL.C *****/
STATUS TCP_Getsockopt_TCP_MSL(INT socketd, UINT32 *msl);

/***** TCP_SFRTO.C *****/
STATUS TCP_Setsockopt_TCP_FIRST_RTO(INT socketd, INT32 timeout);

/***** TCP_GFRTO.C *****/
STATUS TCP_Getsockopt_TCP_FIRST_RTO(INT socketd, INT32 *timeout);

/***** TCP_SMRTO.C *****/
STATUS TCP_Setsockopt_TCP_MAX_RTO(INT socketd, UINT32 timeout);

/***** TCP_GMRTO.C *****/
STATUS TCP_Getsockopt_TCP_MAX_RTO(INT socketd, UINT32 *timeout);

/***** TCP_SMR2.C *****/
STATUS TCP_Setsockopt_TCP_MAX_R2(INT socketd, UINT8 max_retrans);

/***** TCP_GMR2.C *****/
STATUS TCP_Getsockopt_TCP_MAX_R2(INT socketd, UINT8 *max_retrans);

/***** TCP_SDA.C *****/
STATUS TCP_Setsockopt_TCP_DELAY_ACK(INT socketd, UINT32 delay);

/***** TCP_GDA.C *****/
STATUS TCP_Getsockopt_TCP_DELAY_ACK(INT socketd, UINT32 *delay);

/***** TCP_SKAW.C *****/
STATUS TCP_Setsockopt_TCP_KEEPALIVE_WAIT(INT socketd, UINT32 delay);

/***** TCP_GKAW.C *****/
STATUS TCP_Getsockopt_TCP_KEEPALIVE_WAIT(INT socketd, UINT32 *delay);

/***** TCP_SKAR2.C *****/
STATUS TCP_Setsockopt_TCP_KEEPALIVE_R2(INT socketd, UINT8 max_retrans);

/***** TCP_GKAR2.C *****/
STATUS TCP_Getsockopt_TCP_KEEPALIVE_R2(INT socketd, UINT8 *max_retrans);

/***** TCP_SMSR2.C *****/
STATUS TCP_Setsockopt_TCP_MAX_SYN_R2(INT socketd, UINT8 max_retrans);

/***** TCP_GMSR2.C *****/
STATUS TCP_Getsockopt_TCP_MAX_SYN_R2(INT socketd, UINT8 *max_retrans);

/***** TCP_GCC.C *****/
STATUS TCP_Getsockopt_TCP_CONGESTION_CTRL(INT socketd, UINT8 *optval);

/***** TCP_SCC.C *****/
STATUS TCP_Setsockopt_TCP_CONGESTION_CTRL(INT socketd, UINT8 opt_val);

/***** TCP_GCSACK.C *****/
STATUS TCP_Getsockopt_TCP_CFG_SACK(INT socketd, UINT8 *optval);

/***** TCP_SCSACK.C *****/
STATUS TCP_Setsockopt_TCP_CFG_SACK(INT socketd, UINT8 opt_val);

/***** TCP_SACK.C *****/
UINT8 TCP_SACK_Flag_Packets(UINT8 *buffer, TCP_PORT *prt);
NET_BUFFER *TCP_Find_Continuous_SACK_Block(NET_BUFFER *, UINT32 *);
VOID TCP_Build_SACK_Block(UINT8 *, TCP_PORT *, UINT8 *, UINT8, UINT32, UINT32);

/***** TCP_GCDSACK.C *****/
STATUS TCP_Getsockopt_TCP_CFG_DSACK(INT socketd, UINT8 *optval);

/***** TCP_SCDSACK.C *****/
STATUS TCP_Setsockopt_TCP_CFG_DSACK(INT socketd, UINT8 opt_val);

/***** TCP_GWS.C *****/
STATUS TCP_Getsockopt_TCP_WINDOWSCALE(INT socketd, INT *optval);

/***** TCP_SWS.C *****/
STATUS TCP_Setsockopt_TCP_WINDOWSCALE(INT socketd, INT opt_val);

/***** TCP_BUILD_OPTS.C *****/
UINT8 TCP_Build_MSS_Option(UINT8 *buffer, UINT16 mss, UINT8 *bytes_avail);
UINT8 TCP_Build_SACK_Perm_Option(UINT8 *buffer, UINT8 *bytes_avail);
UINT8 TCP_Build_SACK_Option(UINT8 *buffer, TCP_PORT *prt, UINT8 *bytes_avail);
UINT8 TCP_Build_DSACK_Option(UINT8 *buffer, TCP_PORT *prt, UINT8 *bytes_avail);
UINT8 TCP_Build_WindowScale_Option(UINT8 *buffer, TCP_PORT *prt, UINT8 *bytes_avail);
UINT8 TCP_Build_Timestamp_Option(UINT8 *buffer, TCP_PORT *prt, UINT8 *bytes_avail);

/***** TCP_SSWS.C *****/
STATUS TCP_Setsockopt_TCP_RCV_WINDOWSIZE(INT socketd, UINT32 opt_val);

/***** TCP_GSWS.C *****/
STATUS TCP_Getsockopt_TCP_RCV_WINDOWSIZE(INT socketd, UINT32 *optval);

/***** TCP_GRWS.C *****/
STATUS TCP_Getsockopt_TCP_SND_WINDOWSIZE(INT socketd, UINT32 *optval);

/***** TCP_GTS.C *****/
STATUS TCP_Getsockopt_TCP_TIMESTAMP(INT socketd, UINT8 *optval);

/***** TCP_STS.C *****/
STATUS TCP_Setsockopt_TCP_TIMESTAMP(INT socketd, UINT8 opt_val);

/***** tcp_skai.c *****/
STATUS TCP_Setsockopt_TCP_KEEPINTVL(INT socketd, UINT32 interval);

/***** tcp_gkai.c *****/
STATUS TCP_Getsockopt_TCP_KEEPINTVL(INT socketd, UINT32 *interval);

/* External references */
extern struct _TCP_Port *TCP_Ports[TCP_MAX_PORTS];

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif /* TCP_H */
