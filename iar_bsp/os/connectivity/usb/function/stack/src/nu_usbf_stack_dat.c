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
*       nu_usbf_stack_dat.c
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the shared data structure definitions for the
*       device stack component.
*
* DATA STRUCTURES
*
*       usbf_stack_dispatch                 Dispatch Table.
*       usbf_ctrl_request_worker            Routing table for processing
*                                           the standard requests.
*       usbf_num_supported_ctrl_requests    Number of control requests
*                                           being supported.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ==============  USB Include Files ==================================  */

#include "connectivity/nu_usb.h"

/* =====================  Global data =================================  */

/* This is the function stack's dispatch table. Where ever possible, the
 * entries are made up of the parent's function pointers. Unsupported
 * functions are initialized to NU_NULL. Examples of such APIs include
 * NU_USB_STACK_Set_Configuration which are not applicable on the function
 * stack.
 */

const NU_USB_STACK_DISPATCH usbf_stack_dispatch = {

    {
     _NU_USBF_STACK_Delete,
     _NU_USB_Get_Name,
     _NU_USB_Get_Object_Id},

    _NU_USBF_STACK_Add_Hw,
    _NU_USBF_STACK_Remove_Hw,
    _NU_USB_STACK_Register_Drvr,
    _NU_USBF_STACK_Deregister_Drvr,
    _NU_USBF_STACK_Stall_Endp,
    _NU_USBF_STACK_Unstall_Endp,
    _NU_USBF_STACK_Is_Endp_Stalled,
    _NU_USBF_STACK_Submit_IRP,
	_NU_USBF_STACK_Flush_Pipe,
    /* SetConfiguration, Not supported. */
    NU_NULL,
    /* SetInterface, Not supported. */
    NU_NULL,
    _NU_USB_STACK_Get_Config,
    _NU_USB_STACK_Get_Intf,
    _NU_USBF_STACK_Get_Endp_Status,
    _NU_USBF_STACK_Get_Device_Status,
    /* SetDeviceStatus not supported. */
    NU_NULL,
    _NU_USB_STACK_Get_Intf_Status,
    _NU_USB_STACK_Lock,
    _NU_USB_STACK_Unlock,
    _NU_USB_STACK_Is_Valid_Device,
    _NU_USBF_STACK_Start_Session,
    _NU_USBF_STACK_End_Session,

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
	_NU_USBF_STACK_Cancel_IRP,
    _NU_USBF_STACK_Function_Suspend
#else
	_NU_USBF_STACK_Cancel_IRP
#endif
};

/* The following table assists us as the function routing table for
 * processing the standard requests originating from the host.
 * This contains the [USB] Chapter 9 specified standard request
 * processing functions. In the event of any function not being
 * supported, the corresponding entry must be set to
 * 'usbf_request_error'
 */

USBF_CTRL_REQ_WRKR          usbf_ctrl_request_worker[] = {

    usbf_get_status,                        /* GET_STATUS request.       */
    usbf_feature,                           /* S(G)ET_FEATURE request.   */
    usbf_request_error,
    usbf_feature,                           /* S(G)ET_FEATURE request.   */
    usbf_request_error,
    usbf_set_address,                       /* SET_ADDRESS request.      */
    usbf_get_descriptor,                    /* GET_DESCRIPTOR request.   */
    usbf_set_descriptor,                    /* SET_DESCRIPTOR request.   */
    usbf_get_configuration,                 /* GET_CONFIGURATION request.*/
    usbf_set_configuration,                 /* SET_CONFIGURATION request.*/
    usbf_get_interface,                     /* GET_INTERFACE request.    */
    usbf_set_interface,                     /* SET_INTERFACE request.    */

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    usbf_sync_frame,                        /* SYNC_FRAME request.       */
    usbf_set_system_exit_latency,            /* Set SEL Request. */
    usbf_set_isochornous_delay              /* Set Isochoronous Delay
                                               request. */
#else
    usbf_sync_frame                         /* SYNC_FRAME request.       */
#endif
};

/* Number of control requests being supported. */
UINT8                       usbf_num_supported_ctrl_requests =
    (sizeof (usbf_ctrl_request_worker) / sizeof (USBF_CTRL_REQ_WRKR));

/* =====================  #defines ====================================  */

/* ====================  Data Types ===================================  */

/* ====================  Function Definitions ========================== */

/* ======================  End Of File  ================================ */
