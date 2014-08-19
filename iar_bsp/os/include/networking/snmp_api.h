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
*       snmp_api.h                                               
*
*   DESCRIPTION
*
*       This file contains the includes for all API function in
*       SNMPv2.2
*
*   DATA STRUCTURES
*
*   DEPENDENCIES
*
*        externs.h
*        target.h
*        xtypes.h
*        snmp_cfg.h
*        snmp.h
*        timer.h
*        snmp_prt.h
*        1213xxxx.h
*        rip.h
*        prot.h
*        snmp_sys.h
*        asn1.h
*        agent.h
*        link.h
*        mac.h
*        snmp_dis.h
*        snmp_mp.h
*        snmp_ss.h
*        cbsm.h
*        vacm.h
*        tgr_mib.h
*        mib.h
*        snmp_v3.h
*        snmp_no.h
*        no_api.h
*        snmp_g.h
*        usm.h
*        tgr_api.h
*
************************************************************************/

#ifndef SNMP_API_H
#define SNMP_API_H

#include "networking/externs.h"
#include "networking/target.h"
#include "networking/xtypes.h"
#include "networking/snmp_cfg.h"
#include "networking/snmp.h"
#include "networking/timer.h"
#include "networking/snmp_prt.h"
#include "networking/1213xxxx.h"
#include "networking/rip.h"
#include "networking/prot.h"
#include "networking/snmp_sys.h"
#include "networking/asn1.h"
#include "networking/agent.h"
#include "networking/link.h"
#include "networking/mac.h"
#include "networking/snmp_dis.h"
#include "networking/snmp_mp.h"
#include "networking/snmp_ss.h"
#include "networking/cbsm.h"
#include "networking/vacm.h"
#include "networking/tgr_mib.h"
#include "networking/mib.h"

#if (INCLUDE_SNMPv3 == NU_TRUE)
#include "networking/snmp_v3.h"
#endif

#include "networking/snmp_no.h"
#include "networking/no_api.h"
#include "networking/snmp_g.h"

#if (INCLUDE_SNMPv3 == NU_TRUE)
#include "networking/usm.h"
#endif

#include "networking/tgr_api.h"

#endif  /* SNMP_H */


