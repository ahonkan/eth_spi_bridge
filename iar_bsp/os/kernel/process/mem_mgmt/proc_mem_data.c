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
*       proc_mem_data.c
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       This file contains global data structures for use within this
*       component.
*
* DATA STRUCTURES
*
*       PROC_Memory_List
*       PROC_Total_Memory
*       PROC_Kernel_Regions
*
* FUNCTIONS
*
*       PROC_Obtain_New_Memory_CB
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       thread_control.h
*       proc_core.h
*       proc_mem_mgmt.h
*       string.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "os/kernel/process/core/proc_core.h"
#include "proc_mem_mgmt.h"
#include <string.h>

/* PROC_Memory_List is the head pointer of the linked list of
   created memory regions.  If the list is NU_NULL, there are
   no memory regions created. */
CS_NODE            *PROC_Memory_List;

/* PROC_Total_Memory contains the number of currently created
   memory regions. */
UNSIGNED            PROC_Total_Memory;

/* This array contains the kernel regions. */
static PROC_MEMORY  PROC_Kernel_Regions[PROC_KERNEL_REGIONS];

/*************************************************************************
*
* FUNCTION
*
*       PROC_Obtain_New_Memory_CB
*
* DESCRIPTION
*
*       Creates a memory region control block and updates the total
*       number of regions and returns an address to the region
*
* INPUTS
*
*       id                              Associated process ID
*       region                          Return pointer
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       NU_UNAVAILABLE                  Indicates no processes are available
*
*************************************************************************/
STATUS PROC_Obtain_New_Memory_CB(PROC_CB *process, PROC_MEMORY **region)
{
    STATUS       status = NU_UNAVAILABLE;

    /* Verify process and region pointer */
    if ((region != NU_NULL) && (process != NU_NULL))
    {
        /* Check for kernel allocation */
        if (process -> id == PROC_KERNEL_ID)
        {
            /* Verify that there are kernel regions remaining, kernel
               regions are the first regions created and have a fixed
               number */
            if ((process -> owned_total) < PROC_KERNEL_REGIONS)
            {
                /* Use the next element in the array, total
                   will be incremented before the exit of
                   this function */
                *region = &PROC_Kernel_Regions[process -> owned_total];

                /* Mark for success */
                status = NU_SUCCESS;
            }
        }
        else
        {
            /* Allocate a new memory region control block */
            status = PROC_Alloc((VOID **)region, sizeof(PROC_MEMORY), 0);
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Clear the memory */
        memset((*region), 0, sizeof(PROC_MEMORY));

        /* Update global list and count to include new control block
           process will not be valid until internal ID is set */
        NU_Place_On_List(&PROC_Memory_List, &((*region) -> created));
        PROC_Total_Memory += 1;

        /* Set the owner of the block */
        (*region) -> owner = process;

        /* Update the process list of regions */
        NU_Place_On_List(&(process -> owned_regions),
                          &((*region) -> process_region_list));

        /* Increment the number of regions for this process */
        process -> owned_total += 1;
    }

    return(status);
}
