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
*       fs_env.c
*
* COMPONENT
*
*       Environment
*
* DESCRIPTION
*
*       Contains user environment API services.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Set_Current_Dir
*       NU_Current_Dir
*       NU_Set_Default_Drive
*       NU_Get_Default_Drive
*
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/lck_extr.h"
#include "storage/vnode_defs.h"
#include "storage/dh_extr.h"
#include "storage/fsl_extr.h"
#include "storage/user_extr.h"

/************************************************************************
* FUNCTION
*
*       NU_Set_Current_Dir
*
* DESCRIPTION
*
*       Find path. If it is a subdirectory make it the current working
*       directory for the drive.
*
*
* INPUTS
*
*       *name                               Set directory name
*
* OUTPUTS
*
*       NU_SUCCESS                          If the current working
*                                            directory was changed.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_LONGPATH                        Path or directory name too
*                                            long.
*       NUF_ACCES                           Not a directory attributes.
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O function routine
*                                            returned error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOENT                             File not found or path to
*                                            file not found.
*
*************************************************************************/
STATUS NU_Set_Current_Dir(CHAR *path)
{
STATUS  ret_stat;
MTE_S*   mte;
FSDH_S  *dhs = NU_NULL;
VNODE   *vnode;
UINT32  idx = 0;
VOID    *ptr = NU_NULL;
UINT8   user_state;

    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        /* Resolve the mount table entry from the name */
        mte = fsl_mte_from_name(path);
        if (!mte)
            ret_stat = NUF_BADDRIVE;
        else
        {
            /* Lookup the current user */
            idx = fsu_get_user_index();

            /* Get the current user state for the disk */
            if ( fsdh_get_user_state(mte->mte_dh, idx, &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {
                    /* Allocate a FS vnode for the requested path */
                    ret_stat = mte->mte_fs->fs_vnode_allocate(mte->mte_dh, path, &ptr);

                    if (ret_stat == NU_SUCCESS)
                    {

                       /* Get the fsdh struct for this disk */
                       ret_stat = fsdh_get_fsdh_struct(mte->mte_dh, &dhs);
                       if ((ret_stat == NU_SUCCESS) && (dhs->fsdh_cwd_vnodes == NU_NULL))
                       {
                                /* Something is wrong, vnode should be initialized. */
                                ret_stat = NUF_INTERNAL;

                                /* Return the FS vnode as there was an internal error */
                                mte->mte_fs->fs_vnode_deallocate(mte->mte_dh, ptr);
                        }
                    }
                }
                else
                    ret_stat = NUF_NOT_OPENED;
            }
            else
                ret_stat = NUF_BAD_USER;
        }

        /* Deallocate the user's CWD vnode */
        if ((ret_stat == NU_SUCCESS) && dhs)
        {
            vnode = (VNODE*) &dhs->fsdh_cwd_vnodes[idx];

            /* Verify we have something to dealloc */
            if ((vnode->vnode_fsnode) && mte)
                mte->mte_fs->fs_vnode_deallocate(mte->mte_dh, vnode->vnode_fsnode);
            /* vnode->vnode_fsnode may not be set if this is the first time
               a CWD is assigned to the user. */

            /* Setup the new CWD for the user */
            if (ptr)
                vnode->vnode_fsnode = ptr;
        }
    }
    LCK_FS_EXIT()
    return(ret_stat);
}

/************************************************************************
* FUNCTION
*
*       NU_Current_Dir
*
* DESCRIPTION
*
*       Fill in a string with the full path name of the current working
*       directory. Return NO if something went wrong.
*
*
* INPUTS
*
*       *drive                              Drive character
*       *path                               Current directory pointer
*
* OUTPUTS
*
*       NU_SUCCESS                          Returns the path name in
*                                            "path".
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_INVNAME                         Disk not opened yet.
*
*************************************************************************/
STATUS NU_Current_Dir(UINT8 *drive, CHAR *path)
{
STATUS  ret_stat;
MTE_S*  mte;
FSDH_S  *dhs;
VNODE   *vnode;
UINT32  idx;
UINT8   user_state;

    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        /* Resolve the mount table entry from the name */
        mte = fsl_mte_from_fqpath((CHAR*)drive);
        if (!mte)
            ret_stat = NUF_BADDRIVE;
        else
        {
            /* Lookup the current user */
            idx = fsu_get_user_index();

            /* Get the current user state for the disk */
            if ( fsdh_get_user_state(mte->mte_dh, idx, &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {
                    /* Get the fsdh struct for this disk */
                    ret_stat = fsdh_get_fsdh_struct(mte->mte_dh, &dhs);
                    if (ret_stat == NU_SUCCESS)
                    {
                        if (dhs->fsdh_cwd_vnodes == NU_NULL)
                        {
                            /* Something is wrong, vnode should be initialized. */
                            ret_stat = NUF_INTERNAL;
                        }
                        else
                        {
                            /* Get the vnode for this user */
                            vnode = (VNODE*) &dhs->fsdh_cwd_vnodes[idx];

                            /* Dispatch the FS specific convert routine */
                            ret_stat = mte->mte_fs->fs_fsnode_to_string(mte->mte_dh, vnode->vnode_fsnode, path);
                        }
                    }
                }
                else
                    ret_stat = NUF_NOT_OPENED;
            }
            else
                ret_stat = NUF_BAD_USER;

        }
    }
    LCK_FS_EXIT()
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       NU_Set_Default_Drive
*
* DESCRIPTION
*
*       Use this function to set the current default drive that will be
*       used when a path specifier does not contain a drive specifier.
*       Note: The "default" default is zero (drive A:)
*
*
* INPUTS
*
*       driveno                              Drive number
*
* OUTPUTS
*
*       NU_SUCCESS                          Setting the current default
*                                            drive was successful.
*       NUF_BAD_USER                        Not a file user.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADDRIVE                        If the drive is out of range.
*
*************************************************************************/
STATUS NU_Set_Default_Drive(UINT16 driveno)
{
    STATUS      ret_stat;
    MTE_S*      mte;
    UINT8       user_state;
    CHAR        name[] = "a:";

    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        /* Start by assuming success */
        ret_stat = NU_SUCCESS;

        /* Build a drive name from a drive number */
        name[0] = name[0] + (UINT8)driveno;

        /* Resolve the mount table entry from the name */
        mte = fsl_mte_from_fqpath(name);
        if (mte)
        {
            /* Get the current user state for the disk */
            if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {

                    /* Set the user's default drive */
                    fs_user->dfltdrv = mte->mte_drive;
                }
                else
                    ret_stat = NUF_NOT_OPENED;
            }
            else
                ret_stat = NUF_BAD_USER;
        }
        else
            /* The driveno was not valid */
            ret_stat = NUF_BADDRIVE;
    }
    LCK_FS_EXIT()
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       NU_Get_Default_Drive
*
* DESCRIPTION
*
*       Use this function to get the current default drive when a path
*       specifier does not contain a drive specifier.
*
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       Return the current default drive.
*
*************************************************************************/
INT16 NU_Get_Default_Drive(VOID)
{
INT16 ret_val;

    LCK_FS_ENTER()

    /* Get the user's default drive */
    ret_val = fs_user->dfltdrv;

    LCK_FS_EXIT()

    return(ret_val);
}

