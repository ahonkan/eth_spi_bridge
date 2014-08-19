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
*       fs_dsk.c
*
* COMPONENT
*
*       Disk
*
* DESCRIPTION
*
*       Contains API for disk operations.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Open_Disk
*       NU_Close_Disk
*       NU_Disk_Abort
*       NU_Check_Disk
*
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/fd_extr.h"
#include "storage/lck_extr.h"
#include "storage/fsl_extr.h"
#include "storage/user_extr.h"
#include "storage/util_extr.h"
#include "storage/vnode_extr.h"
#include "storage/dh_extr.h"
/************************************************************************
* FUNCTION
*
*       NU_Open_Disk
*
* DESCRIPTION
*
*       Manages user / disk access. Each user of a disk should call this
*       service. This will track usage of a disk. Usage will prevent
*       a disk from being unmounted.
*
*
* INPUTS
*
*       path                                Path name
*
* OUTPUTS
*
*       NU_SUCCESS                          Disk was initialized
*                                            successfully.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_FATCORE                         FAT cache table too small.
*       NUF_NO_PARTITION                    No partition in disk.
*       NUF_FORMAT                          Disk not formatted.
*       NUF_NO_MEMORY                       Can't allocate internal
*                                            buffer.
*       NUF_IO_ERROR                        Driver returned error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*       Other error code                    See the driver error code.
*
*************************************************************************/
STATUS NU_Open_Disk(CHAR *path)
{
    STATUS      ret_val;
    MTE_S*      mte;
    FSDH_S      *dhs;
    VNODE       *vnode;
    UINT32      idx;
    UINT8       user_state;
    CHAR        newpath[4];    
    
    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_val = CHECK_USER()) == NU_SUCCESS)
    {       
        /* Verify user has not already opened disk? */
        /* Resolve mount table entry from path */
        mte = fsl_mte_from_fqpath(path);
        if (mte)
        {
            idx = fsu_get_user_index();
            
            /* Check the disk state */
            ret_val = fsdh_get_user_state(mte->mte_dh, idx, &user_state);
            if (ret_val == NU_SUCCESS)
            {            
                if (user_state == FSDH_USER_OPENED)
                {

                    /* User has already opened the disk */
                    ret_val = NUF_NO_ACTION;
                    
                }
                else if (user_state == FSDH_USER_ABORTED)
                {
                    /* Disk has been aborted. It must be re-mounted */
                    ret_val = NUF_NOT_OPENED;                    
                }
                else if (user_state == FSDH_USER_CLOSED)
                {
                    ret_val = fsdh_get_fsdh_struct(mte->mte_dh,&dhs);
                    if(dhs)
                    {
                        /* Increment the number of open disk. */
                        if(dhs->fsdh_opencount < CFG_NU_OS_STOR_FILE_VFS_NUM_USERS)                           
                            dhs->fsdh_opencount +=1;
                                                
                        /* If check disk has exclusive access don't let this task open the disk. */
                        if(dhs->fsdh_exclusive_access == NU_TRUE)
                        {                            
                            dhs->fsdh_opencount -= 1;
                            ret_val = NUF_ACCES;                           
                        }
                        
                        if (ret_val == NU_SUCCESS)
                        {
                            if (dhs->fsdh_cwd_vnodes)
                            {
                                /* Get the FSDH struct for the disk handle */
                                vnode = (VNODE*) &dhs->fsdh_cwd_vnodes[idx];
                                
                                /* If there is something there, the user has 
                                    already opened the disk */
                                    
                                if (vnode->vnode_fsnode)
                                {
                                    dhs->fsdh_opencount -= 1;                                
                                    ret_val = NUF_NO_ACTION;
                                }
                            }
                            else
                                /* Internal error. fsdh_cwd_vnodes should already be allocated 
                                and we should have failed at the retrieve mte */
                                ret_val = NUF_INTERNAL;
                        }
                        
                    }/* Verify that dhs is valid. */
                }
                else
                    /* Internal error, the state is not recognized */            
                    ret_val = NUF_INTERNAL;
            }
            
            if (ret_val == NU_SUCCESS)
            {
                ret_val = fsdh_set_user_state(mte->mte_dh, idx, FSDH_USER_OPENED);
            }

            /* Setup the default directory for the newly opened drive */
            if (ret_val == NU_SUCCESS)
            {
                NUF_Copybuff(newpath,path,3);
                newpath[2] = '\\';
                newpath[3] = '\0';
                ret_val = NU_Set_Current_Dir(newpath);
            }                            
        }
        else
            ret_val = NUF_BADDRIVE;
            
    }
    LCK_FS_EXIT()
    return(ret_val);
}

/************************************************************************
* FUNCTION
*
*       NU_Close_Disk
*
* DESCRIPTION
*
*       Given a path name containing a valid drive specifier, flush the
*       file allocation table and purge any buffers or objects
*       associated with the drive.
*
*
* INPUTS
*
*       path                                Path name
*
* OUTPUTS
*
*       NU_SUCCESS                          If service is successful.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      Drive not opened.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
#if (FILE_VERSION_COMP == FILE_2_5)
INT     NU_Close_Disk(CHAR *path)
#endif
#if (FILE_VERSION_COMP > FILE_2_5)
STATUS   NU_Close_Disk(CHAR *path)
#endif
{
    STATUS      ret_val;
    MTE_S*      mte;
    FSDH_S      *dhs;
    VNODE       *vnode;
    UINT32      idx;
    UINT8       user_state;
    INT         fs_fd = 0;
    INT         vfs_fd;

    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_val = CHECK_USER()) == NU_SUCCESS)
    {
        /* Resolve mount table entry from path */
        mte = fsl_mte_from_fqpath(path);
        if (mte)
        {
            idx = fsu_get_user_index();
            
            ret_val = fsdh_get_user_state(mte->mte_dh, idx, &user_state);
            if (ret_val == NU_SUCCESS)
            {
                if (user_state == FSDH_USER_CLOSED)
                {
                    /* Disk is already closed */
                    ret_val = NUF_NOT_OPENED;
                }
                else if (user_state == FSDH_USER_ABORTED)
                {
                    /* Disk has been aborted */
                    ret_val = NUF_NOT_OPENED;
                }
                else if (user_state == FSDH_USER_OPENED)
                {   
                    ret_val = fsdh_get_fsdh_struct(mte->mte_dh,&dhs);
                    if(dhs)
                    {
                        if(dhs->fsdh_opencount == 1)
                        {
                            for(vfs_fd = 0; vfs_fd < gl_FD_MAX_FD && dhs->fsdh_opencount == 1; ++vfs_fd)
                            {
                                ret_val = fd_get_fs_fd(vfs_fd,mte->mte_dh,&fs_fd);
                                if(ret_val == NU_SUCCESS)
                                {
                                    /* Dispatch the FSs specific routine */
                                    ret_val = mte->mte_fs->fs_close(fs_fd);

                                    if(ret_val == NU_SUCCESS)
                                    {
                                        /* Free the system file descriptor regardless of file system
                                        specific return */
                                        fd_free_fd(vfs_fd);
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                else
                                {
                                    /* If ret_val is NUF_BADPARM set ret_val to NU_SUCCESS,     */
                                    /* because the current entry in the file descriptor table   */
                                    /* doesn't need to be closed.                               */
                                    if(ret_val == NUF_BADPARM)
                                        ret_val = NU_SUCCESS;
                                }
                            } 
                            dhs->fsdh_opencount -=1 ;

                        }/* dhs.fsdh_opencount == 1 */                        
                    
                        /* Decrement the number of disk open. */
                        if(dhs->fsdh_opencount > 1)
                            dhs->fsdh_opencount -=1;
                        
                    }/* Verify dhs is valid. */ 
                        
                    if ((ret_val == NU_SUCCESS) && (dhs))
                    {
                        if (dhs->fsdh_cwd_vnodes)
                        {
                            /* Get the FSDH struct for the disk handle */
                            vnode = (VNODE*) &dhs->fsdh_cwd_vnodes[idx];
                            
                            /* Verify we have something to dealloc */
                            if (vnode->vnode_fsnode)
                            {
                                mte->mte_fs->fs_vnode_deallocate(mte->mte_dh, vnode->vnode_fsnode);
                                vnode->vnode_fsnode = NU_NULL;
                            }
                            /* User did not open this disk and therefore cannot close it. */
                            else
                            ret_val = NUF_NOT_OPENED;
                        }
                    }
                }
                else
                    /* Internal error, the state is not recognized */            
                    ret_val = NUF_INTERNAL;                    
            }
            
            /* Update user disk state */
            if (ret_val == NU_SUCCESS)
            {
                ret_val = fsdh_set_user_state(mte->mte_dh, idx, FSDH_USER_CLOSED);
            }
        }
        else
            ret_val = NUF_BADDRIVE;
    }    
    
    LCK_FS_EXIT()
    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       NU_Disk_Abort
*
* DESCRIPTION
*
*       If an application senses that there are problems with a disk, it
*       should call NU_Disk_Abort("D:"). This will cause all resources
*       associated with that drive to be freed, but no disk writes will
*       be attempted. All file descriptors associated with the drive
*       become invalid. After correcting the problem call
*       NU_Open_Disk("D:") to re-mount the disk and re-open your files.
*
*
* INPUTS
*
*       path                                Path name
*
* OUTPUTS
*
*       
*	    NU_SUCCESS                          Abort request completed 
*                                           successfully. 
*       NUF_BAD_USER 						Task not registered as a 
*											file user. 
* 		<0 									File system specific status 
*											error value.
*
*************************************************************************/
#if (FILE_VERSION_COMP == FILE_2_5)
VOID NU_Disk_Abort(CHAR *path)
{
#elif (FILE_VERSION_COMP > FILE_2_5)
STATUS NU_Disk_Abort(CHAR *path)
{
#endif

    STATUS ret_stat;
    MTE_S*      mte;
    INT32       idx;
    FSDH_S      *dhs;

    LCK_FS_ENTER()

    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        /* Resolve mount table entry from path */
        mte = fsl_mte_from_fqpath(path);
        if (mte)
        {
            /* Free all user CWD structures */
            pc_free_all_users(mte->mte_dh);       

            /* Dispatch FS specific routine */
            ret_stat = fsdh_get_fsdh_struct(mte->mte_dh,&dhs);
            
            if(ret_stat == NU_SUCCESS)
            {
                if(dhs)
                {  
                    /* Reset number of disk open to zero. */
                    dhs->fsdh_opencount = 0;
                }
            }

            ret_stat = mte->mte_fs->fs_disk_abort(mte->mte_dh);

             /* Free the VFS fd associated with this device. */
            if(ret_stat == NU_SUCCESS)
                ret_stat = fd_free_fd_for_dh(mte->mte_dh);

            /* Update drive id to indicate invalid objects */
            mte->mte_drive_id = NU_NULL;

            /* Update the disk/user status */
            for(idx = 0; idx < VFS_NUM_USERS_WDU; idx++)
            {
                fsdh_set_user_state(mte->mte_dh, idx, FSDH_USER_ABORTED);
            }
        }
    }

    LCK_FS_EXIT()
#if (FILE_VERSION_COMP > FILE_2_5)
    return (ret_stat);
#endif
}

/************************************************************************
* FUNCTION
*
*       NU_Check_Disk
*
* DESCRIPTION
*
*       Used to check the disk for errors. Should only be ran
*       on a drive that is only opened by the task calling
*       NU_Check_Disk. For memory requirements please see the 
*       documentation of your file systems specific check
*       disk.
*
*
* INPUTS
*
*       *path(in)                           Drive Letter.
*       flag(in)                            Option flags to indicate
*                                           which test(s) to run.
*       mode(in)                            Used to indicate
*                                           which mode the test
*                                           should run in.
*
* OUTPUTS
*
*       NU_SUCCESS                          No errors where found on 
*                                           the disk.
*       NUF_ACESS                           More than one task
*                                           has the disk open.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_LOG_FILE_CREATED                If errors where found and a log
*                                           file was created to report them.
*       NUF_FAT_TABLES_DIFFER               FAT tables differ so log file
*                                           couldn't be created.
*
*************************************************************************/
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)
STATUS  NU_Check_Disk(CHAR *path,UINT8 flag,UINT8 mode)
{
    STATUS ret_val;
    MTE_S*      mte;
    FSDH_S      *dhs = NU_NULL;
    OPTION      old_preempt;

    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_val = CHECK_USER()) == NU_SUCCESS)
    {        
        /* Resolve mount table entry from path */
        mte = fsl_mte_from_fqpath(path);
        if(mte)
        {
            ret_val = fsdh_get_fsdh_struct(mte->mte_dh,&dhs);

             /* Don't allow this task to be preempted while setting exclusive access and
                checking disk open count. If we didn't set to no preemption then this task
                could time slice after exclusive access is set, but before opencount is 
                checked. This would allow another task that is in NU_Open_Disk at line
                if(dhs->fsdh_exclusive_access == NU_TRUE) to be true. However if that
                task, then time sliced before decrementing the open count. This task
                could fail because open count isn't 1 and the other task would fail also
                because exclusive access is set to NU_TRUE.*/            

            old_preempt = NU_Change_Preemption(NU_NO_PREEMPT);
            dhs->fsdh_exclusive_access = NU_TRUE;         
            /* Make sure we are the only task with this drive. */
            if(ret_val == NU_SUCCESS && dhs->fsdh_opencount == 1)
            {
                NU_Change_Preemption(old_preempt);
                /* Call file system layer.*/
                ret_val = mte->mte_fs->fs_check_disk(mte->mte_dh,flag,mode);
                dhs->fsdh_exclusive_access = NU_FALSE;            
            }
            else
            {   
                dhs->fsdh_exclusive_access = NU_FALSE;
                NU_Change_Preemption(old_preempt);
                /* Return access error because only one disk should be open. */
                ret_val = NUF_ACCES;
            }        
        }
        else
            ret_val = NUF_BADDRIVE;
    }

    LCK_FS_EXIT()
    return ret_val;

}
#endif
