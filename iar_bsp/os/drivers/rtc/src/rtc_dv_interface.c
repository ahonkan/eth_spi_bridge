/*************************************************************************
*
*               Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       rtc_dv_interface.c
*
*   COMPONENT
*
*       RTC                            - RTC Library
*
*   DESCRIPTION
*
*       This file contains the generic RTC DV interface
*       library functions.
*
*   FUNCTIONS
*
*       RTC_Dv_Register
*       RTC_Dv_Unregister
*       RTC_Dv_Open
*       RTC_Dv_Close
*       RTC_Dv_Ioctl
*
*   DEPENDENCIES
*
*       time.h
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*
*************************************************************************/
#include <time.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"

/* External Functions */
extern VOID    RTC_Tgt_Enable_Device(RTC_INSTANCE_HANDLE *inst_ptr);
extern VOID    RTC_Tgt_Disable_Device(RTC_INSTANCE_HANDLE *inst_ptr);
extern STATUS  RTC_Tgt_Read(VOID* session_handle, VOID *buffer, UINT32 numbyte,
                        OFFSET_T byte_offset, UINT32 *bytes_read);
extern STATUS  RTC_Tgt_Write(VOID *session_handle, const VOID *buffer, UINT32 numbyte,
                            OFFSET_T byte_offset, UINT32 *bytes_written);

/**************************************************************************
* FUNCTION
*
*       RTC_Dv_Register
*
* DESCRIPTION
*
*       This function registers the RTC driver with
*       device manager.
*
* INPUTS
*
*       key                                 Path to registery settings.
*       inst_ptr                            Pointer to RTC structure.
*
* OUTPUTS
*
*       status                              Function completion status.
*
**************************************************************************/
STATUS RTC_Dv_Register(const CHAR * key, RTC_INSTANCE_HANDLE *inst_ptr)
{
    STATUS                  status = NU_SUCCESS;
    DV_DEV_LABEL            rtc_labels[DV_MAX_DEV_LABEL_CNT] = {{RTC_LABEL}};
    INT                     rtc_label_cnt = 1;
    DV_DRV_FUNCTIONS        drv_funcs;

    if(status == NU_SUCCESS)
    {
        drv_funcs.drv_open_ptr  = RTC_Dv_Open;
        drv_funcs.drv_close_ptr = RTC_Dv_Close;
        drv_funcs.drv_read_ptr  = RTC_Tgt_Read;
        drv_funcs.drv_write_ptr = RTC_Tgt_Write;
        drv_funcs.drv_ioctl_ptr = RTC_Dv_Ioctl;

        /* Reset value of device ID. */
        inst_ptr->dev_id = DV_INVALID_DEV;

        /* Register with device manager. */
        status = DVC_Dev_Register(inst_ptr,
                                  rtc_labels,
                                  rtc_label_cnt,
                                  &drv_funcs,
                                  &(inst_ptr->dev_id));
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       RTC_Dv_Unregister
*
* DESCRIPTION
*
*       This function un-registers the RTC driver with
*       device manager.
*
* INPUTS
*
*       key                                 Device registry path.
*       startstop                           Start or Stop the device.
*       dev_id                              Device ID.
*
* OUTPUTS
*
*       status                              Function completion status.
*
**************************************************************************/
STATUS RTC_Dv_Unregister(const CHAR * key, INT startstop, DV_DEV_ID dev_id)
{
    STATUS                  status;
    RTC_INSTANCE_HANDLE     *inst_ptr;

    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(startstop);

    /* Unregister device with device manager. */
    status = DVC_Dev_Unregister(dev_id, (VOID**)&inst_ptr);

    /* Free the instance handle. */
    if ((status == NU_SUCCESS) && (inst_ptr != NU_NULL))
    {
        NU_Deallocate_Memory((VOID*)inst_ptr);
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*       RTC_Dv_Open
*
* DESCRIPTION
*
*       This function opens the device and creates a session handle.
*
* INPUTS
*
*       inst_ptr                            Instance handle.
*       label_list[]                        Access mode (label) of open.
*       labels_cnt                          Number of labels.
*       session_handle                      Pointer to session handle.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*       RTC_DEV_IN_USE                      Device is already opened
*                                           in requested mode.
*
**************************************************************************/
STATUS RTC_Dv_Open(VOID *inst_ptr, DV_DEV_LABEL label_list[],
                        INT label_cnt, VOID **session_handle)
{
    STATUS                  status;
    RTC_INSTANCE_HANDLE     *loc_inst_ptr;
    RTC_SESSION_HANDLE      *ses_ptr;
    UINT32                  open_mode;
    DV_DEV_LABEL            rtc_label = {RTC_LABEL};
    VOID*                   pointer;
    NU_MEMORY_POOL          *System_Memory;

    /* Initialize local variables. */
    status          = NU_SUCCESS;
    open_mode       = 0;
    loc_inst_ptr    = (RTC_INSTANCE_HANDLE*)inst_ptr;

    /* Get open mode requests from labels. */
    if (DVS_Label_List_Contains(label_list, label_cnt, rtc_label) == NU_SUCCESS)
    {
        open_mode |= RTC_OPEN_MODE;
    }

    /* Proceed only if, the device is not opened in RTC mode already */
    if (loc_inst_ptr->device_in_use != NU_TRUE)
    {
        /* Get system memory pool pointer */
        status = NU_System_Memory_Get(&System_Memory, NU_NULL);

        if(status == NU_SUCCESS)
        {
            /* Allocate a new session. */
            status = NU_Allocate_Memory(System_Memory,
                                        &pointer,
                                        sizeof(RTC_SESSION_HANDLE),
                                        NU_NO_SUSPEND);
        }
        if (status == NU_SUCCESS)
        {
            ESAL_GE_MEM_Clear (pointer, sizeof(RTC_SESSION_HANDLE));
            ses_ptr = (RTC_SESSION_HANDLE*)pointer;

            /* Init session. */
            ses_ptr->inst_ptr = inst_ptr;

            /* If open mode request is RTC. */
            if (open_mode & RTC_OPEN_MODE)
            {
                /* Set device in use. */
                loc_inst_ptr->device_in_use = NU_TRUE;
            }

            /* Enable Device*/
            RTC_Tgt_Enable_Device(loc_inst_ptr);

            ses_ptr->open_mode |= open_mode;

            /* Return session handle. */
            *(RTC_SESSION_HANDLE**)session_handle = ses_ptr;
        }
    }
    else
    {
        status = RTC_DEV_IN_USE;
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       RTC_Dv_Close
*
* DESCRIPTION
*
*       This function closes the RTC controller associated with the
*       specified RTC device. The RTC controller is still left running.
*
* INPUTS
*
*       sess_handle                         Nucleus RTC driver session
*                                           handle pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
**************************************************************************/
STATUS RTC_Dv_Close(VOID *sess_handle)
{
    STATUS                 status;
    RTC_SESSION_HANDLE    *ses_ptr;
    RTC_INSTANCE_HANDLE   *inst_ptr;
    INT                    int_level;

    /* Initialize local variables. */
    status  = ~NU_SUCCESS;

    ses_ptr = (RTC_SESSION_HANDLE*)sess_handle;

    /* If a valid session, then close it. */
    if(ses_ptr != NU_NULL)
    {
        /* Get pointer to instance handle. */
        inst_ptr = ses_ptr->inst_ptr;

        if(inst_ptr != NU_NULL)
        {
            if (ses_ptr->open_mode & RTC_OPEN_MODE)
            {
                /* Disable interrupts before clearing shared variable. */
                int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

                /* Set device is closed. */
                inst_ptr->device_in_use = NU_FALSE;

                /* Restore interrupts to previous level. */
                NU_Local_Control_Interrupts(int_level);

                status = NU_SUCCESS;
            }

            /* Free the session. */
            NU_Deallocate_Memory ((VOID*)ses_ptr);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*       RTC_Dv_Ioctl
*
* DESCRIPTION
*
*       This function is responsible for performing miscellaneous control
*       operations.
*
* INPUTS
*
*       sess_ptr                            Session handle of the driver
*       ioctl_cmd                           Ioctl command
*       ioctl_data                          Ioctl data pointer
*       length                               Ioctl length
*
* OUTPUTS
*
*       NU_SUCCESS                     Service completed
*                                              successfully.
*
**************************************************************************/
STATUS RTC_Dv_Ioctl(VOID *sess_ptr, INT ioctl_cmd, VOID *ioctl_data, INT length)
{

    STATUS                  status = DV_INVALID_INPUT_PARAMS;
    DV_IOCTL0_STRUCT        *ioctl0;
    RTC_SESSION_HANDLE      *ses_ptr;
    RTC_INSTANCE_HANDLE     *inst_ptr;
    DV_DEV_LABEL            rtc_label = {RTC_LABEL};

    /* Initialize local variables. */
   ses_ptr     = (RTC_SESSION_HANDLE*)sess_ptr;
   inst_ptr    = ses_ptr->inst_ptr;

    /* Determine the control operation to be performed. */
    switch(ioctl_cmd)
    {
        case DV_IOCTL0:
        {
            if(length == sizeof(DV_IOCTL0_STRUCT))
            {
                ioctl0 = ioctl_data;
                status = DV_IOCTL_INVALID_MODE;

                if (status != NU_SUCCESS)
                {
                    if (DV_COMPARE_LABELS (&(ioctl0->label), &rtc_label) &&
                        (ses_ptr->open_mode & RTC_OPEN_MODE))
                    {
                        ioctl0->base = RTC_MODE_IOCTL_BASE;
                        status = NU_SUCCESS;
                    }
                }
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }

        case RTC_MODE_IOCTL_BASE + RTC_ENABLE_DEVICE:
        {
            /* Call the target specific device enable routine. */
            RTC_Tgt_Enable_Device(inst_ptr);
            status = NU_SUCCESS;
            break;
        }

        case RTC_MODE_IOCTL_BASE + RTC_DISABLE_DEVICE:
        {
            /* Call the target specific device disable routine. */
            RTC_Tgt_Disable_Device(inst_ptr);
            status = NU_SUCCESS;
            break;
        }

        default:
        {
            status = DV_INVALID_INPUT_PARAMS;
            break;
        }
    }

    /* Return the completion status of the service. */
    return (status);
}
