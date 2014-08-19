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
*       nu_usbh_kbd_imp.c
*
*
* COMPONENT
*
*       Nucleus USB Host Keyboard driver.
*
* DESCRIPTION
*
*       This file provides the implementation of keyboard driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       USBH_KBD_Find_Device      Translates session pointer to a keyboard
*                                 pointer.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/************************************************************************/
#include    "connectivity/nu_usb.h"
/************************************************************************/
/************************************************************************
* FUNCTION
*
*       USBH_KBD_Find_Device
*
* DESCRIPTION
*
*       This function finds the keyboard device instance corresponding to
*       the handle provided by the HID driver.
*
* INPUTS
*
*       cb          pointer to Keyboard driver block.
*       handle      handle to search.
*
* OUTPUTS
*
*       non NU_NULL  Pointer to the corresponding NU_USBH_KBD_DEVICE.
*       NU_NULL      Indicates Device doesn't exist.
*
*
*************************************************************************/
NU_USBH_KBD_DEVICE *USBH_KBD_Find_Device(NU_USBH_KBD *cb,
                                         VOID        *handle)
{
    NU_USBH_KBD_DEVICE *next;
    NU_USBH_KBD_DEVICE *keyboard = cb->keyboard_list_head;

    /* Search for handle in the circular list of keyboard instances. */
    while (keyboard)
    {
        next = (NU_USBH_KBD_DEVICE *) keyboard->node.cs_next;
        if (keyboard->handle == handle)
            return (keyboard);

        if ((next == cb->keyboard_list_head) ||
            (cb->keyboard_list_head == NU_NULL))
            return (NU_NULL);
        else
            keyboard = next;
    }
    return (NU_NULL);
}
/************************* end of file *********************************/

