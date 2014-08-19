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
*       1213snmp.c                                               
*
*   DESCRIPTION
*
*       This file contains those functions specific to processing PDU
*       requests on parameters in the SNMP Group.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       snmpInPkts
*       snmpOutPkts
*       snmpInBadVersions
*       snmpInBadCommunityNames
*       snmpInBadCommunityUses
*       snmpInASNParseErrs
*       snmpInTooBigs
*       snmpInNoSuchNames
*       snmpInBadValues
*       snmpInReadOnlys
*       snmpInGenErrs
*       snmpInTotalReqVars
*       snmpInTotalSetVars
*       snmpInGetRequests
*       snmpInGetNexts
*       snmpInSetRequests
*       snmpInGetResponses
*       snmpInTraps
*       snmpOutTooBigs
*       snmpOutNoSuchNames
*       snmpOutBadValues
*       snmpOutGenErrs
*       snmpOutGetRequests
*       snmpOutGetNexts
*       snmpOutSetRequests
*       snmpOutGetResponses
*       snmpOutTraps
*       snmpEnableAuthenTraps
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       xtypes.h
*       snmp.h
*       agent.h
*       mib.h
*       snmp_g.h
*       1213snmp.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/agent.h"
#include "networking/mib.h"
#include "networking/1213snmp.h"

#if (RFC1213_SNMP_INCLUDE == NU_TRUE)

#if (INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE)

extern snmp_stat_t      SnmpStat;

/************************************************************************
*
*   FUNCTION
*
*       snmpInPkts
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInPkts
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInPkts(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns  = SnmpStat.InPkts;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInPkts */

/************************************************************************
*
*   FUNCTION
*
*       snmpOutPkts
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpOutPkts
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpOutPkts(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.OutPkts;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpOutPkts */

/************************************************************************
*
*   FUNCTION
*
*       snmpInBadVersions
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInBadVersions
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInBadVersions(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.InBadVersions;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInBadVersions */

/************************************************************************
*
*   FUNCTION
*
*       snmpInBadCommunityNames
*
*   DESCRIPTION
*
*       This function processes the PDU action on the
*       snmpInBadCommunityNames parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInBadCommunityNames(snmp_object_t *obj, UINT16 idlen,
                               VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = (AgentStatistics())->InBadCommunityNames;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInBadCommunityNames */

/************************************************************************
*
*   FUNCTION
*
*       snmpInBadCommunityUses
*
*   DESCRIPTION
*
*       This function processes the PDU action on the
*       snmpInBadCommunityUses parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInBadCommunityUses(snmp_object_t *obj, UINT16 idlen,
                              VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = (AgentStatistics())->InBadCommunityUses;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInBadCommunityUses */

/************************************************************************
*
*   FUNCTION
*
*       snmpInASNParseErrs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInASNParseErrs
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInASNParseErrs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.InASNParseErrs;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInASNParseErrs */

/************************************************************************
*
*   FUNCTION
*
*       snmpInTooBigs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInTooBigs
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInTooBigs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.InTooBigs;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInTooBigs */

/************************************************************************
*
*   FUNCTION
*
*       snmpInNoSuchNames
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInNoSuchNames
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*       SMNP_GENERROR      The PDU request was not processed.
*
*************************************************************************/
UINT16 snmpInNoSuchNames(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.InNoSuchNames;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInNoSuchNames */

/************************************************************************
*
*   FUNCTION
*
*       snmpInBadValues
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInBadValues
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInBadValues(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.InBadValues;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInBadValues */

/************************************************************************
*
*   FUNCTION
*
*       snmpInReadOnlys
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInReadOnlys
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInReadOnlys(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.InReadOnlys;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInReadOnlys */

/************************************************************************
*
*   FUNCTION
*
*       snmpInGenErrs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInGenErrs
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInGenErrs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.InGenErrs;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInGenErrs */

/************************************************************************
*
*   FUNCTION
*
*       snmpInTotalReqVars
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInTotalReqVars
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInTotalReqVars(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = (AgentStatistics())->InTotalReqVars;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInTotalReqVars */

/************************************************************************
*
*   FUNCTION
*
*       snmpInTotalSetVars
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInTotalSetVars
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInTotalSetVars(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = (AgentStatistics())->InTotalSetVars;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInTotalSetVars */

/************************************************************************
*
*   FUNCTION
*
*       snmpInGetRequests
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInGetRequests
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInGetRequests(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.InGetRequests;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInGetRequests */

/************************************************************************
*
*   FUNCTION
*
*       snmpInGetNexts
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInGetNexts
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInGetNexts(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.InGetNexts;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInGetNexts */

/************************************************************************
*
*   FUNCTION
*
*       snmpInSetRequests
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInSetRequests
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInSetRequests(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.InSetRequests;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInSetRequests */

/************************************************************************
*
*   FUNCTION
*
*       snmpInGetResponses
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInGetResponses
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInGetResponses(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.InGetResponses;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInGetResponses */

/************************************************************************
*
*   FUNCTION
*
*       snmpInTraps
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpInTraps
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpInTraps(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.InTraps;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpInTraps */

/************************************************************************
*
*   FUNCTION
*
*       snmpOutTooBigs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpOutTooBigs
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param                 Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpOutTooBigs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.OutTooBigs;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpOutTooBigs */

/************************************************************************
*
*   FUNCTION
*
*       snmpOutNoSuchNames
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpOutNoSuchNames
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpOutNoSuchNames(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.OutNoSuchNames;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpOutNoSuchNames */

/************************************************************************
*
*   FUNCTION
*
*       snmpOutBadValues
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpOutBadValues
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpOutBadValues(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.OutBadValues;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpOutBadValues */

/************************************************************************
*
*   FUNCTION
*
*       snmpOutGenErrs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpOutGenErrs
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpOutGenErrs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.OutGenErrs;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpOutGenErrs */

/************************************************************************
*
*   FUNCTION
*
*       snmpOutGetRequests
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpOutGetRequests
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpOutGetRequests(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.OutGetRequests;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpOutGetRequests */

/************************************************************************
*
*   FUNCTION
*
*       snmpOutGetNexts
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpOutGetNexts
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpOutGetNexts(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.OutGetNexts;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpOutGetNexts */

/************************************************************************
*
*   FUNCTION
*
*       snmpOutSetRequests
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpOutSetRequests
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpOutSetRequests(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.OutSetRequests;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpOutSetRequests */

/************************************************************************
*
*   FUNCTION
*
*       snmpOutGetResponses
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpOutGetResponses
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpOutGetResponses(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.OutGetResponses;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpOutGetResponses */

/************************************************************************
*
*   FUNCTION
*
*       snmpOutTraps
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpOutTraps
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpOutTraps(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
        obj->Syntax.LngUns = SnmpStat.OutTraps;
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpOutTraps */

/************************************************************************
*
*   FUNCTION
*
*       snmpEnableAuthenTraps
*
*   DESCRIPTION
*
*       This function processes the PDU action on the snmpEnableTraps
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 snmpEnableAuthenTraps(snmp_object_t *obj, UINT16 idlen,
                             VOID *param)
{
	BOOLEAN enabled;
    UINT16  status = SNMP_NOERROR;
    UINT32  commit_temp;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
    {
        switch (obj->Request)
        {
        case SNMP_PDU_NEXT:
#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )
        case SNMP_PDU_BULK:
#endif
        case SNMP_PDU_GET:

            enabled = AgentGetAuthenTraps();
            obj->Syntax.LngUns = (UINT32)(enabled == NU_TRUE ? 1 : 2);
            break;

        case SNMP_PDU_COMMIT:

            commit_temp = obj->Syntax.LngUns;
            enabled = AgentGetAuthenTraps();
            obj->Syntax.LngUns = (UINT32)(enabled == NU_TRUE ? 1 : 2);

            if (commit_temp != obj->Syntax.LngUns)
            {
                status = SNMP_COMMITFAILED;
            }
            break;

        case SNMP_PDU_UNDO:
        case SNMP_PDU_SET:

            if ( (obj->Syntax.LngUns != 1) && (obj->Syntax.LngUns != 2) )
                status = SNMP_WRONGVALUE;
            else
                AgentSetAuthenTraps((obj->Syntax.LngUns == 1) ?
                                                    NU_TRUE : NU_FALSE);

            break;

        default:

            status = SNMP_GENERROR;
        }
    }
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* snmpEnableAuthenTraps */

#endif /* RFC1213_SNMP_INCLUDE */
#endif /* INCLUDE_SNMPv1_FULL_MIBII */



