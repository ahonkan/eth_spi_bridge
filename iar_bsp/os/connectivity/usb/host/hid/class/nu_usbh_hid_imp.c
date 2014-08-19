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
*       nu_usbh_hid_imp.c
*
*
* COMPONENT
*
*       Nucleus USB Host HID Base Class Driver.
*
* DESCRIPTION
*
*       This file contains core routines for Nucleus USB Host Stack's
*       HID class driver component.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       USBH_HID_Cntrl_Complete             Control transfer completion
*                                           callback.
*       USBH_HID_Connect_Users              New device notification to
*                                           users.
*       USBH_HID_Decode_Global_Item         Report descriptor parsing.
*       USBH_HID_Decode_Local_Item          Report descriptor parsing.
*       USBH_HID_Decode_Main_Item           Report descriptor parsing.
*       USBH_HID_Decode_Report_Desc         Report descriptor parsing.
*       USBH_HID_Extract_Item_Prefix        Report descriptor parsing.
*       USBH_HID_Get_HID_Desc               Retrieves HID class descriptor.
*       USBH_HID_Get_Report_Desc            Retrieves report descriptor.
*       USBH_HID_Handle_Main_Data_Item      Report descriptor parsing.
*       USBH_HID_Hnd_Main_Non_Data_Item     Report descriptor parsing.
*       Usbh_HID_Init_Item_St_Table         Report descriptor parsing.
*       USBH_HID_Intr_Complete              Interrupt transfer completion
*                                           callback.
*       USBH_HID_Set_Report_Protocol        Sets the device to use reports.
*       USBH_HID_Task                       HID task.
*       USBH_HID_Find_Device_For_IN_IRP     Finds device pointer for the
*                                           device for which interrupt IRP
*                                           completion callback is
*                                           received.
*
* DEPENDENCIES
*
*       nu_usb.h                            USB Definitions.
*
**************************************************************************/

/* ==============  USB Include Files =================================  */

#include    "connectivity/nu_usb.h"

/* ==========================  Functions ============================== */

/*************************************************************************
* FUNCTION
*
*       USBH_HID_Connect_Users
*
* DESCRIPTION
*
*       Finds the User with matching usages from all the registered Users
*       to this HID class driver.
*
* INPUTS
*
*       cb             Pointer to the HID Driver control block.
*       hid_device     Hid device for matching.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion of service
*       NU_USB_INVLD_ARG    Indicates that the device has not been claimed
*                           by any user.
*
*
*************************************************************************/
STATUS  USBH_HID_Connect_Users (NU_USBH_HID      *cb,
                                USBH_HID_DEVICE  *hid_device)
{
    UINT8 i, j, k;
    UINT32 l;
    STATUS status;
    UNSIGNED num_users;
    UINT8 num_usages;
    BOOLEAN accepted = NU_FALSE;
    BOOLEAN user_found = NU_FALSE;
    NU_USB_USER *users[NU_USB_MAX_USERS];
    NU_USBH_HID_USAGE usages[NU_USB_MAX_HID_USAGES];

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(hid_device);

    /* Find the number of users registered with this class driver. */
    num_users = NU_USB_DRVR_Get_Users_Count ((NU_USB_DRVR *) cb);

    /* Check against maximum possible number of users. */
    if (num_users > NU_USB_MAX_USERS)
        num_users = NU_USB_MAX_USERS;

    /* Get the list of users from parent NU_USB_DRVR control block. */
    num_users = NU_USB_DRVR_Get_Users ((NU_USB_DRVR *) cb, users,
                                        num_users);

    /* Search for the suitable user from the list. */
    for (i = 0; i < num_users; i++)
    {
        if(i >= NU_USB_MAX_USERS)
        {
            status = NU_USB_INVLD_ARG;
            break;
        }

        /* Get number of Usages USER can serve. */
        status = NU_USBH_HID_USER_Get_Num_Usages(
                                            (NU_USBH_HID_USER *)users[i],
                                            cb, &num_usages);

        if (status != NU_SUCCESS)
        {
            break;
        }

        /* Check against maximum possible number of users. */
        if (num_usages > NU_USB_MAX_HID_USAGES)
            num_usages = NU_USB_MAX_HID_USAGES;

        status =
        NU_USBH_HID_USER_Get_Usages( (NU_USBH_HID_USER *)users[i],
                                      cb,
                                      &usages[0],
                                      num_usages);
        if (status != NU_SUCCESS)
        {
            break;
        }

        accepted = NU_FALSE;

        for (j = 0; j< hid_device->num_items; j++)
        {
            if(hid_device->usages[j].user != NU_NULL)
            {
                continue;
            }

            /* For each of the usages reported by the user. */
            for (k=0; k< num_usages; k++)
            {
                /* Check if the item usage page matches the one supported
                 * by the user.
                 */
                if (usages[k].usage_page ==
                   hid_device->usages[j].item->usage_page)
                {
                    /* Now that usage page matches, check the usage_id. */

                    /* if the user says he supports all ids under this
                     * page, then he is the user.
                     */

                    if (usages[k].usage_id == 0xFFFF)
                    {
                        hid_device->usages[j].user =
                        (NU_USBH_HID_USER*)users[i];
                        accepted = NU_TRUE;
                    }
                    else
                    {
                        /* if the device reports the usage_ids using
                         * usage_min and usage_max tags, then check if
                         * user supported usage_id is some where in
                         * between min and max.
                         */

                        if (hid_device->usages[j].item->num_usage_ids == 0)
                        {
                            if ((usages[k].usage_id >=
                                hid_device->usages[j].item->usage_min)
                            &&
                            (usages[k].usage_id <=
                            hid_device->usages[j].item->usage_max))
                            {
                                hid_device->usages[j].user =
                                (NU_USBH_HID_USER*)users[i];
                                accepted = NU_TRUE;
                            }
                        }
                        else
                        {
                            /* if the device reports all usages
                             * independently, in the same item, then check
                             * if the user supported usage_id is reported
                             * by the device.
                             */
                            for (l=0;
                            l < hid_device->usages[j].item->num_usage_ids;
                            l++)
                            {
                                if (usages[k].usage_id ==
                                hid_device->usages[j].item->usage_ids[l])
                                {
                                    hid_device->usages[j].user =
                                    (NU_USBH_HID_USER*)users[i];
                                    accepted = NU_TRUE;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (accepted == NU_TRUE)
        {
            /* User driver, will you service this device ? */
            status = NU_USB_USER_Connect (users[i],
                                         (NU_USB_DRVR*)cb, hid_device);

            /* if not, reset all items. */
            if (status != NU_SUCCESS)
            {
                for (j = 0; j< hid_device->num_items; j++)
                {
                    if (hid_device->usages[j].user ==
                      (NU_USBH_HID_USER*)users[i])
                    {
                        hid_device->usages[j].user = NU_NULL;
                    }
                }
            }
            else
            {
                user_found = NU_TRUE;
            }
        }
    }

    return ( user_found == NU_TRUE ? NU_SUCCESS : NU_USB_NOT_SUPPORTED );
}

/************************************************************************
* FUNCTION
*
*       USBH_HID_Get_HID_Desc
*
* DESCRIPTION
*
*       This function retrieves the HID (class specific) descriptor for
*       the device.
*
* INPUTS
*
*       cb          Pointer to  driver control block.
*       alt_stg     Current alternate setting of the device.
*       hid_dev     HID device control block.
*
* OUTPUTS
*
*       NU_SUCCESS      Service executed successfully.
*       NU_INVALID_DESC Class specific descriptor incorrect.
*
*************************************************************************/
STATUS USBH_HID_Get_HID_Desc  ( NU_USBH_HID         *cb,
                                NU_USB_ALT_SETTG    *alt_stg,
                                USBH_HID_DEVICE     *hid_dev)
{
    STATUS status = NU_SUCCESS;
    UINT32 length;
    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(alt_stg);
    NU_USBH_HID_ASSERT(hid_dev);

    status = NU_USB_ALT_SETTG_Get_Class_Desc (alt_stg,
            (UINT8 **)&(hid_dev->hid_desc), &length);
    if ( status == NU_SUCCESS )
    {
        if ((!hid_dev->hid_desc) ||
            (hid_dev->hid_desc->bNumDescriptors == 0))
        {
            status = NU_USB_INVLD_DESC;
        }
    }
    return (status);
}

/**********************************************************************
* FUNCTION
*       USBH_HID_Task
*
* DESCRIPTION
*
*       This is the task's entry function. It waits for the
*       ev_lock to be available and then processes the events
*       associated.
*
* INPUTS
*
*       argc                            Not used
*       argv                            Not used
*
* OUTPUTS
*       None
*
*
************************************************************************/
VOID USBH_HID_Task (UNSIGNED  argc,
                    VOID     *argv)
{
    NU_USB_IRP *irp;
    UINT32 actual_length;
    NU_USBH_HID *hidcb = (NU_USBH_HID *) argv;
    INT i, j;
    USBH_HID_DEVICE *hidDev;
    STATUS irp_status, status;
    UINT8 users_count =0;
    NU_USBH_HID_USER *users_found[NU_USB_MAX_USERS];

    while (1)
    {
        status = NU_Obtain_Semaphore (  &(hidcb->intr_irp_complete),
                                        NU_SUSPEND  );
        if ( status != NU_SUCCESS )
        {
            /* Fatal error. */
            break;
        }

        users_count = 0;

        /* Find the device. */
        hidDev = USBH_HID_Find_Device_For_IN_IRP(hidcb);
        if(hidDev)
        {
            status = NU_Obtain_Semaphore (&(hidcb->ev_lock), NU_SUSPEND);
            if ( status != NU_SUCCESS )
            {
                /* Fatal error. */
                break;
            }

            hidDev->int_irp_completed  = 0;

            status = NU_Release_Semaphore (&(hidcb->ev_lock));
            if ( status != NU_SUCCESS )
            {
                /* Fatal error. */
                break;
            }

            irp = &(hidDev->interrupt_irp);

            status = NU_USB_IRP_Get_Status(irp, &irp_status);
            if( ( status == NU_SUCCESS ) && ( irp_status == NU_SUCCESS ))
            {
                status = NU_USB_IRP_Get_Actual_Length (irp, &actual_length);
                if ( status == NU_SUCCESS )
                {
                    for (i=0; i<hidDev->num_items; i++)
                    {
                        if((hidDev->usages[i].item->report_id == 0x00) ||
                           (hidDev->usages[i].item->report_id ==
                                                hidDev->raw_report[0]))
                        {
                            if (hidDev->usages[i].user)
                            {
                                for (j=0; j<users_count; j++)
                                {
                                    if(users_found[j] ==
                                                    hidDev->usages[i].user)
                                    {
                                        break;
                                    }
                                }

                                /* New User found. */
                                if (j == users_count)
                                {
                                    users_found[users_count++] =
                                                    hidDev->usages[i].user;
                                }
                            }
                        }
                    }
                }

                /* Give Notify callback to all users associated with the
                 * report.
                 */
                for (i =0; i<users_count; i++)
                {
                    status = NU_USBH_HID_USER_Notify_Report(users_found[i],
                                                   hidcb, hidDev,
                                                   hidDev->raw_report,
                                                   actual_length);
                    NU_USBH_HID_ASSERT( status == NU_SUCCESS );
                }
            }

            if(hidDev->Connected == NU_TRUE)
            {
                /* Submit another IRP. */
                status = NU_USB_PIPE_Submit_IRP(hidDev->interrupt_pipe,
                                                &(hidDev->interrupt_irp));
                NU_USBH_HID_ASSERT( status == NU_SUCCESS );
            }
            else
            {
                if(hidDev->raw_report)
                {
                    /* Free allocated memory. */
                    status = USB_Deallocate_Memory (hidDev->raw_report);
                    NU_USBH_HID_ASSERT(hidDev->raw_report);

                    hidDev->raw_report = NU_NULL;
                }

                for(i = 0; i < hidDev->num_items; i++)
                {
                    if ( hidDev->usages[i].item )
                    {
                        status = USB_Deallocate_Memory (hidDev->usages[i].item);
                        NU_USBH_HID_ASSERT(hidDev->usages[i].item);
                        hidDev->usages[i].item = NU_NULL;
                    }
                }

                for(i = 0; i < hidDev->num_of_coll; i++)
                {
                    if ( hidDev->collection[i] )
                    {
                        status = USB_Deallocate_Memory (hidDev->collection[i]);
                        NU_USBH_HID_ASSERT( status == NU_SUCCESS );
                        hidDev->collection[i] = NU_NULL;
                    }
                }

                status = NU_Delete_Semaphore (&(hidDev->cntrl_irp_complete));
                NU_USBH_HID_ASSERT( status == NU_SUCCESS );

                status = NU_Delete_Semaphore (&(hidDev->hid_lock));
                NU_USBH_HID_ASSERT( status == NU_SUCCESS );

                /* Remove the device Structure from the List... */
                NU_Remove_From_List ((CS_NODE **) & hidcb->dev_list_head,
                                     (CS_NODE *) hidDev);

                /* Deallocate buffer for control transfers. */
                status = USB_Deallocate_Memory (hidDev->control_buffer);

                /* ...and Deallocate DEVICE structure. */
                status = USB_Deallocate_Memory (hidDev);
                NU_USBH_HID_ASSERT( status == NU_SUCCESS );
            }
        }
    }

    /* Control should never come here in normal case. */
    NU_USBH_HID_ASSERT( status == NU_SUCCESS );
}

/*************************************************************************
* FUNCTION
*
*       USBH_HID_Cntrl_Complete
*
* DESCRIPTION
*
*       This function is called when an IRP on control pipe of a
*       HID device gets completed. It takes the context
*       information from the IRP and wakes up the task waiting on
*       submission of IRP.
*
* INPUTS
*
*       pipe            Pointer to the Pipe control block.
*       irp             Pointer to IRP control block.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID USBH_HID_Cntrl_Complete (  NU_USB_PIPE     *pipe,
                                NU_USB_IRP      *irp)
{
    STATUS          status;
    USBH_HID_DEVICE *hid_dev = NU_NULL;

    NU_USBH_HID_ASSERT (irp);

    /* Remove unused parameter warnings. */
    NU_UNUSED_PARAM(pipe);
    NU_UNUSED_PARAM(status);

    status = NU_USB_IRP_Get_Context (irp, (VOID **) &hid_dev);
    NU_USBH_HID_ASSERT( status == NU_SUCCESS );

    /* Wake up the command thread. */
    status = NU_Release_Semaphore (&(hid_dev->cntrl_irp_complete));
    NU_USBH_HID_ASSERT( status == NU_SUCCESS );
}

/*************************************************************************
* FUNCTION
*
*   USBH_HID_Intr_Complete
*
* DESCRIPTION
*
*   This function is called when an IRP on the Interrupt pipe gets
*   completed. It checks for the status and actual length of the
*   transmitted IRP.
*
* INPUTS
*
*   pipe            Pointer to the Pipe control block.
*   irp             Pointer to IRP control block.
*
* OUTPUTS
*
*   None
*
*
*************************************************************************/
VOID    USBH_HID_Intr_Complete ( NU_USB_PIPE    *pipe,
                                 NU_USB_IRP     *irp)
{
    NU_USBH_HID     *hidcb;
    USBH_HID_DEVICE *hid_dev = NU_NULL;
    STATUS          status;

    NU_USBH_HID_ASSERT (irp);

    /* Remove unused parameter warnings. */
    NU_UNUSED_PARAM(pipe);
    NU_UNUSED_PARAM(status);

    status = NU_USB_IRP_Get_Context (irp, (VOID **) &hid_dev);
    NU_USBH_HID_ASSERT( status == NU_SUCCESS );

    hidcb = (NU_USBH_HID *)(hid_dev->drvr);

    /* Establish Mutual exclusion.   */
    status = NU_Obtain_Semaphore (&(hidcb->ev_lock), NU_SUSPEND);
    NU_USBH_HID_ASSERT( status == NU_SUCCESS );

    /* Set interrupt IRP completed flag. It will be reset when we re-submit
     * interrupt IRP in USBH_HID_TASK.
     */

    hid_dev->int_irp_completed = 1;


    /* relinquish Mutual exclusion.   */
    status = NU_Release_Semaphore (&(hidcb->ev_lock));
    NU_USBH_HID_ASSERT( status == NU_SUCCESS );

    /* Wakeup the HID task.    */
    status = NU_Release_Semaphore (&(hidcb->intr_irp_complete));
    NU_USBH_HID_ASSERT( status == NU_SUCCESS );
}

/*************************************************************************
* FUNCTION
*
*       USBH_HID_Get_Report_Desc
*
* DESCRIPTION
*
*       This function gets the report descriptor of the HID device and
*       parses it. It fills in the parsed information in the device
*       structure.
*
* INPUTS
*
*       cb             Pointer to the HID Driver control block.
*       hidDev         Pointer to the device structure.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_USB_INVLD_DESC       Indicates a badly formed/illegal report
*                               descriptor.
*
*************************************************************************/
STATUS USBH_HID_Get_Report_Desc(NU_USBH_HID     *cb,
                                USBH_HID_DEVICE *hidDev)
{
    NU_USBH_CTRL_IRP    *irp = NU_NULL;
    STATUS              irp_status, status, internal_sts = NU_SUCCESS;
    UINT8               *raw_report_desc = NU_NULL;
    UINT16              raw_desc_length;
    UINT32              actual_length;
    UINT8               intf_num, i, rollback = 0;

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(hidDev);

    /* Find the first report descriptor size from HID descriptor. */
    for (i=0; i<hidDev->hid_desc->bNumDescriptors; i++)
    {
        if(hidDev->hid_desc->sub_desc[i][0] == USBH_HID_DESCTYPE_REPORT)
        {
            break;
        }
    }

    if (i >= hidDev->hid_desc->bNumDescriptors)
    {
        return (NU_USB_INVLD_DESC);
    }

    /* Allocate memory for the NU_USBH_CTRL_IRP structure */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,sizeof(NU_USBH_CTRL_IRP),(VOID**)&irp);

    if (status != NU_SUCCESS)
    {
    	return (status);
    }

    do
    {
        raw_desc_length = (UINT8)hidDev->hid_desc->sub_desc[i][1];
        raw_desc_length |= ((UINT8)hidDev->hid_desc->sub_desc[i][2]) << 8;

        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     raw_desc_length,
                                     (VOID**)&raw_report_desc);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        status = NU_USB_INTF_Get_Intf_Num (hidDev->intf, &intf_num);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        /* Form the control request. */
        status = NU_USBH_CTRL_IRP_Create (irp,
            raw_report_desc,                /* data. */
            USBH_HID_Cntrl_Complete,        /* callback function. */
            hidDev,                         /* context. */
            USBH_HID_BMREQTYPE_GET_DESC,    /* bmRequestType.*/
            USBH_HID_BREQUEST_GET_DESC,     /* bRequest. */
            HOST_2_LE16 (USBH_HID_DESCTYPE_REPORT << 8),    /* wValue. */
            HOST_2_LE16 (intf_num),         /* wIndex. */
            HOST_2_LE16 (raw_desc_length)); /* wLength. */
        if ( status != NU_SUCCESS )
        {
            break;
        }

        status = NU_Obtain_Semaphore (&(hidDev->hid_lock), NU_SUSPEND);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        /* Submit the IRP.   */
        status = NU_USB_PIPE_Submit_IRP (hidDev->control_pipe,
                                                    (NU_USB_IRP *)irp);
        if ( status != NU_SUCCESS )
        {
            rollback = 1;
            break;
        }

        status = NU_Release_Semaphore (&(hidDev->hid_lock));
        if ( status != NU_SUCCESS )
        {
            break;
        }

        /* Wait for the the IRP to be complete.  */
        status = NU_Obtain_Semaphore (  &(hidDev->cntrl_irp_complete),
                                        NU_SUSPEND  );
        if ( status != NU_SUCCESS )
        {
            break;
        }

        status = NU_USB_IRP_Get_Status ((NU_USB_IRP *)irp, &irp_status);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        if (irp_status == NU_SUCCESS)
        {
            status = NU_USB_IRP_Get_Actual_Length(  (NU_USB_IRP*)irp,
                                                    &actual_length  );
            if ( status == NU_SUCCESS )
            {
                status = USBH_HID_Decode_Report_Desc(hidDev,
                                                    raw_report_desc,
                                                    actual_length);
            }
        }
        else
        {
            status = irp_status;
        }
    }while(0);

    switch(rollback)
    {
        case 1: NU_Release_Semaphore (&(hidDev->hid_lock));
        default:
        {
            if ( raw_report_desc )
            {
                USB_Deallocate_Memory(raw_report_desc);
            }
            break;
        }
    }

    /* Deallocate memory for the NU_USBH_CTRL_IRP structure */
    if (irp != NU_NULL)
    {
    	internal_sts = USB_Deallocate_Memory(irp);
    }

    return (internal_sts == NU_SUCCESS ? status : internal_sts);
}

/*************************************************************************
* FUNCTION
*
*       USBH_HID_Set_Report_Protocol
*
* DESCRIPTION
*
*       This function sets the device to use a report protocol (as opposed
*       to boot protocol).
*
* INPUTS
*
*       cb             Pointer to the HID Driver control block.
*       hidDev         Pointer to the device structure.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*       NU_USB_STALL_ERR   Indicates a stall returned for the transfer
*       NU_USB_INVLD_ARG   Indicates an error in completion of transfer.
*
*
*************************************************************************/
STATUS USBH_HID_Set_Report_Protocol(NU_USBH_HID     *cb,
                                    USBH_HID_DEVICE *hidDev)
{
    NU_USBH_CTRL_IRP    *irp = NU_NULL;
    STATUS              irp_status, status, internal_sts = NU_SUCCESS;
    UINT8               intf_num, rollback = 0;

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(hidDev);

    /* Allocate memory for the NU_USBH_CTRL_IRP structure */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,sizeof(NU_USBH_CTRL_IRP),(VOID**)&irp);

    if (status != NU_SUCCESS)
    {
    	return (status);
    }

    do
    {
        status = NU_USB_INTF_Get_Intf_Num (hidDev->intf, &intf_num);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        /* Form the control request. */
        status = NU_USBH_CTRL_IRP_Create (irp,
                               /* data . */
                               NU_NULL,
                               /* callback function. */
                               USBH_HID_Cntrl_Complete,
                               /* context. */
                               hidDev,
                               /* bmRequestType. */
                               USBH_HID_BMREQTYPE_HID_SET,
                               /* bRequest. */
                               USBH_HID_BREQUEST_SET_PROTOCOL,
                               /* wValue. */
                               HOST_2_LE16 (USBH_HID_WVALUE_REPORT_PROTOCOL),
                               /* wIndex. */
                               HOST_2_LE16 (intf_num),
                               /* wLength. */
                               0);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        status = NU_Obtain_Semaphore (&(hidDev->hid_lock), NU_SUSPEND);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        /* Submit the IRP.   */
        status = NU_USB_PIPE_Submit_IRP (hidDev->control_pipe,
                                                    (NU_USB_IRP *)irp);
        if ( status != NU_SUCCESS )
        {
            rollback = 1;
            break;
        }

        status = NU_Release_Semaphore (&(hidDev->hid_lock));
        if ( status != NU_SUCCESS )
        {
            break;
        }

        /* Wait for the the IRP to be complete.  */
        status = NU_Obtain_Semaphore (&(hidDev->cntrl_irp_complete),
                                                            NU_SUSPEND);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        /* ...and return status. */
        status = NU_USB_IRP_Get_Status ((NU_USB_IRP *)irp, &irp_status);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        status = irp_status;
    }while(0);

    switch(rollback)
    {
        case 1: NU_Release_Semaphore (&(hidDev->hid_lock));
        default: break;
    }

    /* Deallocate memory for the NU_USBH_CTRL_IRP structure */
    if (irp != NU_NULL)
    {
    	internal_sts = USB_Deallocate_Memory(irp);
    }

    return (internal_sts == NU_SUCCESS ? status : internal_sts);
}

/*************************************************************************
* FUNCTION
*
*       USBH_HID_Decode_Report_Desc
*
* DESCRIPTION
*
*       This function decodes, parses the report descriptor sent by the
*       device and fills in the device structure with relevant
*       information.
*
* INPUTS
*
*       hid_device         Pointer to the device structure
*       report_dscr        Report descriptor
*       report_dscr_length Report descriptor length
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_USB_UNKNOWN_ERR      Indicates an error in decoding.
*
*
*************************************************************************/
STATUS  USBH_HID_Decode_Report_Desc (USBH_HID_DEVICE *hid_device,
                                     UINT8           *report_dscr,
                                     UINT32           report_dscr_length)
{
    USBH_HID_ITEM_PREFIX item_prefix;
    USBH_HID_ITEM_STATE_TABLE item_state_table;
    UINT8 *raw_report_dscr = report_dscr;
    INT8 status = NU_SUCCESS;

    NU_USBH_HID_ASSERT(hid_device);
    NU_USBH_HID_ASSERT(report_dscr);
    NU_USBH_HID_ASSERT(report_dscr_length);

    /* Item_state_table is used to store state of each and every item. */
    status =  Usbh_HID_Init_Item_St_Table (&item_state_table, hid_device);
    if ( status == NU_SUCCESS )
    {
        /* Initialize array of report info in device structure. */
        memset(hid_device->input_report_info,0,
                                            sizeof(USBH_HID_REPORT_INFO));
        memset(hid_device->output_report_info,0,
                                            sizeof(USBH_HID_REPORT_INFO));
        memset(hid_device->feature_report_info,0,
                                            sizeof(USBH_HID_REPORT_INFO));

        /* Control comes here means item_prefix as well as item state table
         * is initialized. Now it can start fetching items one by one and
         * storing in item state table.
         */
        while (report_dscr_length > 0x00)
        {
            raw_report_dscr = USBH_HID_Extract_Item_Prefix(raw_report_dscr,
                                                (UINT32*)&report_dscr_length,
                                                 &item_prefix);
            if (!raw_report_dscr)
            {
                /* There is no item left to decode from report descriptor.
                 */
                break;
            }

            switch (item_prefix.bType)
            {
                case USBH_HID_ITEM_TYPE_MAIN:
                    status = USBH_HID_Decode_Main_Item (&item_state_table,
                                                        &item_prefix);
                    if ( status == NU_SUCCESS )
                    {
                        /* After every MAIN item it has to reset local
                         * items. */
                        memset(&item_state_table.local_items, 0,
                                sizeof(item_state_table.local_items));
                    }
                    break;
                case USBH_HID_ITEM_TYPE_GLOBAL:
                    status = USBH_HID_Decode_Global_Item(&item_state_table,
                                                         &item_prefix);
                    break;
                case USBH_HID_ITEM_TYPE_LOCAL:
                    status = USBH_HID_Decode_Local_Item(&item_state_table,
                                                        &item_prefix);
                    break;
                case USBH_HID_ITEM_TYPE_RESERVED:
                    break;
                default:
                    break;
            }/* switch (item_prefix.bType) */

            if (status != NU_SUCCESS)
            {
                break;
            }
        }/* End of while (report_dsr_length > 0x00) loop.  */
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       Usbh_HID_Init_Item_St_Table
*
* DESCRIPTION
*
*       This function initializes the state table used for decoding.
*
* INPUTS
*
*       hid_device         Pointer to the device structure.
*       item_state_table   State table.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion of the service.
*
*
*************************************************************************/
STATUS Usbh_HID_Init_Item_St_Table (
                       USBH_HID_ITEM_STATE_TABLE    *item_state_table,
                       USBH_HID_DEVICE              *hid_device)
{
    UINT8 index;

    NU_USBH_HID_ASSERT(item_state_table);
    NU_USBH_HID_ASSERT(hid_device);

    item_state_table->hid_device = hid_device;
    item_state_table->coll_ptr =0x00;

    for(index = 0; index <10; index++)
    {
        item_state_table->global_storage[index] = 0x00;
        if ( index >= 1 && index <= 5)
        {
            /* Global tag between from 1 to 5 has sign value. */
            item_state_table->global_storage_sign[index] = 1;
        }
        else
        {
            /* Global tag for 0 and 6,7,8,9 has unsigned value. */
            item_state_table->global_storage_sign[index] = 0;
        }
    }

    memset(&item_state_table->local_items, 0,
            sizeof(item_state_table->local_items));

    item_state_table->global_storage_stack_top = 0;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       USBH_HID_Extract_Item_Prefix
*
* DESCRIPTION
*
*       This function extracts an item from the descriptor and fills in
*       the item_prefix.
*
* INPUTS
*
*       raw_report_dscr    Report descriptor.
*       report_dscr_length Report descriptor length.
*       item_prefix        Item prefixes are stored here.
*
* OUTPUTS
*
*       Pointer to the next item in the descriptor
*       NU_NULL On error.
*
*************************************************************************/
UINT8 *USBH_HID_Extract_Item_Prefix(
                                  UINT8                *raw_report_dscr,
                                  UINT32               *report_dscr_length,
                                  USBH_HID_ITEM_PREFIX *item_prefix)
{
    UINT8 length;
    UINT8 data1 = 0;
    UINT8 data2 = 0;
    UINT8 data3 = 0;
    UINT32 data = 0;

    NU_USBH_HID_ASSERT(raw_report_dscr);
    NU_USBH_HID_ASSERT(report_dscr_length);
    NU_USBH_HID_ASSERT(item_prefix);

    /* First byte tells about Tag, Size and Type of an item. */
    item_prefix->item_prefix = *raw_report_dscr++;
    (*report_dscr_length)--;

    /* Start from LSB : First 2 bits is size, Next 2 bits is bType,
     * Next 4 bits represent bTag.
     */
    item_prefix->bSize = (item_prefix->item_prefix) & 0x03;
    item_prefix->bType  = ((item_prefix->item_prefix) & 0x0C) >> 2;
    item_prefix->bTag   = ((item_prefix->item_prefix) & 0xF0) >> 4;

    if(item_prefix->bTag != USBH_HID_ITEM_LONG)
    {
        if (((*report_dscr_length) < item_prefix->bSize ) ||
             (item_prefix->bSize > 0x03 ))
        {
            /* Error Condition  */
            return (NU_NULL);
        }

        if (item_prefix->bSize == 0x00)
        {
            item_prefix->item_data = 0x00;
        }
        else
        {
            /* For bSize 1,2,3 number of bytes should be 1, 2, 4. We are
             * storing signed as well as unsigned data so that retrieval
             * will be easy.
             */
            length = (1 << (item_prefix->bSize - 1));
            data  = raw_report_dscr[0];

            if(length > 1)
            {
                data1 = raw_report_dscr[1];
                data |= (data1 << 8);

                if(length == 4)
                {
                    data2 = raw_report_dscr[2];
                    data |= (data2 << 16);
                    data3 = raw_report_dscr[3];
                    data |= (data3 << 24);
                }
            }
            item_prefix->item_signed_data = data;
            item_prefix->item_data = ((item_prefix->item_signed_data));

            raw_report_dscr += length;
            *(report_dscr_length) -= length;
        }
        return (raw_report_dscr);
    }
    else
    {
        return (NU_NULL);
    }
}

/*************************************************************************
* FUNCTION
*
*       USBH_HID_Decode_Global_Item
*
* DESCRIPTION
*
*       This function decodes a global item and stores it in the state
*       table.
*
* INPUTS
*
*       item_state_table   State table
*       item_prefix        Item prefix of the item
*
* OUTPUTS
*
*       NU_SUCCESS         on successful decoding
*       NU_USB_UNKNOWN_ERR on error.
*
*
*************************************************************************/
STATUS USBH_HID_Decode_Global_Item(
                              USBH_HID_ITEM_STATE_TABLE *item_state_table,
                              USBH_HID_ITEM_PREFIX      *item_prefix)
{
    STATUS status = NU_SUCCESS;
    UINT8 which_data, index;

    NU_USBH_HID_ASSERT(item_state_table);
    NU_USBH_HID_ASSERT(item_prefix);

    if (item_prefix->bTag == USBH_HID_GLOBAL_ITEM_PUSH )
    {
        for(index = 0; index <10; index++)
        {
            item_state_table->global_storage_stack
            [item_state_table->global_storage_stack_top][index]
            = item_state_table->global_storage[index];
        }
        item_state_table->global_storage_stack_top++;
        status = NU_SUCCESS;
    }
    else
    {
        if (item_prefix->bTag == USBH_HID_GLOBAL_ITEM_POP )
        {
            for(index = 0; index <10; index++)
            {
                if((item_state_table->global_storage_stack_top <= USBH_HID_MAX_STACK_SIZE)&&
                    (item_state_table->global_storage_stack_top > 0x00))
                {
                   item_state_table->global_storage[index]
                   = item_state_table->global_storage_stack
                     [item_state_table->global_storage_stack_top-1][index];
                }
            }
            item_state_table->global_storage_stack_top--;
            status = NU_SUCCESS;
        }
        else
        {
            if(item_prefix->bTag <= USBH_HID_GLOBAL_ITEM_REP_COUNT)
            {
                which_data =
                item_state_table->global_storage_sign[item_prefix->bTag];
                if(which_data)
                {
                    item_state_table->global_storage[item_prefix->bTag] =
                        item_prefix->item_signed_data;
                }
                else
                {
                    item_state_table->global_storage[item_prefix->bTag] =
                        item_prefix->item_data;
                }
                status = NU_SUCCESS;
            }
            else
            {
                /* An unknown global tag is found. Set error condition. */
                status = NU_USB_INVLD_DESC;
            }
        }
    }
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       USBH_HID_Decode_Local_Item
*
* DESCRIPTION
*
*       This function decodes a local item and stores it in the state
*       table.
*
* INPUTS
*
*       item_state_table   State table.
*       item_prefix        Item prefix of the item.
*
* OUTPUTS
*
*       NU_SUCCESS         on successful decoding.
*       NU_USB_UNKNOWN_ERR on error.
*
*
*************************************************************************/
STATUS USBH_HID_Decode_Local_Item(
                             USBH_HID_ITEM_STATE_TABLE *item_state_table,
                             USBH_HID_ITEM_PREFIX      *item_prefix)
{
    UINT32 local_data;
    UINT32 data = 0;

    NU_USBH_HID_ASSERT(item_state_table);
    NU_USBH_HID_ASSERT(item_prefix);

    /* Retrieve data from the item. Local data is always unsigned UINT32
     *  according to HID specification.
     */
    switch (item_prefix->bSize)
    {
        case 1: local_data = (UINT8)(item_prefix->item_data);break;
        case 2: local_data =  (UINT16)( item_prefix->item_data);break;
        case 3: local_data = (UINT32)( item_prefix->item_data);break;
        default : local_data = 0;
    }

    data = (local_data);

    /*  When usage size is less than 4 bytes it means it has to get
     *  USAGE_PAGE from global state table and put it with the USAGE id.
     *  First 16 bit (MSB) is USAGE PAGE and next 16 bits is USAGE ID.
     */
    if (item_prefix->bTag == USBH_HID_LOCAL_ITEM_USAGE )
    {
        item_state_table->local_items.usage = data;

        if (item_state_table->local_items.usage_active == 0x00)
        {
            item_state_table->local_items.usage_active = 0x01;
            item_state_table->local_items.num_usage_ids = 0;
            item_state_table->local_items.usage_min = data;
        }
        else
        {
            item_state_table->local_items.usage_max = data;
        }

        item_state_table->local_items.usage_ids[
            item_state_table->local_items.num_usage_ids++] = data;
    }

    /* Usage min and usage max are used to define usage of controls in
     * bulk.
     */

    if(item_prefix->bTag == USBH_HID_LOCAL_ITEM_USAGE_MIN )
    {
        item_state_table->local_items.num_usage_ids = 0;
        item_state_table->local_items.usage_min = data;
    }

    if (item_prefix->bTag == USBH_HID_LOCAL_ITEM_USAGE_MAX )
    {
        item_state_table->local_items.num_usage_ids = 0;
        item_state_table->local_items.usage_max = data;
    }

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       USBH_HID_Decode_Main_Item
*
* DESCRIPTION
*
*       This function decodes a main item and stores it in the state
*       table.
*
* INPUTS
*
*       item_state_table   State table.
*       item_prefix        Item prefix of the item.
*
* OUTPUTS
*
*       NU_SUCCESS         On successful decoding.
*       NU_USB_UNKNOWN_ERR On error.
*
*************************************************************************/
STATUS USBH_HID_Decode_Main_Item(
                            USBH_HID_ITEM_STATE_TABLE   *item_state_table,
                            USBH_HID_ITEM_PREFIX        *item_prefix)
{
    UINT32 main_data;
    STATUS status =  NU_USB_INVLD_DESC;

    NU_USBH_HID_ASSERT(item_state_table);
    NU_USBH_HID_ASSERT(item_prefix);

    /* MAIN data is always +ve. Since it gives information about either
     * collection type in case of non data MAIN item or information like
     * relative | absolute, variable | array, in case of data MAIN item.
     */
    switch (item_prefix->bSize)
    {
        case 1: main_data = (UINT8)(item_prefix->item_data);break;
        case 2: main_data =  (UINT16)( item_prefix->item_data);break;
        case 3: main_data = (UINT32)( item_prefix->item_data);break;
        default : main_data = 0;
    }

    if (item_prefix->bTag == USBH_HID_MAIN_ITEM_INPUT )
    {
        status = USBH_HID_Handle_Main_Data_Item (item_state_table,
                                                main_data,
                                                USBH_HID_IN_REPORT);
    }

    if (item_prefix->bTag == USBH_HID_MAIN_ITEM_OUTPUT )
    {
        status = USBH_HID_Handle_Main_Data_Item (item_state_table,
                                                main_data,
                                                USBH_HID_OUT_REPORT);
    }

    if (item_prefix->bTag == USBH_HID_MAIN_ITEM_BEG_COLL )
    {
        status = USBH_HID_Hnd_Main_Non_Data_Item(item_state_table,
                                                main_data,
                                                USBH_HID_BEGIN_COLLECTION);
    }

    if (item_prefix->bTag == USBH_HID_MAIN_ITEM_FEATURE )
    {
        status = USBH_HID_Handle_Main_Data_Item (item_state_table,
                                                 main_data,
                                                 USBH_HID_FEATURE_REPORT);
    }
    if (item_prefix->bTag == USBH_HID_MAIN_ITEM_END_COLL )
    {
        status = USBH_HID_Hnd_Main_Non_Data_Item(item_state_table,
                                                 main_data,
                                                 USBH_HID_END_COLLECTION);
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       USBH_HID_Hnd_Main_Non_Data_Item
*
* DESCRIPTION
*
*       This function decodes a non data main item and stores it
*       in the state  table.
*
* INPUTS
*
*       item_state_table   State table.
*       main_data          Data associated with the item.
*       begin_end_col      Specifies if the tag is a begin/end of
*                          collection.
*
* OUTPUTS
*
*       NU_SUCCESS         On successful decoding
*       NU_USB_UNKNOWN_ERR On error.
*
*
*************************************************************************/
STATUS USBH_HID_Hnd_Main_Non_Data_Item(
                            USBH_HID_ITEM_STATE_TABLE   *item_state_table,
                            UINT32                       main_data,
                            UINT8                        begin_end_col)
{
    USBH_HID_DEVICE *hid_device = NU_NULL;
    USBH_HID_COLLECTION *collection = NU_NULL;
    USBH_HID_COLLECTION *parent     = NU_NULL;
    UINT16 i;
    STATUS status = NU_USB_UNKNOWN_ERR;

    NU_USBH_HID_ASSERT(item_state_table);

    /* Get HID device pointer.*/
    hid_device = item_state_table->hid_device;

    /* Is this item in beginning of the collection? */
    if (begin_end_col == USBH_HID_BEGIN_COLLECTION )
    {
        if (hid_device->num_of_coll == USBH_HID_MAX_COLLECTIONS)
        {
            /* No space is available  to store collection hence change
             * the config value and build the project and then run it.
             */
            return(NU_USB_MAX_EXCEEDED);
        }

        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                     sizeof(USBH_HID_COLLECTION),
                                     (VOID**)&collection);
        if(status != NU_SUCCESS)
        {
            return (status);
        }
        /* Initialization of COLLECTION structure. */
        collection->parent = NU_NULL;
        for( i=0; i < USBH_HID_MAX_COLL_CHILDS; i++)
        {
            collection->child[i] =NU_NULL;
        }
        /* Main_data contains information about Type of collection
         * such as APPS,Logical,or Physical etc...
         */
        collection->collection_type = main_data;
        collection->usage = item_state_table->local_items.usage;
        collection->number_of_child = 0;
        collection->max_number_child = USBH_HID_MAX_COLL_CHILDS;
        /*  Add collection to HID device. */
        hid_device->collection[hid_device->num_of_coll] = collection;
        hid_device->num_of_coll++;
        if( item_state_table->coll_ptr == 0x00 )
        {
            /* Update collection Stack. */
            item_state_table->collection_stack[item_state_table->coll_ptr]
                = collection;
            item_state_table->coll_ptr++;
            status = NU_SUCCESS;
        }
        else
        {
            /* Collection stack is not empty, hence new collection has
             * parent and last entry in collection stack got a child so
             * please take care while creating relationship between the
             * two collections.
             */
            if (item_state_table->coll_ptr < USBH_HID_MAX_COLLECTIONS)
            {
                /* Get pointer of parent and Child BEFORE the updating of
                 * the collection stack.
                 */
                parent =
                item_state_table->collection_stack
                [item_state_table->coll_ptr-1];
                /* Update collection Stack.    */
                item_state_table->collection_stack
                [item_state_table->coll_ptr++]
                = collection;
                parent->child[parent->number_of_child] = collection;
                parent->number_of_child++;
                collection->parent = parent;
                status = NU_SUCCESS;
            }
            /*  End of IF (item_state_table->coll_pt). */
            else if (hid_device->num_of_coll < USBH_HID_MAX_COLLECTIONS)
            {
                hid_device->collection[hid_device->num_of_coll] = NU_NULL;
                hid_device->num_of_coll--;
                USB_Deallocate_Memory(collection);
                return (NU_USB_MAX_EXCEEDED);
            }
            /* End of Else. */
        }
        /* End of ELSE  Not first Collection going to stack. */
    }
    /* End of if (begin_end_col == COLLECTION ).  */
    else
    {
        item_state_table->coll_ptr--;
        item_state_table->collection_stack[item_state_table->coll_ptr]
            = NU_NULL;
        status = NU_SUCCESS;
    }
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       USBH_HID_Handle_Main_Data_Item
*
* DESCRIPTION
*
*       This function decodes a data main item and stores it
*       in the state  table.
*
* INPUTS
*
*       item_state_table   State table.
*       main_data          Data associated with the item.
*       report_type        Specifies the report type (in/out/feature).
*
* OUTPUTS
*
*       NU_SUCCESS         On successful decoding.
*       NU_USB_UNKNOWN_ERR On error.
*
*************************************************************************/
STATUS USBH_HID_Handle_Main_Data_Item (
                        USBH_HID_ITEM_STATE_TABLE   *item_state_table,
                        UINT32                       main_data,
                        UINT8                        report_type)
{
    USBH_HID_DEVICE *hid_device;
    NU_USBH_HID_ITEM *main_item = NU_NULL;
    UINT8   i;
    INT32 *global_storage;
    USBH_HID_LOCAL_ITEMS *local_items;
    UINT8 coll_stack_position;
    STATUS status;
    UINT32 total_size = 0;
    UINT16 in_report_id = 0,out_report_id = 0,feature_report_id = 0;

    NU_USBH_HID_ASSERT(item_state_table);

    /* Initialize pointers.  */
    hid_device = item_state_table->hid_device;

    /* Get states of all the state table. */
    global_storage = item_state_table->global_storage;
    local_items = &(item_state_table->local_items);

    coll_stack_position = item_state_table->coll_ptr;

    /* Check for reserved MAIN item. If yes then only increment
     * total_report_size and come back.
     */

    if ( (local_items->usage == 0x00)
            && (local_items->usage_min == 0x00)
            && (local_items->usage_max == 0x00))
    {
        total_size =
        (global_storage[USBH_HID_GLOBAL_ITEM_REP_SIZE])
         * (global_storage[USBH_HID_GLOBAL_ITEM_REP_COUNT]);
        /* For next basic report this total size will be offset. */
        if(report_type == USBH_HID_IN_REPORT)
        {
            if (total_size > hid_device->in_report_size)
            hid_device->in_report_size += total_size;
        }
        if(report_type == USBH_HID_OUT_REPORT)
            hid_device->out_report_size += total_size;
        if(report_type == USBH_HID_FEATURE_REPORT)
            hid_device->feature_report_size += total_size;
        return (NU_SUCCESS);
    }

    status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                 sizeof(NU_USBH_HID_ITEM),
                                 (VOID**)&main_item);

    if (status != NU_SUCCESS)
    {
        return status;
    }

    memset(main_item, 0, sizeof(NU_USBH_HID_ITEM));

    hid_device->usages[hid_device->num_items].item = main_item;
    hid_device->num_items++;
    main_item->report_type = report_type;

    /* This "for" loop to find out index of report according to
     * report id.
     */
    main_item->report_id =
    global_storage[USBH_HID_GLOBAL_ITEM_REP_ID];

    for(i = 0; i< (hid_device->num_items-1); i++)
    {
        if(main_item->report_id == hid_device->usages[i].item->report_id)
            break;
    }
    /* If index is less than number of feature items, it means given id
     * has registry in the report database so don't increment
     * number_of_id.Allocate basic report structure and copy all the state
     *  table items to that.
     */

    if(i >= (hid_device->num_items-1))
    {
        hid_device->num_report_ids++;
        in_report_id = hid_device->num_report_ids - 1;
        out_report_id = hid_device->num_report_ids - 1;
        feature_report_id = hid_device->num_report_ids- 1;
    }
    else
    {
        if(report_type == USBH_HID_IN_REPORT)
        {
            in_report_id = hid_device->num_report_ids - 1;
        }
        if(report_type == USBH_HID_OUT_REPORT)
        {
            out_report_id = hid_device->num_report_ids - 1;
        }
        if(report_type == USBH_HID_FEATURE_REPORT)
        {
            feature_report_id = hid_device->num_report_ids - 1;
        }
    }

    if (hid_device->num_items == NU_USB_MAX_HID_USAGES)
    {
        USB_Deallocate_Memory(main_item);
        hid_device->usages[hid_device->num_items - 1].item = NU_NULL;
        hid_device->num_items--;
        return ( NU_USB_MAX_EXCEEDED);
    }

    if((coll_stack_position <= USBH_HID_MAX_COLLECTIONS )&&
        ( coll_stack_position > 0x00))
    {
        hid_device->usages[hid_device->num_items-1].collection =
        item_state_table->collection_stack[(coll_stack_position-1)];
    }
    /* UPDATE Global, local as well as main item in
     * basic report structure.
     */
    main_item->usage_page    = global_storage[0];
    main_item->logical_min   = global_storage[1];
    main_item->logical_max   = global_storage[2];
    main_item->physical_min  = global_storage[3];
    main_item->physical_max  = global_storage[4];
    main_item->unit_exponent = global_storage[5];
    main_item->unit          = global_storage[6];
    main_item->report_size   = global_storage[7];
    main_item->report_id     = global_storage[8];
    main_item->report_count  = global_storage[9];

    main_item->usage     = local_items->usage;
    main_item->usage_min = local_items->usage_min;
    main_item->usage_max = local_items->usage_max;

    for(i = 0; i < local_items->num_usage_ids; i++)
    {
        main_item->usage_ids[i] = local_items->usage_ids[i];
    }

    main_item->num_usage_ids = local_items->num_usage_ids;
    main_item->main_data = main_data;

    /* Update the total size of basic report for given ID. */
    total_size = (global_storage[USBH_HID_GLOBAL_ITEM_REP_SIZE]) *
        (global_storage[USBH_HID_GLOBAL_ITEM_REP_COUNT]);

    /* Depending on the type of the report, update the information
     * regarding each report, such as report size, offset for each
     * report id. */
    if(report_type == USBH_HID_IN_REPORT)
    {
        main_item->report_offset =
        hid_device->input_report_info[in_report_id].report_size;
        hid_device->input_report_info[in_report_id].report_size +=
        total_size;
        hid_device->input_report_info[in_report_id].type = report_type;
        hid_device->input_report_info[in_report_id].report_id =
        main_item->report_id;
    }
    if(report_type == USBH_HID_OUT_REPORT)
    {
        main_item->report_offset =
        hid_device->output_report_info[out_report_id].report_size;
        hid_device->output_report_info[out_report_id].report_size +=
        total_size;
        hid_device->out_report_size += total_size;
        hid_device->output_report_info[out_report_id].type = report_type;
        hid_device->output_report_info[out_report_id].report_id =
        main_item->report_id;
    }
    if(report_type == USBH_HID_FEATURE_REPORT)
    {
        main_item->report_offset =
        hid_device->feature_report_info[feature_report_id].report_size;
        hid_device->feature_report_info[feature_report_id].report_size +=
        total_size;
        hid_device->feature_report_size += total_size;
        hid_device->feature_report_info[feature_report_id].type =
        report_type;
        hid_device->feature_report_info[feature_report_id].report_id =
        main_item->report_id;
    }
    return (NU_SUCCESS);
}
/*************************************************************************
* FUNCTION
*
*       USBH_HID_Find_Device_For_IN_IRP
*
* DESCRIPTION
*
*       This function finds device pointer for which the interrupt IN
*       IRP is completed. Due to  mutual exclusion only a single device
*       interrupt IRP completion callback is completed at a time.
*
* INPUTS
*
*       cb              Pointer to the class driver's control block.
*
* OUTPUTS
*
*       non NU_NULL     Pointer to the corresponding USBH_HID_DEVICE.
*       NU_NULL         Indicates Device doesn't exist.
*
*************************************************************************/
USBH_HID_DEVICE* USBH_HID_Find_Device_For_IN_IRP(NU_USBH_HID    *cb)
{

    USBH_HID_DEVICE *next, *hid_dev;

    NU_USBH_HID_ASSERT(cb);

    hid_dev = cb->dev_list_head;

    /* Search for device for which interrupt In IRP completion callback
     * has occurred in the circular list of hid device instances.
     */
    while (hid_dev)
    {
        next = (USBH_HID_DEVICE *) hid_dev->node.cs_next;
        /* If Interrupt IRP completed flag is true, return device pointer.
         */
        if (hid_dev->int_irp_completed)
                return hid_dev;
            /* If we have reached end of the list, return NULl. */
            if ((next == cb->dev_list_head)
                || (cb->dev_list_head == NU_NULL))
                return (NU_NULL);
            else
                hid_dev = next;
    }

    return NU_NULL;
}

/* ======================  End Of File  =============================== */

