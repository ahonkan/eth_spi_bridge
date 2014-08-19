/**************************************************************************
*
*           Copyright 2005 Mentor Graphics Corporation
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
*       nu_usbf_kb_ext.c
*
* COMPONENT
*
*       Nucleus USB Function Software : HID Keyboard user Driver.
*
* DESCRIPTION
*
*       This file contains the external Interfaces exposed by Keyboard HID
*       User Driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       nu_os_conn_usb_func_hid_kbd_init    This function initializes the
*                                           function keyboard user driver.
*       NU_USBF_KB_Create                   Creates an instance of  HID
*                                           keyboard User driver.
*       _NU_USBF_KB_Delete                  Deletes an instance of HID
*                                           keyboard User Driver.
*       _NU_USBF_KB_Disconnect              Disconnect Notification
*                                           function.
*       _NU_USBF_KB_Connect                 Connect Notification function.
*       _NU_USBF_KB_New_Command             Processes a new command.
*       _NU_USBF_KB_New_Transfer            Processes a new transfer.
*       _NU_USBF_KB_Notify                  USB event notification
*                                           processing.
*       _NU_USBF_KB_Tx_Done                 Transfer completion
*                                           notification.
*       NU_USBF_KB_Send_Key_Event           Sends the user's data to
*                                           the class driver.
*       NU_USBF_KB_Wait                     Wait function which waits
*                                           for a device to be connected.
*       NU_USBF_KB_Init_GetHandle           This function is used to
*                                           retrieve the function keyboard
*                                           user driver's address.
*       NU_USBF_KB_Reg_Callback             This function is used to
*                                           register the receive callback
*                                           with the user driver.
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions
*
**************************************************************************/

/* ==============  USB Include Files =================================  */
#include    "connectivity/nu_usb.h"

static UINT8 fs_hs_kbd_intf_desc[] =
{
    /* Interface Descriptor. */
    0x09,                                    /* bLength              */
    USB_DT_INTERFACE,                        /* INTERFACE            */
    0x00,                                    /* bInterfaceNumber     */
    0x00,                                    /* bAlternateSetting    */
    0x01,                                    /* bNumEndpoints        */
    HIDF_CLASS,                              /* bInterfaceClass      */
    HIDF_SUBCLASS,                           /* bInterfaceSubClass   */
    0x01,                                    /* bInterfaceProtocol   */
    0x00,                                    /* iInterface           */

    /* HID Descriptor. */
    0x09,                                    /* bLength              */
    USB_DESC_TYPE_HID,                       /* bDescriptorType      */
    0x01,                                    /* bcdHID1              */
    0x01,                                    /* bcdHID2              */
    0x00,                                    /* bCountryCode         */
    0x01,                                    /* bNumDescriptors      */
    0x22,                                    /* bDescriptorType      */
    0x41,                                    /* wDescriptorLength1   */
    0x00,                                    /* wDescriptorLength2   */

    /* Endpoint Descriptors.  */
    0x07,                                    /* bLength              */
    USB_DT_ENDPOINT,                         /* ENDPOINT             */
    USB_DIR_IN,                              /* bEndpointAddress     */
    USB_EP_INTR,                             /* bmAttributes         */
    0x00,                                    /* wMaxPacketSize       */
    0x00,                                    /* wMaxPacketSize       */
    0x00                                     /* bInterval            */
};

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
static UINT8 ss_kbd_intf_desc[] =
{
    /* Interface Descriptor. */
    0x09,                                    /* bLength              */
    USB_DT_INTERFACE,                        /* INTERFACE            */
    0x00,                                    /* bInterfaceNumber     */
    0x00,                                    /* bAlternateSetting    */
    0x01,                                    /* bNumEndpoints        */
    HIDF_CLASS,                              /* bInterfaceClass      */
    HIDF_SUBCLASS,                           /* bInterfaceSubClass   */
    0x01,                                    /* bInterfaceProtocol   */
    0x00,                                    /* iInterface           */

    /* HID Descriptor. */
    0x09,                                    /* bLength              */
    USB_DESC_TYPE_HID,                       /* bDescriptorType      */
    0x01,                                    /* bcdHID1              */
    0x01,                                    /* bcdHID2              */
    0x00,                                    /* bCountryCode         */
    0x01,                                    /* bNumDescriptors      */
    0x22,                                    /* bDescriptorType      */
    0x41,                                    /* wDescriptorLength1   */
    0x00,                                    /* wDescriptorLength2   */

    /* Endpoint Descriptors.  */
    0x07,                                    /* bLength              */
    USB_DT_ENDPOINT,                         /* ENDPOINT             */
    USB_DIR_IN,                              /* bEndpointAddress     */
    USB_EP_INTR,                             /* bmAttributes         */
    0x00,                                    /* wMaxPacketSize       */
    0x00,                                    /* wMaxPacketSize       */
    0x00,                                    /* bInterval            */

    /* Endpoint Companion Descriptor. */
    6,
    USB_DT_SSEPCOMPANION,
    0,
    0,
    0,
    0
};
#endif
/*************************************************************************
*   FUNCTION
*
*       nu_os_conn_usb_func_hid_kbd_init
*
*   DESCRIPTION
*
*       This function initializes the keyboard user driver component.
*
*   INPUTS
*
*       path               Registry path of component.
*       startstop          Flag to find if component is being enabled or
*                          disabled.
*
*   OUTPUTS
*
*       status             Success or failure of the creation of the
*                          underlying initialization routines.
*
*************************************************************************/
STATUS nu_os_conn_usb_func_hid_kbd_init(CHAR *path, INT startstop)
{
    STATUS  status, internal_sts = NU_SUCCESS;
    VOID   *usbf_hid_handle = NU_NULL;
    UINT8   rollback = 0;

    if(startstop)
    {
        /* Allocate memory for keyboard user driver control block. */
        status = USB_Allocate_Object(sizeof(NU_USBF_KB), (VOID **)&NU_USBF_KB_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        /* Create the device subsystem. */
        if (!rollback)
        {
            /* Zero out allocated block. */
            memset(NU_USBF_KB_Cb_Pt, 0, sizeof(NU_USBF_KB));

            /* Create the function HID Keyboard User driver. */
            status = NU_USBF_KB_Create(NU_USBF_KB_Cb_Pt,"USBFKBD", NU_NULL);
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        /* Get the function HID class driver handle. */
        if (!rollback)
        {
            status = NU_USBF_HID_Init_GetHandle(&usbf_hid_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        /* Register the user driver with the class driver. */
        if (!rollback)
        {
            status = NU_USB_DRVR_Register_User ((NU_USB_DRVR *) usbf_hid_handle,
                                                (NU_USB_USER *) NU_USBF_KB_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        /* Register USB Function Keyboard device with DM. */
        if(!rollback)
        {
            status = NU_USB_SYS_Register_Device((VOID *)NU_USBF_KB_Cb_Pt,
                                                NU_USBCOMPF_KBD);
            if(status != NU_SUCCESS)
            {
                rollback = 4;
            }
        }

        /* Clean up in case of error. */
        switch (rollback)
        {
            case 4:
            	internal_sts = NU_USB_SYS_DeRegister_Device( (VOID *)NU_USBF_KB_Cb_Pt,
            			                                    NU_USBCOMPH_KBD );

            case 3:
                internal_sts |= NU_USB_DRVR_Deregister_User((NU_USB_DRVR *) usbf_hid_handle,
                                                           (NU_USB_USER *) NU_USBF_KB_Cb_Pt);

            case 2:
                internal_sts |= _NU_USBF_KB_Delete((VOID *) NU_USBF_KB_Cb_Pt);

            case 1:
                if (NU_USBF_KB_Cb_Pt)
                {
                    internal_sts |= USB_Deallocate_Memory((VOID *)NU_USBF_KB_Cb_Pt);
                    NU_USBF_KB_Cb_Pt = NU_NULL;
                }
                break;

            default:
                break;
        }
    }
     else
    {
        /* Clean up. */
        status = NU_USBF_HID_Init_GetHandle(&usbf_hid_handle);
        if ((usbf_hid_handle) && (NU_USBF_KB_Cb_Pt))
        {
            internal_sts = NU_USB_SYS_DeRegister_Device((VOID *)NU_USBF_KB_Cb_Pt, NU_USBCOMPF_KBD);
            internal_sts |= NU_USB_DRVR_Deregister_User((NU_USB_DRVR *) usbf_hid_handle,
                                                        (NU_USB_USER *) NU_USBF_KB_Cb_Pt);
            internal_sts |= _NU_USBF_KB_Delete(NU_USBF_KB_Cb_Pt);
            internal_sts |= USB_Deallocate_Memory(NU_USBF_KB_Cb_Pt);
            NU_USBF_KB_Cb_Pt = NU_NULL;
        }
    }

    /* internal_sts is not used after this. So to remove
     * KW and PC-Lint warning set it as unused variable.
     */
    NU_UNUSED_PARAM(internal_sts);

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_KB_Create
*
* DESCRIPTION
*
*       HID User Driver initialization routine
*
* INPUTS
*       cb              Pointer to HID user driver control block.
*       name            Name of this USB object.
*       rx_callback     pointer to the function called on receiving
*                       data from the host.
* OUTPUTS
*
*       NU_SUCCESS      Successful Initialization
*
*************************************************************************/
STATUS NU_USBF_KB_Create(NU_USBF_KB   *cb,
                         char *name,
                         KEYBOARD_RX_CALLBACK rx_callback)

{
    STATUS status = NU_SUCCESS;
    UNSIGNED    Initial_Time = 100;
    UNSIGNED    Reschedule_Time = Initial_Time;
    UINT8       rpt_idx = 0;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    if((cb == NU_NULL) || (name == NU_NULL))
    {
        status = NU_USB_INVLD_ARG;
    }

    if(status == NU_SUCCESS)
    {
        /* Reset the values of the control block */
        memset(cb,0,sizeof(NU_USBF_KB));

        /* Call the create function of base function user. */
        status = _NU_USBF_USER_Create ((NU_USBF_USER *) cb,
                                        "keyboard",
                                        USBF_KB_SUBCLASS,
                                        USBF_KB_PROTOCOL,
                                        &usbf_kb_dispatch);

    }

    if(status == NU_SUCCESS)
    {
        /* Create the timer which is used for calculating the time interval
         * between recurring reports sent on the interrupt IN pipe.
         * Initially Timer is disabled.
         */

        status = NU_Create_Timer(&cb->Idle_Timer,"Timer",
                                NU_USBF_KB_Timer_expiration_routine,
                                (UNSIGNED)cb,Initial_Time,
                                Reschedule_Time,
                                NU_DISABLE_TIMER);

        if(status == NU_SUCCESS)
            /* Create an event group which will be used for connection
             * event notification.
             */
            status = NU_Create_Event_Group(
                            (NU_EVENT_GROUP  *)&cb->device_connect_event,
                            "DevEvent");

        /* Create semaphore for connection synchronization. */
        if(status == NU_SUCCESS)
            status = NU_Create_Semaphore(
                                    (NU_SEMAPHORE  *)&cb->connection_lock,
                                    "ConnLCK",
                                     1,
                                     NU_FIFO);

        /* Save the receive callback function pointer in the control block.
         */
        if(status == NU_SUCCESS)
            cb->rx_callback = rx_callback;

        /* Add keyboard function with USB device configuration layer. */
        if(status == NU_SUCCESS)
        {
            /* Add descriptor for FULL and HIGH speed. */
            status = USBF_DEVCFG_Add_Function(USBF_DEF_CONFIG_INDEX,
                                              fs_hs_kbd_intf_desc,
                                              sizeof(fs_hs_kbd_intf_desc),
                                              fs_hs_kbd_intf_desc,
                                              sizeof(fs_hs_kbd_intf_desc),
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                              ss_kbd_intf_desc,
                                              sizeof(ss_kbd_intf_desc),
#endif
                                              &cb->kbd_function);
            if ( status == NU_SUCCESS )
            {
                /* Create and initialize keyboard report descriptor. */
                status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                            USBF_KB_REPORT_DESCRIPTOR_LEN,
                                            (void**)&(cb->kb_report_descriptor));
                if ( status == NU_SUCCESS )
                {
                    /* Reset contents to zero. */
                    memset(cb->kb_report_descriptor, 0x00, USBF_KB_REPORT_DESCRIPTOR_LEN);

                    /* Initialize keyboard report structures. */
                    cb->kb_report_descriptor[rpt_idx++] = 0x05; cb->kb_report_descriptor[rpt_idx++] = 0x01;
                    cb->kb_report_descriptor[rpt_idx++] = 0x09; cb->kb_report_descriptor[rpt_idx++] = 0x06;
                    cb->kb_report_descriptor[rpt_idx++] = 0xA1; cb->kb_report_descriptor[rpt_idx++] = 0x01;
                    cb->kb_report_descriptor[rpt_idx++] = 0x05; cb->kb_report_descriptor[rpt_idx++] = 0x07;
                    cb->kb_report_descriptor[rpt_idx++] = 0x19; cb->kb_report_descriptor[rpt_idx++] = 0xE0;
                    cb->kb_report_descriptor[rpt_idx++] = 0x29; cb->kb_report_descriptor[rpt_idx++] = 0xE7;
                    cb->kb_report_descriptor[rpt_idx++] = 0x15; cb->kb_report_descriptor[rpt_idx++] = 0x00;
                    cb->kb_report_descriptor[rpt_idx++] = 0x25; cb->kb_report_descriptor[rpt_idx++] = 0x01;
                    cb->kb_report_descriptor[rpt_idx++] = 0x95; cb->kb_report_descriptor[rpt_idx++] = 0x08;
                    cb->kb_report_descriptor[rpt_idx++] = 0x75; cb->kb_report_descriptor[rpt_idx++] = 0x01;
                    cb->kb_report_descriptor[rpt_idx++] = 0x81; cb->kb_report_descriptor[rpt_idx++] = 0x02;
                    cb->kb_report_descriptor[rpt_idx++] = 0x95; cb->kb_report_descriptor[rpt_idx++] = 0x08;
                    cb->kb_report_descriptor[rpt_idx++] = 0x75; cb->kb_report_descriptor[rpt_idx++] = 0x01;
                    cb->kb_report_descriptor[rpt_idx++] = 0x81; cb->kb_report_descriptor[rpt_idx++] = 0x01;
                    cb->kb_report_descriptor[rpt_idx++] = 0x05; cb->kb_report_descriptor[rpt_idx++] = 0x08;
                    cb->kb_report_descriptor[rpt_idx++] = 0x19; cb->kb_report_descriptor[rpt_idx++] = 0x01;
                    cb->kb_report_descriptor[rpt_idx++] = 0x29; cb->kb_report_descriptor[rpt_idx++] = 0x03;
                    cb->kb_report_descriptor[rpt_idx++] = 0x95; cb->kb_report_descriptor[rpt_idx++] = 0x03;
                    cb->kb_report_descriptor[rpt_idx++] = 0x75; cb->kb_report_descriptor[rpt_idx++] = 0x01;
                    cb->kb_report_descriptor[rpt_idx++] = 0x91; cb->kb_report_descriptor[rpt_idx++] = 0x02;
                    cb->kb_report_descriptor[rpt_idx++] = 0x95; cb->kb_report_descriptor[rpt_idx++] = 0x01;
                    cb->kb_report_descriptor[rpt_idx++] = 0x75; cb->kb_report_descriptor[rpt_idx++] = 0x05;
                    cb->kb_report_descriptor[rpt_idx++] = 0x91; cb->kb_report_descriptor[rpt_idx++] = 0x01;
                    cb->kb_report_descriptor[rpt_idx++] = 0x05; cb->kb_report_descriptor[rpt_idx++] = 0x07;
                    cb->kb_report_descriptor[rpt_idx++] = 0x19; cb->kb_report_descriptor[rpt_idx++] = 0x00;
                    cb->kb_report_descriptor[rpt_idx++] = 0x2A; cb->kb_report_descriptor[rpt_idx++] = 0xFF;
                    cb->kb_report_descriptor[rpt_idx++] = 0x00; cb->kb_report_descriptor[rpt_idx++] = 0x15;
                    cb->kb_report_descriptor[rpt_idx++] = 0x00; cb->kb_report_descriptor[rpt_idx++] = 0x26;
                    cb->kb_report_descriptor[rpt_idx++] = 0xFF; cb->kb_report_descriptor[rpt_idx++] = 0x00;
                    cb->kb_report_descriptor[rpt_idx++] = 0x95; cb->kb_report_descriptor[rpt_idx++] = 0x06;
                    cb->kb_report_descriptor[rpt_idx++] = 0x75; cb->kb_report_descriptor[rpt_idx++] = 0x08;
                    cb->kb_report_descriptor[rpt_idx++] = 0x81; cb->kb_report_descriptor[rpt_idx++] = 0x00;
                    cb->kb_report_descriptor[rpt_idx++] = 0xC0;
                }
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}/*_NU_USBF_KB_Create */

/*************************************************************************
* FUNCTION
*
*       _NU_USBF_KB_Delete
*
* DESCRIPTION
*       This function deletes an instance of HID Keyboard User driver.
*
* INPUTS
*       cb              Pointer to the USB Object control block.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USBF_KB_Delete (VOID *cb)
{

    STATUS status;
    NU_USBF_KB *hid_user_keyboard = (NU_USBF_KB *)cb;

    if(cb == NU_NULL)
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Call the base user's delete function. */
        status = _NU_USBF_USER_Delete(cb);

        if(status == NU_SUCCESS)
        {
            status = USB_Deallocate_Memory(hid_user_keyboard->kb_report_descriptor);
            status = NU_Delete_Timer(&hid_user_keyboard->Idle_Timer);
            status = NU_Delete_Event_Group(
                                &hid_user_keyboard->device_connect_event);
            status = NU_Delete_Semaphore(
                                &hid_user_keyboard->connection_lock);
        }
    }

    return status;
}/* _NU_USBF_KB_Delete */

/*************************************************************************
* FUNCTION
*       _NU_USBF_KB_Disconnect
*
* DESCRIPTION
*       This function is called by the class driver's disconnect function
*       when a device being served by this user gets disconnected. This
*       cleans up the device specific entries associated with the device.
*
* INPUTS
*       cb              Pointer to user control block.
*       class_driver    Pointer to calling class driver's control block.
*       handle          Handle of the device disconnected.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USBF_KB_Disconnect (NU_USB_USER * cb,
                               NU_USB_DRVR * class_driver,
                               void *handle)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_KB *hid_user_keyboard = (NU_USBF_KB *)cb;

    if((cb == NU_NULL) || (class_driver == NU_NULL))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Clear the data values. */
        hid_user_keyboard->drvr = NU_NULL;
        hid_user_keyboard->handle = NU_NULL;

        /*Reset the connection lock semaphore. */
        status = NU_Reset_Semaphore((NU_SEMAPHORE  *)&hid_user_keyboard->connection_lock, 1);

        /* Reset connection & key event group*/
        status = NU_Set_Events(&hid_user_keyboard->device_connect_event,
                               (UNSIGNED)0,
                               NU_AND);
    }

    return status;
}/* _NU_USBF_KB_Disconnect */

/*************************************************************************
* FUNCTION
*       _NU_USBF_KB_Connect
*
* DESCRIPTION
*       This function is called by class driver's Initialize_intf/Device
*       routine when a new device which can be served by this user is found
*
* INPUTS
*       cb              Pointer to user control block.
*       class_driver    Pointer to calling class driver's control block.
*       handle          Handle for the device connected.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/

STATUS _NU_USBF_KB_Connect (NU_USB_USER * cb,
                            NU_USB_DRVR * class_driver,
                            void *handle)
{

    STATUS status = NU_SUCCESS;

    NU_USBF_KB *hid_user_keyboard = (NU_USBF_KB *)cb;

    if((cb == NU_NULL) || (class_driver == NU_NULL))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Initialize the driver's data to default values. */
        hid_user_keyboard->drvr = class_driver;
        hid_user_keyboard->handle = (USBF_HID_DEVICE*)handle;

        status = NU_Set_Events(&hid_user_keyboard->device_connect_event,
                               HIDF_DEVICE_CONNECT,
                               NU_OR);

    }
    return status;

}/* _NU_USBF_KB_Connect */


/*************************************************************************
* FUNCTION
*
*       NU_USBF_KB_New_Command
*
* DESCRIPTION
*
*       Processes a new command from the Host.
*
*       These commands are class/subclass specific. This function processes
*       the command identified by the 'command' parameter. The length of
*       the command block, in bytes is expected in the 'cmd_len' parameter.
*
*       If there is any data transfer in response to a command, then the
*       location corresponding to the data to be transferred, either to
*       / from Host is filled in the 'data_out' parameter. The length of
*       the data to be transferred is filled in the location pointed to by
*       the 'data_len_out' parameter.
*
*       If there is no data transfer associated with the command, then, the
*       location pointed to by the 'data' is filled in with NU_NULL.
*
*       For unknown and unsupported command, this function returns
*       appropriate error status.
*
* INPUTS
*
*       cb              User control block for which the command is meant
*                       for.
*
*       drvr            Class driver invoking this function
*
*       handle          Identifies for the logical function to which the
*                       command is directed to.
*
*       command         Memory location where the command block is stored
*
*       cmd_len         Length of the command block
*
*       data_out        Memory location where the data pointer for the
*                       transfer is to be stored
*
*       data_len_out    Memory location where the length of data to be
*                       transferred, in bytes, must be filled.
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates that the command has been processed
*                       successfully
*
*       NU_USB_NOT_SUPPORTED
*                       Indicates that the command is unsupported
*
*       NU_USB_INVLD_ARG
*                       Indicates a malformed command block
*
*************************************************************************/

STATUS _NU_USBF_KB_New_Command (NU_USBF_USER * cb,
                                NU_USBF_DRVR * drvr,
                                VOID *handle,
                                UINT8 *command,
                                UINT16 cmd_len,
                                UINT8 **data_out,
                                UINT32 *data_len_out)
{
    STATUS status;

    if((cb == NU_NULL) || (drvr == NU_NULL))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        NU_USBF_HID_CMD *rcvd_cmd = (NU_USBF_HID_CMD *)command;

        /* Call implementation function to process the request */
        status = NU_USBF_KB_Process_Current_Request((NU_USBF_KB *)cb,
                                                    rcvd_cmd,
                                                    data_out,
                                                    data_len_out);

    }

    return status;
}/* _NU_USBF_KB_New_Command */
/*************************************************************************
* FUNCTION
*
*       NU_USBF_KB_User_New_Transfer
*
* DESCRIPTION
*
*       This function processes a transfer request from the host for a
*       previously sent command.
*
*       If there is any data transfer in response to a previously received
*       command, then the  location corresponding to the data to be
*       transferred, either to / from Host is filled in the 'data_out'
*       parameter. The length of the data to be transferred is filled
*       in the location pointed to by the 'data_len_out' parameter.
*
*       If there is no data transfer required, then this function returns
*       appropriate error status.
*
* INPUTS
*
*       cb              User control block
*
*       drvr            Class driver invoking this function
*
*       handle          Identifies for the logical function to which the
*                       transfer is directed to.
*
*       data_out        Memory location where the data pointer for the
*                       transfer is to be stored
*
*       data_len_out    Memory location where the length of data to be
*                       transferred, in bytes, must be filled.
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates that the function has executed
*                       successfully
*       NU_USB_INVLD_ARG
*                       indicates an unexpected transfer request from
*                       the Host.
*
*       NU_USB_NOT_SUPPORTED
*                       indicates that the new transfer requests are
*                       not supported by the user.
*
*************************************************************************/
STATUS _NU_USBF_KB_New_Transfer(NU_USBF_USER * cb,
                                NU_USBF_DRVR * drvr,
                                VOID *handle,
                                UINT8 **data_out,
                                UINT32 *data_len_out)
{

    return(NU_SUCCESS);

}/* _NU_USBF_KB_User_New_Transfer */
/*************************************************************************
* FUNCTION
*
*       NU_USBF_KB_Notify
*
* DESCRIPTION
*
*       This function processes a USB Event. Examples of such USB events
*       include suspend and Reset.
*
*       This function carries out function specific processing for the usb
*       event.
*
*
* INPUTS
*
*       cb              User control block for which the event is meant
*                       for.
*       drvr            Class driver invoking this function
*
*       handle          Identifies the logical function to which the
*                       notification is directed.
*
*       event           The USB event that has occurred.
*
* OUTPUTS
*
*       NU_SUCCESS      indicates that the event has been processed
*                       successfully
*
*       NU_USB_INVLD_ARG
*                       indicates a unexpected event.
*
*       NU_USB_NOT_SUPPORTED
*                       indicates that the event notifications are not
*                       supported by the user.
*
*************************************************************************/
STATUS _NU_USBF_KB_Notify (NU_USBF_USER * cb,
                           NU_USBF_DRVR * drvr,
                           void *handle,
                           UINT32 event)
{
    return (NU_SUCCESS);
}/* NU_USBF_KB_Notify */


/*************************************************************************
* FUNCTION
*
*       NU_USBF_KB_Tx_Done
*
* DESCRIPTION
*
*       This function processes a transfer completion notification for a
*       previously submitted data transfer.
*
*       The completed data transfer is described in the 'completed_data'
*       and the 'completed_data_out' parameters. The 'completed_data'
*       contains the pointer to the memory location to / from which
*       'completed_data_out' bytes have been transferred from/to host.
*
*       If there is still data to be transferred to the previous
*       command, then the  location corresponding to the data to be
*       transferred, either to / from Host is filled in the 'data_out'
*       parameter. The length of the data to be transferred is filled
*       in the location pointed to by the 'data_len_out' parameter.
*
*       If there is no data transfer associated with the command, then, the
*       location pointed to by the 'data' is filled in with NU_NULL.
*
* INPUTS
*
*       cb              User control block
*
*       drvr            Class driver invoking this function
*
*       handle          Identifies the logical function to which the
*                       notification is directed.
*
*       completed_data  Memory location to / from where the data has been
*                       transferred
*
*       completed_data_len
*                       length of data transferred, in bytes.
*
*       data_out        Memory location where the data pointer for the
*                       transfer is to be stored, if pending.
*
*       data_len_out    Memory location where the length of data to be
*                       transferred, in bytes, is to be filled.
*
* OUTPUTS
*
*       NU_SUCCESS      indicates that the function has executed
*                       successfully.
*
*       NU_USB_INVLD_ARG
*                       indicates a unexpected transfer completion.
*
*       NU_USB_NOT_SUPPORTED
*                       indicates that the transfer completion
*                       notifications are not supported by the user.
*
*************************************************************************/
STATUS _NU_USBF_KB_Tx_Done (NU_USBF_USER * cb,
                            NU_USBF_DRVR * drvr,
                            VOID *handle,
                            UINT8 *completed_data,
                            UINT32 completed_data_len,
                            UINT8 **data_out,
                            UINT32 *data_len_out)
{

    STATUS status;
    UINT32 tx_handle;
    NU_USBF_KB *hid = (NU_USBF_KB *)cb;


    status = NU_SUCCESS;
    if((cb == NU_NULL) || (drvr == NU_NULL) || (handle == NU_NULL))
    {
         status = NU_USB_INVLD_ARG;
    }
    else
    {
        tx_handle = *(UINT32 *)handle;

        /* If completion is received for control IN transfer, do nothing.
         */
        if(tx_handle == HIDF_CTRL_IN_XFER_CMPLT)
        {
            status = NU_SUCCESS;
        }

        if(tx_handle == HIDF_REPORT_SENT)
        {

            status = NU_Set_Events(&hid->device_connect_event,
                                    HIDF_DATA_SENT,
                                    NU_OR);
        }

    }

    return (status);
}/* _NU_USBF_KB_Tx_Done*/


/*************************************************************************
* FUNCTION
*       NU_USBF_KB_Send_Key_Event
*
* DESCRIPTION
*       This function is used to pass data from the user driver to the
*       class driver.User driver can send at the maximum of six keys in
*       the key buffer otherwise the keyboard reports a phantom state
*       indexing usage(ErrorRollOver).Class driver sends the keyboard
*       report data on the interrupt IN pipe.
*
*
*
* INPUTS
*       cb                      pointer to User control block.
*       key                     pointer to an array of pointers.
*       buffersize              size of array of pointers key.
*       modifierkeybuffer       pointer to an array of pointers.
*       modifierkeybuffersize   size of array of character pointers
*                               modifierkeybuffer
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion.
*
*
*************************************************************************/
STATUS NU_USBF_KB_Send_Key_Event(NU_USBF_KB *cb,
                                 CHAR **Key,
                                 UINT8 buffersize,
                                 UINT8 *modifierkeybuffer,
                                 UINT8 modifierkeybuffersize)

{
    STATUS status;
    NU_USBF_KB_IN_REPORT *keyboardreport = NU_NULL;
    INT i;
    UINT32 event;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Allocate memory to keyboard report. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 sizeof(NU_USBF_KB_IN_REPORT),
                                 (VOID**)&keyboardreport);
    if ( status == NU_SUCCESS )
    {
        /* First initialize the value of all the fields of the data structure
         * to be 0.
         */
        memset(keyboardreport,0,sizeof(NU_USBF_KB_IN_REPORT));

        /* For each key pressed update the array fields in the keyboard report
         * data.
         */
        for(i=0;i<buffersize;i++)
        {
            NU_USBF_KB_Fill_Key_Data(Key[i],&keyboardreport->keyCode[i]);
        }

        /* Fill up the modifier buffer according to the states of modifier keys
         */
        NU_USBF_KB_Fill_Modifier_Byte(&keyboardreport->modifierByte,
                                      modifierkeybuffer,
                                      modifierkeybuffersize);

        /* Send report data to the class driver*/
        status = NU_USBF_HID_Send_Report((NU_USBF_HID*)cb->drvr,
                                         (UINT8*)keyboardreport,
                                         sizeof(NU_USBF_KB_IN_REPORT),
                                         cb->handle);

        if(status == NU_SUCCESS)
        {
            /* Wait for the completion of data transfer. */
            status = NU_Retrieve_Events(&cb->device_connect_event,
                                        HIDF_DATA_SENT,
                                        NU_AND_CONSUME,
                                        &event,
                                        NU_SUSPEND);
        }
    }

    if ( keyboardreport )
    {
        USB_Deallocate_Memory(keyboardreport);
        keyboardreport = NU_NULL;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;

}/*NU_USBF_KB_Send_Key_Event*/

/*************************************************************************
* FUNCTION
*       NU_USBF_KB_Wait
*
* DESCRIPTION
*       This function provides services for user level threads to wait for
*       a particular device to be connected. Thread goes into a state
*       specified by suspension option if the device is not yet connected.
*       This service returns a device handle as output when the call is
*       successful, which can be used by applications to use other services
*       of USER layer. When a thread first calls this service, it checks if
*       some device belonging to the user is already connected and gives
*       out its handle, else waits for device to be connected. If this
*       service is called again from the same thread, it checks for the
*       next available device and waits if it is not yet connected. This
*       can help applications in traversing the devices to find a suitable
*       device.
*
* INPUTS
*       cb          pointer to User control block.
*       suspend     Suspension option.
*       handle_out  pointer to memory location to hold pointer to the
*                   device handle.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion.
*       NU_TIMEOUT              Indicates  timeout on suspension.
*       NU_NOT_PRESENT          Indicates event flags are not present.
*       NU_USB_INTERNAL_ERROR    Indicates a internal error in USB subsystem
*
*
*************************************************************************/
STATUS NU_USBF_KB_Wait (NU_USBF_KB * cb,
                        UNSIGNED suspend,
                        VOID **handle_out)
{

    STATUS status;
    UINT32 event;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);

    /*This service obtains an instance of the specified semaphore.
    Since instances are implemented with an internal counter,obtaining
    a semaphore translates into decrementing the semaphore.s internal
    counter by one.If the semaphore counter is zero before this call,
    the service cannot be immediately satisfied.*/
    status = NU_Obtain_Semaphore((NU_SEMAPHORE  *)&cb->connection_lock,
                                  suspend);

    if(status == NU_SUCCESS)
    {
        /* Wait for device connect event */
        status = NU_Retrieve_Events(&cb->device_connect_event,
                                    HIDF_DEVICE_CONNECT,
                                    NU_AND,
                                    &event,
                                    suspend);


        if(status != NU_SUCCESS)
        {
            /* Release semaphore if NU_Retrieve_Events service failed. */
            status = NU_Release_Semaphore(
                                (NU_SEMAPHORE *)&cb->connection_lock);
        }
        else
        {
            NU_ASSERT(cb->handle);
            *handle_out = cb->handle;
            /* Release semaphore on getting connection */
            status = NU_Release_Semaphore((NU_SEMAPHORE *)&cb->connection_lock);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_KB_Init_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the function keyboard
*       user driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the user
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a function keyboard
*                           user driver.
*       NU_NOT_PRESENT      Indicates there exists no user driver.
*
*************************************************************************/
STATUS NU_USBF_KB_Init_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBF_KB_Cb_Pt;
    if (NU_USBF_KB_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_KB_Reg_Callback
*
*   DESCRIPTION
*
*       This function is used to register the receive callback with the
*       user driver.
*
*   INPUTS
*
*       cb                  Pointer to the control block of the user
*                           driver.
*       rx_callback         Pointer to the application callback invoked
*                           on receiving data from the USB host.
*
*
*   OUTPUTS
*
*       NU_SUCCESS          Successful registration of the callback
*
*       NU_USB_INVLD_ARG    Invalid argument.
*
*************************************************************************/
STATUS NU_USBF_KB_Reg_Callback(NU_USBF_KB   *cb,
                                 KEYBOARD_RX_CALLBACK rx_callback)
{
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    if (rx_callback == NU_NULL)
        status = NU_USB_INVLD_ARG;

    cb->rx_callback = rx_callback;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_KB_USER_Bind_Interface
*
* DESCRIPTION
*
*       This function actually binds HID keyboard function with USB device
*       configuration. It is necessary that keyboard function is
*       already registered with USB device configuration glue layer.
*
* INPUTS
*
*       cb                      Pointer to keyboard function driver
*                               control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that keyboard function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_KB_USER_Bind_Interface(NU_USBF_KB *cb)
{
    /* Local variables. */
    STATUS     status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;
    if ( cb != NU_NULL )
    {
        status = NU_NOT_PRESENT;
        if ( cb->kbd_function != NU_NULL )
        {
            status = NU_USB_DEVICE_Set_bDeviceClass(cb->kbd_function->device,
                                                    HIDF_CLASS);
            if ( status == NU_SUCCESS )
            {
                /* Enable keyboard function. */
                status = USBF_DEVCFG_Enable_Function(cb->kbd_function);
                if ( status == NU_SUCCESS )
                {
                    /* Bind keyboard function with USB device configuration. */
                    status = USBF_DEVCFG_Bind_Function(cb->kbd_function);
                }
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_KB_USER_Unbind_Interface
*
* DESCRIPTION
*
*       This function actually unbinds HID keyboard function with USB
*       device configuration.
*       It is necessary that keyboard function is already registered
*       with USB device configuration glue layer.
*
* INPUTS
*
*       cb                      Pointer to keyboard function driver
*                               control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that keyboard function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_KB_USER_Unbind_Interface(NU_USBF_KB *cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;
    if ( cb != NU_NULL )
    {
        status = NU_NOT_PRESENT;
        if ( cb->kbd_function != NU_NULL )
        {
            status = USBF_DEVCFG_Unbind_Function(cb->kbd_function);
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_KB_DM_Open
*
* DESCRIPTION
*
*       This function is called by the application when it opens a device
*       for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the keyboard driver passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBF_KB_DM_Open (VOID* dev_handle)
{
    STATUS      status;
    NU_USBF_KB *kbd_cb;

    kbd_cb = (NU_USBF_KB *) dev_handle;
    status = NU_USBF_KB_USER_Bind_Interface(kbd_cb);

    return (status) ;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_KB_DM_Close
*
* DESCRIPTION
*
*       This function is called by the application when it wants to close
*       a device which it has opend already for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the keyboard driver passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBF_KB_DM_Close(VOID* dev_handle)
{
    STATUS      status;
    NU_USBF_KB *kbd_cb;

    kbd_cb = (NU_USBF_KB *) dev_handle;
    status = NU_USBF_KB_USER_Unbind_Interface(kbd_cb);

    return (status) ;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_KB_DM_Read
*
* DESCRIPTION
*
*       This function is called by the application when it wants to read
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the keyboard driver passed as context.
*       buffer             Pointer to memory location where to put the read data.
*       numbyte            Number of bytes to be read.
*       byte_offset        In case read data is to be placed at certain offset
*                          in the buffer.
*       bytes_read_ptr     OUTPUT: Number of bytes actually read.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBF_KB_DM_Read(VOID*    dev_handle,
                           VOID*    buffer,
                           UINT32   numbyte,
                           OFFSET_T byte_offset,
                           UINT32*  bytes_read_ptr)
{
    STATUS status = NU_SUCCESS;
    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_KB_DM_Write
*
* DESCRIPTION
*
*       This function is called by the application when it wants to write
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the keyboard driver passed as context.
*       buffer             Pointer to memory location where data to be written is avaiable.
*       numbyte            Number of bytes to be written.
*       byte_offset        In case data is to be read at certain offset in the buffer.
*       bytes_written_ptr  OUTPUT: Number of bytes actually written.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBF_KB_DM_Write(VOID*       dev_handle,
                            const VOID* buffer,
                            UINT32      numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*     bytes_written_ptr)
{
    STATUS status = NU_SUCCESS;
    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_KB_DM_IOCTL
*
* DESCRIPTION
*
*       This function is called by the application when it wants to perform
*       a control operation on the device.
*
* INPUTS
*
*       dev_handle         Pointer to the keyboard driver passed as context.
*       cmd                IOCTL number.
*       data               IOCTL data pointer of variable type.
*       length             IOCTL data length in bytes.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBF_KB_DM_IOCTL(VOID*     dev_handle,
                            INT       ioctl_num,
                            VOID*     ioctl_data,
                            INT       ioctl_data_len)
{
    STATUS                  status;
    NU_USBF_KB             *kbd_cb = (NU_USBF_KB *)dev_handle;
    NU_USBF_HID            *usbf_hid_pt = NU_NULL;
    USBF_KBD_WAIT_DATA     *kbd_wait_data;
    USBF_KBD_SEND_KEY_DATA *kbd_send_data;

    switch(ioctl_num)
    {
        /*********************************** *
         * USB Function Keyboard User IOCTLS *
         *********************************** */
        case (USB_KBD_IOCTL_BASE + NU_USBF_KBD_IOCTL_REG_CALLBACK):
            status = NU_USBF_KB_Reg_Callback(kbd_cb, *(KEYBOARD_RX_CALLBACK *) ioctl_data);
            break;

        case (USB_KBD_IOCTL_BASE + NU_USBF_KBD_IOCTL_WAIT):
            kbd_wait_data = (USBF_KBD_WAIT_DATA *)ioctl_data;
            status = NU_USBF_KB_Wait(kbd_cb,
                                     kbd_wait_data->suspend,
                                     kbd_wait_data->handle_out);
            break;

        case (USB_KBD_IOCTL_BASE + NU_USBF_KBD_IOCTL_SEND_KEY_EVENT):
            kbd_send_data = (USBF_KBD_SEND_KEY_DATA *)ioctl_data;
            status = NU_USBF_KB_Send_Key_Event(kbd_cb,
                                               kbd_send_data->key,
                                               kbd_send_data->buffersize,
                                               kbd_send_data->modifierkeybuffer,
                                               kbd_send_data->modifierkeybuffersize);
            break;

        /************************* *
         * USB Function HID IOCTLS *
         ************************* */
        case (USB_KBD_IOCTL_BASE + NU_USBF_KBD_IOCTL_GET_HID_CB):
            status = NU_USBF_HID_Init_GetHandle((VOID **)ioctl_data);
            break;

        case (USB_KBD_IOCTL_BASE + NU_USBF_KBD_IOCTL_IS_HID_DEV_CONNECTED):
            status = NU_USBF_HID_Init_GetHandle((VOID **)&usbf_hid_pt);
            if(status == NU_SUCCESS)
            {
                if(usbf_hid_pt->devices[0] != NU_NULL)
                {
                    *(BOOLEAN *)ioctl_data = NU_TRUE;
                }
                else
                {
                    *(BOOLEAN *)ioctl_data = NU_FALSE;
                }
            }
            break;

        default:
        {
            status = NU_USB_NOT_SUPPORTED;
            break;
        }
    }

    return (status);
}

/* ======================  End Of File  =============================== */
