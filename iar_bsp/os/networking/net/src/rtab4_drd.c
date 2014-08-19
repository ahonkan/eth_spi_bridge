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
*   FILE NAME
*
*       rtab4_drd.c
*
*   COMPONENT
*
*       IPv4 Routing
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
*       RTAB4_Delete_Routes_For_Device
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/* The top or root of the route table tree. */
extern ROUTE_NODE   *RTAB4_Root_Node;

extern RTAB_ROUTE_PARMS RTAB4_Parms;

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Delete_Routes_For_Device
*
*   DESCRIPTION
*
*       This function removes all routes from the routing table for a
*       specific device.
*
*   INPUTS
*
*       *name                   A pointer to the name of the device for
*                               which you want to delete all routes.
*
*   OUTPUTS
*
*      NU_SUCCESS               All routes were successfully deleted
*      NU_INVALID_ENTRY         The device is invalid
*
*************************************************************************/
STATUS RTAB4_Delete_Routes_For_Device(const CHAR *name)
{
    ROUTE_NODE      *route;
    DV_DEVICE_ENTRY *device;
    STATUS          status = NU_SUCCESS;

    /* Get a pointer to the device */
    device = DEV_Get_Dev_By_Name(name);

    /* If the device exists in the system */
    if (device != NU_NULL)
    {
        /* Delete all routes from the system */
        do
        {
            memset(&route, 0, sizeof(route));

            /* Find a route in the system using the device */
            route = RTAB_Find_Route_For_Device(device, RTAB4_Root_Node);

            /* If a route was found, delete it */
            if (route)
            {
                status = RTAB_Delete_Node(route, &RTAB4_Parms);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to delete a found route for device",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                    break;
                }
            }
        }
        while (route != NU_NULL);
    }
    else
        status = NU_INVALID_ENTRY;

    return (status);

} /* RTAB4_Delete_Routes_For_Device */

#endif
