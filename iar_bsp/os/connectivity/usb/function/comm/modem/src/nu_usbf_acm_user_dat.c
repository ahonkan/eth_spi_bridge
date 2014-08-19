/**************************************************************************
*
*           Copyright 2005 Mentor Graphics Corporation
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
*       nu_usbf_acm_user_dat.c
*
*
* COMPONENT
*
*       Nucleus USB Function software : ACM User Driver.
*
* DESCRIPTION
*
*       This file defines the dispatch table for ACM User Driver.
*
* DATA STRUCTURES
*
*       usbf_acm_user_dispatch    ACM User Driver dispatch table.
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

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb.h"
/* =====================  Global data ================================  */

/* Control block for Function ACM user driver */
NU_USBF_ACM_USER		*NU_USBF_USER_ACM_Cb_Pt;

/* Control block for function ACM device to be used for
 * ACMF_VERSION 1.5 and less.
 */
#if (ACMF_VERSION_COMP < ACMF_2_0)
NU_USBF_ACM_DEV         NU_USBF_ACM_DEV_CB;
#endif

const NU_USBF_ACM_USER_DISPATCH usbf_acm_user_dispatch = {
    /* COMM user dispatch */
    {
        /* usbf user dispatch */
        {
            /* USB user dispatch */
            {
                /* USB dispatch */
                {
                    _NU_USBF_ACM_USER_Delete,
                    _NU_USB_Get_Name,        /* does not override. */
                    _NU_USB_Get_Object_Id    /* does not override. */
                },

                _NU_USBF_ACM_USER_Connect,
                _NU_USBF_ACM_USER_Disconnect
            },

            _NU_USBF_ACM_USER_New_Command,
            _NU_USBF_ACM_USER_New_Transfer,
            _NU_USBF_ACM_USER_Tx_Done,
            _NU_USBF_ACM_USER_Notify
        },
        _NU_USBF_ACM_DATA_Connect,
        _NU_USBF_ACM_DATA_Disconnect,
        _NU_USBF_USER_COMM_Wait
    }
    /* Extension to Driver Services goes here */
};
/* ======================  End Of File  =============================== */
