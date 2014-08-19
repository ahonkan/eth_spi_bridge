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
*       nu_usbh_nuf_ext.c
*
* COMPONENT
*
*       Nucleus USB Host FILE Driver.
*
* DESCRIPTION
*
*       This file contains external interfaces for Nucleus USB Host
*       File Driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       nu_os_conn_usbh_ms_file_init
*       NU_USBH_MS_Create_Drv               Creates a new File Driver.
*       NU_USBH_MS_Delete_Drv               Deletes an existing file
*                                           driver.
*       NU_USBH_FILE_DM_Open
*       NU_USBH_FILE_DM_Close
*       NU_USBH_FILE_DM_Read
*       NU_USBH_FILE_DM_Write
*       NU_USBH_FILE_DM_IOCTL
*
* DEPENDENCIES
*
*       nu_usbh_nuf_imp.h                   USB File Driver definitions.
*
**************************************************************************/

/* ====================  USB Include Files  ============================ */

#include "drivers/nu_usbh_nuf_imp.h"

/* ====================  Global Data  ================================== */
/*********************************/
/* EXTERNAL VARIABLES            */
/*********************************/

CHAR   USBH_NUF_reg_path[REG_MAX_KEY_LENGTH];

/* ====================  Functions  ==================================== */
/**************************************************************************
*
* FUNCTION
*
*       nu_os_conn_usbh_ms_file_init
*
* DESCRIPTION
*
*       Mass Storage Driver initialization routine.
*
* INPUTS
*
*       path                                Registry path of component.
*       startstop                           Flag to find if component is
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
STATUS nu_os_conn_usbh_ms_file_init(const CHAR *path, INT startstop)
{
    STATUS status;

    if(startstop)
    {
        strcpy(USBH_NUF_reg_path, path);
        status = NU_USBH_NUF_Create_Drv();
    }
    else
    {
        status = NU_SUCCESS;
    }

    return (status);
}


/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_MS_Create_Drv
*
* DESCRIPTION
*
*       This function initializes the file driver of Nucleus FILE.
*
* INPUTS
*
*       None.
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*       NU_INVALID_POOL                     Indicates the supplied pool
*                                           pointer is invalid.
*       NU_INVALID_SIZE                     Indicates the size is larger
*                                           than the pool.
*       NU_NO_MEMORY                        Memory not available.
*
**************************************************************************/
STATUS NU_USBH_NUF_Create_Drv()
{
    STATUS   status = NUF_DRVR_PRESENT;
    status = NU_USBH_MS_USER_SET_App_Callbacks((void*)&app_callbacks);
    if (status == NU_SUCCESS)
    {
        /* Initialize file driver. */
        status = NUF_USBH_Init_File_Driver();
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_MS_Delete_Drv
*
* DESCRIPTION
*
*       This function deletes a file driver. This is called by every MSC
*       user driver at delete time.
*
* INPUTS
*       None.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*
**************************************************************************/
STATUS NU_USBH_NUF_Delete_Drv()
{
    STATUS status;

    status = NUF_USBH_UnInit_File_Driver();
    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_FILE_DM_Open
*
* DESCRIPTION
*
*       This function is called by the application when it wants to open a device
*       for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the net device passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBH_FILE_DM_Open (VOID* dev_handle)
{
    return (NU_SUCCESS);
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_FILE_DM_Close
*
* DESCRIPTION
*
*       This function is called by the application when it wants to close a device
*       which it has opend already for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the net device passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBH_FILE_DM_Close (VOID* dev_handle)
{
    return (NU_SUCCESS);
}
/*************************************************************************
*
*   FUNCTION
*
*       NU_USBH_FILE_DM_Read
*
*   DESCRIPTION
*
*       This function reads from usb ms device.
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Read buffer
*       UINT32       numbyte                - Number of bytes to read
*       OFFSET_T     byte_offset            - Byte offset
*       UINT32       *bytes_read            - Number of bytes read
*
*   OUTPUTS
*
*       STATUS       status                 - NU_SUCCESS
*
*************************************************************************/
STATUS NU_USBH_FILE_DM_Read (VOID *session_handle,
                            VOID *buffer,
                            UINT32 numbyte,
                            OFFSET_T byte_offset,
                            UINT32 *bytes_read_ptr)
{
    NUF_USBH_DEVICE        *device_ptr;
    UINT32 offset_sector = 0;
    UINT32 offset_residue= 0;
    UINT32 size_residue = 0;
    UINT32 remaining_len = 0;
    UINT32 size_sector = 0;
    UINT8 *uncached_buffer;
    UINT8 *buff;
    STATUS status = NU_SUCCESS;

    NU_USB_PTRCHK(session_handle);
    NU_USB_PTRCHK(buffer);
    NU_USB_PTRCHK(bytes_read_ptr);

    device_ptr = (NUF_USBH_DEVICE*)session_handle;

    offset_sector = byte_offset / device_ptr->block_len;
    offset_residue = byte_offset % device_ptr->block_len;
    size_sector = numbyte / device_ptr->block_len;
    size_residue = numbyte % device_ptr->block_len;
    remaining_len = numbyte;
    *bytes_read_ptr = 0;
    buff = (UINT8*)buffer;

    /* Uncached RW buffer. */
    uncached_buffer = device_ptr->rw_buffer;

    do
    {
        /* Acquire device RW lock. */
        status = NU_Obtain_Semaphore(&device_ptr->rw_lock, NU_SUSPEND);
        if (status != NU_SUCCESS)
        {
            break;
        }
        if((size_residue !=0 )||(offset_residue != 0))
        {
            if(offset_residue != 0)
            {
            /*if offset is not on sector boundary then we read the full block
             *and copy only part of data to be sent back in incoming buffer */
                status = NUF_USBH_Media_Io (device_ptr,
                                               offset_sector,
                                               uncached_buffer,
                                               1,
                                               1);
                if(status !=NU_SUCCESS)
                {
                    break;
                }
                memcpy(buff,
                       (void*)&(((UINT8*)uncached_buffer)[offset_residue]),
                       ((device_ptr->block_len)-offset_residue));
                buff = (UINT8*)(&buff[(device_ptr->block_len)-offset_residue]);

                numbyte = numbyte - ((device_ptr->block_len)-offset_residue);
                offset_residue = 0;
                *bytes_read_ptr = ((device_ptr->block_len)-offset_residue);
                /*re calculating the side reside to ensure that when we reach at the end
                 *of read operation the size residue is based on the first less then 1 block read.*/
                offset_sector++;
                size_sector = numbyte / device_ptr->block_len;
                size_residue = numbyte % device_ptr->block_len;
            }

            /* Copying the blocks in between the start and end. */
            status = NUF_USBH_Aligned_Read_Uncached(device_ptr, offset_sector, size_sector, buff);
            if(status !=NU_SUCCESS)
            {
                break;
            }
            /*updating the number of byte read and buffer offset.*/
            offset_sector = offset_sector + size_sector;
            numbyte = numbyte -size_sector * (device_ptr->block_len);
            buff = (UINT8*)(&buff[(size_sector * (device_ptr->block_len))]);
            *bytes_read_ptr = *bytes_read_ptr + size_sector * (device_ptr->block_len);
            if(size_residue != 0)
            {
                /*if the size residue is not zero so well need to copy the last sector in a local buffer
                 * and the copy only number of bytes actually required.*/
                status = NUF_USBH_Media_Io (device_ptr,
                                        offset_sector,
                                        uncached_buffer,
                                        1,
                                        1);
                if(status !=NU_SUCCESS)
                {
                    break;
                }
                memcpy(buff,((VOID*)&uncached_buffer),size_residue);
                *bytes_read_ptr = *bytes_read_ptr + size_residue;
            }
        }
        else
        {
            /* If everything is sector aligned. */
            status = NUF_USBH_Aligned_Read_Uncached(device_ptr, offset_sector, size_sector, buff);
            if(status !=NU_SUCCESS)
            {
                break;
            }
            *bytes_read_ptr = remaining_len;

        }
    }
    while(0);

    /* Release device read/write lock. */
    NU_Release_Semaphore(&device_ptr->rw_lock);

    return (status);
}
/*************************************************************************
*
*   FUNCTION
*
*       NU_USBH_FILE_DM_Write
*
*   DESCRIPTION
*
*       This function writes to the usb ms device.
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Read buffer
*       UINT32       numbyte                - Number of bytes to read
*       OFFSET_T     byte_offset            - Byte offset
*       UINT32       *bytes_written_ptr     - Number of bytes written
*
*   OUTPUTS
*
*       STATUS       status                 - NU_SUCCESS
*
*************************************************************************/
STATUS  NU_USBH_FILE_DM_Write (VOID *session_handle,
                                        const VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_written_ptr)
{
    NUF_USBH_DEVICE        *device_ptr;
    UINT32 offset_sector = 0;
    UINT32 offset_residue= 0;
    UINT32 size_residue = 0;
    UINT32 remaining_len = 0;
    UINT32 size_sector = 0;
    UINT8 *uncached_buffer;
    STATUS status = NU_SUCCESS;
    UINT8 *buff;

    NU_USB_PTRCHK(session_handle);
    NU_USB_PTRCHK(buffer);
    NU_USB_PTRCHK(bytes_written_ptr);

    device_ptr = (NUF_USBH_DEVICE*)session_handle;
    offset_sector = byte_offset / device_ptr->block_len;
    offset_residue = byte_offset % device_ptr->block_len;
    size_sector = numbyte / device_ptr->block_len;
    size_residue = numbyte % device_ptr->block_len;
    remaining_len = numbyte;
    *bytes_written_ptr = 0;
    buff = (UINT8*)buffer;

    /* Uncached RW buffer. */
    uncached_buffer = device_ptr->rw_buffer;

    do
    {
        /* Acquire device read/write lock. */
        status = NU_Obtain_Semaphore(&device_ptr->rw_lock, NU_SUSPEND);
        if (status != NU_SUCCESS)
        {
            break;
        }

        if((size_residue !=0 )||(offset_residue != 0))
        {
            if(offset_residue != 0)
            {
                /*if offset is not on sector boundary then we read the full block
                 *and modify the data which needs to be written  */
                status = NUF_USBH_Media_Io (device_ptr,
                                               offset_sector,
                                               uncached_buffer,
                                               1,
                                               1);
                if(status !=NU_SUCCESS)
                {
                    break;
                }
                memcpy((VOID*)&(((UINT8*)uncached_buffer)[offset_residue]),
                       buff,
                       ((device_ptr->block_len)-offset_residue));
                status = NUF_USBH_Media_Io (device_ptr,
                               offset_sector,
                               uncached_buffer,
                               1,
                               0);

                if(status !=NU_SUCCESS)
                {
                    break;
                }
                /*re calculating the size residue to ensure that when we reach at the end
                 *of write operation, the size residue is based on the first less then 1 block write.*/
                buff = (UINT8*)&(buff[(device_ptr->block_len)-offset_residue]);
                numbyte = numbyte - ((device_ptr->block_len)-offset_residue);
                offset_residue = 0;
                *bytes_written_ptr = ((device_ptr->block_len)-offset_residue);
                /*re calculating the sector boundaries */
                offset_sector++;
                size_sector = numbyte / device_ptr->block_len;
                size_residue = numbyte % device_ptr->block_len;
            }

            /* Read aligned sectors in between. */
            status = NUF_USBH_Aligned_Write_Uncached(device_ptr, offset_sector, size_sector, buff);
            if(status !=NU_SUCCESS)
            {
                break;
            }

            offset_sector = offset_sector + size_sector;
            numbyte = numbyte -size_sector * (device_ptr->block_len);
            buff = (UINT8*)&(buff[(size_sector * (device_ptr->block_len))]);
            *bytes_written_ptr = *bytes_written_ptr + size_sector * (device_ptr->block_len);
            if(size_residue != 0)
            {
                /*if the size residue is not zero so well need to copy the last sector in a local buffer
                 * and then write only number of bytes actually required.*/
                status = NUF_USBH_Media_Io (device_ptr,
                                        offset_sector,
                                        (void*)uncached_buffer,
                                        1,
                                        1);
                if(status !=NU_SUCCESS)
                {
                    break;
                }
                memcpy(((VOID*)&uncached_buffer),buff,size_residue);
                status = NUF_USBH_Media_Io (device_ptr,
                        offset_sector,
                        (void*)uncached_buffer,
                        1,
                        1);
                if(status !=NU_SUCCESS)
                {
                    break;
                }

                *bytes_written_ptr = *bytes_written_ptr + size_residue;
            }
        }
        else
        {
            /* if every thing is sector aligned.*/
            status = NUF_USBH_Aligned_Write_Uncached(device_ptr, offset_sector, size_sector, buff);
            if(status !=NU_SUCCESS)
            {
                break;
            }
            *bytes_written_ptr = remaining_len;

        }
    }
    while(0);

    /* Release device read/write lock. */
    NU_Release_Semaphore(&device_ptr->rw_lock);

    return (status);
}
/*************************************************************************
*
*   FUNCTION
*
*       NU_USBH_FILE_DM_IOCTL
*
*   DESCRIPTION
*
*       This function writes to the usb ms device*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       INT          cmd                    - IOCTL command
*       VOID         *data                  - structure to get or
*                                             set IOCTL data
*       INT          length                 - length of structure
*
*   OUTPUTS
*
*       STATUS       status                 - NU_SUCCESS
*
*************************************************************************/
STATUS   NU_USBH_FILE_DM_IOCTL (VOID *session_handle,
                             INT cmd,
                             VOID *data,
                             INT length)
{

    STATUS                  status;

    FPART_DISK_INFO_S *disk_info= NU_NULL;
    STATUS *chkdsk_sts = NU_NULL;
    NUF_USBH_DEVICE        *device_ptr = NU_NULL;
    STATUS                  pm_status = NU_SUCCESS;
    PM_STATE_ID            *pm_data = NU_NULL;
    CHAR                    in_reg_path[REG_MAX_KEY_LENGTH];
    STORAGE_MW_CONFIG_PATH  *config_path;

    NU_USB_PTRCHK(session_handle);
    NU_USB_PTRCHK(data);


    device_ptr = (NUF_USBH_DEVICE*)session_handle;

    /* Process command */
    switch (cmd)
    {
        case (USB_FILE_IOCTL_BASE + FDEV_GET_DISK_INFO):

            /* Get the dev info structure from the data passed in */
            disk_info = (FPART_DISK_INFO_S *) data;

            strcpy(disk_info->fpart_name, device_ptr->dev_name);



            disk_info->fpart_bytes_p_sec = device_ptr->block_len;
            /* The FPART_DI_RMVBL_MED is a trick to get
               the partition component to use a fake partition. */
            disk_info->fpart_flags = FPART_DI_LBA_SUP | FPART_DI_RMVBL_MED;
            disk_info->fpart_heads = NUF_USBH_HEAD_COUNT;
            disk_info->fpart_secs = NUF_USBH_SECTORS_PER_TRACK;
            disk_info->fpart_tot_sec = device_ptr->last_lba;
            disk_info->fpart_cyls = disk_info->fpart_tot_sec / (NUF_USBH_HEAD_COUNT * NUF_USBH_SECTORS_PER_TRACK);
            status = NU_SUCCESS;
            break;

        case (USB_FILE_IOCTL_BASE + FDEV_GET_DISK_STATUS):

            chkdsk_sts = (STATUS *) data;

            /* Check if the disk has been initialized */
            if (device_ptr->media_status != NUF_USBH_DEV_REMOVED)
            {
                *chkdsk_sts = NU_SUCCESS;
            }
            else
            {
                *chkdsk_sts = NUF_NOT_OPENED;
            }
            status = NU_SUCCESS;
            break;

        case (USB_FILE_IOCTL_BASE + FDEV_GET_MW_CONFIG_PATH):
            /* Get the Config Path structure from the data passed in */
            config_path = (STORAGE_MW_CONFIG_PATH *) data;

            /* Return the middleware config path */
            strcpy(in_reg_path, USBH_NUF_reg_path);
            if(strlen(in_reg_path) <= ((REG_MAX_KEY_LENGTH -1) - strlen("/mw_settings")))
            {
                strcat(in_reg_path, "/mw_settings");
            }
            if(config_path->max_path_len <= REG_MAX_KEY_LENGTH)
            {
                strncpy(config_path->config_path, in_reg_path, config_path->max_path_len);
                status = NU_SUCCESS;
                break;
            }

        case (USB_FILE_IOCTL_BASE + FDEV_FLUSH):
        case (USB_FILE_IOCTL_BASE + FDEV_TRIM):
            status = DV_IOCTL_INVALID_CMD;
            break;

        case (USB_POWER_IOCTL_BASE + POWER_IOCTL_GET_STATE):
            if ((length == sizeof (PM_STATE_ID)) && (data != NU_NULL))
            {
                pm_data = (PM_STATE_ID *)data;
                *pm_data = device_ptr->pm_state;
            }

            status = NU_SUCCESS;
            break;

        case (USB_POWER_IOCTL_BASE + POWER_IOCTL_SET_STATE):
            if ((length == sizeof(PM_STATE_ID)) && (data != NU_NULL))
            {
                pm_data = (PM_STATE_ID*)data;

                if(pm_data)
                {
                    if(((*pm_data == POWER_OFF_STATE)||(*pm_data == USB_POWER_STATE_OFF))
                       &&(device_ptr->pm_state == USB_POWER_STATE_OFF))
                    {
                        status =  NU_USBH_MS_Suspend_Device (device_ptr->dev_handle);
                        if(status == NU_SUCCESS)
                        {
                           device_ptr->pm_state = USB_POWER_STATE_OFF;
                           pm_status = NU_SUCCESS;
                        }
                        else
                           pm_status = PM_TRANSITION_FAILED;

                    }
                    else if(((*pm_data == POWER_ON_STATE)||(*pm_data == USB_POWER_STATE_ON))
                       &&(device_ptr->pm_state == USB_POWER_STATE_OFF))
                    {
                        status =  NU_USBH_MS_Resume_Device (device_ptr->dev_handle);
                        if(status == NU_SUCCESS)
                        {
                           device_ptr->pm_state = USB_POWER_STATE_ON;
                           pm_status = NU_SUCCESS;
                        }
                        else
                           pm_status = PM_TRANSITION_FAILED;

                    }
                    else
                    {
                        pm_status = PM_INVALID_POINTER;
                    }
                }
                else
                {
                    pm_status = PM_INVALID_POINTER;
                }
            }
            status = (STATUS)pm_status;
            break;

        case (USB_POWER_IOCTL_BASE + POWER_IOCTL_GET_STATE_COUNT):
            if ((length == sizeof(PM_STATE_ID)) && (data != NU_NULL))
            {
                pm_data = (PM_STATE_ID *)data;
                *pm_data = USB_TOTAL_POWER_STATES;
            }

            status = DV_IOCTL_INVALID_CMD;
            break;

        default:
            status = DV_IOCTL_INVALID_CMD;
            break;
    }

    return (status);


}
/* ====================  End Of File  ================================== */
