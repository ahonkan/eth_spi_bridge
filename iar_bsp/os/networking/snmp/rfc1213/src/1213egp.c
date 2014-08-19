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
*       1213egp.c                                                
*
* DESCRIPTION
*
*        This file contains those functions specific to processing PDU
*        requests on parameters in the Exterior Gateway Protocol Group.
*
* DATA STRUCTURES
*
*        None
*
* FUNCTIONS
*
*        egpInMsgs
*        egpInErrors
*        egpOutMsgs
*        egpOutErrors
*        egpNeighEntry
*        egpAs
*        egp_Get_Bulk_1213EgpTab
*
* DEPENDENCIES
*
*        nu_net.h
*        snmp_api.h
*        mib.h
*        1213xxxx.h
*        1213egp.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/snmp_api.h"
#include "networking/mib.h"
#include "networking/1213xxxx.h"
#include "networking/1213egp.h"

#if (RFC1213_EGP_INCLUDE == NU_TRUE)

#if (INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE)

extern  rfc1213_vars_t  rfc1213_vars;

UINT16  Get1213EgpTab(snmp_object_t *obj, UINT16 idlen, UINT16 sublen,
                      UINT32 node[], UINT8 getflag);
UINT16  egp_Get_Bulk_1213EgpTab(snmp_object_t *obj, UINT16 idlen,
                                UINT8 getflag, UINT16 sublen,
                                UINT32 *node);

/************************************************************************
*
* FUNCTION
*
*       egpInMsgs
*
* DESCRIPTION
*
*       This function processes the PDU action on the egpInMsgs
*       parameter of MIB II.
*
* INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
* OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 egpInMsgs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;

    else
        obj->Syntax.LngUns = rfc1213_vars.rfc1213_egp.egpInMsgs;

    return (status);

} /* egpInMsgs */

/************************************************************************
*
* FUNCTION
*
*       egpInErrors
*
* DESCRIPTION
*
*       This function processes the PDU action on the egpInErrors
*       parameter of MIB II.
*
* INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
* OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 egpInErrors(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = rfc1213_vars.rfc1213_egp.egpInErrors;

    return (status);

} /* egpInErrors */

/************************************************************************
*
* FUNCTION
*
*       egpOutMsgs
*
* DESCRIPTION
*
*       This function processes the PDU action on the egpOutMsgs
*       parameter of MIB II.
*
* INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
* OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 egpOutMsgs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = rfc1213_vars.rfc1213_egp.egpOutMsgs;

    return (status);

} /* egpOutMsgs */

/************************************************************************
*
* FUNCTION
*
*       egpOutErrors
*
* DESCRIPTION
*
*       This function processes the PDU action on the egpOutErrors
*       parameter of MIB II.
*
* INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
* OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 egpOutErrors(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = rfc1213_vars.rfc1213_egp.egpOutErrors;

    return (status);

} /* egpOutErrors */

/*************************************************************************
* FUNCTION
*
*        egpNeighEntry
*
* DESCRIPTION
*
*        This function is used to handle PDU request of egpNeigh Table
*        entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  length of SNMP request.
*        *param                 Additional parameters (Not used).
*
* OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeded.
*        SNMP_READONLY          If object is read-only.
*        SNMP_ERROR             If invalid value.
*        SNMP_NOSUCHNAME        Object does not exist.
*        SNMP_INCONSISTANTVALUE If commit fails.
*        SNMP_GENERROR          Invalid request.
*
*************************************************************************/
UINT16 egpNeighEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT8           getflag = 0;
    UINT16          sublen = 4;
    UINT16          status = SNMP_NOERROR;

    UINT32          node[SNMP_SIZE_OBJECTID] =
                            {1, 3, 6, 1, 2, 1, 8, 5, 1, 0, 0, 0, 0, 0, 0};

    UNUSED_PARAMETER(param);

    node[9] = obj->Id[9];

    switch (obj->Request)
    {
    case SNMP_PDU_GET:

        /* Set the getflag */
        getflag++;

    case SNMP_PDU_NEXT:

        /* Get the object or the next object */
        if (!Get1213EgpTab(obj, idlen, sublen, node, getflag))
            status = SNMP_NOSUCHNAME;
        break;

#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )

    case SNMP_PDU_BULK:

        status = egp_Get_Bulk_1213EgpTab(obj, idlen, getflag, sublen,
                                         node);
        break;

#endif

    case SNMP_PDU_SET:
    case SNMP_PDU_COMMIT:
    case SNMP_PDU_UNDO:

        status = SNMP_NOSUCHNAME;
        break;

    default:

        status = SNMP_GENERROR;
    }

    /* Return status. */
    return (status);

} /* egpNeighEntry */

/************************************************************************
*
* FUNCTION
*
*       egpAs
*
* DESCRIPTION
*
*       This function processes the PDU action on the egpAs
*       parameter of MIB II.
*
* INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
* OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 egpAs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status for returning success or error code. */
    UINT16          status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = rfc1213_vars.rfc1213_egp.egpAs;

    /* Returning success or error code. */
    return (status);

} /* egpAs */

#if ((INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE))

/************************************************************************
*
* FUNCTION
*
*       egp_Get_Bulk_1213EgpTab
*
* DESCRIPTION
*
*       This function processes the get bulk PDU action.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The action was successfully completed.
*
*************************************************************************/
UINT16 egp_Get_Bulk_1213EgpTab(snmp_object_t *obj, UINT16 idlen,
                               UINT8 getflag, UINT16 sublen, UINT32 *node)
{
    UINT32          maxRepetitions;
    UINT32          index;
    snmp_object_t   dummyObject;
    UINT16          status = SNMP_NOERROR;

    /* Picking the no of repetitions for this particular object. */
    maxRepetitions = obj[0].Syntax.LngUns;

    /* Picking the object */
    dummyObject= obj[0];

    /* running the loop for max number of repetitions */
    for (index = 0; index < maxRepetitions; index++)
    {
        /* Get the object or the next object */
        if (!Get1213EgpTab((&dummyObject), idlen, sublen, node,
                           getflag))
        {
            if (index == 0)
                status = SNMP_NOSUCHNAME;
            break;
        }

        obj[index] = dummyObject;
    }

    /* Returning success or error code. */
    return (status);
} /* egp_Get_Bulk_1213EgpTab */

#endif /* ((INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE)) */
#endif /* INCLUDE_SNMPv1_FULL_MIBII */
#endif /* RFC1213_EGP_INCLUDE */


