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

/***************************************************************************
*
*   FILENAME
*
*       netevent.h
*
*   DESCRIPTION
*
*       This include file will define the events used in Net.
*
*   DATA STRUCTURES
*
*       TQHDR
*       TQE_T
*
*   DEPENDENCIES
*
*       None
*
***************************************************************************/

#ifndef NETEVENT_H
#define NETEVENT_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */


/* Events processed by the Events Dispatcher. */
#define CONOPEN             1   /* connection has opened, CONCLASS */
#define CONCLOSE            2   /* the other side has closed its side of the connection */
#define CONFAIL             3   /* connection open attempt has failed */
#define UNUSED_EVENT_SPOT   4   /* This used to be CONRX, but this has been removed.  */
#define CONNULL             5   /*      Just a null event...     */
                                /*      Used to wake up the dispatcher from a
                                 *      indefinite sleep should a timer queue
                                 *      entry be posted...could be used for other
                                 *      purposes also.
                                 */

#define UDPDATA             6   /* UDP data has arrived on listening port, USERCLASS */
#define TCPRETRANS          7   /* TCP segment retransmission event */
#define WINPROBE            8   /* Window Probe event. */
#define TCPACK              9   /* TCP ACK transmission event */
#define CONTX               10  /* buffer needs to be sent. */
#define SELECT              11  /* TCP Select timeout event. */
#define ARPRESOLVE          12  /* ARP event. */
#define RARP_REQUEST        13  /* A RARP request event. */

/* NOTE: events removed here. i.e. 14 through 24 are now available
         for use.
 */

#define EV_IP_REASSEMBLY    25  /* Timeout the reassembly of an IP datagram. */
#if (INCLUDE_IPV6 == NU_TRUE)
#define EV_IP6_REASSEMBLY   23
#endif

/* ICMP events */
#define ICMP_ECHO_TIMEOUT   26  /* Timeout waiting for a ping reply. */

#define IPDATA              27  /* IP data has arrived */
#define TCPCLOSETIMEOUTSFW2 28  /* Timeout the closing of a connection. */
#define TCPTIMEWAIT         29

/* DHCP events. */
#define DHCP_RENEW          30
#define DHCP_REBIND         31
#define DHCP_NEW_LEASE      32

/* NAT events */
#define NAT_CLEANUP_TCP     33
#define NAT_CLEANUP_UDP     34
#define NAT_TIMEOUT         35
#define NAT_CLOSE           36
#define NAT_CLEANUP_ICMP    37

/* MEMORY events */
#define MEM_RESUME              52

#define NU_CLEAR       0
#define NU_SET         1


/* Define the maximum number of events that can be registered with
   the Events Dispatcher. Remember that these events can be
   registered and unregistered dynamically, so the number doesn't
   need to be extremely high. */
#define EQ_MAX_EVENTS           64

/* Define an offset to the first registered event index. This is for
   backward compatibility with Net products that still use event
   constants. */
#define EQ_FIRST_REG_EVENT      64
#define EQ_LAST_REG_EVENT       (EQ_FIRST_REG_EVENT + EQ_MAX_EVENTS - 1)

/* Methods in which a timer event can be removed from the TQ list. */
#define TQ_CLEAR_ALL            1   /* All instances of event. Data not checked. */
#define TQ_CLEAR_ALL_EXTRA      2   /* Event and data must match. */
#define TQ_CLEAR_EXACT          3   /* Event, data, and extra data must match. */
#define TQ_CLEAR_SEQ            4   /* Event based on TCP sequence number. */

/* The following macro increases the ID counter to allow for
   structure verification */
extern UINT32  EQ_ID_Counter;
#define EQ_ID_VALUE             ++EQ_ID_Counter?EQ_ID_Counter:++EQ_ID_Counter

typedef UNSIGNED                   TQ_EVENT;
typedef VOID (*EQ_Handler)(TQ_EVENT, UNSIGNED, UNSIGNED);

struct tqhdr
{
    struct tqe      *flink, *blink;
};

/* Define a timer queue element for TCP and IP timer events */
struct tqe
{
    struct tqe      *flink,
                    *blink;
    struct tqhdr    duplist;
    TQ_EVENT        tqe_event;
    UNSIGNED        tqe_data;
    UNSIGNED        duetime;
    UNSIGNED        tqe_ext_data;
    UINT32          tqe_id;
};
typedef struct      tqe tqe_t;

/* External References */
extern struct tqhdr EQ_Event_Freelist;
extern struct tqhdr EQ_Event_List;

/* tq.c -- new double link list for the timer stuff, and will be used
   for the defragmentation later */
STATUS              TQ_Post(tqe_t *tqlist, tqe_t *tqe);
STATUS              EQ_Clear_Matching_Timer(struct tqhdr *, TQ_EVENT, UNSIGNED, INT16, UNSIGNED);
STATUS              EQ_Register_Event(EQ_Handler handler, TQ_EVENT *newevt);
STATUS              EQ_Unregister_Event(TQ_EVENT index);


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* NETEVENT_H */
