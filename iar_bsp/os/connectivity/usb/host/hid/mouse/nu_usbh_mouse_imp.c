/**************************************************************************
*
*               Copyright 2004 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbh_mouse_imp.c
*
*
* COMPONENT
*
*       Nucleus USB Host Mouse driver.
*
* DESCRIPTION
*
*       This file provides the implementation of mouse driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       USBH_Mouse_Find_Device              Translates session pointer to
*                                           a mouse pointer.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/************************************************************************/
#include    "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*
*       USBH_Mouse_Find_Device
*
* DESCRIPTION
*
*       This function finds the mouse device instance corresponding to
*       the handle provided by the HID driver.
*
* INPUTS
*
*       cb              Pointer to Mouse driver block.
*       handle          Handle to search.
*
* OUTPUTS
*
*       non NU_NULL     Pointer to the corresponding NU_USBH_MOUSE_DEVICE.
*       NU_NULL         Indicates Device doesn't exist.
*
*
*************************************************************************/
NU_USBH_MOUSE_DEVICE* USBH_Mouse_Find_Device(NU_USBH_MOUSE  *cb,
                                             VOID           *handle)
{
    NU_USBH_MOUSE_DEVICE   *next;
    NU_USBH_MOUSE_DEVICE   *mouse = cb->mouse_list_head;

    /* Search for handle in the circular list of mouse instances. */
    while (mouse)
    {
        next = (NU_USBH_MOUSE_DEVICE *) mouse->node.cs_next;
        if (mouse->handle == handle)
                return (mouse);

        if ((next == cb->mouse_list_head) ||
            (cb->mouse_list_head == NU_NULL))
            return (NU_NULL);
        else
            mouse = next;
    }
    return (NU_NULL);
}
/************************* end of file *********************************/

