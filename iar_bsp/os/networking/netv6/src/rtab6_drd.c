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
*       rtab6_drd.c                                  
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This module contains the functions for deleting all routes for
*       a specified device.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*                                                                       
*       RTAB6_Delete_Routes_For_Device
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"

/* The top or root of the route table tree. */
extern ROUTE_NODE   *RTAB6_Root_Node; 

extern RTAB_ROUTE_PARMS RTAB6_Parms;

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       RTAB6_Delete_Routes_For_Device
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function deletes all routes in the system using the given
*       device.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *device                 A pointer to the device.
*
*   OUTPUTS                                                               
*                         
*       None
*
*************************************************************************/
VOID RTAB6_Delete_Routes_For_Device(const DV_DEVICE_ENTRY *device)
{
    ROUTE_NODE  		*route;
	RTAB6_ROUTE_ENTRY	*rt_entry;

    /* Delete all routes for the specified device */
    do
    {
        memset(&route, 0, sizeof(route));

        /* Find a route for the device */
        route = RTAB_Find_Route_For_Device(device, RTAB6_Root_Node);

        /* If a route was found, delete it */
        if (route)
        {
			/* Get a pointer to the first route entry. */
			rt_entry = route->rt_list_head;

			while (rt_entry)
			{
				/* If this is the route entry associated with the device. */
            	if (strcmp(device->dev_net_if_name,
                           rt_entry->rt_entry_parms.rt_parm_device->dev_net_if_name) == 0)
				{
					/* Delete this route entry. */
					if (RTAB_Delete_Route_Entry(route, (ROUTE_ENTRY*)rt_entry, &RTAB6_Parms) != NU_SUCCESS)
		            {
		                NLOG_Error_Log("Failed to delete route entry",
		                               NERR_RECOVERABLE, __FILE__, __LINE__);
		                break;
		            }

					break;
				}

				/* Get the next route entry. */
				rt_entry = rt_entry->rt_entry_next;
			}
        }
    }
    while (route != NU_NULL);   
    
} /* RTAB6_Delete_Routes_For_Device */
