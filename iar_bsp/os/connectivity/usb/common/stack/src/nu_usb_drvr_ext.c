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
*        nu_usb_drvr_ext.c
*
* COMPONENT
*   USB Base
*
* DESCRIPTION
*    This file contains implementations of NU_USB_DRVR services.
*
* DATA STRUCTURES
*    None
*
* FUNCTIONS
*    _NU_USB_DRVR_Create               -  Create function to be called from
*                                         specialized versions of the driver.
*    _NU_USB_DRVR_Delete               -  Deletes the class driver reference.
*
*    NU_USB_DRVR_Register_User         -  Registers a user with the driver.
*    NU_USB_DRVR_Deregister_User       -  Deregisters the user of the
*                                         driver.
*    NU_USB_DRVR_Examine_Device        -  Inspects the device descriptor
*                                         for a match.
*    NU_USB_DRVR_Examine_Intf          -  Inspects the interface descriptor
*                                         for a match.
*    NU_USB_DRVR_Initialize_Device     -  New device event handler of the
*                                         driver.
*    NU_USB_DRVR_Initialize_Interface  -  New interface event handler of
*                                         the driver.
*    NU_USB_DRVR_Disconnect            -  Disconnect event handler of the
*                                         device.
*    NU_USB_DRVR_Get_Score             -  returns score of the driver.
*    NU_USB_DRVR_Get_Users             -  returns pointers to control
*                                         blocks of the registered users.
*    NU_USB_DRVR_Get_Users_Count       -  returns the count of the number
*                                         of registered users.
*   _NU_USB_DRVR_Examine_Intf          - Inspects interface descriptor for match
*                                        with class drivers.
*   _NU_USB_DRVR_Examine_Device        - Inspects device descriptor for match with
*                                        class drivers.
*   _NU_USB_DRVR_Get_Score             - Returns the score of the driver.
*
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_DRVR_EXT_C
#define USB_DRVR_EXT_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*               _NU_USB_DRVR_Create
*
* DESCRIPTION
*    This function creates a class driver with some basic behavior. This
*    function is called from Create functions of the specialized versions
*    of NU_USB_DRVR.
*
* INPUTS
* cb                 Pointer to the user-supplied driver control block. NOTE:
*                    All subsequent requests made to NU_USB_DRVR require this
*                    pointer.
* subsys             Pointer to the control block of the sub system the driver
*                    belongs to.
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
*    NU_SUCCESS         Indicates successful completion of the service.
*    NU_USB_INVLD_ARG   Indicates that the match_flag is not well formed.
*    NU_USB_NOT_PRESENT Indicates that the maximum number of control blocks
*                       that could be created in the sub system has
*                       exceeded.
*
*************************************************************************/
STATUS _NU_USB_DRVR_Create (NU_USB_DRVR * cb,
                            NU_USB_SUBSYS * subsys,
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
    STATUS status;
    UINT8 valid = 0;

    /* Points for the driver are based on the match flags the driver supports.
     * Vendor Specific Driver is awarded maximum points, and more fields
     * a driver is willing to match, more points it would accumulate.
     */
    UINT8 scores[] = { 10,                  /* USB_MATCH_VNDR_ID */
        4,                                  /* USB_MATCH_PRDCT_ID */
        3,                                  /* USB_MATCH_REL_NUM */
        3,                                  /* USB_MATCH_CLASS */
        2,                                  /* USB_MATCH_SUB_CLASS */
        1                                   /* USB_MATCH_PROTOCOL */
    };
    UINT8 mask, i;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(subsys);
    NU_USB_PTRCHK(dispatch);

    /* Install the dispatch table */
    status = _NU_USB_Create ((NU_USB *) cb, subsys, name, dispatch);
    if (NU_SUCCESS != status)
    {
        return (status);
    }

    /* Verify the validness of the arguments */
    if (match_flag == USB_MATCH_VNDR_ID)
        if (idVendor != 0)
            valid = 1;
    if (match_flag == (USB_MATCH_VNDR_ID | USB_MATCH_PRDCT_ID))
        if ((idVendor != 0) && (idProduct != 0))
            valid = 1;
    if (match_flag ==
        (USB_MATCH_VNDR_ID | USB_MATCH_PRDCT_ID | USB_MATCH_REL_NUM))
        if ((idVendor != 0) && (idProduct != 0))
            valid = 1;
    if (match_flag == USB_MATCH_CLASS)
        if (bInterfaceClass != 0)
            valid = 1;
    if (match_flag == (USB_MATCH_CLASS | USB_MATCH_SUB_CLASS))
        if ((bInterfaceClass != 0) && (bInterfaceSubClass != 0))
            valid = 1;
    if (match_flag ==
        (USB_MATCH_CLASS | USB_MATCH_SUB_CLASS | USB_MATCH_PROTOCOL))
        if ((bInterfaceClass != 0) && (bInterfaceSubClass != 0)
            && (bInterfaceProtocol != 0))
            valid = 1;
    if (!valid)
        return (NU_USB_INVLD_ARG);

    /* Set up the rest of the fields in the structure */
    cb->match_flag = match_flag;
    cb->idVendor = idVendor;
    cb->idProduct = idProduct;
    cb->bcdDeviceLow = bcdDeviceLow;
    cb->bcdDeviceHigh = bcdDeviceHigh;
    cb->bInterfaceClass = bInterfaceClass;
    cb->bInterfaceSubClass = bInterfaceSubClass;
    cb->bInterfaceProtocol = bInterfaceProtocol;
    cb->num_users = 0;

    /* Calculate the score for this driver */
    cb->score = 0;
    mask = 1;
    i = 0;
    /* Calculate the score of this driver */
    while (i < sizeof (scores) / sizeof (UINT8))
    {
        if (cb->match_flag & mask)
        cb->score += scores[i];
        i++;
        mask = mask << 1;
    }

    /* 0 is an invalid score, a malformed driver ! */
    if (cb->score == 0)
    {
        return NU_USB_INVLD_ARG;
    }

    /*  lower cs_priority  value has high priority, so adjust the calculated
     *  score to suit the list sorting criteria.
     */
    cb->score = 0xff - cb->score;

#if (!NU_USB_OPTIMIZE_FOR_SIZE)
    /* Place it in the container of the class driver sub system */
    status = NU_USB_SUBSYS_Add_Node (subsys, cb);
#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

    return (status);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_DRVR_Delete
*
* DESCRIPTION
*       This function deletes a specified class driver.
*
* INPUTS
*       cb      pointer to class driver control block.
*
* OUTPUTS
*       NU_SUCCESS          class driver deleted successfully
*
*************************************************************************/
STATUS _NU_USB_DRVR_Delete (VOID *cb)
{
    NU_USB_PTRCHK(cb);
    return (_NU_USB_Delete (cb));
}

/*************************************************************************
* FUNCTION
*               NU_USB_DRVR_Examine_Intf
*
* DESCRIPTION
*     A driver which can serve device interfaces based on
* class/sub-class/protocol would have this function defined. It
* examines the interface descriptor of the interface to see if there is
* a match. A default implementation of this function is provided by
* _NU_USB_DRVR_Examine_Intf.
* On host s/w, this function is invoked during enumeration of a device and on
* function s/w, this is invoked whenever it receives SET_INTERFACE or
* SET_CONFIGURATION command from the host.
*
* INPUTS
*    cb    Pointer to the driver control block.
*    intf  Pointer to interface descriptor that needs to be matched
*          against.
*
* OUTPUTS
*    NU_SUCCESS         Indicates that the driver found a match and so the
*                       interface is preliminarily accepted by the driver.
*    NU_USB_REJECTED    Indicates that the driver did not find a match and
*                       so the interface is rejected by the driver.
*
*************************************************************************/
STATUS NU_USB_DRVR_Examine_Intf (NU_USB_DRVR * cb,
                                 NU_USB_INTF_DESC * intf)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(intf);

    if (((NU_USB_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
        Examine_Intf)
        return (((NU_USB_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
                Examine_Intf (cb, intf));
    else
        return NU_NOT_PRESENT;
}

/*************************************************************************
* FUNCTION
*               NU_USB_DRVR_Examine_Device
*
* DESCRIPTION
*     A driver which can serve devices  based on vendor id/product id/release
* number would have this function defined. It examines the device descriptor of
* the device to see if there is a match. A default implementation of this
* function is provided by _NU_USB_DRVR_Examine_Device.
* On host s/w, this function is invoked during enumeration of a device and on
* function s/w, this is invoked whenever it receives SET_CONFIGURATION command
* from the host.
*
* INPUTS
*    cb    Pointer to the driver control block.
*    dev   Pointer to device descriptor that needs to be matched
*          against.
*
* OUTPUTS
*    NU_SUCCESS         Indicates that the driver found a match and so the
*                       device is preliminarily accepted by the driver.
*    NU_USB_REJECTED    Indicates that the driver did not find a match and
*                       so the device is rejected by the driver.
*
*************************************************************************/
STATUS NU_USB_DRVR_Examine_Device (NU_USB_DRVR * cb,
                                   NU_USB_DEVICE_DESC * dev)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(dev);

    if (((NU_USB_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
        Examine_Device)
        return (((NU_USB_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
                Examine_Device (cb, dev));
    else
        return NU_NOT_PRESENT;
}

/*************************************************************************
* FUNCTION
*               NU_USB_DRVR_Get_Score
*
* DESCRIPTION
*     This function returns the score assigned to the driver during create.
* This score is the sorting key for the drivers registered with the stack
* and a driver is asked to examine interface/device in the descending order
* of the score. The default implementation of this function is provided by
* _NU_USB_DRVR_Get_Score.
*
* INPUTS
*   cb         pointer to the driver control block.
*   score_out  Pointer to the variable to hold the value of the score.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DRVR_Get_Score (NU_USB_DRVR * cb,
                              UINT8 *score_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(score_out);

    return (((NU_USB_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Get_Score (cb, score_out));
}

/*************************************************************************
* FUNCTION
*               NU_USB_DRVR_Initialize_Device
*
* DESCRIPTION
*    This function is invoked by the stack once NU_USB_DRVR_Examine_Device
* returns NU_SUCCESS. This function of the driver is expected to find all
* the necessary interfaces and pipes and choose a configuration and then
* claim the interface. No default implementation exists for this function and
* so every specialization of NU_USB_DRVR has to implement this for itself.
*
*
* INPUTS
*    cb     Pointer to the driver control block.
*    stack  Pointer to the stack control block, that has invoked this
*           function.
*    dev    pointer to the device control block, that needs to be
*           initialized.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DRVR_Initialize_Device (NU_USB_DRVR * cb,
                                      NU_USB_STACK * stack,
                                      NU_USB_DEVICE * dev)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(stack);
    NU_USB_PTRCHK(dev);

    return (((NU_USB_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Initialize_Device (cb, stack, dev));
}

/*************************************************************************
* FUNCTION
*               NU_USB_DRVR_Initialize_Interface
*
* DESCRIPTION
*    This function is invoked by the stack once NU_USB_DRVR_Examine_Intf
* returns NU_SUCCESS. This function of the driver is expected to find all
* the necessary pipes and choose an alternate setting for the interface  and
* then claim the interface. No default implementation exists for this function
* and so every specialization of NU_USB_DRVR has to implement this for itself.
*
* INPUTS
*    cb     Pointer to the driver control block.
*    stack  Pointer to the stack control block, that has invoked this
*           function.
*    intf   pointer to the interface control block, that needs to be
*           initialized.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DRVR_Initialize_Interface (NU_USB_DRVR * cb,
                                         NU_USB_STACK * stack,
                                         NU_USB_DEVICE * dev,
                                         NU_USB_INTF * intf)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(stack);
    NU_USB_PTRCHK(dev);
    NU_USB_PTRCHK(intf);

    return (((NU_USB_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Initialize_Interface (cb, stack, dev, intf));
}

/*************************************************************************
* FUNCTION
*               NU_USB_DRVR_Disconnect
*
* DESCRIPTION
*    On host s/w, this function is invoked by the stack, whenever the device is
* disconnected. On function s/w this is invoked by the stack whenever the
* device is disconnected or when the active configuration is changed.
*
* INPUTS
*    cb     Pointer to the driver control block.
*    stack  Pointer to the stack control block, that has invoked this
*           function.
*    dev    pointer to the device control block, that needs to be
*           un-initialized.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DRVR_Disconnect (NU_USB_DRVR * cb,
                               NU_USB_STACK * stack,
                               NU_USB_DEVICE * dev)
{
    STATUS status;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(stack);
    NU_USB_PTRCHK(dev);

    status = NU_SUCCESS;

    if (((NU_USB_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Disconnect != NU_NULL)
    {
            status = ((NU_USB_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Disconnect (cb, stack, dev);
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DRVR_Get_Users_Count
*
* DESCRIPTION
*   This function returns the number of registered users of this class
*   driver.
*
* INPUTS
*   cb   Pointer to the driver control block.
*
* OUTPUTS
*   number of users.
*
*************************************************************************/
UNSIGNED NU_USB_DRVR_Get_Users_Count (NU_USB_DRVR * cb)
{
    if(cb == NU_NULL)
    {
        return (0x00);
    }
    return cb->num_users;
}

/*************************************************************************
* FUNCTION
*               NU_USB_DRVR_Get_Users
*
* DESCRIPTION
*    This function returns pointers to control blocks of the registered
* users.
*
* INPUTS
*    cb            Pointer to the driver control block.
*    users         pointer to user supplied array of memory locations to
*                  hold pointers to the user control blocks.
*    num_requested the size of the users array.
*
* OUTPUTS
*   number of user control blocks copied in to users array.
*
*************************************************************************/
UNSIGNED NU_USB_DRVR_Get_Users (NU_USB_DRVR * cb,
                                NU_USB_USER ** users,
                                UNSIGNED num_requested)
{
    UNSIGNED num_users, i;

    if((cb == NU_NULL)||(users == NU_NULL))
    {
        return (0x00);
    }

    if (cb->num_users > num_requested)
        num_users = num_requested;
    else
        num_users = cb->num_users;
    for (i = 0; i < num_users; i++)
        users[i] = cb->users[i];
    return num_users;
}

/*************************************************************************
* FUNCTION
*               _NU_USB_DRVR_Examine_Intf
*
* DESCRIPTION
*    Inspects the interface descriptor for a match with the drivers
* class/sub class/protocol.
*
* INPUTS
*    cb   Pointer to driver control block.
*    intf Pointer to the interface descriptor.
*
* OUTPUTS
*    NU_SUCCESS         Indicates that the driver found a match and so the
*                       interface is preliminarily accepted by the driver.
*    NU_USB_REJECTED    Indicates that the driver did not find a match and
*                       so the interface is rejected by the driver.
*
*************************************************************************/
STATUS _NU_USB_DRVR_Examine_Intf (NU_USB_DRVR * cb,
                                  NU_USB_INTF_DESC * intf)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(intf);

    switch (cb->match_flag)
    {
        case USB_MATCH_CLASS:
            if (cb->bInterfaceClass == intf->bInterfaceClass)
                return (NU_SUCCESS);
            break;
        case USB_MATCH_CLASS | USB_MATCH_SUB_CLASS:
            if ((cb->bInterfaceClass == intf->bInterfaceClass) &&
                (cb->bInterfaceSubClass == intf->bInterfaceSubClass))
                return (NU_SUCCESS);
            break;
        case USB_MATCH_CLASS | USB_MATCH_SUB_CLASS | USB_MATCH_PROTOCOL:
            if ((cb->bInterfaceClass == intf->bInterfaceClass) &&
                (cb->bInterfaceSubClass == intf->bInterfaceSubClass) &&
                (cb->bInterfaceProtocol == intf->bInterfaceProtocol))
                return (NU_SUCCESS);
            break;
            /* other combinations of match flag are invalid here */

    }
    /* No match, reject the interface */
    return (NU_USB_REJECTED);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_DRVR_Examine_Device
*
* DESCRIPTION
*    Inspects the device descriptor for a match with the driver based on
* vendor id/product id/release number.
*
* INPUTS
*    cb   Pointer to driver control block.
*    dev  Pointer to the device descriptor.
*
* OUTPUTS
*    NU_SUCCESS         Indicates that the driver found a match and so the
*                       device is preliminarily accepted by the driver.
*    NU_USB_REJECTED    Indicates that the driver did not find a match and
*                       so the device is rejected by the driver.
*
*************************************************************************/
STATUS _NU_USB_DRVR_Examine_Device (NU_USB_DRVR * cb,
                                    NU_USB_DEVICE_DESC * dev)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(dev);

    switch (cb->match_flag)
    {
        case USB_MATCH_VNDR_ID:
            if (cb->idVendor == dev->idVendor)
                return (NU_SUCCESS);
            break;
        case USB_MATCH_VNDR_ID | USB_MATCH_PRDCT_ID:
            if ((cb->idVendor == dev->idVendor) &&
                (cb->idProduct == dev->idProduct))
                return (NU_SUCCESS);
            break;
        case USB_MATCH_VNDR_ID | USB_MATCH_PRDCT_ID | USB_MATCH_REL_NUM:
            if ((cb->idVendor == dev->idVendor) &&
                (cb->idProduct == dev->idProduct) &&
                ((dev->bcdDevice >= cb->bcdDeviceLow)
                 && (dev->bcdDevice < cb->bcdDeviceHigh)))
                return (NU_SUCCESS);
            /* other combinations of match flag are invalid here */
    }
    /* No match, reject the interface */
    return (NU_USB_REJECTED);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_DRVR_Get_Score
*
* DESCRIPTION
*     This function returns the score assigned to the driver during create.
*
* INPUTS
*   cb         pointer to the driver control block.
*   score_out  Pointer to the variable to hold the value of the score.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS _NU_USB_DRVR_Get_Score (NU_USB_DRVR * cb,
                               UINT8 *score_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(score_out);

    *score_out = cb->score;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*     NU_USB_DRVR_Register_User
*
* DESCRIPTION
*     This function registers a new User of the class driver, to the class
*     driver.
*
* INPUTS
*     cb      Pointer to the driver control block.
*     user    pointer to the user control block.
*
* OUTPUTS
*     NU_SUCCESS           Indicates successful completion of the service.
*     NU_USB_MAX_EXCEEDED  Indicates that the maximum no. of users
*                          registered with this class driver has exceeded
*                          NU_USB_MAX_USERS.
*
*************************************************************************/
STATUS NU_USB_DRVR_Register_User (NU_USB_DRVR * cb,
                                  NU_USB_USER * user)
{
    UINT16 i;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(user);

    i = cb->num_users;
    if (i == NU_USB_MAX_USERS)
    {
        return (NU_USB_MAX_EXCEEDED);
    }

    cb->users[i] = user;

    cb->num_users++;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*     NU_USB_DRVR_Deregister_User
*
* DESCRIPTION
*     This function deregisters a User from the class driver.
*
* INPUTS
*     cb      Pointer to the driver control block.
*     user    pointer to the user control block.
*
* OUTPUTS
*     NU_SUCCESS           Indicates successful completion of the service.
*     NU_USB_INVLD_ARG     Indicates the given user is not found.
*
*************************************************************************/
STATUS NU_USB_DRVR_Deregister_User (NU_USB_DRVR * cb,
                                    NU_USB_USER * user)
{
    UINT16 i;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(user);

    for (i = 0; i < cb->num_users; i++)
        if (cb->users[i] == user)
            break;

    if (i == cb->num_users)
    {
        return NU_USB_INVLD_ARG;
    }

    --cb->num_users;

    /* move the last user to the empty slot */
    if (cb->num_users != i)
    {
        cb->users[i] = cb->users[cb->num_users];
    }

    cb->users[cb->num_users] = NU_NULL;

    return (NU_SUCCESS);
}

/*************************************************************************/

#endif /* USB_DRVR_EXT_C */
/************************* end of file *********************************/

