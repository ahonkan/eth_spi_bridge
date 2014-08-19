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
*        nu_usb_intf_ext.c
*
* COMPONENT
*        USB Base
*
* DESCRIPTION
*        This file contains the implementation of the NU_USB_INTF services.
*
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*    NU_USB_INTF_Claim                       - The driver is assigned the
*                                              ownership of the interface.
*    NU_USB_INTF_Release                     - The driver is relinquished
*                                              of the ownership of the
*                                              interface.
*    NU_USB_INTF_Get_Is_Claimed              - Checks if the interface has
*                                              owner.
*    NU_USB_INTF_Set_Interface               - Sets the specified alt
*                                              setting as active.
*    NU_USB_INTF_Get_Alt_Setting             - returns a pointer to the
*                                              specified alt setting control
*                                              block.
*    NU_USB_INTF_Get_Active_Alt_Setting      - returns pointer to the
*                                              control block of the active alt
*                                              setting.
*    NU_USB_INTF_Get_Active_Alt_Setting_Num  - returns bAlternateSetting
*                                              filed of the active alt settings
*                                              interface descriptor.
*    NU_USB_INTF_Find_Alt_Setting            - finds an alt setting as per
*                                              the specified match criteria.
*    NU_USB_INTF_Get_Desc                    - returns a pointer to the
*                                              interface descriptor of this
*                                              interface.
*    NU_USB_INTF_Get_Intf_Num                - returns bInterfaceNumber
*                                              field of the interface descriptor
*    NU_USB_INTF_Get_Class                   - returns bInterfaceClass
*                                              field of the interface descriptor
*    NU_USB_INTF_Get_SubClass                - returns bInterfaceSubClass
*                                              field of the interface descriptor
*    NU_USB_INTF_Get_Protocol                - returns bInterfaceProtocol
*                                              field of the interface descriptor
*    NU_USB_INTF_Get_String                  - returns interface string in
*                                              ascii format.
*    NU_USB_INTF_Get_String_Num              - returns iInterface field of
*                                              the interface descriptor.
*    NU_USB_INTF_Get_String_Desc             - returns a pointer to the
*                                              interface string descriptor.
*    NU_USB_INTF_Get_Num_Alt_Settings        - returns the number of alt
*                                              settings the interface has.
*    NU_USB_INTF_Get_Cfg                     - returns pointer to the
*                                              control block of the associated
*                                              configuration.
*    NU_USB_INTF_Get_Device                  - returns pointer to the
*                                              control block of the device.
*    NU_USB_INTF_Get_IAD                     - returns pointer to the
*                                              interface associate control
*                                              block if existed.
*
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_INTF_EXT_C
#define USB_INTF_EXT_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_Intf_Num
*
* DESCRIPTION
*    This function returns the bInterfaceNumber field of the currently active
* alternate setting’s interface descriptor, in intf_num_out.
*
* INPUTS
*   cb             Pointer to the interface control block.
*   intf_num_out   Pointer to the variable to hold the bInterfaceNumber field of
*                  the currently active alternate setting’s interface descriptor.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_Intf_Num (NU_USB_INTF * cb,
                                 UINT8 *intf_num_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(intf_num_out);

    *intf_num_out = cb->intf_num;

    return (NU_SUCCESS);

}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_Desc
*
* DESCRIPTION
*    This function returns the bInterfaceNumber field of the currently active
* alternate setting’s interface descriptor, in intf_num_out.
*
* INPUTS
*   cb             Pointer to the interface control block.
*   intf_num_out   Pointer to hold the bInterfaceNumber field of the currently
*                  active alternate setting’s interface descriptor.
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_Desc (NU_USB_INTF * cb,
                             NU_USB_INTF_DESC ** intf_desc_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(intf_desc_out);

    return NU_USB_ALT_SETTG_Get_Desc (cb->current, intf_desc_out);

}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_Class
*
* DESCRIPTION
*    This function returns the bInterfaceClass field of the currently active
* alternate setting’s interface descriptor, in bInterfaceClass_out.
*
* INPUTS
*   cb                  Pointer to the interface control block.
*   bInterfaceClass_out Pointer to the variable to hold bInterfaceClass field
*                       of the currently active alternate setting’s interface
*                       descriptor.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_Class (NU_USB_INTF * cb,
                              UINT8 *bInterfaceClass_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(bInterfaceClass_out);

    return NU_USB_ALT_SETTG_Get_Class (cb->current, bInterfaceClass_out);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_SubClass
*
* DESCRIPTION
*    This function returns the bInterfaceSubClass field of the currently active
* alternate setting’s interface descriptor, in bInterfaceSubClass_out.
*
* INPUTS
*   cb                      Pointer to the interface control block.
*   bInterfaceSubClass_out      Pointer to the variable to hold the
*                           bInterfaceSubClass field of the currently active
*                           alternate setting’s interface descriptor.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_SubClass (NU_USB_INTF * cb,
                                 UINT8 *bInterfaceSubClass_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(bInterfaceSubClass_out);

    return NU_USB_ALT_SETTG_Get_SubClass (cb->current, bInterfaceSubClass_out);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_Protocol
*
* DESCRIPTION
*    This function returns the bInterfaceProtocol field of the currently active
* alternate setting’s interface descriptor, in bInterfaceProtocol_out.
*
* INPUTS
*   cb                      Pointer to the interface control block.
*   bInterfaceProtocol_out      Pointer to the variable to hold the
*                           bInterfaceProtocol field of the currently active
*                           alternate setting’s interface descriptor.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_Protocol (NU_USB_INTF * cb,
                                 UINT8 *bInterfaceProtocol_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(bInterfaceProtocol_out);

    return NU_USB_ALT_SETTG_Get_Protocol (cb->current, bInterfaceProtocol_out);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_String
*
* DESCRIPTION
*    This function copies the interface string corresponding to the currently
* active alternate setting,  in to string_out. English is assumed to be
* the Unicode of the configuration string descriptor. If the Unicode is
* not English, refer to NU_USB_INTF_Get_String_Desc to know how to
* retrieve a displayable string.
*
* INPUTS
*   cb          Pointer to the interface control block.
*   string_out  pointer to the user supplied array of characters of size
*               NU_USB_MAX_STRING_LEN.
*
* OUTPUTS
*   NU_SUCCESS      Indicates successful completion of the service.
*   NU_NOT_PRESENT      Indicates that the interface string number cannot be found
*                   in the device’s set of string descriptors.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_String (NU_USB_INTF * cb,
                               CHAR * string_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(string_out);

    return NU_USB_ALT_SETTG_Get_String (cb->current, string_out);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_String_Num
*
* DESCRIPTION
* This function returns iInterface field of the interface descriptor of
* the currently active alternate setting, in string_num_out. If
* string_num_out is 0, it means that there is no interface string
* descriptor associated with this alternate setting.
*
* INPUTS
*  cb               Pointer to the interface control block.
*  string_num_out       pointer to the variable to hold iInterface field of the
*                   interface descriptor.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_String_Num (NU_USB_INTF * cb,
                                   UINT8 *string_num_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(string_num_out);

    return NU_USB_ALT_SETTG_Get_String_Num (cb->current, string_num_out);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_String_Desc
*
* DESCRIPTION
* This function gets pointer to the string descriptor (in to
* string_desc_out) for the interface string number (iInterface field of
* interface descriptor) of the currently active alternate setting. If the
* Unicode used for string descriptors is English, NU_USB_INTF_Get_ String
* can be used to directly get the ASCII string. For other Unicodes
* NU_USB_INTF_Get_String_Desc function would help retrieve the descriptor
* and then reader may write routines to convert from their Unicode format
* to ASCII string or such other display string notations.
*
* INPUTS
*   cb              Pointer to the interface control block.
*   string_desc_out     pointer to memory location to hold pointer to string
*                   descriptor structure.
*
* OUTPUTS
*   NU_SUCCESS      Indicates successful completion of the service.
*   NU_NOT_PRESENT      Indicates that the interface string number cannot be found
*                   in the device’s set of string descriptors.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_String_Desc (NU_USB_INTF * cb,
                                    UINT16 wLangId,
                                    NU_USB_STRING_DESC * string_desc_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(string_desc_out);

    return NU_USB_ALT_SETTG_Get_String_Desc (cb->current, wLangId,
            string_desc_out);

}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_Num_Alt_Settings
*
* DESCRIPTION
*    The count of the alternate settings that this interface has is returned by
* this function, in number_alt_settings_out.
*
* INPUTS
*   cb                      Pointer to the interface control block.
*   number_alt_settings_out     Number of alternate settings this interface has. It
*                           would be in the range of 1  to NU_USB_MAX_ALT_SETTINGS
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_Num_Alt_Settings (NU_USB_INTF * cb,
                                         UINT8 *number_alt_settings_out)
{
    UINT8 i, count = 0;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(number_alt_settings_out);

    for (i = 0; i < NU_USB_MAX_ALT_SETTINGS; i++)
    {
        if (cb->alt_settg[i].desc)
            count++;
    }
    *number_alt_settings_out = count;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_Alt_Setting
*
* DESCRIPTION
*    This function returns a pointer to the control block of the alternate
* setting (in alt_setting_out) that corresponds to the specified
* bAlternateSettingNum.
*
* INPUTS
*  cb               Pointer to the interface control block.
*  alt_setting_num      bAlternateSettingNum of the desired alternate setting.
*  alt_setting_out      Pointer to a memory location to hold pointer to pointer to
*                   the control block of the desired alternate setting
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_Alt_Setting (NU_USB_INTF * cb,
                                    UINT8 alt_setting_num,
                                    NU_USB_ALT_SETTG ** alt_setting_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(alt_setting_out);

    *alt_setting_out = &cb->alt_settg[alt_setting_num];
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_Active_Alt_Setting
*
* DESCRIPTION
*    This function returns a pointer to the control block of the alternate
* setting (in alt_setting_out) thats currently the active one. NU_NULL is
* returned in alt_setting_out, if no alternate setting is active on the
* interface.
*
* INPUTS
*   cb              Pointer to the interface control block.
*   alt_setting_out     Pointer to memory location to hold the pointer to the
*                   control block of the active alternate setting.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_Active_Alt_Setting (NU_USB_INTF * cb,
                                           NU_USB_ALT_SETTG ** alt_setting_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(alt_setting_out);

    *alt_setting_out = cb->current;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_Active_Alt_Setting_Num
*
* DESCRIPTION
*     This function returns bAlternateSettingNum of the alternate setting (in
* alt_setting_out) thats currently active. NU_NOT_PRESENT is returned, if
* no alternate setting is active on the interface .
*
* INPUTS
*   cb                  Pointer to the interface control block.
*   alt_setting_num_out Pointer to the variable to hold bAlternateSettingNum of
*                       the currently active alternate setting.
*
* OUTPUTS
*    NU_SUCCESS     Indicates successful completion of the service.
*    NU_NOT_PRESENT     Indicates that there is no active alternate setting on the
*                   interface.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_Active_Alt_Setting_Num (NU_USB_INTF * cb,
                                               UINT8 *alt_setting_num_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(alt_setting_num_out);

    if (cb->current)
        return NU_USB_ALT_SETTG_Get_bAlternateSetting (cb->current,
                                                       alt_setting_num_out);
    else
        return NU_NOT_PRESENT;
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Set_Interface
*
* DESCRIPTION
*    This function makes the specified alternate setting as the active
* alternate setting of the interface. The resources necessary in the
* controller h/w are allocated and the required bandwidth on the bus is
* reserved by this function. This function should be used only by class
* drivers on Host side and should not be used by class drivers on the
* function side. Its host’s prerogative to select and activate an
* alternate setting on an interface.
*
* INPUTS
*    cb               Pointer to the interface control block.
*    alt_setting_num  bAlternateSettingNum of the alternate setting that is to
*                     be made active.
* OUTPUTS
*    NU_SUCCESS             Indicates successful completion of the service.
*    NU_USB_NO_BANDWIDTH        Indicates that the device's b/w requirements of the
*                           specified configuration cannot be met.
*    NU_INVALID_SEMAPHORE       Indicates that one of the internal semaphore pointer
*                           is invalid.
*    NU_SEMAPHORE_DELETED       Indicates that one of the internal semaphores was
*                           deleted.
*    NU_UNAVAILABLE             Indicates that one of the internal semaphores is
*                           unavailable.
*    NU_INVALID_SUSPEND     Indicates that this API is called from a non-task
*                           thread.
*    NU_USB_INVLD_ARG       Indicates that one or more arguments passed to this
*                           function are invalid.
*
*************************************************************************/
STATUS NU_USB_INTF_Set_Interface (NU_USB_INTF * cb,
                                  UINT8 alt_setting_num)
{
    STATUS status = NU_SUCCESS;
    NU_USB_STACK *stack;

    NU_USB_PTRCHK(cb);

    status = NU_USB_DEVICE_Get_Stack (cb->device, &stack);
    if (status != NU_SUCCESS)
    {
        return (status);
    }

    /* Host, Function have their own implementations. */
    return NU_USB_STACK_Set_Intf (stack, cb->device, cb->intf_num,
                                  alt_setting_num);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Claim
*
* DESCRIPTION
*    This function sets the driver as the driver owning the interface. A USB
* interface class driver may call this function from its
* NU_USB_DRVR_Initialize_Intf, once it decides to own the interface.
* Drivers usually decide to take the ownership of the interface after
* successfully locating all the necessary pipes and notifying relevant
* users.
*
* INPUTS
*    cb    Pointer to the interface control block. Note: This pointer is passed
*          to the USB driver when the stack invokes USB driver’s
*          NU_USB_DRVR_Initialize_Intf function.
*    drvr  pointer to the control block of the drvr that decided to own the
*          interface.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
/* Services for class drivers */
STATUS NU_USB_INTF_Claim (NU_USB_INTF * cb,
                          NU_USB_DRVR * drvr)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(drvr);

    cb->driver = drvr;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Release
*
* DESCRIPTION
*    This function releases the ownership of the interface. A USB interface
* driver that currently owns the interface is only expected to make this
* function call. Once the ownership is released, any other driver may
* claim ownership of the interface. The delete function of a class driver
* typically calls this function for all owned interfaces.
*
* INPUTS
* cb    Pointer to the interface control block.
* drvr  Pointer to the control block of the driver that's currently owning
*       the interface.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Release (NU_USB_INTF * cb,
                            NU_USB_DRVR * drvr)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(cb->driver);

    cb->driver = NU_NULL;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_Is_Claimed
*
* DESCRIPTION
*    This function returns NU_FALSE in is_claimed_out, if no driver currently
* owns the interface and NU_TRUE otherwise. If a driver currently owns the
* interface its pointer is returned in drvr_out.
*
* INPUTS
*   cb              Pointer to the interface control block.
*   is_claimed_out      Pointer to a variable to hold the ownership status of the
*                   interface.
*   drvr_out        Pointer to a memory location to hold the pointer to the
*                   driver control block that currently owns the interface.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_Is_Claimed (NU_USB_INTF * cb,
                                   BOOLEAN * is_claimed_out,
                                   NU_USB_DRVR ** drvr_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(is_claimed_out);
    NU_USB_PTRCHK(drvr_out);

    *drvr_out = cb->driver;
    if (cb->driver)
    {
        *is_claimed_out = NU_TRUE;
    }
    else
    {
        *is_claimed_out = NU_FALSE;
    }
    return (NU_SUCCESS);

}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_IAD
*
* DESCRIPTION
*   This function returns the pointer to the interface associate control
* block if existed.
* device, in device_out.
*
* INPUTS
*  cb          Pointer to the interface control block.
*  device_out  Pointer to a memory location to hold the pointer to the control
*              block of the interface associate control block.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*    NU_USB_REJECTED indicates there is no interface associate component
*    for this interface
*************************************************************************/

STATUS NU_USB_INTF_Get_IAD (NU_USB_INTF * cb,
                            NU_USB_IAD ** iad_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(iad_out);

    *iad_out = cb->iad;
    if (*iad_out == NU_NULL)
        return NU_USB_REJECTED;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_Cfg
*
* DESCRIPTION
*    This function returns the pointer to the control block of the associated
* configuration, in cfg_out.
*
* INPUTS
*   cb      Pointer to the interface control block.
*   cfg_out     Pointer to a memory location to hold the pointer to the control
*           block of the associated configuration.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_Cfg (NU_USB_INTF * cb,
                            NU_USB_CFG ** cfg_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(cfg_out);

    *cfg_out = cb->cfg;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Get_Device
*
* DESCRIPTION
*   This function returns the pointer to the control block of the associated
* device, in device_out.
*
* INPUTS
*  cb          Pointer to the interface control block.
*  device_out  Pointer to a memory location to hold the pointer to the control
*              block of the associated device.
*
* OUTPUTS
*    NU_SUCCESS Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_INTF_Get_Device (NU_USB_INTF * cb,
                               NU_USB_DEVICE ** device_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(device_out);

    *device_out = cb->device;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_INTF_Find_Alt_Setting
*
* DESCRIPTION
*    This function finds a matching alternate setting among various alternate
* settings of  the interfaces. match_flag specifies the search criteria
* and the rest of the arguments provide the search keys. The pointer to
* control block of the matching alternate setting is returned in
* alt_settg_out.
*
* INPUTS
*   cb                  Pointer to the interface control block.
*   match_flag          OR of one or more of the following:
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
*   alt_settg           Alternate setting Number of the specified interface,
*                       from which the search should begin.
*   bInterfaceClass         The desired value of the bInterfaceClass in the matching
*                       alternate setting.
*   bInterfaceSubClass  The desired value of the bInterfaceSubClass in the
*                       matching alternate setting. Irrelevant, if match_flag
*                       doesn’t contain USB_MATCH_SUB_CLASS.
*   bInterfaceProtocol  The desired value of the bInterfaceProtocol in the
*                       matching alternate setting. Irrelevant, if match_flag
*                       doesn’t contain USB_MATCH_PROTOCOL.
*   alt_settg_out           Pointer to a memory location to hold the pointer to the
*                       control block of the matching alternate setting.
*
* OUTPUTS
*   NU_SUCCESS        Indicates successful completion of the service.
*   NU_NOT_PRESENT        Indicates that no matching alternate setting could be
*                     found.
*   NU_USB_INVLD_ARG  Indicates that the control block is invalid.
*
*************************************************************************/
STATUS NU_USB_INTF_Find_Alt_Setting (NU_USB_INTF * cb,
                                     UINT32 match_flag,
                                     UINT8 alt_settg,
                                     UINT8 bInterfaceClass,
                                     UINT8 bInterfaceSubClass,
                                     UINT8 bInterfaceProtocol,
                                     NU_USB_ALT_SETTG ** alt_settg_out)
{
    UINT8   j, max_alt_set;
    STATUS  status,internal_sts;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(alt_settg_out);
    NU_USB_PTRCHK(cb->device);

    status = NU_USB_DEVICE_Lock(cb->device);

    if (status != NU_SUCCESS)
        return (status);

    if (!NU_USB_STACK_Is_Valid_Device (cb->device->stack, cb->device))
    {
        internal_sts = NU_USB_DEVICE_Unlock(cb->device);
        NU_UNUSED_PARAM(internal_sts);
        return NU_USB_INVLD_ARG;
    }

    if (match_flag & USB_MATCH_ONLY_ACTIVE_ALT_STTG)
    {
        if (!cb->current)
        {
            internal_sts  = NU_USB_DEVICE_Unlock(cb->device);
            NU_UNUSED_PARAM(internal_sts);
            return NU_USB_INVLD_ARG;
        }
        j = cb->current->desc->bAlternateSetting;
        max_alt_set = j + 1;
    }
    else
    {
        j = alt_settg;
        max_alt_set = NU_USB_MAX_ALT_SETTINGS;
    }

    *alt_settg_out = NU_NULL;
    for (; j < max_alt_set; j++)
    {
        if (cb->alt_settg[j].desc)
        {
            if (bInterfaceClass != cb->alt_settg[j].desc->bInterfaceClass)
                continue;

            if (match_flag & USB_MATCH_SUB_CLASS)
                if (bInterfaceSubClass !=
                    cb->alt_settg[j].desc->bInterfaceSubClass)
                    continue;

            if (match_flag & USB_MATCH_PROTOCOL)
                if (bInterfaceProtocol !=
                    cb->alt_settg[j].desc->bInterfaceProtocol)
                    continue;

            /* Matching Interface found!  */
            *alt_settg_out = &cb->alt_settg[j];

            status = NU_USB_DEVICE_Unlock(cb->device);
            return (status);
        }
    }

    internal_sts = NU_USB_DEVICE_Unlock(cb->device);
    NU_UNUSED_PARAM(internal_sts);
    return NU_NOT_PRESENT;
}

/*************************************************************************/

#endif /* USB_INTF_EXT_C */
/*************************** end of file ********************************/

