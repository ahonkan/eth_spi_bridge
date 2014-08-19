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
*       bcmms.c
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Block Cache Manager - Multi-sector
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Device IO buffer for reordering and multi-sector grouping 
*       of IO requests.                                  
*                                                                       
* DATA STRUCTURES                                                       
*
*       BCMMS_CB_List           List of BCMMS_CB structures for currently
*                               operating caches                                                                       
*                                                                       
* FUNCTIONS                                                             
*
*       bcmms_add_to_slot_list
*       bcmms_allocate_slot
*       bcmms_deallocate_slot
*       bcmms_find_longest_sequence
*       bcmms_remove_from_list
*       bcmms_perform_IO
*       bcmms_process_threshold
*       bcmms_is_block_in_cache
*       bcmms_device_has_cache
*
*       bcmms_feature_init
*       bcmms_create
*       bcmms_delete
*       bcmms_read
*       bcmms_write
*       bcmms_flush                                                                 
*
* DEPENDENCIES
*  
*       "plus/nucleus.h"
*       "file/vfs/inc/pcdisk.h"
*       "file/vfs/inc/bcm_extr.h"
*       "file/vfs/inc/bcmms_extr.h"
*       "file/vfs/inc/dev_extr.h"
*       "file/vfs/inc/fsl_extr.h"
*                                                                       
*************************************************************************/

#include "nucleus.h"
#include "storage/pcdisk.h"

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE_REORDER == 1)
#include "os/kernel/plus/supplement/inc/partition_memory.h"
#include "storage/bcm_extr.h"
#include "storage/bcmms_extr.h"
#include "storage/dev_extr.h"
#include "storage/fsl_extr.h"

/* Global data */
BCMMS_CB *BCMMS_CB_List;

/* Local prototypes */
STATIC VOID   bcmms_release_resources(BCMMS_CB *cb);
STATIC VOID   bcmms_add_to_slot_list(BCMMS_CB *cb, BCMMS_SLOT *new_node);
STATIC STATUS bcmms_allocate_slot(BCMMS_CB *cb, BCMMS_SLOT **new_node);
STATIC VOID   bcmms_deallocate_slot(BCMMS_CB *cb, BCMMS_SLOT *node);
STATIC STATUS bcmms_find_longest_sequence(BCMMS_CB *cb, BCMMS_SLOT **ret_start, UNSIGNED *ret_count);
STATIC VOID   bcmms_remove_from_list(BCMMS_CB *cb, BCMMS_SLOT *node);
STATIC STATUS bcmms_perform_IO(BCMMS_CB *cb, BCMMS_SLOT *start_node, UNSIGNED count);
STATIC VOID   bcmms_process_threshold(BCMMS_CB *cb);
STATIC STATUS bcmms_is_block_in_cache(BCMMS_CB *cb, UINT32 block_number, BCMMS_SLOT **ret_slot);

/* Externally defined data */
extern FSDH_S  FS_Disk_Handles[FSDH_MAX_DISKS];

/************************************************************************
* FUNCTION
*
*       bcmms_feature_init   
*
* DESCRIPTION
*
*       Initialize the multi-sector IO scheduling feature.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*
*************************************************************************/
STATUS bcmms_feature_init(VOID)
{
    /* Init the list of the BCMMS control blocks */
    BCMMS_CB_List = NU_NULL;

    return (NU_SUCCESS);    
}

/************************************************************************
* FUNCTION
*
*       bcmms_create   
*
* DESCRIPTION
*
*       Creates a secondary cache for grouping items in to multi-sector
*       requests. NOTE: This should only be called from the block cache
*       component. There are no tests for dh validity as it was performed
*       by the block cache.       
*
* INPUTS
*
*       dh                      Disk handle of the associated device 
*       num_members             Number of blocks in cache
*       block_size              Size of a single block
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NU_NO_MEMORY            Insufficient memory
*       NU_UNAVAILABLE          Device already has a cache
*
*************************************************************************/
STATUS bcmms_create(UINT16 dh, UNSIGNED num_members, UNSIGNED block_size)
{
    STATUS ret_status;
    BCMMS_CB *cb;
    UNSIGNED pool_size;
    VOID *memory;
    
    /* Verify a CB doesn't already exist */
    ret_status = bcmms_device_has_cache(dh, &cb);
    
    if (ret_status == NU_SUCCESS)
    {
        ret_status = NU_UNAVAILABLE;
    }
    else
    {
        cb = (VOID*)NUF_Alloc(sizeof(BCMMS_CB));
        if (cb != NU_NULL)
        {
            /* Clear out the allocated control block memory */
            NUF_Memfill((VOID*)cb, sizeof(BCMMS_CB), 0);
            
            /* Setup the control block data structures */
            cb->next = NU_NULL;
            cb->dh = dh;
            cb->num_members = num_members;    
            cb->empty_count = num_members;
            cb->high_threshold = num_members;
            if (num_members > BCMMS_FIXED_XFER_BUFFER_BLOCK_COUNT)
            {
                cb->low_threshold = (num_members - BCMMS_FIXED_XFER_BUFFER_BLOCK_COUNT);
            }
            else
            {
                cb->low_threshold = 0;  
            }
            
            cb->block_size  = block_size;                                        
            cb->slot_head   = NU_NULL;              
            
            /* Allocate memory for a slot partition pool */
            pool_size = (PM_OVERHEAD + BCMMS_SLOT_SIZE) * num_members;
            cb->slot_memory = NUF_Alloc(pool_size);
            if (cb->slot_memory != NU_NULL)
            {
                /* Create a memory partition pool for the slots */
                ret_status = NU_Create_Partition_Pool(&(cb->slot_pool), "BCMMS_S", 
                                                      cb->slot_memory, pool_size, BCMMS_SLOT_SIZE,
                                                      NU_PRIORITY);
                if (ret_status == NU_SUCCESS)
                {
                    /* Allocate memory for the block buffer pool */
                    pool_size = (PM_OVERHEAD + block_size)  * num_members;
                    cb->block_memory = NUF_Alloc(pool_size);
                    if (cb->block_memory != NU_NULL)
                    {
                        /* Create a memory partition pool for the data */
                        ret_status = NU_Create_Partition_Pool(&(cb->block_pool), "BCMMS_B",
                                                              cb->block_memory, pool_size, block_size,
                                                              NU_PRIORITY);
                        if (ret_status == NU_SUCCESS)
                        {                                                                  
                            /* Create a large buffer for grouping the multi-sector requests */                           
                            pool_size = (block_size * BCMMS_FIXED_XFER_BUFFER_BLOCK_COUNT);
                            memory = NUF_Alloc(pool_size);
                            if (memory != NU_NULL)
                            {
                                /* Assign the buffer to the control block */
                                cb->xfer_buffer = memory;
    
                                /* Add the control block to the list */
                                cb->next = BCMMS_CB_List;
                                BCMMS_CB_List = cb;
                            }
                            else
                            {
                                /* Alloc xfer buffer failed. Set return status */
                                ret_status = NU_NO_MEMORY;                                
                            }
                        }
                    } 
                    else
                    {
                        /* Block memory for partition pool alloc failed. Set return status */
                        ret_status = NU_NO_MEMORY;                         
                    }
                }
            }
            else
            {
                /* Alloc for slot partition pool failed. Set return status */
                ret_status = NU_NO_MEMORY;                
            }
            
            /* Error encountered when allocated cache resources, cleanup */
            if (ret_status != NU_SUCCESS)
            {
                bcmms_release_resources(cb);
            }
        }
        else
        {   
            /* Insufficient memory */
            ret_status = NU_NO_MEMORY;
        }
    }
        
    return (ret_status);    
}

/************************************************************************
* FUNCTION
*
*       bcmms_delete   
*
* DESCRIPTION
*
*       Release the resources from the BCMMS control block
*
* INPUTS
*
*       dh                      Disk handle 
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*
*************************************************************************/
STATUS bcmms_delete(UINT16 dh)
{
    STATUS ret_status;
    BCMMS_CB *search_cb, *cb;
    
    /* Verify device has a cache */
    ret_status = bcmms_device_has_cache(dh, &cb);
    if (ret_status == NU_SUCCESS)
    {
        /* Throw away the cb returned from bcmms_device_has_cache
           since we are going to remove it from the list and need 
           the previous control block from a single linked list */
        search_cb = BCMMS_CB_List;
        
        if (search_cb != NU_NULL)
        {
            /* Catch it if it's the first item in the list */
            if (search_cb->dh == dh)
            {
                BCMMS_CB_List = search_cb->next; /* Assigning list to NU_NULL is valid */
            }
            else
            {
                /* Find the matching control block in the list */
                while (search_cb->next != NU_NULL)    
                {
                    if ((search_cb->next)->dh == dh)
                    {
                        search_cb->next = (search_cb->next)->next;
                        break;
                    }
                    else
                    {
                        search_cb = search_cb->next;
                    }
                }
            }
        }
    }
    
    if (ret_status == NU_SUCCESS)
    {
        /* Flush all items from cache */
        bcmms_flush(cb);        
        
        /* Return resources allocated to the cache */
        bcmms_release_resources(cb);
    }
    
    return (ret_status);    
}
/************************************************************************
* FUNCTION
*
*       bcmms_release_resources   
*
* DESCRIPTION
*
*       Release previously allocated resources.
*
* INPUTS
*
*       cb                      Control block
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID bcmms_release_resources(BCMMS_CB *cb)
{
    if (cb != NU_NULL)
    {
        /* Return resources allocated to the cache */
        if (cb->block_memory != NU_NULL)
        {
            (VOID) NU_Delete_Partition_Pool(&(cb->block_pool));
            (VOID) NU_Deallocate_Memory(cb->block_memory);
        }
        
        if (cb->slot_memory != NU_NULL)
        {
            (VOID) NU_Delete_Partition_Pool(&(cb->slot_pool));
            (VOID) NU_Deallocate_Memory(cb->slot_memory);
        }
        
        if (cb->xfer_buffer != NU_NULL)
        {
            (VOID) NU_Deallocate_Memory(cb->xfer_buffer);
        }
        
        (VOID) NU_Deallocate_Memory((VOID*)cb); 
        
    }
}
/************************************************************************
* FUNCTION
*
*       bcmms_read   
*
* DESCRIPTION
*
*       Read a block. If block data not available in the cache, request
*       data from the device. Blocks read from the device do not enter 
*       cache. 
*
* INPUTS
*
*       cb                      Control block
*       block_number            Block number being read
*       block_data              Block data to be read
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NUF_BADPARM             Invalid input parameter
*
*************************************************************************/
STATUS bcmms_read(BCMMS_CB *cb, UINT32 block_number, VOID *block_data)
{
    STATUS ret_status;
    BCMMS_SLOT *slot;
    
    if ((cb == NU_NULL) || (block_data == NU_NULL))
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        /* Check if the block is in cache */
        ret_status = bcmms_is_block_in_cache(cb, block_number, &slot);
        if (ret_status == NU_SUCCESS)
        {
            /* Copy the existing requested block data */
            NUF_Copybuff(block_data, slot->block_data, cb->block_size);
        }
        else
        {
            /* Request the data from the device */
            ret_status = FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(cb->dh)].fsdh_fdev->fdev_ops.io_proc(cb->dh, 
                                    block_number, block_data, 1, 1);
        }
    }
    
    return (ret_status);    
}
/************************************************************************
* FUNCTION
*
*       bcmms_write   
*
* DESCRIPTION
*
*       Writes a single data block to the cache. Cached block data is 
*       updated. A newly allocated block may trigger a threshold event.
*       Threshold processing will always leave blocks available.        
*
* INPUTS
*
*       cb                      Control block
*       block_number            Block number being written
*       block_data              Block data to be written
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NUF_BADPARM             Invalid input parameter
*
*************************************************************************/
STATUS bcmms_write(BCMMS_CB *cb, UINT32 block_number, VOID *block_data)
{
    STATUS     ret_status;
    BCMMS_SLOT *slot;
    
    if ((cb == NU_NULL) || (block_data == NU_NULL))
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        /* Is the block in cache */
        ret_status = bcmms_is_block_in_cache(cb, block_number, &slot);
        if (ret_status == NU_SUCCESS)
        {
            /* Block is in cache, copy the new data over existing data */
            NUF_Copybuff(slot->block_data, block_data, cb->block_size);
        }
        else
        {
            /* Block is not in cache */
            /* Allocate a slot */
            ret_status = bcmms_allocate_slot(cb, &slot);
            if (ret_status == NU_SUCCESS)
            {
                /* Setup the slot data */
                slot->block_number = block_number;
                NUF_Copybuff(slot->block_data, block_data, cb->block_size);
                
                /* Add it to the cache */
                bcmms_add_to_slot_list(cb, slot);                
                
                /* Process threshold */
                bcmms_process_threshold(cb);
            }            
        }
    }
    
    return (ret_status);    
}
/************************************************************************
* FUNCTION
*
*       bcmms_flush   
*
* DESCRIPTION
*
*       Flush all blocks from the cache. Flush preserves the current 
*       available multi-sector operations by performing them as groups. 
*       Errors are reported using the upper level error queue. 
*
* INPUTS
*
*       cb                      Control block to flush
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID bcmms_flush(BCMMS_CB *cb)
{
    STATUS     status = NU_SUCCESS;
    BCMMS_SLOT *slot_start;
    UNSIGNED   count;
    
    if (cb != NU_NULL)
    {
        while ((cb->empty_count != cb->num_members) && (status == NU_SUCCESS))
        {
            /* Empty the cache starting with the largest
               group working down until the cache is empty. */
            status = bcmms_find_longest_sequence(cb, &slot_start, &count);
            if (status == NU_SUCCESS)
            {
                /* Send the sequence to IO */                    
                status = bcmms_perform_IO(cb, slot_start, count);
            }
        }
    }
}
/************************************************************************
* FUNCTION
*
*       bcmms_device_has_cache   
*
* DESCRIPTION
*
*       Returns whether or not a device has an existing cache.
*
* INPUTS
*
*       dh                      Disk handle of device in question
*       ret_cb                  Control block for matching device
*
* OUTPUTS
*
*       NU_SUCCESS              Device has a cache
*       NU_UNAVAILABLE          Device does not have a cache
*
*************************************************************************/
STATUS bcmms_device_has_cache(UINT16 dh, BCMMS_CB **ret_cb)
{
    STATUS ret_status;
    BCMMS_CB *cb;
    
    if (ret_cb == NU_NULL)
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        ret_status = NU_UNAVAILABLE; /* Assume it is not in list */
    
        /* Start at head of control block list */    
        cb = BCMMS_CB_List;
        
        while (cb != NU_NULL)
        {
            if (cb->dh == dh)
            {
                *ret_cb = cb; /* Setup the return control block */
                ret_status = NU_SUCCESS; /* found a match */
                break;
            }
            
            cb = cb->next;
        }
    }
    
    return (ret_status);    
}

/************************************************************************
* FUNCTION
*
*       bcmms_add_to_slot_list   
*
* DESCRIPTION
*
*       Adds a new slot to the list ordered by the block number. 
*
* INPUTS
*
*       cb                      Control block to add to
*       new_node                New node being added
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID bcmms_add_to_slot_list(BCMMS_CB *cb, BCMMS_SLOT *new_node)
{
    BCMMS_SLOT *search_ptr;

    /* Verify input before dereferencing */
    if ((cb != NU_NULL) && (new_node != NU_NULL))
    {
        /* Determine if the list is non-empty.  */
        if (cb->slot_head != NU_NULL)
        {
            /* Search the list to find the proper place for the new node.  */
            search_ptr =  cb->slot_head;
    
            /* Check for insertion before the first node on the list.  */
            if (search_ptr->block_number > new_node->block_number)
            {
                /* Update the head pointer to point at the new node.  */
                cb->slot_head =  new_node;
            }
            else
            {
                /* We know that the new node is not the first and
                   must be placed somewhere after the head pointer.  */
    
                /* Move search pointer up to the next node since we are trying
                   to find the proper node to insert in front of. */
                search_ptr =  search_ptr->next;
                while ((search_ptr->block_number <= new_node->block_number) &&
                       (search_ptr != cb->slot_head))
                {
                    /* Move along to the next node.  */
                    search_ptr =  search_ptr->next;
                }
            }
    
            /* Insert before search pointer.  */
            new_node->prev         = search_ptr->prev;
            (new_node->prev)->next = new_node;
            new_node->next         = search_ptr;
            (new_node->next)->prev = new_node;
        }
        else
        {
            /* The list is empty, setup the head and the new node.  */
            cb->slot_head  = new_node;
            new_node->prev = new_node;
            new_node->next = new_node;
        }
    }
}

/************************************************************************
* FUNCTION
*
*       bcmms_allocate_slot   
*
* DESCRIPTION
*
*       Allocate a slot from the control block's partition. This does both
*       the data structure and data storage as a single unit.
*
* INPUTS
*
*       cb                      Control block
*       new_node                Newly allocated slot returned
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NUF_BADPARM             Invalid input parameter
*       NU_UNAVAILABLE          No slots are available
*
*************************************************************************/
STATIC STATUS bcmms_allocate_slot(BCMMS_CB *cb, BCMMS_SLOT **new_node)
{
    STATUS ret_status;
    BCMMS_SLOT *local_node;
    VOID    *partition;
    
    if ((cb == NU_NULL) || (new_node == NU_NULL))
    {
        ret_status = NUF_BADPARM;
    }
    else
    {
        /* Make sure a spot is available before attempting to allocate */
        if (cb->empty_count == 0)
        {
            ret_status = NU_UNAVAILABLE;
        }
        else
        {
            /* Get a partition for the data structure */
            ret_status = NU_Allocate_Partition (&(cb->slot_pool), (VOID**)&partition, NU_NO_SUSPEND);
            if (ret_status == NU_SUCCESS)
            {
                /* Cast the partition to our data type, finish initializing the structure */
                local_node = (BCMMS_SLOT*)partition;
                NUF_Memfill((VOID*)local_node, sizeof(BCMMS_SLOT), 0);
                ret_status = NU_Allocate_Partition (&(cb->block_pool), (VOID**)&(local_node->block_data), NU_NO_SUSPEND);
                if (ret_status != NU_SUCCESS)
                {
                    /* Return the data structure slot node */
                    (VOID) NU_Deallocate_Partition((VOID*)local_node);
                }
                else
                {
                    /* Update the control block given successful allocation */
                    cb->empty_count--; 
                    
                    /* Set the pointer for the returned object */
                    (*new_node) = local_node; 
                }                
            }
        }
    }
    
    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       bcmms_deallocate_slot   
*
* DESCRIPTION
*
*       Deallocate a slot. Includes deallocated the data storage.
*
* INPUTS
*
*       cb                      Control block
*       node                    Slot to return
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID bcmms_deallocate_slot(BCMMS_CB *cb, BCMMS_SLOT *node)
{
    if ((cb != NU_NULL) && (node != NU_NULL))
    {
        /* Release the data block partition */
        (VOID) NU_Deallocate_Partition(node->block_data);        
        
        /* Release the slot data structure partition */
        (VOID) NU_Deallocate_Partition((VOID*)node);
        
        /* Update the control block */
        cb->empty_count++;
    }
}
/************************************************************************
* FUNCTION
*
*       bcmms_find_longest_sequence   
*
* DESCRIPTION
*
*       Finds the longest sequence in the list.
*
* INPUTS
*
*       cb                      Control block
*       ret_start               First slot in the sequence
*       ret_count               Number of sequential items in the list.
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NUF_BADPARM             Invalid pointer given
*
*************************************************************************/
STATIC STATUS bcmms_find_longest_sequence(BCMMS_CB *cb, BCMMS_SLOT **ret_start, UNSIGNED *ret_count)
{
    STATUS ret_status;
    
    /* Verify pointers before we start dereferencing */
    if ((cb == NU_NULL) || (ret_start == NU_NULL) || (ret_count == NU_NULL))
    {
        ret_status = NUF_BADPARM;        
    }
    else
    {
        ret_status = NU_SUCCESS;
        
        /* Early exit if cache is empty */
        if (cb->num_members == cb->empty_count)
        {
            *ret_count = 0;
            *ret_start = NU_NULL;
        }
        else
        {
            /* Track the current maximum number of consecutive blocks
               and the starting node of that list */
            UNSIGNED max_count = 0;
            UNSIGNED current_count;
            UNSIGNED current_block_number;
            BCMMS_SLOT *max_start_node = NU_NULL;        
            BCMMS_SLOT *search_ptr;
            BCMMS_SLOT *current_start_node;
            
            /* Initialize for starting with a single node in the list */
            search_ptr = cb->slot_head;
            current_start_node = cb->slot_head;
            current_block_number = search_ptr->block_number;
            current_count = 1;
            
            do
            {
                /* If the next node is the next block in the sequence, 
                   continue with the current sequence */
                if ((current_block_number + 1) == (search_ptr->next)->block_number)
                {
                    search_ptr = search_ptr->next;
                    current_block_number = search_ptr->block_number;
                    current_count++;    
                }
                else
                {
                    /* Next item is not in the sequence. Test if the current
                       sequence is larger than max sequence found */
                    if (current_count > max_count)
                    {
                        max_count = current_count;
                        max_start_node = current_start_node;
                    }
                    
                    /* Start the next sequence search */
                    search_ptr = search_ptr->next;
                    current_start_node = search_ptr;
                    current_block_number = search_ptr->block_number;
                    current_count = 1; 
                }
            } while (search_ptr != cb->slot_head );
            
            *ret_start = max_start_node;
            *ret_count = max_count;
        }
    }
    
    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       bcmms_remove_from_list   
*
* DESCRIPTION
*
*       Removes the given item from the list. NOTE: Assumes the given
*       item is in the list.
*
* INPUTS
*
*       cb                      Control block
*       node                    Node to be removed
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID bcmms_remove_from_list(BCMMS_CB *cb, BCMMS_SLOT *node)
{
    
    if ((cb != NU_NULL) && (node != NU_NULL))
    {    
        /* Determine if this is the only node in the system.  */
        if (node->prev == node)
        {
            /* Yes, this is the only node in the system.  Clear the node's
               pointers and the head pointer.  */
            cb->slot_head = NU_NULL;
        }
        else
        {
            /* Unlink the node from a multiple node list.  */
            (node->prev)->next = node->next;
            (node->next)->prev = node->prev;
    
            /* Check to see if the node to delete is at the head of the
               list. */
            if (node == cb->slot_head)
            {
                /* Move the head pointer to the node after.  */
                cb->slot_head = node->next;
            }
        }
    }
}
/************************************************************************
* FUNCTION
*
*       bcmms_perform_IO   
*
* DESCRIPTION
*
*       Perform the IO request. If multi-sector access fails, attempt to
*       do single sector IO. Errors encountered during the single sector
*       IO are logged to the cache error queue. Will attempt to send
*       at most BCMMS_FIXED_XFER_BUFFER_BLOCK_COUNT sectors in a single
*       request. The calling function is responsible for requests where
*       count is > the max multi-sector request size. 
*
* INPUTS
*
*       cb                      Control block
*       start_node              First node in the linked sequence
*       count                   Number of nodes in sequence to process
*
* OUTPUTS
*
*       NU_SUCCESS              Operation is successful
*       NUF_BADPARM             Invalid input parameter
*
*************************************************************************/
STATIC STATUS bcmms_perform_IO(BCMMS_CB *cb, BCMMS_SLOT *start_node, UNSIGNED count)
{
    STATUS      ret_status, io_status;
    BCMMS_SLOT  *cur_node;
    UINT8       *dest;
    UNSIGNED    cur_count = 0;

    /* Verify parameters */
    if ((cb == NU_NULL) || (start_node == NU_NULL))
    {
        ret_status = NUF_BADPARM;
    }
    else if (count == 0)
    {
        /* short cut out of processing */
        ret_status = NU_SUCCESS;
    }
    else
    {
        /* Set return status, IO errors are async and reported via error queue */
        ret_status = NU_SUCCESS;
        
        /* Process the multi-sector request by creating a single large buffer
           of all the slots' data blocks in the input group */
        
        cur_node = start_node;
        dest = (UINT8*)(cb->xfer_buffer);
        
        /* Gather all data blocks in to the single transfer pool */
        while ((cur_count < count) && (cur_count < BCMMS_FIXED_XFER_BUFFER_BLOCK_COUNT))
        {
            /* Copy the node's data */
            NUF_Copybuff((VOID*)dest, cur_node->block_data, cb->block_size);
        
            cur_count++;
            dest += cb->block_size;
            cur_node = cur_node->next;
        }  
    
        /* Perform the IO request */
        io_status = FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(cb->dh)].fsdh_fdev->fdev_ops.io_proc(cb->dh,
                                 start_node->block_number, cb->xfer_buffer, (UINT16) cur_count, 0);
    
        if (io_status != NU_SUCCESS)
        {
            BCMMS_SLOT  *ss_node;
            UNSIGNED    i;
            
            ss_node = start_node;
            
            /* Multi-sector transfer failed, do the transfer block by block */
            for (i = 0; i < cur_count; ++i)
            {
                io_status = FS_Disk_Handles[FSDH_GET_IDX_FROM_DH(cb->dh)].fsdh_fdev->fdev_ops.io_proc(cb->dh,
                                 ss_node->block_number, ss_node->block_data, 1, 0);
                if (io_status != NU_SUCCESS)
                {
                    /* Report the error to the cache error queue */
                    bcm_report_error(cb->dh, ss_node->block_number, BCM_ERROR_OPERATION_WRITE, io_status);                    
                }
                
                /* Go to the next node */
                ss_node = ss_node->next;
            }
        }
        
        /* Remove the nodes that have been processed */
        cur_node = start_node;
    
        while (cur_count > 0)
        {
            BCMMS_SLOT *next_node;
            
            next_node = cur_node->next; /* track the next node so we don't lose position
                                           once we start removing nodes from the list */
            /* Remove from the list */                                       
            bcmms_remove_from_list(cb, cur_node);
            /* Return the resources from the removed slot */                                       
            bcmms_deallocate_slot(cb, cur_node);
            cur_node = next_node;
            cur_count--; 
        }
    }
    
    return (ret_status);
}
/************************************************************************
* FUNCTION
*
*       bcmms_process_threshold   
*
* DESCRIPTION
*
*       Processes the cache for flushing blocks when a threshold has been
*       reached. Returning from routine guarantees at most low_threshold
*       slots are occupied. 
*
* INPUTS
*
*       cb                      Control block
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID bcmms_process_threshold(BCMMS_CB *cb)
{
    STATUS      status;
    BCMMS_SLOT  *start_slot;
    UNSIGNED    count;
    
    if (cb != NU_NULL)
    {
        if ((cb->num_members - cb->empty_count) >= cb->high_threshold)
        {
            /* The high threshold has been reached. Process list to reduce
               count to at least the low threshold. */
            while ((cb->num_members - cb->empty_count) > cb->low_threshold)
            {
                status = bcmms_find_longest_sequence(cb, &start_slot, &count);                    
                
                if ((status != NU_SUCCESS) || (count == 0))
                {
                    /* There is an internal issue. Count should be at least 1. */
                    break;
                }
                else
                {
                    /* Send the list to IO. Ignore the return value. The 
                       parameters are known good, IO errors are sent to queue */
                    (VOID) bcmms_perform_IO(cb, start_slot, count);
                }
            }
        }  
    }
}
/************************************************************************
* FUNCTION
*
*       bcmms_is_block_in_cache 
*
* DESCRIPTION
*
*       Determines if the given block number is already in the cache. If
*       it is, a pointer to the containing slot is returned. 
*
* INPUTS
*
*       cb                      Control block
*       block_number            Block number to look for
*       ret_slot                Slot containing the found block        
*
* OUTPUTS
*
*       NU_SUCCESS              Block is in cache
*       NUF_BADPARM             Bad parameter provided
*       NU_UNAVAILABLE          Block is not in cache
*
*************************************************************************/
STATIC STATUS bcmms_is_block_in_cache(BCMMS_CB *cb, UINT32 block_number, BCMMS_SLOT **ret_slot)
{
    STATUS ret_status;
    BCMMS_SLOT *cur_node;
    
    if ((cb == NU_NULL) || (ret_slot == NU_NULL))
    {
        ret_status = NUF_BADPARM;
    }
    else if (cb->slot_head == NU_NULL)
    {
        /* List is empty. Don't bother with the search */
        ret_status = NU_UNAVAILABLE;
    }
    else
    {
        /* Assume block is not in cache */
        ret_status = NU_UNAVAILABLE;        
        
        cur_node = cb->slot_head;
        while (block_number >= cur_node->block_number)
        {
            if (block_number == cur_node->block_number)
            {    
                /* Found it. Setup return pointer */
                *ret_slot = cur_node;
                ret_status = NU_SUCCESS;
                break;
            }
            else if (cur_node->next == cb->slot_head)
            {
                /* Hit the end of the list with no match */
                break;
            }
            else
            {
                cur_node = cur_node->next;
            }
        }
    }
    
    return (ret_status);   
}
#endif /*(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE_REORDER == 1) */
