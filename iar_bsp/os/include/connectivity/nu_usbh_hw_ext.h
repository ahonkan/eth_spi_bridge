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
************************************************************************/

/************************************************************************ 
 * 
 * FILE NAME
 *
 *      nu_usbh_hw_ext.h
 * 
 * COMPONENT 
 *      Nucleus USB  Host Software 
 * 
 * DESCRIPTION 
 *
 *      This file contains the exported function names and data structures
 *      for the HW Component.
 * 
 * DATA STRUCTURES 
 *      NU_USBH_HW_DISPATCH   NU_USBH_HW Dispatch table description.
 * 
 * FUNCTIONS 
 *      None
 *
 * DEPENDENCIES 
 *      nu_usb_hw_imp.h       HW Internal Definitions.
 *      nu_usbh_ext.h         USB Host Definitions. 
 * 
 *************************************************************************/
/* ==================================================================== */
#ifndef _NU_USBH_HW_EXT_H_
#define _NU_USBH_HW_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================== Include Files ================================ */
#include "connectivity/nu_usbh_hw_imp.h"
#include "connectivity/nu_usbh_ext.h"

/* ====================  Data definitions  ========================== */

/* USBH_Controller control block. */
typedef struct usbh_hw_dispatch
{
    NU_USB_HW_DISPATCH usb_dispatch;
}
NU_USBH_HW_DISPATCH;

/* ====================  Function Prototypes ========================== */

/* NU_USBH_HW API. */
/* Abstract Class - So No create API */

/* NU_USBH_HW extended API. */
STATUS _NU_USBH_HW_Create (NU_USBH_HW * cb,
                           CHAR * name,
                           NU_MEMORY_POOL * pool,
                           UINT8 number_companion_controllers,
                           UINT8 speed,
                           VOID *base_address,
                           INT vector_number,
                           const VOID *dispatch);

STATUS _NU_USBH_HW_Delete (VOID *cb);

STATUS _NU_USBH_HW_Initialize (NU_USB_HW * cb,
                               NU_USB_STACK * stack);

STATUS _NU_USBH_HW_Uninitialize (NU_USB_HW * cb);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS NU_USBH_HW_Update_Device(NU_USBH_HW *cb,
                                NU_USB_DEVICE *usb_device,
                                UINT8    packet_size,
                                UINT32   sel,
                                BOOLEAN  is_hub,
                                UINT8    tt_time,
                                UINT8    num_ports);

STATUS NU_USBH_HW_Init_Device( NU_USBH_HW  *cb );

STATUS NU_USBH_HW_Dinit_Device( NU_USBH_HW  *cb,
                                UINT8 function_address );

STATUS NU_USBH_HW_Unstall_Pipe( NU_USBH_HW  *cb,
                                UINT8 bEndpointAddress,
                                UINT8 function_address );

STATUS NU_USBH_HW_Disable_Pipe(NU_USBH_HW  *cb,
                               UINT8       function_address,
                               UINT8       endpoint_address);

STATUS NU_USBH_HW_Reset_Bandwidth(NU_USBH_HW  *cb,
                                  UINT8        function_address);

STATUS NU_USBH_HW_Allocate_Bandwidth( NU_USBH_STACK    *cb,
                                   NU_USB_DEVICE    *usb_device,
                                   NU_USB_CFG       *cnfg,
                                   NU_USB_ALT_SETTG *curr_alt_set,
                                   NU_USB_ALT_SETTG *new_alt_set );

STATUS NU_USBH_HW_Update_Exit_Latency(NU_USBH_HW    *hw,
                                   NU_USBH_STACK *cb,
                                   NU_USB_DEVICE *dev,
                                   UINT8 *latency );

#define NU_USBH_HW_Update_Max_Packet_Size(a,b,c) NU_USBH_HW_Update_Device(a,b,c,0,0,0,0)
#define NU_USBH_HW_Update_Hub_Device(a,b,c,d)    NU_USBH_HW_Update_Device(a,b,0,0,NU_TRUE,c,d)
#define NU_USBH_HW_Update_SEL(a,b,c)             NU_USBH_HW_Update_Device(a,b,0,c,0,0,0)

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

/* ==================================================================== */

#endif /* _NU_USBH_HW_EXT_H_ */

/* ======================  End Of File  =============================== */
