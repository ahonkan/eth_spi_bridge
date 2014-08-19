/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS 
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS 
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME                                        
*
*        2466icmp.c                                  
*
*   DESCRIPTION
*
*        This file contains the function that are responsible for handling
*        SNMP requests on ipv6IfIcmpTable.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        Get_ipv6IfIcmpEntry
*        ipv6IfIcmpEntry
*
*   DEPENDENCIES
*
*        nu_net.h
*        snmp_api.h
*        ip6_mib_s.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#include "networking/ip6_mib_s.h"

#if (INCLUDE_IPV6_ICMP_MIB == NU_TRUE)

STATIC UINT16 Get_ipv6IfIcmpEntry(snmp_object_t *, UINT8);

/************************************************************************
*
*   FUNCTION
*
*        Get_ipv6IfIcmpEntry
*
*   DESCRIPTION
*
*        This function is used to handle the GET or GET-NEXT request on
*        ipv6IfIcmpTable.
*
*   INPUTS
*
*         *obj                  SNMP object containing request.
*         getflag               flag to distinguish between get
*                               and get next request.
*
*   OUTPUTS
*
*        SNMP_NO_ERROR          When successful.
*        SNMP_NOSUCHOBJECT      When interface index specified in SNMP
*                               request is invalid.
*        SNMP_ERROR             General error.
*        SNMP_NOSUCHNAME        Invalid SNMP request.
*
************************************************************************/
STATIC UINT16 Get_ipv6IfIcmpEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32      table_oid[] = {1, 3, 6, 1, 2, 1, 56, 1, 1, 1};

    /* Interface index. */
    UINT32      if_index;

    /* Status for returning success or error code. */
    UINT16      status = SNMP_NOERROR;

    /* Get interface index from OID. */
    if_index = obj->Id[IP6_IF_ICMP_IF_INDEX_OFFSET];

    /* If we have GET request. */
    if (getflag)
    {
        /* If we have invalid OID then return error code. */
        if (obj->IdLen != (IP6_IF_ICMP_SUB_LEN + 1))
            status = SNMP_NOSUCHOBJECT;
    }

    /* If we have GET-NEXT request. */
    else
    {
        /* Get the indexes of next ICMP entry. */
        IP6_ICMP_MIB_GET_NEXT(if_index, status);
    }

    if (status == SNMP_NOERROR)
    {
        IP6_MIB_ICMP_STAT_GET(if_index, obj->Id[IP6_IF_ICMP_ATTR_OFFSET],
                              obj->Syntax.LngUns, status);

        /* If request was successful and we had GET-NEXT request then
         * update the object OID.
         */
        if ( (status == SNMP_NOERROR) && (!getflag) )
        {
            /* Update the object identifier. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update the interface index. */
            obj->Id[IP6_IF_ICMP_IF_INDEX_OFFSET] = if_index;

            /* Update OID length. */
            obj->IdLen = (IP6_IF_ICMP_SUB_LEN + 1);
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_ipv6IfIcmpEntry */

/************************************************************************
*
*   FUNCTION
*
*        ipv6IfIcmpEntry
*
*   DESCRIPTION
*
*        This function is used to handle SNMP requests on 
*        ipv6IfIcmpTable.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  Length of SNMP request.
*        *param                 Additional parameters (Not used).
*
*   OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeds.
*        SNMP_READONLY          If object is read-only.
*        SNMP_BADVALUE          If invalid value.
*        SNMP_NOSUCHNAME        Object does not exist.
*        SNMP_NOSUCHOBJECT      Instance does not exist.
*
************************************************************************/
UINT16 ipv6IfIcmpEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16          status;

    /* Flag to distinguish between get and get-next requests. */
    UINT8           getflag = 0;

    /* Avoid compilation warnings. */
    UNUSED_PARAMETER(param);
    UNUSED_PARAMETER(idlen);

    switch(obj->Request)
    {
    case SNMP_PDU_GET:                      /* Get request. */

        /* Set getflag so that it can represent get-request. */
        getflag++;

        /* Fall through the next case for fulfilling the get-request. */

    case SNMP_PDU_NEXT:                     /* Get next request. */

        /* Processing get operations. */
        status = Get_ipv6IfIcmpEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = IPV6_GET_BULK(obj, Get_ipv6IfIcmpEntry);
        break;


    case SNMP_PDU_UNDO:                     /* Undo request. */
    case SNMP_PDU_SET:                      /* Set request. */
    case SNMP_PDU_COMMIT:                   /* Commit request. */
    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* ipv6IfIcmpEntry */

#endif /* (INCLUDE_IPV6_ICMP_MIB == NU_TRUE) */
#endif /* (INCLUDE_SNMP == NU_TRUE) */
