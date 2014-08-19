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
*        2465addr_pre.c                              
*
*   DESCRIPTION
*
*        This file contains the implementation of IPv6 Prefix Address
*        MIBs.
*
*   DATA STRUCTURES
*
*        None
*
*   FUNCTIONS
*
*        Get_ipv6AddrPrefixEntry
*        ipv6AddrPrefixEntry
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

#if (INCLUDE_IP6_MIB_ADDR_PREFIX == NU_TRUE)

STATIC UINT16 Get_ipv6AddrPrefixEntry(snmp_object_t *, UINT8);

/************************************************************************
*
*   FUNCTION
*
*        Get_ipv6AddrPrefixEntry
*
*   DESCRIPTION
*
*        This function is used to handle SNMP GET request on
*        'ipv6AddrPrefixTable'.
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
STATIC UINT16 Get_ipv6AddrPrefixEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32      table_oid[] = {1, 3, 6, 1, 2, 1, 55, 1, 7, 1};

    /* Interface index. */
    UINT32      if_index;

    /* Variable to hold prefix address. */
    UINT8       prefix_addr[IP6_ADDR_LEN];

    /* Prefix length. */
    UINT32      prefix_len;

    /* Variable to use in for loop. */
    INT         i;

    /* Status for returning success or error code. */
    UINT16      status = SNMP_NOERROR;

    /* Getting Interface index. */
    if_index = obj->Id[IPV6_PRE_ADDR_IF_INDEX_OFFSET];

    /* Getting prefix address. */
    for (i = 0; i < IP6_ADDR_LEN; i++)
    {
        /* If we have valid object IPv6 address. */
        if (obj->Id[IPV6_PRE_ADDR_PRE_ADDR_OFFSET + i] <= (UINT32)(0xff))
        {
            prefix_addr[i] =
                    (UINT8)(obj->Id[IPV6_PRE_ADDR_PRE_ADDR_OFFSET + i]);
        }

        /* If we don't have valid IPv6 address then return error code. */
        else
        {
            status = SNMP_NOSUCHOBJECT;
        }
    }

    /* Getting value of prefix length. */
    prefix_len = obj->Id[IPV6_PRE_ADDR_LEN_OFFSET];

    /* If no error till now. */
    if (status == SNMP_NOERROR)
    {
        /* If we have GET request. */
        if (getflag)
        {
            /* If we have invalid OID then return error code. */
            if (obj->IdLen != (IPV6_PRE_ADDR_SUB_LEN + 1) )
            {
                status = SNMP_NOSUCHOBJECT;
            }
        }

        /* If we have GET-NEXT request. */
        else
        {
            /* Get the indexes of next prefix address entry. */
            IP6_MIB_ADDR_PRE_GET_NEXT(if_index, prefix_addr, prefix_len,
                                      status);
        }
    }

    /* If there is no error till now. */
    if (status == SNMP_NOERROR)
    {
        /* Process the request. */
        switch (obj->Id[IPV6_PRE_ADDR_ATTR_OFFSET])
        {
        case 3:     /* ipv6AddrPrefixOnLinkFlag */

            /* Get the value of 'ipv6AddrPrefixOnLinkFlag'. */
            IP6_MIB_ADDR_PRE_GET_ONLINKFLAG(if_index, prefix_addr,
                                            prefix_len, obj->Syntax.LngUns,
                                            status);

            break;

        case 4:      /* ipv6AddrPrefixAutonomousFlag */

            /* Get the value of 'ipv6AddrPrefixAutonomousFlag'. */
            IP6_MIB_ADDR_PRE_GET_AUTMOSFLAG(if_index, prefix_addr,
                                            prefix_len, obj->Syntax.LngUns,
                                            status);

            break;

        case 5:     /* ipv6AddrPrefixAdvPreferredLifetime */

            /* Get the value of 'ipv6AddrPrefixAdvPreferredLifetime'. */
            IP6_MIB_ADDR_PRE_GET_PREFERLIFE(if_index, prefix_addr,
                                            prefix_len, obj->Syntax.LngUns,
                                            status);

            break;

        case 6:     /* ipv6AddrPrefixAdvValidLifetime */

            /* Get the value of 'ipv6AddrPrefixAdvValidLifetime'. */
            IP6_MIB_ADDR_PRE_GET_VALIDLIFE(if_index, prefix_addr,
                                           prefix_len, obj->Syntax.LngUns,
                                           status);

            break;

        default:

            /* We have reached the end of table. */
            status = SNMP_NOSUCHNAME;
            break;
        }

        /* If request was successful and we have GET-NEXT request then
         * update the OID.
         */
        if ( (status == SNMP_NOERROR) && (!getflag) )
        {
            /* Update the table OID. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Updating Interface index. */
            obj->Id[IPV6_PRE_ADDR_IF_INDEX_OFFSET] = if_index;

            /* Updating prefix address. */
            for (i = 0; i < IP6_ADDR_LEN; i++)
            {
                obj->Id[IPV6_PRE_ADDR_PRE_ADDR_OFFSET + i] =
                    (((UINT32)(prefix_addr[i])) & 0xff);
            }

            /* Updating value of prefix length. */
            obj->Id[IPV6_PRE_ADDR_LEN_OFFSET] = prefix_len;

            /* Update the OID length. */
            obj->IdLen = IPV6_PRE_ADDR_SUB_LEN + 1;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_ipv6AddrPrefixEntry */

/************************************************************************
*
*   FUNCTION
*
*        ipv6AddrPrefixEntry
*
*   DESCRIPTION
*
*        This function is used to handle SNMP request on
*        'ipv6AddrPrefixTable'.
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
UINT16 ipv6AddrPrefixEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
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
        status = Get_ipv6AddrPrefixEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = IPV6_GET_BULK(obj, Get_ipv6AddrPrefixEntry);
        break;


    case SNMP_PDU_UNDO:                     /* Undo request. */
    case SNMP_PDU_SET:                      /* Set request. */
    case SNMP_PDU_COMMIT:                   /* Commit request. */
    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* ipv6AddrPrefixEntry */

#endif /* (INCLUDE_IP6_MIB_ADDR_PREFIX == NU_TRUE) */
#endif /* (INCLUDE_SNMP == NU_TRUE) */
