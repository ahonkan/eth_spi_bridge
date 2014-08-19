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
*       ip6_mc_dm.c                                  
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains those functions necessary to maintain the IP
*       layer for IPv6.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Delete_Multi
*                                                                          
*   DEPENDENCIES                                                             
*              
*       externs.h
*       nerrs.h
*       mld6.h
*                                                                          
*************************************************************************/

#include "networking/externs.h"
#include "networking/nerrs.h"
#include "networking/mld6.h"


#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Memory (Arrays) already declared in ip6_mc.c */
extern IP6_MULTI        NET6_MC_Group_Memory[];
extern UINT8            NET6_MC_Group_Memory_Flags[];
extern UINT8            NET6_MC_Group_Memory_IP_Multi_Src_List[NET_MAX_MULTICAST_GROUPS][IP6_ADDR_LEN + 
                                (MAX_MULTICAST_SRC_ADDR * IP6_ADDR_LEN)];
extern MULTI_DEV_STATE  NET6_MC_Group_Memory_Dev_State[];
extern UINT8            NET6_MC_Group_Memory_Src_List[NET_MAX_MULTICAST_GROUPS][(IP6_ADDR_LEN + 
                                (2 * (MAX_MULTICAST_SRC_ADDR * IP6_ADDR_LEN)))];

/* Memory (Arrays) already declared in ip6_pml.c */
extern MULTI_SCK_OPTIONS    NET6_Amm_Memory[];
extern MULTI_SCK_STATE      NET6_Amm_Sck_State_Memory[NSOCKETS][IP6_MAX_MEMBERSHIPS];
extern UINT8                NET6_Amm_Sck_State_Memory_Src_List[NSOCKETS][IP6_ADDR_LEN + 
                                (MAX_MULTICAST_SRC_ADDR * IP6_ADDR_LEN)];
#endif

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Delete_Multi
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Deletes an entry in the Multicast group list.                    
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *ip6_multi              A pointer to an IPv6 Multicast data 
*                               structure.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              The multicast address was deleted.
*       output device status
*                                                                       
*************************************************************************/
STATUS IP6_Delete_Multi(IP6_MULTI *ip6_multi)
{
    IP6_MULTI           **ptr;
    DV_REQ              d_req;
    INT                 old_level;
    STATUS              status = NU_SUCCESS;
    UINT8               *next_hop;
    RTAB6_ROUTE_ENTRY   *rt;
    MULTI_DEV_STATE     *dev_state;
    MULTI_SCK_STATE     *sck_state = NU_NULL;
    struct  sock_struct *sck_ptr;
    MULTI_SCK_OPTIONS   *moptions;

    /*  Temporarily lockout interrupts. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    ip6_multi->ipm6_data.multi_refcount--;

    if (ip6_multi->ipm6_data.multi_refcount == 0)
    {
        /* There are no remaining claims to this record; let MLD know that
         * we are leaving the multicast group. 
         */
        MLD6_Stop_Listening_Multi(ip6_multi);

        /* Remove this one from the list. */
        for (ptr = &(ip6_multi->ipm6_data.multi_device->dev6_multiaddrs);
             *ptr != ip6_multi;
             ptr = &(*ptr)->ipm6_next)
            continue;

        /* Unlink the element */
        *ptr = (*ptr)->ipm6_next;

        NU_BLOCK_COPY(d_req.dvr_dvru.dvru_addrv6, ip6_multi->ipm6_addr, 
                      IP6_ADDR_LEN);

        d_req.dvr_flags = DEV_MULTIV6;

        /*  Restore the previous interrupt lockout level.  */
        NU_Local_Control_Interrupts(old_level);

        /* Notify the driver to update its multicast reception filter. */
        status = (*ip6_multi->ipm6_data.multi_device->dev_ioctl)
                 (ip6_multi->ipm6_data.multi_device, DEV_DELMULTI, &d_req);

        /* Find the node that contains this route and remove it. */
        rt = RTAB6_Find_Route(ip6_multi->ipm6_addr, RT_HOST_MATCH | RT_OVERRIDE_METRIC |
                              RT_OVERRIDE_RT_STATE | RT_OVERRIDE_DV_STATE);

        if (rt)
        {
        	/* Decrement the reference count of the route. */
        	rt->rt_entry_parms.rt_parm_refcnt--;

            /* Get the address to the next hop for the route that we are about to delete */
            next_hop = rt->rt_next_hop.sck_addr;

            /* Remove the route for the multicast address */
            status = RTAB6_Delete_Route(ip6_multi->ipm6_addr, next_hop);

            if (status != NU_SUCCESS)
                NLOG_Error_Log("Failed to remove the route used by the multicast address", 
                                NERR_SEVERE, __FILE__, __LINE__);
        }
                
        else
            NLOG_Error_Log("Failed to find the route used by the multicast address", 
                            NERR_SEVERE, __FILE__, __LINE__);

        /* Find the correct device state structure to remove from 
         *  the device.
         */
        dev_state = ip6_multi->ipm6_data.multi_device->dev6_mld_state_list.head;
        while (dev_state)
        {
            if (memcmp(ip6_multi->ipm6_addr, dev_state->dev_multiaddr, IP6_ADDR_LEN) == 0)
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
                    moptions = sck_ptr->s_moptions_v6;
                    
                    /* Decrement the number of groups that the socket is a
                     *  member of.
                     */
                    moptions->multio_num_mem_v6--;

                    /* If this is the last member of the group, Deallocate
                     *  the structure.
                     */
                    if (moptions->multio_num_mem_v6 == 0)
                    {

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                        if (NU_Deallocate_Memory(moptions) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to deallocate memory for multicast structure", 
                            NERR_SEVERE, __FILE__, __LINE__);
#else
                        /* Reset the memory region to its initialized value */
                        UTL_Zero(&NET6_Amm_Memory[sck_state->sck_socketd], 
                                    sizeof(MULTI_SCK_OPTIONS));
#endif

                        /* Initialize the entry */
                        sck_ptr->s_moptions_v6 = NU_NULL;
                    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                    /* Remove the sck_state source list memory and the multicast addr. */
                    status = NU_Deallocate_Memory(sck_state->sck_multi_addr);

#else
                    /* Set the memory in the array back to its initialized value */
                    UTL_Zero(NET6_Amm_Sck_State_Memory_Src_List[sck_state->sck_socketd],
                        (IP6_ADDR_LEN + (MAX_MULTICAST_SRC_ADDR * IP6_ADDR_LEN)));

                    status = NU_SUCCESS;
#endif

                    if (status == NU_SUCCESS)
                    {
                        /* Remove the structure from the link list before 
                         *  deallocating the memory.
                         */
                        DLL_Remove(&dev_state->dev_sck_state_list, sck_state);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                        /* Remove the device state structure. */
                        if (NU_Deallocate_Memory(sck_state) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to deallocate memory for multicast structure", 
                                NERR_SEVERE, __FILE__, __LINE__);

#else
                        /* Reset the memory array to its initialized value */
                        UTL_Zero(NET6_Amm_Sck_State_Memory[sck_state->sck_socketd],
                            IP6_MAX_MEMBERSHIPS);
#endif

                    }
                    else
                        NLOG_Error_Log("Failed to deallocate memory for multicast structure", 
                        NERR_SEVERE, __FILE__, __LINE__);

                }
                
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                /* Remove the device's source list memory */
                status = NU_Deallocate_Memory(dev_state->dev_multiaddr);
#else
                /* Reset the memory array to its initialized value */
                UTL_Zero(NET6_MC_Group_Memory_Src_List[(UINT8)(ip6_multi - NET6_MC_Group_Memory)], 
                    (IP6_ADDR_LEN + (2 * (MAX_MULTICAST_SRC_ADDR * IP6_ADDR_LEN))));

                status = NU_SUCCESS;
#endif

                if (status == NU_SUCCESS)
                {
                    /* Remove the structure from the link list before 
                     *  deallocating the memory.
                     */
                    DLL_Remove(&ip6_multi->ipm6_data.multi_device->dev6_mld_state_list, 
                                dev_state);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                    /* Remove the device state structure. */
                    if (NU_Deallocate_Memory(dev_state) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to deallocate memory for multicast structure", 
                            NERR_SEVERE, __FILE__, __LINE__);
#else

                    /* Reset the memory array to its initialized value */
                    UTL_Zero(&NET6_MC_Group_Memory_Dev_State[(UINT8)(ip6_multi - NET6_MC_Group_Memory)], 
                                sizeof(MULTI_DEV_STATE));
#endif

                }
                else
                    NLOG_Error_Log("Failed to deallocate memory for multicast structure", 
                            NERR_SEVERE, __FILE__, __LINE__);

                break;
            }
            else
                dev_state = dev_state->dev_state_next;
        }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        /* Deallocate the memory from the ip_multi structure. */
        if (NU_Deallocate_Memory(ip6_multi->ipm6_addr) == NU_SUCCESS)
        {
            if (NU_Deallocate_Memory(ip6_multi) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for multicast structure", 
                    NERR_SEVERE, __FILE__, __LINE__);
        }
        else
            NLOG_Error_Log("Failed to deallocate memory for multicast structure", 
                    NERR_SEVERE, __FILE__, __LINE__);

#else
        /* Reset the memory array to its initialized value */
        UTL_Zero(NET6_MC_Group_Memory_IP_Multi_Src_List[(UINT8)(ip6_multi - NET6_MC_Group_Memory)],
                IP6_ADDR_LEN + (MAX_MULTICAST_SRC_ADDR * IP6_ADDR_LEN));

        /* Turn the memory flag off to indicate the memory is now unused */
        NET6_MC_Group_Memory_Flags[(UINT8)(ip6_multi - NET6_MC_Group_Memory)] = NU_FALSE;

#endif    

    }
    else
        /*  Restore the previous interrupt lockout level.  */
        NU_Local_Control_Interrupts(old_level);

    return (status);          

} /* IP6_Delete_Multi */
#endif

