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
*       nu_usbh_ohci_ext.c
*
*
* COMPONENT
*
*       OHCI Driver / Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains the exported function definitions for
*       the Generic OHCI driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBH_OHCI_Create                 Initializes the OHCI Driver.
*       _NU_USBH_OHCI_Create                Initializes the generic OHCI
*                                           driver.
*       _NU_USBH_OHCI_Delete                Un-initializes the OHCI Driver.
*       NU_USBH_OHCI_Close_Pipe             Closes a pipe on the Hw.
*       NU_USBH_OHCI_Disable_Int            Disables the Hw interrupts.
*       NU_USBH_OHCI_Enable_Int             Enables the Hw interrupts.
*       NU_USBH_OHCI_Flush_Pipe             Flushes all transfers on a pipe
*       NU_USBH_OHCI_ISR                    Hw Interrupt processing.
*       NU_USBH_OHCI_Initialize             Initializes the Hw.
*       NU_USBH_OHCI_Modify_Pipe            Alters the characteristics of
*                                           the pipe.
*       NU_USBH_OHCI_Open_Pipe              Initializes a pipe.
*       NU_USBH_OHCI_Uninitialize           Un-initializes the Hw.
*       NU_USBH_OHCI_Submit_IRP             Accepts a transfer on a pipe.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ==============  USB Include Files =================================== */

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "drivers/nu_usbh_ohci_ext.h"

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_OHCI_Create
*
* DESCRIPTION
*
*       This function initializes the Generic OHCI driver.
*
* INPUTS
*
*       cb                                  OHCI Control Block.
*       name                                Name of the controller.
*       pool                                Memory pool for the driver to
*                                           operate on.
*       base_address                        Memory location in the system
*                                           memory map for the controller.
*       vector                              Interrupt vector associated
*                                           with the OHCI.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver has
*                                           been initialized successfully.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*       NU_NOT_PRESENT                      Indicates a configuration
*                                           problem because of which no
*                                           more USB objects could be
*                                           created.
*
**************************************************************************/
STATUS NU_USBH_OHCI_Create (NU_USBH_OHCI   *cb,
                            CHAR           *name,
                            NU_MEMORY_POOL *pool,
                            VOID           *base_address,
                            INT             vector)
{
    STATUS status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    memset(cb, 0, sizeof(NU_USBH_OHCI));

    cb->cacheable_pool = pool;

    /* Call the base function. */
    status = _NU_USBH_HW_Create((NU_USBH_HW *) cb,
                                name,
                                pool,
                                1,
                                USB_SPEED_FULL,
                                base_address,
                                vector,
                                &usbh_ohci_dispatch);

    /* Return to user mode. */
    NU_USER_MODE();

    NU_ASSERT(status == NU_SUCCESS);
    return (status);
}


/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_OHCI_Create2
*
* DESCRIPTION
*
*       This function initializes the Generic OHCI driver.
*
* INPUTS
*
*       cb                                  OHCI Control Block.
*       name                                Name of the controller.
*       pool                                Memory pool for the driver to
*                                           operate on.
*       base_address                        Memory location in the system
*                                           memory map for the controller.
*       vector                              Interrupt vector associated
*                                           with the OHCI.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver has
*                                           been initialized successfully.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*       NU_NOT_PRESENT                      Indicates a configuration
*                                           problem because of which no
*                                           more USB objects could be
*                                           created.
*
**************************************************************************/
STATUS NU_USBH_OHCI_Create2 (NU_USBH_OHCI   *cb,
                            CHAR           *name,
                            NU_MEMORY_POOL *uncacheable_pool,
                            NU_MEMORY_POOL *cacheable_pool,
                            VOID           *base_address,
                            INT             vector)
{
    STATUS status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    memset(cb, 0, sizeof(NU_USBH_OHCI));

    cb->uncachable_pool = uncacheable_pool;
    cb->cacheable_pool = cacheable_pool;

    /* Call the base function. */
    status = _NU_USBH_HW_Create((NU_USBH_HW *) cb,
                                name,
                                uncacheable_pool,
                                1,
                                USB_SPEED_FULL,
                                base_address,
                                vector,
                                &usbh_ohci_dispatch);

    /* Return to user mode. */
    NU_USER_MODE();

    NU_ASSERT(status == NU_SUCCESS);

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*        _NU_USBH_OHCI_Delete
*
* DESCRIPTION
*
*        This function un-initializes the driver.
*
* INPUTS
*
*        cb                                 OHCI driver to be
*                                           un-initialized.
*
* OUTPUTS
*
*        NU_SUCCESS                         always.
*
**************************************************************************/
STATUS _NU_USBH_OHCI_Delete(VOID *cb)
{
    return _NU_USBH_HW_Delete(cb);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_OHCI_Initialize
*
* DESCRIPTION
*
*       Generic OHCI Initialization function.
*
* INPUTS
*
*       cb                                  OHCI Control block.
*       stack                               Stack object associated with
*                                           the hw.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful initialization of
*                                           the hw
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*       NU_NO_MEMORY                        Indicates that enough memory
*                                           doesn't exist.
*
**************************************************************************/
STATUS NU_USBH_OHCI_Initialize(NU_USB_HW *cb, NU_USB_STACK *stack)
{
    STATUS          status;
    UINT32          read_reg;
    NU_USBH_OHCI   *ohci = (NU_USBH_OHCI *) cb;
    UINT16          rst_count = 0;

    ohci->int_disable_count = NU_NULL;
    ohci->global_pend = NU_NULL;
    ohci->intr_status = NU_NULL;
    ohci->done_q = NU_NULL;

    /* Disable the controller to do a reset. */
    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_CONTROL),
                    0);

    /* Subject it to a software reset of Host Controller. */
    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_COMMAND_STATUS),
                    USBH_OHCI_CS_HCR);
                    
    /* Wait for USB host controller to reset. */
    do
    {
        OHCI_HW_READ32(ohci,
                        (ohci->cb.controller.base_address +
                         USBH_OHCI_HC_COMMAND_STATUS),
                        read_reg);
        rst_count++;

    }while((read_reg & USBH_OHCI_CS_HCR) && (rst_count < USBH_OHCI_MAX_RST_ITERATIONS));

    if (rst_count == USBH_OHCI_MAX_RST_ITERATIONS )
    {
        return ( NU_USB_INTERNAL_ERROR );
    }

    /* Now initialize list heads. */
    /* Reset HcControlHeadED Register. */
    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_CONTROL_HEAD_ED),
                    0);

    /* Reset HcBulkHeadED Register. */
    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_BULK_HEAD_ED),
                    0);

     /* Allocate memory for HCCA, that's aligned to a 256 byte boundary */
    status = NU_Allocate_Aligned_Memory(ohci->cb.pool,
                                        (VOID **) &(ohci->hcca),
                                        sizeof(USBH_OHCI_HCCA),
                                        OHCI_HCCA_ALIGNMENT,
                                        NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_ASSERT(0);
        return status;
    }

    memset((VOID *)ohci->hcca, 0, sizeof(USBH_OHCI_HCCA));

    /* Fill the HCCA pointer in HcHCCA Register. */
    OHCI_WRITE_ADDRESS(ohci,
                       (ohci->cb.controller.base_address +
                        USBH_OHCI_HC_HCCA),
                       ohci->hcca);

    /* Set frame interval in bit times in HcFmInterval Register. */
    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_FM_INTERVAL),
                    USBH_OHCI_DEFAULT_FRAMEINTERVAL);

    /* Set periodic start to be at 90% of frame interval in HcPeriodicStart
     * Register.
     */
    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_PERIODIC_START),
                    USBH_OHCI_PERIODIC_START);

    /* Set the threshold for low speed devices in HcLSThreshold Register.*/
    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_LS_THRESHOLD),
                    USBH_OHCI_LSTHRESH);

    /* Initialize root hub. */
    status = ohci_rh_init(ohci);

    if (status != NU_SUCCESS)
    {
        NU_ASSERT(0);

        NU_Deallocate_Memory(ohci->hcca);

        return ( status );
    }

    /* Set Control to bulk b/w sharing ratio as 4:1. */
    /* 1st read HcControl Register. */
    OHCI_HW_READ32(ohci,
                   (ohci->cb.controller.base_address +
                    USBH_OHCI_HC_CONTROL),
                   read_reg);

    /* Write HcControl Register back by setting CBSR,
     * ControlBulkServiceRatio[1:0] for 4:1.
     */
    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_CONTROL),
                    read_reg | USBH_OHCI_CTL_CBSR_4TO1);

    /* Call parent initialize routine to install LISR */
    status = _NU_USBH_HW_Initialize((NU_USB_HW *) cb,
                                    (NU_USB_STACK *) stack);
    if (status != NU_SUCCESS)
    {
        NU_ASSERT(0);

        NU_Deallocate_Memory(ohci->hcca);

        return status;
    }

    /* Create a lock to protect internal lists of OHCI */
    status = NU_Create_Semaphore(&ohci->protect_sem,
                                 "OHCISEM",
                                 1,
                                 NU_PRIORITY);
    if (status != NU_SUCCESS)
    {
        NU_ASSERT(0);

        NU_Deallocate_Memory(ohci->hcca);

        return status;
    }

    /* Write HcInterruptEnable Register back, by setting
     * SO (Scheduling Overrun), WDH (HcDoneHead Write Back),
     * RD (Resume Detect), UE (Unrecoverable Error) and
     * RHSC (Root Hub Status Change).
     * MIE (Master Interrupt Enable) is not Set.
     */
    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_INT_ENABLE),
                    OHCI_INTR_EN_BMP);

    /* Set RemoteWakeupConnected bit in HCControl register according to
     * user preference.
     */
    OHCI_HW_READ32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_CONTROL),
                    read_reg);

#if(OHCI_REMOTE_WAKEUP_CONNECTED == NU_TRUE)

    read_reg |= (USBH_OHCI_CTL_RWC| USBH_OHCI_CTL_RWE);

#else

    read_reg &= ~(USBH_OHCI_CTL_RWC| USBH_OHCI_CTL_RWE);

#endif

    OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_CONTROL),
                    read_reg);

    /* Make HC operational now. */
    OHCI_HW_WRITE32 (ohci,
                     (ohci->cb.controller.base_address +
                      USBH_OHCI_HC_CONTROL),
                      USBH_OHCI_CTL_HCFS_OP);

    return (NU_SUCCESS);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_OHCI_Uninitialize
*
* DESCRIPTION
*
*       This function un-initializes the OHCI hardware.
*
* INPUTS
*
*       cb                                  OHCI controller to be
*                                           un-initialized.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always.
*
**************************************************************************/
STATUS NU_USBH_OHCI_Uninitialize(NU_USB_HW *cb)
{
    STATUS          status = NU_SUCCESS;
    NU_USBH_OHCI   *ohci = (NU_USBH_OHCI *) cb;
    UINT32 index;
    OHCI_ALIGNED_MEM_POOL   *pool;

    /* Check if any pipes are still open. */
    for(index =0; index < USBH_OHCI_HASH_SIZE; index++ )
    {
        if(ohci->ed_hash_table[index] != NU_NULL)
        {
            status = NU_USB_REJECTED;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Call parent uninitialize routine to de install LISR. */
        status = _NU_USBH_HW_Uninitialize((NU_USB_HW *) cb);

        /* Stop the controller as HW Environment has been
         * un-initialized.
         */
        OHCI_HW_WRITE32(ohci,
                (ohci->cb.controller.base_address +
                 USBH_OHCI_HC_CONTROL),
                0);

        usb_wait_ms(2);
    }

    if(status == NU_SUCCESS)
    {
        /* De-Allocate memory for HCCA. */
        if(ohci->hcca)
        {
            status = NU_Deallocate_Memory(ohci->hcca);
        }

        if(status == NU_SUCCESS)
        {
            for (index = 0; index < OHCI_MAX_TD_GEN_POOLS; index++)
            {
                pool = &(ohci->td_gen_pool[index]);

                if (pool->start_ptr != NU_NULL)
                {
                    status = NU_Deallocate_Memory(pool->start_ptr);
                }
            }
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Delete Semaphore. */
        status = NU_Delete_Semaphore(&ohci->protect_sem);
    }

    NU_ASSERT(status == NU_SUCCESS);
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_OHCI_Enable_Int
*
* DESCRIPTION
*
*       This function enables the interrupts from the hardware.
*
* INPUTS
*
*       cb                                  OHCI controller to enable the
*                                           interrupts for.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always.
*
**************************************************************************/
STATUS NU_USBH_OHCI_Enable_Int(NU_USB_HW *cb)
{
    NU_USBH_OHCI   *ohci = (NU_USBH_OHCI *) cb;

    /* Set Master Interrupt Enable. */
    USBH_OHCI_UNPROTECT;

    return (NU_SUCCESS);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_OHCI_Disable_Int
*
* DESCRIPTION
*
*       This function disables the interrupts from the hardware.
*
* INPUTS
*
*       cb                                  OHCI controller to disable the
*                                           interrupts for.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always.
*
**************************************************************************/

STATUS NU_USBH_OHCI_Disable_Int(NU_USB_HW *cb)
{
    UINT32          intr_en_reg;
    UINT32          intr_sts_reg;
    NU_USBH_OHCI   *ohci = (NU_USBH_OHCI *) cb;

    NU_ASSERT(ohci->int_disable_count == NU_NULL);

    /* Checks are placed to see if this OHCI has generated an interrupt.
     * This is required if multiple host controllers are present and they
     * are using a shared interrupt line.
     */

    /* Read HcInterruptEnable Register. */
    OHCI_HW_READ32(ohci,
                   (ohci->cb.controller.base_address +
                    USBH_OHCI_HC_INT_ENABLE),
                   intr_en_reg);

    /* Check if this OHCI can generate an interrupt. */
    if(intr_en_reg & USBH_OHCI_INT_MIE)
    {
        /* Read HcInterruptStatus Register. */
        OHCI_HW_READ32(ohci,
                   (ohci->cb.controller.base_address +
                   USBH_OHCI_HC_INT_STATUS),
                   intr_sts_reg);

        /* Check if this OHCI has generated an interrupt. */
        if(intr_en_reg & intr_sts_reg)
        {
            /* Disable MIE bit to further process the interrupt. */
            USBH_OHCI_PROTECT;

            /* Store pending interrupts in the control block. */
            ohci->intr_status = (intr_sts_reg & intr_en_reg);

            /* If WritebackDoneHead interrupt is set i.e. HC has
             * updated the HccaDoneHead and HCA has not saved it.
             */
            if (intr_sts_reg & USBH_OHCI_INT_WDH)
            {
                NU_ASSERT(ohci->done_q == NU_NULL);

                /* doneHead is filled by HC at the address pointed by
                 * HCCA.
                 */
                ohci->done_q = (USBH_OHCI_TD *) (ohci->hcca->doneHead);
            }

            /* Write HcInterruptStatus Register to clear all the
             * interrupts.
             */
            OHCI_HW_WRITE32(ohci,
                    (ohci->cb.controller.base_address +
                     USBH_OHCI_HC_INT_STATUS),
                    intr_sts_reg);
        }
    }

    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_OHCI_ISR
*
* DESCRIPTION
*
*       This function processes the interrupt from the OHCI controller.
*
* INPUTS
*
*       cb                                  OHCI controller which generated
*                                           the interrupt.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always.
*
**************************************************************************/
STATUS NU_USBH_OHCI_ISR(NU_USB_HW *cb)
{
    UINT32        intr_status;
    NU_USBH_OHCI *ohci = (NU_USBH_OHCI *) cb;

    /* All the pending interrupts stored in the control block. */
    intr_status = ohci->intr_status;

    /* If there is any interrupt to service. */
    if (intr_status)
    {
        /* Set saved intr_status to zero, as all interrupts are going to be
         * serviced here.
         */
        ohci->intr_status = NU_NULL;

        /* For WDH (HcDoneHead Write acknowledge) interrupt which is
         * pending.
         */
/*-----------------------------------------------------------------------*/
        if (intr_status & USBH_OHCI_INT_WDH)
/*-----------------------------------------------------------------------*/
        {
            /* Start processing done queue. */
            ohci_hndl_td_done_q(ohci, ohci->done_q);

            ohci->done_q = NU_NULL;
        }

/*-----------------------------------------------------------------------*/
        /* Root hub status Change Interrupt. */
        if (intr_status & USBH_OHCI_INT_RHSC)
/*-----------------------------------------------------------------------*/
        {
            /* Process root hub interrupt. */
            ohci_rh_isr(ohci);
        }
/*-----------------------------------------------------------------------*/
        /* SOF Interrupt. This interrupt is not enabled, so just disable
         * it.
         */
        if (intr_status & USBH_OHCI_INT_SF)
/*-----------------------------------------------------------------------*/
        {
            /* Clear the SOF interrupt by writing 1 at that location. */
            OHCI_HW_WRITE32(ohci,
                            (ohci->cb.controller.base_address +
                             USBH_OHCI_HC_INT_DISABLE),
                            USBH_OHCI_INT_SF);
        }

/*-----------------------------------------------------------------------*/
        /* Resume Interrupt (Remote wakeup detected). */
        if (intr_status & USBH_OHCI_INT_RD)
/*-----------------------------------------------------------------------*/
        {
            UINT32 hcControl;

            /* Read HcInterruptStatus Register. */
            OHCI_HW_READ32(ohci,
                   (ohci->cb.controller.base_address + USBH_OHCI_HC_CONTROL),
                   hcControl);

            /* Check if HC is in suspend state. */
            if((hcControl & USBH_OHCI_CTL_HCFS_MASK) ==
                                                USBH_OHCI_CTL_HCFS_SUSPEND)
            {
                /* Clear current function state. */
                hcControl &= ~USBH_OHCI_CTL_HCFS_MASK;

                hcControl |= USBH_OHCI_CTL_HCFS_RESUME;

                OHCI_HW_WRITE32(ohci,
                   (ohci->cb.controller.base_address +
                    USBH_OHCI_HC_CONTROL),
                   hcControl);
            }

            /* Check if HC is in suspend state. */
            if((hcControl & USBH_OHCI_CTL_HCFS_MASK) ==
                                                 USBH_OHCI_CTL_HCFS_RESUME)
            {
                /* Wait for 20ms to complete the resume signaling. */
                usb_wait_ms(20);

                /* Clear current function state. */
                hcControl &= ~USBH_OHCI_CTL_HCFS_MASK;

                /* Change the functional state to operational. */
                OHCI_HW_WRITE32(ohci,
                 (ohci->cb.controller.base_address +
                  USBH_OHCI_HC_CONTROL),
                  (hcControl | USBH_OHCI_CTL_HCFS_OP));

                /* Wait for another 20ms which is required by some OHCI
                 * controllers.
                 */
                usb_wait_ms(20);
            }
        }

/*-----------------------------------------------------------------------*/
        /* Unrecoverable error isn't expected! */
        if (intr_status & USBH_OHCI_INT_UE)
/*-----------------------------------------------------------------------*/
        {
            NU_ASSERT(0);
        }

/*-----------------------------------------------------------------------*/
        /* Schedule overrun isn't expected */
        if (intr_status & USBH_OHCI_INT_SO)
/*-----------------------------------------------------------------------*/
        {
            NU_ASSERT(0);
        }
    }

    return (NU_SUCCESS);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_OHCI_Submit_IRP
*
* DESCRIPTION
*
*       This function accepts a transfer to a given endpoint on a given
*       device. This function checks to see if the device is a root hub. If
*       so appropriate root hub IRP processing function is invoked.
*       Otherwise appropriate list is found for this transfer and is
*       added to the list.
*
* INPUTS
*
*       cb                                  OHCI controller for transfer
*       irp                                 Data transfer description
*       function_address                    Device (address) to which the
*                                           transfer is directed.
*       bEndpointAddress                    Endpoint on the device for
*                                           which this transfer is meant
*                                           for.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates a successful
*                                           submission of data transfer.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*       NU_INVALID_SEMAPHORE                Indicates the semaphore pointer
*                                           is invalid.
*       NU_SEMAPHORE_DELETED                Semaphore was deleted while the
*                                           task was suspended.
*       NU_UNAVAILABLE                      Indicates the semaphore is
*                                           unavailable.
*
**************************************************************************/
USBH_OHCI_ED_INFO  *G_ed_info;

STATUS NU_USBH_OHCI_Submit_IRP(NU_USB_HW *cb,
                                NU_USB_IRP *irp,
                                UINT8 function_address,
                                UINT8 bEndpointAddress)
{
    STATUS              status;
    UINT32              key;
    USBH_OHCI_ED_INFO  *ed_info;
    NU_USBH_OHCI       *ohci = (NU_USBH_OHCI *) cb;
    UINT32 controlinfo;

    /* If the IRP meant for root hub. */
    if (function_address == USB_ROOT_HUB)

                /* Call function to handle all (control) transfers for
                 * Root hub.
                 */
    {
        status = ohci_rh_handle_irp(ohci, irp, bEndpointAddress);
    }
    else
    {
        /* Obtain List Lock Mutex, so that no other resource can work on
         * TDs.
         */
        status = NU_Obtain_Semaphore(&ohci->protect_sem, NU_SUSPEND);
        if (status != NU_SUCCESS)
        {
            NU_ASSERT(0);
            return (status);
        }

        /* Look up pipe in ED hash table. */
        /* Determine the key, based on function address and EP address. */
        key = ohci_ed_key(function_address, bEndpointAddress);

        /*Find ED, based on hash key. */
        ed_info = ohci_hash_find_ed(ohci, key);

        if (ed_info == NU_NULL)
        {
            NU_ASSERT(0);

            NU_Release_Semaphore(&ohci->protect_sem);

            return (NU_USB_INVLD_ARG);
        }

        if (ed_info->type == USB_EP_ISO)
        {
            NU_USB_ISO_IRP *iso_irp = ( NU_USB_ISO_IRP *)irp;
            UINT16 *buf_lengths;
            UINT16 trans_count;
            UINT8 **pdata_array;

            /* Get array of data to be transmitted. */
            NU_USB_ISO_IRP_Get_Lengths(iso_irp, &buf_lengths);
            NU_USB_ISO_IRP_Get_Num_Transactions (iso_irp, &trans_count);

            /* Get buffer pointer for this transaction. */
            NU_USB_ISO_IRP_Get_Buffer_Array(iso_irp, &pdata_array);

            if((buf_lengths == NU_NULL) ||
                (pdata_array == NU_NULL) ||
                (trans_count == NU_NULL) ||
                (trans_count > USBH_OHCI_MAX_ISO_TRANS_PER_IRP))
            {
                status = NU_USB_INVLD_ARG;
                NU_ASSERT(0);
            }
            else
            {

                /* For ISO pending or Large IRPs are not required. */
                status = ohci_handle_iso_irp(ohci, ed_info, irp);
            }
        }
        else
        {
            USBH_OHCI_PROTECT;

            /* If an IRP is already in progress, put this IRP in pending
             * array.
             */
            if (ed_info->irp_info.irp != NU_NULL)
            {
                 OHCI_ADD_TO_LIST(ed_info->pend_irp, irp);

                 USBH_OHCI_UNPROTECT;
            }
            else
            {
                USBH_OHCI_UNPROTECT;

                /* Translate IRP to the TD. */
                // TEMP TEMP
                G_ed_info = ed_info;
                // TEMP TEMP
                status = ohci_handle_irp(ohci, ed_info, irp, NU_TRUE);

                if(status != NU_SUCCESS)
                {
                    
                    OHCI_HW_READ32(ohci, &ed_info->hwEd->controlinfo, controlinfo);
                    
                    controlinfo |= USBH_OHCI_EDCTL_SKIP;
                    
                    OHCI_HW_WRITE32(ohci, &ed_info->hwEd->controlinfo, controlinfo);
                    

                    ohci_dealloc_all_tds(ohci, ed_info);

                    ed_info->irp_info.irp = NU_NULL;
                }
            }
        }

        /* Release List Lock Mutex, so that other resources can work
         * on TDs.
         */
        NU_Release_Semaphore(&ohci->protect_sem);
    }

    NU_ASSERT(status == NU_SUCCESS);

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_OHCI_Flush_Pipe
*
* DESCRIPTION
*
*       Removes all pending TDs from HC's schedule list. Their
*       corresponding IRP callbacks are notified of this transfer
*       termination.
*
* INPUTS
*
*       cb                                  OHCI Control block.
*       function_address                    Device (address) to which the
*                                           transfers must be flushed.
*       bEndpointAddress                    Endpoint on the device for
*                                           which the transfers must be
*                                           flushed.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the TDs
*                                           termination has been
*                                           successfully initiated.
*       NU_USB_INVLD_ARG                    Ohci pointers passed are
*                                           NU_NULL.
*       NU_INVALID_SEMAPHORE                Semaphore pointer is
*                                           invalid.
*       NU_SEMAPHORE_DELETED                Semaphore was deleted while the
*                                           task was suspended.
*       NU_UNAVAILABLE                      Semaphore is unavailable.
*
**************************************************************************/
STATUS NU_USBH_OHCI_Flush_Pipe(NU_USB_HW *cb,
                                UINT8 function_address,
                                UINT8 bEndpointAddress)
{
    STATUS              status;
    UINT32              key;
    USBH_OHCI_ED_INFO  *ed_info;
    NU_USBH_OHCI       *ohci = (NU_USBH_OHCI *) cb;

    if (ohci == NU_NULL)
    {
        NU_ASSERT(NU_NULL);

        return ( NU_USB_INVLD_ARG);
    }

    if (function_address == USB_ROOT_HUB)
    {
        /* No pipes are established for root hub. */
        return (NU_SUCCESS);
    }

    /* Multi-thread safety to ED/TD hash table. */
    status = NU_Obtain_Semaphore(&ohci->protect_sem, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NU_ASSERT(0);

        return status;
    }

    /* Find the key for hashing into the hash table. */
    key = ohci_ed_key(function_address, bEndpointAddress);

    /* Get ed_info pointer from hash table, based on key. */
    ed_info = ohci_hash_find_ed(ohci, key);

    if (ed_info)
    {
        /* Retire all transfers. */
        ohci_unlink_tds(ohci, ed_info, NU_FALSE);

        status = NU_SUCCESS;
    }
    else
    {
        status = NU_NOT_PRESENT;
    }

    NU_Release_Semaphore(&ohci->protect_sem);

    NU_ASSERT(status == NU_SUCCESS);
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_OHCI_Open_Pipe
*
* DESCRIPTION
*
*       Creates endpoint descriptors in the H/W and makes them ready for
*       scheduling transfers.
*
* INPUTS
*
*       cb                                  Handle that identifies the HC
*                                           in the OHCI HC database.
*       function_address                    Identifies the device on which
*                                           the pipe resides
*       bEndpointAddress                    Identifies the endpoint which
*                                           owns the pipe
*       bmAttributes                        Identifies the endpoint type.
*       speed                               USB_SPEED_LOW/USB_SPEED_FULL.
*       ep_max_size                         Maximum packet size the
*                                           endpoint should support.
*       interval                            Interval in micro seconds for
*                                           periodic endpoints.
*       load                                Frame b/w consumed by the
*                                           endpoint.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful creation
*                                           of the pipe in h/w.
*       NU_INVALID_SEMAPHORE                Indicates the semaphore pointer
*                                           is invalid.
*       NU_SEMAPHORE_DELETED                Semaphore was deleted while the
*                                           task was suspended.
*       NU_UNAVAILABLE                      Indicates the semaphore is
*                                           unavailable.
*       NU_USB_INVLD_ARG                    Indicates that the speed is
*                                           passed an invalid value.
*
**************************************************************************/
STATUS NU_USBH_OHCI_Open_Pipe(NU_USB_HW *cb,
                               UINT8 function_address,
                               UINT8 bEndpointAddress,
                               UINT8 bmAttributes,
                               UINT8 speed,
                               UINT16 ep_max_size,
                               UINT32 interval,
                               UINT32 load)
{

    STATUS                  status = NU_SUCCESS;
    UINT32                  ctrlinfo = 0;
    UINT8                   rollback = 0;
    UINT8                   pipe_type;
    UINT8                   endpoint;
    UINT8                   count;
    USBH_OHCI_ED_INFO      *ed_info;
    USBH_OHCI_ISO_ED_INFO  *iso_ed_info;
    USBH_OHCI_TD           *gentd = NU_NULL;
    USBH_OHCI_TD_ISO       *isotd = NU_NULL;
    USBH_OHCI_TD_ISO *td_array_start;
    NU_USBH_OHCI           *ohci = (NU_USBH_OHCI *) cb;

    /* Get pipe type from bmAttributes. */
    pipe_type = bmAttributes & 3;

    /* Get Endpoint number from the bEndpointAddress. */
    endpoint = bEndpointAddress & 0xf;

    /* No pipes need to be established for transfers to Root hub. */
    if (function_address == USB_ROOT_HUB)
    {
        return NU_SUCCESS;
    }

    /* Only supports Full Speed and Low Speed. */
    if ((speed == USB_SPEED_UNKNOWN) || (speed > USB_SPEED_FULL))
    {
        return NU_USB_INVLD_ARG;
    }

    /* Multi-thread safety to ed_hash_table/td_hash_table. */
    status = NU_Obtain_Semaphore(&ohci->protect_sem, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NU_ASSERT(0);

        return (status);
    }

    /* Formulate the control DWORD of the ED. */
    ctrlinfo = ((function_address & USBH_OHCI_EDCTL_FA_MASK) |
                    ((endpoint << 7) & USBH_OHCI_EDCTL_EN_MASK));

    /* TD direction. */
    if (pipe_type == USB_EP_CTRL)
    {
        ctrlinfo |= (USBH_OHCI_EDCTL_DIR_TD);
    }
    else
    {
        ctrlinfo |= (((bEndpointAddress & 0x80) == USB_DIR_OUT) ?
                     USBH_OHCI_EDCTL_DIR_OUT :
                     USBH_OHCI_EDCTL_DIR_IN);
    }

    /* Check if the pipe is already opened. */

    /*Find ED, based on hash key. */
    ed_info = ohci_hash_find_ed(ohci, (ctrlinfo & 0xFFF));

    if(ed_info)
    {
        /* Pipe is already opened. Sending success otherwise stack will
         * report error.
         */
        NU_Release_Semaphore(&ohci->protect_sem);

        return (NU_SUCCESS);
    }
    else
    {
        if (pipe_type != USB_EP_ISO)
        {
            /* Create a SW ed_info structure. */
            status = NU_Allocate_Memory(ohci->cacheable_pool,
                                        (VOID **) &ed_info,
                                        sizeof(USBH_OHCI_ED_INFO),
                                        NU_NO_SUSPEND);

            if (status != NU_SUCCESS)
            {
                rollback = 1;
            }

            if (!rollback)
            {
                memset((VOID *)ed_info, 0, sizeof(USBH_OHCI_ED_INFO));
            }
        }
        else
        {
            /* Create a SW ed_info structure. */
            status = NU_Allocate_Memory(ohci->cacheable_pool,
                                        (VOID **) &iso_ed_info,
                                        sizeof(USBH_OHCI_ISO_ED_INFO),
                                        NU_NO_SUSPEND);

            if (status != NU_SUCCESS)
            {
                rollback = 1;
            }

            if (!rollback)
            {
                memset((VOID *)iso_ed_info, 0, sizeof(USBH_OHCI_ISO_ED_INFO));
                ed_info = (USBH_OHCI_ED_INFO *) iso_ed_info;
            }
        }

        if (!rollback)
        {
            /* Create its corresponding HW ed_info structure. */
            status = ohci_alloc_ed(ohci, &(ed_info->hwEd), NU_NO_SUSPEND);

            if (status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        if (!rollback)
        {
            memset((VOID *)ed_info->hwEd, 0, sizeof(USBH_OHCI_ED));

            /* Adjust interval of periodic pipes to the closest 2**n
             * value.
             */
            if ((pipe_type == USB_EP_ISO) || (pipe_type == USB_EP_INTR))
            {
                ed_info->interval =
                                 ohci_normalize_interval(interval / 1000L);
            }

            ed_info->load = load;
            ed_info->speed = speed;

            /* Only [11:0] are used.  */
            ed_info->key = (ctrlinfo & 0xfff);

            /* Max packet size. */
            ctrlinfo |= (((ep_max_size) << 16) & USBH_OHCI_EDCTL_MPS_MASK);

            /* Speed is low or high. */
            ctrlinfo |= ((speed == USB_SPEED_LOW) ?
                         USBH_OHCI_EDCTL_SPD_LOW :
                         USBH_OHCI_EDCTL_SPD_FULL);
            ed_info->bEndpointAddress = bEndpointAddress;
            ed_info->type = pipe_type;

            switch (pipe_type)
            {
/*=======================================================================*/
                case USB_EP_CTRL:
/*=======================================================================*/
                    ctrlinfo |= (USBH_OHCI_EDCTL_FMT_GEN |
                                 USBH_OHCI_EDCTL_SKIP);

                    OHCI_HW_WRITE32(ohci,
                                    &ed_info->hwEd->controlinfo,
                                    ctrlinfo);

                    /* Create the dummy head-n-tail TD. */
                    status = ohci_alloc_gen_td(ohci,
                                               &gentd,
                                               ed_info,
                                               NU_NO_SUSPEND);

                    if (status != NU_SUCCESS)
                    {
                        rollback = 3;
                    }
                    if (!rollback)
                    {
                        /* This is the dummy TD. Initialize it to null. */
                        memset((VOID *)gentd, 0, sizeof(USBH_OHCI_TD));
                        gentd->ed_info = ed_info;
                        OHCI_HW_WRITE16(ohci, &gentd->psw, USBH_OHCI_CC_NON_ISO);

                        /* Since only dummy TD at the start so,
                         * tdTail == tdHead.
                         */
                        OHCI_WRITE_ADDRESS(ohci,
                                           &ed_info->hwEd->tdTail,
                                           (UINT32) gentd);

                        OHCI_WRITE_ADDRESS(ohci,
                                           &ed_info->hwEd->tdHead,
                                           (UINT32) gentd);

                        /* Insert the ED in to the HC's schedule list. */
                        ohci_scheduleControlPipe(ohci, ed_info);
                    }
                    break;

/*=======================================================================*/
                case USB_EP_ISO:
/*=======================================================================*/
                    /* Set TD associated with this ED to ISO Format. */
                    ctrlinfo |= (USBH_OHCI_EDCTL_FMT_ISO);

                    /* Write 1st DWORD to the HwED. */
                    OHCI_HW_WRITE32(ohci,
                                    &ed_info->hwEd->controlinfo,
                                    ctrlinfo);

                    status = NU_Allocate_Aligned_Memory(ohci->cb.pool,
                     (VOID **)&td_array_start,
                     (OHCI_MAX_ISO_TDS_PER_IRP * sizeof(USBH_OHCI_TD_ISO)),
                      OHCI_ISO_TD_ALIGNMENT,
                      NU_NO_SUSPEND);

                    if(status != NU_SUCCESS)
                    {
                        NU_ASSERT(0);

                        rollback = 3;

                        break;
                    }

                    /* Save starting pointer of the ISO TD array. */
                    iso_ed_info->td_array_start = td_array_start;

                    /* Reset all the TDs. */
                    memset((VOID *)td_array_start,
                     0,
                    (OHCI_MAX_ISO_TDS_PER_IRP * sizeof(USBH_OHCI_TD_ISO)));

                    /* For ISO, allocate all the SW and Hardware TDs, in
                     *  the start.
                     */
                    for (count = 0;
                         count < OHCI_MAX_ISO_TDS_PER_IRP;
                         count++)
                    {
                        isotd = &td_array_start[count];
                        isotd->ed_info = ed_info;
                        isotd->index = count; 
                    }

                    /* Since no TD at the start so,
                     * tdTail == tdHead = NU_NULL.
                     */
                    OHCI_WRITE_ADDRESS(ohci,
                                       &ed_info->hwEd->tdTail,
                                     (UINT32) iso_ed_info->td_array_start);

                    OHCI_WRITE_ADDRESS(ohci,
                                       &ed_info->hwEd->tdHead,
                                     (UINT32) iso_ed_info->td_array_start);

                    /* Default ISO interval. */
                    ed_info->interval = 1;
                    ed_info->index2perdTbl = 0;

                    /* Insert the ED in to the HC's schedule list. */
                    ohci_schedulePeriodicPipe(ohci, ed_info);

                    break;

/*=======================================================================*/
                case USB_EP_INTR:
/*=======================================================================*/
                    ctrlinfo |= (USBH_OHCI_EDCTL_FMT_GEN);

                    OHCI_HW_WRITE32(ohci,
                                    &ed_info->hwEd->controlinfo,
                                    ctrlinfo);

                    /* Create the dummy head-n-tail TD. */
                    status = ohci_alloc_gen_td(ohci,
                                               &gentd,
                                               ed_info,
                                               NU_NO_SUSPEND);

                    if (status != NU_SUCCESS)
                    {
                        rollback = 3;
                    }
                    if (!rollback)
                    {
                        /* This is the dummy TD. Initialize it to null. */
                        memset((VOID *)gentd, 0, sizeof(USBH_OHCI_TD));
                        gentd->ed_info = ed_info;
                        OHCI_HW_WRITE16(ohci, &gentd->psw, USBH_OHCI_CC_NON_ISO);

                        OHCI_WRITE_ADDRESS(ohci,
                                           &ed_info->hwEd->tdTail,
                                           (UINT32) gentd);
                        OHCI_WRITE_ADDRESS(ohci,
                                           &ed_info->hwEd->tdHead,
                                           (UINT32) gentd);

                        /* Insert the ED in to the HC's schedule list. */
                        ed_info->index2perdTbl =
                                    ohci_find_best_schedule(ohci, ed_info);

                        if (ed_info->index2perdTbl ==
                                                USBH_OHCI_INT_SCHEDULE_CNT)
                        {
                            /* Couldn't be accommodated in the HC's
                             * schedule list.
                             */
                            status = NU_USB_SCHEDULE_ERROR;
                            rollback = 4;
                        }
                    }
                    if (!rollback)
                    {
                        ohci_schedulePeriodicPipe(ohci, ed_info);
                    }

                    break;
/*=======================================================================*/
                case USB_EP_BULK:
/*=======================================================================*/
                    ctrlinfo |= (USBH_OHCI_EDCTL_FMT_GEN);

                    OHCI_HW_WRITE32(ohci,
                                    &ed_info->hwEd->controlinfo,
                                    ctrlinfo);

                    /* Create the dummy head-n-tail TD. */
                    status = ohci_alloc_gen_td(ohci,
                                               &gentd,
                                               ed_info,
                                               NU_NO_SUSPEND);

                    if (status != NU_SUCCESS)
                    {
                        rollback = 3;
                    }
                    if (!rollback)
                    {
                        /* This is the dummy TD. Initialize it to null. */
                        memset((VOID *)gentd, 0, sizeof(USBH_OHCI_TD));
                        gentd->ed_info = ed_info;
                        OHCI_HW_WRITE16(ohci, &gentd->psw, USBH_OHCI_CC_NON_ISO);

                        OHCI_WRITE_ADDRESS(ohci,
                                           &ed_info->hwEd->tdTail,
                                           (UINT32) gentd);
                        OHCI_WRITE_ADDRESS(ohci,
                                           &ed_info->hwEd->tdHead,
                                           (UINT32) gentd);

                        /* Insert the ED in to the HC's schedule list. */
                        ohci_scheduleBulkPipe(ohci, ed_info);
                    }
                    break;

                default:
                    NU_ASSERT(0);
                    break;
            }

            /* Add the new ED to the hash table. */
            ohci_hash_add_ed(ohci, ed_info);

            status = NU_SUCCESS;

            rollback = 1;
        }

        switch (rollback)
        {
            case 4:
                if (gentd)
                {
                    ohci_dealloc_gen_td(ohci, gentd);
                }
            case 3:
                ohci_dealloc_ed(ohci, ed_info->hwEd);

            case 2:
                NU_Deallocate_Memory(ed_info);

            case 1:
                NU_Release_Semaphore(&ohci->protect_sem);

                NU_ASSERT(status == NU_SUCCESS);
                break;
        }

        return (status);

    }

}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_OHCI_Close_Pipe
*
* DESCRIPTION
*
*       Terminates pending TDs and releases the ED. The corresponding
*       callbacks are later notified of their canceled TDs.
*
* INPUTS
*
*       cb                                  Handle that identifies the HC
*                                           in the OHCI HC database.
*       function_address                    identifies the device on which
*                                           the pipe resides.
*       bEndpointAddress                    identifies the endpoint which
*                                           owns the pipe.

*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the pipe close
*                                           is successfully initiated.
*       NU_USB_INVLD_ARG                    Indicates that pipe or ohci
*                                           pointers passed are NU_NULL.
*       NU_INVALID_SEMAPHORE                Indicates the semaphore pointer
*                                           is invalid.
*       NU_SEMAPHORE_DELETED                Semaphore was deleted while the
*                                           task was suspended.
*       NU_UNAVAILABLE                      Indicates the semaphore is
*                                           unavailable.
*
**************************************************************************/
STATUS NU_USBH_OHCI_Close_Pipe(NU_USB_HW *cb,
                                UINT8 function_address,
                                UINT8 bEndpointAddress)
{
    STATUS              status;
    UINT32              key;
    USBH_OHCI_ED_INFO  *ed_info;
    NU_USBH_OHCI       *ohci = (NU_USBH_OHCI *) cb;

    if (ohci == NU_NULL)
    {
        NU_ASSERT(0);
        return (NU_USB_INVLD_ARG);
    }

    if (function_address == USB_ROOT_HUB)
    {
        /* No pipes are established for root hub. */
        return NU_SUCCESS;
    }

    status = NU_Obtain_Semaphore(&ohci->protect_sem, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NU_ASSERT(0);
        return (status);
    }

    /* Look up pipe in ED hash table. */
    key = ohci_ed_key(function_address, bEndpointAddress);

    ed_info = ohci_hash_find_ed(ohci, key);
    if (!ed_info)
    {
        NU_ASSERT(0);

        NU_Release_Semaphore(&ohci->protect_sem);

        return (NU_NOT_PRESENT);
    }

    switch (ed_info->type)
    {
        case USB_EP_CTRL:
        case USB_EP_BULK:
            ohci_close_non_periodic_ed(ohci, ed_info);
            break;

        case USB_EP_ISO:
        case USB_EP_INTR:
            ohci_close_periodic_ed(ohci, ed_info);
            break;

        default:
            NU_ASSERT(0);
            break;
    }
    NU_Release_Semaphore(&ohci->protect_sem);

    return (NU_SUCCESS);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_OHCI_Modify_Pipe
*
* DESCRIPTION
*
*       Modifies endpoint descriptors in the H/W and makes them ready for
*       scheduling transfers.
*
* INPUTS
*
*       cb                                  Handle that identifies the HC
*                                           in the OHCI HC database.
*       function_address                    Identifies the device on which
*                                           the pipe resides.
*       bEndpointAddress                    Endpoint which owns the pipe
*       bmAttributes                        Identifies the endpoint type.
*       speed                               Device speed
*                                           USB_SPEED_LOW/USB_SPEED_FULL.
*       ep_max_size                         Maximum packet size the
*                                           endpoint should support.
*       interval                            Interval in micro seconds for
*                                           periodic endpoints.
*       load                                Frame b/w consumed by the
*                                           endpoint.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful creation of the pipe
*                                           in h/w.
*       NU_INVALID_SEMAPHORE                Semaphore pointer is invalid.
*       NU_SEMAPHORE_DELETED                Semaphore was deleted while t
*                                           he task was suspended.
*       NU_UNAVAILABLE                      Semaphore is unavailable.
*       NU_USB_INVLD_ARG                    Speed is passed an invalid
*                                           value.
*
**************************************************************************/
STATUS NU_USBH_OHCI_Modify_Pipe(NU_USB_HW *cb,
                                 UINT8 function_address,
                                 UINT8 bEndpointAddress,
                                 UINT8 bmAttributes,
                                 UINT16 ep_max_size,
                                 UINT32 interval,
                                 UINT32 load)
{
    UINT32              key;
    STATUS              status;
    UINT8               speed;
    USBH_OHCI_ED_INFO  *ed_info;
    NU_USBH_OHCI       *ohci = (NU_USBH_OHCI *) cb;

    if (ohci == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    if (function_address == USB_ROOT_HUB)
    {
        /* No pipes are established for root hub. */
        return NU_SUCCESS;
    }

    status = NU_Obtain_Semaphore(&ohci->protect_sem, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NU_ASSERT(0);

        return (status);
    }

    /* Look up the pipe in ED hash table. */
    key = ohci_ed_key(function_address, bEndpointAddress);

    ed_info = ohci_hash_find_ed(ohci, key);
    if (!ed_info)
    {
        NU_ASSERT(0);

        NU_Release_Semaphore(&ohci->protect_sem);

        return (NU_NOT_PRESENT);
    }
    speed = ed_info->speed;

    NU_Release_Semaphore(&ohci->protect_sem);

    /* Close the pipe. */
    status = NU_USBH_OHCI_Close_Pipe(cb,
                                      function_address,
                                      bEndpointAddress);

    if (status != NU_SUCCESS)
    {
        return status;
    }

    /* Open again with the new set of attributes. */
    return  NU_USBH_OHCI_Open_Pipe(cb,
                                    function_address,
                                    bEndpointAddress,
                                    bmAttributes,
                                    speed,
                                    ep_max_size,
                                    interval,
                                    load);
}

/* ====================  End of File ================================== */
