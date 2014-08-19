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
*       1213udp.c                                                
*
* DESCRIPTION
*
*        This file contains those functions specific to processing PDU
*        requests on parameters in the UDP Group.
*
* DATA STRUCTURES
*
*        None
*
* FUNCTIONS
*
*        udpInDatagrams
*        udpNoPorts
*        udpInErrors
*        udpOutDatagrams
*        udpEntry
*        udp_Get_Bulk_1213UdpTab
*
* DEPENDENCIES
*
*        nu_net.h
*        snmp_api.h
*        1213udp.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/snmp_api.h"
#include "networking/1213udp.h"

#if ((RFC1213_UDP_INCLUDE == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE))

#if (INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE)

UINT16  Get1213UdpTab(snmp_object_t *obj, UINT16 idlen, UINT16 sublen,
                      UINT32 node[], UINT8 getflag);
UINT16  udp_Get_Bulk_1213UdpTab(snmp_object_t *obj, UINT16 idlen,
                                UINT8 getflag, UINT16 sublen,
                                UINT32 *node);

/************************************************************************
*
* FUNCTION
*
*       udpInDatagrams
*
* DESCRIPTION
*
*       This function processes the PDU action on the udpInDatagrams
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
UINT16 udpInDatagrams(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_udpInDatagrams_Get;

    return (status);

} /* udpInDatagrams */

/************************************************************************
*
* FUNCTION
*
*       udpNoPorts
*
* DESCRIPTION
*
*       This function processes the PDU action on the udpNoPorts
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
UINT16 udpNoPorts(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_udpNoPorts_Get;

    return (status);

} /* udpNoPorts */

/************************************************************************
*
* FUNCTION
*
*       udpInErrors
*
* DESCRIPTION
*
*       This function processes the PDU action on the udpInErrors
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
UINT16 udpInErrors(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_udpInErrors_Get;

    return (status);

} /* udpInErrors */

/************************************************************************
*
* FUNCTION
*
*       udpOutDatagrams
*
* DESCRIPTION
*
*       This function processes the PDU action on the udpOutDatagrams
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
UINT16 udpOutDatagrams(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        obj->Syntax.LngUns = MIB2_udpOutDatagrams_Get;

    return (status);

} /* udpOutDatagrams */

/*************************************************************************
* FUNCTION
*
*        udpEntry
*
* DESCRIPTION
*
*        This function is used to handle PDU request of UDP Table
*        entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        getflag                flag to distinguish between get
*                               and get next request.
*
* OUTPUTS
*
*       SNMP_NOSUCHNAME         The object does not exist.
*       SNMP_NOERROR            The action was successfully completed.
*
*************************************************************************/
UINT16 udpEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Flag to distinguish between get and get-next requests. */
    UINT8           getflag = 0;

    /* Sub-length of object identifier. */
    UINT16          sublen = 5;

    /* Status for returning success or error code. */
    UINT16          status = SNMP_NOERROR;

    UINT32          node[SNMP_SIZE_OBJECTID] = {1, 3, 6, 1, 2, 1, 7, 5, 1,
                                                0, 0, 0, 0, 0, 0, 0};

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
        	if (!Get1213UdpTab(obj, idlen, sublen, node, getflag))
            	status = SNMP_NOSUCHNAME;
        	break;

#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )

    	case SNMP_PDU_BULK:

        	status = udp_Get_Bulk_1213UdpTab(obj, idlen, getflag, sublen,
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
	
    return (status);

} /* udpEntry */

#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )

/************************************************************************
*
* FUNCTION
*
*       udp_Get_Bulk_1213UdpTab
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
UINT16 udp_Get_Bulk_1213UdpTab(snmp_object_t *obj, UINT16 idlen,
                               UINT8 getflag, UINT16 sublen, UINT32 *node)
{
    UINT16          status = SNMP_NOERROR;
    UINT16          maxRepetitions, index;
    snmp_object_t   dummyObject;

    /*picking the no of repetitions for this particular object*/
    maxRepetitions = (UINT16)obj[0].Syntax.LngUns;

    /*picking the object*/
    dummyObject = obj[0];

    /*running the loop for max number of repetitions*/
    for (index = 0; index < maxRepetitions; index++)
    {
        /* Get the object or the next object */
        if (!Get1213UdpTab((&dummyObject), idlen, sublen, node,
                           getflag))
        {
            if(index == 0)
                status = SNMP_NOSUCHNAME;
            break;
        }

        obj[index] = dummyObject;
    }

    return (status);

} /* udp_Get_Bulk_1213UdpTab */

#endif /* ( (INCLUDE_SNMPv2 == NU_TRUE) || \
            (INCLUDE_SNMPv3 == NU_TRUE) ) */
#endif /* INCLUDE_SNMPv1_FULL_MIBII */
#endif /* ((RFC1213_UDP_INCLUDE == NU_TRUE) && \
           (MIB2_UDP_INCLUDE == NU_TRUE)) */


