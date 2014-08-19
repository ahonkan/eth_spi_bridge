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
*       pcdisk.h
*
* COMPONENT
*
*       API
*
* DESCRIPTION
*
*       Contains generic includes for API
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/reg_api.h"
#include "storage/file_cfg.h"
#include "storage/fileinit.h"
#include "storage/fs_defs.h"
#include "storage/fsl_defs.h"
#include "storage/fst_defs.h"
#include "storage/dir_defs.h"
#include "storage/dev_defs.h"
#include "storage/dh_extr.h"
#include "storage/date_extr.h"
#include "storage/part_defs.h"
#include "storage/util_extr.h"
#include "storage/user_extr.h"
#include "storage/list.h"
#include "storage/encod_extr.h"
#include "storage/dvc_w.h"
#include "storage/error_extr.h"
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)
#include "storage/bcm_extr.h"
#endif
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1)
#include "storage/uni_util_extr.h"
#endif

#ifndef __PCDISK__
#define __PCDISK__ 1

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/*** FILE DM definitions ***/ 
#define STORAGE_LABEL   {0xb8,0x2f,0x9c,0x12,0xee,0xa1,0x41,0x07,0x86,0x62,0xb4,0x01,0x26,0xc8,0x04,0xd3}

#define STORAGE_CMD_BASE        (DV_IOCTL0 + 1)
#define FDEV_GET_DISK_INFO      0 
#define FDEV_GET_DISK_STATUS    1
#define FDEV_FLUSH              2
#define FDEV_TRIM               3
#define FDEV_GET_MW_CONFIG_PATH 4
#define TOTAL_STORAGE_IOCTLS    5

typedef struct _storage_mw_config_path_struct
{
    CHAR    config_path[REG_MAX_KEY_LENGTH];
    UINT32  max_path_len;

} STORAGE_MW_CONFIG_PATH;

/*** END FILE DM definitions ***/

#define YES                     1
#define NO                      0

#if ( !(defined(FAR)))
    #define FAR
#endif
#if ( !(defined(NEAR)))
    #define NEAR
#endif

/* File creation permissions for open */
/* Note: OCTAL */
#define PS_IWRITE       0000400U         /* Write permitted      */
#define PS_IREAD        0000200U         /* Read permitted. (Always true anyway)*/

/* File Attribute definitions. */
#define ARDONLY     0x01        /* Read only file attributes */
#define AHIDDEN     0x02        /* Hidden file attributes */
#define ASYSTEM     0x04        /* System file attributes */
#define AVOLUME     0x08        /* Volume Label file attributes */
#define ADIRENT     0x10        /* Directory file attributes */
#define ARCHIVE     0x20        /* Archives file attributes */
#define ANORMAL     0x00        /* Normal file attributes */


/* File access flags */
#define PO_RDONLY       0x0000U          /* Open for read only*/
#define PO_WRONLY       0x0001U          /* Open for write only*/
#define PO_RDWR         0x0002U          /* Read/write access allowed.*/
#define PO_APPEND       0x0008U          /* Seek to eof on each write*/
#define PO_CREAT        0x0100U          /* Create the file if it does not exist.*/
#define PO_TRUNC        0x0200U          /* Truncate the file if it already exists*/
#define PO_EXCL         0x0400U          /* Fail if creating and already exists*/
#define PO_TEXT         0x4000U          /* Ignored*/
#define PO_BINARY       0x8000U          /* Ignored. All file access is binary*/
#define PO_NOSHAREANY   0x0004U          /* Wants this open to fail if already
                                           open.  Other opens will fail while
                                           this open is active */
#define PO_NOSHAREWRITE 0x0800U          /* Wants this opens to fail if already
                                           open for write. Other open for
                                           write calls will fail while this
                                           open is active. */
/* Arguments to SEEK */
#define PSEEK_SET       0               /* offset from beginning of file*/
#define PSEEK_CUR       1               /* offset from current file pointer*/
#define PSEEK_END       2               /* offset from end of file*/

/* VFS event group for applications to monitor file system, device, and mount points */  
extern NU_EVENT_GROUP FILE_Application_Event_Group;

/* Nucleus events for application notification of device removal and creation */
#define NUF_EVT_DEVICE_CREATE   0x0001
#define NUF_EVT_DEVICE_DESTROY  0x0002
#define NUF_EVT_MOUNT_CREATE    0x0004
#define NUF_EVT_MOUNT_DESTROY   0x0008
#define NUF_EVT_FS_CREATE       0x0010
#define NUF_EVT_FS_DESTROY      0x0020


#define MAXSECTORS              256     /* Maximum sectors */

/* Number of maximum file sectors(2GB) */
#define MAXFILE_SIZE            4194304 /* (2GB / 512) */


/* Define service completion status constants.  */
/* Nucleus FILE status value */
#define     NUF_BAD_USER        -3000       /* Not a file user. */
#define     NUF_BADDRIVE        -3001       /* Bad drive number. */
#define     NUF_BADPARM         -3002       /* Invalid parameter given */
    /* Disk */
#define     NUF_NOT_OPENED      -3003       /* The disk is not opened yet. */
#define     NUF_NO_DISK         -3004       /* Disk is removed. */
#define     NUF_DISK_CHANGED    -3005       /* Disk is changed. */
#define     NUF_INVALID_CSIZE   -3006       /* The Disk has invalid cluster size. */
#define     NUF_FATCORE         -3007       /* Fat cache table too small. */
#define     NUF_DEFECTIVEC      -3008       /* Defective cluster detected. */
#define     NUF_BADDISK         -3009       /* Bad Disk */
#define     NUF_NO_PARTITION    -3010       /* No partition in disk. */
#define     NUF_ROOT_FULL       -3011       /* Root directory full */
    /* Format */
#define     NUF_NOFAT           -3012       /* No FAT type in this partition. Can't Format */
#define     NUF_FMTCSIZE        -3013       /* Too many clusters for this partition. Can't Format */
#define     NUF_FMTFSIZE        -3014       /* File allocation table too small. Can't Format. */
#define     NUF_FMTRSIZE        -3015       /* Numroot must be an even multiple of 16  */
#define     NUF_FORMAT          -3016       /* Not formatted this disk. */
    /* Path */
#define     NUF_LONGPATH        -3017       /* Path or filename too long */
#define     NUF_INVNAME         -3018       /* Path or filename is invalid.  */
    /* File */
#define     NUF_PEMFILE         -3019       /* No file descriptors available (too many files open). */
#define     NUF_BADFILE         -3020       /* Invalid file descriptor */
#define     NUF_ACCES           -3021       /* Attempt to open a read only file or a special (directory). */
#define     NUF_NOSPC           -3022       /* Write failed. Presumably because of no space. */
#define     NUF_SHARE           -3023       /* The access conflict from multiple task to a specific file. */
#define     NUF_NOFILE          -3024       /* File not found or path to file not found. */
#define     NUF_EXIST           -3025       /* Exclusive access requested but file already exists. */
#define     NUF_NVALFP          -3026       /* Seek to negative file pointer attempted. */
#define     NUF_MAXFILE_SIZE    -3027       /* Over the maximum file size. */
#define     NUF_NOEMPTY         -3028       /* Directory is not empty.     */
#define     NUF_INVPARM         -3029       /* Invalid parameter is specified.  */
#define     NUF_INVPARCMB       -3030       /* Invalid parameter combination is specified.  */
    /* Memory  */
#define     NUF_NO_MEMORY       -3031       /* Can't allocate internal buffer. */
#define     NUF_NO_BLOCK        -3032       /* No block buffer available */
#define     NUF_NO_FINODE       -3033       /* No FINODE buffer available */
#define     NUF_NO_DROBJ        -3034       /* No DROBJ buffer available */
#define     NUF_IO_ERROR        -3035       /* Driver IO function routine returned error */

#define     NUF_INTERNAL        -3036       /* Nucleus FILE internal error */


/* New return values for VFS API */
#define     NUF_IN_USE          -3037       /* Resource is in use, prevents requested operation */

/* Device name errors */
#define     NUF_INVALID_DEVNAME     -3038
#define     NUF_DEVICE_TABLE_FULL   -3039
#define     NUF_DUPLICATE_DEVNAME   -3040

/* Disk handle errors */
#define     NUF_DISK_TABLE_FULL     -3041

/* Mount table errors */
#define     NUF_MOUNT_TABLE_FULL    -3042
#define     NUF_MOUNT_NOT_AVAILABLE -3043    /* Mount is busy or not known to system */

/* File system table errors */
#define     NUF_FS_NOT_FOUND        -3044    /* File system is not known to system */
#define     NUF_DUPLICATE_FSNAME    -3045
#define     NUF_FS_TABLE_FULL       -3046

/* Partition services errors */
#define     NUF_PART_TABLE_FULL     -3047   /* Only four primaries, or one extended and three primaries can exist */
#define     NUF_PART_EXT_EXISTS     -3048   /* Only one extended partition can exist */
#define     NUF_PART_NO_EXT         -3049   /* Logical partitions can only be created within an extended partition */
#define     NUF_PART_LOG_EXISTS     -3050   /* Extended partitions cannot be removed if logical partitions exist */

/* Device driver errors */
#define     NUF_INVALID_DEV_IOCTL   -3051   /* The requested IOCTL command is undefined by the driver. */

/* FAT Check Disk Utility Error Codes */
#define NUF_FAT_TABLES_DIFFER       -3052           /* FAT tables aren't the same. */
#define NUF_LOG_FILE_CREATED        -3053           /* Log file was created when
                                                       NU_Check_Disk was ran. */
#define NUF_CHK_STACK_EMPTY         -3054           /* Stack used for traversing the
                                                       directory hierarchy is empty. */
#define NUF_CHK_CL_INVALID          -3055           /* Cluster value is invalid. */
#define     NUF_NO_ACTION           -3056           /* Function exits without performing requested action */


/* User layer APIs */
STATUS  NU_VFS_Init(VOID* config_s);
STATUS  NU_Register_File_System(CHAR *name, FS_S *fs);
STATUS  NU_Unregister_File_System(CHAR *name);
STATUS  NU_Mount_File_System(CHAR *fs_name, CHAR *mount_point, CHAR* dev_name, VOID *config);
STATUS  NU_Unmount_File_System(CHAR *mount_point);
STATUS  NU_Format(CHAR *fs_name, CHAR *dev_name, VOID **params);
STATUS  NU_Get_Format_Info(CHAR *fs_name, CHAR *dev_name, VOID **params);
STATUS  NU_Open_Disk(CHAR *path) ESAL_TS_RTE_DEPRECATED;
INT     NU_Open(CHAR *name, UINT16 flag, UINT16 mode);
STATUS  NU_Close(INT fd);
INT32   NU_Read(INT fd, CHAR *buf, INT32 count);
INT32   NU_Write(INT fd, CHAR *buf, INT32 count);
INT32   NU_Seek(INT fd, INT32 offset, INT16 origin);
STATUS  NU_Delete(CHAR *name);
STATUS  NU_Truncate(INT fd, INT32 offset);
STATUS  NU_Flush(INT fd);
STATUS  NU_Make_Dir(CHAR *name);
STATUS  NU_Remove_Dir(CHAR *name);
STATUS  NU_Get_Attributes(UINT8 *attr, CHAR *name);
STATUS  NU_Set_Attributes(CHAR *name, UINT8 newattr);
STATUS  NU_FreeSpace(CHAR *, UINT8 *, UINT16 *, UINT32 *, UINT32 *);
STATUS  NU_Get_First(DSTAT *statobj, CHAR *pattern);
STATUS  NU_Get_Next(DSTAT *statobj);
STATUS  NU_Create_File_Device(CHAR *devname, FDEV_OP_S *dev_ops, VOID *args);
STATUS  NU_Remove_File_Device(CHAR *devname);
STATUS  NU_Set_Default_Drive(UINT16 driveno);
INT16   NU_Get_Default_Drive(VOID);
STATUS  NU_Set_Current_Dir(CHAR *path);
STATUS  NU_Current_Dir(UINT8 *drive, CHAR *path);
STATUS  NU_List_File_System(FS_LIST_S **);
STATUS  NU_List_Device(DEV_LIST_S **);
STATUS  NU_List_Mount(MNT_LIST_S **);
STATUS  NU_Free_List(VOID **);
STATUS NU_Storage_Device_Wait(CHAR *mount_name, UNSIGNED total_suspend);


/* Check to see whether to include Check Disk Utility. */
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)
    STATUS  NU_Check_Disk(CHAR *path,UINT8 flag,UINT8 mode);
#endif

#if (FILE_VERSION_COMP == FILE_2_5)
    INT     NU_Close_Disk(CHAR *path);
    VOID    NU_Disk_Abort(CHAR *path);
    INT     NU_Rename(CHAR *name, CHAR *newname);
    VOID    NU_Done(DSTAT *statobj);
    INT     NU_Check_File_User(VOID);
    INT     NU_Become_File_User(VOID);
    VOID    NU_Release_File_User(VOID);
#elif (FILE_VERSION_COMP > FILE_2_5)
    STATUS  NU_Close_Disk(CHAR *path) ESAL_TS_RTE_DEPRECATED;
    STATUS  NU_Disk_Abort(CHAR *path) ESAL_TS_RTE_DEPRECATED;
    STATUS  NU_Rename(CHAR *name, CHAR *newname);
    STATUS  NU_Done(DSTAT *statobj);
    STATUS  NU_Check_File_User(VOID) ESAL_TS_RTE_DEPRECATED;
    STATUS  NU_Become_File_User(VOID) ESAL_TS_RTE_DEPRECATED;
    STATUS  NU_Release_File_User(VOID) ESAL_TS_RTE_DEPRECATED;
#endif

STATUS  NU_Get_Partition_Info(CHAR *, UINT16 *, UINT32 *, UINT32 *, UINT8 *, UINT8 *);
STATUS  NU_List_Partitions(CHAR *dev_name, PFPART_LIST_S *plist);
VOID    NU_Free_Partition_List(PFPART_LIST_S p_list);
STATUS  NU_Create_Partition(CHAR *, UINT16 , UINT32 , UINT32 , UINT8);
STATUS  NU_Delete_Partition(CHAR *dev_name);
STATUS  get_driveno_from_dstat(DSTAT *statobj,INT *drive);
STATUS  NU_Utime(DSTAT *, UINT16, UINT16, UINT16, UINT16 ,UINT16, UINT16);
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)
    STATUS NU_FILE_Cache_Create (CHAR *path, BCM_CACHE_TYPE cache_type, VOID *cache_config);
    STATUS NU_FILE_Cache_Destroy (CHAR *path);
    STATUS NU_FILE_Cache_Get_Config (CHAR *path, VOID **config);
    STATUS NU_FILE_Cache_Set_Config (CHAR *path, VOID *config);
    STATUS NU_FILE_Cache_Flush (CHAR *path);
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */
#endif /* __PCDISK__ */

