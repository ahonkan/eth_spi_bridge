/**************************************************************************
*
*              Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*       nu_usbh_nuf_imp.c
*
* COMPONENT
*
*       Nucleus USB Host FILE Driver.
*
* DESCRIPTION
*
*       This file contains internal functions implementation for Nucleus
*       USB Host File Driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTION
*
*       NUF_USBH_Connect                    Serves a new disk connected.
*       NUF_USBH_Disconnect                 Cleans up when a disk is
*                                           disconnected.
*       NUF_USBH_Media_Init_Device          Initializes connected disk.
*       NUF_USBH_Media_Ready_Device         Makes the connected disk
*                                           ready.
*       NUF_USBH_Allocate_Device_Info       Allocate device structures.
*       NUF_USBH_Event_2_Drives             Delegate events to all logical
*                                           drives.
*       NUF_USBH_Media_Io                   Common IO for logical and
*                                           physical device.
*       NUF_USBH_Strncmp                    Compares two strings's, 'n' characters.
*       NU_USBH_NUF_Set_Dev_Name            Sets unique device name.
*       NUF_USBH_Init_Polling_Task           Initializes device polling task.
*       NUF_USBH_RM_Dev_Poll_Task           Task function that polls devices.
*       NUF_USBH_Init_File_Driver           Initialize File Driver.
*       NUF_USBH_UnInit_File_Driver         Uninitialize File Driver.
*       NUF_USBH_Get_Task_Status             Get tasks status.
*
*
* DEPENDENCIES
*
*       nu_usbh_nuf_imp.h                   USB File Driver Internal
*                                           definitions.
*
**************************************************************************/

/* ===================  Standard Include Files ========================= */

#include "drivers/nu_usbh_nuf_imp.h"

/**************************************************************************
*
* FUNCTION
*
*       NUF_USBH_Connect
*
* DESCRIPTION
*
*       This function is used to initialize the device at the
*       connection time. It sends various setup commands for proper device
*       initialization. Once device is initialized, it then finds logical
*       devices present in the connected device and mount each device on
*       a unique mount point.
*
* INPUTS
*
*       p_handle                            Handle for the device connected
*       subclass                            Subclass this file driver
*                                           serves.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_QUEUE_FULL                       Queue is full.
*       NU_USB_NOT_SUPPORTED                Such notifications are not
*                                           supported on this format.
*       NU_USB_NO_RESPONSE                  Indicates the device is not
*                                           ready.
*       NU_USB_INVLD_ARG                    Indicates that one of the
*                                           arguments is invalid.
*       NU_USB_MAX_EXCEEDED                 Indicates user is already
*                                           serving maximum devices it can
*                                           support.
*       NU_USB_TRANSFER_FAILED              Indicates subclass driver
*                                           returns error.
*       NU_USB_LOCK_UNAVAILABLE             Semaphore unavailable.
*
**************************************************************************/
STATUS NUF_USBH_Connect(VOID *p_handle, UINT8 subclass)
{

    /* Local variables. */
    STATUS                 status;
    NUF_USBH_DEVICE        *device_ptr = NU_NULL;
    DATA_ELEMENT           task_status = 0x00;

    NU_USB_PTRCHK(p_handle);

    do
    {
        /* Allocate physical device  information structure for a connected
         * device.
         */
        status = NUF_USBH_Allocate_Device_Info (p_handle,
                                                subclass,
                                                &device_ptr);
        if (status != NU_SUCCESS)
        {
            status = NU_USB_NOT_SUPPORTED;
            break;
        }

        /* Initialize connected Mass storage device by sending class
         * specific commands like inquiry, read capacity data and mode
         * sense.
         */
        status = NUF_USBH_Media_Init_Device(device_ptr, p_handle);

        if (status == NU_USB_DEVICE_MEDIA_NOT_PRESENT)
        {
            status = NU_SUCCESS;
            /* Skip connect event, if media not present. */
        }

        else if (status != NU_SUCCESS)
        {
            status = NU_USB_TRANSFER_FAILED;
            NUF_USBH_Deallocate_Device_Info(device_ptr);
            break;
        }

        /* Device initialization successful. */
        if ( status == NU_SUCCESS )
        {
            /* Save user pointer in handle. */
            status = NU_USBH_MS_Set_Usrptr(p_handle, device_ptr);

            if (status == NU_SUCCESS)
            {
                /* Placing the device on the device list. */
                status = NU_Obtain_Semaphore(&pUSBH_File_Drvr->dev_list_lock, NU_SUSPEND);

                if(status == NU_SUCCESS)
                {
                    NU_Place_On_List((CS_NODE**)&pUSBH_File_Drvr->head_device,
                                      (CS_NODE*)device_ptr);

                    pUSBH_File_Drvr->rm_dev_count++;

                    /* Resume RM device polling task. */
                    status = NUF_USBH_Get_Task_Status(&pUSBH_File_Drvr->poll_task.task, &task_status);

                    if ( (status == NU_SUCCESS)  &&  (task_status == NU_PURE_SUSPEND) )
                    {
                        status = NU_Resume_Task(&pUSBH_File_Drvr->poll_task.task);
                        NU_USB_ASSERT(status == NU_SUCCESS);
                    }
                    status = NU_Release_Semaphore(&pUSBH_File_Drvr->dev_list_lock);
                }
            }

            /* Inform FILE of newly connected device. */
            if (device_ptr->media_status == NUF_USBH_DEV_MEDIA_READY)
            {
                /* Set unique device name.*/
                status = NU_USBH_NUF_Set_Dev_Name(device_ptr);

                status = NUF_USBH_Event_2_Drives(p_handle,
                                                 NU_USBH_NUF_DRVR_CONNECT);
                if ( status != NU_SUCCESS )
                {
                    status = NU_Obtain_Semaphore(&pUSBH_File_Drvr->dev_list_lock, NU_SUSPEND);
                    /* Remove device from removable devices list. */
                    NU_Remove_From_List((CS_NODE**)&pUSBH_File_Drvr->head_device,
                                           (CS_NODE*)device_ptr);
                    /* Decrement RM device count. */
                    pUSBH_File_Drvr->rm_dev_count--;

                    NUF_USBH_Deallocate_Device_Info(device_ptr);
                    NU_USBH_MS_Set_Usrptr(p_handle, NU_NULL);

                    /* Release device list lock. */
                    NU_Release_Semaphore(&pUSBH_File_Drvr->dev_list_lock);
                    break;
                }
            }
        }

    }while (0);

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NUF_USBH_Disconnect
*
* DESCRIPTION
*
*       This function is called by mass storage driver's disconnect
*       call when a mass storage device is disconnected from the host.
*
* INPUTS
*
*       p_handle                            Handle for the device connected
*       subclass                            Subclass this file driver
*                                           serves.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USB_INTERNAL_ERROR               Indicates NU_Become_File_User
*                                           returns error.
*       NU_USB_NOT_SUPPORTED                Such notifications are not
*                                           supported on this format.
*       NU_USB_INVLD_ARG                    Indicates that one of the
*                                           arguments is invalid.
*       NU_USB_MS_TRANSPORT_ERROR           Indicates Command is not
*                                           completed successfully.
*       NU_USB_MS_TRANSPORT_FAILED          Indicates command failed by the
*                                           media.
*
**************************************************************************/
STATUS NUF_USBH_Disconnect(VOID *p_handle, UINT8 subclass)
{
    STATUS           status;
    NUF_USBH_DEVICE  *device_ptr = NU_NULL;

    NU_USB_PTRCHK(p_handle);

    /* Initialize status. */
    status = NU_SUCCESS;

    /* Get user pointer in handle. */
    device_ptr = (NUF_USBH_DEVICE *) NU_USBH_MS_Get_Usrptr(p_handle);

    /* Validate the control block. */
    if (device_ptr)
    {
        /* Obtain device list lock. */
        status = NU_Obtain_Semaphore(&pUSBH_File_Drvr->dev_list_lock, NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            if (device_ptr->media_status == NUF_USBH_DEV_MEDIA_READY)
            {
                 /* Send device disconnection event. */
                 device_ptr->media_status = NUF_USBH_DEV_MEDIA_NOT_READY;
                 status = NUF_USBH_Event_2_Drives(device_ptr->dev_handle,
                                                  NU_USBH_NUF_DRVR_DISCONNECT);
                 NU_USB_ASSERT(status == NU_SUCCESS);
            }

            /* Remove device from removable devices list. */
            NU_Remove_From_List((CS_NODE**)&pUSBH_File_Drvr->head_device,
                                 (CS_NODE*)device_ptr);
            /* Decrement RM device count. */
            pUSBH_File_Drvr->rm_dev_count--;

            /* Release device list lock. */
            status = NU_Release_Semaphore(&pUSBH_File_Drvr->dev_list_lock);
            NU_USB_ASSERT(status == NU_SUCCESS);

            /* Deallocate device memory. */
            status = NUF_USBH_Deallocate_Device_Info(device_ptr);
            NU_USB_ASSERT(status == NU_SUCCESS);
        }
    }

    /* Clear user pointer in handle. */
    status = NU_USBH_MS_Set_Usrptr(p_handle, NU_NULL);
    NU_USB_ASSERT(status == NU_SUCCESS);

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NUF_USBH_Media_Ready_Device
*
* DESCRIPTION
*
*       This function makes the device ready to respond.
*
* INPUTS
*
*       pcb_device                          Pointer to device information
*       p_handle                            Handle for the device connected
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USB_TRANSFER_FAILED              Indicates subclass driver
*                                           returns error.
*
**************************************************************************/
STATUS NUF_USBH_Media_Ready_Device(NUF_USBH_DEVICE *pcb_device,
                                  VOID            *p_handle)
{
    STATUS  status = -1;
    UINT8  *cmd_ptr;
    UINT8  *data_ptr;
    INT     retry;

    NU_USB_PTRCHK(pcb_device);
    NU_USB_PTRCHK(p_handle);

    /* Move command packet and command data buffer to local. */
    cmd_ptr  = pcb_device->dev_msc->cmd_pkt;
    data_ptr = pcb_device->dev_msc->cmd_data;

    do
    {
        if( (pcb_device->dev_type & NUF_USBH_DEV_CDROM) !=
                                                NUF_USBH_DEV_CDROM)
        {
            /* Call device read routine. */
            status = pcb_device->dev_msc->func.read (p_handle,
                                                     cmd_ptr,
                                                     0,
                                                     1,
                                                     data_ptr,
                                                     NUF_USBH_SIZE_CMDDATA );
            /* Error, MS device is disconnected. Can not proceed further. */
            if (status == NU_USBH_MS_TRANSPORT_ERROR)
            {
                break;
            }
        }

        for (retry = 0; (retry < NUF_USBH_RETRY_INIT_SHORT) &&
                                  (status != NU_SUCCESS); retry++)
        {
            /* Call device request sense routine. */
            status = pcb_device->dev_msc->func.request_sense (p_handle,
                                                              cmd_ptr, data_ptr,
                                                              NUF_USBH_LEN_REQSENSE);
            /* Error, MS device is disconnected. Can not proceed further. */
            if (status == NU_USBH_MS_TRANSPORT_ERROR)
            {
                break;
            }

            if (status == NU_SUCCESS)
            {
                /* Determine if medium present. */
                if ( ((data_ptr[2]  & 0x0F) == NUF_USBH_SK_NOTREADY) &&
                        ((data_ptr[12] & 0xFF) == NUF_USBH_ASC_MNOTPRST) )
                {
                    /* Medium not present. */
                    status = NU_USB_DEVICE_NOT_RESPONDING;
                    break;
                }
            }

            /* Delay for the next command. */
            NU_Sleep(NUF_USBH_TIME_INIT);

            if( (pcb_device->dev_type & NUF_USBH_DEV_CDROM) !=
                    NUF_USBH_DEV_CDROM)
            {
                /* Device is not ready. Retry read routine. */
                status = pcb_device->dev_msc->func.read (p_handle,
                                                         cmd_ptr,
                                                         0,
                                                         1,
                                                         data_ptr,
                                                         NUF_USBH_SIZE_CMDDATA );
                /* Error, MS device is disconnected. Can not proceed further. */
                if (status == NU_USBH_MS_TRANSPORT_ERROR)
                {
                    break;
                }
            }
        }

        /* Error, MS device is disconnected. Can not proceed further. */
        if (status == NU_USBH_MS_TRANSPORT_ERROR)
        {
            break;
        }

        if (status != NU_SUCCESS)
        {
            if( (pcb_device->dev_type & NUF_USBH_DEV_CDROM) ==
                                                            NUF_USBH_DEV_CDROM)
            {
                memset(cmd_ptr, 0, NUF_USBH_M_SENSE10_CMD_LEN);
                memset(data_ptr, 0, NUF_USBH_M_SENSE10_DATA_LEN);

                /* Setting the Mode Sense(10) Commands. */
                cmd_ptr[0] = NUF_USBH_M_SENSE10_CMD_OPCODE;
                cmd_ptr[2] = NUF_USBH_M_SENSE10_CMD_3F;   /* all pages */
                cmd_ptr[8] = NUF_USBH_M_SENSE10_DATA_LEN;

                /* Call device request routine for Mode sense(10) with 3F. */
                status = pcb_device->dev_msc->func.request (p_handle,
                                                            cmd_ptr,
                                                            NUF_USBH_M_SENSE10_CMD_LEN,
                                                            data_ptr,
                                                            NUF_USBH_M_SENSE10_DATA_LEN,
                                                            USB_DIR_IN);
                /* Error, MS device is disconnected. Can not proceed further. */
                if (status == NU_USBH_MS_TRANSPORT_ERROR)
                {
                    break;
                }
            }
            else
            {
                memset(cmd_ptr, 0, NUF_USBH_M_SENSE6_CMD_LEN);
                memset(data_ptr, 0, NUF_USBH_M_SENSE6_DATA_LEN);

                /* Setting the Mode Sense(6) Commands. */
                cmd_ptr[0] = NUF_USBH_M_SENSE6_CMD_OPCODE;
                cmd_ptr[2] = NUF_USBH_M_SENSE6_CMD_1C;
                cmd_ptr[4] = NUF_USBH_M_SENSE6_DATA_LEN;

                /* Call device request routine for Mode sense(6)
                 * with page 1C. */
                status = pcb_device->dev_msc->func.request (p_handle,
                                                            cmd_ptr,
                                                            NUF_USBH_M_SENSE6_CMD_LEN,
                                                            data_ptr,
                                                            NUF_USBH_M_SENSE6_DATA_LEN,
                                                            USB_DIR_IN);
                /* Error, MS device is disconnected. Can not proceed further. */
                if (status == NU_USBH_MS_TRANSPORT_ERROR)
                {
                    break;
                }

                memset(cmd_ptr, 0, NUF_USBH_M_SENSE6_CMD_LEN);
                memset(data_ptr, 0, NUF_USBH_M_SENSE6_DATA_LEN);

                /* Setting the Mode Sense(6) Commands. */
                cmd_ptr[0] = NUF_USBH_M_SENSE6_CMD_OPCODE;
                cmd_ptr[2] = NUF_USBH_M_SENSE6_CMD_3F;  /* all pages */
                cmd_ptr[4] = NUF_USBH_M_SENSE6_DATA_LEN;

                /* Call device request routine for Mode sense(6) with 3F. */
                status = pcb_device->dev_msc->func.request (p_handle,
                                                            cmd_ptr,
                                                            NUF_USBH_M_SENSE6_CMD_LEN,
                                                            data_ptr,
                                                            NUF_USBH_M_SENSE6_DATA_LEN,
                                                            USB_DIR_IN);
                /* Error, MS device is disconnected. Can not proceed further. */
                if (status == NU_USBH_MS_TRANSPORT_ERROR)
                {
                    break;
                }
            }
        }
    }while(0);

    return (status);

}

/**************************************************************************
*
* FUNCTION
*
*       NUF_USBH_Media_Init_Device
*
* DESCRIPTION
*
*       This function initializes the device. If device is ready, Gets
*       logical block address and block length in bytes.
*
* INPUTS
*
*       pcb_device                          Pointer to device information
*       p_handle                            Handle for the device connected
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion.
*       NU_USB_DEVICE_NOT_RESPONDING        Indicates device not responding.
*       NU_USB_TRANSFER_FAILED              Indicates subclass driver
*                                           returns error.
*        NU_USB_DEVICE_MEDIA_NOT_PRESENT    Indicates removable media
*                                           not present
*
**************************************************************************/
STATUS NUF_USBH_Media_Init_Device(NUF_USBH_DEVICE *pcb_device,
                                  VOID            *p_handle)
{
    STATUS  status;
    UINT8  *cmd_ptr;
    UINT8  *data_ptr;
    INT     retry, main_retry;

    NU_USB_PTRCHK(pcb_device);
    NU_USB_PTRCHK(p_handle);

    /* Move command packet and command data buffer to local. */
    cmd_ptr  = pcb_device->dev_msc->cmd_pkt;
    data_ptr = pcb_device->dev_msc->cmd_data;

    /* Call device get inquiry routine. */
    status = pcb_device->dev_msc->func.inquiry(
                                         p_handle, cmd_ptr, data_ptr,
                                         NUF_USBH_LEN_INQUIRY);
    /* Device inquiry. */
    for (retry = 0; (retry < NUF_USBH_RETRY_INIT) &&
                                (status != NU_SUCCESS); retry++)
    {
        /* Call device request sense routine. */
        status = pcb_device->dev_msc->func.request_sense(
                                                p_handle,
                                                cmd_ptr,
                                                data_ptr,
                                                NUF_USBH_LEN_REQSENSE);
        /* Error, device is disconnected. Can not proceed further. */
        if (status == NU_USBH_MS_TRANSPORT_ERROR)
        {
            break;
        }

        /* Delay for the next command. */
        NU_Sleep(NUF_USBH_TIME_INIT);

        /* Retry device get inquiry routine. */
        status = pcb_device->dev_msc->func.inquiry(
                                            p_handle,
                                            cmd_ptr,
                                            data_ptr,
                                            NUF_USBH_LEN_INQUIRY);
        /* Error, device is disconnected. Can not proceed further. */
        if (status == NU_USBH_MS_TRANSPORT_ERROR)
        {
            break;
        }

        /* Delay for the next command. */
        NU_Sleep(NUF_USBH_TIME_INIT);
    }

    /* Device inquiry successful. */
    if (status == NU_SUCCESS)
    {
        /* Set device medium to 0. */
        pcb_device->medium = 0;

        /* Save Peripheral Device Type and Removable Medium (RMB) bit.
         */
        pcb_device->dev_type = (data_ptr[0] & NUF_USBH_DEV_MASK) |
                      ((UINT16) data_ptr[1] << 8);
        /* Save Peripheral Device Type and Removable Medium (RMB) bit.
         */
        pcb_device->medium |= (data_ptr[1] & NUF_USBH_DEV_RMB);

        /* Call device unit ready routine. */
        status = pcb_device->dev_msc->func.unit_ready(p_handle, cmd_ptr);

        /* Unit Ready Command. */
        for (retry = 0; (retry < NUF_USBH_RETRY_INIT) &&
               (status != NU_SUCCESS); retry++)
        {
             /* Call device request sense routine. */
            status = pcb_device->dev_msc->func.request_sense(p_handle,
                                                      cmd_ptr, data_ptr,
                                                      NUF_USBH_LEN_REQSENSE);
            /* Error, device is disconnected. Can not proceed further. */
            if (status == NU_USBH_MS_TRANSPORT_ERROR)
            {
                break;
            }

            if (status == NU_SUCCESS)
            {
                /* Determine if medium present. */
                if (((data_ptr[2]  & 0x0F) == NUF_USBH_SK_NOTREADY) &&
                   ((data_ptr[12] & 0xFF) == NUF_USBH_ASC_MNOTPRST))
                {
                    /* Medium not present. */
                    status = NU_USB_DEVICE_MEDIA_NOT_PRESENT;
                    break;
                }
            }

            /* Delay for the next command. */
            NU_Sleep(NUF_USBH_TIME_INIT);

            /* Device is not ready. Retry unit ready routine. Can not proceed further. */
            status = pcb_device->dev_msc->func.unit_ready(p_handle, cmd_ptr);
            /* Error, device is disconnected. */
            if (status == NU_USBH_MS_TRANSPORT_ERROR)
            {
                break;
            }

            /* Delay for the next command. */
            NU_Sleep(NUF_USBH_TIME_INIT);
        }

        /* Read capacity. */
        if (status == NU_SUCCESS)
        {
            for (main_retry = 0; main_retry < NUF_USBH_RETRY_INIT
                                    ; main_retry++)
            {
                NU_Sleep(NUF_USBH_TIME_INIT);

                /* Call device read capacity routine. */
                status = pcb_device->dev_msc->func.read_capacity(p_handle,
                                                                 cmd_ptr,
                                                                 data_ptr);
                /* Error, device is disconnected. Can not proceed further. */
                if (status == NU_USBH_MS_TRANSPORT_ERROR)
                {
                    break;
                }

                /* Delay for the next command. */
                NU_Sleep(NUF_USBH_TIME_INIT);

                for (retry = 0; (retry < NUF_USBH_RETRY_INIT) &&
                                            (status != NU_SUCCESS); retry++)
                {
                    /* Call device request sense routine. */
                    status = pcb_device->dev_msc->func.request_sense(p_handle,
                                                        cmd_ptr, data_ptr,
                                                        NUF_USBH_LEN_REQSENSE);
                    /* Error, device is disconnected. Can not proceed further. */
                    if (status == NU_USBH_MS_TRANSPORT_ERROR)
                    {
                        break;
                    }

                    if (status == NU_SUCCESS)
                    {
                        /* Determine if medium present. */
                        if (((data_ptr[2]  & 0x0F) == NUF_USBH_SK_NOTREADY) &&
                            ((data_ptr[12] & 0xFF) == NUF_USBH_ASC_MNOTPRST))
                        {
                            /* Medium not present. */
                            status = NU_USB_DEVICE_NOT_RESPONDING;
                            break;
                        }
                    }

                    /* Delay for the next command. */
                    NU_Sleep(NUF_USBH_TIME_INIT);

                    /* Device is not ready. Retry read capacity routine. */
                    status = pcb_device->dev_msc->func.read_capacity(p_handle,
                                                                     cmd_ptr,
                                                                     data_ptr);
                    /* Error, device is disconnected. Can not proceed further. */
                    if (status == NU_USBH_MS_TRANSPORT_ERROR)
                    {
                        break;
                    }
                }

                /* Update LBA and block length. */
                if (status == NU_SUCCESS)
                {
                    /* Logical block address. */
                    pcb_device->last_lba = (UINT32) (data_ptr[3] |
                                  ((UINT32) data_ptr[2] << NUF_USBH_SHIFT_8) |
                                  ((UINT32) data_ptr[1] << NUF_USBH_SHIFT_16)|
                                  ((UINT32) data_ptr[0] << NUF_USBH_SHIFT_24));

                    /* Block length in bytes. */
                    pcb_device->block_len = (UINT32) (data_ptr[7] |
                                  ((UINT32) data_ptr[6] << NUF_USBH_SHIFT_8) |
                                  ((UINT32) data_ptr[5] << NUF_USBH_SHIFT_16)|
                                  ((UINT32) data_ptr[4] << NUF_USBH_SHIFT_24));
                    /* Set device status as ready. */
                    pcb_device->media_status = NUF_USBH_DEV_MEDIA_READY;
                    break;
                }
                /* ERROR, device disconnected. Can not proceed further. */
                else if(status == NU_USBH_MS_TRANSPORT_ERROR)
                {
                    break;
                }
                /* Ready the device. */
                else
                {
                    status = NUF_USBH_Media_Ready_Device(pcb_device, p_handle);

                    /* ERROR, device disconnected. Can not proceed further. */
                    if(status == NU_USBH_MS_TRANSPORT_ERROR)
                    {
                        break;
                    }
                }
            }
        }
    }
    else
    {
        /* Device inquiry failure. */
        status = NU_USB_TRANSFER_FAILED;
    }

    /* Initialize "Logical block address" and "Block length" in byte. */
    if (status != NU_SUCCESS)
    {
        pcb_device->last_lba   = 0UL;
        pcb_device->block_len  = 0UL;
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NUF_USBH_Allocate_Device_Info
*
* DESCRIPTION
*
*       This function allocates memory to the physical device structure
*       and fills in subclass information for this structure..
*
* INPUTS
*
*       p_handle                            Handle for the device connected
*       subclass                            Subclass this file driver.
*                                           serves.
*       ppcb_device                         Double pointer to device
*                                           Control Block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USB_INVLD_ARG                    Indicates that one of the
*                                           arguments is invalid.
*       ....
*
**************************************************************************/
STATUS NUF_USBH_Allocate_Device_Info(VOID             *p_handle,
                                     UINT8             subclass,
                                     NUF_USBH_DEVICE **ppcb_device)
{
    STATUS             status = NU_SUCCESS;
    NUF_USBH_DEVICE    *dev_ptr = NU_NULL;
    NUF_USBH_MSC       *msc_ptr;
    UINT8              rollback = 0;
    USBH_MSCD_FUNC     *dispatch_ptr = NU_NULL;

    NU_USB_PTRCHK(p_handle);
    NU_USB_PTRCHK(ppcb_device);
    do
    {
        /* Initialize handle, user pointer with NULL. */
        status = NU_USBH_MS_Set_Usrptr(p_handle, NU_NULL);
        if (status != NU_SUCCESS)
        {
            break;
        }

        /* Allocate memory to subclass information structure. */
        status = USB_Allocate_Object(sizeof(NUF_USBH_MSC),
                                     (VOID **) &msc_ptr);
        /* Memory allocation failure. */
        if (status != NU_SUCCESS)
        {
            break;
        }

        /* Initialize NUF_USBH_MSC structure. */
        memset(msc_ptr, 0, sizeof(NUF_USBH_MSC));

        /* Allocate a command packet buffer for device. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                     NUF_USBH_SIZE_CMDPKT,
                                     (VOID **) &msc_ptr->cmd_pkt);
        if (status != NU_SUCCESS)
        {
            rollback = 1;
            break;
        }
        /* Initialize command packet buffer. */
        memset(msc_ptr->cmd_pkt, 0, NUF_USBH_SIZE_CMDPKT);

        /* Allocate a command data buffer for device memory. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     NUF_USBH_SIZE_CMDDATA,
                                     (VOID **) &msc_ptr->cmd_data);

        if (status != NU_SUCCESS)
        {
            rollback = 2;
            break;
        }
        /* Initialize command data buffer. */
        memset(msc_ptr->cmd_data, 0, NUF_USBH_SIZE_CMDDATA);
        msc_ptr->subclass = subclass;
        status = NU_USBH_MS_USER_Get_SC_Dispatch(
                             (NU_USBH_MS_USER_DISPATCH **)&dispatch_ptr,
                              p_handle);

        if( status != NU_SUCCESS )
        {
            rollback = 3;
            break;
        }

        msc_ptr->func.request       = dispatch_ptr->request;
        msc_ptr->func.inquiry       = dispatch_ptr->inquiry;
        msc_ptr->func.unit_ready    = dispatch_ptr->unit_ready;
        msc_ptr->func.read_capacity = dispatch_ptr->read_capacity;
        msc_ptr->func.request_sense = dispatch_ptr->request_sense;
        msc_ptr->func.read          = dispatch_ptr->read;
        msc_ptr->func.write         = dispatch_ptr->write;

        status = USB_Allocate_Object(sizeof(NUF_USBH_DEVICE),
                                     (VOID **) &dev_ptr);

        if(status == NU_SUCCESS)
        {
            /* Initialize NUF_USBH_DEVICE structure. */
            memset(dev_ptr, 0, sizeof(NUF_USBH_DEVICE));

            /* Set handle to the device structure. */
            dev_ptr->dev_handle = p_handle;

            /* Set subclass information to device structure. */
            dev_ptr->dev_msc = msc_ptr;

            /* Set device status as not ready. */
            dev_ptr->media_status = NUF_USBH_DEV_MEDIA_NOT_READY;

            /* Create device RW semaphore. */
            status = NU_Create_Semaphore (&dev_ptr->rw_lock,
                                          "NUF_USBH",
                                          1,
                                          NU_FIFO);
            if (status != NU_SUCCESS)
            {
                rollback = 4;
                break;
            }
            /* Allocate uncached RW buffer. */
            /* Used to make sure uncached buffer is used for RW. */
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                         USB_NUF_RW_BUFF_SIZE,
                                         (VOID **)&dev_ptr->rw_buffer);
            if (status != NU_SUCCESS)
            {
                rollback = 5;
                break;
            }
        }
        else
        {
            rollback = 3;
            break;
        }

        *ppcb_device = dev_ptr;
    }while(0);

    switch( rollback )
    {
        case 5:  /* Delete device RW semaphore. */
            NU_Delete_Semaphore(&dev_ptr->rw_lock);
        case 4:            /* De-allocate a device cb. */
            USB_Deallocate_Memory (dev_ptr);
        case 3:            /* De-allocate a command data buffer. */
            USB_Deallocate_Memory (msc_ptr->cmd_data);
        case 2:            /* De-allocate a command packet buffer. */
            USB_Deallocate_Memory (msc_ptr->cmd_pkt);
        case 1:            /* De-allocate a NUF_USBH_MSC structure. */
            USB_Deallocate_Memory (msc_ptr);
        default:
            break;
    }
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NUF_USBH_Deallocate_Device_Info
*
* DESCRIPTION
*
*       This function deallocates memory to the physical device structure.
*
* INPUTS
*
*       dev_ptr         Pointer to device.
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates successful completion
*       ...
*
**************************************************************************/
STATUS NUF_USBH_Deallocate_Device_Info(NUF_USBH_DEVICE  *device_ptr)
{
    STATUS status;

    NU_USB_PTRCHK(device_ptr);

    NU_Obtain_Semaphore(&(device_ptr->rw_lock), NU_SUSPEND);
    
    /* Deallocate uncached RW buffer. */
    status = USB_Deallocate_Memory(device_ptr->rw_buffer);


    /* Deallocate command packet. */
    status |= USB_Deallocate_Memory(device_ptr->dev_msc->cmd_pkt);

    /* Deallocate packet data. */
    status |= USB_Deallocate_Memory(device_ptr->dev_msc->cmd_data);

    /* Deallocate subclass object. */
    status |= USB_Deallocate_Memory(device_ptr->dev_msc);
    device_ptr->dev_msc = NU_NULL;


    /* Delete RW semaphore. */
    status |= NU_Delete_Semaphore (&(device_ptr->rw_lock));

    /* Deallocate device object memory. */
    status |= USB_Deallocate_Memory (device_ptr);
    device_ptr = NU_NULL;

    return status;
}

/**************************************************************************
*
* FUNCTION
*
*       NUF_USBH_Event_2_Drives
*
* DESCRIPTION
*
*       This function is responsible to send connection/disconnection event
*       to the drives detected.
*
* INPUTS
*
*       p_handle                            Device handle.
*       event_code                          Device Connected/Disconnected.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*       NU_QUEUE_FULL                       Queue is full.
*       NU_USB_INVLD_ARG                    Invalid arguments.
*
**************************************************************************/
STATUS NUF_USBH_Event_2_Drives(VOID *p_handle, INT event_code)
{
    STATUS              status = NU_USB_INVLD_ARG;
    NUF_USBH_DEVICE    *device_ptr;

    NU_USB_PTRCHK(p_handle);

    /* Get user pointer in handle. */
    device_ptr = (NUF_USBH_DEVICE *) NU_USBH_MS_Get_Usrptr(p_handle);

    if (device_ptr)
    {
         if(event_code == NU_USBH_NUF_DRVR_CONNECT)
         {
             status = NU_USB_SYS_Register_Device(device_ptr,
                                   NU_USBCOMPH_FILE);

         }
         if(event_code == NU_USBH_NUF_DRVR_DISCONNECT)
         {
             status = NU_USB_SYS_DeRegister_Device(device_ptr,
                                   NU_USBCOMPH_FILE);
         }
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*        NU_USBH_NUF_Media_Io
*
* DESCRIPTION
*
*        Raw read/write function. This function sends a read or write
*        command to the device across USB.
*
* INPUTS
*
*        pcb_device                         Physical device structure.
*        sectors                            Sector number to read/write.
*        p_buffer                           Read/Write buffer.
*        count                              Sector count to be read or
*                                           written.
*        option                             Reading/Writing.
*                                           1: Read operation,
*                                           0: Write operation.
*
* OUTPUTS
*
*        NU_SUCCESS                         Indicates successful completion
*        NU_USB_MS_TRANSPORT_ERR            Indicates Command doesn't.
*                                           completed successfully.
*        NU_USB_MS_TRANSPORT_FAILED         Indicates command failed by the
*                                           media.
*
**************************************************************************/
STATUS NUF_USBH_Media_Io(NUF_USBH_DEVICE *pcb_device,
                         UINT32           sector,
                         VOID            *p_buffer,
                         UINT16           count,
                         INT              option)
{

    /* Local variables. */
    STATUS  status;
    UINT8  *cmd_ptr;

    NU_USB_PTRCHK(pcb_device);
    NU_USB_PTRCHK(p_buffer);

    /* Move command packet and command data buffer to local. */
    cmd_ptr = pcb_device->dev_msc->cmd_pkt;

    if (option)
    {
        /* Call device read routine. */
        status = pcb_device->dev_msc->func.read(
                                         pcb_device->dev_handle,
                                         cmd_ptr,
                                         sector, count, p_buffer,
                                         (pcb_device->block_len * count));
    }

    else
    {
        /* Call device write routine. */
        status = pcb_device->dev_msc->func.write(
                                         pcb_device->dev_handle,
                                         cmd_ptr,
                                         sector,
                                         count,
                                         p_buffer,
                                        (pcb_device->block_len * count));
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*        NU_USBH_NUF_Set_Dev_Name
*
*   DESCRIPTION
*
*       Assigns device unique name.
*
*   INPUTS
*
*       input_name          Pointer to device name (Out parameter).
*
*   OUTPUTS
*
*       NU_SUCCESS           Successful name assignment.
*       NU_USB_NAME_ERROR    Unable to set device name.
*       ...
*
*************************************************************************/
STATUS NU_USBH_NUF_Set_Dev_Name(NUF_USBH_DEVICE* pcb_input_device)
{
    CHAR   USB_Dev_Name[FPART_MAX_PHYS_NAME] = "UMx\0";
    CHAR   USB_Dev_index = 'A';

    NUF_USBH_DEVICE* pcb_curr_device = pUSBH_File_Drvr->head_device;
    
    for(;;)
    {
        if(pcb_curr_device->dev_name[2] == USB_Dev_index)
        {
            USB_Dev_index++;
            pcb_curr_device = (NUF_USBH_DEVICE*)pcb_curr_device->dev_link.cs_next ;
            if(pcb_curr_device == pUSBH_File_Drvr->head_device)
            {
                break;
            }
            else
            {
                pcb_curr_device = pUSBH_File_Drvr->head_device;
            }
        }
        else
        {
            if((pcb_curr_device->dev_name[2] == 0x00)&&(pcb_input_device == pcb_curr_device))
            {
                USB_Dev_Name[2] = USB_Dev_index;
                memcpy(&(pcb_curr_device->dev_name),
                       USB_Dev_Name,
                       FPART_MAX_PHYS_NAME);
                break;
            }
            pcb_curr_device = (NUF_USBH_DEVICE*)pcb_curr_device->dev_link.cs_next;
            if(pcb_curr_device == pUSBH_File_Drvr->head_device)
            {
                break;
            }
        }
    }
    return(NU_SUCCESS);
}
/**************************************************************************
* FUNCTION
*
*        NUF_USBH_Init_Polling_Task
*
* DESCRIPTION
*
*        This function initializes the task needed to poll MS devices
*        with Test Unit Command.
*
*
* INPUTS
*
*        None.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful completion of the service.
*        ...
*
**************************************************************************/

STATUS NUF_USBH_Init_Polling_Task()
{
    STATUS status = NU_SUCCESS;
    UINT   rollback = 0x00;

    do
    {
        /* Allocate memory for task's stack. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                     NUF_USBH_POLL_TASK_STACK_SIZE,
                                     (VOID **) &pUSBH_File_Drvr->poll_task.stack);
        if (status != NU_SUCCESS)
        {
             break;
        }

        /* Create Task. */
         status = NU_Create_Task(&pUSBH_File_Drvr->poll_task.task,
                                 "NUFTask",
                                 NUF_USBH_RM_Dev_Poll_Task,
                                 0,
                                 NU_NULL,
                                 pUSBH_File_Drvr->poll_task.stack,
                                 (UNSIGNED)NUF_USBH_POLL_TASK_STACK_SIZE,
                                 (OPTION) NUF_USBH_POLL_TASK_PRIORITY,
                                 (UNSIGNED) 0,
                                 NU_PREEMPT,
                                 (OPTION) NU_NO_START);
        if (status != NU_SUCCESS)
        {
            rollback = 0x01;
            break;
        }

    }while(0);

    switch(rollback)
    {
        case 0x01:
            status = USB_Deallocate_Memory(pUSBH_File_Drvr->poll_task.stack);
            NU_USB_ASSERT(status == NU_SUCCESS);
        default:
            break;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NUF_USBH_RM_Dev_Poll_Task
*
* DESCRIPTION
*
*        This function polls removable medium device(s) for media
*        insertion/removal. FILE is informed accordingly.
*        Removable devices are polled with Test Unit Ready command
*        to detect media insertion/removal.
*
* INPUTS
*
*        argc             Argument count, dummy variable, used
*                         only for convention.
*        param            Pointer to parameters.
*
* OUTPUTS
*
*        None.
*
**************************************************************************/
VOID NUF_USBH_RM_Dev_Poll_Task(UNSIGNED argc, VOID *param)
{
    NUF_USBH_DEVICE *dev_ptr          = NU_NULL;
    STATUS          status;
    UINT8           *cmd_ptr          = NU_NULL;
    UINT8           *data_ptr         = NU_NULL;
    UINT8           dev_poll_interval = NUF_USBH_DEVS_POLL_INTERVAL;

    /* Variable used for device pointer validation. */
    BOOLEAN         dev_ptr_valid;
    NUF_USBH_DEVICE *tmp_ptr;

    while(1)
    {
        /* Obtain device list semaphore. */
        status = NU_Obtain_Semaphore(&pUSBH_File_Drvr->dev_list_lock, NU_SUSPEND);
        if (status != NU_SUCCESS)
            continue;

        /* Check if no device present in the list. */
        if(pUSBH_File_Drvr->head_device == NU_NULL)
        {
            /* No device attached, suspend device polling task. */
            NU_Release_Semaphore(&pUSBH_File_Drvr->dev_list_lock);
            dev_ptr = NU_NULL;
            NU_Suspend_Task (&pUSBH_File_Drvr->poll_task.task);
            continue;
        }

        /* Validate device pointer. */
        tmp_ptr = (NUF_USBH_DEVICE *)pUSBH_File_Drvr->head_device;

        dev_ptr_valid = NU_FALSE;

        do
        {
            if (dev_ptr == tmp_ptr)
            {
                dev_ptr_valid = NU_TRUE;
                break;
            }
            tmp_ptr = (NUF_USBH_DEVICE *)tmp_ptr->dev_link.cs_next;

        }while(tmp_ptr != (NUF_USBH_DEVICE *)pUSBH_File_Drvr->head_device);

        /* Invalid device pointer. */
        if (!dev_ptr_valid || dev_ptr == NU_NULL)
        {
            /* Initialize device pointer with head device. */
            dev_ptr = (NUF_USBH_DEVICE *)pUSBH_File_Drvr->head_device;
        }

        NU_Release_Semaphore(&pUSBH_File_Drvr->dev_list_lock);

        /*check if the device is removalable */
        if(dev_ptr->medium != NUF_USBH_DEV_RMB)
        {
            /* if we reach here then device is not removable .move to next element in the list  */
            dev_ptr = (NUF_USBH_DEVICE *)dev_ptr->dev_link.cs_next;
            NU_Sleep(dev_poll_interval);
            continue;
        }

        /* Calculate inter device poll interval. */
        dev_poll_interval = NUF_USBH_DEVS_POLL_INTERVAL /
                                          pUSBH_File_Drvr->rm_dev_count;

        /* Obtain device's RW semaphore. */
        status = NU_Obtain_Semaphore(&dev_ptr->rw_lock, NU_NO_SUSPEND);
        if (status != NU_SUCCESS)
        {
            /* Move to next device in list. */
            dev_ptr = (NUF_USBH_DEVICE *)dev_ptr->dev_link.cs_next;
            NU_Sleep(dev_poll_interval);
            continue;
        }

        /* Test Unit Ready Command. */
        cmd_ptr  = dev_ptr->dev_msc->cmd_pkt;
        data_ptr = dev_ptr->dev_msc->cmd_data;
        status = dev_ptr->dev_msc->func.unit_ready(dev_ptr->dev_handle, cmd_ptr);

        /* Check for media presence. */
        if (status == NU_SUCCESS)
        {
            /* Media Inserted. */
            if (dev_ptr->media_status != NUF_USBH_DEV_MEDIA_READY)
            {
                /* Call device read capacity routine. */
                status = dev_ptr->dev_msc->func.read_capacity(dev_ptr->dev_handle,
                                                              cmd_ptr,
                                                              data_ptr);
                if (status == NU_SUCCESS)
                {
                    /* Logical block address. */
                    dev_ptr->last_lba = (UINT32) (data_ptr[3] |
                                  ((UINT32) data_ptr[2] << NUF_USBH_SHIFT_8) |
                                  ((UINT32) data_ptr[1] << NUF_USBH_SHIFT_16)|
                                  ((UINT32) data_ptr[0] << NUF_USBH_SHIFT_24));

                    /* Block length in bytes. */
                    dev_ptr->block_len = (UINT32) (data_ptr[7] |
                                  ((UINT32) data_ptr[6] << NUF_USBH_SHIFT_8) |
                                  ((UINT32) data_ptr[5] << NUF_USBH_SHIFT_16)|
                                  ((UINT32) data_ptr[4] << NUF_USBH_SHIFT_24));

                    /* Get unique available driver letter. */
                    status = NU_USBH_NUF_Set_Dev_Name(dev_ptr);

                    /* Inform FILE of new device. */
                    if (status == NU_SUCCESS)
                    {
                        /* Send device connection event. */
                        status = NUF_USBH_Event_2_Drives(dev_ptr->dev_handle,
                                                         NU_USBH_NUF_DRVR_CONNECT);
                        if (status == NU_SUCCESS)
                        {
                            /* Set device status as ready. */
                            dev_ptr->media_status = NUF_USBH_DEV_MEDIA_READY;
                        }
                    }

                }
            }
        }
        else
        {
            /* Media Removed. */
            if (dev_ptr->media_status == NUF_USBH_DEV_MEDIA_READY)
            {
                dev_ptr->media_status = NUF_USBH_DEV_MEDIA_NOT_READY;
                status = NUF_USBH_Event_2_Drives(dev_ptr->dev_handle,
                                          NU_USBH_NUF_DRVR_DISCONNECT);
            }
        }

        /* Release device RW semaphore. */
        NU_Release_Semaphore(&dev_ptr->rw_lock);

        /* Move to next device in list. */
        dev_ptr = (NUF_USBH_DEVICE *)dev_ptr->dev_link.cs_next;

        NU_Sleep(dev_poll_interval);
    }

}

/**************************************************************************
* FUNCTION
*
*        NUF_USBH_Aligned_Read_Uncached
*
* DESCRIPTION
*
*        This is a helper function called by 'NU_USBH_FILE_DM_Read()' to
*        write given sectors from device using un-cached buffers. It ensures
*        that write call use un-cached buffer for write operation. It creates
*        local un-cached buffer for data to be write and copies write data
*        to passed buffer. Multiple write calls are generated if local buffer
*        is smaller than passed buffer.
*
* INPUTS
*
*        dev_ptr          Pointer to device from which data is to be write.
*        offset_sector    Sector number to start writeing from.
*        size_sector      Total number of sectors to write,
*                         starting from offset_sector.
*        buffer           Supplied buffer for write data.
*
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful completion of the service.
*        ...
*
**************************************************************************/
STATUS NUF_USBH_Aligned_Read_Uncached(NUF_USBH_DEVICE *dev_ptr, UINT32 offset_sector, UINT32 size_sector, VOID* buffer)
{
    STATUS status = NU_SUCCESS;
    UINT32 remaining_sectors;
    UINT32 num_sectors;
    UINT8  *buff;
    UINT8  *rx_buffer;

    NU_USB_PTRCHK(dev_ptr);
    NU_USB_PTRCHK(buffer);

    buff = (UINT8*)buffer;
    rx_buffer = dev_ptr->rw_buffer;

    /* Remaining sectors to write. */
    remaining_sectors = size_sector;

    /* Number of sectors, write in each iteration. */
    num_sectors = USB_NUF_RW_BUFF_SIZE / dev_ptr->block_len;

    do
    {
        /* Read sectors in un-cached memory and copy to supplied buffer. */
        while(remaining_sectors >= num_sectors)
        {
            status = NUF_USBH_Media_Io (dev_ptr,
                                        offset_sector,
                                        (VOID*)rx_buffer,
                                        num_sectors,
                                        1);
            if (status != NU_SUCCESS)
            {
                break;
            }
            /* Copy write sectors to supplied buffer. */
            memcpy(buff, (VOID*)rx_buffer, (num_sectors * dev_ptr->block_len));

            /* Update buffer. */
            buff = (UINT8*) (&buff[(num_sectors * dev_ptr->block_len)]);

            /* Increment offset sector. */
            offset_sector += num_sectors;

            /* Decrement remaining sectors to write. */
            remaining_sectors -= num_sectors;
        }
        /* Error in writing. */
        if(status != NU_SUCCESS)
        {
            break;
        }

        /* Read remaining sector, if any. */
        if (remaining_sectors != 0)
        {
            status = NUF_USBH_Media_Io (dev_ptr,
                                        offset_sector,
                                        (VOID*)rx_buffer,
                                        remaining_sectors,
                                        1);
            if (status != NU_SUCCESS)
            {
                break;
            }
            /* Copy write data to user supplied buffer. */
            memcpy(buff,
                   (VOID*)rx_buffer,
                   remaining_sectors * dev_ptr->block_len);
        }
    }while(0);

    return (status);
}


/**************************************************************************
* FUNCTION
*
*        NUF_USBH_Aligned_Write_Uncached
*
* DESCRIPTION
*
*        This is a helper function called by 'NU_USBH_FILE_DM_Write()' to
*        write given buffer data to device sectors using un-cached buffers.
*        It ensures that write call use un-cached buffer for write operation.
*        It creates local un-cached buffer for data to be writeen and writes
*        them to device. Multiple write calls are required if local buffer
*        is smaller than passed buffer.
*
* INPUTS
*
*        dev_ptr          Pointer to device from which data is to be written.
*        offset_sector    Sector number to start writing to.
*        size_sector      Total number of sectors to write,
*                         starting from offset_sector.
*        buffer           Supplied buffer for write data.
*
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful completion of the service.
*        ...
*
**************************************************************************/
STATUS NUF_USBH_Aligned_Write_Uncached(NUF_USBH_DEVICE *dev_ptr, UINT32 offset_sector, UINT32 size_sector, VOID* buffer)
{
    STATUS status = NU_SUCCESS;
    UINT32 remaining_sectors;
    UINT32 num_sectors;
    UINT8  *buff;
    UINT8  *tx_buffer;

    NU_USB_PTRCHK(dev_ptr);
    NU_USB_PTRCHK(buffer);

    buff = (UINT8*)buffer;
    tx_buffer = dev_ptr->rw_buffer;

    /* Remaining sectors to write. */
    remaining_sectors = size_sector;

    /* Number of sectors to write in each iteration. */
    num_sectors = USB_NUF_RW_BUFF_SIZE / dev_ptr->block_len;

    do
    {
        /* Copy data in local un-cached buffer and write to device. */
        while(remaining_sectors >= num_sectors)
        {
            /* Copy data to local buffer. */
            memcpy((VOID *)tx_buffer, buff, (num_sectors * dev_ptr->block_len));
            status = NUF_USBH_Media_Io (dev_ptr,
                                        offset_sector,
                                        (VOID*)tx_buffer,
                                        num_sectors,
                                        0);
            if (status != NU_SUCCESS)
            {
                break;
            }
            /* Update buffer. */
            buff = (UINT8*) (&buff[(num_sectors * dev_ptr->block_len)]);

            /* Increment offset sector. */
            offset_sector += num_sectors;

            /* Decrement remaining sectors to write. */
            remaining_sectors -= num_sectors;
        }
        /* Error in writing. */
        if(status != NU_SUCCESS)
        {
            break;
        }

        /* Write remaining sectors, if any. */
        if (remaining_sectors != 0)
        {
            memcpy(tx_buffer, buff, remaining_sectors * dev_ptr->block_len);
            status = NUF_USBH_Media_Io (dev_ptr,
                                        offset_sector,
                                        (VOID*)tx_buffer,
                                        remaining_sectors,
                                        0);
            if (status != NU_SUCCESS)
            {
                break;
            }
        }
    }while(0);

    return (status);
}


/**************************************************************************
*
* FUNCTION
*
*       NUF_USBH_Init_File_Driver
*
* DESCRIPTION
*
*       Allocates memory for File Driver and initializes RM device list semaphore
*       and device(s) polling task.
*
* INPUTS
*
*       None.
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*       ...
*
**************************************************************************/
STATUS NUF_USBH_Init_File_Driver()
{
    STATUS status = NU_SUCCESS;
    UINT8 roll_back = 0x00;

    /* Return success, if File driver already initialized. */
    if (pUSBH_File_Drvr)
    {
        return status;
    }

    do
    {
        /* Allocate memory for FILE driver. */
        status = USB_Allocate_Object(sizeof(NUF_USBH_DRVR),
                                     (VOID **)&pUSBH_File_Drvr);
        if (status != NU_SUCCESS)
        {
             break;
        }

        /* Zero out the memory. */
        memset(pUSBH_File_Drvr, 0, sizeof(NUF_USBH_DRVR));

        /* Create device list semaphore. */
        status = NU_Create_Semaphore(&pUSBH_File_Drvr->dev_list_lock,
                                     "USBHSM1",
                                     1,
                                     NU_FIFO);
        if (status != NU_SUCCESS)
        {
            roll_back = 0x01;
            break;
        }

        /* Initialize device polling task. */
        status = NUF_USBH_Init_Polling_Task();
        if (status != NU_SUCCESS)
        {
            roll_back = 0x02;
        }

    }while(0);

    /* Cleanup in case of error. */
    switch(roll_back)
    {
        case 0x02:
            NU_Delete_Semaphore(&pUSBH_File_Drvr->dev_list_lock);
        case 0x01:
            NU_Deallocate_Memory( (VOID *) pUSBH_File_Drvr );
            pUSBH_File_Drvr = NU_NULL;
        default:
            break;
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NUF_USBH_UnInit_File_Driver
*
* DESCRIPTION
*
*       Deletes device list semaphore, device(s) polling task and
*       deallocate associated memory allocations.
*
* INPUTS
*
*       None.
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*       ...
*
**************************************************************************/
STATUS NUF_USBH_UnInit_File_Driver()
{
    STATUS status = NU_SUCCESS;

    do
    {
        /* Delete device list semaphore. */
        status = NU_Delete_Semaphore (&pUSBH_File_Drvr->dev_list_lock);

        /* Terminate task before deleting it. */
        status = NU_Terminate_Task(&pUSBH_File_Drvr->poll_task.task);

        /* Delete tasks stack. */
        status = NU_Deallocate_Memory(pUSBH_File_Drvr->poll_task.stack);

        /* Delete device polling task. */
        status = NU_Delete_Task (&pUSBH_File_Drvr->poll_task.task);

        /* De-allocate memory for USBH FILE driver. */
        NU_Deallocate_Memory( (VOID *) pUSBH_File_Drvr );
        pUSBH_File_Drvr = NU_NULL;

    }while(0);

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NUF_USBH_Get_Task_Status
*
* DESCRIPTION
*
*       Returns current status of supplied task.
*
* INPUTS
*
*       taskp                       Pointer to task, whose
*                                   status is required.
*       task_status                 Task status out variable.
* OUTPUTS
*
*       NU_SUCCESS                  Successful completion.
*       ...
*
**************************************************************************/
STATUS NUF_USBH_Get_Task_Status(NU_TASK *taskp, DATA_ELEMENT *task_status)
{
    STATUS        status;
    CHAR          task_name[8];
    UNSIGNED      scheduled_count;
    OPTION        priority;
    OPTION        preempt;
    UNSIGNED      time_slice;
    VOID          *stack_base;
    UNSIGNED      stack_size;
    UNSIGNED      minimum_stack;

    NU_USB_PTRCHK(taskp);
    NU_USB_PTRCHK(task_status);

    status = NU_Task_Information(
                 taskp,
                 &task_name[0],
                 task_status,
                 &scheduled_count,
                 &priority,
                 &preempt,
                 &time_slice,
                 &stack_base,
                 &stack_size,
                 &minimum_stack
                 );
    return status;
}
