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
*        nu_usb_device_ext.h 
*
*    COMPONENT
*
*        Nucleus USB Software 
*
*    DESCRIPTION
*
*        This file contains the exported function names and data structures
*        for the NU_USB_DEVICE class.
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
*        nu_usb_device_imp.h    Device's internal definitions.
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_DEVICE_EXT_H
#define _NU_USB_DEVICE_EXT_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */

/* ==============  USB Include Files =================================  */
#include        "connectivity/nu_usb_device_imp.h"

/* ================== Function Prototypes  =========================== */
STATUS NU_USB_DEVICE_Get_Desc (NU_USB_DEVICE * cb,
                               NU_USB_DEVICE_DESC ** device_desc_out);

STATUS NU_USB_DEVICE_Get_Stack (NU_USB_DEVICE * cb,
                                NU_USB_STACK ** stack_out);

STATUS NU_USB_DEVICE_Set_Stack (NU_USB_DEVICE * cb,
                                NU_USB_STACK * stack);

STATUS NU_USB_DEVICE_Claim (NU_USB_DEVICE * cb,
                            NU_USB_DRVR * drvr);

STATUS NU_USB_DEVICE_Release (NU_USB_DEVICE * cb);

STATUS NU_USB_DEVICE_Get_Is_Claimed (NU_USB_DEVICE * cb,
                                     BOOLEAN * is_claimed_out,
                                     NU_USB_DRVR ** drvr_out);

STATUS NU_USB_DEVICE_Get_Hw (NU_USB_DEVICE * cb,
                             NU_USB_HW ** hw_out);

STATUS NU_USB_DEVICE_Set_Hw (NU_USB_DEVICE * cb,
                             NU_USB_HW * hw);

/* Services for the device's cfg(s) */
STATUS NU_USB_DEVICE_Get_Num_Cfgs (NU_USB_DEVICE * cb,
                                   UINT8 *number_cfgs_out);

STATUS NU_USB_DEVICE_Get_Cfg (NU_USB_DEVICE * cb,
                              UINT8 cfg_num,
                              NU_USB_CFG ** cfg_out);

STATUS NU_USB_DEVICE_Get_Active_Cfg_Num (NU_USB_DEVICE * cb,
                                         UINT8 *active_cfg_num_out);

STATUS NU_USB_DEVICE_Get_Active_Cfg (NU_USB_DEVICE * cb,
                                     NU_USB_CFG ** cfg_out);

STATUS NU_USB_DEVICE_Set_Active_Cfg (NU_USB_DEVICE * cb,
                                     NU_USB_CFG * cfg);

STATUS NU_USB_DEVICE_Get_Function_Addr (NU_USB_DEVICE * cb,
                                        UINT8 *function_address);

STATUS NU_USB_DEVICE_Get_Parent (NU_USB_DEVICE * cb,
                                 NU_USB_DEVICE ** parent_out);

STATUS NU_USB_DEVICE_Get_Port_Number (NU_USB_DEVICE * cb,
                                      UINT8 *port_num_out);

STATUS NU_USB_DEVICE_Get_OTG_Status (NU_USB_DEVICE * cb,
                                      UINT8 *otg_status_out);

STATUS NU_USB_DEVICE_Get_OTG_Desc (NU_USB_DEVICE * cb,
                                   NU_USB_OTG_DESC ** otg_desc_out);

/* Services for the device's string(s) */
STATUS NU_USB_DEVICE_Get_String (NU_USB_DEVICE * cb,
                                 UINT8 string_num,
                                 CHAR * string_out);

STATUS NU_USB_DEVICE_Get_String_Desc (NU_USB_DEVICE * cb,
                                      UINT8 string_num,
                                      UINT16 wLangId,
                                      NU_USB_STRING_DESC* string_desc_out);

STATUS NU_USB_DEVICE_Get_Manf_String_Num (NU_USB_DEVICE * cb,
                                          UINT8 *string_num_out);

STATUS NU_USB_DEVICE_Get_Manf_String (NU_USB_DEVICE * cb,
                                         CHAR * string_out);

STATUS NU_USB_DEVICE_Get_Manf_String_Desc (NU_USB_DEVICE * cb,
                                           UINT16 wLangId,
                                           NU_USB_STRING_DESC *string_desc_out);

STATUS NU_USB_DEVICE_Get_Product_String_Num (NU_USB_DEVICE * cb,
                                             UINT8 *string_num_out);

STATUS NU_USB_DEVICE_Get_Product_String (NU_USB_DEVICE * cb,
                                            CHAR * string_out);

STATUS NU_USB_DEVICE_Get_Product_String_Desc (NU_USB_DEVICE * cb,
                                              UINT16 wLangId,
                                              NU_USB_STRING_DESC *string_desc_out);

STATUS NU_USB_DEVICE_Get_Serial_Num_String_Num (NU_USB_DEVICE * cb,
                                                UINT8 *string_num_out);

STATUS NU_USB_DEVICE_Get_Serial_Num_String (NU_USB_DEVICE * cb,
                                            CHAR * string_out);

STATUS NU_USB_DEVICE_Get_Serial_Num_String_Desc (NU_USB_DEVICE * cb,
                                                 UINT16 wLangId,
                                                 NU_USB_STRING_DESC* string_desc_out);

/* Services for the device's speed */
STATUS NU_USB_DEVICE_Get_Speed (NU_USB_DEVICE * cb,
                                UINT8 *speed_out);

/* Device status */
STATUS NU_USB_DEVICE_Get_Status (NU_USB_DEVICE * cb,
                                 UINT16 *status_out);
STATUS NU_USB_DEVICE_Set_Status (NU_USB_DEVICE * cb,
                                 UINT16 status);

STATUS NU_USB_DEVICE_Get_bcdUSB (NU_USB_DEVICE * cb,
                                 UINT16 *bcdUSB_out);

STATUS NU_USB_DEVICE_Get_bDeviceClass (NU_USB_DEVICE * cb,
                                       UINT8 *bDeviceClass_out);

STATUS NU_USB_DEVICE_Get_bDeviceSubClass (NU_USB_DEVICE * cb,
                                          UINT8 *bDeviceSubClass_out);

STATUS NU_USB_DEVICE_Get_bDeviceProtocol (NU_USB_DEVICE * cb,
                                          UINT8 *bDeviceProtocol_out);

STATUS NU_USB_DEVICE_Get_bMaxPacketSize0 (NU_USB_DEVICE *cb,
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                          UINT16        *bMaxPacketSize0_out);
#else
                                          UINT8         *bMaxPacketSize0_out);
#endif

STATUS NU_USB_DEVICE_Get_idVendor (NU_USB_DEVICE * cb,
                                   UINT16 *idVendor_out);

STATUS NU_USB_DEVICE_Get_idProduct (NU_USB_DEVICE * cb,
                                    UINT16 *idProduct_out);

STATUS NU_USB_DEVICE_Get_bcdDevice (NU_USB_DEVICE * cb,
                                    UINT16 *bcdDevice_out);

STATUS NU_USB_DEVICE_Lock(NU_USB_DEVICE * cb);

STATUS NU_USB_DEVICE_Unlock(NU_USB_DEVICE * cb);

STATUS NU_USB_DEVICE_Set_Desc (NU_USB_DEVICE * cb,
                               NU_USB_DEVICE_DESC *device_desc);

STATUS NU_USB_DEVICE_Set_String(NU_USB_DEVICE * cb,
                               UINT8 str_index,
                               NU_USB_STRING * string);

STATUS NU_USB_DEVICE_Set_Manf_String (NU_USB_DEVICE * cb,
                                     UINT8 str_index,
                                     NU_USB_STRING *string);

STATUS NU_USB_DEVICE_Set_Product_String (NU_USB_DEVICE * cb,
                                        UINT8 str_index,
                                        NU_USB_STRING *string);

STATUS NU_USB_DEVICE_Set_Serial_Num_String(NU_USB_DEVICE * cb,
                                           UINT8 str_index,
                                           NU_USB_STRING *string);

STATUS NU_USB_DEVICE_Set_Device_Qualifier (NU_USB_DEVICE * cb,
                                 NU_USB_DEV_QUAL_DESC * dev_qualifier);
                                 
STATUS NU_USB_DEVICE_Set_bcdUSB (NU_USB_DEVICE * cb,
                                 UINT16 bcdUSB);

STATUS NU_USB_DEVICE_Set_bMaxPacketSize0 (NU_USB_DEVICE * cb,
                                          UINT8 bMaxPacketSize0);

STATUS NU_USB_DEVICE_Set_bDeviceClass (NU_USB_DEVICE    *cb,
                                       UINT8            bDeviceClass);

STATUS NU_USB_DEVICE_Get_Current_Requirement (NU_USB_DEVICE *cb,
                                              UINT8 cfg_num,
                                              UINT32 *req_current);

/* BOS descriptor is introduced in USB 3.0 specs. */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS NU_USB_DEVICE_Get_BOS        (NU_USB_DEVICE  *cb,
                         NU_USB_BOS  **bos_out);
                                    
STATUS NU_USB_DEVICE_Get_BOS_Desc     (NU_USB_DEVICE     *cb,
                         NU_USB_BOS_DESC     **bos_desc_out);
                                    
STATUS NU_USB_DEVICE_Get_USB2Ext_Desc (NU_USB_DEVICE *cb,
                         NU_USB_DEVCAP_USB2EXT_DESC  **usb2ext_desc_out);

STATUS NU_USB_DEVICE_Get_SuprSpd_Desc(NU_USB_DEVICE   *cb,
                         NU_USB_DEVCAP_SUPERSPEED_DESC  **ss_desc_out);

STATUS NU_USB_DEVICE_Get_CntnrID_Desc(NU_USB_DEVICE   *cb,
                         NU_USB_DEVCAP_CONTAINERID_DESC    **cid_desc_out);

STATUS NU_USB_DEVICE_Set_Link_State ( NU_USB_DEVICE   *device,
                                      UINT8           link_state );

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

/* ==================================================================== */

#endif /* _NU_USB_DEVICE_EXT_H_ */

/* ======================  End Of File  =============================== */

