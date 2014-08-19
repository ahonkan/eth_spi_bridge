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
 *       list.c
 *
 * COMPONENT
 *
 *       LIST
 *
 * DESCRIPTION
 *
 *       File system, device, and mounted storage listing services.  The
 *       list functions place the address of the first valid element of
 *       list into the caller passed pointer.  The functions then link
 *       the rest of the list's elements via the 'next' member.  The
 *       caller simply follows the links until the end, an NU_NULL,
 *       is reached.
 *
 *      When the caller is finished with the list, a call to NU_Free_List
 *      must be made to deallocate the list's memory and return it to the
 *      memory pool.
 *
 * DATA STRUCTURES
 *
 *       FSTE_S FS_Table[];
 *       FDEV_S FDev_Table[];
 *       MTE_S MTE_Table[];
 *
 * FUNCTIONS
 *
 *       NU_List_File_System         Get list of registered FS
 *       NU_List_Device              Get list of valid device
 *       NU_List_Mount               Get list of valid mount
 *       NU_Free_List                Return list memory to pool
 *
 *************************************************************************/
#include "storage/pcdisk.h"
#include "storage/lck_extr.h"

extern FSTE_S FS_Table[];
extern FDEV_S FDev_Table[];
extern MTE_S MTE_Table[];

/*****************************************************************************
 * FUNCTION
 *
 *       NU_List_File_System
 *
 * DESCRIPTION
 *      This function returns a linked list of file systems currently
 *      registered.  If the system can not allocate memory for the entire
 *      list, any memory allocated by the call is freed and a NULL
 *      pointer is set for fs_list and the function returns an error.
 *
 * INPUTS
 *      pointer to a pointer of FS_LIST_S
 *
 * OUTPUTS
 *      NU_SUCCESS
 *          *fs_list_head =  Pointer to list of file system currently registered.
 *      NUF_NO_MEMORY
 *          If there isn't enough memory to allocate a FS_LIST_S
 *      NUF_BADPARM
 *          *fs_list_head = NU_NULL
 *
 *****************************************************************************/
STATUS NU_List_File_System(FS_LIST_S **fs_list_head)
{
    INT         idx;
    FS_LIST_S   *fs_current;
    STATUS      rtn_status = NU_SUCCESS;

    LCK_FS_ENTER()

    /* Verify fs_list_head doesn't point to NU_NULL. */
    if(fs_list_head != NU_NULL)
    {
        LCK_ENTER(LCK_FS_TABLE) /* Lock the file system table */
        *fs_list_head = fs_current = NU_NULL;     /* Initialize pointers. */
        for (idx = 0; idx < FSC_MAX_TABLE_SIZE; idx++)
        {        
            /* Find all valid entries */
            if (FS_Table[idx].fste_flags & FSTE_FL_VALID)
            {
                if (!*fs_list_head)
                {   /* If first, setup start of list. */
                    fs_current = NUF_Alloc(sizeof(FS_LIST_S));

                    if (fs_current)
                    {
                        *fs_list_head = fs_current;
                        fs_current->previous = NU_NULL;
                        fs_current->next = NU_NULL;
                    }
                    else
                    {
                        rtn_status = NU_NO_MEMORY;
                        break;                      /* exit for loop. */
                    }
                }
                else
                {   /* Link in new entry. */
                    fs_current->next = NUF_Alloc(sizeof(FS_LIST_S));

                    if (fs_current->next)
                    {
                        fs_current->next->previous = fs_current;
                        fs_current = fs_current->next;
                        fs_current->next = NU_NULL; /* indicate current end of list. */
                    }
                    else
                    {   /* Can not complete the list, return error to call. */
                        NU_Free_List((VOID *)*fs_list_head); /* Free memory in list */
                        *fs_list_head = NU_NULL;
                        rtn_status = NU_NO_MEMORY;
                        break;                          /* exit for loop. */
                    }
                }
                /* Set name pointer */
                fs_current->fs_name = FS_Table[idx].fste_name;
            }
        }
        LCK_EXIT(LCK_FS_TABLE)  /* Unlock FS table */
    }
    else
    {
        rtn_status = NUF_BADPARM;
    }

    LCK_FS_EXIT()

    return (rtn_status);
}

/*****************************************************************************
 * FUNCTION
 *
 *       NU_List_Device
 *
 * DESCRIPTION
 *      This function returns a linked list of devices currently
 *      registered.  If the system can not allocate memory for the entire
 *      list, any memory allocated by the call is freed and a NULL
 *      pointer is set for dev_list and the function returns an error.
 *
 * INPUTS
 *      pointer to a pointer of DEV_LIST_S
 *
 * OUTPUTS
 *      NU_SUCCESS
 *          *dev_list =  Pointer to list of devices currently registered.
 *      NU_NO_MEMORY
 *          *dev_list = NU_NULL
 *
 *****************************************************************************/
STATUS NU_List_Device(DEV_LIST_S **dev_list_head)
{
    INT         idx;
    DEV_LIST_S  *dev_current;
    STATUS      rtn_status = NU_SUCCESS;

    LCK_FS_ENTER()

    /* Verify dev_list_head doesn't point to NU_NULL. */
    if(dev_list_head != NU_NULL)
    {
        LCK_ENTER(LCK_DEV_TABLE)                    /* Lock the dev table */
        *dev_list_head = dev_current = NU_NULL;     /* Initialize pointers. */
        for (idx = 0; idx < FS_DEV_MAX_DEVICES; idx++)
        {
            /* Find all valid entries */
            if (FDev_Table[idx].fdev_flags & FDEV_FL_VALID)
            {
                if (!*dev_list_head)
                {   /* If first, setup start of list. */
                    dev_current = NUF_Alloc(sizeof(DEV_LIST_S));

                    if (dev_current)
                    {
                        *dev_list_head = dev_current;
                        dev_current->previous = NU_NULL;
                        dev_current->next = NU_NULL;
                    }
                    else
                    {
                        rtn_status = NU_NO_MEMORY;
                        break;                      /* exit for loop. */
                    }
                }
                else
                {   /* Link in new entry. */
                    dev_current->next = NUF_Alloc(sizeof(DEV_LIST_S));

                    if (dev_current->next)
                    {
                        dev_current->next->previous = dev_current;
                        dev_current = dev_current->next;
                        dev_current->next = NU_NULL; /* indicate current end of list. */
                    }
                    else
                    {   /* Can not complete the list, return error to call. */
                        NU_Free_List((VOID *)*dev_list_head); /* Free memory in list */
                        *dev_list_head = NU_NULL;
                        rtn_status = NU_NO_MEMORY;
                        break;                          /* exit for loop. */
                    }
                }
                /* Set name pointer */
                dev_current->dev_name = FDev_Table[idx].fdev_name;
            }
        }
        LCK_EXIT(LCK_DEV_TABLE) /* Unlock the device table */
            
    }
    else
    {
        rtn_status = NUF_BADPARM;
    }

    LCK_FS_EXIT()

    return (rtn_status);
}

/*****************************************************************************
 * FUNCTION
 *
 *       NU_List_Mount
 *
 * DESCRIPTION
 *      This function returns a linked list of mounts currently
 *      registered.  If the system can not allocate memory for the entire
 *      list, any memory allocated by the call is freed and a NULL pointer
 *      is set for mount_list_head and the function returns an error.
 *
 * INPUTS
 *      pointer to a pointer of MNT_LIST_S
 *
 * OUTPUTS
 *      NU_SUCCESS
 *          *mount_list_head =  Pointer to list of mounts currently registered.
 *      NU_NO_MEMORY
 *          *mount_list_head = NU_NULL
 *
 *****************************************************************************/
STATUS NU_List_Mount(MNT_LIST_S **mount_list_head)
{
    INT         idx;
    MNT_LIST_S  *mount_current;
    STATUS      rtn_status = NU_SUCCESS;

    LCK_FS_ENTER()

    /* Verify mount_list_head doesn't point to NU_NULL. */
    if(mount_list_head != NU_NULL)
    {
        LCK_ENTER(LCK_MT_TABLE)                         /* Lock the mount table */
        *mount_list_head = mount_current = NU_NULL;     /* Initialize pointers. */
        for (idx = 0; idx < MTE_MAX_TABLE_SIZE; idx++)
        {
            /* Find all valid entries */
            if ((MTE_Table[idx].mte_flags & MTE_FL_VALID) &&
                    (MTE_Table[idx].mte_flags & MTE_FL_MOUNTED))
            {
                if (!*mount_list_head)
                {   /* If first, setup start of list. */
                    mount_current = (MNT_LIST_S *)NUF_Alloc(sizeof(MNT_LIST_S));

                    if (mount_current)
                    {
                        *mount_list_head = mount_current;
                        mount_current->previous = NU_NULL;
                        mount_current->next = NU_NULL;
                    }
                    else
                    {
                        rtn_status = NU_NO_MEMORY;
                        break;                      /* exit for loop. */
                    }
                }
                else
                {   /* Link in new entry. */
                    mount_current->next = (MNT_LIST_S *)NUF_Alloc(sizeof(MNT_LIST_S));

                    if (mount_current->next)
                    {
                        mount_current->next->previous = mount_current;
                        mount_current = mount_current->next;
                        mount_current->next = NU_NULL; /* indicate current end of list. */
                    }
                    else
                    {   /* Can not complete the list, return error to call. */
                        NU_Free_List((VOID *)*mount_list_head); /* Free memory in list */
                        *mount_list_head = NU_NULL;
                        rtn_status = NU_NO_MEMORY;
                        break;                          /* exit for loop. */
                    }
                }
                /* Set name pointer */
                mount_current->mnt_name = MTE_Table[idx].mte_mount_name;
                mount_current->dev_name = MTE_Table[idx].mte_device_name;
                mount_current->fs_name = MTE_Table[idx].mte_fste->fste_name;
                mount_current->config = ((VOID *)MTE_Table[idx].mte_cp);
            }
        }
        LCK_EXIT(LCK_MT_TABLE)  /* Unlock the mount table */
    }
    else
    {
        rtn_status = NUF_BADPARM;
    }

    LCK_FS_EXIT()

    return (rtn_status);
}

/*****************************************************************************
 * FUNCTION
 *
 *       NU_Free_List
 *
 * DESCRIPTION
 *      This function returns memory from a linked list.  The first
 *      members of each list element is assumed to be a pointer to
 *      the next element.  The remaining members are not significant
 *      to this function.
 *
 * INPUTS
 *      a pointer to a pointer of the list to be freed, cast as a VOID.
 *
 * OUTPUTS
 *      NU_SUCCESS
 *
 *****************************************************************************/
STATUS  NU_Free_List(VOID **list)
{
    DEV_LIST_S  *current, *next;
    STATUS ret_val = NU_SUCCESS;

    LCK_FS_ENTER()

    /* Make sure *list pointer doesn't point to NU_NULL. */
    if(list != NU_NULL)
    {
        current = (DEV_LIST_S *)*list;  /* Use list as a DEV_LIST_S entity. */
        *list = NU_NULL;                /* Set callers pointer to NULL. */
        while (current)                 /* loop through list freeing memory. */
        {
            next = current->next;
            NU_Deallocate_Memory((VOID *)current);
            current = next;
        }
    }
    else
    {
        ret_val = NUF_BADPARM;

    }
    LCK_FS_EXIT()

    return (ret_val);
}

