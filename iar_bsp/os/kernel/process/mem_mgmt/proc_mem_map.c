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
*       proc_mem_map.c
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       This file contains the API related to mapping new memory after a
*       process has been loaded.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       NU_Memory_Map
*       NU_Memory_ID_Get
*       NU_Memory_Share
*       NU_Memory_Map_Information
*       PROC_Mapped_Memory_In_Use
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

extern CS_NODE     *PROC_Memory_List;

/*************************************************************************
*
* PUBLIC FUNCTION
*
*       NU_Memory_Map
*
* DESCRIPTION
*
*       Creates a new memory region for the process containing the address
*       passed.
*
* INPUTS
*
*       mem_id      Pointer for the return identifier
*       name        Name of the region
*       phys_addr   Physical address to be mapped.  If this is beyond a
*                   region boundary the boundary would be moved back to
*                   the aligned region containing the memory, size will
*                   be adjusted to compensate.  If the value is
*                   NU_MEMORY_UNDEFINED a region will be obtained of
*                   appropriate size.
*       actual_addr Return value for the region.  If the phys_addr is
*                   not aligned the value returned in actual_addr will
*                   differ.  If phys_addr is NU_NULL and a region of
*                   appropriate size is available this will hold the
*                   starting address.
*       size        Size requested.  May be smaller than the smallest
*                   region size, but it will be adjusted to match region
*                   sizes.  If the size causes overlap with a mapped
*                   region an error will be returned.
*       options     Set of standard MMU memory region values
*                       NU_MEM_READ, NU_MEM_WRITE, NU_MEM_EXEC,
*                       NU_SHARE_READ, NU_SHARE_WRITE, NU_SHARE_EXEC,
*                       NU_CACHE_INHIBIT, NU_CACHE_WRITE_THROUGH,
*                       NU_CACHE_NO_COHERENT
*
* OUTPUTS
*
*       NU_SUCCESS              Successful completion
*       NU_REGION_OVERLAPS      Address space requested overlaps with an
*                               existing region
*       NU_INVALID_SIZE         Requested memory region size is invalid
*       NU_INVALID_OPTIONS      The options either do not exist or an
*                               attempt to set protection options beyond
*                               what is allowed
*       NU_INVALID_POINTER      Pointer for return address is NU_NULL
*
*************************************************************************/
STATUS NU_Memory_Map(INT *mem_id, CHAR *name, VOID *phys_addr,
                     VOID **actual_addr, UNSIGNED size,
                     UNSIGNED options)
{
    STATUS       status = NU_SUCCESS;
    VOID        *addr;
    PROC_MEMORY *region;

    /* Pointer validation */
    NU_ERROR_CHECK((mem_id == NU_NULL), status, NU_INVALID_POINTER);

    /* Verify the options are within what is allowed */
    NU_ERROR_CHECK(((options & ~PROC_VALID_MAP_OPTIONS) != 0), status, NU_INVALID_OPTIONS);
    NU_ERROR_CHECK((size == 0), status, NU_INVALID_SIZE);

    if (status == NU_SUCCESS)
    {
        /* Determine if the address is set */
        if (phys_addr == NU_MEMORY_UNDEFINED)
        {
            /* Adjust size as needed */
            size = PROC_EXPAND_SIZE(size);

            /* User is requesting a memory region at any available address */
            status = PROC_Alloc(&addr, size, PROC_PAGE_SIZE);

            /* Mark this region as having allocated space */
            options |= PROC_MEM_ALLOCATED;
        }
        else
        {
            /* Set the physical address to the requested address,
               force alignment to the beginning of the region containing
               the requested address */
            addr = (VOID *)(((UNSIGNED)phys_addr) & ~(PROC_PAGE_SIZE - 1));

            /* Update size if needed */
            if (phys_addr != addr)
            {
                /* Requested address is not the start of requested
                   region, add that offset to the size */
                size += (UNSIGNED)phys_addr - (UNSIGNED)addr;

                /* Adjust size as needed */
                size = PROC_EXPAND_SIZE(size);
            }
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Mark this region as created by mem map */
        options |= PROC_MEM_MAPPED;

        /* Check for executable region */
        if ((options & NU_MEM_EXEC) != NU_MEM_EXEC)
        {
            /* This region was not marked as executable */
            options |= PROC_NO_EXECUTE;
        }

        /* Lock critical section */
        TCCT_Schedule_Lock();

        /* Attempt to attach the memory */
        status = PROC_Attach_Memory(PROC_Scheduled_CB, name, options,
                                    size, addr, &region);

        /* Unlock critical section */
        TCCT_Schedule_Unlock();
    }

    if (status == NU_SUCCESS)
    {
        /* Return the actual starting address of the newly
           mapped region */
        if (actual_addr != NU_NULL)
        {
            *actual_addr = region -> phys_base;
        }

        /* Return the new region ID */
        *mem_id = (INT)region;
    }

    return (status);
}

/*************************************************************************
*
* PUBLIC FUNCTION
*
*       NU_Memory_Get_ID
*
* DESCRIPTION
*
*       Obtains a memory identifier for a specified address.
*
* INPUTS
*
*       mem_id      Return pointer for the requested identifier
*       phys_addr   Pointer to memory within the address space that an
*                   ID is requested.  This value can take
*                   NU_MEMORY_UNDEFINED to only search by name
*       name        String value that represents a memory region.  If
*                   this is a non null value it will be used with the
*                   phys_addr to find a specific region
*
* OUTPUTS
*
*       NU_SUCCESS                  Successful completion of the service
*       NU_INVALID_MEMORY_REGION    Address space is not mapped
*       NU_INVALID_POINTER          Pointer for return address is NULL
*       NU_INVALID_OPERATION        Both search parameters are ambiguous
*
*************************************************************************/
STATUS NU_Memory_Get_ID(INT *mem_id, VOID *phys_addr, CHAR *name)
{
    STATUS       status = NU_SUCCESS;
    PROC_MEMORY *region;
    CS_NODE     *node;
    BOOLEAN      addr_verified;
    BOOLEAN      name_verified;

    /* Pointer validation */
    NU_ERROR_CHECK((mem_id == NU_NULL), status, NU_INVALID_POINTER);
    NU_ERROR_CHECK(((phys_addr == NU_MEMORY_UNDEFINED) && (name == NU_NULL)), status, NU_INVALID_OPERATION);

    if (status == NU_SUCCESS)
    {
        /* Begin critical section */
        TCCT_Schedule_Lock();

        /* Get first node of the region list */
        node = PROC_Memory_List;

        /* Set return value to unmapped space */
        status = NU_INVALID_MEMORY_REGION;

        /* Loop through all of the regions in the system */
        while ((status == NU_INVALID_MEMORY_REGION) && (node != NU_NULL))
        {
            /* Get the address of the region */
            region = (PROC_MEMORY *)node;

            /* Address test */
            if ((phys_addr == NU_MEMORY_UNDEFINED) ||
                ((((UNSIGNED)region -> phys_base) <= (UNSIGNED)phys_addr) &&
                (((UNSIGNED)region -> phys_base + region -> size) > (UNSIGNED)phys_addr)))
            {
                /* Mark address as found */
                addr_verified = NU_TRUE;
            }
            else
            {
                /* Mark address not found */
                addr_verified = NU_FALSE;
            }

            /* Name test */
            if ((name == NU_NULL) || (strcmp(region -> name, name) == 0))
            {
                /* Mark name as found */
                name_verified = NU_TRUE;
            }
            else
            {
                /* Mark name not found */
                name_verified = NU_FALSE;
            }

            /* If the region being unmapped has any children
               return an error */
            if ((addr_verified == NU_TRUE) && (name_verified == NU_TRUE))
            {
                /* The region being requested to be unmapped
                   is a parent of another region in the system */
                status = NU_SUCCESS;
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

        /* End critical section */
        TCCT_Schedule_Unlock();
    }

    if (status == NU_SUCCESS)
    {
        *mem_id = (INT)region;
    }

    return (status);
}

/*************************************************************************
*
* PUBLIC FUNCTION
*
*       NU_Memory_Share
*
* DESCRIPTION
*
*       Creates a memory region based from the attributes from an existing
*       memory mapped region.  Attributes will be derived from the shared
*       attributes in source region, region size, and region physical
*       address
*
* INPUTS
*
*       mem_id      This parameter will be a pointer for the return
*                   identifier
*       source_id   Identifier of the region that will be shared
*       name        New region name
*
* OUTPUTS
*
*       NU_SUCCESS                  Successful completion of the service
*       NU_INVALID_MEMORY_REGION    Identifier doesn't reference a valid
*                                   memory region
*       NU_INVALID_POINTER          ID return pointer is invalid
*
*************************************************************************/
STATUS NU_Memory_Share(INT *mem_id, INT source_id, CHAR *name)
{
    STATUS       status = NU_SUCCESS;
    PROC_MEMORY *region;
    PROC_MEMORY *new_region;

    /* Read the region */
    region = (PROC_MEMORY *)source_id;

    /* Pointer validation */
    NU_ERROR_CHECK((mem_id == NU_NULL), status, NU_INVALID_POINTER);

    if (status == NU_SUCCESS)
    {
        /* Lock critical section */
        TCCT_Schedule_Lock();

        /* Create a shared region from the source region. Region verification
           will occur in the attach share call. */
        status = PROC_Attach_Share(PROC_Scheduled_CB, region, name, &new_region);

        /* Unlock critical section */
        TCCT_Schedule_Unlock();

        if (status == NU_SUCCESS)
        {
            /* Return the memory region ID */
            *mem_id = (INT)new_region;
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_Memory_Map_Information
*
* DESCRIPTION
*
*       Returns information for a single memory region
*
* INPUTS
*
*       mem_id                              Memory ID
*       info                                Pointer to information structure
*
* OUTPUTS
*
*       NU_SUCCESS                          Info on memory returned
*       NU_INVALID_POINTER                  Invalid pointer passed to function
*       NU_INVALID_MEMORY_REGION            Invalid memory ID passed to function
*
*************************************************************************/
STATUS NU_Memory_Map_Information(INT mem_id, NU_MEMORY_MAP_INFO *info)
{
    STATUS       status = NU_SUCCESS;
    PROC_MEMORY *region;

    /* Check for errors in parameters */
    NU_ERROR_CHECK((info == NU_NULL), status, NU_INVALID_POINTER);

    /* Ensure success */
    if (status == NU_SUCCESS)
    {
        /* Lock reading of the internal control block */
        TCCT_Schedule_Lock();

        /* Get the memory control block for given id */
        region = (PROC_MEMORY *)mem_id;

        /* Check for a valid memory region */
        if ((region != NU_NULL) && (region -> valid == PROC_MEMORY_ID))
        {
            /* Save data for this memory into the info structure */
            info -> phys_base = region -> phys_base;
            info -> virt_base = region -> virt_base;
            info -> size = region -> size;
            info -> attributes = (PROC_VALID_MAP_OPTIONS & (region -> attributes));
            strncpy(info -> name, region -> name, NU_MAX_NAME);
        }
        else
        {
            /* Return status showing invalid memory */
            status = NU_INVALID_MEMORY_REGION;
        }

        /* Critical section over - unlock scheduler */
        TCCT_Schedule_Unlock();
    }

    /* Return status */
    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Mapped_Memory_In_Use
*
* DESCRIPTION
*
*       Verifies that no regions are currently mapped to another process
*
* INPUTS
*
*       process
*
* OUTPUTS
*
*       NU_TRUE         Another process is using a mapped space
*       NU_FALSE        No regions are being mapped by another process
*
*************************************************************************/
BOOLEAN PROC_Mapped_Memory_In_Use(PROC_CB* process)
{
    BOOLEAN      in_use = NU_FALSE;
    PROC_MEMORY *region;
    CS_NODE     *node;

    if (process != NU_NULL)
    {
        /* Get first node of the region list */
        node = process -> owned_regions;

        /* Loop through all of the regions in the process */
        while ((in_use == NU_FALSE) && (node != NU_NULL))
        {
            /* Get the address of the region */
            region = NU_STRUCT_BASE(node, process_region_list, PROC_MEMORY);

            /* Verify the region is valid and not mapped */
            if ((region != NU_NULL) && (region -> valid == PROC_MEMORY_ID) &&
                (region -> parent == NU_NULL) && (region -> child_count != 0))
            {
                /* This region is mapped by another process */
                in_use = NU_TRUE;
            }
            else
            {
                /* Advance to next node in list */
                node = node -> cs_next;

                /* Determine if the end of the list has been found */
                if (node == process -> owned_regions)
                {
                    /* Exit the loop */
                    node = NU_NULL;
                }
            }
        }
    }

    return(in_use);
}
