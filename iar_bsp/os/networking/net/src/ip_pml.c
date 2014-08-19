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
*       ip_pml.c
*
* DESCRIPTION
*
*       This file contains the implementation to join and leave
*       a multicast group
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IP_Process_Multicast_Listen
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_TRUE) )

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Declare memory for the multicast options for each socket */
MULTI_SCK_OPTIONS NET_Amm_Memory[NSOCKETS];

/* Declare memory for the multicast socket state */
MULTI_SCK_STATE NET_Amm_Sck_State_Memory[NSOCKETS][IP_MAX_MEMBERSHIPS];

/* Declare memory for the src list and multicast addr for the state struct */
UINT8 NET_Amm_Sck_State_Memory_Src_List[NSOCKETS][IP_ADDR_LEN +
                                    (MAX_MULTICAST_SRC_ADDR * IP_ADDR_LEN)];
#endif

/*************************************************************************
*
*   FUNCTION
*
*       IP_Process_Multicast_Listen
*
*   DESCRIPTION
*
*       This function is used to join or leave a multicast group.
*       Previous SetSockOpt API calls that would join or leave a
*       multicast group have been re-routed to call this function.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *interface_addr         Ptr to the IP addr of the device to use
*       *multi_addr             Ptr to the address of the multicast group
*       filter_mode             Filter mode for the socket
*                                   (INCLUDE or EXCLUDE)
*       source_list             Ptr to the list of source addresses that
*                                   should be INCLUDED or EXCLUDED
*                                   depending on the filter mode
*       num_source_addr         Number of source addresses in the source_list
*
*   OUTPUTS
*
*       NU_SUCCESS                  Indicates the socket option was set
*                                   successfully.
*       NU_INVAL                    Either the optval or optlen parameter
*                                       has problems.
*       NU_MEM_ALLOC                If there is not enough memory to allocate
*                                   parameter is invalid.
*       NU_MULTI_TOO_MANY_SRC_ADDRS If the passed in number of src addresses
*                                   exceeds the MAX_MULTICAST_SRC_ADDR value
*
*************************************************************************/
STATUS IP_Process_Multicast_Listen(INT socketd, UINT8 *interface_addr,
                                   UINT8 *multi_addr, UINT16 filter_mode,
                                   const UINT8 *source_list,
                                   UINT16 num_source_addr)
{
    STATUS                  ret_status = NU_INVALID_PARM;
    INT                     message_to_send;
    struct  sock_struct     *sck_ptr = SCK_Sockets[socketd];
    MULTI_SCK_OPTIONS       *moptions;
    DV_DEVICE_ENTRY         *device;
    DV_DEVICE_ENTRY         *dev_last_resort = NU_NULL;
    MULTI_SCK_STATE         *sck_state;
#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
    MULTI_DEV_STATE         *dev_state;
    MULTI_SCK_STATE         *sck_state_head;
#endif
    UINT8                   i;
    UINT16                  j;
    UINT32                  multicast_addr32, interface_addr32;
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    UINT8                   *src_list;
#endif

    /* Due to some legacy functions, we will repeatedly need the multicast
     *  and interface addresses in the UINT32 format.  Get local versions
     *  for them here.
     */
    interface_addr32 = GET32(interface_addr, 0);

    multicast_addr32 = GET32(multi_addr, 0);

    if ( (IP_MULTICAST_ADDR(multicast_addr32)) || (sck_ptr != NU_NULL) )
    {
        /* Is there a multicast option buffer attached to the socket. */
        if (sck_ptr->s_moptions_v4 == NU_NULL)
        {

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
            /* Allocate a multicast option buffer. */
            ret_status = NU_Allocate_Memory(MEM_Cached,
                                            (VOID**)&sck_ptr->s_moptions_v4,
                                            sizeof(MULTI_SCK_OPTIONS),
                                            (UNSIGNED)NU_NO_SUSPEND);
            if (ret_status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to allocate memory", NERR_SEVERE,
                                __FILE__, __LINE__);

                return (ret_status);
            }
#else
            /* Assign memory to the multicast option buffer */
            sck_ptr->s_moptions_v4 = &NET_Amm_Memory[socketd];
#endif

            /* Get a local ptr to the MULTI_SCK_OPTIONS struct */
            moptions = sck_ptr->s_moptions_v4;

            /* Initialize the option buffer to the default values. */
            UTL_Zero(moptions, sizeof(MULTI_SCK_OPTIONS));
            moptions->multio_ttl = IP_DEFAULT_MULTICAST_TTL;
            moptions->multio_loop = IP_DEFAULT_MULTICAST_LOOP;
            moptions->multio_num_mem_v4 = 0;
        }

        else
            moptions = sck_ptr->s_moptions_v4;

        /* Was a specific interface requested. */
        if (interface_addr32 == IP_ADDR_ANY)
        {
            /* If no interface address was given then use the first
             * registered interface that is multicast capable, and
             * which is not the loop back interface.
             */
            for (device = DEV_Table.dv_head;
                 device != NU_NULL;
                 device = device->dev_next)
            {
                if (device->dev_flags & DV_MULTICAST)
                {
                    /* If a loopback interface is present which is
                     * multicast capable then save a pointer to it in
                     * case no other multicast interfaces are found.
                     * Else we have found a match so break.
                     */
                    if (device->dev_type == DVT_LOOP)
                        dev_last_resort = device;
                    else
                        break;
                }
            }

            /* If a non-loopback interface was not found then set it
             * equal to the loopback. Note, it is possible that
             * dev_last_resort will also be NULL.
             */
            if (!device)
                device = dev_last_resort;
        }
        else
        {
            /* A specific interface was requested. Find it. */
            for (device = DEV_Table.dv_head;
                 device != NU_NULL;
                 device = device->dev_next)
            {
                /* Check that this is the interface and that the interface
                 * is multicast capable.
                 */
                if ( (DEV_Find_Target_Address(device, interface_addr32)) &&
                     (device->dev_flags & DV_MULTICAST) )
                    break;
            }
        }

        /* Was an interface found? */
        if (device)
        {
            /* See if membership already exists or if all the membership
             * slots are full.
             */
            for (i = 0; i < moptions->multio_num_mem_v4; i++)
            {
                if ( (moptions->multio_v4_membership[i]->ipm_data.multi_device == device) &&
                     (memcmp(moptions->multio_v4_membership[i]->ipm_addr,
                             multi_addr, IP_ADDR_LEN) == 0) )

                    break;
            }

            /* Create a new membership */
            if ( (ret_status != NU_NOT_A_GROUP_MEMBER) &&
                 (i >= moptions->multio_num_mem_v4) && (i < IP_MAX_MEMBERSHIPS) )
            {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                /* Allocate a multicast socket state buffer. */
                ret_status =  NU_Allocate_Memory(MEM_Cached, (VOID**)&sck_state,
                                                 sizeof(MULTI_SCK_STATE),
                                                 (UNSIGNED)NU_NO_SUSPEND);

                if (ret_status == NU_SUCCESS)
                {
                    ret_status =  NU_Allocate_Memory(MEM_Cached, (VOID**)&src_list,
                                                     (IP_ADDR_LEN +
                                                     (MAX_MULTICAST_SRC_ADDR *
                                                      IP_ADDR_LEN)),
                                                     (UNSIGNED)NU_NO_SUSPEND);

                    if (ret_status == NU_SUCCESS)
                    {
                        /* Assign memory for the multicast address in the
                         * socket state struct
                         */
                        sck_state->sck_multi_addr = src_list;

                        /* Assign memory for the src list in the socket state
                         * struct
                         */
                        sck_state->sck_src_list = src_list + IP_ADDR_LEN;
                    }
                    else
                    {
                        NLOG_Error_Log("Unable to allocate memory", NERR_SEVERE,
                                       __FILE__, __LINE__);

                        /* De-allocate the sck_state memory */
                        if (NU_Deallocate_Memory(sck_state) != NU_SUCCESS)
                            NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                           __FILE__, __LINE__);
                    }
                }
                else
                    NLOG_Error_Log("Unable to allocate memory", NERR_SEVERE,
                                   __FILE__, __LINE__);

                if (ret_status != NU_SUCCESS)
                {
                    /* Deallocate the  memory */
                    if (NU_Deallocate_Memory(sck_ptr->s_moptions_v4) != NU_SUCCESS)
                        NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                       __FILE__, __LINE__);

                    sck_ptr->s_moptions_v4 = NU_NULL;

                    ret_status = NU_INVAL;
                }

                else

#else
                /* Assign memory to the multicast socket state buffer */
                sck_state =
                      &NET_Amm_Sck_State_Memory[socketd][moptions->multio_num_mem_v4];

                /* Assign memory for the multicast address in the socket
                 *  state struct
                 */
                sck_state->sck_multi_addr = (UINT8 *)NET_Amm_Sck_State_Memory_Src_List[socketd];

                /* Assign memory for the src list in the socket state struct */
                sck_state->sck_src_list =
                    (UINT8 *)(NET_Amm_Sck_State_Memory_Src_List[socketd] + IP_ADDR_LEN);

                ret_status = NU_SUCCESS;
#endif

                {
                    /* Clear the two link list ptrs.  If we do not do this,
                     *  we will encounter an error when we go to insert this
                     *  new member onto the list */
                    sck_state->sck_state_next = sck_state->sck_state_prev = NU_NULL;

                    /* Copy the socket descriptor into the struct */
                    sck_state->sck_socketd = socketd;

                    /* Copy the multicast address into the struct */
                    memcpy(sck_state->sck_multi_addr, multi_addr, IP_ADDR_LEN);

                    /* Copy the filter mode into the struct */
                    sck_state->sck_filter_state = filter_mode;

                    /* Copy the number of sources into the structure */
                    sck_state->sck_num_src_addr = num_source_addr;

                    /* Set the family type of this structure */
                    sck_state->sck_family_type = NU_FAMILY_IP;

                    /* If there are any source addresses to be filtered, copy
                     *  them into the structure
                     */
                    if (num_source_addr > 0)
                    {
                        /* Make sure that we are not exceeding the max number of
                         *  of source addresses allowed
                         */
                        if (num_source_addr > MAX_MULTICAST_SRC_ADDR)
                        {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                            /* We cannot add this group due to exceeding the
                             * max number of source addresses.  De-allocate
                             * the memory so that we can error out of the function.
                             */
                            if (NU_Deallocate_Memory(sck_state->sck_multi_addr) != NU_SUCCESS)
                                NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                               __FILE__, __LINE__);

                            if (NU_Deallocate_Memory(sck_ptr->s_moptions_v4) != NU_SUCCESS)
                                NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                               __FILE__, __LINE__);

                            if (NU_Deallocate_Memory(sck_state) != NU_SUCCESS)
                                NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                               __FILE__, __LINE__);
#endif
                            sck_ptr->s_moptions_v4 = NU_NULL;

                            /* Log an error */
                            NLOG_Error_Log("Exceeded max number of src addrs",
                                           NERR_INFORMATIONAL, __FILE__, __LINE__);

                            ret_status = NU_MULTI_TOO_MANY_SRC_ADDRS;
                        }

                        else
                        {
                            /* Copy the source addresses into the structure */
                            for (j = 0; j < num_source_addr; j++)
                                memcpy((sck_state->sck_src_list + (j * IP_ADDR_LEN)),
                                       (source_list + (j * IP_ADDR_LEN)), IP_ADDR_LEN);
                        }
                    }

                    if (ret_status != NU_MULTI_TOO_MANY_SRC_ADDRS)
                    {
                        /* Add a new record to the multicast address list for
                         * the interface.
                         */
                        moptions->multio_v4_membership[i] =
                            IP_Add_Multi(multicast_addr32, device, sck_state);

                        if (moptions->multio_v4_membership[i] != NU_NULL)
                        {
                            /* Increment the number of memberships to which
                             *   the socket belongs
                             */
                            moptions->multio_num_mem_v4++;

                            /* Set the return status */
                            ret_status = NU_SUCCESS;
                        }
                        else
                            ret_status = NU_INVAL;
                    }
                }
            }

            /* The membership already exists. */
            else if ( (i < IP_MAX_MEMBERSHIPS) &&
                      (ret_status != NU_NOT_A_GROUP_MEMBER) )
            {
                /* This call could be to leave the multicast group.  Check to see
                 * if this is the case.
                 */
                if ( (filter_mode == MULTICAST_FILTER_INCLUDE) &&
                     (num_source_addr == 0) )
                {
                    /* We are leaving the group */
                    if (i != moptions->multio_num_mem_v4)
                    {
                        /* Find the socket state structure */
                        sck_state = Multi_Get_Sck_State_Struct(multi_addr,
                                                               moptions,
                                                               device,
                                                               socketd,
                                                               NU_FAMILY_IP);

                        if (sck_state != NU_NULL)
                        {
#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
                            /* Get a ptr to the device state structure
                             *  so that we can remove the sck state
                             *  structure from it's list.
                             */
                            dev_state = Multi_Get_Device_State_Struct(device,
                                      moptions->multio_v4_membership[i]->ipm_addr,
                                      IP_ADDR_LEN);

                            /* Get a pointer to the head of the sck_state
                             *  list.  We will pass this into the update
                             *  device function instead of the sck_state
                             *  that is being removed.
                             */
                            sck_state_head = dev_state->dev_sck_state_list.head;

                            /* Update the device's state to reflect this
                             * socket leaving the group
                             */
                            message_to_send =
                                Multi_Update_Device_State(device,
                                                          multi_addr,
                                                          sck_state_head,
                                                          MULTICAST_LEAVE_GROUP,
                                                          IP_ADDR_LEN);
#else
                            message_to_send = MULTICAST_NO_MSG_TO_SEND;
#endif
                            /* If the return statement is
                             *  MULTICAST_NO_MSG_TO_SEND, then the
                             *  device's state has been properly updated.
                             */
                            if ( (message_to_send != NU_MULTI_TOO_MANY_SRC_ADDRS) &&
                                 (message_to_send != MULTICAST_NO_MSG_TO_SEND) &&
                                 (device->dev_igmp_compat_mode == IGMPV3_COMPATIBILITY) )
                                IGMP_Join(moptions->multio_v4_membership[i],
                                          (UINT8)message_to_send);

                            else if (message_to_send == NU_MULTI_TOO_MANY_SRC_ADDRS)
                                /* We encountered an error in updating the
                                 *  the device state.  Return that error
                                 */
                                return (message_to_send);

                            /* Delete the multicast address record to which the
                             * membership points.
                             */
                            ret_status =
                                IP_Delete_Multi(moptions->multio_v4_membership[i]);

                            if (ret_status == NU_SUCCESS)
                            {
                                /* Remove the now empty space in the
                                 *  membership array.
                                 */
                                i++;

                                for (; (i < IP_MAX_MEMBERSHIPS); i++)
                                {
                                    moptions->multio_v4_membership[i - 1] =
                                        moptions->multio_v4_membership[i];
                                }

                                /* Clear the last entry */
                                moptions->multio_v4_membership[i - 1] = NU_NULL;
                            }
                        } /* if (sck_state != NU_NULL */

                        /* If we were unable to find a matching socket structure,
                         *  then the socket was not a member of the specified group.
                         */
                        else
                            ret_status = NU_NOT_A_GROUP_MEMBER;
                    }
                    else
                        ret_status = NU_INVAL;
                }

                /* If an entry is already present and we are not leaving the group,
                 * then we must be changing the filter mode or the source list.
                 */
                else
                {
                    /* Find the socket state structure */
                    sck_state =
                        Multi_Get_Sck_State_Struct(multi_addr, moptions, device,
                                                   socketd, NU_FAMILY_IP);

                    if (sck_state)
                    {
                        /* Copy the filter mode into the struct */
                        sck_state->sck_filter_state = filter_mode;

#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
                        /* If there are any source addresses to be filtered, copy
                         * them into the structure
                         */
                        if (num_source_addr > 0)
                        {
                            /* Make sure that we are not exceeding the max number of
                             *  of source addresses allowed
                             */
                            if (num_source_addr > MAX_MULTICAST_SRC_ADDR)
                            {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                                /*  We cannot add this group due to exceeding the
                                 *  max number of source addresses.  De-allocate
                                 *  the memory so that we can error out of
                                 *  the function.
                                 */
                                if (NU_Deallocate_Memory(sck_state->sck_multi_addr) != NU_SUCCESS)
                                    NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                                   __FILE__, __LINE__);

                                if (NU_Deallocate_Memory(sck_ptr->s_moptions_v4) != NU_SUCCESS)
                                    NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                                   __FILE__, __LINE__);

                                if (NU_Deallocate_Memory(sck_state) != NU_SUCCESS)
                                    NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                                   __FILE__, __LINE__);
#endif
                                sck_ptr->s_moptions_v4 = NU_NULL;

                                /* Log an error */
                                NLOG_Error_Log("Exceeded max number of src addrs",
                                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                                ret_status = NU_MULTI_TOO_MANY_SRC_ADDRS;
                            }

                            /* Copy the number of sources into the structure */
                            sck_state->sck_num_src_addr = num_source_addr;

                            /* Copy the source addresses into the structure */
                            for (i = 0; i < num_source_addr; i++)
                                memcpy(&sck_state->sck_src_list[i],
                                       &source_list[i], IP_ADDR_LEN);
                        }

                        if (ret_status != NU_MULTI_TOO_MANY_SRC_ADDRS)
                        {
                            /* Update the device's state */
                            message_to_send =
                                Multi_Update_Device_State(device,
                                                          multi_addr, sck_state,
                                                          MULTICAST_UPDATE_GROUP,
                                                          IP_ADDR_LEN);
#else
                            if (device->dev_igmp_compat_mode == IGMPV1_COMPATIBILITY)
                                message_to_send = IGMPV1_HOST_MEMBERSHIP_REPORT;
                            else
                                message_to_send = IGMPV2_HOST_MEMBERSHIP_REPORT;
#endif

                            if (message_to_send > NU_SUCCESS)
                            {
                                /* We must send a message.  Call IGMP_Join to
                                 * handle it from here
                                 */
                                IGMP_Join(moptions->multio_v4_membership[i],
                                          (UINT8)message_to_send);

                                /* Set the return status as successful */
                                ret_status = NU_SUCCESS;
                            }

                            else
                                /* We encountered an error in updating the
                                 *  the device state.  Set the return status.
                                 */
                                ret_status = message_to_send;

#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
                        }
#endif
                    }
                    else
                    {
                        NLOG_Error_Log("Unable to find sck state struct",
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);

                        ret_status = NU_INVAL;
                    }
                }
            }

            else
                /* Trying to add too many groups */
                ret_status =  NU_TOO_MANY_GROUP_MEMBERS;
        }
        else
            ret_status = NU_INVAL;
    }

    else
        ret_status = NU_INVALID_PARM;

    return (ret_status);

} /* IP_Process_Multicast_Listen */

#endif
