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
*       vnode.c
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*       VNODE routines for FAT file system
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       fat_allocate_fsnode
*       fat_deallocate_fsnode
*       fat_fsnode_to_string
*
*************************************************************************/
#include        "storage/fat_defs.h"

extern UINT32 FILE_Unused_Param; /* Used to prevent compiler warnings */
/************************************************************************
* FUNCTION
*
*   fat_allocate_fsnode
*
* DESCRIPTION
*
*   Allocate a FS specific node for the file/directory in the given path
*
* INPUTS
*
*   dh                              Disk handle
*   *path                           String containing path of object
*   **fsnode                        FS specific node
*
* OUTPUTS
*
*   NU_SUCCESS
*
*
*************************************************************************/
STATUS fat_allocate_fsnode(UINT16 dh, CHAR *path, VOID **fsnode)
{
STATUS ret_stat;
FAT_CB *fs_cb;
DROBJ  *parent_obj;
DROBJ  *pobj;
VOID   *path2;
VOID   *filename;
VOID   *fileext;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Check path - Wildcards not allowed */
    ret_stat = pc_use_wdcard((UINT8 *)path);
    if(ret_stat == YES)
    {
        ret_stat = NUF_INVNAME;
    }
    else
        ret_stat = NU_SUCCESS;

    /* Get the fsdh structure for device driver calls */
    if (ret_stat == NU_SUCCESS)
    {
        /* Get the FAT control block */
         ret_stat = fsdh_get_fs_specific(dh, (VOID**)&fs_cb);
    }


    /* Verify that the drive exists */
    if (ret_stat == NU_SUCCESS)
    {
        if (fs_dev_dskchk_proc(dh) != NU_SUCCESS)
            ret_stat = NUF_NO_DISK;
    }

    /* Init for the search */
    if (ret_stat == NU_SUCCESS)
    {
        parent_obj = NU_NULL;
        pobj = NU_NULL;
        fsu_set_user_error(0);

        /* Get the path, file name, and extension */
        ret_stat = pc_parsepath(&path2, &filename, &fileext, (UINT8 *)path);
        if (ret_stat != NU_SUCCESS)
        {
            fsu_set_user_error(PENOENT);
        }
        else
        {
            /* Register drive in use. */
            PC_DRIVE_ENTER(dh, NO)

            /* Find the parent and make sure it is a directory  */
            ret_stat = pc_fndnode(dh, &parent_obj, (UINT8 *)path2);
            if (ret_stat != NU_SUCCESS)
            {
                fsu_set_user_error(PENOENT);
            }
            else if ( (!pc_isadir(parent_obj)) || (pc_isavol(parent_obj)) )
            {
                fsu_set_user_error(PENOENT);
                ret_stat = NUF_ACCES;
            }
            else
            {
                /* Get the directory */
                if ( filename == (VOID *)'\0' || filename == (VOID *)' ' )
                {
                    pobj = parent_obj;
                }
                else
                {
                    /* Lock the parent */
                    PC_INODE_ENTER(parent_obj->finode, YES)
                    /* Find the file and init the structure. */
                    pobj = NU_NULL;
                    ret_stat = pc_get_inode(&pobj, parent_obj, (UINT8 *)filename);
                    /* Release exclusive use of finode. */
                    PC_INODE_EXIT(parent_obj->finode)
                    if (ret_stat != NU_SUCCESS)
                    {
                        /* Free the parent object. */
                        pc_freeobj(parent_obj);
                        fsu_set_user_error(PENOENT);
                    }
                    /* If the request is cd .. then we just found the .. directory entry.
                       We have to call get_mom to access the parent. */
                    else if ( pc_isdotdot((UINT8 *)filename, (UINT8 *)fileext) )
                    {
                        /* Free the parent object. */
                        pc_freeobj(parent_obj);
                        parent_obj = pobj;

                        /* Find parent_obj's parent. By looking back from ".." */
                        ret_stat = pc_get_mom(&pobj, parent_obj);

                        /* Free the parent object. */
                        pc_freeobj(parent_obj);
                    }
                    /* Specified path is not a directory */
                    else if ( !pc_isadir(pobj) )
                    {
                        /* Free the current and parent object. */
                        pc_freeobj(pobj);
                        pc_freeobj(parent_obj);
                        ret_stat = NUF_ACCES;
                    }
                    else
                    {
                        /* Free the parent object */
                        pc_freeobj(parent_obj);
                    }
                }
                if (ret_stat == NU_SUCCESS) /* Everything is fine. */
                {
                    /* Set the return value for the allocated DROBJ */
                    *fsnode = pobj;
                }
            }

            /* Release non-exclusive use of drive. */
            PC_DRIVE_EXIT(dh)
        }


    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return (ret_stat);
}

/************************************************************************
* FUNCTION
*
*   fat_deallocate_fsnode
*
* DESCRIPTION
*
*   Deallocate a FS specific node
*
* INPUTS
*
*   dh                              Disk handle
*   *fsnode                         FS node to deallocate
*
* OUTPUTS
*
*   NU_SUCCESS
*
*
*************************************************************************/
STATUS fat_deallocate_fsnode(UINT16 dh, VOID *fsnode)
{
STATUS ret_stat;
FAT_CB *fs_cb;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Get the FAT control block */
    ret_stat = fsdh_get_fs_specific(dh, (VOID**)&fs_cb);

    if (ret_stat == NU_SUCCESS)
    {
        /* Register drive in use. */
        PC_DRIVE_ENTER(dh, NO)

        pc_freeobj((DROBJ *)fsnode);

        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(dh)
    }
    else
        ret_stat = NUF_INTERNAL;

    /* Restore the kernel state */
    PC_FS_EXIT()

    return (ret_stat);
}
/************************************************************************
* FUNCTION
*
*   fat_fsnode_to_string
*
* DESCRIPTION
*
*   Convert a FS specific node to a string
*
* INPUTS
*
*   dh                              Disk handle
*   *fsnode                         FS node of object to convert
*   *path                           Location to output string
*
* OUTPUTS
*
*   NU_SUCCESS
*
*
*************************************************************************/
STATUS fat_fsnode_to_string(UINT16 dh, VOID *fsnode, CHAR *path)
{
STATUS      ret_stat;
DDRIVE      *pdrive;
DROBJ       *pobj;
FAT_CB      *fs_cb;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Remove compiler warning. */
    FILE_Unused_Param = (UINT32)fsnode;
    /* Get the FAT control block */
    ret_stat = fsdh_get_fs_specific(dh, (VOID**)&fs_cb);

    if (ret_stat == NU_SUCCESS)
    {

        /* Lock the drive since we work upwards in the directory tree */
        PC_DRIVE_ENTER(dh, YES)

        /* Find the drive. */
        pdrive = fs_cb->ddrive;
        if (pdrive)
        {
            /* Get current directory. */
            pobj = pc_get_cwd(pdrive);
            if (pobj)
            {
                /* Get the full path name. */
                if ( !pc_l_pwd((UINT8 *)path, pobj) )
                    ret_stat = NUF_INVNAME;
            }
        }
        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(dh)
    }
    else
        ret_stat = NUF_INTERNAL;

    /* Restore the kernel state */
    PC_FS_EXIT()

    return (ret_stat);
}
