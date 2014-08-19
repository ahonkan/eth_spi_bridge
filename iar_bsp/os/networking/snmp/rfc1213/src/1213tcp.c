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
*       1213tcp.c                                                
*
* DESCRIPTION
*
*       This file contains those functions specific to processing PDU
*       requests on parameters in the TCP Group.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       tcpRtoAlgorithm
*       tcpRtoMin
*       tcpRtoMax
*       tcpMaxConn
*       tcpActiveOpens
*       tcpPassiveOpens
*       tcpAttemptFails
*       tcpEstabResets
*       tcpCurrEstab
*       tcpInSegs
*       tcpOutSegs
*       tcpRetransSegs
*       tcpInErrs
*       tcpOutRsts
*       tcpConnEntry
*       tcp_Get_Bulk_1213TcpTab
*
* DEPENDENCIES
*
*       nu_net.h
*       snmp_api.h
*       mib.h
*       1213tcp.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/snmp_api.h"
#include "networking/mib.h"
#include "networking/1213tcp.h"

#if (INCLUDE_TCP == NU_TRUE)

#if ((RFC1213_TCP_INCLUDE == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE))

#if (INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE)

UINT16  Get1213TcpTab(snmp_object_t *obj, UINT16 idlen, UINT16 sublen,
                      UINT32 node[], UINT8 getflag);
UINT16  Set1213TcpTab(snmp_object_t *obj, UINT16 idlen, UINT16 sublen,
                      UINT32 node[]);
UINT16  tcp_Get_Bulk_1213TcpTab(snmp_object_t *obj, UINT16 idlen,
                                UINT8 getflag, UINT16 sublen,
                                UINT32 *node);

/************************************************************************
*
* FUNCTION
*
*       tcpRtoAlgorithm
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpRtoAlgorithm
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
UINT16 tcpRtoAlgorithm(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngInt = (INT32)MIB2_tcpRtoAlgorithm_Get;

    return (status);

} /* tcpRtoAlgorithm */

/************************************************************************
*
* FUNCTION
*
*       tcpRtoMin
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpRtoMin
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
UINT16 tcpRtoMin(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngInt = (INT32)MIB2_tcpRtoMin_Get;

    return (status);

} /* tcpRtoMin */

/************************************************************************
*
* FUNCTION
*
*       tcpRtoMax
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpRtoMax
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
UINT16 tcpRtoMax(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngInt = (INT32)MIB2_tcpRtoMax_Get;

    return (status);

} /* tcpRtoMax */

/************************************************************************
*
* FUNCTION
*
*       tcpMaxConn
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpMaxConn
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
UINT16 tcpMaxConn(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngInt = MIB2_tcpMaxConn_Get;

    return (status);

} /* tcpMaxConn */

/************************************************************************
*
* FUNCTION
*
*       tcpActiveOpens
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpActiveOpens
*       parameter of MIB II.
*
* INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param                 Unused parameter.
*
* OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 tcpActiveOpens(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_tcpActiveOpens_Get;

    return (status);

} /* tcpActiveOpens */

/************************************************************************
*
* FUNCTION
*
*       tcpPassiveOpens
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpPassiveOpens
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
UINT16 tcpPassiveOpens(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_tcpPassiveOpens_Get;

    return (status);

} /* tcpPassiveOpens */

/************************************************************************
*
* FUNCTION
*
*       tcpAttemptFails
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpAttemptFails
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
UINT16 tcpAttemptFails(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_tcpAttemptFails_Get;

    return (status);

} /* tcpAttemptFails */

/************************************************************************
*
* FUNCTION
*
*       tcpEstabResets
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpEstabResets
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
UINT16 tcpEstabResets(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_tcpEstabResets_Get;

    return (status);

} /* tcpEstabResets */

/************************************************************************
*
* FUNCTION
*
*       tcpCurrEstab
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpCurrEstab
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
UINT16 tcpCurrEstab(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_tcpCurrEstab_Get;

    return (status);

} /* tcpCurrEstab */

/************************************************************************
*
* FUNCTION
*
*       tcpInSegs
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpInSegs
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
UINT16 tcpInSegs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_tcpInSegs_Get;

    return (status);

} /* tcpInSegs */

/************************************************************************
*
* FUNCTION
*
*       tcpOutSegs
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpOutSegs
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
UINT16 tcpOutSegs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_tcpOutSegs_Get;

    return (status);

} /* tcpOutSegs */

/************************************************************************
*
* FUNCTION
*
*       tcpRetransSegs
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpRetransSegs
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
UINT16 tcpRetransSegs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_tcpRetransSegs_Get;

    return (status);

} /* tcpRetransSegs */

/************************************************************************
*
* FUNCTION
*
*       tcpInErrs
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpInErrs
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
UINT16 tcpInErrs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_tcpInErrs_Get;

    return (status);

} /* tcpInErrs */

/************************************************************************
*
* FUNCTION
*
*       tcpOutRsts
*
* DESCRIPTION
*
*       This function processes the PDU action on the tcpOutRsts
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
UINT16 tcpOutRsts(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_tcpOutRsts_Get;

    return (status);

} /* tcpOutRsts */

/*************************************************************************
* FUNCTION
*
*        tcpConnEntry
*
* DESCRIPTION
*
*        This function is used to handle PDU request of tcpConn Table
*        entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  Length of SNMP request.
*        *param                 Additional parameters (Not used).
*
* OUTPUTS
*
*       SNMP_NOSUCHNAME         The object does not exist.
*       SNMP_COMMITFAILED       If Commit Fails.
*       SNMP_READONLY           If Set operation is called on read-only
*                               object.
*       SNMP_GENERROR           If an general error occur while processing
*                               the request.
*       SNMP_NOERROR            The action was successfully completed.
*
*************************************************************************/
UINT16 tcpConnEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Flag to distinguish between get and get-next requests. */
    UINT8           getflag = 0;

    /* Sub-length of object identifier. */
    UINT16          sublen = 10;

    /* Status for returning success or error code. */
    UINT16          status = SNMP_NOERROR;

    UINT32          node[SNMP_SIZE_OBJECTID] = {1, 3, 6, 1, 2, 1, 6, 13,
                                                1, 0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0};

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);

	/* idlen should not exceed the size of node array. */
	if(idlen > SNMP_SIZE_OBJECTID)
	{
		status = SNMP_GENERROR;
	}
	
	else
	{

	    node[9] = obj->Id[9];

    	switch (obj->Request)
    	{
    	case SNMP_PDU_GET:

        	/* Set the getflag */
        	getflag++;

        	/* Fall through the next case */

    	case SNMP_PDU_NEXT:

       		/* Get the object or the next object */
			if (!Get1213TcpTab(obj, idlen, sublen, node, getflag))
           		status = SNMP_NOSUCHNAME;
        	break;

#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )

    	case SNMP_PDU_BULK:

        	status = tcp_Get_Bulk_1213TcpTab(obj, idlen, getflag, sublen,
                                         node);

        	break;
#endif

    	case SNMP_PDU_COMMIT:

        	/* We always have the successful set. */
        	break;

    	case SNMP_PDU_UNDO:
    	case SNMP_PDU_SET:

        	/* If object is tcpConnState then set value because it is the
           	 * only object with write access in this table.
			 */
        	if(obj->Id[9] == 1)
        	{
            	/* Set the object. */
            	status = Set1213TcpTab(obj, idlen, sublen, node);
        	}
        	else
        	{
            	status = SNMP_READONLY;
        	}

        	break;

    	default:

        	return SNMP_GENERROR;
    	}
	}

    /* Return status. */
    return (status);
} /* tcpConnEntry */

#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )

/************************************************************************
*
* FUNCTION
*
*       tcp_Get_Bulk_1213TcpTab
*
* DESCRIPTION
*
*       This function processes the get bulk PDU action.
*
* INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       getflag            Flag to distinguish between get and get next
*                          request.
*       sublen             Length of sub-tree.
*       node               Object Identifier.
*
* OUTPUTS
*
*       Nucleus Status Code:
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The action was successfully completed.
*
*************************************************************************/
UINT16 tcp_Get_Bulk_1213TcpTab(snmp_object_t *obj, UINT16 idlen,
                               UINT8 getflag, UINT16 sublen, UINT32 *node)
{
    UINT16          status = SNMP_NOERROR;
    UINT16          maxRepetitions, index;
    snmp_object_t   dummyObject;

    /*picking the no of repetitions for this particular object*/
    maxRepetitions = (UINT16)obj[0].Syntax.LngUns;

    /*picking the object*/
    dummyObject= obj[0];

    /*running the loop for max number of repetitions*/
    for (index = 0;
         (index < maxRepetitions) && (status == SNMP_NOERROR);
         index++)
    {
        /* Get the object or the next object */
        if (!Get1213TcpTab((&dummyObject), idlen, sublen, node,
                           getflag))
        {
            if (index == 0)
                status = SNMP_NOSUCHNAME;
            break;
        }

        obj[index] = dummyObject;
    }

    return (status);
}

#endif /* ( (INCLUDE_SNMPv2 == NU_TRUE) || \
            (INCLUDE_SNMPv3 == NU_TRUE) ) */
#endif /* INCLUDE_SNMPv1_FULL_MIBII */
#endif /* ((RFC1213_TCP_INCLUDE == NU_TRUE) && \
           (MIB2_TCP_INCLUDE == NU_TRUE)) */
#endif /* (INCLUDE_TCP == NU_TRUE) */


