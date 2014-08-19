/**************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
****************************************************************************/

/****************************************************************************
*
* FILE NAME
*
*        nu_usb_otg_imp.c
*
* COMPONENT
*
*       Nucleus USB Software
*
* DESCRIPTION
*
*       This file provides the implementation of OTG LISR for USB OTG devices.
*
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       NU_USB_OTG_HWCTRL_Install_ISR    common ISR for function and host controllers
*       NU_USB_OTG_ISR                   This function installs ISR for function and 
*                                        host controllers.
* DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*
***********************************************************************************/

/* =======================Include Files =========================== */

#include "connectivity/nu_usb.h"

#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 1

STATUS NU_USB_OTG_Install_ISR(INT vector, VOID(*isr)(INT) );
VOID USBOTG_LISR(INT vector);
VOID(*g_isr1)(INT);

VOID(*g_isr2)(INT);

/**************************************************************************
* FUNCTION
*
*       NU_USB_OTG_Install_ISR
*
* DESCRIPTION
*
*       This function installs ISR for function and host controllers.
*
* INPUTS
*
*       vector                              IRQ number for which ISR is to
*                                           be registered.
*       isr                                 Pointer to ISR.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*
**************************************************************************/
STATUS NU_USB_OTG_Install_ISR(INT vector, VOID(*isr)(INT) )
{
    STATUS status = NU_SUCCESS;

    /* Function pointer for already register ISR. */
    VOID (*temp)(INT);

    if(isr == USBF_LISR)
    {
        /* Store pointer to USB function stack LISR */    
        g_isr1 = isr;
    }
        
    if(isr == USBH_LISR)
    {
        /* Store pointer to USB Host stack LISR */
        g_isr2 = isr;   
    }


    status = NU_Register_LISR(vector, USBOTG_LISR, &temp);

    /* Return status. */
    return (status);
}

/**************************************************************************
* FUNCTION
*
*       USBOTG_LISR
*
* DESCRIPTION
*
*       Common ISR for function and host controller.
*
* INPUTS
*
*       vector                              IRQ number for which ISR is to
*                                           be registered.
*
* OUTPUTS
*
*       None
*
**************************************************************************/
VOID USBOTG_LISR(INT vector)
{
    /* If g_isr1 holds pointer to ISR, activate the ISR. */
    if (g_isr1 != NU_NULL)
    {
        g_isr1(vector);
    }

    /* If g_isr2 holds pointer to ISR, activate the ISR. */
    if (g_isr2 != NU_NULL)
    {
        g_isr2(vector);
    }
}
#endif
