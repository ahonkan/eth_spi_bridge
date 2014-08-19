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
*       xhci_dv_interface.c
*
*
* COMPONENT
*
*       USB XHCI Controller Driver : Nucleus USB Software.
*
* DESCRIPTION
*
*       This is the platform specific file of Nucleus USB XHCI driver.
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
USBH_XHCI_SESSION_HANDLE    *XHCI_Session_Handle = NU_NULL;

/**************************************************************************
*
* FUNCTION
*
*       USBH_XHCI_Register
*
* DESCRIPTION
*
*       This function is called during system initialization to actually
*       initialize the XHCI controller driver.
*       This function actually registers the XHCI driver with device
*       manager and associates an instance handle with it.
*
* INPUTS
*
*       path                                Registery path of XHCI
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
STATUS USBH_XHCI_Register(const CHAR * key, INT startorstop, DV_DEV_ID *dev_id)
{
    STATUS                    status;
    DV_DEV_LABEL              standard_labels[] = {{USBHHW_LABEL}};
    DV_DEV_LABEL              user_label[1];
    DV_DEV_LABEL              device_labels[XHCI_MAX_LABEL_CNT];
    INT                       device_label_cnt = DV_GET_LABEL_COUNT(standard_labels);
    DV_DRV_FUNCTIONS          usbh_xhci_drv_funcs;
    CHAR                      reg_path[REG_MAX_KEY_LENGTH];
    USBH_XHCI_TGT_INFO        *usbh_xhci_tgt;
    VOID                      (*setup_func)(VOID);
    VOID                      (*cleanup_func)(VOID);
    USBH_XHCI_INSTANCE_HANDLE *inst_handle;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif    

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    DV_DEV_LABEL              usbh_pwr_label = {USBH_PWR_LABEL};
    DV_LISTENER_HANDLE        listener_handle;
#endif

    /********************************/
    /* GET A UNUSED INSTANCE HANDLE */
    /********************************/

    /* Allocate memory for the USBH_XHCI_INSTANCE_HANDLE structure */
    status = NU_Allocate_Memory (&System_Memory, (VOID **)&inst_handle, sizeof (USBH_XHCI_INSTANCE_HANDLE), NU_NO_SUSPEND);
    if (status == NU_SUCCESS)
    {
        /* Zero out allocated space */
        (VOID)memset (inst_handle, 0, sizeof (USBH_XHCI_INSTANCE_HANDLE));

        /* Poplulate info */
        inst_handle->device_id = DV_INVALID_DEV;

        /* Allocate memory for the TGT_INFO structure */
        status = NU_Allocate_Memory (&System_Memory,
                                    (VOID**)&usbh_xhci_tgt,
                                    sizeof(USBH_XHCI_TGT_INFO),
                                    NU_NO_SUSPEND);

        if ( status == NU_SUCCESS )
        {
            memset(usbh_xhci_tgt,0x00, sizeof(USBH_XHCI_TGT_INFO));

            status = XHCI_Get_Target_Info( key, usbh_xhci_tgt );
            if ( status == NU_SUCCESS )
            {
                /* Get the device specific label. */
                strncpy(reg_path, key, sizeof(reg_path));
                REG_Get_Bytes (strcat(reg_path,"/labels/usbh_xhci"),
                                                     (UINT8*)&user_label[0],
                                                     sizeof(DV_DEV_LABEL));

                /* Increment device leable count to one to accomodate user label. */
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

                    /* Attach USBH XHCI info to the instance handle */
                    inst_handle->tgt_info = usbh_xhci_tgt;

                    /* Get setup function if exists */
    
                    strncpy(reg_path, key, sizeof(reg_path));
                    strcat(reg_path, "/setup");

                    /* Check to see If there is a setup function? */
                    if (REG_Has_Key(reg_path))
                    {
                        status = REG_Get_UINT32(reg_path, (UINT32 *)&setup_func);
                        if(status == NU_SUCCESS && setup_func != NU_NULL)
                        {
                            inst_handle->setup_func = setup_func;

                            /* Setup USBH_XHCI */
                            (VOID)setup_func();
                        }
                    }

                    /* If there is a cleanup function, save it */
                    strncpy(reg_path, key, sizeof(reg_path));
                    strcat(reg_path, "/cleanup");
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
                    usbh_xhci_drv_funcs.drv_open_ptr  = &USBH_XHCI_Open;
                    usbh_xhci_drv_funcs.drv_close_ptr = &USBH_XHCI_Close;
                    usbh_xhci_drv_funcs.drv_read_ptr  = &USBH_XHCI_Read;
                    usbh_xhci_drv_funcs.drv_write_ptr = &USBH_XHCI_Write;
                    usbh_xhci_drv_funcs.drv_ioctl_ptr = &USBH_XHCI_Ioctl;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                    status = PMI_Device_Initialize(&(inst_handle->pmi_dev), key, device_labels,
                                                   &device_label_cnt, XHCI_UII_BASE);
                    if ( status == NU_SUCCESS )
                    {
                        /* Setup the power device */
                        PMI_Device_Setup(inst_handle->pmi_dev, &USBH_XHCI_Set_State, XHCI_POWER_BASE,
                                                   XHCI_TOTAL_STATE_COUNT, &(inst_handle->device_id), (VOID*)inst_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

                        /* Perform DVFS related setup */
                        status = PMI_DVFS_Setup(inst_handle->pmi_dev, key, (VOID*)inst_handle,
                                       &XHCI_Pwr_Pre_Park, &XHCI_Pwr_Post_Park,
                                       &XHCI_Pwr_Pre_Resume, &XHCI_Pwr_Post_Resume,
                                       NU_NULL);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
                    }

                    if ( status == NU_SUCCESS )
                    {
                        /* Initialize XHCI port power controller driver handle to invalid value. */
                        inst_handle->xhci_pwr_handle = DV_INVALID_HANDLE;
                        
                        /* Register listener with device manager for notifying port power driver. */
                        status = DVC_Reg_Change_Notify(&usbh_pwr_label,
                                                    DV_GET_LABEL_COUNT(usbh_pwr_label),
                                                    USBH_XHCI_PWR_Drvr_Register_CB,
                                                    USBH_XHCI_PWR_Drvr_Unregister_CB,
                                                    inst_handle,
                                                    &listener_handle);
                    }

#endif /* CFG_NU_OS_SVSC_PWR_CORE_ENABLE */
                    if ( status == NU_SUCCESS )
                    {
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                       
                        TCCT_Schedule_Lock();
#endif                        
                        /* Register this device with the Device Manager */
                        status = DVC_Dev_Register((VOID*)inst_handle, device_labels,
                                                device_label_cnt, &usbh_xhci_drv_funcs,
                                                 &(inst_handle->device_id));
                                                 
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                    
                        /* Get default power state */
                        init_pwr_state = PMI_STATE_GET(inst_handle->pmi_dev);   
                                                  
                        /* Trace log */
                        T_DEV_NAME((CHAR*)key, init_pwr_state, XHCI_TOTAL_STATE_COUNT, inst_handle->device_id);
                        
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
*       USBH_XHCI_Unregister
*
* DESCRIPTION
*
*       This function is called when device manager wants to stop xhci.
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
STATUS USBH_XHCI_Unregister (const CHAR *key,
                            INT startstop,
                            DV_DEV_ID dev_id)
{
    STATUS status = NU_SUCCESS;
    USBH_XHCI_INSTANCE_HANDLE *inst_handle;
    
#ifdef CFG_NU_OS_SVSC_PWR_CORE_ENABLE
    STATUS            pm_status;
#endif

    /* Unregister the device */
    status = DVC_Dev_Unregister (dev_id, (VOID*)&inst_handle);

    if(status == NU_SUCCESS )
    {
        if ( inst_handle )
        {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            status = PMI_Device_Unregister(inst_handle->pmi_dev);

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE. */

            /* Deallocate the memory */
            status = NU_Deallocate_Memory(inst_handle->tgt_info);

            /* Deallocate the memory of the instance handle */
            (VOID)NU_Deallocate_Memory(inst_handle);
        }
        else
        {
            status = XHCI_NO_INST_AVAILABLE;
        }
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_XHCI_Open
*
* DESCRIPTION
*
*       This is the actual 'open' handler of XHCI controller driver.
*       Device manager make a call to this driver to actually open the
*       XHCI controller driver and further communication.
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
STATUS USBH_XHCI_Open(VOID           *instance_handle,
                      DV_DEV_LABEL    label_list[],
                      INT             label_cnt,
                      VOID          **session_handle)
{
    USBH_XHCI_INSTANCE_HANDLE   *xhci_inst_handle;
    UINT32                      open_mode_requests = 0;
    STATUS                      status = NU_USB_INVLD_HCI;
    STATUS                   pm_status = NU_SUCCESS;
    INT                         int_level;
    USBH_XHCI_SESSION_HANDLE    *ses_ptr;
    DV_DEV_LABEL                usbh_label  = {USBHHW_LABEL};
    NU_MEMORY_POOL              *usys_pool_ptr;

    xhci_inst_handle = (USBH_XHCI_INSTANCE_HANDLE*) instance_handle;

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
    if (!((xhci_inst_handle->device_in_use == NU_TRUE) && (open_mode_requests & USBHHW_OPEN_MODE)))
    {

        /* Allocate memory for the USBH_XHCI_SESSION_HANDLE structure */
        status = NU_Allocate_Memory (&System_Memory, (VOID *)&ses_ptr, sizeof (USBH_XHCI_SESSION_HANDLE), NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Zero out allocated space */
            (VOID)memset (ses_ptr, 0, sizeof (USBH_XHCI_SESSION_HANDLE));

            /* Disable interrupts */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Place a pointer to instance handle in session handle */
            ses_ptr->xhci_inst_handle = xhci_inst_handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            /* If open mode request is power */
            if (open_mode_requests & POWER_OPEN_MODE)
            {
                ses_ptr->open_modes |= POWER_OPEN_MODE;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

               status = PMI_REQUEST_MIN_OP(XHCI_MIN_OP,xhci_inst_handle->pmi_dev);


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
                
                /* XHCI USB hardware session in a global pointer. */
                XHCI_Session_Handle = ses_ptr;

                /* Set clock enabled to true as we have done it in Register function. */
                XHCI_Session_Handle->is_clock_enabled = NU_TRUE;

                /* Set device in use flag to true */
                xhci_inst_handle->device_in_use = NU_TRUE;
            }

            /* Save the session to the XHCI_CB structure for later use */
            ses_ptr->xhci_cb.ses_handle = (VOID *)ses_ptr;

            /* Set the return address of the session handle */
            *session_handle = (VOID*)ses_ptr;

            /* Restore interrupts to previous level */
            NU_Local_Control_Interrupts(int_level);
        }

        if ((status == NU_SUCCESS )&& (ses_ptr != NU_NULL) && (open_mode_requests & USBHHW_OPEN_MODE))
        {
            /* Allocate memory for cached pool of XHCI. */
            status = NU_Allocate_Memory(&System_Memory,
                                        (VOID**) &(ses_ptr->cached_mem_ptr),
                                        XHCI_CACHED_POOL_SIZE,
                                        NU_NO_SUSPEND);
            if ((status == NU_SUCCESS) && (ses_ptr->cached_mem_ptr != NU_NULL))
            {
                status = NU_Create_Memory_Pool(&(ses_ptr->xhci_cached_pool),
                                            "XHCI-CP",
                                            ses_ptr->cached_mem_ptr,
                                            XHCI_CACHED_POOL_SIZE,
                                            32,
                                            NU_FIFO);
                if ( status == NU_SUCCESS )
                {
                    /* Get system uncached memory pool pointer */
                    status = NU_System_Memory_Get(NU_NULL, &usys_pool_ptr);

                    if (status == NU_SUCCESS)
                    {
                        /* Allocate memory for cached pool of XHCI. */
                        status = NU_Allocate_Memory(usys_pool_ptr,
                                                    (VOID**) &(ses_ptr->uncached_mem_ptr),
                                                    XHCI_UNCACHED_POOL_SIZE,
                                                    NU_NO_SUSPEND);
                    }

                    if ((status == NU_SUCCESS) && (ses_ptr->uncached_mem_ptr != NU_NULL))
                    {
                        status = NU_Create_Memory_Pool(&(ses_ptr->xhci_uncached_pool),
                                                    "XHCI-UCP",
                                                    ses_ptr->uncached_mem_ptr,
                                                    XHCI_UNCACHED_POOL_SIZE,
                                                    32,
                                                    NU_FIFO);
                        if ( status == NU_SUCCESS )
                        {
                            /* Create XHCI controller driver. */
                            status = NU_USBH_XHCI_Create (&(ses_ptr->xhci_cb),
                                                            xhci_inst_handle->tgt_info->name,
                                                            &(ses_ptr->xhci_uncached_pool),
                                                            &(ses_ptr->xhci_cached_pool),
                                                            (VOID*)xhci_inst_handle->tgt_info->base_address,
                                                            xhci_inst_handle->tgt_info->irq);
                            if ( status == NU_SUCCESS )
                            {
                                 ses_ptr->xhci_cb.available_current = xhci_inst_handle->tgt_info->total_current;

                                 /* Save the session to the XHCI_CB structure for later use */
                                 ses_ptr->xhci_cb.ses_handle = (VOID *)ses_ptr;

                                 /* Return XHCI handle as session handle. */
                                 *session_handle = (VOID*) ses_ptr;
                            }
                        }
                    }
                    else
                    {
                        /* Memory allocation for uncached_mem_ptr fails, so deallocate former */ 
                        (VOID)NU_Deallocate_Memory(ses_ptr->cached_mem_ptr);
                    }
                }
            }
            else
            {
                /* Deallocate the entire ses_ptr allocated memory */
                (VOID)NU_Deallocate_Memory(ses_ptr);
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
*       USBH_XHCI_Close
*
* DESCRIPTION
*
*       This is the actual 'close' handler of XHCI controller driver.
*       Device manager make a call to this driver to actually close the
*       XHCI controller driver.
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
STATUS USBH_XHCI_Close(VOID *session_handle)
{
    USBH_XHCI_SESSION_HANDLE *xhci_handle;
    USBH_XHCI_INSTANCE_HANDLE *inst_handle;
    STATUS              status;

    /* Extract pointer to session handle. */
    xhci_handle = (USBH_XHCI_SESSION_HANDLE*) session_handle;
    inst_handle = xhci_handle->xhci_inst_handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABL
    status = PMI_Device_Close(inst_handle->pmi_dev);
    NU_USB_ASSERT( status == NU_SUCCESS );
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* Un-initialize XHCI. */
    status = NU_USBH_XHCI_Uninitialize((NU_USB_HW*)&xhci_handle->xhci_cb);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Delete XHCI driver. */
    status = _NU_USBH_XHCI_Delete(&xhci_handle->xhci_cb);
    NU_USB_ASSERT( status == NU_SUCCESS );

    /* Delete XHCI un-cached memory pool. */
    status = NU_Delete_Memory_Pool(&xhci_handle->xhci_uncached_pool);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Delete XHCI cached memory pool. */
    status = NU_Delete_Memory_Pool(&xhci_handle->xhci_cached_pool);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Deallocate memory assigned to XHCI un-cached memory pool. */
    status = NU_Deallocate_Memory(xhci_handle->uncached_mem_ptr);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Deallocate memory assigned to XHCI cached memory pool. */
    status = NU_Deallocate_Memory(xhci_handle->cached_mem_ptr);
    NU_USB_ASSERT ( status == NU_SUCCESS );

    /* Set device is closed */
    xhci_handle->xhci_inst_handle->device_in_use = NU_FALSE;

    /* Deallocate data */
    (VOID)NU_Deallocate_Memory(xhci_handle);

    return ( NU_SUCCESS ) ;
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_XHCI_Read
*
* DESCRIPTION
*
*       This is the actual 'read' handler of XHCI controller driver.
*       Device manager make a call to this driver to actually read the
*       data from XHCI driver.
*       This function is not used for XHCI, hence it should remain empty.
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
STATUS USBH_XHCI_Read(VOID      *session_handle,
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
*       USBH_XHCI_Write
*
* DESCRIPTION
*
*       This is the actual 'write' handler of XHCI controller driver.
*       Device manager make a call to this driver to actually write the
*       data to XHCI driver.
*       This function is not used for XHCI, hence it should remain empty.
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
STATUS USBH_XHCI_Write(VOID         *session_handle,
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
*       USBH_XHCI_Ioctl
*
* DESCRIPTION
*
*       This is the actual 'ioctl' handler of XHCI controller driver.
*       Device manager make a call to this driver to actually send an
*       IOCTL command to XHCI driver.
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
STATUS USBH_XHCI_Ioctl(VOID    *session_handle,
                        INT     ioctl_num,
                        VOID   *ioctl_data,
                        INT     ioctl_data_len)
{
    USBH_XHCI_SESSION_HANDLE  *xhci_handle;
    USBH_XHCI_INSTANCE_HANDLE *inst_handle;
    DV_IOCTL0_STRUCT         *ioctl0;
    STATUS                    status = NU_SUCCESS;

    /* Extract pointer to session handle. */
    xhci_handle = (USBH_XHCI_SESSION_HANDLE*) session_handle;
    inst_handle = xhci_handle->xhci_inst_handle;

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
                status = PMI_Device_Ioctl(inst_handle->pmi_dev, ioctl_num, ioctl_data, ioctl_data_len,
                                          inst_handle, xhci_handle->open_modes);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

                if ( status != NU_SUCCESS )
                {
                    ioctl0->base = NU_USB_IOCTL_BASE;
                    status = NU_SUCCESS;
                }
            }
            break;
        }

        /********************************
         * USB Host XHCI IOCTLS *
         ********************************/
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_INITIALIZE):
            status = XHCI_Handle_Initialize(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_IO_REQUEST):
            status = XHCI_Handle_IO_Request(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_OPEN_PIPE):
            status = XHCI_Handle_Open_Pipe(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_CLOSE_PIPE):
            status = XHCI_Handle_Close_Pipe(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_MODIFY_PIPE):
            status = XHCI_Handle_Modify_Pipe(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_FLUSH_PIPE):
            status = XHCI_Handle_Flush_Pipe(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_EXECUTE_ISR):
            status = XHCI_Handle_Execute_ISR(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_ENABLE_INT):
            status = XHCI_Handle_Enable_Int(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_DISABLE_INT):
            status = XHCI_Handle_Disable_Int(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_GET_HW_CB):
            status = XHCI_Handle_Get_CB(xhci_handle, (VOID **)ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_GET_SPEED):
            status = XHCI_Handle_Get_Speed(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_IS_CURR_AVAILABLE):
            status = XHCI_Handle_Is_Current_Available(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_RELEASE_POWER):
            status = XHCI_Handle_Release_Power(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_REQ_POWER_DOWN):
            status = XHCI_Handle_Request_Power_Down(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_OPEN_SS_PIPE):
            status = XHCI_Handle_Open_SS_Pipe(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBH_IOCTL_UPDATE_DEVICE):
            status = XHCI_Handle_Update_device(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBH_IOCTL_INIT_DEVICE):
            status = XHCI_Handle_Initialize_Device(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBH_IOCTL_DEINIT_DEVICE):
            status = XHCI_Handle_Uninitialize_Device(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBH_IOCTL_UNSTALL_PIPE):
            status = XHCI_Handle_Unstall_Pipe(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBH_IOCTL_DISABLE_PIPE):
            status = XHCI_Handle_Disable_Pipe(xhci_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBH_IOCTL_RESET_BANDWIDTH):
            status = XHCI_Handle_Reset_Bandwidth(xhci_handle, ioctl_data, ioctl_data_len);
            break;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            /* Call the PMI IOCTL function for Power and UII IOCTLs */
           status = PMI_Device_Ioctl(inst_handle->pmi_dev, ioctl_num, ioctl_data,
                                     ioctl_data_len, inst_handle, xhci_handle->open_modes);

#else
            status = NU_USB_NOT_SUPPORTED;
#endif

            break;
    }

    return ( status );
}

/***********************************************************************
*
*   FUNCTION
*
*       nu_os_drvr_usb_host_xhci_init
*
*   DESCRIPTION
*
*       Provides a place to attach target-specific labels to the component
*       and calls the component driver registration function.
*
*   CALLED BY
*
*       System Registry
*
*   CALLS
*
*       USBH_XHCI_Register
*       USBH_XHCI_Unregister
*
*   INPUTS
*
*       CHAR     *key                       - Key
*       INT      starstop                   - Start or Stop flag
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void nu_os_drvr_usb_host_xhci_init (const CHAR * key, INT startstop)
{
    static DV_DEV_ID xhci_dev_id = DV_INVALID_DEV;

    if (startstop)
    {
        /* Call the USBH_XHCI component registration function */
        (VOID)USBH_XHCI_Register(key, startstop, &xhci_dev_id);
    }
    else
    {
        /* If we are stopping an already started device */
        if (xhci_dev_id != DV_INVALID_DEV)
        {
            /* Call the USBH_XHCI component un-registration function */
            (VOID)USBH_XHCI_Unregister (key, startstop, xhci_dev_id);
        }
    }
}

/* ======================== End of File ================================ */
