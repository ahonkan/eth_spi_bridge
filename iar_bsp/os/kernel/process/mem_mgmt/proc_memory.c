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
*       proc_memory.c
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       This file contains all functionality to attach regions to
*       processes.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       PROC_Mem_Mgmt_Initialize
*       PROC_Create_Process_Memory
*       PROC_Attach_Memory
*       PROC_Share_Memory
*       PROC_Attach_Share
*       PROC_Memory_Check_Overlap
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       proc_core.h
*       thread_control.h
*       proc_mem_mgmt.h
*       stddef.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "os/kernel/process/core/proc_core.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "proc_mem_mgmt.h"
#include <stddef.h>

extern PROC_KERN_MEM PROC_AR_Kern_Mem_Setup[PROC_KERNEL_REGIONS];
extern CS_NODE      *PROC_Memory_List;

STATUS PROC_AR_Mem_Mgmt_Initialize(VOID);
VOID PROC_AR_MMU_Enable(VOID *ttbr);
static BOOLEAN PROC_Memory_Check_Overlap(VOID *start_addr, UNSIGNED size);

/*************************************************************************
*
* FUNCTION
*
*       PROC_Mem_Mgmt_Initialize
*
* DESCRIPTION
*
*       Creates all memory regions required for process.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       Return value from memory management initialize or attach memory
*
*************************************************************************/
STATUS PROC_Mem_Mgmt_Initialize(VOID)
{
    STATUS status;
    INT    mem_index;

    /* Initialize the arch specific code first */
    status = PROC_AR_Mem_Mgmt_Initialize();

    if (status == NU_SUCCESS)
    {
        /* Loop through all of the kernel regions and add
           them to the system */
        for (mem_index = 0; ((mem_index < PROC_KERNEL_REGIONS) && (status == NU_SUCCESS)); mem_index++)
        {
            /* Lock critical section */
            TCCT_Schedule_Lock();

            /* Setup the kernel memory regions */
            status = PROC_Attach_Memory(PROC_Kernel_CB, PROC_AR_Kern_Mem_Setup[mem_index].name,
                                        PROC_AR_Kern_Mem_Setup[mem_index].attributes,
                                        ((UNSIGNED)PROC_AR_Kern_Mem_Setup[mem_index].end -
                                         (UNSIGNED)PROC_AR_Kern_Mem_Setup[mem_index].start),
                                        PROC_AR_Kern_Mem_Setup[mem_index].start, NU_NULL);

            /* Unlock critical section */
            TCCT_Schedule_Unlock();
        }
    }

    /* Enable the MMU */
    if (status == NU_SUCCESS)
    {
        PROC_AR_MMU_Enable(PROC_Kernel_CB -> translation);
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Create_Process_Memory
*
* DESCRIPTION
*
*       Creates all memory regions required for process.
*
* INPUTS
*
*       process                         Pointer to process control block
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       NU_INVALID_PROCESS              Memory already setup
*       NU_INVALID_POINTER              Null passed for process pointer
*
*************************************************************************/
STATUS PROC_Create_Process_Memory(PROC_CB* process)
{
    STATUS    status = NU_SUCCESS;
    UNSIGNED  size;
    VOID     *addr;

    /* Create the main regions for the process */

    /* Calculate the size of the text region */
    size = (UNSIGNED)(process -> rodata_start) - (UNSIGNED)(process -> text_start);
    if (size != 0)
    {
        /* Calculate the text start location */
        addr = (VOID *)((UNSIGNED)process -> load_addr + (UNSIGNED)process -> text_start);

        /* Lock critical section */
        TCCT_Schedule_Lock();

        /* Create and attach the region to the process */
        status = PROC_Attach_Memory(process, "text", (NU_MEM_EXEC | NU_SHARE_EXEC),
                                    size, addr, NU_NULL);

        /* Unlock critical section */
        TCCT_Schedule_Unlock();
    }

    /* Calculate the size of the rodata region */
    size = (UNSIGNED)(process -> data_bss_start) - (UNSIGNED)(process -> rodata_start);
    if ((status == NU_SUCCESS) && (size != 0))
    {
        /* Calculate the rodata start location */
        addr = (VOID *)((UNSIGNED)process -> load_addr + (UNSIGNED)process -> rodata_start);

        /* Create and attach the region to the process */
        status = PROC_Attach_Memory(process, "rodata", (NU_MEM_READ | NU_SHARE_READ | PROC_NO_EXECUTE_LOADED),
                                    size, addr, NU_NULL);
    }

    /* Calculate the size of the data/bss region */
    size = (UNSIGNED)(process -> root_task.tc_stack_start) - ((UNSIGNED)(process -> data_bss_start) + (UNSIGNED)(process -> load_addr));
    if ((status == NU_SUCCESS) && (size != 0))
    {
        /* Calculate the data/bss start location */
        addr = (VOID *)((UNSIGNED)process -> load_addr + (UNSIGNED)process -> data_bss_start);

        /* Create and attach the region to the process */
        status = PROC_Attach_Memory(process, "data",
                                    (NU_MEM_READ | NU_MEM_WRITE | NU_SHARE_READ | NU_SHARE_WRITE | PROC_NO_EXECUTE_LOADED),
                                    size, addr, NU_NULL);
    }

    /* Verify previous actions and a valid stack size */
    if ((status == NU_SUCCESS) && (process -> root_task.tc_stack_size != 0))
    {
        /* Create and attach the region to the process */
        status = PROC_Attach_Memory(process, "stack",
                                    (NU_MEM_READ | NU_MEM_WRITE | NU_SHARE_READ | NU_SHARE_WRITE | PROC_NO_EXECUTE_LOADED),
                                    process -> root_task.tc_stack_size,
                                    process -> root_task.tc_stack_start, NU_NULL);
    }

    /* Verify previous actions and a valid heap size */
    if ((status == NU_SUCCESS) && (process -> heap_size != 0))
    {
        /* Create and attach the region to the process */
        status = PROC_Attach_Memory(process, "heap",
                                    (NU_MEM_READ | NU_MEM_WRITE | NU_SHARE_READ | NU_SHARE_WRITE | PROC_NO_EXECUTE_LOADED),
                                    process -> heap_size, (VOID *)process -> pool,
                                    NU_NULL);
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Attach_Memory
*
* DESCRIPTION
*
*       Setups a new region control block, and attaches the region
*       to the given process.
*
* INPUTS
*
*       process                         Process that the memory will
*                                       be attached
*       name                            Name of the memory region
*       attributes                      Memory settings
*
*           NU_MEM_READ                 Region is read only
*           NU_MEM_WRITE                Region is writable
*           NU_MEM_EXEC                 Code execution
*           NU_SHARE_READ               Shared as read only
*           NU_SHARE_WRITE              Shared as writable
*           NU_SHARE_EXEC               Shared for execution
*           NU_CACHE_INHIBIT            Region disallows all cache features
*           NU_CACHE_WRITE_THROUGH      Region uses write through cache
*           NU_CACHE_NO_COHERENT        Region does not enable cache
*                                       coherency
*       size                            Size of the region
*       addr                            Start address
*       new_region                      Return pointer to newly created
*                                       memory control block (null allowed)
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       NU_MEMORY_OVERLAPS              Memory overlaps with existing
*                                       region
*       NU_INVALID_PROCESS              Process has already started
*       NU_INVALID_POINTER              Unusable pointer to return memory
*                                       address or process control block
*
*************************************************************************/
STATUS PROC_Attach_Memory(PROC_CB *process, CHAR *name, UNSIGNED attributes,
                          UNSIGNED size, VOID *addr, PROC_MEMORY **new_region)
{
    STATUS       status = NU_SUCCESS;
    PROC_MEMORY *region;

    /* If this isn't a shared region verify there isn't
       overlap with an existing region */
    if ((attributes & PROC_MEM_SHARED) != PROC_MEM_SHARED)
    {
        /* Check for overlap */
        if (PROC_Memory_Check_Overlap(addr, size) == NU_TRUE)
        {
            /* Return overlap error */
            status = NU_MEMORY_OVERLAPS;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Obtain a new region */
        status = PROC_Obtain_New_Memory_CB(process, &region);
    }

    if (status == NU_SUCCESS)
    {
        /* Copy name to control block */
        strncpy(region -> name, name, (NU_MAX_NAME - 1));

        /* Adjust size of the region */
        size = PROC_EXPAND_SIZE(size);

        /* Set the size of the region */
        region -> size = size;

        /* Save the attributes */
        region -> attributes = attributes;

        /* Set the start address of the region */
        region -> phys_base = addr;

        /* Setup the target specific */
        status = PROC_AR_Attach_Memory(process, region);

        if (status == NU_SUCCESS)
        {
            /* Set a valid control block ID code */
            region -> valid = PROC_MEMORY_ID;

            /* Check to see if region pointer should be returned */
            if (new_region != NU_NULL)
            {
                *new_region = (VOID *)region;
            }
        }
    }

    return(status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Share_Memory
*
* DESCRIPTION
*
*       This service shares regions with appropriate attributes in one
*       process with another.
*
* INPUTS
*
*       source_id                       Process ID to share with
*       target_id                       Process ID to add shared regions
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful completion of
*                                       the service
*
*************************************************************************/
STATUS PROC_Share_Memory(PROC_CB* src_proc, PROC_CB* tgt_proc)
{
    STATUS       status = NU_INVALID_PROCESS;
    PROC_MEMORY *region;
    CS_NODE     *node;

    /* Make sure the source and target are different */
    if (src_proc -> id != tgt_proc -> id)
    {
        /* Update status based on source process being valid */
        status = NU_SUCCESS;

        /* Get first node of the region list */
        node = src_proc -> owned_regions;

        /* Loop through all of the regions in the source process */
        while ((node != NU_NULL) && (status == NU_SUCCESS))
        {
            /* Get the address of the region */
            region = NU_STRUCT_BASE(node, process_region_list, PROC_MEMORY);

            /* Share and attach the memory region */
            status = PROC_Attach_Share(tgt_proc, region, "shared", NU_NULL);

            /* This is a blind search and unsharable regions may be found */
            if (status == NU_INVALID_MEMORY_REGION)
            {
                /* Don't exit the loop for this region */
                status = NU_SUCCESS;
            }

            /* Advance to next node in list */
            node = node -> cs_next;

            /* Determine if the end of the list has been found */
            if (node == src_proc -> owned_regions)
            {
                /* Exit the loop */
                node = NU_NULL;
            }
        }
    }

    return(status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Attach_Share
*
* DESCRIPTION
*
*       This service shares a specific region with appropriate
*       attributes with the target process.
*
* INPUTS
*
*       proc                            Process to attach
*       region                          Region to share
*       name                            Name of new region
*       new_region                      Return pointer of shared region
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful completion of
*                                       the service
*
*************************************************************************/
STATUS PROC_Attach_Share(PROC_CB* proc, PROC_MEMORY *region, CHAR *name,
                         PROC_MEMORY **new_region)
{
    STATUS       status = NU_SUCCESS;
    UNSIGNED     attr;
    PROC_MEMORY *child_region;

    /* Region verification */
    NU_ERROR_CHECK((region == NU_NULL), status, NU_INVALID_MEMORY_REGION);
    NU_ERROR_CHECK((region -> valid != PROC_MEMORY_ID), status, NU_INVALID_MEMORY_REGION);
    NU_ERROR_CHECK(((region -> attributes & (NU_SHARE_READ | NU_SHARE_WRITE | NU_SHARE_EXEC)) == 0), status, NU_INVALID_MEMORY_REGION);

    if (status == NU_SUCCESS)
    {
        /* Set/translate the new shared options */
        attr = (region -> attributes & (NU_SHARE_READ | NU_SHARE_WRITE | NU_SHARE_EXEC));

        /* Shift the attributes to match non-shared values */
        attr >>= 3;

        /* Cache attributes are inherited. */
        attr |= (region -> attributes & (NU_CACHE_INHIBIT | NU_CACHE_WRITE_THROUGH | NU_CACHE_NO_COHERENT));

        /* If this is a region created using memory map add that attribute */
        attr |= (region -> attributes & PROC_MEM_MAPPED);

        /* Maintain no execute attributes */
         attr |= (region -> attributes & PROC_NO_EXECUTE);

        /* Create the new region */
        status = PROC_Attach_Memory(proc, name, (attr | PROC_MEM_SHARED), region -> size, region -> phys_base, &child_region);

        if (status == NU_SUCCESS)
        {
            /* Point to original region */
            child_region -> parent = region;

            /* Increment the dependency count */
            region -> child_count += 1;

            /* If the newly shared region is needed pass it back */
            if (new_region != NU_NULL)
            {
                *new_region = child_region;
            }
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Memory_Check_Overlap
*
* DESCRIPTION
*
*       This routine checks the new region settings with existing
*       regions to insure that the new region will not conflict
*       with an already created region.
*
* INPUTS
*
*       start_addr
*       size
*
* OUTPUTS
*
*       NU_FALSE            The regions do not overlap
*       NU_TRUE             The regions overlap
*
*************************************************************************/
static BOOLEAN PROC_Memory_Check_Overlap(VOID *start_addr, UNSIGNED size)
{
    BOOLEAN      overlap;
    PROC_MEMORY *region;
    CS_NODE     *node;
    UNSIGNED     region_start;
    UNSIGNED     region_end;
    UNSIGNED     test_start;
    UNSIGNED     test_end;

    /* Initialize local variables */
    overlap = NU_FALSE;
    region_start = (UNSIGNED)start_addr;
    region_end = region_start + size;

    /* Get first node of the region list */
    node = PROC_Memory_List;

    /* Loop through all of the regions in the system */
    while ((overlap == NU_FALSE) && (node != NU_NULL))
    {
        /* Get the address of the region */
        region = (PROC_MEMORY *)node;

        /* Calculate the start and end addresses of the test region */
        test_start = (UNSIGNED)(region -> phys_base);
        test_end = test_start + region -> size;

        /* Test for overlap with the current region */
        if(((region_start >= test_start) && (region_start < test_end)) ||
            ((region_end > test_start) && (region_end <= test_end)))
        {
            overlap = NU_TRUE;
        }

        /* Advance to next node in list */
        node = node -> cs_next;

        /* Determine if the end of the list has been found */
        if (node == PROC_Memory_List)
        {
            /* Exit the loop */
            node = NU_NULL;
        }
    }

    return(overlap);
}
