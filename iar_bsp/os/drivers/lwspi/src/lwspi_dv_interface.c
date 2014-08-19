/**************************************************************************
*            Copyright 2012 Mentor Graphics Corporation
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
*       lwspi_dv_interface.c
*
* COMPONENT
*
*       Lightweight SPI Driver.
*
* DESCRIPTION
*
*       This file contains the generic lightweight SPI driver interface.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       LWSPI_Dv_Register
*       LWSPI_Dv_Unregister
*       LWSPI_Dv_Open
*       LWSPI_Dv_Close
*       LWSPI_Dv_Ioctl
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
#include "bsp/drivers/lwspi/stm32/lwspi_tgt.h"

/* Global variables. */
extern NU_MEMORY_POOL   System_Memory;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern STATUS LWSPI_TGT_Pwr_Default_State(LWSPI_INSTANCE_HANDLE*);
extern STATUS LWSPI_TGT_Pwr_Set_State(VOID*, PM_STATE_ID*);
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
extern STATUS LWSPI_TGT_Pwr_Pre_Park(VOID*);
extern STATUS LWSPI_TGT_Pwr_Post_Park(VOID*);
extern STATUS LWSPI_TGT_Pwr_Pre_Resume(VOID*);
extern STATUS LWSPI_TGT_Pwr_Post_Resume(VOID*);
extern STATUS LWSPI_TGT_Pwr_Resume_End(VOID*);
#endif
#endif
extern VOID LWSPI_TGT_Initialize(LWSPI_INSTANCE_HANDLE*);
extern VOID LWSPI_TGT_Shutdown(LWSPI_INSTANCE_HANDLE*);
extern STATUS LWSPI_TGT_Configure(VOID*, NU_SPI_DEVICE*);
extern STATUS LWSPI_TGT_Read(VOID *, BOOLEAN, NU_SPI_IRP*);
extern STATUS LWSPI_TGT_Write(VOID *, BOOLEAN, NU_SPI_IRP*);
extern STATUS LWSPI_TGT_Write_Read(VOID *, BOOLEAN, NU_SPI_IRP*, NU_SPI_IRP*);

#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
extern VOID    LWSPI_TGT_Intr_Enable(LWSPI_INSTANCE_HANDLE*);
extern BOOLEAN LWSPI_TGT_ISR_Write_Read(VOID *, NU_SPI_IRP*, NU_SPI_IRP*);
extern BOOLEAN LWSPI_TGT_ISR_Read(VOID *, NU_SPI_IRP*);
extern BOOLEAN LWSPI_TGT_ISR_Write(VOID *, NU_SPI_IRP*);
#endif

extern STATUS LWSPI_TGT_DMA_Configure(NU_SPI_DEVICE *device);


/**************************************************************************
* FUNCTION
*
*       LWSPI_Dv_Register
*
* DESCRIPTION
*
*       This function registers the lightweight SPI driver with
*       device manager.
*
* INPUTS
*
*       key                                 Path to registery settings.
*       startstop                           Start or stop flag.
*       dev_id                              Returned Device ID.
*
* OUTPUTS
*
*       NU_SUCCESS                          Function completed
*                                           successfully.
*
**************************************************************************/
STATUS LWSPI_Dv_Register(const CHAR * key, LWSPI_INSTANCE_HANDLE *inst_ptr)
{
    STATUS                  status;
    DV_DEV_LABEL            lwspi_labels[DV_MAX_DEV_LABEL_CNT] = {{LWSPI_LABEL}};
    DV_DEV_LABEL            user_label;
    INT                     lwspi_label_cnt = 1;
    DV_DRV_FUNCTIONS        drv_funcs;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))
    UINT32          init_pwr_state;
#endif

    /* Get the device specific label. */
    status = REG_Get_Bytes_Value(key,
                                "/labels/spi_label",
                                (UINT8*)&user_label,
                                sizeof(DV_DEV_LABEL));
    if(status == NU_SUCCESS)
    {
        /* Copy the stdio label into the array */
        status = DVS_Label_Append (lwspi_labels,
                                DV_MAX_DEV_LABEL_CNT,
                                lwspi_labels,
                                lwspi_label_cnt,
                                &user_label, 1);
        if (status == NU_SUCCESS)
        {
            /* Increment label count */
            lwspi_label_cnt++;
        }
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /* Place device in known low-power state. */
    LWSPI_TGT_Pwr_Default_State(inst_ptr);

    /* Initialize as power device. */
    status = PMI_Device_Initialize(&(inst_ptr->pmi_dev),
                                    key,
                                    lwspi_labels,
                                    &lwspi_label_cnt,
                                    NU_NULL);
    if (status == NU_SUCCESS)
    {
        /* Setup the power device. */
        PMI_Device_Setup(inst_ptr->pmi_dev,
                        &LWSPI_TGT_Pwr_Set_State,
                        LWSPI_POWER_MODE_IOCTL_BASE,
                        LWSPI_POWER_STATE_COUNT,
                        &(inst_ptr->dev_id),
                        (VOID*)inst_ptr);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

        /* Perform DVFS related setup. */
        status = PMI_DVFS_Setup(inst_ptr->pmi_dev,
                                key,
                                (VOID*)inst_ptr,
                                &LWSPI_TGT_Pwr_Pre_Park,
                                &LWSPI_TGT_Pwr_Post_Park,
                                &LWSPI_TGT_Pwr_Pre_Resume,
                                &LWSPI_TGT_Pwr_Post_Resume,
                                &LWSPI_TGT_Pwr_Resume_End);
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
    }
#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

    if(status == NU_SUCCESS)
    {
        drv_funcs.drv_open_ptr  = LWSPI_Dv_Open;
        drv_funcs.drv_close_ptr = LWSPI_Dv_Close;
        drv_funcs.drv_read_ptr  = NU_NULL;
        drv_funcs.drv_write_ptr = NU_NULL;
        drv_funcs.drv_ioctl_ptr = LWSPI_Dv_Ioctl;

        /* Reset value of device ID. */
        inst_ptr->dev_id = DV_INVALID_DEV;
        
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))   
        TCCT_Schedule_Lock();
#endif        
        /* Register with device manager. */
        status = DVC_Dev_Register(inst_ptr,
                                  lwspi_labels,
                                  lwspi_label_cnt,
                                  &drv_funcs,
                                  &(inst_ptr->dev_id));
                                  
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                              
        /* Get default power state */
        init_pwr_state = PMI_STATE_GET(inst_ptr->pmi_dev);    
                                  
        /* Trace log */
        T_DEV_NAME((CHAR*)key, init_pwr_state, LWSPI_POWER_STATE_COUNT, inst_ptr->dev_id);
      
        TCCT_Schedule_Unlock();     
#endif        
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_Dv_Unregister
*
* DESCRIPTION
*
*       This function un-registers the lightweight SPI driver with
*       device manager.
*
* INPUTS
*
*       key                                 Device registry path.
*       startstop                           Start or Stop the device.
*       dev_id                              Device ID.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
**************************************************************************/
STATUS LWSPI_Dv_Unregister(const CHAR * key, INT startstop, DV_DEV_ID dev_id)
{
    STATUS                  status;
    LWSPI_INSTANCE_HANDLE   *inst_ptr;

    /* Unregister device with device manager. */
    status = DVC_Dev_Unregister(dev_id, (VOID**)&inst_ptr);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /* Unregister device with pmi. */
    if(status == NU_SUCCESS)
    {
        /* Place the device in low-power state. */
        LWSPI_TGT_Pwr_Default_State(inst_ptr);

        /* Unregister the device from PMI. */
        status = PMI_Device_Unregister(inst_ptr->pmi_dev);
    }

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* Free the instance handle. */
    NU_Deallocate_Memory((VOID*)inst_ptr);

    return status;
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_Dv_Open
*
* DESCRIPTION
*
*       This function opens the device and creates a session handle.
*
* INPUTS
*
*       inst_ptr                            Instance handle.
*       label_list[]                        Access mode (label) of open.
*       labels_cnt                          Number of labels.
*       session_handle                      Pointer to session handle.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*       SPI_DEV_IN_USE                      Device is already opened
*                                           in requested mode.
*
**************************************************************************/
STATUS LWSPI_Dv_Open(VOID *inst_ptr, DV_DEV_LABEL label_list[],
                        INT label_cnt, VOID **session_handle)
{
    STATUS                  status;
    LWSPI_INSTANCE_HANDLE   *loc_inst_ptr;
    LWSPI_SESSION_HANDLE    *ses_ptr;
    UINT32                  open_mode;
    DV_DEV_LABEL            spi_label = {LWSPI_LABEL};
    VOID*                   pointer;

    /* Initialize local variables. */
    status          = NU_SUCCESS;
    open_mode       = 0;
    loc_inst_ptr    = (LWSPI_INSTANCE_HANDLE*)inst_ptr;

    /* Get open mode requests from labels. */
    if (DVS_Label_List_Contains(label_list, label_cnt, spi_label) == NU_SUCCESS)
    {
        open_mode |= LWSPI_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /* Call the Power device open function. */
    status = PMI_Device_Open (&open_mode, label_list, label_cnt);
#endif

    /* Proceed only if,
     * Either the device is not opened in SPI mode, .i.e. 'device_in_use' not set
     * Or if this is open in power mode.
     */
    if ((loc_inst_ptr->device_in_use != NU_TRUE) || open_mode)
    {
       /* Allocate a new session. */
        status = NU_Allocate_Memory(&System_Memory,
                                    &pointer,
                                    sizeof(LWSPI_SESSION_HANDLE),
                                    NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            ESAL_GE_MEM_Clear (pointer, sizeof(LWSPI_SESSION_HANDLE));
            ses_ptr = (LWSPI_SESSION_HANDLE*)pointer;

            /* Init session. */
            ses_ptr->inst_ptr = inst_ptr;

            /* If open mode request is SPI. */
            if (open_mode & LWSPI_OPEN_MODE)
            {
                /* Set device in use. */
                loc_inst_ptr->device_in_use = NU_TRUE;

                /* Initialize device. */
                LWSPI_TGT_Initialize(loc_inst_ptr);

#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
                /* Enable PR and device interrupts. */
                status = LWSPI_PR_Intr_Enable(ses_ptr);
#endif
            }
        }

        if (status == NU_SUCCESS)
        {
                ses_ptr->open_mode |= open_mode;

                /* Return session handle. */
                *(LWSPI_SESSION_HANDLE**)session_handle = ses_ptr;
        }
    }
    else
    {
        status = SPI_DEV_IN_USE;
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_Dv_Close
*
* DESCRIPTION
*
*       This function stops the SPI controller associated with the
*       specified SPI device.
*
* INPUTS
*
*       sess_handle                         Nucleus SPI driver session
*                                           handle pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
**************************************************************************/
STATUS LWSPI_Dv_Close(VOID *sess_handle)
{
    STATUS                  status;
    LWSPI_SESSION_HANDLE    *ses_ptr;
    LWSPI_INSTANCE_HANDLE   *inst_ptr;
    INT                     int_level;

    /* Initialize local variables. */
    status  = SPI_DEV_DEFAULT_ERROR;
    ses_ptr = (LWSPI_SESSION_HANDLE*)sess_handle;

    /* If a valid session, then close it. */
    if(ses_ptr != NU_NULL)
    {
        /* Get pointer to instance handle. */
        inst_ptr = ses_ptr->inst_ptr;

        if(inst_ptr != NU_NULL)
        {
            if (ses_ptr->open_mode & LWSPI_OPEN_MODE)
            {
                /* Disable interrupts before clearing shared variable. */
                int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

                /* Call shutdown function for SPI. */
                LWSPI_TGT_Shutdown(inst_ptr);

                /* Set device is closed. */
                inst_ptr->device_in_use = NU_FALSE;

                /* Restore interrupts to previous level. */
                NU_Local_Control_Interrupts(int_level);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                status = PMI_Device_Close((inst_ptr->pmi_dev));
#else
                status = NU_SUCCESS;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
            }

            /* Free the session. */
            NU_Deallocate_Memory ((VOID*)ses_ptr);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*       LWSPI_Dv_Ioctl
*
* DESCRIPTION
*
*       This function is responsible for performing miscellaneous control
*       operations.
*
* INPUTS
*
*       sess_ptr                            Session handle of the driver
*       ioctl_cmd                           Ioctl command
*       ioctl_data                          Ioctl data pointer
*       length                              Ioctl length
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
**************************************************************************/
STATUS LWSPI_Dv_Ioctl(VOID *sess_ptr, INT ioctl_cmd, VOID *ioctl_data, INT length)
{
    STATUS                  status;
    DV_IOCTL0_STRUCT        *ioctl0;
    LWSPI_SESSION_HANDLE    *ses_ptr;
    LWSPI_INSTANCE_HANDLE   *inst_ptr;
    DV_DEV_LABEL            spi_label = {LWSPI_LABEL};
    CHAR *                  str_ptr;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE          pmi_dev;
#endif

    /* Initialize local variables. */
    ses_ptr     = (LWSPI_SESSION_HANDLE*)sess_ptr;
    inst_ptr    = ses_ptr->inst_ptr;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    pmi_dev = inst_ptr->pmi_dev;
#endif

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
                status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, ioctl_data, length, inst_ptr,
                                          ses_ptr->open_mode);
#endif
                if (status != NU_SUCCESS)
                {
                    if (DV_COMPARE_LABELS (&(ioctl0->label), &spi_label) &&
                        (ses_ptr->open_mode & LWSPI_OPEN_MODE))
                    {
                        ioctl0->base = LWSPI_SPI_MODE_IOCTL_BASE;
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
        case (LWSPI_SPI_MODE_IOCTL_BASE + LWSPI_IOCTL_GET_BUS_INFO):
        {
            NU_SPI_BUS      *spi_bus;


            spi_bus = (NU_SPI_BUS*) ioctl_data;

            /* Get device name from registry path. */
            str_ptr = &inst_ptr->reg_path[strlen(inst_ptr->reg_path)];
            while (*(--str_ptr) != '/');
            strncpy(spi_bus->name,++str_ptr,NU_SPI_BUS_NAME_LEN);

            /* Ensure name is null terminated. */
            spi_bus->name[NU_SPI_BUS_NAME_LEN - 1] = 0;

            /* Get I/O pointers for this SPI device. */
            spi_bus->io_ptrs.configure      = LWSPI_TGT_Configure;
            spi_bus->io_ptrs.read           = LWSPI_TGT_Read;
            spi_bus->io_ptrs.write          = LWSPI_TGT_Write;
            spi_bus->io_ptrs.write_read     = LWSPI_TGT_Write_Read;
#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
            spi_bus->io_ptrs.isr_read       = LWSPI_TGT_ISR_Read;
            spi_bus->io_ptrs.isr_write      = LWSPI_TGT_ISR_Write;
            spi_bus->io_ptrs.isr_write_read = LWSPI_TGT_ISR_Write_Read;
#endif
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            /* Only initialize check power state function of power management 
            * is enabled in system. 
            */
            spi_bus->io_ptrs.check_power_on = LWSPI_Check_Power_State;
#endif
            /* Save pointer to instance handle. */
            spi_bus->dev_context = inst_ptr;

            /* Set status to success. */
            status = NU_SUCCESS;

            break;
        }
        case (LWSPI_SPI_MODE_IOCTL_BASE + LWSPI_IOCTL_PREP_DMA):
        {
            NU_SPI_DEVICE   *spi_device = (NU_SPI_DEVICE*)ioctl_data;
  
            status = LWSPI_TGT_DMA_Configure(spi_device);

            break;
        }        
        default:
        {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            /* Call the PMI IOCTL function for Power and UII IOCTLs. */
            status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, ioctl_data, length, inst_ptr,
                                      ses_ptr->open_mode);
#else
            status = DV_INVALID_INPUT_PARAMS;
#endif
            break;
        }
    }

    /* Return the completion status of the service. */
    return (status);
}
