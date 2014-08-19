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

***************************************************************************
*
* FILE NAME
*
*       nu_usbf_hw_ext.c
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the implementation of the exported functions
*       for the USB Device Software, Base Controller driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*      _NU_USBF_HW_Create                   Initializes the given hardware.
*      _NU_USBF_HW_Delete                   Un-initializes a given hardware
*       NU_USBF_HW_Get_Capability           Retrieves the capability of the
*                                           hardware.
*       NU_USBF_HW_Get_Endpoint_Status      Retrieves the status of a given
*                                           endpoint.
*       NU_USBF_HW_Get_Status               Retrieves the status of the
*                                           given hardware.
*       NU_USBF_HW_Set_Address              Sets the USB address of the
*                                           hardware.
*       NU_USBF_HW_Stall_Endpoint           Stalls a given endpoint.
*       NU_USBF_HW_Unstall_Endpoint         Un-stall a given endpoint.
*       _NU_USBF_HW_Create                  Initializes the given hardware.
*       _NU_USBF_HW_Initialize              Installs the USBF LISR for each
*                                           vector of the USB h/w.
*       _NU_USBF_HW_Uninitialize            Un-install the USBF LISR for
*                                           each vector of the USB h/w.
*       NU_USBF_HW_Start_HNP                Start HNP Signaling.
*       NU_USBF_HW_Send_FuncWakeNotif       This function is called to send
*                                           a function wake device
*                                           notification to host.
*       NU_USBF_HW_Set_Ux_Enable            This function is called upon 
*                                           receving a Set/Clear Feature U1/U2 
*                                           enable request.
*       NU_USBF_HW_Set_LTM_Enable           This function is called upon 
*                                           receiving a Set/Clear feature 
*                                           LTM enable request 
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
#ifndef USBF_HW_EXT_C
#define USBF_HW_EXT_C
/* ==============  USB Include Files ==================================  */

#include    "connectivity/nu_usb.h"

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_HW_Create
*
* DESCRIPTION
*
*       Initializes the Base Function Controller.
*       The following parameters describe the Controller itself.
*
* INPUTS
*
*       cb                                  FC Control block.
*       name                                Name of the controller.
*       capability                          Hardware capabilities.
*       speed                               USB Speeds supported.
*       base_address                        Base address in the memory map.
*       num_irq                             Number of IRQs supported by the
*                                           FC.
*       irq                                 List of supported IRQs.
*       dispatch                            Dispatch table of the FC.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the FC has been
*                                           initialized.
*       NU_NOT_PRESENT                      Indicates a configuration
*                                           problem because of which no
*                                           more USB Objects could be
*                                           created.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
*
**************************************************************************/
STATUS  _NU_USBF_HW_Create (NU_USBF_HW   *cb,
                            CHAR         *name,
                            UINT32        capability,
                            UINT8         speed,
                            VOID         *base_address,
                            UINT8         num_irq,
                            INT          *irq,
                            const VOID   *dispatch)
{
    STATUS  status;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(dispatch);
    NU_USB_PTRCHK(base_address);
    NU_USB_PTRCHK(name);
    NU_USB_PTRCHK(irq);

    /* Create the extended stack using base class method.    */
    status = _NU_USB_HW_Create ((NU_USB_HW *) cb,
                                &nu_usbf->controller_subsys,
                                name, speed, base_address,
                                num_irq, irq, dispatch);

    if (status != NU_SUCCESS)
    {
        return (status);
    }

    /* Initialize the control block. */
    cb->capability = capability;

    /* To start with Remote wakeup is disabled.  */
    cb->remote_wakeup = 0U;

    return (NU_SUCCESS);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Get_Capability
*
* DESCRIPTION
*
*       Retrieves the hardware capabilities.
*       The capability is a bitmap of the USB Chapter 9 specified
*       standard requests that the hardware is capable of processing
*       without any software intervention.
*
* INPUTS
*
*       cb                                  Hardware control block
*       capability_out                      Memory location where the
*                                           hardware capability is to be
*                                           stored.
*
* OUTPUTS
*
*       NU_SUCCESS
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
**************************************************************************/
STATUS  NU_USBF_HW_Get_Capability (NU_USBF_HW *cb,
                                   UINT32     *capability_out)
{
    STATUS status;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(capability_out);

    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_GET_CAPABILITY),
                          (VOID*)capability_out,
                          sizeof(UINT32));

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Set_Address
*
* DESCRIPTION
*
*       Sets the USB address of the given FC through the dispatch
*       table installed worker function for the FC.
*
* INPUTS
*
*       hw                                  FC for the new address.
*       address                             New address for the FC.
*
* OUTPUTS
*
*       NU_SUCCESS                          The service executed
*                                           successfully.
*       NU_USB_NOT_SUPPORTED                This service is not supported
*                                           by the FC.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
**************************************************************************/
STATUS  NU_USBF_HW_Set_Address (NU_USBF_HW *cb,
                                UINT8       address)
{
    STATUS  status;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);

    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_SET_ADDRESS),
                          (VOID*)&address,
                          sizeof(UINT8));


    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Get_Status
*
* DESCRIPTION
*
*       This function retrieves the status of the given function controller
*       and fills in the memory location pointed to by the status_out
*       parameter. The status is a bitmap formatted to be the same as
*       as the response to a GET_STATUS (DEVICE) standard request.
*
* INPUTS
*
*       hw                                  FC for which the capability is
*                                           required.
*       status_out                          Memory location where the
*                                           status data is to be placed.
*
* OUTPUTS
*
*       NU_SUCCESS                          The service executed
*                                           successfully.
*       NU_USB_NOT_SUPPORTED                This service is not supported
*                                           by the FC.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
**************************************************************************/
STATUS  NU_USBF_HW_Get_Status (NU_USBF_HW *cb,
                               UINT16     *status_out)
{
    STATUS  status;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(status_out);

    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_GET_DEV_STATUS),
                          (VOID*)status_out,
                          sizeof(UINT16));

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Get_Endpoint_Status
*
* DESCRIPTION
*
*       This function retrieves the status of the given endpoint
*       and fills in the memory location pointed to by the status_out
*       parameter. The status is a bitmap formatted to be the same as
*       as the response to a GET_STATUS (ENDPOINT) standard request.
*
* INPUTS
*
*       hw                                  FC containing the endpoint
*       bEndpointAddress                    Endpoint for which the status
*                                           is required.
*       status_out                          Memory location where the
*                                           status data is to be  placed.
*
* OUTPUTS
*
*       NU_SUCCESS                          The service could be executed
*                                           successfully.
*       NU_USB_INVLD_ARG                    Specified endpoint could not be
*                                           found.
*       NU_USB_NOT_SUPPORTED                This service is not supported
*                                           by the FC.
*
**************************************************************************/
STATUS  NU_USBF_HW_Get_Endpoint_Status (NU_USBF_HW *cb,
                                        UINT8       bEndpointAddress,
                                        UINT16     *status_out)
{
    STATUS      status;
    USB_EP_INFO ep_info;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(status_out);
    
    ep_info.interval            = 0;
    ep_info.load                = 0;
    ep_info.max_packet_size     = 0;
    ep_info.endp_sts            = 0;
    ep_info.function_addr       = 0;
    ep_info.endp_addr           = bEndpointAddress;
    ep_info.endp_attrib         = 0;
    ep_info.speed               = 0;
    ep_info.config_num          = 0;
    ep_info.intf_num            = 0;
    ep_info.alt_sttg            = 0;
    
    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_GET_ENDP_STATUS),
                          (VOID*)&ep_info,
                          sizeof(USB_EP_INFO));
    if ( status == NU_SUCCESS )
    {
        *status_out = ep_info.endp_sts;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Stall_Endpoint
*
* DESCRIPTION
*
*       This function stalls the given endpoint on the FC. Once the
*       stalling is complete, the endpoint returns STALL PIDs on the USB,
*       for every token sent by the Host. This continues till the endpoint
*       is explicitly un-stalled, either by the un-stall API or by the Host
*       through a CLEAR_FEATURE command.
*
*       If any transfers are currently pending on the endpoint, then they
*       will be canceled automatically.
*
*       Transfers submitted on stalled endpoints will be kept pending till
*       the endpoint is un-stalled.
*
* INPUTS
*
*       cb                                  FC which owns the endpoint.
*       bEndpointAddress                    Endpoint to be stalled.
*
* OUTPUTS
*
*       NU_SUCCESS                          The service could be executed
*                                           successfully.
*       NU_USB_INVLD_ARG                    Specified endpoint could not be
*                                           found.
*       NU_USB_NOT_SUPPORTED                This service is not supported
*                                           by the FC.
*
**************************************************************************/
STATUS  NU_USBF_HW_Stall_Endpoint (NU_USBF_HW *cb,
                                   UINT8       bEndpointAddress)
{
    STATUS      status;
    USB_EP_INFO ep_info;

    NU_USB_PTRCHK(cb);
    
    ep_info.interval            = 0;
    ep_info.load                = 0;
    ep_info.max_packet_size     = 0;
    ep_info.endp_sts            = 0;
    ep_info.function_addr       = 0;
    ep_info.endp_addr           = bEndpointAddress;
    ep_info.endp_attrib         = 0;
    ep_info.speed               = 0;
    ep_info.config_num          = 0;
    ep_info.intf_num            = 0;
    ep_info.alt_sttg            = 0;

    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_STALL_ENDP),
                          (VOID*)&ep_info,
                          sizeof(USB_EP_INFO));

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Unstall_Endpoint
*
* DESCRIPTION
*
*       This function un-stall the given endpoint on the FC. Once the
*       un-stalling is complete, the endpoint returns normal ACK / NAK
*       responses to the token sent on this endpoint by the Host.
*
*       If there are any transfers pending on the endpoint, they will be
*       resumed immediately after the un-stall process.
*
* INPUTS
*
*       cb                                  FC which owns the endpoint.
*       bEndpointAddress                    Endpoint to be un-stalled.
*
* OUTPUTS
*
*       NU_SUCCESS                          The service could be executed
*                                           successfully.
*       NU_USB_INVLD_ARG                    Specified endpoint could not be
*                                           found.
*       NU_USB_NOT_SUPPORTED                This service is not supported
*                                           by the FC.
*
**************************************************************************/
STATUS  NU_USBF_HW_Unstall_Endpoint (NU_USBF_HW *cb,
                                     UINT8       bEndpointAddress)
{
    STATUS      status;
    USB_EP_INFO ep_info;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    
    ep_info.interval            = 0;
    ep_info.load                = 0;
    ep_info.max_packet_size     = 0;
    ep_info.endp_sts            = 0;
    ep_info.function_addr       = 0;
    ep_info.endp_addr           = bEndpointAddress;
    ep_info.endp_attrib         = 0;
    ep_info.speed               = 0;
    ep_info.config_num          = 0;
    ep_info.intf_num            = 0;
    ep_info.alt_sttg            = 0;

    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_UNSTALL_ENDP),
                          (VOID*)&ep_info,
                          sizeof(USB_EP_INFO));

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_HW_Initialize
*
* DESCRIPTION
*
*       This function installs the USBF LISR for each vector of the USB
*       h/w.
*
* INPUTS
*
*       cb                                  Pointer to the NU_USBF_HW
*                                           control block.
*       stack                               Pointer to the control block of
*                                           the function stack.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_HW_Initialize (NU_USB_HW    *cb,
                                NU_USB_STACK *stack)
{
    UNSIGNED    i;
    STATUS      status = NU_SUCCESS;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(stack);

    /* Call base class Initialize. */
    status = _NU_USB_HW_Initialize (cb, stack);
    if (status != NU_SUCCESS)
    {
        return (status);
    }

    for (i = 0UL; i < cb->num_irq; i++)
    {
        status = USBF_HW_Register_LISR ((NU_USBF_HW *) cb, cb->irq[i]);
        if (status != NU_SUCCESS)
        {
            break;
        }
    }
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_HW_Uninitialize
*
* DESCRIPTION
*
*       This function un-installs the USBF LISR for each vector of the USB
*       h/w.
*
* INPUTS
*
*       cb                                  Pointer to the NU_USBF_HW
*                                           control block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_HW_Uninitialize (NU_USB_HW *cb)
{
    UNSIGNED    i;
    STATUS      status = NU_SUCCESS;
    /* Error checking.   */
    NU_USB_PTRCHK(cb);

    for (i = 0UL; i < cb->num_irq; i++)
    {
        status = USBF_HW_Deregister_LISR ((NU_USBF_HW *) cb, cb->irq[i]);
        if (status != NU_SUCCESS)
        {
            break;
        }
    }
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_HW_Delete
*
* DESCRIPTION
*
*       Un-initializes the hardware.
*
* INPUTS
*
*       cb                                  Hardware control block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Hardware un-initialized
*                                           successfully.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_HW_Delete (VOID *cb)
{
    /* Error checking.   */
    NU_USB_PTRCHK(cb);

    return (_NU_USB_HW_Delete (cb));
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Start_HNP
*
* DESCRIPTION
*
*       This function is called when b_hnp_enable request is received.
*       The HNP signaling will occur when bus is suspended.
*
* INPUTS
*
*       cb                                  FC which owns the endpoint.
*       port_id                             port on which HNP signaling
*                                               is to be initiated.
*
* OUTPUTS
*
*       NU_SUCCESS                          The service could be executed
*                                           successfully.
*       NU_USB_INVLD_ARG                    Specified port could not be
*                                           found.
*       NU_USB_NOT_SUPPORTED                This service is not supported
*                                           by the FC.
*
**************************************************************************/
STATUS  NU_USBF_HW_Start_HNP (NU_USBF_HW *cb,
                              UINT8       port_id)
{
    STATUS          status;
    USB_PORT_INFO   port_info;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);

    port_info.port_id       = port_id;
    port_info.hw_role_out   = 0;
    port_info.delay         = 0;

    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_START_HNP),
                          (VOID*)&port_info,
                          sizeof(USB_PORT_INFO));

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Is_Dual_Speed_Device
*
* DESCRIPTION
*
*       This function is called by stack to check if a USB function 
*       controller supports High speed.
*
* INPUTS
*
*       cb                                  Pointer to function controller
*                                           contol block.
*       is_dual_speed                       Boolean containing High speed
*                                           support TRUE/FALSE when 
*                                           function returns.
*
* OUTPUTS
*
*       NU_SUCCESS                          The service could be executed
*                                           successfully.
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*
**************************************************************************/
STATUS NU_USBF_HW_Is_Dual_Speed_Device( NU_USBF_HW  *cb,
                                        BOOLEAN     *is_dual_speed)

{
    STATUS  status;
    UINT8   speed;
    
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(is_dual_speed);
    
    status = NU_USB_HW_Get_Speed ((NU_USB_HW*)cb, &speed);
    if ( status == NU_SUCCESS )
    {
        if ( speed == USB_SPEED_FULL )
            *is_dual_speed = NU_FALSE;
        else if ( speed == USB_SPEED_HIGH )
            *is_dual_speed = NU_TRUE;
        else
            *is_dual_speed = NU_FALSE;
    }
    
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Get_EP0_Maxp
*
* DESCRIPTION
*
*       This function is called by stack to get maximum packet size of device depending upon 
*      speed. This function will return default EP0 MaxP if controller driver does not support this 
*      IOCTL.
*
* INPUTS
*
*       cb                                 Pointer to function controller.
*                                           contol block.
*       speed                           Speed of USB hardware.
*     maxp                  Maximum packet for this 'speed' returned by USB function 
*                           controller driver.
*
* OUTPUTS
*
*       NU_SUCCESS          The service could be executed
*                                           successfully.
*       NU_USB_INVLD_ARG         An input argument is invalid.
*
**************************************************************************/
STATUS NU_USBF_HW_Get_EP0_Maxp( NU_USBF_HW      *cb,
                                            UINT8           speed,
                                            UINT8           *maxp)
{
    STATUS      status;
    USB_EP_INFO ep_info;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);

    ep_info.interval            = 0;
    ep_info.load                = 0;
    ep_info.max_packet_size     = 0;
    ep_info.endp_sts            = 0;
    ep_info.function_addr       = 0;
    ep_info.endp_addr           = 0;
    ep_info.endp_attrib         = 0;
    ep_info.speed               = speed;
    ep_info.config_num          = 0;
    ep_info.intf_num            = 0;
    ep_info.alt_sttg            = 0;    

    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_GET_EP0_MAXP),
                          (VOID*)&ep_info,
                          sizeof(USB_EP_INFO));
    if ( status == NU_SUCCESS )
    {
        *maxp = ep_info.max_packet_size;
    }
    else
    {
        /* Set MaxP to default value of controller does not support this IOCTL. */
        if ( speed == USB_SPEED_FULL )
            *maxp = 8;
        else if ( speed == USB_SPEED_HIGH )
            *maxp = 64;

        status = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Acquire_Endp
*
* DESCRIPTION
*
*       This function is called by stack to to acquire an endpoint 
*       during creating configuration descriptor.
*
* INPUTS
*
*       cb                                  Pointer to function controller
*                                           contol block.
*       speed                               Speed at which device may 
*                                           operate.
*       attribs                             Endpoint attributes.
*       ep_addr                             Address of endpoint and 
*                                           direction.
*       config_num                          Configuration number.
*       intf_num                            Interface number.
*       alt_sttg                            Alternate setting.
*       ep_num                              Pointer to UINT8 containing 
*                                           endpoint number when function 
*                                           returns.
*       maxp                                Pointer to UINT16 containing 
*                                           maximum packet size when 
*                                           function returns.
*       interval                            Pointer to UINT8 containing 
*                                           interval when function returns.
*
* OUTPUTS
*
*       NU_SUCCESS                          The service could be executed
*                                           successfully.
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*
**************************************************************************/
STATUS NU_USBF_HW_Acquire_Endp( NU_USBF_HW  *cb,
                                UINT8       speed,
                                UINT8       attribs,
                                UINT8       ep_dir,
                                UINT8       config_num,
                                UINT8       intf_num,
                                UINT8       alt_sttg,
                                UINT8       *ep_num,
                                UINT16      *maxp,
                                UINT8       *interval)
{
    STATUS      status;
    USB_EP_INFO ep_info;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(ep_num);
    NU_USB_PTRCHK(maxp);
    NU_USB_PTRCHK(interval);
    
    ep_info.interval            = 0;
    ep_info.load                = 0;
    ep_info.max_packet_size     = 0;
    ep_info.endp_sts            = 0;
    ep_info.function_addr       = 0;
    ep_info.endp_addr           = (0x80 & ep_dir);
    ep_info.endp_attrib         = attribs;
    ep_info.speed               = speed;
    ep_info.config_num          = config_num;
    ep_info.intf_num            = intf_num;
    ep_info.alt_sttg            = alt_sttg;

    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_ACQUIRE_ENDP),
                          (VOID*)&ep_info,
                          sizeof(USB_EP_INFO));
    if ( status == NU_SUCCESS )
    {
        *ep_num     = ep_info.endp_addr;
        *maxp       = ep_info.max_packet_size;
        *interval   = ep_info.interval;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Release_Endp
*
* DESCRIPTION
*
*       This function is called by stack to to release an endpoint 
*       when device is unbound.
*
* INPUTS
*
*       cb                                  Pointer to function controller
*                                           contol block.
*       speed                               Speed at which device may 
*                                           operate.
*       ep_addr                             Address of endpoint and 
*                                           direction.
*       config_num                          Configuration number.
*       intf_num                            Interface number.
*       alt_sttg                            Alternate setting.
*
* OUTPUTS
*
*       NU_SUCCESS                          The service could be executed
*                                           successfully.
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*
**************************************************************************/
STATUS NU_USBF_HW_Release_Endp(NU_USBF_HW   *cb,
                                UINT8       speed,
                                UINT8       ep_addr,
                                UINT8       config_num,
                                UINT8       intf_num,
                                UINT8       alt_sttg)
{
    STATUS      status;
    USB_EP_INFO ep_info;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    
    ep_info.interval            = 0;
    ep_info.load                = 0;
    ep_info.max_packet_size     = 0;
    ep_info.endp_sts            = 0;
    ep_info.function_addr       = 0;
    ep_info.endp_addr           = ep_addr;
    ep_info.endp_attrib         = 0;
    ep_info.speed               = speed;
    ep_info.config_num          = config_num;
    ep_info.intf_num            = intf_num;
    ep_info.alt_sttg            = alt_sttg;

    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_RELEASE_ENDP),
                          (VOID*)&ep_info,
                          sizeof(USB_EP_INFO));

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Enable_Pullup
*
* DESCRIPTION
*
*       Enables pull-up on DP and make device ready for connection with 
*       USB host.
*
* INPUTS
*
*       cb                                  Pointer to function controller
*                                           contol block.
*
* OUTPUTS
*
*       NU_SUCCESS                          The service could be executed
*                                           successfully.
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*
**************************************************************************/
STATUS NU_USBF_HW_Enable_Pullup(NU_USBF_HW  *cb)
{
    STATUS      status;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    
    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_ENABLE_PULLUP),
                          NU_NULL,
                          0);

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_HW_Disable_Pullup
*
* DESCRIPTION
*
*       Disables pull-up on DP. After this host cant identify a device 
*       connection anymore.
*
* INPUTS
*
*       cb                                  Pointer to function controller
*                                           contol block.
*
* OUTPUTS
*
*       NU_SUCCESS                          The service could be executed
*                                           successfully.
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*
**************************************************************************/
STATUS NU_USBF_HW_Disable_Pullup(NU_USBF_HW *cb)
{
    STATUS      status;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    
    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_DISABLE_PULLUP),
                          NU_NULL,
                          0);

    return ( status );
}

/* Following functions should only be visible when stack is configured
 * for USB 3.0. */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/*************************************************************************
*
*   FUNCTION
*
*       NU_USBF_HW_Send_FuncWakeNotif
*
*   DESCRIPTION
*
*       Class driver or application can call this function to send a
*       function wake device notification to host. For more details about
*       USB function wake device notification please refer to section
*       8.5.6.1 of USB 3.0 specification.
*
*   INPUTS
*
*       cb                  - Pointer to NU_USBF_HW control block.
*
*       intf_num            - Interface number which wants to send a
*                             function device wakeup notification.
*
*   OUTPUTS
*
*       NU_SUCCESS          - Operation Completed Successfully. USB
*                             function wake notification is sent to host.
*
*       NU_USB_INVLD_ARG    - Indicates any of the input argument is 
*                             invalid.
*
*************************************************************************/
STATUS NU_USBF_HW_Send_FuncWakeNotif ( NU_USBF_HW *cb, UINT8 intf_num)
{
    STATUS      status;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    
    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_SEND_FUNCWAKENOTIF),
                          (VOID*)&intf_num,
                          sizeof(UINT8));

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USBF_HW_Set_Ux_Enable
*
*   DESCRIPTION
*
*       This function is called by function stack in the event of a 
*       Set/Clear Feature U1/U2 enable request. 
*       This function calls USB function  controller driver dispatch 
*       entry to enable/disable U1 or U2 link state entry.
*
*   INPUTS
*
*       cb                  - Pointer to NU_USBF_HW control block.
*
*       link_state          - Link state U1 or U2.
*
*       enable              - NU_TRUE:  Enable U1/U2 link state entry
                              NU_FALSE: Disable U1/U2 link state entry
*
*   OUTPUTS
*
*       NU_SUCCESS          - Operation Completed Successfully. U1 or U2
*                             entry is enabled/disabled in USB function 
*                             hardware.
*
*       NU_USB_INVLD_ARG    - Indicates any of the input argument is 
*                             invalid.
*
*************************************************************************/
STATUS NU_USBF_HW_Set_Ux_Enable (NU_USBF_HW *cb, UINT8 link_state, BOOLEAN enable)
{
    STATUS      status;
    int         ioctl;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    
    if ( link_state == USB_LINK_STATE_U0 )
        ioctl = NU_USBF_IOCTL_SET_FEATURE_U0_ENABLE;
    else if ( link_state == USB_LINK_STATE_U1 )
        ioctl = NU_USBF_IOCTL_SET_FEATURE_U1_ENABLE;
    else
        return ( NU_USB_INVLD_ARG );
    
    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + ioctl),
                          (VOID*)&enable,
                          sizeof(BOOLEAN));

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USBF_HW_Set_LTM_Enable
*
*   DESCRIPTION
*
*       This function is called by function stack in the event of a 
*       Set/Clear Feature LTM enable request. 
*       This function calls USB function  controller driver dispatch 
*       entry to enable/disable Latency Tolerance Messge generation.
*
*   INPUTS
*
*       cb                  - Pointer to NU_USBF_HW control block.
*
*       enable              - NU_TRUE:  Enable LTM generation.
*                              NU_FALSE: Disable LTM generation.
*
*   OUTPUTS
*
*       NU_SUCCESS          - Operation Completed Successfully. LTM 
*                             generation is enabled/disabled in USB
*                             function hardware.
*
*       NU_USB_INVLD_ARG    - Indicates any of the input argument is 
*                             invalid.
*
*************************************************************************/
STATUS NU_USBF_HW_Set_LTM_Enable (NU_USBF_HW *cb, BOOLEAN enable)
{
    STATUS      status;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    
    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_SET_LTM_ENABLE),
                          (VOID*)&enable,
                          sizeof(BOOLEAN));

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*   NU_USBF_HW_Is_LTM_Capable
*
*   DESCRIPTION
*
*      This function is called by USB function stack to check if USB super speed hardware is 
*     capable of generating LTM messages.
*
*   INPUTS
*
*       cb                  - Pointer to NU_USBF_HW control block.
*
*       is_ltm_capable - NU_TRUE:  HW can generate LTM.
*                              NU_FALSE: HW can't generate LTM.
*
*   OUTPUTS
*
*       NU_SUCCESS   - Operation Completed Successfully.
*       NU_USB_INVLD_ARG    - Indicates any of the input argument is 
*                             invalid.
*
*************************************************************************/
STATUS NU_USBF_HW_Is_LTM_Capable(NU_USBF_HW *cb, BOOLEAN *is_ltm_capable)
{
    STATUS      status;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(is_ltm_capable);
    
    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_CHECK_LTM_CAPABLE),
                          (VOID*)is_ltm_capable,
                          sizeof(BOOLEAN));

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*   NU_USBF_HW_Get_Supported_Speeds
*
*   DESCRIPTION
*
*      This function is called by USB function stack to get all supported speeds of super speed USB 
*    function hardware.
*
*   INPUTS
*
*       cb                  - Pointer to NU_USBF_HW control block.
*
*       supported_speeds - A bitmap of all supported speeds by USB function hardware in following 
*                       format.
*                       
*                       bit 0: Hardware support low speed operations.
*                       bit 1: Hardware support full speed operations.
*                       bit 2: Hardware support high speed operations.
*                       bit 3: Hardware support super speed operations.
*
*   OUTPUTS
*
*       NU_SUCCESS   - Operation Completed Successfully.
*       NU_USB_INVLD_ARG    - Indicates any of the input argument is 
*                             invalid.
*
*************************************************************************/
STATUS NU_USBF_HW_Get_Supported_Speeds(NU_USBF_HW *cb, UINT16 *supported_speeds)
{
    STATUS      status;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(supported_speeds);
    
    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_GET_SUPPORTED_SPEEDS),
                          (VOID*)supported_speeds,
                          sizeof(UINT16));

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*   NU_USBF_HW_Get_U1DevExitLat
*
*   DESCRIPTION
*
*      This function is called by USB function stack to get value of U1 device exit latency.
*
*   INPUTS
*
*       cb                  - Pointer to NU_USBF_HW control block.
*       u1devexitlat - Pointer to UINT8 containing value of U1 device exit latency when 
*                  function returns.
*
*   OUTPUTS
*
*       NU_SUCCESS   - Operation Completed Successfully.
*       NU_USB_INVLD_ARG    - Indicates any of the input argument is 
*                             invalid.
*
*************************************************************************/
STATUS NU_USBF_HW_Get_U1DevExitLat(NU_USBF_HW *cb, UINT8 *u1devexitlat)
{
    STATUS      status;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(u1devexitlat);
    
    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_GET_U1DEVEXITLAT),
                          (VOID*)u1devexitlat,
                          sizeof(UINT8));

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*   NU_USBF_HW_Get_U2DevExitLat
*
*   DESCRIPTION
*
*      This function is called by USB function stack to get value of U2 device exit latency.
*
*   INPUTS
*
*       cb                  - Pointer to NU_USBF_HW control block.
*       u2devexitlat - Pointer to UINT16 containing value of U2 device exit latency when 
*                  function returns.
*
*   OUTPUTS
*
*       NU_SUCCESS   - Operation Completed Successfully.
*       NU_USB_INVLD_ARG    - Indicates any of the input argument is 
*                             invalid.
*
*************************************************************************/
STATUS NU_USBF_HW_Get_U2DevExitLat(NU_USBF_HW *cb, UINT16 *u2devexitlat)
{
    STATUS      status;

    /* Error checking.   */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(u2devexitlat);
    
    /* Invoke the controller function through IOCTL. */
    status = DVC_Dev_Ioctl(((NU_USB_HW *)cb)->dv_handle,
                          (((NU_USB_HW *)cb)->ioctl_base_addr + NU_USBF_IOCTL_GET_U2DEVEXITLAT),
                          (VOID*)u2devexitlat,
                          sizeof(UINT16));

    return ( status );
}

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
#endif /* USBF_HW_EXT_C */
/* ======================  End Of File  ================================ */
