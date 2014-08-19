/**************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbh_ms_user_ext.c
*
*
* COMPONENT
*
*       Nucleus USB Host Mass Storage Class SCSI Subclass Driver.
*
* DESCRIPTION
*
*       This file contains subclass driver's routines for Nucleus USB
*       Host Stack's Mass Storage Class Driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*       NU_USBH_MS_USER_Create              Initialize USBH MS User Driver.
*       NU_USBH_MS_USER_Delete              Delete USBH MS User Driver.
*       NU_USBH_MS_USER_Init                NMI initialization Routine.
*       NU_USBH_MS_USER_GetHandle           Getter function for USBH MS
*                                           User Driver.
*       NU_USBH_MS_USER_SET_App_Callbacks   Setter function for Application
*                                           Connection/Disconnection callback.
*      NU_USBH_MS_USER_Get_SC_Dispatch      Getter function for subclass wrapper
*                                           dispatch tables.
*      nu_os_conn_usb_host_ms_user_init     Registry Initialization function.
*
* DEPENDENCIES
*
*       nu_usb.h                            USB Definitions.
*
**************************************************************************/
#ifndef NU_USBH_MS_USER_EXT_C
#define NU_USBH_MS_USER_EXT_C
/* ==============  USB Include Files ==================================  */
#include "connectivity/nu_usb.h"

/* ==========================  Variables =============================== */
BOOLEAN  Init_Flag_user = NU_FALSE;

/* ==========================  Functions =============================== */

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_MS_USER_Create
*
* DESCRIPTION
*
*       This function creates a USBH MS User driver of mass storage
*       class driver.
*
* INPUTS
*
*       cb                                  Pointer to SCSI driver's
*                                           control block.
*       name                                Name of this SCSI driver.
*       cb_pool                             Pointer to an memory pool for
*                                           control blocks used by MSC
*                                           drivers.
*       tx_pool                             Pointer to an memory pool for
*                                           data to be transferred by MSC
*                                           drivers. If data cache is on,
*                                           the memory pool which this
*                                           pointer specifies should be
*                                           non-cache area.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_INVALID_POOL                     Indicates the supplied pool
*                                           pointer is invalid.
*       NU_INVALID_SIZE                     Indicates the size is larger
*                                           than the pool.
*       NU_NO_MEMORY                        Memory not available.
*       NU_INVALID_POINTER                  Indicates control block is
*                                           invalid.
*       NU_USBH_MS_DUPLICATE_INIT           Indicates initialization error.
*
**************************************************************************/
STATUS NU_USBH_MS_USER_Create (NU_USBH_MS_USER  *cb,
                               CHAR             *name,
                               NU_MEMORY_POOL   *cb_pool)
{
    STATUS  status;
    UINT8 index = 0;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(name);
    NU_USB_MEMPOOLCHK(cb_pool);

    /* Check whether this driver has already been initialized or not. */
    if ( NU_FALSE != Init_Flag_user )
    {
        /* This driver has already been initialized. */
        status = NU_USBH_MS_DUPLICATE_INIT;
    }
    else
    {
        /* Allocate memory for MS user driver dispatch lists. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                     (sizeof(NU_USBH_MS_USER_DISPATCH*) *
                                     MAX_NUM_SUBCLASSES),
                                     (void **)&(cb->dispatch_list));
        if(status == NU_SUCCESS)
        {
            memset((void *) cb->dispatch_list, 0,
                        (sizeof(*(cb->dispatch_list))*MAX_NUM_SUBCLASSES));
        }
        if(status == NU_SUCCESS)
        {
#if INCLUDE_SCSI    /* SCSI */

            status = UHMSU_init_Subclass_index(MS_SC_SCSI);
            if (status == NU_SUCCESS)
            {
                status = UHMSU_Get_Subclass_index(MS_SC_SCSI, &index);
                if (status == NU_SUCCESS)
                {
                    cb->dispatch_list[index]=(NU_USBH_MS_USER_DISPATCH*)&usbh_ms_scsi_dispatch;
                }
            }
            
#endif
#if INCLUDE_UFI   /* UFI */

            status = UHMSU_init_Subclass_index(MS_SC_UFI);
            if (status == NU_SUCCESS)
            {
                status = UHMSU_Get_Subclass_index(MS_SC_UFI, &index);
                if (status == NU_SUCCESS)
                {
                    cb->dispatch_list[index]=(NU_USBH_MS_USER_DISPATCH*)&usbh_ms_ufi_dispatch;
                }
            }
                   
#endif
#if INCLUDE_SFF8020I    /* SFF-8020 */

            status = UHMSU_init_Subclass_index(MS_SC_8020);
            if (status == NU_SUCCESS)
            {
                status = UHMSU_Get_Subclass_index(MS_SC_8020, &index);
                if (status == NU_SUCCESS)
                {
                    cb->dispatch_list[index]=(NU_USBH_MS_USER_DISPATCH*)&usbh_ms_8020_dispatch;
                }

            }
           
#endif
#if INCLUDE_SFF8070I    /* SFF-8070i */

            status = UHMSU_init_Subclass_index(MS_SC_8070);
            if (status == NU_SUCCESS)
            {
                status = UHMSU_Get_Subclass_index(MS_SC_8070, &index);
                if (status == NU_SUCCESS)
                {
                    cb->dispatch_list[index]=(NU_USBH_MS_USER_DISPATCH*)&usbh_ms_8070_dispatch;
                }

            }
            
#endif
        }

        if ( NU_SUCCESS == status )
        {
            /* Initialize parameters. */
            Init_Flag_user = NU_TRUE;

            /* Create base. */
            status = _NU_USBH_USER_Create ( (NU_USBH_USER *)cb,
                                            name,
                                            cb_pool,
                                            0xFF, /* all subclasses are accepted*/
                                            /* Protocol is don't care. */
                                            MS_PR_ANY,
                                            &usbh_ms_user_dispatch );
        }
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBH_MS_USER_Delete
*
* DESCRIPTION
*
*       This function deletes a active USBH MS User driver.
*
* INPUTS
*
*       cb                                  Pointer to NU_USBH_MS_SCSI
*                                           Control block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_INVALID_POINTER                  Indicates control block is
*                                           invalid.
*       NU_USBH_MS_NOT_INITIALIZED          This driver hasn't been
*                                           initialized.
*
**************************************************************************/
STATUS NU_USBH_MS_USER_Delete(VOID *cb)
{
    STATUS  status;

    NU_USB_PTRCHK(cb);

    /* Check whether this driver has already been initialized or not. */
    if ( NU_FALSE == Init_Flag_user )
    {
        status = NU_USBH_MS_NOT_INITIALIZED;
    }
    else
    {
        status = USB_Deallocate_Memory(((NU_USBH_MS_USER*)cb)->dispatch_list);
        NU_USB_ASSERT( status == NU_SUCCESS );
		
        /*  Clear the subclass index list */
        UHMSU_Clear_Subclass_Index_List();

        /* Delete base. */
        status = _NU_USBH_USER_Delete( cb );
        NU_USB_ASSERT( status == NU_SUCCESS );

        Init_Flag_user = NU_FALSE;
    }
    return (status);
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_MS_USER_Init
*
*   DESCRIPTION
*
*       This function initializes the MS User driver component via NMI.
*
*   INPUTS
*
*       pSystem_Memory                      Cached Memory Pool.
*       pUncached_System_Memory             Uncached Memory Pool.
*
*   OUTPUTS
*
*       status             Success or failure of the creation of the
*                          underlying initialization routines.
*
*************************************************************************/
STATUS NU_USBH_MS_USER_Init(NU_MEMORY_POOL* pSystem_Memory, NU_MEMORY_POOL* pUncached_System_Memory)
{
    STATUS  status;
    VOID *usbh_ms_handle;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_MEMPOOLCHK_RETURN(pSystem_Memory);
    NU_USB_MEMPOOLCHK_RETURN(pUncached_System_Memory);

    /* Allocate memory for SCSI user driver control block. */
    status = USB_Allocate_Object(sizeof(NU_USBH_MS_USER),
                                 (VOID **)&NU_USBH_MS_USER_Cb_Pt);

    /* Create the device subsystem. */
    if (status == NU_SUCCESS)
    {
        /* Zero out allocated block. */
        memset(NU_USBH_MS_USER_Cb_Pt, 0, sizeof(NU_USBH_MS_USER));
        status = NU_USBH_MS_USER_Create (NU_USBH_MS_USER_Cb_Pt,
                                        "MS_USER",
                                        pSystem_Memory);
    }

    /*  Get the function mass storage class driver handle. */
    if (status == NU_SUCCESS)
    {
        status = NU_USBH_MS_Init_GetHandle(&usbh_ms_handle);
    }

    /* Register the user driver to the class driver. */
    if (status == NU_SUCCESS)
    {
        status = NU_USB_DRVR_Register_User ( (NU_USB_DRVR *) usbh_ms_handle,
                                    (NU_USB_USER *)
                                     NU_USBH_MS_USER_Cb_Pt);
    }

    if (status != NU_SUCCESS)
    {
        /* Clean up */
        if (NU_USBH_MS_USER_Cb_Pt)
        {
            status = NU_USBH_MS_USER_Delete((VOID *)NU_USBH_MS_USER_Cb_Pt);
            status = USB_Deallocate_Memory(NU_USBH_MS_USER_Cb_Pt); 
            NU_USBH_MS_USER_Cb_Pt = NU_NULL;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_MS_USER_DeInit
*
*   DESCRIPTION
*
*       This function de-initializes the MS User driver component via cleanup.
*
*   INPUTS
*
*       cb                VOID pointer to mass storage user driver control block.
*
*   OUTPUTS
*
*       status             Success or failure of the creation of the
*                          underlying initialization routines.
*
*************************************************************************/
STATUS NU_USBH_MS_USER_DeInit(VOID *cb)
{
    STATUS  status;
    VOID *usbh_ms_handle;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    status = NU_USBH_MS_Init_GetHandle(&usbh_ms_handle);

    if (status == NU_SUCCESS)
    {
        NU_USB_DRVR_Deregister_User (usbh_ms_handle,(NU_USB_USER*)(cb));
        
        NU_USBH_MS_USER_Delete((NU_USB_USER*)(cb));

        USB_Deallocate_Memory(NU_USBH_MS_USER_Cb_Pt);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_MS_USER_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the MS User driver control
*       block.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the user
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a host SCSI
*                           user driver.
*       NU_NOT_PRESENT      Indicate that driver has not been initialized.
*
*************************************************************************/
STATUS NU_USBH_MS_USER_GetHandle(NU_USBH_MS_USER  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(handle);

    status = NU_SUCCESS;
    *handle = NU_USBH_MS_USER_Cb_Pt;
    if (NU_USBH_MS_USER_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }
    else
    {
        *handle = NU_USBH_MS_USER_Cb_Pt;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_MS_USER_SET_App_Callbacks
*
*   DESCRIPTION
*
*       This function is used to set the pllication callback functions
*       for Connection nd Disconnection Events
*
*   INPUTS
*
*       dispatch            pointer to structure containing pointers to
*                           connection disconnection callbacks.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a host SCSI
*                           user driver.
*       NU_NOT_PRESENT      Indicate that driver has not been initialized.
*
*************************************************************************/
STATUS NU_USBH_MS_USER_SET_App_Callbacks(VOID *dispatch)
{
    STATUS status;
    NU_USBH_MS_APP_CALLBACKS *callbacks;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    callbacks = (NU_USBH_MS_APP_CALLBACKS *)dispatch;

    NU_USB_PTRCHK_RETURN(callbacks);

    status = NU_SUCCESS;
    if (NU_USBH_MS_USER_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }
    else
    {
        NU_USBH_MS_USER_Cb_Pt -> app_callbacks = callbacks;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_MS_USER_Get_SC_Dispatch
*
*   DESCRIPTION
*
*       This function is used to get the subclass wrapper function dispatch
*       structure used by user.
*
*   INPUTS
*
*       dispatch            Double pointer used to retrieve the subclass
*                           wrapper dispatch.
*       handle              Handle to MS logical unit device.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a host SCSI
*                           user driver.
*       NU_NOT_PRESENT      Indicate that driver has not been initialized.
*
*************************************************************************/
STATUS NU_USBH_MS_USER_Get_SC_Dispatch(NU_USBH_MS_USER_DISPATCH **dispatch,VOID *handle)
{
    STATUS status = NU_SUCCESS;
    MS_LUN *logical_unit;
    UINT8   subclass;
    UINT8   index;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(handle);

    logical_unit = (MS_LUN*)handle;
    subclass = logical_unit->drive->subclass;
    if (NU_USBH_MS_USER_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }
    else
    {
        status = UHMSU_Get_Subclass_index(subclass, &index);
        if(status == NU_SUCCESS)
        {
            *dispatch = NU_USBH_MS_USER_Cb_Pt -> dispatch_list[index];
            if( *dispatch == NU_NULL )
            {
                status = NU_NOT_PRESENT;
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       nu_os_conn_usb_host_ms_user_init
*
*   DESCRIPTION
*
*       This function initializes the mass storage user drivers.
*
*   INPUTS
*
*       path                                Registry path of component.
*       startstop                           Flag to find if component is
*                                           being enabled or disabled.
*
*   OUTPUTS
*
*       status             Success or failure of the creation of the
*                          underlying initialization routines.
*
*************************************************************************/
STATUS nu_os_conn_usb_host_ms_user_init (const CHAR * key, int startstop)
{
    STATUS status;

    if(startstop)
    {
        /* In following API call, passing memory pool ptr parameters
         * NU_NULL because in ReadyStart memory in USB system is
         * allocated through USB specific memory APIs, not directly
         * with any given memory pool pointer. These parameter remain
         * only for backwards code compatibility. */

        status = NU_USBH_MS_USER_Init(NU_NULL, NU_NULL);
#ifdef CFG_NU_OS_DRVR_USB_HOST_FILE_IF_ENABLE
        if (status == NU_SUCCESS)
        {
            status = nu_os_conn_usbh_ms_file_init(key, startstop);
        }
#endif
    }
    else
    {
        if (NU_USBH_MS_USER_Cb_Pt)
        {
            status = NU_USBH_MS_USER_DeInit((VOID *)NU_USBH_MS_USER_Cb_Pt);
        }

        status = NU_SUCCESS;
    }

    return (status);
}

STATUS  NU_USBH_MS_DM_Open (VOID* dev_handle)
{
    STATUS status =NU_SUCCESS;
    MS_LUN * device_ptr;
    NU_USB_PTRCHK(dev_handle);

    device_ptr = (MS_LUN*) dev_handle;
    status = UHMSU_Get_Subclass_index(device_ptr->drive->subclass, &(device_ptr->pad_idx));
    device_ptr->block_size = 512;

    return (status);
}

STATUS  NU_USBH_MS_DM_Close (VOID* dev_handle)
{
    MS_LUN * device_ptr;
    NU_USB_PTRCHK(dev_handle);

    device_ptr = (MS_LUN*) dev_handle;
    device_ptr->pad_idx = 0;

    return (NU_SUCCESS);
}

STATUS  NU_USBH_MS_DM_Read (VOID *session_handle,
                                        VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_read_ptr)
{
    NU_USBH_MS_USER_DISPATCH *dispatch;
    MS_LUN * device_ptr;
    STATUS status =NU_SUCCESS;
    UINT8 command_buffer[16];

    NU_USB_PTRCHK(session_handle);

    device_ptr = (MS_LUN*) session_handle;

    dispatch = NU_USBH_MS_USER_Cb_Pt->dispatch_list[device_ptr->pad_idx];

    status = dispatch->read((VOID *)session_handle, 
                   command_buffer,
                   (byte_offset/(device_ptr->block_size)),
                   (numbyte/(device_ptr->block_size)),
                   buffer,
                   numbyte);

    *bytes_read_ptr = numbyte;

    return (status);
}

STATUS  NU_USBH_MS_DM_Write (VOID *session_handle,
                                        const VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_written_ptr)
{
    NU_USBH_MS_USER_DISPATCH *dispatch;
    MS_LUN * device_ptr;
    STATUS status =NU_SUCCESS;
    UINT8 command_buffer[16];

    NU_USB_PTRCHK(session_handle);

    device_ptr = (MS_LUN*) session_handle;

    dispatch = NU_USBH_MS_USER_Cb_Pt->dispatch_list[device_ptr->pad_idx];

    status = dispatch->write((VOID *)session_handle, 
                   command_buffer,
                   (byte_offset/(device_ptr->block_size)),
                   (numbyte/(device_ptr->block_size)),
                   (void*)buffer,
                   numbyte);

    *bytes_written_ptr = numbyte;

    return (status);
}

STATUS  NU_USBH_MS_DM_IOCTL (VOID *session_handle,
                                        INT cmd,
                                        VOID *data,
                                        INT length)
{
    NU_USBH_MS_USER_DISPATCH *dispatch;
    MS_LUN * device_ptr;
    STATUS status =NU_SUCCESS;
    UINT8 command_buffer[16];

    NU_USB_PTRCHK(session_handle);
    device_ptr = (MS_LUN*) session_handle;
    dispatch = NU_USBH_MS_USER_Cb_Pt->dispatch_list[device_ptr->pad_idx];

    switch(cmd)
    {
        case (USB_STORE_IOCTL_BASE + USBH_MEDIA_READ_CAPACITY):
            if(length > 8)
            {
            status = dispatch->read_capacity((VOID *)session_handle, 
                           command_buffer,
                           (UINT8*)data);
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }

        break;
        case (USB_STORE_IOCTL_BASE + USBH_MEDIA_INQUIRY):
            if(data != 0)
            {
            status = dispatch->inquiry((VOID *)session_handle, 
                           command_buffer,
                           (UINT8*)data,length);
            }
            else
            {
                status = DV_IOCTL_INVALID_CMD;
            }

        break;
         case (USB_STORE_IOCTL_BASE + USBH_MEDIA_TEST_UNIT_READY):
            status = dispatch->unit_ready((VOID *)session_handle, 
                           command_buffer);
        break;
        case (USB_STORE_IOCTL_BASE + USBH_MEDIA_REQUEST_SENSE):
            if(data != 0)
            {
            status = dispatch->request_sense((VOID *)session_handle, 
                           command_buffer,
                           (UINT8*)data,length);
            }
            else
            {
                status = DV_IOCTL_INVALID_CMD;
            }

        break;
        case (USB_STORE_IOCTL_BASE + USBH_MEDIA_SET_DATA_BUFF_CACHABLE):
            if (data != 0)
            {
                status = NU_USBH_MS_Set_Data_Buff_Cachable(device_ptr->drive,
                                                            *((BOOLEAN*)data));
            }
            else
            {
                status = DV_IOCTL_INVALID_CMD;
            }
        break;
        default:
            status = DV_IOCTL_INVALID_CMD;
        break;
    }

    return (status);
}

#endif /*_NU_USBH_MS_USER_EXT_C*/

/* ======================  End Of File  ================================ */
