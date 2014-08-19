/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       ppp_cfg.h
*
*   COMPONENT
*
*       CONF - PPP Configuration
*
*   DESCRIPTION
*
*       This file contains all configuration options used by PPP. These can
*       be modified in order to fit the specific needs of the application.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_INC_PPP_CFG_H
#define PPP_INC_PPP_CFG_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

/* These definitions control which version of PPP the build should be
   compatible with. This should allow new versions of PPP to be shipped but
   remain compatible with applications designed for previous versions. */

#define     PPP_2_5         1       /* PPP 2.5 */
#define     PPP_3_1         2       /* PPP 3.1 */
#define     PPP_3_2         3       /* PPP 3.2 */
#define     PPP_3_3         4       /* PPP 3.3 */

#define PPP_VERSION_COMP    PPP_3_3        /* The version for which compatibility
                                              is desired. */

/* Do not redefine these macros if using the Nucleus builder to compile the
 * code.  If not using the Nucleus builder, all macros must be explicitly
 * configured here; otherwise, configure the respective .metadata file.
 */

#ifndef CFG_NU_OS_DRVR_PPP_CFG_H

/* Define this macro to NU_TRUE to use Direct Connect Cable Protocol.
   This involves direct cable (e.g. Null Modem cable) between PPP client
   and PPP server. Keeping this macro to NU_FALSE will remove Direct Cable
   Protocol code from PPP to reduce the code size in the PPP library */
#define INCLUDE_PPP_DC_PROTOCOL         NU_FALSE

/* Define this macro to NU_TRUE to enable PPP Multilink Protocol.
   Keeping this macro to NU_FALSE will remove MP code from PPP
   to reduce the code size in the PPP library */
#define INCLUDE_PPP_MP                  NU_FALSE

/* Use the Nucleus Net User Management (UM) module for PPP user
   accounts. If set to NU_FALSE, the simple structure in pw_list.c
   will be used. This option applies only if used as a PPP server. */
#define PPP_ENABLE_UM_DATABASE          NU_TRUE

/* This defines whether Caller Line Identification (CLI) is to be
   supported or not. If the modems or lines does not support CLI then
   change it to NU_FALSE */
#define PPP_ENABLE_CLI                  NU_FALSE

/* If we are a server, then the PAP, CHAP and MSCHAP(v1 & v2) flags
   identify the type of authentication we intend to request from the
   client. If we are a client, then these flags identify what we are
   capable of satisfying for a server. Keeping the macros to NU_FALSE
   will remove authentication protocol specific code's from PPP to
   reduce the code size in the PPP library. Note that to enable MSCHAP
   v1 or v2, PPP_USE_CHAP must be true. Nucleus Cipher Suite library
   must also be present to use CHAP protocols. */
#define PPP_USE_CHAP                    NU_TRUE
#define PPP_USE_PAP                     NU_TRUE
#define PPP_USE_CHAP_MS1                NU_TRUE
#define PPP_USE_CHAP_MS2                NU_TRUE

/* The following constants either enable or disable the PPP options
   we want to send to the peer. They do not restrict the options
   that are requested by the peer. */

/* Will the above local ACCM option be used? */
#define PPP_USE_ACCM                    NU_TRUE

/* Will the max receive unit option be used? This option should
   only be used if the MRU is different from the default size of
   1500 bytes. */
#define PPP_USE_MRU                     NU_FALSE

/* Will protocol field compression be used? */
#define PPP_USE_PFC                     NU_FALSE

/* Will address and control field compression be used? */
#define PPP_USE_ACC                     NU_FALSE

/* Will the magic number configuration option be used? */
#define PPP_USE_MAGIC                   NU_TRUE

/* Will the MPPE protocol be used for encryption. */
#define PPP_ENABLE_MPPE                 NU_FALSE

/* Whether encryption is mandatory. */
#define PPP_REQUIRE_ENCRYPTION          NU_TRUE

/* Enable the type of MPPE encryption to be used. */
#define PPP_USE_128_BIT_ENCRYPTION      NU_TRUE
#define PPP_USE_56_BIT_ENCRYPTION       NU_TRUE
#define PPP_USE_40_BIT_ENCRYPTION       NU_TRUE

/* When dialing into a PPP server it is possible to obtain DNS server
   addresses automatically. This macro defines the default setting for
   getting these addresses. Note that both primary and secondary addresses
   can be requested from the PPP server. */

/* Default for the primary address. */
#define PPP_USE_DNS1                    NU_TRUE

/* Default for the secondary address. PPP_USE_DNS1 must be set in order
   to use this option. */
#define PPP_USE_DNS2                    NU_TRUE


#else

#define INCLUDE_PPP_DC_PROTOCOL         CFG_NU_OS_DRVR_PPP_DC_PROTOCOL_ENABLE
#define INCLUDE_PPP_MP                  CFG_NU_OS_DRVR_PPP_MP_PROTOCOL_ENABLE
#define PPP_ENABLE_UM_DATABASE          CFG_NU_OS_DRVR_PPP_UM_DATABASE_ENABLE
#define PPP_ENABLE_CLI                  CFG_NU_OS_DRVR_PPP_CLI_ENABLE
#define PPP_USE_CHAP                    CFG_NU_OS_DRVR_PPP_USE_CHAP
#define PPP_USE_PAP                     CFG_NU_OS_DRVR_PPP_USE_PAP
#define PPP_USE_CHAP_MS1                CFG_NU_OS_DRVR_PPP_USE_CHAP_MS1
#define PPP_USE_CHAP_MS2                CFG_NU_OS_DRVR_PPP_USE_CHAP_MS2
#define PPP_USE_ACCM                    CFG_NU_OS_DRVR_PPP_USE_ACCM
#define PPP_USE_MRU                     CFG_NU_OS_DRVR_PPP_USE_MRU
#define PPP_USE_PFC                     CFG_NU_OS_DRVR_PPP_USE_PFC
#define PPP_USE_ACC                     CFG_NU_OS_DRVR_PPP_USE_ACC
#define PPP_USE_MAGIC                   CFG_NU_OS_DRVR_PPP_USE_MAGIC
#define PPP_ENABLE_MPPE                 CFG_NU_OS_DRVR_PPP_ENABLE_MPPE
#define PPP_REQUIRE_ENCRYPTION          CFG_NU_OS_DRVR_PPP_REQUIRE_ENCRYPTION
#define PPP_USE_128_BIT_ENCRYPTION      CFG_NU_OS_DRVR_PPP_USE_128_BIT_ENCRYPTION
#define PPP_USE_56_BIT_ENCRYPTION       CFG_NU_OS_DRVR_PPP_USE_56_BIT_ENCRYPTION
#define PPP_USE_40_BIT_ENCRYPTION       CFG_NU_OS_DRVR_PPP_USE_40_BIT_ENCRYPTION
#define PPP_USE_DNS1                    CFG_NU_OS_DRVR_PPP_USE_DNS1
#define PPP_USE_DNS2                    CFG_NU_OS_DRVR_PPP_USE_DNS2
#endif

/* This defines the total length that a password and id can be.
   Note: This is only used if PPP_ENABLE_UM_DATABASE above is set
   to NU_FALSE. */
#define PPP_MAX_ID_LENGTH               32
#define PPP_MAX_PW_LENGTH               32

/* Enable statistics collection for SNMP agent. Each of these
   constants will enable the PPP MIBs that are defined in
   RFC1471, RFC1472, and RFC1473 respectively. Note: Enabling
   the MIB data collection does not require the SNMP agent. */
#ifndef INCLUDE_LCP_MIB

#define INCLUDE_LCP_MIB                 NU_FALSE
#define INCLUDE_SEC_MIB                 NU_FALSE
#define INCLUDE_NCP_MIB                 NU_FALSE

#endif

/* Set the default timeout for link negotiation. If all phases of
   PPP link negotiation do not complete within this time the
   connection attempt will be aborted. */
#define PPP_DEFAULT_NEG_TIMEOUT         (45 * TICKS_PER_SECOND)

/* Define this macro to NU_FALSE if interrupt driven transmission is
   desired .Otherwise polled transmission will be used. */
#define HDLC_POLLED_TX                  NU_TRUE

/******************** Multilink Protocol Configuration ******************/

/* Endpoint discriminator for the MAC class. */
#define PPP_MP_END_MAC      {0x00, 0x0A, 0x00, 0x0B, 0x00, 0x0C}

/* Minimum fragment size to be transmitted, in bytes. */
#define PPP_MP_MIN_FRAG_SIZE               300

/* Max Receive Reconstructed Unit size. */
#define PPP_MP_DEFAULT_MRRU_SIZE           1500

/* Will short sequence number header format be used? */
#define PPP_MP_DEFAULT_USE_SHORT_SEQ_NUM   NU_FALSE

/******************** End Multilink Protocol Configuration **************/

/*********************** LCP Configuration ******************************/

/* These defines are for monitoring the modem link.

   LCP_MAX_ECHO
   is the number of times we will transmit an echo request without
   receiving a reply. After this value is reached the link will
   be considered to be closed. A zero value will disable echo requests,
   although responses are still sent to a requesting peer.

   LCP_ECHO_VALUE
   is the time to wait between echo requests. This value is in clock
   ticks and is specific to the target hardware. A value that is too
   small will just congest the network and slow the PPP link down. It also
   may not give enough time for the reply to come back, in which case the
   link will be considered to be closed even when it is not. */
#define LCP_MAX_ECHO                    2
#define LCP_ECHO_VALUE                  (20 * TICKS_PER_SECOND)

/* This defines which ASCII characters, 0 through 32, will be affected
   by transparency. On some links there may be hardware devices that add
   characters or effect the control characters. This will ensure that the
   control characters are transparent to these devices and that if a device
   adds any characters they will be dropped. Each bit represents its
   corresponding ASCII characters. By default all characters 0 - 32 are
   made transparent. */

/* This define is the default for the foreign end of the link. */
#define HDLC_FOREIGN_DEFAULT_ACCM       0xFFFFFFFFUL

/* This define is for the local end of the link. */
#define HDLC_LOCAL_DEFAULT_ACCM         0xFFFFFFFFUL


/* The following defines are the number of times PPP will retransmit
   the packet.

   LCP_MAX_CONFIGURE    is for configure request packets, LCP and NCP.
   LCP_MAX_TERMINATE    is for terminate request packets.
   LCP_MAX_AUTHENTICATE is for authentication packets, PAP and CHAP.
*/
#define LCP_MAX_CONFIGURE               3
#define LCP_MAX_TERMINATE               3
#define LCP_MAX_AUTHENTICATE            3


/*  This timeout value is the time PPP will wait before retransmitting
    the above mentioned packet types. This value is in clock ticks
    an is specific to the target hardware. */
#define LCP_TIMEOUT_VALUE               (3 * TICKS_PER_SECOND)

/*************************** End LCP ************************************/

/*********************** HDLC Configuration *****************************/

/* This defines the number of buffers in the LISR RX ring buffer. This
   ring buffer is used to hold packets that are being built or have been
   built and are waiting to be processed. This value should not be made
   too small or else packets may become overwritten within the buffer.
   Of course this value is highly dependent on the load placed on the CPU
   and the CPU's processing capabilities. Note that there are these many
   buffers for each UART being used.
*/
#define HDLC_MAX_HOLDING_PACKETS        3

/* This defines the number of pointers in a ring buffer used to pass
   completed packets from the RX LISR to the PPP HISR. These pointers
   point to the buffers mentioned above. The more UARTs that are being
   used the greater this value should be. Example: the above macro
   says that there will be 3 buffers for each UART. If two UARTs are being
   used then there will be 6 total RX buffers. There should be enough
   pointers to pass those buffers to the RX HISR.
*/
#define HDLC_MAX_HOLDING_PACKETS_PTR    6

/* After a packet has been completely transmitted the device that completed
   the transmission is passed to the TX HISR. This is done so that the
   TX HISR can deallocated the buffers used by the packet and so it can
   send the next packet that is in queue, if there is one. This macro
   defines how many pointers are available to pass the device to the
   TX HISR.
*/
#define HDLC_MAX_TX_QUEUE_PTRS          6

/*************************** End HDLC ***********************************/

/*********************** Modem Configuration ****************************/

/* The default number of rings before the phone is answered. To change this
   simply change the 2 to the number of rings that is required.

   The number of rings can also be changed during run time by the
   MDM_Rings_To_Answer() routine.
*/
#define     MDM_RINGS_TO_ANSWER_ON      2

/*************************** End Modem **********************************/

/************************ PPP Task Configuration ************************/

/* Settings for the PPP task are defined below. */
#define PPP_TASK_STACK_SIZE             2000
#define PPP_TASK_PRIORITY               3
#define PPP_TASK_TIME_SLICE             0
#define PPP_TASK_PREEMPT                NU_PREEMPT

/*********************** End PPP Task Configuration *********************/

/*********************** Debug Message Configuration ********************/

/* These defines are used for debugging.

   NU_DEBUG_PPP    will turn on the printing of information about
                   link negotiation.

   PPP_Printf      will direct where to print the above information.

   DEBUG_PKT_TRACE will capture all hdlc packets sent and received,
                   including the escape characters.

   PKT_TRACE_SIZE  defines the size of the packets trace buffers. Note
                   that these are circular buffers.


To use the DEBUG_PKT_TRACE capture, you can define _PRINTF to an
output function appropriate for your target and tools, or you can
send it to a memory buffer. You will need to define the two global
variables below in your application or some other source file for
this to work. Also, this method will not work for print statements
that try to print variables.

Memory buffer:

    extern char  ppp_log[100][50];
    extern char  pindex;

    #define _PRINT(x)           sprintf(ppp_log[(pindex++ % 100)], x)

    #define DEBUG_PKT_TRACE
    #define PKT_TRACE_SIZE      10000

*/

/* Define this to use an output function appropriate for your tools
   and target. All debug information is output using this macro. */
#define PPP_Printf                      /* printf */

/* Each of these will turn printing on or off for debug information
   pertaining to the individual protocols. Check the top of each file for
   even more detailed configuration of the PPP output. */
#define HDLC_DEBUG_PRINT_OK             NU_FALSE
#define MDM_DEBUG_PRINT_OK              NU_FALSE
#define PPP_DEBUG_PRINT_OK              NU_FALSE
#define LCP_DEBUG_PRINT_OK              NU_FALSE
#define NCP_DEBUG_PRINT_OK              NU_FALSE
#define CHAP_DEBUG_PRINT_OK             NU_FALSE
#define PAP_DEBUG_PRINT_OK              NU_FALSE
#define MP_DEBUG_PRINT_OK               NU_FALSE
#define CCP_DEBUG_PRINT_OK              NU_FALSE

/* If PPP_Printf is defined as a standard printf or sprintf function,
   then the raw PPP negotiation packets can be output by defining this
   macro to NU_TRUE. Also, if your target has a file system, then
   define PPP_LOG_TO_FILE to also write the packet trace to the file
   defined by PPP_LOG_FILENAME. */
#define PPP_LOG_PACKET                  NU_FALSE
#define PPP_LOG_TO_FILE                 NU_FALSE
#define PPP_LOG_FILENAME                "c:\\nucleus\\ppp\\ppplog.txt"


/******************** End Debug Message Configuration *******************/

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_PPP_CFG_H */
