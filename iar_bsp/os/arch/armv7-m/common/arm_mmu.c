/***********************************************************************
*
*             Copyright 2010 Mentor Graphics Corporation
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
*       arm_mmu.c
*
*   DESCRIPTION
*
*       This file contains the ARM architecture MMU functions
*
*   FUNCTIONS
*
*       ESAL_CO_MEM_Cache_Enable            Enable cache for the
*                                           given core
*       ESAL_CO_MEM_Region_Setup            Sets-up a region of memory
*                                           based on specified attributes
*
*   DEPENDENCIES
*
*       nucleus.h
*
***********************************************************************/

/* Include required header files */
#include            "nucleus.h"

/* Local Function prototypes */
static VOID    ESAL_CO_MEM_Region_Setup(INT region_num,
                                        UINT32 vrt_addr,
                                        UINT32 phy_addr,
                                        UINT32 size,
                                        ESAL_GE_CACHE_TYPE cache_type,
                                        UINT32 access_type);


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_CO_MEM_Cache_Enable
*
*   DESCRIPTION
*
*       This function initializes the cache as required
*       for the given core.  The memory region data structure
*       (ESAL_DP_MEM_Region_Data) should be utilized to perform
*       this initialization and the cache attributes in this
*       table should be correctly reflected.
*
*   CALLED BY
*
*       ESAL_GE_MEM_Initialize
*
*   CALLS
*
*       ESAL_CO_MEM_Region_Setup
*
*   INPUTS
*
*       avail_mem                           Address of available memory
*
*   OUTPUTS
*
*       VOID *                              Updated available memory
*                                           address
*
***********************************************************************/
VOID    *ESAL_CO_MEM_Cache_Enable(VOID *avail_mem)
{
    /* Call region set-up function to avoid toolset warnings */
    ESAL_CO_MEM_Region_Setup(0, 0, 0, 0, ESAL_NOCACHE, 0);

    /* No cache available - just return available memory */
    return (avail_mem);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_CO_MEM_Region_Setup
*
*   DESCRIPTION
*
*       This function sets-up the region of memory based on the given
*       attributes
*
*   CALLED BY
*
*       ESAL_CO_MEM_Cache_Enable
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       region_num                          Number of region begin setup
*       vrt_addr                            Virtual address of region
*       phy_addr                            Physical address of region
*       size                                Size of region
*       cache_type                          Cache type of region
*       access_type                         Memory access type of region
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID    ESAL_CO_MEM_Region_Setup(INT region_num,
                                        UINT32 vrt_addr,
                                        UINT32 phy_addr,
                                        UINT32 size,
                                        ESAL_GE_CACHE_TYPE cache_type,
                                        UINT32 access_type)
{
     /* Reference unused parameters to prevent toolset warnings */
     NU_UNUSED_PARAM(region_num);
     NU_UNUSED_PARAM(vrt_addr);
     NU_UNUSED_PARAM(phy_addr);
     NU_UNUSED_PARAM(size);
     NU_UNUSED_PARAM(cache_type);
     NU_UNUSED_PARAM(access_type);
}
