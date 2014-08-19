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
*        nu_usb_alt_settg_ext.h 
* 
*    COMPONENT 
*
*        Nucleus USB Software 
* 
*    DESCRIPTION 
*
*        This file contains the declaration for external interfaces exposed
*        by Nucleus USB software's Alternate setting component.
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
*        nu_usb_alt_settg_imp.h    Alternate settings Internal definitions
* 
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_ALT_SETTG_EXT_H_
#define _NU_USB_ALT_SETTG_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================== Include Files ================================ */
#include "connectivity/nu_usb_alt_settg_imp.h"

/* ====================  Function Prototypes ========================== */
STATUS NU_USB_ALT_SETTG_Get_Desc (NU_USB_ALT_SETTG * cb,
                                  NU_USB_INTF_DESC ** intf_desc_out);

STATUS NU_USB_ALT_SETTG_Get_Is_Active (NU_USB_ALT_SETTG * cb,
                                       BOOLEAN * is_active_out);

STATUS NU_USB_ALT_SETTG_Set_Active (NU_USB_ALT_SETTG * cb);

STATUS NU_USB_ALT_SETTG_Get_bAlternateSetting (NU_USB_ALT_SETTG * cb,
                                               UINT8 *bAlternateSetting_out);

STATUS NU_USB_ALT_SETTG_Get_Class (NU_USB_ALT_SETTG * cb,
                                   UINT8 *bInterfaceClass_out);

STATUS NU_USB_ALT_SETTG_Get_SubClass (NU_USB_ALT_SETTG * cb,
                                      UINT8 *bInterfaceSubClass_out);

STATUS NU_USB_ALT_SETTG_Get_Protocol (NU_USB_ALT_SETTG * cb,
                                      UINT8 *bInterfaceProtocol_out);

STATUS NU_USB_ALT_SETTG_Get_Class_Desc (NU_USB_ALT_SETTG * cb,
                                        UINT8 **class_desc_out,
                                        UINT32 *length_out);

STATUS NU_USB_ALT_SETTG_Get_String (NU_USB_ALT_SETTG * cb,
                                    CHAR * string_out);

STATUS NU_USB_ALT_SETTG_Get_String_Num (NU_USB_ALT_SETTG * cb,
                                        UINT8 *string_num_out);

STATUS NU_USB_ALT_SETTG_Get_String_Desc (NU_USB_ALT_SETTG * cb,
                                         UINT16 wLangId,
                                         NU_USB_STRING_DESC * string_desc_out);

STATUS NU_USB_ALT_SETTG_Get_Num_Endps (NU_USB_ALT_SETTG * cb,
                                       UINT8 *number_endps_out);

STATUS NU_USB_ALT_SETTG_Find_Pipe (NU_USB_ALT_SETTG * cb,
                                   UINT32 match_flag,
                                   UINT8 endp_num,
                                   UINT8 direction,
                                   UINT8 type,
                                   NU_USB_PIPE ** pipe_out);

STATUS NU_USB_ALT_SETTG_Get_Endp (NU_USB_ALT_SETTG * cb,
                                  UINT8 number_endp,
                                  NU_USB_ENDP ** endp_out);

/* ==================================================================== */
#endif /* _NU_USB_ALT_SETTG_EXT_H_ */

/* ======================  End Of File  =============================== */
