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
*       bcm.c
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Block Cache Manager
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Device layer LRU block cache manager for mounted file system
*       devices.                                 
*                                                                       
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       BCM_List_Sema               Controls access to the list of cache
*                                   control blocks
*       NU_FILE_Cache_Error_Queue   Error log for async IO access errors      
*       NU_FILE_Cache_Error_Size    Size of a single error log entry
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       bcm_get_slot_at_index
*       bcm_get_slot_at_index
*       bcm_get_block_at_index
*       bcm_get_buffer_offset_at_index
*       bcm_add_to_list
*       bcm_remove_from_list 
*       bcm_allocate_slot 
*       bcm_free_slot 
*       bcm_is_block_in_cache
*       bcm_period_flush_task_entry 
*       bcm_do_replacement
*       bcm_convert_path_to_cb 
*       bcm_process_threshold
*       bcm_report_error
*       bcm_flush_cache
*       bcm_obtain_dev_layer_semaphores
*       bcm_release_dev_layer_semaphores
*
*       bcm_block_cache_feature_init
*       bcm_device_has_cache                  
*       bcm_device_read_request
*       bcm_device_write_request
*       NU_FILE_Cache_Create
*       NU_FILE_Cache_Destroy
*       NU_FILE_Cache_Get_Config
*       NU_FILE_Cache_Set_Config
*       NU_FILE_Cache_Flush
*
* DEPENDENCIES
*  
*       "plus/nucleus.h"
*       "file/vfs/inc/pcdisk.h"
*       "file/vfs/inc/bcm_extr.h"
*       "file/vfs/inc/lck_extr.h"
*       "file/vfs/inc/dh_extr.h"
*       "file/vfs/inc/dev_extr.h"
*       "file/vfs/inc/fsl_extr.h"
*       "file/vfs/inc/part_defs.h"
*                                                                       
*************************************************************************/

#include "nucleus.h"

#include "storage/pcdisk.h"

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1)

#include "storage/bcm_extr.h"
#include "storage/lck_extr.h"
#include "storage/dh_extr.h"
#include "storage/dev_extr.h"
#include "storage/fsl_extr.h"

/* Access control semaphore for control block list operations */
NU_SEMAPHORE BCM_List_Sema;    
/* Pointer to control block list */
BCM_CB *BCM_CB_List;

/* Error queue and data area for reporting async IO errors */
NU_QUEUE    NU_FILE_Cache_Error_Queue;
UNSIGNED    NU_FILE_Cache_Error_Size;
VOID        *BCM_Cache_Errors;

/* Local prototypes */
STATIC STATUS bcm_get_slot_at_index(BCM_SLOT_SET_CB *ss_cb, BCM_SLOT slots[], 
                             BCM_SLOT **ret_slot, UNSIGNED index );
STATIC STATUS bcm_get_ole_at_index(BCM_OLE oles[], BCM_OLE **ret_ole, UNSIGNED index, 
                             UNSIGNED num_members );
STATIC STATUS bcm_get_block_at_index(BCM_SLOT_SET_CB *ss_cb, UINT8 blocks[], 
                               UINT8 **ret_block, UNSIGNED block_index );  
STATIC VOID   bcm_get_buffer_offset_at_index(UINT8 blocks[], UNSIGNED block_size, 
                               UINT8 **ret_offset, UNSIGNED block_index);                                                          
STATIC STATUS bcm_add_to_list(BCM_OL_CB *ol_cb, BCM_OLE *ole);

STATIC STATUS bcm_remove_from_list(BCM_OL_CB *ol_cb, BCM_OLE *ole);
STATIC STATUS bcm_allocate_slot(BCM_SLOT_SET_CB *ss_cb, BCM_SLOT **ret_slot, 
                                 UNSIGNED *ret_slot_idx);
STATIC STATUS bcm_free_slot(BCM_SLOT_SET_CB *ss_cb, UNSIGNED slot_idx);
STATIC STATUS bcm_is_block_in_cache(BCM_CB *bc_cb, UINT32 block_number, UNSIGNED *ret_idx, 
                             BCM_SLOT **ret_slot, BCM_OLE **ret_ole);
STATIC VOID   bcm_period_flush_task_entry(UNSIGNED, VOID *);
STATIC STATUS bcm_do_replacement( BCM_CB *bc_cb);
STATIC STATUS bcm_convert_path_to_cb(CHAR *path, BCM_CB **cb);
STATIC STATUS bcm_process_threshold(BCM_CB *cb);
STATIC STATUS bcm_flush_cache(BCM_CB *cb);
STATIC STATUS bcm_obtain_dev_layer_semaphores(CHAR *path);
STATIC STATUS bcm_release_dev_layer_semaphores(CHAR *path);
STATIC STATUS bcm_do_IO(BCM_CB *cb, UINT32 sector, VOID *buffer, INT reading); 

extern FSDH_S  FS_Disk_Handles[FSDH_MAX_DISKS];

/************************************************************************
* FUNCTION
*
*       bcm_block_cache_feature_init   
*
* DESCRIPTION
*
*       Perform initialization of the device layer block cache component.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NU_INVALID_SEMAPHORE    Semaphore is in use
*       NU_INVALID_QUEUE        Queue is in use
*       NU_INVALID_SUSPEND      Suspend type is invalid
*       NU_NO_MEMORY            Insufficient memory
*
*************************************************************************/
STATUS bcm_block_cache_feature_init(VOID)
{
    STATUS      ret_status;
    UNSIGNED    element_size; /* Number of UNSIGNED */
      
    /* Setup the feature global data structures */
    BCM_CB_List = NU_NULL;
    
    /* Create a semaphore for controlling access to the control block list */ 
    ret_status = NU_Create_Semaphore(&BCM_List_Sema, "BCMLIST", 1, NU_PRIORITY);
    
    if (ret_status == NU_SUCCESS)
    {
        element_size = (sizeof(NU_FILE_CACHE_ERROR) / sizeof(UNSIGNED));
        
        /* Setup the global for use in the error logging */
        NU_FILE_Cache_Error_Size = element_size;
        
        BCM_Cache_Errors = NUF_Alloc((element_size*BCM_BLOCK_CACHE_ERR_LOG_SIZE*sizeof(UNSIGNED)));
        if (BCM_Cache_Errors == NU_NULL)
        {
            /* Set proper return value and delete the semaphore */
            ret_status = NU_NO_MEMORY;
            (VOID) NU_Delete_Semaphore(&BCM_List_Sema);
        }
        else
        {
            /* Clear out the memory used for the queue */
            NUF_Memfill( BCM_Cache_Errors, 
                         (element_size*BCM_BLOCK_CACHE_ERR_LOG_SIZE*sizeof(UNSIGNED)), 0); 
            
            /* Create the error reporting queue */
            ret_status = NU_Create_Queue(&NU_FILE_Cache_Error_Queue, "BCM_ERR", 
                                          BCM_Cache_Errors, 
                                          (element_size*BCM_BLOCK_CACHE_ERR_LOG_SIZE),
                                          NU_FIXED_SIZE,
                                          element_size,
                                          NU_PRIORITY);
            if (ret_status != NU_SUCCESS)
            {
                /* Return the memory and destroy the list semaphore */
                (VOID) NU_Deallocate_Memory(BCM_Cache_Errors);
                (VOID) NU_Delete_Semaphore(&BCM_List_Sema);
            }
        }            
    }                             
    
    return (ret_status);
}

/************************************************************************
* FUNCTION
*
*       bcm_get_slot_at_index
*
* DESCRIPTION
*
*       Return the slot pointer for the given index. Access control for
*       the cache control block is assumed to be managed by the calling
*       routine. 
*             
*
* INPUTS
*   
*       *slot_cb           Slot set control block
*       slots[]            Array of slots
*       **ret_slot         Pointer to the address of the returned slot
*       index              Index in to array of slots to retrieve
*
* OUTPUTS
*
*       NU_SUCCESS         Operation is successful
*       NU_BADPARM         Bad parameter 
*
*************************************************************************/
STATIC STATUS bcm_get_slot_at_index(BCM_SLOT_SET_CB *ss_cb, BCM_SLOT slots[], 
                             BCM_SLOT **ret_slot, UNSIGNED index )
{
    STATUS ret_status;

    /* Validate input pointers are non-null */
    if (ss_cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else if (ret_slot == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else 
    {
        ret_status = NU_SUCCESS;
    }

    /* Validate index is within valid range */
    if (ret_status == NU_SUCCESS)
    {
        if ( index < ss_cb->num_members)
        {
            *ret_slot = &slots[index];
        }
        else
        {
            ret_status = NUF_BADPARM;
        }
    }        

    return (ret_status);
}

/************************************************************************
* FUNCTION
*
*       bcm_get_ole_at_index
*
* DESCRIPTION
*
*       Return the ordered list entry pointer for the given index. Access 
*       control for the cache control block is assumed to be managed by 
*       the calling routine. 
*             
*
* INPUTS
*   
*       oles[]             Array of ordered list entries
*       **ret_ole          Pointer to the address of the returned slot
*       index              Index in to array of OLEs to retrieve
*       num_member         Number of items in the array
*
* OUTPUTS
*
*       NU_SUCCESS         Operation is successful
*       NU_BADPARM         Bad parameter 
*
*************************************************************************/
STATIC STATUS bcm_get_ole_at_index(BCM_OLE oles[], BCM_OLE **ret_ole, UNSIGNED index, 
                             UNSIGNED num_members )
{
    STATUS ret_status;

    /* Validate input pointers are non-null */
    if (ret_ole == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else 
    {
        ret_status = NU_SUCCESS;
    }

    /* Validate index is within valid range */
    if (ret_status == NU_SUCCESS)
    {    
        if ( index < num_members)
        {
            *ret_ole = &oles[index];
        }
        else
        {
            ret_status = NUF_BADPARM;
        }
    }        

    return (ret_status);
}

/************************************************************************
* FUNCTION
*
*       bcm_get_block_at_index
*
* DESCRIPTION
*
*       Return the block address for the requested index. 
*             
*
* INPUTS
*   
*       ss_cb              Slot set control block
*       blocks[]           Array of blocks
*       **ret_block        Pointer to the address of the returned block
*       block_index        Index in to array of ss_cb->block_size blocks to retrieve
*
* OUTPUTS
*
*       NU_SUCCESS         Operation is successful
*       NU_BADPARM         Bad parameter 
*
*************************************************************************/
STATIC STATUS bcm_get_block_at_index(BCM_SLOT_SET_CB *ss_cb, UINT8 blocks[], 
                               UINT8 **ret_block, UNSIGNED block_index )
{
    STATUS ret_status;
    UNSIGNED byte_index;

    /* Validate input pointers are non-null */
    if (ret_block == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else if (ss_cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else 
    {
        ret_status = NU_SUCCESS;
    }

    /* Validate index is within valid range */
    if (ret_status == NU_SUCCESS)
    {
        if ( block_index < ss_cb->num_members)
        {
            byte_index = block_index * ss_cb->block_size;
            *ret_block = &blocks[byte_index];        
        }
        else
        {
            ret_status = NUF_BADPARM;
        }
    }        

    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       bcm_get_buffer_offset_at_index       
*
* DESCRIPTION
*
*       Return the buffer offset given the block offset and block size.  
*             
*
* INPUTS
*   
*       blocks[]        Array of bytes forming blocks
*       block_size      Number of bytes in a single block
*       **ret_offset    Pointer to store return offset 
*       block_index     Index in to blocks[] to return
*       
* OUTPUTS
*
*       None 
*
*************************************************************************/
STATIC VOID bcm_get_buffer_offset_at_index(UINT8 blocks[], UNSIGNED block_size, 
                                           UINT8 **ret_offset, UNSIGNED block_index)
{
    UNSIGNED byte_index;
    
    if (ret_offset != NU_NULL)
    {
        byte_index = block_index * block_size;
        *ret_offset = &blocks[byte_index];            
    }
}

/************************************************************************
* FUNCTION
*
*       bcm_add_to_list
*
* DESCRIPTION
*
*       Add an item to the ordered list, always added as MRU      
*
* INPUTS
*   
*       *ol_cb      Pointer to the ordered list control block
*       *ole        Pointer to the list entry to be added
*
* OUTPUTS
*
*       NU_SUCCESS         Operation is successful
*       NU_BADPARM         Bad parameter, either the control block
*                          or the list entry points to null
*
*************************************************************************/
STATIC STATUS bcm_add_to_list(BCM_OL_CB *ol_cb, BCM_OLE *ole)
{
    BCM_OLE *current_mru;
    STATUS ret_status;
    
    /* Verify input parameters */
    if (ol_cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else if (ole == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_SUCCESS;
    }
    
    if (ret_status == NU_SUCCESS)
    {
        /* Get the current MRU */
        current_mru = ol_cb->bcm_mru;
        
        /* If the list is empty, add as LRU */
        if (current_mru == NU_NULL)
        {
            ol_cb->bcm_lru = ole;
            ole->next = NU_NULL;
            ole->prev = NU_NULL;
        }
        else
        {
            /* List is not empty, fixup for new MRU */
            current_mru->next = ole;
            ole->prev = current_mru;
            ole->next = NU_NULL;           
        }

        /* Set the new MRU */
        ol_cb->bcm_mru = ole;
    }
    
    return (ret_status);
    
    
}
/************************************************************************
* FUNCTION
*
*       bcm_remove_from_list      
*
* DESCRIPTION
*
*       Remove an item from the ordered list
*
* INPUTS
*
*       *ol_cb      Pointer to the ordered list control block
*       *ole        Pointer to the list entry to be removed
*
* OUTPUTS
*
*       NU_SUCCESS         Operation is successful
*       NU_BADPARM         Bad parameter, either the control block
*                           or the list entry points to null
*
*************************************************************************/
STATIC STATUS bcm_remove_from_list(BCM_OL_CB *ol_cb, BCM_OLE *ole)
{
    STATUS ret_status;
    BCM_OLE *tempOle;

    /* Verify input parameters */
    if (ol_cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else if (ole == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    /* Verify we have a good list, must not be empty to remove */
    else if ((ol_cb->bcm_lru == NU_NULL) || (ol_cb->bcm_mru == NU_NULL))
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_SUCCESS;
    }
    
    /* List is not empty, parameters are valid */
    if (ret_status == NU_SUCCESS)
    {
        /* There is only one item in the list, and it is a match */
        if ((ol_cb->bcm_lru == ol_cb->bcm_mru) &&
            (ol_cb->bcm_lru == ole))
        {
            ole->next = NU_NULL;
            ole->prev = NU_NULL;
            ol_cb->bcm_lru = NU_NULL;
            ol_cb->bcm_mru = NU_NULL;
        }
        else 
        {
            if (ol_cb->bcm_mru == ole) /* item is mru */
            {
                ol_cb->bcm_mru = ole->prev; /* This can safely assign null */
                ole->prev->next = ole->next;
                ole->prev = NU_NULL;
            }
            else if (ol_cb->bcm_lru == ole) /* item is lru */
            {
                ol_cb->bcm_lru = ole->next; /* This can safely assign null */
                ole->next->prev = ole->prev;
                ole->next = NU_NULL;   
            }
            else /* item is not mru or lru, verify it is in the list */
            {
                /* Assume failure */
                ret_status = NUF_BADPARM;
                tempOle = ol_cb->bcm_mru;
                while (tempOle != NU_NULL)
                {
                    if (tempOle == ole)
                    {
                        ret_status = NU_SUCCESS;
                        ole->prev->next = ole->next;
                        ole->next->prev = ole->prev;
                        break; /* exit the search loop */
                    }
                    /* Didn't find it, go to previous item in the list */
                    tempOle = tempOle->prev;
               }
            }
        }
    }
    
    /* If successful, reset the pointer for the item */
    if (ret_status == NU_SUCCESS)
    {
        ole->next = NU_NULL;
        ole->prev = NU_NULL;
    }
        
    return (ret_status);
}


/************************************************************************
* FUNCTION
*
*       bcm_allocate_slot         
*
* DESCRIPTION
*
*       Allocate a slot from the slot set. Returns a pointer to slot if able
*       to allocate. Slot is marked occupied by this function.   
*
* INPUTS
*
*       *ss_cb              Slot set control block to allocate from.
*       **slot              Pointer to slot address if alloc was successful
*
* OUTPUTS
*
*       NU_SUCCESS          Operation is successful
*       NUF_BADPARM         Bad parameter, null control block, null pointer
*       NU_UNAVAILABLE      All slots are occupied 
*
*************************************************************************/
STATIC STATUS bcm_allocate_slot(BCM_SLOT_SET_CB *ss_cb, BCM_SLOT **ret_slot, 
                                 UNSIGNED *ret_slot_idx)
{
    STATUS   ret_status;
    UNSIGNED idx;
    BCM_SLOT *slot = NU_NULL;
    
    if (ss_cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else if (ret_slot == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else if (ret_slot_idx == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_SUCCESS;
    }
    
    if (ret_status == NU_SUCCESS)
    {
        if (ss_cb->empty_count > 0)
        {
            /* Verify slot set is valid */
            if (ss_cb->slot_set == NU_NULL)
            {
                ret_status = NUF_BADPARM;
            }
            else
            {
                /* Assume failure in case the empty count and
                   contents don't actually match */
                ret_status = NU_UNAVAILABLE;
                /* For each item in the slot set, check if empty */
                for (idx = 0; idx < ss_cb->num_members; idx++)
                {
                    /* Ignoring return value because we are checking the index
                       as part of the for loop, control block and set have already
                       been verified */
                    (VOID) bcm_get_slot_at_index(ss_cb, ss_cb->slot_set, &slot, idx);
                    
                    /* Check if slot is empty */
                    if (slot->slot_state == BCM_SLOT_STATE_EMPTY)
                    {
                        /* Set new state and clean slot */
                        slot->slot_state = BCM_SLOT_STATE_OCCUPIED;
                        slot->block_number = BCM_UNINITIALIZED_BLOCK;
                        slot->block_state = BCM_BLOCK_STATE_CLEAN;
                        slot->error_pending = NU_FALSE;
                        
                        /* Set return values */
                        *ret_slot = slot;
                        *ret_slot_idx = idx; 
                        
                        /* Update the slot set control block */
                        ss_cb->empty_count--;

                        /* Update the return status */
                        ret_status = NU_SUCCESS;
                        /* Stop the search */
                        break;   
                    }
                }
            }
        }
        else
        {
            /* There are no free slots available */
            ret_status = NU_UNAVAILABLE;
        }
    }

    return (ret_status);
}


/************************************************************************
* FUNCTION
*
*       bcm_free_slot         
*
* DESCRIPTION
*
*       Free the given slot index from the slot control block
*
* INPUTS
*
*       *ss_cb          Slot set control block pointer
*       slot_idx        Index in to the slot set of item to be freed
*
* OUTPUTS
*
*       NU_SUCCESS      Operation is successful
*       NUF_BADPARM     Invalid parameter given
*
*************************************************************************/
STATIC STATUS bcm_free_slot(BCM_SLOT_SET_CB *ss_cb, UNSIGNED slot_idx)
{
    STATUS ret_status;
    BCM_SLOT *slot;

    /* Verify the input parameters */
    if (ss_cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_SUCCESS;
    }
    
    if (ret_status == NU_SUCCESS)
    {
        if (slot_idx < ss_cb->num_members)
        {
            if (ss_cb->slot_set != NU_NULL)
            {
                /* Ignore the return value, control block has been verified, 
                   slot set exists, and index is already validated */
                (VOID) bcm_get_slot_at_index(ss_cb, ss_cb->slot_set, &slot, slot_idx);
                
                /* Reset the slot state to empty */
                slot->slot_state = BCM_SLOT_STATE_EMPTY;
                
                /* Update the slot set control block */
                ss_cb->empty_count++;
               
            }
            else
            {
                /* The slot set was never set up */
                ret_status = NUF_BADPARM;
            }
        }
        else
        {
            /* The given slot index is outside the range of the control block */
            ret_status = NUF_BADPARM;
        }
    }

    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       bcm_is_block_in_cache         
*
* DESCRIPTION
*
*       See if the give block number is in the cache, returns the index and slot
*       set pointer if item is in the cache
*
* INPUTS
*
*       *bc_cb              Block cache control block
*       block_number        The block number to search for
*       *ret_idx            Slot index valid if found
*       **ret_slot          Pointer to the address of the matching slot
*       **ret_ole           List entry for returned object
*
* OUTPUTS
*
*       NU_SUCCESS          Operation is successful
*       NUF_BADPARM         Invalid parameter given, null pointer, uninitialized
*       NU_UNAVAILABLE      Item was not found in the cache
*
*************************************************************************/
STATIC STATUS bcm_is_block_in_cache(BCM_CB *bc_cb, UINT32 block_number, UNSIGNED *ret_idx, 
                                    BCM_SLOT **ret_slot, BCM_OLE **ret_ole)
{
    STATUS          ret_status;
    UNSIGNED        idx;
    BCM_OL_CB       *ol_cb;
    BCM_SLOT_SET_CB *ss_cb;
    BCM_SLOT        *slot = NU_NULL;
    
    /* Verify input parameters */
    if (bc_cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else if (ret_idx == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else if (ret_slot == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else if (ret_ole == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_SUCCESS;
    }
    
    if (ret_status == NU_SUCCESS)
    {
        ol_cb = &(bc_cb->ol_cb);
        ss_cb = &(bc_cb->ss_cb);

        /* Assume no match */
        ret_status = NU_UNAVAILABLE;
      
        if (ss_cb->slot_set == NU_NULL)
        {
            ret_status = NUF_BADPARM;
        }
        else
        {
            /* For each member in the set, check if it matches */
            for (idx = 0; idx < ss_cb->num_members; idx++)
            {
                /* Get the slot at index, ignoring return value as input 
                   parameters have already been verified */
                (VOID) bcm_get_slot_at_index(ss_cb, ss_cb->slot_set, &slot, idx);
                
                /* Check slot state to see if contents are valid */
                if (slot->slot_state == BCM_SLOT_STATE_OCCUPIED)
                {
                    /* Check if cached block matches request */
                    if (slot->block_number == block_number)
                    {
                        /* Setup the return values */
                        *ret_slot = slot;
                        *ret_idx = idx;
                        
                        /* Get the OLE, input parameters have been verified */
                        (VOID) bcm_get_ole_at_index(ol_cb->core, ret_ole, idx, ss_cb->num_members);
                        
                        ret_status = NU_SUCCESS;
                        /* Break from the search */
                        break;
                    }
                }
            }
        }
    }

    return (ret_status);    
}
/************************************************************************
* FUNCTION
*
*       bcm_period_flush_task_entry         
*
* DESCRIPTION
*
*       Task entry point for managing the periodic flush
*
* INPUTS
*
*       argc            Number of arguments in argv
*       *argv           Pointer to cache control block        
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
STATIC VOID bcm_period_flush_task_entry(UNSIGNED argc, VOID *argv)
{
    STATUS evg_status;
    STATUS lock_status;
    UNSIGNED received_events;
    UNSIGNED requested_events;
    UNSIGNED suspension;
    BCM_CB *bc_cb;
    
    if ((argv != NU_NULL) && (argc == 1))
    {
        /* Pick up our control block */
        bc_cb = (BCM_CB*)argv;
    
        /* Set up for the main processing loop */
        suspension = NU_SUSPEND;
        requested_events = (BCM_EVG_TIMEOUT_MODIFIED | 
                            BCM_EVG_TIMEOUT_START | 
                            BCM_EVG_ASYNC_FLUSH);
    
        while(1)
        {
            received_events = 0;
            
            /* Wait on an event or timeout */
            evg_status = NU_Retrieve_Events(&(bc_cb->control_group), requested_events,
                                        NU_OR_CONSUME, &received_events, suspension);
            if (evg_status == NU_GROUP_DELETED)
            {
                /* Exit the main processing loop */
                break;                             
            }

            if (FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(bc_cb->dh)].fsdh_sema != NU_NULL)
            {
                /* Get the device supplied semaphore */
                (VOID) NU_Obtain_Semaphore(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(bc_cb->dh)].fsdh_sema, NU_SUSPEND);    
            }                

            /* Get the device layer device specific semaphore */
            (VOID) NU_Obtain_Semaphore(&(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(bc_cb->dh)].fsdh_vfs_sema), NU_SUSPEND);    

            (VOID) NU_Obtain_Semaphore(&BCM_List_Sema, NU_SUSPEND);

            /* Lock the control block */
            lock_status = NU_Obtain_Semaphore(&(bc_cb->cb_sema), NU_SUSPEND);
            if (lock_status == NU_SUCCESS)
            {
                (VOID) NU_Release_Semaphore(&BCM_List_Sema);
                
                if ( evg_status == NU_TIMEOUT)
                { 
                    /* Timer expired. Process the cache for dirty blocks */
                    /* Return is ignored. Errors are sent to log queue */
                    (VOID) bcm_flush_cache(bc_cb);

                    /* Disable the timer */
                    suspension = NU_SUSPEND;
                }
                else if (evg_status == NU_SUCCESS)
                {
                    /* We received an event. Start event processing */
                    if ((received_events & BCM_EVG_TIMEOUT_MODIFIED) > 0)
                    {
                        /* Set the new timeout period if there is a dirty block. 
                           We will receive an EVG_TIMEOUT_START when a dirty block
                           enters the cache and we start processing the cache using the
                           new timer value */
                        if (bc_cb->dirty_count > 0)
                        {
                            suspension = bc_cb->periodic_flush;
                        }                        
                    }
                    
                    if ((received_events & BCM_EVG_TIMEOUT_START) > 0)
                    {
                        /* A dirty block entered the cache */
                        /* Set the timeout period */
                        suspension = bc_cb->periodic_flush;                        
                    }
                    
                    if ((received_events & BCM_EVG_ASYNC_FLUSH) > 0)
                    {
                        /* Perform an async flush */
                        /* Return is ignored as errors are reported to user
                           using the error log */
                        (VOID) bcm_flush_cache(bc_cb);
                    }
                }
                else
                {
                    /* No further processing of evg_status */                    
                }
 
                /* Release the lock on the control block */
                (VOID) NU_Release_Semaphore(&(bc_cb->cb_sema));
            }
            else
            {
                (VOID) NU_Release_Semaphore(&BCM_List_Sema);
            }
            
            /* Release the device layer device specific lock */
            (VOID) NU_Release_Semaphore(&(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(bc_cb->dh)].fsdh_vfs_sema));

            if (FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(bc_cb->dh)].fsdh_sema != NU_NULL)
            {
                /* Get the device supplied semaphore */
                (VOID) NU_Release_Semaphore(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(bc_cb->dh)].fsdh_sema);    
            }                
            
            if (lock_status != NU_SUCCESS)
            {
                /* Exit the main processing loop, list semaphore
                   acquire should never fail. Cache could be
                   shutting down. */
                break;
            }                        
        }
    }
    
    return;   
}
/************************************************************************
* FUNCTION
*
*       NU_FILE_Cache_Create         
*
* DESCRIPTION
*
*       Creates a device layer block cache for the given mounted device
*       using the given configuration parameters   
*
* INPUTS
*
*       *path                   Path of device to create cache for "A:"
*       cache_type              Encoded cache type to create
*       *cache_config           Configuration settings for cache type
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NU_INVALID_SEMAPHORE    List semaphore was deleted
*       NUF_BADDRIVE            Drive is not mounted
*       NU_UNAVAILABLE          Cache type is not available
*       NU_NO_MEMORY            Insufficient memory
*
*************************************************************************/
STATUS NU_FILE_Cache_Create(CHAR *path, BCM_CACHE_TYPE cache_type, VOID *cache_config)
{
    STATUS  ret_status;
    BCM_CB  *bc_cb;
    BCM_DL_LRU_CONFIG *dl_lru_config = NU_NULL;
    UNSIGNED block_size = 0;
    UNSIGNED num_blocks = 0;
    UNSIGNED periodic_flush_time = 0; 
    UNSIGNED low_threshold = 0; 
    UNSIGNED high_threshold = 0;
    UINT16  dh = 0;    

    LCK_FS_ENTER()
    
    /* Verify input parameters */
    if (cache_type != BCM_CACHE_TYPE_DL_LRU)
    {
        ret_status = NU_UNAVAILABLE;
    }
    else if (cache_config == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_SUCCESS;
    }

    /* Setup the DL LRU cache config parameters */
    if (ret_status == NU_SUCCESS)
    {
        dl_lru_config = (BCM_DL_LRU_CONFIG *)cache_config;        
        low_threshold = dl_lru_config->low_threshold;
        high_threshold = dl_lru_config->high_threshold;
        periodic_flush_time = dl_lru_config->periodic_flush;
        
        /* Verify the input parameters */
        if (low_threshold > high_threshold)
        {
            ret_status = NUF_BADPARM;
        }
        else if (path == NU_NULL)
        {
            ret_status = NUF_BADPARM;
        }
        else
        {
            /* No change in ret_status */
        }
    }


    /* Convert the input path to a disk handle and mte */
    if (ret_status == NU_SUCCESS)
    {
        MTE_S *mte;

        /* Lock the mount table */
        LCK_ENTER(LCK_MT_TABLE)        


        /* Resolve mount table entry from path */
        mte = fsl_mte_from_fqpath(path);
        if(mte != NU_NULL)
        {        
            /* Determine the physical device's block size by starting with the 
               logical device's disk handle */
            FPART_DISK_INFO_S disk_info;
            UINT16            phys_dh;
            CHAR              dev_name[FILE_MAX_DEVICE_NAME];
                
            /* Get the disk handle */
            dh = mte->mte_dh;
            
            /* Get the logical device name */
            ret_status = fsdh_dh_to_devname(dh, &dev_name[0]);
            if (ret_status == NU_SUCCESS)
            {
                /* Transform the logical device name to the physical name */
                NUF_Memfill((VOID*)&dev_name[FPART_MAX_PHYS_NAME-1], (FILE_MAX_DEVICE_NAME-FPART_MAX_PHYS_NAME), 
                            '\0');
                /* Get the physical disk handle */            
                ret_status = fsdh_devname_to_dh(&dev_name[0], &phys_dh);
                if (ret_status == NU_SUCCESS)
                {            
                    /* Request the disk info from the device. */
                    ret_status = fs_dev_ioctl_proc(phys_dh, FDEV_GET_DISK_INFO, (VOID*)&disk_info, sizeof(FPART_DISK_INFO_S));
                }
            }
            
            if (ret_status == NU_SUCCESS)
            {
                block_size = disk_info.fpart_bytes_p_sec;
                /* Verify a valid block size in case a device driver blindly returns
                   the disk_info */
                if ((block_size > BCM_BLOCK_SIZE_4K) || (block_size < BCM_BLOCK_SIZE_512B))
                {
                    ret_status = NUF_BADDRIVE;
                }
                else
                {
                    num_blocks = dl_lru_config->size / block_size;                    
                }
            }
        }
        else /* No matching drive mounted */
        {
            ret_status = NUF_BADDRIVE;
        }
        
        /* Unlock the mount table */
        LCK_EXIT(LCK_MT_TABLE)    
    }
    
    /* Verify the device does not already have an active cache */
    if (ret_status == NU_SUCCESS)
    {
        
        ret_status = NU_Obtain_Semaphore(&BCM_List_Sema, NU_SUSPEND);
        if (ret_status == NU_SUCCESS)
        {
            /* Device must not have an active cache */
            ret_status = bcm_device_has_cache(dh, &bc_cb);
            if (ret_status == NU_SUCCESS)
            {
                ret_status = NU_UNAVAILABLE;
            }
            else
            {
                ret_status = NU_SUCCESS;
            }
                
            (VOID) NU_Release_Semaphore(&BCM_List_Sema);
        }
    }
    
    
    if (ret_status == NU_SUCCESS)
    {
        /* Allocate memory for control structures */
        bc_cb = NUF_Alloc(sizeof(BCM_CB));
        if (bc_cb != NU_NULL)
        {
            /* Clear the control block memory. The fact that members are zero'd will
               be used later for cleanup in case of failure during init */
            NUF_Memfill((VOID*)bc_cb, sizeof(BCM_CB), 0);
            
            /* Allocate a task stack */
            bc_cb->task_stack = NUF_Alloc(BCM_PERIODIC_FLUSH_TASK_STACK_SIZE);

            if (bc_cb->task_stack != NU_NULL)
            {
                /* Create a task */
                ret_status = NU_Create_Task(&(bc_cb->control_task),"BCMTSK", 
                                bcm_period_flush_task_entry, 1, (VOID*)bc_cb,
                                bc_cb->task_stack, 
                                BCM_PERIODIC_FLUSH_TASK_STACK_SIZE,
                                BCM_PERIODIC_FLUSH_TASK_PRIORITY,
                                BCM_PERIODIC_FLUSH_TASK_TIME_SLICE,
                                BCM_PERIODIC_FLUSH_TASK_PREEMPT,
                                NU_NO_START);

                /* Create an event group */
                if (ret_status == NU_SUCCESS)
                {
                    ret_status = NU_Create_Event_Group(&(bc_cb->control_group),"BCMEVG");
                    if (ret_status == NU_SUCCESS)
                    {
                        ret_status = NU_Create_Semaphore(&(bc_cb->cb_sema),
                                                         "BCMCB", 
                                                         1, 
                                                         NU_PRIORITY);
                        if (ret_status != NU_SUCCESS)
                        {
                            /* Remove the event group */
                            (VOID) NU_Delete_Event_Group(&(bc_cb->control_group));
                            /* Destroy the task. */
                            (VOID) NU_Terminate_Task(&(bc_cb->control_task));
                            (VOID) NU_Delete_Task(&(bc_cb->control_task));
                            (VOID) NU_Deallocate_Memory(&(bc_cb->control_task));
                            (VOID) NU_Deallocate_Memory((VOID*)bc_cb);                        
                        }
                    }
                    else /* Failed to create event group */
                    {
                        /* Destroy the task. */
                        (VOID) NU_Terminate_Task(&(bc_cb->control_task));
                        (VOID) NU_Delete_Task(&(bc_cb->control_task));
                        (VOID) NU_Deallocate_Memory(&(bc_cb->control_task));
                        (VOID) NU_Deallocate_Memory((VOID*)bc_cb);                        
                    }
                } /* End create event group, semaphore */
                else
                {
                    /* Failed to create task */    
                    (VOID) NU_Deallocate_Memory(&(bc_cb->control_task));
                    (VOID) NU_Deallocate_Memory((VOID*)bc_cb);
                }
            } 
            else
            {
                /* Task stack failed to allocate */
                ret_status = NU_NO_MEMORY;
                (VOID) NU_Deallocate_Memory((VOID*)bc_cb);
            } /* end allocate task stack */
        }
        else
        {
            /* Control block memory alloc failed */
            ret_status = NU_NO_MEMORY;
        } /* end allocate control block */
    }    
    
    /* Failures during creation of OS primitives and memory allocations
       prior to this point have been handled. Any further allocation failures
       must manage cleaning up the previously created objects */
    if (ret_status == NU_SUCCESS)
    {       
        /* Allocate memory for the slots */
        bc_cb->ss_cb.slot_set = NUF_Alloc(sizeof(BCM_SLOT)*num_blocks);
        if (bc_cb->ss_cb.slot_set != NU_NULL)
        {
            /* Zero out the slot set */
            NUF_Memfill((VOID*)bc_cb->ss_cb.slot_set, sizeof(BCM_SLOT)*num_blocks,0);
            
            /* Allocate memory for the cached blocks */
            bc_cb->ss_cb.block_set = NUF_Alloc(block_size*num_blocks);
            if (bc_cb->ss_cb.block_set != NU_NULL)
            {
                /* Allocate memory for the ordered list entries */
                bc_cb->ol_cb.core = NUF_Alloc(sizeof(BCM_OLE)*num_blocks);
                if (bc_cb->ol_cb.core != NU_NULL)
                {
                    /* Zero out the memory for the ordered list */
                    NUF_Memfill((VOID*)bc_cb->ol_cb.core,sizeof(BCM_OLE)*num_blocks,0);
                }
                else
                {
                    ret_status = NU_NO_MEMORY;
                }    
            }
            else
            {
                ret_status = NU_NO_MEMORY;
            }    
        }
        else
        {
            ret_status = NU_NO_MEMORY;
        }
       
        /* Setup control block parameters */
        if (ret_status == NU_SUCCESS)
        {
            /* Input parameters */
            bc_cb->dh = dh;
            bc_cb->low_threshold = low_threshold;
            bc_cb->high_threshold = high_threshold;
            bc_cb->dirty_count = 0;
            bc_cb->periodic_flush = periodic_flush_time;
            
            /* Initialize the sizes */
            bc_cb->ss_cb.num_members = num_blocks;
            bc_cb->ss_cb.empty_count = num_blocks;
            bc_cb->ss_cb.block_size = block_size;
      
            /* Set the ordered list to empty */
            bc_cb->ol_cb.bcm_lru = NU_NULL;
            bc_cb->ol_cb.bcm_mru = NU_NULL;            
        }
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE_REORDER == 1)
        if (ret_status == NU_SUCCESS)
        {
            ret_status = bcmms_create(dh, BCMMS_BUFFER_BLOCK_COUNT, block_size);
            if (ret_status == NU_SUCCESS)
            {
                BCMMS_CB *bcmms_cb;
                 /* We know a cache exists, just getting the control block pointer */
                (VOID) bcmms_device_has_cache(dh, &bcmms_cb);
                /* Setup the multi-sector control block */                
                bc_cb->bcmms_cb = bcmms_cb;
            }
        }
#endif        
        /* Perform cleanup */
        if (ret_status != NU_SUCCESS)
        {
            /* Release the ordered list entries */
            if (bc_cb->ol_cb.core != NU_NULL)
            {
                (VOID) NU_Deallocate_Memory(bc_cb->ol_cb.core);
            }
            
            /* Release the cached blocks */
            if (bc_cb->ss_cb.block_set != NU_NULL)
            {
                (VOID) NU_Deallocate_Memory((VOID*)bc_cb->ss_cb.block_set);
            }
            
            /* Release the slots */
            if (bc_cb->ss_cb.slot_set != NU_NULL)
            {
                (VOID) NU_Deallocate_Memory((VOID*)bc_cb->ss_cb.slot_set);
            }
            
            /* The following objects have successfully been created
               from the previous allocation group */
            (VOID) NU_Delete_Semaphore(&(bc_cb->cb_sema));
            (VOID) NU_Delete_Event_Group(&(bc_cb->control_group));
            (VOID) NU_Terminate_Task(&(bc_cb->control_task));
            (VOID) NU_Delete_Task(&(bc_cb->control_task));
            if (bc_cb->task_stack != NU_NULL)
            {
                (VOID) NU_Deallocate_Memory((VOID*)bc_cb->task_stack);
            }                 
            (VOID) NU_Deallocate_Memory((VOID*)bc_cb);
            
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE_REORDER == 1)
            (VOID) bcmms_delete(dh);  /* Cleanup allocated resource during failure */
#endif                        
            
        }

        /* Add the control block to the control block list */
        if (ret_status == NU_SUCCESS)
        {
            /* Take the VFS device layer semaphore for the
               specific device we are about to add to the list */
            (VOID) bcm_obtain_dev_layer_semaphores(path);

            /* This should be the LAST thing we do. Once added to
               the list. The cache is available for use. */
            (VOID) NU_Obtain_Semaphore(&BCM_List_Sema, NU_SUSPEND);

            if (BCM_CB_List == NU_NULL)
            {
                BCM_CB_List = bc_cb;
            }
            else /* Insert it in the list */
            {
                bc_cb->next = BCM_CB_List;
                BCM_CB_List = bc_cb;
            }
            /* Release the list semaphore */
            (VOID) NU_Release_Semaphore(&BCM_List_Sema);
            
            (VOID) bcm_release_dev_layer_semaphores(path);
        }    
    }
    
    /* Start the periodic flush task */
    if (ret_status == NU_SUCCESS)
    {
        (VOID) NU_Resume_Task(&(bc_cb->control_task));
    }

    LCK_FS_EXIT()
 
    return (ret_status);
}

/************************************************************************
* FUNCTION
*
*       bcm_device_has_cache
*
* DESCRIPTION
*
*       If a device has a cache, return the control block pointer. Assumes the
*       access to the control block list is protected by the calling function.
*
* INPUTS
*
*       dh                  Disk handle
*       **bc_cb             Pointer to the address of the block cache control 
*                           block
*
* OUTPUTS
*
*       NU_SUCCESS          Device has a cache
*       NU_UNAVAILABLE      Device does not have a cache
*
*************************************************************************/
STATUS bcm_device_has_cache(UINT16 dh, BCM_CB **bc_cb)
{
    STATUS ret_status;
    BCM_CB *current;
    
    if (bc_cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        /* Setup for the search. Assume no match */
        current = BCM_CB_List;
        ret_status = NU_UNAVAILABLE;

        /* Search the list of cache control blocks for a matching disk handle */
        while ((current != NU_NULL) && (ret_status != NU_SUCCESS))
        {
            if (current->dh == dh)
            {
                /* We found a match */
                *bc_cb = current;
                ret_status = NU_SUCCESS;
            }
            else
            {
                current = current->next;
            }
        }
    }
    
    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       bcm_do_replacement
*
* DESCRIPTION
*
*       Perform replacement on cache freeing a block, IO errors are logged
*
* INPUTS
*
*       *bc_cb          Control block pointer
*
* OUTPUTS
*
*       NU_SUCCESS      Operation successful
*       NUF_BADPARM     Invalid control block pointer
*       NU_UNAVAILABLE  Unable to perform replacement, check error log
*
*************************************************************************/
STATIC STATUS bcm_do_replacement( BCM_CB *bc_cb)
{
    STATUS   ret_status;
    STATUS   io_status;
    BCM_SLOT *slot;
    BCM_OLE  *ole;
    UNSIGNED slot_idx;
    STATUS   slot_freed = NU_FALSE;
    
    
    /* Verify input parameter */
    if (bc_cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_SUCCESS;
    }
    
    if (ret_status == NU_SUCCESS)
    {
        /* Get the LRU */
        ole = bc_cb->ol_cb.bcm_lru;
        
        while ( (ole != NU_NULL) && (slot_freed == NU_FALSE))
        {
            slot_idx = ole->slot_idx;
            if (slot_idx < bc_cb->ss_cb.num_members)
            {
                slot = &(bc_cb->ss_cb.slot_set[slot_idx]);
                if (slot->block_state == BCM_BLOCK_STATE_CLEAN)
                {
                    /* The LRU is clean, remove it from the list */
                    (VOID) bcm_remove_from_list(&(bc_cb->ol_cb),ole);
                    (VOID) bcm_free_slot(&(bc_cb->ss_cb),slot_idx);

                    /* signal successful replacement */
                    slot_freed = NU_TRUE; 
                }
                else /* LRU is dirty, try to commit it to disk */
                {
                    UINT8 *buffer;

                    /* Get address of the buffer, ignoring return status as input
                       parameters have been verified */
                    (VOID) bcm_get_block_at_index(&(bc_cb->ss_cb), bc_cb->ss_cb.block_set, 
                                                   &buffer, slot_idx);
                   
                    /* Dispatch the IO request */
                    io_status = bcm_do_IO(bc_cb, slot->block_number, buffer, 0);
                    if (io_status == NU_SUCCESS)
                    {                                    
                        /* Assume this was successful */
                        (VOID) bcm_remove_from_list(&(bc_cb->ol_cb),ole);
                        (VOID) bcm_free_slot(&(bc_cb->ss_cb),slot_idx);     
                        
                        /* Decrement the dirty block count */
                        bc_cb->dirty_count--;
                        
                        /* signal successful replacement */
                        slot_freed = NU_TRUE; 
                    } 
                    else /* there was an IO error */
                    {
                        slot->error_pending++;
                        bcm_report_error(bc_cb->dh, slot->block_number, BCM_ERROR_OPERATION_WRITE, io_status);
                    }
                } /* End else LRU is dirty */               
            }
            
            /* Get the next block in the cache */
            ole = ole->next;
        } /* end While no slot free and still have LRU's */

        /* We freed a slot, IO error is reported to error log */
        if (slot_freed == NU_TRUE)
        {
            ret_status = NU_SUCCESS;
        }
        else
        {
            ret_status = NU_UNAVAILABLE;
        }
    }
    
    return (ret_status);
}


/************************************************************************
* FUNCTION
*
*       bcm_device_read_request         
*
* DESCRIPTION
*
*       Process a device read request. An IO for the requested block is 
*       returned to the caller. Asynchronous IO errors are sent to the
*       cache error log. Device locking is assumed to have been performed
*       by the calling VFS routine.
*
* INPUTS
*
*       dh                      Disk handle
*       sector                  Starting logical sector for the request
*       *buffer                 Pointer to storage location for returned data
*       count                   Number of sectors to read
*
* OUTPUTS
*
*       NU_SUCCESS              Operation successful
*       NUF_BADDRIVE            Invalid disk handle
*       NUF_BADPARM             Invalid parameter, buffer pointer is null
*       NUF_IO_ERROR            Device request returned an IO error for the
*                               requested block. 
*
*************************************************************************/
STATUS bcm_device_read_request( UINT16 dh, UINT32 sector, VOID *buffer, UINT16 count)
{
    STATUS ret_status;    
    BCM_CB *bc_cb = NU_NULL;
    INT    index = 0;

    /* Verify input parameters */
    FSDH_VERIFY_DH(dh, ret_status);
    
    if (ret_status == NU_SUCCESS)
    {
        if (buffer == NU_NULL)
        {
            ret_status = NUF_BADPARM;
        }
    }
    
    /* Determine if the disk has an active cache */
    if (ret_status == NU_SUCCESS)
    {
        /* Lock access to the list of control blocks */
        (VOID) NU_Obtain_Semaphore(&BCM_List_Sema, NU_SUSPEND);
         
        ret_status = bcm_device_has_cache(dh, &bc_cb);
        if (ret_status != NU_SUCCESS) /* Device does not have a cache */
        {
            /* Release the semaphore */
            (VOID) NU_Release_Semaphore(&BCM_List_Sema);
            
            index = FSDH_GET_IDX_FROM_DH(dh);

            if (index < FSDH_MAX_DISKS)
            {
                /* Device does not have cache, go directly to device */
                ret_status = FS_Disk_Handles[index].fsdh_fdev->fdev_ops.io_proc(dh,
                                 sector, buffer, count, 1);
            }
            else
            {
                ret_status = NUF_BADPARM;
            }
        }
        else /* The device has a cache present */
        {
            UNSIGNED  idx;          /* Index in the slot set of the matching sector */
            BCM_SLOT  *slot;        /* Pointer to the matching slot */
            BCM_OLE   *ole = NU_NULL;
            UINT16    io_idx = 0;   /* Count of IO operations performed */
            UINT8     *source = NU_NULL;
            UINT8     *dest;
            
            /* Get the control block lock */
            (VOID) NU_Obtain_Semaphore(&(bc_cb->cb_sema), NU_SUSPEND);
            /* Release the list semaphore */
            (VOID) NU_Release_Semaphore(&BCM_List_Sema);
            
            /* For each sector in the requested count starting at sector, attempt
               to read from cache or request from device */
            while ((io_idx < count) && (ret_status == NU_SUCCESS))
            {
                /* Setup the destination offset */
                bcm_get_buffer_offset_at_index((UINT8*)buffer, bc_cb->ss_cb.block_size, &dest, io_idx);
                
                /* Determine if the block is in cache */
                ret_status = bcm_is_block_in_cache(bc_cb, sector + io_idx, &idx, &slot, &ole);
                if (ret_status == NU_SUCCESS) /* Block is in the cache */
                {
                    /* Get the source address of the block in cache. Return is ignored as
                       the input parameters have been verified */
                    (VOID) bcm_get_block_at_index(&(bc_cb->ss_cb), bc_cb->ss_cb.block_set, 
                                                   &source, idx);
                    
                    /* Copy block data to the buffer */
                    NUF_Copybuff(dest, (VOID*)source, bc_cb->ss_cb.block_size);
    
                    /* Promote the block by removing it then adding it back */
                    (VOID) bcm_remove_from_list(&(bc_cb->ol_cb), ole);
                    (VOID) bcm_add_to_list(&(bc_cb->ol_cb), ole);
                }
                else /* Block is not in the cache */
                {
                    /* Dispatch the IO request */
                    ret_status = bcm_do_IO(bc_cb, sector + io_idx, dest, 1);                
                }
                
                /* Increment the block transfer count */
                io_idx++;
            }
            
            /* Release the control block lock */
            (VOID) NU_Release_Semaphore(&(bc_cb->cb_sema));                
        }
    }
    
    return (ret_status);
}

/************************************************************************
* FUNCTION
*
*       bcm_device_write_request         
*
* DESCRIPTION
*
*       Process a device write request. Handles all device IO
*       dispatch routines. Cached devices are processed according to the 
*       current configuration settings of the device. Non-cached devices
*       are dispatched using their standard IO routines. IO errors for
*       the requested block are returned to the called. Asynchronous IO
*       errors are sent to the cache error log. Device locking is assumed
*       to have been performed by the calling VFS routine.   
*
* INPUTS
*
*       dh                      Disk handle
*       sector                  Starting logical sector for the request
*       *buffer                 Pointer to data to write
*       count                   Number of sectors to write
*
* OUTPUTS
*
*       NU_SUCCESS              Operation successful
*       NUF_BADDRIVE            Invalid disk handle
*       NUF_BADPARM             Invalid parameter, buffer pointer is null
*       NUF_IO_ERROR            Device request returned an IO error 
*
*************************************************************************/
STATUS bcm_device_write_request(UINT16 dh, UINT32 sector, VOID *buffer, UINT16 count)
{
    STATUS ret_status;
    BCM_CB *bc_cb = NU_NULL;
    INT    index = 0;

    /* Verify input parameters */
    FSDH_VERIFY_DH(dh, ret_status);
    if (ret_status ==  NU_SUCCESS)
    {
        if (buffer == NU_NULL)
        {
            ret_status = NUF_BADPARM;
        }
    }
    
    /* Determine if the disk has an active cache */
    if (ret_status == NU_SUCCESS)
    {
        /* Lock access to the list of control blocks */
        (VOID) NU_Obtain_Semaphore(&BCM_List_Sema, NU_SUSPEND);
         
        ret_status = bcm_device_has_cache(dh, &bc_cb);
        if (ret_status != NU_SUCCESS) /* Device does not have a cache */
        {
            /* Release the semaphore */
            (VOID) NU_Release_Semaphore(&BCM_List_Sema);
    
            index = FSDH_GET_IDX_FROM_DH(dh);

            if (index < FSDH_MAX_DISKS)
            {

                /* Device does not have a cache, go directly to device */
                ret_status = FS_Disk_Handles[index].fsdh_fdev->fdev_ops.io_proc(dh,
                                 sector, buffer, count, 0);
            }
            else
            {
                ret_status = NUF_BADPARM;
            }
        }
        else /* The device has a cache present */
        {
            UNSIGNED  idx;              /* Index in the slot set of the matching sector */
            BCM_SLOT  *slot;            /* Pointer to the matching slot */
            BCM_OLE   *ole;
            UINT16    io_idx = 0;       /* Count of IO operations */
            UINT8     *dest = NU_NULL;
            UINT8     *source;
            
            /* Get the control block lock */
            (VOID) NU_Obtain_Semaphore(&(bc_cb->cb_sema), NU_SUSPEND);
            /* Release the list semaphore */
            (VOID) NU_Release_Semaphore(&BCM_List_Sema);
            
            /* For each sector requested in count, attempt to write to the cache */
            while ((io_idx < count) && (ret_status == NU_SUCCESS))
            {
                /* Init for each iteration */
                ole = NU_NULL;
                
                /* Setup the source address of the buffer */
                bcm_get_buffer_offset_at_index((UINT8*)buffer, bc_cb->ss_cb.block_size, &source, io_idx);
                
                /* Determine if the block is in cache */
                ret_status = bcm_is_block_in_cache(bc_cb, sector + io_idx, &idx, &slot, &ole);
                if (ret_status == NU_SUCCESS) /* Block is in the cache */
                {
                    if (slot->block_state == BCM_BLOCK_STATE_DIRTY)
                    {
                        /* Decrease the dirty block count. It will get
                           increased in a single place */
                        bc_cb->dirty_count--;
                    }                
                    
                    /* Calculate the destination address, return value ignored, 
                       input parameters already verified */
                    (VOID) bcm_get_block_at_index(&(bc_cb->ss_cb), bc_cb->ss_cb.block_set, 
                                                   &dest, idx);
                   
                    /* Remove the block from the list */
                    (VOID) bcm_remove_from_list(&(bc_cb->ol_cb), ole);
                }
                else /* Block is not in the cache */
                {
                    /* Check if a slot is available */
                    ret_status = bcm_allocate_slot(&(bc_cb->ss_cb), &slot, &idx);
                    if (ret_status == NU_SUCCESS)
                    {
                        /* Set up the slot */
                        slot->block_number = sector + io_idx;
                        slot->error_pending = 0;
                        
                        /* Set the destination to the new block and slot, return is
                           ignored */
                        (VOID) bcm_get_block_at_index(&(bc_cb->ss_cb), bc_cb->ss_cb.block_set,
                                                         &dest, idx);
                    }
                    else /* No free slots, do replacement */
                    {
                        /* Do LRU replacement to free a block */    
                        ret_status = bcm_do_replacement(bc_cb);
                        if (ret_status == NU_SUCCESS)
                        { 
                            /* A slot should now be free */
                            ret_status = bcm_allocate_slot(&(bc_cb->ss_cb), &slot, &idx);
                            if (ret_status == NU_SUCCESS)
                            {
                                /* Set up the slot */
                                slot->block_number = sector + io_idx;
                                slot->error_pending = 0;
                                
                                /* Set the destination to the new block and slot */
                                (VOID) bcm_get_block_at_index(&(bc_cb->ss_cb), bc_cb->ss_cb.block_set,
                                                               &dest, idx);
                            }
                        }
                    }
    
                    /* Setup of the OLE before leaving this block */
                    if ((ole == NU_NULL) && (ret_status == NU_SUCCESS))
                    {
                        ret_status = bcm_get_ole_at_index(bc_cb->ol_cb.core, &ole, idx, bc_cb->ss_cb.num_members);
                    }
                }
                 
                /* Cache the block */
                if (ret_status == NU_SUCCESS)
                {
                    /* Copy block data to the destination cache block */
                    NUF_Copybuff((VOID*)dest, (VOID*)source, bc_cb->ss_cb.block_size);
                    
                    slot->block_number = sector + io_idx;
                    slot->block_state = BCM_BLOCK_STATE_DIRTY;
                    slot->error_pending = 0;
                    
                    /* Assign the slot index to the OLE */
                    ole->slot_idx = idx;
                    
                    /* Add to MRU */
                    (VOID) bcm_add_to_list(&(bc_cb->ol_cb), ole);
                    
                    /* Update the dirty block count */
                    bc_cb->dirty_count++;
    
                    /* Set periodic flush if enabled and condition met */
                    if ((bc_cb->dirty_count == 1) && (bc_cb->periodic_flush != NU_NO_SUSPEND))
                    {
                        (VOID) NU_Set_Events(&(bc_cb->control_group), BCM_EVG_TIMEOUT_START, NU_OR);
                    }
    
                    /* Process the threshold events */
                    ret_status = bcm_process_threshold(bc_cb);                
                }
                else /* cache mechanism has failed. Write block to device */
                {                   
                     /* Dispatch the IO request */
                     ret_status = bcm_do_IO(bc_cb, sector + io_idx, source, 0);
                }
                
                /* Increment the IO count */
                io_idx++;
            } /* End of while */
                       
            /* Return the control block semaphore */
            (VOID) NU_Release_Semaphore(&(bc_cb->cb_sema));
        }
    }
    
    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       bcm_convert_path_to_cb         
*
* DESCRIPTION
*
*       Converts a path to a cache control block if available.   
*
* INPUTS
*
*       *path           Path of device, "A:"
*       **cb            Pointer to address of control block
*
* OUTPUTS
*
*       NU_SUCCESS      Operation successful
*       NUF_BADPARM     Invalid path or control block pointer
*       NUF_BADDRIVE    Drive does not have an active cache
*
*************************************************************************/
STATIC STATUS bcm_convert_path_to_cb(CHAR *path, BCM_CB **cb)
{
    STATUS ret_status;
    MTE_S   *mte;
    
    /* Verify the input parameters are valid */
    if (path == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else if (cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_SUCCESS;
    }
    
    if (ret_status == NU_SUCCESS)
    {
        /* Lock the mount table */
        LCK_ENTER(LCK_MT_TABLE)        
        
        /* Resolve mount table entry from path */
        mte = fsl_mte_from_fqpath(path);
        if(mte != NU_NULL)
        {        
            /* Verify the device has a cache */
            ret_status = bcm_device_has_cache(mte->mte_dh, cb);
        }
        else
        {
            ret_status = NUF_BADDRIVE;
        }
        
        /* Release the mount table lock */
        LCK_EXIT(LCK_MT_TABLE)
    }

    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       bcm_report_error         
*
* DESCRIPTION
*
*       Handles reporting IO errors to the NU_FILE_Cache_Error_Queue.   
*
* INPUTS
*
*       dh              Disk handle
*       block_number    Block with pending error
*       operation       Encoded IO request type (read, write)
*       io_status       Error as reported from IO dispatch routine
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
VOID bcm_report_error( UINT16 dh, UINT32 sector, BCM_ERROR_OPERATION operation, 
                       STATUS io_status)
{
    BCM_ERROR error;
    MTE_S *mte;
    
    mte = fsl_mte_from_dh(dh);
    if (mte != NU_NULL)
    {
        /* Assign the input parameters to the error message */ 
        error.operation = operation;
        error.block = sector;
        error.device_error = io_status;
        error.time = NU_Retrieve_Clock();
        error.path[0] = mte->mte_mount_name[0];
        error.path[1] = ':';    
       
        /* Ignore queue errors on purpose, reading from queue is a 
           user application responsibility */
        (VOID) NU_Send_To_Queue( &NU_FILE_Cache_Error_Queue, (VOID*)&error, 
                                  NU_FILE_Cache_Error_Size, NU_NO_SUSPEND);
    }
}
/************************************************************************
* FUNCTION
*
*       bcm_flush_cache       
*
* DESCRIPTION
*
*       Reused procedure for flushing the cache within system context. 
*       Assumes the control block is LOCKED by the calling function.
*
* INPUTS
*
*       *cb             Control block of cache to flush
*
* OUTPUTS
*
*       NU_SUCCESS      Operation successful
*       NUF_BADPARM     Invalid control block pointer
*       NUF_IO_ERROR    One or more blocks returned an IO error. Check
*                       error log for block details.
*
*************************************************************************/
STATIC STATUS bcm_flush_cache(BCM_CB *cb)
{
    STATUS      ret_status;
    UNSIGNED    io_error_count = 0;
    UNSIGNED    slot_idx;
    BCM_SLOT    *slot = NU_NULL;
    UINT8       *slot_buffer = NU_NULL;

    /* Verify input parameter */
    if (cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_SUCCESS;
    }

    if (ret_status == NU_SUCCESS)
    {
        /* Only attempt flush if there are dirty blocks */
        if (cb->dirty_count > 0)
        {
            /* For each slot in the cache, commit to device if dirty */
            for (slot_idx = 0; slot_idx < cb->ss_cb.num_members; slot_idx++)
            {
                /* Get the slot, return ignored, parameters have been verified */
                (VOID) bcm_get_slot_at_index(&(cb->ss_cb), cb->ss_cb.slot_set, &slot, slot_idx);
                
                if (slot->slot_state == BCM_SLOT_STATE_OCCUPIED)
                {
                    if (slot->block_state == BCM_BLOCK_STATE_DIRTY)
                    {
                        /* Set the buffer address */
                        (VOID) bcm_get_block_at_index(&(cb->ss_cb), cb->ss_cb.block_set,
                                                       &slot_buffer, slot_idx); 
                        
                        /* Try to write the slot */
                        ret_status = bcm_do_IO(cb, slot->block_number, slot_buffer, 0);
                        if (ret_status != NU_SUCCESS)
                        {
                            slot->error_pending++;
                            bcm_report_error(cb->dh, slot->block_number, BCM_ERROR_OPERATION_WRITE, ret_status);
                            io_error_count++;
                        }
                        else
                        {
                            /* Mark the slot as clean and without error */
                            slot->block_state = BCM_BLOCK_STATE_CLEAN;
                            slot->error_pending = 0;
                            
                            /* Decrement the dirty count */
                            cb->dirty_count--;
                        }
                    }
                }
            } /* End of for each slot */
        }
        
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE_REORDER == 1)
        /* Flush the multi-sector reordering cache */
        bcmms_flush(cb->bcmms_cb);
#endif        
    }

    /* flush encountered an IO error for one or more blocks */
    if (io_error_count > 0)
    {
        ret_status = NUF_IO_ERROR;
    }
    
    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       bcm_process_threshold      
*
* DESCRIPTION
*
*       Process the threshold parameters of the control block. 
*       Assumes the control block is LOCKED by the calling function. IO
*       errors are reported to the error log.
*
* INPUTS
*
*       *cb                 Control block pointer
*
* OUTPUTS
*
*       NU_SUCCESS         Operation successful
*       NUF_BADPARM        Invalid control block pointer
*
*************************************************************************/
STATIC STATUS bcm_process_threshold(BCM_CB *cb)
{
    STATUS ret_status;
    STATUS io_status;
    BCM_OLE *ole;
    BCM_SLOT *slot;
    UNSIGNED slot_idx;
    UINT8 *slot_buffer;
    
    if (cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_SUCCESS;    
    }
    
    if (ret_status == NU_SUCCESS)
    {
        /* If the threshold is reached */
        if (cb->dirty_count >= cb->high_threshold)
        {
            ole = cb->ol_cb.bcm_lru;
            while( (ole != NU_NULL) && (cb->dirty_count > cb->low_threshold))
            {
                slot_idx = ole->slot_idx;
                
                /* Get the slot */                
                ret_status = bcm_get_slot_at_index(&(cb->ss_cb), cb->ss_cb.slot_set, &slot, slot_idx);
                if (ret_status == NU_SUCCESS)
                {
                    if (slot->block_state == BCM_BLOCK_STATE_DIRTY)
                    {
                        /* Set the buffer address, return ignored as parameters have been verified */
                        (VOID) bcm_get_block_at_index(&(cb->ss_cb), cb->ss_cb.block_set, 
                                                       &slot_buffer, slot_idx);
                        
                        /* Try to write the slot */
                        io_status = bcm_do_IO(cb, slot->block_number, slot_buffer, 0);
                        if (io_status != NU_SUCCESS)
                        {
                            slot->error_pending++;
                            bcm_report_error(cb->dh, slot->block_number, BCM_ERROR_OPERATION_WRITE, io_status);
                        }
                        else
                        {
                            /* Mark the slot as clean and without error */
                            slot->block_state = BCM_BLOCK_STATE_CLEAN;
                            slot->error_pending = 0;
                            
                            /* Decrement the dirty count */
                            cb->dirty_count--;
                        }
                    }
                }
                
                /* Get the next block in the cache */
                ole = ole->next;
            } /* End while items in the list and low threshold has not be reached */
        }
    }
    
    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       bcm_obtain_dev_layer_semaphores      
*
* DESCRIPTION
*
*       Obtain the device layer semaphores in a consistent manner. Two 
*       semaphores are used to protect access. The "fsdh_sema" is a device
*       provided semaphore that manages single controller to multi disk handle
*       relationships. The "fsdh_vfs_sema" is a system provided semaphore to
*       prevent concurrency issues when starting/stopping a cache controlled
*       device. 
*
* INPUTS
*
*       *path              The path of the mounted device to lock
*
* OUTPUTS
*
*       NU_SUCCESS         Operation successful
*       NUF_BADPARM        Invalid path parameter
*       NUF_BADDRIVE       Invalid drive, may not be mounted
*
*************************************************************************/
STATIC STATUS bcm_obtain_dev_layer_semaphores(CHAR *path)
{
    STATUS status;  
    MTE_S  *mte; 
    UINT16 dh; 
    
    if (path == NU_NULL)
    {
        status = NUF_BADPARM;
    }
    else
    {
        status = NU_SUCCESS;
    }
    
    if (status == NU_SUCCESS)
    {
        /* Lock the mount table */
        LCK_ENTER(LCK_MT_TABLE)        
        
        /* Resolve mount table entry from path */
        mte = fsl_mte_from_fqpath(path);
        if(mte != NU_NULL)
        {        
            dh = mte->mte_dh;  
        }
        else
        {
            status = NUF_BADDRIVE;
        }
        
        /* Release the mount table lock */
        LCK_EXIT(LCK_MT_TABLE)    
    }

    if (status == NU_SUCCESS)
    {
        /* Not all devices provide the "fsdh_sema" semaphore */
        if (FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_sema != NU_NULL)
        {
            status = NU_Obtain_Semaphore(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_sema, NU_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {   
            /* All disk handles provide a "fsdh_vfs_semaphore" */             
            status = NU_Obtain_Semaphore(&(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_vfs_sema), NU_SUSPEND);
            if (status != NU_SUCCESS)
            {
                if (FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_sema != NU_NULL)
                {
                    (VOID) NU_Release_Semaphore(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_sema);
                }
            }
        }
    }

    return (status);    
}
/************************************************************************
* FUNCTION
*
*       bcm_release_dev_layer_semaphores      
*
* DESCRIPTION
*
*       Release the previously acquired semaphores
*
* INPUTS
*
*       *path              Path of previously locked device
* OUTPUTS
*
*       NU_SUCCESS         Operation successful
*       NUF_BADPARM        Invalid path parameter (null)
*       NUF_BADDRIVE       Drive is not mounted 
*
*************************************************************************/
STATIC STATUS bcm_release_dev_layer_semaphores(CHAR *path)
{
    STATUS status;  
    MTE_S  *mte; 
    UINT16 dh; 
    
    if (path == NU_NULL)
    {
        status = NUF_BADPARM;
    }
    else
    {
        status = NU_SUCCESS;
    }
    
    if (status == NU_SUCCESS)
    {
        /* Lock the mount table */
        LCK_ENTER(LCK_MT_TABLE)        
        
        /* Resolve mount table entry from path */
        mte = fsl_mte_from_fqpath(path);
        if(mte != NU_NULL)
        {        
            dh = mte->mte_dh;  
        }
        else
        {
            status = NUF_BADDRIVE;
        }
        
        /* Release the mount table lock */
        LCK_EXIT(LCK_MT_TABLE)    
    }

    if (status == NU_SUCCESS)
    {
        if (FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_sema != NU_NULL)
        {
            (VOID) NU_Release_Semaphore(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_sema);
        }
                
        (VOID) NU_Release_Semaphore(&(FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(dh)].fsdh_vfs_sema));
    }

    return (status);    
}
/************************************************************************
* FUNCTION
*
*       NU_FILE_Cache_Destroy        
*
* DESCRIPTION
*
*       Destroys an active cache for the given path. All dirty blocks are 
*       flushed to the device in the calling context. All resources are 
*       returned. IO errors are reported using the cache error log and
*       set as the return status. 
*
* INPUTS
*
*       *path              Pointer to an active cache, "A:"
*
* OUTPUTS
*
*       NU_SUCCESS         Operation successful
*       NUF_BADPARM        Invalid path parameter
*       NU_UNAVAILABLE     Device does not have an active cache
*       NUF_IO_ERROR       An IO error occurred, check error log for
*                          details.
*
*************************************************************************/
STATUS NU_FILE_Cache_Destroy(CHAR *path)
{
    STATUS       ret_status;
    STATUS       flush_status = NU_SUCCESS; /* Init this since set path may not
                                               get executed */
    BCM_CB       *cb = NU_NULL;
    BCM_CB       *tempCb;

    LCK_FS_ENTER()

    if (path == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else 
    {
        ret_status = NU_SUCCESS;
    }
        
    if (ret_status == NU_SUCCESS)
    {
        (VOID) bcm_obtain_dev_layer_semaphores(path);
                
        /* Lock the control block list to prevent access */
        ret_status = NU_Obtain_Semaphore(&BCM_List_Sema, NU_SUSPEND);
        if (ret_status == NU_SUCCESS)
        {
            /* Get the control block */
            ret_status = bcm_convert_path_to_cb(path, &cb);
            if (ret_status == NU_SUCCESS)
            {
                /* Get the control block lock */
                (VOID) NU_Obtain_Semaphore(&(cb->cb_sema), NU_SUSPEND);
    
                /* Remove the control block from the list */
                tempCb = BCM_CB_List;
                
                /* If it's the only item in the list */
                if (BCM_CB_List == cb)
                {
                    BCM_CB_List = cb->next; /* Can be null */
                }
                else /* There is more than one item in the list */
                {
                    while (tempCb != NU_NULL)
                    {
                        if (tempCb->next != NU_NULL)
                        {
                            /* Remove it from list. Assigning null is expected */
                            if (tempCb->next == cb)
                            {
                                /* Found it. */
                                tempCb->next = cb->next;
                                break;
                            }
                        }
                        
                        /* Can safely assign null since we exit the while loop */
                        tempCb = tempCb->next;
                    }
                }                                                

                /* Release the list lock */
                (VOID)NU_Release_Semaphore(&BCM_List_Sema);
   
                /* Flush the cache */
                flush_status = bcm_flush_cache(cb);
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE_REORDER == 1)
                /* Cleanup resources for the secondary cache */
                (VOID) bcmms_delete(cb->dh);  
#endif                
                /* Delete the event group */
                ret_status = NU_Delete_Event_Group(&(cb->control_group));
                if (ret_status == NU_SUCCESS)
                {
                    /* Halt the periodic flush task */
                    (VOID) NU_Terminate_Task(&(cb->control_task));
                    (VOID) NU_Delete_Task(&(cb->control_task));
                }
                
                /* OS primitives removed, release the memory */
                if (ret_status == NU_SUCCESS)
                {
                    (VOID) NU_Deallocate_Memory((VOID*)cb->ss_cb.block_set);
                    (VOID) NU_Deallocate_Memory((VOID*)cb->ss_cb.slot_set);
                    (VOID) NU_Deallocate_Memory((VOID*)cb->ol_cb.core);
                    (VOID) NU_Deallocate_Memory((VOID*)cb->task_stack);
    
                    /* Release the control block lock */ 
                    (VOID) NU_Release_Semaphore(&(cb->cb_sema));
    
                    /* Delete the control block semaphore */
                    (VOID) NU_Delete_Semaphore(&(cb->cb_sema));
                    
                    /* Release the control block memory */
                    (VOID) NU_Deallocate_Memory((VOID*)cb);
                }
                else
                {
                    /* Release the control block lock */ 
                    (VOID) NU_Release_Semaphore(&(cb->cb_sema));
                }
            }
            else
            {
                /* Could not find a match, release the list control block */
                (VOID) NU_Release_Semaphore(&BCM_List_Sema);
            }
        }
        
        /* Cleanup holding the device semaphore. */
       (VOID) bcm_release_dev_layer_semaphores(path);
    }
    
    /* If teardown was successful, but there was an IO error from flush
       return the IO error. Otherwise return the teardown. IO error is
       also reported to the error queue */
    if ((ret_status == NU_SUCCESS) && (flush_status != NU_SUCCESS))
    {
        ret_status = flush_status;
    }
    
    LCK_FS_EXIT()
    
    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       NU_FILE_Cache_Get_Config     
*
* DESCRIPTION
*
*       Retrieve the current cache configuration parameters for an active device
*       cache.
*
* INPUTS
*
*       *pointer           Pointer to an active device cache, "A:"
*       **config           Pointer to the address of a configuration structure
*                          used to return the config parameters
*
* OUTPUTS
*
*       NU_SUCCESS         Operation successful
*       NUF_BADPARM        Invalid parameter given, null pointer
*       NUF_BADDRIVE       Drive does not exist
*       NU_UNAVAILABLE     Drive does not have an active cache
*
*************************************************************************/
STATUS NU_FILE_Cache_Get_Config(CHAR *path, VOID **config)
{
    STATUS ret_status;
    BCM_CB *cb = NU_NULL;
    BCM_DL_LRU_CONFIG *ret_config;
    
    LCK_FS_ENTER()
    
    if (path == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else if (config == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_SUCCESS;
    }
    
    if (ret_status == NU_SUCCESS)
    {
        /* Lock the control block list to prevent access */
        (VOID) NU_Obtain_Semaphore(&BCM_List_Sema, NU_SUSPEND);
        
        /* Get the control block */
        ret_status = bcm_convert_path_to_cb(path, &cb);
        if (ret_status == NU_SUCCESS)
        {
            (VOID) NU_Obtain_Semaphore(&(cb->cb_sema), NU_SUSPEND);
            ret_config = (BCM_DL_LRU_CONFIG *)*config;
            
            ret_config->periodic_flush = cb->periodic_flush;
            ret_config->low_threshold = cb->low_threshold;
            ret_config->high_threshold = cb->high_threshold;
            ret_config->size = (cb->ss_cb.num_members * cb->ss_cb.block_size);
            
            (VOID) NU_Release_Semaphore(&(cb->cb_sema));
        }
        /* Release the list semaphore */ 
        (VOID) NU_Release_Semaphore(&BCM_List_Sema);
    }
    
    LCK_FS_EXIT()
    
    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       NU_FILE_Cache_Set_Config      
*
* DESCRIPTION
*
*       Set new cache configuration parameters. NOTE: Device cache size cannot be
*       modified. Recreate the cache to modify a cache size.
*
* INPUTS
*
*       *path                   Pointer to an active device cache, "A:"
*       *config                 Cache configuration parameters to set
*
* OUTPUTS
*
*       NU_SUCCESS             Operation successful
*       NUF_BADPARM            Invalid path or config pointer, invalid config parameter
*       NU_UNAVAILABLE         Drive does not have an active cache
*
*************************************************************************/
STATUS NU_FILE_Cache_Set_Config (CHAR *path, VOID *config)
{
    STATUS ret_status;
    BCM_CB *cb = NU_NULL;
    BCM_DL_LRU_CONFIG *set_config;
    
    LCK_FS_ENTER()

    if (path == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else if (config == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else 
    {
        ret_status = NU_SUCCESS;
    }
        
    if (ret_status == NU_SUCCESS)
    {
        ret_status = bcm_obtain_dev_layer_semaphores(path);
        
        if (ret_status == NU_SUCCESS)
        {
            /* Lock the control block list to prevent access */
            (VOID) NU_Obtain_Semaphore(&BCM_List_Sema, NU_SUSPEND);
            
            /* Get the control block */
            ret_status = bcm_convert_path_to_cb(path, &cb);
            if (ret_status == NU_SUCCESS)
            {
                (VOID) NU_Obtain_Semaphore(&(cb->cb_sema), NU_SUSPEND);
                set_config = (BCM_DL_LRU_CONFIG *)config;
                
                /* Release the list semaphore so others can access. */
                (VOID) NU_Release_Semaphore(&BCM_List_Sema);
                
                /* Validate the new parameters */
                if (set_config->low_threshold > set_config->high_threshold)
                {
                    /* Low must be < high */
                    ret_status = NUF_BADPARM;
                }
                else if ((set_config->low_threshold > cb->ss_cb.num_members) || 
                         (set_config->high_threshold > cb->ss_cb.num_members ))
                {
                    /* Must be within range */
                    ret_status = NUF_BADPARM; 
                }                       
                else  /* Parameters are valid */
                {
                    /* Set the new parameters */
                    if (cb->periodic_flush != set_config->periodic_flush)
                    {
                        cb->periodic_flush = set_config->periodic_flush;
                        
                        /* Notify the period flush task of a timeout change */
                        (VOID) NU_Set_Events(&(cb->control_group), BCM_EVG_TIMEOUT_MODIFIED, NU_OR);
                    }
                    
                    /* Set threshold parameters */
                    if ((cb->low_threshold != set_config->low_threshold) ||
                        (cb->high_threshold != set_config->high_threshold))
                    {
                        cb->low_threshold = set_config->low_threshold;
                        cb->high_threshold = set_config->high_threshold;
                        
                        /* Request processing for the new threshold values */                    
                        ret_status = bcm_process_threshold(cb);
                    }
                }
                                
                /* Release the control block semaphore */
                (VOID) NU_Release_Semaphore(&(cb->cb_sema));
            }
            else
            {
                /* Release the list, we didn't find a match */
                (VOID) NU_Release_Semaphore(&BCM_List_Sema);
            }
            
            /* Release the device protection semaphores */
            (VOID) bcm_release_dev_layer_semaphores(path);
        }
    }

    LCK_FS_EXIT()
    
    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       NU_FILE_Cache_Flush     
*
* DESCRIPTION
*
*       Flush the given mounted device's cache. All dirty blocks are  
*       written to the device. Device block errors are reported to the cache
*       error log and noted by the return value. 
*
* INPUTS
*
*       *path                  Pointer to a path of an active device cache, "A:"
*
* OUTPUTS
*
*       NU_SUCCESS             Operation successful
*       NUF_BADPARM            Invalid path parameter
*       NU_UNAVAILABLE         Device does not have an active cache
*       NU_INVALID_SEMAPHORE   VFS or cache is being shutdown on the device
*       NUF_IO_ERROR           An IO error occurred during the flush. Block
*                              specific errors are reported to error log.
*
*************************************************************************/
STATUS NU_FILE_Cache_Flush(CHAR *path)
{
    STATUS      ret_status;

    BCM_CB      *cb = NU_NULL;

        
    LCK_FS_ENTER()
    
    if (path == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_SUCCESS;
    }
    
    if (ret_status == NU_SUCCESS)
    {
        ret_status = bcm_obtain_dev_layer_semaphores(path);
        
        if (ret_status == NU_SUCCESS)
        {
            /* Lock the control block list to prevent access */
            ret_status =  NU_Obtain_Semaphore(&BCM_List_Sema, NU_SUSPEND);
            if (ret_status == NU_SUCCESS)
            {
                /* Get the control block */
                ret_status = bcm_convert_path_to_cb(path, &cb);
                if (ret_status == NU_SUCCESS)
                {
                    /* Lock the control block */
                    ret_status = NU_Obtain_Semaphore(&(cb->cb_sema), NU_SUSPEND);
                    if (ret_status == NU_SUCCESS)
                    {
                        /* Release the list semaphore so others can access. */
                        (VOID) NU_Release_Semaphore(&BCM_List_Sema);    
            
                        /* Flush the cache */
                        ret_status = bcm_flush_cache(cb);
                        
                        /* Release the control block semaphore */
                        (VOID) NU_Release_Semaphore(&(cb->cb_sema));
                    }
                    else   
                    { 
                        /* Release the list semaphore */
                        (VOID) NU_Release_Semaphore(&BCM_List_Sema);
                    }
                }
                else
                {
                    /* Release the list semaphore */
                    (VOID) NU_Release_Semaphore(&BCM_List_Sema);
                }
            }
            
            (VOID) bcm_release_dev_layer_semaphores(path);
        }
    }    
    
    LCK_FS_EXIT()
    
    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       bcm_do_IO     
*
* DESCRIPTION
*
*       Wrapper for doing single sector IO requests for device with cache. 
*       Handles switching between device direct IO or using the reordering 
*       IO BCMMS component. 
*
* INPUTS
*
*       cb                      BCM control block
*       sector                  Start sector for request
*       *buffer                 Buffer pointer for request
*       reading                 Type of request - 0: write  1: read
*
* OUTPUTS
*
*       NU_SUCCESS             Operation successful
*
*************************************************************************/
STATIC STATUS bcm_do_IO(BCM_CB *cb, UINT32 sector, VOID *buffer, INT reading)
{
    STATUS sts = NUF_BADPARM;
    UINT32 index = 0;
    
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE_REORDER == 1)
    if (reading == 1)
    {
        sts = bcmms_read( cb->bcmms_cb, sector, buffer);    
    }
    else 
    {
        sts = bcmms_write(cb->bcmms_cb, sector, buffer);
    }
#else    
    index = FSDH_GET_IDX_FROM_DH(cb->dh);

    if (index < FSDH_MAX_DISKS)
    {
        /* Block reordering is not in effect */ 
        sts = FS_Disk_Handles[index].fsdh_fdev->fdev_ops.io_proc(cb->dh,
                                 sector, buffer, 1, reading);
    }
#endif                                 

    return (sts);
}
#endif /* (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 1) */
