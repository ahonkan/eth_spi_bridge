/************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation 
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME
*
*        nu_usbh_ehci_cfg.h
*
* COMPONENT
*
*       Nucleus USB Host software
*
* DESCRIPTION
*
*       This file contains definitions for user configurable options for
*       EHCI Host Controller Driver.
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
************************************************************************/

/* ==================================================================== */

#ifndef _NU_USBH_EHCI_CFG_H_
#define _NU_USBH_EHCI_CFG_H_

/* ==================================================================== */

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */


/* ==============  Standard Include Files ============================  */



/* ==================== USB Include Files ============================= */


/* =====================  Global data ================================  */


/* =====================  #defines ===================================  */

/* Max expected QTDs per submitted IRP */

#define USBH_EHCI_MAX_QTDS_PER_IRP      256

#define USBH_EHCI_MAX_ITDS_PER_IRP      32

#define USBH_EHCI_MAX_PERIODIC_PIPES    32



#define USBH_EHCI_FRAME_LIST_SIZE       1024


#define USBH_EHCI_MAX_QH_POOLS          1
#define USBH_EHCI_MAX_QTD_POOLS         2


#define USBH_EHCI_MAX_QHS_PER_POOL      64
#define USBH_EHCI_MAX_QTDS_PER_POOL     MAX_ELEMENTS_PER_POOL


#define USBH_EHCI_NORMALIZE_ISO_BUFFER   NU_FALSE

/* ==================================================================== */

/* ==================================================================== */

#endif /* _NU_USBH_EHCI_CFG_H_ */

/* =======================  End Of File  ============================== */
