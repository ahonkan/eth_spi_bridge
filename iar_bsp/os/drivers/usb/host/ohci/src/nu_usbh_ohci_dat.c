/**************************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*   nu_usbh_ohci_dat.c
*
* COMPONENT
*   OHCI Driver / Nucleus USB Host Software
*
* DESCRIPTION
*   This file contains the shared data definitions for the Generic OHCI
*   driver.
*
* DATA STRUCTURES
*   None.
*
* FUNCTIONS
*   None.
*
* DEPENDENCIES
*  nu_usb.h              All USB definitions.
*
**************************************************************************/

/* ==============  USB Include Files =================================== */
#include "connectivity/nu_usb.h"
#include "drivers/nu_drivers.h"

/* =====================  Global data ================================== */

const NU_USBH_OHCI_DISPATCH usbh_ohci_dispatch =
{
	{
		{
			_NU_USBH_OHCI_Delete,           /* Does not override. */
			_NU_USB_Get_Name,               /* Does not override. */
			_NU_USB_Get_Object_Id           /* Does not override. */
		}
	}
};

/* ====================  End of File =================================== */
