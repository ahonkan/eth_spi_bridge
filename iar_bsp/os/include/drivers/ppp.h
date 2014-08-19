/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       ppp.h
*
*   COMPONENT
*
*       PPP - Core component of PPP
*
*   DESCRIPTION
*
*       This file remains for backward compatibility of the sdc.c driver,
*       which needs the declarations below. This file should be added
*       to any application that uses the serial driver.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nu_ppp.h
*       hdlc.h
*       ppp_dc.h
*       mdm_defs.h
*       mdm_extr.h
*       nu_usb.h
*
*************************************************************************/
#ifndef PPP_INC_PPP_H
#define PPP_INC_PPP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

#include "drivers/nu_ppp.h"
#include "drivers/hdlc.h"

#include "drivers/mdm_defs.h"
#include "drivers/mdm_extr.h"

#include "drivers/serial.h"

#ifdef PPP_USB_ENABLE
#include "usb/base/inc/nu_usb.h"
#endif

extern VOID *HDLC_RX_HISR_Mem;
extern INT PPP_Open_Count;
extern NU_HISR PPP_RX_HISR;
extern VOID Serial_Rx_Task_Entry(UNSIGNED argc, VOID *argv);

#if (HDLC_POLLED_TX == NU_FALSE)
extern NU_HISR PPP_TX_HISR;
extern VOID *HDLC_TX_HISR_Mem;
extern VOID Serial_Tx_Task_Entry(UNSIGNED argc, VOID *argv);
#endif

extern DV_DEVICE_ENTRY *_ppp_tx_dev_ptr_queue[HDLC_MAX_TX_QUEUE_PTRS];
extern UINT8 _ppp_tx_dev_ptr_queue_write;


#define PPP_LAYER                   LINK_LAYER
#define ppp_layer                   dev_link_layer
#define PPP_MAX_TX_QUEUE_PTRS       HDLC_MAX_TX_QUEUE_PTRS
#define PPP_FCS_SIZE                HDLC_FCS_SIZE
#define PPP_MAX_PROTOCOL_SIZE       HDLC_MAX_PROTOCOL_SIZE
#define PPP_MAX_ADDR_CONTROL_SIZE   HDLC_MAX_ADDR_CONTROL_SIZE


#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_PPP_H */
