/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*        ip_tun_mib.c
*
* COMPONENT
*
*        IP - Tunnel MIBs.
*
* DESCRIPTION
*
*        This file contains the implementation of IP Tunnel MIBs.
*
* DATA STRUCTURES
*
*        IP_Tun_Commit_Left
*        TUN_GET_FUNCTION
*
* FUNCTIONS
*
*        Get_TunnelIfEntry
*        Set_TunnelIfEntry
*        TunnelIfEntry
*        Get_TunnelConfigEntry
*        Set_TunnelConfigEntry
*        Create_TunnelConfigEntry
*        Undo_TunnelConfigEntry
*        TunnelConfigEntry
*        Tunnel_Get_Bulk
*
* DEPENDENCIES
*
*        nu_net.h
*        snmp.h
*        ip_tun_mib.h
*
************************************************************************/
#include "networking/nu_net.h"

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

#include "networking/snmp.h"
#include "networking/ip_tun_mib.h"

UINT32 IP_Tun_Commit_Left;

STATIC UINT16 Get_TunnelIfEntry(snmp_object_t *obj, UINT8 getflag);
STATIC UINT16 Set_TunnelIfEntry(snmp_object_t *obj);
STATIC UINT16 Get_TunnelConfigEntry(snmp_object_t *obj, UINT8 getflag);
STATIC UINT16 Set_TunnelConfigEntry(snmp_object_t *obj);
STATIC UINT16 Create_TunnelConfigEntry(snmp_object_t *obj);
STATIC UINT16 Undo_TunnelConfigEntry(snmp_object_t *obj);

/* The following is a definition of a GET function parameter passed to
   Tunnel_Get_Bulk function to be used for actually getting an instance.
*/
typedef UINT16 (*TUN_GET_FUNCTION)(snmp_object_t *obj, UINT8 getflag);

STATIC UINT16 Tunnel_Get_Bulk(snmp_object_t *obj, TUN_GET_FUNCTION
                                                            get_function);

/*************************************************************************
*
* FUNCTION
*
*        Get_TunnelIfEntry
*
* DESCRIPTION
*
*        This function is used to get values of Tunnel IfTable entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        getflag                Flag to distinguish between get
*                               and get next request.
*
* OUTPUTS
*
*       SNMP_NOSUCHNAME         The object does not exist.
*       SNMP_ERROR              An error occur while processing the
*                               request.
*       SNMP_NOERROR            The action was successfully completed.
*
*************************************************************************/
STATIC UINT16 Get_TunnelIfEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Interface index of tunnel. */
    UINT32                  if_index = 0;

    /* Flag to represent the existence of tunnel. */
    UINT32                  tunnel_exists = NU_FALSE;

    /* Object Identifier sub-length. Table entry OID is of length 12
       having ifIndex as index that results in total sub-length of 13. */
    UINT32                  sub_len = 13;

    /* Object identifier of table entry. */
    UINT32                  table_oid[] = {1, 3, 6, 1, 2, 1, 10, 131, 1,
                                           1, 1, 1};

    /* Length of bytes used in opt_val. */
    INT                     length = IP_ADDR_LEN;

    /* Supported options' names. */
    INT                     opt_name[6] = {IP_LOCAL_ADDRESS,
                                           IP_REMOTE_ADDRESS,
                                           IP_ENCAPS_METHOD,
                                           IP_HOP_LIMIT,
                                           IP_SECURITY,
                                           IP_TOS};

    /* Status for returning success or error code. */
    UINT16          status;

    /* Buffer for value to be retrieved. */
    UINT8                   opt_val[IP_ADDR_LEN];

    /* If this is a get request, just check whether there exist a tunnel
       with ifIndex passed in. */
    if(getflag)
    {
        /* Verify that the OID length in the object is equal to the
           instance level OID for the received SNMP object. And that we
           have a valid IfIndex. */
        if((obj->IdLen == (sub_len + 1)) && (obj->Id[sub_len] > 0))
        {
            /* Getting ifIndex specified by SNMP object passed in. */
            if_index = (obj->Id[sub_len] - 1);

            /* Validate the existence of tunnel with specified ifIndex. */
            status = (UINT16)IP_Get_Tunnel_Opt(if_index, IP_IS_TUNNEL,
                                               &tunnel_exists, &length);

            /* If IP_Get_Tunnel_Opt operation failed then return error
               code. */
            if(status != NU_SUCCESS)
                status = SNMP_ERROR;
        }

        /* If we have invalid ifIndex or invalid ID length then return
           error code. */
        else
        {
            status = SNMP_NOSUCHNAME;
        }
    }

    /* This is a get-next request. */
    else
    {
        /* Getting the ifIndex of next Tunnel. */
        status = (UINT16)IP_Get_Tunnel_Opt(obj->Id[sub_len],
                                           IP_NEXT_TUNNEL, &if_index,
                                           &length);

        /* If we did not get the ifIndex or there is no tunnel next then
           return error code. */
        if((status != NU_SUCCESS) || (if_index < obj->Id[sub_len]))
        {
            obj->Id[sub_len - 1]++;

            /* Returning error code. */
            status = SNMP_NOSUCHNAME;
        }

        /* If the attribute is 0, make this attribute 1 because this is
           the next valid attribute. */
        if(obj->Id[sub_len - 1] == 0)
        {
            obj->Id[sub_len - 1] = 1;
        }
    }

    /* If we have successfully got the 'ifIndex' of the tunnel. */
    if(status == SNMP_NOERROR)
    {
        /* If we have valid attribute whose value is to be retrieved. */
        if((obj->Id[sub_len - 1] - 1) < 6)
        {
            /* Getting value to be retrieved in opt_val. */
            status = (UINT16)IP_Get_Tunnel_Opt(if_index,
                                     opt_name[(obj->Id[sub_len - 1] - 1)],
                                               opt_val, &length);

            /* If value is retrieved successfully in opt_val. */
            if(status == NU_SUCCESS)
            {
                switch(obj->Id[sub_len - 1])
                {
                case 1:     /* tunnelIfLocalAddress */
                case 2:     /* tunnelIfRemoteAddress */

                    /* Copying address value from the opt_val into SNMP
                       response object. */
                    NU_BLOCK_COPY(obj->Syntax.BufChr, opt_val,
                                  IP_ADDR_LEN);

                    /* Setting syntax length. */
                    obj->SyntaxLen = IP_ADDR_LEN;

                    break;

                case 3:     /* tunnelIfEncapsMethod  */
                case 5:     /* tunnelIfSecurity */

                    /* Getting value from opt_val into SNMP response
                       object. */
                    obj->Syntax.LngUns = (*((UINT8 *)opt_val));

                    break;

                case 4:     /* tunnelIfHopLimit */

                    /* Getting value from opt_val into SNMP response
                       object. */
                    obj->Syntax.LngUns = (*((UINT32 *)opt_val));

                    break;
                case 6:     /* tunnelIfTOS  */

                    /* Getting value from opt_val into SNMP response
                       object. */
                    obj->Syntax.LngInt = (*((INT8 *)opt_val));

                    break;
                }

                /* Prepend the table's OID. */
                NU_BLOCK_COPY(obj->Id, table_oid,
                              (sub_len - 1) * (sizeof(UINT32)));

                /* Update the value of ifIndex in the OID of the object.
                */
                obj->Id[sub_len] = if_index + 1;

                /* Update the length of the OID. */
                obj->IdLen = sub_len + 1;
            }

            /* If IP_Get_Tunnel_Opt failed then return error code. */
            else
            {
                /* Returning error code. */
                status = SNMP_ERROR;
            }
        }

        /* If we have invalid attribute then return error code. */
        else
        {
            /* Returning error code. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* Returning success or error code. */
    return (status);

} /* Get_TunnelIfEntry */

/*************************************************************************
*
* FUNCTION
*
*        Set_TunnelIfEntry
*
* DESCRIPTION
*
*        This function is used to set values of Tunnel IfTable entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_BADVALUE          The value to set is invalid.
*        SNMP_ERROR             An error occur while processing the
*                               request.
*        SNMP_NOERROR           The action was successfully completed.
*
*************************************************************************/
STATIC UINT16 Set_TunnelIfEntry(snmp_object_t *obj)
{
    /* Interface index of tunnel. */
    UINT32                  if_index;

    /* Flag to represent the existence of tunnel. */
    UINT32                  tunnel_exists = NU_FALSE;

    /* Object Identifier sub-length. Table entry OID is of length 12
       having ifIndex as index this results in sub-length of 13. */
    UINT32                  sub_len = 13;

    /* Length of bytes used in opt_val. */
    INT                     length;

    /* Status for returning success or error code. */
    UINT16                  status;

    INT8                    ip_tos_val;

    /* Verify that the OID length in the object is equal to the
       instance level OID for the received SNMP object. And that we
       have a valid IfIndex. */
    if((obj->IdLen == (sub_len + 1)) && (obj->Id[sub_len] > 0))
    {
        /* Getting ifIndex specified by SNMP object passed in. */
        if_index = (obj->Id[sub_len] - 1);

        /* Validate the existence of tunnel with specified ifIndex. */
        status = (UINT16)IP_Get_Tunnel_Opt(if_index, IP_IS_TUNNEL,
                                           &tunnel_exists, &length);

        /* If IP_Get_Tunnel_Opt operation failed then return error
           code. */
        if(status != NU_SUCCESS)
            status = SNMP_ERROR;

        /* If tunnel exist then proceed to fulfill the set request. */
        else
        {
            switch(obj->Id[sub_len - 1])
            {
            case 4:         /* tunnelIfHopLimit */

                /* If we have valid value of tunnelIfHopLimit then set
                   the value otherwise return error code. */
                if(obj->Syntax.LngUns <= 255)
                {
                    /* Setting value of tunnelIfHopLimit. */
                    status = (UINT16)IP_Set_Tunnel_Opt(if_index,
                                                       IP_HOP_LIMIT,
                                                    &(obj->Syntax.LngUns),
                                                       sizeof(UINT32));

                    /* If tunnelIfHopLimit value was not successfully
                       updated then return error code. */
                    if(status != NU_SUCCESS)
                    {
                        /* Returning error code. */
                        status = SNMP_ERROR;
                    }
                }

                /* Return error code if we did not have valid value. */
                else
                {
                    status = SNMP_WRONGVALUE;
                }

                break;

            case 6:         /* tunnelIfTOS */

                /* If we have valid value of tunnelIfTOS then set
                   the value otherwise return error code. */
                if((obj->Syntax.LngInt >= (-2)) &&
                   (obj->Syntax.LngInt <= 63))
                {
                    /* Getting value to set into a local variable. */
                    ip_tos_val = (INT8)(obj->Syntax.LngInt);

                    /* Setting value to tunnelIfTOS. */
                    status = (UINT16)IP_Set_Tunnel_Opt(if_index,
                                                       IP_HOP_LIMIT,
                                                       &ip_tos_val,
                                                       sizeof(INT8));

                    /* If tunnelIfTOS value was not successfully
                       updated then return error code. */
                    if(status != NU_SUCCESS)
                        status = SNMP_ERROR;
                }

                /* Return error code if we did not have valid value. */
                else
                {
                    status = SNMP_WRONGVALUE;
                }

                break;

            default:        /* Invalid request. */
                status = SNMP_NOSUCHNAME;
            }
        }
    }

    /* If we have invalid ifIndex or invalid ID length then return
       error code. */
    else
    {
        status = SNMP_NOSUCHOBJECT;
    }

    /* Returning success or error code. */
    return (status);

} /* Set_TunnelIfEntry */

/*************************************************************************
*
* FUNCTION
*
*        TunnelIfEntry
*
* DESCRIPTION
*
*        This function is used to handle the PDU request of Tunnel
*        IfTable.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  Length of snmp request.
*        *param                 Additional parameters (Not used).
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_BADVALUE          The value to set is invalid.
*        SNMP_ERROR             An error occur while processing the
*                               request.
*        SNMP_NOERROR           The action was successfully completed.
*
*************************************************************************/
UINT16 TunnelIfEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status for returning success or error code. */
    UINT16        status = SNMP_NOERROR;

    /* Flag to distinguish between get and get-next requests. */
    UINT8         getflag = 0;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);
    UNUSED_PARAMETER(idlen);

    /* Identifying request type and taking appropriate actions. */
    switch(obj->Request)
    {
    case SNMP_PDU_GET:      /* Get request. */

        /* Set getflag so that it can represent get-request. */
        getflag++;

        /* Fall through the next case for fulfilling the get-request. */

    case SNMP_PDU_NEXT:     /* Get-next request */

        /* Processing get or get-next operations. */
        status = Get_TunnelIfEntry(obj, getflag);

        break;

    case SNMP_PDU_UNDO:     /* Undo request */
    case SNMP_PDU_SET:      /* Set request */

        /* Processing of set operations. */
        status = Set_TunnelIfEntry(obj);

        break;

    case SNMP_PDU_BULK:     /* Get-bulk Request. */

        /* Processing of bulk operation. */
        status = Tunnel_Get_Bulk(obj, Get_TunnelIfEntry);

        break;

    case SNMP_PDU_COMMIT:   /* Commit Request. */

        /* We will always have a successful set. */
        break;

    default:

        status = SNMP_GENERROR;
    }

    /* Returning success or error code. */
    return (status);

} /* TunnelIfEntry */

/*************************************************************************
*
* FUNCTION
*
*        Get_TunnelConfigEntry
*
* DESCRIPTION
*
*        This function is used to get values of Tunnel ConfigTable
*        entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        getflag                Flag to distinguish between get
*                               and get next request.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_ERROR             An error occur while processing the
*                               request.
*        SNMP_NOERROR           The action was successfully completed.
*
*************************************************************************/
STATIC UINT16 Get_TunnelConfigEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Variable to hold config ID. */
    UINT32              ip_config_id = 0;

    /* Variable that is to be used in loops. */
    UINT32              loop;

    /* Object Identifier sub-length. Table entry OID is of length 12
       having local addr, remote addr, encapsulation method and config ID
       as indexes. */
    UINT32              sub_len = (14 + (2 * IP_ADDR_LEN));

    /* Object identifier of table entry. */
    UINT32              table_oid[] = {1, 3, 6, 1, 2, 1, 10, 131, 1, 1, 2,
                                       1};

    /* Status for returning success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Variable to hold local address. */
    UINT8               ip_local_address[IP_ADDR_LEN];

    /* Variable to hold remote address. */
    UINT8               ip_remote_address[IP_ADDR_LEN];

    /* Variable to hold encapsulation method. */
    UINT8               ip_encaps_method = 0;

    /* Variable to hold row status. */
    UINT8               row_status;

    /* If this is a get request then check ID length and if we have valid
       ID length then get handle to the Tunnel Configuration. */
    if(getflag)
    {
        /* Validating ID length. */
        if(obj->IdLen == (sub_len + 1))
        {
            /* Copying local and remote addresses from SNMP object passed
               in. */
            for(loop = 0; loop < IP_ADDR_LEN; loop++)
            {
                ip_local_address[loop] =
          ((UINT8)obj->Id[(sub_len - (1 + 2 * IP_ADDR_LEN) + loop)]);

                ip_remote_address[loop] =
              ((UINT8)(obj->Id[sub_len - (1 + IP_ADDR_LEN) + loop]));
            }

            /* Copying encapsulation method. */
            ip_encaps_method = ((UINT8)(obj->Id[sub_len - 1]));

            /* Copying config ID */
            ip_config_id = obj->Id[sub_len];
        }
        /* If we don't have valid OID then return error code. */
        else
        {
            /* Returning error code. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* This is a get-next request. */
    else
    {
        /* Copying local address and remote address from SNMP object
           passed in. */
        for(loop = 0; loop < IP_ADDR_LEN; loop++)
        {
            ip_local_address[loop] =
               ((UINT8)obj->Id[(sub_len - (1 + 2 * IP_ADDR_LEN) + loop)]);

            ip_remote_address[loop] =
                   ((UINT8)(obj->Id[sub_len - (1 + IP_ADDR_LEN) + loop]));
        }

        /* Copying encapsulation method from SNMP object passed in. */
        ip_encaps_method = ((UINT8)(obj->Id[sub_len - 1]));

        /* Copying config ID from SNMP object passed in. */
        ip_config_id = obj->Id[sub_len];


        /* Getting indexes of next tunnel configuration. */
        if(IP_TUNNEL_CONFIG_ENTRY_GET_NEXT(ip_local_address,
                                           ip_remote_address,
                                           ip_encaps_method,
                                           ip_config_id) == NU_FALSE)
        {
            /* Moving to next attribute. */
            obj->Id[sub_len - (2 + 2 * IP_ADDR_LEN)]++;

            /* If we we did not get next tunnel configuration then return
               error code. */
            status = SNMP_NOSUCHNAME;
        }

        /* If the attribute is 0, make this attribute 1 because this is
           the next valid attribute. */
        if(obj->Id[sub_len - (2 + 2 * IP_ADDR_LEN)] == 0)
        {
            obj->Id[sub_len - (2 + 2 * IP_ADDR_LEN)] = 1;
        }
    }

    /* If we have got the handle to the Tunnel Configuration then fulfill
       the request otherwise return error code. */
    if(status == SNMP_NOERROR)
    {
        switch(obj->Id[(sub_len - (2 + 2 * IP_ADDR_LEN))])
        {
        case 5:     /* tunnelConfigIfIndex */

            if(IP_TUNNEL_CONFIG_IFINDEX_GET(ip_local_address,
                                            ip_remote_address,
                                            ip_encaps_method,
                                            ip_config_id,
                                        (obj->Syntax.LngUns)) == NU_FALSE)
            {
                status = SNMP_NOSUCHNAME;
            }

            break;
        case 6:     /* tunnelConfigStatus */

            if(IP_TUNNEL_CONFIG_STATUS_GET(ip_local_address,
                                           ip_remote_address,
                                           ip_encaps_method,
                                           ip_config_id,
                                           row_status) == NU_TRUE)
            {
                obj->Syntax.LngUns = (UINT32)row_status;
            }
            else
            {
                status = SNMP_NOSUCHNAME;
            }

            break;

        default:
            status = SNMP_NOSUCHNAME;
        }

        /* If we have successfully fulfill the request then update the
           OID. */
        if(status == SNMP_NOERROR)
        {
            /* Prepend the table's OID . */
            NU_BLOCK_COPY(obj->Id, table_oid,
               (sub_len - (2 + 2 * IP_ADDR_LEN)) * (sizeof(UINT32)));

            /* Update the value of local and remote address . */
            for(loop = 0; loop < IP_ADDR_LEN; loop++)
            {
                obj->Id[(sub_len - (1 + 2 * IP_ADDR_LEN) + loop)] =
                                       ((UINT32)(ip_local_address[loop]));

                obj->Id[sub_len - (1 + IP_ADDR_LEN) + loop] =
                                      ((UINT32)(ip_remote_address[loop]));
            }

            /* Update the value of encapsulation method */
            obj->Id[sub_len - 1] = ((UINT32)(ip_encaps_method));

            /* Update the value of config ID. */
            obj->Id[sub_len] = ip_config_id;

            /* Update the length of the OID. */
            obj->IdLen = sub_len + 1;
        }
    }

    /* If we did not get the handle to tunnel configuration then return
       error code. */
    else
    {
        /* Returning error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* Returning success or error code. */
    return (status);

} /* Get_TunnelConfigEntry */

/*************************************************************************
*
* FUNCTION
*
*        Set_TunnelConfigEntry
*
* DESCRIPTION
*
*        This function is used to set values of Tunnel IfTable entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_BADVALUE          The value to set is invalid.
*        SNMP_ERROR             An error occur while processing the
*                               request.
*        SNMP_NOERROR           The action was successfully completed.
*
*************************************************************************/
STATIC UINT16 Set_TunnelConfigEntry(snmp_object_t *obj)
{
    /* Variable to hold config ID. */
    UINT32              ip_config_id;

    /* Variable that is to be used in loops. */
    UINT32              loop;

    /* Object Identifier sub-length. Table entry OID is of length 12
       having local addr, remote addr, encapsulation method and config ID
       as indexes. */
    UINT32              sub_len = (14 + (2 * IP_ADDR_LEN));

    /* Status for returning success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Variable to hold local address. */
    UINT8               ip_local_address[IP_ADDR_LEN];

    /* Variable to hold remote address. */
    UINT8               ip_remote_address[IP_ADDR_LEN];

    /* Variable to hold encapsulation method. */
    UINT8               ip_encaps_method;

    /* Variable to hold row status. */
    UINT8               row_status;

    /* Validating ID length. */
    if(obj->IdLen == (sub_len + 1))
    {
        /* Copying local and remote addresses from SNMP object passed
           in. */
        for(loop = 0; loop < IP_ADDR_LEN; loop++)
        {
            ip_local_address[loop] =
          ((UINT8)obj->Id[(sub_len - (1 + 2 * IP_ADDR_LEN) + loop)]);

            ip_remote_address[loop] =
              ((UINT8)(obj->Id[sub_len - (1 + IP_ADDR_LEN) + loop]));
        }

        /* Copying encapsulation method. */
        ip_encaps_method = ((UINT8)(obj->Id[sub_len - 1]));

        /* Copying config ID*/
        ip_config_id = obj->Id[sub_len];

        /* Getting row status, just to check the existence of tunnel
           configuration. */
        if(IP_TUNNEL_CONFIG_STATUS_GET(ip_local_address,ip_remote_address,
                                       ip_encaps_method, ip_config_id,
                                       row_status) == NU_FALSE)
        {
            status = SNMP_NOSUCHNAME;
        }
    }
    /* If we have the invalid OID then return error code. */
    else
    {
        /* Returning error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* If we have valid OID and tunnel configuration exists then process
       the set request. */
    if(status == SNMP_NOERROR)
    {
        switch(obj->Id[(sub_len - (2 + 2 * IP_ADDR_LEN))])
        {
        case 6:     /* tunnelConfigStatus */
            /* If we have invalid value then return error code otherwise
               do nothing. */
            if((obj->Syntax.LngUns - 1) >= 6)
            {
                status = SNMP_WRONGVALUE;
            }

            break;

        default:
            /* We have request to set a invalid attribute, so return error
               code. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* Returning success or error code. */
    return (status);

} /* Set_TunnelConfigEntry */

/*************************************************************************
*
* FUNCTION
*
*        Create_TunnelConfigEntry
*
* DESCRIPTION
*
*        This function is used to set values of Tunnel IfTable entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*
* OUTPUTS
*
*        SNMP_ERROR             An error occur while processing the
*                               request.
*        SNMP_NOERROR           The action was successfully completed.
*
*************************************************************************/
STATIC UINT16 Create_TunnelConfigEntry(snmp_object_t *obj)
{
    /* Variable to hold config ID. */
    UINT32              ip_config_id;

    /* Variable that is to be used in loops. */
    UINT32              loop;

    /* Object Identifier sub-length. Table entry OID is of length 12
       having local addr, remote addr, encapsulation method and config ID
       as indexes. */
    UINT32              sub_len = (14 + (2 * IP_ADDR_LEN));

    /* Status for returning success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Variable to hold local address. */
    UINT8               ip_local_address[IP_ADDR_LEN];

    /* Variable to hold remote address. */
    UINT8               ip_remote_address[IP_ADDR_LEN];

    /* Variable to hold encapsulation method. */
    UINT8               ip_encaps_method;

    /* Validating ID length. */
    if(obj->IdLen == (sub_len + 1))
    {
        /* Copying local and remote addresses from SNMP object passed
           in. */
        for(loop = 0; loop < IP_ADDR_LEN; loop++)
        {
            ip_local_address[loop] =
               ((UINT8)obj->Id[(sub_len - (1 + 2 * IP_ADDR_LEN) + loop)]);

            ip_remote_address[loop] =
                   ((UINT8)(obj->Id[sub_len - (1 + IP_ADDR_LEN) + loop]));
        }

        /* Copying encapsulation method. */
        ip_encaps_method = ((UINT8)(obj->Id[sub_len - 1]));

        /* Copying config ID*/
        ip_config_id = obj->Id[sub_len];

        if(IP_TUNNEL_CONFIG_ENTRY_CREATE(ip_local_address,
                                         ip_remote_address,
                                         ip_encaps_method,
                                         ip_config_id) == NU_FALSE)
        {
            status = SNMP_ERROR;
        }
    }

    /* If we did not have the valid SNMP object then return error code. */
    else
    {
        /* Returning error code. */
        status = SNMP_ERROR;
    }

    /* Returning success or error code. */
    return (status);

} /* Create_TunnelConfigEntry */

/*************************************************************************
* FUNCTION
*
*        Undo_TunnelConfigEntry
*
* DESCRIPTION
*
*        This function is used to undo Group Address table entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*
* OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeds.
*        SNMP_ERROR             If the processing fails.
*
*************************************************************************/
STATIC UINT16 Undo_TunnelConfigEntry(snmp_object_t *obj)
{
    /* Variable to hold config ID. */
    UINT32              ip_config_id;

    /* Variable that is to be used in loops. */
    UINT32              loop;

    /* Object Identifier sub-length. Table entry OID is of length 12
       having local addr, remote addr, encapsulation method and config ID
       as indexes. */
    UINT32              sub_len = (14 + (2 * IP_ADDR_LEN));

    /* Status for returning success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Variable to hold local address. */
    UINT8               ip_local_address[IP_ADDR_LEN];

    /* Variable to hold remote address. */
    UINT8               ip_remote_address[IP_ADDR_LEN];

    /* Variable to hold encapsulation method. */
    UINT8               ip_encaps_method;

    /* Variable to hold the value of row status, */
    UINT8               row_status;

    /* Validating ID length. */
    if(obj->IdLen == (sub_len + 1))
    {
        /* Copying local and remote addresses from SNMP object passed
           in. */
        for(loop = 0; loop < IP_ADDR_LEN; loop++)
        {
            ip_local_address[loop] =
               ((UINT8)obj->Id[(sub_len - (1 + 2 * IP_ADDR_LEN) + loop)]);

            ip_remote_address[loop] =
                   ((UINT8)(obj->Id[sub_len - (1 + IP_ADDR_LEN) + loop]));
        }

        /* Copying encapsulation method. */
        ip_encaps_method = ((UINT8)(obj->Id[sub_len - 1]));

        /* Copying config ID*/
        ip_config_id = obj->Id[sub_len];

        switch(obj->Id[(sub_len - (2+ 2 * IP_ADDR_LEN))])
        {
        case 6      :/* tunnelConfigStatus */

            /* Getting value of row status. */
            row_status = (UINT8)(obj->Syntax.LngUns);

            /* Processing the undo operation. */
            IP_TUNNEL_CONFIG_STATUS_UNDO(ip_local_address,
                                         ip_remote_address,
                                         ip_encaps_method, ip_config_id,
                                         row_status);
            break;

        default:
            /* We got the request of UNDO for invalid attribute. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* If we did not have the valid object ID then return error code. */
    else
    {
        /* Returning error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* Return success or error code. */
    return (status);

} /* Undo_TunnelConfigEntry */

/*************************************************************************
*
* FUNCTION
*
*        TunnelConfigEntry
*
* DESCRIPTION
*
*        This function handles request of Group Address table.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  Length of snmp request.
*        *param                 Additional parameters (Not used).
*
* OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeded.
*        SNMP_ERROR             If invalid value.
*        SNMP_NOSUCHNAME        Object does not exist.
*        SNMP_INCONSISTANTVALUE If commit fails.
*        SNMP_GENERROR          Invalid request.
*
*************************************************************************/
UINT16 TunnelConfigEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Variable to hold config ID. */
    UINT32              ip_config_id;

    /* Variable that is to be used in loops. */
    UINT32              loop;

    /* Object Identifier sub-length. Table entry OID is of length 12
       having local addr, remote addr, encapsulation method and config ID
       as indexes. */
    UINT32              sub_len = (14 + (2 * IP_ADDR_LEN));

    /* Status for returning success or error code. */
    UINT16              status;

    /* Variable to hold local address. */
    UINT8               ip_local_address[IP_ADDR_LEN];

    /* Variable to hold remote address. */
    UINT8               ip_remote_address[IP_ADDR_LEN];

    /* Variable to hold encapsulation method. */
    UINT8               ip_encaps_method;

    /* Flag to distinguish between get and get-next requests. */
    UINT8               getflag = 0;

    /* Variable to hold the value of row status. */
    UINT8               row_status;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);
    UNUSED_PARAMETER(idlen);

    switch(obj->Request)
    {
    case SNMP_PDU_GET:

        getflag++;

        /* Fall through the next case */

    case SNMP_PDU_NEXT:

        /* Processing get operations. */
        status = Get_TunnelConfigEntry(obj, getflag);

        break;

    case SNMP_PDU_SET:

        /* Increment IP_Tun_Commit_Left to reflect total number of commit
           left. */
        IP_Tun_Commit_Left++;

        /* Processing of set operations. */
        status = Set_TunnelConfigEntry(obj);

        /* If the instance was not found, create it. */
        if(status != SNMP_NOSUCHNAME)
            break;

    case SNMP_PDU_CREATE:

        /* Processing of create operations. */
        status = Create_TunnelConfigEntry(obj);

        /* If the entry was successfully created, set the value. */
        if(status == SNMP_NOERROR)
            status = Set_TunnelConfigEntry(obj);

        break;

    case SNMP_PDU_UNDO:

        IP_Tun_Commit_Left = 0;

        /* Processing of undo operations. */
        status = Undo_TunnelConfigEntry(obj);

        break;

    case SNMP_PDU_COMMIT:

        /* We going to perform one commit operation. */
        IP_Tun_Commit_Left--;

        /* Copying local and remote addresses from SNMP object passed
           in. */
        for(loop = 0; loop < IP_ADDR_LEN; loop++)
        {
            ip_local_address[loop] =
               ((UINT8)obj->Id[(sub_len - (1 + 2 * IP_ADDR_LEN) + loop)]);

            ip_remote_address[loop] =
                   ((UINT8)(obj->Id[sub_len - (1 + IP_ADDR_LEN) + loop]));
        }

        /* Copying encapsulation method. */
        ip_encaps_method = ((UINT8)(obj->Id[sub_len - 1]));

        /* Copying config ID*/
        ip_config_id = obj->Id[sub_len];

        /* Copying the value of row status. */
        row_status = (UINT8)obj->Syntax.LngUns;

        status = IP_TUNNEL_CONFIG_STATUS_COMMIT(ip_local_address,
                                                ip_remote_address,
                                                ip_encaps_method,
                                                ip_config_id,
                                                row_status);

        /* If this was the last commit operation and this commit
           operation is also successful. */
        if ((IP_Tun_Commit_Left == 0) && (status == SNMP_NOERROR))
        {
            /* Remove all the entries from temporary list and
               add them to permanent list. */
            IP_TUNNEL_CONFIG_ENTRIES_COMMIT();
        }

        break;

    case SNMP_PDU_BULK:

        /* Processing of bulk operation. */
        status = Tunnel_Get_Bulk(obj, Get_TunnelConfigEntry);

        break;

    default:

        status = SNMP_GENERROR;
    }

    /* Returning status. */
    return (status);

} /* TunnelConfigEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Tunnel_Get_Bulk
*
*   DESCRIPTION
*
*       This function is used to get bulk responses against a single
*       object.
*
* INPUTS
*
*       *obj                    SNMP object containing request.
*       get_function            Pointer to getter function.
*
* OUTPUTS
*
*       SNMP_NOERROR            If processing succeeded.
*       SNMP_NOSUCHNAME         Otherwise.
*
*************************************************************************/
STATIC UINT16 Tunnel_Get_Bulk(snmp_object_t *obj, TUN_GET_FUNCTION
                                                             get_function)
{
    /* SNMP dummy object for making multiple get request. */
    snmp_object_t   dummy_object;

    UINT32          max_repetitions, index;

    /* Status for returning success or error code. */
    UINT16          status = SNMP_NOERROR;

    /* Get the No. of repetitions for this particular object. */
    max_repetitions = obj[0].Syntax.LngUns;

    /* Get the object. */
    dummy_object= obj[0];

    /* Loop for max number of repetitions. */
    for(index = 0;
                  ((index < max_repetitions) && (status == SNMP_NOERROR));
                  index++)
    {
        /* Clear the temporary object where the instance will be
           retrieved. */
        UTL_Zero(dummy_object.Syntax.BufChr, SNMP_SIZE_BUFCHR);

        /* Get the next object. */
        if(get_function((&dummy_object), 0) != NU_SUCCESS)
        {
            /* If no next object was available and this is the first loop
               (meaning we have not gotten any object) return no such
               name. This will tell the MIB engine to not look at the SNMP
               object list. If this is not the first loop (meaning we have
               retrieved at least one instance, return success. The MIB
               engine will retrieve the values from the SNMP object list.
            */
            if(index == 0)
            {
                status = SNMP_NOSUCHNAME;
            }

            break;
        }

        /* The temporary object should indicate that this is a GET-BULK
           request. */
        dummy_object.Request = SNMP_PDU_BULK;

        /* Put the retrieved instance in to the SNMP object list. */
        obj[index] = dummy_object;
    }

    /* Return Status. */
    return (status);

} /* Tunnel_Get_Bulk */


#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

