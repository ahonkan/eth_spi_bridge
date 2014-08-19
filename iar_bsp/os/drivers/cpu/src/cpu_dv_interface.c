/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       cpu_dv_interface.c
*
*   COMPONENT
*
*       CPU Idle and Wakeup CPU Driver
*
*   DESCRIPTION
*
*       Open, Close and Ioctl driver functions
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include <string.h>

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

/* User implementation of CPU idle and wakeup */
extern VOID   CPU_Tgt_Idle(UINT32 expected_idle_time, UINT32 wakeup_constraint);
extern VOID   CPU_Tgt_Wakeup(VOID);

/* Local function to set new op */
extern VOID   CPU_Tgt_Calculate_Scale(VOID);
extern VOID   CPU_Tgt_Set_OP(UINT8 new_op);
extern UINT32 CPU_Tgt_Switch_Clock_Time(UINT8 from_op, UINT8 to_op);
extern UINT32 CPU_Tgt_Get_OP_Frequency(INT op_index);
extern STATUS CPU_Tgt_Get_OP_Additional(INT op_index, VOID *pm_info);
extern STATUS CPU_Tgt_Get_Spec_Frequency(INT op_index, UINT32 *specific_info,
                                         CPU_DVFS_SPECIFIC *specific_info_ioctl);

#if (CFG_NU_OS_DRVR_CPU_VOLTAGE_SCALING == NU_TRUE)
extern UINT32 CPU_Tgt_Get_OP_Voltage(INT op_index);
#endif

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SELFREFRESH == NU_TRUE)
extern VOID   CPU_Tgt_Enter_SelfRefresh(VOID);
extern VOID   CPU_Tgt_Exit_SelfRefresh(VOID);

#endif

/* Array for DVFS OP */
extern UINT32  CPU_OP_Count;
extern UINT8   CPU_Current_OP;

#if (CFG_NU_OS_DRVR_CPU_VOLTAGE_SCALING == NU_FALSE)
/* Voltage is constant in this driver */
#define CPU_VOLTAGE   1
#endif

static BOOLEAN DVFS_Open = NU_FALSE;
static NU_PROTECT CPU_Protect;

/* Global variable that reflects the devices in the system using DMA. Each set bit of this variable
   represents a unique device with active DMA. If this variable is zero, the system can move
   SDRAM into self refresh mode. */
UINT32  System_DMA_flag = 0;

/*************************************************************************
*
* FUNCTION
*
*       CPU_Dv_Register
*
* DESCRIPTION
*
*       Register a CPU driver
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       Return value of DVC_Dev_Register
*
*************************************************************************/
STATUS CPU_Dv_Register(const CHAR * key, VOID *cpu_instance)
{
    DV_DRV_FUNCTIONS functions;
    DV_DEV_ID        dev_id;
    DV_DEV_LABEL     cpu_class_id[] = {{CPU_IDLE_CLASS_LABEL}, {CPU_DVFS_CLASS_LABEL}, {CPU_DRAM_CTRL_CLASS_LABEL}, {CPU_SELFREFRESH_CLASS_LABEL}};



    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(cpu_instance);

    /* Update the functions */
    functions.drv_open_ptr  = CPU_Dv_Open;
    functions.drv_close_ptr = CPU_Dv_Close;
    functions.drv_read_ptr  = NU_NULL;
    functions.drv_write_ptr = NU_NULL;
    functions.drv_ioctl_ptr = CPU_Dv_Ioctl;

    /* Register with the device manager */
    return (DVC_Dev_Register(NU_NULL, cpu_class_id, DV_GET_LABEL_COUNT(cpu_class_id),
                             &functions, &dev_id));
}


/*************************************************************************
*
* FUNCTION
*
*       CPU_Dv_Unregister
*
* DESCRIPTION
*
*       Unregister a CPU driver
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS CPU_Dv_Unregister(const CHAR *key, INT startstop, DV_DEV_ID dev_id)
{
    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(startstop);
    NU_UNUSED_PARAM(dev_id);

    /* CPU driver can never be unregistered */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       CPU_Dv_Open
*
*   DESCRIPTION
*
*       CPU Open function.  This function initializes the operating point
*       data structures.
*
*   INPUT
*
*       instance_handle         Not currently used by this driver
*       label_list              List of labels registered
*       label_cnt               Number of labels registered
*       session_handle          Not currently used by this driver
*
*   OUTPUT
*
*       NU_SUCCESS
*       NU_UNAVAILABLE
*
*************************************************************************/
STATUS CPU_Dv_Open(VOID *instance_handle, DV_DEV_LABEL label_list[], INT label_cnt, VOID **session_handle)
{
    DV_DEV_LABEL             dvfs_label = {CPU_DVFS_CLASS_LABEL};
    DV_DEV_LABEL             dma_ctrl_label = {CPU_DRAM_CTRL_CLASS_LABEL};
    CPU_SESSION_HANDLE       *ses_ptr;
    NU_MEMORY_POOL           *sys_pool_ptr;
    STATUS                    status = NU_SUCCESS;
    static UINT32             Dev_Dma_Id = 0;

    /* Avoid warnings from unused parameters */
    NU_UNUSED_PARAM(instance_handle);
    NU_UNUSED_PARAM(label_cnt);

    if (DV_COMPARE_LABELS(&label_list[0], &dvfs_label))
    {
        NU_Protect(&CPU_Protect);

        /* Verify initialization hasn't already occurred,
           if so just exit */
        if (DVFS_Open == NU_FALSE)
        {
            /* Calculate the numbers for scaling */
            CPU_Tgt_Calculate_Scale();

            /* Open complete */
            DVFS_Open = NU_TRUE;
        }

        NU_Unprotect();
    }
    else if(DV_COMPARE_LABELS(&label_list[0], &dma_ctrl_label))
    {
        NU_Protect(&CPU_Protect);

        /* Allocate memory for the CPU_SESSION_HANDLE structure */
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        if (status == NU_SUCCESS)
        {
            status = NU_Allocate_Memory (sys_pool_ptr, (VOID *)&ses_ptr, sizeof (CPU_SESSION_HANDLE), NU_NO_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {
            /* Zero out allocated space */
            (VOID)memset (ses_ptr, 0, sizeof (CPU_SESSION_HANDLE));

            /* Set open mode */
            ses_ptr->open_modes |= CPU_DRAM_CTRL_OPEN_MODE;

            /* Assign a DMA ID for this session */
            ses_ptr->dma_id = (1 << Dev_Dma_Id);
            Dev_Dma_Id++;

             /* Set the return address of the session handle */
            *session_handle = (VOID*)ses_ptr;

        }

        NU_Unprotect();
    }


    return(NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Dv_Close
*
*   DESCRIPTION
*
*       Device close function
*
*   INPUT
*
*       session_handle          Not currently used by this driver
*
*   OUTPUT
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS CPU_Dv_Close(VOID *session_handle)
{
    CPU_SESSION_HANDLE  *session_ptr;

    /* If a valid session, then close it */
    if(session_handle != NU_NULL)
    {
        /* Initialize local variables */
        session_ptr = (CPU_SESSION_HANDLE*)session_handle;

        if((session_ptr->open_modes & CPU_DRAM_CTRL_OPEN_MODE) == CPU_DRAM_CTRL_OPEN_MODE)
        {
            /*Clear the system DMA flag for this device*/
            System_DMA_flag &= ~(session_ptr->dma_id);

            /* Update open mode flags. */
            session_ptr->open_modes &= ~(session_ptr->open_modes);

            /* Free the session */
            NU_Deallocate_Memory (session_ptr);
        }
    }


    /* CPU driver can never be closed */
    return(NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       CPU_Dv_Ioctl
*
*   DESCRIPTION
*
*       Process all CPU driver ioctl's.  This driver supports dvfs and idle.
*
*   INPUT
*
*       session_handle_ptr      Not currently used by this driver
*       ioctl_num               Command to process
*       ioctl_data_ptr          Data for parameter and return values
*       ioctl_data_len          Length of the data passed in
*
*   OUTPUT
*
*       NU_SUCCESS              This indicates successful completion
*       DV_INVALID_INPUT_PARAMS Data pointer or length is invalid
*
*************************************************************************/
STATUS CPU_Dv_Ioctl(VOID *session_handle_ptr, INT ioctl_num,
                      VOID *ioctl_data_ptr, INT ioctl_data_len)
{
    STATUS               status = DV_INVALID_INPUT_PARAMS;
    DV_IOCTL0_STRUCT    *ioctl0;
    DV_DEV_LABEL         idle_label = {CPU_IDLE_CLASS_LABEL};
    DV_DEV_LABEL         dvfs_label = {CPU_DVFS_CLASS_LABEL};
    DV_DEV_LABEL         dram_label = {CPU_DRAM_CTRL_CLASS_LABEL};
    DV_DEV_LABEL         selfrefresh_label = {CPU_SELFREFRESH_CLASS_LABEL};
    UINT32              *cpu_fn_ptr;
    UINT8               *op_count;
    UINT8               *current_op;
    UINT8               *new_op;
    UINT8                op_index;
    CPU_DVFS_GET_OP     *op;
    CPU_DVFS_ADDITIONAL *additional_info_ioctl;
    CPU_DVFS_FROM_TO    *from_to;
    CPU_DVFS_SPECIFIC   *specific_info_ioctl;
    UINT32              *specific_info;
    BOOLEAN             *dram_ctrl_status;
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SELFREFRESH == NU_TRUE)
    CPU_SELF_REFRESH_CB *selfrefresh_info;
#endif

    CPU_SESSION_HANDLE  *sess_handle = (CPU_SESSION_HANDLE *)session_handle_ptr;

    if (ioctl_data_ptr != NU_NULL)
    {
        switch (ioctl_num)
        {
            case DV_IOCTL0:

                /* Verify data length */
                if (ioctl_data_len == sizeof(DV_IOCTL0_STRUCT))
                {
                    /* Get the ioctl0 structure from the data passed in */
                    ioctl0 = (DV_IOCTL0_STRUCT *) ioctl_data_ptr;

                    if (DV_COMPARE_LABELS(&ioctl0 -> label, &idle_label))
                    {
                        ioctl0 -> base = CPU_IDLE_IOCTL_BASE;
                        status = NU_SUCCESS;
                    }
                    else if (DV_COMPARE_LABELS(&ioctl0 -> label, &dvfs_label))
                    {
                        if (DVFS_Open == NU_TRUE)
                        {
                            ioctl0 -> base = CPU_DVFS_IOCTL_BASE;
                            status = NU_SUCCESS;
                        }
                        else
                        {
                            status = DV_INVALID_INPUT_PARAMS;
                        }
                    }
                    else if (DV_COMPARE_LABELS(&ioctl0 -> label, &dram_label))
                    {
                        ioctl0 -> base = CPU_DRAM_IOCTL_BASE;
                        status = NU_SUCCESS;
                    }

                    else if (DV_COMPARE_LABELS(&ioctl0 -> label, &selfrefresh_label))
                    {
                        ioctl0 -> base = CPU_SELFREFRESH_IOCTL_BASE;
                        status = NU_SUCCESS;
                    }
                }
                break;

            case (CPU_IDLE_IOCTL_BASE + CPU_IDLE_IOCTL_GET_IDLE):

                /* Verify data length */
                if (ioctl_data_len == sizeof(UINT32))
                {
                    /* Get the function pointer for the CPU idle */
                    cpu_fn_ptr = (UINT32 *)ioctl_data_ptr;
                    *cpu_fn_ptr = (UINT32)CPU_Tgt_Idle;

                    status = NU_SUCCESS;
                }
                break;

            case (CPU_IDLE_IOCTL_BASE + CPU_IDLE_IOCTL_GET_WAKEUP):

                /* Verify data length */
                if (ioctl_data_len == sizeof(UINT32))
                {
                    /* Get the function pointer for the CPU wakeup */
                    cpu_fn_ptr = (UINT32 *)ioctl_data_ptr;
                    *cpu_fn_ptr = (UINT32)CPU_Tgt_Wakeup;

                    status = NU_SUCCESS;
                }
                break;

            case (CPU_DVFS_IOCTL_BASE + CPU_DVFS_IOCTL_GET_OP_COUNT):

                /* Verify data length */
                if (ioctl_data_len == sizeof(UINT8))
                {
                    op_count = (UINT8 *) ioctl_data_ptr;
                    *op_count = CPU_OP_Count;

                    status = NU_SUCCESS;
                }

                break;

            case (CPU_DVFS_IOCTL_BASE + CPU_DVFS_IOCTL_GET_OP):

                /* Verify data length */
                if (ioctl_data_len == sizeof(CPU_DVFS_GET_OP))
                {
                    /* Setup return OP structure */
                    op = (CPU_DVFS_GET_OP *) ioctl_data_ptr;

                    /* Read the op index */
                    op_index = CPU_GET_OP_INDEX(op -> pm_op_index);

                    /* Set voltage and frequency for the passed in OP */
#if (CFG_NU_OS_DRVR_CPU_VOLTAGE_SCALING == NU_TRUE)
                    op -> pm_voltage = CPU_Tgt_Get_OP_Voltage(op_index);
#else
                    op -> pm_voltage = CPU_VOLTAGE;
#endif
                    op -> pm_frequency = CPU_Tgt_Get_OP_Frequency(op_index);

                    status = NU_SUCCESS;
                }

                break;

            case (CPU_DVFS_IOCTL_BASE + CPU_DVFS_IOCTL_SET_OP):

                /* Verify data length */
                if (ioctl_data_len == sizeof(UINT8))
                {
                    new_op = (UINT8 *) ioctl_data_ptr;
                    CPU_Tgt_Set_OP(*new_op);
                    status = NU_SUCCESS;
                }

                break;

            case (CPU_DVFS_IOCTL_BASE + CPU_DVFS_IOCTL_FROM_TO):

                /* Verify data length */
                if (ioctl_data_len == sizeof(CPU_DVFS_FROM_TO))
                {
                    from_to = (CPU_DVFS_FROM_TO *) ioctl_data_ptr;
                    from_to -> pm_time = CPU_Tgt_Switch_Clock_Time(from_to -> pm_op_from, from_to -> pm_op_to);

                    status = NU_SUCCESS;
                }

                break;

            case (CPU_DVFS_IOCTL_BASE + CPU_DVFS_IOCTL_GET_CURRENT_OP):

                /* Verify data length */
                if (ioctl_data_len == sizeof(UINT8))
                {
                    current_op = (UINT8 *) ioctl_data_ptr;
                    *current_op = CPU_Current_OP;

                    status = NU_SUCCESS;
                }

                break;

            case (CPU_DVFS_IOCTL_BASE + CPU_DVFS_IOCTL_ADDITIONAL):

                /* Verify data length */
                if (ioctl_data_len == sizeof(CPU_DVFS_ADDITIONAL))
                {
                    additional_info_ioctl = (CPU_DVFS_ADDITIONAL *) ioctl_data_ptr;

                    /* Read the op index */
                    op_index = CPU_GET_OP_INDEX(additional_info_ioctl -> pm_op_id);
                    status = CPU_Tgt_Get_OP_Additional(op_index, (additional_info_ioctl->pm_info));
                }

                break;

            case (CPU_DVFS_IOCTL_BASE + CPU_DVFS_IOCTL_SPECIFIC):

                /* Verify data length */
                if (ioctl_data_len == sizeof(CPU_DVFS_SPECIFIC))
                {
                    specific_info_ioctl = (CPU_DVFS_SPECIFIC *) ioctl_data_ptr;

                    if (sizeof(UINT32) == specific_info_ioctl -> pm_size)
                    {
                        /* Read the OP */
                        op_index = CPU_GET_OP_INDEX(specific_info_ioctl -> pm_op_id);
                        specific_info = specific_info_ioctl -> pm_info;
                        status = CPU_Tgt_Get_Spec_Frequency(op_index, specific_info, specific_info_ioctl);
                    }
                }
                break;

            case (CPU_DRAM_IOCTL_BASE + CPU_DRAM_IOCTL_DISABLE_SELF_REFRESH):

                /* Set the DMA flag bit for this device */
                System_DMA_flag |= sess_handle->dma_id;
                status = NU_SUCCESS;
                break;

            case (CPU_DRAM_IOCTL_BASE + CPU_DRAM_IOCTL_ENABLE_SELF_REFRESH):

                /* Clear the DMA flag bit for this device */
                System_DMA_flag &= ~(sess_handle->dma_id);
                status = NU_SUCCESS;
                break;

            case (CPU_DRAM_IOCTL_BASE + CPU_DRAM_IOCTL_GET_SELF_REFRESH_STATUS):

                /* Verify data length */
                if (ioctl_data_len == sizeof(BOOLEAN))
                {
                    dram_ctrl_status = (BOOLEAN *) ioctl_data_ptr;

                    if(System_DMA_flag)
                        *dram_ctrl_status = NU_TRUE;
                    else
                        *dram_ctrl_status = NU_FALSE;

                    status = NU_SUCCESS;
                }

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SELFREFRESH == NU_TRUE)

                case (CPU_SELFREFRESH_IOCTL_BASE + CPU_SELFREFRESH_IOCTL_INIT):

                /* Verify data length */
                if (ioctl_data_len == sizeof(CPU_SELF_REFRESH_CB))
                {
                    selfrefresh_info = (CPU_SELF_REFRESH_CB *) ioctl_data_ptr;

                    selfrefresh_info->selfrefresh_enter_func = CPU_Tgt_Enter_SelfRefresh;
                    selfrefresh_info->selfrefresh_exit_func = CPU_Tgt_Exit_SelfRefresh;

                    status = NU_SUCCESS;
                }
#endif


            default:
                break;
        }
    }

    return (status);
}

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
