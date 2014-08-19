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
*       fs_fl.c
*
* COMPONENT
*
*       File
*
* DESCRIPTION
*
*       Contains user API for file operations.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Open
*       NU_Close
*       NU_Read
*       NU_Write
*       NU_Seek
*       NU_Delete
*       NU_Rename
*       NU_Truncate
*       NU_Flush
*
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/lck_extr.h"
#include "storage/fd_extr.h"
#include "storage/fsl_extr.h"
#include "storage/user_extr.h"
#include "storage/dh_extr.h"
#include "services/nu_trace_os_mark.h"

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1) 
#include "storage/bcm_extr.h"
#endif

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)
extern NU_SEMAPHORE BCM_List_Sema;    
#endif

/************************************************************************
* FUNCTION
*
*       NU_Open
*
* DESCRIPTION
*
*       Open the file for access as specified in flag. If creating, use
*       mode to set the access permissions on the file.
*
*       Flag values are
*
*       PO_RDONLY                           Open for read only.
*       PO_WRONLY                           Open for write only.
*       PO_RDWR                             Read/write access allowed.
*
*       PO_APPEND                           Seek to eof on each write.
*       PO_CREAT                            Create the file if it does
*                                            not exist.
*       PO_TRUNC                            Truncate the file if it
*                                            already exists.
*       PO_EXCL                             If flag contains
*                                            (PO_CREAT | PO_EXCL) and
*                                            the file already exists
*                                            fail and set
*                                            fs_user->p_errno to EEXIST.
*
*       PO_BINARY                           Ignored. All file access is
*                                            binary.
*       PO_TEXT                             Ignored.
*
*       PO_NOSHAREANY                       Fail if the file is already
*                                            open.
*       PO_NOSHAREWRITE                     Fail if the file is already
*                                            open for write.
*
*       Mode values are
*
*       PS_IWRITE                           Write permitted.
*       PS_IREAD                            Read permitted.
*                                           (Always true anyway)
*
*
* INPUTS
*
*       *name                               Open file name
*       flag                                Open flag
*       mode                                Open mode
*
* OUTPUTS
*
*       Returns a non-negative integer to be used as a file descriptor.
*       Returning a negative integer indicates an error as follows:
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_LONGPATH                        Path or directory name too
*                                            long.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_INVPARM                         Invalid Flag/Mode is specified.
*       NUF_INVPARCMB                       Invalid Flag/Mode combination
*       NUF_PEMFILE                         No file descriptors.
*                                            available.
*                                           (too many files open)
*       NUF_ACCES                           You can't open the file
*                                            which has Directory or
*                                            VOLLABEL attributes.
*       NUF_NOSPC                           No space to create directory
*                                            in this disk.
*       NUF_SHARE                           The access conflict from
*                                            multiple task to a specific
*                                            file.
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_EXIST                           The directory already exists.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOENT                             File not found or path to
*                                            file not found.
*       PEMFILE                             No file descriptors
*                                            available (too many files
*                                            open).
*       PEEXIST                             Exclusive access requested
*                                            but file already exists.
*       PEACCES                             Attempt to open a read only
*                                            file or a special
*                                            (directory) file.
*       PENOSPC                             Create failed.
*       PESHARE                             Already open in exclusive
*                                            mode or we want exclusive
*                                            and it's already open.
*
*************************************************************************/
INT NU_Open(CHAR *name, UINT16 flag, UINT16 mode)
{
    INT         vfs_fd;
    INT         fs_fd;
    MTE_S*      mte;
    UINT8       user_state;

    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((vfs_fd = CHECK_USER()) == NU_SUCCESS)
    {       
        /* Resolve the mount table entry from the name */
        mte = fsl_mte_from_name(name);
        if (mte)
        {
            /* Get the current user state for the disk */        
            if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {
                    fs_fd = mte->mte_fs->fs_open(mte->mte_dh, name, flag, mode);
                    /* Check if the file system returned an error */
                    if (fs_fd < 0)
                        /* Pass file system error back to user */
                        vfs_fd = fs_fd;
                    else
                    {
                        /* Allocate a file descriptor from the system table */
                        vfs_fd = fd_allocate_fd(fs_fd, mte);
                        /* Close the file if a system fd allocation failed */
                        if (vfs_fd < 0)
                            mte->mte_fs->fs_close(fs_fd);
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)                            
                        else
                        {
                            /* Increment the open file count on the mount point. 
                               This will be used to trigger a silent flush when
                               all files are closed on the mount point */
                               mte->mte_file_open_count++;   
                        }
#endif                        
                    }
                }
                else
                    vfs_fd = NUF_NOT_OPENED;
            }
            else
                vfs_fd = NUF_BAD_USER;
                
            /* Trace log */
            T_FILE_OPEN_INFO(mte->mte_mount_name, mte->mte_device_name, name, flag, mode, vfs_fd);
        }
        else
            vfs_fd = NUF_BADDRIVE;
            
        /* Trace log */
        T_FILE_OPEN_STAT(name, flag, mode, vfs_fd);
    }
    LCK_FS_EXIT()
    return(vfs_fd);
}

/************************************************************************
* FUNCTION
*
*       NU_Close
*
* DESCRIPTION
*
*       Close the file updating the disk and freeing all core associated
*       with FD.
*
*
* INPUTS
*
*       vfs_fd                              File descriptor.
*
* OUTPUTS
*
*       NU_SUCCESS                          The file was
*                                            successfully closed.
*       NUF_BAD_USER                        Not a file user.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADFILE                         Invalid file descriptor.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PEBADF                              Invalid file descriptor.
*       PENOSPC                             I/O error.
*
*************************************************************************/
STATUS NU_Close(INT vfs_fd)
{
    INT         fs_fd;
    STATUS      ret_val;
    MTE_S*      mte;
    UINT8       user_state;
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1) 
    BCM_CB      *cb;
    STATUS      local_status;
#endif        
    
    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_val = CHECK_USER()) == NU_SUCCESS)
    {
        /* Resolve the mount table entry from the fd */
        mte = fd_mte_from_fd(vfs_fd);
        if (mte)
        {
            /* Get the current user state for the disk */        
            if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {
                    /* Convert the VFS's fd to the FS's specific fd */
                    fs_fd = fd_handle_from_fd(vfs_fd);
        
                    /* Dispatch the FSs specific routine */
                    ret_val = mte->mte_fs->fs_close(fs_fd);
        
                    /* Free the system file descriptor regardless of file system
                       specific return */
                    fd_free_fd(vfs_fd);
                   
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)                            
                    /* Decrement the open file count on the mount point. 
                       This will be used to trigger a silent flush when
                       all files are closed on the mount point */
                    mte->mte_file_open_count--;
                       
                    if (mte->mte_file_open_count == 0)
                    {
                        /* If the device has a cache enabled, set an async flush event */
                        local_status = NU_Obtain_Semaphore(&BCM_List_Sema, NU_SUSPEND);
                        if (local_status == NU_SUCCESS)
                        {
                            local_status = bcm_device_has_cache(mte->mte_dh, &cb);
                            if ((local_status == NU_SUCCESS) && (cb != NU_NULL))
                            {
                                /* Get the control block semaphore while holding the list
                                   semaphore preventing the device cache from being deleted
                                   while in use */                                    
                                (VOID) NU_Obtain_Semaphore (&(cb->cb_sema),NU_SUSPEND);
                                
                                /* Return the list semaphore */
                                (VOID) NU_Release_Semaphore(&BCM_List_Sema);
                                
                                /* Set the asynchronous flush event */
                                (VOID) NU_Set_Events(&(cb->control_group), BCM_EVG_ASYNC_FLUSH, NU_OR);
                                
                                /* Return the control block semaphore */
                                (VOID) NU_Release_Semaphore(&(cb->cb_sema));
                            }
                            else
                            {
                                /* Device did not have a cache, return the list semaphore */
                                (VOID) NU_Release_Semaphore(&BCM_List_Sema);
                            }
                        }
                    }
#endif
                }
                else
                    ret_val = NUF_NOT_OPENED;
            }
            else
                ret_val = NUF_BAD_USER;
        }
        else
            /* Invalid file descriptor was given, could be closed */
            ret_val = NUF_BADFILE;
            
        /* Trace log */
        T_FILE_CLOSE_STAT(vfs_fd, ret_val);
    }
    LCK_FS_EXIT()
    return(ret_val);
}

/************************************************************************
* FUNCTION
*
*       NU_Read
*
* DESCRIPTION
*
*       Attempt to read count bytes from the current file pointer of
*       file at fd and put them in buf. The file pointer is updated.
*
*
* INPUTS
*
*       fd                                  File descriptor
*       buf                                 Buffer to read data.
*       count                               Number of read bytes.
*
* OUTPUTS
*
*       Returns the number of bytes read or negative value on error.
*
*       INT32                               A non-negative integer to be
*                                            used as a number of bytes
*                                            read.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADFILE                         Invalid file descriptor.
*       NUF_ACCES                           Open flag is  PO_WRONLY.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PEBADF                              Invalid file descriptor.
*       PENOSPC                             Create failed.
*
*************************************************************************/
INT32 NU_Read(INT fd, CHAR *buf, INT32 count)
{
    INT32       ret_val;
    MTE_S*      mte;
    UINT8       user_state;
    
    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_val = CHECK_USER()) == NU_SUCCESS)
    {
        /* Trace log */
        T_FILE_READ_START(count, fd);
        
        /* Resolve the mount table entry from the fd */
        mte = fd_mte_from_fd(fd);
        if (mte)
        {
            /* Get the current user state for the disk */        
            if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {
                    /* Dispatch the FS specific routine */
                    ret_val = mte->mte_fs->fs_read(fd_handle_from_fd(fd), buf, count);
                }
                else
                    ret_val = NUF_NOT_OPENED;
            }
            else
                ret_val = NUF_BAD_USER;
        }
        else
            /* The file descriptor is not valid. */
            ret_val = NUF_BADFILE;
            
        /* Trace log */
        T_FILE_READ_STOP(count, fd, ret_val);
    }
    LCK_FS_EXIT()
    return(ret_val);
}

/************************************************************************
* FUNCTION
*
*       NU_Write
*
* DESCRIPTION
*
*       Attempt to write count bytes from buf to the current file
*       pointer of file at fd. The file pointer is updated.
*
*
* INPUTS
*
*       fd                                  File descriptor.
*       *buf                                Pointer of buffer to write.
*       count                               Write byte count.
*
* OUTPUTS
*
*       Returns the number of bytes written or negative value on error.
*
*       INT32                               A non-negative integer to be
*                                            used as a number of bytes
*                                            written.
*
*       If the return value is negative, the meaning is follows:
*
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADFILE                         File descriptor invalid.
*       NUF_ACCES                           Not a PO_WRONLY or PO_RDWR
*                                            open flag or file
*                                            attributes is ARDONLY.
*       NUF_NOSPC                           Write failed. Presumably
*                                            because of no space.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PEBADF                              Invalid file descriptor.
*       PEACCES                             Attempt to open a read only
*                                            file or a special
*                                            (directory) file.
*       PENOSPC                             Create failed.
*
*************************************************************************/
INT32 NU_Write(INT fd, CHAR *buf, INT32 count)
{
    INT32       ret_val;
    MTE_S*      mte;
    UINT8       user_state;
    
    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_val = CHECK_USER()) == NU_SUCCESS)
    {
        /* Trace log */
        T_FILE_WRITE_START(count, fd);
        
        /* Resolve the mount table entry from the fd */
        mte = fd_mte_from_fd(fd);
        if (mte)
        {
            /* Get the current user state for the disk */        
            if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {
                    /* Dispatch the FS specific routine */
                    ret_val = mte->mte_fs->fs_write(fd_handle_from_fd(fd), buf, count);
                }
                else
                    ret_val = NUF_NOT_OPENED;
            }
            else
                ret_val = NUF_BAD_USER;            
        }            
        else
            /* The file descriptor is not valid. */
            ret_val = NUF_BADFILE;
            
        /* Trace log */
        T_FILE_WRITE_STOP(count, fd, ret_val); 
    }
    LCK_FS_EXIT()
    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       NU_Seek
*
* DESCRIPTION
*
*       Move the file pointer offset bytes from the origin described by
*       "origin". The file pointer is set according to the following rules.
*
*       Origin                                  Rule
*       PSEEK_SET                           offset from beginning of file.
*       PSEEK_CUR                           offset from current file.
*                                            pointer.
*       PSEEK_END                           offset from end of file.
*
*       Attempting to seek beyond end of file puts the file pointer one
*       byte past eof.
*
*
* INPUTS
*
*       fd                                  File descriptor.
*       offset                              Seek bytes.
*       origin                              Origin.
*
* OUTPUTS
*
*       INT32                               A non-negative integer to be
*                                            used as a number of bytes
*                                            seek.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADFILE                         File descriptor invalid.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno will be set with one of the following:
*
*       PEBADF                              File descriptor invalid.
*       PEINVAL                             Seek to negative file
*                                            pointer attempted.
*
*************************************************************************/
INT32 NU_Seek(INT fd, INT32 offset, INT16 origin)
{
    INT32       ret_val;
    MTE_S*      mte;
    UINT8       user_state;
    
    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_val = CHECK_USER()) == NU_SUCCESS)
    {    
        /* Trace log */
        T_FILE_SEEK_START(offset, origin, fd); 
        
        /* Resolve the mount table entry from the fd */
        mte = fd_mte_from_fd(fd);
        if (mte)
        {
            /* Get the current user state for the disk */        
            if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {
                    /* Dispatch the FS specific routine */
                    ret_val = mte->mte_fs->fs_seek(fd_handle_from_fd(fd), offset, origin);
                }
                else
                    ret_val = NUF_NOT_OPENED;
            }
            else
                ret_val = NUF_BAD_USER;         
        }            
        else
            /* The file descriptor is not valid. */
            ret_val = NUF_BADFILE;
            
        /* Trace log */
        T_FILE_SEEK_STOP(offset, origin, fd, ret_val); 
    }
    LCK_FS_EXIT()
    return(ret_val);
}

/************************************************************************
* FUNCTION
*
*       NU_Delete
*
* DESCRIPTION
*
*       Delete the file in "name". Fail if not a simple file, if it
*       is open, does not exist, or is read only.
*
*
* INPUTS
*
*       *name                               File name to be deleted.
*
* OUTPUTS
*
*       NU_SUCCESS                          Delete request completed
*                                            successfully.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_ACCES                           This file has at least one
*                                            of the following attributes:
*                                            RDONLY,HIDDEN,SYSTEM,VOLUME,
*                                            DIRENT.
*       NUF_SHARE                           The access conflict from
*                                            multiple task to a specific
*                                            file.
*       NUF_NOFILE                          The specified file not found.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O function routine
*       NUF_INTERNAL                        Nucleus FILE internal error.
*                                            returned error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOENT                             File not found or path to
*                                            file not found.
*       PEACCES                             Attempt delete a directory
*                                            or an open file.
*       PENOSPC                             Write failed.
*
*************************************************************************/
STATUS NU_Delete(CHAR *name)
{
    STATUS      ret_stat;
    MTE_S*      mte;
    UINT8       user_state;
    
    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        /* Resolve the mount table entry from the name */
        mte = fsl_mte_from_name(name);
        if (mte)
        {
            /* Get the current user state for the disk */        
            if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {
        
                    /* Dispatch the FS specific routine */
                    ret_stat = mte->mte_fs->fs_delete(name);
                }
                else
                    ret_stat = NUF_NOT_OPENED;
            }
            else
                ret_stat = NUF_BAD_USER;
        }
        else
            ret_stat = NUF_BADDRIVE;
            
        /* Trace log */
        T_FILE_DEL_STAT(name, ret_stat);
    }
    LCK_FS_EXIT()
    return(ret_stat);
}

/************************************************************************
* FUNCTION
*
*       NU_Rename
*
* DESCRIPTION
*
*       Renames the file in path ("name") to "newname". Fails if "name" is
*       invalid, "newname" already exists or path not found. Does not test
*       if "name" is a simple file. It is possible to rename directories.
*       (This may change in the multiuser version)
*
*
* INPUTS
*
*       *name                               Old file name
*       *newname                            New file name(Rename)
*
* OUTPUTS
*
*       NU_SUCCESS                          File was successfully renamed.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_ROOT_FULL                       Root directly full.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_ACCES                           You can't change the file
*                                            which has VOLLABEL, HIDDEN,
*                                            or SYSTEM attributes.
*       NUF_NOSPC                           No space to create directory
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOENT                             File not found or path to
*                                            file not found.
*       PEACCES                             Attempt to open a read only
*                                            file or a special
*                                            (directory) file.
*       PEEXIST                             Exclusive access requested
*                                            but file already exists.
*
*************************************************************************/
#if (FILE_VERSION_COMP == FILE_2_5)
INT NU_Rename(CHAR *name, CHAR *newname)
#endif
#if (FILE_VERSION_COMP > FILE_2_5)
STATUS NU_Rename(CHAR *name, CHAR *newname)
#endif
{
    STATUS      ret_val;
    MTE_S*      mte;
    UINT8       user_state;
    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_val = CHECK_USER()) == NU_SUCCESS)
    {
        /* Verify that name and newname don't point to NU_NULL. */
        if((name != NU_NULL) && (newname != NU_NULL))
        {
            /* Resolve the mount table entry from the name */
            mte = fsl_mte_from_name(name);
            if (mte)
            {
                /* Get the current user state for the disk */        
                if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
                {
                    /* Verify disk is opened */
                    if (user_state == FSDH_USER_OPENED)
                    {
                        /* Dispatch the FS specific routine */
                        ret_val = mte->mte_fs->fs_rename(name, newname);
                    }
                    else
                        ret_val = NUF_NOT_OPENED;
                }
                else
                    ret_val = NUF_BAD_USER;
            }
            else
                ret_val = NUF_BADDRIVE;
        }
        else
        {
            ret_val = NUF_BADPARM;
        }
        
        /* Trace log */
        T_FILE_RENAME_STAT(name, newname, ret_val);
    }
    LCK_FS_EXIT()
    return(ret_val);
}

/************************************************************************
* FUNCTION
*
*       NU_Truncate
*
* DESCRIPTION
*
*       Move the file pointer offset bytes from the beginning of the
*       file and truncate the file beyond that point by adjusting the
*       file size and freeing the cluster chain past the file pointer.
*
*
* INPUTS
*
*       fd                                  File descriptor.
*       offset                              Truncate offset(bytes).
*
* OUTPUTS
*
*       NU_SUCCESS                          The file was
*                                            successfully truncated.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADFILE                         File descriptor invalid.
*       NUF_ACCES                           You can't change the file
*                                            which has PO_RDONLY or file
*                                            attributes is ARDONLY.
*       NUF_SHARE                           The access conflict from
*                                            multiple task to a specific
*                                            file.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno will be set with one of the following:
*
*       PEBADF                              File descriptor invalid or
*                                            open read only.
*       PENOSPC                             I/O error.
*       PEINVAL                             Invalid offset.
*       PESHARE                             Can not truncate a file open
*                                            by more than one handle.
*
*************************************************************************/
STATUS NU_Truncate(INT fd, INT32 offset)
{
    STATUS      ret_stat;
    MTE_S*      mte;
    UINT8       user_state;
    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        /* Trace log */
        T_FILE_TRUNC_START(offset, fd);
        
        /* Resolve the mount table entry from the fd */
        mte = fd_mte_from_fd(fd);
        if (mte)
        {
            /* Get the current user state for the disk */        
            if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {
                
                    /* Dispatch the FS specific routine */
                    ret_stat = mte->mte_fs->fs_truncate(fd_handle_from_fd(fd), offset);
                }
                else
                    ret_stat = NUF_NOT_OPENED;
            }
            else
                ret_stat = NUF_BAD_USER;
        }
        else
            /* The file descriptor is not valid. */
            ret_stat = NUF_BADFILE;
            
        /* Trace log */
        T_FILE_TRUNC_STOP(offset, fd, ret_stat);
    }
    LCK_FS_EXIT()
    return(ret_stat);
}

/************************************************************************
* FUNCTION
*
*       NU_Flush
*
* DESCRIPTION
*
*       Flush the file, updating the disk.
*
*
* INPUTS
*
*       fd                                  File descriptor.
*
* OUTPUTS
*
*       NU_SUCCESS                          The file was
*                                            successfully flushed.
*       NUF_BAD_USER                        Not a file user.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADFILE                         Invalid file descriptor.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PEBADF                              Invalid file descriptor.
*       PENOSPC                             I/O error.
*
*************************************************************************/
STATUS NU_Flush(INT fd)
{
    STATUS      ret_val;
    MTE_S*      mte;
    UINT8       user_state;
    
    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_val = CHECK_USER()) == NU_SUCCESS)
    {
        /* Trace log */
        T_FILE_FLUSH_START(fd);
        
        /* Resolve the mount table entry from the fd */
        mte = fd_mte_from_fd(fd);
        if (mte)
        {
            /* Get the current user state for the disk */        
            if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {
                
                    /* Dispatch the FS specific routine */
                    ret_val = mte->mte_fs->fs_flush(fd_handle_from_fd(fd));
                }
                else
                    ret_val = NUF_NOT_OPENED;
            }
            else
                ret_val = NUF_BAD_USER;
        }            
        else
            /* The file descriptor is not valid. */
            ret_val = NUF_BADFILE;
         
        /* Trace log */
        T_FILE_FLUSH_STOP(fd, ret_val);
    }
    LCK_FS_EXIT()
    return(ret_val);
}

