/*************************************************************************
*
*              Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       nu_usbh_xhci_ext.c
*
*
* COMPONENT
*
*       USB XHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains the exported function definitions for
*       the Generic xHCI driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBH_XHCI_Create                 Initializes the generic xHCI
*                                           driver.
*       _NU_USBH_XHCI_Delete                Un-initializes the xHCI Driver.
*       NU_USBH_XHCI_Initialize             Initializes the HW.
*       NU_USBH_XHCI_Uninitialize           Un-initializes the HW.
*       NU_USBH_XHCI_Initialize_Device      Setups HW resources required
*                                           by the device.
*       NU_USBH_XHCI_Uninitialize_Device    Frees the HW resources acquired
*                                           by the device.
*       NU_USBH_XHCI_Open_SS_Pipe           Opens SS endpoint on the HW.
*       NU_USBH_XHCI_Close_Pipe             Closes a pipe on the HW.
*       NU_USBH_XHCI_Open_Pipe              Initializes a pipe.
*       NU_USBH_XHCI_Flush_Pipe             Flushes all transfers on a pipe
*       NU_USBH_XHCI_Unstall_Pipe           Removes stall on the pipe.
*       NU_USBH_XHCI_Disable_Pipe           Disables a pipe.
*       NU_USBH_XHCI_Submit_IRP             Accepts a transfer on a pipe.
*       _NU_USBH_XHCI_Submit_Stream         Accepts stream transfer on bulk
*                                           pipe.
*       NU_USBH_XHCI_ISR                    HW Interrupt processing.
*       NU_USBH_XHCI_Enable_Interrupts      Enables the Hw interrupts.
*       NU_USBH_XHCI_Disable_Interrupts     Disables the Hw interrupts.
*       NU_USBH_XHCI_Update_device          Updates parameters of device
*                                           and pipes.
*       NU_USBH_XHCI_Modify_Pipe            Alters the characteristics of
*                                           the pipe.
*       NU_USBH_XHCI_Reset_Bandwidth        Resets HW allocated bandwidth.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ==============  USB Include Files =================================== */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "drivers/nu_usbh_xhci_ext.h"


/**************************************************************************
*
* FUNCTION
*
*       _NU_USBH_XHCI_Create
*
* DESCRIPTION
*
*       This function initializes the Generic xHCI driver.
*
* INPUTS
*
*       cb                                  xHCI Control Block.
*       name                                Name of the controller.
*       pool                                Memory pool for the driver to
*                                           operate on.
*       base_address                        Memory location in the system
*                                           memory map for the controller.
*       vector                              Interrupt vector associated
*                                           with the xHCI.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver has
*                                           been initialized successfully.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Create(NU_USBH_XHCI    *xhci,
                           CHAR            *name,
                           NU_MEMORY_POOL  *uncacheable_pool,
                           NU_MEMORY_POOL  *cacheable_pool,
                           VOID            *base_address,
                           INT              vector)
{
    STATUS status = NU_SUCCESS;

    /* Parameters validation. */
    NU_USB_PTRCHK_RETURN(xhci);
    NU_USB_PTRCHK_RETURN(name);
    NU_USB_PTRCHK_RETURN(cacheable_pool);
    NU_USB_PTRCHK_RETURN(uncacheable_pool);
    NU_USB_PTRCHK_RETURN(base_address);

    memset(xhci, 0x00, sizeof(NU_USBH_XHCI));

    xhci->cacheable_pool = cacheable_pool;
    xhci->uncachable_pool = uncacheable_pool;

    /* Call the base function. */
    status = _NU_USBH_HW_Create( (NU_USBH_HW *) xhci,
                                 name,
                                 uncacheable_pool,
                                 1,
                                 USB_SPEED_SUPER,
                                 base_address,
                                 vector,
                                 &usbh_xhci_dispatch );

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*        _NU_USBH_XHCI_Delete
*
* DESCRIPTION
*
*        This function un-initializes the driver.
*
* INPUTS
*
*        cb                                 XHCI driver to be
*                                           un-initialized.
*
* OUTPUTS
*
*        NU_SUCCESS                         always.
*
**************************************************************************/
STATUS _NU_USBH_XHCI_Delete(VOID *xhci)
{
    return _NU_USBH_HW_Delete(xhci);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBH_XHCI_Initialize
*
* DESCRIPTION
*
*       This function performs the xHC initialization steps mentioned in
*       the xHCI specs,section 4.2. It also creates and initializes data
*       structures that are necessary for host controller operation.
*
* INPUTS
*
*       cb                                  xHCI Control block.
*       stack                               Stack object associated with
*                                           the HW.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful initialization of
*                                           the hw.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Initialize(NU_USB_HW    *cb,
                               NU_USB_STACK *stack)
{
    /* Progress status. */
    STATUS                      status     = NU_SUCCESS;
    STATUS                      int_status = NU_SUCCESS;

    /* Pointer to xHCD control block. */
    NU_USBH_XHCI                *xhci     = NU_NULL;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(stack);

    /* Setup the register base of xHCD.*/
    xhci     = ( NU_USBH_XHCI *) cb;


    /* Perform host controller hardware specific initializations. */
    status = XHCI_Initialize_HW_Controller( cb );

    if ( status == NU_SUCCESS )
    {
        /* Call parent's initialize routine to install LISR */
        status = _NU_USBH_HW_Initialize( cb, stack );

        /* Initialize the root hub. */
        if ( status == NU_SUCCESS )
        {
            status = XHCI_RH_Initialize( xhci );

            if ( status == NU_SUCCESS )
            {
                /* Put the controller into work. */
                status = XHCI_Start_Controller( xhci );
            }
            else
            {
                (VOID)_NU_USBH_HW_Uninitialize( cb );
            }
        }
    }

    /* Cleanup in case of error. */
    if ( status != NU_SUCCESS )
    {
        int_status = NU_USBH_XHCI_Cleanup( xhci );
        NU_UNUSED_PARAM(int_status);
    }

    return ( status );
}


/**************************************************************************
*
* FUNCTION
*
*       _NU_USBH_XHCI_Uninitialize
*
* DESCRIPTION
*
*       This function un-initializes the xHCI hardware.
*
* INPUTS
*
*       cb                                  xHCI controller to be
*                                           un-initialized.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Uninitialize(NU_USB_HW * cb)
{
    /* Progress status .*/
    STATUS                status    = NU_SUCCESS;

    /* xHCI control block handle. */
    NU_USBH_XHCI          *xhci     = NU_NULL;

    /* Parameter validation. */
    NU_USB_PTRCHK(cb);

    xhci = ( NU_USBH_XHCI * )cb;

    status = XHCI_Stop_Controller( xhci );

    /* Un-initialize host controller hardware. */
    status |= XHCI_Uninitialize_HW_Controller( xhci );

    /* Parent un-intialization. */
    status |= _NU_USBH_HW_Uninitialize( cb );

    /* xHCI memory cleanup. */
    status |= NU_USBH_XHCI_Cleanup( xhci );

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Initialize_Device
*
* DESCRIPTION
*
*       This function creates and initializes data structures required by
*       the host controller for managing the attached device.
*
* INPUTS
*
*       cb                                  Host controller control block
*                                           Handle.
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           initialization of device.
*       NU_USB_INVLD_ARG                    Indicates that any of the
*                                           input parameters is invalid.
*       XHCI_INVLD_DCBAA                    Indicates that DCBBA is not
*                                           initialized.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Initialize_Device(NU_USBH_HW *cb)
{
    /* Progress status. */
    STATUS              status     = NU_SUCCESS;
    STATUS              int_status = NU_SUCCESS;

    /* xHCI control block. */
    NU_USBH_XHCI        *xhci      = NU_NULL;

    /* Pointer to newly allocated device. */
    NU_USBH_XHCI_DEVICE *xhci_dev  = NU_NULL;

    /* Rollback to free resources in case of error. */
    UINT8                roll_back = 0x00;

    /* Slot ID assigned to device. */
    UINT8                slot_id   = 0;

    /* Register value. */
    UINT32               reg_value = 0;

    /* Parameters validation .*/
    NU_USB_PTRCHK(cb);

    /* Get the xhci control block handle.*/
    xhci = (NU_USBH_XHCI *) cb;

    status = NU_USBH_XHCI_Lock( xhci );

    if ( status == NU_SUCCESS )
    {
        /* Get the slot id for the new device from the HW controller.*/
        status = XHCI_Queue_Slot_Control_Command( xhci,
                                                  XHCI_TRB_ENABLE_SLOT,
                                                  0 );
        if ( status == NU_SUCCESS )
        {
            if ( !xhci->slot_id )
            {
                /* Fail to assign slot ID */
                status = XHCI_SLOT_UNAVAILABLE;
                roll_back = 1;
            }
            else
            {
                /* Save the device slot ID in the xHCD device table at index 0.
                 * Initially all devices are assigned address 0.
                 */
                xhci->device_table[0] = xhci->slot_id;
                slot_id = xhci->slot_id;
            }
            if ( status == NU_SUCCESS )
            {
                /* Allocate the device related data structures. */
                status = NU_Allocate_Memory( xhci->cacheable_pool,
                                             (VOID **)&xhci->device[slot_id],
                                             sizeof(NU_USBH_XHCI_DEVICE),
                                             NU_SUSPEND );
                if ( status == NU_SUCCESS )
                {
                    memset( xhci->device[slot_id], 0x00, sizeof(NU_USBH_XHCI_DEVICE) );
                    xhci_dev = xhci->device[slot_id];
                    xhci_dev->slot_id = slot_id;
    
                    /* Allocate xHCI device OUT context. */
                    status = XHCI_Allocate_Device_Context( xhci,
                                                           &xhci_dev->dev_out_ctx,
                                                           XHCI_DEVICE_OUT_CTX );
                    if ( status == NU_SUCCESS )
                    {
                        /* Allocate xHCI device IN context. */
                        status = XHCI_Allocate_Device_Context( xhci,
                                                               &xhci_dev->dev_in_ctx,
                                                               XHCI_DEVICE_IN_CTX );
                        if ( status == NU_SUCCESS )
                        {
                            if ( xhci->dcbaa )
                            {
                                /* Save the device context DS pointer in the DCBAA.*/
                                XHCI_READ_ADDRESS(xhci, &xhci->dcbaa[slot_id].dev_ctx_ptrs_lo, reg_value );
                                reg_value |= (UINT32)xhci_dev->dev_out_ctx->ctx_buffer;
                                XHCI_WRITE_ADDRESS(xhci, &xhci->dcbaa[slot_id].dev_ctx_ptrs_lo,reg_value );
                                XHCI_WRITE_ADDRESS(xhci, &xhci->dcbaa[slot_id].dev_ctx_ptrs_hi,0 );
                                XHCI_READ_ADDRESS(xhci, &xhci->dcbaa[slot_id].dev_ctx_ptrs_lo, reg_value );
    
                                status = XHCI_Allocate_TRB_Ring( xhci, &(xhci_dev->ep_info[0].ring),
                                                                 XHCI_TRANSFER_RING_SEGMENTS,
                                                                 XHCI_TRANS_RING_ALIGNMENT,
                                                                 NU_TRUE );
                                if ( status != NU_SUCCESS )
                                {
                                    roll_back = 5;
                                }
                                else
                                {
                                    xhci_dev->ep_type[0] = USB_EP_CTRL;
                                }
                            }
                            else
                            {
                                roll_back = 5;
                                status = XHCI_INVLD_DCBAA;
                            }
                        }
                        else
                        {
                            roll_back = 4;
                        }
                    }
                    else
                    {
                        roll_back = 3;
                    }
                }
                else
                {
                    roll_back = 2;
                }
            }
        }
    }
    switch (roll_back)
    {
        /* Fall through. */
        case 5:
          int_status = XHCI_Deallocate_Device_Context( xhci_dev->dev_in_ctx );
          xhci_dev->dev_in_ctx = NU_NULL;

        case 4:
            int_status |= XHCI_Deallocate_Device_Context( xhci_dev->dev_out_ctx );
            xhci_dev->dev_out_ctx = NU_NULL;

        case 3:
            int_status |= NU_Deallocate_Memory( xhci_dev );
            xhci_dev = NU_NULL;

        case 2:
            /* Issue disable slot command here.*/
            int_status |= XHCI_Queue_Slot_Control_Command( xhci,
                                                           XHCI_TRB_DISABLE_SLOT,
                                                           xhci->slot_id );
        case 1:
            int_status |= NU_USBH_XHCI_Unlock( xhci );
            return ( status );

        default:
           NU_UNUSED_PARAM(int_status);
    }

    status = NU_USBH_XHCI_Unlock( xhci );

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Uninitialize_Device
*
* DESCRIPTION
*
*       This function frees the HW resources used by the device.
*
* INPUTS
*
*       cb                                  Host controller control
*                                           associated with device.
*       function_address                    Device address.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that device is
*                                           is successfully uninitialized.
*       NU_USB_INVLD_ARG                    Indicates that any of the
*                                           input parameters is invalid.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Uninitialize_Device(NU_USBH_HW *cb,
                                        UINT8      function_address)
{
    /* Progress status. */
    STATUS              status =  NU_SUCCESS;

    /* xHCI control block. */
    NU_USBH_XHCI        *xhci;

    /* Pointer to device to be uninitialized. */
    NU_USBH_XHCI_DEVICE *xhci_dev;

    /* Slot ID for device. */
    UINT8               slot_id = 0;

    /* Loop index. */
    UINT8               index;

    NU_USB_PTRCHK(cb);

    /* Get the xhci cb handle.*/
    xhci = (NU_USBH_XHCI *) cb;

    slot_id = xhci->device_table[function_address];
    xhci_dev = xhci->device[slot_id];
    
    status = NU_USBH_XHCI_Lock( xhci );
    if ( status == NU_SUCCESS )
    {
        status  = XHCI_Queue_Slot_Control_Command( xhci,
                                                   XHCI_TRB_DISABLE_SLOT,
                                                   slot_id );
        for ( index = 0; index < 31; index++ )
        {
            if (xhci_dev->ep_info[index].ring)
            {
                status |= XHCI_Deallocate_TRB_Ring(xhci_dev->ep_info[index].ring);
                xhci_dev->ep_info[index].ring = NU_NULL;
            }
        }
    
        if ( xhci_dev->dev_in_ctx )
        {
            status |= XHCI_Deallocate_Device_Context(xhci_dev->dev_in_ctx);
            xhci_dev->dev_in_ctx = NU_NULL;
        }
    
        if ( xhci_dev->dev_out_ctx )
        {
            status |= XHCI_Deallocate_Device_Context(xhci_dev->dev_out_ctx);
            xhci_dev->dev_out_ctx = NU_NULL;
        }
    
        status |= NU_Deallocate_Memory( xhci_dev );
    
        XHCI_WRITE_ADDRESS(xhci, &xhci->dcbaa[slot_id].dev_ctx_ptrs_lo, 0);

        status |= NU_USBH_XHCI_Unlock( xhci );
    }
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Open_SS_Pipe
*
* DESCRIPTION
*
*       This function setup the endpoint context data structures and
*       allocates Bandwidth required by the endpoint.
*
* INPUTS
*       cb                                  Handle that identifies the HC.
*       function_address                    Identifies the device on which
*                                           the pipe resides
*       bEndpointAddress                    Identifies the endpoint which
*                                           owns the pipe
*       bmAttributes                        Identifies the endpoint type.
*       speed                               USB device speed.
*       ep_max_size                         Maximum packet size the
*                                           endpoint should support.
*       interval                            Interval in micro seconds for
*                                           periodic endpoints.
*       bMaxBurst                           Burst size supported by the
*                                           endpoint.
*       SSEndpCompAttrib                    SS endpoint companion descrip-
*                                           tor attribute field.
*       bytes_per_interval                  Total bytes transferred by the
*                                           endpoint every service interval.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful operation.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Open_SS_Pipe(NU_USB_HW     *cb,
                                  UINT8         function_addr,
                                  UINT8         bEndpointAddress,
                                  UINT8         bmEndpointAttributes,
                                  UINT8         speed,
                                  UINT16        wMaxPacketSize,
                                  UINT32        interval,
                                  UINT32        load,
                                  UINT8         bMaxBurst,
                                  UINT8         SSEndpCompAttrib,
                                  UINT16        bytes_per_interval)
{
    /* Progress status. */
    STATUS                  status       = NU_SUCCESS;
    STATUS                  int_status   = NU_SUCCESS;

    /* Pointer to xHCI control block.*/
    NU_USBH_XHCI            *xhci        = NU_NULL;

    /* xHCI device pointer , required for referencing device context DS. */
    NU_USBH_XHCI_DEVICE     *xhci_device = NU_NULL;
    NU_USBH_XHCI_EP_INFO    *ep_info     = NU_NULL;
    VOID                    *addr        = NU_NULL;

    /* Slot ID associated with the device. */
    UINT8                   slot_id      = 0;

    /* xHCD defined endpoint index against endpoint number.*/
    UINT8                   ep_index     = 0;

    /* Number of streams supported by the bulk endpoint. */
    UINT16                  num_streams  = 0;

    /* Parameters validation.*/
    NU_USB_PTRCHK(cb);
    NU_UNUSED_PARAM(load);

    /* Get the xhci control block handle.*/
    xhci    = (NU_USBH_XHCI *) cb;

    /* Get the device slot id corresponding to function address. */
    slot_id = xhci->device_table[function_addr];

    /* Get the xHCD based endpoint index for endpoint address. */
    ep_index = XHCI_EP_ADDR_TO_EP_INDEX(bEndpointAddress);
    
    status = NU_USBH_XHCI_Lock( xhci );
    if ( status == NU_SUCCESS )
    {
        /* Get the device handle based on the slot id. */
        status = XHCI_Get_Device_Handle( xhci, slot_id, &xhci_device);
    
        if ( (status == NU_SUCCESS) && (ep_index >= 0) )
        {
            /* Get the endpoint handle .*/
            status = XHCI_Get_Endpoint_Handle( xhci_device, ep_index, &ep_info );
    
            if ( status == NU_SUCCESS )
            {
                /* Check if this is bulk endpoint and support streams. */
                if ( ((bmEndpointAttributes & XHCI_EP_MASK) == USB_EP_BULK)
                    && (SSEndpCompAttrib) )
                {
                    num_streams = SSEndpCompAttrib & 0x1F;
                    num_streams = ( 1 << num_streams ) + 1;
                    status = XHCI_Allocate_Streams(xhci, num_streams, &ep_info->stream_info );
                    if ( status == NU_SUCCESS )
                    {
                        addr = ( VOID *)ep_info->stream_info->stream_ctx_array;
                        ep_info->has_streams = NU_TRUE;
                    }
                }
                else if ( ep_index != USB_EP_CTRL)
                {
                    /* Allocate TRB ring used by the endpoint for data transfer. */
                    status  = XHCI_Allocate_TRB_Ring( xhci, &ep_info->ring,
                                                      XHCI_TRANSFER_RING_SEGMENTS,
                                                      XHCI_TRANS_RING_ALIGNMENT,
                                                      NU_TRUE );
                    if ( status == NU_SUCCESS )
                    {
                        addr = ( VOID *)ep_info->ring;
                    }
                }
    
                if ( status == NU_SUCCESS )
                {
                    /* Fill the input context data structures for configure endpoint command. */
                    status = XHCI_Parse_Config_Endpoint_In_Ctx( xhci,
                                                                xhci_device->dev_in_ctx,
                                                                addr,
                                                                bEndpointAddress,
                                                                bmEndpointAttributes, speed,
                                                                bMaxBurst, wMaxPacketSize,
                                                                interval,SSEndpCompAttrib,
                                                                bytes_per_interval );
                    if ( status == NU_SUCCESS )
                    {
                        /* Configure the endpoint in the hardware. */
                        status = XHCI_Queue_Configure_EP_Command( xhci,
                                                                  xhci_device->dev_in_ctx,
                                                                  slot_id );
                        if ( status == NU_SUCCESS )
                        {
                            /* Set up look up table for endpoint type with endpoint number as key. */
                            if ( (bmEndpointAttributes & XHCI_EP_MASK) == USB_EP_ISO )
                            {
                                xhci_device->ep_type[ep_index] =  USB_EP_ISO;
                            }
                            else
                            {
                                xhci_device->ep_type[ep_index] =  USB_EP_BULK;
                            }
                        }
                    }
                }
            }
        }

        int_status = NU_USBH_XHCI_Unlock( xhci );
    }

    return ( (( status == NU_SUCCESS ) ? int_status : status) );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Close_Pipe
*
* DESCRIPTION
*
*       This function frees the resources used by the endpoint.
*
* INPUTS
*
*       cb                                  Handle that identifies the HC
*       function_address                    Identifies the device on which
*                                           the pipe resides
*       bEndpointAddress                    Identifies the endpoint which
*                                           owns the pipe
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful operation.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Close_Pipe(NU_USB_HW *cb,
                                UINT8     function_address,
                                UINT8     bEndpointAddress)
{
    /* Progress status. */
    STATUS                status       = NU_SUCCESS;
    STATUS                int_status   = NU_SUCCESS;

    /* Pointer to xHCI contorl block.*/
    NU_USBH_XHCI          *xhci        = NU_NULL;

    /* xHCI device pointer , required for referencing device context DS. */
    NU_USBH_XHCI_DEVICE   *xhci_device = NU_NULL;

    /* Pointer to transfer ring associated with the endpoint.*/
    NU_USBH_XHCI_RING     *ring        = NU_NULL;

    /* Endpoint control block. */
    NU_USBH_XHCI_EP_INFO  *ep_info     = NU_NULL;

    /* Pointer to device input control context. */
    NU_USBH_XHCI_CTRL_CTX *ctrl_ctx    = NU_NULL;

    /* Pointer device slot context. */
    NU_USBH_XHCI_SLOT_CTX *slot_ctx    = NU_NULL;

    /* Slot ID associated with the device. */
    UINT8                 slot_id      = 0;

    /* xHCD defined endpoint index against endpoint number.*/
    UINT8                 ep_index     = 0;

    /* Loop index. */
    UINT16                index        = 0;

    /* Parameters validation.*/
    NU_USB_PTRCHK(cb);

    xhci = (NU_USBH_XHCI *) cb;

    /* Get the device slot id form the device function address. */
    slot_id = xhci->device_table[function_address];

    /* Get the xHCD bases endpoint index from the endpoint address.*/
    ep_index = XHCI_EP_ADDR_TO_EP_INDEX(bEndpointAddress);

    status = NU_USBH_XHCI_Lock( xhci );
    if ( status == NU_SUCCESS )
    {
        /* Get the device handle based on the slot id. */
        status = XHCI_Get_Device_Handle( xhci, slot_id, &xhci_device);
    
        if ( status == NU_SUCCESS )
        {
            /* Get the endpoint handle .*/
            status = XHCI_Get_Endpoint_Handle( xhci_device, ep_index, &ep_info );
    
            if ( status == NU_SUCCESS )
            {
                if ( ep_index != USB_EP_CTRL )
                {
                    /* Get the slot_context from the xhci device IN Ctx. */
                    status = XHCI_Get_Slot_Context( xhci_device->dev_in_ctx, &slot_ctx );
    
                    if ( status == NU_SUCCESS )
                    {
                        status = XHCI_Get_Input_Control_Context( xhci_device->dev_in_ctx, &ctrl_ctx );
    
                        if ( status == NU_SUCCESS )
                        {
                            /* Drop the endpoint from the xHC scheduling list. */
                            ctrl_ctx->drop_flags = HOST_2_LE32(XHCI_CTRL_CTX_FLAG(ep_index));
                            ctrl_ctx->add_flags  = HOST_2_LE32(SLOT_CTX_SLOT_FLAG);
                            ctrl_ctx->drop_flags &= HOST_2_LE32(~SLOT_CTX_SLOT_FLAG);
    
                            /* Update the last valid endpoint ctx in the slot ctx data
                             * structure.
                             */
                            if ( ((LE32_2_HOST(slot_ctx->device_info1) >> 27) & 0x1F ) >
                                     (ep_index + 1) )
                            {
                                slot_ctx->device_info1 &= HOST_2_LE32(~SLOT_CTX_LAST_CTX_MASK);
                                slot_ctx->device_info1 |= HOST_2_LE32(SLOT_CTX_LAST_CTX(ep_index + 1));
                            }
    
                            /* Free the hardware resources acquired by the endpoint. */
                            status = XHCI_Queue_Configure_EP_Command( xhci,
                                                                      xhci_device->dev_in_ctx,
                                                                      slot_id );
                            if ( status == NU_SUCCESS )
                            {
                                if ( ep_info->has_streams )
                                {
                                    /* Generate the callback for pending IRPs and free the streams.*/
                                    for ( index = 0; index < ep_info->stream_info->num_streams; ++index )
                                    {
                                        status = XHCI_Get_Stream_Ring_handle( ep_info->stream_info ,
                                                                                index, &ring);
    
                                        if (  (status == NU_SUCCESS) && (ring->xhci_irp.irp) )
                                        {
                                            (VOID)XHCI_Handle_IRP_Completion( ring->xhci_irp.irp,
                                                                              NU_USB_IRP_CANCELLED, 0);
                                        }
                                    }
                                    status = XHCI_Deallocate_Streams( xhci, ep_info->stream_info );
                                    ep_info->has_streams = NU_FALSE;
                                    ep_info->stream_info = NU_NULL;
                                }
                                else
                                {
                                    /* Generate the callback for pending IRPs and free the ring.*/
                                    if ( ep_info->ring->xhci_irp.irp )
                                    {
                                        (VOID)XHCI_Handle_IRP_Completion( ep_info->ring->xhci_irp.irp,
                                                                          NU_USB_IRP_CANCELLED, 0);
                                    }
    
                                    status = XHCI_Deallocate_TRB_Ring( ep_info->ring );
                                    ep_info->ring = NU_NULL;
                                }
                            }
                        }
                    }
                }
                else
                {
                    /* Generate the callback for pending IRPs. The endpoint 0 resources
                     * are released when device is de-init by the stack.
                     */
                    if ( ep_info->ring->xhci_irp.irp )
                    {
                        (VOID)XHCI_Handle_IRP_Completion( ep_info->ring->xhci_irp.irp,
                                                          NU_USB_IRP_CANCELLED, 0);
                    }
                }
            }
        }

        int_status = NU_USBH_XHCI_Unlock( xhci );
    }

    return ( (( status == NU_SUCCESS ) ? int_status : status) );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Open_Pipe
*
* DESCRIPTION
*
*       Creates endpoint descriptors in the H/W and makes them ready for
*       scheduling transfers.
*
* INPUTS
*
*       cb                                  Host controller control block
*                                           Handle..
*       function_address                    Identifies the device on which
*                                           the pipe resides
*       bEndpointAddress                    Identifies the endpoint which
*                                           owns the pipe
*       bmAttributes                        Identifies the endpoint type.
*       speed                               USB device speed.
*       ep_max_size                         Maximum packet size the
*                                           endpoint should support.
*       interval                            Interval in micro seconds for
*                                           periodic endpoints.
*       load                                Frame b/w consumed by the
*                                           endpoint.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful creation
*                                           of the pipe in h/w.
*       NU_INVALID_SEMAPHORE                Indicates the semaphore pointer
*
**************************************************************************/
STATUS NU_USBH_XHCI_Open_Pipe(NU_USB_HW    *cb,
                               UINT8        function_addr,
                               UINT8        bEndpointAddress,
                               UINT8        bmAttributes,
                               UINT8        speed,
                               UINT16       ep_max_size,
                               UINT32       interval,
                               UINT32       load)
{
    /* Progress status. */
    STATUS                 status     = NU_SUCCESS;
    STATUS                 int_status = NU_SUCCESS;
    
    /* xHCI control block pointer. */
    NU_USBH_XHCI          *xhci     = NU_NULL;

    /* xHCI device pointer.*/
    NU_USBH_XHCI_DEVICE   *xhci_dev = NU_NULL;

    /* Endpoint control block for opened pipe . */
    NU_USBH_XHCI_EP_INFO  *ep_info  = NU_NULL;

    /* Slot ID associated with the device. */
    UINT8                 slot_id   = 0;

    /* xHCD defined endpoint index against endpoint number.*/
    UINT8                 ep_index  = 0;

    NU_USB_PTRCHK(cb);

    xhci = (NU_USBH_XHCI *)cb;

    /* Get the device slot id form the device function address. */
    slot_id = xhci->device_table[function_addr];

    /* Get the xHCD based endpoint index against endpoint address. */
    ep_index = XHCI_EP_ADDR_TO_EP_INDEX(bEndpointAddress);

    status = NU_USBH_XHCI_Lock( xhci );

    if ( status == NU_SUCCESS )
    {
        /* Get the device handle based on the slot id. */
        status = XHCI_Get_Device_Handle( xhci, slot_id, &xhci_dev);
    
        if ( status == NU_SUCCESS )
        {
            /* Get the endpoint handle .*/
            status = XHCI_Get_Endpoint_Handle( xhci_dev, ep_index, &ep_info );
    
            if ( status == NU_SUCCESS )
            {
                /* Set up look up table for endpoint type with endpoint number as key. */
                if ( ep_index != USB_EP_CTRL )
                {
                    /* Allocate TRB ring used by the endpoint for data transfer. */
                    status  = XHCI_Allocate_TRB_Ring( xhci, &ep_info->ring,
                                                      XHCI_TRANSFER_RING_SEGMENTS,
                                                      XHCI_TRANS_RING_ALIGNMENT,
                                                      NU_TRUE );
                    if ( status == NU_SUCCESS )
                    {
                        /* Fill the input context data structures for configure endpoint command. */
                        status = XHCI_Parse_Config_Endpoint_In_Ctx( xhci,
                                                                    xhci_dev->dev_in_ctx,
                                                                    ( VOID *)ep_info->ring,
                                                                    bEndpointAddress,
                                                                    bmAttributes, speed,
                                                                    0, ep_max_size,
                                                                    interval,0, 0 );
                        if ( status == NU_SUCCESS )
                        {
                            /* Configure the endpoint in the hardware. */
                            status = XHCI_Queue_Configure_EP_Command( xhci,
                                                                      xhci_dev->dev_in_ctx,
                                                                      slot_id );
                            if ( status == NU_SUCCESS )
                            {
                                if ( (bmAttributes & XHCI_EP_MASK) == USB_EP_ISO )
                                {
                                    xhci_dev->ep_type[ep_index] =  USB_EP_ISO;
                                }
                                else
                                {
                                    xhci_dev->ep_type[ep_index] =  USB_EP_BULK;
                                }
                            }
                        }
                    }
                }
            }
        }

        int_status = NU_USBH_XHCI_Unlock( xhci );
    }

    return ( (( status == NU_SUCCESS ) ? int_status : status) );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Flush_Pipe
*
* DESCRIPTION
*
*       Removes all pending TDs from HC's schedule list. Their
*       corresponding IRP callbacks are notified of this transfer
*       termination.
*
* INPUTS
*
*       cb                                  xHCI Control block.
*       function_address                    Device address to which the
*                                           transfers must be flushed.
*       bEndpointAddress                    Endpoint on the device for
*                                           which the transfers must be
*                                           flushed.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the TDs
*                                           termination has been
*                                           successfully initiated.
*       NU_UB_INVLD_ARG                     xhci pointers passed are
*                                           NU_NULL.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Flush_Pipe(NU_USB_HW *cb,
                               UINT8 function_address,
                               UINT8 bEndpointAddress)
{
    /* Progress status. */
    STATUS                status       = NU_SUCCESS;
    STATUS                int_status   = NU_SUCCESS;

    /* Pointer to xHCI control block.*/
    NU_USBH_XHCI          *xhci        = NU_NULL;

    /* xHCI device pointer , required for referencing device context DS. */
    NU_USBH_XHCI_DEVICE   *xhci_device = NU_NULL;

    /* Endpoint control block. */
    NU_USBH_XHCI_EP_INFO  *ep_info     = NU_NULL;

    /* Pointer to transfer ring associated with the endpoint.*/
    NU_USBH_XHCI_RING     *ring        = NU_NULL;

    /* Endpoint context data structure. */
    NU_USBH_XHCI_EP_CTX   *ep_ctx      = NU_NULL;

    /* xHCD defined endpoint index against endpoint number.*/
    UINT8                 ep_index     = 0;

    /* Slot ID associated with the device. */
    UINT8                 slot_id      = 0;

    /* Loop index. */
    UINT8                 index        = 0;


    /* Parameters validation. */
    NU_USB_PTRCHK(cb);

    xhci = (NU_USBH_XHCI *) cb;
    slot_id = xhci->device_table[function_address];
    ep_index = XHCI_EP_ADDR_TO_EP_INDEX(bEndpointAddress);

    status = NU_USBH_XHCI_Lock( xhci );
    if ( status == NU_SUCCESS )
    {
        status = XHCI_Get_Device_Handle( xhci, slot_id, &xhci_device );
        if ( status == NU_SUCCESS )
        {
            status = XHCI_Get_Endpoint_Handle( xhci_device, ep_index, &ep_info );
    
            if ( status == NU_SUCCESS )
            {
                status = XHCI_Get_Endpoint_Context(xhci_device->dev_out_ctx, &ep_ctx, ep_index);
    
                if ( (LE32_2_HOST(ep_ctx->ep_info1) & EP_CTX_STATE_MASK) == XHCI_EP_STATE_RUNNING )
                {
                    status = XHCI_Queue_Stop_Endpoint_Command(xhci, ep_index, 0 , 0 ,slot_id);
                }
                else if ((LE32_2_HOST(ep_ctx->ep_info1) & EP_CTX_STATE_MASK) == XHCI_EP_STATE_HALTED )
                {
                    status = XHCI_Queue_Reset_Endpoint_Command(xhci, slot_id, ep_index );
                }
    
                if ( status == NU_SUCCESS )
                {
                    if ( ep_info->has_streams )
                    {
                        for ( index = 0; index < ep_info->stream_info->num_streams; ++index )
                        {
                            status = XHCI_Get_Stream_Ring_handle( ep_info->stream_info , index, &ring);
    
                            if (  status == NU_SUCCESS  )
                            {
                                if ( ring->xhci_irp.irp )
                                {
                                    (VOID)XHCI_Handle_IRP_Completion( ring->xhci_irp.irp,
                                                                    NU_USB_IRP_CANCELLED, 0);
                                    ring->xhci_irp.irp = NU_NULL;
                                }
    
                                status = XHCI_Reinitialize_Ring( xhci, ring,  ep_index, slot_id );
                                if ( status != NU_SUCCESS )
                                    break;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    else if ( status == NU_SUCCESS )
                    {
                        status = XHCI_Get_Endpoint_Ring_Handle( ep_info , &ring);
    
                        if ( status == NU_SUCCESS )
                        {
                            if ( ring->xhci_irp.irp )
                            {
                                status = XHCI_Handle_IRP_Completion( ring->xhci_irp.irp,
                                                                     NU_USB_IRP_CANCELLED,
                                                                     0 );
                                ring->xhci_irp.irp = NU_NULL;
                            }
    
                            if ( status == NU_SUCCESS )
                            {
                                status = XHCI_Reinitialize_Ring( xhci, ring, ep_index,  slot_id );
                            }
                        }
                    }
                }
            }
        }

        int_status = NU_USBH_XHCI_Unlock( xhci );
    }

    return ( (( status == NU_SUCCESS ) ? int_status : status) );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Unstall_Pipe
*
* DESCRIPTION
*
*       Removes the stall on the given pipe.
*
* INPUTS
*
*       cb                                  xHCI Control block.
*       function_address                    Device address to which the
*                                           transfers must be flushed.
*       bEndpointAddress                    Endpoint on the device for
*                                           which the transfers must be
*                                           flushed.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the TDs
*                                           termination has been
*                                           successfully initiated.
*       NU_USB_INVLD_ARG                    xhci pointers passed are
*                                           NU_NULL.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Unstall_Pipe( NU_USBH_HW *cb,
                                   UINT8      bEndpointAddress,
                                   UINT8      function_address )
{
    /* Progress status. */
    STATUS                status       = NU_SUCCESS;
    STATUS                int_status   = NU_SUCCESS;

    /* xHCI device pointer , required for referencing device context DS. */
    NU_USBH_XHCI_DEVICE   *xhci_device = NU_NULL;

    /* Pointer to xHCI control block. */
    NU_USBH_XHCI          *xhci        = NU_NULL;

    /* Endpoint control block. */
    NU_USBH_XHCI_EP_INFO  *ep_info     = NU_NULL;

    /* Pointer to transfer ring associated with the endpoint. */
    NU_USBH_XHCI_RING     *ring        = NU_NULL;

    /* xHCD defined endpoint index against endpoint number.*/
    UINT8                 ep_index     = 0;

    /* Slot ID associated with the device. */
    UINT8                 slot_id      = 0;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);

    xhci     = (NU_USBH_XHCI *) cb;
    slot_id  = xhci->device_table[function_address];
    ep_index = XHCI_EP_ADDR_TO_EP_INDEX(bEndpointAddress);

    status = NU_USBH_XHCI_Lock( xhci );
    if ( status == NU_SUCCESS )
    {
        status = XHCI_Get_Device_Handle( xhci, slot_id, &xhci_device );
    
        if ( status == NU_SUCCESS )
        {
            status = XHCI_Get_Endpoint_Handle( xhci_device, ep_index, &ep_info );
    
            if ( status == NU_SUCCESS )
            {
                if ( (ep_info->ep_state == XHCI_EP_STATE_HALTED) && (ep_index >= 0) )
                {
                    status = XHCI_Get_Endpoint_Ring_Handle( &xhci_device->ep_info[ep_index], &ring);
    
                    if ( status == NU_SUCCESS )
                    {
                        status = XHCI_Queue_Reset_Endpoint_Command( xhci, slot_id, ep_index );
    
                        if ( status == NU_SUCCESS )
                        {
                            status = XHCI_Reinitialize_Ring( xhci, ring,  ep_index, slot_id );
    
                            if ( status == NU_SUCCESS )
                            {
                                ep_info->ep_state = XHCI_EP_STATE_RUNNING;
                            }
                        }
                    }
                }
            }
        }
        int_status = NU_USBH_XHCI_Unlock( xhci );
    }

    return ( (( status == NU_SUCCESS ) ? int_status : status) );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Disable_Pipe
*
* DESCRIPTION
*
*         This function stops the given endpoint.
*
* INPUTS
*
*       cb                                  xHCI Control block.
*       function_address                    Device address for which
*                                           transfers must be flushed.
*       bEndpointAddress                    Endpoint on the device for
*                                           which the transfers must be
*                                           flushed.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that pipe has been
*                                           successfully stopped.
*       NU_USB_INVLD_ARG                    xhci pointers passed are
*                                           NU_NULL.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Disable_Pipe( NU_USBH_HW *cb,
                                   UINT8 bEndpointAddress,
                                   UINT8 function_address )
{
    /* Progress status. */
    STATUS status                      = NU_SUCCESS;
    STATUS                int_status   = NU_SUCCESS;

    /* Pointer to xHCI control block. */
    NU_USBH_XHCI          *xhci        = NU_NULL;

    /* xHCD defined endpoint index against endpoint number.*/
    UINT8                 ep_index     = 0;

    /* Slot ID associated with the device. */
    UINT8                 slot_id      = 0;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);

    xhci = (NU_USBH_XHCI *) cb;
    slot_id = xhci->device_table[function_address];
    ep_index = XHCI_EP_ADDR_TO_EP_INDEX(bEndpointAddress);
    
    status = NU_USBH_XHCI_Lock( xhci );

    if ( status == NU_SUCCESS )
    {
        /* Queue the disable endpoint command for disabling
         * the TRB processing. 
         */
        status = XHCI_Queue_Command( xhci, 0, 0,0,
                                    (SET_TRB_SLOT_ID(slot_id) |
                                     SET_TRB_TYPE(XHCI_TRB_STOP_EP)
                                    | SET_TRB_EP_ID(ep_index) | SET_TRB_SP_FLAG ));
    
        int_status = NU_USBH_XHCI_Unlock( xhci );
    }

    return ( (( status == NU_SUCCESS ) ? int_status : status) );
}
/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Submit_IRP
*
* DESCRIPTION
*
*       This function accepts a transfer to a given endpoint on a given
*       device. This function checks to see if the device is a root hub.
*       If so appropriate root hub IRP processing function is invoked
*       Otherwise endpoint specific IRP handler function is called.
*
* INPUTS
*
*       cb                                  Host controller control block
*                                           Handle.
*       irp                                 Data transfer description
*       function_address                    Device address to which the
*                                           transfer is directed.
*       bEndpointAddress                    Address of endpoint used for
*                                           transfer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates a successful
*                                           submission of data to HW.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Submit_IRP(NU_USB_HW   *cb,
                                NU_USB_IRP *irp,
                                UINT8      function_address,
                                UINT8      bEndpointAddress)
{
    /* Progress status. */
    STATUS               status;
    STATUS               int_status;

    /* xHCI control block pointer.*/
    NU_USBH_XHCI         *xhci;

    /* xHCI device pointer - device with which I/O is intended. */
    NU_USBH_XHCI_DEVICE  *xhci_dev;

    /* xHCI ep index against the endpoint number. */
    UINT8                ep_index;

    /* xHCI device slot ID - use for accessing xHCI device list. */
    UINT8                slot_id;

    /* Direction of data transfer IN/OUT. */
    BOOLEAN              direction;

    /* Parameters validation .*/
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(irp);

    xhci  = (NU_USBH_XHCI *)cb;
    ep_index = XHCI_EP_ADDR_TO_EP_INDEX(bEndpointAddress);

    /* Endpoint direction, 0 for IN 1 for OUT.*/
    direction = bEndpointAddress & 0x80 ? NU_FALSE : NU_TRUE;

    status = NU_USBH_XHCI_Lock( xhci );

    if ( status == NU_SUCCESS )
    {
        if ( function_address == USB_ROOT_HUB )
        {
            status = XHCI_RH_Handle_IRP( xhci, irp, bEndpointAddress );
        }
        else
        {
            slot_id  = xhci->device_table[function_address];
            xhci_dev = xhci->device[slot_id];

            if ( ep_index  ==  USB_EP_CTRL )
            {
                status = XHCI_Initiate_Control_Transfer( xhci, irp,
                                                         xhci_dev, ep_index );
            }
            else if ( xhci_dev->ep_type[ep_index] == USB_EP_ISO )
            {
                /* Iso endpoints are not supported yet. */
            }
            else
            {
                  /* Bulk and Interrupt endpoints have same handling. */
                  status = XHCI_Initiate_Bulk_Transfer( xhci,  irp, xhci_dev,
                                                        ep_index, direction );
            }
        }
        int_status =  NU_USBH_XHCI_Unlock( xhci );
    }

    return ( (( status == NU_SUCCESS ) ? int_status : status) );

}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Submit_Stream
*
* DESCRIPTION
*
*       This function accepts stream data transfer directed for Bulk endpoint.
*
* INPUTS
*
*       cb                                  Host controller control block
*                                           Handle.
*       stream                              Stream data transfer description.
*       function_address                    Device address to which the
*                                           transfer is directed.
*       bEndpointAddress                    Address of endpoint used for
*                                           transfer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates a successful
*                                           submission of data to HW.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Submit_Stream(NU_USB_HW     *cb,
                                   NU_USB_STREAM *stream,
                                   UINT8         function_address,
                                   UINT8         bEndpointAddress)
{
    /* Progress status. */
    STATUS               status        = NU_SUCCESS;
    STATUS               int_status    = NU_SUCCESS;

    /* xHCI control block pointer.*/
    NU_USBH_XHCI         *xhci         = NU_NULL;

    /* xHCI device pointer - device with which I/O is intended. */
    NU_USBH_XHCI_DEVICE  *xhci_dev     = NU_NULL;

    /* Endpoint info control block. */
    NU_USBH_XHCI_EP_INFO *ep_info      = NU_NULL;

    NU_USBH_XHCI_RING    *ring         = NU_NULL;

    NU_USB_IRP           *irp          = NU_NULL;
    NU_USBH_XHCI_EP_CTX  *ep_ctx       = NU_NULL;

    /* xHCI ep index against the endpoint number. */
    UINT8                ep_index      = 0;

    /* xHCI device slot ID - use for accessing xHCD device list. */
    UINT8                slot_id       = 0;

    UINT16               ep_max_pack   = 0;
    UINT16               stream_id     = 0;
    UINT32               trbs_req      = 0;
    UINT8                index         = 0;
    BOOLEAN              direction;

    /* Parameters validation .*/
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(stream);

    xhci      = (NU_USBH_XHCI *)cb;
    slot_id   = xhci->device_table[function_address];
    irp       = &stream->irp;
    stream_id = stream->stream_id;

    /* Get the xHC driver based endpoint index form the endpoint address.
     * xHCI specs, section 4.5.1
     */
    ep_index = XHCI_EP_ADDR_TO_EP_INDEX(bEndpointAddress);

    /* Endpoint direction, 0 for IN 1 for OUT.*/
    direction = bEndpointAddress & 0x80 ? NU_FALSE : NU_TRUE;

    status = NU_USBH_XHCI_Lock( xhci );
    if ( status == NU_SUCCESS )
    {
        status = XHCI_Get_Device_Handle(xhci, slot_id, &xhci_dev);
        if ( status == NU_SUCCESS )
        {
            status = XHCI_Get_Endpoint_Handle(xhci_dev, ep_index, &ep_info);
            if ( status == NU_SUCCESS )
            {
                status = XHCI_Get_Endpoint_Context( xhci_dev->dev_out_ctx, &ep_ctx, ep_index );
                if ( status == NU_SUCCESS )
                {
                    ep_max_pack = EP_CTX_MAX_PACKET_SIZE(LE32_2_HOST(ep_ctx->ep_info2));

                    status = XHCI_Get_Stream_Ring_handle( ep_info->stream_info, stream_id, &ring );
                    if ( status == NU_SUCCESS )
                    {
                        /* Find the number of TRBs required from the IRP data length. */
                        for (index = 0 ; index < irp->length ; index += XHCI_MAX_TRB_SIZE)
                        {
                            ++trbs_req;
                        }

                        /* One additional TRB for event data TRB.*/
                        ++trbs_req;

                        /* Check if there is room available on the ring for queuing the TRBs. */
                        status = XHCI_Room_On_Ring( xhci, ring, trbs_req ,
                                                    XHCI_TRANS_RING_ALIGNMENT );
                        if ( status == NU_SUCCESS )
                        {
                            /* Construct the td against the TRBs.One additional TRB is required for
                             * event data TRB.
                             */
                            status = XHCI_Process_Bulk_IRP( xhci, irp, ring, stream_id, ep_max_pack,
                                                            trbs_req, direction, NU_FALSE );
                            if ( status == NU_SUCCESS )
                            {
                                /* Ring the endpoint doorbell to initiate the data transfer. */
                                status = XHCI_Ring_Endpoint_Doorbell( xhci, slot_id,
                                                                      stream_id, ep_index );
                            }
                        }
                    }
                 }
            }
        }

        int_status =  NU_USBH_XHCI_Unlock( xhci );
    }

    return ( (( status == NU_SUCCESS ) ? int_status : status) );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_ISR
*
* DESCRIPTION
*
*       This function processes the interrupt from the XHCI controller.
*
* INPUTS
*
*       cb                                  XHCI controller which generated
*                                           the interrupt.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always.
*
**************************************************************************/
STATUS NU_USBH_XHCI_ISR(NU_USB_HW *cb)
{
    /* xHCI control block handle. */
    NU_USBH_XHCI          *xhci    = NU_NULL;

    /* Operational registers base address pointer. */
    NU_USBH_XHCI_OP_REGS  *op_reg  = NU_NULL;

    /* Run Time registers base address pointer. */
    NU_USBH_XHCI_RUN_REGS *rt_reg  = NU_NULL;

    /* Register values. */
    UINT32                reg_val1 = 0;
    UINT32                reg_val2 = 0;

    /* Parameter validation. */
    NU_USB_PTRCHK(cb);

    xhci = (NU_USBH_XHCI *)cb;

    /* Get the Operational registers set handle .*/
    (VOID)XHCI_Get_Operational_Regs_Handle( &xhci->reg_base, &op_reg );

    /* Get the Run Time registers set handle .*/
    (VOID)XHCI_Get_Run_Time_Regs_Handle( &xhci->reg_base, &rt_reg );

    /* Read the USB status register .*/
    XHCI_HW_READ32(xhci, &op_reg->xhci_usb_status, reg_val1);

    /* Read the IMAN register .*/
    XHCI_HW_READ32(xhci, &rt_reg->ir_set[0].xhci_iman, reg_val2);

    /* Check if IP and EINT flags are asserted - which actually generate
     * interrupt.
     */
    if ( reg_val1 & XHCI_STATUS_EINT )
    {
        /* Since the IP and EINT are asserted, proceed with the
         * interrupt processing.
         */
        /* Clear the Event Interrupt Flag of USBSTS register. - Write
         * 1 to clear ,xhci specs, section 5.4.2.
         */
        reg_val1 |= XHCI_STATUS_EINT;
        XHCI_HW_WRITE32(xhci, &op_reg->xhci_usb_status, reg_val1);
        XHCI_HW_READ32(xhci, &op_reg->xhci_usb_status, reg_val1);
        /* Clear the Interrupt Pending flag of the IMAN register. - Write
         * 1 to clear.
         */
        reg_val2 |= XHCI_IMAN_IRQ_PENDING;
        XHCI_HW_WRITE32(xhci, &rt_reg->ir_set[0].xhci_iman, reg_val2);
        XHCI_HW_READ32(xhci, &rt_reg->ir_set[0].xhci_iman, reg_val2);

        /* The event handler function processes all the event TRBs
         * until the ring is empty.
         */
        XHCI_Event_Handler( xhci );

    }

    return (NU_SUCCESS);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Enable_Interrupts
*
* DESCRIPTION
*
*       This function enables the interrupts from the hardware.
*
* INPUTS
*
*       cb                                  XHCI controller to enable the
*                                           interrupts for.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Enable_Interrupts(NU_USB_HW *cb)
{
    /* Progress status .*/
    STATUS                status    = NU_SUCCESS;

    /* xHCI control block handle. */
    NU_USBH_XHCI          *xhci     = NU_NULL;

    /* Parameter validation. */
    NU_USB_PTRCHK(cb);

    xhci = (NU_USBH_XHCI *)cb;

    USBH_XHCI_UNPROTECT

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Disable_Interrupts
*
* DESCRIPTION
*
*       This function disables the interrupts from the hardware.
*
* INPUTS
*
*       cb                                  XHCI controller to disable the
*                                           interrupts for.
*
* OUTPUTS
*
*       NU_SUCCESS                         Always.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Disable_Interrupts(NU_USB_HW *cb)
{
    /* Progress status .*/
    STATUS                status    = NU_SUCCESS;

    /* xHCI control block handle. */
    NU_USBH_XHCI          *xhci     = NU_NULL;

    /* Parameter validation. */
    NU_USB_PTRCHK(cb);
    xhci = (NU_USBH_XHCI *)cb;

    if ( xhci->int_disable_count == NU_NULL )
    {
        if ( status == NU_SUCCESS )
        {
            USBH_XHCI_PROTECT
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Update_device
*
* DESCRIPTION
*
*       This function updates the parameters of device/endpoints
*       like Max Packet size, System Exit Latency etc.
*
* INPUTS
*
*       cb                                Host controller control block
*                                         Handle.
*       usb_device                        USB Device handle
*       packet_size                       EP0 Maximum packet size, required
*                                         to update for full and low speed
*                                         devices.
*       sel                               Host-to-Device exit latency required
*                                         if there is need to set exite latency.
*       is_hub                            NU_TRUE if device is hub.
*       tt_time                           Think time for tt hub.
*       num_ports                         Number of ports in case of hub.
*
* OUTPUTS
*
*       NU_SUCCESS                        Always.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Update_device(NU_USBH_HW    *cb,
                                  NU_USB_DEVICE *usb_device,
                                  UINT8          packet_size,
                                  UINT32         sel,
                                  BOOLEAN        is_hub,
                                  UINT8          tt_time,
                                  UINT8          num_ports)
{
    /* Progress status. */
    STATUS                status         = NU_SUCCESS;
    STATUS                int_status     = NU_SUCCESS;

    /* Pointer to xHCI control block.*/
    NU_USBH_XHCI          *xhci          = NU_NULL;

    /* xHCI device pointer , required for referencing device context DS. */
    NU_USBH_XHCI_DEVICE   *xhci_device   = NU_NULL;

    /* Pointer device slot context. */
    NU_USBH_XHCI_SLOT_CTX *slot_ctx      = NU_NULL;

    /* Pointer to device endpoint context. */
    NU_USBH_XHCI_EP_CTX   *ep_ctx        = NU_NULL;

    /* Pointer to device input control context. */
    NU_USBH_XHCI_CTRL_CTX *ctrl_ctx      = NU_NULL;

    /* Slot ID associated with the device. */
    UINT8                 slot_id        = 0;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(usb_device);

    xhci = ( NU_USBH_XHCI *)cb;
    slot_id = xhci->device_table[usb_device->function_address];
    
    status = NU_USBH_XHCI_Lock( xhci );

    if ( status == NU_SUCCESS )
    {
        /* Copy the endpoint and slot context. */
        status = XHCI_Get_Device_Handle( xhci, slot_id, &xhci_device );
    
        if ( status == NU_SUCCESS )
        {
            /* Copy the out endpoint context into IN endpoint context. This is helpful
             * when some of the the parameters of input context are to be changed by
             * the config endpoint command.
             */
            status = XHCI_Copy_Endpoint_Context( xhci, xhci_device->dev_in_ctx,
                                                 xhci_device->dev_out_ctx, 0x00 );
            if ( status == NU_SUCCESS )
            {
                status = XHCI_Copy_Slot_Context( xhci, xhci_device->dev_in_ctx,
                                                 xhci_device->dev_out_ctx ) ;
                if ( status == NU_SUCCESS )
                {
                    status = XHCI_Get_Slot_Context( xhci_device->dev_in_ctx, &slot_ctx );
    
                    if ( status == NU_SUCCESS )
                    {
                        status = XHCI_Get_Endpoint_Context( xhci_device->dev_in_ctx,
                                                            &ep_ctx, 0x00 );
                        if ( status ==  NU_SUCCESS )
                        {
                            status = XHCI_Get_Input_Control_Context( xhci_device->dev_in_ctx,
                                                                     &ctrl_ctx );
                            if ( status == NU_SUCCESS )
                            {
                                /* Depending on the input parameters update the input context.*/
                                if ( packet_size )
                                {
                                    ctrl_ctx->add_flags &= HOST_2_LE32(~(UINT32) SLOT_CTX_SLOT_FLAG);
                                    ctrl_ctx->add_flags |= HOST_2_LE32(SLOT_CTX_EP0_FLAG);
                                    ep_ctx->ep_info2    &= HOST_2_LE32(~EP_CTX_MAX_PACKET_MASK);
                                    ep_ctx->ep_info2    |= HOST_2_LE32(EP_CTX_SET_MAX_PACKET(packet_size));
    
                                    status = XHCI_Queue_Evaluate_Context_Command( xhci,
                                                                                  xhci_device->dev_in_ctx,
                                                                                  slot_id );
                                }
    
                                if ( sel )
                                {
                                    /* Update host to device exit latency field. */
                                    ctrl_ctx->add_flags = HOST_2_LE32(SLOT_CTX_EP0_FLAG);
                                    slot_ctx->device_info1 |= HOST_2_LE32 (SLOT_CTX_MAX_EXIT_LAT & sel);
    
                                    status = XHCI_Queue_Evaluate_Context_Command( xhci,
                                                                                  xhci_device->dev_in_ctx,
                                                                                  slot_id );
                                }
    
                                if ( is_hub )
                                {
                                    /* Update the hub device field. */
                                    ctrl_ctx->add_flags    = HOST_2_LE32(SLOT_CTX_SLOT_FLAG);
                                    slot_ctx->device_info1 |= HOST_2_LE32(SLOT_CTX_DEV_HUB);
                                    slot_ctx->device_info2 |= HOST_2_LE32(SLOT_CTX_XHCI_MAX_PORTS(num_ports));
                                    slot_ctx->tt_intr_info |= HOST_2_LE32(SLOT_CTX_TT(tt_time));
    
                                    /* xHCI specs v0.95 requires use of Evaluate Context command for updating
                                     * hub info, while v0.96 requires Config Endpoint command.
                                     */
                                    if ( xhci->info_hub.xhci_specs_ver == XHCI_SPECS_0_95)
                                    {
                                        status = XHCI_Queue_Evaluate_Context_Command( xhci,
                                                                                      xhci_device->dev_in_ctx,
                                                                                      slot_id );
                                    }
                                    else
                                    {
                                        status = XHCI_Queue_Configure_EP_Command( xhci,
                                                                                  xhci_device->dev_in_ctx,
                                                                                  slot_id );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        int_status =  NU_USBH_XHCI_Unlock( xhci );
    }

    return ( (( status == NU_SUCCESS ) ? int_status : status) );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Modify_Pipe
*
* DESCRIPTION
*
*       Modifies endpoint descriptors in the H/W and makes them ready for
*       scheduling transfers.
*
* INPUTS
*
*       cb                                  Handle that identifies the HC
*                                           in the xHCI HC database.
*       function_address                    Identifies the device on which
*                                           the pipe resides.
*       bEndpointAddress                    Endpoint which owns the pipe
*       bmAttributes                        Identifies the endpoint type.
*       speed                               Device speed.
*                                           USB_SPEED_LOW/USB_SPEED_FULL.
*       ep_max_size                         Maximum packet size the
*                                           endpoint should support.
*       interval                            Interval in micro seconds for
*                                           periodic endpoints.
*       load                                Frame b/w consumed by the
*                                           endpoint.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always
*
**************************************************************************/
STATUS NU_USBH_XHCI_Modify_Pipe(NU_USB_HW *cb,
                                 UINT8 function_address,
                                 UINT8 bEndpointAddress,
                                 UINT8 bmAttributes,
                                 UINT16 ep_max_size,
                                 UINT32 interval,
                                 UINT32 load)
{
    /* Progress status. */
    STATUS                status         = NU_SUCCESS;
    STATUS                int_status     = NU_SUCCESS;

    /* Pointer to xHCI control block.*/
    NU_USBH_XHCI          *xhci          = NU_NULL;

    /* xHCI device pointer , required for referencing device context DS. */
    NU_USBH_XHCI_DEVICE   *xhci_device   = NU_NULL;

    /* Pointer device slot context. */
    NU_USBH_XHCI_SLOT_CTX *slot_ctx      = NU_NULL;

    /* Slot ID associated with the device. */
    UINT8                 slot_id        = 0;
    
    UINT8                 speed = 0;

    NU_USB_PTRCHK(cb);

    xhci = ( NU_USBH_XHCI *)cb;
    slot_id = xhci->device_table[function_address];

    /* Lock the xHCI driver for exclusive access. */
    status = NU_USBH_XHCI_Lock( xhci );

    if ( status == NU_SUCCESS )
    {
        status = XHCI_Get_Device_Handle( xhci, slot_id, &xhci_device );
	    if ( status == NU_SUCCESS )
	    {
	        status = XHCI_Get_Slot_Context( xhci_device->dev_out_ctx, &slot_ctx );
	        if ( status == NU_SUCCESS )
	        {
	            status = NU_USBH_XHCI_Close_Pipe( cb, function_address, bEndpointAddress );
	            if ( status == NU_SUCCESS )
	            {
	                /* Get the device speed from the output slot context. */
	                speed = LE32_2_HOST(SLOT_CTX_DEV_SPEED(slot_ctx->device_info1));
	
	                /* The speed encoding  used by the xHCI is different from the USB
	                 * stack. LS encoding for USB stack maps to FS and vice versa.
	                 * Encodings for HS and SS are the same.
	                 */
	                if ( speed == XHCI_SLOT_SPEED_LOW )
	                {
	                    speed = USB_SPEED_FULL;
	                }
	                else if ( speed == XHCI_SLOT_SPEED_FULL )
	                {
	                    speed = USB_SPEED_LOW;
	                }
	
	                /* Open endpoint with new parameters. */
	                status = NU_USBH_XHCI_Open_Pipe( cb, function_address, bEndpointAddress,
	                                                 bmAttributes, speed, ep_max_size,
	                                                 interval, load );
	            }
	        }
		}
        int_status =  NU_USBH_XHCI_Unlock( xhci );
    }

    return ( (( status == NU_SUCCESS ) ? int_status : status) );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Reset_Bandwidth
*
* DESCRIPTION
*
*       Resets the bandwidth of controller.
*
* INPUTS
*
*       cb                                  Handle that identifies the HC
*                                           in the xHCI HC database.
*       function_address                    Identifies the device on which
*                                           the pipe resides.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Reset_Bandwidth(NU_USBH_HW *cb,
                                    UINT8 function_address)
{
    return (NU_SUCCESS);
}

/* ======================== End of File ================================ */
