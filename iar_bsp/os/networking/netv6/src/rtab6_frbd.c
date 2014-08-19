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
*       rtab6_frbd.c                                 
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This module contains the function RTAB6_Find_Route_By_Device.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*                                                                       
*       RTAB6_Find_Route_By_Device
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Find_Route_By_Device
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function finds the route associated with the target, a 
*       network route associated with the target or the Default Route.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *ip_addr                The destination IP address of the route
*                               to find.
*       int_index               The index of the interface used by the
*                               route to find.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       A pointer to the ROUTE_ENTRY upon success.
*       NU_NULL if the route does not exist.
*                                                                       
*************************************************************************/
ROUTE_ENTRY *RTAB6_Find_Route_By_Device(UINT8 *ip_addr, 
                                        DV_DEVICE_ENTRY *dev_ptr)
{
    RTAB6_ROUTE_ENTRY   *rt_entry;

    /* Find a route to the destination */
    rt_entry = RTAB6_Next_Hop_Determination(ip_addr);

    /* If a route was found */
    if (rt_entry)
    {
        /* If the index of the interface of this route does not match the 
         * target interface, find the matching route.
         */
        if (rt_entry->rt_entry_parms.rt_parm_device->dev_index != 
            dev_ptr->dev_index)
        {
            /* Decrement the reference count.  This is not the route that
             * will be returned.
             */
            rt_entry->rt_entry_parms.rt_parm_refcnt --;

            /* Start at the beginning of the route list */
            rt_entry = rt_entry->rt_route_node->rt_list_head;

            while (rt_entry)
            {
                /* If the index of the interface of this route matches the 
                 * target, this is the target route.
                 */
                if (rt_entry->rt_entry_parms.rt_parm_device->dev_index == 
                    dev_ptr->dev_index)
                {
                    /* Increment the reference count.  This is the route
                     * that will be returned.
                     */
                    rt_entry->rt_entry_parms.rt_parm_refcnt ++;

                    break;
                }

                else
                    rt_entry = rt_entry->rt_entry_next;

                /* If there is no route via the specified device, add a new route
                 * entry by the specified device assumed to be on-link.
                 */
                if (!rt_entry)
                {
                    if (RTAB6_Add_Route(dev_ptr, ip_addr, NU_NULL, 
                                        128, 0) != NU_SUCCESS)
                        NLOG_Error_Log("Could not add route", NERR_SEVERE, 
                                       __FILE__, __LINE__);

                    /* Restart at the beginning of the route list */
                    rt_entry = RTAB6_Find_Route(ip_addr, 0);
                }
            }
        }
    }

    return ((ROUTE_ENTRY*)rt_entry);

} /* RTAB6_Find_Route_By_Device */
