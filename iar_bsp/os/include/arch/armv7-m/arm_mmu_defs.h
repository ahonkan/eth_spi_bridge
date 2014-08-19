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
*       arm_mmu_defs.h
*
*   DESCRIPTION
*
*       This file contains all definitions, structures, etc for the
*       ARM MMU.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
***********************************************************************/

#ifndef     ARM_MMU_DEFS_H
#define     ARM_MMU_DEFS_H

/* Define core cache availability
   NOTE:  A differentiation is made in ESAL between cache that
          is contained on a processor and cache that is
          inherent as part of a core (L2 vs L1 cache). */
#define         ESAL_CO_CACHE_AVAILABLE                 NU_FALSE



/*********************************************
* Common cache operation macros
*********************************************/

#if (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)

/* This macro invalidates all of the cache at the core level. */
#define         ESAL_CO_MEM_CACHE_ALL_INVALIDATE()                          \
                {                                                           \
                                                                            \
                }

/* This macro invalidates all of the instruction cache at the core level. */
#define         ESAL_CO_MEM_ICACHE_ALL_INVALIDATE()                         \
                {                                                           \
                                                                            \
                }

/* This macro invalidates all of the data cache at the core level. */
#define         ESAL_CO_MEM_DCACHE_ALL_INVALIDATE()                         \
                {                                                           \
                                                                            \
                }

/* This macro invalidates all instruction cache for the specified address
   range at the core level. */
#define         ESAL_CO_MEM_ICACHE_INVALIDATE(addr, size)                   \
                {                                                           \
                                                                            \
                }

/* This macro invalidates all data cache for the specified address
   range at the core level. */
#define         ESAL_CO_MEM_DCACHE_INVALIDATE(addr, size)                   \
                {                                                           \
                                                                            \
                }

/* This macro flushes all data cache to physical memory (writeback cache)
   and invalidates all data cache entries at the core level. */
#define         ESAL_CO_MEM_DCACHE_ALL_FLUSH_INVAL()                        \
                {                                                           \
                                                                            \
                }

/* This macro flushes all data cache to physical memory (writeback cache)
   for the given address range, then invalidates all data cache entries
   at the core level. */
#define         ESAL_CO_MEM_DCACHE_FLUSH_INVAL(addr, size)                  \
                {                                                           \
                                                                            \
                }

#endif  /* (ESAL_CO_CACHE_AVAILABLE == NU_TRUE) */

#endif  /* ARM_MMU_DEFS_H */
