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

/***********************************************************************
*
*   FILE NAME
*
*       igmp.c
*
*   COMPONENT
*
*       IGMP - Internet Group Management Protocol
*
*   DESCRIPTION
*
*       This file contains the IGMP (Internet Group Management Protocol)
*       module. IGMP provides the means for hosts to notify routers
*       of the multicasting groups that the host belongs to.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IGMP_Interpret
*       IGMP_Initialize
*       IGMP_Random_Delay
*       IGMP_Join
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_TRUE) )

#if ( (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV2_COMPATIBILITY) ||   \
      (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) )
TQ_EVENT    IGMPv1_Compatibility_Timer_Event;
#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
TQ_EVENT    IGMPv2_Compatibility_Timer_Event;
#endif
#endif

UNSIGNED    Multicast_IGMPv1_Timer_Index =
                            IGMPV1_MIN_TMR_INDEX;
#if ((IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV2_COMPATIBILITY) ||    \
     (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) )
UNSIGNED    Multicast_IGMPv2_Timer_Index =
                                    IGMPV2_MIN_TMR_INDEX;
#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
UNSIGNED    Multicast_IGMPv3_Mode_Include_Timer_Index =
                                    IGMPV3_MODE_INC_MIN_TMR_INDEX;
UNSIGNED    Multicast_IGMPv3_Mode_Exclude_Timer_Index =
                                    IGMPV3_MODE_EXC_MIN_TMR_INDEX;
UNSIGNED    Multicast_IGMPv3_Change_To_Include_Timer_Index =
                                    IGMPV3_CHANGE_TO_INC_MIN_TMR_INDEX;
UNSIGNED    Multicast_IGMPv3_Change_To_Exclude_Timer_Index =
                                    IGMPV3_CHANGE_TO_EXC_MIN_TMR_INDEX;
UNSIGNED    Multicast_IGMPv3_Allow_New_Sources_Timer_Index =
                                    IGMPV3_ALLOW_NEW_SRCS_MIN_TMR_INDEX;
UNSIGNED    Multicast_IGMPv3_Block_Old_Sources_Timer_Index =
                                    IGMPV3_BLOCK_OLD_SRCS_MIN_TMR_INDEX;
#endif
#endif

extern TQ_EVENT    Multicast_General_Query_Timer_Event;

/***********************************************************************
*
*   FUNCTION
*
*       IGMP_Initialize
*
*   DESCRIPTION
*
*       This function joins the all hosts group on an IP multicasting
*       capable device.  All level two conforming hosts are required to
*       join the all hosts group (224.0.0.1).
*
*   INPUTS
*
*       *device                 Pointer to the device to init IGMP on.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion of the operation.
*       NU_INVAL                Failure
*
*************************************************************************/
STATUS IGMP_Initialize(DV_DEVICE_ENTRY *device)
{
    STATUS                  ret_status;
    IP_MREQ                 mreq;

    /* Initialize the IGMP version mode to the default version */
    device->dev_igmp_compat_mode = IGMP_DEFAULT_COMPATIBILTY_MODE;

    /* Set the query interval value and query response interval to there
        default values */
    device->dev_igmp_last_query_interval = IGMP_QUERY_INTERVAL;

    device->dev_igmp_last_query_response_interval =
                            IGMP_QUERY_RESPONSE_INTERVAL;

    mreq.sck_multiaddr = 0;

    /* Copy the all-hosts group into the IP_MREQ struct */
    PUT32(&mreq.sck_multiaddr, 0, LONGSWAP(IGMP_ALL_HOSTS_GROUP));

    /* Join the all hosts group on this device. Note that IP_Add_Multi
     * will not report the membership in the all hosts group, and a report
     * should not be sent here.  RFC 1112 specifies that membership in
     * the all hosts group is never reported.
     */
    if (IP_Add_Multi(mreq.sck_multiaddr, device, NU_NULL) != NU_NULL)
    {
        /* Now we must add a route to the all router's group for IGMPv1
         * and IGMPv2
         */
        ret_status = RTAB4_Add_Route(device, IGMP_ALL_ROUTERS_GROUP,
                                     0xffffffffUL,
                                     device->dev_addr.dev_addr_list.
                                     dv_head->dev_entry_ip_addr,
                                     (INT32)(RT_UP | RT_HOST |
                                     RT_SILENT | RT_LOCAL));

#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
        if (ret_status == NU_SUCCESS)
        {
            /* Now we need to add the all router's group for IGMPv3.
             * This is the address to which most IGMPv3 messages will sent.
             */
            ret_status = RTAB4_Add_Route(device, IGMPV3_ALL_ROUTERS_GROUP,
                                         0xffffffffUL,
                                         device->dev_addr.dev_addr_list.
                                         dv_head->dev_entry_ip_addr,
                                         (INT32)(RT_UP | RT_HOST |
                                         RT_SILENT | RT_LOCAL));

            if (ret_status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to add route for multicast address",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
        else
#else
        if (ret_status != NU_SUCCESS)
#endif
        {
            NLOG_Error_Log("Failed to add route for multicast address",
                            NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
        ret_status = NU_INVAL;

    return (ret_status);

} /* IGMP_Initialize */

/***********************************************************************
*
*   FUNCTION
*
*       IGMP_Interpret
*
*   DESCRIPTION
*
*       This function will determine if the received IGMP packets are
*       valid.  If they are valid, it will pass them on to Multi_Input.
*
*   INPUTS
*
*       *buf_ptr                Pointer to a buffer containing the IGMP
*                               packet.
*       *device                 Ptr to the device struct
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       -1                      Failure
*
*************************************************************************/
STATUS IGMP_Interpret(NET_BUFFER *buf_ptr,
                      const DV_DEVICE_ENTRY *device)
{
    STATUS      status;

    if (buf_ptr->mem_total_data_len >= IGMP_HEADER_LEN)
    {
        /* Validate the IGMP Checksum */
        if (TLS_IP_Check_Buffer_Chain(buf_ptr) == 0)
        {
            /* Call the central Multicast input function */
            status = Multi_Input(buf_ptr, device, IGMP_GROUP_OFFSET);
        }

        else
        {
            NLOG_Error_Log("Invalid IGMP checksum", NERR_RECOVERABLE,
                           __FILE__, __LINE__);

            status = -1;
        }
    }

    else
        status = -1;

    /* Free the buffer. */
    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

    return (status);

} /* IGMP_Interpret */

/***********************************************************************
*
*   FUNCTION
*
*       IGMP_Join
*
*   DESCRIPTION
*
*       This function sends a multicast group membership report.
*       A timer event is created to send a second copy of the report.
*
*   INPUTS
*
*       *ipm                    A pointer to the multicast group
*                               structure.
*       message_to_send         Type of message that is to be sent
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IGMP_Join(IP_MULTI *ipm, UINT8 message_to_send)
{
    MULTI_DEV_STATE         *igmp_dev_state;
    UINT32                  ipm_addr32;

    /* Convert the multicast address into a UINT32 format */
    ipm_addr32 = GET32(ipm->ipm_addr, 0);

    /* Membership in the all hosts and all routers groups are not
     * reported. It is assumed that all level 2 conforming multicasts
     * hosts are members of this group.
     */
    /* NOTE: When loop back of multicast packets is supported,
     * membership should not be reported in that case either. The
     * interface should be checked here.
     */
    if ( (ipm_addr32 == IGMP_ALL_HOSTS_GROUP) ||
         (ipm_addr32 == IGMP_ALL_ROUTERS_GROUP) ||
         (ipm_addr32 == IGMPV3_ALL_ROUTERS_GROUP) )
    {
        ipm->ipm_data.multi_timer = 0;
        ipm->ipm_data.multi_set_time = 0;
    }

    else
    {
        /* Set the state to idle listener */
        ipm->ipm_data.multi_state = IDLE_LISTENER;

        /* Send a report of the membership.*/
        if (ipm->ipm_data.multi_device->dev_igmp_compat_mode ==
            IGMPV1_COMPATIBILITY)
            message_to_send = IGMPV1_HOST_MEMBERSHIP_REPORT;

        else if (ipm->ipm_data.multi_device->dev_igmp_compat_mode ==
                 IGMPV2_COMPATIBILITY)
            message_to_send = IGMPV2_HOST_MEMBERSHIP_REPORT;

        if (IGMP_Send(ipm, ipm_addr32, message_to_send, 0) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to transmit IGMP message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Set up timer event to send the report again just in case
         *  something goes wrong with the first report. */
        ipm->ipm_data.multi_timer = IGMP_Random_Delay(ipm);

        /* Get a pointer to the device's state struct */
        igmp_dev_state =
            Multi_Get_Device_State_Struct(ipm->ipm_data.multi_device,
                                          ipm->ipm_addr, IP_ADDR_LEN);

        if (igmp_dev_state)
        {
            /* Get the timer index */
            ipm->ipm_data.multi_timer_index =
                    Multi_Get_Timer_Index(ipm->ipm_data.multi_device,
                                          igmp_dev_state, MULTICAST_TYPE_IGMP,
                                          message_to_send);

            /* Set the timer to the new value */
            Multi_Set_Timer(Multicast_General_Query_Timer_Event,
                            (UNSIGNED)(ipm->ipm_data.multi_device->dev_index),
                            ipm->ipm_data.multi_timer,
                            (UNSIGNED)(ipm->ipm_data.multi_timer_index), &ipm->ipm_data);
        }
    }

} /* IGMP_Join */

/***********************************************************************
*
*   FUNCTION
*
*       IGMP_Random_Delay
*
*   DESCRIPTION
*
*       Compute a "random" delay to be used for sending the next IGMP
*       group membership report.
*
*   INPUTS
*
*       *ipm                    A pointer to the multicast group
*                               structure.
*
*   OUTPUTS
*
*       UINT32                  The length of time in ticks before the
*                               next report should be sent.
*
*************************************************************************/
UINT32 IGMP_Random_Delay(IP_MULTI *ipm)
{
    /* Calculate a random delay using the max_resp_delay */
    ipm->ipm_data.multi_timer =
                  MULTI_RANDOM_DELAY(IGMP_UNSOLICITED_REPORT_INTERVAL);

    return (ipm->ipm_data.multi_timer);

} /* IGMP_Random_Delay */

/*************************************************************************
*
*   FUNCTION
*
*       IGMP_Update_Host_Compatibility
*
*   DESCRIPTION
*
*       This function determines the which compatibility mode the device
*       should be in and sets the device to that mode.
*
*   INPUTS
*
*       *buf_ptr                A ptr to the received query
*
*       *multi_ptr              A ptr to the multi structure
*
*   OUTPUTS
*
*       VOID
*
*************************************************************************/
VOID IGMP_Update_Host_Compatibility(const NET_BUFFER *buf_ptr,
                                    MULTI_IP *multi_ptr)
{
    if (multi_ptr != NU_NULL)
    {
        /* Determine the message type */
        if (buf_ptr->data_ptr[IGMP_TYPE_OFFSET] ==
            IGMP_HOST_MEMBERSHIP_QUERY)
        {
#if ( (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV2_COMPATIBILITY) ||   \
      (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY) )

            /* If the total length of the query is exactly 8 octets
             * and the max response code is set to 0, then this is an
             * IGMPv1 query.
             */
            if ( (buf_ptr->data_len == MULTICAST_IGMPV1_QUERY_LENGTH) &&
                 (buf_ptr->data_ptr[IGMP_MAX_RESP_CODE_OFFSET] == 0) )
            {
                /* Since this is a IGMPv1 query, set the device as
                 * being in IGMPv1 compatibility mode
                 */

                /* If there is currently an IGMPv1 timer running,
                 * reset the timer.
                 */
                if (multi_ptr->multi_data.multi_device->dev_igmp_compat_timers &
                    IGMPV1_COMPAT_TIMER_RUNNING)
                {
                    Multi_Unset_Timer(IGMPv1_Compatibility_Timer_Event,
                                      TQ_CLEAR_EXACT,
                                      (UNSIGNED)(multi_ptr->
                                      multi_data.multi_device->dev_index), 0,
                                      &multi_ptr->multi_data);

                    /* Restart the timer with a fresh timeout value */
                    Multi_Set_Timer(IGMPv1_Compatibility_Timer_Event,
                                    (UNSIGNED)(multi_ptr->
                                    multi_data.multi_device->dev_index),
                                    IGMP_OLDER_VERSION_QUERIER_PRESENT_TIMEOUT(multi_ptr->
                                    multi_data.multi_device), 0, &multi_ptr->multi_data);
                }

                /* If the default compatibility mode is not IGMPv1,
                 * then set the compatibility timer.
                 */
#if (IGMP_DEFAULT_COMPATIBILTY_MODE != IGMPV1_COMPATIBILITY)
                else
                {
                    /* Set the device as being in IGMPv1
                     * compatibility mode
                     */
                    multi_ptr->multi_data.multi_device->dev_igmp_compat_mode =
                        IGMPV1_COMPATIBILITY;

                    /* Set the compatibility timer */
                    Multi_Set_Timer(IGMPv1_Compatibility_Timer_Event,
                                    (UNSIGNED)(multi_ptr->
                                    multi_data.multi_device->dev_index),
                                    IGMP_OLDER_VERSION_QUERIER_PRESENT_TIMEOUT(multi_ptr->
                                    multi_data.multi_device), 0, &multi_ptr->multi_data);

                    /* Set the flag indicating that the timer is
                     * running
                     */
                    multi_ptr->multi_data.multi_device->dev_igmp_compat_timers =
                        (multi_ptr->multi_data.multi_device->
                         dev_igmp_compat_timers &
                         IGMPV1_COMPAT_TIMER_RUNNING);
                }
#endif
            }

            /* If the total length of the query is exactly 8 octets
             * and the max response code is not equal to 0, then this
             * is an IGMPv2 query.
             */
            else if ( (buf_ptr->data_len == MULTICAST_IGMPV2_QUERY_LENGTH) &&
                      (buf_ptr->data_ptr[IGMP_MAX_RESP_CODE_OFFSET] != 0) )
            {
#if (IGMP_DEFAULT_COMPATIBILTY_MODE != IGMPV1_COMPATIBILITY)
                /* If we are not currently in IGMPv1 compatibility
                 * mode or if the default mode is not IGMPv1, then
                 * set the device's compatibility mode to IGMPv2
                 */
                if (multi_ptr->multi_data.multi_device->dev_igmp_compat_mode !=
                    IGMPV1_COMPATIBILITY)
                {
                    /* If there is currently a IGMPv2 timer running,
                     * reset the timer.
                     */
                    if (multi_ptr->multi_data.multi_device->dev_igmp_compat_timers &
                        IGMPV2_COMPAT_TIMER_RUNNING)
                    {
#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
                        Multi_Unset_Timer(IGMPv2_Compatibility_Timer_Event,
                                          TQ_CLEAR_EXACT, (UNSIGNED)(multi_ptr->
                                          multi_data.multi_device->dev_index), 0,
                                          &multi_ptr->multi_data);

                        /* Restart the timer with a fresh timeout value */
                        Multi_Set_Timer(IGMPv2_Compatibility_Timer_Event,
                                        (UNSIGNED)(multi_ptr->
                                        multi_data.multi_device->dev_index),
                                        IGMP_OLDER_VERSION_QUERIER_PRESENT_TIMEOUT(multi_ptr->
                                        multi_data.multi_device), 0, &multi_ptr->multi_data);
#endif
                    }
                    /* If the default compatibility mode is not IGMPv2,
                     * then set the compatibility timer.
                     */
#if (IGMP_DEFAULT_COMPATIBILTY_MODE != IGMPV2_COMPATIBILITY)
                    else
                    {
                        /* Set the device as being in IGMPv2
                         * compatibility mode
                         */
                        multi_ptr->multi_data.multi_device->dev_igmp_compat_mode =
                            IGMPV2_COMPATIBILITY;

#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)

                        /* Set the compatibility timer */
                        Multi_Set_Timer(IGMPv2_Compatibility_Timer_Event,
                                        (UNSIGNED)(multi_ptr->
                                        multi_data.multi_device->dev_index),
                                        IGMP_OLDER_VERSION_QUERIER_PRESENT_TIMEOUT(multi_ptr->
                                        multi_data.multi_device), 0, &multi_ptr->multi_data);

                        /* Set the flag indicating that the timer
                         * is running
                         */
                        multi_ptr->multi_data.multi_device->dev_igmp_compat_timers =
                            (multi_ptr->multi_data.multi_device->
                             dev_igmp_compat_timers & IGMPV2_COMPAT_TIMER_RUNNING);
#endif
                    }
#endif
                }
#endif
            }

            /* This is an IGMPv3 query.  */
            else
            {
                /* Save the querier interval into the device
                 * structure
                 */
                multi_ptr->multi_data.multi_device->dev_igmp_last_query_interval =
                    Multi_Calculate_Code_Value(multi_ptr, GET8(buf_ptr->data_ptr,
                                               IGMP_QUERIER_QUERY_INTERVAL_CODE));
            }
#endif
        }
    }

} /* IGMP_Update_Host_Compatibility */

#endif
