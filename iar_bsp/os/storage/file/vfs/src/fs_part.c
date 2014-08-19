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
*       fs_part.c
*
* COMPONENT
*
*       Partition management
*
* DESCRIPTION
*
*       Partition table services and user APIs.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Get_Partition_Info
*       NU_List_Partitions
*       NU_Free_Partition_List
*       NU_Create_Partition
*       NU_Delete_Partition
*       fpart_mount_check
*       fpart_cyl_read
*
*************************************************************************/

#include "storage/pcdisk.h"
#include "storage/part_extr.h"
#include "storage/lck_extr.h"
#include "storage/fsl_extr.h"
#include "storage/dev_extr.h"
#include "storage/list.h"

/************************************************************************
* FUNCTION
*
*   NU_Get_Partition_Info
*
* DESCRIPTION
*
*   Given a valid device name, returns the partition type (logical,
*   primary, or extended); size in MiB; offset in MiB; and the
*   partition identifier as written in the partition table.
*
* INPUTS
*
*   *log_dev_name               Unique string describing device.
*   *ret_part_type              Value to return partition type.
*   *ret_size                   Value to return partition size.
*   *ret_offset                 Value to return partition offset.
*   *ret_offset_units           Units of offset in powers of two
*                                   (offset * 2^offset_units)
*   *ret_part_id                Value to return partition type id.
*
* OUTPUTS
*
*   NUF_INVALID_DEVNAME         Matching device name was not found
*   NU_SUCCESS                  fdev returned
*
*************************************************************************/
STATUS NU_Get_Partition_Info(CHAR *log_dev_name, UINT16 *ret_part_type,
                             UINT32 *ret_size, UINT32 *ret_offset,
                             UINT8 *ret_offset_units, UINT8 *ret_part_id)
{
    STATUS ret_stat;
    FDEV_S *log_fdev;
    CHAR phys_dev_name[FPART_MAX_PHYS_NAME];
    PFPART_LIST_S plist, list_head = NU_NULL;

    LCK_FS_ENTER()

    /* Setup local variables */
    log_fdev = NU_NULL;
    plist = NU_NULL;

    /* Check input parameters */
    if ((!log_dev_name) || (!ret_part_type) ||
        (!ret_size) || (!ret_offset) || (!ret_offset_units) ||
        (!ret_part_id))
        ret_stat = NUF_BADPARM;
    else
        /* Get the device table entry */
        ret_stat = fs_dev_devname_to_fdev(log_dev_name, &log_fdev);

    if (ret_stat == NU_SUCCESS)
    {
        NUF_Copybuff(phys_dev_name, log_dev_name, FPART_MAX_PHYS_NAME);
        phys_dev_name[FPART_MAX_PHYS_NAME - 1] = NU_NULL;

        /* Get a list of all partitions on the physical device */
        ret_stat = NU_List_Partitions(phys_dev_name, &list_head);

    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Set the ret_status to an error. */
        ret_stat = NUF_INVALID_DEVNAME;     /* Could not find partition requested */

        /* Find the specific partition by matching the device name */
        for(plist = list_head; plist != NU_NULL; plist = plist->fpart_next)
        {
            if(NUF_Strncmp(plist->fpart_name, log_dev_name, FPART_MAX_LOG_NAME) == NU_SUCCESS)
            {
                /* Found the matching partition */
                ret_stat = NU_SUCCESS;
                break;
            }
        }
    }

    if(ret_stat == NU_SUCCESS)
    {
        /* Set the offset */
        *ret_offset = plist->fpart_offset;

        /* Set the offset units */
        *ret_offset_units = plist->fpart_offset_units;

        /* Set the partition size in MiB */
        *ret_size = plist->fpart_size;

        /* Set the partition type */
        *ret_part_type = (UINT16) plist->fpart_type;

        /* Set the partition type identifier */
        *ret_part_id = plist->fpart_ent.fpart_id;
    }

    /* Free the list. */
    if (list_head)
        NU_Free_Partition_List(list_head);

    LCK_FS_EXIT()

    return(ret_stat);
}

/************************************************************************
* FUNCTION
*
*   NU_List_Partitions
*
* DESCRIPTION
*
*   Given a valid device name, returns an ordered, linked list of
*   partitions existing on a physical disk. The list is ordered by
*   offset. The clean-up routine, NU_Free_Partition_List, must be called
*   after the list is no longer needed in order to free memory. ret_list
*   is set to NU_NULL on error and any memory used to allocate the
*   list is freed.
*
* INPUTS
*
*   *dev_name                   Unique string describing device.
*   *ret_list                   Pointer to first partition list element.
*
* OUTPUTS
*
*   NUF_INVALID_DEVNAME         Matching device name was not found.
*   NUF_INTERNAL                Nucleus FILE internal error.
*   NUF_NO_MEMORY               Can't allocate internal buffer.
*   NUF_BADPARM                 Invalid parameter specified.
*   NU_SUCCESS                  list returned
*
*************************************************************************/
STATUS NU_List_Partitions(CHAR *dev_name, PFPART_LIST_S *ret_list)
{
    FPART_TABLE_S part;
    STATUS ret_stat, extended_flag = NU_FALSE;
    FDEV_S *fdev;
    INT8   buf[512], *pbuf;
    UINT32 partition_address, extended_base_address;
    UINT16 signature, up2_cyl;
    INT    i;
    FPART_DISK_INFO_S disk_info;
    PFPART_LIST_S p_list = NU_NULL, prev_node = NU_NULL, list_head = NU_NULL;
    CHAR part_name[FPART_MAX_LOG_NAME];
    UINT32      rsec = 0;
    UINT32      psize;

    LCK_FS_ENTER()

    /* Set the partition start address */
    partition_address = 0L;

    /* Verify input parameters */
    if ((!dev_name) || (!ret_list))
        ret_stat = NUF_BADPARM;
    else
        /* Get the device table entry */
        ret_stat = fs_dev_devname_to_fdev(dev_name, &fdev);

    if (ret_stat == NU_SUCCESS)
    {
        NUF_Copybuff(part_name, dev_name, FPART_MAX_PHYS_NAME);
        part_name[3] = part_name[4] = '0';

        /* Get the device specific information */
        ret_stat = fs_dev_ioctl_proc(fdev->fdev_dh, FDEV_GET_DISK_INFO, (VOID *)&disk_info, sizeof(FPART_DISK_INFO_S));
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Check if this is a SAFE device, as SAFE devices do not support DOS partitions*/
        if (disk_info.fpart_flags & FPART_DI_SAFE)
        {
            ret_stat = NUF_NO_PARTITION;
        }
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Calculate the number of sectors per cylinder */
        if (disk_info.fpart_cyls == 0)
            ret_stat = NUF_INTERNAL;        /* invalid disk info */
    }
    if (ret_stat == NU_SUCCESS)
    {
        /* Read DOS partition data */
        ret_stat = fs_dev_io_proc(fdev->fdev_dh, partition_address, buf , 1 , NU_TRUE);
    }
    if (ret_stat == NU_SUCCESS)
    {
        /* Copy the table to a word aligned buffer so some compilers don't screw up */
        pbuf = &buf[0x1be];     /* The info starts at buf[1be] */
        NUF_Copybuff((VOID*)&part, (VOID*)pbuf, sizeof(FPART_TABLE_S));

        /* Check the signature word */
        SWAP16(&signature,&part.fpart_signature);
        if (signature == 0xAA55)
        {
            for(i = 0; i < 4; i++)
            {
                /* Get partition size. */
                SWAP32(&psize, &part.fpart_ents[i].fpart_size);

                if ((part.fpart_ents[i].fpart_id == 0x00) ||
                    ((part.fpart_ents[i].fpart_boot != 0x00) &&
                     (part.fpart_ents[i].fpart_boot != 0x80)) ||
                    (psize == 0) || (psize > disk_info.fpart_tot_sec))
                {
                    /* Invalid table entry, continue looking for valid partitions */
                    continue;
                }
                /* Allocate a list structure */
                p_list = (PFPART_LIST_S)NUF_Alloc(sizeof(FPART_LIST_S));

                if(p_list == NU_NULL)
                {
                    ret_stat = NUF_NO_MEMORY;   /* not enough memory */
                    break;
                }
                /* Copy the table entry into the structure */
                NUF_Copybuff((VOID*)&p_list->fpart_ent, (VOID*)&part.fpart_ents[i], sizeof(FPART_TAB_ENT_S));

                /* Assign type */
                if ((p_list->fpart_ent.fpart_id == 0x05) ||
                    (p_list->fpart_ent.fpart_id == 0x0F) )
                {
                    p_list->fpart_type = FPART_EXTENDED;
                    /* There is an extended partition */
                    extended_flag = NU_TRUE;
                }
                else
                {
                    p_list->fpart_type = FPART_PRIMARY;
                }

                /* Get relative sector. */
                SWAP32(&rsec, &p_list->fpart_ent.fpart_r_sec);

                /* Assign size in MiB */
                p_list->fpart_size = (psize / 0x800) + /* sec to MiB */
                           (((psize * 0x200) % 0x100000) / 0x80000); /* round up */

                /* Assign offset and determine units */
                if((rsec / 0x800) > 0xAL)
                {
                    /* Store the offset in MiB */
                    p_list->fpart_offset = (rsec / 0x800) + /* to MiB */
                                     ((rsec * 0x200 % 0x100000) / 0x80000); /* round up */
                    p_list->fpart_offset_units = 0x14; /* 2^0x14 */
                }
                else
                {
                    /* Store the offset in KB */
                    p_list->fpart_offset = (rsec * 0x200 / 0x400) + /* to KB */
                                     ((rsec * 0x200 % 0x400) / 0x200); /* round up */
                    p_list->fpart_offset_units = 0xA; /* 2^0xA */
                }

                /* Assign the beginning and ending sector */
                p_list->fpart_start = rsec;
                p_list->fpart_end = p_list->fpart_start + psize - 1;

                /* Save relative sector and partition size. */
                p_list->fpart_ent.fpart_r_sec = rsec;
                p_list->fpart_ent.fpart_size = psize;

                /* Link into the list */
                p_list->fpart_prev = prev_node;
                p_list->fpart_next = NU_NULL;
                if(prev_node != NU_NULL)
                {
                    prev_node->fpart_next = p_list;
                }

                prev_node = p_list;
            }
        }
    }
    /* Check if an extended partition was found */
    if ((ret_stat == NU_SUCCESS) && (extended_flag == NU_TRUE))
    {
        /* traverse to the extended partition in the list */
        while((p_list->fpart_type != FPART_EXTENDED) && (p_list->fpart_prev))
            p_list = p_list->fpart_prev;

        /* copy the current list entry */
        prev_node = p_list;
        /* Set the partition start address */
        extended_base_address = partition_address = rsec;
        do
        {
            /* Read DOS partition data */
            ret_stat = fs_dev_io_proc(fdev->fdev_dh, partition_address, buf , 1 , NU_TRUE);
            if (ret_stat != NU_SUCCESS)
                break;  /* Error, we're out of here. */

            /* Copy the table to a word aligned buffer so some compilers don't screw up */
            pbuf = &buf[0x1be];     /* The info starts at buf[1be] */
            NUF_Copybuff((VOID*)&part, (VOID*)pbuf, sizeof(FPART_TABLE_S));
            /* Check the signature word and valid entry. */
            SWAP16(&signature,&part.fpart_signature);
            if ((signature == 0xAA55) && (part.fpart_ents[0].fpart_id != 0x00))
            {
                /* The first entry could be a link to another logical partition */
                if ((part.fpart_ents[0].fpart_id == 0x05) ||
                      (part.fpart_ents[0].fpart_id == 0x0F))
                {
                    /* Get relative sector. */
                    SWAP32(&rsec, &part.fpart_ents[0].fpart_r_sec);

                    /* Add relative sector. */
                    partition_address += rsec;
                }
                else
                {
                    p_list = (PFPART_LIST_S)NUF_Alloc(sizeof(FPART_LIST_S));

                    if(p_list == NU_NULL)
                    {
                        ret_stat = NUF_NO_MEMORY; /* not enough memory */
                        break;
                    }
                    NUF_Copybuff((VOID*)&p_list->fpart_ent, (VOID*)&part.fpart_ents[0], sizeof(FPART_TAB_ENT_S));
                    /* type */
                    p_list->fpart_type = FPART_LOGICAL;

                    /* Get relative sector. */
                    SWAP32(&rsec, &p_list->fpart_ent.fpart_r_sec);

                    /* Get partition size. */
                    SWAP32(&psize, &p_list->fpart_ent.fpart_size);

                    /* size MiB */
                    p_list->fpart_size = (psize / 0x800) + /* sec to MiB */
                                   (((psize * 0x200) % 0x100000) >= 0x80000); /* round up */

                    /* offset */
                    if(((partition_address + rsec) / 0x800) > 0xAL)
                    {
                        /* Store the offset in MiB */
                        p_list->fpart_offset = ((partition_address + rsec) / 0x800) + /* to MiB */
                                         (((partition_address + rsec) * 0x200 % 0x100000) / 0x80000); /* round up */
                        p_list->fpart_offset_units = 0x14; /* 2^0x14 */
                    }
                    else
                    {
                        /* Store the offset in KB */
                        p_list->fpart_offset = ((partition_address + rsec) * 0x200 / 0x400) + /* to KB */
                                         (((partition_address + rsec) * 0x200 % 0x400) / 0x200); /* round up */
                        p_list->fpart_offset_units = 0xA; /* 2^0xA */
                    }

                    /* Assign the beginning and ending sector */
                    p_list->fpart_start = partition_address;
                    p_list->fpart_end = p_list->fpart_start + psize + rsec - 1;


                    /* insert the new entry into the list */
                    p_list->fpart_prev = prev_node;
                    if(prev_node != NU_NULL)
                    {
                        p_list->fpart_next = prev_node->fpart_next;
                        prev_node->fpart_next = p_list;
                    }
                    else
                    {
                        p_list->fpart_next = NU_NULL;
                    }

                    if(p_list->fpart_next != NU_NULL)
                    {
                        p_list->fpart_next->fpart_prev = p_list;
                    }
                    prev_node = p_list;  /* copy the current to previous */

                    /* Save relative sector and partition size. */
                    p_list->fpart_ent.fpart_r_sec = rsec;
                    p_list->fpart_ent.fpart_size = psize;

                    /* Does second entry describe an extended partition. */
                    if ((part.fpart_ents[1].fpart_id == 0x05) ||
                        (part.fpart_ents[1].fpart_id == 0x0F))
                    {
                        /* Get relative sector. */
                        SWAP32(&rsec, &part.fpart_ents[1].fpart_r_sec);

                        /* set the address of the next partition table to read */
                        partition_address = extended_base_address + rsec;
                    }
                    else
                    {
                        break;  /* no, exit do loop. */
                    }
                }
            }
            else
                break;  /* signature bad or empty extended partition. */
        } while(ret_stat == NU_SUCCESS);
    }
    if (ret_stat == NU_SUCCESS)
    {
        if(p_list != NU_NULL)
        {
            /* traverse back to start of list */
            while(p_list->fpart_prev != NU_NULL)
                p_list = p_list->fpart_prev;

            list_head = p_list;
            /* Assign names */
            for(i = 0; p_list != NU_NULL; i++, p_list = p_list->fpart_next)
            {
                part_name[3] = '0' + (CHAR) (i / 10);
                part_name[4] = '0' + (CHAR) (i % 10);
                part_name[5] = NU_NULL;
                NUF_Copybuff(p_list->fpart_name, part_name, FPART_MAX_LOG_NAME);
            }
        }
        else if (disk_info.fpart_flags & FPART_DI_RMVBL_MED)
        {
            /* No partitions were found on the removable disk. */
            p_list = (PFPART_LIST_S)NUF_Alloc(sizeof(FPART_LIST_S));

            if(p_list != NU_NULL)
            {
                list_head = p_list;
                /* Set up a single partition entry based
                   on the physical disk characteristics. */
                p_list->fpart_ent.fpart_boot   = 0xFF;
                p_list->fpart_ent.fpart_s_head = 0x00;
                p_list->fpart_ent.fpart_s_sec  = 0x01;
                p_list->fpart_ent.fpart_s_cyl  = 0x00;

                p_list->fpart_ent.fpart_id = 0x00;

                /* Upper two bits of cylinder are in sector field */
                p_list->fpart_ent.fpart_e_head = (UINT8) (disk_info.fpart_heads - 1);
                up2_cyl = (UINT16) ((disk_info.fpart_cyls - 1)>>8);
                p_list->fpart_ent.fpart_e_sec = (UINT8) (disk_info.fpart_secs | (up2_cyl<<6));
                p_list->fpart_ent.fpart_e_cyl = (UINT8) (((disk_info.fpart_cyls - 1)<<2)>>2);
                p_list->fpart_ent.fpart_r_sec = 0x00;
                p_list->fpart_ent.fpart_size  = disk_info.fpart_tot_sec;
                /* type */
                p_list->fpart_type = FPART_PRIMARY;
                /* size MiB */
                p_list->fpart_size = (p_list->fpart_ent.fpart_size / 0x800) + /* sec to MiB */
                           (((p_list->fpart_ent.fpart_size * 0x200) % 0x100000) / 0x80000); /* round up */
                /* offset */
                if((p_list->fpart_ent.fpart_r_sec / 0x800) > 0xAL)
                {
                    /* Store the offset in MiB */
                    p_list->fpart_offset = (p_list->fpart_ent.fpart_r_sec / 0x800) + /* to MiB */
                                     ((p_list->fpart_ent.fpart_r_sec * 0x200 % 0x100000) / 0x80000); /* round up */
                    p_list->fpart_offset_units = 0x14; /* 2^0x14 */
                }
                else
                {
                    /* Store the offset in KB */
                    p_list->fpart_offset = (p_list->fpart_ent.fpart_r_sec * 0x200 / 0x400) + /* to KB */
                                     ((p_list->fpart_ent.fpart_r_sec * 0x200 % 0x400) / 0x200); /* round up */
                    p_list->fpart_offset_units = 0xA; /* 2^0xA */
                }
                p_list->fpart_start = p_list->fpart_ent.fpart_r_sec;
                p_list->fpart_end = p_list->fpart_start + p_list->fpart_ent.fpart_size - 1;

                p_list->fpart_prev = NU_NULL;
                p_list->fpart_next = NU_NULL;
                part_name[3] = '0';
                part_name[4] = '0';
                part_name[5] = NU_NULL;
                NUF_Copybuff(p_list->fpart_name, part_name, FPART_MAX_LOG_NAME);
            }
            else
            {
                ret_stat = NUF_NO_MEMORY;   /* not enough memory */
            }
        }
        /* set the return pointer */
        *ret_list = list_head;
    }
    else
    {
        /* An error occurred. Try to free up the list. */
        NU_Free_Partition_List(prev_node);
        if (ret_list)
            *ret_list = NU_NULL;
    }

    LCK_FS_EXIT()

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*   NU_Free_Partition_List
*
* DESCRIPTION
*
*   Given a partition list element, traverse to the beginning of
*   the list and free all subsequent list elements.
*
* INPUTS
*
*   *p_list                     Pointer to a partition list element.
*
* OUTPUTS
*
*   None
*
*************************************************************************/
VOID NU_Free_Partition_List(PFPART_LIST_S p_list)
{
    PFPART_LIST_S next;

    LCK_FS_ENTER()

    /* Traverse to beginning of list */
    while((p_list != NU_NULL) && (p_list->fpart_prev != NU_NULL))
    {
        p_list = p_list->fpart_prev;
    }

    /* Deallocate the list */
    while(p_list)
    {
        next = p_list->fpart_next;
        NU_Deallocate_Memory((VOID*)p_list);
        p_list = next;
    }

    LCK_FS_EXIT()

}


/************************************************************************
* FUNCTION
*
*   NU_Create_Partition
*
* DESCRIPTION
*
*   Given a valid device name, create the specified partition on the
*   device. size and offset can both be unspecified. If size is zero,
*   then the size of the free area is calculated. If offset is
*   unspecified (0xFFFFFFFF), then the first free area on the disk
*   is used. The values specified in size and offset may be adjusted so
*   the start and end of the partition lie on cylinder boundaries (i.e.
*   cylinder snapped).
*
* INPUTS
*
*   *dev_name                   Unique string describing device.
*   part_type                   Primary (0), extended (1), or logical (2)
*   size                        Unspecified (0), or a value in
*                                MiB (2^20 bytes). Size may be adjusted
*                                to fall on a cylinder boundary.
*   offset                      Unspecified (0xFFFFFFFF), or a value in
*                                MiB (2^20 bytes). Offset may be adjusted
*                                to fall on a cylinder boundary.
*   part_id                     The partition identifier written into
*                                the partition table. This value must
*                                correspond to the file system type that
*                                the partition will be formatted with.
*
* OUTPUTS
*
*   NUF_INVALID_DEVNAME         Matching device name was not found.
*   NUF_INTERNAL                Nucleus FILE internal error.
*   NUF_NO_MEMORY               Can't allocate internal buffer.
*   NUF_BADPARM                 Invalid parameter given.
*   NUF_PART_TABLE_FULL         Only four primaries, or one extended
*                                and three primaries can exist.
*   NUF_PART_EXT_EXISTS         Only one ext part can exist.
*   NUF_PART_NO_EXT             Log parts can only be created within
*                                an ext part.
*   NU_SUCCESS                  Partition created.
*
*************************************************************************/
STATUS NU_Create_Partition(CHAR *dev_name, UINT16 part_type, UINT32 size, UINT32 offset, UINT8 part_id)
{
    STATUS ret_stat;
    FDEV_S *fdev;
    FPART_DISK_INFO_S disk_info;
    UINT8  num_pri_part = 0, num_ext_part = 0, num_log_part = 0, num_parts, i;
    UINT32 offset_sec = 0, size_sec = 0, partition_address;
    UINT32 cur_size_sec, cur_offset_sec, next_offset_sec, base_sec;
    INT found = NU_FALSE, pad, preceed_flag;
    FPART_LIST_S ext_part_ent;
    PFPART_LIST_S p_list = NU_NULL, list_head = NU_NULL, p_temp = NU_NULL;
    FPART_TAB_ENT_S new_part_ent, next_entry;
    FPART_TABLE_S *part_table;
    INT8   *ptr, buf[520];
    UINT16 up2_cyl, signature;
    UINT32 sec_p_cyl = 0;
    LCK_FS_ENTER()
    ext_part_ent.fpart_start = 0;
    ext_part_ent.fpart_ent.fpart_size = 0;
    ext_part_ent.fpart_ent.fpart_r_sec = 0;
    ext_part_ent.fpart_end = 0;

    /* Get the device table entry */
    ret_stat = fs_dev_devname_to_fdev(dev_name, &fdev);

    if (ret_stat == NU_SUCCESS)
    {
        /* Check if the device is mounted. */
        ret_stat = fpart_mount_check(fdev);
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Get the device specific information */
        ret_stat = fs_dev_ioctl_proc(fdev->fdev_dh, FDEV_GET_DISK_INFO, (VOID *)&disk_info, sizeof(FPART_DISK_INFO_S));
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Calculate the number of sectors per cylinder */
        sec_p_cyl = disk_info.fpart_heads * disk_info.fpart_secs;

        /* Prevent a goose egg error */
        if(sec_p_cyl == 0)
        {
            ret_stat = NUF_INTERNAL;        /* invalid disk info */
        }

    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Calculate partition size in sectors */
        if(size)
        {
            /* Calculate partition total sectors */
            size_sec = size * 0x800;        /* MiB to sectors */

            /* Cylinder snap the size. The total size must be a multiple of cylinder size */
            size_sec = ((size_sec + sec_p_cyl - 1) / sec_p_cyl)  * sec_p_cyl;
        }

        /* Maximum 28-bit */
        if (size_sec > 0xFFFFFFF)
        {
            /* size is too big! */
            ret_stat = NUF_BADPARM;
        }
    }

    /* Make a list of existing partitions */
    if (ret_stat == NU_SUCCESS)
    {
        ret_stat = NU_List_Partitions(fdev->fdev_name , &p_list);
        list_head = p_list;
    }

    /* Check existing partitions */
    if ((ret_stat == NU_SUCCESS) && (list_head != NU_NULL ))
    {
        /* Special case for removable media */
        if ((disk_info.fpart_flags & FPART_DI_RMVBL_MED) &&
            (list_head->fpart_next == NU_NULL) &&
            (list_head->fpart_ent.fpart_boot == 0xFF) )
        {
            /* Free the partition list we were using */
            NU_Free_Partition_List(list_head);
            list_head = p_list = NU_NULL;
        }

        while(p_list != NU_NULL)
        {
            /* Find number of primary partitions */
            if (p_list->fpart_type == FPART_PRIMARY)
            {
                num_pri_part++;
            }
            /* See if there is an extended partition */
            else if (p_list->fpart_type == FPART_EXTENDED)
            {
                num_ext_part++;
            }
            /* Find number of logical partitions */
            else if (p_list->fpart_type == FPART_LOGICAL)
            {
                num_log_part++;
            }

            p_list = p_list->fpart_next;
        }

        /* There can exist a total of four primary partitions,
           or three primaries and one (and only one) extended.
           Logical partitions must exist within an existing
           extended partition. */
        switch(part_type)
        {
            case FPART_PRIMARY:
            {
                if ((num_pri_part + num_ext_part + 1) <= 4)
                {
                    /* Remove the logical partitions from the list
                       because we don't need to worry about them if
                       we are creating a primary partition. */
                    p_list = list_head;
                    while(p_list != NU_NULL)
                    {
                        if(p_list->fpart_type == FPART_LOGICAL)
                        {
                            p_temp = p_list->fpart_next;
                            if(p_list->fpart_prev != NU_NULL)
                            {
                                p_list->fpart_prev->fpart_next = p_list->fpart_next;
                            }
                            if(p_list->fpart_next != NU_NULL)
                            {
                                p_list->fpart_next->fpart_prev = p_list->fpart_prev;
                            }
                            NU_Deallocate_Memory((VOID*)p_list);
                            p_list = p_temp;
                        }
                        else
                        {
                            p_list = p_list->fpart_next;
                        }
                    }
                }
                else
                {
                    ret_stat = NUF_PART_TABLE_FULL;
                }
                break;
            }
            case FPART_EXTENDED:
            {
                if (num_ext_part > 0)
                {
                    ret_stat = NUF_PART_EXT_EXISTS;
                }
                else if ((num_pri_part + 1) > 4)
                {
                    ret_stat = NUF_PART_TABLE_FULL;
                }

                break;
            }
            case FPART_LOGICAL:
            {
                if (num_ext_part == 1)
                {
                    /* Since we will only be working in the extended partition,
                       save the extended partition info and then remove it and
                       all primary partitions from the list. */
                    p_list = list_head;
                    list_head = NU_NULL;
                    while(p_list != NU_NULL)
                    {
                        if( (p_list->fpart_type == FPART_PRIMARY) ||
                            (p_list->fpart_type == FPART_EXTENDED) )
                        {
                            if(p_list->fpart_type == FPART_EXTENDED)
                            {
                                /* copy the extended partition information */
                                NUF_Copybuff((VOID*)&ext_part_ent, (VOID*)p_list, sizeof(FPART_LIST_S));
                            }

                            /* Remove from list */
                            p_temp = p_list->fpart_next;
                            if(p_list->fpart_prev != NU_NULL)
                            {
                                p_list->fpart_prev->fpart_next = p_list->fpart_next;
                            }
                            if(p_list->fpart_next != NU_NULL)
                            {
                                p_list->fpart_next->fpart_prev = p_list->fpart_prev;
                            }
                            NU_Deallocate_Memory((VOID*)p_list);
                            p_list = p_temp;
                        }
                        else
                        {
                            if(list_head == NU_NULL)
                            {
                                /* make list_head the first logical partition */
                                list_head = p_list;
                            }
                            p_list = p_list->fpart_next;
                        }
                    }
                }
                else
                {
                    ret_stat = NUF_PART_NO_EXT;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    /* Calculate partition offset. Also calculate size if size was not specified. */
    if(ret_stat == NU_SUCCESS)
    {
        p_list = list_head;

        if(part_type != FPART_LOGICAL)
        {
            /* From the beginning of the disk */
            base_sec = 0x0L;
        }
        else
        {
            /* From the beginning of the extended partition */
            base_sec = ext_part_ent.fpart_ent.fpart_r_sec;
        }

        if(offset == 0xFFFFFFFF)
        {
            /* offset was not specified */
            if(p_list != NU_NULL)
            {
                /* There are partitions on the disk. Use a first-fit
                   algorithm to find a spot for the new partition. */

                cur_offset_sec = p_list->fpart_start;
                cur_size_sec = p_list->fpart_end - p_list->fpart_start + 1;

                /* The first partition can be offset by one track
                   if it exists at the beginning of the disk. */
                if(cur_offset_sec == disk_info.fpart_secs)
                {
                    cur_offset_sec -= disk_info.fpart_secs;
                    cur_size_sec += disk_info.fpart_secs;
                }

                /* Check if there is free space before the first partition */
                if(cur_offset_sec != base_sec)
                {
                    if(size)
                    {
                        if(size_sec <= cur_offset_sec - base_sec)
                        {
                            /* It will fit here */
                            found = NU_TRUE;
                            offset_sec = base_sec;
                        }
                    }
                    else
                    {
                        /* It will fit here */
                        found = NU_TRUE;
                        size_sec = cur_offset_sec - base_sec;
                        offset_sec = base_sec;
                    }
                }

                /* Try to find free areas within partitions */
                while(!found && (p_list->fpart_next != NU_NULL))
                {
                    next_offset_sec = p_list->fpart_next->fpart_start;

                    /* Check if there is free space between the current partition and the next partition */
                    if(cur_offset_sec != next_offset_sec - cur_size_sec)
                    {
                        /* Was a size specified? */
                        if(size)
                        {
                            /* See if there is room for the partition requested */
                            if(size_sec <= (next_offset_sec - (cur_offset_sec + cur_size_sec)))
                            {
                                /* It will fit here */
                                offset_sec = cur_offset_sec + cur_size_sec;
                                /* Set exit condition */
                                found = NU_TRUE;
                            }
                        }
                        else
                        {
                            /* Make the partition fill the free area found */
                            size_sec = next_offset_sec - (cur_offset_sec + cur_size_sec);
                            offset_sec = cur_offset_sec + cur_size_sec;

                            /* Set exit condition */
                            found = NU_TRUE;
                        }
                    }

                    /* traverse to the next partition */
                    p_list = p_list->fpart_next;

                    cur_offset_sec = p_list->fpart_start;
                    cur_size_sec = p_list->fpart_end - p_list->fpart_start + 1;
                }

                if(found != NU_TRUE)
                {
                    /* There was no fragmented space large enough.
                       See if there is enough freespace at the end. */
                    switch(part_type)
                    {
                        case FPART_PRIMARY:
                        case FPART_EXTENDED:
                        {
                            if((((disk_info.fpart_tot_sec - 1) - p_list->fpart_end) != 0) &&
                               (size_sec <= ((disk_info.fpart_tot_sec - 1) - p_list->fpart_end)))
                            {
                                /* Set the offset to the end of the last partition plus one */
                                offset_sec = p_list->fpart_end + 1;
                                if(size == 0)
                                {
                                    /* Set the size to the entire space at the end of the disk. */
                                    size_sec = disk_info.fpart_tot_sec - offset_sec;
                                }
                            }
                            else
                            {
                                /* The requested size will not fit at the end of the disk. */
                                ret_stat = NUF_BADPARM;
                            }
                            break;
                        }

                        case FPART_LOGICAL:
                        {
                            if(size_sec <= (ext_part_ent.fpart_end - p_list->fpart_end))
                            {
                                /* Set the offset to the end of the last partition plus one */
                                offset_sec = p_list->fpart_end + 1;
                                if(size == 0)
                                {
                                    /* Set the size to the entire space at the end of the extended partition. */
                                    size_sec = ext_part_ent.fpart_end - p_list->fpart_end;
                                }
                            }
                            else
                            {
                                /* The requested size will not fit at the end of the extended partition. */
                                ret_stat = NUF_BADPARM;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
            else
            {
                /* There are no partitions on the disk. */

                switch(part_type)
                {
                    case FPART_PRIMARY:
                    case FPART_EXTENDED:
                    {
                        offset_sec = base_sec; /* Beginning of disk */
                        if(part_type == FPART_EXTENDED)
                        {
                            /* If the extended partition resides at 0,
                               then the address to the logical partition
                               table is offset by one cylinder */
                            offset_sec += sec_p_cyl;
                        }

                        /* Is a size specified? */
                        if(size)
                        {
                            /* See if there is room for the partition requested */
                            if(size_sec > disk_info.fpart_tot_sec)
                            {
                                /* It will not fit here */
                                ret_stat = NUF_BADPARM;
                            }
                        }
                        else
                        {
                            /* Make the partition fill the disk */
                            size_sec = disk_info.fpart_tot_sec - offset_sec;
                        }
                        break;
                    }
                    case FPART_LOGICAL:
                    {
                        /* There are no logical partitions in the extended partition */

                        /* The offset will be the extended start cylinder */
                        offset_sec = ext_part_ent.fpart_start;

                        /* Is a size specified? */
                        if(size)
                        {
                            /* See if there is room for the partition requested */
                            if(size_sec > ext_part_ent.fpart_ent.fpart_size)
                            {
                                /* It will not fit here */
                                ret_stat = NUF_BADPARM;
                            }
                        }
                        else
                        {
                            /* Make the logical partition fill the extended partition */
                            size_sec = ext_part_ent.fpart_ent.fpart_size;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
        else
        {
            /* offset was specified */

            /* Calculate partition offset in sectors */
            offset_sec = offset * 0x800;         /* MiB to sectors */

            /* Cylinder snap the offset. The offset must lie on a cylinder boundary. */
            offset_sec = ((offset_sec + sec_p_cyl - 1) / sec_p_cyl) * sec_p_cyl;

            /* Check that logical will exist within the extended partition */
            if( (part_type == FPART_LOGICAL) &&
                ((offset_sec < ext_part_ent.fpart_start) ||
                 (offset_sec >= ext_part_ent.fpart_end )) )
            {
                /* The specified offset does not exist inside the extended partition. */
                ret_stat = NUF_BADPARM;
            }

            /* Check that the offset will not cause the new partition to start
               in an existing partition. */
            p_list = list_head;
            while(p_list != NU_NULL)
            {
                if((offset_sec >= p_list->fpart_start) &&  /* Starts within */
                   (offset_sec <= p_list->fpart_end))
                {
                    /* The specified offset will overlap an existing partition. */
                    ret_stat = NUF_BADPARM;
                    break;
                }

                if(offset_sec > p_list->fpart_end)
                {
                    if(p_list->fpart_next != NU_NULL)
                    {
                        if(offset_sec < p_list->fpart_next->fpart_start)
                        {
                            if(size == 0)
                            {
                                size_sec = p_list->fpart_next->fpart_start - p_list->fpart_end;
                            }
                            /* quit looping */
                            break;
                        }
                    }
                    else
                    {
                        if(part_type != FPART_LOGICAL)
                        {
                            /* check end of disk */
                            if(offset_sec < (disk_info.fpart_tot_sec - 1))
                            {
                                if(size == 0)
                                {
                                    size_sec = (disk_info.fpart_tot_sec - 1) - p_list->fpart_end;
                                }
                                /* quit looping */
                                break;
                            }
                        }
                        else
                        {
                            /* check end of extended partition */
                            if(offset_sec < ext_part_ent.fpart_end)
                            {
                                if(size == 0)
                                {
                                    size_sec = ext_part_ent.fpart_end - p_list->fpart_end;
                                }
                                /* quit looping */
                                break;
                            }
                        }
                    }
                }

                p_list = p_list->fpart_next;
            }

            /* Check that the new partition will not end in an existing
               partition or contain an existing partition. */
            p_list = list_head;
            while(p_list != NU_NULL)
            {
                if((((offset_sec + size_sec) >= p_list->fpart_start) &&  /* Ends within */
                    ((offset_sec + size_sec) <= p_list->fpart_end)) ||
                   ((p_list->fpart_start >= offset_sec) &&  /* Contains */
                    (p_list->fpart_end <= (offset_sec + size_sec))) )
                {
                    /* The specified offset will overlap or contain an existing partition. */
                    ret_stat = NUF_BADPARM;
                    break;
                }
                p_list = p_list->fpart_next;
            }
        }
    }

    /* At this point there should be a validated and cylinder snapped
       offset_sec and size_sec. Now proceed to modify the partition
       table(s) on the disk. */

    if (ret_stat == NU_SUCCESS)
    {
        /* Build the partition table entry */
        new_part_ent.fpart_boot = 0x00;

        /* Upper two bits of cylinder are in sector field */
        up2_cyl = (UINT16) ((offset_sec / sec_p_cyl) >> 8);
        new_part_ent.fpart_s_sec = (UINT8) (0x01 | (up2_cyl << 6));
        new_part_ent.fpart_s_cyl = (UINT8) (((offset_sec / sec_p_cyl) << 2) >> 2);

        /* Upper two bits of cylinder are in sector field */
        new_part_ent.fpart_e_head = (UINT8)disk_info.fpart_heads - 1;

        if(((part_id == 0xC) ||
            (part_id == 0xE) ||
            (part_id == 0xF)) &&
           (offset_sec + size_sec <= 0xFC0000) )
        {
            /* The relative start and size fields should be used.
               Dummy information is placed in the end CHS fields. */
            new_part_ent.fpart_e_sec = 0xFF;
            new_part_ent.fpart_e_cyl = 0xFF;
        }
        else
        {
            up2_cyl = (UINT16) (((offset_sec + size_sec - 1) / sec_p_cyl) >> 8);
            new_part_ent.fpart_e_sec = (UINT8) (disk_info.fpart_secs | (up2_cyl << 6));
            new_part_ent.fpart_e_cyl = (UINT8) ((((offset_sec + size_sec - 1) / sec_p_cyl) << 2) >> 2);
        }

        new_part_ent.fpart_id = part_id;

        /* If the new logical entry is going to be the only table entry
           or if the new entry is going to be inserted to the first entry. */
        if ((list_head == NU_NULL) || (offset_sec < list_head->fpart_start))
        {
            if(part_type == FPART_EXTENDED)
            {
                new_part_ent.fpart_s_head = 0;
                new_part_ent.fpart_r_sec = offset_sec;
                new_part_ent.fpart_size = size_sec;
            }
            else
            {
                new_part_ent.fpart_s_head = 1;
                new_part_ent.fpart_r_sec = disk_info.fpart_secs;
                new_part_ent.fpart_size = size_sec - disk_info.fpart_secs;
            }
        }
        else
        {
            if(part_type != FPART_LOGICAL)
            {
                /* Primary or extended */
                new_part_ent.fpart_s_head = 0;
                new_part_ent.fpart_r_sec = offset_sec;
                new_part_ent.fpart_size = size_sec;
            }
            else
            {
                new_part_ent.fpart_s_head = 1;
                new_part_ent.fpart_r_sec = disk_info.fpart_secs;
                new_part_ent.fpart_size = size_sec - disk_info.fpart_secs;
            }
        }
    }

    if(ret_stat == NU_SUCCESS)
    {
        /* the new list entry should be inserted before p_list */
        p_temp = (PFPART_LIST_S)NUF_Alloc(sizeof(FPART_LIST_S));
        if(p_temp == NU_NULL)
        {
            ret_stat = NUF_NO_MEMORY;       /* not enough memory */
        }
    }

    if(ret_stat == NU_SUCCESS)
    {
        NUF_Memfill((VOID*)p_temp, sizeof(FPART_LIST_S), 0);

        p_temp->fpart_type = (enum FPART_TYPE)part_type;
        p_temp->fpart_size = (size_sec / 0x800) + /* sectors to MiB */
                       (((size_sec * 0x200) % 0x100000) / 0x80000); /* round up */
        p_temp->fpart_offset = (offset_sec / 0x800) + /* sectors to MiB */
                         (((offset_sec * 0x200) % 0x100000) / 0x80000); /* round up */
        p_temp->fpart_offset_units = 0x14; /* MiB */
        p_temp->fpart_ent = new_part_ent;
        p_temp->fpart_start = offset_sec;
        p_temp->fpart_end = p_temp->fpart_start + size_sec - 1;


        /* Insert the new partition into the in-memory list */
        if(list_head != NU_NULL)
        {
            p_list = list_head;

            preceed_flag = NU_FALSE;

            /* Traverse to the entry that should precede the new one or the last entry */
            while(NU_TRUE)
            {
                if (p_temp->fpart_start < p_list->fpart_start)
                {
                    /* the new partition starts before the current */
                    preceed_flag = NU_TRUE;
                    break;
                }
                else if(p_list->fpart_next == NU_NULL)
                {
                    /* last partition */
                    break;
                }

                p_list = p_list->fpart_next;
            }

            if(preceed_flag != NU_TRUE)
            {
                p_temp->fpart_next = p_list->fpart_next;
                p_temp->fpart_prev = p_list;
            }
            else
            {
                p_temp->fpart_prev = p_list->fpart_prev;
                p_temp->fpart_next = p_list;
            }

            if(p_temp->fpart_prev != NU_NULL)
            {
                p_temp->fpart_prev->fpart_next = p_temp;
            }

            if(p_temp->fpart_next != NU_NULL)
            {
                p_temp->fpart_next->fpart_prev = p_temp;
            }

            if (p_temp->fpart_prev == NU_NULL)
            {
                /* set the list head if the new entry became the first */
                list_head = p_temp;
            }
        }
        else
        {
            list_head = p_temp;
        }

        /* Insert the new entry into the existing table */
        ptr = buf;
        pad = 4 - ((int)ptr % 4);
        ptr += (pad+2);

        NUF_Memfill(ptr, 0x200, 0x00);

        part_table = (FPART_TABLE_S *)(ptr + 0x1BE);

        /* Signature word */
        signature = 0xAA55;
        SWAP16(&part_table->fpart_signature, &signature);

        p_list = list_head;

        if(part_type == FPART_LOGICAL)
        {
            num_parts = num_log_part + 1;   /* add one to the number of partitions */

            /* The first logical is written at the beginning of the extended partition */
            partition_address = ext_part_ent.fpart_ent.fpart_r_sec;

            for(i = 0; i < num_parts; i++)
            {
                /* Build the table */
                if(p_list->fpart_next == NU_NULL)
                {
                    /* last logical */
                    NUF_Copybuff((VOID*)&part_table->fpart_ents[0], (VOID*)&p_list->fpart_ent, sizeof(FPART_TAB_ENT_S));
                    NUF_Memfill((VOID*)&part_table->fpart_ents[1], sizeof(FPART_TAB_ENT_S), 0x0);
                }
                else
                {
                    NUF_Copybuff((VOID*)&part_table->fpart_ents[0], (VOID*)&p_list->fpart_ent, sizeof(FPART_TAB_ENT_S));
                    NUF_Copybuff((VOID*)&next_entry, (VOID*)&p_list->fpart_next->fpart_ent, sizeof(FPART_TAB_ENT_S));

                    next_entry.fpart_s_head = 0;

                    /* Adjust the size field */
                    next_entry.fpart_size += next_entry.fpart_r_sec;

                    if(p_list->fpart_next->fpart_end <= 0xFC0000)
                    {
                        next_entry.fpart_id = 0x05;
                    }
                    else
                    {
                        next_entry.fpart_id = 0x0F;
                    }

                    /* The relative sector of the second entry is the offset in sectors
                       from the beginning of the the extended partition. */
                    next_entry.fpart_r_sec = p_list->fpart_next->fpart_start - ext_part_ent.fpart_ent.fpart_r_sec;
                    NUF_Copybuff((VOID*)&part_table->fpart_ents[1], (VOID*)&next_entry, sizeof(FPART_TAB_ENT_S));
                }

                /* Write the new table */
                ret_stat = fs_dev_io_proc(fdev->fdev_dh, partition_address, ptr , 1 , NU_FALSE);
                if(ret_stat != NU_SUCCESS)
                {
                    /* error writing to disk */
                    break;
                }

                if(p_list->fpart_next != NU_NULL)
                {
                    /* Increment partition_address to point to the beginning of the next */
                    partition_address = p_list->fpart_next->fpart_start;
                }
                
                p_list = p_list->fpart_next;
            }
        }
        else
        {
            /* The main partition table will be written at sector 0 */
            partition_address = 0x0L;

            /* add one to the number of partitions */
            num_parts = num_pri_part + num_ext_part + 1;

            /* Build the partition table */
            for(i = 0; i < num_parts; i++)
            {
                NUF_Copybuff((VOID*)&part_table->fpart_ents[i], (VOID*)&p_list->fpart_ent, sizeof(FPART_TAB_ENT_S));
                p_list = p_list->fpart_next;
            }

            /* Write the new table */
            ret_stat = fs_dev_io_proc(fdev->fdev_dh, partition_address, ptr , 1 , NU_FALSE);
        }
    }

    /* Now register the new partition with the upper layers */

    if (ret_stat == NU_SUCCESS)
    {
        ret_stat = finit_remove_logical_devices(fdev->fdev_name);
        if (ret_stat == NU_SUCCESS)
        {
            ret_stat = finit_volume_enumeration(fdev->fdev_name);
        }
    }

    /* Free the partition list we were using */
    NU_Free_Partition_List(list_head);

    LCK_FS_EXIT()

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*   NU_Delete_Partition
*
* DESCRIPTION
*
*   Given a valid device name, match the associated partition and remove
*   it from the partition table. Extended partitions must be empty (i.e.
*   no logical partitions exist).
*
* INPUTS
*
*   *dev_name                   Unique string describing device.
*
* OUTPUTS
*
*   NUF_INVALID_DEVNAME         Matching device name was not found.
*   NUF_INTERNAL                Nucleus FILE internal error.
*   NUF_PART_LOG_EXISTS         Ext part cannot be removed.
*   NU_SUCCESS                  Partition removed.
*
*************************************************************************/
STATUS NU_Delete_Partition(CHAR *log_dev_name)
{
    STATUS ret_stat, rem_first_log = NU_FALSE;
    CHAR phys_dev_name[FPART_MAX_PHYS_NAME];
    FDEV_S *log_fdev, *phys_fdev;
    FPART_DISK_INFO_S disk_info;
    PFPART_LIST_S p_list, list_head = NU_NULL, p_temp, p_to_remove = NU_NULL;
    FPART_LIST_S ext_part_ent;
    UINT16 signature;
    FPART_TAB_ENT_S next_entry;
    FPART_TABLE_S *part_table;
    INT8   *ptr, buf[520];
    INT pad, num_parts = 0, i;
    UINT32 partition_address = 0L;
    LCK_FS_ENTER()
    ext_part_ent.fpart_start = 0;
    ext_part_ent.fpart_ent.fpart_r_sec = 0;

    /* Get the device table entry */
    ret_stat = fs_dev_devname_to_fdev(log_dev_name, &log_fdev);

    if (ret_stat == NU_SUCCESS)
    {
        /* Parse the physical name from the logical name */
        NUF_Copybuff(phys_dev_name, log_dev_name, FPART_MAX_PHYS_NAME);

        phys_dev_name[FPART_MAX_PHYS_NAME - 1] = NU_NULL;

        /* Get the device table entry */
        ret_stat = fs_dev_devname_to_fdev(phys_dev_name, &phys_fdev);
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Check if the device is mounted. */
        ret_stat = fpart_mount_check(phys_fdev);
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Get the device specific information */
        ret_stat = fs_dev_ioctl_proc(phys_fdev->fdev_dh, FDEV_GET_DISK_INFO, (VOID *)&disk_info, sizeof(FPART_DISK_INFO_S));
    }

    if(ret_stat == NU_SUCCESS)
    {
        /* Get a list of all partitions on the physical disk */
        ret_stat = NU_List_Partitions(phys_fdev->fdev_name , &list_head);
    }

    if(ret_stat == NU_SUCCESS)
    {
        /* Set the ret_status to an error. */
        ret_stat = NUF_INVALID_DEVNAME;     /* Could not find partition requested */

        /* Find the specific partition by matching the device name */
        for(p_list = list_head; p_list != NU_NULL; p_list = p_list->fpart_next)
        {
            if(NUF_Strncmp(p_list->fpart_name, log_dev_name, FPART_MAX_LOG_NAME) == NU_SUCCESS)
            {
                /* Found the matching partition */
                ret_stat = NU_SUCCESS;

                /* Make a copy of the list pointer */
                p_to_remove = p_list;
                break;
            }
        }
    }

    if (ret_stat == NU_SUCCESS)
    {
        switch(p_to_remove->fpart_type)
        {
            case FPART_PRIMARY:
            {
                /* Remove the logical partitions from the list
                   because we don't need to worry about them if
                   we are deleting a primary partition. */
                p_list = list_head;
                while(p_list != NU_NULL)
                {
                    if(p_list->fpart_type == FPART_LOGICAL)
                    {
                        p_temp = p_list->fpart_next;
                        if(p_list->fpart_prev != NU_NULL)
                        {
                            p_list->fpart_prev->fpart_next = p_list->fpart_next;
                        }
                        if(p_list->fpart_next != NU_NULL)
                        {
                            p_list->fpart_next->fpart_prev = p_list->fpart_prev;
                        }
                        NU_Deallocate_Memory((VOID*)p_list);
                        p_list = p_temp;
                    }
                    else
                    {
                        num_parts++;
                        p_list = p_list->fpart_next;
                    }
                }
                break;
            }
            case FPART_EXTENDED:
            {
                p_list = list_head;
                while(p_list != NU_NULL)
                {
                    if(p_list->fpart_type == FPART_LOGICAL)
                    {
                        /* Extended partitions must not contain any logical partitions. */
                        ret_stat = NUF_PART_LOG_EXISTS;
                        break;
                    }
                    num_parts++;
                    p_list = p_list->fpart_next;
                }
                break;
            }
            case FPART_LOGICAL:
            {
                /* Since we will only be working in the extended partition,
                   save the extended partition info and then remove it and
                   all primary partitions from the list. */
                p_list = list_head;
                list_head = NU_NULL;
                while(p_list != NU_NULL)
                {
                    if( (p_list->fpart_type == FPART_PRIMARY) ||
                        (p_list->fpart_type == FPART_EXTENDED) )
                    {
                        if(p_list->fpart_type == FPART_EXTENDED)
                        {
                            /* copy the extended partition information */
                            NUF_Copybuff((VOID*)&ext_part_ent, (VOID*)p_list, sizeof(FPART_LIST_S));
                        }

                        /* Remove from list */
                        p_temp = p_list->fpart_next;
                        if(p_list->fpart_prev != NU_NULL)
                        {
                            p_list->fpart_prev->fpart_next = p_list->fpart_next;
                        }
                        if(p_list->fpart_next != NU_NULL)
                        {
                            p_list->fpart_next->fpart_prev = p_list->fpart_prev;
                        }
                        NU_Deallocate_Memory((VOID*)p_list);
                        p_list = p_temp;
                    }
                    else
                    {
                        if(list_head == NU_NULL)
                        {
                            /* make list_head the first logical partition */
                            list_head = p_list;
                        }
                        num_parts++;
                        p_list = p_list->fpart_next;
                    }
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    if (ret_stat == NU_SUCCESS)
    {
        if(p_to_remove->fpart_prev)
        {
            p_to_remove->fpart_prev->fpart_next = p_to_remove->fpart_next;
        }
        else
        {
            /* the first partition in the list is being removed */
            if((p_to_remove->fpart_type == FPART_LOGICAL) &&
               (p_to_remove->fpart_next != NU_NULL))
            {
                rem_first_log = NU_TRUE;
            }
            else
            {
                /* update the list_head */
                list_head = p_to_remove->fpart_next;
            }
        }

        if(!rem_first_log)
        {
            /* remove from partition list */
            if(p_to_remove->fpart_next)
            {
                p_to_remove->fpart_next->fpart_prev = p_to_remove->fpart_prev;
            }

            NU_Deallocate_Memory((VOID*)p_to_remove);
        }
    }

    /* Write out the new partition table(s) */
    if (ret_stat == NU_SUCCESS)
    {
        /* Build the new partition table */
        ptr = buf;
        pad = 4 - ((int)ptr % 4);
        ptr += (pad+2);

        NUF_Memfill(ptr, 0x200, 0x00);

        part_table = (FPART_TABLE_S *)(ptr + 0x1BE);

        /* Signature word */
        signature = 0xAA55;
        SWAP16(&part_table->fpart_signature, &signature);

        p_list = list_head;

        /* subtract one from the number of partitions */
        num_parts--;

        if(num_parts == 0)
        {
            /* We removed the only partition. Write an empty table to the disk. */
            if(p_to_remove->fpart_type != FPART_LOGICAL)
            {
                partition_address = 0L;
            }
            else
            {
                partition_address = ext_part_ent.fpart_start;
            }
            ret_stat = fs_dev_io_proc(phys_fdev->fdev_dh, partition_address, ptr , 1 , NU_FALSE);
        }
        else if(p_to_remove->fpart_type == FPART_LOGICAL)
        {
            for(i = 0; i < num_parts; i++)
            {
                /* Build the table */
                if(rem_first_log)
                {
                    partition_address = ext_part_ent.fpart_start;

                    /* removing first logical */
                    NUF_Copybuff((VOID*)&part_table->fpart_ents[0], (VOID*)&p_list->fpart_next->fpart_ent, sizeof(FPART_TAB_ENT_S));
                    NUF_Memfill((VOID*)&part_table->fpart_ents[1], sizeof(FPART_TAB_ENT_S), 0x0);

                    part_table->fpart_ents[0].fpart_s_head = 0;

                    /* Adjust the size field */
                    part_table->fpart_ents[0].fpart_size += part_table->fpart_ents[0].fpart_r_sec;

                    /* The relative sector is the offset in sectors
                       from the beginning of the the extended partition. */
                    part_table->fpart_ents[0].fpart_r_sec = p_list->fpart_next->fpart_start - ext_part_ent.fpart_start;

                    if((part_table->fpart_ents[0].fpart_size +
                        part_table->fpart_ents[0].fpart_r_sec) <= 0xFC0000)
                    {
                        part_table->fpart_ents[0].fpart_id = 0x05;
                    }
                    else
                    {
                        part_table->fpart_ents[0].fpart_id = 0x0F;
                    }

                    /* increment numparts by one since we are writing the placeholder entry */
                    num_parts++;
                    rem_first_log = NU_FALSE;
                }
                else if((p_list != NU_NULL)&&(p_list->fpart_next == NU_NULL))
                {
                    partition_address = p_list->fpart_start;

                    /* last logical */
                    NUF_Copybuff((VOID*)&part_table->fpart_ents[0], (VOID*)&p_list->fpart_ent, sizeof(FPART_TAB_ENT_S));
                    NUF_Memfill((VOID*)&part_table->fpart_ents[1], sizeof(FPART_TAB_ENT_S), 0x0);
                }
                else if(p_list != NU_NULL)
                {
                    partition_address = p_list->fpart_start;

                    NUF_Copybuff((VOID*)&part_table->fpart_ents[0], (VOID*)&p_list->fpart_ent, sizeof(FPART_TAB_ENT_S));
                    NUF_Copybuff((VOID*)&next_entry, (VOID*)&p_list->fpart_next->fpart_ent, sizeof(FPART_TAB_ENT_S));

                    next_entry.fpart_s_head = 0;

                    /* Adjust the size field */
                    next_entry.fpart_size += next_entry.fpart_r_sec;

                    if(p_list->fpart_next->fpart_end <= 0xFC0000)
                    {
                        next_entry.fpart_id = 0x05;
                    }
                    else
                    {
                        next_entry.fpart_id = 0x0F;
                    }

                    /* The relative sector of the second entry is the offset in sectors
                       from the beginning of the the extended partition. */
                    next_entry.fpart_r_sec = p_list->fpart_next->fpart_start - ext_part_ent.fpart_ent.fpart_r_sec;
                    NUF_Copybuff((VOID*)&part_table->fpart_ents[1], (VOID*)&next_entry, sizeof(FPART_TAB_ENT_S));
                }

                /* Write the new table */
                ret_stat = fs_dev_io_proc(phys_fdev->fdev_dh, partition_address, ptr , 1 , NU_FALSE);
                if(ret_stat != NU_SUCCESS)
                {
                    /* error writing to disk */
                    break;
                }

                if(p_list != NU_NULL)
                {
                    p_list = p_list->fpart_next;
                }
            }
        }
        else
        {
            /* The main partition table will be written at sector 0 */
            partition_address = 0x0L;

            /* Build the partition table */
            for(i = 0; i < num_parts; i++)
            {
                if (p_list != NU_NULL)
                {
                    NUF_Copybuff((VOID*)&part_table->fpart_ents[i], (VOID*)&p_list->fpart_ent, sizeof(FPART_TAB_ENT_S));
                    p_list = p_list->fpart_next;
                }
            }

            /* Write the new table */
            ret_stat = fs_dev_io_proc(phys_fdev->fdev_dh, partition_address, ptr , 1 , NU_FALSE);
        }
    }

    /* Now register the new partition with the upper layers */
    if (ret_stat == NU_SUCCESS)
    {
        ret_stat = finit_remove_logical_devices(phys_fdev->fdev_name);
        if (ret_stat == NU_SUCCESS)
        {
            ret_stat = finit_volume_enumeration(phys_fdev->fdev_name);
        }
    }

    /* Free the partition list we were using */
    NU_Free_Partition_List(list_head);

    LCK_FS_EXIT()

    return(ret_stat);
}


STATUS fpart_mount_check(FDEV_S *phys_fdev)
{
    STATUS ret_stat;
    MNT_LIST_S  *mount_list, *mnt_index;

    ret_stat = NU_List_Mount(&mount_list);
    if (ret_stat == NU_SUCCESS)
    {
        mnt_index = mount_list;
        /* Loop through the mount table checking for entries
         * that have the same physical device name
         */
        while (mnt_index)
        {
            if (NUF_Strncmp(phys_fdev->fdev_name, mnt_index->dev_name, FPART_MAX_PHYS_NAME - 1) == NU_SUCCESS)
            {
                /* Fail if there is a mte in use whose name matches the physical dev name*/
                ret_stat = NUF_IN_USE;
                break;
            }
            else
                mnt_index = mnt_index->next;
        }
    }
    NU_Free_List((VOID **)&mount_list);
    return(ret_stat);
}



VOID fpart_cyl_read(FPART_TAB_ENT_S entry, UINT16 *start_cyl, UINT16 *end_cyl)
{
    UINT16 up2_cyl;

    up2_cyl = (UINT16) (entry.fpart_s_sec >> 6);
    *start_cyl =  (UINT16)((up2_cyl << 8) | entry.fpart_s_cyl);

    up2_cyl = (UINT16) (entry.fpart_e_sec >> 6);
    *end_cyl =  (UINT16)((up2_cyl << 8) | entry.fpart_e_cyl);
}

