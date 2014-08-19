/*************************************************************************
*
*            Copyright Mentor Graphics Corporation 2012
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
* FILENAME
*
*      sntpc.h
*
* DESCRIPTION
*
*       This include file defines Nucleus SNTP Client API, constants
*       and data structures.
*
* DATA STRUCTURES
*
*      SNTPC_TIME
*      SNTPC_SERVER
*
* DEPENDENCIES
*
*      nu_net.h
*
*************************************************************************/
#ifndef SNTPC_H
#define SNTPC_H

#include "networking/nu_net.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Time Zone Standard Abbreviations.
 * Each Time Zone is defined as an offset of "minutes" relative to
 * the "Greenwich Mean Time".
 */
#define SNTPC_TIMEZONE_GMT          0    /* Greenwich Mean Time */
#define SNTPC_TIMEZONE_WET          0    /* Western Europe Time */
#define SNTPC_TIMEZONE_CET          60   /* Central Europe Time */
#define SNTPC_TIMEZONE_EET          120  /* Eastern Europe Time */
#define SNTPC_TIMEZONE_MSK          180  /* Moscow Time */
#define SNTPC_TIMEZONE_PERM_RUS     240  /* Perm Russia Time */
#define SNTPC_TIMEZONE_UZBEKISTAN   300  /* Uzbekistan Time */
#define SNTPC_TIMEZONE_BANGLADESH   360  /* Bangladesh Time */
#define SNTPC_TIMEZONE_BANGKOK      420  /* Bangkok Time */
#define SNTPC_TIMEZONE_AWST         480  /* Australian Western Standard Time */
#define SNTPC_TIMEZONE_SEOUL        540  /* Seoul, South Korea Time */
#define SNTPC_TIMEZONE_ACST         570  /* Australian Central Standard Time */
#define SNTPC_TIMEZONE_AEST         600  /* Australian eastern Standard Time */
#define SNTPC_TIMEZONE_MAGADAN      660  /* Magadan, Russia Time */
#define SNTPC_TIMEZONE_FIJI         720  /* Fiji Islands Time */
#define SNTPC_TIMEZONE_AZORES       -60  /* Azores Islands Time */
#define SNTPC_TIMEZONE_MIDATLANTIC  -120 /* Mid-Atlantic Time */
#define SNTPC_TIMEZONE_RIO          -180 /* Rio de Janeiro, Brazil Time */
#define SNTPC_TIMEZONE_AST          -240 /* Atlantic Standard Time */
#define SNTPC_TIMEZONE_EST          -300 /* Eastern Standard Time */
#define SNTPC_TIMEZONE_CST          -360 /* Central Standard Time */
#define SNTPC_TIMEZONE_MST          -420 /* Mountain Standard Time */
#define SNTPC_TIMEZONE_PST          -480 /* Pacific Standard Time */
#define SNTPC_TIMEZONE_AKST         -540 /* Alaska Standard Time */
#define SNTPC_TIMEZONE_HST          -600 /* Hawaiian Standard Time */
#define SNTPC_TIMEZONE_SAMOA        -660 /* Samoa Time */
#define SNTPC_TIMEZONE_ENIWETOK     -720 /* Eniwetok Time */


/* If it is desired not to use the macro SNTPC_SERVER_DAYLIGHT_SAVING, the
    following macros can be used in it's place. */
#define SNTPC_TIMEZONE_BST          60   /* British Summer Time */
#define SNTPC_TIMEZONE_IST          60   /* Irish Summer Time */
#define SNTPC_TIMEZONE_WEST         60   /* Western Europe Summer Time */
#define SNTPC_TIMEZONE_CEST         120  /* Central Europe Summer Time */
#define SNTPC_TIMEZONE_EEST         180  /* Eastern Europe Summer Time */
#define SNTPC_TIMEZONE_MSD          240  /* Moscow Summer Time */
#define SNTPC_TIMEZONE_ADT          -180 /* Atlantic Daylight Time */
#define SNTPC_TIMEZONE_EDT          -240 /* Eastern Daylight Time */
#define SNTPC_TIMEZONE_CDT          -300 /* Central Daylight Time */
#define SNTPC_TIMEZONE_MDT          -360 /* Mountain Daylight Time */
#define SNTPC_TIMEZONE_PDT          -420 /* Pacific Daylight Time */
#define SNTPC_TIMEZONE_AKDT         -480 /* Alaska Daylight Time */
#define SNTPC_TIMEZONE_AEDT         600  /* Australian Eastern Daylight Time */
#define SNTPC_TIMEZONE_ACDT         630  /* Australian Central Daylight Time */


/* Time zone letter abbreviations are sometimes used by military, NATO, or any
    organization that uses the 24 hour clock. */
#define SNTPC_TIMEZONE_Y            -720
#define SNTPC_TIMEZONE_X            -660
#define SNTPC_TIMEZONE_W            -600
#define SNTPC_TIMEZONE_V            -540
#define SNTPC_TIMEZONE_U            -480
#define SNTPC_TIMEZONE_T            -420
#define SNTPC_TIMEZONE_S            -360
#define SNTPC_TIMEZONE_R            -300
#define SNTPC_TIMEZONE_Q            -240
#define SNTPC_TIMEZONE_P            -180
#define SNTPC_TIMEZONE_O            -120
#define SNTPC_TIMEZONE_N            -60
#define SNTPC_TIMEZONE_Z             0
#define SNTPC_TIMEZONE_A             60
#define SNTPC_TIMEZONE_B             120
#define SNTPC_TIMEZONE_C             180
#define SNTPC_TIMEZONE_D             240
#define SNTPC_TIMEZONE_E             300
#define SNTPC_TIMEZONE_F             360
#define SNTPC_TIMEZONE_G             420
#define SNTPC_TIMEZONE_H             480
#define SNTPC_TIMEZONE_I             540
#define SNTPC_TIMEZONE_K             600
#define SNTPC_TIMEZONE_L             660
#define SNTPC_TIMEZONE_M             720

/* Backward-compatibility macros for legacy applications. */
#define SNTPC_DEFAULT_TIMEZONE          SNTPC_TIMEZONE_CST
#define SNTPC_DEFAULT_DAYLIGHT_SAVINGS  NU_FALSE

/* Number of seconds to adjust for daylight savings. */
#define SNTPC_DAYLIGHT_SAVINGS_ADJUST   3600

/* Error codes */
#define SNTPC_ALREADY_EXISTS        -11000
#define SNTPC_DOES_NOT_EXIST        -11001
#define SNTPC_LIST_FULL             -11002
#define SNTPC_NOT_SYNCED            -11003
#define SNTPC_OWP_NOT_RUN           -11004

/* This structure is used to keep time. It can be used to represent
 * either localtime or Prime Epoch time, depending on the context in
 * which it is used. */
typedef struct _sntpc_time
{
    UINT32          sntpc_seconds;      /* Seconds. */
    UINT32          sntpc_useconds;     /* Microseconds. */
    INT             sntpc_is_dst;       /* NU_TRUE for daylight-saving. */
} SNTPC_TIME;

/* Server structure */
typedef struct _sntpc_server
{
    struct addr_struct      sntpc_server_addr;
    CHAR            *sntpc_server_hostname; /* Hostname of server. If this
                                             * is non-NULL then it
                                             * overrides the IP address
                                             * specified in sntpc_server_addr.
                                             */
    UINT32          sntpc_poll_interval;/* Min. interval in seconds. Zero
                                         * for no polling */
    UINT16          sntpc_src_port;     /* UDP local port. Zero for
                                         * don't-care. */
    UINT8           sntpc_pad[2];
} SNTPC_SERVER;

/* Functions prototypes. */
STATUS SNTPC_Add_Server(SNTPC_SERVER *server);
STATUS SNTPC_Delete_Server(SNTPC_SERVER *server);
VOID   SNTPC_Purge_Server_List(VOID);
STATUS SNTPC_Server_Query(struct addr_struct *addr,
                          CHAR *server_hostname,
                          SNTPC_SERVER *ret_server);
STATUS SNTPC_Set_Timezone(INT16 timezone, UINT8 dst_enabled);
STATUS SNTPC_Get_Timezone(INT16 *timezone, UINT8 *dst_enabled);
STATUS SNTPC_Get_Time(SNTPC_TIME *current_time);
STATUS SNTPC_Get_Time_From_Server(SNTPC_TIME *current_time,
                                  SNTPC_SERVER *server);

#ifdef          __cplusplus
}

#endif /* _cplusplus */
#endif /* SNTPC_H */

