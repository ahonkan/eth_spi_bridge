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
*        2465if.c                                    
*
*   DESCRIPTION
*
*        This file contains the implementation of Interface group of IPv6
*        MIB.
*
*   DATA STRUCTURES
*
*        None
*
*   FUNCTIONS
*
*        ipv6Interfaces
*        ipv6IfTableLastChange
*        Get_ipv6IfEntry
*        Set_ipv6IfEntry
*        ipv6IfEntry
*        Get_ipv6IfStatsEntry
*        ipv6IfStatsEntry
*
*   DEPENDENCIES
*
*        nu_net.h
*        snmp_api.h
*        mib.h
*        ip6_mib_s.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#include "networking/mib.h"
#include "networking/ip6_mib_s.h"

#if (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE)

STATIC UINT16 Get_ipv6IfEntry(snmp_object_t *, UINT8);
STATIC UINT16 Set_ipv6IfEntry(snmp_object_t *);
STATIC UINT16 Get_ipv6IfStatsEntry(snmp_object_t *, UINT8);

/************************************************************************
*
*   FUNCTION
*
*        ipv6Interfaces
*
*   DESCRIPTION
*
*        This function is used to handle SNMP request on 'ipv6Interfaces'.
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
*        SNMP_GENERROR          General error.
*        SNMP_NOSUCHNAME        Invalid request.
*
************************************************************************/
UINT16 ipv6Interfaces(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16                  status;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);

    /* If we have valid SNMP request. */
    if (MibSimple(obj, idlen) == NU_TRUE)
    {
        IP6_IF_MIB_GET_NUMBER(obj->Syntax.LngUns, status);
    }

    /* If we have invalid SNMP request. */
    else
    {
        status = SNMP_NOSUCHNAME;
    }

    /* Return success or error code. */
    return (status);

} /* ipv6Interfaces */

/************************************************************************
*
*   FUNCTION
*
*        ipv6IfTableLastChange
*
*   DESCRIPTION
*
*        This function is used to handle SNMP requests on
*        'ipv6IfTableLastChange'.
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
*        SNMP_NOSUCHNAME        When fail.
*
************************************************************************/
UINT16 ipv6IfTableLastChange(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16          status;

    /* Avoid compilation warning. */
    UNUSED_PARAMETER(param);

    /* If we have valid SNMP request. */
    if (MibSimple(obj, idlen) == NU_TRUE)
    {
        IP6_MIB_IF_TABLE_LAST_CHNG_GET(obj->Syntax.LngUns, status);
    }

    /* If we did not get valid SNMP request then return error code. */
    else
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* Return success or error code. */
    return (status);

} /* ipv6IfTableLastChange */

/************************************************************************
*
*   FUNCTION
*
*        Get_ipv6IfEntry
*
*   DESCRIPTION
*
*        This function is used to handle GET request on 'ipv6Table'.
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
STATIC UINT16 Get_ipv6IfEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32                  table_oid[] = {1, 3, 6, 1, 2, 1, 55, 1, 5, 1};

    /* Interface index. */
    UINT32                  if_index;

    /* Status for returning success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Get interface index. */
    if_index = obj->Id[IP6_IF_IF_INDEX_OFFSET];

    /* If we have GET request. */
    if (getflag)
    {
        /* Verify that the OID length in the object is equal to the
         * instance level OID for the received SNMP object. And that we
         * have a valid IfIndex.
         */
        if( (obj->IdLen != (IP6_IF_SUB_LEN + 1)) || (!(if_index)) )
        {
            status = SNMP_NOSUCHOBJECT;
        }
    }

    /* If we have GET-NEXT request. */
    else
    {
        /* Getting interface index next of next interface index. */
        IP6_IF_MIB_GET_NEXT_INDEX(if_index, status);
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        switch(obj->Id[IP6_IF_ATTR_OFFSET])
        {
        case 2:     /* ipv6IfDescr */

            /* Getting the value of 'ipv6IfDescr'. */
            IP6_IF_MIB_GET_DESCR(if_index, obj->Syntax.BufChr, status);

            /* If we successfully get the value of 'ipv6IfDescr' then set
             * the syntax length.
             */
            if (status == SNMP_NOERROR)
            {
                /* Updating the syntax length. */
                obj->SyntaxLen = strlen((CHAR *)(obj->Syntax.BufChr));
            }

            break;

        case 3:     /* ipv6IfLowerLayer */

            /* Getting the value 'ipv6IfLowerLayer'. */
            IP6_IF_MIB_GET_LOWER_LAYER(if_index, obj->Syntax.BufInt,
                                       obj->SyntaxLen, status);

            break;

        case 4:     /* ipv6IfEffectiveMtu */

            /* Getting the value of 'ipv6IfEffectiveMtu'. */
            IP6_IF_MIB_GET_EFFECTIVE_MTU(if_index, obj->Syntax.LngUns,
                                         status);

            break;

        case 5:     /* ipv6IfReasmMaxSize */

            /* Getting the value of 'ipv6IfReasmMaxSize'. */
            IP6_IF_MIB_GET_REASM_MAX_SIZE(if_index,
                                          obj->Syntax.LngUns, status);

            break;

        case 6:     /* ipv6IfIdentifier */

            /* Getting the value of 'ipv6IfIdentifier'. */
            IP6_IF_MIB_GET_IDENTIFIER(if_index, obj->Syntax.BufChr, status);
            
            if (status == SNMP_NOERROR)
            {
                obj->SyntaxLen = IP6_IF_IDENTIFIER_SYN_LEN;
            }

            break;

        case 7:     /* ipv6IfIdentifierLength */

            /* Getting the value of 'ipv6IfIdentifierLength'. */
            IP6_IF_MIB_GET_IDENTIFIER_LEN(if_index, obj->Syntax.LngUns,
                                          status);

            break;

        case 8:     /* ipv6IfPhysicalAddress */

            /* Getting the value of 'ipv6IfPhysicalAddress' */
            IP6_MIB_IF_PHY_ADDR_GET(if_index, obj->Syntax.BufChr,
                                    obj->SyntaxLen, status);

            break;

        case 9:     /* ipv6IfAdminStatus */

            /* Getting the value of 'ipv6IfAdminStatus'. */
            IP6_MIB_IF_ADMIN_STATUS_GET(if_index, obj->Syntax.LngUns,
                                        status);

            break;

        case 10:    /* ipv6IfOperStatus */

            /* Getting the value of 'ipv6IfOperStatus'. */
            IP6_MIB_IF_OPER_STATUS_GET(if_index, obj->Syntax.LngUns,
                                       status);

            break;

        case 11:    /* ipv6IfLastChange */

            /* Getting the value of 'ipv6IfLastChange' */
            IP6_MIB_IF_LAST_CHANGE_GET(if_index, obj->Syntax.LngUns,
                                       status);

            break;

        default:

            /* We have reached the end of the table and no attribute is
             * left. So return error code.
             */
            status = SNMP_NOSUCHNAME;
        }

        /* If the request was successful and the request was of GET-NEXT
         * then update the object OID.
         */
        if ( (status == SNMP_NOERROR) && (!getflag) )
        {
            /* Prepend the table's OID . */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update the value of ifIndex in the OID of the object. */
            obj->Id[IP6_IF_IF_INDEX_OFFSET] = if_index;

            /* Update the length of the OID. */
            obj->IdLen = IP6_IF_SUB_LEN + 1;
        }
    }

    /* Returning success or error code. */
    return (status);

} /* Get_ipv6IfEntry */

/************************************************************************
*
*   FUNCTION
*
*        Set_ipv6IfEntry
*
*   DESCRIPTION
*
*        This function is used to handle SET requests on 'ipv6IfTable'.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
*
*   OUTPUTS
*
*        SNMP_NOERROR           When successful.
*        SNMP_NOSUCHOBJECT      Interface index passed in SNMP request is
*                               invalid.
*        SNMP_ERROR             General error.
*        SNMP_WRONGLENGTH       Value to set has invalid size.
*        SNMP_WRONGVALUE        Value set is invalid.
*        SNMP_NOSUCHNAME        SNMP request is invalid.
*
************************************************************************/
STATIC UINT16 Set_ipv6IfEntry(snmp_object_t *obj)
{
    /* Status for returning success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Verify that the OID length in the object is equal to the instance
     * level OID for the received SNMP object. And that a valid IfIndex
     * exists.
     */
    if ( (obj->IdLen == (IP6_IF_SUB_LEN + 1)) &&
         (obj->Id[IP6_IF_IF_INDEX_OFFSET] > 0) )
    {
        switch(obj->Id[IP6_IF_ATTR_OFFSET])
        {
        case 2:     /* ipv6IfDescr */

            /* Setting the value to 'ipv6IfDescr'. */
            IP6_IF_MIB_SET_DESCR(obj->Id[IP6_IF_IF_INDEX_OFFSET],
                                 obj->Syntax.BufChr,
                                 status);

            break;

        case 6:     /* ipv6IfIdentifier */

            if (obj->SyntaxLen == IP6_IF_IDENTIFIER_SYN_LEN)
            {
                /* Setting value to 'ipv6IfIdentifier'. */
                IP6_IF_MIB_SET_IDENTIFIER(obj->Id[IP6_IF_IF_INDEX_OFFSET],
                                          obj->Syntax.BufChr, status);
            }
            else
                status = SNMP_WRONGLENGTH;

            break;

        case 7:     /* ipv6IfIdentifierLength */

            /* Setting value to 'ipv6IfIdentifierLength'. */
            IP6_IF_MIB_SET_IDENTIFIER_LEN(obj->Id[IP6_IF_IF_INDEX_OFFSET],
                                          obj->Syntax.LngUns, status);

            break;

        case 9:     /* ipv6IfAdminStatus */

            /* Setting value to ipv6IfAdminStatus. */
            IP6_MIB_IF_ADMIN_STATUS_SET(obj->Id[IP6_IF_IF_INDEX_OFFSET],
                                        obj->Syntax.LngInt, status);

            break;

        default:
            /* We have reached the end of the table. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* If we have invalid OID then return error code. */
    else
    {
        /* Returning error code. */
        status = SNMP_NOSUCHOBJECT;
    }

    /* Return success or error code. */
    return (status);

} /* Set_ipv6IfEntry */

/************************************************************************
*
* FUNCTION
*
*        ipv6IfEntry
*
* DESCRIPTION
*
*        This function is used to handle SNMP requests on 'ipv6IfTable'.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  Length of SNMP request.
*        *param                 Additional parameters (Not used).
*
* OUTPUTS
*
*        SNMP_NOERROR           When successful.
*        SNMP_NOSUCHOBJECT      Interface index passed in SNMP request 
*                               is invalid.
*        SNMP_ERROR             General error.
*        SNMP_WRONGLENGTH       Value to set has invalid size.
*        SNMP_WRONGVALUE        Value set is invalid.
*        SNMP_NOSUCHNAME        SNMP request is invalid.
*
************************************************************************/
UINT16 ipv6IfEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
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
        status = Get_ipv6IfEntry(obj, getflag);

        break;

    case SNMP_PDU_UNDO:                     /* Undo request. */
    case SNMP_PDU_SET:                      /* Set request. */

        /* Processing of set operations. */
        status = Set_ipv6IfEntry(obj);
        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = IPV6_GET_BULK(obj, Get_ipv6IfEntry);
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

} /* ipv6IfEntry */

/************************************************************************
*
*   FUNCTION
*
*        Get_ipv6IfStatsEntry
*
*   DESCRIPTION
*
*        This function is used to handle GET requests on
*        'ipv6IfStatsEntry'.
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
STATIC UINT16 Get_ipv6IfStatsEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32                  table_oid[] = {1, 3, 6, 1, 2, 1, 55, 1, 6, 1};

    /* Interface index. */
    UINT32                  if_index;

    /* Status for returning success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Getting interface index. */
    if_index = obj->Id[IP6_IF_STAT_IF_INDEX_OFFSET];

    /* If we have GET request. */
    if (getflag)
    {
        /* Verify that the OID length in the object is equal to the
         * instance level OID for the received SNMP object. And that we
         * have a valid IfIndex.
         */
        if ( (obj->IdLen != (IP6_IF_STAT_SUB_LEN + 1)) ||
             (!(if_index)) )
        {
            status = SNMP_NOSUCHOBJECT;
        }
    }

    /* If we have GET-NEXT request. */
    else
    {
        /* Getting interface index next of next interface index. */
        IP6_IF_MIB_GET_NEXT_INDEX(if_index, status);
    }

    /* If no error till now then proceed. Otherwise return error code. */
    if (status == SNMP_NOERROR)
    {
        /* Get the value of counter specified by 
         * 'obj->Id[IP6_IF_STAT_ATTR_OFFSET]'.
         */
        IP6_IF_MIB_GET_STATUS(if_index,
                              (INT)(obj->Id[IP6_IF_STAT_ATTR_OFFSET]), 
                              obj->Syntax.LngUns, status);

        /* If the request was successful and the request was of GET-NEXT
         * then update the object OID.
         */
        if ((status == SNMP_NOERROR) && (!getflag))
        {
            /* Prepend the table's OID . */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update the value of ifIndex in the OID of the object. */
            obj->Id[IP6_IF_STAT_IF_INDEX_OFFSET] = if_index;

            /* Update the length of the OID. */
            obj->IdLen = (IP6_IF_STAT_SUB_LEN + 1);
        }
    }

    /* Returning success or error code. */
    return (status);

} /* Get_ipv6IfStatsEntry */

/************************************************************************
*
*   FUNCTION
*
*        ipv6IfStatsEntry
*
*   DESCRIPTION
*
*        This function is used to handle SNMP requests on
*        'ipv6IfStatsTable'.
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
UINT16 ipv6IfStatsEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
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
        status = Get_ipv6IfStatsEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = IPV6_GET_BULK(obj, Get_ipv6IfStatsEntry);
        break;


    case SNMP_PDU_UNDO:                     /* Undo request. */
    case SNMP_PDU_SET:                      /* Set request. */
    case SNMP_PDU_COMMIT:                   /* Commit request. */
    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* ipv6IfStatsEntry */

#endif /* (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE) */
#endif /* (INCLUDE_SNMP == NU_TRUE) */
