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
*       fd.c
*                                                                       
* COMPONENT                                                             
*                                                                       
*       File Descriptor
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       File descriptor services.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None                                                           
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       fd_init_fd_table
*       fd_allocate_fd
*       fd_free_fd
*       fd_free_fd_for_dh
*       fd_mte_from_fd
*       fd_handle_from_fd
*       fd_get_fs_fd
*                                                                       
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/fd_defs.h"
#include "storage/lck_extr.h"

FD_S *FD_Table;

/* Global maximum number of open files. Set from registry option. */
UINT16 gl_VFS_MAX_OPEN_FILES = 0;

/* Max number of file descriptors */
UINT16 gl_FD_MAX_FD;

/************************************************************************
* FUNCTION                                                              
*     
*   fd_init_fd_table   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Initialize the file descriptor table.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*   None
*                                                                         
* OUTPUTS                                                               
*       
*   STATUS                      NU_SUCCESS or NUF_NO_MEMORY
*   
*                                                                       
*************************************************************************/
STATUS fd_init_fd_table(VOID)
{
INT     idx;

    STATUS ret_stat = NU_SUCCESS;

    FD_Table = NUF_Alloc((sizeof(FD_S) * gl_FD_MAX_FD));
    if (FD_Table != NU_NULL)
    {
        for (idx = 0; idx < gl_FD_MAX_FD; idx++)
        {
            /* Zero/Init the descriptor table entry */
            FD_Table[idx].fd_flags = 0;
            FD_Table[idx].fd_fs_handle = -1;
            FD_Table[idx].fd_mte = NU_NULL;
        }
    }
    else
    {
        ret_stat = NUF_NO_MEMORY;
    }

   return (ret_stat);


}

/************************************************************************
* FUNCTION                                                              
*     
*   fd_allocate_fd   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Allocates a file descriptor to for mapping to file system specific
*   file descriptors.   
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*   *mte                        Mount table entry that uses this fd
*   handle                      File system handle for assigning to
*                               an fd.
*                                                                         
* OUTPUTS                                                               
*       
*   INT                         >=0 the allocation was successful
*                               descriptor returned is used as the fd
*   NUF_PEMFILE                 No file descriptor available.
*   NUF_BADPARM                 Invalid parameter.
*                                                                       
*************************************************************************/
INT fd_allocate_fd(INT handle, MTE_S *mte)
{
FD_S *fd;
INT ret_val;
INT idx;

    fd = NU_NULL;
    ret_val = NUF_PEMFILE;

    if (!mte)
        ret_val = NUF_BADPARM;
    else
    {
        /* Lock the FD table */
        LCK_ENTER(LCK_FD_TABLE)

        /* Find an empty location in the file descriptor table */
        for (idx = 0; idx<gl_FD_MAX_FD; idx++)
        {
            /* Search for an invalid/empty entry */
            if ( ! (FD_Table[idx].fd_flags & FD_FL_VALID) )
            {
                fd = &FD_Table[idx];
                ret_val = idx;
                break;
            }

        }
        /* If a fd was allocated, setup the entry */
        if (fd)
        {
            fd->fd_fs_handle = handle;
            fd->fd_mte = mte;
            fd->fd_drive_id = mte->mte_drive_id;
            fd->fd_flags |= FD_FL_VALID;
        }

        /* Unlock the FD table */
        LCK_EXIT(LCK_FD_TABLE)

    }

    return (ret_val);
}

/************************************************************************
* FUNCTION                                                              
*     
*   fd_free_fd   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Returns a file descriptor to the empty state.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*   fd                          File descriptor to free.
*                                                                         
* OUTPUTS                                                               
*       
*   None
*                                                                       
*************************************************************************/
VOID fd_free_fd(INT fd)
{
FD_S *pfd;
    

    if ((fd >= 0) && (fd < gl_FD_MAX_FD) )
    {
        /* Lock the table */
        LCK_ENTER(LCK_FD_TABLE)

        /* Get the fd structure from the table */
        pfd = &FD_Table[fd];

        /* Clear the contents of the fd entry*/
        pfd->fd_fs_handle = -1;
        pfd->fd_mte = NU_NULL;
        pfd->fd_drive_id = NU_NULL;
        pfd->fd_flags &= ~FD_FL_VALID;

        /* Unlock FD table */
        LCK_EXIT(LCK_FD_TABLE)
    }
}

/************************************************************************
* FUNCTION                                                              
*     
*   fd_free_fd_for_dh   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Frees all the VFS file descriptors associated with the passed
*   in dh.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*   dh                                  Disk handle.   
*                                                                             
* OUTPUTS                                                               
*       
*
*   NU_SUCCESS                          Should always return success.
*                                                                       
*************************************************************************/
STATUS  fd_free_fd_for_dh(INT dh)
{
STATUS ret_val = NU_SUCCESS;
FD_S *pfd;
INT vfs_fd;

    LCK_ENTER(LCK_FD_TABLE)
    for(vfs_fd = 0; vfs_fd < gl_FD_MAX_FD; vfs_fd++)
    {
        /* Get the fd structure from the table */
        pfd = &FD_Table[vfs_fd];

        if(pfd->fd_flags & FD_FL_VALID)
        {
            /* Verify that fd_mte is valid. */
            if(pfd->fd_mte)
            {
                if(pfd->fd_mte->mte_dh == dh)
                {
                    /* Clear the contents of the fd entry*/
                    pfd->fd_fs_handle = -1;
                    pfd->fd_mte = NU_NULL;
                    pfd->fd_drive_id = NU_NULL;
                    pfd->fd_flags &= ~FD_FL_VALID;
                }               
            }            
        }        
    }

    LCK_EXIT(LCK_FD_TABLE)
    
    return ret_val;

}

/************************************************************************
* FUNCTION                                                              
*     
*   fd_mte_from_fd
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Returns a mount table entry from a file descriptor.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*   fd                          File descriptor used to lookup MTE
*                                                                         
* OUTPUTS                                                               
*       
*   MTE_S*                      Mount table entry associated with input 
*                               handle
*                                                                       
*************************************************************************/
MTE_S*  fd_mte_from_fd(INT fd)
{
MTE_S*  mte = NU_NULL;

    if ( (fd >= 0) && (fd < gl_FD_MAX_FD) )
    {
        /* Lock the table */
        LCK_ENTER(LCK_FD_TABLE)

        mte = FD_Table[fd].fd_mte;

        /* Verify the mte assigned for this file descriptor is valid */
        if ( (mte) && (FD_Table[fd].fd_drive_id != FD_Table[fd].fd_mte->mte_drive_id) )
            mte = NU_NULL;

        /* Unlock the table */
        LCK_EXIT(LCK_FD_TABLE)
    }

    return (mte);

}

/************************************************************************
* FUNCTION                                                              
*     
*   fd_handle_from_fd
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Returns file system handle from a file descriptor.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*   fd                          File descriptor used to find associate
*                               file system handle
*                                                                         
* OUTPUTS                                                               
*       
*   INT                         File System handle associated with fd
*                                                                       
*************************************************************************/
INT fd_handle_from_fd(INT fd)
{
INT handle = NUF_BADPARM;

    if ((fd >= 0) && (fd < gl_FD_MAX_FD) )
    {
        /* Lock the table */
        LCK_ENTER(LCK_FD_TABLE)
    
        /* Get the handle */
        handle = FD_Table[fd].fd_fs_handle;
    
        /* Unlock the table */
        LCK_EXIT(LCK_FD_TABLE)
    }

    return (handle);
}

/************************************************************************
* FUNCTION                                                              
*     
*   fd_get_fs_fd   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Gets the file system descriptor based on the VFS file
*   descriptor passed in and disk handle that are passed in.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*   vfs_fd                              VFS File descriptor.
*   dh                                  Disk handle.   
*                                                                             
* OUTPUTS                                                               
*       
*   fd                                  The file system descriptor that
*                                       corresponds to the VFS file
*                                       descriptor.
*   
*   NU_SUCCESS                          If a FD_Table entry is valid
*                                       and matches the passed in disk
*                                       handle.
*   NUF_BADPARAM                        If a FD_Table entry is invalid
*                                       or doesn't match the passed in 
*                                       disk handle.
*                                                                       
*************************************************************************/
STATUS  fd_get_fs_fd(INT vfs_fd,INT dh, INT *fd)
{
    STATUS ret_val = NU_SUCCESS;

    LCK_ENTER(LCK_FD_TABLE)

        if ((vfs_fd >= 0) && (vfs_fd < gl_FD_MAX_FD) )
        {
            if(FD_Table[vfs_fd].fd_flags & FD_FL_VALID)
            {
                /* Verify that fd_mte is valid. */
                if(FD_Table[vfs_fd].fd_mte)
                {
                    if(FD_Table[vfs_fd].fd_mte->mte_dh == dh)
                    {
                        *fd = FD_Table[vfs_fd].fd_fs_handle;
                    }
                    else
                        ret_val = NUF_BADPARM;
                }
                else
                    ret_val = NUF_BADPARM;            
            }
            else
                ret_val = NUF_BADPARM;            
        }
        else
            ret_val = NUF_BADPARM;
        
    LCK_EXIT(LCK_FD_TABLE)
    
    return ret_val;

}
