/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

*************************************************************************
*
* FILE NAME
*
*        nu_usbh_ctrl_irp_ext.c
*
* COMPONENT
*   USB Host Stack
*
* DESCRIPTION
*   This file contains the function implementations of NU_USB_CTRL_IRP services
*
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*
*  NU_USBH_CTRL_IRP_Create          - Initializes the Control IRP control block.
*  NU_USBH_CTRL_IRP_Get_Direction   - Returns the direction of the data transfer
*                                     phase of the IRP.
*  NU_USBH_CTRL_IRP_Get_Setup_Pkt   - Returns pointer to the setup packet
*                                     associated with the IRP.
*  NU_USBH_CTRL_IRP_Get_bRequest    - Returns the bRequest field of the setup
*                                     packet of the IRP.
*  NU_USBH_CTRL_IRP_Get_bmRequestType - Returns the bmRequestType field of the
*                                       setup packet of the IRP.
*  NU_USBH_CTRL_IRP_Get_wIndex      - Returns the wIndex field of the setup
*                                     packet of the IRP.
*  NU_USBH_CTRL_IRP_Get_wLength     - Returns the wLength field of the setup
*                                     packet of the IRP.
*  NU_USBH_CTRL_IRP_Get_wValue      - Returns the wValue field of the setup
*                                     packet of the IRP.
*  NU_USBH_CTRL_IRP_Set_bRequest    - Sets the bRequest field of the setup
*                                     packet of the IRP.
*  NU_USBH_CTRL_IRP_Set_bmRequestType - Sets the bmRequestType field of the
*                                       setup packet of the IRP.
*  NU_USBH_CTRL_IRP_Set_wIndex        - Sets the wIndex field of the setup
*                                       packet of the IRP.
*  NU_USBH_CTRL_IRP_Set_wLength       - Sets the wlength field of the setup
*                                       packet of the IRP.
*  NU_USBH_CTRL_IRP_Set_wValue        - Sets the wvalue field of the setup
*                                       packet of the IRP.
*
* 	DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USBH_CTRL_IRP_EXT_C
#define USBH_CTRL_IRP_EXT_C

/* ==============  Standard Include Files ============================  */
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*  NU_USBH_CTRL_IRP_Create
*
* DESCRIPTION
*    Create the control IRP control block out of the user provided control
* block memory.  Control IRP is a specialization of IRP.It fills up the set up
* packet fields for the setup phase and data fields for the data phase of the
* control transfer.
*
* INPUTS
*   cb             - User supplier pointer to the memory of the control IRP
*                    control block.
*   data           - Pointer to the data buffer for IN/OUT data transfers
*                    during data phase.
*   callback       - The function ptr that would invoked by the stack, once
*                    the IRP transfer attempt is completed by the stack.
*                    Note: This callback function should avoid making calls that
*                    need NU_SUSPEND option.
*
*   context       -  A cookie that's passed to the callback function, when
*                    its invoked by the stack. The caller can store
*                    information in this pointer that can help,  identify
*                    the IRP when the callback is invoked.
*   bmRequestType - The field of the setup packet defined by the USB standard.
*   bRequest      - The field of the setup packet defined by the USB standard.
*   wValue        - The field of the setup packet defined by the USB standard.
*   wIndex        - The field of the setup packet defined by the USB standard.
*   wLength       - The field of the setup packet defined by the USB standard.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Create (NU_USBH_CTRL_IRP * cb,
                                UINT8 *data,
                                NU_USB_IRP_CALLBACK callback,
                                VOID *context,
                                UINT8 bmRequestType,
                                UINT8 bRequest,
                                UINT16 wValue,
                                UINT16 wIndex,
                                UINT16 wLength)
{
    STATUS status = NU_SUCCESS;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(callback);

    /* Call base class's create */
    status = NU_USB_IRP_Create ((NU_USB_IRP *) cb,
                                LE16_2_HOST (wLength), data, 1, 0, callback,
                                context, 0);

    if (status != NU_SUCCESS)
        return (status);

    cb->setup_data.bmRequestType = bmRequestType;
    cb->setup_data.bRequest = bRequest;
    cb->setup_data.wValue = wValue;
    cb->setup_data.wIndex = wIndex;
    cb->setup_data.wLength = wLength;

    return (status);
}

/*************************************************************************
* FUNCTION
*    NU_USBH_CTRL_IRP_Get_bmRequestType
*
* DESCRIPTION
*    This function returns the bmRequestType field of the setup packet
* associated with the control irp.
*
* INPUTS
*   cb                Pointer to the control IRP control block.
*   bmRequestType_out Pointer to memory location to hold the value of the
*                     bmRequestType field of the setup packet.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Get_bmRequestType (NU_USBH_CTRL_IRP * cb,
                                           UINT8 *bmRequestType_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(bmRequestType_out);

    *bmRequestType_out = cb->setup_data.bmRequestType;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*    NU_USBH_CTRL_IRP_Set_bmRequestType
*
* DESCRIPTION
*   This function sets the bmRequestType field of the setup packet
* associated with the control IRP.
*
* INPUTS
*   cb                Pointer to the control IRP control block.
*   bmRequestType     bmRequestType value of the setup packet associated
*                     with the control IRP.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Set_bmRequestType (NU_USBH_CTRL_IRP * cb,
                                           UINT8 bmRequestType)
{
    NU_USB_PTRCHK(cb);

    cb->setup_data.bmRequestType = bmRequestType;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*    NU_USBH_CTRL_IRP_Get_bRequest
*
* DESCRIPTION
*    This function returns the bRequest field of the setup packet
* associated with the control irp.
*
* INPUTS
*   cb                Pointer to the control IRP control block.
*   bRequest_out      Pointer to memory location to hold the value of the
*                     bRequest field of the setup packet.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Get_bRequest (NU_USBH_CTRL_IRP * cb,
                                      UINT8 *bRequest_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(bRequest_out);

    *bRequest_out = cb->setup_data.bRequest;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*    NU_USBH_CTRL_IRP_Set_bRequest
*
* DESCRIPTION
*   This function sets the bRequest field of the setup packet
* associated with the control IRP.
*
* INPUTS
*   cb                Pointer to the control IRP control block.
*   bRequest          bRequest value of the setup packet associated
*                     with the control IRP.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Set_bRequest (NU_USBH_CTRL_IRP * cb,
                                      UINT8 bRequest)
{
    NU_USB_PTRCHK(cb);

    cb->setup_data.bRequest = bRequest;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*    NU_USBH_CTRL_IRP_Get_wIndex
*
* DESCRIPTION
*    This function returns the wIndex field of the setup packet
* associated with the control irp.
*
* INPUTS
*   cb                Pointer to the control IRP control block.
*   wIndex_out        Pointer to memory location to hold the value of the
*                     wIndex field of the setup packet.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Get_wIndex (NU_USBH_CTRL_IRP * cb,
                                    UINT16 *wIndex_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(wIndex_out);

    *wIndex_out = cb->setup_data.wIndex;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*    NU_USBH_CTRL_IRP_Set_wIndex
*
* DESCRIPTION
*   This function sets the wIndex field of the setup packet
* associated with the control IRP.
*
* INPUTS
*   cb                Pointer to the control IRP control block.
*   wIndex            wIndex value of the setup packet associated
*                     with the control IRP.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Set_wIndex (NU_USBH_CTRL_IRP * cb,
                                    UINT16 wIndex)
{
    NU_USB_PTRCHK(cb);

    cb->setup_data.wIndex = wIndex;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*    NU_USBH_CTRL_IRP_Get_wValue
*
* DESCRIPTION
*    This function returns the wValue field of the setup packet
* associated with the control irp.
*
* INPUTS
*   cb                Pointer to the control IRP control block.
*   wValue_out        Pointer to memory location to hold the value of the
*                     wValue field of the setup packet.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Get_wValue (NU_USBH_CTRL_IRP * cb,
                                    UINT16 *wValue_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(wValue_out);

    *wValue_out = cb->setup_data.wValue;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*    NU_USBH_CTRL_IRP_Set_wValue
*
* DESCRIPTION
*   This function sets the wValue field of the setup packet
* associated with the control IRP.
*
* INPUTS
*   cb                Pointer to the control IRP control block.
*   wValue            wValue value of the setup packet associated
*                     with the control IRP.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Set_wValue (NU_USBH_CTRL_IRP * cb,
                                    UINT16 wValue)
{
    NU_USB_PTRCHK(cb);

    cb->setup_data.wValue = wValue;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*    NU_USBH_CTRL_IRP_Get_wLength
*
* DESCRIPTION
*    This function returns the wLength field of the setup packet
* associated with the control irp.
*
* INPUTS
*   cb                Pointer to the control IRP control block.
*   wLength_out       Pointer to memory location to hold the value of the
*                     wLength field of the setup packet.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Get_wLength (NU_USBH_CTRL_IRP * cb,
                                     UINT16 *wLength_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(wLength_out);

    *wLength_out = cb->setup_data.wLength;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*    NU_USBH_CTRL_IRP_Set_wLength
*
* DESCRIPTION
*   This function sets the wLength field of the setup packet
* associated with the control IRP.
*
* INPUTS
*   cb                Pointer to the control IRP control block.
*   wLength           wLength value of the setup packet associated
*                     with the control IRP.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Set_wLength (NU_USBH_CTRL_IRP * cb,
                                     UINT16 wLength)
{
    NU_USB_PTRCHK(cb);

    cb->setup_data.wLength = wLength;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*     NU_USBH_CTRL_IRP_Get_Direction
*
* DESCRIPTION
*    This function returns the direction of the transfer during the data
* phase of this control IRP, as gathered from the bmRequestType field.
*
* INPUTS
*   cb             Pointer to the control IRP control block.
*   direction_out  Pointer to variable to hold the value of the direction-
*                  USB_DIR_IN or USB_DIR_OUT.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Get_Direction (NU_USBH_CTRL_IRP * cb,
                                       UINT8 *direction_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(direction_out);

    if (cb->setup_data.bmRequestType & USB_REQ_IN)
        *direction_out = USB_DIR_IN;
    else
        *direction_out = USB_DIR_OUT;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*        NU_USBH_CTRL_IRP_Get_Setup_Pkt
*
* DESCRIPTION
*   This function returns pointer to the setup packet associated with the
* control IRP.
*
* INPUTS
*   cb        Pointer to the control IRP control block.
*   pkt_out   Pointer to memory location to hold the pointer to the setup
*             packet associated with the control IRP.
*
* OUTPUTS
*   NU_SUCCESS            Indicates successful completion of the service.
*   NU_USB_INVLD_ARG      Invalid parameters.
*************************************************************************/
STATUS NU_USBH_CTRL_IRP_Get_Setup_Pkt (NU_USBH_CTRL_IRP * cb,
                                       NU_USB_SETUP_PKT ** pkt_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(pkt_out);

    *pkt_out = &(cb->setup_data);
    return (NU_SUCCESS);
}

/************************************************************************/

#endif /* USBH_CTRL_IRP_EXT_C */
/* ======================  End Of File  =============================== */
