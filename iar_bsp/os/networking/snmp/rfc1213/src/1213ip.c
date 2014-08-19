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
*       1213ip.c                                                 
*
* DESCRIPTION
*
*        This file contains those functions specific to processing PDU
*        requests on parameters in the IP Group.
*
*   DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*        ipForwarding
*        ipDefaultTTL
*        ipInReceives
*        ipInHdrErrors
*        ipInAddrErrors
*        ipForwDatagrams
*        ipInUnknownProtos
*        ipInDiscards
*        ipInDelivers
*        ipOutRequests
*        ipOutDiscards
*        ipOutNoRoutes
*        ipReasmTimeout
*        ipReasmReqds
*        ipReasmOKs
*        ipReasmFails
*        ipFragOKs
*        ipFragFails
*        ipFragCreates
*        ipRoutingDiscards
*        ipAddrEntry
*        ipRouteEntry
*        ipNetToMediaEntry
*        ip_Get_Bulk_1213IpAddrTab
*        ip_Get_Bulk_1213IpRouteTab
*        ip_Get_Bulk_1213IpNet2MediaTab
*
*   DEPENDENCIES
*
*        nu_net.h
*        snmp_api.h
*        mib.h
*        1213ip.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/snmp_api.h"
#include "networking/mib.h"
#include "networking/1213ip.h"

#if (RFC1213_IP_INCLUDE == NU_TRUE)

#if (INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE)

UINT16  Get1213IpAddrTab(snmp_object_t *obj, UINT16 idlen, UINT16 sublen,
                         UINT32 node[], UINT8 getflag);
UINT16  Get1213IpRouteTab(snmp_object_t *obj, UINT16 idlen, UINT16 sublen,
                          UINT32 node[], UINT8 getflag);
UINT16  Set1213IpRouteTab (snmp_object_t *obj, UINT16 idlen);
UINT16  Get1213IpNet2MediaTab(snmp_object_t *obj, UINT16 idlen,
                             UINT16 sublen, UINT32 node[], UINT8 getflag);
UINT16  Set1213IpNet2MediaTab(snmp_object_t *obj, UINT16 idlen);
UINT16  ip_Get_Bulk_1213IpAddrTab(snmp_object_t *obj, UINT16 idlen,
                                  UINT8 getflag, UINT16 sublen,
                                  UINT32 *node);
UINT16  ip_Get_Bulk_1213IpRouteTab(snmp_object_t *obj, UINT16 idlen,
                                   UINT8 getflag, UINT16 sublen,
                                   UINT32 *node);
UINT16  ip_Get_Bulk_1213IpNet2MediaTab(snmp_object_t *obj, UINT16 idlen,
                              UINT8 getflag, UINT16 sublen, UINT32 *node);

#if ( (RFC1213_IP_INCLUDE == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE) )
/*************************************************************************
*
* FUNCTION
*
*        ipForwarding
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipForwarding
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*        SNMP_GENERROR          The PDU action was not completed.
*
*************************************************************************/
UINT16 ipForwarding(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status;

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
    INT32   commit_temp;
#endif

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
    {
        status = SNMP_NOERROR;

        if (obj->Request == SNMP_PDU_SET)
        {

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
            if (obj->Syntax.LngInt == 1)
                MIB2_ipForwarding_Set(1);

            else
            if (obj->Syntax.LngInt == 2)
                MIB2_ipForwarding_Set(2);

            else
#endif
            if (obj->Syntax.LngInt != 2)
                status = SNMP_WRONGVALUE;
        }

        /* In order to process commit*/
        else if (obj->Request == SNMP_PDU_COMMIT)
        {
#if (INCLUDE_IP_FORWARDING == NU_TRUE)

            /* Storing incoming value */
            commit_temp = obj->Syntax.LngInt;

            /* Picking new value */
            obj->Syntax.LngInt = MIB2_ipForwarding_Get;

            /* Comparing both values */
            if (commit_temp != obj->Syntax.LngInt)
            {
                status = SNMP_COMMITFAILED;
            }

#else
            obj->Syntax.LngInt = 2;
#endif

        }

        else
        {

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
            obj->Syntax.LngInt = MIB2_ipForwarding_Get;
#else
            obj->Syntax.LngInt = 2;
#endif

            obj->SyntaxLen = sizeof(obj->Syntax.LngInt);
        }
    }
    else
    {
        status = SNMP_NOSUCHNAME;
    }

    /* Returning success or error code. */
    return (status);

} /* ipForwarding */

/*************************************************************************
*
* FUNCTION
*
*        ipDefaultTTL
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipDefaultTTL
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipDefaultTTL(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;
    INT32   commit_temp;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_TRUE)
    {
        if (obj->Request == SNMP_PDU_SET)
        {
            if (obj->Syntax.LngInt > 1 && obj->Syntax.LngInt < 256) 
            {
                MIB2_ipDefaultTTL_Set((UINT8)obj->Syntax.LngInt);
            }
            else
            {
                status = SNMP_WRONGVALUE;
            }
        }

        /*in order to process commit*/
        else if (obj->Request == SNMP_PDU_COMMIT)
        {
            /*storing incoming value*/
            commit_temp = obj->Syntax.LngInt;

            /* picking new value */
            obj->Syntax.LngInt = MIB2_ipDefaultTTL_Get;

            /*comparing both values*/
            if (commit_temp != obj->Syntax.LngInt)
            {
                status = SNMP_COMMITFAILED;
            }
        }

        else
        {
            obj->Syntax.LngInt = MIB2_ipDefaultTTL_Get;
            obj->SyntaxLen = sizeof(obj->Syntax.LngInt);
        }
    }
    else
        status = SNMP_NOSUCHNAME;

    /* Return status. */
    return (status);

} /* ipDefaultTTL */

/*************************************************************************
*
* FUNCTION
*
*        ipInReceives
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipInReceives
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipInReceives(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;

    else if (obj->Request != SNMP_PDU_SET)
        obj->Syntax.LngUns = MIB2_ipInReceives_Get;

    else
        status = SNMP_NOSUCHNAME;

    /* Return status. */
    return (status);

} /* ipInReceives */

/*************************************************************************
*
* FUNCTION
*
*        ipInHdrErrors
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipInHdrErrors
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*        SNMP_GENERROR          The PDU action was not completed.
*
*************************************************************************/
UINT16 ipInHdrErrors(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;

    else if (obj->Request != SNMP_PDU_SET)
        obj->Syntax.LngUns = MIB2_ipInHdrErrors_Get;

    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* ipInHdrErrors */

/*************************************************************************
*
* FUNCTION
*
*        ipInAddrErrors
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipInAddrErrors
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipInAddrErrors(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipInAddrErrors_Get;

    return (status);

} /* ipInAddrErrors */

/*************************************************************************
*
* FUNCTION
*
*        ipForwDatagrams
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipForwDatagrams
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipForwDatagrams(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipForwDatagrams_Get;

    return (status);

} /* ipForwDatagrams */

/*************************************************************************
*
* FUNCTION
*
*        ipInUnknownProtos
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipInUnknownProtos
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipInUnknownProtos(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipInUnknownProtos_Get;

    return (status);

} /* ipInUnknownProtos */

/*************************************************************************
*
* FUNCTION
*
*        ipInDiscards
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipInDiscards
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipInDiscards(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipInDiscards_Get;

    return (status);

} /* ipInDiscards */

/*************************************************************************
*
* FUNCTION
*
*        ipInDelivers
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipInDelivers
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipInDelivers(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipInDelivers_Get;

    return (status);

} /* ipInDelivers */

/*************************************************************************
*
* FUNCTION
*
*        ipOutRequests
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipOutRequests
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipOutRequests(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipOutRequests_Get;

    return (status);

} /* ipOutRequests */

/*************************************************************************
*
* FUNCTION
*
*        ipOutDiscards
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipOutDiscards
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipOutDiscards(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipOutDiscards_Get;

    return (status);

} /* ipOutDiscards */

/*************************************************************************
*
* FUNCTION
*
*        ipOutNoRoutes
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipOutNoRoutes
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipOutNoRoutes(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipOutNoRoutes_Get;

    return (status);

} /* ipOutNoRoutes */

/*************************************************************************
*
* FUNCTION
*
*        ipReasmTimeout
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipReasmTimeout
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipReasmTimeout(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngInt = (INT32)MIB2_ipReasmTimeout_Get;

    return (status);

} /* ipReasmTimeout */

/*************************************************************************
*
* FUNCTION
*
*        ipReasmReqds
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipReasmReqds
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipReasmReqds(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipReasmReqds_Get;

    return (status);

} /* ipReasmReqds */

/*************************************************************************
*
* FUNCTION
*
*        ipReasmOKs
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipReasmOKs
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipReasmOKs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipReasmOKs_Get;

    return (status);

} /* ipReasmOKs */

/*************************************************************************
*
* FUNCTION
*
*        ipReasmFails
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipReasmFails
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipReasmFails(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipReasmFails_Get;

    return (status);

} /* ipReasmFails */

/*************************************************************************
*
* FUNCTION
*
*        ipFragOKs
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipFragOKs
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipFragOKs(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipFragOKs_Get;

    return (status);

} /* ipFragOKs */

/*************************************************************************
*
* FUNCTION
*
*        ipFragFails
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipFragFails
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipFragFails(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipFragFails_Get;

    return (status);

} /* ipFragFails */

/*************************************************************************
*
* FUNCTION
*
*        ipFragCreates
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipFragCreates
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipFragCreates(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipFragCreates_Get;

    return (status);

} /* ipFragCreates */

/*************************************************************************
*
* FUNCTION
*
*        ipRoutingDiscards
*
* DESCRIPTION
*
*        This function processes the PDU action on the ipRoutingDiscards
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 ipRoutingDiscards(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_ipRoutingDiscards_Get;

    return (status);

} /* ipRoutingDiscards */

/*************************************************************************
* FUNCTION
*
*        ipAddrEntry
*
* DESCRIPTION
*
*        This function is used to handle PDU request of ipAddr Table
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
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The action was successfully completed.
*
*************************************************************************/
UINT16 ipAddrEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT8           getflag = 0;
    UINT16          sublen = MIB2_MAX_NETADDRSIZE;
    UINT16          status = SNMP_NOERROR;
    UINT32          node[SNMP_SIZE_OBJECTID] = {1, 3, 6, 1, 2, 1, 4, 20,
                                                1, 0, 0, 0, 0, 0, 0};

	/* idlen should not exceed the size of node array. */
	if(idlen > SNMP_SIZE_OBJECTID)
	{
		status = SNMP_GENERROR;
	}
	
	else
	{
    	node[9] = obj->Id[9];

    	UNUSED_PARAMETER(param);

    	switch (obj->Request)
    	{
    	case SNMP_PDU_GET:

        	/* Set the getflag */
        	getflag++;

    	case SNMP_PDU_NEXT:

        	/* Get the object or the next object */
        	if (!Get1213IpAddrTab(obj, idlen, sublen, node, getflag))
            	status = SNMP_NOSUCHNAME;

        	break;

#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )

    	case SNMP_PDU_BULK:

        	status = ip_Get_Bulk_1213IpAddrTab(obj, idlen, getflag, sublen,
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
	}
	
    /* Return success or error code. */
    return (status);

} /* ipAddrEntry */

/*************************************************************************
* FUNCTION
*
*        ipRouteEntry
*
* DESCRIPTION
*
*        This function is used to handle PDU request of ipRoute Table
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
*       SNMP_GENERROR           If an general error occur while processing
*                               the request.
*       SNMP_NOERROR            The action was successfully completed.
*
*************************************************************************/
UINT16 ipRouteEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT8           getflag = 0;
    UINT16          sublen = MIB2_MAX_NETADDRSIZE;
    UINT16          status = SNMP_NOERROR;
    UINT32          node[SNMP_SIZE_OBJECTID] = {1, 3, 6, 1, 2, 1, 4, 21, 1
                                                , 0, 0, 0, 0, 0, 0};

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
        	getflag ++;

    		case SNMP_PDU_NEXT:

        	/* Get the object or the next object */
        	if (!Get1213IpRouteTab(obj, idlen, sublen, node, getflag))
            	status = SNMP_NOSUCHNAME;
        	break;

#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )

    	case SNMP_PDU_BULK:
	
        	status = ip_Get_Bulk_1213IpRouteTab(obj, idlen, getflag, sublen,
                                            node);
        	break;

#endif

    	case SNMP_PDU_COMMIT:

        	/* Return success code as we always have successful set. */
        	break;

    	case SNMP_PDU_UNDO:
    	case SNMP_PDU_SET:

        	if (!(memcmp(obj->Id, node,
                     (unsigned int)((idlen * sizeof(UINT32))))))
            	status = Set1213IpRouteTab(obj, idlen);
        	else
            	status = SNMP_NOSUCHNAME;

        	break;

    	default:

        	status = SNMP_GENERROR;

    	}
	}
    
	return (status);

} /* ipRouteEntry */

#endif  /* RFC1213_IP_INCLUDE == NU_TRUE */

/*************************************************************************
* FUNCTION
*
*        ipNetToMediaEntry
*
* DESCRIPTION
*
*        This function is used to handle PDU request of ipNetToMedia Table
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
UINT16 ipNetToMediaEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Flag to distinguish between get and get-next requests. */
    UINT8           getflag = 0;

    /* Sub-length of Object Identifier. */
    UINT16          sublen = MIB2_MAX_NETADDRSIZE;

    /* Status for returning success or error code. */
    UINT16          status = SNMP_NOERROR;

    UINT32          node[SNMP_SIZE_OBJECTID] = {1, 3, 6, 1, 2, 1, 4, 22,
                                                1, 0, 0, 0, 0, 0, 0, 0};

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);

    node[9] = obj->Id[9];

    switch (obj->Request)
    {
    case SNMP_PDU_GET:

        /* Set the getflag */
        getflag++;

        /* Fall through the next case */

    case SNMP_PDU_NEXT:

        /* Get the object or the next object */
        if (!Get1213IpNet2MediaTab(obj, idlen, sublen, node, getflag))
            status = SNMP_NOSUCHNAME;
        break;

#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )

    case SNMP_PDU_BULK:

        status = ip_Get_Bulk_1213IpNet2MediaTab(obj, idlen, getflag,
                                                sublen, node);
        break;

#endif

    case SNMP_PDU_COMMIT:

        /* Returning success code as we always have the successful set.
         */

        break;

    case SNMP_PDU_UNDO:
    case SNMP_PDU_SET:

        /* Compare the nodes. */
        if (!(memcmp(obj->Id, node,
             (unsigned int)((idlen * sizeof(UINT32))))))
            status = Set1213IpNet2MediaTab(obj, idlen);
        else
            status = SNMP_NOSUCHNAME;

        break;

    default:

        status = SNMP_GENERROR;
    }

    /* Return status */
    return (status);

} /* ipNetToMediaEntry */

#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )

/*************************************************************************
*
* FUNCTION
*
*       ip_Get_Bulk_1213IpAddrTab
*
* DESCRIPTION
*
*       This function processes the get bulk PDU action.
*
* INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*
* OUTPUTS
*
*       Nucleus Status Code:
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The action was successfully completed.
*
*************************************************************************/
UINT16 ip_Get_Bulk_1213IpAddrTab(snmp_object_t *obj, UINT16 idlen,
                                 UINT8 getflag, UINT16 sublen,
                                 UINT32 *node)
{
    UINT16          status = SNMP_NOERROR;
    UINT16          maxRepetitions,index;
    snmp_object_t   dummyObject;

    /*picking the no of repetitions for this particular object*/
    maxRepetitions = (UINT16)obj[0].Syntax.LngUns;

    /*picking the object*/
    dummyObject= obj[0];

    /*running the loop for max number of repetitions*/
    for (index = 0; index < maxRepetitions; index++)
    {
        /* Get the object or the next object */
        if (!Get1213IpAddrTab((&dummyObject), idlen, sublen,
                              node, getflag))
        {
            if (index == 0)
                status = SNMP_NOSUCHNAME;
            break;
        }

        obj[index] = dummyObject;
    }

    /* Return status */
    return (status);

} /* ip_Get_Bulk_1213IpAddrTab */

/*************************************************************************
*
* FUNCTION
*
*       ip_Get_Bulk_1213IpRouteTab
*
* DESCRIPTION
*
*       This function processes the get bulk PDU action.
*
* INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*
* OUTPUTS
*
*       Nucleus Status Code:
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The action was successfully completed.
*
*************************************************************************/
UINT16 ip_Get_Bulk_1213IpRouteTab(snmp_object_t *obj, UINT16 idlen,
                                  UINT8 getflag, UINT16 sublen,
                                  UINT32 *node)
{
    UINT16          status = SNMP_NOERROR;
    UINT16          maxRepetitions,index;
    snmp_object_t   dummyObject;

    /*picking the no of repetitions for this particular object*/
    maxRepetitions = (UINT16)obj[0].Syntax.LngUns;

    /*picking the object*/
    dummyObject= obj[0];

    /*running the loop for max number of repetitions*/
    for (index = 0; index < maxRepetitions; index++)
    {
        /* Get the object or the next object */
        if (!Get1213IpRouteTab((&dummyObject), idlen, sublen, node,
                               getflag))
        {
            if (index == 0)
                status = SNMP_NOSUCHNAME;
            break;
        }

        obj[index] = dummyObject;
    }

    /* Return status */
    return (status);

} /* ip_Get_Bulk_1213IpRouteTab */

/*************************************************************************
*
* FUNCTION
*
*       ip_Get_Bulk_1213IpNet2MediaTab
*
* DESCRIPTION
*
*       This function processes the get bulk PDU action.
*
* INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*
* OUTPUTS
*
*       Nucleus Status Code:
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The action was successfully completed.
*
*************************************************************************/
UINT16 ip_Get_Bulk_1213IpNet2MediaTab(snmp_object_t *obj, UINT16 idlen,
                                      UINT8 getflag, UINT16 sublen,
                                      UINT32 *node)
{
    UINT16          status = SNMP_NOERROR;
    UINT16          maxRepetitions,index;
    snmp_object_t   dummyObject;

    /*picking the no of repetitions for this particular object*/
    maxRepetitions = (UINT16)obj[0].Syntax.LngUns;

    /*picking the object*/
    dummyObject= obj[0];

    /*running the loop for max number of repetitions*/
    for (index = 0; index < maxRepetitions; index++)
    {
        /* Get the object or the next object */
        if (!Get1213IpNet2MediaTab((&dummyObject), idlen, sublen,
                                   node, getflag))
        {
            if (index == 0)
                status = SNMP_NOSUCHNAME;
            break;
        }

        obj[index] = dummyObject;
    }

    /* Return status */
    return (status);

} /* ip_Get_Bulk_1213IpNet2MediaTab */

#endif /* ( (INCLUDE_SNMPv2 == NU_TRUE) || \
            (INCLUDE_SNMPv3 == NU_TRUE) ) */
#endif /* ( (RFC1213_IP_INCLUDE == NU_TRUE) && \
            (MIB2_IP_INCLUDE == NU_TRUE) ) */
#endif /* INCLUDE_SNMPv1_FULL_MIBII */


