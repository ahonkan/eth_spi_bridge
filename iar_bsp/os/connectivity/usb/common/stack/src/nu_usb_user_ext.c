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
*        nu_usb_user_ext.c
*
* COMPONENT
*
*       Nucleus USB Software
*
* DESCRIPTION
*
*       This file provides the implementation of external interfaces of
*   Base Class User (User Layer).
*
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       _NU_USB_USER_Create         Protected constructor API to be used
*                                   by extenders.
*       NU_USB_USER_Get_Subclass    Returns the subclass this user is
*                                   registered this.
*       NU_USB_USER_Get_Protocol    Returns the protocol this user is
*                                   registered this.
*       NU_USB_USER_Connect         Base connect callback behavior.
*       NU_USB_USER_Disconnect      Base disconnect callback behavior.
*       _NU_USB_USER_Delete         Deletes a User Driver.
* DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_USER_EXT_C
#define USB_USER_EXT_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/***********************************************************************/

/*************************************************************************
* FUNCTION
*               _NU_USB_USER_Create
*
* DESCRIPTION
*
*       Protected Call used by extenders of user to create the base user
*       control block.
*
* INPUTS
*
*       cb                  User Control Block.
*       subsys              subsystem this instance is associated with.
*       name                Name of Instance.
*       bInterfaceSubclass  Subclass this user serves.
*       bInterfaceProtocol  Protocol this user serves.
*       dispatch            Dispatch table.
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*
*************************************************************************/

STATUS _NU_USB_USER_Create (NU_USB_USER * cb,
                            NU_USB_SUBSYS * subsys,
                            CHAR * name,
                            UINT8 bInterfaceSubclass,
                            UINT8 bInterfaceProtocol,
                            const VOID *dispatch)
{
    STATUS status;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(subsys);
    NU_USB_PTRCHK(name);
    NU_USB_PTRCHK(dispatch);

    /* Install the dispatch table */
    status = _NU_USB_Create ((NU_USB *) cb, subsys, name, dispatch);
    if (NU_SUCCESS != status)
    {
        return (status);
    }

    /* Setup data. */
    cb->bInterfaceSubclass = bInterfaceSubclass;
    cb->bInterfaceProtocol = bInterfaceProtocol;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_USER_Get_Subclass
*
* DESCRIPTION
*
*    This function returns the subclass this user is created to serve.
*
* INPUTS
*
*       cb              Concerned User's control block.
*       subclass_out    Placeholder for returned subclass.
*
* OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/

STATUS NU_USB_USER_Get_Subclass (NU_USB_USER * cb,
                                 UINT8 *subclass_out)
{
    NU_USB_PTRCHK(cb);

    /* Return the subclass from control block */
    *subclass_out = cb->bInterfaceSubclass;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_USER_Get_Protocol
*
* DESCRIPTION
*
*    This function returns the protocol this user is created to serve.
*
* INPUTS
*
*       cb              Concerned User's control block.
*       protocol_out    Placeholder for returned protocol.
*
* OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS NU_USB_USER_Get_Protocol (NU_USB_USER * cb,
                                 UINT8 *protocol_out)
{
    NU_USB_PTRCHK(cb);

    /* Return the subclass from control block */
    *protocol_out = cb->bInterfaceProtocol;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_USER_Connect
*
* DESCRIPTION
*
*       Connect callback called by class driver when a device suitable
*       for this user is detected by it.
*
* INPUTS
*
*       cb              User's Control Block
*       class_driver    DRVR control block calling the callback.
*       handle          context information.
*
* OUTPUTS
*
*************************************************************************/

STATUS NU_USB_USER_Connect (NU_USB_USER * cb,
                            NU_USB_DRVR * class_driver,
                            VOID *handle)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(class_driver);
    NU_USB_PTRCHK(((NU_USB *) cb)->usb_dispatch);

    /* Call the function from dispatch table */
    return (((NU_USB_USER_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Connect (cb, class_driver, handle));
}

/*************************************************************************
* FUNCTION
*               NU_USB_USER_Disconnect
*
* DESCRIPTION
*
*       Disconnect callback called by class driver when a device being
*       served by this user is disconnected across bus.
*
* INPUTS
*
*       cb              User's Control Block
*       class_driver    DRVR control block calling the callback.
*       handle          context information.
*
* OUTPUTS
*
*************************************************************************/

STATUS NU_USB_USER_Disconnect (NU_USB_USER * cb,
                               NU_USB_DRVR * class_driver,
                               VOID *handle)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(class_driver);
    NU_USB_PTRCHK(handle);
    NU_USB_PTRCHK(((NU_USB *) cb)->usb_dispatch);

    /* Call the function from dispatch table */
    return (((NU_USB_USER_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Disconnect (cb, class_driver, handle));
}

/*************************************************************************
*
* FUNCTION
*               _NU_USB_USER_Delete
*
* DESCRIPTION
*       This function deletes a specified user driver.
*
* INPUTS
*       cb      pointer to user driver control block.
*
* OUTPUTS
*       NU_SUCCESS          user driver deleted successfully
*
*************************************************************************/

STATUS _NU_USB_USER_Delete (VOID *cb)
{

    NU_USB_PTRCHK(cb);
    return (_NU_USB_Delete (cb));

}

/*************************************************************************/

#endif /* USB_USER_EXT_C */
/*************************** end of file ********************************/

