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
*       pc_part.c
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*       FAT specific wrapper for creating partitions.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Create_FAT_Partition
*       fat_set_type_id
*
*************************************************************************/

#include        "storage/fat_defs.h"

STATUS fat_set_type_id(UINT8 *part_id, INT8 lba_support,
                           UINT32 part_totalsec, UINT8 fat_type,
                           UINT16 part_type);

/************************************************************************
* FUNCTION
*
*   NU_Create_FAT_Partition
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
*   fat_type                    The FAT file system type. FAT12 (0),
*                                FAT16 (1), or FAT32 (2).The partition
*                                identifier written into the partition
*                                table is determined based on fat_type
*                                and partition size.
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
STATUS NU_Create_FAT_Partition(CHAR *dev_name, UINT16 part_type, UINT32 size, UINT32 offset, UINT8 fat_type)
{
    STATUS ret_stat;
    UINT8  part_type_id;
    INT8   lba_mode = NU_TRUE;

    /* LBA mode is used by default. */
    ret_stat = fat_set_type_id(&part_type_id, lba_mode, size, fat_type, part_type);

    if(ret_stat == NU_SUCCESS)
    {
        ret_stat = NU_Create_Partition(dev_name, part_type, size, offset, part_type_id);
    }

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*   fat_set_type_id
*
* DESCRIPTION
*
*   Return a valid FAT partition identifier for the given parameters.
*
* INPUTS
*
*   *part_id                    Return value of partition identifier.
*   lba_support                 logical block addressing (LBA) = NU_TRUE
*                                or cylinder-head-sector (CHS) = NU_FALSE
*   size                        Unspecified (0), or a value in
*                                MiB (2^20 bytes).
*   fat_type                    The FAT file system type. FAT12 (0),
*                                FAT16 (1), or FAT32 (2).The partition
*                                identifier written into the partition
*                                table is determined based on fat_type
*                                and partition size.
*   part_type                   Primary (0), extended (1), or logical (2)
*
* OUTPUTS
*
*   NUF_IDE_DISK_SIZE           Invalid partition size.
*   NUF_IDE_FAT_TYPE            Invalid FAT type.
*   NU_SUCCESS                  Valid partition id returned.
*
*************************************************************************/
STATUS fat_set_type_id(UINT8 *part_id, INT8 lba_support,
                           UINT32 size, UINT8 fat_type,
                           UINT16 part_type)
{
    STATUS  ret_stat = NU_SUCCESS;

    /* If size is unspecified, then use the minimums
       to calculate the partition type id.
       NOTE: This may result in incorrect size / type
             matching, but this will get corrected when
             the disk is formatted. */

    /* Set partition type */
    switch (fat_type)
    {
        /* FAT12 */
        case FSFAT_12:
        {
            if (size < 0x10)                /* < 16MiB */
            {
                *part_id = 0x01;
            }
            else
            {
                /* too large for type */
                ret_stat = NUF_IDE_DISK_SIZE;
            }
            break;
        }

        /* FAT16 */
        case FSFAT_16:
        {
            if ((size) && (size < 0x10))    /* < 16MiB */
            {
                /* too small for type */
                ret_stat = NUF_IDE_DISK_SIZE; 
            }
            else if (size <= 0x20)          /* <= 32MiB */
            {
                if (part_type == (UINT16)FPART_EXTENDED)
                {
                    /* Extended partition */
                    *part_id = 0x05;
                }
                else
                {
                    /* Primary or logical partition */
                    *part_id = 0x04;
                }
            }
            else if (size < 0x1000)         /* < 4GiB */
            {
                if (lba_support)
                {
                    if (part_type == (UINT16)FPART_EXTENDED)
                    {
                        /* Extended partition using LBA */
                        *part_id = 0x0F;
                    }
                    else
                    {
                        /* Primary or logical partition using LBA */
                        *part_id = 0x0E;
                    }
                }
                else
                {
                    if (part_type == (UINT16)FPART_EXTENDED)
                    {
                        /* Extended partition */
                        *part_id = 0x05;
                    }
                    else
                    {
                        /* Primary or logical partition */
                        *part_id = 0x06;
                    }
                }
            }
            else
            {
                /* too large for type */
                ret_stat = NUF_IDE_DISK_SIZE;
            }
            break;
        }

        /* FAT32 */
        case FSFAT_32:
        {
            if ((size) && (size < 0x21))    /* < 33MiB */
            {
                /* too small for type */
                ret_stat = NUF_IDE_DISK_SIZE;
            }
            else if (size < 0x200000)       /* < 2TiB */
            {
                if (lba_support)
                {
                    if (part_type == (UINT16)FPART_EXTENDED)
                    {
                        /* Extended partition using LBA */
                        *part_id = 0x0F;
                    }
                    else
                    {
                        /* Primary or logical partition using LBA */
                        *part_id = 0x0C;
                    }
                }
                else
                {
                    if (part_type == (UINT16)FPART_EXTENDED)
                    {
                        /* Extended partition using LBA
                           Supports at most 8.4 GB disks. See type 0F above. Using type 05
                           for extended partitions beyond 8 GB may lead to data corruption. */
                        *part_id = 0x05;
                    }
                    else
                    {
                        /* Primary or logical partition NOT using LBA
                           Partitions up to 2047GB */
                        *part_id = 0x0B;
                    }
                }
            }
            else
            {
                /* too large for type */
                ret_stat = NUF_IDE_DISK_SIZE;
            }
            break;
        }

        default:
        {
            ret_stat = NUF_IDE_FAT_TYPE;    /* FAT type ERROR */
            break;
        }

    };

    return(ret_stat);
}

