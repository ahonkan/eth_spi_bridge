/*************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   DESCRIPTION
*
*       This file contains generic interface specifications for Power -
*       aware device drivers
*
*************************************************************************/

/**********************************/
/* INCLUDE FILES                  */
/**********************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
static STATUS PMI_DVFS_Notify(VOID *pmi_dev, PM_DVFS_NOTIFY_TYPE type);
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

/*************************************************************************
*
*   FUNCTION
*
*       PMI_Device_Setup
*
*   DESCRIPTION
*
*       This function performs basic device setup for device power-aware
*       capabilities.
*
*   INPUTS
*
*       pmi_dev
*       set_state_func
*       power_base
*       max_power_states
*       *dev_id
*       *instance_handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID PMI_Device_Setup(PMI_DEV_HANDLE pmi_dev, PMI_DRV_SET_STATE_FN set_state_func,
                      INT power_base, INT max_power_states, DV_DEV_ID *dev_id,
                      VOID *instance_handle)
{
    PMI_DEV *pmi_dev_entry;

    if ((dev_id != NU_NULL) && (pmi_dev != NU_NULL) && (set_state_func != NU_NULL) &&
        (power_base > 0) && (max_power_states > 0))
    {
        pmi_dev_entry = (PMI_DEV*)pmi_dev;

        /* Initialize pmi_dev structure */
        pmi_dev_entry->dev_set_state = set_state_func;
        pmi_dev_entry->power_base = power_base;
        pmi_dev_entry->max_power_states = max_power_states;
        pmi_dev_entry->dev_id = dev_id;

        /* Set the state of the device to its default power state */
        (VOID)pmi_dev_entry->dev_set_state(instance_handle, &pmi_dev_entry->def_pwr_state);
    }
}

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       PMI_DVFS_Setup
*
*   DESCRIPTION
*
*       This function performs basic device setup related to DVFS.
*       If the device is affected by OP changes, callback functions are
*       registered here.
*
*   INPUTS
*
*       pmi_dev
*       *key
*       *instance_handle
*       park_pre_event_set
*       park_post_event_set
*       resume_pre_event_set
*       resume_post_event_set
*       resume_complete
*
*   OUTPUTS
*
*       status
*
*************************************************************************/
STATUS PMI_DVFS_Setup(PMI_DEV_HANDLE pmi_dev, const CHAR *key, VOID *instance_handle,
                      PMI_DRV_PARK_RESUME_FN park_pre_event_set,
                      PMI_DRV_PARK_RESUME_FN park_post_event_set,
                      PMI_DRV_PARK_RESUME_FN resume_pre_event_set,
                      PMI_DRV_PARK_RESUME_FN resume_post_event_set,
                      PMI_DRV_PARK_RESUME_FN resume_complete)
{
    STATUS     reg_status = NU_SUCCESS;
    STATUS     status = NU_SUCCESS;
    STATUS     pm_status = NU_SUCCESS;
    UINT8      op_pt_cnt;
    PMI_DEV    *pmi_dev_entry;
    PMI_DVFS   *pmi_dvfs_ptr;

    if (pmi_dev)
    {
        pmi_dev_entry = (PMI_DEV*)pmi_dev;
        pmi_dvfs_ptr = &(pmi_dev_entry->pmi_dvfs);

        /* Obtain all MPL information from the registry */
        if(reg_status == NU_SUCCESS)
        {
            reg_status = REG_Get_UINT32_Value (key, "/mpl_settings/ref_freq", (UINT32*)&(pmi_dvfs_ptr->mpl_ref_frequency));
        }
        if(reg_status == NU_SUCCESS)
        {
            reg_status = REG_Get_UINT32_Value (key, "/mpl_settings/ref_park", (UINT32*)&(pmi_dvfs_ptr->mpl_ref_park));                                               
        }
        if(reg_status == NU_SUCCESS)
        {
            reg_status = REG_Get_UINT32_Value (key, "/mpl_settings/ref_resume", (UINT32*)&(pmi_dvfs_ptr->mpl_ref_resume));
        }
        if(reg_status == NU_SUCCESS)
        {
            reg_status = REG_Get_UINT32_Value (key, "/mpl_settings/ref_duration", (UINT32*)&(pmi_dvfs_ptr->mpl_ref_duration));
        }
        
        if (reg_status == NU_SUCCESS)
        {
        
            /* Device specific function that will run before the event group is changed */
            pmi_dvfs_ptr->park_pre_evt_set = park_pre_event_set;

            /* Device specific function that will run after the event group is changed */
            pmi_dvfs_ptr->park_post_evt_set = park_post_event_set;

            /* Device specific function that will run before the event group is changed */
            pmi_dvfs_ptr->res_pre_evt_set = resume_pre_event_set;

            /* Device specific function that will run after the event group is changed */
            pmi_dvfs_ptr->res_post_evt_set = resume_post_event_set;

            /* Device specific function that will run to complete resume process */
            pmi_dvfs_ptr->res_complete = resume_complete;

            /********************************/
            /* REGISTER WITH DVFS           */
            /********************************/
            
            /* Populate the pmi_dvfs_ptr with the instance handle */
            pmi_dvfs_ptr->instance_handle = instance_handle;
            
            /* Register with DVFS Services */
            pm_status = NU_PM_DVFS_Register(&(pmi_dvfs_ptr->pmi_dvfs_handle),
                                            (VOID*)(pmi_dev),
                                            &PMI_DVFS_Notify);

            /* If DVFS register fails */
            if ((pm_status != NU_SUCCESS) && (pm_status != PM_ALREADY_REGISTERED))
            {
                /* Get the number of operating points */
                pm_status = NU_PM_Get_OP_Count (&op_pt_cnt);

                if (pm_status == NU_SUCCESS)
                {
                    /* Request highest operating point */
                    pm_status = NU_PM_Request_Min_OP((op_pt_cnt - 1), &(pmi_dev_entry->min_op_request_handle));
                }
            }
        }

        if ((pm_status != NU_SUCCESS) || (reg_status != NU_SUCCESS))
        {
            status = PMI_DEV_ERROR;
        }
    }
    else
    {
        status = PMI_DEV_ERROR;
    }

    return (status);
}
#endif

/*************************************************************************
*
*   FUNCTION
*
*       PMI_Device_Initialize
*
*   DESCRIPTION
*
*       This function performs initialization required for Power-Aware
*       device drivers
*
*   INPUTS
*
*       *pmi_dev
*       *key
*       all_device_labels[]
*       total_label_cnt
*       uii_base
*
*   OUTPUTS
*
*       status
*
*************************************************************************/
STATUS PMI_Device_Initialize(PMI_DEV_HANDLE *pmi_dev, const CHAR *key,
                             DV_DEV_LABEL all_device_labels[], INT *total_label_cnt,
                             INT uii_base)
{
    STATUS          status = NU_SUCCESS;
    DV_DEV_LABEL    pmi_label = {POWER_CLASS_LABEL};
    PMI_DEV         *pmi_dev_entry;
    NU_MEMORY_POOL  *sys_pool_ptr;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
    STATUS          reg_status = NU_SUCCESS;
    DV_DEV_LABEL    uii_label = {PM_UII_CLASS_LABEL};
    BOOLEAN         uii_dev = NU_FALSE;
#endif

    /* Allocate a block of memory for the power device */
    status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

    if (status == NU_SUCCESS)
    {
        /* Allocate memory for the device structure */
        status = NU_Allocate_Memory(sys_pool_ptr, (VOID *)&pmi_dev_entry, sizeof(PMI_DEV), NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Clear the allocated memory */
            memset(pmi_dev_entry, 0, sizeof(PMI_DEV));

            /* Copy power label */
            status = DVS_Label_Append (all_device_labels, DV_MAX_DEV_LABEL_CNT,
                                       all_device_labels, *total_label_cnt,
                                       &pmi_label, 1);

            /* Update total label count */
            *total_label_cnt += 1;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)

            /* See if device is marked UII */   
            reg_status = REG_Get_Boolean_Value (key, "/uii_dev", &uii_dev);
            
            /* Increase total label count if the device is marked UII */
            if((reg_status == NU_SUCCESS) && (uii_dev == NU_TRUE) && (uii_base > 0))
            {
                /* Copy the UII label if this is a UII device */
                status = DVS_Label_Append (all_device_labels, DV_MAX_DEV_LABEL_CNT,
                                           all_device_labels, *total_label_cnt,
                                           &uii_label, 1);

                /* Update total label count */
                *total_label_cnt += 1;

                /* Update the device UII base in the device structure */
                pmi_dev_entry->pmi_uii.uii_base = uii_base;
            }
#endif /*  (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */

            /* Clear the event group memory */
            memset(&(pmi_dev_entry->pwr_state_evt_grp), 0, sizeof(NU_EVENT_GROUP));

            /* Create an event that we can use to suspend on */
            status = NU_Create_Event_Group(&(pmi_dev_entry->pwr_state_evt_grp), "PMI_EVT_GRP");

            if (status == NU_SUCCESS)
            {
                /* Initialize resume event */
                status = NU_Set_Events(&(pmi_dev_entry->pwr_state_evt_grp), PMI_RESUME_EVT, NU_OR);
            }

            /******************************************/
            /* PLACE DEVICE IN KNOWN, LOW-POWER STATE */
            /******************************************/
            if (status == NU_SUCCESS)
            {
                /* Initialize device state to OFF */
                pmi_dev_entry->current_state = 0;
                pmi_dev_entry->def_pwr_state = 0;

                /* Retrieve the default power state of the device */                                           
                (VOID) REG_Get_UINT32_Value (key, "/tgt_settings/def_pwr_state", (UINT32*)&(pmi_dev_entry->def_pwr_state));    
            }

            /* Return the handle to the device */
            *pmi_dev = (PMI_DEV_HANDLE)pmi_dev_entry;
        }
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       PMI_Device_Unregister
*
*   DESCRIPTION
*
*       This function performs unregistration specific to power devices
*
*   INPUTS
*
*       pmi_dev
*
*   OUTPUTS
*
*       status
*
*************************************************************************/
STATUS PMI_Device_Unregister(PMI_DEV_HANDLE pmi_dev)
{
    STATUS      status = PMI_DEV_ERROR;
    PMI_DEV     *pmi_dev_entry;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
    PMI_DVFS    *pmi_dvfs_ptr;
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


    if (pmi_dev)
    {
        pmi_dev_entry = (PMI_DEV*)pmi_dev;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
        
        pmi_dvfs_ptr = &(pmi_dev_entry->pmi_dvfs);

        /* Check if the device has registered with DVFS and dvfs_handle exists */
        if(pmi_dvfs_ptr->pmi_dvfs_handle)
        {
            /* Update MPL for DVFS */
            (VOID)PMI_DVFS_Update_MPL_Value((VOID*)pmi_dev_entry, PM_NOTIFY_OFF);

            /* Unregister with DVFS */
            status = NU_PM_DVFS_Unregister(pmi_dvfs_ptr->pmi_dvfs_handle);
            
            pmi_dvfs_ptr->pmi_dvfs_handle = NU_NULL;
        }
        
        /* Release min request for DVFS OP */
        if ((pmi_dev_entry->min_op_request_handle) && (status == NU_SUCCESS))
        {
            /* Release minimum request */
            (VOID)NU_PM_Release_Min_OP(pmi_dev_entry->min_op_request_handle);
        }
    
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

        if (status == NU_SUCCESS)
        {
            /* Delete event that we used to suspend on */
            status = NU_Delete_Event_Group(&(pmi_dev_entry->pwr_state_evt_grp));
        }
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       PMI_Device_Open
*
*   DESCRIPTION
*
*       This function opens the power-aware device
*
*   INPUTS
*
*       *mode
*       labels_list[]
*       labels_cnt
*
*   OUTPUTS
*
*       status
*
*************************************************************************/
STATUS PMI_Device_Open(UINT32 *mode, DV_DEV_LABEL labels_list[], INT labels_cnt)
{
    STATUS          status = PMI_DEV_ERROR;
    DV_DEV_LABEL    power_label = {POWER_CLASS_LABEL};
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
    DV_DEV_LABEL    uii_label   = {PM_UII_CLASS_LABEL};
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */

    /* Get open mode requests from labels */
    status = DVS_Label_List_Contains(labels_list, labels_cnt, power_label);
    if (status == NU_SUCCESS)
    {
        *mode |= POWER_OPEN_MODE;
    }

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)

    /* Get open mode requests from labels */
    status = DVS_Label_List_Contains(labels_list, labels_cnt, uii_label);
    if (status == NU_SUCCESS)
    {
        *mode |= UII_OPEN_MODE;
    }

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */

    /* Check if any mode requests were valid */
    if (*mode & (POWER_OPEN_MODE | UII_OPEN_MODE))
    {
        status = NU_SUCCESS;
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       PMI_Device_Close
*
*   DESCRIPTION
*
*       This function closes a power-aware device
*
*   INPUTS
*
*       pmi_dev
*
*   OUTPUTS
*
*       status
*
*************************************************************************/
STATUS PMI_Device_Close(PMI_DEV_HANDLE pmi_dev)
{
    STATUS      status = NU_SUCCESS;
    
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

    PMI_DEV     *pmi_dev_entry;
    PMI_DVFS    *pmi_dvfs_ptr;
    
    if (pmi_dev)
    {
        pmi_dev_entry = (PMI_DEV*)pmi_dev;       
        pmi_dvfs_ptr = &(pmi_dev_entry->pmi_dvfs);

        /* Check if the device has registered with DVFS and dvfs_handle exists */
        if(pmi_dvfs_ptr->pmi_dvfs_handle)
        {
            /* Update MPL for DVFS */
            (VOID)PMI_DVFS_Update_MPL_Value((VOID*)pmi_dev_entry, PM_NOTIFY_OFF);
        }

        /* Check if the device has placed a min OP request */
        if (pmi_dev_entry->min_op_request_handle)
        {
            /* Release minimum request */
            (VOID)NU_PM_Release_Min_OP(pmi_dev_entry->min_op_request_handle);
        }
    }

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       PMI_Device_Ioctl
*
*   DESCRIPTION
*
*       This function controls IO operations for a power-aware driver.
*
*   INPUTS
*
*       pmi_dev
*       cmd
*       *data
*       length
*       inst_handle
*       open_modes
*
*   OUTPUTS
*
*       status
*
*************************************************************************/
STATUS  PMI_Device_Ioctl(PMI_DEV_HANDLE pmi_dev, INT cmd, VOID *data, INT length,
                         VOID *inst_handle, UINT32 open_modes)
{
    STATUS              status = NU_SUCCESS;
    STATUS              pm_status = NU_SUCCESS;
    PMI_DEV             *pmi_dev_entry;
    DV_DEV_LABEL        power_label = {POWER_CLASS_LABEL};
    INT                 power_base;
    PM_STATE_ID         *pm_temp;
    DV_IOCTL0_STRUCT    *ioctl0;
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
    PMI_UII             *pmi_uii;
    INT                 uii_base;
    DV_DEV_LABEL        uii_label = {PM_UII_CLASS_LABEL};
    UINT16              *wd_temp;
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */

    if (pmi_dev)
    {
        pmi_dev_entry = (PMI_DEV*)pmi_dev;       

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
        pmi_uii = &(pmi_dev_entry->pmi_uii);
        uii_base = pmi_uii->uii_base;
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */
        
        power_base = pmi_dev_entry->power_base;

        /* Process command */
        if (DV_IOCTL0 == cmd)
        {
            if (length == sizeof(DV_IOCTL0_STRUCT))
            {
                ioctl0 = data;

                if (DV_COMPARE_LABELS (&(ioctl0->label), &power_label) &&
                    (open_modes & POWER_OPEN_MODE))
                {
                    ioctl0->base = power_base;
                }
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)

                else if (DV_COMPARE_LABELS (&(ioctl0->label), &uii_label) &&
                         (open_modes & UII_OPEN_MODE))
                {
                    ioctl0->base = uii_base;
                }
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */
                else
                {
                    status = DV_IOCTL_INVALID_MODE;
                }
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
        }

        /*******************/
        /* Power Ioctls    */
        /*******************/
        else if ((power_base + POWER_IOCTL_GET_STATE) == cmd)
        {
            if ((length == sizeof(PM_STATE_ID)) && (data != NU_NULL))
            {
                pm_temp = (PM_STATE_ID *)data;
                *pm_temp = pmi_dev_entry->current_state;
            }
        }
        else if ((power_base + POWER_IOCTL_SET_STATE) == cmd)
        {
            if ((length == sizeof(PM_STATE_ID)) && (data != NU_NULL))
            {
                pm_temp = (PM_STATE_ID*)data;

                pm_status = pmi_dev_entry->dev_set_state(inst_handle, pm_temp);
                if (pm_status != NU_SUCCESS)
                {
                    status = DV_INVALID_INPUT_PARAMS;
                }
            }
        }
        else if ((power_base + POWER_IOCTL_GET_STATE_COUNT) == cmd)
        {
            if ((length == sizeof(PM_STATE_ID)) && (data != NU_NULL))
            {
                pm_temp = (PM_STATE_ID *)data;
                *pm_temp = pmi_dev_entry->max_power_states;
            }
        }

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)

        /* UII Ioctls */
        else if ((uii_base + PM_UII_SET_TIMEOUT) == cmd)
        {
            if((length == sizeof(UINT16)) && (data != NU_NULL))
            {
                wd_temp = (UINT16 *)data;

                /* Check if there is an existing watchdog */
                /* It needs to be deleted as a new timeout value has been requested */
                if ((pmi_uii->wd_handle) != NU_NULL)
                {
                    /* Delete the watchdog */
                    (VOID)NU_PM_Delete_Watchdog((pmi_uii->wd_handle));
                }

                /* If timeout is non-zero */
                if (wd_temp != 0)
                {
                    /* Create a new wd with whatever timeout is requested */
                    pm_status = NU_PM_Create_Watchdog(*wd_temp, &(pmi_uii->wd_handle));

                    if (pm_status == NU_SUCCESS)
                    {
                        /* Place timeout value in a session handle variable */
                        pmi_uii->wd_timeout = *wd_temp;

                        /* Register for WD Notification Services */
                        pm_status = NU_PM_Set_Watchdog_Notification((pmi_uii->wd_handle), PM_WD_EXPIRED,
                                                                    *(pmi_dev_entry->dev_id));
                        pm_status = NU_PM_Set_Watchdog_Notification((pmi_uii->wd_handle), PM_WD_NOT_EXPIRED,
                                                                    *(pmi_dev_entry->dev_id));
                        pm_status = NU_PM_Set_Watchdog_Notification((pmi_uii->wd_handle), PM_WD_DELETED,
                                                                    *(pmi_dev_entry->dev_id));
                        if (pm_status == NU_SUCCESS)
                        {
                            status = NU_SUCCESS;
                        }
                        else
                        {
                            status = NU_UNAVAILABLE;
                        }
                    }
                }
            }
        }
        else if ((uii_base + PM_UII_GET_TIMEOUT) == cmd)
        {
            if((length == sizeof(UINT16)) && (data != NU_NULL))
            {
                wd_temp = (UINT16 *)data;
                *wd_temp = pmi_uii->wd_timeout;
            }
        }
        else
        {
            status = NU_UNAVAILABLE;
        }
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */
    }
    else
    {
        status = NU_UNAVAILABLE;
    }
    
    return (status);
}


#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       PMI_Reset_Watchdog
*
*   DESCRIPTION
*
*       This function resets a UII watchdog
*
*   INPUTS
*
*       pmi_dev
*
*   OUTPUTS
*
*       pm_status
*
*************************************************************************/
STATUS PMI_Reset_Watchdog(PMI_DEV_HANDLE pmi_dev)
{
    PMI_UII     *temp_uii;
    STATUS       pm_status = NU_SUCCESS;
    PMI_DEV     *pmi_dev_entry;

    if (pmi_dev)
    {
        pmi_dev_entry = (PMI_DEV*)pmi_dev;
        temp_uii = &(pmi_dev_entry->pmi_uii);

        if (temp_uii->wd_handle != NU_NULL)
        {
            /* Reset the watchdog */
            pm_status = NU_PM_Reset_Watchdog(temp_uii->wd_handle);
        }
    }
    return (pm_status);
}

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */


#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       PMI_DVFS_Update_MPL_Value
*
*   DESCRIPTION
*
*       DVFS notification
*
*   INPUT
*
*       pmi_dev
*       notification
*
*   OUTPUT
*
*       pm_status
*
*************************************************************************/
STATUS PMI_DVFS_Update_MPL_Value(PMI_DEV_HANDLE pmi_dev, PM_DVFS_NOTIFY notification)
{
    STATUS    pm_status = PMI_DEV_ERROR;
    PM_MPL    temp_mpl;
    UINT8     volt_id;
    UINT8     freq_id;
    UINT32    frequency;
    UINT32    temp_var;
    UINT32    scaling_factor;
    PMI_DVFS  *pmi_dvfs;
    PMI_DEV   *pmi_dev_entry;
    
    if (pmi_dev)
    {    
        pmi_dev_entry = (PMI_DEV*)pmi_dev;
        pmi_dvfs = &(pmi_dev_entry->pmi_dvfs);

        pm_status = NU_PM_Get_Current_VF(&freq_id, &volt_id);

        if (pm_status == NU_SUCCESS)
        {
            /* Get current frequency */
            pm_status = NU_PM_Get_Freq_Info(freq_id, &frequency);

            /* Check if frequency is valid */
            if ((pm_status == NU_SUCCESS) && (frequency > 0))
            {
                temp_var = frequency/1000;

                if (temp_var > 0)
                {
                    /* Scale the frequency */
                    scaling_factor = ((pmi_dvfs->mpl_ref_frequency)/temp_var);

                    /* Update the mpl parameters */
                    temp_mpl.pm_park_time   = (pmi_dvfs->mpl_ref_park) * (scaling_factor)/1000;
                    temp_mpl.pm_resume_time = (pmi_dvfs->mpl_ref_resume) * (scaling_factor)/1000;
                    temp_mpl.pm_duration    = pmi_dvfs->mpl_ref_duration;

                    /* Update mpl */
                    pm_status = NU_PM_DVFS_Update_MPL(pmi_dvfs->pmi_dvfs_handle, &temp_mpl, notification);
                }
                else
                {
                    pm_status = PM_UNEXPECTED_ERROR;
                }
            }
        }
    }

    return(pm_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       PMI_Min_OP_Pt_Calc
*
*   DESCRIPTION
*
*       This function computes the minimum DVFS operating point required for
*       proper operation at the given frequency.
*
*   INPUT
*
*       min_freq
*       *min_op_pt
*       pmi_dev
*       *ref_clock
*
*   OUTPUT
*
*       pm_status
*
*************************************************************************/
STATUS PMI_Min_OP_Pt_Calc(PMI_DEV_HANDLE pmi_dev, CHAR *ref_clock, UINT32 min_freq, UINT8 *min_op_pt)
{
    STATUS           pm_status;
    UINT8            max_op_pt_cnt;
    UINT8            highest_op_pt;
    INT              curr_op_pt;
    UINT32           ref_freq;

    /* Get the maximum number of operating points */
    pm_status = NU_PM_Get_OP_Count (&max_op_pt_cnt);

    if (pm_status == NU_SUCCESS)
    {
        /* The highest op pt is one less than the maximum number of op pts */
        highest_op_pt = max_op_pt_cnt - 1;

        /* Loop from highest op pt to lowest */
        for (curr_op_pt = highest_op_pt; curr_op_pt >= 0; curr_op_pt--)
        {
            /* Get additional info */
            pm_status = NU_PM_Get_OP_Specific_Info(curr_op_pt, ref_clock, &ref_freq, sizeof(ref_freq));

            if (pm_status == NU_SUCCESS)
            {
                /* Save operating point */
                *min_op_pt = curr_op_pt;

                /* This device cannot function if the op frequency is smaller than the minimum frequency */
                if (ref_freq < min_freq)
                {
                    /* Update min op pt */
                    *min_op_pt = curr_op_pt + 1;

                    break;
                }
            }
        }
    }

    return (pm_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMI_DVFS_Notify
*
*   DESCRIPTION
*
*       This function will notify the device when to park or to resume
*
*   INPUT
*
*       pmi_dev
*       type
*
*   OUTPUT
*
*       pm_status
*
*************************************************************************/
static STATUS PMI_DVFS_Notify(VOID *pmi_dev, PM_DVFS_NOTIFY_TYPE type)
{
    PMI_DEV     *pmi_dev_entry = (PMI_DEV*)pmi_dev;
    PMI_DVFS    *pmi_dvfs_ptr = &(pmi_dev_entry->pmi_dvfs);
    STATUS      status = NU_SUCCESS;
    STATUS      pm_status = NU_SUCCESS;
    INT         int_level;

    switch (type)
    {
        case PM_PARK:

            if (pmi_dvfs_ptr->park_pre_evt_set != NU_NULL)
            {
                /* Call the Pre - Event set park function */
                status = pmi_dvfs_ptr->park_pre_evt_set(pmi_dvfs_ptr->instance_handle);
            }

            if (status == NU_SUCCESS)
            {
                /* Place the device in park mode */
                pmi_dev_entry->is_parked = NU_TRUE;

                /* Clear if Resume event was set */
                status = NU_Set_Events(&(pmi_dev_entry->pwr_state_evt_grp), PMI_PARKED_EVT, NU_AND);

                if ((status == NU_SUCCESS) && (pmi_dvfs_ptr->park_post_evt_set != NU_NULL))
                {
                    /* Call the Post - Event set park function */
                    status = pmi_dvfs_ptr->park_post_evt_set(pmi_dvfs_ptr->instance_handle);
                }
            }

            if (status == NU_SUCCESS)
            {
                pm_status = NU_SUCCESS;
            }

            break;

        case PM_RESUME:
            
            /* Disable interrupts for critical section */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Resume the device only if the current state is still on. A set state cmd may have turned off
            the device during a park/resume call. The device must not be enabled if its state is off. */
            if (pmi_dev_entry->current_state != POWER_OFF_STATE)
            {
                if (pmi_dvfs_ptr->res_pre_evt_set != NU_NULL)
                {
                    /* Call the Pre - Event Set Resume function */
                    status = pmi_dvfs_ptr->res_pre_evt_set(pmi_dvfs_ptr->instance_handle);
                }

                if (status == NU_SUCCESS)
                {
                    /* Update MPL for USART */
                    (VOID)PMI_DVFS_Update_MPL_Value(pmi_dev, PM_NOTIFY_ON);

                    if (pmi_dvfs_ptr->res_post_evt_set != NU_NULL)
                    {
                        /* Call the Post - Event set resume function */
                        status = pmi_dvfs_ptr->res_post_evt_set(pmi_dvfs_ptr->instance_handle);
                    }
                }
            }

            /* Set resume flag in event group */
            status = NU_Set_Events(&(pmi_dev_entry->pwr_state_evt_grp), PMI_RESUME_EVT, NU_OR);

            if (status == NU_SUCCESS)
            {
                /* The device is resumed */
                pmi_dev_entry->is_parked = NU_FALSE;

                if ((status == NU_SUCCESS) && (pmi_dvfs_ptr->res_complete != NU_NULL))
                {
                    /* Call the final resume function, which sets the device to be resumed */
                    status = pmi_dvfs_ptr->res_complete(pmi_dvfs_ptr->instance_handle);
                }
            }

            if (status == NU_SUCCESS)
            {
                pm_status = NU_SUCCESS;
            }

            /* Restore interrupts to original level. */
            (VOID)NU_Local_Control_Interrupts(int_level);

            break;

        default:

            break;
    }

    return (pm_status);
}
#endif /*  (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
