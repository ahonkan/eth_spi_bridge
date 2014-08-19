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
*       nu_usbh_ms_user_imp.c
*
*
* COMPONENT
*
*        Nucleus USB Host Mass Storage class User Driver.
*
* DESCRIPTION
*
*       This file contains connection /disconnection callback function
*       implementation.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTION
*
*       _NU_USBH_MS_USER_Connect            Connect callback from class
*                                           driver.
*       _NU_USBH_MS_USER_Disconnect         Disconnect callback.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ==============  Standard Include Files =============================  */

#include "connectivity/nu_usb.h"

/*****************************Global Data***************************/

static UINT32  subclass_index_list[MAX_NUM_SUBCLASSES];
static UINT8  subclass_index = 0;

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBH_MS_USER_Connect
*
* DESCRIPTION
*
*       This function is the called by Mass Storage class driver's
*       connect callback when a Mass storage device with USER subclass
*       is detected on the bus.
*
* INPUTS
*
*       cb                                  Pointer to the user control
*                                           block.
*       class_driver                        Pointer to caller class driver
*                                           control block.
*       handle                              Handle for device connected.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USB_MAX_EXCEEDED                 Indicates user is already
*                                           serving maximum devices it can
*                                           support.
*       NU_USB_NOT_SUPPORTED                The unsupported device is
*                                           attached.
*       NU_USB_NO_RESPONSE                  Indicates the device is not
*                                           ready.
*
**************************************************************************/
STATUS _NU_USBH_MS_USER_Connect (NU_USB_USER  * cb,
                                 NU_USB_DRVR  * class_driver,
                                 VOID         * handle)
{
    STATUS  status = NU_SUCCESS;
    UINT8   rollback;
    UINT8   subclass;
    MS_LUN *logical_unit;
    rollback = 0;
    logical_unit = (MS_LUN*)handle;
    subclass = logical_unit->drive->subclass;
    logical_unit->pad_idx = 0;

    if((NU_USBH_MS_USER_Cb_Pt)->app_callbacks != NU_NULL)
    {
        if(( NU_USBH_MS_USER_Cb_Pt->app_callbacks)->app_connect)
        {
            status = (((NU_USBH_MS_APP_CALLBACKS *)
                             (( NU_USBH_MS_USER_Cb_Pt)->app_callbacks))->
                              app_connect((void*)handle,subclass));
        }
        else
        {
            status = NU_NOT_PRESENT;
        }
    }
    else
    {
        NU_USB_SYS_Register_Device(handle,
                                   NU_USBCOMPH_STORAGE);
    }

    if ( NU_SUCCESS == status )
    {
        /* If _NU_USBH_MS_USER_Connect return error status to
         * Nucleus USB at connect event, Nucleus USB stack doesn't
         * notify disconnect event when the device is detached.
         * So, below functions are called only when NU_USBH_MS_Connect
         * returns NU_SUCCESS.
         */
        status = _NU_USBH_USER_Connect (cb, class_driver, handle);
        if ( status != NU_SUCCESS )
        {
            rollback = 1;
        }

        if ( !rollback)
        {
            status = _NU_USBH_USER_Open_Device((NU_USBH_USER *)cb, handle);
            if ( status != NU_SUCCESS )
            {
                rollback = 2;
            }
        }
    }

    switch(rollback)
    {
        case 2:     _NU_USBH_USER_Disconnect (cb, class_driver, handle);

        default:    break;
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBH_MS_USER_Disconnect
*
* DESCRIPTION
*
*       This function is the called by Mass Storage class driver's
*       disconnect callback when a USER Disk being served by this USER
*       user is disconnected from the bus.
*
* INPUTS
*
*       cb                                  Pointer to the user control
*                                           block.
*       class_driver                        Pointer to caller class driver
*                                           control block.
*       handle                              Handle for device disconnected.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USB_INTERNAL_ERROR               Indicates a internal error in
*                                           disconnect process.
*       NU_NOT_PRESENT                      If control block is deleted
*                                           before call completion.
*
**************************************************************************/
STATUS _NU_USBH_MS_USER_Disconnect ( NU_USB_USER * cb,
                                     NU_USB_DRVR * class_driver,
                                     VOID        * handle)
{
    STATUS status;
    UINT8   subclass;
    MS_LUN *logical_unit;

    logical_unit = (MS_LUN*)handle;
    subclass = logical_unit->drive->subclass;

    if((NU_USBH_MS_USER_Cb_Pt)->app_callbacks != NU_NULL)
    {
        if(( NU_USBH_MS_USER_Cb_Pt->app_callbacks)->app_disconnect)
        {
            status = (((NU_USBH_MS_APP_CALLBACKS *)
                             (( NU_USBH_MS_USER_Cb_Pt)->app_callbacks))->
                              app_disconnect((VOID*)handle,subclass));

        }
        else
        {
            status = NU_NOT_PRESENT;
        }
    }
    else
    {
        status = NU_USB_SYS_DeRegister_Device(handle,
                                              NU_USBCOMPH_STORAGE);
    }

    NU_USB_ASSERT( status == NU_SUCCESS );

    status = _NU_USBH_USER_Close_Device ((NU_USBH_USER *) cb, handle);
    NU_USB_ASSERT( status == NU_SUCCESS );

    status = _NU_USBH_USER_Disconnect (cb, class_driver, handle);
    NU_USB_ASSERT( status == NU_SUCCESS );

    return (status);
}
/**************************************************************************
*
* FUNCTION
*
*       UHMSU_init_Subclass_index
*
* DESCRIPTION
*
*       This function maintains an index list for each of subclass
*       supported by this user driver
*
* INPUTS
*
*       subclass                            subclass code
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_INVALID_ENTRY                    A matching entry already exist
*       NU_NOT_PRESENT                      No space is available for a new
*                                           entry.
*
**************************************************************************/
STATUS UHMSU_init_Subclass_index(UINT8 subclass)
{
    UINT32     index = 0;
    STATUS     status = NU_INVALID_ENTRY;
    UINT32     hash = 0;
    BOOLEAN is_present = NU_FALSE;

    /* Calculat a hash on the basis of subclass. */
    hash = ((0x55 << 16) | (subclass << 8) | (0xAA));

    for(index = 0; index < subclass_index; index++)
    {
        if(subclass_index_list[index] == hash)
        {
            is_present = NU_TRUE;
            break;
        }
    }

    if(!is_present)
    {
        if(subclass_index < MAX_NUM_SUBCLASSES)
        {
            subclass_index_list[subclass_index] = hash;
            subclass_index++;
            status = NU_SUCCESS;
        }
        else
            status = NU_NOT_PRESENT;
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       UHMSU_Get_Subclass_index
*
* DESCRIPTION
*
*       This function retrives the index of the subclass supported by
*       this user driver.
*
* INPUTS
*
*       subclass                            subclass code
*       index_return                        variable to get the  index value
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_NOT_PRESENT                      A requested entry is not present.
*
**************************************************************************/
STATUS UHMSU_Get_Subclass_index(UINT8 subclass, UINT8 *index_return)
{
    UINT32 index = 0;
    STATUS status = NU_NOT_PRESENT;
    UINT32 hash = 0;

    /* Calculat a hash on the basis of subclass. */
    hash = ((0x55 << 16) | (subclass << 8) | (0xAA));

    for(index = 0; index < subclass_index; index++)
    {
        if(subclass_index_list[index] == hash)
        {
            *index_return = index;
            status = NU_SUCCESS;
            break;
        }
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       UHMSU_Clear_Subclass_Index_List
*
* DESCRIPTION
*
*       This function clears the subclass_index and subclass_index_list 
*       
*
* INPUTS
*
*       NONE
*       
* OUTPUTS
*
*       NONE
*       
**************************************************************************/

VOID UHMSU_Clear_Subclass_Index_List(VOID)
{
    memset(subclass_index_list, 0x00, sizeof(subclass_index_list));
    subclass_index=0;
}


/* ======================  End Of File  ================================ */

