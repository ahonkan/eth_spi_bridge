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
*       i2c_dv_interface.c
*
*   COMPONENT
*
*       I2C                                 - I2C controller driver
*
*   DESCRIPTION
*
*       This file contains the generic I2C library functions.
*
*   FUNCTIONS
*
*       I2C_Dv_Register
*       I2C_Dv_Unregister
*       I2C_Dv_Open
*       I2C_Dv_Close
*       I2C_Dv_Ioctl
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       i2c_driver_defs.h
*       reg_api.h
*       power_core.h
*       power_interface.h
*       i2c.h
*       i2c_dv_interface.h
*       i2c_common.h
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "connectivity/nu_connectivity.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "services/nu_trace_os_mark.h"

/* Functions required to be implemented by driver. */
extern STATUS I2C_Tgt_Init(VOID *session_handle);
extern STATUS I2C_Tgt_Read (VOID* session_handle, VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_read);
extern STATUS I2C_Tgt_Write(VOID* session_handle, const VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_written);
extern STATUS I2C_Tgt_Send_Start_Address(I2C_INSTANCE_HANDLE *i_handle, VOID *data);
extern STATUS I2C_Tgt_Send_Restart_Address(I2C_INSTANCE_HANDLE *i_handle, VOID *data);
extern STATUS I2C_Tgt_Send_Address2(I2C_INSTANCE_HANDLE *i_handle, VOID *data);
extern STATUS I2C_Tgt_Send_Stop(I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Tgt_Send_Ack(I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Tgt_Send_Nack(I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Tgt_Check_Data_Ack(I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Tgt_Check_Bus_Free(I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Tgt_Check_Address_Ack(I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Tgt_Set_Node_Mode_RX(I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Tgt_Set_Node_Mode_TX(I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Tgt_Free_Bus(I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Tgt_Set_Slave_Address(I2C_INSTANCE_HANDLE *i_handle);
extern STATUS I2C_Tgt_Set_Master_Mode(I2C_INSTANCE_HANDLE *i_handle);
#if (CFG_NU_OS_CONN_I2C_NODE_TYPE == 1)
extern STATUS I2C_Tgt_Set_Slave_Mode(I2C_INSTANCE_HANDLE *i_handle);
#endif /* CFG_NU_OS_CONN_I2C_NODE_TYPE == 1 */
extern STATUS I2C_Tgt_Driver_Set_Baudrate (I2C_INSTANCE_HANDLE *i_handle, UINT16 baud_rate);
extern VOID   I2C_Tgt_Enable_Interrupt(I2C_INSTANCE_HANDLE *i_handle);
extern VOID   I2C_Tgt_Disable_Interrupt(I2C_INSTANCE_HANDLE *i_handle);
extern VOID   I2C_Tgt_Enable_Device (I2C_INSTANCE_HANDLE *i_handle);
extern VOID   I2C_Tgt_Disable_Device (I2C_INSTANCE_HANDLE *i_handle);
extern VOID   I2C_Tgt_Shutdown (I2C_INSTANCE_HANDLE *i_handle);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

extern VOID      I2C_Tgt_Pwr_Default_State (I2C_INSTANCE_HANDLE *inst_handle);
extern STATUS    I2C_Tgt_Pwr_Set_State (VOID *inst_handle, PM_STATE_ID *state);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

extern STATUS I2C_Tgt_Pwr_Pre_Park(VOID *instance_handle);
extern STATUS I2C_Tgt_Pwr_Post_Park(VOID *instance_handle);
extern STATUS I2C_Tgt_Pwr_Pre_Resume(VOID *instance_handle);
extern STATUS I2C_Tgt_Pwr_Post_Resume(VOID *instance_handle);
extern STATUS I2C_Tgt_Pwr_Resume_End(VOID *instance_handle);

#endif

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

extern STATUS I2C_Tgt_Pwr_Hibernate_Restore (I2C_SESSION_HANDLE * session_handle);

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

#define MW_CONFIG_PATH  ("/mw_settings")

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Dv_Register
*
*   DESCRIPTION
*
*       This function registers the I2C device.
*
*   INPUTS
*
*       key                                 - Path to registry
*       instance_handle                     - Serial instance structure
*
*   OUTPUTS
*
*       status                              - status
*
*************************************************************************/
STATUS  I2C_Dv_Register (const CHAR * key, I2C_INSTANCE_HANDLE *instance_handle)
{
    STATUS          status = NU_SUCCESS;
    DV_DEV_LABEL    all_labels[3] = {{I2C_LABEL}};
    INT             all_labels_cnt = 1;
    DV_DEV_LABEL    reg_label;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif    

    /* DVR function pointers */
    DV_DRV_FUNCTIONS i2c_drv_funcs =
    {
        I2C_Dv_Open,
        I2C_Dv_Close,
        I2C_Tgt_Read,
        I2C_Tgt_Write,
        I2C_Dv_Ioctl
    };

    /* Get a unique device label if attached */
    status = REG_Get_Bytes_Value(key, "/labels/instance_label", (UINT8 *)(&reg_label), sizeof(DV_DEV_LABEL));

    if(status == NU_SUCCESS)
    {
        /* Copy the instance label into the array */
        status = DVS_Label_Append (all_labels, 5, all_labels, all_labels_cnt, &reg_label, 1);

        /* Increment label count */
        all_labels_cnt += 1;
    }

    /* Set status successful in case unique device label is not attached. */
    status = NU_SUCCESS;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /******************************************/
    /* PLACE DEVICE IN KNOWN, LOW-POWER STATE */
    /******************************************/

    /* Default state */
    I2C_Tgt_Pwr_Default_State (instance_handle);

    /********************************/
    /* INITIALIZE AS POWER DEVICE   */
    /********************************/
    status = PMI_Device_Initialize(&(instance_handle->pmi_dev), key, all_labels,
                                   &all_labels_cnt, NU_NULL);

    if (status == NU_SUCCESS)
    {
        /* Setup the power device */
        PMI_Device_Setup(instance_handle->pmi_dev, &I2C_Tgt_Pwr_Set_State, I2C_POWER_BASE,
                         I2C_TOTAL_POWER_STATE_COUNT, &(instance_handle->dev_id), (VOID*)instance_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

        /* Perform DVFS related setup */
        status = PMI_DVFS_Setup(instance_handle->pmi_dev, key, (VOID*)instance_handle,
                       &I2C_Tgt_Pwr_Pre_Park, &I2C_Tgt_Pwr_Post_Park,
                       &I2C_Tgt_Pwr_Pre_Resume, &I2C_Tgt_Pwr_Post_Resume,
                       &I2C_Tgt_Pwr_Resume_End);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
    }

#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

    /********************************/
    /* REGISTER WITH DEVICE MANAGER */
    /********************************/
    if (status == NU_SUCCESS)
    {
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))       
        TCCT_Schedule_Lock();
#endif
        /* Register this device with the Device Manager. */
        status = DVC_Dev_Register(instance_handle,
                                  all_labels,
                                  all_labels_cnt,
                                  &i2c_drv_funcs,
                                  &(instance_handle->dev_id));

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                    
        /* Get default power state */
        init_pwr_state = PMI_STATE_GET(instance_handle->pmi_dev); 
                           
        /* Trace log */
        T_DEV_NAME((CHAR*)key, init_pwr_state, I2C_TOTAL_POWER_STATE_COUNT, instance_handle->dev_id);

        TCCT_Schedule_Unlock();
#endif        
    } 
    
    /* Return completion status of the service. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Dv_Unregister
*
*   DESCRIPTION
*
*       This function unregisters the I2C hardware
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
STATUS   I2C_Dv_Unregister (const CHAR *key, INT startstop, DV_DEV_ID dev_id)
{
    STATUS                status = NU_SUCCESS;
    I2C_INSTANCE_HANDLE  *i_handle;

    /*****************************************/
    /* UNREGISTER DEVICE WITH DEVICE MANAGER */
    /*****************************************/

    /* Unregister the device with Device Manager */
    status = DVC_Dev_Unregister(dev_id, (VOID**)&i_handle);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /*****************************************/
    /* UNREGISTER DEVICE WITH PMI            */
    /*****************************************/
    
    if(status == NU_SUCCESS)
    {
        /* Place the device in low-power state */
        I2C_Tgt_Pwr_Default_State(i_handle);

        /* Unregister the device from PMI. */
        status = PMI_Device_Unregister(i_handle->pmi_dev);
    }
    
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /***********************************/
    /* FREE THE INSTANCE HANDLE */
    /***********************************/
    if ((status == NU_SUCCESS)  && (i_handle != NU_NULL))
    {
        /* Free the instance */
        status = NU_Deallocate_Memory (i_handle);
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Dv_Open
*
*   DESCRIPTION
*
*       This function creates a session handle
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
STATUS I2C_Dv_Open (VOID *i2c_inst_ptr_void, DV_DEV_LABEL labels_list[],
                    INT labels_cnt, VOID* *session_handle)
{
    STATUS               status = NU_SUCCESS;
    I2C_SESSION_HANDLE  *s_handle = NU_NULL;
    I2C_INSTANCE_HANDLE *i_handle = (I2C_INSTANCE_HANDLE*)i2c_inst_ptr_void;
    UINT32               open_mode_requests = 0;
    DV_DEV_LABEL         i2c_label = {I2C_LABEL};
    NU_MEMORY_POOL      *sys_pool_ptr;

    /* Check if the label list contains the serial label */
    if (DVS_Label_List_Contains (labels_list, labels_cnt, i2c_label) == NU_SUCCESS)
    {
        open_mode_requests |= I2C_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /* Call the Power device open function */
    status = PMI_Device_Open (&open_mode_requests, labels_list, labels_cnt);
#endif

    /* Proceed only if:
        Either the device is not opened in I2C mode, .i.e. 'device_in_use' not set
        Or if this is open in power mode
    */
    if ((i_handle->device_in_use != NU_TRUE) || open_mode_requests)
    {
        /* Get system memory pool pointer */
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Allocate memory for session pointer */
            status = NU_Allocate_Memory(sys_pool_ptr, (VOID**)&s_handle,
                                         sizeof(I2C_SESSION_HANDLE), NU_NO_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {
            (VOID)memset ((VOID*)s_handle, 0, sizeof (I2C_SESSION_HANDLE));
            
            /* Place a pointer to instance handle in session handle */
            s_handle->instance_handle = i_handle;

            /* If the open mode request is I2C */
            if (open_mode_requests & I2C_OPEN_MODE)
            { 
                I2C_Tgt_Init (s_handle);
                i_handle->device_in_use = NU_TRUE;
            }
            s_handle->open_modes = open_mode_requests;

            /* Set the return address of the session handle */
            *session_handle = (VOID*)s_handle;
        }
    }
    else
    {
        /* No session found */
        status = I2C_NO_SESSION_AVAILABLE;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Dv_Close
*
*   DESCRIPTION
*
*       This function deletes the session handle
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
STATUS   I2C_Dv_Close(VOID *session_ptr_void)
{
    STATUS     status = NU_SUCCESS;
    I2C_SESSION_HANDLE  *s_handle;
    I2C_INSTANCE_HANDLE *i_handle;

    /* If a valid session, then close it */
    if(session_ptr_void != NU_NULL)
    {
        /* Initialize local variables */
        s_handle = (I2C_SESSION_HANDLE*)session_ptr_void;
        i_handle = s_handle->instance_handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

        (VOID)PMI_Device_Close(i_handle->pmi_dev);

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

        /* Check if device is in open mode */
        if ((s_handle->open_modes & I2C_OPEN_MODE) == I2C_OPEN_MODE)
        {
            /* Call shutdown function for I2C. */
            I2C_Tgt_Shutdown(i_handle);
            
            i_handle->device_in_use = NU_FALSE;
        }

        /* Free the allocated session space */
        NU_Deallocate_Memory (s_handle);

    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Dv_Ioctl
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
*       STATUS   status                     - NU_SUCCESS
*                                           - or error code
*
*************************************************************************/
STATUS I2C_Dv_Ioctl(VOID *sess_handle_ptr_void, INT ioctl_cmd, VOID *data, INT length)
{
    I2C_SESSION_HANDLE   *s_handle = (I2C_SESSION_HANDLE*)sess_handle_ptr_void;
    I2C_INSTANCE_HANDLE  *i_handle = s_handle->instance_handle;
    I2C_CB               *i2c_cb = I2C_GET_CB_FROM_SESSION_HANDLE(s_handle);
    DV_IOCTL0_STRUCT     *ioctl0;
    STATUS               status = NU_SUCCESS;
    DV_DEV_LABEL         i2c_label = {I2C_LABEL};

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    PMI_DEV_HANDLE          pmi_dev = i_handle->pmi_dev;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* Process command */
    switch (ioctl_cmd)
    {
    case DV_IOCTL0:
    {

        if(length == sizeof(DV_IOCTL0_STRUCT))
        {
            ioctl0 = (DV_IOCTL0_STRUCT*)data;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, data, length, i_handle,
                                      s_handle->open_modes);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

            if (DV_COMPARE_LABELS (&(ioctl0->label), &i2c_label) &&
                         (s_handle->open_modes & I2C_OPEN_MODE))
            {
                ioctl0->base = I2C_MODE_IOCTL_BASE;
                status = NU_SUCCESS;
            }
        }
        else
        {
            status = DV_IOCTL_INVALID_LENGTH;
        }
        break;

    }

    /* Send start signal and address. Same operation in the case */
    /* of Restart command. */
    case I2C_MODE_IOCTL_BASE + I2C_SEND_START_ADDRESS:
        status = I2C_Tgt_Send_Start_Address(i_handle, data);
        break;

    case I2C_MODE_IOCTL_BASE + I2C_SEND_RESTART_ADDRESS:
        status = I2C_Tgt_Send_Restart_Address(i_handle, data);
        break;

    /* Send the second byte of 10-bit address type. */
    case I2C_MODE_IOCTL_BASE + I2C_SEND_ADDRESS2:

        status = I2C_Tgt_Send_Address2(i_handle, data);
        break;

    /* Send STOP signal to terminate transfer for the specified slave. */
    case I2C_MODE_IOCTL_BASE + I2C_SEND_STOP:

        status = I2C_Tgt_Send_Stop(i_handle);
        break;

    /* Send acknowledgment of data byte. */
    case I2C_MODE_IOCTL_BASE + I2C_SEND_ACK:

        status = I2C_Tgt_Send_Ack(i_handle);
        break;

    /* Don't send acknowledgment for the data byte transfer. */
    case I2C_MODE_IOCTL_BASE + I2C_SEND_NACK:

        status = I2C_Tgt_Send_Nack(i_handle);
        break;

    /* Check for acknowledgment of data byte. */
    case I2C_MODE_IOCTL_BASE + I2C_CHECK_DATA_ACK:

        status = I2C_Tgt_Check_Data_Ack(i_handle);
        break;

    /* Check if I2C bus is not occupied. */
    case I2C_MODE_IOCTL_BASE + I2C_CHECK_BUS_FREE:
        status = I2C_Tgt_Check_Bus_Free(i_handle);
        break;

    /* Set the baud rate for I2C network transfer. */
    case I2C_MODE_IOCTL_BASE + I2C_SET_BAUDRATE:

        /* Set baud rate for the network transfer. */
        status = I2C_Tgt_Driver_Set_Baudrate(i_handle, *((UINT16*)data));
        break;

    /* Check if slave has acknowledge the address reception/matching. */
    case I2C_MODE_IOCTL_BASE + I2C_CHECK_ADDRESS_ACK:

        status = I2C_Tgt_Check_Address_Ack(i_handle);
        break;

    /* Set node mode to receiver. */
    case I2C_MODE_IOCTL_BASE + I2C_SET_NODE_MODE_RX:

        status = I2C_Tgt_Set_Node_Mode_RX(i_handle);
        break;

    /* Set node mode to transmitter. */
    case I2C_MODE_IOCTL_BASE + I2C_SET_NODE_MODE_TX:

        status = I2C_Tgt_Set_Node_Mode_TX(i_handle);
        break;

    /* Make the I2C bus free by terminating transfer in progress, if any. */
    case I2C_MODE_IOCTL_BASE + I2C_FREE_BUS:

        status = I2C_Tgt_Free_Bus(i_handle);

        /* Set node state to idle. */
        i2c_cb->i2c_node_state = I2C_NODE_IDLE;

        break;

    /* Set slave address whether it is 7-bit or 10-bit. */
    case I2C_MODE_IOCTL_BASE + I2C_SET_SLAVE_ADDRESS:
        status = I2C_Tgt_Set_Slave_Address(i_handle);
        break;

    case I2C_MODE_IOCTL_BASE + I2C_ENABLE_INTERRUPT:
    {
        I2C_Tgt_Enable_Interrupt (i_handle);
        break;

    }
    case I2C_MODE_IOCTL_BASE + I2C_DISABLE_INTERRUPT:
    {
        I2C_Tgt_Disable_Interrupt (i_handle);
        break;

    }
    case I2C_MODE_IOCTL_BASE + I2C_ENABLE_DEVICE:
    {
        I2C_Tgt_Enable_Device(i_handle);
        break;

    }
    case I2C_MODE_IOCTL_BASE + I2C_DISABLE_DEVICE:
    {
        I2C_Tgt_Disable_Device(i_handle);
        break;

    }
    case I2C_MODE_IOCTL_BASE + I2C_SET_MASTER_MODE:
        status = I2C_Tgt_Set_Master_Mode(i_handle);
        break;

#if (CFG_NU_OS_CONN_I2C_NODE_TYPE == 1)
    case I2C_MODE_IOCTL_BASE + I2C_SET_SLAVE_MODE:
            status = I2C_Tgt_Set_Slave_Mode(i_handle);
            break;
#endif /* (CFG_NU_OS_CONN_I2C_NODE_TYPE == 1) */

    case I2C_MODE_IOCTL_BASE + I2C_SET_CONTROL_BLOCK:
    {
        I2C_GET_CB_FROM_SESSION_HANDLE(s_handle) = (I2C_CB*)data;
        break;
    }
    case I2C_MODE_IOCTL_BASE + I2C_GET_MW_CONFIG_PATH:
    {
        CHAR* config_path;

        if(length >= (strlen(i_handle->reg_path)+strlen(MW_CONFIG_PATH)+1))
        {
            config_path = (CHAR *) data;

             /* Return the middleware config path */
            strcpy(config_path, i_handle->reg_path);
            strcat(config_path, MW_CONFIG_PATH);
            status = NU_SUCCESS;
        }
        else
        {
           status = I2C_DEV_REG_PATH_TOO_LONG;
        }

        break;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

    case I2C_MODE_IOCTL_BASE + I2C_CMD_PWR_HIB_RESTORE:
    {
        /* Call hibernate restore for I2C session. */
        status = I2C_Tgt_Pwr_Hibernate_Restore(s_handle);
        break;
    }
#endif

#endif

    default:

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

        /* Call the PMI IOCTL function for Power and UII IOCTLs */
        status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, data, length, i_handle,
                                  s_handle->open_modes);

#else

        status = DV_INVALID_INPUT_PARAMS;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

        break;
    }

    /* Return the completion status of the service. */
    return (status);
}
