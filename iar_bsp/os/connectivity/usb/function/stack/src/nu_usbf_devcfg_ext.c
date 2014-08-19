/**************************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*       nu_usbf_devcfg_ext.c
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the implementation of the exported functions
*       for dynamic descriptor loading and unloading.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBF_DEVICE_Create_Config
*       NU_USBF_DEVICE_Register_Function
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
#ifndef USBF_DEVCFG_EXT_C
#define USBF_DEVCFG_EXT_C
/* ==============  USB Include Files ==================================  */

#include    "connectivity/nu_usb.h"

/* ================= Data Structures =================================== */

USBF_DEVICE_CONFIG      usbf_device_cfg;
USBF_STACK_EP_MAP       usbf_stack_ep_map;
NU_USB_DEVICE           usbf_device;

static STATUS usbf_get_free_string(USBF_STRING**);
static STATUS usbf_make_unicode_string(CHAR*, CHAR*, UINT8);
static BOOLEAN usbf_check_empty_config(NU_USB_DEVICE*, UINT8);
static STATUS usb_get_speed_incices(NU_USB_DEVICE*, UINT8*, UINT8*);

/* =================== Composite Device API ============================ */

STATUS USBF_DEVCFG_Device_Initialize(NU_USB_DEVICE *device,
                                    UINT16 vend_id,
                                    UINT16  prod_id,
                                    CHAR    *manuf_str,
                                    CHAR    *prod_str,
                                    CHAR    *sernum_str)
{
    NU_USB_DEVICE_DESC dev_desc;
    STATUS status;

    NU_USB_PTRCHK(device);

    memset((UINT8*)device, 0x00, sizeof(NU_USB_DEVICE));

    dev_desc.bLength            = 18;
    dev_desc.bDescriptorType    = USB_DT_DEVICE;
    dev_desc.bcdUSB             = BCD_USB_VERSION_11;
    dev_desc.bDeviceClass       = 0;
    dev_desc.bDeviceSubClass    = 0;
    dev_desc.bDeviceProtocol    = 0;
    dev_desc.bMaxPacketSize0    = 0x08;
    dev_desc.idVendor           = vend_id;
    dev_desc.idProduct          = prod_id;
    dev_desc.bcdDevice          = 0x0001;
    dev_desc.iManufacturer      = 0;
    dev_desc.iProduct           = 0;
    dev_desc.iSerialNumber      = 0;
    dev_desc.bNumConfigurations = 0;

    status = USBF_DEVCFG_Initialize(device);
    if ( status == NU_SUCCESS )
    {
        status = NU_USB_DEVICE_Set_Desc(device, &dev_desc);
        if ( status == NU_SUCCESS )
        {
            status = USBF_DEVCFG_Add_Lang_String(device);
            if ( status == NU_SUCCESS )
            {
                if ( (status == NU_SUCCESS) && (manuf_str != NU_NULL) )
                {
                    status = USBF_DEVCFG_Add_Manuf_String(device, manuf_str);
                }

                if ( (status == NU_SUCCESS) && (prod_str != NU_NULL) )
                {
                    status = USBF_DEVCFG_Add_Product_String(device, prod_str);
                }

                if ( (status == NU_SUCCESS) && (sernum_str != NU_NULL) )
                {
                    status = USBF_DEVCFG_Add_Serial_String(device, sernum_str);
                }
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Initialize
*
* DESCRIPTION
*
*       This function initializes the USB device configuration component.
*       This is called by Nucleus USB function stack immediately after
*       creating an instance of NU_USB_DEVICE.
*
* INPUTS
*
*       device                              Pointer to NU_USB_DEVICE
*                                           control block. NU_USB_DEVICE
*                                           actually represents a device
*                                           in Nucleus USB.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates that USB device
*                                           configuration component has
*                                           been initialized successfully.
*       NU_NOT_PRESETNT                     Either USB device
*                                           configuration component does
*                                           not exist or already
*                                           initialzed.
*
**************************************************************************/
STATUS USBF_DEVCFG_Initialize(NU_USB_DEVICE *device)
{
    NU_USB_PTRCHK(device);

    if ( usbf_device_cfg.device == NU_NULL )
    {
        memset((UINT8*)&usbf_device_cfg,
                0x00,
                sizeof(USBF_DEVICE_CONFIG));
        usbf_device_cfg.device = device;

        /* Indicate successfuly create of composite device CB. */
        return ( NU_SUCCESS );
    }

    /* Composite device is already created. */
    return ( NU_NOT_PRESENT );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Uninitialize
*
* DESCRIPTION
*
*       This function Uninitializes the USB device configuration component.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       NU_SUCCESS                          Function always returns
*                                           NU_SUCCESS.
*
**************************************************************************/
STATUS USBF_DEVCFG_Uninitialize(VOID)
{
    /* Reset contents of cmposite device control block. */
    usbf_device_cfg.device = NU_NULL;
    memset((UINT8*)&usbf_device_cfg,
        0x00,
        sizeof(USBF_DEVICE_CONFIG));

    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Get_Handle
*
* DESCRIPTION
*
*       This function returns pointer to USBF_DEVICE_CONFIG control
*       block which forms the basic device configuration inforamation
*       layer.
*
* INPUTS
*
*       devcfg_cb                           Pointer to USBF_DEVICE_CONFIG.
*                                           This points to a valid address
*                                           of USB device configuration
*                                           control block when function
*                                           returns.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*       NU_NOT_PRESENT                      USB device configuration
*                                           component is not initialized.
*
**************************************************************************/
STATUS USBF_DEVCFG_Get_Handle(USBF_DEVICE_CONFIG **devcfg_cb)
{
    NU_USB_PTRCHK(devcfg_cb);

    if ( usbf_device_cfg.device != NU_NULL )
    {
        *devcfg_cb = &usbf_device_cfg;
        return ( NU_SUCCESS );
    }

    return ( NU_NOT_PRESENT );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Get_Device_Handle
*
* DESCRIPTION
*
*       This function returns pointer to NU_USB_DEVICE control
*       block.
*
* INPUTS
*
*       devc_cb                             Pointer to NU_USB_DEVICE
*                                           control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Get_Device_Handle(NU_USB_DEVICE **dev_cb)
{
    NU_USB_PTRCHK(dev_cb);

    *dev_cb = &usbf_device;
    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Add_Function
*
* DESCRIPTION
*
*       Adds a new function to USB device configuration repository.
*       This function is called by class driver during creation to
*       register its interface and class specific descriptor with USB
*       device configuration information.
*       Function allocates an instance of USB_FUNCTION control block for
*       each class driver which contains all relevant information of caller
*       class.
*       This function only registers the descriptors in device
*       configuration information base, desciptors are only puplated in
*       NU_USB_DEVICE when application tries to use a particular class.
*       When a function is added initially it is disabled and is not
*       considred for binding with NU_USB_DEVICE unless it is enabled,
*
* INPUTS
*
*       config_index                        Device configuration index.
*       raw_descriptor                      Pointer to a buffer containing
*                                           raw interface and class
*                                           sepecific descriptors of class
*                                           caller class.
*       raw_desc_len                        Length of data in buffer
*                                           pointed by 'raw_descriptor'.
*       func_out                            Pointer to USBF_FUNCTION
*                                           control block pointing to a
*                                           valid USB_FUNCTION control
*                                           block when function returns.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Add_Function(UINT8   config_index,
                                UINT8   *fs_raw_descriptor,
                                UINT16  fs_raw_desc_len,
                                UINT8   *hs_raw_descriptor,
                                UINT16  hs_raw_desc_len,
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                UINT8   *ss_raw_descriptor,
                                UINT16  ss_raw_desc_len,
#endif
                                USBF_FUNCTION **func_out)
{
    STATUS          status;
    UINT8           index;
    USBF_FUNCTION   *usb_function;

    NU_USB_PTRCHK(fs_raw_descriptor);
    NU_USB_PTRCHK(hs_raw_descriptor);
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    NU_USB_PTRCHK(ss_raw_descriptor);
#endif
    NU_USB_PTRCHK(func_out);
    NU_USB_PTRCHK(usbf_device_cfg.device);

    status  = NU_USB_INVLD_ARG;
    if ( config_index < USBF_MAX_CONFIGURATIONS )
    {
        status = NU_NOT_PRESENT;
        for(index = 0; index < USBF_MAX_FUNCTIONS; index++ )
        {
            usb_function = &usbf_device_cfg.usb_functions[config_index][index];
            if(usb_function->is_used == NU_FALSE)
            {
                usb_function->intf_raw_desc[USB_SPEED_FULL] = fs_raw_descriptor;
                usb_function->desc_len[USB_SPEED_FULL]      = fs_raw_desc_len;
                usb_function->intf_raw_desc[USB_SPEED_HIGH] = hs_raw_descriptor;
                usb_function->desc_len[USB_SPEED_HIGH]      = hs_raw_desc_len;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                usb_function->intf_raw_desc[USB_SPEED_SUPER] = ss_raw_descriptor;
                usb_function->desc_len[USB_SPEED_SUPER]      = ss_raw_desc_len;
#endif
                usb_function->config_index  = config_index;
                usb_function->func_index    = index;
                usb_function->device        = usbf_device_cfg.device;
                usb_function->is_enabled    = NU_FALSE;
                usb_function->is_used       = NU_TRUE;

                *func_out = usb_function;

                status = NU_SUCCESS;
                break;
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Delete_Function
*
* DESCRIPTION
*
*       Deletes a previously registered interface descriptor of a class
*       from USB device configuration repository.
*
* INPUTS
*
*       cb                                  Pointer to USBF_FUNCTION
*                                           control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Delete_Function(USBF_FUNCTION *cb)
{
    NU_USB_PTRCHK(cb);

    cb->is_enabled  = NU_FALSE;
    cb->is_used     = NU_FALSE;

    /* Reset contents of USB function slot. */
    memset((UINT8*)cb, 0x00, sizeof(USBF_FUNCTION));

    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Enable_Function
*
* DESCRIPTION
*
*       Enables a USB function for binding with NU_USB_DEVICE instance
*       so that associate interface and class specific descriptors are
*       included in configuration descriptor.
*
* INPUTS
*
*       cb                                  Pointer to USBF_FUNCTION
*                                           control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Enable_Function(USBF_FUNCTION *cb)
{
    NU_USB_PTRCHK(cb);

    if ( cb->is_used == NU_TRUE )
    {
        cb->is_enabled  = NU_TRUE;
        return ( NU_SUCCESS );
    }

    return ( NU_USB_INVLD_ARG );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Disable_Function
*
* DESCRIPTION
*
*       Disables a USB function for binding with NU_USB_DEVICE instance
*       so that associate interface and class specific descriptors are
*       not included in configuration descriptor.
*
* INPUTS
*
*       cb                                  Pointer to USBF_FUNCTION
*                                           control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Disable_Function(USBF_FUNCTION *cb)
{
    NU_USB_PTRCHK(cb);

    if ( cb->is_used == NU_TRUE )
    {
        cb->is_enabled  = NU_FALSE;
        return ( NU_SUCCESS );
    }

    return ( NU_USB_INVLD_ARG );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Bind_Function
*
* DESCRIPTION
*
*       This functions binds a pre registered function with NU_USB_DEVICE
*       instance. Binding includes adding associated interface and class
*       specific descriptors in specific configuration, alligning interface
*       numbers and acquire endpoints for each alternate setting.
*       Once registered, a USB function is disable by default. It is the
*       responsibility of caller to enable it first before performing a
*       bind operation on it.
*
* INPUTS
*
*       cb                                  Pointer to USBF_FUNCTION
*                                           control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Bind_Function(USBF_FUNCTION *cb)
{
    STATUS  status;
    UINT8   *tmp_buf, *tmp_buff_new, *buffer;
    UINT16  config_len, maxp;
    UINT8   num_interfaces, endp_num, interval;
    UINT16  size;
    UINT16  temp_length;
    UINT8   *uint8_ptr, spd_start_idx, spd_end_idx, speed_index;
    NU_USB_HDR       *hdr;
    NU_USB_CFG_DESC  *cfg_descriptor;
    NU_USB_INTF_DESC *intf_descriptor = NU_NULL;
    NU_USB_ENDP_DESC *endp_descriptor = NU_NULL;
    NU_USB_IAD_DESC  *iad_descriptor  = NU_NULL;
    NU_USB_DEVICE    *device;
    NU_USBF_STACK    *usbf_stack;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(cb->device);

    status = NU_USB_INVLD_ARG;
    if ( (cb->is_used == NU_TRUE) || (cb->is_enabled == NU_TRUE) )
    {
        if ( cb->config_index < USBF_MAX_CONFIGURATIONS )
        {
            device  = cb->device;

            status = usb_get_speed_incices(device, &spd_start_idx, &spd_end_idx);
            if ( status == NU_SUCCESS )
            {
                for(speed_index = spd_start_idx; speed_index <= spd_end_idx; speed_index++ )
                {
                    tmp_buf = device->raw_descriptors[cb->config_index][speed_index];
                    if ( tmp_buf != NU_NULL )
                    {
                        if ( cb->intf_raw_desc[speed_index] != NU_NULL )
                        {
                            cfg_descriptor = (NU_USB_CFG_DESC*)tmp_buf;

                            /* Allocate enough memory to save newly registered descriptors. */
                            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                                    (cb->desc_len[speed_index] + HOST_2_LE16(cfg_descriptor->wTotalLength)),
                                                    (VOID**)&tmp_buff_new);
                            if ( (status == NU_SUCCESS) && (tmp_buff_new != NU_NULL) )
                            {
                                /* Get length of configuration descriptor. */
                                uint8_ptr = (UINT8*)&cfg_descriptor->wTotalLength;
                                config_len = (UINT8)*uint8_ptr++;
                                config_len |= (UINT8)*uint8_ptr;

                                /* Copy existing configuration descriptor to newly allocated buffer. */
                                memcpy(tmp_buff_new, tmp_buf, config_len);

                                /* Copy raw descriptor in configuration descriptor. */
                                memcpy((tmp_buff_new + config_len), cb->intf_raw_desc[speed_index], cb->desc_len[speed_index]);

                                /* Point config descriptor pointer to newly allocated buffer. */
                                cfg_descriptor = (NU_USB_CFG_DESC*) tmp_buff_new;

                                num_interfaces  = cfg_descriptor->bNumInterfaces;
                                size            = cb->desc_len[speed_index];
                                buffer          = (tmp_buff_new + config_len);

                                while( (size > 2) && (status == NU_SUCCESS) )
                                {
                                    hdr = (NU_USB_HDR *) buffer;
                                    if ((hdr->bLength > size) || (hdr->bLength == 0))
                                    {
                                        status = NU_USB_INVLD_DESC;
                                        break;
                                    }

                                    switch (hdr->bDescriptorType)
                                    {
                                    case USB_DT_INTERFACE_ASSOC:
                                        iad_descriptor = (NU_USB_IAD_DESC*) buffer;

                                        /* Set Class, subclass and protocol of device. */
                                        device->device_descriptor.bDeviceClass      = MULTI_INTERFACE_FUNC_CLASS;
                                        device->device_descriptor.bDeviceSubClass   = MULTI_INTERFACE_FUNC_SUBCLASS;
                                        device->device_descriptor.bDeviceProtocol   = MULTI_INTERFACE_FUNC_PROTOCOL;

                                        iad_descriptor->bFirstInterface = num_interfaces;
                                        break;
                                    case USB_DT_INTERFACE:
                                        intf_descriptor = (NU_USB_INTF_DESC*) buffer;

                                        /* Only treat it as a new interface if alternate setting is zero. */
                                        if ( intf_descriptor != NU_NULL )
                                        {
                                            if ( intf_descriptor->bAlternateSetting == 0 )
                                                intf_descriptor->bInterfaceNumber = num_interfaces++;
                                            else
                                                intf_descriptor->bInterfaceNumber = (num_interfaces - 1);
                                        }
                                        else
                                        {
                                            status = NU_USB_INVLD_DESC;
                                        }
                                        break;
                                    case USB_DT_ENDPOINT:
                                        endp_descriptor = (NU_USB_ENDP_DESC*) buffer;

                                        if ( (intf_descriptor != NU_NULL) && (endp_descriptor != NU_NULL) )
                                        {
                                                status = NU_USBF_HW_Acquire_Endp((NU_USBF_HW*)device->hw,
                                                                        speed_index,
                                                                        endp_descriptor->bmAttributes,
                                                                        (endp_descriptor->bEndpointAddress & 0x80),
                                                                        (cb->config_index + 1),
                                                                        intf_descriptor->bInterfaceNumber,
                                                                        intf_descriptor->bAlternateSetting,
                                                                        &endp_num,
                                                                        &maxp,
                                                                        &interval);
                                            if ( status == NU_SUCCESS )
                                            {
                                                endp_descriptor->bEndpointAddress |= endp_num;
                                                endp_descriptor->wMaxPacketSize0 = (UINT8)(maxp);
                                                endp_descriptor->wMaxPacketSize1 = (UINT8)(maxp >> 8);
                                                endp_descriptor->bInterval = interval;
                                            }
                                        }
                                        else
                                        {
                                            status = NU_USB_INVLD_DESC;
                                        }
                                        break;
                                    }

                                    size    -= hdr->bLength;
                                    buffer  += hdr->bLength;
                                }

                                if ( status == NU_SUCCESS )
                                {
                                    /* Save number of interfaces in configuration descriptor. */
                                    cfg_descriptor->bNumInterfaces = num_interfaces;

                                    /* Add length of newly registered descriptor to total length of configuration
                                     * descriptor.
                                     */
                                    temp_length = HOST_2_LE16(cfg_descriptor->wTotalLength);
                                    temp_length += cb->desc_len[speed_index];
                                    tmp_buff_new[2] = (UINT8)temp_length;
                                    tmp_buff_new[3] = (UINT8)(temp_length >> 8);

                                    /* Save pointer to newly allocated raw descriptor buffer. */
                                    device->raw_descriptors[cb->config_index][speed_index] = tmp_buff_new;

                                    /* Deallocate old buffer. */
                                    USB_Deallocate_Memory(tmp_buf);
                                }
                            }
                        }
                    }
                }
            }

            /* There was an error in execution then retain the old descriptors. */
            if ( status == NU_SUCCESS )
            {
                status = NU_USBF_Init_GetHandle((VOID**) &usbf_stack);
                if ( status == NU_SUCCESS )
                {
                    /* Detach device first. */
                    NU_USBF_STACK_Detach_Device(usbf_stack, device);

                    /* Call attach device to let the stack parse and setup descriptors. */
                    status = NU_USBF_STACK_Attach_Device(usbf_stack, device);
                }
                else
                {
                    for(speed_index = 0; speed_index < NU_USB_MAX_SPEEDS; speed_index++ )
                    {
                        tmp_buf = device->raw_descriptors[cb->config_index][speed_index];
                        if(tmp_buf != NU_NULL)
                        {
                            USB_Deallocate_Memory(tmp_buf);
                            device->raw_descriptors[cb->config_index][speed_index] = NU_NULL;
                        }
                    }

                    status = USBF_DEVCFG_Recreate_Config(cb->device, cb->config_index);
                }
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Unbind_Function
*
* DESCRIPTION
*
*       This functions Unbinds a binded function with NU_USB_DEVICE
*       instance. Once unbound a USB function is disabled and can not
*       be bound again unless it is enabled again.
*
* INPUTS
*
*       cb                                  Pointer to USBF_FUNCTION
*                                           control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Unbind_Function(USBF_FUNCTION *cb)
{
    STATUS          status;
    UINT8           *raw_desc, *buffer, speed_index, index;
    UINT16          size;
    NU_USB_DEVICE   *device;
    USBF_FUNCTION   *usbf_func;
    NU_USB_CFG_DESC *cfg_descriptor;
    NU_USB_INTF_DESC *intf_descriptor = NU_NULL;
    NU_USB_ENDP_DESC *endp_descriptor = NU_NULL;
    NU_USB_HDR      *hdr;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(cb->device);

    status = NU_USB_INVLD_ARG;
    device = cb->device;
    if ( (cb->is_used == NU_TRUE) || (cb->is_enabled == NU_TRUE) )
    {
        /* Detach device first. */
        status = NU_USBF_STACK_Detach_Device((NU_USBF_STACK*)(device->stack), device);
        if ( status == NU_SUCCESS )
        {
            if ( cb->config_index < USBF_MAX_CONFIGURATIONS )
            {
                for(speed_index = 0; speed_index < NU_USB_MAX_SPEEDS; speed_index++ )
                {
                    raw_desc = device->raw_descriptors[cb->config_index][speed_index];
                    if ( raw_desc != NU_NULL )
                    {
                        cfg_descriptor  = (NU_USB_CFG_DESC*)raw_desc;
                        size            = cfg_descriptor->wTotalLength;
                        buffer          = raw_desc;

                        while( (size > 2) && (status == NU_SUCCESS) )
                        {
                            hdr = (NU_USB_HDR *) buffer;
                            if ((hdr->bLength > size) || (hdr->bLength == 0))
                            {
                                status = NU_USB_INVLD_DESC;
                                break;
                            }

                            switch (hdr->bDescriptorType)
                            {
                            case USB_DT_INTERFACE:
                                intf_descriptor = (NU_USB_INTF_DESC*) buffer;
                                if ( intf_descriptor == NU_NULL )
                                {
                                    status = NU_USB_INVLD_DESC;
                                }
                                break;
                            case USB_DT_ENDPOINT:
                                endp_descriptor = (NU_USB_ENDP_DESC*) buffer;

                                if ( (intf_descriptor != NU_NULL) && (endp_descriptor != NU_NULL) )
                                {
                                    status = NU_USBF_HW_Release_Endp((NU_USBF_HW*)device->hw,
                                                                speed_index,
                                                                endp_descriptor->bEndpointAddress,
                                                                (cb->config_index + 1),
                                                                intf_descriptor->bInterfaceNumber,
                                                                intf_descriptor->bAlternateSetting);
                                }
                                else
                                {
                                    status = NU_USB_INVLD_DESC;
                                }
                                break;
                            }

                            size    -= hdr->bLength;
                            buffer  += hdr->bLength;
                        }
                    }
                }

                if ( status == NU_SUCCESS )
                {
                    /* Recreate configuration. */
                    status = USBF_DEVCFG_Recreate_Config(cb->device, cb->config_index);
                    if ( status == NU_SUCCESS )
                    {
                        /* Disable the function slot. */
                        cb->is_enabled = NU_FALSE;

                        /* Bind remaining interfaces again. */
                        for(index = 0; index < USBF_MAX_FUNCTIONS; index++ )
                        {
                            usbf_func = &usbf_device_cfg.usb_functions[cb->config_index][index];
                            if ( usbf_func )
                            {
                                if ( (usbf_func->is_enabled == NU_TRUE) && (usbf_func->is_used == NU_TRUE) )
                                {
                                    status = USBF_DEVCFG_Bind_Function(usbf_func);
                                    if ( status != NU_SUCCESS )
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Create_Config
*
* DESCRIPTION
*
*       This functions creates a new configuration in NU_USB_DEVICE
*       control block and return its index as an output argument.
*
* INPUTS
*
*       device                              Pointer to NU_USB_DEVICE
*                                           control block. It actually
*                                           reporesents a USB device.
*       config_index_out                    Output argument containing
*                                           configuration index of newly
*                                           created configuration.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*       NU_NOT_PRESENT                      There is no empty slot for
*                                           new configuration.
*
**************************************************************************/
STATUS USBF_DEVCFG_Create_Config(NU_USB_DEVICE *device, UINT8 *config_index_out)
{
    STATUS  status;
    UINT8   config_index, speed_index;
    UINT8   *tmp_buf;
    UINT8   spd_start_idx, spd_end_idx;

    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(config_index_out);

    *config_index_out   = 0xFF;
    status              = NU_NOT_PRESENT;

    /* First find an empty slot for new configuration. */
    for(config_index = 0;
        config_index < USBF_MAX_CONFIGURATIONS; config_index++)
    {
        if ( usbf_check_empty_config(device, config_index) == NU_TRUE )
        {
            status = usb_get_speed_incices(device, &spd_start_idx, &spd_end_idx);
            if ( status == NU_SUCCESS )
            {
                for(speed_index = spd_start_idx;
                    speed_index <=  spd_end_idx; speed_index++ )
                {
                    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                                 USB_CONFIG_DESCRPTR_SIZE,
                                                 (VOID**)&tmp_buf);

                    if ( status == NU_SUCCESS )
                    {
                        tmp_buf[0] = USB_CONFIG_DESCRPTR_SIZE;          /* bLength                  */
                        tmp_buf[1] = USB_DT_CONFIG;                     /* Configuration Descriptor */
                        tmp_buf[2] = USB_CONFIG_DESCRPTR_SIZE;          /* length                   */
                        tmp_buf[3] = 0;                                 /* length                   */
                        tmp_buf[4] = 0;                                 /* bNumInterfaces           */
                        tmp_buf[5] = (config_index + 1);                /* bConfigurationValue      */
                        tmp_buf[6] = 0;                                 /* iConfiguration           */
                        tmp_buf[7] = 0xC0;                              /* bmAttributes             */
                        tmp_buf[8] = 0;                                 /* power                    */

                        /* Save allocated buffer to first free raw_descriptor index. */
                        device->raw_descriptors[config_index][speed_index] = tmp_buf;
                    }
                }

                /* Decrement number of configurations in device descriptor. */
                device->device_descriptor.bNumConfigurations++;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_FALSE )
                if ( spd_end_idx == USB_SPEED_HIGH )
                {
                    /* Increment number of configurations in device qualifier descriptor. */
                    device->device_qualifier.bNumConfigurations++;
                }
#endif
                /* Allocate memory for device configuration. */
                status = USB_Allocate_Object(sizeof(NU_USB_CFG),
                                             (VOID**)&device->config_descriptors[config_index]);
                if ( status == NU_SUCCESS )
                {
                    if ( (config_index + 1) < USBF_MAX_CONFIGURATIONS )
                        device->config_descriptors[config_index + 1] = NU_NULL;

                    /* Return configuration index. */
                    *config_index_out = config_index;
                    break;
                }
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Delete_Config
*
* DESCRIPTION
*
*       This functions deletes a previously created configuration in
*       NU_USB_DEVICE control block.
*
* INPUTS
*
*       device                              Pointer to NU_USB_DEVICE
*                                           control block. It actually
*                                           reporesents a USB device.
*       config_index                        Configuration index to be
*                                           deleted.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Delete_Config(NU_USB_DEVICE  *device,
                                    UINT8       config_index)
{
    STATUS  status;
    UINT8   speed_index;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_FALSE )
    UINT8   speed;
#endif

    NU_USB_PTRCHK(device);

    status = NU_USB_INVLD_ARG;

    /* Check if configuration index is valid or not. */
    if ( config_index < USBF_MAX_CONFIGURATIONS )
    {
        for(speed_index = 0; speed_index < NU_USB_MAX_SPEEDS; speed_index++ )
        {

            if ( device->raw_descriptors[config_index][speed_index] != NU_NULL )
            {
                /* Deallocate raw descriptor. */
                status = USB_Deallocate_Memory(device->raw_descriptors[config_index][speed_index]);
                if ( status == NU_SUCCESS )
                {
                    device->raw_descriptors[config_index][speed_index] = NU_NULL;
                }
            }

            status = USB_Deallocate_Memory(device->config_descriptors[config_index]);
            if ( status == NU_SUCCESS )
            {
                device->config_descriptors[config_index] = NU_NULL;
            }

            /* Decrement number of configurations in device descriptor. */
            device->device_descriptor.bNumConfigurations--;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_FALSE )
            status = NU_USB_HW_Get_Speed((NU_USB_HW*)device->hw, &speed);
            if ( speed == USB_SPEED_HIGH )
            {
                /* Increment number of configurations in device qualifier descriptor. */
                device->device_qualifier.bNumConfigurations--;
            }
#endif
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Recreate_Config
*
* DESCRIPTION
*
*       This functions recreates the specific configuration in
*       NU_USB_DEVICE control block.
*
* INPUTS
*
*       device                              Pointer to NU_USB_DEVICE
*                                           control block. It actually
*                                           reporesents a USB device.
*       config_index                        Configuration index to be
*                                           recreated.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*       NU_NOT_PRESENT                      Configuration does not exist.
*
**************************************************************************/
STATUS USBF_DEVCFG_Recreate_Config(NU_USB_DEVICE    *device,
                                        UINT8       config_index)
{
    STATUS  status;
    UINT8   *tmp_buf;
    UINT8  speed_index;

    NU_USB_PTRCHK(device);

    status = NU_NOT_PRESENT;

    /* Check if configuration index is valid or not. */
    if ( config_index < USBF_MAX_CONFIGURATIONS )
    {
        for(speed_index = 0; speed_index < NU_USB_MAX_SPEEDS; speed_index++ )
        {
            if ( device->raw_descriptors[config_index][speed_index] != NU_NULL )
            {
                status = USB_Deallocate_Memory(device->raw_descriptors[config_index][speed_index]);
                device->raw_descriptors[config_index][speed_index] = NU_NULL;

                if ( status == NU_SUCCESS )
                {
                    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                                 USB_CONFIG_DESCRPTR_SIZE,
                                                 (VOID**)&tmp_buf);
                    if ( status == NU_SUCCESS )
                    {
                        tmp_buf[0] = USB_CONFIG_DESCRPTR_SIZE;          /* bLength                  */
                        tmp_buf[1] = USB_DT_CONFIG;                     /* Configuration Descriptor */
                        tmp_buf[2] = USB_CONFIG_DESCRPTR_SIZE;          /* length                   */
                        tmp_buf[3] = 0;                                 /* length                   */
                        tmp_buf[4] = 0;                                 /* bNumInterfaces           */
                        tmp_buf[5] = (config_index + 1);                /* bConfigurationValue      */
                        tmp_buf[6] = 0;                                 /* iConfiguration           */
                        tmp_buf[7] = 0;                                 /* bmAttributes             */
                        tmp_buf[8] = 0;                                 /* power                    */

                        /* Save allocated buffer to first free raw_descriptor index. */
                        device->raw_descriptors[config_index][speed_index] = tmp_buf;
                    }
                }
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Activate_Device
*
* DESCRIPTION
*
*       Activates the USB device by enabling DP pullup so that it is
*       ready to be connected with USB host and communicate with it.
*
* INPUTS
*
*       device                              Pointer to NU_USB_DEVICE
*                                           control block. It actually
*                                           reporesents a USB device.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Activate_Device(NU_USB_DEVICE *device)
{
    STATUS status;

    NU_USB_PTRCHK(device);

    status = NU_USBF_HW_Enable_Pullup((NU_USBF_HW*)device->hw);
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_DEVCFG_Deactivate_Device
*
* DESCRIPTION
*
*       Deactivates the USB device by disabling DP pullup so that it is
*       no more ready to be connected with USB host.
*
* INPUTS
*
*       device                              Pointer to NU_USB_DEVICE
*                                           control block. It actually
*                                           reporesents a USB device.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Deactivate_Device(NU_USB_DEVICE *device)
{
    STATUS status;

    NU_USB_PTRCHK(device);

    status = NU_USBF_HW_Disable_Pullup((NU_USBF_HW*)device->hw);
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Add_Lang_String
*
* DESCRIPTION
*
*       Adds the language string descriptor to NU_USB_DEVICE. Language
*       stings descriptor always resides on index zero.
*
* INPUTS
*
*       device                              Pointer to NU_USB_DEVICE
*                                           control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Add_Lang_String(NU_USB_DEVICE *device)
{
    STATUS          status;
    NU_USB_STRING   *str_desc;
    USBF_STRING     *str;

    NU_USB_PTRCHK(device);

    str = &usbf_device_cfg.usb_strings[0];

    /* Allocate memory for string descriptors. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                sizeof(NU_USB_STRING),
                                (VOID**)&(str_desc));
    if ( status == NU_SUCCESS )
    {
        str_desc->str_index = 0;
        str_desc->wLangId   = 0;
        str_desc->string[0] = 4;
        str_desc->string[1] = 3;
        str_desc->string[2] = 9;
        str_desc->string[3] = 4;

        str->is_used        = NU_TRUE;
        str->index          = 0;
        str->usb_string     = str_desc;

        status = NU_USB_DEVICE_Set_String(device, 0, str->usb_string);
    }
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Add_Manuf_String
*
* DESCRIPTION
*
*       Adds the manufacturer string descriptor to NU_USB_DEVICE.
*
* INPUTS
*
*       device                              Pointer to NU_USB_DEVICE
*                                           control block.
*       string                              Character pointer pointing
*                                           to a valid string.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Add_Manuf_String(NU_USB_DEVICE *device, CHAR *string)
{
    STATUS  status;
    UINT8   str_len;
    UINT8   index;
    USBF_STRING *str;

    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(string);

    status = usbf_get_free_string(&str);
    if ( status == NU_SUCCESS )
    {
        str_len = strlen(string);
        status  = NU_USB_INVLD_ARG;
        if ( ((str_len * 2) + 2) < NU_USB_MAX_STRING_LEN )
        {
            index                           = 0;
            str->usb_string->str_index       = str->index;
            str->usb_string->wLangId         = 0x0409;
            str->usb_string->string[index++] = ((str_len * 2) + 2);
            str->usb_string->string[index++] = 0x03;
            status = usbf_make_unicode_string((str->usb_string->string + index), string, str_len);
            if ( status == NU_SUCCESS )
            {
                status = NU_USB_DEVICE_Set_Manf_String(device, str->index, str->usb_string);
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Add_Product_String
*
* DESCRIPTION
*
*       Adds the product string descriptor to NU_USB_DEVICE.
*
* INPUTS
*
*       device                              Pointer to NU_USB_DEVICE
*                                           control block.
*       string                              Character pointer pointing
*                                           to a valid string.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Add_Product_String(NU_USB_DEVICE *device, CHAR *string)
{
    STATUS  status;
    UINT8   str_len;
    UINT8   index;
    USBF_STRING *str;

    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(string);

    status = usbf_get_free_string(&str);
    if ( status == NU_SUCCESS )
    {
        str_len = strlen(string);
        status  = NU_USB_INVLD_ARG;
        if ( ((str_len * 2) + 2) < NU_USB_MAX_STRING_LEN )
        {
            index                           = 0;
            str->usb_string->str_index       = str->index;
            str->usb_string->wLangId         = 0x0409;
            str->usb_string->string[index++] = ((str_len * 2) + 2);
            str->usb_string->string[index++] = 0x03;
            status = usbf_make_unicode_string((str->usb_string->string + index), string, str_len);
            if ( status == NU_SUCCESS )
            {
                status = NU_USB_DEVICE_Set_Product_String(device, str->index, str->usb_string);
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Add_Serial_String
*
* DESCRIPTION
*
*       Adds the serial number string descriptor to NU_USB_DEVICE.
*
* INPUTS
*
*       device                              Pointer to NU_USB_DEVICE
*                                           control block.
*       string                              Character pointer pointing
*                                           to a valid string.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Add_Serial_String(NU_USB_DEVICE *device, CHAR *string)
{
    STATUS  status;
    UINT8   str_len;
    UINT8   index;
    USBF_STRING *str;

    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(string);

    status = usbf_get_free_string(&str);
    if ( status == NU_SUCCESS )
    {
        str_len = strlen(string);
        status  = NU_USB_INVLD_ARG;
        if ( ((str_len * 2) + 2) < NU_USB_MAX_STRING_LEN )
        {
            index                           = 0;
            str->usb_string->str_index       = str->index;
            str->usb_string->wLangId         = 0x0409;
            str->usb_string->string[index++] = ((str_len * 2) + 2);
            str->usb_string->string[index++] = 0x03;
            status = usbf_make_unicode_string((str->usb_string->string + index), string, str_len);
            if ( status == NU_SUCCESS )
            {
                status = NU_USB_DEVICE_Set_Serial_Num_String(device, str->index, str->usb_string);
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Add_Config_String
*
* DESCRIPTION
*
*       Adds the configuration string descriptor to a particular
*       configuration descriptor.
*
* INPUTS
*
*       config_index                        Configuration index for which
*                                           the string is to be added.
*       device                              Pointer to NU_USB_DEVICE
*                                           control block.
*       string                              Character pointer pointing
*                                           to a valid string.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Add_Config_String(UINT8 config_index, NU_USB_DEVICE *device, CHAR *string)
{
    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(string);

    return ( NU_SUCCESS );
}


/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Add_Intf_String
*
* DESCRIPTION
*
*       Adds the interface string descriptor to a particular
*       interface descriptor.
*
* INPUTS
*
*       usbf_func                           Pointer to USBF_FUNCTION
*                                           control block.
*       device                              Pointer to NU_USB_DEVICE
*                                           control block.
*       string                              Character pointer pointing
*                                           to a valid string.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Add_Intf_String(USBF_FUNCTION *usbf_func, NU_USB_DEVICE *device, CHAR *string)
{
    NU_USB_PTRCHK(usbf_func);
    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(string);

    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Add_String
*
* DESCRIPTION
*
*       Adds the interface string descriptor to a particular
*       interface descriptor.
*
* INPUTS
*
*       usbf_func                           Pointer to USBF_FUNCTION
*                                           control block.
*       device                              Pointer to NU_USB_DEVICE
*                                           control block.
*       string                              Character pointer pointing
*                                           to a valid string.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Add_String(NU_USB_DEVICE *device, CHAR *string)
{
    STATUS  status;
    UINT8   str_len;
    UINT8   index;
    USBF_STRING *str;

    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(string);

    status = usbf_get_free_string(&str);
    if ( status == NU_SUCCESS )
    {
        str_len = strlen(string);
        status  = NU_USB_INVLD_ARG;
        if ( ((str_len * 2) + 2) < NU_USBF_MAX_UNICODE_STRING_LEN )
        {
            index                           = 0;
            str->usb_string->str_index       = str->index;
            str->usb_string->wLangId         = 0x0409;
            str->usb_string->string[index++] = ((str_len * 2) + 2);
            str->usb_string->string[index++] = 0x03;
            status = usbf_make_unicode_string((str->usb_string->string + index), string, str_len);
            if ( status == NU_SUCCESS )
            {
                status = NU_USB_DEVICE_Set_String(device, str->index, str->usb_string);
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Check_Endpoiint
*
* DESCRIPTION
*
*       This function verifies presense of described endpoint in same
*       alternate setting or a different interface of same configuration.
*
* INPUTS
*
*       speed                               Speed Full / High.
*       config_num                          Configuration number.
*       intf_num                            Interface number.
*       alt_sttg                            Alternate setting.
*       endp_addr                           Endpoint address.
*       endp_dir                            Endpoint direction.
*
* OUTPUTS
*
*       NU_FALSE                            Endpoint is not already used
*                                           in same alternate setting or
*                                           a different interface of same
*                                           configuration or any input
*                                           argument is invalid.
*       NU_TRUE                             Interface is already in use.
*
**************************************************************************/
BOOLEAN USBF_DEVCFG_Check_Endpoiint(UINT8   speed,
                                    UINT8   config_num,
                                    UINT8   intf_num,
                                    UINT8   alt_sttg,
                                    UINT8   endp_addr,
                                    UINT8   endp_dir)
{
    UINT8   speed_idx, intf_idx, alt_sttg_idx, ep_idx;
    USBF_EPM_CFG *tmp_cfg;
    USBF_EPM_EP *tmp_ep;

   if ( (config_num == 0)                          ||
        (config_num > NU_USB_MAX_CONFIGURATIONS)   ||
        (intf_num >= NU_USB_MAX_INTERFACES)        ||
        (alt_sttg >= NU_USB_MAX_ALT_SETTINGS) )
    {
        return ( NU_FALSE );
    }

    if ( speed == USB_SPEED_FULL )  speed_idx = 0;
    else if ( speed == USB_SPEED_HIGH )  speed_idx = 1;
    else return ( NU_FALSE );

    /* Get relevant configuration. */
    tmp_cfg = &(usbf_stack_ep_map.cfg[speed_idx][config_num - 1]);

    /* Same endpoint is only allowed in difference alternate settings of same
     * interface.
     */
    for(intf_idx = 0; intf_idx < NU_USB_MAX_INTERFACES; intf_idx++ )
    {
        for(alt_sttg_idx = 0; alt_sttg_idx < NU_USB_MAX_ALT_SETTINGS; alt_sttg_idx++ )
        {
            for(ep_idx = 0; ep_idx < NU_USB_MAX_ENDPOINTS; ep_idx++ )
            {
                tmp_ep = &(tmp_cfg->intf[intf_idx].alt_sttg[alt_sttg_idx].ep[ep_idx]);
                if ( tmp_ep->is_used == NU_TRUE )
                {
                    if ( (tmp_ep->ep_num    == endp_addr)   &&
                         (tmp_ep->direction == endp_dir) )
                    {
                        if ( (tmp_ep->intf_num == intf_num) &&
                             (tmp_ep->alt_sttg != alt_sttg) )
                        {
                            return ( NU_FALSE );
                        }
                        else
                        {
                            return ( NU_TRUE );
                        }
                    }
                }
            }
        }
    }

    return ( NU_FALSE );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Save_Endpoiint
*
* DESCRIPTION
*
*       This function saves specified endpoint in endpoint map.
*
* INPUTS
*
*       speed                               Speed Full / High.
*       config_num                          Configuration number.
*       intf_num                            Interface number.
*       alt_sttg                            Alternate setting.
*       endp_addr                           Endpoint address.
*       endp_dir                            Endpoint direction.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Operation completed
*                                           successfuly.
*       NU_NOT_PRESENT                      Specified endpoint can't be
*                                           saved.
*
**************************************************************************/
STATUS USBF_DEVCFG_Save_Endpoiint(  UINT8   speed,
                                    UINT8   config_num,
                                    UINT8   intf_num,
                                    UINT8   alt_sttg,
                                    UINT8   endp_addr,
                                    UINT8   endp_dir)
{
    UINT8   speed_idx, ep_idx;
    USBF_EPM_CFG *tmp_cfg;
    USBF_EPM_ALT_STTG *tmp_alt_sttg;
    USBF_EPM_EP *tmp_ep;

    if ( (config_num == 0)                          ||
         (config_num > NU_USB_MAX_CONFIGURATIONS)   ||
         (intf_num >= NU_USB_MAX_INTERFACES)        ||
         (alt_sttg >= NU_USB_MAX_ALT_SETTINGS) )
    {
        return ( NU_USB_INVLD_ARG );
    }

    if ( speed == USB_SPEED_FULL )  speed_idx = 0;
    else if ( speed == USB_SPEED_HIGH )  speed_idx = 1;
    else if ( speed == USB_SPEED_SUPER )  speed_idx = 2;
    else return ( NU_USB_INVLD_ARG );

    /* Get relevant configuration. */
    tmp_cfg = &(usbf_stack_ep_map.cfg[speed_idx][config_num - 1]);

    /* Get relevang alternate setting. */
    tmp_alt_sttg = &(tmp_cfg->intf[intf_num].alt_sttg[alt_sttg]);

    for(ep_idx = 0; ep_idx < NU_USB_MAX_ENDPOINTS; ep_idx++ )
    {
        tmp_ep = &(tmp_alt_sttg->ep[ep_idx]);

        if ( tmp_ep->is_used == NU_FALSE )
        {
            tmp_ep->is_used     = NU_TRUE;
            tmp_ep->config_num  = config_num;
            tmp_ep->intf_num    = intf_num;
            tmp_ep->alt_sttg    = alt_sttg;
            tmp_ep->ep_num      = endp_addr;
            tmp_ep->direction   = endp_dir;

            return ( NU_SUCCESS );
        }
    }

    return ( NU_NOT_PRESENT );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Delete_Endpoiint
*
* DESCRIPTION
*
*       This function deletes specified endpoint in endpoint map.
*
* INPUTS
*
*       speed                               Speed Full / High.
*       config_num                          Configuration number.
*       intf_num                            Interface number.
*       alt_sttg                            Alternate setting.
*       endp_addr                           Endpoint address.
*       endp_dir                            Endpoint direction.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Operation completed
*                                           successfuly.
*       NU_NOT_PRESENT                      Specified endpoint can't be
*                                           deleted.
*
**************************************************************************/
STATUS USBF_DEVCFG_Delete_Endpoiint(UINT8   speed,
                                    UINT8   config_num,
                                    UINT8   intf_num,
                                    UINT8   alt_sttg,
                                    UINT8   endp_addr,
                                    UINT8   endp_dir)
{
    UINT8   speed_idx, ep_idx;
    USBF_EPM_CFG *tmp_cfg;
    USBF_EPM_ALT_STTG *tmp_alt_sttg;
    USBF_EPM_EP *tmp_ep;

    if ( (config_num == 0)                          ||
         (config_num > NU_USB_MAX_CONFIGURATIONS)   ||
         (intf_num >= NU_USB_MAX_INTERFACES)        ||
         (alt_sttg >= NU_USB_MAX_ALT_SETTINGS) )
    {
        return ( NU_USB_INVLD_ARG );
    }

    if ( speed == USB_SPEED_FULL )  speed_idx = 0;
    else if ( speed == USB_SPEED_HIGH )  speed_idx = 1;
    else return ( NU_USB_INVLD_ARG );

    /* Get relevant configuration. */
    tmp_cfg = &(usbf_stack_ep_map.cfg[speed_idx][config_num - 1]);

    /* Get relevang alternate setting. */
    tmp_alt_sttg = &(tmp_cfg->intf[intf_num].alt_sttg[alt_sttg]);

    for(ep_idx = 0; ep_idx < NU_USB_MAX_ENDPOINTS; ep_idx++ )
    {
        tmp_ep = &(tmp_alt_sttg->ep[ep_idx]);

        if ( (tmp_ep->is_used == NU_TRUE)               &&
             (tmp_ep->intf_num == intf_num)             &&
             (tmp_ep->alt_sttg == alt_sttg)             &&
             (tmp_ep->ep_num == (endp_addr & 0x7F))     &&
             (tmp_ep->direction == endp_dir) )
        {
            memset(tmp_ep, 0x00, sizeof(USBF_EPM_EP));
            return ( NU_SUCCESS );
        }
    }

    return ( NU_NOT_PRESENT );
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_get_free_string
*
* DESCRIPTION
*
*       This is an internal function. This function finds an empty slot
*       for creating the string descriptor and return its pointer.
*
* INPUTS
*
*       string                              Pointer to USBF_STRING
*                                           pointing to an empty slot
*                                           when function returns.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*       NU_NOT_PRESENT                      There is no empty slot for
*                                           accomodating a new string.
*
**************************************************************************/
static STATUS usbf_get_free_string(USBF_STRING **string)
{
    UINT8           index;
    NU_USB_STRING   *str_desc;
    STATUS          status;

    NU_USB_PTRCHK(string);

    for(index = 1; index < USBF_MAX_STRINGS; index++ )
    {
        if( usbf_device_cfg.usb_strings[index].is_used == NU_FALSE )
        {
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                        sizeof(NU_USB_STRING),
                                        (VOID**)&(str_desc));
            if ( status == NU_SUCCESS )
            {
                usbf_device_cfg.usb_strings[index].is_used = NU_TRUE;
                usbf_device_cfg.usb_strings[index].index = index;
                usbf_device_cfg.usb_strings[index].usb_string = str_desc;
                *string = &usbf_device_cfg.usb_strings[index];
                return ( NU_SUCCESS );
            }
            else
            {
                return ( status );
            }
        }
    }

    return ( NU_NOT_PRESENT );
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_make_unicode_string
*
* DESCRIPTION
*
*       This is an internal function. This function makes a unicode
*       string from ascii.
*
* INPUTS
*
*       str_dest                            Character pointer to
*                                           destination string.
*       str_src                             Character pointer to
*                                           source string.
*       len                                 Length of source string.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*       NU_SUCCESS                          Indicates a successful
*                                           completion of service.
*
**************************************************************************/
static STATUS usbf_make_unicode_string(CHAR *str_dest, CHAR *str_src, UINT8 len)
{
    UINT8 src_index, dest_index;

    NU_USB_PTRCHK(str_dest);
    NU_USB_PTRCHK(str_src);

    for(src_index = dest_index = 0; src_index < (len); src_index++)
    {
        str_dest[dest_index++] = str_src[src_index];
        str_dest[dest_index++] = 0;
    }

    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_check_empty_config
*
* DESCRIPTION
*
*       This function checks if a particular configuration index is busy
*       or free.
*
* INPUTS
*
*       device                              Pointer to USB device control
*                                           block.
*       config_idx                          Configuration index to be
*                                           checked free or not.
*
* OUTPUTS
*
*       NU_FALSE                            Configuration index is not
*                                           free.
*       NU_TRUE                             Configuration index is free.
*
**************************************************************************/
static BOOLEAN usbf_check_empty_config(NU_USB_DEVICE *device, UINT8 config_idx)
{
    UINT8 speed_idx;

    for(speed_idx = 0; speed_idx < NU_USB_MAX_SPEEDS; speed_idx++ )
    {
        if ( device->raw_descriptors[config_idx][speed_idx] )
        {
            return ( NU_FALSE );
        }
    }

    return ( NU_TRUE );
}

/**************************************************************************
*
* FUNCTION
*
*       usb_get_speed_incices
*
* DESCRIPTION
*
*       This function returns start and end speed indices based on
*       supported speed of USB function hardware.
*
* INPUTS
*
*       device                              Pointer to USB device control
*                                           block.
*       strt_idx                            Start speed index.
*       end_idx                             End speed index.
*
* OUTPUTS
*
*       NU_FALSE                            Configuration index is not
*                                           free.
*       NU_TRUE                             Configuration index is free.
*
**************************************************************************/
static STATUS usb_get_speed_incices(NU_USB_DEVICE *device, UINT8 *strt_idx, UINT8 *end_idx)
{
    STATUS  status;
    UINT8   speed;

    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(strt_idx);
    NU_USB_PTRCHK(end_idx);

    status = NU_USB_HW_Get_Speed((NU_USB_HW*)device->hw, &speed);
    if ( status == NU_SUCCESS )
    {
        switch(speed)
        {
            case USB_SPEED_LOW:
                *strt_idx   = USB_SPEED_LOW;
                *end_idx    = USB_SPEED_LOW;
                status      = NU_SUCCESS;
                break;
            case USB_SPEED_FULL:
                *strt_idx   = USB_SPEED_FULL;
                *end_idx    = USB_SPEED_FULL;
                status      = NU_SUCCESS;
                break;
            case USB_SPEED_HIGH:
                *strt_idx   = USB_SPEED_FULL;
                *end_idx    = USB_SPEED_HIGH;
                status      = NU_SUCCESS;
                break;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
            case USB_SPEED_SUPER:
                *strt_idx   = USB_SPEED_FULL;
                *end_idx    = USB_SPEED_SUPER;
                status      = NU_SUCCESS;
                break;
#endif
            default:
                *strt_idx   = 0;
                *end_idx    = 0;
                status      = NU_USB_INVLD_SPEED;
                break;
        }
    }

    return ( status );
}

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Create_BOS
*
* DESCRIPTION
*
*       This function is called to create skeleton of binary object store descriptor.
*
* INPUTS
*
*       device                  Pointer to NU_USB_DEVICE.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        An input argument is invalid.
*       NU_SUCCESS              Indicates a successful
*                               completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Create_BOS(NU_USB_DEVICE *device)
{
    STATUS  status;
    UINT8   *tmp_buf;

    NU_USB_PTRCHK(device);

    /* Allocate memory for raw USB BOS descriptor. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 USBF_DEVCFG_MAX_BOS_DESC_LEN,
                                 (VOID**)&tmp_buf);
    if ( status == NU_SUCCESS )
    {
        /* Reset contents of newly allocated buffer. */
        memset(tmp_buf, 0x00, USBF_DEVCFG_MAX_BOS_DESC_LEN);

        /* Allocate memfor for BOS descriptor control block. */
        status = USB_Allocate_Object(sizeof(NU_USB_BOS),
                                     (VOID**)&device->bos);
        if ( status == NU_SUCCESS )
        {
            /* Initialize initial value of BOS descriptor. */
            tmp_buf[0] = BOS_DESC_LENGTH;                   /* bLength              */
            tmp_buf[1] = USB_DT_BOS;                        /* bDescriptorType      */
            tmp_buf[2] = BOS_DESC_LENGTH;                   /* wTotalLength             */
            tmp_buf[3] = 0;                                 /* wTotalLength             */
            tmp_buf[4] = 0;                                 /* bNumDeviceCaps       */

            /* Save newly crated buffer to raw bos descriptor. */
            device->raw_bos_descriptors = tmp_buf;
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Add_BOS_DevCap
*
* DESCRIPTION
*
*       This function is called to add a new device capability descriptor to BOS.
*
* INPUTS
*
*      device                    Pointer to NU_USB_DEVICE.
*      buffer                    Pointer to buffer containing device capability descriptor.
*      length                    Length of device capability descriptor.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG         An input argument is invalid.
*       NU_SUCCESS               Indicates a successful
*                                completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Add_BOS_DevCap(NU_USB_DEVICE *device,
                                                 UINT8          *buffer,
                                                 UINT16         length)
{
    UINT8   *tmp_buf;
    UINT16  bos_len;

    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(device->raw_bos_descriptors);
    NU_USB_PTRCHK(device->bos);

    /* Get pointer to raw bos descriptor. */
    tmp_buf = device->raw_bos_descriptors;

    bos_len = ((tmp_buf[3] << 8) | tmp_buf[2]);

    NU_USB_ASSERT((bos_len + length) < USBF_DEVCFG_MAX_BOS_DESC_LEN);

    /* Copy device capability descriptor. */
    memcpy((tmp_buf + bos_len), buffer, length);

    /* Update length of BOS descriptor.  */
    bos_len += length;

    tmp_buf[2]  = (UINT8) bos_len;
    tmp_buf[3]  = (UINT8) (bos_len >> 8);

    /* Increment number of device capabilities. */
    tmp_buf[4]++;

    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Add_BOS_DevCap_USB2Ext
*
* DESCRIPTION
*
*       This function is called to add a USB 2.0 extension device capability descriptor.
*
* INPUTS
*
*       device                            Pointer to NU_USB_DEVICE.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        An input argument is invalid.
*       NU_SUCCESS                  Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Add_BOS_DevCap_USB2Ext(NU_USB_DEVICE *device)
{
    STATUS  status;
    UINT8   tmp_buf[DEVCAP_USB2_EXT_LEN];

    NU_USB_PTRCHK(device);

    tmp_buf[0] = DEVCAP_USB2_EXT_LEN;   /* bLength              */
    tmp_buf[1] = USB_DT_DEVCAP;         /* bDescriptorType      */
    tmp_buf[2] = USB_DCT_USB2EXT;       /* bDevCapabilityType       */
    tmp_buf[3] = 2;                     /* bmAttributes         */
    tmp_buf[4] = 0;
    tmp_buf[5] = 0;
    tmp_buf[6] = 0;

    /* Add USB 2.0 extension device capability to BOS descriptor. */
    status = USBF_DEVCFG_Add_BOS_DevCap(device, tmp_buf, DEVCAP_USB2_EXT_LEN);

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_DEVCFG_Add_BOS_DevCap_SS
*
* DESCRIPTION
*
*       This function is called to add a USB Super Speed device capability descriptor.
*
* INPUTS
*
*       device                            Pointer to NU_USB_DEVICE.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        An input argument is invalid.
*       NU_SUCCESS                  Indicates a successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBF_DEVCFG_Add_BOS_DevCap_SS(NU_USB_DEVICE *device)
{
    STATUS  status;
    BOOLEAN is_ltm_capble;
    UINT16  supported_speeds, u2devexitlat;
    UINT8   tmp_buf[DEVCAP_SS_LEN];
    UINT8   index, u1devexitlat;

    NU_USB_PTRCHK(device);

    tmp_buf[0] = DEVCAP_SS_LEN;     /* bLength              */
    tmp_buf[1] = USB_DT_DEVCAP;     /* bDescriptorType      */
    tmp_buf[2] = USB_DCT_USBSS;     /* bDevCapabilityType       */

    status = NU_USBF_HW_Is_LTM_Capable((NU_USBF_HW*)device->hw,
                                        &is_ltm_capble);
    if ( status == NU_SUCCESS )
    {
        /* Encode LTM capability. */
        tmp_buf[3] = ((is_ltm_capble & 1) << 1);

        /* Get supported speeds. */
        status = NU_USBF_HW_Get_Supported_Speeds((NU_USBF_HW*)device->hw,
                                                &supported_speeds);
        if ( status == NU_SUCCESS )
        {
            /* Encode supported speeds. */
            tmp_buf[4] = (UINT8) supported_speeds;
            tmp_buf[5] = (UINT8) (supported_speeds >> 8);

            /* Encode functionality support. */
            for(index = 0; index < 16; index++ )
            {
                if ( supported_speeds & (1 << index) )
                {
                    tmp_buf[6] = index;
                    break;
                }
            }

            /* Get U1DevExitLat value. */
            status = NU_USBF_HW_Get_U1DevExitLat((NU_USBF_HW*)device->hw,
                                                &u1devexitlat);
            if ( status == NU_SUCCESS )
            {
                tmp_buf[7] = u1devexitlat;

                /* Get U2DevExitLat value. */
                status = NU_USBF_HW_Get_U2DevExitLat((NU_USBF_HW*)device->hw,
                                                    &u2devexitlat);
                if ( status == NU_SUCCESS )
                {
                    tmp_buf[8] = (UINT8) u2devexitlat;
                    tmp_buf[9] = (UINT8) (u2devexitlat >> 8);

                    status = USBF_DEVCFG_Add_BOS_DevCap(device,
                                                        tmp_buf,
                                                        DEVCAP_SS_LEN);
                }
            }
        }
    }

    return ( status );
}

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
/*************************************************************************/
#endif /* USBF_DEVCFG_EXT_C */
/* ======================  End Of File  ================================ */
