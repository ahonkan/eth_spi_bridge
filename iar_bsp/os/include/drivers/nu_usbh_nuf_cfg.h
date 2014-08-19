/**************************************************************************
*
*              Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*       nu_usbh_nuf_cfg.h
*
* COMPONENT
*
*       Nucleus USB Host FILE Driver.
*
* DESCRIPTION
*
*       This file contains configuration for Nucleus USB Host File
*       Drivers.
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

#ifndef     _NU_USBH_NUF_CFG_H

#ifdef      __cplusplus
extern  "C" {                               /* C declarations in C++.    */
#endif

#define _NU_USBH_NUF_CFG_H

/* ======================  Configurable Macros  ======================== */

#define NUF_USBH_MAX_DEVICES            8   /* Maximum USB Drives.       */

/* Device Polling Task's Stack Size. */
#if (NU_USB_OPTIMIZE_FOR_SIZE)
#define NUF_USBH_POLL_TASK_STACK_SIZE   6 * NU_MIN_STACK_SIZE
#else
#define NUF_USBH_POLL_TASK_STACK_SIZE   2048
#endif /* NU_USB_OPTIMIZE_FOR_SIZE */

/* Device Polling Task's Priority. */
#define NUF_USBH_POLL_TASK_PRIORITY     (MSC_EVENT_REPORTER_TASK_PRIORITY+2)

/* Devices pool time. Time in milliseconds which all devices are polled.*/
#define NUF_USBH_DEVS_POLL_INTERVAL     250

/* Sector Size */
#define SECTOR_SIZE                     512
/*
 * Size of un-cached memory pool (in the form of sectors) used by Nucleus USB Host FILE Interface for I/O.
 * It is maximum number of sectors that Nucleus USB Host FILE Interface can request for read/write to USB Host driver.
 * It is synchronized with Nucleus FAT File System which can also read/write maximum 256 sectors at a time.
 * Where the size of each sector is 512 Bytes.
*/
#define USB_NUF_RW_BUFF_SIZE            CFG_NU_OS_DRVR_USB_HOST_FILE_IF_NUM_SECTORS * SECTOR_SIZE

/* ===================================================================== */
#ifdef      __cplusplus
}                                           /* End of C declarations.    */
#endif

#endif      /* _NU_USBH_NUF_CFG_H        */

/* ======================  End Of File  ================================ */
