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

***************************************************************************
*
* FILE NAME
*
*       nu_usbf_ext.c
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the exported function implementations for the
*       Nucleus USB  device stack, singleton object.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBF_Create                      Singleton Object Creation.
*       NU_USBF_Delete                      Singleton Object Deletion.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
#ifndef _USBF_EXT_C
#define _USBF_EXT_C
/* ===============  USB Include Files =================================  */

#include    "connectivity/nu_usb.h"

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_Create
*
* DESCRIPTION
*
*       Singleton Object initialization for USB Device Stack
*       It creates and initializes all the contained subsystems and
*       initializes the Lisr <-> Vector map.
*
* INPUTS
*
*       singleton_cb                        Singleton object(control block)
*                                           to be initialized.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the
*                                           Initialization has been
*                                           successful.
*
**************************************************************************/
STATUS  NU_USBF_Create (NU_USBF *singleton_cb)
{
    STATUS  status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checking   */
    NU_USB_PTRCHK_RETURN(singleton_cb);

    nu_usbf = singleton_cb;
    memset(nu_usbf, 0, sizeof(NU_USBF));

#if (!NU_USB_OPTIMIZE_FOR_SIZE)
    /* Create all the sub systems of the Device. */

    /* Controller subsystem. */
    status = NU_USBF_SUBSYS_Create (
                (NU_USBF_SUBSYS *) (&(singleton_cb->controller_subsys)));
    if (status == NU_SUCCESS)
    {
        status = NU_USB_SUBSYS_Set_Signature ((NU_USB_SUBSYS *) &
                             (singleton_cb->controller_subsys),
                             NU_USBF_HW_ID);
    }

    /* Stack subsystem. */
    if (status == NU_SUCCESS)
    {
        status = NU_USBF_SUBSYS_Create (
                    (NU_USBF_SUBSYS *) &(singleton_cb->stack_subsys));
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USB_SUBSYS_Set_Signature ((NU_USB_SUBSYS *) &
                                 (singleton_cb->stack_subsys),
                                 NU_USBF_STACK_ID);
    }

    /* Class driver subsystem. */
    if (status == NU_SUCCESS)
    {
        status = NU_USBF_SUBSYS_Create ((NU_USBF_SUBSYS *) &
                           (singleton_cb->class_driver_subsys));
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USB_SUBSYS_Set_Signature ((NU_USB_SUBSYS *) &
                                     (singleton_cb->class_driver_subsys),
                                     NU_USBF_DRVR_ID);
    }
    /* User subsystem. */
    if (status == NU_SUCCESS)
    {
        status = NU_USBF_SUBSYS_Create (
                (NU_USBF_SUBSYS *) &(singleton_cb->user_subsys));
    }
    if (status == NU_SUCCESS)
    {
        status = NU_USB_SUBSYS_Set_Signature (
                (NU_USB_SUBSYS *) &(singleton_cb->user_subsys), NU_USBF_USER_ID);
    }

#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

    /* Reset the IRQ-FC map. No entries in there now. */
    singleton_cb->num_irq_entries = 0;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_Delete
*
* DESCRIPTION
*
*       This function un-initializes the singleton object.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the singleton
*                                           has been uninitialized
*                                           successfully.
*
**************************************************************************/
STATUS  NU_USBF_Delete (VOID)
{
    STATUS  status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

#if (!NU_USB_OPTIMIZE_FOR_SIZE)
    /* Delete all the sub systems of the function software. */

    /* Controller SubSystem. */
    status = NU_USBF_SUBSYS_Delete (
                (NU_USB_SUBSYS *) &(nu_usbf->controller_subsys));

    /* Stack SubSystem. */
    status |= NU_USBF_SUBSYS_Delete (
                (NU_USB_SUBSYS *) &(nu_usbf->stack_subsys));

    /* Class driver SubSystem. */
    status |= NU_USBF_SUBSYS_Delete (
                (NU_USB_SUBSYS *) &(nu_usbf->class_driver_subsys));

    /* User SubSystem. */
    status |= NU_USBF_SUBSYS_Delete ((NU_USB_SUBSYS *) &(nu_usbf->user_subsys));

#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

    /* Set the global subsystem pointer to NU_NULL. */
    nu_usbf = NU_NULL;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************/
#endif /* _USBF_EXT_C */
/* ======================  End Of File  ================================ */
