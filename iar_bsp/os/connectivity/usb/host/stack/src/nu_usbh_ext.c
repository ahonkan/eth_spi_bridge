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
*        nu_usbh_ext.c
*
* COMPONENT
*       Nucleus USB Host software.
*
* DESCRIPTION
*       This file contains the implementation of services offered by
*       NU_USBH singleton component.
*
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       NU_USBH_Create                      Create singleton
*       NU_USBH_Delete                      Delete singleton
*
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USBH_EXT_C
#define USBH_EXT_C

/* ==============  Standard Include Files ============================  */
#include    "connectivity/nu_usb.h"

/* ====================  Function Definitions ========================= */

/*************************************************************************
* FUNCTION
*        NU_USBH_Create
*
* DESCRIPTION
*       This creates the NU_USBH singleton. It initializes all the
*       subsystems of host singleton.
*
* INPUTS
*       singleton_cb        Pointer to singleton control block
*
* OUTPUTS
*       NU_SUCCESS          Always
*       NU_USB_INVLD_ARG    Invalid arguments
*************************************************************************/
STATUS NU_USBH_Create (NU_USBH * singleton_cb)
{
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(singleton_cb);

    nu_usbh = singleton_cb;
    memset(nu_usbh, 0, sizeof(NU_USBH));

    singleton_cb->num_irq_entries = 0;

#if (!NU_USB_OPTIMIZE_FOR_SIZE)

    /* Create all the sub systems of the Host */
    status = NU_USBH_SUBSYS_Create ((NU_USBH_SUBSYS
                            *) (&(singleton_cb->controller_subsys)));

    if (status == NU_SUCCESS)
    {
        status = NU_USB_SUBSYS_Set_Signature ((NU_USB_SUBSYS *) &
                                 (singleton_cb->controller_subsys),
                                 NU_USBH_HW_ID);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_SUBSYS_Create ((NU_USBH_SUBSYS *) & (singleton_cb->stack_subsys));
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USB_SUBSYS_Set_Signature ((NU_USB_SUBSYS *) &
                                     (singleton_cb->stack_subsys),
                                     NU_USBH_STACK_ID);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_SUBSYS_Create ((NU_USBH_SUBSYS *) &
                           (singleton_cb->class_driver_subsys));
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USB_SUBSYS_Set_Signature ((NU_USB_SUBSYS *) &
                                 (singleton_cb->class_driver_subsys),
                                 NU_USBH_DRVR_ID);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_SUBSYS_Create ((NU_USBH_SUBSYS *) & (singleton_cb->user_subsys));
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USB_SUBSYS_Set_Signature ((NU_USB_SUBSYS *) &
                                 (singleton_cb->user_subsys), NU_USBH_USER_ID);
    }

#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USBH_Delete
*
* DESCRIPTION
*       This function deletes the instance of USBH subsystem.And deletes
*       all the subsystem contained in it.
*
* INPUTS
*       None
*
* OUTPUTS
*       NU_SUCCESS      Always
*
*************************************************************************/
STATUS NU_USBH_Delete ()
{
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

#if (!NU_USB_OPTIMIZE_FOR_SIZE)

    /* Delete all the sub systems of the host */
    status = NU_USB_SUBSYS_Delete ((NU_USB_SUBSYS *) & (nu_usbh->controller_subsys));

    status |= NU_USB_SUBSYS_Delete ((NU_USB_SUBSYS *) & (nu_usbh->stack_subsys));

    status |= NU_USB_SUBSYS_Delete ((NU_USB_SUBSYS *) & (nu_usbh->class_driver_subsys));

    status |= NU_USB_SUBSYS_Delete ((NU_USB_SUBSYS *) & (nu_usbh->user_subsys));

#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

    nu_usbh = NU_NULL;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/************************************************************************/

#endif /* USBH_EXT_C */
/* ======================  End Of File  =============================== */
