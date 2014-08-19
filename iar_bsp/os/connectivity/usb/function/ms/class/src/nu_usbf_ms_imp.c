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
*       nu_usbf_ms_imp.c
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the internal functions of the Mass Storage
*       class driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_MSF_Command_IRP_Complete         Processes a received command.
*       NU_MSF_Data_IRP_Complete            Data transfer completion
*                                           processing.
*       NU_MSF_Process_CBW                  Processes a received CBW.
*       NU_MSF_Process_Command              MS event handling function in task mode.
*       NU_MSF_Stall_Pipes                  Stalls BULK IN and OUT pipes.
*       NU_MSF_Unstall_Pipes                Unstalls BULK IN and OUT pipes.
*       NU_MSF_Submit_Command_IRP           Submits a transfer to receive
*                                           a CBW.
*       NU_MSF_Submit_Status_IRP            Submits a CSW for transfer to
*                                           Host.
*       NU_MSF_Initialize_Device            Creates USB environment.
*       NU_MSF_Release_Memory               Releases memory used by task.
*       NU_MSF_Process_Data_And_Status      Process data and status stage
*                                           of MS specific commands.
*       NU_MSF_Process_Connect_event        Notify the user driver about
*                                           the connection.
*       NU_MSF_Init_Task                    Initialize command processing
*                                           task.
*       NU_MSF_Clear_Feature_Callback       Clear Feature callback function
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
/* USB Include Files. */
#include    "connectivity/nu_usb.h"

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Command_IRP_Complete
*
* DESCRIPTION
*
*        CBW received Completion Callback. This function is called by
*        stack when an IRP over Bulk Out pipe gets completed that was
*        submitted for receiving CBW.
*
* INPUTS
*
*        pcb_pipe          Pointer to the pipe control block on which this
*                          transfer has happened.
*        pcb_irp           Pointer to the IRP Transfer that has been
*                          complete.
*
* OUTPUTS
*
*        None.
*
**************************************************************************/
VOID NU_MSF_Command_IRP_Complete (NU_USB_PIPE *pcb_pipe,
                                  NU_USB_IRP  *pcb_irp)
{
    /* Local variables. */
    NU_USBF_MS_DEVICE *ms_device_ptr = NU_NULL;
    STATUS             status, irp_status;
    UINT8              cbw_status = MS_CBW_BAD;

    /* Get device ptr */
    status = NU_USB_IRP_Get_Context (pcb_irp, (VOID **)&ms_device_ptr);
    NU_USB_ASSERT( status == NU_SUCCESS );

    /* Get status of IRP completed. */
    status = NU_USB_IRP_Get_Status (pcb_irp, &irp_status);

    /* Was this IRP successful? */
    if ( status == NU_SUCCESS 
        && irp_status == NU_SUCCESS )
    {
        /* Validate CBW */
        cbw_status = NU_MSF_Is_Valid_CBW(pcb_irp);
        if ( cbw_status == MS_CBW_OK )
        {
#ifdef NU_USBF_MS_TASK_MODE
            /* Set the notification that CBW is received. */
            status = NU_Set_Events (&ms_device_ptr->event_group,
                                    NU_USBF_MS_COMMAND,
                                    NU_OR);
#else
            status = NU_MSF_Process_CBW(ms_device_ptr);
#endif
        }
        else
        {
            ms_device_ptr->invalid_cbw = NU_TRUE;
            status = NU_MSF_Stall_Pipes (ms_device_ptr);
        }
    }
}

#ifdef NU_USBF_MS_TASK_MODE
/**************************************************************************
* FUNCTION
*
*        NU_MSF_Process_Command
*
* DESCRIPTION
*
*        Task execution function. This function processes MS
*        events accordingly.
*
* INPUTS
*
*        argc                   Argument count, dummy variable, used
*                               only for convention
*
*        pcb_ms_device          Pointer to the device on which a CBW
*                               has been received.
*
* OUTPUTS
*
*        None.
*
**************************************************************************/
VOID NU_MSF_Process_Command (UNSIGNED  argc,
                             VOID     *pcb_ms_device)
{

    /* Local variables. */
    NU_USBF_MS_DEVICE *ms_device_ptr = pcb_ms_device;
    UNSIGNED           event;
    STATUS             status;
    
    /* This task process events mentioned in 'flag' variable.*/
    UNSIGNED  flag = NU_USBF_MS_COMMAND | NU_USBF_MS_DATA_STATUS |
                     NU_USBF_MS_CLEAR_FEATURE;

    /* Remove unused variable warning. */
    NU_UNUSED_PARAM(status);

    for(;;)
    {
        /* Wait for events. */
        status = NU_Retrieve_Events (&ms_device_ptr->event_group, flag,
                             NU_OR_CONSUME, &event, NU_SUSPEND);
        NU_USB_ASSERT( status == NU_SUCCESS );

        if(event & NU_USBF_MS_COMMAND)
        {
            /* Process Command Block Wrapper(CBW)*/
            status = NU_MSF_Process_CBW(ms_device_ptr);
            NU_USB_ASSERT( status == NU_SUCCESS );
            continue;
        }

        if(event & NU_USBF_MS_DATA_STATUS)
        {
            /* Mass Storage specific data and status specific. */
            status = NU_MSF_Process_Data_And_Status(ms_device_ptr);
            NU_USB_ASSERT( status == NU_SUCCESS );
            continue;
        }

        if(event & NU_USBF_MS_CLEAR_FEATURE)
        {
            /* If CBW is invalid, keep on stalling BULK
             * endpoints until Reset Recovery. */
            status = NU_MSF_Clear_Feature_Callback(pcb_ms_device);
            NU_USB_ASSERT( status == NU_SUCCESS );
            continue;
        }

    }

}

#endif

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Process_Connect_event
*
* DESCRIPTION
*
*        This function first finds the users associated with this class
*        and then notify the user driver about the connect event.
*
* INPUTS
*
*        pcb_ms_device          Pointer to the device on which a CBW
*                               has been received.
*
* OUTPUTS
*
*        NU_SUCCESS             Indicates that the command has been
*                               processed successfully.
*        NU_NOT_PRESENT         No user was found.
*        NU_USB_INVLD_ARG       Indicates a malformed command block.
*
**************************************************************************/

STATUS NU_MSF_Process_Connect_event (NU_USBF_MS_DEVICE *pcb_ms_device)
{
    STATUS             status = NU_NOT_PRESENT;
    UINT8              sub_class, intf_sub_class, user_index;
    NU_USB_USER       *user_list_ptr[NU_USB_MAX_USERS];
    NU_USB_USER       *user_ptr;
    UNSIGNED           num_users;
    NU_USBF_MS        *ms_ptr = (NU_USBF_MS *)pcb_ms_device->usb_drvr;

    /* Initialize FC pointer. */
    NU_USBF_HW *fc_ptr = (NU_USBF_HW *)pcb_ms_device->dev->hw;

    /* Get the number of users associated. */
    num_users = NU_USB_DRVR_Get_Users (pcb_ms_device->usb_drvr,
                                       &user_list_ptr[0],
                                       (UNSIGNED)NU_USB_MAX_USERS);
    /* Validate each user. */
    for (user_index = 0;
       (( user_index < (UINT8)num_users)&&(user_index < NU_USB_MAX_USERS));
      user_index++)
    {

        /* Get first user from the list of users. */
        user_ptr   = user_list_ptr[user_index];

        /* Get the sub class for this user. */
        status = NU_USB_USER_Get_Subclass (user_ptr, &sub_class);

        /* Keep on finding until we find any or terminating condition
         * arrives.
         */
        if(status != NU_SUCCESS)
        {
            continue;
        }

        /* Get subclass for this interface. */
        status = NU_USB_INTF_Get_SubClass (pcb_ms_device->intf,
                                           &intf_sub_class);
        if (status != NU_SUCCESS)
        {
            break;
        }

        /* If the user provided sub class while creating user driver
         * matches with that of given in interface descriptor?
         */
        if(sub_class == intf_sub_class)
        {

            /* Initialize user control block present in mass storage
             * device control block.
             */
            pcb_ms_device->user = user_ptr;

            /* Give connect callback to the associated users. */
            status = NU_USB_USER_Connect (user_ptr,
                                          pcb_ms_device->usb_drvr,
                                          (VOID *)pcb_ms_device);
        }

        /* If the operation was successful? */
        if(status == NU_SUCCESS)
        {
            break;                   /* Exit loop. */
        }

        /* On failure, mark the user driver control block as
         * invalid.
         */
        pcb_ms_device->user = NU_NULL;

    }

    /* Did we find any user or we found the loop terminating condition?
     */
    if(num_users == (UNSIGNED)user_index)
    {
        status = NU_NOT_PRESENT;        /* No user is present. */
    }

    if(status == NU_SUCCESS)
    {
        /* Get Function controller capabilities. It encircles, which
         * standard requests hardware is capable of processing
         * automatically without any need of software.
         */
         status = NU_USBF_HW_Get_Capability (fc_ptr,
                                                &(ms_ptr->hw_capability));
    }

    if((status == NU_SUCCESS) &&
        !(ms_ptr->hw_capability & (0x01 << USB_SET_CONFIGURATION)))
    {
        /* There are some hardware that don't process set configuration
         * request implicitly. In such cases connection notification
         * happens to come before 'Initialize interface' callback hence
         * 'NU_USBF_MS_Notify' will not find valid mass storage control
         * block until 'initialize interface' gets called. It would be
         * necessary to notify the users again on initialize interface
         * callback. We are checking here hardware capabilities because
         * we don't want to notify the users if the hardware implicitly
         * handles 'set configuration' request. It is due to the fact
         * that 'initialize interface' will be called even before USB
         * mass storage device is connected to the host. So it will be
         * a false impression to the users that device is connected.
         * sh3dsp board with SH7727R2 controller is an example of it.
         */

        /* Call mass storage notify event functionality. */
        status = _NU_USBF_MS_Notify (
                                     pcb_ms_device->usb_drvr,
                                     pcb_ms_device->stack,
                                     pcb_ms_device->dev,
                                     (UINT32)USBF_EVENT_CONNECT);
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Process_CBW
*
* DESCRIPTION
*
*        This function validates the CBW and informs the client driver
*        of the new command. A CBW has been received.
*
* INPUTS
*
*        pcb_ms_device          Pointer to the device on which a CBW
*                               has been received.
*
* OUTPUTS
*
*        NU_SUCCESS             Indicates that the command has been
*                               processed successfully.
*        NU_USB_NOT_SUPPORTED   Indicates that the command is unsupported.
*        NU_USB_INVLD_ARG       Indicates a malformed command block.
*        NU_NOT_PRESENT         Indicates that the pipe is invalid
*        NU_NO_MEMORY           Indicates failure of memory allocation
*
**************************************************************************/
STATUS NU_MSF_Process_CBW (NU_USBF_MS_DEVICE *pcb_ms_device)
{

    /* Local variables. */
    STATUS       status;
    NU_USB_PIPE *pipe_ptr;
    MSF_BOT_CBW *cbw_ptr = (pcb_ms_device->cbw);
    UINT8        cbw_flags;

    /* Dummy Loop. */
    do
    {
        /* CBW is already validated. */
        /* Set residue field as transfer length of CBW. */
        pcb_ms_device->residue = cbw_ptr->dCBWTransferLength;

        /* Reset LUN bits inside CBWCB command block. */
        cbw_ptr->cbwcb[USBF_SPEC_MSF_CBWCB_LUN_BYTE] &=
                                               USBF_SPEC_MSF_CBW_LUN_MASK;

        /* Initialize LUN bits with proper value.  */
        cbw_ptr->cbwcb[USBF_SPEC_MSF_CBWCB_LUN_BYTE] |=
                        (cbw_ptr->bCBWLUN << NU_USBF_MS_SHIFT_FIVE_BITS);

        /* Decipher the data transfer direction and fill in the endpoint
         * number in the IRP.
         */
        cbw_flags = cbw_ptr->bCBWFlags & USB_DIR_IN;
        pipe_ptr  = (cbw_flags == USB_DIR_IN)?
                    pcb_ms_device->in_pipe : pcb_ms_device->out_pipe;

        status = NU_USBF_USER_New_Command (
                                      pcb_ms_device->user,
                                      pcb_ms_device->drvr,
                                      (VOID *)pcb_ms_device,
                                      cbw_ptr->cbwcb,
                                      (UINT16)USB_SPEC_MSF_CBW_CMD_LEN,
                                      &pcb_ms_device->data,
                                      &pcb_ms_device->data_len);
        /* Some error occurred while processing the command. */
        if (status == NU_USBF_MS_SEND_STALL)
        {
            if(cbw_ptr->dCBWTransferLength)
            {
                pcb_ms_device->cmd_failed = NU_TRUE;
                status = NU_USB_PIPE_Stall(pipe_ptr);
                break;
            }

            /* No data stage submit CSW with error status. */
            status = NU_MSF_Submit_Status_IRP (
                                     pcb_ms_device,
                                     (UINT8)USB_SPEC_MSF_CSW_CMD_FAILED);
            break;
        }

        /* Command executed successfully, but no data exchange is expected as part of this command. */
        /* Since this case is likely to execute most of the times.
         * We are checking it first.
         */
        if (cbw_ptr->dCBWTransferLength == (UINT32)0)
        {
            /* Case # 2 and 3 (Hn < Di-D0). Page 19 of Bulk only transport.
             */
            if (pcb_ms_device->data_len)
            {
                /* Submit CSW with phase error status.*/
                status = NU_MSF_Submit_Status_IRP (pcb_ms_device,
                                                    USB_SPEC_MSF_CSW_PHASE_ERROR);
            }
            else
            {
                /* Case # 1.*/
                status = NU_MSF_Submit_Status_IRP (pcb_ms_device,
                                                    USB_SPEC_MSF_CSW_CMD_SUCCESS);
            }

            break;
        }

        /* Command is executed successfuly. Wait for application to kick off data phase of command. */
        if(status == NU_USBF_MS_IO_PENDING)
        {
            status = NU_SUCCESS;

            /* case # 9 and 4. */
            if(!pcb_ms_device->data_len)
            {
                pcb_ms_device->cmd_failed = NU_TRUE;
                status = NU_USB_PIPE_Stall(pipe_ptr);
                break;
            }

            /* Since this case is likely to execute most of the times.
             * We are checking it first.
             */
            /* If host and function data direction mismatch. */
            else if((cbw_ptr->bCBWFlags != pcb_ms_device->data_direction) &&
                            (pcb_ms_device->data_len))
            {
                /* Case # 10. */
                pcb_ms_device->phase_error = NU_TRUE;
                status = NU_USB_PIPE_Stall(pipe_ptr);
                break;
            }

            /* Set BOT stage to data stage. */
            pcb_ms_device->bot_stage = USBF_MS_BOT_STAGE_DATA;
            break;
        }
        
        /* Since this case is likely to execute most of the times.
         * We are checking it first.
         */
        /* If host and function data direction mismatch. */
        if((cbw_ptr->bCBWFlags != pcb_ms_device->data_direction) &&
                        (pcb_ms_device->data_len))
        {
            /* Case # 10. */
            pcb_ms_device->phase_error = NU_TRUE;
            status = NU_USB_PIPE_Stall(pipe_ptr);
            break;
        }

        /* Command has executed successfuly. SCSI driver has kicked off data phase of this command. */
        /* Case # 6. */
        if(pcb_ms_device->data_len <= cbw_ptr->dCBWTransferLength)
        {
            /* case # 9 and 4. */
            if(!pcb_ms_device->data_len)
            {
                pcb_ms_device->cmd_failed = NU_TRUE;
                status = NU_USB_PIPE_Stall(pipe_ptr);
                
                break;
            }
            
            /* Case # 5 & 11 -- case # 6 & 12. Prepare IRP to submit data.
             */
            status = NU_USB_IRP_Create(&pcb_ms_device->bulk_out_irp,
                                       pcb_ms_device->data_len,
                                       pcb_ms_device->data,
                                       NU_FALSE,
                                       NU_FALSE,
                                       NU_MSF_Data_IRP_Complete,
                                       (VOID *)pcb_ms_device,
                                       0);
            if(status != NU_SUCCESS)
            {
                break;
            }
                
            /* Update the residue. */
            pcb_ms_device->residue -= pcb_ms_device->bulk_out_irp.length;

            /* Submit the transfer. */
            status = NU_USB_PIPE_Submit_IRP (pipe_ptr,
                                             &pcb_ms_device->bulk_out_irp);
            break;
        }

        /* Following cases are separated from the above as they are not
         * likely to occur every time as these are error cases. We do not
         * need to check them in normal flow.
         */
        if (pcb_ms_device->data_len > cbw_ptr->dCBWTransferLength)
        {
            /* Case # 13 & 7 */
            pcb_ms_device->phase_error = NU_TRUE;
            status = NU_USB_PIPE_Stall(pipe_ptr);
        }

    }while(0);

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Process_Data_And_Status
*
* DESCRIPTION
*
*        This function informs the client driver of the new command.
*        A CBW has been received.
*
* INPUTS
*
*        pcb_ms_device          Pointer to the device on which a CBW
*                               has been received.
*
* OUTPUTS
*
*        NU_SUCCESS             Indicates that the command has been
*                               processed successfully.
*        NU_USB_NOT_SUPPORTED   Indicates that the command is unsupported.
*        NU_USB_INVLD_ARG       Indicates a malformed command block.
*        NU_NOT_PRESENT         Indicates that the pipe is invalid
*        NU_NO_MEMORY           Indicates failure of memory allocation
*
**************************************************************************/
STATUS NU_MSF_Process_Data_And_Status(NU_USBF_MS_DEVICE *pcb_ms_device)
{

    UINT32             temp;
    UINT8             *data_out_ptr;
    UINT32             rem_len;
    UINT32             length;
    UINT32             actual_length;
    STATUS             status;
    NU_USB_PIPE       *pcb_pipe = pcb_ms_device->data_pipe;
    NU_USB_IRP        *pcb_irp = pcb_ms_device->data_irp_complete;

    /* Get actual length of data transferred by this IRP. */
    status = NU_USB_IRP_Get_Actual_Length (
                                        pcb_ms_device->data_irp_complete,
                                           &actual_length);
    if (status == NU_SUCCESS)
    {
        /* Get length of data transferred by this IRP. */
        status = NU_USB_IRP_Get_Length (pcb_ms_device->data_irp_complete,
                                        &length);
        if ((status == NU_SUCCESS)&&(length > 0x00))
        {

            /* Data transfer is (partially/fully) complete. */
            temp                    = (length - actual_length);

            /* Increment residue, temp would contain zero if the
             * transfer is not ended by a short packet.
             */
            pcb_ms_device->residue += temp;

            /* Set data length with actual no of bytes associated
             * with completed IRP.
             */
            pcb_ms_device->data_len = actual_length;

            /* Transfer completion notification. */
            status = NU_USBF_USER_Tx_Done (
                                             pcb_ms_device->user,
                                             pcb_ms_device->drvr,
                                             (VOID *)pcb_ms_device,
                                             pcb_ms_device->data,
                                             pcb_ms_device->data_len,
                                             &data_out_ptr,
                                             &rem_len);
            if(status == NU_USBF_MS_IO_PENDING)
            {
                status = NU_SUCCESS;
            }
            else if (status == NU_SUCCESS)
            {

                pcb_ms_device->data     = data_out_ptr;

                /* In case of partial transfers, get the remaining
                * length.
                */
                pcb_ms_device->data_len = rem_len;

                /* if as per CBW residue is there and the transfer
                * has not ended by a short packet, and Client says
                * more data is there, we try to rope-in another
                * transfer.
                */
                if ((pcb_ms_device->residue != (UINT32)0) &&
                    (temp == (UINT32)0) &&
                    (data_out_ptr != NU_NULL))
                {
                    /* Truncate the unwanted length. */
                    if (pcb_ms_device->data_len >
                        pcb_ms_device->residue)
                    {
                         status = NU_USB_IRP_Set_Length (
                                                   pcb_irp,
                                                   pcb_ms_device->residue);
                    }
                    else
                    {
                         status = NU_USB_IRP_Set_Length (
                                                  pcb_irp,
                                                  pcb_ms_device->data_len);
                    }

                    if (status == NU_SUCCESS)
                    {
                        /* Set IRP data pointer. */
                        status = NU_USB_IRP_Set_Data (
                                                    pcb_irp,
                                                    data_out_ptr);

                        if (status == NU_SUCCESS)
                        {
                             /* Update the residue. */
                             pcb_ms_device->residue -= pcb_irp->length;

                             /* Submit Data transfer. */
                             pcb_ms_device->bot_stage = USBF_MS_BOT_STAGE_DATA;
                             status = NU_USB_PIPE_Submit_IRP (
                                                              pcb_pipe,
                                                              pcb_irp);
                        }
                    }
                }
                /* Now, CBW is processed completely. Submit IRP for
                * CSW.
                */
                else
                {

                     /* Case # 5 & 11. */
                     if(pcb_ms_device->residue != 0)
                     {
                          pcb_ms_device->cmd_failed = NU_TRUE;
                          status = NU_USB_PIPE_Stall(pcb_pipe);
                      }
                     else
                     {
                         status = NU_MSF_Submit_Status_IRP (pcb_ms_device,
                                                       (UINT8)USB_SPEC_MSF_CSW_CMD_SUCCESS);
                     }
                }
            }

            else
            {
                 if(pcb_ms_device->residue != 0)
                 {
                     /* Case # 11 and case # 5 */
                     pcb_ms_device->cmd_failed = NU_TRUE;
                     status = NU_USB_PIPE_Stall(pcb_pipe);
                 }

                 else
                 {
                     status = NU_MSF_Submit_Status_IRP (pcb_ms_device,
                                                        (UINT8)USB_SPEC_MSF_CSW_CMD_SUCCESS);
                 }
            }
        }
        else
        {
            if(pcb_ms_device->residue == 0x08)
            {
                 pcb_ms_device->residue = 0x00;
            }
            status = NU_MSF_Submit_Status_IRP (pcb_ms_device,
                                               (UINT8)USB_SPEC_MSF_CSW_CMD_FAILED);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Data_IRP_Complete
*
* DESCRIPTION
*
*        Data transfer completion Callback.
*
* INPUTS
*
*        pcb_irp                      Completed Data transfer.
*        pcb_pipe                     Pointer to the USB pipe.
*
* OUTPUTS
*
*        None.
*
**************************************************************************/
VOID NU_MSF_Data_IRP_Complete (NU_USB_PIPE *pcb_pipe,
                               NU_USB_IRP  *pcb_irp)
{

    /* Local variables. */
    NU_USBF_MS_DEVICE *ms_device_ptr;
    STATUS             status, irp_status;

    ms_device_ptr = NU_NULL;
    status = NU_USB_IRP_Get_Status (pcb_irp, &irp_status);

    /* Validate arrived IRP. */
    if (irp_status == NU_SUCCESS)
    {
        /* Get context data pointer for this IRP. */
        status = NU_USB_IRP_Get_Context (pcb_irp,
                                        (VOID **) &ms_device_ptr);
        if(status == NU_SUCCESS)
        {

            ms_device_ptr->data_irp_complete  = pcb_irp;
            ms_device_ptr->data_pipe          = pcb_pipe;
#ifdef NU_USBF_MS_TASK_MODE

            /* Set the notification that disk is connected. */
            status = NU_Set_Events (&ms_device_ptr->event_group,
                                    NU_USBF_MS_DATA_STATUS,
                                    NU_OR);

#else
            status = NU_MSF_Process_Data_And_Status(ms_device_ptr);
#endif
        }
    }
    NU_USB_ASSERT( status == NU_SUCCESS );
}

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Submit_Command_IRP
*
* DESCRIPTION
*
*        Command IRP submission. This function submits CBW IRP over Bulk
*        OUT pipe to the stack.
*
* INPUTS
*        pcb_ms_device       Pointer to the Mass Storage device.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful completion.
*        NU_USB_INVLD_ARG    Indicates that one or more arguments passed
*                            to this function are invalid.
*        NU_NOT_PRESENT      Indicates that the pipe is invalid.
*        NU_NO_MEMORY        Indicates failure of memory allocation.
*
**************************************************************************/
STATUS NU_MSF_Submit_Command_IRP (NU_USBF_MS_DEVICE *pcb_ms_device)
{

    /* Local variables. */
    STATUS      status;
    NU_USB_IRP *irp_ptr = &(pcb_ms_device->bulk_out_irp);

    /* Set IRP length to 31. CBW is always 31 bytes, as per Bulk Only
     * Transport specification.
     */
    status = NU_USB_IRP_Create (irp_ptr,
                              (UINT32)USB_SPEC_MSF_CBW_LENGTH,
                              (UINT8 *) (pcb_ms_device->cbw),
                              NU_TRUE,
                              NU_FALSE,
                              NU_MSF_Command_IRP_Complete,
                              (VOID *)pcb_ms_device,
                              0x00);

    if (status == NU_SUCCESS)
    {
        pcb_ms_device->bot_stage = USBF_MS_BOT_STAGE_CMD;
        status = NU_USB_PIPE_Submit_IRP (
                                 pcb_ms_device->out_pipe,
                                 irp_ptr);
    }

    /* Return appropriate status. */
    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Submit_Status_IRP
*
* DESCRIPTION
*
*        Status IRP submission. This function submits an IRP over Bulk IN
*        pipe to send command status wrapper(CSW) to the host.CSW indicates
*        whether previously sent command by host was completed successfully
*        or not.
*
* INPUTS
*
*        pcb_ms_device       Pointer to the Mass Storage device.
*        status              Status to be sent in the CSW.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful completion.
*        NU_USB_INVLD_ARG    Indicates that one or more args passed to this
*                            function are invalid.
*        NU_NOT_PRESENT      Indicates that the pipe is invalid.
*        NU_NO_MEMORY        Indicates failure of memory allocation.
*
**************************************************************************/
STATUS NU_MSF_Submit_Status_IRP (NU_USBF_MS_DEVICE *pcb_ms_device,
                                 UINT8              status)
{

    /* Local variables. */
    STATUS       return_status;
    NU_USB_IRP  *irp_ptr = &(pcb_ms_device->bulk_in_irp);
    MSF_BOT_CSW *csw_ptr = (pcb_ms_device->csw);

    return_status = NU_MSF_Submit_Command_IRP (pcb_ms_device);

    if(return_status == NU_SUCCESS)
    {

        /* Fill in the status and other fields of CSW as per Bulk Only
         * Transport specification of mass storage.
         */
        csw_ptr->bCSWStatus    = status;

        /* Set signature of the CSW to be sent. */
        csw_ptr->dCSWSignature = HOST_2_LE32 (USB_SPEC_MSF_CSW_SIGNATURE);

        /* Set tag of the CSW to be sent. */
        csw_ptr->dCSWTag       = HOST_2_LE32 (pcb_ms_device->cbw->dCBWTag);

        /* Set CSW residue field. */
        csw_ptr->dCSWResidue   = HOST_2_LE32 (pcb_ms_device->residue);
        /* Set length of the IRP that needs to be submitted for CSW
         * transmission. CSW is always 13 bytes according to Bulk only
         * specification.
         */

        return_status = NU_USB_IRP_Set_Length (
                                         irp_ptr,
                                         (UINT32)USB_SPEC_MSF_CSW_LENGTH);
    }
    /* Was IRP length set successfully? */
    if (return_status == NU_SUCCESS)
    {

        /* Set CSW data to be sent. */
        return_status = NU_USB_IRP_Set_Data (irp_ptr, (UINT8 *) csw_ptr);

        if (return_status == NU_SUCCESS)
        {
            /* For CSW transfers, NU_MSF_Status_IRP_Complete handles
             * transfer completion.
             */
            return_status = NU_USB_IRP_Set_Callback (
                                               irp_ptr,
                                               NU_NULL);

            /* Was IRP completion callback set successfully? */
            if (return_status == NU_SUCCESS)
            {
                /* Set mass storage device control block pointer as
                 * context.
                 */
                return_status = NU_USB_IRP_Set_Context (irp_ptr,
                                                   (VOID *)pcb_ms_device);
                if (return_status == NU_SUCCESS)
                {

                    /* Submit the IRP over Bulk IN pipe for CSW. */
                    return_status = NU_USB_PIPE_Submit_IRP (
                                                    pcb_ms_device->in_pipe,
                                                    irp_ptr);
                }
            }
        }
    }

    /* Return appropriate status. */
    return return_status;
}

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Stall_Pipes
*
* DESCRIPTION
*
*        This function stalls both the Bulk IN and Bulk OUT pipes.
*
* INPUTS
*
*        pcb_ms_device      Storage device on which, the pipes are to be
*                           stalled.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful completion.
*        NU_USB_INVLD_ARG    Indicates Pipe control is not valid.
*
**************************************************************************/
STATUS NU_MSF_Stall_Pipes (const NU_USBF_MS_DEVICE *pcb_ms_device)
{

    /* Local variables. */
    STATUS status;

    /* Stall the Bulk IN Pipe. */
    status = NU_USB_PIPE_Stall (pcb_ms_device->in_pipe);
    if(status == NU_SUCCESS)
    {
        /* Stall the Bulk OUT Pipe. */
        status = NU_USB_PIPE_Stall (pcb_ms_device->out_pipe);
    }

    /* Return appropriate status. */
    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Initialize_Device
*
* DESCRIPTION
*
*        This function Initializes the device by finding Control, Bulk IN
*        and Bulk OUT pipes on which all the USB communication takes place.
*        It is also responsible to create Bulk IN and Bulk OUT IRPs that
*        needs to be submitted over Bulk IN and Bulk OUT pipes.
*
* INPUTS
*
*        pcb_ms_device      Mass Storage device control block.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful completion of the service.
*        NU_NOT_PRESENT     Indicates that no matching pipe could be found.
*        NU_USB_INVLD_ARG   Indicates that the control block is invalid.
*
**************************************************************************/
STATUS NU_MSF_Initialize_Device (NU_USBF_MS_DEVICE *pcb_ms_device)
{

    /* Initialize status with success. */
    STATUS status = NU_SUCCESS;

    /* Dummy loop. It would be helpful in returning the function status by
     * breaking the loop as soon as any error occurs.
     */
    do
    {
        /* Find the default(control) pipe. */
        status = NU_USB_ALT_SETTG_Find_Pipe (pcb_ms_device->alt_settg,
                                             (UINT32)USB_MATCH_EP_ADDRESS,
                                             (UINT8)0,
                                             (UINT8)0,
                                             (UINT8)0,
                                             &pcb_ms_device->ctrl_pipe);
        /* Was the control pipe found? */
        if(status != NU_SUCCESS)
        {
            break;
        }

        /* Find the Bulk IN pipe. */
        status = NU_USB_ALT_SETTG_Find_Pipe (
                                            pcb_ms_device->alt_settg,
                                            (UINT32)USB_MATCH_EP_TYPE |
                                            (UINT32)USB_MATCH_EP_DIRECTION,
                                            (UINT8)0,
                                            (UINT8)USB_DIR_IN,
                                            (UINT8)USB_EP_BULK,
                                            &pcb_ms_device->in_pipe);

        /* Was the bulk in pipe found? */
        if(status != NU_SUCCESS)
        {
            /* Exit loop and return status. */
            break;
        }

        /* Find the Bulk OUT pipe. */
        status = NU_USB_ALT_SETTG_Find_Pipe (
                                            pcb_ms_device->alt_settg,
                                            (UINT32)USB_MATCH_EP_TYPE |
                                            (UINT32)USB_MATCH_EP_DIRECTION,
                                            (UINT8)0,
                                            (UINT8)USB_DIR_OUT,
                                            (UINT8)USB_EP_BULK,
                                            &pcb_ms_device->out_pipe);

        /* Was bulk out pipe found? */
        if(status != NU_SUCCESS)
        {
            break;                           /* Exit loop and return. */
        }

        /* Create the bulk out IRP. */
        status = NU_USB_IRP_Create (
                                    &pcb_ms_device->bulk_out_irp,
                                    (UINT32)0,
                                    (UINT8 *)NU_NULL,
                                    (UINT8)0,
                                    (UINT8)0,
                                     NU_NULL,
                                    (VOID *)pcb_ms_device,
                                    (UINT32)0);
        /* Bulk out IRP creation failed? */
        if(status != NU_SUCCESS)
        {
            break;                           /* Exit loop and return. */
        }

        /* Create the bulk in IRP. */
        status = NU_USB_IRP_Create (
                                    &pcb_ms_device->bulk_in_irp,
                                    (UINT32)0,
                                    (UINT8 *)NU_NULL,
                                    (UINT8)0,
                                    (UINT8)0,
                                     NU_NULL,
                                    (VOID *)pcb_ms_device,
                                    (UINT32)0);
        /* Bulk in IRP creation failed? */
        if(status != NU_SUCCESS)
        {
            break;                           /* Exit loop and return. */
        }

        /* Create the control IRPs. */
        status = NU_USB_IRP_Create (
                                   &pcb_ms_device->ctrl_irp,
                                    (UINT32)0,
                                    (UINT8 *)NU_NULL,
                                    (UINT8)0,
                                    (UINT8)0,
                                    NU_NULL,
                                    (VOID *)pcb_ms_device,
                                    (UINT32)0);

        /* If the control IRP creation fail? */
        if(status != NU_SUCCESS)
        {
            /* Exit loop and return status. */
            break;
        }

#ifdef NU_USBF_MS_TASK_MODE

        /* Release memory if acquired by the command processing task. */
        status = NU_MSF_Release_Memory(pcb_ms_device);

        /* On failure, exit loop and return status. */
        if (status != NU_SUCCESS)
        {
            break;
        }

        /* Create event and task to process mass storage specific
         * commands.
         */
        status = NU_MSF_Init_Task(pcb_ms_device);
        if (status != NU_SUCCESS)
        {
            break;
        }
#endif

    }while(0);

    /* Return appropriate status. */
    return status;
}

#ifdef NU_USBF_MS_TASK_MODE
/**************************************************************************
* FUNCTION
*
*        NU_MSF_Init_Task
*
* DESCRIPTION
*
*        This function initializes the task needed to process mass storage
*        events(connect/disconnect) and commands.
*
* INPUTS
*
*        pcb_ms_device      Mass Storage device control block.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful completion of the service.
*        NU_USB_INVLD_ARG   Invalid arguments.
*
**************************************************************************/
STATUS NU_MSF_Init_Task( NU_USBF_MS_DEVICE *pcb_ms_device)
{

    NU_USBF_MS *ms_ptr = (NU_USBF_MS *)pcb_ms_device->usb_drvr;
    STATUS status;

    do
    {

        /* Create event group. */
        status = NU_Create_Event_Group(&pcb_ms_device->event_group,
                                       "ms_event");
        /* On failure, exit loop and return status. */
        if (status != NU_SUCCESS)
        {
            break;
        }

        /* Allocating memory required for MS task's stack. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                     (UNSIGNED)(NU_USBF_MS_TASK_STACK_SIZE),
                                     &pcb_ms_device->ms_stack);
        if(status != NU_SUCCESS)
        {
            pcb_ms_device->roll_back = NU_USBF_MS_DELETE_EVENT;
            break;
        }

        /* Allocating memory required for Ms command processing task's
         * stack.
         */
        status = USB_Allocate_Object( sizeof(NU_TASK),
                                      (VOID**)&pcb_ms_device->ms_task );
        if(status != NU_SUCCESS)
        {
            pcb_ms_device->roll_back = NU_USBF_MS_MEM_STACK;
            break;
        }

        /* Initializing the whole block to 0x00 value. */
        memset (pcb_ms_device->ms_task,
                0,
                sizeof (NU_TASK));

        status = NU_Create_Task(
                           pcb_ms_device->ms_task,
                           "MS_Task",
                           NU_MSF_Process_Command,
                           (UNSIGNED)0x00,
                           pcb_ms_device,
                           pcb_ms_device->ms_stack,
                           (UNSIGNED)(NU_USBF_MS_TASK_STACK_SIZE),
                           (OPTION)NU_USBF_MS_TASK_PRIORITY,
                           (UNSIGNED)0,
                           (OPTION)NU_USBF_MS_TASK_PREEMPTION,
                           (OPTION)NU_START);
        if(status != NU_SUCCESS)
        {
            pcb_ms_device->roll_back = NU_USBF_MS_MEM_TASK;
            break;
        }

        pcb_ms_device->roll_back = NU_USBF_MS_DELETE_TASK;

    }while(0);                            /* End of dummy loop. */

    /* Return appropriate status. */
    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Release_Memory
*
* DESCRIPTION
*
*        This function deallocates the memory used by Initialize_Task
*        function.
*
* INPUTS
*
*        pcb_ms_device      Mass Storage device control block.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful completion of the service.
*        NU_USB_INVLD_ARG   Invalid arguments.
*
**************************************************************************/
STATUS NU_MSF_Release_Memory( NU_USBF_MS_DEVICE *pcb_ms_device)
{

    STATUS status = NU_USB_INVLD_ARG;

    switch (pcb_ms_device->roll_back)
    {

        case NU_USBF_MS_DELETE_TASK:

            /* All the memory acquired by Task and event group needs to be
             * deleted.
             */
            status = NU_Terminate_Task(pcb_ms_device->ms_task);
            if( status != NU_SUCCESS )
            {
                break;
            }
            status = NU_Delete_Task(pcb_ms_device->ms_task);
            if( status != NU_SUCCESS )
            {
                break;
            }

        case NU_USBF_MS_MEM_TASK:

            /* Deallocate task and stack memory also delete event group.*/
            status = USB_Deallocate_Memory(pcb_ms_device->ms_task);
            if( status != NU_SUCCESS )
            {
                break;
            }

        case NU_USBF_MS_MEM_STACK:

            /* Deallocate task stack and delete event group. */
            status = USB_Deallocate_Memory(pcb_ms_device->ms_stack);
            if( status != NU_SUCCESS )
            {
                break;
            }

        case NU_USBF_MS_DELETE_EVENT:

            /* Delete event group. */
            status = NU_Delete_Event_Group(&pcb_ms_device->event_group);
            if( status != NU_SUCCESS )
            {
                break;
            }

        default:

            /* Return success. */
            status = NU_SUCCESS;
        break;
    }

    /* Return status. */
    return status;
}
#endif

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Unstall_Pipes
*
* DESCRIPTION
*
*        This function unstalls Bulk IN and Bulk OUT pipes if they are
*        stalled.
*
* INPUTS
*
*        pcb_ms_device      Storage device on which, the pipes are to be
*                           stalled.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful completion.
*        NU_USB_INVLD_ARG    Indicates Pipe control is not valid.
*
**************************************************************************/
STATUS NU_MSF_Unstall_Pipes (NU_USBF_MS_DEVICE *pcb_ms_device)
{

    /* Local variables. */
    STATUS  status;
    BOOLEAN is_stalled;

    status = NU_USB_PIPE_Get_Is_Stalled (pcb_ms_device->in_pipe,
                                         &is_stalled);
    if((status == NU_SUCCESS) && (is_stalled == NU_TRUE))
    {
        /* Unstall the Bulk BULK IN Pipe. */
        status = NU_USB_PIPE_Unstall (pcb_ms_device->in_pipe);
    }
    
    if( status == NU_SUCCESS )
    {
        status = NU_USB_PIPE_Get_Is_Stalled (pcb_ms_device->out_pipe,
                                             &is_stalled);
        if((status == NU_SUCCESS) && (is_stalled == NU_TRUE))
        {
            /* Unstall the Bulk OUT Pipe. */
            status = NU_USB_PIPE_Unstall (pcb_ms_device->out_pipe);
        }
    }

    /* Return appropriate status. */
    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Clear_Feature_Callback
*
* DESCRIPTION
*
*        This function handles clear feature callback.
*
* INPUTS
*
*        pcb_ms_device      Pointer to mass storage device control block.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful completion.
*        NU_USB_INVLD_ARG    Indicates that one or more args passed to this
*                            function are invalid.
*        NU_NOT_PRESENT      Indicates that the pipe is invalid.
*        NU_NO_MEMORY        Indicates failure of memory allocation.
*
**************************************************************************/
STATUS NU_MSF_Clear_Feature_Callback(NU_USBF_MS_DEVICE *pcb_ms_device)
{

    STATUS status = NU_SUCCESS;

    /* Invalid CBW received, stall BULK pipes. */
    if( pcb_ms_device->invalid_cbw == NU_TRUE)
    {
        status = NU_MSF_Stall_Pipes(pcb_ms_device);
    }

    /* Submit CSW with failure/success(both are allowed from BOT specs.).
     */
    if(pcb_ms_device->cmd_failed == NU_TRUE)
    {
        status = NU_MSF_Submit_Status_IRP(pcb_ms_device,
                                         (UINT8)USB_SPEC_MSF_CSW_CMD_FAILED);
        pcb_ms_device->cmd_failed = NU_FALSE;
    }

    /* Phase error occurred, submit CSW with phase error status. */
    if(pcb_ms_device->phase_error == NU_TRUE)
    {
        status = NU_MSF_Submit_Status_IRP(pcb_ms_device,
                                      (UINT8)USB_SPEC_MSF_CSW_PHASE_ERROR);

        pcb_ms_device->phase_error = NU_FALSE;
    }

    /* Return appropriate status. */
    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_MSF_Is_Valid_CBW
*
* DESCRIPTION
*
*        This fuction validates(validation and meaninfulness) of passed CBW.
*
* INPUTS
*
*        pcb_irp           Pointer to the IRP Transfer that has been
*                          complete.
*
* OUTPUTS
*
*        MS_CBW_OK  Indicates CBW is valid.
*        MS_CBW_BAD Indicates invalid CBW.
*
**************************************************************************/
UINT8 NU_MSF_Is_Valid_CBW (NU_USB_IRP  *pcb_irp)
{
    /* Local Variables */
    UINT32             actual_length;
    STATUS             status;
    UINT8              cbw_valid;
    NU_USBF_MS_DEVICE *ms_device_ptr = NULL;
    MSF_BOT_CBW       *cbw_ptr = NULL;
    
    do
    {
        cbw_valid = MS_CBW_OK;
        status = NU_USB_IRP_Get_Context (pcb_irp, (VOID **)&ms_device_ptr);
        if ( status != NU_SUCCESS )
        {
            cbw_valid = MS_CBW_BAD;
            break;
        }
        cbw_ptr = (ms_device_ptr->cbw);

        /* Endian conversion */
        cbw_ptr->dCBWTag   =
               LE32_2_HOST(cbw_ptr->dCBWTag);
        cbw_ptr->dCBWSignature =
               LE32_2_HOST(cbw_ptr->dCBWSignature);
        cbw_ptr->dCBWTransferLength =
               LE32_2_HOST(cbw_ptr->dCBWTransferLength);

        /* CBW Validation */
        /* Length Check */
        status = NU_USB_IRP_Get_Actual_Length(pcb_irp, &actual_length);
        if ( actual_length != (UINT32)USB_SPEC_MSF_CBW_LENGTH )
        {
            cbw_valid = MS_CBW_BAD;
            break;
        }
        /* Signature Check */
        if ( cbw_ptr->dCBWSignature != USB_SPEC_MSF_CBW_SIGNATURE )
        {
            cbw_valid = MS_CBW_BAD;
            break;
        }

        /* CBW Meaningfulness  Check */
        /* Reserved Bits Check */
        if ( (cbw_ptr->bCBWLUN & USB_SPEC_MSF_CBW_RESERVED_BITS_MASK) != (UINT8)0x00 )
        {
            cbw_valid = MS_CBW_BAD;
            break;
        }
        /* Valid LUN Check */
        if ( (cbw_ptr->bCBWLUN & USB_SPEC_MSF_CBW_LUN_MASK) >
              *(ms_device_ptr->max_lun) )
        {
            cbw_valid = MS_CBW_BAD;
            break;
        }

        /* Command Block Length Check */
        if ( cbw_ptr->bCBWCBLength > USB_SPEC_MSF_CBW_CMD_LEN_MAX )
        {
            cbw_valid = MS_CBW_BAD;
            break;
        }

    }while(0);

    return cbw_valid;
}

/************************* End Of File ***********************************/

