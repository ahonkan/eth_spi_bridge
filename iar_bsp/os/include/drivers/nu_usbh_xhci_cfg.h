/*************************************************************************
*
*              Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       nu_usbh_xhci_cfg.h
*
* COMPONENT
*
*       USB XHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains definitions for user configurable options for
*       xHCI Host Controller Driver.
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

#ifndef _NU_USBH_XHCI_CFG_H_
#define _NU_USBH_XHCI_CFG_H_

/* TRBS_PER_SEGMENT must be a multiple of 4, since the command ring is
 * 64-byte aligned. It must also be greater than 16.
 */
#define XHCI_TRBS_PER_SEGMENT               16

/* Maximum data transferred by the TRB. */
#define XHCI_TRB_MAX_BUFF_SIZE              (64*1024)

/* Number of segments in the command,transfer and event ring.*/
#define XHCI_EVENT_RING_SEGMENTS            1
#define XHCI_CMD_RING_SEGMENTS              1
#define XHCI_TRANSFER_RING_SEGMENTS         1

/* Max number of USB devices for any host controller, section 6.1,maximum
 * value is 256
 */
#define  XHCI_MAX_SLOTS                     20

/* MaxPorts ,Section 5.3.3. maximum number is 127. */
#define  XHCI_MAX_PORTS                     16

/* Maximum size of ring - 64KB maximum .*/
#define XHCI_RING_MAX_SIZE                  (4*1024)

#endif /* _NU_USBH_XHCI_CFG_H_ */

/* ======================== End of File ================================ */
