/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME
*                                                                                 
*       net6_cfg.h
*
*   DESCRIPTION
*
*       This file holds all defines that control the various configuration 
*       settings of Nucleus IPv6.
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
*       None.
*
*************************************************************************/
#ifndef NET6_CFG_H
#define NET6_CFG_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define     IPV6_1_1            1       /* IPv6 1.1 */
#define     IPV6_1_2            2       /* IPv6 1.2 */
#define     IPV6_1_3            3       /* IPv6 1.3 */
#define     IPV6_1_4            4       /* IPv6 1.4 */

#define IPV6_VERSION_COMP    IPV6_1_4      /* The version for which compatibility
                                              is desired. */

/* Check that Multicast is enabled. */
#ifdef CFG_NU_OS_NET_IPV6_ENABLE
#if (CFG_NU_OS_NET_STACK_INCLUDE_IP_MULT != NU_TRUE)
#error "Multi-cast support must be enabled when using IPv6.  Multi-cast support is enabled by setting nu.os.net.stack.include_ip_multi=true within the UI configurator tool or via a configuration file."
#endif
#endif

/********************** Feature Set Configuration ***********************/

/*  The following macros control which network protocols and features will be 
 *  included in the build of the Nucleus IPv6 stack.
 */

/* Set this to NU_FALSE to reduce the final footprint if the node is operating 
 * as an IPv6 host only. 
 */
#if (CFG_NU_OS_NET_IPV6_INCLUDE_ROUTER_SPT)
#define INCLUDE_IPV6_ROUTER_SUPPORT     NU_TRUE
#else
#define INCLUDE_IPV6_ROUTER_SUPPORT     NU_FALSE
#endif

/* Set this to NU_TRUE to include DHCPv6 client support on the node. */
#if (CFG_NU_OS_NET_IPV6_INCLUDE_DHCP6)
#define INCLUDE_DHCP6                   NU_TRUE
#else
#define INCLUDE_DHCP6                   NU_FALSE
#endif

/* Set this value to NU_FALSE to disable Duplicate Address Detection in the 
 * system.  If enabled, Duplicate Address Detection will insure that all IPv6 
 * addresses assigned to IPv6-enabled devices are unique.
 */  
#if (CFG_NU_OS_NET_IPV6_INCLUDE_DAD6)
#define INCLUDE_DAD6                    NU_TRUE
#else
#define INCLUDE_DAD6                    NU_FALSE
#endif

/* Set this macro to NU_TRUE to include IPv6 MIBs. */
#define INCLUDE_IPV6_MIB                NU_FALSE

/****************** End Feature Set Configuration ***********************/


/* Nucleus IPv6 relies on several tasks to perform its duties. 
 * The settings for the creation of these tasks are defined below. 
 */

/* The RIPng task is responsible for processing all RIPng packets
 * The settings for this task are defined here 
 */

/* The maximum number of routes to put in a RIP-II packet.
 *
 *             +-                                                   -+
 *             | MTU - sizeof(IPv6_hdrs) - UDP_hdrlen - RIPng_hdrlen |
 * #RTEs = INT | --------------------------------------------------- |
 *             |                      RTE_size                       |
 *             +-                                                   -+
 */
#define RIPNG_MAX_PER_PACKET        25

/* The size of the transmit / receive buffer for sending and receiving
 * RIPng packets.
 */
#define RIPNG_BUFFER_SIZE           ((RIPNG_MAX_PER_PACKET * RIPNG_RTE_LEN) + RIPNG_HEADER_LEN)

#define RIPNG_STACK_SIZE            (3000 + (RIPNG_BUFFER_SIZE))
#define RIPNG_PRIORITY              20
#define RIPNG_TIME_SLICE            0
#define RIPNG_PREEMPT               NU_PREEMPT

/* The DHCPv6 Client task is responsible for processing all incoming DHCPv6
 * Server messages and transmitting all outgoing DHCPv6 Client messages.
 */

#define DHCP6_CLIENT_RX_TASK_SIZE           CFG_NU_OS_NET_IPV6_DHCP6_RX_STACK_SIZE
#define DHCP6_CLIENT_RX_TASK_PRIORITY       3
#define DHCP6_CLIENT_RX_TASK_SLICE          0
#define DHCP6_CLIENT_RX_TASK_PREEMPT        NU_PREEMPT

/* The DHCPv6 event task is responsible for handling all DHCPv6 Client 
 * events that are triggered in the system.
 */

#define DHCP6_CLIENT_EVENT_TASK_SIZE        CFG_NU_OS_NET_IPV6_DHCP6_EVT_STACK_SIZE
#define DHCP6_CLIENT_EVENT_TASK_PRIORITY    3
#define DHCP6_CLIENT_EVENT_TASK_SLICE       0
#define DHCP6_CLIENT_EVENT_TASK_PREEMPT     NU_PREEMPT

#define DHCP6_EVENT_Q_NUM_ELEMENTS          10


/***************** Nucleus IPv6 Configuration Section *******************/

/*  In this section can be found various timeouts and size limitations used
 *  by the IPv6 product.
 */

/************************** DAD ******************************************
 *
 */

/* RFC 2462 - section 5.1 - The number of Duplicate Address Detection 
 * packets to transmit when performing DAD on an address.
 */
#define DAD6_TRANS_COUNT                1


/************************** ICMPv6 ***************************************
 *
 */

/* RFC 2462 - section 5.5.3.e.1 - The default Valid Lifetime for a Prefix */
#define IP6_DEFAULT_VALID_LIFETIME      (2 * 60 * 60 * SCK_Ticks_Per_Second) /* 2 hours */

/* The default timeout value used when sending an ICMPv6 Echo Request */
#define ICMP6_DEFAULT_ECHO_TIMEOUT      (5 * SCK_Ticks_Per_Second)

/* Protocol Constants per RFC 2461 - these constants may be overridden 
 * by specific documents that describe how IPv6 operates over different 
 * link layers.
 */

/* Host Constants */

/* RFC 2461 - section 10 - The maximum delay before starting Router 
 * Discovery during Stateless Address Autoconfiguration.
 */
#define IP6_MAX_RTR_SOLICITATION_DELAY     (1 * SCK_Ticks_Per_Second)

/* RFC 2461 - section 10 - The maximum delay between Router Solicitation 
 * messages when performing Router Discovery during Stateless Address 
 * Autoconfiguration.
 */
#define IP6_MAX_RTR_SOLICITATION_INTERVAL  (4 * SCK_Ticks_Per_Second)

/* RFC 2461 - section 10 - The maximum number of Router Solicitation 
 * messages to transmit when performing Router Discovery during Stateless 
 * Address Autoconfiguration.
 */
#define IP6_MAX_RTR_SOLICITATIONS          3                        

/* Node Constants */

/* RFC 2461 - section 10 - The maximum number of Neighbor Solicitation 
 * messages to be transmitted when performing Neighbor Discovery.
 */
#define IP6_MAX_MULTICAST_SOLICIT          3                        

/* RFC 2461 - section 10 - The maximum number of Neighbor Solicitation 
 * messages to be transmitted when performing Neighbor Unreachability 
 * Detection on a unicast destination address.
 */
#define IP6_MAX_UNICAST_SOLICIT            3                        

/* RFC 2461 - section 10 - The default number of seconds, after which, 
 * if communication with a node has not occurred, the node should be 
 * considered unreachable.
 */
#define IP6_REACHABLE_TIME                 (30 * SCK_Ticks_Per_Second)

/* The maximum value of the Reachable Time */
#define IP6_MAX_REACHABLE_TIME             3600000UL

/* RFC 2461 - section 10 - The number of seconds to wait before 
 * retransmitting a Neighbor Discovery message.
 */
#define IP6_RETRANS_TIMER                  (1 * SCK_Ticks_Per_Second)

/* RFC 2461 - section 10 - The amount of time to delay before transmitting 
 * the first Neighbor Unreachability Detection probe.
 */
#define IP6_DELAY_FIRST_PROBE_TIME         (5 * SCK_Ticks_Per_Second)

/* RFC 2461 - section 10 - When computing a random value for Neighbor 
 * Discovery, the minimum allowable value of that random value.
 */
#define IP6_MIN_RANDOM_FACTOR              (SCK_Ticks_Per_Second >> 1)

/* RFC 2461 - section 10 - When computing a random value for Neighbor 
 * Discovery, the maximum allowable value of that random value.
 */
#define IP6_MAX_RANDOM_FACTOR              (SCK_Ticks_Per_Second + IP6_MIN_RANDOM_FACTOR)

/* RESERVED FOR FUTURE USE */
#define IP6_MAX_ANYCAST_DELAY_TIME         (1 * SCK_Ticks_Per_Second)
#define IP6_MAX_NEIGHBOR_ADVERTISEMENT     3                        

/* Router Constants */
#define IP6_MAX_INITIAL_RTR_ADVERT_INTERVAL    16
#define IP6_MAX_RA_DELAY_TIME                  (SCK_Ticks_Per_Second >> 1)

/* RFC 2461 - Section 6.2.1 - The maximum time allowed between sending
 * unsolicited multicast Router Advertisements from the interface, in 
 * seconds.  MUST be no less than 4 seconds and no greater than 1800 seconds.
 */
#define IP6_DEFAULT_MAX_RTR_ADVERT_INTERVAL    600

/* RFC 2461 - section 6.2.1 - The minimum time allowed between sending
 * unsolicited multicast Router Advertisements from the interface, in 
 * seconds.  MUST be no less than 3 seconds and no greater than 
 * .75 * MaxRtrAdvInterval.
 */
#define IP6_DEFAULT_MIN_RTR_ADVERT_INTERVAL    (IP6_DEFAULT_MAX_RTR_ADVERT_INTERVAL / 3)

/* RFC 2461 - section 6.2.1 - The value to be placed in MTU options sent by 
 * the router.  A value of zero indicates that no MTU options are sent.
 */
#define IP6_DEFAULT_ADVERT_LINK_MTU            0

/* RFC 2461 - section 6.2.1 - The value to be placed in the Reachable Time field
 * in the Router Advertisement messages sent by the router.  The value zero means 
 * unspecified (by this router).  MUST be no greater than 3,600,000 milliseconds 
 * (1 hour).
 */
#define IP6_DEFAULT_ADVERT_REACHABLE_TIME      0

/* RFC 2461 - section 6.2.1 - The value to be placed in the Retrans Timer field
 * in the Router Advertisement messages sent by the router.  The value zero means 
 * unspecified (by this router).
 */
#define IP6_DEFAULT_ADVERT_RETRANS_TIMER       0

/* RFC 2461 - section 6.2.1 - The default value to be placed in the Cur Hop Limit
 * field in the Router Advertisement messages sent by the router.  The value 
 * should be set to that current diameter of the Internet.  The value zero
 * means unspecified (by this router).  Default:  The value specified in the 
 * "Assigned Numbers" RFC that was in effect at the time of implementation.   
 */
#define IP6_DEFAULT_ADVERT_CUR_HOP_LIMIT       64

/* RFC 2461 - section 6.2.6 - The rate of consecutive router advertisements
 * to be transmitted to the all-nodes multicast address
 */
#define IP6_MIN_DELAY_BETWEEN_RAS               (3 * SCK_Ticks_Per_Second)

/* RFC 2461 - section 6.2.1 - The value to be placed in the Router Lifetime field
 * of Router Advertisements sent from the interface, in seconds.  MUST be either 
 * zero or between MaxRtrAdvInterval and 9000 seconds.  A value of zero indicates 
 * that the router is not to be used as a default router.
 */
#define IP6_DEFAULT_ADVERT_DEFAULT_LIFETIME    (3 * IP6_DEFAULT_MAX_RTR_ADVERT_INTERVAL)
                     
/* RFC 2463 - section 2.4.f - The rate of ICMPv6 error messages to be 
 * transmitted.  The default is 1 per second.
 */
#define ICMP6_ERROR_MSG_RATE_LIMIT             (1 * SCK_Ticks_Per_Second)

/* RFC 2461 - section 6.2.1 - The value to be placed in the Valid Lifetime
 * in the Prefix Information option, in seconds.  The default is 2592000
 * seconds. 
 */
#define IP6_DEFAULT_ADV_VALID_LIFETIME         2592000UL

/* RFC 2461 - section 6.2.1 - The value to be placed in the Preferred Lifetime
 * in the Prefix Information option, in seconds.  The default is 604800
 * seconds. 
 */
#define IP6_DEFAULT_ADV_PREFERRED_LIFETIME     604800UL


/************************** IPv6 General *********************************
 *
 */

/* The default Hop Limit for IPv6 packets */
#define IP6_HOP_LIMIT                   64

/* default Traffic Class for IPv6 packets */
#define IP6_TCLASS_DEFAULT              0

/* Default TTL to put in Multicast Packets */
#define IP6_DEFAULT_MULTICAST_TTL       1

/* Loopback is not yet supported. */
#define IP6_DEFAULT_MULTICAST_LOOP      0


/************************** MLD ******************************************
 *
 */

/* Maximum number of groups to which a socket can be a member */
#define IP6_MAX_MEMBERSHIPS                 10

/* Default MLD version to be used.  This does not guarantee the version
 * that the host will always operate, but this is the highest version that
 * the host will run.  For example, if the value is MLDV1_COMPATIBILITY, 
 * then the host will never run as a MLDv2 host.
 */
#define MLD6_DEFAULT_COMPATIBILTY_MODE     MLDV1_COMPATIBILITY

/* RFC 2710 - section 4 - When a node ceases to listen to a multicast 
 * address on an interface, it should send a single Done Message to the 
 * link-scope all-routers multicast address.  If the node's most recent 
 * Report Message was suppressed by hearing another Report Message, it 
 * may send nothing, as it is highly likely that there is another listener 
 * for that address still present on the same link.  If this optimization 
 * is implemented, it must be able to be turned off but should default
 * to on.
 */
#define MLD6_OPTIMIZE_DONE_REPORTS          NU_TRUE

/* Timers and Default Values */

/* RFC 2710 - section 7.1 - Allows tuning for the expected packet loss on 
 * a link.  If a link is expected to be lossy, the Robustness Variable may 
 * be increased.  MLD is robust to (Robustness Variable - 1) packet losses.  
 * The Robustness Variable must not be 0 and should not be 1.
 */
#define MLD6_ROBUST_VARIABLE                2

#if (MLD6_ROBUST_VARIABLE == 0)
    #error RFC 2710 - The Robustness Variable MUST not be 0.
#endif

/* RFC 2710 - section 7.2 - The interval between General Queries sent by 
 * the Querier.  By varying the Query Interval, an administrator may tune 
 * the number of MLD messages on the link; larger values can cause MLD 
 * queries to be sent less often.
 */
#define MLD6_QUERY_INTERVAL                 (125 * SCK_Ticks_Per_Second) /* 125 seconds */

/* RFC 2710 - section 7.3 - The Maximum Response Delay inserted into the 
 * periodic General Queries.  By varying the Query Response Interval, an 
 * administrator may tune the burstiness of MLD messages on the link; 
 * larger values make the traffic less bursty, as node responses are spread 
 * out over a larger interval.  The number of seconds represented by the 
 * Query Response Interval must be less than the Query Interval.
 */
#define MLD6_QUERY_RESPONSE_INTERVAL        (10 * SCK_Ticks_Per_Second)  /* 10 seconds */

/* RFC 2710 - section 7.4 - The amount of time that must pass before a 
 * router decides there are no more listeners for an address on a link.  
 * This value is not configurable.
 */
#define MLD6_MULTICAST_LISTENER_INTERVAL    (MLD6_ROBUST_VARIABLE * \
                                             MLD6_QUERY_INTERVAL +  \
                                             MLD6_QUERY_RESPONSE_INTERVAL)

/* RFC 2710 - section 7.5 - The length of time that must pass before a router 
 * decides that there is no longer another router which should be the querier 
 * on a link. This value is not configurable.
 */
#define MLD6_OTHER_QUERY_PRESENT_INTERVAL   (MLD6_ROBUST_VARIABLE * \
                                             MLD6_QUERY_INTERVAL +  \
                                             (MLD6_QUERY_RESPONSE_INTERVAL / 2))

/* RFC 2710 - section 7.6 - The interval between General Queries sent by a 
 * Querier on startup. 
 */
#define MLD6_STARTUP_QUERY_INTERVAL         (MLD6_QUERY_INTERVAL / 4)

/* RFC 2710 - section 7.7 - The number of queries sent out on startup, 
 * separated by the Startup Query Interval. 
 */
#define MLD6_STARTUP_QUERY_COUNT            MLD6_ROBUST_VARIABLE

/* RFC 2710 - section 7.8 - The Maximum Response Delay inserted into 
 * Multicast-Address-Specific Queries sent in response to a Done Message, and 
 * is also the amount of time between Multicast-Address-Specific Query 
 * Messages.  This value may be tuned to modify the leave-latency of the link.  
 * A reduced value results in reduced time to detect the departure of the last 
 * listener for an address.
 */
#define MLD6_LAST_LISTENER_QUERY_INTERVAL   (1 * SCK_Ticks_Per_Second) /* 1 second */

/* RFC 2710 - section 7.9 - The number of Multicast-Address-Specific Queries 
 * sent before the router assumes there are no remaining listeners for an 
 * address on a link.
 */
#define MLD6_LAST_LISTENER_QUERY_COUNT      MLD6_ROBUST_VARIABLE

/* RFC 2710 - section 7.10 - The time between repetitions of a node's initial 
 * report of interest in a multicast address.
 */
#define MLD6_UNSOLICITED_REPORT_INTERVAL    (10 * SCK_Ticks_Per_Second) /* 10 seconds */

/* ID MLDv2  - section 8.12 - The time for transitioning a host back to MLDv2 mode 
 * once an older version query is heard.  When an older version query is received, 
 * hosts set their Older Version Querier Present Timer to Older Version Querier Interval
 */
#define MLD6_OLDER_VERSION_QUERIER_PRESENT_TIMEOUT(device)   ((MLD6_ROBUST_VARIABLE * \
                                                  ((DV_DEVICE_ENTRY *)device)->dev6_mld_last_query_int) + \
                                                    MLD6_QUERY_RESPONSE_INTERVAL)


/************************** Neighbor Cache *******************************
 *
 */

/* The maximum number of packets to keep queued up while Address Resolution
 * is being performed.  The queue MUST hold at least one packet and may
 * contain more.
 */
#define NC_MAX_QUEUED_PACKETS           3

/* The maximum number of seconds to keep a Neighbor Cache entry that has not
 * been accessed.
 */
#define NC_TIMEOUT_ENTRY                (60 * SCK_Ticks_Per_Second * 10) /* 10 minutes */

/* The number of ethernet Neighbor Cache entries */
#define IP6_ETH_NEIGHBOR_CACHE_ENTRIES  10


/************************** RIPng ***************************************
 *
 */

/* Set this macro to NU_TRUE to disable Split Horizon with Poisoned Reverse */
#define RIPNG_DISABLE_POISONED_REVERSE       NU_FALSE

/* RFC 2453 - section 3.8 - The number of seconds between broadcasts of the 
 * complete routing table 
 */
#define RIPNG_BTIME                          (30 * SCK_Ticks_Per_Second)

/* The maximum number of entries to collect when performing Garbage collection */
#define RIPNG_MAX_DELETES                    25

/* RFC 2453 - section 3.8 - The number of seconds from the time a route is 
 * set to be deleted and the time it is actually deleted.
 */
#define RIPNG_DELETE_INTERVAL                (180 * SCK_Ticks_Per_Second)

/* RFC 2453 - section 3.8 - The number of seconds between performing 
 * garbage collection on the routing table.
 */
#define RIPNG_GARBAGE_COLLECTION_INTERVAL    (120 * SCK_Ticks_Per_Second)

/* The amount of time for a route to go unused before it is considered for
 * garbage collection.
 */
#define RIPNG_RT_LIFE_TIME                   ((UINT32)(180 * SCK_Ticks_Per_Second))


/********************* IPv6 MIBs Configurations *************************/

/* Set this macro to NU_FALSE to exclude the IPv6 Interface group MIBs. */
#define INCLUDE_IP6_MIB_IF_GROUP        INCLUDE_IPV6_MIB

/* Set this macro to NU_FALSE to exclude the IPv6 Address Prefix group MIBs. */
#define INCLUDE_IP6_MIB_ADDR_PREFIX     INCLUDE_IPV6_MIB

/* Set this macro to NU_FALSE to exclude the IPv6 Interface Address group MIBs. */
#define INCLUDE_IPV6_IF_ADDR_MIB        INCLUDE_IPV6_MIB

/* Set this macro to NU_FALSE to exclude the IPv6 Route Table group MIBs. */
#define INCLUDE_IPV6_RT                 INCLUDE_IPV6_MIB

/* Set this macro to NU_FALSE to exclude the IPv6 ICMP group MIBs. */
#define INCLUDE_IPV6_ICMP_MIB           INCLUDE_IPV6_MIB

/* Set this macro to NU_FALSE to exclude the IPv6 TCP group MIBs. */
#define INCLUDE_IPV6_TCP_MIB            INCLUDE_IPV6_MIB

/* Set this macro to NU_FALSE to exclude the IPv6 UDP group MIBs. */
#define INCLUDE_IPV6_UDP_MIB            INCLUDE_IPV6_MIB

/* Set this macro to NU_FALSE to exclude the IPv6 MLD group MIBs. */
#define INCLUDE_IPV6_MLD_MIB            INCLUDE_IPV6_MIB

/* Set this macro to NU_FALSE to exclude the IPv6 Net To Media MIBs. */
#define INCLUDE_IPV6_MIB_NTM            INCLUDE_IPV6_MIB


/******************* DHCPv6 Client Configurations ***********************/

/* Set this value to NU_TRUE when performing TAHI testing. */
#define DHCP6_ENABLE_TAHI_TESTING           NU_FALSE

/* The maximum number of retransmissions to send.  If DHCP6_MAX_RETRANS_COUNT
 * and DHCP6_MAX_RETRANS_DUR are both set to zero, the client will continue 
 * to retransmit the respective message(s) indefinitely until a reply is 
 * received.  Otherwise, the client will stop retransmitting when either
 * DHCP6_MAX_RETRANS_COUNT or DHCP6_MAX_RETRANS_DUR are reached.
 */
#define DHCP6_MAX_RETRANS_COUNT     0

/* The upper bound on the retransmission timer for retransmitting DHCP client
 * messages.  This value is set to 120 seconds by default.
 */
#define DHCP6_MAX_RETRANS_TIME      DHCP6_SOL_MAX_RT

/* The maximum time retransmissions may persist.  If DHCP6_MAX_RETRANS_COUNT
 * and DHCP6_MAX_RETRANS_DUR are both set to zero, the client will continue 
 * to retransmit the respective message(s) indefinitely until a reply is 
 * received.  Otherwise, the client will stop retransmitting when either
 * DHCP6_MAX_RETRANS_COUNT or DHCP6_MAX_RETRANS_DUR are reached.
 */
#define DHCP6_MAX_RETRANS_DUR       0

/* Length of the buffer used for DHCPv6 client transmissions. */
#define DHCP6_BUFFER_LEN            512

/* The starting value of the Transaction ID to add to DHCPv6 client 
 * packets. 
 */
#define DHCP6_TRANS_ID_START        "100"

/* The total size of the buffer to create to hold incoming DHCP packets. */
#define DHCP6_RX_BUFFER_LEN         1024

/* The default values of T1 and T2 if the server leaves the decision up to 
 * the client.
 */
#define DHCP6_T1_DEFAULT                    86400
#define DHCP6_T2_DEFAULT                    172800


/****************** End Nucleus IPv6 Configuration **********************/

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* ifndef NET6_CFG_H */
