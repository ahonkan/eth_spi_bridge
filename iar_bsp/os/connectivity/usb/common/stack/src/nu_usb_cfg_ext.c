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
*        nu_usb_cfg_ext.c
*
* COMPONENT
*   USB Base
*
* DESCRIPTION
*    This file contains implementation of NU_USB_CFG services.
*
* DATA STRUCTURES
*
*
* FUNCTIONS
*   NU_USB_CFG_Get_Is_Active        - returns if the config is active.
*   NU_USB_CFG_Set_Is_Active        - Activates or deactivates config.
*   NU_USB_CFG_Get_Desc             - returns a pointer to the config
*                                     descriptor.
*   NU_USB_CFG_Get_Cfg_Value        - returns bConfigurationValue field.
*   NU_USB_CFG_Get_Is_Self_Powered  - returns if config makes device self
*                                     powered.
*   NU_USB_CFG_Get_Is_Wakeup        - returns if config makes device remote
*                                     wakeup capable.
*   NU_USB_CFG_Get_Max_Power        - returns the max power needed by this
*                                     config.
*   NU_USB_CFG_Get_wTotalLength     - returns the total length of this
*                                     config.
*   NU_USB_CFG_Get_Num_Intfs        - returns the number of interfaces in
*                                     this config.
*   NU_USB_CFG_Get_Intf             - returns pointer to interface control
*                                     block.

*   NU_USB_CFG_Find_Alt_Setting     - Finds an alternate setting as per the
*                                     specified match criteria.
*   NU_USB_CFG_Get_String           - returns a copy of the config string.
*   NU_USB_CFG_Get_String_Num       - returns the string index of the
*                                     config string.
*   NU_USB_CFG_Get_String_Desc      - returns a pointer to the config's
*                                     string descriptor.
*   NU_USB_CFG_Get_Device           - returns the pointer to the device
*                                     control block.
*   NU_USB_CFG_Get_Num_IADs         - returns the number of interface
*                                     associates in this config.
*   NU_USB_CFG_Get_IAD              - returns pointer to associate interface
*                                     control*
*
* DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_CFG_EXT_C
#define USB_CFG_EXT_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_Desc
*
* DESCRIPTION
*    This function returns pointer to configuration descriptor (in
* cfg_desc_out) associated with the specified configuration control block.
*
*
* INPUTS
*    cb            Pointer to the configuration control block.
*    cfg_desc_out  Pointer to a memory location to hold the pointer to the
*                  configuration descriptor structure.
*
* OUTPUTS
*   NU_SUCCESS  Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_Desc (NU_USB_CFG * cb,
                            NU_USB_CFG_DESC ** cfg_desc_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(cfg_desc_out);
    NU_USB_PTRCHK(cb->desc);

    *cfg_desc_out = cb->desc;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*               NU_USB_CFG_Get_OTG_Desc
*
* DESCRIPTION
*
*    This function returns pointer to the OTG descriptor (in
*    otg_desc_out) associated with the specified configuration control block.
*
* INPUTS
*    cb            Pointer to the configuration control block.
*    otg_desc_out  Pointer to a memory location to hold the pointer to the
*                  OTG descriptor structure.
*
* OUTPUTS
*   NU_SUCCESS  Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_OTG_Desc (NU_USB_CFG * cb,
                                NU_USB_OTG_DESC ** otg_desc_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(otg_desc_out);
    NU_USB_PTRCHK(cb->otg_desc);

    *otg_desc_out = cb->otg_desc;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_Class_Desc
*
* DESCRIPTION
*   This function returns pointer to the class specific configuration
*   descriptor (in class_desc_out) and its length in length_out.
*
*
* INPUTS
*   cb                     Pointer to the configuration control block.
*   class_desc_out         Pointer to a memory location to hold the pointer
*                          to class specific configuration descriptor.
*   length_out             Pointer to the variable to hold the length of
*                          the descriptor in bytes.
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_Class_Desc (NU_USB_CFG * cb,UINT8 **class_desc_out,
                                  UINT32 *length_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(class_desc_out);
    NU_USB_PTRCHK(length_out);

    *class_desc_out = cb->class_specific;
    *length_out = cb->length;

    return (NU_SUCCESS);

}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_Is_Active
*
* DESCRIPTION
*     This function returns NU_TRUE in is_active_out, if it’s the currently
* active configuration of the device, else is_active_out would contain
* NU_FALSE on return.
*
* INPUTS
*     cb                 Pointer to the configuration control block.
*     is_active_out      Pointer to the variable to hold active state of the
*                    configuration.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_Is_Active (NU_USB_CFG * cb,
                                 BOOLEAN * is_active_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(is_active_out);

    *is_active_out = cb->is_active;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Set_Is_Active
*
* DESCRIPTION
*      If is_active is NU_TRUE, this function sets the configuration as active
* on the device.  The USB bandwidth and necessary resources in the
* controller hardware are allocated to make the configuration active. If
* is_active is NU_FALSE, and if the configuration is currently active,
* this function makes the current configuration as inactive and puts the
* device is un-configured state.USB interface drivers aren’t expected to
* make this call as they just own an interface on the device and so only
* USB device drivers are entitled to make this call. This function should
* be used only by class drivers on Host side. It should not be used by
* class drivers on the function side. Its host’s prerogative to select and
* activate/deactivate a configuration on the device.
*
*
* INPUTS
*     cb            Pointer to the configuration control block that is to
*               activated/deactivated.
*     is_active NU_TRUE to make the configuration active and NU_FALSE to make
*               the configuration inactive
*
* OUTPUTS
*    NU_SUCCESS             Indicates successful completion of the service.
*    NU_USB_NO_BANDWIDTH        Indicates that the device's b/w requirements of the
*                           specified configuration cannot be met.
*    NU_INVALID_SEMAPHORE       Indicates that the one of internal semaphore pointer
*                           is corrupted.
*    NU_SEMAPHORE_DELETED       Indicates that one of the internal semaphore was
*                           deleted.
*    NU_UNAVAILABLE             Indicates that one of the internal semaphore is
*                           unavailable.
*    NU_INVALID_SUSPEND     Indicates that this API is called from a non-task
*                           thread.
*    NU_USB_INVLD_ARG       Indicates that device control block or configuration
*                           control block passed to this function are invalid.
*
*************************************************************************/

STATUS NU_USB_CFG_Set_Is_Active (NU_USB_CFG * cb,
                                 BOOLEAN is_active)
{
    STATUS status;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(cb->device);
    NU_USB_PTRCHK(cb->desc);

    if (is_active == NU_TRUE)
        return NU_USB_STACK_Set_Config (cb->device->stack, cb->device,
                                        cb->desc->bConfigurationValue);
    else
    {
        status = NU_USB_STACK_Set_Config (cb->device->stack, cb->device, 0);
        if (status == NU_SUCCESS)
            cb->is_active = NU_FALSE;
        return (status);
    }
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_Cfg_Value
*
* DESCRIPTION
*    This function returns the bConfigurationValue field of the associated
* configuration descriptor, in cfg_value_out.
*
* INPUTS
*    cb             Pointer to the configuration control block.
*    cfg_value_out      Pointer to the variable to hold the bConfigurationValue.
*
* OUTPUTS
*    NU_SUCCESS             Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_Cfg_Value (NU_USB_CFG * cb,
                                 UINT8 *cfg_value_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(cfg_value_out);
    NU_USB_PTRCHK(cb->desc);

    *cfg_value_out = cb->desc->bConfigurationValue;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_Is_Self_Powered
*
* DESCRIPTION
*     This function returns the NU_TRUE in is_self_powered_out if
* bmAttributes field of the associated configuration descriptor indicates
* self powered and NU_FALSE otherwise.
*
* INPUTS
*   cb                  Pointer to the configuration control block.
*   is_self_powered_out Pointer to the variable to hold the self powered
*                       nature of the configuration..
*
* OUTPUTS
*    NU_SUCCESS             Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_Is_Self_Powered (NU_USB_CFG * cb,
                                       BOOLEAN * is_self_powered_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(is_self_powered_out);
    NU_USB_PTRCHK(cb->desc);

    /* Extract the self powered bits from the attributes field */
    if( cb->desc->bmAttributes & 0x40 )
        *is_self_powered_out = NU_TRUE;
    else
        *is_self_powered_out = NU_FALSE;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_Is_Wakeup
*
* DESCRIPTION
*     This function returns the NU_TRUE in is_wakeup_out, if bmAttributes
* field of the associated configuration descriptor indicates wakeup
* capable and NU_FALSE otherwise.
*
* INPUTS
*    cb             Pointer to the configuration control block.
*    is_wakeup_out      Pointer to the variable to hold the wakeup capability of
*                   the configuration.
*
* OUTPUTS
*    NU_SUCCESS             Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_Is_Wakeup (NU_USB_CFG * cb,
                                 BOOLEAN * is_wakeup_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(is_wakeup_out);
    NU_USB_PTRCHK(cb->desc);

    /* Extract the wake up bits from the attributes field */
    if( cb->desc->bmAttributes & 0x20 )
                *is_wakeup_out = NU_TRUE;
        else
                *is_wakeup_out = NU_FALSE;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_Max_Power
*
* DESCRIPTION
*   This function returns the bMaxPower field of the associated configuration
* descriptor, in max_power_out.
*
* INPUTS
*   cb              Pointer to the configuration control block.
*   max_power_out       Pointer to the variable to hold the bMaxPower.
*
*
* OUTPUTS
*    NU_SUCCESS             Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_Max_Power (NU_USB_CFG * cb,
                                 UINT8 *max_power_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(max_power_out);
    NU_USB_PTRCHK(cb->desc);

    *max_power_out = cb->desc->bMaxPower;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_Num_Intfs
*
* DESCRIPTION
*    This function returns the bNumInterfaces field of the associated
* configuration descriptor in number_intfs_out.
*
*
* INPUTS
*  cb               Pointer to the configuration control block.
*  number_intfs_out     pointer to the variable to hold bNumInterfaces field.
*
* OUTPUTS
*    NU_SUCCESS             Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_Num_Intfs (NU_USB_CFG * cb,
                                 UINT8 *number_intfs_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(number_intfs_out);
    NU_USB_PTRCHK(cb->desc);

    *number_intfs_out = cb->desc->bNumInterfaces;

    return (NU_SUCCESS);

}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_Intf
*
* DESCRIPTION
*    This function returns the pointer (in intf_out) to the control block of
* the interface associated with the specified interface.
*
* INPUTS
*  cb       Pointer to the configuration control block.
*  intf_num     Interface number whose interface control block pointer is desired.
*  intf_out     Pointer to a memory location to hold pointer to the interface
*           control block.
*
*
* OUTPUTS
*    NU_SUCCESS             Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_Intf (NU_USB_CFG * cb,
                            UINT8 intf_num,
                            NU_USB_INTF ** intf_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(intf_out);

    if( intf_num > NU_USB_MAX_INTERFACES )
    {
        return NU_USB_INVLD_ARG;
    }

    *intf_out = &cb->intf[intf_num];

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_wTotalLength
*
* DESCRIPTION
*    This function returns the wTotalLength field of the associated
* configuration descriptor, in wtotalLength_out.
*
* INPUTS
*    cb                 Pointer to the configuration control block.
*    wTotalLength_out   Pointer to the variable to hold the wTotalLength.
*
* OUTPUTS
*    NU_SUCCESS             Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_wTotalLength (NU_USB_CFG * cb,
                                    UINT32 *wTotalLength_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(wTotalLength_out);
    NU_USB_PTRCHK(cb->desc);

    *wTotalLength_out = cb->desc->wTotalLength;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_String
*
* DESCRIPTION
*     This function copies the configuration string in to string_out. English
* is assumed to be the Unicode of the configuration string descriptor. If
* the Unicode is not English, refer to NU_USB_CFG_Get_String_Desc to know
* how to retrieve a displayable string.
*
*
* INPUTS
*    cb         Pointer to the configuration control block.
*    string_out pointer to the user supplied array of characters of size
*               NU_USB_MAX_STRING_LEN.
*
* OUTPUTS
*    NU_SUCCESS     Indicates successful completion of the service.
*    NU_NOT_PRESENT     Indicates that the configuration string number cannot be
*                   found in the device’s set of string descriptors.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_String (NU_USB_CFG * cb,
                              CHAR * string_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(string_out);
    NU_USB_PTRCHK(cb->desc);

    /* all string functions are implemented by NU_USB_DEVICE */
    return NU_USB_DEVICE_Get_String (cb->device, cb->desc->iConfiguration,
                                     string_out);
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_String_Num
*
* DESCRIPTION
*     This function returns iConfiguration field of the configuration
* descriptor in string_num_out. If string_num_out is 0, it means that
* there is no configuration string descriptor associated with this
* configuration.
*
* INPUTS
*  cb               Pointer to the configuration control block.
*  string_num_out       pointer to the variable to hold iConfiguration field of
*                   the configuration descriptor.
*
* OUTPUTS
*    NU_SUCCESS     Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_String_Num (NU_USB_CFG * cb,
                                  UINT8 *string_num_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(string_num_out);
    NU_USB_PTRCHK(cb->desc);

    *string_num_out = cb->desc->iConfiguration;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_String_Desc
*
* DESCRIPTION
*      This function gets pointer to the string descriptor (in to
* string_desc_out) for the configuration string number (iConfiguration
* field of configuration descriptor). If the Unicode used for string
* descriptors is English, NU_USB_CFG_Get_String can be used to directly
* get the ASCII string. For other Unicodes NU_USB_CFG_Get_String_Desc
* function would help retrieve the descriptor and then reader may write
* routines to convert from their Unicode format to ASCII string or such
* other display string notations.
*
*
* INPUTS
*   cb              Pointer to the configuration control block.
*   string_desc_out     pointer to memory location to hold pointer to string
*                   descriptor structure.
*
* OUTPUTS
*    NU_SUCCESS     Indicates successful completion of the service.
*    NU_NOT_PRESENT     Indicates that the configuration string number cannot be
*
*************************************************************************/

STATUS NU_USB_CFG_Get_String_Desc (NU_USB_CFG * cb,
                                   UINT16 wLangId,
                                   NU_USB_STRING_DESC * string_desc_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(string_desc_out);
    NU_USB_PTRCHK(cb->desc);

    return NU_USB_DEVICE_Get_String_Desc (cb->device, cb->desc->iConfiguration,
                                          wLangId,
                                          string_desc_out);
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_Device
*
* DESCRIPTION
*     This function returns the pointer to the control block of the associated
* device, in device_out.
*
* INPUTS
*   cb          Pointer to the device control block.
*   device_out  Pointer to memory location to hold pointer to the device
*               control block.
*
* OUTPUTS
*    NU_SUCCESS     Indicates successful completion of the service.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_Device (NU_USB_CFG * cb,
                              NU_USB_DEVICE ** device_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(device_out);

    *device_out = cb->device;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Find_Alt_Setting
*
* DESCRIPTION
*     This function finds a matching alternate setting among various alternate
* settings of all the interfaces. match_flag specifies the search criteria
* and the rest of the arguments provide the search keys. The pointer to
* control block of the matching alternate setting is returned in
* alt_settg_out.
*
* INPUTS
*    cb                 Pointer to the configuration control block.
*    match_flag         OR of one or more of the following:
*                       USB_MATCH_ONLY_ACTIVE_ALT_STTG- match only against the
*                       active alternate setting. Do not consider non-active
*                       alternate settings for a match.
*                       USB_MATCH_ CLASS-search for alternate setting whose
*                       bInterfaceClass equals the specified bInterfaceClass.
*                       USB_MATCH_SUB_CLASS -search for alternate setting whose
*                       bInterfaceSubClass equals the specified
*                       bInterfaceSubClass. match_flag must also necessarily
*                       contain USB_MATCH_CLASS.
*                       USB_MATCH_PROTOCOL-search for alternate setting whose
*                       bInterfaceProtocol equals the specified
*                       bInterfaceProtocol. match_flag must also necessarily
*                       contain USB_MATCH_SUB_CLASS.
*    intf_num           Interface number from which the search should begin.
*    alt_settg          Alternate setting Number of the specified interface,
*                       from which the search should begin.
*    bInterfaceClass    The desired value of the bInterfaceClass in the
*                       matching alternate setting.
*    bInterfaceSubClass The desired value of the bInterfaceSubClass in the
*                       matching alternate setting. Irrelevant, if match_flag
*                       doesn’t contain USB_MATCH_SUB_CLASS.
*    bInterfaceProtocol The desired value of the bInterfaceProtocol in the
*                       matching alternate setting. Irrelevant, if match_flag
*                       doesn’t contain USB_MATCH_PROTOCOL.
*    alt_settg_out          Pointer to a memory location to hold the pointer to the
*                       control block of the matching alternate setting.
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion of the service.
*   NU_USB_INVLD_ARG    Indicates that configuration control block or 
*                       output pointer passed to this function are invalid. 
*   NU_NOT_PRESENT      Indicates that specified Alternate setting 
*                       doesn't exist.
*************************************************************************/

STATUS NU_USB_CFG_Find_Alt_Setting (NU_USB_CFG * cb,
                                    UINT32 match_flag,
                                    UINT8 intf_num,
                                    UINT8 alt_settg,
                                    UINT8 bInterfaceClass,
                                    UINT8 bInterfaceSubClass,
                                    UINT8 bInterfaceProtocol,
                                    NU_USB_ALT_SETTG ** alt_settg_out)
{
    UINT8 i;
    STATUS status;
    NU_USB_ALT_SETTG *alt_setting = NU_NULL;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(alt_settg_out);
    NU_USB_PTRCHK(cb->desc);
    NU_USB_PTRCHK(cb->device);
    NU_USB_PTRCHK(cb->device->stack);

    /* Check the validity of the device control block */
    if (!NU_USB_STACK_Is_Valid_Device (cb->device->stack, cb->device))
    {
        return NU_USB_INVLD_ARG;
    }

    /* For every interface contained in the interface, search the alternate
     * settings.
     */
    for (i = intf_num; i < cb->desc->bNumInterfaces; i++)
    {
        status = NU_USB_INTF_Find_Alt_Setting (&cb->intf[i], match_flag, alt_settg,
                                      bInterfaceClass, bInterfaceSubClass,
                                      bInterfaceProtocol, &alt_setting);

        /* We don't check status here as in case of non-success status
         * alt_setting is NU_NULL and we want to check for the next
         * interface. If no alternate setting would be found, we shall
         * return NU_NOT_PRESENT in the end.
         */
        if (alt_setting)
        {
            if (status != NU_SUCCESS)
            {
                return (status);
            }

            *alt_settg_out = alt_setting;
            return (NU_SUCCESS);
        }
    }

    return NU_NOT_PRESENT;
}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_Num_IADs
*
* DESCRIPTION
*    This function returns the bNumInterfaces field of the associated
* configuration descriptor in number_intfs_out.
*
*
* INPUTS
*  cb               Pointer to the configuration control block.
*  number_iads_out      pointer to the variable to hold the number of interface
*                   associate component.
*
* OUTPUTS
*    NU_SUCCESS             Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_CFG_Get_Num_IADs (NU_USB_CFG * cb,
                                UINT8 *number_iads_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(number_iads_out);
    NU_USB_PTRCHK(cb->desc);

    /* Cast to UINT8 to remove Lint Warning. */
    *number_iads_out = (UINT8)cb->numIADs;

    return (NU_SUCCESS);

}

/*************************************************************************
* FUNCTION
*               NU_USB_CFG_Get_IAD
*
* DESCRIPTION
*    This function returns the pointer (in iad_out) to the control block of
* the interface associate component, which the specified interface belongs to.
*
* INPUTS
*  cb       Pointer to the configuration control block.
*  intf_num     Interface number whose interface control block pointer is desired.
*  iad_out      Pointer to a memory location to hold pointer to the interface
*           associate control block.
*
*
* OUTPUTS
*    NU_SUCCESS             Indicates successful completion of the service.
*    NU_USB_INVLD_ARG       Indicates no appropriate interface associate
*                           component found.
*
*************************************************************************/

STATUS NU_USB_CFG_Get_IAD (NU_USB_CFG * cb,
                           UINT8 intf_num,
                           NU_USB_IAD ** iad_out)
{
    UINT8 i;
    STATUS status;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(iad_out);

    if( intf_num < NU_USB_MAX_INTERFACES )
    {
        return NU_USB_INVLD_ARG;
    }

    for (i = 0; i < (UINT8)cb->numIADs; i++)
    {
        status = NU_USB_IAD_Check_Interface(&cb->iad[i], intf_num);
        if (status == NU_SUCCESS)
        {
            *iad_out = &cb->iad[i];
            return (NU_SUCCESS);
        }
    }

    return NU_USB_INVLD_ARG;
}

/*************************************************************************/

#endif /* USB_CFG_EXT_C */
/************************* end of file *********************************/

