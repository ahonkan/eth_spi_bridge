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
*       nu_usbf_mse_ext.c
*
* COMPONENT
*
*       Nucleus USB Function Software : HID Mouse User Driver.
*
* DESCRIPTION

*       This file contains the external Interfaces exposed by
*       HID Mouse User Driver.
*
* DATA STRUCTURES

*       None.
*
* FUNCTIONS
*
*       nu_os_conn_usb_func_hid_mse_init    This function initializes the
*                                           function mouse user driver.
*       NU_USBF_MSE_Create                  Creates an instance of  HID
*                                           User Driver.
*       _NU_USBF_MSE_Delete                 Deletes an  instance of HID
*                                           User Driver.
*       _NU_USBF_MSE_Disconnect             Disconnect  Notification
*                                           function.
*       _NU_USBF_MSE_Connect                Connect Notification function.
*       _NU_USBF_MSE_New_Command            Processes a new command.
*       _NU_USBF_MSE_New_Transfer           Processes a new transfer.
*       _NU_USBF_MSE_Notify                 USB event notification
*                                           processing.
*       _NU_USBF_MSE_Tx_Done                Transfer completion
*                                           notification.
*       NU_USBF_MSE_Send_Lft_Btn_Click      Sends a Left Button Click to
*                                           the user driver.
*       NU_USBF_MSE_Send_Mdl_Btn_Click      Sends a Middle  button click to
*                                           the user driver.
*       NU_USBF_MSE_Send_Rht_Btn_Click      Sends a Right button click  to
*                                           the user driver.
*       NU_USBF_MSE_Move_Pointer            Sends a mouse move command to
*                                           the user driver.
*       NU_USBF_MSE_Wait                    Wait function for the user
*                                           driver. User driver waits until
*                                           a device is connected to the
*                                           host.
*       NU_USBF_MSE_Init_GetHandle          This function is used to
*                                           retrieve the function mouse
*                                           user driver's address.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ==============  USB Include Files =================================  */
#include    "connectivity/nu_usb.h"

static UINT8 fs_hs_mse_intf_desc[] =
{
    /* Interface Descriptor. */
    0x09,                                    /* bLength              */
    USB_DT_INTERFACE,                        /* INTERFACE            */
    0x00,                                    /* bInterfaceNumber     */
    0x00,                                    /* bAlternateSetting    */
    0x01,                                    /* bNumEndpoints        */
    HIDF_CLASS,                              /* bInterfaceClass      */
    HIDF_SUBCLASS,                           /* bInterfaceSubClass   */
    0x02,                                    /* bInterfaceProtocol   */
    0x00,                                    /* iInterface           */

    /* HID Descriptor. */
    0x09,                                    /* bLength              */
    USB_DESC_TYPE_HID,                       /* bDescriptorType      */
    0x00,                                    /* bcdHID1              */
    0x01,                                    /* bcdHID2              */
    0x00,                                    /* bCountryCode         */
    0x01,                                    /* bNumDescriptors      */
    0x22,                                    /* bDescriptorType      */
    0x32,                                    /* wDescriptorLength1   */
    0x00,                                    /* wDescriptorLength2   */

    /* Endpoint Descriptors. */
    0x07,                                    /* bLength              */
    USB_DT_ENDPOINT,                         /* ENDPOINT             */
    USB_DIR_IN,                              /* bEndpointAddress     */
    USB_EP_INTR,                             /* bmAttributes         */
    0x00,                                    /* wMaxPacketSize       */
    0x00,                                    /* wMaxPacketSize       */
    0x00                                     /* bInterval            */
};

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
static UINT8 ss_mse_intf_desc[] =
{
    /* Interface Descriptor. */
    0x09,                                    /* bLength              */
    USB_DT_INTERFACE,                        /* INTERFACE            */
    0x00,                                    /* bInterfaceNumber     */
    0x00,                                    /* bAlternateSetting    */
    0x01,                                    /* bNumEndpoints        */
    HIDF_CLASS,                              /* bInterfaceClass      */
    HIDF_SUBCLASS,                           /* bInterfaceSubClass   */
    0x02,                                    /* bInterfaceProtocol   */
    0x00,                                    /* iInterface           */

    /* HID Descriptor. */
    0x09,                                    /* bLength              */
    USB_DESC_TYPE_HID,                       /* bDescriptorType      */
    0x00,                                    /* bcdHID1              */
    0x01,                                    /* bcdHID2              */
    0x00,                                    /* bCountryCode         */
    0x01,                                    /* bNumDescriptors      */
    0x22,                                    /* bDescriptorType      */
    0x32,                                    /* wDescriptorLength1   */
    0x00,                                    /* wDescriptorLength2   */

    /* Endpoint Descriptors. */
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
*       nu_os_conn_usb_func_hid_mse_init
*
*   DESCRIPTION
*
*       This function initializes the mouse user driver component.
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
STATUS nu_os_conn_usb_func_hid_mse_init(CHAR *path, INT startstop)
{
    STATUS  status, internal_sts = NU_SUCCESS;
    VOID   *usbf_hid_handle = NU_NULL;
    UINT8   rollback = 0;

    if(startstop)
    {
        /* Allocate memory for mouse user driver control block. */
        status = USB_Allocate_Object(sizeof(NU_USBF_MSE), (VOID **)&NU_USBF_MSE_Cb_Pt);
        if (status != NU_SUCCESS)
        {
            rollback = 1;
        }

        /* Create the device subsystem. */
        if (!rollback)
        {
            /* Zero out allocated block. */
            memset(NU_USBF_MSE_Cb_Pt, 0, sizeof(NU_USBF_MSE));

            /* Create the function HID Mouse User driver. */
            status = NU_USBF_MSE_Create (NU_USBF_MSE_Cb_Pt, "mouse");
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        /*  Get the function HID class driver handle. */
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
            status = NU_USB_DRVR_Register_User((NU_USB_DRVR *) usbf_hid_handle,
                                               (NU_USB_USER *) NU_USBF_MSE_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        /* Register USB Function Mouse device with DM. */
        if(!rollback)
        {
            status = NU_USB_SYS_Register_Device((VOID *)NU_USBF_MSE_Cb_Pt,
                                                NU_USBCOMPF_MSE);
            if(status != NU_SUCCESS)
            {
                rollback = 4;
            }
        }

        /* Clean up in case of error. */
        switch (rollback)
        {
            case 4:
            	internal_sts = NU_USB_SYS_DeRegister_Device((VOID *)NU_USBF_MSE_Cb_Pt,
            			                                    NU_USBCOMPH_MSE );

            case 3:
                internal_sts |= NU_USB_DRVR_Deregister_User((NU_USB_DRVR *) usbf_hid_handle,
                                                           (NU_USB_USER *) NU_USBF_MSE_Cb_Pt);

            case 2:
                internal_sts |= _NU_USBF_MSE_Delete((VOID *) NU_USBF_MSE_Cb_Pt);

            case 1:
                if (NU_USBF_MSE_Cb_Pt)
                {
                    internal_sts |= USB_Deallocate_Memory((VOID *)NU_USBF_MSE_Cb_Pt);
                    NU_USBF_MSE_Cb_Pt = NU_NULL;
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
        if ((usbf_hid_handle) && (NU_USBF_MSE_Cb_Pt))
        {
            internal_sts |= NU_USB_SYS_DeRegister_Device((VOID *)NU_USBF_MSE_Cb_Pt, NU_USBCOMPF_MSE);
            internal_sts |= NU_USB_DRVR_Deregister_User((NU_USB_DRVR *) usbf_hid_handle,
                                                        (NU_USB_USER *) NU_USBF_MSE_Cb_Pt);
            internal_sts |= _NU_USBF_MSE_Delete(NU_USBF_MSE_Cb_Pt);
            internal_sts |= USB_Deallocate_Memory(NU_USBF_MSE_Cb_Pt);
            NU_USBF_MSE_Cb_Pt = NU_NULL;
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
*       NU_USBF_MSE_Create
*
* DESCRIPTION
*
*       HID User Driver initialization routine
*
* INPUTS
*       cb              Pointer to HID user driver control block.
*       name            Name of this USB object.
*
* OUTPUTS
*
*       NU_SUCCESS      Successful Initialization
*
*************************************************************************/
STATUS NU_USBF_MSE_Create(NU_USBF_MSE   *cb,char *name)
{
    STATUS status = NU_SUCCESS;
    UNSIGNED    TimerID = 0;
    UNSIGNED    Initial_Time = 100;
    UNSIGNED    Reschedule_Time = 100;
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
        memset(cb,0,sizeof(NU_USBF_MSE));

        /* Call the create function of base function user. */
        status = _NU_USBF_USER_Create ((NU_USBF_USER *) cb,"mouse",
                             MSE_SUBCLASS,MSE_PROTOCOL,&usbf_mse_dispatch);

    }

    if(status == NU_SUCCESS)
    {
        /* Create the timer which is used for calculating the time
        interval between recurring reports sent on the interrupt IN pipe.
        Initially Timer is disabled. */


        status = NU_Create_Timer(&cb->Idle_Timer,"Timer",
                                NU_USBF_MSE_Timer_expiration_routine,
                                TimerID,Initial_Time,
                                Reschedule_Time,
                                NU_DISABLE_TIMER);

        if(status == NU_SUCCESS)
            /* Create an event group which will be used for connection
            event notification */
            status = NU_Create_Event_Group(
                     (NU_EVENT_GROUP *)&cb->device_connect_event,
                     "DevEvent");

        /* Create semaphore for connection synchronization. */
        if(status == NU_SUCCESS)
            status = NU_Create_Semaphore(
                     (NU_SEMAPHORE  *)&cb->connection_lock,
                     "ConnLCK", 1, NU_FIFO);

        /* Add mouse function with USB device configuration layer. */
        if(status == NU_SUCCESS)
        {
            /* Add descriptor for FULL and HIGH speed. */
            status = USBF_DEVCFG_Add_Function(USBF_DEF_CONFIG_INDEX,
                                              fs_hs_mse_intf_desc,
                                              sizeof(fs_hs_mse_intf_desc),
                                              fs_hs_mse_intf_desc,
                                              sizeof(fs_hs_mse_intf_desc),
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                              ss_mse_intf_desc,
                                              sizeof(ss_mse_intf_desc),
#endif
                                              &cb->mse_function);

            if ( status == NU_SUCCESS )
            {
                /* Create and initialize mouse report descriptor. */
                status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                             MUS_REPORT_DESCRIPTOR_SIZE,
                                             (VOID**) &(cb->mse_report_descriptor));
                if ( status == NU_SUCCESS )
                {
                    /* Reset contents to zero. */
                    memset(cb->mse_report_descriptor, 0x00, MUS_REPORT_DESCRIPTOR_SIZE);

                    /* Initialize mouse report structures. */
                    /*Usage_Page (Generic Desktop)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x05; cb->mse_report_descriptor[rpt_idx++] = 0x01;
                    cb->mse_report_descriptor[rpt_idx++] = 0x09; cb->mse_report_descriptor[rpt_idx++] = 0x02;   /* Usage (Mouse)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0xA1; cb->mse_report_descriptor[rpt_idx++] = 0x01;   /* Collection (Application)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x09; cb->mse_report_descriptor[rpt_idx++] = 0x01;   /* Usage(Pointer)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0xA1; cb->mse_report_descriptor[rpt_idx++] = 0x00;   /* Collection (Physical)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x05; cb->mse_report_descriptor[rpt_idx++] = 0x09;   /* Usage Page(Button)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x19; cb->mse_report_descriptor[rpt_idx++] = 0x01;   /* Usage Minimum (Button 1)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x29; cb->mse_report_descriptor[rpt_idx++] = 0x03;   /* Usage Maximum (Button 3)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x15; cb->mse_report_descriptor[rpt_idx++] = 0x00;    /* Logical Minimum (0)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x25; cb->mse_report_descriptor[rpt_idx++] = 0x01;    /* Logical Maximum (1)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x95; cb->mse_report_descriptor[rpt_idx++] = 0x03;    /* Report Count(3)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x75; cb->mse_report_descriptor[rpt_idx++] = 0x01;    /* Report Size(1) */
                    cb->mse_report_descriptor[rpt_idx++] = 0x81; cb->mse_report_descriptor[rpt_idx++] = 0x02;    /* Input (Variable)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x95; cb->mse_report_descriptor[rpt_idx++] = 0x01;    /* Report Count (1) */
                    cb->mse_report_descriptor[rpt_idx++] = 0x75; cb->mse_report_descriptor[rpt_idx++] = 0x05;    /* Report Size  (5) */
                    cb->mse_report_descriptor[rpt_idx++] = 0x81; cb->mse_report_descriptor[rpt_idx++] = 0x03;    /* Input (Constant,Variable)*/
                    /* Usage Page (Generic Desktop)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x05; cb->mse_report_descriptor[rpt_idx++] = 0x01;
                    cb->mse_report_descriptor[rpt_idx++] = 0x09; cb->mse_report_descriptor[rpt_idx++] = 0x30;    /* Usage X*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x09; cb->mse_report_descriptor[rpt_idx++] = 0x31;    /* Usage Y*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x15; cb->mse_report_descriptor[rpt_idx++] = 0x81;    /* Logical Minimum (-127)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x25; cb->mse_report_descriptor[rpt_idx++] = 0x7F;    /* Logical Maximum (127)*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x75; cb->mse_report_descriptor[rpt_idx++] = 0x08;    /* Report Size*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x95; cb->mse_report_descriptor[rpt_idx++] = 0x02;    /* Report Count*/
                    cb->mse_report_descriptor[rpt_idx++] = 0x81; cb->mse_report_descriptor[rpt_idx++] = 0x06;    /* Input*/
                    cb->mse_report_descriptor[rpt_idx++] = 0xC0;                                                 /* End Collection*/
                    cb->mse_report_descriptor[rpt_idx++] = 0xC0;                                                 /* End Collection*/
                }
            }
        }
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBF_MSE_Delete
*
* DESCRIPTION
*       This function deletes an instance of HID User driver.
*
* INPUTS
*       cb              Pointer to the USB Object control block.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USBF_MSE_Delete (VOID    *cb)
{

    STATUS status;
    NU_USBF_MSE *mse = (NU_USBF_MSE *)cb;

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
            /* Delete the components created in the create function*/
            status = USB_Deallocate_Memory(mse->mse_report_descriptor);
            status = NU_Delete_Timer(&mse->Idle_Timer);
            if(status == NU_SUCCESS)
                status = NU_Delete_Event_Group(&mse->device_connect_event);
            if(status == NU_SUCCESS)
                status = NU_Delete_Semaphore(&mse->connection_lock);
        }
    }

    return status;
}/* _NU_USBF_MSE_Delete */

/*************************************************************************
* FUNCTION
*       _NU_USBF_MSE_Disconnect
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
STATUS _NU_USBF_MSE_Disconnect (NU_USB_USER * cb,
                                NU_USB_DRVR * class_driver,
                                void *handle)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_MSE *mse = (NU_USBF_MSE *)cb;

    if((cb == NU_NULL) || (class_driver == NU_NULL))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Clear the data values. */
        mse->drvr = NU_NULL;
        mse->handle = NU_NULL;
    }
    return status;
}/* _NU_USBF_MSE_Disconnect */

/*************************************************************************
* FUNCTION
*       _NU_USBF_MSE_Connect
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
STATUS _NU_USBF_MSE_Connect (NU_USB_USER    *cb,
                             NU_USB_DRVR *  class_driver,
                             void *handle)
{

    STATUS status = NU_SUCCESS;

    NU_USBF_MSE *mse = (NU_USBF_MSE *)cb;

    if((cb == NU_NULL) || (class_driver == NU_NULL))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Initialize the driver's data to default values. */
        mse->drvr = class_driver;
        mse->handle = (USBF_HID_DEVICE*)handle;

        /* Set device connection event in the event group */
        status = NU_Set_Events(&mse->device_connect_event,
                               HIDF_DEVICE_CONNECT,NU_OR);

    }
    return status;

}/* _NU_USBF_MSE_Connect    */


/*************************************************************************
* FUNCTION
*
*       NU_USBF_MSE_New_Command
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
STATUS _NU_USBF_MSE_New_Command (NU_USBF_USER * cb,
                                 NU_USBF_DRVR * drvr,
                                 VOID *handle,
                                 UINT8 *command,
                                 UINT16 cmd_len,
                                 UINT8 **data_out, UINT32 *data_len_out)
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
        status = NU_USBF_MSE_Process_Current_Request((NU_USBF_MSE   *)cb,
                                                      rcvd_cmd,
                                                      data_out,
                                                      data_len_out);

    }

    return status;
}/* _NU_USBF_MSE_New_Command    */
/*************************************************************************
* FUNCTION
*
*       NU_USBF_MSE_New_Transfer
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
*       NU_SUCCESS      indicates that the function has executed
*                       successfully
*
*       NU_USB_INVLD_ARG
*                       indicates a unexpected transfer request from
*                       the Host.
*
*       NU_USB_NOT_SUPPORTED
*                       indicates that the new transfer requests are
*                       not supported by the user.
*
*************************************************************************/
STATUS _NU_USBF_MSE_New_Transfer(NU_USBF_USER * cb,
                                 NU_USBF_DRVR * drvr,
                                 VOID *handle,
                                 UINT8  **data_out,
                                 UINT32 *data_len_out)
{

    return(NU_SUCCESS);

}/* _NU_USBF_MSE_New_Transfer */
/*************************************************************************
* FUNCTION
*
*       NU_USBF_MSE_Notify
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
*       cb              User control block for which the event is meant for
*
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
STATUS _NU_USBF_MSE_Notify (NU_USBF_USER    * cb,
                            NU_USBF_DRVR * drvr,
                            void *handle,
                            UINT32 event)
{
    return (NU_SUCCESS);
}/* NU_USBF_MSE_Notify */


/*************************************************************************
* FUNCTION
*
*       NU_USBF_MSE_Tx_Done
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
*       handle          Identifies for the logical function to which the
*                       notification is directed to.
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
*                       indicates an unexpected transfer completion.
*
*       NU_USB_NOT_SUPPORTED
*                       indicates that the transfer completion notification
*                       are not supported by the user.
*
*************************************************************************/
STATUS _NU_USBF_MSE_Tx_Done (NU_USBF_USER * cb,
                             NU_USBF_DRVR * drvr,
                             VOID *handle,
                             UINT8  *completed_data,
                             UINT32 completed_data_len,
                             UINT8  **data_out,
                             UINT32 *data_len_out)
{

    STATUS status;
    UINT32 tx_handle;
    NU_USBF_MSE *hid = (NU_USBF_MSE *)cb;


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
            /* Set Data Sent Event */
            status = NU_Set_Events(&hid->device_connect_event,
                                   HIDF_DATA_SENT,
                                   NU_OR);
        }

    }

    return (status);
}/* _NU_USBF_MSE_Tx_Done*/



/*************************************************************************
*   FUNCTION
*
*   NU_USBF_MSE_Send_Lft_Btn_Click
*
*   DESCRIPTION
*
*       This function is called when the application clicks left button of
*       mouse.It fills up the report data and passes it to the  host on
*       the interrupt in pipe.
*
*
*   INPUTS
*
*       cb              User control block
*
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates that the function has executed
*                           successfully.
*
*
*************************************************************************/
STATUS  NU_USBF_MSE_Send_Lft_Btn_Click(NU_USBF_MSE      *cb)
{
    STATUS  status = NU_SUCCESS;
    UINT32  event = 0;
    UINT8  *buffer = NU_NULL;
    UINT32 length = MSE_REPORT_LEN;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);

    /* Allocate memory for mouse report. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 MSE_REPORT_LEN,
                                 (VOID**) &(buffer));
    if ( status == NU_SUCCESS )
    {
        buffer[0] = 0x01;
        buffer[1] = 0x00;
        buffer[2] = 0x00;

        /* Call class driver function to send data */
        status = NU_USBF_HID_Send_Report((NU_USBF_HID*)cb->drvr,
                                          buffer,
                                          length,
                                          cb->handle);

        if(status == NU_SUCCESS)
        {
            /* Wait for the completion of data transfer. */
            status = NU_Retrieve_Events(&cb->device_connect_event,
                                        HIDF_DATA_SENT,NU_AND_CONSUME,
                                        &event,
                                        NU_SUSPEND);
        }

        buffer[0] = 0x00;
        buffer[1] = 0x00;
        buffer[2] = 0x00;

        status = NU_USBF_HID_Send_Report((NU_USBF_HID*)cb->drvr,
                                          buffer,
                                          length,
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

    if ( buffer )
    {
        USB_Deallocate_Memory(buffer);
        buffer = NU_NULL;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*   FUNCTION
*
*   NU_USBF_MSE_Send_Rht_Btn_Click
*
*   DESCRIPTION
*
*       This function is called when the application clicks right button
*       of mouse. It fills up the report data and passes it to the host
*       on the interrupt in pipe.
*
*
*   INPUTS
*
*       cb              User control block
*
*   OUTPUTS
*
*       NU_SUCCESS      indicates that the function has executed
*                       successfully.
*
*
*************************************************************************/
STATUS  NU_USBF_MSE_Send_Rht_Btn_Click(NU_USBF_MSE      *cb)
{
    STATUS  status = NU_SUCCESS;
    UINT32  event = 0;
    UINT8   *buffer = NU_NULL;
    UINT32  length = MSE_REPORT_LEN;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);

    /* Allocate memory for mouse report. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 MSE_REPORT_LEN,
                                 (VOID**) &(buffer));
    if(status == NU_SUCCESS)
    {
        buffer[0] = 0x02;
        buffer[1] = 0x00;
        buffer[2] = 0x00;

        status = NU_USBF_HID_Send_Report((NU_USBF_HID*)cb->drvr,
                                          buffer,
                                          length,
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
        buffer[0] = 0x00;
        buffer[1] = 0x00;
        buffer[2] = 0x00;

        status = NU_USBF_HID_Send_Report((NU_USBF_HID*)cb->drvr,
                                          buffer,
                                          length,
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

    if ( buffer )
    {
        USB_Deallocate_Memory(buffer);
        buffer = NU_NULL;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    return status;

}
/*************************************************************************
*   FUNCTION
*
*   NU_USBF_MSE_Send_Mdl_Btn_Click
*
*   DESCRIPTION
*
*       This function is called when the application clicks middle button
*       of mouse.It fills up the report data and passed it to the host on
*       the interrupt in pipe.
*
*
*   INPUTS
*       cb              User control block
*
*
*   OUTPUTS
*       NU_SUCCESS      Indicates that the function has executed
*                       successfully.
*
*
*
*************************************************************************/
STATUS  NU_USBF_MSE_Send_Mdl_Btn_Click(NU_USBF_MSE      *cb)
{
    STATUS  status = NU_SUCCESS;
    UINT32  event = 0;
    UINT8   *buffer = NU_NULL;
    UINT32  length = MSE_REPORT_LEN;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);

    /* Allocate memory for mouse report. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 MSE_REPORT_LEN,
                                 (VOID**) &(buffer));
    if(status == NU_SUCCESS)
    {
        buffer[0] = 0x04;
        buffer[1] = 0x00;
        buffer[2] = 0x00;

        /* Send data to the class driver */
        status = NU_USBF_HID_Send_Report((NU_USBF_HID*)cb->drvr,
                                          buffer,
                                          length,
                                          cb->handle);

        if(status == NU_SUCCESS)
        {
            /* Wait for the completion of data transfer. */
            status = NU_Retrieve_Events(&cb->device_connect_event,
                                        HIDF_DATA_SENT,
                                        NU_AND_CONSUME,
                                        &event, NU_SUSPEND);
        }
        buffer[0] = 0x00;
        buffer[1] = 0x00;
        buffer[2] = 0x00;

        status = NU_USBF_HID_Send_Report((NU_USBF_HID*)cb->drvr,
                                          buffer,
                                          length,
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

    if ( buffer )
    {
        USB_Deallocate_Memory(buffer);
        buffer = NU_NULL;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    return status;

}
/*************************************************************************
*   FUNCTION
*
*   NU_USBF_MSE_Move_Pointer
*
*   DESCRIPTION
*
*       This function is called when the application sends a mouse move
*       command to the user.It fills up the report data and passes it to
*       the host on the interrupt in pipe.
*
*
*   INPUTS
*
*       cb              User control block
*       X               Movement in X direction.
*       Y               Movement in Y direction.
*
*
*   OUTPUTS
*
*       NU_SUCCESS      Indicates that the function has executed
*                       successfully.
*
*************************************************************************/
STATUS  NU_USBF_MSE_Move_Pointer(NU_USBF_MSE    *cb,UINT8 X,UINT8 Y)
{
    STATUS  status;
    UINT32  event;
    UINT8   *buffer = NU_NULL;
    UINT32  length = MSE_REPORT_LEN;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);

    /* Allocate memory for mouse report. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 MSE_REPORT_LEN,
                                 (VOID**) &(buffer));
    if ( status == NU_SUCCESS )
    {
        memset(buffer, 0x00, MSE_REPORT_LEN);

        buffer[0] = 0x00;
        buffer[1] += X;
        buffer[2] += Y;

        status = NU_USBF_HID_Send_Report((NU_USBF_HID*)cb->drvr,
                                          buffer,
                                          length,
                                          cb->handle);

        if(status == NU_SUCCESS)
        {
            /* Wait for the completion of data transfer. */
            status = NU_Retrieve_Events(&cb->device_connect_event,
                                        HIDF_DATA_SENT,NU_AND_CONSUME,
                                        &event, NU_SUSPEND);
        }
    }

    if ( buffer )
    {
        USB_Deallocate_Memory(buffer);
        buffer = NU_NULL;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
* FUNCTION
*       NU_USBF_MSE_Wait
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
*       service is called again from the same thread, It checks for the
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
*       NU_USB_INTERNAL_ERROR   Indicates a internal error in USB subsystem
*
*************************************************************************/
STATUS NU_USBF_MSE_Wait (NU_USBF_MSE * cb,
                         UNSIGNED   suspend,
                         VOID   **handle_out)
{

    STATUS status;
    UINT32 event;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Validate control block pointer*/
    NU_ASSERT(cb);

    /* Obtains an instance of semaphore*/
    status = NU_Obtain_Semaphore((NU_SEMAPHORE  *)&cb->connection_lock,
                                  suspend);


    if(status == NU_SUCCESS)
    {
        /* Task is suspended until we retrieve the device connection event */
        status = NU_Retrieve_Events(&cb->device_connect_event,
                                    HIDF_DEVICE_CONNECT,
                                    NU_AND, &event, suspend);

    /* In case of error release the semaphore */
        if(status != NU_SUCCESS)
        {
            status = NU_Release_Semaphore(
                            (NU_SEMAPHORE *)&cb->connection_lock);
        }
        else
        {
            /* Validate the device handle in the user control block*/
            NU_ASSERT(cb->handle);

            /* Pass it to the calling module */
            *handle_out = cb->handle;

            /* Release semaphore */
            status = NU_Release_Semaphore((NU_SEMAPHORE *)&cb->connection_lock);
        }
    }
    /* Revert back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_MSE_Init_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the function mouse
*       user driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the user
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a function mouse
*                           user driver.
*       NU_NOT_PRESENT      Indicates there exists no user driver.
*
*************************************************************************/
STATUS NU_USBF_MSE_Init_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBF_MSE_Cb_Pt;
    if (NU_USBF_MSE_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_MSE_USER_Bind_Interface
*
* DESCRIPTION
*
*       This function actually binds HID mouse function with USB device
*       configuration. It is necessary that mouse function is
*       already registered with USB device configuration glue layer.
*
* INPUTS
*
*       cb                      Pointer to mouse function driver
*                               control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that mouse function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_MSE_USER_Bind_Interface(NU_USBF_MSE *cb)
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
        if ( cb->mse_function != NU_NULL )
        {
            status = NU_USB_DEVICE_Set_bDeviceClass(cb->mse_function->device,
                                                    HIDF_CLASS);
            if ( status == NU_SUCCESS )
            {
                /* Enable mouse function. */
                status = USBF_DEVCFG_Enable_Function(cb->mse_function);
                if ( status == NU_SUCCESS )
                {
                    /* Bind mouse function with USB device configuration. */
                    status = USBF_DEVCFG_Bind_Function(cb->mse_function);
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
*        NU_USBF_MSE_USER_Unbind_Interface
*
* DESCRIPTION
*
*       This function actually unbinds HID mouse function with USB
*       device configuration.
*       It is necessary that mouse function is already registered
*       with USB device configuration glue layer.
*
* INPUTS
*
*       cb                      Pointer to mouse function driver
*                               control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that mouse function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_MSE_USER_Unbind_Interface(NU_USBF_MSE *cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;
    if ( cb != NU_NULL )
    {
        status = NU_NOT_PRESENT;
        if ( cb->mse_function != NU_NULL )
        {
            status = USBF_DEVCFG_Unbind_Function(cb->mse_function);
        }
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    return ( status );
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
STATUS  NU_USBF_MSE_DM_Open (VOID* dev_handle)
{
    STATUS      status;
    NU_USBF_MSE *mse_cb;

    mse_cb = (NU_USBF_MSE *) dev_handle;
    status = NU_USBF_MSE_USER_Bind_Interface(mse_cb);

    return (status) ;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_MSE_DM_Close
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
STATUS  NU_USBF_MSE_DM_Close(VOID* dev_handle)
{
    STATUS      status;
    NU_USBF_MSE *mse_cb;

    mse_cb = (NU_USBF_MSE *) dev_handle;
    status = NU_USBF_MSE_USER_Unbind_Interface(mse_cb);

    return (status) ;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_MSE_DM_Read
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
STATUS  NU_USBF_MSE_DM_Read(VOID*    dev_handle,
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
*       NU_USBF_MSE_DM_Write
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
STATUS  NU_USBF_MSE_DM_Write(VOID*       dev_handle,
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
*       NU_USBF_MSE_DM_IOCTL
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
**************************************************************************/
STATUS  NU_USBF_MSE_DM_IOCTL(VOID*     dev_handle,
                             INT       ioctl_num,
                             VOID*     ioctl_data,
                             INT       ioctl_data_len)
{
    STATUS                  status;
    NU_USBF_MSE            *mse_cb = (NU_USBF_MSE *)dev_handle;
    NU_USBF_HID            *usbf_hid_pt = NU_NULL;
    USBF_MSE_WAIT_DATA     *mse_wait_data;
    NU_USBF_MSE_REPORT     *mse_report;

    switch(ioctl_num)
    {
        /******************************** *
         * USB Function Mouse User IOCTLS *
         ******************************** */
        case (USB_MSE_IOCTL_BASE + NU_USBF_MSE_IOCTL_WAIT):
            mse_wait_data = (USBF_MSE_WAIT_DATA *)ioctl_data;
            status = NU_USBF_MSE_Wait(mse_cb,
                                     mse_wait_data->suspend,
                                     mse_wait_data->handle_out);

        case (USB_MSE_IOCTL_BASE + NU_USBF_MSE_IOCTL_SEND_LFT_BTN_CLICK):
            status = NU_USBF_MSE_Send_Lft_Btn_Click(mse_cb);
            break;

        case (USB_MSE_IOCTL_BASE + NU_USBF_MSE_IOCTL_SEND_RHT_BTN_CLICK):
            status = NU_USBF_MSE_Send_Rht_Btn_Click(mse_cb);
            break;

        case (USB_MSE_IOCTL_BASE + NU_USBF_MSE_IOCTL_SEND_MDL_BTN_CLICK):
            status = NU_USBF_MSE_Send_Mdl_Btn_Click(mse_cb);
            break;

        case (USB_MSE_IOCTL_BASE + NU_USBF_MSE_IOCTL_MOVE_POINTER):
            mse_report = (NU_USBF_MSE_REPORT *)ioctl_data;
            status = NU_USBF_MSE_Move_Pointer(mse_cb,
                                              mse_report->MouseX,
                                              mse_report->MouseY);
            break;

        /************************* *
         * USB Function HID IOCTLS *
         ************************* */
        case (USB_MSE_IOCTL_BASE + NU_USBF_MSE_IOCTL_GET_HID_CB):
            status = NU_USBF_HID_Init_GetHandle((VOID **)ioctl_data);
            break;

        case (USB_MSE_IOCTL_BASE + NU_USBF_MSE_IOCTL_IS_HID_DEV_CONNECTED):
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
