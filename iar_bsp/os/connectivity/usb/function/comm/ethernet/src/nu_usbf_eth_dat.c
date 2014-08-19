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
*       nu_usbf_eth_dat.c
*
* COMPONENT
*
*       Nucleus USB Function software : Ethernet User Driver.
*
* DESCRIPTION
*
*       This file defines the dispatch table for Ethernet User Driver.
*
* DATA STRUCTURES
*
*       usbf_eth_dispatch                   Ethernet User Driver dispatch
*                                           table.
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
NU_USBF_ETH		*NU_USBF_USER_ETH_Cb_Pt;
/* Control block for function Ethernet device to be used for
 * EF_VERSION 1.6 and less.
 */
#if (EF_VERSION_COMP < EF_2_0)
NU_USBF_ETH_DEV         NU_USBF_ETH_DEV_CB;
#endif

const NU_USBF_ETH_DISPATCH usbf_eth_dispatch =
{
    /* COMM user dispatch */
    {
        /* USBF user dispatch */
        {
            /* USB user dispatch */
            {
                /* USB dispatch */
                {
                    _NU_USBF_ETH_Delete,
                    _NU_USB_Get_Name,       /* does not override. */
                    _NU_USB_Get_Object_Id   /* does not override. */
                },

                _NU_USBF_ETH_Connect,
                _NU_USBF_ETH_Disconnect
            },

            _NU_USBF_ETH_New_Command,
            _NU_USBF_ETH_New_Transfer,
            _NU_USBF_ETH_Tx_Done,
            _NU_USBF_ETH_Notify
        },
        _NU_USBF_ETH_DATA_Connect,
        _NU_USBF_ETH_DATA_Disconnect,
        _NU_USBF_USER_COMM_Wait
    }

};

/* ======================  End Of File  =============================== */
