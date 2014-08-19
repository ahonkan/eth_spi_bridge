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
* FILE NAME
*
*       nu_usbf_scsi_media_dat.c
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the shared data definitions for the Mass
*       Storage SCSI user driver.
*
* DATA STRUCTURES
*
*       NU_USBF_SCSI_MEDIA_DISPATCH         USB Function SCSI Media dispatch.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
/* USB Include Files */
#include    "connectivity/nu_usb.h"

/* =====================  Global data ================================  */

/* Creating a global instance for SCSI Media driver's dispatch table. */
const NU_USBF_SCSI_MEDIA_DISPATCH Scsi_Ram_Dispatch = {
    {
     /* USB dispatch. */
     _NU_USBF_SCSI_MEDIA_Delete,
     _NU_USB_Get_Name,
     _NU_USB_Get_Object_Id,
    },

    /* SCSI media dispatch. */
    _NU_USBF_SCSI_MEDIA_Connect,
    _NU_USBF_SCSI_MEDIA_Disconnect,
    _NU_USBF_SCSI_MEDIA_Transfer,
    _NU_USBF_SCSI_MEDIA_Tx_Done,
    _NU_USBF_SCSI_MEDIA_Ready,
    _NU_USBF_SCSI_MEDIA_Sense,
    _NU_USBF_SCSI_MEDIA_Inquiry,
    _NU_USBF_SCSI_MEDIA_Mode_Sense6,
    _NU_USBF_SCSI_MEDIA_Mode_Sel6,
    _NU_USBF_SCSI_MEDIA_Snd_Diag,
    _NU_USBF_SCSI_MEDIA_Format,
    _NU_USBF_SCSI_MEDIA_ReserveUnit,
    _NU_USBF_SCSI_MEDIA_ReleaseUnit,
    _NU_USBF_SCSI_MEDIA_Capacity,
    _NU_USBF_SCSI_MEDIA_Verify,
    _NU_USBF_SCSI_MEDIA_Command_23,
    _NU_USBF_SCSI_MEDIA_Reset_Device,
    NU_NULL,
    NU_NULL,
    _NU_USBF_SCSI_MEDIA_Mode_Sense_10,
    _NU_USBF_SCSI_MEDIA_Mode_Sel_10,
    _NU_USBF_SCSI_MEDIA_Prevent_Allow,
    _NU_USBF_SCSI_MEDIA_Start_Stop
};

/************************* End Of File ***********************************/
