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
*    FILE NAME
*
*        nu_usb_cfg_ext.h 
* 
*    COMPONENT 
*
*        Nucleus USB Software 
* 
*    DESCRIPTION
*
*        This file contains the declaration for external interfaces exposed
*        by Nucleus USB software's Configuration component.
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
*        nu_usb_cfg_imp.h             Configuration's Internal definitions
* 
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_CFG_EXT_H_
#define _NU_USB_CFG_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================== Include Files ================================ */
#include "connectivity/nu_usb_cfg_imp.h"

/* ====================  Function Prototypes ========================== */
STATUS NU_USB_CFG_Get_Desc (NU_USB_CFG * cb,
                            NU_USB_CFG_DESC ** cfg_desc_out);

/* OTG descriptor of the device */
STATUS NU_USB_CFG_Get_OTG_Desc (NU_USB_CFG * cb,
                                NU_USB_OTG_DESC ** otg_desc_out);

/* Services for the active field */
STATUS NU_USB_CFG_Get_Is_Active (NU_USB_CFG * cb,
                                 BOOLEAN * is_active_out);
STATUS NU_USB_CFG_Set_Is_Active (NU_USB_CFG * cb,
                                 BOOLEAN is_active);

/* Services for the cfg value */
STATUS NU_USB_CFG_Get_Cfg_Value (NU_USB_CFG * cb,
                                 UINT8 *cfg_value_out);

/* Services for the status */
STATUS NU_USB_CFG_Get_Is_Self_Powered (NU_USB_CFG * cb,
                                       BOOLEAN * is_self_powered_out);
STATUS NU_USB_CFG_Get_Is_Wakeup (NU_USB_CFG * cb,
                                 BOOLEAN * is_wakeup_out);

/* Services for the max power value */
STATUS NU_USB_CFG_Get_Max_Power (NU_USB_CFG * cb,
                                 UINT8 *max_power_out);

/* Services for the intfs */
STATUS NU_USB_CFG_Get_Num_Intfs (NU_USB_CFG * cb,
                                 UINT8 *number_intfs_out);
STATUS NU_USB_CFG_Get_Intf (NU_USB_CFG * cb,
                            UINT8 intf_num,
                            NU_USB_INTF ** intf_out);
STATUS NU_USB_CFG_Get_wTotalLength (NU_USB_CFG * cb,
                                    UINT32 *wTotalLength_out);

/* Services for the cfgs's string(s) */
STATUS NU_USB_CFG_Get_String (NU_USB_CFG * cb,
                              CHAR * string_out);
STATUS NU_USB_CFG_Get_String_Num (NU_USB_CFG * cb,
                                  UINT8 *string_num_out);
STATUS NU_USB_CFG_Get_String_Desc (NU_USB_CFG * cb,
                                   UINT16 wLangId,
                                   NU_USB_STRING_DESC * string_desc_out);

/* Class specific descriptor service */
STATUS NU_USB_CFG_Get_Class_Desc (NU_USB_CFG * cb,
                                  UINT8 **class_desc_out,
                                  UINT32 *length_out);

/* Services to access associated objects */
STATUS NU_USB_CFG_Get_Device (NU_USB_CFG * cb,
                              NU_USB_DEVICE ** device_out);

STATUS NU_USB_CFG_Find_Alt_Setting (NU_USB_CFG * cb,
                                    UINT32 match_flag,
                                    UINT8 intf_num,
                                    UINT8 alt_settg,
                                    UINT8 bInterfaceClass,
                                    UINT8 bInterfaceSubClass,
                                    UINT8 bInterfaceProtocol,
                                    NU_USB_ALT_SETTG ** alt_settg_out);

STATUS NU_USB_CFG_Get_IAD (NU_USB_CFG * cb,
                           UINT8 intf_num,
                           NU_USB_IAD ** iad_out);

STATUS NU_USB_CFG_Get_Num_IADs (NU_USB_CFG * cb,
                                UINT8 *number_iads_out);

/* ==================================================================== */
#endif /* _NU_USb_CFG_EXT_H_ */
/* ======================  End Of File  =============================== */

