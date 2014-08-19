/*************************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       sdio_hcd_dv_interface.c
*
* COMPONENT
*
*       Nucleus SD/MMC Card Driver : Hardware layer.
*
* DESCRIPTION
*
*       This file contains the implementation of the Nucleus SD/MMC
*       Card Driver services.
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "connectivity/ctsystem.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "services/nu_trace_os_mark.h"

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
extern STATUS Sdio_Hcd_Tgt_Initialize(VOID *target_info_ptr);
extern STATUS Sdio_Hcd_Tgt_Deinitialize(SDIO_HCD_INSTANCE *instance_handle);
extern STATUS Sdio_Hcd_Tgt_Remove(VOID *target_info_ptr);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

extern VOID      Sdio_Hcd_Tgt_Pwr_Default_State (VOID *inst_handle);
extern STATUS Sdio_Hcd_Tgt_Pwr_Set_State(VOID *inst_handle, PM_STATE_ID *state);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

extern STATUS   Sdio_Hcd_Tgt_Pwr_Notify_Park1(VOID *instance_handle);
extern STATUS   Sdio_Hcd_Tgt_Pwr_Notify_Park2(VOID *instance_handle);
extern STATUS   Sdio_Hcd_Tgt_Pwr_Notify_Resume1(VOID *instance_handle);
extern STATUS   Sdio_Hcd_Tgt_Pwr_Notify_Resume2(VOID *instance_handle);
extern STATUS   Sdio_Hcd_Tgt_Pwr_Notify_Resume3(VOID *instance_handle);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

extern STATUS HCD_Tgt_Pwr_Hibernate_Restore (SDIO_HCD_SESSION *session_handle);

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */


#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

/*************************************************************************
*
*   FUNCTION
*
*       Sdio_Hcd_Dv_Register
*
*   DESCRIPTION
*
*       This function registers the SPISDIO controller
*
*   INPUTS
*
*       VOID            *instance_handle           - Target info
*
*   OUTPUTS
*
*       STATUS          status
*
*************************************************************************/
STATUS Sdio_Hcd_Dv_Register(const CHAR *key, SDIO_HCD_INSTANCE *instance_handle, DV_DEV_ID *dev_id_ptr)
{
    STATUS              status = NU_SUCCESS;
    DV_DEV_LABEL        labels_list[SDIO_HCD_MAX_MODE_NUM] = {{SDIO_HCD_LABEL}};
    INT                 labels_cnt = 1;
    DV_DRV_FUNCTIONS    sdio_hcd_drv_funcs;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif    

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /******************************************/
    /* PLACE DEVICE IN KNOWN, LOW-POWER STATE */
    /******************************************/
    Sdio_Hcd_Tgt_Pwr_Default_State (instance_handle);

    /********************************/
    /* INITIALIZE AS POWER DEVICE   */
    /********************************/
    status = PMI_Device_Initialize(&(instance_handle->pmi_dev), key, labels_list,
                                   &labels_cnt, SDIO_HCD_UII_BASE);

    if (status == NU_SUCCESS)
    {
        /* Setup the power device */
        PMI_Device_Setup(instance_handle->pmi_dev, &Sdio_Hcd_Tgt_Pwr_Set_State, SDIO_HCD_POWER_BASE,
                         SDIO_TOTAL_STATE_COUNT, &(instance_handle->dev_id), (VOID*)instance_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

        /* Perform DVFS related setup */
        PMI_DVFS_Setup(instance_handle->pmi_dev, key, (VOID*)instance_handle,
                       &Sdio_Hcd_Tgt_Pwr_Notify_Park1, &Sdio_Hcd_Tgt_Pwr_Notify_Park2,
                       &Sdio_Hcd_Tgt_Pwr_Notify_Resume1, &Sdio_Hcd_Tgt_Pwr_Notify_Resume2,
                       &Sdio_Hcd_Tgt_Pwr_Notify_Resume3);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
    }

#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

    /*********************************/
    /* REGISTER WITH DEVICE MANAGER  */
    /*********************************/

    /* Populate function pointers */
    sdio_hcd_drv_funcs.drv_open_ptr = &Sdio_Hcd_Dv_Open;
    sdio_hcd_drv_funcs.drv_close_ptr = &Sdio_Hcd_Dv_Close;
    sdio_hcd_drv_funcs.drv_read_ptr = NU_NULL;
    sdio_hcd_drv_funcs.drv_write_ptr = NU_NULL;
    sdio_hcd_drv_funcs.drv_ioctl_ptr = &Sdio_Hcd_Dv_Ioctl;

    if (status == NU_SUCCESS)
    {
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))       
        TCCT_Schedule_Lock();
#endif        
        /********************************/
        /* REGISTER WITH DM             */
        /********************************/
        /* Register this device with the Device Manager */
        status = DVC_Dev_Register((VOID*)instance_handle, labels_list, labels_cnt,
                                  &sdio_hcd_drv_funcs, &(instance_handle->dev_id));
        *dev_id_ptr = (instance_handle->dev_id);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                   
        /* Get default power state */
        init_pwr_state = PMI_STATE_GET(instance_handle->pmi_dev);
        
        /* Trace log */
        T_DEV_NAME((CHAR*)key, init_pwr_state, SDIO_TOTAL_STATE_COUNT, instance_handle->dev_id);
       
        TCCT_Schedule_Unlock();
#endif        
    } 
    
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SDIO_HCD_Dv_Unregister
*
*   DESCRIPTION
*
*       This function unregisters the SPISDIO controller
*
*   INPUTS
*
*       DV_DEV_ID      dev_id               - Device ID
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS or error code
*
*************************************************************************/
STATUS Sdio_Hcd_Dv_Unregister (const CHAR *key, INT startstop, DV_DEV_ID dev_id)
{
    SDIO_HCD_INSTANCE *instance_handle;
    STATUS status = NU_SUCCESS;


    /* Unregister the device with Device Manager */
    status = DVC_Dev_Unregister(dev_id, (VOID**)&instance_handle);

    if (status == NU_SUCCESS)
    {
        status = Sdio_Hcd_Tgt_Remove((instance_handle->target_info_ptr));

        if (status == NU_SUCCESS)
        {
            /* Remove from power */
            instance_handle->device_in_use = NU_FALSE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            /* Place the device in low-power state */
            Sdio_Hcd_Tgt_Pwr_Default_State(instance_handle);

            status = PMI_Device_Unregister(instance_handle->pmi_dev);

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

            /* Deallocate memory */
            status = NU_Deallocate_Memory((VOID*)instance_handle);
        }
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Sdio_Hcd_Dv_Open
*
*   DESCRIPTION
*
*       This function creates a session handle
*
*   INPUTS
*
*       VOID          *instance_handle      - Instance handle of the driver
*       DV_DEV_LABEL  labels_list[]         - Access mode (label) of open
*       INT           labels_cnt            - Number of labels
*       VOID*         *session_handle       - Session handle
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS or error code
*
*************************************************************************/
STATUS Sdio_Hcd_Dv_Open(VOID *instance_handle, DV_DEV_LABEL labels_list[], INT labels_cnt,
                                   VOID* *session_handle)
{
    STATUS              status = DV_INVALID_INPUT_PARAMS;
    DV_DEV_LABEL        sdio_hcd_label = {SDIO_HCD_LABEL};
    UINT32              sdio_hcd_modes = 0;
    SDIO_HCD_INSTANCE   *inst_handle = (SDIO_HCD_INSTANCE*)instance_handle;

    /* Search for valid label */
    if (DVS_Label_List_Contains (labels_list, labels_cnt, sdio_hcd_label) == NU_SUCCESS)
    {
        sdio_hcd_modes |= SDIO_HCD_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /* Call the Power device open function */
    status = PMI_Device_Open (&sdio_hcd_modes, labels_list, labels_cnt);

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* If device is already open AND if the open request contains serial mode, return a error. */
    if (!((inst_handle->device_in_use == NU_TRUE) && (sdio_hcd_modes & SDIO_HCD_OPEN_MODE)))
    {
        /* Save instance_handle as my session handle */
        *session_handle = (VOID*)inst_handle;

        /* Increment Open count */
        inst_handle->open_cnt++;

        /* Save mode */
        inst_handle->mode |= sdio_hcd_modes;

        if (sdio_hcd_modes & SDIO_HCD_OPEN_MODE)
        {
            /* Initialize device. */
            status = Sdio_Hcd_Tgt_Initialize((inst_handle->target_info_ptr));

            /* Increment initialize count if host controller is initialize without error */
            if(status == NU_SUCCESS)
            {
                /* Register with DVFS */
                inst_handle->device_in_use = NU_TRUE;
            }
        }
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SDIO_HCD_Dv_Close
*
*   DESCRIPTION
*
*       This function deletes the session handle
*
*   INPUTS
*
*       VOID   *handle_ptr                       - Session handle of the device
*
*   OUTPUTS
*
*       STATUS status                            - NU_SUCCESS or error code
*
*************************************************************************/
STATUS Sdio_Hcd_Dv_Close(VOID *instance_handle)
{
    STATUS              status = NU_SUCCESS;
    SDIO_HCD_INSTANCE   *inst_handle = (SDIO_HCD_INSTANCE*)instance_handle;


    /* Decrement Open counter if required. */
    if (inst_handle->open_cnt > 0)
    {
        inst_handle->open_cnt--;
    }

    if (inst_handle->open_cnt < 1)
    {
        /* Close */
        status = Sdio_Hcd_Tgt_Deinitialize(inst_handle);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            status = PMI_Device_Close((inst_handle->pmi_dev));

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SDIO_HCD_Dv_Ioctl
*
*   DESCRIPTION
*
*       This function provides IOCTL functionality.
*
*   INPUTS
*
*       VOID            *sess_handle_ptr     - Session handle of the driver
*       UINT32          cmd                 - Ioctl command
*       VOID            *data               - Ioctl data pointer
*       UINT32          length              - Ioctl length
*
*   OUTPUTS
*
*       STATUS   status                     - NU_SUCCESS
*                                           - or error code
*
*************************************************************************/
STATUS Sdio_Hcd_Dv_Ioctl(VOID *sess_handle_ptr, INT ioctl_cmd, VOID *data, INT length)
{

    DV_IOCTL0_STRUCT    *ioctl0;
    STATUS              status = DV_INVALID_INPUT_PARAMS;
    DV_DEV_LABEL        sdio_hcd_label = {SDIO_HCD_LABEL};
    SDIO_HCD_SESSION    *session_handle = ((SDIO_HCD_SESSION*) sess_handle_ptr);
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE          pmi_dev = session_handle->pmi_dev;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */


    /* Process command */
    switch (ioctl_cmd)
    {
        case DV_IOCTL0:

            if (length == sizeof(DV_IOCTL0_STRUCT))
            {
                if (data != NU_NULL)
                {
                    ioctl0 = (DV_IOCTL0_STRUCT*)data;
                    status = DV_IOCTL_INVALID_MODE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                    status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, data, length, sess_handle_ptr,
                                              session_handle->mode);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

                    /* Check if label matched */
                    if ((DV_COMPARE_LABELS(&(ioctl0->label), &sdio_hcd_label)) &&
                        (session_handle->mode & SDIO_HCD_OPEN_MODE))
                    {
                        ioctl0->base = SDIO_HCD_BASE;
                        status = NU_SUCCESS;
                        break;
                    }
                }
                else
                {
                    status = DV_INVALID_INPUT_PARAMS;
                }
            }
            break;

        case (SDIO_HCD_BASE + SDIO_GET_MW_CONFIG_PATH):

            if (data != NU_NULL)
            {
                status = NU_SUCCESS;
                if( strlen(session_handle->key) < length )
                { 
                   (VOID) strncpy((CHAR*) data, session_handle->key,length);
                   (VOID) strncat((CHAR*) data, "/mw_settings",length);
                }else
                {
                    status = DV_INVALID_INPUT_PARAMS;
                }
            }

            break;

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))

        case (SDIO_HCD_BASE + HCD_PWR_HIB_RESTORE):

            /* Call hibernate restore for HCD session. */
            status = HCD_Tgt_Pwr_Hibernate_Restore(session_handle);

            break;
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE)) */

        default:

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, data, length, sess_handle_ptr,
                                      session_handle->mode);

#else

            status = DV_INVALID_INPUT_PARAMS;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

            break;
    }

    return (status);
}
