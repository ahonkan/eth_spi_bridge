/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       proc_mem_delete.c
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       This file contains functionality to remove memory regions
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       PROC_Delete_Memory
*       PROC_Delete_Process_Memory
*       PROC_Unshare_Memory
*       NU_Memory_Unmap
*       PROC_Mapped_Memory_Clean_Up
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       proc_core.h
*       thread_control.h
*       proc_mem_mgmt.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "os/kernel/process/core/proc_core.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "proc_mem_mgmt.h"

/* External symbols */
extern CS_NODE     *PROC_Memory_List;
extern UNSIGNED     PROC_Total_Memory;

/*************************************************************************
*
* FUNCTION
*
*       PROC_Delete_Memory
*
* DESCRIPTION
*
*       This function removes the memory region from all appropriate
*       lists and removes the validity value.
*
* INPUTS
*
*       region
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*
*************************************************************************/
static STATUS PROC_Delete_Memory(PROC_MEMORY *region)
{
    STATUS  status;

    /* Detach the memory */
    status = PROC_AR_Detach_Memory(region);

    if (status == NU_SUCCESS)
    {
        /* Lock the process deletion */
        TCCT_Schedule_Lock();

        /* Remove from the list */
        NU_Remove_From_List(&PROC_Memory_List, &(region -> created));
        PROC_Total_Memory -= 1;

        /* Update the process list of regions */
        NU_Remove_From_List(&(region -> owner -> owned_regions),
                            &(region -> process_region_list));

        /* Decrement the number of regions for this process */
        region -> owner -> owned_total -= 1;

        /* Mark region as invalid */
        region -> valid = 0;

        /* Determine if this was a shared region */
        if (region -> parent != NU_NULL)
        {
            /* Decrement the dependency count */
            region -> parent -> child_count -= 1;
        }

        /* Unlock the process deletion critical section */
        TCCT_Schedule_Unlock();

        /* Free the region control block */
        status = PROC_Free((VOID *)region);
    }

    return(status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Delete_Process_Memory
*
* DESCRIPTION
*
*       Removes all regions from the process
*
* INPUTS
*
*       process
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*
*************************************************************************/
STATUS PROC_Delete_Process_Memory(PROC_CB *process)
{
    STATUS       status = NU_SUCCESS;
    PROC_MEMORY *region;
    CS_NODE     *node;

    /* Verify the process is valid */
    NU_ERROR_CHECK((process == NU_NULL), status, NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
        /* Get first node of the region list */
        node = process -> owned_regions;

        /* Loop through all of the regions in the process */
        while (node != NU_NULL)
        {
            /* Get the address of the region */
            region = NU_STRUCT_BASE(node, process_region_list, PROC_MEMORY);

            /* Verify the region */
            if ((region != NU_NULL) && (region -> valid == PROC_MEMORY_ID))
            {
                status = PROC_Delete_Memory(region);
            }

            /* Refresh the node */
            node = process -> owned_regions;
        }

        /* Clean up all remaining process items, such as translation tables */
        status = PROC_AR_Mem_Mgmt_Cleanup(process);
    }

    return(status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Unshare_Memory
*
* DESCRIPTION
*
*       Removes all shared regions in the target process that were
*       previously shared with the source
*
* INPUTS
*
*       src_proc
*       tgt_proc
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*
*************************************************************************/
STATUS PROC_Unshare_Memory(PROC_CB* src_proc, PROC_CB* tgt_proc)
{
    STATUS       status = NU_SUCCESS;
    PROC_MEMORY *region;
    CS_NODE     *node;

    /* Verify the process is valid */
    NU_ERROR_CHECK((src_proc == NU_NULL), status, NU_INVALID_POINTER);
    NU_ERROR_CHECK((tgt_proc == NU_NULL), status, NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
        /* Get first node of the region list */
        node = tgt_proc -> owned_regions;

        /* Loop through all of the regions in the process */
        while ((status == NU_SUCCESS) && (node != NU_NULL))
        {
            /* Get the address of the region */
            region = NU_STRUCT_BASE(node, process_region_list, PROC_MEMORY);

            /* Verify the region is valid and a member of the target process */
            if ((region != NU_NULL) && (region -> valid == PROC_MEMORY_ID) &&
                (region -> parent != NU_NULL) && (region -> parent -> owner == src_proc))
            {
                /* This is a region shared with the target process
                   remove it */
                status = PROC_Delete_Memory(region);

                if (status == NU_SUCCESS)
                {
                    /* Refresh the node list, as the node may have been removed from the list */
                    node = tgt_proc -> owned_regions;
                }
            }
            else
            {
                /* Advance to next node in list */
                node = node -> cs_next;

                /* Determine if the end of the list has been found */
                if (node == tgt_proc -> owned_regions)
                {
                    /* Exit the loop */
                    node = NU_NULL;
                }
            }
        }
    }

    return(status);
}

/*************************************************************************
*
* PUBLIC FUNCTION
*
*       NU_Memory_Unmap
*
* DESCRIPTION
*
*       Removes any region associated with the unique identifier
*
* INPUTS
*
*       mem_id      Identifier of the region to be unmapped
*
* OUTPUTS
*
*       NU_SUCCESS                  Successful completion
*       NU_INVALID_MEMORY_REGION    Identifier doesn't reference a valid
*                                   memory region
*       NU_MEMORY_IS_SHARED         Memory region is shared by other memory
*                                   regions and cannot be deleted
*
*************************************************************************/
STATUS NU_Memory_Unmap(INT mem_id)
{
    STATUS       status = NU_SUCCESS;
    PROC_MEMORY *region;

    /* Read the region */
    region = (PROC_MEMORY *)mem_id;

    /* Region verification */
    NU_ERROR_CHECK((region == NU_NULL), status, NU_INVALID_MEMORY_REGION);
    NU_ERROR_CHECK((region -> valid != PROC_MEMORY_ID), status, NU_INVALID_MEMORY_REGION);
    NU_ERROR_CHECK(((region -> attributes & PROC_MEM_MAPPED) != PROC_MEM_MAPPED), status, NU_INVALID_MEMORY_REGION);

    /* Check to see if this is a shared region, shared regions cannot be parents,
       if the requested region to be unmapped is a parent of another region
       return error. */
    NU_ERROR_CHECK(((region -> parent == NU_NULL) && (region -> child_count != 0)), status, NU_MEMORY_IS_SHARED);

    if (status == NU_SUCCESS)
    {
        /* Determine if this region was allocated
           when mapped */
        if ((region -> attributes & PROC_MEM_ALLOCATED) == PROC_MEM_ALLOCATED)
        {
            /* Free the space mapped by the region */
            status = PROC_Free(region -> phys_base);
        }

        if (status == NU_SUCCESS)
        {
            /* It is safe to remove this region */
            status = PROC_Delete_Memory(region);
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Mapped_Memory_Clean_Up
*
* DESCRIPTION
*
*       Finds any mapped regions in the process and unmaps them.
*
* INPUTS
*
*       process
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID PROC_Mapped_Memory_Clean_Up(PROC_CB* process)
{
    PROC_MEMORY *region;
    CS_NODE     *node;

    if (process != NU_NULL)
    {
        /* Get first node of the region list */
        node = process -> owned_regions;

        /* Loop through all of the regions in the process */
        while (node != NU_NULL)
        {
            /* Get the address of the region */
            region = NU_STRUCT_BASE(node, process_region_list, PROC_MEMORY);

            /* Advance to next node in list before the check
               to avoid reseting the search */
            node = node -> cs_next;

            /* Determine if the end of the list has been found */
            if (node == process -> owned_regions)
            {
                /* Exit the loop */
                node = NU_NULL;
            }

            /* Verify the region is valid and not mapped */
            if ((region != NU_NULL) && (region -> valid == PROC_MEMORY_ID) &&
                ((region -> attributes & PROC_MEM_MAPPED) == PROC_MEM_MAPPED))
            {
                /* This is a mapped region, unmap it */
                (VOID)NU_Memory_Unmap((INT)region);
            }
        }
    }
}
