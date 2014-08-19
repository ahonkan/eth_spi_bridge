/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
*                      All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       dma.c
*
* COMPONENT
*
*       DMA Device Interface
*
* DESCRIPTION
*
*       This file contains the routine for DMA device interface.
*
* DATA STRUCTURES
*
*       DMA_Device_Handle
*
* FUNCTIONS
*
*       NU_DMA_Open
*       NU_DMA_Data_Transfer
*       NU_DMA_Acquire_Channel
*       NU_DMA_Release_Channel
*       NU_DMA_Reset_Channel
*       NU_DMA_Close
*       DMA_Get_Device_CB_Index
*       DMA_Comp_Callback
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/nu_services.h"
#include    "drivers/nu_drivers.h"

/* Device handle array. */
static DMA_DEVICE_HANDLE DMA_Device_Handle[DMA_MAX_DEVICES];

/* Function prototypes. */
static UINT8 DMA_Get_Device_CB_Index(DMA_DEVICE_HANDLE dev_handle);
static VOID DMA_Comp_Callback(DMA_CHANNEL * dma_channel, STATUS status);



/*************************************************************************
*
* FUNCTION
*
*       NU_DMA_Open
*
* DESCRIPTION
*
*       This function opens dma device.
*
* INPUTS
*       dma_dev_id                       - DMA device ID to open.
*                                          This id is defined in platform file.
*       *dma_handle_ptr                  - Pointer to location where dma
*                                          handle would be returned.
*
* OUTPUTS
*
*       NU_SUCCESS                      - Successful completion
*       NU_INVALID_POINTER              - Invalid pointer
*       NU_DMA_DEVICE_NOT_FREE          - DMA device control block is not free
*
*************************************************************************/
STATUS NU_DMA_Open(UINT8 dma_dev_id,
                   DMA_DEVICE_HANDLE *dma_handle_ptr)
{
    STATUS          status = NU_SUCCESS;
    UINT8           index;
    DV_DEV_LABEL    labels = {DMA_LABEL};
    DV_DEV_HANDLE   dev_handle = DV_INVALID_HANDLE;
    INT             int_level;
    INT             i;
    NU_MEMORY_POOL  *sys_pool_ptr;

    /* Check if pointer is invalid. */
    if (dma_handle_ptr == NU_NULL)
    {
        status = NU_INVALID_POINTER;
    }

    /* Disable interrupts before clearing shared variable */
    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    if (status == NU_SUCCESS)
    {
        /* Check if device is already open with this device index. */
        for(index = 0; index < DMA_MAX_DEVICES; index++)
        {
            if(DMA_Device_Handle[index] != NU_NULL &&
               DMA_Device_Handle[index]->dma_dev_id == dma_dev_id)
            {
                /* Increment user count. */
                DMA_Device_Handle[index]->user_count++;

                /* Return Handler. */
                *dma_handle_ptr = DMA_Device_Handle[index];

                break;
            }
        }
    }

    /* Check if device is not opened. */
    if (status == NU_SUCCESS && index == DMA_MAX_DEVICES)
    {
        /* Get free control block. */
        index = DMA_Get_Device_CB_Index(NU_NULL);

        /* Check if could not find free control block. */
        if (index == DMA_MAX_DEVICES)
        {
            status = NU_DMA_DEVICE_NOT_FREE;
        }
        else
        {
            /* Build device label using  dma device id. */
            labels.data[0] = dma_dev_id;

            /* Open DMA device. */
            status = DVC_Dev_Open (&labels, &dev_handle);

            /* Get system memory pool pointer. */
            if (status == NU_SUCCESS)
            {
                status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);
            }

            /* Allocate memory for dma device. */
            if (status == NU_SUCCESS)
            {
                status = NU_Allocate_Memory (sys_pool_ptr, (VOID**)&DMA_Device_Handle[index],
                                             sizeof(DMA_DEVICE), NU_NO_SUSPEND);
            }

            if (status == NU_SUCCESS)
            {
                /* Clear dma device control block. */
                ESAL_GE_MEM_Clear(DMA_Device_Handle[index], sizeof(DMA_DEVICE));

                /* Set user count. */
                DMA_Device_Handle[index]->user_count = 1;

                /* Save device id. */
                DMA_Device_Handle[index]->dma_dev_id = dma_dev_id;

                /* Save device handle. */
                DMA_Device_Handle[index]->dev_handle = dev_handle;

                /* Create event group. Each channel would require 2 event bit.
                   One for completion and other for error. */
                for (i = 0 ; i < ((DMA_MAX_CHANNELS + 15) >> 4) && status == NU_SUCCESS; i++)
                {
                    status = NU_Create_Event_Group(&(DMA_Device_Handle[index]->chan_comp_evt[i]), "DMA_Evt");
                }
            }

            if (status == NU_SUCCESS)
            {
                /* Return Handler. */
                *dma_handle_ptr = DMA_Device_Handle[index];

                /* Set completion callback function. */
                status = DVC_Dev_Ioctl (dev_handle, DMA_SET_COMP_CALLBACK,
                                        DMA_Comp_Callback, sizeof(VOID *));
            }
        }
    }

    /* Restore interrupts to previous level */
    NU_Local_Control_Interrupts(int_level);

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_DMA_Data_Transfer
*
* DESCRIPTION
*
*       This function transfer the data on DMA channel.
*
* INPUTS
*       chan_handle                      - Channel Handle
*       *dma_req_ptr                     - Pointer to transfer requests
*       total_requests                   - Total transfer requests.
*       is_cached                        - Flag to indicate cached memory buffer.
*       req_type                         - Transfer request type.
*       suspend                          - Function suspension option
*
* OUTPUTS
*
*       NU_SUCCESS                      - Successful completion
*       NU_DMA_INVALID_HANDLE           - Invalid channel handle
*       NU_DMA_DRIVER_ERROR             - Some error from underlying driver.
*       NU_TIMEOUT                      - If timeout on service
*       NU_DMA_INVALID_REQUEST_COUNT    - If invalid request count is made.  
*                                         Cannot use DMA_LENGTH_CONTINUOUS
*                                         in SYNC mode. 
*************************************************************************/
STATUS NU_DMA_Data_Transfer(DMA_CHAN_HANDLE chan_handle,
                            DMA_REQ * dma_req_ptr,
                            UINT32 total_requests,
                            UINT8 is_cached,
                            DMA_REQUEST_TYPE req_type,
                            UNSIGNED suspend)
{
    STATUS              status = NU_SUCCESS;
    UINT8               chan_idx;
    UINT8               user_idx;
    UINT8               dev_idx;
    DMA_CHANNEL         *channel;
    DMA_DEVICE_HANDLE   dma_handle;
    UNSIGNED            retrieved_flags;
#if (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)
    UINT32              i;
#endif

    /* Validate channel handle. */
    if (!DMA_CHECK_VALID_CHAN_HANDLE(chan_handle))
    {
        status = NU_DMA_INVALID_HANDLE;
    }

    if (status == NU_SUCCESS)
    {
        /* Get channel index. */
        chan_idx = DMA_GET_CHAN_INDEX(chan_handle);

        /* Get user index. */
        user_idx = DMA_GET_USER_INDEX(chan_handle);

        /* Get device index. */
        dev_idx = DMA_GET_DEV_CB_INDEX(chan_handle);

        /* Get device handle. */
        dma_handle = DMA_Device_Handle[dev_idx];

        /* Validate device handle. */
        if (dma_handle == NU_NULL)
        {
            status = NU_DMA_INVALID_HANDLE;
        }
        else
        {
            /* Get channel pointer. */
            channel = &dma_handle->channels[chan_idx][user_idx];

            /* Check if channel is enabled. */
            if (channel->enabled == NU_FALSE)
            {
                status = NU_DMA_INVALID_HANDLE;
            }
        }
    }

    if ( (total_requests == DMA_LENGTH_CONTINUOUS) && 
          ((req_type == DMA_SYNC_SEND) || 
           (req_type == DMA_SYNC_RECEIVE) || 
           (req_type == DMA_SYNC_MEM_TRANS)) )
      return NU_DMA_INVALID_REQUEST_COUNT;

    
    /* Obtain semaphore for exclusive access on channel. */
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore(&dma_handle->chan_semaphore[chan_idx], suspend);
    }

    if (status == NU_SUCCESS)
    {
        /* Save request pointer. */
        channel->cur_req_ptr = dma_req_ptr;

        /* Save total requests. */
        channel->cur_req_length = total_requests;

        /* Save request type. */
        channel->cur_req_type = req_type;

#if (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)

        /* Check if cached buffers. */
        if (is_cached)
        {
            /* Check if memory to peripheral or memory to memory transfer. */
            if (req_type == DMA_SYNC_SEND || req_type == DMA_SYNC_MEM_TRANS ||
                req_type == DMA_ASYNC_SEND || req_type == DMA_ASYNC_MEM_TRANS)
            {
                /* Flush invalidate all source buffers. */
                for(i = 0; i < total_requests; i++)
                {
                    ESAL_GE_MEM_DCACHE_FLUSH_INVAL(dma_req_ptr[i].src_ptr,
                        dma_req_ptr[i].length + ESAL_CO_MEM_CACHE_LINE_SIZE);
                }
            }

            /* Check if peripheral to memory or memory to memory transfer. */
            if (req_type == DMA_SYNC_RECEIVE || req_type == DMA_SYNC_MEM_TRANS ||
                req_type == DMA_ASYNC_RECEIVE || req_type == DMA_ASYNC_MEM_TRANS)
            {
                /* Flush invalidate all destination buffers. It is required for safe invalidate
                   operation after data transfer. */
                for(i = 0; i < total_requests; i++)
                {
                    ESAL_GE_MEM_DCACHE_FLUSH_INVAL(dma_req_ptr[i].dst_ptr,
                        dma_req_ptr[i].length + ESAL_CO_MEM_CACHE_LINE_SIZE);
                }
            }
        }

#endif
        /* Trigger driver's data transfer IOCTL. */
        status = DVC_Dev_Ioctl (dma_handle->dev_handle, DMA_DATA_TRANSFER,
                                (VOID* )channel, sizeof(DMA_CHANNEL *));

        /* Check if synchronous data transfer operation. */
        if ((req_type == DMA_SYNC_SEND ||
            req_type == DMA_SYNC_RECEIVE ||
            req_type ==DMA_SYNC_MEM_TRANS) && status == NU_SUCCESS)
        {
            /* Wait of completion or error event on channel. */
            status = NU_Retrieve_Events(&dma_handle->chan_comp_evt[chan_idx >> 4],
                                        (3 << ((chan_idx & 0xF) << 1)), NU_OR_CONSUME,
                                        &retrieved_flags, NU_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Release channel semaphore. */
                status = NU_Release_Semaphore(&dma_handle->chan_semaphore[chan_idx]);

                /* Check if transfer completion event. */
                if ((retrieved_flags & (1 << ((chan_idx & 0xF) << 1))) != 0)
                {
#if (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)
                    /* Check if cached buffers and synchronous data receive operation. */
                    if (is_cached && (channel->cur_req_type == DMA_SYNC_RECEIVE ||
                                      channel->cur_req_type == DMA_SYNC_MEM_TRANS))
                    {
                        /* Invalidate all destination buffers. */
                        for(i = 0; i < channel->cur_req_length; i++)
                        {
                            ESAL_GE_MEM_DCACHE_INVALIDATE(channel->cur_req_ptr[i].dst_ptr, channel->cur_req_ptr[i].length + ESAL_CO_MEM_CACHE_LINE_SIZE);
                        }
                    }
#endif
                }
                else
                {
                    /* Internal DMA driver error. */
                    status = NU_DMA_DRIVER_ERROR;
                }
            }
        }
    }

    return status;
}


/*************************************************************************
*
* FUNCTION
*
*       NU_DMA_Acquire_Channel
*
* DESCRIPTION
*
*       This function acquires DMA channel.
*
* INPUTS
*
*       dma_handle                       - DMA device handle
*       chan_handle_ptr                  - Pointer to location where channel
*                                          handle would be returned.
*       hw_chan_id                       - Hardware channel ID. This channel
*                                          id would be used by underlying
*                                          DMA device driver.
*       peri_id                          - Peripheral id. This id would be
*                                          used by underlying DMA device driver.
*       comp_callback                    - Completion call back function pointer
*
* OUTPUTS
*
*       NU_SUCCESS                      - Successful completion
*       NU_DMA_INVALID_HANDLE           - Invalid device handle
*
*************************************************************************/
STATUS NU_DMA_Acquire_Channel(DMA_DEVICE_HANDLE dma_handle,
                              DMA_CHAN_HANDLE *chan_handle_ptr,
                              UINT8 hw_chan_id,
                              UINT8 peri_id,
                              VOID (*comp_callback)(DMA_CHAN_HANDLE , DMA_REQ *, UINT32 , STATUS )
                              )
{
    STATUS status = NU_SUCCESS;
    UINT8 chan_idx;
    UINT8 user_idx;
    UINT8 dev_index;
    INT         int_level;

    /* Get device index. */
    dev_index = DMA_Get_Device_CB_Index(dma_handle);

    /* Disable interrupts before clearing shared variable */
    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Check if device not found. */
    if (dev_index == DMA_MAX_DEVICES)
    {
        status = NU_DMA_INVALID_HANDLE;
    }
    else
    {
        /* Find Channel with this hw_chan_id. */
        for(chan_idx = 0; chan_idx < DMA_MAX_CHANNELS &&
            dma_handle->channels_inuse >= (1 << chan_idx); chan_idx++)
        {
            /* Optimized search for enabled channels only. */
            if (dma_handle->channels_inuse & (1<<chan_idx))
            {
                for(user_idx = 0; user_idx < DMA_MAX_CHAN_USERS; user_idx++)
                {
                    /* Check if channel is enabled with same hardware channel id. */
                    if (dma_handle->channels[chan_idx][user_idx].enabled
                        && (dma_handle->channels[chan_idx][user_idx].hw_chan_id == hw_chan_id))
                    {
                        /* Check if channel is enabled with same hardware channel  and peripheral id. */
                        if (dma_handle->channels[chan_idx][user_idx].peri_id == peri_id)
                        {
                            /* Two channel cannot be acquired with same channel and peripheral id. */
                            status = NU_DMA_CHANNEL_ALREADY_OPEN;
                            break;
                        }

                        /* Set status to indicate that channel is already opened with this channel id. */
                        status = NU_DMA_CHANNEL_USER_EXIST;
                    }
                }

                /* Entry found. */
                if (status != NU_SUCCESS)
                {
                    break;
                }
            }
        }

        /* Check if new channel id. */
        if (status == NU_SUCCESS)
        {
            user_idx = 0;

            /* Find Free Channel. */
            for(chan_idx = 0; chan_idx < DMA_MAX_CHANNELS; chan_idx++)
            {
                /* Check if channel is not in use. */
                if ((dma_handle->channels_inuse & (1 << chan_idx)) == 0)
                {
                    break;
                }
            }

            /* Check maximum channels limit reached. */
            if (chan_idx == DMA_MAX_CHANNELS)
            {
                status = NU_DMA_CHANNEL_MAX_REACHED;
            }
        }
        /* Check if some user already exist with this hardware channel id. */
        else if (status == NU_DMA_CHANNEL_USER_EXIST)
        {
            /* Find free slot for new user. */
            for(user_idx = 0; user_idx < DMA_MAX_CHAN_USERS; user_idx++)
            {
                if (dma_handle->channels[chan_idx][user_idx].enabled == NU_FALSE)
                {
                    break;
                }
            }

            /* Check maximum users limit reached. */
            if (user_idx == DMA_MAX_CHAN_USERS)
            {
                status = NU_DMA_CHANNEL_MAX_USER_REACHED;
            }
        }

        /* Check if got free slot for channel. */
        if(status == NU_SUCCESS || status == NU_DMA_CHANNEL_USER_EXIST)
        {
            /* Set flag to indicate channel is enabled. */
            dma_handle->channels[chan_idx][user_idx].enabled = NU_TRUE;

            /* Save peripheral ID. */
            dma_handle->channels[chan_idx][user_idx].peri_id = peri_id;

            /* Save channel ID. */
            dma_handle->channels[chan_idx][user_idx].hw_chan_id = hw_chan_id;

            /* Set flag to indicate channel is in use. */
            dma_handle->channels_inuse |= (1<<chan_idx);

            /* Create channel handle. */
            *chan_handle_ptr = chan_idx | (user_idx << 8) | (dev_index << 16) | DMA_CHANNEL_HANDLE_SIGNATURE;

            /* Save channel handle. */
            dma_handle->channels[chan_idx][user_idx].chan_handle = *chan_handle_ptr;
        }
    }

    /* If first user. */
    if (status == NU_SUCCESS)
    {
        /* Create channel semaphore. */
        status = NU_Create_Semaphore(&dma_handle->chan_semaphore[chan_idx], "DMA_SEM", 1, NU_FIFO);
    }

    /* Restore interrupts to previous level */
    NU_Local_Control_Interrupts(int_level);

    /* Check if got free slot for channel. */
    if(status == NU_SUCCESS || status == NU_DMA_CHANNEL_USER_EXIST)
    {
        /* Save completion callback function. */
        dma_handle->channels[chan_idx][user_idx].comp_callback = comp_callback;

        /* Call driver IOCTL to acquire channel.  */
        status = DVC_Dev_Ioctl(dma_handle->dev_handle, DMA_ACQUIRE_CHANNEL,
                               &(dma_handle->channels[chan_idx][user_idx]), sizeof(DMA_CHANNEL *));
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_DMA_Release_Channel
*
* DESCRIPTION
*
*       This function releases DMA channel.
*
* INPUTS
*
*       chan_handle                      - DMA channel handle
*
* OUTPUTS
*
*       NU_SUCCESS                      - Successful completion
*       NU_DMA_INVALID_HANDLE           - Invalid channel handle
*
*************************************************************************/
STATUS NU_DMA_Release_Channel(DMA_CHAN_HANDLE chan_handle)
{
    STATUS  status = NU_SUCCESS;
    UINT8   chan_idx;
    UINT8   user_idx;
    UINT8   dev_idx;
    INT     int_level;

#if (DMA_MAX_CHAN_USERS > 1)
    UINT8 index;
#endif

    /* Validate channel handle. */
    if (!DMA_CHECK_VALID_CHAN_HANDLE(chan_handle))
    {
        status = NU_DMA_INVALID_HANDLE;
    }

    if (status == NU_SUCCESS)
    {
        /* Get channel index. */
        chan_idx = DMA_GET_CHAN_INDEX(chan_handle);

        /* Get user index. */
        user_idx = DMA_GET_USER_INDEX(chan_handle);

        /* Get device index. */
        dev_idx = DMA_GET_DEV_CB_INDEX(chan_handle);

        if (DMA_Device_Handle[dev_idx] == NU_NULL && DMA_Device_Handle[dev_idx]->channels[chan_idx][user_idx].enabled == NU_FALSE)
        {
            status = NU_DMA_INVALID_HANDLE;
        }
        else
        {
            /* Disable interrupts before clearing shared variable */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Set flag to indicate channel is disabled. */
            DMA_Device_Handle[dev_idx]->channels[chan_idx][user_idx].enabled = NU_FALSE;

#if (DMA_MAX_CHAN_USERS > 1)

            /* Check of some other user is already using the channel. */
            for (index = 0; index < DMA_MAX_CHAN_USERS; index++)
            {
                if ((index != user_idx) &&
                    DMA_Device_Handle[dev_idx]->channels[chan_idx][index].enabled)
                {
                    break;
                }
            }

            /* Check if no other user found. */
            if (index == DMA_MAX_CHAN_USERS)
#endif
            {
                /* Set flag to indicate channel is not in use. */
                DMA_Device_Handle[dev_idx]->channels_inuse &= ~(1 << chan_idx);

                /* Delete semaphore. */
                status = NU_Delete_Semaphore(&DMA_Device_Handle[dev_idx]->chan_semaphore[chan_idx]);
            }

            /* Restore interrupts to previous level */
            NU_Local_Control_Interrupts(int_level);

            if (status == NU_SUCCESS)
            {
                /* Call driver IOCTL to release channel.  */
                status = DVC_Dev_Ioctl (DMA_Device_Handle[dev_idx]->dev_handle, DMA_RELEASE_CHANNEL,
                                        &(DMA_Device_Handle[dev_idx]->channels[chan_idx][user_idx]), sizeof(DMA_CHANNEL *));
            }
        }
    }

    return status;
}


/*************************************************************************
*
* FUNCTION
*
*       NU_DMA_Reset_Channel
*
* DESCRIPTION
*
*       This function releases DMA channel.
*
* INPUTS
*
*       chan_handle                      - DMA channel handle
*
* OUTPUTS
*
*       NU_SUCCESS                      - Successful completion
*       NU_DMA_INVALID_HANDLE           - Invalid channel handle
*
*************************************************************************/
STATUS NU_DMA_Reset_Channel(DMA_CHAN_HANDLE chan_handle)
{
    STATUS  status = NU_SUCCESS;
    UINT8   chan_idx;
    UINT8   user_idx;
    UINT8   dev_idx;

    /* Validate channel handle. */
    if (!DMA_CHECK_VALID_CHAN_HANDLE(chan_handle))
    {
        status = NU_DMA_INVALID_HANDLE;
    }

    if (status == NU_SUCCESS)
    {
        /* Get channel index. */
        chan_idx = DMA_GET_CHAN_INDEX(chan_handle);

        /* Get user index. */
        user_idx = DMA_GET_USER_INDEX(chan_handle);

        /* Get device index. */
        dev_idx = DMA_GET_DEV_CB_INDEX(chan_handle);

        /* Check if channel is not enabled. */
        if (DMA_Device_Handle[dev_idx]->channels[chan_idx][user_idx].enabled == NU_FALSE)
        {
            status = NU_DMA_INVALID_HANDLE;
        }
        else
        {
            /* Call driver IOCTL to reset channel.  */
            status = DVC_Dev_Ioctl (DMA_Device_Handle[dev_idx]->dev_handle, DMA_RESET_CHANNEL,
                                    &(DMA_Device_Handle[dev_idx]->channels[chan_idx][user_idx]), sizeof(DMA_CHANNEL *));

            /* Reset channel semaphore. */
            NU_Reset_Semaphore(&DMA_Device_Handle[dev_idx]->chan_semaphore[chan_idx], 1);
        }
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_DMA_Close
*
* DESCRIPTION
*
*       This function closes DMA device.
*
* INPUTS
*
*       chan_handle                      - DMA channel handle
*
* OUTPUTS
*
*       NU_SUCCESS                      - Successful completion
*       NU_DMA_INVALID_HANDLE           - Invalid device handle
*
*************************************************************************/
STATUS NU_DMA_Close(DMA_DEVICE_HANDLE dma_handle)
{
    STATUS  status = NU_SUCCESS;
    UINT8   index = 0;
    UINT32  enable_flag;
    INT     int_level;

    if(dma_handle != NU_NULL)
    {
        /* Get device index. */
        index = DMA_Get_Device_CB_Index(dma_handle);

        /* Check if device found. */
        if (index != DMA_MAX_DEVICES)
        {
            /* Disable interrupts before clearing shared variable */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Check if last user of device. */
            if (dma_handle->user_count == 1)
            {
                /* Close DMA device. */
                status = DVC_Dev_Close(dma_handle->dev_handle);

                if (status == NU_SUCCESS)
                {
                    /* Clear DMA device handle. */
                    DMA_Device_Handle[index] = NU_NULL;

                    /* Delete event groups. */
                    for (index = 0; index < ((DMA_MAX_CHANNELS + 15) >> 4) && status == NU_SUCCESS; index++)
                    {
                        status = NU_Delete_Event_Group(&dma_handle->chan_comp_evt[index]);
                    }
                }

                if (status == NU_SUCCESS)
                {
                    /* Get enabled channels flags. */
                    enable_flag = dma_handle->channels_inuse;

                    index = 0;

                    /* Delete semaphores for all enabled devices. */
                    while(enable_flag && status == NU_SUCCESS)
                    {
                        if (enable_flag & (1 << index))
                        {
                            status = NU_Delete_Semaphore(&dma_handle->chan_semaphore[index]);
                            enable_flag &= ~(1 << index);
                        }

                        index++;
                    }
                }
                if (status == NU_SUCCESS)
                {
                    /* Delete device control block memory.*/
                    status = NU_Deallocate_Memory(dma_handle);
                }
            }
            /* Otherwise this is not last user. */
            else
            {
                /* Decrement device user count. */
                dma_handle->user_count--;
            }

            /* Restore interrupts to previous level */
            NU_Local_Control_Interrupts(int_level);

        }
        else
        {
            /* Set status to indicate invalid device handle. */
            status = NU_DMA_INVALID_HANDLE;
        }

    }
    else
    {
        /* Set status to indicate invalid device handle. */
        status = NU_DMA_INVALID_HANDLE;
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       DMA_Get_Device_CB_Index
*
* DESCRIPTION
*
*       This function search through device array for specified device handle.
*
* INPUTS
*
*       dev_handle                       - DMA device handle
*
* OUTPUTS
*
*       UINT8 0-(DMA_MAX_DEVICES-1)      - Index of device in array
*             DMA_MAX_DEVICES            - device not found
*
*************************************************************************/
static UINT8 DMA_Get_Device_CB_Index(DMA_DEVICE_HANDLE dev_handle)
{
    UINT8 index;

    /* Search through device array for specified device handle. */
    for(index = 0; index < DMA_MAX_DEVICES; index++)
    {
        if(DMA_Device_Handle[index] == dev_handle)
        {
            /* Device found. */
            break;
        }
    }

    return index;
}

/*************************************************************************
*
* FUNCTION
*
*       DMA_Comp_Callback
*
* DESCRIPTION
*
*       This is transfer completion callback function.
*
* INPUTS
*
*       dma_channel                      - Pointer of DMA channel
*       status                           - Completion status of request
*
* OUTPUTS
*
*       None
*
*************************************************************************/
static VOID DMA_Comp_Callback(DMA_CHANNEL * dma_channel, STATUS status)
{
    UINT8 chan_idx;
    UINT8 dev_idx;
    UINT32 event;

    /* Get channel index. */
    chan_idx = DMA_GET_CHAN_INDEX(dma_channel->chan_handle);

    /* Get device index. */
    dev_idx = DMA_GET_DEV_CB_INDEX(dma_channel->chan_handle);

    /* Check if synchronous data transfer operation. */
    if (dma_channel->cur_req_type == DMA_SYNC_SEND ||
        dma_channel->cur_req_type == DMA_SYNC_RECEIVE ||
        dma_channel->cur_req_type == DMA_SYNC_MEM_TRANS)
    {
        /* If successful completion. */
        if (status == NU_SUCCESS)
        {
            /* Calculate event bit for successful completion. */
            event = 1 << ((chan_idx & 0xF) << 1);
        }
        else
        {
            /* Calculate event bit for error. */
            event = (2 << ((chan_idx & 0xF) << 1));
        }

        /* Check if valid handle. */
        if (DMA_Device_Handle[dev_idx] != NU_NULL)
        {
            /* Set event to indicate transfer completion. */
            NU_Set_Events(&DMA_Device_Handle[dev_idx]->chan_comp_evt[chan_idx >> 4], event, NU_OR);
        }
    }

    /* Check if asynchronous data transfer operation. */
    else if (dma_channel->cur_req_type == DMA_ASYNC_SEND ||
             dma_channel->cur_req_type == DMA_ASYNC_RECEIVE ||
             dma_channel->cur_req_type == DMA_ASYNC_MEM_TRANS)
    {
        /* Check if valid handle. */
        if (DMA_Device_Handle[dev_idx] != NU_NULL)
        {
            /* Release channel semaphore if there aren't any more elements to transfer */
            if (dma_channel->cur_req_length == 0)
              NU_Release_Semaphore(&DMA_Device_Handle[dev_idx]->chan_semaphore[chan_idx]);
        }

        /* Check if user callback registered for this channel. */
        if (dma_channel->comp_callback != NU_NULL)
        {
            /* Check user callback function. */
            dma_channel->comp_callback(dma_channel->chan_handle, dma_channel->cur_req_ptr, dma_channel->cur_req_length, status);
        }
    }
}

