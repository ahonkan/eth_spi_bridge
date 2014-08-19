/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       esal_mem.c
*
*   COMPONENT
*
*       ESAL - Embedded Software Abstraction Layer
*
*   DESCRIPTION
*
*       This file contains the generic functions related to memory
*
*   FUNCTIONS
*
*       ESAL_GE_MEM_Region_Addr_Search      Search for region (static)
*       ESAL_GE_MEM_Remaining_Size_Get      Get size of mem region
*       ESAL_GE_MEM_Next_Match_Find         Find addr of next matching
*                                           memory
*
*   DEPENDENCIES
*
*       esal.h                              Embedded Software
*                                           Abstraction Layer external
*                                           interface
*
***********************************************************************/

/* Include required header files */
#include        "nucleus.h"
#include        "os/kernel/plus/core/inc/esal.h"

/* Define global variables */
#if (CFG_NU_OS_KERN_PLUS_CORE_ROM_SUPPORT == NU_TRUE)
const   INT     ESAL_GE_MEM_ROM_Support_Enabled = CFG_NU_OS_KERN_PLUS_CORE_ROM_SUPPORT;
#else
const   INT     ESAL_GE_MEM_ROM_Support_Enabled = NU_FALSE;
#endif

/* Prototype local functions */
static  INT     ESAL_GE_MEM_Region_Addr_Search(VOID                 *start_addr,
                                               ESAL_GE_MEM_REGION   **mem_region,
                                               VOID                 **region_end);

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_MEM_Region_Addr_Search
*
*   DESCRIPTION
*
*       This function searches for a particular start address in all
*       the memory regions.  If a region is found containing the
*       start address, a pointer to this region is returned to the
*       caller along with the end address of this region.
*
*   CALLED BY
*
*       ESAL_GE_MEM_Remaining_Size_Get
*       ESAL_GE_MEM_Next_Match_Get
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       start_addr                          Start address to search for
*       mem_region                          Pointer to memory region
*                                           returned to caller
*       region_end                          Pointer to end address
*
*   OUTPUTS
*
*       INT                                 Index into region array for
*                                           memory region found
*
***********************************************************************/
static  INT     ESAL_GE_MEM_Region_Addr_Search(VOID                 *start_addr,
                                               ESAL_GE_MEM_REGION   **mem_region,
                                               VOID                 **region_end)
{
    INT         count = 0;
    INT         region_found = NU_FALSE;
    VOID        *region_start;


    /* Loop through all memory regions */
    while ((!region_found) && (count < ESAL_DP_MEM_Num_Regions))
    {
        /* Get start and end addresses for memory region */
        region_start = ESAL_DP_MEM_Region_Data[count].virtual_start_addr;
        *region_end = (VOID *)((VOID_CAST)region_start +
                               (UINT)ESAL_DP_MEM_Region_Data[count].size);

        /* Check if address is within the current region */
        if ( (((VOID_CAST)start_addr) >= ((VOID_CAST)region_start)) &&
             (((VOID_CAST)start_addr) < ((VOID_CAST)*region_end)) )
        {
            /* Set flag to show region found */
            region_found = NU_TRUE;
        }
        else
        {
            /* Check next region */
            count++;
        }

    }   /* while */

    /* Check if region found */
    if (region_found)
    {
        /* Point memory region to found region */
        *mem_region = (ESAL_GE_MEM_REGION *)&ESAL_DP_MEM_Region_Data[count];
    }
    else
    {
        /* Point memory region to NULL */
        *mem_region = NU_NULL;
        count = 0;
    }

    /* Return region number */
    return (count);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_MEM_Remaining_Size_Get
*
*   DESCRIPTION
*
*       This function gets the remaining memory size, in bytes, starting
*       at the specified start address and up to the end of the containing
*       memory region
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       ESAL_GE_MEM_Region_Addr_Search      Find a region given an addr
*
*   INPUTS
*
*       start_addr                          Start addr of memory
*
*   OUTPUTS
*
*       UINT32                              Size of remaining memory
*
***********************************************************************/
UINT32    ESAL_GE_MEM_Remaining_Size_Get(VOID *start_addr)
{
    VOID                *region_end;
    UINT32              remaining_size;
    ESAL_GE_MEM_REGION  *region;


    /* Get region information for this memory */
    ESAL_GE_MEM_Region_Addr_Search(start_addr, &region, &region_end);

    /* Check if region found */
    if (region)
    {
        /* Calculate remaining size (in bytes) */
        remaining_size = (UINT32)((VOID_CAST)region_end -
                                  (VOID_CAST)start_addr);
    }
    else
    {
        /* Set remaining size to error since region not found */
        remaining_size = (UINT32)ESAL_GE_MEM_ERROR;
    }

    /* Return remaining size to caller */
    return (remaining_size);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_GE_MEM_Next_Match_Find
*
*   DESCRIPTION
*
*       This function finds the next memory region matching the memory
*       type and memory cache type starting from the specified address.
*       The start address of a region matching these criteria is returned
*       to the caller.  If memory matching these criteria is not found,
*       an error value is returned to the caller.
*
*       NOTE:  If the region containing the start address matches
*              the requested attributes, the start address will be
*              returned to the caller
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       ESAL_GE_MEM_Region_Addr_Search      Find a region given an addr
*
*   INPUTS
*
*       start_addr                          Start addr to start search
*       cache_type                          Cache type of memory
*                                           searching for
*       mem_type                            Type of memory searching for
*                                           (ie ROM / RAM / etc)
*
*   OUTPUTS
*
*       VOID *                              Pointer to memory found
*       ESAL_GE_MEM_ERROR                   Error - no memory found
*                                           matching request
*
***********************************************************************/
VOID    *ESAL_GE_MEM_Next_Match_Find (VOID                   *start_addr,
                                      ESAL_GE_CACHE_TYPE     cache_type,
                                      ESAL_GE_MEMORY_TYPE    mem_type,
                                      UINT32                 access_type)
{
    VOID                *region_end;
    ESAL_GE_MEM_REGION  *region;
    INT                 region_num;
    INT                 total_regions_searched;


    /* Get region information for this memory */
    region_num = ESAL_GE_MEM_Region_Addr_Search(start_addr, &region, &region_end);
    
    /* If the number of regions exceeds the max there is an issue with the
       region pointer */
    if (region_num > ESAL_DP_MEM_Num_Regions)
    {
        /* Force error condition */
        region = NU_NULL;
    }

    /* Check if region found */
    if (region)
    {
        /* Check if the region containing the start address matches the
           requested attributes */
        if ((ESAL_DP_MEM_Region_Data[region_num].cache_type  != cache_type) ||
            (!(ESAL_DP_MEM_Region_Data[region_num].access_type & access_type)) ||
            (ESAL_DP_MEM_Region_Data[region_num].mem_type    != mem_type))
        {
            /* Region containing start address does not match the requested
               attributes.  Initialize the number of regions searched */
            total_regions_searched = 0;

            /* Loop through all the remaining regions to see
               if any memory regions match the requested attributes. */
            do
            {
                /* Move to the next memory region */
                region_num++;

                /* Check if the memory region number needs to wrap */
                if (region_num == ESAL_DP_MEM_Num_Regions)
                {
                    /* Wrap the region number to the first region */
                    region_num = 0;
                }

                /* Increment the total count of regions searched */
                total_regions_searched++;

            /* Loop until a memory region is found that matches all the requested
               criteria (cache type / memory type) or until all the
               memory regions have been searched */
            } while ( ((ESAL_DP_MEM_Region_Data[region_num].cache_type  != cache_type) ||
                       (!(ESAL_DP_MEM_Region_Data[region_num].access_type & access_type)) ||
                       (ESAL_DP_MEM_Region_Data[region_num].mem_type    != mem_type))  &&
                      (total_regions_searched < ESAL_DP_MEM_Num_Regions) );

            /* Check if a memory region was found that matches the requested criteria. */
            if (total_regions_searched != ESAL_DP_MEM_Num_Regions)
            {
                /* Set the return address to the start of the found memory region's
                   start address */
                start_addr = ESAL_DP_MEM_Region_Data[region_num].virtual_start_addr;
            }
            else
            {
                /* Set return value to show error - memory region was not
                   found containing the requested attributes */
                start_addr = (VOID *)ESAL_GE_MEM_ERROR;
            }

        }

    }
    else
    {
        /* Set return value to show error - memory region was not
           found containing the start address */
        start_addr = (VOID *)ESAL_GE_MEM_ERROR;
    }

    /* Return first uncached memory address to caller */
    return (start_addr);
}

