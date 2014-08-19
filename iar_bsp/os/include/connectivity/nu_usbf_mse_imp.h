/**************************************************************************
*
*           Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbf_mse_imp.h
*
* COMPONENT
*
*       Nucleus USB Function Software :     HID Mouse User Driver
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for HID User Driver.
*
* DATA STRUCTURES
*
*       NU_USBF_MSE_DISPATCH                HID User Driver Dispatch Table.
*       NU_USBF_MSE                         HID User Driver Control Block.
*
* FUNCTIONS

*       None.
*
* DEPENDENCIES
*
*       nu_usbf_user_ext.h                  Function user external
*                                           definitions.
*       nu_usbf_mse_dat.h                   Dispatch Table Definitions.
*
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_MSE_IMP_H_
#define _NU_USBF_MSE_IMP_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */
#include "connectivity/nu_usbf_user_ext.h"

#define MSE_SUBCLASS                1       /* Boot Device */
#define MSE_PROTOCOL                2       /* Mouse */
#define MSE_REPORT_LEN              3       /* Report Length For Mouse */

#define MUS_REPORT_DESCRIPTOR_SIZE  50

typedef struct _nu_usbf_mse_report
{
    UINT8   Button;
    UINT8   MouseX;
    UINT8   MouseY;

    /* To properly align this structure on 32-bit boundary */
    UINT8   pad[1];
}
NU_USBF_MSE_REPORT;

typedef struct _nu_usbf_mse
{
    NU_USBF_USER    parent;                 /*Parent's control block.*/
    NU_TIMER    Idle_Timer;                 /*Timer which determines time
                                            interval for report transfers
                                            on interrupt in pipe.*/

    NU_USBF_MSE_REPORT  User_Report;        /* User Report*/
    NU_EVENT_GROUP  device_connect_event;   /* Event Group which will be
                                            used for connection event
                                            notification*/
    NU_SEMAPHORE    connection_lock;        /* Semaphore used for device
                                            connection synchronization */
    NU_USBF_DRVR    *drvr;                  /*Pointer to driver for this
                                            user*/
    void    *handle;
    UINT8   Protocol;                       /*User protocol*/

    /* Device configuration allocated USB function. */
    USBF_FUNCTION       *mse_function;

    /* Pointer to mouse report descriptor. */
    UINT8               *mse_report_descriptor;

    UINT8   pad[3];                         /* To properly align this
                                             structure on 32-bit
                                             boundary */
}
NU_USBF_MSE;

/* ============= Dispatch Table ======================================= */
typedef struct _nu_usbf_mse_dispatch
{
    NU_USBF_USER_DISPATCH   dispatch;
}
NU_USBF_MSE_DISPATCH;

/* Timer expiration routine */
void NU_USBF_MSE_Timer_expiration_routine(UNSIGNED ID);
STATUS NU_USBF_MSE_Process_Current_Request(NU_USBF_MSE *hiduser,
                                           NU_USBF_HID_CMD *cmd,
                                           UINT8 **data_out,
                                           UINT32 *data_len_out);

#include "connectivity/nu_usbf_mse_dat.h"
/* ==================================================================== */
#endif /* _NU_USBF_MSE_IMP_H_ */
/* ======================  End Of File  =============================== */
