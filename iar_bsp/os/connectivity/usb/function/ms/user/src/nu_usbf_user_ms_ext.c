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
*       nu_usbf_user_ms_ext.c
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the exported function implementations for the
*       Mass Storage class User driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       _NU_USBF_USER_MS_Delete             Un-initializes the Mass Storage
*                                           User.
*       NU_USBF_USER_MS_Get_Max_LUN         Retrieves the max LUN supported
*                                           by the storage device.
*       NU_USBF_USER_MS_Reset               Resets the storage device to a
*                                           known state.
*       _NU_USBF_USER_MS_Create             Initializes the Mass storage
*                                           user driver.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
/* USB Include Files. */
#include    "connectivity/nu_usb.h"

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_MS_Create
*
* DESCRIPTION
*
*        Initializes the Mass storage user driver.
*
* INPUTS
*
*        pcb_user_ms         Mass Storage User control block.
*        p_name              Name of the driver.
*        subclass            USB Subclass supported by this driver.
*        max_lun             Maximum Logical Unit Number(LUN) supported by
*                            this driver.
*        p_dispatch          Pointer to the dispatch table.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful initialization.
*        NU_USB_INVLD_ARG    Indicates an invalid argument.
*        NU_NOT_PRESENT      Indicates a configuration error because of
*                            which no more USB objects could be created.
*
**************************************************************************/
STATUS _NU_USBF_USER_MS_Create (NU_USBF_USER_MS *pcb_user_ms,
                                CHAR            *p_name,
                                UINT8            subclass,
                                UINT8            max_lun,
                                const VOID      *p_dispatch)
{

    STATUS status;                          /* Local variable. */

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;

    if((!pcb_user_ms) || (!p_name) || (!p_dispatch) || (subclass == 0))
    {
        status = NU_USB_INVLD_ARG;
    }

    /* max_lun is no longer in use. To avoid warning. */
    max_lun = max_lun;

    if(status == NU_SUCCESS)
    {
        /* Call parent create function. */
        status = _NU_USBF_USER_Create ((NU_USBF_USER *) pcb_user_ms,
                                       p_name,
                                       subclass,
                                       (UINT8)0,
                                       p_dispatch);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;                           /* Return status. */
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_USER_MS_Reset
*
* DESCRIPTION
*
*        Resets the user. Stalls all the transfers on going, clears any
*        errors and brings the device to a stable state.
*
* INPUTS
*
*        pcb_user_ms             Mass Storage User control block.
*        p_drvr                  Mass storage class driver control block.
*        p_handle                Unique mass storage device identification.
*
* OUTPUTS
*
*        NU_SUCCESS              Indicates successful processing of this
*                                event.
*        NU_USB_NOT_SUPPORTED    Indicates that the media doesn't support
*                                this event.
*        NU_USB_INVLD_ARG        Indicates an invalid parameter.
*
**************************************************************************/
STATUS NU_USBF_USER_MS_Reset (NU_USB_USER *pcb_user_ms,
                              NU_USB_DRVR *p_drvr,
                              VOID        *p_handle)
{
    STATUS status = NU_USB_INVLD_ARG;

    if ((pcb_user_ms) && (p_drvr) && (p_handle))
    {
        status = NU_USB_NOT_SUPPORTED;
        if (((NU_USBF_USER_MS_DISPATCH *)
            (((NU_USB *) pcb_user_ms)->usb_dispatch))->Reset != NU_NULL)
        {
            /* Call mass storage user dispatch Reset functionality. */
            status = ((NU_USBF_USER_MS_DISPATCH *)
                     (((NU_USB *) pcb_user_ms)->usb_dispatch))->
                     Reset (pcb_user_ms, p_drvr, p_handle);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_USER_MS_Get_Max_LUN
*
* DESCRIPTION
*
*        This function retrieves the maximum Logical Unit Number (LUN)
*        supported by the storage device.
*
* INPUTS
*
*        pcb_user            Pointer to Mass Storage User control block.
*        p_drvr              Pointer to Mass storage class driver control
*                            block.
*        p_handle            Pointer to the unique Mass Storage device
*                            identification.
*        p_max_lun_out       Pointer to the location where the maximum
*                            logical unit value needs to be stored.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful initialization.
*        NU_USB_INVLD_ARG    Indicates invalid parameter.
*
**************************************************************************/
STATUS NU_USBF_USER_MS_Get_Max_LUN (const NU_USB_USER *pcb_user_ms,
                                    const NU_USB_DRVR *p_drvr,
                                    const VOID        *p_handle,
                                    UINT8             *p_max_lun_out)
{

    STATUS status = NU_USB_INVLD_ARG;
    /* Assign maximum logical unit number to p_max_lun_out. Max LUN is
     * actually supplied by the user when user ms create function is
     * called.
     */
    if ((pcb_user_ms) && (p_drvr) && (p_handle) && (p_max_lun_out))
    {
        status = NU_USB_NOT_SUPPORTED;
        if (((NU_USBF_USER_MS_DISPATCH *)
            (((NU_USB *) pcb_user_ms)->usb_dispatch))->Get_Max_Lun !=
                                NU_NULL)
        {
            /* Call mass storage user dispatch Reset functionality. */
            status = ((NU_USBF_USER_MS_DISPATCH *)
                     (((NU_USB *) pcb_user_ms)->usb_dispatch))->
                     Get_Max_Lun (pcb_user_ms,
                                  p_drvr,
                                  p_handle,
                                  p_max_lun_out);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_MS_Delete
*
* DESCRIPTION
*
*        Un-initializes the Mass Storage user driver.
*
* INPUTS
*
*        pcb_usb          Pointer to the USB control block.
*
* OUTPUTS
*
*        NU_SUCCESS       Indicates successful un-initialization.
*
**************************************************************************/
STATUS _NU_USBF_USER_MS_Delete (NU_USB *pcb_usb)
{
   STATUS status = NU_USB_INVLD_ARG;

    /* Call stack user delete functionality. */
    if(pcb_usb)
    {
        status = _NU_USBF_USER_Delete (pcb_usb);
    }

    return status;
}

/************************* End Of File ***********************************/
