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
*       ip6_pml.c                                    
*
*   DESCRIPTION
*
*       This file contains the implementation to join and leave
*       a multicast group
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP6_Process_Multicast_Listen
*
*   DEPENDENCIES
*
*       nu_net.h
*       mld6.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/mld6.h"

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Declare memory for the multicast options for each socket */
MULTI_SCK_OPTIONS NET6_Amm_Memory[NSOCKETS];

/* Declare memory for the multicast socket state */
MULTI_SCK_STATE NET6_Amm_Sck_State_Memory[NSOCKETS][IP6_MAX_MEMBERSHIPS];

/* Declare memory for the src list and multicast addr for the state struct */
UINT8 NET6_Amm_Sck_State_Memory_Src_List[NSOCKETS][IP6_ADDR_LEN + 
                                    (MAX_MULTICAST_SRC_ADDR * IP6_ADDR_LEN)];
#endif

/*************************************************************************
*                                                                         
*   FUNCTION                                                              
*                                                                         
*       IP6_Process_Multicast_Listen                                                     
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
*       device_index            Device index used to find the proper device
*       *multi_addr             Ptr to the address of the multicast group
*       filter_mode             Filter mode for the socket (INCLUDE or 
*                               EXCLUDE)
*       *source_list            Ptr to the list of source addresses that 
*                               should be INCLUDED or EXCLUDED 
*                               depending on the filter mode
*       num_source              Number of source addresses in the 
*                               source_list
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS                  Indicates the socket option was set 
*                                   successfully. 
*       NU_INVALID_PARM             One of the passed-in parameters is 
*                                   invalid.
*       NU_MEM_ALLOC                If there is not enough memory to allocate
*                                   parameter is invalid.
*       NU_NOT_A_GROUP_MEMBER       Unable to join the multicast group.
*       NU_MULTI_TOO_MANY_SRC_ADDRS If the passed in number of src addresses
*                                   exceeds the MAX_MULTICAST_SRC_ADDR value
*       NU_INVAL                    An invalid operation has taken place
*                                                                         
*************************************************************************/
STATUS IP6_Process_Multicast_Listen(INT socketd, UINT32 device_index, 
                                    const UINT8 *multi_addr, UINT16 filter_mode, 
                                    const UINT8 *source_list, UINT16 num_source_addr)
{
    STATUS                  ret_status = NU_INVALID_PARM;
    struct  sock_struct     *sck_ptr = SCK_Sockets[socketd];
    MULTI_SCK_OPTIONS       *moptions = NU_NULL;
    DV_DEVICE_ENTRY         *device;
    MULTI_SCK_STATE         *sck6_state;
#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
    MULTI_DEV_STATE         *dev_state;
    MULTI_SCK_STATE         *sck6_state_head;
    INT                     message_to_send;
#endif
    UINT16                  j;
    UINT8                   i;
    UINT8                   leaving_group = NU_FALSE;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)  
    UINT8                   *src_list;
#endif

    /* Verify the data that has been passed in is correct */
    if ( (IPV6_IS_ADDR_MULTICAST(multi_addr)) && (sck_ptr != NU_NULL) &&
         (!((num_source_addr > 0) && (source_list == NU_NULL))) )
    {
        /* Check to see if this is a call to leave a multicast group */
        if ( (filter_mode == MULTICAST_FILTER_INCLUDE) && 
             (num_source_addr == 0) )
             leaving_group = NU_TRUE;

        /* Is there a multicast option buffer attached to the socket. */
        if (sck_ptr->s_moptions_v6 == NU_NULL)
        {
            if (leaving_group == NU_FALSE)
            {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)     
                /* Allocate a multicast option buffer. */
                ret_status = NU_Allocate_Memory(MEM_Cached, 
                                                (VOID**)&sck_ptr->s_moptions_v6,
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
                sck_ptr->s_moptions_v6 = &NET6_Amm_Memory[socketd]; 
#endif

                /* Get a local ptr to the MULTI_SCK_OPTIONS struct */
                moptions = sck_ptr->s_moptions_v6;

                /* Initialize the option buffer to the default values. */
                UTL_Zero(moptions, sizeof(MULTI_SCK_OPTIONS));
                moptions->multio_ttl = IP6_DEFAULT_MULTICAST_TTL;
                moptions->multio_loop = IP6_DEFAULT_MULTICAST_LOOP;
                moptions->multio_num_mem_v6 = 0;
            }

            else
                ret_status = NU_NOT_A_GROUP_MEMBER;
        }

        else
            moptions = sck_ptr->s_moptions_v6;

        /* Get a ptr to the device */
        device = DEV_Get_Dev_By_Index(device_index);

        if (moptions != NU_NULL)
        {
            /* See if membership already exists or if all the membership 
             * slots are full. 
             */
            for (i = 0; i < moptions->multio_num_mem_v6; i++)
            {
                if ( (moptions->multio_v6_membership[i]->ipm6_data.multi_device == 
                                                   device) &&
                     (memcmp(moptions->multio_v6_membership[i]->ipm6_addr, 
                             multi_addr, IP6_ADDR_LEN) == 0) )
                    break;
            }

            /* Create a new membership */
            if ( (i >= moptions->multio_num_mem_v6) && (i < IP6_MAX_MEMBERSHIPS) )
            {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)     
                /* Allocate a multicast socket state buffer. */
                ret_status =  NU_Allocate_Memory(MEM_Cached, (VOID**)&sck6_state,
                                                 sizeof(MULTI_SCK_STATE), 
                                                 (UNSIGNED)NU_NO_SUSPEND);

                if (ret_status == NU_SUCCESS)
                {
                    ret_status =  NU_Allocate_Memory(MEM_Cached, (VOID**)&src_list,
                                                     (IP6_ADDR_LEN + (MAX_MULTICAST_SRC_ADDR * 
                                                      IP6_ADDR_LEN)), 
                                                     (UNSIGNED)NU_NO_SUSPEND);

                    if (ret_status == NU_SUCCESS)
                    {
                        /* Assign memory for the multicast address in the 
                         * socket state struct 
                         */
                        sck6_state->sck_multi_addr = src_list;

                        /* Assign memory for the src list in the socket state 
                         * struct 
                         */
                        sck6_state->sck_src_list = src_list + IP6_ADDR_LEN;
                    }
                    else
                    {
                        NLOG_Error_Log("Unable to allocate memory", NERR_SEVERE,
                                       __FILE__, __LINE__);

                        /* De-allocate the sck_state memory */
                        if (NU_Deallocate_Memory(sck6_state) != NU_SUCCESS)
                            NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                           __FILE__, __LINE__);
                    }
                }
                else
                    NLOG_Error_Log("Unable to allocate memory", NERR_SEVERE,
                                   __FILE__, __LINE__);
#else
                /* Assign memory to the multicast socket state buffer */
                sck6_state = 
                      &NET6_Amm_Sck_State_Memory[socketd][moptions->multio_num_mem_v6];
        
                /* Assign memory for the multicast address in the socket 
                 *  state struct 
                 */
                sck6_state->sck_multi_addr = &NET6_Amm_Sck_State_Memory_Src_List[socketd];

                /* Assign memory for the src list in the socket state struct */
                sck6_state->sck_src_list = 
                    &NET6_Amm_Sck_State_Memory_Src_List[socketd] + IP6_ADDR_LEN;

                ret_status = NU_SUCCESS;
#endif

                if (ret_status == NU_SUCCESS)
                {
                    /* Clear the two link list ptrs.  If we do not do this,
                     *  we will encounter an error when we go to insert this
                     *  new member onto the list */
                    sck6_state->sck_state_next = sck6_state->sck_state_prev = NU_NULL;

                    /* Copy the socket descriptor into the struct */
                    sck6_state->sck_socketd = socketd;

                    /* Copy the multicast address into the struct */
                    memcpy(sck6_state->sck_multi_addr, multi_addr, IP6_ADDR_LEN);
                    
                    /* Copy the filter mode into the struct */
                    sck6_state->sck_filter_state = filter_mode;
        
                    /* Copy the number of sources into the structure */
                    sck6_state->sck_num_src_addr = (UINT16)num_source_addr;

                    /* Set the family type of this structure */
                    sck6_state->sck_family_type = NU_FAMILY_IP6;
                    
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
                            /* We can not add this group due to exceeding the
                             * max number of source addresses.  De-allocate
                             * the memory so that we can error out of the function.
                             */
                            if (NU_Deallocate_Memory(sck6_state) != NU_SUCCESS)
                                NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                               __FILE__, __LINE__);

                            if (NU_Deallocate_Memory(src_list) != NU_SUCCESS)
                                NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                               __FILE__, __LINE__);

                            if (NU_Deallocate_Memory(sck_ptr->s_moptions_v6) != NU_SUCCESS)
                                NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                               __FILE__, __LINE__);
#endif

                            sck_ptr->s_moptions_v6 = NU_NULL;

                            /* Log an error */
                            NLOG_Error_Log("Exceeded max number of src addrs",
                                           NERR_INFORMATIONAL, __FILE__, __LINE__);

                            ret_status = NU_MULTI_TOO_MANY_SRC_ADDRS;
                        }
                        
                        else
                        {
                            /* Copy the source addresses into the structure */
                            for (j = 0; j < num_source_addr; j++)
                                memcpy((sck6_state->sck_src_list + (j * IP6_ADDR_LEN)), 
                                       (source_list + (j * IP6_ADDR_LEN)), IP6_ADDR_LEN);
                        }
                    }

                    if (ret_status != NU_MULTI_TOO_MANY_SRC_ADDRS)
                    {
                        /* Add a new record to the multicast address list for 
                         * the interface. 
                         */
                        moptions->multio_v6_membership[i] = 
                                IP6_Add_Multi(multi_addr, device, sck6_state);

                        if (moptions->multio_v6_membership[i] != NU_NULL)
                        {
                            /* Increment the number of memberships to which
                             * the socket belongs 
                             */
                            moptions->multio_num_mem_v6++;
                            
                            /* Set the return status */
                            ret_status = NU_SUCCESS;
                        }
                        else
                            ret_status = NU_INVAL;
                    }
                }
                else
                {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)     
                    /* Deallocate the  memory */
                    if (NU_Deallocate_Memory(sck_ptr->s_moptions_v6) != NU_SUCCESS)
                        NLOG_Error_Log("Unable to deallocate memory", NERR_SEVERE,
                                       __FILE__, __LINE__);
#endif
                    sck_ptr->s_moptions_v6 = NU_NULL;

                    ret_status = NU_INVAL;
                }
            }

            /* The membership already exists. */
            else if (i < IP6_MAX_MEMBERSHIPS)
            {
                /* This call could be to leave the multicast group.  Check to see 
                 * if this is the case.
                 */
                if (leaving_group == NU_TRUE)
                {
                    /* We are leaving the group */
                    if (i != moptions->multio_num_mem_v6)
                    {
                        /* Find the socket state structure */
                        sck6_state = 
                            Multi_Get_Sck_State_Struct(multi_addr, moptions, 
                                                        device, socketd, 
                                                        NU_FAMILY_IP6);

                        if (sck6_state != NU_NULL)
                        {
#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
                            /* Get a ptr to the device state structure
                             *  so that we can remove the sck state 
                             *  structure from it's list. 
                             */
                            dev_state = Multi_Get_Device_State_Struct(device,
                                      moptions->multio_v6_membership[i]->ipm6_addr,
                                      IP6_ADDR_LEN);

                            /* Get a pointer to the head of the sck_state 
                             * list.  We will pass this into the update 
                             * device function instead of the sck_state 
                             * that is being removed.
                             */
                            sck6_state_head = dev_state->dev_sck_state_list.head;

                            /* Update the device's state to reflect this 
                             * socket leaving the group 
                             */
                            message_to_send = 
                                Multi_Update_Device_State(device, 
                                                          multi_addr, 
                                                          sck6_state_head, 
                                                          MULTICAST_LEAVE_GROUP, 
                                                          IP6_ADDR_LEN);

                            /* If the return statement is 
                             * MULTICAST_NO_MSG_TO_SEND, then the 
                             * device's state has been properly updated.
                             */
                            if ( (message_to_send != NU_MULTI_TOO_MANY_SRC_ADDRS) && 
                                 (message_to_send != MULTICAST_NO_MSG_TO_SEND) )
                                MLD6_Start_Listening(moptions->multio_v6_membership[i], 
                                                     (UINT8)message_to_send);

                            else if (message_to_send == NU_MULTI_TOO_MANY_SRC_ADDRS)
                                /* An error occurred while updating the 
                                 *  device state.  Return the error.
                                 */
                                return (message_to_send);
#endif
                            /* Delete the multicast address record to which the 
                             * membership points. 
                             */
                            ret_status = 
                                IP6_Delete_Multi(moptions->multio_v6_membership[i]);


                            if ( (ret_status == NU_SUCCESS) && 
                                 (moptions->multio_num_mem_v6) )
                            {
                                /* Remove the now empty space in the membership 
                                 * array. 
                                 */
                                i++;

                                for (; (i < moptions->multio_num_mem_v6) && 
								     (i < IP6_MAX_MEMBERSHIPS); i++)
								{
                                    moptions->multio_v6_membership[i - 1] = 
                                        moptions->multio_v6_membership[i];
								}

                                /* Clear the last entry */
                                moptions->multio_v6_membership[i - 1] = NU_NULL;
                            }

                            else
                                /* We just removed the last entry.  Set it to NULL */
                                moptions->multio_v6_membership[0] = NU_NULL;

                        } /* if (sck_state != NU_NULL) */

                        /* If we were unable to find a matching socket structure,
                         * then the socket was not a member of the specified group.
                         */
                        else
                            ret_status = NU_NOT_A_GROUP_MEMBER;
                    }
                    else
                        ret_status = NU_INVAL;
                }
        
#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
                /* If an entry is already present and we are not leaving the group, 
                 * then we must be changing the filter mode or the source list. 
                 */
                else 
                {
                    /* Find the socket state structure */
                    sck6_state = 
                        Multi_Get_Sck_State_Struct(multi_addr, moptions, 
                                                    device, socketd, NU_FAMILY_IP6);

                    if (sck6_state)
                    {
                        /* Copy the filter mode into the struct */
                        sck6_state->sck_filter_state = filter_mode;
        
                        /* If there are any source addresses to be filtered, copy 
                         * them into the structure 
                         */
                        if (num_source_addr > 0)
                        {
                            /* Make sure that we are not exceeding the max number of
                             * of source addresses allowed 
                             */
                            if (num_source_addr > MAX_MULTICAST_SRC_ADDR)
                            {
                                /* We can not add this group due to exceeding the
                                 * max number of source addresses.  De-allocate
                                 * the memory so that we can error out of 
                                 * the function.
                                 */
                                NU_Deallocate_Memory(sck6_state);
                                NU_Deallocate_Memory(src_list);
                                NU_Deallocate_Memory(sck_ptr->s_moptions_v6);

                                sck_ptr->s_moptions_v6 = NU_NULL;

                                /* Log an error */
                                NLOG_Error_Log("Exceeded max number of src addrs",
                                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                                ret_status = NU_MULTI_TOO_MANY_SRC_ADDRS;
                            }

                            /* Copy the number of sources into the structure */
                            sck6_state->sck_num_src_addr = num_source_addr;

                            /* Copy the source addresses into the structure */
                            for (i = 0; i < num_source_addr; i++)
                                memcpy(&sck6_state->sck_src_list[i], &source_list[i], 
                                       IP6_ADDR_LEN);
                        }

                        if ( (ret_status != NU_MULTI_TOO_MANY_SRC_ADDRS) &&
                             (ret_status != NU_INVALID_PARM) )
                        {
                            /* Update the device's state */
                            message_to_send = 
                                Multi_Update_Device_State(device, multi_addr, sck6_state, 
                                                          MULTICAST_UPDATE_GROUP,  
                                                          IP6_ADDR_LEN);

                            /* A error occurred while updating the device
                             * state.  Set the ret_status to the error.
                             */
                            if (message_to_send < MULTICAST_NO_MSG_TO_SEND)
                                ret_status = message_to_send;

                            else if (message_to_send > NU_SUCCESS)
                            {
                                MLD6_Start_Listening(moptions->multio_v6_membership[i], 
                                                     (UINT8)message_to_send);

                                /* Set the return status as successful */
                                ret_status = NU_SUCCESS;
                            }
                        }
                    } /* if (sck6_state) */

                    else
                    {        
                        NLOG_Error_Log("Unable to find sck state struct", 
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);
                    
                        ret_status = NU_INVAL;
                    }
                }
#endif  /* #if (MLD6_DEFAULT_COMPATIBILITY_MODE == MLDV2_COMPATIBILITY) */
            } /* else if ( (i < IP6_MAX_MEMBERSHIPS) &&... */

            /* Trying to add too many groups */
            else                 
                ret_status =  NU_TOO_MANY_GROUP_MEMBERS;
        }
    }

    else
        ret_status = NU_INVALID_PARM;

    return (ret_status);

} /* IP6_Process_Multicast_Listen */
#endif
