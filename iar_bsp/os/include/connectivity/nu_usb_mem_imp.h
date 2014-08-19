/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************/

/*************************************************************************
 *
 * FILE NAME
 *
 *        nu_usb_mem_imp.h
 *
 * COMPONENT
 *        OS Services : Memory
 *
 * DESCRIPTION
 *
 *        This file contains the structures and API declaration for
 *        memory services in OS services component of Nucleus USB software.
 *
 * DATA STRUCTURES
 *          USB_ALIGNED_MEM_POOL    Mem pool Control Block.
 *
 * FUNCTIONS
 *          None
 * DEPENDENCIES
 *          None
 *
 ************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_OS_MEM_H_
#define _NU_USB_OS_MEM_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ======================= #defines ================================== */
/* should not be more than 255, UINT8  */
#define MAX_ELEMENTS_PER_POOL 128

/* USB Memory pools specs. */
#define NU_USB_MIN_ALLOCATION       32
#define NU_USB_CACHED_POOL_SIZE     CFG_NU_OS_CONN_USB_COM_STACK_CACHED_POOL_SIZE
#define NU_USB_UNCACHED_POOL_SIZE   CFG_NU_OS_CONN_USB_COM_STACK_UNCACHED_POOL_SIZE

/* USB memory type definitions. */
#define USB_MEM_TYPE_CACHED     0
#define USB_MEM_TYPE_UNCACHED   1

/* External API definition for USB memory services. */
#define USB_Initialize_Memory_Pools     usb_initialize_memory_pools
#define USB_Uninitialize_Memory_Pools   usb_uninitialize_memory_pools

/* Following APIs appear only if separate memory pools are created
 * by USB stack for internal use. */
#if (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE)

#define USB_Allocate_Object             usb_allocate_object
#define USB_Allocate_Memory             usb_allocate_memory
#define USB_Allocate_Aligned_Memory     usb_allocate_aligned_memory
#define USB_Deallocate_Memory           usb_deallocate_memory

#else
/* If USB pools are not created then map USB memory APIs to
 * Nucleus memory APIs */

extern NU_MEMORY_POOL                  *USB_Cached_Memory_Pool_Ptr;
extern NU_MEMORY_POOL                  *USB_Uncached_Memory_Pool_Ptr;

#define USB_Allocate_Object(size,ptr_out)   NU_Allocate_Memory           \
                                            (USB_Cached_Memory_Pool_Ptr, \
                                            ptr_out,                     \
                                            size,                        \
                                            NU_NO_SUSPEND)

#define USB_Allocate_Memory(mem_type,size,ptr_out)                       \
        NU_Allocate_Memory                                               \
        ((mem_type == USB_MEM_TYPE_CACHED) ? USB_Cached_Memory_Pool_Ptr : USB_Uncached_Memory_Pool_Ptr, \
        ptr_out,                                                         \
        size,                                                            \
        NU_NO_SUSPEND)

#define USB_Allocate_Aligned_Memory(mem_type,size,alignment,ptr_out)     \
        NU_Allocate_Aligned_Memory                                       \
        ((mem_type == USB_MEM_TYPE_CACHED) ? USB_Cached_Memory_Pool_Ptr : USB_Uncached_Memory_Pool_Ptr, \
        ptr_out,                                                         \
        size,                                                            \
        alignment,                                                       \
        NU_NO_SUSPEND)

#define USB_Deallocate_Memory(mem_ptr)      NU_Deallocate_Memory(mem_ptr)

#endif /* (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE) */

/* usb_alloc_aligned_memory_pool is deprecated */
#define usb_alloc_aligned_memory_pool(a, b, c, d, e, f, g, h) \
        NU_Allocate_Aligned_Memory(a, c, (d*e), f, g); *b=*c

/* ====================  Function Prototypes ========================== */
VOID   usb_wait_ms (UINT32 msecs);
UINT16 endianswap16 (UINT16 x);
UINT32 endianswap32 (UINT32 x);

STATUS usb_initialize_memory_pools(NU_MEMORY_POOL*, NU_MEMORY_POOL*);
STATUS usb_uninitialize_memory_pools(VOID);
#if (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE)
STATUS usb_allocate_object(UINT32, VOID**);
STATUS usb_allocate_memory(UINT8, UINT32, VOID**);
STATUS usb_allocate_aligned_memory(UINT8, UINT32, UINT32, VOID**);
STATUS usb_deallocate_memory(VOID*);
#endif /* (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE) */

/* USB_ALIGNED_MEM_POOL is deprecated */
typedef struct
{                                           /* Aligned pools of Memory */
    CS_NODE list;
    UINT8 avail_elements;                   /* No. of unallocated elements in the pool */
    UINT8 bits[MAX_ELEMENTS_PER_POOL / 8];  /* bit map to manage availability status */
    UINT8 *actual_ptr;                      /* Ptr returned by NU_Allocate_Memory */
    UINT8 *aligned_ptr;                     /* actual_ptr aligned to alignment value */
}
USB_ALIGNED_MEM_POOL;

/* ==================================================================== */
#endif /*_NU_USB_OS_MEM_H_*/

/* ======================  End Of File  =============================== */

