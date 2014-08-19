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
* FILE NAME
*
*       ip_mc_dm.c
*
* DESCRIPTION
*
*       This file contains the implementation of IP_Delete_Multi.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IP_Delete_Multi
*       IGMP_Leave
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"


#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_TRUE) )

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Memory (Arrays) already declared in ip_mc.c */
extern IP_MULTI NET_MC_Group_Memory[];
extern UINT8 NET_MC_Group_Memory_IP_Multi_Src_List[NET_MAX_MULTICAST_GROUPS][IP_ADDR_LEN +
                                        (MAX_MULTICAST_SRC_ADDR * IP_ADDR_LEN)];
extern MULTI_DEV_STATE NET_MC_Group_Memory_Dev_State[];
extern UINT8 NET_MC_Group_Memory_Src_List[NET_MAX_MULTICAST_GROUPS][(IP_ADDR_LEN +
                                        (2 * (MAX_MULTICAST_SRC_ADDR * IP_ADDR_LEN)))];
extern UINT8    NET_MC_Group_Memory_Flags[];

/* Memory (Arrays) already declared in ip_pml.c */
extern MULTI_SCK_OPTIONS NET_Amm_Memory[];
extern MULTI_SCK_STATE NET_Amm_Sck_State_Memory[NSOCKETS][IP_MAX_MEMBERSHIPS];
extern UINT8 NET_Amm_Sck_State_Memory_Src_List[NSOCKETS][IP_ADDR_LEN +
                                        (MAX_MULTICAST_SRC_ADDR * IP_ADDR_LEN)];
#endif

/* Externs */
extern TQ_EVENT    Multicast_General_Query_Timer_Event;
extern TQ_EVENT    Multicast_Address_Specific_Timer_Event;

/*************************************************************************
*
*   FUNCTION
*
*       IP_Delete_Multi
*
*   DESCRIPTION
*
*       Deletes an entry in the Multicast group list.
*
*   INPUTS
*
*       *ipm                    Pointer to the multicast group structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation
*       output device status
*
*************************************************************************/
STATUS IP_Delete_Multi(IP_MULTI *ipm)
{
    IP_MULTI            *ptr;
    DEV_IF_ADDRESS      *if_addr = &ipm->ipm_data.multi_device->dev_addr;
    DV_DEVICE_ENTRY     *dv_ptr = ipm->ipm_data.multi_device;
    UINT8               dev_addr[IP_ADDR_LEN];
    DV_REQ              d_req;
    INT                 old_level;
    STATUS              status = NU_SUCCESS;
    struct  sock_struct *sck_ptr;
    MULTI_SCK_OPTIONS   *moptions;
    MULTI_DEV_STATE     *dev_state;
    MULTI_SCK_STATE     *sck_state = NU_NULL;

    /*  Temporarily lockout interrupts. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    if (--ipm->ipm_data.multi_refcount == 0)
    {
        /* There are no remaining claims to this record; let IGMP know that
         * we are leaving the multicast group.
         */
        IGMP_Leave(ipm);

        /* Check for a valid device address list head. */
        if(ipm->ipm_data.multi_device->dev_addr.dev_addr_list.
                dv_head != NU_NULL)
        {
            PUT32(dev_addr, 0,
                  ipm->ipm_data.multi_device->dev_addr.dev_addr_list.
                  dv_head->dev_entry_ip_addr);
        }

        /* Delete the route to this multicast address using the specified
         * device address as the next-hop.
         */
        if (RTAB4_Delete_Route(ipm->ipm_addr, dev_addr) != NU_SUCCESS)
            NLOG_Error_Log("Failed to delete route for multicast address",
                           NERR_SEVERE, __FILE__, __LINE__);

        /* Get a pointer to the first entry in the list. */
        ptr = if_addr->dev_multiaddrs;

        /* If the first entry is the one being deleted. */
        if (ptr == ipm)
        {
            /* Set the first one to the head of the list. */
            if_addr->dev_multiaddrs = ipm->ipm_next;

            /* Update the device's multicast address structure pointer. */
            dv_ptr->dev_multi_addr = if_addr->dev_multiaddrs;
         }

         else
         {
             /* Find the target entry to remove. */
             while (ptr)
             {
                 /* If the next one is the one being deleted. */
                 if (ptr->ipm_next == ipm)
                 {
                     /* Set the next pointer to the next entry after the one
                      * being removed.
                      */
                     ptr->ipm_next = ptr->ipm_next->ipm_next;

                     break;
                  }

                  ptr = ptr->ipm_next;
              }
          }

        d_req.dvr_addr = GET32(ipm->ipm_addr, 0);
        d_req.dvr_flags = 0;

        /* Restore the previous interrupt lockout level.  */
        NU_Local_Control_Interrupts(old_level);

        /* Notify the driver to update its multicast reception filter. */
        if (ipm->ipm_data.multi_device->dev_ioctl)
        {
            status =
            (*ipm->ipm_data.multi_device->dev_ioctl)(ipm->ipm_data.multi_device,
                                                     DEV_DELMULTI, &d_req);
        }

        /* Find the correct device state structure to remove from
         * the device.
         */
        dev_state = ipm->ipm_data.multi_device->dev_igmp_state_list.head;

        while (dev_state)
        {
            if (memcmp(ipm->ipm_addr, dev_state->dev_multiaddr,
                       IP_ADDR_LEN) == 0)
            {
                /* Get a ptr to the sck_state structure */
                if (dev_state->dev_sck_state_list.head ==
                    dev_state->dev_sck_state_list.tail)
                    sck_state = dev_state->dev_sck_state_list.head;

                if (sck_state)
                {
                    /* Get socket ptr */
                    sck_ptr = SCK_Sockets[sck_state->sck_socketd];

                    /* Get a ptr to the multicast socket options */
                    moptions = sck_ptr->s_moptions_v4;

                    /* Decrement the number of groups that the socket is a
                     * member of.
                     */
                    moptions->multio_num_mem_v4--;

                    /* If this is the last member of the group, Deallocate
                     * the structure.
                     */
                    if (moptions->multio_num_mem_v4 == 0)
                    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                        if (NU_Deallocate_Memory(moptions) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to deallocate memory for multicast structure",
                                            NERR_SEVERE, __FILE__, __LINE__);
#else
                        /* Reset the memory region to its initialized value */
                        UTL_Zero(&NET_Amm_Memory[sck_state->sck_socketd],
                                    sizeof(MULTI_SCK_OPTIONS));
#endif
                        sck_ptr->s_moptions_v4 = NU_NULL;
                    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

                    /* Remove the sck_state source list memory and the multicast addr. */
                    status = NU_Deallocate_Memory(sck_state->sck_multi_addr);

                    if (status == NU_SUCCESS)
#else
                    /* Set the memory in the array back to its initialized value */
                    UTL_Zero(NET_Amm_Sck_State_Memory_Src_List[sck_state->sck_socketd],
                                (IP_ADDR_LEN + (MAX_MULTICAST_SRC_ADDR * IP_ADDR_LEN)));
#endif

                    {
                        /* Remove the structure from the link list before
                         * deallocating the memory.
                         */
                        DLL_Remove(&dev_state->dev_sck_state_list, sck_state);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                        /* Remove the device state structure. */
                        if (NU_Deallocate_Memory(sck_state) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to deallocate memory for multicast structure",
                                           NERR_SEVERE, __FILE__, __LINE__);

#else
                        /* Reset the memory array to its initialized value */
                        UTL_Zero(NET_Amm_Sck_State_Memory[sck_state->sck_socketd],
                                    IP_MAX_MEMBERSHIPS);
#endif
                    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                    else
                        NLOG_Error_Log("Failed to deallocate memory for multicast structure",
                                       NERR_SEVERE, __FILE__, __LINE__);
#endif
                }
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

                /* Remove the device's source list memory */
                status = NU_Deallocate_Memory(dev_state->dev_multiaddr);

                if (status == NU_SUCCESS)

#else
                /* Reset the memory array to its initialized value */
                UTL_Zero(NET_MC_Group_Memory_Src_List[(UINT8)(ipm - NET_MC_Group_Memory)],
                    (IP_ADDR_LEN + (2 * (MAX_MULTICAST_SRC_ADDR * IP_ADDR_LEN))));

                status = NU_SUCCESS;
#endif
                {
                    /* Remove the structure from the link list before
                     * deallocating the memory.
                     */
                    DLL_Remove(&ipm->ipm_data.multi_device->dev_igmp_state_list,
                               dev_state);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                    /* Remove the device state structure. */
                    if (NU_Deallocate_Memory(dev_state) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to deallocate memory for multicast structure",
                                       NERR_SEVERE, __FILE__, __LINE__);
#else

                    /* Reset the memory array to its initialized value */
                    UTL_Zero(&NET_MC_Group_Memory_Dev_State[(UINT8)(ipm - NET_MC_Group_Memory)],
                        sizeof(MULTI_DEV_STATE));
#endif
                }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                else
                    NLOG_Error_Log("Failed to deallocate memory for multicast structure",
                                   NERR_SEVERE, __FILE__, __LINE__);
#endif

                break;
            }
            else
                dev_state = dev_state->dev_state_next;
        }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        /* Deallocate the memory from the ip_multi structure. */
        if (NU_Deallocate_Memory(ipm->ipm_addr) == NU_SUCCESS)
        {
            if (NU_Deallocate_Memory(ipm) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for multicast structure",
                               NERR_SEVERE, __FILE__, __LINE__);
        }
        else
            NLOG_Error_Log("Failed to deallocate memory for multicast structure",
                           NERR_SEVERE, __FILE__, __LINE__);

#else
        /* Reset the memory array to its initialized value */
        UTL_Zero(NET_MC_Group_Memory_IP_Multi_Src_List[(UINT8)(ipm - NET_MC_Group_Memory)],
                    IP_ADDR_LEN + (MAX_MULTICAST_SRC_ADDR * IP_ADDR_LEN));

        /* Turn the memory flag off to indicate the memory is now unused */
        NET_MC_Group_Memory_Flags[(UINT8)(ipm - NET_MC_Group_Memory)] = NU_FALSE;

#endif
    }
    else
    {
        /*  Restore the previous interrupt lockout level.  */
        NU_Local_Control_Interrupts(old_level);
    }

    return (status);

} /* IP_Delete_Multi */

/***********************************************************************
*
*   FUNCTION
*
*       IGMP_Leave
*
*   DESCRIPTION
*
*       This function kills any timer events that are pending when
*       membership in a multicast group is dropped and sends a
*       LEAVE message if we are the last host to send a report
*       for the group.
*
*   INPUTS
*
*       *ipm                    A pointer to the multicast group
*                               structure.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IGMP_Leave(IP_MULTI *ipm)
{
    STATUS  status;

    /* If there is a timer event pending to send a group membership report,
     * clear the timer event.
     */
    if (ipm->ipm_data.multi_timer)
    {
        ipm->ipm_data.multi_timer = 0;

        /* Unset the currently running timer for this multicast group */
        status = TQ_Timerunset(Multicast_General_Query_Timer_Event,
                               TQ_CLEAR_EXACT,
                               (UNSIGNED)(ipm->ipm_data.multi_device->dev_index),
                               (UNSIGNED)(ipm->ipm_data.multi_timer_index));

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to stop the multicast timer",
                           NERR_SEVERE, __FILE__, __LINE__);

            NET_DBG_Notify(status, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }

        status = TQ_Timerunset(Multicast_Address_Specific_Timer_Event,
                               TQ_CLEAR_EXACT,
                               (UNSIGNED)(ipm->ipm_data.multi_device->dev_index),
                               (UNSIGNED)(ipm->ipm_data.multi_timer_index));

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to stop the multicast timer",
                           NERR_SEVERE, __FILE__, __LINE__);

            NET_DBG_Notify(status, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }
    }

    /* Set the state to non-listener */
    ipm->ipm_data.multi_state = NON_LISTENER;

    /* If we were the last host to send a report for the group that
     * we are about to leave, then we should send a LEAVE message to the
     * all-routers group.  This is done because we may be the last host
     * member of the group.
     */
    if (ipm->ipm_data.multi_sent_last_report == NU_TRUE)
    {
        /* Send a report of the membership.*/
        if (ipm->ipm_data.multi_device->dev_igmp_compat_mode ==
            IGMPV2_COMPATIBILITY)
        {
            if (IGMP_Send(ipm, IGMP_ALL_ROUTERS_GROUP,
                          IGMP_GROUP_LEAVE_MESSAGE, 0) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to transmit IGMP message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

}/* IGMP_Leave */

#endif
