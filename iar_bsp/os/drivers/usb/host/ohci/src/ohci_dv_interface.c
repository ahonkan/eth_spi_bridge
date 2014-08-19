/**************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       ohci_dv_interface.c
*
*
* COMPONENT
*
*       USB OHCI Controller Driver : Nucleus USB Software.
*
* DESCRIPTION
*
*       This is the platform specific file of Nucleus USB OHCI driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_drivers.h
*       nu_services.h
*
**************************************************************************/

/* ==============  USB Include Files =================================== */

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "services/nu_trace_os_mark.h"

/* =====================  Global data ================================== */

extern NU_MEMORY_POOL       System_Memory;
USBH_OHCI_SESSION_HANDLE    *OHCI_Session_Handle = NU_NULL;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)
USBH_OHCI_INSTANCE_HANDLE *OHCI_Inst_Handle_Array[OHCI_MAX_INSTANCES];
INT                        OHCI_Instance_Index = 0;
#endif
extern STATUS OHCI_Tgt_Pwr_Set_State(VOID *inst_handle, PM_STATE_ID *state);
extern STATUS OHCI_Tgt_PWR_Enable (VOID *sess_handle);
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
extern STATUS   OHCI_Tgt_Pwr_Notify_Park1(VOID *instance_handle);
extern STATUS   OHCI_Tgt_Pwr_Notify_Park2(VOID *instance_handle);
extern STATUS   OHCI_Tgt_Pwr_Notify_Resume1(VOID *instance_handle);
extern STATUS   OHCI_Tgt_Pwr_Notify_Resume2(VOID *instance_handle);
extern STATUS   OHCI_Tgt_Pwr_Notify_Resume3(VOID *instance_handle);
#endif
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)
extern STATUS   OHCI_Tgt_Pwr_Hibernate_Restore (USBH_OHCI_SESSION_HANDLE * session_handle);
extern STATUS   OHCI_Tgt_Pwr_Hibernate_Enter (USBH_OHCI_INSTANCE_HANDLE * inst_handle);
#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */
#endif
extern VOID USBH_TGT_Disable_VBUS(VOID);
extern VOID USBH_TGT_Enable_VBUS(VOID);
/**************************************************************************
*
* FUNCTION
*
*       USBH_OHCI_Register
*
* DESCRIPTION
*
*       This function is called during system initialization to actually
*       initialize the OHCI controller driver.
*       This function actually registers the OHCI driver with device
*       manager and associates an instance handle with it.
*
* INPUTS
*
*       path                                Registery path of OHCI
*                                           parameters.
*       options                             Options specifying start or
*                                           stop.
*       DV_DEV_ID     *dev_id               Returned Device ID
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver has
*                                           been initialized successfully.
*
**************************************************************************/
STATUS USBH_OHCI_Register(const CHAR * key, INT startorstop, DV_DEV_ID *dev_id)
{
    STATUS                    status;
    DV_DEV_LABEL              standard_labels[] = {{USBHHW_LABEL}};
    DV_DEV_LABEL              user_label[1];
    DV_DEV_LABEL              device_labels[OHCI_MAX_LABEL_CNT];
    INT                       device_label_cnt = DV_GET_LABEL_COUNT(standard_labels);
    DV_DRV_FUNCTIONS          usbh_ohci_drv_funcs;
    CHAR                      reg_path[REG_MAX_KEY_LENGTH];
    USBH_OHCI_TGT_INFO        *usbh_ohci_tgt;
    VOID                      (*setup_func)(VOID);
    VOID                      (*cleanup_func)(VOID);
    USBH_OHCI_INSTANCE_HANDLE *inst_handle;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif    

    /********************************/
    /* GET A UNUSED INSTANCE HANDLE */
    /********************************/

    /* Allocate memory for the USBH_OHCI_INSTANCE_HANDLE structure */
    status = NU_Allocate_Memory (&System_Memory, (VOID **)&inst_handle, sizeof (USBH_OHCI_INSTANCE_HANDLE), NU_NO_SUSPEND);
    if (status == NU_SUCCESS)
    {
        /* Zero out allocated space */
        (VOID)memset (inst_handle, 0, sizeof (USBH_OHCI_INSTANCE_HANDLE));

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))
        OHCI_Inst_Handle_Array[OHCI_Instance_Index] = inst_handle;

        /* Ensure index has not overrun */
        if (OHCI_Instance_Index < OHCI_MAX_INSTANCES)
        {
            /* Increment the instance index */
            OHCI_Instance_Index++;
        }

        status = NU_Allocate_Memory (&System_Memory, (VOID **)&inst_handle->config_path,
                                     REG_MAX_KEY_LENGTH, NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            strncpy(inst_handle->config_path, key, REG_MAX_KEY_LENGTH);
        }
#endif
        /* Populate info */
        inst_handle->device_id = DV_INVALID_DEV;

        /* Allocate memory for the TGT_INFO structure */
        status = NU_Allocate_Memory (&System_Memory,
                                    (VOID**)&usbh_ohci_tgt,
                                    sizeof(USBH_OHCI_TGT_INFO),
                                    NU_NO_SUSPEND);

        if ( status == NU_SUCCESS )
        {
            memset(usbh_ohci_tgt,0x00, sizeof(USBH_OHCI_TGT_INFO));

            status = OHCI_Get_Target_Info( key, usbh_ohci_tgt );
            if ( status == NU_SUCCESS )
            {
                /* Get the device specific label. */
                REG_Get_Bytes_Value (key, "/labels/usbh_ohci", (UINT8*)&user_label[0], sizeof(DV_DEV_LABEL));

                /* Increment device label count to one to accommodate user label. */
                device_label_cnt++;

                /* Append standard USB label and user label. */
                status = DVS_Label_Append(&device_labels[0], device_label_cnt,
                                          user_label, 1,
                                          &standard_labels[0], 1);

                if (status == NU_SUCCESS)
                {
                    /****************************/
                    /* Fill USB Instance Handle */
                    /****************************/

                    /* Attach USBH OHCI info to the instance handle */
                    inst_handle->tgt_info = usbh_ohci_tgt;

                    /* Get setup function if exists */
                    memset(reg_path, 0x00, sizeof(reg_path));
                    strncpy(reg_path, key, strlen(key)+1);
                    strncat(reg_path, "/setup", REG_MAX_KEY_LENGTH-strlen(reg_path));

                    /* Check to see If there is a setup function? */
                    if (REG_Has_Key(reg_path))
                    {
                        status = REG_Get_UINT32(reg_path, (UINT32 *)&setup_func);
                        if(status == NU_SUCCESS && setup_func != NU_NULL)
                        {
                            inst_handle->setup_func = setup_func;
                        }
                    }

                    /* If there is a cleanup function, save it */
                    memset(reg_path, 0x00, sizeof(reg_path));
                    strncpy(reg_path, key, strlen(key)+1);
                    strncat(reg_path, "/cleanup", REG_MAX_KEY_LENGTH-strlen(reg_path));
                    if (REG_Has_Key(reg_path))
                    {
                        REG_Get_UINT32(reg_path, (UINT32 *)&cleanup_func);
                        if ( cleanup_func != NU_NULL )
                        {
                            inst_handle->cleanup_func = cleanup_func;
                        }
                    }

                    /*********************************/
                    /* REGISTER WITH DEVICE MANAGER  */
                    /*********************************/

                    /* Populate function pointers */
                    usbh_ohci_drv_funcs.drv_open_ptr  = &USBH_OHCI_Open;
                    usbh_ohci_drv_funcs.drv_close_ptr = &USBH_OHCI_Close;
                    usbh_ohci_drv_funcs.drv_read_ptr  = &USBH_OHCI_Read;
                    usbh_ohci_drv_funcs.drv_write_ptr = &USBH_OHCI_Write;
                    usbh_ohci_drv_funcs.drv_ioctl_ptr = &USBH_OHCI_Ioctl;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                    status = PMI_Device_Initialize(&(inst_handle->pmi_dev), key, device_labels,
                                                   &device_label_cnt, OHCI_UII_BASE);
                    if ( status == NU_SUCCESS )
                    {
                        /* Setup the power device */
                        PMI_Device_Setup(inst_handle->pmi_dev, &OHCI_Tgt_Pwr_Set_State, OHCI_POWER_BASE,
                                                   OHCI_TOTAL_STATE_COUNT, &(inst_handle->device_id), (VOID*)inst_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

                        /* Perform DVFS related setup */
                        status = PMI_DVFS_Setup(inst_handle->pmi_dev, key, (VOID*)inst_handle,
                                       &OHCI_Tgt_Pwr_Notify_Park1, &OHCI_Tgt_Pwr_Notify_Park2,
                                       &OHCI_Tgt_Pwr_Notify_Resume1, &OHCI_Tgt_Pwr_Notify_Resume2,
                                       NU_NULL);
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
                    }
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE. */

                    if ( status == NU_SUCCESS )
                    {
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                       
                        TCCT_Schedule_Lock();
#endif                        
                        /* Register this device with the Device Manager */
                        status = DVC_Dev_Register((VOID*)inst_handle, device_labels,
                                                device_label_cnt, &usbh_ohci_drv_funcs,
                                                 &(inst_handle->device_id));

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                   
                        /* Get default power state */
                        init_pwr_state = PMI_STATE_GET(inst_handle->pmi_dev);
                                             
                        /* Trace log */
                        T_DEV_NAME((CHAR*)key, init_pwr_state, OHCI_TOTAL_STATE_COUNT, inst_handle->device_id);
                        
                        TCCT_Schedule_Unlock();                                                  
#endif                                                 
                        if ( status == NU_SUCCESS)
                        {
                            *dev_id = inst_handle->device_id;
                        }
                    }
                }
            }
        }
        else
        {
            /* Deallocate the entire inst_handle allocated memory */
            (VOID)NU_Deallocate_Memory(inst_handle);
        }
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_OHCI_Unregister
*
* DESCRIPTION
*
*       This function is called when device manager wants to stop ohci.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           stopped successfully.
*
**************************************************************************/
STATUS USBH_OHCI_Unregister (const CHAR *key,
                            INT startstop,
                            DV_DEV_ID dev_id)
{
    STATUS status = NU_SUCCESS;
    USBH_OHCI_INSTANCE_HANDLE *inst_handle;

#ifdef CFG_NU_OS_SVSC_PWR_CORE_ENABLE
    STATUS            pm_status;
#endif

    /* Unregister the device */
    status = DVC_Dev_Unregister (dev_id, (VOID**)&inst_handle);

    if(status == NU_SUCCESS )
    {
        if ( inst_handle )
        {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            status = PMI_Device_Unregister(inst_handle->pmi_dev);

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE. */

            /* Deallocate the memory */
            status = NU_Deallocate_Memory(inst_handle->tgt_info);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))

            (VOID)NU_Deallocate_Memory(inst_handle->config_path);

            /* Decrement the instance index */
            OHCI_Instance_Index--;
#endif
            /* Deallocate the memory of the instance handle */
            (VOID)NU_Deallocate_Memory(inst_handle);
        }
        else
        {
            status = OHCI_NO_INST_AVAILABLE;
        }
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_OHCI_Open
*
* DESCRIPTION
*
*       This is the actual 'open' handler of OHCI controller driver.
*       Device manager make a call to this driver to actually open the
*       OHCI controller driver and further communication.
*
* INPUTS
*
*       instance_handle                     Pointer to an instance handle
*                                           created during initialization.
*       label_list                          List of labels associated with
*                                           this driver.
*       session_handle                      Driver creates a session handle
*                                           and return it in output argument.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           opened successfully.
*
**************************************************************************/
STATUS USBH_OHCI_Open(VOID           *instance_handle,
                      DV_DEV_LABEL    label_list[],
                      INT             label_cnt,
                      VOID          **session_handle)
{
    USBH_OHCI_INSTANCE_HANDLE   *ohci_inst_handle;
    UINT32                      open_mode_requests = 0;
    STATUS                      status = NU_USB_INVLD_HCI;
    STATUS                      pm_status = NU_SUCCESS;
    INT                         int_level;
    USBH_OHCI_SESSION_HANDLE    *ses_ptr;
    DV_DEV_LABEL                usbh_label  = {USBHHW_LABEL};
    NU_MEMORY_POOL              *usys_pool_ptr;
#ifndef CFG_NU_OS_SVCS_PWR_ENABLE
    VOID                        (*setup_func)(VOID);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    ohci_inst_handle = (USBH_OHCI_INSTANCE_HANDLE*) instance_handle;

    /* Get open mode from labels. */
    if (DVS_Label_List_Contains(label_list, label_cnt, usbh_label) == NU_SUCCESS)
    {
        open_mode_requests |= USBHHW_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /* Call the Power device open function */
    status = PMI_Device_Open (&open_mode_requests, label_list, label_cnt);
#endif


    /* If device is already open AND if the open request contains USB mode, return an error. */
    if (!((ohci_inst_handle->device_in_use == NU_TRUE) && (open_mode_requests & USBHHW_OPEN_MODE)))
    {

        /* Allocate memory for the USBH_OHCI_SESSION_HANDLE structure */
        status = NU_Allocate_Memory (&System_Memory, (VOID *)&ses_ptr, sizeof (USBH_OHCI_SESSION_HANDLE), NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Zero out allocated space */
            (VOID)memset (ses_ptr, 0, sizeof (USBH_OHCI_SESSION_HANDLE));

            /* Disable interrupts */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Place a pointer to instance handle in session handle */
            ses_ptr->ohci_inst_handle = ohci_inst_handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            /* If open mode request is power */
            if (open_mode_requests & POWER_OPEN_MODE)
            {
                ses_ptr->open_modes |= POWER_OPEN_MODE;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

               status = PMI_REQUEST_MIN_OP(OHCI_MIN_OP,ohci_inst_handle->pmi_dev);


#endif /* #if ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))*/
            }

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
            if ((open_mode_requests & UII_OPEN_MODE))
            {
                ses_ptr->open_modes |= UII_OPEN_MODE;
            }
#endif /* #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE). */
#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

            /* If the open mode request is USBH HW */
            if (open_mode_requests & USBHHW_OPEN_MODE)
            {
                ses_ptr->open_modes |= USBHHW_OPEN_MODE;

                /* OHCI USB hardware session in a global pointer. */
                OHCI_Session_Handle = ses_ptr;

                /* Set clock enabled to true as we have done it in Register function. */
                OHCI_Session_Handle->is_clock_enabled = NU_TRUE;

                /* Set device in use flag to true */
                ohci_inst_handle->device_in_use = NU_TRUE;
            }

            /* Save the session to the OHCI_CB structure for later use */
            ses_ptr->ohci_cb.ses_handle = (VOID *)ses_ptr;

            /* Set the return address of the session handle */
            *session_handle = (VOID*)ses_ptr;

            /* Restore interrupts to previous level */
            NU_Local_Control_Interrupts(int_level);
        }

        if ((status == NU_SUCCESS )&& (ses_ptr != NU_NULL) && (open_mode_requests & USBHHW_OPEN_MODE))
        {
            /* Allocate memory for cached pool of OHCI. */
            status = NU_Allocate_Memory(&System_Memory,
                                        (VOID**) &(ses_ptr->cached_mem_ptr),
                                        OHCI_CACHED_POOL_SIZE,
                                        NU_NO_SUSPEND);
            if ((status == NU_SUCCESS) && (ses_ptr->cached_mem_ptr != NU_NULL))
            {
                status = NU_Create_Memory_Pool(&(ses_ptr->ohci_cached_pool),
                                               "OHCI-CP",
                                               ses_ptr->cached_mem_ptr,
                                               OHCI_CACHED_POOL_SIZE,
                                               32,
                                               NU_FIFO);
                if ( status == NU_SUCCESS )
                {
                    /* Get system uncached memory pool pointer */
                    status = NU_System_Memory_Get(NU_NULL, &usys_pool_ptr);

                    if (status == NU_SUCCESS)
                    {
                        /* Allocate memory for cached pool of OHCI. */
                        status = NU_Allocate_Memory(usys_pool_ptr,
                                                    (VOID**) &(ses_ptr->uncached_mem_ptr),
                                                    OHCI_UNCACHED_POOL_SIZE,
                                                    NU_NO_SUSPEND);
                    }

                    if ((status == NU_SUCCESS) && (ses_ptr->uncached_mem_ptr != NU_NULL))
                    {
                        status = NU_Create_Memory_Pool(&(ses_ptr->ohci_uncached_pool),
                                                       "OHCI-UCP",
                                                       ses_ptr->uncached_mem_ptr,
                                                       OHCI_UNCACHED_POOL_SIZE,
                                                       32,
                                                       NU_FIFO);
                        if ( status == NU_SUCCESS )
                        {
                            /* Create OHCI controller driver. */
                            status = NU_USBH_OHCI_Create2 (&(ses_ptr->ohci_cb),
                                                            ohci_inst_handle->tgt_info->name,
                                                            &(ses_ptr->ohci_uncached_pool),
                                                            &(ses_ptr->ohci_cached_pool),
                                                            (VOID*)ohci_inst_handle->tgt_info->base_address,
                                                            ohci_inst_handle->tgt_info->irq);
                            if ( status == NU_SUCCESS )
                            {
                                 ses_ptr->ohci_cb.available_current = ohci_inst_handle->tgt_info->total_current;

                                 /* Save the session to the OHCI_CB structure for later use */
                                 ses_ptr->ohci_cb.ses_handle = (VOID *)ses_ptr;

                                 /* Return OHCI handle as session handle. */
                                 *session_handle = (VOID*) ses_ptr;

                                 /* Set that device is not initialized as yet. */
                                 ses_ptr->is_device_init = NU_FALSE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

                                /* This case occurs when the power controller opens the OHCI driver
                                 * before the USBH stack OR if device is set to power on state at startup.
                                 * In that case the state is saved and OHCI is enabled when opened by the
                                 * USBH stack(USBHHW_OPEN_MODE). */
                                if ((PMI_STATE_GET(ohci_inst_handle->pmi_dev) == OHCI_ON) ||
                                    (PMI_STATE_GET(ohci_inst_handle->pmi_dev) == POWER_ON_STATE))
                                {
                                    status = OHCI_Tgt_PWR_Enable((VOID*)ses_ptr);
                                }
#else
                                /* If there are no power services in the system then setup clocks and
                                 * enable VBUS right away. */
                                setup_func = ohci_inst_handle->setup_func;
                                if(setup_func != NU_NULL)
                                {
                                    /* Enable clock for OHCI controller. */
                                    (VOID)setup_func();

                                    /* Enable VBUS supply on all ports. */
                                    (VOID)USBH_TGT_Enable_VBUS();
                                }
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
                            }
                        }
                    }
                    else
                    {
                        /* Memory allocation for uncached_mem_ptr fails, so deallocate former */
                        (VOID)NU_Deallocate_Memory(ses_ptr->cached_mem_ptr);
                    }
                }
                else
                {
                    /* Deallocate the entire ses_ptr allocated memory */
                    (VOID)NU_Deallocate_Memory(ses_ptr);
                }
            }
        }
    }

    if (pm_status != NU_SUCCESS)
    {
        status = pm_status;
    }

    /* Return progress of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_OHCI_Close
*
* DESCRIPTION
*
*       This is the actual 'close' handler of OHCI controller driver.
*       Device manager make a call to this driver to actually close the
*       OHCI controller driver.
*
* INPUTS
*
*       session_handle                      Pointer to session handle
*                                           returned during device open.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
STATUS USBH_OHCI_Close(VOID *session_handle)
{
    USBH_OHCI_SESSION_HANDLE  *ohci_handle;
    USBH_OHCI_INSTANCE_HANDLE *inst_handle;
    VOID                      (*cleanup_func)(VOID);
    STATUS                    status;

    /* Extract pointer to session handle. */
    ohci_handle = (USBH_OHCI_SESSION_HANDLE*) session_handle;
    inst_handle = ohci_handle->ohci_inst_handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    status = PMI_Device_Close(inst_handle->pmi_dev);
    NU_USB_ASSERT( status == NU_SUCCESS );
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* Un-initialize OHCI. */
    status = NU_USBH_OHCI_Uninitialize((NU_USB_HW*)&ohci_handle->ohci_cb);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Set that device is not initialized now. */
    ohci_handle->is_device_init = NU_FALSE;

    /* Disable power on all ports. */
    USBH_TGT_Disable_VBUS();

    cleanup_func = inst_handle->cleanup_func;
    if(cleanup_func != NU_NULL)
    {
        /* Disable USBH OHCI Clocks. */
        (VOID)cleanup_func();
    }

    /* Delete OHCI driver. */
    status = _NU_USBH_OHCI_Delete(&ohci_handle->ohci_cb);
    NU_USB_ASSERT( status == NU_SUCCESS );

    /* Delete OHCI un-cached memory pool. */
    status = NU_Delete_Memory_Pool(&ohci_handle->ohci_uncached_pool);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Delete OHCI cached memory pool. */
    status = NU_Delete_Memory_Pool(&ohci_handle->ohci_cached_pool);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Deallocate memory assigned to OHCI un-cached memory pool. */
    status = NU_Deallocate_Memory(ohci_handle->uncached_mem_ptr);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Deallocate memory assigned to OHCI cached memory pool. */
    status = NU_Deallocate_Memory(ohci_handle->cached_mem_ptr);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Set device is closed */
    inst_handle->device_in_use = NU_FALSE;

    /* Deallocate data */
    (VOID)NU_Deallocate_Memory(ohci_handle);

    return ( NU_SUCCESS ) ;
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_OHCI_Read
*
* DESCRIPTION
*
*       This is the actual 'read' handler of OHCI controller driver.
*       Device manager make a call to this driver to actually read the
*       data from OHCI driver.
*       This function is not used for OHCI, hence it should remain empty.
*
* INPUTS
*
*       session_handle                      Pointer to session handle
*                                           returned during device open.
*       buffer                              Pointer to buffer where data
*                                           is to be saved.
*       numbytes                            Length of data present in
*                                           'buffer'.
*       byte_offset
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
STATUS USBH_OHCI_Read(VOID      *session_handle,
                    VOID        *buffer,
                    UINT32       numbyte,
                    OFFSET_T     byte_offset,
                    UINT32      *bytes_read_ptr)
{
    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_OHCI_Write
*
* DESCRIPTION
*
*       This is the actual 'write' handler of OHCI controller driver.
*       Device manager make a call to this driver to actually write the
*       data to OHCI driver.
*       This function is not used for OHCI, hence it should remain empty.
*
* INPUTS
*
*       session_handle                      Pointer to session handle
*                                           returned during device open.
*       buffer                              Pointer to buffer containing
*                                           data to be written.
*       numbytes                            Length of data present in
*                                           'buffer'.
*       byte_offset
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
STATUS USBH_OHCI_Write(VOID         *session_handle,
                        const VOID  *buffer,
                        UINT32       numbyte,
                        OFFSET_T     byte_offset,
                        UINT32      *bytes_written_ptr)
{
    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_OHCI_Ioctl
*
* DESCRIPTION
*
*       This is the actual 'ioctl' handler of OHCI controller driver.
*       Device manager make a call to this driver to actually send an
*       IOCTL command to OHCI driver.
*
* INPUTS
*
*       session_handle                      Pointer to session handle
*                                           returned during device open.
*       ioctl_num                           IOCTL identifier.
*       ioctl_data                          Data associated with this
*                                           IOCTL command.
*       ioctl_data_len                      Length of IOCTL data.
*
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
STATUS USBH_OHCI_Ioctl(VOID    *session_handle,
                        INT     ioctl_num,
                        VOID   *ioctl_data,
                        INT     ioctl_data_len)
{
    USBH_OHCI_SESSION_HANDLE  *ohci_handle;
    DV_IOCTL0_STRUCT          *ioctl0;
    STATUS                    status = NU_SUCCESS;
    DV_DEV_LABEL              ohci_label = {USBHHW_LABEL};

    /* Extract pointer to session handle. */
    ohci_handle = (USBH_OHCI_SESSION_HANDLE*) session_handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    if((ioctl_num >= (NU_USB_IOCTL_BASE + NU_USB_IOCTL_INITIALIZE)) &&
       (ioctl_num <= (NU_USB_IOCTL_BASE + NU_USB_IOCTL_DISABLE_INT)) &&
       (PMI_STATE_GET(ohci_handle->ohci_inst_handle->pmi_dev) == OHCI_OFF))
    {
        return  OHCI_DEVICE_PWR_STATE_OFF;
    }
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    switch(ioctl_num)
    {
        case DV_IOCTL0:
        {
            if (ioctl_data_len == sizeof(DV_IOCTL0_STRUCT))
            {
                /* Get the ioctl0 structure from the data passed in */
                ioctl0 = (DV_IOCTL0_STRUCT *)ioctl_data;
                status = DV_IOCTL_INVALID_MODE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                status = PMI_Device_Ioctl(ohci_handle->ohci_inst_handle->pmi_dev, ioctl_num, ioctl_data, ioctl_data_len,
                                          ohci_handle->ohci_inst_handle, ohci_handle->open_modes);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

                if (status != NU_SUCCESS)
                {
                    /* If the mode requested is supported and if the session was opened for that mode */
                    if ((DV_COMPARE_LABELS (&(ioctl0->label), &ohci_label)) &&
                        (ohci_handle->open_modes & USBHHW_OPEN_MODE))
                    {
                        ioctl0->base = NU_USB_IOCTL_BASE;
                        status = NU_SUCCESS;
                    }
                }
            }
            break;
        }

        /********************************
         * USB Host OHCI IOCTLS *
         ********************************/
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_INITIALIZE):
            status = OHCI_Handle_Initialize(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_IO_REQUEST):
            status = OHCI_Handle_IO_Request(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_OPEN_PIPE):
            status = OHCI_Handle_Open_Pipe(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_CLOSE_PIPE):
            status = OHCI_Handle_Close_Pipe(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_MODIFY_PIPE):
            status = OHCI_Handle_Modify_Pipe(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_FLUSH_PIPE):
            status = OHCI_Handle_Flush_Pipe(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_EXECUTE_ISR):
            status = OHCI_Handle_Execute_ISR(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_ENABLE_INT):
            status = OHCI_Handle_Enable_Int(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_DISABLE_INT):
            status = OHCI_Handle_Disable_Int(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_GET_HW_CB):
            status = OHCI_Handle_Get_CB(ohci_handle, (VOID **)ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_GET_SPEED):
            status = OHCI_Handle_Get_Speed(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_IS_CURR_AVAILABLE):
            status = OHCI_Handle_Is_Current_Available(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_RELEASE_POWER):
            status = OHCI_Handle_Release_Power(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_REQ_POWER_DOWN):
            status = OHCI_Handle_Request_Power_Down(ohci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_UNINITIALIZE):
            status = OHCI_Handle_Uninitialize(ohci_handle, ioctl_data, ioctl_data_len);
            break;
#if (USB_TEST_MODE_SUPPORT == NU_TRUE)
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_TEST_MODE):
            status = USBH_HWCTRL_Tgt_Handle_Test_Mode(ohci_handle, ioctl_data, ioctl_data_len);
            break;
#endif

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))

        case (NU_USB_IOCTL_BASE + NU_USBH_PWR_HIB_RESTORE):

            /* Call hibernate restore for USB session. */
            status = OHCI_Tgt_Pwr_Hibernate_Restore(ohci_handle);

            break;
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE)) */

        default:

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            /* Call the PMI IOCTL function for Power and UII IOCTLs */
           status = PMI_Device_Ioctl(ohci_handle->ohci_inst_handle->pmi_dev, ioctl_num, ioctl_data,
                                     ioctl_data_len, ohci_handle->ohci_inst_handle, ohci_handle->open_modes);

#else
            status = NU_USB_NOT_SUPPORTED;
#endif

            break;
    }

    return ( status );
}


#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))

/**************************************************************************
*
* FUNCTION
*
*       OHCI_HWCTRL_Hibernate
*
* DESCRIPTION
*
*       This function is called to put the EHCI controller into hibernate
*       mode.
*
* INPUTS
*
*       key                                 Registry path of USBH HWCTRL
*                                           parameters.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver has
*                                           been initialized successfully.
*       REG_BAD_PATH                        Invalid registry path.
*
**************************************************************************/
STATUS   OHCI_HWCTRL_Hibernate (const CHAR * key)
{
    UINT8            instance_index = 0;
    STATUS           status = REG_BAD_PATH;

    /* Search every node until the reg path entry is found */
    for (instance_index = 0; (instance_index < OHCI_Instance_Index); instance_index++)
    {
        /* Verify this is a valid instance handle slot */
        if (OHCI_Inst_Handle_Array[instance_index] != NU_NULL)
        {
            if (strncmp(key, OHCI_Inst_Handle_Array[instance_index] -> config_path, strlen(key)) == 0)
            {
                /* Call the Hibernate function for the device */
                OHCI_Tgt_Pwr_Hibernate_Enter(OHCI_Inst_Handle_Array[instance_index]);

                status = NU_SUCCESS;
            }
        }
    }

    return status;
}

/**************************************************************************
*
* FUNCTION
*
*       OHCI_HWCTRL_Resume
*
* DESCRIPTION
*
*       This function is called to bring the OHCI controller out of hibernate.
*
* INPUTS
*
*       key                                 Registry path of USBH HWCTRL
*                                           parameters.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver has
*                                           been initialized successfully.
*       REG_BAD_PATH                        Invalid registry path.
*
**************************************************************************/
STATUS   OHCI_HWCTRL_Resume(const CHAR * key)
{
    UINT8            instance_index = 0;
    STATUS           status = REG_BAD_PATH;

    /* Search every node until the reg path entry is found */
    for (instance_index = 0; (instance_index < OHCI_Instance_Index); instance_index++)
    {
        /* Verify this is a valid instance handle slot */
        if (OHCI_Inst_Handle_Array[instance_index] != NU_NULL)
        {
            if (strncmp(key, OHCI_Inst_Handle_Array[instance_index] -> config_path, strlen(key)) == 0)
            {
                /* Restore the device from a hibernate state.*/
                (VOID)NU_PM_Restore_Hibernate_Device (OHCI_Inst_Handle_Array[instance_index] -> device_id,
                                                      (NU_USB_IOCTL_BASE + NU_USBH_PWR_HIB_RESTORE));

                status = NU_SUCCESS;
            }
        }
    }

    return status;
}
#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */
/* ======================== End of File ================================ */
