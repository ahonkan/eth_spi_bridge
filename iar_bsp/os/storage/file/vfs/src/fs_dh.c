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
*       fs_dh.c
*
* COMPONENT
*
*       Disk handle
*
* DESCRIPTION
*
*       Disk handle services.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       fsdh_init_handles
*       fsdh_allocate_dh
*       fsdh_free_dh
*       fsdh_set_dh_specific
*       fsdh_get_dh_specific
*       fsdh_get_fsdh_struct
*       fsdh_set_fs_specific
*       fsdh_get_fs_specific
*       fsdh_dh_to_devname
*       fsdh_devname_to_dh
*       fsdh_set_semaphore
*       fsdh_get_semaphore
*       fsdh_set_user_state
*       fsdh_get_user_state
*
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/dh_defs.h"
#include "storage/lck_extr.h"
#include "storage/util_extr.h"

FSDH_S  FS_Disk_Handles[FSDH_MAX_DISKS];

/* Min disk id value is 1. */
UINT8 g_disk_id = 0;

/************************************************************************
* FUNCTION
*
*   fsdh_init_handles
*
* DESCRIPTION
*
*   Initialize the disk handle table.
*
* INPUTS
*
*   None
*
* OUTPUTS
*
*   NU_SUCCESS
*
*************************************************************************/
STATUS fsdh_init_handles()
{
UINT16  idx;
static  INT32 fsdh_run_once = 0;
STATUS  sts;

    /* Verify the handles have not been initialized */
    if (fsdh_run_once)
        sts = NU_SUCCESS;
    else
    {
        for (idx = 0; idx < FSDH_MAX_DISKS; idx++)
        {
            NUF_Memfill(&FS_Disk_Handles[idx], sizeof(FSDH_S), 0);
        }
        fsdh_run_once = 1;
        sts = NU_SUCCESS;
    }

    return (sts);

}

/************************************************************************
* FUNCTION
*
*   fsdh_allocate_dh
*
* DESCRIPTION
*
*   Allocate a disk handle. The disk handle is returned by the dh pointer.
*   A dh contains both the disk id and a index into FS_Disk_Handle.
*   The most significant 8 bits represent the disk id and the least
*   significant 8 bits represent an index into the FS_Disk_Handle array.
*
* INPUTS
*
*   *dh                             Pointer to disk handle
*
* OUTPUTS
*
*   NUF_BADPARM                     Invalid pointer
*   NUF_DISK_TABLE_FULL             Disk handle table is full
*   NU_SUCCESS                      Disk handle allocated
*
*************************************************************************/
STATUS fsdh_allocate_dh(UINT16 *dh)
{
UINT16 idx;
STATUS sts;
FSDH_S *fsdh = NU_NULL;

    /* Verify pointer */
    if (!dh)
        sts = NUF_BADPARM;
    else
    {
        /* Lock the table */
        LCK_ENTER(LCK_DH_TABLE)

        /* Find an empty handle entry */
        for (idx = 0; idx < FSDH_MAX_DISKS; idx++)
        {
            if (!(FS_Disk_Handles[idx].fsdh_flags & FSDH_F_VALID))
            {
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)
                /* Setup a semaphore for the disk handle */
                sts = NU_Create_Semaphore(&(FS_Disk_Handles[idx].fsdh_vfs_sema), "DHSEMA", 1, NU_PRIORITY);
                if (sts != NU_SUCCESS)
                {
                    break;
                }    
#endif
                /* Setup a pointer to the empty location */
                fsdh = &FS_Disk_Handles[idx];

                /* Store the disk id. */
                *dh = FSDH_GET_NEXT_DISK_ID(g_disk_id);

                /* Move disk id to left 8 bits. */
                *dh <<= 8;

                /* Store disk handle index number, typecast because we only interested in
                least significant 8 bits. */
                *dh |= (UINT8)idx;

                /* Now the dh's most significant 8 bits are the dh id,
                   and the least significant 8 bits represent an index
                   into the FS_Disk_Handle array. */

                /* Set the disk id. */
                fsdh->fsdh_disk_id = (UINT8)FSDH_GET_DISK_ID_FROM_DH(*dh);

                break;
            }
        }

        if (fsdh)
        {
            /* Mark the handle as valid, has been allocated */
            fsdh->fsdh_flags |= FSDH_F_VALID;
            fsdh->fsdh_exclusive_access = NU_FALSE; /* This should only be modified by NU_Check_Disk. */
            sts = NU_SUCCESS;
        }
        else
            sts = NUF_DISK_TABLE_FULL;

        /* Unlock the table */
        LCK_EXIT(LCK_DH_TABLE)
    }

    return (sts);
}

/************************************************************************
* FUNCTION
*
*   fsdh_free_dh
*
* DESCRIPTION
*
*   Returns a disk handle to the system.
*
* INPUTS
*
*   dh                              Disk handle
*
* OUTPUTS
*
*   NU_SUCCESS                      Disk handle freed
*   NUF_BADPARM                     Invalid disk handle
*
*************************************************************************/
STATUS fsdh_free_dh(UINT16 dh)
{
STATUS  sts;
FSDH_S* fsdh;

    /* Lock the table */
    LCK_ENTER(LCK_DH_TABLE)

    /* Verify disk handle */
    if (FSDH_GET_IDX_FROM_DH(dh) < FSDH_MAX_DISKS)
    {
        /* Get the disk handle structure */
        fsdh = &FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)];

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)
        /* Return the semaphore */
        (VOID) NU_Delete_Semaphore(&(fsdh->fsdh_vfs_sema));
#endif
        /* Rest values */
        NUF_Memfill(fsdh, sizeof(FSDH_S), 0);

        sts = NU_SUCCESS;
    }
    else
        sts = NUF_BADPARM;

    /* Unlock the table */
    LCK_EXIT(LCK_DH_TABLE)

    return (sts);
}

/************************************************************************
* FUNCTION
*
*   fsdh_set_dh_specific
*
* DESCRIPTION
*
*   Set disk handle specific information to be used by the disk driver
*   layer.
*
* INPUTS
*
*   dh                              Disk handle
*   *specific                       Pointer to driver specific information
*                                   to be used only by the driver
*   *disk_name                      Pointer to string holding disk name
*
* OUTPUTS
*
*   NU_SUCCESS                      Set was successful
*   NUF_BADPARM                     Invalid disk handle
*
*************************************************************************/
STATUS fsdh_set_dh_specific(UINT16 dh, VOID *specific, CHAR *disk_name)
{
UINT32 idx;
FSDH_S *fsdh;
STATUS sts =  NU_SUCCESS;

    /* Lock the table */
    LCK_ENTER(LCK_DH_TABLE)

    /* Verify disk handle */
    if (FSDH_GET_IDX_FROM_DH(dh) < FSDH_MAX_DISKS)
    {
        fsdh = &FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)];

        /* Verify the disk handle is valid */
        if (!(fsdh->fsdh_flags & FSDH_F_VALID) || (fsdh->fsdh_disk_id != FSDH_GET_DISK_ID_FROM_DH(dh)))
            sts = NUF_BADPARM;

        /* Set the specific item */
        if (sts == NU_SUCCESS)
        {
            fsdh->fsdh_specific = specific;
        }

        /* Set the disk_name if one is provided */
        if ((sts == NU_SUCCESS) && (disk_name))
        {
            for (idx = 0; idx < FSDH_MAX_DISK_NAME; idx++)
            {
                fsdh->fsdh_disk_name[idx] = disk_name[idx];
                if (disk_name[idx] == '\0')
                    break;
            }

        }

    }
    else
        sts = NUF_BADPARM;


    /* Unlock the table */
    LCK_EXIT(LCK_DH_TABLE)


    return (sts);
}

/************************************************************************
* FUNCTION
*
*   fsdh_get_dh_specific
*
* DESCRIPTION
*
*   Retrieves the driver specific information from the given disk handle.
*
*
* INPUTS
*
*   dh                              Disk handle
*   **specific                      Pointer to driver specific information
*                                   to be used only by the driver
*   *disk_name                      Pointer to string holding disk name
*
* OUTPUTS
*
*   NU_SUCCESS                      Get specific successful
*   NUF_BADPARM                     Invalid disk handle parameter
*
*************************************************************************/
STATUS fsdh_get_dh_specific(UINT16 dh, VOID **specific, CHAR *disk_name)
{
UINT32 idx;
FSDH_S *fsdh;
STATUS sts = NU_SUCCESS;

    /* Lock the table */
    LCK_ENTER(LCK_DH_TABLE)

    /* Verify pointers */
    if (!(specific))
        sts = NUF_BADPARM;
    /* Verify the disk handle is within range */
    else if (FSDH_GET_IDX_FROM_DH(dh) < FSDH_MAX_DISKS)
    {
        fsdh = &FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)];

        /* Can only set disk handles that are valid */
        if (!(fsdh->fsdh_flags & FSDH_F_VALID))
            sts = NUF_BADPARM;

        /* Get the specific value */
        if (sts == NU_SUCCESS)
        {
            *specific = fsdh->fsdh_specific;

        }

        /* Get the disk_name if a pointer is provided */
        if ((sts == NU_SUCCESS) && (disk_name))
        {
            /* Copy the disk name */
            for (idx = 0; idx < FSDH_MAX_DISK_NAME; idx++)
            {
                disk_name[idx] = fsdh->fsdh_disk_name[idx];
                if (fsdh->fsdh_disk_name[idx] == '\0')
                    break;
            }

            sts = NU_SUCCESS;
        }

    }
    else
        sts = NUF_BADPARM;

    /* Unlock the table */
    LCK_EXIT(LCK_DH_TABLE)

    return (sts);
}

/************************************************************************
* FUNCTION
*
*   fsdh_get_fsdh_struct
*
* DESCRIPTION
*
*   Retrieve the fsdh structure associated with the disk handle.
*
*
* INPUTS
*
*   dh                              Disk handle
*   **dhs                           Disk handle structure associated
*                                   with requested 'dh'
*
* OUTPUTS
*
*   NU_SUCCESS                      Request was successful
*   NUF_BADPARM                     Invalid disk handle
*
*************************************************************************/
STATUS fsdh_get_fsdh_struct(UINT16 dh, FSDH_S **dhs)
{
STATUS sts;

    /* Lock the table */
    LCK_ENTER(LCK_DH_TABLE)

    /* Verify disk handle */
    FSDH_VERIFY_DH(dh,sts);

    if(sts == NU_SUCCESS)
        *dhs = &FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)];

    /* Unlock the table */
    LCK_EXIT(LCK_DH_TABLE)

    return (sts);
}

/************************************************************************
* FUNCTION
*
*   fsdh_set_fs_specific
*
* DESCRIPTION
*
*   Set disk handle specific information to be used by the file system
*   layer.
*
* INPUTS
*
*   dh                              Disk handle
*   *specific                       Pointer to driver specific information
*                                   to be used only by the driver
*
* OUTPUTS
*
*   NU_SUCCESS                      Request was successful
*   NUF_BADPARM                     Invalid disk handle
*
*************************************************************************/
STATUS fsdh_set_fs_specific(UINT16 dh, VOID *specific)
{
FSDH_S *fsdh;
STATUS sts =  NU_SUCCESS;

    /* Lock the table */
    LCK_ENTER(LCK_DH_TABLE)

    /* Verify disk handle */
    if (FSDH_GET_IDX_FROM_DH(dh) < FSDH_MAX_DISKS)
    {
        fsdh = &FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)];

        /* Verify the disk handle is valid */
        if (!(fsdh->fsdh_flags & FSDH_F_VALID) || (fsdh->fsdh_disk_id != FSDH_GET_DISK_ID_FROM_DH(dh)))
            sts = NUF_BADPARM;

        /* Set the specific item */
        if (sts == NU_SUCCESS)
        {
            fsdh->fsdh_fs = specific;
        }
    }
    else
        sts = NUF_BADPARM;

    /* Unlock the table */
    LCK_EXIT(LCK_DH_TABLE)

    return (sts);
}
/************************************************************************
* FUNCTION
*
*   fsdh_get_fs_specific
*
* DESCRIPTION
*
*   Retrieves the file system specific information from the given disk
*   handle.
*
*
* INPUTS
*
*   dh                              Disk handle
*   **specific                      FS specific information
*                                   to be used only by the FS
*
* OUTPUTS
*
*   NU_SUCCESS                      Request was successful
*   NUF_BADPARM                     Invalid disk handle
*
*************************************************************************/
STATUS fsdh_get_fs_specific(UINT16 dh,VOID **specific)
{
FSDH_S *fsdh;
STATUS sts = NU_SUCCESS;

    /* Lock the table */
    LCK_ENTER(LCK_DH_TABLE)

    /* Verify the disk handle is within range */
    if (FSDH_GET_IDX_FROM_DH(dh) < FSDH_MAX_DISKS)
    {
        fsdh = &FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)];

        /* Can only set disk handles that are valid */
        if (!(fsdh->fsdh_flags & FSDH_F_VALID) || (fsdh->fsdh_disk_id != FSDH_GET_DISK_ID_FROM_DH(dh)))
            sts = NUF_BADPARM;

        /* Get the specific value */
        if (sts == NU_SUCCESS)
        {
            *specific = fsdh->fsdh_fs;
        }
    }
    else
        sts = NUF_BADPARM;

    /* Unlock the table */
    LCK_EXIT(LCK_DH_TABLE)

    return (sts);
}

/************************************************************************
* FUNCTION
*
*   fsdh_dh_to_devname
*
* DESCRIPTION
*
*   Retrieves the device name associated with a disk handle.
*
*
* INPUTS
*
*   dh                              Disk handle
*   *devname                        Device name of the associated disk handle
*
* OUTPUTS
*
*   NU_SUCCESS                      Request was successful
*   NUF_BADPARM                     Invalid disk handle
*
*************************************************************************/
STATUS fsdh_dh_to_devname(UINT16 dh, CHAR *devname)
{
FSDH_S *fsdh;
STATUS sts = NU_SUCCESS;

    /* Lock the table */
    LCK_ENTER(LCK_DH_TABLE)

    /* Verify the disk handle is within range */
    if (FSDH_GET_IDX_FROM_DH(dh) < FSDH_MAX_DISKS)
    {
        fsdh = &FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)];

        /* Can only set disk handles that are valid */
        if (!(fsdh->fsdh_flags & FSDH_F_VALID) || (fsdh->fsdh_disk_id != FSDH_GET_DISK_ID_FROM_DH(dh)))
            sts = NUF_BADPARM;

        /* Verify the dev structure */
        if (sts == NU_SUCCESS)
        {
            if(fsdh->fsdh_fdev)
            {
                /* Copy the device name */
                NUF_Copybuff(devname,fsdh->fsdh_fdev->fdev_name,FS_DEV_MAX_DEVNAME);
            }
            else
                sts = NUF_BADPARM;
        }
    }
    else
        sts = NUF_BADPARM;

    /* Unlock the table */
    LCK_EXIT(LCK_DH_TABLE)

    return (sts);
}

/************************************************************************
* FUNCTION
*
*   fsdh_devname_to_dh
*
* DESCRIPTION
*
*   Retrieves the disk handle associated with a device name.
*
*
* INPUTS
*
*   *devname                        Device name of the associated disk handle
*   dh                              Disk handle
*
* OUTPUTS
*
*   NU_SUCCESS                      Request was successful
*   NUF_BADPARM                     Invalid disk handle
*
*************************************************************************/
STATUS fsdh_devname_to_dh(CHAR *devname, UINT16 *dh)
{
    FSDH_S *fsdh;
    UINT16 idx, found = NU_FALSE;
    STATUS sts = NU_SUCCESS;

    if ((!devname) || (!dh))
    {
        sts = NUF_BADPARM;
    }
    else
    {
        /* Lock the table */
        LCK_ENTER(LCK_DH_TABLE)
    
        /* Search disk handle entries for devname. */
        for(idx = 0; idx < FSDH_MAX_DISKS; idx++)
        {
            fsdh = &FS_Disk_Handles[idx];
            if (fsdh->fsdh_flags & FSDH_F_VALID)    /* if entry is valid, */
            {
                if(NU_SUCCESS == NUF_Strncmp(devname,fsdh->fsdh_fdev->fdev_name,FS_DEV_MAX_DEVNAME))
                {
                    found = NU_TRUE;
                    /* Store the disk id. */
                    *dh = fsdh->fsdh_disk_id;
                    /* Move disk id to the left 8 bits. */
                    *dh <<= 8;
                    /* Store FS_Disk_Handle index number, typecast because
                       we only interested in least significant 8 bits. */
                    *dh |= (UINT8)idx;
                    /* Now the dh's most significant 8 bits are the dh id,
                       and the least significant 8 bits represent an index
                       into the FS_Disk_Handle array. */
                    break;
                }
            }
        }
    
        if(!found)
        {
            sts = NUF_BADPARM;
        }
    
        /* Unlock the table */
        LCK_EXIT(LCK_DH_TABLE)
    }
    
    return (sts);
}

/************************************************************************
* FUNCTION
*
*   fsdh_dh_idx_to_dh
*
* DESCRIPTION
*
*   Given a disk handle index find its disk handle value. A disk handle
*   value consist of two components, the most significant 8 bits
*   represent a disk handle id, and the least significant 8 bits
*   represent the index into the FS_Disk_Handle array.
*
*
* INPUTS
*
*   dh_idx(in)                      Disk handle index(most significant
*                                   8 bits of dh).
*   *dh(out)                        Disk handle
*
* OUTPUTS
*
*   NU_SUCCESS                      Request was successful
*   NUF_BADPARM                     Invalid disk handle index
*
*************************************************************************/
STATUS fsdh_dh_idx_to_dh(UINT16 dh_idx, UINT16* dh)
{
STATUS ret_val = NU_SUCCESS;
FSDH_S *fsdh;

    if((dh_idx < FSDH_MAX_DISKS) && (dh))
    {
        fsdh = &FS_Disk_Handles[dh_idx];

        if (fsdh->fsdh_flags & FSDH_F_VALID)
        {
            /* Get the dh disk id. */
            *dh = fsdh->fsdh_disk_id;

            /* Shift it to the upper most 8 bits. */
            *dh <<= 8;

            /* Store dh index in the right most 8 bits. */
            *dh |= (UINT8)dh_idx;
        }
        else
            ret_val = NUF_BADPARM;
    }
    else
        ret_val = NUF_BADPARM;

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   fsdh_set_semaphore
*
* DESCRIPTION
*
*   Set disk handle specific semaphore for devices requiring protecting
*   shared resources.
*
* INPUTS
*
*   dh                              Disk handle
*   *semaphore                      Pointer to driver specific semaphore
*
* OUTPUTS
*
*   NU_SUCCESS                      Request was successful
*   NUF_BADPARM                     Invalid disk handle
*
*************************************************************************/
STATUS fsdh_set_semaphore(UINT16 dh, NU_SEMAPHORE *semaphore)
{
FSDH_S *fsdh;
STATUS sts =  NU_SUCCESS;

    /* Lock the table */
    LCK_ENTER(LCK_DH_TABLE)

    /* Verify the disk handle is within range */
    if (FSDH_GET_IDX_FROM_DH(dh) < FSDH_MAX_DISKS)
    {
        fsdh = &FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)];

        /* Verify the disk handle is valid */
        if (!(fsdh->fsdh_flags & FSDH_F_VALID) || (fsdh->fsdh_disk_id != FSDH_GET_DISK_ID_FROM_DH(dh)))
            sts = NUF_BADPARM;

        /* Set the specific item */
        if (sts == NU_SUCCESS)
        {
            fsdh->fsdh_sema = semaphore;
        }
    }
    else
        sts = NUF_BADPARM;

    /* Unlock the table */
    LCK_EXIT(LCK_DH_TABLE)

    return (sts);
}
/************************************************************************
* FUNCTION
*
*   fsdh_get_semaphore
*
* DESCRIPTION
*
*   Retrieves the disk handle specific semaphore
*
*
* INPUTS
*
*   dh                              Disk handle
*   **semaphore                     Pointer to retrieved semaphore
*
* OUTPUTS
*
*   NU_SUCCESS                      Request was successful
*   NUF_BADPARM                     Invalid disk handle
*
*************************************************************************/
STATUS fsdh_get_semaphore(UINT16 dh, NU_SEMAPHORE **semaphore)
{
FSDH_S *fsdh;
STATUS sts = NU_SUCCESS;

    /* Lock the table */
    LCK_ENTER(LCK_DH_TABLE)

    /* Verify the disk handle is within range */
    if (FSDH_GET_IDX_FROM_DH(dh) < FSDH_MAX_DISKS)
    {
        fsdh = &FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)];

        /* Can only set disk handles that are valid */
        if (!(fsdh->fsdh_flags & FSDH_F_VALID) || (fsdh->fsdh_disk_id != FSDH_GET_DISK_ID_FROM_DH(dh)))
            sts = NUF_BADPARM;

        /* Get the specific value */
        if (sts == NU_SUCCESS)
        {
            *semaphore = fsdh->fsdh_sema;
        }
    }
    else
        sts = NUF_BADPARM;

    /* Unlock the table */
    LCK_EXIT(LCK_DH_TABLE)

    return (sts);
}

/************************************************************************
* FUNCTION
*
*   fsdh_set_user_state
*
* DESCRIPTION
*
*   Given a disk handle and a user index, assign the user state value.
*
* INPUTS
*
*   dh                          Disk handle
*   idx                         FILE User index
*   state                       User - Disk state (closed, opened, aborted)
*
* OUTPUTS
*
*   NU_SUCCESS                  State was set
*   NUF_BADPARM                 Invalid parameter given
*
*************************************************************************/
STATUS fsdh_set_user_state(UINT16 dh, UINT32 idx, UINT8 state)
{
FSDH_S *fsdh;
STATUS sts = NU_SUCCESS;

    /* Lock the table */
    LCK_ENTER(LCK_DH_TABLE)

    /* Verify the disk handle and user index are within range */
    if ((FSDH_GET_IDX_FROM_DH(dh) < FSDH_MAX_DISKS) && (idx < VFS_NUM_USERS_WDU))
    {
        fsdh = &FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)];

        /* Can only set disk handles that are valid */
        if (!(fsdh->fsdh_flags & FSDH_F_VALID))
            sts = NUF_BADPARM;

        /* Set the specific value */
        if (sts == NU_SUCCESS)
        {
            /* Set the state value */
            fsdh->fsdh_user_state[idx] = state;
        }
    }
    else
        sts = NUF_BADPARM;

    /* Unlock the table */
    LCK_EXIT(LCK_DH_TABLE)

    return (sts);
}

/************************************************************************
* FUNCTION
*
*   fsdh_get_user_state
*
* DESCRIPTION
*
*   Given a disk handle and a user index, retrieve the user state.
*
* INPUTS
*
*   dh                          Disk handle
*   idx                         FILE User index
*   state                       User - Disk state (closed, opened, aborted)
*
* OUTPUTS
*
*   NU_SUCCESS                  State was set
*   NUF_BADPARM                 Invalid parameter given
*
*************************************************************************/
STATUS fsdh_get_user_state(UINT16 dh, UINT32 idx, UINT8 *state)
{
FSDH_S *fsdh;
STATUS sts = NU_SUCCESS;

    /* Lock the table */
    LCK_ENTER(LCK_DH_TABLE)

    /* Verify the disk handle and user index are within range, state not null */
    if ((FSDH_GET_IDX_FROM_DH(dh) < FSDH_MAX_DISKS) && (idx < VFS_NUM_USERS_WDU) && (state))
    {
        fsdh = &FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)];

        /* Can only set disk handles that are valid */
        if (!(fsdh->fsdh_flags & FSDH_F_VALID))
            sts = NUF_BADPARM;

        /* Set the specific value */
        if (sts == NU_SUCCESS)
        {
            /* Get the state value */
            *state = fsdh->fsdh_user_state[idx];
        }

    }
    else
        sts = NUF_BADPARM;

    /* Unlock the table */
    LCK_EXIT(LCK_DH_TABLE)

    return (sts);
}

