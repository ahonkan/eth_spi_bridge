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
*       nu_usbf_hid_ext.c
*
* COMPONENT
*
*       Nucleus USB Function Software : HID Class Driver.
*
* DESCRIPTION
*       This file contains the external Interfaces exposed by
*       HID Class Driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       nu_os_conn_usb_func_hid_class_init  This function is used to
*                                           initialize the function HID
*                                           component.
*       NU_USBF_HID_Create                  Creates an instance of HID
*                                           Class Driver.
*       _NU_USBF_HID_Delete                 Deletes an instance of HID
*                                           Class Driver.
*       _NU_USBF_HID_Disconnect             Disconnect event handler for
*                                           the device.
*       _NU_USBF_HID_Initialize_Intf        Connection event handler for
*                                           the  interface.
*       _NU_USBF_HID_Examine_Intf           Notification for
*                                           SET_CONFIGURATION.
*       _NU_USBF_HID_New_Transfer           New transfer handling.
*       _NU_USBF_HID_New_Setup              Class specific request
*                                           handling.
*       _NU_USBF_HID_Set_Intf               New alternate setting
*                                           processing.
*       _NU_USBF_HID_Notify                 USB Events processing.
*       NU_USBF_HID_Send_Report             Sends report data to the class
*                                           driver.
*       NU_USBF_HID_Init_GetHandle          This function is used to
*                                           retrieve address of the
*                                           function HID Class driver.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ==============  USB Include Files =================================  */
#include    "connectivity/nu_usb.h"
#include    "services/reg_api.h"

/*********************************/
/* GLOBAL VARIABLES            */
/*********************************/
/* Global variables used for storing HID specific strings. */
CHAR    USBF_HID_Configuration_String[NU_USB_MAX_STRING_LEN];
CHAR    USBF_HID_Interface_String[NU_USB_MAX_STRING_LEN];

/* ==========================  Functions ============================== */

/*************************************************************************
*
*   FUNCTION
*
*       nu_os_conn_usb_func_hid_class_init
*
*   DESCRIPTION
*
*       This function initializes the function HID component.
*
*   INPUTS
*
*       path               Registry path of component.
*       startstop          Flag to find if component is being enabled or
*                          disabled.
*
*   OUTPUTS
*
*       status             Success or failure of creation of the
*                          underlying initialization routines.
*
*************************************************************************/
STATUS nu_os_conn_usb_func_hid_class_init(CHAR *path, INT startstop)
{
    STATUS  status, reg_status, internal_sts = NU_SUCCESS;
    VOID   *stack_handle = NU_NULL;
    CHAR    usb_func_hid_path[80];
    UINT8   rollback = 0;

    usb_func_hid_path[0] = '\0';
    strcat(usb_func_hid_path, path);

    /* Save registry settings of USB Function HID Class. */
    strcat(usb_func_hid_path, "/configstring");
    reg_status = REG_Get_String(usb_func_hid_path, USBF_HID_Configuration_String, NU_USB_MAX_STRING_LEN);
    if(reg_status == NU_SUCCESS)
    {
        usb_func_hid_path[0] = '\0';
        strcat(usb_func_hid_path, path);
        strcat(usb_func_hid_path, "/interfacestring");
        reg_status = REG_Get_String(usb_func_hid_path, USBF_HID_Interface_String, NU_USB_MAX_STRING_LEN);
    }

    if(startstop)
    {
        /* Allocate memory for USB function HID class driver. */
        status = USB_Allocate_Object(sizeof(NU_USBF_HID),
                                     (VOID **)&NU_USBF_HID_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        if(!rollback)
        {
            /* Zero out allocated block. */
            memset(NU_USBF_HID_Cb_Pt, 0, sizeof(NU_USBF_HID));

            /* Create the function HID Class driver. */
            status = NU_USBF_HID_Create(NU_USBF_HID_Cb_Pt, "USBFHID",
                                        NU_NULL);
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        /* Get the function stack handle. */
        if(!rollback)
        {
            status = NU_USBF_Init_GetHandle(&stack_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

         /* Register with the stack. */
        if(!rollback)
        {
            status = NU_USB_STACK_Register_Drvr((NU_USB_STACK *) stack_handle,
                                                (NU_USB_DRVR *) NU_USBF_HID_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        /* Clean up in case of error. */
        switch (rollback)
        {
            case 3:
                internal_sts = _NU_USBF_HID_Delete ((VOID *) NU_USBF_HID_Cb_Pt);

            case 2:
                if (NU_USBF_HID_Cb_Pt)
                {
                    internal_sts |= USB_Deallocate_Memory((VOID *)NU_USBF_HID_Cb_Pt);
                    NU_USBF_HID_Cb_Pt = NU_NULL;
                }

            case 1:
            case 0:
                break;
        }
    }
    else
    {
        /* Clean up. */
        status = NU_USBF_Init_GetHandle (&stack_handle);
        if((stack_handle) && (NU_USBF_HID_Cb_Pt))
        {
            internal_sts = NU_USB_STACK_Deregister_Drvr(stack_handle,
                                                        (NU_USB_DRVR *)NU_USBF_HID_Cb_Pt);
            internal_sts |= _NU_USBF_HID_Delete(NU_USBF_HID_Cb_Pt);
            internal_sts |= USB_Deallocate_Memory(NU_USBF_HID_Cb_Pt);
            NU_USBF_HID_Cb_Pt = NU_NULL;
        }
    }

    /* internal_sts is not used after this. So to remove
     * KW and PC-Lint warning set it as unused variable.
     */
    NU_UNUSED_PARAM(internal_sts);

    return status;
}

/*************************************************************************
* FUNCTION
*
*        NU_USBF_HID_Create
*
* DESCRIPTION
*
*       This function initializes the HID class driver. All the
*       internal data structures are initialized. Subsequently, users can
*       be registered with this class driver.
*
*       Once, initialized successfully, this class driver must be
*       registered with a stack. Together with a registered user, this
*       enables the HID functionality of the usb device. HID
*       class driver starts responding to the Host requests to
*       communicate with the HID device.
*
* INPUTS
*       cb          pointer to HID driver control block.
*       name        Name of this Class driver.
*       pool        Pointer to memory for the driver.
*
* OUTPUTS
*
*        NU_SUCCESS          Successful Initialization
*        NU_USB_INVLD_ARG    Indicates incorrect parameters.
*        NU_USB_NOT_PRESENT  Indicates a configuration problem because of
*                            which no more USB objects could be created.
*
*
*************************************************************************/
STATUS NU_USBF_HID_Create (NU_USBF_HID *cb, CHAR *name,
                           NU_MEMORY_POOL *pool)
{
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Check the input parameters */
    if((cb == NU_NULL) || (name == NU_NULL))
    {
        status = NU_USB_INVLD_ARG;
    }

    if (status == NU_SUCCESS)
    {
        /* Reset the control block */
        memset((VOID *)cb, 0, sizeof(NU_USBF_HID));

        cb->mem_pool = pool;

        /* Invoke the parents create function */
        status = _NU_USBF_DRVR_Create (&cb->parent,
                                       name,
                                      (USB_MATCH_CLASS),
                                       0,
                                       0,
                                       0,
                                       0,
                                      (HIDF_CLASS),
                                       0,
                                       0,
                                       &usbf_hid_dispatch);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();;

    return(status);
}


/*************************************************************************
* FUNCTION
*
*       _NU_USBF_HID_Delete
*
* DESCRIPTION
*      This function deletes an instance of HID Class driver.
*      Uninitializes the HID class driver.It disowns all the interfaces
*      being served by this driver.
*
*      Once uninitialized the HID functionality of the device
*      will be lost and the device stops responding to any of the Host
*      requests to communicate with the device.
*
* INPUTS
*       cb              pointer to the USB Object control block.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*
*************************************************************************/
STATUS _NU_USBF_HID_Delete (VOID *cb)
{
    STATUS status;

    /* Call the base driver's delete function. */
    status =  _NU_USBF_DRVR_Delete(cb);

    return status;
}
/*************************************************************************
* FUNCTION
*       _NU_USBF_HID_Initialize_Intf
*
* DESCRIPTION
*       Connect notification function invoked by stack when driver is given
*       an opportunity to own an Interface
*
* INPUTS
*
*       cb          Pointer to Class Driver Control Block.
*       stk         Pointer to Stack Control Block of the calling stack.
*       dev         Pointer to Device Control Block of the device found.
*       intf        Pointer to Interface control Block to be served by this
*                   class driver.
*
* OUTPUTS
*
*       NU_SUCCESS  Indicates successful completion of the service.
*
*************************************************************************/
STATUS _NU_USBF_HID_Initialize_Intf(NU_USB_DRVR * cb,
                                    NU_USB_STACK * stack,
                                    NU_USB_DEVICE * dev,
                                    NU_USB_INTF * intf)
{

    STATUS status = NU_SUCCESS;
    NU_USBF_HID *hid = (NU_USBF_HID*)cb;
    BOOLEAN isclaimed = NU_FALSE;
    NU_USB_DRVR *old;
    INT i;
    USBF_HID_DEVICE *device = NU_NULL;


    /* Release the interface if already claimed. */
    NU_USB_INTF_Get_Is_Claimed(intf,&isclaimed,&old);

    if(isclaimed)
    {
        NU_USB_INTF_Release(intf, old);
    }

    /* Claim the interface. */
    NU_USB_INTF_Claim (intf, cb);

    /* Save the status for current interface. */
    hid->temp_hid_dev[hid->hid_init_intf] = dev;
    hid->temp_hid_intf[hid->hid_init_intf] = intf;



    /* Find the empty slot amongst the device array*/
    for(i = 0; i < HIDF_MAX_DEVICES; i++)
    {
        if(hid->devices[i] == NU_NULL)
        {
            break;
        }
    }

    /* In case no device is found set the error */
    if(i == HIDF_MAX_DEVICES)
    {
        status = NU_USB_MAX_EXCEEDED;
    }
    else
    {
        /* Allocate new device structure. */
        status = USB_Allocate_Object(sizeof(USBF_HID_DEVICE),
                                     (VOID **)&device);

        if(status == NU_SUCCESS)
        {
            /* Store the device in the empty slot */
            memset(device, 0, sizeof(USBF_HID_DEVICE));
            hid->devices[i] = device;

            /* Initialize the device structure for the received values.*/
            device->hid_dev = hid->temp_hid_dev[hid->hid_init_intf] ;
            device->hid_intf = hid->temp_hid_intf[hid->hid_init_intf] ;
            device->hid_user = NU_NULL;
            device->drvr = (NU_USBF_DRVR *)hid;

            /* Allocate memory for command buffer. */
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                         HIDF_MAX_OUT_REPORT_LENGTH,
                                         (VOID**) &(device->cmd_buffer));
            if ( status == NU_SUCCESS )
            {
                status = _NU_USBF_HID_Set_Intf((NU_USB_DRVR *)hid,
                                        NU_NULL,
                                        device->hid_dev,
                                        device->hid_intf,
                                        &device->hid_intf->alt_settg[0]);
                if(status == NU_SUCCESS)
                {
                    hid->hid_init_intf++;
                }
            }

        }
    }
    return(status);
}


/*************************************************************************
* FUNCTION
*
*       _NU_USBF_HID_Disconnect
*
* DESCRIPTION
*
*       Disconnect Callback function, invoked by stack when a
*   Device/Interface served by HID is removed from the BUS.
*
* INPUTS
*
*       cb      Pointer to Class Driver Control Block claimed by this
*               Interface.
*       stack     Pointer to Stack Control Block.
*       device     Pointer to NU_USB_DEVICE Control Block disconnected.
*
* OUTPUTS
*       NU_SUCCESS       Indicates successful completion of the service.
*       NU_USB_INVLD_ARG Indicates invalid arguments.
*
*************************************************************************/
STATUS _NU_USBF_HID_Disconnect (NU_USB_DRVR * cb,
                                NU_USB_STACK * stack,
                                NU_USB_DEVICE * device)
{
    STATUS status;
    INT i;
    USBF_HID_DEVICE *dev;
    NU_USBF_HID* hid = (NU_USBF_HID*)cb;

    status = NU_SUCCESS;
    /* Iterate through the devices and find out the device
       for which the function is invoked */
    for(i = 0; i < HIDF_MAX_DEVICES; i++)
    {
        dev = hid->devices[i];

        if((dev != NU_NULL) && (dev->hid_dev == device))
        {
            hid->hid_init_intf--;
            USBF_HID_Handle_Disconnected(dev);
            hid->devices[i] = NU_NULL;
        }

    }

    return status;
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBF_HID_Set_Intf
*
* DESCRIPTION
*   Notifies driver of change in alternate setting.
*
* INPUTS
*
*       cb          Class driver for which this callback is meant for
*       stack       Stack invoking this function
*       device      Device on which this event has happened
*       intf        Interface which is affected
*       alt_settg   New alternate setting for the interface
*
*
* OUTPUTS
*
*       NU_SUCCESS  indicates that the class driver could process the
*                   event successfully.
*
*       NU_USB_INVLD_ARG    indicates that one of the input parameters has
*                           been incorrect AND/OR the event could not be
*                           processed without an error
*
*
*************************************************************************/
STATUS _NU_USBF_HID_Set_Intf (NU_USB_DRVR * cb,
                              NU_USB_STACK * stack,
                              NU_USB_DEVICE * device,
                              NU_USB_INTF * intf,
                              NU_USB_ALT_SETTG * alt_settg)
{
    STATUS status;
    INT i;
    NU_USBF_HID *hid = (NU_USBF_HID*)cb;
    USBF_HID_DEVICE *dev = NU_NULL;
    NU_USBF_HW* fc_ptr;
    UINT32 hw_capability = 0UL;

    fc_ptr = (NU_USBF_HW*) device->hw;


    /* Find device, for which callback is received. */
    for(i = 0; i < HIDF_MAX_DEVICES; i++)
    {
        dev = hid->devices[i];
        if((dev != NU_NULL) && (dev->hid_dev == device) &&
           (dev->hid_intf == intf))
        {
            break;
        }
    }

    /* In case no device is found report error */
    if(i == HIDF_MAX_DEVICES)
    {
        status = NU_NOT_PRESENT;
    }
    else
    {

        dev->hid_alt_set = alt_settg;

        /* Get Pipes */

        /* Find the default pipe */

        status = NU_USB_ALT_SETTG_Find_Pipe (alt_settg,
                                            USB_MATCH_EP_ADDRESS,
                                            0,
                                            0,
                                            0,
                                            &dev->control_pipe);

        if(status == NU_SUCCESS)
        {
            /* Find the Interrupt IN pipe  */

            status = NU_USB_ALT_SETTG_Find_Pipe (alt_settg,
                                             USB_MATCH_EP_TYPE |
                                             USB_MATCH_EP_DIRECTION,
                                             0,
                                             USB_DIR_IN,
                                             USB_EP_INTR,
                                             &dev->intr_in_pipe);


            if (status == NU_SUCCESS)
            {


                /*Find the optional Interrupt OUT pipe  */
                NU_USB_ALT_SETTG_Find_Pipe (alt_settg,
                                             USB_MATCH_EP_TYPE |
                                             USB_MATCH_EP_DIRECTION,
                                             1,
                                             USB_DIR_OUT,
                                             USB_EP_INTR,
                                             &dev->intr_out_pipe);
            }
        }

        if(status == NU_SUCCESS)
        {
            NU_USB_USER *prev_user;
            UNSIGNED num_users;
            NU_USB_USER *user_list[NU_USB_MAX_USERS];
            NU_USB_USER *user;
            UINT8 sub_class, protocol, intf_protocol, intf_sub_class;

            /* Give a disconnect callback to the user */
            prev_user = dev->hid_user;
            intf = dev->hid_intf;

            if(prev_user != NU_NULL)
                NU_USB_USER_Disconnect (prev_user,(NU_USB_DRVR*)hid,NU_NULL);


            /* Get number of registered users */
            num_users = NU_USB_DRVR_Get_Users((NU_USB_DRVR*)hid,&user_list[0],
                                               NU_USB_MAX_USERS);

            /* Get the interface descriptor and find out who is the user */
            /* go through all users to find who should be owning this */
            for(i = 0; i < num_users; i++)
            {
                if(i >= NU_USB_MAX_USERS)
                {
                    status = NU_NOT_PRESENT;
                    break;
                }
                user = user_list[i];

                status = NU_USB_USER_Get_Subclass(user, &sub_class);

                if (status != NU_SUCCESS)
                    continue;

                status = NU_USB_USER_Get_Protocol (user, &protocol);
                if (status != NU_SUCCESS)
                    continue;

                NU_USB_INTF_Get_SubClass (intf, &intf_sub_class);
                NU_USB_INTF_Get_Protocol( intf, &intf_protocol);

                if(    (sub_class == intf_sub_class)     &&
                    (protocol     == intf_protocol)    &&
                    (sub_class > 0))
                 {
                        /* Save user in the control block of device*/
                        dev->hid_user = user;
                        break;

                }
                else
                {
                    dev->hid_user = NU_NULL;
                }
            }

            /* In case no user is found report error status */
            if (i == num_users)
                status = NU_NOT_PRESENT;
           /*
            * Some boards implicitly handle the SET CONFIGURATION request (for
            * example the SH3DSP board with SH7727R2 controller). The
            * _NU_USBF_HID_Set_Intf function on such boards gets called even
            * when the target is not connected to the host. The function
            * _NU_USBF_HID_Notify is called on such boards upon connection.
            * In that case, the USBF_HID_Connect_User function should
            * be called from the _NU_USBF_HID_Notify function and not from the
            * _NU_USBF_HID_Set_Intf function, otherwise the user will get
            * notified of the connection prematurely.
            *
            * For boards that do not handle the SET CONFIGURATION request
            * implicitly, the _NU_USBF_HID_Set_Intf function gets called
            * when the target is connected to the host and hence the
            * USBF_HID_Connect_User has to be called from here.
            *
            * Here, we check for the hardware capabilities of the board to
            * see if the SET CONFIGURATION is being implicitly handled or not.
            * If not, this _NU_USBF_HID_Set_Intf function will be called
            * when the target is connected to the host and hence we call the
            * the USBF_HID_Connect_User function.
            */
            status = NU_USBF_HW_Get_Capability (fc_ptr, &hw_capability);

            if (status == NU_SUCCESS &&
              !(hw_capability & (0x01 << USB_SET_CONFIGURATION)))
            {
                status = USBF_HID_Connect_User(dev);
            }
        }
    }

    return status;
}

/*************************************************************************
* FUNCTION
*
*      _NU_USBF_HID_New_Setup
*
* DESCRIPTION
*
*       Processes a new class specific SETUP packet from the Host.
*
*       The format of the setup packet is defined by the corresponding
*       class specification / custom protocol. The setup packet is
*       validated by this function and processed further as per the class
*       specification.
*
*       If there is any data phase associated with the setup request,
*       then the NU_USB_Submit_Irp function can be used to submit the
*       transfer to the stack. Status phase is automatically handled by the
*       Stack. If there is no data transfer associated with the command,
*       then no transfer is submitted.
*
*       For unknown and unsupported command, this function returns
*       appropriate error status. If this function returns any status other
*       than NU_SUCCESS, then the default endpoint will be stalled.
*
* INPUTS
*
*       cb          Class driver for which this callback is meant for
*       stack       Stack invoking this function
*       device      Device on which this event has happened
*       setup       the 8 byte setup packet originating from the Host
*
* OUTPUTS
*
*       NU_SUCCESS  indicates that the class driver could process the
*                   event successfully.
*
*       NU_USB_INVLD_ARG    indicates the one of the input parameters has
*                           been incorrect AND/OR the event could not be
*                           processed without an error
*
*
*************************************************************************/
STATUS _NU_USBF_HID_New_Setup(NU_USB_DRVR * cb,
                              NU_USB_STACK * stack,
                              NU_USB_DEVICE * dev,
                              NU_USB_SETUP_PKT * setup)
{
    STATUS status = NU_USB_INVLD_ARG;
    NU_USBF_HID *hid = (NU_USBF_HID*)cb;
    USBF_HID_DEVICE *device = NU_NULL;
    UINT8 intf_num;
    INT i;


    /* Check for the device who is recipient for this setup. */
    for(i = 0; i < HIDF_MAX_DEVICES; i++)
    {
        device = hid->devices[i];
        if((device != NU_NULL) && (device->hid_dev == dev))
        {
            NU_USB_INTF_Get_Intf_Num(device->hid_intf, &intf_num);
            if(intf_num == (UINT8)setup->wIndex)
            {
                break;
            }
        }
    }

    /* If no device is found report the error status */
    if(i == HIDF_MAX_DEVICES)
    {
        status = NU_NOT_PRESENT;
    }
    else if(device)
    {
        /* Copy the setup packet to the control block's setup packet */
        memcpy(&device->setup_pkt,setup,sizeof(NU_USB_SETUP_PKT));

        /* Handle class specific request */
        status = USBF_HID_Handle_Class_Req_Rcvd(device);
    }
    return(status);
}
/*************************************************************************
* FUNCTION
*
*       _NU_USBF_HID_New_Transfer
*
* DESCRIPTION
*
*       Processes the new transfer request from the Host.
*
*       If there is any data transfer pending with the class driver,
*       then the NU_USB_Submit_Irp function can be used to submit the
*       transfer to the stack. If there is no data transfer associated
*       with the command, then no transfer is submitted.
*
*       This function is never invoked on default control endpoint. Since
*       stack ignores the return value of this function, any required error
*       processing is carried out by the class driver itself.
*
*
* INPUTS
*
*       cb          Class driver for which this callback is meant for
*       stack       Stack invoking this function
*       device      Device on which this event has happened
*       pipe        Pipe on which the data transfer is initiated by the
*                   Host
*
* OUTPUTS
*
*       NU_SUCCESS  indicates that the class driver could process the
*                   event successfully.
*
*       NU_USB_INVLD_ARG    indicates the one of the input parameters has
*                           been incorrect AND/OR the event could not be
*                           processed without an error.
*
*
*************************************************************************/
STATUS _NU_USBF_HID_New_Transfer(NU_USB_DRVR * cb,
                                  NU_USB_STACK * stack,
                                  NU_USB_DEVICE * device,
                                  NU_USB_PIPE * pipe)
{

    /* Fill your implementation here */

    return NU_SUCCESS;

}
/*************************************************************************
* FUNCTION
*
*       _NU_USBF_HID_Notify
*
* DESCRIPTION
*   Notifies Driver of USB Events.
*
* INPUTS
*
*       cb          Class driver for which this callback is meant for
*       stack       Stack invoking this function
*       device      Device on which this event has happened
*       event       USB event that has occurred
*
* OUTPUTS
*
*       NU_SUCCESS  indicates that the class driver could process the
*                   event successfully.
*
*       NU_USB_INVLD_ARG    indicates the one of the input parameters has
*                           been incorrect AND/OR the event could not be
*                           processed without an error
*
*************************************************************************/
STATUS _NU_USBF_HID_Notify(NU_USB_DRVR * cb,
                           NU_USB_STACK * stack,
                           NU_USB_DEVICE * dev,
                           UINT32 event)
{
    STATUS status = NU_SUCCESS;
    USBF_HID_DEVICE *device;
    NU_USBF_HID *hid = (NU_USBF_HID *)cb;
    NU_USBF_HW  *fc_ptr;
    UINT32 hw_capability = 0UL;
    INT i;

    fc_ptr = (NU_USBF_HW*)dev->hw;

     /* Check for the device who is recipient for this setup. */
    for(i = 0; i < HIDF_MAX_DEVICES; i++)
    {
        device = hid->devices[i];
        if((device != NU_NULL) && (device->hid_dev == dev))
        {
            switch(event)
            {
                case USBF_EVENT_RESET:
                {

                    if(device->hid_user!= NU_NULL)
                    {
                        /*
                        * Some boards implicitly handle the SET CONFIGURATION request
                        * (for example the SH3DSP board with SH7727R2 controller). The
                        * _NU_USBF_HID_Set_Intf function on such boards gets called
                        * even when the target is not connected to the host. The
                        * function_NU_USBF_HID_Notify is called on such boards upon
                        * connection. In that case, the USBF_HID_Connect_User function
                        * should be called from the _NU_USBF_HID_Notify function and
                        * not from the _NU_USBF_HID_Set_Intf function, otherwise the
                        * user will get notified of the connection prematurely.
                        *
                        * Here, we check for the hardware capabilities of the board to
                        * see if the SET CONFIGURATION is being implicitly handled or
                        * not. If this request is being handled implicitly, the
                        * _NU_USBF_HID_Notify function will notify about the device
                        * connection and we will call the USBF_HID_Connect_User
                        * function from here.
                        */
                        if(status == NU_SUCCESS)
                        {
                            status = NU_USBF_HW_Get_Capability (fc_ptr, &hw_capability);

                            if (status == NU_SUCCESS &&
                                (hw_capability & (0x01 << USB_SET_CONFIGURATION)))
                               {
                                   status = NU_USBF_USER_Notify (device->hid_user, (NU_USB_DRVR *)device->drvr,
                                    NU_NULL, device->usb_event);

                                   /* Flush all pipes. */
                                if(device->control_pipe)
                                {
                                    NU_USB_PIPE_Flush(device->control_pipe);
                                }

                                if(device->intr_in_pipe)
                                {
                                    NU_USB_PIPE_Flush(device->intr_in_pipe);
                                }
                                if(device->intr_out_pipe)
                                {
                                    NU_USB_PIPE_Flush(device->intr_out_pipe);
                                }

                                status = USBF_HID_Connect_User(device);

                                break;

                            }
                            else
                            {
                                /* Notify current event to the upper layers.
                                */

                                /* Save the received event in driver's data. */
                                device->usb_event = event;
                                hid->hid_init_intf--;
                                USBF_HID_Handle_USB_Event_Rcvd(device);
                                hid->devices[i] = NU_NULL;
                                break;
                            }
                        }
                    }
                }

                case USBF_EVENT_CONNECT:
                {
                    /* Notify current event to the upper layers.
                    */
                    /* Save the received event in driver's data. */
                    device->usb_event = event;
                    USBF_HID_Handle_USB_Event_Rcvd(device);
                    break;
                }
                case USBF_EVENT_DISCONNECT:
                {

                    /* Notify current event to the upper layers.
                     */

                    /* Save the received event in driver's data. */
                    device->usb_event = event;
                    hid->hid_init_intf--;
                    USBF_HID_Handle_USB_Event_Rcvd(device);
                    hid->devices[i] = NU_NULL;
                    break;
                }

                /* Just ignore all other events. */
                case USBF_EVENT_STACK_SHUTDOWN:
                case USBF_EVENT_SUSPEND:
                case USBF_EVENT_RESUME:
                default:
                    break;

            }
        }
    }
    return status;

}
/*************************************************************************
* FUNCTION
*
*       NU_USBF_HID_Send_Report
*
* DESCRIPTION
*       Sends the HID report to Host through interrupt endpoint.
*
* INPUTS
*
*       hid         Pointer to HID class driver control block
*       report      Buffer containing the report data
*       len         Length of event data.
*       handle      Pointer to the device.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates that the class driver could process
*                           the event successfully.
*
*       NU_USB_INVLD_ARG    Indicates that one of the input parameters has
*                           been incorrect AND/OR the event could not be
*                           processed without an error
*************************************************************************/
STATUS NU_USBF_HID_Send_Report(NU_USBF_HID *hid, UINT8 *report,
                               UINT32 len,
                               VOID *handle)
{
    STATUS status;
    USBF_HID_DEVICE *device;
    UINT16 intr_maxp;
    NU_USB_ENDP *intr_endp;
    NU_USB_IRP  *report_irp;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    if((hid == NULL) || (handle == NULL))
        status = NU_USB_INVLD_ARG;
    else
    {
        device = (USBF_HID_DEVICE   *)handle;
        device->report_data = report;
        device->report_len = len;

        report_irp  = &device->intr_irp;

        /* Create the IRP for this data transfer */
        status = NU_USB_IRP_Create(report_irp,
                                   device->report_len,
                                   device->report_data,
                                   NU_TRUE,
                                   NU_FALSE,
                                   USBF_HID_Interrupt_Data_IRP_Cmplt,
                                   (VOID *)device,
                                   0);

        if(status == NU_SUCCESS)
        {
            device->report_tx_state = 0;

            if(status == NU_SUCCESS)
            {
                /* If transfer length is whole number multiple of
                * endpoint's maxp, update the flag to send zero length
                  after the transfer.*/
                status = NU_USB_PIPE_Get_Endp(device->intr_in_pipe,
                                              &intr_endp);

                if(status == NU_SUCCESS)
                {
                    status = NU_USB_ENDP_Get_Max_Packet_Size(intr_endp,
                                                         &intr_maxp);
                    if(status == NU_SUCCESS)
                    {
                        if((device->report_len > 0) &&
                        (device->report_len % intr_maxp == 0))
                        {
                            device->report_tx_state = HIDF_SEND_ZERO_LEN;
                        }
                    }
                }
            }

            /* Submit the IRP on the interrupt pipe. */
            status = NU_USB_PIPE_Submit_IRP(device->intr_in_pipe,
                                            report_irp);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}   /* NU_USBF_HID_Send_Report */

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_HID_Init_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the function HID
*       class driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the class
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a function HID
*                           class driver.
*       NU_NOT_PRESENT      Indicates there exists no class driver.
*
*************************************************************************/
STATUS NU_USBF_HID_Init_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBF_HID_Cb_Pt;

    if (NU_USBF_HID_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/* ======================  End Of File  =============================== */
