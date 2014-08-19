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
*    FILE NAME 
*
*        nu_usb_drvr_ext.h 
*
*    COMPONENT
*
*        Nucleus USB Software
*
*    DESCRIPTION
*
*        This file contains the exported function names and data structures
*        for the common (base) class driver component.
* 
*    DATA STRUCTURES
*
*        NU_USB_DRVR          Class driver control block description
*
*    FUNCTIONS
*
*        None 
*
*    DEPENDENCIES 
*
*        nu_usb_drvr_imp.h    USB class driver control block
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_DRVR_EXT_H
#define _NU_USB_DRVR_EXT_H
/* ===================================================================  */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb_drvr_imp.h"

/* =====================  #defines ===================================  */

/* ====================  Data Types ==================================  */

typedef struct usb_driver_dispatch
{
    NU_USB_DISPATCH dispatch;

    STATUS (*Examine_Intf) (NU_USB_DRVR * cb,
                            NU_USB_INTF_DESC * intf);

    STATUS (*Examine_Device) (NU_USB_DRVR * cb,
                              NU_USB_DEVICE_DESC * dev);

    STATUS (*Get_Score) (NU_USB_DRVR * cb,
                         UINT8 *score_out);

    STATUS (*Initialize_Device) (NU_USB_DRVR * cb,
                                 NU_USB_STACK * stack,
                                 NU_USB_DEVICE * dev);

    STATUS (*Initialize_Interface) (NU_USB_DRVR * cb,
                                    NU_USB_STACK * stack,
                                    NU_USB_DEVICE * dev,
                                    NU_USB_INTF * intf);

    STATUS (*Disconnect) (NU_USB_DRVR * cb,
                          NU_USB_STACK * stack,
                          NU_USB_DEVICE * dev);
}
NU_USB_DRVR_DISPATCH;

/* ====================  Function Prototypes ========================== */

/* Abstract class - so no create API */

/* Create function fills in the given control block with the information
 * supplied. It then calculates and fills in the score of the driver */

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
                            const VOID *dispatch);

/* The following functions have default implementations that work for Most
 * of the class / device level drivers. If need be they can be overridden
 * */

/* This function checks to see if the given interface can be served by
 * 'this' class driver. If so an NU_SUCCESS is returned. */

STATUS NU_USB_DRVR_Examine_Intf (NU_USB_DRVR * cb,
                                 NU_USB_INTF_DESC * intf);

/* This function checks to see if the given device can be served by
 * 'this' class driver. If so an NU_SUCCESS is returned. */

STATUS NU_USB_DRVR_Examine_Device (NU_USB_DRVR * cb,
                                   NU_USB_DEVICE_DESC * dev);

/* This function returns the score of the driver. The score is calculated
 * at the 'create' time based on the class driver parameters in the control
 * block.  It can be overridden to return a higher score to ensure that
 * 'this' driver always gets priority ahead of others. */

STATUS NU_USB_DRVR_Get_Score (NU_USB_DRVR * cb,
                              UINT8 *score_out);

/* User services */
STATUS NU_USB_DRVR_Register_User (NU_USB_DRVR * cb,
                                  NU_USB_USER * user);

STATUS NU_USB_DRVR_Deregister_User (NU_USB_DRVR * cb,
                                    NU_USB_USER * user);

/* Stack API. */
STATUS NU_USB_DRVR_Initialize_Device (NU_USB_DRVR * cb,
                                      NU_USB_STACK * stack,
                                      NU_USB_DEVICE * dev);
STATUS NU_USB_DRVR_Initialize_Interface (NU_USB_DRVR * cb,
                                         NU_USB_STACK * stack,
                                         NU_USB_DEVICE * dev,
                                         NU_USB_INTF * intf);

STATUS NU_USB_DRVR_Disconnect (NU_USB_DRVR * cb,
                               NU_USB_STACK * stack,
                               NU_USB_DEVICE * dev);

UNSIGNED NU_USB_DRVR_Get_Users_Count (NU_USB_DRVR * cb);

UNSIGNED NU_USB_DRVR_Get_Users (NU_USB_DRVR * cb,
                                NU_USB_USER ** users,
                                UNSIGNED num_requested);

/* The following functions have default implementations that work for Most
 * of the class / device level drivers. If need be they can be overridden
 * */

/* This function checks to see if the given interface can be served by
 * 'this' class driver. If so an NU_SUCCESS is returned. */

STATUS _NU_USB_DRVR_Examine_Intf (NU_USB_DRVR * cb,
                                  NU_USB_INTF_DESC * intf);

/* This function checks to see if the given device can be served by
 * 'this' class driver. If so an NU_SUCCESS is returned. */

STATUS _NU_USB_DRVR_Examine_Device (NU_USB_DRVR * cb,
                                    NU_USB_DEVICE_DESC * dev);

/* This function returns the score of the driver. The score is calculated
 * at the 'create' time based on the class driver parameters in the control
 * block.  It can be overridden to return a higher score to ensure that
 * 'this' driver always gets priority ahead of others. */

STATUS _NU_USB_DRVR_Get_Score (NU_USB_DRVR * cb,
                               UINT8 *score_out);

STATUS _NU_USB_DRVR_Delete (VOID *cb);

/* ==================================================================== */

#endif /* _NU_USB_DRVR_EXT_H  */

/* ======================  End Of File  =============================== */

