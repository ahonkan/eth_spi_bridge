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
 *      nu_usb.h
 * 
 * COMPONENT 
 *      Nucleus USB Software
 * 
 * DESCRIPTION 
 *      This is the central header file for whole Nucleus USB software.
 *      Applications using Nucleus USB includes this and gets the services
 *      of all the Nucleus USB products.
 *
 * 
 * DATA STRUCTURES 
 *      None
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      nu_usb_ext.h           Nucleus USB external definitions.
 * 
 *************************************************************************/
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#define _NUCLEUS_USB_H_
#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif

/* ===================== Include Files ================================ */
#define USB_STACK_TESTS             CFG_NU_OS_CONN_USB_COM_STACK_TESTS_ENABLE

/* USB 2.0 Test Mode Support */
#define USB_TEST_MODE_SUPPORT       CFG_NU_OS_CONN_USB_COM_STACK_TEST_MODES_SUPPORT

#include "connectivity/nu_usb_ext.h"

#define USB_ERR
#if(defined(USB_ERR))

#if(USB_STACK_TESTS == NU_TRUE)

#define DEFAULT_UNIT_TEST_CASE_ID   0

#include "common/mapped_files/plus_mapped_files/nu_plus_err.h"

#include "common/mapped_files/usb_mapped_files/inc/usb_stack_ext_err.h"
#include "common/mapped_files/usb_mapped_files/inc/usb_intf_ext_err.h"
#include "common/mapped_files/usb_mapped_files/inc/usb_alt_settg_ext_err.h"
#include "common/mapped_files/usb_mapped_files/inc/usb_device_ext_err.h"
#include "common/mapped_files/usb_mapped_files/inc/usb_cfg_ext_err.h"
#include "common/mapped_files/usb_mapped_files/inc/usb_drvr_ext_err.h"
#include "common/mapped_files/usb_mapped_files/inc/usb_user_ext_err.h"
#include "common/mapped_files/usb_mapped_files/inc/usb_hw_ext_err.h"
#include "common/mapped_files/usb_mapped_files/inc/usb_mem_imp_err.h"

#include "common/mapped_files/usbh_mapped_files/inc/usbh_ext_err.h"
#include "common/mapped_files/usbh_mapped_files/inc/usbh_stack_ext_err.h"
#include "common/mapped_files/usbh_mapped_files/inc/usbh_drvr_hub_imp_err.h"
#include "common/mapped_files/usbh_mapped_files/inc/usbh_stack_imp_err.h"

#include "common/mapped_files/usbf_mapped_files/inc/usbf_drvr_ext_err.h"
#include "common/mapped_files/usbf_mapped_files/inc/usbf_ext_err.h"
#include "common/mapped_files/usbf_mapped_files/inc/usbf_stack_ext_err.h"
#include "common/mapped_files/usbf_mapped_files/inc/usbf_stack_imp_err.h"

#endif /* USB_STACK_TESTS == NU_TRUE */
#endif /* USB_ERR */

/* ==================================================================== */
#ifdef          __cplusplus
}                                           /* End of C declarations     */
#endif
#endif /* _NUCLEUS_USB_H_ */

/* ======================  End Of File  =============================== */
