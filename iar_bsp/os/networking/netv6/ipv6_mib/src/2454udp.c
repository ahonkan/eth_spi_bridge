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
*        2454udp.c                                   
*
*   DESCRIPTION
*
*        This file contains the functions that handles SNMP requests on
*        ipv6UdpTable
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        Get_ipv6UdpEntry
*        ipv6UdpEntry
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

#if (INCLUDE_IPV6_UDP_MIB == NU_TRUE)

STATIC UINT16 Get_ipv6UdpEntry(snmp_object_t *, UINT8);

/************************************************************************
*
*   FUNCTION
*
*        Get_ipv6UdpEntry
*
*   DESCRIPTION
*
*        This function is used to handle GET requests on ipv6UdpTable.
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
STATIC UINT16 Get_ipv6UdpEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32                  table_oid[] = {1, 3, 6, 1, 2, 1, 7, 6, 1};

    /* Local address. */
    UINT8                   local_addr[IP6_ADDR_LEN];

    /* Interface index. */
    UINT32                  if_index;

    /* Variable to use in for loop. */
    INT                     i;

    /* Local port. */
    UINT16                  local_port;

    /* Status to return success or error code. */
    UINT16                  status;

    /* Getting local address. */
    for (i = 0; i < IP6_ADDR_LEN; i++)
    {
        local_addr[i] = 
            (UINT8)(obj->Id[IP6_UDP_LOCAL_ADDR_OFFSET + i]);
    }

    /* Getting value of local port. */
    local_port = (UINT16)obj->Id[IP6_UDP_LOCAL_PORT_OFFSET];

    /* Getting interface index. */
    if_index = obj->Id[IP6_UDP_IF_INDEX_OFFSET];

    /* If this is GET request. */
    if (getflag)
    {
        if ( (obj->IdLen != (IP6_UDP_SUB_LEN + 1)) ||
             (obj->Id[IP6_UDP_ATTR_OFFSET] != 3) )
        {
             status = SNMP_NOSUCHNAME;
        }
        else
        {
            IP6_MIB_UDP_SOCK_INFO_GET(local_addr, local_port, if_index,
                                      status);
        }
    }

    /* If this is GET-NEXT request. */
    else
    {
        /* Process the GET-NEXT operation. */
        IP6_MIB_UDP_SOCK_INFO_GET_NEXT(local_addr, local_port, 
                                       if_index, status);

        /* If there was no error in processing GET-NEXT operation then
         * update the OID.
         */
        if (status == SNMP_NOERROR)
        {
            /* Setting table OID. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Setting attribute. */
            obj->Id[IP6_UDP_ATTR_OFFSET] = 3;

            /* Setting local address. */
            for (i = 0; i < IP6_ADDR_LEN; i++)
            {
                obj->Id[IP6_UDP_LOCAL_ADDR_OFFSET + i] =
                    (((UINT32)local_addr[i]) & 0xff);
            }

            /* Setting value of local port. */
            obj->Id[IP6_UDP_LOCAL_PORT_OFFSET] =
                (((UINT32)local_port) & 0xffff);

            /* Setting interface index. */
            obj->Id[IP6_UDP_IF_INDEX_OFFSET] = if_index;

            /* Set OID length. */
            obj->IdLen = IP6_UDP_SUB_LEN + 1;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_ipv6UdpEntry */

/************************************************************************
*
*   FUNCTION
*
*        ipv6UdpEntry
*
*   DESCRIPTION
*
*        This function is used to handle all the SNMP requests on
*        ipv6UdpTable.
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
UINT16 ipv6UdpEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
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
        status = Get_ipv6UdpEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = IPV6_GET_BULK(obj, Get_ipv6UdpEntry);
        break;

    case SNMP_PDU_UNDO:                     /* Undo request. */
    case SNMP_PDU_SET:                      /* Set request. */
    case SNMP_PDU_COMMIT:                   /* Commit request. */
    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* ipv6UdpEntry */

#endif /* (INCLUDE_IPV6_UDP_MIB == NU_TRUE) */
#endif /* (INCLUDE_SNMP == NU_TRUE) */
