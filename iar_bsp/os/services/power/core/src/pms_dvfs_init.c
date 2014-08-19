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
*       pms_dvfs_init.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for DVFS initialization
*
*   DATA STRUCTURES
*
*       PM_DVFS_Initialization_Status
*
*   FUNCTIONS
*
*       PMS_DVFS_Status_Check
*       PMS_DVFS_Pre_Initialize
*       PMS_DVFS_Post_Initialize
*       PMS_DVFS_Device_Register_Callback
*       PMS_DVFS_Allocate_Arrays
*       PMS_DVFS_Place_Frequency
*       PMS_DVFS_Place_OP
*       PMS_DVFS_Place_Voltage
*       PMS_DVFS_Place_Startup_OP
*
*   DEPENDENCIES
*
*       power_core.h
*       dvfs.h
*       initialization.h
*       cpu_dvfs.h
*       reg_api.h
*       runlevel_init.h
*       string.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/power_core.h"
#include "os/services/power/core/inc/dvfs.h"
#include "os/services/power/core/inc/initialization.h"
#include "services/cpu_dvfs.h"
#include "services/reg_api.h"
#include "services/runlevel_init.h"
#include "services/nu_trace_os_mark.h"
#include <string.h>

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

extern DV_DEV_HANDLE  PM_DVFS_CPU_Handle;
extern UINT32         PM_DVFS_CPU_Base;
extern UINT8          PM_DVFS_OP_Count;
extern UINT8          PM_DVFS_Current_OP;
extern UINT8          PM_DVFS_Minimum_OP;
extern UINT8          PM_DVFS_Voltage_Count;
extern UINT8          PM_DVFS_Frequency_Count;
extern PM_OP        **PM_DVFS_OP_Array;
extern PM_VOLTAGE   **PM_DVFS_Voltage_Array;
extern PM_FREQUENCY **PM_DVFS_Frequency_Array;
extern DV_DEV_ID      PM_DVFS_Device_ID;
extern NU_MEMORY_POOL System_Memory;

/* Local copy of initialization status */
static STATUS PM_DVFS_Initialization_Status = PM_DVFS_DRIVER_NOT_FOUND;

static STATUS PMS_DVFS_Device_Register_Callback(DV_DEV_ID device, VOID *context);
static STATUS PMS_DVFS_Allocate_Arrays(NU_MEMORY_POOL* mem_pool_ptr, UINT32 entries);
static STATUS PMS_DVFS_Place_Voltage(NU_MEMORY_POOL* mem_pool_ptr, UINT16 voltage, UINT8* voltage_id_ptr);
static STATUS PMS_DVFS_Place_Frequency(NU_MEMORY_POOL* mem_pool_ptr, UINT32 frequency, UINT8 voltage_id, UINT8* frequency_id_ptr);
static STATUS PMS_DVFS_Place_OP(NU_MEMORY_POOL* mem_pool_ptr, UINT8 driver_op_id, UINT8 voltage_id, UINT8 frequency_id);
static STATUS PMS_DVFS_Place_Startup_OP(NU_MEMORY_POOL* mem_pool_ptr, UINT8 op_count);

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Status_Check
*
*   DESCRIPTION
*
*       This function first checks the status of the initialization.
*       Then if that is complete it returns the state of the DVFS
*       initialization which is set in the DVFS post schedule
*       initialization task.
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       NU_SUCCESS                  DVFS is fully initialized
*       PM_NOT_INITIALIZED          PMS is not initialized
*       PM_DVFS_DRIVER_NOT_FOUND    PMS is initialized but no CPU driver
*                                   has been initialized
*
*************************************************************************/
STATUS PMS_DVFS_Status_Check(VOID)
{
    STATUS pm_status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First check the initialization status */
    pm_status = PMS_Initialization_Status_Check();

    /* If initialization is complete return
       the local DVFS initialization status */
    if (pm_status == NU_SUCCESS)
    {
        pm_status = PM_DVFS_Initialization_Status;
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (pm_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Pre_Initialize
*
*   DESCRIPTION
*
*       This function initializes the DVFS component of the PMS.
*
*   INPUT
*
*       mem_pool_ptr        Pointer to memory pool for allocation
*
*   OUTPUT
*
*       NU_SUCCESS          Successful initialization
*       PM_UNEXPECTED_ERROR Unexpected error has occurred
*
*************************************************************************/
STATUS PMS_DVFS_Pre_Initialize(NU_MEMORY_POOL *mem_pool_ptr)
{
    STATUS pm_status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Create the set OP interface */
    pm_status = PMS_DVFS_Set_OP_Initialize(mem_pool_ptr);

    /* Return to user mode */
    NU_USER_MODE();

    return (pm_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Post_Initialize
*
*   DESCRIPTION
*
*       This function adds the notification for any CPU_DVFS_CLASS_LABEL
*       device registration event.
*
*   INPUT
*
*       mem_pool_ptr        Pointer to memory pool for allocation
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID PMS_DVFS_Post_Initialize(NU_MEMORY_POOL* mem_pool_ptr)
{
    DV_DEV_LABEL       dvfs_class_id = {CPU_DVFS_CLASS_LABEL};
    DV_LISTENER_HANDLE listener_id;

    /* Call DM API to add callback for device register event. We are not
     * interested in un-registration at the moment. */
    (VOID)DVC_Reg_Change_Notify(&dvfs_class_id,
                                DV_GET_LABEL_COUNT(dvfs_class_id),
                                &PMS_DVFS_Device_Register_Callback,
                                NU_NULL,
                                NU_NULL,
                                &listener_id);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Device_Register_Callback
*
*   DESCRIPTION
*
*       This function initializes the DVFS component of the PMS.  This
*       function is called when any device with CPU_DVFS_CLASS_LABEL is
*       registered with device manager. This function opens the CPU driver
*       and updates all related DVFS structures.
*
*   INPUT
*
*       device                              Newly registered Device ID.
*       context                             Any context from caller.
*
*   OUTPUT
*
*       NU_SUCCESS                          Function returns success.
*       (Device Manager error values)
*
*************************************************************************/
static STATUS PMS_DVFS_Device_Register_Callback(DV_DEV_ID device, VOID *context)
{
    STATUS           status;
    UINT8            op_count = 0;
    UINT8            op_index;
    UINT8            voltage_id = 0;
    UINT8            frequency_id = 0;
    UINT8            init_op = 0;
    DV_DEV_LABEL     dvfs_class_id = {CPU_DVFS_CLASS_LABEL};
    NU_MEMORY_POOL  *mem_pool_ptr = &System_Memory;
    CPU_DVFS_GET_OP  ioctl_get_op;
    DV_IOCTL0_STRUCT ioctl0;

    /* Open the CPU driver for DVFS usage, save the handle for later usage */
    status = DVC_Dev_ID_Open(device, &dvfs_class_id, DV_GET_LABEL_COUNT(dvfs_class_id), &PM_DVFS_CPU_Handle);
    if (status == NU_SUCCESS)
    {
        /* Save the DVFS ID */
        PM_DVFS_Device_ID = device;

        /* Get ioctl base */
        ioctl0.label = dvfs_class_id;
        status = DVC_Dev_Ioctl(PM_DVFS_CPU_Handle, DV_IOCTL0, &ioctl0, sizeof(ioctl0));
        if (status == NU_SUCCESS)
        {
            /* Update the saved ioctl base */
            PM_DVFS_CPU_Base = ioctl0.base;

            /* Get the number of OPs */
            status = DVC_Dev_Ioctl(PM_DVFS_CPU_Handle, (PM_DVFS_CPU_Base + CPU_DVFS_IOCTL_GET_OP_COUNT), &op_count, sizeof(op_count));

            if (status == NU_SUCCESS)
            {
                /* Allocate all the arrays, allocate 1 extra to use as the
                   startup index */
                status = PMS_DVFS_Allocate_Arrays(mem_pool_ptr, op_count + 1);
            }

            /* Get each OP and populate the arrays */
            for(op_index = 0; ((op_index < op_count) && (status == NU_SUCCESS)); op_index++)
            {
                /* fill out the structure and call appropriate ioctl
                   to get the next operating point */
                ioctl_get_op.pm_op_index = op_index;
                status = DVC_Dev_Ioctl(PM_DVFS_CPU_Handle, (PM_DVFS_CPU_Base + CPU_DVFS_IOCTL_GET_OP), &ioctl_get_op, sizeof(ioctl_get_op));
                if (status == NU_SUCCESS)
                {
                    /* Place the voltage information in the voltage array */
                    status = PMS_DVFS_Place_Voltage(mem_pool_ptr, ioctl_get_op.pm_voltage, &voltage_id);
                    if (status == NU_SUCCESS)
                    {
                        /* Place the frequency information */
                        status = PMS_DVFS_Place_Frequency(mem_pool_ptr, ioctl_get_op.pm_frequency, voltage_id, &frequency_id);
                        if (status == NU_SUCCESS)
                        {
                            /* Now frequency and voltage is setup place the operating points */
                            status = PMS_DVFS_Place_OP(mem_pool_ptr, ioctl_get_op.pm_op_index, voltage_id, frequency_id);
                            
                            /* Trace log */ 
                            T_OP_INFO(op_index, voltage_id, ioctl_get_op.pm_voltage, frequency_id, ioctl_get_op.pm_frequency);
                            
                        } /* End place OP */
                    } /* End place frequency */
                } /* End place voltage */
            } /* OP Setup loop */

            /* Place the special case startup OP, everything about this will be last in the list regardless if
               a frequency or voltage already exists or if an OP happens to match it. */
            if (status == NU_SUCCESS)
            {
                status = PMS_DVFS_Place_Startup_OP(mem_pool_ptr, op_count);
            }

            if (status == NU_SUCCESS)
            {
                /* Set the current minimum to the lowest available */
                PM_DVFS_Minimum_OP = 0;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

                /* Set the hibernate exit (resume) OP to the highest in the system */
                status = NU_PM_Set_Hibernate_Exit_OP(PM_DVFS_OP_Count - 1);

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

            }

            if (status == NU_SUCCESS)
            {
                /* Get the current OP */
                status = DVC_Dev_Ioctl(PM_DVFS_CPU_Handle, (PM_DVFS_CPU_Base + CPU_DVFS_IOCTL_GET_CURRENT_OP), &PM_DVFS_Current_OP, sizeof(PM_DVFS_Current_OP));
                if (status == NU_SUCCESS)
                {
                    /* Determine the appropriate OP to use after DVFS
                       initialization completion */
                    if (CFG_NU_OS_SVCS_PWR_CORE_INITIAL_OP > (PM_DVFS_OP_Count - 1))
                    {
                        /* Set to the highest one available */
                        init_op = (PM_DVFS_OP_Count - 1);
                    }
                    else
                    {
                        /* Use the one defined by configuration options */
                        init_op = CFG_NU_OS_SVCS_PWR_CORE_INITIAL_OP;
                    }

                    /* Check to see if an OP switch is required */
                    if (init_op != PM_DVFS_Current_OP)
                    {
                        /* Set to the desired OP, calling the set OP API
                           at this point is not possible, call the task
                           directly to enforce all DVFS notification to occur.
                           DVFS registration can occur before DVFS initialization
                           is complete. */
                        PMS_DVFS_Set_OP_Task_Entry((UNSIGNED)init_op, NU_NULL);
                    }
                    else
                    {
                        T_OP_TRANS(init_op, status);
                    }

                    /* Indicate that a CPU driver has been registered and all
                       data structures are initialized */
                    PM_DVFS_Initialization_Status = NU_SUCCESS;
                }
            }
        }
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Allocate_Arrays
*
*   DESCRIPTION
*
*       This function creates arrays for operating points, frequencies,
*       and voltages.  These will all be set to the number of OPs in
*       the system.
*
*   INPUT
*
*       mem_pool_ptr        Pointer to memory pool for allocation
*       entries             Number of entries for each array
*
*   OUTPUT
*
*       Return value for NU_Allocate_Memory
*
*************************************************************************/
static STATUS PMS_DVFS_Allocate_Arrays(NU_MEMORY_POOL* mem_pool_ptr, UINT32 entries)
{
    STATUS  status;
    VOID   *op_ptr;
    VOID   *freq_ptr;
    VOID   *volt_ptr;
    UINT32  size;

    /* Size of the arrays will the size of a pointer multiplied by the number of op entries */
    size = sizeof(VOID *) * entries;

    /* Allocate the array of OPs */
    status = NU_Allocate_Memory(mem_pool_ptr, &op_ptr, size, NU_NO_SUSPEND);
    if (status == NU_SUCCESS)
    {
        /* Allocate the array of OPs */
        status = NU_Allocate_Memory(mem_pool_ptr, &freq_ptr, size, NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Allocate the array of OPs */
            status = NU_Allocate_Memory(mem_pool_ptr, &volt_ptr, size, NU_NO_SUSPEND);
            if (status != NU_SUCCESS)
            {
                /* Failure occurred getting more memory for voltage deallocate
                   the op and frequency memory */
                (VOID)NU_Deallocate_Memory(op_ptr);
                (VOID)NU_Deallocate_Memory(freq_ptr);
            }
        }
        else
        {
            /* Failure occurred getting more memory for frequency deallocate the op memory */
            (VOID)NU_Deallocate_Memory(op_ptr);
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Clear the allocated memory */
        memset(op_ptr, 0, size);
        memset(freq_ptr, 0, size);
        memset(volt_ptr, 0, size);

        /* Set the actual array pointers */
        PM_DVFS_OP_Array = op_ptr;
        PM_DVFS_Voltage_Array = volt_ptr;
        PM_DVFS_Frequency_Array = freq_ptr;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Place_Voltage
*
*   DESCRIPTION
*
*       This function sets up a voltage control block and places it
*       in the voltage array.
*
*   INPUT
*
*       mem_pool_ptr        Pointer to memory pool for allocation
*       voltage             Actual voltage value
*       voltage_id_ptr      Returns the new index for the voltage
*
*   OUTPUT
*
*       Return value for NU_Allocate_Memory
*
*************************************************************************/
static STATUS PMS_DVFS_Place_Voltage(NU_MEMORY_POOL* mem_pool_ptr, UINT16 voltage,
                                     UINT8* voltage_id_ptr)
{
    UINT8       voltage_index;
    BOOLEAN     voltage_flag = NU_FALSE;
    STATUS      status = NU_SUCCESS;
    PM_VOLTAGE *voltage_ptr;

    /* Search current voltage list */
    for(voltage_index = 0; ((voltage_index < PM_DVFS_Voltage_Count) &&
                            (PM_DVFS_Voltage_Array[voltage_index] != 0)); voltage_index++)
    {
        /* if voltage exists mark as found and return index */
        if (PM_DVFS_Voltage_Array[voltage_index] -> pm_voltage == voltage)
        {
            *voltage_id_ptr = voltage_index;
            voltage_flag = NU_TRUE;
        }
    }

    /* Check to see if an existing voltage was found */
    if (voltage_flag == NU_FALSE)
    {
        /* allocate a voltage control block */
        status = NU_Allocate_Memory(mem_pool_ptr, (VOID *)&voltage_ptr, sizeof(PM_VOLTAGE), NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* increment number of unique voltages */
            PM_DVFS_Voltage_Count++;

            /* Write voltage value to control block */
            voltage_ptr -> pm_voltage = voltage;

            /* Place new voltage block pointer in array */
            PM_DVFS_Voltage_Array[voltage_index] = voltage_ptr;

            /* Return new index */
            *voltage_id_ptr = voltage_index;
        }
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Place_Frequency
*
*   DESCRIPTION
*
*       This function sets up a frequency control block and places it
*       in the frequency array.  It also scans for previous frequencies
*       to update with a lower voltage ID if found.
*
*   INPUT
*
*       mem_pool_ptr        Pointer to memory pool for allocation
*       frequency           Actual frequency value
*       voltage_id          Voltage ID associated with this frequency
*       frequency_id_ptr    Returns the new index for the frequency
*
*   OUTPUT
*
*       Return value for NU_Allocate_Memory
*
*************************************************************************/
static STATUS PMS_DVFS_Place_Frequency(NU_MEMORY_POOL* mem_pool_ptr, UINT32 frequency,
                                       UINT8 voltage_id, UINT8* frequency_id_ptr)
{
    UINT8         frequency_index;
    BOOLEAN       frequency_flag = NU_FALSE;
    STATUS        status = NU_SUCCESS;
    PM_FREQUENCY *frequency_ptr;
    UINT32        old_voltage;
    UINT32        new_voltage;

    /* Search current frequency list */
    for(frequency_index = 0; ((frequency_index < PM_DVFS_Frequency_Count) &&
                            (PM_DVFS_Frequency_Array[frequency_index] != 0)); frequency_index++)
    {
        /* if frequency exists mark as found and return index */
        if (PM_DVFS_Frequency_Array[frequency_index] -> pm_frequency == frequency)
        {
            *frequency_id_ptr = frequency_index;
            frequency_flag = NU_TRUE;

            /* check voltage */
            old_voltage = PM_DVFS_Voltage_Array[PM_DVFS_Frequency_Array[frequency_index] -> pm_volt_id] -> pm_voltage;
            new_voltage = PM_DVFS_Voltage_Array[voltage_id] -> pm_voltage;
            if (old_voltage > new_voltage)
            {
                /* if voltage is less update to new id */
                PM_DVFS_Frequency_Array[frequency_index] -> pm_volt_id = voltage_id;
            }
        }
    }

    /* Check to see if an existing frequency was found */
    if (frequency_flag == NU_FALSE)
    {
        /* allocate a frequency control block */
        status = NU_Allocate_Memory(mem_pool_ptr, (VOID *)&frequency_ptr, sizeof(PM_FREQUENCY), NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* increment number of unique frequencies */
            PM_DVFS_Frequency_Count++;

            /* Write frequency value to control block */
            frequency_ptr -> pm_frequency = frequency;
            frequency_ptr -> pm_volt_id = voltage_id;

            /* Place new frequency block pointer in array */
            PM_DVFS_Frequency_Array[frequency_index] = frequency_ptr;

            /* Return new index */
            *frequency_id_ptr = frequency_index;
        }
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Place_OP
*
*   DESCRIPTION
*
*       This function sets up an operating point control block and places
*       it in the operating point array.
*
*   INPUT
*
*       mem_pool_ptr        Pointer to memory pool for allocation
*       driver_op_id        Operating point used in the CPU driver
*       voltage_id          Voltage ID associated with this OP
*       frequency_id        Frequency ID associated with this OP
*
*   OUTPUT
*
*       Return value for NU_Allocate_Memory
*
*************************************************************************/
static STATUS PMS_DVFS_Place_OP(NU_MEMORY_POOL* mem_pool_ptr, UINT8 driver_op_id,
                                UINT8 voltage_id, UINT8 frequency_id)
{
    STATUS  status;
    PM_OP  *op_ptr;

    /* Allocate operating point control block */
    status = NU_Allocate_Memory(mem_pool_ptr, (VOID *)&op_ptr, sizeof(PM_OP), NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* update with driver op id */
        op_ptr -> pm_op_id = driver_op_id;

        /* update with voltage id */
        op_ptr -> pm_volt_id = voltage_id;

        /* update with frequency id */
        op_ptr -> pm_freq_id = frequency_id;

        /* increment the number of OPs */
        PM_DVFS_OP_Count++;

        /* Place new op block pointer in array */
        PM_DVFS_OP_Array[(PM_DVFS_OP_Count - 1)] = op_ptr;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Place_Startup_OP
*
*   DESCRIPTION
*
*       This function sets up an operating point control block for
*       the startup OP in the last element of the arrays.
*
*   INPUT
*
*       mem_pool_ptr        Pointer to memory pool for allocation
*       op_count            Number of OPs will also serve as the index
*
*   OUTPUT
*
*       Return value for called functions DVC_Dev_Ioctl and
*       NU_Allocate_Memory
*
*************************************************************************/
static STATUS PMS_DVFS_Place_Startup_OP(NU_MEMORY_POOL* mem_pool_ptr, UINT8 op_count)
{
    STATUS           status = NU_SUCCESS;
    CPU_DVFS_GET_OP  ioctl_get_op;
    PM_VOLTAGE      *voltage_ptr;
    PM_FREQUENCY    *frequency_ptr;
    PM_OP           *op_ptr;

    /* Fill out the structure  with the startup index and call appropriate ioctl
       to get the operating point */
    ioctl_get_op.pm_op_index = PM_STARTUP_OP;
    status = DVC_Dev_Ioctl(PM_DVFS_CPU_Handle, (PM_DVFS_CPU_Base + CPU_DVFS_IOCTL_GET_OP), &ioctl_get_op, sizeof(ioctl_get_op));

    /* Setup the voltage */
    if (status == NU_SUCCESS)
    {
        /* allocate a voltage control block */
        status = NU_Allocate_Memory(mem_pool_ptr, (VOID *)&voltage_ptr, sizeof(PM_VOLTAGE), NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Write voltage value to control block */
            voltage_ptr -> pm_voltage = ioctl_get_op.pm_voltage;

            /* Place new voltage block pointer in array */
            PM_DVFS_Voltage_Array[PM_DVFS_Voltage_Count] = voltage_ptr;
        }
    }

    /* Setup the frequency */
    if (status == NU_SUCCESS)
    {
        /* allocate a frequency control block */
        status = NU_Allocate_Memory(mem_pool_ptr, (VOID *)&frequency_ptr, sizeof(PM_FREQUENCY), NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Write frequency value to control block */
            frequency_ptr -> pm_frequency = ioctl_get_op.pm_frequency;
            frequency_ptr -> pm_volt_id = PM_DVFS_Voltage_Count;

            /* Place new frequency block pointer in array */
            PM_DVFS_Frequency_Array[PM_DVFS_Frequency_Count] = frequency_ptr;
        }
    }

    /* Create the OP */
    if (status == NU_SUCCESS)
    {
        /* Allocate operating point control block */
        status = NU_Allocate_Memory(mem_pool_ptr, (VOID *)&op_ptr, sizeof(PM_OP), NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* update with driver op id */
            op_ptr -> pm_op_id = PM_STARTUP_OP;

            /* update with voltage id */
            op_ptr -> pm_volt_id = PM_DVFS_Voltage_Count;

            /* update with frequency id */
            op_ptr -> pm_freq_id = PM_DVFS_Frequency_Count;

            /* Place new op block pointer in array */
            PM_DVFS_OP_Array[op_count] = op_ptr;
        }
    }

    return (status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


