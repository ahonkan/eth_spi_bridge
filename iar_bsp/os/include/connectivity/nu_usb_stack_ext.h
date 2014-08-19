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
*        nu_usb_stack_ext.h
*
* COMPONENT
*
*        Nucleus USB Software
*
* DESCRIPTION
*
*        This file contains the exported function names for the common 
*        stack component.
*
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS 
*       None 
*
* DEPENDENCIES 
*       nu_usb_stack_imp.h          Base Stack Internal definitions.
*
************************************************************************/

/* ==================================================================== */

#ifndef _NU_USB_STACK_EXT_H
#define _NU_USB_STACK_EXT_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */

/* ==============  USB Include Files =================================  */

#include        "connectivity/nu_usb_stack_imp.h"

/* ====================  Function Prototypes ========================== */

STATUS _NU_USB_STACK_Create (NU_USB_STACK * cb,
                             NU_USB_SUBSYS * subsys,
                             CHAR * name,
                             const VOID *dispatch);

/* The following functions are to be over ridden by the child stack drivers
 * extending the base stack functionality */

STATUS NU_USB_STACK_Add_Hw (NU_USB_STACK * cb,
                            NU_USB_HW * controller);

STATUS NU_USB_STACK_Remove_Hw (NU_USB_STACK * cb,
                               NU_USB_HW * controller);

/* Class driver registration and De-registration */

STATUS NU_USB_STACK_Register_Drvr (NU_USB_STACK * cb,
                                   NU_USB_DRVR * driver);

STATUS NU_USB_STACK_Deregister_Drvr (NU_USB_STACK * cb,
                                     NU_USB_DRVR * driver);

/* Endpoint STALL/UNSTALL API   */

STATUS NU_USB_STACK_Stall_Endpoint (NU_USB_STACK * cb,
                                    NU_USB_PIPE * pipe);

STATUS NU_USB_STACK_Unstall (NU_USB_STACK * cb,
                             NU_USB_PIPE * pipe);

STATUS NU_USB_STACK_Is_Endpoint_Stalled (NU_USB_STACK * cb,
                                         NU_USB_PIPE * pipe,
                                         DATA_ELEMENT * status);

/* Data Transfers   */

STATUS NU_USB_Submit_IRP (NU_USB_STACK * cb,
                          NU_USB_IRP * irp,
                          NU_USB_PIPE * pipe);

STATUS NU_USB_Cancel_IRP (NU_USB_STACK * cb,
						  NU_USB_PIPE * pipe);
STATUS NU_USB_STACK_Flush_Pipe (NU_USB_STACK * cb,
                                NU_USB_PIPE * pipe);

/* Facts interface for stacks. */

STATUS NU_USB_Stack_Information (NU_USB_STACK * cb,
                                 CHAR * name);

STATUS NU_USB_STACK_Get_Config (NU_USB_STACK * cb,
                                NU_USB_DEVICE * device,
                                UINT8 *cnfgno);

STATUS NU_USB_STACK_Set_Config (NU_USB_STACK * cb,
                                NU_USB_DEVICE * device,
                                UINT8 cnfgno);

STATUS NU_USB_STACK_Get_Interface (NU_USB_STACK * cb,
                                   NU_USB_DEVICE * device,
                                   UINT8 intf_num,
                                   UINT8 *alt_setting);

STATUS NU_USB_STACK_Set_Intf (NU_USB_STACK * cb,
                              NU_USB_DEVICE * device,
                              UINT8 interface_index,
                              UINT8 alt_setting_index);

STATUS NU_USB_STACK_Get_Endpoint_Status (NU_USB_STACK * cb,
                                         NU_USB_PIPE * pipe,
                                         UINT16 *status);

STATUS NU_USB_STACK_Get_Dev_Status (NU_USB_STACK * cb,
                                    NU_USB_DEVICE * device,
                                    UINT16 *status);

STATUS NU_USB_STACK_Set_Device_Status (NU_USB_STACK * cb,
                                       NU_USB_DEVICE * device,
                                       UINT16 status);

STATUS NU_USB_STACK_Lock (NU_USB_STACK * cb);

STATUS NU_USB_STACK_Unlock (NU_USB_STACK * cb);

BOOLEAN NU_USB_STACK_Is_Valid_Device (NU_USB_STACK * cb,
                                      NU_USB_DEVICE * device);

STATUS NU_USB_STACK_Start_Session (NU_USB_STACK * cb, NU_USB_HW * hw, 
                                   UINT8 port_id, UINT16 delay);

STATUS NU_USB_STACK_End_Session (NU_USB_STACK * cb,
                                 NU_USB_HW * hw,
                                 UINT8 port_id);

STATUS _NU_USB_STACK_Delete (VOID *cb);

STATUS NU_USB_STACK_OTG_Report_Status (NU_USB_STACK * cb,
                                       STATUS status_in);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS NU_USB_STACK_Function_Suspend ( NU_USB_STACK *cb, NU_USB_INTF *intf,
                               BOOLEAN func_suspend, BOOLEAN rmt_wakeup);

#endif

/* ==================================================================== */

#endif /* _NU_USB_STACK_EXT_H  */

/* ======================  End Of File  =============================== */

