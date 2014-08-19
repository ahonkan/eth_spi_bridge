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
*       snmp_prt.h                                               
*
*   DESCRIPTION
*
*       User system configuration file.
*
*   DATA STRUCTURES
*
*       xq_t
*       tmr_t
*
*   DEPENDENCIES
*
*       target.h
*       timer.h
*
************************************************************************/

#ifndef SNMP_PRT_H
#define SNMP_PRT_H

#include "networking/target.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/* Define whether to include RMON (RFC 1757) */
#define INCLUDE_MIB_RMON1 NU_FALSE

/* Determine whether this is an RMON LITE build */
#if (INCLUDE_MIB_RMON1 == NU_TRUE)
#include "rmon/inc/1757xxxx.h"
#if ( (RFC1757_ALRM_INCLUDE == NU_TRUE) &&  \
      (RFC1757_EVNT_INCLUDE == NU_TRUE) &&  \
      (RFC1757_HIST_INCLUDE == NU_TRUE) &&  \
      (RFC1757_STAT_INCLUDE == NU_TRUE) &&  \
      (RFC1757_CAP_INCLUDE == NU_FALSE) &&  \
      (RFC1757_FILT_INCLUDE == NU_FALSE) && \
      (RFC1757_HOST_INCLUDE == NU_FALSE) && \
      (RFC1757_MATX_INCLUDE == NU_FALSE) && \
      (RFC1757_TOPN_INCLUDE == NU_FALSE) )
#define RFC1757_RMON_LITE   NU_TRUE
#else
#define RFC1757_RMON_LITE   NU_FALSE
#endif
#else
#define RFC1757_RMON_LITE   NU_FALSE
#endif

/*---------------------- SNMPv1/MIB2 Defines --------------------------*/
#define MIBII_SYSCONTACT        "john_doe@yourcompany.com"
#define MIBII_SYSDESCRIPTION    "Nucleus SNMP Agent"
#define MIBII_SYSLOCATION       "Mobile, AL"
#define MIBII_OBJECTID          "1.3.6.1.4.1.1103.11"
#define MIBII_SERVICES          79

/*--------------------End of SNMPv1/MIB2 Defines -----------------------*/

/*------------------- XRMON1/XRMON1-Lite Defines -----------------------*/

#define XRMON_MAXHOSTS          512     /* Maximum size of Host table   */
#define XRMON_MAXHOSTPAIRS      512     /* Maximum size of Matrix table */
#define XRMON_MAXNODES          512     /* Must match above two values  */
#define XRMON_MAXBUCKETS        10      /* Number of history buckets    */
#define XRMON_MAXLOGS           10      /* Maximum logs                 */
#define XRMON_MAXTOPNS          512     /* Maximum HostTopNs            */
#define XRMON_MAXPKTSIZE        2048    /* Buffer size for each capture */
#define XRMON_MAXRIPS           128     /* Maximum capture buffers      */

#define XRMON_CBUFSIZE          (XRMON_MAXRIPS * XRMON_MAXPKTSIZE)
/*------------------- End of XRMON1/XRMON1-Lite Defines ----------------*/

/*====================== End of User Defines ===========================*/

/*==================  Library Includes/Defines =========================*/

#define x_isdigit(c)  (c >= '0' && c <= '9')


/*
 * Globally used structures, defines, typedefs, ...
 */

#define     x_memcpy    memcpy
#define     x_memset    memset
#define     x_memcmp    memcmp

typedef struct xq_s
{
    struct xq_s        *xq_next;
    struct xq_s        *xq_prev;
} xq_t;

typedef struct tmr_s
{
    struct tmr_s    *forw;
    struct tmr_s    *back;
    VOID            (*func)(VOID *);
    VOID            *arg;
    NU_TIMER        *tmr_ptr;
    unsigned char   name[8];
} tmr_t;

#include "networking/timer.h"
#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* SNMP_PRT_H */
