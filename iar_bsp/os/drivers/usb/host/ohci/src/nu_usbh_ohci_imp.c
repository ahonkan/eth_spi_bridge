/**************************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*       nu_usbh_ohci_imp.c
*
*
* COMPONENT
*
*       OHCI Driver / Nucleus USB Host Software
*
* DESCRIPTION
*
*       This file contains the worker functions for the Generic OHCI
*       driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*      ohci_alloc_gen_td              Allocates a generic TD.
*      ohci_cc_2_usb_status           Converts OHCI status to generic error
*                                     code.
*      ohci_check_iso_trans_per_td    Checks number of ISO transfers that
*                                     can be sent in one TD.
*      ohci_close_non_periodic_ed     Removes a-periodic EDs.
*      ohci_close_periodic_ed         Removes a-periodic EDs.
*      ohci_dealloc_all_irp           Removes all the IRPs submitted on a
*                                     Control, Bulk or Interrupt endpoint.
*      ohci_dealloc_all_iso_irp       Removes all the IRPs submitted on an
*                                     Isochronous endpoint.
*      ohci_dealloc_all_iso_tds       Removes all scheduled TDs form the
*                                     Isochronous endpoint.
*      ohci_dealloc_all_tds           Removes all scheduled TDs form the
*                                     Control, Bulk or Interrupt endpoint.
*      ohci_dealloc_gen_td            De-allocates a generic TD.
*      ohci_ed_key                    Frames a hash key.
*      ohci_fill_iso_td               Fills a pre-allocated hardware ISO TD.
*      ohci_fill_td                   Fills a pre-allocated hardware TD.
*      ohci_find_best_schedule        Finds the schedule for a transfer.
*      ohci_find_td_count             Calculates the TD count.
*      ohci_handle_irp                Transfers submission handling.
*      ohci_handle_iso_irp            Transfers submission handling for ISO.
*      ohci_hash_add_ed               Adds ED into the hash table.
*      ohci_hash_delete_ed            Removes ED from the hash table.
*      ohci_hash_find_ed              Locates an ED in the hash table
*      ohci_hndl_retired_iso_td       Handles retried TDs for ISO.
*      ohci_hndl_retired_td           Handles retried TDs.
*      ohci_hndl_td_done_q            Handles DoneQ interrupt.
*      ohci_make_td                   Fills the TD structure.
*      ohci_master_int_dis            Disables OHCI’s master interrupt.
*      ohci_master_int_en             Enables OHCI’s master interrupt.
*      ohci_normalize_interval        Normalizes the transmission interval.
*      ohci_rh_clear_feature          Root Hub request processing.
*      ohci_rh_clear_hub_feature      Root Hub request processing.
*      ohci_rh_clear_port_feature     Root Hub request processing.
*      ohci_rh_get_descriptor         Root Hub request processing.
*      ohci_rh_get_state              Root Hub request processing.
*      ohci_rh_get_status             Root Hub request processing.
*      ohci_rh_handle_irp             Root Hub transfer processing.
*      ohci_rh_init                   Initializes root hub.
*      ohci_rh_invalid_cmd            Root Hub request processing.
*      ohci_rh_isr                    Root-hub ISR.
*      ohci_rh_nothing_to_do_cmd      Root Hub request processing.
*      ohci_rh_set_feature            Root Hub request processing.
*      ohci_rh_set_hub_feature        Root Hub request processing.
*      ohci_rh_set_port_feature       Root Hub request processing.
*      ohci_scheduleBulkPipe          Bulk pipes scheduling.
*      ohci_scheduleControlPipe       Control pipes scheduling.
*      ohci_schedulePeriodicPipe      Periodic pipe scheduling.
*      ohci_unlink_tds                Unlinks the TDs.
*
* DEPENDENCIES
*
*       nu_usb.h                            USB Definitions.
*       nu_usbh_ohci_dvm.h
*
**************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "drivers/nu_usbh_ohci_ext.h"
#include "drivers/nu_drivers.h"



/* ============  Descriptors of Root Hub =============================  */

static UINT8    rh_dev_desc[] =
{
    /* Device Descriptor    */

    18,                                      /* bLength.           */
    1,                                       /* DEVICE.            */
    0x10,                                    /* USB 1.1            */
    0x1,                                     /* USB 1.1            */
    0x09,                                    /* HUB CLASS.         */
    0x00,                                    /* Subclass.          */
    0x00,                                    /* Protocol.          */
    0x08,                                    /* bMaxPktSize0.      */
    0x00,                                    /* idVendor.          */
    0x0,                                     /* idVendor.          */
    0x0,                                     /* idProduct.         */
    0x0,                                     /* idProduct.         */
    0x0,                                     /* bcdDevice.         */
    0x0,                                     /* bcdDevice.         */
    0x0,                                     /* iManufacturer.     */
    0x0,                                     /* iProduct.          */
    0x0,                                     /* iSerial Number.    */
    1                                        /* One configuration. */
};

static UINT8    rh_cfg_desc[] =
{
    /* Configuration Descriptor. */

    9,                                       /* bLength.             */
    2,                                       /* CONFIGURATION.       */
    25,                                      /* length.              */
    0,                                       /* length.              */
    1,                                       /* bNumInterfaces.      */
    0x01,                                    /* bConfigurationValue. */
    0x00,                                    /* iConfiguration.      */
    0xc0,                                    /* bmAttributes.        */
    0,                                       /* power.               */

    /* Interface Descriptor. */

    9,                                       /* bLength.            */
    4,                                       /* INTERFACE.          */
    0,                                       /* bInterfaceNumber.   */
    0,                                       /* bAlternateSetting.  */
    1,                                       /* bNumEndpoints.      */
    0x09,                                    /* bInterfaceClass.    */
    0x00,                                    /* bInterfaceSubClass. */
    0x00,                                    /* bInterfaceProtocol. */
    0x00,                                    /* iInterface.         */

    /* Endpoint Descriptor      */

    7,                                       /* bLength.          */
    5,                                       /* ENDPOINT.         */
    0x81,                                    /* bEndpointAddress. */
    0x03,                                    /* bmAttributes.     */
    0x10,                                    /* wMaxPacketSize.   */
    0x00,                                    /* wMaxPacketSize.   */
    0x01                                     /* bInterval.        */
};

/**************************************************************************
*
* FUNCTION
*
*       ohci_ed_key
*
* DESCRIPTION
*
*       Forms a key for hashing ED in to hash table, based on the function
*       address and bEndpointAddress.
*
* INPUTS
*
*       function_address                    USB Function Address.
*       bEndpointAddress                    bEndpointAddress field of the
*                                           endpoint descriptor.
*
* OUTPUTS
*
*       UINT32                              key To be used with hash
*                                           function for hashing the ED.
*
**************************************************************************/

UINT32 ohci_ed_key(UINT8 function_address, UINT8 bEndpointAddress)
{
    UINT32  key;
    UINT8   endpoint = bEndpointAddress & 0xf;

    /* The key is made of 7 bits of function_address, 4 bit endpoint number
     * of bEndpointAddress and 1 direction bit of bEndpointAddress.
     */
    key = (function_address & USBH_OHCI_EDCTL_FA_MASK) |
          ((endpoint << 7) & USBH_OHCI_EDCTL_EN_MASK);

    if ((bEndpointAddress & USB_DIR_IN) == NU_NULL)
    {
        key |= (USBH_OHCI_EDCTL_DIR_OUT);
    }

    return (key);
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_alloc_gen_td
*
* DESCRIPTION
*
*       Allocates a TD at a memory address that is 16 byte aligned. This
*       can be used for Control/Bulk/Interrupt TDs.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       **td_ptr                            Return ptr that would contain
*                                           the address of the TD upon
*                                           successful allocation.
*       suspend                             Suspend option - NU_SUSPEND or
*                                           NU_NO_SUSPEND.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful allocation
*                                           of the ED.
*       NU_NO_MEMORY                        Indicates that the memory
*                                           allocation has
*                                           failed or that all the TD pools
*                                           are fully occupied.
*
*
**************************************************************************/
STATUS ohci_alloc_gen_td(NU_USBH_OHCI *ohci,
                         USBH_OHCI_TD **td_ptr,
                         USBH_OHCI_ED_INFO *ed_info,
                         UNSIGNED suspend)
{
    STATUS                  status = NU_SUCCESS;
    UINT8                   id;
    UINT8 pool_count;
    OHCI_ALIGNED_MEM_POOL   *pool;
    UINT8 pool_int_required = NU_TRUE;


    /* Check if OHCI's control block is valid. */
    NU_ASSERT(ohci != NU_NULL);
    NU_ASSERT(~((UINT32)ohci & 0x3));

    USBH_OHCI_PROTECT;

    /* Search for a free TD in the allocated pools. */
    for (pool_count = 0; pool_count < OHCI_MAX_TD_GEN_POOLS; pool_count++)
    {
        pool = &(ohci->td_gen_pool[pool_count]);

        /* Check if Pool's control block is valid. */
        NU_ASSERT(pool != NU_NULL);
        NU_ASSERT(~((UINT32)pool & 0x3));

        /* check if this pool has been initialized and there are TDs
         * available.
         */
        if ((pool->start_ptr) && (pool->avail_elements))
        {
            pool_int_required = NU_FALSE;

            break;
        }
    }

    if(pool_int_required == NU_TRUE)
    {
        /* See if a pool slot is free and if so allocate a new pool and
         * grab the ED from the newly allocated pool.
         */
        for (pool_count = 0; pool_count < OHCI_MAX_TD_GEN_POOLS; pool_count++)
        {
            status = NU_NO_MEMORY;

            pool = &(ohci->td_gen_pool[pool_count]);

            /* Check if Pool's control block is valid. */
            NU_ASSERT(pool != NU_NULL);
            NU_ASSERT(~((UINT32)pool & 0x3));

            if (pool->start_ptr == NU_NULL)
            {
                status = NU_Allocate_Aligned_Memory(ohci->cb.pool,
                            (VOID **) &(pool->start_ptr),
                            (OHCI_MAX_TDS_PER_POOL * sizeof(USBH_OHCI_TD)),
                            OHCI_ED_TD_ALIGNMENT,
                            suspend);

                if (status == NU_SUCCESS)
                {
                    /* Initialize the available elements in this pool. */
                    pool->avail_elements = OHCI_MAX_TDS_PER_POOL;
                }

                break;
            }
        }
    }

    if((status == NU_SUCCESS) && (pool_count != OHCI_MAX_TD_GEN_POOLS))
    {
        id = usb_get_id(pool->bits,
                         OHCI_MAX_TDS_PER_POOL / 8,
                         OHCI_MAX_TDS_PER_POOL - 1);

        if (id == USB_NO_ID)
        {
            NU_ASSERT(0);

            /* This was not suppose to happen as we have checked that
             * elements are available.
             */
            status = NU_INVALID_MEMORY;
        }
        else
        {
            /* Check if Pool's start pointer is valid. */
            NU_ASSERT(pool->start_ptr != NU_NULL);
            NU_ASSERT(~((UINT32)pool->start_ptr & 0x3));

            *td_ptr = (USBH_OHCI_TD *)
                      (pool->start_ptr + (id * sizeof(USBH_OHCI_TD)));

            pool->avail_elements--;
        }
     }

    USBH_OHCI_UNPROTECT;

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_dealloc_gen_td
*
* DESCRIPTION
*
*       De-allocates the TD that was earlier allocated from the TD pool. If
*       the deallocation results in all the elements in the pool to be
*       available and if its not the first pool, then the pool itself is
*       released. It thus ensures that at least 1 pool is always available
*       even if no TDs are allocated from it.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *td_ptr                             TD ptr that has to be released.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           de-allocation of the TD.
*       NU_NOT_PRESENT                      Indicates that the ed_ptr is
*                                           not found in the pool table.
*
**************************************************************************/
STATUS  ohci_dealloc_gen_td (NU_USBH_OHCI *ohci, USBH_OHCI_TD *td_ptr)
{
    OHCI_ALIGNED_MEM_POOL   *pool;
    UINT32 first_td;
    UINT32 last_td;
    UINT8 pool_count;
    STATUS status = NU_INVALID_MEMORY;
    UINT8 id;

    USBH_OHCI_PROTECT;

    /* Search for a free TD in the allocated pools. */
    for (pool_count = 0; pool_count < OHCI_MAX_TD_GEN_POOLS; pool_count++)
    {
        /* Get the control block of the pool to which this TD belongs. */
        pool = &(ohci->td_gen_pool[pool_count]);

        /* Get the address of the first TD form this pool. */
        first_td = (UINT32) pool->start_ptr;

        /* Get the address of the last TD by adding total number of TDs in
         * the start address.
         */
        last_td = first_td +
                      (sizeof(USBH_OHCI_TD) * (OHCI_MAX_TDS_PER_POOL - 1));

        /* Check if the TD belongs to this pool. */
        if(((UINT32) td_ptr >= first_td) && ((UINT32) td_ptr <= last_td))
        {
            /* Calculate the ID of this TD. */
            id = (((UINT32)td_ptr - first_td) / sizeof(USBH_OHCI_TD));

            /* Mark the TD as free. */
            usb_release_id(pool->bits, (OHCI_MAX_TDS_PER_POOL / 8), id);

            /* Increment number of available TDs in this pool. */
            pool->avail_elements++;

           /* Always have at least one pool , the first pool  to prevent
             * memory fragmentation due to frequent pool
             * allocation and deallocation.
             */
           if ((pool->avail_elements == OHCI_MAX_TDS_PER_POOL) &&
               (pool_count!= NU_NULL))
           {
                /* No TDs of this pool are in use, so release it. */
                NU_Deallocate_Memory(pool->start_ptr);

                memset(pool, 0, sizeof(OHCI_ALIGNED_MEM_POOL));
           }

           status = NU_SUCCESS;

           break;
        }
    }

    USBH_OHCI_UNPROTECT;

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_cc_2_usb_status
*
* DESCRIPTION
*
*       Translates the OHCI TD's condition code that describes the
*       retirement reason of the TD in to generic USB status code of IRP.
*
* INPUTS
*
*       cc                                  Condition code nibble of the
*                                           TD.
*
* OUTPUTS
*
*       UINT32                              IRP status code corresponding
*                                           to TD's condition code.
*
**************************************************************************/
INT32 ohci_cc_2_usb_status(UINT8 cc)
{
    switch (cc)
    {
        case USBH_OHCI_CC_NO_ERROR:
            /* No errors detected. */
            return NU_SUCCESS;

        case USBH_OHCI_CC_CRC:
            /* CRC error */
            return NU_USB_CRC_ERR;

        case USBH_OHCI_CC_BITSTUFF:
            /* Bit stuffing error. */
            return NU_USB_BITSTUFF_ERR;

        case USBH_OHCI_CC_DATA_TOGGLE:
            /* Data toggle mismatch. */
            return NU_USB_TOGGLE_ERR;

        case USBH_OHCI_CC_STALL:
            /* Endpoint stall. */
            return NU_USB_STALL_ERR;

        case USBH_OHCI_CC_NO_RESPONSE:
            /* Device not responding. */
            return NU_USB_NO_RESPONSE;

        case USBH_OHCI_CC_PID_CHECK:
            /* Check bits on PID failed. */
            return NU_USB_INVLD_PID;

        case USBH_OHCI_CC_UNEXPECTED_PID:
            /* Unexpected PID. */
            return NU_USB_UNEXPECTED_PID;

        case USBH_OHCI_CC_DATA_OVERRUN:
            /*Packet exceeded MPS. */
            return NU_USB_DATA_OVERRUN;

        case USBH_OHCI_CC_DATA_UNDERRUN:
            /* Packet less than MPS. */
            return NU_USB_DATA_UNDERRUN;

        case USBH_OHCI_CC_BFR_OVERRUN:
            return NU_USB_BFR_OVERRUN;
            /* Host bus couldn't keep up. */

        case USBH_OHCI_CC_BFR_UNDERRUN:
            /* Host bus couldn't keep up. */
            return NU_USB_BFR_UNDERRUN;

        case USBH_OHCI_CC_NOT_ACCESSED:
            /* Set by software. */
            return NU_USB_NOT_ACCESSED;

        case USBH_OHCI_CC_ED_HALTED:
            /* Endpoint halted, may be due to failure of handshake. */
            return NU_USB_EP_HALTED;

        case USBH_OHCI_TD_CANCELLED:
            /* TD is terminated, may be due to closure of the pipe. */
            return NU_USB_IRP_CANCELLED;

        default:
            return NU_USB_UNKNOWN_ERR;
    }
}

/**************************************************************************
*
* FUNCTION
*
*       normalize_interval
*
* DESCRIPTION
*
*       Normalizes the EP's interval in to one of the  1,2,4,8,16, or 32
*       that it is closest to.
*
* INPUTS
*
*       interval                            Interval in micro seconds.
*
* OUTPUTS
*
*       UINT8                               Interval that's a 2**n.
*
**************************************************************************/
UINT8 ohci_normalize_interval(UINT32 interval)
{
    UINT8   i;

    for (i = 2; i <= USBH_OHCI_INT_SCHEDULE_CNT; i <<= 1)
    {
        if (i > interval)
        {
            break;
        }
    }

    return (i >> 1);
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_hash_add_ed
*
* DESCRIPTION
*
*       It adds the ED in to the hash table. ed_info->key is used as the
*       key to the hashing function. If more than a single ed_info is
*       mapped to the same hashed entry, such colliding entries are in to
*       the linked list maintained for each hash entry.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                             Identifies the ED that has to
*                                           be inserted.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the ED has been
*                                           successfully added to the ED
*                                           hash table.
*       NU_NO_MEMORY                        Indicates failure of memory
*                                           allocation.
*
**************************************************************************/
STATUS ohci_hash_add_ed(NU_USBH_OHCI *ohci, USBH_OHCI_ED_INFO *ed_info)
{
    STATUS          status;
    UINT32          index;
    USBH_OHCI_HASH *newnode;

    USBH_OHCI_PROTECT;

    /* Get index from the key by applying hash function. */
    index = USBH_OHCI_ED_HASH_FUNC(ed_info->key);

    NU_ASSERT(index < USBH_OHCI_HASH_SIZE);

    /* Allocate memory for new entry for hash table. */
    status = NU_Allocate_Memory(ohci->cacheable_pool,
                                (VOID **) &newnode,
                                sizeof(USBH_OHCI_HASH),
                                NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        USBH_OHCI_UNPROTECT;

        return status;
    }

    /* Fill-n-place it in the list of the hash entry. */
    newnode->hcca_addr = ed_info->hwEd;
    newnode->descriptor = ed_info;
    newnode->key = ed_info->key;
    newnode->list.cs_next = NU_NULL;
    newnode->list.cs_previous = NU_NULL;

    NU_Place_On_List((CS_NODE **) &(ohci->ed_hash_table[index]),
                     (CS_NODE *) newnode);

    USBH_OHCI_UNPROTECT;

    return NU_SUCCESS;
}

/**************************************************************************
*
* FUNCTION
*
*        ohci_hash_delete_ed
*
* DESCRIPTION
*
*        It deletes the ED from the hash table. ed_info->key is used as the
*        key to the hashing function.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                             Identifies the ED that has to
*                                           be deleted.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the ED has been
*                                           successfully deleted from the
*                                           ED hash table.
*       NU_NOT_PRESENT                      Indicates that the ED couldn't
*                                           be located in the hash table.
*
**************************************************************************/
STATUS ohci_hash_delete_ed(NU_USBH_OHCI *ohci, USBH_OHCI_ED_INFO *ed_info)
{
    UINT32          index;
    USBH_OHCI_HASH *curEd;

    USBH_OHCI_PROTECT;

    /* Find the index in to the hash table. */
    index = USBH_OHCI_ED_HASH_FUNC(ed_info->key);

    NU_ASSERT(index < USBH_OHCI_HASH_SIZE);

    curEd = ohci->ed_hash_table[index];

    while (curEd != NU_NULL)
    {
        if (curEd->hcca_addr == ed_info->hwEd)
        {
            /* Check if the rest of the fields are as expected. */
            if (curEd->descriptor != ed_info)
            {
                NU_ASSERT(0);
            }
            if (curEd->key != ed_info->key)
            {
                NU_ASSERT(0);
            }

            NU_Remove_From_List((CS_NODE **) &(ohci->ed_hash_table[index]),
                                (CS_NODE *)
                                curEd);

            NU_Deallocate_Memory(curEd);

            USBH_OHCI_UNPROTECT;

            return NU_SUCCESS;
        }

        curEd = (USBH_OHCI_HASH *) curEd->list.cs_next;

        if (curEd == ohci->ed_hash_table[index])
        {
            curEd = NU_NULL;
        }
    }

    USBH_OHCI_UNPROTECT;

    return NU_NOT_PRESENT;
}

/**************************************************************************
*
* FUNCTION
*
*      ohci_hash_find_ed
*
* DESCRIPTION
*
*       It searches the ED hash table based on the hash key.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       key                                 Key that has to be used by the
*                                           hashing function.
*
* OUTPUTS
*
*       USBH_OHCI_ED_INFO*                  Ptr to the ED information node
*                                           that has been found using the
*                                           key, or NU_NULL, if the key
*                                           couldn't be successfully
*                                           located in the hash table.
*
**************************************************************************/
USBH_OHCI_ED_INFO * ohci_hash_find_ed(NU_USBH_OHCI *ohci, UINT32 key)
{
    USBH_OHCI_HASH *node;
    UINT32 index;
    USBH_OHCI_ED_INFO *ed_info = NU_NULL;


    USBH_OHCI_PROTECT;

    index = USBH_OHCI_ED_HASH_FUNC(key);

    NU_ASSERT(index < USBH_OHCI_HASH_SIZE);

    /* Find the index in to the hash table */
    node = ohci->ed_hash_table[index];

    while (node)
    {
        if (node->key == key)
        {
            ed_info =  (USBH_OHCI_ED_INFO *) (node->descriptor);
            break;
        }

        node = (USBH_OHCI_HASH *) node->list.cs_next;

        if (node == ohci->ed_hash_table[index])
        {
            NU_ASSERT(0);

            node = NU_NULL;
        }
    }

    USBH_OHCI_UNPROTECT;

    return (ed_info);
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_dealloc_all_tds
*
* DESCRIPTION
*
*       This function will browse through all the TDs placed for an ED and
*       deallocate them. Except the dummy TD.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                            Identifies the ED whose TDs
                                            have to be terminated.
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/
VOID ohci_dealloc_all_tds(NU_USBH_OHCI *ohci,
                     USBH_OHCI_ED_INFO *ed_info)
{
    USBH_OHCI_TD          *tdHead;
    USBH_OHCI_TD          *tdTail;
    USBH_OHCI_TD          *tdFree;
    UINT32 tdCount = NU_NULL;


    OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdHead, tdHead);

    /* Clear C and H bit from TD Queue Head pointer. */
    tdHead = (USBH_OHCI_TD *)((UINT32)tdHead & 0xFFFFFFFC);

    OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdTail, tdTail);

    NU_ASSERT((~((UINT32)tdTail & 0xF)));

    while((tdHead != tdTail) && (tdCount < OHCI_MAX_TDS_PER_IRP) && (tdHead != NU_NULL))
    {
        /* TD are always 16 bytes aligned. */
        NU_ASSERT((~((UINT32)tdHead & 0xF)));

        if ( (tdHead->non_cache_ptr != NU_NULL) &&
             (tdHead->data          != tdHead->non_cache_ptr) )
        {
            ohci_transfer_done(ohci,
                             tdHead->data,
                             tdHead->non_cache_ptr,
                             tdHead->non_cache_ptr,
                             tdHead->length,
                             NU_FALSE);
        }

        /* Save current TD to de-allocate it. */
        tdFree = tdHead;

        /* Jump to the next TD in the loop. */
        OHCI_READ_ADDRESS(ohci, &tdHead->nextTd, tdHead);

        /* Deallocate the hwTD. */
        ohci_dealloc_gen_td(ohci, tdFree);

        /* Keep a check on TD's processed so far to prevent from looping
         * around forever.
         */
        tdCount++;
    }

    /* Write Head TD in Ed equal to Tail TD. */
    OHCI_WRITE_ADDRESS(ohci, &ed_info->hwEd->tdHead, tdTail);

}

/**************************************************************************
*
* FUNCTION
*
*       ohci_dealloc_all_iso_tds
*
* DESCRIPTION
*
*       This function will browse through all the TDs placed for an ED and
*       deallocate any buffers allocated for them.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                            Identifies the ED whose TDs
                                            have to be terminated.
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/
VOID ohci_dealloc_all_iso_tds(NU_USBH_OHCI *ohci,
                     USBH_OHCI_ISO_ED_INFO *iso_ed_info)
{
    USBH_OHCI_TD_ISO          *tdHead;
    USBH_OHCI_TD_ISO          *tdTail;
    UINT32 tdCount = NU_NULL;


    OHCI_READ_ADDRESS(ohci,
                      &((USBH_OHCI_ED_INFO *) iso_ed_info)->hwEd->tdHead,
                      tdHead);

    /* Clear C and H bit from TD Queue Head pointer. */
    tdHead = (USBH_OHCI_TD_ISO *)((UINT32)tdHead & 0xFFFFFFFC);

    OHCI_READ_ADDRESS(ohci,
                      &((USBH_OHCI_ED_INFO *) iso_ed_info)->hwEd->tdTail,
                      tdTail);

    NU_ASSERT((~((UINT32)tdTail & 0xF)));

    while((tdHead != tdTail) && (tdCount < OHCI_MAX_ISO_TDS_PER_IRP))
    {
        /* TD are always 16 bytes aligned. */
        NU_ASSERT((~((UINT32)tdHead & 0xF)));

#if(USBH_OHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)

        if ( (tdHead->non_cache_ptr != NU_NULL) &&
             (tdHead->data          != tdHead->non_cache_ptr) )
        {
            ohci_transfer_done(ohci,
                             tdHead->data,
                             tdHead->non_cache_ptr,
                             tdHead->non_cache_ptr,
                             tdHead->length,
                             NU_FALSE);

            tdHead->non_cache_ptr = NU_NULL;
        }
#endif

        if(tdHead != NU_NULL)
        {
            /* Jump to the next TD in the loop. */
            OHCI_READ_ADDRESS(ohci, &tdHead->nextTd, tdHead);
        }

        /* Keep a check on TD's processed so far to prevent from looping
         * around forever.
         */
        tdCount++;
    }

    /* Write Head TD in Ed equal to Tail TD. */
    OHCI_WRITE_ADDRESS(ohci,
                       &((USBH_OHCI_ED_INFO *) iso_ed_info)->hwEd->tdHead,
                       tdTail);

    iso_ed_info->free_tds = OHCI_MAX_ISO_TDS_PER_IRP - 1;

}

/**************************************************************************
*
* FUNCTION
*
*       ohci_dealloc_all_irp
*
* DESCRIPTION
*
*       This function will browse through all the TDs placed for an ED and
*       deallocate them. Except the dummy TD.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                            Identifies the ED whose TDs
                                            have to be terminated.
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/
VOID ohci_dealloc_all_irp(NU_USBH_OHCI *ohci,
                     USBH_OHCI_ED_INFO *ed_info,
                     STATUS irp_status)
{
    NU_USB_IRP *pirp;
    NU_USB_PIPE *pipe;
    NU_USB_IRP_CALLBACK callback;

    if(ed_info->irp_info.irp != NU_NULL)
    {
        pirp = ed_info->irp_info.irp;

        /* Get pipe. */
        NU_USB_IRP_Get_Pipe(pirp, &pipe);

        /* Get the IRP's call back function. */
        NU_USB_IRP_Get_Callback (pirp, &callback);

        /* Set IRP's status. */
        NU_USB_IRP_Set_Status(pirp, irp_status);

        /* Call the callback function. */
        if(callback)
        {
            callback(pipe, pirp);
        }

        /* If the  ED is on global pending list, remove it. */
        if(ed_info->on_global_pending == NU_TRUE)
        {
            ed_info->on_global_pending = NU_FALSE;

            OHCI_REMOVE_FROM_LIST(ohci->global_pend, ed_info);
        }
        ed_info->irp_info.irp = NU_NULL;
    }

    /* See if there are any other IRP pending for this endpoint. */
    while (ed_info->pend_irp != NU_NULL)
    {
        /* Retire all the pending IRP's with the same response. */

        /* Pending IRP. */
        pirp = ed_info->pend_irp;

        /* Remove pending IRP form the list. */
        OHCI_REMOVE_FROM_LIST(ed_info->pend_irp, pirp);

        /* Get pipe. */
        NU_USB_IRP_Get_Pipe(pirp, &pipe);

        /* Get the IRP's call back function. */
        NU_USB_IRP_Get_Callback (pirp, &callback);

        /* Set IRP's status. */
        NU_USB_IRP_Set_Status(pirp, irp_status);

        /* Set actual length. */
        NU_USB_IRP_Set_Actual_Length(pirp, NU_NULL);

        /* Call the callback function. */
        if(callback)
        {
            callback(pipe, pirp);
        }
    }
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_dealloc_all_iso_irp
*
* DESCRIPTION
*
*       This function will browse through all the TDs placed for an ED and
*       deallocate them. Except the dummy TD.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                            Identifies the ED whose TDs
                                            have to be terminated.
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/
VOID ohci_dealloc_all_iso_irp(NU_USBH_OHCI *ohci,
                     USBH_OHCI_ISO_ED_INFO *iso_ed_info,
                     STATUS irp_status)
{
    NU_USB_IRP *irp;
    NU_USB_PIPE *pipe;
    NU_USB_IRP_CALLBACK callback;

    /* Keep retiring all the submitted IRPs. */
    while(iso_ed_info->done_irp_index != iso_ed_info->incoming_irp_index)
    {
        irp = iso_ed_info->irp_info_array[iso_ed_info->done_irp_index].irp;

        if(irp == NU_NULL)
        {
            NU_ASSERT(0);

            OHCI_MOD_INC(iso_ed_info->done_irp_index,
                         USBH_OHCI_MAX_PEND_IRPS);

            continue;
        }

        /* Update the status of IRP. */
        NU_USB_IRP_Set_Status(irp, irp_status);

        /* Call the callback function. */
        NU_USB_IRP_Get_Callback(irp, &callback);
        NU_USB_IRP_Get_Pipe(irp, &pipe);

        if (callback)
        {
            callback(pipe, irp);
        }

        /* Jump to the next IRP to be retired. */
        OHCI_MOD_INC(iso_ed_info->done_irp_index, USBH_OHCI_MAX_PEND_IRPS);

    }

    /* Reinitializing the ED indexes. */
    iso_ed_info->no_more_irp = NU_FALSE;
    iso_ed_info->last_irp = 0;
    iso_ed_info->done_irp_index = 0;
    iso_ed_info->sch_irp_index = 0;
    iso_ed_info->incoming_irp_index = 0;
    iso_ed_info->trans_start_flag = 0;
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_hndl_retired_td
*
* DESCRIPTION
*
*       Handles TDs that have been retired. If all the TDs of an IRP are
*       retired then its callback is invoked by this function.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *td                                 Ptr to the TD that got retired.
*       flag                                Indicates if the TD is retired
*                                           by HW or abnormally terminated
*                                           by S/W due to pipe close or
*                                           flush. (USBH_OHCI_TD_RETIRED or
*                                           USBH_OHCI_TD_CANCELED)
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/
VOID ohci_hndl_retired_td(NU_USBH_OHCI *ohci, USBH_OHCI_TD *hwtd)
{
    BOOLEAN                accept_short_pkts;
    UINT32                 read_buffer_end;
    UINT32                 read_current_buffer;
    UINT32                 dummy_td;
    UINT32                 actual_length;
    UINT32                 cntrl_info;
    UINT32                 cc;
    UINT32                 toggle = NU_NULL;
    UINT32                 xfr_len = NU_NULL;
    USBH_OHCI_TD          *tdHead;
    USBH_OHCI_TD_ISO *iso_hwtd;
    USBH_OHCI_ED_INFO     *ed_info = NU_NULL;
    USBH_OHCI_IRP_INFO    *irp_info = NU_NULL;
    NU_USB_IRP_CALLBACK    callback;
    NU_USB_IRP            *irp = NU_NULL;
    NU_USB_PIPE           *pipe;
    BOOLEAN                 sch_global = NU_FALSE;
    UINT32 controlinfo;
    BOOLEAN                sync = NU_FALSE;
    UINT16 psw;

    OHCI_HW_READ16(ohci, &hwtd->psw, psw);

    /* For ISO, call ISO retire handler routine. */
    if ((psw & USBH_OHCI_CC_NON_ISO_MASK) != USBH_OHCI_CC_NON_ISO)
    {
        /* Translate TD to tdinfo pointer that stores it s/w info. */
        iso_hwtd = (USBH_OHCI_TD_ISO *) hwtd;

        NU_ASSERT(iso_hwtd->ed_info->type == USB_EP_ISO);

        ohci_hndl_retired_iso_td(ohci, iso_hwtd);
    }
    else
    {
        /* Get TD's IRP, ED. */
        ed_info = hwtd->ed_info;

        if(ed_info)
        {
            NU_ASSERT(ed_info->type != USB_EP_ISO);

            irp_info = &ed_info->irp_info;

            if(irp_info)
            {
                irp = irp_info->irp;
            }
        }

        if((ed_info == NU_NULL)  ||
           (irp_info == NU_NULL) ||
           (irp == NU_NULL))
        {
            /* Deallocate any hwenv data buffers. */
            if( (hwtd->non_cache_ptr    != NU_NULL) &&
                (hwtd->data             != hwtd->non_cache_ptr) )
            {
                ohci_transfer_done(ohci,
                                 hwtd->data,
                                 hwtd->non_cache_ptr,
                                 hwtd->non_cache_ptr,
                                 hwtd->length,
                                 NU_FALSE);
            }

            /* Deallocate the hwTD. */
            ohci_dealloc_gen_td(ohci, hwtd);

            return;
        }

        /* Read TD Control Info (1st Word). */
        OHCI_HW_READ32(ohci, &(hwtd->controlInfo), cntrl_info);

        /* Condition Code. TD retirement status. */
        cc = (cntrl_info >> 28) & 0xf;

        if((cc == USBH_OHCI_CC_NO_ERROR) ||
           (cc == USBH_OHCI_CC_DATA_UNDERRUN))
        {
            /* Read buffer end, for this TD. */
            OHCI_READ_ADDRESS(ohci, &(hwtd->be), read_buffer_end);

            if ( ((cntrl_info & USBH_OHCI_TD_CTL_PID_MASK) !=
                                                USBH_OHCI_TD_CTL_PID_SETUP)
                 &&
                 (read_buffer_end != NU_NULL)
               )

            {
                NU_USB_IRP_Get_Actual_Length(irp, &actual_length);

                OHCI_READ_ADDRESS(ohci, &(hwtd->cbp), read_current_buffer);

                /* A zero value of CBP (read_current_buffer), shows zero
                 * length data packet, or all the bytes have been
                 * transferred.
                 */
                if (read_current_buffer)
                {
                    /* For less data transferred, as specified in TD, then
                     * get the length by taking difference of BE to buffer
                     * start and subtract it from the what data transfer
                     * expected.
                     */
                    xfr_len = (hwtd->length -
                              (read_buffer_end + 1 - read_current_buffer));
                }
                else
                {
                    xfr_len = (hwtd->length);
                }

                actual_length += xfr_len;

                /* Update Actual length of the transaction. */
                NU_USB_IRP_Set_Actual_Length(irp, actual_length);

                if ((hwtd->non_cache_ptr) &&
                    (hwtd->ed_info->bEndpointAddress & USB_DIR_IN))
                {
                    sync = NU_TRUE;
                }
            }
        }

        /* De-allocate the non-cache buffer associated with this TD.
         * Also copy the contents to buffer allocated by application.
         */
        if( (hwtd->non_cache_ptr    != NU_NULL) &&
            (hwtd->data             != hwtd->non_cache_ptr) )
        {
            ohci_transfer_done(ohci,
                             hwtd->data,
                             hwtd->non_cache_ptr,
                             hwtd->non_cache_ptr,
                             xfr_len,
                             sync);
        }

        hwtd->non_cache_ptr = NU_NULL;

        /* Update the number of TDs services so far. */
        irp_info->done_tds++;

        /* Read HeadP field of the corresponding ED. */
        OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdHead, tdHead);

        /* Save the control info for later use. */
        OHCI_HW_READ32(ohci, &hwtd->controlInfo, controlinfo);

        /* Deallocate the hwTD. */
        ohci_dealloc_gen_td(ohci, hwtd);

        /* If TD is in error, ED is halted under normal condition,
         * terminate all the pending TDs of the IRP.
         */
        if ((cc != USBH_OHCI_CC_NO_ERROR) &&
            ((UINT32)tdHead & USBH_OHCI_ED_PTR_HALTED))
        {
            UINT32 ed_ctrlinfo;

            /* Skip this endpoint unless another IRP is scheduled on it. */
            OHCI_HW_READ32(ohci, &ed_info->hwEd->controlinfo, ed_ctrlinfo);

            ed_ctrlinfo |= USBH_OHCI_EDCTL_SKIP;

            OHCI_HW_WRITE32(ohci, &ed_info->hwEd->controlinfo, ed_ctrlinfo);

            /* Treat under run as an error, if the IRP doesn't like short
             * packets.
             */
            if (cc == USBH_OHCI_CC_DATA_UNDERRUN)
            {
                /* Read the toggle carry bit and save it. It is only saved
                 * for data-under run case. In case of a halt, it is rest
                 * to 0.
                 */
                OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdHead, toggle);
                toggle &= USBH_OHCI_ED_PTR_TGL_CARRY;

                NU_USB_IRP_Get_Accept_Short_Packets(irp,
                                                    &accept_short_pkts);

                if (accept_short_pkts)
                {
                    cc = USBH_OHCI_CC_NO_ERROR;
                }
            }

            /* Delete all the TDs associated with this ED. */
            ohci_dealloc_all_tds(ohci, ed_info);

            irp_info->done_tds = irp_info->req_tds;

            /* Tail pointer in ED always points to the dummy TD. Not
             * converting the address as it will be written back in ED.
             */
            OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdTail, dummy_td);

            /* Update the saved toggle information. */
            dummy_td |= toggle;

            /* Clear the Halt and update Head pointer equal to Tail
             * pointer.
             */
            OHCI_WRITE_ADDRESS(ohci, &ed_info->hwEd->tdHead, dummy_td);
        }

        /* Submit more TDs (if needed) only if, the endpoint did not
         * face an Halt.
         */
        else
        {
            /* Check if more TDs are required for this IRP. */
            if ((irp_info->sch_tds < irp_info->req_tds)

                /* If an interrupt was scheduled for this TD, schedule More
                 * TDs now.
                 */
                && ((controlinfo & USBH_OHCI_TICTL_DI_MASK) !=
                     (USBH_OHCI_TICTL_DI_NONE << USBH_OHCI_TICTL_DI_ROT)) )
            {
                ohci_handle_irp(ohci, ed_info, irp, NU_FALSE);
            }
            else
            {
                /* If there are no pending IRP for this ED, Try to schedule
                 * from global pending list.
                 */
                sch_global = NU_TRUE;
            }
        }

        /* All the TD for the current IRP finished or short packet
         * received.
         */
        if (irp_info->done_tds == irp_info->req_tds)
        {
            /* All TDs of this IRP have been processed, invoke the IRP
             * callback.
             */
            NU_USB_IRP_Set_Status(irp, ohci_cc_2_usb_status(cc));
            NU_USB_IRP_Get_Callback(irp, &callback);
            NU_USB_IRP_Get_Pipe(irp, &pipe);

            if (callback)
            {
                callback(pipe, (NU_USB_IRP *) irp);
            }

            irp_info->irp = NU_NULL;

            /* Any transfer error seen on the ED. */
            if (cc != USBH_OHCI_CC_NO_ERROR)
            {
                /* Remove all the pending IRPs. */
                ohci_dealloc_all_irp(ohci,
                                     ed_info,
                                     ohci_cc_2_usb_status(cc));
            }
            else
            {
                STATUS  status;

                /* Handle pending IRPs if any on this ED. */
                if(ed_info->pend_irp != NU_NULL)
                {
                    irp = ed_info->pend_irp;

                    /* Remove pending IRP form the list. */
                    OHCI_REMOVE_FROM_LIST(ed_info->pend_irp, irp);

                    status = ohci_handle_irp(ohci, ed_info, irp, NU_TRUE);

                    if (status != NU_SUCCESS)
                    {
                        ohci_dealloc_all_tds(ohci, ed_info);

                        /* Invoke the callback with the failure reason. */
                        NU_USB_IRP_Get_Callback(irp, &callback);
                        NU_USB_IRP_Get_Pipe(irp, &pipe);
                        NU_USB_IRP_Set_Status(irp, status);

                        if(callback)
                        {
                            callback(pipe, (NU_USB_IRP *) irp);
                        }
                    }
                    else
                    {
                        /* Another IRP for this ED has been scheduled. */
                        sch_global = NU_FALSE;
                    }
                }/* (ed_info->pend_irp != NU_NULL) */
            }/*  (cc != USBH_OHCI_CC_NO_ERROR) */
        }/* (irp_info->done_tds == irp_info->req_tds) */

        if(sch_global == NU_TRUE)
        {
            /* Check if there are any EDs on the pending list. */
            if(ohci->global_pend != NU_NULL)
            {
                STATUS status;

                ed_info = ohci->global_pend;

                ed_info->on_global_pending = NU_FALSE;

                /* No need to protect as this function is only called from
                 * ISR.
                 */
                OHCI_REMOVE_FROM_LIST(ohci->global_pend, ed_info);

                NU_ASSERT(ed_info->irp_info.irp != NU_NULL);

                /* Try to schedule the ED. If the schedule fails due to
                 * un-availability of the memory, ED will be placed on
                 * global pending list by the following call.
                 */
                status = ohci_handle_irp(ohci,
                                         ed_info,
                                         ed_info->irp_info.irp,
                                         NU_FALSE);

                if(status != NU_SUCCESS)
                {
                    NU_ASSERT(0);

                    ohci_dealloc_all_tds(ohci, ed_info);

                    ohci_dealloc_all_irp(ohci, ed_info, status);
                }
            }
        }
    }/* Non ISO ED. */
}

/**************************************************************************
* FUNCTION
*   ohci_hndl_retired_iso_td
*
* DESCRIPTION
*   Handles TDs that have been retired. If all the TDs of an IRP are
*   retired then its callback is invoked by this function.
*
* INPUTS
*   ohci  Handle that identifies the HC in the OHCI HC database.
*   td    Pointer to the TD that got retired.
*   flag  Indicates if the TD is retired by HW or abnormally
*         terminated by S/W due to pipe close or flush.
*         (USBH_OHCI_TD_RETIRED or USBH_OHCI_TD_CANCELED)
*
* OUTPUTS
*   VOID.
*
*************************************************************************/
VOID ohci_hndl_retired_iso_td(NU_USBH_OHCI *ohci,
                              USBH_OHCI_TD_ISO *iso_hwTd)
{
    BOOLEAN                 ohci_error = NU_FALSE;
    UINT8                 **pbuffer_array;
    UINT8                   iso_trans_per_td;
    UINT8                   ep_type;
    UINT16                  td_len;
    UINT16                 *actual_lengths, *lengths;
    UINT32                  tdHead;
    UINT16                  psw;
    UINT32                  cntrl_info = 0;
    UINT32                  cc = 0, i, count;

#if(USBH_OHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)
    BOOLEAN                 sync;
#endif

    USBH_OHCI_TD_ISO *next_iso_hwTD;
    USBH_OHCI_ISO_IRP_INFO *sch_irp_info = NU_NULL;
    USBH_OHCI_ISO_IRP_INFO *done_irp_info = NU_NULL;
    NU_USB_ISO_IRP         *irp = NU_NULL;
    NU_USB_IRP             *base_irp;
    USBH_OHCI_ED_INFO      *ed_info = NU_NULL;
    USBH_OHCI_ISO_ED_INFO  *iso_ed_info = NU_NULL;
    NU_USB_IRP_CALLBACK     callback;
    NU_USB_PIPE            *pipe;

    /* Get TD's IRP, ED. */
    ed_info = iso_hwTd->ed_info;

    if(ed_info)
    {
        NU_ASSERT(ed_info->type == USB_EP_ISO);

        /* Get ISO ED INFO structure corresponding to ED Structure. */
        iso_ed_info = (USBH_OHCI_ISO_ED_INFO *) ed_info;

        /* Get TD PRIV (IRP CB) at done_irp_index. */
        done_irp_info =
                 &iso_ed_info->irp_info_array[iso_ed_info->done_irp_index];

        if(done_irp_info)
        {
            irp = (NU_USB_ISO_IRP *) done_irp_info->irp;
        }
    }

    if((iso_ed_info   == NU_NULL) ||
       (done_irp_info == NU_NULL) ||
       (irp           == NU_NULL))
    {
        NU_ASSERT(0);

#if(USBH_OHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)

        /* De-allocate the non-cache buffer associated with this TD. Also
         * copy the contents to buffer allocated by application.
         */
        if ( (iso_hwTd->non_cache_ptr   != NU_NULL) &&
             (iso_hwTd->data            != iso_hwTd->non_cache_ptr) )
        {
            ohci_transfer_done(ohci,
                             iso_hwTd->data,
                             iso_hwTd->non_cache_ptr,
                             iso_hwTd->non_cache_ptr,
                             iso_hwTd->length,
                             NU_FALSE);

            iso_hwTd->non_cache_ptr = NU_NULL;
        }

#endif /* USBH_OHCI_NORMALIZE_ISO_BUFFER */

        if(iso_ed_info != NU_NULL)
        {
             iso_ed_info->free_tds++;
        }

        return;
    }

    ep_type = (iso_hwTd->ed_info->bEndpointAddress & 0x80);

    /* Read TD Control Info (1st Word). */
    OHCI_HW_READ32(ohci, &(iso_hwTd->controlinfo), cntrl_info);

    /* Condition Code. TD retirement status. */
    cc = (cntrl_info >> 28) & 0xf;

    NU_USB_ISO_IRP_Get_Actual_Lengths(irp, &actual_lengths);

    NU_USB_ISO_IRP_Get_Lengths(irp, &lengths);

    /* TD has been accessed by OHCI Controller. Update the actual lengths
     * array.
     */
    for (count = 0; count <= iso_hwTd->iso_trans_per_td ; count++)
    {
        OHCI_HW_READ16(ohci, &(iso_hwTd->psw[count]), psw);

        if ((psw & 0x0000E000) != 0x0000E000)
        {
            if (ep_type == USB_DIR_OUT)
            {
                /* For OUT Transactions, actual length is the length
                  * defined in lengths array.
                  */
                actual_lengths[done_irp_info->done_trans + count] =
                                lengths[done_irp_info->done_trans + count];
            }
            else
            {
                /* For IN Transactions, actual length is read from the
                  * TD.
                  */
                actual_lengths[done_irp_info->done_trans + count] =
                                                          psw & 0x00000fff;
            }
        }
        else
        {
            /* If OHCI Hardware has not done this transaction yet
             * set actual length to zero.
             */
            actual_lengths[done_irp_info->done_trans + count] = 0;
        }

    }

#if(USBH_OHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)

    /* De-allocate the non-cache buffer associated with this TD. Also copy
     * the contents to buffer allocated by application.
     */
    if (iso_hwTd->non_cache_ptr)
    {
        if ((iso_hwTd->ed_info->bEndpointAddress & 0x80) == USB_DIR_OUT)
        {
            sync = NU_FALSE;
        }
        else
        {
            sync = NU_TRUE;
        }

        if ( (iso_hwTd->non_cache_ptr   != NU_NULL) &&
             (iso_hwTd->data            != iso_hwTd->non_cache_ptr) )
        {
            ohci_transfer_done(ohci,
                             iso_hwTd->data,
                             iso_hwTd->non_cache_ptr,
                             iso_hwTd->non_cache_ptr,
                             iso_hwTd->length,
                             sync);
        }

        iso_hwTd->non_cache_ptr = NU_NULL;
    }

#endif

    /* Read HeadP field of the corresponding ED. */
    OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdHead, tdHead);

    /* Update the number of TDs serviced so far. */
    done_irp_info->done_trans += iso_hwTd->iso_trans_per_td + 1;

    /* done_tds represents total transactions done for this IRP. */
    NU_USB_ISO_IRP_Set_Actual_Num_Transactions (irp,
                                                done_irp_info->done_trans);

    /* Increase the global free TD count. */
    iso_ed_info->free_tds++;

    /* If TD is in error, ED is halted under normal condition , terminate
     * all the pending TDs of the IRP.
     */
    if ((tdHead & 1) ||
          (cc == USBH_OHCI_CC_STALL) ||
          (cc == USBH_OHCI_CC_BFR_OVERRUN))
    {

        /* Delete all the TDs associated with this ED. */
        for (i = 0; i < OHCI_MAX_ISO_TDS_PER_IRP; i++)
        {
            /* HW TD that is to be removed.*/
            next_iso_hwTD = &iso_ed_info->td_array_start[i];

#if(USBH_OHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)

            if ( (next_iso_hwTD->non_cache_ptr  != NU_NULL) &&
                 (next_iso_hwTD->data           != next_iso_hwTD->non_cache_ptr) )
            {
                ohci_transfer_done(ohci,
                                 next_iso_hwTD->data,
                                 next_iso_hwTD->non_cache_ptr,
                                 next_iso_hwTD->non_cache_ptr,
                                 next_iso_hwTD->length,
                                 NU_FALSE);

                next_iso_hwTD->non_cache_ptr = NU_NULL;
            }
#endif

        }

        /* Set count to total number of transactions for this IRP, so that
         * callback function of this IRP is called later in this function.
         */
        done_irp_info->done_trans = done_irp_info->req_trans;

        ohci_error = NU_TRUE;

        /* Update ED TailP also as 1st TD address */
        OHCI_WRITE_ADDRESS(ohci, &ed_info->hwEd->tdTail,
                                   (UINT32) iso_ed_info->td_array_start);

        /* Clear the halt in the hardware and write 1st TD address, so that
         * no further transaction is done, as far as user submit a new IRP.
         */
        OHCI_WRITE_ADDRESS(ohci, &ed_info->hwEd->tdHead,
                                   (UINT32) iso_ed_info->td_array_start);

    }

    /* Submit more TDs (if needed) only if, the endpoint did not
     * face an Halt and there are more.
     */
    if (((tdHead & 1) == 0) && (ohci_error == NU_FALSE))
    {
        /* Load IRP, transactions of which will be submitted. */
        sch_irp_info =
                  &iso_ed_info->irp_info_array[iso_ed_info->sch_irp_index];

        /* Another IRP has been submitted, from the time no_more_irp was
         * marked last time.
         */
        if ((iso_ed_info->sch_irp_index != iso_ed_info->incoming_irp_index)
            && (iso_ed_info->no_more_irp == NU_TRUE))
        {
            iso_ed_info->no_more_irp = NU_FALSE;
        }

        /* submit_next_td = 1, force this function to occupy only one more
         * empty slot of TD if available. This keeps load on this function
         * called from ISR moderate.
         */
        while ((iso_ed_info->free_tds != 0) &&
               (iso_ed_info->no_more_irp == NU_FALSE))
        {
            /* If all the TDs associated with this IRP has been submitted.
             */
            if (sch_irp_info->sch_trans == sch_irp_info->req_trans)
            {
                /* Increment the index. */
                OHCI_MOD_INC(iso_ed_info->sch_irp_index,
                             USBH_OHCI_MAX_PEND_IRPS);

                /* If next irp index reaches incoming irp index. */
                if (iso_ed_info->sch_irp_index ==
                           iso_ed_info->incoming_irp_index)
                {
                    iso_ed_info->no_more_irp = NU_TRUE;

                    /* Break the submission loop. */
                    break;
                }

                NU_ASSERT(iso_ed_info->sch_irp_index <
                                                  USBH_OHCI_MAX_PEND_IRPS);
                NU_ASSERT(iso_ed_info->incoming_irp_index <
                                                  USBH_OHCI_MAX_PEND_IRPS);
                NU_ASSERT(iso_ed_info->done_irp_index <
                                                  USBH_OHCI_MAX_PEND_IRPS);

                /* Get irp_info at the next irp index. */
                sch_irp_info =
                  &iso_ed_info->irp_info_array[iso_ed_info->sch_irp_index];
            }

            /* If there are more TDs to submit. */
            if (iso_ed_info->no_more_irp == NU_FALSE)
            {
                NU_ASSERT(sch_irp_info);

                irp = (NU_USB_ISO_IRP *) sch_irp_info->irp;

                NU_ASSERT(irp);

                /* Get buffer length associated with this IRP. */
                NU_USB_ISO_IRP_Get_Lengths(irp, &lengths);

                /* Get buffer pointer associated with this IRP. */
                NU_USB_ISO_IRP_Get_Buffer_Array(irp, &pbuffer_array);


                /* Find maximum number of transactions that can be
                 * accommodated in one TD.
                 */
                iso_trans_per_td = ohci_check_iso_trans_per_td(ohci,
                                   sch_irp_info,
                                   (NU_USB_ISO_IRP *) irp,
                                   &td_len);

                /* Set TD Control word with Delay Interrupt of 6 and
                 * Frame count as per calculated.
                 */
                cntrl_info = 0x00000000;
                cntrl_info |= OHCI_TD_DI_SET(0);
                cntrl_info |= OHCI_TD_FC_SET(iso_trans_per_td);

                /* Get HwTD at the next free location. */
                next_iso_hwTD =
                  &iso_ed_info->td_array_start[iso_ed_info->next_td_index];

                /* Fill the space with new TD of current IRP. */
                ohci_fill_iso_td(ohci,
                             cntrl_info,
                             pbuffer_array[sch_irp_info->sch_trans],
                             td_len,
                             ed_info,
                             iso_ed_info->next_td_index,
                             next_iso_hwTD);

                /* next_td_index can't be more than
                 * OHCI_MAX_ISO_TDS_PER_IRP.
                 */
                OHCI_MOD_INC(iso_ed_info->next_td_index,
                             OHCI_MAX_ISO_TDS_PER_IRP);

                /* TD is submitted, decrement the global free TD count and
                 * increment the TD count associated with this irp.
                 */
                iso_ed_info->free_tds--;

                /* Increment the index associated with IRP the number of
                 * transactions submitted in this TD.
                 */
                sch_irp_info->sch_trans += iso_trans_per_td + 1;
            }
            break;
        }
    }

    /* Again load current IRP. */
    done_irp_info =
                 &iso_ed_info->irp_info_array[iso_ed_info->done_irp_index];

    /* Invoke Callback, if all the TDs of this IRP has been processed. */
    if (done_irp_info->done_trans == done_irp_info->req_trans)
    {
        base_irp = done_irp_info->irp;

        NU_ASSERT(base_irp);

        /* Update the status of IRP. */
        NU_USB_IRP_Set_Status(base_irp, ohci_cc_2_usb_status(cc));

        /* Call the callback function. */
        NU_USB_IRP_Get_Callback(base_irp, &callback);
        NU_USB_IRP_Get_Pipe(base_irp, &pipe);

        if (callback)
        {
            callback(pipe, base_irp);
        }

         /* If there is some error, retire all the pending IRPs also, with
         * the same error code.
         */
        if (ohci_error)
        {
            ohci_dealloc_all_iso_irp(ohci,
                                     iso_ed_info,
                                     ohci_cc_2_usb_status(cc));
        }
        else
        {
            /* Otherwise increment the current index. */
            OHCI_MOD_INC(iso_ed_info->done_irp_index,
                         USBH_OHCI_MAX_PEND_IRPS);
        }
    }
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_unlink_tds
*
* DESCRIPTION
*
*       Walks the list of TDs pending in the ED's TD list and forcefully
*       retires them. This is used while closing  or while flushing the
*       pipe.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                             Identifies the ED whose TDs
                                            have to be terminated.
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/
VOID ohci_unlink_tds(NU_USBH_OHCI *ohci,
                     USBH_OHCI_ED_INFO *ed_info,
                     BOOLEAN remove_dummy_td)
{

    UINT32              td_head_addr;
    UINT32              td_tail_addr;
    USBH_OHCI_TD       *cur_td;
    USBH_OHCI_TD       *cur_iso_td;
    UINT32 ed_ctrlinfo;

    /* Set sKip bit in ED. */
    OHCI_HW_READ32(ohci, &ed_info->hwEd->controlinfo, ed_ctrlinfo);

    ed_ctrlinfo |= USBH_OHCI_EDCTL_SKIP;

    OHCI_HW_WRITE32(ohci, &ed_info->hwEd->controlinfo, ed_ctrlinfo);

    /* Wait for the SOF. */
    usb_wait_ms(1);

    if (ed_info->type != USB_EP_ISO)
    {
        /* Make sure interrupt form some other ED does not get this ED
         * scheduled if it is on global pending list.
         */
        USBH_OHCI_PROTECT

        /* Remove this ED from global pending list. */
        if(ed_info->on_global_pending == NU_TRUE)
        {
            OHCI_REMOVE_FROM_LIST(ohci->global_pend, ed_info);
        }

        USBH_OHCI_UNPROTECT

        /* As sKip bit is set, this ED will not get any further
         * interrupts.
         */

        /* Get the address of Head TD. */
        OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdHead, td_head_addr);

        /* Clear C and H bits. */
        cur_td = (USBH_OHCI_TD *) ((td_head_addr) & 0xfffffff0);

        /* Get the address of Tail TD. */
        OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdTail, td_tail_addr);

        /* If Head TD and Tail TD address are different, then there are TD
         * present for this ED.
         */
        if((UINT32)cur_td != td_tail_addr)
        {
            ohci_dealloc_all_tds(ohci, ed_info);
        }

        /* Remove all the Transfers pending on this ED. */
        ohci_dealloc_all_irp(ohci, ed_info, NU_USB_IRP_CANCELLED);

        if (remove_dummy_td == NU_TRUE)
        {
            ohci_dealloc_gen_td(ohci, (USBH_OHCI_TD *) td_tail_addr);

            td_tail_addr = NU_NULL;

            /* Clear tdTail address in ED. */
            OHCI_WRITE_ADDRESS(ohci, &ed_info->hwEd->tdTail, td_tail_addr);
        }

        /* Update/clear TD Head address in ED. */
        OHCI_WRITE_ADDRESS(ohci, &ed_info->hwEd->tdHead, td_tail_addr);

    }

    /* Incase of ISO there is no dummy TD. */
    else
    {
        USBH_OHCI_ISO_ED_INFO  *iso_ed_info =
                                         (USBH_OHCI_ISO_ED_INFO *) ed_info;

        USBH_OHCI_PROTECT;

        if(iso_ed_info->done_irp_index != iso_ed_info->incoming_irp_index)
        {
            OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdHead, td_head_addr);

            /* Clear C and H bits. */
            cur_iso_td = (USBH_OHCI_TD *) ((td_head_addr) & 0xfffffff0);

            OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdTail, td_tail_addr);

            /* If Head TD and Tail TD address are different, then there are
             * TD present for this ED.
             */
            if((UINT32)cur_iso_td != td_tail_addr)
            {
                ohci_dealloc_all_iso_tds(ohci, iso_ed_info);
            }

            /* Remove all the IRP submitted on this pipe. */
            ohci_dealloc_all_iso_irp(ohci,
                             iso_ed_info,
                             NU_USB_IRP_CANCELLED);
        }

        USBH_OHCI_UNPROTECT;
    }
}


/**************************************************************************
*
* FUNCTION
*
*       ohci_scheduleControlPipe
*
* DESCRIPTION
*
*       Places Control ED in the HC's schedule list.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                             Identifies the ED that has to
*                                           be scheduled.
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/
VOID ohci_scheduleControlPipe(NU_USBH_OHCI *ohci,
                              USBH_OHCI_ED_INFO *ed_info)
{
    UINT32  read_reg;

    /* Clear next ED pointer. */
    OHCI_WRITE_ADDRESS(ohci, &ed_info->hwEd->nextEd, NU_NULL);

    USBH_OHCI_PROTECT;

    if (ohci->last_cntrl == NU_NULL)
    {
        /* This will be the first ED on the control list. */
        OHCI_WRITE_ADDRESS(ohci,
                           (ohci->cb.controller.base_address +
                            USBH_OHCI_HC_CONTROL_HEAD_ED),
                           (ed_info->hwEd));

        OHCI_HW_WRITE32(ohci,
                        (ohci->cb.controller.base_address +
                         USBH_OHCI_HC_CONTROL_CUR_ED),
                        0);
    }
    else
    {
        OHCI_WRITE_ADDRESS(ohci,
                           &ohci->last_cntrl->hwEd->nextEd,
                           ed_info->hwEd);
    }

    ohci->last_cntrl = ed_info;

    /* Ensure that Control list is enabled. */
    OHCI_HW_READ32(ohci,
                       (ohci->cb.controller.base_address +
                        USBH_OHCI_HC_CONTROL),
                       read_reg);

    OHCI_HW_WRITE32(ohci,
                        (ohci->cb.controller.base_address +
                         USBH_OHCI_HC_CONTROL),
                        read_reg | USBH_OHCI_CTL_CLE);

    USBH_OHCI_UNPROTECT;
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_scheduleBulkPipe
*
* DESCRIPTION
*
*       Places Bulk ED in the HC's schedule list.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                             Identifies the ED that has to
*                                           be scheduled.
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/
VOID ohci_scheduleBulkPipe(NU_USBH_OHCI *ohci, USBH_OHCI_ED_INFO *ed_info)
{
    UINT32  read_reg;

    /* Clear next ED pointer. */
    OHCI_WRITE_ADDRESS(ohci, &ed_info->hwEd->nextEd, NU_NULL);

    USBH_OHCI_PROTECT;

    if (ohci->last_bulk == NU_NULL)
    {

        /* This will be the first ED on the bulk list. */
        OHCI_WRITE_ADDRESS(ohci,
                           (ohci->cb.controller.base_address +
                            USBH_OHCI_HC_BULK_HEAD_ED),
                           (ed_info->hwEd));

        OHCI_HW_WRITE32(ohci,
                        (ohci->cb.controller.base_address +
                         USBH_OHCI_HC_BULK_CUR_ED),
                        0);

    }
    else
    {
        /* Append this ED to the end of the bulk list. */
        OHCI_WRITE_ADDRESS(ohci,
                           &ohci->last_bulk->hwEd->nextEd,
                           ed_info->hwEd);
    }

    ohci->last_bulk = ed_info;

    /* Ensure that bulk list is enabled. */
    OHCI_HW_READ32(ohci,
                       (ohci->cb.controller.base_address +
                        USBH_OHCI_HC_CONTROL),
                       read_reg);

    OHCI_HW_WRITE32(ohci,
                        (ohci->cb.controller.base_address +
                         USBH_OHCI_HC_CONTROL),
                        read_reg | USBH_OHCI_CTL_BLE);


    USBH_OHCI_UNPROTECT;
}

/**************************************************************************
*
* FUNCTION
*
*        ohci_schedulePeriodicPipe
*
* DESCRIPTION
*
*        Places ISO/Interrupt ED in the HC's schedule list. They are sorted
*        on the interval, with higher the value of the interval, its ED is
*        more close to the head.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                             Identifies the ED that has to
*                                           be scheduled.
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/
VOID ohci_schedulePeriodicPipe(NU_USBH_OHCI *ohci,
                               USBH_OHCI_ED_INFO *ed_info)
{
    UINT32              key;
    UINT8               i;
    UINT32              read_reg;
    UINT32              addr;
    USBH_OHCI_ED_INFO  *cur_ed_info;
    USBH_OHCI_ED       *cur, **prev, *hwEd;

    /* Get hwED from ed_info control block. */
    hwEd = (USBH_OHCI_ED *) ((ed_info->hwEd));

    USBH_OHCI_PROTECT;

    for (i = ed_info->index2perdTbl;
         i < USBH_OHCI_INT_SCHEDULE_CNT;
         i += ed_info->interval)
    {
        /* Get address of HccaInterruptTable one by one. */
        prev = (USBH_OHCI_ED **) &ohci->hcca->periodicEdTable[i];

        /* Read contents of ED from the address. */
        OHCI_READ_ADDRESS(ohci, prev, addr);
        cur = (USBH_OHCI_ED *) addr;

        /* Keep going on, up to ED exist in link list for one interrupt
         * slot.Ref fig. 5-5. Also if there is no existing ED at this slot,
         * skip searching and add ED at this point.
         */
        while (cur && cur != hwEd)
        {
            /* Read 1st DWORD of ED. */
            OHCI_HW_READ32(ohci, &cur->controlinfo, key);
            key &= 0xfff;

            /* Find ED Info block based on key i.e. FA , EN and D. */
            cur_ed_info = ohci_hash_find_ed(ohci, key);

            if(cur_ed_info == NU_NULL)
            {
                NU_ASSERT(0);

                USBH_OHCI_UNPROTECT;

                return;
            }

            /* If interval of the new ED is more than interval of existing
             * ED, then update the ed here.
             */
            if (ed_info->interval > cur_ed_info->interval)
            {
                break;
            }

            /* Get next ED. */
            prev = (USBH_OHCI_ED **) &cur->nextEd;
            OHCI_READ_ADDRESS(ohci, prev, addr);
            cur = (USBH_OHCI_ED *) addr;
        }
        if (cur != hwEd)
        {
            /* Update the current schedule by adding ED. */
            hwEd->nextEd = (UINT32) *prev;
            OHCI_WRITE_ADDRESS(ohci, prev, hwEd);
        }
        /* Update the load info. */
        ohci->load[i] += ed_info->load;
    }


    OHCI_HW_READ32(ohci,
                       (ohci->cb.controller.base_address +
                        USBH_OHCI_HC_CONTROL),
                       read_reg);

    if (ed_info->type == USB_EP_ISO)
    {
        /* Enable ISO List Processing. */
        read_reg |= USBH_OHCI_CTL_IE;
    }

    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_CONTROL),
                    read_reg | USBH_OHCI_CTL_PLE);

    USBH_OHCI_UNPROTECT;
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_find_best_schedule
*
* DESCRIPTION
*
*       Finds a best schedule among the 32 schedules of OHCI HC, based on
*       the scheduling frequency requirements of the periodic ED.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                             Identifies the periodic ED for
*                                           which best schedule is to be
*                                           found.
*
* OUTPUTS
*
*       UINT8                               Returns best schedule in the
*                                           range 0 to
*                                           USBH_OHCI_INT_SCHEDULE_CNT-1 if
*                                           found, or
*                                           USBH_OHCI_INT_SCHEDULE_CNT
*                                           if the ED cannot be
*                                           accommodated.
*
**************************************************************************/
UINT8 ohci_find_best_schedule(NU_USBH_OHCI *ohci,
                              USBH_OHCI_ED_INFO *ed_info)
{
    UINT8   branch = USBH_OHCI_INT_SCHEDULE_CNT;
    UINT8   i, j;

    /* Find the schedule that would be of lowest load with ed_info->load.*/
    for (i = 0; i < ed_info->interval; i++)
    {
        if ((branch == USBH_OHCI_INT_SCHEDULE_CNT) ||
            (ohci->load[branch] > ohci->load[i]))
        {
            for (j = i; j < USBH_OHCI_INT_SCHEDULE_CNT;
                 j += ed_info->interval)
            {
                if ((ohci->load[j] + ed_info->load) > USB1_BANDWIDTH)
                {
                    break;
                }
            }
            if (j >= USBH_OHCI_INT_SCHEDULE_CNT)
            {
                branch = i;
            }
        }
    }
    return branch;
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_hndl_td_done_q
*
* DESCRIPTION
*
*       Handles write back to done Q interrupt. Rearranges the done Q in
*       the chronological order with the oldest retired TD at the head and
*       then processes each of these TDs.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/

VOID ohci_hndl_td_done_q(NU_USBH_OHCI *ohci, USBH_OHCI_TD *head)
{
    UINT32        addr;
    USBH_OHCI_TD *cur, *td, *prev;

    /* Reverses the done Q. */

    /* Read address of TD Head. */
    OHCI_READ_ADDRESS(ohci, &head, addr);

    /* current TD is always at the head of done_q. */
    cur = (USBH_OHCI_TD *) ((UINT32) (addr) & 0xfffffff0);

    prev = NU_NULL;

    /* Reverse the order, so that the oldest TD should be processed 1st.
     * Ref. Sec. 5.2.9.
     */
    while (cur != NU_NULL)
    {
        /* Read address of next TD from the current TD. */
        OHCI_READ_ADDRESS(ohci, &cur->nextTd, addr);

        /* Update nextTD field of current TD to the next TD pointed. */
        cur->nextTd = (UINT32) prev;

        prev = cur;

        cur = (USBH_OHCI_TD *) (addr & 0xfffffff0);
    }
    /* Chronologically oldest is now at the head. */
    cur = prev;

    /* Start processing retired TDs, one by one. 'cur' TD is at the top and
     * is the oldest one.
     */
    while (cur != NU_NULL)
    {
        td = cur;

        /* Get address of next TD from the current TD. */
        cur = (USBH_OHCI_TD *) ((UINT32) (cur->nextTd) & 0xfffffff0);

        /* Handle the retire TD. */
        ohci_hndl_retired_td(ohci, td);

    }
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_close_periodic_ed
*
* DESCRIPTION
*
*       Removes periodic ED from HC's schedule list.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                            Identifies the periodic ED that
*                                           has to be removed.
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/
VOID ohci_close_periodic_ed(NU_USBH_OHCI *ohci, USBH_OHCI_ED_INFO *ed_info)
{
    UINT8                   i;
    UINT32                  addr;
    UINT32                  read_reg;
    USBH_OHCI_ED           *prevHwEd, *curHwEd;
    UINT32 ed_ctrlinfo;

    /* Set sKip bit in ED. */
    OHCI_HW_READ32(ohci, &ed_info->hwEd->controlinfo, ed_ctrlinfo);

    ed_ctrlinfo |= USBH_OHCI_EDCTL_SKIP;

    OHCI_HW_WRITE32(ohci, &ed_info->hwEd->controlinfo, ed_ctrlinfo);

    OHCI_HW_READ32(ohci,
                   (ohci->cb.controller.base_address +
                    USBH_OHCI_HC_CONTROL),
                   read_reg);

    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_CONTROL),
                    read_reg & ~(USBH_OHCI_CTL_PLE));

    /* Wait for HC to finish accessing any TD. */
    usb_wait_ms(1);

    USBH_OHCI_PROTECT;

    /* Search and remove the ED from the table. */
    for (i = ed_info->index2perdTbl;
         i < USBH_OHCI_INT_SCHEDULE_CNT;
         i += ed_info->interval)
    {
        ohci->load[i] -= ed_info->load;

        OHCI_READ_ADDRESS(ohci, &ohci->hcca->periodicEdTable[i], addr);

        curHwEd = (USBH_OHCI_ED *) addr;

        /* If it is the head of the schedule list. */
        if (curHwEd == ed_info->hwEd && curHwEd != NU_NULL)
        {
            ohci->hcca->periodicEdTable[i] = (USBH_OHCI_ED *)
                                             curHwEd->nextEd;
            continue;
        }

        /* Search the rest of the list. */
        prevHwEd = NU_NULL;
        while (curHwEd && curHwEd != ed_info->hwEd)
        {
            prevHwEd = curHwEd;

            OHCI_READ_ADDRESS(ohci, &curHwEd->nextEd, addr);

            curHwEd = (USBH_OHCI_ED *) addr;
        }
        if ((prevHwEd) && (curHwEd == ed_info->hwEd) && (curHwEd != NU_NULL))
        {
            /* Remove curHwEd from the list. */
            prevHwEd->nextEd = curHwEd->nextEd;
        }
    }

    OHCI_HW_READ32(ohci,
                   (ohci->cb.controller.base_address +
                    USBH_OHCI_HC_CONTROL),
                   read_reg);

    OHCI_HW_WRITE32(ohci,
                (ohci->cb.controller.base_address +
                 USBH_OHCI_HC_CONTROL),
                read_reg | USBH_OHCI_CTL_PLE);

     USBH_OHCI_UNPROTECT;

    /* Terminate the pending TDs. */
    ohci_unlink_tds(ohci, ed_info, NU_TRUE);

    /* Remove the ED from the hash table. */
    ohci_hash_delete_ed(ohci, ed_info);

    /* Delete the h/w ED. */
    ohci_dealloc_ed(ohci, ed_info->hwEd);

    if (ed_info->type == USB_EP_ISO)
    {
        /* Remove all the TD associated with this ED. */
        NU_Deallocate_Memory(
                      ((USBH_OHCI_ISO_ED_INFO *) ed_info)->td_array_start);

    }

    /* Delete the s/w ED. */
    NU_Deallocate_Memory(ed_info);

}

/**************************************************************************
*
* FUNCTION
*
*       ohci_close_non_periodic_ed
*
* DESCRIPTION
*
*       Removes Bulk/Control ED from HC's schedule list.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                             Identifies the non-periodic ED
*                                           that has to be removed.
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/
VOID ohci_close_non_periodic_ed(NU_USBH_OHCI *ohci,
                                USBH_OHCI_ED_INFO *ed_info)
{
    UINT32             *head_ed_reg, *cur_ed_reg;
    UINT32              addr_ed;
    UINT32              read_reg;
    UINT32              control_info;
    UINT32              addr;
    UINT32              mask;
    UINT32              list_filled;
    USBH_OHCI_ED       *prevHwEd, *curHwEd, *headHwEd;
    USBH_OHCI_ED_INFO **list_tail;
    UINT32 ed_ctrlinfo;

    /* Set sKip bit in ED. */
    OHCI_HW_READ32(ohci, &ed_info->hwEd->controlinfo, ed_ctrlinfo);

    ed_ctrlinfo |= USBH_OHCI_EDCTL_SKIP;

    OHCI_HW_WRITE32(ohci, &ed_info->hwEd->controlinfo, ed_ctrlinfo);

    if (ed_info->type == USB_EP_CTRL)
    {
        mask = USBH_OHCI_CTL_CLE;
    }
    else
    {
        mask = USBH_OHCI_CTL_BLE;
    }

    /* Disable list processing. */
    OHCI_HW_READ32(ohci,
                   (ohci->cb.controller.base_address +
                    USBH_OHCI_HC_CONTROL),
                   read_reg);

    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_CONTROL),
                    read_reg & ~mask);

    usb_wait_ms(1);

    USBH_OHCI_PROTECT;

    /* As list processing is now stopped, reading ED head register is safe.
     */
    if (ed_info->type == USB_EP_CTRL)
    {
        list_filled = USBH_OHCI_CS_CLF;

        cur_ed_reg = (UINT32 *)
                     (ohci->cb.controller.base_address +
                      USBH_OHCI_HC_CONTROL_CUR_ED);

        head_ed_reg = (UINT32 *)
                      (ohci->cb.controller.base_address +
                       USBH_OHCI_HC_CONTROL_HEAD_ED);

        list_tail = &(ohci->last_cntrl);
    }
    else
    {
        list_filled = USBH_OHCI_CS_BLF;

        cur_ed_reg = (UINT32 *)
                     (ohci->cb.controller.base_address +
                      USBH_OHCI_HC_BULK_CUR_ED);

        head_ed_reg = (UINT32 *)
                      (ohci->cb.controller.base_address +
                       USBH_OHCI_HC_BULK_HEAD_ED);

        list_tail = &(ohci->last_bulk);
    }

    /* Traverse the list and drop the ed from the ed list. */
    OHCI_READ_ADDRESS(ohci, head_ed_reg, addr_ed);

    headHwEd = (USBH_OHCI_ED *) addr_ed;

    if (ed_info->hwEd == headHwEd)
    {
        /* Head ED is the one to be removed. */

        if(headHwEd != NU_NULL)
        {
            OHCI_READ_ADDRESS(ohci, &headHwEd->nextEd, addr_ed);
        }

        OHCI_WRITE_ADDRESS(ohci, head_ed_reg, addr_ed);

        if ((*list_tail == ed_info) && (headHwEd != NU_NULL))
        {
            if (headHwEd->nextEd)
            {
                OHCI_READ_ADDRESS(ohci, &headHwEd->nextEd, addr_ed);

                OHCI_HW_READ32(ohci,
                               &(((USBH_OHCI_ED *) addr_ed)->controlinfo),
                               control_info);

                *list_tail = ohci_hash_find_ed(ohci, control_info & 0xfff);
            }
            else
            {
                *list_tail = NU_NULL;
            }
        }
    }
    else
    {
         /* Search the list. */
        curHwEd = headHwEd;
        prevHwEd = curHwEd;

        while ((curHwEd != NU_NULL) && (curHwEd != ed_info->hwEd))
        {
            prevHwEd = curHwEd;
            OHCI_READ_ADDRESS(ohci, &curHwEd->nextEd, addr);
            curHwEd = (USBH_OHCI_ED *) addr;
        }

        if ((curHwEd == ed_info->hwEd) && (curHwEd != NU_NULL))
        {
            if (*list_tail == ed_info)
            {
                OHCI_HW_READ32(ohci, &prevHwEd->controlinfo, control_info);
                *list_tail = ohci_hash_find_ed(ohci, control_info & 0xfff);
            }
            prevHwEd->nextEd = curHwEd->nextEd;
        }
        else
        {
            NU_ASSERT(0);
        }
    }

    OHCI_HW_WRITE32(ohci, cur_ed_reg, 0);

    if (*list_tail != NU_NULL)
    {
        OHCI_HW_WRITE32(ohci,
                        (ohci->cb.controller.base_address +
                         USBH_OHCI_HC_COMMAND_STATUS),
                         list_filled);

        /* Enable list processing. */
        OHCI_HW_READ32(ohci,
               (ohci->cb.controller.base_address +
                USBH_OHCI_HC_CONTROL),
               read_reg);

          OHCI_HW_WRITE32(ohci,
                (ohci->cb.controller.base_address +
                 USBH_OHCI_HC_CONTROL),
                read_reg | mask);
    }

    /* Terminate the pending TDs. */
    ohci_unlink_tds(ohci, ed_info, NU_TRUE);

    /* Remove the ED from the hash table. */
    ohci_hash_delete_ed(ohci, ed_info);

    ed_info->load = 0xDEADBEAF;

    /* Delete the hardware ED. */
    ohci_dealloc_ed(ohci, ed_info->hwEd);

    /* Delete the ed_info structure. */
    NU_Deallocate_Memory(ed_info);

    USBH_OHCI_UNPROTECT;

}

/**************************************************************************
*
* FUNCTION
*
*       ohci_fill_td
*
* DESCRIPTION
*
*       Fills a pre-allocated h/w TD and inserts it in the ED's TD list.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       info                                TD's control info, the first
*                                           dword.
*       *data                               Data ptr where the data to be
*                                           sent is stored / data to be
*                                           received is stored.
*       len                                 Data length.
*       irp                                 Ptr to the IRP for which the
*                                           TD is to be created.
*       index                               TD index of the IRP.
*       hwTd                                Pointer to h/w TD.
*
* OUTPUTS
*
*       VOID.
*
**************************************************************************/
VOID ohci_fill_td(NU_USBH_OHCI *ohci,
                    UINT32 info,
                    UINT8 *irp_buf,
                    UINT8 *noncache_buf,
                    UINT16 len,
                    USBH_OHCI_IRP_INFO *irp_info,
                    USBH_OHCI_ED_INFO *ed_info,
                    USBH_OHCI_TD *hwTd)
{
    UINT32                 addr;
    USBH_OHCI_TD *td_to_fill;


    /* Get address of TD Queue Tail Pointer, from relative ED. */
    OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdTail, addr);

    /* Update info in TD_INFO Control Block. */
    td_to_fill = (VOID *) addr;

    if(td_to_fill != NU_NULL)
    {
        /* Set Data pointers and lengths pointers. */
        td_to_fill->data = irp_buf;
        td_to_fill->length = len;
        td_to_fill->ed_info = ed_info;
        td_to_fill->non_cache_ptr = noncache_buf;
        OHCI_HW_WRITE16(ohci, &td_to_fill->psw, USBH_OHCI_CC_NON_ISO);

        /* Update 1st byte of TD. */
        OHCI_HW_WRITE32(ohci,
                            &(td_to_fill->controlInfo),
                            info);

        /* Fill up General TD. */
        OHCI_WRITE_ADDRESS(ohci,
                               &(td_to_fill->cbp),
                               td_to_fill->non_cache_ptr);

        if (td_to_fill->non_cache_ptr)
        {
            OHCI_WRITE_ADDRESS(ohci,
                                   &(td_to_fill->be),
                                   (td_to_fill->non_cache_ptr + len - 1));
        }
        else
        {
            td_to_fill->be = NU_NULL;
        }

        /* Reset the given TD. */
        hwTd->controlInfo = NU_NULL;
        hwTd->cbp = NU_NULL;
        hwTd->nextTd = NU_NULL;
        hwTd->be = NU_NULL;

        OHCI_WRITE_ADDRESS(ohci,
                               &(td_to_fill->nextTd),
                               hwTd);

        /* Set the new tail TD. */
        OHCI_WRITE_ADDRESS(ohci, &ed_info->hwEd->tdTail, hwTd);
    }
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_fill_iso_td
*
* DESCRIPTION
*
*       Fills a pre-allocated h/w TD and inserts it in the ED's TD list.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       info                                TD's control info, the first
*                                           dword.
*       *data                               Data ptr where the data to be
*                                           sent is stored / data to be
*                                           received is stored.
*       len                                 Data length.
*       irp                                 Ptr to the IRP for which the
*                                           TD is to be created.
*       index                               TD index of the IRP.
*       hwTd                                Pointer to h/w TD.
*
* OUTPUTS
*
*       UINT32                              In case of ISO it returns no.
*                                           of ISO buffers in the ISO TD
*                                           that have been filled. In other
*                                           cases it returns 0.
*
**************************************************************************/

#define OHCI_ISO_FRAME_MARGIN 4

UINT32 ohci_fill_iso_td(NU_USBH_OHCI *ohci,
                    UINT32 info,
                    UINT8 *data,
                    UINT32 len,
                    USBH_OHCI_ED_INFO *ed,
                    UINT32 index,
                    USBH_OHCI_TD_ISO *iso_hwTd)
{
    BOOLEAN                sync;
    UINT32                 cur_frame_num;
    UINT32                 tra_frame_num;
    UINT32                 data_pointer;
    UINT32                 base_addr;
    UINT16                 offset;
    UINT16                *buf_lengths;
    UINT8                  next_index , last_index;
    UINT8                  last_iso_trans;


    USBH_OHCI_ISO_ED_INFO *iso_ed_info = NU_NULL;
    NU_USB_ISO_IRP        *iso_irp     = NU_NULL;
    USBH_OHCI_ISO_IRP_INFO     *irp_info     = NU_NULL;
    UINT32 next_hwTd;

#if(USBH_OHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)
    UINT32                  *raw_data;
    BOOLEAN                 buff_type;
#endif


    /* For ISO, get TD from the global TD index count. */
    iso_ed_info = (USBH_OHCI_ISO_ED_INFO *) ed;

    /* Get next index from the global TD index count. */
    next_index = index + 1;

    if (next_index == OHCI_MAX_ISO_TDS_PER_IRP)
    {
        next_index = 0;
    }
    next_hwTd = (UINT32)(&iso_ed_info->td_array_start[next_index]);

    /* Read Transactions per TD from the value passed to this function
     * from calling function.
     */
    iso_hwTd->iso_trans_per_td = (info >> USBH_OHCI_TICTL_FC_ROT) & 0x07;

    /* Get IRP related to this TD. */
    irp_info = &iso_ed_info->irp_info_array[iso_ed_info->sch_irp_index];
    iso_irp = (NU_USB_ISO_IRP *) irp_info->irp;

    /* Set Data pointers and lengths pointers. */
    iso_hwTd->data = data;
    iso_hwTd->length = len;

    /* To keep data as it is in Normalize Buffer. */
    if ((iso_hwTd->ed_info->bEndpointAddress & 0x80) == USB_DIR_OUT)
    {
        sync = NU_TRUE;
    }
    /* To make data little Endian in Normalize Buffer. */
    else
    {
        sync = NU_FALSE;
    }

#if(USBH_OHCI_NORMALIZE_ISO_BUFFER == NU_TRUE)
    /* If buffer type is cachable then allocate a non-cachable
     * buffer for data transfer.
     */
    NU_USB_IRP_Get_Buffer_Type_Cachable((NU_USB_IRP*) iso_irp, &buff_type);
    if ( buff_type )
    {
        ohci_normalize_buffer(ohci,
                              iso_hwTd->data,
                              iso_hwTd->length,
                              4,
                              sync,
                              &iso_hwTd->non_cache_ptr,
                              &raw_data);
    }
    else
    {
        iso_hwTd->non_cache_ptr = iso_hwTd->data;
    }
#else
    sync =  sync;
    iso_hwTd->non_cache_ptr = iso_hwTd->data;
#endif

    /* Fill ISO TD. For ISO TD, update only 1st PSW field. */
    /* Determine buffer page 0. */
    data_pointer  = (UINT32) iso_hwTd->non_cache_ptr;
    data_pointer &= 0xFFFFF000;
    base_addr = data_pointer;

    /* Fill up ISO TD, by writing buffer page 0. */
    OHCI_WRITE_ADDRESS(ohci,
                       &(iso_hwTd->bp0),
                       data_pointer);

    /* Get array of data to be transmitted. */
    NU_USB_ISO_IRP_Get_Lengths(iso_irp, &buf_lengths);

/*-----------------------------------------------------------------------*/

    /* Get starting data pointer. */
    data_pointer = (UINT32) iso_hwTd->non_cache_ptr;

    /* Determine Offset for 1st Transaction. */
    offset = data_pointer & 0x00000FFF;
    offset |= 0x0000E000;

    /* Update data pointer. */
    data_pointer  += buf_lengths[irp_info->sch_trans];

    /* Write Offset for 1st transaction. */
    OHCI_HW_WRITE16(ohci,
                    &(iso_hwTd->psw[0]),
                    offset);

/*-----------------------------------------------------------------------*/

    if (iso_hwTd->iso_trans_per_td > 0)
    {
        /* Determine Offset for 2nd Transaction. */
        offset = (data_pointer & 0x00000FFF);
        offset |= 0x0000E000;

        /* Check whether data pointer is in 1st page or not. */
        if ((data_pointer & 0xFFFFF000) != base_addr)
        {
            offset |= 0x00001000;
        }

        /* Update data pointer. */
        data_pointer  += buf_lengths[irp_info->sch_trans + 1];
    }

    /* Write Offset for 2nd transaction. */
    OHCI_HW_WRITE16(ohci,
                    &(iso_hwTd->psw[1]),
                    offset);

/*-----------------------------------------------------------------------*/

    if (iso_hwTd->iso_trans_per_td > 1)
    {
        /* Determine Offset for 3rd Transaction. */
        offset = data_pointer & 0x00000FFF;
        offset |= 0x0000E000;

        /* Check whether data pointer is in 1st page or not. */
        if ((data_pointer & 0xFFFFF000) != base_addr)
        {
            offset |= 0x00001000;
        }

        /* Update data pointer. */
        data_pointer  += buf_lengths[irp_info->sch_trans + 2];

        /* Write Offset for 3rd transaction. */
        OHCI_HW_WRITE16(ohci,
                        &(iso_hwTd->psw[2]),
                        offset);

/*-----------------------------------------------------------------------*/

        if (iso_hwTd->iso_trans_per_td > 2)
        {

            /* Determine Offset for 4th Transaction. */
            offset = (data_pointer  & 0x00000FFF);
            offset |= 0x0000E000;

            /* Check whether data pointer is in 1st page or not. */
            if ((data_pointer & 0xFFFFF000) != base_addr)
            {
                offset |= 0x00001000;
            }

            /* Update data pointer. */
            data_pointer  += buf_lengths[irp_info->sch_trans + 3];
        }

        /* Write Offset for 4th transaction. */
        OHCI_HW_WRITE16(ohci,
                        &(iso_hwTd->psw[3]),
                        offset);
    }

/*-----------------------------------------------------------------------*/

    if (iso_hwTd->iso_trans_per_td > 3)
    {
        /* Determine Offset for 5th Transaction. */
        offset = data_pointer & 0x00000FFF;
        offset |= 0x0000E000;

        /* Check whether data pointer is in 1st page or not. */
        if ((data_pointer & 0xFFFFF000) != base_addr)
        {
            offset |= 0x00001000;
        }

        /* Update data pointer. */
        data_pointer  += buf_lengths[irp_info->sch_trans + 4];

        /* Write Offset for 5th transaction. */
        OHCI_HW_WRITE16(ohci,
                        &(iso_hwTd->psw[4]),
                        offset);

/*-----------------------------------------------------------------------*/

        if (iso_hwTd->iso_trans_per_td > 4)
        {
            /* Determine Offset for 6th Transaction. */
            offset = (data_pointer & 0x00000FFF);
            offset |= 0x0000E000;

            /* Check whether data pointer is in 1st page or not. */
            if ((data_pointer & 0xFFFFF000) != base_addr)
            {
                offset |= 0x00001000;
            }

            /* Update data pointer. */
            data_pointer  += buf_lengths[irp_info->sch_trans + 5];
        }

        /* Write Offset for 6th transaction. */
        OHCI_HW_WRITE16(ohci,
                        &(iso_hwTd->psw[5]),
                        offset);
     }

/*-----------------------------------------------------------------------*/

    if (iso_hwTd->iso_trans_per_td > 5)
    {
        /* Determine Offset for 7th Transaction. */
        offset = data_pointer & 0x00000FFF;
        offset |= 0x0000E000;

        /* Check whether data pointer is in 1st page or not. */
        if ((data_pointer & 0xFFFFF000) != base_addr)
        {
            offset |= 0x00001000;
        }

        /* Update data pointer. */
        data_pointer  += buf_lengths[irp_info->sch_trans + 6];

        /* Write Offset for 7th transaction. */
        OHCI_HW_WRITE16(ohci,
                        &(iso_hwTd->psw[6]),
                        offset);

/*-----------------------------------------------------------------------*/

        if (iso_hwTd->iso_trans_per_td > 6)
        {
            /* Determine Offset for 8th Transaction. */
            offset = (data_pointer & 0x00000FFF);
            offset |= 0x0000E000;

            /* Check whether data pointer is in 1st page or not. */
            if ((data_pointer & 0xFFFFF000) != base_addr)
            {
                offset |= 0x00001000;
            }

            /* Update data pointer. */
            data_pointer += buf_lengths[irp_info->sch_trans + 7];
        }

        /* Write Offset for 7th and 8th transaction. */
        OHCI_HW_WRITE16(ohci,
                        &(iso_hwTd->psw[7]),
                        offset);
    }

/*-----------------------------------------------------------------------*/

    if (iso_hwTd->non_cache_ptr)
    {
        OHCI_WRITE_ADDRESS(ohci,
                           &(iso_hwTd->be),
                           (data_pointer - 1));
    }
    else
    {
        iso_hwTd->be = NU_NULL;
    }

    /* Set nextTD field to pointer of current TD. */
    OHCI_WRITE_ADDRESS(ohci,
                       &(iso_hwTd->nextTd),
                       next_hwTd);

    /* Set the new tail TD */
    OHCI_WRITE_ADDRESS(ohci, &ed->hwEd->tdTail, next_hwTd);

    /* If TD associated with this transaction has already started,
     * just set the frame number with reference to previous TD.
     */
    if (iso_ed_info->trans_start_flag)
    {

        if (index == 0)
        {
            last_index = OHCI_MAX_ISO_TDS_PER_IRP - 1;
        }
        else
        {
            last_index = index - 1;
        }

        /* Get Transaction / TD for last TD, so that we may set the
         * new frame number by incrementing this much number of
         * transactions.
         */
        last_iso_trans =
                  iso_ed_info->td_array_start[last_index].iso_trans_per_td;
        tra_frame_num =
                       iso_ed_info->last_td_frame_num + last_iso_trans + 1;

        /* Read Current Frame Number. */
        OHCI_HW_READ32(ohci,
                       (ohci->cb.controller.base_address +
                        USBH_OHCI_HC_FM_NUMBER),
                       cur_frame_num);

        /* If new frame number is less than or equal to current frame
         * number, this means we are late in submitting the new frame
         * so set new frame number incrementing the current frame
         * number.
         */
        if (tra_frame_num <= cur_frame_num)
        {
            tra_frame_num = cur_frame_num + OHCI_ISO_FRAME_MARGIN;
        }

        /* Save the frame number in control block. */
        iso_ed_info->last_td_frame_num = tra_frame_num;

        info |= tra_frame_num;
    }
    else
    {
        /* Read Current Frame Number. */
        OHCI_HW_READ32(ohci,
                       (ohci->cb.controller.base_address +
                        USBH_OHCI_HC_FM_NUMBER),
                       cur_frame_num);

        /* Update Frame Info. */
        cur_frame_num &= 0x0000ffff;

        /* Start from the 2nd to this frame, as there may be case, when
         * updating this result in hardware, frame has gone. */
        tra_frame_num = cur_frame_num + OHCI_ISO_FRAME_MARGIN;

        info |= (tra_frame_num);

        /* Mark start of transaction. */
        iso_ed_info->trans_start_flag = 1;

        /* Save the frame number in control block. */
        iso_ed_info->last_td_frame_num = tra_frame_num;
    }

    /* Update 1st byte of TD. */
    OHCI_HW_WRITE32(ohci,
                    &(iso_hwTd->controlinfo),
                    info);

    return 0;
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_make_td
*
* DESCRIPTION
*
*       Allocates a TD, fills it and inserts it in the ED's TD list.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       info                                TD's control info, the first
*                                           dword.
*       *data                               Data ptr where the data to be
*                                           sent is stored / data to be
*                                           received is stored.
*       len                                 Data length.
*       *irp                                Ptr to the IRP for which the
*                                           TD is to created.
*       index                               TD index of the IRP.
*       iso_index                           Used only for ISO TDs. It
*                                           identifies the index in the ISO
*                                           Packet.
*
* OUTPUTS
*    Status returned by ohci_alloc_gen_td.
*
**************************************************************************/
STATUS ohci_make_td (NU_USBH_OHCI      *ohci,
                     UINT32             dword0,
                     UINT8             *irp_buf,
                     UINT32             len,
                     USBH_OHCI_IRP_INFO *irp_info,
                     USBH_OHCI_ED_INFO *ed_info,
                     USBH_OHCI_TD **filled_hwTd,
                     USBH_OHCI_TD *previous_hwTd)
{
    STATUS status;
    USBH_OHCI_TD *hwTd;
    BOOLEAN sync;
    BOOLEAN buff_type;
    UINT8  *raw_addr;
    UINT8 *non_cache_ptr;
    BOOLEAN set_intr_on_prev = NU_FALSE;

    /* If IRP requires more TD than USBH_OHCI_MAX_TDS_PER_IRP set interrupt
      * on delay to zero to allow remaining TDs to be scheduled.
      * Otherwise set the interrupt generation on last TD.
      */
    if (((irp_info->req_tds > OHCI_MAX_TDS_PER_IRP) &&
         ((irp_info->sch_tds & (OHCI_DELAY_INTR_NUM_TD -1)) == NU_NULL))

        ||

        /* Set the interrupt on the last TD. */
        (irp_info->sch_tds == (irp_info->req_tds - 1)))
    {
        dword0 |= OHCI_TD_DI_SET (0);
    }
    else
    {
        dword0 |= OHCI_TD_DI_SET (7);
    }

    NU_ASSERT (ed_info->type != USB_EP_ISO);
    NU_ASSERT (irp_info->sch_tds < irp_info->req_tds);

    status =
            ohci_alloc_gen_td (ohci, &hwTd, ed_info, NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* In previous implementation of hardware environments it was not
         * possible to directly allocate aligned memory. That's why
         * raw_addr was used. Now it is not required.
         */
        if(len == NU_NULL)
        {
            non_cache_ptr = NU_NULL;
        }
        else
        {

            /* To keep data as it is in Normalize Buffer. */
            if (((ed_info->bEndpointAddress & 0x80) == USB_DIR_OUT) ||
                (ed_info->type == USB_EP_CTRL))
            {
                sync = NU_TRUE;
            }
            /* To make data little endian in Normalize Buffer. */
            else
            {
                sync = NU_FALSE;
            }

            /* The value returned in len is immaterial, since td->length
             * is either 4k or less and Normalize_Buffer cannot make it
             * less than its unit_size, i.e 4k.
             */
            USBH_OHCI_PROTECT;

            /* If buffer type is cachable then allocate a non-cachable
             * buffer for data transfer.
             */
            NU_USB_IRP_Get_Buffer_Type_Cachable(irp_info->irp, &buff_type);
            if ( buff_type )
            {
                status = ohci_normalize_buffer(ohci,
                                      irp_buf,
                                      len,
                                      4,
                                      sync,
                                      &non_cache_ptr,
                                      &raw_addr);
            }
            else
            {
                non_cache_ptr = irp_buf;
            }

            USBH_OHCI_UNPROTECT;

            if(status != NU_SUCCESS)
            {
                /* If other than NU_NO_MEMORY status is returned. */
                NU_ASSERT(status == NU_NO_MEMORY);

                /* De allocate the TD as memory is not available to copy
                 * the buffers.
                 */
                ohci_dealloc_gen_td(ohci, hwTd);

                set_intr_on_prev = NU_TRUE;
            }
        }

        if(status == NU_SUCCESS)
        {
            UINT32 addr;

            OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdTail, addr);

            ohci_fill_td(ohci, dword0, irp_buf, non_cache_ptr, len,
                                                  irp_info, ed_info, hwTd);

            irp_info->sch_tds++;

            *filled_hwTd = (USBH_OHCI_TD *) addr;
        }
    }

    else
    {
        set_intr_on_prev = NU_TRUE;
    }

    if(set_intr_on_prev == NU_TRUE)
    {
        /* Unable to allocate more TDs. Set Interrupt on the last
         * scheduled TD.
         */
        if(previous_hwTd != NU_NULL)
        {
            UINT32 td_ctrlinfo;

            /* Set the interrupt bit on the last scheduled TD. */
            OHCI_HW_READ32(ohci, &previous_hwTd->controlInfo, td_ctrlinfo);
            td_ctrlinfo &= ~(USBH_OHCI_TICTL_DI_NONE << USBH_OHCI_TICTL_DI_ROT);
            OHCI_HW_WRITE32(ohci, &previous_hwTd->controlInfo, td_ctrlinfo);
        }

        *filled_hwTd = NU_NULL;
    }

    return(status);
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_find_td_count
*
* DESCRIPTION
*
*       Finds the number of TDs required for this IRP.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *irp                                Ptr to the IRP for which the TD
*                                           count has to be found.
*       maxPacket                           Max Packet size supported by
*                                           the pipe of this IRP.
*       *count                              Return pointer that would
*                                           contain the TD count upon
*                                           successful completion of this
*                                           function.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the count has
*                                           been successfully calculated.
*       NU_INVALID_SIZE                     Indicates that the
*                                           irp->usb_irp.length is greater
*                                           than 4k for control transfer.
*
**************************************************************************/
STATUS ohci_find_td_count(NU_USBH_OHCI *ohci,
                          USBH_OHCI_ED_INFO *ed_info,
                          NU_USB_IRP *irp,
                          UINT32 maxPacket,
                          UINT32 *count)
{
    BOOLEAN use_empty_pkt;
    UINT32  tdcount = 0;
    UINT32  length;

    /* Get Length of the Data transfer associated with this transfer. */
    NU_USB_IRP_Get_Length(irp, &length);

    /* Select the TD count based on the type of EP. */
    switch (ed_info->type)
    {
        case USB_EP_CTRL:
            /* Control transfer cannot have more than
             * USBH_OHCI_MAX_DATA_PER_TD data in their data phase.
             */
            if (length > USBH_OHCI_MAX_DATA_PER_TD)
            {
                return (NU_INVALID_SIZE);
            }

            /* 1 for setup, 1 for status phase and may be more. */
            tdcount = 2;

            /* Bulk, Interrupt and also control. */
        default:

            /* USBH_OHCI_MAX_DATA_PER_TD can be filled in a single TD */
            tdcount += (length >> USBH_OHCI_MAX_DATA_PER_TD_SHIFT);

            /* For length not multiple of USBH_OHCI_MAX_DATA_PER_TD,
             * increment it for remaining amount.
             */
            if ((length & USBH_OHCI_MAX_DATA_PER_TD_MASK) != NU_NULL)
            {
                tdcount++;
            }

            NU_USB_IRP_Get_Use_Empty_Pkt(irp, &use_empty_pkt);

            /* If length is less than USBH_OHCI_MAX_DATA_PER_TD bytes. */
            if (tdcount == NU_NULL)
            {
                tdcount++;
            }
            else
            {
                /* May be Zero length packet is needed to mark the transfer
                 * completion.
                 */
                if ((use_empty_pkt) && ((length % maxPacket) == NU_NULL))
                {
                    tdcount++;
                }
            }
            break;
    }

    *count = tdcount;

    return NU_SUCCESS;
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_handle_irp
*
* DESCRIPTION
*
*       Handles translating the IRP in to TD required to initiate the
*       transfer.
*
* INPUTS
*
*       *ohci                               Handle that identifies the HC
*                                           in the OHCI HC database.
*       *ed_info                             Identifies the ED over which
*                                           the IRP is to be transferred.
*       *irp                                Ptr to the IRP that has to be
*                                           transferred.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the count has
*                                           been successfully calculated.
*       NU_INVALID_SIZE               Indicates that the
*                                           irp->usb_irp.length is greater
*                                           than 4k for control transfer.
*       NU_NO_MEMORY                        Indicates that memory
*                                           allocation failed.
*
**************************************************************************/
STATUS ohci_handle_irp(NU_USBH_OHCI *ohci,
                       USBH_OHCI_ED_INFO *ed_info,
                       NU_USB_IRP *irp,
                       BOOLEAN update_ed_info)
{
    STATUS              status = NU_SUCCESS;
    UINT32              tdcount = NU_NULL;
    UINT32              maxPacket = 0x00;
    USBH_OHCI_IRP_INFO  *irp_info = &ed_info->irp_info;
    UINT8              *data;
    UINT32              controlinfo = NU_NULL;
    UINT32              tdHead;
    NU_USB_SETUP_PKT   *setup_data;
    UINT8               direction;
    UINT32              data_len;
    UINT16 previous_sch_tds;
    UINT32 current_td_count;
    BOOLEAN use_empty_packet = NU_FALSE;

    USBH_OHCI_TD *filled_td;
    USBH_OHCI_TD *previous_td = NU_NULL;
    status = NU_USB_IRP_Get_Use_Empty_Pkt (irp, &use_empty_packet);

    /* Read 1st byte, (Control Info) of EP Descriptor from the HCCA.
     * Ref. 4.2.1.
     */
    OHCI_HW_READ32(ohci, &ed_info->hwEd->controlinfo, controlinfo);

    /* Get Maxp of this EP from control Info. Ref. 4.2.1. */
    maxPacket = (controlinfo & USBH_OHCI_EDCTL_MPS_MASK) >> 16;

    if(update_ed_info == NU_TRUE)
    {
       /* Set Actual length if the IRP as zero, as transfer has not been
         * started yet.
         */
        NU_USB_IRP_Set_Actual_Length(irp, NU_NULL);

        /* Get Length of data transfer associated with this IRP. */
        NU_USB_IRP_Get_Length(irp, &data_len);

        irp_info->next_data_len = data_len;

        /* Get Data pointer associated with this IRP. */
        if (data_len)
        {
            NU_USB_IRP_Get_Data(irp, (UINT8 **) &data);
        }
        else
        {
            data = NU_NULL;
        }

        irp_info->next_data = data;

        irp_info->irp = irp;
        irp_info->done_tds = NU_NULL;
        irp_info->sch_tds = NU_NULL;

        /* Find the no. of TDs needed by this IRP. */
        status = ohci_find_td_count(ohci, ed_info, irp,
                                                      maxPacket, &tdcount);

        if (status != NU_SUCCESS)
        {
            return (status);
        }

        irp_info->req_tds = tdcount;

        /* Get address of TD Head for this EP. */
        OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdHead, tdHead);

        if (tdHead & USBH_OHCI_ED_PTR_HALTED)
        {
            NU_ASSERT(0);
        }

        /* Disable this endpoint for the transfers while scheduling is
          * being done.
          */
        controlinfo |= USBH_OHCI_EDCTL_SKIP;
        OHCI_HW_WRITE32(ohci, &ed_info->hwEd->controlinfo, controlinfo);

    }

    /* Save the previously scheduled TDs count. */
    previous_sch_tds = irp_info->sch_tds;

    ed_info->on_global_pending = NU_FALSE;

    switch (ed_info->type)
    {
/*=======================================================================*/
        case USB_EP_INTR:
        case USB_EP_BULK:
/*=======================================================================*/
            /* Formulate the TD's control info - the first DWORD of a TD.*/
            if ((ed_info->bEndpointAddress & 0x80) == USB_DIR_IN)
            {
                controlinfo = USBH_OHCI_TD_CTL_TOGGLE_USEED |
                              USBH_OHCI_TICTL_CC_MASK |
                              USBH_OHCI_TD_CTL_PID_IN;
            }
            else
            {
                controlinfo = USBH_OHCI_TD_CTL_TOGGLE_USEED |
                              USBH_OHCI_TICTL_CC_MASK |
                              USBH_OHCI_TD_CTL_PID_OUT;
            }

            /* Fill the TDs. */

            /* While buffer remaining user allocated buffer length is
             * greater than USBH_OHCI_MAX_DATA_PER_TD.
             * next_index is initially at zero.
             */

            current_td_count = irp_info->sch_tds - irp_info->done_tds;

            while ((irp_info->next_data_len >= USBH_OHCI_MAX_DATA_PER_TD) &&
                     (current_td_count < OHCI_MAX_TDS_PER_IRP))
            {
                status = ohci_make_td(ohci,
                              controlinfo,
                              irp_info->next_data,
                              USBH_OHCI_MAX_DATA_PER_TD,
                              irp_info,
                              ed_info,
                              &filled_td,
                              previous_td);

                /* If more TDs are cant be allocated,  previous_td is used
                 * by ohci_make_td to set interrupt on it.
                 */
                previous_td = filled_td;

                /* TD couldn't be completed. */
                if (status != NU_SUCCESS)
                {
                    break;
                }
                else
                {
                    current_td_count++;

                    /* Increment the next_data pointer by length of 1
                     * buffer.
                     */
                    irp_info->next_data += USBH_OHCI_MAX_DATA_PER_TD;

                    /* Decrement the total user allocated buffer by
                     * USBH_OHCI_MAX_DATA_PER_TD.
                     */
                    irp_info->next_data_len -= USBH_OHCI_MAX_DATA_PER_TD;
                }
            }

            /* If next_data_len < USBH_OHCI_MAX_DATA_PER_TD and next_index
             * has not reached 32.
             */
            if ((current_td_count < OHCI_MAX_TDS_PER_IRP) &&
                (status == NU_SUCCESS))
            {
                /* If next_data_len > 0. */
                if (irp_info->next_data_len)
                {
                    /* make another TD. */
                    status = ohci_make_td(ohci,
                                  controlinfo,
                                  irp_info->next_data,
                                  irp_info->next_data_len,
                                  irp_info,
                                  ed_info,
                                  &filled_td,
                                  previous_td);

                    /* If more TDs are cant be allocated,  previous_td is
                     * used by ohci_make_td to set interrupt on it.
                     */
                    previous_td = filled_td;

                    if(status == NU_SUCCESS)
                    {
                        irp_info->next_data_len = 0;

                        current_td_count++;
                    }
                }

                /* Check if a zero length packet is required. */
                if ((current_td_count < OHCI_MAX_TDS_PER_IRP) &&
                    (irp_info->sch_tds < irp_info->req_tds) &&
                    (status == NU_SUCCESS))
                {
                    /* IRP needs more TDs and there is room for more TDs.*/
                    status = ohci_make_td(ohci,
                                      controlinfo,
                                      NU_NULL,
                                      NU_NULL,
                                      irp_info,
                                      ed_info,
                                      &filled_td,
                                      previous_td);

                    if(status == NU_SUCCESS)
                    {
                        current_td_count++;
                    }
                }
            }

            /* If no TD can be scheduled, place this IRP in a global
             * pending list that will be accessed when any other IRP
             * retires.
             */
            if(status != NU_SUCCESS)
            {
                if(status == NU_NO_MEMORY)
                {
                    if(irp_info->sch_tds == previous_sch_tds)
                    {
                        USBH_OHCI_PROTECT;
                        /* Out of memory. put the Ed in the global pending
                         * list.
                         */
                        ed_info->on_global_pending = NU_TRUE;

                        OHCI_ADD_TO_LIST(ohci->global_pend, ed_info);
                        USBH_OHCI_UNPROTECT;
                    }

                    /* Continue with the TDs that have been scheduled. */
                    status = NU_SUCCESS;
                }
                else
                {
                    NU_ASSERT(NU_NULL);
                }
            }

            /* As this function can be called form handle_retire_td and
             * submit IRP, make sure that there is a need for enabling
             * the ED i.e some TDs have been scheduled.
             */
            if((irp_info->sch_tds != previous_sch_tds) &&
                 (status == NU_SUCCESS))
            {
                /* Enable this endpoint for transfers. */
                OHCI_HW_READ32(ohci, &ed_info->hwEd->controlinfo, controlinfo);

                controlinfo &= ~USBH_OHCI_EDCTL_SKIP;

                OHCI_HW_WRITE32(ohci, &ed_info->hwEd->controlinfo, controlinfo);

                /* Tell the HC that there are new TDs ready for transfer.*/
                if (ed_info->type == USB_EP_BULK)
                {
                    OHCI_HW_WRITE32(ohci,
                                (ohci->cb.controller.base_address +
                                 USBH_OHCI_HC_COMMAND_STATUS),
                                 USBH_OHCI_CS_BLF);
                }
            }

            break;
/*=======================================================================*/
        case USB_EP_CTRL:
/*=======================================================================*/

            NU_USBH_CTRL_IRP_Get_Direction((NU_USBH_CTRL_IRP *)irp,
                                           &direction);


/*-----------------------------------------------------------------------*/
            if(ed_info->next_state == USBH_OHCI_CTRL_SETUP)
/*-----------------------------------------------------------------------*/
            {

                controlinfo = USBH_OHCI_TICTL_CC_MASK |
                              USBH_OHCI_TD_CTL_PID_SETUP |
                              USBH_OHCI_TD_CTL_TOGGLE_DATA0;

                NU_USBH_CTRL_IRP_Get_Setup_Pkt((NU_USBH_CTRL_IRP *)irp,
                                               &setup_data);

                status = ohci_make_td(ohci,
                             controlinfo,
                             (UINT8 *) setup_data,
                             sizeof(NU_USB_SETUP_PKT),
                             irp_info,
                             ed_info,
                             &filled_td,
                             previous_td);

                /* If more TDs are cant be allocated,  previous_td is used
                 * by ohci_make_td to set interrupt on it.
                 */
                previous_td = filled_td;

                if(status == NU_NO_MEMORY)
                {
                    USBH_OHCI_PROTECT;
                    /* No memory is available. Put ED on global pending
                     * list.
                     */
                    ed_info->on_global_pending = NU_TRUE;

                    OHCI_ADD_TO_LIST(ohci->global_pend, ed_info);

                    USBH_OHCI_UNPROTECT;
                }
                else if(status == NU_SUCCESS)
                {
                    ed_info->next_state = USBH_OHCI_CTRL_DATA;
                }
            }
/*-----------------------------------------------------------------------*/
            if((ed_info->next_state == USBH_OHCI_CTRL_DATA) &&
               (status == NU_SUCCESS))
/*-----------------------------------------------------------------------*/
            {
                if (irp_info->next_data_len > 0)
                {
                    controlinfo = USBH_OHCI_TICTL_CC_MASK |
                                  USBH_OHCI_TD_CTL_BFR_RND |
                                  USBH_OHCI_TD_CTL_TOGGLE_DATA1;

                    if (direction == USB_DIR_IN)
                    {
                        controlinfo |= USBH_OHCI_TD_CTL_PID_IN;
                    }
                    else
                    {
                         controlinfo |= USBH_OHCI_TD_CTL_PID_OUT;
                    }

                    status = ohci_make_td(ohci,
                                 controlinfo,
                                 irp_info->next_data,
                                 irp_info->next_data_len,
                                 irp_info,
                                 ed_info,
                                 &filled_td,
                                 previous_td);

                    /* If more TDs are cant be allocated,  previous_td is
                     * used by ohci_make_td to set interrupt on it.
                     */
                    previous_td = filled_td;

                    /* If this stage couldn't be scheduled, put the IRP on
                     * global pending list.
                     */
                    if((status == NU_NO_MEMORY) &&
                         (irp_info->sch_tds == previous_sch_tds))
                    {
                        USBH_OHCI_PROTECT;
                        OHCI_ADD_TO_LIST(ohci->global_pend, ed_info);
                        ed_info->on_global_pending = NU_TRUE;
                        USBH_OHCI_UNPROTECT;
                    }
                    else if(status == NU_SUCCESS)
                    {
                        if ((use_empty_packet)
                            && ((irp_info->next_data_len % maxPacket) == 0x00))
                        {
                            controlinfo = USBH_OHCI_TICTL_CC_MASK |
                                  USBH_OHCI_TD_CTL_BFR_RND |
                                  USBH_OHCI_TD_CTL_TOGGLE_DATA0;

                            if (direction == USB_DIR_IN)
                            {
                                controlinfo |= USBH_OHCI_TD_CTL_PID_IN;
                            }
                            else
                            {
                                controlinfo |= USBH_OHCI_TD_CTL_PID_OUT;
                            }

                            status = ohci_make_td(ohci,
                                                 controlinfo,
                                                 NU_NULL,
                                                 0,
                                                 irp_info,
                                                 ed_info,
                                                 &filled_td,
                                                 previous_td);

                            /* If more TDs are cant be allocated,  previous_td is
                             * used by ohci_make_td to set interrupt on it.
                             */
                            previous_td = filled_td;

                            /* If this stage couldn't be scheduled, put the IRP on
                             * global pending list.
                             */
                            if((status == NU_NO_MEMORY) &&
                                 (irp_info->sch_tds == previous_sch_tds))
                            {
                                USBH_OHCI_PROTECT;
                                OHCI_ADD_TO_LIST(ohci->global_pend, ed_info);
                                ed_info->on_global_pending = NU_TRUE;
                                USBH_OHCI_UNPROTECT;
                            }
                        }
                        ed_info->next_state = USBH_OHCI_CTRL_STATUS;
                    }
                }
                else
                {
                  ed_info->next_state = USBH_OHCI_CTRL_STATUS;
                }
            }

/*-----------------------------------------------------------------------*/
            if((ed_info->next_state == USBH_OHCI_CTRL_STATUS) &&
               (status == NU_SUCCESS))
/*-----------------------------------------------------------------------*/
            {

                if (direction == USB_DIR_IN)
                {
                    controlinfo = USBH_OHCI_TICTL_CC_MASK |
                                  USBH_OHCI_TD_CTL_PID_OUT |
                                  USBH_OHCI_TD_CTL_TOGGLE_DATA1;
                }
                else
                {
                    controlinfo = USBH_OHCI_TICTL_CC_MASK |
                                  USBH_OHCI_TD_CTL_PID_IN |
                                  USBH_OHCI_TD_CTL_TOGGLE_DATA1;
                }

                status = ohci_make_td(ohci,
                                controlinfo,
                                 NU_NULL,
                                 NU_NULL,
                                 irp_info,
                                 ed_info,
                                 &filled_td,
                                 previous_td);

                if((status == NU_NO_MEMORY) &&
                     (irp_info->sch_tds == previous_sch_tds))
                {
                    USBH_OHCI_PROTECT;
                    OHCI_ADD_TO_LIST(ohci->global_pend, ed_info);
                    ed_info->on_global_pending = NU_TRUE;
                    USBH_OHCI_UNPROTECT;
                }
                else if(status == NU_SUCCESS)
                {
                    ed_info->next_state = USBH_OHCI_CTRL_SETUP;
                }
            }

            /* As on no memory status, either there are some TD's for this
             * ED that have been scheduled or the ED has been placed on the
             * global pending list. In both cases return success to the
             * calling function. Otherwise IRP will be retired with
             * failure status.
             */
            if(status == NU_NO_MEMORY)
            {
                status = NU_SUCCESS;
            }

            /* If any TD has been scheduled for this IRP, enable the ED. */
            if(irp_info->sch_tds != previous_sch_tds)
            {
                /* Enable this endpoint for transfers. */
                OHCI_HW_READ32(ohci, &ed_info->hwEd->controlinfo, controlinfo);

                controlinfo &= ~USBH_OHCI_EDCTL_SKIP;

                OHCI_HW_WRITE32(ohci, &ed_info->hwEd->controlinfo, controlinfo);

                OHCI_HW_WRITE32(ohci,
                           (ohci->cb.controller.base_address +
                            USBH_OHCI_HC_COMMAND_STATUS),
                            USBH_OHCI_CS_CLF);
            }

            break;

/*-----------------------------------------------------------------------*/
        default:
/*-----------------------------------------------------------------------*/
            break;
    }

    /* If the status is NOT NU_SUCCESS either submit IRP or
     * handle_retire_td will return an error.
     */
    return (status);
}

/************************************************************************
* FUNCTION
*   ohci_handle_iso_irp
*
* DESCRIPTION
*   Handles translating the IRP in to TD required to initiate the transfer.
*
* INPUTS
*   ohci   Handle that identifies the HC in the OHCI HC database.
*   ed_info Identifies the ED over which the IRP is to be transferred.
*   irp    Pointer to the IRP that has to be transferred.
*
* OUTPUTS
*    NU_SUCCESS              Indicates that the count has been
*                            successfully calculated.
*    NU_INVALID_SIZE   Indicates that the irp->usb_irp.length is
*                            greater than 4k for control transfer.
*    NU_NO_MEMORY            Indicates that memory allocation failed.
*    NU_USB_OHCI_ISO_MAX_IRP Indicates maximum number of IRPs has been
*                            submitted, and new IRP can't be submitted
*                            before retirement of already submitted IRP.
*
*************************************************************************/
STATUS ohci_handle_iso_irp(NU_USBH_OHCI *ohci,
                           USBH_OHCI_ED_INFO *ed_info,
                           NU_USB_IRP *irp)
{
    UINT8                 **pdata_array;
    UINT8                   iso_trans_per_td;
    UINT16                  trans_count = NU_NULL;
    UINT32                  controlinfo = NU_NULL;
    UINT32                  tdHead;
    UINT32                  count;
    UINT16                  td_len;
    USBH_OHCI_TD_ISO                   *iso_hwTD;
    USBH_OHCI_ISO_IRP_INFO      *iso_irp_info = NU_NULL;
    USBH_OHCI_ISO_ED_INFO  *iso_ed_info;

    NU_ASSERT(ohci != NU_NULL);
    NU_ASSERT(ed_info != NU_NULL);
    NU_ASSERT(irp != NU_NULL);

    /* Typecast the ed_info to iso_ed_info. */
    iso_ed_info = (USBH_OHCI_ISO_ED_INFO *) ed_info;

    /* Check maximum IRPs has been submitted. */
    if (((iso_ed_info->incoming_irp_index + 1)
           == iso_ed_info->done_irp_index) ||
        ((iso_ed_info->incoming_irp_index == USBH_OHCI_MAX_PEND_IRPS) &&
         (iso_ed_info->done_irp_index == 0)))
    {
        return (NU_USB_MAX_EXCEEDED);
    }

    /* Set IRP Status as not executed yet. */
    NU_USB_IRP_Set_Status(irp, NU_USB_NOT_ACCESSED);

    /* Get address of TD Head for this EP. */
    OHCI_READ_ADDRESS(ohci, &ed_info->hwEd->tdHead, tdHead);
    if (tdHead & 1)
    {
        NU_ASSERT(0);
    }

    /* Read 1st byte, (Control Info) of EP Descriptor from the HCCA.
     * Ref. 4.2.1.
     */
    OHCI_HW_READ32(ohci, &ed_info->hwEd->controlinfo, controlinfo);

    NU_USB_ISO_IRP_Get_Num_Transactions ((NU_USB_ISO_IRP *) irp,
                                         &trans_count);

    iso_irp_info =
             &iso_ed_info->irp_info_array[iso_ed_info->incoming_irp_index];

    memset((VOID *)iso_irp_info, 0, sizeof(USBH_OHCI_ISO_IRP_INFO));

    /* Fill iso_irp_info structure. */
    iso_irp_info->req_trans    = trans_count;
    iso_irp_info->ed        = ed_info;
    iso_irp_info->irp       = irp;

    USBH_OHCI_PROTECT;

    /* If incoming_irp_index is equal to done_irp_index, this indicates
     * the 1st IRP has been submitted by application.
     */
    if (iso_ed_info->incoming_irp_index == iso_ed_info->done_irp_index)
    {
        /* Total free TDs are 32. Use only 31 at a time. */
        iso_ed_info->free_tds = OHCI_MAX_ISO_TDS_PER_IRP - 1;
        iso_ed_info->trans_start_flag = 0;
        iso_ed_info->sch_irp_index = iso_ed_info->done_irp_index;

        /* Get buffer pointer for this transaction. */
        NU_USB_ISO_IRP_Get_Buffer_Array((NU_USB_ISO_IRP *) irp,
                                                             &pdata_array);

        count = 0;

        NU_ASSERT(iso_irp_info->req_trans != 0);

         /* Submit TD 0 to maximum of 30. */
        while((count < (OHCI_MAX_ISO_TDS_PER_IRP - 1)) &&
              (iso_irp_info->sch_trans < iso_irp_info->req_trans))
        {
            /* Find maximum number of transactions that can be
             * accommodated in one TD.
             */
            iso_trans_per_td = ohci_check_iso_trans_per_td(ohci,
                                  iso_irp_info,
                                  (NU_USB_ISO_IRP *) irp,
                                  &td_len);

            /* Set Control DWORD by setting DI to 7. */
            controlinfo = 0x00000000;
            controlinfo |= OHCI_TD_DI_SET(0);
            controlinfo |= OHCI_TD_FC_SET(iso_trans_per_td);

            /* Get hwTd pointer from SW TD. */
            iso_hwTD =
                  &iso_ed_info->td_array_start[iso_ed_info->next_td_index];

            /* Call Fill TD function to allocate new TD to OHCI Hardware.*/
            ohci_fill_iso_td(ohci,
                         controlinfo,
                         pdata_array[iso_irp_info->sch_trans],
                         td_len,
                         ed_info,
                         iso_ed_info->next_td_index,
                         iso_hwTD);

            NU_ASSERT(iso_irp_info->req_trans != 0);

            /* Increment the global TD index. This index can go max
             * of 32.
             */
            OHCI_MOD_INC(iso_ed_info->next_td_index,
                         OHCI_MAX_ISO_TDS_PER_IRP);

            /* Decrement the global free TD count. */
            iso_ed_info->free_tds--;

            /* Increment the transaction index associated with IRP. */
            iso_irp_info->sch_trans += iso_trans_per_td + 1;

            count++;
        }

        /* Reset sKip bit. */
        OHCI_HW_READ32(ohci, &ed_info->hwEd->controlinfo, controlinfo);

        controlinfo &= ~USBH_OHCI_EDCTL_SKIP;

        OHCI_HW_WRITE32(ohci, &ed_info->hwEd->controlinfo, controlinfo);
    }

    OHCI_MOD_INC(iso_ed_info->incoming_irp_index, USBH_OHCI_MAX_PEND_IRPS);

    USBH_OHCI_UNPROTECT;

    return NU_SUCCESS;
}

/************************************************************************
* FUNCTION
*   ohci_check_iso_trans_per_td
*
* DESCRIPTION
*   Find out the maximum number of transactions that can be packed in one
*   ISO TD.
*
* INPUTS
*   ohci    Handle that identifies the HC in the OHCI HC database.
*   irp_info Identifies IRP associated with this TD.
*   irp     Pointer to the IRP that has to be transferred.
*
* OUTPUTS
*   Number of ISO transaction, that can be transferred in one TD.
*
*************************************************************************/
UINT8 ohci_check_iso_trans_per_td(NU_USBH_OHCI *ohci,
                                  USBH_OHCI_ISO_IRP_INFO *irp_info,
                                  NU_USB_ISO_IRP *irp,
                                  UINT16  *td_length)
{
    UINT8      i;
    UINT8    **pdata_array;
    UINT16    *buf_lengths;
    UINT16     num_transactions;
    UINT32     index;
    UINT32     offset;

    /* Get Array of pointers, pointing to the data buffers submitted for
     * ISO transactions.
     */
    NU_USB_ISO_IRP_Get_Buffer_Array(irp, &pdata_array);

    /* Get Array of lengths of these data buffers. */
    NU_USB_ISO_IRP_Get_Lengths(irp, &buf_lengths);

    /* Get total number of transactions associated with IRP. */
    NU_USB_ISO_IRP_Get_Num_Transactions(irp, &num_transactions);

    NU_ASSERT(pdata_array != NU_NULL);
    NU_ASSERT(buf_lengths != NU_NULL);
    NU_ASSERT(num_transactions != NU_NULL);

    /* Get starting offset of data pointed by next transaction to be done.
     */
    offset =  ((UINT32)(pdata_array[irp_info->sch_trans]) & 0x00000fff);

    /* Set length equal to length of 1st transaction. */
    *td_length = buf_lengths[irp_info->sch_trans];

    /* Initialize counter i. */
    i = 0;

    while(i < 7)
    {
        index = irp_info->sch_trans + i;

        /* If index + 1 is equal to number of transactions, index is
         * pointing to the last element and i defines the max transactions
         * which can be submitted in one TD.
         */
        if (index + 1 == num_transactions)
        {
            break;
        }

        /* Check whether data buffers are continuous or not. One TD can
         * handle only continuous data buffer.
         */
        if ((pdata_array[index] + buf_lengths[index]) !=
                                                      pdata_array[index+1])
        {
            break;
        }

        /* Check data pointers cross two page boundary or not, as one TD
         * can only support buffers not crossing one 4k page boundary.
         */
        offset += buf_lengths[index + 1];
        if ((offset >> 13) != 0)
        {
            break;
        }

         /* Update total length associated with this TD. */
        *td_length += buf_lengths[index + 1];

        i++;
    }

    return i;
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_invalid_cmd
*
* DESCRIPTION
*
*       This function handles all the un-supported commands.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*       irp                                 Pointer to the Data transfer.
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED.               Always
*
**************************************************************************/
STATUS ohci_rh_invalid_cmd(NU_USBH_OHCI *ohci, NU_USBH_CTRL_IRP *irp)
{
    /* Check if OHCI's control block is valid. */
    NU_ASSERT(ohci != NU_NULL);
    NU_ASSERT(~((UINT32)ohci & 0x3));

    /* Check if IRP is valid. */
    NU_ASSERT(irp != NU_NULL);
    NU_ASSERT(~((UINT32)irp & 0x3));

    return ( NU_USB_NOT_SUPPORTED );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_nothing_to_do_cmd
*
* DESCRIPTION
*
*       This function handles all the commands for which no action is
*       required from the driver.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*       irp                                 Pointer to the Data transfer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always
*
**************************************************************************/
STATUS ohci_rh_nothing_to_do_cmd(NU_USBH_OHCI *ohci,
                                   NU_USBH_CTRL_IRP *irp)
{
    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_set_hub_feature
*
* DESCRIPTION
*
*       This function handles the SET_FEATURE request for the root-hub.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*       feature                             Feature to be set.
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED                Always
*
**************************************************************************/
STATUS ohci_rh_set_hub_feature(NU_USBH_OHCI *ohci, UINT8 feature)
{
    return ( NU_USB_NOT_SUPPORTED );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_set_port_feature
*
* DESCRIPTION
*
*       This function handles the SET_FEATURE request for the ports.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*       feature                             Feature to be set.
*       port                                Port for which the feature  is
*                                           to be set.
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED                If the specified feature
*                                           doesn't exist.
*       NU_SUCCESS                          On Success.
*
**************************************************************************/
STATUS ohci_rh_set_port_feature(NU_USBH_OHCI *ohci,
                                UINT8 port,
                                UINT8 feature)
{
    STATUS status = NU_SUCCESS;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
    USBH_OHCI_SESSION_HANDLE *session_handle;
#endif

    UINT32 feature_bmp = (0x1 << feature);

    /* Check if OHCI's control block is valid. */
    NU_ASSERT(ohci != NU_NULL);
    NU_ASSERT(~((UINT32)ohci & 0x3));

    /* If invalid, return error. */
    if (feature_bmp & ~(OHCI_RH_SPRTD_SET_FEATURE_MAP))
    {
        status =  NU_USB_NOT_SUPPORTED;
        NU_ASSERT(0);
    }
    else
    {
        /* Since all the invlid options have been ruled out, so just write the
         * bitmap in the port status register. Writing 0 has no effect.
         */

        OHCI_HW_WRITE32(ohci,
                        (ohci->cb.controller.base_address +
                         USBH_OHCI_HC_RH_PORT_STATUS +
                         OHCI_GET_PORTSTS_REG_OFFSET(port)),
                        feature_bmp);

        /* In case of port suspend feature selector wait for the suspend 
         * to complete.
         */
        if(feature_bmp & (0x1 << USBH_HUB_FEATURE_PORT_SUSPEND))
        {
            NU_Sleep(2);
        }

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
        if (feature_bmp & (0x1 << USBH_HUB_FEATURE_PORT_RESET))
        {
            session_handle = (USBH_OHCI_SESSION_HANDLE *)ohci->ses_handle;

            if ( session_handle != NU_NULL )
            {
                /* Reset the watchdog */
                (VOID)PMI_Reset_Watchdog(session_handle->ohci_inst_handle->pmi_dev);
            }
        }
#endif
    }


    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_clear_hub_feature
*
* DESCRIPTION
*
*       This function handles the CLEAR_FEATURE request for the root-hub.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*       feature                             The feature  is to be cleared.
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED                If the specified feature
*                                           doesn't exist.
*       NU_SUCCESS                          On Success.
*
**************************************************************************/
STATUS ohci_rh_clear_hub_feature(NU_USBH_OHCI *ohci, UINT8 feature)
{
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Only over current feature is supported at the Hub level. any thing
     * else is an error.
     */
    if (feature == USBH_HUB_FEATURE_C_OVER_CURRENT)
    {
        OHCI_HW_WRITE32(ohci,
                        (ohci->cb.controller.base_address +
                         USBH_OHCI_HC_RH_STATUS),
                        USBH_OHCI_RHS_OCIC);

        status = NU_SUCCESS;
    }

    return (status );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_clear_port_feature
*
* DESCRIPTION
*
*       This function handles the CLEAR_FEATURE request for the ports.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*       feature                             Feature to be cleared.
*       port                                Port for which the feature is
*                                           to be cleared.
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED                If the specified feature
*                                           doesn't exist.
*       NU_USB_INVLD_ARG
*       NU_SUCCESS                          On Success.
*
**************************************************************************/

STATUS ohci_rh_clear_port_feature(NU_USBH_OHCI *ohci,
                                  UINT8 port,
                                  UINT8 feature)
{
    UINT32 feature_bmp = (0x1 << feature);
    STATUS status = NU_SUCCESS;
    UINT32 port_sts;

    /* If an invalid request, return error.  */
    if (feature_bmp & ~(OHCI_RH_SPRTD_CLEAR_FEATURE_MAP))
    {
        NU_ASSERT(0);
        status = NU_USB_NOT_SUPPORTED;
    }

    /* If the port is to be resumed. */
    else if(feature_bmp & (0x1 << USBH_HUB_FEATURE_PORT_SUSPEND))
    {
        /* Need to resume the port. Read the port status register and check
         * if the port is in suspended state. If so than resume it.
         */
        OHCI_HW_READ32(ohci, 
                      (ohci->cb.controller.base_address +
                       USBH_OHCI_HC_RH_PORT_STATUS +
                       OHCI_GET_PORTSTS_REG_OFFSET(port)),
                      port_sts);

        if ( port_sts  & ( 0x1 << USBH_HUB_FEATURE_PORT_SUSPEND ))
        {
            feature_bmp = 1 << USBH_HUB_FEATURE_PORT_OVER_CURRENT;

            /* Clear the feature.  */
            OHCI_HW_WRITE32(ohci,
                           (ohci->cb.controller.base_address +
                           USBH_OHCI_HC_RH_PORT_STATUS +
                           OHCI_GET_PORTSTS_REG_OFFSET(port) ),
                           feature_bmp);

            /* Wait for resume signalling to complete. Slightly large time
             * is chosen so that the code works fine for different HW's.
             */
            NU_Sleep(10);

        }
    }

    else
    {
        /* Clear the feature.  */
        OHCI_HW_WRITE32(ohci,
                       (ohci->cb.controller.base_address +
                       USBH_OHCI_HC_RH_PORT_STATUS +
                       OHCI_GET_PORTSTS_REG_OFFSET(port) ),
                       feature_bmp);
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_clear_feature
*
* DESCRIPTION
*
*       This function handles the CLEAR_FEATURE request for the root-hub.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*       irp                                 Pointer to the Data transfer.
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED                If the specified feature
*                                           doesn't exist or in the event
*                                           of invalid request.
*       NU_SUCCESS                          On Success.
*
**************************************************************************/
STATUS ohci_rh_clear_feature(NU_USBH_OHCI *ohci, NU_USBH_CTRL_IRP *irp)
{
    UINT8   bmRequestType;
    UINT16  wIndex;
    UINT16  wValue;
    STATUS status = NU_USB_NOT_SUPPORTED;

    NU_USBH_CTRL_IRP_Get_bmRequestType(irp, &bmRequestType);

    NU_USBH_CTRL_IRP_Get_wValue(irp, &wValue);

    wValue = LE16_2_HOST(wValue);

    NU_USBH_CTRL_IRP_Get_wIndex(irp, &wIndex);

    wIndex = LE16_2_HOST(wIndex);

    if (!USBH_RH_IS_CLASS_SPECIFIC(bmRequestType))
    {
        /* Standard clear features are not applicable. */
        status =  NU_USB_NOT_SUPPORTED;
    }
    else
    {
        /* Only the class specific clear features are handled. */
        switch (USBH_RH_GET_RECIPIENT(bmRequestType))
        {
           case USBH_RH_RECIPIENT_DEVICE:
                status = ohci_rh_clear_hub_feature(ohci,
                                                      USBH_RH_LSB(wValue));
                break;

            case USBH_RH_RECIPIENT_PORT:
                status = ohci_rh_clear_port_feature(ohci,
                                         USBH_RH_LSB(wIndex),
                                         USBH_RH_LSB(wValue));
                break;
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_set_feature
*
* DESCRIPTION
*
*       This function handles the SET_FEATURE request for the root-hub.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*       irp                                 Pointer to the Data transfer.
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED                If the specified feature
*                                           doesn't exist or in the event
*                                           of invalid request.
*       NU_SUCCESS                          On Success.
*
**************************************************************************/
STATUS ohci_rh_set_feature(NU_USBH_OHCI *ohci, NU_USBH_CTRL_IRP *irp)
{
    UINT8   bmRequestType;
    UINT16  wIndex;
    UINT16  wValue;
    STATUS status = NU_USB_NOT_SUPPORTED;

    NU_USBH_CTRL_IRP_Get_bmRequestType(irp, &bmRequestType);

    NU_USBH_CTRL_IRP_Get_wValue(irp, &wValue);

    wValue = LE16_2_HOST(wValue);

    NU_USBH_CTRL_IRP_Get_wIndex(irp, &wIndex);

    wIndex = LE16_2_HOST(wIndex);

    if (!USBH_RH_IS_CLASS_SPECIFIC(bmRequestType))
    {
        /* Standard clear features are not applicable for us.  */
        status = NU_USB_NOT_SUPPORTED;
    }
    else
    {
        /* Only the class specific clear features are handled.  */
        switch (USBH_RH_GET_RECIPIENT(bmRequestType))
        {
            case USBH_RH_RECIPIENT_DEVICE:
                status = ohci_rh_set_hub_feature(ohci,USBH_RH_LSB(wValue));
                break;

            case USBH_RH_RECIPIENT_PORT:
                status = ohci_rh_set_port_feature(ohci,
                                       USBH_RH_LSB(wIndex),
                                       USBH_RH_LSB(wValue));
                break;
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_get_state
*
* DESCRIPTION
*
*       This function handles the GET_STATE request for the root-hub.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*       irp                                 Pointer to the Data transfer.
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED                Always.
*
**************************************************************************/
STATUS ohci_rh_get_state(NU_USBH_OHCI *ohci, NU_USBH_CTRL_IRP *irp)
{
    return ( NU_USB_NOT_SUPPORTED );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_get_descriptor
*
* DESCRIPTION
*
*       This function handles the GET_DESCRIPTOR for the root-hub.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*       irp                                 Pointer to the Data transfer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always.
*
**************************************************************************/
STATUS ohci_rh_get_descriptor(NU_USBH_OHCI *ohci, NU_USBH_CTRL_IRP *irp)
{
    UINT16  wLength;
    UINT8  *irp_buffer;
    UINT8   bmRequestType;
    UINT16  wValue;
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Depending on the descriptor required, the data is copied to the user
     * buffer. Length is set properly and the control is returned to the
     * caller.
     */
    USBH_RH_WLENGTH(irp);

    USBH_RH_GET_BUFFER(irp);

    NU_USBH_CTRL_IRP_Get_bmRequestType(irp, &bmRequestType);

    NU_USBH_CTRL_IRP_Get_wValue(irp, &wValue);

    wValue = LE16_2_HOST(wValue);

    if (!USBH_RH_IS_CLASS_SPECIFIC(bmRequestType))
    {
        /* Device Descriptor. */
        if (USBH_RH_MSB(wValue) == USB_DT_DEVICE)
        {
            if (wLength > rh_dev_desc[OHCI_RH_LENGTH_OFFSET_DEV_DESC])
            {
                wLength = rh_dev_desc[OHCI_RH_LENGTH_OFFSET_DEV_DESC];
            }
            memcpy(irp_buffer, rh_dev_desc, wLength);
        }
        else
        {
            if (wLength >= rh_cfg_desc[OHCI_RH_LENGTH_OFFSET_CONFIG_DESC])
            {
                wLength = rh_cfg_desc[OHCI_RH_LENGTH_OFFSET_CONFIG_DESC];
            }

            memcpy(irp_buffer, rh_cfg_desc, wLength);
        }
        USBH_RH_SET_LENGTH(irp, wLength);

        status = NU_SUCCESS;

    }
    else
    {
        /* GET_USBH_HUB_DESCRIPTOR command. */

        if (wLength >= ohci->rh_hub_desc[OHCI_RH_LENGTH_OFFSET_HUB_DESC])
        {
            wLength = ohci->rh_hub_desc[OHCI_RH_LENGTH_OFFSET_HUB_DESC];
        }

        memcpy(irp_buffer, ohci->rh_hub_desc, wLength);

        USBH_RH_SET_LENGTH(irp, wLength);

        status = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_get_status
*
* DESCRIPTION
*
*       This function handles the GET_STATUS for the root-hub.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*       irp                                 Pointer to the Data transfer.
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED                If the specified feature
*                                           doesn't exist or in the event
*                                           of invalid request.
*       NU_SUCCESS                          On success.
*
**************************************************************************/
STATUS ohci_rh_get_status(NU_USBH_OHCI *ohci, NU_USBH_CTRL_IRP *irp)
{
    UINT8  *irp_buffer;
    UINT8   port;
    UINT8   bmRequestType;
    UINT16  wIndex;
    UINT16  wValue;
    UINT16  wLength;
    STATUS  status = NU_USB_NOT_SUPPORTED;
    UINT32 rh_status;

    USBH_RH_WLENGTH(irp);

    USBH_RH_GET_BUFFER(irp);

    NU_USBH_CTRL_IRP_Get_bmRequestType(irp, &bmRequestType);

    NU_USBH_CTRL_IRP_Get_wValue(irp, &wValue);

    wValue = LE16_2_HOST(wValue);

    NU_USBH_CTRL_IRP_Get_wIndex(irp, &wIndex);

    wIndex = LE16_2_HOST(wIndex);

    if (!USBH_RH_IS_CLASS_SPECIFIC(bmRequestType))
    {
        /* Standard request is not expected. */
        status = NU_SUCCESS;
    }
    else
    {
        /* Class specific request. */
        switch (USBH_RH_GET_RECIPIENT(bmRequestType))
        {
            case USBH_RH_RECIPIENT_DEVICE:
                OHCI_HW_READ32(ohci,
                               (ohci->cb.controller.base_address +
                                USBH_OHCI_HC_RH_STATUS),
                               rh_status);

                rh_status &= (USBH_RH_HUB_STATUS_MASK);

                *(UNSIGNED *) irp_buffer = HOST_2_LE32(rh_status);

                USBH_RH_SET_LENGTH(irp, sizeof(UINT32));

                status = NU_SUCCESS;
            break;

            case USBH_RH_RECIPIENT_PORT:
                port = USBH_RH_LSB(wIndex);

                OHCI_HW_READ32(ohci,
                           (ohci->cb.controller.base_address +
                            USBH_OHCI_HC_RH_PORT_STATUS +
                            OHCI_GET_PORTSTS_REG_OFFSET(port)),
                           rh_status);

                rh_status &= (USBH_RH_PORT_STATUS_MASK);

                *(UNSIGNED *) irp_buffer = HOST_2_LE32(rh_status);

                USBH_RH_SET_LENGTH(irp, sizeof(UINT32));

                status = NU_SUCCESS;
            break;
        }
    }

    return ( status );
}

/* Array of Root Hub functions. */
STATUS (*ohci_rh_request[]) (NU_USBH_OHCI *ohci, NU_USBH_CTRL_IRP *irp) =
{
    ohci_rh_get_status,                      /* GET_STATUS.        */
    ohci_rh_clear_feature,                   /* CLEAR_FEATURE.     */
    ohci_rh_get_state,                       /* USBH_HUB_SPECIFIC. */
    ohci_rh_set_feature,                     /* SET_FEATURE.       */
    ohci_rh_invalid_cmd,                     /* UN_SPECIFIED.      */
    ohci_rh_nothing_to_do_cmd,               /* SET_ADDRESS.       */
    ohci_rh_get_descriptor,                  /* GET_DESCRIPTOR.    */
    ohci_rh_invalid_cmd,                     /* SET_DESCRIPTOR.    */
    ohci_rh_nothing_to_do_cmd,               /* GET_CONFIGURATION. */
    ohci_rh_nothing_to_do_cmd,               /* SET_CONFIGURATION. */
    ohci_rh_invalid_cmd,                     /* GET_INTERFACE.     */
    ohci_rh_invalid_cmd,                     /* SET_INTERFACE.     */
    ohci_rh_invalid_cmd                      /* SYNC FRAME.        */
};

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_handle_irp
*
* DESCRIPTION
*
*       This function handles all the control transfers that are
*       directed towards the root-hub.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*       irp                                 Pointer to the Data transfer.
*
* OUTPUTS
*
*       status                              IRP execution status.
*
**************************************************************************/
STATUS ohci_rh_handle_irp(NU_USBH_OHCI *ohci,
                                NU_USB_IRP   *irp,
                                UINT8         bEndpointAddress)
{
    STATUS              irp_status;
    STATUS              status = NU_SUCCESS;
    UINT32               irp_length;
    UINT32              *buffer;
    NU_USBH_CTRL_IRP    *ctrl_irp;
    NU_USB_IRP_CALLBACK  callback;
    NU_USB_PIPE         *pipe;
    UINT8 retire_irp = NU_FALSE;
    UINT8                bRequest;
    UINT8               *irp_buffer;

    /* First handle the non-control endpoint requests. */
    if ((bEndpointAddress & OHCI_EP_ADDR_MASK) != NU_NULL)
    {
        /* Check if an updated status is available. */
        if (ohci->rh_status.status_map == NU_NULL)
        {
            /* Save IRP pointer. */
            ohci->rh_status.irp = irp;
        }
        else
        {
            /* Clear any saved IRP. */
            ohci->rh_status.irp = NU_NULL;

            NU_USB_IRP_Get_Data(irp, (UINT8 **) &buffer);

            *buffer = HOST_2_LE32(ohci->rh_status.status_map);

            NU_USB_IRP_Set_Actual_Length(irp, sizeof(UINT32));

            /* Disable interrupts of this OHCI. */
            USBH_OHCI_PROTECT;

            ohci->rh_status.status_map = NU_NULL;

            /* Enable interrupts of this OHCI. */
            USBH_OHCI_UNPROTECT;

            irp_status = NU_SUCCESS;

            retire_irp = NU_TRUE;
        }
    }
    else
    {
        /* Control Transfers. */
        ctrl_irp = (NU_USBH_CTRL_IRP *) irp;

        USBH_RH_GET_BUFFER(ctrl_irp);

        USBH_RH_GET_LENGTH(ctrl_irp);

        /* Clear the buffer. */
        memset((VOID *)irp_buffer, NU_NULL, irp_length);

        USBH_RH_SET_LENGTH(ctrl_irp, NU_NULL);

        USBH_RH_BREQUEST(ctrl_irp);

        if (bRequest >= (sizeof(ohci_rh_request) / sizeof(UINT32)))
        {
            irp_status =  NU_USB_INVLD_ARG;

            status =  NU_USB_INVLD_ARG;
        }
        else
        {
            /* Invoke the function appropriate for the request. */
            irp_status = ohci_rh_request[bRequest] (ohci, ctrl_irp);
        }

        retire_irp = NU_TRUE;
    }

    if(retire_irp == NU_TRUE)
    {
        NU_USB_IRP_Set_Status(irp, irp_status);

        NU_USB_IRP_Get_Callback(irp, &callback);

        NU_USB_IRP_Get_Pipe(irp, &pipe);

        if(callback != NU_NULL)
        {
            callback(pipe, irp);
        }
    }

    return ( status );
}

/**************************************************************************
* FUNCTION
*
*       ohci_rh_init
*
* DESCRIPTION
*
*       This function initializes the root-hub.
*
* INPUTS
*
*       ohci                  Pointer to the OHCI structure containing
*                             root-hub data.
*
* OUTPUTS
*
*       NU_USB_MAX_EXCEEDED   Number of downstream ports are greater than
*                             what OHCI is configured for.
*       NU_SUCCESS            Root hub successfully initialized.
*
**************************************************************************/
STATUS ohci_rh_init(NU_USBH_OHCI *ohci)
{
    UINT32 descA = NU_NULL;
    UINT32 descB = NU_NULL;
    UINT8 port_count;

    /* Calculate what is to be written into the registers. */

    /* Read HcRhDescriptorA Register. */
    OHCI_HW_READ32(ohci,
                   (ohci->cb.controller.base_address +
                    USBH_OHCI_HC_RH_DESCR_A),
                   descA);

    /* Keep NumberDownstreamPorts[7:0] and DeviceType[10] and Reset rest
     * of descriptor.
     */
    descA &= (USBH_RH_DESCA_NUM_PORTS_MASK | USBH_RH_DESCA_DT_MASK);

    /* Check if OHCI is configured for the number of available ports. */
    if(OHCI_MAX_PORTS <  (descA & USBH_RH_DESCA_NUM_PORTS_MASK))
    {
        return (NU_USB_MAX_EXCEEDED);
    }

    /* Hub Descriptor length. */
    ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_LENGTH] = OHCI_RH_DESC_LENGTH;

    /* descriptor type = USBH_HUB_DESCRIPTOR. */
    ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_TYPE] = OHCI_RH_DESC_TYPE;

    /* NDP (NumberDownstreamPorts). */
    ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_NDP] =
                                    (descA & USBH_RH_DESCA_NUM_PORTS_MASK);

    /* Set the port power configuration as specified by user. */
#if (OHCI_PORT_CFG_POWER == OHCI_PORT_CFG_POWER_ALWAYS_ON)

    /* Set NoPowerSwitching[9], Ports are always powered on when the HC is
     * powered on.
     */
    descA |= OHCI_RH_DESCA_NPS;

    /* Not valid as NPS is set. */
    descA &= ~(OHCI_RH_DESCA_PSM);

#elif(OHCI_PORT_CFG_POWER == OHCI_PORT_CFG_POWER_GROUP)

    /* Clear NoPowerSwitching[9] and PowerSwitchingMode[8]. Ports are
     * powered as group.
     */
    descA &= ~(OHCI_RH_DESCA_NPS | OHCI_RH_DESCA_PSM);

#elif(OHCI_PORT_CFG_POWER == OHCI_PORT_CFG_POWER_INDIVIDUAL)

    /* Clear NoPowerSwitching[9] and set PowerSwitchingMode[8]. Ports are
     * powered individually.
     */
    descA &= ~(OHCI_RH_DESCA_NPS);
    descA |= OHCI_RH_DESCA_PSM;

#else

#error Invalid configuration for OHCI_PORT_CFG_POWER in nu_usbh_ohci_cfg.h file.

#endif /* OHCI_PORT_CFG_POWER */

#if (OHCI_PORT_CFG_OC == OHCI_PORT_CFG_OC_NOT_SUPPORTED)

    /* Set NoOverCurrentProtection[12] as configured by user. */
    descA |= (OHCI_RH_DESCA_NOCP);
    descA &= ~(OHCI_RH_DESCA_OCPM);

#elif (OHCI_PORT_CFG_OC == OHCI_PORT_CFG_OC_GROUP)

    /* Clear NoOverCurrentProtection[12] and OverCurrentProtectionMode[11]
     * as configured by user.
     */
    descA &= ~(OHCI_RH_DESCA_NOCP | OHCI_RH_DESCA_OCPM);

#elif (OHCI_PORT_CFG_OC == OHCI_PORT_CFG_OC_INDIVIDUAL)

    /* Clear NoOverCurrentProtection[12]. */
    descA &= ~(OHCI_RH_DESCA_NOCP);

    /* Set OverCurrentProtectionMode[11] to enable individual detection. */
    descA |= (OHCI_RH_DESCA_OCPM);

#else

#error Invalid configuration for OHCI_PORT_CFG_OC in nu_usbh_ohci_cfg.h file.

#endif /* OHCI_PORT_CFG_OC */

    /* wHubCharacteristics.  */
    ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_HUB_CHAR_0] =
                           (OHCI_PORT_CFG_POWER | (OHCI_PORT_CFG_OC << 3));
    /* wHubCharacteristics.  */
    ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_HUB_CHAR_1] = NU_NULL;

    /* bPwrOn2PwrGood.       */
    ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_POTPGT] = OHCI_PORT_CFG_POTPGT;

    /* bHubContrCurrent.     */
    ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_CTRL_CURR] = NU_NULL;

    /* Devices Removable.    */
    ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_DEV_REMOVABLE_0] =
                                            (UINT8)OHCI_PORT_CFG_DR;

    /* Devices Removable.    */
    ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_DEV_REMOVABLE_1] =
                                            (UINT8)(OHCI_PORT_CFG_DR >> 8);

    /* PortPwrCtrlMask.      */
    ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_PWR_CTRL_MASK_0] =
                                (UINT8)OHCI_PORT_CFG_POWER_CTRLMASK;
    /* PortPwrCtrlMask.      */
    ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_PWR_CTRL_MASK_1] =
                                (UINT8)(OHCI_PORT_CFG_POWER_CTRLMASK >> 8);

    /* PPCM (PortPowerControlMask[31:16]) and DeviceRemovable settings. */
    descB = (((UINT32)OHCI_PORT_CFG_POWER_CTRLMASK << 16) |
                                                       (OHCI_PORT_CFG_DR));

    /* Write the decoded values into the registers. */
    /* Update HcRhDescriptorA Register. */
    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_RH_DESCR_A),
                    descA);


    /* Update HcRhDescriptorB Register. */
    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_RH_DESCR_B),
                    descB);

    /* Initialize the status to zero. */
    ohci->rh_status.previous.hub_status = NU_NULL;

    for (port_count = 0;
         port_count < ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_NDP];
         port_count++)
    {
        ohci->rh_status.previous.port_status[port_count] = NU_NULL;
    }

    ohci->rh_status.status_map = NU_NULL;

    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_rh_isr
*
* DESCRIPTION
*
*       This function processes the root-hub-interrupt.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
VOID ohci_rh_isr(NU_USBH_OHCI *ohci)
{
    /* HcRhStatus or HcRhPortStatus[] registers have changed. */

    /* First read the HcRhStatus register. */
    UINT32              read_reg;
    UINT32              report = NU_NULL;
    NU_USB_IRP_CALLBACK callback;
    NU_USB_PIPE        *pipe;
    UINT8                port_num;

    /* Read HcRhStatus Register. */
    OHCI_HW_READ32(ohci,
                   (ohci->cb.controller.base_address +
                    USBH_OHCI_HC_RH_STATUS),
                   read_reg);

    /* Check for the remote wakeup bit and clear it. */
    if (read_reg & USBH_OHCI_RHS_DRWE)
    {
        OHCI_HW_WRITE32(ohci,
                        (ohci->cb.controller.base_address +
                         USBH_OHCI_HC_RH_STATUS),
                        USBH_OHCI_RHS_CLR_RWE);

        read_reg &= ~(USBH_OHCI_RHS_CLR_RWE);
    }

    /* Check for a change. */
    if (read_reg != ohci->rh_status.previous.hub_status)
    {
        /* Update Hub status in the control block. */
        ohci->rh_status.previous.hub_status = read_reg;

        report |=
                OHCI_RH_STATUS_CHANGE_BMP(OHCI_GLOBAL_STATUS_CHANGE_INDEX);
    }

    /* Check for port statuses. */
    for (port_num = 1;
         port_num <= ohci->rh_hub_desc[OHCI_RH_DESC_INDEX_NDP];
         port_num++)
    {
        /* Keep reading port status one by one. */
        OHCI_HW_READ32(ohci,
                       (ohci->cb.controller.base_address +
                        USBH_OHCI_HC_RH_PORT_STATUS +
                        OHCI_GET_PORTSTS_REG_OFFSET(port_num)),
                       read_reg);

        /* Check for a change. */
        if (read_reg != ohci->rh_status.previous.port_status[port_num - 1])
        {
            /* Update Port status in the control block. */
            ohci->rh_status.previous.port_status[port_num - 1] = read_reg;

            /* Update report to store in status map. */
            report |= OHCI_RH_STATUS_CHANGE_BMP(port_num);
        }
    }

    /* If there are any changes then store the value.  */
    if (report != NU_NULL)
    {
        /* This will be sent to the Hub Class driver when it polls next. */
        if (ohci->rh_status.irp != NU_NULL)
        {
            UINT32 *buf;

            NU_USB_IRP_Get_Data(ohci->rh_status.irp, (UINT8 **) &buf);

            *buf = HOST_2_LE32(report);

            NU_USB_IRP_Set_Actual_Length(ohci->rh_status.irp,
                                         sizeof(UINT32));

            ohci->rh_status.status_map = NU_NULL;

            /* IRP is now complete. Update the status and invoke the
             * callback.
             */
            NU_USB_IRP_Set_Status((NU_USB_IRP *) ohci->rh_status.irp,
                                  NU_SUCCESS);

            NU_USB_IRP_Get_Callback((NU_USB_IRP *) (ohci->rh_status.irp),
                                    &callback);

            NU_USB_IRP_Get_Pipe((NU_USB_IRP *)(ohci->rh_status.irp),&pipe);

            if(callback != NU_NULL)
            {
                callback(pipe, (NU_USB_IRP *) ohci->rh_status.irp);
            }

            ohci->rh_status.irp = NU_NULL;
        }
        else
        {
            ohci->rh_status.status_map = report;
        }
    }
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_master_int_en
*
* DESCRIPTION
*
*       This function Sets the MIE bit in HcInterruptEnable
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
VOID ohci_master_int_en(NU_USBH_OHCI *ohci)
{

    if(ohci->int_disable_count)
    {
        ohci->int_disable_count--;
    }

    /* Enable interrupts only when this routine is called as many times as
     * disable interrupt routine is called.
     */
    if(ohci->int_disable_count == NU_NULL)
    {
        /* Set MIE bit in HcInterruptEnable Register. */
        OHCI_HW_WRITE32(ohci,
                        (ohci->cb.controller.base_address +
                         USBH_OHCI_HC_INT_ENABLE),
                        USBH_OHCI_INT_MIE);
    }
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_master_int_dis
*
* DESCRIPTION
*
*       This function sets the MIE bit in HcInterruptDisable
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
*                                           containing root-hub data.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
VOID ohci_master_int_dis(NU_USBH_OHCI *ohci)
{
    /* Set MIE bit in HcInterruptDisable Register. */
    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_INT_DISABLE),
                    USBH_OHCI_INT_MIE);

    /* Keep track of interrupt disable calls. Only the top most
     * enable/disable pair will change interrupts.
     */
    ohci->int_disable_count++;
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_transfer_done
*
* DESCRIPTION
*
*       This function is called on completion of transfer if OHCI
*       allocated an uncachable buffer for handling an IRP.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
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
STATUS ohci_transfer_done(NU_USBH_OHCI    *ohci,
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
*       ohci_normalize_buffer
*
* DESCRIPTION
*
*       This function converts "buffer" into a cache-coherent, aligned
*       memory buffer.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure
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
STATUS ohci_normalize_buffer(NU_USBH_OHCI    *ohci,
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

/**************************************************************************
*
* FUNCTION
*
*       ohci_read32
*
* DESCRIPTION
*
*       This function reads a 32-bit value from a 32-bit memory address.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure.
*       address                             Address of the memory from
*                                           where data is to be read.
*       data_out                            Pointer containing value
*                                           when function returns.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
void ohci_read32(NU_USBH_OHCI    *ohci,
                UINT32          *address,
                UINT32          *data_out)
{
    if(((USBH_OHCI_SESSION_HANDLE *)(ohci->ses_handle))->is_clock_enabled == NU_TRUE)
    {
        *data_out = LE32_2_HOST(ESAL_GE_MEM_READ32(address));
    }
    else
    {
        *data_out = 0;
    }
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_read16
*
* DESCRIPTION
*
*       This function reads a 16-bit value from a 16-bit memory address.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure.
*       address                             Address of the memory from
*                                           where data is to be read.
*       data_out                            Pointer containing value
*                                           when function returns.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
void ohci_read16(NU_USBH_OHCI   *ohci,
                UINT16          *address,
                UINT16          *data_out)
{
    if(((USBH_OHCI_SESSION_HANDLE *)(ohci->ses_handle))->is_clock_enabled == NU_TRUE)
    {
        *data_out =  LE16_2_HOST(ESAL_GE_MEM_READ16(address));
    }
    else
    {
        *data_out = 0;
    }
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_write32
*
* DESCRIPTION
*
*       This function writes a 32-bit value to a 32-bit memory address.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure.
*       address                             Address of the memory
*                                           where data is to be written.
*       data_out                            Value.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
void ohci_write32(NU_USBH_OHCI  *ohci,
                UINT32          *address,
                UINT32          data)
{
    if(((USBH_OHCI_SESSION_HANDLE *)(ohci->ses_handle))->is_clock_enabled == NU_TRUE)
    {
        ESAL_GE_MEM_WRITE32(address, HOST_2_LE32(data));
    }
}

/**************************************************************************
*
* FUNCTION
*
*       ohci_write16
*
* DESCRIPTION
*
*       This function writes a 16-bit value to a 16-bit memory address.
*
* INPUTS
*
*       ohci                                Pointer to the OHCI structure.
*       address                             Address of the memory
*                                           where data is to be written.
*       data                                Value.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
void ohci_write16(NU_USBH_OHCI   *ohci,
                UINT16          *address,
                UINT16          data)
{
    if(((USBH_OHCI_SESSION_HANDLE *)(ohci->ses_handle))->is_clock_enabled == NU_TRUE)
    {
        ESAL_GE_MEM_WRITE16(address, HOST_2_LE16(data));
    }
}

/* ====================  End of File. ================================== */
