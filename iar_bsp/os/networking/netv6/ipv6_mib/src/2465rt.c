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
*        2465rt.c                                    
*
*   DESCRIPTION
*
*        This file contains the functions implementing IPv6 Route Table
*        MIB.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        ipv6RouteNumber
*        ipv6DiscardedRoutes
*        Get_ipv6RouteEntry
*        Set_ipv6RouteEntry
*        ipv6RouteEntry
*
*   DEPENDENCIES
*
*        nu_net.h
*        snmp_api.h
*        mib.h
*        ip6_mib.h
*        ip6_mib_s.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#include "networking/mib.h"
#include "networking/ip6_mib.h"
#include "networking/ip6_mib_s.h"

#if (INCLUDE_IPV6_RT == NU_TRUE)

STATIC UINT16 Get_ipv6RouteEntry(snmp_object_t *, UINT8);
STATIC UINT16 Set_ipv6RouteEntry(snmp_object_t *);

/************************************************************************
*
*   FUNCTION
*
*        ipv6RouteNumber
*
*   DESCRIPTION
*
*        This function is used to get the value of number route in the
*        route table.
*
*   INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
*   OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_ERROR             General error.
*        SNMP_NOERROR           The request was processed successfully.
*
************************************************************************/
UINT16 ipv6RouteNumber(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status for returning success or error code. */
    UINT16      status;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);

    /* Validating SNMP request. */
    if (MibSimple(obj, idlen) == NU_FALSE)
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }
    else
    {
        /* Getting count of instances in route table. */
        IP6_MIB_RT_GET_NUMBER((obj->Syntax.LngUns), status);
    }

    /* Return success or error code. */
    return (status);

} /* ipv6RouteNumber */

/************************************************************************
*
*   FUNCTION
*
*        ipv6DiscardedRoutes
*
*   DESCRIPTION
*
*        This function is used to get the value of number of discarded
*        routes.
*
*   INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
*   OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
************************************************************************/
UINT16 ipv6DiscardedRoutes(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status for returning success or error code. */
    UINT16      status;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);

    /* Validating SNMP request. */
    if (MibSimple(obj, idlen) == NU_FALSE)
    {
        /* We don't have valid request. Return error code. */
        status = SNMP_NOSUCHNAME;
    }
    else
    {
        /* Since we don't discard any route, so return count of discarded
         * routes as 'zero(0)'.
         */
        obj->Syntax.LngUns = 0;

        /* Return success code. */
        status = SNMP_NOERROR;
    }

    /* Return success or error code. */
    return (status);

} /* ipv6DiscardedRoutes */

/************************************************************************
*
*   FUNCTION
*
*        Get_ipv6RouteEntry
*
*   DESCRIPTION
*
*        This function is used to handle GET request on ipv6RouteTable.
*
*   INPUTS
*
*         *obj                  SNMP object containing request.
*         getflag               flag to distinguish between get
*                               and get next request.
*
*   OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOSUCHOBJECT      The instance does not exist in the table.
*        SNMP_ERROR             General error.
*        SNMP_NOERROR           The action was successfully completed.
*
************************************************************************/
STATIC UINT16 Get_ipv6RouteEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32                  table_oid[] = {1, 3, 6, 1, 2, 1, 55, 1, 11, 1};

    /* IPv6 Route destination. */
    UINT8                   ipv6_rt_dest[IP6_ADDR_LEN];

    /* Route prefix length. */
    UINT32                  ipv6_rt_pfx_len;

    /* Route index. */
    UINT32                  ipv6_rt_index;

    /* Variable to use in for-loop. */
    INT                     i;

    /* Status for returning success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Get route destination address. */
    for (i = 0; i < IP6_ADDR_LEN; i++)
    {
        ipv6_rt_dest[i] =
            (UINT8)(obj->Id[IP6_ROUTE_DEST_OFFSET + i]);
    }

    /* Getting prefix length. */
    ipv6_rt_pfx_len = obj->Id[IP6_ROUTE_PFX_LEN_OFFSET];

    /* Getting route index. */
    ipv6_rt_index = obj->Id[IP6_ROUTE_INDEX_OFFSET];

    /* If it is a GET request. */
    if (getflag)
    {
        /* Check if we have valid OID length. */
        if (obj->IdLen != (IP6_ROUTE_SUB_LEN + 1))
        {
            /* Return error code, because we don't have valid OID length.
             */
            status = SNMP_NOSUCHOBJECT;
        }
    }

    /* If it is get-next request. */
    else
    {
        /* Get indexes of next route entry. */
        IP6_MIB_RT_GET_NEXT(ipv6_rt_dest, ipv6_rt_pfx_len, 
                            ipv6_rt_index, status);
    }

    /* If no error till now then proceeds. */
    if (status == SNMP_NOERROR)
    {
        switch(obj->Id[IP6_ROUTE_ATTR_OFFSET])
        {
        case 4:     /* ipv6RouteIfIndex */

            /* Get the value of ipv6RouteIfIndex. */
            IP6_MIB_RT_GET_IF_INDEX(ipv6_rt_dest, ipv6_rt_pfx_len,
                                    ipv6_rt_index, (obj->Syntax.LngUns),
                                    status);

            break;

        case 5:     /* ipv6RouteNextHop */

            /* Get the value of ipv6RouteNextHop. */
            IP6_MIB_RT_GET_NEXT_HOP(ipv6_rt_dest, ipv6_rt_pfx_len,
                                    ipv6_rt_index, (obj->Syntax.BufChr),
                                    status);

            /* If we successfully got the value of ipv6RouteNextHop then
             * update the syntax length.
             */
            if (status == SNMP_NOERROR)
                obj->SyntaxLen = IP6_ADDR_LEN;

            break;

        case 6:     /* ipv6RouteType */

            /* Get the value of ipv6RouteType. */
            IP6_MIB_RT_GET_TYPE(ipv6_rt_dest, ipv6_rt_pfx_len,
                                ipv6_rt_index, (obj->Syntax.LngUns),
                                status);

            break;

        case 7:     /* ipv6RouteProtocol */

            /* Get the value of ipv6RouteProtocol. */
            IP6_MIB_RT_GET_PROTOCOL(ipv6_rt_dest, ipv6_rt_pfx_len,
                                    ipv6_rt_index, (obj->Syntax.LngUns),
                                    status);

            break;

        case 8:     /* ipv6RoutePolicy */

            /* Get the value of ipv6RoutePolicy. */
            IP6_MIB_RT_GET_POLICY(ipv6_rt_dest, ipv6_rt_pfx_len,
                                  ipv6_rt_index, (obj->Syntax.LngUns),
                                  status);

            break;

        case 9:     /* ipv6RouteAge */

            /* Get the value of ipv6RouteAge. */
            IP6_MIB_RT_GET_AGE(ipv6_rt_dest, ipv6_rt_pfx_len,
                               ipv6_rt_index, (obj->Syntax.LngUns),
                               status);

            break;

        case 10:    /* ipv6RouteNextHopRDI */

            /* Get the value of ipv6RouteNextHopRDI. */
            IP6_MIB_RT_GET_NEXT_HOP_RDI(ipv6_rt_dest, ipv6_rt_pfx_len,
                                        ipv6_rt_index,
                                        (obj->Syntax.LngUns), status);

            break;

        case 11:    /* ipv6RouteMetric */

            /* Get the value of ipv6RouteMetric. */
            IP6_MIB_RT_GET_METRIC(ipv6_rt_dest, ipv6_rt_pfx_len,
                                  ipv6_rt_index, (obj->Syntax.LngUns),
                                  status);

            break;

        case 12:    /* ipv6RouteWeight */

            /* Get the value of ipv6RouteWeight. */
            IP6_MIB_RT_GET_WEIGHT(ipv6_rt_dest, ipv6_rt_pfx_len,
                                  ipv6_rt_index, (obj->Syntax.LngUns),
                                  status);

            break;

        case 13:    /* ipv6RouteInfo */

            /* Get the value of ipv6RouteInfo. */
            IP6_MIB_RT_GET_INFO(ipv6_rt_dest, ipv6_rt_pfx_len,
                                ipv6_rt_index, (obj->Syntax.BufInt),
                                (obj->SyntaxLen), status);

            break;

        case 14:    /* ipv6RouteValid */

            /* Get the value of ipv6RouteValid. */
            IP6_MIB_RT_GET_VALID(ipv6_rt_dest, ipv6_rt_pfx_len,
                                 ipv6_rt_index, (obj->Syntax.LngUns),
                                 status);

            break;

        default:
            /* We have reached the end of the table.
               There are no more attributes in this table. */
            status = SNMP_NOSUCHNAME;
        }

        /* If request full-filled successfully and request was of GET_NEXT
         * then update the object OID.
         */
        if (status == SNMP_NOERROR)
        {
            /* Update the table OID. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update route destination address. */
            for (i = 0; i < IP6_ADDR_LEN; i++)
            {
                obj->Id[IP6_ROUTE_DEST_OFFSET + i] =
                    (((UINT32)ipv6_rt_dest[i]) & 0xff);
            }

            /* Update prefix length. */
            obj->Id[IP6_ROUTE_PFX_LEN_OFFSET] = ipv6_rt_pfx_len;

            /* Update route index. */
            obj->Id[IP6_ROUTE_INDEX_OFFSET] = ipv6_rt_index;

            obj->IdLen = (IP6_ROUTE_SUB_LEN + 1);
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_ipv6RouteEntry */

/************************************************************************
*
*   FUNCTION
*
*        Set_ipv6RouteEntry
*
*   DESCRIPTION
*
*        This function is used to handle SET request on ipv6RouteTable.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
*
*   OUTPUTS                                                              
*
*        SNMP_NOERROR           If the processing succeeds.
*        SNMP_BADVALUE          If invalid value.
*        SNMP_NOSUCHOBJECT      If the instance does not exist in table.
*        SNMP_ERROR             General error.
*        SNMP_NOSUCHNAME        Object does not exist.
*
************************************************************************/
STATIC UINT16 Set_ipv6RouteEntry(snmp_object_t *obj)
{
    /* IPv6 Route destination. */
    UINT8                   ipv6_rt_dest[IP6_ADDR_LEN];

    /* Route prefix length. */
    UINT32                  ipv6_rt_pfx_len;

    /* Route index. */
    UINT32                  ipv6_rt_index;

    /* Variable to use in for-loop. */
    INT                     i;

    /* Status for returning success or error code. */
    UINT16                  status;

    /* Validate the OID. */
    if (obj->IdLen != (IP6_ROUTE_SUB_LEN + 1))
    {
        /* Return error code because, we have invalid OID. */
        status = SNMP_NOSUCHOBJECT;
    }
    else
    {
        /* Get route destination address. */
        for (i = 0; i < IP6_ADDR_LEN; i++)
        {
            ipv6_rt_dest[i] =
                (UINT8)(obj->Id[IP6_ROUTE_DEST_OFFSET + i]);
        }

        /* Getting prefix length. */
        ipv6_rt_pfx_len = obj->Id[IP6_ROUTE_PFX_LEN_OFFSET];

        /* Getting route index. */
        ipv6_rt_index = obj->Id[IP6_ROUTE_INDEX_OFFSET];

        switch(obj->Id[IP6_ROUTE_ATTR_OFFSET])
        {
        case 14:    /* ipv6RouteValid */

            /* Set value to ipv6RouteValid. */
            IP6_MIB_RT_SET_VALID(ipv6_rt_dest, ipv6_rt_pfx_len,
                                 ipv6_rt_index, obj->Syntax.LngUns,
                                 status);

            break;

        default:
            status = SNMP_NOSUCHNAME;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Set_ipv6RouteEntry */

/************************************************************************
*
*   FUNCTION
*
*        ipv6RouteEntry
*
*   DESCRIPTION
*
*        This is used to handle SNMP requests on ipv6RouteTable.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  length of SNMP request.
*        *param                 Additional parameters (Not used).
*
*   OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeded.
*        SNMP_BADVALUE          If invalid value.
*        SNMP_NOSUCHNAME        Object does not exist.
*        SNMP_NOSUCHOBJECT      If the instance does not exist in table.
*        SNMP_ERROR             General error.
*        SNMP_GENERROR          Invalid request.
*
************************************************************************/
UINT16 ipv6RouteEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Flag to distinguish between get and get-next requests. */
    UINT8         getflag = 0;

    /* Status for returning success or error code. */
    UINT16        status;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);
    UNUSED_PARAMETER(idlen);

    /* Identifying request type and taking appropriate actions. */
    switch(obj->Request)
    {
    case SNMP_PDU_GET:                      /* Get request. */

        /* Set getflag so that it can represent get-request. */
        getflag++;

        /* Fall through the next case for fulfilling the get-request. */

    case SNMP_PDU_NEXT:                     /* Get next request. */

        /* Processing get operations. */
        status = Get_ipv6RouteEntry(obj, getflag);
        break;

    case SNMP_PDU_UNDO:                     /* Undo request. */
    case SNMP_PDU_SET:                      /* Set request. */

        /* Processing of set operations. */
        status = Set_ipv6RouteEntry(obj);
        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = IPV6_GET_BULK(obj, Get_ipv6RouteEntry);
        break;

    case SNMP_PDU_COMMIT:                   /* Commit request. */

        /* We will always have a successful set. */
        status = SNMP_NOERROR;
        break;

    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* ipv6RouteEntry */

#endif /* (INCLUDE_IPV6_RT == NU_TRUE) */
#endif /* (INCLUDE_SNMP == NU_TRUE) */
