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
*       serial_dv_interface.c
*
*   COMPONENT
*
*       SERIAL                              - Serial Library
*
*   DESCRIPTION
*
*       This file contains the generic Serial DV interface 
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
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))

static VOID Serial_Dv_Pwr_RX_HISR(VOID);

#endif /* #if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)) */

/* External function prototypes */
extern VOID    Serial_Tgt_Setup(SERIAL_INSTANCE_HANDLE *instance_handle, SERIAL_ATTR *attrs);
extern VOID    Serial_Tgt_Enable(SERIAL_INSTANCE_HANDLE *instance_handle);
extern VOID    Serial_Tgt_Rx_Int_Enable(SERIAL_INSTANCE_HANDLE *instance_handle);
extern VOID    Serial_Tgt_Disable(SERIAL_INSTANCE_HANDLE *instance_handle);
extern STATUS  Serial_Tgt_Baud_Rate_Set(SERIAL_INSTANCE_HANDLE *inst_handle, UINT32 baud_rate);
extern INT     Serial_Tgt_Tx_Busy(SERIAL_INSTANCE_HANDLE *instance_handle);
extern STATUS  Serial_Tgt_Read (VOID* sess_handle, VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_read);
extern STATUS  Serial_Tgt_Write (VOID* sess_handle, const VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_written);
extern VOID    Serial_Tgt_LISR (INT vector);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern VOID     Serial_Tgt_Pwr_Default_State (SERIAL_INSTANCE_HANDLE *inst_handle);
extern STATUS   Serial_Tgt_Pwr_Set_State (VOID *inst_handle, PM_STATE_ID *state);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

extern STATUS   Serial_Tgt_Pwr_Hibernate_Restore (SERIAL_SESSION_HANDLE * session_handle);

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
extern STATUS   Serial_Tgt_Pwr_Min_OP_Pt_Calc (SERIAL_INSTANCE_HANDLE *inst_handle, UINT32 baud_rate, UINT8* min_op_pt);
extern STATUS   Serial_Tgt_Pwr_Notify_Park1(VOID *instance_handle);
extern STATUS   Serial_Tgt_Pwr_Notify_Park2(VOID *instance_handle);
extern STATUS   Serial_Tgt_Pwr_Notify_Resume1(VOID *instance_handle);
extern STATUS   Serial_Tgt_Pwr_Notify_Resume2(VOID *instance_handle);
extern STATUS   Serial_Tgt_Pwr_Notify_Resume3(VOID *instance_handle);
#endif
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Dv_Register
*
*   DESCRIPTION
*
*       This function registers the hardware and places it in a
*       known, low-power state
*
*   INPUTS
*
*       key                                 - Path to registry
*       instance_handle                     - Serial instance structure
*
*   OUTPUTS
*
*       status                              - NU_SUCCESS or
*                                             SERIAL_NO_INSTANCE_AVAILABLE or
*                                             SERIAL_REGISTRY_ERROR or
*                                             SERIAL_TOO_MANY_LABELS
*
*************************************************************************/
STATUS  Serial_Dv_Register (const CHAR *key, SERIAL_INSTANCE_HANDLE *instance_handle)
{
    INT             status = NU_SUCCESS;
    DV_DEV_LABEL    all_labels[5] = {{SERIAL_LABEL}};
    INT             all_labels_cnt = 1;
    DV_DEV_LABEL    stdio_label = {STDIO_LABEL};
    DV_DEV_LABEL    reg_label;
    BOOLEAN         stdio_en = NU_FALSE;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif    

    /* DVR function pointers */
    DV_DRV_FUNCTIONS uart_drv_funcs =
    {
        Serial_Dv_Open,
        Serial_Dv_Close,
        Serial_Tgt_Read,
        Serial_Tgt_Write,
        Serial_Dv_Ioctl
    };

    /* See if it is marked STDIO */
    status = REG_Get_Boolean_Value (key, "/stdio", &stdio_en);

    /* Make the count 1 if the device is marked STDIO */
    if(stdio_en == NU_TRUE)
    {
        /* Copy the stdio label into the array */
        status = DVS_Label_Append (all_labels, 5, all_labels, all_labels_cnt, &stdio_label, 1);

        /* Increment label count */
        all_labels_cnt += 1;
    }

    /* Get a unique device label if attached */
    if (REG_Get_Bytes_Value(key, "/labels/instance_label", (UINT8 *)(&reg_label), sizeof(DV_DEV_LABEL)) == NU_SUCCESS)
    {
        /* Copy the stdio label into the array */
        status = DVS_Label_Append (all_labels, 5, all_labels, all_labels_cnt, &reg_label, 1);

        /* Increment label count */
        all_labels_cnt += 1;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /******************************************/
    /* PLACE DEVICE IN KNOWN, LOW-POWER STATE */
    /******************************************/

    /* Default state */
    Serial_Tgt_Pwr_Default_State (instance_handle);

    /********************************/
    /* INITIALIZE AS POWER DEVICE   */
    /********************************/
    status = PMI_Device_Initialize(&(instance_handle->pmi_dev), key, all_labels,
                                   &all_labels_cnt, SERIAL_UII_BASE);

    if (status == NU_SUCCESS)
    {
        /* Setup the power device */
        PMI_Device_Setup(instance_handle->pmi_dev, &Serial_Tgt_Pwr_Set_State, SERIAL_POWER_BASE,
                         SERIAL_TOTAL_STATE_COUNT, &(instance_handle->dev_id), (VOID*)instance_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

        /* Perform DVFS related setup */
        status = PMI_DVFS_Setup(instance_handle->pmi_dev, key, (VOID*)instance_handle,
                                &Serial_Tgt_Pwr_Notify_Park1, &Serial_Tgt_Pwr_Notify_Park2,
                                &Serial_Tgt_Pwr_Notify_Resume1, &Serial_Tgt_Pwr_Notify_Resume2,
                                &Serial_Tgt_Pwr_Notify_Resume3);
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
    }

#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

    
    
    if (status == NU_SUCCESS)
    {
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))       
        TCCT_Schedule_Lock();
#endif        
        /********************************/
        /* REGISTER WITH DM             */
        /********************************/
        status = DVC_Dev_Register(instance_handle, all_labels,
                                  all_labels_cnt, &uart_drv_funcs, &(instance_handle->dev_id));

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                             
        /* Get default power state */
        init_pwr_state = PMI_STATE_GET(instance_handle->pmi_dev);

        /* Trace log */
        T_DEV_NAME((CHAR*)key, init_pwr_state, SERIAL_TOTAL_STATE_COUNT, instance_handle->dev_id);

        TCCT_Schedule_Unlock();
#endif        
    }  

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Serial_Dv_Unregister
*
*   DESCRIPTION
*
*       This function unregisters the hardware
*
*   INPUTS
*
*       CHAR         *key                   - Path to registry
*       INT          startstop              - Option to Register or Unregister
*       DV_DEV_ID    *dev_id                - Device ID
*
*   OUTPUTS
*
*       STATUS       status                 - NU_SUCCESS or error code
*
*************************************************************************/
STATUS   Serial_Dv_Unregister (const CHAR *key, INT startstop, DV_DEV_ID dev_id)
{
    STATUS                status = NU_SUCCESS;
    SERIAL_INSTANCE_HANDLE *inst_handle;

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
            /* Place the device in low-power state */
            Serial_Tgt_Pwr_Default_State(inst_handle);

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
*       Serial_Dv_Open
*
*   DESCRIPTION
*
*       This function opens the device and creates a session handle
*
*   INPUTS
*
*       VOID            *instance_handle    - Device ID
*       DV_DEV_LABEL    labels_list[]       - Access mode (label) of open
*       UINT32          labels_cnt          - Number of labels
*       VOID            **session_handle    - Pointer to Pointer of session handle
*
*   OUTPUTS
*
*       STATUS          success             - 0 for success, negative for failure
*
*************************************************************************/
STATUS Serial_Dv_Open(VOID *serial_inst_ptr_void, DV_DEV_LABEL label_list[],
                      INT label_cnt, VOID* *session_handle)
{
    SERIAL_INSTANCE_HANDLE  *serial_inst_ptr = (SERIAL_INSTANCE_HANDLE*)serial_inst_ptr_void;
    VOID                    *pointer = NU_NULL;
    SERIAL_SESSION_HANDLE   *ses_ptr = NU_NULL;
    UINT32                  open_mode_requests = 0;
    STATUS                  status = SERIAL_SESSION_NOT_AVAILABLE;
    INT                     int_level;
    DV_DEV_LABEL            serial_label = {SERIAL_LABEL};
    NU_MEMORY_POOL          *sys_pool_ptr;

    /* Check if the label list contains the serial label */
    if (DVS_Label_List_Contains (label_list, label_cnt, serial_label) == NU_SUCCESS)
    {
        open_mode_requests |= SERIAL_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /* Call the Power device open function */
    status = PMI_Device_Open (&open_mode_requests, label_list, label_cnt);
#endif

    /* If device is already open AND if the open request contains serial mode, return a error. */
    if (!((serial_inst_ptr->device_in_use == NU_TRUE) && (open_mode_requests & SERIAL_OPEN_MODE)))
    {
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Allocate a new session */
            status = NU_Allocate_Memory (sys_pool_ptr, &pointer,
                                         sizeof(SERIAL_SESSION_HANDLE), NU_NO_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {
            (VOID)memset(pointer, 0, sizeof(SERIAL_SESSION_HANDLE));
            
            ses_ptr = (SERIAL_SESSION_HANDLE*)pointer;

            /* Initialize session handle */
            ses_ptr->instance_ptr = serial_inst_ptr;           

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))

            /* If open mode request is UII and the previous operation is successful */
            if ((open_mode_requests & UII_OPEN_MODE) && (status == NU_SUCCESS))
            {
                /* Allocate memory */
                status = NU_Allocate_Memory (sys_pool_ptr, &pointer, SERIAL_UII_HISR_STK_SIZE, NU_NO_SUSPEND);

                if(status == NU_SUCCESS)
                {
                    /* Save off the stack pointer */
                    serial_inst_ptr->rx_hisr_stk_ptr = pointer;

                    /* Clear the memory we just allocated */
                    (VOID)memset((VOID*)pointer, 0, SERIAL_UII_HISR_STK_SIZE);

                    /* Create RX HISR */
                    status = NU_Create_HISR (&(serial_inst_ptr->rx_hisr), "Watchdog RX HISR", Serial_Dv_Pwr_RX_HISR,
                                             0, pointer, SERIAL_UII_HISR_STK_SIZE);

                    if (status == NU_SUCCESS)
                    {
                        serial_inst_ptr->rx_hisr.tc_app_reserved_1 = (UNSIGNED)(serial_inst_ptr)->pmi_dev;
                    }

                    else
                    {
                        (VOID)NU_Deallocate_Memory(pointer);
                    }
                }
            }
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)) */

            /* If the open mode request is serial and the previous operation is successful */
            if ((open_mode_requests & SERIAL_OPEN_MODE) && (status == NU_SUCCESS))
            {
                /* Disable interrupts while de-initializing hardware */
                int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

                /* Configure the transfer properties of the device */
                Serial_Tgt_Setup(serial_inst_ptr, &(serial_inst_ptr->attrs));

                if((serial_inst_ptr->attrs.rx_mode) == USE_IRQ)
                {
                    /* Enable RX interrupt on controller. */
                    Serial_Tgt_Rx_Int_Enable (serial_inst_ptr);
                }

                /* Enable processor level interrupts, register ISR */
                Serial_PR_Int_Enable(ses_ptr);

                /* Set Device in use flag to true */
                serial_inst_ptr->device_in_use = NU_TRUE;

                /* Restore interrupts to previous level */
                NU_Local_Control_Interrupts(int_level);
            }

            if (status == NU_SUCCESS)
            {
                serial_inst_ptr->open_modes |= open_mode_requests;

                /* Place session pointer in handle that can be returned */
                *session_handle = ses_ptr;
            }
        } /* If session is available for use */
    }

    else
    {
        /* Already opened in Serial mode */
        status = SERIAL_ALREADY_OPEN;
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Serial_DV_Close
*
*   DESCRIPTION
*
*       This function closes the device and deletes the session handle
*
*   INPUTS
*
*       VOID    *sess_handle                - Session handle of the device
*
*   OUTPUTS
*
*       STATUS   status                     - NU_SUCCESS or error code
*
*************************************************************************/
STATUS Serial_Dv_Close(VOID *session_ptr_void)
{
    SERIAL_INSTANCE_HANDLE *inst_ptr;
    SERIAL_SESSION_HANDLE  *session_ptr;
    STATUS      status = NU_SUCCESS;
    INT         int_level;

    /* If a valid session, then close it */
    if(session_ptr_void != NU_NULL)
    {
        /* Initialize local variables */
        session_ptr = (SERIAL_SESSION_HANDLE*)session_ptr_void;
        inst_ptr = session_ptr->instance_ptr;

        if((inst_ptr->open_modes & SERIAL_OPEN_MODE) == SERIAL_OPEN_MODE)
        {
            /* Disable interrupts before clearing shared variable */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Set device is closed */
            inst_ptr->device_in_use = NU_FALSE;

            /* Disable */
            Serial_Tgt_Disable(inst_ptr);

            /* Disable processor level interrupts */
            Serial_PR_Int_Disable(inst_ptr);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            status = PMI_Device_Close((inst_ptr->pmi_dev));

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
                
            /* Restore interrupts to previous level */
            NU_Local_Control_Interrupts(int_level);
        }

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))

        if((inst_ptr->open_modes & UII_OPEN_MODE) == UII_OPEN_MODE)
        {
            /* Delete the UII HISR */
            NU_Delete_HISR(&(inst_ptr->rx_hisr));

            /* Deallocate memory for UII HISR stack */
            NU_Deallocate_Memory(inst_ptr->rx_hisr_stk_ptr);
        }
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)) */
        
        /* Update open mode flags. */
        inst_ptr->open_modes &= ~(inst_ptr->open_modes);

        /* Free the session */
        NU_Deallocate_Memory (session_ptr);
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Serial_DV_Ioctl
*
*   DESCRIPTION
*
*       This function provides IOCTL functionality.
*
*   INPUTS
*
*       VOID      *sess_handle_ptr          - Session handle of the driver
*       UINT32    ioctl_cmd                 - Ioctl command
*       VOID      *data                     - Ioctl data pointer
*       UINT32    length                    - Ioctl length
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS
*                                           - or error code
*
*************************************************************************/
STATUS Serial_Dv_Ioctl(VOID *sess_handle_ptr_void, INT ioctl_cmd, VOID *data, INT length)
{
    SERIAL_SESSION_HANDLE   *sess_handle_ptr = (SERIAL_SESSION_HANDLE*)sess_handle_ptr_void;
    SERIAL_INSTANCE_HANDLE  *inst_ptr = sess_handle_ptr->instance_ptr;
    DV_IOCTL0_STRUCT        *ioctl0;
    STATUS                  status = NU_SUCCESS;
    DV_DEV_LABEL            serial_label = {SERIAL_LABEL};

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    PMI_DEV_HANDLE          pmi_dev = inst_ptr->pmi_dev;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

    STATUS                  pm_status = PM_UNEXPECTED_ERROR;
    UINT8                   op_pt;

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* Process command */
    switch (ioctl_cmd)
    {
        case DV_IOCTL0:

            if (length == sizeof(DV_IOCTL0_STRUCT))
            {
                ioctl0 = data;
                status = DV_IOCTL_INVALID_MODE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, data, length, inst_ptr,
                                          inst_ptr->open_modes);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

                if (status != NU_SUCCESS)
                {
                    /* If the mode requested is supported and if the session was opened for that mode */
                    if ((DV_COMPARE_LABELS (&(ioctl0->label), &serial_label)) &&
                        (inst_ptr->open_modes & SERIAL_OPEN_MODE))
                    {
                        ioctl0->base = IOCTL_SERIAL_BASE;
                        status = NU_SUCCESS;
                    }
                }
            }

            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }

            break;


        case (IOCTL_SERIAL_BASE + SERIAL_COMP_GET_ATTR_CMD):

            /* Get dev attributes */
            if(length == sizeof(SERIAL_SESSION *))
            {
                /* Save new serial driver attributes. */
                ((SERIAL_SESSION *)data)->baud_rate = inst_ptr->attrs.baud_rate;
                ((SERIAL_SESSION *)data)->data_bits = inst_ptr->attrs.data_bits;
                ((SERIAL_SESSION *)data)->flow_ctrl = inst_ptr->attrs.flow_ctrl;
                ((SERIAL_SESSION *)data)->parity = inst_ptr->attrs.parity;
                ((SERIAL_SESSION *)data)->rx_mode = inst_ptr->attrs.rx_mode;
                ((SERIAL_SESSION *)data)->tx_mode = inst_ptr->attrs.tx_mode;
                ((SERIAL_SESSION *)data)->stop_bits = inst_ptr->attrs.stop_bits;
            }

            else
            {
                status = NU_SERIAL_IOCTL_INVALID_LENGTH;
            }


            break;


        case (IOCTL_SERIAL_BASE + SERIAL_COMP_SET_ATTR_CMD):

            if(length == sizeof(SERIAL_SESSION *))
           {
                /* Save new serial driver attributes. */
                inst_ptr->attrs.baud_rate = ((SERIAL_SESSION *)data)->baud_rate;
                inst_ptr->attrs.data_bits = ((SERIAL_SESSION *)data)->data_bits;
                inst_ptr->attrs.flow_ctrl = ((SERIAL_SESSION *)data)->flow_ctrl;
                inst_ptr->attrs.parity = ((SERIAL_SESSION *)data)->parity;
                inst_ptr->attrs.rx_mode = ((SERIAL_SESSION *)data)->rx_mode;
                inst_ptr->attrs.tx_mode = ((SERIAL_SESSION *)data)->tx_mode;
                inst_ptr->attrs.stop_bits = ((SERIAL_SESSION *)data)->stop_bits;

                /* Stop device */
                Serial_Tgt_Disable (inst_ptr);

                /* Reconfigure */
                Serial_Tgt_Setup(inst_ptr, &(inst_ptr->attrs));

                /* Set baud rate */
                Serial_Tgt_Baud_Rate_Set (inst_ptr, inst_ptr->attrs.baud_rate);

                /* Restart device */
                Serial_Tgt_Enable (inst_ptr);
            }

            else
            {
                status = NU_SERIAL_IOCTL_INVALID_LENGTH;
            }


            break;

        case (IOCTL_SERIAL_BASE + SERIAL_GET_MW_SETTINGS_PATH):

            if(length >= REG_MAX_KEY_LENGTH)
            {
                /* Return the middleware config path */
                strcpy(data, inst_ptr->reg_path);
                strcat(data, "/mw_settings");
                status = NU_SUCCESS;
            }

            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }

            break;

        case (IOCTL_SERIAL_BASE+SERIAL_COMP_GET_TX_MODE):

            if(length == sizeof(UINT32))
            {
                *(UINT32*)data = inst_ptr->attrs.tx_mode;
            }

            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }

            break;

        case (IOCTL_SERIAL_BASE+SERIAL_COMP_GET_RX_MODE):

            if(length == sizeof(UINT32))
            {
                *(UINT32*)data = inst_ptr->attrs.rx_mode;
            }

            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }

            break;

        case (IOCTL_SERIAL_BASE + SERIAL_DEV_SET_MW_CB):

            if(length == sizeof(SERIAL_SESSION))
            {
                sess_handle_ptr->ser_mw_ptr = data;

            }

            else
            {
                status = NU_SERIAL_IOCTL_INVALID_LENGTH;
            }

            break;

        case (IOCTL_SERIAL_BASE + SERIAL_DEV_ENABLE):

            /* If SERIAL session AND If the device state is ON AND the device is not Parked */
            if (inst_ptr->open_modes & SERIAL_OPEN_MODE)
            {

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))

                if(((PMI_STATE_GET(pmi_dev) == SERIAL_ON) || (PMI_STATE_GET(pmi_dev) == POWER_ON_STATE)) &&
                    (PMI_IS_PARKED(pmi_dev) == NU_FALSE) )
                {
                    /* Determine the min op pt for the device's baud rate */
                    status = Serial_Tgt_Pwr_Min_OP_Pt_Calc(inst_ptr, (inst_ptr->attrs.baud_rate), &op_pt);

                    if(status == NU_SUCCESS)
                    {
                        /* First ensure that DVFS is at min op before enabled */
                        pm_status = PMI_REQUEST_MIN_OP(op_pt, pmi_dev);

                        if(pm_status == NU_SUCCESS)
                        {
                            /* Update MPL*/
                            (VOID)PMI_DVFS_Update_MPL_Value(pmi_dev, PM_NOTIFY_ON);

                        }
                    }
                }
                if (pm_status == NU_SUCCESS)
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)) */
                {
                    /* Set baud rate */
                    Serial_Tgt_Baud_Rate_Set (inst_ptr, inst_ptr->attrs.baud_rate);

                    /* Enable */
                    Serial_Tgt_Enable (inst_ptr);
                }
            }

            break;

        case (IOCTL_SERIAL_BASE + SERIAL_COMP_IS_TXBSY_CMD):

            if(length == sizeof(INT))
            {
                *(INT*)data = Serial_Tgt_Tx_Busy (inst_ptr);
            }

            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }

            break;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

        case (IOCTL_SERIAL_BASE + SERIAL_PWR_HIB_RESTORE):

            /* Call hibernate restore for serial session. */
            status = Serial_Tgt_Pwr_Hibernate_Restore(sess_handle_ptr);

            break;

    #endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

        default:

            /* Call the PMI IOCTL function for Power and UII IOCTLs */
            status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, data, length, inst_ptr,
                                      inst_ptr->open_modes);

            break;

#else

        default:

            status = DV_INVALID_INPUT_PARAMS;

            break;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    }

    return (status);
}


#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE))

/**************************************************************************
*
* FUNCTION
*
*       Serial_Dv_Pwr_RX_HISR
*
* DESCRIPTION
*
*       This function is responsible for resetting a RX watchdog.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       None
*
**************************************************************************/
VOID Serial_Dv_Pwr_RX_HISR(VOID)
{
    NU_HISR         *hcb;
    PMI_DEV_HANDLE  pmi_dev;

    /* Get the HISR control block */
    hcb = (NU_HISR*)NU_Current_HISR_Pointer();

    if(hcb != NU_NULL)
    {
        /* Get the serial port structure saved off in HISR control block */
        pmi_dev = (PMI_DEV_HANDLE)hcb->tc_app_reserved_1;

        /* Reset the watchdog */
        (VOID)PMI_Reset_Watchdog(pmi_dev);
    }
}
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)) */
