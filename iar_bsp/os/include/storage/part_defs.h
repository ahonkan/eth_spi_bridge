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
*       part_defs.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Partition
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains defines and structure definitions for partition 
*       services.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       FPART_DISK_INFO_S           Physical device info structure.
*       FPART_TAB_ENT_S             Single partition entry structure.
*       FPART_TABLE_S               Partition table structure.
*       FPART_LIST_S                List of partitions for a device.
*       PFPART_LIST_S               Pointer to a FPART_LIST_S.
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#ifndef PART_DEFS_H
#define PART_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


#define FPART_MAX_PHYS_NAME   4
#define FPART_MAX_LOG_NAME    FILE_MAX_DEVICE_NAME

/* Enumerated type to describe partition types. */
enum FPART_TYPE
{
    FPART_PRIMARY,
    FPART_EXTENDED,
    FPART_LOGICAL
};

/* Physical disk info structure. */
typedef struct file_part_disk_info_struct
{
    CHAR   fpart_name[FILE_MAX_DEVICE_NAME]; /* Device name defined by driver */
    UINT32 fpart_tot_sec;                   /* Total number of physical sectors */
    UINT32 fpart_bytes_p_sec;               /* Number of bytes per sector */
    UINT32 fpart_cyls;                      /* Total number of cylinders. Valid range is 0 - 1024 */
    UINT32 fpart_heads;                     /* Number of heads per cylinder. Valid range is 0 - 255 */
    UINT32 fpart_secs;                      /* Number of sectors per track. Valid range is 1 - 63 */ 
    UINT32 fpart_flags;                     /* Bit fields are defined below. */
    VOID*  fpart_spec;                      /* Pointer for the driver to pass file system specific info */ 
} FPART_DISK_INFO_S;

/* Disk info flags */
#define FPART_DI_LBA_SUP   0x01             /* Bit one set to one indicates the device can use LBA */
#define FPART_DI_RMVBL_MED 0x02             /* Bit two set to one indicates a removable device */
#define FPART_DI_RAMDISK   0x04             /* Bit three set to one indicates a ram disk */
#define FPART_DI_SAFE      0x08             /* Bit four set to one indicates a SAFE device */
/* Partition descriptions structure. */
typedef struct file_part_table_entry_struct
{
    UINT8  fpart_boot;                      /* Boot signature */
    UINT8  fpart_s_head;                    /* Start head */
    UINT8  fpart_s_sec;                     /* Start sector (Bit0-5) Bit6 and 7 are cylinder number */
    UINT8  fpart_s_cyl;                     /* Start cylinder upper two bit of starting cylinder number are in StartSector field. */
    UINT8  fpart_id;                        /* Partition type identifier */
    UINT8  fpart_e_head;                    /* End head */
    UINT8  fpart_e_sec;                     /* End sector (Bit0-5) Bit6 and 7 are cylinder number*/
    UINT8  fpart_e_cyl;                     /* End cylinder upper two bit of ending cylinder number are in StartSector field. */
    UINT32 fpart_r_sec;                     /* Relativity sector */
    UINT32 fpart_size;                      /* Size of partition */

} FPART_TAB_ENT_S;


/* Partition table descriptions structure. */
typedef struct file_part_table_struct
{
    FPART_TAB_ENT_S fpart_ents[4];          /* Entry table */
    UINT16          fpart_signature;        /* should be 0xAA55 */

} FPART_TABLE_S;

/* List element for describing the partitions on a physical device. */
typedef struct file_part_list_struct
{
    CHAR   fpart_name[FPART_MAX_LOG_NAME];  /* device name */
    UINT32 fpart_offset;                    /* offset from physical 0 */
    UINT8  fpart_offset_units;              /* units of offset in powers of two 
                                               (offset * 2^offset_units) */
    UINT32 fpart_size;                      /* size in MiB */
    enum FPART_TYPE fpart_type;             /* partition type: primary, extended, or logical */
    FPART_TAB_ENT_S fpart_ent;              /* partition table entry read from disk */
    UINT32 fpart_start;                     /* physical starting sector */
    UINT32 fpart_end;                       /* physical ending sector */
    struct file_part_list_struct *fpart_next;        /* next in list */
    struct file_part_list_struct *fpart_prev;        /* previous in list */
} FPART_LIST_S, *PFPART_LIST_S;


#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* PART_DEFS_H */
