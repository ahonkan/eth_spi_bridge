/**************************************************************************
*
*              Copyright 2011 Mentor Graphics Corporation
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
*       usbh_hwctrl_dv_interface.c
*
* COMPONENT
*
*       Generic USB Host Controller Driver
*
* DESCRIPTION
*
*       This file provides generic layer for USB host controller drivers to
*       communicate with device manager.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       USBH_HWCTRL_Register
*       USBH_HWCTRL_Unregister
*       USBH_HWCTRL_Open
*       USBH_HWCTRL_Close
*       USBH_HWCTRL_Ioctl
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_drivers.h
*       nu_services.h
*
**************************************************************************/

/* =====================  Include Files  =============================== */

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "services/nu_trace_os_mark.h"

/* =====================  Global data  ================================= */

extern  NU_MEMORY_POOL      System_Memory;
USBH_HWCTRL_SESSION_HANDLE *USBH_HWCTRL_Session_Handle = NU_NULL;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)
USBH_HWCTRL_INSTANCE_HANDLE *USBH_Inst_Handle_Array[USBH_MAX_INSTANCES];
INT                         USBH_Instance_Index = 0;
#endif
#endif

/* ==================  External Function Prototypes  =================== */
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern STATUS USBH_HWCTRL_Tgt_Pwr_Set_State(VOID *inst_handle, PM_STATE_ID *state);
extern STATUS USBH_HWCTRL_Tgt_Pwr_Enable (VOID *sess_handle);
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
extern STATUS   USBH_HWCTRL_Tgt_Pwr_Notify_Park1(VOID *instance_handle);
extern STATUS   USBH_HWCTRL_Tgt_Pwr_Notify_Park2(VOID *instance_handle);
extern STATUS   USBH_HWCTRL_Tgt_Pwr_Notify_Resume1(VOID *instance_handle);
extern STATUS   USBH_HWCTRL_Tgt_Pwr_Notify_Resume2(VOID *instance_handle);
extern STATUS   USBH_HWCTRL_Tgt_Pwr_Notify_Resume3(VOID *instance_handle);
#endif
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)
extern STATUS   USBH_Tgt_Pwr_Hibernate_Restore (USBH_HWCTRL_SESSION_HANDLE * session_handle);
extern STATUS   USBH_Tgt_Pwr_Hibernate_Enter (USBH_HWCTRL_INSTANCE_HANDLE * session_handle);
#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */
#endif
extern STATUS USBH_HWCTRL_Tgt_Open(VOID *);
extern STATUS USBH_HWCTRL_Tgt_Close(VOID *);
extern STATUS USBH_HWCTRL_Tgt_Disable_VBUS(NU_USB_HW*);
extern STATUS USBH_HWCTRL_Tgt_Enable_VBUS(NU_USB_HW*);
extern  STATUS  USBH_HWCTRL_Tgt_Create(VOID **,CHAR*,VOID*,INT,UINT32,NU_MEMORY_POOL*,NU_MEMORY_POOL*);
extern  STATUS  USBH_HWCTRL_Tgt_Uninitialize(NU_USB_HW*);
extern  STATUS  USBH_HWCTRL_Tgt_Delete(VOID*);

#if (USB_TEST_MODE_SUPPORT == NU_TRUE)
extern STATUS USBH_HWCTRL_Tgt_Handle_Test_Mode (USBH_HWCTRL_SESSION_HANDLE *handle,
                                VOID *ioctl_data,
                                INT ioctl_data_len);
#endif


/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Register
*
* DESCRIPTION
*
*       This function is called during system initialization to actually
*       initialize the USBH controller driver.
*       This function actually registers the USBH HW driver with device
*       manager and associates an instance handle with it.
*
* INPUTS
*
*       key                                 Registry path of USBH HWCTRL
*                                           parameters.
*       startstop                           Options specifying start or
*                                           stop.
*       dev_id                              Returned Device ID.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver has
*                                           been initialized successfully.
*       NU_NO_MEMORY                        Not enough memory for allocation.
*       REG_BAD_PATH                        Invalid registry path.
*
**************************************************************************/
STATUS USBH_HWCTRL_Register(const CHAR * key, INT startstop, DV_DEV_ID *dev_id)
{
    STATUS                    status;
    DV_DEV_LABEL              standard_labels[] = {{USBHHW_LABEL}};
    DV_DEV_LABEL              user_label[1];
    DV_DEV_LABEL              device_labels[USBH_HWCTRL_MAX_LABEL_CNT];
    INT                       device_label_cnt = DV_GET_LABEL_COUNT(standard_labels);
    DV_DRV_FUNCTIONS          usbh_hwctrl_drv_funcs;
    CHAR                      reg_path[REG_MAX_KEY_LENGTH];
    USBH_HWCTRL_TGT_INFO     *usbh_hwctrl_tgt;
    VOID                      (*setup_func)(VOID);
    VOID                      (*cleanup_func)(VOID);
    USBH_HWCTRL_INSTANCE_HANDLE *inst_handle;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif    

    /* Allocate memory for the USBH_HWCTRL_INSTANCE_HANDLE structure. */
    status = NU_Allocate_Memory (&System_Memory, (VOID **)&inst_handle,
                                 sizeof (USBH_HWCTRL_INSTANCE_HANDLE), NU_NO_SUSPEND);
    if (status == NU_SUCCESS)
    {
        /* Zero out allocated space. */
        (VOID)memset (inst_handle, 0, sizeof (USBH_HWCTRL_INSTANCE_HANDLE));

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))
        USBH_Inst_Handle_Array[USBH_Instance_Index] = inst_handle;

        /* Ensure index has not overrun */
        if (USBH_Instance_Index < USBH_MAX_INSTANCES)
        {
            /* Increment the instance index */
            USBH_Instance_Index++;
        }
#endif

        /* Populate info. */
        inst_handle->device_id = DV_INVALID_DEV;

        status = NU_Allocate_Memory (&System_Memory, (VOID **)&inst_handle->config_path,
                                     REG_MAX_KEY_LENGTH, NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            strncpy(inst_handle->config_path, key, REG_MAX_KEY_LENGTH);
        }

        /* Allocate memory for the USBH_HWCTRL_TGT_INFO structure. */
        status = NU_Allocate_Memory(&System_Memory,
                                    (VOID**)&usbh_hwctrl_tgt,
                                    sizeof(USBH_HWCTRL_TGT_INFO),
                                    NU_NO_SUSPEND);

        if ( status == NU_SUCCESS )
        {
            /* Zero out allocated space. */
            memset(usbh_hwctrl_tgt,0x00, sizeof(USBH_HWCTRL_TGT_INFO));

            /* Get information from registry. */
            status = USBH_HWCTRL_Get_Target_Info( key, usbh_hwctrl_tgt );
            if ( status == NU_SUCCESS )
            {
                /* Get the device specific label. */
                status = REG_Get_Bytes_Value (key, "/labels/usbh", (UINT8*)&user_label[0], sizeof(DV_DEV_LABEL));
                if(status == NU_SUCCESS)
                {
                    /* Increment device label count to one to accommodate user label. */
                    device_label_cnt++;

                    /* Append standard USB label and user label. */
                    status = DVS_Label_Append(&device_labels[0], device_label_cnt,
                                              user_label, 1,
                                              &standard_labels[0], 1);
                }

                /* If specific device label if found and appended successfully. */
                if (status == NU_SUCCESS)
                {
                    /****************************/
                    /* Fill USB Instance Handle */
                    /****************************/

                    /* Attach USBH HWCTRL info to the instance handle. */
                    inst_handle->tgt_info = usbh_hwctrl_tgt;

                    /* Get setup function if exists. */
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

                    /* If there is a cleanup function, save it. */
                    memset(reg_path, 0x00, sizeof(reg_path));
                    strncpy(reg_path, key, strlen(key)+1);
                    strncat(reg_path, "/cleanup", REG_MAX_KEY_LENGTH-strlen(reg_path));
                    if (REG_Has_Key(reg_path))
                    {
                        REG_Get_UINT32(reg_path, (UINT32 *)&cleanup_func);
                        if ( cleanup_func != NU_NULL )
                        {
                            inst_handle->cleanup_func = cleanup_func;

                            /* Call cleanup to ensure that device is at lowest possible
                               power consumption state at startup. */
                            (VOID)cleanup_func();

                        }
                    }

                    /*********************************/
                    /* REGISTER WITH DEVICE MANAGER  */
                    /*********************************/

                    /* Populate function pointers. */
                    usbh_hwctrl_drv_funcs.drv_open_ptr  = &USBH_HWCTRL_Open;
                    usbh_hwctrl_drv_funcs.drv_close_ptr = &USBH_HWCTRL_Close;
                    usbh_hwctrl_drv_funcs.drv_read_ptr  = NU_NULL;
                    usbh_hwctrl_drv_funcs.drv_write_ptr = NU_NULL;
                    usbh_hwctrl_drv_funcs.drv_ioctl_ptr = &USBH_HWCTRL_Ioctl;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                    status = PMI_Device_Initialize(&(inst_handle->pmi_dev), key, device_labels,
                                                   &device_label_cnt, USBH_HWCTRL_UII_BASE);
                    if ( status == NU_SUCCESS )
                    {
                        /* Setup the power device. */
                        PMI_Device_Setup(inst_handle->pmi_dev, &USBH_HWCTRL_Tgt_Pwr_Set_State, USBH_HWCTRL_POWER_BASE,
                                         USBH_HWCTRL_TOTAL_STATE_COUNT, &(inst_handle->device_id), (VOID*)inst_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
                        /* Perform DVFS related setup. */
                        status = PMI_DVFS_Setup(inst_handle->pmi_dev, key, (VOID*)inst_handle,
                                                &USBH_HWCTRL_Tgt_Pwr_Notify_Park1, &USBH_HWCTRL_Tgt_Pwr_Notify_Park2,
                                                &USBH_HWCTRL_Tgt_Pwr_Notify_Resume1, &USBH_HWCTRL_Tgt_Pwr_Notify_Resume2,
                                                NU_NULL);
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
                    }
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE. */
                    if ( status == NU_SUCCESS )
                    {
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                       
                        TCCT_Schedule_Lock();
#endif                        
                        /* Register this device with the Device Manager. */
                        status = DVC_Dev_Register((VOID *)inst_handle, device_labels,
                                                  device_label_cnt, &usbh_hwctrl_drv_funcs,
                                                  &(inst_handle->device_id));
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                   
                        /* Get default power state */
                        init_pwr_state = PMI_STATE_GET(inst_handle->pmi_dev);
                                          
                        /* Trace log */
                        T_DEV_NAME((CHAR*)key, init_pwr_state, USBH_HWCTRL_TOTAL_STATE_COUNT, inst_handle->device_id);
               
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
            /* Deallocate the entire inst_handle allocated memory. */
            (VOID)NU_Deallocate_Memory(inst_handle);
        }
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Unregister
*
* DESCRIPTION
*
*       This function is called when device manager wants to stop USBH
*       controller.
*
* INPUTS
*
*       key                                 Registry path of USBH HWCTRL
*                                           parameters.
*       startstop                           Options specifying start or
*                                           stop.
*       dev_id                              ID of the device to be
*                                           unregistered.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           stopped successfully.
*       DV_DEV_NOT_REGISTERED               Device with given ID is not
*                                           registered.
*       USBH_HWCTRL_NO_INST_AVAILABLE       No instance to unregister.
*
**************************************************************************/
STATUS USBH_HWCTRL_Unregister (const CHAR *key,
                               INT startstop,
                               DV_DEV_ID dev_id)
{
    STATUS status;
    USBH_HWCTRL_INSTANCE_HANDLE *inst_handle;

    /* Unregister the device. */
    status = DVC_Dev_Unregister (dev_id, (VOID*)&inst_handle);
    if(status == NU_SUCCESS )
    {
        if ( inst_handle )
        {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            status = PMI_Device_Unregister(inst_handle->pmi_dev);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE. */
            if(status == NU_SUCCESS)
            {
                /* Deallocate the memory. */
                status = NU_Deallocate_Memory(inst_handle->tgt_info);

                (VOID)NU_Deallocate_Memory(inst_handle->config_path);

                /* Deallocate the memory of the instance handle. */
                (VOID)NU_Deallocate_Memory(inst_handle);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))

                /* Decrement the instance index */
                USBH_Instance_Index--;
#endif
            }
        }
        else
        {
            status = USBH_HWCTRL_NO_INST_AVAILABLE;
        }
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Open
*
* DESCRIPTION
*
*       This is the actual 'open' handler of USBH hardware controller driver.
*       Device manager make a call to this driver to actually open the
*       USBH hardware controller driver for further communication.
*
* INPUTS
*
*       instance_handle                     Pointer to an instance handle
*                                           created during initialization.
*       label_list                          List of labels associated with
*                                           this driver.
*       label_cnt                           Total number of labels.
*       session_handle                      Driver creates a session handle
*                                           and return it in output argument.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           opened successfully.
*       NU_NO_MEMORY                        Not enough memory for allocation.
*       NU_USB_INVLD_HCI                    Either session handle is
*                                           invalid or already opened by
*                                           host stack.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*
**************************************************************************/
STATUS USBH_HWCTRL_Open(VOID           *instance_handle,
                        DV_DEV_LABEL    label_list[],
                        INT             label_cnt,
                        VOID          **session_handle)
{
    USBH_HWCTRL_INSTANCE_HANDLE    *usbh_hwctrl_inst_handle;
    USBH_HWCTRL_SESSION_HANDLE     *ses_ptr;
    NU_MEMORY_POOL                 *usys_pool_ptr;
    UINT32                          open_mode_requests = 0;
    STATUS                          status = NU_USB_INVLD_HCI;
    INT                             int_level;
    UINT8                           rollback = 0;
    DV_DEV_LABEL                    usbh_label  = {USBHHW_LABEL};

    usbh_hwctrl_inst_handle = (USBH_HWCTRL_INSTANCE_HANDLE*) instance_handle;

    /* Get open mode from labels. */
    if (DVS_Label_List_Contains(label_list, label_cnt, usbh_label) == NU_SUCCESS)
    {
        open_mode_requests |= USBH_HWCTRL_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /* Call the Power device open function */
    status = PMI_Device_Open (&open_mode_requests, label_list, label_cnt);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* If device is already open AND if the open request contains USB mode, return an error. */
    if (!((usbh_hwctrl_inst_handle->device_in_use == NU_TRUE) && (open_mode_requests & USBH_HWCTRL_OPEN_MODE)))
    {
        /* Allocate memory for the USBH_HWCTRL_SESSION_HANDLE structure */
        status = NU_Allocate_Memory (&System_Memory, (VOID *)&ses_ptr,
                                     sizeof (USBH_HWCTRL_SESSION_HANDLE), NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Zero out allocated space. */
            (VOID)memset (ses_ptr, 0, sizeof (USBH_HWCTRL_SESSION_HANDLE));

            /* Disable interrupts. */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Place a pointer to instance handle in session handle. */
            ses_ptr->usbh_hwctrl_inst_handle = usbh_hwctrl_inst_handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            /* If open mode request is power */
            if (open_mode_requests & POWER_OPEN_MODE)
            {
                ses_ptr->open_modes |= POWER_OPEN_MODE;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
               status = PMI_REQUEST_MIN_OP(USBH_HWCTRL_MIN_OP, usbh_hwctrl_inst_handle->pmi_dev);
#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE */
            }

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
            if ((open_mode_requests & UII_OPEN_MODE))
            {
                ses_ptr->open_modes |= UII_OPEN_MODE;
            }
#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG */
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

            /* If the open mode request is USBH HW. */
            if (open_mode_requests & USBH_HWCTRL_OPEN_MODE)
            {
                ses_ptr->open_modes |= USBH_HWCTRL_OPEN_MODE;

                /* USBH_HWCTRL USB hardware session in a global pointer. */
                USBH_HWCTRL_Session_Handle = ses_ptr;

                /* Set clock enabled to true as we have done it in Register function. */
                USBH_HWCTRL_Session_Handle->is_clock_enabled = NU_TRUE;

                /* Set device in use flag to true. */
                usbh_hwctrl_inst_handle->device_in_use = NU_TRUE;
            }

            /* Set the return address of the session handle */
            *session_handle = (VOID *)ses_ptr;

            /* Restore interrupts to previous level */
            NU_Local_Control_Interrupts(int_level);
        }

        if ((status == NU_SUCCESS )&& (ses_ptr != NU_NULL) && (open_mode_requests & USBH_HWCTRL_OPEN_MODE))
        {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            /* This case occurs when the power controller opens the USBH HWCTRL driver
             * before the USBH stack. In that case the state is saved and USBH HWCTRL is
             * enabled when opened by the USBH stack (USBH_HWCTRL_OPEN_MODE).
             */
            if ( PMI_STATE_GET(usbh_hwctrl_inst_handle->pmi_dev) == USBH_HWCTRL_ON )
            {
                status = USBH_HWCTRL_Tgt_Pwr_Enable((VOID*)ses_ptr);
            }
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

            if ( status == NU_SUCCESS )
            {
                /* Get system uncached memory pool pointer. */
                status = NU_System_Memory_Get(NU_NULL, &usys_pool_ptr);

                /* Create memory pools only if relevant USB stack option is true. */
#if (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE)
                if (status == NU_SUCCESS)
                {
                    /* Allocate memory for cached pool of USBH_HWCTRL. */
                    status = NU_Allocate_Memory(&System_Memory,
                                                (VOID**) &(ses_ptr->cached_mem_ptr),
                                                USBH_HWCTRL_CACHED_POOL_SIZE,
                                                NU_NO_SUSPEND);
                    if ((status == NU_SUCCESS) && (ses_ptr->cached_mem_ptr != NU_NULL))
                    {
                        rollback = 1;
                        status = NU_Create_Memory_Pool(&(ses_ptr->usbh_hwctrl_cached_pool),
                                                       "UHW_CP",
                                                       ses_ptr->cached_mem_ptr,
                                                       USBH_HWCTRL_CACHED_POOL_SIZE,
                                                       32,
                                                       NU_FIFO);
                        if ( status == NU_SUCCESS )
                        {
                            rollback = 2;
                            /* Allocate memory for uncached pool of USBH HWCTRL. */
                            status = NU_Allocate_Memory(usys_pool_ptr,
                                                        (VOID**) &(ses_ptr->uncached_mem_ptr),
                                                        USBH_HWCTRL_UNCACHED_POOL_SIZE,
                                                        NU_NO_SUSPEND);
                            if ((status == NU_SUCCESS) && (ses_ptr->uncached_mem_ptr != NU_NULL))
                            {
                                rollback = 3;
                                status = NU_Create_Memory_Pool(&(ses_ptr->usbh_hwctrl_uncached_pool),
                                                               "UHW_UCP",
                                                               ses_ptr->uncached_mem_ptr,
                                                               USBH_HWCTRL_UNCACHED_POOL_SIZE,
                                                               32,
                                                               NU_FIFO);
                            }
                        }
                    }
                }
#endif
                if ( status == NU_SUCCESS )
                {
                    rollback = 4;
                    /* Create USBH hardware controller driver. */
                    status = USBH_HWCTRL_Tgt_Create(&(ses_ptr->usbh_hwctrl_cb),
                                                    usbh_hwctrl_inst_handle->tgt_info->name,
                                                    (VOID*)usbh_hwctrl_inst_handle->tgt_info->base_address,
                                                    usbh_hwctrl_inst_handle->tgt_info->irq,
                                                    usbh_hwctrl_inst_handle->tgt_info->total_current,
#if (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE)
                                                    &(ses_ptr->usbh_hwctrl_cached_pool),
                                                    &(ses_ptr->usbh_hwctrl_uncached_pool));
#else
                                                    &(System_Memory),
                                                    usys_pool_ptr);
#endif
                    if ( status == NU_SUCCESS )
                    {
                        rollback = 5;
                        /* Call target specific open function. */
                        status = USBH_HWCTRL_Tgt_Open((VOID*) ses_ptr);

                        if ( status == NU_SUCCESS )
                        {
                            /* Return USBH_HWCTRL handle as session handle. */
                            *session_handle = (VOID *) ses_ptr;
#ifndef CFG_NU_OS_SVCS_PWR_ENABLE
                            rollback = 6;
                            /* If there is no power controller then enable VBUS now. */
                            status = USBH_HWCTRL_Tgt_Enable_VBUS((NU_USB_HW *)ses_ptr->usbh_hwctrl_cb);
#endif
                        }
                    }
                }

                if(status != NU_SUCCESS)
                {
                    switch (rollback)
                    {
                        /* Case 6 can only happen if power is not enabled. */
#ifndef CFG_NU_OS_SVCS_PWR_ENABLE
                        case 6:
                            USBH_HWCTRL_Tgt_Close((VOID*) ses_ptr);
#endif
                        case 5:
                            USBH_HWCTRL_Tgt_Delete(&(ses_ptr->usbh_hwctrl_cb));
                            /* Case 4-1 can only happen if pools are required to be created separately. */
#if (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE)
                        case 4:
                            NU_Delete_Memory_Pool(&(ses_ptr->usbh_hwctrl_uncached_pool));
                        case 3:
                            NU_Deallocate_Memory(ses_ptr->uncached_mem_ptr);
                        case 2:
                            NU_Delete_Memory_Pool(&(ses_ptr->usbh_hwctrl_cached_pool));
                        case 1:
                            NU_Deallocate_Memory(ses_ptr->cached_mem_ptr);
#endif
                    }
                }
            }
         }
    }

    /* Return progress of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Close
*
* DESCRIPTION
*
*       This is the actual 'close' handler of USBH hardware controller driver.
*       Device manager make a call to this driver to actually close the
*       USBH hardware controller driver.
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
STATUS USBH_HWCTRL_Close(VOID *session_handle)
{
    USBH_HWCTRL_SESSION_HANDLE  *usbh_hwctrl_handle;
    USBH_HWCTRL_INSTANCE_HANDLE *inst_handle;
    STATUS                       status;

    /* Extract pointer to session handle. */
    usbh_hwctrl_handle = (USBH_HWCTRL_SESSION_HANDLE*) session_handle;
    inst_handle = usbh_hwctrl_handle->usbh_hwctrl_inst_handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABL
    status = PMI_Device_Close(inst_handle->pmi_dev);
    NU_USB_ASSERT( status == NU_SUCCESS );
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* Un-initialize USBH HWCTRL. */
    status = USBH_HWCTRL_Tgt_Uninitialize((NU_USB_HW*)usbh_hwctrl_handle->usbh_hwctrl_cb);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    status = USBH_HWCTRL_Tgt_Disable_VBUS((NU_USB_HW*)usbh_hwctrl_handle->usbh_hwctrl_cb);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Delete USBH HWCTRL driver. */
    status = USBH_HWCTRL_Tgt_Delete(usbh_hwctrl_handle->usbh_hwctrl_cb);
    NU_USB_ASSERT( status == NU_SUCCESS );

#if (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE)
    /* Delete USBH HWCTRL un-cached memory pool. */
    status = NU_Delete_Memory_Pool(&usbh_hwctrl_handle->usbh_hwctrl_uncached_pool);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Delete USBH HWCTRL cached memory pool. */
    status = NU_Delete_Memory_Pool(&usbh_hwctrl_handle->usbh_hwctrl_cached_pool);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Deallocate memory assigned to USBH HWCTRL un-cached memory pool. */
    status = NU_Deallocate_Memory(usbh_hwctrl_handle->uncached_mem_ptr);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Deallocate memory assigned to USBH HWCTRL cached memory pool. */
    status = NU_Deallocate_Memory(usbh_hwctrl_handle->cached_mem_ptr);
    NU_USB_ASSERT ( status == NU_SUCCESS );
#endif

    /* Set device is closed. */
    inst_handle->device_in_use = NU_FALSE;

    /* Call target specific close function. */
    status = USBH_HWCTRL_Tgt_Close((VOID*)usbh_hwctrl_handle);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Deallocate data. */
    (VOID)NU_Deallocate_Memory(usbh_hwctrl_handle);

    return ( NU_SUCCESS ) ;
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Ioctl
*
* DESCRIPTION
*
*       This is the actual 'ioctl' handler of USBH hardware controller driver.
*       Device manager make a call to this driver to actually send an
*       IOCTL command to USBH hardware controller driver.
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
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*       DV_IOCTL_INVALID_MODE               IOCTL is called with invalid
*                                           session handle.
*       NU_USB_NOT_SUPPORTED                Given IOCTL command is not
*                                           supported.
*
**************************************************************************/
STATUS USBH_HWCTRL_Ioctl(VOID    *session_handle,
                         INT     ioctl_num,
                         VOID   *ioctl_data,
                         INT     ioctl_data_len)
{
    USBH_HWCTRL_SESSION_HANDLE  *usbh_hwctrl_handle;
    DV_IOCTL0_STRUCT            *ioctl0;
    STATUS                       status;
    DV_DEV_LABEL                 usbh_hwctrl_label = {USBHHW_LABEL};

    /* Extract pointer to session handle. */
    usbh_hwctrl_handle = (USBH_HWCTRL_SESSION_HANDLE*) session_handle;

    switch(ioctl_num)
    {
        case DV_IOCTL0:
        {
            status = NU_USB_INVLD_ARG;
            if (ioctl_data_len == sizeof(DV_IOCTL0_STRUCT))
            {
                /* Get the ioctl0 structure from the data passed in */
                ioctl0 = (DV_IOCTL0_STRUCT *)ioctl_data;
                status = DV_IOCTL_INVALID_MODE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                status = PMI_Device_Ioctl(usbh_hwctrl_handle->usbh_hwctrl_inst_handle->pmi_dev, ioctl_num, ioctl_data, ioctl_data_len,
                                          usbh_hwctrl_handle->usbh_hwctrl_inst_handle, usbh_hwctrl_handle->open_modes);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

                if (status != NU_SUCCESS)
                {
                    /* If the mode requested is supported and if the session was opened for that mode. */
                    if ((DV_COMPARE_LABELS (&(ioctl0->label), &usbh_hwctrl_label)) &&
                        (usbh_hwctrl_handle->open_modes & USBH_HWCTRL_OPEN_MODE))
                    {
                        ioctl0->base = NU_USB_IOCTL_BASE;
                        status = NU_SUCCESS;
                    }
                }
            }
            break;
        }

        /*********************************
         * USB Host HW Controller IOCTLS *
         *********************************/
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_INITIALIZE):
            status = USBH_HWCTRL_Handle_Initialize(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_IO_REQUEST):
            status = USBH_HWCTRL_Handle_IO_Request(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_OPEN_PIPE):
            status = USBH_HWCTRL_Handle_Open_Pipe(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_CLOSE_PIPE):
            status = USBH_HWCTRL_Handle_Close_Pipe(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_MODIFY_PIPE):
            status = USBH_HWCTRL_Handle_Modify_Pipe(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_FLUSH_PIPE):
            status = USBH_HWCTRL_Handle_Flush_Pipe(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_EXECUTE_ISR):
            status = USBH_HWCTRL_Handle_Execute_ISR(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_ENABLE_INT):
            status = USBH_HWCTRL_Handle_Enable_Int(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_DISABLE_INT):
            status = USBH_HWCTRL_Handle_Disable_Int(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_GET_HW_CB):
            status = USBH_HWCTRL_Handle_Get_CB(usbh_hwctrl_handle, (VOID **)ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_GET_SPEED):
            status = USBH_HWCTRL_Handle_Get_Speed(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_IS_CURR_AVAILABLE):
            status = USBH_HWCTRL_Handle_Is_Current_Available(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_RELEASE_POWER):
            status = USBH_HWCTRL_Handle_Release_Power(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_REQ_POWER_DOWN):
            status = USBH_HWCTRL_Handle_Request_Power_Down(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_UNINITIALIZE):
            status = USBH_HWCTRL_Handle_Uninitialize(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
#if (USB_TEST_MODE_SUPPORT == NU_TRUE)
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_TEST_MODE):
            status = USBH_HWCTRL_Tgt_Handle_Test_Mode(usbh_hwctrl_handle, ioctl_data, ioctl_data_len);
            break;
#endif

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))

        case (NU_USB_IOCTL_BASE + NU_USBH_PWR_HIB_RESTORE):

            /* Call hibernate restore for USB session. */
            status = USBH_Tgt_Pwr_Hibernate_Restore(usbh_hwctrl_handle);

            break;
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE)) */

        default:

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            /* Call the PMI IOCTL function for Power and UII IOCTLs. */
           status = PMI_Device_Ioctl(usbh_hwctrl_handle->usbh_hwctrl_inst_handle->pmi_dev, ioctl_num, ioctl_data,
                                     ioctl_data_len, usbh_hwctrl_handle->usbh_hwctrl_inst_handle, usbh_hwctrl_handle->open_modes);

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
*       USBH_Tgt_Pwr_Hibernate_Enter
*
* DESCRIPTION
*
*       This function is called during system initialization to actually
*       initialize the USBH controller driver.
*       This function actually registers the USBH HW driver with device
*       manager and associates an instance handle with it.
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
*       NU_NO_MEMORY                        Not enough memory for allocation.
*       REG_BAD_PATH                        Invalid registry path.
*
**************************************************************************/
STATUS   USBH_HWCTRL_Hibernate (const CHAR * key)
{
    UINT8            instance_index = 0;
    STATUS           status = REG_BAD_PATH;

    /* Search every node until the reg path entry is found */
    for (instance_index = 0; (instance_index < USBH_Instance_Index); instance_index++)
    {
        /* Verify this is a valid instance handle slot */
        if (USBH_Inst_Handle_Array[instance_index] != NU_NULL)
        {
            if (strncmp(key, USBH_Inst_Handle_Array[instance_index] -> config_path, strlen(key)) == 0)
            {
                /* Call the Hibernate function for the device */
                USBH_Tgt_Pwr_Hibernate_Enter(USBH_Inst_Handle_Array[instance_index]);

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
*       USBH_HWCTRL_Resume
*
* DESCRIPTION
*
*       This function is called during system initialization to actually
*       initialize the USBH controller driver.
*       This function actually registers the USBH HW driver with device
*       manager and associates an instance handle with it.
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
*       NU_NO_MEMORY                        Not enough memory for allocation.
*       REG_BAD_PATH                        Invalid registry path.
*
**************************************************************************/
STATUS   USBH_HWCTRL_Resume(const CHAR * key)
{
    UINT8            instance_index = 0;
    STATUS           status = REG_BAD_PATH;

    /* Search every node until the reg path entry is found */
    for (instance_index = 0; (instance_index < USBH_Instance_Index); instance_index++)
    {
        /* Verify this is a valid instance handle slot */
        if (USBH_Inst_Handle_Array[instance_index] != NU_NULL)
        {
            if (strncmp(key, USBH_Inst_Handle_Array[instance_index] -> config_path, strlen(key)) == 0)
            {
                /* Restore the device from a hibernate state.*/
                (VOID)NU_PM_Restore_Hibernate_Device (USBH_Inst_Handle_Array[instance_index] -> device_id,
                                                      (NU_USB_IOCTL_BASE + NU_USBH_PWR_HIB_RESTORE));

                status = NU_SUCCESS;
            }
        }
    }

    return status;
}

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

/* ======================== End of File ================================ */
