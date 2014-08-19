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
* FILE NAME
*   nu_usbh_ohci_cfg.h
*
* COMPONENT
*   Nucleus USB Host Software.
*
* DESCRIPTION
*   This file contains definitions for user configurable options for
*   OHCI Host Controller Driver.
*
* DATA STRUCTURES
*   None.
*
* FUNCTIONS
*   None.
*
* DEPENDENCIES
*   None.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_OHCI_CFG_H_
#define _NU_USBH_OHCI_CFG_H_
/* ===================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

/* OHCI_PORT_CFG_POWER define configures the root port's power switching.
 * valid values for this define are
 * OHCI_PORT_CFG_POWER_GROUP     : Power switching is applied to all ports.
 * OHCI_PORT_CFG_POWER_INDIVIDUAL: Power Switching is applied to
 *                                 individual ports.
 * OHCI_PORT_CFG_POWER_ALWAYS_ON : Power Switching is not available.
 * Please refer to OHCI specification article 7.4.1.
 */
#define OHCI_PORT_CFG_POWER OHCI_PORT_CFG_POWER_GROUP

/* OHCI_PORT_CFG_OC define configures the root port's over current
 * detection. valid values for this define are
 * OHCI_PORT_CFG_OC_GROUP        : Over current detection is applied to
 *                                 all ports.
 * OHCI_PORT_CFG_OC_INDIVIDUAL   : Over current detection is applied to
 *                                 individual ports.
 * OHCI_PORT_CFG_OC_NOT_SUPPORTED: Over current detection is not available.
 * Please refer to OHCI specification article 7.4.1.
 */
#define OHCI_PORT_CFG_OC OHCI_PORT_CFG_OC_GROUP

/* Any bit set in this define will represent a device that is not removable
 * on that port. Please refer to OHCI specification article 7.4.2.
 */
#define OHCI_PORT_CFG_DR 0x0000

/* Any bit set in this define will represent that the port is effected by
 * per-port power control. Please refer to OHCI specification article 7.4.2
 */
#define OHCI_PORT_CFG_POWER_CTRLMASK 0xFFFF

/* Power on to power good time. Specified in ms and used as
 * specified_value * 2. Please refer to OHCI specification article 7.4.1.
 */
#define OHCI_PORT_CFG_POTPGT 2

/* Maximum number of down stream ports supported by this controller. */
#define OHCI_MAX_PORTS 4

/* Set the value of the following define to NU_TRUE if OHCI hardware
 * supports remote wakeup.
 */
#define OHCI_REMOTE_WAKEUP_CONNECTED NU_TRUE

/* Number of TDs to delay the interrupt if more than
 * USBH_OHCI_MAX_TDS_PER_IRP number of TDs are required. Must be defined
 * in powers of 2.
 */
#define OHCI_DELAY_INTR_NUM_TD 4

/* Maximum number of CTRL, BULK or INTR TDs that can be scheduled
 * simultaneously for any endpoint.
 */
#define OHCI_MAX_TDS_PER_IRP 32

/* Maximum number of ISO TDs that can be scheduled simultaneously for any
 * ISO endpoint.
 */
#define OHCI_MAX_ISO_TDS_PER_IRP 32


/* Following defines configures how many total TDs can be allocated from
 * memory. OHCI allocates one memory pool at a time and when all the TDs
 * are consumed next pool is allocated. Each TD requires 4096 bytes of
 * un-cacheable memory to if the buffers provided by application are in
 * cacheable memory or if cache flushing mechanism is not available. Memory
 * of un-cacheable buffers is allocated in hardware environment's
 * normalize_buffer and Tx_Done routines.
 */
#define OHCI_MAX_TD_GEN_POOLS 2
#define OHCI_MAX_TDS_PER_POOL 32

#define USBH_OHCI_NORMALIZE_ISO_BUFFER       NU_FALSE

/* ===================================================================== */
#endif /* _NU_USBH_OHCI_CFG_H_ */

/* ======================  End Of File.  =============================== */
