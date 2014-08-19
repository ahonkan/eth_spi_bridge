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
 *      nu_usb_sys_ext.h
 * 
 * COMPONENT 
 *      Nucleus USB Software
 * 
 * DESCRIPTION 
 *
 *      This file describes differnt control blocks, macros and other 
 *      related data used to interface USB connectivity stack with other
 *      system components like device and power manager.
 * 
 * DATA STRUCTURES 
 *      NU_USB_SYS_SESSION     Control block for a single session opened by
 *                             the mw/application for a specific mode.
 *      NU_USB_SYS_DEVICE      Control block for each device registered with the DM.
 *      NU_USB_SYS_SERVER      Control block of system module providing services 
 *                             to interact with the DM.
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      nu_usb.h
 *      dev_extr.h
 *      power_core.h
 * 
 *************************************************************************/

/* ===================================================================  */
#ifndef _NU_USB_SYS_EXT_H_
#define _NU_USB_SYS_EXT_H_
/* ===================================================================  */
#include "kernel/dev_mgr.h"
#include "services/power_core.h"
#include "services/reg_api.h"


#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* =================== Definitions Macros =============================  */

/* These are compnent IDs defined for differnt USB components like class
 * and user drivers.
 */

#define NU_USBCOMP_FUNCTION                  0x00
#define NU_USBCOMP_HOST                      0x10
#define NU_USBCOMPF_STORAGE                  0x01
#define NU_USBCOMPF_RNDIS                    0x02
#define NU_USBCOMPF_CDC                      0x03
#define NU_USBCOMPF_MODEM                    0x04
#define NU_USBCOMPF_NET                      0x05
#define NU_USBCOMPF_KBD                      0x06
#define NU_USBCOMPF_MSE                      0x07
#define NU_USBCOMPF_DFU                      0x08

#define NU_USBCOMPH_STORAGE                  0x11
#define NU_USBCOMPH_RNDIS                    0x12
#define NU_USBCOMPH_CDC                      0x13
#define NU_USBCOMPH_MODEM                    0x14
#define NU_USBCOMPH_NET                      0x15
#define NU_USBCOMPH_FILE                     0x16
#define NU_USBCOMPH_KBD                      0x17
#define NU_USBCOMPH_MSE                      0x18
#define NU_USBCOMPH_AUDIO                    0x19


/* Session States of a particular session. */
#define USB_SYS_SESS_STATE_AVLB              0x30
#define USB_SYS_SESS_STATE_OPEN              0x31
#define USB_SYS_STATE_DISCONNECTED           0x32
#define USB_SYS_STATE_CONNECTED              0x33
#define USB_SYS_SESS_STATE_CLOSED            0x34
#define USB_SYS_SESS_STATE_IDLE              0x35
#define USB_SYS_SESS_STATE_SUSPENDED         0x36
#define USB_SYS_SESS_STATE_ACTIVE            0x37

/* Operating modes of a session, must be exclusive */
#define USB_SYS_SESS_MODE_POWER              0x001
#define USB_SYS_SESS_MODE_USB                0x002
#define USB_SYS_SESS_MODE_NETWORKING         0x004
#define USB_SYS_SESS_MODE_STORAGE            0x008
#define USB_SYS_SESS_MODE_ETHERNET           0x010
#define USB_SYS_SESS_MODE_FILE               0x020
#define USB_SYS_SESS_MODE_MODEM              0x040
#define USB_SYS_SESS_MODE_CDC                0x080
#define USB_SYS_SESS_MODE_KBD                0x100
#define USB_SYS_SESS_MODE_MSE                0x200
#define USB_SYS_SESS_MODE_AUDIO              0x400
#define USB_SYS_SESS_MODE_DFU                0x800


#define USB_POWER_STATE_ON                   0x01
#define USB_POWER_STATE_OFF                  0x00

#define USB_TOTAL_POWER_STATES               0x02



/* Maximum number of sessions/device supported by USB system. */
#define USB_SYS_MAX_SESSIONS                 2

/* IOCTL bases for different functions. */
/* IOCTL bases for different functions. */
#define USB_POWER_IOCTL_BASE    (DV_IOCTL0 + 100)

#ifdef CFG_NU_OS_NET_ENABLE
#if defined (CFG_NU_OS_DRVR_USB_HOST_NET_IF_ENABLE)  || defined (CFG_NU_OS_DRVR_USB_FUNC_NET_IF_ENABLE)
#define USB_ETHERNET_IOCTL_BASE (DV_IOCTL0 + 1)
#define USB_NET_IOCTL_BASE      (USB_ETHERNET_IOCTL_BASE + TOTAL_ETHERNET_IOCTLS )
#endif
#endif

#ifdef CFG_NU_OS_STOR_FILE_VFS_ENABLE
#ifdef CFG_NU_OS_DRVR_USB_HOST_FILE_IF_ENABLE
#define USB_FILE_IOCTL_BASE     (DV_IOCTL0 + 1)
#endif
#endif

#define USB_MODEM_IOCTL_BASE    (DV_IOCTL0 + 1)
#define USB_STORE_IOCTL_BASE    (DV_IOCTL0 + 1)
#define USB_MEDIA_IOCTL_BASE    (DV_IOCTL0 + 1)
#define USB_CDC_IOCTL_BASE      (DV_IOCTL0 + 1)
#define USB_KBD_IOCTL_BASE      (DV_IOCTL0 + 1)
#define USB_MSE_IOCTL_BASE      (DV_IOCTL0 + 1)
#define USB_AUDIO_IOCTL_BASE    (DV_IOCTL0 + 1)
#define USB_DFU_IOCTL_BASE      (DV_IOCTL0 + 1)
#define USB_RNDIS_IOCTL_BASE    (DV_IOCTL0 + 1)

/* Standard 128-bit GUID's for USB Drivers*/
#define     USBFUNCTION_LABEL   {0x87,0x8e,0x2f,0xa4,0x6b,0xc9,0x43,0x4b,0x87,0x62,0x0b,0x41,0xd7,0x6f,0x9a,0xc6}
#define     USBDEVICE_LABEL     {0x6e,0x3e,0xc3,0xef,0xa6,0x33,0x4a,0xe9,0x94,0xa4,0xee,0x65,0x0b,0x0e,0x0e,0x28}

#define     USBH_STORE_LABEL    {0xd0,0xd9,0xe5,0x97,0xae,0x01,0x47,0xcc,0xb5,0xb8,0x6e,0x75,0x9a,0xb5,0xcf,0x77}
#define     USBF_STORE_LABEL    {0x4f,0x81,0x6a,0x53,0x3c,0x8c,0x41,0xd2,0xab,0x3b,0xa7,0xd0,0x9e,0xec,0x56,0x06}
#define     USBH_ETHERNET_LABEL {0xb2,0x65,0x6f,0xfc,0xe5,0xe3,0x4b,0xc7,0xbe,0xa2,0xc3,0xa9,0x8b,0xc1,0x77,0x41}
#define     USBH_MODEM_LABEL    {0xae,0xc4,0x94,0x0c,0xf9,0x54,0x40,0xfd,0xbb,0x9e,0xcf,0x7b,0x34,0x9c,0x12,0x24}
#define     USBF_MODEM_LABEL    {0xb4,0xd8,0x4d,0x28,0x9d,0x64,0x48,0x03,0x81,0xad,0x1a,0xf7,0x6f,0x2d,0x01,0x9f}
#define     USBF_ETHERNET_LABEL {0xbf,0x9e,0xb8,0x8e,0x32,0xf1,0x45,0xcc,0xa9,0x61,0x4b,0x9f,0xfe,0xf6,0xd9,0x45}
#define     USBF_RNDIS_LABEL    {0x41,0xe1,0xfd,0x86,0x24,0x58,0x44,0x50,0x80,0xcd,0xe7,0x6c,0x89,0x40,0x56,0xff}
#define     USBF_KEYBOARD_LABEL {0x5f,0x0a,0xdd,0x76,0x49,0x31,0x48,0xb9,0x84,0xec,0xe7,0x16,0x6b,0x58,0x20,0x0f}
#define     USBF_MOUSE_LABEL    {0x18,0xbd,0x54,0xa4,0x4f,0xfb,0x4b,0x71,0xad,0xcb,0x4c,0xad,0x47,0xae,0xfc,0x4f}
#define     USBF_DFU_RTM_LABEL  {0x42,0xd1,0x5f,0x62,0x0f,0xa4,0x4c,0x64,0x8d,0x81,0x6c,0x89,0x9f,0x30,0x59,0x83}
#define     USBF_DFU_STDA_LABEL {0x06,0x33,0x67,0x2d,0xab,0x14,0x49,0x20,0xa9,0x33,0x17,0x0e,0x0d,0x05,0xe8,0xa1}

#define     USBH_FILE_LABEL     {0x61,0xaa,0x79,0x58,0x7e,0x13,0x42,0x34,0xa1,0x9a,0xa3,0x2d,0x75,0xd7,0xb9,0x11}
#define     USBF_FILE_LABEL     {0x26,0xef,0xdb,0xe4,0x5b,0xe1,0x4b,0x90,0xac,0x42,0x6d,0x59,0x05,0x94,0xd9,0xd7}
#define     USBH_NET_LABEL      {0x56,0x14,0x95,0x84,0x4c,0x9a,0x4c,0xca,0x9d,0xdc,0x7d,0x5d,0x9e,0x6e,0x91,0x8e}
#define     USBF_NET_LABEL      {0x87,0xe0,0xaf,0x16,0x58,0xd9,0x47,0xdb,0xb9,0x05,0x92,0x33,0xc9,0xe4,0xaa,0xae}
#define     USB_RS1_LABEL       {0xfe,0x61,0x3e,0xe5,0xbb,0x0b,0x44,0x5b,0xbf,0xc6,0xce,0xdd,0xc2,0x44,0x9c,0xa9}
#define     USB_RS2_LABEL       {0xd1,0x63,0xfc,0x94,0x8f,0x98,0x40,0x08,0x86,0x0d,0x43,0x25,0x25,0xa0,0x02,0xcb}
#define     USB_RS3_LABEL       {0xb5,0x95,0x0b,0xcd,0x51,0x64,0x42,0x24,0x95,0x76,0xdf,0x62,0x74,0xac,0xbc,0x47}
#define     USB_RS4_LABEL       {0xdd,0x63,0x01,0x33,0x8a,0x72,0x4d,0x91,0xb0,0xa4,0x6c,0xaa,0xf1,0xdd,0xb5,0x64}

#define     USBH_KBD_LABEL      {0x7e,0x74,0xb0,0xbc,0xc0,0x4c,0x42,0x2f,0xb6,0xeb,0x92,0xbd,0xec,0xf7,0x52,0x1c}
#define     USBH_MSE_LABEL      {0x8c,0x56,0xe3,0xec,0x5b,0x00,0x4c,0x5c,0xa2,0xb0,0xcd,0x42,0x43,0x2a,0x48,0x6a}
#define     USBH_AUDIO_LABEL    {0x09,0x28,0x85,0x5e,0x60,0xdd,0x45,0xc8,0xbf,0xc8,0x41,0xd0,0x76,0x7d,0x0a,0x76}


/* ====================== Data Structures =============================  */
typedef struct nu_usb_sys_session
{
    VOID*                sesion_handle;      
    UINT32               modes;   
    UINT32               sess_state;
    VOID*                dev_inst;
    UINT32               usr_var;
    VOID*                usr_ptr; 
}NU_USB_SYS_SESSION;


/* This Control Block will be created for each of function class driver and each of connected device
 * in host case. This will be nucleus conatining all the dm and class driver specific links of a 
 * device.
 */
typedef struct nu_usb_sys_device
{
    CS_NODE              dev_link;           /* link list container. */                                            
    DV_DRV_FUNCTIONS     drvr_func_table;    /* Table containing the function pointers into the 
                                                class/user driver. */ 
    DV_DEV_LABEL         dev_labels[DV_MAX_DEV_LABEL_CNT];
    NU_USB_SYS_SESSION   seesions[USB_SYS_MAX_SESSIONS];
    INT                  dev_label_cnt;                                            
    UINT32               usr_var;
    VOID*                usr_ptr;     
    DV_DEV_ID            drvr_id;            /* ID returned from DM at registeration time. */
    UINT16               dev_type;
    UINT8                dummy_bytes[2];  
}NU_USB_SYS_DEVICE;


/* ====================== Data Structures =============================  */
/* This Control Block will be present in the system at zero time and will be initialized by the 
 * stack at initilization time through API call.
 */
typedef struct nu_usb_sys_ser
{
    NU_MEMORY_POOL*      mem_pool;           /* Memory pool pointer for internal use. */
    NU_USB_SYS_DEVICE*   dev_head;           /* Device link list header. */                                            
}NU_USB_SYS_SERVER;

/* ====================== Function Prototypes ========================== */

STATUS NU_USB_SYS_Register_Device(
                           VOID*             dev_handle,
                           UINT8             comp_id);
                           
STATUS NU_USB_SYS_DeRegister_Device(
                           VOID*             dev_handle,
                           UINT8             comp_id);

STATUS NU_USB_SYS_Event_Report(
                           VOID*             dev_handle,
                           UINT8*            comp_id,
                           UINT8             event);

STATUS NU_USB_SYS_Handle_Function_Dev(VOID*              dev_handle,
                                      UINT8              comp_id,
                                      NU_USB_SYS_DEVICE* ptr_curr_dev);

STATUS NU_USB_SYS_Handle_Host_Dev(VOID*              dev_handle,
                                  UINT8              comp_id,
                                  NU_USB_SYS_DEVICE* ptr_curr_dev);

STATUS NU_USB_SYS_Open_Device(VOID*        instance_handle,
                              DV_DEV_LABEL label_list[],
                              INT          label_cnt,
                              VOID**       session_handle_ptr);

STATUS NU_USB_SYS_Close_Device(VOID*  session_handle_ptr);

STATUS NU_USB_SYS_IOCTL(VOID    *session_handle,
                        INT      cmd,
                        VOID    *data,
                        INT      length);

STATUS NU_USB_SYS_Read (VOID *session_handle,
                        VOID *buffer,
                        UINT32 numbyte,
                        OFFSET_T byte_offset,
                        UINT32 *bytes_read_ptr);



STATUS  NU_USB_SYS_Write (VOID *session_handle,
                          const VOID *buffer,
                          UINT32 numbyte,
                          OFFSET_T byte_offset,
                          UINT32 *bytes_written_ptr);

STATUS NU_USB_SYS_Get_Dev_ID(VOID *dev_handle, DV_DEV_ID *dev_id_out);

UINT32 NU_USB_SYS_Get_Oper_Mode(DV_DEV_LABEL     labels_list[],
                                INT              labels_cnt);

/* ===================================================================== */
#endif /* _NU_USB_SYS_EXT_H_ */
/* ====================== end of file ================================== */

