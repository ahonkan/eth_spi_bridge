/*************************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                      All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       can_dv_interface.c
*
* COMPONENT
*
*       Can Device Manager Interface - Hardware Driver for CAN controller.
*
* DESCRIPTION
*
*       This file contains the routine for integration of AtmelCAN
*       controller with Device Manager.
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
*       nu_services.h
*       nu_connectivity.h
*       nu_drivers.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/nu_services.h"
#include    "connectivity/nu_connectivity.h"
#include    "drivers/nu_drivers.h"
#include    "os/kernel/plus/core/inc/thread_control.h"
#include    "services/nu_trace_os_mark.h"

/* =====================  Global data ================================== */

/*********************************/
/* EXTERNAL VARIABLES            */
/*********************************/
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern STATUS CAN_Tgt_Pwr_Set_State (VOID *instance_handle, PM_STATE_ID *state);
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)  
extern STATUS   CAN_Tgt_Pwr_Notify_Park1(VOID *instance_handle);
extern STATUS   CAN_Tgt_Pwr_Notify_Resume1(VOID *instance_handle);
#endif
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

extern  STATUS   CAN_Tgt_Open(VOID *session_handle);
extern  STATUS   CAN_Tgt_Terminate_Can(VOID *session_handle);

/* Function prototypes for IOCTL calls. */
extern  STATUS  CAN_Tgt_Write_Driver                (CAN_HANDLE can_dev);
extern  STATUS  CAN_Tgt_Sleep_Node                  (CAN_HANDLE can_dev);
extern  STATUS  CAN_Tgt_Wakeup_Node                 (CAN_HANDLE can_dev);
extern  STATUS  CAN_Tgt_Set_AcpMask                 (CAN_HANDLE can_dev,
                                                     UINT8 mask_buf,
                                                     UINT32 mask);
extern  VOID    CAN_Tgt_Driver_HISR                 (VOID);
extern  STATUS  CAN_Tgt_Set_Baud_Rate               (UINT16 baud_rate,
                                                     CAN_INSTANCE_HANDLE *instance_handle);

#if         (NU_CAN_AUTOMATIC_RTR_RESPONSE && NU_CAN_SUPPORTS_RTR)

extern  STATUS  CAN_Tgt_Assign_Msgbuff              (CAN_PACKET *can_msg);
extern  STATUS  CAN_Tgt_Release_Msgbuff             (CAN_PACKET *can_msg);
extern  VOID    CAN_Tgt_Send_RTR_Response           (CAN_CB *can_cb,
                                                    INT rtr_mb_no);

#endif      /* (NU_CAN_AUTOMATIC_RTR_RESPONSE && NU_CAN_SUPPORTS_RTR) */


/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
static STATUS CAN_Dv_Open (VOID            *instance_handle,
                           DV_DEV_LABEL    label_list[],
                           INT             label_cnt,
                           VOID            **session_handle);
static STATUS CAN_Dv_Close (VOID *session_handle);
static STATUS CAN_Dv_Ioctl (VOID *session_handle,
                            INT  ioctl_cmd,
                            VOID *ioctl_data,
                            INT  ioctl_data_len);

/**************************************************************************
*
* FUNCTION
*
*       CAN_Dv_Register
*
* DESCRIPTION
*
*       This function registers the CAN driver with device manager
*       and associates an instance handle with it.
*
* INPUTS
*
*       key                                 Key.
*       inst_handle                         can instance pointer
*
* OUTPUTS
*
*       status
*
**************************************************************************/
STATUS CAN_Dv_Register(const CHAR * key, CAN_INSTANCE_HANDLE *inst_handle)
{
    STATUS                  status;
    STATUS                  reg_stat = REG_TYPE_ERROR;
    DV_DEV_LABEL            can_labels[CAN_MAX_LABEL_CNT] = {{CAN_LABEL}};
    DV_DEV_LABEL            can_reg_label;
    DV_DRV_FUNCTIONS        can_drv_funcs;
    INT                     can_label_cnt = 1;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif    

    /* Get the device specific label. */
    reg_stat = REG_Get_Bytes_Value(key,"/labels/can",(UINT8*)&can_reg_label, sizeof(DV_DEV_LABEL));

    if (reg_stat == NU_SUCCESS)
    {
        /* Copy the stdio label into the array */
        status = DVS_Label_Append (can_labels, CAN_MAX_LABEL_CNT, can_labels,
                                   can_label_cnt, &can_reg_label, 1);

        if (status == NU_SUCCESS)
        {
            /* Increment label count */
            can_label_cnt++;
        }
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /********************************/
    /* INITIALIZE AS POWER DEVICE   */
    /********************************/
    status = PMI_Device_Initialize(&(inst_handle->pmi_dev), key, can_labels,
                                   &can_label_cnt, NU_NULL);

    if (status == NU_SUCCESS)
    {
        /* Setup the power device */
        PMI_Device_Setup(inst_handle->pmi_dev, &CAN_Tgt_Pwr_Set_State, NU_CAN_POWER_BASE,
                         CAN_TOTAL_POWER_STATE_COUNT, &(inst_handle->device_id), (VOID*)inst_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

        /* Perform DVFS related setup */
        status = PMI_DVFS_Setup(inst_handle->pmi_dev, key, (VOID*)inst_handle,
                                &CAN_Tgt_Pwr_Notify_Park1, NU_NULL, 
                                &CAN_Tgt_Pwr_Notify_Resume1, NU_NULL, NU_NULL);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
    }

#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

    /*********************************/
    /* REGISTER WITH DEVICE MANAGER  */
    /*********************************/

    /* Populate function pointers. */
    can_drv_funcs.drv_open_ptr  = &CAN_Dv_Open;
    can_drv_funcs.drv_close_ptr = &CAN_Dv_Close;
    can_drv_funcs.drv_read_ptr  = NU_NULL;
    can_drv_funcs.drv_write_ptr = NU_NULL;
    can_drv_funcs.drv_ioctl_ptr = &CAN_Dv_Ioctl;

    if (status == NU_SUCCESS)
    {
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))     
        TCCT_Schedule_Lock();
#endif        
        /********************************/
        /* REGISTER WITH DM             */
        /********************************/
        /* Register this device with the Device Manager. */
        status = DVC_Dev_Register((VOID*)inst_handle, can_labels,
                                  can_label_cnt, &can_drv_funcs,
                                  &(inst_handle->device_id));
                                  
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                   
        /* Get default power state */
        init_pwr_state = PMI_STATE_GET(inst_handle->pmi_dev);                                    
        
        /* Trace log */
        T_DEV_NAME((CHAR*)key, init_pwr_state, CAN_TOTAL_POWER_STATE_COUNT, inst_handle->device_id);
       
        TCCT_Schedule_Unlock();
#endif        
    } 
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       CAN_Dv_Unregister
*
* DESCRIPTION
*
*       This function unregisters the CAN driver from device manager.
*
* INPUTS
*
*       key                                 Key.
*       startstop                           Start or Stop flag.
*       dev_id                              Device ID to be unregistered.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver has
*                                           been unregistered successfully.
*       Error Code                          Some error has occured while
*                                           unregistering the driver.
*
**************************************************************************/
STATUS CAN_Dv_Unregister (const CHAR *key, INT startstop, DV_DEV_ID dev_id)
{
    STATUS status;
    CAN_INSTANCE_HANDLE *inst_handle;


    /* Unregister the device. */
    status = DVC_Dev_Unregister (dev_id, (VOID*)&inst_handle);

    /* Deallocate the memory for the instance. */
    if(status == NU_SUCCESS)
    {
        status = NU_Deallocate_Memory(inst_handle);
    }

    return (status);
}


/**************************************************************************
*
* FUNCTION
*
*       CAN_Dv_Open
*
* DESCRIPTION
*
*       This is the actual 'open' handler of CAN hardware controller
*       driver. Any other component which needs to communicate through CAN
*       must open it first.
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
*       NU_SUCCESS                          Indicates that the driver
*                                           opened successfully.
*       Error Code                          Some error has occured while
*                                           opening the driver.
*
**************************************************************************/
static STATUS CAN_Dv_Open(VOID           *instance_handle,
                          DV_DEV_LABEL    label_list[],
                          INT             label_cnt,
                          VOID          **session_handle)
{
    CAN_SESSION_HANDLE     *ses_ptr;
    CAN_INSTANCE_HANDLE    *can_instance;
    VOID                   *pointer;
    STATUS                  status = ~NU_SUCCESS;
    UINT32                  open_mode_requests = 0;
    INT                     int_level;
    DV_DEV_LABEL            can_label = {CAN_LABEL};
    NU_MEMORY_POOL          *sys_pool_ptr;


    can_instance = (CAN_INSTANCE_HANDLE *) instance_handle;

    if (DVS_Label_List_Contains (label_list, label_cnt, can_label) == NU_SUCCESS)
    {
        open_mode_requests |= CAN_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /* Call the Power device open function */
    status = PMI_Device_Open (&open_mode_requests, label_list, label_cnt);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* If device is already open AND if the open request contains CAN mode, return an error. */
    if (!((can_instance->device_in_use == NU_TRUE) && (open_mode_requests & CAN_OPEN_MODE)))
    {
        /* Get pointer to system memory pool */
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Allocate a new session. */
            status = NU_Allocate_Memory (sys_pool_ptr, &pointer,
                                         sizeof(CAN_SESSION_HANDLE), NU_NO_SUSPEND);
            if (status == NU_SUCCESS)
            {
                ESAL_GE_MEM_Clear (pointer, sizeof(CAN_SESSION_HANDLE));
                ses_ptr = (CAN_SESSION_HANDLE *)pointer;

                /* Disable interrupts. */
                int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

                /* Place a pointer to instance handle in session handle. */
                ses_ptr->instance_handle = can_instance;

                /* If the open mode request is CAN. */
                if (open_mode_requests & CAN_OPEN_MODE)
                {
                    /* Set device in use flag to true */
                    can_instance->device_in_use = NU_TRUE;
                    
                    /* Allocate memory for CAN's HISR */
                    status = NU_Allocate_Memory (sys_pool_ptr, &pointer, CAN_HISR_STK_SIZE, NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        /* Save off the stack pointer */
                        can_instance->can_hisr_stk_ptr = pointer;

                        /* Clear the memory we just allocated */
                        (VOID)memset((VOID*)pointer, 0, CAN_HISR_STK_SIZE);

                        /* Create RX HISR */
                        status = NU_Create_HISR (&(can_instance->can_hisr), "CAN HISR", 
                                    CAN_Tgt_Driver_HISR, CAN_HANDLER_PRIORITY, pointer, 
                                    CAN_HISR_STK_SIZE);

                    }
                    else
                    {
                        (VOID)NU_Deallocate_Memory(pointer);
                    }
                
                    /* Initialize device. */
                    CAN_Tgt_Open(ses_ptr);
                }
                
                /* Update open modes */
                ses_ptr->open_modes |= open_mode_requests;

                /* Set the return address of the session handle. */
                *session_handle = (VOID*)ses_ptr;

                /* Restore interrupts to previous level. */
                NU_Local_Control_Interrupts(int_level);
            }
        }
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       CAN_Dv_Close
*
* DESCRIPTION
*
*       This is the actual 'close' handler of CAN hardware controller
*       driver. Once any component, which already has opened CAN driver,
*       is done with CAN functionality, it can close it through
*       device manager.
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
*       Error Code                          Some error has occured while
*                                           closing the driver.
*
**************************************************************************/
static STATUS CAN_Dv_Close (VOID *session_handle)
{
    CAN_SESSION_HANDLE    *can_s_handle;
    CAN_INSTANCE_HANDLE   *can_i_handle;
    STATUS                 status;
#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)
    
    UINT8                  can_dev;
#endif 

    /* Extract pointer to session handle. */
    can_s_handle = (CAN_SESSION_HANDLE*) session_handle;
    can_i_handle = (can_s_handle->instance_handle);

    if(can_s_handle->open_modes == CAN_OPEN_MODE)
    {
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))

        (VOID)PMI_DVFS_Update_MPL_Value((can_i_handle->pmi_dev), PM_NOTIFY_OFF);
        (VOID)PMI_RELEASE_MIN_OP(can_i_handle->pmi_dev);
        
#endif 
#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)
  
        status = CANS_Get_Device_Index(can_s_handle->instance_handle->device_id, &can_dev);

        if (status == NU_SUCCESS)
        {
         	CAN_Loopback_Shutdown(can_dev);
        }
#else
        /* Close CAN controller. */
        CAN_Tgt_Terminate_Can(can_s_handle);
#endif

        /* Set device in use flag to false. */
        can_i_handle->device_in_use = NU_FALSE;
                
        /* Delete the CAN HISR */
        NU_Delete_HISR(&(can_i_handle->can_hisr));

        /* Deallocate memory for CAN HISR stack */
        NU_Deallocate_Memory(can_i_handle->can_hisr_stk_ptr);
    }

    /* Free the handle. */
    status = NU_Deallocate_Memory (can_s_handle);

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       CAN_Dv_Ioctl
*
* DESCRIPTION
*
*       This function is Ioctl routine for CAN hardware controller driver,
*       which is registered with device manager. This function calls
*       a specific Ioctl based on ioctl_cmd.
*
* INPUTS
*
*       dev_handle                          This parameter is the device
*                                           handle of the device to control.
*       ioctl_cmd                           This parameter is the ioctl
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
*       Error Code                          Some error has occured while
*                                           invoking an IOCTL.
*
**************************************************************************/
static STATUS CAN_Dv_Ioctl(VOID *session_handle, INT ioctl_cmd,
                           VOID *ioctl_data, INT ioctl_data_len)
{
    CAN_SESSION_HANDLE     *can_s_handle = (CAN_SESSION_HANDLE *)session_handle;
    CAN_INSTANCE_HANDLE    *can_i_handle = can_s_handle->instance_handle;
    DV_IOCTL0_STRUCT       *ioctl0;
    DV_DEV_LABEL            can_label = {CAN_LABEL};
    STATUS                  status = NU_SUCCESS;
    CAN_PACKET              *can_msg;
    
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    PMI_DEV_HANDLE  pmi_dev = can_i_handle->pmi_dev;
    
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
    
    switch(ioctl_cmd)
    {
        case DV_IOCTL0:

            if (ioctl_data_len == sizeof(DV_IOCTL0_STRUCT))
            {
                ioctl0 = (DV_IOCTL0_STRUCT *)ioctl_data;
                status = DV_IOCTL_INVALID_MODE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, ioctl_data, ioctl_data_len, can_i_handle,
                                          can_s_handle->open_modes);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

                if (status != NU_SUCCESS)
                {
                    /* Get the ioctl0 structure from the data passed in */
                    if (DV_COMPARE_LABELS (&(ioctl0->label), &can_label)
                            && (can_s_handle->open_modes & CAN_OPEN_MODE) )
                    {
                        ioctl0->base = NU_CAN_IOCTL_BASE;
                        status = NU_SUCCESS;
                    }
                }
            }
            break;

        /********************************/
        /* CAN Hardware IOCTLS          */
        /********************************/

        case (NU_CAN_IOCTL_BASE + NU_CAN_INITIALIZE):

            break;
            
        case (NU_CAN_IOCTL_BASE + NU_CAN_DATA_REQUEST):
        case (NU_CAN_IOCTL_BASE + NU_CAN_REMOTE_REQUEST):
        {
            can_msg = (CAN_PACKET*) ioctl_data;
#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)
            status = CAN_Loopback_Write_Driver(can_msg->can_dev);
#else
            status = CAN_Tgt_Write_Driver(can_msg->can_dev);
#endif
            
        }
        break;

        case (NU_CAN_IOCTL_BASE + NU_CAN_DATA_WRITE):
        {
        }
        break;

        case (NU_CAN_IOCTL_BASE + NU_CAN_SLEEP_NODE):
        {
            CAN_HANDLE* can_dev = (CAN_HANDLE*) ioctl_data;
#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)
            status = CAN_Loopback_Sleep(*can_dev);
#else
            status = CAN_Tgt_Sleep_Node (*can_dev);
#endif
        }
        break;

        case (NU_CAN_IOCTL_BASE + NU_CAN_WAKEUP_NODE):
        {
            CAN_HANDLE* can_dev = (CAN_HANDLE*) ioctl_data;
#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)
            status = CAN_Loopback_Wakeup(*can_dev);
#else
            status = CAN_Tgt_Wakeup_Node (*can_dev);
#endif
        }
        break;

        case (NU_CAN_IOCTL_BASE + NU_CAN_SET_ACP_MASK):
        {
            CAN_DRV_IOCTL_ACP_MASK  *can_drv_ioctl_acp_mask;
            can_drv_ioctl_acp_mask = (CAN_DRV_IOCTL_ACP_MASK*) ioctl_data;
#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)
            status = CAN_Loopback_Set_Accept_Mask(can_drv_ioctl_acp_mask->can_dev_id,
                                        can_drv_ioctl_acp_mask->buffer_no,
                                        can_drv_ioctl_acp_mask->mask_value);
#else
            status = CAN_Tgt_Set_AcpMask(can_drv_ioctl_acp_mask->can_dev_id,
                                        can_drv_ioctl_acp_mask->buffer_no,
                                        can_drv_ioctl_acp_mask->mask_value);
#endif
        }
        break;

        case (NU_CAN_IOCTL_BASE + NU_CAN_SET_BAUD_RATE):
        {
            CAN_DRV_IOCTL_SET_BAUD  *can_drv_ioctl_set_baud;
            can_drv_ioctl_set_baud = (CAN_DRV_IOCTL_SET_BAUD*) ioctl_data;
#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)  
            status = CAN_Loopback_Set_BaudRate(can_drv_ioctl_set_baud->baud_rate,
                                           can_i_handle);
#else
            status = CAN_Tgt_Set_Baud_Rate(can_drv_ioctl_set_baud->baud_rate,
                                           can_i_handle);
#endif
        }
        break;


#if         (NU_CAN_AUTOMATIC_RTR_RESPONSE && NU_CAN_SUPPORTS_RTR)

        case (NU_CAN_IOCTL_BASE + NU_CAN_ASSIGN_MSG_BUFFER):
        {
            can_msg = (CAN_PACKET*) ioctl_data;
#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)  
            status = CAN_Loopback_Assign_RTR_Buf(can_msg);
#else
            status = CAN_Tgt_Assign_Msgbuff(can_msg);    
#endif
        }
        break;

        case (NU_CAN_IOCTL_BASE + NU_CAN_RELEASE_MSG_BUFFER):
        {
            can_msg = (CAN_PACKET*) ioctl_data;
#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)  
            status = CAN_Loopback_Release_RTR_Buf(can_msg);
#else
            status = CAN_Tgt_Release_Msgbuff(can_msg);
#endif
        }
        break;

        case (NU_CAN_IOCTL_BASE + NU_CAN_SEND_RTR_RESPONSE):
        {
        }
        break;

#endif      /* (NU_CAN_AUTOMATIC_RTR_RESPONSE && NU_CAN_SUPPORTS_RTR) */

        default:
        {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            /* Call the PMI IOCTL function for Power and UII IOCTLs */
            status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, ioctl_data, ioctl_data_len, can_i_handle,
                                      can_s_handle->open_modes);

#else

            status = DV_INVALID_INPUT_PARAMS;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
        }
        break;

    }

    return (status);
}

/* ======================== End of File ================================ */
