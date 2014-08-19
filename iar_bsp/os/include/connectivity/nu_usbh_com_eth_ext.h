/**************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*       nu_usbh_com_eth_ext.h
*
*
* COMPONENT
*       Nucleus USB host software : Communication user driver
*
* DESCRIPTION
*       This file contains definitions for external Interfaces exposed by
*       Communication ethernet user driver.
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       None
*
* DEPENDENCIES
*       nu_usbh_com_eth_imp.h         Internal Definitions.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_COM_ETH_EXT_H_
#define _NU_USBH_COM_ETH_EXT_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ===================================================================== */

/* =========================== Include Files =========================== */
/* Making internals visible */
#include "connectivity/nu_usbh_com_eth_imp.h"

/* ========================= Functions Prototypes ====================== */

/* Add the prototypes for extra services provided by COM_ETH here */
STATUS NU_USBH_COM_ETH_Create (
       NU_USBH_COM_ETH*       pcb_user_drvr,
       CHAR*                  p_name,
       NU_MEMORY_POOL*        p_memory_pool,
       NU_USBH_COM_ETH_HANDL* p_handlers);
STATUS _NU_USBH_COM_ETH_Delete (
       VOID*                pcb_user_drvr);

STATUS _NU_USBH_COM_ETH_Connect_Handler(
       NU_USB_USER*         pcb_user,
       NU_USB_DRVR*         pcb_drvr,
       VOID*                pcb_curr_device,
       VOID*                information);

STATUS _NU_USBH_COM_ETH_Disconnect_Handler(
       NU_USB_USER*         pcb_user_drvr,
       NU_USB_DRVR*         pcb_com_drvr,
       VOID*                pcb_curr_device);

STATUS _NU_USBH_COM_ETH_Intr_Handler(
       VOID*                pcb_curr_device,
       NU_USBH_COM_XBLOCK*  pcb_xblock);

STATUS nu_os_conn_usb_host_comm_eth_init(CHAR*   path,
                                         INT    startstop);

STATUS NU_USBH_COM_ETH_GetHandle(VOID  **handle);

VOID NU_USBH_COM_ETH_Reg_Hndlr(NU_USBH_COM_ETH*       pcb_user_drvr,
                         NU_USBH_COM_ETH_HANDL* p_handlers);

STATUS    NU_USBH_ETH_DM_Open (VOID* dev_handle);

STATUS    NU_USBH_ETH_DM_Close(VOID* dev_handle);

STATUS NU_USBH_ETH_DM_Read( VOID*    dev_handle,
                            VOID*    buffer,
                            UINT32   numbyte,
                            OFFSET_T byte_offset,
                            UINT32*  bytes_read_ptr);
                            
STATUS NU_USBH_ETH_DM_Write(VOID*       dev_handle,
                            const VOID* buffer,
                            UINT32      numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*     bytes_written_ptr);
                            
STATUS    NU_USBH_ETH_DM_IOCTL(VOID*     dev_handle,
                               INT       ioctl_num,
                               VOID*     ioctl_data,
                               INT       ioctl_data_len);                                                        


#endif /* _NU_USBH_COM_ETH_EXT_H_ */
