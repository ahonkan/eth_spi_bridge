/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                      All Rights Reserved.
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
*       keypad_dv_interface.c
*
*   COMPONENT
*
*       Keypad Driver
*
*   DESCRIPTION
*
*       This files contains the generic routines for Atmel keypad driver.
*
*   DATA STRUCTURES
*
*       KP_Instances
*       KP_Sessions
*
*   FUNCTIONS
*
*       Keypad_Dv_Register
*       Keypad_Dv_Unregister
*       Keypad_Dv_Open
*       Keypad_Dv_Close
*       Keypad_Dv_Ioctl
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "services/nu_trace_os_mark.h"

/***********************************/
/* LOCAL FUNCTION PROTOTYPES       */
/***********************************/
static STATUS    Keypad_Dv_Open(VOID *instance_handle,
                                DV_DEV_LABEL labels_list[],
                                INT labels_cnt,
                                VOID* *session_handle);
static STATUS    Keypad_Dv_Close(VOID *handle_ptr);
static STATUS    Keypad_Dv_Ioctl(VOID *session_handle,
                                 INT cmd,
                                 VOID *data,
                                 INT length);

extern STATUS      KPT_Tgt_Initialize(KP_INSTANCE_HANDLE *kp_i_handle);
extern STATUS      KPT_Tgt_Close(KP_INSTANCE_HANDLE *kp_i_handle);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern STATUS Keypad_Tgt_Pwr_Set_State(VOID *i_handle, PM_STATE_ID *state);
#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

/*************************************************************************
*
*   FUNCTION
*
*       Keypad_Dv_Register
*
*   DESCRIPTION
*
*       This function registers the keypad device.
*
*   INPUTS
*
*       key                                 Key
*       kp_i_handle                         pointer to instance structure
*
*   OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully
*
*************************************************************************/
STATUS Keypad_Dv_Register (const CHAR *key, KP_INSTANCE_HANDLE *kp_i_handle)
{
    STATUS               status = NU_SUCCESS;
    DV_DEV_LABEL         kp_labels[3] = {{KEYPAD_LABEL}};
    DV_DRV_FUNCTIONS     kp_drv_funcs;
    INT                  kp_labels_cnt = 1;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif  

    /* Suppress warnings */
    NU_UNUSED_PARAM(key);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /********************************/
    /* INITIALIZE AS POWER DEVICE   */
    /********************************/
    status = PMI_Device_Initialize(&(kp_i_handle->pmi_dev), key, kp_labels,
                                   &kp_labels_cnt, KP_UII_BASE);

    if (status == NU_SUCCESS)
    {
        /* Setup the power device */
        PMI_Device_Setup(kp_i_handle->pmi_dev, &Keypad_Tgt_Pwr_Set_State, KP_POWER_BASE,
                         KP_TOTAL_STATE_COUNT, &(kp_i_handle->dev_id), (VOID*)kp_i_handle);

    }

#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

    /*********************************/
    /* REGISTER WITH DEVICE MANAGER  */
    /*********************************/

    /* Populate function pointers */
    kp_drv_funcs.drv_open_ptr  = &Keypad_Dv_Open;
    kp_drv_funcs.drv_close_ptr = &Keypad_Dv_Close;
    kp_drv_funcs.drv_read_ptr  = NU_NULL;
    kp_drv_funcs.drv_write_ptr = NU_NULL;
    kp_drv_funcs.drv_ioctl_ptr = &Keypad_Dv_Ioctl;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))       
    TCCT_Schedule_Lock();
#endif
    /* Register this device with the Device Manager */
    status = DVC_Dev_Register((VOID*)kp_i_handle, kp_labels,
                              kp_labels_cnt, &kp_drv_funcs,
                              &(kp_i_handle->dev_id));
                              
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                  
    /* Get default power state */
    init_pwr_state = PMI_STATE_GET(kp_i_handle->pmi_dev); 
                             
    /* Trace log */
    T_DEV_NAME((CHAR*)key, init_pwr_state, KP_TOTAL_STATE_COUNT, kp_i_handle->dev_id);
    
    TCCT_Schedule_Unlock(); 
#endif    
                              
    /* Return completion status of the service. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Keypad_Dv_Unregister
*
*   DESCRIPTION
*
*       This function unregisters the keypad device.
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
*       [Error Code]                        Error codes from device
*                                           manager requests
*
*************************************************************************/
STATUS  Keypad_Dv_Unregister   (const CHAR * key, INT startstop, DV_DEV_ID dev_id)
{
    STATUS               status = NU_SUCCESS;
    KP_INSTANCE_HANDLE  *inst_handle;
    INT                  int_level;


    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(startstop);

    /* Unregister the device with Device Manager */
    status = DVC_Dev_Unregister(dev_id, (VOID**)&inst_handle);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    if(status == NU_SUCCESS)
    {
        status = PMI_Device_Unregister(inst_handle->pmi_dev);
    }

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* Set dev id to a negative value */
    inst_handle->dev_id = -1;

    /* Disable interrupts before clearing shared variable */
    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Clear the flag in instance handle */
    inst_handle->register_flag = 0;

    /* Restore interrupts to previous level */
    NU_Local_Control_Interrupts(int_level);

    /* Return completion status of the service. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Keypad_Dv_Open
*
*   DESCRIPTION
*
*       This function creates a session handle.
*
*   INPUTS
*
*       instance_handle                     Instance handle of the driver
*       labels_list                         Access mode (label) of open
*       labels_cnt                          Number of labels
*       session_handle                      Session handle of the driver
*
*   OUTPUTS
*
*       NU_SUCCESS                          Successful completion
*       KP_SESSION_NOT_FOUND                Cannot find a session
*
*************************************************************************/
static STATUS Keypad_Dv_Open (VOID *instance_handle, DV_DEV_LABEL labels_list[],
                              INT labels_cnt, VOID* *session_handle)
{
    STATUS              status = NU_SUCCESS;
    INT                 int_level;
    KP_SESSION_HANDLE   *ses_ptr = (KP_SESSION_HANDLE*)-1;
    KP_INSTANCE_HANDLE  *kp_i_handle = ((KP_INSTANCE_HANDLE*)instance_handle);
    UINT32              open_mode_requests = 0;
    DV_DEV_LABEL        kp_label = {KEYPAD_LABEL};
    NU_MEMORY_POOL      *sys_pool_ptr;


    /* Get open mode requests from labels */
    if (DVS_Label_List_Contains (labels_list, labels_cnt, kp_label) == NU_SUCCESS)
    {
        open_mode_requests |= KP_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /* Call the Power device open function */
    status = PMI_Device_Open (&open_mode_requests, labels_list, labels_cnt);

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */


    /* If device is already open AND if the open request contains keypad open mode, return an error. */
    if (!((kp_i_handle->device_in_use == NU_TRUE) && (open_mode_requests & KP_OPEN_MODE)))
    {
        /* Disable interrupts */
        int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

        /* Get pointer to system memory pool */
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Allocate a new session */
            status = NU_Allocate_Memory (sys_pool_ptr, (VOID**)&ses_ptr,
                                         sizeof(KP_SESSION_HANDLE), NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                (VOID)memset(ses_ptr, 0, sizeof(KP_SESSION_HANDLE));

                /* Place a pointer to instance handle in session handle */
                ses_ptr->inst_info = instance_handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

                /* If open mode request is power */
                if (open_mode_requests & POWER_OPEN_MODE)
                {
                    ses_ptr->open_modes |= POWER_OPEN_MODE;
                }

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

                /* Request is to open keypad for use. */
                if (open_mode_requests & KP_OPEN_MODE)
                {
                    ses_ptr->open_modes |= KP_OPEN_MODE;

                    /* Set device in use flag to true */
                    kp_i_handle->device_in_use = NU_TRUE;

                    /* If the open mode request is keypad and the previous operation is successful */
                    status = KPT_Tgt_Initialize(kp_i_handle);
                }
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))

                /* If open mode request is PM_UII_MODE */
                if (open_mode_requests & UII_OPEN_MODE)
                {
                    ses_ptr->open_modes |= UII_OPEN_MODE;
                }

#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)) */

                /* Set the return address of the session handle */
                *session_handle = (VOID*)ses_ptr;

                /* If initialization failure deallocate memory */
                if (status != NU_SUCCESS)
                {
                    (VOID)NU_Deallocate_Memory(ses_ptr);
                }

            }
        }

        /* Restore interrupts to previous level */
        NU_Local_Control_Interrupts(int_level);
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Keypad_Dv_Close
*
*   DESCRIPTION
*
*       This function deletes the session handle.
*
*   INPUTS
*
*       session_handle                      Session handle pointer
*
*   OUTPUTS
*
*       NU_SUCCESS                          Successful completion
*
*************************************************************************/
static STATUS Keypad_Dv_Close(VOID *session_handle)
{
    STATUS              status = NU_SUCCESS;
    KP_SESSION_HANDLE   *kp_s_handle = (KP_SESSION_HANDLE*)session_handle;
    KP_INSTANCE_HANDLE  *kp_i_handle = (kp_s_handle->inst_info);


    /* Check if device is open in keypad mode. */
    if (kp_s_handle->open_modes & KP_OPEN_MODE)
    {
        /* Call close function of keypad. */
        KPT_Tgt_Close(kp_i_handle);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

        status = PMI_Device_Close((kp_i_handle->pmi_dev));

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

        /* Set device is closed */
        kp_i_handle->device_in_use = NU_FALSE;

        /* Mark the session as free. */
        kp_s_handle->open_modes = 0;
    }

    /* Deallocate session memory */
    (VOID)NU_Deallocate_Memory(kp_s_handle);

    /* Return the completion status. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Keypad_Dv_Ioctl
*
*   DESCRIPTION
*
*       This function controls IO operations of the keypad.
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
*       NU_SUCCESS
*
*************************************************************************/
static STATUS  Keypad_Dv_Ioctl(VOID *session_handle, INT cmd, VOID *data, INT length)
{
    STATUS              status = NU_SUCCESS;
    DV_IOCTL0_STRUCT    *ioctl0;
    KP_SESSION_HANDLE   *kp_s_handle = (KP_SESSION_HANDLE *)session_handle;
    DV_DEV_LABEL        kp_label = {KEYPAD_LABEL};

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE      pmi_dev = ((kp_s_handle->inst_info)->pmi_dev);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */


    /* Process command */
    switch (cmd)
    {
        case DV_IOCTL0:

            if (length == sizeof(DV_IOCTL0_STRUCT))
            {
                ioctl0 = data;
                status = DV_IOCTL_INVALID_MODE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                status = PMI_Device_Ioctl(pmi_dev, cmd, data, length, (kp_s_handle->inst_info),
                                          kp_s_handle->open_modes);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

                if (status != NU_SUCCESS)
                {
                    /* If the mode requested is supported and if the session was opened for that mode */
                    if ((DV_COMPARE_LABELS (&(ioctl0->label), &kp_label)) &&
                        (kp_s_handle->open_modes & KP_OPEN_MODE))
                    {
                        ioctl0->base = KP_IOCTL_BASE;
                        status = NU_SUCCESS;
                    }
                }
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }

            break;

        default:
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            /* Call the PMI IOCTL function for Power and UII IOCTLs */
            status = PMI_Device_Ioctl(pmi_dev, cmd, data, length, (kp_s_handle->inst_info),
                                      kp_s_handle->open_modes);

#else

            status = NU_UNAVAILABLE;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

            break;
    }

    return (status);
}
