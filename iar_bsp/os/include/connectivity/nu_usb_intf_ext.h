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
 *      nu_usb_intf_ext.h 
 * 
 * COMPONENT 
 *      Nucleus USB Software 
 * 
 * DESCRIPTION 
 *      This file contains the declaration for external interfaces exposed
 *      by Nucleus USB software's Interface component.
 *
 * 
 * DATA STRUCTURES 
 *          None
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      nu_usb_intf_imp.h               Interface's Internal definitions
 * 
 *************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_INTF_EXT_H_
#define _NU_USB_INTF_EXT_H_

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ===================== Include Files ================================ */
#include "connectivity/nu_usb_intf_imp.h"

/* ====================  Function Prototypes ========================== */

STATUS NU_USB_INTF_Get_Intf_Num (NU_USB_INTF * cb,
                                 UINT8 *intf_num_out);

STATUS NU_USB_INTF_Get_Desc (NU_USB_INTF * cb,
                             NU_USB_INTF_DESC ** intf_desc_out);

STATUS NU_USB_INTF_Get_Status (NU_USB_INTF * cb,
                               UINT16 *status_out);

STATUS NU_USB_INTF_Get_Class (NU_USB_INTF * cb,
                              UINT8 *bInterfaceClass_out);

STATUS NU_USB_INTF_Get_SubClass (NU_USB_INTF * cb,
                                 UINT8 *bInterfaceSubClass_out);

STATUS NU_USB_INTF_Get_Protocol (NU_USB_INTF * cb,
                                 UINT8 *bInterfaceProtocol_out);

STATUS NU_USB_INTF_Get_String (NU_USB_INTF * cb,
                               CHAR * string_out);

STATUS NU_USB_INTF_Get_String_Num (NU_USB_INTF * cb,
                                   UINT8 *string_num_out);

STATUS NU_USB_INTF_Get_String_Desc (NU_USB_INTF * cb,
                                    UINT16 wLangId,
                                    NU_USB_STRING_DESC * string_desc_out);

STATUS NU_USB_INTF_Get_Num_Alt_Settings (NU_USB_INTF * cb,
                                         UINT8 *number_alt_settings_out);

STATUS NU_USB_INTF_Get_Alt_Setting (NU_USB_INTF * cb,
                                    UINT8 alt_setting_num,
                                    NU_USB_ALT_SETTG ** alt_setting_out);
STATUS NU_USB_INTF_Get_Active_Alt_Setting (NU_USB_INTF * cb,
                                           NU_USB_ALT_SETTG ** alt_setting_out);
STATUS NU_USB_INTF_Get_Active_Alt_Setting_Num (NU_USB_INTF * cb,
                                               UINT8 *alt_setting_num_out);
STATUS NU_USB_INTF_Set_Interface (NU_USB_INTF * cb,
                                  UINT8 alt_setting_num);

/* Services for class drivers */
STATUS NU_USB_INTF_Claim (NU_USB_INTF * cb,
                          NU_USB_DRVR * drvr);
STATUS NU_USB_INTF_Release (NU_USB_INTF * cb,
                            NU_USB_DRVR * drvr);
STATUS NU_USB_INTF_Get_Is_Claimed (NU_USB_INTF * cb,
                                   BOOLEAN * is_claimed_out,
                                   NU_USB_DRVR ** drvr_out);
STATUS NU_USB_INTF_Get_IAD (NU_USB_INTF * cb,
                            NU_USB_IAD ** iad_out);
STATUS NU_USB_INTF_Get_Cfg (NU_USB_INTF * cb,
                            NU_USB_CFG ** cfg_out);
STATUS NU_USB_INTF_Get_Device (NU_USB_INTF * cb,
                               NU_USB_DEVICE ** device_out);

STATUS NU_USB_INTF_Find_Alt_Setting (NU_USB_INTF * cb,
                                     UINT32 match_flag,
                                     UINT8 alt_settg,
                                     UINT8 bInterfaceClass,
                                     UINT8 bInterfaceSubClass,
                                     UINT8 bInterfaceProtocol,
                                     NU_USB_ALT_SETTG ** alt_settg_out);

/* ==================================================================== */

#endif /* _NU_USB_INTF_EXT_H_ */

/* ======================  End Of File  =============================== */

