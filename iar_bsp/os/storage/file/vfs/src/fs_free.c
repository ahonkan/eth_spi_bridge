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
*       fs_free.c
*
* COMPONENT
*
*       Drive Operation
*
* DESCRIPTION
*
*       Contains API for describing available space on a storage device.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_FreeSpace
*
*************************************************************************/
#include "nucleus.h"

#include "storage/pcdisk.h"
#include "storage/lck_extr.h"
#include "storage/fsl_extr.h"
#include "storage/user_extr.h"
#include "storage/fs_extr.h"
#include "storage/dh_extr.h"
/************************************************************************
* FUNCTION
*
*       NU_FreeSpace
*
* DESCRIPTION
*
*       Given a path containing a valid drive specifier count the number
*       of free clusters, sector per cluster, bytes per sector and number
*       of total clusters on the drive.
*
*
* INPUTS
*
*       path                                Drive character
*       secpcluster                         Sector per cluster
*       bytepsec                            Bytes per sector
*       freecluster                         Number of free clusters
*       totalcluster                        Number of total clusters
*
* OUTPUTS
*
*       NU_SUCCESS                          If service is successful.
*                                           Returns the sector per
*                                            cluster, bytes per sector,
*                                            number of free clusters and
*                                            number of total clusters.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      Drive not opened.
*       NUF_NO_DISK                         Disk is removed.
*
*************************************************************************/
STATUS NU_FreeSpace(CHAR *path, UINT8 *secpcluster, UINT16 *bytepsec,
                     UINT32 *freecluster, UINT32 *totalcluster)
{
STATUS      ret_stat;
MTE_S*      mte;
UINT8       user_state;

    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        /* Resolve the mount table entry using the path */
        mte = fsl_mte_from_fqpath(path);
        if (mte)
        {
            /* Get the current user state for the disk */        
            if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {
                
                    /* Dispatch the FS specific routine */
                    ret_stat = mte->mte_fs->fs_freespace(mte->mte_dh, secpcluster, bytepsec,
                                                        freecluster, totalcluster);
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
    LCK_FS_EXIT()
    return(ret_stat);
}

