/**************************************************************************
*            Copyright 2007 Mentor Graphics Corporation
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
*       spi_dv_interface.c
*
* COMPONENT
*
*       SPI_DRIVER - Nucleus SPI Driver
*
* DESCRIPTION
*
*       This file contains the generic SPI DV interface 
*       library functions..
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       SPI_Dv_Register 
*       SPI_Dv_Unregister 
*       SPI_Dv_Open 
*       SPI_Dv_Close
*       SPI_Dv_Ioctl 
*
*   DEPENDENCIES
*
*       nucleus.h"
*       nu_kernel.h"
*       nu_services.h"
*       nu_drivers.h"
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "connectivity/nu_connectivity.h"
#include "drivers/nu_drivers.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "services/nu_trace_os_mark.h"

/**********************************/
/* EXTERNAL VARIABLE DECLARATIONS */
/**********************************/
extern NU_MEMORY_POOL       System_Memory;
#define MW_CONFIG_PATH  ("/mw_settings")

/**********************************/
/* EXTERNAL FUNCTION PROTOTYPES   */
/**********************************/
extern VOID        SPI_Tgt_Intr_Enable(SPI_INSTANCE_HANDLE* spi_inst_ptr);
extern VOID        SPI_Tgt_Intr_Disable(SPI_INSTANCE_HANDLE* spi_inst_ptr);
extern VOID        SPI_Tgt_Device_Enable(SPI_INSTANCE_HANDLE* spi_inst_ptr);
extern VOID        SPI_Tgt_Device_Disable(SPI_INSTANCE_HANDLE* spi_inst_ptr);
extern VOID        SPI_Tgt_Set_Driver_Mode(SPI_INSTANCE_HANDLE* spi_inst_ptr);
extern VOID        SPI_Tgt_LISR(INT vector);
extern VOID        SPI_Tgt_Initialize(SPI_INSTANCE_HANDLE* spi_inst_ptr);
extern VOID        SPI_Tgt_Shutdown (SPI_INSTANCE_HANDLE* spi_inst_ptr);
extern STATUS  SPI_Tgt_Read(SPI_INSTANCE_HANDLE *spi_inst_ptr, SPI_CB *spi_cb, UINT32 *buffer);
extern STATUS  SPI_Tgt_Write(SPI_INSTANCE_HANDLE * spi_inst_ptr, SPI_CB *spi_cb, UINT16 address,UINT32 data, BOOLEAN thread_context);
extern STATUS  SPI_Tgt_Set_SPI_Mode(SPI_INSTANCE_HANDLE *spi_inst_ptr, UINT16 address, UINT8 spi_clock_phase, UINT8 spi_clock_polarity);
extern STATUS  SPI_Tgt_Set_Transfer_Size(SPI_INSTANCE_HANDLE *spi_inst_ptr, UINT16 address, UINT8 spi_transfer_size);
extern STATUS  SPI_Tgt_Set_Baud_Rate(SPI_INSTANCE_HANDLE * spi_inst_ptr, UINT16 address,UINT32 baud_rate);
extern STATUS  SPI_Tgt_Set_Slave_Select_Polarity(SPI_INSTANCE_HANDLE *spi_inst_ptr, UINT16 address, UINT8 spi_ss_polarity);
extern STATUS  SPI_Tgt_Set_Bit_Order(SPI_INSTANCE_HANDLE *spi_inst_ptr, UINT16 address, UINT8 spi_bit_order);
extern STATUS  SPI_Tgt_Set_Slave_Attributes(SPI_INSTANCE_HANDLE *spi_inst_ptr, SPI_TRANSFER_CONFIG *spi_slaves_attribs, INT spi_master_mode);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

extern STATUS      SPI_Tgt_Pwr_Default_State (SPI_INSTANCE_HANDLE *spi_inst_ptr);
extern STATUS   SPI_Tgt_Pwr_Set_State(VOID* instance_handle, PM_STATE_ID *state);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

extern STATUS      SPI_Tgt_Pwr_Pre_Park(VOID *instance_handle);
extern STATUS      SPI_Tgt_Pwr_Post_Park(VOID *instance_handle);
extern STATUS      SPI_Tgt_Pwr_Pre_Resume(VOID *instance_handle);
extern STATUS      SPI_Tgt_Pwr_Post_Resume(VOID *instance_handle);
extern STATUS      SPI_Tgt_Pwr_Resume_End(VOID *instance_handle);

#endif

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

extern STATUS   SPI_Tgt_Pwr_Hibernate_Restore (SPI_DRV_SESSION * session_handle);

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */


#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

/**************************************************************************
* FUNCTION
*
*       SPI_Dv_Register
*
* DESCRIPTION
*
*       This function registers the SPI driver with Nucleus SPI.
*
* INPUTS
*
*       CHAR          *key                  - Key
*       INT           startstop             - Start or stop flag
*       DV_DEV_ID     *dev_id               - Returned Device ID
*
* OUTPUTS
*
*       STATUS        status                - NU_SUCCESS or error code
*
**************************************************************************/
STATUS SPI_Dv_Register (const CHAR * key, SPI_INSTANCE_HANDLE* spi_inst_ptr)
{
    STATUS                  status;
    DV_DEV_LABEL            standard_labels[5] = {{SPI_LABEL}};
    DV_DEV_LABEL            all_labels[DV_MAX_DEV_LABEL_CNT];
    DV_DEV_LABEL            user_label;
    INT                     std_label_cnt = 1;
    INT                     all_labels_cnt;
    INT                     user_label_cnt = 0;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif    

    /* DVR function pointers */
    DV_DRV_FUNCTIONS spi_drv_funcs =
    {
        SPI_Dv_Open,
        SPI_Dv_Close,
        NU_NULL,
        NU_NULL,
        SPI_Dv_Ioctl
    };

    spi_inst_ptr->dev_id = DV_INVALID_DEV;
    
    if((user_label_cnt + std_label_cnt) <= DV_MAX_DEV_LABEL_CNT)
    {
        /**************************/
        /* COMBINE ALL LABELS     */
        /**************************/

        /* Get the device specific label */
        status = REG_Get_Bytes_Value (key, "/labels/spi_label", (UINT8 *)&user_label, sizeof(DV_DEV_LABEL));
        if(status == NU_SUCCESS)
        {
            user_label_cnt = 1;
            
            memcpy(all_labels, standard_labels, std_label_cnt*sizeof (DV_DEV_LABEL));
            memcpy(all_labels+std_label_cnt, &user_label, user_label_cnt*sizeof (DV_DEV_LABEL));
            all_labels_cnt = std_label_cnt + user_label_cnt;
        }
        else
        {
            status = SPI_DEV_REGISTRY_ERROR;
        }


        if(status == NU_SUCCESS)
        {
        
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            /******************************************/
            /* PLACE DEVICE IN KNOWN, LOW-POWER STATE */
            /******************************************/
            SPI_Tgt_Pwr_Default_State(spi_inst_ptr);

            /********************************/
            /* INITIALIZE AS POWER DEVICE   */
            /********************************/
            status = PMI_Device_Initialize(&(spi_inst_ptr->pmi_dev), key, all_labels,
                                           &all_labels_cnt, NU_NULL);

            if (status == NU_SUCCESS)
            {
                /* Setup the power device */
                PMI_Device_Setup(spi_inst_ptr->pmi_dev, &SPI_Tgt_Pwr_Set_State, SPI_POWER_BASE,
                                 SPI_TOTAL_POWER_STATE_COUNT, &(spi_inst_ptr->dev_id), (VOID*)spi_inst_ptr);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

                /* Perform DVFS related setup */
                status = PMI_DVFS_Setup(spi_inst_ptr->pmi_dev, key, (VOID*)spi_inst_ptr,
                               &SPI_Tgt_Pwr_Pre_Park, &SPI_Tgt_Pwr_Post_Park,
                               &SPI_Tgt_Pwr_Pre_Resume, &SPI_Tgt_Pwr_Post_Resume,
                               &SPI_Tgt_Pwr_Resume_End);
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
            }
    
#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))   
            TCCT_Schedule_Lock();
#endif            
            /********************************/
            /* REGISTER WITH DM             */
            /********************************/
                status = DVC_Dev_Register(spi_inst_ptr,
                                          all_labels,
                                          all_labels_cnt,
                                          &spi_drv_funcs,
                                          &(spi_inst_ptr->dev_id));

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                   
        /* Get default power state */
        init_pwr_state = PMI_STATE_GET(spi_inst_ptr->pmi_dev);
                                         
            /* Trace log */
            T_DEV_NAME((CHAR*)key, init_pwr_state, SPI_TOTAL_POWER_STATE_COUNT, spi_inst_ptr->dev_id);
           
            TCCT_Schedule_Unlock();
#endif            
        }
        else
        {
            status = SPI_DRV_TGT_SETUP_FAILED;
        }
    }
    /* If an instance structure was obtained, but device was not registered for some reason
    then release instance structure */
    if (status != NU_SUCCESS)
    {
        INT     int_level;
        
        /* Disable interrupts before setting shared variable */
        int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* clear the instance structure */
        ESAL_GE_MEM_Clear ((VOID*)spi_inst_ptr, (INT)sizeof(SPI_INSTANCE_HANDLE));

        /* Restore interrupts to previous level */
        NU_Local_Control_Interrupts(int_level);
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       SPI_Dv_Unregister
*
* DESCRIPTION
*
*       This function un-registers the SPI driver with Nucleus SPI.
*
* INPUTS
*
*       key                                 - Device registry path
*       startstop                           - Start or Stop the device
*       dev_id                              - Device ID
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
**************************************************************************/
STATUS SPI_Dv_Unregister (const CHAR * key, INT startstop, DV_DEV_ID dev_id)
{
    STATUS              status;
    SPI_INSTANCE_HANDLE *spi_inst_ptr;

    /*****************************************/
    /* UNREGISTER DEVICE WITH DEVICE MANAGER */
    /*****************************************/

    /* Unregister the device with Device Manager */
    status = DVC_Dev_Unregister(dev_id, (VOID**)&spi_inst_ptr);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /*****************************************/
    /* UNREGISTER DEVICE WITH PMI            */
    /*****************************************/

    if(status == NU_SUCCESS)
    {
        /* Place the device in low-power state */
        SPI_Tgt_Pwr_Default_State(spi_inst_ptr);

        /* Unregister the device from PMI. */
        status = PMI_Device_Unregister(spi_inst_ptr->pmi_dev);
    }

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /***********************************/
    /* FREE THE INSTANCE HANDLE */
    /***********************************/

    NU_Deallocate_Memory ((VOID*) spi_inst_ptr);

    return status;
}

/**************************************************************************
* FUNCTION
*
*       SPI_Dv_Open
*
* DESCRIPTION
*
*       This function opens the device and creates a session handle.
*
* INPUTS
*
*       VOID            *inst_ptr           - Instance handle 
*       DV_DEV_LABEL    label_list[]        - Access mode (label) of open
*       INT             labels_cnt          - Number of labels
*       VOID            **session_handle    - Pointer to Pointer of session handle
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*       SPI_DRV_NO_SESSION_AVAILABLE
*       SPI_DEV_REGISTRY_ERROR
*       SPI_DEV_ALREADY_IN_USE
*       SPI_DRV_INVALID_OPEN_MODE
*       SPI_PMS_ERROR
*
**************************************************************************/
STATUS SPI_Dv_Open (VOID *inst_ptr, DV_DEV_LABEL label_list[],
                        INT label_cnt, VOID* *session_handle)
{
    STATUS              status = NU_SUCCESS;
    SPI_INSTANCE_HANDLE *spi_inst_ptr = inst_ptr;
    SPI_DRV_SESSION     *spi_ses_ptr;
    UINT32              open_mode_requests = 0;
    DV_DEV_LABEL        spi_label = {SPI_LABEL};
    VOID*               pointer;

    /* Get open mode requests from labels */
    if (DVS_Label_List_Contains (label_list, label_cnt, spi_label) == NU_SUCCESS)
    {
        open_mode_requests |= SPI_OPEN_MODE;
    }
    
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /* Call the Power device open function */
    status = PMI_Device_Open (&open_mode_requests, label_list, label_cnt);
#endif
     
    /* Proceed only if:
        Either the device is not opened in SPI mode, .i.e. 'device_in_use' not set
        Or if this is open in power mode
    */
    if (spi_inst_ptr->device_in_use != NU_TRUE || open_mode_requests)
    {
       /* Allocate a new session */
        status = NU_Allocate_Memory (&System_Memory, &pointer, 
                    sizeof(SPI_DRV_SESSION), NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            ESAL_GE_MEM_Clear (pointer, sizeof(SPI_DRV_SESSION));
            spi_ses_ptr = (SPI_DRV_SESSION*)pointer;
            
            /* Init session */
            spi_ses_ptr->spi_inst_ptr = spi_inst_ptr;
                
            /* If open mode request is SPI */
            if (open_mode_requests & SPI_OPEN_MODE)
            {
                /* Set device in use */
                spi_inst_ptr->device_in_use = NU_TRUE;

                /* Initialize device. */
                SPI_Tgt_Initialize(spi_inst_ptr);

                /* Set Master / Slave mode */
                SPI_Tgt_Set_Driver_Mode(spi_inst_ptr);
                
                /* Enable PR and device interrupts */
                SPI_PR_Intr_Enable(spi_ses_ptr);
            }
            
            if (status == NU_SUCCESS)
            {
                spi_ses_ptr->open_mode |= open_mode_requests;

                /* Return session handle */
                *(SPI_DRV_SESSION **)session_handle = spi_ses_ptr;
            }
        }
    }
    else
    {
        status = SPI_DEV_ALREADY_IN_USE;    
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       SPI_Dv_Close
*
* DESCRIPTION
*
*       This function stops the SPI controller associated with the
*       specified Nucleus SPI device.
*
* INPUTS
*
*      *sess_handle                         Nucleus SPI driver session
*                                           handle pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
**************************************************************************/
STATUS SPI_Dv_Close(VOID *sess_handle)
{
    STATUS              status = SPI_DRV_DEFAULT_ERROR;
    SPI_DRV_SESSION     *spi_ses_ptr = sess_handle;
    SPI_INSTANCE_HANDLE *spi_inst_ptr;
    INT                 int_level;

    /* If a valid session, then close it */
    if(spi_ses_ptr != NU_NULL)
    {
        spi_inst_ptr = spi_ses_ptr->spi_inst_ptr;
        
        if(spi_inst_ptr != NU_NULL)
        {
        	if (spi_ses_ptr->open_mode == SPI_OPEN_MODE)
            {
                /* Disable interrupts before clearing shared variable */
                int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);
        
                /* Call shutdown function for SPI. */
                SPI_Tgt_Shutdown(spi_inst_ptr);
                
                /* Set device is closed */
                spi_inst_ptr->device_in_use = NU_FALSE;
                
                /* Restore interrupts to previous level */
                NU_Local_Control_Interrupts(int_level);
                
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

                status = PMI_Device_Close((spi_inst_ptr->pmi_dev));

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
                
                status = NU_SUCCESS;
            }
            
            /* Free the session */
            NU_Deallocate_Memory ((VOID*)spi_ses_ptr);
        }
    }
    
    return status;
}

/**************************************************************************
* FUNCTION
*
*       SPI_Dv_Ioctl
*
* DESCRIPTION
*
*       This function is responsible for performing miscellaneous control
*       operations.
*
* INPUTS
*
*       *sess_ptr                           - Session handle of the driver
*       ioctl_cmd                           - Ioctl command
*       *ioctl_data                         - Ioctl data pointer
*       length                              - Ioctl length
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       SPI_UNSUPPORTED_CONTROLLER          The specified controller is
*                                           not supported.
*
**************************************************************************/
STATUS SPI_Dv_Ioctl (VOID *sess_ptr, INT ioctl_cmd, VOID *ioctl_data, INT length)
{
    STATUS                  status = NU_SUCCESS;
    SPI_TRANSFER_CONFIG     *spi_config;
    SPI_DRV_IOCTL_DATA      *spi_ioctl_data;
    SPI_DRV_IOCTL_WR_DATA   *spi_ioctl_wr_data;
    DV_IOCTL0_STRUCT        *ioctl0;
    SPI_DRV_SESSION         *spi_ses_ptr = (SPI_DRV_SESSION*)sess_ptr;
    SPI_INSTANCE_HANDLE     *spi_inst_ptr = spi_ses_ptr->spi_inst_ptr;
    SPI_CB                  *spi_cb;
    DV_DEV_LABEL            spi_label = {SPI_LABEL};
    
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    PMI_DEV_HANDLE          pmi_dev = spi_inst_ptr->pmi_dev;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* Determine the control operation to be performed. */
    switch(ioctl_cmd)
    {
        case DV_IOCTL0:
        {
            if(length == sizeof(DV_IOCTL0_STRUCT))
            {
                ioctl0 = ioctl_data;
                status = DV_IOCTL_INVALID_MODE;
                
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, ioctl_data, length, spi_inst_ptr,
                                          spi_ses_ptr->open_mode);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
                
                if (status != NU_SUCCESS)
                {
                    if (DV_COMPARE_LABELS (&(ioctl0->label), &spi_label) && 
                        (spi_ses_ptr->open_mode & SPI_OPEN_MODE))
                    {
                        ioctl0->base = SPI_MODE_IOCTL_BASE;
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
        
        case (SPI_MODE_IOCTL_BASE + SPI_GET_MW_CONFIG_PATH):
        {
           if((strlen(spi_inst_ptr->reg_path)+strlen(MW_CONFIG_PATH)+1) <= length)
           {
                /* Return the middleware config path */
                strcpy(ioctl_data, spi_inst_ptr->reg_path);
                strcat(ioctl_data, MW_CONFIG_PATH);
                status = NU_SUCCESS;
           }
           else
           {
               status = DV_IOCTL_INVALID_LENGTH;
           }
        
            break;
        }
        
        case (SPI_MODE_IOCTL_BASE+SPI_SET_CB):
        {
            if(length == sizeof(SPI_CB))
            {
                spi_ses_ptr->spi_inst_ptr->spi_cb = ioctl_data;
                status = NU_SUCCESS;
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }
        
        case (SPI_MODE_IOCTL_BASE+SPI_GET_SLAVE_CNT):
        {
            if(length == sizeof(UINT16))
            {
                *(UINT16*)ioctl_data = spi_ses_ptr->spi_inst_ptr->spi_slave_cnt;
                status = NU_SUCCESS;
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }
        
        case (SPI_MODE_IOCTL_BASE+SPI_GET_MASTER_SLAVE_MODE):
        {
            spi_cb = spi_ses_ptr->spi_inst_ptr->spi_cb;
            
            /* Fill in driver attributes */
            spi_cb->spi_master_mode = spi_inst_ptr->spi_master_mode;
            
            break;
        }
        
        case (SPI_MODE_IOCTL_BASE+SPI_GET_INTR_POLL_MODE):
        {
            spi_cb = spi_ses_ptr->spi_inst_ptr->spi_cb;
            
            /* Fill in driver attributes */
            spi_cb->spi_driver_mode = spi_inst_ptr->spi_driver_mode;
            
            break;
        }
        
        /* Set baud rate. This is applicable to SPI master devices only. */
        case (SPI_MODE_IOCTL_BASE+SPI_SET_BAUD_RATE):
        {
            if(length == sizeof(SPI_DRV_IOCTL_DATA))
            {
                spi_ioctl_data = ioctl_data;
                
                spi_config = spi_ioctl_data->xfer_attrs;
            
                /* Set new baud rate. */
                status = SPI_Tgt_Set_Baud_Rate(spi_inst_ptr, spi_ioctl_data->address,
                                                  spi_config->spi_baud_rate);
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }
    
        /* Set SPI mode. */
        case (SPI_MODE_IOCTL_BASE+SPI_SET_MODE):
        {
            if(length == sizeof(SPI_DRV_IOCTL_DATA))
            {
                spi_ioctl_data = ioctl_data;
                
                spi_config = spi_ioctl_data->xfer_attrs;         

                status = SPI_Tgt_Set_SPI_Mode(spi_inst_ptr, spi_ioctl_data->address,
                                                spi_config->spi_clock_phase,
                                                spi_config->spi_clock_polarity);
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }
      
        /* Set the slave-select polarity. */
        case (SPI_MODE_IOCTL_BASE+SPI_SET_SS_POLARITY):
        {
            if(length == sizeof(SPI_DRV_IOCTL_DATA))
            {
                spi_ioctl_data = ioctl_data;
                
                spi_config = spi_ioctl_data->xfer_attrs;
                
                status = SPI_Tgt_Set_Slave_Select_Polarity(spi_inst_ptr, spi_ioctl_data->address,
                                                    spi_config->spi_ss_polarity);
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }
        
        /* Set bit order. */
        case (SPI_MODE_IOCTL_BASE+SPI_SET_BIT_ORDER):
        {
            /* Bit order is not configurable so make sure we are trying to set the default
               bit order otherwise, its an error. */
            if(length == sizeof(SPI_DRV_IOCTL_DATA))
            {
                spi_ioctl_data = ioctl_data;
                
                spi_config = spi_ioctl_data->xfer_attrs;

                status = SPI_Tgt_Set_Bit_Order(spi_inst_ptr, spi_ioctl_data->address,
                                                spi_config->spi_bit_order);
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }
    
        /* Set transfer size. */
        case (SPI_MODE_IOCTL_BASE+SPI_SET_TRANSFER_SIZE):
        {
            if(length == sizeof(SPI_DRV_IOCTL_DATA))
            {
                spi_ioctl_data = ioctl_data;
                
                spi_config = spi_ioctl_data->xfer_attrs;
                
                status = SPI_Tgt_Set_Transfer_Size(spi_inst_ptr, spi_ioctl_data->address,
                                                        spi_config->spi_transfer_size);
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }
    
        /* Setup available salve addresses for the device. */
        case (SPI_MODE_IOCTL_BASE+SPI_SETUP_SLAVE_ADDRS):
        {
            spi_cb = spi_ses_ptr->spi_inst_ptr->spi_cb;
            
            status = SPI_Tgt_Set_Slave_Attributes(spi_inst_ptr, spi_cb->spi_slaves_attribs,
                                                        spi_cb->spi_master_mode);

            break;
        }
        
        /* Setup available salve addresses for the device. */
        case (SPI_MODE_IOCTL_BASE+SPI_ENABLE_DEVICE):
        {
            SPI_Tgt_Device_Enable(spi_ses_ptr->spi_inst_ptr);
    
            break;
        }
        
        case (SPI_MODE_IOCTL_BASE+SPI_DRV_READ):
        {
            if(length == sizeof(UINT32))
            {
                /* Call the existing Read function */
                status = SPI_Tgt_Read(spi_inst_ptr, spi_ses_ptr->spi_inst_ptr->spi_cb, 
                                        (UINT32*)ioctl_data);
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            
            break;
        }
        
        case (SPI_MODE_IOCTL_BASE+SPI_DRV_WRITE):
        {
            if(length == sizeof(SPI_DRV_IOCTL_WR_DATA))
            {
                spi_ioctl_wr_data = ioctl_data;
                
                /* Call the existing Write function*/
                status = SPI_Tgt_Write(spi_inst_ptr, spi_ses_ptr->spi_inst_ptr->spi_cb, 
                                        spi_ioctl_wr_data->address, spi_ioctl_wr_data->wr_data_val, 
                                        spi_ioctl_wr_data->thread_context);
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }

        case (SPI_MODE_IOCTL_BASE+SPI_APPLY_TRANSFER_ATTRIBS):
        {
            if(length == sizeof(SPI_DRV_IOCTL_DATA))
            {
                spi_ioctl_data = ioctl_data;

                spi_config = spi_ioctl_data->xfer_attrs;
                
                /* Apply Baud Rate settings. */
                status = SPI_Tgt_Set_Baud_Rate(spi_inst_ptr, spi_ioctl_data->address,
                                                  spi_config->spi_baud_rate);

                /* Apply Phase/Polarity settings. */
                if (status == NU_SUCCESS)
                {
                    status = SPI_Tgt_Set_SPI_Mode(spi_inst_ptr, spi_ioctl_data->address,
                                                    spi_config->spi_clock_phase,
                                                    spi_config->spi_clock_polarity);
                }

                /* Apply Transfer Size settings. */
                if (status == NU_SUCCESS)
                {
                    status = SPI_Tgt_Set_Transfer_Size(spi_inst_ptr, spi_ioctl_data->address,
                                                            spi_config->spi_transfer_size);
                }

                /* Apply Slave Select Polarity settings. */
                if (status == NU_SUCCESS)
                {
                    status = SPI_Tgt_Set_Slave_Select_Polarity(spi_inst_ptr, spi_ioctl_data->address,
                                                    spi_config->spi_ss_polarity);
                }

                /* Apply Bit Order settings. */
                if (status == NU_SUCCESS)
                {
                    status = SPI_Tgt_Set_Bit_Order(spi_inst_ptr, spi_ioctl_data->address,
                                                spi_config->spi_bit_order);
                }
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }

        case (SPI_MODE_IOCTL_BASE+SPI_GET_SIM_TX_ATTRIBS):
        {
            spi_cb = spi_ses_ptr->spi_inst_ptr->spi_cb;
            
            /* Fill in driver attributes */
            spi_cb->spi_sim_tx_attribs = spi_inst_ptr->spi_sim_tx_attribs;
            
            break;
        }
        
        case (SPI_MODE_IOCTL_BASE+SPI_HANDLE_CHIPSELECT):
        {
        
            if(length == sizeof(SPI_DRV_IOCTL_DATA))
            {
                spi_ioctl_data = ioctl_data;
                
                spi_config = spi_ioctl_data->xfer_attrs;
                
                spi_cb = spi_ses_ptr->spi_inst_ptr->spi_cb;
                
                spi_cb->spi_slaves_attribs[spi_ioctl_data->address].handle_chipselect = spi_config->handle_chipselect;
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }
        
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE  == NU_TRUE))

        case (SPI_MODE_IOCTL_BASE + SPI_PWR_HIB_RESTORE):

            /* Call hibernate restore for SPI session. */
            status = SPI_Tgt_Pwr_Hibernate_Restore(spi_ses_ptr);

            break;
#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE)) */
        
        default:
        {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            /* Call the PMI IOCTL function for Power and UII IOCTLs */
            status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, ioctl_data, length, spi_inst_ptr,
                                      spi_ses_ptr->open_mode);

#else

            status = DV_INVALID_INPUT_PARAMS;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
        
            break;
        }
    }

    /* Return the completion status of the service. */
    return (status);
}

