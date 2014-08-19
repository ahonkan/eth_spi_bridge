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
*       fs_users.c
*
* COMPONENT
*
*       User
*
* DESCRIPTION
*
*       Contains API and services for managing the file system users.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Become_File_User                 Register a task as an FILE
*                                            user.
*       NU_Release_File_User                Register task as no longer
*                                            an RTFS user.
*       NU_Check_File_User                  Check if task is a
*                                            registered RTFS user.
*       fs_current_user_structure           Current task's
*                                            FILE_SYSTEM_USER structure
*                                            pointer.
*       pc_free_all_users                   Free all cwd objects for a
*                                            drive.
*
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/dh_extr.h"
#include "storage/vnode_defs.h"
#include "storage/fsl_extr.h"
#include "storage/user_extr.h"
#include "storage/lck_extr.h"
#include "storage/error_extr.h"
#include "storage/util_extr.h"

PFILE_SYSTEM_USER    user_heap = 0; /* File system user structure pointer. */

/* Task_Map correlates nufp_task_pointer and user_heap; index zero is the default user */
INT16                Task_Map[VFS_NUM_USERS_WDU];

/*  The following table maintains a mapping between Nucleus PLUS
 * pointers and Task IDs. Default user is not mapped in this array,
 * so the max size is CFG_NU_OS_STOR_FILE_VFS_NUM_USERS.
 */
NU_TASK         *nufp_task_pointer[CFG_NU_OS_STOR_FILE_VFS_NUM_USERS];

/* Implement these macros for your kernel. */
/* Return CONTEXT_HANDLE_TYPE that uniquely represents the current task.
   May not be zero. */
/* NUCLEUS - We set GET_CONTEXT_HANDLE() to (NU_Current_Task_ID()+1) so
   we are sure not to get a zero.  If we are using Nucleus PLUS, the task
   ID must be converted to a task pointer.  NUFP_Current_Task_ID performs
   this conversion. */

#define GET_CONTEXT_HANDLE() (fsu_get_context_handle())


/* Put the (UINT16) X into the task control block of the current task */

/* NUCLEUS - We use an array of ints to map nucleus tasks to RTFS user
   structures. This is done because RTFS user structures use a lot of
   core (200 or so bytes). And it is not necessary to have one RTFS user
   structure per nucleus task. We only need one structure per file
   system user. When a nucleus task calls NU_Become_File_User() it will
   get a map index in this table. GET_RTFS_TASKNO() and SET_RTFS_TASKNO()
   are macros described below */

/*  Nucleus PLUS uses task pointers not IDs.  The call to NUFP_Current_Task_ID
    makes the appropriate conversion.  */
#define SET_RTFS_TASKNO(X) fsu_set_rtfs_taskno(X)

/* Return the UINT16 assigned to this task by SET_RTFS_TASK_NO() If
   SET_RTFS_TASK_NO() was never called, this routine may return a random value*/
#define GET_RTFS_TASKNO()  fsu_get_rtfs_taskno()

   
/************************************************************************
* FUNCTION
*
*       fsu_get_context_handle
*
* DESCRIPTION
*
*       Returns the context_handle for the current task. 
*
* INPUTS
*
*       NONE
*
* OUTPUTS
*
*       -1                      task does not have a context_handle
*       != (-1)                 context_handle
*
*************************************************************************/
INT fsu_get_context_handle()
{
INT task_id;
INT context_handle;

    task_id = NUFP_Current_Task_ID();
    if (task_id >= 0)
        context_handle = task_id +1;
    else
        context_handle = task_id;

    return (context_handle);
}
/************************************************************************
* FUNCTION
*
*       fsu_set_rtfs_taskno
*
* DESCRIPTION
*
*       Sets the index into user_heap for the current task.
*       Index zero is the default user, and will never be assigned
*       a task_id value.
*
* INPUTS
*
*       rtfs_taskno             rtfs_taskno value to set
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
VOID fsu_set_rtfs_taskno(INT rtfs_taskno)
{
    INT task_id;

    task_id = NUFP_Current_Task_ID();

    if ((task_id >= 0) && (task_id < CFG_NU_OS_STOR_FILE_VFS_NUM_USERS))
    {
        /* Since the default user exists at index zero, increment the index by one */
        Task_Map[task_id + 1] = (INT16)rtfs_taskno;
    }
}

/************************************************************************
* FUNCTION
*
*       fsu_get_rtfs_taskno
*
* DESCRIPTION
*
*       Gets the index into user_heap for the current task.
*       Index zero is the default user, which is returned when
*       NUFP_Current_Task_ID cannot locate the current task.
*
* INPUTS
*
*       NONE
*
* OUTPUTS
*
*       rtfs_taskno             rtfs_taskno value for current task
*       						 or 0 for default user
*
*************************************************************************/
INT fsu_get_rtfs_taskno()
{
    INT task_id;
    INT rtfs_taskno;

    task_id = NUFP_Current_Task_ID();
	if ((task_id >= 0) && (task_id < CFG_NU_OS_STOR_FILE_VFS_NUM_USERS))
    {
        /* Since the default user exists at index zero, increment the index by one */
        rtfs_taskno = Task_Map[task_id + 1];
    }
    else
    {
        rtfs_taskno = 0; /* Default user index into user_heap */
    }

    return rtfs_taskno;
}

/************************************************************************
* FUNCTION
*
*       fsu_map_task_id
*
* DESCRIPTION
*
*       Returns the task_id for the given task pointer. Returns
*       -1 if the entry is not mapped. Default user is not mapped
*       in this array, so the max size is CFG_NU_OS_STOR_FILE_VFS_NUM_USERS.
*
* INPUTS
*
*       task_ptr                NU_TASK* of the task to be mapped
*
* OUTPUTS
*
*       -1                      NU_TASK* was not found
*       != (-1)                 task_id of NU_TASK* mapped entry
*
*************************************************************************/
INT fsu_map_task_id(NU_TASK *task_ptr)
{
STATUS done = 0;
INT task_id;

    /*  Search for the Task Pointer.  */
    for (task_id = 0; task_id < CFG_NU_OS_STOR_FILE_VFS_NUM_USERS; task_id++)
    {
        /*  If we found the entry, then return the ID.  */
        if (nufp_task_pointer[task_id] == task_ptr)
        {
            done = 1;
            break;
        }
    }

    if (!done)
        task_id = -1;

    return (task_id);
}

/************************************************************************
* FUNCTION
*
*       fsu_deallocate_task_id
*
* DESCRIPTION
*
*       Removes a task's entry in the nufp_task_pointer[] array. The task
*       must already have an entry mapped. Returns the task_id of the 
*       deallocated entry. Returns -1 if the task was not found.
*       Default user is not mapped in this array, so the max size
*       is CFG_NU_OS_STOR_FILE_VFS_NUM_USERS.
*
* INPUTS
*
*       task_ptr                NU_TASK* of the task to be removed
*
* OUTPUTS
*
*       -1                      NU_TASK* was not found
*       != (-1)                 task_id of NU_TASK* entry removed
*
*************************************************************************/
INT fsu_deallocate_task_id(NU_TASK *task_ptr)
{
INT task_id;

    task_id = fsu_map_task_id(task_ptr);

	if ((task_id >= 0) && (task_id < CFG_NU_OS_STOR_FILE_VFS_NUM_USERS))
    {
        nufp_task_pointer[task_id] = NU_NULL;
    }

    return (task_id);
}

/************************************************************************
* FUNCTION
*
*       fsu_allocate_task_id
*
* DESCRIPTION
*
*       Allocate a task entry in the nufp_task_pointer[] array. The task
*       must not be mapped. An entry must be available. Returns the 
*       task_id of the allocated entry.
*       Default user is not mapped in this array, so the max size is
*       CFG_NU_OS_STOR_FILE_VFS_NUM_USERS.
*
*
* INPUTS
*
*       task_ptr                NU_TASK* of the task to be added
*
* OUTPUTS
*
*       -1                      Table full or task already in table
*       != (-1)                 task_id of NU_TASK* entry added
*
*************************************************************************/
INT fsu_allocate_task_id(NU_TASK *task_ptr)
{
INT task_id;

    /* Verify task ID is not already allocated */
    task_id = fsu_map_task_id(task_ptr);

    if (task_id < 0)
    {
        /*  There is not one already established, so find a blank spot,
            set up the pointer, and return the ID. */
        for (task_id = 0; task_id < CFG_NU_OS_STOR_FILE_VFS_NUM_USERS; task_id++)
        {
            /*  If we found a blank entry, then return the ID.  */
            if (nufp_task_pointer[task_id] == NU_NULL)
            {
                /*  Save the entry so that we can find it next time.  */
                nufp_task_pointer[task_id] = task_ptr;

                break;
            }
        }

    }
    else
        /* ID is already allocated, use nufp_map_task_id to get the index. 
           Otherwise, trying to allocate twice is an error. */ 
        task_id = -1;

    return (task_id);

}

/************************************************************************
* FUNCTION
*
*       NU_Become_File_User
*
* DESCRIPTION
*
*       In a multitasking environment this function must be called by a
*       task before it may use the API. This reserves a user structure
*       from the pool of user structures for the task.
*
* API DEPRECATION NOTICE:
*
*       This API will be removed in a future release of the product.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*      NU_SUCCESS           if the task may use the file system
*      NUF_INTERNAL         if too many users already.
*      NU_INVALID_MEMORY    if user_heap not initialized.
*
*
*************************************************************************/
#if (FILE_VERSION_COMP == FILE_2_5)
    INT     NU_Become_File_User(VOID)
#elif (FILE_VERSION_COMP > FILE_2_5)
    STATUS  NU_Become_File_User(VOID)                               /*__fn__*/
#endif
{
    UINT16              i;
    PFILE_SYSTEM_USER   p;
    OPTION              preempt_status;
    CONTEXT_HANDLE_TYPE context_handle;
    STATUS              ret_stat = NU_INVALID_MEMORY;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();
    LCK_ENTER(LCK_USR_TABLE);   /* Get lock for user list */

    /* Change to no preemption.  */
    preempt_status = NU_Change_Preemption(NU_NO_PREEMPT);
    /* Move file system user structure to local. */
    p = user_heap;
    if (p)
    {
        /* Get the number of context. */
        context_handle = GET_CONTEXT_HANDLE();
        
        /* Verify task pointer to task_id mapping not performed */
        if (context_handle < 0)
        {
            /* Allocate a task pointer to task_id mapping entry */
            if (fsu_allocate_task_id(NU_Current_Task_Pointer()) < 0)
            { 
                /* User table is full */
                ret_stat = NUF_INTERNAL;
            }
            else
            {
                /* Allocate successful, get the context handle */
                context_handle = GET_CONTEXT_HANDLE();
                if (context_handle < 0)
                {
                    ret_stat = NUF_INTERNAL;
                }
                else
                    ret_stat = NU_SUCCESS;
            }
        }
        else
            ret_stat = NU_SUCCESS;

        /* We should now have a valid context handle for this task */
        if (ret_stat == NU_SUCCESS)
        {

            /* Reset ret_stat to track if the task is already a user */
            ret_stat = NUF_INTERNAL;

            /* Check NU_Become_File_User call. */
            /* A task that has already call NU_Become_File_User will
               have an existing context handle in the user array */
            for (i = 0; i < VFS_NUM_USERS_WDU; i++, p++)
            {
                if (p->context_handle == context_handle)
                {
                    ret_stat = NU_SUCCESS;
                    break;
                }
            }
        
            /* First time calling NU_Become_File_User so we find an
               empty entry in user_heap and setup for the task */
            if (ret_stat != NU_SUCCESS)
            {
                /* Move user_heap to local pointer. Skip the default
                 * user entry by starting at index 1 */
                p = &user_heap[1];
                ret_stat = NUF_INTERNAL;
                for (i = 1; i < VFS_NUM_USERS_WDU; i++, p++)
                {
                    if (!p->context_handle)
                    {
                        /* Initialize FILE_SYSTEM_USER structure. */
                        NUF_Memfill(p, sizeof(FILE_SYSTEM_USER), (UINT8) 0);

                        /* Set the number of context. */
                        p->context_handle = context_handle;

                        /* Set the default drive to invalid */
                        p->dfltdrv = NO_DRIVE;

                        /* Set task id. */
                        SET_RTFS_TASKNO(i);
                        ret_stat = NU_SUCCESS;
                        break;
                    }
                }
            }
        }
    }
    

    /* Deallocate the task pointer to task_id mapping if the
       request to become file user fails */
    if (ret_stat != NU_SUCCESS)
    {
        fsu_deallocate_task_id(NU_Current_Task_Pointer());
    }
    
    NU_Change_Preemption(preempt_status);   /* Restore previous state. */
    LCK_EXIT(LCK_USR_TABLE) /* Release user table lock */
    NU_USER_MODE();

#if (FILE_VERSION_COMP == FILE_2_5)
    if (ret_stat == NU_SUCCESS)
        ret_stat = YES;
    else
        ret_stat = NO;
#endif

    return(ret_stat);
}

/************************************************************************
* FUNCTION
*
*       NU_Check_File_User
*
* DESCRIPTION
*
*       If the the current task is registered as an RTFS user it
*       returns NU_SUCCESS otherwise it returns NUF_BAD_USER.
*
* API DEPRECATION NOTICE:
*
*       This API will be removed in a future release of the product.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       NU_SUCCESS      if the task may use the file system.
*       NUF_BAD_USER    if not registered.
*
*************************************************************************/
#if (FILE_VERSION_COMP == FILE_2_5)
    INT     NU_Check_File_User(VOID)
#elif (FILE_VERSION_COMP > FILE_2_5)
    STATUS  NU_Check_File_User(VOID)                            /*__fn__*/
#endif
{
    STATUS      ret_stat;
    INT         i;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();
    LCK_ENTER(LCK_USR_TABLE);   /* Get lock for user list */
    i = GET_RTFS_TASKNO();      /* Get task id. */

    /* Check Nucleus File user. Verify the RTFS taskno is valid */
    if ( ( (i >= 0) && (i < VFS_NUM_USERS_WDU) ) &&
         (user_heap[i].context_handle == GET_CONTEXT_HANDLE()))
    {
        ret_stat = NU_SUCCESS;
    }
    else
    {
        pc_report_error(PCERR_BAD_USER);
        ret_stat = NUF_BAD_USER;
    }
    LCK_EXIT(LCK_USR_TABLE);    /* Release user table lock */
    NU_USER_MODE();

#if (FILE_VERSION_COMP == FILE_2_5)
    if (ret_stat == NU_SUCCESS)
        ret_stat = YES;
    else
        ret_stat = NO;
#endif

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       NU_Release_File_User
*
* DESCRIPTION
*
*       When a task is through with RTFS it should call here. This frees
*       up a user structure so it may be used by other tasks (callers to
*       NU_Become_File_User).
*
*       Subsequent API calls will fail if this routine has been called.
*
* API DEPRECATION NOTICE:
*
*       This API will be removed in a future release of the product.
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
#if (FILE_VERSION_COMP == FILE_2_5)
    VOID NU_Release_File_User(VOID)                                 /*__fn__*/
#elif (FILE_VERSION_COMP > FILE_2_5)
    STATUS NU_Release_File_User(VOID)
#endif
{
    STATUS  ret_stat;
    MTE_S*   mte;
    FSDH_S  *dhs;
    VNODE   *vnode;
    UINT32  idx;
    INT     task_id, task_num;
    UINT16  i;
    UINT16  dh;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();
    /* Check Nucleus File user. */
    ret_stat = NU_Check_File_User();
    if (ret_stat == NU_SUCCESS)
    {
        /* Loop thru all established disk handles, verifying that the user
         * vnodes have been deallocated.
         */
        idx = fsu_get_user_index();
        for (i = 0; i<FSDH_MAX_DISKS; i++)
        {
            /* If a disk handle index is invalid then try the next one,
               because an invalid dh entry in our array FS_Disk_Handles doesn't
               necessarily mean that the rest of the entries are invalid. */
            ret_stat = fsdh_dh_idx_to_dh(i,&dh);
            if(ret_stat != NU_SUCCESS)
            {
                /* Reset to success because an invalid dh index, doesn't
                   mean NU_Release_File_User failed. */
                ret_stat = NU_SUCCESS;
                continue;
            }

            /* Try to convert the dh to a mte */
            mte = fsl_mte_from_dh(dh);
            if (mte)
            {
                /* Get the FSDH struct for the disk handle */
                ret_stat = fsdh_get_fsdh_struct(dh,&dhs);
                if (ret_stat == NU_SUCCESS)
                {
                    /* Verify that we have a cwd_vnode structure. */
                    if (dhs->fsdh_cwd_vnodes)
                    {
                        vnode = (VNODE*) &dhs->fsdh_cwd_vnodes[idx];
                        /* Check if the vnode is in use. */
                        if (vnode && vnode->vnode_fsnode)
                        {
                            ret_stat = NUF_IN_USE;
                            break;
                        }
                    }
                }
            }
        
        }
        if (ret_stat == NU_SUCCESS)
        {
            LCK_ENTER(LCK_USR_TABLE);           /* Lock table while releasing user */

            task_num = GET_RTFS_TASKNO();
            task_id = NUFP_Current_Task_ID();
            if ((task_id >= 0) && (task_id < CFG_NU_OS_STOR_FILE_VFS_NUM_USERS))
            {
                /*  remove from nufp_task_pointer table*/
                nufp_task_pointer[task_id] = NU_NULL;
            }
            
            if ( (task_num >= 0) && (task_num < VFS_NUM_USERS_WDU) )
            {
                /* Cleanup user_heap entry. */
                user_heap[task_num].context_handle = 0;
                user_heap[task_num].p_errno = 0;
                user_heap[task_num].dfltdrv = NO_DRIVE;
            }
            LCK_EXIT(LCK_USR_TABLE);            /* Release user table lock */

        }
    }
    NU_USER_MODE();
#if (FILE_VERSION_COMP > FILE_2_5)
    return (ret_stat);
#endif
}
/************************************************************************
* FUNCTION
*
*       fs_current_user_structure
*
* DESCRIPTION
*
*       This function is called every time the user or the file system
*       kernel invokes the fs_user macro. It returns the task's private
*       FILE_SYSTEM_USER structure that was allocated by calling
*       NU_Become_File_User(). If the task has not previously called
*       NU_Become_File_User then the default user entry (index zero)
*       is returned.
*
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       Current task's FILE_SYSTEM_USER structure pointer. Index zero is
*       the default user.
*
*************************************************************************/
PFILE_SYSTEM_USER fs_current_user_structure(VOID)
{
    INT                i;
    FILE_SYSTEM_USER   *ret_pointer;

    /* Get task id. */
    i = GET_RTFS_TASKNO();

    /* Check Nucleus File user.  Verify the RTFS taskno is valid. */
    if ( (i >= 0) && (i < VFS_NUM_USERS_WDU) )
    {
        ret_pointer = &user_heap[i]; /* Index zero is the default user */
    }
    else
    {
        /* Since the GET_RTFS_TASKNO macro always returns a value between
         * 0 and VFS_NUM_USERS_WDU, this should never occur. This else
         * condition was added to resolve false positive warnings in static
         * analysis tools. */
        ret_pointer = &user_heap[0]; /* Index zero is the default user */
    }

    return(ret_pointer);
}


/************************************************************************
* FUNCTION
*
*       fs_init_users
*
* DESCRIPTION
*
*       Initialize user structure.
*
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS
*       NU_NO_MEMORY                Memory allocation failed
*
*************************************************************************/
STATUS fsu_init_users(VOID)
{
    STATUS ret_stat = NU_SUCCESS;
    INT pool_size;

    Task_Map[0] = 0; /* Initialize map to default user structure */

    /* Verify user structure is not initialized */
    if (!user_heap)
    {
        /* Allocate memory for user list. */
        pool_size = sizeof(FILE_SYSTEM_USER) * VFS_NUM_USERS_WDU;
        user_heap = NUF_Alloc(pool_size);
        if (!user_heap)
            ret_stat = NU_NO_MEMORY;
        else
            NUF_Memfill(user_heap, pool_size, (UINT8)0);
    }
    return (ret_stat);
}

/************************************************************************
 * FUNCTION
 *
 *       pc_free_all_users
 *
 * DESCRIPTION
 *
 *       Free all cwd objects for a drive.
 *
 *
 * INPUTS
 *
 *       dh                          Disk handle
 *
 * OUTPUTS
 *
 *       None.
 *
 *************************************************************************/
VOID pc_free_all_users(UINT16 dh)
{
INT16               i;
MTE_S               *mte;
FSDH_S              *dhs;
VNODE               *vnode;

    /* Move file system user structure to local. */
    mte = fsl_mte_from_dh(dh);
    if (mte)
    {
        if ( fsdh_get_fsdh_struct(dh,&dhs) == NU_SUCCESS)
        {
            /* For each user, deallocate vnode if assigned */
            for (i = 0; i < VFS_NUM_USERS_WDU; i++)
            {
                if (dhs->fsdh_cwd_vnodes)
                {
                    /* Get the FSDH struct for the disk handle */
                    vnode = (VNODE*) &dhs->fsdh_cwd_vnodes[i];
                    /* Verify we have something to dealloc */
                    if (vnode->vnode_fsnode)
                    {
                        mte->mte_fs->fs_vnode_deallocate(dh, vnode->vnode_fsnode);
                        vnode->vnode_fsnode = NU_NULL;
                    }
                }
            }
        }
    }
}

/************************************************************************
 *  FUNCTION
 *
 *       NUFP_Current_Task_ID
 *
 *  DESCRIPTION
 *
 *       This function converts a Task Pointer to a Task ID.
 *
 *  INPUTS
 *
 *       None.
 *
 *  OUTPUTS
 *
 *       task_id                The id of the task that represents the 
 *                              task pointer.
 *       -1                     Task was not in map
 *
 *************************************************************************/
INT NUFP_Current_Task_ID(VOID)
{
    return (fsu_map_task_id(NU_Current_Task_Pointer()));
}

/************************************************************************
*  FUNCTION
*
*       fsu_set_user_error
*
*  DESCRIPTION
*
*       Set the error number per user structure
*
*  INPUTS
*
*
*  OUTPUTS
*
*
*************************************************************************/
VOID fsu_set_user_error(UINT32 file_errno)
{
    fs_user->p_errno = file_errno;
}

/************************************************************************
*  FUNCTION
*
*       fsu_get_user_error
*
*  DESCRIPTION
*
*       Return the error number per user structure
*
*  INPUTS
*
*       None.
*
*  OUTPUTS
*
*       UINT32                      errno
*
*************************************************************************/
UINT32 fsu_get_user_error()
{
    return (fs_user->p_errno);
}

/************************************************************************
*  FUNCTION
*
*       fsu_get_user_index
*
*  DESCRIPTION
*
*       Rerun the user index
*
*  INPUTS
*
*       None.
*
*  OUTPUTS
*
*
*************************************************************************/
UINT32 fsu_get_user_index()
{
    return (GET_RTFS_TASKNO());
}

/************************************************************************
*  FUNCTION
*
*       fsu_get_default_user_index
*
*  DESCRIPTION
*
*       Return the default user index
*
*  INPUTS
*
*       None.
*
*  OUTPUTS
*       The default user index
*
*************************************************************************/
UINT32 fsu_get_default_user_index()
{
    return (Task_Map[0]);
}

/************************************************************************
* FUNCTION
*
*       pc_memfill
*
* DESCRIPTION
*
*       Fill "vto" with "size" instances of "c"
*       Fill a buffer with a character
*
*
* INPUTS
*
*       vto                                 Copy to data buffer
*       size                                Size
*       c                                   Character
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_memfill(VOID *vto, INT size, UINT8 c)
{
    UINT8 *to = (UINT8 *)vto;

    while (size--)
        *to++ = c;
}
