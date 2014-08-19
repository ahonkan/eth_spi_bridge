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
*        nu_usb_device_ext.c
*
* COMPONENT
*     USB Base
*
* DESCRIPTION
*    This file contains implementation of NU_USB_DEVICE services.
*
*
* DATA STRUCTURES
*     None
*
* FUNCTIONS
*   NU_USB_DEVICE_Claim                     - Assigns ownership of the
*                                             device to the driver.
*   NU_USB_DEVICE_Release                   - The driver relinquishes the
*                                             ownership of the device.
*   NU_USB_DEVICE_Get_Is_Claimed            - Checks if there is owner for
*                                             the device.
*   NU_USB_DEVICE_Get_Num_Cfgs              - returns the number of
*                                             configurations the device has.
*   NU_USB_DEVICE_Get_Active_Cfg_Num        - returns the configuration
*                                             number of the active configuration.
*   NU_USB_DEVICE_Get_Cfg                   - returns pointer to the
*                                             control of the specified
*                                             configuration.
*   NU_USB_DEVICE_Get_Active_Cfg            - returns pointer to the
*                                             control block of the active
*                                             configuration.
*   NU_USB_DEVICE_Set_Active_Cfg            - Activates/deactivates a
*                                             configuration.
*   NU_USB_DEVICE_Get_Parent                - returns pointer to the device
*                                             control block of the parent hub.
*   NU_USB_DEVICE_Get_Port_Number           - returns the port # on the
*                                             parent hub, to which this device
*                                             is connected.
*   NU_USB_DEVICE_Get_Desc                  - returns the pointer to the
*                                             device descriptor.
*   NU_USB_DEVICE_Get_Function_Addr         - returns the USB function
*                                             address assigned to the device.
*   NU_USB_DEVICE_Get_String                - returns the ascii string
*                                             corresponding to the specified
*                                             string index.
*   NU_USB_DEVICE_Get_String_Desc           - returns pointer to the string
*                                             descriptor that corresponds to
*                                             the specified string index.
*   NU_USB_DEVICE_Get_Manf_String           - returns  a copy of manufacturing
*                                             string in ascii format.
*   NU_USB_DEVICE_Get_Manf_String_Num       - returns the string index of
*                                             the manufacturing string.
*   NU_USB_DEVICE_Get_Manf_String_Desc      - returns a pointer to
*                                             manufacturing string descriptor.
*   NU_USB_DEVICE_Get_Product_String        - returns a copy of the product
*                                             string in ascii format.
*   NU_USB_DEVICE_Get_Product_String_Num    - returns string index of the
*                                             product string.
*   NU_USB_DEVICE_Get_Product_String_Desc   - returns pointer to string
*                                             descriptor of the product string.
*   NU_USB_DEVICE_Get_Serial_Num_String     - returns a copy of serial number
*                                             string in ascii format.
*   NU_USB_DEVICE_Get_Serial_Num_String_Num - returns the string index of
*                                             the serial number string.
*   NU_USB_DEVICE_Get_Serial_String_Desc    - returns a pointer to the
*                                             serial number string descriptor.
*   NU_USB_DEVICE_Get_Speed                 - returns the speed of the
*                                             device.
*   NU_USB_DEVICE_Get_bcdUSB                - returns the bcd version
*                                             supported by the device.
*   NU_USB_DEVICE_Get_bDeviceClass          - returns the class code of the
*                                             device.
*   NU_USB_DEVICE_Get_bDeviceSubClass       - returns the sub class code of
*                                             the device.
*   NU_USB_DEVICE_Get_bDeviceProtocol       - returns the protocol code of
*                                             the device.
*   NU_USB_DEVICE_Get_bMaxPacketSize0       - returns the max packet size
*                                             of the endpoint 0.
*   NU_USB_DEVICE_Get_idVendor              - returns the vendor id.
*   NU_USB_DEVICE_Get_idProduct             - returns product id.
*   NU_USB_DEVICE_Get_bcdDevice             - returns the release number.
*   NU_USB_DEVICE_Get_Status                - returns the status of the
*                                             device.
*   NU_USB_DEVICE_Set_Status                - sets the status of the
*                                             device.
*   NU_USB_DEVICE_Get_Stack                 - returns pointer to the
*                                             associated stack control block.
*   NU_USB_DEVICE_Get_Hw                    - returns pointer to the
*                                             associated h/w control block.
*   NU_USB_DEVICE_Set_Hw                    - sets the  pointer to the
*                                             control block of the h/w thats
*                                             associated with the device.
*   NU_USB_DEVICE_Get_BOS                   - User can call this function to
*                                             get a pointer of NU_USB_BOS
*                                             control block. A difference
*                                             must be observed in this API
*                                             and NU_USB_DEVIC_Get_BOS_Desc
*                                             which returns a pointer to BOS
*                                              descriptor.
*   NU_USB_DEVICE_Get_BOS_Desc              - This API is used for extracting
*                                             BOS descriptor from NU_USB_DEVICE
*                                             control block. This will only get
*                                             the BOS descriptor not the device
*                                             capability descriptor. Separate
*                                             APIs are provided to get device
*                                             capability descriptors.
*   NU_USB_DEVICE_Get_USB2Ext_Desc          - This API is used for extracting
*                                             USB 2 extension device capability
*                                             descriptor from NU_USB_DEVICE
*                                             control block. According to
*                                             USB 3.0 specifications, a Super
*                                             Speed device shall include a
*                                             USB 2.0 extension device
*                                             capability descriptor.
*   NU_USB_DEVICE_Get_SuprSpd_Desc          - This API is used for extracting
*                                             Super Speed device capability
*                                             descriptor from NU_USB_DEVICE
*                                             control block. According to
*                                             USB 3.0 specifications, all
*                                             Super Speed devices shall
*                                             include implement a Super
*                                             Speed device capability
*                                             descriptor.
*   NU_USB_DEVICE_Get_CntnrID_Desc          - This API is used for extracting
*                                             Container ID device capability
*                                             descriptor from NU_USB_DEVICE
*                                             control block. According to
*                                             USB 3.0 specifications, this
*                                             descriptor is only mandatory for
*                                             Hubs.
*   NU_USB_DEVICE_Get_MaxPacketSize0        - This API calculates MaxP 
*                                             according to the speed of 
*                                             USB device and returns it as 
*                                             UINT16 type variable.
*   NU_USB_DEVICE_Set_Link_State            - This API updates 'link_state'
*                                             variable on NU_USB_DEVICE
*                                             control block to newly
*                                             reported link state.
*       
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_DEVICE_EXT_C
#define USB_DEVICE_EXT_C

#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*               usb_string_desc_2_ascii
*
* DESCRIPTION
*   Converts a UNICODE string in English to ascii.
*
* INPUTS
*   string_desc  Pointer to string descriptor.
*   string       array in to which the ascii string is copied.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS usb_string_desc_2_ascii (NU_USB_STRING * string_desc,
                                CHAR * string)
{
    UNSIGNED i, j;
    CHAR *str;
    UINT8 bLength;

    NU_USB_PTRCHK(string_desc);
    NU_USB_PTRCHK(string);

    str = (CHAR *)&string_desc->string[2];
    bLength = string_desc->string[0];

    if(string_desc->wLangId != USB_LANGID_ENGLISH)
        return NU_USB_INVLD_ARG;

    if(bLength > NU_USB_MAX_STRING_LEN)
        return NU_USB_INVLD_ARG;

    for (j = 0, i = 0; i < bLength - 2; i += 2)
    {
        if (j >= NU_USB_MAX_STRING_LEN - 1)
            break;

        if (i >= NU_USB_MAX_STRING_LEN - 2)
            break;

        if (str[i + 1])         /* English: Higher order byte has to be 0 */
            string[j++] = '?';  /* Unknown CHAR */
        else
            string[j++] = str[i];
    }

    string[j] = 0;              /* Null termination */
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Desc
*
* DESCRIPTION
*    This function returns a pointer to device descriptor, in device_desc_out.
*
* INPUTS
*    cb               Pointer to the device control block.
*    device_desc_out  Pointer to memory location to hold pointer to device
*                     descriptor.
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Desc (NU_USB_DEVICE * cb,
                               NU_USB_DEVICE_DESC ** device_desc_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device_desc_out);

    *device_desc_out = &cb->device_descriptor;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Stack
*
* DESCRIPTION
*    This function returns the pointer to the control block of the associated
* stack, in stack_out.
*
* INPUTS
*   cb         Pointer to the device control block.
*   stack_out  Pointer to a memory location to hold the pointer to the control
*              block of the associated stack.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Stack (NU_USB_DEVICE * cb,
                                NU_USB_STACK ** stack_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(stack_out);

    *stack_out = cb->stack;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Set_Stack
*
* DESCRIPTION
*    This function sets the pointer to the control block of the associated
* stack.
*
* INPUTS
*   cb         Pointer to the device control block.
*   stack      Pointer to the associated stack control block.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_Stack (NU_USB_DEVICE * cb,
                                NU_USB_STACK * stack)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(stack);

    cb->stack = stack;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Claim
*
* DESCRIPTION
*
*       This function sets the driver that owns the device. A USB device class
* driver may call this function from its NU_USB_DRVR_Initialize_Device after
* successfully locating all the necessary pipes and notifying relevant users.
* USB Interface class drivers are not expected to make this function call.
*
* INPUTS
*   cb      Pointer to the device control block. Note: This pointer is passed to
*           the USB driver when the stack invokes USB driver’s
*           NU_USB_DRVR_Initialize_Device/NU_USB_DRVR_Initialize_Intf function.
*   drvr        Pointer to the control block of the driver that has decide to own
*           the device.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Claim (NU_USB_DEVICE * cb,
                            NU_USB_DRVR * drvr)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(drvr);

    cb->driver = drvr;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Release
*
* DESCRIPTION
*     This function releases the driver's ownership of the device. A USB device
* driver that currently owns the device is only expected to make this function
* call. Once the ownership is released, any other driver may claim
* ownership of the device. The delete function of a class driver typically
* calls this function for all the owned devices.
*
*
* INPUTS
*   cb  Pointer to the device control block.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Release (NU_USB_DEVICE * cb)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    cb->driver = NU_NULL;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Is_Claimed
*
* DESCRIPTION
*    This function returns NU_FALSE in is_claimed_out, if no driver currently
* owns the device and NU_TRUE otherwise. If a driver currently owns the
* device its pointer is returned in drvr_out.
*
* INPUTS
*   cb              Pointer to the device control block.
*   is_claimed_out      Pointer to a variable to hold the ownership status of the
*                   device
*   drvr_out        Pointer to a memory location to hold the pointer to the
*                   driver control block that currently owns the device.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Is_Claimed (NU_USB_DEVICE * cb,
                                     BOOLEAN * is_claimed_out,
                                     NU_USB_DRVR ** drvr_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(is_claimed_out);
    NU_USB_PTRCHK_RETURN(drvr_out);

    if (cb->driver)
    {
        *is_claimed_out = NU_TRUE;
        *drvr_out = cb->driver;
    }
    else
    {
        *is_claimed_out = NU_FALSE;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Hw
*
* DESCRIPTION
*    This function returns the pointer to the control block of the associated
* H/W, in hw_out.
*
* INPUTS
*   cb      Pointer to the device control block.
*   hw_out      Pointer to a memory location to hold the pointer to the control
*           block of the associated H/W.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Hw (NU_USB_DEVICE * cb,
                             NU_USB_HW ** hw_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(hw_out);

    *hw_out = cb->hw;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Set_Hw
*
* DESCRIPTION
*    This function sets pointer to the control block of the h/w associated
* with this device.
*
* INPUTS
*   cb      Pointer to the device control block.
*   hw      pointer to the control block of the associated h/w.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_Hw (NU_USB_DEVICE * cb,
                             NU_USB_HW * hw)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(hw);

    cb->hw = hw;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Num_Cfgs
*
* DESCRIPTION
*     This function returns the number configurations that the device supports
* in number_cfgs_out.
*
* INPUTS
*  cb               Pointer to the device control block.
*  number_cfgs_out      Pointer to variable to hold number of configurations.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Num_Cfgs (NU_USB_DEVICE * cb,
                                   UINT8 *number_cfgs_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(number_cfgs_out);

    *number_cfgs_out = cb->device_descriptor.bNumConfigurations;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Cfg
*
* DESCRIPTION
*     This function returns pointer to the configuration control block (in
* cfg_out) for the specified bConfigurationValue. If 0 is specified for
* cfg_num, then pointer to the control block of the currently active
* configuration is returned in cfg_out.
*
* INPUTS
*   cb      Pointer to the device control block.
*   cfg_out     Pointer to a memory location to hold pointer to the control block
*           of the requested configuration.
*
* OUTPUTS
*    NU_SUCCESS     Indicates successful completion of the service.
*    NU_NOT_PRESENT     Indicates that the requested bConfigurationValue is not
*                   present in the device or if 0 is specified for cfg_num, it
*                   means that the device is not in the configured state.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Cfg (NU_USB_DEVICE * cb,
                              UINT8 cfg_num,
                              NU_USB_CFG ** cfg_out)
{
    STATUS status = NU_SUCCESS;
    UINT8 cfg_number;
    UINT8 i;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cfg_out);

    *cfg_out = NU_NULL;
    if (cfg_num == 0)
    {
        /* return the currently active configuration ptr */
        if (cb->active_cnfg_num == NU_USB_MAX_CONFIGURATIONS)
        {
            /* Switch back to user mode. */
            NU_USER_MODE();
            return NU_NOT_PRESENT;  /* No active config */
        }
        else
        {
            *cfg_out = cb->config_descriptors[cb->active_cnfg_num];

            /* Switch back to user mode. */
            NU_USER_MODE();
            return (NU_SUCCESS);
        }
    }

    /* Locate the requested configuration */
    for (i = 0; i < NU_USB_MAX_CONFIGURATIONS; i++)
    {
        if (cb->config_descriptors[i])
        {
            status = NU_USB_CFG_Get_Cfg_Value (cb->config_descriptors[i], &cfg_number);

            /* In case of non-success status we want to iterate for the next
             * configuration. So no need to check status.
             */

            if (cfg_num == cfg_number)
            {
                *cfg_out = cb->config_descriptors[i];

                /* Switch back to user mode. */
                NU_USER_MODE();
                return (status);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Active_Cfg_Num
*
* DESCRIPTION
* This function returns the bConfigurationValue of the configuration (in
* active_cfg_num_out) that’s currently active on the device. If the device
* is not  configured state, 0 is returned in  active_cfg_num_out.
*
* INPUTS
*   cb                  Pointer to the device control block.
*   active_cfg_num_out  Pointer to a variable to hold the active configuration
*                       number.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Active_Cfg_Num (NU_USB_DEVICE * cb,
                                         UINT8 *active_cfg_num_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(active_cfg_num_out);

    status = NU_SUCCESS;
    if(cb->active_cnfg_num == NU_USB_MAX_CONFIGURATIONS)
    {
        /* Device not yet configured */
        *active_cfg_num_out = NU_USB_MAX_CONFIGURATIONS;
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        *active_cfg_num_out =
            cb->config_descriptors[cb->active_cnfg_num]->desc->bConfigurationValue;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Active_Cfg
*
* DESCRIPTION
* This function returns the pointer to control block of the currently
* active configuration on the device in cfg_out. If the device is not in
* the configured state, NU_NULL is returned in cfg_out.
*
* INPUTS
*   cb      Pointer to the device control block.
*   cfg_out     Pointer to a memory location to hold the pointer to configuration
*           control block.
*
* OUTPUTS
*    NU_SUCCESS     Indicates successful completion of the service.
*    NU_NOT_PRESENT     Indicates that an active configuration is not present on
*                   the device.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Active_Cfg (NU_USB_DEVICE * cb,
                                     NU_USB_CFG ** cfg_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cfg_out);

    status = NU_USB_DEVICE_Get_Cfg (cb, 0, cfg_out);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Function_Addr
*
* DESCRIPTION
*    This function returns the function address(in function_address_out);
* assigned by the host to the device during enumeration. Root hub’s
* function address is defined by the macro USB_ROOT_HUB. This function
* should not be used on the function side and could be used only on the
* host side.
*
*
* INPUTS
*   cb                   Pointer to the device control block.
*   function_address_out Pointer to the variable to hold the function address
*                        of the device. Function addresses are in the range of
*                        0 to 127.  Function address 0 indicates that the device
*                        is still in un-addressed state.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Function_Addr (NU_USB_DEVICE * cb,
                                        UINT8 *function_address)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(function_address);

    *function_address = cb->function_address;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Parent
*
* DESCRIPTION
*    This function returns pointer to the device control block of the hub to
* which it’s physically connected in parent_out. There is no parent hub to
* a root hub and so if this function is called for root hub it returns
* NU_USB_INVLD_ARG.
*
* INPUTS
*   cb          Pointer to the device control block.
*   parent_out  Pointer to a memory location to hold the pointer to the device
*               control block of the hub to which the device is physically
*               connected.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*    NU_USB_INVLD_ARG   Indicates that either ‘cb’ points to an invalid device
*                       control block or that it points to root hub control
*                       block.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Parent (NU_USB_DEVICE * cb,
                                 NU_USB_DEVICE ** parent_out)
{
    STATUS status = NU_USB_INVLD_ARG;
    STATUS internal_sts = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(parent_out);

    /* Prevent modifications the stack */
    status = NU_USB_DEVICE_Lock (cb);
    if (status != NU_SUCCESS)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (status);
    }

    /* Check if the device is valid */
    if (!NU_USB_STACK_Is_Valid_Device (cb->stack, cb))
    {
        internal_sts |= NU_USB_DEVICE_Unlock (cb);
        NU_UNUSED_PARAM(internal_sts);

        /* Switch back to user mode. */
        NU_USER_MODE();
        return (status);
    }

    /* A root hub cannot have a parent ! */
    if (cb->function_address == USB_ROOT_HUB)
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        *parent_out = cb->parent;
        status = NU_SUCCESS;
    }

    /* Done. Allow any modifications to the stack */
    status |= NU_USB_DEVICE_Unlock(cb);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Port_Number
*
* DESCRIPTION
*     This function returns port number on the hub to which it’s physically
* connected in port_num_out. There is no parent hub to a root hub and so
* if this function is called for root hub it returns NU_USB_INVLD_ARG.
*
* INPUTS
*   cb            Pointer to the device control block.
*   port_num_out  Pointer to the variable to hold the port number on the hub to
*                 which the device is physically connected.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*    NU_USB_INVLD_ARG   Indicates that either ‘cb’ points to an invalid device
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Port_Number (NU_USB_DEVICE * cb,
                                      UINT8 *port_num_out)
{
    STATUS status       = NU_USB_INVLD_ARG;
    STATUS internal_sts = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(port_num_out);

    /* Prevent modifications to the stack */
    status = NU_USB_DEVICE_Lock (cb);
    if (status != NU_SUCCESS)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (status);
    }

    /* Check if the device is valid */
    if (!NU_USB_STACK_Is_Valid_Device (cb->stack, cb))
    {
        internal_sts |= NU_USB_DEVICE_Unlock (cb);
        NU_UNUSED_PARAM(internal_sts);

        /* Switch back to user mode. */
        NU_USER_MODE();
        return (status);
    }

    /* A root hub cannot have a parent ! */
    if (cb->function_address == USB_ROOT_HUB)
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        *port_num_out = cb->port_number;
        status = NU_SUCCESS;
    }

    /* Done. Allow modifications to the stack. */
    status |= NU_USB_DEVICE_Unlock (cb);


    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_String
*
* DESCRIPTION
*     This function copies the string(in to string_out) corresponding to
* specified string number. The string descriptors are assumed to be in
* English. The total number of the string descriptors in the device cannot
* be more than NU_USB_MAX_STRINGS.
*
* INPUTS
*  cb           Pointer to the device control block.
*  string_num   number of the string as assigned in device or configuration
*               or interface descriptor.
*  string_out   pointer to the user supplied array of characters of size
*               NU_USB_MAX_STRING_LEN.
*
* OUTPUTS
*   NU_SUCCESS      Indicates successful completion of the service.
*   NU_NOT_PRESENT      Indicates that the specified string number cannot be found
*                   in the device’s set of string descriptors.
*
*************************************************************************/
/* Services for the device's string(s) */
STATUS NU_USB_DEVICE_Get_String (NU_USB_DEVICE * cb,
                                 UINT8 string_num,
                                 CHAR * string_out)
{
    UINT8 i;
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string_out);

    status = NU_NOT_PRESENT;
    for (i = 0; i < NU_USB_MAX_STRINGS; i++)
        if ((cb->string_descriptors[i]) &&
            (cb->string_descriptors[i]->str_index == string_num))
        {
            status = usb_string_desc_2_ascii (cb->string_descriptors[i],
                                            string_out);
            break;
        }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_String_Desc
*
* DESCRIPTION
*    This function gets pointer to the string descriptor (in to
* string_desc_out) for the specified string number. If the Unicode used for
* string descriptors is English, NU_USB_DEVICE_Get_String can be used to
* directly get the ASCII string. For other Unicodes this function would help
* retrieve the descriptor and then reader may write routines to convert from
* their Unicode format to ASCII string or such other display string notations.
*
* INPUTS
*   cb              Pointer to the device control block.
*   string_num      number of the string as assigned in device or configuration
*                   or interface descriptor.
*   string_desc_out     pointer to memory location to hold pointer to string
*                   descriptor structure.
*
* OUTPUTS
*   NU_SUCCESS      Indicates successful completion of the service.
*   NU_NOT_PRESENT      Indicates that the specified string number cannot be found
*                   in the device’s set of string descriptors.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_String_Desc (NU_USB_DEVICE * cb,
                                      UINT8 string_num,
                                      UINT16 wLangId,
                                      NU_USB_STRING_DESC * string_desc_out)
{
    UINT8 i;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string_desc_out);

    /* locate the specified string index */
    for (i = 0; i < NU_USB_MAX_STRINGS; i++)
    {
        if ((cb->string_descriptors[i]) &&
            (cb->string_descriptors[i]->str_index == string_num) &&
            (cb->string_descriptors[i]->wLangId == wLangId))
        {
            string_desc_out->bLength = cb->string_descriptors[i]->string[0];
            string_desc_out->bDescriptorType =
                cb->string_descriptors[i]->string[1];
            string_desc_out->bString = &cb->string_descriptors[i]->string[2];


            /* Switch back to user mode. */
            NU_USER_MODE();


            return (NU_SUCCESS);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return NU_NOT_PRESENT;
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Manf_String
*
* DESCRIPTION
*     This function copies the manufacturer string of the device in to
* string_out. English is assumed to be the Unicode of the manufacturer
* string descriptor. If the Unicode is not English, refer to
* NU_USB_DEVICE_Get_Manf_String_Desc to know how to retrieve a displayable
* string.
*
* INPUTS
*   cb          Pointer to the device control block.
*   string_out  pointer to the user supplied array of characters of size
*               NU_USB_MAX_STRING_LEN.
*
* OUTPUTS
*   NU_SUCCESS      Indicates successful completion of the service.
*   NU_NOT_PRESENT      Indicates that the manufacturer string number cannot be
*                   found in the device’s set of string descriptors.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Manf_String (NU_USB_DEVICE * cb,
                                      CHAR * string_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string_out);

    status = NU_NOT_PRESENT;
    if (cb->device_descriptor.iManufacturer)
    {
        status = NU_USB_DEVICE_Get_String (cb,
                        cb->device_descriptor.iManufacturer,
                        string_out);
    }
    else
    {
        /*  else No such string */
        string_out[0] = 0;          /* Null string */
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Manf_String_Num
*
* DESCRIPTION
*     This function returns iManufacturer field of the device descriptor in
* string_num_out. If string_num_out is 0, it means that there is no
* manufacturer string descriptor associated with this device.
*
*
* INPUTS
*   cb              Pointer to the device control block.
*   string_num_out      pointer to the variable to hold iManufacturer field of the
*                   device descriptor.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Manf_String_Num (NU_USB_DEVICE * cb,
                                          UINT8 *string_num_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string_num_out);

    *string_num_out = cb->device_descriptor.iManufacturer;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Manf_String_Desc
*
* DESCRIPTION
*    This function gets pointer to the string descriptor (in to
* string_desc_out) for the manufacturer string number (iManufacturer field
* of device descriptor). If the Unicode used for string descriptors is
* English, NU_USB_DEVICE_Get_Manf_String can be used to directly get the
* ASCII string. For other Unicodes this function would help retrieve the
* descriptor and then reader may write routines to convert from their
* Unicode format to ASCII string or such other display string notations.
*
* INPUTS
*   cb              Pointer to the device control block.
*   string_desc_out     pointer to memory location to hold pointer to string
*                   descriptor structure.
*
* OUTPUTS
*   NU_SUCCESS      Indicates successful completion of the service.
*   NU_NOT_PRESENT      Indicates that the manufacturer string number cannot be
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Manf_String_Desc (NU_USB_DEVICE * cb,
                                           UINT16 wLangId,
                                           NU_USB_STRING_DESC *string_desc_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string_desc_out);

    status = NU_NOT_PRESENT;
    if (cb->device_descriptor.iManufacturer)
    {
        status =  NU_USB_DEVICE_Get_String_Desc (cb,
                                    cb->device_descriptor.
                                    iManufacturer, wLangId,
                                    string_desc_out);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /*  else No such string */
    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Product_String
*
* DESCRIPTION
*     This function copies the product string of the device in to string_out.
* English is assumed to be the Unicode of the manufacturer string
* descriptor. If the Unicode is not English, refer to
* NU_USB_DEVICE_Get_Product_String_Desc to know how to retrieve a
* displayable string.
*
* INPUTS
*   cb          Pointer to the device control block.
*   string_out  pointer to the user supplied array of characters of size
*               NU_USB_MAX_STRING_LEN.
*
* OUTPUTS
*   NU_SUCCESS      Indicates successful completion of the service.
*   NU_NOT_PRESENT      Indicates that the product string number cannot be found in
*                   the device’s set of string descriptors.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Product_String (NU_USB_DEVICE * cb,
                                         CHAR * string_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string_out);

    status = NU_NOT_PRESENT;
    if (cb->device_descriptor.iProduct)
    {
        status = NU_USB_DEVICE_Get_String (
                        cb, cb->device_descriptor.iProduct,
                        string_out);
    }
    else
    {
        /*  else No such string */
        string_out[0] = 0;          /* Null string */
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Product_String_Num
*
* DESCRIPTION
*    This function returns iProduct field of the device descriptor in
* string_num_out. If string_num_out is 0, it means that there is no
* product string descriptor associated with this device.
*
* INPUTS
*   cb              Pointer to the device control block.
*   string_num_out      pointer to the variable to hold iProduct field of the
*                   device descriptor.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Product_String_Num (NU_USB_DEVICE * cb,
                                             UINT8 *string_num_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string_num_out);

    *string_num_out = cb->device_descriptor.iProduct;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Product_String_Desc
*
* DESCRIPTION
*     This function gets pointer to the string descriptor (in to
* string_desc_out) for the product string number (iProduct field of device
* descriptor). If the Unicode used for string descriptors is English,
* NU_USB_DEVICE_Get_Product_String can be used to directly get the ASCII
* string. For other Unicodes this function would help retrieve the
* descriptor and then reader may write routines to convert from their
* Unicode format to ASCII string or such other display string notations.
*
* INPUTS
*   cb              Pointer to the device control block.
*   string_desc_out     pointer to memory location to hold pointer to string
*                   descriptor structure.
*
* OUTPUTS
*   NU_SUCCESS      Indicates successful completion of the service.
*   NU_NOT_PRESENT      Indicates that the product string number cannot be found in
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Product_String_Desc (NU_USB_DEVICE * cb,
                                              UINT16 wLangId,
                                              NU_USB_STRING_DESC *string_desc_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string_desc_out);

    status = NU_NOT_PRESENT;
    if (cb->device_descriptor.iProduct)
    {
        status = NU_USB_DEVICE_Get_String_Desc (cb,
                            cb->device_descriptor.iProduct,
                            wLangId,
                            string_desc_out);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /*  else No such string */
    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Serial_Num_String
*
* DESCRIPTION
*     This function copies the serial number string of the device in to
* string_out. English is assumed to be the Unicode of the serial number
* string descriptor. If the Unicode is not English, refer to
* NU_USB_DEVICE_Get_Serial_Num_String_Desc to know how to retrieve a
* displayable string.
*
* INPUTS
*   cb          Pointer to the device control block.
*   string_out  pointer to the user supplied array of characters of size
*               NU_USB_MAX_STRING_LEN.
*
* OUTPUTS
*   NU_SUCCESS      Indicates successful completion of the service.
*   NU_NOT_PRESENT      Indicates that the serial number string number cannot be
*                   found in the device’s set of string descriptors.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Serial_Num_String (NU_USB_DEVICE * cb,
                                            CHAR * string_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string_out);

    status = NU_SUCCESS;
    if (cb->device_descriptor.iSerialNumber)
    {
        status = NU_USB_DEVICE_Get_String (cb,
                                         cb->device_descriptor.iSerialNumber,
                                         string_out);
    }
    else
    {
        /*  else No such string */
        string_out[0] = 0;          /* Null string */
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Serial_Num_String_Num
*
* DESCRIPTION
* This function returns iSerialNumber field of the device descriptor in
* string_num_out. If string_num_out is 0, it means that there is no serial
* number string descriptor associated with this device.
*
* INPUTS
*   cb              Pointer to the device control block.
*   string_num_out      pointer to the variable to hold iSerialNumber field of the device descriptor.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Serial_Num_String_Num (NU_USB_DEVICE * cb,
                                                UINT8 *string_num_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string_num_out);

    *string_num_out = cb->device_descriptor.iSerialNumber;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Serial_Num_String_Desc
*
* DESCRIPTION
* This function gets pointer to the string descriptor (in to
* string_desc_out) for the serial number string number (iSerialNumber
* field of device descriptor). If the Unicode used for string descriptors
* is English, NU_USB_DEVICE_Get_Serial_Num_String can be used to directly
* get the ASCII string. For other Unicodes this function would help
* retrieve the descriptor and then reader may write routines to convert
* from their Unicode format to ASCII string or such other display string
* notations.
*
* INPUTS
*    cb                 Pointer to the device control block.
*    string_desc_out    pointer to memory location to hold pointer to string
*                       descriptor structure.
*
* OUTPUTS
*   NU_SUCCESS      Indicates successful completion of the service.
*   NU_NOT_PRESENT      Indicates that the serial number string number cannot be
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Serial_Num_String_Desc (NU_USB_DEVICE * cb,
                                                 UINT16 wLangId,
                                                 NU_USB_STRING_DESC *string_desc_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string_desc_out);

    status = NU_NOT_PRESENT;
    if (cb->device_descriptor.iSerialNumber)
    {
        status = NU_USB_DEVICE_Get_String_Desc (cb,
                                cb->device_descriptor.
                                iSerialNumber,
                                wLangId,
                                string_desc_out);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /*  else No such string */
    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Speed
*
* DESCRIPTION
*    This function returns the speed of the device in speed_out.
*
* INPUTS
*   cb          Pointer to the device control block.
*   speed_out   Pointer to the variable to hold the speed of the device. The
*               possible return values are USB_SPEED_UNKNOWN, USB_SPEED_LOW,
*               USB_SPEED_FULL, USB_SPEED_HIGH.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
/* Services for the device's speed */
STATUS NU_USB_DEVICE_Get_Speed (NU_USB_DEVICE * cb,
                                UINT8 *speed_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(speed_out);

    *speed_out = cb->speed;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_bcdUSB
*
* DESCRIPTION
*    This function returns the USB version supported by the device in BCD
* format (field bcdUSB of device descriptor).
*
* INPUTS
*   cb          Pointer to the device control block.
*   bcdUSB_out  Pointer to the variable to hold the BCD version of the USB
*               supported by the device.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_bcdUSB (NU_USB_DEVICE * cb,
                                 UINT16 *bcdUSB_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(bcdUSB_out);

    *bcdUSB_out = cb->device_descriptor.bcdUSB;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_bDeviceClass
*
* DESCRIPTION
*     This function returns the class code of the device(bDeviceClass field of
* the device descriptor), in bDeviceClass_out.
*
* INPUTS
*   cb                 Pointer to the device control block.
*   bDeviceClass_out   Pointer to the variable to hold the class code of the
*                      device.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_bDeviceClass (NU_USB_DEVICE * cb,
                                       UINT8 *bDeviceClass_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(bDeviceClass_out);

    *bDeviceClass_out = cb->device_descriptor.bDeviceClass;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_bDeviceSubClass
*
* DESCRIPTION
*    This function returns the sub class code of the device (bDeviceSubClass
* field of the device descriptor), in bDeviceSubClass_out.
*
* INPUTS
*  cb                   Pointer to the device control block.
*  bDeviceSubClass_out  Pointer to the variable to hold the sub class code of
*                       the device.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_bDeviceSubClass (NU_USB_DEVICE * cb,
                                          UINT8 *bDeviceSubClass_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(bDeviceSubClass_out);

    *bDeviceSubClass_out = cb->device_descriptor.bDeviceSubClass;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_bDeviceProtocol
*
* DESCRIPTION
*    This function returns the class code of the device (bDeviceProtocol field
* of the device descriptor), in bDeviceProtocol_out.
*
* INPUTS
*   cb                  Pointer to the device control block.
*   bDeviceProtocol_out Pointer to the variable to hold the protocol code of
*                       the device.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_bDeviceProtocol (NU_USB_DEVICE * cb,
                                          UINT8 *bDeviceProtocol_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(bDeviceProtocol_out);

    *bDeviceProtocol_out = cb->device_descriptor.bDeviceProtocol;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_bMaxPacketSize0
*
* DESCRIPTION
*    This function returns the maximum packet size of the endpoint 0
* (bMaxPacketSize0 field of the device descriptor), in bMaxPacketSize0_out.
*
* INPUTS
*  cb                   Pointer to the device control block.
*  bMaxPacketSize0_out  pointer to the variable to hold the maximum packet size
*                       of endpoint 0.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_bMaxPacketSize0 (NU_USB_DEVICE *cb,
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                          UINT16        *bMaxPacketSize0_out)
#else
                                          UINT8         *bMaxPacketSize0_out)
#endif
{
    STATUS status = NU_USB_INVLD_ARG;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    UINT8  speed_out,temp;
#endif

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(bMaxPacketSize0_out);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    *bMaxPacketSize0_out = 1;

    status = NU_USB_DEVICE_Get_Speed (cb,&speed_out);  
    if (status == NU_SUCCESS)
    {
        if (speed_out == USB_SPEED_SUPER)
        {
            temp = cb->device_descriptor.bMaxPacketSize0;
            
            /* In Super Speed mode bMaxPacketSize0 value can
               only be 09H. */
            if (temp != USB_SS_MAXP_ENDP0_DEF_VALUE)
            {
                status = NU_USB_INVLD_DESC;
            }
            else
            {
                /* In case of super speed maxp value is determined as
                   2exp(bMaxPacketSize0).  */
                *bMaxPacketSize0_out <<=  USB_SS_MAXP_ENDP0_DEF_VALUE;
                status = NU_SUCCESS;
            }
        }
        else
        {
            *bMaxPacketSize0_out = cb->device_descriptor.bMaxPacketSize0;
            status = NU_SUCCESS;
        }
    }
#else
    *bMaxPacketSize0_out = cb->device_descriptor.bMaxPacketSize0;
    status = NU_SUCCESS;
#endif

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_idVendor
*
* DESCRIPTION
*    This function returns the vendor id (idVendor field of the device
* descriptor), in idVendor_out.
*
* INPUTS
*   cb             Pointer to the device control block.
*   idVendor_out   pointer to the variable to hold the vendor id.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_idVendor (NU_USB_DEVICE * cb,
                                   UINT16        * idVendor_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(idVendor_out);

    *idVendor_out = cb->device_descriptor.idVendor;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_idProduct
*
* DESCRIPTION
*    This function returns the product id (idProduct field of the device
* descriptor), in idProduct_out.
*
* INPUTS
*   cb              Pointer to the device control block.
*   idProduct_out       pointer to the variable to hold the product id.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_idProduct (NU_USB_DEVICE * cb,
                                    UINT16        * idProduct_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(idProduct_out);

    *idProduct_out = cb->device_descriptor.idProduct;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_bcdDevice
*
* DESCRIPTION
*    This function returns the release number in BCD form (bcdDevice field of
* the device descriptor), in bcdDevice_out.
*
* INPUTS
*   cb              Pointer to the device control block.
*   bcdDevice_out   pointer to the variable to hold the device release number.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_bcdDevice (NU_USB_DEVICE * cb,
                                    UINT16 *bcdDevice_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(bcdDevice_out);

    *bcdDevice_out = cb->device_descriptor.bcdDevice;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Set_Active_Cfg
*
* DESCRIPTION
*    This function sets the specified configuration as active on the device.
* The USB bandwidth and necessary resources in the controller hardware are
* allocated to make the configuration active. USB interface drivers aren’t
* expected to make this call as they just own an interface on the device
* and so only USB device drivers are entitled to make this call.  This
* function should be used only by class drivers on Host side. It should
* not be used by class drivers on the function side. Its host’s
* prerogative to select and activate a configuration on the device.
*
* INPUTS
*   cb  Pointer to the device control block.
*   cfg Pointer to the control block of the configuration that is to made
*       active.
*
* OUTPUTS
*   NU_SUCCESS              Indicates successful completion of the service.
*   NU_USB_NO_BANDWIDTH     Indicates that the device's b/w requirements of the
*                           specified configuration cannot be met.
*   NU_INVALID_SEMAPHORE    Indicates that the one of internal semaphore pointer
*                           is corrupted.
*   NU_SEMAPHORE_DELETED    Indicates that one of the internal semaphore was
*                           deleted.
*   NU_UNAVAILABLE          Indicates that one of the internal semaphore is
*                           unavailable.
*   NU_INVALID_SUSPEND      Indicates that this API is called from a non-task
*                           thread.
*   NU_USB_INVLD_ARG        Indicates that device control block or
*                           configuration control block passed to this function
*                           are invalid.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_Active_Cfg (NU_USB_DEVICE * cb,
                                     NU_USB_CFG * cfg)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cfg);

    /* Remove Lint warning. */
    NU_UNUSED_PARAM(cb);
    status = NU_USB_CFG_Set_Is_Active (cfg, 1);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_Status
*
* DESCRIPTION
*     This function returns the status of the device in status_out. status_out
* is a 16 bit field and the meaning of each bit is described below. D0 is
* the least significant bit. The Self Powered field indicates whether the
* device is currently self-powered. If D0 is reset to zero, the device is
* bus-powered. If D0 is set to one, the device is self-powered.The Remote
* Wakeup field indicates whether the device is currently enabled to
* request remote wakeup. The default mode for devices that support remote
* wakeup is disabled. If D1 is reset to zero, the ability of the device to
* signal remote wakeup is disabled. If D1 is set to one, the ability of
* the device to signal remote wakeup is enabled.
*
* INPUTS
*   cb          Pointer to the device control block.
*   status_out  Pointer to the variable to hold the device status.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Status (NU_USB_DEVICE * cb,
                                 UINT16 *status_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->stack);
    NU_USB_PTRCHK_RETURN(status_out);

    status = NU_USB_STACK_Get_Dev_Status (cb->stack, cb, status_out);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Set_Status
*
* DESCRIPTION
*      This function enables or disables the remote wake up capability of the
* device. Bit D1 of status should be 1 to enable and 0 to disable remote
* wake up. Rest of the bits in the status are reserved for now.  This
* function should be used on Host side. It should not be used by on the
* function side. Its host’s prerogative to enable or disable remote wakeup
* capability.
*
* INPUTS
*   cb          Pointer to the device control block.
*   status_out  pointer to the variable to hold the device status.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_Status (NU_USB_DEVICE * cb,
                                 UINT16 status)
{
    STATUS ret_status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->stack);

    ret_status = NU_USB_STACK_Set_Device_Status (cb->stack, cb, status);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ret_status;
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Get_OTG_Desc
*
* DESCRIPTION
*
*    This function returns a pointer to device OTG descriptor,
*    in otg_desc_out.
*
* INPUTS
*
*    cb               Pointer to the device control block.
*    otg_desc_out     Pointer to memory location to hold pointer to OTG
*                     descriptor.
*
* OUTPUTS
*
*    NU_SUCCESS       Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_OTG_Desc (NU_USB_DEVICE * cb,
                                   NU_USB_OTG_DESC ** otg_desc_out)
{
    NU_USB_CFG *active_cfg;
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(otg_desc_out);

    status = NU_USB_DEVICE_Get_Active_Cfg(cb, &active_cfg);

    if(active_cfg)
    {
        NU_USB_CFG_Get_OTG_Desc(active_cfg, otg_desc_out);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*               NU_USB_DEVICE_Get_OTG_Status
*
* DESCRIPTION
*
*    This function fills the current OTG status of the device in the
*    location pointed to by otg_desc_out.
*
*    The filled value is an 8-bit bitmap as described below :
*
*        Bit b0 indicates SRP support by the device.
*        Bit b1 indicates HNP support by the device.
*        Bits b2, b3 are reserved.
*        Bit b4 indicates if SRP is enabled on the device.
*        Bit b5 indicates if HNP is enabled on the device.
*        Bits b6, b7 are reserved.
*
*    Bit b0 and b2, if set, indicate that SRP/HNP is supported by the
*    device. If not set, device doesn't support these protocols and is not
*    OTG capable. Bits b4 and b5 must be ignored if b0 and b1 are not set
*    respectively.
*
*    Bit b4 and b5, if set, indicate that SRP/HNP is enabled on the device
*    by the host. If not set, device and is not currently OTG capable.
*
*
* INPUTS
*
*    cb               Pointer to the device control block.
*    otg_status_out   Pointer to memory location to hold OTG status
*
* OUTPUTS
*
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_OTG_Status (NU_USB_DEVICE * cb,
                                     UINT8         * otg_status_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(otg_status_out);

    *otg_status_out = cb->otg_status;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       NU_USB_DEVICE_Get_Current_Requirement
*
* DESCRIPTION
*
*       This function gets the value of the current required by a particular
*       configuration.
*
* INPUTS
*
*       cb              Pointer to the device control block.
*       cfg_num         number of configuration configuration for which
*                       current requirement is requested.
*       req_current     pointer to memory location to hold return value of
*                       the current.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the service.
*       NU_USB_INVLD_ARG        Indicates that the specified configuration 
*                               doesn't exists..
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_Current_Requirement (NU_USB_DEVICE *cb,
                                              UINT8 cfg_num,
                                              UINT32 *req_current)
{
    NU_USB_CFG *curr_cfg;
    STATUS status = NU_USB_INVLD_ARG;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(req_current);

    /* Find the configuration for which current requirement is requested. */
    curr_cfg = cb->config_descriptors[cfg_num];
    if(curr_cfg != NU_NULL)
    {
        *req_current = curr_cfg->desc->bMaxPower;
        status = NU_SUCCESS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Lock
*
* DESCRIPTION
*
*    This function locks the device for any data access for any other 
*    thread.
*
* INPUTS
*
*    cb               Pointer to the device control block.
*
* OUTPUTS
*
*    NU_SUCCESS       Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Lock(NU_USB_DEVICE * cb)
{
    STATUS  status;

    NU_USB_PTRCHK(cb);
    status = NU_Obtain_Semaphore(&(cb->lock), NU_SUSPEND);
    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_DEVICE_Unlock
*
* DESCRIPTION
*
*    This function unlocks the device for any data access by any thread.
*
* INPUTS
*
*    cb               Pointer to the device control block.
*
* OUTPUTS
*
*    NU_SUCCESS       Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Unlock(NU_USB_DEVICE * cb)
{
    STATUS status;

    NU_USB_PTRCHK(cb);
    status = NU_Release_Semaphore(&(cb->lock));
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USB_DEVICE_Set_Desc
*
* DESCRIPTION
*
*       This function initializes the device descriptor of a USB device.
*
* INPUTS
*
*       cb                  Pointer to the device control block.
*       device_desc         Pointer to memory location to holding pointer 
*                           to device descriptor.
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion of the 
*                           service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_Desc (NU_USB_DEVICE * cb,
                               NU_USB_DEVICE_DESC *device_desc)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device_desc);

    cb->device_descriptor.bLength               = device_desc->bLength;
    cb->device_descriptor.bDescriptorType       = device_desc->bDescriptorType;
    cb->device_descriptor.bcdUSB                = device_desc->bcdUSB;
    cb->device_descriptor.bDeviceClass          = device_desc->bDeviceClass;
    cb->device_descriptor.bDeviceSubClass       = device_desc->bDeviceSubClass;
    cb->device_descriptor.bDeviceProtocol       = device_desc->bDeviceProtocol;
    cb->device_descriptor.bMaxPacketSize0       = device_desc->bMaxPacketSize0;
    cb->device_descriptor.idVendor              = device_desc->idVendor;
    cb->device_descriptor.idProduct             = device_desc->idProduct;
    cb->device_descriptor.bcdDevice             = device_desc->bcdDevice;
    cb->device_descriptor.iManufacturer         = device_desc->iManufacturer;
    cb->device_descriptor.iProduct              = device_desc->iProduct;
    cb->device_descriptor.iSerialNumber         = device_desc->iSerialNumber;
    cb->device_descriptor.bNumConfigurations    = device_desc->bNumConfigurations;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       NU_USB_DEVICE_Set_String
*
* DESCRIPTION
*
*       This function sets the string pointed by 'string' at 
*       the specific index identified by 'str_index' variable.
*
* INPUTS
*
*       cb                  Pointer to the device control block.
*       str_index           number of the string as assigned in device 
*                           or configuration or interface descriptor.
*       string              Pointer to NU_USB_STRING control block, 
*                           holding a valid string.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion of the 
*                           service.
*       NU_USB_INVLD_ARG    Any of the input argument is invalid.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_String (NU_USB_DEVICE  *cb,
                                UINT8           str_index,
                                NU_USB_STRING   *string)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string);

    if ( str_index >= NU_USB_MAX_STRINGS )
    {
	    /* Switch back to user mode. */
	    NU_USER_MODE();
        return ( NU_USB_INVLD_ARG );
    }

    /* Save pointer to string descriptor. */
    cb->string_descriptors[str_index] = string; 
    cb->num_string_descriptors++;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( NU_SUCCESS );
}

/*************************************************************************
* FUNCTION
*
*       NU_USB_DEVICE_Set_Manf_String
*
* DESCRIPTION
*
*       This function takes pointer to NU_USB_STRING control block as an 
*       argument and save it to specific index in string descriptor array 
*       in NU_USB_DEVICE control block.
*       On successfully saving the string descriptor this function updates 
*       the iManufacturer member variable to point to the correct string 
*       descriptor index.
*
* INPUTS
*
*       cb                  Pointer to the device control block.
*       str_index           Index where this string descriptor is to be 
*                           saved.
*       string              pointer to NU_USB_STRING control block, 
*                           holding a valid string.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion of the 
*                           service.
*       NU_USB_INVLD_ARG    Indicates that the manufacturer string 
*                           number is invalid.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_Manf_String (NU_USB_DEVICE * cb,
                                     UINT8          str_index,
                                     NU_USB_STRING  *string)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string);

    status = NU_USB_INVLD_ARG;
    if ( str_index < NU_USB_MAX_STRINGS )
    {
        status =  NU_USB_DEVICE_Set_String (cb,
                                            str_index,
                                            string);
        if ( status == NU_SUCCESS )
        {
            cb->device_descriptor.iManufacturer = str_index;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /*  else No such string */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USB_DEVICE_Set_Product_String
*
* DESCRIPTION
*
*       This function takes a pointer to NU_USB_STRING control block as 
*       an argument and save it to specific index in string descriptor 
*       array in NU_USB_DEVICE control block.
*       On successfully saving the string descriptor this function updates 
*       the iProduct member variable to point to the correct string 
*       descriptor index.
*
* INPUTS
*
*       cb                  Pointer to the device control block.
*       str_index           Index where this string descriptor is to be 
*                           saved.
*       string              pointer to NU_USB_STRING control block, 
*                           holding a valid string.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion of the 
*                           service.
*       NU_USB_INVLD_ARG    Indicates that the product string 
*                           number is invalid.

*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_Product_String(NU_USB_DEVICE   *cb,
                                       UINT8            str_index,
                                       NU_USB_STRING    *string)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string);

    status = NU_USB_INVLD_ARG;
    if ( str_index < NU_USB_MAX_STRINGS )
    {
        status =  NU_USB_DEVICE_Set_String (cb,
                                            str_index,
                                            string);
        if ( status == NU_SUCCESS )
        {
            cb->device_descriptor.iProduct = str_index;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /*  else No such string */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USB_DEVICE_Set_Serial_Num_String
*
* DESCRIPTION
*
*       This function takes a pointer to NU_USB_STRING control block as 
*       an argument and save it to specific index in string descriptor 
*       array in NU_USB_DEVICE control block.
*       On successfully saving the string descriptor this function updates 
*       the iSerialNumber member variable to point to the correct string 
*       descriptor index.
*
* INPUTS
*
*       cb                  Pointer to the device control block.
*       str_index           Index where this string descriptor is to be 
*                           saved.
*       string              pointer to NU_USB_STRING control block, 
*                           holding a valid string.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion of the 
*                           service.
*       NU_USB_INVLD_ARG    Indicates that the manufacturer string 
*                           number is invalid.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_Serial_Num_String(NU_USB_DEVICE    *cb,
                                           UINT8            str_index,
                                           NU_USB_STRING    *string)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(string);

    status = NU_USB_INVLD_ARG;
    if ( str_index < NU_USB_MAX_STRINGS )
    {
        status =  NU_USB_DEVICE_Set_String (cb,
                                            str_index,
                                            string);
        if ( status == NU_SUCCESS )
        {
            cb->device_descriptor.iSerialNumber = str_index;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /*  else No such string */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USB_DEVICE_Set_Device_Qualifier
*
* DESCRIPTION
*
*       This function takes a pointer to NU_USB_DEVICE_QUALIFIER 
*       structure and save it's values in NU_USB_DEVICE control block.
*
* INPUTS
*
*       cb              Pointer to the device control block.
*       dev_qualifier   Pointer to device qualifier descriptor.
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_Device_Qualifier (NU_USB_DEVICE * cb,
                                    NU_USB_DEV_QUAL_DESC * dev_qualifier)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(dev_qualifier);

    cb->device_qualifier.bLength            = dev_qualifier->bLength;
    cb->device_qualifier.bDescriptorType    = dev_qualifier->bDescriptorType;
    cb->device_qualifier.bcdUSB             = dev_qualifier->bcdUSB;
    cb->device_qualifier.bDeviceClass       = dev_qualifier->bDeviceClass;
    cb->device_qualifier.bDeviceSubClass    = dev_qualifier->bDeviceSubClass;
    cb->device_qualifier.bDeviceProtocol    = dev_qualifier->bDeviceProtocol;
    cb->device_qualifier.bMaxPacketSize0    = dev_qualifier->bMaxPacketSize0;
    cb->device_qualifier.bNumConfigurations = dev_qualifier->bNumConfigurations;
    cb->device_qualifier.bReserved          = dev_qualifier->bReserved;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       NU_USB_DEVICE_Set_bcdUSB
*
* DESCRIPTION
*
*       This function sets the bcdUSB field of device descriptor.
*
* INPUTS
*
*       cb              Pointer to the device control block.
*       bcdUSB          bcdUSB value of device.
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_bcdUSB (NU_USB_DEVICE *cb,
                                 UINT16         bcdUSB)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    cb->device_descriptor.bcdUSB = bcdUSB;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       NU_USB_DEVICE_Set_bMaxPacketSize0
*
* DESCRIPTION
*
*       This function sets the Max packet sizeof default control 
*       endpoint field of device descriptor.
*
* INPUTS
*
*       cb              Pointer to the device control block.
*       bMaxPacketSize0 Max packet size of control endpoint.
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_bMaxPacketSize0 (NU_USB_DEVICE * cb,
                                          UINT8 bMaxPacketSize0)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    cb->device_descriptor.bMaxPacketSize0 = bMaxPacketSize0;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*       NU_USB_DEVICE_Set_bDeviceClass
*
* DESCRIPTION
*       This function returns the class code of the device(bDeviceClass field of
*       the device descriptor), in bDeviceClass_out.
*
* INPUTS
*       cb                  Pointer to the device control block.
*       bDeviceClass_out    Pointer to the variable to hold the class 
*                           code of the device.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion of the 
*                           service.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_bDeviceClass (NU_USB_DEVICE    *cb,
                                       UINT8            bDeviceClass)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    cb->device_descriptor.bDeviceClass = bDeviceClass;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/* Following functions should only be visible when stack is configured
 * for Super Speed USB (USB 3.0). */

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVICE_Get_BOS
*
*   DESCRIPTION
*
*       User can call this function to get a pointer of NU_USB_BOS control
*       block. A difference must be observed in this API and
*       NU_USB_DEVIC_Get_BOS_Desc which returns a pointer to BOS
*       descriptor.
*
*       NOTE: The function USB_Parse_Bos_Descriptor must be called before
*       calling this function. If this function is called before the call
*       to USB_Parse_Bos_Descriptor, the function will return will the
*       error code of NU_NOT_PRESENT.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_DEVICE control block.
*
*       bos_out         - Double pointer to NU_USB_BOS control block.
*                         This will point to a valid NU_USB_BOS control
*                         block or NU_NULL when the function returns.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘bos_out’ points to a valid BOS descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates BOS descriptor is not present.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_BOS        (NU_USB_DEVICE  *cb,
                                    NU_USB_BOS      **bos_out)
{
    STATUS status;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(bos_out);

    /* Check if bos is valid. */
    if (cb->bos != NU_NULL)
    {
        *bos_out    = cb->bos;
        status      = NU_SUCCESS;
    }
    else
    {
        status = NU_NOT_PRESENT;
    }
    
    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVICE_Get_BOS_Desc
*
*   DESCRIPTION
*
*       This API is used for extracting BOS descriptor from NU_USB_DEVICE
*       control block. This will only get the BOS descriptor not the device
*       capability descriptor. Separate APIs are provided to get device
*       capability descriptors.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_DEVICE control block.
*
*       bos_desc_out    - Double pointer to NU_USB_BOS_DESC control block
*                         This will point to a valid BOS descriptor or
*                         NU_NULL when the function returns.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘bos_desc_out’ points to a valid BOS descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - Indicates BOS descriptor is not present.
*
*       NU_USB_INVLD_DESC
*                       - Descriptor type of BOS descriptor (pointed by
*                         ‘cb->bos_desc’) is not USB_DT_BOS.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_BOS_Desc (NU_USB_DEVICE     *cb,
                                   NU_USB_BOS_DESC   **bos_desc_out)
{
    STATUS status;
    NU_USB_BOS_DESC *bos_desc;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(bos_desc_out);
    
    /* Initialize status with a default value. */
    status = NU_NOT_PRESENT;
    
    if ( cb->bos != NU_NULL)
    {
        *bos_desc_out   = NU_NULL;
        bos_desc        = cb->bos->bos_desc;
        
        /* Check if bos_desc is valid. */
        if (bos_desc != NU_NULL)
        {
            /* If the descriptor type is BOS descriptor. */
            if (bos_desc->bDescriptor == USB_DT_BOS)
            {
                *bos_desc_out   = bos_desc;
                status          = NU_SUCCESS;
            }
            else
            {
                status = NU_USB_INVLD_DESC;
            }
        }
    }
    
    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVICE_Get_USB2Ext_Desc
*
*   DESCRIPTION
*
*       This API is used for extracting USB 2.0 extension device capability
*       descriptor from NU_USB_DEVICE control block. According to USB 3.0
*       specifications, a Super Speed device shall include a USB 2.0
*       extension device capability descriptor.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_DEVICE control block.
*
*       usb2ext_desc_out
*                       - Double pointer to USB 2.0 extension device
*                         capability descriptor. This will point to a valid
*                         USB 2.0 extension descriptor or NU_NULL when
*                         function will return.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘usb2ext_desc_out’ points to a valid USB 2.0
*                         extension descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_USB_INVLD_DESC
*                       - Indicates one of the following 2 conditions:
*                         1. Descriptor type of USB 2.0 extension device
*                            capability descriptor (pointed by 
*                            ‘cb->devcap_usb2ext_desc’) is not
                             USB_DT_DEVCAP.
*                         2. Device capability type of USB 2.0 extension
*                            descriptor is not USB_DCT_USB2EXT.
*
*       NU_NOT_PRESENT  - USB 2.0 extension device capability descriptor is
*                         not present.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_USB2Ext_Desc (NU_USB_DEVICE  *cb,
                            NU_USB_DEVCAP_USB2EXT_DESC **usb2ext_desc_out)
{
    STATUS status;
    NU_USB_DEVCAP_USB2EXT_DESC *usb2ext_desc;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(usb2ext_desc_out);
    
    /* Initialize local variables. */
    status          = NU_NOT_PRESENT;
    usb2ext_desc    = NU_NULL;
        
    if (cb->bos != NU_NULL)
    {
        *usb2ext_desc_out   = NU_NULL;
        usb2ext_desc        = cb->bos->devcap_usb2ext_desc;
        
        /* If cb->bos->devcap_usb2exe_desc is valid. */
        if (usb2ext_desc != NU_NULL)
        {
            /* Descriptor is a device capability descriptor 
               and Device capability type is USB2EXT. */
            if ((usb2ext_desc->bDescriptorType == USB_DT_DEVCAP)
                && (usb2ext_desc->bDeviceCapabilityType ==
                                                     USB_DCT_USB2EXT))
            {
                *usb2ext_desc_out   = usb2ext_desc;
                status              = NU_SUCCESS;
            }
            else
            {
                status = NU_USB_INVLD_DESC;
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVICE_Get_SuprSpd_Desc
*
*   DESCRIPTION
*
*       This API is used for extracting Super Speed device capability
*       descriptor from NU_USB_DEVICE control block. According to USB 3.0
*       specifications, all Super Speed devices shall implement a
*       Super Speed device capability descriptor.
*
*   IPNUT
*
*       cb              - Pointer to NU_USB_DEVICE control block.
*
*       ss_desc_out     - Double pointer to Super Speed device capability
*                         descriptor. This will point to a valid Super
*                         Speed descriptor or NU_NULL when function will
*                         return.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘ss_desc_out’ points to a valid Super Speed
*                         descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_USB_INVLD_DESC
*                       - Indicates one of the following 2 conditions:
*                         1. Descriptor type of Super Speed device
*                            capability descriptor (pointed by 
*                            ‘cb->devcap_ss_desc’) is not USB_DT_DEVCAP.
*                         2. Device capability type of SuperSpeed device
*                            capability descriptor is not USB_DCT_USBSS.
*
*       NU_NOT_PRESENT  - Super Speed device capability descriptor is not
*                         present.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_SuprSpd_Desc(NU_USB_DEVICE   *cb,
                              NU_USB_DEVCAP_SUPERSPEED_DESC **ss_desc_out)
{
    STATUS status;
    NU_USB_DEVCAP_SUPERSPEED_DESC *ss_devcap_desc;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(ss_desc_out);
    
    /* Initialize local variables. */
    status          = NU_NOT_PRESENT;
    ss_devcap_desc  = NU_NULL;
    
    if (cb->bos != NU_NULL)
    {
        *ss_desc_out    = NU_NULL;
        ss_devcap_desc  = cb->bos->devcap_ss_desc;

        /* If bc->bos->devcap_ss_desc is valid. */
        if (ss_devcap_desc != NU_NULL)
        {
            /* If descriptor type is device capability. 
               and capability type is SuperSpeed. */
            if ((ss_devcap_desc->bDescriptorType == USB_DT_DEVCAP) &&
               (ss_devcap_desc->bDeviceCapabilityType ==
                                                        USB_DCT_USBSS))
            {
                *ss_desc_out    = ss_devcap_desc;
                status          = NU_SUCCESS;
            }
            else
            {
                status = NU_USB_INVLD_DESC;
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVICE_Get_CntnrID_Desc
*
*   DESCRIPTION
*
*       This API is used for extracting Container ID device capability
*       descriptor from NU_USB_DEVICE control block. According to USB 3.0
*       specifications, this descriptor is only mandatory for Hubs.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_DEVICE control block.
*
*       ss_desc_out     - Double pointer to Container ID device
*                         capability descriptor. This will point to a valid
*                         Container ID descriptor or NU_NULL when function
*                         will return.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘cid_desc_out’ points to a valid Container ID
*                         descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Indicates any of the input argument is invalid.
*
*       NU_USB_INVLD_DESC
*                       - Indicates one of the following 2 conditions:
*                         1. Descriptor type of ContainerID device
*                            capability descriptor (pointed by
*                            ‘cb->devcap_cid_desc’) is not USB_DT_DEVCAP.
*                         2. Device capability type of ContainerID device
*                            capability descriptor is not USB_DCT_CONTID.
*
*       NU_NOT_PRESENT  - Container ID device capability descriptor is not
*                         present.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Get_CntnrID_Desc(NU_USB_DEVICE   *cb,
                             NU_USB_DEVCAP_CONTAINERID_DESC **cid_desc_out)
{
    STATUS status;
    NU_USB_DEVCAP_CONTAINERID_DESC *cid_devcap_desc;
    
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cid_desc_out);
    
    /* Initialize local variables. */
    status              = NU_NOT_PRESENT;
    cid_devcap_desc     = NU_NULL;
    
    if (cb->bos != NU_NULL)
    {
        *cid_desc_out = NU_NULL;
        cid_devcap_desc = cb->bos->devcap_cid_desc;

        /* If bc->bos->devcap_cid_desc is valid. */
        if (cid_devcap_desc != NU_NULL)
        {
            /* If descriptor type is device capability. 
               and capability type is SuperSpeed. */
            if ((cid_devcap_desc->bDescriptorType == USB_DT_DEVCAP) &&
                (cid_devcap_desc->bDeviceCapabilityType ==
                                                       USB_DCT_CONTID))
            {
                *cid_desc_out = cid_devcap_desc;
                status = NU_SUCCESS;
            }
            else
            {
                status = NU_USB_INVLD_DESC;
            }
        }
    }
    
    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_DEVICE_Set_Link_State
*
*   DESCRIPTION
*
*       This API updates 'link_state' variable on NU_USB_DEVICE control
*       block to newly reported link state.
*
*   INPUTS
*
*       device              - Pointer to NU_USB_DEVICE control block.
*       link_state          - Variable containing current link state to be
*                             assigned.
*
*   OUTPUTS
*
*       NU_SUCCESS          - Indicates operation completed successfully.
*
*       NU_USB_INVLD_ARG    - Indicates any of the input argument is 
*                             invalid.
*
*************************************************************************/
STATUS NU_USB_DEVICE_Set_Link_State ( NU_USB_DEVICE   *device,
                                      UINT8           link_state )
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(device);

    device->link_state = link_state;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( NU_SUCCESS );
}
#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

#endif /* USB_DEVICE_EXT_C */
/*************************** end of file *********************************/
