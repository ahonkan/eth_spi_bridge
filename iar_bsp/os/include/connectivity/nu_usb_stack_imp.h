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
* FILE NAME 
*
*        nu_usb_stack_imp.h
*
* COMPONENT
*
*        Nucleus USB Software
*
* DESCRIPTION
*
*        This file contains the internal function names and data structures
*        for the common stack component.
*
*
* DATA STRUCTURES
*       usb_device_descriptor       - device descriptor as defined in USB 2.0 
*                                   specification table 9.8
*       usb_device_qualifier        -device qualifier (table 9.9)
*       usb_string_descriptor       -string descriptor (table 9.15)
*       usb_endpoint_descriptor     -endpoint descriptor (table 9.13)
*       usb_interface_descriptor    -interface descriptor (9.12)
*       usb_config_descriptor       -configuration descriptor (9.10)
*       _nu_usb_setup_pkt           -Setup Packet for control transactions
*       usb_hdr                     -USB requests header
*       usbh_drvr_list              -List of drivers.
*       _nu_usb_stack               -Stack Control Block
*       _usb_stack_dispatch         -Dispatch table for stack.
*
* FUNCTIONS
*
*       None 
*
* DEPENDENCIES 
*       nu_usb_stack_cfg.h      Base Stack's Configuration Definitions.
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_STACK_IMP_H
#define _NU_USB_STACK_IMP_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */

/* ==============  Forward Declarations ============================  */
typedef struct nu_usb_drvr      NU_USB_DRVR;
typedef struct _nu_usb_device   NU_USB_DEVICE;
typedef struct _nu_usb_irp      NU_USB_IRP;
typedef struct _nu_usb_iso_irp  NU_USB_ISO_IRP;
typedef struct usb_edp_info     NU_USB_EDP_INFO;
typedef struct nu_usb_alt_settg NU_USB_ALT_SETTG;
typedef struct nu_usb_intf      NU_USB_INTF;
typedef struct nu_usb_iad       NU_USB_IAD;
typedef struct nu_usb_cfg       NU_USB_CFG;
typedef struct nu_usb_pipe      NU_USB_PIPE;
typedef struct nu_usb_user      NU_USB_USER;
typedef struct nu_usb_hw        NU_USB_HW;

/* Callback function type for reporting OTG failures. */
typedef VOID (*NU_USB_STACK_OTG_STATUS_REPORT)(STATUS status_in);

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb_stack_cfg.h"

/* =====================  Global data ================================  */

/* =====================  #defines ===================================  */

/* USB Speed definitions    */
#define USB_SPEED_UNKNOWN       0
#define USB_SPEED_LOW           1
#define USB_SPEED_FULL          2
#define USB_SPEED_HIGH          3
#define USB_SPEED_SUPER         4

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#define NU_USB_MAX_SPEEDS		5
#else
#define NU_USB_MAX_SPEEDS		4
#endif

#define USB_DIR_IN              0x80
#define USB_DIR_OUT             0

#define USB_EP_CTRL             0x0
#define USB_EP_ISO              0x1
#define USB_EP_BULK             0x2
#define USB_EP_INTR             0x3

#define USB_MAX_ADDRESS         127           /* USB 2.0 Spec 9.4.6 */

/*Defines for bcdUSB field in device descriptor and device qualifier descriptor*/
#define BCD_USB_VERSION_11      0x0110
#define BCD_USB_VERSION_20      0x0200
#define BCD_USB_VERSION_30      0x0300

/* Standard Requests    */
#define USB_GET_STATUS              (UINT8)0x00
#define USB_CLEAR_FEATURE           (UINT8)0x01
#define USB_SET_FEATURE             (UINT8)0x03
#define USB_SET_ADDRESS             (UINT8)0x05
#define USB_GET_DESCRIPTOR          (UINT8)0x06
#define USB_SET_DESCRIPTOR          (UINT8)0x07
#define USB_GET_CONFIGURATION       (UINT8)0x08
#define USB_SET_CONFIGURATION       (UINT8)0x09
#define USB_GET_INTERFACE           (UINT8)0x0A
#define USB_SET_INTERFACE           (UINT8)0x0B
#define USB_SYNC_FRAME              (UINT8)0x0C

/* For MTP get functional desciptor request. */
#define MTP_GET_FUNCTIONAL_DESC     (UINT8)0x6F

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
/* Additional USB 3.0 requets. */
#define SET_SEL                     (UINT8)0x30
#define SET_ISOCH_DELAY             (UINT8)0x31
#endif
/* Request recipients   */
#define USB_RECIPIENT_DEVICE        0x00
#define USB_RECIPIENT_INTERFACE     0x01
#define USB_RECIPIENT_ENDPOINT      0x02
#define USB_RECIPIENT_OTHER         0x03

/* Feature list         */
#define USB_FEATURE_ENDPOINT_HALT       0x00
#define USB_FEATURE_REMOTE_WAKEUP   	0x01
#define USB_FEATURE_ENDPOINT_HALT   	0x00
#define USB_FEATURE_TEST_MODE       	0x02
#define USB_FEATURE_B_HNP_ENABLE        0x03
#define USB_FEATURE_A_HNP_SUPPORT       0x04
#define USB_FEATURE_A_ALT_HNP_SUPPORT   0x05

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#define USB_FEATURE_FUNCTION_SUSPEND    0x00
#define USB_FEATURE_U1_ENABLE			0x30
#define USB_FEATURE_U2_ENABLE			0x31
#define USB_FEATURE_LTM_ENABLE          0x32
#endif

/* Bit-Masks for Get_Status(Device) response. */
#define USB_STS_SELF_POWERED_BIT                (1 << 0)
#define USB_STS_REMOTE_WAKEUP_BIT               (1 << 1)
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#define USB_STS_U1_ENABLE_BIT                   (1 << 2)
#define USB_STS_U2_ENABLE_BIT                   (1 << 3)
#define USB_STS_LTM_ENABLE_BIT                  (1 << 4)
#endif

/* Bit-Masks for Get_Status(Interface) response. */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#define USB_STS_FUNC_REMOTE_WAKEUP_BIT          (1 << 0)
#define USB_STS_FUNC_REMOTE_WAKEUP_ENABLE_BIT   (1 << 1)
#endif

/* USB Function Suspend bit-masks. */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#define FUNCTION_SUSPEND_BIT_POS        (1 << 0)
#define FUNCTION_REMOTE_WAKEUP_BIT_POS  (1 << 1)
#endif

/* Device States        */
#define USB_STATE_DETACHED          0x00
#define USB_STATE_ATTACHED          0x01
#define USB_STATE_POWERED           0x02
#define USB_STATE_DEFAULT           0x03
#define USB_STATE_ADDRESSED         0x04
#define USB_STATE_CONFIGURED        0x05
#define USB_STATE_SUSPENDED         0x10

/* table 9.5 of USB 2.0 : Descriptor types */
#define USB_DT_DEVICE               0x01
#define USB_DT_CONFIG               0x02
#define USB_DT_STRING               0x03
#define USB_DT_INTERFACE            0x04
#define USB_DT_ENDPOINT             0x05
#define USB_DT_DEVICE_QUALIFIER     0x06
#define USB_DT_OTHER_SPEED_CONFIG   0x07
#define USB_DT_INTERFACE_POWER      0x08
#define USB_DT_OTG                  0x09
#define USB_DT_DEBUG                0x0A
#define USB_DT_INTERFACE_ASSOC      0x0B
#define USB_DT_CLASSSPC      		0x24

/* Table 9.5 of USB 3.0 specifications : Descriptor types */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#define USB_DT_BOS                  0x0F
#define USB_DT_DEVCAP               0x10
#define USB_DT_SSEPCOMPANION        0x30
#endif

/* table 9.6 of USB 2.0 : Features  */
#define USB_EP_HALT                 0
#define USB_REMOTE_WAKEUP           1
#define USB_TEST_MODE               2

/* HSET test modes */
#define USB_TEST_J                  1 
#define USB_TEST_K                  2
#define USB_TEST_SE0_NAK            3
#define USB_TEST_PACKET             4
#define USB_TEST_FORCE_ENABLE       5
#define USB_TEST_CTRL_DELAY         6

/* Language IDs for String descriptors */
#define USB_LANGID_ENGLISH          0x0409

#define USB_CONFIG_DESCRPTR_SIZE    9
#define USB_IAD_DESCRPTR_SIZE       8

#define USB_ROOT_HUB                1   /* default address */

/* Class Codes */
#define USB_HUB_CLASS_CODE          9
#define USB_MS_CLASS_CODE           8
#define USB_HID_CLASS_CODE          3
#define USB_AUDIO_CLASS_CODE        1

#ifndef NU_NULL
#define NU_NULL                     0
#endif

/* Descriptor Lookup Query Flags, continuation of above flags */
#define USB_MATCH_ONLY_ACTIVE_ALT_STTG  0x100
#define USB_SEARCH_ALL_INTERFACES       0x200
#define USB_MATCH_EP_ADDRESS            0x400
#define USB_MATCH_EP_TYPE               0x800
#define USB_MATCH_EP_DIRECTION          0x1000

#define USB_REQ_RECP_DEV    0
#define USB_REQ_RECP_INTF   1
#define USB_REQ_RECP_EP     2
#define USB_REQ_STD         0
#define USB_REQ_CLASS       0x20
#define USB_REQ_VEND        0x40
#define USB_REQ_OUT         0
#define USB_REQ_IN          0x80

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
/* Table 9-11 of USB 3.0 specifications : Device Capability Type Codes */
#define USB_DCT_WUSB        1
#define USB_DCT_USB2EXT     2
#define USB_DCT_USBSS       3
#define USB_DCT_CONTID      4

/* Table 9-14 of USB 3.0 specifications : ContainerID's SIZE */
#define CONTAINER_ID_SIZE   16

/* Table 9-11 of USB 3.0 specifications : Device Capability related 
   miscellaneous constants */
#define BOS_DESC_LENGTH     5
#define DEVCAP_USB2_EXT_LEN	7
#define DEVCAP_SS_LEN		10

#define MIN_DEV_CAP         1
#define MAX_DEV_CAP         3

/* Default value of the control endpoint maxp for Super Speed devices */
#define USB_SS_MAXP_ENDP0_DEF_VALUE        9

/* Link state definitions according to USB 3.0 specifications. */
#define USB_LINK_STATE_U0	0
#define USB_LINK_STATE_U1	1
#define USB_LINK_STATE_U2	2
#define USB_LINK_STATE_U3	3
#endif

/* ====================  Data Types ==================================  */

/* table  9.8 of USB 2.0 */
typedef struct usb_device_descriptor
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT16 bcdUSB;
    UINT8 bDeviceClass;
    UINT8 bDeviceSubClass;
    UINT8 bDeviceProtocol;
    UINT8 bMaxPacketSize0;
    UINT16 idVendor;
    UINT16 idProduct;
    UINT16 bcdDevice;
    UINT8 iManufacturer;
    UINT8 iProduct;
    UINT8 iSerialNumber;
    UINT8 bNumConfigurations;
}
NU_USB_DEVICE_DESC;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
/* Table 9-9 of USB 3.0 specifications : BOS Descriptor */
typedef struct usb_bos_descriptor
{
    UINT8 bLength;
    UINT8 bDescriptor;
    UINT16 wTotalLength;
    UINT8 bNumDeviceCaps;
}
NU_USB_BOS_DESC;

/* Device capability Descriptor Header */
typedef struct usb_device_capability_header
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bDevCapabilityType;
}
NU_USB_DEVCAP_HDR;

/* Table 9-10 of USB 3.0 specifications : Device Capability Descriptor */
typedef struct usb_devcap_usb2ext_desc
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bDeviceCapabilityType;
    UINT32 bmAttributes;
}
NU_USB_DEVCAP_USB2EXT_DESC;

/* Table 9-10 of USB 3.0 specifications : Device Capability Descriptor */
typedef struct usb_devcap_superspeed_desc
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bDeviceCapabilityType;
    UINT8 bmAttributes;
    UINT16 wSpeedsSupported;
    UINT8 bFunctionalitySupported;
    UINT8 bU1DevExitLat;
    UINT16 wU2DevExitLat;
}
NU_USB_DEVCAP_SUPERSPEED_DESC;

/* Table 9-10 of USB 3.0 specifications : Device Capability Descriptor */
typedef struct usb_devcap_containerid_desc
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bDeviceCapabilityType;
    UINT8 bReserved;
    UINT8 ContainerID[CONTAINER_ID_SIZE];
}
NU_USB_DEVCAP_CONTAINERID_DESC;

/* This structure is filled up after calling the function
   USB_Parse_BOS_Descriptor, and after a successful call to the said
   function, the pointers point to valid descriptors present in the
   raw bos descriptors. */
typedef struct usb_bos
{
    NU_USB_BOS_DESC                    *bos_desc;
    NU_USB_DEVCAP_USB2EXT_DESC         *devcap_usb2ext_desc;
    NU_USB_DEVCAP_SUPERSPEED_DESC      *devcap_ss_desc;
    NU_USB_DEVCAP_CONTAINERID_DESC     *devcap_cid_desc; 
}
NU_USB_BOS;

/* Table 9-20 of USB 3.0 specifications :
   SuperSpeed Endpoint Companion Descriptor. */
typedef struct nu_usb_ssepcomanion_desc
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bMaxBurst;
    UINT8 bmAttributes;
    UINT16 wBytesPerInterval;
}NU_USB_SSEPCOMPANION_DESC;
#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

/* table  9.9 of USB 2.0 */
typedef struct usb_device_qualifier
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT16 bcdUSB;
    UINT8 bDeviceClass;
    UINT8 bDeviceSubClass;
    UINT8 bDeviceProtocol;
    UINT8 bMaxPacketSize0;
    UINT8 bNumConfigurations;
    UINT8 bReserved;
}
NU_USB_DEV_QUAL_DESC;

/* table  9.13 of USB 2.0 */
typedef struct usb_endpoint_descriptor
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bEndpointAddress;
    UINT8 bmAttributes;

    /* To avoid address error - accessing UINT16 from Odd address,
     * split them in to 2 UINT8s while accessing wMaxPacketSize
     */
    UINT8 wMaxPacketSize0;
    UINT8 wMaxPacketSize1;
    UINT8 bInterval;
    UINT16 extention;
}
NU_USB_ENDP_DESC;

/* table  9.12 of USB 2.0 */
typedef struct usb_interface_descriptor
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bInterfaceNumber;
    UINT8 bAlternateSetting;
    UINT8 bNumEndpoints;
    UINT8 bInterfaceClass;
    UINT8 bInterfaceSubClass;
    UINT8 bInterfaceProtocol;
    UINT8 iInterface;
}
NU_USB_INTF_DESC;

/* table  9.10 of USB 2.0 */
typedef struct usb_config_descriptor
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT16 wTotalLength;
    UINT8 bNumInterfaces;
    UINT8 bConfigurationValue;
    UINT8 iConfiguration;
    UINT8 bmAttributes;
    UINT8 bMaxPower;
}
NU_USB_CFG_DESC;

typedef struct usb_otg_descriptor
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bmAttributes; 
} 
NU_USB_OTG_DESC;

/* table  9-Z of USB Engineering Change Notice -
 * Interface Association Descriptors
 */
typedef struct usb_iad_descriptor
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bFirstInterface;
    UINT8 bInterfaceCount;
    UINT8 bFunctionClass;
    UINT8 bFunctionSubClass;
    UINT8 bFunctionProtocol;
    UINT8 iFunction;
}
NU_USB_IAD_DESC;

/* The following are the internal arrangement of these descriptors  */
typedef struct _nu_usb_setup_pkt
{
    UINT8 bmRequestType;
    UINT8 bRequest;
    UINT16 wValue;
    UINT16 wIndex;
    UINT16 wLength;
}
NU_USB_SETUP_PKT;

typedef struct usb_hdr
{
    UINT8 bLength;
    UINT8 bDescriptorType;
}
NU_USB_HDR;

typedef struct usbh_drvr_list
{
    CS_NODE list_node;
    NU_USB_DRVR *driver;

}
USB_DRVR_LIST;

typedef struct _nu_usb_string
{
    UINT16  str_index;
    UINT16  wLangId;
    CHAR    string[NU_USB_MAX_STRING_LEN];
}
NU_USB_STRING;

typedef struct _nu_usb_string_desc
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    CHAR *bString;
}
NU_USB_STRING_DESC;

typedef struct _nu_usb_stack
{
    NU_USB usb;

    /* Number of currently registered class drivers     */
    UINT32 num_class_drivers;

    /* Head to the list of class drivers registered with the stack */
    USB_DRVR_LIST *class_driver_list_head;

    /* List entries of class drivers registered with the stack */
    USB_DRVR_LIST class_drivers[NU_USB_MAX_CLASS_DRIVERS];

    /* Function for reporting OTG failures. */
    NU_USB_STACK_OTG_STATUS_REPORT otg_status_func;
}
NU_USB_STACK;

typedef struct _nu_usb_stack_dispatch
{
    NU_USB_DISPATCH dispatch;
    STATUS (*Add_Controller) (NU_USB_STACK * cb,
                              NU_USB_HW * controller);

    STATUS (*Remove_Controller) (NU_USB_STACK * cb,
                                 NU_USB_HW * controller);

    STATUS (*Register_Class_Driver) (NU_USB_STACK * cb,
                                     NU_USB_DRVR * driver);

    STATUS (*Deregister_Class_Driver) (NU_USB_STACK * cb,
                                       NU_USB_DRVR * driver);

    STATUS (*Stall_Endpoint) (NU_USB_STACK * cb,
                              NU_USB_PIPE * pipe);

    STATUS (*Unstall_Endpoint) (NU_USB_STACK * cb,
                                NU_USB_PIPE * pipe);

    STATUS (*Is_Endpoint_Stalled) (NU_USB_STACK * cb,
                                   NU_USB_PIPE * pipe,
                                   DATA_ELEMENT * status);

    STATUS (*Submit_IRP) (NU_USB_STACK * cb,
                          NU_USB_IRP * irp,
                          NU_USB_PIPE * pipe);
	
	STATUS (*Flush_Pipe) (NU_USB_STACK * cb,
                          NU_USB_PIPE * pipe);

    STATUS (*Set_Configuration) (NU_USB_STACK * cb,
                                 NU_USB_DEVICE * device,
                                 UINT8 cnfgno);

    STATUS (*Set_Interface) (NU_USB_STACK * cb,
                             NU_USB_DEVICE * device,
                             UINT8 interface_index,
                             UINT8 alt_setting_index);

    STATUS (*Get_Configuration) (NU_USB_STACK * cb,
                                 NU_USB_DEVICE * device,
                                 UINT8 *cnfgno);

    STATUS (*Get_Interface) (NU_USB_STACK * cb,
                             NU_USB_DEVICE * device,
                             UINT8 intf_num,
                             UINT8 *alt_setting);

    STATUS (*Get_Endpoint_Status) (NU_USB_STACK * cb,
                                   NU_USB_PIPE * pipe,
                                   UINT16 *status);

    STATUS (*Get_Device_Status) (NU_USB_STACK * cb,
                                 NU_USB_DEVICE * device,
                                 UINT16 *status_out);

    STATUS (*Set_Device_Status) (NU_USB_STACK * cb,
                                 NU_USB_DEVICE * device,
                                 UINT16 status);

    STATUS (*Get_Interface_Status) (NU_USB_STACK * cb,
                                    NU_USB_DEVICE * device,
                                    UINT8 interface_index,
                                    UINT16 *status);

    STATUS (*Lock) (NU_USB_STACK * cb);

    STATUS (*Unlock) (NU_USB_STACK * cb);

    BOOLEAN (*Is_Valid_Device) (NU_USB_STACK * cb,
                                NU_USB_DEVICE * device);

    STATUS (*Start_Session) (NU_USB_STACK * cb,
                             NU_USB_HW * hw, UINT8 port_id,
                             UINT16 delay);

    STATUS (*End_Session) (NU_USB_STACK * cb,
                           NU_USB_HW * hw,
                           UINT8 port_id);
	
	STATUS (*Cancel_IRP) (NU_USB_STACK * cb,
                          NU_USB_PIPE * pipe);						 

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    STATUS (*Function_Suspend) ( NU_USB_STACK *cb, NU_USB_INTF *intf,
                                BOOLEAN func_suspend, BOOLEAN rmt_wakeup);
#endif
}
NU_USB_STACK_DISPATCH;

/* ====================  Function Prototypes ========================== */
STATUS _NU_USB_STACK_Register_Drvr (NU_USB_STACK * stack,
                                    NU_USB_DRVR * class_driver);

STATUS _NU_USB_STACK_Deregister_Drvr (NU_USB_STACK * stack,
                                      NU_USB_DRVR * class_driver);

STATUS _NU_USB_STACK_Get_Config (NU_USB_STACK * stack,
                                 NU_USB_DEVICE * device,
                                 UINT8 *cnfgno_out);

STATUS _NU_USB_STACK_Get_Intf (NU_USB_STACK * stack,
                               NU_USB_DEVICE * device,
                               UINT8 interface_index,
                               UINT8 *alt_setting_index_out);

STATUS _NU_USB_STACK_Get_Intf_Status (NU_USB_STACK * cb,
                                      NU_USB_DEVICE * device,
                                      UINT8 interface_index,
                                      UINT16 *status);

STATUS USB_Parse_Descriptors (NU_USB_DEVICE * dev,
                              UINT8 *raw_cfg,
                              NU_USB_CFG * cfg,
                              UINT16 size);

STATUS _NU_USB_STACK_Lock (NU_USB_STACK * cb);

STATUS _NU_USB_STACK_Unlock (NU_USB_STACK * cb);

BOOLEAN _NU_USB_STACK_Is_Valid_Device (NU_USB_STACK * cb,
                                       NU_USB_DEVICE * device);

STATUS _NU_USB_STACK_Start_Session (NU_USB_STACK * cb,
                        NU_USB_HW * hw,
                        UINT8 port_id, UINT16 delay);

STATUS _NU_USB_STACK_End_Session (NU_USB_STACK * cb,
                        NU_USB_HW * hw,
                        UINT8 port_id);

STATUS NU_USB_STACK_OTG_Reg_Status (NU_USB_STACK * cb,
                             NU_USB_STACK_OTG_STATUS_REPORT status_func);

STATUS NU_USB_STACK_Report_Failure (NU_USB_STACK * cb,
                             STATUS status_in);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS USB_Parse_BOS_Descriptor     (NU_USB_DEVICE  *device,
                                     UINT8          *raw_bos_desc,
                                     NU_USB_BOS     *bos,
                                     UINT16         size);

#endif
#endif /* _NU_USB_STACK_IMP_H  */

/* ======================  End Of File  =============================== */

