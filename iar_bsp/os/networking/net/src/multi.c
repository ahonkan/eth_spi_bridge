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
*
*   FILE NAME
*
*       multi.c
*
*   COMPONENT
*
*       Multicast messages:  Both IGMP and MLD
*
*   DESCRIPTION
*
*       This file contains the IP-agnostic module for handling of
*       multicast messages.  This module was created due to the
*       overlap between IGMP (Internet Group Management Protocol) and
*       MLD (Multicast Listener Discovery).
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       Multi_Init
*       Multi_Input
*       Multi_Output
*       Multi_Update_Entry
*       Multi_Timer_Expired
*       Multi_Calculate_Code_Value
*       Multi_Update_Device_State
*       Multi_Verify_Src_By_Filter
*       Multi_Get_Device_State_Struct
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/mld6.h"
#endif

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

#if (INCLUDE_IPV4 == NU_TRUE)
extern UNSIGNED    Multicast_IGMPv1_Timer_Index;

#if ((IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV2_COMPATIBILITY) ||    \
     (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) )
extern UNSIGNED    Multicast_IGMPv2_Timer_Index;
extern TQ_EVENT    IGMPv1_Compatibility_Timer_Event;

#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
extern UNSIGNED    Multicast_IGMPv3_Mode_Include_Timer_Index;
extern UNSIGNED    Multicast_IGMPv3_Mode_Exclude_Timer_Index;
extern UNSIGNED    Multicast_IGMPv3_Change_To_Include_Timer_Index;
extern UNSIGNED    Multicast_IGMPv3_Change_To_Exclude_Timer_Index;
extern UNSIGNED    Multicast_IGMPv3_Allow_New_Sources_Timer_Index;
extern UNSIGNED    Multicast_IGMPv3_Block_Old_Sources_Timer_Index;
extern TQ_EVENT    IGMPv2_Compatibility_Timer_Event;

#endif
#endif

#endif

#if (INCLUDE_IPV6 == NU_TRUE)

extern UNSIGNED    Multicast_MLDv1_Timer_Index;

#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
extern UNSIGNED    Multicast_MLDv2_Mode_Include_Timer_Index;
extern UNSIGNED    Multicast_MLDv2_Mode_Exclude_Timer_Index;
extern UNSIGNED    Multicast_MLDv2_Change_To_Include_Timer_Index;
extern UNSIGNED    Multicast_MLDv2_Change_To_Exclude_Timer_Index;
extern UNSIGNED    Multicast_MLDv2_Allow_New_Sources_Timer_Index;
extern UNSIGNED    Multicast_MLDv2_Block_Old_Sources_Timer_Index;
extern TQ_EVENT    MLDv1_Compatibility_Timer_Event;
#endif
#endif

TQ_EVENT    Multicast_General_Query_Timer_Event;
TQ_EVENT    Multicast_Address_Specific_Timer_Event;

#if (INCLUDE_IPV6 == NU_TRUE)
static  TQ_EVENT    MLD6_Send_Startup_Report_Event;

extern UINT8        IP6_All_Nodes_Multi[];
extern UINT8        IP6_All_Hosts_Multi[];
extern UINT8        IP6_MLDv2_All_Routers_Multi[];

#endif

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Init
*
*   DESCRIPTION
*
*       This function initializes the events that will be used for
*       Multicast Listener Discovery.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID Multi_Init(VOID)
{
    /* Register the event to handle an expired a multicast timers */
    if (EQ_Register_Event(Multi_Timer_Expired,
                          &Multicast_General_Query_Timer_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register multicast event", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    if (EQ_Register_Event(Multi_Timer_Expired,
                          &Multicast_Address_Specific_Timer_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register multicast event", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

#if (INCLUDE_IPV4 == NU_TRUE)

#if ((IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV2_COMPATIBILITY) ||    \
     (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) )

    /* Register the event to handle an expired compatibility timers */
    if (EQ_Register_Event(Multi_Timer_Expired,
                          &IGMPv1_Compatibility_Timer_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register IGMPv1 Compatibility event",
                        NERR_SEVERE, __FILE__, __LINE__);
    }

#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)

    if (EQ_Register_Event(Multi_Timer_Expired,
                          &IGMPv2_Compatibility_Timer_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register IGMPv2 Compatibility event",
                        NERR_SEVERE, __FILE__, __LINE__);
    }

#endif
#endif

#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    /* Register the event to send a startup report */
    if (EQ_Register_Event(MLD6_Send_Startup_Report,
                          &MLD6_Send_Startup_Report_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register MLD event", NERR_SEVERE,
                 __FILE__, __LINE__);
    }

#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)

    if (EQ_Register_Event(Multi_Timer_Expired,
                          &MLDv1_Compatibility_Timer_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register MLDv1 Compatibility event",
                        NERR_SEVERE, __FILE__, __LINE__);
    }

#endif
#endif

} /* Multi_Init */

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Input
*
*   DESCRIPTION
*
*       This function processes an incoming multicast message accordingly.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the received packet.
*       *device                 A pointer to the device on which the
*                               message was received.
*       multicast_addr_offset   Offset into the buffer for the multicast
*                               address.
*       max_resp_delay          The max response delay that will be used
*                               in scheduling our response.
*
*   OUTPUTS
*
*       0                       Success.
*       -1                      Invalid input.
*
*************************************************************************/
STATUS Multi_Input(const NET_BUFFER *buf_ptr, const DV_DEVICE_ENTRY *device,
                   UNSIGNED multicast_addr_offset)
{
    UINT8       *multicast_address;
    MULTI_IP    *multi_ip = NU_NULL;
    UINT8       protocol_mode = 0;
    STATUS      ret_status = NU_SUCCESS;
#if (INCLUDE_IPV4 == NU_TRUE)
    UINT32      multi_addr32;
#endif
    UINT32      max_resp_delay;

    /* Extract the multicast address from the message */
    multicast_address = (UINT8 *)(&buf_ptr->data_ptr[multicast_addr_offset]);

#if (INCLUDE_IPV4 == NU_TRUE)

    /* Search for a matching entry on the interface */
    if (multicast_addr_offset == IGMP_GROUP_OFFSET)
    {
        multi_ip = (MULTI_IP *)IP_Lookup_Multi(GET32(multicast_address, 0),
                                               &device->dev_addr);

        /* If the returned value is null, then this is most likely a
         * general query.  Set the variable to the first multicast group
         * of the device.
         */
        if (!(multi_ip))
            multi_ip = (MULTI_IP *)device->dev_addr.dev_multiaddrs;

        protocol_mode = MULTICAST_TYPE_IGMP;

        /* Determine the version of the querier or host that sent the
           message.  We may have to change our compatibility mode based
           on this message. */
#if ( (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV2_COMPATIBILITY) ||   \
      (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) )
        IGMP_Update_Host_Compatibility(buf_ptr, multi_ip);
#endif

        /* Calculate the max response delay using the max response code
         * from the received query
         */
        max_resp_delay = Multi_Calculate_Code_Value(multi_ip,
        										    GET8(buf_ptr->data_ptr, IGMP_MAX_RESP_CODE_OFFSET)) *
        										    (SCK_Ticks_Per_Second / 10);
    }

#if (INCLUDE_IPV6 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    if (multicast_addr_offset == MLD6_MULTI_ADDR_OFFSET)
    {
        multi_ip =
            (MULTI_IP *)IP6_Lookup_Multi(multicast_address,
                                         device->dev6_multiaddrs);

        /* If the returned value is null, then this is most likely a
         *  general query.  Set the variable to the first multicast group
         *  of the device.
         */
        if (!(multi_ip))
            multi_ip = (MULTI_IP *)device->dev6_multiaddrs;

        protocol_mode = MULTICAST_TYPE_MLD;

        /* Determine the version of the querier or host that sent the
           message.  We may have to change our compatibility mode based
           on this message. */
#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
        MLD6_Update_Host_Compatibility(buf_ptr, multi_ip);
#else
        multi_ip->multi_data.multi_device->dev6_mld_compat_mode =
                                        MLDV1_COMPATIBILITY;
#endif

        /* Calculate the max response delay */
        max_resp_delay = MLD6_Calculate_Max_Response_Delay(GET16(buf_ptr->data_ptr,
                                                    	   MLD6_MAX_RESP_OFFSET));
    }

#endif

    else
    {
        NLOG_Error_Log("Unrecognized multicast_addr_offset",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (-1);
    }

    /* Process the received multicast packet */
    switch (buf_ptr->data_ptr[MULTICAST_MESSAGE_TYPE_OFFSET])
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        case IGMP_HOST_MEMBERSHIP_QUERY :
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        case MLD6_LISTENER_QUERY:
#endif
            /* If a matching entry was found and the Query Type is
             * Multicast-Address-Specific, update the corresponding
             * multicast address data structure.
             */
            if (
#if (INCLUDE_IPV4 == NU_TRUE)
                (IP_MULTICAST_ADDR(IP_ADDR(multicast_address)))
#if (INCLUDE_IPV6 == NU_TRUE)
                ||
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                 (IPV6_IS_ADDR_MULTICAST(multicast_address))
#endif
                     )

                Multi_Update_Entry(buf_ptr, multi_ip, protocol_mode,
                                   MULTICAST_ADDRESS_SPECIFIC_QUERY,
                                   max_resp_delay);

            else if (
#if (INCLUDE_IPV4 == NU_TRUE)
                ( (IP_MULTICAST_ADDR(IP_ADDR(multicast_address))) &&
                (buf_ptr->data_ptr[IGMP_NUMBER_OF_SOURCE_ADDRESSES] != 0) )
#if (INCLUDE_IPV6 == NU_TRUE)
                ||
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                ( (IPV6_IS_ADDR_MULTICAST(multicast_address)) &&
                (buf_ptr->data_ptr[MLD6_NUMBER_OF_SOURCES] != 0) )
#endif
                             )

                Multi_Update_Entry(buf_ptr, multi_ip, protocol_mode,
                                   MULTICAST_ADDRESS_AND_SOURCE_SPECIFIC_QUERY,
                                   max_resp_delay);

            /* Otherwise, the Query Type is GENERAL.  Loop through all
             * multicast groups.
             */
            else if (
#if (INCLUDE_IPV4 == NU_TRUE)
                (GET32(multicast_address, 0) == 0)
#if (INCLUDE_IPV6 == NU_TRUE)
                ||
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                (IPV6_IS_ADDR_UNSPECIFIED(multicast_address))
#endif
                        )
            {
                /* Get a pointer to the first multicast address in the
                 * list
                 */
#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
                if (protocol_mode == MULTICAST_TYPE_IGMP)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
                    multi_ip = (MULTI_IP *)device->dev_multi_addr;

#if (INCLUDE_IPV6 == NU_TRUE)
                else if (protocol_mode == MULTICAST_TYPE_MLD)
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                    multi_ip = (MULTI_IP *)device->dev6_multiaddrs;
#endif

                /* Update each multicast data structure accordingly */
                while (multi_ip != NU_NULL)
                {
#if (INCLUDE_IPV4 == NU_TRUE)
                    /* Convert the multicast address to UINT32 */
                    multi_addr32 = GET32(multi_ip->multi_addr, 0);
#endif

                    /* If the multicast address is one that we should
                     *  send a response for, update it.
                     */

                    if (
#if (INCLUDE_IPV4 == NU_TRUE)
                        ( (multi_addr32 != IGMP_ALL_HOSTS_GROUP) &&
                          (multi_addr32 != IGMP_ALL_ROUTERS_GROUP) &&
                          (multi_addr32 != IGMPV3_ALL_ROUTERS_GROUP) )
#if (INCLUDE_IPV6 == NU_TRUE)
                             &&
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                       ( (memcmp(multi_ip->multi_addr, IP6_All_Nodes_Multi,
                                    IP6_ADDR_LEN) != 0) &&
                         (memcmp(multi_ip->multi_addr, IP6_All_Hosts_Multi,
                                    IP6_ADDR_LEN) != 0) )
#endif
                                )
                    {
                        Multi_Update_Entry(buf_ptr, multi_ip,
                                           protocol_mode,
                                           MULTICAST_GENERAL_QUERY,
                                           max_resp_delay);
                    }

                    /* Get a pointer to the next Multicast address in the
                     * list
                     */
                    multi_ip = multi_ip->multi_next;
                } /* while (multi_ip != NU_NULL) */
            } /* else if (... */

            break;
#if (INCLUDE_IPV4 == NU_TRUE)
        case IGMPV1_HOST_MEMBERSHIP_REPORT :
        case IGMPV2_HOST_MEMBERSHIP_REPORT :
        case IGMPV3_HOST_MEMBERSHIP_REPORT :
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        case MLDV1_LISTENER_REPORT:
        case MLDV2_LISTENER_REPORT:
#endif

            /* If the interface is a member of the multicast group and the
             * current MLD state is DELAYING_LISTENER, process the Report.
             */
            if (multi_ip->multi_data.multi_state == DELAYING_LISTENER)
            {
                /* Unset the previous timer for the multicast group */
                Multi_Unset_Timer(Multicast_Address_Specific_Timer_Event,
                                  TQ_CLEAR_EXACT,
                                  (UNSIGNED)(multi_ip->multi_data.
                                  multi_device->dev_index),
                                  multi_ip->multi_data.multi_timer_index,
                                  &multi_ip->multi_data);

                /* Since we are unsure if this report is in response
                 * to a general query or a address-specific query,
                 * walk both lists to ensure that we have unset the timer
                 */
                Multi_Unset_Timer(Multicast_General_Query_Timer_Event,
                                  TQ_CLEAR_EXACT,
                                  (UNSIGNED)(multi_ip->multi_data.
                                  multi_device->dev_index),
                                  multi_ip->multi_data.multi_timer_index,
                                  &multi_ip->multi_data);

                /* Set the state of MLD to IDLE_LISTENER */
                multi_ip->multi_data.multi_state = IDLE_LISTENER;
            }

            break;

        /* Ignore unrecognized multicast messages in order to maintain
         * backward compatibility.
         */
        default:

            NLOG_Error_Log("Unrecognized multicast message received",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            break;

    } /* switch (message_rec) */

    return (ret_status);

} /* Multi_Input */

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Output
*
*   DESCRIPTION
*
*       This function handles the calling of the proper multicast
*       message sending functions.
*
*   INPUTS
*
*       *multi_ptr              A pointer to either the IP or IPv6
*                               Multicast data structure.
*       protocol_type           The protocol of the message to send:
*                               MLD or IGMP.
*       message_type            The type of message that will be sent.
*       max_response_delay      The Maximum Response Delay field is
*                               meaningful only in Query messages, and
*                               specifies the maximum allowed delay
*                               before sending a responding Report, in
*                               units of milliseconds.  In all other
*                               messages, it is set to zero by the
*                               sender and ignored by receivers.
*
*   OUTPUTS
*
*       status                  Returning status from the transmitting
*                                   functions that are called.
*
*************************************************************************/
STATUS Multi_Output(VOID *multi_ptr, UNSIGNED protocol_type,
                    UINT8 message_type, UINT16 max_response_delay)
{
    STATUS              status;

#if (INCLUDE_IPV4 == NU_TRUE)
    IP_MULTI            *ip_multi;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    IP6_MULTI           *ip6_multi;
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
    /* Setup the local variables based on the passed in variables */
    if (protocol_type == MULTICAST_TYPE_IGMP)
#else
    UNUSED_PARAMETER(protocol_type);
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* Get the IP_MULTI ptr from the passed in variable */
        ip_multi = (IP_MULTI *)multi_ptr;

        /* Call the proper output routine */
        if ( (ip_multi->ipm_data.multi_device->dev_igmp_compat_mode ==
              IGMPV1_COMPATIBILITY) ||
             (ip_multi->ipm_data.multi_device->dev_igmp_compat_mode ==
              IGMPV2_COMPATIBILITY) )
            status = IGMP_Send(ip_multi, GET32(ip_multi->ipm_addr, 0),
                               message_type, (UINT8)max_response_delay);

#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
        else
            status = IGMP_Send(ip_multi, IGMPV3_ALL_ROUTERS_GROUP,
                               message_type, (UINT8)max_response_delay);
#endif

        if (status != NU_SUCCESS)
            NLOG_Error_Log("IGMP message could not be sent",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

#if (INCLUDE_IPV6 == NU_TRUE)
    else if (protocol_type == MULTICAST_TYPE_MLD)
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    {
        /* Get the IP6_MULTI ptr from the passed in variable */
        ip6_multi = (IP6_MULTI *)multi_ptr;

        /* Call the proper output routine */
        if (ip6_multi->ipm6_data.multi_device->dev6_mld_compat_mode ==
            MLDV1_COMPATIBILITY)
        {
            /* Send the message to the multicast address */
            status = MLD6_Output(ip6_multi, message_type,
                                 ip6_multi->ipm6_addr, max_response_delay);
        }

        else
        {
            /* Send message to the MLDv2 All-Router's address */
            status = MLD6_Output(ip6_multi, message_type,
                                 IP6_MLDv2_All_Routers_Multi, max_response_delay);
        }

        if (status != NU_SUCCESS)
            NLOG_Error_Log("MLD message could not be sent",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
        status = -1;
#endif
#endif

    return (status);

} /* Multi_Output */

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Update_Entry
*
*   DESCRIPTION
*
*       RFC 2710 Section 4
*
*       When a node receives a General Query, it sets a delay timer
*       for each multicast address to which it is listening on the
*       interface from which it received the Query, excluding the
*       link-scope all-nodes address and any multicast addresses
*       of scope 0 or 1.  Each timer is set to a different random
*       value.  If a timer for any address is already running,
*       it is reset to the new random value only if the requested
*       Maximum Response Delay is less than the remaining value of
*       the running timer.  If the Query Packet specifies a Maximum
*       Response Delay of zero, each timer is set to zero and timer
*       expiration is performed immediately.
*
*       When a node receives a Multicast-Address Specific Query, if
*       it is listening to the queried Multicast Address on the
*       interface from which the Query was received, it sets a delay
*       timer for that address to a random value.  If a timer for
*       the address is already running, it is reset to the new
*       random value only if the requested Maximum Response Delay
*       is less than the remaining value of the running timer.  If
*       the Query packet specifies a Maximum Response Delay of zero,
*       the timer is set to zero and timer expiration is performed
*       immediately.
*
*   STATE DIAGRAM
*                               ----------------
*                              |                |
*                              |                |
*                              |                |
*                              |                |
*                   ---------->|  Non-Listener  |<----------
*                  |           |                |           |
*                  |           |                |           |
*                  |           |                |           |
*                  |           |                |           |
*                  |            ----------------            |
*                  |stop listening      |start listening    |stop listening
*                  |(stop timer,        |(send report,      |(send done if
*                  | send done if       | set flag,         | flag set)
*                  | flag set)          | start timer)      |
*                  |                    |                   |
*          ---------------- <-----------             ----------------
*         |                |                        |                |
*         |                |<-----------------------|                |
*         |                |     query received     |                |
*         |                |     (start timer)      |                |
*    ---->|    Delaying    |----------------------->|      Idle      |
*   |     |    Listener    |     report received    |    Listener    |
*   |     |                |     (stop timer,       |                |
*   |     |                |      clear flag)       |                |
*   |     |                |----------------------->|                |
*   |      ----------------      timer expired       ----------------
*   |query received|             (send report,
*   |(reset timer  |              set flag)
*   | if Max Resp  |
*   | Delay < timer|
*    --------------
*
*   INPUTS
*
*       *buf_ptr                A ptr to the received multicast message
*       *multicast_struct       A ptr to either the IGMP or MLD multicast
*                               struct.
*       protocol_type           Flag used to determine if this is an IGMP
*                               or MLD message
*       query_type              Type of query that was received (General,
*                               Group-Specific, or
*                               Group-And-Source-Specific query)
*       max_resp_delay          The calculated maximum response delay.
*                               This value will be used to set timers
*                               for our responses to the query
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Multi_Update_Entry(const NET_BUFFER *buf_ptr, VOID *multicast_struct,
                        UINT8 protocol_type, UINT8 query_type,
                        UINT32 max_resp_delay)
{
    MULTI_DATA              *multi;
    STATUS                  status = NU_SUCCESS;
    DV_DEVICE_ENTRY         *device;
    UINT8                   addr_len;
    MULTI_DEV_STATE         *dev_state;
    MULTI_IP                *multi_ip;
    UNSIGNED                time_diff;

#if ( ((INCLUDE_IPV4 == NU_TRUE) && \
       (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)) || \
      ((INCLUDE_IPV6 == NU_TRUE) && \
       (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)) )
    UINT16                  num_src_in_query, i, j;
    UINT8                   multi_addr_offset = 0, num_src_addr_offset = 0;

    /* These two variables are initialized and "used" to get rid of
     * compiler warnings for Real View tools.
     */
    UNUSED_PARAMETER(multi_addr_offset);
    UNUSED_PARAMETER(num_src_addr_offset);
#endif

#if ( (IGMP_DEFAULT_COMPATIBILTY_MODE != IGMPV3_COMPATIBILITY) || \
    ((INCLUDE_IPV6 == NU_TRUE) && \
    (MLD6_DEFAULT_COMPATIBILTY_MODE != MLDV2_COMPATIBILITY)) )

    UNUSED_PARAMETER (buf_ptr);
#endif

    /* Set the local variables to the proper timer depending upon the
       protocol of the query. */
#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
    if (protocol_type == MULTICAST_TYPE_IGMP)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
    {
        addr_len = IP_ADDR_LEN;
#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
        multi_addr_offset = IGMP_SOURCE_ADDRESS;
        num_src_addr_offset = IGMP_NUMBER_OF_SOURCE_ADDRESSES;
#endif
    }
#if (INCLUDE_IPV6 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    {
        addr_len = IP6_ADDR_LEN;
#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
        multi_addr_offset = MLD6_SOURCE_ADDRESSES;
        num_src_addr_offset = MLD6_NUMBER_OF_SOURCES;
#endif
    }
#endif

    /* Cast the passed-in ptr to the MULTI_IP struct */
    multi_ip = (MULTI_IP *)multicast_struct;

    /* Get a ptr to the multicast data */
    multi = &multi_ip->multi_data;

    /* Get a ptr to the device struct */
    device = multi->multi_device;

    /* Get a pointer to the device's state structure. */
    dev_state =
        Multi_Get_Device_State_Struct(device, multi_ip->multi_addr,
                                      addr_len);

    /* Calculate how many ticks until the next timer is set to expire. */
    time_diff = TQ_Check_Duetime(multi->multi_set_time + multi->multi_timer);

    /* If the timer has already expired, inflate time_diff. */
    if (!time_diff)
        time_diff = 0xffffffff;

    /* The received queries will have responses scheduled depending upon
       the following:  */

    if (query_type == MULTICAST_GENERAL_QUERY)
    {
        if (multi->multi_state == DELAYING_LISTENER)
        {
            /* If there is a pending response to a previous General Query
               scheduled sooner than the selected delay, no additional
               response needs to be scheduled. */
            if (max_resp_delay > time_diff)
                status = NU_NO_ACTION;

            /* Unset the previous timer */
            else
            {
                Multi_Unset_Timer(Multicast_General_Query_Timer_Event,
                                  TQ_CLEAR_EXACT,
                                  (UNSIGNED)(device->dev_index),
                                  (UNSIGNED)multi->multi_timer_index, multi);
            }
        }

        /* Otherwise, the state is currently IDLE_LISTENER,
         * change it to DELAYING_LISTENER.
         */
        else if (multi->multi_state == IDLE_LISTENER)
            multi->multi_state = DELAYING_LISTENER;

        else
            status = NU_NO_ACTION;

        /* If the received Query is a General Query, the General Query timer
           is used to schedule a response to the General Query after
           the selected delay.  Any previously pending response to a
           General Query is canceled. */
        if (status == NU_SUCCESS)
        {
            /* Calculate a random delay using the max_resp_delay */
            multi->multi_timer = MULTI_RANDOM_DELAY(max_resp_delay);

            /* Get the proper timer index to use */
            multi->multi_timer_index =
                Multi_Get_Timer_Index(device, dev_state, protocol_type, 0);

            /* Set the timer to the new value */
            Multi_Set_Timer(Multicast_General_Query_Timer_Event,
                            (UNSIGNED)(device->dev_index), multi->multi_timer,
                            (UNSIGNED)(multi->multi_timer_index), multi);
        }
    }

#if ( ((INCLUDE_IPV4 == NU_TRUE) && \
       (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)) || \
      ((INCLUDE_IPV6 == NU_TRUE) && \
       (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)) )

    /* If the received Query is a Multicast Address-Specific Query
     * or a Multicast Address-and-Source-Specific Query and there is
     * no pending response to a previous Query for this group, then
     * the Address-Specific timer is used to schedule a report.  If
     * the received Query is a Source-Specific query, then the list of
     * queried sources is recorded to be used when generating a response.
     */
    else if ( ((query_type == MULTICAST_ADDRESS_SPECIFIC_QUERY) ||
               (query_type == MULTICAST_ADDRESS_AND_SOURCE_SPECIFIC_QUERY)) &&
              (multi->multi_state == IDLE_LISTENER) )
    {
        /* Change state to delaying */
        multi->multi_state = DELAYING_LISTENER;

        /* Calculate a random delay using the max_resp_delay */
        multi->multi_timer = MULTI_RANDOM_DELAY(max_resp_delay);

        /* Get the proper timer index to use */
        multi->multi_timer_index =
            Multi_Get_Timer_Index(device, dev_state, protocol_type, 0);

        /* If this query is Source-Specific, then allocate a IP_MULTI or
         * IP6_MULTI structure to store off the list of queried sources.
         */
        if ( (query_type == MULTICAST_ADDRESS_AND_SOURCE_SPECIFIC_QUERY) &&
             (dev_state != NU_NULL) )
        {
            /* Set the multi flag to show that there is a report pending
             * for sources
             */
            multi->multi_source_addr_pend_report = NU_TRUE;

            /* Get the number of src addresses in the query from the
             * received message.
             */
            multi_ip->multi_num_query_src_list =
                GET16(buf_ptr->data_ptr, num_src_addr_offset);

            /* Search through the query's source list to see if there is
             * a match.  If there is a match, then copy the entire source
             * list into the multi structure.  We will determine exactly
             * which addresses to respond to later.
             */
            for (i = 0; i < multi_ip->multi_num_query_src_list; i++)
            {
                /* Call the proper function to verify if we are receiving
                 * messages sent from the source.
                 */
                if (Multi_Verify_Src_By_Filter((UINT8 *)(&buf_ptr->data_ptr[multi_addr_offset +
                                               (i * addr_len)]), dev_state,
                                               MULTICAST_VERIFY_DEVICE,
                                               addr_len) == MULTICAST_ACCEPT_SOURCE)
                {
                    /* Now copy all of the source addresses into the pending
                     * report structure
                     */
                    memcpy(multi_ip->multi_query_src_list,
                           &buf_ptr->data_ptr[multi_addr_offset],
                           (multi_ip->multi_num_query_src_list * addr_len));
                }

                else
                {
                    /* If we are not accepting any messages from any of the
                     * sources listed in the query, then there is no need to
                     * schedule a response.
                     */
                    status = NU_NO_ACTION;
                    break;
                }
            }
        }

        if (status != NU_NO_ACTION)
        {
            /* Set the timer to the new value */
            Multi_Set_Timer(Multicast_Address_Specific_Timer_Event,
                            (UNSIGNED)(multi->multi_device->dev_index),
                            multi->multi_timer,
                            (UNSIGNED)(multi->multi_timer_index), multi);
        }
    }

    /* If there already is a pending response to a previous Query
     * scheduled for this group, and either the new Query is a Multicast
     * Address-Specific Query or the recorded source-list associated with
     * the group is empty, then the group source-list is cleared and a
     * single response is scheduled using the Multicast Address timer.
     * The new response is scheduled to be sent at the earliest of the
     * remaining time for the pending report and the selected delay.
     */
    else if ( (multi->multi_state == DELAYING_LISTENER) &&
              ( (query_type == MULTICAST_ADDRESS_SPECIFIC_QUERY) ||
                (multi->multi_source_addr_pend_report == NU_FALSE) ) )
    {
        /* Determine if the new response delay is shorter than the current
         * timer value and set the timer to the lower value
         */
        if (max_resp_delay < time_diff)
        {
            /* Calculate a random delay using the max_resp_delay */
            multi->multi_timer = MULTI_RANDOM_DELAY(max_resp_delay);

            /* Unset the currently running timer for this multicast group */
            Multi_Unset_Timer(Multicast_General_Query_Timer_Event,
                              TQ_CLEAR_EXACT,
                              (UNSIGNED)(multi->multi_device->dev_index),
                              (UNSIGNED)multi->multi_timer_index, multi);

            Multi_Unset_Timer(Multicast_Address_Specific_Timer_Event,
                              TQ_CLEAR_EXACT,
                              (UNSIGNED)(multi->multi_device->dev_index),
                              (UNSIGNED)multi->multi_timer_index, multi);

            /* Set the timer to the new value */
            Multi_Set_Timer(Multicast_Address_Specific_Timer_Event,
                            (UNSIGNED)(multi->multi_device->dev_index),
                            multi->multi_timer,
                            (UNSIGNED)(multi->multi_timer_index), multi);
        }
    }

    /* If the received Query is a Multicast Address-and-Source-Specific
     * Query and there is a pending response for this group with a
     * non-empty source-list, then the group source list is augmented to
     * contain the list of sources in the new Query and a single response
     * is scheduled using the group timer.  The new response is scheduled
     * to be sent at the earliest of the remaining time for the pending
     * report and the selected delay.
     */
    else if ( (query_type == MULTICAST_ADDRESS_AND_SOURCE_SPECIFIC_QUERY) &&
              ( (multi->multi_state == DELAYING_LISTENER) &&
                (multi->multi_source_addr_pend_report == NU_TRUE) ) )
    {
        /* Get the number of sources that are contained in the query */
        num_src_in_query = INTSWAP(GET16(buf_ptr, num_src_addr_offset));

        /* Verify that we are accepting messages from at least one of
         * the source addresses listed in the query.  If so, we will
         * add any new entries to the list of sources to which we
         * may respond .
         */
        for (i = 0; i < num_src_in_query; i++)
        {
            /* Call the proper function to verify if we are receiving
             * messages sent from the source.
             */
            if (Multi_Verify_Src_By_Filter((UINT8 *)(&buf_ptr->
                                           data_ptr[multi_addr_offset
                                           + (i * addr_len)]), dev_state,
                                           MULTICAST_VERIFY_DEVICE,
                                           addr_len) == MULTICAST_ACCEPT_SOURCE)
            {
                /* We must ensure that we do not add duplicate source
                 * addresses to the list. So, we will compare each address
                 * with the addresses that are currently on the list.  If
                 * a match is not found, then it will be added to the list.
                 */
                for (j = 0; j < multi_ip->multi_num_query_src_list; j++)
                {
                    if (memcmp((multi_ip->multi_query_src_list + (j * addr_len)),
                               (buf_ptr->data_ptr + multi_addr_offset + (i * addr_len)),
                               addr_len) == 0)
                    {
                        /* We found a match.  We do not need to add it
                         * to the list. Break.
                         */
                        break;
                    }
                }

                /* If a match was not found, add the source to the
                 * list
                 */
                if ( (j >= multi_ip->multi_num_query_src_list) &&
                     (j < MAX_MULTICAST_SRC_ADDR) )
                {
                    /* Add the new source */
                    memcpy((multi_ip->multi_query_src_list + ((j+1) * addr_len)),
                           &buf_ptr->data_ptr[multi_addr_offset + (i * addr_len)],
                           addr_len);
                }
            }
        }
    }
#endif /*  #if ( (IGMP_DEFAULT_COMPATIBILITY_MODE... */

} /* Multi_Update_Entry */

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Timer_Expired
*
*   DESCRIPTION
*
*       This function is called when the Timer expires for a Multicast
*       event
*
*   INPUTS
*
*       event                   The event being handled.
*       dev_index               The index associated with the device on
*                               which the timer has expired.
*       timer_index             The index of the Multicast address which
*                               expired.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Multi_Timer_Expired(TQ_EVENT event, UNSIGNED dev_index,
                         UNSIGNED timer_index)
{
    DV_DEVICE_ENTRY         *device;
    MULTI_DATA              *multi_data = NU_NULL;
    UNSIGNED                protocol_type = 0;
    INT                     message_type = 0;
    UINT16                  max_resp_delay = 0;
    MULTI_IP                *multi_ip = NU_NULL;
    MULTI_DEV_STATE         *dev_state = NU_NULL;
    UINT8                   addr_len = 0;
    STATUS                  status;

#if ( (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) ||     \
      ((INCLUDE_IPV6 == NU_TRUE) && \
       (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)) )
    UINT16                  i, j;
#endif

    /* Get a pointer to the device associated with the device index. */
    device = DEV_Get_Dev_By_Index((UINT32)dev_index);

    if (device)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
#if ( (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) ||    \
      (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV2_COMPATIBILITY) )

        if (event == IGMPv1_Compatibility_Timer_Event)
        {
            /* The IGMPv1 compatibility timer has expired.  We must first
             * determine the default compatibility mode.  If the default
             * mode is IGMPv2, we can set the mode to IGMPv2.  If it is
             * IGMPv3, we must first see if there is an IGMPv2 timer
             * running.  If a timer is not running, we can set the mode
             * to IGMPv3.
             */
#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV2_COMPATIBILITY)

            /* Set the device's mode to IGMPv2 */
            device->dev_igmp_compat_mode = IGMPV2_COMPATIBILITY;
#else

            /* Check to see if the IGMPv2 timer is running */
            if (device->dev_igmp_compat_timers & IGMPV2_COMPAT_TIMER_RUNNING)
            {
                /* The IGMPv2 timer is running.  Therefore we can not
                 * switch directly to IGMPv3 mode.  Set the device to
                 * IGMPv2 mode.
                 */
                device->dev_igmp_compat_mode = IGMPV2_COMPATIBILITY;
            }
            else
            {
                /* Set the device to IGMPv3 mode */
                device->dev_igmp_compat_mode = IGMPV3_COMPATIBILITY;
            }
#endif
            /* Clear the timer running flag */
            device->dev_igmp_compat_timers =
                device->dev_igmp_compat_timers & ~IGMPV1_COMPAT_TIMER_RUNNING;
        }

#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)

        else if (event == IGMPv2_Compatibility_Timer_Event)
        {
            /* The IGMPv2 compatibility timer has expired.  We must first
             * determine if the IGMPv1 timer is running.  If it is, then
             * we will leave the device's mode as IGMPv1.  If the timer
             * is not running we can set the mode to IGMPv3.
             */
            if (device->dev_igmp_compat_timers & IGMPV1_COMPAT_TIMER_RUNNING)
            {
                /* Ensure that the device is still in IGMPv1 mode */
                device->dev_igmp_compat_mode = IGMPV1_COMPATIBILITY;
            }

            else
            {
                /* Set the mode to IGMPv3 mode */
                device->dev_igmp_compat_mode = IGMPV3_COMPATIBILITY;
            }

            /* Clear the timer running flag */
            device->dev_igmp_compat_timers =
                device->dev_igmp_compat_timers & ~IGMPV2_COMPAT_TIMER_RUNNING;
        }

#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        else
#endif
#endif
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) &&                 \
      (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY) )

        if (event == MLDv1_Compatibility_Timer_Event)
        {
            /* The MLDv1 compatibility timer has expired.  Set the
             * device's mode to MLDv2 mode.
             */
            device->dev6_mld_compat_mode = MLDV2_COMPATIBILITY;

            /* Clear the timer running flag */
            device->dev6_mld_compat_timers =
                device->dev6_mld_compat_timers & ~MLDV1_COMPAT_TIMER_RUNNING;
        }
#endif

        /* Get a pointer to the first Multicast address in the list */
#if (INCLUDE_IPV4 == NU_TRUE)

        if ( (timer_index >= IGMPV1_MIN_TMR_INDEX) &&
             (timer_index <= IGMPV3_BLOCK_OLD_SRCS_MAX_TMR_INDEX) )
        {
            multi_ip = (MULTI_IP *)device->dev_addr.dev_multiaddrs;
            addr_len = IP_ADDR_LEN;

            /* Set the protocol type */
            protocol_type = MULTICAST_TYPE_IGMP;
        }

#if (INCLUDE_IPV6 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

            if ( (timer_index >= MLDV1_MIN_TMR_INDEX) &&
                 (timer_index <= MLDV2_BLOCK_OLD_SRCS_MAX_TMR_INDEX) )
            {
                multi_ip = (MULTI_IP *)device->dev6_multiaddrs;
                addr_len = IP6_ADDR_LEN;

                /* Set the protocol type */
                protocol_type = MULTICAST_TYPE_MLD;
            }
#endif

        if (multi_ip)
        {
            /* Traverse the list of Multicast addresses searching for the
             * matching data structure that triggered the event.
             */
            do
            {
                /* If this is the target, process it */
                if (multi_ip->multi_data.multi_timer_index == timer_index)
                {
                    /* Save a pointer to the multicast data */
                    multi_data = &multi_ip->multi_data;

                    /* Get a pointer to the device's state struct */
                    dev_state =
                        Multi_Get_Device_State_Struct(device, multi_ip->multi_addr,
                                                      addr_len);

                    if (dev_state)
                    {
                        /* Determine the message type that should be sent
                         * by the value of the timer index
                         */
#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
                        if (addr_len == IP_ADDR_LEN)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
                            message_type = IGMP_MESSAGE_TYPE(timer_index);
#if (INCLUDE_IPV6 == NU_TRUE)
                        else
#endif
#endif
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
                        if (addr_len == IP6_ADDR_LEN)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                            message_type = MLD6_MESSAGE_TYPE(timer_index);
#endif
                        break;
                    }

                    else
                    {
                        /* Unable to find the device state struct.  Leave */
                        multi_data = NU_NULL;
                        break;
                    }
                } /* if (timer_index... */

                /* Otherwise, get the next Multicast address in the list */
                multi_ip = multi_ip->multi_next;

            } while (multi_ip);

            /* If a match was found, set the state to IDLE_LISTENER */
            if (multi_ip)
                multi_ip->multi_data.multi_state = IDLE_LISTENER;
        }

        if ( (multi_data != NU_NULL) && (message_type != NU_INVAL) )
        {
            /* Once the timer for a pending response record has expired,
             * the system transmits, on the associated interface, one or more
             * report messages carrying one or more current-state records as
             * follows:
             */

            /* If the expired timer is the General Query timer, then one
             * Current-State Record is sent for each multicast address for
             * which the specified interface has reception state.  The
             * Current-State Record carries the multicast address and its
             * associated filter mode (MULTICAST_MODE_IS_INCLUDE or
             * MULTICAST_MODE_IS_EXCLUDE) and source list.  Multiple
             * Current-State Records are packed into individual Report
             * messages, to the extent possible.
             */
            if (event == Multicast_General_Query_Timer_Event)
            {
                /* Send the report in response to the General Query */
                status = Multi_Output(multi_ip, protocol_type, (UINT8)message_type,
                                      max_resp_delay);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to send the multicast report",
                                   NERR_SEVERE, __FILE__, __LINE__);

                    NET_DBG_Notify(status, __FILE__, __LINE__,
                        NU_Current_Task_Pointer(), NU_NULL);
                }
            }

#if ( (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) ||     \
      ((INCLUDE_IPV6 == NU_TRUE) && \
       (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)) )

            /* If the expired timer is an Address-Specific timer and the list
             * of recorded sources for the multicast address is empty (i.e:
             * it is a pending response to an Address-Specific Query), then a
             * single Current-State Record is sent for that address.  The
             * Current-State Record carries the multicast address and its
             * associated filter mode (MODE_IS_INCLUDE or MODE_IS_EXCLUDE)
             * and source list.
             */
            else if (event == Multicast_Address_Specific_Timer_Event)
            {
                if (multi_data->multi_source_addr_pend_report == NU_FALSE)
                {
                    /* We use the device's multicast address link list to find a
                     * match for the timer index.  If the multicast address has
                     * been removed from the device, we will not find a match
                     * for the timer index.  Therefore, we will never send a
                     * report for a multicast address to which we are no longer
                     * listen. Therefore, we can send the report now.
                     */
                    status = Multi_Output(multi_ip, protocol_type, (UINT8)message_type,
                                          max_resp_delay);

                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to send the multicast report",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(status, __FILE__, __LINE__,
                            NU_Current_Task_Pointer(), NU_NULL);
                    }
                }

                /* If the expired timer is an Address-Specific timer and the list
                 * of recorded sources for the multicast address is not empty
                 * (i.e:  it is a pending response to a Address-and-Source-Specific
                 * Query), then the contents of the responding Current-State
                 * Record is determined from the interface state and the pending
                 * response record, as specified below:
                                set of sources in the
            interface state    pending response record     Current-State Record
            ---------------    -----------------------     --------------------
              INCLUDE (A)                 B                   IS_IN (A*B)
                                                         (Intersection of A and B)
              EXCLUDE (A)                 B                   IS_IN (B-A)
                                                         (Exclusion of A from B)

                 * If the resulting Current-State Record has an empty set of
                 * source addresses, then no response is sent.
                 */
                else
                {
                    /* Compare the report pending source list with the current
                     * interface source list to ensure that we do not respond
                     * to source that we no longer listen to.
                     */
                    /* Set the number of src_to_report to 0 */
                    dev_state->dev_num_src_to_report = 0;

                    for (i = 0; i < multi_ip->multi_num_query_src_list; i++)
                    {
                        for (j = 0; j < dev_state->dev_num_src_addr; j++)
                        {
                            /* Verify if we are accepting messages from the
                             * source
                             */
                            if (Multi_Verify_Src_By_Filter((UINT8 *)(multi_ip->
                                        multi_query_src_list + (i * addr_len)),
                                        dev_state, MULTICAST_VERIFY_DEVICE,
                                        addr_len) == MULTICAST_ACCEPT_SOURCE)
                            {
                                /* Copy the src addr into the src_to_report array */
                                memcpy(dev_state->dev_src_to_report +
                                       (dev_state->dev_num_src_to_report * addr_len),
                                       (multi_ip->multi_query_src_list +
                                       (i * addr_len)), addr_len);

                                /* Increment the number of srcs that are in the
                                 * src_to_report array
                                 */
                                dev_state->dev_num_src_to_report++;

                                break;
                            }
                        }
                    }

                    /* Send the report */
                    status = Multi_Output(multi_ip, protocol_type, (UINT8)message_type,
                                          max_resp_delay);

                    if (status != NU_SUCCESS)
                    {
                         NLOG_Error_Log("Failed to send the multicast report",
                                       NERR_SEVERE, __FILE__, __LINE__);

                         NET_DBG_Notify(status, __FILE__, __LINE__,
                             NU_Current_Task_Pointer(), NU_NULL);
                    }
                }
            }
#endif /* #if ( (IGMP_DEFAULT_COMPATIBILITY_MODE... */

            else
                NLOG_Error_Log("Multi timer expired with no matching entry",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }
} /* Multi_Timer_Expired */

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Calculate_Code_Value
*
*   DESCRIPTION
*
*       This function is used to calculate the proper value for certain
*       parameters that are received in IGMPv3/MLDv2 queries, such
*       as max response code and querier's query interval code.
*
*   INPUTS
*
*       rec_value               This is the value that was received
*                               in the query.
*
*   OUTPUTS
*
*       The calculated value for the parameter
*
*************************************************************************/
UINT32 Multi_Calculate_Code_Value(MULTI_IP *multi_ip, UINT8 rec_value)
{
    UINT32  calc_value;

#if (INCLUDE_IPV4 == NU_TRUE)
    if (multi_ip->multi_data.multi_device->dev_igmp_compat_mode == IGMPV3_COMPATIBILITY)
    {
		/* RFC 3376 - section 4.1.1 - If Max Resp Code >= 128, Max Resp Code
		 * represents a floating-point value as follows:
		 *
		 *        0 1 2 3 4 5 6 7
      	 *       +-+-+-+-+-+-+-+-+
         *       |1| exp | mant  |
         *       +-+-+-+-+-+-+-+-+
		 *
		 * Max Response Time = (mant | 0x10) << (exp + 3)
		 */
		if (rec_value & 0x80)
		{
			calc_value = (UINT32)(((rec_value & 0x0F) | 0x10) <<  \
								(((rec_value >> 4) & 0x07) + 3));
		}

		else
		{
			calc_value = ((UINT32)rec_value & 0x000000FF);
		}
    }

    else
#endif
    {
    	calc_value = rec_value;
    }

    return (calc_value);

} /* Multi_Calculate_Code_Value */

#if ( (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) ||     \
      ((INCLUDE_IPV6 == NU_TRUE) && \
       (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)) )

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Update_Device_State
*
*   DESCRIPTION
*
*       This function will calculate the new src list for the device,
*       determine the correct message to send, and calculate the proper
*       src addresses to report.
*
*   INPUTS
*
*       *device                 The device that will be updated
*       *multi_addr             Ptr to the multicast address
*       *socket_state           Ptr to the socket state structure of the
*                               socket that has recently joined a group
*                               or is the head of the socket state list
*                               if a socket is leaving a group.
*       socket_action           The action that has caused the invocation
*                               of this function
*       addr_len                Used to determine the multicast format that
*                               called this function (IGMP(IPv4)/MLD(IPv6)
*
*   OUTPUTS
*
*       INT
*
*************************************************************************/
STATUS Multi_Update_Device_State(const DV_DEVICE_ENTRY *device,
                                 const UINT8 *multi_addr,
                                 const MULTI_SCK_STATE *socket_state,
                                 UINT8 socket_action, UINT8 addr_len)
{
    UINT8           new_source_list[MAX_MULTICAST_SRC_ADDR][MAX_ADDRESS_SIZE];
    UINT8           src_list_to_report[MAX_MULTICAST_SRC_ADDR][MAX_ADDRESS_SIZE];
    UINT8           prev_dev_src_list[MAX_MULTICAST_SRC_ADDR][MAX_ADDRESS_SIZE];
    INT             ret_status = NU_SUCCESS;
    INT             message_to_send = 0;
    UINT16          prev_dev_num_src_list = 0,
                    prev_device_state = 0;
    MULTI_DEV_STATE *device_state = NU_NULL;
    MULTI_SCK_STATE *msck_state_head;
    INT16           num_src_in_list = 0;
    INT16           num_src_to_report = 0;

    if (multi_addr != NU_NULL)
    {
        device_state =
            Multi_Get_Device_State_Struct(device, multi_addr, addr_len);
    }

    if (socket_state == NU_NULL)
    {
        /* This state is most likely due to the initialization of IGMP and/or
         *  MLD.  The device state will not change and there is no message to
         *  send.
         */
        if (socket_action != MULTICAST_LEAVE_GROUP)
            ret_status = MULTICAST_NO_MSG_TO_SEND;

        else if (device_state != NU_NULL)
        {
            /* The socket is the last member to leave the group on this
             * interface. Therefore, we can set the device back to it's
             * initialized state.
             */
            /* Set the device's filter mode to INCLUDE */
            device_state->dev_filter_state = MULTICAST_FILTER_INCLUDE;

            /* Clear the device's number of src addrs */
            device_state->dev_num_src_addr = 0;

            /* Clear the device's src list */
            UTL_Zero(device_state->dev_src_addrs,
                    (MAX_MULTICAST_SRC_ADDR * addr_len) );

            /* Send a leave message */
            message_to_send = MULTICAST_CHANGE_TO_INCLUDE_MODE;
        }
    }

    else if (device_state != NU_NULL)
    {
        /* If this function has been called due to a socket leaving a
         * multicast group or a group has changed it's source list, then the
         * device's source list will have to be recalculated from scratch.
         */
        if ( (socket_action == MULTICAST_LEAVE_GROUP) ||
             (socket_action == MULTICAST_UPDATE_GROUP) )
        {
            /* Save off the current value of the device state so that we can
             * determine which kind of message, if any, to send.
             */
            prev_device_state = device_state->dev_filter_state;

            prev_dev_num_src_list = device_state->dev_num_src_addr;

            /* Copy the device's source list into the newly allocated
             * block
             */
            if (prev_dev_num_src_list)
                memcpy(prev_dev_src_list, device_state->dev_src_addrs,
                       (prev_dev_num_src_list * addr_len) );

            /* Set the device's state as INCLUDE and clear out the source list.
             * This is the state to which the device is initialized.
             */
            device_state->dev_filter_state = MULTICAST_FILTER_INCLUDE;

            /* Now, clear the device state so that it appears we are not
             * members of any groups.
             */
            device_state->dev_num_src_addr = 0;
        }

        /* Get a ptr to the head of the device's socket state list. */
        msck_state_head = device_state->dev_sck_state_list.head;

        if ( (socket_action == MULTICAST_LEAVE_GROUP) ||
             (socket_action == MULTICAST_UPDATE_GROUP) ||
             (socket_action == MULTICAST_JOIN_GROUP) )
        {
            /* This while loop is in place because the device's source list has
             * to be completely rebuild when a socket leaves the group.
             */
            while (msck_state_head != NU_NULL)
            {
                /* If the current state of the device is INCLUDE and the new
                 * socket is set to EXCLUDE, then the device's state will become
                 * EXCLUDE
                 */
                if ( (msck_state_head->sck_filter_state == MULTICAST_FILTER_EXCLUDE) &&
                     (device_state->dev_filter_state == MULTICAST_FILTER_INCLUDE) )
                {
                    /* Set the device's filter to EXCLUDE */
                    device_state->dev_filter_state = MULTICAST_FILTER_EXCLUDE;

                    num_src_in_list =
                        Multi_Src_List_Different_Mode(device_state->dev_src_addrs,
                                                      device_state->dev_num_src_addr,
                                                      msck_state_head->sck_src_list,
                                                      msck_state_head->sck_num_src_addr,
                                                      (UINT8*)new_source_list,
                                                      addr_len);

                    /* Set the return status to reflect the change in the
                     * device state
                     */
                    if (num_src_in_list != NU_MULTI_TOO_MANY_SRC_ADDRS)
                        message_to_send = MULTICAST_CHANGE_TO_EXCLUDE_MODE;
                }

                /* If the current state of the device is EXCLUDE and the new
                 * socket is set to INCLUDE, then the device's state will remain
                 * EXCLUDE
                 */
                else if ( (msck_state_head->sck_filter_state == MULTICAST_FILTER_INCLUDE) &&
                          (device_state->dev_filter_state == MULTICAST_FILTER_EXCLUDE) )
                {
                    /* We must now search the device's source list for any
                     * matches with the new socket's INCLUDE source list. If we
                     * find a match, it will be removed from the device's
                     * source list.
                     */
                    num_src_in_list =
                        Multi_Src_List_Different_Mode(device_state->dev_src_addrs,
                                                      device_state->dev_num_src_addr,
                                                      msck_state_head->sck_src_list,
                                                      msck_state_head->sck_num_src_addr,
                                                      new_source_list[0],
                                                      addr_len);

                    /* If we have added more sources to the device's source list,
                     * set the return status to reflect the change.
                     */
                    if (num_src_in_list != NU_MULTI_TOO_MANY_SRC_ADDRS)
                    {
                        if (num_src_in_list < (INT16)device_state->dev_num_src_addr)
                            message_to_send = MULTICAST_ALLOW_NEW_SOURCES;
                        else
                            message_to_send = MULTICAST_NO_MSG_TO_SEND;
                    }
                }

                /* If the current state of the device is EXCLUDE and the new
                 * socket is set to EXCLUDE, then the device's state will remain
                 * EXCLUDE
                 */
                else if ( (msck_state_head->sck_filter_state == MULTICAST_FILTER_EXCLUDE) &&
                          (device_state->dev_filter_state == MULTICAST_FILTER_EXCLUDE) )
                {
                    /* Now we must determine the correct source addresses to
                     * place into the device's source list
                     */
                    num_src_in_list =
                        Multi_Src_List_Same_Mode(device_state->dev_src_addrs,
                                                 device_state->dev_num_src_addr,
                                                 msck_state_head->sck_src_list,
                                                 msck_state_head->sck_num_src_addr,
                                                 new_source_list[0],
                                                 src_list_to_report[0], addr_len);

                    if (num_src_in_list  != NU_MULTI_TOO_MANY_SRC_ADDRS)
                    {
                        /* If we have added more sources to the device's
                         *  source list, set the return status to reflect
                         *  the change.
                         */
                        if (num_src_in_list < (INT16)device_state->dev_num_src_addr)
                            message_to_send = MULTICAST_ALLOW_NEW_SOURCES;
                        else
                            message_to_send = MULTICAST_NO_MSG_TO_SEND;
                    }
                }

                /* If the current state of the device is INCLUDE and the new
                 * socket is set to INCLUDE, then the device's state will remain
                 * INCLUDE
                 */
                else if ( (msck_state_head->sck_filter_state == MULTICAST_FILTER_INCLUDE) &&
                          (device_state->dev_filter_state == MULTICAST_FILTER_INCLUDE) )
                {
                    /* Initialize num_src_to_report variable */
                    num_src_to_report = 0;

                    /* All of the addresses from the two source lists will be
                     * added to the device's new source list, but we will only
                     * add the address once
                     */

                    /* We will start by copying the entire device source list
                     * into the new source list.  Since this is an INCLUDE-INCLUDE
                     * operation, no old sources will be blocked.
                     */
                    memcpy(new_source_list, device_state->dev_src_addrs,
                           (device_state->dev_num_src_addr * addr_len));

                    /* Set the number of addresses that have just been copied
                     * into the list
                     */
                    num_src_in_list =
                        Multi_Src_List_Same_Mode((UINT8 HUGE*)new_source_list,
                                                 device_state->dev_num_src_addr,
                                                 msck_state_head->sck_src_list,
                                                 msck_state_head->sck_num_src_addr,
                                                 new_source_list[0],
                                                 src_list_to_report[0], addr_len);


                    if (num_src_in_list != NU_MULTI_TOO_MANY_SRC_ADDRS)
                    {
                        /* If we have added more source addrs to INCLUDE, then set
                         * the return status to reflect the change in the device's
                         * state.
                         */
                        if (num_src_in_list > (INT16)device_state->dev_num_src_addr)
                            message_to_send = MULTICAST_ALLOW_NEW_SOURCES;
                    }
                }

                if ( (num_src_in_list != NU_MULTI_TOO_MANY_SRC_ADDRS) ||
                     (message_to_send != MULTICAST_NO_MSG_TO_SEND) )
                {
                    /* Copy the source list to report */
                    memcpy(device_state->dev_src_to_report, src_list_to_report,
                           (unsigned int)(num_src_to_report * addr_len));

                    /* Set the number of src addrs to report */
                    device_state->dev_num_src_to_report = (UINT16)num_src_to_report;

                    /* Copy the new source list into the device state */
                    memcpy(device_state->dev_src_addrs, new_source_list,
                           (unsigned int)(num_src_in_list * addr_len));

                    /* Set the number of src addrs now in the list */
                    device_state->dev_num_src_addr = (UINT16)num_src_in_list;
                }

                /* We only need to go through the while loop once for joining
                 * a group
                 */
                if (socket_action == MULTICAST_JOIN_GROUP)
                    break;

                /* Increment the socket state ptr to the next structure */
                else
                    msck_state_head = msck_state_head->sck_state_next;
            }

            /* Now, if a member was leaving the group, we need to calculate the
             * what message to send and what sources to report.
             */
            if (socket_action != MULTICAST_JOIN_GROUP)
            {
                /* The message to send will depend on the previous device state
                 * and it's src list and the new device state and src list.
                 */
                if ( (device_state->dev_filter_state == MULTICAST_FILTER_EXCLUDE) &&
                     (prev_device_state == MULTICAST_FILTER_EXCLUDE) )
                {
                    /* We will not be reporting a filter mode change.  However,
                     * we can still have  BLOCK_OLD_SOURCES.  Compare the two src
                     * lists for matches.  If a match is not found in the
                     * previous src list for an entry in the new source list, we
                     * are blocking a source they we previously were accepting.
                     * report the address.
                     */
                    num_src_to_report =
                        Multi_Src_List_Different_Mode((UINT8 HUGE *)new_source_list,
                                                      device_state->dev_num_src_addr,
                                                      (UINT8 HUGE*)prev_dev_src_list,
                                                      prev_dev_num_src_list,
                                                      (UINT8 *)device_state->dev_src_to_report,
                                                      addr_len);

                    /* Copy the number of src addrs to report to the structure */
                    if (num_src_to_report != NU_MULTI_TOO_MANY_SRC_ADDRS)
                    {
                        device_state->dev_num_src_to_report =
                            (UINT16)num_src_to_report;
                    }

                    /* If we are reporting any sources, set the message_to_send to
                     * MULTICAST_BLOCK_OLD_SOURCES
                     */
                    if (num_src_to_report > 0)
                        message_to_send = MULTICAST_BLOCK_OLD_SOURCES;
                    else
                        message_to_send = MULTICAST_NO_MSG_TO_SEND;
                }

                else if ( (device_state->dev_filter_state == MULTICAST_FILTER_INCLUDE) &&
                          (prev_device_state == MULTICAST_FILTER_INCLUDE) )
                {
                    /* We will not be reporting a filter mode change.  However,
                     * we can still have BLOCK_OLD_SOURCES.  Compare the two src
                     * lists for matches.  If an entry is in the previous device
                     * list and not in the new device list, then that source
                     *  is now being blocked.  Report the address.
                     */
                    num_src_to_report =
                        Multi_Src_List_Different_Mode((UINT8 HUGE*)new_source_list,
                                                      device_state->dev_num_src_addr,
                                                      (UINT8 HUGE*)prev_dev_src_list,
                                                      prev_dev_num_src_list,
                                                      (UINT8 *)device_state->dev_src_to_report,
                                                      addr_len);

                    /* Copy the number of src addrs to report to the structure */
                    if (num_src_to_report != NU_MULTI_TOO_MANY_SRC_ADDRS)
                    {
                        device_state->dev_num_src_to_report =
                            (UINT16)num_src_to_report;
                    }

                    /* If we are reporting any sources, set the message_to_send to
                     * MULTICAST_BLOCK_OLD_SOURCES
                     */
                    if (num_src_to_report > 0)
                        message_to_send = MULTICAST_BLOCK_OLD_SOURCES;
                    else
                        message_to_send = MULTICAST_NO_MSG_TO_SEND;
                }

                else if ( (device_state->dev_filter_state == MULTICAST_FILTER_INCLUDE) &&
                          (prev_device_state == MULTICAST_FILTER_EXCLUDE) )
                {
                    /* Set the message_to_send as MULTICAST_CHANGE_TO_INCLUDE_MODE */
                    message_to_send = MULTICAST_CHANGE_TO_INCLUDE_MODE;

                    /* We will report the whole device src addr list.  Therefore,
                     * copy the list into the array.
                     */
                    memcpy(device_state->dev_src_to_report,
                           device_state->dev_src_addrs,
                           (device_state->dev_num_src_addr * addr_len));

                    device_state->dev_num_src_to_report =
                        device_state->dev_num_src_addr;
                }

                else if ( (device_state->dev_filter_state == MULTICAST_FILTER_EXCLUDE) &&
                          (prev_device_state == MULTICAST_FILTER_INCLUDE) )
                {
                    /* Set the message_to_send as MULTICAST_CHANGE_TO_EXCLUDE_MODE */
                    message_to_send = MULTICAST_CHANGE_TO_EXCLUDE_MODE;

                    /* We will report the whole device src addr list.  Therefore,
                     * copy the list into the array.
                     */
                    memcpy(device_state->dev_src_to_report,
                           device_state->dev_src_addrs,
                           (device_state->dev_num_src_addr * addr_len));

                    device_state->dev_num_src_to_report =
                                        device_state->dev_num_src_addr;
                }
                else
                    /* Return so that no message will be sent */
                    message_to_send = MULTICAST_NO_MSG_TO_SEND;
            }
        }
    }

    if (ret_status == NU_SUCCESS)
        return (message_to_send);

    else
        return (ret_status);

} /* Multi_Update_Device_State */

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Verify_Src_By_Filter
*
*   DESCRIPTION
*
*       Searches through the specified src list for a match to the
*       passed in src_addr.
*
*   INPUTS
*
*       *src_addr               Target multicast address
*       *state_struct           A pointer to the state structure which
*                               we are trying to find a match.
*       state_type              Specifies the state structure type that
*                               was passed in (device or socket);
*       addr_length             Used to determine the IP version for
*                               which this operation is to be performed
*
*   OUTPUTS
*
*       INT                     MULTICAST_ACCEPT_SOURCE
*                               MULTICAST_REFUSE_SOURCE
*                               NU_INVALID_PARM
*
*************************************************************************/
INT Multi_Verify_Src_By_Filter(const UINT8 *src_addr, VOID *state_struct,
                               UINT8 state_type, UINT8 addr_len)
{
    UINT16                  filter_state = 0;
    UINT16                  num_src_in_list = 0;
    UINT16                  i;
    MULTI_DEV_STATE         *dev_state = NU_NULL;
    MULTI_SCK_STATE         *sck_state = NU_NULL;
    STATUS                  ret_status = MULTICAST_ACCEPT_SOURCE;

    if (state_type == MULTICAST_VERIFY_DEVICE)
    {
        /* Cast the passed-in ptr to the dev_state struct */
        dev_state = (MULTI_DEV_STATE *)state_struct;

        /* Get the filter state */
        filter_state = dev_state->dev_filter_state;

        /* Get the number of source addresses in the filter list */
        num_src_in_list = dev_state->dev_num_src_addr;
    }

    else if (state_type == MULTICAST_VERIFY_SOCKET)
    {
        /* Cast the passed-in ptr to the dev state struct */
        sck_state = (MULTI_SCK_STATE *)state_struct;

        /* Get the filter state */
        filter_state = sck_state->sck_filter_state;

        /* Get the number of source addresses in the filter list */
        num_src_in_list = sck_state->sck_num_src_addr;
    }

    /* Loop through the source list to see if the source is one that we
     * wish to receive messages from
     */
    for (i = 0; i < num_src_in_list; i++)
    {
        if ( ((state_type == MULTICAST_VERIFY_DEVICE) &&
              (memcmp(((UINT8 *)dev_state->dev_src_addrs + (i * addr_len)),
                      src_addr, addr_len)) ) ||
             ((state_type == MULTICAST_VERIFY_SOCKET) &&
              (memcmp(((UINT8 *)sck_state->sck_src_list + (i * addr_len)),
                      src_addr, addr_len)) ) )
        {
            if (filter_state == MULTICAST_FILTER_INCLUDE)
                ret_status = MULTICAST_ACCEPT_SOURCE;

            else if (filter_state == MULTICAST_FILTER_EXCLUDE)
                ret_status = MULTICAST_REFUSE_SOURCE;

            break;
        }
    }

    if (i >= num_src_in_list)
    {
        if (filter_state == MULTICAST_FILTER_INCLUDE)
            ret_status = MULTICAST_REFUSE_SOURCE;

        else if (filter_state == MULTICAST_FILTER_EXCLUDE)
            ret_status = MULTICAST_ACCEPT_SOURCE;

        else
            ret_status = NU_INVALID_PARM;
    }

    return (ret_status);

} /* Multi_Verify_Src_By_Filter */

#endif /*  Mode compatibility check */

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Get_Device_State_Struct
*
*   DESCRIPTION
*
*       Return a ptr to the device state for the multicast
*       group that is desired
*
*   INPUTS
*
*       *device                 Ptr to the device
*       *multiaddr              Target multicast address
*       addr_len                Used to determine IP version being used
*
*   OUTPUTS
*
*       VOID *                  Ptr to the device state struct
*
*************************************************************************/
MULTI_DEV_STATE *Multi_Get_Device_State_Struct(const DV_DEVICE_ENTRY *device,
                                               const UINT8 *multiaddr,
                                               UINT8 addr_len)
{
    MULTI_DEV_STATE *dev_state;

    /* Get the head of the device's state list */
#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
    if (addr_len == IP_ADDR_LEN)
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        dev_state = device->dev_igmp_state_list.head;

#if (INCLUDE_IPV6 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        dev_state = device->dev6_mld_state_list.head;
#endif

    /* Loop through all of the device state entries until we find one
     * that matches the multicast address.
     */
    while (dev_state != NU_NULL)
    {
        if (!memcmp(multiaddr, dev_state->dev_multiaddr, addr_len))
            break;

        /* Increment to the next state structure */
        dev_state = dev_state->dev_state_next;
    }

    return (dev_state);

} /* Multi_Get_Device_State_Struct */

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Get_Timer_Index
*
*   DESCRIPTION
*
*       Determine the proper timer index to use based upon the device's
*       current compatibility state and the type of message that
*       will be sent
*
*   INPUTS
*
*       *device                 Ptr to the device
*       *dev_state              Ptr to the device's state struct
*       protocol_type           Type of protocol that has called this
*                               function (IGMP/MLD)
*       message_to_send         The type of message that is to be sent
*                               can determine which timer index needs
*                               to be used. This is why this variable
*                               is not always used.
*
*   OUTPUTS
*
*       UINT32                  Timer index value
*
*************************************************************************/
UINT32 Multi_Get_Timer_Index(const DV_DEVICE_ENTRY *device,
                             const MULTI_DEV_STATE *dev_state,
                             UINT8 protocol_type, UINT8 message_to_send)
{
    UINT32  timer_index = 0;

#if ( (IGMP_DEFAULT_COMPATIBILTY_MODE != IGMPV3_COMPATIBILITY) || \
    ((INCLUDE_IPV6 == NU_TRUE) && \
    (MLD6_DEFAULT_COMPATIBILTY_MODE != MLDV2_COMPATIBILITY)) )
    UNUSED_PARAMETER (dev_state);
#endif

    if (message_to_send == 0)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        if (protocol_type == MULTICAST_TYPE_IGMP)
        {
            /* Determine the correct timer index to use */
            switch (device->dev_igmp_compat_mode)
            {
                case IGMPV1_COMPATIBILITY:
                    timer_index = Multicast_IGMPv1_Timer_Index++;

                    /* Increment the global timer index variable */
                    if (Multicast_IGMPv1_Timer_Index >
                            IGMPV1_MAX_TMR_INDEX)
                        Multicast_IGMPv1_Timer_Index =
                            IGMPV1_MIN_TMR_INDEX;

                    break;

#if ((IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV2_COMPATIBILITY) ||    \
     (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) )
                case IGMPV2_COMPATIBILITY:

                    timer_index = Multicast_IGMPv2_Timer_Index++;

                    /* Increment the global timer index variable */
                    if (Multicast_IGMPv2_Timer_Index >
                            IGMPV2_MAX_TMR_INDEX)
                        Multicast_IGMPv2_Timer_Index =
                            IGMPV2_MIN_TMR_INDEX;

                    break;

#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
                case IGMPV3_COMPATIBILITY:

                    /* The timer index to use will be based upon the
                     * filter mode of the device
                     */
                    if (dev_state->dev_filter_state ==
                            MULTICAST_FILTER_INCLUDE)
                    {
                        timer_index =
                            Multicast_IGMPv3_Mode_Include_Timer_Index++;

                        /* Increment the global timer index variable */
                        if (Multicast_IGMPv3_Mode_Include_Timer_Index >
                                IGMPV3_MODE_INC_MAX_TMR_INDEX)
                            Multicast_IGMPv3_Mode_Include_Timer_Index =
                                IGMPV3_MODE_INC_MIN_TMR_INDEX;
                    }

                    else
                    {
                        timer_index =
                                Multicast_IGMPv3_Mode_Exclude_Timer_Index++;

                        /* Increment the global timer index variable */
                        if (Multicast_IGMPv3_Mode_Exclude_Timer_Index >
                                IGMPV3_MODE_EXC_MAX_TMR_INDEX)
                            Multicast_IGMPv3_Mode_Exclude_Timer_Index =
                                IGMPV3_MODE_EXC_MIN_TMR_INDEX;
                    }

                    break;
#endif
#endif
                default:
                    break;
            }
        }
#if (INCLUDE_IPV6 == NU_TRUE)
        else
#endif
#else
        UNUSED_PARAMETER(dev_state);
#endif /* (INCLUDE_IPV4 == NU_TRUE) */

#if (INCLUDE_IPV6 == NU_TRUE)
        if (protocol_type == MULTICAST_TYPE_MLD)
        {
            /* Determine the correct timer index to use */
            switch (device->dev6_mld_compat_mode)
            {
            case MLDV1_COMPATIBILITY:

                timer_index = Multicast_MLDv1_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_MLDv1_Timer_Index > MLDV1_MAX_TMR_INDEX)
                    Multicast_MLDv1_Timer_Index = MLDV1_MIN_TMR_INDEX;

                break;

#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
            case MLDV2_COMPATIBILITY:

                /* The timer index to use will be based upon the filter
                 * mode of the device
                 */
                if (dev_state->dev_filter_state ==
                        MULTICAST_FILTER_INCLUDE)
                {
                    timer_index = Multicast_MLDv2_Mode_Include_Timer_Index++;

                    /* Increment the global timer index variable */
                    if (Multicast_MLDv2_Mode_Include_Timer_Index >
                            MLDV2_MODE_INC_MAX_TMR_INDEX)
                        Multicast_MLDv2_Mode_Include_Timer_Index =
                             MLDV2_MODE_INC_MIN_TMR_INDEX;
                }

                else
                {
                    timer_index = Multicast_MLDv2_Mode_Exclude_Timer_Index++;

                    /* Increment the global timer index variable */
                    if (Multicast_MLDv2_Mode_Exclude_Timer_Index >
                            MLDV2_MODE_EXC_MAX_TMR_INDEX)
                        Multicast_MLDv2_Mode_Exclude_Timer_Index =
                             MLDV2_MODE_EXC_MIN_TMR_INDEX;
                }

                break;
#endif
            default:
                break;
            }
        }
#endif /* (INCLUDE_IPV6 == NU_TRUE) */
    }

    else
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        if (protocol_type == MULTICAST_TYPE_IGMP)
        {
            /* Set the timer index */
            switch (message_to_send)
            {
            case IGMPV1_HOST_MEMBERSHIP_REPORT:

                timer_index = Multicast_IGMPv1_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_IGMPv1_Timer_Index > IGMPV1_MAX_TMR_INDEX)
                    Multicast_IGMPv1_Timer_Index = IGMPV1_MIN_TMR_INDEX;

                break;

#if ((IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV2_COMPATIBILITY) ||    \
     (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) )
            case IGMPV2_HOST_MEMBERSHIP_REPORT:

                timer_index = Multicast_IGMPv2_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_IGMPv2_Timer_Index > IGMPV2_MAX_TMR_INDEX)
                    Multicast_IGMPv2_Timer_Index = IGMPV2_MIN_TMR_INDEX;

                break;

#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
            case MULTICAST_MODE_IS_INCLUDE:

                timer_index = Multicast_IGMPv3_Mode_Include_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_IGMPv3_Mode_Include_Timer_Index >
                        IGMPV3_MODE_INC_MAX_TMR_INDEX)
                    Multicast_IGMPv3_Mode_Include_Timer_Index =
                        IGMPV3_MODE_INC_MIN_TMR_INDEX;

                break;

            case MULTICAST_MODE_IS_EXCLUDE:

                timer_index = Multicast_IGMPv3_Mode_Exclude_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_IGMPv3_Mode_Exclude_Timer_Index >
                        IGMPV3_MODE_EXC_MAX_TMR_INDEX)
                    Multicast_IGMPv3_Mode_Exclude_Timer_Index =
                        IGMPV3_MODE_EXC_MIN_TMR_INDEX;

                break;

            case MULTICAST_CHANGE_TO_INCLUDE_MODE:

                timer_index = Multicast_IGMPv3_Change_To_Include_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_IGMPv3_Change_To_Include_Timer_Index >
                        IGMPV3_CHANGE_TO_INC_MAX_TMR_INDEX)
                    Multicast_IGMPv3_Change_To_Include_Timer_Index =
                        IGMPV3_CHANGE_TO_INC_MIN_TMR_INDEX;

                break;

            case MULTICAST_CHANGE_TO_EXCLUDE_MODE:

                timer_index = Multicast_IGMPv3_Change_To_Exclude_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_IGMPv3_Change_To_Exclude_Timer_Index >
                        IGMPV3_CHANGE_TO_EXC_MAX_TMR_INDEX)
                    Multicast_IGMPv3_Change_To_Exclude_Timer_Index =
                        IGMPV3_CHANGE_TO_EXC_MIN_TMR_INDEX;

                break;

            case MULTICAST_ALLOW_NEW_SOURCES:

                timer_index = Multicast_IGMPv3_Allow_New_Sources_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_IGMPv3_Allow_New_Sources_Timer_Index >
                        IGMPV3_ALLOW_NEW_SRCS_MAX_TMR_INDEX)
                    Multicast_IGMPv3_Allow_New_Sources_Timer_Index =
                        IGMPV3_ALLOW_NEW_SRCS_MIN_TMR_INDEX;

                break;

            case MULTICAST_BLOCK_OLD_SOURCES:

                timer_index = Multicast_IGMPv3_Block_Old_Sources_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_IGMPv3_Block_Old_Sources_Timer_Index >
                        IGMPV3_BLOCK_OLD_SRCS_MAX_TMR_INDEX)
                    Multicast_IGMPv3_Block_Old_Sources_Timer_Index =
                        IGMPV3_BLOCK_OLD_SRCS_MIN_TMR_INDEX;

                break;
#endif
#endif
            default :

                break;
            }
        }
#if (INCLUDE_IPV6 == NU_TRUE)
        else
#endif
#endif /* INCLUDE_IPV4 */

#if (INCLUDE_IPV6 == NU_TRUE)
        if (protocol_type == MULTICAST_TYPE_MLD)
        {
            /* Set the timer index */
            switch (message_to_send)
            {
            case MLDV1_LISTENER_REPORT:

                timer_index = Multicast_MLDv1_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_MLDv1_Timer_Index > MLDV1_MAX_TMR_INDEX)
                    Multicast_MLDv1_Timer_Index = MLDV1_MIN_TMR_INDEX;

                break;

#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
            case MULTICAST_MODE_IS_INCLUDE:

                timer_index = Multicast_MLDv2_Mode_Include_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_MLDv2_Mode_Include_Timer_Index >
                        MLDV2_MODE_INC_MAX_TMR_INDEX)
                    Multicast_MLDv2_Mode_Include_Timer_Index =
                        MLDV2_MODE_INC_MIN_TMR_INDEX;

                break;

            case MULTICAST_MODE_IS_EXCLUDE:

                timer_index = Multicast_MLDv2_Mode_Exclude_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_MLDv2_Mode_Exclude_Timer_Index >
                        MLDV2_MODE_EXC_MAX_TMR_INDEX)
                    Multicast_MLDv2_Mode_Exclude_Timer_Index =
                        MLDV2_MODE_EXC_MIN_TMR_INDEX;

                break;

            case MULTICAST_CHANGE_TO_INCLUDE_MODE:

                timer_index = Multicast_MLDv2_Change_To_Include_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_MLDv2_Change_To_Include_Timer_Index >
                        MLDV2_CHANGE_TO_INC_MAX_TMR_INDEX)
                    Multicast_MLDv2_Change_To_Include_Timer_Index =
                        MLDV2_CHANGE_TO_INC_MIN_TMR_INDEX;

                break;

            case MULTICAST_CHANGE_TO_EXCLUDE_MODE:

                timer_index = Multicast_MLDv2_Change_To_Exclude_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_MLDv2_Change_To_Exclude_Timer_Index >
                        MLDV2_CHANGE_TO_EXC_MAX_TMR_INDEX)
                    Multicast_MLDv2_Change_To_Exclude_Timer_Index =
                        MLDV2_CHANGE_TO_EXC_MIN_TMR_INDEX;

                break;

            case MULTICAST_ALLOW_NEW_SOURCES:

                timer_index = Multicast_MLDv2_Allow_New_Sources_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_MLDv2_Allow_New_Sources_Timer_Index >
                        MLDV2_ALLOW_NEW_SRCS_MAX_TMR_INDEX)
                    Multicast_MLDv2_Allow_New_Sources_Timer_Index =
                        MLDV2_ALLOW_NEW_SRCS_MIN_TMR_INDEX;

                break;

            case MULTICAST_BLOCK_OLD_SOURCES:

                timer_index = Multicast_MLDv2_Block_Old_Sources_Timer_Index++;

                /* Increment the global timer index variable */
                if (Multicast_MLDv2_Block_Old_Sources_Timer_Index >
                        MLDV2_BLOCK_OLD_SRCS_MAX_TMR_INDEX)
                    Multicast_MLDv2_Block_Old_Sources_Timer_Index =
                        MLDV2_BLOCK_OLD_SRCS_MIN_TMR_INDEX;

                break;
#endif
            default :

                break;
            }
        }
#endif
    }

    return (timer_index);

} /* Multi_Get_Timer_Index */

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Src_List_Different_Mode
*
*   DESCRIPTION
*
*       This routine copies all addresses that exist in only one of
*       the incoming source lists into a new source list.
*
*   INPUTS
*
*       *addr_list_1            A pointer to the first source list.
*       num_addr_in_list_1      The number of addresses in the first
*                               source list.
*       *addr_list_2            A pointer to the second source list.
*       num_addr_in_list_2      The number of addresses in the second
*                               source list.
*       *new_src_list           A pointer to the new source list being
*                               built.
*       addr_len                The length of addresses in the source
*                               lists.
*
*   OUTPUTS
*
*       The number of addresses copied into the new source list.
*
*************************************************************************/
INT16 Multi_Src_List_Different_Mode(const UINT8 HUGE *addr_list_1,
                                    UINT16 num_addr_in_list_1,
                                    const UINT8 HUGE *addr_list_2,
                                    UINT16 num_addr_in_list_2,
                                    UINT8 *new_src_list, UINT8 addr_len)
{
    INT16   num_src_in_list = 0;
    UINT16  i, j;

    /* Now we must determine the correct source addresses to
     * place into the device's source list.
     */
    for (j = 0; j < num_addr_in_list_1; j++)
    {
        for (i = 0; i < num_addr_in_list_2; i++)
        {
            /* If the two addresses match, then the address will
             * not be added to the device's source list.
             */
            if (memcmp((addr_list_1 + (i * addr_len)),
                       (addr_list_2 + (j * addr_len)),
                       addr_len) == 0)
            {
                break;
            }
        }

        /* If a match was not found in the source list and we desire to
         * copy unique addresses, then the addr can be added to the
         * device's src list
         */
        if (i >= num_addr_in_list_2)
        {
            /* Ensure that we do not exceed the list's capacity
             * for source addresses.  If so, return an error
             */
            if (num_src_in_list >= MAX_MULTICAST_SRC_ADDR)
            {
                num_src_in_list = NU_MULTI_TOO_MANY_SRC_ADDRS;
                break;
            }

            /* Copy the address to the new_source_list */
            memcpy((new_src_list + (num_src_in_list * addr_len)),
                   (addr_list_2 + (j * addr_len)),
                   addr_len );

            /* Increment the num_src_in_list variable */
            num_src_in_list++;
        }
    }

    return (num_src_in_list);

} /* Multi_Src_List_Different_Mode */

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Src_List_Same_Mode
*
*   DESCRIPTION
*
*       This routine copies all addresses that exist in both of the
*       incoming source lists into a new source list.
*
*   INPUTS
*
*       *addr_list_1            A pointer to the first source list.
*       num_addr_in_list_1      The number of addresses in the first
*                               source list.
*       *addr_list_2            A pointer to the second source list.
*       num_addr_in_list_2      The number of addresses in the second
*                               source list.
*       *new_src_list           A pointer to the new source list being
*                               built.
*       *src_list_to_report     A pointer to a list of source addresses
*                               that will be listed on the interface to
*                               be accepted.
*       addr_len                The length of addresses in the source
*                               lists.
*
*   OUTPUTS
*
*       The number of addresses in the src_list_to_report list.
*
*************************************************************************/
INT16 Multi_Src_List_Same_Mode(const UINT8 HUGE *addr_list_1,
                               UINT16 num_addr_in_list_1,
                               const UINT8 HUGE *addr_list_2,
                               UINT16 num_addr_in_list_2,
                               UINT8 *new_src_list,
                               UINT8 *src_list_to_report, UINT8 addr_len)
{
    INT16   num_src_in_list = 0, num_src_to_report = 0;
    UINT16  i, j;

    /* Now we must determine the correct source addresses to
     * place into the device's source list
     */
    for (i = 0; i < num_addr_in_list_1; i++)
    {
        for (j = 0; j < num_addr_in_list_2; j++)
        {
            /* If the two addresses match, then that address
             * should be in the device's source list.
             */
            if (memcmp((addr_list_1 + (i * addr_len)),
                       ((UINT8 *)addr_list_2 + (j * addr_len)),
                       addr_len) == 0)
            {
                /* Ensure that we do not exceed the list's
                 * capacity for source addresses.  If so,
                 * return an error
                 */
                if (num_src_in_list >= MAX_MULTICAST_SRC_ADDR)
                {
                    num_src_in_list = NU_MULTI_TOO_MANY_SRC_ADDRS;
                    break;
                }

                /* Copy the address to the new_source_list */
                memcpy((new_src_list + (num_src_in_list * addr_len)),
                       ((UINT8 *)addr_list_2 + (j * addr_len)),
                       addr_len);

                /* Increment the num_src_in_list variable */
                num_src_in_list++;

                break;
            }
        }

        if (num_src_in_list == NU_MULTI_TOO_MANY_SRC_ADDRS)
            break;
    }

    if (num_src_in_list != NU_MULTI_TOO_MANY_SRC_ADDRS)
    {
        /* Now that we have the new source list for the device, we
         * must create the list of sources that we will report as
         * now being accepted.
         */
        for (i = 0; i < (UINT16)num_src_in_list; i++)
        {
            for (j = 0; j < num_addr_in_list_1; j++)
            {
                if (memcmp((new_src_list + (i * addr_len)),
                           (addr_list_1 + (j * addr_len)),
                           addr_len) == 0)
                {
                    /* Ensure that we do not exceed the list's
                     * capacity for source addresses.  If so,
                     * return an error
                     */
                    if (num_src_to_report >= MAX_MULTICAST_SRC_ADDR)
                    {
                        num_src_to_report = NU_MULTI_TOO_MANY_SRC_ADDRS;
                        break;
                    }

                    /* Copy the address to the new_source_list */
                    memcpy((src_list_to_report + (num_src_to_report * addr_len)),
                           (addr_list_1 + (j * addr_len)),
                           addr_len);

                    /* Increment the num_src_in_list variable */
                    num_src_to_report++;
                    break;
                }
            }
            if (num_src_to_report == NU_MULTI_TOO_MANY_SRC_ADDRS)
                break;
        }
    }

    return (num_src_to_report);

} /* Multi_Src_List_Same_Mode */

#endif /*#if (INCLUDE_IP_MULTICASTING == NU_TRUE)*/

