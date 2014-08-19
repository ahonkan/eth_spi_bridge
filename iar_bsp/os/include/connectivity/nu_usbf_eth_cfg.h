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
* FILE NAME
*
*       nu_usbf_eth_cfg.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Ethernet User Driver.
*
* DESCRIPTION
*
*       This file contains configuration definitions for Ethernet and 
*       RNDIS.
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

/* ==================================================================== */
#ifndef _NU_USBF_ETH_CFG_H_
#define _NU_USBF_ETH_CFG_H_

#ifdef          __cplusplus
extern  "C"                                 /* C declarations in C++    */
{
#endif

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

/* ==================================================================== */
#define INC_RNDIS           1
#define INC_ETH             2
#define INC_ETH_AND_RNDIS   3

/* Modify this constant to include RNDIS, Ethernet or both
 * configurations in ethernet demo. This modification is necessary
 * for targets which do not support multiple configurations.
 */
#define ETHF_DESC_CONF      INC_ETH_AND_RNDIS

/* Modify the "max_seg_size" metadata option according to maximum 
 * ethernet frame size of application. NF_MAXIMUM_FRAME_SIZE constant in file
 * nu_usbf_ndis_imp.h should be same as this.
 */
#define ETHF_MAX_SEG_SIZE   CFG_NU_OS_CONN_USB_FUNC_COMM_ETH_MAX_SEG_SIZE
/* ==================================================================== */
#ifdef          __cplusplus
}                                           /* End of C declarations    */
#endif

#endif                                      /* _NU_USBF_ETH_CFG_H_ */

/* ======================  End Of File  =============================== */
