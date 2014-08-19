/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
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
*       hibernate_dv_interface.c
*
*   COMPONENT
*
*        Hibernate driver
*
*   DESCRIPTION
*
*       This file contains the Hibernate Middleware specific functions.
*
*   FUNCTIONS
*
*       Hibernate_Dv_Register
*       Hibernate_Dv_Open
*       Hibernate_Dv_Close
*       Hibernate_Dv_Ioctl
*       Hibernate_Dv_Unregister
*       Hibernate_Dv_Store_Memory
*       Hibernate_Dv_Save_Dyn_Mem
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "os/services/power/core/inc/hibernate.h"
#include "os/services/power/core/inc/nvm.h"
#include "os/kernel/plus/core/inc/dynamic_memory.h"
#include <string.h>

/*********************************/
/* MACROS                        */
/*********************************/

/*******************************/
/* LOCAL VARIABLE DECLARATIONS */
/*******************************/

/*********************************/
/* INTERNAL FUNCTION PROTOTYPES  */
/*********************************/
STATUS Hibernate_Dv_Store_Memory(VOID);
VOID PMS_Hibernate_Exit(VOID);

static VOID Hibernate_Dv_Save_Dyn_Mem(NU_MEMORY_POOL *pool_ptr, 
                                      HIBERNATE_SAVE_POOL_INFO *pool_info_ptr);

/*********************************/
/* FUNCTION DEFINITIONS          */
/*********************************/

/*************************************************************************
*
*   FUNCTION
*
*       Hibernate_Dv_Register
*
*   DESCRIPTION
*
*       This function registers the hardware
*
*   INPUTS
*
*       key                      - Path to registry
*       instance_handle          - Hibernate instance structure
*
*   OUTPUTS
*
*       status                   - NU_SUCCESS
*
*************************************************************************/
STATUS  Hibernate_Dv_Register(const CHAR *key, VOID *instance_handle)
{
    DV_DRV_FUNCTIONS functions;
    DV_DEV_ID        dev_id;
    DV_DEV_LABEL     class_id[] = {{HIBERNATE_LABEL}};

    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(instance_handle);

    /* Update the functions */
    functions.drv_open_ptr  = Hibernate_Dv_Open;
    functions.drv_close_ptr = Hibernate_Dv_Close;
    functions.drv_read_ptr  = NU_NULL;
    functions.drv_write_ptr = NU_NULL;
    functions.drv_ioctl_ptr = Hibernate_Dv_Ioctl;

    Hibernate_Tgt_Initialize();

    /* Register with the device manager */
    return (DVC_Dev_Register(NU_NULL, class_id, DV_GET_LABEL_COUNT(class_id),
                             &functions, &dev_id));
}

/*************************************************************************
*
*   FUNCTION
*
*       Hibernate_Dv_Unregister
*
*   DESCRIPTION
*
*       This function unregisters the hardware
*
*   INPUTS
*
*       key                      - Path to registry
*       startstop                - Option to Register or Unregister
*       dev_id                   - Device ID
*
*   OUTPUTS
*
*        status                  - NU_SUCCESS
*
*************************************************************************/
STATUS Hibernate_Dv_Unregister(const CHAR *key, INT startstop, DV_DEV_ID dev_id)
{
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       Hibernate_Dv_Open
*
*   DESCRIPTION
*
*       This function opens the device and creates a session handle
*
*   INPUTS
*
*       instance_handle         - Device ID
*       labels_list[]           - Access mode (label) of open
*       labels_cnt              - Number of labels
*       session_handle          - Pointer to Pointer of session handle
*
*   OUTPUTS
*
*       success                  - NU_SUCCESS
*
*************************************************************************/
STATUS Hibernate_Dv_Open(VOID *instance_handle, DV_DEV_LABEL label_list[],
                         INT label_cnt, VOID**session_handle)
{
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       Hibernate_DV_Close
*
*   DESCRIPTION
*
*       This function closes the device and deletes the session handle
*
*   INPUTS
*
*       sess_handle                - Session handle of the device
*
*   OUTPUTS
*
*       status                     - NU_SUCCESS
*
*************************************************************************/
STATUS Hibernate_Dv_Close(VOID *session_handle)
{
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       Hibernate_DV_Ioctl
*
*   DESCRIPTION
*
*       This function provides IOCTL functionality.
*
*   INPUTS
*
*       sess_handle_ptr             - Session handle of the driver
*       ioctl_cmd                   - Ioctl command
*       data                        - Ioctl data pointer
*       length                      - Ioctl length
*
*   OUTPUTS
*
*       status                      - NU_SUCCESS
*
*************************************************************************/
STATUS Hibernate_Dv_Ioctl(VOID *session_handle, INT ioctl_cmd, VOID *data, INT length)
{
    DV_IOCTL0_STRUCT        *ioctl0;
    STATUS                  status = DV_INVALID_INPUT_PARAMS;
    DV_DEV_LABEL            hibernate_label = {HIBERNATE_LABEL};

    /* Avoid warnings from unused parameters */
    NU_UNUSED_PARAM(session_handle);

    switch (ioctl_cmd)
    {
        case DV_IOCTL0:

            /* Verify data pointer and length */
            if ((data != NU_NULL) && (length == sizeof(DV_IOCTL0_STRUCT)))
            {
                /* Get the ioctl0 structure from the data passed in */
                ioctl0 = (DV_IOCTL0_STRUCT *) data;

                if (DV_COMPARE_LABELS(&ioctl0 -> label, &hibernate_label))
                {
                    ioctl0 -> base = HIBERNATE_IOCTL_BASE;
                    status = NU_SUCCESS;
                }
                else
                {
                    status = DV_INVALID_INPUT_PARAMS;
                }
            }
            break;


        case (HIBERNATE_IOCTL_BASE + HIBERNATE_IOCTL_INIT_SHUTDOWN):

            /* Store memory resources in preparation for shutdown. */
            status = Hibernate_Dv_Store_Memory();

            if (status == NU_SUCCESS)
            {
                /* Shut down the target. */
                status = Hibernate_Tgt_Shutdown();
            }

            break;

        case (HIBERNATE_IOCTL_BASE + HIBERNATE_IOCTL_INIT_STANDBY):

            /* Call target specific function here */
            status = NU_SUCCESS;
            Hibernate_Tgt_Standby_Enter();

            break;

        case (HIBERNATE_IOCTL_BASE + HIBERNATE_IOCTL_EXIT_STANDBY):

            /* Call target specific function here */
            status = NU_SUCCESS;
            Hibernate_Tgt_Standby_Exit();

            break;

        case (HIBERNATE_IOCTL_BASE + HIBERNATE_IOCTL_TGT_EXIT):

            /* Call target specific function here. */
            status = NU_SUCCESS;
            Hibernate_Tgt_Exit();

            break;

        default:

        status = NU_SUCCESS;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Hibernate_Dv_Store_Memory
*
*   DESCRIPTION
*
*       This function gets all target specific regions and sends that
*       information to the NVM driver.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates successful operation.
*
*       NU_INVALID_OPERATION - Indicates no NVM device found.
*
*       <OTHER> - Indicates (other) internal error occurred.
*
*************************************************************************/
STATUS Hibernate_Dv_Store_Memory(VOID)
{
    STATUS                   status = NU_SUCCESS;
    UINT32                   count, region, size;
    UINT                     index;
    INT                      int_level;
    HIBERNATE_SAVE_POOL_INFO sys_pool_info;
    HIBERNATE_SAVE_POOL_INFO usys_pool_info;
    HB_RESUME_CB             resume_cb;
    NU_MEMORY_POOL          *sys_mem;
    NU_MEMORY_POOL          *usys_mem;
    VOID                    *region_start;

    /* Avoid warnings when NVM not available */
    NU_UNUSED_PARAM(index);

    /* Get the number of regions. */
    count = Hibernate_Tgt_Get_Region_Count();

    /* If number of volatile memory regions is 0 there is nothing to
       do here. */
    if (count > 0)
    {
        /* Disable interrupts for the operation. */
        int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Call target function to do any pre-save setup such as a cache
           flush. */
        Hibernate_Tgt_Pre_Save();

        /* Reset the NVM driver and device. */
        status = PM_NVM_RESET();

        if (status == NU_SUCCESS)
        {
            /* Setup the hibernate resume structure. */
            resume_cb.resume_pending = NU_TRUE;
            resume_cb.hb_exit_func = PMS_Hibernate_Exit;
            for (region = 0; region < HB_MAX_REGION_COUNT; region++)
            {
                resume_cb.restore_addr[region] = NU_NULL;
            }

            /* Save the restore address as the starting address. */
            region = 0;
            while (region < count)
            {
                Hibernate_Tgt_Get_Region_Info(region,
                                              &(resume_cb.restore_addr[region]),
                                              &size);
                region++;
            }

            /* Get memory pool resources. */
            status = NU_System_Memory_Get(&sys_mem, &usys_mem);
        }

        if (status == NU_SUCCESS)
        {
            /* Save cached system memory pool ranges. */
            Hibernate_Dv_Save_Dyn_Mem(sys_mem, &sys_pool_info);
            
            /* Add the system memory pool to the region list */
            resume_cb.restore_addr[region] = sys_pool_info.hib_pool_start_addr;
            region++;
            
            /* Add the system memory pool footer to the region list */
            resume_cb.restore_addr[region] = sys_pool_info.hib_pool_footer_addr;
            region++;

            /* Save uncached system memory pool ranges. */
            Hibernate_Dv_Save_Dyn_Mem(usys_mem, &usys_pool_info);
            
            /* Add uncached system memory pool to the region list */
            resume_cb.restore_addr[region] = usys_pool_info.hib_pool_start_addr;
            region++;
            
            /* Add the uncached system memory pool footer to the region list */
            resume_cb.restore_addr[region] = usys_pool_info.hib_pool_footer_addr;

            /* Save the hibernate resume structure to NVM. */
            status = PM_NVM_WRITE(&resume_cb, sizeof(resume_cb), &index, NU_FALSE);
        }

        if (status == NU_SUCCESS)
        {
            /* NOTE: At this point the NVM contains the hibernate resume
               structure indicating a pending resume operation. */

            /* Write the static memory resources to NVM. */
            region = 0;
            while ((status == NU_SUCCESS) &&
                   (region < count))
            {
                Hibernate_Tgt_Get_Region_Info(region, &region_start, &size);
                status = PM_NVM_WRITE(region_start, size, &index, NU_FALSE);
                region++;
            }

            /* Write the dynamic memory pools to NVM. */
            if (status == NU_SUCCESS)
            {
                /* Write the main portion of the system memory pool to NVM */
                status = PM_NVM_WRITE(sys_pool_info.hib_pool_start_addr, sys_pool_info.hib_pool_size, &index, NU_FALSE);
            }
            
            if (status == NU_SUCCESS)
            {
                /* Write the footer of the system memory pool to NVM */
                status = PM_NVM_WRITE(sys_pool_info.hib_pool_footer_addr, sys_pool_info.hib_pool_footer_size, &index, NU_FALSE);
            }
            
            if (status == NU_SUCCESS)
            {
                /* Write the main portion of the uncached system memory pool to NVM */
                status = PM_NVM_WRITE(usys_pool_info.hib_pool_start_addr, usys_pool_info.hib_pool_size, &index, NU_FALSE);
            }
            
            if (status == NU_SUCCESS)
            {
                /* Write the footer of the uncached system memory pool to NVM */
                status = PM_NVM_WRITE(usys_pool_info.hib_pool_footer_addr, usys_pool_info.hib_pool_footer_size, &index, NU_FALSE);
            }

            /* ERROR RECOVERY: If an error is detected during writing to
               NVM then the hibernate resume structure will be updated to
               indicate there is no pending hibernate resume to ensure
               that corrupted NVM is not used during the next restart. */
            if (status != NU_SUCCESS)
            {
                /* Reset the NVM driver write operation. */
                (VOID)PM_NVM_RESET();

                /* Update the hibernate resume structure in indicate there
                   is no pending hibernate resume operation. */
                resume_cb.resume_pending = NU_FALSE;

                /* Save the hibernate resume structure to NVM. */
                (VOID)PM_NVM_WRITE((VOID *)&resume_cb, sizeof(resume_cb), &index, NU_FALSE);

                /* Close the NVM device. */
                (VOID)PM_NVM_CLOSE();
            }

        }

        if (status == NU_SUCCESS)
        {
            /* Close the NVM device. */
            status = PM_NVM_CLOSE();
        }

        /* Restore interrupt level. */
        (VOID)NU_Local_Control_Interrupts(int_level);
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Hibernate_Dv_Save_Dyn_Mem
*
*   DESCRIPTION
*
*       This function...
*
*   INPUTS
*
*       pool_ptr            Pointer to memory pool to store
*       dev_handle          Handle to open NVM device
*       base                Ioctl base
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID Hibernate_Dv_Save_Dyn_Mem(NU_MEMORY_POOL *pool_ptr, 
                                      HIBERNATE_SAVE_POOL_INFO *pool_info_ptr)
{
    DM_HEADER             *memory_ptr;
    DM_HEADER             *end_ptr;
    DM_HEADER             *last_memory;

    /*****************************************/
    /* Find the last allocation in the table */
    /*****************************************/

    /* Initialize the loop */
    last_memory = NU_NULL;
    memory_ptr = pool_ptr -> dm_memory_list;
    end_ptr = (DM_HEADER*)pool_ptr -> dm_start_address;
    end_ptr = end_ptr -> dm_previous_memory;
    do
    {
        /* Only count areas that are allocated */
        if (memory_ptr -> dm_memory_free == NU_FALSE)
        {
            /* record last memory, compare with current */
            if ((memory_ptr > last_memory) && (memory_ptr != end_ptr))
            {
                last_memory = memory_ptr;
            }
        }

        /* Move to next header */
        memory_ptr = memory_ptr -> dm_next_memory;

    } while (memory_ptr != pool_ptr -> dm_memory_list);

    /* Check to see if the pool has any allocations */
    if (last_memory != NU_NULL)
    {
        /* Move the last memory so contain the allocated area */
        last_memory = last_memory -> dm_next_memory;
    }
    else
    {
        /* No allocations set last memory to start of the pool
           to save the headers */
        last_memory = pool_ptr -> dm_start_address;
    }

    /* Add the size of the header of the next free block */
    last_memory = (DM_HEADER *)(((UNSIGNED)last_memory) + sizeof(DM_HEADER));

    /* Setup the write structure */
    pool_info_ptr -> hib_pool_start_addr = pool_ptr -> dm_start_address;
    pool_info_ptr -> hib_pool_size = ((UNSIGNED)last_memory) - ((UNSIGNED)pool_ptr -> dm_start_address);

    /* Save the final free block */
    pool_info_ptr -> hib_pool_footer_addr = (VOID *)end_ptr;
    pool_info_ptr -> hib_pool_footer_size = sizeof(DM_ALLOCATED);
}


