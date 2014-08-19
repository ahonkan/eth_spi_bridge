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
*       dma_common.c
*
* COMPONENT
*
*       DMA Device Interface
*
* DESCRIPTION
*
*       This file contains the common routines for DMA Driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       DMA_Add_Chan_Req
*       DMA_Remove_Chan_Req
*       DMA_Trans_Complete
*       DMA_PR_Int_Enable
*       DMA_PR_Int_Disable
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       dma_common.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/nu_services.h"
#include    "drivers/nu_drivers.h"
#include    "drivers/dma_common.h"

/*************************************************************************
*
* FUNCTION
*
*       DMA_Add_Chan_Req
*
* DESCRIPTION
*
*       This function insert the channel in data transfer request list.
*
* INPUTS
*
*       *inst_ptr                        - Device instance handle
*       channel                          - Channel handle
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID DMA_Add_Chan_Req(DMA_INSTANCE_HANDLE  *inst_ptr, DMA_CHANNEL * channel)
{
    INT         int_level;

    if (channel != NU_NULL && inst_ptr != NU_NULL)
    {
        /* Disable interrupts before accessing shared list. */
        int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Set next element as null. */
        channel->next = NU_NULL;

        /* Check if list is empty. */
        if (inst_ptr->chan_req_first == NU_NULL)
        {
            /* Set this channel as first element. */
            inst_ptr->chan_req_first = channel;
        }
        else
        {
            /* Put this channel tail. */
            inst_ptr->chan_req_last->next = (VOID *) channel;
        }

        /* Update last request pointer. */
        inst_ptr->chan_req_last = channel;

        /* Restore interrupts to previous level */
        NU_Local_Control_Interrupts(int_level);
    }
}

/*************************************************************************
*
* FUNCTION
*
*       DMA_Remove_Chan_Req
*
* DESCRIPTION
*
*       This function removes the channel from data transfer request list.
*
* INPUTS
*
*       *inst_ptr                        - Device instance handle
*       hw_chan_id                       - Hardware channel id
*
* OUTPUTS
*
*       DMA_CHANNEL*                     - Channel pointer that is found
*                                          and removed for this channel id.
*
*************************************************************************/
DMA_CHANNEL * DMA_Remove_Chan_Req(DMA_INSTANCE_HANDLE  *inst_ptr, UINT8 hw_chan_id)
{
    DMA_CHANNEL * chan_req;
    DMA_CHANNEL * last_chan_req;
    INT         int_level;

    /* Disable interrupts before accessing shared list. */
    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Get first element pointer. */
    chan_req = inst_ptr->chan_req_first;

    /* Loop through the list. */
    while (chan_req != NU_NULL)
    {
        /* Check if hardware ID match. */
        if (chan_req->hw_chan_id == hw_chan_id)
        {
            /* Exit from loop. */
            break;
        }

        /* Move to next element in the list. */
        last_chan_req = chan_req;
        chan_req = (DMA_CHANNEL *) chan_req->next;
    }

    /* Check if channel found. */
    if(chan_req != NU_NULL)
    {
        /* Check if it is the first element in the list. */
        if (chan_req == inst_ptr->chan_req_first)
        {
            /* Update first request pointer. */
            inst_ptr->chan_req_first =  (DMA_CHANNEL *)inst_ptr->chan_req_first->next;
        }
        /* Check if it is the last element in the list. */
        else if (chan_req == inst_ptr->chan_req_last)
        {
            /* Update last request pointer. */
            last_chan_req->next = NU_NULL;
            inst_ptr->chan_req_last = last_chan_req;
        }
        else
        {
            /* Move to next element. */
            last_chan_req->next = chan_req->next;
        }
    }

    /* Restore interrupts to previous level */
    NU_Local_Control_Interrupts(int_level);

    return chan_req;
}

/*************************************************************************
*
* FUNCTION
*
*       DMA_Trans_Complete
*
* DESCRIPTION
*
*       This function performs transfer completion for the specified hw channel.
*
* INPUTS
*
*       *inst_ptr                        - Device instance handle
*       hw_chan_id                       - Hardware channel id
*       status                           - Completion status
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID DMA_Trans_Complete(DMA_INSTANCE_HANDLE  *inst_ptr, UINT8 hw_chan_id, STATUS status)
{
    DMA_CHANNEL *dma_chan;

    /* Check if instance handler is not null. */
    if (inst_ptr != NU_NULL)
    {
        /* Check if the channel has no more elements to send.  Use the chan_req_first
         * as the current channel.  The DMA driver code curently is inconsistent with
         * hw_chan_id and peri_id usage.  This needs to be resolved when additional 
         * devices need to be brought online that shares the same dma hardware channel
         */             
        if (inst_ptr->chan_req_first->cur_req_length  == 0) 
        {
          /* Search the channel in request list with this hw channel id. */
          dma_chan = DMA_Remove_Chan_Req(inst_ptr, hw_chan_id);
        }
        else
        	dma_chan = inst_ptr->chan_req_first;

        /* If valid channel handle found and callback is registered. */
        if (dma_chan != NU_NULL && inst_ptr->cmp_callback != NU_NULL)
        {
            /* Invoke generic dma driver completion callback function. */
            inst_ptr->cmp_callback(dma_chan, status);
        }
    }
}

/*************************************************************************
*
* FUNCTION
*
*       DMA_PR_Int_Enable
*
* DESCRIPTION
*
*       This function initializes processor-level DMA interrupt
*
* INPUTS
*
*       *inst_ptr                        - Device instance handle
*
* OUTPUTS
*
*       STATUS         status            - Status resulting from
*                                          NU_Register_LISR calls.
*
*************************************************************************/
STATUS DMA_PR_Int_Enable(DMA_INSTANCE_HANDLE  *inst_ptr)
{
    STATUS                status = NU_SUCCESS;
    VOID                  (*old_lisr)(INT);

    /* Register DMA interrupt handler for the interrupt vector. */
    status = NU_Register_LISR(inst_ptr->dma_vector, DMA_Tgt_LISR, &old_lisr);

    /* Check if registration successful */
    if (status == NU_SUCCESS)
    {
        /* Register the DMA data structure with this vector id */
        ESAL_GE_ISR_VECTOR_DATA_SET (inst_ptr->dma_vector, inst_ptr);

        /* Enable the DMA interrupt */
        (VOID) ESAL_GE_INT_Enable (inst_ptr->dma_vector,
                                   inst_ptr->dma_irq_type,
                                   inst_ptr->dma_irq_priority);
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       DMA_PR_Int_Disable
*
* DESCRIPTION
*
*       This function initializes processor-level DMA interrupt
*
* INPUTS
*
*       *inst_ptr                        - Device instance handle
*
* OUTPUTS
*
*       STATUS         status            - Status resulting from
*                                          NU_Register_LISR calls.
*
*************************************************************************/
STATUS DMA_PR_Int_Disable(DMA_INSTANCE_HANDLE  *inst_ptr)
{
    STATUS                status = NU_SUCCESS;
    VOID                  (*old_lisr)(INT);

    /* Disable the DMA interrupt */
    (VOID) ESAL_GE_INT_Disable (inst_ptr->dma_vector);

    /* Unregister DMA handler for the interrupt id. */
    status = NU_Register_LISR (inst_ptr->dma_vector, NU_NULL, &old_lisr);

    /* Clear data structure with this vector id */
    ESAL_GE_ISR_VECTOR_DATA_SET (inst_ptr->dma_vector, NU_NULL);

    return status;
}

