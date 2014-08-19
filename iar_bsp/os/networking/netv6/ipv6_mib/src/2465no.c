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
*        2465no.c                                    
*
*   DESCRIPTION
*
*        This file contains the implementation of the function that is
*        responsible for IPv6 notifications.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        IP6_Send_Notification
*
*   DEPENDENCIES
*
*        nu_net.h
*        snmp_api.h
*        ip6_mib_s.h
*        ip6_mib_no_s.h
*
************************************************************************/

#include "networking/nu_net.h"

#if ( (INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE) )
#include "networking/snmp_api.h"
#include "networking/ip6_mib_s.h"
#include "networking/2465no.h"

/************************************************************************
*
*   FUNCTION
*
*        IP6_Send_Notification
*
*   DESCRIPTION
*
*        This function is responsible for sending 'ipv6IfStateChange'
*        notification trap.
*
*   INPUTS
*
*        *dev                   Handle to the interface device.
*
*   OUTPUTS
*
*        None.
*
************************************************************************/
VOID IP6_Send_Notification(DV_DEVICE_ENTRY *dev)
{
    /* Handle to the notification request structure. */
    SNMP_NOTIFY_REQ_STRUCT  *snmp_notification;

    /* OID of 'ipv6IfStateChange'. */
    UINT32                  ipv6_if_state_change_oid[] =
                                IPV6_IF_STATE_CHANGE_OID;

    /* OID of 'ipv6IfDescr'. */
    UINT32                  ipv6_if_descr_oid[] = 
                                IPV6_IF_DESCR_OID;

    /* OID of 'ipv6IfOperStatus'. */
    UINT32                  ipv6_if_oper_status_oid[] =
                                IPV6_IF_OPER_STATUS_OID;

    /* If we successfully got the handle to the notification. */
    if (SNMP_Get_Notification_Ptr(&snmp_notification) == NU_SUCCESS)
    {
        /* Clear out the notification structure. */
        UTL_Zero(snmp_notification, sizeof(SNMP_NOTIFY_REQ_STRUCT));

        /* Update the notification OID. */
        NU_BLOCK_COPY(snmp_notification->OID.notification_oid,
                      ipv6_if_state_change_oid,
                      sizeof(ipv6_if_state_change_oid));

        /* Update the notification OID length. */
        snmp_notification->OID.oid_len = IPV6_IF_STATE_CHANGE_OID_LEN;

        /* Set first bound object as 'ipv6IfDescr'. */
        NU_BLOCK_COPY(snmp_notification->snmp_object_list[0].Id,
                        ipv6_if_descr_oid, sizeof(ipv6_if_descr_oid));

        /* Set interface index in OID. */
        snmp_notification->snmp_object_list[0].
            Id[(IPV6_IF_DESCR_OID_LEN - 1)] = (dev->dev_index + 1);

        /* Set OID length. */
        snmp_notification->snmp_object_list[0].IdLen =
            IPV6_IF_DESCR_OID_LEN;

        /* Set request type. */
        snmp_notification->snmp_object_list[0].Request = SNMP_PDU_GET;

        /* Update the syntax length. */
        snmp_notification->snmp_object_list[0].SyntaxLen =
            strlen(dev->ip6_interface_mib->ip6_if_desc);

        /* Update the value for 'ipv6IfDescr'. */
        NU_BLOCK_COPY(snmp_notification->snmp_object_list[0].Syntax.BufChr,
                      dev->ip6_interface_mib->ip6_if_desc,
                      snmp_notification->snmp_object_list[0].SyntaxLen);

        /* Set object type. */
        snmp_notification->snmp_object_list[0].Type = SNMP_OCTETSTR;

        /* Set second bound object as 'ipv6IfOperStatus'. */
        NU_BLOCK_COPY(snmp_notification->snmp_object_list[1].Id,
                      ipv6_if_oper_status_oid,
                      sizeof(ipv6_if_oper_status_oid));

        /* Update the interface index in the OID. */
        snmp_notification->snmp_object_list[1].
            Id[(IPV6_IF_OPER_STATUS_OID_LEN - 1)] = (dev->dev_index  + 1);

        /* Set OID length. */
        snmp_notification->snmp_object_list[1].IdLen =
            IPV6_IF_OPER_STATUS_OID_LEN;

        /* Set request type. */
        snmp_notification->snmp_object_list[1].Request = SNMP_PDU_GET;

        /* Set the value for 'ipv6IfOperStatus'. */
        snmp_notification->snmp_object_list[1].Syntax.LngUns =
            (((UINT32)(dev->dev_mibInterface.statusOper)) & 0xFFFF);

        /* Set the object type. */
        snmp_notification->snmp_object_list[1].Type = SNMP_INTEGER;

        /* Update the number of bound objects. */
        snmp_notification->snmp_object_list_len = 2;

        /* Send the notification. */
        if (SNMP_Notification_Ready(snmp_notification) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to send SNMP notification.",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to get notification handle",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
}
#endif /* ((INCLUDE_SNMP == NU_TRUE) &&
           (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE)) */
