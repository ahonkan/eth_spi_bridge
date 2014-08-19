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
*        2452tcp.c                                   
*
*   DESCRIPTION
*
*        This file contains the functions that handles SNMP requests on
*        ipv6TcpConnTable.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        Get_ipv6TcpConnEntry
*        Set_ipv6TcpConnEntry
*        ipv6TcpConnEntry
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

#if (INCLUDE_IPV6_TCP_MIB == NU_TRUE)

STATIC UINT16 Get_ipv6TcpConnEntry(snmp_object_t *, UINT8);
STATIC UINT16 Set_ipv6TcpConnEntry(snmp_object_t *);

/************************************************************************
*
*   FUNCTION
*
*        Get_ipv6TcpConnEntry
*
*   DESCRIPTION
*
*        This function is used to handle GET and GET-NEXT requests on
*        ipv6TcpConnTable.
*
*   INPUTS
*
*         *obj                  SNMP object containing request.
*         getflag               Flag to distinguish between get
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
STATIC UINT16 Get_ipv6TcpConnEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32                  table_oid[] = {1, 3, 6, 1, 2, 1, 6, 16, 1};

    /* Local address. */
    UINT8                   local_addr[IP6_ADDR_LEN];

    /* Remote address. */
    UINT8                   remote_addr[IP6_ADDR_LEN];

    /* Local port. */
    UINT16                  local_port;

    /* Remote port. */
    UINT16                  remote_port;

    /* Local interface index. */
    UINT32                  if_index;

    /* Variable to use in for loop. */
    INT                     i;

    /* Status for returning success or error code. */
    UINT16                  status;

    /* Getting local address. */
    for (i = 0; i < IP6_ADDR_LEN; i++)
    {
        local_addr[i] =
            (UINT8)(obj->Id[IP6_TCP_CONN_LOCAL_ADDR_OFFSET+ i]);
    }

    /* Getting value of local port. */
    local_port = (UINT16)(obj->Id[IP6_TCP_CONN_LOCAL_PORT_OFFSET]);

    /* Getting remote address. */
    for (i = 0; i < IP6_ADDR_LEN; i++)
    {
        remote_addr[i] =
            (UINT8)(obj->Id[IP6_TCP_CONN_REMOT_ADDR_OFFSET + i]);        
    }

    /* Getting remote port. */
    remote_port = (UINT16)(obj->Id[IP6_TCP_CONN_REMOT_PORT_OFFSET]);

    /* Getting interface index. */
    if_index = obj->Id[IP6_TCP_CONN_IF_INDEX_OFFSET];

    /* If we are handling GET request. */
    if (getflag)
    {
        /* We did not get the valid OID then return error code. */
        if ( (obj->IdLen != (IP6_TCP_CONN_SUB_LEN + 1)) ||
             (obj->Id[IP6_TCP_CONN_ATTR_OFFSET] != 6) )
        {
            status = SNMP_NOSUCHNAME;
        }
        else
        {
            IP6_MIB_TCP_GET_STATE(local_addr, local_port, remote_addr,
                                  remote_port, if_index,
                                  (obj->Syntax.LngUns), status);
        }
    }

    /* If we are handling GET-NEXT request. */
    else
    {
        IP6_MIB_TCP_GET_NEXT_STATE(local_addr, local_port,remote_addr,
                                   remote_port, if_index,
                                   (obj->Syntax.LngUns), status);

        /* If the operation was successful then update the OID. */
        if (status == SNMP_NOERROR)
        {
            /* Updating the table OID. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Setting attribute. */
            obj->Id[IP6_TCP_CONN_ATTR_OFFSET] = 6;

            /* Setting local address. */
            for (i = 0; i < IP6_ADDR_LEN; i++)
            {
                obj->Id[IP6_TCP_CONN_LOCAL_ADDR_OFFSET + i] =
                    (((UINT32)local_addr[i]) & 0xff);
            }

            /* Setting value of local port. */
            obj->Id[IP6_TCP_CONN_LOCAL_PORT_OFFSET] =
                (((UINT32)local_port) & 0xffff);

            /* Setting remote address. */
            for (i = 0; i < IP6_ADDR_LEN; i++)
            {
                obj->Id[IP6_TCP_CONN_REMOT_ADDR_OFFSET + i] =
                    (((UINT32)remote_addr[i]) & 0xff);
            }

            /* Setting remote port. */
            obj->Id[IP6_TCP_CONN_REMOT_PORT_OFFSET] = (UINT32)remote_port;

            /* Setting interface index. */
            obj->Id[IP6_TCP_CONN_IF_INDEX_OFFSET] = if_index;

            /* Update the OID length. */
            obj->IdLen = (IP6_TCP_CONN_SUB_LEN + 1);
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_ipv6TcpConnEntry */

/************************************************************************
*
*   FUNCTION
*
*        Set_ipv6TcpConnEntry
*
*   DESCRIPTION
*
*        This function is used to handle the SET requests on
*        ipv6TcpConnTable.
*
*   INPUTS
*
*        *obj                   SNMP object containing request.
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
STATIC UINT16 Set_ipv6TcpConnEntry(snmp_object_t *obj)
{
    /* Local address. */
    UINT8                   local_addr[IP6_ADDR_LEN];

    /* Remote address. */
    UINT8                   remote_addr[IP6_ADDR_LEN];

    /* Local port. */
    UINT16                  local_port;

    /* Remote port. */
    UINT16                  remote_port;

    /* Local interface index. */
    UINT32                  if_index;

    /* Variable to use in for loop. */
    INT                     i;

    /* Status for returning success or error code. */
    UINT16                  status;

    /* If we have valid OID then proceeds. */
    if ( (obj->IdLen == (IP6_TCP_CONN_SUB_LEN + 1)) ||
         (obj->Id[IP6_TCP_CONN_ATTR_OFFSET] == 6) )
    {
        /* Getting local address. */
        for (i = 0; i < IP6_ADDR_LEN; i++)
        {
            local_addr[i] =
                (UINT8)(obj->Id[IP6_TCP_CONN_LOCAL_ADDR_OFFSET+ i]);
        }

        /* Getting value of local port. */
        local_port = (UINT16)(obj->Id[IP6_TCP_CONN_LOCAL_PORT_OFFSET]);

        /* Getting remote address. */
        for (i = 0; i < IP6_ADDR_LEN; i++)
        {
            remote_addr[i] =
                (UINT8)(obj->Id[IP6_TCP_CONN_REMOT_ADDR_OFFSET + i]);        
        }

        /* Getting remote port. */
        remote_port = (UINT16)(obj->Id[IP6_TCP_CONN_REMOT_PORT_OFFSET]);

        /* Getting interface index. */
        if_index = obj->Id[IP6_TCP_CONN_IF_INDEX_OFFSET];

        IP6_MIB_TCP_SET_STATE(local_addr, local_port, remote_addr,
                              remote_port, if_index, obj->Syntax.LngUns,
                              status);
    }

    /* If we don't have the valid OID then return error code. */
    else
    {
        status = SNMP_NOSUCHNAME;
    }

    /* Return success or error code. */
    return (status);

} /* Set_ipv6TcpConnEntry */

/************************************************************************
*
*   FUNCTION
*
*        ipv6TcpConnEntry
*
*   DESCRIPTION
*
*        This function is used to handle all the SNMP request on
*        ipv6TcpConnTable.
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
*        SNMP_READONLY          If object is read-only.
*        SNMP_BADVALUE          If invalid value.
*        SNMP_NOSUCHNAME        Object does not exist.
*        SNMP_GENERROR          Invalid request.
*        SNMP_NOSUCHOBJECT      Instance does not exist.
*
************************************************************************/
UINT16 ipv6TcpConnEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
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
        status = Get_ipv6TcpConnEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = IPV6_GET_BULK(obj, Get_ipv6TcpConnEntry);
        break;

    case SNMP_PDU_UNDO:                     /* Undo request. */
    case SNMP_PDU_SET:                      /* Set request. */

        /* Process the SET or UNDO operation. */
        status = Set_ipv6TcpConnEntry(obj);
        break;

    case SNMP_PDU_COMMIT:                   /* Commit request. */

        /* We always have the commit set. */
        status = SNMP_NOERROR;
        break;

    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* ipv6TcpConnEntry */

#endif /* (INCLUDE_IPV6_TCP_MIB == NU_TRUE) */
#endif /* (INCLUDE_SNMP == NU_TRUE) */
