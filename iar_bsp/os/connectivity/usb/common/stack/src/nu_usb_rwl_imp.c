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

/***********************************************************************
 *
 * FILE NAME
 *
 *      nu_usb_rwl_imp.c
 *
 * COMPONENT
 *        OS Services: Read/Write Lock
 *
 * DESCRIPTION 
 *    This file contains functions that implement Read/Write Lock. This lock
 * provides shared access to multiple reader tasks or an exclusive access to
 * a writer task.
 *
 *
 * DATA STRUCTURES
 *  None
 *
 * FUNCTIONS
 *    os_create_rw_lock        Creates a Read/Write lock.
 *    os_destroy_rw_lock       Destroys a Read/Write lock.
 *    os_grab_rw_lock          Grabs the read/write lock in Read or Write mode.
 *    os_release_rw_lock       Releases the read/write access on the lock.
 *    os_upgrade_rw_lock       Upgrades a task having Read access to have an 
 *                               exclusive write access.
 *    os_downgrade_rw_lock     Downgrades a task having an exclusive Write 
 *                              access to a shared read access.
 * DEPENDENCIES 
 *
 *    nu_usb.h                    All USB Definition
 *
 ************************************************************************/
#ifndef USB_RWL_IMP_C
#define	USB_RWL_IMP_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/************************************************************************
 *
 * FUNCTION
 *    os_create_rw_lock
 *
 * DESCRIPTION
 *    It creates semaphores that are used in the realization of the read/write 
 * lock.
 *
 * INPUTS
 *   rw_lock_t *rw   Ptr to read/write control block.
 * 
 * OUTPUTS
 *    STATUS   NU_SUCCESS Indicates successful creation of the read/write lock.
 *             NU_INVALID_POINTER Indicates that the argument passed is NU_NULL.
 *             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is invalid.
 *
 *************************************************************************/
STATUS os_create_rw_lock (rw_lock_t * rw)
{
    STATUS status       = NU_SUCCESS;
	STATUS internal_sts = NU_SUCCESS;
    UINT8 i;

    if (rw == NU_NULL) 
        return NU_INVALID_POINTER;

    status = NU_Create_Semaphore (&(rw->access_lock), "RW_ACC", 1, NU_PRIORITY);
    if (status != NU_SUCCESS)
        return (status);
    status =
        NU_Create_Semaphore (&(rw->database_lock), "RW_DB", 1, NU_PRIORITY);
    if (status != NU_SUCCESS)
    {
        internal_sts |= NU_Delete_Semaphore (&(rw->access_lock));
		NU_UNUSED_PARAM(internal_sts);
        return (status);
    }
    status = NU_Create_Semaphore (&(rw->num_clients), "RW_CLNT", RW_MAX_READERS,
                                  NU_PRIORITY);
    if (status != NU_SUCCESS)
    {
        internal_sts |= NU_Delete_Semaphore (&(rw->database_lock));
        internal_sts |= NU_Delete_Semaphore (&(rw->access_lock));
		NU_UNUSED_PARAM(internal_sts);
        return (status);
    }
    for (i = 0; i <= RW_MAX_READERS; i++)
    {
        rw->tasks[i] = NU_NULL;
        rw->recursive_cnt[i] = 0;
    }
    return (status);
}

/************************************************************************
 *
 * FUNCTION
 *   os_destroy_rw_lock
 *
 * DESCRIPTION
 *   It destroys the read/write by freeing the deleting the associated 
 * semaphores.
 *
 * INPUTS
 *   rw_lock_t *rw   Ptr to read/write control block.
 * 
 * OUTPUTS
 *    STATUS   NU_SUCCESS Indicates successful deletion of the read/write lock.
 *             NU_INVALID_POINTER Indicates that the argument passed is NU_NULL.
 *
 *************************************************************************/
STATUS os_destroy_rw_lock (rw_lock_t * rw)
{
    STATUS status = NU_SUCCESS;

    if (rw == NU_NULL)
        return NU_INVALID_POINTER;

    status |= NU_Delete_Semaphore (&(rw->num_clients));
    status |= NU_Delete_Semaphore (&(rw->database_lock));
    status |= NU_Delete_Semaphore (&(rw->access_lock));

    return (status);
}

/************************************************************************
 *
 * FUNCTION
 *    os_grab_rw_lock
 * DESCRIPTION
 *   This function provides a reader task to get a shared access on the lock
 * or a writer to get an exclusive access on it. A reader task may be blocked
 * by this call if a writer is already having an exclusive access on it or if
 * there are already max number of readers (RW_MAX_READERS) are sharing it. 
 * Similarly a writer task may get blocked in this function, if reader task(s)
 * are already sharing it.
 * 
 * INPUTS
 *   rw_lock_t *rw   Ptr to read/write control block.
 * 
 * OUTPUTS
 *    STATUS   NU_SUCCESS Indicates successful grabbing of the read/write lock.
 *             NU_INVALID_POINTER Indicates that the argument passed is NU_NULL.
 *             NU_INVALID_OPERATION Indicates that the mode passed is neither
 *                WRITE_MODE nor READ_MODE.
 *             NU_INVALID_SUSPEND Indicates that this API is attempted
 *                   from a non-task thread.
 *             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
 *                   was suspended.
 *             NU_UNAVAILABLE Indicates the semaphore is unavailable.
 *
 *************************************************************************/
STATUS os_grab_rw_lock (rw_lock_t * rw,
                        UINT8 mode)
{
    STATUS status;
	STATUS internal_sts = NU_SUCCESS;
    UINT8 i, j, done = 0;

    if (rw == NU_NULL)
        return NU_INVALID_POINTER;
	
    status = NU_Obtain_Semaphore (&(rw->database_lock), NU_SUSPEND);
    if (status != NU_SUCCESS)
        return (status);
    if (mode == WRITE_MODE)
    {
        if (rw->tasks[0] == NU_Current_Task_Pointer ())
        {
            rw->recursive_cnt[0]++;
            done = 1;
        }
    }
    else
    {
        /* If a writer asks for read access, just increment the recursive cnt 
         */
        for (i = 0; i <= RW_MAX_READERS; i++)
        {
            if (rw->tasks[i] == NU_Current_Task_Pointer ())
            {
                rw->recursive_cnt[i]++;
                done = 1;
                break;
            }
        }
    }
    status = NU_Release_Semaphore (&(rw->database_lock));
    if (done)
        return (NU_SUCCESS);

    status |= NU_Obtain_Semaphore (&(rw->access_lock), NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        return (status);
    }
    if (mode == READ_MODE)
    {
        status = NU_Obtain_Semaphore (&(rw->num_clients), NU_SUSPEND);
        if (status != NU_SUCCESS)
        {
            internal_sts |= NU_Release_Semaphore (&(rw->access_lock));
			NU_UNUSED_PARAM(internal_sts);
            return (status);
        }
        status = NU_Obtain_Semaphore (&(rw->database_lock), NU_SUSPEND);
        if (status != NU_SUCCESS)
        {
            internal_sts |=  NU_Release_Semaphore (&(rw->num_clients));
            internal_sts |= NU_Release_Semaphore (&(rw->access_lock));
			NU_UNUSED_PARAM(internal_sts);
            return (status);
        }
        for (i = 1; i <= RW_MAX_READERS; i++)
        {
            if (rw->tasks[i] == 0)
            {
                rw->tasks[i] = NU_Current_Task_Pointer ();
                rw->recursive_cnt[i]++;
                internal_sts |=  NU_Release_Semaphore (&(rw->database_lock));
                internal_sts |=  NU_Release_Semaphore (&(rw->access_lock));
				NU_UNUSED_PARAM(internal_sts);
                return (NU_SUCCESS);
            }
        }
    }

    if (mode == WRITE_MODE)
    {
        /* Kick out all the readers */
        for (i = 0; i < RW_MAX_READERS; i++)
        {
            status = NU_Obtain_Semaphore (&(rw->num_clients), NU_SUSPEND);
            if (status != NU_SUCCESS)
            {
                for (j = 0; j < i; j++)
                {
					internal_sts |=  NU_Release_Semaphore (&(rw->num_clients));
                }
				internal_sts |= NU_Release_Semaphore (&(rw->access_lock));
				NU_UNUSED_PARAM(internal_sts);
                return (status);
            }
        }
        status = NU_Obtain_Semaphore (&(rw->database_lock), NU_SUSPEND);
        if (status != NU_SUCCESS)
        {
            for (i = 0; i < RW_MAX_READERS; i++)
            {
                internal_sts |=  NU_Release_Semaphore (&(rw->num_clients));
            }
            internal_sts |=  NU_Release_Semaphore (&(rw->access_lock));
			NU_UNUSED_PARAM(internal_sts);
            return (status);
        }
        if (rw->tasks[0] != NU_NULL)
            NU_USB_ASSERT (0);
        if (rw->recursive_cnt[0] != 0)
            NU_USB_ASSERT (0);
        rw->tasks[0] = NU_Current_Task_Pointer ();
        rw->recursive_cnt[0] = 1;

        internal_sts |= NU_Release_Semaphore (&(rw->database_lock));
        internal_sts |= NU_Release_Semaphore (&(rw->access_lock));
		NU_UNUSED_PARAM(internal_sts);
        return (NU_SUCCESS);;

    }
    /* Invalid Mode */
    return NU_INVALID_OPERATION;
}

/************************************************************************
 *
 * FUNCTION
 *    os_release_rw_lock
 * DESCRIPTION
 *    This is used by a task to relinquish the owned the read/write lock. The
 * behavior is undefined, if the task doesn't own the lock and tries to release
 * it or task owns it in Read mode and tries to release it in write mode or
 * vice-versa. The ownership is lost only when release is done in equal no.of 
 * times that the lock was grabbed by the task.
 * INPUTS
 *   rw_lock_t *rw   Ptr to read/write control block.
 * 
 * OUTPUTS
 *    STATUS   NU_SUCCESS Indicates successful release of the read/write lock.
 *             NU_INVALID_POINTER Indicates that the argument passed is NU_NULL.
 *             NU_INVALID_OPERATION Indicates that the mode passed is neither
 *                WRITE_MODE nor READ_MODE.
 *             NU_INVALID_SUSPEND Indicates that this API is attempted
 *                   from a non-task thread.
 *             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
 *                   was suspended.
 *             NU_UNAVAILABLE Indicates the semaphore is unavailable.
 *
 *************************************************************************/
STATUS os_release_rw_lock (rw_lock_t * rw,
                           UINT8 mode)
{
    STATUS status;
	STATUS internal_sts = NU_SUCCESS;
    UINT8 i;

    if (rw == NU_NULL)
        return NU_INVALID_POINTER;

    status = NU_Obtain_Semaphore (&(rw->database_lock), NU_SUSPEND);
    if (status != NU_SUCCESS)
        return (status);

    if (mode == WRITE_MODE)
    {
        if (rw->tasks[0] != NU_Current_Task_Pointer ())
        {
            internal_sts |= NU_Release_Semaphore (&(rw->database_lock));
			NU_UNUSED_PARAM(internal_sts);
            return NU_NOT_PRESENT;
        }
        rw->recursive_cnt[0]--;
        if (rw->recursive_cnt[0] != 0)
        {
            internal_sts |= NU_Release_Semaphore (&(rw->database_lock));
			NU_UNUSED_PARAM(internal_sts);
            return (NU_SUCCESS);
        }
        rw->tasks[0] = NU_NULL;
        internal_sts |= NU_Release_Semaphore (&(rw->database_lock));
        for (i = 0; i < RW_MAX_READERS; i++)
            internal_sts |= NU_Release_Semaphore (&(rw->num_clients));
         return (NU_SUCCESS);
    }
    if (mode == READ_MODE)
    {
        for (i = 0; i <= RW_MAX_READERS; i++)
        {
            if (rw->tasks[i] == NU_Current_Task_Pointer ())
            {
                rw->recursive_cnt[i]--;
                /* if i is 0, (the write mode ), the last release should be with mode 
                 * = WRITE and not READ_MODE 
                 */
                if (rw->recursive_cnt[i] == 0)
                {
                    rw->tasks[i] = NU_NULL;
                    internal_sts |= NU_Release_Semaphore (&(rw->database_lock));
                    internal_sts |=  NU_Release_Semaphore (&(rw->num_clients));
					NU_UNUSED_PARAM(internal_sts);
                    return (NU_SUCCESS);
                }
                else
                {
                    internal_sts |= NU_Release_Semaphore (&(rw->database_lock));
					NU_UNUSED_PARAM(internal_sts);
                    return (NU_SUCCESS);
                }
            }
        }
		
        internal_sts |= NU_Release_Semaphore (&(rw->database_lock));
		NU_UNUSED_PARAM(internal_sts);
        return NU_NOT_PRESENT;
    }
    /* Invalid Mode */
    return NU_INVALID_OPERATION;
}

/************************************************************************
 *
 * FUNCTION
 *    os_upgrade_rw_lock
 *
 * DESCRIPTION
 *     This function upgrades a task thats already having a shared Read lock 
 * to gain an exclusive write access on the lock.  The reader's recursive count
 * should be 1, meaning the task shouldn't have grabbed the Read lock more
 * than once.The behavior is undefined if the task calls this function without
 * having a Read lock or if the task is already holding the write lock.
 * WRITE_MODE should be used to release this lock after this successfully 
 * upgrades the reader.
 *
 * INPUTS
 *   rw_lock_t *rw   Ptr to read/write control block.
 * 
 * OUTPUTS
 *    STATUS   NU_SUCCESS Indicates successful upgrade to write mode.
 *             NU_INVALID_POINTER Indicates that the argument passed is NU_NULL.
 *             NU_INVALID_OPERATION Indicates that the mode passed is neither
 *                WRITE_MODE nor READ_MODE or recursive cnt is > 1.
 *             NU_INVALID_SUSPEND Indicates that this API is attempted
 *                   from a non-task thread.
 *             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
 *                   was suspended.
 *             NU_UNAVAILABLE Indicates the semaphore is unavailable.
 *
 *************************************************************************/
STATUS os_upgrade_rw_lock (rw_lock_t * rw)
{
    STATUS status;
    STATUS internal_sts = NU_SUCCESS;
	UINT8 i, j;

    if (rw == NU_NULL)
        return NU_INVALID_POINTER;
		
    status = NU_Obtain_Semaphore (&(rw->database_lock), NU_SUSPEND);
    if (status != NU_SUCCESS)
        return (status);
    for (i = 1; i <= RW_MAX_READERS; i++)
    {
        if (rw->tasks[i] == NU_Current_Task_Pointer ())
            break;
    }
    if (i > RW_MAX_READERS)
    {
        internal_sts = NU_Release_Semaphore (&(rw->database_lock));
		NU_UNUSED_PARAM(internal_sts);
        return NU_NOT_PRESENT;
    }
    if (rw->recursive_cnt[i] != 1)
    {
        internal_sts = NU_Release_Semaphore (&(rw->database_lock));
        /* The recursive cnt must first be made to 1 , before upgrading */
		NU_UNUSED_PARAM(internal_sts);
        return NU_INVALID_OPERATION;
    }

    rw->tasks[i] = NU_NULL;
    rw->recursive_cnt[i] = 0;
    status = NU_Release_Semaphore (&(rw->database_lock));

    status |= NU_Obtain_Semaphore (&(rw->access_lock), NU_SUSPEND);
    if (status != NU_SUCCESS)
        return (status);

    /* Kick out all the other readers */
    for (i = 0; i < RW_MAX_READERS - 1; i++)
    {
        status = NU_Obtain_Semaphore (&(rw->num_clients), NU_SUSPEND);
        if (status != NU_SUCCESS)
        {
            for (j = 0; j < i; j++)
			{
                internal_sts |= NU_Release_Semaphore (&(rw->num_clients));
			}
			internal_sts |= NU_Release_Semaphore (&(rw->access_lock));
			NU_UNUSED_PARAM(internal_sts);
            return (status);
        }
    }
    status = NU_Obtain_Semaphore (&(rw->database_lock), NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        for (i = 0; i < RW_MAX_READERS; i++)
        {
            internal_sts |= NU_Release_Semaphore (&(rw->num_clients));
        }
        internal_sts |= NU_Release_Semaphore (&(rw->access_lock));
		NU_UNUSED_PARAM(internal_sts);
        return (status);
    }
    if (rw->recursive_cnt[0] != 0)
        NU_USB_ASSERT (0);
    rw->tasks[0] = NU_Current_Task_Pointer ();
    rw->recursive_cnt[0] = 1;

    status = NU_Release_Semaphore (&(rw->database_lock));
    status |= NU_Release_Semaphore (&(rw->access_lock));
    return (status);
}

/************************************************************************
 *
 * FUNCTION
 *    os_downgrade_rw_lock
 *
 * DESCRIPTION
 *     This function causes the task thats currently holding an exclusive write
 * access on the lock, to have a shared read access on it. The behavior is 
 * undefined, if the function is called by a task that doesn't have a write 
 * access. The task calling this function should have it recursive cnt to be 1, 
 * meaning it should have invoked os_grab_rw_lock in WRITE_MODE exactly once 
 * in the past. After this function successfully downgrades the task to reader,
 * the ownership on the lock can be released using READ_MODE.
 *
 * INPUTS
 *   rw_lock_t *rw   Ptr to read/write control block.
 * 
 * OUTPUTS
 *    STATUS   NU_SUCCESS Indicates successful downgrading to read mode.
 *             NU_INVALID_POINTER Indicates that the argument passed is NU_NULL.
 *             NU_INVALID_OPERATION Indicates that the mode passed is neither
 *                WRITE_MODE nor READ_MODE or recursive cnt is > 1.
 *             NU_INVALID_SUSPEND Indicates that this API is attempted
 *                   from a non-task thread.
 *             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
 *                   was suspended.
 *             NU_UNAVAILABLE Indicates the semaphore is unavailable.
 *
 *************************************************************************/
STATUS os_downgrade_rw_lock (rw_lock_t * rw)
{
    STATUS status;
	STATUS internal_sts = NU_SUCCESS;
    UINT8 i;

    if (rw == NU_NULL)
        return NU_INVALID_POINTER;
    	
    status = NU_Obtain_Semaphore (&(rw->database_lock), NU_SUSPEND);
    if (status != NU_SUCCESS)
        return (status);

    rw->tasks[0] = NU_NULL;
    rw->recursive_cnt[0] = 0;
    rw->tasks[1] = NU_Current_Task_Pointer ();
    rw->recursive_cnt[1] = 1;
    internal_sts |= NU_Release_Semaphore (&(rw->database_lock));
    /* Keep one instance for yourself */
    for (i = 0; i < RW_MAX_READERS - 1; i++)
    {
        status |= NU_Release_Semaphore (&(rw->num_clients));
    }
	NU_UNUSED_PARAM(internal_sts);
    return (NU_SUCCESS);
}

/*************************************************************************/

#endif /* USB_RWL_IMP_C */
/*************************** end of file ********************************/

