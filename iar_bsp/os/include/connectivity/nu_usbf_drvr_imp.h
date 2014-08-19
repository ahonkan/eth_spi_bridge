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
*       nu_usbf_drvr_imp.h 
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the internal declarations for the device
*       (base) class driver.
*
*
* DATA STRUCTURES
*
*       NU_USBF_DRVR                        Stack base class driver control
*                                           block.
*       NU_USBF_DRVR_DISPATCH               Stack base class driver
*                                           dispatch table.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_ext.h           USB Function prototypes
*
**************************************************************************/

#ifndef     _NU_USBF_DRVR_IMP_H
#define     _NU_USBF_DRVR_IMP_H

/* ==============  USB Include Files ==================================  */

#include "connectivity/nu_usbf_ext.h"

/* ====================  Data Types ===================================  */

typedef struct nu_usbf_drvr_dispatch
{
    NU_USB_DRVR_DISPATCH dispatch;

    STATUS (*Set_Intf) (NU_USB_DRVR * cb,
                        NU_USB_STACK * stack,
                        NU_USB_DEVICE * device,
                        NU_USB_INTF * intf,
                        NU_USB_ALT_SETTG * alt_settting);

    STATUS (*New_Setup) (NU_USB_DRVR * cb,
                         NU_USB_STACK * stack,
                         NU_USB_DEVICE * device,
                         NU_USB_SETUP_PKT * setup);

    STATUS (*New_Transfer) (NU_USB_DRVR * cb,
                            NU_USB_STACK * stack,
                            NU_USB_DEVICE * device,
                            NU_USB_PIPE * pipe);

    STATUS (*Notify) (NU_USB_DRVR * cb,
                      NU_USB_STACK * stack,
                      NU_USB_DEVICE * device,
                      UINT32 event);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                      
    STATUS (*Get_Status) (  NU_USB_DRVR *cb,
                            UINT16       *status_out);

#endif
}
NU_USBF_DRVR_DISPATCH;

/* ====================  Function Prototypes =========================== */

/* ===================================================================== */

#endif      /* _NU_USBF_DRVR_IMP_H.       */

/* ======================  End Of File  ================================ */

