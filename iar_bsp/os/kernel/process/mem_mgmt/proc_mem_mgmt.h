/*************************************************************************
*
*             Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************

************************************************************************
*
* FILE NAME
*
*       proc_mem_mgmt.h
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       Internal prototypes, definitions, and data type for the memory
*       management component of processes
*
* DATA STRUCTURES
*
*       None
*
************************************************************************/
#ifndef PROC_MEM_MGMT_H
#define PROC_MEM_MGMT_H

/* Memory region control block identifier is 'MEMR' */
#define PROC_MEMORY_ID              0x4D454D52UL

/* Alternative internal memory region attributes */
#define PROC_MEM_MAPPED             (1 << 31)
#define PROC_MEM_ALLOCATED          (1 << 30)
#define PROC_MEM_SHARED             (1 << 29)
#define PROC_NO_EXECUTE             (1 << 28)

/* Determines if regions created by the system during,
   kernel initialization and process load, will enforce
   execution restrictions on data. */
#if (CFG_NU_OS_KERN_PROCESS_MEM_MGMT_NO_EXECUTE == NU_TRUE)
#define PROC_NO_EXECUTE_LOADED      PROC_NO_EXECUTE
#else
#define PROC_NO_EXECUTE_LOADED      0
#endif

#define PROC_VALID_MAP_OPTIONS      (NU_MEM_READ | NU_MEM_WRITE | NU_MEM_EXEC | NU_SHARE_READ | NU_SHARE_WRITE | NU_SHARE_EXEC | NU_CACHE_INHIBIT | NU_CACHE_WRITE_THROUGH | NU_CACHE_NO_COHERENT)

#if !defined(PROC_ARCH_MEM_REGIONS_OVERRIDE) || (PROC_ARCH_MEM_REGIONS_OVERRIDE == 0)

/* Default number of kernel regions text, rodata, data/bss, RTL, trampoline */
#define PROC_KERNEL_REGIONS         4

#else

/* Use architecture over-ride for number of kernel regions */
#define PROC_KERNEL_REGIONS         PROC_ARCH_MEM_REGIONS_OVERRIDE

#endif

/* Service routine completion status constants for Memory Region support */
struct PROC_MEMORY_CB
{
    CS_NODE         created;                /* List of memory regions */
    UNSIGNED        valid;                  /* Valid control block ID code */
    CHAR            name[NU_MAX_NAME];      /* Memory region name */
    UNSIGNED        size;                   /* Size of the region */
    UNSIGNED        attributes;             /* Access, cache, share */
    VOID           *phys_base;              /* Physical base address */
    VOID           *virt_base;              /* Virtual base address, in most cases
                                               this will map to the physical address.
                                               This address only comes into play where
                                               and entire address space MUST be mapped
                                               virutally.  In this case the mem table
                                               for the BSP will dictate the mapping */

    /* Memory region may be on a list of process-owned region peers */
    struct PROC_CB_STRUCT
                   *owner;                  /* Process owning this memory region */
    CS_NODE         process_region_list;    /* Node for process */

    /* If this is a shared region */
    struct PROC_MEMORY_CB
                   *parent;                 /* region that this is based from */

    /* If this is a parent region */
    UNSIGNED        child_count;            /* Number of children regions currently
                                               sharing this region */
};

typedef struct PROC_MEMORY_CB  PROC_MEMORY;

/* Service routine completion status constants for Memory Region support */
struct PROC_KERN_MEM_CB
{
    CHAR            name[NU_MAX_NAME];
    UNSIGNED        attributes;
    VOID           *start;
    VOID           *end;
};

typedef struct PROC_KERN_MEM_CB  PROC_KERN_MEM;

/* PROC_EXPAND_SIZE is used to expand the size of a region
   that begins on a page boundary to be integrally divisible
   by the minimum page size. */
#define PROC_EXPAND_SIZE(size)  (((((UNSIGNED)(size)) + (PROC_PAGE_SIZE) - 1)/ \
                                 (PROC_PAGE_SIZE)) * (PROC_PAGE_SIZE))

STATUS PROC_Obtain_New_Memory_CB(PROC_CB *process, PROC_MEMORY **region);
STATUS PROC_Attach_Memory(PROC_CB *process, CHAR *name,
                          UNSIGNED attributes, UNSIGNED size,
                          VOID *addr, PROC_MEMORY **new_region);
STATUS PROC_Share_Memory(PROC_CB *src_proc, PROC_CB *tgt_proc);
STATUS PROC_Create_Process_Memory(PROC_CB *process);
STATUS PROC_Delete_Process_Memory(PROC_CB *process);
STATUS PROC_Unshare_Memory(PROC_CB* src_proc, PROC_CB* tgt_proc);
STATUS PROC_AR_Attach_Memory(PROC_CB* process, PROC_MEMORY* region);
STATUS PROC_AR_Detach_Memory(PROC_MEMORY* region);
STATUS PROC_AR_Mem_Mgmt_Cleanup(PROC_CB *process);
STATUS PROC_Attach_Share(PROC_CB* proc, PROC_MEMORY *region, CHAR *name, PROC_MEMORY **new_region);

#endif /* PROC_MEM_MGMT_H */
