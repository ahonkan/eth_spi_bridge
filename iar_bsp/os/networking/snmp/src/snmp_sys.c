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
*       snmp_sys.c                                               
*
*   DESCRIPTION
*
*       This file contains all functions to maintain the global
*       xsmnp_cfg data structure and the main function to process
*       incoming requests.
*
*   DATA STRUCTURES
*
*       mib_element_t
*
*   FUNCTIONS
*
*       snmp_init
*       SNMP_Mib_Init
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       xtypes.h
*       snmp.h
*       sys.h
*       agent.h
*       snmp_g.h
*       mib.h
*		snmp_comp.h
*       usm_mib.h
*       vacm_mib.h
*       engine_mib.h
*       cbsm_mib.h
*       mpd_mib.h
*       no_mib.h
*		1213xxxx.h
*       1213if.h
*       1213ip.h
*       1213sys.h
*       1213tran.h
*       1213tcp.h
*       1213udp.h
*       1213icmp.h
*       1213egp.h
*       1213snmp.h
*       ring.h
*       hash.h
*       bool.h
*       1757xxxx.h
*       1757log.h
*       1757evnt.h
*       1757stat.h
*       1757alrm.h
*       1757chan.h
*       1757filt.h
*       1757cap.h
*       1757host.h
*       1757matx.h
*       1757topn.h
*       1757hist.h
*       ent_mib.h
*       1213oid.h
*       usm_oid.h
*       vacm_oid.h
*       engine_oid.h
*		cbsm_oid.h
*       mpd_oid.h
*       tgr_oid.h
*       no_oid.h
*       1757oid.h
*       1471oid.h
*       1472oid.h
*       1473oid.h
*       ent_oid.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"

#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/sys.h"
#include "networking/agent.h"
#include "networking/snmp_g.h"
#include "networking/mib.h"
#include "networking/snmp_comp.h"

/*header files will be included only if their macro's are set true*/
#if( INCLUDE_SNMPv3 )
#if (INCLUDE_MIB_USM == NU_TRUE)
#include "networking/usm_mib.h"
#endif
#endif

#if (INCLUDE_MIB_VACM == NU_TRUE)
#include "networking/vacm_mib.h"
#endif

#if (INCLUDE_MIB_SNMP_ENGINE == NU_TRUE)
#include "networking/engine_mib.h"
#endif

#if( INCLUDE_SNMPv1 || INCLUDE_SNMPv2 )
#if (INCLUDE_MIB_CBSM == NU_TRUE)
#include "networking/cbsm_mib.h"
#endif
#endif

#if (INCLUDE_MIB_MPD == NU_TRUE)
#include "networking/mpd_mib.h"
#endif

#if (INCLUDE_MIB_NO == NU_TRUE)
#include "networking/no_mib.h"
#endif

#include "networking/1213xxxx.h"
#include "networking/1213if.h"
#include "networking/1213ip.h"
#include "networking/1213sys.h"
#include "networking/1213tran.h"
#include "networking/1213tcp.h"
#include "networking/1213udp.h"
#include "networking/1213icmp.h"
#include "networking/1213egp.h"
#include "networking/1213snmp.h"

#if (INCLUDE_MIB_RMON1==NU_TRUE)
#include "networking/ring.h"
#include "networking/hash.h"
#include "networking/bool.h"
#include "rmon/inc/1757xxxx.h"
#include "rmon/inc/1757log.h"
#include "rmon/inc/1757evnt.h"
#include "rmon/inc/1757stat.h"
#include "rmon/inc/1757alrm.h"

#if (RFC1757_RMON_LITE == NU_FALSE)
#include "rmon/inc/1757chan.h"
#include "rmon/inc/1757filt.h"
#include "rmon/inc/1757cap.h"
#include "rmon/inc/1757host.h"
#include "rmon/inc/1757matx.h"
#include "rmon/inc/1757topn.h"
#endif

#include "rmon/inc/1757hist.h"
#endif


/* This file contains include for the enterprise specific MIBs. */
#include "networking/ent_mib.h"

/*
 * The MIB (s)
 */
STATIC mib_element_t SnmpObj[] =
{
#include "networking/1213oid.h"       /* RFC 1213, MIB-II */
#if( INCLUDE_SNMPv3 )
/*user based security model (RFC 2574)*/
#if (INCLUDE_MIB_USM == NU_TRUE)
#include "networking/usm_oid.h"
#endif
#endif

/*view based access control model (RFC 2575)*/
#if (INCLUDE_MIB_VACM == NU_TRUE)
#include "networking/vacm_oid.h"
#endif

/*MIB for SNMP engine from SNMPv3 framework(RFC 2571)*/
#if (INCLUDE_MIB_SNMP_ENGINE == NU_TRUE)
#include "networking/engine_oid.h"
#endif

#if( INCLUDE_SNMPv1 || INCLUDE_SNMPv2)
/*MIB for community based security model (RFC 2576)*/
#if (INCLUDE_MIB_CBSM == NU_TRUE)
#include "networking/cbsm_oid.h"
#endif
#endif

/*MIB for message processing and dispatching (RFC 2572)*/
#if (INCLUDE_MIB_MPD == NU_TRUE)
#include "networking/mpd_oid.h"
#endif

/*MIB for target address table and params table (RFC 2573)*/
#if (INCLUDE_MIB_TARGET == NU_TRUE)
#include "networking/tgr_oid.h"
#endif

/*MIB for notification originator (RFC 2573)*/
#if (INCLUDE_MIB_NO == NU_TRUE)
#include "networking/no_oid.h"
#endif

/* RFC 1757, RMON version 1 */
#if (INCLUDE_MIB_RMON1==NU_TRUE)
#include "rmon/inc/1757oid.h"
#endif

/* PPP MIB - RFC 1471 */
#if (INCLUDE_LCP_MIB == NU_TRUE)
#include "networking/1471oid.h"
#endif

/* PPP MIB - RFC 1472 */
#if (INCLUDE_SEC_MIB == NU_TRUE)
#include "networking/1472oid.h"
#endif

/* PPP MIB - RFC 1473 */
#if (INCLUDE_NCP_MIB == NU_TRUE)
#include "networking/1473oid.h"
#endif

#include "networking/ent_oid.h"               /* Enterprise Specific MIBS */

};

#if (INCLUDE_MIB_RMON1==NU_TRUE)
extern BOOL Rmon1Init(VOID);
#endif

snmp_cfig_t                         snmp_cfg;

extern UINT32                       get_authentrap(VOID);
extern UINT32                       get_coldstarttrap(VOID);
extern UINT32                       get_snmpport(VOID);

#if (INCLUDE_MIB_RMON1==NU_TRUE)
extern UINT32                       get_maxbuff(VOID);
extern UINT32                       get_maxHTS(VOID);
extern UINT32                       get_maxMTS(VOID);
extern UINT32                       get_maxbuckets(VOID);
extern UINT32                       get_maxlogs(VOID);
#endif

/************************************************************************
*
*   FUNCTION
*
*       snmp_init
*
*   DESCRIPTION
*
*       This function initializes all of the elements of the global
*       variable xsmnp_cfg.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_BAD_PARAMETER
*
*************************************************************************/
STATUS snmp_init (VOID)
{
    STATUS  status = NU_SUCCESS;
    INT     len;

    get_system_contact((UINT8*)&snmp_cfg.sys_contact[0]);
    get_system_description((UINT8*)&snmp_cfg.sys_description[0]);
    get_system_location((UINT8*)&snmp_cfg.sys_location[0]);

    if (NU_Get_Host_Name((CHAR *)(snmp_cfg.sys_name), SNMP_MAX_STR)
                                                            == NU_SUCCESS)
    {
        len = strlen((CHAR *)(snmp_cfg.sys_name));

        if (len < (SNMP_MAX_STR - 2))
        {
            if (len)
            {
                snmp_cfg.sys_name[len] = '.';
                len++;
            }

            if (NU_Get_Domain_Name((CHAR *)(&(snmp_cfg.sys_name[len])),
                                 (SNMP_MAX_STR - len)) != NU_SUCCESS)
            {
                status = NU_SUCCESS;
            }
        }
        else
        {
            status = SNMP_BAD_PARAMETER;
        }
    }
    else
    {
        status = SNMP_BAD_PARAMETER;
    }

    get_system_objectid(&snmp_cfg.sys_objectid[0]);

    snmp_cfg.sys_services       = (UINT32)get_system_services();
    snmp_cfg.authentrap_enable  = get_authentrap();
    snmp_cfg.coldtrap_enable    = get_coldstarttrap();
    snmp_cfg.local_port         = get_snmpport();

#if (INCLUDE_MIB_RMON1==NU_TRUE)
    snmp_cfg.cbuff_size                 = get_maxbuff();
    snmp_cfg.host_maxnrhosts            = get_maxHTS();
    snmp_cfg.matrix_maxnrsrcdsts        = get_maxMTS();
    snmp_cfg.hist_maxnrbuckets          = get_maxbuckets();
    snmp_cfg.event_maxnrlogs            = get_maxlogs();
    snmp_cfg.disc_maxnrnodes            = 10000L;
    snmp_cfg.disc_nodetimeout           = 10000L;
#endif

    return status;

} /* snmp_init */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Mib_Init
*
*   DESCRIPTION
*
*       This function Initializes the Management Information Base.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS              MIB registeration successfull.
*       SNMP_NO_MEMORY          Memory allocation failed
*       SNMP_BAD_PARAMETER      Atleast one of the MIB objects is 
*                               already registered
*       NU_INVALID_SEMAPHORE    Invalid semaphore pointer
*       NU_INVALID_SUSPEND      Suspension requested from non-task
*
*************************************************************************/
STATUS SNMP_Mib_Init (VOID)
{
    STATUS status; 

	status = MibInit(SnmpObj, sizeof(SnmpObj)/sizeof(SnmpObj[0]));

    return status;

} /* SNMP_Mib_Init */

