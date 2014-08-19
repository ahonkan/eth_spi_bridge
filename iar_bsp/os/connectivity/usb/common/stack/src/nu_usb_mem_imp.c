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
************************************************************************

 *************************************************************************
 *
 * FILE NAME
 *
 *        nu_usb_mem_imp.c
 *
 * COMPONENT
 *        OS Services: Memory
 *
 * DESCRIPTION
 *        This file contains the implementation of functions for  memory
 *        services in OS services component of Nucleus USB software.These
 *        services are used across the host, device and OTG USB stacks.
 *
 * DATA STRUCTURES
 *    None
 *
 * FUNCTIONS
 *  endianswap16                    -swaps endianness for 16 bits
 *  endianswap32                    -swaps endianness for 32 bits
 *
 * DEPENDENCIES
 *    nu_usb.h                      All USB definitions
 *
 ************************************************************************/
#ifndef USB_MEM_IMP_C
#define USB_MEM_IMP_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

#if (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE)
/* USB Memory pools Variables. */
static NU_MEMORY_POOL       USB_Cached_Memory_Pool;
static NU_MEMORY_POOL       USB_Uncached_Memory_Pool;
static VOID                *Cached_Memory_Pointer;
static VOID                *Uncached_Memory_Pointer;
static BOOLEAN              USB_Pools_Initialized = NU_FALSE;
#else
NU_MEMORY_POOL             *USB_Cached_Memory_Pool_Ptr;
NU_MEMORY_POOL             *USB_Uncached_Memory_Pool_Ptr;
#endif /* (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE) */

/************************************************************************
 *
 * FUNCTION
 *
 *      usb_wait_ms
 *
 * DESCRIPTION
 *
 *      Puts the calling task to sleep for specified no. of milliseconds.
 *      Minimum sleep time is usually 10ms.
 *
 * INPUTS
 *
 *      UINT32                              msecs No of milliseconds to
 *                                          sleep.
 *
 * OUTPUTS
 *
 *      None.
 *
 *************************************************************************/
VOID usb_wait_ms (UINT32 msecs)
{
    /* For less than a tick, sleep two ticks to ensure one full tick */
    if (msecs <= (1000 / NU_PLUS_Ticks_Per_Second))
    {
        NU_Sleep(2);
    }
    else
    {
        NU_Sleep(((NU_PLUS_Ticks_Per_Second * msecs) / 1000) + 1);
    }
}

/************************************************************************
 *
 * FUNCTION
 *
 *      endianswap16
 *
 * DESCRIPTION
 *
 *      This function swaps endianness of a 16 bit input
 *
 * INPUTS
 *
 *      x                                   16 bit input.
 *
 * OUTPUTS
 *
 *      UINT16                              endian swapped output.
 *
 *************************************************************************/
UINT16 endianswap16 (UINT16 x)
{
    UINT16 temp1 = 1;
    UINT8 *temp2 = (UINT8 *) &temp1;

    if (!*temp2)
    {
        x =(UINT16) ((((x) & 0xff00) >> 8) | (((x) & 0x00ff) << 8));
    }
    return x;
}

/************************************************************************
 *
 * FUNCTION
 *
 *      endianswap32
 *
 * DESCRIPTION
 *
 *      This function swaps endianness of a 32 bit input
 *
 * INPUTS
 *
 *      x                                   32 bit input.
 *
 * OUTPUTS
 *
 *      UINT16                              endian swapped output.
 *
 *************************************************************************/
UINT32 endianswap32 (UINT32 x)
{
    UINT32 temp1 = 1;
    UINT8 *temp2 = (UINT8 *) &temp1;

    if (!*temp2)
    {
        x = ((((x) & 0xff000000) >> 24) |
             (((x) & 0x00ff0000) >> 8) |
             (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24));
    }
    return x;
}

/************************************************************************
 *
 * FUNCTION
 *
 *      usb_initialize_memory_pools
 *
 * DESCRIPTION
 *
 *      This function initializes cached and uncached memory pools for
 *      USB subsystem.
 *
 * INPUTS
 *
 *      cached_pool                         Pointer to System cached
 *                                          memory pool.
 *      uncached_pool                       Pointer to system uncached
 *                                          memory pool.
 *
 * OUTPUTS
 *
 *      NU_USB_INVLD_ARG                    Any of input argument is
 *                                          invalid.
 *      NU_SUCCESS                          Successfult completion of
 *                                          service.
 *
 *************************************************************************/
STATUS usb_initialize_memory_pools( NU_MEMORY_POOL *cached_pool,
                                    NU_MEMORY_POOL *uncached_pool)
{
    STATUS  status = NU_SUCCESS;
#if (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE)
    UINT8   rollback;

    NU_USB_PTRCHK(cached_pool);
    NU_USB_PTRCHK(uncached_pool);

    /* Initialize local variables with a default value. */
    rollback    = 0;

    if( !USB_Pools_Initialized )
    {
        do
        {
            /* Allocate memory for cached USB memory pool. */
            status = NU_Allocate_Memory(cached_pool,
                                        &Cached_Memory_Pointer,
                                        NU_USB_CACHED_POOL_SIZE,
                                        NU_NO_SUSPEND);
            if (status != NU_SUCCESS)
            {
                rollback = 1;
                break;
            }

            /* Create cached memory pool for USB subsystem. */
            status = NU_Create_Memory_Pool (&USB_Cached_Memory_Pool,
                                            "C-USBP",
                                            Cached_Memory_Pointer,
                                            NU_USB_CACHED_POOL_SIZE,
                                            NU_USB_MIN_ALLOCATION,
                                            NU_FIFO);
            if (status != NU_SUCCESS)
            {
                rollback = 2;
                break;
            }

            /* Allocate memory for Uncached USB memory pool. */
            status = NU_Allocate_Memory(uncached_pool,
                                        &Uncached_Memory_Pointer,
                                        NU_USB_UNCACHED_POOL_SIZE,
                                        NU_NO_SUSPEND);
            if (status != NU_SUCCESS)
            {
                rollback = 3;
                break;
            }

            /* Create un-cached memory pool for USB subsystem. */
            status = NU_Create_Memory_Pool (&USB_Uncached_Memory_Pool,
                                            "UC-USBP",
                                            Uncached_Memory_Pointer,
                                            NU_USB_UNCACHED_POOL_SIZE,
                                            NU_USB_MIN_ALLOCATION,
                                            NU_FIFO);
            if (status != NU_SUCCESS)
            {
                rollback = 4;
                break;
            }
        }while(0);
    }

    /* Clean up in case error occurs. */
    switch (rollback)
    {
        case 4:
            NU_Deallocate_Memory((VOID *) Uncached_Memory_Pointer);
            Uncached_Memory_Pointer = NU_NULL;
        case 3:
            NU_Delete_Memory_Pool(&USB_Cached_Memory_Pool);
            memset(&USB_Cached_Memory_Pool, 0x00, sizeof(NU_MEMORY_POOL));
        case 2:
            NU_Deallocate_Memory((VOID *) Cached_Memory_Pointer);
            Cached_Memory_Pointer = NU_NULL;
        case 1:
            /* Print debug message. */
            NU_Printf_USB_Msg ("USB Pools not Created.\r\n");
            break;
        case 0:
            USB_Pools_Initialized = NU_TRUE;

            /* Print debug message. */
            NU_Printf_USB_Msg ("USB Memory Pools Created successfully.\r\n");
    }
#else
    USB_Uncached_Memory_Pool_Ptr = uncached_pool;
    USB_Cached_Memory_Pool_Ptr = cached_pool;
#endif /* (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE) */
    return ( status );
}

/************************************************************************
 *
 * FUNCTION
 *
 *      usb_uninitialize_memory_pools
 *
 * DESCRIPTION
 *
 *      This function uninitializes cached and uncached memory pools for
 *      USB subsystem.
 *
 * INPUTS
 *
 *      None.
 *
 * OUTPUTS
 *
 *      NU_USB_INVLD_MEM_POOL               USB memory pools are invalie
 *                                          / not initialized.
 *      NU_SUCCESS                          Successfult completion of
 *                                          service.
 *
 *************************************************************************/
STATUS usb_uninitialize_memory_pools()
{
    STATUS  status = NU_SUCCESS;
#if (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE)
    /* Initialize status variable with a default value. */
    status = NU_USB_INVLD_MEM_POOL;

    if( USB_Pools_Initialized )
    {
        /* Deallocate USB cached and uncached memory pools. */
        status = NU_Delete_Memory_Pool(&USB_Cached_Memory_Pool);
        NU_USB_ASSERT( status == NU_SUCCESS );

        status = NU_Delete_Memory_Pool(&USB_Uncached_Memory_Pool);
        NU_USB_ASSERT( status == NU_SUCCESS );

        /* Reset memory pools control blocks. */
        memset(&USB_Cached_Memory_Pool, 0x00, sizeof(NU_MEMORY_POOL));
        memset(&USB_Uncached_Memory_Pool, 0x00, sizeof(NU_MEMORY_POOL));

        /* Deallocate memory reserved for USB cached memory pool. */
        if ( Cached_Memory_Pointer )
        {
            status = NU_Deallocate_Memory(Cached_Memory_Pointer);
            NU_USB_ASSERT( status == NU_SUCCESS );

            Cached_Memory_Pointer = NU_NULL;
        }

        /* Deallocate memory reserved for USB uncached memory pool. */
        if ( Uncached_Memory_Pointer )
        {
            status = NU_Deallocate_Memory(Uncached_Memory_Pointer);
            NU_USB_ASSERT( status == NU_SUCCESS );

            Uncached_Memory_Pointer = NU_NULL;
        }

        /* Mark USB pools status as un-initialized. */
        USB_Pools_Initialized = NU_FALSE;
    }
#else
    USB_Uncached_Memory_Pool_Ptr = NU_NULL;
    USB_Cached_Memory_Pool_Ptr = NU_NULL;
#endif /* (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE) */
    return ( status );
}

#if (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE)
/************************************************************************
 *
 * FUNCTION
 *
 *      usb_allocate_object
 *
 * DESCRIPTION
 *
 *      This function allocates requested size of memory block from
 *      cached memory pool. Memory allocations for control blocks must
 *      be done using this service.
 *
 * INPUTS
 *
 *      size                                Size of memory block.
 *      ptr_out                             Output argument containing
 *                                          address of allocated memory
 *                                          block when function returns.
 *
 * OUTPUTS
 *
 *      NU_USB_INVLD_ARG                    Any of the input argument is
 *                                          invalid.
 *      NU_USB_INVLD_MEM_POOL               USB memory pools are invalie
 *                                          / not initialized.
 *      NU_SUCCESS                          Successfult completion of
 *                                          service.
 *
 *************************************************************************/
STATUS usb_allocate_object(UINT32 size, VOID **ptr_out)
{
    STATUS status;

    NU_USB_PTRCHK(ptr_out);

    /* Initialize status to a default value. */
    status      = NU_USB_INVLD_MEM_POOL;
    *ptr_out    = NU_NULL;

    /* Only allocate memory of pool is already created. */
    if ( USB_Pools_Initialized )
    {
        /* Objects / control blocks are always allocated memory
         * from cached memory.
         */
        status = NU_Allocate_Memory(&USB_Cached_Memory_Pool,
                                    ptr_out,
                                    size,
                                    NU_NO_SUSPEND);
    }

    return ( status );
}

/************************************************************************
 *
 * FUNCTION
 *
 *      usb_allocate_memory
 *
 * DESCRIPTION
 *
 *      This function allocates requested size of memory block from
 *      requested pool (cached / uncached).
 *
 * INPUTS
 *
 *      mem_type                            Cached or Uncached memory.
 *                                          1. For cached memory caller
 *                                             must pass
 *                                             USB_MEM_TYPE_CACHED.
 *                                          2. For uncached memory caller
 *                                             must pass
 *                                             USB_MEM_TYPE_UNCACHED.
 *      size                                Size of memory block.
 *      ptr_out                             Output argument containing
 *                                          address of allocated memory
 *                                          block when function returns.
 *
 * OUTPUTS
 *
 *      NU_USB_INVLD_ARG                    Any of the input argument is
 *                                          invalid.
 *      NU_USB_INVLD_MEM_POOL               USB memory pools are invalie
 *                                          / not initialized.
 *      NU_SUCCESS                          Successfult completion of
 *                                          service.
 *
 *************************************************************************/
STATUS usb_allocate_memory( UINT8   mem_type,
                            UINT32  size,
                            VOID    **ptr_out)
{
    STATUS          status;
    NU_MEMORY_POOL  *tmp_pool;

    NU_USB_PTRCHK(ptr_out);

    /* Initialize status to a default value. */
    status      = NU_USB_INVLD_MEM_POOL;
    tmp_pool    = NU_NULL;

    /* Only allocate memory of pool is already created. */
    if ( USB_Pools_Initialized )
    {
        /* Get appropriate memory pool for requested memory block. */
        switch(mem_type)
        {
            case USB_MEM_TYPE_CACHED:
                tmp_pool = &USB_Cached_Memory_Pool;
                break;
            case USB_MEM_TYPE_UNCACHED:
                tmp_pool = &USB_Uncached_Memory_Pool;
                break;
            default:
                tmp_pool    = NU_NULL;
                status      = NU_USB_INVLD_ARG;
                break;
        }

        if ( tmp_pool != NU_NULL )
        {
            /* Allocate requested size of memory. */
            status = NU_Allocate_Memory(tmp_pool,
                                        ptr_out,
                                        size,
                                        NU_NO_SUSPEND);
        }
    }

    return ( status );
}

/************************************************************************
 *
 * FUNCTION
 *
 *      usb_allocate_aligned_memory
 *
 * DESCRIPTION
 *
 *      This function allocates requested size of aligned memory block
 *      from requested pool (cached / uncached).
 *
 * INPUTS
 *
 *      mem_type                            Cached or Uncached memory.
 *                                          1. For cached memory caller
 *                                             must pass
 *                                             USB_MEM_TYPE_CACHED.
 *                                          2. For uncached memory caller
 *                                             must pass
 *                                             USB_MEM_TYPE_UNCACHED.
 *      size                                Size of memory block.
 *      alignment                           Requested alignment of
 *                                          memory.
 *      ptr_out                             Output argument containing
 *                                          address of allocated memory
 *                                          block when function returns.
 *
 * OUTPUTS
 *
 *      NU_USB_INVLD_ARG                    Any of the input argument is
 *                                          invalid.
 *      NU_USB_INVLD_MEM_POOL               USB memory pools are invalie
 *                                          / not initialized.
 *      NU_SUCCESS                          Successfult completion of
 *                                          service.
 *
 *************************************************************************/
STATUS usb_allocate_aligned_memory( UINT8   mem_type,
                                    UINT32  size,
                                    UINT32  alignment,
                                    VOID    **ptr_out)
{
    STATUS          status;
    NU_MEMORY_POOL  *tmp_pool;

    NU_USB_PTRCHK(ptr_out);

    /* Initialize status to a default value. */
    status      = NU_USB_INVLD_MEM_POOL;
    tmp_pool    = NU_NULL;

    /* Only allocate memory of pool is already created. */
    if ( USB_Pools_Initialized )
    {
        /* Get appropriate memory pool for requested memory block. */
        switch(mem_type)
        {
            case USB_MEM_TYPE_CACHED:
                tmp_pool = &USB_Cached_Memory_Pool;
                break;
            case USB_MEM_TYPE_UNCACHED:
                tmp_pool = &USB_Uncached_Memory_Pool;
                break;
            default:
                tmp_pool    = NU_NULL;
                status      = NU_USB_INVLD_ARG;
                break;
        }

        if ( tmp_pool != NU_NULL )
        {
            /* Allocate requested size of aligned memory. */
            status = NU_Allocate_Aligned_Memory(tmp_pool,
                                                ptr_out,
                                                size,
                                                alignment,
                                                NU_NO_SUSPEND);
        }
    }

    return ( status );
}

/************************************************************************
 *
 * FUNCTION
 *
 *      usb_deallocate_memory
 *
 * DESCRIPTION
 *
 *      This function deallocates a previously allocated memory.
 *
 * INPUTS
 *
 *      mem_ptr                             Pointer containing address
 *                                          of memory to be de-allocated.
 *
 * OUTPUTS
 *
 *      NU_USB_INVLD_ARG                    Any of the input argument is
 *                                          invalid.
 *      NU_USB_INVLD_MEM_POOL               USB memory pools are invalie
 *                                          / not initialized.
 *      NU_SUCCESS                          Successfult completion of
 *                                          service.
 *
 *************************************************************************/
STATUS usb_deallocate_memory(VOID *mem_ptr)
{
    STATUS status;

    NU_USB_PTRCHK(mem_ptr);

    /* Initialize status to a default value. */
    status      = NU_USB_INVLD_MEM_POOL;

    /* Only de-allocate memory of pool is already created. */
    if ( USB_Pools_Initialized )
    {
        status = NU_Deallocate_Memory(mem_ptr);
    }

    return ( status );
}

#endif /* (CFG_NU_OS_CONN_USB_COM_STACK_CREATE_MEM_POOLS == NU_TRUE) */

/*************************************************************************/

#endif /* USB_MEM_IMP_C */
/*************************** end of file ********************************/

