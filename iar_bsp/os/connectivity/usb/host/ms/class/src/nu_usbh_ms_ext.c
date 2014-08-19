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
*       nu_usbh_ms_ext.c
*
*
* COMPONENT
*
*       Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains the external Interfaces exposed by Nucleus
*       USB Host Mass Storage Class Driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       nu_os_conn_usb_host_ms_user_init    Registry Initialization
*                                           function.
*       NU_USBH_MS_Create                   Creates Mass Storage Class
*                                           Driver.
*       NU_USBH_MS_Transport                Transports a mass storage
*                                           command to a mass storage
*                                           device.
*       _NU_USBH_MS_Delete                  Deletes a Mass Storage Class
*                                           Driver.
*       _NU_USBH_MS_Initialize_Intf         Connect callback function.
*       _NU_USBH_MS_Disconnect              Disconnect callback function.
*       NU_USBH_MS_Set_Usrptr               Sets user pointer of handle
*       NU_USBH_MS_Get_Usrptr               Gets user pointer of handle
*       NU_USBH_MS_Init_GetHandle           Getter function for class
*                                           driver handle.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
/*Defines for unit testing.*/
#ifndef NU_USBH_MS_EXT_C
#define NU_USBH_MS_EXT_C

/* ====================  USB Include Files  ============================ */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "services/runlevel_init.h"

/*********************************/
/* EXTERNAL VARIABLES            */
/*********************************/
extern NU_USBH_MS         *NU_USBH_MS_Cb_Pt;

/* ====================  Functions  ==================================== */
/**************************************************************************
*
* FUNCTION
*
*       nu_os_conn_usb_host_ms_class_init
*
* DESCRIPTION
*
*       Mass Storage Driver initialization routine.
*
* INPUTS
*
*       path                                Registry path of component.
*       compctrl                           Flag to find if component is
*                                           being enabled or disabled.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful initialization.
*       NU_USB_INVLD_ARG                    Indicates that parameters are
*                                           NU_NULL.
*       NU_INVALID_POOL                     Indicates the supplied pool
*                                           pointer is invalid.
*       NU_INVALID_SIZE                     Indicates the size is larger
*                                           than the pool.
*       NU_NO_MEMORY                        Memory not available.
*       NU_USBH_MS_DUPLICATE_INIT           Indicates initialization error
*
**************************************************************************/
STATUS nu_os_conn_usb_host_ms_class_init(CHAR *path, INT compctrl)
{
    STATUS  status = NU_SUCCESS, internal_sts = NU_SUCCESS;
    UINT8   rollback = 0;
    NU_USB_STACK * stack_handle;

    if (compctrl == RUNLEVEL_START)
    {
        /* Allocate memory for USB host MS class driver. */
        status = USB_Allocate_Object(sizeof(NU_USBH_MS),
                                     (VOID **)&NU_USBH_MS_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }
        
        if(!rollback) 
        {
            /* Zero out allocated block. */
            memset(NU_USBH_MS_Cb_Pt, 0, sizeof(NU_USBH_MS));

            /* Initialize MSC protocol driver.
             * In following API call, passing memory pool ptr parameter
             * NU_NULL because in ReadyStart memory in USB system is
             * allocated through USB specific memory APIs, not directly
             * with any given memory pool pointer. This parameter remains
             * only for backwards code compatibility. */

            status = NU_USBH_MS_Create(NU_USBH_MS_Cb_Pt, "MS", NU_NULL);
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }   
        
        /* Get the host stack handle */
        if(!rollback)
        {
            status = NU_USBH_Init_GetHandle ((VOID*)&stack_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        if(!rollback)
        {
            status = NU_USB_STACK_Register_Drvr ((NU_USB_STACK *) stack_handle,
                                             (NU_USB_DRVR *) NU_USBH_MS_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }
        if (status == NU_SUCCESS)
        {
            /* Create & Register user. */
            internal_sts = nu_os_conn_usb_host_ms_user_init(path, compctrl);
            NU_USB_ASSERT( internal_sts == NU_SUCCESS );
        }
        else
        {
            switch (rollback)
            {
                case 0x03:
                    internal_sts = _NU_USBH_MS_Delete((void*) NU_USBH_MS_Cb_Pt);
                    NU_USB_ASSERT( internal_sts == NU_SUCCESS );
                case 0x02:
                    internal_sts |= USB_Deallocate_Memory((void*) NU_USBH_MS_Cb_Pt);
                    NU_USB_ASSERT( internal_sts == NU_SUCCESS );
                case 0x01:
                case 0x00:
                    NU_UNUSED_PARAM(internal_sts);
                    break;
            }
        }
    }
    else if(compctrl == RUNLEVEL_STOP)
    {
        internal_sts = nu_os_conn_usb_host_ms_user_init(path, compctrl);
        NU_USB_ASSERT( internal_sts == NU_SUCCESS );

        status = NU_USBH_Init_GetHandle ((VOID*)&stack_handle);

        if (status == NU_SUCCESS)
        {
            _NU_USB_STACK_Deregister_Drvr ( stack_handle,
                                            (NU_USB_DRVR *)NU_USBH_MS_Cb_Pt);
        }

         _NU_USBH_MS_Delete((void*)NU_USBH_MS_Cb_Pt);
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_MS_Create.
*
* DESCRIPTION
*
*       Mass Storage Driver initialization routine.
*
* INPUTS
*       cb                                  Pointer to Mass Storage Driver
*                                           control block.
*       name                                Name of this USB object.
*       pool                                Pointer to Memory Pool used by
*                                           the driver.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful Initialization.
*       NU_USB_INVLD_ARG                    Indicates that the driver value
*                                           passed is NU_NULL or that the
*                                           drvr->match_flag contain
*                                           invalid values or that the
*                                           drvr->initialize_device
*                                           function pointer is with
*                                           USB_MATCH_VNDR_ID field
*                                           set in the drvr->match_flag.
*       NU_INVALID_SEMAPHORE                Indicates the semaphore pointer
*                                           is invalid.
*       NU_SEMAPHORE_DELETED                Semaphore was deleted while the
*                                           task was suspended.
*       NU_UNAVAILABLE                      Indicates the semaphore is
*                                           unavailable.
*       NU_INVALID_SUSPEND                  Indicates that this API is
*                                           called from a non-task thread.
*
**************************************************************************/
STATUS NU_USBH_MS_Create (NU_USBH_MS * cb,
                          CHAR * name,
                          NU_MEMORY_POOL * pool)
{
    STATUS status   = NU_SUCCESS , internal_sts = NU_SUCCESS;
    UINT8 roll_back = 0x00;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(name);
    NU_USB_MEMPOOLCHK_RETURN(pool);

    cb->ms_mem_pool = pool;

    do
    {
        /* 1: Create Driver */
        status = _NU_USBH_DRVR_Create ((NU_USB_DRVR *) cb, name,
                                      USB_MATCH_CLASS, 0, 0, 0, 0,
                                      USB_MS_CLASS_CODE, 0, 0,
                                      &usbh_ms_dispatch);
        if(status != NU_SUCCESS)
        {
          roll_back = 0x00;
          break;
        }

        /* 2: Create Semaphore for drive lock */
        status = NU_Create_Semaphore (&(cb->driver_lock),
                                        "MS_LOCK",
                                        1,
                                        NU_FIFO);
        if(status != NU_SUCCESS)
        {
          roll_back = 0x01;
          break;
        }

        /* 3: Allocate Memory pool for the Event Queue */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                     NU_USBH_MS_QUEUE_SIZE,
                                     &(cb->p_queue_mem));
        if(status != NU_SUCCESS)
        {
          roll_back = 0x02;
          break;
        }

        memset(cb->p_queue_mem, 0x00, NU_USBH_MS_QUEUE_SIZE);

        /* 4: Create Event Queue */
        status = NU_Create_Queue (&(cb->ms_queue),
                        "event_qu",
                                    cb->p_queue_mem,
                                    NU_USBH_MS_QUEUE_ELEMENTS,
                                    NU_FIXED_SIZE,
                                    NU_USBH_MS_MSG_SIZE,
                                    NU_PRIORITY);
        if(status != NU_SUCCESS)
        {
          roll_back = 0x03;
          break;
        }

        cb->connect_count = 0x00;
        cb->queue_av      = NU_USBH_MS_QUEUE_ELEMENTS;

        /* 5: Allocate Memory Pool for Task Stack */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                     MSC_EVENT_REPORTER_TASK_STACK,
                                     &cb->p_task_stack);
        if(status != NU_SUCCESS)
        {
            roll_back = 0x04;
            break;
        }

        /* 6: Create Event Reporter Task */
        status = NU_Create_Task (&(cb->disconn_task),
                                 "MS_EV_RP",
                                 UHMS_Event_Reporter,
                                 0,
                                 cb,
                                 cb->p_task_stack,
                                 MSC_EVENT_REPORTER_TASK_STACK,
                                 MSC_EVENT_REPORTER_TASK_PRIORITY,
                                 MSC_EVENT_REPORTER_TASK_TIMESLICE,
                                 MSC_EVENT_REPORTER_TASK_PREEMPTION,
                                 NU_START);
        if(status != NU_SUCCESS)
        {
            roll_back = 0x05;
            break;
        }

    } while(0);

    /* Cleanup in case something has gone wrong */
    switch(roll_back)
    {
      case 0x05:  
          internal_sts = USB_Deallocate_Memory (cb->p_task_stack);
          NU_USB_ASSERT( internal_sts == NU_SUCCESS );
          
      case 0x04:  
          internal_sts = NU_Delete_Queue      (&(cb->ms_queue));
          NU_USB_ASSERT( internal_sts == NU_SUCCESS );
          
      case 0x03:  
          internal_sts = USB_Deallocate_Memory (cb->p_queue_mem);
          NU_USB_ASSERT( internal_sts == NU_SUCCESS );
          
      case 0x02:  
          internal_sts = NU_Delete_Semaphore (&(cb->driver_lock));
          NU_USB_ASSERT( internal_sts == NU_SUCCESS );
          
      case 0x01:  
          internal_sts = _NU_USBH_DRVR_Delete ((VOID*)cb);
          NU_USB_ASSERT( internal_sts == NU_SUCCESS );
          break;
           
      default:
          break;
    }

    if(status == NU_SUCCESS)
    {
      NU_USBH_MS_Cb_Pt = cb;
    }

    NU_USER_MODE();

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       NU_USBH_MS_Transport.
*
* DESCRIPTION
*
*
*
* INPUTS
*
*       cb                                  Pointer to mass storage driver
*                                           control block.
*       user                                Pointer to user control block
*                                           calling this API.
*       session                             Pointer to handle of a LUN on
*                                           device, used to uniquely
*                                           identify the device.
*       command                             Command to be transmitted to
*                                           the device.
*       cmd_length                          Length of the command.
*       data_buffer                         Pointer to send/receive data
*                                           buffer.
*       data_length                         Length of data to be sent/
*                                           received.
*       direction                           Direction of data transfer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USB_MS_TRANSPORT_ERR             Indicates Command doesn't
*                                           completed successfully.
*       NU_USB_MS_TRANSPORT_FAILED          Indicates command failed by the
*                                           media.
*
**************************************************************************/
STATUS NU_USBH_MS_Transport (NU_USBH_MS * cb,
                             NU_USBH_USER * user,
                             VOID *session,
                             VOID *command,
                             UINT8 cmd_length,
                             VOID *data_buffer,
                             UINT32 data_length,
                             UINT8 direction)
{
    STATUS status = NU_USBH_MS_TRANSPORT_ERROR;
    STATUS internal_sts;
    NU_USBH_MS_DRIVE *currDrive = NU_NULL;
    UINT8 lun = 0x00;
    USBH_MS_CB cmd_blk;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(session);
    NU_USB_PTRCHK_RETURN(command);

    internal_sts = NU_SUCCESS;

    /* Remove unused parameter warning. */
    NU_UNUSED_PARAM(user);

    do
    {
        currDrive = UHMS_Validate_Drive (cb, session);
        if(currDrive == NU_NULL)
        {
            status = NU_INVALID_POINTER;
            break;
        }

        status = NU_Obtain_Semaphore (&(currDrive->drive_lock),
                              NU_SUSPEND);
        if(status != NU_SUCCESS)
        {
            break;
        }

        lun = ((MS_LUN *) session)->lun;

        /* Initialize the Command Block used by Internal Transport
         * Functions.
         */
        cmd_blk.command = command;
        cmd_blk.cmd_length = cmd_length;
        cmd_blk.data_buf = data_buffer;
        cmd_blk.buf_length = data_length;
        cmd_blk.direction = direction;

        /* Bulk Only Transport. */
        if (currDrive->protocol == MS_PR_BULK)
        {
            status = UHMS_Bulk_Transport (cb,
                                         currDrive->stack,
                                         currDrive,
                                         lun,
                                         &cmd_blk);
        }

        /* Control/Bulk Transport. */
        else if (currDrive->protocol == MS_PR_CB)
        {
            status = UHMS_CB_Transport (cb,
                                       currDrive->stack,
                                       currDrive,
                                       &cmd_blk);
        }

        /* Control/Bulk/Interrupt Transport. */
        else if (currDrive->protocol == MS_PR_CBI)
        {
            status = UHMS_CBI_Transport (cb,
                                        currDrive->stack,
                                        currDrive,
                                          &cmd_blk);
        }

        internal_sts = NU_Release_Semaphore(&(currDrive->drive_lock));

    } while(0);

    NU_USER_MODE();

    return (internal_sts == NU_SUCCESS ? status : internal_sts);
}

/**************************************************************************
* FUNCTION
*
*       NU_USBH_MS_Set_Data_Buff_Cachable.
*
* DESCRIPTION
*
*
*
* INPUTS
*
*       drive                               Pointer to mass storage drive.
*       data_buff_type                      Type of data buffer
*                                           NU_TRUE:    Cachable
*                                           NU_FALSE:   Non Cachable
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    An input argument is invalid.
*
**************************************************************************/
STATUS NU_USBH_MS_Set_Data_Buff_Cachable(NU_USBH_MS_DRIVE   *drive,
                                         BOOLEAN             data_buff_type)
{
    STATUS status;

    status = NU_USB_INVLD_ARG;
    if ( drive != NU_NULL )
    {
        drive->is_data_buff_cachable = data_buff_type;
        status = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBH_MS_Delete
*
* DESCRIPTION
*
*       This function deletes a Mass Storage driver. All Interfaces
*       claimed by this driver are given disconnect callback and the
*       interfaces are released. Driver is also deregistered from the
*       stack it was registered before deletion. Note that this function
*       does not free the memory associated with the Mass Storage Driver
*       control block.
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
**************************************************************************/
STATUS _NU_USBH_MS_Delete (VOID *cb)
{
    NU_USBH_MS *mscb;
    NU_USBH_MS_DRIVE *next, *drive;
    CHAR queue_name[8];
    VOID *start_address;
    UNSIGNED size;
    UNSIGNED available;
    UNSIGNED messages;
    OPTION message_type;
    UNSIGNED message_size;
    OPTION suspend_type;
    UNSIGNED tasks_suspended;
    NU_TASK *first_task;
    STATUS status;

    NU_USB_PTRCHK(cb);

    mscb = (NU_USBH_MS *) cb;
    drive = mscb->session_list_head;

    /* For each mass storage interface connected. */
    while (drive)
    {
        next = (NU_USBH_MS_DRIVE *) drive->node.cs_next;

        /* And send disconnect event to each. */
        status = _NU_USBH_MS_Disconnect ((NU_USB_DRVR *) mscb,
                                         drive->stack,
                                         drive->device);
        NU_USB_ASSERT( status == NU_SUCCESS );

        if ((next == mscb->session_list_head) ||
            (mscb->session_list_head == NU_NULL))
        {
            drive = NU_NULL;
        }
        else
        {
            drive = next;
        }
    }

    /* Cleanup */

    do
    {
        NU_Queue_Information(&mscb->ms_queue, queue_name, &start_address,
                                  &size, &available, &messages,
                                  &message_type, &message_size,
                                  &suspend_type, &tasks_suspended,
                                  &first_task);
        NU_Sleep(10);
    }while(messages != 0);

    status = NU_Delete_Semaphore(&mscb->driver_lock);
    NU_USB_ASSERT( status == NU_SUCCESS );

    status = NU_Delete_Queue(&mscb->ms_queue);
    NU_USB_ASSERT( status == NU_SUCCESS );

    status = NU_Terminate_Task(&mscb->disconn_task);
    NU_USB_ASSERT( status == NU_SUCCESS );

    status = NU_Delete_Task(&mscb->disconn_task);
    NU_USB_ASSERT( status == NU_SUCCESS );

    status = USB_Deallocate_Memory(mscb->p_queue_mem);
    NU_USB_ASSERT( status == NU_SUCCESS );

    status = USB_Deallocate_Memory(mscb->p_task_stack);
    NU_USB_ASSERT( status == NU_SUCCESS );

    /* Call Base Behavior. */
    status = _NU_USBH_DRVR_Delete (cb);
    NU_USB_ASSERT( status == NU_SUCCESS );

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBH_MS_Initialize_Intf
*
* DESCRIPTION
*
*       Connect Callback function invoked by stack when a Interface
*       with mass storage class is found on a device.
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
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*       NU_NOT_PRESENT                      Indicates No Alternate Setting
*                                           with supported protocols is
*                                           found. Indicates No user
*                                           associated with the subclass is
*                                           found. Indicates, Endpoints
*                                           required by the protocol are
*                                           not found.
*       NU_USB_INVLD_ARG                    Indicates some control block(s)
*                                           is(are) deleted before
*                                           completion.
*       NU_NO_MEMORY                        Indicates Memory Pool that the
*                                           DRIVE structure is full.
*       NU_USB_INVLD_DESC                   Indicates some descriptors of
*                                           Device are incorrect.
*       NU_USB_MAX_EXCEEDED                 Indicates Class Driver is
*                                           already serving as many devices
*                                           as it can support.
*       NU_INVALID_SUSPEND                  Indicates call is made from a
*                                           non thread context.
*
**************************************************************************/
STATUS _NU_USBH_MS_Initialize_Intf (NU_USB_DRVR * cb,
                                    NU_USB_STACK * stk,
                                    NU_USB_DEVICE * dev,
                                    NU_USB_INTF * intf)
{
    STATUS status, temp_sts;                /* New device connected.     */
    UINT8 protocol = 0x00;
    NU_USB_DEVICE *device = NU_NULL;
    NU_USBH_MS *mscb = NU_NULL;
    NU_USBH_MS_DRIVE *currDrive = NU_NULL;
    NU_USB_ALT_SETTG *alt_setting = NU_NULL;
    UINT8 alt_setting_num = 0x00;
    UINT8 sub_class = 0x00;
    UNSIGNED  event_message[NU_USBH_MS_MSG_SIZE];
    UINT8 roll_back = 0x00;

    status = NU_NOT_PRESENT;
    device = dev;
    mscb = (NU_USBH_MS *) cb;
    alt_setting_num = 0;
    event_message[0] = MSC_CONNECT_MESSAGE;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(stk);
    NU_USB_PTRCHK(dev);
    NU_USB_PTRCHK(intf);

    /* Search for suitable alternate setting of current interface. */
    for (alt_setting_num = 0 ; alt_setting_num < NU_USB_MAX_ALT_SETTINGS ; )
    {
        /* Find a alternate setting on the device with Matching Class
         * Code.
         */
        temp_sts = NU_USB_INTF_Find_Alt_Setting (intf, USB_MATCH_CLASS,
                                               alt_setting_num, USB_MS_CLASS_CODE,
                                               0, 0, &alt_setting);
        if (temp_sts == NU_SUCCESS)
        {
          /* Get the protocol associated with the Alternate Setting. */
          temp_sts = NU_USB_ALT_SETTG_Get_Protocol (alt_setting, &protocol);
          NU_USB_ASSERT( temp_sts == NU_SUCCESS );
          
          /* check the bInterfaceProtocol for attached interface. */
          if ( (protocol == MS_PR_BULK) ||
               (protocol == MS_PR_CB)   ||
               (protocol == MS_PR_CBI) )
          {
              /* A supportable Alternate setting is found on the device. */
              status = NU_SUCCESS;
              break;
          }
        }
        alt_setting_num++;
    }

    do
    {
        /* Protocol not matched any standard protocols. */
        if (status != NU_SUCCESS)
        {
            break;
        }

        /* Allocate a NU_USBH_MS_DRIVE structure for connected device.
        * This structure is allocated for each Interface of this class
        * connected to the host.
        */
        status = USB_Allocate_Object (sizeof(NU_USBH_MS_DRIVE),
                                      (VOID **) &currDrive);
        if (status != NU_SUCCESS)
        {
            roll_back = 0x00;
            break;
        }

        memset (currDrive, 0, sizeof(NU_USBH_MS_DRIVE));

        /* Allocate memory for Control IRP. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(NU_USBH_CTRL_IRP),
                                     (VOID **)&(currDrive->ctrl_irp));
        if (status != NU_SUCCESS)
        {
            roll_back = 0x01;
            break;
        }

        /* find a NU_USB_USER suitable for the device. We match
        * bInterfaceSubClass for it.
        */
        status = NU_USB_ALT_SETTG_Get_SubClass (alt_setting, &sub_class);
        if ( status != NU_SUCCESS )
        {
            roll_back = 0x02;
            break;
        }
        currDrive->subclass = sub_class;
        currDrive->user = UHMS_Find_User (mscb, sub_class);

        if (currDrive->user == NU_NULL)
        {
            /* Suitable User is not found */
            status = NU_NOT_PRESENT;
            roll_back = 0x02;
            break;
        }

        /* Find the default pipe. */
        status = NU_USB_ALT_SETTG_Find_Pipe (alt_setting,
                                             USB_MATCH_EP_ADDRESS,
                                             0, 0, 0,
                                             &currDrive->control_pipe);
        if (status != NU_SUCCESS)
        {
            roll_back = 0x02;
            break;
        }

        /* Find the bulk in pipe.*/
        status = NU_USB_ALT_SETTG_Find_Pipe (alt_setting,
                                             USB_MATCH_EP_TYPE |
                                             USB_MATCH_EP_DIRECTION,
                                             0, 0x80, 2,
                                             &currDrive->bulk_in_pipe);
        if (status != NU_SUCCESS)
        {
            roll_back = 0x02;
            break;
        }

        /* Find the bulk out pipe. */
        status = NU_USB_ALT_SETTG_Find_Pipe (alt_setting,
                                             USB_MATCH_EP_TYPE |
                                             USB_MATCH_EP_DIRECTION,
                                             0, 0, 2,
                                             &currDrive->bulk_out_pipe);
        if (status != NU_SUCCESS)
        {
            roll_back = 0x02;
            break;
        }

        /* If protocol is CBI an Interrupt Endpoint is needed */
        if (protocol == MS_PR_CBI)
        {
            /* Find the interrupt pipe */
            status = NU_USB_ALT_SETTG_Find_Pipe (alt_setting,
                                                 USB_MATCH_EP_TYPE,
                                                 0, 0, 3,
                                                 &currDrive->interrupt_pipe);
            if (status != NU_SUCCESS)
            {
              roll_back = 0x02;
              break;
            }
        }

        /* Set Our alternate setting as the
        * current alternate setting.
        */
        status = NU_USB_ALT_SETTG_Set_Active (alt_setting);
        if (status != NU_SUCCESS)
        {
            roll_back = 0x02;
            break;
        }

        /* Initialize the drive structure. */
        currDrive->protocol = protocol;
        currDrive->device = device;
        currDrive->intf = intf;

        /* Allocate memory for Bulk Only's CBW and CSW. */
        if(protocol == MS_PR_BULK)
        {
           status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                        sizeof(MS_BOT_CBW),
                                        (VOID **)&currDrive->cbw_ptr);
            if (status != NU_SUCCESS)
            {
                roll_back = 0x02;
                break;
            }
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                         sizeof(MS_BOT_CSW),
                                         (VOID **)&currDrive->csw_ptr);
            if (status != NU_SUCCESS)
            {
                roll_back = 0x02;
                break;
            }
        }

        /* Create Tx Request Semaphore */
        status = NU_Create_Semaphore (&(currDrive->tx_request),
                                      "MSTXREQ", 0, NU_FIFO);
        if (status != NU_SUCCESS)
        {
            roll_back = 0x02;
            break;
        }

        /* Create Drive Lock Semaphore */
        status = NU_Create_Semaphore (&(currDrive->drive_lock),
                                      "MSDRVLK", 1, NU_FIFO);
        if (status != NU_SUCCESS)
        {
            roll_back = 0x03;
            break;
        }

        status = UHMS_Bulk_Get_Max_LUN (mscb,
                                        stk,
                                        currDrive);
        if (status != NU_SUCCESS)
        {
            roll_back = 0x04;
            break;
        }

        /* Fill in other information for DRIVE. */
        currDrive->drvr = cb;
        currDrive->stack = stk;
        currDrive->alt_settg = alt_setting;

        /* Set data buffer type of device to cachable by default. */
        currDrive->is_data_buff_cachable = NU_TRUE;

        status = NU_Obtain_Semaphore (&mscb->driver_lock, NU_SUSPEND);
        if ( status != NU_SUCCESS )
        {
            roll_back = 0x04;
            break;
        }

        /* Place Drive Structure on the list */
        NU_Place_On_List ( (CS_NODE **) & mscb->session_list_head,
                          (CS_NODE *) currDrive);

        status = NU_Release_Semaphore (&mscb->driver_lock);
        if ( status != NU_SUCCESS )
        {
            roll_back = 0x05;
            break;
        }

        /* Claim Interface */
        status = NU_USB_INTF_Claim (intf, cb);
        if ( status != NU_SUCCESS )
        {
            roll_back = 0x05;
            break;
        }

        currDrive->device_state = NU_USBH_MS_CONNECTED;

        event_message[1] = (UNSIGNED)currDrive;

        status = NU_Obtain_Semaphore (&mscb->driver_lock,
                                      NU_SUSPEND);
        if ( status != NU_SUCCESS )
        {
            roll_back = 0x06;
            break;
        }

        /* If queue has space for more than 1 event */
        if((mscb->queue_av - mscb->connect_count) > 0x01)
        {
            /* Send the connection message */
            status = NU_Send_To_Queue(&mscb->ms_queue,
                                      event_message,
                                      NU_USBH_MS_MSG_SIZE,
                                      NU_NO_SUSPEND);
            if(status == NU_SUCCESS)
            {
                /* update the queue capacity status variables */
                mscb->queue_av -= 2;
            }
            else
            {
                roll_back = 0x06;
            }
        }
        else
        {
            status = NU_NO_MEMORY;
            roll_back = 0x06;
        }

        status = NU_Release_Semaphore (&mscb->driver_lock);
        NU_USB_ASSERT( status == NU_SUCCESS );
    } while(0);

    /* Cleanup in case something has gone wrong */
    switch(roll_back)
    {
        case 0x06:
            temp_sts = NU_USB_INTF_Release (intf, cb);
            NU_USB_ASSERT( temp_sts == NU_SUCCESS );

        case 0x05:
            temp_sts = NU_Obtain_Semaphore (&mscb->driver_lock, NU_SUSPEND);
            NU_USB_ASSERT( temp_sts == NU_SUCCESS );

            NU_Remove_From_List ((CS_NODE **) & mscb->session_list_head,
                                 (CS_NODE *) currDrive);

            temp_sts = NU_Release_Semaphore (&mscb->driver_lock);
            NU_USB_ASSERT( temp_sts == NU_SUCCESS );

        case 0x04:
            temp_sts = NU_Delete_Semaphore (&(currDrive->drive_lock));
            NU_USB_ASSERT( temp_sts == NU_SUCCESS );

        case 0x03:
            temp_sts = NU_Delete_Semaphore (&(currDrive->tx_request));
            NU_USB_ASSERT( temp_sts == NU_SUCCESS );

        case 0x02:
            temp_sts = USB_Deallocate_Memory (currDrive->ctrl_irp);
            NU_USB_ASSERT( temp_sts == NU_SUCCESS );
            if (currDrive->cbw_ptr)
            {
                temp_sts = USB_Deallocate_Memory(currDrive->cbw_ptr);
                NU_USB_ASSERT( temp_sts == NU_SUCCESS );
            }
            if (currDrive->csw_ptr)
            {
                temp_sts = USB_Deallocate_Memory(currDrive->csw_ptr);
                NU_USB_ASSERT( temp_sts == NU_SUCCESS );
            }

        case 0x01:
            temp_sts = USB_Deallocate_Memory (currDrive);
            NU_USB_ASSERT( temp_sts == NU_SUCCESS );
            break;

        default:
            break;
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBH_MS_Disconnect
*
* DESCRIPTION
*
*       Disconnect Callback function, invoked by stack when a Interface
*       with Mass Storage Class is removed from the BUS.
*
* INPUTS
*
*       cb                                  Pointer to Class Driver Control
*                                           Block claimed this interface.
*       stk                                 Pointer to Stack Control Block.
*       dev                                 Pointer to NU_USB_DEVICE
*                                           Control Block disconnected.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*
**************************************************************************/
STATUS _NU_USBH_MS_Disconnect (NU_USB_DRVR * cb,
                               NU_USB_STACK * stack,
                               NU_USB_DEVICE * device)
{
    NU_USBH_MS *mscb;
    NU_USBH_MS_DRIVE *next = NU_NULL,
                     *drive = NU_NULL,
                     *found = NU_NULL;
    STATUS status = NU_NOT_PRESENT;
    STATUS internal_sts = NU_NOT_PRESENT;
    UNSIGNED  event_message[NU_USBH_MS_MSG_SIZE];

    /* Remove unused parameter warning. */
    NU_UNUSED_PARAM(stack);

    mscb = (NU_USBH_MS *) cb;
    drive = mscb->session_list_head;
    event_message[0] = MSC_DISCONNECT_MESSAGE;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(stack);
    NU_USB_PTRCHK(device);

    status = NU_Obtain_Semaphore (&mscb->driver_lock, NU_SUSPEND);
    if ( status == NU_SUCCESS )
    {
        /* Scan the List of DRIVES and Cleanup all associated ones. */
        while (drive)
        {
            next = (NU_USBH_MS_DRIVE *) drive->node.cs_next;

            /* If this Drive is Associated with the device disconnected. */
            if ( (drive->device == device) &&
                 (drive->device_state == NU_USBH_MS_CONNECTED) )
            {
                found =  drive;
            }

            if ((next == mscb->session_list_head) ||
                (mscb->session_list_head == NU_NULL))
            {
               drive = NU_NULL;
               break;
            }
            else
            {
                drive = next;
            }
        }

        if(found)
        {
            found->device_state = NU_USBH_MS_DISCONNECTED;

            status = NU_USB_STACK_Flush_Pipe (stack, found->bulk_in_pipe);
            NU_USB_ASSERT( status == NU_SUCCESS );

            status = NU_USB_STACK_Flush_Pipe (stack, found->bulk_out_pipe);
            NU_USB_ASSERT( status == NU_SUCCESS );

            status = NU_USB_STACK_Flush_Pipe (stack, found->control_pipe);
            NU_USB_ASSERT( status == NU_SUCCESS );

            if(found->interrupt_pipe)
            {
              status = NU_USB_STACK_Flush_Pipe (stack, found->interrupt_pipe);
              NU_USB_ASSERT( status == NU_SUCCESS );
            }

            event_message[1] = (UNSIGNED)found;
            status = NU_Send_To_Queue(&mscb->ms_queue,
                                      event_message,
                                      NU_USBH_MS_MSG_SIZE,
                                      NU_NO_SUSPEND);
        }
        else
        {
          status = NU_NOT_PRESENT;
        }


        internal_sts = NU_Release_Semaphore (&mscb->driver_lock);
    }
    /* Drive not found is not possible since this is only called by stack
     * so we return success always.
     */
    return (internal_sts == NU_SUCCESS ? status : internal_sts);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_MS_Set_Usrptr
*
* DESCRIPTION
*
*       Set user pointer of the handle.
*
* INPUTS
*
*       handle                              Handle.
*       user_pointer                        User's pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*       NU_USB_INVLD_ARG                    Invalid parameter.
*
**************************************************************************/
STATUS  NU_USBH_MS_Set_Usrptr(VOID *handle, VOID *user_pointer)
{
    STATUS  status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(handle);
    ((MS_LUN *)handle)->user_pointer = user_pointer;
    status = NU_SUCCESS;

    /* Return to user mode.      */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_MS_Get_Usrptr
*
* DESCRIPTION
*
*       Get user pointer of the handle
*
* INPUTS
*
*       handle                              Handle.
*
* OUTPUTS
*
*       User's pointer.
*
**************************************************************************/
VOID    *NU_USBH_MS_Get_Usrptr(VOID *handle)
{
    VOID* userptr;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    if ( handle == (VOID *) 0 )
    {
        /* Return to user mode.     */
        NU_USER_MODE();
        return ((VOID *)0);
    }
    else
    {
        userptr = (((MS_LUN *)handle)->user_pointer);

        /* Return to user mode.     */
        NU_USER_MODE();
        return (userptr);
    }
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_MS_Init_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the host mass storage
*       class driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the class
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicate there exists a host mass storage
*                           class driver.
*       NU_NOT_PRESENT      Indicate that driver has not been initialized.
*
*************************************************************************/
STATUS NU_USBH_MS_Init_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(handle);

    status = NU_SUCCESS;
    *handle = NU_USBH_MS_Cb_Pt;
    if (NU_USBH_MS_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*       NU_USBH_MS_Suspend_Device.
*
* DESCRIPTION
*
*       Suspends a connected storage device.
*
* INPUTS
*
*       session                             Pointer to handle of a LUN on
*                                           device, used to uniquely
*                                           identify the device.
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*
**************************************************************************/
STATUS NU_USBH_MS_Suspend_Device (VOID *session)
{
    STATUS internal_status, status = NU_USBH_MS_TRANSPORT_ERROR;
    NU_USBH_MS_DRIVE *currDrive = NU_NULL;

    NU_USB_PTRCHK(session);

    do
    {
        currDrive = UHMS_Validate_Drive (NU_USBH_MS_Cb_Pt,
                                         session);
        if(currDrive == NU_NULL)
        {
            status = NU_INVALID_POINTER;
            break;
        }

        status = NU_Obtain_Semaphore (&(currDrive->drive_lock),
                                      NU_SUSPEND);
        if(status != NU_SUCCESS)
        {
            break;
        }

        status = NU_USBH_STACK_Suspend_Device( (NU_USBH_STACK*)currDrive->stack,
                                         currDrive->device);

        internal_status = NU_Release_Semaphore(&(currDrive->drive_lock));

    } while(0);

    return (status != NU_SUCCESS ? status : internal_status);;
}

/**************************************************************************
* FUNCTION
*
*       NU_USBH_MS_Resume_Device.
*
* DESCRIPTION
*
*       Resume a suspended storage device.
*
* INPUTS
*
*       session                             Pointer to handle of a LUN on
*                                           device, used to uniquely
*                                           identify the device.
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*
**************************************************************************/
STATUS NU_USBH_MS_Resume_Device (VOID *session)
{
    STATUS internal_status, status = NU_USBH_MS_TRANSPORT_ERROR;
    NU_USBH_MS_DRIVE *currDrive = NU_NULL;

    NU_USB_PTRCHK(session);

    do
    {
        currDrive = UHMS_Validate_Drive (NU_USBH_MS_Cb_Pt,
                                         session);
        if(currDrive == NU_NULL)
        {
            status = NU_INVALID_POINTER;
            break;
        }

        status = NU_Obtain_Semaphore (&(currDrive->drive_lock),
                                      NU_SUSPEND);
        if(status != NU_SUCCESS)
        {
            break;
        }

        status = NU_USBH_STACK_Resume_Device( (NU_USBH_STACK*)currDrive->stack,
                                         currDrive->device);

        internal_status = NU_Release_Semaphore(&(currDrive->drive_lock));

    } while(0);

    return (status != NU_SUCCESS ? status : internal_status);;
}

#endif
/* ====================  End Of File  ================================== */
