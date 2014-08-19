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
*       agent.c                                                  
*
*   DESCRIPTION
*
*       This file contains all functions specific to maintaining the
*       agent's communities and hosts and the functions to process a
*       manager's request and send out traps.
*
*   DATA STRUCTURES
*
*       agentStat.
*
*   FUNCTIONS
*
*       AgentStatistics
*       AgentGetAuthenTraps
*       AgentSetAuthenTraps
*       AgentSetColdTraps
*       SendENTTrap
*
*   DEPENDENCIES
*
*       snmp.h
*       agent.h
*       snmp_no.h
*       1213sys.h
*
*************************************************************************/
#include "networking/snmp.h"
#include "networking/agent.h"
#include "networking/snmp_no.h"
#include "networking/1213sys.h"

agent_stat_t            agentStat;
extern  snmp_cfig_t     snmp_cfg;

/************************************************************************
*
*   FUNCTION
*
*       AgentStatistics
*
*   DESCRIPTION
*
*       This function returns a pointer to the global variable agentStat.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       *agentStat
*
*************************************************************************/
agent_stat_t *AgentStatistics(VOID)
{
    return (&agentStat);

} /* AgentStatistics */

/************************************************************************
*
*   FUNCTION
*
*       AgentGetAuthenTraps
*
*   DESCRIPTION
*
*       This function returns a value indicating whether or not
*       traps are enabled on the agent.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_TRUE                 Traps are enabled.
*       NU_FALSE                Traps are not enabled.
*
*************************************************************************/
BOOLEAN AgentGetAuthenTraps(VOID)
{
    BOOLEAN traps_enabled;
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    traps_enabled = (BOOLEAN)snmp_cfg.authentrap_enable;

    NU_USER_MODE();

    return (traps_enabled);

} /* AgentGetAuthenTraps */

/************************************************************************
*
*   FUNCTION
*
*       AgentSetAuthenTraps
*
*   DESCRIPTION
*
*       This function either enables or disables traps on the agent node.
*       If the user passes in NU_TRUE, traps will be enabled.  If the user
*       passes in NU_FALSE, traps will be disabled.
*
*   INPUTS
*
*       enable
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID AgentSetAuthenTraps(BOOLEAN enable)
{
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (enable == NU_TRUE)
        snmp_cfg.authentrap_enable = NU_TRUE;
    else
        snmp_cfg.authentrap_enable = NU_FALSE;

    NU_USER_MODE();

} /* AgentSetAuthenTraps */

/************************************************************************
*
*   FUNCTION
*
*       AgentSetColdTraps
*
*   DESCRIPTION
*
*       This function enables or disables cold traps on the agent node.
*       If the user passes in NU_TRUE, cold traps will be enabled.  If
*       the user passes in NU_FALSE, cold traps will be disabled.
*
*   INPUTS
*
*       enable
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID AgentSetColdTraps(BOOLEAN enable)
{
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (enable == NU_TRUE)
        snmp_cfg.coldtrap_enable = NU_TRUE;
    else
        snmp_cfg.coldtrap_enable = NU_FALSE;

    NU_USER_MODE();

} /* AgentSetColdTraps */

/************************************************************************
*
*   FUNCTION
*
*       SendENTTrap
*
*   DESCRIPTION
*
*       This function sends an enterprise specific trap
*
*   INPUTS
*
*       UINT16 gen
*       UINT16 spec
*       snmp_object_t *list
*       UINT16 listLen
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SendENTTrap(UINT32 gen, UINT32 spec, const snmp_object_t *list,
                 UINT16 listLen)
{
    SNMP_NOTIFY_REQ_STRUCT  *snmp_notification;
    UINT32                  linkup_trap[] = SNMP_LINK_UP_TRAP;
    UINT32                  linkdown_trap[] = SNMP_LINK_DOWN_TRAP;
    UINT32                  coldstart_trap[] = SNMP_COLD_START_TRAP;
    UINT32                  warmstart_trap[] = SNMP_WARM_START_TRAP;
    UINT32                  ent_trap[] = SNMP_ENTSPECIFIC_TRAP;
    STATUS                  status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    UNUSED_PARAMETER(spec);

    /* Verify that list length, is not greater than the permitted length.
     */
    if(listLen > SNMP_MAX_NOTIFICATION_OBJECTS)
    {
        return;
    }

    /* Get an empty location to place the notification. */
    if(SNMP_Get_Notification_Ptr(&snmp_notification) != NU_SUCCESS)
    {
        NU_USER_MODE();
        return;
    }

    switch (gen)
    {
    case SNMP_TRAP_LINKUP:

        NU_BLOCK_COPY(snmp_notification->OID.notification_oid,
                      linkup_trap, sizeof(UINT32) * SNMP_TRAP_OID_LEN);

        break;

    case SNMP_TRAP_LINKDOWN:

        NU_BLOCK_COPY(snmp_notification->OID.notification_oid,
                      linkdown_trap, sizeof(UINT32) * SNMP_TRAP_OID_LEN);

        break;

    case SNMP_TRAP_COLDSTART:

        NU_BLOCK_COPY(snmp_notification->OID.notification_oid,
                      coldstart_trap, sizeof(UINT32) * SNMP_TRAP_OID_LEN);

        break;

    case SNMP_TRAP_WARMSTART:

        NU_BLOCK_COPY(snmp_notification->OID.notification_oid,
                      warmstart_trap, sizeof(UINT32) * SNMP_TRAP_OID_LEN);

        break;

    case SNMP_TRAP_ENTSPECIFIC:

        NU_BLOCK_COPY(snmp_notification->OID.notification_oid, ent_trap,
                      sizeof(UINT32) * SNMP_TRAP_OID_LEN);

        break;

    default:

        status = SNMP_ERROR;
        break;

    }

    if (status == NU_SUCCESS)
    {
        snmp_notification->OID.oid_len = SNMP_TRAP_OID_LEN;

        NU_BLOCK_COPY(snmp_notification->snmp_object_list, list,
                      listLen * sizeof(snmp_object_t));

        snmp_notification->snmp_object_list_len = listLen;

        /* The notification is ready to be sent. */
        if (SNMP_Notification_Ready(snmp_notification) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to send notification/trap",
                            NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    NU_USER_MODE();

} /* SendENTTrap */


