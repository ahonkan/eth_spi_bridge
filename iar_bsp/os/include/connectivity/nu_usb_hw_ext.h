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
*        nu_usb_hw_ext.h 
*
*    COMPONENT
*
*        Nucleus USB Device Software
*
*    DESCRIPTION
*
*        This file contains the exported function names and data structures
*        for the common controller driver component.
* 
*    DATA STRUCTURES
*
*        None 
*
*    FUNCTIONS
*
*        None 
*
*    DEPENDENCIES 
*
*        nu_usb_hw_imp.h     HW Internal definitions
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_HW_EXT_H
#define _NU_USB_HW_EXT_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */
/* HSET Test mode Test Packet */
#define NU_USB_HW_TEST_PACKET_SIZE 53

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb_hw_imp.h"

extern UINT8 NU_USB_HW_Test_Packet[NU_USB_HW_TEST_PACKET_SIZE];

typedef VOID (*NU_USB_HW_ROLESWITCH_CALLBACK)(NU_USB_HW *hw, UINT8 port_id, UINT8 role);

typedef struct usb_hw_dispatch
{
    NU_USB_DISPATCH dispatch;
}
NU_USB_HW_DISPATCH;

/* ====================  Function Prototypes ========================== */
/* Create  */
STATUS _NU_USB_HW_Create (NU_USB_HW * cb,
                          NU_USB_SUBSYS * subsys,
                          CHAR * name,
                          UINT8 speed,
                          VOID *base_address,
                          UINT8 num_irq,
                          INT * irq,
                          const VOID *dispatch);

/* Callback API */
STATUS NU_USB_HW_Initialize (NU_USB_HW * cb,
                             NU_USB_STACK * stack_cb);

STATUS NU_USB_HW_Uninitialize (NU_USB_HW * cb);

STATUS NU_USB_HW_Get_Speed (NU_USB_HW * cb,
                            UINT8 *speed);

STATUS NU_USB_HW_ISR (NU_USB_HW * cb);

STATUS NU_USB_HW_Get_Stack (NU_USB_HW * cb,
                            NU_USB_STACK ** stack_cb);

STATUS NU_USB_HW_Submit_IRP (NU_USB_HW * cb,
                             NU_USB_IRP * irp,
                             UINT8 function_addr,
                             UINT8 bEndpointAddress);

STATUS NU_USB_HW_Flush_Pipe (NU_USB_HW * cb,
                             UINT8 function_addr,
                             UINT8 bEndpointAddress);

STATUS NU_USB_HW_Enable_Interrupts (NU_USB_HW * cb);

STATUS NU_USB_HW_Disable_Interrupts (NU_USB_HW * cb);

STATUS NU_USB_HW_Open_Pipe (NU_USB_HW * cb,
                            UINT8 function_addr,
                            UINT8 bEndpointAddress,
                            UINT8 bmEndpointAttributes,
                            UINT8 speed,
                            UINT16 wMaxPacketSize,
                            UINT32 interval,
                            UINT32 load);

STATUS NU_USB_HW_Close_Pipe (NU_USB_HW * cb,
                             UINT8 function_addr,
                             UINT8 bEndpointAddress);

STATUS NU_USB_HW_Modify_Pipe (NU_USB_HW * cb,
                              UINT8 function_addr,
                              UINT8 bEndpointAddress,
                              UINT8 bmEndpointAttributes,
                              UINT16 wMaxPacketSize,
                              UINT32 interval,
                              UINT32 load);

STATUS _NU_USB_HW_Initialize (NU_USB_HW * cb,
                              NU_USB_STACK * stack_cb);

STATUS _NU_USB_HW_Delete (VOID *cb);

STATUS NU_USB_HW_Get_Role (NU_USB_HW * cb,
                           UINT8 port_id,
                           UINT8 *role_out);

STATUS NU_USB_HW_Start_Session (NU_USB_HW * cb, UINT8 port_id,
                                UINT16 delay);

STATUS NU_USB_HW_End_Session (NU_USB_HW * cb,
                              UINT8 port_id);

STATUS NU_USB_HW_Notify_Role_Switch (NU_USB_HW *hw, 
        NU_USB_HW_ROLESWITCH_CALLBACK role_switch_cb);

STATUS NU_USB_HW_Test_Mode (NU_USB_HW *hw,
                            UINT8 mode);

                            
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS NU_USB_HW_Open_SS_Pipe (NU_USB_HW * cb,
                            	UINT8 function_addr,
                            	UINT8 bEndpointAddress,
                            	UINT8 bmEndpointAttributes,
                            	UINT8 speed,
                            	UINT16 wMaxPacketSize,
                            	UINT32 interval,
	                         	UINT32 load,
                            	UINT8 bMaxBurst,
                            	UINT8 bmSSEndpCompAttrib,
								UINT16 bytes_per_interval);
									
STATUS NU_USB_HW_Modify_SS_Pipe    (NU_USB_HW * cb,
                                    UINT8 function_addr,
                                    UINT8 bEndpointAddress,
                                    UINT8 bmEndpointAttributes,
                                    UINT16 wMaxPacketSize,
                                    UINT32 interval,
                                    UINT32 load,
                                    UINT8 bMaxBurst,
                                    UINT8 bmSSEndpCompAttrib);

STATUS NU_USB_HW_Update_BELT_Value (NU_USB_HW   *cb,
                                    UINT16      belt_value );

STATUS NU_USB_HW_Update_Power_Mode (NU_USB_HW   *cb,
                                    UINT8       power_mode );
#endif

/* ==================================================================== */

#endif /* _NU_USB_HW_EXT_H    */

/* =======================  End Of File  ============================== */

