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
*       nu_usbf_user_scsi_dat.c
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the shared data definitions for the Mass storage
*       SCSI user driver.
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
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
/* USB Include Files. */
#include "connectivity/nu_usb.h"

/* =====================  Global data  ================================= */
NU_USBF_USER_SCSI   *NU_USBF_USER_SCSI_Cb_Pt;

const NU_USBF_USER_SCSI_DISPATCH usbf_user_scsi_dispatch =
{
    /* Mass Storage User dispatch. */
    {
        /* USBF user dispatch. */
        {
            /* USB user dispatch. */
            {
                /* USB dispatch. */
                {
                    _NU_USBF_USER_SCSI_Delete,
                    _NU_USB_Get_Name,
                    _NU_USB_Get_Object_Id,
                },

                _NU_USBF_USER_SCSI_Connect,
                _NU_USBF_USER_SCSI_Disconnect,
            },

             _NU_USBF_USER_SCSI_Command,
             _NU_USBF_USER_SCSI_Transfer,
             _NU_USBF_USER_SCSI_Tx_Done,
             _NU_USBF_USER_SCSI_Notify
        },

     _NU_USBF_USER_SCSI_Reset,
     _NU_USBF_USER_SCSI_Get_Max_Lun

	},

    _NU_USBF_USER_SCSI_Reg_Media,
    _NU_USBF_USER_SCSI_Dereg_Media
};

/* Command Processing Table. */
NU_USBF_USER_SCSI_CMD nu_usbf_user_scsi_command_table[] = {

/* Since read/write are executed most of the times. They are listed on top
 * this will reduce searching time.*/

    {USB_SPEC_MSF_READ_10,         USB_DIR_IN,
                                   NU_USBF_SCSI_MEDIA_Read_10},

    {USB_SPEC_MSF_WRITE_10,        USB_DIR_OUT,
                                   NU_USBF_SCSI_MEDIA_Write_10},

    {USB_SPEC_MSF_READ_6,          USB_DIR_IN,
                                   NU_USBF_SCSI_MEDIA_Read_6},

    {USB_SPEC_MSF_WRITE_6,         USB_DIR_OUT,
                                   NU_USBF_SCSI_MEDIA_Write_6},

    {USB_SPEC_MSF_WRITE_12,        USB_DIR_OUT,
                                   NU_USBF_SCSI_MEDIA_Write_12},

    {USB_SPEC_MSF_READ_12,         USB_DIR_IN,
                                   NU_USBF_SCSI_MEDIA_Read_12},

    {USB_SPEC_MSF_TEST_UNIT_READY, USB_SPEC_MSF_NO_DATA_STAGE,
                                   NU_USBF_SCSI_MEDIA_Ready},

    {USB_SPEC_MSF_REQUEST_SENSE,   USB_DIR_IN,
                                   NU_USBF_SCSI_MEDIA_Sense},

    {USB_SPEC_MSF_INQUIRY,         USB_DIR_IN,
                                   NU_USBF_SCSI_MEDIA_Inquiry},

    {USB_SPEC_MSF_READ_CAPACITY,   USB_DIR_IN,
                                   NU_USBF_SCSI_MEDIA_Capacity},

    {USB_SPEC_MSF_MODE_SELECT_6,   USB_DIR_OUT,
                                   NU_USBF_SCSI_MEDIA_Mode_Sel6},

    {USB_SPEC_MSF_MODE_SELECT_10,  USB_DIR_OUT,
                                   NU_USBF_SCSI_MEDIA_Mode_Sel_10},

    {USB_SPEC_MSF_MODE_SENSE_6,    USB_DIR_IN,
                                   NU_USBF_SCSI_MEDIA_Mode_Sense_6},

    {USB_SPEC_MSF_MODE_SENSE_10,   USB_DIR_IN,
                                   NU_USBF_SCSI_MEDIA_Mode_Sense_10},

    {USB_SPEC_MSF_SEND_DIAGNOSTIC, USB_DIR_OUT,
                                   NU_USBF_SCSI_MEDIA_Snd_Diag},

    {USB_SPEC_MSF_FORMAT_UNIT,     USB_DIR_OUT,
                                   NU_USBF_SCSI_MEDIA_Format},

    {USB_SPEC_MSF_RESERVE_UNIT,    USB_DIR_OUT,
                                   NU_USBF_SCSI_MEDIA_Reserve_Unit},

    {USB_SPEC_MSF_RELEASE_UNIT,    USB_SPEC_MSF_NO_DATA_STAGE,
                                   NU_USBF_SCSI_MEDIA_Release_Unit},

    {USB_SPEC_MSF_VERIFY,          USB_SPEC_MSF_NO_DATA_STAGE,
                                   NU_USBF_SCSI_MEDIA_Verify},

    {USB_SPEC_MSF_PREVENT_ALLOW,   USB_SPEC_MSF_NO_DATA_STAGE,
                                   NU_USBF_SCSI_MEDIA_Prevent_Allow},

    {USB_SPEC_MSF_START_STOP,      USB_SPEC_MSF_NO_DATA_STAGE,
                                   NU_USBF_SCSI_MEDIA_Start_Stop}

};
/************************* End Of File ***********************************/
