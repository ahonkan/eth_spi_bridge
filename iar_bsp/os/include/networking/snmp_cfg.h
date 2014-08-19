/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
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
*       snmp_cfg.h                                               
*
*   DESCRIPTION
*
*       This file contains the data structures used to
*       configure Nucleus SNMP
*
*   DATA STRUCTURES
*
*       snmp_cfig_t
*       SNMP_TARGET_ADDR_TABLE
*       SNMP_TARGET_PARAMS_TABLE_CONFIG
*       SNMP_NOTIFY_TABLE_CONFIG
*       SNMP_NOTIFY_FILTER_PROFILE_TABLE_CONFIG
*       SNMP_NOTIFY_FILTER_TABLE_CONFIG
*
*   DEPENDENCIES
*
*       nu_net.h
*       xtypes.h
*
************************************************************************/
#ifndef SNMP_CFG_H
#define SNMP_CFG_H

#include "networking/nu_net.h"
#include "networking/xtypes.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#include "nucleus_gen_cfg.h"

#if (CFG_NU_OS_NET_STACK_INCLUDE_IPV4 == NU_FALSE)
#error SNMP requires IPv4. Please enable the "include_ipv4" option in NET stack to use SNMP.
#endif

#define     SNMP_2_3            1   /* SNMP 2.3 */
#define     SNMP_2_4            2   /* SNMP 2.4 */
#define     SNMP_VERSION_COMP   SNMP_2_4

/* Following three macros specify the message processing models to be
 * included in the build.
 */
#define INCLUDE_SNMPv1                  CFG_NU_OS_NET_SNMP_INCLUDE_SNMPV1
#define INCLUDE_SNMPv2                  CFG_NU_OS_NET_SNMP_INCLUDE_SNMPV2
#define INCLUDE_SNMPv3                  CFG_NU_OS_NET_SNMP_INCLUDE_SNMPV3

#if ((INCLUDE_SNMPv3 == NU_TRUE) && !defined(CFG_NU_OS_NET_SSL_OPENSSL_ENABLE))
#error SNMPv3 requires openssl/crypto
#endif

/* Enable Nucleus NAFEM MIBS. */
#define INCLUDE_NAFEM_MIB               NU_FALSE

/* Define whether to include any PPP MIBS (RFC 1471, 1472 & 1473). Each
   individual PPP MIB can be added or removed from within the PPP
   configuration header, /os/include/drivers/ppp_cfg.h. */
#define INCLUDE_PPP_MIB                 NU_FALSE

/* The Following macros define inclusion/exclusion of various components
 * of SNMP MIB. Please set all those macros to NU_FALSE whose
 * corresponding component is not desired in the build.
 */
#define INCLUDE_MIB_USM                 NU_TRUE
#define INCLUDE_MIB_VACM                NU_TRUE
#define INCLUDE_MIB_SNMP_ENGINE         NU_TRUE
#define INCLUDE_MIB_CBSM                NU_TRUE
#define INCLUDE_MIB_MPD                 NU_TRUE
#define INCLUDE_MIB_TARGET              NU_TRUE
#define INCLUDE_MIB_NO                  NU_TRUE

/* The SNMP Init task is responsible for initializing SNMP when Nucleus
 * Middle-ware Initialization is being used.
 */
#define SNMP_INIT_STACK_SIZE        3000
#define SNMP_INIT_PRIORITY          1   
#define SNMP_INIT_TIME_SLICE        0
#define SNMP_INIT_PREEMPT           NU_PREEMPT

/* Define the following macro to NU_TRUE if configuration through the
 * serial interface is desired.
 */
#define SNMP_CONFIG_SERIAL              NU_FALSE

/* The following macros define the buffer sizes that for the message
 * processing models. These buffers are used during the processing
 * of a request. SNMPv1 and SNMPv2 buffers should be greater than
 * 150 and SNMPv3 buffer should be greater than 300.
 */
#define SNMP_V1_BUFFER_SIZE             1000
#define SNMP_V2_BUFFER_SIZE             1000
#define SNMP_V3_BUFFER_SIZE             1400

/* The following macro defines the maximum number of SNMP objects that
 * can be part of a PDU.
 */
#define AGENT_LIST_SIZE                 16

/* -------- Configurations for the User-based Security Model. --------- */

/* SNMP specifications specify a conversion function that converts a
 * USM password in to a key which is then used for either authentication
 * or privacy. This conversion is processor intensive and can lead to
 * large initialization times as well as system slow-down on slow
 * processors. By setting the following macro to NU_FALSE, this
 * conversion is disabled. Users will then be responsible for
 * passing a key instead of a password to the USM module. A command-line
 * utility has been provided which does the conversion from password
 * to key. This utility can be used to generate the keys.
 */
#define USM_PASSWORD_2_KEY              NU_TRUE

/* Information for the first user to be setup. These values
 * are configured through the configuration utility. Please
 * refer to snmp_cfg.c for a list of all users that are
 * added during configuration.
 */
#define USM_USER_NAME_INITIAL           "initial"
#define USM_AUTH_PASSWORD_INITIAL       "authentic"
#define USM_PRIV_PASSWORD_INITIAL       "private8"

/* Specify the IP addresses that the Agent knows of by default.
 * These are added to the Target hosts table in snmp_cfg.c.
 */
#define SNMP_HOST1_ADDRESS              {192,168,0,22}
#define SNMP_HOST2_ADDRESS              {192,168,0,22}
#define SNMP_HOST3_ADDRESS              {192,168,0,22}
#define SNMP_HOST4_ADDRESS              {192,168,0,22}
#define SNMP_HOST5_ADDRESS              {192,168,0,22}

/* Number of USM Users that are added during configuration of
 * USM.
 */
#define USM_MAX_USER_USERS              3

/* Total no. of Authentication Protocols. */
#define USM_MAX_AUTH_PROTOCOLS          3

/* Total no. of Privacy Protocols. */
#define USM_MAX_PRIV_PROTOCOLS          2

/* ----- End of Configurations for the User-based Security Model. ----- */

/* ---------- Configurations for the CBSMv1 and CBSMv2. --------------- */

/* Number of CBSM communities that will be added during configuration
 * of CBSM.
 */
#define CBSM_MAX_COMMUNITIES            1

/* -------- End of Configurations for the CBSMv1 and CBSMv2. ---------- */

/* ----- Configurations for the View-based Access Control Model. ------ */

/* The following define gives the number of context names that will be
 * added during initializations.
 */
#define VACM_CONTEXT_TBL_SIZE           1

/* The following define gives the number of security to group entries that
 * will be added during configuration.
 */
#define VACM_SEC2GRP_TBL_SIZE           5

/* The following define gives the number of access entries that will be
 * added during configuration.
 */
#define VACM_ACCESS_TBL_SIZE            2

/* The following define gives the number of views that will be
 * added during configuration.
 */
#define VACM_MIB_VIEW_TBL_SIZE          2

/* ---- End of Configurations for View-based Access Control Model. ---- */

/* ------- Configuration of Notification Originator Application  ------ */

/* Each notification type may have a number of objects that it will send
 * with in the notification message. The following macro defines the
 * largest number of such objects that can be sent. Note that this number
 * does not include sysUptime and the notification OID that is always sent
 * with an SNMPv2 and SNMP v3 notification.
 */
#define SNMP_MAX_NOTIFICATION_OBJECTS   3

/* Number of entries to be added to the Notify table during initialization.
 */
#define NOTIFY_TBL_SIZE                 1

/* Number of entries to be added to the Filter Profile Table during
 * initialization.
 */
#define FLTR_PROF_TBL_SIZE              5

/* Number of entries to be added to the Filter Table during initialization.
 */
#define FLTR_TBL_SIZE                   1

/* --- End of Configuration for Notification Originator Application --- */

/* ----------------- Configuration of SNMP Targets -------------------- */

/* Number of entries to be added to the Target Address Table during
 * initialization.
 */
#define TGR_ADDR_TBL_SIZE               5

/* Number of entries to be added to the Target Params Table during
 * initialization.
 */
#define TGR_PARAMS_TBL_SIZE             5

/* -------------- End of Configuration of SNMP Targets ---------------- */

/* ---------------- Configuration of SNMP File Storage ---------------- */

/* Defines the storage type to be used for table entries that are added
 * during initial configuration.
 */
#define SNMP_STORAGE_DEFAULT            SNMP_STORAGE_NONVOLATILE

/* Define this macro to NU_TRUE if storage to a file system is required.
 */
#define SNMP_ENABLE_FILE_STORAGE        NU_FALSE

/* Following two macros define SNMP drive and directory where SNMP related
 * files will be stored.
 */
#define SNMP_DRIVE                      0
#define SNMP_DIR                        "snmp"

/* File name for SNMP Engine data. */
#define SNMP_FILE                       "engine.dat"

/* The USM data will be stored in this file. */
#define USM_FILE                        "usm.dat"

/* The CBSM data will be stored in this file. */
#define CBSM_FILE                       "cbsm.dat"

/* The target address table of Target MIB is stored in this file. */
#define TARGET_ADDR_FILE                "target_addr.dat"

/* The target params table of Target MIB is stored in this file. */
#define TARGET_PARAMS_FILE                "target_params.dat"

/* The VACM data will be stored in these files. */
#define VACM_SEC2GROUP_STRUCT_FILE      "group.dat"
#define VACM_ACCESS_STRUCT_FILE         "access.dat"
#define VACM_VIEWTREE_STRUCT_FILE       "viewtree.dat"

/* ------------- End of Configuration of SNMP File Storage ------------ */

/* ------------------ SNMP System Configuration ----------------------- */

/* The IP address length. */
#define SNMP_IP_ADDR_LEN                MAX_ADDRESS_SIZE

/* Following define specifies the number of requests that can be pending
 * with a Command Responder application at a particular time.
 */
#define SNMP_CR_QSIZE                   1

/* The enterprise number as assigned by IANA in hex. */
#define SNMP_IANA_NUMBER                0x00000450UL

/* Different string lengths. */
#define SNMP_MAX_STR                    MIB2_MAX_STRSIZE
#define MAX_SYS_STRINGS                 MIB2_MAX_STRSIZE
#define SNMP_HOSTNAME_LEN               16
#define SNMP_COMMNAME_LEN               16
#define DATESTRLENGTH                   18
#define SNMP_BUFSIZE                    1024

/* Specifies the 1 second interval. */
#define SNMP_HZ                         1000

/* --------------- End of SNMP System Configuration ------------------- */

/* ------------------ End of Configurable Section --------------------- */

/* Check whether the buffer size is valid. */
#if ((SNMP_V1_BUFFER_SIZE < 150) || (SNMP_V2_BUFFER_SIZE < 150) || (SNMP_V3_BUFFER_SIZE < 300))
#error The buffer size should be greater than 150 for SNMPv1 and SNMPv2 and 300 for SNMPv3.
#endif

#if ((INCLUDE_MIB2_RFC1213 == NU_FALSE) && (INCLUDE_SNMP == NU_TRUE))
#error RFC1213 must be included in order to use SNMP.
#endif

/* Defining no. of Message Processing models */
#if( INCLUDE_SNMPv1 && INCLUDE_SNMPv2 && INCLUDE_SNMPv3)
#define SNMP_MP_MODELS_NO           3
#else
#if( (INCLUDE_SNMPv1 && INCLUDE_SNMPv2) || (INCLUDE_SNMPv1 && INCLUDE_SNMPv3) || (INCLUDE_SNMPv2 && INCLUDE_SNMPv3) )
#define SNMP_MP_MODELS_NO           2
#else
#define SNMP_MP_MODELS_NO           1
#endif
#endif


/* Defining no. of Security models. */
#if( INCLUDE_SNMPv1 && INCLUDE_SNMPv2 && INCLUDE_SNMPv3)
#define SNMP_SS_MODELS_NO           3
#else
#if( (INCLUDE_SNMPv1 && INCLUDE_SNMPv2) || (INCLUDE_SNMPv1 && INCLUDE_SNMPv3) || (INCLUDE_SNMPv2 && INCLUDE_SNMPv3) )
#define SNMP_SS_MODELS_NO           2
#else
#define SNMP_SS_MODELS_NO           1
#endif
#endif

typedef struct snmp_cfig_s
{
    /* SNMP System Group */
    INT8            sys_contact[SNMP_MAX_STR];      /* Company name       */
    INT8            sys_description[SNMP_MAX_STR];  /* SNMP, sys description    */
    INT8            sys_location[SNMP_MAX_STR];     /* here, place of sys */
    INT8            sys_name[SNMP_MAX_STR];         /* IP host name       */
    UINT8           sys_objectid[SNMP_MAX_STR];     /* Enterprise MIB ID  */
    UINT32          sys_services;                   /* MIBII_SERVICES     */

    /* SNMP Traps */
    UINT8           trapComm[SNMP_MAX_STR];         /* trap community name*/
    UINT32          trapLen;                        /* trap name length   */
    UINT32          authentrap_enable;              /* enable authen traps*/
    UINT32          coldtrap_enable;                /* enable cold traps */

    UINT16          totalInterfaces;

    /* Make the structure word aligned. */
    UINT8           snmp_pad[2];

    /* SNMP Ports */
    UINT32          local_port;                     /* SNMP_PORT         */

#if (INCLUDE_MIB_RMON1 == NU_TRUE)
    /* XRMON1 (RFC1757) Control */
    UINT32          cbuff_size;                     /* 256k               */
    UINT32          host_maxnrhosts;                /* 2000               */
    UINT32          matrix_maxnrsrcdsts;            /* 5000               */
    UINT32          hist_maxnrbuckets;              /* 2000               */
    UINT32          event_maxnrlogs;                /* 1000               */
    UINT32          disc_maxnrnodes;                /* 8000               */
    UINT32          disc_nodetimeout;               /* 10000              */
    UINT32          topn_maxnrentries;              /* 100                */
#endif


} snmp_cfig_t;

typedef struct snmp_target_addr_table
{
    CHAR      snmp_target_addr_name[32];    /* null terminated */
    UINT32    snmp_target_addr_tDomain;
    UINT8     snmp_target_addr_tAddress[MAX_ADDRESS_SIZE];
    UINT8     snmp_target_addr_tfamily;
    INT32     snmp_target_addr_trsvd1;
    UINT8     snmp_target_addr_tag_list[32];
    UINT32    tag_list_len;
    CHAR      snmp_target_addr_params[32];
    INT32     snmp_target_addr_trsvd2;
    UINT16    snmp_target_addr_port_num;
    UINT8     snmp_pad;

} SNMP_TARGET_ADDR_TABLE;

typedef struct snmp_target_params_table_config
{
    UINT8           snmp_params_name[32];
    INT8            snmp_mp_model;
    INT8            snmp_security_model;
    UINT8           snmp_security_name[32];
    INT8            snmp_security_level;

    /* Make the structure word aligned. */
    UINT8           snmp_pad[1];
}SNMP_TARGET_PARAMS_TABLE_CONFIG;

typedef struct snmp_notify_table_config
{
    UINT8           snmp_table_name[32];
    UINT8           snmp_target_tag_name[32];
    INT8            snmp_target_tag_length;

    /* Make the structure word aligned. */
    UINT8           snmp_pad[3];
}SNMP_NOTIFY_TABLE_CONFIG;

typedef struct snmp_notify_filter_profile_table_config
{
    UINT8           snmp_params_name[32];
    UINT8           snmp_filter_name[32];
}SNMP_NOTIFY_FILTER_PROFILE_TABLE_CONFIG;

typedef struct snmp_notify_filter_table_config
{
    UINT8           snmp_filter_name[32];
    UINT32          snmp_notify_filter_subtree[32];
    INT8            snmp_subtree_len;
    UINT8           snmp_notify_filter_mask[4];
    INT8            snmp_mask_len;    /* no. of octets */
    INT8            snmp_filter_type;

    /* Make the structure word aligned. */
    UINT8           snmp_pad[1];
}SNMP_NOTIFY_FILTER_TABLE_CONFIG;

VOID    set_hostid(UINT32 ip);

#define SNMP_SET_HOST_ID        set_hostid

#ifdef          __cplusplus
}
#endif /* __cplusplus */
#endif /* SNMP_CFG_H */


