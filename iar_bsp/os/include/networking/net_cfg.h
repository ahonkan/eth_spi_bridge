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
*
*   FILENAME
*
*       net_cfg.h
*
*   DESCRIPTION
*
*       This file will hold all defines that control the various
*       configuration settings of Nucleus NET.
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
#ifndef NET_CFG_H
#define NET_CFG_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* These definitions control which version of NET the build should be
   compatible with. This should allow new versions of NET to be shipped but
   remain compatible with applications or drivers designed for previous
   versions. */
#define     NET_4_0         1       /* NET 4.0 */
#define     NET_4_2         2       /* NET 4.2 */
#define     NET_4_3         3       /* NET 4.3 */
#define     NET_4_4         4       /* NET 4.4 */
#define     NET_4_5         5       /* NET 4.5 */
#define     NET_5_1         6       /* NET 5.1 */
#define     NET_5_2         7       /* NET 5.2 */
#define     NET_5_3         8       /* NET 5.3 */
#define     NET_5_4         9       /* NET 5.4 */
#define     NET_5_5         10      /* NET 5.5 */

#define NET_VERSION_COMP    NET_5_5        /* The version for which compatibility
                                              is desired. */


/************************ Feature Set Configuration *************************

    The following macros control which network protocols and features will be
    included in the build of the Nucleus NET stack. Keep in mind that some
    protocols make use of other protocols, therefore it is not always possible to
    remove certain protocols from the build. One example would be UDP. UDP is
    used by DHCP and some other protocols, so if you want to remove UDP you
    must also remove all the protocols that use UDP. You can look at the
    #error macros below to see which protocols are dependent on other
    protocols.

*/

/* Set this to NU_TRUE to build static version of Net. */
#define INCLUDE_STATIC_BUILD            NU_FALSE

/* Change this macro to NU_TRUE to enable the safety precautions outlined
   in draft-ietf-tcpm-tcpsecure-00.txt which reduce or eliminate the
   vulnerability of TCP connections.
*/
#define TCPSECURE_DRAFT                 NU_FALSE

#define DHCP_VALIDATE_CALLBACK          NU_FALSE

/* Use the RFC 4361 style for the Client ID option in IPv4 DHCP packets. */
#define INCLUDE_DHCP_RFC4361_CLIENT_ID  NU_TRUE

/* Set this value to NU_TRUE to enable the GRE protocol for PPTP tunneling. */
#define INCLUDE_GRE                     NU_FALSE

/* Set this macro to NU_TRUE to enable IP Tunnels. */
#define INCLUDE_IP_TUNNEL               NU_FALSE

/* Set this value to NU_TRUE to enable interface stack. */
#define INCLUDE_IF_STACK                NU_FALSE

/* Set this value to NU_TRUE to get more statics while using SNMP. */
#define INCLUDE_IF_EXT                  NU_FALSE

/* Include MIB-2 statistics gathering in NET - This must always be NU_FALSE
 * when using SNMP Research SNMP. */
#ifdef CFG_NU_OS_NET_SNMP_ENABLE
#define INCLUDE_MIB2_RFC1213            NU_TRUE
#else
#define INCLUDE_MIB2_RFC1213            NU_FALSE
#endif

/* Set this macro to NU_TRUE to include IP Tunnel MIBs. */
#define INCLUDE_IP_TUN_MIB              NU_FALSE

/* This macro specifies whether SNMP will be initialized by NET. To build in
   SNMP support change the NU_FALSE to a NU_TRUE.  Note that SNMP is a separate
   product and must have been purchased and installed for this option to work. */
#ifdef CFG_NU_OS_NET_SNMP_ENABLE
#define INCLUDE_SNMP                    NU_TRUE
#else
#define INCLUDE_SNMP                    NU_FALSE
#endif

/* Determine whether SNMP Research SNMP is being used */
#if ( (INCLUDE_MIB2_RFC1213 == NU_FALSE) && (INCLUDE_SNMP == NU_TRUE) )
#define INCLUDE_SR_SNMP                 NU_TRUE
#else
#define INCLUDE_SR_SNMP                 NU_FALSE
#endif

/* Set this macro to NU_TRUE if the system will contain one or more VLAN enabled
   interfaces. */
#define INCLUDE_VLAN                    NU_FALSE

/* This macro controls whether to use the standard or canonical form for
   MAC addresses.  Set this to NU_FALSE to reverse MAC address bit ordering.
   For Type II Ethernet frames NU_TRUE is the correct setting. */
#define VLAN_CANONICAL_FORM             NU_TRUE

/* Set this macro if VLAN is to be implemented in software */
#define USE_SW_VLAN_METHOD              NU_FALSE

/* MAC BRIDGE */

/* Set this macro to NU_TRUE to enable MAC Bridge operations. */

#define INCLUDE_MAC_BRIDGE              NU_FALSE


/* Do not redefine these macros if using the Nucleus builder to compile the
 * code.  If not using the Nucleus builder, all macros must be explicitly
 * configured here; otherwise, configure the respective .metadata file.
 */
#ifndef CFG_NU_OS_NET_STACK_CFG_H

/* Set this to NU_FALSE to exclude UDP. */
#define INCLUDE_UDP                     NU_TRUE

/* Set this to NU_FALSE to exclude TCP. */
#define INCLUDE_TCP                     NU_TRUE

/* Change this macro to NU_FALSE to disable inclusion of Congestion Control
   within the TCP module.  This functionality is enabled by default.
*/
#define INCLUDE_CONGESTION_CONTROL      NU_TRUE

/* By default, Path MTU Discovery is included in the Nucleus NET library for
   both IPv4 and IPv6 (if IPv6 is enabled).  Change this definition to
   NU_FALSE to exclude Path MTU Discovery.
*/
#define INCLUDE_PMTU_DISCOVERY          NU_TRUE

/* Change this macro to NU_TRUE to enable TCP Keep-Alive in the system.
   Note that you must set the appropriate socket option to enable Keep-Alive
   for a connection. */
#define INCLUDE_TCP_KEEPALIVE           NU_FALSE

/* Change this macro to NU_TRUE to include functionality for the NewReno
   algorithim within the TCP Congestion Control module as per RFC 3782.
   This functionality is disabled by default.
*/
#define INCLUDE_NEWRENO                 NU_FALSE

/* Change this macro to NU_TRUE to include functionality for TCP SACKs
   per RFC 2018.  This functionality can also be configured on a per-socket
   basis at run-time when compilation support is enabled.
*/
#define NET_INCLUDE_SACK                NU_TRUE

/* Change this macro to NU_TRUE to include functionality for TCP D-SACKs
   per RFC 2883.  This functionality can also be configured on a per-socket
   basis at run-time when compilation support is enabled.
*/
#define NET_INCLUDE_DSACK               NU_TRUE

/* Change this macro to NU_FALSE to exclude functionality for the TCP
   Window Scale option per RFC 1323.  This functionality can also be
   configured on a per-socket basis at run-time when compilation support
   is enabled.
*/
#define NET_INCLUDE_WINDOWSCALE         NU_TRUE

/* Change this macro to NU_FALSE to exclude functionality for the TCP
   Timestamp option per RFC 1323.  This functionality can also be
   configured on a per-socket basis at run-time when compilation support
   is enabled.
*/
#define NET_INCLUDE_TIMESTAMP           NU_TRUE

/* Change this macro to NU_FALSE to exclude functionality for the TCP
   Limited Transmit algorithm per RFC 3042.
*/
#define NET_INCLUDE_LMTD_TX             NU_TRUE

/* By default raw IP is not included in the Nucleus NET build.  Set this
   to NU_TRUE to include raw IP.
*/
#define INCLUDE_IP_RAW                  NU_FALSE

/* This macro controls whether the code to forward IP packets will be included
   in the library.
*/
#define INCLUDE_IP_FORWARDING           NU_FALSE

/* By default IP reassembly is included in the Nucleus NET library. Change
   this definition to a NU_FALSE to exclude IP reassembly. */
#define INCLUDE_IP_REASSEMBLY           NU_TRUE

/* By default IP fragmentation is included in the Nucleus NET library. Change
   this definition to a NU_FALSE to exclude IP fragmentation. */
#define INCLUDE_IP_FRAGMENT             NU_TRUE

/* By default IP Multicasting is included in the Nucleus NET library. Change
   this definition to a NU_FALSE to exclude IP Multicasting. */
#define INCLUDE_IP_MULTICASTING         NU_TRUE

/* Should the loopback device be included and initialized in the
   build of NET. This is NU_TRUE by default.
*/
#define INCLUDE_LOOPBACK_DEVICE         NU_TRUE

/* By default, ARP is included in the Nucleus NET build.  Set this macro
   to NU_FALSE to exclude ARP from the build.
*/
#define INCLUDE_ARP                     NU_TRUE

/* By default RARP is not included in the Nucleus NET build.  To include RARP
   change the NU_FALSE to a NU_TRUE. See the Nucleus NET reference manual
   for more information on RARP.
*/
#define INCLUDE_RARP                    NU_FALSE

/* By default DNS is included in the Nucleus NET build.  To exclude it change
   the NU_TRUE below to a NU_FALSE.  See the Nucleus NET reference Manual
   for more information on DNS.
*/
#define INCLUDE_DNS                     NU_TRUE

/* By default DHCP client is included in the Nucleus NET build.  To exclude
   it change the NU_TRUE below to a NU_FALSE. DHCP validate callback and
   vendor options callback can be enabled in a like manner. See the
   Nucleus NET reference Manual for more information on DHCP.
*/
#define INCLUDE_DHCP                    NU_TRUE

/* By default BOOTP client is not included in the Nucleus Net Build. To include
   it change the NU_FALSE below to a NU_TRUE.  See the Nucleus Net
   reference Manual for more information on BOOTP.
*/
#define INCLUDE_BOOTP                   NU_FALSE

/* By default IPv4 is included in the Nucleus NET library.  Change this
   definition to NU_FALSE to exclude IPv4. */
#define INCLUDE_IPV4                    NU_TRUE

/* By default IPv6 is not included in the Nucleus NET library.  Change this
   definition to NU_TRUE to include IPv6. Note that IPv6 is a separate product
   and must have been purchased and installed for this option to work. */
#define INCLUDE_IPV6                    NU_TRUE

/* This macro will enable Nucleus IPsec support.  Note that Nucleus IPsec
   is a separate product and must have been purchased and installed for
   this option to work. */
#define INCLUDE_IPSEC                   NU_TRUE

/* This macro will enable IKE support.  Note that IKE is a component of
   Nucleus IPsec, which is a separate product and must have been
   purchased and installed for this option to work. */
#define INCLUDE_IKE                     NU_TRUE

/* Set this macro if one or more interfaces in the system supports hardware
   offloading capabilities */
#define HARDWARE_OFFLOAD                NU_FALSE

/* This macro will enable the advanced debugging features of Nucleus NET */
#define NU_DEBUG_NET                    NU_FALSE

/* This macro will enable the notification module within Nucleus NET that
 * is used with the debug module and can also be used to notify the
 * application layer of events that occur within network drivers or
 * other parts of the TCP/IP stack.
 */
#define NU_ENABLE_NOTIFICATION          NU_FALSE

/* Set this macro to NU_TRUE to enable IPv4 link-local configuration
 * per RFC 3927 on the node.
 */
#define INCLUDE_LL_CONFIG               NU_FALSE

/* Log all NET errors to an error tracking array */
#define INCLUDE_NET_ERROR_LOGGING       NU_TRUE

/* Validate input parameters in all API routines. */
#define INCLUDE_NET_API_ERR_CHECK       NU_TRUE

/* Enable minimal ICMP components; Echo Reply, Destination Unreachable,
 * and Time Exceeded.
 */
#define INCLUDE_LITE_ICMP               NU_FALSE

/* Include TCP out of order processing.  If disabled, packet received out
 * of order will be discarded by TCP.
 */
#define INCLUDE_TCP_OOO                 NU_TRUE

/* Enable the SO_REUSEADDR option that will allow multiple IP addresses to
 * bind to the same port.
 */
#define INCLUDE_SO_REUSEADDR            NU_TRUE

/* Enable the mDNS protocol in the networking stack. */
#define INCLUDE_MDNS                    NU_FALSE

/* Include the NAT module in the library */
#define INCLUDE_NAT                     NU_FALSE

#else

#define INCLUDE_UDP                     CFG_NU_OS_NET_STACK_INCLUDE_UDP
#define INCLUDE_TCP                     CFG_NU_OS_NET_STACK_INCLUDE_TCP
#define INCLUDE_CONGESTION_CONTROL      CFG_NU_OS_NET_STACK_INCLUDE_CONGESTION_CTRL
#define INCLUDE_PMTU_DISCOVERY          CFG_NU_OS_NET_STACK_INCLUDE_PMTU_DISCVRY
#define INCLUDE_TCP_KEEPALIVE           CFG_NU_OS_NET_STACK_INCLUDE_TCP_KEEPALIVE
#define INCLUDE_NEWRENO                 CFG_NU_OS_NET_STACK_INCLUDE_NEWRENO
#define INCLUDE_IP_FORWARDING           CFG_NU_OS_NET_STACK_INCLUDE_IP_FWD
#define INCLUDE_IP_REASSEMBLY           CFG_NU_OS_NET_STACK_INCLUDE_IP_RASM
#define INCLUDE_IP_FRAGMENT             CFG_NU_OS_NET_STACK_INCLUDE_IP_FRAG
#define INCLUDE_IP_MULTICASTING         CFG_NU_OS_NET_STACK_INCLUDE_IP_MULT
#define INCLUDE_LOOPBACK_DEVICE         CFG_NU_OS_NET_STACK_INCLUDE_LOOPBACK
#define INCLUDE_ARP                     CFG_NU_OS_NET_STACK_INCLUDE_ARP
#define INCLUDE_RARP                    CFG_NU_OS_NET_STACK_INCLUDE_RARP
#define INCLUDE_DNS                     CFG_NU_OS_NET_STACK_INCLUDE_DNS
#define INCLUDE_DHCP                    CFG_NU_OS_NET_STACK_INCLUDE_DHCP
#define INCLUDE_BOOTP                   CFG_NU_OS_NET_STACK_INCLUDE_BOOTP
#define INCLUDE_IPV4                    CFG_NU_OS_NET_STACK_INCLUDE_IPV4
#define HARDWARE_OFFLOAD                CFG_NU_OS_NET_STACK_INCLUDE_HW_OFFLOAD
#define NU_DEBUG_NET                    CFG_NU_OS_NET_STACK_INCLUDE_NET_DEBUG
#define NU_ENABLE_NOTIFICATION          CFG_NU_OS_NET_STACK_INCLUDE_NOTIFICATIONS
#define NET_INCLUDE_SACK                CFG_NU_OS_NET_STACK_INCLUDE_SACK
#define NET_INCLUDE_DSACK               CFG_NU_OS_NET_STACK_INCLUDE_DSACK
#define NET_INCLUDE_WINDOWSCALE         CFG_NU_OS_NET_STACK_INCLUDE_WINDOWSCALE
#define NET_INCLUDE_TIMESTAMP           CFG_NU_OS_NET_STACK_INCLUDE_TIMESTAMP
#define NET_INCLUDE_LMTD_TX             CFG_NU_OS_NET_STACK_INCLUDE_LMTD_TX
#define INCLUDE_IP_RAW                  CFG_NU_OS_NET_STACK_INCLUDE_IP_RAW
#define INCLUDE_LL_CONFIG               CFG_NU_OS_NET_STACK_INCLUDE_LL_CONFIG
#define INCLUDE_NET_ERROR_LOGGING       CFG_NU_OS_NET_STACK_INCLUDE_NET_ERROR_LOGGING
#define INCLUDE_NET_API_ERR_CHECK       CFG_NU_OS_NET_STACK_INCLUDE_NET_API_ERR_CHECK
#define INCLUDE_LITE_ICMP               CFG_NU_OS_NET_STACK_INCLUDE_LITE_ICMP
#define INCLUDE_TCP_OOO                 CFG_NU_OS_NET_STACK_INCLUDE_TCP_OOO
#define INCLUDE_SO_REUSEADDR            CFG_NU_OS_NET_STACK_INCLUDE_SO_REUSEADDR
#define INCLUDE_MDNS                    CFG_NU_OS_NET_STACK_INCLUDE_MDNS

#ifdef CFG_NU_OS_NET_IPV6_ENABLE
#define INCLUDE_IPV6                    NU_TRUE
#else
#define INCLUDE_IPV6                    NU_FALSE
#endif

#ifdef CFG_NU_OS_NET_IPSEC_ENABLE
#define INCLUDE_IPSEC                   NU_TRUE
#else
#define INCLUDE_IPSEC                   NU_FALSE
#endif

#ifdef CFG_NU_OS_NET_IKE_ENABLE
#define INCLUDE_IKE                     NU_TRUE
#else
#define INCLUDE_IKE                     NU_FALSE
#endif

#ifdef CFG_NU_OS_NET_NAT_ENABLE
#define INCLUDE_NAT                     NU_TRUE
#else
#define INCLUDE_NAT                     NU_FALSE
#endif

#endif

/******************** End Feature Set Configuration *************************/


/******************** Begin Configuration Validation ************************/

/* Make sure that configuration
   to this point is valid.
   Do not modify this section. */

/* If using Nucleus SIM, enable hardware offloading since certain PC drivers
   do not support the disabling of hardware checksum offloading. */
#ifdef NU_SIMULATION

#undef HARDWARE_OFFLOAD
#define HARDWARE_OFFLOAD                NU_TRUE

#endif

/* DNS requires UDP. */
#if ((INCLUDE_UDP == NU_FALSE) && (INCLUDE_DNS == NU_TRUE))
    #error UDP must be included in order to use DNS
#endif

/* DHCP requires UDP. */
#if ((INCLUDE_UDP == NU_FALSE) && (INCLUDE_DHCP == NU_TRUE))
    #error UDP must be included in order to use DHCP
#endif

/* BOOTP requires UDP. */
#if ((INCLUDE_UDP == NU_FALSE) && (INCLUDE_BOOTP == NU_TRUE))
    #error UDP must be included in order to use BOOTP
#endif

/* SNMP requires UDP. */
#if ((INCLUDE_UDP == NU_FALSE) && (INCLUDE_SNMP == NU_TRUE))
    #error UDP must be included in order to use SNMP
#endif

/* Duplicate Address Detection uses Multicasting.  If multicasting support
   is not enabled, Duplicate Address Detection cannot be enabled either. */
#if ((INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_FALSE) && \
     (INCLUDE_DAD6 == NU_TRUE))
    #error Multicasting must be included in order to perform Duplicate Address Detection
#endif

/* Only use the socket interface if at least one socket protocol is included. */
#if (INCLUDE_TCP == NU_TRUE || INCLUDE_UDP == NU_TRUE || INCLUDE_IP_RAW == NU_TRUE)
#define INCLUDE_SOCKETS                 NU_TRUE
#else
#define INCLUDE_SOCKETS                 NU_FALSE
#endif

#if ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_IPV4 == NU_FALSE) )
#undef  INCLUDE_MIB2_RFC1213
#define  INCLUDE_MIB2_RFC1213           NU_FALSE
#endif

#if ( (INCLUDE_IPV6 == NU_FALSE) && (INCLUDE_IPV4 == NU_FALSE) )
    #error IPv4 and IPv6 cannot both be excluded from the build.
#endif

#if ( (NET_INCLUDE_DSACK == NU_TRUE) && (NET_INCLUDE_SACK == NU_FALSE) )
    #error SACK support must be enabled to use D-SACK.
#endif

/* mDNS uses multicasting.  Multicast support must be included in the stack
 * to make use of mDNS services.
 */
#if ( (INCLUDE_MDNS == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_FALSE))
    #error Multicasting support must be enabled to use mDNS.
#endif

/* If the debug module is enabled, the notification module must also
 * be enabled in order to report debug information to the application.
 */
#if (NU_DEBUG_NET == NU_TRUE)
#undef  NU_ENABLE_NOTIFICATION
#define NU_ENABLE_NOTIFICATION  NU_TRUE
#endif

/******************** End Configuration Validation **************************/



/******************* Nucleus NET Configuration Section **********************

    In this section can be found various timeouts and size limitations used
    by the NET stack.

*/

/********************** System Wide Settings ********************************
 *
 */

#ifndef CFG_NU_OS_NET_STACK_CFG_H
/* Local host's name. Maximum of MAX_HOST_NAME_LENGTH characters long. */
#define HOSTNAME                    "localhost"
#else
#define HOSTNAME                    CFG_NU_OS_NET_STACK_HOSTNAME
#endif

/* Local domain. Maximum of MAX_DOMAIN_NAME_LENGTH characters long. */
#define DOMAINNAME                  "localdomain.com "

#ifndef CFG_NU_OS_NET_STACK_CFG_H
/* Default Name to be used for DNS-SD service advertisement */
#define DNS_DEFAULT_INSTANCE_NAME   "Nucleus Device"
#else
#define DNS_DEFAULT_INSTANCE_NAME   CFG_NU_OS_NET_STACK_DNS_DEFAULT_NAME
#endif

#ifndef CFG_NU_OS_NET_STACK_CFG_H
/* Memory Pool to be used for net buffers . */
#define BUFS_IN_UNCACHED_MEM         NU_TRUE
#else
#define BUFS_IN_UNCACHED_MEM         CFG_NU_OS_NET_STACK_BUFS_IN_UNCACHED_MEM
#endif

/* Max size of the local host name */
#define MAX_HOST_NAME_LENGTH        32

/* Max size of the local domain name */
#define NET_MAX_DOMAIN_NAME_LENGTH  32

/* Nucleus NET relies on several tasks to perform its duties.
   The settings for the creation of these tasks are defined below. */

/* Configure the size of system tasks and HISRs within Nucleus NET. */
#define NU_MIN_NET_STACK_SIZE       3000

/* The events dispatcher is responsible for processing all timing related
   events for NET and other networking products. The settings for this task
   are defined here. */
#ifdef CFG_NU_OS_NET_IPSEC_ENABLE
#define EV_STACK_SIZE               CFG_NU_OS_NET_STACK_EV_STACK_SIZE + 1000
#else
#define EV_STACK_SIZE               CFG_NU_OS_NET_STACK_EV_STACK_SIZE
#endif
#define EV_PRIORITY                 3
#define EV_TIME_SLICE               0
#define EV_PREEMPT                  NU_PREEMPT


/* The timer task is responsible for processing all received packets.
   The settings for this task are defined here. The default value of
   TM_STACK_SIZE is increased by 5000 bytes because
   Nucleus WPA Supplicant requires more stack space for this task. */
#if defined(CFG_NU_OS_NET_IPSEC_ENABLE) || defined(CFG_NU_OS_NET_WPA_SUPP_ENABLE)
#define TM_STACK_SIZE               CFG_NU_OS_NET_STACK_TM_STACK_SIZE + 5000
#else
#define TM_STACK_SIZE               CFG_NU_OS_NET_STACK_TM_STACK_SIZE
#endif

#define TM_PRIORITY                 3
#define TM_TIME_SLICE               0
#define TM_PREEMPT                  NU_PREEMPT


/* The RIP2 task is responsible for processing all RIP packets
   The settings for this task are defined here */

#define RIP2_BUFFER_SIZE            512
#define RIP2_STACK_SIZE             (3000 + (RIP2_BUFFER_SIZE))
#define RIP2_PRIORITY               20
#define RIP2_TIME_SLICE             0
#define RIP2_PREEMPT                NU_PREEMPT


/* The DHCP Event Handler task is responsible for renewing and rebinding
   of the device's IP address in accordance with the DHCP protocol.
   The settings for this task are defined here. */

#define DHCP_STACK_SIZE             CFG_NU_OS_NET_STACK_DHCP_STACK_SIZE
#define DHCP_PRIORITY               3
#define DHCP_TIME_SLICE             0
#define DHCP_PREEMPT                NU_PREEMPT


/* The NLOG_COMM task is responsible for sending IP, TCP, UDP, ARP, and ICMP
   header info to a remote client through a configurable socket.
   The settings for this task are defined here */

#define NLOG_COMM_STACK_SIZE        NU_MIN_NET_STACK_SIZE
#define NLOG_COMM_PRIORITY          10
#define NLOG_COMM_TIME_SLICE        0
#define NLOG_COMM_PREEMPT           NU_PREEMPT

/* Nucleus NET relies on queues to transmit and receive messages to and from
   tasks.  The settings for the creation of these queues are outlined below */

/* The Event Queue is used to send messages and info to the EventsDispatcher
   task in NET.  The settings for this queue are defined here */
#define SCK_EVENT_Q_NUM_ELEMENTS    CFG_NU_OS_NET_STACK_NUM_EVT_Q_ELEMENTS  /* number of elements in the
                                                                               event queue */

/* The DHCP Event queue is used to send messages to the DHCP_Events_Dispatcher task.
   The settings for this queue are defined here */
#define DHCP_Q_SIZE                 5                       /* Max # of messages that can
                                                               be held by the DHCP queue. */

/* Use the DUID Assigned by Vendor Based on Enterprise Number method to
 * generate the DUID for the node by default.  This can be changed at run-time.
 * Note that this method requires the vendor's registered Private Enterprise
 * Number as maintained by IANA and identifier to be configured below.
 * Please see section 9.3 of RFC 3315.
 */
#define INCLUDE_DHCP_DUID_EN        NU_FALSE

/* Use the DUID Based on Link-layer Address method to generate the DUID for
 * the node by default.  This can be changed at run-time.  Note that this
 * method requires that the interface being used to generate the DUID be a
 * permanent interface on the node.  Please see section 9.4 of RFC 3315.
 */
#define INCLUDE_DHCP_DUID_LL        NU_TRUE

/* The Private Enterprise Number assigned to this node by the IANA.  This
 * must be an unsigned 32-bit value.  This value is used in creating the
 * DUID for DUID Assigned by Vendor Based on Enterprise Number method.
 */
#define DHCP_DUID_PRIV_ENT_NO       0x12345678

/* The identifier to be used in creating the DUID for the node.  This is a
 * variable-length value represented in hexadecimal format.  This value is
 * used in creating the DUID for DUID Assigned by Vendor Based on Enterprise
 * Number method.
 */
#define DHCP_DUID_ID_NO             "thisistheidnostring"

/* Maximum possible length of the DUID for IPv4 and IPv6 DHCP client
 * transactions.
 */
#define DHCP_DUID_MAX_ID_NO_LEN     64

/* The hardware type assigned by the IANA, as described in RFC 826.  This
 * value is used in creating the DUID for DUID Based on Link-layer Address
 * method.
 */
#define DHCP_DUID_HW_TYPE           1

/* The first IAID to assign to the first interface that is enabled.  Each
 * interface afterwards will be assigned a successive IAID.  The IAID can
 * be retrieved and changed at run-time.
 */
#define DHCP_IAID_START             0x12345678

/* Do not redefine these macros if using the Nucleus builder to compile the
 * code.  If not using the Nucleus builder, all macros must be explicitly
 * configured here; otherwise, configure the respective .metadata file.
 */
#ifndef CFG_NU_OS_NET_STACK_CFG_H
#define MAX_BUFFERS                 200                     /* The max number of internal NET   */
                                                            /* buffers. These are used for TX   */
                                                            /* and RX of data packets.          */

#define NET_PARENT_BUFFER_SIZE      512
#define MAX_REASM_MAX_SIZE          65535                   /* This is the maximum value that   */
                                                            /* the maximum reassembly size can  */
                                                            /* be set to for a device.          */
#else

#define MAX_BUFFERS                     CFG_NU_OS_NET_STACK_MAX_BUFS
#define NET_PARENT_BUFFER_SIZE          CFG_NU_OS_NET_STACK_BUF_SIZE
#define MAX_REASM_MAX_SIZE              CFG_NU_OS_NET_STACK_REASM_SIZE

#endif

#define NET_FREE_BUFFER_THRESHOLD   10
/* This is the minimum size that a NET buffer may be.  This minimum
 * value is to insure that the IP and transport layer headers will
 * fit into a single buffer.
 */
#define NET_MIN_PARENT_BUFFER_SIZE  128

#if (NET_PARENT_BUFFER_SIZE < NET_MIN_PARENT_BUFFER_SIZE)
#error Illegal value for NET_PARENT_BUFFER_SIZE
#endif

/* Ensure the BOOTP header will fit in a single buffer. */
#if ( (INCLUDE_BOOTP == NU_TRUE) && (NET_PARENT_BUFFER_SIZE < (NET_MAX_MAC_HEADER_SIZE + IP_HEADER_LEN + UDP_HEADER_LEN + BOOTP_HEADER_LEN)) )
#error Illegal value for NET_PARENT_BUFFER_SIZE when using BOOTP
#endif

/* The maximum size of sticky options that can be set at one time on a socket. */
#define STICKY_OPTIONS_MAX_SIZE     (256 + sizeof(tx_ancillary_data))

/* The size of the buffer suspension HISR */
#if ((INCLUDE_NET_ERROR_LOGGING == NU_TRUE) || (PRINT_NET_ERROR_MSG == NU_TRUE))
#define NET_BUFFER_SUSPENSION_HISR_SIZE     CFG_NU_OS_NET_STACK_BUF_HISR_STACK_SIZE + 512
#else
#define NET_BUFFER_SUSPENSION_HISR_SIZE     CFG_NU_OS_NET_STACK_BUF_HISR_STACK_SIZE
#endif

#define NLOG_IS_LOGGING_HISR_SIZE           NU_MIN_NET_STACK_SIZE

/************************** TCP Settings ************************************
 *
 */

#define MAXRTO          ((UINT32)(240 * SCK_Ticks_Per_Second))      /* Maximum retransmit timeout.*/

/* The minimum value for the Retransmit Timer.  Section 2.4 of RFC 2988
 * recommends setting this value to 1 second; however, after extensive
 * testing and customer feedback, this value has been set to 1/4 second by
 * default.
 */
#define MINRTO          ((UINT32)(SCK_Ticks_Per_Second >> 2))

#define WAITTIME        ((UINT32)(2 * SCK_Ticks_Per_Second))        /* Length of time to wait before
                                                                           reusing a port. */

#define MAX_RETRANSMITS 5                                           /* The max number of times to       */
                                                                    /* retransmit a packet before       */
                                                                    /* giving up.                       */

#define WINDOW_SIZE     16000                                       /* Size of buffers for TCP in/out   */
                                                                    /* windows.                         */


#if (INCLUDE_TCP == NU_TRUE)
#ifndef CFG_NU_OS_NET_STACK_CFG_H
#define TCP_MAX_PORTS   30                                          /* Maximum number of TCP ports.     */
#else
#define TCP_MAX_PORTS   CFG_NU_OS_NET_STACK_TCP_MAX_PORTS
#endif
#endif

/* SWSOVERRIDE is the amount of time to wait before overriding the Nagle
   algorithm.  The Nagle algorithm is aimed at preventing the transmission of
   lots of tiny packets.  However, we only want to delay a packet for a short
   period of time.  RFC 1122 recommends a delay of 0.1 to 1.0 seconds.  We
   default to a delay of a 0.25 second. */
#define SWSOVERRIDE     (SCK_Ticks_Per_Second >> 2)                 /* Delay of a 1/4 second */

/* PROBETIMEOUT is the delay before a window probe is sent.  */
#define PROBETIMEOUT    (5 * SCK_Ticks_Per_Second)                  /* Delay of 5 seconds. */

/* The longest amount of time to delay between successive TCP Window Probes */
#define MAX_PROBETIMEOUT    (60 * SCK_Ticks_Per_Second)             /* Delay of 60 seconds */

/* The maximum number of TCP Window Probes to transmit */
#define TCP_MAX_PROBE_COUNT     10

/* This is the number of ticks to delay sending an ACK.  A value of
   approximately a 1/5 of a second (200ms) is recommended. */
#define TCP_ACK_TIMEOUT (SCK_Ticks_Per_Second / 5)                  /* Delay of 1/5 second */

/* The maximum number of Keep-Alive packets transmitted before timing
 * out a connection.
 */
#define TCP_MAX_KEEPALIVES      10

/* RFC 1122 - The delay (amount of time for a connection to remain idle)
 * before the first Keep-Alive packet is transmitted.  Default = 2 hours.
 */
#define TCP_KEEPALIVE_DELAY     (((SCK_Ticks_Per_Second * 60) * 60) * 2)

/* RFC 1122 - The time interval between Keep-Alive packets.  Default = 75 seconds. */
#define TCP_KEEPALIVE_INTERVAL  (75 * SCK_Ticks_Per_Second)

/* RFC 2581 - The number of incoming duplicate ACKs that will trigger
 * Fast Retransmit
 */
#define TCP_FAST_RETRANS_DUP_ACKS   3

/* The default value for the Retransmit Timer.  Section 2.1 of RFC 2988
 * recommends setting this value to 3 seconds; however, after extensive
 * testing and customer feedback, this value has been set to 1 second by
 * default.
 */
#define TCP_RTTDFLT                 (1 * SCK_Ticks_Per_Second)

/* The amount of time to remain in the TCPCLOSETIMEOUTSFW2 state without
 * receiving a FIN from the other side of the connection.
 */
#define TCP_SFW2_TIMEOUT            (30 * SCK_Ticks_Per_Second)

/* Initial setting for TCP congestion slow start threshold */
#define TCP_SLOW_START_THRESHOLD    65535UL

/* The number of full-sized segments to delay before transmitting
 * an ACK for the data.  RFC 1122 - section 4.2.3.2 - A TCP SHOULD
 * implement a delayed ACK, but an ACK should not be excessively
 * delayed; in particular, the delay MUST be less than 0.5 seconds,
 * and in a stream of full-sized segments there SHOULD be an ACK
 * for at least every second segment.  Default : 2.
 */
#define TCP_DELAY_ACK_THRESH        2

/* The number of retransmission attempts to be made before clearing SRTT
 * and RTTVAR for the connection per RFC 2988 section 5: "Note that a TCP
 * implementation MAY clear SRTT and RTTVAR after backing off the timer
 * multiple times as it is likely that the current SRTT and RTTVAR are
 * bogus in this situation."  Set this value to MAX_RETRANSMITS to disable
 * this functionality.
 */
#define TCP_CLEAR_SRTT_RETRANSMITS  2

/* The number of retransmission attempts to be made before attempting to
 * find a new route to the destination.  It is possible that the current
 * route has gone down.  Set this value to MAX_RETRANSMITS to disable
 * this functionality.
 */
#define TCP_FIND_NEW_ROUTE          2

/* Configure this value to the number of data packets to retransmit upon
 * receipt of a partial ACK within the Congestion Control algorithm.  This
 * value is 1 by default, but according to section 5 of RFC 3782,
 * retransmitting multiple packets upon receipt of a partial ACK could
 * hasten recovery from multiple packet loss within the same window.
 */
#define TCP_PARTIAL_ACK_RETRANS_COUNT   1

/* The maximum allowable scale factor for the TCP Window Scale option as
 * defined by RFC 1323.  The default value is 14.
 */
#define TCP_MAX_WINDOWSCALE_FACTOR      14

/* The maximum size of the configurable Window Size. */
#define TCP_MAX_WINDOW_SIZE             1073725440

#if (WINDOW_SIZE > TCP_MAX_WINDOW_SIZE)
#error The configured window size is larger than the maximum allowable value
#endif

/************************** UDP Settings ************************************
 *
 */

#if (INCLUDE_UDP == NU_TRUE)
#ifndef CFG_NU_OS_NET_STACK_CFG_H
#define UDP_MAX_PORTS   30                                  /* Maximum number of UDP ports.     */
#else
#define UDP_MAX_PORTS   CFG_NU_OS_NET_STACK_UDP_MAX_PORTS
#endif
#endif

#define UMAX_DGRAMS     5                                   /* Maximum number UDP datagrams     */
                                                            /* that can be buffered for a       */
                                                            /* single port.                     */

/************************** IPRaw Settings **********************************
 *
 */

#if (INCLUDE_IP_RAW == NU_TRUE)
#ifndef CFG_NU_OS_NET_STACK_CFG_H
#define IPR_MAX_PORTS   30                                  /* Maximum number of RAW IP ports.  */
#else
#define IPR_MAX_PORTS   CFG_NU_OS_NET_STACK_IPR_MAX_PORTS
#endif
#endif

#define IMAX_DGRAMS     10                                  /* Maximum number RAW IP datagrams */
                                                            /* that can be buffered for a       */
                                                            /* single port.                     */

/******************** PMTU Discovery Settings *******************************
 *
 */

#define PMTU_INC_TIME              (60 * (10 * SCK_Ticks_Per_Second))

#define PMTU_INC_INTERVAL          (60 * (1 * SCK_Ticks_Per_Second))


/************************** ARP Settings ************************************
 *
 */

#define CACHETO             ((UINT32)(400 * SCK_Ticks_Per_Second))  /* ARP cache timeout. */
#define ARPTO               ((UINT32)(1 * SCK_Ticks_Per_Second))    /* ARP retransmit timeout. */

#ifndef CFG_NU_OS_NET_STACK_CFG_H
#define ARP_CACHE_LENGTH    10                                      /* Size of the ARP cache. */
#else
#define ARP_CACHE_LENGTH    CFG_NU_OS_NET_STACK_ARP_CACHE_LENGTH
#endif

/************************** Logging *****************************************
 *
 */

#define PUTS(a)
#define PUTCH(a)

/*
 * This define will be used to specify the maximum number of errors allowed
 * in the system before they roll over.
 */
#define NLOG_MAX_ENTRIES          50

/* Max number of chars allowed for filename storage */
#define NLOG_MAX_FILENAME         50

/* Max number of chars allowed for error message storage */
#define NLOG_MAX_MSG_SIZE         64

/* Maximum buffer size for the message to be placed. */
#define NLOG_MAX_BUFFER_SIZE      256


/* The macros below deal with NET error reporting and also allow for
   tracking of IP, TCP, and UDP packets.  The items will be logged into
   an array of structures for each protocol.  The array for the NET errors
   can be found in nlog.c and the protocol logging array is also found in
   this file.
*/

/* Print the errors to a console */
#define PRINT_NET_ERROR_MSG         NU_FALSE

/* Log all IP header info to a tracking array */
#define INCLUDE_IP_INFO_LOGGING     NU_FALSE

/* Print the IP header to a console */
#define PRINT_IP_MSG                NU_FALSE

/* Log all TCP header info to a tracking array */
#define INCLUDE_TCP_INFO_LOGGING    NU_FALSE

/* Print the TCP header to a console */
#define PRINT_TCP_MSG               NU_FALSE

/* Log all UDP header info to a tracking array */
#define INCLUDE_UDP_INFO_LOGGING    NU_FALSE

/* Print the UDP header to a console */
#define PRINT_UDP_MSG               NU_FALSE

/* Log all ARP header info to a tracking array */
#define INCLUDE_ARP_INFO_LOGGING    NU_FALSE

/* Print the ARP header to a console */
#define PRINT_ARP_MSG               NU_FALSE

/* Log all ICMP header info to a tracking array */
#define INCLUDE_ICMP_INFO_LOGGING   NU_FALSE

/* Print the ICMP header to a console */
#define PRINT_ICMP_MSG              NU_FALSE

/* Send the Error/Info string to a remote client */
#define NLOG_INFO_SEND              NU_FALSE

#if (NLOG_INFO_SEND == NU_TRUE)

/* If it is desired to send the error/info out a network port to a remote
   client, then the port and IP address can be configured below.
*/
#define NLOG_PORT                   20001
#define NLOG_CLIENT_IP_0            200
#define NLOG_CLIENT_IP_1            100
#define NLOG_CLIENT_IP_2            200
#define NLOG_CLIENT_IP_3            100

/* The following two macros control the rate-limiting of messages transmitted
 * out a network port.  The purpose of these macros is to prevent an
 * infinite transmission of error messages out the network port when a
 * send error occurs during the transmission of the error message and to
 * limit the number of resources used to transmit error messages.
 * Configure these macros so the stack will transmit a maximum of
 * NLOG_COUNT_THRESH messages each NLOG_TIME_THRESH time period.
 */

/* The time period over which the rate-limiting is measured.  The default
 * is 2 seconds.
 */
#define NLOG_TIME_THRESH        (2 * TICKS_PER_SECOND)

/* The count of messages used in the rate-limiting.  The default is 5. */
#define NLOG_COUNT_THRESH       5

#endif

/************************** NET Debug ************************************
 *
 */

/* The length of the list of notification structures to pass to the API */
#define NET_NTFY_LIST_LENGTH        10

/* The maximum number of entries in the notification queue */
#define NET_NTFY_MAX_ENTRIES        10

/* The size of the notification HISR */
#define NET_NTFY_HISR_SIZE          NU_MIN_NET_STACK_SIZE



/************************** RARP *****************************************
 *
 */

/* The maximum number of RARP messages to send */
#define RARP_MAX_ATTEMPTS           5



/************************** ICMP *****************************************
 *
 */

/* The default timeout value used when sending an ICMPv4 Echo Request */
#define ICMP_DEFAULT_ECHO_TIMEOUT           (5 * SCK_Ticks_Per_Second)

/* The length of the data in the ICMP Echo Request packet */
#define ICMP_ECHO_REQUEST_LENGTH            32

/* The default time interval for rate limiting outgoing error messages */
#define ICMP_ERROR_MSG_RATE_LIMIT           (1 * SCK_Ticks_Per_Second)

/* The default message count for rate limiting outgoing error messages */
#define ICMP_ERROR_MSG_COUNT_RATE_LIMIT     1


/************************** DEVICES **************************************
 *
 */

/* The maximum length of a device name. The value should be a multiple of 4. */
#define DEV_NAME_LENGTH             16

/* The default routing metric to be used for each device in the system. */
#define DEV_DEFAULT_METRIC          1



/************************** DHCP *****************************************
 *
 */

/* in/out buffer size */
#define IOSIZE                      1500

/* The maximum size of the DHCP_Backoff array */
#define DHCP_MAX_BACKOFFS            8


/************************** DNS ******************************************
 *
 */

#ifndef CFG_NU_OS_NET_STACK_CFG_H

/* The max number of DNS servers that can be registered with Nucleus NET. */
#define DNS_MAX_DNS_SERVERS         5

/* The max number of IP addresses for each DNS host record */
#define DNS_MAX_IP_ADDRS            5

/* The maximum size of an incoming DNS message */
#define DNS_MAX_MESSAGE_SIZE        512

/* The maximum number of times to query a DNS server */
#define DNS_MAX_ATTEMPTS            5

/* The maximum number of MX records to return to a user. */
#define DNS_MAX_MX_RECORDS          10

/* The maximum name length of a MX server to return to a user. */
#define MAX_MX_NAME_SIZE            128

/* The TTL of a local host record created by mDNS, in seconds. */
#define MDNS_LOCAL_TTL              120

/* The max number of elements pending handling in the mDNS queue for
 * probing, announcing and continuous resolution.
 */
#define MDNS_EVENT_Q_NUM_ELEMENTS   8

/* The priority of the mDNS master task. */
#define MDNS_MASTER_TASK_PRIO       25

/* The priority of the mDNS wake task. */
#define MDNS_WAKE_TASK_PRIO         25

/* The size of the mDNS Master task. */
#define MDNS_STACK_SIZE             2000

/* The size of the mDNS wake task. */
#define MDNS_WAKE_STACK_SIZE        256

/* The callback signal used by mDNS to inform the application of a change to an
 * interested record.
 */
#define MDNS_SIGNAL                 31

#else
#define DNS_MAX_DNS_SERVERS             CFG_NU_OS_NET_STACK_DNS_MAX_DNS_SERVERS
#define DNS_MAX_IP_ADDRS                CFG_NU_OS_NET_STACK_DNS_MAX_IP_ADDRS
#define DNS_MAX_MESSAGE_SIZE            CFG_NU_OS_NET_STACK_DNS_MAX_MESSAGE_SIZE
#define DNS_MAX_ATTEMPTS                CFG_NU_OS_NET_STACK_DNS_MAX_ATTEMPTS
#define DNS_MAX_MX_RECORDS              CFG_NU_OS_NET_STACK_DNS_MAX_MX_RECORDS
#define MAX_MX_NAME_SIZE                CFG_NU_OS_NET_STACK_MAX_MX_NAME_SIZE
#define MDNS_LOCAL_TTL                  CFG_NU_OS_NET_STACK_MDNS_LOCAL_TTL
#define MDNS_EVENT_Q_NUM_ELEMENTS       CFG_NU_OS_NET_STACK_NUM_MDNS_Q_ELEMENTS
#define MDNS_MASTER_TASK_PRIO           CFG_NU_OS_NET_STACK_MDNS_TSK_PRIO
#define MDNS_WAKE_TASK_PRIO             CFG_NU_OS_NET_STACK_MDNS_WAKE_TSK_PRIO
#define MDNS_STACK_SIZE                 CFG_NU_OS_NET_STACK_MDNS_MASTER_TSK_SIZE
#define MDNS_WAKE_STACK_SIZE            CFG_NU_OS_NET_STACK_MDNS_WAKE_TSK_SIZE
#define MDNS_SIGNAL                     CFG_NU_OS_NET_STACK_MDNS_SIGNAL
#endif

#if (MDNS_WAKE_TASK_SIZE < NU_MIN_STACK_SIZE)
/* Redefine the task size to be the min for this platform. */
#undef  MDNS_WAKE_TASK_SIZE
#define MDNS_WAKE_TASK_SIZE             NU_MIN_STACK_SIZE
#endif

/************************** IP *******************************************
 *
 */

/* This value defines the minimum value that the maximum reassembly size
 * can be set to for a device.
 */
#define MIN_REASM_MAX_SIZE              576

/* Default TTL to put in Multicast Packets */
#define IP_DEFAULT_MULTICAST_TTL        1

/* Loopback is not yet supported. */
#define IP_DEFAULT_MULTICAST_LOOP       0

/* RFC 1122 recommends a default TTL for fragments of 60 to 120 seconds. */
#define IP_FRAG_TTL                     ((UINT32)(SCK_Ticks_Per_Second * 60))



/************************** RIP-II ***************************************
 *
 */

/* Set this macro to NU_TRUE to disable Split Horizon with Poisoned Reverse */
#define RIP2_DISABLE_POISONED_REVERSE       NU_FALSE

/* RFC 2453 - section 3.8 - The number of seconds between broadcasts of the
 * complete routing table
 */
#define RIP2_BTIME                          (30 * SCK_Ticks_Per_Second)

/* The maximum number of entries to collect when performing Garbage collection */
#define RIP2_MAX_DELETES                    25

/* The maximum number of routes to put in a RIP-II packet. */
#define RIP2_MAX_PER_PACKET                 25

/* RFC 2453 - section 3.8 - The number of seconds from the time a route is
 * set to be deleted and the time it is actually deleted.
 */
#define RIP2_DELETE_INTERVAL                (180 * SCK_Ticks_Per_Second)

/* RFC 2453 - section 3.8 - The number of seconds between performing
 * garbage collection on the routing table.
 */
#define RIP2_GARBAGE_COLLECTION_INTERVAL    (120 * SCK_Ticks_Per_Second)

/* The amount of time for a route to go unused before it is considered for
 * garbage collection.
 */
#define RIP2_RT_LIFE_TIME                   ((UINT32)(180 * SCK_Ticks_Per_Second))


/************************** IGMP ******************************************
 *
 */

#ifndef CFG_NU_OS_NET_STACK_CFG_H
/* Maximum number of multicast groups that are supported by a socket */
#define IP_MAX_MEMBERSHIPS          10
#else
#define IP_MAX_MEMBERSHIPS          CFG_NU_OS_NET_STACK_IP_MAX_MEMBERSHIPS
#endif

#ifndef CFG_NU_OS_NET_STACK_CFG_H
/* This is the maximum number of source addresses that can be maintained by
 *  each multicast group on a per device and per socket basis.  Therefore,
 *  the maximum number can be a cumulative effect.  For example, if a device
 *  supports several members that have a filter state of INCLUDE and if the
 *  number of source addresses that are in the INCLUDE list exceeds the
 *  MAX_MULTICAST_SRC_ADDR value, then the member that is attempting to
 *  join the group on that device will receive an error.
 */
#define MAX_MULTICAST_SRC_ADDR      10
#else
#define MAX_MULTICAST_SRC_ADDR      CFG_NU_OS_NET_STACK_MAX_MCAST_SRCADDR
#endif

/* Timers and Default Values */

/* Default IGMP version to be used.  This does not guarantee the version
 *  that the host will always operate, but this is the highest version that
 *  the host will run.  For example, if the value is IGMPV2_COMPATIBILITY,
 *  then the host will never run as a IGMPv3 host.  However, it could function
 *  as a IGMPv1 host.
 */
#define IGMP_DEFAULT_COMPATIBILTY_MODE      IGMPV3_COMPATIBILITY


/* RFC 3376 - section 8.1 - Allows tuning for the expected packet loss on
 * a link.  If a link is expected to be lossy, the Robustness Variable may
 * be increased.  IGMP is robust to (Robustness Variable - 1) packet losses.
 * The Robustness Variable must not be 0 and should not be 1.
 */
#define IGMP_ROBUST_VARIABLE        2

#if (IGMP_ROBUST_VARIABLE == 0)
    #error RFC 3376 - The Robustness Variable MUST not be 0.
#endif

/* RFC 3376 - section 8.2 - The interval between General Queries sent by
 * the Querier.  By varying the Query Interval, an administrator may tune
 * the number of IGMP messages on the link; larger values can cause MLD
 * queries to be sent less often.
 */
#define IGMP_QUERY_INTERVAL         (125 * SCK_Ticks_Per_Second) /* 125 seconds */

/* RFC 3376 - section 8.3 - The Maximum Response Delay inserted into the
 * periodic General Queries.  By varying the Query Response Interval, an
 * administrator may tune the burstiness of MLD messages on the link;
 * larger values make the traffic less bursty, as node responses are spread
 * out over a larger interval.  The number of seconds represented by the
 * Query Response Interval must be less than the Query Interval.
 */
#define IGMP_QUERY_RESPONSE_INTERVAL        (10 * SCK_Ticks_Per_Second)  /* 10 seconds */

/* RFC 3376 - section 8.4 - The amount of time that must pass before a
 * router decides there are no more listeners for an address on a link.
 * This value is not configurable.
 */
#define IGMP_GROUP_MEMBERSHIP_INTERVAL     (IGMP_ROBUST_VARIABLE * \
                                            IGMP_QUERY_INTERVAL +  \
                                            IGMP_QUERY_RESPONSE_INTERVAL)

/* RFC 3376 - section 8.5 - The length of time that must pass before a router
 * decides that there is no longer another router which should be the querier
 * on a link. This value is not configurable.
 */
#define IGMP_OTHER_QUERY_PRESENT_INTERVAL   (IGMP_ROBUST_VARIABLE * \
                                             IGMP_QUERY_INTERVAL +  \
                                             (IGMP_QUERY_RESPONSE_INTERVAL / 2))

/* RFC 3376 - section 8.6 - The interval between General Queries sent by a
 * Querier on startup.
 */
#define IGMP_STARTUP_QUERY_INTERVAL         (IGMP_QUERY_INTERVAL / 4)

/* RFC 3376 - section 8.7 - The number of queries sent out on startup,
 * separated by the Startup Query Interval.
 */
#define IGMP_STARTUP_QUERY_COUNT            IGMP_ROBUST_VARIABLE

/* RFC 3376 - section 8.8 - The Maximum Response Delay inserted into
 * Multicast-Address-Specific Queries sent in response to a Done Message, and
 * is also the amount of time between Multicast-Address-Specific Query
 * Messages.  This value may be tuned to modify the leave-latency of the link.
 * A reduced value results in reduced time to detect the departure of the last
 * listener for an address.
 */
#define IGMP_LAST_MEMBER_QUERY_INTERVAL   (1 * SCK_Ticks_Per_Second) /* 1 second */

/* RFC 3376 - section 8.9 - The number of Multicast-Address-Specific Queries
 * sent before the router assumes there are no remaining listeners for an
 * address on a link.
 */
#define IGMP_LAST_MEMBER_QUERY_COUNT      IGMP_ROBUST_VARIABLE

/* RFC 3376 - section 8.10 - The time between repetitions of a node's initial
 * report of interest in a multicast address.
 */
#define IGMP_UNSOLICITED_REPORT_INTERVAL    (1 * SCK_Ticks_Per_Second) /* 1 seconds */

/* RFC 3376 - section 8.12 - The time for transitioning a host back to IGMPv3 mode
 * once an older version query is heard.  When an older version query is received,
 * hosts set their Older Version Querier Present Timer to Older Version Querier Interval
 */
#define IGMP_OLDER_VERSION_QUERIER_PRESENT_TIMEOUT(device)   ((IGMP_ROBUST_VARIABLE * \
                                                    ((DV_DEVICE_ENTRY *)device)->dev_igmp_last_query_interval) + \
                                                    IGMP_QUERY_RESPONSE_INTERVAL)

/* RFC 3376 - section 8.13 - The time for transitioning a host back to IGMPv3 mode
 * once an older version report is heard.  When an older version report is received,
 * routers set their Older Host Present Timer to Older Host Present Interval
 */
#define IGMP_OLDER_HOST_PRESENT_INTERVAL       ((IGMP_ROBUST_VARIABLE * \
                                                 IGMP_QUERY_INTERVAL) + \
                                                 IGMP_QUERY_RESPONSE_INTERVAL)


/************************** USER MANAGEMENT ******************************
 *
 */

/* Min and Max string limits */
#define UM_MIN_NAME_SIZE            1      /* Minimum user name length */
#define UM_MAX_NAME_SIZE            32     /* Maximum user name length */
#define UM_MIN_PW_SIZE              1      /* Minimum password length  */
#define UM_MAX_PW_SIZE              32     /* Maximum password length  */

/* Change this macro to NU_FALSE to ignore case in the User Management
 * module.
 */
#define UM_CASE_SENSITIVE           NU_TRUE    /* case sensitivity flag */

/******************************* MIB-II **********************************
 *
 */

/* Exclude the gathering of the various MIB-II statistics from Nucleus NET
 * by setting the respective macro to NU_FALSE.
 */
#define MIB2_IP_INCLUDE       INCLUDE_MIB2_RFC1213
#define MIB2_IF_INCLUDE       INCLUDE_MIB2_RFC1213
#define MIB2_TCP_INCLUDE      INCLUDE_MIB2_RFC1213
#define MIB2_UDP_INCLUDE      INCLUDE_MIB2_RFC1213
#define MIB2_SYS_INCLUDE      INCLUDE_MIB2_RFC1213
#define MIB2_AT_INCLUDE       INCLUDE_MIB2_RFC1213
#define MIB2_ICMP_INCLUDE     INCLUDE_MIB2_RFC1213

/* Do not modify this section.  If IPv4 has been excluded from the build,
 * these MIB-II components must also be excluded.
 */
#if (INCLUDE_IPV4 == NU_FALSE)

#undef MIB2_IP_INCLUDE
#define MIB2_IP_INCLUDE       NU_FALSE

#undef MIB2_IF_INCLUDE
#define MIB2_IF_INCLUDE       NU_FALSE

#undef MIB2_TCP_INCLUDE
#define MIB2_TCP_INCLUDE      NU_FALSE

#undef MIB2_UDP_INCLUDE
#define MIB2_UDP_INCLUDE      NU_FALSE

#undef MIB2_ICMP_INCLUDE
#define MIB2_ICMP_INCLUDE     NU_FALSE

#undef MIB2_AT_INCLUDE
#define MIB2_AT_INCLUDE       NU_FALSE

#endif

/***************** IPv4 Link-Local Address Configuration *****************
 *
 */

/* The amount of time to delay before sending the initial ARP Probe. */
#define LL_PROBE_WAIT_TIME     ((UINT32)(1 * SCK_Ticks_Per_Second))

/* The minimum delay between ARP Probes. */
#define LL_PROBE_MIN_TIME      ((UINT32)(1 * SCK_Ticks_Per_Second))

/* The maximum delay between ARP Probes. */
#define LL_PROBE_MAX_TIME      ((UINT32)(2 * SCK_Ticks_Per_Second))

/* The maximum number of ARP Probes to transmit before transitioning to
 * the Annoucement phase.
 */
#define LL_PROBE_NUM           3

/* The maximum number of Announcements to transmit. */
#define LL_ANNOUNCE_NUM        2

/* The delay between Announcement packets. */
#define LL_ANNOUNCE_INTERVAL   ((UINT32)(2 * SCK_Ticks_Per_Second))

/* The amount of time to delay before sending the initial Announcement. */
#define LL_ANNOUNCE_WAIT        ((UINT32)(2 * SCK_Ticks_Per_Second))

/* The maximum number of conflicts to withstand before rate-limiting
 * ARP Probes for a new address.
 */
#define LL_MAX_COLLISIONS      10

/* The delay between successive ARP Probes once LL_MAX_COLLISIONS has
 * been reached.
 */
#define LL_RATE_LIMIT_INTERVAL ((UINT32)(60 * SCK_Ticks_Per_Second))

/******************** End Nucleus NET Configuration *************************/

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* ifndef NET_CFG_H */
