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
*        2465ntm.c                                   
*
*   DESCRIPTION
*
*        This file contains the functions that are responsible for
*        handling SNMP requests on ipv6NetToMediaTable.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        Get_ipv6NetToMediaEntry
*        ipv6NetToMediaEntry
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

#if (INCLUDE_IPV6_MIB_NTM == NU_TRUE)

STATIC UINT16 Get_ipv6NetToMediaEntry(snmp_object_t *, UINT8);

/************************************************************************
*
*   FUNCTION
*
*        Get_ipv6NetToMediaEntry
*
*   DESCRIPTION
*
*        This is used to handle SNMP Get requests on ipv6NetToMediaTable.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
*        getflag                flag to distinguish between get 
*                               and get next request.
*
*   OUTPUTS
*
*        SNMP_NOERROR           When successful.
*        SNMP_NOSUCHOBJECT      When interface index specified in SNMP
*                               request is invalid.
*        SNMP_ERROR             General error.
*        SNMP_NOSUCHNAME        Invalid SNMP request.
*
************************************************************************/
STATIC UINT16 Get_ipv6NetToMediaEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32          table_oid[] = {1, 3, 6, 1, 2, 1, 55, 1, 12, 1};

    /* Interface index. */
    UINT32          if_index;

    /* IPv6 Address. */
    UINT8           ipv6_addr[IP6_ADDR_LEN];

    /* Variable to use in for-loop. */
    INT             i;

    /* Status to return success or error code. */
    UINT16          status = SNMP_NOERROR;

    /* Get interface index. */
    if_index = obj->Id[IP6_NTM_IF_INDEX_OFFSET];

    /* Get IPv6 address. */
    for (i = 0; i < IP6_ADDR_LEN; i++)
    {
        /* If we have valid IPv6 address. */
        if (obj->Id[IP6_NTM_IF_NET_ADDR_OFFSET + i] <= (UINT32)(0xff))
        {
            ipv6_addr[i] =
                (UINT8)(obj->Id[IP6_NTM_IF_NET_ADDR_OFFSET + i]);
        }

        /* If we don't have valid IPv6 address then return error code. */
        else
        {
            status = SNMP_NOSUCHOBJECT;
        }
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* If we are handling GET request. */
        if (getflag)
        {
            if (obj->IdLen != (IP6_NTM_SUB_LEN + 1))
                status = SNMP_NOSUCHOBJECT;
        }

        /* If we are handling GET-NEXT request. */
        else
        {
            IP6_MIB_NTM_GET_NEXT_INDEX(if_index, ipv6_addr, status);
        }
    }

    /* If we did not encounter any error till now then proceed otherwise
       return error code. */
    if (status == SNMP_NOERROR)
    {
        /* Process the GET / GET-NEXT request. */
        IP6_MIB_NTM_GET(if_index, ipv6_addr, &(obj->Syntax),
                        (INT)(obj->Id[IP6_NTM_ATTR_OFFSET]), status);

        /* If we are handling GET-NEXT request and we did not encounter
         * any error then update the OID.
         */
        if ( (status == SNMP_NOERROR) && (!(getflag)) )
        {
            /* Set syntax length in case of ipv6NetToMediaPhysAddress. */
            if (obj->Id[IP6_NTM_ATTR_OFFSET] == 2)
                obj->SyntaxLen = DADDLEN;

            /* Updating OID. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Set interface index. */
            obj->Id[IP6_NTM_IF_INDEX_OFFSET] = if_index;

            /* Set IPv6 address. */
            for (i = 0; i < IP6_ADDR_LEN; i++)
            {
                obj->Id[IP6_NTM_IF_NET_ADDR_OFFSET + i] =
                    (UINT32)(((UINT32)ipv6_addr[i]) & 0xff);
            }

            /* Update the OID length. */
            obj->IdLen = IP6_NTM_SUB_LEN + 1;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_ipv6NetToMediaEntry */

/************************************************************************
*
*   FUNCTION
*
*        ipv6NetToMediaEntry
*
*   DESCRIPTION
*
*        This function is used to handle SNMP requests on
*        ipv6NetToMediaTable.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  Length of SNMP request.
*        *param                 Additional parameters (Not used).
*
*   OUTPUTS
*
*        SNMP_NOERROR           When successful.
*        SNMP_NOSUCHOBJECT      When interface index specified in SNMP
*                               request is invalid.
*        SNMP_ERROR             General error.
*        SNMP_NOSUCHNAME        Invalid SNMP request.
*        SNMP_GENERROR          Invalid request type.
*
************************************************************************/
UINT16 ipv6NetToMediaEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status for returning success or error code. */
    UINT16                  status;

    /* Flag to distinguish between get and get-next requests. */
    UINT8                   getflag = 0;

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

    case SNMP_PDU_NEXT:                     /* Get-next request. */

        /* Processing get operations. */
        status = Get_ipv6NetToMediaEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = IPV6_GET_BULK(obj, Get_ipv6NetToMediaEntry);

        break;

    case SNMP_PDU_SET:                      /* Set request. */
    case SNMP_PDU_UNDO:                     /* Undo request */
    case SNMP_PDU_COMMIT:                   /* Commit request. */
    default:
        status = SNMP_GENERROR;
    }

    /* Returning success or error code. */
    return (status);

} /* ipv6NetToMediaEntry */

#endif /* (INCLUDE_IPV6_MIB_NTM == NU_TRUE) */
#endif /* (INCLUDE_SNMP == NU_TRUE) */
