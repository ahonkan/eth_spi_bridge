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
*       mount.c
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*       Handles FAT file system initialization and mounting
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       fat_init
*       fat_mount
*
*************************************************************************/
#include        "storage/fat_defs.h"
#include        "storage/fd_defs.h"

/* Globals */
UINT16 gl_NFINODES;
UINT16 gl_NDROBJS;
UINT16 gl_NHANDLES;
UINT16 gl_NUF_NUM_EVENTS;

static INT8 FAT_Is_Initialized = NU_FALSE;
INT8   FAT_Total_Mounted = 0;
extern UINT32 FILE_Unused_Param; /* Used to prevent compiler warnings */

/************************************************************************
* FUNCTION
*
*       fat_init
*
* DESCRIPTION
*
*       Perform FAT system initialization.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS                          Mount successful.
*
*************************************************************************/
STATUS fat_init(VOID)
{
STATUS      sts;

    sts = NU_SUCCESS;

    /* These used to be defines, but now that they are globals they
     * have to be set within the initialization. */
    /* Directory Object Needs. Conservative guess is One CWD per user
     * per drive + One per file + one per User for directory traversal
     */
    gl_NFINODES = (CFG_NU_OS_STOR_FILE_VFS_NUM_USERS * CFG_NU_OS_STOR_FILE_FS_FAT_MAX_DRVS +
                          CFG_NU_OS_STOR_FILE_VFS_NUM_USERS + gl_VFS_MAX_OPEN_FILES);
    gl_NDROBJS  = (CFG_NU_OS_STOR_FILE_VFS_NUM_USERS * CFG_NU_OS_STOR_FILE_FS_FAT_MAX_DRVS +
                          CFG_NU_OS_STOR_FILE_VFS_NUM_USERS + gl_VFS_MAX_OPEN_FILES);

    /* This is how many event handles we will allocate at startup. */
    gl_NHANDLES = (CFG_NU_OS_STOR_FILE_FS_FAT_MAX_DRVS*3) + gl_NFINODES;

    gl_NUF_NUM_EVENTS = (CFG_NU_OS_STOR_FILE_FS_FAT_MAX_DRVS * 3) + gl_NFINODES + 3;


    /* Check for one time only initialization */
    if (FAT_Is_Initialized == NU_FALSE)
    {
        /* Perform FAT fs init for entire system */
        if ( pc_memory_init() == YES)
            sts = NU_SUCCESS;
        else
            sts = NU_UNAVAILABLE;

        /* Update FAT initialization status */
        if (sts == NU_SUCCESS)
            FAT_Is_Initialized = NU_TRUE;
        else
            FAT_Is_Initialized = NU_FALSE;

    }

    return (sts);
}

/************************************************************************
* FUNCTION
*
*       fat_uninit
*
* DESCRIPTION
*
*       Frees the resources allocated by fat_init.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS                          Resources freed.
*
*************************************************************************/
STATUS fat_uninit(VOID)
{
STATUS      sts;

    sts = NU_SUCCESS;

    /* Check for one time only initialization */
    if (FAT_Is_Initialized == NU_TRUE)
    {
        /* Perform FAT fs init for entire system */
        if ( pc_memory_close() == YES)
            sts = NU_SUCCESS;
        else
            sts = NU_UNAVAILABLE;

        /* Update FAT initialization status */
        if (sts == NU_SUCCESS)
            FAT_Is_Initialized = NU_FALSE;
        else
            FAT_Is_Initialized = NU_TRUE;

    }

    return (sts);
}

/************************************************************************
* FUNCTION
*
*       fat_mount
*
* DESCRIPTION
*
*       Performs initialization of a specific FAT file system instance.
*
* INPUTS
*
*       dh                                  Disk handle
*
* OUTPUTS
*
*       NU_SUCCESS                          Mount successful.
*       NU_NO_MEMORY                        Memory allocation failed.
*       NU_UNAVAILABLE                      Max FAT drive count reached
*
*************************************************************************/
STATUS fat_mount (UINT16 dh, VOID *config)
{
STATUS      sts;
INT         open_needed = NU_TRUE;
FAT_CB      *fs_cb = NU_NULL;
INT8        allocated_locks = 0;

    /* Must be last line in declarations */
    PC_FS_ENTER()
          
    sts = NU_SUCCESS;
    /* Remove compiler warning. */
    FILE_Unused_Param = (UINT32)config;
    
    if (FAT_Is_Initialized == NU_TRUE)
    {
        /* Determine if the disk is already opened */
        sts = fsdh_get_fs_specific(dh, (VOID **)&fs_cb);
        if ((sts == NU_SUCCESS) && (fs_cb))
        {
            /* Verify we have a control block */
            if (fs_cb->ddrive)
            {
                /* If already openend, no need to reopen */
                if (fs_cb->ddrive->opencount > 0)
                {
                    open_needed = NU_FALSE;
                }
            }
        }

        /* Allocate memory for the FS control block */
        if ((sts == NU_SUCCESS) && (open_needed == NU_TRUE))
        {
            /* Verify enough resources will be available for mounting */
            if (FAT_Total_Mounted == CFG_NU_OS_STOR_FILE_FS_FAT_MAX_DRVS)
            {
                sts = NU_UNAVAILABLE;
            }
            else
            {
                fs_cb = NUF_Alloc((INT)sizeof(FAT_CB));
                if (fs_cb)
                {
                    /* Zero the structure */
                    NUF_Memfill(fs_cb, sizeof(FAT_CB), (UINT8) 0);

                    /* Assign it to the FS specific disk handle */
                    sts = fsdh_set_fs_specific(dh, (UNSIGNED*)fs_cb);
                }
                else
                    sts = NU_NO_MEMORY;
            }
        }

        /* Allocate memory for the FS control block */
        if ((sts == NU_SUCCESS) && (open_needed == NU_TRUE))
        {
            /* Allocate Drive memory */
            fs_cb->ddrive = (DDRIVE *)NUF_Alloc(sizeof(DDRIVE));
            if (!fs_cb->ddrive)
            {
                sts = NUF_NO_MEMORY;
            }
            else
            {
                /* Zero the structure so all of our initial values are right */
                NUF_Memfill(fs_cb->ddrive, sizeof(DDRIVE), (UINT8) 0);
            }
        }

        if ((sts == NU_SUCCESS) && (open_needed == NU_TRUE))
        {
            /* Allocate locks for this drive */
            sts = pc_alloc_lock(&(fs_cb->drive_lock.wait_handle));

            if (sts == NU_SUCCESS)
            {
                fs_cb->drive_lock.opencount = 0;
                fs_cb->drive_lock.exclusive = NO;
                fs_cb->drive_lock.dh = dh;
                ++allocated_locks;

                sts = pc_alloc_lock(&(fs_cb->drive_io_lock.wait_handle));
                if (sts == NU_SUCCESS)
                {
                    fs_cb->drive_io_lock.opencount = 0;
                    fs_cb->drive_io_lock.exclusive = NO;
                    fs_cb->drive_io_lock.dh = dh;
                    ++allocated_locks;

                    sts = pc_alloc_lock(&(fs_cb->fat_lock.wait_handle));
                    if (sts == NU_SUCCESS)
                    {
                        fs_cb->fat_lock.opencount = 0;
                        fs_cb->fat_lock.exclusive = NO;
                        fs_cb->fat_lock.dh = dh;
                        ++allocated_locks;
                    }
                }
            }
        }
    }

    /* Perform disk specific initialization */
    if (sts == NU_SUCCESS)
        sts = pc_dskinit(dh);

    if (sts == NU_SUCCESS)
    {    
        FAT_Total_Mounted++;  /* Increment the total current # of mounted FAT devices */
    }
    else 
    {        
        /* Deallocate locks. */
        while(allocated_locks > 0)
        {
            if(allocated_locks == 1)
            {
                pc_dealloc_lock(fs_cb->drive_lock.wait_handle);                
            }
			if(allocated_locks == 2)
			{            
                pc_dealloc_lock(fs_cb->drive_io_lock.wait_handle);            
            } 
			if(allocated_locks == 3) 
			{
                pc_dealloc_lock(fs_cb->fat_lock.wait_handle);
            }
            --allocated_locks;                

        }
   
        /* Return the allocated memory */
        if (fs_cb)
        {
            if (fs_cb->ddrive) 
            {
                (VOID)NU_Deallocate_Memory(fs_cb->ddrive);
            }

            (VOID)NU_Deallocate_Memory((VOID*)fs_cb);
        }

        /* Set dh fs specific information to NULL. */
        fsdh_set_fs_specific(dh, (UNSIGNED*)NU_NULL);
    }

    /* Release the lock */
    PC_FS_EXIT()

    return (sts);
}
