/**************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation
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
*
*       nu_usbh_ms_user_imp.h
*
*
* COMPONENT
*
*        Nucleus USB Host Mass Storage class User Driver.
*
* DESCRIPTION
*
*       This file contains data structure definitions and constants for
*       the USBH MS User driver component.
*
* DATA STRUCTURES
*
*       nu_usbh_ms_user                     User control block.
*       nu_usbh_ms_app_callbacks            Application call back structure.
*       nu_usbh_ms_user_dispatch            Command set structure.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbh_ms_user_dat.h               SCSI,UFI and other wrappers
*                                           dispatch table definitions
*
**************************************************************************/

/* ===================================================================== */

#ifndef     _NU_USBH_MS_USER_IMP_H_
#define     _NU_USBH_MS_USER_IMP_H_

/* ===================================================================== */

#ifndef     _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/*The following macros tell the mass storage user driver about the command set
which need to be supported*/
#define INCLUDE_SCSI            CFG_NU_OS_CONN_USB_HOST_MS_CLASS_ENABLE_SUBCLASS_SCSI
#define INCLUDE_UFI             CFG_NU_OS_CONN_USB_HOST_MS_CLASS_ENABLE_SUBCLASS_UFI
#define INCLUDE_SFF8020I        CFG_NU_OS_CONN_USB_HOST_MS_CLASS_ENABLE_SUBCLASS_SFF8020
#define INCLUDE_SFF8070I        CFG_NU_OS_CONN_USB_HOST_MS_CLASS_ENABLE_SUBCLASS_SFF8070
/* ====================Device manager related things==================== */
#define USBH_MEDIA_INQUIRY                  (USB_STORE_IOCTL_BASE + 0)
#define USBH_MEDIA_TEST_UNIT_READY          (USB_STORE_IOCTL_BASE + 1)
#define USBH_MEDIA_REQUEST_SENSE            (USB_STORE_IOCTL_BASE + 2)
#define USBH_MEDIA_READ_CAPACITY            (USB_STORE_IOCTL_BASE + 3)
#define USBH_MEDIA_REQUEST                  (USB_STORE_IOCTL_BASE + 4)
#define USBH_MEDIA_SET_DATA_BUFF_CACHABLE   (USB_STORE_IOCTL_BASE + 5)

/* ====================  Data Structures  ============================== */
/* Control block. */
typedef struct nu_usbh_ms_user_dispatch
{

    /* Request routine. */
    STATUS  (*request)(VOID *handle, VOID *command, UINT32 cmd_len,
                       VOID *buffer, UINT32 buf_len,
                       UINT8 direction);

    /* Inquiry routine. */
    STATUS  (*inquiry)(VOID *handle, UINT8 *command, VOID *buffer,
                       UINT8 buf_len);

    /* Unit read routine. */
    STATUS  (*unit_ready)(VOID *handle, UINT8 *command);

    /* Read capacity routine. */
    STATUS  (*read_capacity)(VOID *handle, UINT8 *command,
                             UINT8 *buffer);

    /* Request sense routine. */
    STATUS  (*request_sense)(VOID *handle, UINT8 *command, VOID *buffer,
                             UINT8 buf_len);

    /* Read routine. */
    STATUS  (*read)(VOID *handle, UINT8 *command, UINT32 lba_addr,
                    UINT32 count, VOID *buffer,
                    UINT32 buf_len);

    /* Write routine. */
    STATUS  (*write)(VOID *handle, UINT8 *command, UINT32 lba_addr,
                     UINT32 count, VOID *buffer,
                     UINT32 buf_len);

}NU_USBH_MS_USER_DISPATCH;

typedef struct nu_usbh_ms_app_callbacks
{

    STATUS (*app_connect) (VOID *handle,
                           UINT8 subclass);
    STATUS (*app_disconnect) (VOID *handle,
                           UINT8 subclass);

}   NU_USBH_MS_APP_CALLBACKS;

typedef struct nu_usbh_ms_user
{
    NU_USBH_USER     cb;
    NU_USBH_MS_USER_DISPATCH ** dispatch_list;
    NU_USBH_MS_APP_CALLBACKS * app_callbacks;

}   NU_USBH_MS_USER;

/* ====================  Function prototypes =========================== */

STATUS _NU_USBH_MS_USER_Connect(NU_USB_USER *, NU_USB_DRVR *, VOID *);
STATUS _NU_USBH_MS_USER_Disconnect(NU_USB_USER *, NU_USB_DRVR *, VOID *);
STATUS UHMSU_init_Subclass_index(UINT8 subclass);
STATUS UHMSU_Get_Subclass_index(UINT8 subclass,UINT8 *index_return);
VOID UHMSU_Clear_Subclass_Index_List(VOID);
/* ====================  USB Include Files  ============================ */
#if INCLUDE_SCSI    /* SCSI */
#include "connectivity/nu_usbh_ms_scsi_imp.h"
#endif
#if INCLUDE_UFI   /* UFI */
#include "connectivity/nu_usbh_ms_ufi_imp.h"
#endif
#if INCLUDE_SFF8020I    /* SFF-8020 */
#include "connectivity/nu_usbh_ms_8020_imp.h"
#endif
#if INCLUDE_SFF8070I    /* SFF-8070i */
#include "connectivity/nu_usbh_ms_8070_imp.h"
#endif

#include "connectivity/nu_usbh_ms_user_dat.h"

/* ===================================================================== */

#endif      /* _NU_USBH_MS_USER_IMP_H_ */

/* ====================  End Of File  ================================== */
