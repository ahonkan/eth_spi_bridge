/************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME
*
*        nu_usbh_ehci_imp.c
*
* COMPONENT
*
*       Nucleus USB Host software
*
* DESCRIPTION
*
*       This file contains the internal function definitions for the
*       EHCI driver
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*      ehci_alloc_qh                   Allocates a QH
*      ehci_alloc_qtd                  Allocates a QTD
*      ehci_check_retire_transfer      Checks and Retires a transfer
*      ehci_create_qtd                 Creates a Qtd
*      ehci_dealloc_qh                 Deallocates a QH
*      ehci_dealloc_qtd                Deallocates a QTD
*      ehci_fill_qtd                   Initializes a QTD
*      ehci_find_best_periodic_schedule Finds a frame for a given QH
*      ehci_find_uframe_index          Finds a uframe for a given QH
*      ehci_insert_qtd                 Inserts the head qTD into a QH
*      ehci_link_async_qh              Links an QH into async schedule
*      ehci_normalize_status           Converts EHCI status to USB status
*      ehci_prepare_burst              Prepares a burst in a transfer
*      ehci_qh_handle_ctrl_irp         Handles control transfers
*      ehci_qh_handle_data_irp         Handles Bulk/Interrupt transfers
*      ehci_qh_handle_irp              Handles transfers
*      ehci_qh_key                     Returns the key for a QH
*      ehci_retire_qh                  Retires a QH
*      ehci_retire_transfer            Retires a Transfer
*      ehci_rh_cl_clear_feature        Clear feature handling for EHCI RH
*      ehci_rh_cl_get_status           Get Status handling for EHCI RH
*      ehci_rh_cl_set_feature          Set feature handling for EHCI RH
*      ehci_rh_class_request           Class request handling for EHCI RH
*      ehci_rh_fill_rh_descriptor      Creates the descriptor from Registers
*      ehci_rh_get_descriptor          Get Descriptor handling for EHCI RH
*      ehci_rh_handle_irp              Data transfers from EHCI RH
*      ehci_rh_init                    EHCI RH Initialization
*      ehci_rh_invalid_cmd             Handles all invalid ctrl requests
*      ehci_rh_isr                     Root Hub/port interrupt servicing
*      ehci_rh_nothing_to_do_cmd       Always success ctrl requests
*      ehci_rh_std_request             Standard control request processing
*      ehci_scan_async_list            Scans async list for retired QHs
*      ehci_scan_periodic_list         Scans periodic list for retired QHs
*      ehci_schedule_intr_pipe         Schedules an interrupt QH
*      ehci_schedule_periodic_qh       Schedules a periodic QH
*      ehci_unlink_async_qh            Unlinks QH from async list
*      ehci_unlink_periodic_qh         Unlinks QH from periodic list
*      hash_add_qh                     Adds QH to a hash table
*      hash_delete_qh                  Deletes QH from the hash table
*      hash_find_qh                    Finds a QH in a hash table
*
* DEPENDENCIES
*
*       nu_usb.h                       All USB Definitions
*       nu_usbh_ehci_dvm.h
*
************************************************************************/

/* ==============  Standard Include Files ============================  */

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "drivers/nu_usbh_ehci_ext.h"
#include "drivers/nu_drivers.h"

/* ====================  Function Definitions ========================= */

/************************************************************************
*
* FUNCTION
*
*      ehci_unlink_periodic_qh
*
* DESCRIPTION
*
*      Unlinks a periodic QH from the periodic list.
*
* INPUTS
*
*      ehci    handle that identifies the EHCI controller.
*      qhinfo  the QH to be unlinked
*
* OUTPUTS
*
*      None
*
*************************************************************************/

VOID ehci_unlink_periodic_qh (NU_USBH_EHCI * ehci,
                              USBH_EHCI_QH_INFO * qhinfo)
{
    USBH_EHCI_QH *qh, **prev, *cur;

    UINT32 temp1, temp2;
    INT i;
    /* Mask for off-loading micro frames */
    UINT8 frame_mask;
    /* Loop counter for removing micro-frame load. */
    UINT8 loop;

    USBH_EHCI_PROTECT;

    if(qhinfo->type == USB_EP_INTR)
    {

        qh = (USBH_EHCI_QH *) ((UINT32) (qhinfo->hwqh) & (~0x1F));

        /* Periodic pipes.
         * We know the frame_index on which this QH is placed in the
         * periodic list. Search for this QH in the periodic list, starting
         * from that frame, in the interval of this QH, remove this QH.
         */

        /* For each frame this QH is scheduled, */

        for (i = qhinfo->index_per_tbl; i < USBH_EHCI_FRAME_LIST_SIZE;
             i += qhinfo->interval)
        {
            /* Find where this QH is linked in the frame */

            prev = (USBH_EHCI_QH **) & ehci->periodic_list_base[i];

            cur = *prev;
            temp2 = (UINT32)&cur;

            EHCI_READ_ADDRESS(ehci, temp2, temp1);

            cur = (USBH_EHCI_QH *) (temp1 & ~0x1E);

            while ((!((UINT32) (cur) & 0x01)) && (cur != qh))
            {
                prev = (USBH_EHCI_QH **) & (cur->next);

                EHCI_READ_ADDRESS(ehci, &cur->next, temp1);

                cur = (USBH_EHCI_QH *) (temp1 & ~0x1E);
            }

            /* We found where this QH lies in the schedule */
            /* unlink this */

            *prev = (USBH_EHCI_QH *) (qh->next);

            /* Update the load on this frame */

            ehci->frame_load[i] -= qhinfo->load;

            /* we also have micro frame level loading maintained */

            if ((qhinfo->speed != USB_SPEED_HIGH) &&
                ((qhinfo->bEndpointAddress & USB_DIR_IN) == USB_DIR_IN) &&
                (qhinfo->uframe < 5))
            {
                /* The load is with c-spilt that may in any of the next 3
                 * uframes.
                 */
                ehci->uframe_load[i][qhinfo->uframe + 1] -= qhinfo->load;
                ehci->uframe_load[i][qhinfo->uframe + 2] -= qhinfo->load;
                ehci->uframe_load[i][qhinfo->uframe + 3] -= qhinfo->load;
            }
            else
            {
                ehci->uframe_load[i][qhinfo->uframe] -= qhinfo->load;
            }
        }
    }
    else
    {

        /* Update the load on this frame */

        ehci->frame_load[qhinfo->index_per_tbl] -= qhinfo->load;

        if ((qhinfo->speed != USB_SPEED_HIGH) &&
                ((qhinfo->bEndpointAddress & USB_DIR_IN) == USB_DIR_IN))
        {
            /* For IN transfers load is in complete split packets. */
            frame_mask = (qhinfo->mask & 0x0000FF00) >> 8;
        }
        else
        {
            /* For high speed or full speed OUT packets load is in
             * start split frames.
             */
            frame_mask = (qhinfo->mask & 0x000000FF);
        }

        for ( loop =0; loop < 8; loop++ )
        {
           /* If a packet is scheduled in this micro-frame, off load
            * it.
            */
            if ( frame_mask & ( 0x01 << loop ) )
            {
                /* Subtract the load of this endpoint form this
                 * micro-frame.
                 */
                ehci->uframe_load [qhinfo->index_per_tbl][loop] -= qhinfo->load;

                /* In case of error reinitialize uframe_load for this
                 * micro-frame.
                 */
                if ( (INT32 ) ehci->uframe_load[qhinfo->index_per_tbl][loop] < 0 )
                {
                    ehci->uframe_load [qhinfo->index_per_tbl][loop] = NU_NULL;
                }
            }
        }


    }

    USBH_EHCI_UNPROTECT;
    return;
}

/************************************************************************
*
* FUNCTION
*
*      ehci_unlink_async_qh
*
* DESCRIPTION
*
*      Unlinks a async QH from the async list.
*
* INPUTS
*
*      ehci    handle that identifies the EHCI controller.
*      qhinfo  the QH to be unlinked
*
* OUTPUTS
*
*      None
*
*************************************************************************/

VOID ehci_unlink_async_qh (NU_USBH_EHCI * ehci,
                           USBH_EHCI_QH_INFO * qh_info)
{

    USBH_EHCI_QH *qh, *cur;
    UINT32 temp1;

    /* Async QH.
     * Find the QH in the async qh list.
     * unlink the QH from the list,
     * Uncache the Async List, wait for the AAE bit to be set,
     * now we are sure hw is no longer processing our QH
     */

    qh = (USBH_EHCI_QH *) ((UINT32) (qh_info->hwqh) & (~0x1F));

    EHCI_READ_ADDRESS(ehci, &qh->next, temp1);

    cur = (USBH_EHCI_QH *) (temp1 & (~0x1F));

    cur->prev = qh->prev;
    qh->prev->next = qh->next;

    /* now turn on the async doorbell. this makes the HC notice the
     * link change we made */

    EHCI_HW_SET_BITS (ehci, ehci->ehci_oprreg_base + USBCMD, (0x1 << 6));

    /* HC will acknowledge the change in at max 1ms */

    do
    {
        /* This bit will be reset to '0' when the HC acknowledges */
        EHCI_HW_READ32 (ehci, ehci->ehci_oprreg_base + USBCMD, temp1);
    }

    /* On some targets this bit does not set automatically and we need to move on. */
#if (CFG_NU_OS_DRVR_USB_HOST_EHCI_DOORBELL_BIT_FIX == NU_TRUE)
    while(0);
#else
    while(temp1 & (0x1 << 6));
#endif

    return;
}

/************************************************************************
*
* FUNCTION
*
*      ehci_link_async_qh
*
* DESCRIPTION
*
*      links a async QH into the async list.
*
* INPUTS
*
*      ehci    handle that identifies the EHCI controller.
*      qhinfo  the QH to be linked
*
* OUTPUTS
*
*      None
*
************************************************************************/

VOID ehci_link_async_qh (NU_USBH_EHCI * ehci,
                         USBH_EHCI_QH_INFO * qh_info)
{
    USBH_EHCI_QH *stored_next_qh;
    UINT32 temp1;



    EHCI_READ_ADDRESS(ehci, &ehci->dummy_qh->next, temp1);
    stored_next_qh = (USBH_EHCI_QH *) ( temp1 & ~0x1F);

    qh_info->hwqh->prev = ehci->dummy_qh;
    qh_info->hwqh->next = ehci->dummy_qh->next;

    temp1 = (((UINT32) (qh_info->hwqh)) | 0x02);

    EHCI_WRITE_ADDRESS( ehci, &ehci->dummy_qh->next, temp1);

    stored_next_qh->prev = qh_info->hwqh;

    qh_info->qh_state = USBH_EHCI_QH_READY;

    return;
}

/*************************************************************************
*
* FUNCTION
*
*      ehci_qh_key
*
* DESCRIPTION
*
*      returns a key for hashing QH in to hash table, based on the function
*      address and bEndpointAddress
*
* INPUTS
*
*      function_address  USB Function Address.
*      bEndpointAddress  bEndpointAddress field of the endpoint descriptor.
*
* OUTPUTS
*
*      key  to be used with hash function for hashing the QH
*
*************************************************************************/

UINT32 ehci_qh_key (UINT8 function_address,
                    UINT8 bEndpointAddress)
{
    UINT32 key = 0;

    /* The key is made of 7 bits of function_address, 8 bit endpoint
     * number of bEndpointAddress.
     */

    key |= (bEndpointAddress);
    key = key << 8;
    key |= (function_address & 0x7f);

    return (key);
}


/************************************************************************
*
* FUNCTION
*
*      ehci_alloc_qh
*
* DESCRIPTION
*
*      Allocates an QH at a memory address that is 32 byte aligned.
*
* INPUTS
*
*      ehci        handle that identifies the EHCI controller.
*      qh_ptr      Allocated QH pointer will be returned using this
*      suspend     suspend option - NU_SUSPEND or NU_NO_SUSPEND.
*
* OUTPUTS
*
*   NU_SUCCESS     Indicates successful allocation of the ED.
*   NU_NO_MEMORY   Memory not available.
*
*************************************************************************/

STATUS ehci_alloc_qh (NU_USBH_EHCI * ehci,
                      USBH_EHCI_QH ** qh_ptr,
                      UNSIGNED suspend)
{
    STATUS status = NU_SUCCESS;
    UINT8                   id;
    UINT8 pool_count;
    EHCI_ALIGNED_MEM_POOL   *pool;
    UINT8 pool_int_required = NU_TRUE;


    /* Check if EHCI's control block is valid. */
    NU_ASSERT(ehci != NU_NULL);

    USBH_EHCI_PROTECT;

    /* Search for a free QH in the allocated pools. */
    for (pool_count = 0; pool_count < USBH_EHCI_MAX_QH_POOLS; pool_count++)
    {
        pool = &(ehci->qh_pool[pool_count]);

        /* Check if Pool's control block is valid. */
        NU_ASSERT(pool != NU_NULL);


        /* check if this pool has been initialized and there are QHs available. */
        if ((pool->start_ptr) && (pool->avail_elements))
        {
            pool_int_required = NU_FALSE;

            break;
        }
    }

    if(pool_int_required == NU_TRUE)
    {
        /* See if a pool slot is free and if so allocate a new pool and grab
         * the QH from the newly allocated pool.
         */
        for (pool_count = 0; pool_count < USBH_EHCI_MAX_QH_POOLS; pool_count++)
        {
            status = NU_NO_MEMORY;

            pool = &(ehci->qh_pool[pool_count]);

            /* Check if Pool's control block is valid. */
            NU_ASSERT(pool != NU_NULL);


            if (pool->start_ptr == NU_NULL)
            {
                status = NU_Allocate_Aligned_Memory(ehci->cb.pool,
                                                (VOID **) &(pool->start_ptr),
                                                (USBH_EHCI_MAX_QHS_PER_POOL * sizeof(USBH_EHCI_QH)),
                                                USBH_EHCI_QH_TD_ALIGNMENT,
                                                suspend);

                if (status == NU_SUCCESS)
                {
                    /* Initialize the available elements in this pool. */
                    pool->avail_elements = USBH_EHCI_MAX_QHS_PER_POOL;
                 }

                break;
            }
        }
    }

    if((status == NU_SUCCESS) && (pool_count != USBH_EHCI_MAX_QH_POOLS))
    {
        id = usb_get_id(pool->bits,
                         USBH_EHCI_MAX_QHS_PER_POOL / 8,
                         USBH_EHCI_MAX_QHS_PER_POOL - 1);

        if (id == USB_NO_ID)
        {
            NU_ASSERT(0);
            /* This was not suppose to happen as we have checked that elements are available. */
            status = NU_INVALID_MEMORY;
        }
        else
        {
            /* Check if Pool's start pointer is valid. */
            NU_ASSERT(pool->start_ptr != NU_NULL);

            *qh_ptr = (USBH_EHCI_QH *)
                      (pool->start_ptr + (id * sizeof(USBH_EHCI_QH)));

            pool->avail_elements--;
        }
     }

    USBH_EHCI_UNPROTECT;

    return ( status );

    }

/*************************************************************************
*
* FUNCTION
*
*      hash_delete_qh
*
* DESCRIPTION
*
*      Deletes the QH from the hash table. qhinfo->key is used
*      as the key to the hashing function.
*
* INPUTS
*
*      ehci   handle that identifies the EHCI controller.
*      qhinfo Identifies the QH that has to be deleted.
*
* OUTPUTS
*
*      NU_SUCCESS  Indicates that the QH has been successfully deleted
*                  from the QH hash table.
*      NU_NOT_PRESENT Indicates that the QH couldn't be located in the
*                     hash table.
*
*************************************************************************/

STATUS hash_delete_qh (NU_USBH_EHCI * ehci,
                       USBH_EHCI_QH_INFO * qhinfo)
{
    UINT32 index;
    USBH_EHCI_HASH *node;

    /* Find the index in to the hash table */

    index = USBH_EHCI_HASH_FUNC (qhinfo->key);

    node = ehci->qh_async_ht[index];

    while (node != NU_NULL)
    {
        if (node->qh_info == qhinfo)
        {
            if (node->key != qhinfo->key)
                NU_ASSERT(0);

            NU_Remove_From_List ((CS_NODE **) & (ehci->qh_async_ht[index]),
                                  (CS_NODE *) node);

            NU_Deallocate_Memory (node);

            return NU_SUCCESS;
        }

        node = (USBH_EHCI_HASH *) node->list.cs_next;

        if (node == ehci->qh_async_ht[index])
        {
            node = NU_NULL;
        }
    }

    return NU_NOT_PRESENT;
}

/*************************************************************************
*
* FUNCTION
*
*      hash_add_qh
*
* DESCRIPTION
*
*      It adds the QH in to the hash table. qhinfo->key is used as the
*      key to the hashing function. If more than a single qhinfo is mapped
*      to the same hashed entry, such colliding entries are in to the
*      linked list maintained for each hash entry.
*
* INPUTS
*
*      ehci    handle that identifies the EHCI controller.
*      qhinfo  Identifies the QH that has to be inserted.
*
* OUTPUTS
*      NU_SUCCESS   Indicates that the QH has been successfully added
*                           to the QH hash table.
*      NU_NO_MEMORY Indicates failure of memory allocation
*
*************************************************************************/

STATUS hash_add_qh (NU_USBH_EHCI * ehci,
                    USBH_EHCI_HASH ** ht,
                    USBH_EHCI_QH_INFO * qhinfo)
{
    STATUS status;
    USBH_EHCI_HASH *newnode;
    UINT32 index = USBH_EHCI_HASH_FUNC (qhinfo->key);

    USBH_EHCI_PROTECT;
    status =
        NU_Allocate_Memory (ehci->cacheable_pool, (VOID **) &newnode,
                            sizeof (USBH_EHCI_HASH), NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
        return status;

    /* Fill-n-place it in the list of the hash entry */

    newnode->qh_info = qhinfo;
    newnode->key = qhinfo->key;
    newnode->list.cs_next = NU_NULL;
    newnode->list.cs_previous = NU_NULL;

    NU_Place_On_List ((CS_NODE **) & (ht[index]), (CS_NODE *) newnode);
    USBH_EHCI_UNPROTECT;

    return NU_SUCCESS;
}

/*************************************************************************
*
* FUNCTION
*
*      hash_find_qh
*
* DESCRIPTION
*
*      It searches the QH hash table based on the hash key.
*
* INPUTS
*
*      ht      Hash table to search
*      key     Key that has to be used by the hashing function.
*
* OUTPUTS
*
*      Ptr to the QH info node that has been found using the key,
*      NU_NULL, if the key couldn't be successfully located in
*                   the hash table.
*
*************************************************************************/

USBH_EHCI_QH_INFO *hash_find_qh (USBH_EHCI_HASH ** ht,
                                 UINT32 key)
{
    USBH_EHCI_HASH *node;

    /* Find the index in to the hash table */
    node = ht[USBH_EHCI_HASH_FUNC (key)];

    while (node)
    {
        if (node->key == key)
            return (USBH_EHCI_QH_INFO *) (node->qh_info);

        node = (USBH_EHCI_HASH *) node->list.cs_next;

        if (node == ht[USBH_EHCI_HASH_FUNC (key)])
            node = NU_NULL;
    }

    return NU_NULL;
}


/************************************************************************
*
* FUNCTION
*
*      ehci_create_qtd
*
* DESCRIPTION
*
*      Allocates a QTD and places it to a burst
*
* INPUTS
*
*      ehci        EHCI controller handle
*      qh_info     QH to which this QTD belongs to
*      transfer    Transfer to which this QTD belongs to
*      burst       Burst to which this QTD belongs to
*      exceed_max_qtds Can we exceed the QTD limit?
*
* OUTPUTS
*
*      NU_SUCCESS      On Successful creation of the qtd
*      NU_NO_MEMORY    Indicates exhaustion of available Memory
*
*************************************************************************/

STATUS ehci_create_qtd (NU_USBH_EHCI * ehci,
                        USBH_EHCI_QH_INFO * qh_info,
                        USBH_EHCI_IRP * transfer,
                        USBH_EHCI_BURST * burst,
                        BOOLEAN exceed_max_qtds)
{
    STATUS status;
    USBH_EHCI_QTD *qtd;

    if ((burst->num_qtds == USBH_EHCI_MAX_QTDS_PER_BURST) &&
        (exceed_max_qtds == NU_FALSE))
    {
        return NU_USB_MAX_EXCEEDED;
    }

    /* Create a QTD */

    status = ehci_alloc_qtd (ehci, &burst->hwqtd[burst->num_qtds],
                            NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        return NU_NO_MEMORY;
    }

    /* Link burst and qtd */

    qtd = burst->hwqtd[burst->num_qtds];

    /* Initialize the Qtd */

    memset (qtd, 0, sizeof (USBH_EHCI_QTD));

    EHCI_HW_WRITE32(ehci, &qtd->next, 0x01);

    if((qh_info->type == USB_EP_BULK) && (qh_info->bEndpointAddress & 0x80))
    {
        EHCI_WRITE_ADDRESS(ehci, &qtd->alt_next,
                           (UINT32)(qh_info->dummy_qtd));
    }
    else
    {
        EHCI_HW_WRITE32(ehci, &qtd->alt_next, 0x01);
    }

    qtd->burst = burst;
    qtd->index = burst->num_qtds;

    burst->num_qtds++;


    return NU_SUCCESS;
}




/*************************************************************************
*
* FUNCTION
*
*      ehci_fill_qtd
*
* DESCRIPTION
*
*      Prepares a QTD with given information.
*
* INPUTS
*
*      ehci        EHCI controller handle
*      qtd         QTd to be filled in
*      buf         Data buffer pointer
*      length      Length of data transfer
*      token       Qtd Token
*
* OUTPUTS
*
*      count       Number of bytes filled in the QTD.
*
*************************************************************************/

UINT32 ehci_fill_qtd (NU_USBH_EHCI * ehci,
                      USBH_EHCI_QTD * qtd,
                      UINT8 *buf,
                      UINT32 length,
                      UINT32 token)
{
    UINT32 count;
    UINT32 buffer_addr = (UINT32) buf;
    INT i;
    UINT16 mps = qtd->burst->transfer->qh->maxpacket;

    /* EHCI allows each QTD to transfer a maximum of 20K bytes of data.
     * This is divided into 5 contiguous buffers. Each buffer can hold a
     * 'page' size of data ie, 4K bytes. The first buffer may start from
     * the middle of a page. Other buffers shall not.
     *
     * The input parameters for this function specify the length of the data
     * and the buffer pointer for data transfer. The following lines of
     * code will convert this into a QTD buffers, taking into account the
     * above mentioned EHCI requirements.
     * */

    /* The first buffer pointer */

    EHCI_WRITE_ADDRESS(ehci, &qtd->buffer[0], buffer_addr);

    /* find out how many bytes are to be transferred in this page */

    count = 0x1000 - (buffer_addr & 0x0fff);

    /* If the transfer is within this first page, don't bother. */

    if (length <= count)
    {
        count = length;
    }
    else
    {
        /* Data transfer extends across pages. So its the software Job to
         * divide this into buffers as EHCI requires.
         */

        /* The next buffer starts, aligned on a page boundary */

        buffer_addr += 0x1000;
        (buffer_addr) &= ~0x0fff;


        for (i = 1; (count < length) && (i < 5); i++)
        {
            /* next buffer pointer. */
            EHCI_WRITE_ADDRESS(ehci, &qtd->buffer[i], buffer_addr);

            buffer_addr += 0x1000;

            if ((count + 0x1000) < length)
                count += 0x1000;
            else
                count = length;
        }

        if (count < length)
        {
            count /= (mps);
            count *= (mps);
        }

    }

    /* QTD Token */

    EHCI_HW_WRITE32(ehci, &qtd->token, ((count << 16) | token));


    return count;
}

/*************************************************************************
*
* FUNCTION
*
*      ehci_fill_itd
*
* DESCRIPTION
*
*      Prepares a ITD with given information.
*
* INPUTS
*
*      ehci        EHCI controller handle
*      itd         ITD to be filled in
*      buf         Data buffer pointer
*      length      Length of data transfer
*      uframe      Micro frame number
*      ioc         Interrupt on completion flag
*      address     Endpoint address
*
* OUTPUTS
*
*      NU_SUCCESS      On Successful creation of the itd
*      NU_NO_MEMORY    Indicates exhaustion of available Memory
*
*************************************************************************/

UINT32 ehci_fill_itd (NU_USBH_EHCI * ehci,
                      USBH_EHCI_ITD * itd,
                      UINT8 *buf,
                      UINT32 length,
                      UINT16 uframe,
                      UINT8 ioc,
                      UINT32 address,
                      UINT8 direction)
{
    UINT32 count;
    UINT32 buffer_addr;
    UINT32 pg = itd->pg;
    UINT32 trans = 0;
#if (USBH_EHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)
    BOOLEAN sync = NU_TRUE;
#endif



    itd->data[uframe] = buf;
    itd->length[uframe] = length;

#if (USBH_EHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)

    if(direction & USB_DIR_IN)
        sync = NU_FALSE;

/*    NU_USB_HWENV_Normalize_Buffer(ehci->hwenv,
                              itd->data[uframe],
                              length,
                              1,
                              sync,
                              &itd->non_cache_ptr[uframe],
                              &itd->raw_data[uframe]);
*/
#else
    itd->non_cache_ptr[uframe] = buf;
#endif


    buffer_addr = (UINT32)itd->non_cache_ptr[uframe];

    itd->trans_per_td++;

    /* Set the active bit of ITD transaction. */
    trans = EHCI_ITD_ACTIVE;

    /* Set interrupt on complete field. */
    if(ioc)
        trans |= EHCI_ITD_IOC;

    /* Set page select (PG) field */
    trans |= (pg & 0x07) << 12;

    /* Transaction offset . */
    trans |= buffer_addr & EHCI_ITD_OFFSET;

    /* Transaction length. */
    trans |= length << 16;

    uframe &= 0x07;


    EHCI_HW_WRITE32(ehci, &itd->trans_info[uframe],trans);

    /* EHCI allows each ITD to transfer a maximum of 24K bytes of data.
     * This is divided into 7 contiguous buffers. Each buffer can hold a
     * 'page' size of data ie, 4K bytes. The first buffer may start from
     * the middle of a page. Other buffers shall not.
     *
     * The input parameters for this function specify the length of the data
     * and the buffer pointer for data transfer. The following lines of
     * code will convert this into ITD buffers, taking into account the
     * above mentioned EHCI requirements.
     * */

    buffer_addr = (buffer_addr & ~0x0fff) | (itd->buffer[pg] & 0x00000fff);

    /* The first buffer pointer */
    EHCI_WRITE_ADDRESS(ehci, &itd->buffer[pg], buffer_addr);

    /* find out how many bytes are to be transferred in this page */

    count = 0x1000 - (buffer_addr & 0x0fff);

    /* If the transfer is within this first page, don't bother. */

    if (length <= count)
    {
        count = length;

    }
    else
    {
        /* Data transfer extends across pages. So its the software Job to
         * divide this into buffers as EHCI requires.
         */
        ++pg;
        itd->pg = pg;
        /* The next buffer starts, aligned on a page boundary */
        buffer_addr += 0x1000;
        buffer_addr = (buffer_addr & ~0x0fff) | (itd->buffer[pg] & 0x00000fff);

        /* next buffer pointer. */
        EHCI_WRITE_ADDRESS(ehci, &itd->buffer[pg], buffer_addr);
    }

    return NU_SUCCESS;
}

/*************************************************************************
*
* FUNCTION
*
*      ehci_fill_sitd
*
* DESCRIPTION
*
*      Prepares a SITD with given information.
*
* INPUTS
*
*      ehci         EHCI controller handle
*      sitd         SITD to be filled in
*      buf          Data buffer pointer
*      length       Length of data transfer
*      ioc          Interrupt on completion flag
*      direction    Direction of endpoint
*      mask         Start split and complete split mask.
*      address      Endpoint address
*
*
* OUTPUTS
*
*      NU_SUCCESS      On Successful creation of the sitd
*      NU_NO_MEMORY    Indicates exhaustion of available Memory
*
*************************************************************************/

UINT32 ehci_fill_sitd (NU_USBH_EHCI *ehci,
                      USBH_EHCI_SITD *sitd,
                      UINT8 *buf,
                      UINT32 length,
                      UINT8 ioc,
                      UINT8 direction,
                      UINT32 mask,
                      UINT32 ssplit_info,
                      UINT32 address)
{
    UINT32 count;
    UINT32 buffer_addr;
    UINT32 trans =0;
    UINT32 temp,offset;

#if (USBH_EHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)
    BOOLEAN sync = NU_TRUE;
#endif


    sitd->data = buf;
    sitd->length = length;

#if (USBH_EHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)

    if(direction & USB_DIR_IN)
        sync = NU_FALSE;

/*    NU_USB_HWENV_Normalize_Buffer(ehci->hwenv,
                              sitd->data,
                              length,
                              1,
                              sync,
                              &sitd->non_cache_ptr,
                              &sitd->raw_data);
*/
#else
    sitd->non_cache_ptr = sitd->data;
#endif

    buffer_addr = (UINT32)sitd->non_cache_ptr;

    /* Set the active bit of SITD transaction. */
    trans = EHCI_SITD_ACTIVE;

    /* Set interrupt on complete field for transaction. */
    if(ioc)
        trans |= EHCI_SITD_IOC;

    /* Transaction length. */
    trans |= length << 16;

    EHCI_HW_WRITE32(ehci, &sitd->results,trans);

    EHCI_HW_WRITE32(ehci, &(sitd->ep_controlinfo1),
                                    address);


    /* EHCI allows each SITD to transfer a maximum of 1023 bytes of data.
     * This is divided into 2 contiguous buffers. Each buffer can hold a
     * 'page' size of data ie, 4K bytes. The first buffer may start from
     * the middle of a page. Other buffers shall not.
     *
     * The input parameters for this function specify the length of the data
     * and the buffer pointer for data transfer. The following lines of
     * code will convert this into SITD buffers, taking into account the
     * above mentioned EHCI requirements.
     * */

    /* The first buffer pointer */
    EHCI_WRITE_ADDRESS(ehci, &sitd->buffer[0], buffer_addr);

    /* find out how many bytes are to be transferred in this page */

    count = 0x1000 - (buffer_addr & 0x0fff);

    /* If the transfer is within this first page, don't bother. */

    if (length <= count)
    {
        count = length;
        temp  = buffer_addr + length;
        offset = temp & ~0x0fff;
    }
    else
    {
        count = length;
        /* The next buffer starts, aligned on a page boundary */
        temp = buffer_addr + 0x1000;
        offset = temp & ~0x0fff;
    }


    if(direction != USB_DIR_IN)
    {
        offset |= ssplit_info;
    }

     /* next buffer pointer. */
     EHCI_WRITE_ADDRESS(ehci, &sitd->buffer[1], offset);
     EHCI_HW_WRITE32(ehci, &sitd->ep_controlinfo2, mask);



    return NU_SUCCESS;
}
/************************************************************************
*
* FUNCTION
*
*      ehci_insert_qtd
*
* DESCRIPTION
*
*      Inserts a QTD into a QH.
*
* INPUTS
*
*      ehci        EHCI controller handle
*      qtd         QTd to be filled in
*      qh_info     QH to which this QTD must be placed
*
* OUTPUTS
*
*      None
*
*************************************************************************/

VOID ehci_insert_qtd (NU_USBH_EHCI * ehci,
                      USBH_EHCI_QH_INFO * qh_info,
                      USBH_EHCI_QTD * qtd)
{

    USBH_EHCI_QTD *cur;
    UINT32 temp1;

    EHCI_READ_ADDRESS(ehci, &qh_info->hwqh->next_qtd, temp1);

    if (temp1 == 1)
    {
        EHCI_WRITE_ADDRESS(ehci, &qh_info->hwqh->next_qtd, (UINT32)(qtd));

        qh_info->qtd_head = qtd;
    }
    else
    {
        cur = (USBH_EHCI_QTD *) ((temp1) & 0xffffffe0);

        EHCI_READ_ADDRESS(ehci, &cur->next, temp1);

        /* Search until T-bit set is seen */
        while ((temp1 & 0x1) != 1)
        {
            cur = (USBH_EHCI_QTD *) (temp1 & 0xffffffe0);
            EHCI_READ_ADDRESS(ehci, &cur->next, temp1);

        }
        EHCI_WRITE_ADDRESS(ehci, &cur->next, (UINT32)(qtd));

    }

    if (qh_info->qtd_head == NU_NULL)
    {
        qh_info->qtd_head = qtd;
    }
}

/************************************************************************
*
* FUNCTION
*
*      ehci_alloc_qtd
*
* DESCRIPTION
*
*      Allocates a QTD at a memory address that is 32 byte aligned. This
*      can be used for Control/Bulk/Interrupt QTDs.
*
* INPUTS
*
*    ehci      handle that identifies the EHCI controller.
*    qtd_ptr   Return ptr that would contain the address of the
*              TD upon successful allocation.
*    suspend   suspend option - NU_SUSPEND or NU_NO_SUSPEND.
*
* OUTPUTS
*
*   NU_SUCCESS     Indicates successful allocation of the QH.
*   NU_NO_MEMORY   Indicates that the memory allocation has
*                  failed.
*
*************************************************************************/

STATUS ehci_alloc_qtd (NU_USBH_EHCI * ehci,
                       USBH_EHCI_QTD ** qtd_ptr,
                       UNSIGNED suspend)
{
    STATUS status = NU_SUCCESS;
    UINT8                   id;
    UINT8 pool_count;
    EHCI_ALIGNED_MEM_POOL   *pool;
    UINT8 pool_int_required = NU_TRUE;


    /* Check if EHCI's control block is valid. */
    NU_ASSERT(ehci != NU_NULL);

    USBH_EHCI_PROTECT;

    /* Search for a free QTD in the allocated pools. */
    for (pool_count = 0; pool_count < USBH_EHCI_MAX_QTD_POOLS; pool_count++)
    {
        pool = &(ehci->qtd_pool[pool_count]);

        /* Check if Pool's control block is valid. */
        NU_ASSERT(pool != NU_NULL);


        /* check if this pool has been initialized and there are TDs available. */
        if ((pool->start_ptr) && (pool->avail_elements))
        {
            pool_int_required = NU_FALSE;

            break;
        }
    }

    if(pool_int_required == NU_TRUE)
    {
        /* See if a pool slot is free and if so allocate a new pool and grab
         * the TD from the newly allocated pool.
         */
        for (pool_count = 0; pool_count < USBH_EHCI_MAX_QTD_POOLS; pool_count++)
        {
            status = NU_NO_MEMORY;

            pool = &(ehci->qtd_pool[pool_count]);

            /* Check if Pool's control block is valid. */
            NU_ASSERT(pool != NU_NULL);


            if (pool->start_ptr == NU_NULL)
            {
                status = NU_Allocate_Aligned_Memory(ehci->cb.pool,
                                                (VOID **) &(pool->start_ptr),
                                                (USBH_EHCI_MAX_QTDS_PER_POOL * sizeof(USBH_EHCI_QTD)),
                                                USBH_EHCI_QH_TD_ALIGNMENT,
                                                suspend);

                if (status == NU_SUCCESS)
                {
                    /* Initialize the available elements in this pool. */
                    pool->avail_elements = USBH_EHCI_MAX_QTDS_PER_POOL;
                 }

                break;
            }
        }
    }

    if((status == NU_SUCCESS) && (pool_count != USBH_EHCI_MAX_QTD_POOLS))
    {
        id = usb_get_id(pool->bits,
                         USBH_EHCI_MAX_QTDS_PER_POOL / 8,
                         USBH_EHCI_MAX_QTDS_PER_POOL - 1);

        if (id == USB_NO_ID)
        {
            NU_ASSERT(0);
            /* This was not suppose to happen as we have checked that elements are available. */
            status = NU_INVALID_MEMORY;
        }
        else
        {
            /* Check if Pool's start pointer is valid. */
            NU_ASSERT(pool->start_ptr != NU_NULL);


            *qtd_ptr = (USBH_EHCI_QTD *)
                      (pool->start_ptr + (id * sizeof(USBH_EHCI_QTD)));

            pool->avail_elements--;
        }
     }

    USBH_EHCI_UNPROTECT;

    return ( status );


}

/************************************************************************
*
* FUNCTION
*
*      ehci_dealloc_qtd
*
* DESCRIPTION
*
*      Deallocates the QTD that was earlier allocated from EHCI pool.
*
* INPUTS
*
*      ehci      handle that identifies the EHCI controller.
*      qtd_ptr   QTD ptr that has to be released.
*
* OUTPUTS
*
*      NU_SUCCESS          Indicates successful deallocation of the QTD.
*      NU_INVALID_POINTER  Indicates that the qtd_ptr is invalid.
*
*
*************************************************************************/

STATUS ehci_dealloc_qtd (NU_USBH_EHCI * ehci,
                         USBH_EHCI_QTD * qtd_ptr)
{
    EHCI_ALIGNED_MEM_POOL   *pool;
    UINT32 first_td;
    UINT32 last_td;
    UINT8 pool_count;
    STATUS status = NU_INVALID_MEMORY;
    UINT8 id;

    USBH_EHCI_PROTECT;

    /* Search for a free TD in the allocated pools. */
    for (pool_count = 0; pool_count < USBH_EHCI_MAX_QTD_POOLS; pool_count++)
    {
        /* Get the control block of the pool to which this TD belongs. */
        pool = &(ehci->qtd_pool[pool_count]);

        /* Get the address of the first TD form this pool. */
        first_td = (UINT32) pool->start_ptr;

        /* Get the address of the last TD by adding total number of TDs in the start address. */
        last_td = first_td + (sizeof(USBH_EHCI_QTD) * (USBH_EHCI_MAX_QTDS_PER_POOL - 1));

        /* Check if the TD belongs to this pool. */
        if(((UINT32) qtd_ptr >= first_td) && ((UINT32) qtd_ptr <= last_td))
        {
            /* Calculate the ID of this TD. */
            id = (((UINT32)qtd_ptr - first_td) / sizeof(USBH_EHCI_QTD));

            /* Mark the TD as free. */
            usb_release_id(pool->bits, (USBH_EHCI_MAX_QTDS_PER_POOL / 8), id);

            /* Increment number of available TDs in this pool. */
            pool->avail_elements++;

           /* Always have at least one pool , the first pool  to prevent
             * memory fragmentation due to frequent pool
             * allocation and deallocation.
             */
           if ((pool->avail_elements == USBH_EHCI_MAX_QTDS_PER_POOL) &&
               (pool_count!= NU_NULL))
           {
                /* No TDs of this pool are in use, so release it. */
                NU_Deallocate_Memory(pool->start_ptr);

                memset(pool, 0, sizeof(EHCI_ALIGNED_MEM_POOL));
           }

           status = NU_SUCCESS;

           break;
        }
    }

    USBH_EHCI_UNPROTECT;

    return ( status );
}

/************************************************************************
*
* FUNCTION
*
*       ehci_qh_handle_ctrl_irp
*
* DESCRIPTION
*
*       Handles translating the IRP in to Control QTDs required to
*       initiate the transfer.
*
* INPUTS
*
*       ehci    handle that identifies the EHCI controller.
*       qhinfo  Identifies the QH over which the IRP is to be
*               transferred.
*       transfer Transfer associated with the IRP
*       setup   8 byte setup data to be transferred
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates that the transfer has been
*                               successfully setup.
*       NU_USB_INVLD_BUFF_LEN   Indicates that the irp length is
*                               greater than 16k for control transfer.
*       NU_NO_MEMORY            Indicates that memory allocation failed.
*
*************************************************************************/

static STATUS ehci_qh_handle_ctrl_irp (NU_USBH_EHCI * ehci,
                                       USBH_EHCI_QH_INFO * qhinfo,
                                       USBH_EHCI_IRP * transfer,
                                       NU_USB_SETUP_PKT * setup)
{
    UINT32 token = 0;

    UINT8 *data = transfer->rem_data;
    UINT32 data_length = transfer->rem_length;
    UINT8 direction = transfer->direction;
    STATUS status;
    BOOLEAN sync = NU_TRUE;
    BOOLEAN buff_type;
    UINT32  temp1;

    USBH_EHCI_BURST *burst = &transfer->burst[0];

    /* tokens for individual QTDs...    */

    /* Qtd status set active */

    token = (0x1 << 7);         /* Status   */

    /* set allowable error count 3 */

    token |= (0x03 << 10);      /* Cerr     */

    /* For Control transfers, we require at most, 3 QTds. */
    /* First for SETUP phase,
     * Second for Data phase(if any), note that a max of 16K is only allowed for
     * data transfers, and a QTd allows 16-20K per QTd. So, one QTD is
     * sufficient.
     * Third for a STATUS phase for an empty length packet
     **/

    /* SETUP Phase QTD  */

    status = ehci_create_qtd (ehci, qhinfo, transfer, burst, NU_FALSE);

    /* Check if there are errors during creating QTDs */
    if (status != NU_SUCCESS)
        return status;

    /* Normalization of buffers */

    burst->data = (UINT8 *) setup;
    /* after normalization */

    burst->length = 8;

    /* Get buffer type, cachable or non-cachable. */
    NU_USB_IRP_Get_Buffer_Type_Cachable(transfer->irp, &buff_type);
    if ( buff_type == NU_TRUE )
    {
        status = ehci_normalize_buffer (ehci, burst->data, 8, 1,
                                NU_TRUE, &burst->norm_data,&burst->raw_data);
    }
    else
    {
        status              = NU_SUCCESS;
        burst->norm_data    = burst->data;
        burst->raw_data     = burst->data;
    }

    if(status != NU_SUCCESS)
        return (status);

    /* Fill the qTd with setup PID */
    ehci_fill_qtd (ehci, burst->hwqtd[0], burst->norm_data, burst->length,
                   token | (2 << 8));

    /* Data and Status Phase has the Data 1 toggle. */

    token |= 0x80000000;        /* qtd toggle (0x01 << 31 ) */

    /* Check if this transfer has a data phase at all */

    if (data_length != 0)
    {
        /* This is the data phase QTD */

        /* PID field    */

        if (direction == USB_DIR_IN)
        {
            token |= (0x1 << 8);
            sync = NU_FALSE;
        }

        /* Create and link the data phase QTd    */
        status = ehci_create_qtd (ehci, qhinfo, transfer, burst, NU_FALSE);

        /* Check if there are errors during creating QTDs */
        if (status != NU_SUCCESS)
            return status;

        /* Normalization of buffers */

        transfer->burst[1].data = (UINT8 *) data;
        /* after normalization */
        transfer->burst[1].norm_data = (UINT8 *) data;
        transfer->burst[1].length = data_length;

        NU_USB_IRP_Get_Buffer_Type_Cachable(transfer->irp, &buff_type);
        if ( buff_type == NU_TRUE )
        {
            status = ehci_normalize_buffer (ehci, data, data_length, 1,
                                          sync, &transfer->burst[1].norm_data,
                                          &transfer->burst[1].raw_data);
        }
        else
        {
            status = NU_SUCCESS;
            transfer->burst[1].norm_data    = data;
            transfer->burst[1].raw_data     = data;
        }

        if(status != NU_SUCCESS)
            return (status);

        /* Fill the qTd */
        ehci_fill_qtd (ehci,burst->hwqtd[1], transfer->burst[1].norm_data,
                       transfer->burst[1].length, token);

        /* Undo the token PID changes */
        token &= ~(0x1 << 8);
    }

    /* Status Phase PID */

    /* If data phase is IN then an OUT token goes as a status phase. */
    /* Otherwise, an IN token goes as a status phase                */
    /* If there is no data phase, IN token is the status phase      */

    if (direction == USB_DIR_OUT)
    {
        token |= (0x1 << 8);
    }

    /* Create and link the status phase QTd    */

    status = ehci_create_qtd (ehci, qhinfo, transfer, burst, NU_FALSE);

    /* Check if there are errors during creating QTDs */
    if (status != NU_SUCCESS)
        return status;

    /* Fill the qTd */
    ehci_fill_qtd (ehci, burst->hwqtd[burst->num_qtds - 1], NU_NULL, 0,
                   token);

    /* Set the remaining length to 0 */
    transfer->rem_length = 0;

    /* Link the bursts */

    /* setup->[data->]status */
    EHCI_WRITE_ADDRESS( ehci,
                        &transfer->burst[0].hwqtd[0]->next,
                        (UINT32) (transfer->burst[0].hwqtd[1]));


    /* If there is a data phase, the following statement would do the
     * needful. otherwise its overridden immediately after.
     */

    EHCI_WRITE_ADDRESS( ehci,
                &transfer->burst[0].hwqtd[1]->next,
                (UINT32) (transfer->burst[0].hwqtd[burst->num_qtds - 1]));


    EHCI_WRITE_ADDRESS( ehci,
                    &transfer->burst[0].hwqtd[burst->num_qtds - 1]->next,
                    0x1);


    EHCI_HW_READ32(ehci,
                    &transfer->burst[0].hwqtd[burst->num_qtds - 1]->token,
                    temp1);

    temp1 |= (1 << 15);

    EHCI_HW_WRITE32(ehci,
                    &transfer->burst[0].hwqtd[burst->num_qtds - 1]->token,
                    temp1);


    transfer->ref_count = 1;

    /* Start transfer */

    ehci_insert_qtd (ehci, qhinfo, transfer->burst[0].hwqtd[0]);

    return NU_SUCCESS;
}


/************************************************************************
*
* FUNCTION
*
*       ehci_qh_handle_data_irp
*
* DESCRIPTION
*
*       Handles translating the IRP in to Interrupt QTDs required to
*       initiate the transfer.
*
* INPUTS
*
*       ehci    handle that identifies the EHCI controller.
*       qhinfo  Identifies the QH over which the IRP is to be
*               transferred.
*       transfer Transfer associated with the IRP
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates that the transfer has been
*                               successfully setup.
*       NU_NO_MEMORY            Indicates that memory allocation failed.
*
*************************************************************************/

static STATUS ehci_qh_handle_data_irp (NU_USBH_EHCI * ehci,
                                       USBH_EHCI_QH_INFO * qhinfo,
                                       USBH_EHCI_IRP * transfer)
{
    STATUS status;

    USBH_EHCI_BURST *burst;

    burst = &transfer->burst[0];

    /* fill in the burst */

    status = ehci_prepare_burst (ehci, qhinfo, transfer, burst);

    if (status != NU_SUCCESS)
        return status;

    /* submit this burst */
    burst->state = USBH_EHCI_BURST_ACTIVE;

    ehci_insert_qtd (ehci, qhinfo, burst->hwqtd[0]);

    if (transfer->rem_length != 0)
    {
        burst = &transfer->burst[1];

        /* fill in the burst */

        status = ehci_prepare_burst (ehci, qhinfo, transfer, burst);

        NU_ASSERT (status == NU_SUCCESS);

    }

    return NU_SUCCESS;

}

/************************************************************************
*
* FUNCTION
*
*      ehci_prepare_burst
*
* DESCRIPTION
*
*      Prepares a burst for subsequent transfer on the USB wire.
*
* INPUTS
*
*      ehci     handle that identifies the EHCI controller.
*      qhinfo   Identifies the QH over which the IRP is to be
*               transferred.
*      transfer Transfer associated with the IRP
*      burst    Burst pointer
*
* OUTPUTS
*
*      NU_SUCCESS              Indicates that the burst has been
*                               successfully setup.
*      NU_NO_MEMORY            Indicates that memory allocation failed.
*
*
*
*************************************************************************/

STATUS ehci_prepare_burst (NU_USBH_EHCI * ehci,
                           USBH_EHCI_QH_INFO * qhinfo,
                           USBH_EHCI_IRP * transfer,
                           USBH_EHCI_BURST * burst)
{
    STATUS status;
    UINT32 token = 0;
    UINT32 reqd_tds;
    UINT32 length;
    UINT8 *data;
    UINT32 i;
    UINT32 temp;
    BOOLEAN use_empty_pkt;
    BOOLEAN sync = NU_TRUE;
    BOOLEAN buff_type;
    UINT32  temp1;

    /* Qtd status set active, allowable error count 3 */

    token = (0x1 << 7) | (0x03 << 10);  /* Status   */

    if (transfer->direction == USB_DIR_IN)
    {
        token |= (0x1 << 8);
        sync = NU_FALSE;
    }


    /* Calculate how many Tds are required for the burst */

    burst->data = transfer->rem_data;
    length = transfer->rem_length;

    if (length > USBH_EHCI_BURST_SIZE)
        length = USBH_EHCI_BURST_SIZE;

    burst->length = length;

    NU_USB_IRP_Get_Buffer_Type_Cachable(transfer->irp, &buff_type);
    if ( buff_type == NU_TRUE )
    {
        status = ehci_normalize_buffer (ehci, burst->data, length,
                             1, sync, &burst->norm_data, &burst->raw_data);
    }
    else
    {
        status = NU_SUCCESS;
        burst->norm_data = burst->data;
        burst->raw_data = burst->data;
    }

    if(status != NU_SUCCESS)
        return (status);

    data = burst->norm_data;
    length = burst->length;

    reqd_tds = length / USBH_EHCI_QTD_DATA_SIZE;

    if (length % USBH_EHCI_QTD_DATA_SIZE)
        reqd_tds++;

    /* Check we have required number of qTds already allocated.
     * if not allocate them
     */

    if (burst->num_qtds < reqd_tds)
    {
        for (i = burst->num_qtds; i < reqd_tds; i++)
        {
            status = ehci_create_qtd (ehci, qhinfo, transfer, burst, NU_FALSE);

            if (status != NU_SUCCESS)
                return status;
        }
    }
    else if (burst->num_qtds > reqd_tds)
    {
        for (i = reqd_tds; i < (USBH_EHCI_MAX_QTDS_PER_BURST + 1); i++)
        {
            status = ehci_dealloc_qtd (ehci, burst->hwqtd[i]);

            if (status != NU_SUCCESS)
                return (status);
        }
    }

    burst->num_qtds = reqd_tds;

    /* Fill in the first qTD */
    i = 0;

    temp = USBH_EHCI_QTD_DATA_SIZE;

    if (length < temp)
        temp = length;

    ehci_fill_qtd (ehci, burst->hwqtd[0], data, temp, token);

    length -= temp;

    data += temp;

    /* Setup QTDs for transfer */

    while (length != 0)
    {
        i++;

        if (length < temp)
            temp = length;

        /* Fill the qTd with relevant data */
        ehci_fill_qtd (ehci, burst->hwqtd[i], data, temp, token);

        /* link with prev qtd */
        EHCI_WRITE_ADDRESS(ehci, &burst->hwqtd[i - 1]->next,
                            (UINT32) (burst->hwqtd[i]));


        length -= temp;

        data += temp;

    }

    NU_ASSERT ((i + 1) == reqd_tds);

    /* Burst now ready */

    burst->state = USBH_EHCI_BURST_READY;

    /* update the transfer status. length & data */

    transfer->rem_length -= burst->length;
    transfer->rem_data += burst->length;

    /* If we have exhausted the length of data to be transferred then
     * check for empty packet */

    if (transfer->rem_length == 0)
    {
        /* If an empty packet needs to be sent, setup another qtd */
        UINT32 irp_length;

        NU_USB_IRP_Get_Use_Empty_Pkt (transfer->irp, &use_empty_pkt);
        NU_USB_IRP_Get_Length (transfer->irp, &irp_length);

        if ((use_empty_pkt)
            && ((irp_length % (qhinfo->maxpacket & 0x3FF)) == 0))
        {
            status = ehci_create_qtd (ehci, qhinfo, transfer, burst,
                                      NU_TRUE);

            /* Check if there are errors during creating QTD */
            if (status != NU_SUCCESS)
                return status;

            /* Fill the qTd with relevant data */
            (VOID) ehci_fill_qtd (ehci, burst->hwqtd[burst->num_qtds - 1],
                                  (UINT8 *) data, 0, token);

            /* link with prev qtd */
            EHCI_WRITE_ADDRESS(ehci, &burst->hwqtd[i]->next,
                            (UINT32) (burst->hwqtd[burst->num_qtds - 1]));
            i++;
        }

    }

    /* Last qTD has ioc set to 1 and next qtd not existing */

    EHCI_HW_READ32(ehci, &burst->hwqtd[i]->token, temp1);
    temp1 |= (1 << 15);
    EHCI_HW_WRITE32(ehci, &burst->hwqtd[i]->token, temp1);

    EHCI_HW_WRITE32(ehci, &burst->hwqtd[i]->next, 0x01);

    return NU_SUCCESS;
}

/* ==================================================================== */

/*                  ROOT HUB                                            */

/* ==================================================================== */

static UINT8 ehci_rh_dev_desc[] = {
    18,                                     /* bLength          */
    1,                                      /* DEVICE           */
    0x00,                                   /* USB 2.0          */
    0x02,                                   /* USB 2.0          */
    0x09,                                   /* HUB CLASS        */
    0x00,                                   /* Subclass         */
    0x00,                                   /* Protocol         */
    0x08,                                   /* bMaxPktSize0     */
    0x00,                                   /* idVendor         */
    0x0,                                    /* idVendor         */
    0x0,                                    /* idProduct        */
    0x0,                                    /* idProduct        */
    0x0,                                    /* bcdDevice        */
    0x0,                                    /* bcdDevice        */
    0x0,                                    /* iManufacturer    */
    0x0,                                    /* iProduct         */
    0x0,                                    /* iSerial Number   */
    1                                       /* One configuration */
};


static UINT8 ehci_rh_cfg_desc[] = {

    /* Configuration Descriptor */

    9,                                      /* bLength              */
    2,                                      /* CONFIGURATION        */
    25,                                     /* length               */
    00,                                     /* length               */
    1,                                      /* bNumInterfaces       */
    0x01,                                   /* bConfigurationValue  */
    0x00,                                   /* iConfiguration       */
    0xc0,                                   /* bmAttributes         */
    0,                                      /* power                */

    /* Interface Descriptor */

    9,                                      /* bLength              */
    4,                                      /* INTERFACE            */
    0,                                      /* bInterfaceNumber     */
    0,                                      /* bAlternateSetting    */
    1,                                      /* bNumEndpoints        */
    0x09,                                   /* bInterfaceClass      */
    0x00,                                   /* bInterfaceSubClass   */
    0x00,                                   /* bInterfaceProtocol   */
    0x00,                                   /* iInterface           */

    /* Endpoint Descriptor  */

    7,                                      /* bLength          */
    5,                                      /* ENDPOINT         */
    0x81,                                   /* bEndpointAddress */
    0x03,                                   /* bmAttributes     */
    0x10,                                   /* wMaxPacketSize   */
    0x00,                                   /* wMaxPacketSize   */
    0xf                                     /* bInterval        */
};

/* EHCI roothub handling */

/************************************************************************
*
* FUNCTION
*
*       ehci_rh_fill_rh_descriptor
*
* DESCRIPTION
*
*       This function reads the capabilities of the EHCI and fills in a
*       Hub class specific descriptor for roothub.
*
* INPUTS
*
*       ehci  :  Pointer to the EHCI structure containing roothub data
*       rh_desc_out   :  Byte array to fill in the Hub descriptor
*
* OUTPUTS
*
*       Always NU_SUCCESS
*
*************************************************************************/

/* Hub Descriptor : Section 11:23.2 of the Specification */

static UNSIGNED ehci_rh_fill_rh_descriptor (NU_USBH_EHCI * ehci,
                                            UINT8 *rh_desc_out)
{
    UINT32 temp = 0;
    UINT32 temp1 = 0;

    /* hub descriptor for a roothub follows the standard format, with the
     * following fields fixed.
     *
     *  wHubCharacteristics[2] = 0 ; Its not a compound device.
     *  wHubCharacteristics[TTThinkTime] is given an arbitrary value.
     *  wHubCharacteristics[bPwrOn2PwrGood] is 0 ms as this is a soft  hub
     *  Most of the devices on the ports are removable.
     *
     *  Some of these assumptions may be overridden by the programmer.
     **/

    rh_desc_out[0] = 11;        /* bDescLength */
    rh_desc_out[1] = 0x29;      /* bDescriptorType */

    /* For EHCI, the Number of Ports are available in the HCSPARAMS
     * register, bits 0-3
     * */

    EHCI_HW_READ32 (ehci, ehci->cb.controller.base_address + (HCSPARAMS), temp);

    rh_desc_out[2] = temp & 0x0F;

    /* Some portions of  the wHubCharacteristics is available in the
     * HCSPARAMS register */

    temp1 = 0;

    /* If this EHCI supports per port power switching */

    if (temp & 0x0010)
    {
        temp1 |= 0x01;

    }                           /* Otherwise, both d0 and d1 are zeros */

    /* Root hub is not part of any compound device so temp1[bit2] = 0 */

    /* Overcurrent protection mode is Global so temp1[bit2] = 0 */

    /* No FS/LS device directly connected to this hub. so TTThinktime is
     * not to be bothered */

    /* If this EHCI supports per port power indicators */

    if (temp & (0x1 << 16))
    {
        temp1 |= (0x01 << 7);

    }                           /* Otherwise, d7 is zero */

    rh_desc_out[3] = (temp1 >> 8);
    rh_desc_out[4] = (temp1 >> 16);

    /* Power-ON-2-Power-Good time is given as 2ms */

    rh_desc_out[5] = 1;

    /* Root Hub is powered by the System. Controller current requirements
     * are not to be bothered */

    rh_desc_out[6] = 0;

    /* All devices are removable to this Hub. We support 15 ports per RH.
     * so both the 'DeviceRemovable' bytes are zero */

    rh_desc_out[7] = 0;
    rh_desc_out[8] = 0;

    /* PortPwrCtrlMask is now defunct. We just initialize this to 0xFF as
     * required by the spec */

    rh_desc_out[9] = 0xFF;
    rh_desc_out[10] = 0xFF;

    /* Return the length of this descriptor */

    return (11);

}

/************************************************************************
*
* FUNCTION
*
*       ehci_rh_invalid_cmd
*
* DESCRIPTION
*
*       This function is used for all invalid requests as far as the root
*       hub is concerned.
*
* INPUTS
*
*       ehci  :  Pointer to the EHCI structure containing roothub data
*       irp   :  Pointer to the Data transfer
*
* OUTPUTS
*
*       Always NU_USB_NOT_SUPPORTED
*
*************************************************************************/

STATUS ehci_rh_invalid_cmd (NU_USBH_EHCI * ehci,
                            NU_USBH_CTRL_IRP * irp)
{
    return NU_USB_NOT_SUPPORTED;
}

/************************************************************************
*
* FUNCTION
*
*       ehci_rh_cl_get_status
*
* DESCRIPTION
*
*       This function handles the SET_FEATURE request for the roothub
*
* INPUTS
*
*
*       ehci   :  Pointer to the EHCI structure containing roothub data
*       irp    :  Pointer to the Data transfer
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED  if the specified feature doesn't exist or in
*                             the event of invalid request
*       NU_SUCCESS            On Success
*
*************************************************************************/

STATUS ehci_rh_cl_get_status (NU_USBH_EHCI * ehci,
                              NU_USBH_CTRL_IRP * irp)
{
    UINT8 bmRequestType;
    UINT16 wIndex;
    UINT8 port;
    UINT32 temp, temp1;
    UINT8 *irp_buffer;

    NU_USB_IRP_Get_Data ((NU_USB_IRP *) irp, &irp_buffer);
    NU_USBH_CTRL_IRP_Get_bmRequestType (irp, &bmRequestType);
    NU_USBH_CTRL_IRP_Get_wIndex (irp, &wIndex);

    wIndex = LE16_2_HOST(wIndex);
    /* Class specific GET STATUS.
     * At ehci rh, no hub level set features are supported.
     * only port level are supported. Hub status is always returned
     * success.
     */

    if (USBH_EHCI_RH_GET_RECIPIENT (bmRequestType) != USBH_EHCI_RH_RECIPIENT_PORT)
    {
        return NU_SUCCESS;
    }

    port = USBH_EHCI_RH_LSB (wIndex);
    if ( (port < 1) || (port > 15) )
    {
        return NU_SUCCESS;
    }

    /* Read the stored status for this port from the database */

    EHCI_HW_READ_REG (ehci,
                      EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, port), temp1);
    temp1 |= ehci->rh_status.previous.port_status[port - 1];

    temp = 0;

    /* If we don't own the port, then we don't report status either */

    if (temp1 & EHCI_PORT_OWNER)
    {
        return NU_SUCCESS;
    }

    /* We need to frame the status in a way defined by the Hub Class spec
     * */

    /* First, the change bits,
     *
     *  Connect change, enable change, suspend change,
     *  over current change and reset change.
     *
     *  These bits occupy the Upper 16-bit word of the status.
     */

    /* Reset Change */

    if (temp1 & (0x80000000))
    {
        temp |= ((0x1 << 4) << 16);
    }

    /* Suspend Change */

    if (temp1 & (0x1 << 30))
    {
        temp |= ((0x1 << 2) << 16);
    }

    /* Connect change */

    if (temp1 & EHCI_PORT_CONNECT_CHANGE)
    {
        temp |= (0x1 << 16);
    }

    /* Enable change */

    if (temp1 & EHCI_PORT_ENABLE_CHANGE)
    {
        temp |= ((0x1 << 1) << 16);
    }

    /* Overcurrent change */

    if (temp1 & EHCI_PORT_OVERCURRENT_CHANGE)
    {
        temp |= ((0x1 << 3) << 16);
    }


    /* Now the status bits */

    /* Connect, Enable, Suspend, OverCurrent, Reset, Power,
     * Device speed indication are supported. */

    /* Connect status */

    if (temp1 & EHCI_PORT_CONNECT)
    {
        temp |= (0x1);
    }

    /* Enable status */

    if (temp1 & EHCI_PORT_ENABLED)
    {
        temp |= (0x1 << 1);
    }

    /* Suspend status */

    if (temp1 & EHCI_PORT_SUSPEND)
    {
        temp |= (0x1 << 2);
    }

    /* Overcurrent status */

    if (temp1 & EHCI_PORT_OVERCURRENT)
    {
        temp |= (0x1 << 3);
    }

    /* Reset status */

    if (temp1 & EHCI_PORT_RESET)
    {
        temp |= (0x1 << 4);
    }

    /* Power status */

    if (temp1 & EHCI_PORT_POWER)
    {
        temp |= (0x1 << 8);
    }

    /* high speed device only */

    temp |= (0x1 << 10);

    /* Copy this data to the irp buffer */

    temp1 = HOST_2_LE32 (temp);

    memcpy (irp_buffer, (UINT8 *) &temp1, 4);

    NU_USB_IRP_Set_Actual_Length ((NU_USB_IRP *) irp, 4);

    return NU_SUCCESS;
}

/************************************************************************
*
* FUNCTION
*
*       ehci_rh_nothing_to_do_cmd
*
* DESCRIPTION
*
*       This function is used for all valid requests for which root hub
*       doesn't have any thing to be done. These are not expected from
*       the USBD. But if they manage to creep-in, we return success.
*
* INPUTS
*
*       ehci  :  Pointer to the EHCI structure containing roothub data
*       irp   :  Pointer to the Data transfer
*
* OUTPUTS
*
*       Always NU_SUCCESS
*
*************************************************************************/


STATUS ehci_rh_nothing_to_do_cmd (NU_USBH_EHCI * ehci,
                                  NU_USBH_CTRL_IRP * irp)
{
    return NU_SUCCESS;
}


/************************************************************************
*
* FUNCTION
*
*       ehci_rh_cl_set_feature
*
* DESCRIPTION
*
*       This function handles the SET_FEATURE request for the roothub
*
* INPUTS
*
*
*       ehci   :  Pointer to the EHCI structure containing roothub data
*       irp    :  Pointer to the Data transfer
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED  if the specified feature doesn't exist or in
*                             the event of invalid request
*       NU_SUCCESS            On Success
*
*************************************************************************/

STATUS ehci_rh_cl_set_feature (NU_USBH_EHCI * ehci,
                               NU_USBH_CTRL_IRP * irp)
{
    UINT8 bmRequestType;
    UINT16 wIndex;
    UINT16 wValue;

    UINT8 port, feature;
    UINT32 temp;

    NU_USBH_CTRL_IRP_Get_bmRequestType (irp, &bmRequestType);
    NU_USBH_CTRL_IRP_Get_wValue (irp, &wValue);
    NU_USBH_CTRL_IRP_Get_wIndex (irp, &wIndex);
    wValue = LE16_2_HOST(wValue);
    wIndex = LE16_2_HOST(wIndex);

    /* Class specific SET FEATURE.
     * At ehci rh, no hub level set features are supported.
     * only port level are supported.
     * */

    if (USBH_EHCI_RH_GET_RECIPIENT (bmRequestType) != USBH_EHCI_RH_RECIPIENT_PORT)
    {
        return NU_USB_NOT_SUPPORTED;
    }

    port = USBH_EHCI_RH_LSB (wIndex);
    if ( (port < 1) || (port > 15) )
    {
        return ( NU_USB_INVLD_ARG);
    }

    feature = USBH_EHCI_RH_LSB (wValue);

    /* Read the port status.
     * discard if not owned by us
     * */

    EHCI_HW_READ_REG (ehci,
                      EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, port), temp);

    if (temp & EHCI_PORT_OWNER)
    {
        return NU_SUCCESS;
    }

    /*
     * We support only PORT_RESET, PORT_SUSPEND and PORT_POWER features.
     * We always use Amber color for indicator.
     *
     * */

    /*
     * Support for power feature is straight forward.
     * we set the corresponding bit in the PORT status register and forget
     * it.
     */

    if (feature == PORT_POWER)
    {
        temp |= EHCI_PORT_POWER;
        EHCI_HW_WRITE_REG (ehci,
                           EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, port),
                           temp);

        return NU_SUCCESS;
    }

    /*
     *  WE USE THE RESERVED BITS OF THE STATUS WORD TO STORE THE RESET
     *  COMPLETION AND THE SUSPEND COMPLETION STATUS.
     **/


    /* Support for the suspend is slightly involved. We need to simulate
     * the suspend change event in the port status. The suspend change
     * needs to be communicated to the Hub class driver on a
     * Get_port_status command. However the EHCI doesn't have any such
     * notification.
     */

    if (feature == PORT_SUSPEND)
    {
        temp |= EHCI_PORT_SUSPEND;
        EHCI_HW_WRITE_REG (ehci,
                           EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, port),
                           temp);

        /* wait for arbitrary time */

        usb_wait_ms (2);        /* 2ms */

        /* indicate suspend completion next time when asked for */
        ehci->rh_status.previous.port_status[port - 1] |= (0x1 << 30);

        return NU_SUCCESS;
    }


    /* Support for RESET is the most complicated. As with suspend, we need
     * to communicate RESET_COMPLETE to the Hub class driver. In addition,
     *
     *  a)  If the device is a low speed device as per b11:10, then the
     *      port must be relinquished to companion controller.
     *
     *  b)  If the device is a full/low speed, the HC will change the
     *      ownership automatically at the end of reset.
     *
     *  c)  The end of reset is to be driven by the software.(Take that!).
     *
     *  At the end of reset if the port is not enabled, then the device is
     *  a low or full speed device.
     */

    if (feature == PORT_RESET)
    {
        /* If the Line status indicates a Low speed device, hand over */

        if ((temp & EHCI_PORT_LINESTATUS) == 0x0400)
        {
            temp |= EHCI_PORT_OWNER;
            EHCI_HW_WRITE_REG (ehci,
                               EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, port),
                               temp);

            /* disconnection is reported subsequently */

            /* reset the port status change */
            ehci->rh_status.previous.port_status[port - 1] = 0x0;
            return NU_SUCCESS;
        }

        /* Apply Reset for 200ms. Only 100ms as per spec, but we give
         * some more time
         * */

        temp |= EHCI_PORT_RESET;
        EHCI_HW_WRITE_REG (ehci,
                           EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, port),
                           temp);

        /* wait for 200ms */

        usb_wait_ms (200);      /* 200 ms */

        temp &= ~EHCI_PORT_RESET;
        EHCI_HW_WRITE_REG (ehci,
                           EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, port),
                           temp);

        /* wait for 200ms */

        usb_wait_ms (200);      /* 200 ms */

        /* Read the port status. If the port is enabled, then the device is
         * high speed. Otherwise, disconnect will be reported anyway
         */

        EHCI_HW_READ_REG (ehci,
                          EHCI_GET_PORT_REG (ehci->ehci_oprreg_base,
                                             port), temp);

        if (temp & EHCI_PORT_ENABLED)
        {
            /* indicate reset completion next time when asked for */
            ehci->rh_status.previous.port_status[port - 1] |= (0x80000000);
        }
        else
        {
            /* Manually relinquish the port */
            temp |= EHCI_PORT_OWNER;
            EHCI_HW_WRITE_REG (ehci,
                               EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, port),
                               temp);
            /* reset the port status change */
            ehci->rh_status.previous.port_status[port - 1] = 0x0;
        }

        return NU_SUCCESS;

    }

    /* Unsupported feature request */

    return NU_USB_NOT_SUPPORTED;
}


/************************************************************************
*
* FUNCTION
*
*       ehci_rh_cl_clear_feature
*
* DESCRIPTION
*
*       This function handles the CLEAR_FEATURE request for the roothub
*
* INPUTS
*
*
*       ehci   :  Pointer to the EHCI structure containing roothub data
*       irp    :  Pointer to the Data transfer
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED  if the specified feature doesn't exist or in
*                             the event of invalid request
*       NU_SUCCESS            On Success
*
*************************************************************************/

STATUS ehci_rh_cl_clear_feature (NU_USBH_EHCI * ehci,
                                 NU_USBH_CTRL_IRP * irp)
{
    UINT8 bmRequestType;
    UINT16 wIndex;
    UINT16 wValue;

    UINT8 port, feature;
    UINT32 temp;

    NU_USBH_CTRL_IRP_Get_bmRequestType (irp, &bmRequestType);
    NU_USBH_CTRL_IRP_Get_wValue (irp, &wValue);
    NU_USBH_CTRL_IRP_Get_wIndex (irp, &wIndex);

    wValue = LE16_2_HOST(wValue);
    wIndex = LE16_2_HOST(wIndex);
    /* Class specific CLEAR FEATURE.
     * At ehci rh, no hub level features are supported.
     * only port level features are supported.
     * */

    if (USBH_EHCI_RH_GET_RECIPIENT (bmRequestType) != USBH_EHCI_RH_RECIPIENT_PORT)
    {
        return NU_USB_NOT_SUPPORTED;
    }

    port = USBH_EHCI_RH_LSB (wIndex);
    if ( (port < 1) || (port > 15) )
    {
        return ( NU_USB_INVLD_ARG);
    }
    feature = USBH_EHCI_RH_LSB (wValue);

    /* Read the port status.
     * discard if not owned by us
     * */

    EHCI_HW_READ_REG (ehci,
                      EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, port),
                      temp);

    if (temp & EHCI_PORT_OWNER)
    {
        return NU_SUCCESS;
    }

    /* Check the feature and clear it */

    switch (feature)
    {
        case PORT_POWER:

            /* Clear port power. */
            temp &= ~(EHCI_PORT_POWER);
            break;

        case PORT_SUSPEND:

            /* Set force resume. */
            temp |= (EHCI_PORT_FORCE_RESUME);
            EHCI_HW_WRITE_REG (ehci,
                               EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, port), temp);

            /* Wait for 2 mseconds. */
            usb_wait_ms (2);

            /* Clear force resume. */
            temp &= ~(EHCI_PORT_FORCE_RESUME);

            /* Report this as a suspend change */
            ehci->rh_status.previous.port_status[port - 1] |= (0x1 << 30);
            break;

        case PORT_ENABLE:

            /* Disable port */
            temp &= ~(EHCI_PORT_ENABLED);
            break;

        case C_PORT_CONNECTION:

            /* Clear port connection change */

            temp |= (EHCI_PORT_CONNECT_CHANGE);
            break;

        case C_PORT_ENABLE:

            /* Clear port enable change */

            temp |= (EHCI_PORT_ENABLE_CHANGE);
            break;

        case C_PORT_OVER_CURRENT:

            /* Clear port enable change */

            temp |= (EHCI_PORT_OVERCURRENT_CHANGE);
            break;


        case C_PORT_SUSPEND:

            /* This is a software maintained change */
            ehci->rh_status.previous.port_status[port - 1] &= ~(0x1 << 30);

            return NU_SUCCESS;

        case C_PORT_RESET:

            /* This is a software maintained change */
            ehci->rh_status.previous.port_status[port - 1] &= ~(0x80000000);
            return NU_SUCCESS;

        default:

            return NU_USB_NOT_SUPPORTED;

    }

    /* Update the change in the hardware */

    EHCI_HW_WRITE_REG (ehci,
                  EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, port), temp);

    return NU_SUCCESS;

}


/************************************************************************
*
* FUNCTION
*
*       ehci_rh_get_descriptor
*
* DESCRIPTION
*
*       This function handles the GET_DESCRIPTOR for the roothub
*
* INPUTS
*
*       ehci  :  Pointer to the EHCI structure containing roothub data
*       irp   :  Pointer to the Data transfer
*
* OUTPUTS
*
*       NU_SUCCESS    Always
*
*************************************************************************/


STATUS ehci_rh_get_descriptor (NU_USBH_EHCI * ehci,
                               NU_USBH_CTRL_IRP * irp)
{
    UINT16 wLength;
    UINT8 *irp_buffer;
    UINT8 bmRequestType;
    UINT16 wValue;

    /* Depending on the descriptor required, the data is copied to the user
     * buffer. Length is set properly and the control is returned to the
     * caller.
     */

    NU_USBH_CTRL_IRP_Get_wLength (irp, &wLength);
    wLength = LE16_2_HOST(wLength);
    NU_USB_IRP_Get_Data ((NU_USB_IRP *) irp, &irp_buffer);

    NU_USBH_CTRL_IRP_Get_bmRequestType (irp, &bmRequestType);
    NU_USBH_CTRL_IRP_Get_wValue (irp, &wValue);
    wValue = LE16_2_HOST(wValue);

    /* If this is a standard get descriptor request, */

    if ((bmRequestType & 0x20) == 0)
    {
        /* Device Descriptor  */
        if (USBH_EHCI_RH_MSB (wValue) == 0x1)
        {
            if (wLength > ehci_rh_dev_desc[0])
                wLength = ehci_rh_dev_desc[0];

            memcpy (irp_buffer, ehci_rh_dev_desc, wLength);

        }
        else
        {
            /* Configuration Descriptor */

            if (wLength >= ehci_rh_cfg_desc[2])
                wLength = ehci_rh_cfg_desc[2];

            memcpy (irp_buffer, ehci_rh_cfg_desc, wLength);

        }

        NU_USB_IRP_Set_Actual_Length ((NU_USB_IRP *) irp, wLength);

        return NU_SUCCESS;
    }

    /* GET_USB_HUB_DESCRIPTOR command    */

    if (wLength >= ehci->rh_hub_desc[0])
        wLength = ehci->rh_hub_desc[0];

    memcpy (irp_buffer, ehci->rh_hub_desc, wLength);

    NU_USB_IRP_Set_Actual_Length ((NU_USB_IRP *) irp, wLength);

    return NU_SUCCESS;
}

/************************************************************************
* FUNCTION
*
*       ehci_rh_init
*
* DESCRIPTION
*
*       This function initializes the root-hub for the EHCI controller
*
* INPUTS
*
*       ehci    :   EHCI CB containing the RH Data
*       rh      :   Roothub descriptor
*
* OUTPUTS
*
*      NU_SUCCESS        Always
*
*************************************************************************/


STATUS ehci_rh_init (NU_USBH_EHCI * ehci)
{
    UINT32 i;

    /* First, get the roothub descriptor */

    (VOID) ehci_rh_fill_rh_descriptor (ehci, ehci->rh_hub_desc);

    /* Initialize the roothub status    */

    ehci->rh_status.previous.hub_status = 0x0;

    /* clear the status and initialize each port */

    for (i = 1; i <= ehci->rh_hub_desc[2]; i++)
    {
        /* port status is reset */
        ehci->rh_status.previous.port_status[i - 1] = 0;

        /* For the port,
         * 1. The Power is enabled.            b12 = 1
         * 2. Port is given an Amber Indicator b15:14 = 01b
         * 3. Port owner is set to EHCI.       b13=0
         * 4. Port is not suspended            b7=0
         */

        EHCI_HW_WRITE_REG (ehci,
                           EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, i),
                           ((0x1 << 12) | (0x1 << 14)));
    }

    /* status bitmap is reset */

    ehci->rh_status.status_map = 0x0;

    return NU_SUCCESS;
}


/**********************************************************************
*
* FUNCTION
*
*       ehci_rh_isr
*
* DESCRIPTION
*
*       This function processes the Port status change interrupt for the
*       EHCI  controller.
*
*       A Status change event occurred in one of the ports owned by the EHCI
*       controller has changed.The relevant port status register contains
*       the specific information about the change.
*
*       Process algorithm, for each port, is as follows:
*
*       1. Read the status from each port. Discard the port if this is not
*       owned by the EHCI (ie, companion controller)
*       2. Compare the status to previous recorded status. if same, advance
*       to next port
*       3. If there is a change, this is noted.
*       4. If an IRP is pending, then, the data is converted to what the
*       hub class spec defines and the IRP complete callback is invoked.
*
* INPUTS
*
*       ehci    :   EHCI CB containing the RH Data
*
* OUTPUTS
*
*       None
*
*************************************************************************/

VOID ehci_rh_isr (NU_USBH_EHCI * ehci)
{

    UINT32 temp;
    UINT32 report = 0;
    INT i = 0;

    NU_USB_IRP_CALLBACK callback;
    NU_USB_PIPE *pipe;

    UINT16 *buf;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
    USBH_EHCI_SESSION_HANDLE *session_handle;
#endif /* #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)*/

    /* A Status change event occurred in one of the ports owned by the EHCI
     * controller has changed.The relevant port status register contains
     * the specific information about the change.
     **/

    /* For each available port, do the following */

    for (i = 1; i <= ehci->rh_hub_desc[2]; i++)
    {
        /* Read the port status, */

        EHCI_HW_READ_REG (ehci,
                          EHCI_GET_PORT_REG (ehci->ehci_oprreg_base, i), temp);

        /* Do we EHCI guys own this port at all ? */

        if ((temp & EHCI_PORT_OWNER) == 0)
        {
            /* Its owned by the EHCI. Its a high speed port */

            /* Is this the port that has some status change event ?
             * Find out by comparing with previously recorded status
             * */

            if (temp != ehci->rh_status.previous.port_status[i - 1])
            {
                /* Yes, this is one of those ports. Note down the status
                 * and the port.
                 * */

                ehci->rh_status.previous.port_status[i - 1] = temp;
                report |= (0x1 << i);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
                session_handle = (USBH_EHCI_SESSION_HANDLE *)ehci->ses_handle;

                if ( session_handle != NU_NULL )
                {
                    /* Reset the watchdog */
                    (VOID)NU_PM_Reset_Watchdog(session_handle->ehci_inst_handle->pmi_dev);
                }
#endif /* #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)*/

            }
        }
    }

    if (report == 0)
        return;

    /* Now that we have found the ports that have changed, check to see if
     * there is a pending IRP. If so, update the data buffer and invoke the
     * callback.
     *
     * If no IRP is pending, then make a note that there has been an update
     * in status change and return.
     * */

    if (ehci->rh_status.irp != NU_NULL)
    {
        /* There is an IRP pending. Update the data */

        NU_USB_IRP_Get_Data (ehci->rh_status.irp, (UINT8 **) &buf);

        *buf = HOST_2_LE16 (report);

        NU_USB_IRP_Set_Actual_Length (ehci->rh_status.irp, 2);

        ehci->rh_status.status_map = 0;

        /* IRP is now complete. Update the status and invoke the callback
         * */

        NU_USB_IRP_Set_Status ((NU_USB_IRP *) (ehci->rh_status.irp),
                               NU_SUCCESS);

        NU_USB_IRP_Get_Callback ((NU_USB_IRP *) (ehci->rh_status.irp),
                                 &callback);

        NU_USB_IRP_Get_Pipe ((NU_USB_IRP *) (ehci->rh_status.irp), &pipe);

        callback (pipe, (NU_USB_IRP *) ehci->rh_status.irp);

        ehci->rh_status.irp = NU_NULL;

    }
    else
    {
        /* This will be sent to the Hub Class driver when it polls next */

        ehci->rh_status.status_map = report;
    }

    return;

}

/*
 * Dispatch tables for processing class specific requests and the standard
 * requests
 * */

STATUS (*ehci_rh_class_request[]) (NU_USBH_EHCI * ehci,
                                   NU_USBH_CTRL_IRP * irp) =
{
    ehci_rh_cl_get_status,      /* GET_STATUS      */
        ehci_rh_cl_clear_feature,   /* CLEAR_FEATURE   */
        ehci_rh_invalid_cmd,    /* Invalid command */
        ehci_rh_cl_set_feature, /* SET_FEATURE     */
        ehci_rh_invalid_cmd,    /* UN_SPECIFIED    */
        ehci_rh_invalid_cmd,    /* UN_SPECIFIED    */
        ehci_rh_get_descriptor, /* GET_DESCRIPTOR  */
        ehci_rh_invalid_cmd,    /* SET_DESCRIPTOR  */
        ehci_rh_invalid_cmd,    /* Clear_TT_Buffer */
        ehci_rh_invalid_cmd,    /* Reset_TT        */
        ehci_rh_invalid_cmd,    /* Get_TT_State    */
        ehci_rh_invalid_cmd     /* Stop_TT         */
};

/* Check if this table can be optimized */

STATUS (*ehci_rh_std_request[]) (NU_USBH_EHCI * ehci,
                                 NU_USBH_CTRL_IRP * irp) =
{
    ehci_rh_nothing_to_do_cmd,  /* GET_STATUS      */
        ehci_rh_nothing_to_do_cmd,  /* CLEAR_FEATURE   */
        ehci_rh_invalid_cmd,    /* Invalid command */
        ehci_rh_nothing_to_do_cmd,  /* SET_FEATURE     */
        ehci_rh_invalid_cmd,    /* UN_SPECIFIED    */
        ehci_rh_nothing_to_do_cmd,  /* SET_ADDRESS     */
        ehci_rh_get_descriptor, /* GET_DESCRIPTOR  */
        ehci_rh_invalid_cmd,    /* SET_DESCRIPTOR  */
        ehci_rh_nothing_to_do_cmd,  /* GET_CONFIGURATION */
        ehci_rh_nothing_to_do_cmd,  /* SET_CONFIGURATION */
        ehci_rh_nothing_to_do_cmd,  /* GET_INTERFACE    */
        ehci_rh_invalid_cmd,    /* SET_INTERFACE    */
        ehci_rh_invalid_cmd     /* SYNC_FRAME       */
};


/***********************************************************************
*
* FUNCTION
*
*       ehci_rh_handle_irp
*
* DESCRIPTION
*
*       This function handles all the control and interrupt transfers
*       that are directed towards the EHCI roothub.
*
* INPUTS
*
*       ehci    :   EHCI CB containing the RH Data
*       irp     :   Submitted data transfer
*       bEndpointAddress
*               :   Endpoint number on which the data transfer is
*                   requested.
*
* OUTPUTS
*
*       NU_SUCCESS always.
*
*************************************************************************/

STATUS ehci_rh_handle_irp (NU_USBH_EHCI * ehci,
                           NU_USB_IRP * irp,
                           UINT8 bEndpointAddress)
{
    UINT16 *buffer;
    UINT32 irp_length;
    STATUS irp_status;
    UINT8 bRequest;
    UINT8 bmRequestType;

    NU_USBH_CTRL_IRP *ctrl_irp;
    NU_USB_IRP_CALLBACK callback;
    NU_USB_PIPE *pipe;

    STATUS (**rh_request) (NU_USBH_EHCI * ehci,
                           NU_USBH_CTRL_IRP * irp);


    NU_USB_IRP_Get_Data (irp, (UINT8 **) &buffer);
    NU_USB_IRP_Get_Length (irp, &irp_length);


    /* Any hub supports 2 kinds of transfers : Control and Interrupt. */

    /* Check if the transfer is an interrupt transfer. Control transfers
     * are directed to endpoint zero, so....
     * */

    if ((bEndpointAddress & 0xf) != 0)
    {
        /* Interrupt transfer. Store the transfer request */

        ehci->rh_status.irp = irp;

        /* Check if an updated status is available  */

        if (ehci->rh_status.status_map != 0)
        {
            /* There status pending. Update the data */

            *buffer = HOST_2_LE16 (ehci->rh_status.status_map);

            NU_USB_IRP_Set_Actual_Length (ehci->rh_status.irp, 2);

            ehci->rh_status.status_map = 0;

            /* IRP is now complete. Update the status and invoke the
             * callback
             */

            NU_USB_IRP_Set_Status ((NU_USB_IRP *) (ehci->rh_status.irp),
                                   NU_SUCCESS);

            NU_USB_IRP_Get_Callback ((NU_USB_IRP *) (ehci->rh_status.irp),
                                     &callback);

            NU_USB_IRP_Get_Pipe ((NU_USB_IRP *) (ehci->rh_status.irp), &pipe);

            callback (pipe, (NU_USB_IRP *) ehci->rh_status.irp);;
        }

        return (NU_SUCCESS);
    }

    /* Control Transfers  */

    ctrl_irp = (NU_USBH_CTRL_IRP *) irp;
    memset (buffer, 0, irp_length);

    NU_USB_IRP_Set_Actual_Length (irp, 0);

    NU_USBH_CTRL_IRP_Get_bRequest (ctrl_irp, &bRequest);
    NU_USBH_CTRL_IRP_Get_bmRequestType (ctrl_irp, &bmRequestType);

    /* We have a separate dispatch table for class specific requests and
     * standard requests
     * */

    if (bmRequestType & 0x20)
    {
        /* for class specific requests, */

        if (bRequest >= (sizeof (ehci_rh_class_request) / 4))
        {
            NU_USB_IRP_Set_Status (irp, NU_USB_INVLD_ARG);
            return (NU_USB_INVLD_ARG);
        }

        rh_request = ehci_rh_class_request;
    }
    else
    {
        /* for standard requests, */

        if (bRequest >= (sizeof (ehci_rh_std_request) / 4))
        {
            NU_USB_IRP_Set_Status (irp, NU_USB_INVLD_ARG);
            return (NU_USB_INVLD_ARG);
        }

        rh_request = ehci_rh_std_request;
    }

    /* Invoke the function appropriate for the request */
    irp_status = rh_request[bRequest] (ehci, ctrl_irp);

    /* Handling complete. set the transfer status */

    NU_USB_IRP_Set_Status (irp, irp_status);

    /* Invoke the completion callback */

    NU_USB_IRP_Get_Callback (irp, &callback);
    NU_USB_IRP_Get_Pipe (irp, &pipe);
    callback (pipe, irp);

    return (NU_SUCCESS);
}

/************************************************************************
* FUNCTION
*
*      ehci_scan_async_list
*
* DESCRIPTION
*
*      This function scans the async list for any retired qtds and
*      re-submits another bursts / retires transfers
*
* INPUTS
*
*      ehci        EHCI Control block
*
* OUTPUTS
*
*      None
*
*************************************************************************/

VOID ehci_scan_async_list (NU_USBH_EHCI * ehci)
{
    USBH_EHCI_QH *qh, *first;
    UINT32 temp1;

    first = ehci->dummy_qh;

    EHCI_READ_ADDRESS(ehci, &first->next, temp1);

    qh = (USBH_EHCI_QH *) (temp1 & ~0x1F);

    /* For each of the open pipes, */

    while (qh != first)
    {
        /* check and retire any transfers */

        ehci_check_retire_transfer (ehci, qh->info);

        /* Go to the next open pipe */

        EHCI_READ_ADDRESS(ehci, &qh->next, temp1);

        qh = (USBH_EHCI_QH *) (temp1 & ~0x1F);

    }

    return;
}

/************************************************************************
* FUNCTION
*
*      ehci_find_best_periodic_schedule
*
* DESCRIPTION
*
*       Given a QH and its aperiodicity requirements in terms of number of
*       frames, this function returns the frame index which is optimal
*       place for the QH in the periodic schedule.
*
* INPUTS
*
*      ehci        EHCI Control block
*      qh_info     QH information for the endpoint.
*      period      Periodically required for the QH, in terms of frames.
*
* OUTPUTS
*
*      frame_index                 where the QH can be placed.
*      USBH_EHCI_FRAME_LIST_SIZE   indicates that the QH could not be
*                                  placed anywhere in the list.
*
*************************************************************************/


UINT8 ehci_find_best_periodic_shcedule (NU_USBH_EHCI * ehci,
                                        USBH_EHCI_QH_INFO * qh_info,
                                        UINT32 period)
{

    UINT32 prev_load = USB2_BANDWIDTH + 1;
    UINT32 frame_index = USBH_EHCI_FRAME_LIST_SIZE;
    UINT32 i, j;


    /* For each frame in the aperiodicity    */

    for (i = 0; i < period; i++)
    {
        /* If the current frame is better than the previous frame
         * in terms of load, then this frame is a likely candidate for
         * inserting this QH */

        if (ehci->frame_load[i] < prev_load)
        {
            /* Check if this frame and other frames that this QH may
             * become part, don't exceed the USB allowed bandwidth when
             * this QH is added.
             */

            for (j = i; j < USBH_EHCI_FRAME_LIST_SIZE; j += period)
            {
                if ((ehci->frame_load[j] + qh_info->load) > USB2_BANDWIDTH)
                {
                    prev_load = USB2_BANDWIDTH;
                    break;
                }
            }

            /* If things are thus far okay, then this frame is a likely
             * candidate for inserting our QH. This becomes our
             * benchmark frame for comparing against rest from now on.
             * */
            if (prev_load != USB2_BANDWIDTH)
            {
                prev_load = ehci->frame_load[i];
                frame_index = i;
            }

        }

    }

    return frame_index;
}

/************************************************************************
* FUNCTION
*
*       ehci_find_uframe_index
*
* DESCRIPTION
*
*       Finds a micro frame index for a given periodic transaction
*
* INPUTS
*
*       ehci        EHCI Control block
*       qh_info     QH information for the endpoint.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates that the pipe close is
*                               successfully initiated
*       NU_UNAVAILABLE          Indicates that qh could not be placed into
*                               the periodic schedule.
*
*************************************************************************/

/*
 * For split transaction S-Split cannot be in uframe index 5,6 or 7.
 * C-Split is in x+1,x+2,x+3 uframes, if S-Split is in x uframe.
 */

UINT8 ehci_find_uframe_index (NU_USBH_EHCI * ehci,
                              USBH_EHCI_QH_INFO * qh_info)
{

    UINT8 i;

    /* For each uframe   of the frame */

    for (i = 1; i < 8; i++)
    {

        if ((qh_info->speed != USB_SPEED_HIGH) && (i > 4))
            return 8;           /* No uframe index found */
        if (ehci->uframe_load[qh_info->index_per_tbl][i] +
            qh_info->load < USB2_BANDWIDTH / 8)
        {
            if (((qh_info->bEndpointAddress & USB_DIR_IN) == USB_DIR_IN)
                &&(qh_info->speed != USB_SPEED_HIGH))
            {
                /* If In Split-transaction, load is with any of the 3
                 * c-splits.
                 */
                if ((i != 0) && (i < 6))
                {
                    if ((ehci->uframe_load[qh_info->index_per_tbl][i + 1] +
                            qh_info->load < USB2_BANDWIDTH / 8)
                    && (ehci->uframe_load[qh_info->index_per_tbl][i + 2] +
                            qh_info->load < USB2_BANDWIDTH / 8))
                    return (i - 1);
                    /* S-Split should be in i-1, C-Split is in i,
                     * i+1, i+2
                     */
                }
            }
            else
            {
                return i;
            }
        }
    }
    return i;        /* No uframe index found with enough b/w */

}

/************************************************************************
* FUNCTION
*
*       ehci_schedule_intr_pipe
*
* DESCRIPTION
*
*       Places a given interrupt QH into the Periodic Schedule as per the
*       interval of the corresponding endpoint.
*
* INPUTS
*
*       ehci        EHCI Control block
*       qh_info     QH information for the endpoint.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates that the pipe close is
*                               successfully initiated
*       NU_UNAVAILABLE          Indicates that qh could not be placed into
*                               the periodic schedule.
*
*************************************************************************/

STATUS ehci_schedule_intr_pipe (NU_USBH_EHCI * ehci,
                                USBH_EHCI_QH_INFO * qh_info)
{
    UINT32 temp1, temp2, temp3;
    UINT32 frame_index;
    UINT8 mult;
    UINT32 i;

    /* The endpoint interval is in qh_info->interval. This is in
     * microframes. This now needs to be divided into frames and sub-frames.
     * */

    /* Periodically in frames. */

    temp1 = (qh_info->interval) >> 3;

    /* .... and in micro frames */

    temp2 = qh_info->interval - (temp1 << 3);

    /* transactions per micro frame, as read from wMaxPacketSize */

    mult = ((qh_info->maxpacket >> 11) & 0x03) + 1;


    if (temp1 == 0)
    {
        /* This endpoint is to be scheduled every frame, temp2 being the
         * aperiodicity */

        temp1 = 1;

    }
    else
    {
        /*
         * The other point is that the frame processing is a binary tree.
         * which means that periods that are multiple of two are only
         * possible. In the ehci case, periods that are 2, 4, 8, 16 until
         * 1024 are possible. so temp1 needs to be normalized in this case.
         */

        for (i = 2; i <= USBH_EHCI_FRAME_LIST_SIZE; i <<= 1)
        {
            if (i > temp1)
                break;
        }

        temp1 = (i >> 1);
        temp2 = 2;
    }

    /* This endpoint is to be scheduled every temp1 frame.
     * As an approximation, for simplicity, we are ignoring temp2, that
     * gives out the exact frame in which this Pipe may have to be
     * scheduled. We consider the temp2 then, and schedule accordingly.
     */

    frame_index = ehci_find_best_periodic_shcedule (ehci, qh_info, temp1);

    if (frame_index == USBH_EHCI_FRAME_LIST_SIZE)
        return NU_NO_MEMORY;

    qh_info->index_per_tbl = frame_index;

    qh_info->uframe = ehci_find_uframe_index (ehci, qh_info);
    if (qh_info->uframe >= 8)
        return NU_NO_MEMORY;

    qh_info->interval = temp1;

    /* Now that we know what frame number we are ready, update the SMask as
     * required.
     */

    /* we need to find out, where in the micro frame list that this
     * should be inserted. for example, if aperiodicity is 2, then the
     * QHs may be tried in 1-3-5-7 frames or 2-4-6-8 micro frames.
     * we need to find out if this is 1 or 2 to start with this is
     * based on the load in the corresponding microframe.
     */

    /* s-mask */
    temp2 = (1 << qh_info->uframe);


    /* c-mask */
    if (qh_info->speed != USB_SPEED_HIGH)
    {
        temp2 |= (1 << (qh_info->uframe + 9));
        temp2 |= (1 << (qh_info->uframe + 10));
        temp2 |= (1 << (qh_info->uframe + 11));
    }

    /* Update the Mult field    */

    temp2 |= (mult << 30);

    EHCI_HW_READ32(ehci, &qh_info->hwqh->ep_controlinfo2, temp3);
    temp3 |= temp2;
    EHCI_HW_WRITE32(ehci, &qh_info->hwqh->ep_controlinfo2, temp3);

    ehci_schedule_periodic_qh (ehci, qh_info, temp1);

    qh_info->qh_state = USBH_EHCI_QH_READY;

    return NU_SUCCESS;

}

/************************************************************************
* FUNCTION
*
*       ehci_schedule_iso_pipe
*
* DESCRIPTION
*
*       Places a given isochronous TD or split isochronous TD into the
*       periodic Schedule as per the interval of the corresponding endpoint.
*
* INPUTS
*
*       ehci        EHCI Control block
*       qh_info     QH information for the endpoint.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates that the pipe close is
*                               successfully initiated
*       NU_UNAVAILABLE          Indicates that ITD/Sitd could not be placed
*                               into the periodic schedule.
*
*************************************************************************/
STATUS ehci_schedule_iso_pipe (NU_USBH_EHCI * ehci,
                               USBH_EHCI_QH_INFO * qh_info)
{
    UINT32 temp1;
    UINT32 frame_index;
    UINT32 i;

    /* Maximum packet size of this endpoint */
    UINT16 maxp = qh_info->maxpacket;

    /* Limit for searching minimum loaded micro frame. */
    UINT32 uf_search_limit;

    /* Micro frame number of the minimum loaded micro frame. */
    UINT32 min_load_uf = 0;

    /* Number of complete splits to be sent. */
    UINT8 cs_count ;

    /* The endpoint interval is in qh_info->interval. This is in
     * microframes. This now needs to be divided into frames and sub-frames.
     * */

    /* Periodically in frames. */

    temp1 = (qh_info->interval) >> 3;


    if (temp1 == 0)

    {
        /* This endpoint is to be scheduled every frame, temp2 being the
         * aperiodicity */
        temp1 = 1;

    }
    else
    {
        /*
         * The other point is that the frame processing is a binary tree.
         * which means that periods that are multiple of two are only
         * possible. In the ehci case, periods that are 2, 4, 8, 16 until
         * 1024 are possible. so temp1 needs to be normalized in this case.
         */

        for (i = 2; i <= USBH_EHCI_FRAME_LIST_SIZE; i <<= 1)
        {
            if (i > temp1)
                break;
        }

        temp1 = (i >> 1);


    }


    /* This endpoint is to be scheduled every temp1 frame. */

    frame_index = ehci_find_best_periodic_shcedule (ehci, qh_info, temp1);

    if (frame_index == USBH_EHCI_FRAME_LIST_SIZE)
        return NU_NO_MEMORY;


    qh_info->index_per_tbl = frame_index;


    qh_info->uframe = ehci_find_uframe_index (ehci, qh_info);

    if (qh_info->uframe >= 8)
        return NU_NO_MEMORY;

    /* Since qh_info->uframe value is offset by 1 , we correct it here
     * to be in the range 0 - 7.
     */
    qh_info->next_uframe = qh_info->uframe - 1;

    /* Update load in this frame */
    ehci->frame_load[qh_info->index_per_tbl] += qh_info->load;

    if(qh_info->speed != USB_SPEED_HIGH)
    {
        qh_info->interval = temp1;

        /* Set the value of Start split mask */
        if((qh_info->bEndpointAddress & USB_DIR_IN) != USB_DIR_IN)
        {
            /* For OUT packets CS count represents number of
             * SS packets.
             */
            if( maxp < USBH_EHCI_ISO_OUT_SPLIT_MAXP )
            {
                cs_count = 1;
            }
            else
            {
                cs_count = maxp / USBH_EHCI_ISO_OUT_SPLIT_MAXP;
            }

            /* Look out for remaining data. */
            if( maxp % USBH_EHCI_ISO_OUT_SPLIT_MAXP )
            {
                /* Another CS is required for remaining data.
                 */
                cs_count++;
            }

            /* We can schedule OUT transfers up to micro-frame
             * 7. Depending upon the number of packets we need
             * to send.
             */


            uf_search_limit = 8 - cs_count;
            i = 0;


        }
        else
        {
            /* Determine how many CS packets are required for
             * this endpoint.
             */
            if( maxp < USBH_EHCI_ISO_IN_SPLIT_MAXP )
            {
                cs_count = 1;
            }
            else
            {
                cs_count = maxp / USBH_EHCI_ISO_IN_SPLIT_MAXP;

                /* Look out for remaining data. */
                if( maxp % USBH_EHCI_ISO_IN_SPLIT_MAXP )
                {
                    /* Another CS is required for remaining
                     * data.
                     */
                    cs_count++;
                }
            }

            /* An extra CS in case hub sends a NYET on the
             * first packet. maximum CS that we can send is
             * 6.
             */
            if ( cs_count < 6 )
            {
                cs_count++;
            }

            /* EHCI restrict ISO IN Start-Split to
             * micro-frame 0 to 3.
             * Find the micro-frame for complete-split within
             * micro-frame 2 to 7 depending upon the CS count.
             */
            uf_search_limit = 8 - cs_count;

            /* Start searching from micro-frame 2. */
            i = 2;

            /* We cant start complete split form micro-frame
             * 0.
             */
            min_load_uf = 2;

        }

        /* Find minimum load micro-frame. */
        for( ; i < uf_search_limit; i++ )
        {
            if( ehci->uframe_load[qh_info->index_per_tbl][ i ] <
                          ehci->uframe_load[qh_info->index_per_tbl][ min_load_uf ])
            {
                min_load_uf = i;
            }
        }

        /* Increase the load in the micro-frames. */
        for( i = 0; i < cs_count; i++ )
        {
            /* Check for not to exceed the maximum array size. */
            if((min_load_uf + i) < 8)
            {
                ehci->uframe_load[qh_info->index_per_tbl][ min_load_uf + i ] +=
                                                                 qh_info->load;
            }
        }

        /* For IN transfers complete-split is scheduled as data
          * is in it. Start-split is adjusted accordingly.
          */
        if((qh_info->bEndpointAddress ) & USB_DIR_IN )
        {
            /* Schedule complete-split, as IN data will be in
             * it.
             */
            for( i = min_load_uf;
                 i < ( min_load_uf + cs_count );
                 i++ )
            {
                /* sITD uframe complete split mask is
                 * after start split mask so we add 8 here
                 * which is size of split mask. */
                qh_info->mask |=  0x01 << i ;
            }

            qh_info->mask = qh_info->mask << 8;

            /* Start-split will be two frames before
             * complete-split.
             */
            qh_info->mask |= ( 0x01 << ( min_load_uf - 2 ));
         }
         else                     /* USB_DIR_OUT */
         {
            /* Start-split will be placed at minimum load
             * location.
             */
            for( i = min_load_uf;
                 i < ( min_load_uf + cs_count );
                 i++ )
            {

               qh_info->mask |= ( 0x1 << (i+1) );
            }

            /* Set T-Count */
            qh_info->ssplit_info = cs_count;

            /* Set TP field */
            if((cs_count > 1) && (cs_count < 3))
               qh_info->ssplit_info |= 1 << 3;
         }
    }
    else
    {
        /* Choose micro-frame depending upon interval. e.g if
         * interval is 2, we can send data in micro-frame 0,2,
         * 4,6 or 1,3,5,7. We'll start from the micro-frame
         * that has minimum load.
         */

         /* Now schedule packet(s) starting with minimum load
          * micro-frame.
          */
          for( i =  qh_info->next_uframe; i < 8; i += qh_info->interval)
          {
              ehci->uframe_load[qh_info->index_per_tbl][ i ] += qh_info->load;
              qh_info->mask |= ( 0x1 << i );
          }

    }

    qh_info->qh_state = USBH_EHCI_QH_READY;

    return NU_SUCCESS;

}
/************************************************************************
* FUNCTION
*
*      ehci_scan_periodic_list
*
* DESCRIPTION
*
*      This function scans the periodic list for any retired qtds and
*      re-submits another bursts / retires transfers
*
* INPUTS
*
*      ehci        EHCI Control block
*
* OUTPUTS
*
*      None
*
*************************************************************************/

VOID ehci_scan_periodic_list (NU_USBH_EHCI * ehci)
{
    UINT32 i,n;
    USBH_EHCI_QH_INFO *qh_info;


    /* One of the active QTds with IOC 1 may be retired. we need to check
     * for completion.
     */

    /* For each of the active QHs */
    for (i = 0; i < ehci->num_active_qh_periodic;)
    {
        /* Get the QH info for this pipe */
        qh_info = ehci->active_qh_periodic[i];

        /* check and retire any completed transfers */
        if(qh_info->type == USB_EP_ISO)
        {
            ehci_scan_isochronous_tds(ehci);
            break;
        }
        else
        {
            ehci_check_retire_transfer (ehci, qh_info);
            /* Is the QH still active? */
            if (qh_info->qtd_head == NU_NULL)
            {
                /* QH is no longer active. Remove it from the active list */
                ehci->num_active_qh_periodic--;
                for (n = i; n < ehci->num_active_qh_periodic; n++)
                {
                    ehci->active_qh_periodic[n] =
                            ehci->active_qh_periodic[n + 1];
                }
                ehci->active_qh_periodic[ehci->num_active_qh_periodic] =
                NU_NULL;
            }
            else
                i++;
        }
    }
    /* processed the periodic list for any retired transfers */
}

/************************************************************************
* FUNCTION
*
*      ehci_schedule_periodic_qh
*
* DESCRIPTION
*
*      This function places a periodic QH in the periodic schedule
*
* INPUTS
*
*      ehci        EHCI Control block
*      qh_info     QH for the endpoint
*      interval    required periodicity for the endpoint
*
* OUTPUTS
*
*      NU_SUCCESS  QH placed on the list successfully
*
*************************************************************************/
STATUS ehci_schedule_periodic_qh (NU_USBH_EHCI * ehci,
                                  USBH_EHCI_QH_INFO * qh_info,
                                  UINT32 interval)
{
    USBH_EHCI_QH *qh = qh_info->hwqh;
    USBH_EHCI_QH *cur, *temp, **prev;
    INT i;
    UINT32 temp1,temp2;

    EHCI_HW_WRITE32(ehci, &qh->next, 0x01);

    qh_info->qtd_head = NU_NULL;

    for (i = qh_info->index_per_tbl; i < USBH_EHCI_FRAME_LIST_SIZE;
         i += interval)
    {
        prev = (USBH_EHCI_QH **) & ehci->periodic_list_base[i];
        cur = *prev;

        temp2 = (UINT32)&cur;

        EHCI_READ_ADDRESS(ehci, temp2, temp1);

        cur = (USBH_EHCI_QH *) (temp1 & ~0x1E);

        while (!((UINT32) (cur) & 0x01) && (cur != qh))
        {
            if (qh_info->interval > cur->info->interval)
            {
                break;
            }

            prev = (USBH_EHCI_QH **) & (cur->next);

            EHCI_READ_ADDRESS(ehci, &cur->next, temp1);
            cur = (USBH_EHCI_QH *) (temp1 & ~0x1E);
        }

        if (cur != qh)
        {
            temp = *prev;
            qh->next = (UINT32) temp;
            EHCI_WRITE_ADDRESS(ehci, prev, (((UINT32)qh) | 0x02));
        }

        ehci->frame_load[i] += qh_info->load;

        if ((qh_info->speed != USB_SPEED_HIGH) &&
            ((qh_info->bEndpointAddress & USB_DIR_IN) == USB_DIR_IN) &&
            (qh_info->uframe < 5))
        {
            /* The load is with c-spilt that may in any of the next 3 uframes. */
            ehci->uframe_load[i][qh_info->uframe + 1] += qh_info->load;
            ehci->uframe_load[i][qh_info->uframe + 2] += qh_info->load;
            ehci->uframe_load[i][qh_info->uframe + 3] += qh_info->load;

        }
        else
        {
            ehci->uframe_load[i][qh_info->uframe] += qh_info->load;
        }

    }

    return NU_SUCCESS;
}

/************************************************************************
* FUNCTION
*
*      ehci_schedule_iso_itd
*
* DESCRIPTION
*
*      This function places a high speed iso td in the periodic schedule
*
* INPUTS
*
*      ehci        EHCI Control block
*      qh_info     QH for the endpoint
*      pitd        Pointer to the ITD
*      frame       Location in the periodic list.
*
* OUTPUTS
*
*      NU_SUCCESS  ITD placed on the list successfully
*
*************************************************************************/
STATUS ehci_schedule_iso_itd (NU_USBH_EHCI * ehci,
                              USBH_EHCI_QH_INFO * qh_info,
                              USBH_EHCI_ITD *pitd,
                              UINT32 frame)
{
    USBH_EHCI_ITD *itd = pitd;
    USBH_EHCI_ITD **prev;


    prev = (USBH_EHCI_ITD**) &ehci->periodic_list_base[frame];
    itd->next = (UINT32)(*prev);
    itd->frame = frame;
    EHCI_WRITE_ADDRESS(ehci,prev,(UINT32)itd);

    return NU_SUCCESS;
}

/************************************************************************
* FUNCTION
*
*      ehci_schedule_iso_sitd
*
* DESCRIPTION
*
*      This function places a high speed iso sitd in the periodic schedule
*
* INPUTS
*
*      ehci        EHCI Control block
*      qh_info     QH for the endpoint
*      psitd       Pointer to the SITD
*      interval    required periodicity for the endpoint
*
* OUTPUTS
*
*      NU_SUCCESS  SITD placed on the list successfully
*
*************************************************************************/
STATUS ehci_schedule_iso_sitd (NU_USBH_EHCI * ehci,
                               USBH_EHCI_QH_INFO * qh_info,
                               USBH_EHCI_SITD *psitd,
                               UINT32 interval,
                               UINT8 direction)
{
    USBH_EHCI_SITD *sitd = psitd;
    USBH_EHCI_SITD **prev;
    UINT32 temp,now;

    USBH_EHCI_ISO_QH_INFO   *iso_qhinfo  = (USBH_EHCI_ISO_QH_INFO*)qh_info;


    /* If transactions for this queue head have started , calculate
     * frame number according to the interval of this endpoint.
     */
    if(iso_qhinfo->trans_start_flag)
    {
        iso_qhinfo->last_td_frame_num +=interval;
        iso_qhinfo->last_td_frame_num  %= USBH_EHCI_FRAME_LIST_SIZE;

    }
    else
    {
        now = 0;

        /* Read current value of frame index register */
        EHCI_HW_READ32 (ehci, ehci->ehci_oprreg_base + FRINDEX, temp);
        ehci->next_uframe = temp;
        temp = temp % (USBH_EHCI_FRAME_LIST_SIZE << 3);
        now = temp >> 3;

        iso_qhinfo->trans_start_flag = 1;
        iso_qhinfo->last_td_frame_num = now + 2;
        iso_qhinfo->last_td_frame_num =
            iso_qhinfo->last_td_frame_num % USBH_EHCI_FRAME_LIST_SIZE;
    }

    EHCI_HW_WRITE32(ehci, &sitd->next, 0x01);

    /* Link sitd in the periodic frame list.  */
    prev = (USBH_EHCI_SITD**) &ehci->periodic_list_base[iso_qhinfo->last_td_frame_num];

    sitd->backpointer = NU_NULL;
    sitd->next = (UINT32)(*prev);
    sitd->frame = iso_qhinfo->last_td_frame_num;

    /* Write address of the sitd in the periodic list and also set its
     * type.
     */
    EHCI_WRITE_ADDRESS(ehci,prev,(UINT32)sitd | 0x04);





    return NU_SUCCESS;
}


/************************************************************************
*
* FUNCTION
*
*      ehci_dealloc_qh
*
* DESCRIPTION
*
*      Deallocates an QH that was earlier allocated from the QH pool.
*      If the deallocation results in all the elements in the pool to be
*      available and if its not the first pool, then the pool itself is
*      released. It thus ensures that at least 1 pool is always available
*      even  if no QHs are allocated from it.
*
* INPUTS
*
*      ehci    handle that identifies the HC in the EHCI HC database.
*      qh_ptr  QH ptr that has to be freed.
*
* OUTPUTS
*
*      NU_SUCCESS      Indicates successful deallocation of the QH.
*      NU_NOT_PRESENT  Indicates that the ed_ptr is not found in
*                      the pool table.
*************************************************************************/

STATUS ehci_dealloc_qh (NU_USBH_EHCI * ehci,
                        USBH_EHCI_QH * qh_ptr)
{
    EHCI_ALIGNED_MEM_POOL   *pool;
    UINT32 first_td;
    UINT32 last_td;
    UINT8 pool_count;
    STATUS status = NU_INVALID_MEMORY;
    UINT8 id;

    USBH_EHCI_PROTECT;

    /* Search for a free QH in the allocated pools. */
    for (pool_count = 0; pool_count < USBH_EHCI_MAX_QH_POOLS; pool_count++)
    {
        /* Get the control block of the pool to which this QH belongs. */
        pool = &(ehci->qh_pool[pool_count]);

        /* Get the address of the first QH form this pool. */
        first_td = (UINT32) pool->start_ptr;

        /* Get the address of the last QH by adding total number of QHs in the start address. */
        last_td = first_td + (sizeof(USBH_EHCI_QH) * (USBH_EHCI_MAX_QHS_PER_POOL - 1));

        /* Check if the QH belongs to this pool. */
        if(((UINT32) qh_ptr >= first_td) && ((UINT32) qh_ptr <= last_td))
        {
            /* Calculate the ID of this QH. */
            id = (((UINT32)qh_ptr - first_td) / sizeof(USBH_EHCI_QH));

            /* Mark the QH as free. */
            usb_release_id(pool->bits, (USBH_EHCI_MAX_QHS_PER_POOL / 8), id);

            /* Increment number of available QHs in this pool. */
            pool->avail_elements++;

           /* Always have at least one pool , the first pool  to prevent
             * memory fragmentation due to frequent pool
             * allocation and deallocation.
             */
           if ((pool->avail_elements == USBH_EHCI_MAX_QHS_PER_POOL) &&
               (pool_count!= NU_NULL))
           {
                /* No TDs of this pool are in use, so release it. */
                NU_Deallocate_Memory(pool->start_ptr);

                memset(pool, 0, sizeof(EHCI_ALIGNED_MEM_POOL));
           }

           status = NU_SUCCESS;

           break;
        }
    }

    USBH_EHCI_UNPROTECT;

    return ( status );
}

/*************************************************************************
*
* FUNCTION
*
*      ehci_retire_qh
*
* DESCRIPTION
*
*      This function retires all transfer associated with the QH
*      with a given transfer status. If there are any pending IRPs
*      associated with the QH, they are also retired.
*
* INPUTS
*
*      ehci    handle that identifies the HC in the EHCI HC database.
*      qhinfo  QH ptr that has to be freed.
*      tx_status   Status of the transfer (can be forced)
*
* OUTPUTS
*
*      None
*
*************************************************************************/

/* Retire each of the transfers of the QH with the supplied status */

VOID ehci_retire_qh (NU_USBH_EHCI * ehci,
                     USBH_EHCI_QH_INFO * qhinfo,
                     STATUS tx_status)
{
    USBH_EHCI_QH *qh;
    USBH_EHCI_QTD *qtd;

    /* if the parameters are not valid, return */

    if ((qhinfo == NU_NULL) || (qhinfo->hwqh == NU_NULL))
        return;

    qh = qhinfo->hwqh;

    /* HW, don't process any more QTDs */

    EHCI_HW_WRITE32(ehci, &qh->next_qtd, 1);


    /* head of the current transfer */

    qtd = qhinfo->qtd_head;

    qhinfo->qtd_head = NU_NULL;

    /* While we have a valid qtd */

    if ((qtd != NU_NULL) && (((UINT32) qtd & 0x01) == 0))
    {
        ehci_retire_transfer (ehci, qtd->burst->transfer, qtd->burst,
                              tx_status);
    }

    /* retired all the active transfers */

    /* Remove all transfers from the pend_q and retire them */

    while (qhinfo->pend_irp != NU_NULL)
    {
        /* there are pending IRPs */
        NU_USB_IRP *next_irp;
        NU_USB_PIPE *pipe;
        NU_USB_IRP_CALLBACK callback;

        /* get hold of the IRP */
        next_irp = qhinfo->pend_irp;

        /* remove it from the list */
        NU_Remove_From_List ((CS_NODE **) & (qhinfo->pend_irp),
                              (CS_NODE *) next_irp);

        /* Update the IRP status */

        NU_USB_IRP_Set_Status (next_irp, NU_USB_IRP_CANCELLED);

        /* Invoke the transfer completion callback */

        NU_USB_IRP_Get_Callback (next_irp, &callback);
        NU_USB_IRP_Get_Pipe (next_irp, &pipe);

        callback (pipe, next_irp);

    }

    return;
}

/************************************************************************
*
* FUNCTION
*
*      ehci_retire_transfer
*
* DESCRIPTION
*
*      This function retires the current transfer associated with the QH
*      with a given transfer status.
*
* INPUTS
*
*      ehci        handle that identifies the HC in the EHCI HC database.
*      transfer    Current ongoing transfer
*      burst       Current Burst
*      tx_status   Status of the transfer (can be forced)
*
* OUTPUTS
*
*      None
*
*************************************************************************/
/* This function retires a Transfer */

VOID ehci_retire_transfer (NU_USBH_EHCI * ehci,
                           USBH_EHCI_IRP * transfer,
                           USBH_EHCI_BURST * burst,
                           STATUS tx_status)
{
    UINT32 i, j;
    STATUS status;
    UINT32 rem_len= 0;
    UINT32 total_len;
    UINT32 rem_irp_len = 0;

    BOOLEAN short_ok;

    NU_USB_IRP_CALLBACK callback;

    NU_USB_PIPE *pipe;
    NU_USB_IRP *irp;
    USBH_EHCI_QH_INFO *qhinfo = transfer->qh;
    BOOLEAN sync = NU_FALSE;
    UINT32  temp1;
    UINT32 temp_index = 0;

    if (transfer->qh->type != USB_EP_CTRL)
    {
        if (qhinfo->bEndpointAddress & 0x80)
                sync = NU_TRUE;

        /* Check if we need to continue the transfer */

        if (tx_status == NU_SUCCESS)
        {
            i = (burst->index + 1) % 2;

            burst->state = USBH_EHCI_BURST_IDLE;

            if (transfer->burst[i].state == USBH_EHCI_BURST_READY)
            {
                /* submit this pre-prepared burst */

                ehci_insert_qtd (ehci, transfer->qh,
                                 transfer->burst[i].hwqtd[0]);
                transfer->burst[i].state = USBH_EHCI_BURST_ACTIVE;
            }

            /* Calculate remaining length of this burst. */
            for (j = 0; j < burst->num_qtds; j++)
            {
                EHCI_HW_READ32(ehci, &burst->hwqtd[j]->token, temp1);
                rem_len += ((temp1 >> 16) & (0x7FFF));
            }

            /* de-normalize the burst */
            if ( (burst->raw_data   != NU_NULL) &&
                 (burst->data       != burst->raw_data) )
            {
                status = ehci_transfer_done (ehci, burst->data,
                                      burst->norm_data,  burst->raw_data, (burst->length - rem_len), sync);
            }

            /* check if some more are to be prepared */

            if (transfer->rem_length != 0)
            {
                status =
                    ehci_prepare_burst (ehci, transfer->qh, transfer, burst);

                if (status != NU_SUCCESS)
                {
                    tx_status = status;
                }
            }

            if ((burst->state != USBH_EHCI_BURST_IDLE) ||
                (transfer->burst[i].state != USBH_EHCI_BURST_IDLE))
            {
                return;
            }
        }
        else
        {
            /* Calculate remaining length of this burst. */
            for (j = 0; j < burst->num_qtds; j++)
            {
                EHCI_HW_READ32(ehci, &burst->hwqtd[j]->token, temp1);
                rem_len += ((temp1 >> 16) & (0x7FFF));
            }

            /* de-normalize the burst */
            if ( (burst->raw_data   != NU_NULL) &&
                 (burst->data       != burst->raw_data) )
            {
                status = ehci_transfer_done (ehci, burst->data,
                                      burst->norm_data,  burst->raw_data, (burst->length - rem_len), sync);
            }

            burst->state = USBH_EHCI_BURST_IDLE;
            rem_irp_len += rem_len;
            rem_len = 0;

            /* if we had another burst ready while this returned failure,
             * we de-normalize that too */

            i = (burst->index + 1) % 2;

            if (transfer->burst[i].state == USBH_EHCI_BURST_READY)
            {
                burst = &transfer->burst[1];
                for (j = 0; j < burst->num_qtds; j++)
                {
                    EHCI_HW_READ32(ehci, &burst->hwqtd[j]->token, temp1);

                    rem_len += ((temp1 >> 16) & (0x7FFF));
                }

                if ( (transfer->burst[i].raw_data   != NU_NULL) &&
                     (transfer->burst[i].data       != transfer->burst[i].raw_data) )
                {
                    status = ehci_transfer_done (ehci, transfer->burst[i].data,
                                      transfer->burst[i].norm_data,
                                      transfer->burst[i].raw_data,
                                      (transfer->burst[i].length - rem_len), sync);
                }

                transfer->burst[i].state = USBH_EHCI_BURST_IDLE;
                rem_irp_len += rem_len;
                rem_len = 0;
            }

        }
    }
    else
    {
        /* Control transfers need separate Tx_Dones */

        if(tx_status == NU_USB_DATA_UNDERRUN)
            tx_status = NU_SUCCESS;

        /* Setup packet */
        if( (burst->raw_data != NU_NULL) &&
            (burst->data     != burst->raw_data) )
        {
            status = ehci_transfer_done (ehci, burst->data,
                                  burst->norm_data, burst->raw_data, burst->length, NU_FALSE);
        }

        temp_index = burst->index;

        for (i = 0; i < 2; i++)
        {
            burst = &transfer->burst[i];

            if((i != temp_index) && (burst->state != USBH_EHCI_BURST_READY))
                continue;

            for (j = 1; j < burst->num_qtds; j++)
            {
                EHCI_HW_READ32(ehci, &burst->hwqtd[j]->token, temp1);

                rem_len += ((temp1 >> 16) & (0x7FFF));
            }

        }


        /* If there is a data phase then there will be a Tx_Done for it too
         * */

        if (transfer->burst[1].length != 0)
        {
            if (transfer->direction == USB_DIR_IN)
                sync = NU_TRUE;

            if( (transfer->burst[1].raw_data != NU_NULL) &&
                (transfer->burst[1].data     != transfer->burst[1].raw_data) )
            {
                status = ehci_transfer_done (ehci, transfer->burst[1].data,
                                      transfer->burst[1].norm_data,
                                      transfer->burst[1].raw_data,
                                      (transfer->burst[1].length - rem_len), sync);
            }

            rem_irp_len += rem_len;
            rem_len = 0;
        }
    }


    if((transfer->qh->type != USB_EP_CTRL) && (tx_status == NU_SUCCESS))
    {
        /* Calculate how much length was pending to be transferred */
        rem_irp_len = transfer->rem_length;
        temp_index = burst->index;

        for (i = 0; i < 2; i++)
        {
            burst = &transfer->burst[i];

            if((i != temp_index) && (burst->state != USBH_EHCI_BURST_READY))
                continue;

            for (j = 0; j < burst->num_qtds; j++)
            {
                EHCI_HW_READ32(ehci, &burst->hwqtd[j]->token, temp1);

                rem_irp_len += ((temp1 >> 16) & (0x7FFF));
            }

        }
    }

    irp = transfer->irp;

    /* Update the IRP with the length of the received qTd */

    NU_USB_IRP_Get_Length (transfer->irp, &total_len);

    NU_USB_IRP_Set_Actual_Length (transfer->irp, (total_len - rem_irp_len));

    /* Invoke the transfer completion callback */

    NU_USB_IRP_Get_Callback (transfer->irp, &callback);

    NU_USB_IRP_Get_Pipe (transfer->irp, &pipe);

    NU_USB_IRP_Get_Accept_Short_Packets(transfer->irp, &short_ok);

    if((short_ok == NU_TRUE) && (tx_status == NU_USB_DATA_UNDERRUN))
    {
        tx_status = NU_SUCCESS;
    }

    /* Update the IRP status */

    NU_USB_IRP_Set_Status (transfer->irp, tx_status);

    /* Deallocate the memory associated with this transfer */

    for (i = 0; i < 2; i++)
    {
        burst = &transfer->burst[i];

        for (j = 0; j < burst->num_qtds; j++)
        {
            status = ehci_dealloc_qtd (ehci, burst->hwqtd[j]);
        }

        burst->num_qtds = 0;
    }

    NU_Deallocate_Memory (transfer);


    /* Inform the IRP completion */
    if(callback)
        callback (pipe, irp);
}



/************************************************************************
*
* FUNCTION
*
*      ehci_check_retire_transfer
*
* DESCRIPTION
*
*      This function checks a QH for any retired transfer. Retires one
*      if it finds.
*
* INPUTS
*
*      ehci        handle that identifies the HC in the EHCI HC database.
*      qhinfo      QH to be checked for retired transfer
*
* OUTPUTS
*
*      None
*
*************************************************************************/

VOID ehci_check_retire_transfer (NU_USBH_EHCI * ehci,
                                 USBH_EHCI_QH_INFO * qhinfo)
{

    USBH_EHCI_QH *qh;
    USBH_EHCI_QTD *qtd;
    USBH_EHCI_IRP *transfer;
    USBH_EHCI_BURST *burst;
    UINT32 next, temp1;
    STATUS tx_status;
    NU_USB_IRP *next_irp;
    UINT32 l = 0;

    if ((qhinfo == NU_NULL) || (qhinfo->hwqh == NU_NULL))
    {
        return;
    }

    qh = (USBH_EHCI_QH *) ((UINT32) (qhinfo->hwqh) & ~0x1F);
    qtd = (USBH_EHCI_QTD *) ((UINT32) (qhinfo->qtd_head) & ~0x1F);
    if (qtd == NU_NULL)
    {
        return;
    }

    EHCI_HW_READ32(ehci, &qtd->token, temp1);
    if (temp1 & 0x80)
    {
#if(CFG_NU_OS_DRVR_USB_HOST_EHCI_ATMEL_TD_FIX == NU_TRUE)
        /* For Atmel targets clear the active TD bit forcefully. */
        temp1 &= ~(0x7FFF0080);
        EHCI_HW_WRITE32(ehci, &qtd->token, temp1);
#elif(CFG_NU_OS_DRVR_USB_HOST_EHCI_TI_TD_FIX == NU_TRUE)
        /* For TI targets wait till hardware clears the active TD bit. */
        while((EHCI_HW_READ32(ehci, &qtd->token, temp1)) & 0x80);
#else
        return;
#endif
    }

    /* In case of control endpoints we make sure that active
     * bit of last QTD is zero before we retire the transfer.
     * In case of halt condition or babble we retire the transfer.
     */

    if((qhinfo->type == USB_EP_CTRL) && (!(temp1 & 0x50)))
    {
        burst = qtd->burst;
        transfer = burst->transfer;

        if((burst->num_qtds > 1) && (burst->num_qtds < 3))
        {
            EHCI_HW_READ32(ehci,
                    &transfer->burst[0].hwqtd[burst->num_qtds - 1]->token,
                    temp1);

            while ((temp1 & 0x80))
            {
                EHCI_HW_READ32(ehci,
                    &transfer->burst[0].hwqtd[burst->num_qtds - 1]->token,
                    temp1);

                l++;

                if(l > USBH_EHCI_MAX_ITERATIONS)
                    break;
            }
            l = 0;
            qtd = transfer->burst[0].hwqtd[burst->num_qtds - 1];
        }
        else if((burst->num_qtds > 1) && (burst->num_qtds == 3))
        {
            EHCI_HW_READ32(ehci,
                    &transfer->burst[0].hwqtd[1]->token,
                    temp1);

            while ((temp1 & 0x80))
            {
                EHCI_HW_READ32(ehci,
                    &transfer->burst[0].hwqtd[1]->token,
                    temp1);
                l++;

                if(l > USBH_EHCI_MAX_ITERATIONS)
                    break;
            }
            l = 0;

            if(temp1 & 0x50)
            {
                qtd = transfer->burst[0].hwqtd[1];

            }

            if(!(temp1 & 0x50))
            {
                EHCI_HW_READ32(ehci,
                    &transfer->burst[0].hwqtd[burst->num_qtds - 1]->token,
                    temp1);

                while ((temp1 & 0x80))
                {
                    EHCI_HW_READ32(ehci,
                    &transfer->burst[0].hwqtd[burst->num_qtds - 1]->token,
                    temp1);
                    l++;

                    if(l > USBH_EHCI_MAX_ITERATIONS)
                        break;
                }

                qtd = transfer->burst[0].hwqtd[burst->num_qtds - 1];
            }

        }

    }

    /* Now that head qtd is retired, more qtds may have retired,
     * process all of the retired qTDs till we find,
     *
     * 1. An active QTd
     * 2. End of all QTds for this QH. This means that QH itself is no
     *    longer active.
     */

    /* While the current Qtd is inactive */

    while (!(temp1 & 0x80) && (qtd != NU_NULL))
    {
        burst = qtd->burst;
        transfer = burst->transfer;

        /* we need to store the next qtd pointer. the retiring td
         * handler may choose to free all QTds deciding that that is
         * the end of transfer.
         */

        EHCI_READ_ADDRESS(ehci, &qtd->next, next);

        /* Check if this Transfer is to be retired now.
         * If any of the following conditions are met, we retire the
         * transfer.
         * 1. Short packet received.
         * 2.  Babble or endpoint halted condition occurs.
         * 3. Data transfer is complete.
         */

        if ((temp1 & 0x7FFF0000) ||    /* Transfer incomplete  */
            (temp1 & 0x50) ||  /* Transfer error       */
            /* This is the last TD  */
            (qtd == burst->hwqtd[burst->num_qtds - 1]) ||
            (next & 0x01)
            )
        {
            BOOLEAN fix_short_pkt_bug = NU_FALSE;

            /* Check to see if any more qtds are linked ? */

            if( (qh->info->type == USB_EP_BULK) &&
                (qh->info->bEndpointAddress & 0x80))

            {
                fix_short_pkt_bug = NU_TRUE;
            }

            /* check the status of transfer.
             * if an error, convert that to Stack expected error.
             * if necessary, clear the error condition.
             */


            ehci_normalize_status (ehci, qtd, &tx_status);

            qh->info->qtd_head = NU_NULL;

            EHCI_HW_WRITE32(ehci, &qh->next_qtd, 0x01);

            if(fix_short_pkt_bug == NU_TRUE )
            {
                ehci_unlink_async_qh(ehci, qh->info);

                EHCI_HW_WRITE32(ehci, &qh->next_qtd, 0x01);

                EHCI_HW_WRITE32(ehci, &qh->current_qtd, 0x0);

                EHCI_HW_WRITE32(ehci, &qh->alt_next_qtd, 0x01);

                ehci_link_async_qh(ehci, qh->info);
            }
            /* Convert pending IRP to transfers. */
            if (qhinfo->pend_irp != NU_NULL)
            {
                /* get hold of the IRP */
                next_irp = qhinfo->pend_irp;

                /* remove it from the list */
                NU_Remove_From_List ((CS_NODE **) & (qhinfo->pend_irp),
                                      (CS_NODE *) next_irp);

                /* process the irp */
                ehci_qh_handle_irp (ehci, qhinfo, next_irp);

            }

            /* Retire the transfer */
            ehci_retire_transfer (ehci, transfer, burst, tx_status);

            return;
        }

        /* go to the next qtd. */
        qtd = (USBH_EHCI_QTD *) (next & ~0x1F);

        if(qtd != NU_NULL)
            EHCI_HW_READ32(ehci, &qtd->token, temp1);

        l = 0;

        while((temp1 & 0x80) && (qtd != NU_NULL) )
        {
            EHCI_HW_READ32(ehci,&qtd->token,temp1);
            l++;
            if(l > USBH_EHCI_MAX_ITERATIONS)
                break;
        }
    }

    return;
}

/************************************************************************
*
* FUNCTION
*
*      ehci_check_retire_fs_iso_transfer
*
* DESCRIPTION
*
*      This function checks SITDs of the submitted IRP for any retired
*      SITDs. Retires if it finds one.Also submits more SITDs for
*      any pending IRPs if free SITD is available.
*
* INPUTS
*
*      ehci        handle that identifies the HC in the EHCI HC database.
*      qhinfo      QH to be checked for retired transfer
*      irp_priv    Pointer to the ISO IRP structure
*      index       Index of the QHinfo in the periodic list.
*      flag        Indicates if the SITD is retired by HW or abnormally
*                  terminated by the software due to pipe close or flush.
*                  (USBH_EHCI_ITD_RETIRED or USBH_EHCI_ITD_CANCELLED)
*
* OUTPUTS
*
*      NU_SUCCESS                   sITD has been retired successfully.
*      NU_INVALID_POINTER           Invalid sITD pointer.
*      USBH_EHCI_SITD_NOT_RETIRED   sITD still active.
*
*************************************************************************/

STATUS ehci_retire_fs_iso_transfer (NU_USBH_EHCI * ehci,
                                    USBH_EHCI_QH_INFO* qhinfo,
                                    USBH_EHCI_ISO_IRP *irp_priv,
                                    USBH_EHCI_SITD *psitd,
                                    UINT32 flag
                                    )
{
    USBH_EHCI_SITD *sitd;
    NU_USB_ISO_IRP *irp = NU_NULL;
    UINT32 i,j;
    UINT8 direction = qhinfo->bEndpointAddress & 0x80;
    UINT32  temp;
    USBH_EHCI_ISO_QH_INFO *iso_qhinfo = (USBH_EHCI_ISO_QH_INFO*)qhinfo;
    UINT16 *lengths;
    UINT16 *actual_lengths;
    BOOLEAN submit_next_td;
    UINT8 **pbuffer_array;
    USBH_EHCI_ISO_IRP *current_irp_priv;
    USBH_EHCI_ISO_IRP *next_irp_priv;
    NU_USB_IRP *base_irp;
    NU_USB_IRP_CALLBACK callback;
    NU_USB_PIPE *pipe;
    USBH_EHCI_SITD **prev;
#if(USBH_EHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)
    BOOLEAN sync;
#endif
    STATUS tx_status = NU_SUCCESS;
    USBH_EHCI_SITD* sitd_array = (USBH_EHCI_SITD*)(iso_qhinfo->td_array_start);

    /* In case ITD is cancelled , retire all pending transfers. */
    if(flag == USBH_EHCI_ITD_CANCELLED)
    {
        j = 0;

        while(j < iso_qhinfo->next_td_index)
        {
            sitd = (USBH_EHCI_SITD*)(&sitd_array[j]);

            if(sitd->raw_data != NU_NULL)
            {
                prev = (USBH_EHCI_SITD**) &ehci->periodic_list_base[sitd->frame];
                *prev = (USBH_EHCI_SITD*)sitd->next;

                  #if(USBH_EHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)

                  if(direction & USB_DIR_OUT)
                  sync = NU_FALSE;
                  else
                  sync = NU_TRUE;

                  NU_USB_HWENV_Tx_Done(ehci->hwenv,
                              sitd->data,
                              sitd->non_cache_ptr,
                              sitd->raw_data,
                              sitd->length,
                              1,
                              sync);
                  #endif

                  sitd->non_cache_ptr = NU_NULL;
                  sitd->raw_data = NU_NULL;
            }
            j++;
          }

        for(i = iso_qhinfo->current_irp_index;i<iso_qhinfo->incoming_irp_index;i++)
        {
            irp_priv = &iso_qhinfo->irps[i];
            base_irp = (NU_USB_IRP*)irp_priv->irp;

            /* Call the callback function. */
            NU_USB_IRP_Get_Callback(base_irp, &callback);
            NU_USB_IRP_Get_Pipe(base_irp, &pipe);
            NU_USB_IRP_Set_Status (base_irp, NU_USB_IRP_CANCELLED);
            NU_USB_ISO_IRP_Set_Actual_Num_Transactions (irp_priv->irp, irp_priv->cnt);

            if (callback)
            {
                callback(pipe, base_irp);
            }
        }

        iso_qhinfo->current_irp_index = 0;
        iso_qhinfo->incoming_irp_index = 0;
        iso_qhinfo->next_irp_index = 0;
        iso_qhinfo->trans_start_flag = 0;
        iso_qhinfo->no_more_irp = 0;
        iso_qhinfo->last_irp = NU_FALSE;

        return NU_SUCCESS;
    }

    /* Pointer of the SITD */
    sitd = psitd;

    irp = irp_priv->irp;

    /* Check if the sitd is retired. */
    EHCI_HW_READ32(ehci, &sitd->results, temp);

    /* Update periodic list to remove pointer of the retired SITD. */
    prev = (USBH_EHCI_SITD**) &ehci->periodic_list_base[sitd->frame];
    *prev = (USBH_EHCI_SITD*)sitd->next;


    NU_USB_ISO_IRP_Get_Actual_Lengths(irp, &actual_lengths);
    NU_USB_ISO_IRP_Get_Lengths(irp, &lengths);


    /* If the current SITD is inactive */
    if (!(temp & EHCI_SITD_ACTIVE))
    {
        if(direction)
            actual_lengths[irp_priv->cnt] =  lengths[irp_priv->cnt] - EHCI_SITD_TOTAL_BYTES(temp);
        else
            /* OUT */
            actual_lengths[irp_priv->cnt] = lengths[irp_priv->cnt];

#if(USBH_EHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)

         if(direction & USB_DIR_OUT)
             sync = NU_FALSE;
         else
             sync = NU_TRUE;

/*         NU_USB_HWENV_Tx_Done(ehci->hwenv,
                              sitd->data,
                              sitd->non_cache_ptr,
                              sitd->raw_data,
                              actual_lengths[irp_priv->cnt],
                              1,
                              sync);
*/
#endif
            sitd->non_cache_ptr = NU_NULL;
            sitd->raw_data = NU_NULL;

            irp_priv->cnt++;

            iso_qhinfo->ret_td_index++;

            if(iso_qhinfo->ret_td_index == USBH_EHCI_MAX_ITDS_PER_IRP)
                iso_qhinfo->ret_td_index = 0;

            /* Increase the global free TD count. */
            iso_qhinfo->free_tds++;
        }

         /* Load IRP, transactions of which will be submitted. */
         next_irp_priv = &iso_qhinfo->irps[iso_qhinfo->next_irp_index];

         /* Another IRP has been submitted, from the time no_more_irp was
          * marked last time.
          */
         if ((iso_qhinfo->next_irp_index != iso_qhinfo->incoming_irp_index)
             && (iso_qhinfo->no_more_irp == 1))
         {
             iso_qhinfo->no_more_irp = 0;
         }

         /* submit_next_td = 1, force this function to occupy only one more
          * empty slot of TD if available. This keeps load on this function
          * called from ISR moderate.
          */
         submit_next_td = 1;

         while ((iso_qhinfo->free_tds != 0) &&
            (iso_qhinfo->no_more_irp == 0) && (submit_next_td != 0))
         {
             submit_next_td = 0;

             /* If all the TDs associated with this IRP has been submitted. */
             if (next_irp_priv->next_index == next_irp_priv->no_tds)
             {
                 /* Increment the index. */
                 iso_qhinfo->next_irp_index++;

                 if (iso_qhinfo->next_irp_index == USBH_EHCI_MAX_PEND_IRPS)
                 {
                     iso_qhinfo->next_irp_index = 0;
                 }

                 /* At this point, submit more IRPs, if space is available. */
                 submit_next_td = 1;

                  /* If next irp index reaches incoming irp index. */
                 if (iso_qhinfo->next_irp_index ==
                            iso_qhinfo->incoming_irp_index)
                 {
                     /* No more IRPs to submit. */
                     iso_qhinfo->last_irp = iso_qhinfo->next_irp_index - 1;
                     iso_qhinfo->no_more_irp = 1;

                     /* Break the submission loop. */
                     break;
                 }

                 /* Get td_priv at the next irp index. */
                 next_irp_priv =
                      &iso_qhinfo->irps[iso_qhinfo->next_irp_index];

             }


             /* If there are more TDs to submit. */
             if ((iso_qhinfo->no_more_irp == 0))
             {
                 irp = next_irp_priv->irp;

                 /* Get buffer length associated with this IRP. */
                 NU_USB_ISO_IRP_Get_Lengths(irp, &lengths);

                 /* Get buffer pointer associated with this IRP. */
                 NU_USB_ISO_IRP_Get_Buffer_Array(irp, &pbuffer_array);


                /* Get HwTD at the next free location. */
                sitd = (USBH_EHCI_SITD*)&sitd_array[iso_qhinfo->next_td_index];


                ehci_fill_sitd(ehci,sitd,
                               pbuffer_array[next_irp_priv->next_index],
                               lengths[next_irp_priv->next_index],
                               1,
                               direction,
                               qhinfo->mask,
                               qhinfo->ssplit_info,
                               qhinfo->address);

                ehci_schedule_iso_sitd(ehci,qhinfo,sitd,1,direction);

                /* next_td_index can't be more than
                 * USBH_EHCI_MAX_ITDS_PER_IRP.
                 */
                iso_qhinfo->next_td_index++;

                if (iso_qhinfo->next_td_index == USBH_EHCI_MAX_ITDS_PER_IRP)
                {
                    iso_qhinfo->next_td_index = 0;
                }

                /* TD is submitted, decrement the global free TD count and
                 * increment the TD count associated with this irp.
                 */
                iso_qhinfo->free_tds--;

                /* Increment the index associated with IRP the number of
                 * transactions submitted in this IRP.
                 */
                next_irp_priv->next_index += 1;

             }
         }

         /* Again load current IRP. */
         current_irp_priv =
               &iso_qhinfo->irps[iso_qhinfo->current_irp_index];


         /* Invoke Callback, if all the TDs of this IRP has been processed. */
         if (current_irp_priv->cnt == current_irp_priv->no_tds)
         {
             base_irp = (NU_USB_IRP*)current_irp_priv->irp;

             ehci_normalize_sitd_trans_status(ehci, temp, &tx_status,direction);

             /* Call the callback function. */
             NU_USB_IRP_Get_Callback(base_irp, &callback);
             NU_USB_IRP_Get_Pipe(base_irp, &pipe);
             NU_USB_IRP_Set_Status (base_irp, tx_status);
             NU_USB_ISO_IRP_Set_Actual_Num_Transactions (current_irp_priv->irp, current_irp_priv->cnt);

             if (callback)
             {
                 callback(pipe, base_irp);

             }

             /* Check for last IRP callback. */
             if ((iso_qhinfo->last_irp == iso_qhinfo->current_irp_index) &&
                  (iso_qhinfo->no_more_irp == 1))
             {
                 iso_qhinfo->no_more_irp = 0;
                 iso_qhinfo->last_irp = 0;
                 iso_qhinfo->current_irp_index = 0;
                 iso_qhinfo->next_irp_index = 0;
                 iso_qhinfo->incoming_irp_index = 0;

             }
             else
             {
                 /* Otherwise increment the current index. */
                 iso_qhinfo->current_irp_index++;
                 if (iso_qhinfo->current_irp_index == USBH_EHCI_MAX_PEND_IRPS)
                 {
                     iso_qhinfo->current_irp_index = 0;
                 }
             }
         }

    return NU_SUCCESS;
}
/************************************************************************
*
* FUNCTION
*
*      ehci_normalize_status
*
* DESCRIPTION
*
*      This function checks for the status of a retired qtd and converts
*      the status code to the one expected by the IRP callback routines.
*      If the error needs the qh to be reset, the error condition is
*      removed from the qh.
*
* INPUTS
*
*   ehci    handle that identifies the HC in the EHCI HC database.
*   qtd     qtd to retrieve status from
*   tx_status_out  Pointer where the normalized status is kept
*
*************************************************************************/

VOID ehci_normalize_status (NU_USBH_EHCI * ehci,
                            USBH_EHCI_QTD * qtd,
                            STATUS *tx_status_out)
{
    UINT32 token;
    UINT32 qtd_status;
    UINT32 rem_length;
    UINT8 speed;

    EHCI_HW_READ32(ehci, &qtd->token, token);
    qtd_status = token & 0x7F;
    rem_length = (token & 0x7FFF0000) >> 16;
    speed = qtd->burst->transfer->qh->speed;

    /* if qtd retired successfully, then return */

    if (qtd_status == 0x0)
    {
        if (rem_length == 0)
            *tx_status_out = NU_SUCCESS;
        else
            *tx_status_out = NU_USB_DATA_UNDERRUN;

        return;
    }

    /* if the qtd is halted */

    if (qtd_status & (0x1 << 6))
    {
        /* This can be because of babble / error counter reaching 0 or by a
         * STALL handshake.
         */

        /* If a babble error caused the halt */

        if (qtd_status & (0x1 << 4))
        {
            *tx_status_out = NU_USB_DATA_OVERRUN;
        }

        /* If error counter reaches 0 */

        else if (!(token & 0x0C00))
        {
            *tx_status_out = NU_USB_EP_HALTED;
        }

        else
        {
            *tx_status_out = NU_USB_STALL_ERR;
        }

        /* clear the halt condition from the qh */

        EHCI_HW_WRITE32(ehci,
                        &qtd->burst->transfer->qh->hwqh->transfer_result,
                        0x0);

    }

    /* If a buffer error occurs */

    else if (qtd_status & (0x1 << 5))
    {
        /* over run or under run */

        if(((token & 0x0300) >> 8) == 0x01)
        {
             if (EHCI_QTD_ERR_COUNT(token) != 0)
                 *tx_status_out = NU_SUCCESS;
             else
                *tx_status_out = NU_USB_BFR_UNDERRUN;
        }
        else
        {
             if (EHCI_QTD_ERR_COUNT(token) != 0)
                 *tx_status_out = NU_SUCCESS;
             else
                *tx_status_out = NU_USB_BFR_OVERRUN;
        }
    }

    /* If a transaction error occurs */

    else if (qtd_status & (0x1 << 3))
    {
         if (EHCI_QTD_ERR_COUNT(token) != 0)
             *tx_status_out = NU_SUCCESS;
         else
            *tx_status_out = NU_USB_UNKNOWN_ERR;
    }

    else if (qtd_status & (0x1 << 2))
    {
        if(speed != USB_SPEED_HIGH)
        {
            *tx_status_out = NU_SUCCESS;
        }
    }
    else if (qtd_status & 0x1)
    {
        /* if high speed, ignore it */

        if(speed == USB_SPEED_HIGH)
        {
            *tx_status_out = NU_SUCCESS;
        }
        else
        {
            *tx_status_out = NU_USB_INVLD_PID;
        }
    }
    else
    {
        *tx_status_out = NU_SUCCESS;
    }

    return;
}

/************************************************************************
*
* FUNCTION
*
*      ehci_normalize_itd_trans_status
*
* DESCRIPTION
*
*      This function checks for the status of a retired itd and converts
*      the status code to the one expected by the IRP callback routines.
*
* INPUTS
*
*   ehci           handle that identifies the HC in the EHCI HC database.
*   status         status of retired ITD
*   tx_status_out  Pointer where the normalized status is kept
*   direction      IN/OUT direction of transfer
*
*************************************************************************/
VOID ehci_normalize_itd_trans_status(NU_USBH_EHCI * ehci, UINT32 status,
                                     STATUS * tx_status_out, UINT8 direction)
{

    UINT32 itd_status;

    itd_status = status & 0xF0000000;

    /* if itd retired successfully, then return */
    if (itd_status == 0)
    {
        *tx_status_out = NU_SUCCESS;
        return;
    }

    /* If a babble error occurred */
    if (itd_status & EHCI_ITD_BABBLE)
    {
       *tx_status_out = NU_USB_DATA_OVERRUN;
    }
    /* If a buffer error occurs */
    else if (itd_status & EHCI_ITD_BUF_ERR)
    {
        /* over run or under run */

        if(direction == USB_DIR_IN)
        {
            *tx_status_out = NU_USB_BFR_UNDERRUN;
        }
        else
        {
            *tx_status_out = NU_USB_BFR_OVERRUN;
        }
    }

    /* If a transaction error occurs */

    else if (itd_status & EHCI_ITD_TRA_ERR)
    {
         *tx_status_out = NU_USB_UNKNOWN_ERR;
    }
    else
    {
        *tx_status_out = NU_SUCCESS;
    }
    return;
}
/************************************************************************
*
* FUNCTION
*
*      ehci_normalize_sitd_trans_status
*
* DESCRIPTION
*
*      This function checks for the status of a retired sitd and converts
*      the status code to the one expected by the IRP callback routines.
*
* INPUTS
*
*   ehci           handle that identifies the HC in the EHCI HC database.
*   status         status
*   tx_status_out  Pointer where the normalized status is kept
*   direction      Direction of transfer
*
*************************************************************************/
VOID ehci_normalize_sitd_trans_status (NU_USBH_EHCI * ehci,
                                       UINT32 status,
                                       STATUS *tx_status_out,
                                       UINT8 direction)
{
    UINT32 sitd_status;


    sitd_status = status & 0x000000FF;

    /* if sitd retired successfully, then return */
    if (sitd_status == 0)
    {
        *tx_status_out = NU_SUCCESS;
        return;
    }

    /* If a babble error occurred */
    if (sitd_status & EHCI_SITD_BABBLE)
    {
       *tx_status_out = NU_USB_DATA_OVERRUN;
    }
    /* If a buffer error occurs */
    else if (sitd_status & EHCI_SITD_BUFFER_ERR)
    {
        /* over run or under run */
        if(direction == USB_DIR_IN)
        {
            *tx_status_out = NU_USB_BFR_UNDERRUN;
        }
        else
        {
            *tx_status_out = NU_USB_BFR_OVERRUN;
        }
    }
    /* If a transaction error occurs */
    else if (sitd_status & EHCI_SITD_TX_ERR)
    {
         *tx_status_out = NU_USB_UNKNOWN_ERR;
    }
    else
    {
        *tx_status_out = NU_SUCCESS;
    }

    return;

}
/************************************************************************
*
* FUNCTION
*
*      ehci_handle_iso_irp
*
* DESCRIPTION
*
*     Handles translating the IRP in to ITD's or split ITD's required to
*     initiate the transfer
*
* INPUTS
*
*      ehci    handle that identifies the HC in the EHCI HC database.
*      qhinfo  Identifies the QH over which the data is to be transferred.
*      irp     Ptr to the IRP that has to be transferred.
*
*
* OUTPUTS
*
*      NU_SUCCESS If the transfer has been setup successfully
*      NU_NO_MEMORY if available memory has been exhausted
*
*************************************************************************/
STATUS ehci_handle_iso_irp(NU_USBH_EHCI * ehci,
                           USBH_EHCI_QH_INFO * qhinfo,
                           NU_USB_IRP *irp)
{

    STATUS status = NU_USB_INVLD_ARG;
    USBH_EHCI_ISO_IRP *irp_priv = NU_NULL;
    NU_USB_PIPE *pipe;
    NU_USB_DEVICE *device, *parent;
    UINT8 port_num, hub_addr, speed;
    UINT8 i;
    NU_USB_ISO_IRP *iso_irp = (NU_USB_ISO_IRP*)irp;
    UINT32 temp1 = 0;
    USBH_EHCI_ISO_QH_INFO   *iso_qhinfo;
    UINT16 no_transactions = 0;
    UINT8 **pdata_array;
    VOID *Td;
    USBH_EHCI_ITD *itd, *itd_array_start;
    UINT32 count = 0;
    UINT32 direction;
    UINT16 *lengths;
    UINT32 uframe = 0,frame;
    UINT8 trans_per_td = 0;
    UINT8 ioc = 0;
    USBH_EHCI_SITD  *sitd_array_start;
    UINT32 temp = 0;

    /* Endpoint direction */
    direction = qhinfo->bEndpointAddress & USB_DIR_IN;

    iso_qhinfo = (USBH_EHCI_ISO_QH_INFO *)qhinfo;

    /* Check maximum IRPs has been submitted. */
    if (((iso_qhinfo->incoming_irp_index + 1)
           == iso_qhinfo->current_irp_index) ||
        ((iso_qhinfo->incoming_irp_index == USBH_EHCI_MAX_PEND_IRPS) &&
         (iso_qhinfo->current_irp_index == 0)))
    {
        return USBH_EHCI_ISO_MAX_IRP;
    }

    /* If the queue head has not been scheduled yet, schedule it */
    if (qhinfo->qh_state == USBH_EHCI_QH_NOT_READY)
    {
        /* For FULL/LOW speed devices, get the nearest Hi-speed hub
         * and fill it in the sitd. This is for split transfers.
         */
        if (qhinfo->speed != USB_SPEED_HIGH)
        {
            NU_USB_IRP_Get_Pipe (irp, &pipe);
            NU_USB_PIPE_Get_Device (pipe, &device);

            do
            {
                status = NU_USB_DEVICE_Get_Parent (device, &parent);

                if (status != NU_SUCCESS)
                    return status;

                NU_USB_DEVICE_Get_Speed (parent, &speed);
                device = parent;
            }
            while (speed != USB_SPEED_HIGH);

            NU_USB_DEVICE_Get_Function_Addr (parent, &hub_addr);

            status = NU_USB_DEVICE_Get_Port_Number (device, &port_num);

            if (status != NU_SUCCESS)
                return status;

            temp1 |= (hub_addr << 16);
            temp1 |= (port_num << 24);

            qhinfo->address |= temp1;

            /* Check endpoint direction . */
            if(direction)
                qhinfo->address |= 0x80000000; /*1 << 31*/
        }

        /* find the location in the periodic schedule
         *  to insert the sitd or itd
         */
        status = ehci_schedule_iso_pipe (ehci, qhinfo);

        if (status != NU_SUCCESS)
           return status;
    }

    irp_priv = &iso_qhinfo->irps[iso_qhinfo->incoming_irp_index];

    memset(irp_priv,0,sizeof(USBH_EHCI_ISO_IRP));

    /* Number of Iso transactions in the IRP */
    status = NU_USB_ISO_IRP_Get_Num_Transactions (iso_irp ,
                                        &no_transactions);


    if(qhinfo->speed == USB_SPEED_HIGH)
    {
        if(qhinfo->interval < 8)
        {
            trans_per_td = 8/qhinfo->interval;

            irp_priv->no_tds = no_transactions/trans_per_td;

            if(no_transactions % trans_per_td)
                irp_priv->no_tds++;
        }
        else
            irp_priv->no_tds = no_transactions;
    }
    else
        irp_priv->no_tds = no_transactions;

    irp_priv->irp = iso_irp;



    /* If incoming_irp_index is equal to current_irp_index, this indicates
     * the 1st IRP has been submitted by application.
     */

    if (iso_qhinfo->incoming_irp_index == iso_qhinfo->current_irp_index)
    {
        iso_qhinfo->trans_start_flag = 0;
        USBH_EHCI_PROTECT;
        /* Increment the incoming IRP count. */
        iso_qhinfo->incoming_irp_index++;

        if (iso_qhinfo->incoming_irp_index == USBH_EHCI_MAX_PEND_IRPS)
        {
            iso_qhinfo->incoming_irp_index = 0;
        }
        USBH_EHCI_UNPROTECT;
        /* Total free TDs are USBH_EHCI_MAX_ITDS_PER_IRP.
         * (0-USBH_EHCI_MAX_ITDS_PER_IRP -1). */
        iso_qhinfo->free_tds = USBH_EHCI_MAX_ITDS_PER_IRP - 1;

        /* Get buffer pointer for this transaction. */
        status = NU_USB_ISO_IRP_Get_Buffer_Array((NU_USB_ISO_IRP *) irp,
                                                             &pdata_array);
        NU_USB_ISO_IRP_Get_Lengths(iso_irp, &lengths);


        count = 0;

        for (i = 0; i < ehci->num_active_qh_periodic; i++)
        {
            if (ehci->active_qh_periodic[i] == qhinfo)
                break;
        }

        if (i == ehci->num_active_qh_periodic)
        {
            ehci->active_qh_periodic[i] = qhinfo;
            ehci->num_active_qh_periodic++;
        }


        if(qhinfo->speed != USB_SPEED_HIGH)
        {
            sitd_array_start = (USBH_EHCI_SITD*)iso_qhinfo->td_array_start;

            /* Submit TD 0 to maximum of USBH_EHCI_MAX_ITDS_PER_IRP -2. */
            while((count < (USBH_EHCI_MAX_ITDS_PER_IRP - 1)) &&
                  (irp_priv->next_index < irp_priv->no_tds))
            {

                /* Get Td pointer from Qhinfo. */
                Td = (VOID*)&sitd_array_start[iso_qhinfo->next_td_index];

                ehci_fill_sitd(ehci,(USBH_EHCI_SITD*)Td, pdata_array[count],lengths[count],1,
                                direction,qhinfo->mask,qhinfo->ssplit_info,qhinfo->address);

                ehci_schedule_iso_sitd(ehci,qhinfo,(USBH_EHCI_SITD*)Td,1,direction);

                /* Increment the global TD index. This index can go max
                 * of USBH_EHCI_MAX_ITDS_PER_IRP-1.
                 */
                iso_qhinfo->next_td_index++;

                if (iso_qhinfo->next_td_index == USBH_EHCI_MAX_ITDS_PER_IRP)
                {
                    iso_qhinfo->next_td_index = 0;
                }

                /* Decrement the global free TD count. */
                iso_qhinfo->free_tds--;

                /* Increment the transaction index associated with IRP. */
                irp_priv->next_index +=1;
                count++;
            }
        }
        else
        {
            itd_array_start = (USBH_EHCI_ITD*)iso_qhinfo->td_array_start;

            /* Read current value of frame index register */
            EHCI_HW_READ32 (ehci, ehci->ehci_oprreg_base + FRINDEX, temp);
            temp = temp % (USBH_EHCI_FRAME_LIST_SIZE << 3);
            ehci->next_uframe = temp;
            iso_qhinfo->last_td_frame_num = temp >> 3;
            iso_qhinfo->last_td_frame_num +=2;
            iso_qhinfo->last_td_frame_num %= USBH_EHCI_FRAME_LIST_SIZE;

            /* Submit TD 0 to maximum of required TDs. */
            while((count < USBH_EHCI_MAX_ITDS_PER_IRP - 1) &&
                  (irp_priv->next_index < no_transactions))
            {
                 /* Get Td pointer from Qhinfo. */
                Td = (VOID*)&itd_array_start[iso_qhinfo->next_td_index];

                itd = (USBH_EHCI_ITD*)Td;

                uframe = qhinfo->next_uframe;
                frame =  iso_qhinfo->last_td_frame_num;

                qhinfo->next_uframe += qhinfo->interval;

                if(qhinfo->next_uframe >= 8)
                    iso_qhinfo->last_td_frame_num += 1;

                qhinfo->next_uframe %= 8;
                iso_qhinfo->last_td_frame_num %= USBH_EHCI_FRAME_LIST_SIZE;

                /* Set interrupt on completion bit only for the
                 * last transaction in the ITd.
                 */
                if((iso_qhinfo->last_td_frame_num != frame)
                    || (irp_priv->next_index == (no_transactions - 1))
                    || (count == USBH_EHCI_MAX_ITDS_PER_IRP - 2))
                    ioc = 1;
                else
                    ioc = 0;

                ehci_fill_itd(ehci,itd, pdata_array[irp_priv->next_index],
                                  lengths[irp_priv->next_index],uframe,
                                  ioc,qhinfo->address,direction);

                /* Schedule the ITD in the frame */
                if((iso_qhinfo->last_td_frame_num != frame)
                    || (irp_priv->next_index == (no_transactions - 1))
                    || (count == USBH_EHCI_MAX_ITDS_PER_IRP - 2))
                {
                    /* For last transaction increment the frame */
                    if (((irp_priv->next_index == no_transactions - 1)
                        || (count == USBH_EHCI_MAX_ITDS_PER_IRP - 2))
                        && (iso_qhinfo->last_td_frame_num == frame))
                    {
                        iso_qhinfo->last_td_frame_num += 1;
                        qhinfo->next_uframe -= (8 - qhinfo->next_uframe);
                    }

                    ehci_schedule_iso_itd(ehci,qhinfo,itd,frame);

                    /* Increment the global TD index. This index can go max
                     * of USBH_EHCI_MAX_ITDS_PER_IRP-1.
                     */
                   iso_qhinfo->next_td_index++;

                   if (iso_qhinfo->next_td_index == USBH_EHCI_MAX_ITDS_PER_IRP)
                   {
                        iso_qhinfo->next_td_index = 0;
                   }

                   /* Decrement the global free TD count. */
                   iso_qhinfo->free_tds--;
                   count++;
                }
                /* Increment the transaction index associated with IRP. */
                irp_priv->next_index +=1;
            }
        }

    }
    else
    {
        /* Increment the incoming IRP count. */
        iso_qhinfo->incoming_irp_index++;

        if (iso_qhinfo->incoming_irp_index == USBH_EHCI_MAX_PEND_IRPS)
        {
            iso_qhinfo->incoming_irp_index = 0;
        }
        USBH_EHCI_UNPROTECT;

    }

    return (status);

}
/************************************************************************
*
* FUNCTION
*
*      ehci_qh_handle_irp
*
* DESCRIPTION
*
*     Handles translating the IRP in to QTD required to initiate the
*     transfer
*
* INPUTS
*
*      ehci    handle that identifies the HC in the EHCI HC database.
*      qhinfo  Identifies the QH over which the data is to be transferred.
*      irp     Ptr to the IRP that has to be transferred.
*
* OUTPUTS
*
*      NU_SUCCESS If the transfer has been setup successfully
*      NU_NO_MEMORY if available memory has been exhausted
*
*************************************************************************/

STATUS ehci_qh_handle_irp (NU_USBH_EHCI * ehci,
                           USBH_EHCI_QH_INFO * qhinfo,
                           NU_USB_IRP * irp)
{
    STATUS status = NU_USB_INVLD_ARG;
    USBH_EHCI_IRP *transfer = NU_NULL;
    NU_USB_PIPE *pipe;
    NU_USB_DEVICE *device, *parent;
    UINT8 port_num, hub_addr, speed;

    NU_USB_SETUP_PKT *setup;
    UINT8 *data;
    UINT32 data_length;
    UINT8 direction;

    UINT32 i = 0, j = 0;
    UINT32 temp1;

    /* If the queue has not been scheduled yet, schedule it */

    if (qhinfo->qh_state == USBH_EHCI_QH_NOT_READY)
    {
        /* For FULL/LOW speed devices, get the nearest hi-speed hub
         * and fill it in the qh. This is for split transfers
         */

        if (qhinfo->speed != USB_SPEED_HIGH)
        {
            NU_USB_IRP_Get_Pipe (irp, &pipe);

            NU_USB_PIPE_Get_Device (pipe, &device);

            do
            {
                status = NU_USB_DEVICE_Get_Parent (device, &parent);

                if(status != NU_SUCCESS)
                    return (status);

                NU_USB_DEVICE_Get_Speed (parent, &speed);

                device = parent;
            }
            while (speed != USB_SPEED_HIGH);

            NU_USB_DEVICE_Get_Function_Addr (parent, &hub_addr);

            status = NU_USB_DEVICE_Get_Port_Number (device, &port_num);

            if(status != NU_SUCCESS)
                return (status);


            /* Update hub addr/port number fields of qh */
            EHCI_HW_READ32(ehci, &qhinfo->hwqh->ep_controlinfo2, temp1);
            temp1 |= (hub_addr << 16);
            temp1 |= (port_num << 23);

            EHCI_HW_WRITE32(ehci, &qhinfo->hwqh->ep_controlinfo2, temp1);

        }


        /* Insert this QH into the Schedule */

        if (qhinfo->type == USB_EP_INTR)
        {
            status = ehci_schedule_intr_pipe (ehci, qhinfo);

            if(status != NU_SUCCESS)
                return (status);
        }
        else
        {
            ehci_link_async_qh (ehci, qhinfo);
        }

    }

    /* Allocate the transfer structure for this IRP */
    USBH_EHCI_PROTECT;
    status = NU_Allocate_Memory (ehci->cacheable_pool, (VOID *) &transfer,
                                 sizeof (USBH_EHCI_IRP), NU_NO_SUSPEND);
    USBH_EHCI_UNPROTECT;
    if (status != NU_SUCCESS)
        return status;

    /* Initialize the Transfer structure */

    memset (transfer, 0, sizeof (USBH_EHCI_IRP));

    transfer->qh = qhinfo;
    transfer->irp = irp;

    for (i = 0; i < USBH_EHCI_MAX_BURSTS; i++)
    {
        transfer->burst[i].transfer = transfer;
        transfer->burst[i].index = i;
        transfer->burst[i].num_qtds = 0;
        transfer->burst[i].state = USBH_EHCI_BURST_IDLE;
    }

    /* Gather IRP details   */

    NU_USB_IRP_Get_Length (irp, &data_length);
    NU_USB_IRP_Get_Data (irp, &data);

    direction = qhinfo->bEndpointAddress & USB_DIR_IN;

    /* Set the transfer state */
    transfer->rem_data = data;
    transfer->rem_length = data_length;
    transfer->direction = direction;

    switch (qhinfo->type)
    {
        case USB_EP_CTRL:

            NU_USBH_CTRL_IRP_Get_Setup_Pkt ((NU_USBH_CTRL_IRP *) irp, &setup);
            NU_USBH_CTRL_IRP_Get_Direction ((NU_USBH_CTRL_IRP *) irp,
                                            &direction);
            transfer->direction = direction;

            status = ehci_qh_handle_ctrl_irp (ehci, qhinfo, transfer, setup);
            break;

        case USB_EP_BULK:
        case USB_EP_INTR:

            status = ehci_qh_handle_data_irp (ehci, qhinfo, transfer);

            if (status != NU_SUCCESS)
                break;

            if (qhinfo->type == USB_EP_INTR)
            {

                for (i = 0; i < ehci->num_active_qh_periodic; i++)
                {
                    if (ehci->active_qh_periodic[i] == qhinfo)
                        break;
                }

                if (i == ehci->num_active_qh_periodic)
                {
                    ehci->active_qh_periodic[i] = qhinfo;
                    ehci->num_active_qh_periodic++;
                }
            }
            break;

        default:
            NU_ASSERT(0);
    }


    if (status != NU_SUCCESS)
    {
        /* the submit_*_irp has returned failure. we need to reclaim all
         * the allocated memory - QTDs, EHCI_IRP (transfer)
         */
        for (i = 0; i < USBH_EHCI_MAX_BURSTS; i++)
        {
            for (j = 0; j < transfer->burst[i].num_qtds; j++)
            {
                ehci_dealloc_qtd (ehci, transfer->burst[i].hwqtd[j]);
            }
        }
        NU_Deallocate_Memory (transfer);
    }

    return status;
}


/************************************************************************
*
* FUNCTION
*
*      ehci_delete_iso_tds
*
* DESCRIPTION
*
*      This function deletes allocated isochronous TDs.
*
* INPUTS
*
*      qhinfo      Isochronous QHinfo.
*
* OUTPUTS
*
*      None
*
*************************************************************************/
VOID ehci_delete_iso_tds(USBH_EHCI_ISO_QH_INFO* qhinfo)
{

        NU_Deallocate_Memory(qhinfo->td_array_start);
        qhinfo->td_array_start = NU_NULL;

}

/************************************************************************
*
* FUNCTION
*
*      ehci_check_retire_hs_iso_transfer
*
* DESCRIPTION
*
*      This function checks ITDs of the submitted IRP for any retired
*      ITDs. Retires if it finds one.Also submits more ITDs for
*      any pending IRPs if free ITD is available.
*
* INPUTS
*
*      ehci        handle that identifies the HC in the EHCI HC database.
*      qhinfo      QH to be checked for retired transfer
*      irp_priv    Pointer to the ISO IRP structure
*      index       Index of the QHinfo in the periodic list.
*      flag        Indicates if the ITD is retired by HW or abnormally
*                  terminated by the software due to pipe close or flush.
*                  (USBH_EHCI_ITD_RETIRED or USBH_EHCI_ITD_CANCELLED)
*
* OUTPUTS
*
*      NU_SUCCESS                     ITD retired successfully.
*      USBH_EHCI_ITD_NOT_RETIRED      ITD not retired.
*
*************************************************************************/

STATUS ehci_retire_hs_iso_transfer (NU_USBH_EHCI * ehci,
                                    USBH_EHCI_QH_INFO* qhinfo,
                                    USBH_EHCI_ISO_IRP *irp_priv,
                                    USBH_EHCI_ITD *pitd,
                                    UINT32 flag)
{
    USBH_EHCI_ITD *itd;
    NU_USB_ISO_IRP *irp = NU_NULL;
    UINT32 i,j,n;
    UINT8 direction = qhinfo->bEndpointAddress & 0x80;
    UINT32  temp = 0;
    USBH_EHCI_ISO_QH_INFO *iso_qhinfo = (USBH_EHCI_ISO_QH_INFO*)qhinfo;
    UINT16 *lengths;
    UINT16 *actual_lengths;
    BOOLEAN submit_next_td;
    UINT8 **pbuffer_array;
    USBH_EHCI_ISO_IRP *current_irp_priv;
    NU_USB_IRP *base_irp;
    NU_USB_IRP_CALLBACK callback;
    NU_USB_PIPE *pipe;
    USBH_EHCI_ITD **prev;
    UINT32 uframe,frame;
#if(USBH_EHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)
    BOOLEAN sync = NU_FALSE;
#endif
    UINT8 ioc = 0;
    UINT16 no_transactions = 0;
    BOOLEAN check_trans = NU_FALSE;
    STATUS tx_status = NU_SUCCESS;
    USBH_EHCI_ITD* itd_array = (USBH_EHCI_ITD*)(iso_qhinfo->td_array_start);



    if(flag == USBH_EHCI_ITD_CANCELLED)
    {
        j = iso_qhinfo->current_irp_index;

        while(j < iso_qhinfo->incoming_irp_index)
        {
            irp_priv = &iso_qhinfo->irps[j];
            for(i=0;i<irp_priv->next_index;i++)
            {
                itd = (USBH_EHCI_ITD*)&itd_array[i];
                prev = (USBH_EHCI_ITD**) &ehci->periodic_list_base[itd->frame];
                *prev = (USBH_EHCI_ITD*)itd->next;


                for(n=0;n<8;n+=qhinfo->interval)
                {

                    #if(USBH_EHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)

                    if(direction & USB_DIR_OUT)
                        sync = NU_FALSE;
                    else
                        sync = NU_TRUE;

/*                    NU_USB_HWENV_Tx_Done(ehci->hwenv,
                                     itd->data[n],
                                     itd->non_cache_ptr[n],
                                     itd->raw_data[n],
                                     itd->length[n],
                                     1,
                                     sync);
*/
                    #endif
                }

            }

            base_irp = (NU_USB_IRP*)irp_priv->irp;


            NU_USB_IRP_Get_Callback(base_irp, &callback);
            NU_USB_IRP_Get_Pipe(base_irp, &pipe);
            NU_USB_IRP_Set_Status (base_irp, NU_USB_IRP_CANCELLED);
            NU_USB_ISO_IRP_Set_Actual_Num_Transactions (irp_priv->irp, irp_priv->cnt);

            if (callback)
            {
                callback(pipe, base_irp);
            }

            j++;
        }
        return NU_SUCCESS;
    }


    itd = (USBH_EHCI_ITD*)&itd_array[iso_qhinfo->ret_td_index];

    irp = irp_priv->irp;

    NU_USB_ISO_IRP_Get_Actual_Lengths(irp, &actual_lengths);
    NU_USB_ISO_IRP_Get_Lengths(irp, &lengths);
    NU_USB_ISO_IRP_Get_Num_Transactions (irp ,
                                    &no_transactions);

    /* For last IRP number of transactions  can be less
     * than 8.
     */
    if(no_transactions % irp_priv->no_tds)
    {
        if((irp_priv->cnt + itd->trans_per_td) == no_transactions)
            check_trans = NU_TRUE;
    }

    for(i=0;i<8;i+=qhinfo->interval)
    {
        if(check_trans)
        {
            if((i+1) > itd->trans_per_td)
                break;
        }

        EHCI_HW_READ32(ehci,&itd->trans_info[i],temp);

        if(temp & EHCI_ITD_ACTIVE)
        {
            /* ITD  still active. Return from the retirement routine. */
            return USBH_EHCI_ITD_NOT_RETIRED;
        }
    }

    for(i=0;i<8;i+=qhinfo->interval)
    {
        if(check_trans)
        {
            if((i+1) > itd->trans_per_td)
                break;
        }

        EHCI_HW_READ32(ehci,&itd->trans_info[i],temp);


        if(direction)
            actual_lengths[irp_priv->cnt] =  lengths[irp_priv->cnt] - EHCI_ITD_TRA_LENGTH(temp);
        else
            /* OUT */
            actual_lengths[irp_priv->cnt] = lengths[irp_priv->cnt];



#if(USBH_EHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)

        if(direction & USB_DIR_OUT)
            sync = NU_FALSE;
        else
            sync = NU_TRUE;

/*        NU_USB_HWENV_Tx_Done(ehci->hwenv,
                             itd->data[i],
                             itd->non_cache_ptr[i],
                             itd->raw_data[i],
                             actual_lengths[irp_priv->cnt],
                             1,
                             sync);
*/
#endif
        itd->non_cache_ptr[i] = NU_NULL;
        itd->raw_data[i] = NU_NULL;
        irp_priv->cnt++;
        if(irp_priv->cnt == no_transactions)
            break;
    }


    /* If the current ITD is inactive */
    if (!(temp & EHCI_ITD_ACTIVE))
    {
        itd->trans_per_td = 0;
        iso_qhinfo->ret_td_index++;

        if(iso_qhinfo->ret_td_index == USBH_EHCI_MAX_ITDS_PER_IRP)
            iso_qhinfo->ret_td_index = 0;

        /* Unlink the ITD from the periodic schedule */
        prev = (USBH_EHCI_ITD**) &ehci->periodic_list_base[itd->frame];
        *prev = (USBH_EHCI_ITD*)itd->next;

        itd->pg = 0;

        /* Increase the global free TD count. */
        iso_qhinfo->free_tds++;

        /* Load IRP, transactions of which will be submitted. */
        irp_priv = &iso_qhinfo->irps[iso_qhinfo->next_irp_index];

        /* Another IRP has been submitted, from the time no_more_irp was
         * marked last time.
         */
        if ((iso_qhinfo->next_irp_index != iso_qhinfo->incoming_irp_index)
            && (iso_qhinfo->no_more_irp == 1))
        {
            iso_qhinfo->no_more_irp = 0;
        }

        /* submit_next_td = 1, force this function to occupy only one more
         * empty slot of TD if available. This keeps load on this function
         * called from ISR moderate.
         */
        submit_next_td = 1;

        while ((iso_qhinfo->free_tds != 0) &&
           (iso_qhinfo->no_more_irp == 0) && (submit_next_td != 0))
        {

            submit_next_td = 0;

            NU_USB_ISO_IRP_Get_Num_Transactions (irp_priv->irp ,
                                    &no_transactions);

            /* If all the TDs associated with this IRP has been submitted. */
            if (irp_priv->next_index == no_transactions)
            {
                /* Increment the index. */
                iso_qhinfo->next_irp_index++;

                if (iso_qhinfo->next_irp_index == USBH_EHCI_MAX_PEND_IRPS)
                {
                    iso_qhinfo->next_irp_index = 0;
                }

                /* At this point, submit more IRPs, if space is available. */
                submit_next_td = 1;

                /* If next irp index reaches incoming irp index. */
                if (iso_qhinfo->next_irp_index ==
                           iso_qhinfo->incoming_irp_index)
                {
                    /* No more IRPs to submit. */
                    iso_qhinfo->last_irp = iso_qhinfo->next_irp_index - 1;
                    iso_qhinfo->no_more_irp = 1;

                    /* Break the submission loop. */
                    break;
                }

                /* Get td_priv at the next irp index. */
                irp_priv =
                     &iso_qhinfo->irps[iso_qhinfo->next_irp_index];
            }

            /* If there are more TDs to submit. */
            if (iso_qhinfo->no_more_irp == 0)
            {
                irp = irp_priv->irp;

                /* Get buffer length associated with this IRP. */
                NU_USB_ISO_IRP_Get_Lengths(irp, &lengths);

                /* Get buffer pointer associated with this IRP. */
                NU_USB_ISO_IRP_Get_Buffer_Array(irp, &pbuffer_array);

                NU_USB_ISO_IRP_Get_Num_Transactions (irp ,
                                        &no_transactions);

                for (i=0; i<8; i+=qhinfo->interval)
                {
                    /* Get itd at the next free location. */
                    itd = (USBH_EHCI_ITD*)&itd_array[iso_qhinfo->next_td_index];

                    uframe = qhinfo->next_uframe;
                    frame =  iso_qhinfo->last_td_frame_num;

                    qhinfo->next_uframe += qhinfo->interval;

                    if(qhinfo->next_uframe >= 8)
                        iso_qhinfo->last_td_frame_num += 1;

                    qhinfo->next_uframe %= 8;
                    iso_qhinfo->last_td_frame_num %= USBH_EHCI_FRAME_LIST_SIZE;


                    if( (iso_qhinfo->last_td_frame_num != frame)
                        || (irp_priv->next_index == no_transactions - 1))
                        ioc = 1;
                    else
                        ioc = 0;

                    ehci_fill_itd(ehci,itd, pbuffer_array[irp_priv->next_index],
                                      lengths[irp_priv->next_index],
                                      uframe,ioc,qhinfo->address,direction);


                    if((iso_qhinfo->last_td_frame_num != frame)
                        || (irp_priv->next_index == (no_transactions - 1)))
                    {
                        ehci_schedule_iso_itd(ehci,qhinfo,itd,frame);

                        /* next_td_index can't be more than
                         * USBH_EHCI_MAX_ITDS_PER_IRP.
                         */
                        iso_qhinfo->next_td_index++;

                        if(iso_qhinfo->next_td_index == USBH_EHCI_MAX_ITDS_PER_IRP)
                             iso_qhinfo->next_td_index =  0;

                        /* TD is submitted, decrement the global free TD count and
                         * increment the TD count associated with this irp.
                         */
                        iso_qhinfo->free_tds--;

                    }

                    if((irp_priv->next_index  == no_transactions - 1)
                        && (iso_qhinfo->last_td_frame_num == frame))
                    {
                        iso_qhinfo->last_td_frame_num += 1;
                        qhinfo->next_uframe -= (8 - qhinfo->next_uframe);
                        irp_priv->next_index += 1;
                        break;
                    }
                     /* Increment the index associated with IRP the number of
                      * transactions submitted in this TD.
                      */
                    irp_priv->next_index += 1;
                }


            }
        }

        /* Again load current IRP. */
        current_irp_priv =
              &iso_qhinfo->irps[iso_qhinfo->current_irp_index];

        NU_USB_ISO_IRP_Get_Num_Transactions (current_irp_priv->irp ,
                                        &no_transactions);


        /* Invoke Callback, if all the TDs of this IRP has been processed. */
        if (current_irp_priv->cnt == no_transactions)
        {
            base_irp = (NU_USB_IRP*)current_irp_priv->irp;

            ehci_normalize_itd_trans_status(ehci, temp,
                                     &tx_status, direction);

            /* Call the callback function. */
            NU_USB_IRP_Get_Callback(base_irp, &callback);
            NU_USB_IRP_Get_Pipe(base_irp, &pipe);
            NU_USB_IRP_Set_Status (base_irp, tx_status);
            NU_USB_ISO_IRP_Set_Actual_Num_Transactions (current_irp_priv->irp, current_irp_priv->cnt);

            if (callback)
            {
                callback(pipe, base_irp);
            }

            /* Check for last IRP callback. */
            if ((iso_qhinfo->last_irp == iso_qhinfo->current_irp_index) &&
                 (iso_qhinfo->no_more_irp == 1))
            {
                /* Reinitializing the ED indexes. */
                iso_qhinfo->no_more_irp = 0;
                iso_qhinfo->last_irp = 0;
                iso_qhinfo->current_irp_index = 0;
                iso_qhinfo->next_irp_index = 0;
                iso_qhinfo->incoming_irp_index = 0;
            }
            else
            {
                /* Otherwise increment the current index. */
                iso_qhinfo->current_irp_index++;
                if (iso_qhinfo->current_irp_index == USBH_EHCI_MAX_PEND_IRPS)
                {
                    iso_qhinfo->current_irp_index = 0;
                }
            }

        }
        return (NU_SUCCESS);
    }

    return (USBH_EHCI_ITD_NOT_RETIRED);
}

/************************************************************************
*
* FUNCTION
*
*      ehci_disable_interrupts
*
* DESCRIPTION
*
*      This function disables all the interrupts of EHCI by clearing
*      interrupt enable register.
*
* INPUTS
*
*      ehci        EHCI controller handle
*
* OUTPUTS
*
*      NONE
*
*
*************************************************************************/
VOID ehci_disable_interrupts(NU_USBH_EHCI   *ehci)
{

    /* Disable the EHCI Interrupts */
    EHCI_HW_WRITE32 (ehci, ehci->ehci_oprreg_base + USBINTR,0);

    /* Keep track of interrupt disable calls. Only the top most
     * enable/disable pair will change interrupts. */

    ehci->int_disable_count++;

}

/************************************************************************
*
* FUNCTION
*
*      ehci_enable_interrupts
*
* DESCRIPTION
*
*      This function enables interrupts of EHCI by setting bits of
*      interrupt enable register.
*
* INPUTS
*
*      ehci        EHCI controller handle
*
* OUTPUTS
*
*      NONE
*
*
*************************************************************************/

VOID ehci_enable_interrupts(NU_USBH_EHCI    *ehci)
{

    if(ehci->int_disable_count)
    {
        ehci->int_disable_count--;
    }

    /* Enable interrupts only when this routine is called as many times as
     * disable interrupt routine is called.
     */

    if(ehci->int_disable_count == NU_NULL)
    {
        /* Enable the EHCI Interrupts */
        EHCI_HW_WRITE32 (ehci, ehci->ehci_oprreg_base + USBINTR,
                     USBH_EHCI_INTR_ENABLE_MASK);
    }

}


/************************************************************************
* FUNCTION
*
*      ehci_scan_isochronous_tds
*
* DESCRIPTION
*
*      This function scans the periodic list for any retired sitds or itds
*      and re-submits another irp / retires transfers
*
* INPUTS
*
*      ehci        EHCI Control block
*
* OUTPUTS
*
*      None
*
*************************************************************************/

VOID ehci_scan_isochronous_tds (NU_USBH_EHCI * ehci)
{

    USBH_EHCI_ISO_QH_INFO *iso_qhinfo = NU_NULL;
     USBH_EHCI_ISO_IRP  *irp_priv;
    UINT32 l=0;
    UINT32 frame ,clock,now_uframe,mod,temp,type,now,prev;
    UINT32 results = EHCI_SITD_ACTIVE;
    USBH_EHCI_SITD *sitd;
    USBH_EHCI_ITD  *itd;
    mod = USBH_EHCI_FRAME_LIST_SIZE << 3;

    now_uframe = ehci->next_uframe;

    EHCI_HW_READ32 (ehci, ehci->ehci_oprreg_base + FRINDEX, clock);
    clock = clock % mod;

    for(;;)
    {
        frame = now_uframe >> 3;
        if(frame != (clock >> 3))
        {
            now_uframe |= 0x07;
        }
        frame %= USBH_EHCI_FRAME_LIST_SIZE;

        prev =  (UINT32)&ehci->periodic_list_base[frame];
        EHCI_READ_ADDRESS(ehci,prev,temp);
        type = temp & (0x03 << 1);


        while((temp != NU_NULL)  &&  (!(temp & 0x1)))
        {
            switch ( type )
            {
                case USBH_EHCI_TYPE_SITD:

                    sitd = (USBH_EHCI_SITD*)(temp & ~0x1E);

                    while( results & EHCI_SITD_ACTIVE )
                    {
                        EHCI_HW_READ32(ehci, &(sitd->results), results);
                        l++;
                        if ( l > 10)
                            break;
                    }
                    l = 0;

                    if((!(results & EHCI_SITD_ACTIVE)) && (!(temp & 0x01)))
                    {
                        iso_qhinfo = sitd->qhinfo;


                        if(iso_qhinfo)
                        {
                            if(iso_qhinfo->current_irp_index != iso_qhinfo->incoming_irp_index)
                            {
                                irp_priv = &iso_qhinfo->irps[iso_qhinfo->current_irp_index];

                                if(irp_priv != NU_NULL)
                                {

                                        ehci_retire_fs_iso_transfer(ehci,
                                                      (USBH_EHCI_QH_INFO*)iso_qhinfo,irp_priv,sitd,
                                                      USBH_EHCI_ITD_RETIRED
                                                      );

                                }
                            }
                        }
                        EHCI_READ_ADDRESS(ehci,&sitd->next,temp);
                        results = EHCI_SITD_ACTIVE;
                        type = temp & (0x03 << 1);

                        if(temp & 0x01)
                        {
                            temp = NU_NULL;
                        }
                    }
                    else
                        temp = NU_NULL;
                    break;

                    case USBH_EHCI_TYPE_ITD:

                        itd = (USBH_EHCI_ITD*)(temp & ~0x1E);


                        if( (!(temp & 0x01)))
                        {
                            iso_qhinfo = itd->qhinfo;

                            if(iso_qhinfo)
                            {
                                if(iso_qhinfo->current_irp_index != iso_qhinfo->incoming_irp_index)
                                {
                                    irp_priv = &iso_qhinfo->irps[iso_qhinfo->current_irp_index];

                                    if(irp_priv != NU_NULL)
                                    {

                                            ehci_retire_hs_iso_transfer(ehci,
                                                          (USBH_EHCI_QH_INFO*)iso_qhinfo,irp_priv,itd,
                                                          USBH_EHCI_ITD_RETIRED
                                                          );
                                    }
                                }
                            }
                            EHCI_READ_ADDRESS(ehci,&itd->next,temp);

                            type = temp & (0x03 << 1);

                            if(temp & 0x01)
                            {
                                temp = NU_NULL;
                            }
                        }
                        else
                            temp = NU_NULL;
                    break;
                    default:
                        temp = NU_NULL;
            }
        }

        if(now_uframe == clock)
        {
            ehci->next_uframe = now_uframe;
            EHCI_HW_READ32 (ehci, ehci->ehci_oprreg_base + FRINDEX, now);
            now = now % mod;
            if(now_uframe == now)
                break;

            clock = now;
        }
        else
        {
            now_uframe++;
            now_uframe %= mod;

        }
    }
}

/**************************************************************************
*
* FUNCTION
*
*       ehci_transfer_done
*
* DESCRIPTION
*
*       This function is called on completion of transfer if EHCI
*       allocated an uncachable buffer for handling an IRP.
*
* INPUTS
*
*       ehci                                Pointer to the EHCI structure
*                                           containing root-hub data.
*       actual_buffer                       Actual buffer in which data
*                                           is to be copied.
*       non_cache_buffer                    Non-cachabke buffer containing
*                                           data.
*       raw_buffer                          Pointer to original uncachable
*                                           buffer allocated earlier.
*       length                              Length of data to be copied.
*       sync                                Specifies if contents of actual
*                                           buffer and uncachable buffer
*                                           should be synchronized or not.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
**************************************************************************/
STATUS ehci_transfer_done(NU_USBH_EHCI    *ehci,
                          UINT8           *actual_buffer,
                          UINT8           *non_cache_buffer,
                          UINT8           *raw_buffer,
                          UINT32          length,
                          BOOLEAN         sync)
{
    STATUS status;

    /* Initialize status as success. */
    status = NU_SUCCESS;

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       ehci_normalize_buffer
*
* DESCRIPTION
*
*       This function converts "buffer" into a cache-coherent, aligned
*       memory buffer.
*
* INPUTS
*
*       ehci                                Pointer to the EHCI structure
*                                           containing root-hub data.
*       buffer                              Pointer to buffer to be
*                                           normalized.
*       length                              Length of the buffer.
*       alignment                           Byte alignment required for the
*                                           buffer.
*       sync                                Output buffer contents needed
*                                           to be same as input buffer if
*                                           TRUE.
*       buffer_out                          Pointer to a memory location
*                                           to hold the pointer to a
*                                           aligned, non-cached buffer.
*       raw_buffer                          Pointer to a memory location to
*                                           hold the pointer to the memory
*                                           buffer, out of which aligned
*                                           memory pointed to by
*                                           ‘buffer_out’ is formed.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
**************************************************************************/
STATUS ehci_normalize_buffer(NU_USBH_EHCI    *ehci,
                             UINT8           *buffer,
                             UINT32          length,
                             UINT32          alignment,
                             BOOLEAN         sync,
                             UINT8           **buffer_out,
                             UINT8           **raw_buffer)
{
    STATUS status;

    /* Initialize status as success. */
    status = NU_SUCCESS;

    if ( length )
    {
        *buffer_out = buffer;
        *raw_buffer = *buffer_out;
    }
    else
    {
        *buffer_out = NU_NULL;
        *raw_buffer = NU_NULL;
    }

    return (status);
}
/* =======================  End Of File  ============================== */
