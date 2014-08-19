/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       1213if.c                                                 
*
* DESCRIPTION
*
*        This file contains those functions specific to processing PDU
*        requests on parameters in the Interfaces Group.
*
* DATA STRUCTURES
*
*        None
*
* FUNCTIONS
*
*        IfNumber
*        IfTableLastChange
*        IfStackLastChange
*        Get_If1213Entry
*        Set_If1213Entry
*        If1213Entry
*        Get_IfXEntry
*        Set_IfXEntry
*        IfXEntry
*        Get_IfStackEntry
*        IfStackEntry
*        Get_IfRcvAddressEntry
*        IfRcvAddressEntry
*
* DEPENDENCIES
*
*        nu_net.h
*        snmp_api.h
*        snmp_comp.h
*        mib.h
*        1213xxxx.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/snmp_api.h"
#include "networking/snmp_comp.h"
#include "networking/mib.h"
#include "networking/1213xxxx.h"

#if ( (RFC1213_IF_INCLUDE == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) )

#if ((INCLUDE_IF_STACK == NU_TRUE) && (INCLUDE_IF_STACK_MIB == NU_TRUE))

#endif /* ((INCLUDE_IF_STACK == NU_TRUE) && \
           (INCLUDE_IF_STACK_MIB == NU_TRUE)) */
#if (INCLUDE_SNMP == NU_TRUE)
extern UINT32                       MIB2_If_Tbl_Last_Change;
#endif

/************************************************************************
*
* FUNCTION
*
*        IfNumber
*
* DESCRIPTION
*
*        This function processes the PDU action on the IfNumber
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 IfNumber(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status for returning success or error code. */
    UINT16  status = SNMP_NOERROR;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);

    /* Validating SNMP request. */
    if(MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        /* Getting count of interfaces. */
        obj->Syntax.LngInt = (INT32)(MIB2_totalInterface_Get);

    /* Return status. */
    return (status);

} /* IfNumber */

#if ((INCLUDE_IF_EXT == NU_TRUE) && (INCLUDE_IF_EXT_MIB == NU_TRUE))

/************************************************************************
*
* FUNCTION
*
*        IfTableLastChange
*
* DESCRIPTION
*
*        This function processes the PDU action on the IfTableLastChange
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 IfTableLastChange(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status for returning success or error code. */
    UINT16  status = SNMP_NOERROR;

    /* Avoiding compilation warnings */
    UNUSED_PARAMETER(param);

    /* Validating the request. */
    if(MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        /* Getting value of time stamp representing time of last change in
           ifTable. */
        obj->Syntax.LngUns = MIB2_IfTableLastChange_Get;

    /* Return status. */
    return (status);

} /* IfTableLastChange */

#endif /* ((INCLUDE_IF_EXT == NU_TRUE) && \
           (INCLUDE_IF_EXT_MIB == NU_TRUE)) */

#if ((INCLUDE_IF_STACK == NU_TRUE) && (INCLUDE_IF_STACK_MIB == NU_TRUE))
/*************************************************************************
*
* FUNCTION
*
*        IfStackLastChange
*
* DESCRIPTION
*
*        This function processes the PDU action on the IfStackLastChange
*        parameter of MIB II.
*
* INPUTS
*
*        *obj                   A pointer to the object.
*        idlen                  The length of the object ID.
*        *param                 Unused parameter.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The request was processed successfully.
*
*************************************************************************/
UINT16 IfStackLastChange(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status for returning success or error code. */
    UINT16  status = SNMP_NOERROR;

    /* Avoiding compilation warnings */
    UNUSED_PARAMETER(param);

    /* Validating SNMP request. */
    if(MibSimple(obj, idlen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        /* Getting the time of last change in ifStackTable. */
        obj->Syntax.LngUns = MIB2_IF_STACK_LAST_CHANGE_GET;

    /* Return status. */
    return (status);

} /* IfStackLastChange */

#endif /* ((INCLUDE_IF_STACK == NU_TRUE) && \
           (INCLUDE_IF_STACK_MIB == NU_TRUE)) */

/*************************************************************************
* FUNCTION
*
*         Get_If1213Entry
*
* DESCRIPTION
*
*         This function is used to get values of If table entries.
*
* INPUTS
*
*         *obj                  SNMP object containing request.
*         getflag               flag to distinguish between get
*                               and get next request.
*
* OUTPUTS
*
*        SNMP_NOSUCHNAME        The object does not exist.
*        SNMP_NOERROR           The action was successfully completed.
*
*************************************************************************/
UINT16 Get_If1213Entry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object Identifier sub-length. Table entry OID is of length 9 having
       ifIndex  as index this results in sub-length of 10.  */
    UINT32                  sub_len = 10;

    /* Object identifier of table entry. */
    UINT32                  table_oid[9] = {1, 3, 6, 1, 2, 1, 2, 2, 1};

    /* Interface index. */
    UINT32                  if_index = 0;

    /* Status for returning success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* If this is a get request, just get the instance for the device
       Entry. */
    if(getflag)
    {
        /* Verify that the OID length in the object is equal to the
           instance level OID for the received SNMP object. And that we
           have a valid IfIndex. */
        if((obj->IdLen == sub_len + 1) && (obj->Id[sub_len] > 0))
        {
            /* Get the ifIndex. Subtract one, because the device
               index for Nucleus NET starts at 0. IfIndex for MIB-2
               starts at 1. */
            if_index = obj->Id[sub_len] - 1;
        }
        else
        {
            /* Returning error code. */
            status = SNMP_NOSUCHINSTANCE;
        }
    }

    /* This is a get-next request. */
    else
    {
#if (NET_VERSION_COMP > NET_5_1)
        if_index = obj->Id[sub_len];

        /* Invoke function responsible for getting the next device's
           ifIndex. */
        status = MIB2_Get_Next_Device_By_Index(&if_index);


        if (status != NU_SUCCESS)
            status = SNMP_NOSUCHINSTANCE;

#else /* (NET_VERSION_COMP > NET_5_1) */
        /* Getting next interface index. */
        if(MIB2_totalInterface_Get > obj->Id[sub_len])
        {
            if_index = obj->Id[sub_len];
        }
        else
        {
            if_index = 0;
        }
#endif /* (NET_VERSION_COMP > NET_5_1) */

        /* If the index of the next device received is less than or equal
           to the index passed, then increment the second last identifier.
           We do this because we have exhausted all the device instances
           for the current attribute (signified by the second last
           identifier). We are incrementing to go to the next attribute.
        */
        if((obj->Id[sub_len] != 0) &&
           (if_index <= (obj->Id[sub_len] - 1)))
        {
            obj->Id[sub_len - 1]++;

            /* Returning error code. */
            status = SNMP_NOSUCHINSTANCE;
        }

        /* If the attribute is 0, make this attribute 1 because this is
           the next valid attribute. */
        if(obj->Id[sub_len - 1] == 0)
        {
            obj->Id[sub_len - 1] = 1;
        }
    }

    /* If we have obtained the handle to the device then process
       the request. */
    if(status == SNMP_NOERROR)
    {
        switch(obj->Id[sub_len - 1])
        {
        case 1:     /* IfIndex */
            /* Getting ifIndex of the interface. */
            if(MIB2_ifIndex_Get(if_index) == NU_SUCCESS)
            {
                /* Copying ifIndex in SNMP response object. We add 1
                   because ifIndex in SNMP starts from 1 and in NUCLEUS
                   it starts from 0. */
                obj->Syntax.LngUns = if_index + 1;
            }
            else
            {
                /* Setting error code to status. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 2:     /* IfDescr */
            /* Getting interface description value. */
            if(MIB2_ifDescr_Get(if_index, obj->Syntax.BufChr,
                                obj->SyntaxLen) == MIB2_UNSUCCESSFUL)
            {
                /* Setting error code to status. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 3:     /* IfType */
            /* Getting interface type. */
            if(MIB2_ifType_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 4:     /* IfMtu */
            /* Getting value of interface MTU. */
            if(MIB2_ifMtu_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting error code to status. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 5:     /* IfSpeed */
            /* Getting interface speed value. */
            if(MIB2_ifSpeed_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 6:     /* IfPhysAddress */
            /* Getting interface PHY Address. */
            if(MIB2_ifAddr_Get(if_index, obj->Syntax.BufChr,
                                obj->SyntaxLen) == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 7:     /* IfAdminStatus */
            /* Getting value of interface Admin status. */
            if(MIB2_ifStatusAdmin_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 8:     /* IfOperStatus */
            /* Getting value of interface operator status. */
            if(MIB2_ifStatusOper_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 9:     /* IfLastChange */
            /* Getting value interface last change. */
            if(MIB2_ifLastChange_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 10:    /* IfInOctets */
            /* Getting count of In Octets (number of octets interface had
               received). */
            if(MIB2_ifInOctets_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 11:    /* IfInUcastPkts */
            /* Getting count of In Unicast Packets (number of unicast
               packets an interface have received). */
            if(MIB2_ifInUcastPkts_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 12:    /* IfInNUcastPkts */
            /* Getting count of In Non-Unicast Packets (number of 
            non-unicast packets an interface have received). */
            if(MIB2_ifInNUcastPkts_Get(if_index, obj->Syntax.LngInt)
                == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 13:    /* IfInDiscards */
            /* Getting count of In Discard (number of packets received but
               discarded). */
            if(MIB2_ifInDiscards_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 14:    /* IfInErrors */
            /* Getting count of In Errors (number of packets received that
               had errors). */
            if(MIB2_ifInErrors_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 15:    /* IfInUnknownProtos */
            /* Getting count of In Unknown Protocol (number of packets
             * with unknown protocol received).
             */
            if(MIB2_ifInUnknownProtos_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 16:    /* IfOutOctets */
            /* Getting count of Out Octets (number of octets sent by the
               interface). */
            if(MIB2_ifOutOctets_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 17:    /* IfOutUcastPkts */
            /* Getting count of Out Unicast Packets (number of unicast
               packets sent by the interface). */
            if(MIB2_ifOutUcastPkts_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 18:    /* IfOutNUcastPkts */
            /* Getting count of Out Non-Unicast Packets (number of 
            non-unicast packets sent by the interface). */
            if(MIB2_ifOutNUcastPkts_Get(if_index, obj->Syntax.LngInt)
                == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 19:    /* IfOutDiscards */
            /* Getting count of Out Discard (number of packets sent
               outward but discarded). */
            if(MIB2_ifOutDiscards_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 20:    /* IfOutErrors */
            /* Getting count of Out Errors (number packets sent by the
               interface that have error)*/
            if(MIB2_ifOutErrors_Get(if_index, obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 21:    /* IfOutQErrors */
            /* Getting Length of Out Errors packet
            (length of the output packet queue in packets)*/
            if(MIB2_ifOutQLen_Get(if_index, obj->Syntax.LngInt)
                == MIB2_UNSUCCESSFUL)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 22:    /* ifSpecific */
            /* Getting value of ifSpecific. */
            if(MIB2_ifSpecific_Get(if_index,
                obj->Syntax.BufChr, obj->SyntaxLen) != NU_SUCCESS)
            {
                /* Returning error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        default:
            /* We have reached the end of the table.
               There are no more attributes in this table. */
            status = SNMP_NOSUCHNAME;

            break;
        }

        /* If the request was successful. */
        if(status == NU_SUCCESS)
        {
            /* Prepend the table's OID . */
            NU_BLOCK_COPY(obj->Id, table_oid,
                          (sub_len - 1) * (sizeof(UINT32)));

            /* Update the value of ifIndex in the OID of the object. */
            obj->Id[sub_len] = if_index + 1;

            /* Update the length of the OID. */
            obj->IdLen = sub_len + 1;
        }
    }

    /* Return status */
    return (status);

} /* Get_If1213Entry */

/*************************************************************************
* FUNCTION
*
*        Set_If1213Entry
*
* DESCRIPTION
*
*        This function is used to set values of If table entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*
* OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeds.
*        SNMP_NOSUCHINSTANCE    If the instance is not present.
*        SNMP_WRONGVALUE        If invalid value.
*        SNMP_NOSUCHNAME        Object does not exist.
*
*************************************************************************/
UINT16 Set_If1213Entry(snmp_object_t *obj)
{
    /* Object Identifier sub-length. Table entry OID is of length 9 having
       ifIndex  as index this results in sub-length of 10. */
    UINT32                  sub_len = 10;

    /* Status for returning success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Verify that the OID length in the object is equal to the instance
       level OID for the received SNMP object. And that a valid IfIndex
       exists. */
    if((obj->IdLen == sub_len + 1) && (obj->Id[sub_len] > 0))
    {
        switch(obj->Id[sub_len - 1])
        {
        case 7:     /* ifAdminStatus */
            /* Verifying the existence of the interface or device. */
            if(MIB2_Get_IfIndex((UINT32)(obj->Id[sub_len] - 1))
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Returning error code if interface does not exists */
                status = SNMP_NOSUCHINSTANCE;
            }

            /* Verifying the value to be set. */
            else if ((obj->Syntax.LngUns - 1) > 1)
            {
                /* Returning error code if value was invalid. */
                status = SNMP_WRONGVALUE;
            }

            /* Setting value to admin status. */
            else if (MIB2_ifStatusAdmin_Set((obj->Id[sub_len] - 1),
                                            obj->Syntax.LngInt)
                                                     == MIB2_UNSUCCESSFUL)
            {
                /* Returning error code if set operation failed. */
                status = SNMP_GENERROR;
            }

            break;

        default:

            /* End of the table reached.
               There are no more attributes in this table. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* If we have invalid OID then return error code. */
    else
    {
        /* Returning error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* Return success or error code. */
    return (status);

} /* Set_If1213Entry */

/*************************************************************************
* FUNCTION
*
*        If1213Entry
*
* DESCRIPTION
*
*        This function is used to handle requests of If table.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  length of SNMP request.
*        *param                 Additional parameters (Not used).
*
* OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeded.
*        SNMP_NOSUCHINSTANCE    If the instance is not present.
*        SNMP_WRONGVALUE        If invalid value.
*        SNMP_NOSUCHNAME        Object does not exist.
*        SNMP_INCONSISTANTVALUE If commit fails.
*        SNMP_GENERROR          Invalid request.
*
*************************************************************************/
UINT16 If1213Entry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Flag to distinguish between get and get-next requests. */
    UINT8         getflag = 0;

    /* Status for returning success or error code. */
    UINT16        status = SNMP_NOERROR;

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

    case SNMP_PDU_NEXT:     /* Get next request. */

        /* Processing get operations. */
        status = Get_If1213Entry(obj, getflag);
        break;

    case SNMP_PDU_UNDO:     /* Undo request. */
    case SNMP_PDU_SET:      /* Set request. */

        /* Processing of set operations. */
        status = Set_If1213Entry(obj);
        break;

    case SNMP_PDU_BULK:     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_If1213Entry);
        break;

    case SNMP_PDU_COMMIT:   /* Commit request. */

        /* We will always have a successful set. */
        break;

    default:            /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return status. */
    return (status);

} /* If1213Entry */

#if ((INCLUDE_IF_EXT == NU_TRUE) && (INCLUDE_IF_EXT_MIB == NU_TRUE))

/*************************************************************************
* FUNCTION
*
*        Get_IfXEntry
*
* DESCRIPTION
*
*        This function is used to get values of IfX table entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        getflag                flag to distinguish between get
*                               and get next request.
*
* OUTPUTS
*
*       SNMP_NOSUCHNAME         The object does not exist.
*       SNMP_NOERROR            The action was successfully completed.
*
*************************************************************************/
UINT16 Get_IfXEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* device index or ifIndex. */
    UINT32                  if_index = 0;

    /* Object Identifier sub-length. Table entry OID is of length 10
     * having ifIndex  as index this results in sub-length of 11.
     */
    UINT32                  sub_len = 11;

    /* Object identifier of table entry. */
    UINT32                  table_oid[10] = {1, 3, 6, 1, 2, 1, 31, 1, 1,
                                             1};

    /* Status for returning success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* If this is a get request, just get the instance for the device
       Entry. */
    if(getflag)
    {
        /* Verify that the OID length in the object is equal to the
           instance level OID for the received SNMP object. And that we
           have a valid IfIndex. */
        if((obj->IdLen == sub_len + 1) && (obj->Id[sub_len] > 0))
        {
            /* Get the ifIndex. Subtract one, because the device
               index for Nucleus NET starts at 0. IfIndex for MIB-2
               starts at 1. */
            if_index = obj->Id[sub_len] - 1;
        }
    }

    /* This is a get-next request. */
    else
    {
        if_index = obj->Id[sub_len];

        /* Invoke function responsible for getting the next device. */
        status = MIB2_Get_Next_IfIndex_Ext(&if_index);

        /* If we have not found the next device. */
        if(status != NU_SUCCESS)
            status = SNMP_NOSUCHINSTANCE;

        /* If the index of the next device received is less than or equal
           to the index passed, then increment the second last identifier.
           We do this because we have exhausted all the device instances
           for the current attribute (signified by the second last
           identifier). We are incrementing to go to the next attribute.
        */
        else if((obj->Id[sub_len] != 0) &&
                (if_index <= (obj->Id[sub_len] - 1)))
        {
            obj->Id[sub_len - 1]++;

            /* Setting device entry pointer to null. */
            status = SNMP_NOSUCHNAME;
        }

        /* If the attribute is 0, make this attribute 1 because this is
           the next valid attribute. */
        if(obj->Id[sub_len - 1] == 0)
        {
            obj->Id[sub_len - 1] = 1;
        }
    }


    if(status == SNMP_NOERROR)
    {
        switch(obj->Id[sub_len - 1])
        {
        case 1:     /* ifName */
            /* Getting ifName value. */
            if(MIB2_IfName_Get(if_index, obj->Syntax.BufChr) ==
                                                            NU_SUCCESS)
            {
                /* Getting ifName length. */
                obj->SyntaxLen = strlen((CHAR *)(obj->Syntax.BufChr));
            }
            else
            {
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 2:     /* ifInMulticastPtks */
            /* Getting count of In Multicast Packets. */
            if(MIB2_InMulticast_Pkts_Get(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 3:     /* ifInBroadcastPkts */
            /* Getting count of In Broadcast Packets. */
            if(MIB2_InBroadcast_Pkts_Get(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 4:     /* ifOutMulticatsPkts */
            /* Getting count of Out Multicast Packets. */
            if(MIB2_OutMulticast_Pkts_Get(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 5:     /* ifOutBroadcastPkts */
            /* Getting count of Out Broadcast Packets. */
            if(MIB2_OutBroadcast_Pkts_Get(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 6:     /* ifHCInOctets */
            /* Getting 64 bit count of In Octets. */
            if(MIB2_ifInOctets_Get64(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 7:     /* ifHCInUcastPkts */
            /* Getting 64 bit count of In Unicast Packets. */
            if(MIB2_ifInUcastPkts_Get64(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 8:     /* ifHCInMulticastPkts */
            /* Getting 64 bit count of In Multicast Packets. */
            if(MIB2_InMulticast_Pkts_Get64(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 9:     /* ifHCInBroadcastPkts */
            /* Getting 64 bit count of In Broadcast Packets. */
            if(MIB2_InBroadcast_Pkts_Get64(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 10:    /* ifHCOutOctets */
            /* Getting 64 bit counter of Out Octets. */
            if(MIB2_ifOutOctets_Get64(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 11:    /* ifHCOutUcastPkts */
            /* Getting 64 bit count of Out Unicast Packets. */
            if(MIB2_ifOutUcastPkts_Get64(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 12:    /* ifHCOutMulticastPkts */
            /* Getting 64 bit counter of Out Multicast Packets. */
            if(MIB2_OutMulticast_Pkts_Get64(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 13:    /* ifHCOutBroadcastPkts */
            /* Getting 64 bit counter of Out Broadcast Packets. */
            if(MIB2_OutBroadcast_Pkts_Get64(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 14:    /* ifLinkUpDownTrapEnable */
            /* Getting ifLinkUpDownTrapEnable value. */
            if(MIB2_IfLinkTrapEnable_Get(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 15:    /* ifHighSpeed */
            /* Getting ifHighSpeed value. */
            if(MIB2_IfHighSpeed_Get(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 16:    /* ifPromiscuousMode */
           /* Getting value of interface Promiscuous Mode. */
           if(MIB2_Promiscuous_Mode_Get(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
           {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
           }

           break;

        case 17:    /* ifConnectorPresent */
            /* Getting value of interface Connector Present. */
            if(MIB2_IfConnectorPresent_Get(if_index, obj->Syntax.LngUns)
                                                            != NU_SUCCESS)
            {
                /* Setting status to error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 18:    /* ifAlias */
            /* Getting ifAlias value. */
            if(MIB2_IfAlias_Get(if_index, obj->Syntax.BufChr) ==
                                                            NU_SUCCESS)
            {
                /* Getting value length of interface Alias */
                obj->SyntaxLen = strlen((CHAR *)(obj->Syntax.BufChr));
            }
            else
            {
                /* Returning error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;

        case 19:    /* ifCounterDiscontinuityTime */
            /* Getting value of counter discontinuity. */
            if(MIB2_IfCounterDiscontinTime_Get(if_index,
                                        obj->Syntax.LngUns) != NU_SUCCESS)
            {
                /* Returning error code. */
                status = SNMP_NOSUCHNAME;
            }

            break;


        default:
            /* We have reached the end of the table.
               There are no more attributes in this table. */
            status = SNMP_NOSUCHNAME;
        }

        /* If the request was successful. */
        if(status == NU_SUCCESS)
        {
            /* Prepend the table's OID . */
            NU_BLOCK_COPY(obj->Id, table_oid,
                          (sub_len - 1) * (sizeof(UINT32)));

            /* Update the value of ifIndex in the OID of the object. */
            obj->Id[sub_len] = if_index + 1;

            /* Update the length of the OID. */
            obj->IdLen = sub_len + 1;
        }
    }

    /* If we did not find the handle to the device then return error code.
     */
    else
    {
        /* Returning error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* Return success or error code. */
    return (status);

} /* Get_IfXEntry */

/*************************************************************************
* FUNCTION
*
*        Set_IfXEntry
*
* DESCRIPTION
*
*        This function is used to set values of IfX table entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*
* OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeded.
*        SNMP_READONLY          If object is read-only.
*        SNMP_WRONGVALUE        If invalid value.
*        SNMP_NOSUCHNAME        Object does not exist.
*
*************************************************************************/
UINT16 Set_IfXEntry(snmp_object_t *obj)
{
    /* Object Identifier sub-length. Table entry OID is of length 10
       having ifIndex  as index this results in sub-length of 11. */
    UINT32                  sub_len = 11;

    /* Status for returning success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Verify that the OID length in the object is equal to the instance
       level OID for the received SNMP object. And that we have a valid
       IfIndex. */
    if((obj->IdLen == sub_len + 1) && (obj->Id[sub_len] > 0))
    {
        switch(obj->Id[sub_len - 1])
        {
        case 14:    /* ifLinkUpDownTrapEnable */
            /* Validating the value before setting it. */
            if((obj->Syntax.LngUns - 1) <= 1)
            {
                /* Setting value to Link Up Down Trap Enable. */
                if(MIB2_IfLinkTrapEnable_Set((obj->Id[sub_len] - 1),
                                        obj->Syntax.LngUns) != NU_SUCCESS)
                {
                    /* Returning error code. */
                    status = SNMP_NOSUCHNAME;
                }
            }

            /* We have invalid value. */
            else
            {
                /* Setting status to error code. */
                status = SNMP_WRONGVALUE;
            }

            break;

        case 18:    /* ifAlias */
            /* Validating the value before setting it. */
            if(obj->SyntaxLen <= 64)
            {
                /* Setting value to interface Alias. */
                if(MIB2_IfAlias_Set(obj->Id[sub_len] - 1,
                                        obj->Syntax.BufChr) != NU_SUCCESS)
                {
                    /* Returning error code. */
                    status = SNMP_NOSUCHNAME;
                }
            }

            /* We have invalid value. */
            else
            {
                /* Setting status to error code. */
                status = SNMP_WRONGVALUE;
            }

            break;

        default:

            /* We have reached the end of the table.
               There are no more attributes in this table. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* If we did not have the valid OID then return error code. */
    else
    {
        /* Returning error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* Return success or error code. */
    return (status);

} /* Set_IfXEntry */

/*************************************************************************
* FUNCTION
*
*        IfXEntry
*
* DESCRIPTION
*
*        This function is used to handle requests of IfX table.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  length of SNMP request.
*        *param                 Additional parameters (Not used).
*
* OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeded.
*        SNMP_READONLY          If object is read-only.
*        SNMP_WRONGVALUE        If invalid value.
*        SNMP_NOSUCHNAME        Object does not exist.
*        SNMP_INCONSISTANTVALUE If commit fails.
*        SNMP_GENERROR          Invalid request.
*
*************************************************************************/
UINT16 IfXEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Flag to distinguish between get and get-next requests. */
    UINT8         getflag = 0;

    /* Status for returning success or error code. */
    UINT16        status = SNMP_NOERROR;

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

    case SNMP_PDU_NEXT:     /* Get next request. */

        /* Processing get operations. */
        status = Get_IfXEntry(obj, getflag);

        break;

    case SNMP_PDU_UNDO:     /* Undo request. */
    case SNMP_PDU_SET:      /* Set request. */

        /* Processing of set operations. */
        status = Set_IfXEntry(obj);

        break;

    case SNMP_PDU_BULK:     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_IfXEntry);

        break;

    case SNMP_PDU_COMMIT:   /* Commit request. */

        /* We will always have a successful set. */
        break;

    default:                /* Invalid request */
        /* Setting status to error code. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IfXEntry */

#endif /* ((INCLUDE_IF_EXT == NU_TRUE) && \
           (INCLUDE_IF_EXT_MIB == NU_TRUE)) */

#if ((INCLUDE_IF_STACK == NU_TRUE) && (INCLUDE_IF_STACK_MIB == NU_TRUE))
/*************************************************************************
* FUNCTION
*
*        Get_IfStackEntry
*
* DESCRIPTION
*
*        This function is used to get values of IfStack table entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        getflag                flag to distinguish between get
*                               and get next request.
*
* OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeded.
*        SNMP_ERROR             If error occur in processing the request.
*        SNMP_NOSUCHNAME        If object does not exist.
*
*************************************************************************/
UINT16 Get_IfStackEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Higher interface index. */
    UINT32                      higher_interface = 0;

    /* Lower interface index. */
    UINT32                      lower_interface = 0;

    /* Object Identifier sub-length. Table entry OID is of length 10
     * having higher ifIndex and lower ifIndex as indexes this results in
     * sub-length of 12.
     */
    UINT32                      sub_len = 12;

    /* Object identifier of table entry. */
    UINT32                      table_oid[10] = {1, 3, 6, 1, 2, 1, 31, 1,
                                                 2, 1};

    /* Status for returning success or error code. */
    UINT16                      status = SNMP_NOERROR;

    /* Variable for holding row status of interface stack entry. */
    UINT8                       row_status;

    /* If this is a get request, just get the instance for the Stack
       Entry. */
    if(getflag)
    {
        /* Verify that the OID length in the object is equal to the
           instance level OID for the received SNMP object. And that we
           have a valid IfIndex of higher and lower interfaces. */
        if((obj->IdLen == sub_len + 1) &&
           ((obj->Id[sub_len - 1] > 0) || (obj->Id[sub_len] > 0)))
        {
            /* Get the stack entry. */
            status = MIB2_IF_STACK_GET(obj->Id[sub_len - 1],
                                       obj->Id[sub_len]);

            if(status != NU_SUCCESS)
            {
                status = SNMP_NOSUCHNAME;
            }
            else
            {
                /* Assigning higher_interface value of index passed. */
                higher_interface = obj->Id[sub_len - 1];

                /* Assigning lower_interface value of index passes. */
                lower_interface = obj->Id[sub_len];
            }
        }
    }

    /* This is a get-next request. */
    else
    {
        higher_interface = obj->Id[sub_len - 1];
        lower_interface = obj->Id[sub_len];

        /* Invoke function responsible for getting the next stack
           entry. */
        status = MIB2_IF_STACK_GET_NEXT(higher_interface,
                                        lower_interface);

        if(status != NU_SUCCESS)
        {
            status = SNMP_NOSUCHNAME;
        }

        /* If the ifIndex of the next stack entry is less than or
         * equal to the index passed, then increment the third last
         * identifier. We do this because we have exhausted all the
         * entries for the current attribute (signified by the third last
         * identifier). We are incrementing to go to the next attribute.
         */
        if((status == SNMP_NOERROR) &&
           ((higher_interface < obj->Id[sub_len - 1]) ||
            ((higher_interface == obj->Id[sub_len - 1] ) &&
             (lower_interface <= obj->Id[sub_len]))))
        {
            obj->Id[sub_len - 2]++;

            /* Returning error code. */
            status = SNMP_NOSUCHNAME;
        }

        /* If the attribute is 0, make this attribute 1 because this is
           the next valid attribute. */
        if(obj->Id[sub_len - 2] == 0)
        {
            obj->Id[sub_len - 2] = 1;
        }

    }

    /* If we have found the stack entry then process the get-request,
       otherwise return error code. */
    if(status == SNMP_NOERROR)
    {
        switch(obj->Id[sub_len - 2])
        {
        case 3:         /* ifStackStatus */
            /* Getting row status. */
            if (MIB2_IF_STACK_ROW_STATUS_GET(higher_interface,
                               lower_interface, row_status) == NU_SUCCESS)
            {
                /* Copying row status value in SNMP object. */
                obj->Syntax.LngUns = (UINT32)row_status;
            }

            else
            {
                status = SNMP_ERROR;
            }

            break;

        default:
            /* We have reached the end of the table. */
            status = SNMP_NOSUCHNAME;
        }

        /* If the processing was successful. */
        if(status == SNMP_NOERROR)
        {
            /* Prepend the table's OID. */
            NU_BLOCK_COPY(obj->Id, table_oid,
                          (sub_len - 2) * (sizeof(UINT32)));

            /* Update the Object's OID. */
            obj->Id[sub_len - 1] = higher_interface;
            obj->Id[sub_len] = lower_interface;

            /* Update length of OID. */
            obj->IdLen = sub_len + 1;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_IfStackEntry */

/*************************************************************************
* FUNCTION
*
*        IfStackEntry
*
* DESCRIPTION
*
*        This function handles request of IfStack table.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  Length of SNMP request.
*        *param                 Additional parameters (Not used).
*
* OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeded.
*        SNMP_READONLY          If object is read-only.
*        SNMP_ERROR             if error occur in processing the request.
*        SNMP_NOSUCHNAME        Object does not exist.
*        SNMP_INCONSISTANTVALUE If commit fails.
*        SNMP_GENERROR          Invalid request.
*
*************************************************************************/
UINT16 IfStackEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status for returning success or error code. */
    UINT16                          status;

    /* Flag to distinguish between get and get-next requests. */
    UINT8                           getflag = 0;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);
    UNUSED_PARAMETER(idlen);

    /* Identifying request type and taking appropriate actions. */
    switch(obj->Request)
    {
    case SNMP_PDU_GET:          /* Get request. */
        /* Set getflag so that it can represent get-request. */
        getflag++;

        /* Fall through the next case for fulfilling the get-request. */

    case SNMP_PDU_NEXT:         /* Get-next request. */

        /* Processing get operations. */
        status = Get_IfStackEntry(obj, getflag);
        break;

    case SNMP_PDU_SET:          /* Set request. */
    case SNMP_PDU_CREATE:       /* Create request. */
    case SNMP_PDU_UNDO:         /* Undo request. */
    case SNMP_PDU_COMMIT:       /* Commit request. */

        status = SNMP_NOSUCHNAME;
        break;

    case SNMP_PDU_BULK:

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_IfStackEntry);
        break;

    default:
        status = SNMP_GENERROR;
        break;
    }

    /* Returning success or error code. */
    return (status);

} /* IfStackEntry */

#endif /* ((INCLUDE_IF_STACK == NU_TRUE) && \
           (INCLUDE_IF_STACK_MIB == NU_TRUE)) */

#if (INCLUDE_RCV_ADDR_MIB == NU_TRUE)
/*************************************************************************
* FUNCTION
*
*        Get_IfRcvAddressEntry
*
* DESCRIPTION
*
*        This function is used to get values of IfRcvAddress table
*        entries.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        getflag                flag to distinguish between get
*                               and get next request.
*
* OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeded.
*        SNMP_NOSUCHNAME        If object does not exist.
*
*************************************************************************/
UINT16 Get_IfRcvAddressEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object Identifier sub-length. Table entry OID is of length 10
     * having ifIndex of length 1 and PHY address of length
     * MIB2_MAX_PADDRSIZE as indexes this results in sub-length of
     * 11 + MIB2_MAX_PADDRSIZE.
     */
    UINT32                  sub_len = 11;

    /* Object identifier of table entry. */
    UINT32                  table_oid[10] = {1, 3, 6, 1, 2, 1, 31, 1, 4,
                                             1};

    /* Device index or interface index. */
    UINT32                  if_index = 0;

    /* Variable for loop. */
    UINT32                  loop;

    /* Status for returning success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* PHY Receive address. */
    UINT8                   phy_rcv_address[MIB2_MAX_PADDRSIZE];

    /* If this is a get request, just get the instance for the Stack
       Entry. */
    if(getflag)
    {
        /* Verify that the OID length in the object is equal to the
           instance level OID for the received SNMP object. */
        if((obj->IdLen == sub_len + 1 + MIB2_MAX_PADDRSIZE) &&
           (obj->Id[sub_len] > 0))
        {
            /* Getting if_index. */
            if_index = obj->Id[sub_len];

            /* Getting Physical Address value. */
            for(loop = 0; loop < MIB2_MAX_PADDRSIZE; ++loop)
            {
                phy_rcv_address[loop] = 
                    (UINT8)obj->Id[(sub_len + 1) + loop];
            }

            /* Check the existence of RCV address. */
            if (MIB2_RCV_ADDR_GET((if_index - 1), phy_rcv_address)
                                                            != NU_SUCCESS)
            {
                status = SNMP_NOSUCHNAME;
            }
        }

        /* If we have invalid OID then return error code. */
        else
        {
            /* Returning error code. */
            status = SNMP_NOSUCHNAME;
        }
    }
    /* This is a get-next request. */
    else
    {
        /* Copying interface index value. */
        if_index = obj->Id[sub_len];

        /* Getting Physical Address value. */
        for(loop = 0; loop < MIB2_MAX_PADDRSIZE; ++loop)
        {
            phy_rcv_address[loop] = (UINT8)obj->Id[sub_len + 1 + loop];
        }

        /* Getting if_index and physical address for the next device. */
        if(MIB2_RCV_ADDR_GET_NEXT(if_index, phy_rcv_address) ==
                                                        MIB2_UNSUCCESSFUL)
        {
            /* Returning error code if operation failed. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* If we have found the address entry then process the get-request
       otherwise return error code. */
    if(status == SNMP_NOERROR)
    {
        switch(obj->Id[sub_len - 1])
        {
        case 2:         /* ifRcvAddressStatus */

            /* Returning row status as 'ACTIVE'. */
            obj->Syntax.LngUns = SNMP_ROW_ACTIVE;

            break;

        case 3:         /* ifRcvAddressType */

            /* Returning storage type as 'VOLATILE'. */
            obj->Syntax.LngUns = SNMP_STORAGE_VOLATILE;

            break;

        default:
            /* We have reached the end of the table. */
            status = SNMP_NOSUCHNAME;
        }

        /* If the processing was successful. */
        if(status == SNMP_NOERROR)
        {
            /* Prepend the table's OID. */
            NU_BLOCK_COPY(obj->Id, table_oid, 
                ((sub_len - 1) * sizeof(UINT32)));

            /* Update the Object's OID. */
            obj->Id[sub_len] = if_index;


            /* Copying Physical Address. */
            for(loop = 0; loop < MIB2_MAX_PADDRSIZE; ++loop)
            {
                obj->Id[sub_len + 1 + loop] = (UINT32)phy_rcv_address[loop];
            }

            /* Update length of OID. */
            obj->IdLen = sub_len + 1 + MIB2_MAX_PADDRSIZE;
        }
    }

    /* Returning success or error code. */
    return (status);

} /* Get_IfRcvAddressEntry */

/*************************************************************************
* FUNCTION
*
*        IfRcvAddressEntry
*
* DESCRIPTION
*
*        This function handles request of IfStack table.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  length of SNMP request.
*        *param                 Additional parameters (Not used).
*
* OUTPUTS
*
*        SNMP_NOERROR           If the processing succeeded.
*        SNMP_READONLY          If object is read-only.
*        SNMP_ERROR             if invalid value.
*        SNMP_NOSUCHNAME        Object does not exist.
*        SNMP_INCONSISTANTVALUE If commit fails.
*        SNMP_GENERROR          Invalid request.
*
*************************************************************************/
UINT16 IfRcvAddressEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
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
    case SNMP_PDU_GET:      /* Get request. */
        /* Set getflag so that it can represent get-request. */
        getflag++;

        /* Fall through the next case for fulfilling the get-request. */

    case SNMP_PDU_NEXT:     /* Get-next request. */

        /* Processing get operations. */
        status = Get_IfRcvAddressEntry(obj, getflag);

        break;

    case SNMP_PDU_SET:
    case SNMP_PDU_UNDO:
    case SNMP_PDU_COMMIT:

        status = SNMP_NOSUCHNAME;

        break;

    case SNMP_PDU_BULK:     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_IfRcvAddressEntry);

        break;

    default:
        status = SNMP_GENERROR;
    }

    /* Returning success or error code. */
    return (status);

} /* IfRcvAddressEntry */

#endif /* (INCLUDE_RCV_ADDR_MIB == NU_TRUE) */

#endif /* ( (RFC1213_IF_INCLUDE == NU_TRUE) && \
            (MIB2_IF_INCLUDE == NU_TRUE) ) */


