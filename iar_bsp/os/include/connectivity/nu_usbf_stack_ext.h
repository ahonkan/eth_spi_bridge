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
*       nu_usbf_stack_ext.h
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the exported function declarations for the stack
*       component of the Nucleus USB Device Software.
*
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_stack_imp.h
*
**************************************************************************/

#ifndef _NU_USBF_STACK_EXT_H
#define _NU_USBF_STACK_EXT_H

/* ==============  USB Include Files ==================================  */

#include    "connectivity/nu_usbf_stack_imp.h"

/* ====================  Function Prototypes =========================== */

/* Stack creation and deletion APIs.    */
STATUS NU_USBF_STACK_Create (NU_USBF_STACK *cb,
                             CHAR          *name);

STATUS _NU_USBF_STACK_Delete (VOID *cb);

/* Extended Stack creation.  */
STATUS _NU_USBF_STACK_Create (NU_USBF_STACK *cb,
                              CHAR          *name,
                              const VOID    *dispatch);

STATUS NU_USBF_STACK_Attach_Device (NU_USBF_STACK *cb,
                                    NU_USB_DEVICE *dev);

STATUS  NU_USBF_STACK_Detach_Device (NU_USBF_STACK *cb,
                                     NU_USB_DEVICE *usb_device);

STATUS _NU_USBF_STACK_Start_Session (NU_USB_STACK *cb,
                                     NU_USB_HW    *hw,
                                     UINT8 port_id, UINT16 delay);

STATUS _NU_USBF_STACK_End_Session (NU_USB_STACK *cb,
                                   NU_USB_HW    *hw,
                                   UINT8         port_id);

/* USBF Callback API.    */
STATUS NU_USBF_STACK_New_Setup (NU_USB_STACK     *cb,
                                NU_USBF_HW       *fc,
                                NU_USB_SETUP_PKT *setup);

STATUS NU_USBF_STACK_New_Transfer (NU_USB_STACK *cb,
                                   NU_USBF_HW   *fc,
                                   UINT8         bEndpointAddress);

STATUS NU_USBF_STACK_Notify (NU_USB_STACK *cb,
                             NU_USBF_HW   *fc,
                             UINT32        event);

STATUS  NU_USBF_STACK_Speed_Change (NU_USB_STACK    *cb,
                                    NU_USBF_HW      *fc,
                                    UINT32          speed
#if CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE
                                    ,
                                    UINT8           maxp0
#endif
                                    );

STATUS _NU_USBF_STACK_Add_Hw (NU_USB_STACK *cb,
                              NU_USB_HW    *fc);

STATUS _NU_USBF_STACK_Remove_Hw (NU_USB_STACK *cb,
                                 NU_USB_HW    *fc);

/* Class driver registration and de-registration. */
STATUS NU_USBF_STACK_Register_Drvr (NU_USBF_STACK *cb,
                                     NU_USB_DRVR  *driver);

STATUS _NU_USBF_STACK_Deregister_Drvr (NU_USB_STACK *cb,
                                       NU_USB_DRVR  *driver);

/* Endpoint STALL/UN-STALL API.   */
STATUS _NU_USBF_STACK_Stall_Endp (NU_USB_STACK *cb,
                                  NU_USB_PIPE  *pipe);

STATUS _NU_USBF_STACK_Unstall_Endp (NU_USB_STACK *cb,
                                    NU_USB_PIPE  *pipe);

STATUS _NU_USBF_STACK_Get_Endp_Status (NU_USB_STACK *cb,
                                       NU_USB_PIPE  *pipe,
                                       UINT16       *endpoint_status_out);

STATUS _NU_USBF_STACK_Get_Device_Status (NU_USB_STACK  *cb,
                                         NU_USB_DEVICE *device,
                                         UINT16        *status_out);

STATUS _NU_USBF_STACK_Is_Endp_Stalled (NU_USB_STACK *cb,
                                       NU_USB_PIPE  *pipe,
                                       DATA_ELEMENT *status);

/* Data Transfers.   */
STATUS _NU_USBF_STACK_Submit_IRP (NU_USB_STACK *cb,
                                  NU_USB_IRP   *irp,
                                  NU_USB_PIPE  *pipe);

STATUS _NU_USBF_STACK_Cancel_IRP (NU_USB_STACK *cb,
                                  NU_USB_PIPE  *pipe);
STATUS _NU_USBF_STACK_Flush_Pipe (NU_USB_STACK *cb,
                                  NU_USB_PIPE  *pipe);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS _NU_USBF_STACK_Function_Suspend ( NU_USB_STACK *cb, NU_USB_INTF *intf,
                                    BOOLEAN func_suspend, BOOLEAN rmt_wakeup);

STATUS NU_USBF_STACK_Start_Link_Transition (NU_USBF_STACK *cb,
                                            NU_USB_DEVICE *device,
                                            UINT16        idle_period);

#endif
/* ===================================================================== */

#endif                                      /* _NU_USBF_STACK_EXT_H      */

/* ====================  End Of File =================================== */

