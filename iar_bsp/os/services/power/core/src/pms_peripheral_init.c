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
*        pms_peripheral_init.c
*
*   COMPONENT
*
*        Peripheral
*
*   DESCRIPTION
*
*        This file initializes the peripheral state service for devices.
*
*   DATA STRUCTURES
*
*        None
*
*   FUNCTIONS
*
*        PMS_Peripheral_Initialize
*        PMS_Peripheral_Device_Register_Callback
*        PMS_Peripheral_Device_Unregister_Callback
*
*   DEPENDENCIES
*
*       peripheral.h
*       power_core.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/power_core.h"
#include "os/services/power/core/inc/peripheral.h"

#if ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_PERIPHERAL == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE))

extern NU_MEMORY_POOL System_Memory;

PM_DEVICE_INFO *PM_Dev_Info_Array[PM_MAX_DEV_CNT];

static STATUS   PMS_Peripheral_New_Device_Index(INT *index_ptr);
static STATUS   PMS_Peripheral_Device_Register_Callback(DV_DEV_ID device, VOID *context);
static STATUS   PMS_Peripheral_Device_Unregister_Callback(DV_DEV_ID device, VOID *context);

/*************************************************************************
*
*   FUNCTION
*
*       PMS_Peripheral_Initialize
*
*   DESCRIPTION
*
*       This function adds the notification functions for any POWER_CLASS_LABEL
*       device register/unregister event.
*
*   INPUT
*
*       mem_pool                                          Memory Pool
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID PMS_Peripheral_Initialize(NU_MEMORY_POOL* mem_pool)
{
    DV_DEV_LABEL       power_class_id = {POWER_CLASS_LABEL};
    DV_LISTENER_HANDLE listener_id;

    /* Call DM API to add callback for device register/unregister event. */
    (VOID)DVC_Reg_Change_Notify(&power_class_id,
                                DV_GET_LABEL_COUNT(power_class_id),
                                &PMS_Peripheral_Device_Register_Callback,
                                &PMS_Peripheral_Device_Unregister_Callback,
                                NU_NULL,
                                &listener_id);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_Peripheral_Device_Register_Callback
*
*   DESCRIPTION
*
*       This function initializes the Power Device Structures for PMS.
*       This is actually a callback function which is invoked when a
*       device with POWER_CLASS_LABEL label is registered with DM.
*
*   INPUT
*
*       device                              Newly registered Device ID.
*       context                             Any context from caller.
*
*   OUTPUT
*
*       NU_SUCCESS
*       NU_INVALID_POINTER
*       NU_UNAVAILABLE
*
*************************************************************************/
static STATUS PMS_Peripheral_Device_Register_Callback(DV_DEV_ID device, VOID *context)
{
    STATUS          status;
    INT             periph_index = 0;
    STATUS          pm_status;
    NU_MEMORY_POOL *mem_pool_ptr = &System_Memory;
    PM_DEVICE_INFO *new_dev_info;

    /* Allocate memory for the device structure */
    status = NU_Allocate_Memory(mem_pool_ptr, (VOID*)&new_dev_info,
                                sizeof(PM_DEVICE_INFO), NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Clear the memory we just allocated */
        (VOID)memset((VOID*)new_dev_info, 0, sizeof(PM_DEVICE_INFO));

        /* Zero maybe a valid value for device handle, so change it to
            something else */
        new_dev_info->dev_handle = PMS_EMPTY_DEV_HANDLE;

        /* Save the device ID */
        new_dev_info->dev_id = device;

        /* Set status to unexpected error. */
        status = DV_UNEXPECTED_ERROR;

        /* Find a new index to place the device in the array */
        pm_status = PMS_Peripheral_New_Device_Index(&periph_index);

        /* Verify a proper index has been found */
        if (pm_status == NU_SUCCESS)
        {
            /* Place the structure in an array (indexed by dev_id) */
            PM_Dev_Info_Array[periph_index] = new_dev_info;

            /* pm_status is success, so same goes for status. */
            status = NU_SUCCESS;
        }
        else
        {
            /* If a valid location wasn't found this is unexpected
               raise an error */
            NU_PM_Throw_Error(pm_status, NU_NULL, 0);
        }
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_Peripheral_Device_Unregister_Callback
*
*   DESCRIPTION
*
*       This function is a callback function and comes into action when a
*       device with POWER_CLASS_LABEL label is unregistered from DM.
*
*   INPUT
*
*       device                              Newly unregistered Device ID.
*       context                             Any context from caller.
*
*   OUTPUT
*
*       NU_SUCCESS
*       PM_INVALID_DEVICE_ID
*       PM_INVALID_POINTER
*
*************************************************************************/
static STATUS PMS_Peripheral_Device_Unregister_Callback(DV_DEV_ID device, VOID *context)
{
    INT             periph_index = 0;
    STATUS          status = DV_UNEXPECTED_ERROR;
    STATUS          pm_status;
    PM_DEVICE_INFO *rem_dev_info;


    /* Find the index for the device ID */
    pm_status = PMS_Peripheral_Get_Device_Index(device, &periph_index);

    if (pm_status == NU_SUCCESS)
    {
        /* Read the device entry */
        rem_dev_info = PM_Dev_Info_Array[periph_index];

        /* Deallocate memory for the device structure */
        (VOID)NU_Deallocate_Memory(rem_dev_info);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE)

        (VOID) NU_PM_Unmap_System_Power_State(device);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE) */

        /* Clear the entry */
        PM_Dev_Info_Array[periph_index] = NU_NULL;

        /* pm_status is success, so same goes for status. */
        status = NU_SUCCESS;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_Peripheral_Get_Device_Index
*
*   DESCRIPTION
*
*       This function finds the index into the peripheral array for a
*       specific device ID.  If the ID isn't found an error is returned.
*
*   INPUT
*
*       dev_id                              Device ID to search for
*       periph_index_ptr                    Pointer to return index
*
*   OUTPUT
*
*       NU_SUCCESS
*       PM_INVALID_DEVICE_ID
*       PM_INVALID_POINTER
*
*************************************************************************/
STATUS PMS_Peripheral_Get_Device_Index(DV_DEV_ID dev_id, INT *periph_index_ptr)
{
    STATUS  pm_status = PM_INVALID_DEVICE_ID;
    INT     periph_index;

    /* Check the return pointer */
    if (periph_index_ptr == NU_NULL)
    {
        pm_status = PM_INVALID_POINTER;
    }
    else
    {
        /* Search every node until the device ID is found */
        for (periph_index = 0; ((periph_index < PM_MAX_DEV_CNT) && (pm_status != NU_SUCCESS)); periph_index++)
        {
            /* Verify this is a valid device block */
            if (PM_Dev_Info_Array[periph_index] != NU_NULL)
            {
                if (PM_Dev_Info_Array[periph_index] -> dev_id == dev_id)
                {
                    /* Device found, return success and exit the loop */
                    pm_status = NU_SUCCESS;

                    /* Update the return index */
                    *periph_index_ptr = periph_index;
                }
            }
        }
    }

    return (pm_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_Peripheral_New_Device_Index
*
*   DESCRIPTION
*
*       This function finds an unused index into the peripheral array.
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       NU_SUCCESS                      Successful run of the function
*       PM_UNEXPECTED_ERROR             Return value would result in array
*                                       overrun.
*
*************************************************************************/
static STATUS PMS_Peripheral_New_Device_Index(INT *index_ptr)
{
    INT     periph_index = 0;
    STATUS  pm_status = NU_SUCCESS;

    /* Search every node until the device ID is found */
    while ((periph_index < PM_MAX_DEV_CNT) && (PM_Dev_Info_Array[periph_index] != NU_NULL))
    {
        periph_index++;
    }

    if (periph_index < PM_MAX_DEV_CNT)
    {
        /* If the index has valid after the search we found a slot */
        *index_ptr = periph_index;
    }
    else
    {
        /* The index would result in an overrun return an error */
        pm_status = PM_UNEXPECTED_ERROR;
    }

    return (pm_status);
}

#endif  /* ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_PERIPHERAL == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE)) */


