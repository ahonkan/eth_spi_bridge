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
*       nu_usbf_hw_ext.h 
*
*
* COMPONENT
*
*       Stack Component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the exported function prototype declarations
*       for the common hw controller driver .
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
*       nu_usbf_hw_imp.h       USB Function hardware
*
**************************************************************************/

#ifndef     _NU_USBF_HW_EXT_H
#define     _NU_USBF_HW_EXT_H

/* ==============  USB Include Files ==================================  */

#include    "connectivity/nu_usbf_hw_imp.h"

/* Create and Delete    */
STATUS _NU_USBF_HW_Create (NU_USBF_HW   *cb,
                           CHAR         *name,
                           UINT32        capability,
                           UINT8         speed,
                           VOID         *base_address,
                           UINT8         num_irq,
                           INT          *irq,
                           const VOID   *dispatch);

STATUS _NU_USBF_HW_Delete (VOID *cb);

STATUS _NU_USBF_HW_Initialize (NU_USB_HW    *cb,
                               NU_USB_STACK *stack);

STATUS _NU_USBF_HW_Uninitialize (NU_USB_HW *cb);

/* Callback API */
STATUS NU_USBF_HW_Get_Capability (NU_USBF_HW *cb,
                                  UINT32     *capability);

STATUS NU_USBF_HW_Set_Address (NU_USBF_HW *cb,
                               UINT8       address);

STATUS NU_USBF_HW_Get_Status (NU_USBF_HW *cb,
                              UINT16     *status);

STATUS NU_USBF_HW_Get_Endpoint_Status (NU_USBF_HW *cb,
                                       UINT8       bEndpointAddress,
                                       UINT16     *status);

STATUS NU_USBF_HW_Stall_Endpoint (NU_USBF_HW *cb,
                                  UINT8       bEndpointAddress);

STATUS NU_USBF_HW_Unstall_Endpoint (NU_USBF_HW *cb,
                                    UINT8       bEndpointAddress);

STATUS NU_USBF_HW_Start_HNP (NU_USBF_HW *cb, UINT8 port_id);

STATUS NU_USBF_HW_Is_Dual_Speed_Device(	NU_USBF_HW	*cb, BOOLEAN *is_dual_speed);

STATUS NU_USBF_HW_Get_EP0_Maxp( NU_USBF_HW *cb, UINT8 speed, UINT8 *maxp);

STATUS NU_USBF_HW_Acquire_Endp(	NU_USBF_HW	*cb,
								UINT8		speed,
								UINT8		attribs,
								UINT8		ep_dir,
								UINT8		config_num,
								UINT8		intf_num,
								UINT8		alt_sttg,
								UINT8		*ep_num,
								UINT16		*maxp,
								UINT8		*interval);

STATUS NU_USBF_HW_Release_Endp(	NU_USBF_HW	*cb,
								UINT8		speed,
								UINT8		ep_addr,
								UINT8		cfg_num,
								UINT8		intf_num,
								UINT8		alt_sttg);

STATUS NU_USBF_HW_Enable_Pullup(NU_USBF_HW	*cb);
STATUS NU_USBF_HW_Disable_Pullup(NU_USBF_HW	*cb);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS NU_USBF_HW_Send_FuncWakeNotif (NU_USBF_HW *cb, UINT8 intf_num);

STATUS NU_USBF_HW_Set_Ux_Enable (NU_USBF_HW *cb, UINT8 link_state, BOOLEAN enable);

STATUS NU_USBF_HW_Set_LTM_Enable (NU_USBF_HW *cb, BOOLEAN enable);

STATUS NU_USBF_HW_Is_LTM_Capable(NU_USBF_HW *cb, BOOLEAN *is_ltm_capable);

STATUS NU_USBF_HW_Get_Supported_Speeds(NU_USBF_HW *cb, UINT16 *supported_speeds);

STATUS NU_USBF_HW_Get_U1DevExitLat(NU_USBF_HW *cb, UINT8 *u1devexitlat);

STATUS NU_USBF_HW_Get_U2DevExitLat(NU_USBF_HW *cb, UINT16 *u2devexitlat);

#endif

/* ===================================================================== */

#endif      /* _NU_USBF_HW_EXT_H         */

/* =======================  End Of File  ============================== */

