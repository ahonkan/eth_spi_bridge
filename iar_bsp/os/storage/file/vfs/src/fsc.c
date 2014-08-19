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
*       fsc.c
*
* COMPONENT
*
*       Core
*
* DESCRIPTION
*
*       Contains core file system and initialization routines
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Register_File_System
*       NU_Unregister_File_System
*       NU_Mount_File_System
*       NU_Unmount_File_System
*       NU_VFS_Init
*
*************************************************************************/
#include "nucleus.h"

#include "storage/pcdisk.h"
#include "storage/lck_extr.h"
#include "storage/dh_extr.h"
#include "storage/fd_extr.h"
#include "storage/dev_extr.h"
#include "storage/vnode_extr.h"
#include "storage/fst_defs.h"
#include "storage/fsl_extr.h"
#include "storage/user_extr.h"
#include "storage/util_extr.h"


#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)
    #include "storage/bcm_extr.h"
    #include "storage/bcmms_extr.h"
#endif 

FSTE_S FS_Table[FSC_MAX_TABLE_SIZE];
UINT32 FILE_Drive_Id = 0;
NU_MEMORY_POOL *FILE_Alloc_Pool;
extern NU_EVENT_GROUP FILE_Application_Event_Group;
extern MTE_S MTE_Table[MTE_MAX_TABLE_SIZE];
extern NU_MEMORY_POOL *FILE_Alloc_Pool;

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1)
    extern STATUS uni_setup_cp(MTE_S *mte);
#endif

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)
    extern NU_SEMAPHORE BCM_List_Sema; 
#endif

/* CP_Table defined in either fs_encod.c when Unicode support is not enabled, */
/* otherwise it is in uni_util.c                                              */
extern CPTE_S CP_Table[CP_MAX_TABLE_SIZE];

#define NURFS_CHECK(X,Y,Z)      \
    if (X == NU_SUCCESS)        \
    {                           \
        if (Z != NU_NULL)       \
            Y = Z;              \
        else                    \
            X = NUF_BADPARM;    \
    }


/************************************************************************
* FUNCTION
*
*   NU_Register_File_System
*
* DESCRIPTION
*
*   Register a file system for use with Nucleus FILE. Takes a string
*   describing the file system type along with a pointer to a FS_S
*   structure for file system specific dispatch routines.
*
* INPUTS
*
*   *name                           Pointer to string describing FS
*   *fs                             Pointer to FS_S structure for file
*                                   system specific dispatch routines.
*
* OUTPUTS
*
*   NU_SUCCESS                      File system successfully registered
*   NUF_BADPARM                     Invalid pointer or parameter given
*
*************************************************************************/
STATUS NU_Register_File_System(CHAR *name, FS_S *fs)
{
STATUS  ret_stat = NU_SUCCESS;
FSTE_S  *fste;
UINT32  idx;

    LCK_FS_ENTER()

    /* Must be set to NULL so we know when an empty slot was found */
    fste = NU_NULL;

    /* Verify input pointers are not null */
    if (!name || !fs)
        ret_stat = NUF_BADPARM;
    else
    {
        /* Lock the file system table */
        LCK_ENTER(LCK_FS_TABLE)

        /* Verify that the name is unique */
        for (idx = 0; idx < FSC_MAX_TABLE_SIZE; idx++)
        {
            /* Look for valid entries */
            if (FS_Table[idx].fste_flags & FSTE_FL_VALID)
            {
                /* Check if the names are equal */
                if (NUF_Strncmp(FS_Table[idx].fste_name, name,
                                FSC_MAX_FS_NAME) == 0)
                    ret_stat = NUF_DUPLICATE_FSNAME;
            }
        }

        if (ret_stat == NU_SUCCESS)
        {
            /* Find an empty spot in the file system table */
            for (idx = 0; idx < FSC_MAX_TABLE_SIZE; idx++)
            {
                /* Empty slot does not have FSTE_FL_VALID flag set */
                if (!(FS_Table[idx].fste_flags & FSTE_FL_VALID))
                {
                    fste = &FS_Table[idx];
                    break;
                }
            }

            /* Determine if we found a spot in the file system
               table. */
            if (!fste)
                ret_stat = NUF_FS_TABLE_FULL;
        }

        /* Copy the entry contents */
        if (ret_stat == NU_SUCCESS)
        {
            /* Clear the contents */
            NUF_Memfill(fste, sizeof(FSTE_S), 0);

            /* Copy the file system name */
            for (idx = 0; idx < FSC_MAX_FS_NAME; idx++)
                fste->fste_name[idx] = name[idx];
        }

        /* Assign required file system operations */
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_close, fs->fs_close)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_delete, fs->fs_delete)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_disk_abort, fs->fs_disk_abort)
#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)        
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_check_disk, fs->fs_check_disk)
#endif        
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_flush, fs->fs_flush)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_get_attr, fs->fs_get_attr)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_mkdir, fs->fs_mkdir)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_mount, fs->fs_mount)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_unmount, fs->fs_unmount)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_init , fs->fs_init)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_uninit , fs->fs_uninit)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_open, fs->fs_open)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_freespace, fs->fs_freespace)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_read, fs->fs_read)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_utime, fs->fs_utime)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_rename, fs->fs_rename)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_rmdir, fs->fs_rmdir)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_seek, fs->fs_seek)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_set_attr, fs->fs_set_attr)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_truncate, fs->fs_truncate)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_write, fs->fs_write)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_get_first, fs->fs_get_first)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_get_next, fs->fs_get_next)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_done, fs->fs_done)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_format, fs->fs_format)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_get_format_info, fs->fs_get_format_info)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_fsnode_to_string, fs->fs_fsnode_to_string)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_vnode_allocate, fs->fs_vnode_allocate)
        NURFS_CHECK(ret_stat, fste->fste_fs.fs_vnode_deallocate, fs->fs_vnode_deallocate)

        if ( ret_stat == NU_SUCCESS)
        {
            /* Assign non-required file system operations. Otherwise, remain NU_NULL */
            if (fs->fs_release)
                fste->fste_fs.fs_release = fs->fs_release;
            if (fs->fs_reclaim)
                fste->fste_fs.fs_reclaim = fs->fs_reclaim;
        }

        /* Dispatch the fs_init routine */
        if ((ret_stat == NU_SUCCESS) && (fste->fste_fs.fs_init) )
            ret_stat = fste->fste_fs.fs_init();

        /* Having an init routine isn't required. The entry is invalid
           if there is a routine and it returns an error. */
        if (ret_stat == NU_SUCCESS)
            /* Mark entry as valid */
            fste->fste_flags |= FSTE_FL_VALID;

        /* Unlock FS table */
        LCK_EXIT(LCK_FS_TABLE)
    }

    /* Notify application of file system registration */
    if (ret_stat == NU_SUCCESS)
        NU_Set_Events(&FILE_Application_Event_Group,NUF_EVT_FS_CREATE,NU_OR);

    LCK_FS_EXIT()

    return (ret_stat);
}

/************************************************************************
* FUNCTION
*
*   NU_Unregister_File_System
*
* DESCRIPTION
*
*   Removes a previously registered file system identified by 'name'.
*   The file system must not be in use.
*
* INPUTS
*
*   *name                           Pointer to string describing FS
*
* OUTPUTS
*
*   NU_SUCCESS                      File system successfully unregistered
*   NUF_BADPARM                     Invalid pointer or parameter given
*   NUF_IN_USE                      File system is currently in use by
*                                   a mounted drive
*
*************************************************************************/
STATUS NU_Unregister_File_System(CHAR *name)
{
STATUS  ret_stat = NUF_BADPARM;
UINT32    idx;

    LCK_FS_ENTER()

    /* Verify input pointer is not null */
    if (!name)
        ret_stat = NUF_BADPARM;
    else
    {
        /* Lock the file system table */
        LCK_ENTER(LCK_FS_TABLE)

        /* Find the matching file system entry */
        for (idx = 0; idx < FSC_MAX_TABLE_SIZE; idx++)
        {
            /* Verify entry is valid */
            if (!(FS_Table[idx].fste_flags & FSTE_FL_VALID))
                continue;
            else if (NUF_Strncmp(name, &FS_Table[idx].fste_name[0], FSC_MAX_FS_NAME) == 0)
            {
                /* Verify entry is not in use */
                if (FS_Table[idx].fste_cnt == 0)
                {
                    /* Mark entry as invalid */
                    FS_Table[idx].fste_flags ^= FSTE_FL_VALID;
                    if((FS_Table[idx].fste_fs.fs_uninit))
                        ret_stat = FS_Table[idx].fste_fs.fs_uninit();
                    else
                        ret_stat = NU_SUCCESS;
                    break;
                }
                else
                {
                    /* Entry currently in use. Unable to remove. */
                    ret_stat = NUF_IN_USE;
                    break;
                }
            }
        }

        /* Unlock FS table */
        LCK_EXIT(LCK_FS_TABLE)

    }


    /* Notify application of file system registration */
    if (ret_stat == NU_SUCCESS)
        NU_Set_Events(&FILE_Application_Event_Group,NUF_EVT_FS_DESTROY,NU_OR);

    LCK_FS_EXIT()

    return (ret_stat);
}


/************************************************************************
* FUNCTION
*
*   NU_Mount_File_System
*
* DESCRIPTION
*
*   Add a file system and disk to the system and the location specified
*   by *mount_point using the disk specified by *dev_name with the file
*   system specified by *fs_name.
*
* INPUTS
*
*   *fs_name                        String describing file system type
*   *mount_point                    Location to mount storage device.
*   *dev_name                       String describing the disk name.
*   *config                         File system specific configuration
*                                   settings.
*
* OUTPUTS
*
*   NU_SUCCESS                      Disk was mounted
*   NUF_FS_NOT_FOUND                File system type not found
*   NUF_BADPARM                     Invalid pointer or parameter
*   NUF_MOUNT_NOT_AVAILABLE         Mount location is not available
*   NUF_MOUNT_TABLE_FULL            Mount table is full
*
*************************************************************************/
STATUS NU_Mount_File_System(CHAR *fs_name, CHAR *mount_point, CHAR* dev_name, VOID *config)
{
UINT32  idx;
INT     found_cp = 0;
MTE_S   *mte = NU_NULL;
FSTE_S  *fste = NU_NULL;
FDEV_S  *dev = NU_NULL;
UINT32  flags;
STATUS  ret_stat = NU_SUCCESS;
INT16   dn;
CPTE_S  *cpte;

    LCK_FS_ENTER()

    /* Verify input pointers are not null */
    if ( (!fs_name) || (!mount_point) || (!dev_name) )
        ret_stat = NUF_BADPARM;

    /* Verify the device is mountable */
    if (ret_stat == NU_SUCCESS)
    {
        ret_stat = fs_dev_get_fdev_flags(dev_name, &flags);
        if ((ret_stat == NU_SUCCESS) && (flags & FDEV_FL_NOT_MOUNTABLE))
            ret_stat = NUF_MOUNT_NOT_AVAILABLE;

    }

    /* Lock the FS table */
    LCK_ENTER(LCK_FS_TABLE)

    if (ret_stat == NU_SUCCESS)
    {
        /* Find the matching file system entry */
        for (idx = 0; idx < FSC_MAX_TABLE_SIZE; idx++)
        {
            if (FS_Table[idx].fste_flags & FSTE_FL_VALID)
            {
                if (NUF_Strncmp(fs_name, &FS_Table[idx].fste_name[0], FSC_MAX_FS_NAME) == 0)
                {
                    fste = &FS_Table[idx];
                    /* Update the usage count so we can prevent this table entry from being
                       removed while in use. FS table lock is held when this count is
                       modified */
                    fste->fste_cnt++;
                    break;
                }
            }
        }

        if (!fste)
        {
            /* Matching entry was not found in the table */
            ret_stat = NUF_FS_NOT_FOUND;
        }
    }

    /* Unlock the FS table */
    LCK_EXIT(LCK_FS_TABLE)

    /* Find the matching device name */
    if (ret_stat == NU_SUCCESS)
    {
        /* Lock the dev table */
        LCK_ENTER(LCK_DEV_TABLE)

        /* Grab the fdev structure for this device */
        ret_stat = fs_dev_devname_to_fdev(dev_name, &dev);

        /* Update the count for the device while lock is held */
        if (dev)
            dev->fdev_cnt++;

        /* Unlock table */
        LCK_EXIT(LCK_DEV_TABLE)
    }

    /* Lock the mount table to prevent changes while searching */
    LCK_ENTER(LCK_MT_TABLE)

    /* Verify the mount point is available and the device is not currently mounted */
    if (ret_stat == NU_SUCCESS)
    {
        for (idx = 0; idx < MTE_MAX_TABLE_SIZE; idx++)
        {
            if ( (MTE_Table[idx].mte_flags & MTE_FL_VALID) &&
                 ((*mount_point == MTE_Table[idx].mte_mount_name[0]) ||
                  (MTE_Table[idx].mte_dh == dev->fdev_dh)))
            {
                ret_stat = NUF_MOUNT_NOT_AVAILABLE;
                break;
            }
        }
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Find an empty location in the table */
        for (idx = 0; idx < MTE_MAX_TABLE_SIZE; idx++)
        {
            /* Verify table entry is not in use */
            if (!(MTE_Table[idx].mte_flags & MTE_FL_VALID) &&
                 (MTE_Table[idx].mte_cnt == 0) )
            {
                mte = &MTE_Table[idx];
                /* Update the usage count on the table entry while the
                   table lock is held. This will prevent others from
                   using the entry */
                mte->mte_cnt++;
                break;
            }
         }

        if (!mte)
            /* Couldn't find an empty location in the mount table */
            ret_stat = NUF_MOUNT_TABLE_FULL;
    }

    /* Unlock the mount table */
    LCK_EXIT(LCK_MT_TABLE)

    if (ret_stat == NU_SUCCESS)
    {
        /* Setup the table entry */

        for (idx = 0; idx < MTE_MAX_MOUNT_NAME; idx++)
            mte->mte_mount_name[idx] = mount_point[idx];

        for (idx = 0; idx < MTE_MAX_DEVICE_NAME; idx++)
            mte->mte_device_name[idx] = dev_name[idx];

        /* Assign the file system operations */
        mte->mte_fs = &fste->fste_fs;

        /* Assign the file system table entry */
        mte->mte_fste = fste;

        /* Link the device structure */
        if(dev)
        {
            mte->mte_fdev = dev;
    
           /* Assign disk handle */
            mte->mte_dh = dev->fdev_dh;
    
            /* Setup the CWD vnodes for this disk handle */
            ret_stat = fsv_init_vnodes(mte->mte_dh);
        }
        else
        {
            ret_stat = NUF_BADPARM;
        }
        /* Verify mount point is a single character. */
        if (mount_point[1] != '\0')
        {
            ret_stat = NUF_BADPARM;
        }

        if (ret_stat == NU_SUCCESS)
        {
            /* Setup the mount points device id */
            LCK_ENTER(LCK_MT_TABLE)
            mte->mte_drive_id = FILE_Drive_Id;
            FILE_Drive_Id++;
            /* Check for rollover */
            if (FILE_Drive_Id == 0)
                FILE_Drive_Id = 1;
            LCK_EXIT(LCK_MT_TABLE)

            /* Mark table entry as valid */
            mte->mte_flags |= MTE_FL_VALID;

            /* Convert the mount point to a drive number {a=A=0,b=B=1,etc.} */

            /* Convert character to a driver number, case insensitive */
            if ( ((*mount_point) >= 'A') && ((*mount_point) <= 'Z') )
                dn = (INT16) (*mount_point - 'A');
            else if ( ((*mount_point) >= 'a') && ((*mount_point) <= 'z') )
                dn = (INT16) (*mount_point - 'a');
            else
                dn = NO_DRIVE;

            /* Set drive number for later usage */
            if (dn != NO_DRIVE)
                mte->mte_drive = dn;
            else
                ret_stat = NUF_BADPARM;

        }

        if (ret_stat == NU_SUCCESS)
            /* Call the file system specific mount routine */
            ret_stat = mte->mte_fs->fs_mount(mte->mte_dh, config);

        if (ret_stat != NU_SUCCESS)
        {
            /* Mark table entry as invalid */
            mte->mte_flags &= ~MTE_FL_VALID;

            /* FS mount failed. Vnodes need to be freed. */
            fsv_release_vnodes(mte->mte_dh);

            /* The mount failed, we need to release the usage count
               for the table entry to allow reuse of the entry */
            LCK_ENTER(LCK_MT_TABLE)
            mte->mte_cnt--;
            LCK_EXIT(LCK_MT_TABLE)
        }
    }

    if (ret_stat != NU_SUCCESS)
    {
        /* Restore the usage count of the FS table entry because
           of a problem mounting a file system */
        if (fste)
        {
            LCK_ENTER(LCK_FS_TABLE)
            fste->fste_cnt--;
            LCK_EXIT(LCK_FS_TABLE)
        }

        /* Restore the device usage count due to mount failure */
        if (dev)
        {
            LCK_ENTER(LCK_DEV_TABLE)
            dev->fdev_cnt--;
            LCK_EXIT(LCK_DEV_TABLE)
        }
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Set the mounted flag. */
        mte->mte_flags |= MTE_FL_MOUNTED;

        /* Notify application of device mount */
    	NU_Set_Events(&FILE_Application_Event_Group,NUF_EVT_MOUNT_CREATE,NU_OR);

    	/* Set default user disk state */
        idx = fsu_get_default_user_index();
        ret_stat = fsdh_set_user_state(mte->mte_dh, idx, FSDH_USER_OPENED);
    }

    /* Load codepage(s) */
    if(ret_stat == NU_SUCCESS)
    {
        /* Verify that our codepage is in the table. */
        for(idx = 0; idx < CP_MAX_TABLE_SIZE; ++idx)
        {
            /* If using default codepage. */
            if((config == NU_NULL) && (CP_Table[idx].cp == DEFAULT_CODEPAGE))
            {
                found_cp = 1;
                break;
            }
            /* Using user supplied codepage. */
            else
            {
                /* If we are using the Safe FS set it to ASCII. */
                if((NUF_Strncmp("SAFE",fs_name,FSC_MAX_FS_NAME) == 0) && (CP_Table[idx].cp == CP_ASCII))
                {
                    found_cp = 1;                                        
                    break;
                }
                if((config != NU_NULL) && (CP_Table[idx].cp == ((CPTE_S*)config)->cp))
                {
                    found_cp = 1;
                    break;
                }
            }
        }

        /* Set a pointer to the codepage that is to be mounted with this device. */
        cpte = &CP_Table[idx];

        if(found_cp)
        {
            /* Ascii support. */
            if(cpte->cp == CP_ASCII)
            {
                cpte->cp_op.cp_to_utf8 = cp_2_utf8;
                cpte->cp_op.utf8_to_cp = utf8_2_cp;
                mte->mte_cp = cpte;
            }
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1)
            else
            {
                mte->mte_cp = cpte;
                /* Else call the unicode initialization routine. */
                if(CP_Table[idx].cp_op.cp_to_utf8 == NU_NULL
                    && CP_Table[idx].cp_op.utf8_to_cp == NU_NULL)
                {
                    ret_stat = uni_setup_cp(mte);
                }
            }
#endif
        }/* found_cp */
        else
        {
            ret_stat = NUF_BADPARM;
        }
    }

    LCK_FS_EXIT()

    return (ret_stat);
}
/************************************************************************
* FUNCTION
*
*   NU_Unmount_File_System
*
* DESCRIPTION
*
*   Remove a previously mounted file system instance. The instance must
*   not be in use.
*
* INPUTS
*
*   *mount_point                    Location of mounted storage instance
*                                   to be removed.
*
* OUTPUTS
*
*   NU_SUCCESS                      Disk successfully unmounted
*   NUF_MOUNT_NOT_AVAILABLE         Invalid location given
*   NUF_BADPARM                     Invalid pointer or parameter
*   NUF_IN_USE                      Disk is currently in use
*
*************************************************************************/
STATUS NU_Unmount_File_System(CHAR *mount_point)
{
STATUS ret_stat = NU_SUCCESS;
MTE_S  *mte =  NU_NULL;
INT16 dn;
FSDH_S *dhs;

    LCK_FS_ENTER()

    /* Verify parameter */
    if (!mount_point)
        ret_stat = NUF_BADPARM;

    /* Lock the mount table */
    LCK_ENTER(LCK_MT_TABLE)

    if (ret_stat == NU_SUCCESS)
    {
        /* Verify mount point is single character */
        if (mount_point[1] != '\0')
            dn = NO_DRIVE;
        /* Convert character to a driver number, case insensitive */
        else if ( ((*mount_point) >= 'A') && ((*mount_point) <= 'Z') )
            dn = (INT16) (*mount_point - 'A');
        else if ( ((*mount_point) >= 'a') && ((*mount_point) <= 'z') )
            dn = (INT16) (*mount_point - 'a');
        else
            dn = NO_DRIVE;

        if (dn != NO_DRIVE)
            mte = fsl_mte_from_drive_n(dn);
        else
            ret_stat = NUF_BADPARM;
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Verify lookup was successful */
        if (!mte)
            ret_stat = NUF_MOUNT_NOT_AVAILABLE;
        /* Verify there are no other users */
        else 
        {
            ret_stat = fsdh_get_fsdh_struct(mte->mte_dh,&dhs);
            if(ret_stat == NU_SUCCESS)
            {
                if((mte->mte_cnt != 1) || (dhs->fsdh_opencount != 0))
                    ret_stat = NUF_IN_USE;
            }
        }
    }

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1) 
    /* Verify the device does not have a currently active cache */
    if (ret_stat == NU_SUCCESS)
    {
        BCM_CB *cb;
        
        /* Get the list semaphore */
        (VOID) NU_Obtain_Semaphore(&BCM_List_Sema, NU_SUSPEND);
        
        /* Check if the device has an active cache */
        ret_stat = bcm_device_has_cache (mte->mte_dh, &cb);
        if (ret_stat == NU_SUCCESS)
        {
            /* Device should not have an active cache */
            ret_stat = NUF_IN_USE;
        }
        else
        {
            /* Set the return status back to success */
            ret_stat = NU_SUCCESS;               
        }
        
        /* Release the list semaphore */
        (VOID) NU_Release_Semaphore(&BCM_List_Sema);
        
    }
#endif

    if (ret_stat == NU_SUCCESS)
    {
        /* Dispatch unmount routine */
        ret_stat = mte->mte_fs->fs_unmount(mte->mte_dh);
    }

    if (ret_stat == NU_SUCCESS)
    {

        /* Clear the mount entries drive id */
        mte->mte_drive_id = 0;

        /* Lock device table */
        LCK_ENTER(LCK_DEV_TABLE)

        /* Lower the device usage */
        mte->mte_fdev->fdev_cnt--;

        /* Free the disk handle */

        /* Unlock the device table */
        LCK_EXIT(LCK_DEV_TABLE)

        /* Lock the FS table */
        LCK_ENTER(LCK_FS_TABLE)

        /* Lower the usage of the FS */
        mte->mte_fste->fste_cnt--;

        /* Unlock the FS table */
        LCK_EXIT(LCK_FS_TABLE)
    }
    if (ret_stat == NU_SUCCESS)
    {
        /* Vnodes need to be freed. */
        fsv_release_vnodes(mte->mte_dh);

        /* Reset the user disk state for each user */
        for (dn = 0; dn < VFS_NUM_USERS_WDU; dn++)
        {
            /* Ignore the return value as we know the number of users and
               the state is valid */
            fsdh_set_user_state(mte->mte_dh, dn, FSDH_USER_CLOSED);
        }

        /* Clear the FS specific information for this disk handle */
        fsdh_set_fs_specific(mte->mte_dh, NU_NULL);

        /* Remove from mount table */
        mte->mte_cnt--; /* Lower the count */

        /* Mark table entry as unmounted */
        mte->mte_flags &= ~MTE_FL_MOUNTED;

        /* Mark mount table entry as invalid */
        mte->mte_flags &= ~MTE_FL_VALID;
    }

    /* Unlock the mount table */
    LCK_EXIT(LCK_MT_TABLE)

    /* Notify application of device mount */
    if (ret_stat == NU_SUCCESS)
        NU_Set_Events(&FILE_Application_Event_Group,NUF_EVT_MOUNT_DESTROY,NU_OR);

    LCK_FS_EXIT()

    return (ret_stat);
}

/************************************************************************
* FUNCTION
*
*   NU_VFS_Init
*
* DESCRIPTION
*
*   Perform initialization of VFS modules. This service must be completed
*   before using any Nucleus FILE services.
*
* INPUTS
*
*   None
*
* OUTPUTS
*
*   NU_SUCCESS                      VFS was successfully initialized
*   ! NU_SUCCESS                    Error occurred during initialization
*
*************************************************************************/
STATUS NU_VFS_Init(VOID *config_s)
{
STATUS sts = NU_SUCCESS;

    LCK_FS_ENTER()

    if (!config_s)
        sts = NUF_BADPARM;
    else
        FILE_Alloc_Pool = (NU_MEMORY_POOL*)config_s;

    if (sts == NU_SUCCESS)
        /* Initialize the generic locks for VFS structures */
        sts = lck_init_locks();

    if (sts == NU_SUCCESS)
        /* Initialize the file descriptor table */
        sts = fd_init_fd_table();

    if (sts == NU_SUCCESS)
        /* Initialize the disk handles */
        sts = fsdh_init_handles();

    if (sts == NU_SUCCESS)
        sts = fsu_init_users();

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)
    if (sts == NU_SUCCESS)
    {
        sts = bcm_block_cache_feature_init();
    }
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE_REORDER == 1)    
    if (sts == NU_SUCCESS)
    {
        sts = bcmms_feature_init();
    }
#endif /*#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE_REORDER == 1) */    
#endif /*#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1) */

    /* Configure device notification event group */
    if (sts == NU_SUCCESS)
    {
        sts = NU_Create_Event_Group(&FILE_Application_Event_Group, "NUFAEG");
    }

    /* Initialize the device id counter */
    if (sts == NU_SUCCESS)
        FILE_Drive_Id = 1;


    LCK_FS_EXIT()

    return (sts);

}
