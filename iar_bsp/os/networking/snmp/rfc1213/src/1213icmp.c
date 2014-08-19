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
*       1213icmp.c                                               
*
*       This file contains those functions specific to processing PDU
*       requests on parameters in the ICMP Group.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       icmpInMsgs
*       icmpInErrors
*       icmpInDestUnreachs
*       icmpInTimeExcds
*       icmpInParmProbs
*       icmpInSrcQuenchs
*       icmpInRedirects
*       icmpInEchos
*       icmpInEchoReps
*       icmpInTimestamps
*       icmpInTimestampReps
*       icmpInAddrMasks
*       icmpInAddrMaskReps
*       icmpOutMsgs
*       icmpOutErrors
*       icmpOutDestUnreachs
*       icmpOutTimeExcds
*       icmpOutParmProbs
*       icmpOutSrcQuenchs
*       icmpOutRedirects
*       icmpOutEchos
*       icmpOutEchoReps
*       icmpOutTimestamps
*       icmpOutTimestampReps
*       icmpOutAddrMasks
*       icmpOutAddrMaskReps
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       xtypes.h
*       snmp.h
*       snmp_sys.h
*       mib.h
*       1213icmp.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/snmp_sys.h"
#include "networking/mib.h"
#include "networking/1213icmp.h"

#if ((RFC1213_ICMP_INCLUDE == NU_TRUE) && (MIB2_ICMP_INCLUDE == NU_TRUE))

#if (INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*       icmpInMsgs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInMsgs
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
UINT16 icmpInMsgs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInMsgs_Get;

    return (status);

} /* icmpInMsgs */

/************************************************************************
*
*   FUNCTION
*
*       icmpInErrors
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInErrors
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
UINT16 icmpInErrors(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInErrors_Get;

    return (status);

} /* icmpInErrors */

/************************************************************************
*
*   FUNCTION
*
*       icmpInDestUnreachs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInDestUnreachs
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
UINT16 icmpInDestUnreachs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInDestUnreachs_Get;

    return (status);

} /* icmpInDestUnreachs */

/************************************************************************
*
*   FUNCTION
*
*       icmpInTimeExcds
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInTimeExcds
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
UINT16 icmpInTimeExcds(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInTimeExcds_Get;

    return (status);

} /* icmpInTimeExcds */

/************************************************************************
*
*   FUNCTION
*
*       icmpInParmProbs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInParmProbs
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
UINT16 icmpInParmProbs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInParmProbs_Get;

    return (status);

} /* icmpInParmProbs */

/************************************************************************
*
*   FUNCTION
*
*       icmpInSrcQuenchs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInSrcQuenchs
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
UINT16 icmpInSrcQuenchs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInSrcQuenchs_Get;

    return (status);

} /* icmpInSrcQuenchs */

/************************************************************************
*
*   FUNCTION
*
*       icmpInRedirects
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInRedirects
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
UINT16 icmpInRedirects(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInRedirects_Get;

    return (status);

} /* icmpInRedirects */

/************************************************************************
*
*   FUNCTION
*
*       icmpInEchos
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInEchos
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
UINT16 icmpInEchos(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInEchos_Get;

    return (status);

} /* icmpInEchos */

/************************************************************************
*
*   FUNCTION
*
*       icmpInEchoReps
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInEchoReps
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
UINT16 icmpInEchoReps(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInEchoReps_Get;

    return (status);

} /* icmpInEchoReps */

/************************************************************************
*
*   FUNCTION
*
*       icmpInTimestamps
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInTimestamps
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
UINT16 icmpInTimestamps(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInTimeStamps_Get;

    return (status);

} /* icmpInTimestamps */

/************************************************************************
*
*   FUNCTION
*
*       icmpInTimestampReps
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInTimestampReps
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
UINT16 icmpInTimestampReps(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInTimeStampReps_Get;

    return (status);

} /* icmpInTimestampReps */

/************************************************************************
*
*   FUNCTION
*
*       icmpInAddrMasks
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInAddrMasks
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
UINT16 icmpInAddrMasks(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInAddrMasks_Get;

    return (status);

} /* icmpInAddrMasks */

/************************************************************************
*
*   FUNCTION
*
*       icmpInAddrMaskReps
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpInAddrMaskReps
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
UINT16 icmpInAddrMaskReps(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpInAddrMaskReps_Get;

    return (status);

} /* icmpInAddrMaskReps */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutMsgs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutMsgs
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
UINT16 icmpOutMsgs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutMsgs_Get;

    return (status);

} /* icmpOutMsgs */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutErrors
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutErrors
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
UINT16 icmpOutErrors(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutErrors_Get;

    return (status);

} /* icmpOutErrors */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutDestUnreachs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutDestUnreachs
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
UINT16 icmpOutDestUnreachs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutDestUnreachs_Get;

    return (status);

} /* icmpOutDestUnreachs */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutTimeExcds
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutTimeExcds
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
UINT16 icmpOutTimeExcds(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutTimeExcds_Get;

    return (status);

} /* icmpOutTimeExcds */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutParmProbs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutParmProbs
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
UINT16 icmpOutParmProbs(snmp_object_t *obj, UINT16 idlen, VOID *param )
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutParmProbs_Get;

    return (status);

} /* icmpOutParmProbs */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutSrcQuenchs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutSrcQuenchs
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
UINT16 icmpOutSrcQuenchs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutSrcQuenchs_Get;

    return (status);

} /* icmpOutSrcQuenchs */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutRedirects
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutRedirects
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
UINT16 icmpOutRedirects(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutRedirects_Get;

    return (status);

} /* icmpOutRedirects */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutEchos
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutEchos
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
UINT16 icmpOutEchos(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutEchos_Get;

    return (status);

} /* icmpOutEchos */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutEchoReps
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutEchoReps
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
UINT16 icmpOutEchoReps(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutEchoReps_Get;

    return (status);

} /* icmpOutEchoReps */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutTimestamps
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutTimestamps
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
UINT16 icmpOutTimestamps(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutTimestamps_Get;

    return (status);

} /* icmpOutTimestamps */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutTimestampReps
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutTimestampReps
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
UINT16 icmpOutTimestampReps(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutTimestampReps_Get;

    return (status);

} /* icmpOutTimestampReps */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutAddrMasks
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutAddrMasks
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
UINT16 icmpOutAddrMasks(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutAddrMasks_Get;

    return (status);

} /* icmpOutAddrMasks */

/************************************************************************
*
*   FUNCTION
*
*       icmpOutAddrMaskReps
*
*   DESCRIPTION
*
*       This function processes the PDU action on the icmpOutAddrMaskReps
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
UINT16 icmpOutAddrMaskReps(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_icmpOutAddrMaskReps_Get;

    return (status);

} /* icmpOutAddrMaskReps */

#endif /* INCLUDE_SNMPv1_FULL_MIBII */
#endif /* ((RFC1213_ICMP_INCLUDE == NU_TRUE) && \
           (MIB2_ICMP_INCLUDE == NU_TRUE)) */
