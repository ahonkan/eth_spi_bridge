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
 *     nu_usbh_stack_ext.h 
 * 
 * COMPONENT 
 *     Nucleus USB Host stack
 * 
 * DESCRIPTION 
 *
 *     This file contains the exported function names and data structures
 *     for USB Host stack.
 * 
 * DATA STRUCTURES 
 *     None
 * 
 * FUNCTIONS 
 *     None
 *
 * DEPENDENCIES 
 *     nu_usbh_stack_imp.h       Host stack Internal Definitions.
 * 
 *************************************************************************/
/* ==================================================================== */
#ifndef _NU_USBH_STACK_EXT_H_
#define _NU_USBH_STACK_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================== Include Files ================================ */
#include "connectivity/nu_usbh_stack_imp.h"

/* ====================  Function Prototypes ========================== */

/* NU_USBH_STACK Stack API. */
STATUS NU_USBH_STACK_Create (NU_USBH_STACK * cb,
                             CHAR * name,
                             NU_MEMORY_POOL * pool,
                             VOID *stack_task_stack_address,
                             UNSIGNED stack_task_stack_size,
                             OPTION stack_task_priority,
                             VOID *hub_task_stack_address,
                             UNSIGNED hub_task_stack_size,
                             OPTION hub_task_priority,
                             VOID *hisr_stack_address,
                             UNSIGNED hisr_stack_size,
                             OPTION hisr_priority);
							 
STATUS NU_USBH_STACK_Suspend_Device (NU_USBH_STACK *stk, 
                                  NU_USB_DEVICE *dev);

/* Deprecated */
STATUS NU_USBH_STACK_Suspend_Bus (NU_USBH_STACK *cb, 
                                  NU_USBH_HW *hw, 
                                  UINT8 port_id);

STATUS NU_USBH_STACK_Resume_Device (NU_USBH_STACK *cb, 
                                    NU_USB_DEVICE *dev);

/* NU_USBH_STACK_Resume_Bus was renamed as NU_USBH_STACK_Resume_Device */
#define NU_USBH_STACK_Resume_Bus NU_USBH_STACK_Resume_Device

NU_USB_DEVICE * NU_USBH_STACK_Get_Devices (NU_USBH_HW *hw);

STATUS NU_USBH_STACK_Switch_Config(NU_USBH_STACK *stack,
                    		   	  NU_USB_DEVICE *device,
                     		   	  UINT8 config_num);
									
STATUS NU_USBH_STACK_Get_Config_Info(NU_USB_DEVICE *device,
                     		   	     UINT8 *num_config,
								     UINT8 *config_values);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS NU_USBH_STACK_U1_Enable         (NU_USB_STACK  *cb,
                                        NU_USB_DEVICE *device);
                                       
STATUS NU_USBH_STACK_U2_Enable         (NU_USB_STACK  *cb,
                                        NU_USB_DEVICE *device);
                                       
STATUS NU_USBH_STACK_LTM_Enable        (NU_USB_STACK  *cb,
                                        NU_USB_DEVICE *device);
                                       
STATUS _NU_USBH_STACK_Function_Suspend (NU_USB_STACK    *cb,
                                        NU_USB_INTF     *intf,
                                        BOOLEAN         func_suspend,
                                        BOOLEAN         rmt_wakeup);
                                       
STATUS NU_USBH_STACK_U1_Disable        (NU_USB_STACK  *cb,
                                        NU_USB_DEVICE *device);
                                       
STATUS NU_USBH_STACK_U1_Disable        (NU_USB_STACK  *cb,
                                        NU_USB_DEVICE *device);
                                       
STATUS NU_USBH_STACK_LTM_Disable       (NU_USB_STACK  *cb,
                                        NU_USB_DEVICE *device);

#endif      /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
/* ==================================================================== */

#endif /* _NU_USBH_STACK_EXT_H_ */

/* ======================  End Of File  =============================== */
