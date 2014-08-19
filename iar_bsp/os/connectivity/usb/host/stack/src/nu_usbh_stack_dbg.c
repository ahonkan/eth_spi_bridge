/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

*************************************************************************
*
* FILE NAME 
*
*        nu_usbh_stack_dbg.c 
*
* COMPONENT
*        Nucleus USB Host Stack.
* 
* DESCRIPTION 
*       This file contains routines which provide facts about Nucleus USB
*       Host Stack.
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       NU_USBH_Get_Root_Hubs_Count     -returns the number of root hubs
*       NU_USBH_Get_Root_Hubs           -returns a list of root hubs.
*       NU_USBH_Get_Device_Count        -returns the number of device on a
*                                       root hub.
*       NU_USBH_Get_Devices             -returns a list of devices on a
*                                       root hub.
* 
* DEPENDENCIES 
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USBH_STACK_DBG_C
#define USBH_STACK_DBG_C

/* ==============  Standard Include Files ============================  */
#include    "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION 
*        NU_USBH_Get_Root_Hubs_Count
*
* DESCRIPTION
*       This function returns the number of root hubs served by given
*       Nucleus USB Host stack.
* 
* INPUTS 
*       stack       Pointer to Nucleus USB Host stack control block
*
* OUTPUTS
*       UNSIGNED    number of root hubs.
*
*************************************************************************/
UNSIGNED NU_USBH_Get_Root_Hubs_Count (NU_USBH_STACK * stack)
{
    UNSIGNED i, count = 0;
    if(stack == NU_NULL)
    {
        return count;
    }

    for (i = 0; i < NU_USBH_MAX_HW; i++)
    {
        if (stack->bus_resources[i].controller)
        {
            count++;
        }
    }

    return (count);
}

/*************************************************************************
* FUNCTION 
*        NU_USBH_Get_Root_Hubs
*
* DESCRIPTION
*
*       This function builds a list of NU_USB_DEVICE pointers for all root
*       hubs, starting at the specified location. The number of root hub 
*       pointers placed in the list is equivalent to the total number of 
*       root hubs or the maximum number of pointers specified in the call.
* 
* INPUTS 
*       stack               pointer to host stack control block.
*       root_hub_list       pointer to the list area.
*       max_root_hubs       maximum number of pointers requested.
*
* OUTPUTS
*       UNSIGNED            Number of pointers returned in list area.
*
*************************************************************************/
UNSIGNED NU_USBH_Get_Root_Hubs (NU_USBH_STACK * stack,
                                NU_USB_DEVICE ** root_hub_list,
                                UNSIGNED max_root_hubs)
{
    UINT8 i, cnt = 0;

    if(stack == NU_NULL
       || root_hub_list == NU_NULL)
    {
        return cnt;
    }

    for (i = 0; i < NU_USBH_MAX_HW; i++)
    {
        if (stack->bus_resources[i].controller)
        {
            root_hub_list[cnt++] = stack->bus_resources[i].root_hub;
            if (cnt == max_root_hubs)
                break;
        }
    }

    return (cnt);
}

/*************************************************************************
* FUNCTION 
*        NU_USBH_Get_Device_Count
*
* DESCRIPTION
*       This function returns the number of devices attached and being
*       served by a given root hub.
* 
* INPUTS 
*       stack           pointer to stack control block
*       root_hub        pointer to device control block for root hub.
*
* OUTPUTS
*       UNSIGNED        device count of root hub.
*
*************************************************************************/
UNSIGNED NU_USBH_Get_Device_Count (NU_USBH_STACK * stack,
                                   NU_USB_DEVICE * root_hub)
{
    UNSIGNED            count = 0;
    NU_USB_DEVICE       *dev;
    USBH_BUS_RESOURCES  *bus;
    STATUS status = NU_SUCCESS;
    STATUS internal_sts = NU_SUCCESS;

    if(stack == NU_NULL
       || root_hub == NU_NULL)
    {
        return count;
    }

    status = _NU_USBH_STACK_Lock ((NU_USB_STACK *) stack);

    if (status != NU_SUCCESS)
    {
		return (status);
    }
	
    bus = usbh_find_bus (stack, root_hub);

    if (bus == NU_NULL)
    {
        internal_sts = _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);
        NU_UNUSED_PARAM(internal_sts);
        return 0;
    }

    dev = bus->dev_list;

    while (dev)
    {
        count++;
        dev = (NU_USB_DEVICE *) dev->node.cs_next;

        /* Device is till not fully formed, so stop */
        if ((dev->function_address != USB_ROOT_HUB) && (dev->parent == NU_NULL))
            dev = NU_NULL;

        if (dev == bus->dev_list)
            dev = NU_NULL;
    }

    internal_sts = _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);
    NU_UNUSED_PARAM(internal_sts);

    return (count);
}

/*************************************************************************
* FUNCTION 
*        NU_USBH_Get_Devices
*
* DESCRIPTION
*
*       This function builds a list of NU_USB_DEVICE pointers for all
*       devices connected to a given root hub, starting at the specified 
*       location. The number of device pointers placed in the list is 
*       equivalent to the total number of devices connected to the root hub
*       or the maximum number of pointers specified in the call.
* 
* INPUTS 
*       stack               pointer to host stack control block.
*       root_hub            pointer to device control block for root hub.
*       device_list         pointer to the list area.
*       max_devices         maximum number of pointers requested.
*
* OUTPUTS
*       UNSIGNED            Number of pointers returned in list area.
*
*************************************************************************/
UNSIGNED NU_USBH_Get_Devices (NU_USBH_STACK * stack,
                              NU_USB_DEVICE * root_hub,
                              NU_USB_DEVICE ** device_list,
                              UNSIGNED max_devices)
{
    NU_USB_DEVICE       *dev;
    UNSIGNED            count = 0;
    USBH_BUS_RESOURCES  *bus;
    STATUS status = NU_SUCCESS;
    STATUS internal_sts = NU_SUCCESS;

    if(stack == NU_NULL
       || root_hub == NU_NULL
       || device_list == NU_NULL)
    {
        return count;
    }

    status = _NU_USBH_STACK_Lock ((NU_USB_STACK *) stack);

    if (status != NU_SUCCESS)
    {
		return (status);
    }
    bus = usbh_find_bus (stack, root_hub);

    if (bus == NU_NULL)
    {
        internal_sts = _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);
        NU_UNUSED_PARAM(internal_sts);
        return 0;
    }

    dev = bus->dev_list;
    while (dev)
    {
        device_list[count++] = dev;
        if (count == max_devices)
            break;

        dev = (NU_USB_DEVICE *) dev->node.cs_next;

        /* Device is till not fully formed, so stop */
        if ((dev->function_address != USB_ROOT_HUB) && (dev->parent == NU_NULL))
            dev = NU_NULL;

        if (dev == bus->dev_list)
            dev = NU_NULL;
    }

    internal_sts = _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);
    NU_UNUSED_PARAM(internal_sts);

    return (count);
}

/************************************************************************/

#endif /* USBH_STACK_DBG_C */
/* ======================  End Of File  =============================== */
