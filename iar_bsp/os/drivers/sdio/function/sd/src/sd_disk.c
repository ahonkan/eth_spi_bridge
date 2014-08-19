/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       sd_disk.c
*
* DESCRIPTION
*
*       Nucleus SDIO FILE Function Driver - Application layer
*
* DATA STRUCTURES
*
*       sd_phy_cb                   SD Physical Disk Control.
*
* FUNCTIONS
*
*       SD_Register                 Register the SD device with DM
*       SD_Unregister               Unregister the SD device from DM
*       SD_DVM_Open                 Open the SD device
*       SD_DVM_Close                Close the SD device
*       SD_DVM_Read                 Read a block from the SD device
*       SD_DVM_Write                Write a block to the SD device
*       SD_DCM_Ioctl                Send I/O commands to the SD device
*
* DEPENDENCIES
*
*       nucleus.h                   System definitions
*       sd_disk.h                   Disk definitions
*       sd_imp.h
*       ctsystem.h
*       sdio_busdriver.h
*       _sdio_defs.h
*       sdio_lib.h
*
*************************************************************************/
#include "nucleus.h"                   /* System definitions */
#include "drivers/sd_disk.h"   /* Disk definitions  */
#include "drivers/sd_imp.h"

#include "connectivity/ctsystem.h"
#include "connectivity/sdio_busdriver.h"
#include "connectivity/_sdio_defs.h"
#include "connectivity/sdio_lib.h"

/* Open Modes */
#define STORAGE_OPEN_MODE           0x1
#define POWER_OPEN_MODE             0x2
#define SD_OPEN_MODE                0x4

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE 	
#define MAX_NUM_OF_MODES            3
#else
#define MAX_NUM_OF_MODES            2
#endif

/* Power Event Mask */
#define SD_POWER_EVENT_MASK         0x3

/* Power Base */
#define SD_POWER_BASE               (STORAGE_CMD_BASE + TOTAL_STORAGE_IOCTLS)

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE 	
/* SD Media Base */
#define SD_MEDIA_BASE               (SD_POWER_BASE + POWER_IOCTL_TOTAL)
#else
/* SD Media Base */
#define SD_MEDIA_BASE               (STORAGE_CMD_BASE + TOTAL_STORAGE_IOCTLS)
#endif

/* Power States */
#define SD_OFF                      0
#define SD_ON                       1

/* SD total power states */
#define SD_TOTAL_STATE_COUNT        2

/* Minimum DVFS OP for SD to perform correctly */
#define SD_MIN_DVFS_OP              1

/* SD True/False values */
#define SD_TRUE                     1
#define SD_FALSE                    0

/* SD disk information defines */
#define SD_FPART_HEADS              255
#define SD_FPART_SECTORS            63

typedef struct  _sd_session_handle_struct
{
    UINT32             open_modes;
    SD_INSTANCE_HANDLE *inst_info;

} SD_SESSION_HANDLE;

STATUS      SD_DVM_Open (VOID *instance_handle,
                         DV_DEV_LABEL labels_list[],
                         INT labels_cnt,
                         VOID* *session_handle);
STATUS      SD_DVM_Close(VOID *handle_ptr);
STATUS      SD_DVM_Read (VOID *session_handle,
                         VOID *buffer,
                         UINT32 numbyte,
                         OFFSET_T byte_offset,
                         UINT32 *bytes_read_ptr);
STATUS      SD_DVM_Write (VOID *session_handle,
                          const VOID *buffer,
                          UINT32 numbyte,
                          OFFSET_T byte_offset,
                          UINT32 *bytes_written_ptr);
STATUS      SD_DVM_Ioctl(VOID *session_handle,
                         INT cmd,
                         VOID *data,
                         INT length);

extern  NU_MEMORY_POOL  System_Memory;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE 
static STATUS SD_Set_State  (VOID *inst_handle, PM_STATE_ID *state);
#endif
/* Better to allocate static memory for Instance and Session handles. DEV_INFO is
   contained in Instance handle and session handle contains a pointer to instance struct. */

/*********************************/
/* Global Static Variables       */
/*********************************/

/*************************************************************************
*
*   FUNCTION
*
*       SD_Register
*
*   DESCRIPTION
*
*       This function registers the SD hardware
*
*   INPUTS
*
*       SD_PHYS_DISK_CTRL *pdch             - SD Control Block

*   OUTPUTS
*
*       STATUS            status            - NU_SUCCESS or error code
*
*************************************************************************/
STATUS SD_Register (SD_PHYS_DISK_CTRL *pdcb)
{
    STATUS             status = NU_SUCCESS;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE 	
    CHAR               reg_path[REG_MAX_KEY_LENGTH];
    PM_STATE_ID        def_pwr_state;
    DV_DEV_LABEL       sd_labels[] = {{SD_LABEL}, {STORAGE_LABEL}, {POWER_CLASS_LABEL}};
#else 	
    DV_DEV_LABEL       sd_labels[] = {{SD_LABEL}, {STORAGE_LABEL}};
#endif
    INT                sd_label_cnt = DV_GET_LABEL_COUNT(sd_labels);
    DV_DRV_FUNCTIONS   sd_drv_funcs;
    CHAR               dev_name[FILE_MAX_DEVICE_NAME + 1] = "sd\0\0";
    SD_INSTANCE_HANDLE *inst_handle;
    SD_TGT             *tgt_info_ptr;
    NU_EVENT_GROUP     *read_event;
    NU_EVENT_GROUP     *write_event;
    SD_DEV             *sd_dev;


    /* Allocate memory for the SD instance handle */
    status = NU_Allocate_Memory(&System_Memory, (VOID**)&inst_handle,
                                sizeof(SD_INSTANCE_HANDLE), NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Zero out allocated space */
        (VOID)memset ((VOID*)inst_handle, 0, sizeof (SD_INSTANCE_HANDLE));

        /* Create the full device name */
        dev_name[2] = 'a' + pdcb->p_SDDevice->pHcd->SlotNumber;

        /* Save the device name in the PDCB */
        strncpy(pdcb->name, dev_name, sizeof(pdcb->name));

        /* Clear the open modes variable. */
        inst_handle->pdcb.open_modes = 0;

        /* Clear out the open count */
        pdcb->open_count = 0;

        /* Populate the rest of the instance handle */
        inst_handle->dev_id = DV_INVALID_DEV;

        /* Save the PDCB info in the instance handle */
        memcpy(&(inst_handle->pdcb), pdcb, sizeof(SD_PHYS_DISK_CTRL));

        /* Save a pointer to the instance handle in the device structure. */
        pdcb->p_SDDevice->inst_handle = (VOID *)inst_handle;

        /* Save the config path in the instance handle */
        tgt_info_ptr = (SD_TGT *)pdcb->p_SDFunction->pContext;
        strncpy(inst_handle->config_path, tgt_info_ptr->config_path, sizeof(inst_handle->config_path));

        /****************************************/
        /* CREATE EVENTS THAT THE DEVICE CAN    */
        /* USE TO SUSPEND ON, IN READ AND WRITE */
        /****************************************/

        /* Get a pointer to the dev_info structure */
        sd_dev = &(inst_handle->dev_info);

        /* Allocate memory for the read event control block */
        status = NU_Allocate_Memory(&System_Memory, (VOID**)&read_event,
                                   (UNSIGNED)(sizeof(NU_EVENT_GROUP)), (UNSIGNED)NU_NO_SUSPEND);

        /* If we successfully allocated memory for the event control block */
        if (status == NU_SUCCESS)
        {
            /* Clear the memory we just allocated */
            (VOID)memset((VOID*)read_event, 0, sizeof(NU_EVENT_GROUP));

            /* Create an event that we can use to suspend on */
            status = NU_Create_Event_Group(read_event, "Read");

            if (status == NU_SUCCESS)
            {
                /* Save the read event in the device structure */
                sd_dev->read_event_ptr = read_event;
            }
            else
            {
                /* Deallocate the memory */
                (VOID)NU_Deallocate_Memory(read_event);
            }
        }

        if (status == NU_SUCCESS)
        {
            /* Allocate memory for the write event control block */
            status = NU_Allocate_Memory(&System_Memory, (VOID**)&write_event,
                                       (UNSIGNED)(sizeof(NU_EVENT_GROUP)), (UNSIGNED)NU_NO_SUSPEND);

            /* If we successfully allocated memory for the event control block */
            if (status == NU_SUCCESS)
            {
                /* Clear the memory we just allocated */
                (VOID)memset((VOID*)write_event, 0, sizeof(NU_EVENT_GROUP));

                /* Create an event that we can use to suspend on */
                status = NU_Create_Event_Group(write_event, "Write");

                if (status == NU_SUCCESS)
                {
                    /* Save the write event in the device structure */
                    sd_dev->write_event_ptr = write_event;
                }
                else
                {
                    /* Deallocate the memory */
                    (VOID)NU_Deallocate_Memory(write_event);

                    /* Delete read event */
                    (VOID)NU_Delete_Event_Group(read_event);

                    /* Deallocate the memory */
                    (VOID)NU_Deallocate_Memory(read_event);
                }
            }
            else
            {
                /* Delete read event */
                (VOID)NU_Delete_Event_Group(read_event);

                /* Deallocate the memory */
                (VOID)NU_Deallocate_Memory(read_event);
            }
        }

        if (status == NU_SUCCESS)
        {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE		
            /******************************************/
            /* PLACE DEVICE IN KNOWN, LOW-POWER STATE */
            /******************************************/
            /* Get the default power state from Registry */
            strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
            status = REG_Get_UINT32 (strcat(reg_path,"/tgt_settings/def_pwr_state"),
                                     (UINT32*)&def_pwr_state);

            if (status == NU_SUCCESS)
            {
            
			/* Set the state of the device to the device state passed in from the registry entry */
                status = SD_Set_State (inst_handle, &def_pwr_state);

                if (status == NU_SUCCESS)
                {
#endif
     				/*********************************/
                    /* REGISTER WITH DEVICE MANAGER  */
                    /*********************************/

                    /* Populate function pointers */
                    sd_drv_funcs.drv_open_ptr  = &SD_DVM_Open;
                    sd_drv_funcs.drv_close_ptr = &SD_DVM_Close;
                    sd_drv_funcs.drv_read_ptr  = &SD_DVM_Read;
                    sd_drv_funcs.drv_write_ptr = &SD_DVM_Write;
                    sd_drv_funcs.drv_ioctl_ptr = &SD_DVM_Ioctl;

                    /* Register this device with the Device Manager */
                    status = DVC_Dev_Register(inst_handle, sd_labels,
                                              sd_label_cnt, &sd_drv_funcs,
                                              &inst_handle->dev_id);
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE                
				}
            }
#endif
            if (status != NU_SUCCESS)
            {
                /* Delete write event */
                (VOID)NU_Delete_Event_Group(write_event);

                /* Deallocate the memory */
                (VOID)NU_Deallocate_Memory(write_event);

                /* Delete read event */
                (VOID)NU_Delete_Event_Group(read_event);

                /* Deallocate the memory */
                (VOID)NU_Deallocate_Memory(read_event);
            }
        }

        if (status != NU_SUCCESS)
        {
            status = SD_TGT_SETUP_FAILED;
        }
    }

    return (status);
}
/*************************************************************************
*
*   FUNCTION
*
*       SD_Unregister
*
*   DESCRIPTION
*
*       This function unregisters the SD hardware
*
*   INPUTS
*
*       DV_DEV_ID     dev_id                - Device ID
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS or error code
*
*************************************************************************/
STATUS SD_Unregister (DV_DEV_ID dev_id)
{
    STATUS             status;
    SD_INSTANCE_HANDLE *inst_handle;

    /* Unregister the device */
    status = DVC_Dev_Unregister (dev_id, (VOID*)&inst_handle);

    if (status == NU_SUCCESS)
    {
        /* Delete event that we used to suspend on */
        (VOID)NU_Delete_Event_Group(inst_handle->dev_info.read_event_ptr);

        /* Deallocate the memory */
        (VOID)NU_Deallocate_Memory(inst_handle->dev_info.read_event_ptr);

        /* Detele event that we used to suspend on */
        (VOID)NU_Delete_Event_Group(inst_handle->dev_info.write_event_ptr);

        /* Deallocate the memory */
        (VOID)NU_Deallocate_Memory(inst_handle->dev_info.write_event_ptr);

        /* Deallocate the memory */
        (VOID)NU_Deallocate_Memory((VOID*)inst_handle);
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SD_DVM_Open
*
*   DESCRIPTION
*
*       This function creates a session handle
*
*   INPUTS
*
*       VOID          *instance_handle      - Instance handle of the driver
*       DV_DEV_LABEL  labels_list[]         - Access mode (label) of open
*       INT           labels_cnt            - Number of labels
*       VOID*         *session_handle       - Session handle
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS or error code
*
*************************************************************************/
STATUS SD_DVM_Open (VOID *instance_handle,
                    DV_DEV_LABEL labels_list[],
                    INT labels_cnt,
                    VOID* *session_handle)
{
    STATUS              status = NU_SUCCESS;
    INT                 i;
    INT                 int_level;
    SD_INSTANCE_HANDLE  *inst_handle = (SD_INSTANCE_HANDLE*)instance_handle;
    SD_SESSION_HANDLE   *sess_ptr;
    UINT32              open_mode_requests = 0;
    SD_DEV              *sd_dev = &(((SD_INSTANCE_HANDLE*)instance_handle)->dev_info);
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    DV_DEV_LABEL        open_label_list[MAX_NUM_OF_MODES] = {{STORAGE_LABEL},
                                                             {SD_LABEL},
                                                             {POWER_CLASS_LABEL}};
    INT                 open_mode_list[MAX_NUM_OF_MODES] = {STORAGE_OPEN_MODE,
                                                            SD_OPEN_MODE,
                                                            POWER_OPEN_MODE};
#else
    DV_DEV_LABEL        open_label_list[MAX_NUM_OF_MODES] = {{STORAGE_LABEL},
                                                             {SD_LABEL}};
    INT                 open_mode_list[MAX_NUM_OF_MODES] = {STORAGE_OPEN_MODE,
                                                            SD_OPEN_MODE};
#endif


    /* Clear session handle */
    *session_handle = NU_NULL;

    /* Get open mode requests from labels */
    for (i = 0; i < MAX_NUM_OF_MODES; i++)
    {
        if (DVS_Label_List_Contains (labels_list, labels_cnt, open_label_list[i]) == NU_SUCCESS)
        {
            open_mode_requests |= open_mode_list[i];
        }
    }

    /* If device is already open in storage/SD mode AND if the open request contains
     * storage/SD mode, return an error. */
    if (!((inst_handle->pdcb.open_modes & STORAGE_OPEN_MODE) && (open_mode_requests & STORAGE_OPEN_MODE)) &&
        !((inst_handle->pdcb.open_modes & SD_OPEN_MODE) && (open_mode_requests & SD_OPEN_MODE)))
    {
        /* Allocate memory */
        status = NU_Allocate_Memory(&System_Memory, (VOID**)&sess_ptr,
                                    sizeof(SD_SESSION_HANDLE), NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Zero out allocated space */
            (VOID)memset ((VOID*)sess_ptr, 0, sizeof (SD_SESSION_HANDLE));

            /* Disable interrupts while accessing shared variables */
            int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

            /* Save the instance_handle of the driver in the session ptr */
            sess_ptr->inst_info = instance_handle;

            /* If open mode request is power */
            if (open_mode_requests & POWER_OPEN_MODE)
            {
                sess_ptr->open_modes |= POWER_OPEN_MODE;
            }

#ifdef  CFG_NU_OS_STOR_ENABLE
            /* If the open mode request is Storage */
            if (open_mode_requests & STORAGE_OPEN_MODE)
            {
                sess_ptr->open_modes |= STORAGE_OPEN_MODE;

                /* Increment the open count */
                inst_handle->pdcb.open_count++;

                /* Save this open mode. */
                inst_handle->pdcb.open_modes |= STORAGE_OPEN_MODE;

                /* Set device in use flag to true */
                sd_dev->device_in_use = NU_TRUE;
            }
#endif /* #ifdef  CFG_NU_OS_STOR_ENABLE */

            if (open_mode_requests & SD_OPEN_MODE)
            {
                sess_ptr->open_modes |= SD_OPEN_MODE;

                /* Increment the open count */
                inst_handle->pdcb.open_count++;

                /* Save this open mode. */
                inst_handle->pdcb.open_modes |= SD_OPEN_MODE;

                /* Set device in use flag to true */
                sd_dev->device_in_use = NU_TRUE;
            }

            /* Set the return address of the session handle */
            *session_handle = (VOID*)sess_ptr;

            /* Restore interrupts to previous level */
            (VOID)NU_Local_Control_Interrupts (int_level);

        }
    }
    else
    {
        /* Device is already in USE  */
        status = NUF_IN_USE;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SD_DVM_Close
*
*   DESCRIPTION
*
*       This function deletes the session handle
*
*   INPUTS
*
*       VOID   *session_handle              - Session handle of the device
*
*   OUTPUTS
*
*       STATUS status                       - NU_SUCCESS or error code
*
*************************************************************************/
STATUS SD_DVM_Close(VOID *session_handle)
{
    STATUS             status = NU_SUCCESS;
    SD_SESSION_HANDLE  *sess_handle = (SD_SESSION_HANDLE*)session_handle;
    SD_INSTANCE_HANDLE *inst_handle = (SD_INSTANCE_HANDLE*)(sess_handle->inst_info);
    SD_DEV             *sd_dev = &(inst_handle->dev_info);

#ifdef  CFG_NU_OS_STOR_ENABLE
    /* If the open mode request was Storage */
    if (sess_handle->open_modes & STORAGE_OPEN_MODE)
    {
        /* Decrement the open count */
        inst_handle->pdcb.open_count--;

        /* Clear this open mode. */
        inst_handle->pdcb.open_modes &= ~STORAGE_OPEN_MODE;

        /* If no one is using this device then set device in use flag to false */
        if(inst_handle->pdcb.open_count == 0)
        {
            sd_dev->device_in_use = NU_FALSE;
        }
    }
#endif /* #ifdef  CFG_NU_OS_STOR_ENABLE */

    if (sess_handle->open_modes & SD_OPEN_MODE)
    {
        /* Decrement the open count */
        inst_handle->pdcb.open_count--;

        /* Clear this open mode. */
        inst_handle->pdcb.open_modes &= ~SD_OPEN_MODE;

        /* If no one is using this device then set device in use flag to false */
        if(inst_handle->pdcb.open_count == 0)
        {
            sd_dev->device_in_use = NU_FALSE;
        }
    }

    /* Deallocate the memory */
    (VOID)NU_Deallocate_Memory((VOID*)sess_handle);

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SD_DVM_Read
*
*   DESCRIPTION
*
*       This function reads from the SD hardware
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Read buffer
*       INT          numbyte                - Number of bytes to read
*       OFFSET_T     byte_offset            - byte offset from zero to start read
*       INT          *bytes_read_ptr        - Pointer to return number of bytes read
*
*   OUTPUTS
*
*       STATUS       sts                    - NU_SUCCESS or error code
*
*************************************************************************/
STATUS SD_DVM_Read (VOID *session_handle, VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_read_ptr)
{
    STATUS             sts = NU_SUCCESS;
    SD_SESSION_HANDLE  *sess_handle = (SD_SESSION_HANDLE*)session_handle;
    SD_INSTANCE_HANDLE *inst_handle = (SD_INSTANCE_HANDLE*)(sess_handle->inst_info);
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE 	
    SD_DEV             *sd_dev = &(inst_handle->dev_info);
    UNSIGNED           retrieved_events;
#endif

    /* Check the buffer pointer */
    if ((buffer == NU_NULL)  || (bytes_read_ptr == NU_NULL))
    {
        /* Return error status */
        sts = NUF_IO_ERROR;
    }
    else
    {
	
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE 	
       /* Check current state of the device. If the device is off, suspend on an
           event until the device power state changes to ON */
        if (sess_handle->inst_info->dev_info.current_state == SD_OFF)
        {
            /* Now retrieve the event to cause suspension */
            sts = NU_Retrieve_Events(sd_dev->read_event_ptr, SD_POWER_EVENT_MASK,
                                     NU_AND, &retrieved_events, NU_SUSPEND);
        }
#endif
        /* Device state is now ON, so we can read */
        if (sts == NU_SUCCESS)
        {
            *bytes_read_ptr = 0;

            sts =  NU_SDIO_FDR_Imp_RDWR((SDDEVICE*)inst_handle->pdcb.p_SDDevice,
                             (byte_offset / NU_SDIO_FILE_STD_BLKSIZE),  /* sector */
                             (UINT8*)buffer,
                             (numbyte / NU_SDIO_FILE_STD_BLKSIZE),      /* count */
                             1);                                        /* read */

            /* If the request succeeded */
            if (sts == NU_SUCCESS)
            {
                /* Set the number of bytes read */
                *bytes_read_ptr = numbyte;
            }
        }
    }

    return (sts);
}

/*************************************************************************
*
*   FUNCTION
*
*       SD_DVM_Write
*
*   DESCRIPTION
*
*       This function writes to the SD hardware
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Write buffer
*       INT          numbyte                - Number of bytes to write
*       OFFSET_T     byte_offset            - byte offset from zero to start write
*       INT          *bytes_written_ptr     - Pointer to return number of bytes written
*
*   OUTPUTS
*
*       INT          bytes_written          - Number of bytes written
*
*************************************************************************/
STATUS SD_DVM_Write (VOID *session_handle, const VOID *buffer, UINT32 numbyte, OFFSET_T byte_offset, UINT32 *bytes_written_ptr)
{
    STATUS             sts = NU_SUCCESS;
    SD_SESSION_HANDLE  *sess_handle = (SD_SESSION_HANDLE*)session_handle;
    SD_INSTANCE_HANDLE *inst_handle = (SD_INSTANCE_HANDLE*)(sess_handle->inst_info);
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE 	
    SD_DEV             *sd_dev = &(inst_handle->dev_info);
    UNSIGNED           retrieved_events;
#endif

    /* Check the buffer pointer */
    if ((buffer == NU_NULL)  || (bytes_written_ptr == NU_NULL))
    {
        /* Return error status */
        sts = NUF_IO_ERROR;
    }
    else
    {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE 	
       /* Check current state of the device. If the device is off, suspend on an
           event until the device power state changes to ON */
        if (sess_handle->inst_info->dev_info.current_state == SD_OFF)
        {
            /* Now retrieve the event to cause suspension */
            sts = NU_Retrieve_Events(sd_dev->write_event_ptr, SD_POWER_EVENT_MASK,
                                     NU_AND, &retrieved_events, NU_SUSPEND);
        }
#endif
        /* Device state is now ON, so we can transmit */
        if (sts == NU_SUCCESS)
        {
            *bytes_written_ptr = 0;

            sts =  NU_SDIO_FDR_Imp_RDWR((SDDEVICE*)inst_handle->pdcb.p_SDDevice,
                             (byte_offset / NU_SDIO_FILE_STD_BLKSIZE),  /* sector */
                             (UINT8*)buffer,
                             (numbyte / NU_SDIO_FILE_STD_BLKSIZE),      /* count */
                             0);                                        /* write */

            /* If the request succeeded */
            if (sts == NU_SUCCESS)
            {
                /* Set the number of bytes written */
                *bytes_written_ptr = numbyte;
            }
        }
    }

    return (sts);
}

/*************************************************************************
*
*   FUNCTION
*
*       SD_DVM_Ioctl
*
*   DESCRIPTION
*
*       This function controls IO operations of the SD driver.
*
*   INPUTS
*       VOID          *session_handle       - Session handle of the driver
*       INT           cmd                   - Ioctl command
*       VOID          *data                 - Ioctl data pointer
*       INT           length                - Ioctl length
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS
*
*************************************************************************/
STATUS SD_DVM_Ioctl(VOID *session_handle, INT cmd, VOID *data, INT length)
{
    STATUS                 status = NU_SUCCESS;
    INT                    i;
    DV_IOCTL0_STRUCT       *ioctl0;
    FPART_DISK_INFO_S      *disk_info;
    SD_SESSION_HANDLE      *sess_handle = (SD_SESSION_HANDLE*)session_handle;
    SD_INSTANCE_HANDLE     *inst_handle = (SD_INSTANCE_HANDLE*)(sess_handle->inst_info);
#ifdef  CFG_NU_OS_STOR_ENABLE
    STATUS                 *chkdsk_sts;
    STORAGE_MW_CONFIG_PATH *config_path;
    CHAR                   reg_path[REG_MAX_KEY_LENGTH];
#endif
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE 	
    STATUS                 pm_status;
    PM_STATE_ID            *pm_temp;
    DV_DEV_LABEL           open_label_list[MAX_NUM_OF_MODES] = {{STORAGE_LABEL},
                                                                {SD_LABEL},
                                                                {POWER_CLASS_LABEL}};
    INT                    open_mode_list[MAX_NUM_OF_MODES] = {STORAGE_OPEN_MODE,
                                                               SD_OPEN_MODE,
                                                               POWER_OPEN_MODE};
#else
    DV_DEV_LABEL           open_label_list[MAX_NUM_OF_MODES] = {{STORAGE_LABEL},
                                                                {SD_LABEL}};
    INT                    open_mode_list[MAX_NUM_OF_MODES] = {STORAGE_OPEN_MODE,
                                                               SD_OPEN_MODE};
#endif
    INT                    open_mode_requests = 0;

    /* Process command */
    switch (cmd)
    {
        case DV_IOCTL0:

            if (length == sizeof(DV_IOCTL0_STRUCT))
            {
                /* Get the ioctl0 structure from the data passed in */
                ioctl0 = (DV_IOCTL0_STRUCT *) data;

                /* Get open mode requests from labels */
                for (i = 0; i < MAX_NUM_OF_MODES; i++)
                {
                    status = DVS_Label_List_Contains (&(ioctl0->label), 1, open_label_list[i]);
                    if (status == NU_SUCCESS)
                    {
                        open_mode_requests = open_mode_list[i];
                        break;
                    }
                }

                /* If the mode requested is supported and if the session was opened for that mode */
                if ((open_mode_requests & STORAGE_OPEN_MODE) &&
                    (((SD_SESSION_HANDLE*)sess_handle)->open_modes & open_mode_requests))
                {
                    ioctl0->base = STORAGE_CMD_BASE;
                }
                else if ((open_mode_requests & POWER_OPEN_MODE &&
                         ((SD_SESSION_HANDLE*)sess_handle)->open_modes & open_mode_requests))
                {
                    ioctl0->base = SD_POWER_BASE;
                }
                else if ((open_mode_requests & SD_OPEN_MODE &&
                         ((SD_SESSION_HANDLE*)sess_handle)->open_modes & open_mode_requests))
                {
                    ioctl0->base = SD_MEDIA_BASE;
                }
                else
                {
                    status = DV_IOCTL_INVALID_MODE;
                }
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }

            break;

#ifdef  CFG_NU_OS_STOR_ENABLE
        case (STORAGE_CMD_BASE + FDEV_GET_DISK_INFO):

            /* Get the dev info structure from the data passed in */
            disk_info = (FPART_DISK_INFO_S *) data;

            /* Populate FPART_DISK_INFO_S data */
            strncpy(disk_info->fpart_name, inst_handle->pdcb.name, sizeof(disk_info->fpart_name));
            disk_info->fpart_flags = FPART_DI_RMVBL_MED;
            disk_info->fpart_bytes_p_sec = NU_SDIO_FILE_STD_BLKSIZE;   /* Fixed at 512. */
            disk_info->fpart_heads = SD_FPART_HEADS;
            disk_info->fpart_secs = SD_FPART_SECTORS;
            disk_info->fpart_tot_sec = inst_handle->pdcb.card_cap;                  /* Total capacity. */
            disk_info->fpart_cyls = disk_info->fpart_tot_sec / (SD_FPART_SECTORS * SD_FPART_HEADS);

            break;

        case (STORAGE_CMD_BASE + FDEV_GET_DISK_STATUS):

            chkdsk_sts = (STATUS *) data;

            *chkdsk_sts = NU_SUCCESS;

            break;

        case (STORAGE_CMD_BASE + FDEV_FLUSH):

            break;

        case (STORAGE_CMD_BASE + FDEV_TRIM):

            break;

        case (STORAGE_CMD_BASE + FDEV_GET_MW_CONFIG_PATH):

            config_path = (STORAGE_MW_CONFIG_PATH *) data;

            /* Return the middleware config path */
            strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
            strcat(reg_path, "/mw_settings");
            strncpy(config_path->config_path, reg_path, config_path->max_path_len);

            break;
#endif /* #ifdef  CFG_NU_OS_STOR_ENABLE */

        case (SD_MEDIA_BASE + SD_MEDIA_INFO_IOCTL):

            /* Get the dev info structure from the data passed in */
            disk_info = (FPART_DISK_INFO_S *) data;

            /* Populate FPART_DISK_INFO_S data */
            strncpy(disk_info->fpart_name, inst_handle->pdcb.name, sizeof(disk_info->fpart_name));
            disk_info->fpart_flags = FPART_DI_RMVBL_MED;
            disk_info->fpart_bytes_p_sec = NU_SDIO_FILE_STD_BLKSIZE;   /* Fixed at 512. */
            disk_info->fpart_heads = SD_FPART_HEADS;
            disk_info->fpart_secs = SD_FPART_SECTORS;
            disk_info->fpart_tot_sec = inst_handle->pdcb.card_cap;     /* Total capacity. */
            disk_info->fpart_cyls = disk_info->fpart_tot_sec / (SD_FPART_SECTORS * SD_FPART_HEADS);

            break;

        /* Power IOCTLs */
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE 
        case (SD_POWER_BASE + POWER_IOCTL_GET_STATE):

            if((length == sizeof(PM_STATE_ID)) && (data != NU_NULL))
            {
                pm_temp = (PM_STATE_ID *)data;
                *pm_temp = inst_handle->dev_info.current_state;
            }

            break;

        case (SD_POWER_BASE + POWER_IOCTL_SET_STATE):

            if((length == sizeof(PM_STATE_ID)) && (data != NU_NULL))
            {
                pm_temp = (PM_STATE_ID*)data;

                pm_status = SD_Set_State(inst_handle, pm_temp);
                if (pm_status != NU_SUCCESS)
                {
                    status = DV_INVALID_INPUT_PARAMS;
                }
            }

            break;

        case (SD_POWER_BASE + POWER_IOCTL_GET_STATE_COUNT):

            if((length == sizeof(PM_STATE_ID)) && (data != NU_NULL))
            {
                pm_temp = (PM_STATE_ID *)data;
                *pm_temp = SD_TOTAL_STATE_COUNT;
            }

            break;
#endif
        default:

                status = DV_IOCTL_INVALID_CMD;

            break;
    }

    return (status);
}
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE 
/*************************************************************************
*
*   FUNCTION
*
*       SD_Set_State
*
*   DESCRIPTION
*
*       This is the IOCTL function that sets the state of the SD
*
*   INPUT
*
*       VOID        *inst_handle            - Instance handle
*       PM_STATE_ID *state                  - Power state
*
*   OUTPUT
*
*       STATUS   pm_status               - NU_SUCCESS
*
*************************************************************************/
static STATUS SD_Set_State(VOID *inst_handle, PM_STATE_ID *state)
{
    STATUS      pm_status = NU_SUCCESS;
    SD_DEV      *sd_dev = &(((SD_INSTANCE_HANDLE*)inst_handle)->dev_info);

    /* Check input parameters */
    if ((inst_handle == NU_NULL) || (state == NU_NULL))
    {
        pm_status = PM_INVALID_PARAMETER;
    }
    else
    {
        /* Enable SD only if already OFF */
        if (((*state == SD_ON) || (*state == POWER_ON_STATE)) && (sd_dev->current_state == SD_OFF))
        {
            if (sd_dev->device_in_use == NU_TRUE)
            {
                /* Update the state of the device in the State Information structure */
                sd_dev->current_state = SD_ON;

                 /* Now set the write event because the device is enabled */
                (VOID)NU_Set_Events(sd_dev->write_event_ptr, SD_POWER_EVENT_MASK, NU_OR);

                /* Now set the read event because the device is enabled */
                (VOID)NU_Set_Events(sd_dev->read_event_ptr, SD_POWER_EVENT_MASK, NU_OR);

            }
            else
            {
                /* Device is not in use yet, so just change the state of the device */
                sd_dev->current_state = *state;
            }
        }

        /* Disable RD only if already ON */
        if ((*state == SD_OFF) && ((sd_dev->current_state == SD_ON) || (sd_dev->current_state == POWER_ON_STATE)))
        {
            /* Reset the events as previous resumes and ON may have incremented count */
            (VOID)NU_Set_Events(sd_dev->write_event_ptr, ~SD_POWER_EVENT_MASK, NU_AND);
            (VOID)NU_Set_Events(sd_dev->read_event_ptr, ~SD_POWER_EVENT_MASK, NU_AND);

            /* Update the state of the device in the State Information structure */
            sd_dev->current_state = SD_OFF;
        }
    }

    return (pm_status);
}
#endif
