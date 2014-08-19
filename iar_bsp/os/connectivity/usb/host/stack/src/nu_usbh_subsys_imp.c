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
*        nu_usbh_subsys_imp.c
*
* COMPONENT
*       Nucleus USB Host Software
*
* DESCRIPTION
*       This file contains the implementation of Internal functions of
*       Nucleus USB Host Subsystem
*
*
* DATA STRUCTURES
*       None.
*
* FUNCTIONS
*        NU_USBH_SUBSYS_Create       -creates a host subsystem.
*       _NU_USBH_SUBSYS_Delete      -deletes a subsystem
*       _NU_USBH_SUBSYS_Lock        -grabs a lock
*       _NU_USBH_SUBSYS_Unlock      -releases a lock
*
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USBH_SUBSYS_IMP_C
#define USBH_SUBSYS_IMP_C

/* ==============  Standard Include Files ============================  */
#include    "connectivity/nu_usb.h"

#if (!NU_USB_OPTIMIZE_FOR_SIZE)

/* ====================  Function Definitions ========================= */

/*************************************************************************
* FUNCTION
*        _NU_USBH_SUBSYS_Create
*
* DESCRIPTION
*       This function initializes the subsystem for Host Software. This is
*       a protected interface and extenders of subsystem can provide their
*       own dispatch table using this function.
*
* INPUTS
*       cb          pointer to host subsystem control block
*       dispatch    host subsystem dispatch table.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/
STATUS NU_USBH_SUBSYS_Create (NU_USBH_SUBSYS * cb)
{
    STATUS status;

    if(cb == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    status =
        NU_Create_Semaphore (&(((NU_USBH_SUBSYS *) cb)->lock), "USBH-LK", 1,
                             NU_PRIORITY);
    if (status != NU_SUCCESS)
        return (status);

    /* Base class behavior */
    return (_NU_USB_SUBSYS_Create
            ((NU_USB_SUBSYS *) cb, &usbh_subsys_dispatch));
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_SUBSYS_Delete
*
* DESCRIPTION
*       This function deletes a reference to Nucleus USB Host Subsystem.
*
* INPUTS
*       cb      Pointer to subsystem control block.
*
* OUTPUTS
*       NU_SUCCESS      Always.
*
*************************************************************************/
STATUS _NU_USBH_SUBSYS_Delete (NU_USB_SUBSYS * cb)
{
    STATUS internal_sts = NU_SUCCESS;
    if(cb == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    internal_sts = NU_Delete_Semaphore (&((NU_USBH_SUBSYS *) cb)->lock);

    /* Base Class behavior */
    internal_sts |= _NU_USB_SUBSYS_Delete (cb);

    NU_UNUSED_PARAM(internal_sts);
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_SUBSYS_Lock
*
* DESCRIPTION
*       This function is called via dispatch table. This provides the
*       implementation of virtual function of base class. This function is
*       used for protecting critical data structures. Host subsystem
*       implements it using a Semaphore.
*
* INPUTS
*       cb      Pointer to subsystem control block.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*       NU_INVALID_SUSPEND      Suspension from non-task
*
*************************************************************************/
STATUS _NU_USBH_SUBSYS_Lock (NU_USB_SUBSYS * cb)
{
    if(cb == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    return NU_Obtain_Semaphore (&(((NU_USBH_SUBSYS *) cb)->lock), NU_SUSPEND);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_SUBSYS_Unlock
*
* DESCRIPTION
*       This function is called via dispatch table. This provides the
*       implementation of virtual function of base class. This function is
*       used for protecting critical data structures. Host subsystem
*       implements it using a Semaphore.
*
* INPUTS
*       cb      Pointer to subsystem control block.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*
*************************************************************************/
STATUS _NU_USBH_SUBSYS_Unlock (NU_USB_SUBSYS * cb)
{
    if(cb == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }
    return NU_Release_Semaphore (&(((NU_USBH_SUBSYS *) cb)->lock));
}

#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

/************************************************************************/

#endif /* USBH_SUBSYS_IMP_C */
/* ======================  End Of File  =============================== */
