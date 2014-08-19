/**************************************************************************
*
*               Copyright Mentor Graphics Corporation 2012
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
*   nu_usbf_dfu_cfg.h
*
* COMPONENT
*
*   Nucleus USB Function Software : DFU Class Driver
*
* DESCRIPTION
*
*   This file contains the configurable parameters for DFU Class Driver.
*
* DATA STRUCTURES
*
*   None.
*
* FUNCTIONS
*
*   None.
*
* DEPENDENCIES
*
*   None.
*
**************************************************************************/

/*======================================================================*/
#ifndef _NU_USBF_DFU_CFG_H_

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_DFU_CFG_H_

/*======================= macros =======================================*/
/* This macro is used to represent the number of timer ticks which make up
 * the 1 ms. Ten ticks is equal to 1ms.
 */
#define DFUF_NUM_OF_TMR_TICKS_FOR_1_MS  \
            10

/* This macro is used for creating the detach timer to represent the
 * initial timer ticks.
 */
#define DFUF_WDETACH_TMOUT_IN_TICKS  \
            CFG_NU_OS_CONN_USB_FUNC_DFU_DETACH_TIMEOUT_TICKS

/* This is the time, in milliseconds, that the device will wait after
 * receipt of the DFU_DETACH request from the host. If this time elapses
 * without a USB reset being received by the device, then the device will
 * terminate the reconfiguration phase and revert back to the normal
 * operation.
 */
#define DFUF_WDETACH_TMOUT_IN_MS  \
            CFG_NU_OS_CONN_USB_FUNC_DFU_DETACH_TIMEOUT

/* This is the minimum time, in milliseconds, that the host should wait
 * before sending a DFU_GETSTATUS request subsequent to a DFU_DNLOAD or
 * DFU_UPLOAD request. This configurable parameter allows the device to
 * dynamically adjust the amount of time that the device expects the host to
 * wait between the status phase of the next DFU_DNLOAD and the subsequent
 * solicitation of the device’s status via DFU_GETSTATUS. This permits the
 * device to vary the delay depending on its need to erase memory, program
 * the memory, etc.
 */
#define DFUF_POLL_TM_OUT  \
            CFG_NU_OS_CONN_USB_FUNC_DFU_POLL_TIME

/* Macro for DFU Runtime functional descriptor bmAttributes. This is the
 * configuration for which the device is able to communicate via USB after
 * Manifestation phase.
 */
#define DFUF_RTM_FUN_DES_BMATTRIBUTES  \
            ( (CFG_NU_OS_CONN_USB_FUNC_DFU_BITWILL_DETACH << 3) | \
              (CFG_NU_OS_CONN_USB_FUNC_DFU_BITMENIFEST_TOLERANT << 2) | \
              (CFG_NU_OS_CONN_USB_FUNC_DFU_BIT_CANUPLOAD << 1) | \
               CFG_NU_OS_CONN_USB_FUNC_DFU_BIT_CANDNLOAD )

/* Maximum numbers of bytes that the device can accept per control write
 * transaction.  This is also the size of the data buffer in the DFU class
 * driver, which is used for buffering the firmware data.
 */
#define DFUF_WTRANSFER_SIZE  \
            CFG_NU_OS_CONN_USB_FUNC_DFU_TRANSFER_SIZE

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif /* _NU_USBF_DFU_CFG_H_ */

/*============================  End Of File  ===========================*/
