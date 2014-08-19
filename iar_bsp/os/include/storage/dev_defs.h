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
*       dev_defs.h
*
* COMPONENT
*
*       Device
*
* DESCRIPTION
*
*       Contains device defines and dispatch dispatch structures
*
* DATA STRUCTURES
*
*       FDEV_OP_S                   Operations dispatch structure
*       FDEV_S                      File device structure
*
* FUNCTIONS
*
*       None.
*
*************************************************************************/
#ifndef DEV_DEFS_H
#define DEV_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Structure definition for device operations dispatch table */
typedef struct file_dev_op_struct
{

    /* Open */
    STATUS     (*open_proc)(UINT16 dh, CHAR *devname, VOID *args);

    /* Close */
    STATUS     (*close_proc)(UINT16 dh);

    /* Read & Write sector offset. Offset is physical */
    STATUS     (*io_proc)(UINT16 dh, UINT32 sector, VOID *buffer, UINT16 count, INT reading);

    /* ioctl. User function call to device driver. */
    STATUS     (*ioctl_proc)(UINT16 dh, UINT16 command, VOID *buffer, INT ioctl_data_len);

    /* Return disk presence */
    STATUS     (*dskchk_proc)(UINT16 dh);

}FDEV_OP_S;

enum FDEV_IOCTL_COMMAND
{
    FDEV_GET_DISK_INFO,
    FDEV_REFRESH_LOG_DEV
};

/* Configuration defines and limits */
#define FS_DEV_MAX_DEVNAME  FILE_MAX_DEVICE_NAME
#define FS_DEV_MAX_DEVICES  CFG_NU_OS_STOR_FILE_VFS_MAX_DEVICES

/* File device structure flag defines */
#define FDEV_FL_VALID         0x1     /* Device table entry is valid */
#define FDEV_FL_NOT_MOUNTABLE 0x2     /* Device cannot be mounted */
/* Structure definition for a device table entry */
typedef struct file_dev_struct
{
    FDEV_OP_S   fdev_ops;
    UINT16      fdev_dh;            /* Disk handle */
    UINT32      fdev_flags;
    UINT32      fdev_cnt;
    UINT32      fdev_last;          /* Last element from listing, only used in
                                       get first/next/done operations */
    CHAR        fdev_name[FS_DEV_MAX_DEVNAME];
}FDEV_S;


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* DEV_DEFS_H */
