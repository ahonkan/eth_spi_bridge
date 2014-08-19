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
*       dvc_w.c
*                                                                       
* COMPONENT                                                             
*                                                                       
*       VFS Device Manager Wrapper Layer
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       The Nucleus Device Manager interface requirements are complied 
*       with through this layer.                                 
*                                                                       
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       vfs_dvc_phys_ops            The physical operations structure 
*                                    passed into NU_Create_File_Device 
*       vfs_dvc_log_ops             The logical operations structure 
*                                    passed into NU_Create_File_Device 
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       vfs_dvc_close_wrapper
*       vfs_dvc_io_wrapper
*       vfs_dvc_ioctl_wrapper
*       vfs_dvc_dskchk_wrapper
*       vfs_dvc_log_open_wrapper
*       vfs_dvc_log_close_wrapper 
*       vfs_dvc_log_io_wrapper 
*       vfs_dvc_log_ioctl_wrapper 
*       vfs_dvc_log_dskchk_wrapper
*
* DEPENDENCIES
*  
*       "file/vfs/inc/pcdisk.h"
*                                                                       
*************************************************************************/

#include "storage/pcdisk.h"


FDEV_OP_S vfs_dvc_phys_ops = 
{
    vfs_dvc_open,  /* Opens a device by name */
    vfs_dvc_close_wrapper, /* Closes a device */
    vfs_dvc_io_wrapper,    /* Sends a I/O command to a device */
    vfs_dvc_ioctl_wrapper, /* Sends an IOCTL command to a device */
    vfs_dvc_dskchk_wrapper
};

FDEV_OP_S vfs_dvc_log_ops = 
{
    vfs_dvc_log_open_wrapper,  /* Open is not required */
    vfs_dvc_log_close_wrapper, /* Closes a device */
    vfs_dvc_log_io_wrapper,    /* Sends a I/O command to a device */
    vfs_dvc_log_ioctl_wrapper, /* Sends an IOCTL command to a device */
    vfs_dvc_log_dskchk_wrapper /* Gets the status of the device */
};

UINT32 *VFS_DVC_UNUSED_PARAMETER;

/************************************************************************
* FUNCTION
*
*       vfs_dvc_open   
*
* DESCRIPTION
*
*       Allocate a physical control block and associate it with 
*       the disk handle.
*
* INPUTS
*
*       dh                      Physical device handle
*       devname                 Name of the device
*       args                    Pointer to a disk info structure
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NU_NO_MEMORY            Allocate failed 
*
*************************************************************************/
STATUS vfs_dvc_open(UINT16 dh, CHAR *devname, VOID *args)
{
    STATUS sts;
    FDRV_PHY_CB_S *pphy_cb;

    /* Handle the unused parameters */
    VFS_DVC_UNUSED_PARAMETER = (UINT32*)args;

    /* Allocate a physical control block */
    pphy_cb = NUF_Alloc(sizeof(FDRV_PHY_CB_S));
    if(pphy_cb != NU_NULL)
    {
        memset(pphy_cb, 0, sizeof(FDRV_PHY_CB_S));

        /* Store the f_dev_handle in the dh specific */
        sts = fsdh_set_dh_specific(dh, pphy_cb, NU_NULL);
        
        if(sts != NU_SUCCESS)
        {
            (VOID)NU_Deallocate_Memory(pphy_cb);
        }
    }
    else
    {
        sts = NU_NO_MEMORY;
    }
    
    return(sts);
}
    

/************************************************************************
* FUNCTION
*
*       vfs_dvc_close_wrapper   
*
* DESCRIPTION
*
*       Call the Device Manager Close
*
* INPUTS
*
*       dh                      Physical device handle
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NUF_INTERNAL            Device control block was invalid
*
*************************************************************************/
STATUS vfs_dvc_close_wrapper(UINT16 dh)
{
    STATUS sts;
    FDRV_PHY_CB_S *pphy_cb;
    STATUS temp_sts;
    DV_DEV_LABEL dev_label = {STORAGE_LABEL};
    DV_DEV_ID dev_id_list[DV_DISCOVERY_TASK_MAX_ID_CNT];
    extern INT DVD_Max_Dev_Id_Cnt;
    INT id_cnt = DVD_Max_Dev_Id_Cnt;
    
    /* Get the DM device handle */ 
    sts = fsdh_get_dh_specific(dh, (VOID**)&pphy_cb, NU_NULL);
    if (sts == NU_SUCCESS)
    {
        if(pphy_cb != NU_NULL)
        {

            /* Get a list of Storage devs */
            sts = DVC_Dev_ID_Get(&dev_label, 1, dev_id_list, &id_cnt);
            if (sts == NU_SUCCESS)
            {
                /* Check if dev is in list */
                for (;id_cnt > 0; id_cnt--)
                {
                    if (dev_id_list[id_cnt-1] == pphy_cb->fdrv_dm_cb.dev_id)
                    {
                        /* If the device is found, then the call to close was
                         * not from the DM unregister callback.
                         * Call the DM close function */
                        sts = DVC_Dev_Close(pphy_cb->fdrv_dm_cb.dh);
                        break;
                    }
                }
            }
            
            /* Deallocate the control block */
            temp_sts = NU_Deallocate_Memory((VOID*)pphy_cb);
            if(sts == NU_SUCCESS)
            {
                sts = temp_sts;
            }
        
            /* Clear the specific data */ 
            temp_sts = fsdh_set_dh_specific(dh, NU_NULL, NU_NULL);
            if(sts == NU_SUCCESS)
            {
                sts = temp_sts;
            }
        }
        else
        {
            sts = NUF_INTERNAL;
        }           
    }
    
    return(sts);
}   


/************************************************************************
* FUNCTION
*
*       vfs_dvc_io_wrapper   
*
* DESCRIPTION
*
*       Call the Device Manager Read or Write
*
* INPUTS
*
*       dh                      Physical device handle
*       sector                  Physical sector address (offest from zero)
*       buffer                  Address in memory to perform I/O
*       count                   Number of physical sectors to transfer
*       reading                 NU_TRUE = read; NU_FALSE = write 
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NUF_IO_ERROR            The byte count value returned does not
*                               match the requested I/O count.
*       NUF_INTERNAL            Device control block was invalid
*
*************************************************************************/
STATUS vfs_dvc_io_wrapper(UINT16 dh, UINT32 sector, VOID *buffer, UINT16 count, INT reading)
{
    STATUS sts;
    FDRV_PHY_CB_S *pphy_cb;
    
    /* Get the DM device handle */ 
    sts = fsdh_get_dh_specific(dh, (VOID**)&pphy_cb, NU_NULL);
    if(sts == NU_SUCCESS)
    {
        if((pphy_cb != NU_NULL) && (pphy_cb->fdrv_bytes_p_sec != 0))
        {
            if(reading == NU_TRUE)
            {
                /* Call the read function */
                sts = DVC_Dev_Read(pphy_cb->fdrv_dm_cb.dh, buffer, (UINT32)count * pphy_cb->fdrv_bytes_p_sec, (OFFSET_T) sector * pphy_cb->fdrv_bytes_p_sec, NU_NULL);
            }
            else
            {
                /* Call the write function */
                sts = DVC_Dev_Write(pphy_cb->fdrv_dm_cb.dh, buffer, (UINT32)count * pphy_cb->fdrv_bytes_p_sec, (OFFSET_T) sector * pphy_cb->fdrv_bytes_p_sec, NU_NULL);
            }
        }
        else
        {
            sts = NUF_INTERNAL;
        }
    }
    
    return(sts);
}


/************************************************************************
* FUNCTION
*
*       vfs_dvc_ioctl_wrapper   
*
* DESCRIPTION
*
*       Call the Device Manager IOCTL
*
* INPUTS
*
*       dh                      Physical device handle
*       command                 The IOCTL command ID
*       buffer                  Address in memory to transfer data
*       ioctl_data_len          The size of the data being transferred
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NUF_INTERNAL            Device control block was invalid
*
*************************************************************************/
STATUS vfs_dvc_ioctl_wrapper(UINT16 dh, UINT16 command, VOID *buffer, INT ioctl_data_len)
{
    STATUS sts;
    FDRV_PHY_CB_S *pphy_cb;
    DV_DEV_LABEL device_type_label = {STORAGE_LABEL};
    DV_IOCTL0_STRUCT ioctl0;

    /* Get the DM device handle */ 
    sts = fsdh_get_dh_specific(dh, (VOID*)&pphy_cb, NU_NULL);
    if (sts == NU_SUCCESS)
    {
        if(pphy_cb != NU_NULL)
        {
            ioctl0.label = device_type_label;
            sts = DVC_Dev_Ioctl(pphy_cb->fdrv_dm_cb.dh, DV_IOCTL0, &ioctl0, sizeof(ioctl0));
            if(sts == NU_SUCCESS)
            {
                /* Call the DM ioctl function */
                sts = DVC_Dev_Ioctl(pphy_cb->fdrv_dm_cb.dh, ioctl0.base + command, buffer, ioctl_data_len);
            }
        }
        else
        {
            sts = NUF_INTERNAL;
        }
    }
    
    return(sts);
}


/************************************************************************
* FUNCTION
*
*       vfs_dvc_dskchk_wrapper   
*
* DESCRIPTION
*
*       Call the IOCTL with the disk check command
*
* INPUTS
*
*       dh                      Physical device handle
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*
*************************************************************************/
STATUS vfs_dvc_dskchk_wrapper(UINT16 dh)
{
    STATUS sts;
    STATUS chkdsk_sts;
    
    /* Call the DM ioctl function */
    sts = vfs_dvc_ioctl_wrapper(dh, FDEV_GET_DISK_STATUS, &chkdsk_sts, sizeof(STATUS));
    if (sts == NU_SUCCESS)
    {
        sts = chkdsk_sts;
    }
    
    return(sts);
}


/************************************************************************
* FUNCTION
*
*       vfs_dvc_log_open_wrapper   
*
* DESCRIPTION
*
*       Allocate a logical control block and associate it with the 
*       disk handle. Increment physical device open count.  
*
* INPUTS
*
*       dh                      Logical device handle
*       devname                 Name of the device
*       args                    Unused
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NU_NO_MEMORY            Allocate failed 
*       NUF_INTERNAL            Device control block was invalid
*
*************************************************************************/
STATUS vfs_dvc_log_open_wrapper(UINT16 dh, CHAR *devname, VOID *args)
{
    STATUS sts;
    CHAR phys_name[FPART_MAX_PHYS_NAME] = {0};
    FDEV_S  *phys_dev;
    FDRV_LOG_CB_S *p_lcb;
    
    /* Handle the unused parameters */
    VFS_DVC_UNUSED_PARAMETER = (UINT32*)&dh;
    VFS_DVC_UNUSED_PARAMETER = (UINT32*)args;
    
    /* Copy the physical name */
    NUF_Copybuff(phys_name, devname, FPART_MAX_PHYS_NAME);
    phys_name[FPART_MAX_PHYS_NAME-1] = '\0';
    
    /* Use the physical name to get the FDEV_S */
    sts = fs_dev_devname_to_fdev(phys_name, (VOID*)&phys_dev);
    if(sts == NU_SUCCESS)
    {
        if(phys_dev != NU_NULL)
        { 
            /* Allocate a logical control block */
            p_lcb = NUF_Alloc(sizeof(FDRV_LOG_CB_S));
            
            if(p_lcb != NU_NULL)
            {
                memset(p_lcb, 0, sizeof(FDRV_LOG_CB_S));

                /* Store the logical control block in the dh specific */
                sts = fsdh_set_dh_specific(dh, p_lcb, NU_NULL);
                if(sts != NU_SUCCESS)
                {
                    (VOID)NU_Deallocate_Memory(p_lcb);
                }
                else
                {
                    /* Increment the physical device open count */
                    phys_dev->fdev_cnt++;
                }
            }
            else
            {
                sts = NU_NO_MEMORY;
            }
        }
        else
        {
            sts = NUF_INTERNAL;
        }
    }
    
    return(sts);
}   


/************************************************************************
* FUNCTION
*
*       vfs_dvc_log_close_wrapper   
*
* DESCRIPTION
*
*       Stub for VFS device interface compliance 
*
* INPUTS
*
*       dh                      Logical device handle
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NUF_INTERNAL            Device control block was invalid
*
*************************************************************************/
STATUS vfs_dvc_log_close_wrapper(UINT16 dh)
{
    STATUS sts;
    STATUS temp_sts;
    FDRV_LOG_CB_S *p_lcb;
    CHAR phys_name[FPART_MAX_PHYS_NAME] = {0};
    FDEV_S  *phys_dev;
    
    /* Get the logical control block */ 
    sts = fsdh_get_dh_specific(dh, (VOID**)&p_lcb, NU_NULL);    
    if (sts == NU_SUCCESS)
    {
        if(p_lcb != NU_NULL)
        {
            /* Copy the physical name */
            NUF_Copybuff(phys_name, p_lcb->fdrv_name, FPART_MAX_PHYS_NAME);
            phys_name[FPART_MAX_PHYS_NAME-1] = '\0';
            
            /* Use the physical name to get the FDEV_S */
            sts = fs_dev_devname_to_fdev(phys_name, &phys_dev);
            if(sts == NU_SUCCESS)
            {
                if(phys_dev != NU_NULL)
                {
                    /* Decrement the physical device open count */
                    phys_dev->fdev_cnt--;
                                        
                    /* Free the memory for the logical control block */
                    sts = NU_Deallocate_Memory((VOID*)p_lcb);
                    
                    /* Clear the specific data */ 
                    temp_sts = fsdh_set_dh_specific(dh, NU_NULL, NU_NULL);
                    if(sts == NU_SUCCESS)
                    {
                        sts = temp_sts;
                    }                   
                }
                else
                {
                    sts = NUF_INTERNAL;
                }
            }
        }
        else
        {
            sts = NUF_INTERNAL;
        }
    }
    
    return(sts);
}   


/************************************************************************
* FUNCTION
*
*       vfs_dvc_log_io_wrapper   
*
* DESCRIPTION
*
*       Call the physical device I/O routine after offsetting the sector 
*       with the address of the partition start. 
*
* INPUTS
*
*       dh                      Logical device handle
*       sector                  Logical sector address (offest from partition start)
*       buffer                  Address in memory to perform I/O
*       count                   Number of physical sectors to transfer
*       reading                 TRUE = read; FALSE = write 
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NUF_BADPARM             The specified sector or count would 
*                               extend beyond the boundaries of the 
*                               partition
*       NUF_INTERNAL            Device control block was invalid
*
*************************************************************************/
STATUS vfs_dvc_log_io_wrapper(UINT16 dh, UINT32 sector, VOID *buffer, UINT16 count, INT reading)
{
    STATUS sts;
    FDRV_LOG_CB_S *p_lcb;
    
    /* Get the logical control block */ 
    sts = fsdh_get_dh_specific(dh, (VOID**)&p_lcb, NU_NULL);    
    if (sts == NU_SUCCESS)
    {
        if(p_lcb != NU_NULL)
        {
            /* Validate the sector and count parameters */
            if ((count != 0) &&
                ((p_lcb->fdrv_start + sector + count -1) <= p_lcb->fdrv_end))
            {
                /* Add the offset to the logical sector and call the physical device io */
                sts = fs_dev_io_proc(p_lcb->fdrv_vfs_dh, p_lcb->fdrv_start + sector, buffer, count, reading);
            }
            else
            {
                sts = NUF_BADPARM;
            }
        }
        else
        {
            sts = NUF_INTERNAL;
        }
    }
    
    return(sts);
}


/************************************************************************
* FUNCTION
*
*       vfs_dvc_log_ioctl_wrapper   
*
* DESCRIPTION
*
*       Call the physical device IOCTL routine
*
* INPUTS
*
*       dh                      Logical device handle
*       command                 The IOCTL command ID
*       buffer                  Address in memory to transfer data
*       ioctl_data_len          The size of the data being transferred
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NUF_INTERNAL            Device control block was invalid
*
*************************************************************************/
STATUS vfs_dvc_log_ioctl_wrapper(UINT16 dh, UINT16 command, VOID *buffer, INT ioctl_data_len)
{
    STATUS sts;
    FDRV_LOG_CB_S *p_lcb;
    
    /* Get the logical control block */ 
    sts = fsdh_get_dh_specific(dh, (VOID**)&p_lcb, NU_NULL);    
    if (sts == NU_SUCCESS)
    {
        if(p_lcb != NU_NULL)
        {
            /* Call the physical ioctl function */
            sts = fs_dev_ioctl_proc(p_lcb->fdrv_vfs_dh, command, buffer, ioctl_data_len);
        }
        else
        {
            sts = NUF_INTERNAL;
        }
    }
    
    return(sts);
}
 
 
/************************************************************************
* FUNCTION
*
*       vfs_dvc_log_dskchk_wrapper   
*
* DESCRIPTION
*
*       Call the physical device disk check routine
*
* INPUTS
*
*       dh                      Physical device handle
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NUF_INTERNAL            Device control block was invalid
*
*************************************************************************/
STATUS vfs_dvc_log_dskchk_wrapper(UINT16 dh)
{
    STATUS sts;
    FDRV_LOG_CB_S *p_lcb;
    
    /* Get the logical control block */ 
    sts = fsdh_get_dh_specific(dh, (VOID**)&p_lcb, NU_NULL);
    if (sts == NU_SUCCESS)
    {
        if(p_lcb != NU_NULL)
        {
            /* Call the physical disk check function */
            sts = fs_dev_dskchk_proc(p_lcb->fdrv_vfs_dh);
        }
        else
        {
            sts = NUF_INTERNAL;
        }
    }
    
    return(sts);
}
