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
*        nu_usb_hw_ext.c
*
* COMPONENT
*
*        Nucleus USB Software : Hardware layer.
*
* DESCRIPTION
*
*        This file contains the implementation of the NU_USB_HW services.
*
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*       NU_USB_HW_Close_Pipe            -closes a pipe in hardware
*       NU_USB_HW_Disable_Interrupts    -Disables interrupts on hw
*       NU_USB_HW_Enable_Interrupts     -Enables interrupts on hw.
*       NU_USB_HW_Flush_Pipe            -flushes a pipe in hardware
*       NU_USB_HW_Get_Speed             -gives the speed of controller.
*       NU_USB_HW_Get_Stack             -Returns associated stack control
*                                       block.
*       NU_USB_HW_ISR                   -Interrupt service routine of HW.
*       NU_USB_HW_Initialize            -Initialize Hardware controller.
*       NU_USB_HW_Modify_Pipe           -modify pipe's attributes.
*       NU_USB_HW_Open_Pipe             -opens a pipe in hardware
*       NU_USB_HW_Submit_IRP            -Submit an IRP on a pipe in HW.
*       NU_USB_HW_Uninitialize          -Stops hardware controller.
*       _NU_USB_HW_Create               -Protected Constructor used by
*                                       extenders to initialize themselves.
*       _NU_USB_HW_Initialize          - Initialize the ptr to stack field in the
*                                         control block.
*       _NU_USB_HW_Delete              - Deletes a HW driver.
*      NU_USB_HW_Open_SS_Pipe           -This function is used to init
*                                        SS endpoint on the HW.
*      NU_USB_HW_Modify_SS_Pipe         -This function is used to modify 
*                                        the charactristics of SS endpoint.
*      NU_USB_HW_Update_Power_Mode     -This API will be called by stack
*                                        to update the power mode of
*                                        hardware.
*      NU_USB_HW_Update_BELT_Value      -This API is called by stack to
*                                        update the HW BELT value.
*
*
* DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_HW_EXT_C
#define USB_HW_EXT_C

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb.h"

/* HSET Test Mode TEST Packet data */
UINT8 NU_USB_HW_Test_Packet[NU_USB_HW_TEST_PACKET_SIZE] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
    0xee, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xbf, 0xdf,
    0xef, 0xf7, 0xfb, 0xfd, 0xfc, 0x7e, 0xbf, 0xdf,
    0xef, 0xf7, 0xfb, 0xfd, 0x7e
};

/*************************************************************************
* FUNCTION
*       _NU_USB_HW_Create
*
* DESCRIPTION
*   This function creates the NU_USB_HW portion of the control block.It
* initializes various fields of the control block.This is invoked from the
* create function of the more specialized variant of NU_USB_HW.
*
* INPUTS
*   cb           - Pointer to NU_USB_HW control block or a derivative of it.
*   subsys       - Pointer to sub system control block that this variant
*                  belongs to.
*   name         - 7 character null terminated string naming this h/w.
*   speed        - USB speed of the controller
*   base_address - base address for the register set of the h/w.
*   num_irq      - Number of interrupt vectors associated with the h/w.
*   irq          - array of interrupt vector numbers. size of the array is
*                  num_irq.
*   dispatch     - Pointer to dispatch table.
*
* OUTPUTS
*    NU_SUCCESS            Indicates successful completion of the service.
*    NU_USB_NOT_PRESENT Indicates that the maximum number of control blocks
*                       that could be created in the sub system has
*                       exceeded.
*
*************************************************************************/
STATUS _NU_USB_HW_Create (NU_USB_HW * cb,
                          NU_USB_SUBSYS * subsys,
                          CHAR * name,
                          UINT8 speed,
                          VOID *base_address,
                          UINT8 num_irq,
                          INT * irq,
                          const VOID *dispatch)
{
    STATUS status;
    UINT8 i;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(subsys);
    NU_USB_PTRCHK(name);
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(base_address);
    NU_USB_PTRCHK(irq);
    NU_USB_PTRCHK(dispatch);
    NU_USB_PTRCHK(num_irq);

    /* Create the extended stack using base class method    */

    status = _NU_USB_Create ((NU_USB *) cb, subsys, name, dispatch);

    if (status != NU_SUCCESS)
        return (status);

    /* Initialize the control block */
    cb->base_address    = base_address;
    cb->speed           = speed;
    cb->num_irq         = num_irq;

    /* Copy the IRQs    */
    i = 0;
    while (i < num_irq)
    {
        cb->irq[i] = irq[i];
        i++;
    }

    cb->stack_cb = NU_NULL;

    return (NU_SUCCESS);

}

/*************************************************************************
* FUNCTION
*        _NU_USB_HW_Delete
*
* DESCRIPTION
*       This function deletes a specified hardware driver.
*
* INPUTS
*       cb      pointer to hardware driver control block.
*
* OUTPUTS
*       NU_SUCCESS          hardware driver deleted successfully
*
*************************************************************************/
STATUS _NU_USB_HW_Delete (VOID *cb)
{

    NU_USB_PTRCHK(cb);

    return (_NU_USB_Delete (cb));

}

/*************************************************************************
* FUNCTION
*     NU_USB_HW_Initialize
*
* DESCRIPTION
*    This function initializes the h/w. It invokes the function pointer
* thats installed in the dispatch table by the derivative of NU_USB_HW.
*
* INPUTS
*   cb       Pointer to the h/w control block.
*   stack_cb Pointer to the control block of the stack that has invoked
*            this function.
*
* OUTPUTS
*    NU_SUCCESS               Indicates successful completion of the service.
*    NU_USB_NOT_SUPPORTED  Indicates that this h/w derivative doesn't need
*                          initialization.
*    other return values are implementation specific to the derivative of
*    NU_USB_HW.
*
*************************************************************************/
STATUS NU_USB_HW_Initialize (NU_USB_HW * cb,
                             NU_USB_STACK * stack_cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(stack_cb);

    status = DVC_Dev_Ioctl(cb->dv_handle,
                           (cb->ioctl_base_addr + NU_USB_IOCTL_INITIALIZE),
                           (VOID*) stack_cb,
                           sizeof(NU_USB_STACK));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*    NU_USB_HW_Uninitialize
*
* DESCRIPTION
*     This function stops the h/w. The function pointer installed in the
* dispatch table by the derivative of the NU_USB_HW is invoked by this
* function.
*
* INPUTS
*   cb Pointer to control block of the derivative of NU_USB_HW.
*
* OUTPUTS
*    NU_SUCCESS               Indicates successful completion of the service.
*    NU_USB_NOT_SUPPORTED  Indicates that this h/w derivative doesn't have
*                          un-initialization.
*    other return values are implementation specific to the derivative of
*    NU_USB_HW.
*
*************************************************************************/
STATUS NU_USB_HW_Uninitialize (NU_USB_HW * cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_UNINITIALIZE),
                        NU_NULL,
                        0);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*    NU_USB_HW_Get_Speed
*
* DESCRIPTION
*   This function returns the speed of the controller.
*
* INPUTS
*   cb
*   speed_out Pointer to the variable to hold the speed of the controller.
*             Speed is one of the following:
*                         USB_SPEED_UNKNOWN, USB_SPEED_LOW, USB_SPEED_FULL,
*                         USB_SPEED_HIGH
*
* OUTPUTS
*    NU_SUCCESS            Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_HW_Get_Speed (NU_USB_HW * cb,
                            UINT8 *speed_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(speed_out);

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_GET_SPEED),
                        (VOID*) speed_out,
                        sizeof(UINT8));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*    NU_USB_HW_ISR
*
* DESCRIPTION
*     This function is the interrupt handler for the h/w. The function
* installed in the dispatch table for ISR is invoked.
*
* INPUTS
*   cb   Pointer to the control block of the derivative of NU_USB_HW.
*   irq  interrupt vector number.
*
* OUTPUTS
*    NU_SUCCESS               Indicates successful completion of the service.
*    NU_USB_NOT_SUPPORTED  Indicates that this h/w derivative doesn't have
*                          interrupt handler.
*
*************************************************************************/
STATUS NU_USB_HW_ISR (NU_USB_HW * cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_EXECUTE_ISR),
                        NU_NULL,
                        0);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*    NU_USB_HW_Get_Stack
*
* DESCRIPTION
*    This function returns the pointer to the associated stack control block.
*
* INPUTS
*   cb        Pointer to control block of derivative of NU_USB_HW.
*   stack_cb  Pointer to memory location to hold the pointer to the stack
*             control block.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_HW_Get_Stack (NU_USB_HW * cb,
                            NU_USB_STACK ** stack_cb)
{
    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE_ISR();

    if((cb == NU_NULL) || (stack_cb == NU_NULL))
    {
        /* Switch back to user mode. */
        NU_USER_MODE_ISR();
        return NU_USB_INVLD_ARG;
    }
    else
    {
        *stack_cb = cb->stack_cb;
    }

    /* Switch back to user mode. */
    NU_USER_MODE_ISR();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*     NU_USB_HW_Submit_IRP
*
* DESCRIPTION
*    This function submits an IRP to the h/w for transfer over USB. On host
* s/w, this IRP may be destined for any endpoint on any device connected to
* the host, including the root hub.  On function s/w the IRP is destined to
* the host through any of the endpoints, the device supports. The completion of the
* IRP transfer is  notified by the h/w by invoking the callback function
* installed in the IRP (through NU_USB_IRP_Set_Callback function). The status field
* in the IRP (retrieved through NU_USB_IRP_Get_Status and set using
* NU_USB_IRP_Set_Status) conveys the completion status of the IRP. The following
* are the possible values of the status field.
*    NU_SUCCESS           -  IRP transfer is successful.
*    NU_USB_CRC_ERR       -  transfer encountered CRC error.
*    NU_USB_BITSTUFF_ERR  -  transfer encountered bit stuffing error.
*    NU_USB_TOGGLE_ERR    -  transfer encountered data0/data 1 toggle
*                            error.
*    NU_USB_STALL_ERR     -  IRP received a stall handshake.
*    NU_USB_NO_RESPONSE   -  No response received for the token.
*    NU_USB_INVLD_PID     -  packet id sent out by the h/w is invalid.
*    NU_USB_UNEXPECTED_PID-  unexpected packet id received.
*    NU_USB_DATA_OVERRUN  -  The amount of data returned by the endpoint
*                            exceeded either the size of the maximum data packet
*                            allowed from the endpoint or the remaining buffer
*                            size.
*    NU_USB_DATA_UNDERRUN -  The endpoint returned less than MaximumPacketSize
*                            and that amount was not sufficient to fill the
*                            specified buffer.
*    NU_USB_BFR_OVERRUN   -  H/W received data from endpoint
*                            faster than it could be written to system memory.
*
*    NU_USB_BFR_UNDERRUN  -   H/W could not retrieve data from system memory
*                             fast enough to keep up with data USB data rate.
*    NU_USB_NOT_ACCESSED  -   H/W did not attempt the transfer.
*    NU_USB_EP_HALTED     -   Endpoint is halted due to unknown error.
*    NU_USB_IRP_CANCELLED -   Endpoint is closed while the transfer is
*                             pending or some one invoked flush pipe while this
*                             IRP is pending transfer.
*    NU_USB_UNKNOWN_ERR   -  Unknown error during the transfer.
*
* INPUTS
*   cb               Pointer to the control block of the derivative of
*                    NU_USB_HW.
*   irp              Pointer to the IRP control block.
*   function_addr    USB function address to which the IRP has to be sent.
*                    On function s/w, the function_addr is filled as 0, as the
*                    IRP is always addressed to the host. On host s/w
*                    function address of root hub is USB_ROOT_HUB.
*   bEndpointAddress USB endpoint address field of the endpoint descriptor.
*                    It identifies the endpoint and the direction of the
*                    transfer.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*       NU_USB_NOT_SUPPORTED    Indicates hardware don't provide this
*                               feature.
*
*************************************************************************/
STATUS NU_USB_HW_Submit_IRP (NU_USB_HW * cb,
                             NU_USB_IRP * irp,
                             UINT8 function_addr,
                             UINT8 bEndpointAddress)
{
    STATUS status;
    USB_IRP_INFO irp_info;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb); 
    NU_USB_PTRCHK_RETURN(irp);

    irp_info.irp        = irp;
    irp_info.endp_addr  = bEndpointAddress;
    irp_info.func_addr  = function_addr;

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_IO_REQUEST),
                        (VOID*) &irp_info,
                        sizeof(USB_IRP_INFO));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*    NU_USB_HW_Flush_Pipe
*
* DESCRIPTION
*       This function invokes the Flush_Pipe function installed in the
*       dispatch table. This call to this function flushes all pending
*       IRP's on a pipe and make it empty.
*
* INPUTS
*       cb                  pointer to NU_USB_HW control block.
*       function_addr       Device address on the hardware.
*       bEndpointAddress    address of endpoint on the device.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*       NU_USB_NOT_SUPPORTED    Indicates hardware don't provide this
*                               feature.
*
*************************************************************************/
STATUS NU_USB_HW_Flush_Pipe (NU_USB_HW * cb,
                             UINT8 function_addr,
                             UINT8 bEndpointAddress)
{
    STATUS      status;
    USB_EP_INFO ep_info;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    ep_info.interval            = 0;
    ep_info.load                = 0;
    ep_info.max_packet_size     = 0;
    ep_info.endp_sts            = 0;
    ep_info.function_addr       = function_addr;
    ep_info.endp_addr           = bEndpointAddress;
    ep_info.endp_attrib         = 0;
    ep_info.speed               = 0;
    ep_info.config_num          = 0;
    ep_info.intf_num            = 0;
    ep_info.alt_sttg            = 0;

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_FLUSH_PIPE),
                        (VOID*) (&ep_info),
                        sizeof(USB_EP_INFO));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USB_HW_Enable_Interrupts
*
* DESCRIPTION
*       This function disables the interrupts on hardware
*
* INPUTS
*       cb      Pointer to NU_USB_HW control block.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*       NU_USB_NOT_SUPPORTED    Indicates hardware don't provide this
*                               feature.
*
*************************************************************************/
STATUS NU_USB_HW_Enable_Interrupts (NU_USB_HW * cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_ENABLE_INT),
                        NU_NULL,
                        0);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USB_HW_Disable_Interrupts
*
* DESCRIPTION
*       This function Enables hardware controller interrupts.
*
* INPUTS
*       cb      Pointer to NU_USB_HW control block.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*       NU_USB_NOT_SUPPORTED    Indicates hardware don't provide this
*                               feature.
*
*************************************************************************/
STATUS NU_USB_HW_Disable_Interrupts (NU_USB_HW * cb)
{
    STATUS status;

    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE_ISR();

    NU_USB_PTRCHK_RETURN(cb);

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_DISABLE_INT),
                        NU_NULL,
                        0);

    NU_USER_MODE_ISR();

    return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USB_HW_Open_Pipe
*
* DESCRIPTION
*       This function is used to Initialize a pipe on the hardware
*       controller. Once a pipe is initialized it can offer its services.
*
* INPUTS
*       cb                      pointer to NU_USB_HW control block
*       function_addr           address of device on hardware.
*       bEndpointAddress        address of endpoint on device.
*       bmEndpointAttributes    Endpoint Characteristics.
*       speed                   Speed of data transmission on Endpoint.
*       wMaxPacketSize          Maximum packet size endpoint can support.
*       interval                Polling interval on endpoint.
*       load                    data payload of endpoint/pipe.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion.
*       NU_USB_NOT_SUPPORTED    Indicates Feature is not supported in
*                               hardware.
*       NU_USB_INVLD_ARG        Indicates that one of the arguments is invalid.
*       NU_UNAVAILABLE          Indicates that a internal semaphore is
*                               unavailable.
*
*************************************************************************/
STATUS NU_USB_HW_Open_Pipe (NU_USB_HW * cb,
                            UINT8 function_addr,
                            UINT8 bEndpointAddress,
                            UINT8 bmEndpointAttributes,
                            UINT8 speed,
                            UINT16 wMaxPacketSize,
                            UINT32 interval,
                            UINT32 load)
{
    STATUS          status;
    USB_EP_INFO     ep_info;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    ep_info.interval            = interval;
    ep_info.load                = load;
    ep_info.max_packet_size     = wMaxPacketSize;
    ep_info.endp_sts            = 0;
    ep_info.function_addr       = function_addr;
    ep_info.endp_addr           = bEndpointAddress;
    ep_info.endp_attrib         = bmEndpointAttributes;
    ep_info.speed               = speed;
    ep_info.config_num          = 0;
    ep_info.intf_num            = 0;
    ep_info.alt_sttg            = 0;

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_OPEN_PIPE),
                        (VOID*) (&ep_info),
                        sizeof(USB_EP_INFO));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USB_HW_Close_Pipe
*
* DESCRIPTION
*       This function is used to close an opened pipe on a device. Once the
*       pipe is closed it can't provide any further services.
*
* INPUTS
*       cb                  pointer to NU_USB_HW control block.
*       function_addr       Address of device in the hardware.
*       bEndpointAddress    Address of Endpoint on the device.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion.
*       NU_USB_NOT_SUPPORTED    Indicates Feature is not supported in
*                               hardware.
*       NU_USB_INVLD_ARG        Indicates that pipe or HW pointers passed are NU_NULL.
*       NU_UNAVAILABLE          Indicates that a internal semaphore is
*                               unavailable.
*
*************************************************************************/
STATUS NU_USB_HW_Close_Pipe (NU_USB_HW * cb,
                             UINT8 function_addr,
                             UINT8 bEndpointAddress)
{
    STATUS      status;
    USB_EP_INFO ep_info;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    ep_info.interval            = 0;
    ep_info.load                = 0;
    ep_info.max_packet_size     = 0;
    ep_info.endp_sts            = 0;
    ep_info.function_addr       = function_addr;
    ep_info.endp_addr           = bEndpointAddress;
    ep_info.endp_attrib         = 0;
    ep_info.speed               = 0;
    ep_info.config_num          = 0;
    ep_info.intf_num            = 0;
    ep_info.alt_sttg            = 0;

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_CLOSE_PIPE),
                        (VOID*) (&ep_info),
                        sizeof(USB_EP_INFO));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*    NU_USB_HW_Modify_Pipe
*
* DESCRIPTION
*       This function is used to modify the characteristics of a pipe.
*
* INPUTS
*       cb                      pointer to NU_USB_HW control block.
*       function_addr           function address of the device.
*       bEndpointAddress        address of endpoint on the device.
*       bmEndpointAttributes    Endpoint Attributes.
*       wMaxPacketSize          Maximum packet size of endpoint.
*       interval                Polling interval of endpoint.
*       load                    Data payload supported on endpoint.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion.
*       NU_USB_NOT_SUPPORTED    Indicates hardware don't support Pipe
*                               attributes changes.
*       NU_USB_INVLD_ARG        Indicates one or more arguments are
*                               invalid.
*       NU_NOT_PRESENT          Indicates endpoint not present.
*
*************************************************************************/
STATUS NU_USB_HW_Modify_Pipe (NU_USB_HW * cb,
                              UINT8 function_addr,
                              UINT8 bEndpointAddress,
                              UINT8 bmEndpointAttributes,
                              UINT16 wMaxPacketSize,
                              UINT32 interval,
                              UINT32 load)
{
    STATUS          status;
    USB_EP_INFO     ep_info;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    ep_info.interval            = interval;
    ep_info.load                = load;
    ep_info.max_packet_size     = wMaxPacketSize;
    ep_info.function_addr       = function_addr;
    ep_info.endp_addr           = bEndpointAddress;
    ep_info.endp_attrib         = bmEndpointAttributes;
    ep_info.speed               = 0;
    ep_info.config_num          = 0;
    ep_info.intf_num            = 0;
    ep_info.alt_sttg            = 0;

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_MODIFY_PIPE),
                        (VOID*) (&ep_info),
                        sizeof(USB_EP_INFO));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*        _NU_USBH_HW_Initialize
*
* DESCRIPTION
*
*       Initializes the control block.
*
* INPUTS
*
*       cb      HW Control block
*       stack   Stack object associated with the hw
*
* OUTPUTS
*
*       NU_SUCCESS      indicates successful initialization of the hw
*
*************************************************************************/
STATUS _NU_USB_HW_Initialize (NU_USB_HW * cb,
                              NU_USB_STACK * stack)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(stack);

    cb->stack_cb = stack;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*        NU_USB_HW_Get_Role
*
* DESCRIPTION
*
*       Retrieves the current (OTG) role of the hardware on the given port.
*       The role is a UINT8 bitmap, each bit explained as given below:
*
*       This function must be supported by those HW that have a MiniAB
*       port. If port_id doesn't correspond to a MiniAB port, then this
*       function must return NU_USB_NOT_SUPPORTED;
*
*       b0 : Indicates if a plug is inserted in the Mini-AB port.
*            b0 = 1, Indicates that there is a plug in the port,
*            b0 = 0, Indicates an unconnected port,
*
*       b1 : Indicates the type of plug inserted.
*            b1 = 1, indicates a Mini-A plug
*            b1 = 0, indicates a Mini-B plug
*
*       b2 : Indicates current role of the dual role device
*            b2 = 1, indicates a Host role
*            b2 = 0, indicates a device role
*
* INPUTS
*
*       cb          HW Control block
*       port_id     Port identifier on which the OTG status is required.
*       role_out    Location where the current OTG role is filled in.
*
* OUTPUTS
*
*       NU_SUCCESS      indicates successful completion of the service
*       NU_USB_NOT_SUPPORTED    indicates that the HW doesn't support this
*                               service.
*
*************************************************************************/
STATUS NU_USB_HW_Get_Role (NU_USB_HW * cb,
                            UINT8 port_id,
                            UINT8 *role_out)
{
    STATUS          status;
    USB_PORT_INFO   port_info;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(role_out);

    port_info.port_id       = port_id;
    port_info.hw_role_out   = 0;
    port_info.delay         = 0;

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_GET_ROLE),
                        (VOID*) (&port_info),
                        sizeof(USB_PORT_INFO));
    if ( status == NU_SUCCESS )
    {
        *role_out = port_info.hw_role_out;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*    NU_USB_HW_Start_Session
*
* DESCRIPTION
*    This function starts an OTG session on the given port of the hardware.
*
* INPUTS
*   cb        Pointer to control block of derivative of NU_USB_HW.
*   port_id   port identifier where the OTG device is connected.
*   delay     time in ms for which Start session will be attempted.
*
* OUTPUTS
*
*       NU_SUCCESS  on success when a session is started.
*       NU_USB_BUSY Session already active.
*       NU_USB_NOT_SUPPORTED    This function is not supported by HW
*       NU_USB_INVLD_ARG        This function is not supported in
*                               current mode
*
*************************************************************************/
STATUS NU_USB_HW_Start_Session (NU_USB_HW * cb, UINT8 port_id, UINT16 delay)
{
    STATUS          status;
    USB_PORT_INFO   port_info;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    port_info.port_id       = port_id;
    port_info.hw_role_out   = 0;
    port_info.delay         = delay;

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_START_SESSION),
                        (VOID*) (&port_info),
                        sizeof(USB_PORT_INFO));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*    NU_USB_HW_End_Session
*
* DESCRIPTION
*    This function starts an OTG session on the given port of the hardware.
*
* INPUTS
*   cb        Pointer to control block of derivative of NU_USB_HW.
*   port_id   port identifier where the OTG device is connected
*
* OUTPUTS
*
*       NU_SUCCESS  on success when a session is started.
*       NU_USB_BUSY Session already active.
*       NU_USB_NOT_SUPPORTED    This function is not supported by HW
*       NU_USB_INVLD_ARG        This function is not supported in
*                               current mode
*
*************************************************************************/
STATUS NU_USB_HW_End_Session (NU_USB_HW * cb, UINT8 port_id)
{
    STATUS          status;
    USB_PORT_INFO   port_info;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    port_info.port_id       = port_id;
    port_info.hw_role_out   = 0;
    port_info.delay         = 0;

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_END_SESSION),
                        (VOID*) (&port_info),
                        sizeof(USB_PORT_INFO));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*        NU_USB_HW_Notify_Role_Switch
*
* DESCRIPTION
*
*       This function registers the callback for notifying any
*       role-switching that happens on the Host Hardware
*
* INPUTS
*
*   cb               -  Pointer to the NU_USB_HW control block.
*   role_switch_cb   -  Callback to be invoked on a role-switch
*
* OUTPUTS
*
*   NU_SUCCESS                Indicates successful completion of the service.
*   NU_USB_NOT_SUPPORTED    Such notifications are not supported on this
*                           HW
*
*************************************************************************/
STATUS NU_USB_HW_Notify_Role_Switch (NU_USB_HW *hw,
        NU_USB_HW_ROLESWITCH_CALLBACK role_switch_cb)
{
    STATUS              status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(hw);
    NU_USB_PTRCHK_RETURN(hw->dv_handle);

    status = DVC_Dev_Ioctl(hw->dv_handle,
                        (hw->ioctl_base_addr + NU_USB_IOCTL_NOTIF_ROLE_SWITCH),
                        (VOID*) role_switch_cb,
                        sizeof(NU_USB_HW_ROLESWITCH_CALLBACK));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*        NU_USB_HW_Test_Mode
*
* DESCRIPTION
*
*       This function puts the controller into a test mode for certification
*
* INPUTS
*
*   cb               -  Pointer to the NU_USB_HW control block.
*   mode             -  Test mode to place the controller into
*
* OUTPUTS
*
*   NU_SUCCESS                Indicates successful completion of the service.
*   NU_USB_NOT_SUPPORTED    Such notifications are not supported on this
*                           HW
*
*************************************************************************/
STATUS NU_USB_HW_Test_Mode(NU_USB_HW *hw, UINT8 mode)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(hw);
    NU_USB_PTRCHK_RETURN(hw->dv_handle);

#if (USB_TEST_MODE_SUPPORT == NU_TRUE)
    status = DVC_Dev_Ioctl(hw->dv_handle,
                        (hw->ioctl_base_addr + NU_USB_IOCTL_TEST_MODE),
                        &mode,
                        sizeof(UINT8));
#else
    status = NU_USB_NOT_SUPPORTED;
#endif

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}


/*************************************************************************
* FUNCTION
*
*        NU_USB_HW_Request_Power_Down_Mode
*
* DESCRIPTION
*
*      USB stack may call this function to request a power down mode in case of device 
*      disconnection. This IOCTL only request the hardware to go to power down mode, 
*      actual decision of going to power down mode is solely at hardware controller's own 
*      discretion.
*
* INPUTS
*
*   cb               -  Pointer to the NU_USB_HW control block.
*
* OUTPUTS
*
*   NU_SUCCESS                Indicates successful completion of the service.
*   NU_USB_NOT_SUPPORTED    Such notifications are not supported on this
*                           HW
*
*************************************************************************/
STATUS NU_USB_HW_Request_Power_Down_Mode (NU_USB_HW *hw)
{
    STATUS              status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(hw);
    NU_USB_PTRCHK_RETURN(hw->dv_handle);

    status = DVC_Dev_Ioctl(hw->dv_handle,
                        (hw->ioctl_base_addr + NU_USB_IOCTL_REQ_POWER_DOWN),
                        NU_NULL,
                        0);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}


/*************************************************************************/

/* Following functions should only be visible when stack is configured
 * for Super Speed USB (USB 3.0). */

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/*************************************************************************
* FUNCTION
*
*		NU_USB_HW_Open_SS_Pipe 
*                                                                      
* DESCRIPTION
*
*       This function is used to Initialize a Super Speed pipe on the 
*		hardware controller. Once a pipe is initialized it can offer its 
*		services.
*                                                                       
* INPUTS
*
*       cb                      pointer to NU_USB_HW control block
*       function_addr           address of device on hardware.
*       bEndpointAddress        address of endpoint on device.
*       bmEndpointAttributes    Endpoint Characteristics.
*       speed                   Speed of data transmission on Endpoint.
*       wMaxPacketSize          Maximum packet size endpoint can support.
*       interval                Polling interval on endpoint.
*       load                    data payload of endpoint/pipe.
*		bMaxBurst				Maximum supported burst by endpoint.
*		bmSSEndpCompAttrib		bmAttributes field of SuperSpeed endpoint 
*								companion descriptor.
*                                                                      
* OUTPUTS     
*
*       NU_SUCCESS              Indicates successful completion.
*       NU_USB_NOT_SUPPORTED    Indicates Feature is not supported in
*                               hardware.
*       NU_USB_INVLD_ARG        Indicates that one of the arguments is invalid.
*       NU_UNAVAILABLE          Indicates that a internal semaphore is 
*                               unavailable.   
*
*************************************************************************/
STATUS NU_USB_HW_Open_SS_Pipe (NU_USB_HW * cb,
                            	UINT8 function_addr,
                            	UINT8 bEndpointAddress,
                            	UINT8 bmEndpointAttributes,
                            	UINT8 speed,
                            	UINT16 wMaxPacketSize,
                            	UINT32 interval,
	                         	UINT32 load,
                            	UINT8 bMaxBurst,
                            	UINT8 bmSSEndpCompAttrib,
								UINT16 wBytesPerInterval)
{
    STATUS          status;
    USB_EP_INFO     ep_info;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    ep_info.interval            = interval;
    ep_info.load                = load;
    ep_info.max_packet_size     = wMaxPacketSize;
    ep_info.function_addr       = function_addr;
    ep_info.endp_addr           = bEndpointAddress;
    ep_info.endp_attrib         = bmEndpointAttributes;
    ep_info.speed               = speed;
    ep_info.config_num          = 0;
    ep_info.intf_num            = 0;
    ep_info.alt_sttg            = 0;
    ep_info.max_burst           = bMaxBurst;
    ep_info.ep_comp_attrib      = bmSSEndpCompAttrib;
    ep_info.bytes_per_interval  = wBytesPerInterval;

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_OPEN_SS_PIPE),
                        (VOID*) (&ep_info),
                        sizeof(USB_EP_INFO));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*		NU_USB_HW_Modify_SS_Pipe 
*                                                                      
* DESCRIPTION
*
*       This function is used to modify the characteristics of a Super 
*		Speed pipe.
*                                                                       
* INPUTS
*
*       cb                      pointer to NU_USB_HW control block
*       function_addr           address of device on hardware.
*       bEndpointAddress        address of endpoint on device.
*       bmEndpointAttributes    Endpoint Characteristics.
*       speed                   Speed of data transmission on Endpoint.
*       wMaxPacketSize          Maximum packet size endpoint can support.
*       interval                Polling interval on endpoint.
*       load                    data payload of endpoint/pipe.
*		bMaxBurst				Maximum supported burst by endpoint.
*		bmSSEndpCompAttrib		bmAttributes field of SuperSpeed endpoint 
*								companion descriptor.
*                                                                      
* OUTPUTS     
*
*       NU_SUCCESS              Indicates successful completion.
*       NU_USB_NOT_SUPPORTED    Indicates Feature is not supported in
*                               hardware.
*       NU_USB_INVLD_ARG        Indicates that one of the arguments is invalid.
*       NU_UNAVAILABLE          Indicates that a internal semaphore is 
*                               unavailable.   
*
*************************************************************************/
STATUS NU_USB_HW_Modify_SS_Pipe (NU_USB_HW * cb,
                            	UINT8 function_addr,
                            	UINT8 bEndpointAddress,
                            	UINT8 bmEndpointAttributes,
                            	UINT16 wMaxPacketSize,
                            	UINT32 interval,
                            	UINT32 load,
                            	UINT8 bMaxBurst,
                            	UINT8 bmSSEndpCompAttrib)
{
    STATUS          status;
    USB_EP_INFO     ep_info;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    ep_info.interval            = interval;
    ep_info.load                = load;
    ep_info.max_packet_size     = wMaxPacketSize;
    ep_info.function_addr       = function_addr;
    ep_info.endp_addr           = bEndpointAddress;
    ep_info.endp_attrib         = bmEndpointAttributes;
    ep_info.speed               = 0;
    ep_info.config_num          = 0;
    ep_info.intf_num            = 0;
    ep_info.alt_sttg            = 0;
    ep_info.max_burst           = bMaxBurst;
    ep_info.ep_comp_attrib      = bmSSEndpCompAttrib;
    ep_info.bytes_per_interval  = 0;

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_MODIFY_SS_PIPE),
                        (VOID*) (&ep_info),
                        sizeof(USB_EP_INFO));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*        NU_USB_HW_Update_Power_Mode
*
* DESCRIPTION
*
*       This function is called by stack to scale power mode of USB 
*		controller according to current link state.
*
* INPUTS
*
*   	cb               	- Pointer to the NU_USB_HW control block.
*   	power_mode       	- Power mode to be enabled in hardware.
*
* OUTPUTS
*
*   	NU_USB_INVLD_ARG	- Any of the input arguments is invalid.
*
*************************************************************************/
STATUS NU_USB_HW_Update_Power_Mode( NU_USB_HW   *cb,
                                    UINT8       power_mode )
{
    STATUS          status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_UPDATE_PWR_MODE),
                        (VOID*) (&power_mode),
                        sizeof(UINT8));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*        NU_USB_HW_Update_BELT_Value
*
* DESCRIPTION
*
*       This function is called by stack to update BELT value of USB 
*		controller according to current link state.
*
* INPUTS
*
*   	cb               	- Pointer to the NU_USB_HW control block.
*   	belt_value       	- New BELT value.
*
* OUTPUTS
*
*   	NU_USB_INVLD_ARG	- Any of the input arguments is invalid.
*
*************************************************************************/
STATUS NU_USB_HW_Update_BELT_Value( NU_USB_HW   *cb,
                                    UINT16      belt_value )
{
    STATUS          status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    status = DVC_Dev_Ioctl(cb->dv_handle,
                        (cb->ioctl_base_addr + NU_USB_IOCTL_UPDATE_BELT_VAL),
                        (VOID*) (&belt_value),
                        sizeof(UINT16));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}
#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

#endif /* USB_HW_EXT_C */
/* =======================  End Of File  ============================== */
