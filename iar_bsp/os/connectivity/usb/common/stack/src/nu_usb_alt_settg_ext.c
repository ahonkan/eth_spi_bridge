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
*        nu_usb_alt_settg_ext.c
*
* COMPONENT
*   USB Base
*
* DESCRIPTION
*   This file contains implementations of NU_USB_ALT_SETTG services.
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*   NU_USB_ALT_SETTG_Get_Is_Active  - returns if the alt setting is active.
*   NU_USB_ALT_SETTG_Set_Active     - sets the alternate setting as active.
*   NU_USB_ALT_SETTG_Get_Class_Desc - returns the pointer to the class
*                                     specific descriptor of the alt setting.
*   NU_USB_ALT_SETTG_Find_Pipe      - Finds a matching pipe as match
*                                     specified match criteria.
*   NU_USB_ALT_SETTG_Get_Desc       - returns a pointer to the interface
*                                     descriptor corresponding to this alt
*                                     setting.
*   NU_USB_ALT_SETTG_Get_bAlternateSetting - returns alt setting number.
*   NU_USB_ALT_SETTG_Get_Class             - returns interface class of the
*                                            alt setting.
*   NU_USB_ALT_SETTG_Get_SubClass          - returns interface sub class of
*                                            the alt setting.
*   NU_USB_ALT_SETTG_Get_Protocol          - returns the interface protocol
*                                            of the alt setting.
*   NU_USB_ALT_SETTG_Get_String            - returns a copy of the
*                                            interface string.
*   NU_USB_ALT_SETTG_Get_String_Num        - returns string index of the
*                                            interface string of the alt
*                                            setting.
*   NU_USB_ALT_SETTG_Get_String_Desc       - returns pointer to interface
*                                            string descriptor.
*   NU_USB_ALT_SETTG_Get_Num_Endps         - returns the number of
*                                            endpoints in the alt setting.
*   NU_USB_ALT_SETTG_Get_Endp              - returns pointer to endpoint
*
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
**************************************************************************/
#ifndef USB_ALT_SETTG_EXT_C
#define USB_ALT_SETTG_EXT_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/**************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Get_Desc
*
* DESCRIPTION
*   This function returns pointer to the associated interface descriptor
* in intf_desc_out.
*
*
* INPUTS
*   cb             pointer to the alternate setting control block.
*   intf_desc_out  pointer to memory location to hold pointer to the
*                  interface descriptor.
*
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Get_Desc (NU_USB_ALT_SETTG *  cb,
                                  NU_USB_INTF_DESC ** intf_desc_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(intf_desc_out);

    *intf_desc_out = cb->desc;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Get_Is_Active
*
* DESCRIPTION
*
* This function returns NU_TRUE in is_active_out, if it’s the currently
* active alternate setting on the interface, else is_active_out would
* contain NU_FALSE on return.
*
*
* INPUTS
*  cb           Pointer to the alternate setting control block.
* is_active_out Pointer to the variable to hold active state of the
*               alternate setting.
*
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Get_Is_Active (NU_USB_ALT_SETTG * cb,
                                       BOOLEAN * is_active_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(is_active_out);

    *is_active_out = cb->is_active;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Set_Active
*
* DESCRIPTION
*
* This function makes the alternate setting as the active alternate setting
* of the interface. The resources necessary in the controller h/w are
* allocated and the required bandwidth on the bus is reserved by this
* function. This function should be used only by class drivers on Host
* side and should not be used by class drivers on the function side. Its
* host’s prerogative to select and activate an alternate setting on an
* interface.
*
*
* INPUTS
*   cb              Pointer to the alternate setting control block.
*   alt_setting_num     bAlternateSettingNum of the alternate setting that is
*                   to be made active.
*
*
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion of the service.
*   NU_USB_NO_BANDWIDTH Indicates that the device's b/w requirements of the
*                       specified configuration cannot be met.
*  NU_INVALID_SEMAPHORE Indicates that one of the internal semaphore pointer
*                       is invalid.
*  NU_SEMAPHORE_DELETED Indicates that one of the internal semaphores was
*                       deleted.
*  NU_UNAVAILABLE           Indicates that one of the internal semaphores is
*                       unavailable.
*  NU_INVALID_SUSPEND   Indicates that this API is called from a non-task
*                       thread.
*  NU_USB_INVLD_ARG         Indicates that one or more arguments passed to this
*                       function are invalid.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Set_Active (NU_USB_ALT_SETTG * cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->intf);
    NU_USB_PTRCHK_RETURN(cb->intf->device);
    NU_USB_PTRCHK_RETURN(cb->desc);

    status =  NU_USB_STACK_Set_Intf (cb->intf->device->stack,
                                  cb->intf->device,
                                  cb->intf->intf_num,
                                  cb->desc->bAlternateSetting);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Get_bAlternateSetting
*
* DESCRIPTION
*    This function returns bAlternateSetting field of the associated interface
* descriptor, in bAlternateSetting_out.
*
*
* INPUTS
*   cb                     Pointer to the alternate setting control block.
*   bAlternateSetting_out  Pointer to variable to hold the bAlternateSetting
*                          field of the associated interface descriptor.
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Get_bAlternateSetting (NU_USB_ALT_SETTG * cb,
                                               UINT8 *bAlternateSetting_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->desc);
    NU_USB_PTRCHK_RETURN(bAlternateSetting_out);

    *bAlternateSetting_out = cb->desc->bAlternateSetting;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Get_Class
*
* DESCRIPTION
*    This function returns bInterfaceClass field of the associated interface
* descriptor, in bInterfaceClass_out.
*
*
* INPUTS
*   cb                     Pointer to the alternate setting control block.
*   bInterfaceClass_out    Pointer to variable to hold the bInterfaceClass
*                          field of the associated interface descriptor.
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Get_Class (NU_USB_ALT_SETTG * cb,
                                   UINT8 *bInterfaceClass_out)
{

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->desc);
    NU_USB_PTRCHK_RETURN(bInterfaceClass_out);

    *bInterfaceClass_out = cb->desc->bInterfaceClass;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Get_SubClass
*
* DESCRIPTION
*    This function returns bInterfaceSubClass field of the associated
* interface descriptor, in bInterfaceSubClass_out.
*
*
* INPUTS
*   cb                     Pointer to the alternate setting control block.
*   bInterfaceSubClass_out Pointer to variable to hold the bInterfaceSubClass
*                          field of the associated interface descriptor.
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Get_SubClass (NU_USB_ALT_SETTG * cb,
                                      UINT8 *bInterfaceSubClass_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->desc);
    NU_USB_PTRCHK_RETURN(bInterfaceSubClass_out);

    *bInterfaceSubClass_out = cb->desc->bInterfaceSubClass;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Get_Protocol
*
* DESCRIPTION
*    This function returns bInterfaceProtocol field of the associated
* interface descriptor, in bInterfaceProtocol_out.
*
* INPUTS
*   cb                     Pointer to the alternate setting control block.
*   bInterfaceProtocol_out Pointer to variable to hold the
*                          bInterfaceProtocol field of the associated
*                          interface descriptor.
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Get_Protocol (NU_USB_ALT_SETTG * cb,
                                      UINT8 *bInterfaceProtocol_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->desc);
    NU_USB_PTRCHK_RETURN(bInterfaceProtocol_out);

    *bInterfaceProtocol_out = cb->desc->bInterfaceProtocol;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Get_Class_Desc
*
* DESCRIPTION
*   This function returns pointer to the class specific interface descriptor
*   (in class_desc_out) and its length in length_out.
*
*
* INPUTS
*   cb                     Pointer to the alternate setting control block.
*   class_desc_out         Pointer to a memory location to hold the pointer
*                          to class specific interface descriptor.
*   length_out             Pointer to the variable to hold the length of
*                          the descriptor in bytes.
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Get_Class_Desc (NU_USB_ALT_SETTG * cb,
                                        UINT8 **class_desc_out,
                                        UINT32 *length_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(class_desc_out);
    NU_USB_PTRCHK_RETURN(length_out);

    *class_desc_out = cb->class_specific;
    *length_out = cb->length;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Get_String
*
* DESCRIPTION
*    This function copies the interface string of the alternate setting,
* in to string_out. English is assumed to be the Unicode of the configuration
* string descriptor. If the Unicode is not English, refer to
* NU_USB_INTF_Get_String_Desc to know how to retrieve a displayable string.
*
*
* INPUTS
*   cb          Pointer to the alternate setting control block.
*   string_out  pointer to the user supplied array of characters of size
*               NU_USB_MAX_STRING_LEN.
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion of the service.
*   NU_NOT_PRESENT      Indicates that the interface string number cannot be
*                       found in the device’s set of string descriptors.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Get_String (NU_USB_ALT_SETTG * cb,
                                    CHAR * string_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->intf);
    NU_USB_PTRCHK_RETURN(cb->intf->device);
    NU_USB_PTRCHK_RETURN(cb->desc);
    NU_USB_PTRCHK_RETURN(string_out);

    status = NU_USB_DEVICE_Get_String (cb->intf->device,
                                       cb->desc->iInterface,
                                       string_out);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Get_String_Num
*
* DESCRIPTION
*   This function returns iInterface field of the interface descriptor,
* in string_num_out. If string_num_out is 0, it means that there is no
* interface string descriptor associated with this alternate setting.
*
*
* INPUTS
*   cb              Pointer to the alternate setting control block.
*   string_num_out      pointer to the variable to hold iInterface field of the
*                   interface descriptor.
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Get_String_Num (NU_USB_ALT_SETTG * cb,
                                        UINT8 *string_num_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->desc);
    NU_USB_PTRCHK_RETURN(string_num_out);

    *string_num_out = cb->desc->iInterface;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Get_String_Desc
*
* DESCRIPTION
*     This function gets pointer to the string descriptor (in to
* string_desc_out) for the interface string number (iInterface field of
* interface descriptor) . If the Unicode used for string descriptors is English,
* NU_USB_INTF_Get_ String can be used to directly get the ASCII string. For
* other Unicodes NU_USB_INTF_Get_String_Desc function would help retrieve the
* descriptor and then reader may write routines to convert from their Unicode
* format to ASCII string or such other display string notations.
*
* INPUTS
*   cb              Pointer to the alternate setting control block.
*   string_desc_out     pointer to memory location to hold pointer to string
*                   descriptor structure.
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion of the service.
*   NU_NOT_PRESENT          Indicates that the interface string number cannot be
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Get_String_Desc (NU_USB_ALT_SETTG * cb,
                                         UINT16 wLangId,
                                         NU_USB_STRING_DESC * string_desc_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->desc);
    NU_USB_PTRCHK_RETURN(string_desc_out);
    NU_USB_PTRCHK_RETURN(cb->intf);
    NU_USB_PTRCHK_RETURN(cb->intf->device);

    status = NU_USB_DEVICE_Get_String_Desc (cb->intf->device,
                                          cb->desc->iInterface,
                                          wLangId,
                                          string_desc_out);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Get_Num_Endps
*
* DESCRIPTION
*    This function returns bNumEndpoints field of the associated
* interface descriptor, in number_endps_out.
*
* INPUTS
*   cb                     Pointer to the alternate setting control block.
*   number_endps_out       Pointer to variable to hold the
*                          bNumEndpoints field of the associated
*                          interface descriptor.
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Get_Num_Endps (NU_USB_ALT_SETTG * cb,
                                       UINT8 *number_endps_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->desc);
    NU_USB_PTRCHK_RETURN(number_endps_out);

    *number_endps_out = cb->desc->bNumEndpoints;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ALT_SETTG_Find_Pipe
*
* DESCRIPTION
*    This function finds a matching among those contained in the alternate
* setting. match_flag specifies the search criteria and other arguments
* provide the search keys.
*
* INPUTS
*   cb          Pointer to the alternate setting control block.
*   match_flag  OR of one or more of the following:
*               USB_MATCH_EP_ADDRESS- Search for pipe with endpoint number
*               specified in endp_num.
*               USB_MATCH_EP_TYPE - Search for pipe whose endpoint type is
*               same as that specified in type.
*               USB_MATCH_EP_DIRECTION-Search for pipe whose endpoint
*               direction is same as that specified in type.
*
*
*   endp_num    Endpoint number in the range of 0 to 15.
*   direction   Either USB_DIR_IN or USB_DIR_OUT.
*   type        One of - USB_EP_CTRL, USB_EP_ISO, USB_EP_BULK, USB_EP_INTR
*   pipe_out    Pointer to memory location to hold the pointer to the matching
*               pipe.
*
* OUTPUTS
*   NU_SUCCESS        Indicates successful completion of the service.
*   NU_NOT_PRESENT        Indicates that no matching pipe could be found.
*   NU_USB_INVLD_ARG  Indicates that the control block is invalid.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Find_Pipe (NU_USB_ALT_SETTG * cb,
                                   UINT32 match_flag,
                                   UINT8 endp_num,
                                   UINT8 direction,
                                   UINT8 type,
                                   NU_USB_PIPE ** pipe_out)
{
    UINT8   bEndpointAddress, j;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cb->intf);
    NU_USB_PTRCHK_RETURN(cb->intf->device);
    NU_USB_PTRCHK_RETURN(cb->intf->device->stack);
    NU_USB_PTRCHK_RETURN(pipe_out);

    if (!NU_USB_STACK_Is_Valid_Device (cb->intf->device->stack,
                                       cb->intf->device))
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return NU_USB_INVLD_ARG;
    }

    bEndpointAddress = endp_num | direction;

    /* Search for a reference endpoint, if any      */
    /* This endpoint is given to us in 'address'    */
    j = 0;

    if ((endp_num == 0) && (match_flag & USB_MATCH_EP_ADDRESS))
    {
        /* return the default pipe */
        *pipe_out = &(cb->intf->device->ctrl_pipe);

        /* Switch back to user mode. */
        NU_USER_MODE();
        return (NU_SUCCESS);
    }

    if (endp_num != 0)
    {
        /* First find the 'address' */
        for (j = 0; j < cb->desc->bNumEndpoints; j++)
            if (bEndpointAddress == cb->endp[j].desc->bEndpointAddress)
                break;

        if (j == cb->desc->bNumEndpoints)   /* Reference endpoint not found  */
        {
            /* Switch back to user mode. */
            NU_USER_MODE();
            return (NU_USB_INVLD_DESC);
        }

        /* If the user wants only this, return  */
        if (match_flag == USB_MATCH_EP_ADDRESS)
        {
            *pipe_out = &cb->endp[j].pipe;
            /* Switch back to user mode. */
            NU_USER_MODE();
            return (NU_SUCCESS);
        }
    }
    else
    {
        /* No reference endpoint given  */
        if (match_flag == USB_MATCH_EP_ADDRESS)
        {
            /* Switch back to user mode. */
            NU_USER_MODE();
            return NU_USB_INVLD_ARG;
        }
    }

    for (; j < cb->desc->bNumEndpoints; j++)
    {
        if (match_flag & USB_MATCH_EP_TYPE)
            if (type != (cb->endp[j].desc->bmAttributes & 0x03))
                continue;

        if (match_flag & USB_MATCH_EP_DIRECTION)
            if (direction != (cb->endp[j].desc->bEndpointAddress & 0x80))
                continue;

        /* Endpoint found!  */
        *pipe_out = &cb->endp[j].pipe;

        /* Switch back to user mode. */
        NU_USER_MODE();

        return (NU_SUCCESS);
    }

    /* No endpoint with corresponding matching criteria found  */

    /* Switch back to user mode. */
    NU_USER_MODE();

    return NU_NOT_PRESENT;
}

/*************************************************************************
* FUNCTION
*     NU_USB_ALT_SETTG_Get_Endp
*
* DESCRIPTION
*     This function returns the End Point associated with the alternate
*     setting, with given end point number.
*
* INPUTS
*     cb              Pointer to the alternate setting control block.
*     number_endp     Endpoint number to be retrieved
*     endp_out        Pointer to the memory location to hold the pointer to
*                     the end point associated given alternate setting and
*                     endpoint number
*
* OUTPUTS
*     NU_SUCCESS      Indicates successful completion of the service.
*     NU_NOT_PRESENT  Indicates that the required end point is not available.
*
*************************************************************************/
STATUS NU_USB_ALT_SETTG_Get_Endp (NU_USB_ALT_SETTG * cb,
                                  UINT8 number_endp,
                                  NU_USB_ENDP ** endp_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(endp_out);

    if (number_endp < NU_USB_MAX_ENDPOINTS)
    {
        *endp_out = &(cb->endp[number_endp]);
        status = NU_SUCCESS;
    }
    else
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************/

#endif /* USB_ALT_SETTG_EXT_C */
/*************************** end of file ********************************/
