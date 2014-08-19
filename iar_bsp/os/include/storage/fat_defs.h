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
/************************************************************************
* FILE NAME
*
*       fat_defs.h
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*       This file contains Nucleus FILE constants common to both the
*       application and the actual Nucleus FILE components.  This file
*       also contains data structure definitions that hide internal
*       information from the application.
*
* DATA STRUCTURES
*
*       FATSWAP                             Fat blocks structure.
*       DDRIVE                              Block 0 image.
*       DOSINODE                            Dos Directory Entry.
*       BLKBUFF                             Block buffer.
*       LNAMINFO                            Long file name information.
*       LNAMENT                             Long file name Directory
*                                            Entry.
*       FINODE                              DOS entry.
*       DIRBLK                              Directory information.
*       DROBJ                               Object used to find a dirent
*                                            on a disk and its parent's.
**       PC_FILE                             Internal file representation.
*       FAT_DSTAT                               File search structure.
*       FMTPARMS                            Format parameter block.
*       FILE_SYSTEM_USER                    File system user structure.
*       _PC_BDEVSW                          Device driver dispatch table.
*       _NUF_FSTBL                          File system dispatch table.
*       PTABLE_ENTRY                        Partition table descriptions.
*       PTABLE                              Boot Record structure.
*
* FUNCTIONS
*
*       None.
*
*************************************************************************/

#ifndef __FAT_DEFS__
#define __FAT_DEFS__ 1

#ifdef          __cplusplus
    extern  "C" {                           /* C declarations in C++ */
#endif /* _cplusplus */

#include "nucleus.h"       
#include "storage/pcdisk.h"
#include "storage/dir_defs.h"
#include "storage/dev_extr.h"

#ifdef DEBUG
    #define PRINT_STRING    UART_Put_String
    void va_error_print(char* format, ...);
    #define DEBUG_PRINT     va_error_print
#else
    #define PRINT_STRING
#endif  /* ifdef DEBUG */

/* Use the x86 processor. */
#define X86                     0           /* 1 : Use  0 : Not use */

#define BLOCKEQ0                0L

/* ======== MULTI TASKING SUPPORT   =============== */

extern UINT16 gl_NFINODES;
extern UINT16 gl_NDROBJS;


/* Alternate short file name (SFN) generation. 
   Setting this to true will allow Nucleus FILE to generate a SFN using a hash
   function on its long file name (LFN) after SFN_ATTEMPTS have been made to create a SFN the 
   standard way. The standard way to generate a SFN for similar LFNs is to keep
   incrementing the numeric tail until a free entry is found. This can be very time 
   consuming, when there are alot of similar LFNs in a single directory. Setting 
   this to NU_TRUE allows Nucleus FILE to perform a hash on the LFN in an attempt 
   to generate a more unique SFN, thus increasing the chances that a free entry is 
   found on the first try. Setting this to NU_TRUE will greatly improve performance
   when there are a large number of similar LFNs in a single directory. */  

#ifndef ENABLE_SFN_GEN
    #define ENABLE_SFN_GEN         NU_TRUE
#endif

/* Attempts before generating short file names (SFNs). 
   If ENABLE_SFN_GEN is NU_TRUE then this is the number of SFN's that will be created
   using the standard SFN algorithm, before performing a hash on the LFN. */
#ifndef SFN_STANDARD_ATTEMPTS
    #define SFN_STANDARD_ATTEMPTS           4
#endif

/* NOTE: See the end of this file for more tunable constants which pertain
 * to the Nucleus environment, specifically allocating memory.
 */

/*=============== END TUNABLE CONSTANTS ==========================*/

/*=============== MULTI TASKING DATA STRUCTURES and MACROS =======*/

/* Setup the LOCK_METHOD define */
#if (CFG_NU_OS_STOR_FILE_VFS_NUM_USERS == 1)
    /* See the porting manual and pc_locks.c for a description of this constant */
    #define LOCK_METHOD             0
#else
    /* Fine grained multitask support */
    #define LOCK_METHOD             2
#endif  /* Num users > 1 */

/* These macros are API call prolog and epilogs. In multitasking
 * mode they lock/unlock the RTFS semaphore. In single tasking
 * mode they do nothing
 */
#define PC_FS_ENTER()           UINT16 process_flags = pc_fs_enter();
#define PC_FS_EXIT()            pc_fs_exit(process_flags);

/* Natural type for an event handle (see pc_locks.c)  */
typedef int WAIT_HANDLE_TYPE;

/* This is how many event handles we will allocate at startup. */
extern UINT16 gl_NHANDLES;

/* Lock object: We keep track of opencount and exclusive internally.
 * wait_handle is a handle to an event channel that is provided by
 * the executive or os environment. via pc_alloc_lock(). When we need
 * to trigger an event on the channel we call pc_wake_lock(wait_handle)
 * when we need to block we call pc_wait_lock(wait_handle)
 */
typedef struct lockobj
{
    WAIT_HANDLE_TYPE    wait_handle;
    INT16               opencount;
    UINT16              dh;
    INT                 exclusive;
} LOCKOBJ;

#if (LOCK_METHOD == 2)
    /* Special macros providing fine grained reentrancy */
    #define PC_DRIVE_ENTER(X, Y)    pc_drive_enter((X), (Y));
    #define PC_DRIVE_EXIT(X)        pc_drive_exit((X));
    #define PC_INODE_ENTER(X,Y)     pc_inode_enter((X), (Y));
    #define PC_INODE_EXIT(X)        pc_inode_exit((X));
    #define PC_FAT_ENTER(X)         pc_fat_enter((X));
    #define PC_FAT_EXIT(X)          pc_fat_exit((X));
    #define PC_DRIVE_IO_ENTER(X)    pc_drive_io_enter((X));
    #define PC_DRIVE_IO_EXIT(X)     pc_drive_io_exit((X));
#else
    /* LOCK_METHOD 1 or 0 */
    #define PC_DRIVE_ENTER(X, Y)
    #define PC_DRIVE_EXIT(X)
    #define PC_INODE_ENTER(X, Y)
    #define PC_INODE_EXIT(X)
    #define PC_FAT_ENTER(X)
    #define PC_FAT_EXIT(X)
    #define PC_DRIVE_IO_ENTER(X)
    #define PC_DRIVE_IO_EXIT(X)
#endif /*  (LOCK_METHOD == 2) */

/* Changing to '/' should give unix style path separators. */
#define BACKSLASH '\\'

#define PCDELETE (UINT8) 0xE5       /* MS-DOS file deleted char */

/* Structure used to track cached fat blocks */
typedef struct fatswap
{
    UINT32      n_to_swap;          /* Next to swap. For implementing round robin */
    UINT32      base_block;         /* base_block of FAT sector index in data_map */
    /*  These two counters track cache usage as we fill it. Eventually the
        FAT fills and we go into swapping mode at steady state */
    UINT16      n_blocks_used;      /* How many blocks in the cache have we used */
    INT         n_blocks_total;     /* How many blocks are available in the cache */

    UINT8       *pdirty;            /* BIT-map of blocks needing flushing */
    INT         block_0_is_valid;   /* If YES then data_map[0] contains the offset
                                       of the first block of the FAT */
    INT         data_map_size;      /* data_map table size */
    UINT16      *data_map;          /* Table that maps block numbers in the fat to
                                       block offsets in our data array. zero means
                                       the block is not mapped. Except.. data_map[0]
                                       contains block zero of the FAT which
                                       is at location 0 in the data array */
    UINT8 FAR   *data_array;        /* Block buffer area on Intel systems always FAR */
} FATSWAP;

/* Structure to contain block 0 image from the disk */
typedef struct ddrive
{
    UINT32      fs_type;            /* FAT_FILE_SYSTEM */
    UINT16      dh;                 /* Disk handle from VFS */
    INT16       opencount;          /* Drive open count */
    UINT32      bytespcluster;      /* Bytes per cluster */
    UINT32      byte_into_cl_mask;  /* And this with file pointer to get the
                                       byte offset into the cluster */
    UINT16      fasize;             /* Nibbles per fat entry. (2 or 4) */
    UINT32      rootblock;          /* First block of root dir */
    UINT32      firstclblock;       /* First block of cluster area */
    UINT32      maxfindex;          /* Last element in the fat */
    UINT16      secproot;           /* blocks in root dir */
    INT         fat_is_dirty;       /* FAT update flag */
    INT         use_fatbuf;         /* Use FAT buffer flag */
    FATSWAP     fat_swap_structure; /* Fat swap structure if swapping */
    UINT16      log2_secpalloc;     /* Log of sectors per cluster */
    UINT32      maxfsize_cluster;   /* Maximum file cluster size */
    UINT16      valid_fsinfo;       /* fsinfo valid  flag */
          /******** BPB  ***********/
    INT8        oemname[8];         /* 0x03 OEM name */
    UINT16      bytspsector;        /* 0x0b Must be 512 for this implementation */
    UINT8       secpalloc;          /* 0x0d Sectors per cluster */
    UINT16      fatblock;           /* 0x0e Reserved sectors before the FAT */
    UINT8       numfats;            /* 0x10 Number of FATS on the disk */
    UINT16      numroot;            /* 0x11 Maximum # of root dir entries */
    UINT32      numsecs;            /* 0x13 Total # sectors on the disk */
    UINT8       mediadesc;          /* 0x15 Media descriptor byte */
    UINT32      secpfat;            /* 0x16 Size of each fat */
    UINT16      secptrk;            /* 0x18 sectors per track */
    UINT16      numhead;            /* 0x1a number of heads */
    UINT32      numhide;            /* 0x1c # hidden sectors */
    UINT32      bignumsecs;         /* 0x20 Sectors per FAT32 */
 /* FAT16/12 */
    UINT8       phys_num;           /* 0x24 physical drive number */
    UINT8       xtbootsig;          /* 0x26 extend boot record signature */
    UINT32      volid;              /* 0x27 or 0x43 (FAT32) volume serial number */
    UINT8       vollabel[11];       /* 0x2b or 0x47 (FAT32) volume label */
 /* FAT32 */
    UINT32      bigsecpfat;         /* 0x24 Size of each FAT on the FAT32 */
    UINT16      fat_flag;           /* 0x28 FAT flag */
    UINT16      file_version;       /* 0x2a file system version */
    UINT32      rootdirstartcl;     /* 0x2c root cluster */
    UINT16      fsinfo;             /* 0x30 fsinfo block */
    UINT16      bpb_backup;         /* 0x32 BPB backup */
 /* FAT32 FSINFO */
    UINT32      free_clusters_count; /* If non-zero pc_free may use this value */
    UINT32      free_contig_pointer; /* Efficiently stored */
} DDRIVE;


/* FAT FS control block */
typedef struct fat_fs_cb
{
    UINT32      signature;
    DDRIVE      *ddrive;            /* NUF_Drive_Pointers */
    INT         fat_type;           /* NUF_Fat_Type */
    INT         drive_fat_size;     /* NUF_Drive_Fat_Size */
    LOCKOBJ     drive_lock;         /* Lock for DDRIVE struct */
    LOCKOBJ     drive_io_lock;      /* Lock for entering drivers */
    LOCKOBJ     fat_lock;           /* Lock for FAT table */
}FAT_CB;

/* Typedef for an entry in the FAT cache size table. This table is used
 * to set a specific size FAT cache for specific devices.
 */
typedef struct fat_cache_size_table_entry
{
    INT         size;               /* Size of the FAT cache, previously
                                       held in NUF_Drive_Fat_Size array */
    CHAR        devname[5];         /* Device name to match */
}FAT_CS_TE;

#define FAT_DEFAULT_CACHE_SIZE      CFG_NU_OS_STOR_FILE_FS_FAT_DEFAULT_CACHE_SIZE
#define FAT_CACHE_TABLE_END_SIZE    -1

/* Set the following define to the name of a global variable whose type
 * is an array of FAT_CS_TE. This array contains specific FAT cache table
 * sizes for specific device names. If not defined, the default cache size
 * is used.
 */
/*  #define FAT_CACHE_TABLE         my_fat_cache_table */

/* Dos Directory Entry Memory Image of Disk Entry */
#define INOPBLOCK       16          /* 16 of these fit in a block */
typedef struct dosinode
{
    UINT8       fname[8];           /* 00-07h   File name */
    UINT8       fext[3];            /* 08-0Ah   File extension */
    UINT8       fattribute;         /*    0Bh   File attributes */
    UINT8       reserve;            /*    0Ch   Reserved */
    UINT8       fcrcmsec;           /*    0Dh   File create centesimal millisecond */
    UINT16      fcrtime;            /* 0E-0Fh   File create time */
    UINT16      fcrdate;            /* 10-11h   File create date */
    UINT16      faccdate;           /* 12-13h   Access date */
    UINT16      fclusterhigh;       /* 14-15h   High cluster for data file */
    UINT16      fuptime;            /* 16-17h   File update time */
    UINT16      fupdate;            /* 18-19h   File update */
    UINT16      fclusterlow;        /* 1A-1Bh   Low cluster for data file */
    UINT32      fsize;              /* 1C-1Fh   File size */
} DOSINODE;

/* Block buffer */
typedef struct blkbuff
{
    struct      blkbuff *pnext;     /* Next block buffer pointer */
    UINT32      lru;                /* Last recently used stuff */
    INT16       locked;             /* Reserve */
    DDRIVE      *pdrive;            /* Drive block 0 image */
    UINT32      blockno;            /* I/O block number */
    INT16       use_count;          /* use count */
    INT         io_pending;         /* I/O pending flag */
    UINT8       data[512];          /* I/O data buffer */
} BLKBUFF;

/* Long file name information */
typedef struct lnaminfo
{
    BLKBUFF     *lnamblk1;          /* Long file name block buffer 1    */
    BLKBUFF     *lnamblk2;          /* Long file name block buffer 2    */
    BLKBUFF     *lnamblk3;          /* Long file name block buffer 3    */
    INT16       lnam_index;         /* Long file name start index       */
    INT16       lnament;            /* Number of long file name entries */
    UINT8       lnamchk;            /* Check value of short file name   */
} LNAMINFO;

/* Long file name Directory Entry Memory Image of Disk Entry */
typedef struct lnament
{
    UINT8       ent_num;            /*    00h   Long filename entry number  */
    UINT8       str1[10];           /* 01-0Ah   File name                   */
    UINT8       attrib;             /*    0Bh   File attributes, always 0Fh */
    UINT8       reserve;            /*    0Ch   Reserved                    */
    UINT8       snamchk;            /*    0Dh   Short file name check       */
    UINT8       str2[12];           /* 0E-19h   File name                   */
    UINT8       fatent[2];          /* 1A-1Bh   File entry number, always 00*/
    UINT8       str3[4];            /* 1C-1Fh   File name                   */
} LNAMENT;

/* Internal representation of DOS entry */
typedef struct finode
{
    UINT8       fname[MAX_SFN];         /* File name */
    UINT8       fext[MAX_EXT];          /* File extension */
    UINT8       fattribute;         /* File attributes */
    UINT8       reserve;            /* Reserved */
    UINT8       fcrcmsec;           /* File create centesimal millisecond */
    UINT16      fcrtime;            /* File create time */
    UINT16      fcrdate;            /* File create date */
    UINT16      faccdate;           /* Access date */
    UINT16      fuptime;            /* File update time */
    UINT16      fupdate;            /* File update */
    UINT32      fcluster;           /* Cluster for data file */
    UINT32      fsize;              /* File size */
    UINT32      alloced_size;       /* Size rounded up to the nearest cluster
                                        - only maintained for files */
    UINT8       file_delete;        /* Set to indicate that the file is deleted. */
    INT16   opencount;

/* If the finode is an open file the following flags control the sharing.
 * they are maintained by fat_open
 */
#ifdef OF_WRITE
    #undef OF_WRITE
#endif

#define OF_WRITE            0x01    /* File is open for write by someone */
#define OF_WRITEEXCLUSIVE   0x02    /* File is open for write by someone
                                       they wish exclusive write access */
#define OF_EXCLUSIVE        0x04    /* File is open with exclusive access not
                                       sharing write or read */
    INT16       openflags;          /* For Files. Track how files have it open */
    DDRIVE      *my_drive;          /* Block 0 image from this disk */
    UINT32      my_block;           /* Number of this file directory entry block */
    INT16       my_index;           /* Directory entry index */
    LOCKOBJ     lock_object;        /* for locking during critical sections */
    struct finode *pnext;           /* Next DOS entry pointer */
    struct finode *pprev;           /* Previous DOS entry pointer */
    UINT16      abs_length;         /* Absolute path length */
} FINODE;

/* contain location information for a directory */
typedef struct dirblk
{
    UINT32      my_frstblock;       /* First block in this directory */
    UINT32      my_block;           /* Current block number */
    UINT16      my_index;           /* dirent number in my cluster   */
    /* fname[0] == "\0" entry blocks and index */
    UINT32      end_block;          /* End block number in this directory entry */
    UINT16      end_index;          /* End directory entry number in my cluster */
} DIRBLK;

/* Object used to find a dirent on a disk and its parent's */
#define FUSE_UPBAR      2            /* Short file name used upperbar
                                         First six characters of the file name plus ~n */
typedef struct drobj
{
    DDRIVE      *pdrive;            /* Block 0 image from the disk */
    FINODE      *finode;            /* Dos entry */
    DIRBLK      blkinfo;            /* Directory information */
    INT         isroot;             /* True if this is the root */
    BLKBUFF     *pblkbuff;          /* Block buffer pointer */
    LNAMINFO    linfo;              /* for long file name */
} DROBJ;

/* Internal file representation */
typedef struct pc_file
{
    DROBJ       *pobj;              /* Info for getting at the inode */
    UINT16      flag;               /* Access flags from fat_open(). */
    UINT32      fptr;               /* Current file pointer */
    UINT32      fptr_cluster;       /* Current cluster boundary for fptr */
    UINT32      fptr_block;         /* Block address at boundary of fprt_cluster */
    INT         is_free;            /* If YES this FILE may be used (see pc_memry.c) */
    INT         fupdate;            /* File update flag */
    INT         at_eof;             /* True if fptr was > alloced size last time we set
                                       it. If so synch_file pointers will fix it if the
                                       file has expanded. */
} PC_FILE;

/* File search structure */
typedef struct fat_dstat_struct
{
    CHAR       sfname[MAX_SFN +1];    /* Null terminated file and extension */
    CHAR       fext[MAX_EXT+1];
    CHAR       lfname[MAX_LFN+1];  /* Null terminated long file name */
    UINT8       fattribute;         /* File attributes */
    UINT8       fcrcmsec;           /* File create centesimal millisecond */
    UINT16      fcrtime;            /* File create time */
    UINT16      fcrdate;            /* File create date */
    UINT16      faccdate;           /* Access date */
    UINT16      fclusterhigh;       /* High cluster for data file */
    UINT16      fuptime;            /* File update time */
    UINT16      fupdate;            /* File update */
    UINT16      fclusterlow;        /* Low cluster for data file */
    UINT32      fsize;              /* File size */

    /* INTERNAL */
    UINT8       pname[MAX_LFN+1];  /* Pattern. */
    UINT8       pext[MAX_EXT+1];
    INT8        path[MAX_LFN+1];
    UINT16      dh;                 /* Disk handle. */
    DROBJ       *pobj;              /* Info for getting at the inode */
    DROBJ       *pmom;              /* Info for getting at parent inode */
    struct fat_dstat_struct   *next;
} FAT_DSTAT;

/* FAT types */
enum FSFAT_TYPE
{
    FSFAT_12,
    FSFAT_16,
    FSFAT_32,
    FSFAT_AUTO
};

/* User supplied parameter block for formatting */
typedef struct fmtparms
{
    enum FSFAT_TYPE fat_type;       /* FAT12, FAT16, or FAT32 */
    UINT8       partdisk;           /* 1 : Partitioned disk, 0 : Not a partitioned disk */
    UINT16      bytepsec;           /* Bytes per sector */
    UINT8       secpalloc;          /* Sectors per cluster */
    UINT16      secreserved;        /* Reserved sectors before the FAT */
    UINT8       numfats;            /* Number of FATS on the disk */
    UINT16      numroot;            /* Maximum # of root dir entries. FAT32 always 0 */
    UINT8       mediadesc;          /* Media descriptor byte */
    UINT16      secptrk;            /* Sectors per track */
    UINT16      numhead;            /* Number of heads */
    UINT16      numcyl;             /* Number of cylinders */
    UINT32      totalsec;           /* Total sectors */

    INT8        oemname[9];             /* Only first 8 bytes are used */
    UINT8       physical_drive_no;      /* 0x00 for floppies and 0x80 for hard disks */
    UINT32      binary_volume_label;    /* Volume ID or Serial Number */
    INT8        text_volume_label[12];  /* Volume Label */
} FMTPARMS;

/* Arguments to po_extend_file */
#define PC_FIRST_FIT    1
#define PC_BEST_FIT     2
#define PC_WORST_FIT    3

/* IDE Driver Error code */
#define     NUF_IDE_ASSIGN              -3100      /* Logical drive assign error. */
#define     NUF_IDE_NUM_LOGICAL         -3101      /* NUM_LOGICAL_DRIVES set error. */
#define     NUF_IDE_NUM_PHYSICAL        -3102      /* NUM_PHYSICAL_DRIVES set error. */
#define     NUF_IDE_LOG_TABLE           -3103      /* LOG_DISK_TABLE setup error. */
#define     NUF_IDE_PHYS_TALBE          -3104      /* PHYS_DISK_TABLE setup error. */
#define     NUF_IDE_INITIALIZE          -3105      /* Initialize error. See c_s[].err_code */
#define     NUF_IDE_NOT_SETCOUNT        -3106      /* Read/Write sector count is zero. */
#define     NUF_IDE_NOT_LOG_OPENED      -3107      /* Logical drive is not opened. */
#define     NUF_IDE_NOT_PHYS_OPENED     -3108      /* Physical drive is not opened. */
#define     NUF_IDE_DISK_SIZE           -3109      /* illegal partition size. */
#define     NUF_IDE_FAT_TYPE            -3110      /* illegal FAT type. */
#define     NUF_IDE_NO_DOSPART          -3111      /* NO DOS partition in disk. */
#define     NUF_IDE_NO_EXTPART          -3112      /* NO Extension partition in disk. */
#define     NUF_IDE_NOT_CAPACITY        -3113      /* Partition capacity error. */
#define     NUF_IDE_OVER_PART           -3114      /* Over the partition end sectors. */
#define     NUF_IDE_MAX_BLOCKS          -3115      /* More than max blocks access. */
#define     NUF_IDE_RESET               -3116      /* Controller reset failed in initialize. */
#define     NUF_IDE_DIAG_FAILED         -3117      /* Drive diagnostic failed in initialize. */
#define     NUF_IDE_SETMULTIPLE         -3118      /* Set multiple mode command failed. */
#define     NUF_IDE_INITPARMS           -3119      /* initialize parameters failed in initialize */
#define     NUF_IDE_NOT_READY           -3120      /* Drive not ready. */
#define     NUF_IDE_CMDERROR            -3121      /* IDE command error. See error register. */
#define     NUF_IDE_BUSERROR            -3122      /* DRQ should be asserted but it isn't. */
#define     NUF_IDE_EVENT_TIMEOUT       -3123      /* Event timeout. */

/* Partition table descriptions. */
typedef struct ptable_entry
{
    UINT8       boot;                /* BootSignature */
    UINT8       s_head;              /* Start Head */
    UINT8       s_sec;               /* Start Sector(Bit0-5) Bit6 and 7 are cylinder number */
    UINT8       s_cyl;               /* Start Cylinder Upper two bit of starting cylinder number are in StartSector field. */
    UINT8       p_id;                /* Partition type identifier */
    UINT8       e_head;              /* End Head */
    UINT8       e_sec;               /* End Sector(Bit0-5) Bit6 and 7 are cylinder number*/
    UINT8       e_cyl;               /* End Cylinder Upper two bit of ending cylinder number are in StartSector field. */
    UINT32      r_sec;               /* Relativity sector */
    UINT32      p_size;              /* Size of partition */
} PTABLE_ENTRY;

/* (Master) Boot Record structure */
typedef struct ptable
{
    PTABLE_ENTRY    ents[4];        /* Entry table */
    UINT16          signature;      /* should be 0xAA55 */
} PTABLE;

/* Nucleus FILE uses event groups to synchronize access to file system objects
 * via calls to NU_Retrieve_Events() and NU_Set_Events() in Nucleus PLUS.
 *
 * NUF_FIRST_EVENT_NUMBER is used by pc_memory_init to assign event handles to
 * various file system objects.
 *
 * If device drivers are being built, we grab a few event channels for interrupt
 * processing in the drivers.
 *
 * gl_NUF_NUM_EVENTS is the total number of events used by NUCLEUS File.
 * IN_System_Event_Groups must be at least this large.
 */

#define NUF_FIRST_EVENT_NUMBER  0        /* May be changed */
extern UINT16 gl_NUF_NUM_EVENTS;

/*  This external declaration is for the conversion between
 *  event group IDs and the pointers used by Nucleus PLUS.
 */
extern  NU_EVENT_GROUP  *NUFP_Events;

#define FAT_FILE_SYSTEM         1

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#include "storage/fsdev_extr.h"
/* Do not change the path for fat_extr.h. */
#include "storage/fat_extr.h"

#endif   /* __FAT_DEFS__ */

