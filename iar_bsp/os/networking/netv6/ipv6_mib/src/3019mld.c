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
*        3019mld.c                                   
*
*   DESCRIPTION
*
*        This file contains the function that responsible of handling
*        SNMP request on MLD Group MIBs.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        Get_mldInterfaceEntry
*        mldInterfaceEntry
*        Get_mldCacheEntry
*        mldCacheEntry
*
*   DEPENDENCIES
*
*        nu_net.h
*        ip6_mib.h
*        snmp_api.h
*        ip6_mib_s.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)

#include "networking/ip6_mib.h"
#include "networking/snmp_api.h"
#include "networking/ip6_mib_s.h"

#if ((INCLUDE_IPV6_MLD_MIB == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_TRUE))

STATIC UINT16 Get_mldInterfaceEntry(snmp_object_t *, UINT8);
STATIC UINT16 Get_mldCacheEntry(snmp_object_t *, UINT8);

/************************************************************************
*
*   FUNCTION
*
*        Get_mldInterfaceEntry
*
*   DESCRIPTION
*
*        This function is used to handle the SNMP GET requests on
*        mldInterfaceTable.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
*        getflag                Flag to distinguish between get
*                               and get next request.
*
*   OUTPUTS
*
*        SNMP_NOERROR           When successful.
*        SNMP_NOSUCHOBJECT      When there does not exist the required 
*                               MLD enabled interface device.
*        SNMP_ERROR             General error.
*        SNMP_NOSUCHNAME        Invalid request.
*
************************************************************************/
STATIC UINT16 Get_mldInterfaceEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32              table_oid[] = {1, 3, 6, 1, 2, 1, 91, 1, 1, 1};

    /* Interface index. */
    UINT32              if_index;

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Getting value of interface index. */
    if_index = obj->Id[IP6_MLD_IF_IF_INDEX_OFFSET];

    /* If it is GET request. */
    if (getflag)
    {
        /* If we have invalid OID then return error code. */
        if (obj->IdLen != (IP6_MLD_IF_SUB_LEN + 1))
            status = SNMP_NOSUCHOBJECT;
    }

    /* If we have GET-NEXT request. */
    else
    {
        IP6_MLD_MIB_GET_NEXT_INDEX(if_index, status);
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        switch(obj->Id[IP6_MLD_IF_ATTR_OFFSET])
        {
        case 3:     /* mldInterfaceStatus */

            /* Get the value of mldInterfaceStatus. */
            IP6_MLD_MIB_GET_IF_STATUS(if_index, obj->Syntax.LngUns,
                                      status);

            break;

        case 5:     /* mldInterfaceQuerier */

            /* Get the value of mldInterfaceQuerier. */
            IP6_MLD_MIB_GET_IF_QRIER_ADDR(if_index, obj->Syntax.BufChr,
                                          status);

            if (status == SNMP_NOERROR)
                obj->SyntaxLen = IP6_ADDR_LEN;

            break;

        default:
            /* We have reached at end of the table. */
            status = SNMP_NOSUCHNAME;
        }

        /* If the operation was successful and the operation was GET-NEXT
         * then update OID.
         */
        if ( (status == SNMP_NOERROR) && (!getflag) )
        {
            /* Update the table OID. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update the interface index. */
            obj->Id[IP6_MLD_IF_IF_INDEX_OFFSET] = if_index;

            /* Update the OID length. */
            obj->IdLen = IP6_MLD_IF_SUB_LEN + 1;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_mldInterfaceEntry */

/************************************************************************
*
*   FUNCTION
*
*        mldInterfaceEntry
*
*   DESCRIPTION
*
*        This function is used to handle SNMP requests on
*        mldInterfaceTable.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  Length of snmp request.
*        *param                 Additional parameters (Not used).
*
*   OUTPUTS
*
*        SNMP_NOERROR           When successful.
*        SNMP_NOSUCHOBJECT      When there does not exist the required 
*                               MLD enabled interface device.
*        SNMP_ERROR             General error.
*        SNMP_NOSUCHNAME        Invalid request.
*        SNMP_GENERROR          Invalid request type.
*
************************************************************************/
UINT16 mldInterfaceEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
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
        status = Get_mldInterfaceEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = IPV6_GET_BULK(obj, Get_mldInterfaceEntry);
        break;

    case SNMP_PDU_UNDO:                     /* Undo request. */
    case SNMP_PDU_SET:                      /* Set request. */
    case SNMP_PDU_COMMIT:                   /* Commit request. */
    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* mldInterfaceEntry */

/************************************************************************
*
*   FUNCTION
*
*        Get_mldCacheEntry
*
*   DESCRIPTION
*
*        This function is used to handle the SNMP GET requests on
*        mldCacheTable.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
*        getflag                Flag to distinguish between get
*                               and get next request.
*
*   OUTPUTS
*
*        SNMP_NOERROR           When successful.
*        SNMP_NOSUCHOBJECT      When there does not exist the required 
*                               MLD cache entry.
*        SNMP_ERROR             General error.
*        SNMP_NOSUCHNAME        Invalid request.
*
************************************************************************/
STATIC UINT16 Get_mldCacheEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32              table_oid[] = {1, 3, 6, 1, 2, 1, 91, 1, 2, 1};

    /* Interface index. */
    UINT32              if_index;

    /* Address of MLD cache entry. */
    UINT8               addr[IP6_ADDR_LEN];

    /* Variable to use in for-loop. */
    INT                 i;

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Getting address of the cache entry. */
    for (i = 0; i < IP6_ADDR_LEN; i++)
    {
        addr[i] = (UINT8)(obj->Id[IP6_MLD_CACHE_ADDR_OFFSET + i]);
    }

    /* Getting interface index. */
    if_index = obj->Id[IP6_MLD_CACHE_IF_INDEX_OFFSET];

    /* If we have GET request. */
    if (getflag)
    {
        /* If we have invalid OID then return error code. */
        if (obj->IdLen != IP6_MLD_CACHE_SUB_LEN + 1)
        {
            status = SNMP_NOSUCHOBJECT;
        }
    }

    /* If we have GET-NEXT request then get the indexes of the next entry.
     */
    else
    {
        IP6_MLD_MIB_GET_NEXT_CACHE(addr, if_index, status);
    }

    if (status == SNMP_NOERROR)
    {
        switch(obj->Id[IP6_MLD_CACHE_ATTR_OFFSET])
        {
        case 3:     /* mldCacheSelf */

            /* Get the value of mldCacheSelf. */
            IP6_MLD_MIB_GET_CACHE_SELF(addr, if_index, obj->Syntax.LngUns,
                                       status);

            break;

        case 7:     /* mldCacheStatus */

            /* Get the value of mldCacheStatus. */
            IP6_MLD_MIB_GET_CACHE_STATUS(addr, if_index,
                                         obj->Syntax.LngUns, status);

            break;

        default:
            /* We have reached at end of the table. */
            status = SNMP_NOSUCHNAME;

            break;
        }

        if ( (status == SNMP_NOERROR) && (!getflag) )
        {
            /* Update OID. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Setting address of the cache entry. */
            for (i = 0; i < IP6_ADDR_LEN; i++)
            {
                obj->Id[IP6_MLD_CACHE_ADDR_OFFSET + i] =
                                    (((UINT32)addr[i]) & 0xff);
            }

            /* Setting interface index. */
            obj->Id[IP6_MLD_CACHE_IF_INDEX_OFFSET] = if_index;

            /* Update the OID Length. */
            obj->IdLen = (IP6_MLD_CACHE_SUB_LEN + 1);
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_mldCacheEntry */

/************************************************************************
*
*   FUNCTION
*
*        mldCacheEntry
*
*   DESCRIPTION
*
*        This function is used to SNMP requests on mldCacheTable.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  Length of snmp request.
*        *param                 Additional parameters (Not used).
*
*   OUTPUTS
*
*        SNMP_NOERROR           When successful.
*        SNMP_NOSUCHOBJECT      When there does not exist the required 
*                               MLD cache entry.
*        SNMP_ERROR             General error.
*        SNMP_NOSUCHNAME        Invalid request.
*        SNMP_GENERROR          Invalid request type.
*
************************************************************************/
UINT16 mldCacheEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
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
    case SNMP_PDU_GET:      /* Get request. */

        /* Set getflag so that it can represent get-request. */
        getflag++;

        /* Fall through the next case for fulfilling the get-request. */

    case SNMP_PDU_NEXT:     /* Get next request. */

        /* Processing get operations. */
        status = Get_mldCacheEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = IPV6_GET_BULK(obj, Get_mldCacheEntry);
        break;


    case SNMP_PDU_UNDO:     /* Undo request. */
    case SNMP_PDU_SET:      /* Set request. */
    case SNMP_PDU_COMMIT:   /* Commit request. */
    default:                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* mldCacheEntry */

#endif /* ((INCLUDE_IPV6_MLD_MIB == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_TRUE)) */
#endif /* (INCLUDE_SNMP == NU_TRUE) */
