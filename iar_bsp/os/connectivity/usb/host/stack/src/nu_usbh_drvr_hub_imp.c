/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************/

/************************************************************************
 *
 * FILE NAME
 *     nu_usbh_drvr_hub_imp.c
 *
 * COMPONENT
 *     Nucleus USB Host Stack
 *
 * DESCRIPTION
 *     This file contains Nucleus USB Host Stack: Hub class driver
 *     implementation.
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     NU_USBH_HUB_Create                    Create a new hub driver.
 *     NU_USBH_HUB_Allocate_Power            Allocate power to device
 *     NU_USBH_HW_Release_Power              Release power from device.
 *     NU_USBH_HUB_Disconnect                Disconnect device from hub.
 *     usb_find_hub                          Finds hub from the list
 *     usb_hub_feature_req                   Issues a control request to
 *                                           the given hub
 *     usb_hub_desc_req                      Issues a control request to
 *                                           the given hub
 *     usb_hub_status_req                    Issues a control request to
 *                                           the given hub
 *     USBH_HUB_Allocate_Power               Power budgeting function.
 *     USBH_HUB_Release_Power                Power budgeting function.
 *     _NU_USBH_HUB_Disconnect               Cleans up allocated memory for
 *                                           a DIRTY hub.
 *     USBH_HUB_Initialize_Intf              Processes and adds a newly
 *                                           detected hub to the Hub List
 *     usb_hub_shutdown                      Removes hub and all connected
 *                                           devices from subsystem
 *     usb_hub_port_status_change            Processes the port change
 *                                           events reported by a HUB
 *     usb_hub_device_status_change          Processes the status change
 *                                           events reported by a HUB
 *     usb_hub_port_debounce                 This function implements
 *                                           the debounce wait for the
 *                                           given port
 *     usb_hub_port_reset                    This function resets the given
 *                                           port of a given hub
 *     usb_hub_port_connect                  This function is called when a
 *                                           new device is reported on a
 *                                           hub
 *     usb_hub_ctrl_tx_complete              Ctrl endpoint transfer
 *                                           completion handler. Releases
 *                                           the irpCompleteLock on
 *                                           successful transfers
 *     usb_hub_intr_tx_complete              Intr endpoint transfer
 *                                           completion handler. Submits a
 *                                           new IRP and processes the
 *                                           events in the first one
 *     USBH_Hub_Task                         USB-HUB task's entry function
 *     USBH_HUB_Disconnect_Device            Disconnect a device from
 *                                           the topology
 *     _NU_USBH_HUB_Delete                   Deinitializes the hub
 *                                           class driver.
 *     usbh_hub_get_port_capabilities        This function retrieves OTG
 *                                           capabilities of the port.
 *     usbh_hub_suspend_port                 This function suspends a given
 *                                           port of the hub.
 *     usbh_hub_resume_port                  This function resumes a given
 *                                           port of the hub.
 *     usbh_hub_find_companion               This function will search the
 *                                           companion hub on the basis of
 *                                           ContainerID present in BOS
 *                                           descriptor.
 *     usbh_hub_set_depth_req                This request sets the value
 *                                           that the hub uses to determine
 *                                           the index into the route string
 *                                           index of the hub.
 *     usbh_hub_get_port_err_cnt_req         This hub class request returns
 *                                           the number of link errors
 *                                           detected by the hub on port
 *                                           indicated by wIndex.
 *     usbh_hub_port_set_timeout             This function is used to set
 *                                           U1 and U2 inactivity timeout
 *                                           of for the specified port.
 *     usbh_hub_port_rw_mask                 This function is used to set
 *                                           remote wakeup mask for the
 *                                           specified port.
 *    usbh_hub_port_set_link_state           This function sets link state.
 *    usbh_hub_port_suspend_link             This function suspends the
 *                                           link.
 *    usbh_hub_port_resume_link              This function resumes the
 *                                           link.
 *    usbh_hub_port_bh_reset                 This function starts warm
 *                                           reset on the port.
 *
 * DEPENDENCIES
 *     nu_usb.h                              All USB Definitions
 *
 ************************************************************************/
#ifndef USBH_DRVR_HUB_IMP_C
#define USBH_DRVR_HUB_IMP_C

/* ==============  Standard Include Files ============================  */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"

/* =====================  #defines ===================================  */

/* =====================  Global data ================================  */

/* =====================  Functions ==================================  */

static STATUS usb_hub_feature_req (NU_USBH_STACK * cb,
                                   NU_USBH_DEVICE_HUB * hub,
                                   UINT8 feature,
                                   UINT8 portNum,
                                   UINT8 selector,
                                   UINT8 featureType,
                                   UINT8 cmd);

static STATUS usb_hub_desc_req (NU_USBH_STACK * cb,
                                NU_USBH_DEVICE_HUB * hub);

static STATUS usb_hub_status_req (NU_USBH_STACK * cb,
                                  NU_USBH_DEVICE_HUB * hub,
                                  USBH_HUB_STATUS * hubStatus,
                                  UINT8 portNum,
                                  UINT8 reqType);

static VOID usb_hub_shutdown (NU_USBH_DRVR_HUB * hub_driver,
                              NU_USBH_STACK * cb,
                              NU_USBH_DEVICE_HUB * hub);

static VOID usb_hub_port_status_change (NU_USBH_STACK * cb,
                                        NU_USBH_DEVICE_HUB * hub,
                                        UINT32 status,
                                        UINT8 speed);

static VOID usb_hub_device_status_change (NU_USBH_STACK * cb,
                                          NU_USBH_DEVICE_HUB * hub);

static STATUS usb_hub_port_debounce (NU_USBH_STACK * cb,
                                     NU_USBH_DEVICE_HUB * hub,
                                     UINT8 portNum,
                                     USBH_HUB_STATUS * portStatus);

static STATUS usb_hub_port_reset (NU_USBH_STACK * cb,
                                  NU_USBH_DEVICE_HUB * hub,
                                  UINT8 portNum,
                                  UINT8 *speed);

VOID usb_hub_port_connect (NU_USBH_STACK * cb,
                           NU_USB_DEVICE * device,
                           UINT8 portNum,
                           UINT8 speed);

static VOID usb_hub_ctrl_tx_complete (NU_USB_PIPE * pipe,
                                      NU_USB_IRP * irp);

static VOID usb_hub_intr_tx_complete (NU_USB_PIPE * pipe,
                                      NU_USB_IRP * irp);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#if 0
static STATUS usbh_hub_port_bh_reset (NU_USBH_STACK * cb,
                                      NU_USBH_DEVICE_HUB * hub,
                                      UINT8 portNum,
                                      UINT8 *speed);
#endif
#endif
/*************************************************************************
* FUNCTION
*        NU_USBH_HUB_Create
*
* DESCRIPTION
*       This function creates a hub class driver. Host stack creates it
*       internally and provides its own pool as the pool required by hub
*       class driver. This creates a task, which will be woken up whenever
*       some device is connected/disconnected/suspended/resumed in the bus.
*
* INPUTS
*       cb                      pointer to HUB class driver control block.
*       stack                   pointer to host stack control block.
*       pool                    pointer to memory pool required.
*       hub_tak_stack_address   pointer to location for hub task's stack.
*       hub_task_stack_size     size of hub task stack.
*       hub_task_priority       priority of hub task.
*
* OUTPUTS
*       NU_USB_SUCCESS          Indicates hub Class driver Initialized
*                               properly.
*       NU_USB_INVLD_ARG        Indicates some argument is invalid.
*       NU_INVALID_MEMORY       Stack pointer is NU_NULL.
*       NU_INVALID_SIZE         Stack size is too small.
*       NU_INVALID_PRIORITY     Invalid task priority.
*
*************************************************************************/
STATUS NU_USBH_HUB_Create (NU_USBH_DRVR_HUB * hcb,
                           NU_USBH_STACK * stack,
                           NU_MEMORY_POOL * pool,
                           VOID *hub_task_stack_address,
                           UNSIGNED hub_task_stack_size,
                           OPTION hub_task_priority)
{
    STATUS         status = NU_SUCCESS;
    STATUS         internal_sts = NU_SUCCESS;
    UINT8          rollback = 0;
    NU_USBH_SUBSYS *subsys;

    /* Validation of parameters. */
    NU_USB_MEMPOOLCHK(pool);
    if (hcb == NU_NULL || stack == NU_NULL
        || hub_task_stack_address == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }

    /* This function is the entry point into the Hub Class driver.
     * This function creates the tUSB-HUB task and registers itself with the
     * USBD. Before creating the task, it allocates memory for the task
     * control block and the task stack. It is allocated from a memory pool
     * that is given as an input parameter.
     *
     * A small error handling function is implemented, that does the
     * clean up for most of the errors occurred in this function.
     * the switch case takes a number and the cleanup falls through.
     * Since there were many system calls made in the init call, very
     * thorough cleanup is necessary.
     */
    subsys = &nu_usbh->class_driver_subsys;

    /* sanity check for root hub address (1 by default)
       which is user configurable  */
    if ((USB_ROOT_HUB == 0) || (USB_ROOT_HUB >= USB_MAX_DEVID))
    {
        return NU_USB_INVLD_DEV_ID;
    }

    hcb->hubMemPool = pool;

    memset (&(hcb->hubTask), 0, sizeof (NU_TASK));

    /* create a queue to store upto 2 IRP pointers */
    status = NU_Create_Queue(&(hcb->queue), "HubQueue", &(hcb->irplist),
                             USBH_HUB_QUEUE_MSGS, NU_FIXED_SIZE, 1, NU_FIFO);

    if(status != NU_SUCCESS)
    {
        rollback = 1;
    }

    if (!rollback)
    {
        status = NU_Create_Semaphore (&(hcb->irpCompleteLock), "USBHHUB3",
                                      0, NU_FIFO);

        if (status != NU_SUCCESS)
        {
            rollback = 2;
        }
    }

    if (!rollback)
    {
        /* Create the task tUSBD    */
        status = NU_Create_Task (&hcb->hubTask, "USBH-HUB", USBH_Hub_Task,
                                 0, hcb, hub_task_stack_address,
                                 hub_task_stack_size, hub_task_priority, 0,
                                 NU_PREEMPT, NU_NO_START);

        if (status != NU_SUCCESS)
        {
            rollback = 3;
        }
    }

    if (!rollback)
    {
        status = NU_Resume_Task((NU_TASK *)&hcb->hubTask);
        if (status != NU_SUCCESS)
        {
            rollback = 4;
        }
    }

    /* Base class behavior */
    if (!rollback)
    {
        status = _NU_USB_DRVR_Create ((NU_USB_DRVR *) hcb, (NU_USB_SUBSYS *) subsys,
                                  "USBH-HUB", USB_MATCH_CLASS, 0, 0, 0, 0,
                                  USB_HUB_CLASS_CODE, 0, 0, &usbh_hub_dispatch);

        if (status != NU_SUCCESS)
        {
            rollback = 4;
        }
    }

    if (!rollback)
    {
        hcb->stack = stack;
    }

    switch (rollback)
    {
        case 4:
            internal_sts = NU_Terminate_Task (&hcb->hubTask);
            internal_sts |= NU_Delete_Task (&hcb->hubTask);
        case 3:
            internal_sts |= NU_Delete_Semaphore (&(hcb->irpCompleteLock));
        case 2:
            internal_sts |= NU_Delete_Queue(&(hcb->queue));
        default:
            break;
    }

    NU_UNUSED_PARAM(internal_sts);
    return (status);
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      NU_USBH_HUB_Allocate_Power
 *
 * DESCRIPTION
 *      Power budgeting function.
 *
 * INPUTS
 *      cb      pointer to hub class driver control block
 *      device  pointer to device control block of device.
 *      power   Power Required.
 *
 * OUTPUTS
 *
 *      NU_SUCCESS          if required power is available
 *      NU_USB_NO_POWER     if not
 *
 ************************************************************************/
STATUS NU_USBH_HUB_Allocate_Power (NU_USBH_DRVR_HUB * cb,
                                   NU_USB_DEVICE * device,
                                   UINT16 power)
{
    NU_UNUSED_PARAM(cb);
    NU_UNUSED_PARAM(device);
    NU_UNUSED_PARAM(power);
    return (NU_SUCCESS);
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      NU_USBH_HW_Release_Power
 *
 * DESCRIPTION
 *      Power budgeting function
 *
 * INPUTS
 *      cb      pointer to hub class driver control block
 *      device  pointer to device control block of device.
 *      power   Power Required.
 *
 * OUTPUTS
 *      NU_SUCCESS      always
 *
 ************************************************************************/
STATUS NU_USBH_HW_Release_Power (NU_USBH_DRVR_HUB * cb,
                                 NU_USB_DEVICE * device,
                                 UINT16 power)
{
    NU_UNUSED_PARAM(cb);
    NU_UNUSED_PARAM(device);
    NU_UNUSED_PARAM(power);
    return (NU_SUCCESS);
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      NU_USBH_HUB_Disconnect
 *
 * DESCRIPTION
 *      Disconnect a device from the topology
 *
 * INPUTS
 *      cb      pointer to hub class driver control block
 *      device  pointer to device control block of device.
 *
 * OUTPUTS
 *      NU_SUCCESS  on success
 *      NU_USB_INVLD_ARG        Indicates some argument is invalid.
 *
 ************************************************************************/
STATUS NU_USBH_HUB_Disconnect (NU_USBH_DRVR_HUB * cb,
                               NU_USB_DEVICE * device)
{
    if ( cb == NU_NULL || device == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }
    return USBH_HUB_Disconnect_Device (cb, device);
}

/************************************************************************
 *
 * FUNCTION
 *
 *      usb_find_hub
 *
 * DESCRIPTION
 *      This function finds hub in the list of hubs
 *
 * INPUTS
 *      hcb         pointer to hub driver control block.
 *      device      pointer to device control block for this hub.
 *
 * OUTPUTS
 *      NU_USBH_DEVICE_HUB*     Pointer to the hub
 *      NU_NULL                 if hub can't be found.
 *      NU_USB_INVLD_ARG        Indicates some argument is invalid.
 ************************************************************************/
NU_USBH_DEVICE_HUB *usb_find_hub (NU_USBH_DRVR_HUB * hcb,
                                  NU_USB_DEVICE * device)
{
    NU_USBH_DEVICE_HUB *hub;

    if ( hcb == NU_NULL || device == NU_NULL)
    {
        return (NU_NULL);
    }

    hub = hcb->session_list_head;

    while (hub)
    {
        if (hub->device == device)
        {
            return (hub);
        }

        hub = (NU_USBH_DEVICE_HUB *) hub->node.cs_next;

        if (hub == hcb->session_list_head)
        {
            hub = NU_NULL;
        }
    }
    return (NU_NULL);
}

/************************************************************************
 *
 * FUNCTION
 *
 *      usb_hub_feature_req
 *
 * DESCRIPTION
 *      This function issues a control request to the given hub
 *
 * INPUTS
 *      cb              pointer to stack control block
 *      hub             pointer to the hub driver control block
 *      feature         Feature to be set or cleared on the 'hub'
 *      portNum         Port number for the port on the hub.
 *      featureType     Port Request or a Hub request ?
 *      cmd             bRequest to be sent
 *
 * OUTPUTS
 *      NU_SUCCESS      Success
 *      NU_USB_INVLD_ARG        Indicates some argument is invalid.
 ************************************************************************/
static STATUS usb_hub_feature_req (NU_USBH_STACK * cb,
                                 NU_USBH_DEVICE_HUB * hub,
                                 UINT8 feature,
                                 UINT8 portNum,
                                 UINT8 selector,
                                 UINT8 featureType,
                                 UINT8 cmd)
{
    NU_USBH_CTRL_IRP *irp;
    STATUS  status;
    STATUS  internal_sts = NU_SUCCESS;
    UINT16 w_index;

    if ( cb == NU_NULL || hub == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }

    irp = hub->ctrlIrp;
    w_index = ( selector << 8 ) | portNum ;

    status = NU_Obtain_Semaphore (&(hub->lock), NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        return (status);
    }

    /* The Most part of this IRP would have been filled by the connect
     * code.
     */
    status = NU_USBH_CTRL_IRP_Set_bRequest (irp, cmd);

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_wLength (irp, 0);

        if (status == NU_SUCCESS)
        {
            if (featureType == USBH_HUB_PORT_REQ)
            {
                status = NU_USBH_CTRL_IRP_Set_bmRequestType (irp, 0x23);
            }
            else
            {
                status = NU_USBH_CTRL_IRP_Set_bmRequestType (irp, 0x20);
                portNum = 0;
            }
        }
        if (status == NU_SUCCESS)
        {
            status = NU_USBH_CTRL_IRP_Set_wValue (irp,
                                                  HOST_2_LE16 (feature));
        }

        if (status == NU_SUCCESS)
        {
            status = NU_USBH_CTRL_IRP_Set_wIndex (irp,
                                                  HOST_2_LE16 (w_index));
        }

        if (status == NU_SUCCESS)
        {
            status = NU_USB_IRP_Set_Data ((NU_USB_IRP *) irp, NU_NULL);
        }

        if (status == NU_SUCCESS)
        {
            status = NU_USB_IRP_Set_Length ((NU_USB_IRP *) irp, 0);
        }

        if (status == NU_SUCCESS)
        {
            /* Submit the IRP   */
            status = NU_USB_PIPE_Submit_IRP (hub->default_pipe,
                                            (NU_USB_IRP *) irp);
        }

        /* Wait for the semaphore   */
        if (status == NU_SUCCESS)
        {
            status = NU_Obtain_Semaphore (&(cb->hub_driver.irpCompleteLock),
                                      (NU_PLUS_Ticks_Per_Second * 5));
        }
    }

    internal_sts = NU_Release_Semaphore (&(hub->lock));

    if (status == NU_SUCCESS)
    {
        return (internal_sts);
    }
    else
    {
        return (status);
    }
}

/************************************************************************
 * FUNCTION
 *      usb_hub_desc_req
 *
 * DESCRIPTION
 *      This function issues a control request to the given hub
 *
 * INPUTS
 *      cb      pointer to stack control block
 *      hub     pointer to hub driver control block.
 *
 * OUTPUTS
 *  NU_SUCCESS              Indicates successful completion
 *  NU_USB_BITSTUFF_ERR     Indicates bitstuff error
 *  NU_USB_TOGGLE_ERR       Indicates data toggle error
 *  NU_USB_STALL_ERR        Indicates stall handshake
 *  NU_USB_NO_RESPONSE      Indicates nak
 *  NU_USB_INVLD_PID        Indicates bad PID
 *  NU_USB_UNEXPECTED_PID   Indicates out of order PID
 *  NU_USB_DATA_OVERRUN     Indicates data overflow
 *  NU_USB_DATA_UNDERRUN    Indicates data underflow
 *  NU_USB_BFR_OVERRUN      Indicated buffer overflow
 *  NU_USB_BFR_UNDERRUN     Indicates buffer underflow
 *  NU_USB_EP_HALTED        Indicates pipe stalled
 *  NU_USB_INVLD_ARG        Indicates some argument is invalid.
 ************************************************************************/
static STATUS usb_hub_desc_req (NU_USBH_STACK * cb,
                                NU_USBH_DEVICE_HUB * hub)
{

    NU_USBH_CTRL_IRP *irp;
    STATUS irp_status, status, internal_sts = NU_SUCCESS;

    if ( cb == NU_NULL || hub == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }

    irp = hub->ctrlIrp;

    status = NU_Obtain_Semaphore (&(hub->lock), NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        return (status);
    }

    /* The Most part of this IRP would have been filled by the connect
     * code.
     */
    status = NU_USBH_CTRL_IRP_Set_bRequest (irp, 0x06);

    if ( status == NU_SUCCESS )
    {

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

        if (hub->device->speed == USB_SPEED_SUPER )
        {
            status = NU_USBH_CTRL_IRP_Set_wLength (irp, HOST_2_LE16 (USBH_SS_HUB_DESC_LENGTH));

            if ( status == NU_SUCCESS )
            {
                status = NU_USBH_CTRL_IRP_Set_wValue (irp,
                                 HOST_2_LE16 ((USBH_SS_HUB_DESC_ID) << 8));
            }
        }

        else
#endif
        {
            status = NU_USBH_CTRL_IRP_Set_wLength (irp, HOST_2_LE16 (USBH_HUB_MAX_DESC_LEN));
            if (status == NU_SUCCESS)
            {
                status = NU_USBH_CTRL_IRP_Set_wValue (irp,
                                 HOST_2_LE16 ((USBH_HUB_DESC_ID) << 8));
            }
        }
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_bmRequestType (irp, 0xa0);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_wIndex (irp, 0);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Set_Data ((NU_USB_IRP *) irp, (UINT8 *) hub->raw_desc);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Set_Length ((NU_USB_IRP *) irp, USBH_HUB_MAX_DESC_LEN);
    }

    if (status == NU_SUCCESS)
    {
        /* Submit the IRP   */
        status = NU_USB_PIPE_Submit_IRP (hub->default_pipe, (NU_USB_IRP *) irp);
    }

    /* Wait for the semaphore   */
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore (&(cb->hub_driver.irpCompleteLock),
                                      (NU_PLUS_Ticks_Per_Second * 5));
    }

    if (status == NU_SUCCESS)
    {
        /* Now the transfer is complete.    */
        internal_sts = NU_USB_IRP_Get_Status ((NU_USB_IRP *) irp, &irp_status);
        internal_sts |= NU_Release_Semaphore (&(hub->lock));
        NU_UNUSED_PARAM(internal_sts);
        return (irp_status);
    }
    else
    {
        internal_sts = NU_Release_Semaphore (&(hub->lock));
        NU_UNUSED_PARAM(internal_sts);
        return (status);
    }
}

/************************************************************************
 *
 * FUNCTION
 *
 *      usb_hub_status_req
 *
 * DESCRIPTION
 *      This function issues a control request to the given hub
 *
 * INPUTS
 *      cb          pointer to stack control block.
 *      hub         pointer to the hub driver control block
 *      hubStatus   Pointer where the retrieved status is to be stored
 *      portNum     Port number for the port on the hub.
 *      reqType     Port Request or a Hub request ?
 *
 * OUTPUTS
 *      NU_USB_INVLD_ARG        Indicates some argument is invalid.
 *
 ************************************************************************/
static STATUS usb_hub_status_req (NU_USBH_STACK * cb,
                                NU_USBH_DEVICE_HUB * hub,
                                USBH_HUB_STATUS * hubStatus,
                                UINT8 portNum,
                                UINT8 reqType)
{

    UINT32 *temp_buffer = NU_NULL;
    UINT32 temp;
    STATUS irp_status, status, internal_sts = NU_SUCCESS;
    NU_USBH_CTRL_IRP *irp;

    if ( cb == NU_NULL || hub == NU_NULL || hubStatus == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }

    irp = hub->ctrlIrp;

    status = NU_Obtain_Semaphore (&(hub->lock), NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        return (status);
    }

    /* The Most part of this IRP would have been filled by the connect
     * cbAlternateSettingode.
     */
    if (reqType == USBH_HUB_PORT_REQ)
    {
        status = NU_USBH_CTRL_IRP_Set_bmRequestType (irp, 0xa3);
    }
    else
    {
        status = NU_USBH_CTRL_IRP_Set_bmRequestType (irp, 0xa0);
        portNum = 0;
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_bRequest (irp, USBH_HUB_CMD_GET_STATUS);
    }
    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_wLength (irp, HOST_2_LE16 (4));
    }
    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_wValue (irp, 0);
    }
    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_wIndex (irp, HOST_2_LE16 (portNum));
    }
    if(status == NU_SUCCESS)
    {
        /* Allocate uncached memory buffer for data IO. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(UINT32),
                                     (VOID **) &temp_buffer);
    }
    if (status == NU_SUCCESS)
    {
        memset(temp_buffer, 0, sizeof(UINT32));
        status = NU_USB_IRP_Set_Data ((NU_USB_IRP *) irp, (UINT8 *) temp_buffer);
    }
    if (status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Set_Length ((NU_USB_IRP *) irp, sizeof(UINT32));
    }
    if (status == NU_SUCCESS)
    {
        /* Submit the IRP   */
        status = NU_USB_PIPE_Submit_IRP (hub->default_pipe, (NU_USB_IRP *) irp);
    }
    /* Wait for the semaphore   */
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore (&(cb->hub_driver.irpCompleteLock),
                                       (NU_PLUS_Ticks_Per_Second * 5));
    }

    /* Now the transfer is complete. */
    internal_sts = NU_USB_IRP_Get_Status ((NU_USB_IRP *) irp, &irp_status);

    if ((status == NU_SUCCESS) && (irp_status == NU_SUCCESS))
    {
        temp = LE32_2_HOST (*temp_buffer);

        hubStatus->status = (UINT16) (temp & 0x00FFFF);
        hubStatus->change = (UINT16) ((temp >> 16) & 0x00FFFF);
    }
    else
    {
        hubStatus->status = 0;
        hubStatus->change = 0;
    }

    if(temp_buffer)
    {
        internal_sts |= USB_Deallocate_Memory(temp_buffer);
    }
    internal_sts |= NU_Release_Semaphore (&(hub->lock));
    NU_UNUSED_PARAM(internal_sts);

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usb_hub_allocate_power
 *
 * DESCRIPTION
 *      Power budgeting function
 *
 * INPUTS
 *      cb      pointer to hub driver control block
 *      device  pointer to device control block
 *      power   Power required
 *
 * OUTPUTS
 *
 *      NU_SUCCESS          if required power is available
 *      NU_USB_NO_POWER     if not
 *
 ************************************************************************/
STATUS USBH_HUB_Allocate_Power (NU_USBH_DRVR_HUB * cb,
                                NU_USB_DEVICE * device,
                                UINT16 power)
{
    NU_UNUSED_PARAM(cb);
    NU_UNUSED_PARAM(device);
    NU_UNUSED_PARAM(power);
    return (NU_SUCCESS);
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usb_hub_release_power
 *
 * DESCRIPTION
 *      Power budgeting function
 *
 * INPUTS
 *      cb      pointer to hub driver control block
 *      device  pointer to device control block
 *      power   Power released
 *
 * OUTPUTS
 *
 *  NU_SUCCESS      always
 *
 ************************************************************************/
STATUS USBH_HUB_Release_Power (NU_USBH_DRVR_HUB * cb,
                               NU_USB_DEVICE * device,
                               UINT16 power)
{
    return (NU_SUCCESS);
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      _NU_USBH_HUB_Disconnect
 *
 * DESCRIPTION
 *      Cleans up allocated memory for a DIRTY hub
 *
 * INPUTS
 *      driver      pointer to class driver control block.
 *      stk         pointer to stack control block.
 *      dev         pointer to device control block.
 *
 * OUTPUTS
 *      NU_SUCCESS  Indicates successful completion
 *      NU_USB_INVLD_ARG        Indicates some argument is invalid.
 ************************************************************************/
STATUS _NU_USBH_HUB_Disconnect (NU_USB_DRVR * driver,
                                NU_USB_STACK * stk,
                                NU_USB_DEVICE * dev)
{
    NU_USBH_DRVR_HUB *hub_driver;
    NU_USBH_DEVICE_HUB *hub;
    STATUS internal_sts = NU_SUCCESS;

    if ( driver == NU_NULL || dev == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }

    NU_UNUSED_PARAM(stk);

    hub_driver = (NU_USBH_DRVR_HUB *) driver;
    hub = usb_find_hub (hub_driver, dev);

    if (hub == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    if ((hub->refCount != 0) && (!(USBH_IS_ROOT_HUB(dev))))
    {
        hub->state = USBH_HUB_DIRTY;
    }
    else
    {
        /* remove from the session list */
        NU_Remove_From_List ((CS_NODE **) & (hub_driver->session_list_head),
                             (CS_NODE *) hub);

        internal_sts = NU_Delete_Semaphore (&(hub->lock));
        internal_sts |= USB_Deallocate_Memory ((VOID *)hub->status_irp[0].port_status);
        internal_sts |= USB_Deallocate_Memory ((VOID *)hub->status_irp[1].port_status);
        internal_sts |= USB_Deallocate_Memory ((VOID *)hub->raw_desc);
        internal_sts |= USB_Deallocate_Memory (hub->ctrlIrp);
        internal_sts |= USB_Deallocate_Memory (hub);
        NU_UNUSED_PARAM(internal_sts);
    }

    return (NU_SUCCESS);
}

/**********************************************************************
 * FUNCTION
 *      USBH_HUB_Initialize_Intf
 *
 * DESCRIPTION
 *      Processes and adds a newly detected hub to the Hub List
 *
 * INPUTS
 *      cb                      pointer to class driver control block.
 *      stk                     pointer to stack control block.
 *      dev                     pointer to device control block.
 *      intf                    pointer to interface control block.
 *
 * OUTPUTS
 *
 *      NU_SUCCESS              Indicates successful completion of the
 *                              service.
 *      NU_NOT_PRESENT          Indicates No suitable Alternate Setting
 *                              is found.
 *                              Indicates, Endpoints required are not found.
 *      NU_USB_INVLD_ARG        Indicates some control block(s) is(are)
 *                              deleted before completion.
 *      NU_NO_MEMORY            Indicates Memory Pool to allocate HUB
 *                              structure is full.
 *      NU_USB_INVLD_DESC       Indicates some descriptors of Device are
 *                              incorrect.
 *      NU_USB_MAX_EXCEEDED     Indicates Class Driver is already serving
 *                              as many devices as it can support.
 *      NU_INVALID_SUSPEND      Indicates call is made from a non thread
 *                              context.
 *
 ************************************************************************/
STATUS USBH_HUB_Initialize_Intf (NU_USB_DRVR * cb,
                                NU_USB_STACK * stk,
                                NU_USB_DEVICE * dev,
                                NU_USB_INTF * intf)
{
    /* New hub detected. */
    NU_USBH_DEVICE_HUB *hub = NU_NULL;
    NU_USB_CFG         *cfg;
    NU_USB_CFG_DESC    *cfgDesc;
    NU_USB_ENDP        *endp;
    NU_USBH_DRVR_HUB   *hub_driver;
    USBH_HUB_STATUS     portStatus;
    STATUS              status = NU_SUCCESS;
    STATUS              int_status = NU_SUCCESS;
    INT                 index = 0;
    UINT16              mps;
    UINT8               roll_back = 0;
    USBH_HUB_DESC_HEADER *hub_hdr       = NU_NULL;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

    NU_USBH_DEVICE_HUB   *parent_hub    = NU_NULL;
    UINT8                hub_depth      = 0;
    UINT8                tt_time        = 0;
    UINT8                num_ports      = 0;
    NU_USBH_DEVICE_HUB   *companion_hub = NU_NULL;
    NU_USB_DEVICE        *temp_device   = NU_NULL;
#endif      /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

    if ( cb == NU_NULL || stk == NU_NULL || dev == NU_NULL
        || intf == NU_NULL )
    {
        return (NU_USB_INVLD_ARG);
    }

    hub_driver = (NU_USBH_DRVR_HUB *) cb;

    do
    {
        status = USB_Allocate_Object(sizeof (NU_USBH_DEVICE_HUB),
                                     (VOID **) &hub);
        if (status != NU_SUCCESS || hub == NU_NULL)
        {
            roll_back = 1;
            break;
        }

        memset (hub, 0, sizeof (NU_USBH_DEVICE_HUB));

        /* Fill session identifiers. */
        hub->stack = stk;
        hub->drvr = cb;
        hub->intf = intf;
        hub->device = dev;

        /* Find matching alternate setting. */
        status = NU_USB_INTF_Find_Alt_Setting (intf,
                         USB_MATCH_CLASS | USB_MATCH_ONLY_ACTIVE_ALT_STTG
                        /* match flag */ ,
                         0,
                         USB_HUB_CLASS_CODE,
                         0,
                         0,
                         &hub->alt_settg);
        if(status != NU_SUCCESS)
        {
            roll_back = 2;
            break;
        }

        /* Find the default pipe */
        status = NU_USB_ALT_SETTG_Find_Pipe (hub->alt_settg,
                                             USB_MATCH_EP_ADDRESS,
                                             0,
                                             0,
                                             0,
                                             &hub->default_pipe);
        if (status != NU_SUCCESS)
        {
            roll_back = 2;
            break;
        }

        /* Find the interrupt pipe  */
        status =  NU_USB_ALT_SETTG_Find_Pipe (hub->alt_settg,
                                              USB_MATCH_EP_TYPE,
                                              0,
                                              0,
                                              3,
                                              &hub->interrupt_pipe);
        if (status != NU_SUCCESS)
        {
            roll_back = 2;
            break;
        }

        (VOID) NU_USB_PIPE_Get_Endp (hub->interrupt_pipe, &endp);
        (VOID) NU_USB_ENDP_Get_Max_Packet_Size (endp, &mps);

        status = NU_USB_DEVICE_Get_Active_Cfg (dev, &cfg);
        if (status != NU_SUCCESS)
        {
            roll_back = 2;
            break;
        }

        status = NU_USB_CFG_Get_Desc (cfg, &cfgDesc);
        if(status != NU_SUCCESS)
        {
            roll_back = 2;
            break;
        }

        /* Initialize the hub structure */
        for (index = 0; index <= USBH_HUB_MAX_CHILDREN; index++)
        {
            hub->child[index] = NU_NULL;
        }

        hub->state = USBH_HUB_NORMAL;
        hub->device = dev;
        hub->refCount = 0;
        hub->error_count = 0;

        /* Available Power with this HUB. */
        /* Check if the hub is a self powered or bus powered. */
        if ((cfgDesc->bmAttributes) & 0x40)
        {
            /* No power budgeting for self powered hubs. */
            hub->availablePower = 0;
        }
        else
        {
            /* Section 7.2 of USB spec. */
            hub->availablePower = 200;
        }

        /* Allocate memory for Endpoint 0 IRP. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(NU_USBH_CTRL_IRP),
                                     (VOID **)&(hub->ctrlIrp));
        if (status != NU_SUCCESS)
        {
            roll_back = 2;
            break;
        }

        memset (hub->ctrlIrp, 0, sizeof(NU_USBH_CTRL_IRP));
        /* Now fill the IRPs. */
        /* The control IRP. */
        status = NU_USBH_CTRL_IRP_Create (hub->ctrlIrp,
                                     0,
                                     /* Callback function */
                                     usb_hub_ctrl_tx_complete,

                                     /* context */
                                     NU_NULL,

                                     /* bmRequestType */
                                     0,

                                     /* bRequest */
                                     0,

                                     /* wValue   */
                                     0,

                                     /* wIndex   */
                                     0,

                                     /* wLength  */
                                     0);
        if (status != NU_SUCCESS)
        {
            roll_back = 3;
            break;
        }

        status = NU_Create_Semaphore (&(hub->lock), "HubLock", 1, NU_FIFO);
        if (status != NU_SUCCESS)
        {
            roll_back = 3;
            break;
        }

        /* The interrupt status IRPs. */
        hub->status_irp[0].hub    = hub;
        hub->status_irp[0].index  = 0;

        /* Allocate uncached memory buffer for port status value. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     mps,
                                     (VOID **) &(hub->status_irp[0].port_status));
        if (status != NU_SUCCESS)
        {
            roll_back = 4;
            break;
        }

        memset (hub->status_irp[0].port_status, 0, mps);
        status = NU_USB_IRP_Create (
                           &(hub->status_irp[0].irp),
                           mps,
                           (hub->status_irp[0].port_status),
                           NU_TRUE,
                           NU_FALSE,
                           usb_hub_intr_tx_complete,
                           &hub->status_irp[0],
                           0);
        if (status != NU_SUCCESS)
        {
            roll_back = 5;
            break;
        }

        hub->status_irp[1].hub = hub;
        hub->status_irp[1].index = 1;

        /* Allocate uncached memory buffer for port status value. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     mps,
                                     (VOID **) &(hub->status_irp[1].port_status));
        if (status != NU_SUCCESS)
        {
            roll_back = 5;
            break;
        }

        memset (hub->status_irp[1].port_status, 0, mps);
        status = NU_USB_IRP_Create (
                       &(hub->status_irp[1].irp),
                       mps,
                       (hub->status_irp[1].port_status),
                       NU_TRUE,
                       NU_FALSE,
                       usb_hub_intr_tx_complete,
                       &hub->status_irp[1],
                       0);
        if (status != NU_SUCCESS)
        {
            roll_back = 6;
            break;
        }

        status = NU_Reset_Semaphore (&(hub_driver->irpCompleteLock),0);
        if (status != NU_SUCCESS)
        {
            roll_back = 6;
            break;
        }

        /* Allocate uncached memory buffer for hub raw buffers. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     USBH_HUB_MAX_DESC_LEN,
                                     (VOID **) &hub->raw_desc);
        if(status != NU_SUCCESS)
        {
            roll_back = 6;
            break;
        }
        memset (hub->raw_desc, 0, USBH_HUB_MAX_DESC_LEN);

        /* Get and Fill the Hub descriptor. */
        status = usb_hub_desc_req ((NU_USBH_STACK *) stk, hub);
        if (status != NU_SUCCESS)
        {
            roll_back = 7;
            break;
        }

        /* Now we need to populate the usb 2.0 and usb 3.0 desc
         * depending on the device speed.
         */
        USBH_HUB_INIT_DESCRIPTOR(hub, dev->speed);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

        /* Since the set_hub_depth request is only for SuperSpeed device
         * therfore we need to check device speed before using this
         * request.
         */
        if (!USBH_IS_ROOT_HUB(dev))
        {
            temp_device = dev;

            if (temp_device->speed == USB_SPEED_SUPER)
            {
                /* Increment the hub depth for each hub other than
                 * the root hub.The hub at tier 2 has depth 0 as per
                 * specs.
                 */

                while ( !USBH_IS_ROOT_HUB(temp_device->parent) )
                {
                     hub_depth++;
                     temp_device = temp_device->parent ;
                }

                status = usbh_hub_set_depth_req((NU_USBH_STACK *) stk, hub,
                                                 hub_depth );
                if (status != NU_SUCCESS)
                {
                    roll_back = 7;
                    break;
                }
            }
        }

        /* Update the device status as hub in the HW. */
        if ( !USBH_IS_ROOT_HUB(dev) && (dev->speed == USB_SPEED_HIGH) )
        {
            tt_time = USBH_HUB_THINK_TIME(hub->hub20_desc->wHubCharacteristics0);
            num_ports = hub->hub20_desc->bNbrPorts;

            status = NU_USBH_HW_Update_Hub_Device((NU_USBH_HW *)dev->hw, dev, tt_time, num_ports );
            if (status != NU_SUCCESS)
            {
                roll_back = 7;
                break;
            }
        }

#endif      /* (  CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
        /* Since the first seven fields of USB 3.0 and USB 2.0 hub desc
         * are same so we can save them in the hub desc header structure
         * for further reference.
         */
        hub_hdr = (USBH_HUB_DESC_HEADER *) hub->raw_desc;

        if (USBH_IS_ROOT_HUB (dev) == 0)
        {
            int_status |= usb_hub_get_hub_status ((NU_USBH_STACK *) stk,
                                                  hub, &portStatus);

            for (index = 1; index <= hub_hdr->bNbrPorts; index++)
            {
                int_status |= usb_hub_get_port_status (
                                                   (NU_USBH_STACK *) stk,
                                                   hub,
                                                   &portStatus,
                                                   (UINT8)index);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

                if (!(portStatus.status & USBH_SS_HUB_FEATURE_PORT_POWER))
                {
                        int_status |= usb_hub_set_port_feature (
                                                      (NU_USBH_STACK *) stk,
                                                      hub,
                                                      USBH_HUB_FEATURE_PORT_POWER,
                                                      (UINT8)index,
                                                      0x00);
                }
#else
                int_status |= usb_hub_set_port_feature (
                                              (NU_USBH_STACK *) stk,
                                              hub,
                                              USBH_HUB_FEATURE_PORT_POWER,
                                              (UINT8)index,
                                              0x00);

#endif
            }
        }

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

        /* Since the isochronous delay value is required for SuperSpeed
         * devices only, therefore we need to check device speed before
         * carry on with its calculation.
         */
        if ((dev->speed == USB_SPEED_SUPER) && (hub->hub30_desc != NU_NULL))
        {
            if (USBH_IS_ROOT_HUB (dev))
            {
                /* If the hub is root hub then isoch_delay field will hold
                 * the value of wHubDelay field present in the usb3.0 hub
                 * descriptor.
                 */
                hub->isoch_delay = hub->hub30_desc->wHubDelay;
            }
            else
            {
                /* Find the parent hub and extract the isoch_delay field
                 * from its control block. The value of isoch_delay field
                 * for newhub would be sum of wHubDelay value of new hub
                 * and isoch_delay value of parent hub.
                 */
                parent_hub = usb_find_hub(hub_driver, dev->parent);

                if (parent_hub == NU_NULL)
                {
                    roll_back = 7;
                    status = NU_USB_INVLD_HUB;
                    break;
                }

                hub->isoch_delay = parent_hub->isoch_delay +
                                               hub->hub30_desc->wHubDelay;
            }
        }

        /* Find the companion hub. */
        status = usbh_hub_find_companion(hub_driver, hub, &companion_hub);

        if (status == NU_SUCCESS)
        {
	        hub->companion_hub_device = companion_hub;
	        companion_hub->companion_hub_device = hub;
        }


#endif      /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )  */

        NU_Place_On_List ((CS_NODE **) & (hub_driver->session_list_head),
                           (CS_NODE *) hub);

        /* Submit an IRP. */
        status = NU_USB_PIPE_Submit_IRP (hub->interrupt_pipe,
                               (NU_USB_IRP *) & (hub->status_irp[0].irp));
        if (status != NU_SUCCESS)
        {
            roll_back = 8;
        }

    } while(0);

    /* If something went wrong then rollback. */
    switch(roll_back)
    {
        case 8:
            NU_Remove_From_List ((CS_NODE **) &(hub_driver->session_list_head),
                                 (CS_NODE *) hub);
        case 7:
            if(hub)
             {
                int_status |= USB_Deallocate_Memory ((VOID *)hub->raw_desc);
             }
        case 6:
            if(hub)
             {
                int_status |= USB_Deallocate_Memory(hub->status_irp[1].port_status);
             }
        case 5:
            if(hub)
             {
                int_status |= USB_Deallocate_Memory(hub->status_irp[0].port_status);
             }
        case 4:
             if(hub)
             {
                int_status |= NU_Delete_Semaphore (&(hub->lock));
             }
        case 3:
              if(hub)
              {
                 int_status |= USB_Deallocate_Memory (hub->ctrlIrp);
              }
        case 2:
             if(hub)
             {
                int_status |= USB_Deallocate_Memory (hub);
             }
        case 1:
            NU_UNUSED_PARAM(int_status);
            return status;
    }

    hub->refCount++;
    int_status |= NU_USB_INTF_Claim (intf, cb);

    if (status == NU_SUCCESS)
    {
        return (int_status);
    }
    else
    {
        return (status);
    }
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usb_hub_shutdown
 *
 * DESCRIPTION
 *      Hub shutdown routine. Removes hub and all connected devices from
 *      subsystem.
 *
 * INPUTS
 *      hub_driver      pointer to hub driver control block
 *      cb              pointer to stack control block
 *      hub             pointer to the hub to be shutdown
 *
 * OUTPUTS
 *      NU_USB_INVLD_ARG        Indicates some argument is invalid.
 *
 ************************************************************************/
static VOID usb_hub_shutdown (NU_USBH_DRVR_HUB * hub_driver,
                              NU_USBH_STACK * cb,
                              NU_USBH_DEVICE_HUB * hub)
{
    /* recursively traverse through the children of the hubs
     * and de-enumerate them.
     * */
    NU_USB_DEVICE *dev;
    NU_USBH_DEVICE_HUB *newHub;
    STATUS status = NU_SUCCESS;
    USBH_HUB_DESC_HEADER *hub_hdr;
    INT i = 0;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    UINT8 func_addr;
#endif

    if ( hub_driver == NU_NULL || cb == NU_NULL || hub == NU_NULL )
    {
        return ;
    }

    hub_hdr = (USBH_HUB_DESC_HEADER *) hub->raw_desc;

    for (i = 1; i <= hub_hdr->bNbrPorts; i++)
    {
        if(i > USBH_HUB_MAX_CHILDREN)
        {
            break;
        }
        dev = hub->child[i];

        /* If no device is connected on this port, continue */
        if (dev == NU_NULL)
            continue;

        /* If the connected device is a hub, recurse.
         * otherwise proceed with de-enumeration
         * */
        if ((newHub = usb_find_hub (hub_driver, dev)) != NU_NULL)
        {
            /* This child is a hub   */
            usb_hub_shutdown (hub_driver, cb, newHub);
        }
        else
        {
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
            func_addr = dev->function_address;
#endif
            status = USBH_Deenumerate_Device (cb, dev);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

            /* Release the resuorces acquired by the HW controller. */
            status |= NU_USBH_HW_Dinit_Device((NU_USBH_HW *)hub->device->hw, func_addr);

#endif

            NU_UNUSED_PARAM(status);
        }
        hub->child[i] = NU_NULL;
    }

    /* all children are quelched. die now */
    hub->state = USBH_HUB_SHUTDOWN;

    status = USBH_Deenumerate_Device (cb, hub->device);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

    /* Release the resuorces acquired by the HW controller. */
    status |= NU_USBH_HW_Dinit_Device((NU_USBH_HW *)hub->device->hw,
                                      hub->device->function_address);

#endif

    NU_UNUSED_PARAM(status);
}

/**********************************************************************
 * FUNCTION
 *      usb_hub_port_status_change
 *
 * DESCRIPTION
 *      This function processes the port change events reported by a
 *      HUB.
 *
 * INPUTS
 *      cb      pointer to stack control block.
 *      hub     Pointer to the hub that reported status change events
 *      status  Status reported.
 *
 * OUTPUTS
 *      NU_USB_INVLD_ARG        Indicates some argument is invalid.
 *
 ************************************************************************/
static VOID usb_hub_port_status_change (NU_USBH_STACK * cb,
                                        NU_USBH_DEVICE_HUB * hub,
                                        UINT32 status,
                                        UINT8 speed)
{
    /* This function is called by the tUSBD task whenever there is a
     * port status change reported in the interrupt endpoint data for
     * any hub.
     */
    UINT8               portNum, portMax;
    USBH_HUB_STATUS     portStatus;
    NU_USB_DEVICE       *childDev;
    NU_USBH_DEVICE_HUB  *newHub;
    STATUS              int_status = NU_SUCCESS;
    USBH_HUB_DESC_HEADER *hub_hdr;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    UINT8 func_addr;
#endif

    status >>= 1;

    if ( cb == NU_NULL || hub == NU_NULL )
    {
        return ;
    }

    hub_hdr = (USBH_HUB_DESC_HEADER *) hub->raw_desc;

    /* wait for 100 msec (debounce time) before requesting port status */
    usb_wait_ms(100);

    if(hub_hdr->bNbrPorts <= USBH_HUB_MAX_CHILDREN)
        portMax = hub_hdr->bNbrPorts;
    else
        portMax = USBH_HUB_MAX_CHILDREN;

    for (portNum = 1; portNum <= portMax; portNum++, status >>= 1)
    {
        /* If no status change for a port, ignore it    */
        if ((status & 0x01) == 0)
            continue;

        /* Retrieve the Port Status and Port Change status      */
        int_status |= usb_hub_get_port_status (cb, hub, &portStatus, portNum);

        /* Check if there is an SRP event from a root-hub. */
        if((portStatus.change & USBH_HUB_BIT_C_PORT_SRP) && USBH_IS_ROOT_HUB(hub->device))
        {
            /* Clear the event at the root-hub */
            int_status |= usb_hub_clear_port_feature (cb, hub,
                                        USBH_HUB_FEATURE_C_PORT_SRP,
                                        portNum, 0x00);

            /* The device needs to be reset and re-enumerated. */
            /* de-enumerate the device */
            childDev = hub->child[portNum];

            newHub = usb_find_hub (&(cb->hub_driver), childDev);

            if (newHub == NU_NULL)
            {
                int_status |= USBH_Deenumerate_Device (cb, childDev);
            }
            else
            {
                usb_hub_shutdown (&(cb->hub_driver), cb, newHub);
            }

            hub->child[portNum] = NU_NULL;

            /* power-up the port */
             int_status |= usb_hub_set_port_feature (cb, hub,USBH_HUB_FEATURE_PORT_POWER, portNum, 0x00);

            /* re-start enumeration */
            /* The following two lines simulate a port-connection.
             * code that follows will re-enumerate the device, from the
             * beginning.
             */
            portStatus.change |= USBH_HUB_BIT_C_PORT_CONNECTION;
            portStatus.status |= USBH_HUB_BIT_PORT_CONNECTION;
        }

        if (portStatus.change & USBH_HUB_BIT_C_PORT_CONNECTION)
        {
            /* connection changed : clear the event */
             int_status |= usb_hub_clear_port_feature (cb, hub,
                                        USBH_HUB_FEATURE_C_PORT_CONNECTION,
                                        portNum, 0x00);

            if (!(portStatus.status & USBH_HUB_BIT_PORT_CONNECTION))
            {
                childDev = hub->child[portNum];

                if (childDev == NU_NULL)
                    continue;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

                func_addr = childDev->function_address;

#else
                int_status |= usb_hub_clear_port_feature (cb, hub,
                                            USBH_HUB_FEATURE_PORT_ENABLE,
                                            portNum, 0x00);

#endif

                newHub = usb_find_hub (&(cb->hub_driver), childDev);

                if (newHub == NU_NULL)
                {
                    /* if a normal device       */
                    int_status |= USBH_Deenumerate_Device (cb, childDev);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

                    /* Free the data structures used by the controller driver.*/
                    int_status |= NU_USBH_HW_Dinit_Device((NU_USBH_HW *)hub->device->hw, func_addr);
#endif
                }
                else
                {
                    usb_hub_shutdown (&(cb->hub_driver), cb, newHub);
                }

                hub->child[portNum] = NU_NULL;
            }
            else
            {
                /* new connection here */
                /* check if the max depth exceeded  */
                INT            depth = 0;
                NU_USB_DEVICE  *parent = hub->device;
                NU_USB_DEVICE  *dev;

                do
                {
                    dev = parent;
                    depth++;
                }
                while ((NU_USB_DEVICE_Get_Parent (dev, &parent) == NU_SUCCESS)
                       && (depth < USBH_MAX_HUB_CHAIN));

                /* allow device connection only if within the permitted
                 * depth
                 * */
                if (depth < USBH_MAX_HUB_CHAIN)
                {

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

                    /* Initialize the data structures required by the controller driver.*/
                    int_status = NU_USBH_HW_Init_Device((NU_USBH_HW *)hub->device->hw);

#endif

                    usb_hub_port_connect (cb, hub->device, portNum,
                                          speed);
                }
                else
                {
                    /* Report status that attached device is not supported. */
                    int_status |= NU_USB_STACK_OTG_Report_Status((NU_USB_STACK *)cb,
                                                   NU_USB_MAX_HUB_EXCEEDED);
                }
            }
        }

        if (portStatus.change & USBH_HUB_BIT_C_PORT_ENABLE)
        {
            int_status |= usb_hub_clear_port_feature (cb, hub,
                                        USBH_HUB_FEATURE_C_PORT_ENABLE,
                                        portNum, 0x00);
        }

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )


        /* Acknowledge the link state state change. This is valid for
         * USB 3.0 devices only.
         */
        if (speed == USB_SPEED_SUPER)
        {
            if (portStatus.change & USBH_HUB_BIT_C_PORT_LS)
            {
                int_status |= usb_hub_clear_port_feature (cb, hub,
                                        USBH_HUB_FEATURE_C_PORT_LINK_STATE,
                                        portNum, 0x00);
            }
        }
        else
#endif
        {
            if (portStatus.change & USBH_HUB_BIT_C_PORT_SUSPEND)
            {
                int_status |= usb_hub_clear_port_feature (cb, hub,
                                          USBH_HUB_FEATURE_C_PORT_SUSPEND,
                                          portNum, 0x00);
            }
        }

        if (portStatus.change & USBH_HUB_BIT_C_PORT_OVER_CURRENT)
        {
            int_status |= usb_hub_clear_port_feature (cb, hub,
                                        USBH_HUB_FEATURE_C_PORT_OVER_CURRENT,
                                        portNum, 0x00);

            /* we need to wait till the over current condition disappears
             * and then set power for the port.
             * then wait for the power to settle down and then start
             * enumerating the device.
             *
             * Section 11.12.5 USB 2.0 Specification
             */
            if (!(portStatus.status & USBH_HUB_BIT_PORT_OVER_CURRENT))
            {
                int_status |= usb_hub_set_port_feature (cb, hub, USBH_HUB_FEATURE_PORT_POWER,
                                          portNum, 0x00);

                /* wait for the power to settle down */
                HUB_SLEEP (hub_hdr->bPwrOn2PwrGood * 2);
            }
        }

        if (portStatus.change & USBH_HUB_BIT_C_PORT_RESET)
        {
            int_status |= usb_hub_clear_port_feature (cb, hub, USBH_HUB_FEATURE_C_PORT_RESET,
                                        portNum, 0x00);
        }
    }/* end of for-loop for all ports. */

    NU_UNUSED_PARAM(int_status);

}/* Port Status change processed. */

/**********************************************************************
 *
 * FUNCTION
 *
 *      usb_hub_device_status_change
 *
 * DESCRIPTION
 *      This function processes the status change events reported by a
 *      HUB.
 *
 * INPUTS
 *      cb      pointer to host stack control block
 *      hub     Pointer to the hub that reported status change events
 *
 * OUTPUTS
 *      NU_USB_INVLD_ARG        Indicates some argument is invalid.
 *
 ************************************************************************/
static VOID usb_hub_device_status_change (NU_USBH_STACK * cb,
                                          NU_USBH_DEVICE_HUB * hub)
{
    /* This function is called by the tUSBD task whenever there is a
     * port status change reported in the interrupt endpoint data for
     * any hub.
     */
    UINT32 portNum;
    USBH_HUB_STATUS hubStatus;
    STATUS          int_status = NU_SUCCESS;
    USBH_HUB_DESC_HEADER *hub_hdr;

    if ( cb == NU_NULL || hub == NU_NULL )
    {
        return ;
    }

    hub_hdr = (USBH_HUB_DESC_HEADER *) hub->raw_desc;

    /* Retrieve the Port Status and Port Change status      */
    int_status |= usb_hub_get_hub_status (cb, hub, &hubStatus);

    if (hubStatus.change & USBH_HUB_BIT_C_POWER_LOCAL)
    {
         int_status |= usb_hub_clear_hub_feature (cb, hub, USBH_HUB_FEATURE_C_LOCAL_POWER);
    }

    if (hubStatus.change & USBH_HUB_BIT_C_OVER_CURRENT)
    {
         int_status |= usb_hub_clear_hub_feature (cb, hub, USBH_HUB_FEATURE_C_OVER_CURRENT);

        /* Wait for the over current to disappear   */

        if (!(hubStatus.status & USBH_HUB_BIT_OVER_CURRENT))

            for (portNum = 1; portNum <= hub_hdr->bNbrPorts; portNum++)
            {
                /* enable power for these ports */
                int_status |=  usb_hub_set_port_feature (cb, hub, USBH_HUB_FEATURE_PORT_POWER,
                                          (UINT8)portNum, 0x00);
            }

        /* wait for the power to settle down */
        HUB_SLEEP (hub_hdr->bPwrOn2PwrGood * 2);
    }

    NU_UNUSED_PARAM(int_status);
}

/**********************************************************************
 * FUNCTION
 *
 *      usb_hub_port_debounce
 *
 * DESCRIPTION
 *      This function implements the debounce wait for the given port
 *
 * INPUTS
 *      cb          pointer to host stack control block
 *      hub         Pointer to the hub
 *      portNum     Port to be debounced!.
 *      portStatus  Pointer where the retrieved status is stored
 *
 * OUTPUTS
 *
 *      NU_SUCCESS                      if port reports same status
 *                                      after debounce wait
 *      NU_USB_DEVICE_NOT_RESPONDING    if status changes after debounce wait
 *      NU_USB_INVLD_ARG        `       Indicates some argument is invalid.
 ************************************************************************/
static STATUS usb_hub_port_debounce (NU_USBH_STACK * cb,
                                     NU_USBH_DEVICE_HUB * hub,
                                     UINT8 portNum,
                                     USBH_HUB_STATUS * portStatus)
{
    /* The specification requires a debounce time of 100ms.
     * If there is a disconnection detected, the debouncing restarts.
     * To handle some devices, which may require longer durations to
     * stabilize, we wait for 200ms, checking the connection at every 50ms.
     * */
    UINT32 delay;
    UINT32 errCount = 0;
    STATUS status = NU_SUCCESS;

    if (cb == NU_NULL || hub == NU_NULL || portStatus  == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }

    for (delay = 0; delay < USBH_HUB_DEBOUNCE_TIME;)
    {
        /* Sleep for some time  */
        HUB_SLEEP (USBH_HUB_DEBOUNCE_STEP);

        /* Check the status now */
        status |= usb_hub_get_port_status (cb, hub, portStatus, portNum);

        /* if disconnected already, then re-start the debounce  */
        if (!(portStatus->status & USBH_HUB_BIT_PORT_CONNECTION))
        {
            status |= usb_hub_clear_port_feature (cb, hub,
                                        USBH_HUB_FEATURE_C_PORT_CONNECTION,
                                        portNum, 0x00);
            errCount++;
            /* check the errors */
            if (errCount >= USBH_HUB_MAX_DEBOUNCE_ERRORS)
            {
                return NU_USB_DEVICE_NOT_RESPONDING;
            }

            delay = 0;
        }
        else
        {
            delay += USBH_HUB_DEBOUNCE_STEP;
        }
    }

    NU_UNUSED_PARAM(status);
    /* now the hub port is stabilized.
     * Check if the device is still connected or not
     * */
    if (portStatus->status & USBH_HUB_BIT_PORT_CONNECTION)
    {
        return (NU_SUCCESS);
    }
    else
    {
        return NU_USB_DEVICE_NOT_RESPONDING;
    }
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usb_hub_port_reset
 *
 * DESCRIPTION
 *      This function resets the given port of a given hub.
 *
 * INPUTS
 *  'cb'        : pointer to stack control block
 *  'hub'       : Pointer to the hub driver control block
 *  'portNum'   : Port to be issued a RESET
 *  'speed'     : Pointer where speed of the detected device is stored
 *
 * OUTPUTS
 *
 *  NU_SUCCESS                   if port reset is successful
 *  NU_USB_DEVICE_NOT_RESPONDING if Reset could not be completed
 *  NU_USB_INVLD_ARG             Indicates some argument is invalid.
 ************************************************************************/
static STATUS usb_hub_port_reset (NU_USBH_STACK * cb,
                                  NU_USBH_DEVICE_HUB * hub,
                                  UINT8 portNum,
                                  UINT8 *speed)
{
    /* The algorithm followed is as follows :
     *
     * 1. A set_feature(port_reset) will be issued.
     * 2. we wait for specified duration.
     * 3. We check if port status is updated to be reset and port is
     * enabled. If so, we read the speed of the connected device and
     * return.
     * 4. If device is disconnected then error will be returned.
     * 5. If neither 3 or 4 happen, we increment the delay time and issue a
     * reset again.
     * 6. All this is done in a loop
     * */

    UINT32 delay = USBH_HUB_RESET_MIN_TIME;
    UINT32 i = 0, j = 0;
    USBH_HUB_STATUS portStatus;
    STATUS status = NU_USB_DEVICE_NOT_RESPONDING;

    if (cb == NU_NULL || hub == NU_NULL || speed  == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }

    for (i = 0; i < USBH_HUB_RESET_MAX_TRIES; i++)
    {
        /* Issue a port reset   */
        status |= usb_hub_set_port_feature (cb, hub, USBH_HUB_FEATURE_PORT_RESET,
                                  portNum, 0x00);

        for (j = 0; j < 5; j++)
        {
            /* Allow the reset signalling   */
            HUB_SLEEP (delay);

            /* get the port status and check for the status of reset    */
            status |= usb_hub_get_port_status (cb, hub, &portStatus, portNum);

            /* See if the device is disconnected    */
            if (!(portStatus.status & USBH_HUB_BIT_PORT_CONNECTION))
            {
                return (status);
            }

            if ((portStatus.change & USBH_HUB_BIT_C_PORT_RESET) &&
                (portStatus.status & USBH_HUB_BIT_PORT_ENABLE))
            {
                /* Reset signalling is complete */
                status |= usb_hub_clear_port_feature (cb, hub,
                                            USBH_HUB_FEATURE_C_PORT_RESET,
                                            portNum, 0x00);

                /* We need to decode the bits of of wPortStatus
                 * registers depending on the speed we initially got.
                 * This is required because the bit encoding in the
                 * USB 3.0 wPortStatus register is different from the
                 * bit enoding of USB 2.0 wPortStatus register.
                 */

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                /* After a reset, the port would have been enabled.
                 * Read the port speed.
                 */
                if (*speed != USB_SPEED_SUPER)
#endif
                {

                    if (portStatus.status & USBH_HUB_BIT_PORT_LOW_SPEED)
                    {
                        *speed = USB_SPEED_LOW;
                    }
                    else if (portStatus.status & USBH_HUB_BIT_PORT_HIGH_SPEED)
                    {
                        *speed = USB_SPEED_HIGH;
                    }
                    else
                    {
                        *speed = USB_SPEED_FULL;
                    }
                }

                /* As we have to return NU_SUCCESS in this case so set
                 * status as unused parameter to remove KW and
                 * PC-Lint warnings.
                 */
                NU_UNUSED_PARAM(status);
                return (NU_SUCCESS);
            }

            /* next trial, wait for long time.  */
            delay = USBH_HUB_RESET_MAX_TIME;
        }
    }

    if ((!USBH_IS_ROOT_HUB(hub->device)))
    {

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

        if ( *speed == USB_SPEED_SUPER )
        {
            status |=  usbh_hub_port_suspend_link(cb, hub, portNum);
        }
        else
#endif

        {
            status |= usb_hub_set_port_feature(cb, hub,
                               USBH_HUB_FEATURE_PORT_SUSPEND,
                               portNum, 0x00);
            status |= usb_hub_clear_port_feature(cb, hub,
                                                 USBH_HUB_FEATURE_PORT_SUSPEND,
                                                portNum, 0x00);
        }
    }

    return (status);
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usb_hub_port_connect
 *
 * DESCRIPTION
 *      This function is called when a new device is reported on a hub
 *
 * INPUTS
 *      cb          pointer to stack control block
 *      device      pointer to device control block connected.
 *      portNum     Port on which new device is detected
 *      speed       speed of the detected device
 *
 * OUTPUTS
 *      NU_USB_INVLD_ARG        Indicates some argument is invalid.
 *
 ************************************************************************/
VOID usb_hub_port_connect (NU_USBH_STACK * cb,
                           NU_USB_DEVICE * device,
                           UINT8 portNum,
                           UINT8 speed)
{
    NU_USBH_DEVICE_HUB  *hub;
    INT                 i = 0;
    USBH_HUB_STATUS     portStatus;
    NU_USB_DEVICE       *child = NU_NULL;
    NU_USB_DEVICE       *root_hub;
    STATUS  int_status = NU_SUCCESS;

    if (cb == NU_NULL || device == NU_NULL)
    {
        return ;
    }

    /* There is a connect on this port of the hub,  */

    /* The algorithm is simple...
     *
     * 1. Wait for the debounce time on the port.
     * 2. In a loop, try the following -
     *    1. Issue a Port_Reset for a specified interval.
     *    2. Ask the USBD to enumerate device.
     */

    hub = usb_find_hub (&(cb->hub_driver), device);

    if (hub == NU_NULL)
    {
        return;
    }

    /* Traverse down the hubs until we arrive at the root hub */
    root_hub = device->parent;
    if (root_hub != NU_NULL)
    {
        while(root_hub->parent != NU_NULL)
        {
            root_hub = root_hub->parent;
        }
    }

    /* First wait for the debounce time */
    if (usb_hub_port_debounce (cb, hub, portNum, &portStatus) != NU_SUCCESS)
    {
        /* Not much we can do about.
         * This device is behaving irresponsibly.
         * */
        return;
    }

    /* Applying Reset and Enumeration in a loop */
    for (i = 0; i < USBH_HUB_MAX_ENUM_RETRIES; i++)
    {
        /* Reset the port */
        if (usb_hub_port_reset (cb, hub, portNum, &speed) != NU_SUCCESS)
        {
            /* disable the port */
            break;
        }

        /* Enumerate the device */
        if (hub->child[portNum])
        {
            NU_USBH_DEVICE_HUB *newHub;

            /* First deenumerate the existing child */
            newHub = usb_find_hub (&(cb->hub_driver), hub->child[portNum]);

            if (newHub == NU_NULL)
            {
                /* if a normal device */
                int_status |= USBH_Deenumerate_Device (cb, hub->child[portNum]);
            }
            else
            {
               usb_hub_shutdown (&(cb->hub_driver), cb, newHub);
            }
            hub->child[portNum] = NU_NULL;
        }

        if (USBH_Enumerate_Device (cb, device, portNum, speed, &child) ==
            NU_SUCCESS)
        {
            /* Enumeration over */
            /* Add this child to the tree   */
            hub->child[portNum] = child;

            return;
        }

        /* Check if the device is still connected */
        /* Check the status now */
        int_status |=  usb_hub_get_port_status (cb, hub, &portStatus, portNum);

        /* if disconnected already, cancel enumeration retries */
        if (!(portStatus.status & USBH_HUB_BIT_PORT_CONNECTION))
        {
            int_status |=  usb_hub_clear_port_feature (cb, hub,
                                        USBH_HUB_FEATURE_C_PORT_CONNECTION,
                                        portNum, 0x00);
            break;
        }
    }

    if(device->parent != root_hub)
    {
        /* error conditions     */
        /* Either reset or the enumeration has failed.*/
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

        if ( speed == USB_SPEED_SUPER )
        {
            int_status |=  usbh_hub_port_suspend_link(cb, hub, portNum);
        }

    else

#endif
        {
            int_status |= usb_hub_set_port_feature(cb, hub,
                               USBH_HUB_FEATURE_PORT_SUSPEND,
                               portNum, 0x00);
            int_status |=  usb_hub_clear_port_feature (cb, hub,
                               USBH_HUB_FEATURE_PORT_ENABLE,
                               portNum, 0x00);
        }
    }
    NU_UNUSED_PARAM(int_status);
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usb_hub_ctrl_tx_complete
 *
 * DESCRIPTION
 *      This function is the Ctrl endpoint transfer completion handler.
 *      It releases the irpCompleteLock on successful transfers
 *
 * INPUTS
 *      pipe                pointer to pipe control block.
 *      irp                 pointer to the completed IRP's control block
 *
 * OUTPUTS
 *      NU_USB_INVLD_ARG        Indicates some argument is invalid.
 *
 ************************************************************************/
static VOID usb_hub_ctrl_tx_complete (NU_USB_PIPE * pipe,
                                      NU_USB_IRP * irp)
{
    /* This function just releases the irpCompleteLock and lets the
     * control return to the function that initiated the Control
     * transfer
     * */
    NU_USBH_STACK *stack;
    STATUS status  = NU_SUCCESS;

    if (pipe == NU_NULL)
    {
        return ;
    }

    stack = (NU_USBH_STACK *) (pipe->device->stack);
    status = NU_Release_Semaphore (&(stack->hub_driver.irpCompleteLock));

    NU_UNUSED_PARAM(status);
}

/**********************************************************************
 *
 * FUNCTION
 *      usb_hub_intr_tx_complete
 *
 * DESCRIPTION
 *      This function is the Intr endpoint transfer completion handler.
 *      It submits a new IRP and processes the events in the first  one
 *
 * INPUTS
 *      pipe            pointer to pipe control block
 *      irp             pointer to the completed IRP's control block
 *
 * OUTPUTS
 *      NU_USB_INVLD_ARG        Indicates some argument is invalid.
 *
 ************************************************************************/
static VOID usb_hub_intr_tx_complete (NU_USB_PIPE * pipe,
                                      NU_USB_IRP * irp)
{
    NU_USBH_STACK     *stack;
    NU_USBH_DRVR_HUB  *hub_driver;
    STATUS status  = NU_SUCCESS;

    if (pipe == NU_NULL)
    {
        return ;
    }
    stack = (NU_USBH_STACK *) (pipe->device->stack);
    hub_driver = &stack->hub_driver;

    status = NU_Send_To_Queue(&(hub_driver->queue), (VOID **) &irp, 1, NU_NO_SUSPEND);
    NU_UNUSED_PARAM(status);
}

/**********************************************************************
 * FUNCTION
 *      USBH_Hub_Task
 *
 * DESCRIPTION
 *      This is the USB-HUB task's entry function. It waits for the
 *        eventListLock to be available and then processes the events
 *        associated.
 *
 * INPUTS
 *      argc                            Not used
 *      argv                            Not used
 *
 * OUTPUTS
 *      None
 *
 ************************************************************************/
VOID USBH_Hub_Task (UNSIGNED argc, VOID *argv)
{
    NU_USB_IRP          *irp;
    NU_USBH_DEVICE_HUB  *hub, *parent_hub;
    UINT32              data;
    NU_USB_DEVICE       *parent;
    UINT8               portNumber;
    UINT32              curIrpIndex;
    UINT32              nextIrpIndex;
    NU_USB_IRP          *nextIrp;
    NU_USBH_DRVR_HUB    *hcb = (NU_USBH_DRVR_HUB *) argv;
    NU_USBH_STACK       *stack;
    USBH_IRP_HUB        *context = NU_NULL;
    STATUS              irp_status, int_status = NU_SUCCESS;
    UINT32              *irp_data = NU_NULL;
    NU_USB_DEVICE       *device = NU_NULL;
    NU_USB_PIPE         *pipe = NU_NULL;
    UINT8               speed;
    UNSIGNED            actual_size =0 ;

    while (1)
    {
        int_status |= NU_Receive_From_Queue(&(hcb->queue), &irp, 1, &actual_size, NU_SUSPEND);

        if(actual_size == 1)
        {
            int_status |= NU_USB_IRP_Get_Context (irp, (VOID **) &context);
            hub = context->hub;
        }
        else
        {
            continue;
        }

        /* Check if the hub is to be discarded */
        stack = hcb->stack;

        /* decrement the reference count        */
        /* clean up the device if found DIRTY   */
        if (hub->refCount != 0)
        {
            hub->refCount--;
        }

        if (hub->state == USBH_HUB_SHUTDOWN)
        {
            /* no more irps */
            continue;
        }
        else if (hub->state == USBH_HUB_DIRTY)
        {
            if (hub->refCount == 0)
            {
                int_status |= _NU_USBH_HUB_Disconnect ((NU_USB_DRVR *) hcb,
                                         (NU_USB_STACK *) stack,
                                         (NU_USB_DEVICE *) hub->device);
            }
            continue;
        }

        int_status |= NU_USB_IRP_Get_Status (irp, &irp_status);

        if ((irp_status != NU_SUCCESS) && (irp_status != NU_USB_IRP_CANCELLED))
        {
            /* Keep track of how many errors the device is making
             */
            hub->error_count++;
            if (hub->error_count >= 10)
            {
                /* Too many errors, discard the device */
                /* find its parent. */
                if (NU_USB_DEVICE_Get_Parent (hub->device, &parent) !=
                    NU_SUCCESS)
                {
                    continue;
                }
                /* get the port number  */
                if (NU_USB_DEVICE_Get_Port_Number (hub->device, &portNumber) !=
                    NU_SUCCESS)
                {
                    continue;
                }
                /* find its parent hub. */
                if ((parent_hub = usb_find_hub (hcb, parent)) == NU_NULL)
                {
                    continue;
                }
                else
                {
                    usb_hub_shutdown (hcb, stack, hub);
                    parent_hub->child[portNumber] = NU_NULL;
                }
                continue;
            }
        }
        else
            /* Reset the error counter on successful transfer. */
            hub->error_count = 0;

        /* Submit another IRP */
        curIrpIndex = context->index;
        nextIrpIndex = (curIrpIndex + 1) % USBH_HUB_MAX_STATUS_IRPS;
        nextIrp = &hub->status_irp[nextIrpIndex].irp;
        hub->refCount++;
        if (NU_USB_PIPE_Submit_IRP (hub->interrupt_pipe, (NU_USB_IRP *) nextIrp)
            != NU_SUCCESS)
        {
            hub->refCount--;
        }

        if (irp_status != NU_SUCCESS)
            continue;

        /* Successful irp. Check for the PortStatusChange bitmap   */
        int_status |= NU_USB_IRP_Get_Data (irp, (UINT8 **) &irp_data);
        data = LE32_2_HOST ((*irp_data));

        if (data == 0)
        {
            continue;
        }

        /* Has the hub status changed ?         */
        if (data & 0x01)
        {
            /* Process the hub status change    */
            usb_hub_device_status_change (stack, hub);
        }

        /* We need device speed in order to distinguish USB 3.0 and
         * USB 2.0 device. For this we proceed as follow
         */

        /* Get the point pointer from the IRP control block. */
        NU_USB_IRP_Get_Pipe(irp, &pipe);

        /* Get the device pointer from the pipe control block. */
        NU_USB_PIPE_Get_Device(pipe, &device);

        /* Get the speed HUB spped where IRP was submitted. */
        NU_USB_DEVICE_Get_Speed(device, &speed);

        /* Remove the Hub Status Bit            */
        data &= ~0x01;

        /* If any of the Port Status is changed */
        if (data != 0)
        {
            /* Process the port status change   */
           usb_hub_port_status_change (stack, hub, data, speed);
        }

        NU_UNUSED_PARAM(int_status);
    }
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      USBH_HUB_Disconnect_Device
 *
 * DESCRIPTION
 *      Disconnect a device from the topology
 *
 * INPUTS
 *      hub_driver      pointer to hub driver control block
 *      dev             pointer to device disconnected.
 *
 * OUTPUTS
 *      NU_SUCCESS          on success
 *      NU_USB_INVLD_ARG    if device could not be found
 *
 ************************************************************************/
STATUS USBH_HUB_Disconnect_Device (NU_USBH_DRVR_HUB * hub_driver,
                                   NU_USB_DEVICE * dev)
{
    NU_USBH_DEVICE_HUB *hub;
    NU_USB_DEVICE *parent;
    UINT8 portNumber;
    STATUS int_status = NU_SUCCESS;

    if (hub_driver == NU_NULL || dev == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }

    hub = usb_find_hub (hub_driver, dev);
    if (hub == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    /* if its the root-hub, shutdown    */
    if (USBH_IS_ROOT_HUB (dev))
    {
        usb_hub_shutdown (hub_driver, hub_driver->stack, hub);
        return (NU_SUCCESS);
    }

    /* find its parent hub  */
    if (NU_USB_DEVICE_Get_Parent (dev, &parent) != NU_SUCCESS)
    {
        return NU_USB_INVLD_ARG;
    }

    /* get the port number  */
    if (NU_USB_DEVICE_Get_Port_Number (dev, &portNumber) != NU_SUCCESS)
    {
        return NU_USB_INVLD_ARG;
    }

    if ((hub = usb_find_hub (hub_driver, parent)) == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }
    /* disable the hub-port */
    int_status |= usb_hub_clear_port_feature (hub_driver->stack, hub,
                                USBH_HUB_FEATURE_PORT_ENABLE, portNumber, 0x00);

    /* call de-enumerate function   */
    int_status |= USBH_Deenumerate_Device (hub_driver->stack, dev);
    hub->child[portNumber] = NU_NULL;

    NU_UNUSED_PARAM(int_status);
    return (NU_SUCCESS);
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      _NU_USBH_HUB_Delete
 *
 * DESCRIPTION
 *
 *      This function deinitializes the hub class driver.
 *
 * INPUTS
 *      cb          pointer to hub class driver control block
 *
 * OUTPUTS
 *      NU_SUCCESS          always
 *      NU_USB_INVLD_ARG    Invalid parameters
 *
 ************************************************************************/
STATUS _NU_USBH_HUB_Delete (VOID *cb)
{
    NU_USBH_DRVR_HUB   *hub_driver   = NU_NULL;
    NU_USBH_DEVICE_HUB *hub          = NU_NULL;
    STATUS              status       = NU_SUCCESS;
    STATUS              internal_sts = NU_SUCCESS;

    if (cb == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }

    hub_driver = (NU_USBH_DRVR_HUB *) cb;

    /* find the first hub */
    hub = hub_driver->session_list_head;

    /* de-init all the hubs */
    while ((hub != NU_NULL) && (status == NU_SUCCESS))
    {
        hub->state = USBH_HUB_DIRTY;

        status = NU_USB_INTF_Release (hub->intf, (NU_USB_DRVR *) cb);

        if (status == NU_SUCCESS)
        {
            status = _NU_USBH_HUB_Disconnect ((NU_USB_DRVR *) hub_driver,
                                           hub->stack, hub->device);
        }

        /* The session list head moves to the next node(HUB DEVICE)
         * as the hub is deleted in the _NU_USBH_HUB_Disconnect API.
         * Assigning the session_list_head to the hub makes it point
         * to the next hub device.
         */
         hub = hub_driver->session_list_head;
    }

    internal_sts |= NU_Terminate_Task (&hub_driver->hubTask);
    internal_sts |= NU_Delete_Task (&hub_driver->hubTask);
    internal_sts |= NU_Delete_Semaphore (&(hub_driver->irpCompleteLock));
    internal_sts |= NU_Delete_Queue(&(hub_driver->queue));
    internal_sts |= NU_USB_STACK_Deregister_Drvr ((NU_USB_STACK *) (hub_driver->stack),
                                         (NU_USB_DRVR *) hub_driver);
    internal_sts |= _NU_USB_DRVR_Delete((VOID *) hub_driver);

    if (status == NU_SUCCESS)
    {
        return (internal_sts);
    }
    else
    {
        return (status);
    }
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usbh_hub_get_port_capabilities
 *
 * DESCRIPTION
 *      This function retrieves OTG capabilities of the port.
 *
 * INPUTS
 *      hub_driver      pointer to hub driver control block
 *      hub_dev         pointer to hub device.
 *      port            port number
 *      capability_out  location where the port capabilities are to be
 *                      filled.
 *
 * OUTPUTS
 *
 *      NU_SUCCESS          on success
 *      NU_USB_INVLD_ARG    if device /port could not be found
 *
 ************************************************************************/
STATUS usbh_hub_get_port_capabilities (NU_USBH_DRVR_HUB * hub_driver,
                                       NU_USB_DEVICE * hub_dev,
                                       UINT8 port_num,
                                       UINT8 *capability_out)
{
    NU_USBH_DEVICE_HUB *hub;
    INT offset;
    UINT8 *ptr;
    UINT8 byte_pos;
    UINT8 bit_pos;

    if (hub_driver == NU_NULL || hub_dev == NU_NULL
        || capability_out == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }
    /* Assuming that this function is called only for USB 2.0 devices.
     * That is why we are using the USB 2.0 desc.
     */

    hub = usb_find_hub (hub_driver, hub_dev);

    if (hub == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    if (hub->hub20_desc->bNbrPorts < port_num)
    {
        return NU_USB_INVLD_ARG;
    }

    offset = (hub->hub20_desc->bNbrPorts + 8 ) / 8;

    *capability_out = 0;

    ptr = &hub->hub20_desc->misc[0][0];

    byte_pos = (port_num) / 8;
    bit_pos = (port_num) % 8;

    /* advance ptr through DeviceRemovable and PortPwrMask */
    ptr += 2*offset;

    /* SRP support */
    *capability_out = (UINT8)(ptr[byte_pos] & (0x1 << bit_pos));

    ptr += offset;

    /* HNP support */
    *capability_out |= ((ptr[byte_pos] & (0x1 << bit_pos)) << 1);

    return (NU_SUCCESS);
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usbh_hub_suspend_port
 *
 * DESCRIPTION
 *
 *      This function suspends a given port of the hub.
 *      if port_num is 255, all ports on the hub will be suspended.
 *
 * INPUTS
 *
 *      hub_driver      pointer to hub driver control block
 *      hub_dev         pointer to hub device.
 *      port            port number
 *
 * OUTPUTS
 *
 *      NU_SUCCESS          on success
 *      NU_USB_INVLD_ARG    if device /port could not be found
 *
 ************************************************************************/
STATUS usbh_hub_suspend_port(NU_USBH_DRVR_HUB * hub_driver,
                             NU_USB_DEVICE * hub_dev,
                             UINT8 port_num)
{
    NU_USBH_DEVICE_HUB *hub;
    USBH_HUB_DESC_HEADER *hub_hdr;

    if (hub_driver == NU_NULL || hub_dev == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }

    hub = usb_find_hub (hub_driver, hub_dev);

    if (hub == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    hub_hdr = (USBH_HUB_DESC_HEADER *) hub->raw_desc;

    if(port_num != 255)
    {
        if (hub_hdr->bNbrPorts < port_num)
        {
            return NU_USB_INVLD_ARG;
        }
    }
    return (usb_hub_set_port_feature (hub_driver->stack, hub,
                    USBH_HUB_FEATURE_PORT_SUSPEND, port_num, 0x00));
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usbh_hub_resume_port
 *
 * DESCRIPTION
 *
 *      This function resumes a given port of the hub.
 *      if port_num is 255, all ports on the hub will be resumed.
 *
 * INPUTS
 *
 *      hub_driver      pointer to hub driver control block
 *      hub_dev         pointer to hub device.
 *      port            port number
 *
 * OUTPUTS
 *
 *      NU_SUCCESS          on success
 *      NU_USB_INVLD_ARG    if device /port could not be found
 *
 ************************************************************************/
STATUS usbh_hub_resume_port(NU_USBH_DRVR_HUB * hub_driver,
                             NU_USB_DEVICE * hub_dev,
                             UINT8 port_num)
{
    NU_USBH_DEVICE_HUB *hub;
    USBH_HUB_DESC_HEADER *hub_hdr;

    if (hub_driver == NU_NULL || hub_dev == NU_NULL)
    {
        return (NU_USB_INVLD_ARG);
    }
    hub = usb_find_hub (hub_driver, hub_dev);

    if (hub == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    hub_hdr = (USBH_HUB_DESC_HEADER *) hub->raw_desc;

    if(port_num != 255)
    {
        if (hub_hdr->bNbrPorts < port_num)
            return NU_USB_INVLD_ARG;
    }

    return (usb_hub_clear_port_feature (hub_driver->stack, hub,
                    USBH_HUB_FEATURE_PORT_SUSPEND, port_num, 0x00));
}

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
/**********************************************************************
 *
 * FUNCTION
 *
 *      usbh_hub_find_companion
 *
 * DESCRIPTION
 *
 *      This function will search the companion hub on the basis of
 *      ContainerID present in BOS descriptor.
 *
 * INPUTS
 *
 *      cb              pointer to hub driver control block
 *      hub             pointer to hub device.
 *      companion_hub   Pointer to NU_USBH_DEVICE_HUB control block,
 *                      representing the companion hub already connected.
 *
 * OUTPUTS
 *
 *      NU_SUCCESS          Operation Completed Successfully.
 *      NU_USB_INVLD_ARG    Any of the input argument is invalid.
 *      NU_USB_NOT_FOUND    No companion hub present.
 *
 ************************************************************************/
STATUS usbh_hub_find_companion  (NU_USBH_DRVR_HUB   *cb,
                                 NU_USBH_DEVICE_HUB *hub,
                                 NU_USBH_DEVICE_HUB **companion_hub)
{
    STATUS             status        = NU_SUCCESS;
    NU_USBH_DEVICE_HUB *comp_hub     = NU_NULL;
    NU_USB_BOS         *bos          = NU_NULL;
    UINT8              *cntr_id_src  = NU_NULL;
    UINT8              *cntr_id_dest = NU_NULL;
    UINT8              index         = 0;
    BOOLEAN            hub_found     = NU_TRUE;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(hub);

    comp_hub = cb->session_list_head;

    /* If there is no hub present in the system. */
    if (comp_hub == NU_NULL)
    {
        *companion_hub = NU_NULL;
        status      = NU_USB_NOT_FOUND;
    }

    /* Get the BOS descriptor handle. */
    if (status == NU_SUCCESS)
    {
        status = NU_USB_DEVICE_Get_BOS(hub->device, &bos);
    }

    /* Get the container ID for Container ID descriptor. */
    if (status == NU_SUCCESS)
    {
        status = NU_USB_DEVCAP_CntnrID_Get_CID(bos, &cntr_id_src);
    }

    /* Search the companion hub on the basis of container ID found above.*/
    while ((status == NU_SUCCESS) && (comp_hub != NU_NULL))
    {
        status = NU_USB_DEVICE_Get_BOS(comp_hub->device, &bos);

        if (status == NU_SUCCESS)
        {
            status = NU_USB_DEVCAP_CntnrID_Get_CID(bos, &cntr_id_dest);
        }

        if (status == NU_SUCCESS)
        {
            /* Match the 16 byte container ID. */
            for (index = 0 ; index < CONTAINER_ID_SIZE ; index++)
            {
                if (cntr_id_src[index] != cntr_id_dest[index])
                {
                     hub_found = NU_FALSE;
                     break;
                }
            }

            if (hub_found == NU_TRUE)
            {
                *companion_hub = comp_hub;
                break;
            }

            comp_hub = (NU_USBH_DEVICE_HUB *) comp_hub->node.cs_next;

            if ( comp_hub == cb->session_list_head)
            {
                comp_hub = NU_NULL;
            }
       }
    }

    if ( (status != NU_SUCCESS) || (comp_hub == NU_NULL) )
    {
       *companion_hub = NU_NULL;
       status = NU_USB_NOT_FOUND;
    }

    return status;
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usbh_hub_set_depth_req
 *
 * DESCRIPTION
 *
 *     This request sets the value that the hub uses to determine
 *     the index into the route string index of the hub.
 *
 * INPUTS
 *
 *      cb              Pointer to hub driver control block
 *      hub             Pointer to hub device.
 *      hub_depth       Variable containing the level of hub depth.
 *                      This value should not be greater than 4.
 *
 * OUTPUTS
 *
 *      NU_SUCCESS          Operation Completed Successfully.
 *      NU_USB_INVLD_ARG    Any of the input argument is invalid.
 *
 ************************************************************************/
STATUS usbh_hub_set_depth_req    (NU_USBH_STACK       *cb,
                                  NU_USBH_DEVICE_HUB  *hub,
                                  UINT8               hub_depth)
{
    STATUS           status     = NU_SUCCESS;
    STATUS           irp_status = NU_SUCCESS;
    STATUS           int_status = NU_SUCCESS;
    NU_USBH_CTRL_IRP *irp       = NU_NULL;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(hub);

    if (hub_depth > USBH_HUB_MAX_DEPTH)
    {
        status = NU_USB_INVLD_ARG;
    }

    irp = hub->ctrlIrp;

    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore (&(hub->lock), NU_SUSPEND);
    }

    /* Fill the control packet for SET_HUB_DEPTH request. */
    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_bRequest (irp,
                                                USBH_HUB_CMD_SET_DEPTH);
    }

    if(status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_wLength (irp,0x00);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_bmRequestType (irp, 0x20);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_wValue (irp, HOST_2_LE16(hub_depth));
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_wIndex (irp, 0);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Set_Data ((NU_USB_IRP *) irp, NU_NULL);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Set_Length ((NU_USB_IRP *) irp, 0x00);
    }

    if (status == NU_SUCCESS)
    {
        /* Submit the IRP   */
        status = NU_USB_PIPE_Submit_IRP (hub->default_pipe,
                                        (NU_USB_IRP *) irp);
    }

    /* Wait for the semaphore. */
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore (&(cb->hub_driver.irpCompleteLock),
                                      (NU_PLUS_Ticks_Per_Second * 5));
    }

    if (status == NU_SUCCESS)
    {
        /* Now the transfer is complete. */
        status = NU_USB_IRP_Get_Status ((NU_USB_IRP *) irp, &irp_status);
    }

    if (status == NU_SUCCESS)
    {
        status = irp_status;
    }

    int_status = NU_Release_Semaphore (&(hub->lock));

    return ((status == NU_SUCCESS) ? int_status : status);

}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usbh_hub_get_port_err_cnt_req
 *
 * DESCRIPTION
 *
 *     This hub class request returns the number of link errors detected
 *     by the hub on  the port indicated by wIndex.
 *
 * INPUTS
 *
 *      cb              Pointer to hub driver control block
 *      hub             Pointer to hub device.
 *      port_num        Port number for which host wants to get the link
 *                      error count.
 *      port_err_cnt    Pointer to UINT16 variable for holding number of
 *                      link state errors when function returns.
 *
 * OUTPUTS
 *
 *      NU_SUCCESS          Operation Completed Successfully.
 *      NU_USB_INVLD_ARG    Any of the input argument is invalid.
 *
 ************************************************************************/
STATUS usbh_hub_get_port_err_cnt_req (NU_USBH_STACK       *cb,
                                      NU_USBH_DEVICE_HUB  *hub,
                                      UINT8               port_num,
                                      UINT16              *port_err_cnt)
{
    STATUS           status       = NU_SUCCESS;
    STATUS           irp_status   = NU_SUCCESS;
    STATUS           int_status   = NU_SUCCESS;
    NU_USBH_CTRL_IRP *irp         = NU_NULL;
    UINT16           *temp_buffer = NU_NULL;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(hub);
    NU_USB_PTRCHK(port_err_cnt);

    irp = hub->ctrlIrp;

    status = NU_Obtain_Semaphore (&(hub->lock), NU_SUSPEND);

    /* Fill the control packet for GET_PORT_ERROR_COUNT request. */
    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_bRequest (irp,
                                            USBH_HUB_CMD_GET_PORT_ERR_CNT);
    }

    if(status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_wLength (irp, HOST_2_LE16(0x02));
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_bmRequestType (irp, 0x80);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_wValue (irp, 0x00);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_CTRL_IRP_Set_wIndex (irp, HOST_2_LE16(port_num));
    }

    if(status == NU_SUCCESS)
    {
        /* Allocate uncached memory buffer for data IO. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(UINT16),
                                     (VOID **) &temp_buffer);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Set_Data ((NU_USB_IRP *) irp,
                                      (UINT8 *) temp_buffer);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Set_Length ((NU_USB_IRP *) irp, sizeof(UINT16) );
    }

    if (status == NU_SUCCESS)
    {
        /* Submit the IRP. */
        status = NU_USB_PIPE_Submit_IRP (hub->default_pipe,
                                         (NU_USB_IRP *) irp);
    }

    /* Wait for the semaphore. */
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore (&(cb->hub_driver.irpCompleteLock),
                                      (NU_PLUS_Ticks_Per_Second * 5));
    }

    if (status == NU_SUCCESS)
    {
        /* Now the transfer is complete. */
        status = NU_USB_IRP_Get_Status ((NU_USB_IRP *) irp, &irp_status);
    }

    if (status == NU_SUCCESS)
    {
        *port_err_cnt = LE16_2_HOST( *temp_buffer );
        status = irp_status;
    }

    if(temp_buffer)
    {
        int_status = USB_Deallocate_Memory(temp_buffer);
    }

    int_status = NU_Release_Semaphore (&(hub->lock));

    return ((status == NU_SUCCESS) ? int_status : status);
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usbh_hub_port_set_timeout
 *
 * DESCRIPTION
 *
 *      This function is used to set U1 and U2 inactivity timeout values
 *      for the specified port.
 *
 * INPUTS
 *
 *      cb               Pointer to hub driver control block
 *      hub              Pointer to hub device.
 *      port_num         Port number for which downstream time value need
 *                       to be set.
 *      feature_selector Feature selector U1/U2 timeout.
 *      timeout_val      Time out value.
 *
 * OUTPUTS
 *
 *      NU_SUCCESS          Operation Completed Successfully.
 *      NU_USB_INVLD_ARG    Any of the input arguments is invalid.
 *
 ************************************************************************/
STATUS usbh_hub_port_set_timeout(NU_USBH_STACK       *cb,
                                 NU_USBH_DEVICE_HUB  *hub,
                                 UINT8               port_num,
                                 UINT8               feature_selector,
                                 UINT8               timeout_val)
{
    STATUS status = NU_SUCCESS;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(hub);


    status = usb_hub_set_port_feature(cb, hub, feature_selector, port_num,
                                       timeout_val);
    return (status);
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usbh_hub_port_rw_mask
 *
 * DESCRIPTION
 *
 *      This function is used to set remote wakeup mask for the specified
 *      port.
 *
 * INPUTS
 *
 *      cb                Pointer to hub driver control block
 *      hub               Pointer to hub device.
 *      port_num          Port number for which remote wake mask
 *                        need to be set.
 *      remote_wake_mask  Port remote wake mask encoding, for details
 *                        refer to table 10.14 of usb 3.0 spec.
 *
 * OUTPUTS
 *
 *      NU_SUCCESS          Operation Completed Successfully.
 *      NU_USB_INVLD_ARG    Any of the input argument is invalid.
 *
 ************************************************************************/
STATUS usbh_hub_port_rw_mask (NU_USBH_STACK       *cb,
                              NU_USBH_DEVICE_HUB  *hub,
                              UINT8               port_num,
                              UINT8               remote_wake_mask)
{
    STATUS status = NU_SUCCESS;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(hub);

    status = usb_hub_set_port_feature(cb, hub,
                                      USBH_HUB_FEATURE_PORT_RW_MASK,
                                      port_num,
                                      remote_wake_mask);
   return status;
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usbh_hub_port_set_link_state
 *
 * DESCRIPTION
 *
 *      This funtion transition the desired link to the state identified
 *      by ‘link_state’. There are a few constraints about the behavior
 *      of this request. Pleaser refer to section 10.14.2.10 of USB 3.0
 *      specifications.
 *
 * INPUTS
 *
 *      cb                Pointer to hub driver control block
 *      hub               Pointer to hub device.
 *      port_num          Port number associated with the link.
 *      link_state        Desired link state.
 *
 * OUTPUTS
 *
 *      NU_SUCCESS           Operation Completed Successfully.
 *      NU_USB_INVLD_ARG     Any of the input argument is invalid.
 *      NU_USB_NOT_SUPPORTED Transition is not supported according to
 *                           USB 3.0 specification.
 *      NU_USB_INVLD_DEVICE  Invalid device.
 ************************************************************************/
STATUS usbh_hub_port_set_link_state (NU_USBH_STACK       *cb,
                                     NU_USBH_DEVICE_HUB  *hub,
                                     UINT8               port_num,
                                     UINT8               link_state)
{
    STATUS          status = NU_SUCCESS;
    NU_USB_DEVICE   *dev   = NU_NULL;
    USBH_HUB_STATUS portStatus;
    UINT8           current_state = 0;      /*Represent current link state
                                             */

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(hub);

    dev = hub->device;

    if (dev == NU_NULL)
    {
        status =  NU_USB_INVLD_DEVICE;
    }

    if (status ==  NU_SUCCESS)
    {
        status = usb_hub_get_port_status(cb, hub, &portStatus, port_num);
    }

    if (status == NU_SUCCESS)
    {
        if (!(portStatus.status & USBH_HUB_BIT_C_PORT_ENABLE))
        {
            status = NU_USB_NOT_SUPPORTED;
        }
    }

    /* Get the link status bits from the wPortStatus register. */
    if (status == NU_SUCCESS)
    {
        current_state =
              (UINT8)((portStatus.status & USBH_HUB_LINK_STATE_MASK) >> 5);

        if((current_state == USB_LINK_STATE_SSDISABLE)
           ||(current_state == USB_LINK_STATE_RXDETECT)
           ||(current_state == USB_LINK_STATE_SSINACTIVE))
        {
            status = NU_USB_NOT_SUPPORTED;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* We need to check the current link state, before transitioning
         * it to the new state. For example if desired link state is U1
         * and cuurent link state is U2 then link transition is invalid
         * and device behaviour is undefined. For more constraints on link
         * transitions please refer to to section 10.14.2.10 of USB 3.0
         * specifications.
         */
        switch (link_state)
        {
            case USB_LINK_STATE_U0 :
            break;

            case USB_LINK_STATE_U1 :

                if (current_state != USB_LINK_STATE_U0)
                {
                    status = NU_USB_NOT_SUPPORTED;
                }

            break;

            case USB_LINK_STATE_U2 :
                if (current_state != USB_LINK_STATE_U0)
                {
                    status = NU_USB_NOT_SUPPORTED;
                }

            break;

            case USB_LINK_STATE_U3 :
            break;

            case USB_LINK_STATE_SSDISABLE:
            break;

            case USB_LINK_STATE_RXDETECT:
                if (current_state == USB_LINK_STATE_SSDISABLE)
                {
                    status = NU_USB_NOT_SUPPORTED;
                }

            break;

            default:
                status = NU_USB_NOT_SUPPORTED;
            break;

        }
    }

    /* Set the desired link state. */
    if (status == NU_SUCCESS)
    {
        status = usb_hub_set_port_feature(cb, hub,
                                          USBH_HUB_FEATURE_PORT_LINK_STATE,
                                          port_num,
                                          link_state);
    }

    return status;
}
/**********************************************************************
 *
 * FUNCTION
 *
 *      usbh_hub_port_suspend_link
 *
 * DESCRIPTION
 *
 *      This funtion transitions the desired link to suspend(U3) state.
 *
 * INPUTS
 *
 *      cb                Pointer to hub driver control block
 *      hub               Pointer to hub device.
 *      port_num          Port number associated with the link.
 *
 * OUTPUTS
 *
 *      NU_SUCCESS          Operation Completed Successfully.
 *      NU_USB_INVLD_ARG    Any of the input argument is invalid.
 *
 ************************************************************************/
STATUS usbh_hub_port_suspend_link(NU_USBH_STACK       *cb,
                                  NU_USBH_DEVICE_HUB  *hub,
                                  UINT8               port_num)
{
    STATUS status = NU_SUCCESS;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(hub);

    status = usbh_hub_port_set_link_state (cb, hub, port_num,
                                           USB_LINK_STATE_U3);

    return status;
}

/**********************************************************************
 *
 * FUNCTION
 *
 *      usbh_hub_port_resume_link
 *
 * DESCRIPTION
 *
 *      This funtion transitions the desired link to resume(U0) state.
 *
 * INPUTS
 *
 *      cb                Pointer to hub driver control block
 *      hub               Pointer to hub device.
 *      port_num          Port number associated with the link.
 *
 * OUTPUTS
 *
 *      NU_SUCCESS          Operation Completed Successfully.
 *      NU_USB_INVLD_ARG    Any of the input argument is invalid.
 *      NU_USB_DEVICE_NOT_RESPONDING Failed in getting desired response
 *                                   from the device.
 ************************************************************************/
STATUS usbh_hub_port_resume_link(NU_USBH_STACK       *cb,
                                 NU_USBH_DEVICE_HUB  *hub,
                                 UINT8               port_num)
{
    STATUS status = NU_SUCCESS;
    USBH_HUB_STATUS  portStatus;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(hub);

    /* Call the usbh_hub_port_set_link_state function with the link state
     * parameter set to USB_LINK_STATE_U0.
     */
    status =  usbh_hub_port_set_link_state (cb, hub, port_num,
                                            USB_LINK_STATE_U0);

    /* Since setting the link state to U0 causes the C_PORT_LINK_STATE bit
     * of wPortChange to set to 1.Therfore we need to acknowledge the
     * change by clearing it.
     */

    if (status == NU_SUCCESS)
    {
        status = usb_hub_get_port_status(cb, hub, &portStatus, port_num);

        if (status == NU_SUCCESS)
        {
            if (!(portStatus.change & USBH_HUB_BIT_C_PORT_LS))
            {
                status = NU_USB_DEVICE_NOT_RESPONDING;
            }
        }
    }

    if (status == NU_SUCCESS)
    {
        status = usb_hub_clear_port_feature(cb, hub,
                                       USBH_HUB_FEATURE_C_PORT_LINK_STATE,
                                       port_num,0x00);
    }

    return status;
}
#if 0
/**********************************************************************
 *
 * FUNCTION
 *
 *      usbh_hub_port_bh_reset
 *
 * DESCRIPTION
 *      This function initiates warm reset on the given port of a given
 *      hub.
 *
 * INPUTS
 *  'cb'        : pointer to stack control block
 *  'hub'       : Pointer to the hub driver control block
 *  'portNum'   : Port to be issued a RESET
 *  'speed'     : Pointer where speed of the detected device is stored
 *
 * OUTPUTS
 *
 *  NU_SUCCESS                   if port reset is successful
 *  NU_USB_DEVICE_NOT_RESPONDING if Reset could not be completed
 *  NU_USB_INVLD_ARG             Indicates some argument is invalid.
 ************************************************************************/
static STATUS usbh_hub_port_bh_reset (NU_USBH_STACK * cb,
                                      NU_USBH_DEVICE_HUB * hub,
                                      UINT8 portNum,
                                      UINT8 *speed)
{
    /* The algorithm followed is as follows :
     *
     * 1. A set_feature(port_reset) will be issued.
     * 2. we wait for specified duration.
     * 3. We check if port status is updated to be reset and port is
     *    enabled. If so, we read the speed of the connected device and
     *    return.
     * 4. If device is disconnected then error will be returned.
     * 5. If neither 3 or 4 happen, we increment the delay time and issue
     *    a reset again.
     * 6. All this is done in a loop.
     */

    UINT32 delay = USBH_HUB_RESET_MIN_TIME;
    UINT32 i = 0, j = 0;
    USBH_HUB_STATUS portStatus;
    STATUS status = NU_USB_DEVICE_NOT_RESPONDING;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(hub);
    NU_USB_PTRCHK(speed);

    for (i = 0; i < USBH_HUB_RESET_MAX_TRIES; i++)
    {
        /* Issue a port reset   */
        status |= usb_hub_set_port_feature(cb, hub,
                                           USBH_HUB_FEATURE_PORT_RESET_BH,
                                           portNum, 0x00);

        for (j = 0; j < 5; j++)
        {
            /* Allow the reset signalling   */
            HUB_SLEEP (delay);

            /* get the port status and check for the status of reset*/
            status |= usb_hub_get_port_status (cb, hub, &portStatus,
                                                         portNum);

            /* See if the device is disconnected    */
            if (!(portStatus.status & USBH_HUB_BIT_PORT_CONNECTION))
            {
                return (status);
            }

            if ((portStatus.change & USBH_HUB_BIT_C_PORT_RESET) &&
                (portStatus.status & USBH_HUB_BIT_PORT_ENABLE))
            {
                /* Reset signalling is complete */
                status |= usb_hub_clear_port_feature (cb, hub,
                                         USBH_HUB_FEATURE_C_PORT_RESET_BH,
                                         portNum, 0x00);

                /* Read the port speed. */

                if (!(portStatus.status & USBH_HUB_PORT_SS_MASK))
                {
                    *speed = USB_SPEED_SUPER;
                }

                /* As we have to return NU_SUCCESS in this case so set
                 * status as unused parameter to remove KW and
                 * PC-Lint warnings.
                 */
                NU_UNUSED_PARAM(status);
                return (NU_SUCCESS);
            }

            /* next trial, wait for long time. */
            delay = USBH_HUB_RESET_MAX_TIME;
        }
    }

    if (!USBH_IS_ROOT_HUB(hub->device))
    {
        status |=  usbh_hub_port_suspend_link(cb, hub, portNum);
    }

    return (status);
}
#endif
#endif     /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
/************************************************************************/

#endif /* USBH_DRVR_HUB_IMP_C */
/* ======================  End Of File  =============================== */
