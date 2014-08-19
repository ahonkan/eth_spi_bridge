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
*
*       nu_usbf_comm_data_ext.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Data Interface Class Driver
*
* DESCRIPTION
*
*       This file contains definitions for external Interfaces exposed by
*       Data Interface Class Driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nu_usbf_comm_data_imp.h             Internal Definitions.
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_COMM_DATA_EXT_H_
#define _NU_USBF_COMM_DATA_EXT_H_

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
#include "connectivity/nu_usbf_comm_data_imp.h"

/* ==================================================================== */
/* NU_USBF_COMM_DATA Constructor. */
/* The prototype may take more parameters depending on implementation */
STATUS NU_USBF_COMM_DATA_Create (NU_USBF_COMM_DATA *cb, CHAR *name);

STATUS NU_USBF_COMM_DATA_Send(NU_USBF_DRVR  *cb, UINT8 *buffer,
                                            UINT32 length, VOID *handle);

STATUS NU_USBF_COMM_DATA_Get_Rcvd(NU_USBF_DRVR  *cb, UINT8 **data_out,
                            UINT32 *data_len_out, VOID *handle);

STATUS NU_USBF_COMM_DATA_Config_Xfers(NU_USBF_COMM_DATA* data_drvr,
                                    VOID *handle, COMMF_DATA_CONF *conf);

STATUS NU_USBF_COMM_DATA_Reg_Rx_Buffer(NU_USBF_COMM_DATA* data_drvr,
                           VOID *handle,
                           COMMF_RX_BUF_GROUP *buffer_group);

STATUS NU_USBF_COMM_DATA_Dis_Reception(NU_USBF_COMM_DATA* data_drvr,
                                                            VOID *handle);

STATUS NU_USBF_COMM_DATA_Rbg_Create(COMMF_RX_BUF_GROUP* buff_grp,
                                      COMMF_BUFF_FINISHED  callback,
                                      UINT8** buff_array,
                                      UINT32  num_of_buffs,
                                      UINT32  buff_size);

STATUS NU_USBF_COMM_DATA_Cancel_Io(
            NU_USBF_COMM_DATA *cb,
            VOID * handle);

/* Initialization function called to initialize Function COMM DATA driver. */
STATUS nu_os_conn_usb_func_comm_data_init(CHAR *path, INT startstop);

/* Get Handle to the Function COMM DATA driver */
STATUS NU_USBF_COMM_DATA_GetHandle ( VOID** handle );

/* ====================  DATA Services ========================= */

STATUS  _NU_USBF_COMM_DATA_Init_Intf (NU_USB_DRVR * cb,
                                    NU_USB_STACK * stack,
                                    NU_USB_DEVICE * dev,
                                    NU_USB_INTF * intf);

STATUS  _NU_USBF_COMM_DATA_Disconnect (NU_USB_DRVR * cb,
                          NU_USB_STACK * stack, NU_USB_DEVICE * dev);

STATUS _NU_USBF_COMM_DATA_Set_Intf (NU_USB_DRVR * cb,
                             NU_USB_STACK * stack,
                             NU_USB_DEVICE * device,
                             NU_USB_INTF * intf,
                             NU_USB_ALT_SETTG * alt_settting);

STATUS  _NU_USBF_COMM_DATA_New_Setup (NU_USB_DRVR * cb,
                         NU_USB_STACK * stack,
                         NU_USB_DEVICE * device,
                         NU_USB_SETUP_PKT * setup);

STATUS  _NU_USBF_COMM_DATA_New_Transfer (NU_USB_DRVR * cb,
                            NU_USB_STACK * stack,
                            NU_USB_DEVICE * device, NU_USB_PIPE * pipe);

STATUS  _NU_USBF_COMM_DATA_Notify (NU_USB_DRVR * cb,
                      NU_USB_STACK * stack,
                      NU_USB_DEVICE * device, UINT32 event);

STATUS _NU_USBF_COMM_DATA_Delete (VOID *cb);

/* ==================================================================== */
#ifdef          __cplusplus
}                                           /* End of C declarations    */
#endif

#endif      /* _NU_USBF_COMM_DATA_EXT_H_*/

/* ======================  End Of File  =============================== */
