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
 *      nu_usb_imp.h
 *
 * COMPONENT
 *      USB Base
 *
 * DESCRIPTION
 *      This file contains the function names and data structures
 *      for the USB base component.
 *
 *
 * DATA STRUCTURES
 *      nu_usb             USB Object control block
 *      usb_dispatch       NU_USB's dispatch table
 *
 * FUNCTIONS
 *      None
 *
 * DEPENDENCIES
 *
 *      nucleus.h               System definitions
 *      nu_kernel.h             Kernel definitions
 *      string.h                String Services Definitions
 *      string.h                Posix String Services Definitions
 *      cs_defs.h
 *      cs_extr.h               Common Services definitions
 *      er_extr.h               Error Services definitions
 *
 *      All the external definitions files of USB products purchased. These
 *      are filled in by Nucleus Configuration tool while installing the
 *      product.
 *
 *************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_IMP_H_
#define _NU_USB_IMP_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "kernel/dev_mgr.h"

#ifdef INCLUDE_NU_POSIX
#if (INCLUDE_NU_POSIX == NU_TRUE)
#include "posix_rtl/inc/string.h"
#else
#include <string.h>
#endif
#else
#include <string.h>
#endif

#if ((PLUS_VERSION_COMP == PLUS_1_14) || \
     (PLUS_VERSION_COMP == PLUS_1_13) || \
     (PLUS_VERSION_COMP == PLUS_1_11))
#include "plus/cs_defs.h"
#include "plus/cs_extr.h"
#include "plus/er_extr.h"
#else
#include "kernel/nu_kernel.h"
#endif

/* ====================  Forward Declarations ========================= */
typedef struct nu_usb NU_USB;
typedef struct nu_usb_subsys NU_USB_SUBSYS;

/* =========================  Common Macros =========================== */

/* Macro to set whether to use MMU support in USB Public APIs or not. */
#define USB_MMU_SUPPORT_ENABLE              NU_FALSE

/* Macro to set whether to check memory pool pointers for being NULL or not. */
#define USB_MEM_POOL_PTR_CHECK              NU_FALSE

/* Define this macro if you want to enable the input parameter validation
 * in all APIs offered by the USB stack and its related class drivers. By
 * default this macro is on and user is advised to keep this macro on
 * during development so that he can trace any invalid inputs. If a user
 * wants speed optimization he can disable this by undefining or
 * commenting the definition below.
 */
#define NU_USB_PARAMETER_CHECK

/*if OTG support has been enabled, both USB function and stack should also be enabled*/
#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 1
#if !(defined(CFG_NU_OS_CONN_USB_FUNC_STACK_ENABLE) && defined(CFG_NU_OS_CONN_USB_HOST_STACK_ENABLE))
#error OTG support requires on nu.os.conn.usb.func.stack and nu.os.conn.usb.host.stack. Both must be enabled.
#endif
extern STATUS NU_USB_OTG_Install_ISR(INT vector, VOID(*isr)(INT) );
#endif

#define NU_USB_OPTIMIZE_FOR_SIZE            CFG_NU_OS_CONN_USB_COM_STACK_OPTIMIZE_FOR_SIZE
#if (NU_USB_OPTIMIZE_FOR_SIZE)
/* Disable parameter validation if size optimization is required. */
#undef NU_USB_PARAMETER_CHECK
#endif

#ifdef NU_USB_PARAMETER_CHECK
#define NU_USB_PTRCHK(a) \
    if((a) == NU_NULL)     \
        return (NU_USB_INVLD_ARG);
#else
#define NU_USB_PTRCHK(a)
#endif

#ifdef NU_USB_PARAMETER_CHECK
#if (USB_MMU_SUPPORT_ENABLE == NU_TRUE)
#define NU_USB_PTRCHK_RETURN(a) \
    if((a) == NU_NULL){         \
        NU_USER_MODE();         \
        return (NU_USB_INVLD_ARG);\
    }
#else
#define NU_USB_PTRCHK_RETURN(a)             NU_USB_PTRCHK(a)
#endif
#else
#define NU_USB_PTRCHK_RETURN(a)
#endif

#if (USB_MEM_POOL_PTR_CHECK == NU_TRUE)
#define NU_USB_MEMPOOLCHK(a)                 NU_USB_PTRCHK(a)
#define NU_USB_MEMPOOLCHK_RETURN(a)          NU_USB_PTRCHK_RETURN(a)
#else
#define NU_USB_MEMPOOLCHK(a)
#define NU_USB_MEMPOOLCHK_RETURN(a)
#endif

/* ====================  Data Structures ============================== */

/* USB control block. */
typedef struct usb_dispatch
{
    STATUS (*Delete) (VOID *cb);
    STATUS (*Get_Name) (NU_USB * cb, CHAR * name_out);
    STATUS (*Get_Object_Id) (NU_USB * cb, UINT32 *id_out);
}
NU_USB_DISPATCH;

struct nu_usb
{
    CS_NODE               list_node;
    const NU_USB_DISPATCH *usb_dispatch;
    CHAR                  name[NU_MAX_NAME];
    NU_USB_SUBSYS         *subsys;
    UINT32                object_id;
};

/* ====================  Function Prototypes ========================== */

/* =======================  USB Include Files ========================= */

/* USB Base Stack */
#include "connectivity/nu_usb_error.h"
#include "connectivity/nu_usb_subsys_ext.h"
#include "connectivity/nu_usb_stack_ext.h"
#include "connectivity/nu_usb_pmg_ext.h"
#include "connectivity/nu_usb_bos_ext.h"
#include "connectivity/nu_usb_drvr_ext.h"
#include "connectivity/nu_usb_pipe_ext.h"
#include "connectivity/nu_usb_endp_ext.h"
#include "connectivity/nu_usb_alt_settg_ext.h"
#include "connectivity/nu_usb_intf_ext.h"
#include "connectivity/nu_usb_iad_ext.h"
#include "connectivity/nu_usb_cfg_ext.h"
#include "connectivity/nu_usb_device_ext.h"
#include "connectivity/nu_usb_init_ext.h"

#include "connectivity/nu_usb_hw_ext.h"
#include "connectivity/nu_usb_irp_ext.h"
#include "connectivity/nu_usb_iso_irp_ext.h"
#include "connectivity/nu_usb_rwl_imp.h"
#include "connectivity/nu_usb_mem_imp.h"
#include "connectivity/nu_usb_mutex_imp.h"
#include "connectivity/nu_usb_user_ext.h"
#include "connectivity/nu_usb_sys_ext.h"
#include "connectivity/nu_usb_strm_ext.h"

/* USB Function Stack */
#ifdef CFG_NU_OS_CONN_USB_FUNC_STACK_ENABLE
#include "connectivity/nu_usbf_user_ext.h"
#include "connectivity/nu_usbf_subsys_ext.h"
#include "connectivity/nu_usbf_stack_ext.h"
#include "connectivity/nu_usbf_init_ext.h"
#include "connectivity/nu_usbf_hw_ext.h"
#include "connectivity/nu_usbf_ext.h"
#include "connectivity/nu_usbf_drvr_ext.h"
#include "connectivity/nu_usbf_devcfg_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_HOST_STACK_ENABLE
/* USB Host Stack */
#include "connectivity/nu_usbh_user_ext.h"
#include "connectivity/nu_usbh_stack_ext.h"
#include "connectivity/nu_usbh_init_ext.h"
#include "connectivity/nu_usbh_hw_ext.h"
#include "connectivity/nu_usbh_ext.h"
#include "connectivity/nu_usbh_drvr_ext.h"
#include "connectivity/nu_usbh_ctrl_irp_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_HOST_COMM_CLASS_ENABLE
/* USB Host Communications */
#include "connectivity/nu_usbh_com_acm_ext.h"
#include "connectivity/nu_usbh_com_ecm_ext.h"
#include "connectivity/nu_usbh_com_ext.h"
#include "connectivity/nu_usbh_com_user_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_FUNC_COMM_CLASS_ENABLE
/* USB Function Communications */
#include "connectivity/nu_usbf_comm_data_ext.h"
#include "connectivity/nu_usbf_comm_ext.h"
#include "connectivity/nu_usbf_user_comm_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_HOST_COMM_ETH_ENABLE
/* USB Host Ethernet Communications */
#include "connectivity/nu_usbh_com_eth_ext.h"
#endif


#ifdef CFG_NU_OS_CONN_USB_HOST_HID_CLASS_ENABLE
/* USB Host HID. */
#include "connectivity/nu_usbh_hid_ext.h"
#include "connectivity/nu_usbh_hid_user_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_HOST_HID_KEYBOARD_ENABLE
#include "connectivity/nu_usbh_kbd_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_HOST_HID_MOUSE_ENABLE
#include "connectivity/nu_usbh_mouse_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_HOST_AUDIO_CLASS_ENABLE
/* USB Host Audio. */
#include "connectivity/nu_usbh_audio.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_FUNC_COMM_CLASS_ENABLE
/* USB Function Ethernet Communications */
#include "connectivity/nu_usbf_eth_cfg.h"
#include "connectivity/nu_usbf_eth_ext.h"
/* USB Function RNDIS */
#include "connectivity/nu_usbf_ndis_user_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_HOST_COMM_MDM_ENABLE
/* USB Host Modem Communications */
#include "connectivity/nu_usbh_com_mdm_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_FUNC_COMM_MDM_ENABLE
/* USB Function Modem Communications */
#include "connectivity/nu_usbf_acm_user_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_FUNC_HID_CLASS_ENABLE
/* USB Function HID Class Driver */
#include "connectivity/nu_usbf_hid_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_FUNC_HID_KBD_ENABLE
/* USB Function HID Keyboard User Driver */
#include "connectivity/nu_usbf_kb_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_FUNC_HID_MSE_ENABLE
/* USB Function HID Mouse User Driver */
#include "connectivity/nu_usbf_mse_ext.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_FUNC_DFU_ENABLE
/* USB Function DFU Class Driver */
#include "connectivity/nu_usbf_dfu.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_HOST_MS_ENABLE
/* USB Host Mass Storage */
#include "connectivity/nu_usbh_ms_ext.h"
#include "connectivity/nu_usbh_ms_user_ext.h"
#endif


#ifdef CFG_NU_OS_CONN_USB_FUNC_MS_ENABLE
/* USB Function Mass Storage */
#include "connectivity/nu_usbf_ms_ext.h"
#include "connectivity/nu_usbf_scsi_media_ext.h"
#include "connectivity/nu_usbf_user_ms_ext.h"
#include "connectivity/nu_usbf_user_scsi_ext.h"
#endif

/* Include files to run USB stack tests. */
#if  (USB_STACK_TESTS == NU_TRUE)

/* USB Virtual Hardware Drivers. */
#include "hardware_drivers/usb/host/virtual/inc/nu_usbh_vcd_ext.h"
#include "hardware_drivers/usb/function/virtual/inc/nu_usbf_vcd_ext.h"
#include "hardware_drivers/usb/physical/virtual/inc/nu_usb_vpl_ext.h"

/* USB Function Audio */
#include "os/include/connectivity/nu_usbf_audio_ext.h"
#include "os/include/connectivity/nu_usbf_audio_user_ext.h"
#include "os/include/connectivity/nu_usbf_audio_ctrl_ext.h"
#include "os/include/connectivity/nu_usbf_audio_strm_ext.h"

/* USB Host Audio */
#include "os/include/connectivity/nu_usbh_audio_ext.h"
#include "os/include/connectivity/nu_usbh_audio_user_ext.h"

#endif

#if (USB_TEST_MODE_SUPPORT == NU_TRUE)

/* USB 2.0 Compliance Test Support*/
#include "connectivity/nu_usb_test_ext.h"

#ifdef CFG_NU_OS_CONN_USB_FUNC_ENABLE
#include "connectivity/nu_usbf_test_ext.h"
#endif /* CFG_NU_OS_CONN_USB_FUNC_ENABLE */

#ifdef CFG_NU_OS_CONN_USB_HOST_ENABLE
#include "connectivity/nu_usbh_test_ext.h"
#endif

#endif/* USB_TEST_MODE_SUPPORT */

#ifdef CFG_NU_OS_NET_ENABLE
#ifdef CFG_NU_OS_DRVR_USB_HOST_NET_IF_ENABLE
/* USB Host NET Driver */
#include "drivers/nu_usbh_net_ext.h"
#endif /* CFG_NU_OS_DRVR_USB_HOST_NET_IF_ENABLE */

#ifdef CFG_NU_OS_DRVR_USB_FUNC_NET_IF_ENABLE
/* USB Function NET Driver */
#include "drivers/nu_usbf_net_ext.h"
#endif /* CFG_NU_OS_DRVR_USB_FUNC_NET_IF_ENABLE */

#endif /* CFG_NU_OS_NET_ENABLE */

#ifdef CFG_NU_OS_STOR_FILE_ENABLE
#ifdef CFG_NU_OS_DRVR_USB_HOST_FILE_IF_ENABLE
/* USB Host FILE Driver */
#include "drivers/nu_usbh_nuf_ext.h"
#endif /* CFG_NU_OS_DRVR_USB_HOST_FILE_IF_ENABLE */
#endif /* CFG_NU_OS_STOR_FILE_ENABLE */


#endif

/************************* end of file *********************************/
