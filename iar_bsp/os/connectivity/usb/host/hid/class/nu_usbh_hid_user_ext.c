/**************************************************************************
*
*               Copyright 2004 Mentor Graphics Corporation
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
*       nu_usbh_hid_user_ext.c
*
*
* COMPONENT
*
*       Nucleus USB Host HID Base Class Driver.
*
* DESCRIPTION
*
*       This file contains the implementation of external interfaces
*       exported by Nucleus USB Host HID User Layer.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       _NU_USBH_HID_USER_Create            Initializes an HID user driver.
*       NU_USBH_HID_USER_Notify_Report      Notifies a user driver of HID
*                                           report.
*       NU_USBH_HID_USER_Get_Usages         Get usages supported by an HID
*                                           user.
*       NU_USBH_HID_USER_Get_Num_Usages     Get number of such usages
*                                           supported.
*       _NU_USBH_HID_USER_Delete            Uninitializes an HID user
*                                           driver.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

#include    "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*
*       _NU_USBH_HID_USER_Create
*
* DESCRIPTION
*
*       This function initializes the data structures required by
*       NU_USBH_HID_USER. This is used by extenders of HID user layer
*       to initialize base resources.
*
* INPUTS
*       cb                  pointer to user control block.
*       name                name for this USB object.
*       pool                pointer to memory pool used by USER.
*       num_usages          number of HID usages supported.
*       dispatch            pointer to dispatch table filled by child.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion.
*       NU_INVALID_SEMAPHORE    Indicates control block is invalid.
*       NU_INVALID_GROUP        Indicates control block is invalid.
*
*
*************************************************************************/
STATUS _NU_USBH_HID_USER_Create (NU_USBH_HID_USER   *cb,
                                 CHAR               *name,
                                 NU_MEMORY_POOL     *pool,
                                 UINT8               num_usages,
                                 const VOID         *dispatch)
{
    STATUS status;
    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(name);
    NU_USBH_HID_ASSERT(dispatch);
    NU_USB_MEMPOOLCHK(pool);

    /* Create base. */
    status = _NU_USBH_USER_Create ((NU_USBH_USER *) cb, name,
                                    pool, 0, 0, dispatch);

    if (status == NU_SUCCESS)
    {
        cb->num_usages = num_usages;
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_HID_USER_Notify_Report
*
* DESCRIPTION
*
*       This function notifies the concerned HID user driver of the report
*       sent by the HID device.
*
* INPUTS
*
*       cb          pointer to HID_USER control block.
*       drvr        class driver control block.
*       handle      handle/cookie for the concerned device.
*       report_data Report sent by the device.
*       report_len  Length of the report.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    Indicates some argument became stale before
*                           call gets completed.
*       NU_NOT_PRESENT      Indicates Device doesn't exist.
*
*
*************************************************************************/
STATUS NU_USBH_HID_USER_Notify_Report(NU_USBH_HID_USER  *cb,
                                      NU_USBH_HID       *driver,
                                      VOID              *handle,
                                      UINT8             *report_data,
                                      UINT16             report_len)
{
    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(driver);
    NU_USBH_HID_ASSERT(handle);
    NU_USBH_HID_ASSERT(report_data);
    if (((NU_USBH_HID_USER_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Notify_Report)
    return (((NU_USBH_HID_USER_DISPATCH *)(((NU_USB *) cb)->usb_dispatch))
           ->Notify_Report(cb, driver, handle, report_data, report_len ));
    else
        return (NU_NOT_PRESENT);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_HID_USER_Get_Usages
*
* DESCRIPTION
*
*       This function invokes the concerned HID user driver's Get_Usages
*       function through the dispatch table.
*
* INPUTS
*
*       cb             Pointer to HID_USER control block.
*       drvr           Class driver control block.
*       handle         Handle/cookie for the concerned device.
*       usage_out      Location where the supported usages must be filled.
*       num_usages_out Location where the number of supported usages
*                      must be filled.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    Indicates some argument became stale before
*                           call gets completed.
*       NU_NOT_PRESENT      Indicates Device doesn't exist.
*
*
*************************************************************************/
STATUS NU_USBH_HID_USER_Get_Usages (NU_USBH_HID_USER    *cb,
                                    NU_USBH_HID         *drvr,
                                    NU_USBH_HID_USAGE   *usage_out,
                                    UINT8                num_usages)
{
    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(drvr);
    NU_USBH_HID_ASSERT(usage_out);
    return(((NU_USBH_HID_USER_DISPATCH *)
            (((NU_USB *) cb)->usb_dispatch))->Get_Usages(cb,
                                                         drvr,
                                                         usage_out,
                                                         num_usages));
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_HID_USER_Get_Num_Usages
*
* DESCRIPTION
*
*       This function retrieves the number of usages supported by the
*       the concerned HID user driver. This information is obtained during
*       the HID user driver creation.
*
* INPUTS
*
*       cb              Pointer to HID_USER control block.
*       class_driver    Class driver control block.
*       num_usages_out  Location where the number of supported usages
*                       will be filled.
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates successful completion.
*
*
*************************************************************************/
STATUS  NU_USBH_HID_USER_Get_Num_Usages (
                                        NU_USBH_HID_USER  *cb,
                                        NU_USBH_HID       *class_driver,
                                        UINT8             *num_usages_out)
{
    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(class_driver);
    NU_USBH_HID_ASSERT(num_usages_out);
    *num_usages_out = cb->num_usages;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*       _NU_USBH_HID_USER_Delete
*
* DESCRIPTION
*
*       This function deletes a specified HID user driver.
*
* INPUTS
*
*       cb           Pointer to HID_USER control block.
*
* OUTPUTS
*
*       NU_SUCCESS   User driver deleted successfully.
*
*
*************************************************************************/
STATUS  _NU_USBH_HID_USER_Delete (VOID *cb)
{
    NU_USBH_HID_ASSERT(cb);
    return (NU_SUCCESS);
}

/*************************** end of file ********************************/

