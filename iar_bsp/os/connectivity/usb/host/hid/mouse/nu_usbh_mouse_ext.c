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
*       nu_usbh_mouse_ext.c
*
*
* COMPONENT
*
*       Nucleus USB Host Mouse Driver.
*
* DESCRIPTION
*
*       This file contains the implementation of external interfaces
*       exported by Nucleus USB Host Mouse driver.
*
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*       NU_USBH_MOUSE_Create                Creates an instance of Mouse
*                                           driver.
*       _NU_USBH_MOUSE_Notify_Report        Handles Report generated by
*                                           Mouse.
*       _NU_USBH_MOUSE_Get_Usages           Reports the usages supported
*                                           by the mouse driver.
*       NU_USBH_MOUSE_Reg_Event_Handler     Notes down the handler for
*                                           the key up/down events.
*       NU_USBH_MOUSE_Reg_State_Handlers    Registers callback functions
*                                           for application notification.
*       _NU_USBH_MOUSE_Connect              Handles a newly connected
*                                           mouse event.
*       NU_USBH_MOUSE_Get_Info              Returns Information about
*                                           Mouse.
*       _NU_USBH_MOUSE_Disconnect           Handles disconnection event
*                                           for the mouse.
*       _NU_USBH_MOUSE_Delete               Deletes the instance of the
*                                           mouse driver.
*       NU_USBH_MOUSE_Init_GetHandle        This function is used to
*                                           retrieve the mouse user
*                                           driver address.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

#include    "connectivity/nu_usb.h"
#include    "services/runlevel_init.h"

/*************************************************************************
*   FUNCTION
*
*       nu_os_conn_usb_host_hid_mouse_init
*
*   DESCRIPTION
*
*       This function initializes the HID MSE driver component.
*
*   INPUTS
*
*       path                                Registry path of component.
*       startstop                           Flag to find if component is
*                                           being enabled or disabled.
*
*   OUTPUTS
*
*       status             Success or failure of the creation of the
*                          underlying initialization routines.
*
*************************************************************************/
STATUS nu_os_conn_usb_host_hid_mouse_init(CHAR *path, INT compctrl)
{
    STATUS  status = NU_SUCCESS,internal_sts = NU_SUCCESS;
    VOID *usbh_hid_handle;
    UINT8 rollback = 0;

    if(compctrl == RUNLEVEL_START)
    {
        /* Allocate memory for mouse user driver control block. */
        status = USB_Allocate_Object(sizeof(NU_USBH_MOUSE),
                                     (VOID **)&NU_USBH_MOUSE_Cb_Pt);

        /* Create the device subsystem. */
        if (status == NU_SUCCESS)
        {
            /* Zero out allocated block. */
            memset(NU_USBH_MOUSE_Cb_Pt, 0, sizeof(NU_USBH_MOUSE));

            status = NU_USBH_MOUSE_Create (NU_USBH_MOUSE_Cb_Pt,
                                           "USBH_MSE",
                                           NU_NULL);

            /*  Get the host HID class driver handle. */
            if (status == NU_SUCCESS)
            {
                status = NU_USBH_HID_Init_GetHandle(&usbh_hid_handle);

                /* Register the user driver to the class driver. */
                if (status == NU_SUCCESS)
                {
                   status = NU_USB_DRVR_Register_User ( (NU_USB_DRVR *) usbh_hid_handle,
                                                (NU_USB_USER *)
                                                 NU_USBH_MOUSE_Cb_Pt);
                   if ( status != NU_SUCCESS )
                   {
                       rollback = 3;
                   }
                }
                else
                {
                    rollback = 3;
                }
            }
            else
            {
                rollback = 2;
            }
        }
        else
        {
        	rollback=1;
        }
    }
    else if(compctrl== RUNLEVEL_STOP)
    {
        internal_sts |= NU_USBH_HID_Init_GetHandle(&usbh_hid_handle);
        internal_sts |= NU_USB_DRVR_Deregister_User( (NU_USB_DRVR *) usbh_hid_handle,
                                     (NU_USB_USER *)NU_USBH_MOUSE_Cb_Pt);
        rollback = 2;
    }

    switch( rollback )
    {
        case 3:
            internal_sts |= _NU_USBH_MOUSE_Delete((void*)NU_USBH_MOUSE_Cb_Pt);

        case 2:
            internal_sts |= USB_Deallocate_Memory(NU_USBH_MOUSE_Cb_Pt);
            NU_USBH_MOUSE_Cb_Pt = NU_NULL;
          
        case 1:
            
        default:
            NU_UNUSED_PARAM(internal_sts);
    }

    return (status);
}
/*************************************************************************
* FUNCTION
*
*       NU_USBH_MOUSE_Create
*
* DESCRIPTION
*
*       This function initializes the data structures required by
*       NU_USBH_HID_USER. This is used by extenders of user layer
*       to initialize base resources.
*
* INPUTS
*       cb                  pointer to mouse driver control block.
*       name                name for this USB object.
*       pool                pointer to memory pool for use by mouse
*                           driver.
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion.
*       NU_INVALID_SEMAPHORE    Indicates control block is invalid.
*
*
*************************************************************************/
STATUS NU_USBH_MOUSE_Create (NU_USBH_MOUSE  *cb,
                             CHAR           *name,
                             NU_MEMORY_POOL *pool)
{
    STATUS status = NU_INVALID_SEMAPHORE;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    if(cb != NU_NULL)
    {
        cb->usages[0].usage_page = 0x09;        /* Button. */
        cb->usages[0].usage_id = 0xFFFF;
        cb->usages[1].usage_page = 1;           /* Generic Desktop.
                                                 */
        cb->usages[1].usage_id = 0x30;          /* X.
                                                 */
        cb->usages[2].usage_page = 1;           /* Generic Desktop.
                                                 */
        cb->usages[2].usage_id = 0x31;          /* Y.
                                                 */
        cb->usages[3].usage_page = 1;           /* Generic Desktop.
                                                 */
        cb->usages[3].usage_id = 0x38;          /* Wheel.
                                                 */
        cb->mouse_list_head = NU_NULL;

        /* Create base. */
        status = _NU_USBH_HID_USER_Create ((NU_USBH_HID_USER *) cb,
                         name, pool, 4, &USBH_Mouse_Dispatch);
    }

    /* Return to user mode */
    NU_USER_MODE();
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBH_MOUSE_Notify_Report
*
* DESCRIPTION
*
*       This function handles the report sent by a mouse.
*
* INPUTS
*
*       cb          pointer to USER control block.
*       drvr        class driver control block.
*       handle      handle/cookie for the concerned device.
*       report_data Report sent by the device.
*       report_len  Length of the report.
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful completion.
*       NU_NOT_PRESENT                  Indicates Device doesn't exist.
*       NU_INVALID_POINTER              Invalid input parameter.
*
*
*************************************************************************/
STATUS _NU_USBH_MOUSE_Notify_Report (NU_USBH_HID_USER   *cb,
                                     NU_USBH_HID        *driver,
                                     VOID               *handle,
                                     UINT8              *report_data,
                                     UINT16              report_len)
{
    NU_USBH_MOUSE *mousecb = (NU_USBH_MOUSE *) cb;
    NU_USBH_MOUSE_DEVICE *mouse;
    UINT8 report_id = 0;

    mouse = USBH_Mouse_Find_Device (mousecb, handle);
    if (mouse == NU_NULL)
        return (NU_NOT_PRESENT);

    if (mousecb->ev_handler == NU_NULL)
            return NU_INVALID_POINTER;

    /* Do the device items support report id. */
    if (mouse->mouse_items[0].report_id)
    {
        if (report_data[0] == 0)
            /* Report in invalid format. */
            NU_ASSERT(0);
        report_id = report_data[0];
        report_data++;
    }

    /* Offset is in bits so we need to add 7 to make it aligned on the
     * byte boundary.
     */
    if (mouse->report_info[0].rep_id== report_id)
        mouse->event.x = report_data[(mouse->report_info[0].offset+7)/8];
    if (mouse->report_info[1].rep_id== report_id)
        mouse->event.y = report_data[(mouse->report_info[1].offset+7)/8];
    if ((mouse->wheel_present) &&
        (mouse->report_info[2].rep_id== report_id))
        mouse->event.wheel = report_data
        [(mouse->report_info[2].offset+7)/8];
    if (mouse->report_info[3].rep_id== report_id)
        mouse->event.buttons = report_data
        [(mouse->report_info[3].offset+7)/8];

    /* Call Event Handler.  */
    mousecb->ev_handler (mousecb, mouse, &mouse->event);
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBH_MOUSE_Get_Usages
*
* DESCRIPTION
*       Reports the usage pages supported by the mouse driver.
*
* INPUTS
*
*       cb          Pointer to Mouse driver control block.
*       drvr        Pointer to HID class driver control block.
*       handle      Handle/cookie for the concerned device.
*       usage_out   Pointer to memory location where the supported usages
*                   must be filled.
*       num_usages  Max number of usages that may be filled at
*                   'usage_out'.
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS _NU_USBH_MOUSE_Get_Usages (NU_USBH_HID_USER  *cb,
                                  NU_USBH_HID       *drvr,
                                  NU_USBH_HID_USAGE *usage_out,
                                  UINT8              num_usages)
{
    UINT8   num;
    UINT8   i;

    if (cb->num_usages > num_usages)
        num = num_usages;
    else
        num = cb->num_usages;

    for (i = 0; i < num; i++)
        usage_out[i] = ((NU_USBH_MOUSE *) cb)->usages[i];

  return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_MOUSE_Reg_Event_Handler
*
* DESCRIPTION
*
*       Registers the function pointer that has to be invoked upon
*       detecting a mouse event.
*
* INPUTS
*
*       cb          Pointer to Mouse driver control block.
*       func        Pointer to the event handler.
*
* OUTPUTS
*
*       NU_SUCCESS            Indicates successful completion.
*
*
*************************************************************************/
STATUS NU_USBH_MOUSE_Reg_Event_Handler (NU_USBH_MOUSE               *cb,
                                        NU_USBH_MOUSE_Event_Handler  func)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    cb->ev_handler = func;

    /* Return to user mode */
    NU_USER_MODE();
    return (NU_SUCCESS);
}

/**************************************************************************
* FUNCTION
*
*   NU_USBH_MOUSE_Reg_State_Handlers
*
* DESCRIPTION
*   Registers the function pointers for the connection and disconnection
*   callback routines implemented in application.
*
* INPUTS
*
*   drvr_cb     Pointer to mouse driver control block.
*   context     Pointer which will be passed as context in the callback.
*   call_back   Pointer to structure containing the pointers to callback
*               routines.
*
* OUTPUTS
*
*   NU_SUCCESS            Indicates successful completion.
*   NU_INVALID_POINTER    Some invalid parameter is passed.
*
**************************************************************************/
STATUS NU_USBH_MOUSE_Reg_State_Handlers(NU_USBH_MOUSE*           drvr_cb,
                                        VOID*                    context,
                                        NU_USBH_MOUSE_CALLBACK*  call_back)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    if((!drvr_cb)||(!call_back))
    {
        status = NU_INVALID_POINTER;
    }
    else
    {
        drvr_cb->call_back = call_back;
        drvr_cb->context   = context;
        status = NU_SUCCESS;

    }

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBH_MOUSE_Connect
*
* DESCRIPTION
*       This function creates an instance of an internal data structure to
*       manage the newly connected mouse and does some initial settings
*       (LEDs, Idle rate) of the mouse.
*
* INPUTS
*       cb              Pointer to the mouse driver control block.
*       class_driver    Pointer to  HID class driver's control block.
*       handle          Handle for the device connected.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_MAX_EXCEEDED indicates user is already serving maximum
*                           devices it can support.
*       NU_INVALID_GROUP    Indicates invalid control block.
*
*
*************************************************************************/
STATUS _NU_USBH_MOUSE_Connect (NU_USB_USER  *cb,
                               NU_USB_DRVR  *class_driver,
                               VOID         *handle)
{
    STATUS status;
    NU_USBH_MOUSE   *mousecb = (NU_USBH_MOUSE *)cb;
    NU_USBH_MOUSE_DEVICE *mouse = NU_NULL;
    UINT8 i, j, rollback = 0;
    BOOLEAN button_offset = NU_FALSE;

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(class_driver);
    NU_USBH_HID_ASSERT(handle);

    do
    {
        /* Allocate and initialize a new instance of NU_USBH_MOUSE_DEVICE.
         */
        status = USB_Allocate_Object (sizeof (NU_USBH_MOUSE_DEVICE),
                                      (VOID **)&mouse);
        if (status != NU_SUCCESS)
        {
            break;
        }

        memset(mouse, 0, sizeof (NU_USBH_MOUSE_DEVICE));

        /* Store the handle. */
        mouse->handle = handle;

        /* Get mouse items. */
        status = NU_USBH_HID_Get_Items(
                            (NU_USBH_HID *)class_driver,
                            (NU_USBH_HID_USER*)cb, handle,
                            mouse->mouse_items,
                            4,
                            &mouse->num_items);
        if (status != NU_SUCCESS)
        {
            rollback = 1;
            break;
        }

        for(i = 0; (i < mouse->num_items) && (i < MAX_USBH_HID_MOUSE_ITEMS); i++)
        {
            if (mouse->mouse_items[i].report_size  > 8)
            {
                /* All reports are assumed to be 8 bit long, if not discard
                 * it.
                 */
                status = NU_USB_INVLD_DESC;
                break;
            }
            if (mouse->mouse_items[i].num_usage_ids == 0)
            {
                /* Read usage min-max to get to know the values. */
                mouse->mouse_items[i].num_usage_ids =
                mouse->mouse_items[i].usage_max -
                mouse->mouse_items[i].usage_min +1;

                if (mouse->mouse_items[i].num_usage_ids >
                   NU_USB_MAX_HID_USAGES - 1)
                   mouse->mouse_items[i].num_usage_ids
                   = NU_USB_MAX_HID_USAGES - 1;

                for(j = 0;j < mouse->mouse_items[i].num_usage_ids;j++)
                {
                    mouse->mouse_items[i].usage_ids[j] = j +
                                            mouse->mouse_items[i].usage_min;
                }
            }

            for(j = 0;j<mouse->mouse_items[i].num_usage_ids;j++)
            {
                switch (mouse->mouse_items[i].usage_ids[j])
                {
                    case 0x30:                  /* X. */
                        mouse->report_info[0].rep_id
                        = mouse->mouse_items[i].report_id;
                        mouse->report_info[0].offset
                        = mouse->mouse_items[i].report_offset + j*8;
                        break;
                    case 0x31:                  /* Y. */
                        mouse->report_info[1].rep_id
                        = mouse->mouse_items[i].report_id;
                        mouse->report_info[1].offset
                        = mouse->mouse_items[i].report_offset + j*8;

                        break;
                    case 0x38:                  /* Wheel. */
                        mouse->report_info[2].rep_id
                        = mouse->mouse_items[i].report_id;
                        mouse->report_info[2].offset
                        = mouse->mouse_items[i].report_offset + j*8;
                        mouse->wheel_present = 1;
                        break;
                    default :                   /* Button. */
                        if ((mouse->mouse_items[i].usage_page == 9) &&
                            (!button_offset))
                        {
                            button_offset = NU_TRUE;
                            mouse->report_info[3].rep_id
                            = mouse->mouse_items[i].report_id;
                            mouse->report_info[3].offset
                            = mouse->mouse_items[i].report_offset + j*8;
                        }
                        break;
                }
            }
        }

        if ( status != NU_SUCCESS )
        {
            rollback = 1;
            break;
        }

        /* Place Device Structure on the list. */
        NU_Place_On_List ((CS_NODE **) & mousecb->mouse_list_head,
                           (CS_NODE *) mouse);

        /* Calling the base behavior. */
        status = _NU_USBH_USER_Connect (cb, class_driver, handle);
        if (status != NU_SUCCESS)
        {
            rollback = 2;
            break;
        }

        if(mousecb->call_back != NU_NULL)
        {
            status = mousecb->call_back->Connection(mousecb->context,
                                                mouse);
            if ( status != NU_SUCCESS )
            {
                rollback = 3;
            }
        }
        else
        {
            status =  NU_USB_SYS_Register_Device((void *)mouse,
                                         NU_USBCOMPH_MSE);
            if ( status != NU_SUCCESS )
            {
                rollback = 3;
            }
        }
    }while(0);

    switch(rollback)
    {
        case 3: _NU_USBH_USER_Disconnect(cb, class_driver, handle);
        case 2: NU_Remove_From_List (  (CS_NODE**)&mousecb->mouse_list_head,
                                        (CS_NODE*)mouse);
        case 1: USB_Deallocate_Memory(mouse);
        default: break;
    }
    return (status);
}

/************************************************************************
*
* FUNCTION
*
*       _NU_USBH_MOUSE_Disconnect
*
* DESCRIPTION
*       This function deletes the instance that corresponds to the mouse
*       thats disconnected and releases all the associated resources.
*
* INPUTS
*       cb              Pointer to the mouse driver control block.
*       class_driver    Pointer to HID class driver's control block.
*       handle          Handle for the device disconnected.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*       NU_NOT_PRESENT  Indicates that the handle cannot be located.
*
*
************************************************************************/
STATUS _NU_USBH_MOUSE_Disconnect (  NU_USB_USER *cb,
                                    NU_USB_DRVR *class_driver,
                                    VOID        *handle)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_MOUSE   *mousecb = (NU_USBH_MOUSE *)cb;
    NU_USBH_MOUSE_DEVICE *mouse;

    /* Call the base behavior. */
    status = _NU_USBH_USER_Disconnect (cb, class_driver, handle);

    mouse = USBH_Mouse_Find_Device((NU_USBH_MOUSE *)cb, handle);
    if(mouse == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }
    else
    {
        if(mousecb->call_back != NU_NULL)
        {
            status |= mousecb->call_back->Disconnection(mousecb->context,
                                                       mouse);
        }
        else
        {
            status |= NU_USB_SYS_DeRegister_Device( mouse, NU_USBCOMPH_MSE );
        }

        /* Remove Device Structure on the list. */
        NU_Remove_From_List ((CS_NODE **) & mousecb->mouse_list_head,
                             (CS_NODE *) mouse);

        status |= USB_Deallocate_Memory(mouse);
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_MOUSE_Get_Info
*
* DESCRIPTION
*       This function  returns information about a mouse like number of
*       buttons and if wheel is present of not.
*
* INPUTS
*       cb                    Pointer to the mouse driver control block.
*       handle                Handle for the device disconnected.
*       num_buttons_out       Pointer to location in which number of
*                             buttons is returned.
*       is_wheel_present_out  Pointer to location in which tells if
*                             wheel is present or not.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion.
*       NU_NOT_PRESENT          Indicates that mouse is not present.
*
*************************************************************************/
STATUS NU_USBH_MOUSE_Get_Info(NU_USBH_MOUSE *cb,
                              VOID          *handle,
                              UINT8         *num_buttons_out,
                              UINT8         *is_wheel_present_out)
{
    NU_USBH_MOUSE_DEVICE *mouse;
    UINT8 i;
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    mouse = USBH_Mouse_Find_Device((NU_USBH_MOUSE *)cb, handle);

    if(mouse == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }
    else
    {
        for(i = 0; i<mouse->num_items; i++)
        {
            /* Button. */
            if (mouse->mouse_items[i].usage_page == 9)
            {
                *num_buttons_out = mouse->mouse_items[i].report_count;
                break;
            }
        }
        *is_wheel_present_out = mouse->wheel_present;
        status = NU_SUCCESS;
    }
    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBH_MOUSE_Delete
*
* DESCRIPTION
*
*       This function deletes the instance of the mouse driver and
*       releases all the resources associated with the mouse devices
*       managed by this driver.
*
* INPUTS
*
*       cb          Pointer to the mouse driver control block.
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates successful completion.
*
*
*************************************************************************/
STATUS _NU_USBH_MOUSE_Delete (VOID *cb)
{
    NU_USBH_MOUSE   *mousecb = (NU_USBH_MOUSE *)cb;
    NU_USBH_MOUSE_DEVICE *next;
    NU_USBH_MOUSE_DEVICE *mouse = mousecb->mouse_list_head;

    while (mouse)
    {
        next = (NU_USBH_MOUSE_DEVICE *) mouse->node.cs_next;

        /* Remove Device Structure on the list. */
        NU_Remove_From_List ((CS_NODE **) & mousecb->mouse_list_head,
                             (CS_NODE *) mouse);

        USB_Deallocate_Memory(mouse);
        if ((next == mousecb->mouse_list_head) || (next == NU_NULL))
            mouse = NU_NULL;
        else
            mouse = next;
    }
    return (_NU_USBH_USER_Delete(cb));
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_MOUSE_Init_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the host mouse
*       user driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the user
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a host mouse
*                           user driver.
*       NU_NOT_PRESENT      Indicates there exists no user driver.
*
*************************************************************************/
STATUS  NU_USBH_MOUSE_Init_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBH_MOUSE_Cb_Pt;

    if (NU_USBH_MOUSE_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_MSE_DM_Open
*
* DESCRIPTION
*
*       This function is called by the application when it opens a device
*       for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the mouse driver passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/

STATUS  NU_USBH_MSE_DM_Open (VOID* dev_handle)
{
    return (NU_SUCCESS);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_MSE_DM_Close
*
* DESCRIPTION
*
*       This function is called by the application when it wants to close a device
*       which it has opend already for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the mouse driver passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBH_MSE_DM_Close (VOID* dev_handle)
{
    return ( NU_SUCCESS );
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_MSE_DM_IOCTL
*
* DESCRIPTION
*
*       This function is called by the application when it wants to perform
*       a control operation on the device.
*
* INPUTS
*
*       dev_handle         Pointer to the mouse driver passed as context.
*       cmd                IOCTL number.
*       data               IOCTL data pointer of variable type.
*       length             IOCTL data length in bytes.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
*************************************************************************/
STATUS  NU_USBH_MSE_DM_IOCTL (void *session_handle,
                              INT cmd,
                              void *data,
                              int length)
{
    STATUS status;

    /* Get the keyboard device handle. */
    NU_USBH_MOUSE_DEVICE *MSE_device = (NU_USBH_MOUSE_DEVICE *)session_handle;

    /* Get the IOCTL control block. */
    HID_MSE_IOCTL_DATA *ioctl_data = (HID_MSE_IOCTL_DATA* )data;

    switch(cmd)
    {
        /******************************** *
         * USB Host Mouse User IOCTLS *
         ******************************** */

        case (USB_MSE_IOCTL_BASE + USBH_MSE_Get_USAGES):

            status = _NU_USBH_MOUSE_Get_Usages ((NU_USBH_HID_USER *)NU_USBH_MOUSE_Cb_Pt,
                                                NU_NULL,
                                                (NU_USBH_HID_USAGE*)ioctl_data->data_buffer,
                                                ioctl_data->usages);
        break;

        case (USB_MSE_IOCTL_BASE + USBH_MSE_REG_EVENT_HANDLER):

            status = NU_USBH_MOUSE_Reg_Event_Handler(NU_USBH_MOUSE_Cb_Pt,
                                                    (NU_USBH_MOUSE_Event_Handler)ioctl_data->call_back);
        break;

        case (USB_MSE_IOCTL_BASE + USBH_MSE_REG_STATE_HANDLER):

            status = NU_USBH_MOUSE_Reg_State_Handlers(NU_USBH_MOUSE_Cb_Pt,
                                                   ioctl_data->context,
                                                   (NU_USBH_MOUSE_CALLBACK*)ioctl_data->call_back);
        break;

        case (USB_MSE_IOCTL_BASE + USBH_MSE_DELETE):

            status = _NU_USBH_MOUSE_Delete(NU_USBH_MOUSE_Cb_Pt);

        break;

        case (USB_MSE_IOCTL_BASE + USBH_MSE_GET_HANDLE):

            status = NU_USBH_MOUSE_Init_GetHandle(&ioctl_data->data_buffer);

        break;

        case (USB_MSE_IOCTL_BASE + USBH_MSE_GET_INFO):

            status = NU_USBH_MOUSE_Get_Info(NU_USBH_MOUSE_Cb_Pt,MSE_device,
                                           &ioctl_data->num_buttons,&ioctl_data->wheel_present);

        break;

        default:
            status = DV_IOCTL_INVALID_CMD;
        break;
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_MSE_DM_Read
*
* DESCRIPTION
*
*       This function is called by the application when it wants to read
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the mouse driver passed as context.
*       buffer             Pointer to memory location where to put the read data.
*       numbyte            Number of bytes to be read.
*       byte_offset        In case read data is to be placed at certain offset in the buffer.
*       bytes_read_ptr     OUTPUT: Number of bytes actually read.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBH_MSE_DM_Read (VOID *session_handle,
                             VOID *buffer,
                             UINT32 numbyte,
                             OFFSET_T byte_offset,
                             UINT32 *bytes_read_ptr)
{
    return ( NU_SUCCESS );
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_MSE_DM_Write
*
* DESCRIPTION
*
*       This function is called by the application when it wants to write
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the mouse driver passed as context.
*       buffer             Pointer to memory location where data to be written is available.
*       numbyte            Number of bytes to be written.
*       byte_offset        In case data is to be read at certain offset in the buffer.
*       bytes_written_ptr  OUTPUT: Number of bytes actually written.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/

STATUS  NU_USBH_MSE_DM_Write (VOID *session_handle,
                              const VOID *buffer,
                              UINT32 numbyte,
                              OFFSET_T byte_offset,
                              UINT32 *bytes_written_ptr)
{
    return ( NU_SUCCESS );
}
/*************************** end of file ********************************/

