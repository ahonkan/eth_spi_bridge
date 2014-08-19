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
*       agent.h                                                  
*
*   DESCRIPTION
*
*       This file contains functions, macros and data structures
*       specific to the agent side.
*
*   DATA STRUCTURES
*
*       agent_stat_t
*
*   DEPENDENCIES
*
*       target.h
*       snmp_cfg.h
*       snmp_prt.h
*       xtypes.h
*       snmp.h
*       prot.h
*       snmp_sys.h
*       asn1.h
*
*************************************************************************/

#ifndef AGENT_H
#define AGENT_H

#include "networking/target.h"
#include "networking/snmp_cfg.h"
#include "networking/snmp_prt.h"
#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/prot.h"
#include "networking/snmp_sys.h"
#include "networking/asn1.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#define AGENT_BUFFER_SIZE       1472    /* 1500 - 8 (UDP) - 20 (IP) */
#define AGENT_COMMUNITY_SIZE    32      /*255*/
#define AGENT_ADDR_SIZE         16

typedef struct agent_stat_s     agent_stat_t;

struct agent_stat_s
{
    UINT32   InTotalReqVars;
    UINT32   InTotalSetVars;
    UINT32   InBadCommunityNames;
    UINT32   InBadCommunityUses;
    UINT32   datagrams;
    UINT32   errors;
};

agent_stat_t *AgentStatistics(VOID);
BOOLEAN         AgentGetAuthenTraps(VOID);
VOID         AgentSetAuthenTraps(BOOLEAN);
VOID         AgentSetColdTraps (BOOLEAN);
VOID         SendENTTrap(UINT32, UINT32, const snmp_object_t *, UINT16);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


