/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       nu_usbf_comm_data_ext.c
*
* COMPONENT
*
*       Nucleus USB Function Software : Data Interface Class Driver.
*
* DESCRIPTION
*
*       This file contains the external Interfaces exposed by
*       Data Interface Class Driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       NU_USBF_COMM_DATA_Init              Initializes the component of
*                                           Data Interface Class Driver.
*       NU_USBF_COMM_DATA_GetHandle         Used to retrieve address of
*                                           Data Interface Class Driver.
*       NU_USBF_COMM_DATA_Create            Creates an instance of Data
*                                           Interface Class Driver.
*       _NU_USBF_COMM_DATA_Delete           Deletes an instance of Data
*                                           Interface Class Driver.
*       _NU_USBF_COMM_DATA_Init_Intf        Connection event handler for
*                                           the interface.
*       _NU_USBF_COMM_DATA_Disconnect       Disconnection event handle of
*                                           Data Interface class driver.
*       _NU_USBF_COMM_DATA_Set_Intf         Set_Intf request handler for
*                                           driver.
*       _NU_USBF_COMM_DATA_New_Setup        Class specific request handler.
*       _NU_USBF_COMM_DATA_New_Transfer     New transfer handler.
*       _NU_USBF_COMM_DATA_Notify           USB Event processing routine.
*       NU_USBF_COMM_DATA_Send              Data transmission routine.
*       NU_USBF_COMM_DATA_Get_Rcvd          Received data parameters
*                                           retrieval routine.
*       NU_USBF_COMM_DATA_Reg_Rx_Buffer     New receive buffer registration
*                                           routine.
*       NU_USBF_COMM_DATA_Config_Xfers      Data transfer configuration
*                                           routine.
*       NU_USBF_COMM_DATA_Dis_Reception     Data receive disabling routine.
*       NU_USBF_COMM_DATA_Rbg_Create        Receive buffer group
*                                           initialization.
*       NU_USBF_COMM_DATA_Cancel_Io         Cancels ongoing IO on data
*                                           interface.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions
*       runlevel_init.h
*
*************************************************************************/

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb.h"
#include "services/runlevel_init.h"

/* ==========================  Functions ============================== */

/*************************************************************************
* FUNCTION
*
*       NU_USBF_COMM_DATA_Init
*
* DESCRIPTION
*
*       Function Communication Class Driver Data Component initialization
*
* INPUTS
*
*       path                                Registry path of component.
*       startstop                           Flag to find if component is
*                                           being enabled or disabled.
*
* OUTPUTS
*
*       status          Status of initialization (NU_SUCCESS when successful)
*
*************************************************************************/
STATUS nu_os_conn_usb_func_comm_data_init(CHAR *path, INT startstop)
{
    VOID   *stack_handle = NU_NULL;
    UINT8   rollback = 0;
    STATUS  status = NU_SUCCESS, internal_sts = NU_SUCCESS;

    if (startstop == RUNLEVEL_START)
    {
        /* Allocate Memory for the Function COMM Data component control block. */
        status = USB_Allocate_Object(sizeof(NU_USBF_COMM_DATA),
                                     (VOID**)&NU_USBF_COMM_DATA_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        if (!rollback)
        {
            /* Zero out the memory allocated for Function COMM Data control
             * block.
             */
            memset (NU_USBF_COMM_DATA_Cb_Pt, 0, sizeof(NU_USBF_COMM_DATA));
            status = NU_USBF_COMM_DATA_Create(NU_USBF_COMM_DATA_Cb_Pt,
                                              "USBF-COMM-DATA");
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        /* Get the function stack handle */
        if (!rollback)
        {
            status = NU_USBF_Init_GetHandle (&stack_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        if (!rollback)
        {
           status = NU_USBF_STACK_Register_Drvr ((NU_USBF_STACK *) stack_handle,
                                                (NU_USB_DRVR *) NU_USBF_COMM_DATA_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        /* Clean up in case error occurs. */
        switch (rollback)
        {
            case 3:
                internal_sts = _NU_USBF_COMM_DATA_Delete ((VOID *) NU_USBF_COMM_DATA_Cb_Pt);

            case 2:
                if (NU_USBF_COMM_DATA_Cb_Pt)
                {
                    internal_sts |= USB_Deallocate_Memory((VOID *)NU_USBF_COMM_DATA_Cb_Pt);
                    NU_USBF_COMM_DATA_Cb_Pt = NU_NULL;
                }

            case 1:
            case 0:
            /* internal_sts is not used after this. So to remove
             * KW and PC-Lint warning set it as unused variable.
             */
            NU_UNUSED_PARAM(internal_sts);
        }
    }
    else if (startstop == RUNLEVEL_STOP)
    {
        status = NU_USBF_Init_GetHandle (&stack_handle);

        if (status == NU_SUCCESS)
        {
            NU_USB_STACK_Deregister_Drvr(stack_handle, (NU_USB_DRVR *)NU_USBF_COMM_DATA_Cb_Pt);
            _NU_USBF_COMM_DATA_Delete(NU_USBF_COMM_DATA_Cb_Pt);
            USB_Deallocate_Memory(NU_USBF_COMM_DATA_Cb_Pt);
        }

        status = NU_SUCCESS;
    }

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_COMM_DATA_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the Function COMM Data
*        driver's address.
*
*   INPUTS
*
*       handle                   Double pointer used to retrieve the class
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicate there exists a Function DATA
*                            class driver.
*       NU_NOT_PRESENT      Indicate that handle does not exist
*
*************************************************************************/
STATUS NU_USBF_COMM_DATA_GetHandle ( VOID** handle )
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBF_COMM_DATA_Cb_Pt;
    if ( NU_USBF_COMM_DATA_Cb_Pt == NU_NULL )
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
*       NU_USBF_COMM_DATA_Create
*
* DESCRIPTION
*
*       Data Interface Class Driver initialization routine.
*
* INPUTS
*
*       cb                                  Pointer to Driver control block
*       name                                Name of this USB object.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful Initialization
*       NU_USB_INVLD_ARG                    Some input argument(s) are
*                                           invalid.
*
*************************************************************************/
STATUS  NU_USBF_COMM_DATA_Create (NU_USBF_COMM_DATA *cb, CHAR *name)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    if (cb == NU_NULL)
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Reset the control block */
        memset((VOID *)cb, 0, sizeof(NU_USBF_COMM_DATA));

        /* Call NU_USBF_DRVR behavior using _NU_USBF_DRVR_Create
         * by passing cb, address to usbf_comm_data_dispatch as the
         * dispatch table and your specific match spec.
         */
        status = _NU_USBF_DRVR_Create (&cb->parent, name,
                                   (USB_MATCH_CLASS), 0, 0, 0, 0,
                                   (COMMF_DATA_CLASS), 0, 0,
                                   &usbf_comm_data_dispatch);
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBF_COMM_DATA_Delete
*
* DESCRIPTION
*
*       This function deletes an instance of Data Interface Class driver.
*
* INPUTS
*
*       cb                                  Pointer to the USB Object
*                                           control block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*
*************************************************************************/
STATUS  _NU_USBF_COMM_DATA_Delete (VOID *cb)
{
    /* Call NU_USBF_DRVR's Behavior. */
    return ( _NU_USBF_DRVR_Delete(cb));
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_COMM_DATA_Init_Intf
*
* DESCRIPTION
*
*       Connect notification function invoked by stack when driver is
*       given an opportunity to own an Interface.
*
* INPUTS
*
*       cb                                  Pointer to Class Driver Control
*                                           Block.
*       stk                                 Pointer to Stack Control Block
*                                           of the calling stack.
*       dev                                 Pointer to Device Control Block
*                                           of the device found.
*       intf                                Pointer to Interface control
*                                           Block to be served by this
*                                           class driver.
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*       NU_NOT_PRESENT                      Indicates the some required
*                                           resource could not be found.
*
*************************************************************************/
STATUS  _NU_USBF_COMM_DATA_Init_Intf (NU_USB_DRVR * cb, NU_USB_STACK * stk,
                         NU_USB_DEVICE * dev, NU_USB_INTF * intf)
{
    NU_USBF_COMM_DATA * data;
    NU_USBF_COMM *comm = NU_NULL;
    NU_USB_DRVR *old;
    NU_USB_CFG *cfg;
    NU_USB_INTF *master_intf = NU_NULL;
    BOOLEAN is_claimed = NU_FALSE;
    BOOLEAN bAvailable;

    INT i;
    STATUS status;
    UINT8 class_code = 0;
    UINT8 intf_num = 0;
    USBF_COMM_DEVICE *device = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    data = (NU_USBF_COMM_DATA *) cb;
    bAvailable = NU_FALSE;

    /* Find the current active configuration. */
    status = NU_USB_DEVICE_Get_Active_Cfg (dev, &cfg);

    if(status == NU_SUCCESS)
    {
        /* Data interface needs to know its Master interface (Communication
         * interface). It will share the resources of its Master.
         */
        status = NU_USB_INTF_Get_Intf_Num(intf, &intf_num);
    }

    if (status == NU_SUCCESS)
    {
        /* Try to find a communication interface just before this data
           interface.
         */
        for (i = intf_num-1;
            (i >= 0) && (status == NU_SUCCESS);
            i--)
        {
            status = NU_USB_CFG_Get_Intf(cfg, i, &master_intf);

            if(status == NU_SUCCESS)
            {
                status = NU_USB_INTF_Get_Class(master_intf, &class_code);
            }

            if(status == NU_SUCCESS)
            {
                if (class_code == COMMF_CLASS)
                {
                    break;
                }
            }
        }

        if(status == NU_SUCCESS)
        {
            /* If Master interface is not found, return error. */
            if (i < 0)
            {
                status = NU_NOT_PRESENT;
            }
        }

        if(status == NU_SUCCESS)
        {
            /* Find the driver of master interface. */
            status = NU_USB_INTF_Get_Is_Claimed(
                        master_intf,
                        &is_claimed,
                        (NU_USB_DRVR**)&comm);
        }

        if(status == NU_SUCCESS)
        {
            if (!is_claimed)
            {
                status = NU_NOT_PRESENT;
            }
        }

        if(status == NU_SUCCESS)
        {
            data->commf_mng_drvr = comm;

            /* Find communication device initialized by Master
             * interface.
             */
            for (i = 0; i < COMMF_MAX_DEVICES; i++)
            {
                device = &comm->devices[i];
                if ((device->is_valid) && (device->dev == dev) &&
                    (device->master_intf == master_intf))
                {
                    break;
                }
            }

            if (i == COMMF_MAX_DEVICES)
            {
                status = NU_NOT_PRESENT;
            }

            else
            {
                /* Update device properties. */
                device->slave_intf = intf;
                device->data_drvr = (NU_USBF_DRVR*)cb;

                status = COMMF_Init_Rx_Buff_Grp_Queue(device);

                if(status == NU_SUCCESS)
                {
                    status = COMMF_Init_Tx_Queue(device);
                }

                /* Find set_intf handler with default alt_sttg. */
                if(status == NU_SUCCESS)
                {
                    status = _NU_USBF_COMM_DATA_Set_Intf (cb,
                                stk,
                                device->dev,
                                device->slave_intf,
                                &device->slave_intf->alt_settg[0]);
                }

                if (status == NU_SUCCESS)
                {
                    /* Release the interface if already claimed. */
                    status = NU_USB_INTF_Get_Is_Claimed(
                                intf,
                                &is_claimed,
                                &old);

                    if(status == NU_SUCCESS)
                    {
                        if (is_claimed)
                        {
                            status = NU_USB_INTF_Release(intf, old);
                        }
                    }

                    /* Claim the interface. */
                    if(status == NU_SUCCESS)
                    {
                        status = NU_USB_INTF_Claim (intf, cb);
                    }

                    /* If no transfer is in progress, submit the
                     * request to receive data.
                     */
                    if(status == NU_SUCCESS)
                    {
                        status = COMMF_Get_Event(
                                    device,
                                    COMMF_RX_IN_PROGRESS,
                                    &bAvailable,
                                    NU_NO_SUSPEND,
                                    NU_AND);

                        if (status == NU_SUCCESS)
                        {
                            if(bAvailable == NU_FALSE)
                            {
                                status = COMMF_DATA_Submit_Rx_Data_IRP(
                                            device);
                            }
                        }
                    }
                }
            }
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_COMM_DATA_Disconnect
*
* DESCRIPTION
*
*       Disconnect Callback function, invoked by stack when a
*       Device/Interface served by Data Interface is removed from the BUS.
*
* INPUTS
*
*       cb                                  Pointer to Class Driver Control
*                                           Block claimed this Interface.
*       stack                               Pointer to Stack Control Block.
*       dev                                 Pointer to NU_USB_DEVICE
*                                           Control Block disconnected.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*       NU_NOT_PRESENT                      Indicates the some required
*                                           resource could not be found.
*
*************************************************************************/
STATUS  _NU_USBF_COMM_DATA_Disconnect (NU_USB_DRVR * cb,
                    NU_USB_STACK * stack,
                    NU_USB_DEVICE * dev)
{
    NU_USBF_COMM_DATA *data;
    USBF_COMM_DEVICE *device;
    STATUS status;

    INT i;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    data = (NU_USBF_COMM_DATA*)cb;
    status = NU_NOT_PRESENT;
    /* If driver is not properly initialized,don’t proceed. */
    if (data->commf_mng_drvr != NU_NULL)
    {
        /* Check for device for which disconnect is received.*/
        for (i = 0; i < COMMF_MAX_DEVICES; i++)
        {
            device = &data->commf_mng_drvr->devices[i];
            if ((device->is_valid) && (device->dev == dev))
            {
                status = NU_USB_INTF_Release (
                            device->slave_intf,
                            (NU_USB_DRVR *) data);

                /* Un-initialize the current slave interface. */
                if(status == NU_SUCCESS)
                {
                    device->slave_intf = NU_NULL;

                    status = NU_USBF_USER_COMM_DATA_Discon(
                            device->user,
                            device->data_drvr,
                            device);

                    if(status == NU_SUCCESS)
                    {
                        device->is_valid = 0;

                        if (device->in_pipe)
                        {
                            status = NU_USB_PIPE_Flush(device->in_pipe);
                        }
                    }
                }

                if(status == NU_SUCCESS)
                {
                    if (device->out_pipe)
                    {
                        status = NU_USB_PIPE_Flush(device->out_pipe);
                    }
                }
            }
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_COMM_DATA_Set_Intf
*
* DESCRIPTION
*
*       This function processes the new alternate setting request from
*       Host.
*
* INPUTS
*
*       cb                                  Class driver for which this
*                                           callback is meant for.
*       stack                               Stack invoking this function.
*       device                              Device on which this event has
*                                           happened.
*       intf                                Interface which is affected.
*       alt_settg                           New alternate setting for the
*                                           interface.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the class driver
*                                           could process the event
*                                           successfully.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
*       NU_NOT_PRESENT                      Indicates the some required
*                                           resource could not be found.
*
*************************************************************************/
STATUS  _NU_USBF_COMM_DATA_Set_Intf (NU_USB_DRVR * cb,
                              NU_USB_STACK * stack,
                              NU_USB_DEVICE * dev,
                              NU_USB_INTF * intf,
                              NU_USB_ALT_SETTG * alt_settg)
{
    STATUS status;
    USBF_COMM_DEVICE *device = NU_NULL;
    NU_USBF_COMM_DATA *data;
    UINT8 num_endp;
    NU_USB_PIPE *pipe = NU_NULL;
    INT i;
    BOOLEAN bAvailable;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    data = (NU_USBF_COMM_DATA *) cb;
    bAvailable = NU_FALSE;

    /* If Set_Intf is called before the Initialize_Intf, return error. */
    if (data->commf_mng_drvr == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }
    else
    {
        for (i = 0; i < COMMF_MAX_DEVICES; i++)
        {
            /* Check for device for which connect is received.*/
            device = &data->commf_mng_drvr->devices[i];

            /* If device is not properly initialized, return error. */
            if ((device->is_valid) && (device->dev == dev) &&
                                            (device->slave_intf == intf))
            {
                break;
            }
        }

        if (i == COMMF_MAX_DEVICES)
        {
            status = NU_NOT_PRESENT;
        }
        else
        {
            device->slave_alt_settg = alt_settg;

            /* According to Communication class definition, Data Interface
             * should have 2 alternate settings. Default alternate setting
             * have no endpoints and other alternate setting will
             * have endpoints for data transfers. If current alternate
             * setting have more than zero endpoint, only then pipes will
             * be found and connect callback will be sent to user driver.
             */

            /*
             * Find the number of endpoints in current alternate setting.
             */
            status = NU_USB_ALT_SETTG_Get_Num_Endps(alt_settg, &num_endp);

            /* If number of endpoints in current alternate setting are
             * non-zero, then proceed further. */
            if (num_endp > 0)
            {
                /* Initialize the data transfer state for device. */
                device->data_state = 0;

                status = COMMF_Clear_Event(
                            device,
                            COMMF_TX_IN_PROGRESS);

                /*
                 * If any receive transfer was in progress, invalidate it.
                 */
                if(status == NU_SUCCESS)
                {
                    status = COMMF_Get_Event(
                                device,
                                COMMF_RX_IN_PROGRESS,
                                &bAvailable,
                                NU_NO_SUSPEND,
                                NU_AND);
                }

                if(status == NU_SUCCESS)
                {
                    if(bAvailable == NU_TRUE)
                    {
                        status = NU_USB_PIPE_Flush(device->out_pipe);

                        if(status == NU_SUCCESS)
                        {
                            status = COMMF_Clear_Event(
                                        device,
                                        COMMF_RX_IN_PROGRESS);
                        }
                    }
                }

                /* Find the Bulk IN pipe. */
                if(status == NU_SUCCESS)
                {
                    status = NU_USB_ALT_SETTG_Find_Pipe (alt_settg,
                                 USB_MATCH_EP_TYPE |
                                 USB_MATCH_EP_DIRECTION,
                                 0, USB_DIR_IN,
                                 USB_EP_BULK,
                                 &pipe);
                }

                if (status == NU_SUCCESS)
                {
                    device->in_pipe = pipe;

                    /* Find the Bulk OUT pipe. */
                    status = NU_USB_ALT_SETTG_Find_Pipe (alt_settg,
                                  USB_MATCH_EP_TYPE |
                                  USB_MATCH_EP_DIRECTION,
                                  0,
                                  USB_DIR_OUT, USB_EP_BULK,
                                  &pipe);

                    if (status == NU_SUCCESS)
                    {
                        device->out_pipe = pipe;

                        /* No user drivers are registered with this driver.
                         * It uses the user driver of Communication class
                         * driver. If user of Master driver is not
                         * initialized, return error.
                         */
                        if (device->user == NU_NULL)
                        {
                            status = NU_NOT_PRESENT;
                        }
                        if(status == NU_SUCCESS)
                        {
                            status = NU_USBF_USER_COMM_DATA_Connect(
                                    device->user,
                                    device->data_drvr,
                                    device);
                        }
                    }
                }
            }
            else
            {
                /* If Host selects alternate setting without endpoints,
                 * uninitialized the pipes, if already found.
                 */
                device->in_pipe = NU_NULL;
                device->out_pipe = NU_NULL;
            }
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_COMM_DATA_New_Setup
*
* DESCRIPTION
*
*       Processes a new class specific SETUP packet from the Host.
*       The format of the setup packet is defined by the corresponding
*       class specification / custom protocol. The setup packet is
*       validated by this function and processed further as per the class
*       specification.
*       If there is any data phase associated with the setup request,
*       then the NU_USB_Submit_Irp function can be used to submit the
*       transfer to the stack. Status phase is automatically handled by
*       the Stack. If there is no data transfer associated with the
*       command, then no transfer is submitted.
*       For unknown and unsupported command, this function returns
*       appropriate error status. If this function returns any status
*       other than NU_SUCCESS, then the default endpoint will be stalled.
*
* INPUTS
*
*       cb                                  Class driver for which this
*                                           callback is meant for.
*       stack                               Stack invoking this function.
*       device                              Device on which this event has
*                                           happened.
*       setup                               the 8 byte setup packet
*                                           originating from the Host.
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED                Indicates that this call is not
*                                           supported by this driver.
*
*************************************************************************/
STATUS  _NU_USBF_COMM_DATA_New_Setup (NU_USB_DRVR * cb,
                                   NU_USB_STACK * stack,
                                   NU_USB_DEVICE * dev,
                                   NU_USB_SETUP_PKT * setup)
{
    /* Control requests should be received on communication interface, not
     * data interface.
     */
    return (NU_USB_NOT_SUPPORTED);
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBF_COMM_DATA_New_Transfer
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
*       stack ignores the return value of this function, any required
*       error processing is carried out by the class driver itself.
*
*
* INPUTS
*
*       cb                                  Class driver for which this
*                                           callback is meant for
*       stack                               Stack invoking this function
*       device                              Device on which this event has
*                                           happened
*       pipe                                Pipe on which the data transfer
*                                           is initiated by the Host
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the class driver
*                                           could process the event
*                                           successfully.
*
*************************************************************************/
STATUS  _NU_USBF_COMM_DATA_New_Transfer(NU_USB_DRVR * cb,
                                  NU_USB_STACK * stack,
                                  NU_USB_DEVICE * device,
                                  NU_USB_PIPE * pipe)
{

    /* This situation is not expected, so it is being left un-handled. */
    return (NU_SUCCESS);

}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_COMM_DATA_Notify
*
* DESCRIPTION
*
*       Notifies Driver of USB Events.
*
* INPUTS
*
*       cb                                  Class driver for which this
*                                           callback is meant for
*       stack                               Stack invoking this function
*       dev                                 Device on which this event has
*                                           happened
*       event                               USB event that has occurred
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the class driver
*                                           could process the event
*                                           successfully.
*
*************************************************************************/
STATUS  _NU_USBF_COMM_DATA_Notify(NU_USB_DRVR * cb,
                            NU_USB_STACK * stack,
                            NU_USB_DEVICE * dev, UINT32 event)
{
    STATUS status = NU_SUCCESS;

    /* All USB events are handled by Communication class driver, so this
     * function is being left un-handled.
     */

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_COMM_DATA_Send
*
* DESCRIPTION
*
*       This function sends the communication data to USB Host.
*
* INPUTS
*
*       cb                                  Pointer to the USB driver
*                                           control block.
*       buffer                              Pointer to the data buffer.
*       length                              Data transfer length.
*       delineate                           Tells whether zero length
*                                           should be sent as end of
*                                           transfer for length multiple of
*                                           maximum packet size
*       handle                              Handle for this transfer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USB_INVLD_ARG                    Indicates that some input
*                                           argument is invalid.
*
*************************************************************************/
STATUS  NU_USBF_COMM_DATA_Send(NU_USBF_DRVR  *cb,
                              UINT8 *buffer,
                              UINT32 length,
                              void *handle)
{
    USBF_COMM_DEVICE *device;
    NU_USBF_COMM_DATA *data;
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    data = (NU_USBF_COMM_DATA *) cb;

    NU_ASSERT(cb);

    /* This function assumes that length will be up to the Maximum payload
     * size. If larger length is given, return error. Or communication
     * device is not properly initialized, return error.
     */
    if ((cb == NU_NULL) || (data->commf_mng_drvr == NU_NULL) ||
                                                    (handle == NU_NULL) )
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        device = (USBF_COMM_DEVICE *)handle;

        status = COMMF_Enqueue_Tx_Transfer(
                    device,
                    buffer,
                    length);

        if(status == NU_SUCCESS)
        {
            status = COMMF_DATA_Submit_Tx_Data_IRP(device);
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_COMM_DATA_Get_Rcvd
*
* DESCRIPTION
*
*       This function is called by user driver for retrieving received
*       data parameters after getting the New_Transfer callback from the
*       data class interface driver.
*
* INPUTS
*
*       cb                                  Pointer to the USB driver
*                                           control block.
*       data_out                            Pointer to location for buffer.
*       data_len_out                        Pointer to location for length.
*       handle                              Handle for current transfer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USB_INVLD_ARG                    Indicates that some input
*                                           argument is invalid.
*
*************************************************************************/
STATUS  NU_USBF_COMM_DATA_Get_Rcvd(NU_USBF_DRVR  *cb,
                                   UINT8 **data_out,
                                   UINT32 *data_len_out,
                                   VOID *handle)
{
    USBF_COMM_DEVICE *device;
    NU_USBF_COMM_DATA *data;
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    data = (NU_USBF_COMM_DATA *) cb;
    status = NU_SUCCESS;

    NU_ASSERT(cb);

    /* Return error, if some input argument is invalid. */
    if ((cb == NU_NULL) || (data->commf_mng_drvr == NU_NULL))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        device = (USBF_COMM_DEVICE *)handle;
        if (device == NU_NULL)
        {
            status = NU_USB_INVLD_ARG;
        }
        else
        {
            *data_out = device->cmplt_rx_buffer;
            *data_len_out = device->cmplt_rx_buffer_len;
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_COMM_DATA_Reg_Rx_Buffer
*
* DESCRIPTION
*
*       This function registers a buffer group for receiving data from
*       Host.
*
* INPUTS
*
*       cb                                  Pointer to the USB driver
*                                           control block.
*       handle                              Handle identifying the device.
*       buffer_group                        Pointer to buffer group of
*                                           receive buffers.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USB_INVLD_ARG                    Indicates that some input
*                                           argument is invalid.
*
*************************************************************************/
STATUS  NU_USBF_COMM_DATA_Reg_Rx_Buffer(NU_USBF_COMM_DATA* data_drvr,
                                        VOID *handle,
                                        COMMF_RX_BUF_GROUP *buffer_group)
{
    STATUS status;
    USBF_COMM_DEVICE * device;
    BOOLEAN bAvailable;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    bAvailable = NU_FALSE;
    status = NU_SUCCESS;

    /* If input arguments are invalid, return error. */
    if ((handle == NU_NULL) ||
        (buffer_group == NU_NULL) ||
        (buffer_group->num_of_buffers == 0))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        device = handle;

        status = COMMF_Enqueue_Rx_Buff_Grp(device, buffer_group);
        if(status == NU_SUCCESS)
        {
            /* If no transfer is in progress, submit the request to receive
             * data.
             */
            status = COMMF_Get_Event(
                        device,
                        COMMF_RX_IN_PROGRESS,
                        &bAvailable,
                        NU_NO_SUSPEND,
                        NU_AND);

            if(status == NU_SUCCESS)
            {
                if(bAvailable == NU_FALSE)
                {
                    status = COMMF_DATA_Submit_Rx_Data_IRP(device);
                }
            }
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_COMM_DATA_Config_Xfers
*
* DESCRIPTION
*
*       This function configures the data transfer resources depending
*       upon the requirement of user driver/application.
*
* INPUTS
*
*       cb                                  Pointer to the USB driver
*                                           control block.
*       handle                              Handle for current transfer.
*       conf                                Pointer to transfers
*                                           configuration structure.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USB_INVLD_ARG                    Indicates that some input
*                                           argument is invalid.
*
*************************************************************************/
STATUS  NU_USBF_COMM_DATA_Config_Xfers(NU_USBF_COMM_DATA* data_drvr,
                                       VOID *handle,
                                       COMMF_DATA_CONF *conf)
{
    USBF_COMM_DEVICE *device;
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    device = (USBF_COMM_DEVICE *)handle;

    if ((handle == NU_NULL) || (conf == NU_NULL) )
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Initialize whether this device should send a zero length packet
         * if transmit buffer is multiple of maximum packet size of
         *  endpoint.
         */
        device->delineate = conf->delineate;
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_COMM_DATA_Dis_Reception
*
* DESCRIPTION
*
*       This function disables the receive data transfer for this device.
*
* INPUTS
*
*       cb                                  Pointer to the USB driver
*                                           control block.
*       handle                              Handle for current transfer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USB_INVLD_ARG                    Indicates that some input
*                                           argument is invalid.
*
*************************************************************************/
STATUS  NU_USBF_COMM_DATA_Dis_Reception(NU_USBF_COMM_DATA* data_drvr,
                                        VOID *handle)
{
    USBF_COMM_DEVICE *device;
    STATUS status;
    BOOLEAN bAvailable;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    device = (USBF_COMM_DEVICE *)handle;
    status = NU_SUCCESS;
    bAvailable = NU_FALSE;

    if (handle == NU_NULL)
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        status = COMMF_Clear_Rx_Buff_Grp_Queue(device);

        /* If any receive transfer is in progress, clear it. */
        if(status == NU_SUCCESS)
        {
            status = COMMF_Get_Event(
                        device,
                        COMMF_RX_IN_PROGRESS,
                        &bAvailable,
                        NU_NO_SUSPEND,
                        NU_AND);
        }

        if(status == NU_SUCCESS)
        {
            if(bAvailable == NU_TRUE)
            {
                status = NU_USB_PIPE_Flush(device->out_pipe);
            }
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_COMM_DATA_Rbg_Create
*
* DESCRIPTION
*
*       This function initializes the receive buffer group.
*
* INPUTS
*
*       buff_grp                            Pointer to buffer group.
*       callback                            Buffer group finished callback.
*       buff_array                          Array containing pointer to
*                                           receive buffers.
*       num_of_buffs                        Number of buffers in array
*       buff_size                           Size of each buffer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*
*************************************************************************/
STATUS  NU_USBF_COMM_DATA_Rbg_Create(COMMF_RX_BUF_GROUP* buff_grp,
                                      COMMF_BUFF_FINISHED  callback,
                                      UINT8** buff_array,
                                      UINT32  num_of_buffs,
                                      UINT32  buff_size)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_ASSERT(buff_grp);
    memset(buff_grp, 0, sizeof(COMMF_RX_BUF_GROUP));

    buff_grp->callback = callback;
    buff_grp->buffer_array = buff_array;
    buff_grp->num_of_buffers = num_of_buffs;
    buff_grp->buffer_size = buff_size;

    /* Revert to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_COMM_DATA_Cancel_Io
*
* DESCRIPTION
*
*       This function cancels all TX operations submitted to data interface
*       of communication class driver. It has no effect on RX operations.
*
* INPUTS
*
*       *cb                                 Pointer to the USB driver
*                                           control block.
*       *handle                             Identification handle for the
*                                           device.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that IO cancellation
*                                           is successful.
*       NU_USB_INVLD_ARG                    Indicates that in_pipe's
*                                           reference is not initialized
*                                           properly. Device is in
*                                           disconnected state.
*
*************************************************************************/
STATUS  NU_USBF_COMM_DATA_Cancel_Io(
            NU_USBF_COMM_DATA *cb,
            VOID * handle)
{
    STATUS status;
    USBF_COMM_DEVICE *device;
    COMMF_CMPL_CTX ctx;
    VOID * buffer = NU_NULL;
    UINT32 length = 0;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    if ((cb == NU_NULL) || (handle == NU_NULL) )
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        device = (USBF_COMM_DEVICE*)handle;
        do
        {
            status = COMMF_Dequeue_Tx_Transfer(device,
                                               &buffer,
                                               &length);

            if(status == NU_SUCCESS)
            {
                ctx.handle = handle;
                ctx.status = NU_USB_IRP_CANCELLED;
                ctx.transfer_type = COMMF_DATA_SENT;

                /* Pass notification to user. */
                (VOID)NU_USBF_USER_Tx_Done (
                                          device->user,
                                          device->data_drvr,
                                          (VOID*)&ctx,
                                          buffer,
                                          0,
                                          NU_NULL,
                                          NU_NULL);
            }
        }while(status == NU_SUCCESS);

        if ((device->in_pipe) != NU_NULL)
        {
            status = NU_USB_PIPE_Flush(device->in_pipe);
        }

        else
        {
            status = NU_INVALID_POINTER;
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/* ======================  End Of File  =============================== */
