/*************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
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
*       touchpanel_dv_interface.c
*
*   COMPONENT
*
*       TOUCHPANEL                          - Touchpanel Library
*
*   DESCRIPTION
*
*       This file contains the generic Touchpanel DV interface
*       library functions.
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "services/nu_trace_os_mark.h"

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/

/* External function prototypes */
extern VOID Touchpanel_Tgt_Initialize(TOUCHPANEL_INSTANCE_HANDLE *inst_handle);
extern VOID Touchpanel_Tgt_Shutdown (TOUCHPANEL_INSTANCE_HANDLE *inst_handle);
extern VOID Touchpanel_Tgt_Enable(VOID);
extern VOID Touchpanel_Tgt_Disable(VOID);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern STATUS Touchpanel_Tgt_Pwr_Set_State (VOID *inst_handle, PM_STATE_ID *state);
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
extern STATUS Touchpanel_Tgt_Pwr_Notify_Park1(VOID *instance_handle);
extern STATUS Touchpanel_Tgt_Pwr_Notify_Park2(VOID *instance_handle);
extern STATUS Touchpanel_Tgt_Pwr_Notify_Resume1(VOID *instance_handle);
extern STATUS Touchpanel_Tgt_Pwr_Notify_Resume2(VOID *instance_handle);
extern STATUS Touchpanel_Tgt_Pwr_Resume_Complete(VOID *instance_handle);
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)
extern STATUS Touchpanel_Tgt_Pwr_Hibernate_Restore (TOUCHPANEL_SESSION_HANDLE *    session_handle);
#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Dv_Register
*
*   DESCRIPTION
*
*       This function registers touchpanel with Device Manager.
*
*   INPUTS
*
*       key                                 Key
*       startstop                           Start or stop flag
*       dev_id                              Returned Device ID
*       instance_handle                     Touchpanel instance structure
*
*   OUTPUTS
*
*       status                              - NU_SUCCESS or
*                                             TOUCHPANEL_REGISTRY_ERROR
*
*************************************************************************/
STATUS Touchpanel_Dv_Register(const CHAR *key, INT startstop, DV_DEV_ID *dev_id,
                              TOUCHPANEL_INSTANCE_HANDLE *instance_handle)
{
    STATUS               status = NU_SUCCESS;
    DV_DEV_LABEL         all_labels[TOUCHPANEL_MAX_LABEL_CNT]= {{TOUCHPANEL_LABEL}};
    INT                  all_label_cnt = 1;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif    

    /* DVR function pointers */
    DV_DRV_FUNCTIONS touchpanel_drv_funcs =
    {
        Touchpanel_Dv_Open,
        Touchpanel_Dv_Close,
        NU_NULL,
        NU_NULL,
        Touchpanel_Dv_Ioctl
    };


    /* Suppress warnings */
    NU_UNUSED_PARAM(startstop);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /********************************/
    /* INITIALIZE AS POWER DEVICE   */
    /********************************/
    status = PMI_Device_Initialize(&(instance_handle->pmi_dev), key,
                                   all_labels, &all_label_cnt, TOUCHPANEL_UII_BASE);

    if (status == NU_SUCCESS)
    {
        /* Setup as power device. */
        PMI_Device_Setup(instance_handle->pmi_dev, &Touchpanel_Tgt_Pwr_Set_State,
                         TOUCHPANEL_POWER_BASE, TOUCHPANEL_TOTAL_STATE_COUNT,
                         &instance_handle->dev_id, (VOID*)instance_handle);
    }
    
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

        /* Perform DVFS related setup */
        PMI_DVFS_Setup(instance_handle->pmi_dev, key, (VOID*)instance_handle,
                       &Touchpanel_Tgt_Pwr_Notify_Park1, &Touchpanel_Tgt_Pwr_Notify_Park2,
                       &Touchpanel_Tgt_Pwr_Notify_Resume1, &Touchpanel_Tgt_Pwr_Notify_Resume2,
                       &Touchpanel_Tgt_Pwr_Resume_Complete);
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
    
#endif

    /*********************************/
    /* REGISTER WITH DEVICE MANAGER  */
    /*********************************/
    if (status == NU_SUCCESS)
    {
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))   
        TCCT_Schedule_Lock();
#endif
        /* Register this device with the Device Manager. */
        status = DVC_Dev_Register((VOID*)instance_handle, all_labels,
                                  all_label_cnt, &touchpanel_drv_funcs,
                                  &instance_handle->dev_id);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                   
        /* Get default power state */
        init_pwr_state = PMI_STATE_GET(instance_handle->pmi_dev);
                                  
        /* Trace log */
        T_DEV_NAME((CHAR*)key, init_pwr_state, TOUCHPANEL_TOTAL_STATE_COUNT, instance_handle->dev_id);
     
        TCCT_Schedule_Unlock();
#endif        
    } 
    
    /* Return the device ID. */
    if (status == NU_SUCCESS)
    {
        *dev_id = instance_handle->dev_id;
    }
    else
    {
        status = TOUCHPANEL_REGISTRY_ERROR;
    }

    /* Return completion status of the service. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Dv_Unregister
*
*   DESCRIPTION
*
*       This function unregisters the Touchpanel controller.
*
*   INPUTS
*
*       key                                 Path to registry
*       startstop                           Option to Register or Unregister
*       dev_id                              Device ID
*
*   OUTPUTS
*
*       NU_SUCCESS                          Successful completion
*       [Error Code]                        Error codes from device
*                                           manager requests
*
*************************************************************************/
STATUS Touchpanel_Dv_Unregister(const CHAR *key, INT startstop, DV_DEV_ID dev_id)
{
    STATUS                                   status = NU_SUCCESS;
    TOUCHPANEL_INSTANCE_HANDLE *inst_handle;

    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(startstop);

    /*****************************************/
    /* UNREGISTER DEVICE WITH DEVICE MANAGER */
    /*****************************************/
    status = DVC_Dev_Unregister(dev_id, (VOID**)&inst_handle);

    /******************************************/
    /* PLACE DEVICE IN KNOWN, LOW-POWER STATE */
    /******************************************/
    if ((status == NU_SUCCESS) && (inst_handle != NU_NULL))
    {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

        if(status == NU_SUCCESS)
        {
            status = PMI_Device_Unregister(inst_handle->pmi_dev);
        }

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

        /***********************************/
        /* FREE THE INSTANCE HANDLE */
        /***********************************/
        NU_Deallocate_Memory ((VOID*) inst_handle);
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Dv_Open
*
*   DESCRIPTION
*
*       This function opens the device and creates a session handle
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
*       TOUCHPANEL_ALREADY_OPEN             Session already open
*
*************************************************************************/
STATUS Touchpanel_Dv_Open(VOID *instance_handle, DV_DEV_LABEL labels_list[],
                         INT labels_cnt, VOID* *session_handle)
{
    STATUS                  status = NU_SUCCESS;
    TOUCHPANEL_SESSION_HANDLE *ses_ptr;
    TOUCHPANEL_INSTANCE_HANDLE *inst_handle = (TOUCHPANEL_INSTANCE_HANDLE*)instance_handle;
    UINT32                  open_mode_requests = 0UL;
    DV_DEV_LABEL            touchpanel_label = {TOUCHPANEL_LABEL};
    VOID*                   pointer;

    /* Check if the label list contains the touchpanel label */
    if (DVS_Label_List_Contains (labels_list, labels_cnt, touchpanel_label) == NU_SUCCESS)
    {
        open_mode_requests |= TOUCHPANEL_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    status = PMI_Device_Open(&open_mode_requests, labels_list, labels_cnt);
#endif

    /* If device is already open AND if the open request contains touchpanel open mode, return an error. */
    if (!((inst_handle->dev_in_use == NU_TRUE) && (open_mode_requests & TOUCHPANEL_OPEN_MODE)))
    {
        /* Allocate a new session */
        status = NU_Allocate_Memory (&System_Memory, &pointer,
                    sizeof(TOUCHPANEL_SESSION_HANDLE), NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            (VOID)memset(pointer, 0, sizeof(TOUCHPANEL_SESSION_HANDLE));

            ses_ptr = (TOUCHPANEL_SESSION_HANDLE*)pointer;

            /* Place a pointer to instance handle in session handle */
            ses_ptr->inst_info = instance_handle;

            /* Assigning the open mode requests. */
            ses_ptr->open_modes |= open_mode_requests;

            /* Request is to open touchpanel for use. */
            if (open_mode_requests & TOUCHPANEL_OPEN_MODE)
            {
                /* Set device in use flag to true */
                inst_handle->dev_in_use = NU_TRUE;
                Touchpanel_Tgt_Initialize(inst_handle);
            }

            /* Set the return address of the session handle */
            *session_handle = (VOID*)ses_ptr;

        }
    }
    else
    {
        /* Already opened in Touchpanel mode */
        status = TOUCHPANEL_ALREADY_OPEN;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Dv_Close
*
*   DESCRIPTION
*
*       This function closes the device and deletes the session handle
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
STATUS Touchpanel_Dv_Close(VOID *session_handle)
{
    STATUS                     status = NU_SUCCESS;
    TOUCHPANEL_SESSION_HANDLE  *tpc_handle;
    TOUCHPANEL_INSTANCE_HANDLE *inst_handle;

    /* If a valid session, then close it */
    if (session_handle != NU_NULL)
    {
        /* Initialize local variables */
        tpc_handle = (TOUCHPANEL_SESSION_HANDLE *)session_handle;
        inst_handle = tpc_handle->inst_info;

        /* Check if device is open in touchpanel mode. */
        if (tpc_handle->open_modes & TOUCHPANEL_OPEN_MODE)
        {
            /* Call shutdown function of touch panel. */
            Touchpanel_Tgt_Shutdown(inst_handle);

            /* Set device is closed */
            inst_handle->dev_in_use = NU_FALSE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            status = PMI_Device_Close(inst_handle->pmi_dev);
#endif
        }

        /* Free the session */
        NU_Deallocate_Memory (tpc_handle);
    }

    /* Return the completion status. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Dv_Ioctl
*
*   DESCRIPTION
*
*       This function provides IOCTL functionality
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
*       DV_INVALID_INPUT_PARAMS             Specified IOCTL code is unknown
*
*************************************************************************/
STATUS Touchpanel_Dv_Ioctl(VOID *session_handle, INT cmd, VOID *data, INT length)
{
    STATUS              status = NU_SUCCESS;
    DV_IOCTL0_STRUCT    *ioctl0;

    TOUCHPANEL_SESSION_HANDLE *tpc_handle = (TOUCHPANEL_SESSION_HANDLE *)session_handle;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    TOUCHPANEL_INSTANCE_HANDLE *inst_handle = tpc_handle->inst_info;
#endif

    DV_DEV_LABEL            touchpanel_label = {TOUCHPANEL_LABEL};

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
                status = PMI_Device_Ioctl(inst_handle->pmi_dev, cmd, data, length, inst_handle,
                                          tpc_handle->open_modes);
#endif
                if (status != NU_SUCCESS)
                {
                    /* If the mode requested is supported and if the session was opened for that mode */
                    if (DV_COMPARE_LABELS (&(ioctl0->label), &touchpanel_label) &&
                        (tpc_handle->open_modes & TOUCHPANEL_OPEN_MODE))
                    {
                        ioctl0->base = IOCTL_TOUCHPANEL_BASE;
                        status = NU_SUCCESS;
                    }
                }
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }

            break;
            
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)
                        
            case (IOCTL_TOUCHPANEL_BASE + TOUCHPANEL_PWR_HIB_RESTORE):
                        
                /* Call hibernate restore for touchpanel session. */
                status = Touchpanel_Tgt_Pwr_Hibernate_Restore(tpc_handle);
                    
                break;
                        
#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

        default:

            status = PMI_Device_Ioctl((inst_handle->pmi_dev), cmd, data, length, inst_handle,
                                      tpc_handle->open_modes);
#else
            status = DV_INVALID_INPUT_PARAMS;
#endif
            break;
    }

    return (status);
}
