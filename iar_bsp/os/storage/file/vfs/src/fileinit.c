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
*
*  FILE NAME
*
*      fileinit.c
*
*  COMPONENT
*
*      Initialization
*
*  DESCRIPTION
*
* This module contains functions used to initialize the hardware
* in your system.  It must be modified for your individual setup.
* The function file_init() must be called from a task to setup
* the file system before any other tasks attempt to use it.
* The task that calls file_init() will automatically be setup
* as a File User.
*
*  FUNCTIONS
*
*       nu_os_stor_file_vfs_init
*       file_init
*       Storage_Device_Register_Callback
*       Storage_Device_Unregister_Callback
*       finit_file_systems
*       finit_add_device
*       finit_register_device
*       finit_volume_enumeration
*       finit_mount_device
*       finit_auto_format_device
*       finit_remove_logical_devices
*       finit_find_unused_mount_point
*
************************************************************************/
/* Include necessary Nucleus PLUS files.  */
#include    "storage/pcdisk.h"
#include    "storage/fileinit.h"
#include    "storage/fsl_extr.h"
#include    "storage/fd_defs.h"
#include    "services/runlevel_init.h"
#include    "services/nu_trace_os_mark.h"

#ifdef CFG_NU_OS_STOR_FILE_FS_FAT_ENABLE
#include    "storage/fat_defs.h"
#endif

extern NU_MEMORY_POOL System_Memory;
extern FDEV_OP_S vfs_dvc_log_ops;
extern FDEV_OP_S vfs_dvc_phys_ops;

#ifdef CFG_NU_OS_STOR_FILE_FS_FAT_ENABLE
extern STATUS FAT_Init_FS(CHAR *fsname);
#endif

#ifdef CFG_NU_OS_STOR_FILE_FS_SAFE_ENABLE
extern STATUS WR_Safe_Init_FS(CHAR *fsname);
#endif

STATIC STATUS finit_add_device(DV_DEV_ID dev_id, DV_DEV_LABEL *device_type_label, INT label_count);
STATIC STATUS finit_file_systems(VOID);
STATIC STATUS finit_mount_device(FDRV_LOG_CB_S *p_lcb, BOOLEAN primary);
STATIC STATUS finit_register_device(FDRV_DM_CB_S dm_cb, FPART_DISK_INFO_S *dev_info);
STATIC STATUS finit_auto_format_device(FDRV_LOG_CB_S *p_lcb, CHAR *fs, CHAR *mp);
STATIC STATUS finit_find_unused_mount_point(CHAR *mp);
STATIC STATUS finit_unregister_device(DV_DEV_ID dev_id);
STATIC STATUS finit_get_device_name_by_id(CHAR *dev_name, DV_DEV_ID dev_id);
STATIC STATUS finit_abort_volume(CHAR *dev_name);
STATIC STATUS finit_clear_count(CHAR* dev_name);
STATIC STATUS finit_remove_device(CHAR *dev_name);

/* Call back routines for STORAGE_LABEL device register/unregister event. */
STATIC STATUS Storage_Device_Register_Callback(DV_DEV_ID device, VOID *context);
STATIC STATUS Storage_Device_Unregister_Callback(DV_DEV_ID device, VOID *context);

/* --------------------------------------------------
 * File System Initialization data.
 * --------------------------------------------------*/
typedef struct init_fs
{
    CHAR    *fsname;            /* FSC_MAX_FS_NAME - 1 characters */
    STATUS (*init)(CHAR*);
} INIT_FS;

INIT_FS file_systems[] =
{
#ifdef CFG_NU_OS_STOR_FILE_FS_FAT_ENABLE
        {"FAT", FAT_Init_FS},
#endif
#ifdef CFG_NU_OS_STOR_FILE_FS_SAFE_ENABLE
        {"SAFE",WR_Safe_Init_FS},
#endif
        {0, 0}
};

CHAR finit_reg_key[REG_MAX_KEY_LENGTH];

/**********************************************************************
*
*  FUNCTION
*
*      nu_os_stor_file_vfs_init
*
*  DESCRIPTION
*      Component initialization function. Starts Storage initialization
*       and saves registration key path.
*
*  ROUTINES CALLED
*      file_init()
*
*  INPUTS
*       *key                Unused
*       compctrl
*
*  OUTPUTS
*       None
*
************************************************************************/
VOID nu_os_stor_file_vfs_init (const CHAR * key, INT compctrl)
{
    STATUS status;
    CHAR tmp_path[REG_MAX_KEY_LENGTH];

#if (CFG_NU_OS_STOR_FILE_VFS_EXPORT_SYMBOLS == NU_TRUE)

    /* Keep symbols for nu.os.stor.file.vfs */
    NU_KEEP_COMPONENT_SYMBOLS(NU_OS_STOR_FILE_VFS);

#endif /* CFG_NU_OS_STOR_FILE_VFS_EXPORT_SYMBOLS */

    if(compctrl == RUNLEVEL_START)
    {
        /* Retrieve run-time initialization constants. */
        status = (REG_Has_Key(key) ? NU_SUCCESS : NUF_INTERNAL);
        if (status == NU_SUCCESS)
        {
            /* Set the global variable for the maximum number of open files */
            strncpy(tmp_path, key, strlen(key)+1);
            strncat(tmp_path, FINIT_REG_OPT_MAX_OPEN_FILES, REG_MAX_KEY_LENGTH-strlen(tmp_path));
            status = REG_Get_UINT16(tmp_path, &gl_VFS_MAX_OPEN_FILES);
        }

        if (status == NU_SUCCESS)
        {
            gl_FD_MAX_FD = (gl_VFS_MAX_OPEN_FILES * CFG_NU_OS_STOR_FILE_VFS_NUM_USERS);

            /* Initialize the component */
            status = file_init(&System_Memory);
        }

        if (status == NU_SUCCESS)
        {

            /* Save the config path */
            strncpy(finit_reg_key, key, sizeof(finit_reg_key));
        }

        if (status != NU_SUCCESS)
        {
            /* TODO  Report error */
        }
    }
}

/**********************************************************************
*
*  FUNCTION
*
*      file_init
*
*  DESCRIPTION
*      This function is responsible for initializing the file system
*       and creating the device discovery task.
*
*  ROUTINES CALLED
*      NU_VFS_Init()
*      finit_file_systems()
*
*  INPUTS
*       *config_s                   Configuration parameters - memory pool
*                                   used for memory allocation
*
*  OUTPUTS
*       NU_SUCCESS                  Initialization completed successfully
*       < 0                         Component specific error
*
************************************************************************/
STATUS  file_init(VOID *config_s)
{
    STATUS             status;     /* Return value from function calls */
    DV_DEV_LABEL       storage_device_label = {STORAGE_LABEL};
    DV_LISTENER_HANDLE listener_id;

    status = NU_VFS_Init(config_s); /* Init the generic FILE component */
    if (status == NU_SUCCESS)
    {
        status = finit_file_systems(); /* Init file systems */

        if (status == NU_SUCCESS)
        {
            /* Call DM API to add callbacks for storage devices register
             * and unregister event. */
            status = DVC_Reg_Change_Notify(&storage_device_label,
                                           DV_GET_LABEL_COUNT(storage_device_label),
                                           &Storage_Device_Register_Callback,
                                           &Storage_Device_Unregister_Callback,
                                           NU_NULL,
                                           &listener_id);
        }
    }

    return(status);
}

/*****************************************************************************
*
*   FUNCTION
*
*       Storage_Device_Register_Callback
*
*   DESCRIPTION
*       Callback function for new storage device addition event.
*
*   INPUTS
*       device                      Device ID of newly registered storage
*                                   device.
*       context                     Context information for this callback.
*                                   Unused (null) for this component.
*
*   OUTPUTS
*       NU_SUCCESS                  Request completed successfully.
*       < 0                         Component specific error.
*
*****************************************************************************/
STATIC STATUS Storage_Device_Register_Callback(DV_DEV_ID device, VOID *context)
{
    STATUS          status;   /* Return value from function call. */
    DV_DEV_LABEL    storage_device_label = {STORAGE_LABEL};

    status = finit_add_device(device,
                              &storage_device_label,
                              DV_GET_LABEL_COUNT(storage_device_label));

    return status;
}

/*****************************************************************************
*
*   FUNCTION
*
*       Storage_Device_Unregister_Callback
*
*   DESCRIPTION
*       Callback function for previous storage device removal event.
*
*   INPUTS
*       device                      Device ID of newly unregistered storage
*                                   device.
*       context                     Context information for this callback.
*                                   Unused (null) for this component.
*
*   OUTPUTS
*       NU_SUCCESS                  Request completed successfully.
*       < 0                         Component specific error.
*
*****************************************************************************/
STATIC STATUS Storage_Device_Unregister_Callback(DV_DEV_ID device, VOID *context)
{
    STATUS status;             /* Return value from function call. */

    status = finit_unregister_device(device);

    return status;
}

/*****************************************************************************
*
*   FUNCTION
*
*       finit_file_systems
*
*   DESCRIPTION
*       Performs initialization for file systems configured for static
*       initialization.
*
*   INPUTS
*       None
*
*   OUTPUTS
*       NU_SUCCESS                  Request completed successfully
*       < 0                         Component specific error
*
*****************************************************************************/
STATIC STATUS finit_file_systems(VOID)
{
    STATUS sts = NU_SUCCESS;
    INT    idx = 0;

    while( (file_systems[idx].init) && (sts == NU_SUCCESS) )
    {
        sts = file_systems[idx].init(file_systems[idx].fsname);
        idx++;
    }
    return (sts);
}


/*****************************************************************************
*
*   FUNCTION
*
*       finit_add_device
*
*   DESCRIPTION
*       Creates a physical device, logical devices for all partitions,
*        and mounts the logical devices according to the registry data.
*
*   INPUTS
*       dev_id              DM ID to pass into Open routine
*       device_type_label   DM label list pointer
*       label_count         Number of labels in the list
*
*   OUTPUTS
*       NU_SUCCESS                  Request completed successfully
*       < 0                         Component specific error
*
*****************************************************************************/
STATIC STATUS finit_add_device(DV_DEV_ID dev_id, DV_DEV_LABEL *device_type_label, INT label_count)
{
    STATUS sts;
    DV_DEV_HANDLE dev_handle;
    DV_IOCTL0_STRUCT ioctl0;
    FPART_DISK_INFO_S dev_info;
    FDRV_DM_CB_S dm_cb;


    memset(&dev_info, 0, sizeof(dev_info));

    /* Open the device so that the IOCTL can be used. */
    sts = DVC_Dev_ID_Open(dev_id, device_type_label, label_count, &dev_handle);
    if(sts == NU_SUCCESS)
    {
        /* Fill out a structure to pass the device handle and device id into finit_register_device */
        dm_cb.dh = dev_handle;
        dm_cb.dev_id = dev_id;

        /* Get the IOCTL base */
        ioctl0.label = *device_type_label;
        sts = DVC_Dev_Ioctl(dev_handle, DV_IOCTL0, &ioctl0, sizeof(ioctl0));
        if(sts == NU_SUCCESS)
        {
            /* Get the device info structure */
            sts = DVC_Dev_Ioctl(dev_handle, (ioctl0.base + FDEV_GET_DISK_INFO), &dev_info, sizeof(dev_info));
            if(sts == NU_SUCCESS)
            {
                /* Register the physical device with VFS */
                sts = finit_register_device(dm_cb, &dev_info);
                if(sts == NU_SUCCESS)
                {
                    /* Enumerate the partitions on the drive and create logical devices */
                    sts = finit_volume_enumeration(dev_info.fpart_name);
                }
                else
                {
                    /* There was a problem registering the device */
                    (VOID)DVC_Dev_Close(dev_handle);
                }
            }
            else
            {
                /* There was a problem registering the device */
                (VOID)DVC_Dev_Close(dev_handle);
            }
        }
        else
        {
            /* There was a problem registering the device */
            (VOID)DVC_Dev_Close(dev_handle);
        }
    }

    return(sts);
}

/************************************************************************
*
*  FUNCTION
*
*       finit_register_device
*
*  DESCRIPTION
*
*       Given a DM device handle and a disk info structure, create a
*       VFS physical device.
*
*  INPUTS
*
*       dev_handle          DM device handle returned by Open
*       dev_info            Device information structure
*
*  OUTPUTS
*
*       sts
*       NU_NO_MEMORY
*       NUF_BADPARM
*
**************************************************************************/
STATIC STATUS finit_register_device(FDRV_DM_CB_S dm_cb, FPART_DISK_INFO_S *dev_info)
{
    STATUS sts;
    FDRV_PHY_CB_S *pphy_cb;
    UINT16 phy_dh;

    /* Check that the name was set */
    if((dev_info != NU_NULL) && (dev_info->fpart_name != NU_NULL))
    {
        /* Create physical device */
        sts = NU_Create_File_Device(dev_info->fpart_name, &vfs_dvc_phys_ops, NU_NULL);
        if(sts == NU_SUCCESS)
        {
            /* Trace log */
            T_PHY_DEV_LIST(dev_info->fpart_name);
            
            /* Get the VFS dh */
            sts = fsdh_devname_to_dh(dev_info->fpart_name, &phy_dh);
            if(sts == NU_SUCCESS)
            {
                sts = fsdh_get_dh_specific(phy_dh, (VOID*)&pphy_cb, NU_NULL);
                if((sts == NU_SUCCESS)&&(pphy_cb != NU_NULL))
                {
                    pphy_cb->fdrv_dm_cb = dm_cb;
                    pphy_cb->fdrv_bytes_p_sec = dev_info->fpart_bytes_p_sec;
                    /* Associate the driver specific data with the physical device */
                    pphy_cb->fdrv_spec = dev_info->fpart_spec;
                }
            }
        }
    }
    else
    {
        /* invalid device name */
        sts = NUF_BADPARM;
    }

    return(sts);
}

/************************************************************************
* FUNCTION
*
*       finit_volume_enumeration
*
* DESCRIPTION
*       Create logical devices for each partition of a physical device
*
* INPUTS
*       dev_name                Physical device name
*
* OUTPUTS
*       NU_SUCCESS              Operation is successful
*       NU_NO_MEMORY            Insufficient memory
*
*************************************************************************/
STATUS finit_volume_enumeration(CHAR* dev_name)
{
    STATUS sts;
    PFPART_LIST_S part_list = NU_NULL, p_list = NU_NULL;
    FDRV_LOG_CB_S *p_lcb;
    UINT8 i = 0;
    UINT16 phys_dh, log_dh;
    BOOLEAN primary = NU_TRUE;
    CHAR log_name[FILE_MAX_DEVICE_NAME];

    /* Get the disk handle of the physical device */
    sts = fsdh_devname_to_dh(dev_name, &phys_dh);
    if(sts == NU_SUCCESS)
    {
        /* Get a list of the partitions on the disk */
        sts = NU_List_Partitions(dev_name, &part_list);
        if(sts == NUF_NO_PARTITION)
        {
            /* SAFE devices fail with this error code */
            sts = NU_SUCCESS;
        }
    }
    if (sts == NU_SUCCESS)
    {
        p_list = part_list;

        /* Loop through the list of partitions */
        do
        {
            /* Create a logical name */
            memset(log_name, 0, FILE_MAX_DEVICE_NAME);
            NUF_Copybuff(log_name, dev_name, FPART_MAX_PHYS_NAME-1);
            log_name[3] = '0'+i/10;
            log_name[4] = '0'+i%10;
            i++;

            /* Register the logical device with VFS */
            sts = NU_Create_File_Device(log_name, &vfs_dvc_log_ops, NU_NULL);
            if(sts == NU_SUCCESS)
            {
                /* Get the VFS dh */
                sts = fsdh_devname_to_dh(log_name, &log_dh);
                if(sts == NU_SUCCESS)
                {
                    /* Get the logical control block allocated by open */
                    sts = fsdh_get_dh_specific(log_dh, (VOID*)&p_lcb, NU_NULL);
                    if((sts == NU_SUCCESS)&&(p_lcb != NU_NULL))
                    {
                        /* Set the physical disk handle */
                        p_lcb->fdrv_vfs_dh = phys_dh;

                        /* Store the name */
                        strcpy(p_lcb->fdrv_name, log_name);

                        /* Fill out the logical control block with data from the partition structure */
                        if(p_list != NU_NULL)
                        {
                            p_lcb->fdrv_start =  p_list->fpart_start;
                            p_lcb->fdrv_end = p_list->fpart_end;
                        }

                        /* Mount the logical device */
                        sts = finit_mount_device(p_lcb, primary);
                        if(sts != NU_SUCCESS)
                        {
                            /* Log the error */
    /* TODO log error mounting a volume */
                        }
                        if((sts == NU_SUCCESS)&&(primary == NU_TRUE))
                        {
                            /* The next partition will use the auto_mnt_pt_start_loc */
                            primary = NU_FALSE;
                        }
                    }
                }
            }
            else
            {
/* TODO Log error creating device */
            }

            if(p_list != NU_NULL)
            {
                /* Advance to the next partition in the list */
                p_list = p_list->fpart_next;
            }
        }while(p_list != NU_NULL); /* end while */
    }


    /* Free the partition list */
    (VOID)NU_Free_Partition_List(part_list);

    return(sts);
}

/************************************************************************
* FUNCTION
*
*       finit_mount_device
*
* DESCRIPTION
*       Given a logical device, read in its mount configuration and
*       mount the volume. Parameter checked for NULL before calling.
*
* INPUTS
*       p_lcb                   Logical control block pointer
*       primary                 If true, then the mount point is
*                                enumerated starting with
*                                auto_mnt_pt_start_loc
*
* OUTPUTS
*       NU_SUCCESS              Operation is successful
*       NU_NO_MEMORY            Insufficient memory
*
*************************************************************************/
STATIC STATUS finit_mount_device(FDRV_LOG_CB_S *p_lcb, BOOLEAN primary)
{
    STATUS sts;
    UINT16 log_dh;
    STORAGE_MW_CONFIG_PATH config;
    CHAR tmp_path[REG_MAX_KEY_LENGTH];
    CHAR mp[] = {'\0','\0'};
    CHAR fs[FILE_MAX_FILE_SYSTEM_NAME];
    BOOLEAN auto_fmt = NU_FALSE;
    CPTE_S default_codepage = {DEFAULT_CODEPAGE,{NU_NULL,NU_NULL}};

    /* Clear locals */
    memset(fs, 0, FILE_MAX_FILE_SYSTEM_NAME);
    memset(&config, 0, sizeof(config));

    config.max_path_len = REG_MAX_KEY_LENGTH;

    /* Get the VFS dh */
    sts = fsdh_devname_to_dh(p_lcb->fdrv_name, &log_dh);
    if(sts == NU_SUCCESS)
    {
        /* Get the configuration path using the device IOCTL */
        sts = fs_dev_ioctl_proc(log_dh, FDEV_GET_MW_CONFIG_PATH, &config, sizeof(config));
        if(sts == NU_SUCCESS)
        {
            /* Validate the supplied config path */
            if(config.config_path != NU_NULL)
            {
                sts = REG_Has_Key(config.config_path);
                if(sts == NU_TRUE)
                {
                    sts = NU_SUCCESS;
                }
                else
                {
                    /* Config path is invalid. Set status to not NU_SUCCESS
                       so the default registry settings will be used. */
                    sts = !NU_SUCCESS;
                }
            }
            else
            {
                /* Config path is invalid. Set status to not NU_SUCCESS
                   so the default registry settings will be used. */
                sts = !NU_SUCCESS;
            }
        }
        if(sts == NU_SUCCESS)
        {
            /* Read in the registry settings for the device from the driver */
            if(primary == NU_TRUE)
            {
                /* Get the mount point */
                strncpy(tmp_path, config.config_path, strlen(config.config_path)+1);
                strncat(tmp_path, FINIT_REG_OPT_MNT_PT, REG_MAX_KEY_LENGTH-strlen(tmp_path));
                sts = REG_Get_String (tmp_path, mp, 1);
            }
            else
            {
                /* This is not the primary partition, so find a mount point
                   starting with the auto_mnt_pt_start_loc */
                strncpy(tmp_path, finit_reg_key, strlen(finit_reg_key)+1);
                strncat(tmp_path, FINIT_REG_OPT_AUTO_MNT_PT, REG_MAX_KEY_LENGTH-strlen(tmp_path));
                sts = REG_Get_String (tmp_path, mp, sizeof(mp)-1);
                if(sts == NU_SUCCESS)
                {
                    /* Validate the mount point */
                    sts = finit_find_unused_mount_point(mp);
                }
            }

            if(sts == NU_SUCCESS)
            {
                /* Get the file system type */
                strncpy(tmp_path, config.config_path, strlen(config.config_path)+1);
                strncat(tmp_path, FINIT_REG_OPT_MNT_FS, REG_MAX_KEY_LENGTH-strlen(tmp_path));
                sts = REG_Get_String (tmp_path, fs, FILE_MAX_FILE_SYSTEM_NAME);
            }

            if(sts == NU_SUCCESS)
            {
                /* Get the auto-format setting */
                strncpy(tmp_path, config.config_path, strlen(config.config_path)+1);
                strncat(tmp_path, FINIT_REG_OPT_MNT_AUTO_FMT, REG_MAX_KEY_LENGTH-strlen(tmp_path));
                sts = REG_Get_Boolean (tmp_path, &auto_fmt);
            }
        }
        else
        {
            /* Use the default mount configurations */
            if(primary == NU_TRUE)
            {
                /* Use the default mount point */
                strncpy(tmp_path, finit_reg_key, strlen(finit_reg_key)+1);
                strncat(tmp_path, FINIT_REG_OPT_GROUP_MNT, REG_MAX_KEY_LENGTH-strlen(tmp_path));
                strncat(tmp_path, FINIT_REG_OPT_MNT_PT, REG_MAX_KEY_LENGTH-strlen(tmp_path));
                sts = REG_Get_String (tmp_path, mp, 1);
            }
            else
            {
                /* This is not the primary partition, so find a mount point
                   starting with the auto_mnt_pt_start_loc */
                strncpy(tmp_path, finit_reg_key, strlen(finit_reg_key)+1);
                strncat(tmp_path, FINIT_REG_OPT_AUTO_MNT_PT, REG_MAX_KEY_LENGTH-strlen(tmp_path));
                sts = REG_Get_String (tmp_path, mp, sizeof(mp)-1);
            }

            if(sts == NU_SUCCESS)
            {
                /* Validate the mount point */
                sts = finit_find_unused_mount_point(mp);
            }

            if(sts == NU_SUCCESS)
            {
                /* Use the default file system type */
                strncpy(tmp_path, finit_reg_key, strlen(finit_reg_key)+1);
                strncat(tmp_path, FINIT_REG_OPT_GROUP_MNT, REG_MAX_KEY_LENGTH-strlen(tmp_path));
                strncat(tmp_path, FINIT_REG_OPT_MNT_FS, REG_MAX_KEY_LENGTH-strlen(tmp_path));
                sts = REG_Get_String (tmp_path, fs, FILE_MAX_FILE_SYSTEM_NAME);
            }

            if(sts == NU_SUCCESS)
            {
                /* Use the default auto-format setting */
                strncpy(tmp_path, finit_reg_key, strlen(finit_reg_key)+1);
                strncat(tmp_path, FINIT_REG_OPT_GROUP_MNT, REG_MAX_KEY_LENGTH-strlen(tmp_path));
                strncat(tmp_path, FINIT_REG_OPT_MNT_AUTO_FMT, REG_MAX_KEY_LENGTH-strlen(tmp_path));
                sts = REG_Get_Boolean (tmp_path, &auto_fmt);
            }
        }

        if(sts == NU_SUCCESS)
        {
            /* Attempt to mount the volume */
            sts = NU_Mount_File_System(fs, mp, p_lcb->fdrv_name, &default_codepage);
            if((sts == NUF_FORMAT)&&(auto_fmt == NU_TRUE))
            {
                /* Format the volume and re-try mounting */
                sts = finit_auto_format_device(p_lcb, fs, mp);
                if(sts == NU_SUCCESS)
                {
                    /* Now mount the volume */
                    sts = NU_Mount_File_System(fs, mp, p_lcb->fdrv_name, &default_codepage);
                }
            }
        }
    }
    
    /* Trace log */
    T_LOG_DEV_INFO(p_lcb->fdrv_name, fs, mp, auto_fmt, p_lcb->fdrv_start, p_lcb->fdrv_end, sts);

    return(sts);
}

/*****************************************************************************
*
*   FUNCTION
*
*       finit_auto_format_device
*
*   DESCRIPTION
*       Given a pointer to a device list, attempt to format and
*       mount the device automatically.
*
*   INPUTS
*       p_lcb               Logical control block pointer
*       fs                  File system type to mount (and possibly format)
*       mp                  Mount point to associate with device
*
*   OUTPUTS
*       NU_SUCCESS          Request completed successfully
*       NUF_BADPARM         Invalid device list
*
*****************************************************************************/
STATIC STATUS finit_auto_format_device(FDRV_LOG_CB_S *p_lcb, CHAR *fs, CHAR *mp)
{
    STATUS sts;
#ifdef CFG_NU_OS_STOR_FILE_FS_FAT_ENABLE
    FMTPARMS fmt, *pfmt;
#else
    INT fmt, *pfmt;
#endif
    pfmt = &fmt;

    memset(pfmt, 0, sizeof(fmt));

    if((p_lcb != NU_NULL)&&(fs != NU_NULL)&&(mp != NU_NULL))
    {
        /* Get the format parameters */
        sts = NU_Get_Format_Info(fs, (CHAR*)p_lcb->fdrv_name, (VOID**)&pfmt);

        if (sts == NU_SUCCESS)
        {
            /* Format the disk */
            sts = NU_Format(fs, (CHAR*)p_lcb->fdrv_name, (VOID**)&pfmt);
        }
    }
    else
    {
        sts = NUF_BADPARM;
    }

    return(sts);
}

/************************************************************************
* FUNCTION
*
*       finit_find_unused_mount_point
*
* DESCRIPTION
*       Find an available mount point starting the search with the value
*       the mp parameter. If none are available, then set "mp" to NU_NULL.
*
* INPUTS
*       mp                  Mount point to begin searching from
*
* OUTPUTS
*       NUF_MOUNT_TABLE_FULL No mount points are available
*       mp                  Mount point
*
*************************************************************************/
STATIC STATUS finit_find_unused_mount_point(CHAR *mp)
{
    STATUS sts, temp_sts;
    MNT_LIST_S *pmount_list_head, *pmount_list;
    INT start, i = 0;
    BOOLEAN wrap = NU_FALSE, mounted;

    if((mp != NU_NULL) && ( ((*mp) >= 'A') && ((*mp) <= 'Z') ) && (mp[1] == '\0'))
    {
        /* Check if the logical device is already mounted */
        sts = NU_List_Mount(&pmount_list_head);
        if (sts == NU_SUCCESS)
        {
            /* Start searching based on the passed in value. Stop when a free mp is found,
               or the search has wrapped and incremented back to the start */
            for(start = i = *mp - 'A'; ((wrap == NU_FALSE)||(i != start)); i++)
            {
                mp[0] = 'A' + i;
                pmount_list = pmount_list_head;

                mounted = NU_FALSE;

                while(pmount_list != NU_NULL)
                {
                    if(strcmp(pmount_list->mnt_name, mp))
                    {
                        /* Mount point does not match, try next */
                        pmount_list = pmount_list->next;
                    }
                    else
                    {
                        mounted = NU_TRUE;
                        break;
                    }
                }
                if(mounted == NU_FALSE)
                {
                    /* Found a mount point that is available */
                    break;
                }

                if(i == CFG_NU_OS_STOR_FILE_VFS_MAX_MOUNT_POINTS-1)
                {
                    /* The next iteration will start over at "A" */
                    if(wrap == NU_FALSE)
                    {
                        wrap = NU_TRUE;
                        i = -1;
                    }
                }
            }

            if (mounted == NU_TRUE)
            {
                sts = NUF_MOUNT_TABLE_FULL;

                /* Couldn't find a free mount point */
                mp = NU_NULL;
            }

            temp_sts = NU_Free_List((VOID**)&pmount_list_head);
            if(sts == NU_SUCCESS)
            {
                sts = temp_sts;
            }
        }
    }
    else
    {
        sts = NUF_BADPARM;
    }

    return(sts);
}

/************************************************************************
* FUNCTION
*
*       finit_remove_logical_devices
*
* DESCRIPTION
*       Remove logical devices associated with a physical device
*
* INPUTS
*       dev_name                Physical device name
*
* OUTPUTS
*       NU_SUCCESS              Operation is successful
*
*************************************************************************/
STATUS finit_remove_logical_devices(CHAR* dev_name)
{
    STATUS sts = NU_SUCCESS;
    STATUS temp_sts;
    BOOLEAN tmp;
    DEV_LIST_S *pdev_list_head, *pdev_list;
    CHAR tmp_name[FILE_MAX_DEVICE_NAME] = {0};

    if(dev_name == NU_NULL)
    {
        sts = NUF_BADPARM;
    }

    if(sts == NU_SUCCESS)
    {
        /* Get a list of all devices registered with VFS */
        sts = NU_List_Device(&pdev_list_head);
    }

    if (sts == NU_SUCCESS)
    {
        /* For each device on the list */
        pdev_list = pdev_list_head;
        while(pdev_list != NU_NULL)
        {
            /* Clear the array */
            memset(tmp_name, 0, FILE_MAX_DEVICE_NAME);

            /* Copy the name to avoid warnings */
            strcpy(tmp_name, pdev_list->dev_name);

            /* if its a logical device and the name matches */
            tmp = NUF_Strncmp(dev_name, tmp_name, FILE_MAX_DEVICE_NAME-3);
            if((!tmp) && (tmp_name[3] != NU_NULL))
            {
                /* Abort the disk and unmount the volume */
                sts = finit_abort_volume(tmp_name);
                if(sts != NU_SUCCESS)
                {
/* TODO Log the error aborting the volume */
                }

                /* Remove the device */
                temp_sts = finit_remove_device(tmp_name);
                if(sts == NU_SUCCESS)
                {
                    sts = temp_sts;
                    if(sts != NU_SUCCESS)
                    {
/* TODO Log the error removing the device */
                    }
                }
            }

            /* Iterate to the next device */
            pdev_list = pdev_list->next;
        } /* while */

        /* Free the device list */
        temp_sts = NU_Free_List((VOID**)&pdev_list_head);
        if(sts == NU_SUCCESS)
        {
            sts = temp_sts;
        }
    }

    return(sts);
}

/*****************************************************************************
*
*   FUNCTION
*
*       finit_remove_device
*
*   DESCRIPTION
*       Removes the device from VFS and cleans up resources.
*
*   INPUTS
*       dev_id              DM ID of device to remove
*
*   OUTPUTS
*       NU_SUCCESS                  Request completed successfully
*
*****************************************************************************/
STATIC STATUS finit_unregister_device(DV_DEV_ID dev_id)
{
    STATUS sts;
    CHAR dev_name[FILE_MAX_DEVICE_NAME] = {0};

    /* Use the device id to look up the physical disk that was removed */
    sts = finit_get_device_name_by_id(dev_name, dev_id);
    if(sts == NU_SUCCESS)
    {
        /* Remove the logical devices associated with the physical device */
        sts = finit_remove_logical_devices(dev_name);
        if(sts != NU_SUCCESS)
        {
            /* TODO Log an error */
        }
        /* Trace log */
        T_LOG_DEV_REMOVAL(dev_name, sts);

        /* Remove the physical device */
        sts = finit_remove_device(dev_name);
        if(sts != NU_SUCCESS)
        {
            /* TODO Log an error */
        }
        /* Trace log */
        T_PHY_DEV_REMOVAL(dev_name, sts);
    }

    return(sts);
}

/************************************************************************
* FUNCTION
*
*       finit_get_device_name_by_id
*
* DESCRIPTION
*       Look up the device name based on a Device Manager device ID
*
* INPUTS
*       dev_name            Pointer to physical device name
*       dev_id              DM ID of device
*
* OUTPUTS
*       NU_SUCCESS          Operation is successful
*       NUF_BADPARM         Invalid dev_name pointer
*       NUF_INVALID_DEVNAME Device could not be found
*
*************************************************************************/
STATIC STATUS finit_get_device_name_by_id(CHAR *dev_name, DV_DEV_ID dev_id)
{
    STATUS sts = NU_SUCCESS;
    STATUS temp_sts;
    DEV_LIST_S *pdev_list_head, *pdev_list;
    UINT16 vfs_dh;
    FDRV_PHY_CB_S *temp_cb;
    BOOLEAN found = NU_FALSE;
    CHAR tmp_name[FILE_MAX_DEVICE_NAME] = {0};

    if(dev_name == NU_NULL)
    {
        sts = NUF_BADPARM;
    }

    if(sts == NU_SUCCESS)
    {
        /* Get a list of all devices registered with VFS */
        sts = NU_List_Device(&pdev_list_head);
    }

    if (sts == NU_SUCCESS)
    {
        /* For each device on the list */
        pdev_list = pdev_list_head;
        while(pdev_list != NU_NULL)
        {
           /* Clear the array */
            memset(tmp_name, 0, FILE_MAX_DEVICE_NAME);

            /* Copy the name to avoid warnings */
            strcpy(tmp_name, pdev_list->dev_name);

            /* if its a physical device */
            if(tmp_name[3] == NU_NULL)
            {
                /* Get the VFS dh */
                sts = fsdh_devname_to_dh(tmp_name, &vfs_dh);
                if(sts == NU_SUCCESS)
                {
                    /* Retrieve the control block from the dh specific */
                    sts = fsdh_get_dh_specific(vfs_dh, (VOID*)&temp_cb, dev_name);
                    if((sts == NU_SUCCESS) && (temp_cb->fdrv_dm_cb.dev_id == dev_id))
                    {
                        /* Found it */
                        found = NU_TRUE;
                        break;
                    }
                }
            }

            /* Iterate to the next device */
            pdev_list = pdev_list->next;
        } /* while */

        if((sts == NU_SUCCESS)&&(found != NU_TRUE))
        {
            /* Indicate that the device was not found and the dev_name is not valid */
            sts = NUF_INVALID_DEVNAME;
        }

        /* Free the device list */
        temp_sts = NU_Free_List((VOID**)&pdev_list_head);
        if(sts == NU_SUCCESS)
        {
            sts = temp_sts;
        }
    }

    return(sts);
}

/************************************************************************
* FUNCTION
*
*       finit_abort_volume
*
* DESCRIPTION
*       Call NU_Disk_Abort on the logical device.
*
* INPUTS
*       dev_name            logical device name
*
* OUTPUTS
*       NU_SUCCESS          Operation is successful
*       NUF_BADPARM         Invalid dev_name pointer
*       NUF_INVALID_DEVNAME Device could not be found
*
*************************************************************************/
STATIC STATUS finit_abort_volume(CHAR *dev_name)
{
    STATUS sts;
    STATUS temp_sts;
    UINT16 dh;
    MTE_S* mte;
    CHAR fqpath[] = "0:\0";

    /* Disk abort requires that the task is a file user */
    sts = NU_Become_File_User();
    if(sts == NU_SUCCESS)
    {
        if(dev_name == NU_NULL)
        {
            sts = NUF_BADPARM;
        }
    }

    if(sts == NU_SUCCESS)
    {
        /* Get the dh */
        sts = fsdh_devname_to_dh(dev_name, &dh);
        if(sts == NU_SUCCESS)
        {
            /* Get the mount table entry */
            mte = fsl_mte_from_dh(dh);
            if(mte != NU_NULL)
            {
                /* Remove the device */
                fqpath[0] = *mte->mte_mount_name;
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)
                /* Ignore the return value as the device is gone */
                (VOID) NU_FILE_Cache_Destroy(fqpath);
#endif
                sts = NU_Disk_Abort(fqpath);
                if(sts == NU_SUCCESS)
                {
                    /* Unmount requires a mount point */
                    fqpath[1] = '\0';

                    /* Free mount point resources */
                    sts = NU_Unmount_File_System(fqpath);
                }
            }
            else
            {
                sts = NUF_INVALID_DEVNAME;
            }
        }
    }

    /* Release the user resource */
    temp_sts = NU_Release_File_User();
    if(sts == NU_SUCCESS)
    {
        sts = temp_sts;
    }

    return(sts);
}

/*****************************************************************************
*
*   FUNCTION
*
*       finit_clear_count
*
*   DESCRIPTION
*       Look up the device structure and clear the count.
*
*   INPUTS
*       dev_name            device name
*
*   OUTPUTS
*       NU_SUCCESS          Request completed successfully
*       NUF_INTERNAL        The device has invalid data associated with it
*
*****************************************************************************/
STATIC STATUS finit_clear_count(CHAR* dev_name)
{
    STATUS sts;
    FDEV_S  *pdev;

    /* Use the device name to get the FDEV_S */
    sts = fs_dev_devname_to_fdev(dev_name, (VOID*)&pdev);
    if(sts == NU_SUCCESS)
    {
        if(pdev != NU_NULL)
        {
            /* Clear the device open count */
            pdev->fdev_cnt = 0;
        }
        else
        {
            sts = NUF_INTERNAL;
        }
    }

    return(sts);
}

/*****************************************************************************
*
*   FUNCTION
*
*       finit_remove_device
*
*   DESCRIPTION
*       Removes the device from VFS.
*
*   INPUTS
*       dev_name            device name
*
*   OUTPUTS
*       NU_SUCCESS          Request completed successfully
*
*****************************************************************************/
STATIC STATUS finit_remove_device(CHAR *dev_name)
{
    STATUS sts;

    /* Clear the open count */
    sts = finit_clear_count(dev_name);
    if(sts == NU_SUCCESS)
    {
        /* Remove the device */
        sts = NU_Remove_File_Device(dev_name);
    }

    return(sts);
}
