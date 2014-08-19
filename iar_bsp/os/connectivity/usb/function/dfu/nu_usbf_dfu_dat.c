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
*   nu_usbf_dfu_dat.c
*
* COMPONENT
*
*   Nucleus USB Function software : DFU class driver.
*
* DESCRIPTION
*
*   This file defines the dispatch table and other global data for DFU
*   class driver.
*
* DATA STRUCTURES
*
*   DFUF_Usbf_Dispatch   DFU Class dispatch table.
*   DFUF_Rqst_Handlers   Routines modeled to act as a DFU states.
*
* FUNCTIONS
*
*   None.
*
* DEPENDENCIES
*
*   nu_usb.h
*
**************************************************************************/

/*==============  USB Include Files ====================================*/
#include "connectivity/nu_usb.h"
#include "nu_usbf_dfu_ext.h"

/*======================  Global data ==================================*/
NU_USBF_DFU *NU_USBF_DFU_Cb_Pt;

const NU_USBF_DFU_DISPATCH DFUF_Usbf_Dispatch =
{
    {
        {
            {
                _NU_USBF_DFU_Delete,
                _NU_USB_Get_Name,            /* does not override. */
                _NU_USB_Get_Object_Id        /* does not override. */
            },

            /* An interface driver need not have Examine_Device function
             * and similarly a device driver need not have
             * Examine_Interface function pointer.
             */
            _NU_USB_DRVR_Examine_Intf,
            NU_NULL,
            _NU_USB_DRVR_Get_Score,

            /* Only one of these will be called by stack depending on
             * whether the class driver is a interface/device driver.
             */
            NU_NULL,
            _NU_USBF_DFU_Initialize_Intf,
            _NU_USBF_DFU_Disconnect
        },
        _NU_USBF_DFU_Set_Intf,
        _NU_USBF_DFU_New_Setup,
        NU_NULL,

        /* Notifies driver of any events. */
        _NU_USBF_DFU_Notify
    }

};

/* Array containing the Request handler pointers */
const DFU_RQST_HANDLER DFUF_Rqst_Handlers[NUM_DFU_STATE_HANDLERS] =
{
    DFUF_State_App_Idle,
    DFUF_State_App_Detach,
    DFUF_State_Dfu_Idle,
    DFUF_State_Dfu_Dnload_Sync,
    DFUF_State_Dfu_Dnload_Busy,
    DFUF_State_Dfu_Dnload_Idle,
    DFUF_State_Dfu_Mnfst_Sync,
    DFUF_State_Dfu_Mnfst,
    DFUF_State_Dfu_Mnfst_wt_Reset,
    DFUF_State_Dfu_Upload_Idle,
    DFUF_State_Dfu_Error
};
/*============================  End Of File  ===========================*/
