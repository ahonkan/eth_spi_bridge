/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*
*       pc_memry.c
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*       System specific memory management routines.
*
* DATA STRUCTURES
*
*       finode_lock_handles                 FINODE lock list.
*       handles_are_alloced                 Memory allocate flag
*                                            (pc_memory_init use).
*       *inoroot                            Beginning of inode pool.
*       *mem_block_pool                     Memory block pool list.
*       *mem_file_pool                      Memory file pool list.
*       *mem_drobj_pool                     Memory DROBJ pool list.
*       *mem_drobj_freelist                 Memory DROBJ free list.
*       *mem_finode_pool                    Memory FINODE pool list.
*       *mem_finode_freelist                Memory FINODE free list.
*       NUFP_Events                         File system event group list.
*       NUF_FILE_SYSTEM_MUTEX               File system semaphore.
*
* FUNCTIONS
*
*       NUF_Alloc                           Allocate memory.
*       pc_memory_init                      This routine must be called
*                                            before any file system
*                                            routines.
*       pc_memory_drobj                     If called with a null
*                                            pointer, allocates and
*                                            zeros the space needed to
*                                            store a DROBJ structure.
*       pc_memory_finode                    If called with a null
*                                            pointer, allocates and
*                                            zeros the space needed to
*                                            store a FINODE structure.
*
*************************************************************************/

#include        "nucleus.h"
#include        "storage/fat_defs.h"
#include        "storage/fd_defs.h"


/* Things we need if using fine-grained multitasking */
#if (LOCK_METHOD == 2)
/* Semaphore handles used for reentrancy control on fats, drives, and finodes */
WAIT_HANDLE_TYPE        *finode_lock_handles;
#endif

/* List of users. The only user structure in single tasking environments  */
FINODE                  *inoroot = 0;             /* Beginning of inode pool */
BLKBUFF                 *mem_block_pool = 0;
PC_FILE                 *mem_file_pool = 0;
DROBJ                   *mem_drobj_pool = 0;
DROBJ                   *mem_drobj_freelist = 0;
FINODE                  *mem_finode_pool = 0;
FINODE                  *mem_finode_freelist = 0;

/*  The following is a definition which allows the Event IDs used
    by Nucleus PLUS.  */
NU_EVENT_GROUP          *NUFP_Events;

/* Pointer to lock array that needs to be allocated / deallocated */
extern UINT8 *available_locks;

/*  The following is the definition of the Semaphore used by Nucleus FILE. */
NU_SEMAPHORE            NUF_FILE_SYSTEM_MUTEX;
#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)
/* This lock prevents other task from calling this API until it is complete. */
NU_SEMAPHORE            NUF_CHKDSK_MUTEX;
#endif

extern UINT32 FILE_Unused_Param; /* Used to prevent compiler warnings */

/************************************************************************
* FUNCTION
*
*       pc_memory_init
*
* DESCRIPTION
*
*       This routine must be called before any file system routines.
*       Its job is to allocate tables needed by the file system. We
*       chose to implement memory management this way to provide maximum
*       flexibility for embedded system developers. In the reference
*       port we use malloc to allocate the various chunks of memory we
*       need, but we could just have easily compiled the tables into the
*       BSS section of the program.
*
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       YES on success or no ON Failure.
*
*************************************************************************/
INT pc_memory_init(VOID)
{
STATUS              sts;
INT16               i;
INT16               j;
volatile UNSIGNED   pool_size;
UINT16              event_id;
DROBJ               *pobj;
FINODE              *pfi;
OPTION              preempt_status;

    preempt_status = NU_Change_Preemption(NU_NO_PREEMPT);

    sts = NU_SUCCESS;

    /* Remove compiler warning. */
    FILE_Unused_Param = (UINT32)sts;

    /*  Initialize all of the Events.  We don't know how many events the
        user is going to define because he may or may not use the IDE,
        FLOPPY, or RAMDISK drivers. */
    NUFP_Events = NUF_Alloc(sizeof(NU_EVENT_GROUP) * gl_NUF_NUM_EVENTS);
    if (NUFP_Events != NU_NULL)
    {
        NUF_Memfill(NUFP_Events, (sizeof(NU_EVENT_GROUP) * gl_NUF_NUM_EVENTS), 0);
        for (event_id = 0; event_id < gl_NUF_NUM_EVENTS; event_id++)
        {

            /*  Create the Event Group. */
            if (NU_Create_Event_Group(&(NUFP_Events[event_id]), "EVENT") != NU_SUCCESS)
            {
                /* Restore the previous preemption state. */
                NU_Change_Preemption(preempt_status);
                return(NO);
            }
        }
    }
    else
    {
        goto meminit_failed;
    }

    /*  Initialize all of the Semaphores.  */
    if (NU_Create_Semaphore(&NUF_FILE_SYSTEM_MUTEX, "SEM 0", 1,
                                    NU_FIFO) != NU_SUCCESS)
    {
        /* Restore the previous preemption state. */
        NU_Change_Preemption(preempt_status);
        return(NO);
    }
    
#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)
    /* Check disk exclusive Semaphore. */
    if (NU_Create_Semaphore(&NUF_CHKDSK_MUTEX, "CHKSEM", 1, NU_FIFO) != NU_SUCCESS)
    {
        /* Restore the previous preemption state. */
        NU_Change_Preemption(preempt_status);
        return(NO);
    }
#endif

/* NUCLEUS. Event handles are simple indices under Nucleus */

    mem_block_pool      = NU_NULL;
    mem_file_pool       = NU_NULL;
    mem_drobj_pool      = NU_NULL;
    mem_drobj_freelist  = NU_NULL;
    mem_finode_pool     = NU_NULL;
    mem_finode_freelist = NU_NULL;

/* NUCLEUS - The current user is a macro not a constant */

    /* Allocate the available_locks array */
    available_locks = NUF_Alloc(sizeof(UINT8) * gl_NUF_NUM_EVENTS);
    if (available_locks != NU_NULL)
    {
        NUF_Memfill(available_locks, (sizeof(UINT8) * gl_NUF_NUM_EVENTS), 0);
    }
    else
    {
        available_locks = NU_NULL;
        goto meminit_failed;
    }

#if (LOCK_METHOD == 2)
    /* Allocate all message handles needed by RTFS. */
    finode_lock_handles = NUF_Alloc(sizeof(WAIT_HANDLE_TYPE) * gl_NFINODES);
    if (finode_lock_handles != NU_NULL)
    {
        NUF_Memfill(finode_lock_handles, (sizeof(WAIT_HANDLE_TYPE) * gl_NFINODES), 0);
        for (i = 0; i < gl_NFINODES; i++)
        {
               sts = pc_alloc_lock(&(finode_lock_handles[i]));
               if (sts != NU_SUCCESS)
               {
                   /* Restore the previous preemption state. */
                   NU_Change_Preemption(preempt_status);
                   pc_report_error(PCERR_INITALLOC);
                   return(NO);
               }
        }
    }
    else
    {
        goto meminit_failed;
    }
#endif  /* LOCK_METHOD == 2 */

    /* Allocate user file structures. Set each structure's is_free
       field to YES. The file structure allocator uses this field to
       determine if a file is available for use */
    pool_size = sizeof(PC_FILE);
    pool_size *= gl_VFS_MAX_OPEN_FILES;
    mem_file_pool = NUF_Alloc((INT)pool_size);

    if (!mem_file_pool)
    {
        mem_file_pool = NU_NULL;
        goto meminit_failed;
    }
    for (i = 0; i < gl_VFS_MAX_OPEN_FILES; i++)
        mem_file_pool[i].is_free = YES;

    /* Allocate block buffer pool and make a null terminated list
       linked with pnext. The block buffer pool code manages this list
       directly */
    pool_size = sizeof(BLKBUFF);
    pool_size *= CFG_NU_OS_STOR_FILE_FS_FAT_NUM_BLOCK_BUFFERS;
    mem_block_pool = NUF_Alloc((INT)pool_size);
    if (!mem_block_pool)
    {
        mem_block_pool = NU_NULL;
        goto meminit_failed;
    }

    for (i = 0, j = 1; i < (CFG_NU_OS_STOR_FILE_FS_FAT_NUM_BLOCK_BUFFERS-1); i++, j++)
    {
        NUF_Memfill(&mem_block_pool[i], sizeof(BLKBUFF), (UINT8) 0);
        mem_block_pool[i].pnext = mem_block_pool + j;
    }
    NUF_Memfill(&mem_block_pool[CFG_NU_OS_STOR_FILE_FS_FAT_NUM_BLOCK_BUFFERS-1], sizeof(BLKBUFF), (UINT8) 0);
    mem_block_pool[CFG_NU_OS_STOR_FILE_FS_FAT_NUM_BLOCK_BUFFERS-1].pnext = NU_NULL;

    /* Allocate DROBJ structures and make a NULL terminated freelist using
       pdrive as the link. This linked freelist structure is used by the
       DROBJ memory allocator routine. */
    pool_size = sizeof(DROBJ);
    pool_size *= gl_NDROBJS;
    mem_drobj_pool = NUF_Alloc((INT)pool_size);
    if (!mem_drobj_pool)
    {
        mem_drobj_pool = NU_NULL;
        goto meminit_failed;
    }
    mem_drobj_freelist = mem_drobj_pool;
    for (i = 0, j = 1; i < (gl_NDROBJS-1); i++, j++)
    {
        pobj = mem_drobj_freelist + j;
        mem_drobj_freelist[i].pdrive = (DDRIVE *) pobj;
    }
    mem_drobj_freelist[gl_NDROBJS-1].pdrive = (DDRIVE *) NU_NULL;

    /* Allocate FINODE structures and make a NULL terminated freelist using
       pnext as the link. This linked freelist is used by the FINODE
       memory allocator routine */
    pool_size = sizeof(FINODE);
    pool_size *= gl_NFINODES;
    mem_finode_pool = NUF_Alloc((INT)pool_size);
    if (!mem_finode_pool)
    {
        mem_finode_pool = NU_NULL;
        goto meminit_failed;
    }
    mem_finode_freelist = mem_finode_pool;

#if (LOCK_METHOD == 2)
    /* Copy lock handles into our new finode structures */
    for (i = 0,pfi = mem_finode_freelist; i < gl_NFINODES; i++, pfi++)
        pfi->lock_object.wait_handle = finode_lock_handles[i];
#endif  /* LOCK_METHOD == 2 */

    pfi = mem_finode_freelist = mem_finode_pool;
    for (i = 0; i < (gl_NFINODES-1); i++)
    {
        pfi++;
        mem_finode_freelist->pnext = pfi;
        mem_finode_freelist++;
        mem_finode_freelist->pnext = NU_NULL;
    }
    mem_finode_freelist = mem_finode_pool;

    /* Restore the previous preemption state. */
    NU_Change_Preemption(preempt_status);

    return(YES);

meminit_failed:
    /* Now deallocate all of our internal structures. */
    if(NUFP_Events)
        NU_Deallocate_Memory((VOID *)NUFP_Events);
    if (available_locks)
        NU_Deallocate_Memory((VOID *)available_locks);
#if (LOCK_METHOD == 2)
    if(finode_lock_handles)
        NU_Deallocate_Memory((VOID *)finode_lock_handles);
#endif
    if (mem_block_pool)
        NU_Deallocate_Memory((VOID *)mem_block_pool);
    if (mem_file_pool)
        NU_Deallocate_Memory((VOID *)mem_file_pool);
    if (mem_drobj_pool)
        NU_Deallocate_Memory((VOID *)mem_drobj_pool);
    if (mem_finode_pool)
        NU_Deallocate_Memory((VOID *)mem_finode_pool);

    /* Clear all pointer. */
    NUFP_Events = NU_NULL;
    available_locks = NU_NULL;
#if (LOCK_METHOD == 2)
    finode_lock_handles = NU_NULL;
#endif
    mem_block_pool = NU_NULL;
    mem_file_pool = NU_NULL;
    mem_drobj_pool = NU_NULL;
    mem_drobj_freelist = NU_NULL;
    mem_finode_pool = NU_NULL;
    mem_finode_freelist = NU_NULL;
/* NUCLEUS - The current user is a macro not a constant */

    NU_Change_Preemption(preempt_status);
    pc_report_error(PCERR_INITALLOC);
    return(NO);
}

/************************************************************************
* FUNCTION
*
*       pc_memory_close
*
* DESCRIPTION
*
*       Free all memory used by the file system and make it ready to run
*       again.
*
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
INT pc_memory_close(VOID)                                      /*__fn__*/
{
STATUS      ret_val = YES;
OPTION      preempt_status;
UINT16      event_id;


    /* Make sure we aren't interrupted. */
    preempt_status = NU_Change_Preemption(NU_NO_PREEMPT);
    

     /*  Delete all of the Events created in pc_memory_init. */
    for (event_id = 0; event_id < gl_NUF_NUM_EVENTS; event_id++)
    {

        /*  Delete the Event Group. */
        if (NU_Delete_Event_Group(&NUFP_Events[event_id]) != NU_SUCCESS)
        {
            /* Restore the previous preemption state. */
            NU_Change_Preemption(preempt_status);
            return(NO);
        }

    }

    /*  Remove semaphores needed by the filesystem.  */
    if (NU_Delete_Semaphore(&NUF_FILE_SYSTEM_MUTEX) != NU_SUCCESS)
    {
        /* Restore the previous preemption state. */
        NU_Change_Preemption(preempt_status);
        return(NO);
    }
    
#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)   
    /* Check disk Semaphore. */
    if (NU_Delete_Semaphore(&NUF_CHKDSK_MUTEX) != NU_SUCCESS)
    {
        /* Restore the previous preemption state. */
        NU_Change_Preemption(preempt_status);
        return(NO);
    }
#endif

    /* Now deallocate all of our internal structures */
    if (mem_block_pool)
        ret_val = NU_Deallocate_Memory((VOID *)mem_block_pool);
    
    if (mem_file_pool && ret_val == NU_SUCCESS)    
        ret_val = NU_Deallocate_Memory((VOID *)mem_file_pool);

    if (mem_drobj_pool && ret_val == NU_SUCCESS)
        ret_val = NU_Deallocate_Memory((VOID *)mem_drobj_pool);
    
    if (mem_finode_pool && ret_val == NU_SUCCESS)
        ret_val = NU_Deallocate_Memory((VOID *)mem_finode_pool);

    if(NUFP_Events && (ret_val == NU_SUCCESS))
        ret_val = NU_Deallocate_Memory((VOID *)NUFP_Events);

    if (available_locks && (ret_val == NU_SUCCESS))
        ret_val = NU_Deallocate_Memory((VOID *)available_locks);

#if (LOCK_METHOD == 2)
    if(finode_lock_handles && (ret_val == NU_SUCCESS))
        ret_val = NU_Deallocate_Memory((VOID *)finode_lock_handles);
#endif

    /* Clear a few values. This allows us to close down all memory used
       by the file system an then re-activate it by calling
       pc_memory_init. */    
    inoroot = NU_NULL;
    mem_block_pool      = NU_NULL;
    mem_file_pool       = NU_NULL;
    mem_drobj_pool      = NU_NULL;
    mem_drobj_freelist  = NU_NULL;
    mem_finode_pool     = NU_NULL;
    mem_finode_freelist = NU_NULL;
    NUFP_Events = NU_NULL;
    available_locks = NU_NULL;
#if (LOCK_METHOD == 2)
    finode_lock_handles = NU_NULL;
#endif
    
    if(ret_val == NU_SUCCESS)
        ret_val = YES;

    /* Set the preemption status back. */
    NU_Change_Preemption(preempt_status);

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*       pc_memory_drobj
*
* DESCRIPTION
*
*       If called with a null pointer, allocates and zeros the space
*       needed to store a DROBJ structure. If called with a NON-NULL
*       pointer, the DROBJ structure is returned to the heap.
*
*
* INPUTS
*
*       *pobj                               Drive object structure
*
* OUTPUTS
*
*       If an ALLOC, returns a valid pointer or NU_NULL if no more core.
*       If a free, the return value is the input.
*
*************************************************************************/
DROBJ *pc_memory_drobj(DROBJ *pobj)
{
DROBJ       *preturn;
DROBJ       *ret_val;

    if (pobj)
    {
        /* Free it by putting it at the head of the freelist
           NOTE: pdrive is used to link the freelist */
        /* Set the next DDRIVE pointer. */
        pobj->pdrive = (DDRIVE *) mem_drobj_freelist;

        /* Set DROBJ memory pool free list. */
        mem_drobj_freelist = pobj;

        ret_val = pobj;
    }
    else
    {
        /* Alloc: return the first structure from the freelist */

        /* Get DROBJ memory pool pointer. */
        preturn =  mem_drobj_freelist;
        if (preturn)
        {
            /* Move to the next DROBJ memory pool. */
            mem_drobj_freelist = (DROBJ *) preturn->pdrive;

            /* Initialize DROBJ. */
            NUF_Memfill(preturn, sizeof(DROBJ), (UINT8) 0);

            ret_val = preturn;
        }
        else
        {
            pc_report_error(PCERR_DROBJALLOC);
            ret_val = NU_NULL;
        }
    }

    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       pc_memory_finode
*
* DESCRIPTION
*
*       If called with a null pointer, allocates and zeros the space
*       needed to store a FINODE structure. If called with a NON-NULL
*       pointer, the FINODE structure is returned to the heap.
*
*
* INPUTS
*
*       *pinode                             If NULL is specified, it
*                                            allocates FINODE memory.
*                                            If valid pointer is input,
*                                            it is freed.
*
* OUTPUTS
*
*       If an ALLOC, returns a valid pointer or NU_NULL if no more core.
*       If a free, the return value is the input.
*
*************************************************************************/
FINODE *pc_memory_finode(FINODE *pinode)
{
FINODE      *preturn;
#if (LOCK_METHOD == 2)
WAIT_HANDLE_TYPE wait_handle;
#endif  /* LOCK_METHOD == 2 */
FINODE      *ret_val;

    if (pinode)
    {
        /* Free it by putting it at the head of the freelist */
        /* Set the next FINODE pointer. */
        pinode->pnext = mem_finode_freelist;

        /* Set FINODE memory pool free list. */
        mem_finode_freelist = pinode;
#ifdef DEBUG_FI
         DEBUG_PRINT ("Free FINODE 0x%08x \n",(unsigned int)pinode);
#endif
        ret_val = pinode;
    }
    else
    {
        /* Alloc: return the first structure from the freelist */

        /* Get FINODE memory pool pointer. */
        preturn =  mem_finode_freelist;
        if (preturn)
        {
            /* Move to the next FINODE memory pool. */
            mem_finode_freelist = preturn->pnext;
            /* Zero the structure. wait_handle can't be zeroed so
               push it and pop it after zeroing */
#if (LOCK_METHOD == 2)
            /* Evacuate the number of wait handle. */
            wait_handle = preturn->lock_object.wait_handle;
            /* Initialize FINODE. */
            NUF_Memfill(preturn, sizeof(FINODE), (UINT8) 0);
            /* Set the number of wait handle. */
            preturn->lock_object.wait_handle = wait_handle;

#else   /* LOCK_METHOD != 2 */

            /* Initialize FINODE. */
            NUF_Memfill(preturn, sizeof(FINODE), (UINT8) 0);

#endif  /* LOCK_METHOD == 2 */
#ifdef DEBUG_FI
         DEBUG_PRINT ("Alloc FINODE 0x%08x \n",(unsigned int)preturn);
#endif

            ret_val = preturn;
        }
        else
        {
            pc_report_error(PCERR_FINODEALLOC);

            ret_val = NU_NULL;
        }
    }

    return(ret_val);
}
