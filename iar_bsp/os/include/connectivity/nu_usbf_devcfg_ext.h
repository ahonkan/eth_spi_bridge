/**************************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*       nu_usbf_devcfg_ext.h
*
*
* COMPONENT
*
*       Nucleus USB Device Software
*
* DESCRIPTION
*
*       This is the glue layer for USB device configuration. APIs in this
*       file help class driver to dynamically register/unregister
*       descriptors.
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
*       None.
*
**************************************************************************/

/* ===================================================================== */

#ifndef _NU_USBF_DEVCFG_EXT_H
#define _NU_USBF_DEVCFG_EXT_H

/* ==============  USB Include Files =================================== */


/* ====================  Data Types ==================================== */

/* Number of functions allowed in a composite device. */
#define USBF_MAX_FUNCTIONS      (NU_USB_MAX_CLASS_DRIVERS * NU_USB_MAX_USERS)

/* Maximum configurations. */
#define USBF_MAX_CONFIGURATIONS NU_USB_MAX_CONFIGURATIONS

/* Maximum string descriptors. */
#define USBF_MAX_STRINGS        NU_USB_MAX_STRINGS

/* Default configuration index. */
#define USBF_DEF_CONFIG_INDEX   0

/* Three speeds Full, High and Super. */
#define USBF_MAX_SPEEDS         3

/* Length of SuperSpeed BOS descriptor. */
#define USBF_DEVCFG_MAX_BOS_DESC_LEN    42

typedef struct _usbf_function
{
    NU_USB_DEVICE *device;
    UINT8   *intf_raw_desc[NU_USB_MAX_SPEEDS];
    UINT16  desc_len[NU_USB_MAX_SPEEDS];
    BOOLEAN is_used;
    BOOLEAN is_enabled;
    UINT8   config_index;
    UINT8   func_index;
    UINT8   pad[2];
}USBF_FUNCTION;

typedef struct _usbf_string
{
    NU_USB_STRING *usb_string;
    BOOLEAN is_used;
    UINT8   index;
    UINT8   pad[2];
}USBF_STRING;

typedef struct _usbf_device_config
{
    NU_USB_DEVICE   *device;
    USBF_FUNCTION   usb_functions[USBF_MAX_CONFIGURATIONS][USBF_MAX_FUNCTIONS];
    USBF_STRING     usb_strings[USBF_MAX_STRINGS];
}USBF_DEVICE_CONFIG;

typedef struct _usbf_epm_ep
{
    BOOLEAN is_used;
    UINT8   config_num;
    UINT8   intf_num;
    UINT8   alt_sttg;
    UINT8   ep_num;
    UINT8   direction;
    UINT8   pad[2];
}USBF_EPM_EP;

typedef struct _usbf_epm_alt_sttg
{
    USBF_EPM_EP ep[NU_USB_MAX_ENDPOINTS];
}USBF_EPM_ALT_STTG;

typedef struct _usbf_epm_intf
{
    USBF_EPM_ALT_STTG alt_sttg[NU_USB_MAX_ALT_SETTINGS];
}USBF_EPM_INTF;

typedef struct _usbf_epm_cfg
{
    USBF_EPM_INTF intf[NU_USB_MAX_INTERFACES];
}USBF_EPM_CFG;

typedef struct _usbf_stack_ep_map
{
    USBF_EPM_CFG cfg[USBF_MAX_SPEEDS][NU_USB_MAX_CONFIGURATIONS];
}USBF_STACK_EP_MAP;

/* ====================  Function Prototypes =========================== */

STATUS USBF_DEVCFG_Device_Initialize(NU_USB_DEVICE*, UINT16, UINT16, CHAR*, CHAR*, CHAR*);
STATUS USBF_DEVCFG_Create_Device(VOID);
STATUS USBF_DEVCFG_Initialize(NU_USB_DEVICE*);
STATUS USBF_DEVCFG_Uninitialize(VOID);
STATUS USBF_DEVCFG_Get_Handle(USBF_DEVICE_CONFIG**);
STATUS USBF_DEVCFG_Get_Device_Handle(NU_USB_DEVICE**);
STATUS USBF_DEVCFG_Add_Function(UINT8, UINT8*, UINT16, UINT8*, UINT16,
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                UINT8*, UINT16,
#endif
                                USBF_FUNCTION**);
STATUS USBF_DEVCFG_Delete_Function(USBF_FUNCTION*);
STATUS USBF_DEVCFG_Enable_Function(USBF_FUNCTION*);
STATUS USBF_DEVCFG_Disable_Function(USBF_FUNCTION*);
STATUS USBF_DEVCFG_Bind_Function(USBF_FUNCTION*);
STATUS USBF_DEVCFG_Unbind_Function(USBF_FUNCTION*);
STATUS USBF_DEVCFG_Create_Config(NU_USB_DEVICE*, UINT8*);
STATUS USBF_DEVCFG_Delete_Config(NU_USB_DEVICE*, UINT8);
STATUS USBF_DEVCFG_Recreate_Config(NU_USB_DEVICE*, UINT8);
STATUS USBF_DEVCFG_Activate_Device(NU_USB_DEVICE*);
STATUS USBF_DEVCFG_Deactivate_Device(NU_USB_DEVICE*);
STATUS USBF_DEVCFG_Add_Lang_String(NU_USB_DEVICE*);
STATUS USBF_DEVCFG_Add_Manuf_String(NU_USB_DEVICE*, CHAR*);
STATUS USBF_DEVCFG_Add_Product_String(NU_USB_DEVICE*, CHAR*);
STATUS USBF_DEVCFG_Add_Serial_String(NU_USB_DEVICE*, CHAR*);
STATUS USBF_DEVCFG_Add_Config_String(UINT8, NU_USB_DEVICE*, CHAR*);
STATUS USBF_DEVCFG_Add_Intf_String(USBF_FUNCTION*, NU_USB_DEVICE*, CHAR*);
STATUS USBF_DEVCFG_Add_String(NU_USB_DEVICE*, CHAR*);
BOOLEAN USBF_DEVCFG_Check_Endpoiint(UINT8, UINT8, UINT8, UINT8, UINT8, UINT8);
STATUS USBF_DEVCFG_Save_Endpoiint(UINT8, UINT8, UINT8, UINT8, UINT8, UINT8);
STATUS USBF_DEVCFG_Delete_Endpoiint(UINT8, UINT8, UINT8, UINT8, UINT8, UINT8);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
STATUS USBF_DEVCFG_Create_BOS(NU_USB_DEVICE*);
STATUS USBF_DEVCFG_Add_BOS_DevCap(NU_USB_DEVICE*, UINT8*, UINT16);
STATUS USBF_DEVCFG_Add_BOS_DevCap_USB2Ext(NU_USB_DEVICE*);
STATUS USBF_DEVCFG_Add_BOS_DevCap_SS(NU_USB_DEVICE*);
#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

/* ===================================================================== */

#endif                                      /* _NU_USBF_DEVCFG_EXT_H     */

/* ======================  End Of File  ================================ */
