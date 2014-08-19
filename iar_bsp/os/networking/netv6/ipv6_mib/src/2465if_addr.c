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
*        2465if_addr.c                               
*
*   DESCRIPTION
*
*        This file contains the functions that implements MIBs for
*        ipv6AddrTable.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        Get_ipv6AddrEntry
*        ipv6AddrEntry
*
*   DEPENDENCIES
*
*        nu_net.h
*        ip6_mib
*        snmp_api.h
*        ip6_mib_s.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/ip6_mib.h"
#include "networking/snmp_api.h"
#include "networking/ip6_mib_s.h"

#if (INCLUDE_IPV6_IF_ADDR_MIB == NU_TRUE)

STATIC UINT16 Get_ipv6AddrEntry(snmp_object_t *, UINT8);

/************************************************************************
*
*   FUNCTION
*
*        Get_ipv6AddrEntry
*
*   DESCRIPTION
*
*        This function is used to handle the GET request on ipv6AddrTable.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
*        getflag                Flag to distinguish GET and GET-NEXT
*                               request.
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
STATIC UINT16 Get_ipv6AddrEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Interface index. */
    UINT32          if_index;

    /* IPv6 address of the interface. */
    UINT8           ipv6_addr[IP6_ADDR_LEN];

    /* Object identifier of table entry. */
    UINT32          table_oid[] = {1, 3, 6, 1, 2, 1, 55, 1, 8, 1};

    /* Variable to use in for loop. */
    INT             i;

    /* Status to return success or error code. */
    UINT16          status = SNMP_NOERROR;

    /* Getting Interface index. */
    if_index = obj->Id[IP6_IF_ADDR_IF_INDEX_OFFSET];

    /* Getting IPv6 address of interface. */
    for (i = 0; i < IP6_ADDR_LEN; i++)
    {
        /* If we have valid IPv6 address. */
        if (obj->Id[IP6_IF_ADDR_ADDR_OFFSET + i] <= (UINT32)(0xff))
        {
            ipv6_addr[i] = (UINT8)obj->Id[IP6_IF_ADDR_ADDR_OFFSET + i];
        }

        /* Return error code if we don't have valid IPv6 address. */
        else
        {
            status = SNMP_NOSUCHOBJECT;
        }
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* If this GET request then check OID validity. */
        if (getflag)
        {
            /* If we have invalid OID then return error code. */
            if (obj->IdLen != (IP6_IF_ADDR_SUB_LEN  + 1))
            {
                /* Return error code. */
                status = SNMP_NOSUCHOBJECT;
            }
        }

        /* If we have GET-NEXT request. */
        else
        {
            /* Get the indexes of next interface address entry. */
            IP6_IF_ADDR_MIB_GET_NEXT(if_index, ipv6_addr, status);
        }
    }

    /* If there is no error till now. */
    if (status == SNMP_NOERROR)
    {
        switch(obj->Id[IP6_IF_ADDR_ATTR_OFFSET])
        {
        case 2:     /* ipv6AddrPfxLength */

            /* Get the value of ipv6AddrPfxLength. */
            IP6_IF_ADDR_MIB_GET_PFX_LENGTH(if_index, ipv6_addr,
                                           (obj->Syntax.LngUns), status);

            break;

        case 3:     /* ipv6AddrType */

            /* Get the value of ipv6AddrType. */
            IP6_IF_ADDR_MIB_GET_TYPE(if_index, ipv6_addr,
                                     (obj->Syntax.LngUns), status);

            break;

        case 4:     /* ipv6AddrAnycastFlag */

            /* Get the value of ipv6AddrAnycastFlag. */
            IP6_IF_ADDR_MIB_GET_ANYCASTFLAG(if_index, ipv6_addr,
                                            (obj->Syntax.LngUns), status);

            break;

        case 5:     /* ipv6AddrStatus */

            /* Get the value of ipv6AddrStatus. */
            IP6_IF_ADDR_MIB_GET_STATUS(if_index, ipv6_addr,
                                        (obj->Syntax.LngUns), status);

            break;

        default:
            /* We have reached at end of table. */
            status = SNMP_NOSUCHNAME;
        }

        /* If the request was successful and we have GET-NEXT request then
         * update the object OID.
         */
        if ( (status == SNMP_NOERROR) && (!getflag) )
        {
            /* Update the table entry OID. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Updating Interface index. */
            obj->Id[IP6_IF_ADDR_IF_INDEX_OFFSET] = if_index;

            /* Update the ipv6AddrAddress. */
            for (i = 0; i < IP6_ADDR_LEN; i++)
            {
                obj->Id[(IP6_IF_ADDR_ADDR_OFFSET + i)] =
                    (((UINT32)ipv6_addr[i]) & 0xff);
            }

            /* Update OID length. */
            obj->IdLen = IP6_IF_ADDR_SUB_LEN + 1;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_ipv6AddrEntry */

/************************************************************************
*
*   FUNCTION
*
*        ipv6AddrEntry
*
*   DESCRIPTION
*
*        This function handle all SNMP request for the ipv6AddrTable.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  Length of SNMP request.
*        *param                 Additional parameters (Not used).
*
*   OUTPUTS
*
*        SNMP_NO_ERROR          When successful.
*        SNMP_NOSUCHOBJECT      When interface index specified in SNMP
*                               request is invalid.
*        SNMP_ERROR             General error.
*        SNMP_NOSUCHNAME        Invalid SNMP request.
*        SNMP_GEN_ERROR         General error.
*
************************************************************************/
UINT16 ipv6AddrEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16      status;

    /* Flag to distinguish between get and get-next requests. */
    UINT8       getflag = 0;

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
        status = Get_ipv6AddrEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = IPV6_GET_BULK(obj, Get_ipv6AddrEntry);
        break;


    case SNMP_PDU_UNDO:                     /* Undo request. */
    case SNMP_PDU_SET:                      /* Set request. */
    case SNMP_PDU_COMMIT:                   /* Commit request. */
    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* ipv6AddrEntry */

#endif /* (INCLUDE_IPV6_IF_ADDR_MIB == NU_TRUE) */
#endif /* (INCLUDE_SNMP == NU_TRUE) */
