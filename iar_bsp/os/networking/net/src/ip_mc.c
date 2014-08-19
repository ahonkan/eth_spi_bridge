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
*       ip_mc.c
*
* DESCRIPTION
*
*       This file contains routines for IP multicasting.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IP_Add_Multi
*       IP_Lookup_Multi
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_TRUE) )

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for the multicast groups */
IP_MULTI NET_MC_Group_Memory[NET_MAX_MULTICAST_GROUPS];

/* Declare memory for the multicast group addr and src list */
UINT8 NET_MC_Group_Memory_IP_Multi_Src_List[NET_MAX_MULTICAST_GROUPS][IP_ADDR_LEN +
                                    (MAX_MULTICAST_SRC_ADDR * IP_ADDR_LEN)];

/* Declare flag array for the memory declare above */
UINT8 NET_MC_Group_Memory_Flags[NET_MAX_MULTICAST_GROUPS] ={0};

/* Declare memory for the device state struct */
MULTI_DEV_STATE NET_MC_Group_Memory_Dev_State[NET_MAX_MULTICAST_GROUPS];

/* Declare memory for the src lists and multicast addr in the
 *  device structure.
 */
UINT8 NET_MC_Group_Memory_Src_List[NET_MAX_MULTICAST_GROUPS][(IP_ADDR_LEN +
                                    (2 * (MAX_MULTICAST_SRC_ADDR * IP_ADDR_LEN)))];
#endif

/***********************************************************************
*
*   FUNCTION
*
*       IP_Add_Multi
*
*   DESCRIPTION
*
*       Adds an entry into the multicast group list.
*
*   INPUTS
*
*       m_addr                  Multicast address (in host byte order)
*       *dev                    Pointer to device structure
*
*   OUTPUTS
*
*       NU_NULL                 Failure
*       IP_MULTI*               Successful operation
*
*************************************************************************/
IP_MULTI *IP_Add_Multi(UINT32 m_addr, DV_DEVICE_ENTRY *dev,
                       MULTI_SCK_STATE *sck_state)
{
    IP_MULTI            *ipm;
    IP_MULTI            *saved_addr;
    MULTI_DEV_STATE     *device_state = NU_NULL;
    DEV_IF_ADDRESS      *if_addr = &dev->dev_addr;
    STATUS              status;
    DV_REQ              d_req;
    INT                 old_level;
    INT                 message_to_send;
    UINT8               multi_addr[IP_ADDR_LEN];
#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    INT                 i;              /* Counter to traverse an array */
#else
    UINT8               *src_lists = NU_NULL, *org_src_lists = NU_NULL;
    UINT8               *ipm_list = NU_NULL;
#endif

#if (IGMP_DEFAULT_COMPATIBILTY_MODE != IGMPV3_COMPATIBILITY)
    UNUSED_PARAMETER (sck_state);
#endif

    /*  Temporarily lockout interrupts. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Determine if the interface is already a member of the group. */
    ipm = IP_Lookup_Multi(m_addr, if_addr);

    if (ipm != NU_NULL)
    {
        /* The interface is already a member of the group. Increment the
         *  reference count.
         */
        ipm->ipm_data.multi_refcount++;

        /* Get a ptr to the device state struct so that we can add our
         *  sck_state struct to it's sck_state list.
         */
        device_state = Multi_Get_Device_State_Struct(dev, ipm->ipm_addr,
                                                    IP_ADDR_LEN);

        /* Add the socket state to the device's socket state list if
         *  it is not null.  The socket state will be null if this is a
         *  membership of the all-hosts or all-routers group.
         */
        if ( (sck_state != NU_NULL) && (device_state != NU_NULL) )
            DLL_Enqueue(&device_state->dev_sck_state_list, sck_state);

        /*  Restore the previous interrupt lockout level.  */
        NU_Local_Control_Interrupts(old_level);
    }
    else
    {

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
         /* This is a new group membership request. Allocate memory
          * for it.
          */
        status = NU_Allocate_Memory(MEM_Cached, (VOID **)&ipm,
                                    sizeof(*ipm),
                                    (UNSIGNED)NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Allocate memory for the IP_MULTI addr and src list */
            status = NU_Allocate_Memory(MEM_Cached, (VOID **)&ipm_list,
                                        (IP_ADDR_LEN +
                                        (MAX_MULTICAST_SRC_ADDR * IP_ADDR_LEN)),
                                        (UNSIGNED)NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Set the mem ptr for the multicast addr */
                ipm->ipm_addr = ipm_list;

                /* Set the mem ptr for the src addr list */
                ipm->ipm_query_src_list = ipm_list + IP_ADDR_LEN;
            }

            /* Allocate memory for the device state for this multicast
             * group */
            status = NU_Allocate_Memory(MEM_Cached, (VOID **)&device_state,
                                        sizeof(MULTI_DEV_STATE),
                                        (UNSIGNED)(NU_NO_SUSPEND));

            if (status == NU_SUCCESS)
            {
                /* Allocate memory for the device state for this multicast
                 * group */
                status = NU_Allocate_Memory(MEM_Cached, (VOID **)&src_lists,
                                            (IP_ADDR_LEN +
                                            (2 * (MAX_MULTICAST_SRC_ADDR *
                                                  IP_ADDR_LEN))),
                                            (UNSIGNED)(NU_NO_SUSPEND));

                if (status == NU_SUCCESS)
                {
                	/* Save the original pointer in case we need to deallocate
                	 * this memory.
                	 */
                	org_src_lists = src_lists;

                    /* Set the mem ptr to the memory for the multicast
                     *  addr
                     */
                    device_state->dev_multiaddr = src_lists;

                    /* Increment the mem ptr */
                    src_lists += IP_ADDR_LEN;

                    /* Set a mem ptr to the location for the device src
                     *  list
                     */
                    device_state->dev_src_addrs = src_lists;

                    /* Increment the mem ptr */
                    src_lists += (MAX_MULTICAST_SRC_ADDR * IP_ADDR_LEN);

                    /* Set the mem ptr to the location for the device list
                     *   of src addrs to report
                     */
                    device_state->dev_src_to_report = src_lists;

                    /* Initialize the sck state list */
                    device_state->dev_sck_state_list.head =
                        device_state->dev_sck_state_list.tail = NU_NULL;
                }
            }
        }

#else
        /* Traverse the flag array to find the unused memory location */
        for (i=0;
             (NET_MC_Group_Memory_Flags[i] != NU_FALSE) && (i != NET_MAX_MULTICAST_GROUPS);
             i++)
            ;
        if (i != NET_MAX_MULTICAST_GROUPS)
        {
            /* Assign the unused memory to the group membership request */
            ipm = &NET_MC_Group_Memory[i];

            /* Set a mem ptr for the multicast addr */
            ipm->ipm_addr = (UINT8 *)NET_MC_Group_Memory_IP_Multi_Src_List[i];

            /* Set a mem ptr for the src addr list */
            ipm->ipm_query_src_list =
                   (UINT8 *)(NET_MC_Group_Memory_IP_Multi_Src_List[i] +  IP_ADDR_LEN);

            /* Turn the memory flag on */
            NET_MC_Group_Memory_Flags[i] = NU_TRUE;
            status = NU_SUCCESS;

            /* Assign the unused memory to the device state struct */
            device_state = &NET_MC_Group_Memory_Dev_State[i];

            /* Set the mem ptr to the memory for the multicast
             *  addr
             */
            device_state->dev_multiaddr =
                                (UINT8 *)NET_MC_Group_Memory_Src_List[i];

            /* Set a mem ptr to the location for the device src
             *  list
             */
            device_state->dev_src_addrs =
                (UINT8 *)(NET_MC_Group_Memory_Src_List[i] + IP_ADDR_LEN);

            /* Set the mem ptr to the location for the device list
             *   of src addrs to report
             */
            device_state->dev_src_to_report =
                (UINT8 *)(NET_MC_Group_Memory_Src_List[i] +
                         (IP_ADDR_LEN + (MAX_MULTICAST_SRC_ADDR * IP_ADDR_LEN)));
        }
        else
            status = NU_NO_MEMORY;
#endif

        if (status != NU_SUCCESS)
        {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

        	/* Clean up the previously allocated memory. */
        	if (ipm)
        	{
        		if (NU_Deallocate_Memory(ipm) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to deallocate memory for multicast structure",
                                   NERR_SEVERE, __FILE__, __LINE__);

        		if (ipm_list)
        		{
        			if (NU_Deallocate_Memory(ipm_list) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to deallocate memory for multicast structure",
                                       NERR_SEVERE, __FILE__, __LINE__);

        			if (device_state)
        			{
        				if (NU_Deallocate_Memory(device_state) != NU_SUCCESS)
        	                NLOG_Error_Log("Failed to deallocate memory for multicast structure",
        	                               NERR_SEVERE, __FILE__, __LINE__);

        				if (org_src_lists)
        				{
        					if (NU_Deallocate_Memory(org_src_lists) != NU_SUCCESS)
        		                NLOG_Error_Log("Failed to deallocate memory for multicast structure",
        		                               NERR_SEVERE, __FILE__, __LINE__);
        				}
        			}
        		}
        	}
#endif

            NU_Local_Control_Interrupts(old_level);

            return (NU_NULL);
        }

        /* Initialize the multicast group. */
        PUT32(ipm->ipm_addr, 0, m_addr);
        ipm->ipm_data.multi_device = dev;
        ipm->ipm_data.multi_refcount = 1;
        ipm->ipm_data.multi_timer_index = 0;
        ipm->ipm_data.multi_timer = 0;

         /* Save the start of the list */
        saved_addr = if_addr->dev_multiaddrs;

        /* Link it into the group membership list for this interface. */
        ipm->ipm_next = if_addr->dev_multiaddrs;
        if_addr->dev_multiaddrs = ipm;

        /* Now, add the IP_MULTI ptr to the DV_DEVICE_ENTRY struct */
        dev->dev_multi_addr = ipm;

        /* Copy the multicast address into the struct */
        memcpy(device_state->dev_multiaddr, ipm->ipm_addr, IP_ADDR_LEN);

        /* Initialize the device filter mode to be INCLUDE with
         *  a src list of 0.  This will represent not listening to
         *  to the group.  Multi_Update_Device_State will put the
         *  proper values into the structure for the new socket
         *  that is joining the group.
         */
        device_state->dev_filter_state = MULTICAST_FILTER_INCLUDE;

        device_state->dev_num_src_addr = 0;

        /* Add the new device state to the device's link list */
        DLL_Enqueue(&dev->dev_igmp_state_list, device_state);


        /* Add the socket state to the device's socket state list if
         *  it is not null.  The socket state will be null if this is a
         *  membership of the all-hosts or all-routers group.
         */
        if (sck_state != NU_NULL)
            DLL_Enqueue(&device_state->dev_sck_state_list, sck_state);

        /*  Restore the previous interrupt lockout level.  */
        NU_Local_Control_Interrupts(old_level);

        UTL_Zero(&d_req, sizeof(DV_REQ));

        d_req.dvr_addr = m_addr;
        d_req.dvr_flags = 0;

        /* Now ask the driver to update its multicast reception filter. */
        if ( (dev->dev_ioctl == NU_NULL) ||
             ((*dev->dev_ioctl)(dev, DEV_ADDMULTI, &d_req) != NU_SUCCESS) )
        {
            /*  Temporarily lockout interrupts. */
            old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Since we were unsuccessful in updating the device's
             * reception filter, restore the IP_MULTI list back to it's
             * previous state.
             */
            if_addr->dev_multiaddrs = saved_addr;

            /* Restore the DV_DEVICE_ENTRY struct to it's previous state */
            dev->dev_multi_addr = saved_addr;

            /* Remove the new device state from the device's linked list */
            DLL_Remove(&dev->dev_igmp_state_list, device_state);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
            /* Deallocate all previously allocated memory. */
            if (NU_Deallocate_Memory(ipm) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for multicast structure",
                               NERR_SEVERE, __FILE__, __LINE__);

			if (NU_Deallocate_Memory(ipm_list) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for multicast structure",
                               NERR_SEVERE, __FILE__, __LINE__);

			if (NU_Deallocate_Memory(device_state) != NU_SUCCESS)
				NLOG_Error_Log("Failed to deallocate memory for multicast structure",
							   NERR_SEVERE, __FILE__, __LINE__);

			if (NU_Deallocate_Memory(org_src_lists) != NU_SUCCESS)
				NLOG_Error_Log("Failed to deallocate memory for multicast structure",
							   NERR_SEVERE, __FILE__, __LINE__);

#else
            /* Turn off the memory flag to show that the memory is
             * unused now
             */
            NET_MC_Group_Memory_Flags[(UINT8)(ipm - NET_MC_Group_Memory)] = NU_FALSE;
#endif
            /*  Restore the previous interrupt lockout level.  */
            NU_Local_Control_Interrupts(old_level);

            return (NU_NULL);
        }

        /* Add a route to this multicast address. */
        if (RTAB4_Add_Route(dev, m_addr, 0xffffffffUL,
                            dev->dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr,
                            (INT32)(RT_UP | RT_HOST | RT_SILENT |
                            RT_STATIC | RT_LOCAL)) != NU_SUCCESS)
            NLOG_Error_Log("Failed to add route for multicast address",
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Convert the multicast address into an array of UINT8's so that it
     * can be passed into the update device function.
     */
    PUT32(multi_addr, 0, m_addr);

#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
    /* Update the device's state to see if we need to send a message */
    message_to_send =
        Multi_Update_Device_State(dev, multi_addr, sck_state,
                                  MULTICAST_JOIN_GROUP, IP_ADDR_LEN);

    /* If an error occurred while updating the device state, we must return
     *  a NULL ptr.
     */
    if (message_to_send == NU_MULTI_TOO_MANY_SRC_ADDRS)
        ipm = NU_NULL;

#else
    if (ipm->ipm_data.multi_device->dev_igmp_compat_mode == IGMPV1_COMPATIBILITY)
        message_to_send = IGMPV1_HOST_MEMBERSHIP_REPORT;
    else
        message_to_send = IGMPV2_HOST_MEMBERSHIP_REPORT;
#endif

    if ( (ipm) && (message_to_send > 0) )
        /* Inform IGMP of the new group membership. */
            IGMP_Join(ipm, (UINT8)message_to_send);

    return (ipm);

} /* IP_Add_Multi */

/***********************************************************************
*
*   FUNCTION
*
*       IP_Lookup_Multi
*
*   DESCRIPTION
*
*       Searches through the multicast group list for a match.
*
*   INPUTS
*
*       m_addr                  Target multicast address
*       *if_addr                A pointer to the structure which tells
*                               whether the device allows multicast
*                               addresses
*
*   OUTPUTS
*
*       IP_MULTI*               Pointer to the multicast address
*       NU_NULL                 The address does not exist on any device.
*
*************************************************************************/
IP_MULTI *IP_Lookup_Multi(UINT32 m_addr, const DEV_IF_ADDRESS *if_addr)
{
    IP_MULTI            *ipm;

    /* Search the multicast group list for a match. */
    for (ipm = if_addr->dev_multiaddrs;
         ipm != NU_NULL && (GET32(ipm->ipm_addr, 0) != m_addr);
         ipm = ipm->ipm_next )
    {
        ;
    }

    return (ipm);

} /* IP_Lookup_Multi */

#endif
