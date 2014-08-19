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
*        nu_usbh_drvr_ext.c
*
* COMPONENT
*      Nucleus USB Host Stack
*
* DESCRIPTION
*    This file contains implementations of NU_USBH_DRVR services.
*
* DATA STRUCTURES
*    None
*
* FUNCTIONS
*    _NU_USBH_DRVR_Create               - Create function to be called from
*                                         specialized versions of the
*                                         host USB drivers.
*   _NU_USBH_DRVR_Delete                - Deletes an instance of h class driver
*
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USBH_DRVR_EXT_C
#define USBH_DRVR_EXT_C

/* ==============  Standard Include Files ============================  */
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*        _NU_USBH_DRVR_Create
*
* DESCRIPTION
*    This function invokes _NU_USB_DRVR_Create to initialize the NU_USB_DRVR
* part of the control block, with host's class driver sub system pointer.
*
* INPUTS
* cb                 Pointer to the user-supplied driver control block. NOTE:
*                    All subsequent requests made to NU_USB_DRVR require this
*                    pointer.
* name               Pointer to a 7-character name for the driver. The name must
*                    be null-terminated.
* match_flag         OR of one or more of the following:
*                    USB_MATCH_VNDR_ID - The driver can serve those device's
*                    whose device descriptor's idVendor matches the idVendor of
*                    this driver.
*                    USB_MATCH_PRDCT_ID - The driver can serve those device's
*                    whose device descriptor's idVendor and idProduct match that
*                    of this driver.if USB_MATCH_PRDCT_ID is in match flag,
*                    USB_MATCH_VNDR_ID must necessarily be part of the
*                    match_flag.
*                    USB_MATCH_REL_NUM - The driver can serve those device's
*                    whose device descriptor's idVendor , idProduct match that
*                    of this driver and devices bcdDevice is in the range of
*                    bcdDeviceLow and bcdDevice high of the driver.if
*                    USB_MATCH_REL_NUM is in match flag, USB_MATCH_PRDCT_ID
*                    must necessarily be part of the match_flag.
*                    USB_MATCH_CLASS - The driver can serve those device
*                    interfaces whose interface descriptor's bInterfaceClass
*                    matches that of this driver.
*                    USB_MATCH_SUB_CLASS - The driver can serve those device
*                    interfaces whose interface descriptor's bInterfaceClass and
*                    bInterfaceSubClass matches that of this driver. If
*                    USB_MATCH_SUB_CLASS is in match flag, USB_MATCH_CLASS must
*                    necessarily be part of the match flag.
*                    USB_MATCH_PROTOCOL - The driver can serve those device
*                    interfaces whose interface descriptor's bInterfaceClass,
*                    bInterfaceSubClass and bInterfaceProtocol matches that of
*                    this driver. If USB_MATCH_SUB_CLASS is in match flag,
*                    USB_MATCH_CLASS must necessarily be part of the match flag.
*
* idVendor           vendor id of the devices that this driver can serve.
*                    Irrelevant, if USB_MATCH_VNDR_ID is not set in the
*                    match_flag.
* idProduct          product id of the devices that this driver can serve.
*                    Irrelevant, if USB_MATCH_PRDCT_ID is not set in the
*                    match_flag.
* bcdDeviceLow       Release numbers of the device that this driver can serve.
*                    The lowest number of this release number range. Irrelevant
*                    if USB_MATCH_REL_NUM is not set in the match flag.
* bcdDeviceHigh      Release numbers of the device that this driver can serve.
*                    The highest number of this release number range. Irrelevant
*                    if USB_MATCH_REL_NUM is not set in the match flag.
* bInterfaceClass    interface class code of the device interfaces that this
*                    driver can serve. Irrelevant, if USB_MATCH_CLASS is not
*                    set.
* bInterfaceSubClass interface sub class code of the device interfaces that this
*                    driver can serve. Irrelevant, if USB_MATCH_SUB_CLASS is not
*                    set.
* bInterfaceProtocol interface protocol code of the device interfaces that this
*                    driver can serve. Irrelevant, if USB_MATCH_PROTOCOL is not
*                    set.
*
* OUTPUTS
*    NU_SUCCESS           Indicates successful completion of the service.
*    NU_USB_INVLD_ARG     Indicates that the match_flag is not well formed.
*    NU_USB_NOT_PRESENT   Indicates that the maximum number of control blocks
*                         that could be created in the sub system has
*                         exceeded.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS _NU_USBH_DRVR_Create (NU_USBH_DRVR * cb,
                             CHAR * name,
                             UINT32 match_flag,
                             UINT16 idVendor,
                             UINT16 idProduct,
                             UINT16 bcdDeviceLow,
                             UINT16 bcdDeviceHigh,
                             UINT8 bInterfaceClass,
                             UINT8 bInterfaceSubClass,
                             UINT8 bInterfaceProtocol,
                             const VOID *dispatch)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(dispatch);
    NU_USB_PTRCHK(nu_usbh);

    return _NU_USB_DRVR_Create (cb,
                                (NU_USB_SUBSYS *) & (nu_usbh->
                                                     class_driver_subsys), name,
                                match_flag, idVendor, idProduct, bcdDeviceLow,
                                bcdDeviceHigh, bInterfaceClass,
                                bInterfaceSubClass, bInterfaceProtocol,
                                dispatch);

}

/*************************************************************************
* FUNCTION
*        _NU_USBH_DRVR_Delete
*
* DESCRIPTION
*       This function deletes a specified host class driver.
*
* INPUTS
*       cb      pointer to host class driver control block.
*
* OUTPUTS
*       NU_SUCCESS          host class driver deleted successfully
*       NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS _NU_USBH_DRVR_Delete (VOID *cb)
{
    NU_USB_PTRCHK(cb);

    return (_NU_USB_DRVR_Delete (cb));
}

/************************************************************************/

#endif /* USBH_DRVR_EXT_C */
/* ======================  End Of File  =============================== */
