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
*        nu_usbh_ehci_ext.c
*
* COMPONENT
*
*       Nucleus USB Host software
*
* DESCRIPTION
*
*       This file contains the exported function definitions for the
*       EHCI driver
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
 *      NU_USBH_EHCI_Create         Creates the Controller Driver
 *      NU_USBH_EHCI_Close_Pipe    Closes a HW Pipe
 *      NU_USBH_EHCI_Create        Creates the Controller Driver
 *      NU_USBH_EHCI_Delete        Delete the Controller Driver
 *      NU_USBH_EHCI_Disable_Int   Disables the interrupts from the EHCI
 *      NU_USBH_EHCI_Enable_Int    Enables the interrupts from the EHCI
 *      NU_USBH_EHCI_Flush_Pipe    Flushes all pending IRPs from a pipe
 *      NU_USBH_EHCI_ISR           EHCI Interrupt service routine
 *      NU_USBH_EHCI_Initialize    Initializes the controller
 *      NU_USBH_EHCI_Modify_Pipe   Alters the nature of a pipe
 *      NU_USBH_EHCI_Open_Pipe     Creates a pipe
 *      NU_USBH_EHCI_Submit_IRP    Submits a transfer on a pipe
 *      NU_USBH_EHCI_Uninitialize  Uninitializes the controller

*
* DEPENDENCIES
*
*       nu_usb.h
*
************************************************************************/

/* ==============  Standard Include Files ============================  */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "drivers/nu_usbh_ehci_ext.h"


/* ====================  Function Definitions ========================= */


/*************************************************************************
* FUNCTION
*
*       NU_USBH_EHCI_Create
*
* DESCRIPTION
*
*       This function initializes the Generic EHCI driver
*
* INPUTS
*
*       cb          EHCI Control Block
*       name        Name of the controller
*       pool        Memory pool for the driver to operate on
*       base_address Memory location in the system memory map for the
*                   controller.
*       vector      Interrupt vector associated with the EHCI.
*
* OUTPUTS
*
*       NU_SUCCESS      indicates that the driver has been initialized
*                       successfully.
*       NU_USB_INVLD_ARG indicates an invalid parameter
*       NU_NOT_PRESENT  indicates a configuration problem because of which
*                       no more USB objects could be created.
*
*************************************************************************/

STATUS NU_USBH_EHCI_Create (NU_USBH_EHCI * cb,
                            CHAR * name,
                            NU_MEMORY_POOL * pool,
                            VOID *base_address,
                            INT vector)
{
    STATUS status = NU_SUCCESS;

    if(cb == NU_NULL
       || pool == NU_NULL
       || base_address == NU_NULL)
        status = NU_USB_INVLD_ARG;

    if(status == NU_SUCCESS)
	{
        status = _NU_USBH_EHCI_Create (cb, name, pool, base_address,
                                  vector,&usbh_ehci_dispatch);
		if ( status == NU_SUCCESS )
		{
			cb->cacheable_pool = pool;
		}
	}

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_EHCI_Create2
*
* DESCRIPTION
*
*       This function initializes the Generic EHCI driver
*
* INPUTS
*
*       cb                  EHCI Control Block
*       name                Name of the controller
*       uncacheable_pool    Non cacheable memory pool for the
*                           driver to operate on.
*       cacheable_pool      Cacheable memory pool for the driver
*                           to operate on.
*       base_address        Memory location in the system memory map for the
*                           controller.
*       vector              Interrupt vector associated with the EHCI.
*
* OUTPUTS
*
*       NU_SUCCESS          indicates that the driver has been initialized
*                           successfully.
*       NU_USB_INVLD_ARG    indicates an invalid parameter
*       NU_NOT_PRESENT      indicates a configuration problem because of which
*                           no more USB objects could be created.
*
*************************************************************************/

STATUS NU_USBH_EHCI_Create2 (NU_USBH_EHCI * cb,
                            CHAR * name,
                            NU_MEMORY_POOL *uncacheable_pool,
                            NU_MEMORY_POOL *cacheable_pool,
                            VOID *base_address,
                            INT vector)
{
    STATUS status = NU_SUCCESS;

    if(cb == NU_NULL
       || uncacheable_pool == NU_NULL
       || cacheable_pool == NU_NULL
       || base_address == NU_NULL)
        status = NU_USB_INVLD_ARG;

    if(status == NU_SUCCESS)
	{
        status = _NU_USBH_EHCI_Create (cb, name, uncacheable_pool, base_address,
                                       vector,&usbh_ehci_dispatch);

		if(status == NU_SUCCESS)
		{
			cb->cacheable_pool	= cacheable_pool;
			cb->uncachable_pool	= uncacheable_pool;
		}
	}

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBH_EHCI_Create
*
* DESCRIPTION
*
*       This function initializes the Generic EHCI driver
*
* INPUTS
*
*       cb          EHCI Control Block
*       name        Name of the controller
*       pool        Memory pool for the driver to operate on
*       base_address Memory location in the system memory map for the
*                   controller.
*       vector      Interrupt vector associated with the EHCI.
*       dispatch    Dispatch table for the controller.
*
* OUTPUTS
*
*       NU_SUCCESS      indicates that the driver has been initialized
*                       successfully.
*       NU_USB_INVLD_ARG indicates an invalid parameter
*       NU_NOT_PRESENT  indicates a configuration problem because of which
*                       no more USB objects could be created.
*
*************************************************************************/

STATUS _NU_USBH_EHCI_Create (NU_USBH_EHCI * cb,
                             CHAR * name,
                             NU_MEMORY_POOL * pool,
                             VOID *base_address,
                             INT vector,
                             const VOID *dispatch)
{
    STATUS status = NU_SUCCESS;

    memset((VOID *)cb, 0, sizeof(NU_USBH_EHCI));

    status = _NU_USBH_HW_Create ((NU_USBH_HW *) cb, name, pool, 1,
                                 USB_SPEED_HIGH, base_address, vector,
                                 dispatch);
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBH_EHCI_Delete
*
* DESCRIPTION
*
*        This function uninitializes the driver.
*
* INPUTS
*
*        cb         EHCI driver to be uninitialized.
*
* OUTPUTS
*
*        NU_SUCCESS always.
*
*************************************************************************/
STATUS _NU_USBH_EHCI_Delete (VOID *cb)
{
    return _NU_USBH_HW_Delete (cb);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_EHCI_Initialize
*
* DESCRIPTION
*
*       Generic EHCI Initialization function
*
* INPUTS
*
*       cb      EHCI Control block
*       stack   Stack object associated with the hw
*
* OUTPUTS
*
*       NU_SUCCESS      indicates successful initialization of the hw
*       NU_USB_INVLD_ARG indicates an invalid parameter
*       NU_NO_MEMORY    indicates that enough memory doesn't exist.
*
*************************************************************************/
STATUS NU_USBH_EHCI_Initialize (NU_USB_HW * cb,
                                 NU_USB_STACK * stack)
{

    STATUS status;
    NU_USBH_EHCI *ehci = (NU_USBH_EHCI *) cb;

    UINT32 temp;
    UINT16 rst_count = 0;

    /* Initialize saved interrupt status. */
    ehci->intr_status = 0;

    /* Get the capability ad operational bases */

    ehci->ehci_capreg_base = (UINT8 *) (ehci->cb.controller.base_address);

    EHCI_HW_READ32 (ehci, ehci->ehci_capreg_base + CAPLENGTH, temp);

    ehci->ehci_oprreg_base = (ehci->ehci_capreg_base + (UINT8)(temp));

    /* check HC version */

    if ((temp >> 16) < 0x0095)
        return NU_USB_NOT_SUPPORTED;

    /* Subject the controller to a software reset */

    EHCI_HW_WRITE32 (ehci, ehci->ehci_oprreg_base + USBCMD,
                     ((0x1 << 16) | CMD_HC_RESET));
                     
    /* Wait for host controller to reset. */
    do
    {
        EHCI_HW_READ32 (ehci, ehci->ehci_oprreg_base + USBCMD,
                         temp);
        rst_count++;
    }while((temp & CMD_HC_RESET) && (rst_count < USBH_EHCI_MAX_RST_ITERATIONS));

    if (rst_count == USBH_EHCI_MAX_RST_ITERATIONS )
    {
        return ( NU_USB_INTERNAL_ERROR );
    }

    /* Allocate memory for periodic frame list, aligned at 4k boundary */

    status = NU_Allocate_Aligned_Memory(ehci->cb.pool,
                                    (VOID **) &(ehci->periodic_list_base),
                                    USBH_EHCI_FRAME_LIST_SIZE * 4,
                                    (4*1024),
                                    NU_NO_SUSPEND);


    if (status != NU_SUCCESS)
    {
        return status;
    }

    /* Set T bit to 1 throughout the Periodic List */

    memset (ehci->periodic_list_base, 1,
            (USBH_EHCI_FRAME_LIST_SIZE * sizeof (UINT32)));

    /* Frame load is zero, to start with for each frame */

    memset (ehci->frame_load, 0, sizeof (ehci->frame_load));

    ehci->num_active_qh_periodic = 0;

    /* Set the periodic list base address */

    EHCI_WRITE_ADDRESS (ehci, (ehci->ehci_oprreg_base + PERIODICLISTBASE),
                              (UINT32)(ehci->periodic_list_base));

    /* Async QHs */

    /* Create a dummy QH structure */

    status = ehci_alloc_qh (ehci, &(ehci->dummy_qh), NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_Deallocate_Memory (ehci->periodic_list_base);
        return ( status );
    }

    /* Prepare the Dummy QH */

    memset (ehci->dummy_qh, 0, sizeof (USBH_EHCI_QH));

    /* Set type field */
    EHCI_WRITE_ADDRESS(ehci, &(ehci->dummy_qh->next),
                       (((UINT32) ehci->dummy_qh) | 0x02));


    EHCI_WRITE_ADDRESS(ehci, &(ehci->dummy_qh->next_qtd), 0x01);


    EHCI_HW_WRITE32(ehci, &(ehci->dummy_qh->ep_controlinfo1), (0x1 << 15));


    ehci->dummy_qh->info = NU_NULL;
    ehci->dummy_qh->prev = ehci->dummy_qh;  /* circularly linked */

    /* Set the Async list base address */
    EHCI_WRITE_ADDRESS (ehci, ehci->ehci_oprreg_base + ASYNCLISTADDR,
                           (UINT32) ehci->dummy_qh);

    /* Enable periodic and async list */

    EHCI_HW_SET_BITS (ehci, ehci->ehci_oprreg_base + USBCMD,
                      (CMD_ASYNC_ENABLE | CMD_PERIODIC_ENABLE));

    /* write a 1 to CONFIGFLAG reg */
    EHCI_HW_WRITE32 (ehci, ehci->ehci_oprreg_base + CONFIGFLAG, FLAG_CF);

    /* Initialize root hub */
    ehci_rh_init (ehci);

    /* Call parent's initialize routine to install LISR */
    _NU_USBH_HW_Initialize (cb, stack);

    /* Create a lock to protect internal lists of EHCI */
    status = NU_Create_Semaphore(&ehci->protect_sem, "EHCISEM", 1, NU_PRIORITY);


    if (status != NU_SUCCESS)
    {
        NU_USBH_EHCI_Uninitialize ((NU_USB_HW *) ehci);

        return ( status );
    }


    /* Start the EHCI */

    EHCI_HW_SET_BITS (ehci, ehci->ehci_oprreg_base + USBCMD, CMD_HC_RUN);


    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_EHCI_Uninitialize
*
* DESCRIPTION
*
*       This function uninitializes the EHCI hardware
*
* INPUTS
*
*       cb      EHCI controller to be uninitialized
*
* OUTPUTS
*
*       NU_SUCCESS  always.
*
*************************************************************************/

STATUS NU_USBH_EHCI_Uninitialize (NU_USB_HW * cb)
{
	STATUS status = NU_SUCCESS;
	UINT8 pool_count = 0;
	EHCI_ALIGNED_MEM_POOL   *pool;
	
    NU_USBH_EHCI *ehci = (NU_USBH_EHCI *) cb;

    status = NU_USBH_EHCI_Disable_Int (cb);

	if (status == NU_SUCCESS)
	{
	    /* Call parent's uninitialize routine to de install LISR */
	    _NU_USBH_HW_Uninitialize (cb);

	    NU_Deallocate_Memory(ehci->periodic_list_base);

	    status = ehci_dealloc_qh (ehci, ehci->dummy_qh);

		if (status == NU_SUCCESS)
		{
			for ( pool_count = 0; pool_count < USBH_EHCI_MAX_QH_POOLS; pool_count++)
			{
			    /* Get the control block of the pool. */
                pool = &(ehci->qh_pool[pool_count]); 

				if (pool->start_ptr)
				{				
					 NU_Deallocate_Memory(pool->start_ptr);         					                                                                    
                     memset(pool, 0, sizeof(EHCI_ALIGNED_MEM_POOL));
				}
			}
		}
        /* Stop the controller */
        EHCI_HW_CLR_BITS (ehci, ehci->ehci_oprreg_base + USBCMD, (CMD_HC_RUN));

        status = NU_Delete_Semaphore(&ehci->protect_sem);
        memset(&ehci->protect_sem,0x00,sizeof(NU_SEMAPHORE));
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_EHCI_Enable_Int
*
* DESCRIPTION
*
*       This function enables the interrupts from the hardware
*
* INPUTS
*
*       cb      EHCI controller to enable the interrupts for
*
* OUTPUTS
*
*       NU_SUCCESS  always.
*
*************************************************************************/
STATUS NU_USBH_EHCI_Enable_Int (NU_USB_HW * cb)
{
    NU_USBH_EHCI *ehci = (NU_USBH_EHCI *) cb;
    
    /* Enable the EHCI Interrupts */
    USBH_EHCI_UNPROTECT;

    return (NU_SUCCESS);
}


/*************************************************************************
* FUNCTION
*
*       NU_USBH_EHCI_Disable_Int
*
* DESCRIPTION
*
*       This function disables the interrupts from the hardware
*
* INPUTS
*
*       cb      EHCI controller to disable the interrupts for
*
* OUTPUTS
*
*       NU_SUCCESS  always.
*
*************************************************************************/

STATUS NU_USBH_EHCI_Disable_Int (NU_USB_HW * cb)
{
    NU_USBH_EHCI *ehci = (NU_USBH_EHCI *) cb;
    UINT32       intr_en_reg = 0;
    UINT32       intr_sts_reg = 0;

    /* Checks are placed to see if EHCI has generated an interrupt.
     * This is required if multiple host controllers are present
     * and they are using a shared interrupt line.
     */

    /* Read USB Interrupt Enable register */
    EHCI_HW_READ32 (ehci,ehci->ehci_oprreg_base + USBINTR, intr_en_reg);

    /* Check if this EHCI can generate an interrupt */
    if(intr_en_reg & USBH_EHCI_INTR_ENABLE_MASK)
    {
        /* Disable the interrupts to further process the interrupts. */
        USBH_EHCI_PROTECT;

        /* Read USB Status register */
        EHCI_HW_READ32 (ehci, ehci->ehci_oprreg_base + USBSTS, intr_sts_reg);

        /* Check if EHCI has generated an interrupt */
        if(intr_en_reg & intr_sts_reg)
        {

            /* Store pending interrupts in the control block. */
            ehci->intr_status = intr_sts_reg ;

            /* Clear the interrupts */
            EHCI_HW_WRITE32 (ehci, ehci->ehci_oprreg_base + USBSTS,
                             (intr_sts_reg & intr_en_reg));
        }
    }

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_EHCI_ISR
*
* DESCRIPTION
*
*       This function processes the interrupt from the EHCI controller
*
* INPUTS
*
*       cb      EHCI controller which generated the interrupt
*
* OUTPUTS
*
*       NU_SUCCESS  always.
*
*************************************************************************/
STATUS NU_USBH_EHCI_ISR (NU_USB_HW * cb)
{

    NU_USBH_EHCI *ehci = (NU_USBH_EHCI *) cb;
    volatile UINT32 intr_stat;

    intr_stat = ehci->intr_status;

    /* Process only required set of interrupts */

    intr_stat &= USBH_EHCI_INTR_ENABLE_MASK;

    if (intr_stat)
    {
        /* Set saved interrupt status to zero, as all the interrupts
         * are going to be serviced here .
         */
        ehci->intr_status = 0;

        /* Port status change interrupt ??  */

        if (intr_stat & EHCI_STS_PORT_CHANGE)
        {
            ehci_rh_isr (ehci);
        }

        /* Transfer complete, or Td retired interrupt
         * Or an error-during-transfer interrupt
         */

        if ((intr_stat & EHCI_STS_INTERRUPT) ||
            (intr_stat & EHCI_INTR_ERR_INTERRUPT))
        {
            ehci_scan_async_list (ehci);
            ehci_scan_periodic_list (ehci);

        }

        /* Unrecoverable Host System Error. */

        if (intr_stat & EHCI_INTR_HOST_ERROR)
        {
            NU_ASSERT (0);
        }
    }

    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_EHCI_Submit_IRP
*
* DESCRIPTION
*
*       This function accepts a transfer to a given endpoint on a given
*       device. This function checks to see if the device is a root hub.If
*       so appropriate root hub Irp processing function is invoked.
*       Otherwise appropriate list is found for this transfer and is
*       added to the list.
*
* INPUTS
*
*       cb                  EHCI controller for transfer
*       irp                 Data transfer description
*       function_address    Device (address) to which the transfer is
*                           directed.
*       bEndpointAddress    Endpoint on the device for which this transfer
*                           is meant for.
*
* OUTPUTS
*
*       NU_SUCCESS           indicates a successful submission of data
*                            transfer
*       NU_USB_INVLD_ARG     indicates an invalid parameter.
*       NU_INVALID_SEMAPHORE Indicates the semaphore pointer is invalid.
*       NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                            was suspended.
*       NU_UNAVAILABLE       Indicates the semaphore is unavailable.
*
*************************************************************************/
STATUS NU_USBH_EHCI_Submit_IRP (NU_USB_HW * cb,
                                 NU_USB_IRP * irp,
                                 UINT8 function_address,
                                 UINT8 bEndpointAddress)
{

    NU_USBH_EHCI *ehci = (NU_USBH_EHCI *) cb;
    STATUS status;
    UINT32 key;
    USBH_EHCI_QH_INFO *qhinfo;

    /* Is the IRP meant for root hub ? */

    if (function_address == USB_ROOT_HUB)
    {
        status = ehci_rh_handle_irp (ehci, irp, bEndpointAddress);
        return ( status );
    }

    /* Non roothub data transfers. Obtain the necessary lock */
    status = NU_Obtain_Semaphore(&ehci->protect_sem, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        return ( status );
    }

    /* Find the QH corresponding to this endpoint in the Hash table */

    key = ehci_qh_key (function_address, bEndpointAddress);

    status = NU_USB_INVLD_ARG;

    qhinfo = hash_find_qh (ehci->qh_async_ht, key);

    if (qhinfo != NU_NULL)
    {
        /* To start with, IRP's actual length is 0  */

        NU_USB_IRP_Set_Actual_Length (irp, 0);

        /* We found the corresponding QH        */
        /* Check if any transfer is ongoing */
        if (qhinfo->type != USB_EP_ISO)
        {
            USBH_EHCI_PROTECT;
            if (qhinfo->qtd_head != NU_NULL)
            {
                if (qhinfo->type != USB_EP_CTRL)
                {
                    /* Another transfer is on going. Pend the IRP */
                    irp->node.cs_next = NU_NULL;
                    irp->node.cs_previous = NU_NULL;
                    NU_Place_On_List ((CS_NODE **) & (qhinfo->pend_irp),
                                       (CS_NODE *) irp);
                    status = NU_SUCCESS;
                    USBH_EHCI_UNPROTECT;
                }
                else
                {
                    /* No pipelining of transfers for control endpoints */
                    status = NU_NO_MEMORY;
                    USBH_EHCI_UNPROTECT;
                }
            }
            else
            {
                /* Process the IRP */
                status = ehci_qh_handle_irp (ehci, qhinfo, irp);
				
                USBH_EHCI_UNPROTECT;
            }
        }
        else
        {

            status = ehci_handle_iso_irp(ehci, qhinfo, irp);

        }
    }

    NU_Release_Semaphore(&(ehci->protect_sem));


    return ( status );
}

/************************************************************************
*
* FUNCTION
*
*       NU_USBH_EHCI_Flush_Pipe
*
* DESCRIPTION
*
*   Removes all pending TDs from HC's schedule list. Their corresponding
*   IRP callbacks are notified of this transfer termination.
*
* INPUTS
*
*       cb                  EHCI Control block
*       function_address    Device (address) to which the transfers must
*                           be flushed
*       bEndpointAddress    Endpoint on the device for which the transfers
*                           must be flushed
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates that the TDs termination has
*                               been successfully initiated.
*       NU_USB_INVLD_ARG        Indicates that ehci ptrs passed are
*                               NU_NULL.
*       NU_INVALID_SEMAPHORE    Indicates the semaphore pointer is
*                               invalid.
*       NU_SEMAPHORE_DELETED    Semaphore was deleted while the task
*                               was suspended.
*       NU_UNAVAILABLE          Indicates the semaphore is unavailable.
*
*************************************************************************/


STATUS NU_USBH_EHCI_Flush_Pipe (NU_USB_HW * cb,
                                 UINT8 function_address,
                                 UINT8 bEndpointAddress)
{

    NU_USBH_EHCI *ehci = (NU_USBH_EHCI *) cb;
    UINT32 key;
    STATUS status;
    USBH_EHCI_QH_INFO *qhinfo;

    NU_ASSERT (ehci != NU_NULL);

    /* There is no flush pipe for roothub. because no pipes are
     * established for root hub. So return success
     * */

    if (function_address == USB_ROOT_HUB)
        return NU_SUCCESS;

    /* Multi-thread safety to QH/QTD hash table */

    status = NU_Obtain_Semaphore(&ehci->protect_sem, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        return ( status );
    }

    /* Find the QH to be removed */
    key = ehci_qh_key (function_address, bEndpointAddress);

    qhinfo = hash_find_qh (ehci->qh_async_ht, key);

    if (qhinfo == NU_NULL)
    {
        NU_Release_Semaphore(&(ehci->protect_sem));
        return NU_NOT_PRESENT;
    }

    if(qhinfo->type == USB_EP_ISO)
    {
        if(qhinfo->speed == USB_SPEED_FULL)
        {
            status = ehci_retire_fs_iso_transfer(ehci,
                                 qhinfo,
                                 NU_NULL,
                                 0,
                                 USBH_EHCI_ITD_CANCELLED
                                 );
        }
        else
        {
            status = ehci_retire_hs_iso_transfer(ehci,
                                 qhinfo,
                                 NU_NULL,
                                 0,
                                 USBH_EHCI_ITD_CANCELLED);
        }
    }
    /* Flush only if this pipe is scheduled and with a transfer on it . */
    else if ((qhinfo->qh_state == USBH_EHCI_QH_READY) &&
        (qhinfo->qtd_head != NU_NULL))
    {
        /* Flush only if this pipe is scheduled and with a transfer on it . */
        /* Unlink the async QH */

        if ((qhinfo->type == USB_EP_CTRL) || (qhinfo->type == USB_EP_BULK))
        {
            ehci_unlink_async_qh (ehci, qhinfo);
        }

        /* Retire the transfers */

        ehci_retire_qh (ehci, qhinfo, NU_USB_IRP_CANCELLED);
        qhinfo->qtd_head = NU_NULL;

        /* Link back the async QH */

        if ((qhinfo->type == USB_EP_CTRL) || (qhinfo->type == USB_EP_BULK))
        {
            /* Link the QH back */
            ehci_link_async_qh (ehci, qhinfo);
        }
    }

    NU_Release_Semaphore(&(ehci->protect_sem));

    return NU_SUCCESS;

}

/************************************************************************
*
* FUNCTION
*
*       NU_USBH_EHCI_Open_Pipe
*
* DESCRIPTION
*
*       Creates endpoint descriptors in the H/W and makes them ready for
*       scheduling transfers.
*
* INPUTS
*
*       cb                handle that identifies the HC in the EHCI HC
*                         database.
*       function_address  identifies the device on which the pipe resides
*       bEndpointAddress  identifies the endpoint which owns the pipe
*       bmAttributes      identifies the endpoint type.
*       speed             Device speed - USB_SPEED_LOW/USB_SPEED_FULL.
*       ep_max_size       Maximum pkt size the endpoint should support.
*       interval          interval in micro seconds for periodic endpoints.
*       load              frame b/w consumed by the endpoint.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates successful creation of the pipe
*                               in h/w.
*       NU_INVALID_SEMAPHORE    Indicates the semaphore pointer is
*                               invalid.
*       NU_SEMAPHORE_DELETED    Semaphore was deleted while the task
*                               was suspended.
*       NU_UNAVAILABLE          Indicates the semaphore is unavailable.
*       NU_USB_INVLD_ARG        Indicates that the speed is passed an
*                               invalid value.
*
*************************************************************************/
STATUS NU_USBH_EHCI_Open_Pipe (NU_USB_HW * cb,
                                UINT8 function_address,
                                UINT8 bEndpointAddress,
                                UINT8 bmAttributes,
                                UINT8 speed,
                                UINT16 ep_max_size,
                                UINT32 interval,
                                UINT32 load)
{
    NU_USBH_EHCI *ehci = (NU_USBH_EHCI *) cb;
    STATUS status;
    USBH_EHCI_QH_INFO *qhinfo;
    USBH_EHCI_ISO_QH_INFO   *iso_qhinfo;
    UINT32 qh_control_info1 = 0, qh_control_info2 = 0;
    UINT8 rollback = 0;
    UINT8 pipe_type = bmAttributes & 3;
    UINT8 endpoint = bEndpointAddress & 0xf;
    UINT32 key = 0;
    UINT32 token = 0;
    USBH_EHCI_ITD  *itd;
    USBH_EHCI_ITD  *itd_array_start;
    USBH_EHCI_SITD *sitd;
    USBH_EHCI_SITD  *sitd_array_start;
    UINT32 i = 0,count = 0;

    NU_ASSERT (ehci != NU_NULL);

    NU_ASSERT (speed != USB_SPEED_UNKNOWN);

    /* No pipes need to established for transfers to Root hub */
    if (function_address == USB_ROOT_HUB)
        return NU_SUCCESS;

    /* Multi-thread safety to ed_hash_table/td_hash_table */
    status = NU_Obtain_Semaphore(&ehci->protect_sem,NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        return status;
    }

    /* check if already a pipe exists for this endpoint */

    key = ehci_qh_key (function_address, bEndpointAddress);

    qhinfo = hash_find_qh (ehci->qh_async_ht, key);

    if (qhinfo != NU_NULL)
    {
        /* a QH already exists for this endpoint */

        NU_ASSERT (0);

        /* Unlock mutex before returning */
        NU_Release_Semaphore(&(ehci->protect_sem));
        return NU_USB_INVLD_ARG;
    }

    if(pipe_type != USB_EP_ISO)
    {

        USBH_EHCI_PROTECT;

        /* Create a s/w Qhinfo structure */
        status = NU_Allocate_Memory (ehci->cacheable_pool, (VOID **) &qhinfo,
                               sizeof (USBH_EHCI_QH_INFO), NU_NO_SUSPEND);

        USBH_EHCI_UNPROTECT;

        if (status != NU_SUCCESS)
            rollback = 1;

        if (!rollback)
        {
            /* Reset the QH info structure  */
            memset (qhinfo, 0, sizeof (USBH_EHCI_QH_INFO));
        }

        USBH_EHCI_PROTECT;

        status = ehci_alloc_qh (ehci, &(qhinfo->hwqh), NU_NO_SUSPEND);

        USBH_EHCI_UNPROTECT;

        if (status != NU_SUCCESS)
            rollback = 2;

        if(!rollback)
        {
            memset (qhinfo->hwqh, 0, sizeof(USBH_EHCI_QH));
            qhinfo->hwqh->info = qhinfo;
        }
    }
    else
    {
        /* Create a s/w Iso-qhinfo structure */
        USBH_EHCI_PROTECT;

        status = NU_Allocate_Memory (ehci->cacheable_pool, (VOID **) &iso_qhinfo,
                               sizeof (USBH_EHCI_ISO_QH_INFO), NU_NO_SUSPEND);
        USBH_EHCI_UNPROTECT;

        if (status != NU_SUCCESS)
            rollback = 1;

        if (!rollback)
        {
            /* Reset the Iso-Qh info structure  */
            memset (iso_qhinfo, 0, sizeof (USBH_EHCI_ISO_QH_INFO));
            qhinfo = (USBH_EHCI_QH_INFO *)iso_qhinfo;
        }

    }

    if(!rollback)
    {
        /* create the dummy qtd for bulk IN transfers */

        if ((pipe_type == USB_EP_BULK) && (bEndpointAddress & 0x80))
        {
            USBH_EHCI_PROTECT;
            status = ehci_alloc_qtd (ehci, &qhinfo->dummy_qtd,
                                     NU_NO_SUSPEND);
            USBH_EHCI_UNPROTECT;

            if (status != NU_SUCCESS)
            {
                rollback = 3;
            }

            if(!rollback)
            {
                /* fill the qtd */

                memset (qhinfo->dummy_qtd, 0, sizeof (USBH_EHCI_QTD));

                EHCI_HW_WRITE32(ehci, &qhinfo->dummy_qtd->next, 0x01);

                EHCI_HW_WRITE32(ehci, &qhinfo->dummy_qtd->alt_next, 0x01);

                token = (0x03 << 10);

                if(bEndpointAddress & 0x80)
                    token |= (0x1 << 8);

                EHCI_HW_WRITE32(ehci, &qhinfo->dummy_qtd->token, token);
            }
        }
    }

    if (!rollback)
    {

        qhinfo->qh_state = USBH_EHCI_QH_NOT_READY;

        /* Adjust interval of periodic pipes to the closest 2**n value */

        if ((pipe_type == USB_EP_INTR) || (pipe_type == USB_EP_ISO))
        {
            qhinfo->interval = (interval/125L);   /* Micro Frames */
        }


        /* As EHCI operates in High Speed, adjusting the load for full
         * speed and low speed devices.
         */
        if (speed == USB_SPEED_FULL)
        {
            qhinfo->load = (load >> 5) + 1;
        }
        else if (speed == USB_SPEED_LOW)
        {
            qhinfo->load = (load >> 8) + 1;
        }
        else
        {
            qhinfo->load = load;
        }

        qhinfo->speed = speed;
        qhinfo->func_address = function_address;
        qhinfo->bEndpointAddress = bEndpointAddress;
        qhinfo->type = pipe_type;
        qhinfo->maxpacket = ep_max_size;
        qhinfo->address = function_address;
        qhinfo->address |= bEndpointAddress << 8;


        if(pipe_type != USB_EP_ISO)
        {
            /* The following are the QH words */
            /* Device Address  */
            qh_control_info1 |= (function_address & 0x7F);
            qh_control_info1 |= (endpoint & 0x0F) << 8;    /* Endpoint    */

            /* Endpoint Speed */
            if (speed == USB_SPEED_HIGH)    /* EPS      */
            {
                qh_control_info1 |= (2 << 12);
            }
             else if (speed == USB_SPEED_LOW)
             {
                 qh_control_info1 |= (1 << 12);
             }

             if (pipe_type == USB_EP_CTRL)
             {
                 /* Data toggle carry from Incoming qtd */
                 qh_control_info1 |= (1 << 14);  /* dtc      */
             }

             /* H bit will be set while scheduling the pipe */

             qh_control_info1 |= (ep_max_size << 16); /* Maximum Packet Size */

             if ((speed != USB_SPEED_HIGH) && (pipe_type == USB_EP_CTRL))
             {
                 /* non high speed Control end point */
                 qh_control_info1 |= (1 << 27);  /* C */
             }

             /* One transaction per micro frame, will be updated later if
              * required */

             qh_control_info2 |= (1 << 30);  /* Mult */

             EHCI_HW_WRITE32(ehci, &(qhinfo->hwqh->ep_controlinfo1),
                             qh_control_info1);

             EHCI_HW_WRITE32(ehci, &(qhinfo->hwqh->ep_controlinfo2),
                             qh_control_info2);

             EHCI_HW_WRITE32(ehci, &(qhinfo->hwqh->next_qtd), 1);

             EHCI_HW_WRITE32(ehci, &(qhinfo->hwqh->alt_next_qtd), 1);
        }
        else
        {
            if(qhinfo->speed == USB_SPEED_HIGH)
            {

                /* Device Address  */
                qh_control_info1 |= (function_address & 0x7F);
                qh_control_info1 |= (endpoint & 0x0F) << 8;    /* Endpoint */
                qh_control_info2 |=  ep_max_size;

                if(qhinfo->bEndpointAddress & USB_DIR_IN)
                    qh_control_info2 |= (1 << 11); /* IN/OUT */

                status = NU_Allocate_Aligned_Memory(ehci->cb.pool,
                     (VOID **)&itd_array_start,
                     (USBH_EHCI_MAX_ITDS_PER_IRP * sizeof(USBH_EHCI_ITD)),
                      USBH_EHCI_QH_TD_ALIGNMENT,
                      NU_NO_SUSPEND);

                if(status != NU_SUCCESS)
                    rollback = 2;

                if(!rollback)
                {
                    iso_qhinfo->td_array_start = itd_array_start;

                    /* Reset all the TDs. */
                    memset(itd_array_start,
                    0,
                    (USBH_EHCI_MAX_ITDS_PER_IRP * sizeof(USBH_EHCI_ITD)));

                    for(count = 0; count < USBH_EHCI_MAX_ITDS_PER_IRP; count++)
                    {
                        itd = &itd_array_start[count];
                        itd->qhinfo = iso_qhinfo;

                        EHCI_HW_WRITE32(ehci,&(itd->buffer[0]),qh_control_info1 & 0x0fff);
                        EHCI_HW_WRITE32(ehci,&(itd->buffer[1]),qh_control_info2 & 0x0fff);
                        /* Setting Mult as 1 */
                        EHCI_HW_WRITE32(ehci,&(itd->buffer[2]),0x00000001);
                    }
                }
            }
            else
            {
                 status = NU_Allocate_Aligned_Memory(ehci->cb.pool,
                     (VOID **)&sitd_array_start,
                     (USBH_EHCI_MAX_ITDS_PER_IRP * sizeof(USBH_EHCI_SITD)),
                      USBH_EHCI_QH_TD_ALIGNMENT,
                      NU_NO_SUSPEND);

                if(status != NU_SUCCESS)
                    rollback = 2;

                if(rollback == 0)
                {
                    iso_qhinfo->td_array_start = sitd_array_start;

                    /* Reset all the TDs. */
                    memset(sitd_array_start,
                     0,
                    (USBH_EHCI_MAX_ITDS_PER_IRP * sizeof(USBH_EHCI_SITD)));


                    for(count = 0; count < USBH_EHCI_MAX_ITDS_PER_IRP; count++)
                    {
                        sitd = &sitd_array_start[count];
                        sitd->qhinfo = iso_qhinfo;

                    }
                }

            }

            /* Add QH to the list of active periodic QHs */
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

		if (!rollback)
		{
        	qhinfo->key = ehci_qh_key (function_address, bEndpointAddress);

        	/* Add the QH to the corresponding Hashtable    */
        	status = hash_add_qh (ehci, ehci->qh_async_ht, qhinfo);

			if (status != NU_SUCCESS)
				rollback = 4;

			if(!rollback)
			{
        		status = NU_SUCCESS;

        		rollback = 1;
			}	
		}			

    }

    switch (rollback)
    {
    	case 4:
			if (pipe_type == USB_EP_ISO)
				NU_Deallocate_Memory(iso_qhinfo->td_array_start);
        case 3:    
			if (pipe_type != USB_EP_ISO)
            status = ehci_dealloc_qh(ehci, qhinfo->hwqh);            
        case 2:            
            NU_Deallocate_Memory (qhinfo);            
        case 1:
            NU_Release_Semaphore(&(ehci->protect_sem));
            return status;
    }

    return NU_SUCCESS;
}

/************************************************************************
* FUNCTION
*
*       NU_USBH_EHCI_Close_Pipe
*
* DESCRIPTION
*
*       Terminates pending TDs and releases the ED. The corresponding
*       callbacks are later notified of their canceled TDs.
*
* INPUTS
*
*       cb                handle that identifies the HC in the EHCI HC
*                         database.
*       function_address  identifies the device on which the pipe resides
*       bEndpointAddress  identifies the endpoint which owns the pipe
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates that the pipe close is
*                               successfully initiated
*       NU_USB_INVLD_ARG        Indicates that pipe or EHCI ptrs passed
*                               are NU_NULL.
*       NU_INVALID_SEMAPHORE    Indicates the semaphore pointer is
*                               invalid.
*       NU_SEMAPHORE_DELETED    Semaphore was deleted while the task
*                               was suspended.
*       NU_UNAVAILABLE          Indicates the semaphore is unavailable.
*
*************************************************************************/
STATUS NU_USBH_EHCI_Close_Pipe (NU_USB_HW * cb,
                                 UINT8 function_address,
                                 UINT8 bEndpointAddress)
{

    STATUS status;

    NU_USBH_EHCI *ehci = (NU_USBH_EHCI *) cb;

    UINT32 key = ehci_qh_key (function_address, bEndpointAddress);

    USBH_EHCI_QH_INFO *qhinfo;

    USBH_EHCI_QH *qh;

    UINT32 i , n;



    /* No pipes need to closed for Root hub */

    if (function_address == USB_ROOT_HUB)
        return NU_SUCCESS;

    /* Multi-thread safety to ed_hash_table/td_hash_table */

    status = NU_Obtain_Semaphore(&ehci->protect_sem, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        return status;
    }

    /* Get the qhinfo for this pipe . */

    qhinfo = hash_find_qh (ehci->qh_async_ht, key);

    /* Check if this QH is already closed */

    if (qhinfo == NU_NULL)
    {
        NU_Release_Semaphore(&(ehci->protect_sem));
        return NU_SUCCESS;
    }

    qh = (USBH_EHCI_QH *) ((UINT32) (qhinfo->hwqh) & (~0x1F));

    /* Closing is required only if the pipes are scheduled */

    if (qhinfo->qh_state == USBH_EHCI_QH_READY)
    {
        /* the way of closing pipes is different between Async and
         * Periodic pipes. */

        if ((qhinfo->type == USB_EP_CTRL) || (qhinfo->type == USB_EP_BULK))
        {
            /* Async QH. */
            ehci_unlink_async_qh (ehci, qhinfo);
        }
        else
        {
            EHCI_HW_CLR_BITS (ehci, ehci->ehci_oprreg_base + USBCMD,
                             (1 << 4));

            usb_wait_ms(10);
            if(qhinfo->type == USB_EP_INTR)
            {
                /* Periodic QH */
                ehci_unlink_periodic_qh (ehci, qhinfo);

            }
            else if(qhinfo->type == USB_EP_ISO)
            {
                if(qhinfo->speed == USB_SPEED_FULL)
                {
                    status = ehci_retire_fs_iso_transfer(ehci,
                                 qhinfo,
                                 NU_NULL,
                                 0,
                                 USBH_EHCI_ITD_CANCELLED
                                );
                }
                else
                {
                    status = ehci_retire_hs_iso_transfer(ehci,
                                 qhinfo,
                                 NU_NULL,
                                 0,
                                 USBH_EHCI_ITD_CANCELLED);
                }

                ehci_delete_iso_tds((USBH_EHCI_ISO_QH_INFO*)qhinfo);

                /* Periodic QH */
                ehci_unlink_periodic_qh (ehci, qhinfo);
            }
            EHCI_HW_SET_BITS (ehci, ehci->ehci_oprreg_base + USBCMD,
                             (1 << 4));

            for (i = 0; i < ehci->num_active_qh_periodic; i++)
            {
                if (ehci->active_qh_periodic[i] == qhinfo)
                {
                    ehci->num_active_qh_periodic--;
                    for (n = i; n < ehci->num_active_qh_periodic; n++)
                    {
                        ehci->active_qh_periodic[n] =
                            ehci->active_qh_periodic[n + 1];
                    }
                    ehci->active_qh_periodic[ehci->num_active_qh_periodic]
                    = NU_NULL;
                }
            }

        }

        /* Check if there are any pending transfers and retire them * */

        if (qhinfo->qtd_head != NU_NULL)
        {
            ehci_retire_qh (ehci, qhinfo, NU_USB_IRP_CANCELLED);
            qhinfo->qtd_head = NU_NULL;
        }

    }

    /* Remove the QH from the hash table */

    status = hash_delete_qh (ehci, qhinfo);

	if (status == NU_SUCCESS)
	{	
	    /* delete the dummy qTd */
	    if((qhinfo->type == USB_EP_BULK) && (qhinfo->bEndpointAddress & 0x80))
	        status = ehci_dealloc_qtd (ehci, qhinfo->dummy_qtd);

	    /* Deallocate the memory for this QH and QHINFO */
	    if(qhinfo->type != USB_EP_ISO)
	        status = ehci_dealloc_qh (ehci, qh);

	    NU_Deallocate_Memory (qhinfo);

	    status = NU_Release_Semaphore(&(ehci->protect_sem));
	}	

    return (status);
}

/*************************************************************************
* FUNCTION
*       NU_USBH_EHCI_Modify_Pipe
*
* DESCRIPTION
*
*       Modifies endpoint descriptors in the H/W and makes them ready for
*       scheduling transfers.
*
* INPUTS
*
*       cb                handle that identifies the HC in the EHCI HC
*                         database.
*       function_address  identifies the device on which the pipe resides
*       bEndpointAddress  identifies the endpoint which owns the pipe
*       bmAttributes      identifies the endpoint type.
*       speed             Device speed - USB_SPEED_LOW/USB_SPEED_FULL/
*                         USB_SPEED_HIGH.
*       ep_max_size       Maximum pkt size the endpoint should support.
*       interval          interval in micro seconds for periodic endpoints.
*       load              frame b/w consumed by the endpoint.
*
* OUTPUTS
*
*     NU_SUCCESS              Indicates successful creation of the pipe in
*                             h/w.
*     NU_INVALID_SEMAPHORE    Indicates the semaphore pointer is
*                              invalid.
*     NU_SEMAPHORE_DELETED    Semaphore was deleted while the task
*                              was suspended.
*     NU_UNAVAILABLE          Indicates the semaphore is unavailable.
*     NU_USB_INVLD_ARG        Indicates that the speed is passed an
*                             invalid value.
*************************************************************************/

STATUS NU_USBH_EHCI_Modify_Pipe (NU_USB_HW * cb,
                                  UINT8 function_address,
                                  UINT8 bEndpointAddress,
                                  UINT8 bmAttributes,
                                  UINT16 ep_max_size,
                                  UINT32 interval,
                                  UINT32 load)
{
    STATUS status;

    NU_USBH_EHCI *ehci = (NU_USBH_EHCI *) cb;

    UINT32 key = ehci_qh_key (function_address, bEndpointAddress);

    USBH_EHCI_QH_INFO *qhinfo;

    UINT8 speed;

    /* No pipes need to modified for Root hub */

    if (function_address == USB_ROOT_HUB)
        return NU_SUCCESS;

    /* Multi-thread safety to ed_hash_table/td_hash_table */

    status = NU_Obtain_Semaphore(&(ehci->protect_sem), NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        return status;
    }

    /* Get the speed of the pipe from the qhinfo for this. */

    qhinfo = hash_find_qh (ehci->qh_async_ht, key);
	if ( qhinfo == NU_NULL )
	{
		return ( NU_USB_INVLD_ARG );
	}

    speed = qhinfo->speed;

    /* Close the pipe and open it again */

    status = NU_USBH_EHCI_Close_Pipe (cb, function_address,
                                        bEndpointAddress);

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_EHCI_Open_Pipe (cb, function_address,
                                          bEndpointAddress,
                                          bmAttributes, speed,
                                          ep_max_size, interval, load);
    }

   status = NU_Release_Semaphore(&(ehci->protect_sem));

   return (status);
}

/* =======================  End Of File  ============================== */
