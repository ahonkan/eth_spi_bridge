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
*       usbf_dv_interface.c
*
*   COMPONENT
*
*       USBF                                - USBF Library
*
*   DESCRIPTION
*
*       This file contains the generic USBF DV interface
*       library functions.
*
*   FUNCTIONS
*
*       USBF_Dv_Register
*       USBF_Dv_Unregister
*       USBF_Dv_Open
*       USBF_Dv_Close
*       USBF_Dv_Ioctl
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       nu_connectivity.h
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "connectivity/nu_connectivity.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "services/nu_trace_os_mark.h"

/*********************************/
/* EXTERNAL VARIABLES            */
/*********************************/
extern NU_MEMORY_POOL System_Memory;
#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 1
extern OTG_HWCTRL_SESSION_HANDLE   *OTG_HWCTRL_Session_Handle;
#endif
/*********************************/
/* Global Variables              */
/*********************************/
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
UINT8 USBF_PNP_Task_Stack[USBF_PNP_TASK_MAX_STACK];
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */

/* External function prototypes */
extern VOID USB_USBF_Enable (VOID *inst_handle);
extern VOID USB_USBF_Disable (VOID *inst_handle);
extern VOID USBF_PNP_Task_Entry(UNSIGNED argc, VOID *argv);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern STATUS USBF_Tgt_Pwr_Set_State(VOID *sess_hdl, PM_STATE_ID *state);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
extern STATUS USBF_Tgt_Pwr_Notify_Park1(VOID *instance_handle);
extern STATUS USBF_Tgt_Pwr_Notify_Park2(VOID *instance_handle);
extern STATUS USBF_Tgt_Pwr_Notify_Resume1(VOID *instance_handle);
extern STATUS USBF_Tgt_Pwr_Notify_Resume2(VOID *instance_handle);
extern STATUS USBF_Tgt_Pwr_Notify_Resume3(VOID *instance_handle);
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

extern STATUS   USBF_Tgt_Pwr_Hibernate_Restore (USBF_SESSION_HANDLE * session_handle);

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

extern VOID* NU_USBF_Tgt_Get_Control_Block(VOID);

extern VOID NU_USBF_Tgt_Set_Session_Handle(VOID* session_handle);

extern STATUS USBF_HWCTRL_Tgt_Handle_Initialize(USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS USBF_HWCTRL_Tgt_Handle_IO_Request(USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS  USBF_HWCTRL_Tgt_Handle_Open_Pipe (USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS  USBF_HWCTRL_Tgt_Handle_Close_Pipe (USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS  USBF_HWCTRL_Tgt_Handle_Modify_Pipe (USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS  USBF_HWCTRL_Tgt_Handle_Flush_Pipe (USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS  USBF_HWCTRL_Tgt_Handle_Stall_Endp (USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS  USBF_HWCTRL_Tgt_Handle_Unstall_Endp (USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS  USBF_HWCTRL_Tgt_Handle_Execute_ISR (USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS  USBF_HWCTRL_Tgt_Handle_Enable_Int (USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS  USBF_HWCTRL_Tgt_Handle_Disable_Int (USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS  USBF_HWCTRL_Tgt_Handle_Set_Address (USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS  USBF_HWCTRL_Tgt_Handle_Get_Dev_Status (USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS  USBF_HWCTRL_Tgt_Handle_Get_EP_Status (USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS USBF_HWCTRL_Tgt_Handle_Get_CB(USBF_SESSION_HANDLE *handle,
                                VOID **ioctl_data,
                                INT ioctl_data_len);

extern STATUS USBF_HWCTRL_Tgt_Handle_Get_Capability (USBF_SESSION_HANDLE *handle,
                                VOID *ioctl_data,
                                INT ioctl_data_len);

#if (USB_TEST_MODE_SUPPORT == NU_TRUE)								
extern STATUS USBF_HWCTRL_Tgt_Handle_Test_Mode (USBF_SESSION_HANDLE *handle,
                                VOID *ioctl_data,
                                INT ioctl_data_len);
#endif
                                
#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 1
extern STATUS USBF_HWCTRL_Tgt_Handle_Start_HNP(USBF_SESSION_HANDLE *handle,
                                VOID **ioctl_data,
                                INT ioctl_data_len);
#endif

extern STATUS USBF_HWCTRL_Tgt_Handle_Acquire_Endp(USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS USBF_HWCTRL_Tgt_Handle_Release_Endp(USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS USBF_HWCTRL_Tgt_Handle_Enable_Pullup(USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS USBF_HWCTRL_Tgt_Handle_Disable_Pullup(USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS NU_USBF_HWCTRL_Tgt_Handle_Get_Speed(USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);

extern STATUS USBF_HWCTRL_Tgt_Handle_Get_EP0_MAXP(USBF_SESSION_HANDLE *handle,
                                       VOID *ioctl_data,
                                       INT ioctl_data_len);

extern STATUS NU_USBF_HWCTRL_Tgt_Uninitialize(USBF_SESSION_HANDLE *handle);

extern STATUS  NU_USBF_HWCTRL_Tgt_Create(VOID   *pcb,
                              CHAR             *name,
                              UINT32            capability,
                              UINT8             speed,
                              VOID             *base_address,
                              UINT8             num_irq,
                              INT              *irq );
                              
extern STATUS _NU_USBF_HWCTRL_Tgt_Delete(VOID *cb);

/**************************************************************************
*
* FUNCTION
*
*       USBF_Dv_Register
*
* DESCRIPTION
*
*       This function registers the function controller driver
*       with device manager and associates an instance handle with it.
*
*   INPUTS
*
*       key                                 Key
*       startstop                           Start or stop flag
*       dev_id                              Returned Device ID
*       instance_handle                     USBF instance structure
*
*   OUTPUTS
*
*       status                              - NU_SUCCESS or
*                                             USBF_REGISTRY_ERROR
*
*
**************************************************************************/
STATUS USBF_Dv_Register(const CHAR * key, INT startorstop, DV_DEV_ID *dev_id,
                        USBF_INSTANCE_HANDLE *instance_handle)
{
    STATUS                  status;
    STATUS                  reg_stat = REG_TYPE_ERROR;
    DV_DEV_LABEL            user_label;
    DV_DEV_LABEL            device_labels[USBF_MAX_LABEL_CNT] = {{USBFHW_LABEL}};
    INT                     device_label_cnt = 1;
    DV_DRV_FUNCTIONS        usbf_hw_drv_funcs;
    CHAR                    reg_path[REG_MAX_KEY_LENGTH];
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif    

    /* Suppress warnings */
    NU_UNUSED_PARAM(startorstop);

    /* Get the device specific label. */
    strncpy(reg_path, key, sizeof(reg_path));
    reg_stat = REG_Get_Bytes_Value (key, "/labels/usbf", (UINT8*)&user_label, sizeof(DV_DEV_LABEL));

    if (reg_stat == NU_SUCCESS)
    {
        /* Append standard USB label. */
        status = DVS_Label_Append(device_labels, USBF_MAX_LABEL_CNT, device_labels,
                                  device_label_cnt, &user_label, 1);

        if (status == NU_SUCCESS)
        {
            /* Increment device label count to one to accommodate user label. */
            device_label_cnt++;
        }
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /********************************/
    /* INITIALIZE AS POWER DEVICE   */
    /********************************/
    status = PMI_Device_Initialize(&(instance_handle->pmi_dev), key,
                                   device_labels, &device_label_cnt, USBF_UII_BASE);

    if (status == NU_SUCCESS)
    {
        /* Setup as power device. */
        PMI_Device_Setup(instance_handle->pmi_dev, &USBF_Tgt_Pwr_Set_State,
                         USBF_POWER_BASE, USBF_TOTAL_STATE_COUNT,
                         &(instance_handle->device_id), (VOID*)instance_handle);
    }
    
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

    /* Perform DVFS related setup */
    PMI_DVFS_Setup(instance_handle->pmi_dev, key, (VOID*)instance_handle,
                   &USBF_Tgt_Pwr_Notify_Park1,
                   &USBF_Tgt_Pwr_Notify_Park2,
                   &USBF_Tgt_Pwr_Notify_Resume1,
                   &USBF_Tgt_Pwr_Notify_Resume2,
                   &USBF_Tgt_Pwr_Notify_Resume3);
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
    /* Create event for connect / disconnect reporting. */
    status = NU_Create_Event_Group(&instance_handle->pnp_event, "USBF-EG");

    if ( status == NU_SUCCESS )
    {
        /* Create task for sensing connection and reporting minimum operating point. */
        status = NU_Create_Task(&instance_handle->pnp_task,
                                "USBF-PNP",
                                USBF_PNP_Task_Entry,
                                1,
                                instance_handle,
                                USBF_PNP_Task_Stack,
                                USBF_PNP_TASK_MAX_STACK,
                                1,
                                1,
                                NU_PREEMPT,
                                NU_START);
    }
#endif /* #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */

#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

    if (status == NU_SUCCESS)
    {
        /*********************************/
        /* REGISTER WITH DEVICE MANAGER  */
        /*********************************/

        /* Populate function pointers */
        usbf_hw_drv_funcs.drv_open_ptr  = &USBF_Dv_Open;
        usbf_hw_drv_funcs.drv_close_ptr = &USBF_Dv_Close;
        usbf_hw_drv_funcs.drv_read_ptr  = NU_NULL;
        usbf_hw_drv_funcs.drv_write_ptr = NU_NULL;
        usbf_hw_drv_funcs.drv_ioctl_ptr = &USBF_Dv_Ioctl;

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))           
        TCCT_Schedule_Lock();
#endif
        /* Register this device with the Device Manager */
        status = DVC_Dev_Register((VOID*)instance_handle, device_labels,
                                  device_label_cnt, &usbf_hw_drv_funcs,
                                  &(instance_handle->device_id));
                                  
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                    
        /* Get default power state */
        init_pwr_state = PMI_STATE_GET(instance_handle->pmi_dev); 
                             
        /* Trace log */
        T_DEV_NAME((CHAR*)key, init_pwr_state, USBF_TOTAL_STATE_COUNT, instance_handle->device_id);
  
        TCCT_Schedule_Unlock();                                  
#endif
    }

    if(status == NU_SUCCESS)
    {
        *dev_id = instance_handle->device_id;
    }
    else
    {
        status = USBF_REGISTRY_ERROR;
    }

    /* Return completion status of the service. */
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_Dv_Unregister
*
* DESCRIPTION
*
*       This function unregisters the function controller driver
*       with device manager.
*
* INPUTS
*
*       key                                 Path to registry
*       startstop                           Option to Register or Unregister
*       dev_id                              Device ID
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion
*       [Error Code]                        Error codes from device
*                                           manager requests
*
**************************************************************************/
STATUS USBF_Dv_Unregister (const CHAR *key,
                           INT startstop,
                           DV_DEV_ID dev_id)
{
    STATUS status = NU_SUCCESS;
    USBF_INSTANCE_HANDLE *inst_handle;

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
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_Dv_Open
*
* DESCRIPTION
*
*       This is the actual 'open' handler of function controller driver.
*       Device manager make a call to this driver to actually open
*       the function controller driver for further communication.
*
* INPUTS
*
*       instance_handle                     Pointer to an instance handle
*                                           created during initialization.
*       label_list                          List of labels associated with
*                                           this driver.
*       label_cnt                           Number of labels
*       session_handle                      Driver creates a session handle
*                                           and return it in output argument.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion
*       USBF_ALREADY_OPEN                   Session already open
*
**************************************************************************/
STATUS USBF_Dv_Open(VOID         *instance_handle,
                    DV_DEV_LABEL  label_list[],
                    INT           label_cnt,
                    VOID         **session_handle)
{
    STATUS                  status;
    UINT32                  open_mode_requests = 0;
    INT                     int_level;
    USBF_SESSION_HANDLE     *ses_ptr;
    USBF_INSTANCE_HANDLE    *usbf_instance;
    DV_DEV_LABEL            usbf_label = {USBFHW_LABEL};
    VOID                    *pointer;
    
    usbf_instance = (USBF_INSTANCE_HANDLE *)instance_handle;

   /* Get open mode requests from labels */
    if (DVS_Label_List_Contains (label_list, label_cnt, usbf_label) == NU_SUCCESS)
    {
        open_mode_requests |= USBFHW_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /* Call the Power device open function */
    status = PMI_Device_Open (&open_mode_requests, label_list, label_cnt);
#endif

    /* If device is already open AND if the open request contains USB mode, return a error. */
    if ((usbf_instance->device_in_use != NU_TRUE) && (open_mode_requests & USBFHW_OPEN_MODE))
    {
        /* Allocate a new session */
        status = NU_Allocate_Memory (&System_Memory, &pointer, 
                    sizeof(USBF_SESSION_HANDLE), NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            (VOID)memset(pointer, 0, sizeof(USBF_SESSION_HANDLE));

            ses_ptr = (USBF_SESSION_HANDLE*)pointer;

            ses_ptr->usbf_cb = NU_USBF_Tgt_Get_Control_Block();

            /* Disable interrupts */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Place a pointer to instance handle in session handle */
            ses_ptr->instance_handle = usbf_instance;

            /* If the open mode request is USBF HW */
            if (open_mode_requests & USBFHW_OPEN_MODE)
            {
                /* Set device in use flag to true */
                usbf_instance->device_in_use = NU_TRUE;
            }

            /* Update open modes */
            ses_ptr->open_modes |= open_mode_requests;

            /* Set the return address of the session handle */
            *session_handle = (VOID*)ses_ptr;
#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 1     
            OTG_HWCTRL_Session_Handle->usbf_handle = ses_ptr;
#endif
            /* Restore interrupts to previous level */
            NU_Local_Control_Interrupts(int_level);
            
            /* If the open mode request is USBF HW and the previous operation is successful */
            if ((open_mode_requests & USBFHW_OPEN_MODE))
            {
                /* Create the function controller driver. */
                status = NU_USBF_HWCTRL_Tgt_Create(ses_ptr->usbf_cb,
                                                    usbf_instance->name,
                                                    usbf_instance->capability,
                                                    usbf_instance->speed,
                                                    (VOID*)(usbf_instance->base_address),
                                                    usbf_instance->num_irq,
                                                    &(usbf_instance->irq));
                if(status == NU_SUCCESS)
                {
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))
                    /* Update MPL for DVFS. */
                    PMI_DVFS_Update_MPL_Value(usbf_instance->pmi_dev, PM_NOTIFY_ON);
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)) */

                    /* Save the session handle */
                    NU_USBF_Tgt_Set_Session_Handle((VOID*)ses_ptr);

                }
            }
        }

    }
    else
    {
        /* Already opened in USBF mode */
        status = USBF_ALREADY_OPEN;
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       USBF_Dv_Close
*
* DESCRIPTION
*
*       This is the actual 'close' handler of function controller driver.
*       Device manager make a call to this driver to actually close
*       the function controller driver.
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
STATUS USBF_Dv_Close(VOID *session_handle)
{
    USBF_SESSION_HANDLE    *usbf_handle;
    USBF_INSTANCE_HANDLE   *i_handle;
    STATUS                 status = NU_SUCCESS;

    /* If a valid session, then close it */
    if (session_handle != NU_NULL)
    {
        /* Extract pointer to session handle. */
        usbf_handle = (USBF_SESSION_HANDLE*)session_handle;
        i_handle = usbf_handle->instance_handle;

        /* Check if device is open in USBF mode. */
        if (usbf_handle->open_modes & USBFHW_OPEN_MODE)
        {
            /* Un-initialize function controller driver. */
            status = NU_USBF_HWCTRL_Tgt_Uninitialize(usbf_handle);

            /* Delete function controller driver. */
            status |= _NU_USBF_HWCTRL_Tgt_Delete((VOID *)&(usbf_handle->usbf_cb));

            /* Set device is closed */
            i_handle->device_in_use = NU_FALSE;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            status = PMI_Device_Close(i_handle->pmi_dev);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
        }

        /* Free the handle */
        status |= NU_Deallocate_Memory (usbf_handle);
    }

    /* Return the completion status. */
    return (status);
}

/**************************************************************************
* FUNCTION
*
*       USBF_Dv_Ioctl
*
* DESCRIPTION
*
*       This function is Ioctl routine for function controller driver,
*       which is registered with device manager. This function calls 
*       a specific Ioctl based on ioctl_num.
*
* INPUTS
*
*       dev_handle                          This parameter is the device
*                                           handle of the device to control.
*       ioctl_num                           This parameter is the ioctl
*                                           command.
*       ioctl_data                          This parameter is the optional
*                                           ioctl data.
*       ioctl_data_len                      This parameter is the size, in
*                                           bytes, of the ioctl data.
*                                           
* OUTPUTS                                   
*                                           
*       NU_SUCCESS                          Indicates successful
*                                           initialization of controller.
*       DV_IOCTL_INVALID_LENGTH             Specified data length is invalid
*       DV_INVALID_INPUT_PARAMS             Specified IOCTL code is unknown
*
**************************************************************************/
STATUS USBF_Dv_Ioctl(VOID   *session_handle,
                     INT     ioctl_cmd,
                     VOID   *ioctl_data,
                     INT     ioctl_data_len)
{
    USBF_SESSION_HANDLE     *usbf_hw_handle;
    DV_IOCTL0_STRUCT        *ioctl0;
    STATUS                  status = DV_IOCTL_INVALID_MODE;
    DV_DEV_LABEL            usbf_label = {USBFHW_LABEL};

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    USBF_INSTANCE_HANDLE    *inst_handle;
    PMI_DEV_HANDLE          pmi_dev;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* Extract pointer to usbf controller driver session handle. */
    usbf_hw_handle  = (USBF_SESSION_HANDLE *)session_handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    inst_handle     = usbf_hw_handle->instance_handle;
    pmi_dev         = inst_handle->pmi_dev;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    switch(ioctl_cmd)
    {
        case DV_IOCTL0:
        {
            if (ioctl_data_len == sizeof(DV_IOCTL0_STRUCT))
            {
                /* Get the ioctl0 structure from the data passed in */
                ioctl0 = (DV_IOCTL0_STRUCT *)ioctl_data;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, ioctl_data, ioctl_data_len, inst_handle,
            		                  usbf_hw_handle->open_modes);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
                if (status != NU_SUCCESS)
                {
                    if (DV_COMPARE_LABELS (&(ioctl0->label), &usbf_label)  
                            && (usbf_hw_handle->open_modes & USBFHW_OPEN_MODE) )
                    {
                        ioctl0->base = NU_USB_IOCTL_BASE;
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
        /********************************
         * USB Function Hardware IOCTLS *
         ********************************/
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_INITIALIZE):
            status = USBF_HWCTRL_Tgt_Handle_Initialize(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_IO_REQUEST):
            status = USBF_HWCTRL_Tgt_Handle_IO_Request(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_OPEN_PIPE):
            status = USBF_HWCTRL_Tgt_Handle_Open_Pipe(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_CLOSE_PIPE):
            status = USBF_HWCTRL_Tgt_Handle_Close_Pipe(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_MODIFY_PIPE):
            status = USBF_HWCTRL_Tgt_Handle_Modify_Pipe(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_FLUSH_PIPE):
            status = USBF_HWCTRL_Tgt_Handle_Flush_Pipe(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBF_IOCTL_STALL_ENDP):
            status = USBF_HWCTRL_Tgt_Handle_Stall_Endp(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBF_IOCTL_UNSTALL_ENDP):
            status = USBF_HWCTRL_Tgt_Handle_Unstall_Endp(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_EXECUTE_ISR):
            status = USBF_HWCTRL_Tgt_Handle_Execute_ISR(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_ENABLE_INT):
            status = USBF_HWCTRL_Tgt_Handle_Enable_Int(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_DISABLE_INT):
            status = USBF_HWCTRL_Tgt_Handle_Disable_Int(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBF_IOCTL_SET_ADDRESS):
            status = USBF_HWCTRL_Tgt_Handle_Set_Address(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBF_IOCTL_GET_DEV_STATUS):
            status = USBF_HWCTRL_Tgt_Handle_Get_Dev_Status(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBF_IOCTL_GET_ENDP_STATUS):
            status = USBF_HWCTRL_Tgt_Handle_Get_EP_Status(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_GET_SPEED):
            status = NU_USBF_HWCTRL_Tgt_Handle_Get_Speed(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_GET_HW_CB):
            status = USBF_HWCTRL_Tgt_Handle_Get_CB(usbf_hw_handle, (VOID **)ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBF_IOCTL_GET_CAPABILITY):
            status = USBF_HWCTRL_Tgt_Handle_Get_Capability(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
#if (USB_TEST_MODE_SUPPORT == NU_TRUE)			
        case (NU_USB_IOCTL_BASE + NU_USB_IOCTL_TEST_MODE):
            status = USBF_HWCTRL_Tgt_Handle_Test_Mode(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
#endif			
#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 1
        case (NU_USB_IOCTL_BASE + NU_USBF_IOCTL_START_HNP):
            status = USBF_HWCTRL_Tgt_Handle_Start_HNP(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
#endif
        case (NU_USB_IOCTL_BASE + NU_USBF_IOCTL_ACQUIRE_ENDP):
            status = USBF_HWCTRL_Tgt_Handle_Acquire_Endp(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBF_IOCTL_RELEASE_ENDP):
            status = USBF_HWCTRL_Tgt_Handle_Release_Endp(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBF_IOCTL_ENABLE_PULLUP):
            status = USBF_HWCTRL_Tgt_Handle_Enable_Pullup(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBF_IOCTL_DISABLE_PULLUP):
            status = USBF_HWCTRL_Tgt_Handle_Disable_Pullup(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;
        case (NU_USB_IOCTL_BASE + NU_USBF_IOCTL_GET_EP0_MAXP):
            status = USBF_HWCTRL_Tgt_Handle_Get_EP0_MAXP(usbf_hw_handle, ioctl_data, ioctl_data_len);
            break;

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))

        case (NU_USB_IOCTL_BASE + NU_USBF_PWR_HIB_RESTORE):

            /* Call hibernate restore for USB session. */
            status = USBF_Tgt_Pwr_Hibernate_Restore(usbf_hw_handle);

            break;
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE)) */

        default:
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

        /* Call the PMI IOCTL function for Power and UII IOCTLs */
        status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, ioctl_data, ioctl_data_len, inst_handle,
        		                  usbf_hw_handle->open_modes);

#else

        status = DV_INVALID_INPUT_PARAMS;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
            break;
    }

    return (status);
}
