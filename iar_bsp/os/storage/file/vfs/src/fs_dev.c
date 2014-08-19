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
*       fs_dev.c
*
* COMPONENT
*
*       Device
*
* DESCRIPTION
*
*       Device table services and user APIs.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       fs_dev_clear_table_entry
*       fs_dev_init_table
*       fs_dev_devname_to_fdev
*       fs_dev_set_fdev_flags
*       fs_dev_get_fdev_flags
*       NU_Create_File_Device
*       NU_Remove_File_Device
*
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/lck_extr.h"
#include "storage/dev_extr.h"
#include "storage/dh_extr.h"
#include "storage/util_extr.h"
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)
#include "storage/bcm_extr.h"
#endif

FDEV_S FDev_Table[FS_DEV_MAX_DEVICES];
NU_EVENT_GROUP FILE_Application_Event_Group;
extern FSDH_S  FS_Disk_Handles[FSDH_MAX_DISKS];

#define NUCFD_CHECK(X,Y,Z)      \
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
*   fs_dev_clear_table_entry
*
* DESCRIPTION
*
*   Clears a dev table entry
*
* INPUTS
*
*   *fdev                       Pointer to device table entry to be
*                               cleared
*
* OUTPUTS
*
*   None
*
*************************************************************************/
VOID fs_dev_clear_table_entry(FDEV_S *fdev)
{
    /* Verify the input parameters */
    if (fdev)
    {
        /* Clear the entry members */
        fdev->fdev_name[0]            = '\0';
        fdev->fdev_flags              = 0;
        fdev->fdev_cnt                = 0;
        fdev->fdev_ops.open_proc      = NU_NULL;
        fdev->fdev_ops.close_proc     = NU_NULL;
        fdev->fdev_ops.io_proc        = NU_NULL;
        fdev->fdev_ops.ioctl_proc     = NU_NULL;
        fdev->fdev_ops.dskchk_proc    = NU_NULL;

        /* Release the disk handler */
        fsdh_free_dh(fdev->fdev_dh);
    }
}

/************************************************************************
* FUNCTION
*
*   fs_dev_init_table
*
* DESCRIPTION
*
*   Initialize the device table.
*
* INPUTS
*
*   None
*
* OUTPUTS
*
*   NU_SUCCESS                  Always returns success.
*
*************************************************************************/
STATUS fs_dev_init_table()
{
    INT idx;

    /* Initialize the dev table entries */
    for (idx = 0; idx < FS_DEV_MAX_DEVICES; idx++)
    {
        /* Clear this table entry */
        fs_dev_clear_table_entry(&FDev_Table[idx]);
    }

    return (NU_SUCCESS);

}

/************************************************************************
* FUNCTION
*
*   fs_dev_devname_to_fdev
*
* DESCRIPTION
*
*   Search the device table for a valid entry that matches the given 
*   devname.
*     NOTE: Assumes that the device table is locked or adequate
*     protection is taken by the calling function.
*
* INPUTS
*
*   *devname                    String describing device
*   **fdev                      fdev returned associated with devname
*
* OUTPUTS
*
*   NUF_BADPARM                 Invalid parameter
*   NUF_INVALID_DEVNAME         Matching device name was not found
*   NU_SUCCESS                  fdev returned
*
*************************************************************************/
STATUS fs_dev_devname_to_fdev(CHAR *devname, FDEV_S **fdev)
{
    INT     idx;
    STATUS sts = NUF_INVALID_DEVNAME;

    /* Verify input parameters */
    if ((!devname) || (!fdev))
        sts = NUF_BADPARM;
    else
    {
        /* Search the device table for a valid entry that matches
           the given devname */
        for (idx = 0; idx < FS_DEV_MAX_DEVICES; idx++)
        {
            /* Compare the device names to determine a match */
            if ((FDev_Table[idx].fdev_flags & FDEV_FL_VALID) &&
                (NUF_Strncmp(devname, &FDev_Table[idx].fdev_name[0], FS_DEV_MAX_DEVNAME) == 0))
            {
                /* A match was found. Setup the returned FDEV_S
                   structure pointer */
                *fdev = &FDev_Table[idx];
                sts = NU_SUCCESS;
                break;
            }
        }
    }
    return (sts);
}
/************************************************************************
* FUNCTION
*
*   fs_dev_getset_fdev_flags
*
* DESCRIPTION
*
*   Service gets or sets flags for the fdev identified by devname depending
*   on the value of the set parameter.
*
* INPUTS
*
*   *devname                    Unique string describing device
*   *flags                      Flags to be set for this device
*   set                         NU_TRUE = set the flag
*                               NU_FALSE = return the flag
*
* OUTPUTS
*
*   NU_SUCCESS                  Device successfully created
*   NUF_BADPARM                 Invalid parameter
*
*************************************************************************/
STATUS fs_dev_getset_fdev_flags (CHAR *devname, UINT32 *flags, UINT8 set)
{
STATUS sts;
FDEV_S *fdev;

    /* Verify input pointers */
    if ((!devname) || (!flags))
        sts = NUF_BADPARM;
    else
    {
        /* Convert the device name to an fdev structure */
        sts = fs_dev_devname_to_fdev(devname, &fdev);
        if (sts == NU_SUCCESS)
        {
            /* Set the new flags */
            if (set == NU_TRUE)
                fdev->fdev_flags = *flags;
            /* Retrieve the existing flags */
            else if (set == NU_FALSE)
                *flags = fdev->fdev_flags;
            /* Invalid parameter for 'set' passed in */
            else
                sts = NUF_BADPARM;
        }
    }

    return (sts);
}
/************************************************************************
* FUNCTION
*
*   fs_dev_set_fdev_flags
*
* DESCRIPTION
*
*   Service sets flags for the fdev identified by devname.
*
* INPUTS
*
*   *devname                    Unique string describing device
*   flags                       Flags to be set for this device
*
* OUTPUTS
*
*   NU_SUCCESS                  Device successfully created
*   NUF_BADPARM                 Invalid parameter
*
*************************************************************************/
STATUS fs_dev_set_fdev_flags(CHAR *devname, UINT32 flags)
{
    return (fs_dev_getset_fdev_flags(devname, &flags, NU_TRUE));
}
/************************************************************************
* FUNCTION
*
*   fs_dev_get_fdev_flags
*
* DESCRIPTION
*
*   Service gets flags for the fdev identified by devname.
*
* INPUTS
*
*   *devname                    Unique string describing device
*   *flags                      Flags currently set for this device
*
* OUTPUTS
*
*   NU_SUCCESS                  Device successfully created
*   NUF_BADPARM                 Invalid parameter
*
*************************************************************************/
STATUS fs_dev_get_fdev_flags(CHAR *devname, UINT32 *flags)
{
    return (fs_dev_getset_fdev_flags(devname, flags, NU_FALSE));
}


/************************************************************************
* FUNCTION
*
*   NU_Create_File_Device
*
* DESCRIPTION
*
*   Create a device for use with a file system
*
* INPUTS
*
*   *devname                    Unique string describing device
*   *dev_ops                    Device operations structure to be used
*                               with the newly created device.
*   *args                       Device specific arguments for
*
* OUTPUTS
*
*   NU_SUCCESS                  Device successfully created
*   NUF_BADPARM                 Invalid parameter
*   NUF_DEVICE_TABLE_FULL       Device table is full
*   NUF_DUPLICATE_DEVNAME       Device name is not unique
*   NUF_IO_ERROR                Device open returned an error
*
*************************************************************************/
STATUS NU_Create_File_Device(CHAR *devname, FDEV_OP_S *dev_ops, VOID *args)
{
FDEV_S  *fdev;
FDEV_S  *free_fdev = NU_NULL;
FSDH_S  *dhs = NU_NULL;
INT     idx;
STATUS  dev_sts;
UINT16  dh;
STATUS  sts = NU_SUCCESS;

    LCK_FS_ENTER()

    /* Verify input parameters */
    if (!devname)
        sts = NUF_BADPARM;
    else if (!dev_ops)
        sts = NUF_BADPARM;

    /* Get a device table entry for the new device */
    if (sts == NU_SUCCESS)
    {
        /* Lock the dev table */
        LCK_ENTER(LCK_DEV_TABLE)

        /* Verify that the device doesn't already exist in the table */
        if (fs_dev_devname_to_fdev(devname, &fdev) == NUF_INVALID_DEVNAME)
        {

            /* Find an empty entry in the device table */
            for (idx = 0; idx < FS_DEV_MAX_DEVICES; idx++)
            {
                /* Device entry not valid, empty entry is found  and not in
                   use */
                if ( (!(FDev_Table[idx].fdev_flags & FDEV_FL_VALID)) &&
                     (FDev_Table[idx].fdev_cnt == 0) )
                {
                    /* Setup pointer to an empty table entry */
                    free_fdev = &FDev_Table[idx];

                    /* Update the usage count while the table lock is held */
                    free_fdev->fdev_cnt++;
                    break;
                }
            }

            /* If no empty entries found, the device table is full */
            if (!free_fdev)
               sts = NUF_DEVICE_TABLE_FULL;
        }
        else
            /* A duplicate device name exists in the table */
            sts = NUF_DUPLICATE_DEVNAME;

        /* Unlock the device table */
        LCK_EXIT(LCK_DEV_TABLE)

    }

    /* Setup the device table entry */
    if (sts == NU_SUCCESS)
    {
        /* Copy the device name */
        NUF_Copybuff(&free_fdev->fdev_name[0], devname, FS_DEV_MAX_DEVNAME);

        /* Allocate a disk handle */
        sts = fsdh_allocate_dh(&dh);

        if (sts == NU_SUCCESS)
        {
            /* Get a disk handle struct */
            sts = fsdh_get_fsdh_struct(dh, &dhs);

            if (sts == NU_SUCCESS)
            {
                /* Associate the disk handle with a device */
                dhs->fsdh_fdev = free_fdev;

                /* Assign the disk handle in the device */
                free_fdev->fdev_dh = dh;

                sts = fsdh_set_dh_specific(dh, NU_NULL, free_fdev->fdev_name);
            }
            else
                fsdh_free_dh(dh);

        }

        if (sts != NU_SUCCESS)
        {
            /* Lower the device count */
            LCK_ENTER(LCK_DEV_TABLE)
            free_fdev->fdev_cnt--;
            LCK_EXIT(LCK_DEV_TABLE)
        }
    }

    /* Copy dispatch routines. Dispatch routines can point to null */
    if (sts == NU_SUCCESS)
    {
        free_fdev->fdev_ops.open_proc = dev_ops->open_proc;
        NUCFD_CHECK(sts, free_fdev->fdev_ops.close_proc , dev_ops->close_proc);
        NUCFD_CHECK(sts, free_fdev->fdev_ops.io_proc    , dev_ops->io_proc);
        NUCFD_CHECK(sts, free_fdev->fdev_ops.ioctl_proc , dev_ops->ioctl_proc);
        NUCFD_CHECK(sts, free_fdev->fdev_ops.dskchk_proc, dev_ops->dskchk_proc);

        if (sts != NU_SUCCESS)
        {
            /* Lower the device count */
            LCK_ENTER(LCK_DEV_TABLE)
            free_fdev->fdev_cnt--;
            LCK_EXIT(LCK_DEV_TABLE)

            /* Return the disk handle */
            fsdh_free_dh(dh);

        }
    }

    /* Mark the table entry as valid */
    if (sts == NU_SUCCESS)
    {
        free_fdev->fdev_flags |= FDEV_FL_VALID;
    }

    /* Call the devices open routine */
    if (sts == NU_SUCCESS)
    {
        /* Verify an init routine exists */
        if (free_fdev->fdev_ops.open_proc)
        {
            /* Dispatch the device init routine */
            dev_sts = free_fdev->fdev_ops.open_proc(dh, devname, args);

            /* Manage the device return types */
            if (dev_sts != NU_SUCCESS)
            {
                /* Mark the table entry as invalid */
                free_fdev->fdev_flags &= !FDEV_FL_VALID;

                /* Set our count back to zero. */
                free_fdev->fdev_cnt--;

                /* Return the disk handle */
                fsdh_free_dh(dh);

                sts = dev_sts;
            }
            else
            {
                sts = NU_SUCCESS;
            }
        }
    }

    if(sts == NU_SUCCESS)
    {
        /* Lower the usage count as the device is valid, but
        not yet mounted. It can be removed while not
        in use or currently mounted */
        LCK_ENTER(LCK_DEV_TABLE)
        free_fdev->fdev_cnt--;
        LCK_EXIT(LCK_DEV_TABLE)

        /* Set application notify event */
        NU_Set_Events(&FILE_Application_Event_Group,NUF_EVT_DEVICE_CREATE,NU_OR);

    }

    LCK_FS_EXIT()

    return (sts);
}

/************************************************************************
* FUNCTION
*
*   NU_Remove_File_Device
*
* DESCRIPTION
*
*   Remove a file storage device that has previously been created. The
*   device must not be in use for this service to successfully complete.
*
* INPUTS
*
*   devname                     Device name to be removed
*
* OUTPUTS
*
*   NU_SUCCESS                  Removal successful
*   NUF_BADPARM                 Invalid devname pointer
*   NUF_INVALID_DEVNAME         Devname not found
*   NUF_IN_USE                  Device is in use
*
*************************************************************************/
STATUS NU_Remove_File_Device(CHAR *devname)
{
FDEV_S  *fdev;
STATUS  sts = NU_SUCCESS;

    LCK_FS_ENTER()

    /* Verify parameter */
    if (!devname)
        sts = NUF_BADPARM;

    /* Lock the device table */
    LCK_ENTER(LCK_DEV_TABLE)

    if (sts == NU_SUCCESS)
    {
        /* Get the device table entry */
        sts = fs_dev_devname_to_fdev(devname, &fdev);

        if (sts == NU_SUCCESS)
        {
            /* Verify that the device in not in use */
            if (fdev->fdev_cnt != 0)
            {
                sts = NUF_IN_USE;
            }
            else
                sts = fs_dev_close_proc(fdev->fdev_dh);
        }

        /* Clear the device table entry */
        if (sts == NU_SUCCESS)
            /* Also releases the disk handle allocated during
               create device. */
            fs_dev_clear_table_entry(fdev);

    }

    /* Unlock the device table */
    LCK_EXIT(LCK_DEV_TABLE)

    /* Set application notify event */
    if (sts == NU_SUCCESS)
        NU_Set_Events(&FILE_Application_Event_Group,NUF_EVT_DEVICE_DESTROY,NU_OR);


    LCK_FS_EXIT()

    return (sts);
}
/************************************************************************
* FUNCTION
*
*   fs_dev_open_proc
*
* DESCRIPTION
*
*   Wrapper routine for dispatching device driver operations.
*
* INPUTS
*
*   dh                              Disk handle assigned to device
*   *devname                        Pointer to device name
*   *args                           Device specific arguments
*
* OUTPUTS
*
*   NU_SUCCESS                      Device successfully opened
*   NUF_BADPARM                     Invalid disk handle
*   < 0                             Device specific error
*
*************************************************************************/
STATUS fs_dev_open_proc(UINT16 dh, CHAR *devname, VOID *args)
{
STATUS sts;

    /* Verify input parameters */
    FSDH_VERIFY_DH(dh, sts);
    if(sts == NU_SUCCESS)
    {
        if(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_fdev->fdev_ops.open_proc != NU_NULL)
        {
            sts = FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_fdev->fdev_ops.open_proc(dh, devname, args);
        }
    }

    return (sts);
}

/************************************************************************
* FUNCTION
*
*   fs_dev_close_proc
*
* DESCRIPTION
*
*   Wrapper routine for dispatching the device close operation.
*
* INPUTS
*
*   dh                              Disk handle assigned to device
*
* OUTPUTS
*
*   NU_SUCCESS                      Device successfully closed
*   NUF_BADPARM                     Invalid disk handle
*   < 0                             Device specific error
*
*************************************************************************/
STATUS fs_dev_close_proc(UINT16 dh)
{
STATUS sts;

    /* Verify input parameters */
    FSDH_VERIFY_DH(dh, sts);
    if(sts == NU_SUCCESS)
        sts = FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_fdev->fdev_ops.close_proc(dh);

    return (sts);
}

/************************************************************************
* FUNCTION
*
*   fs_dev_io_proc
*
* DESCRIPTION
*
*   Wrapper for requesting device IO operation
*
* INPUTS
*
*   dh                              Disk handle assigned to device
*   sector                          Logical base sector for IO request
*   *buffer                         Pointer to memory for source or
*                                   destination
*   count                           Number of 512-byte blocks requested
*   reading                         Type of operation requested
*
* OUTPUTS
*
*   NU_SUCCESS                      IO request successfully completed
*   NUF_BADPARM                     Invalid parameter given
*   < 0                             Device specific error
*
*************************************************************************/
STATUS fs_dev_io_proc(UINT16 dh, UINT32 sector, VOID *buffer, UINT16 count, INT reading)
{
STATUS sts;
OPTION preempt_status;

    /* Verify input parameters */
    FSDH_VERIFY_DH(dh, sts);
    if(sts == NU_SUCCESS)
    {
        if (FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_sema)
        {
            /* Change task to no preemption.  */
            preempt_status = NU_Change_Preemption(NU_NO_PREEMPT);

            /* See if we can obtain the io mutex. */
            sts = NU_Obtain_Semaphore(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_sema, NU_NO_SUSPEND);

            /* Another task is already suspended on this mutex so release file system semaphore. */
            if(sts == NU_UNAVAILABLE)
            {
                fs_release(dh);

                /* Set our preemption status back to what it originally was. */
                NU_Change_Preemption(preempt_status);
                /* Wait until we have the io mutex. */
                NU_Obtain_Semaphore(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_sema, NU_SUSPEND);

                /* Reclaim FS semaphore. */
                fs_reclaim(dh);       
                               
            }
            else
            {
                /* Set our preemption status back to what it originally was. */
                NU_Change_Preemption(preempt_status);

            }
            
        }
        
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)   
        (VOID) NU_Obtain_Semaphore(&(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_vfs_sema), NU_SUSPEND);

        if (reading == YES)
        {
            sts = bcm_device_read_request(dh, sector, buffer, count);
        }
        else
        {
            sts = bcm_device_write_request(dh, sector, buffer, count);
        }
        
        (VOID) NU_Release_Semaphore(&(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_vfs_sema));
#else        
        sts = FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_fdev->fdev_ops.io_proc(dh, sector, buffer, count, reading);
#endif
        if(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_sema)
        {   
            NU_Release_Semaphore(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_sema);
        }
    }
    return (sts);
}

/************************************************************************
* FUNCTION                                                              
*     
*   fs_dev_ioctl_proc   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Wrapper routine for requesting IOCTL functions
*                                                                       
* INPUTS                                                                
*   
*   dh
*   command
*   *buffer
*
* OUTPUTS
*       
*   NU_SUCCESS
*   NUF_BADPARM
*   < 0
*                                                                       
*************************************************************************/
STATUS fs_dev_ioctl_proc(UINT16 dh, UINT16 command, VOID *buffer, INT ioctl_data_len)
{
STATUS sts;

    /* Verify input parameters */
    FSDH_VERIFY_DH(dh, sts);
    if(sts == NU_SUCCESS)
        sts = FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_fdev->fdev_ops.ioctl_proc(dh, command, buffer, ioctl_data_len);

    return (sts);
}

/************************************************************************
* FUNCTION                                                              
*     
*   fs_dev_dskchk_proc
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Wrapper routine for verifying device presence.
*                                                                       
* INPUTS                                                                
*   
*   dh
*
* OUTPUTS
*       
*   NU_SUCCESS
*   NUF_BADPARM
*   < 0
*                                                                       
*************************************************************************/
STATUS fs_dev_dskchk_proc(UINT16 dh)
{
STATUS sts;

    
    /* Verify input parameters */
    FSDH_VERIFY_DH(dh, sts);
    if(sts == NU_SUCCESS)
    {
        if (FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_fdev->fdev_ops.dskchk_proc)
            /* Dispatch the device specific operation */
            sts = FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_fdev->fdev_ops.dskchk_proc(dh);
        else
            /* Operation not required, so assume device is available */
            sts = NU_SUCCESS;
    }

    return (sts);
}
