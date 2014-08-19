/**************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

************************************************************************
*
* FILE NAME
*
*       nu_usbf_user_comm_imp.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Function Communication Driver
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for ETH User Driver.
*
* DATA STRUCTURES
*
*       NU_USBF_USER_COMM                   COMM Base User Driver Control
*                                           Block.
*       NU_USBF_USER_COMM_DISPATCH          COMM Base User Driver Dispatch
*                                           Table.
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nu_usbf_user_ext.h                  Base function user definitions.
*       nu_usbf_comm_ext.h                  Base COMM function user
*                                           definitions.
*
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_USER_COMM_IMP_H_
#define _NU_USBF_USER_COMM_IMP_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */
#include "connectivity/nu_usbf_user_ext.h"
#include "connectivity/nu_usbf_comm_ext.h"

/* ============= Constants ============= */

#define     COMMF_MNG_INIT              1
#define     COMMF_DATA_INIT             2
#define     COMMF_MNG_DATA_INIT         3

/* ============= Control Block ============= */

/* Redefine if your control block is any different from this. */
typedef struct _nu_usbf_user_comm
{
    NU_USBF_USER parent;
    /* Other New members of control block (if any) goes here */

    NU_SEMAPHORE comm_lock;                 /* User access semaphore.   */

    /* Device initialization event.*/
    NU_EVENT_GROUP  device_init_event;

    /* Pointer for management driver.*/
    NU_USBF_DRVR* mng_drvr;

    NU_USBF_DRVR* data_drvr;                /* Pointer for data driver. */

    /* Handle for active communication device. */
    VOID *handle;

    /* Data interface presence flag*/
    BOOLEAN require_data_intf;

    /* To properly align this structure on 32-bit boundary */
    UINT8   pad[3];
}
NU_USBF_USER_COMM;

/* ============= Dispatch Table ============= */
typedef struct _nu_usbf_user_comm_dispatch
{
    NU_USBF_USER_DISPATCH   dispatch;

    /* This function is called by class driver, when data interface
     * is initialized.
     */
    STATUS (*DATA_Connect) (NU_USB_USER * user,
                            NU_USB_DRVR * class_driver,
                            VOID *handle);

    /* This function is called by class driver, when data interface
     * is un-initialized.
     */
    STATUS (*DATA_Disconnect) (NU_USB_USER * user,
                               NU_USB_DRVR * class_driver,
                               VOID *handle);

    /* This function block the higher level thread for initialization of
     * communication device associated with this device.
     */
    STATUS (*Wait) (NU_USBF_USER_COMM * user,
                    UNSIGNED suspend,
                    VOID **handle_out);

}
NU_USBF_USER_COMM_DISPATCH;

/* ==================================================================== */

#endif      /* _NU_USBF_USER_COMM_IMP_H_*/

/* ======================  End Of File  =============================== */

