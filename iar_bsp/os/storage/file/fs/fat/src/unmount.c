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
*       unmount.c
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*       FAT specific unmount file system operation.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       fat_unmount
*
*************************************************************************/
#include        "storage/fat_defs.h"

extern INT8   FAT_Total_Mounted;
/************************************************************************
* FUNCTION
*
*       fat_unmount
*
* DESCRIPTION
*
*       Performs removal of FAT file system from system.
*
* INPUTS
*
*       dh                                  Disk handle
*
* OUTPUTS
*
*       NU_SUCCESS                          Unmount successful.
*
*************************************************************************/
STATUS fat_unmount (UINT16 dh)
{
    STATUS sts;
    FAT_CB *fs_cb = NU_NULL;
    PC_FS_ENTER()   /* Must be last line in declarations */

    /* Determine if the disk is already opened */
    sts = fsdh_get_fs_specific(dh, (VOID **)&fs_cb);
    if ((sts == NU_SUCCESS) && (fs_cb))
    {
        /* Verify we have a control block */
        if (fs_cb->ddrive)
        {
            /* Unmount a disk. */
            sts = pc_idskclose(dh);
            if (sts == NUF_NOT_OPENED)
                sts = NU_SUCCESS;       /* Not open is not an error here! */

            /* Release the handles */
            pc_dealloc_lock(fs_cb->drive_lock.wait_handle);
            pc_dealloc_lock(fs_cb->drive_io_lock.wait_handle);
            pc_dealloc_lock(fs_cb->fat_lock.wait_handle);

            /* Release the memory for FAT caching */
            if (fs_cb->ddrive->fat_swap_structure.data_array)
                NU_Deallocate_Memory(fs_cb->ddrive->fat_swap_structure.data_array);
            fs_cb->ddrive->fat_swap_structure.data_array = (UINT8 FAR *)0;

            if (fs_cb->ddrive->fat_swap_structure.data_map)
                NU_Deallocate_Memory(fs_cb->ddrive->fat_swap_structure.data_map);
            fs_cb->ddrive->fat_swap_structure.data_map = NU_NULL;

            if (fs_cb->ddrive->fat_swap_structure.pdirty)
                NU_Deallocate_Memory(fs_cb->ddrive->fat_swap_structure.pdirty);
            fs_cb->ddrive->fat_swap_structure.pdirty = NU_NULL;

            /* return the memory */
            NU_Deallocate_Memory((VOID*)fs_cb->ddrive);
        }

        /* return the memory */
        NU_Deallocate_Memory((VOID*)fs_cb);

        /* Decrement the number of FAT drives mounted */
        FAT_Total_Mounted--;
    }

    /* Release the lock */
    PC_FS_EXIT()
    return (sts);
}
