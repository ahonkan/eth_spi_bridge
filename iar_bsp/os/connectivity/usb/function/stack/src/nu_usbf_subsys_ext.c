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
*       nu_usbf_subsys_ext.c
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the implementation of the exported functions of
*       the USB Device Stack, Subsystem.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBF_SUBSYS_Create               Initializes a subsystem.
*       NU_USBF_SUBSYS_Delete               Disables a subsystem.
*
* DEPENDENCIES
*
*       nu_usb.h
*
**************************************************************************/
#ifndef USBF_SUBSYS_EXT_C
#define USBF_SUBSYS_EXT_C
/* ==============  USB Include Files ==================================  */

#include    "connectivity/nu_usb.h"

/* ====================  Function Definitions ========================== */
#if (!NU_USB_OPTIMIZE_FOR_SIZE)
/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_SUBSYS_Create
*
* DESCRIPTION
*
*       Initializes a USBF Subsystem.
*
* INPUTS
*
*       cb                                  Subsystem to be initialized.
*
* OUTPUTS
*
*       NU_SUCCESS                          Subsystem created successfully.
*       NU_NOT_PRESENT                      Indicates a configuration
*                                           problem because of which no
*                                           more USB Objects could be
*                                           created.
*
**************************************************************************/
STATUS  NU_USBF_SUBSYS_Create (NU_USBF_SUBSYS *cb)
{
    /* Error checks. */
    if (cb  == NU_NULL)
        return NU_USB_INVLD_ARG;
    /* Base class behavior. */
    return (_NU_USB_SUBSYS_Create
            ((NU_USB_SUBSYS *) cb, &usbf_subsys_dispatch));
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_SUBSYS_Delete
*
* DESCRIPTION
*
*       Disables a USBF Subsystem.
*
* INPUTS
*
*       cb                                  Subsystem to be uninitialized.
*
* OUTPUTS
*
*       NU_SUCCESS                          Subsystem created successfully.
*       NU_NOT_PRESENT                      Indicates that the supplied
*                                           subsystem could not be found.
*
**************************************************************************/
STATUS  NU_USBF_SUBSYS_Delete (NU_USBF_SUBSYS *cb)
{
    /* Error checks. */
    if (cb  == NU_NULL)
        return NU_USB_INVLD_ARG;
    return (NU_USB_SUBSYS_Delete ((NU_USB_SUBSYS *) cb));
}

#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

/*************************************************************************/
#endif /* USBF_SUBSYS_EXT_C */
/* ======================  End Of File  ================================ */

