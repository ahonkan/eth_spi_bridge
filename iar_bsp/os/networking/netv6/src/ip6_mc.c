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
*       ip6_mc.c                                     
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains routines for IPv6 multicasting.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Add_Multi
*       IP6_Allocate_Multi
*       IP6_Lookup_Multi
*                                                                          
*   DEPENDENCIES                                                             
*              
*       externs.h
*       mld6.h
*                                                                          
*************************************************************************/

#include "networking/externs.h"
#include "networking/mld6.h"

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for the multicast groups */
IP6_MULTI NET6_MC_Group_Memory[NET_MAX_MULTICAST_GROUPS];

/* Declare memory for the multicast group addr and src list */
UINT8 NET6_MC_Group_Memory_IP_Multi_Src_List[NET_MAX_MULTICAST_GROUPS][IP6_ADDR_LEN + 
                                    (MAX_MULTICAST_SRC_ADDR * IP6_ADDR_LEN)];

/* Declare flag array for the memory declare above */
UINT8 NET6_MC_Group_Memory_Flags[NET_MAX_MULTICAST_GROUPS] ={0};

/* Declare memory for the device state struct */
MULTI_DEV_STATE NET6_MC_Group_Memory_Dev_State[NET_MAX_MULTICAST_GROUPS];

/* Declare memory for the src lists and multicast addr in the 
 *  device structure.
 */
UINT8 NET6_MC_Group_Memory_Src_List[NET_MAX_MULTICAST_GROUPS][(IP6_ADDR_LEN + 
                                    (2 * (MAX_MULTICAST_SRC_ADDR * IP6_ADDR_LEN)))];
#endif

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       IP6_Add_Multi
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function adds a multicast address to the multicast group
*       associated with the device and sends a MLD report to join
*       the group on the link as necessary.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *m_addr                 A pointer to the multicast address.
*       *dev                    A pointer to the device entry.
*       *sck_state              A pointer to the multicast state 
*                               structure.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       ipm                     A pointer to the multicast structure.
*       NU_NULL                 The address was not added to the group.
*
*************************************************************************/
IP6_MULTI *IP6_Add_Multi(const UINT8 *m_addr, DV_DEVICE_ENTRY *dev, 
                         MULTI_SCK_STATE *sck_state)
{
	IP6_MULTI           *ipm6;

    /* Allocate the data structures for the new membership and add the
     * group to the respective lists.
     */
    ipm6 = IP6_Allocate_Multi(m_addr, dev, sck_state);

    /* If resources were successfully allocated and a message should be sent. */
    if ( (ipm6)&& (ipm6->ipm6_msg_to_send > 0) )
	{
		/* Inform MLD of the new group membership. */
		MLD6_Start_Listening(ipm6, (UINT8)ipm6->ipm6_msg_to_send);

		/* Indicate that a message has been sent. */
		ipm6->ipm6_msg_to_send = MULTICAST_NO_MSG_TO_SEND;
	}

    return (ipm6);

} /* IP6_Add_Multi */

/*************************************************************************
*
*   FUNCTION
*
*       IP6_Allocate_Multi
*
*   DESCRIPTION
*
*       This function adds a multicast address to the multicast group
*       associated with the device.
*
*   INPUTS
*
*       *m_addr                 A pointer to the multicast address.
*       *dev                    A pointer to the device entry.
*       *sck_state              A pointer to the multicast state
*                               structure.
*
*   OUTPUTS
*
*       ipm                     A pointer to the multicast structure.
*       NU_NULL                 The address was not added to the group.
*
*************************************************************************/
IP6_MULTI *IP6_Allocate_Multi(const UINT8 *m_addr, DV_DEVICE_ENTRY *dev,
        				      MULTI_SCK_STATE *sck_state)
{
    IP6_MULTI           *ipm6;
    IP6_MULTI           *saved_addr;
    MULTI_DEV_STATE     *device_state = NU_NULL;
    UINT8               *src_lists = NU_NULL, *ipm6_lists = NU_NULL;
    STATUS              status;
    DV_REQ              d_req;
    INT                 old_level;
#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    INT                 i;                  /* Counter to traverse an array */
#endif

#if (MLD6_DEFAULT_COMPATIBILTY_MODE != MLDV2_COMPATIBILITY)
    UNUSED_PARAMETER(sck_state);
#endif

	/*  Temporarily lockout interrupts. */
	old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

	/* Determine if the interface is already a member of the group. */
	ipm6 = IP6_Lookup_Multi(m_addr, dev->dev6_multiaddrs);

	if (ipm6 != NU_NULL)
	{
		/* The interface is already a member of the group. Increment the
		   reference count. */
		ipm6->ipm6_data.multi_refcount++;

		/* Get a ptr to the device state struct so that we can add our
		 *  sck_state struct to it's sck_state list.
		 */
		device_state = Multi_Get_Device_State_Struct(dev, ipm6->ipm6_addr,
													 IP6_ADDR_LEN);

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
		 /* This is a new group membership request. Allocate memory for it. */
		status = NU_Allocate_Memory(MEM_Cached, (VOID **)&ipm6,
									sizeof(IP6_MULTI),
									(UNSIGNED)NU_NO_SUSPEND);

		if (status == NU_SUCCESS)
		{
			/* Allocate memory for the multi address and src list */
			status = NU_Allocate_Memory(MEM_Cached, (VOID **)&ipm6_lists,
										(IP6_ADDR_LEN +
										 (MAX_MULTICAST_SRC_ADDR * IP6_ADDR_LEN)),
										(UNSIGNED)(NU_NO_SUSPEND));

			if (status == NU_SUCCESS)
			{
				/* Set a mem ptr to the multicast addr in the multi struct */
				ipm6->ipm6_addr = ipm6_lists;

				/* Set a mem ptr to the multicast query addr list */
				ipm6->ipm6_query_src_list = ipm6_lists + IP6_ADDR_LEN;

				/* Allocate memory for the device state for this multicast group */
				status = NU_Allocate_Memory(MEM_Cached, (VOID **)&device_state,
											sizeof(MULTI_DEV_STATE),
											(UNSIGNED)(NU_NO_SUSPEND));

				if (status == NU_SUCCESS)
				{
					/* Allocate memory for the device state multi addr and src lists */
					status = NU_Allocate_Memory(MEM_Cached, (VOID **)&src_lists,
												(2 * (IP6_ADDR_LEN + (MAX_MULTICAST_SRC_ADDR *
												 IP6_ADDR_LEN))),
												(UNSIGNED)(NU_NO_SUSPEND));

					if (status == NU_SUCCESS)
					{
						/* Set the mem ptr to the memory for the multicast
						 *  addr
						 */
						device_state->dev_multiaddr = src_lists;

						/* Set a mem ptr to the location for the device src
						 *  list
						 */
						device_state->dev_src_addrs = src_lists + IP6_ADDR_LEN;

						/* Set the mem ptr to the location for the device list
						 *   of src addrs to report
						 */
						device_state->dev_src_to_report =
							src_lists + (IP6_ADDR_LEN + (MAX_MULTICAST_SRC_ADDR * IP6_ADDR_LEN));

						/* Initialize the socket state list */
						device_state->dev_sck_state_list.head =
							device_state->dev_sck_state_list.tail = NU_NULL;
					}
				}
			}
		}
#else
		/* Traverse the flag array to find the unused memory location */
		for (i=0; (NET6_MC_Group_Memory_Flags[i] != NU_FALSE) &&
			 (i != NET_MAX_MULTICAST_GROUPS); i++)
			;

		if (i != NET_MAX_MULTICAST_GROUPS)
		{
			/* Assign the unused memory to the group membership request */
			ipm6 = &NET6_MC_Group_Memory[i];

			/* Set a ptr to the  unused memory location for the multicast addr */
			ipm6->ipm6_addr = &NET6_MC_Group_Memory_IP_Multi_Src_List[i];

			/* Set a ptr to the unused memory location for the query src list */
			ipm6->ipm6_query_src_list =
					&NET6_MC_Group_Memory_IP_Multi_Src_List + IP6_ADDR_LEN;

			/* Turn the memory flag on */
			NET6_MC_Group_Memory_Flags[i] = NU_TRUE;

			/* Assign the unused memory to the device state struct */
			device_state = &NET6_MC_Group_Memory_Dev_State[i];

			/* Set the mem ptr to the memory for the multicast
			 *  addr
			 */
			device_state->dev_multiaddr = &NET6_MC_Group_Memory_Src_List[i];

			/* Set a mem ptr to the location for the device src
			 *  list
			 */
			device_state->dev_src_addrs =
				&NET6_MC_Group_Memory_Src_List[i] + IP6_ADDR_LEN;

			/* Set the mem ptr to the location for the device list
			 *   of src addrs to report
			 */
			device_state->dev_src_to_report = &NET6_MC_Group_Memory_Src_List[i] +
				(IP6_ADDR_LEN + (MAX_MULTICAST_SRC_ADDR * IP6_ADDR_LEN));

			status = NU_SUCCESS;
		}
		else
			status = NU_NO_MEMORY;
#endif

		if (status != NU_SUCCESS)
		{
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

			/* Clean up the previously allocated memory. */
			if (ipm6)
			{
				if (NU_Deallocate_Memory(ipm6) != NU_SUCCESS)
					NLOG_Error_Log("Failed to deallocate memory for multicast structure",
								   NERR_SEVERE, __FILE__, __LINE__);

				if (ipm6_lists)
				{
					if (NU_Deallocate_Memory(ipm6_lists) != NU_SUCCESS)
						NLOG_Error_Log("Failed to deallocate memory for multicast structure",
									   NERR_SEVERE, __FILE__, __LINE__);

					if (device_state)
					{
						if (NU_Deallocate_Memory(device_state) != NU_SUCCESS)
							NLOG_Error_Log("Failed to deallocate memory for multicast structure",
										   NERR_SEVERE, __FILE__, __LINE__);

						if (src_lists)
						{
							if (NU_Deallocate_Memory(src_lists) != NU_SUCCESS)
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
		memcpy(ipm6->ipm6_addr, m_addr, IP6_ADDR_LEN);
		ipm6->ipm6_data.multi_device = dev;
		ipm6->ipm6_data.multi_refcount = 1;
		ipm6->ipm6_msg_to_send = 0;

		/* Save the start of the list */
		saved_addr = dev->dev6_multiaddrs;

		/* Link it into the group membership list for this interface. */
		ipm6->ipm6_next = dev->dev6_multiaddrs;
		dev->dev6_multiaddrs = ipm6;

		/* Copy the multicast address into the struct */
		memcpy(device_state->dev_multiaddr, ipm6->ipm6_addr, IP6_ADDR_LEN);

		/* Initialize the device filter mode to be INCLUDE with
		 *  a src list of 0.  This will represent not listening to
		 *  to the group.  Multi_Update_Device_State will put the
		 *  proper values into the structure for the new socket
		 *  that is joining the group.
		 */
		device_state->dev_filter_state = MULTICAST_FILTER_INCLUDE;

		device_state->dev_num_src_addr = 0;

		/* Add the new device state to the device's link list */
		DLL_Enqueue(&dev->dev6_mld_state_list, device_state);

		 /* Add the socket state to the device's socket state list if
		 *  it is not null.  The socket state will be null if this is a
		 *  membership of the all-hosts or all-routers group.
		 */
		if (sck_state != NU_NULL)
			DLL_Enqueue(&device_state->dev_sck_state_list, sck_state);

		/*  Restore the previous interrupt lockout level.  */
		NU_Local_Control_Interrupts(old_level);

		UTL_Zero(&d_req, sizeof(DV_REQ));

		memcpy(d_req.dvr_dvru.dvru_addrv6, m_addr, IP6_ADDR_LEN);

		d_req.dvr_flags = DEV_MULTIV6;

		/* Now ask the driver to update its multicast reception filter. */
		if ( (dev->dev_ioctl == NU_NULL) ||
			 ((*dev->dev_ioctl)(dev, DEV_ADDMULTI, &d_req) != NU_SUCCESS) )
		{
			NLOG_Error_Log("IPv6 multicast group not joined",
						   NERR_SEVERE, __FILE__, __LINE__);

			/*  Temporarily lockout interrupts. */
			old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

			/* Since we were unsuccessful in updating the device's
			 * reception filter, restore the IP6_MULTI list back to it's
			 * previous state.
			 */
			dev->dev6_multiaddrs = saved_addr;

			/* Remove the new device state from the device's linked list */
			DLL_Remove(&dev->dev6_mld_state_list, device_state);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

			/* Deallocate all previously allocated memory. */
			if (NU_Deallocate_Memory(ipm6) != NU_SUCCESS)
				NLOG_Error_Log("Failed to deallocate memory for multicast structure",
							   NERR_SEVERE, __FILE__, __LINE__);

			if (NU_Deallocate_Memory(ipm6_lists) != NU_SUCCESS)
				NLOG_Error_Log("Failed to deallocate memory for multicast structure",
							   NERR_SEVERE, __FILE__, __LINE__);

			if (NU_Deallocate_Memory(device_state) != NU_SUCCESS)
				NLOG_Error_Log("Failed to deallocate memory for multicast structure",
							   NERR_SEVERE, __FILE__, __LINE__);

			if (NU_Deallocate_Memory(src_lists) != NU_SUCCESS)
				NLOG_Error_Log("Failed to deallocate memory for multicast structure",
							   NERR_SEVERE, __FILE__, __LINE__);

#else
			/* Turn off the memory flag to show that the memory is
			 * unused now
			 */
			NET6_MC_Group_Memory_Flags[ipm6 - NET6_MC_Group_Memory] = NU_FALSE;
#endif
			/*  Restore the previous interrupt lockout level.  */
			NU_Local_Control_Interrupts(old_level);

			return (NU_NULL);
		}

		/* Add a route for the multicast address */
		if (RTAB6_Add_Route(dev, m_addr, NU_NULL, 128,
							(RT_UP | RT_HOST | RT_SILENT | RT_LOCAL)) != NU_SUCCESS)
			NLOG_Error_Log("Failed to add route for the multicast address",
						   NERR_SEVERE, __FILE__, __LINE__);

#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)

		/* Update the device's state to see if we need to send a message */
		ipm6->ipm6_msg_to_send =
			Multi_Update_Device_State(dev, m_addr, sck_state, MULTICAST_JOIN_GROUP,
									  IP6_ADDR_LEN);

		/* If the device's filter mode did not change (message_to_send <= 0),
		 * there is no reason to send an IGMP message.
		 */
		if (ipm6->ipm6_msg_to_send == NU_MULTI_TOO_MANY_SRC_ADDRS)
			ipm6 = NU_NULL;

#else
		ipm6->ipm6_msg_to_send = MLDV1_LISTENER_REPORT;
#endif
	}

	return (ipm6);

} /* IP6_Allocate_Multi */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       IP6_Lookup_Multi
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function searches through the multicast group for a match.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *m_addr                 A pointer to the multicast address.
*       *if_addr                A pointer to the address associated with 
*                               the device.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       IP6_MULTI*              Pointer to the multicast address
*       NU_NULL                 The address does not exist in the group.
*
*************************************************************************/
IP6_MULTI *IP6_Lookup_Multi(const UINT8 *m_addr, IP6_MULTI *if_addr)
{
    IP6_MULTI            *ipm6;

    /* Search the multicast group list for a match. */
    for (ipm6 = if_addr; ipm6 != NU_NULL; ipm6 = ipm6->ipm6_next)
    {
        if (memcmp(ipm6->ipm6_addr, m_addr, IP6_ADDR_LEN) == 0)
            break;
    }

    return (ipm6);

} /* IP6_Lookup_Multi */
#endif
