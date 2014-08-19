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

************************************************************************
*
* FILE NAME
*
*       nu_usbf_comm_data_imp.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Data Interface Class Driver
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for Data Interface Class Driver.
*
* DATA STRUCTURES
*
*       NU_USBF_COMM_DATA_DISPATCH          Communication Class Driver
*                                           Dispatch Table.
*       NU_USBF_COMM_DATA                   Communication Class Driver
*                                           Control Block.
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nu_usbf_comm_ext.h                  Communication interface class
*                                           definitions.
*       nu_usbf_comm_data_dat.h             Dispatch Table Definitions.
*       nu_usbf_drvr_ext.h                  Function class driver interface
*                                           definition.
*
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_COMM_DATA_IMP_H_
#define _NU_USBF_COMM_DATA_IMP_H_

#ifdef          __cplusplus
extern  "C"                                 /* C declarations in C++    */
{
#endif

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */
#include "connectivity/nu_usbf_drvr_ext.h"
#include "connectivity/nu_usbf_comm_ext.h"

/* ============= Constants ============= */
#define COMMF_DATA_CLASS     0x0A

/* =============Dispatch Table ============= */
typedef struct _nu_usbf_comm_data_dispatch
{
    NU_USBF_DRVR_DISPATCH   dispatch;
}
NU_USBF_COMM_DATA_DISPATCH;

/* ============= Control Block ============= */

typedef struct _nu_usbf_comm_data
{
    NU_USBF_DRVR parent;

    /* Pointer to Communication device initialize by NU_USBF_COMM
     * (Communication Interface driver).
     */
    NU_USBF_COMM* commf_mng_drvr;           /* Pointer to COMM driver.  */
}
NU_USBF_COMM_DATA;

/* =============== Functions =============== */

STATUS COMMF_DATA_Submit_Rx_Data_IRP (USBF_COMM_DEVICE* device);

STATUS  COMMF_DATA_Submit_Rx_IRP_Impl (USBF_COMM_DEVICE* device);

VOID COMMF_DATA_Tx_IRP_Complete (NU_USB_PIPE * pipe, NU_USB_IRP  *irp);

VOID COMMF_DATA_Rx_IRP_Complete (NU_USB_PIPE * pipe, NU_USB_IRP  *irp);

STATUS COMMF_DATA_Submit_Tx_Data_IRP (USBF_COMM_DEVICE* device);

STATUS COMMF_DATA_Submit_Tx_IRP_Impl (USBF_COMM_DEVICE* device);

STATUS    COMMF_Enqueue_Tx_Transfer (
                USBF_COMM_DEVICE * device,
                VOID * buffer,
                UINT32 length);

STATUS    COMMF_Dequeue_Tx_Transfer (
                USBF_COMM_DEVICE * device,
                VOID ** buffer,
                UINT32 * length);

STATUS    COMMF_Enqueue_Rx_Buff_Grp (
                USBF_COMM_DEVICE * device,
                COMMF_RX_BUF_GROUP * buff_grp);

STATUS    COMMF_Clear_Rx_Buff_Grp_Queue (
                USBF_COMM_DEVICE * device);

STATUS    COMMF_Get_Rx_Buffer (
                USBF_COMM_DEVICE * device,
                VOID ** buffer,
                UINT32 * length);

STATUS    COMMF_Put_Back_Rx_Buffer (
                USBF_COMM_DEVICE * device);

STATUS    COMMF_Is_Rx_Buff_Grp_Finished (
                USBF_COMM_DEVICE * device,
                BOOLEAN * bFinished,
                COMMF_RX_BUF_GROUP ** group);

STATUS    COMMF_Init_Rx_Buff_Grp_Queue (
                USBF_COMM_DEVICE * device);

STATUS    COMMF_Uninit_Rx_Buff_Grp_Queue (
                USBF_COMM_DEVICE * device);

/* ==================================================================== */

#include "connectivity/nu_usbf_comm_data_dat.h"

/* ==================================================================== */
#ifdef          __cplusplus
}                                           /* End of C declarations    */
#endif

#endif      /* _NU_USBF_COMM_DATA_IMP_H_*/

/* ======================  End Of File  =============================== */

