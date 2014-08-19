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
*       dev6_rtr.c                                   
*                                                                       
*   DESCRIPTION                                                           
*                     
*       This file contains those functions necessary to manage the 
*       interfaces on a router-enabled node.                           
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*           
*       DEV6_Initialize_Router
*       DEV6_Configure_Router
*       DEV6_Configure_Prefix_List
*                                                                       
*   DEPENDENCIES                                                          
*               
*       target.h                                                        
*       externs.h
*       prefix6.h
*       net6.h
*       nd6.h
*       nd6radv.h
*                                                                       
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/prefix6.h"
#include "networking/nd6.h"
#include "networking/nd6radv.h"

#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

extern UINT8    IP6_Solicited_Node_Multi[];
extern UINT8    IP6_All_Routers_Multi[];
extern TQ_EVENT ICMP6_RtrSol_Event;
extern TQ_EVENT IP6_Transmit_Rtr_Adv_Event;

/**************************************************************************
*
*   FUNCTION                                                                 
*                                                                          
*       DEV6_Initialize_Router                                                     
*                                                                          
*   DESCRIPTION                                                              
*                                                                          
*       This function performs the necessary actions when an interface
*       is enabled to transmit Router Advertisements.
*                                                                          
*   INPUTS                                                                   
*                                                                          
*       *dev_ptr                A pointer to the interface.
*                                                                          
*   OUTPUTS                                                                  
*                                                                          
*       None
*                                                                          
****************************************************************************/
VOID DEV6_Initialize_Router(DV_DEVICE_ENTRY *dev_ptr)
{
    UINT32  rtr_adv_timer;

#if (INCLUDE_IP_MULTICASTING)
    
    /* Join the All-Router multicast group */
    if (IP6_Add_Multi(IP6_All_Routers_Multi, dev_ptr, NU_NULL) == NU_NULL)
        NLOG_Error_Log("Failed to join the All-Router multicast address group", 
                       NERR_SEVERE, __FILE__, __LINE__);
#endif

    /* Compute a random delay for transmitting the first Router
     * Advertisement.
     */
    rtr_adv_timer = 
        ND6_Compute_Random_Timeout(dev_ptr->dev6_MinRtrAdvInterval,
                                   dev_ptr->dev6_MaxRtrAdvInterval);

    /* RFC 4861 - section 6.2.4 - For the first few advertisements
     * if the randomly chosen interval is greater than 
     * MAX_INITIAL_RTR_ADVERT_INTERVAL, the timer SHOULD be set 
     * to MAX_INITIAL_RTR_ADVERT_INTERVAL.
     */
    if (rtr_adv_timer > IP6_MAX_INITIAL_RTR_ADVERT_INTERVAL)
        rtr_adv_timer = IP6_MAX_INITIAL_RTR_ADVERT_INTERVAL;

    /* Set the timer to transmit the first Router Advertisement */
    if (TQ_Timerset(IP6_Transmit_Rtr_Adv_Event, (UINT32)dev_ptr->dev_index, 
                    rtr_adv_timer * SCK_Ticks_Per_Second, 
                    ND6RADV_UNSOL_ADV) != NU_SUCCESS)
        NLOG_Error_Log("Failed to set timer to transmit Router Advertisement", 
                       NERR_SEVERE, __FILE__, __LINE__);

} /* DEV6_Initialize_Router */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       DEV6_Configure_Router
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function configures router-specific parameters for an 
*       IPv6-enabled interface.
*                                                                         
*   INPUTS                                                                
*                                                 
*       *device                 A pointer to the interface to configure.
*       flags                   Flags to set or remove from the interface:
*
*                               DV6_ISROUTER - This interface should 
*                                              transmit Router Advertisement 
*                                              messages and process Router 
*                                              Solicitation messages.
*
*                               DV6_ADV_MGD  - Set the Managed Address 
*                                              Configuration flag in 
*                                              outgoing Router Advertisement 
*                                              messages.
*
*                               DV6_ADV_OTH_CFG - Set the Other Stateful 
*                                                 Configuration flag in 
*                                                 outgoing Router 
*                                                 Advertisement messages.
*
*       flag_opts               Instructions on what to do with the
*                               flags specified:
*
*                               DV6_REM_FLGS    Remove the indicated flags 
*                                               from the interface.  
*                               DV6_ADD_FLGS    Add the indicated flags 
*                                               to the interface.  
*                               DV6_IGN_FLGS    Leave  the flags on the 
*                                               interface unmodified.
*       *rtr_opts               A pointer to the parameters to configure.
*                                                 
*   OUTPUTS                                                               
*                                                                         
*       None
*
*************************************************************************/
VOID DEV6_Configure_Router(DV_DEVICE_ENTRY *dev_ptr, UINT32 flags,
                           INT flag_opts, const DEV6_RTR_OPTS *rtr_opts)
{
#if (INCLUDE_IP_MULTICASTING)
    UINT32          delay;
    UINT32          prev_flags;
    IP6_MULTI       *ipm;
    UINT16          saved_lifetime;
    DEV6_IF_ADDRESS *link_local_addr;
#endif

    UINT32          rtr_adv_timer;

    /* Remove the specified flags from the device */
    if (flag_opts == DV6_REM_FLGS)
    {
#if (INCLUDE_IP_MULTICASTING)
        /* Save a pointer to the flags */
        prev_flags = dev_ptr->dev_flags;
#endif

        /* Remove the flags */
        dev_ptr->dev_flags &= ~flags;

#if (INCLUDE_IP_MULTICASTING)

        /* If the interface is no longer used to transmit Router 
         * Advertisement messages, leave the All-Routers Multicast
         * group and transmit a final Router Advertisement message
         * with a valid lifetime of zero.
         */
        if ( (prev_flags & DV6_ISROUTER) && (flags & DV6_ISROUTER) )
        {
            /* If multicasting is supported on the interface. */
            if (dev_ptr->dev_flags & DV_MULTICAST)
            {
                /* Unset the interval timer to transmit the next unsolicited 
                 * Router Advertisement.
                 */
                if (TQ_Timerunset(IP6_Transmit_Rtr_Adv_Event, TQ_CLEAR_ALL_EXTRA, 
                                  dev_ptr->dev_index, 0) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to stop the timer to transmit the next Router Advertisement", 
                                   NERR_SEVERE, __FILE__, __LINE__);

                /* Save the current lifetime value */                                  
                saved_lifetime = dev_ptr->dev6_AdvDefaultLifetime;

                /* Set the lifetime to zero so the router will not be used as a 
                 * default router by any nodes on the link.
                 */
                dev_ptr->dev6_AdvDefaultLifetime = 0;

                /* Set the next time for a Router Advertisement to be transmitted
                 * to zero.
                 */
                dev_ptr->dev6_next_radv_time = 0;

                /* Transmit the final Router Advertisement */
                ND6RADV_Output(dev_ptr);

                /* Restore the value of the lifetime */
                dev_ptr->dev6_AdvDefaultLifetime = saved_lifetime;

                /* Get a pointer to the multicast entry for the All-Routers
                 * Multicast group.
                 */
                ipm = IP6_Lookup_Multi(IP6_All_Routers_Multi, 
                                       dev_ptr->dev6_multiaddrs);

                /* If an entry was found, leave the group */
                if (ipm)
                {
                    if (IP6_Delete_Multi(ipm) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to leave the All-Router multicast address group", 
                                       NERR_SEVERE, __FILE__, __LINE__);
                }

                /* Get a pointer to the link-local address on the interface */
                link_local_addr = IP6_Find_Link_Local_Addr(dev_ptr);

                if (link_local_addr)
                {
                    /* RFC 4861 section 6.3.7 - Before a host sends an initial solicitation,
                     * it SHOULD delay the transmission for a random amount of time between 0 
                     * and MAX_RTR_SOLICITATION_DELAY.
                     */
                    delay = ICMP6_Random_Delay(link_local_addr->dev6_ip_addr, 
                                               IP6_Solicited_Node_Multi, 
                                               IP6_MAX_RTR_SOLICITATION_INTERVAL);

                    /* Set an event to send a Router Solicitation */
                    if (TQ_Timerset(ICMP6_RtrSol_Event, dev_ptr->dev_index, delay, 
                                    IP6_MAX_RTR_SOLICITATIONS) != NU_SUCCESS)
                        NLOG_Error_Log("Could not set the event to transmit an initial Router Solicitation", 
                                       NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }
#endif

    }

    /* Add the specified flags to the device */
    else if (flag_opts == DV6_ADD_FLGS)
    {
#if (INCLUDE_IP_MULTICASTING)
        /* Save a pointer to the flags */
        prev_flags = dev_ptr->dev_flags;
#endif

        /* Add the flags */
        dev_ptr->dev_flags |= flags;

#if (INCLUDE_IP_MULTICASTING)

        /* If the interface was configured to transmit Router
         * Advertisement messages, join the All-Routers Multicast
         * group and begin transmitting Router Advertisements.
         */
        if ( (!(prev_flags & DV6_ISROUTER)) && (flags & DV6_ISROUTER) )
        {
            /* If multicasting is supported on the interface. */
            if (dev_ptr->dev_flags & DV_MULTICAST)
                DEV6_Initialize_Router(dev_ptr);
        }
#endif
    }

    /* A value of -2 leaves the value unmodified */
    if ( (rtr_opts->rtr_MaxRtrAdvInterval != -2) || 
         (rtr_opts->rtr_MinRtrAdvInterval != -2) )
    {
        /* The maximum time allowed between sending unsolicited multicast 
         * Router Advertisements from the interface, in seconds.  MUST be 
         * no less than 4 seconds and no greater than 1800 seconds.
         */
        if ( (rtr_opts->rtr_MaxRtrAdvInterval >= 4) && 
             (rtr_opts->rtr_MaxRtrAdvInterval <= 1800) )
            dev_ptr->dev6_MaxRtrAdvInterval = (UINT16)rtr_opts->rtr_MaxRtrAdvInterval;
        else if (rtr_opts->rtr_MaxRtrAdvInterval != -2)
            dev_ptr->dev6_MaxRtrAdvInterval = IP6_DEFAULT_MAX_RTR_ADVERT_INTERVAL;

        /* The minimum time allowed between sending unsolicited multicast 
         * Router Advertisements from the interface, in seconds.  MUST be 
         * no less than 3 seconds and no greater than .75 * MaxRtrAdvInterval.
         */
        if ( (rtr_opts->rtr_MinRtrAdvInterval >= 3) &&
             ((UINT16)rtr_opts->rtr_MinRtrAdvInterval <= ((3 * dev_ptr->dev6_MaxRtrAdvInterval) >> 2)) )
            dev_ptr->dev6_MinRtrAdvInterval = (UINT16)rtr_opts->rtr_MinRtrAdvInterval;
        else if (rtr_opts->rtr_MinRtrAdvInterval != -2)
            dev_ptr->dev6_MinRtrAdvInterval = IP6_DEFAULT_MIN_RTR_ADVERT_INTERVAL;

        /* If the initial Router Advertisements have already been sent */
        if (dev_ptr->dev6_next_radv_time != 0)
        {
            /* Unset the current event */
            if (TQ_Timerunset(IP6_Transmit_Rtr_Adv_Event, TQ_CLEAR_ALL_EXTRA, 
                              dev_ptr->dev_index, 0) != NU_SUCCESS)
                NLOG_Error_Log("Failed to stop the timer to transmit the next Router Advertisement", 
                               NERR_SEVERE, __FILE__, __LINE__);

            else
            {
                /* Compute a random delay for transmitting the next Router
                 * Advertisement based on the new min and/or max values.
                 */
                rtr_adv_timer = 
                    ND6_Compute_Random_Timeout(dev_ptr->dev6_MinRtrAdvInterval,
                                               dev_ptr->dev6_MaxRtrAdvInterval);

                /* If the last unsolicited Router Advertisement was transmitted 
                 * less than the minimum advertisement value for the interface 
                 * seconds ago, reset the new timer value to the minimum 
                 * advertisement value for the interface.
                 */
                if ( (((rtr_adv_timer * SCK_Ticks_Per_Second) + NU_Retrieve_Clock()) -
                        dev_ptr->dev6_next_radv_time) < 
                     (dev_ptr->dev6_MinRtrAdvInterval * SCK_Ticks_Per_Second) )
                    rtr_adv_timer = dev_ptr->dev6_MinRtrAdvInterval;

                /* Set the timer to transmit the next Router Advertisement */
                if (TQ_Timerset(IP6_Transmit_Rtr_Adv_Event, dev_ptr->dev_index, 
                                rtr_adv_timer * SCK_Ticks_Per_Second, 
                                ND6RADV_UNSOL_ADV) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to set timer to transmit Router Advertisement", 
                                   NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* A value of -2 leaves the value unmodified */
    if (rtr_opts->rtr_AdvLinkMTU != -2)
    {
        /* The value to be placed in MTU options sent by the router.  A value of 
         * zero indicates that no MTU options are sent.
         */
        if (rtr_opts->rtr_AdvLinkMTU != -1)
            dev_ptr->dev6_AdvLinkMTU = (UINT32)rtr_opts->rtr_AdvLinkMTU;
        else
            dev_ptr->dev6_AdvLinkMTU = IP6_DEFAULT_ADVERT_LINK_MTU;
    }

    /* A value of -2 leaves the value unmodified */
    if (rtr_opts->rtr_AdvReachableTime != -2)
    {
        /* The value to be placed in the Reachable Time field in the Router 
         * Advertisement messages sent by the router.  The value zero means 
         * unspecified (by this router).  MUST be no greater than 3,600,000 
         * milliseconds (1 hour).
         */
        if ( (rtr_opts->rtr_AdvReachableTime != -1) && 
             (rtr_opts->rtr_AdvReachableTime <= 3600000L) )
            dev_ptr->dev6_AdvReachableTime = (UINT32)rtr_opts->rtr_AdvReachableTime;
        else
            dev_ptr->dev6_AdvReachableTime = IP6_DEFAULT_ADVERT_REACHABLE_TIME;
    }

    /* A value of -2 leaves the value unmodified */
    if (rtr_opts->rtr_AdvRetransTimer != -2)
    {
        /* The value to be placed in the Retrans Timer field in the Router 
         * Advertisement messages sent by the router.  The value zero means 
         * unspecified (by this router).
         */
        if (rtr_opts->rtr_AdvRetransTimer != -1)
            dev_ptr->dev6_AdvRetransTimer = (UINT32)rtr_opts->rtr_AdvRetransTimer;
        else
            dev_ptr->dev6_AdvRetransTimer = IP6_DEFAULT_ADVERT_RETRANS_TIMER;
    }

    /* A value of -2 leaves the value unmodified */
    if (rtr_opts->rtr_AdvCurHopLimit != -2)
    {
        /* The default value to be placed in the Cur Hop Limit field in the
         * Router Advertisement messages sent by the router.  The value zero
         * means unspecified (by this router).
         */
        if (rtr_opts->rtr_AdvCurHopLimit != -1)
            dev_ptr->dev6_AdvCurHopLimit = (UINT8)rtr_opts->rtr_AdvCurHopLimit;
        else
            dev_ptr->dev6_AdvCurHopLimit = IP6_DEFAULT_ADVERT_CUR_HOP_LIMIT;
    }

    /* A value of -2 leaves the value unmodified */
    if (rtr_opts->rtr_AdvDefaultLifetime != -2)
    {
        /* The value to be placed in the Router Lifetime field of Router 
         * Advertisements sent from the interface, in seconds.  A value of 
         * zero indicates that the router is not to be used as a default router.
         * MUST be either zero or between MaxRtrAdvInterval and 9000 seconds.
         */
        if ( (rtr_opts->rtr_AdvDefaultLifetime != -1) &&
             ((rtr_opts->rtr_AdvDefaultLifetime == 0) ||
              (((UINT16)rtr_opts->rtr_AdvDefaultLifetime >= dev_ptr->dev6_MaxRtrAdvInterval) &&
               (rtr_opts->rtr_AdvDefaultLifetime <= 9000))) )
            dev_ptr->dev6_AdvDefaultLifetime = (UINT16)rtr_opts->rtr_AdvDefaultLifetime;
        else
            dev_ptr->dev6_AdvDefaultLifetime = IP6_DEFAULT_ADVERT_DEFAULT_LIFETIME;
    }

} /* DEV6_Configure_Router */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       DEV6_Configure_Prefix_List
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function adds the list of prefixes to the Prefix List for
*       the device.  If the PRFX6_NO_ADV_AUTO flag is not set for the
*       prefix, the routine also assigns an address to the device created 
*       from the prefix and the interface identifier.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *dev_ptr                A pointer to the device entry.
*       *prefix_list            A pointer to the list of prefixes to add
*                               to the Prefix list for the device.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         The prefix length is invalid.
*       NU_NO_MEMORY            Insufficient memory.
*
*************************************************************************/
STATUS DEV6_Configure_Prefix_List(DV_DEVICE_ENTRY *dev_ptr, 
                                  DEV6_PRFX_ENTRY *prefix_list)
{
    STATUS          status = NU_SUCCESS;
    DEV6_PRFX_ENTRY *current_entry = prefix_list;

    /* While we have not reached the last entry - signified by all
     * zero prefix.
     */
    while (!(IPV6_IS_ADDR_UNSPECIFIED(current_entry->prfx_prefix)))
    {
        /* The link-local prefix cannot be advertised */
        if (!(IPV6_IS_ADDR_LINKLOCAL(current_entry->prfx_prefix)))
        {     
            /* Set up the prefix entry and create a new IPv6 address
             * from the prefix.
             */
            status = PREFIX6_Configure_DEV_Prefix_Entry(current_entry, 
                                                        dev_ptr);

            if (status != NU_SUCCESS)
            	break;
        }
        
        /* Get the next entry in the array */
        current_entry = 
            (DEV6_PRFX_ENTRY*)((UINT8 HUGE*)current_entry + sizeof(DEV6_PRFX_ENTRY));
    }

    return (status);

} /* DEV6_Configure_Prefix_List */

#endif
