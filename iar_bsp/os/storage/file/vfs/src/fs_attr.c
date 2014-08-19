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
*
* FILE NAME
*
*       fs_attr.c
*
* COMPONENT
*
*       Attributes
*
* DESCRIPTION
*
*       File attribute services.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Set_Attributes
*       NU_Get_Attributes
*
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/lck_extr.h"
#include "storage/fsl_extr.h"
#include "storage/user_extr.h"
#include "storage/dh_extr.h"
/************************************************************************
* FUNCTION
*
*       NU_Set_Attributes
*
* DESCRIPTION
*
*       Set a file attributes.
*
*
* INPUTS
*
*       *name                               File name
*       newattr                             New file attribute
*
* OUTPUTS
*
*       NU_SUCCESS                          The attributes were
*                                            set successfully.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_ACCES                           You can't change VOLLABELs.
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
*       PENOSPC                             I/O error.
*
*************************************************************************/
STATUS NU_Set_Attributes(CHAR *name, UINT8 newattr)
{
    STATUS      ret_stat;
    MTE_S*      mte;
    UINT8       user_state;

    LCK_FS_ENTER()

    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {        
        /* Verify parameters aren't NU_NULL. */
        if(name != NU_NULL)
        {
            /* Convert input name to a mount table entry */
            mte = fsl_mte_from_name(name);

            /* Dispatch the file system routine for the requested
               input name */
            if (mte)
            {
                /* Get the current user state for the disk */        
                if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
                {
                    /* Verify disk is opened */
                    if (user_state == FSDH_USER_OPENED)
                    {
                        ret_stat = mte->mte_fs->fs_set_attr(name, newattr);
                    }
                    else
                        ret_stat = NUF_NOT_OPENED;
                }
                else
                    ret_stat = NUF_BAD_USER;
            }
            else
                ret_stat = NUF_BADDRIVE;
        }
        else
        {
            ret_stat = NUF_BADPARM;
        }
    }
    LCK_FS_EXIT()
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       NU_Get_Attributes
*
* DESCRIPTION
*
*       Get a file's attributes.
*
*
* INPUTS
*
*       attr                                Attribute
*       name                                File name
*
* OUTPUTS
*
*       File attributes (returned by reference in "attr").
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_ACCES                           You can't get attributes the
*                                            file which has "." or "..".
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOENT                             File not found or path to
*                                            file not found.
*
*************************************************************************/
STATUS NU_Get_Attributes(UINT8 *attr, CHAR *name)
{
    STATUS      ret_stat;
    MTE_S*      mte;
    UINT8       user_state;

    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        /* Verify parameters aren't NU_NULL. */
        if((attr != NU_NULL) && (name != NU_NULL))
        {
            /* Convert input name to a mount table entry */
            mte = fsl_mte_from_name(name);

            /* Dispatch the file system routine for the requested
               input name */
            if(mte)
            {
                /* Get the current user state for the disk */        
                if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
                {
                    /* Verify disk is opened */
                    if (user_state == FSDH_USER_OPENED)
                    {
                        ret_stat = mte->mte_fs->fs_get_attr(attr, name);
                    }
                    else
                        ret_stat = NUF_NOT_OPENED;
                }
                else
                    ret_stat = NUF_BAD_USER;
            }
            else
                ret_stat = NUF_BADDRIVE;
        }
        else
        {
            ret_stat = NUF_BADPARM;
        }
    }
    LCK_FS_EXIT()
    return(ret_stat);
}

