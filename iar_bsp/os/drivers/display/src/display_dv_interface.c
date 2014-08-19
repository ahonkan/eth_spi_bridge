/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
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
*       display_dv_interface.c
*
*   COMPONENT
*
*       Display - Display driver
*
*   DESCRIPTION
*
*       This file contains the generic routines of LCD driver.
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "services/nu_trace_os_mark.h"

/*****************************/
/* FUNCTION PROTOTYPES       */
/*****************************/
extern VOID    *Display_Tgt_Allocate_Framebuffer(VOID *tgt_info);
extern STATUS  Display_Tgt_Initialize(DISPLAY_INSTANCE_HANDLE *instance_ptr, VOID *display_framebuffer_addr);
extern VOID    Display_Tgt_Shutdown(DISPLAY_INSTANCE_HANDLE *instance_ptr);
extern VOID    Display_Tgt_Pre_Process_Hook(VOID);
extern VOID    Display_Tgt_Post_Process_Hook(VOID);
extern VOID    Display_Tgt_Fill_Rect_Hook(UINT16 x_min, UINT16 x_max, UINT16 y_min, UINT16 y_max, UINT32 fill_colour);
extern VOID    Display_Tgt_Set_Pixel_Hook(UINT16 x_coordinate, UINT16 y_coordinate, UINT32 fill_colour);
extern VOID    Display_Tgt_Get_Pixel_Hook(UINT16 x_coordinate, UINT16 y_coordinate, UINT32* fill_colour_ptr);
extern VOID    Display_Tgt_Read_Palette(VOID *palette_ptr);
extern VOID    Display_Tgt_Write_Palette(VOID *palette_ptr);
extern STATUS  Display_Tgt_Contrast_Get(VOID *tgt_info, INT8 *contrast_ptr);
extern STATUS  Display_Tgt_Contrast_Set(VOID *tgt_info, INT8 contrast);
extern STATUS  Display_Tgt_Backlight_Status(VOID *tgt_info, BOOLEAN  *backlight_status_ptr);
extern STATUS  Display_Tgt_Backlight_Set_ON(VOID *tgt_info);
extern STATUS  Display_Tgt_Backlight_Set_OFF(VOID *tgt_info);
extern STATUS  Display_Tgt_Set_State_ON(VOID *instance_handle);
extern STATUS  Display_Tgt_Set_State_OFF(VOID *instance_handle);
extern STATUS  Display_Tgt_Min_OP_Pt_Calc (UINT8* min_op_pt);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern STATUS   Display_Tgt_Pwr_Set_State(VOID *instance_handle, PM_STATE_ID *state);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

extern STATUS   Display_Tgt_Pwr_Hibernate_Restore (DISPLAY_SESSION_HANDLE * session_handle);

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

/*************************************************************************
*
*   FUNCTION
*
*       Display_Dv_Register
*
*   DESCRIPTION
*
*       This function registers the LCD device.
*
*   INPUTS
*
*       key                                 Key
*       startstop                           Start or stop flag
*       dev_id                              Pointer to the location
*                                           where device ID of the
*                                           registered device will be
*                                           returned
*
*   OUTPUTS
*
*       NU_SUCCESS                          Registration successful
*       LCDC_PM_ERROR                       Error occurred in power
*                                           management requests
*
*************************************************************************/
STATUS Display_Dv_Register(const CHAR *key,
                               DISPLAY_INSTANCE_HANDLE *inst_handle)
{
    STATUS               status = NU_SUCCESS;
    INT                  new_label_cnt = 1;
    DV_DEV_LABEL         new_labels[3] = {{DISPLAY_LABEL}};
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif    

    /* DVR function pointers */
    DV_DRV_FUNCTIONS lcdc_drv_funcs =
    {
        Display_Dv_Open,
        Display_Dv_Close,
        NU_NULL,
        NU_NULL,
        Display_Dv_Ioctl
    };

    if (inst_handle != NU_NULL)
    {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

        /********************************/
        /* INITIALIZE AS POWER DEVICE   */
        /********************************/
        status = PMI_Device_Initialize(&(inst_handle->pmi_dev), key,
                                       new_labels, &new_label_cnt, 0);

        if (status == NU_SUCCESS)
        {
            /* Setup the power device */
            PMI_Device_Setup(inst_handle->pmi_dev, Display_Tgt_Pwr_Set_State, DISPLAY_POWER_BASE,
                             DISPLAY_TOTAL_STATE_COUNT, &inst_handle->dev_id, (VOID*)inst_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

            /* Perform DVFS related setup */
            PMI_DVFS_Setup(inst_handle->pmi_dev, key, (VOID*)inst_handle,
                           &Display_Tgt_Set_State_OFF, NU_NULL,
                           &Display_Tgt_Set_State_ON, NU_NULL, NU_NULL);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
        }

#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

        /*********************************/
        /* REGISTER WITH DEVICE MANAGER  */
        /*********************************/  
        if (status == NU_SUCCESS)
        { 
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))           
            TCCT_Schedule_Lock();
#endif
            /* Register this device with the Device Manager */
            status = DVC_Dev_Register((VOID*)inst_handle, new_labels,
                                      new_label_cnt, &lcdc_drv_funcs,
                                      &inst_handle->dev_id);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                   
            /* Get default power state */
            init_pwr_state = PMI_STATE_GET(inst_handle->pmi_dev);

            /* Trace log */
            T_DEV_NAME((CHAR*)key, init_pwr_state, DISPLAY_TOTAL_STATE_COUNT, inst_handle->dev_id);        
            
            TCCT_Schedule_Unlock();
#endif            
        }
    }
    
    /* Return completion status of the service. */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Display_Dv_Unregister
*
*   DESCRIPTION
*
*       This function unregisters the LCDC device.
*
*   INPUTS
*
*       key                                 Key
*       startstop                           Start or stop flag
*       dev_id                              ID of the device to be
*                                           unregistered
*
*   OUTPUTS
*
*       NU_SUCCESS                          Successful completion
*       LCDC_PM_ERROR                       Error occurred in power
*                                           management requests
*       other                               Error codes from device
*                                           manager requests
*
*************************************************************************/
STATUS Display_Dv_Unregister (const CHAR * key, INT startstop, DV_DEV_ID dev_id)
{
    STATUS               status;
    DISPLAY_INSTANCE_HANDLE *inst_handle;

    /* Unregister the device with Device Manager */
    status = DVC_Dev_Unregister(dev_id, (VOID**)&inst_handle);

    if ((status == NU_SUCCESS) && (inst_handle != NU_NULL))
    {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
        status = PMI_Device_Unregister(inst_handle->pmi_dev);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

        /* Zero-ize instance structure */
        (VOID)memset((VOID *)inst_handle, 0, (INT)sizeof(DISPLAY_INSTANCE_HANDLE));

        /* Deallocate memory instance memory */
        status = NU_Deallocate_Memory((VOID*)inst_handle);
    }

    /* Return completion status of the service. */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Display_Dv_Open
*
*   DESCRIPTION
*
*       This function creates a session handle
*
*   INPUTS
*
*       instance_handle                     Instance handle of the driver
*       labels_list                         Access modes (labels) to open
*       labels_cnt                          Number of labels
*       session_handle                      Pointer to the location where
*                                           session handle will be returned
*
*   OUTPUTS
*
*       NU_SUCCESS                          Successful completion
*       NU_NO_MEMORY                        Memory not available
*
*************************************************************************/
STATUS Display_Dv_Open (VOID *instance_handle, DV_DEV_LABEL labels_list[],
                        INT labels_cnt, VOID **session_handle)
{
    DISPLAY_INSTANCE_HANDLE *instance_ptr = (DISPLAY_INSTANCE_HANDLE*)instance_handle;
    STATUS                  status = NU_SUCCESS;
    DISPLAY_SESSION_HANDLE  *ses_ptr = (DISPLAY_SESSION_HANDLE*) - 1;
#ifndef SMART_LCD
    VOID                    *tgt_info = instance_ptr->display_tgt_ptr;
#endif /* SMART_LCD */
    UINT32                  open_mode_requests = 0;
    DV_DEV_LABEL            display_labels = {DISPLAY_LABEL};
    INT                     display_list = DISPLAY_OPEN_MODE;
    VOID                    *frame_buffer = NU_NULL;
    NU_MEMORY_POOL          *sys_pool_ptr;
    
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    PMI_DEV_HANDLE          pmi_dev = instance_ptr->pmi_dev;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
    UINT8                   min_op_pt;
    STATUS                  pm_status = NU_SUCCESS;

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */


    /* Get open mode requests from labels */
    if (DVS_Label_List_Contains (labels_list, labels_cnt, display_labels) == NU_SUCCESS)
    {
        open_mode_requests |= display_list;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    status = PMI_Device_Open (&open_mode_requests, labels_list, labels_cnt);
#endif

    /* If device is already open AND if the open request contains serial mode, return a error. */
    if (!((instance_ptr->device_in_use == NU_TRUE) && (open_mode_requests & DISPLAY_OPEN_MODE)))
    {
        /* Get system memory pool pointer */
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Allocate memory for session pointer */
            status = NU_Allocate_Memory(sys_pool_ptr, (VOID**)&ses_ptr,
                                        sizeof(DISPLAY_SESSION_HANDLE), NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Zero out allocated space */
                (VOID)memset ((VOID*)ses_ptr, 0, sizeof (DISPLAY_SESSION_HANDLE));

                /* Place a pointer to instance handle in session handle */
                ses_ptr->inst_info = instance_ptr;

                ses_ptr->open_modes |= open_mode_requests;

                /* If the open mode request is Display */
                if (open_mode_requests & DISPLAY_OPEN_MODE)
                {
                    /* Set device in use flag to true */
                    instance_ptr->device_in_use = NU_TRUE;
                }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

                /* If open mode request is power */
                if (open_mode_requests & POWER_OPEN_MODE)
                {
                    /* Check if LCD is in ON state. */
                    if (instance_ptr->device_in_use)
                    {
                        PMI_STATE_SET(pmi_dev, LCD_ON);
                    }
                }

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

                /* Set the return address of the session handle */
                *session_handle = (VOID*)ses_ptr;

                /* If the open mode request is serial and the previous operation is successful */
                if ((open_mode_requests & DISPLAY_OPEN_MODE))
                {
#ifndef SMART_LCD
                    /* no frame buffer will be allocated for smart lcd */
                    frame_buffer = Display_Tgt_Allocate_Framebuffer(tgt_info);
                    if (frame_buffer != NU_NULL)
                    {
#endif
                                                       
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))
                        /* Determine the min OP point for the LCD driver. */
                        status = Display_Tgt_Min_OP_Pt_Calc(&min_op_pt);         

                        if(status == NU_SUCCESS)
                        {
                            /* First ensure that DVFS is at min op before DMA is enabled */
                            pm_status = PMI_REQUEST_MIN_OP(min_op_pt, pmi_dev);
                            
                            if (pm_status == NU_SUCCESS)
                            {
                                /* Update MPL for DVFS. */
                                pm_status = PMI_DVFS_Update_MPL_Value(pmi_dev, PM_NOTIFY_ON);
                            }
                        }
                        
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)) */

                        instance_ptr->display_framebuffer_addr = frame_buffer;
                        status = Display_Tgt_Initialize(instance_ptr, frame_buffer);
#ifndef SMART_LCD
                    }
#endif
                }
            }
        }
    }
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Display_Dv_Close
*
*   DESCRIPTION
*
*       This function deletes the session handle
*
*   INPUTS
*
*       session_handle                      Session handle pointer
*
*   OUTPUTS
*
*       NU_SUCCESS                          Successful completion
*       NU_INVALID_POINTER                  Indicates the supplied
*                                           pointer is invalid
*
*************************************************************************/
STATUS Display_Dv_Close(VOID *session_handle)
{
    STATUS                  status = NU_SUCCESS;
    DISPLAY_SESSION_HANDLE  *lcdc_handle = (DISPLAY_SESSION_HANDLE *)session_handle;
    DISPLAY_INSTANCE_HANDLE *inst_ptr = lcdc_handle->inst_info;

    /* Check if device is open in display mode. */
    if (lcdc_handle->open_modes & DISPLAY_OPEN_MODE)
    {
        /* Deallocate the frame buffer. */
        status = NU_Deallocate_Memory(inst_ptr->display_framebuffer_addr);

        if (status == NU_SUCCESS)
        {
            Display_Tgt_Shutdown(inst_ptr);

            /* Set device is closed */
            inst_ptr->device_in_use = NU_FALSE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            status = PMI_Device_Close(inst_ptr->pmi_dev);

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

            /* Mark the session as free. */
            lcdc_handle->open_modes = 0;

            /* Deallocate memory for session handle */
            status = NU_Deallocate_Memory(session_handle);
        }
    }

    /* Return the completion status. */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Display_Dv_Ioctl
*
*   DESCRIPTION
*
*       This function controls IO operations of the LCD.
*
*   INPUTS
*
*       session_handle                      Session handle of the driver
*       cmd                                 Ioctl command
*       data                                Ioctl data pointer
*       length                              Ioctl length
*
*   OUTPUTS
*
*       NU_SUCCESS                          Successful completion
*       DV_IOCTL_INVALID_MODE               Invalid open mode/label
*       DV_IOCTL_INVALID_LENGTH             Specified data length is invalid
*       NU_UNAVAILABLE                      Specified IOCTL code is unknown
*
*************************************************************************/
STATUS  Display_Dv_Ioctl(VOID *session_handle, INT cmd, VOID *data, INT length)
{
    DISPLAY_SESSION_HANDLE  *lcdc_handle = (DISPLAY_SESSION_HANDLE *)session_handle;
    DISPLAY_INSTANCE_HANDLE *instance_ptr = lcdc_handle->inst_info;
    STATUS                  status = NU_SUCCESS;
    DV_IOCTL0_STRUCT        *ioctl0;
    VOID                    *tgt_info = instance_ptr->display_tgt_ptr;
    DV_DEV_LABEL            display_label = {DISPLAY_LABEL};
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE          pmi_dev = instance_ptr->pmi_dev;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */


    /* Process command */
    switch (cmd)
    {
        case DV_IOCTL0:

            if (length == sizeof(DV_IOCTL0_STRUCT))
            {
                /* Get the ioctl0 structure from the data passed in */
                ioctl0 = (DV_IOCTL0_STRUCT *) data;
                status = DV_IOCTL_INVALID_MODE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                status = PMI_Device_Ioctl(pmi_dev, cmd, data, length, (VOID*)(instance_ptr),
                                          (lcdc_handle->open_modes));
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

                if (status != NU_SUCCESS)
                {
                    /* If the mode requested is supported and if the session was opened for that mode */
                    if ((lcdc_handle->open_modes & DISPLAY_OPEN_MODE) &&
                        (DV_COMPARE_LABELS(&(ioctl0->label), &display_label)))
                    {
                        ioctl0->base = IOCTL_DISPLAY_BASE;
                        status = NU_SUCCESS;
                    }
                }

            }

            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }

            break;

            /*******************/
            /* Display Ioctls */
            /*******************/
        case (IOCTL_DISPLAY_BASE + DISPLAY_GET_FRAME_BUFFER):

            /* Return the framebuffer address. */
            *((VOID **)data) = instance_ptr->display_framebuffer_addr;

            break;

        case (IOCTL_DISPLAY_BASE + DISPLAY_GET_PREPROCESS_HOOK):

            /* Return pre-process hook function pointer. */
            *((VOID (*(*))(VOID))data) = Display_Tgt_Pre_Process_Hook;

            break;

        case (IOCTL_DISPLAY_BASE + DISPLAY_GET_POSTPROCESS_HOOK):

            /* Return post-process hook function pointer. */
            *((VOID (*(*))(VOID))data) = Display_Tgt_Post_Process_Hook;

            break;

        case (IOCTL_DISPLAY_BASE + DISPLAY_GET_FILLRECT_HOOK):

            /* Return fill-rectangle hook function pointer. */
            *((VOID (*(*))(UINT16, UINT16, UINT16, UINT16, UINT32))data) = Display_Tgt_Fill_Rect_Hook;

            break;

        case (IOCTL_DISPLAY_BASE + DISPLAY_GET_SETPIXEL_HOOK):

            /* Return set pixel hook function pointer. */
            *((VOID (*(*))(UINT16, UINT16, UINT32))data) = Display_Tgt_Set_Pixel_Hook;

            break;

        case (IOCTL_DISPLAY_BASE + DISPLAY_GET_GETPIXEL_HOOK):

            /* Return get pixel hook function pointer. */
            *((VOID (*(*))(UINT16, UINT16, UINT32*))data) = Display_Tgt_Get_Pixel_Hook;

            break;


        case (IOCTL_DISPLAY_BASE + DISPLAY_GET_READ_PALETTE_FUNC):

            /* Return the read palette function pointer. */
            *((VOID (*(*))(VOID *))data) = Display_Tgt_Read_Palette;

            break;

        case (IOCTL_DISPLAY_BASE + DISPLAY_GET_WRITE_PALETTE_FUNC):

            /* Return the write palette function pointer. */
            *((VOID (*(*))(VOID *))data) = Display_Tgt_Write_Palette;

            break;

        case (IOCTL_DISPLAY_BASE + DISPLAY_GET_MW_CONFIG_PATH):

            if (length >= strlen(instance_ptr->mw_config_path))
            {
                /* Return the framebuffer address. */
                strcpy((char *)data, instance_ptr->mw_config_path);
            }

            break;

        case (IOCTL_DISPLAY_BASE + DISPLAY_GET_CONTRAST):

            /* Return current contrast value. */
            status = Display_Tgt_Contrast_Get(tgt_info, (INT8 *)data);
            
            break;

        case (IOCTL_DISPLAY_BASE + DISPLAY_SET_CONTRAST):

            /* Return current contrast value. */
            status = Display_Tgt_Contrast_Set(tgt_info, *(INT8 *)data);
            
            break;

        case (IOCTL_DISPLAY_BASE + DISPLAY_BACKLIGHT_STATUS):

            /* Return current contrast value. */
            status = Display_Tgt_Backlight_Status(tgt_info, (BOOLEAN *)data);
            
            break;

        case (IOCTL_DISPLAY_BASE + DISPLAY_BACKLIGHT_CTRL):

            if (*(BOOLEAN *)data == NU_TRUE)
            {
                status = Display_Tgt_Backlight_Set_ON(tgt_info);
            }
            else
            {
                status = Display_Tgt_Backlight_Set_OFF(tgt_info);
            }
            
            break;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)
            
        case (IOCTL_DISPLAY_BASE + DISPLAY_PWR_HIB_RESTORE):
            
            /* Call hibernate restore for display session. */
            status = Display_Tgt_Pwr_Hibernate_Restore(lcdc_handle);
        
            break;
            
#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

        default:

            status = PMI_Device_Ioctl(pmi_dev, cmd, data, length, (VOID*)(lcdc_handle->inst_info),
                                      (lcdc_handle->open_modes));
#else

            status = NU_UNAVAILABLE;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
            break;
    }

    /* Return the completion status. */
    return (status);
}
