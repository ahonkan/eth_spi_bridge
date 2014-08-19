/*************************************************************************
*
*              Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       nu_usbh_xhci_imp.c
*
*
* COMPONENT
*
*      USB xHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains the worker functions for the Generic xHCI
*       driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       XHCI_Initiate_Bulk_Transfer         This function initiates data
*                                           transfer over bulk endpoints.
*
*       XHCI_Handle_Bulk_TD                 This function parses the bulk TDs
*                                           and places them on the Transfer ring
*                                           for execution by the xHC.
*
*       XHCI_Initiate_Control_Transfer      This function initiates data
*                                           transfer over control endpoints.
*
*       XHCI_Handle_Control_TD              This function parses the control TDs
*                                           and places them on the Transfer ring
*                                           for execution by the xHC.
*
*       XHCI_Handle_Rx_Event                This function handles data transfer
*                                           completion by calling the respective
*                                           functions of transfer types.
*
*       XHCI_Complete_Bulk_Transfer         This function handles data transfer
*                                           completion over bulk endpoints.
*
*       XHCI_Process_Bulk_IRP               This function initializes xHCI
*                                           specific IRP structure and TD.
*
*
*       XHCI_Process_Remaining_IRP          This function processes remaining
*                                           data in the IRP for large data
*                                           transfer.
*
*       XHCI_Handle_Stream_Completion       This function handles data transfer
*                                           completion over bulk streaming
*                                           endpoints.
*
*       XHCI_Complete_Control_Transfer      This function handles data transfer
*                                           completion over control endpoint.
*
*       XHCI_Handle_IRP_Completion          This function sets IRP status,length
*                                           of data transfer and call
*                                           callback function.
*
*       NU_USBH_XHCI_Unlock                 This function unlocks xHCD.
*
*       NU_USBH_XHCI_Lock                   This function locks xHCD for
*                                           exclusive access.
*
*       XHCI_Queue_Command                  This function places command TRB on
*                                           command ring and handles its
*                                            completion.
*
*       XHCI_Queue_Slot_Control_Command     This function parses slot control
*                                           command TRB and queues the command
*                                           on the command ring.
*
*
*       XHCI_Queue_Configure_EP_Command     This function parses configure
*                                           endpoint command TRB and queues the
*                                           command on the command ring.
*
*
*       XHCI_Queue_NOOP_Command             This function parses NOOP command
*                                           TRB and queues the command on the
*                                           command ring.
*
*       XHCI_Queue_Set_TR_Dequeue_Command   This function parses set TR dequeue
*                                           command TRB and queues the command
*                                           on the command ring.
*
*       XHCI_Queue_Address_Device_Command   This function parses address device
*                                           command TRB and queues the command
*                                           on the command ring.
*
*       XHCI_Parse_Config_Endpoint_In_Ctx   This function parses input context
*                                           for configure endpoint command.
*
*       XHCI_Queue_Evaluate_Context_Command This function parses evaluate
*                                           context command TRB and queues the
*                                           command on the command ring
*
*       XHCI_Queue_Reset_Endpoint_Command   This function parses reset endpoint
*                                           command TRB and queues the command
*                                           on the command ring.
*
*       XHCI_RH_Get_Status                  This function handles Get_Port_Status
*                                           request for the root hub.
*
*       XHCI_RH_Clear_Feature               This function handles Clear_Port_Feature
*                                           request for the root hub.
*
*       XHCI_Setup_RH_Ports                 This function binds ports to the
*                                           respective on the basis of xHCI
*                                           Extended capability data structures.
*
*       XHCI_RH_Get_State                   This function handles Get_State
*                                           request for root ports.
*
*       XHCI_RH_Set_Feature                 This function handles port
*                                           Set_Port_Feature request for root
*                                           hub.
*
*       XHCI_RH_Invalid_CMD                 This function handles non-supported
*                                           request for the root hub.
*
*       XHCI_RH_Nothing_To_Do_CMD           This function handles port
*                                           Set_Feature request for root hub.
*
*       XHCI_RH_Get_Descriptor              This function handles Get_Descriptor
*                                           request for root hub.
*
*       XHCI_Allocate_TRB_Ring              This function allocates TRB ring
*                                           according to given alignment and
*                                           number of segments.
*
*       XHCI_Allocate_TRB_Segment           This function allocates TRB segment.
*
*       XHCI_Deallocate_TRB_Segment         This function de-allocates TRB
*                                           segment.
*
*       XHCI_Link_TRB_Segments              This function links TRB segments.
*
*       XHCI_Deallocate_TRB_Ring            This function deallocates TRB ring.
*
*       XHCI_Room_On_Ring                   This function checks if there are
*                                           enough free TRBs on the ring for
*                                           data transfer.
*
*       XHCI_Reinitialize_Ring              This function reinitializes ring.
*
*       XHCI_Queue_TRB_Segments             This function inserts new segment
*                                           in the ring.
*
*       XHCI_Allocate_Scratchpad_Memory     This function allocates scratchpad
*                                           memory for host controller.
*
*       XHCI_Deallocate_Dcratchpad_Memory   This function frees memory allocated
*                                           for scratchpad buffer.
*
*       XHCI_Allocate_Device_Context        This function allocates memory for
*                                           xHCD device context.
*
*       XHCI_Inc_Ring_Enqueue_Ptr           This function increments ring
*                                           enqueue pointer.
*
*       XHCI_Inc_Ring_Dequeue_Ptr           This function increments ring
*                                           de-queue pointer.
*
*       XHCI_Get_Input_Control_Context      This function returns pointer to
*                                           input control context.
*
*       XHCI_Get_Slot_Context               This function is used to get device
*                                           slot context.
*
*       XHCI_Get_Endpoint_Context           This function is used to get device
*                                           endpoint context.
*
*       XHCI_Get_Capability_Regs_Handle     This function returns capability
*                                           register set handle.
*
*       XHCI_Get_Operational_Regs_Handle    This function returns operational
*                                           registers set handle.
*
*       XHCI_Get_Doorbell_Regs_Handle       This function returns doorbell
*                                           registers set handle.
*
*       XHCI_Get_Run_Time_Regs_Handle       This function returns run time
*                                           registers set handle.
*
*       XHCI_Get_Stream_Ring_handle         This function returns handle of ring
*                                           associated with the stream.
*
*       XHCI_Get_Endpoint_Handle            This function is used to obtain
*                                           endpoint info control block of
*                                           particular endpoint.
*
*       XHCI_Get_Device_Handle              This function returns xHCD device
*                                           handle.
*
*       XHCI_Ring_Endpoint_Doorbell         This function writes doorbell
*                                           register to begin data transfer on
*                                           transfer ring.
*
*       XHCI_Init_IRP                       This function initializes xHCI
*                                           defined IRP structure.
*
*
*       XHCI_Allocate_Streams               This function allocates xHCD data
*                                           structure required for streams
*                                           management.
*
*       XHCI_Deallocate_Streams             This function frees stream related
*                                           data structures.
*       XHCI_Initialize_HW_Controller       This function performs HW specific
*                                           initializations.
*
*       XHCI_Disable_Interrupts             This function disables host
*                                           controller interrupts.
*
*       XHCI_Enable_Interrupts              This function enables host controller
*                                           interrupts.
*
*       XHCI_Start_Controller               This function starts host controller.
*
*       XHCI_Stop_Controller                This function stops host controller.
*
* DEPENDENCIES
*
*       nu_usb.h                            USB Definitions.
*       nu_usbh_xhci_dvm.h                  Device Manager Interface File.
* 
**************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "drivers/nu_usbh_xhci_ext.h"
#include "drivers/nu_drivers.h"

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Initiate_Bulk_Transfer
*
* DESCRIPTION
*
*       This function initiates data transfer over bulk endpoints.
*
* INPUTS
*
*       xhci                Pointer to xHC driver control block.
*       irp                 Pointer to IRP to be transferred.
*       xhci_dev            Pointer to xHCD device control block.
*       ep_index            Index of endpoint used for data transfer.
*       direction           Direction of data transfer IN/OUT.
*
* OUTPUTS
*
*       NU_SUCCESS          Successful completion.
*       NU_USB_INVLD_ARG    Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Initiate_Bulk_Transfer(NU_USBH_XHCI        *xhci,
                                   NU_USB_IRP          *irp,
                                   NU_USBH_XHCI_DEVICE *xhci_dev,
                                   UINT8               ep_index,
                                   BOOLEAN             direction)
{
    /* Progress status. */
    STATUS               status;

    /* Reference to the transfer descriptor constructed against TRBs. */

    /* Pointer to the address of ring associated with the endpoint. */
    NU_USBH_XHCI_RING    *ring;

    /* Pointer to the endpoint control block for which transfer is directed. */
    NU_USBH_XHCI_EP_INFO *ep_cb;

    /* Pointer endpoint context data structure. */
    NU_USBH_XHCI_EP_CTX  *ep_ctx;

    /* Max packet size for the endpoint. */
    UINT16               ep_max_pack = 0;

    /* TRBs required for the data transfer.*/
    UINT32               trbs_req    = 0;

    /* Loop index. */
    UINT32               index       = 0;

    BOOLEAN              zlp         = NU_FALSE;

    BOOLEAN              use_empty_packet;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(irp);
    NU_USB_PTRCHK(xhci_dev);

    /* Get the endpoint handle for which transfer is directed. */
    status = XHCI_Get_Endpoint_Handle( xhci_dev, ep_index, &ep_cb );

    if ( status == NU_SUCCESS )
    {
        /* Get the TRB ring handle. */
        status = XHCI_Get_Endpoint_Ring_Handle( ep_cb, &ring );

        if ( status == NU_SUCCESS )
        {
            status = XHCI_Get_Endpoint_Context( xhci_dev->dev_out_ctx, &ep_ctx, ep_index );

            if ( status == NU_SUCCESS )
            {
                /* Get the max packet size from the endpoint context data structure.*/
                ep_max_pack = EP_CTX_MAX_PACKET_SIZE(LE32_2_HOST(ep_ctx->ep_info2));

                if ( status == NU_SUCCESS )
                {
                    /* Find the number of TRBs required for the data transfer. */
                    for (index = 0 ; index < irp->length ; (index = index + XHCI_TRB_MAX_BUFF_SIZE))
                    {
                        ++trbs_req;
                    }

                    status = NU_USB_IRP_Get_Use_Empty_Pkt (irp, &use_empty_packet);

                    if ( status == NU_SUCCESS )
                    {
                        /* If use empty packet flag is true and irp length is multiple of
                         * maxP for out endpoints than transfer will end up with zlp.
                         */
                        if ( (use_empty_packet) && 
                             ((irp->length % ep_max_pack) == 0) &&
                             ( direction == 1) )
                        {
                           /* One additional TRB for ZLP.*/
                           ++trbs_req;
                           zlp = NU_TRUE;
                        }

                        /* Check if there is room available on the ring for queuing TRBs. */
                        status = XHCI_Room_On_Ring( xhci, ep_cb->ring, trbs_req,
                                                    XHCI_TRANS_RING_ALIGNMENT );
    
                        if ( status == NU_SUCCESS )
                        {
                            status = XHCI_Process_Bulk_IRP( xhci, irp, ring, 0, ep_max_pack, 
                                                            trbs_req, direction, zlp );
                            if ( status == NU_SUCCESS )
                            {
                                /* Ring the endpoint doorbell to initiate the data transfer. */
                                status = XHCI_Ring_Endpoint_Doorbell( xhci, xhci_dev->slot_id, 0,
                                                                      ep_index );
                            }
                        }
                    }
                }
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Process_Bulk_IRP
*
* DESCRIPTION
*
*       This function translates the usb IRP into xHCI specific IRP structure
*       and constructs TD for it.
*
* INPUTS
*
*       xhci               Pointer to xHC driver control block.
*       ring               Pointer to transfer ring.
*       irp                Pointer to IRP to be transferred.
*       stream_id          Stream ID to identify the stream used
*                          for data transfer.
*       ep_max_pack        Maximum packet size for endpoint.
*       trbs_req           Number of TRBs required for data transfer.
*       direction          Direction of data transfer IN/OUT.
*       use_empty_pkt      Terminate transfer by ZLP.
*
* OUTPUTS
*
*       NU_SUCCESS         Successful completion.
*       NU_USB_INVLD_ARG   Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Process_Bulk_IRP( NU_USBH_XHCI      *xhci,
                              NU_USB_IRP        *irp,
                              NU_USBH_XHCI_RING *ring,
                              UINT16            stream_id,
                              UINT16            ep_max_pack,
                              UINT32            trbs_req,
                              BOOLEAN           direction,
                              BOOLEAN           use_empty_pkt)
{
    /* Progress status. */
    STATUS           status;

    /* Pointer to xHCI specific IRP control block. */
    NU_USBH_XHCI_IRP *xhci_irp;

    /* Pointer to td constructed for transfer. */
    NU_USBH_XHCI_TD  *td;

    /* Parameters validation .*/
    NU_USB_PTRCHK(ring);

    /* Get the xhci_irp handle. */
    xhci_irp = &ring->xhci_irp;

    /* Initialize xhci specific IRP structure . */
    status = XHCI_Init_IRP( xhci_irp, irp, &td, ep_max_pack,
                            trbs_req, direction, use_empty_pkt );
    if ( status == NU_SUCCESS )
    {
        if ( ring->available_trbs > trbs_req )
        {
            td->num_trbs = trbs_req;
            xhci_irp->tx_length = irp->length;
        }
        else
        {
            td->num_trbs = ring->available_trbs;
            xhci_irp->tx_length = td->num_trbs * XHCI_TRB_MAX_BUFF_SIZE;
        }

        /* Call bulk endpoint TD handler. */
        status = XHCI_Handle_Bulk_TD( xhci, ring, xhci_irp,
                                      stream_id, ep_max_pack );
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Bulk_TD
*
* DESCRIPTION
*
*       This function parses the TRBs for bulk transfer and places them on
*       the Transfer ring for execution by the xHC. The ring pointer is also
*       incremented.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       ring                 Pointer to transfer ring.
*       xhci_irp             Pointer to IRP to be transferred.
*       stream_id            Stream ID to identify the stream used
*                            for data transfer.
*       max_packet           Maximum packet size for endpoint.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Handle_Bulk_TD(NU_USBH_XHCI      *xhci,
                           NU_USBH_XHCI_RING *ring,
                           NU_USBH_XHCI_IRP  *xhci_irp,
                           UINT16            stream_id,
                           UINT16            max_packet)
{
    /* Progress status.*/
    STATUS status = NU_SUCCESS;

    NU_USBH_XHCI_TD *td;
  /* TRB control field.*/
    UINT32 trb_ctrl_field;

    /* TRB buffer length. */
    UINT32 trb_buff_len;

    /* IRP data transfer length. */
    UINT32 irp_length;

    /* Ring cycle state. */
    UINT32 cycle_state;

    /* Transfer TRB, TD size field. */
    UINT32 td_size;

    /* TRBs required. */
    UINT32 trbs_req;

    UINT32 packet_tx;

    UINT32 index;

    BOOLEAN first_trb = NU_TRUE;

    /* Length of data transfer untill the current TRB. */
    UINT32 running_total    = 0;
    UINT32 packet_count     = 0;
    BOOLEAN buff_type_cach  = NU_FALSE;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(ring);
    NU_USB_PTRCHK(xhci_irp);

    /* Initialize the TD fields. */

    td = &xhci_irp->td;
    td->first_trb = ring->enqueue_ptr;
    trbs_req      = td->num_trbs;
    irp_length    = xhci_irp->tx_length;

    /* Get the ring cycle state field. */
    cycle_state = ring->cycle_state;

    /* Data transfer length for the first TRB.*/
    if (irp_length > XHCI_TRB_MAX_BUFF_SIZE)
    {
        trb_buff_len = XHCI_TRB_MAX_BUFF_SIZE;
    }
    else
    {
        trb_buff_len = irp_length;
    }

    /* If transfer is to be terminated by the zlp than decrement
     * TRB count as required TRBs count is valid for TRBs with data
     * length greater than 0.
     */
    if ( xhci_irp->empty_packet )
    {
        --trbs_req;
    }

    /* Count the number of packets required for data transfer. */
    for ( index = 0; index < irp_length; index += max_packet)
    {
        ++packet_count;
    }

    (VOID)NU_USB_IRP_Get_Buffer_Type_Cachable( xhci_irp->irp, &buff_type_cach );

    if ( buff_type_cach )
    {
        /* Normalize the buffer.*/
        status = xhci_normalize_buffer(xhci,(UINT8 *)xhci_irp->buffer,
                                       irp_length, 4096, xhci_irp->direction,
                                       &td->normal_buffer, &td->raw_buffer );
    }
   else
   {
       td->normal_buffer = (UINT8 *)xhci_irp->irp->buffer;
       td->raw_buffer    = (UINT8 *)xhci_irp->irp->buffer;
   }

    if ( status == NU_SUCCESS )
    {
        /* The Interrupt  on Short  Packet (ISP)  flag is  set in  every TRB of TD.
         * While the Interrupt On Completion (IOC) flags is set only in the last TRB
         * of TD . In case of streams,  td is terminated by the event data  TRB and
         * IOC flag is set in the event data TRB only.If transfer is terminated by zlp
         * then IOC is set in the zero length TRB.
         */

        do
        {
            td_size = irp_length - running_total;
            trb_ctrl_field = 0;

            /* TD Bytes remaining  = Sum of TRB transfer length fields from
             * and including current TRB to and including the last TRB.
             * If TD Bytes remaining is greater than 32768 then
               TD size = TD bytes remaining >> 10
               else
               TD size = 31;
             * Here td_size is used initially as TD bytes remaining - xHCI specs,
             * section 4.11.2.4. */

            packet_tx = (running_total + trb_buff_len)/max_packet;

            if ( trbs_req > 1 )
            {
                if ( (packet_count - packet_tx) > 31)
                {
                    td_size = 31;
                }
                else
                {
                    td_size = packet_count - packet_tx;
                }
            }
            else
            {
                td_size = 0;
            }

            if ( !stream_id )
            {
                /* Chain bit is set in the all but the last TRB of TD.*/
                if ( trbs_req > 1 )
                {
                    trb_ctrl_field  = SET_NORM_TRB_CHAIN | SET_NORM_TRB_ISP;
                }
                else
                {
                    if ( !xhci_irp->empty_packet )
                    {
                        trb_ctrl_field = SET_NORM_TRB_IOC | SET_NORM_TRB_ISP;
                        td->last_trb   = ring->enqueue_ptr;
                    }
                    else
                    {
                        trb_ctrl_field = SET_NORM_TRB_ISP;
                    }
                }
            }

            if (first_trb)
            {
                first_trb = NU_FALSE;
            }
            else
            {
                trb_ctrl_field |= ring->cycle_state;
            }

            /* Fill the required fields of transfer TRB and place it on the ring. */
            XHCI_FILL_TRB(ring->enqueue_ptr,
                         /* TRB parameter field - containing address of data to be transferred. */
                         (((UINT32)td->normal_buffer + running_total)),
                         ((UINT32)0x00),

                         /* Transfer TRB status field. */
                         (SET_NORM_TRB_LEN(trb_buff_len) | SET_NORM_TRB_TD_SIZE(td_size) |
                          SET_NORM_TRB_INTR_TARGET(0)),

                          /* Transfer TRB control field .*/
                         (trb_ctrl_field | SET_TRB_TYPE(XHCI_TRB_NORMAL)));

            /* Increment the ring enqueue pointer .*/
            (VOID)XHCI_Inc_Ring_Enqueue_Ptr( ring, NU_TRUE );

            /* Decrement the required TRB count. */
            --trbs_req;

            /* Increment the data transfer length until the current TRB. */
            running_total += trb_buff_len;

            /* Find the transfer buffer length of next TRB. */
            trb_buff_len = irp_length - running_total;

            if ( trb_buff_len > XHCI_TRB_MAX_BUFF_SIZE )
            {
                trb_buff_len = XHCI_TRB_MAX_BUFF_SIZE;
            }

        } while ( running_total < irp_length );

        if ( xhci_irp->empty_packet )
        {
             /* Fill the required fields of transfer TRB and place it on the ring. */
             XHCI_FILL_TRB(ring->enqueue_ptr,
                 /* TRB parameter field - containing address of data to be transferred. */
                 ((UINT32)0x00),
                 ((UINT32)0x00),

                 /* Transfer TRB status field. */
                 (SET_NORM_TRB_LEN(0) | SET_NORM_TRB_TD_SIZE(0) |
                  SET_NORM_TRB_INTR_TARGET(0)),

                  /* Transfer TRB control field .*/
                 (ring->cycle_state | SET_TRB_TYPE(XHCI_TRB_NORMAL)|SET_NORM_TRB_IOC)
                 ) ;
                 td->last_trb  = ring->enqueue_ptr;
                 (VOID)XHCI_Inc_Ring_Enqueue_Ptr( ring, NU_TRUE );

        }

        /* Initialize the event data TRB for streams.*/
        if ( stream_id )
        {
            /* Fill the required fields of Event data TRB and place it on the ring. */
            XHCI_FILL_TRB(ring->enqueue_ptr,
                         (stream_id),
                         (UINT32)0x00,
                         SET_NORM_TRB_INTR_TARGET(0),
                         (SET_NORM_TRB_IOC | SET_TRB_TYPE(XHCI_TRB_EVENT_DATA)));

            /* Increment the ring enqueue pointer .*/
            (VOID)XHCI_Inc_Ring_Enqueue_Ptr( ring, NU_TRUE );
        }
        /* Hand over TRBs to hardware. */
        td->first_trb->control |= HOST_2_LE32(cycle_state);
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Initiate_Control_Transfer
*
* DESCRIPTION
*
*       This function initiates data transfer over control endpoints.
*
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       irp                  Pointer to IRP to be transferred.
*       xhci_dev             Pointer to xHCD device control block.
*       ep_index             Index of endpoint used for data transfer.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Initiate_Control_Transfer(NU_USBH_XHCI        *xhci,
                                      NU_USB_IRP          *irp,
                                      NU_USBH_XHCI_DEVICE *xhci_dev,
                                      UINT8               ep_index)
{
    /* Progress status. */
    STATUS              status;

    /* Setup packet handle. */
    NU_USB_SETUP_PKT    *setup_pkt;

    /* Control IRP handle. */
    NU_USBH_CTRL_IRP    *ctrl_irp;

    /* Control transfer TD. */
    NU_USBH_XHCI_TD      *td;

    /* Pointer to the ring associated with the endpoint. */
    NU_USBH_XHCI_RING    *ring;

    /* Pointer to the endpoint control block for which transfer is directed. */
    NU_USBH_XHCI_EP_INFO *ep_cb;

    NU_USBH_XHCI_EP_CTX  *ep_ctx;

    NU_USBH_XHCI_IRP     *xhci_irp;

    /* TRBs required for control transfer .*/
    UINT32            trbs_req       = 0;

    UINT32            index          = 0;

    /* TRB control field .*/
    UINT32            trb_ctrl_field = 0;

    /* Transfer ring cycle state. */
    UINT32            cycle_state    = 0;

    UINT16            ep_max_pack    = 0;

    /* Setup packet fields. */
    UINT16            wValue         = 0;
    UINT16            wIndex         = 0;
    UINT16            wLength        = 0;
    BOOLEAN           direction_in   = NU_FALSE;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(irp);
    NU_USB_PTRCHK(xhci_dev);

    ctrl_irp = (NU_USBH_CTRL_IRP *)irp;
    setup_pkt = &ctrl_irp->setup_data;
    wValue    = LE16_2_HOST(setup_pkt->wValue);
    wIndex    = LE16_2_HOST(setup_pkt->wIndex);
    wLength   = LE16_2_HOST(setup_pkt->wLength);

    if ( setup_pkt->bRequest == USB_SET_ADDRESS )
    {
        /* The set address request is handled by the HW controller. */
        status =  XHCI_Queue_Address_Device_Command( xhci, irp, xhci_dev, xhci_dev->slot_id,
                                               ep_index, (UINT8)LE16_2_HOST(setup_pkt->wValue) );
    }
    else
    {
        /* Get the endpoint handle for which transfer is directed. */
        status = XHCI_Get_Endpoint_Handle( xhci_dev, 0x00, &ep_cb );

        if ( status == NU_SUCCESS )
        {
            status = XHCI_Get_Endpoint_Context( xhci_dev->dev_out_ctx, &ep_ctx, ep_index );

            if ( status == NU_SUCCESS )
            {
                /* Get the max packet size for control pipe. */
                ep_max_pack = EP_CTX_MAX_PACKET_SIZE(LE32_2_HOST(ep_ctx->ep_info2));

                /* Get the TRB ring handle associated with the control pipe. */
                status = XHCI_Get_Endpoint_Ring_Handle( ep_cb, &ring );

                /* Find the number of TRBs required for the data transfer. */
                for (index = 0 ; index < irp->length ; index += XHCI_MAX_TRB_SIZE)
                {
                    ++trbs_req;
                }

                if ( status == NU_SUCCESS )
                {
                    cycle_state = ring->cycle_state;

                    /* If there is space available on the ring for queuing TRBs.*/
                    status = XHCI_Room_On_Ring( xhci, ring, (trbs_req + 2), XHCI_TRANS_RING_ALIGNMENT );

                    if ( status == NU_SUCCESS )
                    {
                        /* Create the TD against the TRBs required. We create only one
                         * TD for the control transfer. The IOC flag is set only in the
                         * last(status stage) TRB of TD.ISP is not set in any of the
                         * TRB.Therefore event is generated only after completion of
                         * status stage.
                         */

                        /* Get the xhci_irp handle. */
                        xhci_irp = &ring->xhci_irp;
                        irp = &ctrl_irp->usb_irp;
                        direction_in = (setup_pkt->bmRequestType & USB_DIR_IN) ? NU_TRUE:NU_FALSE;

                        /* Initialize the xHCI IRP */
                        status = XHCI_Init_IRP( xhci_irp, irp, &td, ep_max_pack, (trbs_req + 2),
                                                !direction_in, NU_FALSE );

                        if ( status == NU_SUCCESS )
                        {
                            /* Initialize the TD control block fields. */
                            td->first_trb  = ring->enqueue_ptr;
                            td->num_trbs = trbs_req + 2;
                            /* Initialize the setup stage TRB - xHCI specs,section
                             * 6.4.1.2.1.
                             */
                            XHCI_FILL_TRB((ring->enqueue_ptr),
                                         (setup_pkt->bmRequestType | setup_pkt->bRequest << 8
                                         |( wValue << 16 )),
                                         (wIndex  | ((wLength) << 16)),
                                         (SET_NORM_TRB_LEN(8) | SET_NORM_TRB_INTR_TARGET(0)),
                                         (SET_NORM_TRB_IDT  | SET_TRB_TYPE(XHCI_TRB_SETUP)));

                            /* Increment the ring enqueue pointer. */
                            (VOID)XHCI_Inc_Ring_Enqueue_Ptr( ring, NU_TRUE );

                            if ( irp->length > 0 )
                            {
                                status = XHCI_Handle_Control_TD( xhci, ring, td, xhci_irp,
                                                                 direction_in, ep_max_pack);
                            }
                        }
                    }
                }
            }
        }

        if ( status == NU_SUCCESS )
        {
            /* Save the last TRB pointer in the TD. This will help us in
             * handling the control transfer completion.
             */
            td->last_trb = ring->enqueue_ptr;

            /* Direction of status stage is opposite to data stage. */
            if ((irp->length > 0) && (setup_pkt->bmRequestType & USB_DIR_IN))
            {
                trb_ctrl_field = 0;

            }
            else
            {
                trb_ctrl_field = SET_TRB_DIR_IN;
            }

            /* Initialize the status stage TRB - xHCI specs,section 6.4.1.2.3. */
            XHCI_FILL_TRB((ring->enqueue_ptr),
                          ((UINT32) 0x00),
                          ((UINT32) 0x00),
                          (SET_NORM_TRB_INTR_TARGET(0)),
                          (trb_ctrl_field | SET_NORM_TRB_IOC
                          | SET_TRB_TYPE(XHCI_TRB_STATUS)
                          | ring->cycle_state));

            /* Increment the ring enqueue pointer. */
            (VOID)XHCI_Inc_Ring_Enqueue_Ptr( ring, NU_TRUE );

             td->first_trb->control |= HOST_2_LE32(cycle_state);

             /* Ring the endpoint doorbell to start transfer. */
             status = XHCI_Ring_Endpoint_Doorbell( xhci, xhci_dev->slot_id, 0, ep_index );
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Control_TD
*
* DESCRIPTION
*
*       This function parses the TRBs for control transfer and places them on
*       the Transfer ring for execution by the xHC. The ring pointer is also
*       incremented.
*
*
* INPUTS
*
*       xhci                Pointer to xHC driver control block.
*       ring                Pointer to transfer ring.
*       td                  Pointer to transfer descriptor.
*       irp                 Pointer to IRP to be transferred.
*       data_in             Direction of data transfer 1 for IN and 0 for OUT.
*       max_packet          Maximum packet size for endpoint.
*
* OUTPUTS
*
*       NU_SUCCESS         Successful completion.
*       NU_USB_INVLD_ARG   Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Handle_Control_TD(NU_USBH_XHCI       *xhci,
                              NU_USBH_XHCI_RING  *ring,
                              NU_USBH_XHCI_TD    *td,
                              NU_USBH_XHCI_IRP   *xhci_irp,
                              BOOLEAN            data_in,
                              UINT16             max_packet)
{
    /* Progress status. */
    STATUS            status = NU_SUCCESS;
    /* Length of remaining data transfer. */
    UINT32            td_size        = 0;

    /* TRB buffer length. */
    UINT32            trb_buff_len   = 0;

    /* IRP data transfer length. */
    UINT32            irp_length     = 0;

    /* Length of data transfer until the current TRB. */
    UINT32            running_total  = 0;

    BOOLEAN           first_data_trb = NU_TRUE;

    /* Packets required for data transfer. */
    UINT32            packet_count   = 0;

    /* Number of packets transferred. */
    UINT32            packet_tx      = 0;

    /* Loop index. */
    UINT32            index          = 0;

    /* TRBs required. */
    UINT32 trbs_req                  = 0;

    UINT32  trb_ctrl_field           = 0;

    /* Decrement TRB count by 2 as setup and status stage TRBs are not included. */
    trbs_req   = xhci_irp->trbs_req - 2;
    irp_length = xhci_irp->tx_length;

    /* Count the number of packets required for data transfer. */
    for ( index = 0; index < irp_length; index += max_packet )
    {
        ++packet_count;
    }

    /* Normalize the buffer.*/
    status = xhci_normalize_buffer(xhci, xhci_irp->buffer, irp_length, 4096,
                                   xhci_irp->direction ,&td->normal_buffer,
                                   &td->raw_buffer );

    if ( status == NU_SUCCESS )
    {
        if ( irp_length > XHCI_TRB_MAX_BUFF_SIZE )
        {
            trb_buff_len = XHCI_TRB_MAX_BUFF_SIZE;
        }
        else
        {
            trb_buff_len = irp_length;
        }

        do
        {
            packet_tx = (running_total + trb_buff_len)/max_packet;

            if ( first_data_trb )
            {
                /* Set the TRB type of the first TRB of data stage as DATA stage
                 * TRB. Data stage TRB can be chained with the Normal TRBs if data
                 * spans multiple TRBs.
                 */
                trb_ctrl_field = SET_TRB_TYPE(XHCI_TRB_DATA);

                /* If there is data stage than we need to set the data
                 * transfer direction in the data stage TRB.
                 */
                if (data_in)
                {
                    trb_ctrl_field |= SET_TRB_DIR_IN;
                }

                first_data_trb = NU_FALSE;
            }
            else
            {
                trb_ctrl_field = SET_TRB_TYPE(XHCI_TRB_NORMAL);
            }

            if ( trbs_req > 1 )
            {
                trb_ctrl_field  |= SET_NORM_TRB_CHAIN ;

                /* TD Bytes remaining  = Sum of TRB transfer length fields from
                 * and including current TRB to and including the last TRB.
                 * Here td_size is used initially as TD bytes remaining - xHCI specs,
                 * section 4.11.2.4. */

                if ( (packet_count - packet_tx) > 31)
                {
                    td_size = 31;
                }
                else
                {
                    td_size = packet_count - packet_tx;
                }
            }
            else
            {
                td_size = 0;
            }

            trb_ctrl_field |= ring->cycle_state;

            /* Initialize the data stage TRB - xHCI
             * specs,section 6.4.1.2.2.
             */
            XHCI_FILL_TRB((ring->enqueue_ptr),
                          (((UINT32)td->normal_buffer + running_total)),
                          ((UINT32)0x00),
                          (SET_NORM_TRB_LEN(trb_buff_len) |
                           SET_NORM_TRB_TD_SIZE(td_size)|
                           SET_NORM_TRB_INTR_TARGET(0)),
                          (trb_ctrl_field));

            /* Increment the ring enqueue pointer. */
           (VOID)XHCI_Inc_Ring_Enqueue_Ptr( ring , NU_TRUE );

            /* Decrement the required TRB count. */
            --trbs_req;

            /* Increment the data transfer length until the current TRB. */
            running_total += trb_buff_len;

            /* Find the transfer buffer length of next TRB. */
            trb_buff_len = irp_length - running_total;

            if ( trb_buff_len > XHCI_TRB_MAX_BUFF_SIZE )
            {
                trb_buff_len = XHCI_TRB_MAX_BUFF_SIZE;
            }

        } while ( running_total < irp_length );
    }

    return ( status);
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Rx_Event
*
* DESCRIPTION
*
*       This function deciphers the transfer event TRB and invokes completion
*       handler function for respective transfer types.
*
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block
*       event_trb            Pointer to event TRB which generated the event.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Handle_Rx_Event(NU_USBH_XHCI     *xhci,
                            NU_USBH_XHCI_TRB *event_trb)
{
     /* Progress status. */
    STATUS               status;

    /* Pointer to xHCI device. */
    NU_USBH_XHCI_DEVICE  *xhci_dev;

    /* xhci endpoint control block handle for which event is generated. */
    NU_USBH_XHCI_EP_INFO *ep_info;

    /* Pointer to completed transfer descriptor. */
    NU_USBH_XHCI_IRP     *curr_irp;

    /* Ring on which data transferred occurred. */
    NU_USBH_XHCI_RING    *ring;

    /* xHCD based endpoint index. */
    UINT8                ep_index;

    /* Slot ID. */
    UINT8                slot_id ;

    /* Stream ID. */
    UINT16               stream_id;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(event_trb);

    /* Get the slot id from the event TRB.*/
    slot_id  = GET_EVENT_TRB_SLOT_ID(LE32_2_HOST(event_trb->control));

    /* Get the endpoint index from the transfer event TRB, our indexing is  0 based that's why
     * subtract 1.
     */
    ep_index = GET_EVENT_TRB_EP_ID(LE32_2_HOST(event_trb->control)) - 1;

    status = XHCI_Get_Device_Handle( xhci , slot_id, &xhci_dev );

    if ( status == NU_SUCCESS )
    {
        status = XHCI_Get_Endpoint_Handle( xhci_dev, ep_index, &ep_info );

        if ( status == NU_SUCCESS )
        {
            if ( ep_info->has_streams && (LE32_2_HOST(event_trb->control) & GET_EVENT_TRB_ED) )
            {
                /* Get the stream ID from the event TRB. */
                stream_id = LE32_2_HOST(event_trb->parameter[0]);

                /* Get the pointer to ring associated with the stream. */
                status = XHCI_Get_Stream_Ring_handle( ep_info->stream_info, stream_id, &ring );
                if ( status == NU_SUCCESS )
                {
                   status = XHCI_Handle_Stream_Completion( xhci, ep_info, event_trb, ring );
                }
            }
            else
            {
                /* Get the pointer for completed IRP. */
                curr_irp = &ep_info->ring->xhci_irp;

                if (ep_index == USB_EP_CTRL )
                {
                    status = XHCI_Complete_Control_Transfer(xhci,  ep_info, event_trb,  curr_irp);
                }
                else if ( xhci_dev->ep_type[ep_index] == USB_EP_ISO)
                {
                }
                else
                {
                    status = XHCI_Complete_Bulk_Transfer(xhci, ep_info, event_trb, curr_irp);
                }
            }
        }
    }

    return  ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Stream_Completion
*
* DESCRIPTION
*
*       This function handles data transfer completion over bulk streaming
*       endpoints.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       ep_info              Pointer to xHCD endpoint info structure.
*       event_trb            Pointer to event TRB which generated the event.
*       ring                 Pointer to transfer ring used for data transfer.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Handle_Stream_Completion(NU_USBH_XHCI         *xhci,
                                     NU_USBH_XHCI_EP_INFO *ep_info,
                                     NU_USBH_XHCI_TRB     *event_trb,
                                     NU_USBH_XHCI_RING    *ring)
{
     /* Progress status. */
    STATUS            status      = NU_SUCCESS;
    STATUS            int_status  = NU_SUCCESS;

    /* Pointer to completed transfer descriptor. */
    NU_USBH_XHCI_TD   *curr_td    = NU_NULL;

    NU_USBH_XHCI_IRP  *xhci_irp   = NU_NULL;

    /* TRB completion code. */
    UINT32            trb_code    = 0;

    /* Length of data transferred. */
    UINT32            data_length = 0;

    /* Loop index. */
    UINT16            index       = 0;

    UINT16            stream_id;

    UINT8             ep_index;

    UINT8             slot_id;

    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(ep_info);
    NU_USB_PTRCHK(event_trb);
    NU_USB_PTRCHK(ring);

    xhci_irp  = &ring->xhci_irp;
    curr_td   = &xhci_irp->td;
    stream_id = LE32_2_HOST(event_trb->parameter[0]);
    ep_index  = GET_EVENT_TRB_EP_ID(LE32_2_HOST(event_trb->control)) - 1;
    slot_id   = GET_EVENT_TRB_SLOT_ID(LE32_2_HOST(event_trb->control));

    status = xhci_transfer_done (xhci, xhci_irp->buffer, curr_td->normal_buffer,
                                 curr_td->raw_buffer, xhci_irp->tx_length,
                                 1, xhci_irp->direction );

    if ( status == NU_SUCCESS )
    {
        /* Investigate the TD completion status .*/
        switch(trb_code)
        {
            case XHCI_TRB_COMPL_SUCCESS:
            break;

            case XHCI_TRB_COMPL_STALL:
                 ep_info->ep_state = XHCI_EP_STATE_HALTED;
                 int_status        = NU_USB_STALL_ERR;
            break;

            case XHCI_TRB_COMPL_SHORT_TX:
                 int_status = NU_USB_DATA_UNDERRUN;
            break;

            default:
              int_status = XHCI_ERR_FATAL;
            break;
        }

        data_length = GET_EVENT_TRB_LENGTH(LE32_2_HOST(event_trb->status));

        /* Increment ring dequeue pointer. */
        for ( index = 0; index < curr_td->num_trbs; index ++)
        {
            (VOID)XHCI_Inc_Ring_Dequeue_Ptr( ring, NU_FALSE );
        }

        status = XHCI_Process_Remaining_IRP( xhci, ring, xhci_irp,
                                             int_status, data_length,
                                             ep_index, stream_id, slot_id );
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Complete_Bulk_Transfer
*
* DESCRIPTION
*
*       This function handles data transfer completion over bulk endpoints.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       ep_info              Pointer to xHCD endpoint info structure.
*       event_trb            Pointer to event TRB which generated the event.
*       curr_irp             Pointer to completed irp.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Complete_Bulk_Transfer(NU_USBH_XHCI  *xhci,
                            NU_USBH_XHCI_EP_INFO *ep_info,
                            NU_USBH_XHCI_TRB     *event_trb,
                            NU_USBH_XHCI_IRP     *curr_irp)
{
    /* Progress status. */
    STATUS            status          = NU_SUCCESS;
    STATUS            int_status      = NU_SUCCESS;

    /* Ring associated with the data transfer completion. */
    NU_USBH_XHCI_RING *ep_ring;

    NU_USBH_XHCI_TRB  *td_first_trb;

    NU_USBH_XHCI_TRB  *last_trb;

    /* Completed IRP. */
    NU_USBH_XHCI_TD   *td;

    /* TRB completion code. */
    UINT32            trb_code;

    /* Length of data transferred. */
    UINT32            data_length = 0;
    UINT8             ep_index;
    UINT8             slot_id;

    /* Parameters validation .*/
    NU_USB_PTRCHK(ep_info);
    NU_USB_PTRCHK(event_trb);
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(curr_irp);

    td              = &curr_irp->td;
    ep_ring         = ep_info->ring;
    trb_code        = GET_EVENT_TRB_COMP_CODE(LE32_2_HOST(event_trb->status));
    last_trb        =( NU_USBH_XHCI_TRB * ) LE32_2_HOST(event_trb->parameter[0]);
    td_first_trb    = td->first_trb;
    ep_index        = GET_EVENT_TRB_EP_ID(LE32_2_HOST(event_trb->control)) - 1;
    slot_id         = GET_EVENT_TRB_SLOT_ID(LE32_2_HOST(event_trb->control));

    if ( (td->raw_buffer  != NU_NULL) &&
         (td->raw_buffer  != curr_irp->irp->buffer) )
    {
        status = xhci_transfer_done(xhci, curr_irp->buffer,
                                    td->normal_buffer, td->raw_buffer,
                                    curr_irp->tx_length, 1, !curr_irp->direction );
    }

    if ( status == NU_SUCCESS )
    {
        /* Investigate the TD completion status .*/
        switch(trb_code)
        {
            case XHCI_TRB_COMPL_SUCCESS:

                if (last_trb != td->last_trb)
                {
                    status = XHCI_ERR_FATAL;
                    while(1);
                }
            break;

            case XHCI_TRB_COMPL_STALL:
                 ep_info->ep_state = XHCI_EP_STATE_HALTED;
                 int_status        = NU_USB_STALL_ERR;
            break;

            case XHCI_TRB_COMPL_SHORT_TX:
                 int_status = NU_USB_DATA_UNDERRUN;
            break;

            default:
              int_status = NU_USB_DEVICE_NOT_RESPONDING;
            break;
        }

        if ( int_status == NU_SUCCESS )
        {
            /* If every thing goes well. */
            data_length = curr_irp->tx_length;
        }
        else
        {
            /* If transfer is terminated by short length packet or stall, then
             * we have to go through the TD to find the length of data
             * transferred by the TRBs.
             */
            if ( td->num_trbs == 1 )
            {
                /* If there is only one TRB in td then we just need to subtract
                 * the event TRB transfer length from the IRP data length.The
                 * event TRB Transfer length indicates the residue number of
                 * bytes not transferred - xHCI specs, section 6.4.2.1,
                 * table 83.
                 */
                data_length = GET_NORM_TRB_LEN(LE32_2_HOST(td_first_trb->status)) -
                                       GET_EVENT_TRB_LENGTH(LE32_2_HOST(event_trb->status));
            }
            else
            {
                /* Start from the first TRB in TD and move ahead until last
                 * TRB (last_trb) is reached accumulate the data length
                 * transferred by each TRB.
                 */
                while ( td_first_trb != last_trb )
                {
                    data_length += GET_NORM_TRB_LEN(LE32_2_HOST(td_first_trb->status));
                    ++td_first_trb;

                    /* In case of link TRB load the td_first_trb with the
                     * address of first TRB in the next segment.
                     */
                    if (GET_TRB_TYPE(LE32_2_HOST(td_first_trb->control)) == XHCI_TRB_LINK)
                    {
                        td_first_trb = (NU_USBH_XHCI_TRB *)
                                                   LE32_2_HOST(td_first_trb->parameter[0]);
                    }
                }

                data_length += GET_NORM_TRB_LEN(LE32_2_HOST(td_first_trb->status)) -
                                       GET_EVENT_TRB_LENGTH(LE32_2_HOST(event_trb->status));
            }
        }

        /* Increment ring dequeue pointer. */
        while ( ep_ring->dequeue_ptr != last_trb )
        {
            (VOID)XHCI_Inc_Ring_Dequeue_Ptr( ep_ring, NU_FALSE );
        }

        (VOID)XHCI_Inc_Ring_Dequeue_Ptr(  ep_ring, NU_FALSE );

        status = XHCI_Process_Remaining_IRP( xhci, ep_ring, curr_irp, int_status,
                                             data_length, ep_index, 0, slot_id );

    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Process_Remaining_IRP
*
* DESCRIPTION
*
*       This function processes remaining data in the IRP for large data
*       transfer.
*
*
* INPUTS
*
*       xhci                Pointer to xHC driver control block.
*       ep_info             Pointer to xHCD endpoint info structure.
*       event_trb           Pointer to event TRB which generated the event.
*       xhci_irp            Pointer to IRP completed.
*       irp_status          Status of completed IRP.
*       data_length         Length of data transferred by the IRP.
*       ep_index            Endpoint index.
*       strm_id             Stream ID in case of bulk streaming EPs.
*       slot_id             Slot ID for device.
*
* OUTPUTS
*
*       NU_SUCCESS          Successful completion.
*       NU_USB_INVLD_ARG    Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Process_Remaining_IRP(NU_USBH_XHCI      *xhci,
                                  NU_USBH_XHCI_RING *ring,
                                  NU_USBH_XHCI_IRP  *xhci_irp,
                                  STATUS            irp_status,
                                  UINT32            data_length,
                                  UINT8             ep_index,
                                  UINT16            strm_id,
                                  UINT8             slot_id)
{
    STATUS status;
    BOOLEAN retire_irp = NU_TRUE;
    NU_USB_IRP *irp = NU_NULL;

    irp = xhci_irp->irp;

    /* Update the total transferred length. */
    xhci_irp->total_length += data_length;

    if ( irp_status == NU_SUCCESS )
    {
        /* Data is remaining so process it first. */
        if ( (xhci_irp->rem_length - data_length) > 0 )
        {
            xhci_irp->buffer += data_length;
            xhci_irp->rem_length -= data_length;
            xhci_irp->rem_trbs -= xhci_irp->td.num_trbs;

            if ( xhci_irp->rem_trbs > ring->available_trbs )
            {
                xhci_irp->td.num_trbs = ring->available_trbs;
                xhci_irp->tx_length   = xhci_irp->td.num_trbs * XHCI_TRB_MAX_BUFF_SIZE;
            }
            else
            {
                xhci_irp->td.num_trbs = xhci_irp->rem_trbs;
                xhci_irp->tx_length   = xhci_irp->rem_length;
            }

            status = XHCI_Handle_Bulk_TD( xhci, ring,  xhci_irp, 0 , xhci_irp->max_p );

            if ( status == NU_SUCCESS )
            {
                /* Ring the endpoint doorbell to initiate the data transfer. */
                status = XHCI_Ring_Endpoint_Doorbell( xhci, slot_id, 0, ep_index );
                retire_irp = NU_FALSE;
            }
        }
    }

    if ( retire_irp == NU_TRUE )
    {
        xhci_irp->irp = NU_NULL;
        status = XHCI_Handle_IRP_Completion( irp, irp_status, xhci_irp->total_length );
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Complete_Control_Transfer
*
* DESCRIPTION
*
*       This function handles data transfer completion over control endpoint.
*
* INPUTS
*
*       xhci               Pointer to xHC driver control block.
*       ep_info            Pointer to xHCD endpoint info structure.
*       event_trb          Pointer to event TRB which generated the event.
*       xhci_irp           Pointer to completed IRP.
*
* OUTPUTS
*
*       NU_SUCCESS         Successful completion.
*       NU_USB_INVLD_ARG   Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Complete_Control_Transfer(NU_USBH_XHCI           *xhci,
                                      NU_USBH_XHCI_EP_INFO   *ep_info,
                                      NU_USBH_XHCI_TRB       *event_trb,
                                      NU_USBH_XHCI_IRP       *xhci_irp)
{
    /* Progress status .*/
    STATUS            status          = NU_SUCCESS;
    STATUS            int_status      = NU_SUCCESS;

    NU_USBH_XHCI_RING *ep_ring        = NU_NULL;

    /* First TRB in TD. */
    NU_USBH_XHCI_TRB  *td_first_trb   = NU_NULL;

    /* Last TRB in TD. */
    NU_USBH_XHCI_TRB  *last_trb       = NU_NULL;

    NU_USB_IRP        *irp            = NU_NULL;

    NU_USBH_XHCI_TD   *td             = NU_NULL;

    /* TRB completion code. */
    UINT32            trb_code        = 0;

    /* Data length transferred. */
    UINT32            data_length     = 0;

    UINT32            num_data_trbs   = 0;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(ep_info);
    NU_USB_PTRCHK(event_trb);
    NU_USB_PTRCHK(xhci_irp);

    ep_ring         = ep_info->ring;
    trb_code        = GET_EVENT_TRB_COMP_CODE(LE32_2_HOST(event_trb->status));
    last_trb        = ( NU_USBH_XHCI_TRB * ) LE32_2_HOST(event_trb->parameter[0]);
    irp             = xhci_irp->irp;
    td              = &xhci_irp->td;
    td_first_trb    = td->first_trb;
    num_data_trbs   = td->num_trbs - 2;

    if ( xhci_irp->tx_length > 0 )
    {
        status = xhci_transfer_done(xhci, xhci_irp->buffer,
                                    td->normal_buffer, td->raw_buffer,
                                    xhci_irp->tx_length, 1, !xhci_irp->direction);
    }

    if ( status == NU_SUCCESS )
    {
        /* Investigate the TD completion status .*/
        switch(trb_code)
        {
            case XHCI_TRB_COMPL_SUCCESS:
                /* Double check - The HW may be buggy. */
               if (last_trb != td->last_trb)
               {
                   status = XHCI_ERR_FATAL;
                   while(1);
               }
            break;

            case XHCI_TRB_COMPL_STALL:
                 ep_info->ep_state = XHCI_EP_STATE_HALTED;
                 int_status        = NU_USB_STALL_ERR;
            break;

            case XHCI_TRB_COMPL_SHORT_TX:
                 int_status = NU_USB_DATA_UNDERRUN;
            break;

            default:
              int_status = NU_USB_DEVICE_NOT_RESPONDING;
            break;
        }

        if ( int_status == NU_SUCCESS )
        {
            /* If every thing goes well. */
            data_length = irp->length;
        }
        else if ( num_data_trbs )
        {
            ++td_first_trb;

            /* If transfer is terminated by short length packet or stall, then
             * we have to go through the TD to find the length of data
             * transferred by the TRBs.
             */
            if ( num_data_trbs == 1 )
            {
                /* If there is only one TRB in td then we just need to subtract
                 * the event TRB transfer length from the IRP data length.The
                 * event TRB Transfer length indicates the residue number of
                 * bytes not transferred - xHCI specs, section 6.4.2.1,
                 * table 83.
                 */
                data_length = GET_NORM_TRB_LEN(LE32_2_HOST(td_first_trb->status)) -
                                       GET_EVENT_TRB_LENGTH(LE32_2_HOST(event_trb->status));
            }
            else
            {
                /* Start from the first TRB in TD and move ahead until last
                 * TRB (last_trb) is reached accumulate the data length
                 * transferred by each TRB.
                 */
                --last_trb;

                while ( td_first_trb != last_trb )
                {
                    data_length += GET_NORM_TRB_LEN(LE32_2_HOST(td_first_trb->status));
                    ++td_first_trb;

                    /* In case of link TRB load the td_first_trb with the
                     * address of first TRB in the next segment.
                     */
                    if (GET_TRB_TYPE(LE32_2_HOST(td_first_trb->control)) == XHCI_TRB_LINK)
                    {
                        td_first_trb = (NU_USBH_XHCI_TRB *)
                                                   LE32_2_HOST(td_first_trb->parameter[0]);
                    }
                }

                data_length += GET_NORM_TRB_LEN(LE32_2_HOST(td_first_trb->status)) -
                                       GET_EVENT_TRB_LENGTH(LE32_2_HOST(event_trb->status));
                ++last_trb;
            }
        }

        /* Increment ring dequeue pointer. */
        while ( ep_ring->dequeue_ptr != last_trb )
        {
            (VOID)XHCI_Inc_Ring_Dequeue_Ptr( ep_ring, NU_FALSE );
        }

        (VOID)XHCI_Inc_Ring_Dequeue_Ptr(  ep_ring, NU_FALSE );

        xhci_irp->irp = NU_NULL;
        status = XHCI_Handle_IRP_Completion( irp, int_status, data_length );
    }

    return( status );
}
/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_IRP_Completion
*
* DESCRIPTION
*
*       This function sets IRP status, length of data transfer and call
*       callback function.
*
*
* INPUTS
*
*       irp                  Pointer to completed IRP.
*       irp_status           IRP completion status.
*       data_length          Length of data transferred by the IRP.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Handle_IRP_Completion(NU_USB_IRP  *irp,
                                  STATUS      irp_status,
                                  UINT32      data_length)
{
    /* Progress status. */
    STATUS              status = NU_SUCCESS;

    /* Call-back function of the IRP. */
    NU_USB_IRP_CALLBACK callback;

    /* Pipe on which IRP is submitted. */
    NU_USB_PIPE         *pipe_ptr ;

    BOOLEAN              short_ok = NU_FALSE;

    irp->actual_length = data_length;

    status = NU_USB_IRP_Get_Accept_Short_Packets(irp, &short_ok);

    if ( status == NU_SUCCESS )
    {
        if((short_ok == NU_TRUE) && (irp_status == NU_USB_DATA_UNDERRUN))
        {
            irp_status = NU_SUCCESS;
        }

        irp->status = irp_status;

        /* Get the call-back function. */
        status = NU_USB_IRP_Get_Callback ( irp, &callback );
        if ( status == NU_SUCCESS )
        {
            /* Get the pipe on which this IRP is submitted. */
            status = NU_USB_IRP_Get_Pipe (irp, &pipe_ptr );

            /* If no errors so far. */
            if ( status == NU_SUCCESS )
            {
                /* If call-back function exists. */
                if ( callback != NU_NULL )
                {
                    /* Report the completion of the IRP. */
                    callback ( pipe_ptr, irp );
                }
            }
        }
    }

    /* Return the status. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Lock
*
* DESCRIPTION
*
*       This function locks xHCD for exclusive access.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Lock(NU_USBH_XHCI *xhci)
{
    STATUS status;

    NU_USB_PTRCHK(xhci);

    status = NU_Obtain_Semaphore(&xhci->protect_sem, NU_SUSPEND);

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Unlock
*
* DESCRIPTION
*
*       This function unlocks xHCD.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Unlock(NU_USBH_XHCI *xhci)
{
    STATUS status = NU_SUCCESS;

    NU_USB_PTRCHK(xhci);

    status = NU_Release_Semaphore( &xhci->protect_sem );

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Ring_Endpoint_Doorbell
*
* DESCRIPTION
*
*       This function writes doorbell register to begin data transfer on
*       transfer ring.
*
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       slot_id              Slot ID of device towards which transfer
*                            is directed.
*       stream_id            Stream ID to identify the stream.
*       ep_index             Index of endpoint used for data transfer.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS XHCI_Ring_Endpoint_Doorbell(NU_USBH_XHCI *xhci,
                                   UINT8        slot_id,
                                   UINT16       stream_id,
                                   UINT8        ep_index)
{
    STATUS                status = NU_SUCCESS;
    NU_USBH_XHCI_DB_ARRAY *db_reg = NU_NULL;
    UINT32                 field  = 0;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);

    status  = XHCI_Get_Doorbell_Regs_Handle( &xhci->reg_base, &db_reg );
    if (status == NU_SUCCESS)
    {
        XHCI_HW_READ32(xhci, &db_reg->db_array[slot_id],field);
        field |= XHCI_DB_SET_STREAM_ID(stream_id) | XHCI_DB_SET_ENDPOINT_INDEX(ep_index);
        XHCI_HW_WRITE32(xhci, &db_reg->db_array[slot_id],field);
    }

   return ( status );
}

/*=======================================================================
                        Command Management Unit
=======================================================================*/

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Queue_Command
*
* DESCRIPTION
*
*       This function places command TRB on command ring and handles its
*       completion.
*
*
* INPUTS
*
*       xhci       Pointer to xHC driver control block.
*       trb_info1  TRB parameter field.
*       trb_info2  TRB parameter field.
*       trb_info3  TRB status field.
*       trb_info4  TRB control field.
*
* OUTPUTS
*
*       NU_SUCCESS            Successful completion.
*       NU_USB_INVLD_ARG      Any of the input argument in invalid.
*       TRB_COMPLETION_CODES  Number of TRB completion codes as defined by the
*                             specs.
*
**************************************************************************/
STATUS XHCI_Queue_Command(NU_USBH_XHCI *xhci,
                          UINT32       trb_info1,
                          UINT32       trb_info2,
                          UINT32       trb_info3,
                          UINT32       trb_info4)
{
    /* Progress status.*/
    STATUS            status     = NU_SUCCESS;

    /* Command completion status. */
    UINT32            cmd_status = 0;

    /* Pointer to host controller Command Ring. */
    NU_USBH_XHCI_RING *cmd_ring  = NU_NULL;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);

    /* Get the command ring handle from the command ring control block .*/
    status = XHCI_Get_Command_Ring_Handle( &xhci->cmd_ring_cb, &cmd_ring );

    if ( status == NU_SUCCESS )
    {
        /* Check if there is room available on the ring .*/
        status  = XHCI_Room_On_Ring( xhci, cmd_ring, 1, XHCI_CMD_RING_ALIGNMENT );

        if( status == NU_SUCCESS )
        {
            /* Fill the command TRB.*/
            XHCI_FILL_TRB(cmd_ring->enqueue_ptr,
                          trb_info1,
                          trb_info2,
                          trb_info3,
                          (trb_info4 | cmd_ring->cycle_state));

            /* Increment the command ring enqueue pointer. */
            status = XHCI_Inc_Ring_Enqueue_Ptr( cmd_ring, NU_TRUE );

            if ( status == NU_SUCCESS )
            {
                /* Ring the command doorbell. */
                status = XHCI_Ring_Command_Doorbell( xhci );

                if ( status == NU_SUCCESS )
                {
                    /* Acquire the command completion lock.*/
                    status = NU_Obtain_Semaphore( &xhci->cmd_ring_cb.cmd_comp_lock,
                                                  (NU_PLUS_Ticks_Per_Second * 5) );

                    if ( status == NU_SUCCESS )
                    {
                        /* Only command failure codes are handled here. The success code
                         * is passed to calling function of respective commands.
                         */
                        cmd_status = xhci->cmd_ring_cb.cmd_status;
                        status = cmd_status;

                        switch ( cmd_status )
                        {
                            case XHCI_TRB_COMPL_SUCCESS:
                                status = NU_SUCCESS;
                            break;

                            case XHCI_TRB_COMPL_NO_MEM:
                                status = XHCI_TRB_COMPL_NO_MEM;
                            break;

                            case XHCI_TRB_COMPL_BW_ERR:
                                status = XHCI_TRB_COMPL_BW_ERR;
                            break;

                            case XHCI_TRB_COMPL_NOSTOTS_ERR:
                                status = XHCI_TRB_COMPL_NOSTOTS_ERR;
                            break;

                            case XHCI_TRB_COMPL_EBADSLOT:
                                status = XHCI_TRB_COMPL_EBADSLOT;
                            break;

                            case XHCI_TRB_COMPL_PARAM_INVAID:
                                status = XHCI_TRB_COMPL_PARAM_INVAID;
                            break;

                            case XHCI_TRB_COMPL_CTX_STATE:
                                status = XHCI_TRB_COMPL_CTX_STATE;
                            break;

                            case XHCI_TRB_COMPL_INVALID_TRB_ERR:
                                status = XHCI_TRB_COMPL_INVALID_TRB_ERR;
                            break;

                            case XHCI_TRB_RSVD:
                                status = NU_SUCCESS;
                            break;

                            default:
                                status = XHCI_ERR_FATAL;
                            break;
                        }
                    }
                }
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Queue_Slot_Control_Command
*
* DESCRIPTION
*
*       This function parses slot control command TRB and queues the command
*       on the command ring.
*
*
* INPUTS
*
*       xhci       Pointer to xHC driver control block.
*       trb_type   Identify TRB type as ENABLE_SLOT or DISABLE SLOT TRB.
*       slot_id    Slot ID of device - required for disable slot command only.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Queue_Slot_Control_Command(NU_USBH_XHCI *xhci,
                                       UINT32       trb_type,
                                       UINT8        slot_id)
{
    /* Progress status. */
    STATUS status     = NU_SUCCESS;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);

    /* Place the command on the command ring .*/
    status = XHCI_Queue_Command( xhci, 0, 0, 0,
                               (SET_TRB_SLOT_ID(slot_id) | SET_TRB_TYPE(trb_type)) );

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*      XHCI_Handle_Reset_Device_Command
*
* DESCRIPTION
*
*      This function parses Reset device command TRB and queues the command
*      on the command ring.
*
* INPUTS
*
*      xhci       Pointer to xHC driver control block.
*      slot_id    Slot ID for device.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Queue_Reset_Device_Command(NU_USBH_XHCI *xhci,
                                       UINT8         slot_id)
{
    /* Progress status. */
    STATUS status = NU_SUCCESS;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);

    status = XHCI_Queue_Command( xhci, 0, 0, 0,
                                (SET_TRB_SLOT_ID(slot_id) | SET_TRB_TYPE(XHCI_TRB_RESET_DEV)) );

    return ( status );

}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Queue_NOOP_Command
*
* DESCRIPTION
*
*       This function parses NOOP command TRB and queues the command on the
*       command ring.
*
* INPUTS
*
*       xhci    Pointer to xHC driver control block.
*
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Queue_NOOP_Command(NU_USBH_XHCI *xhci)
{
    /* Progress status. */
    STATUS status = NU_SUCCESS;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);

    status = XHCI_Queue_Command( xhci, 0, 0, 0,
                                 SET_TRB_TYPE(XHCI_TRB_CMD_NOOP) );

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*      XHCI_Queue_Set_TR_Dequeue_Command
*
* DESCRIPTION
*
*      This function parses set TR dequeue command TRB and queues the command
*      on the command ring.
*
* INPUTS
*
*      xhci                Pointer to xHC driver control block.
*      deq_ptr             Ring dequeue pointer, used by host controller
*                          for retrieving TRBs.
*      ep_index            Index of endpoint for which command is directed.
*      cycle_state         Transfer ring cycle bit.
*      stream_id           Stream ID to identify the stream ring.
*      slot_id             Slot ID of device for which command is directed.
*
* OUTPUTS
*
*      NU_SUCCESS          Successful completion.
*      NU_USB_INVLD_ARG    Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Queue_Set_TR_Dequeue_Command(NU_USBH_XHCI  *xhci,
                                          VOID         *deq_ptr,
                                          UINT8        ep_index,
                                          UINT32       cycle_state,
                                          UINT16       stream_id,
                                          UINT8        slot_id)
{
    STATUS status = NU_SUCCESS;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(deq_ptr);

    status = XHCI_Queue_Command( xhci, (UINT32)deq_ptr | cycle_state, 0,
                                 SET_STRM_ID_FOR_TRB(stream_id),
                                 SET_TRB_SLOT_ID(slot_id) | SET_TRB_EP_ID(ep_index) |
                                 SET_TRB_TYPE(XHCI_TRB_SET_DEQ) );

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*      XHCI_Queue_Stop_Endpoint_Command
*
* DESCRIPTION
*
*      This function parses stop endpoint command TRB and queues the command
*      on the command ring.
*
* INPUTS
*
*      xhci                 Pointer to xHC driver control block.
*      ep_index             Index of endpoint for which command is directed.
*      cycle_state          Transfer ring cycle bit.
*      stream_id            Stream ID to identify the stream ring.
*      slot_id              Slot ID of device for which command is directed.
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*      NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Queue_Stop_Endpoint_Command(NU_USBH_XHCI *xhci,
                                        UINT8        ep_index,
                                        UINT32       cycle_state,
                                        UINT16       stream_id,
                                        UINT8        slot_id)
{
    STATUS status = NU_SUCCESS;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);

    status = XHCI_Queue_Command( xhci, 0, 0, 0,
                                 SET_TRB_SLOT_ID(slot_id) |
                                 SET_TRB_EP_ID(ep_index)  |
                                 SET_TRB_TYPE(XHCI_TRB_STOP_EP));

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*      XHCI_Queue_Configure_EP_Command
*
* DESCRIPTION
*
*      This function parses configure endpoint command TRB and queues the
*      command on the command ring.
*
* INPUTS
*
*      xhci                 Pointer to xHC driver control block.
*      in_ctx               xHC device input context used for passing
*                           command parameters.
*      slot_id              Slot ID of device for which command is directed.
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*      NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Queue_Configure_EP_Command(NU_USBH_XHCI         *xhci,
                                       NU_USBH_XHCI_RAW_CTX *in_ctx,
                                       UINT8                slot_id)
{
    STATUS status = NU_SUCCESS;

    /* Parameter validation. */
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(in_ctx);
    NU_USB_PTRCHK(in_ctx->ctx_buffer);

    status = XHCI_Queue_Command( xhci, (UINT32)in_ctx->ctx_buffer, 0, 0,
                                 SET_TRB_SLOT_ID(slot_id) |
                                 SET_TRB_TYPE(XHCI_TRB_CONFIG_EP) );

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*      XHCI_Queue_Evaluate_Context_Command
*
* DESCRIPTION
*
*      This function parses evaluate context command TRB and queues the
*      command on the command ring.
*
* INPUTS
*
*      xhci                 Pointer to xHC driver control block.
*      in_ctx               xHC driver device input context used for passing
*                           command parameters.
*      slot_id              Slot ID of device for which command is directed.
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*      NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Queue_Evaluate_Context_Command( NU_USBH_XHCI  *xhci,
                                     NU_USBH_XHCI_RAW_CTX *in_ctx,
                                     UINT8                slot_id )
{
    /* Progress status .*/
    STATUS   status     = NU_SUCCESS;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(in_ctx);

    /* Place the command on the ring.*/
    status = XHCI_Queue_Command( xhci, (UINT32)in_ctx->ctx_buffer, 0, 0,
                           SET_TRB_SLOT_ID(slot_id) | SET_TRB_TYPE(XHCI_TRB_EVAL_CONTEXT) );

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*      XHCI_Queue_Reset_Endpoint_Command
*
* DESCRIPTION
*
*      This function parses reset endpoint command TRB and queues the
*      command on the command ring.
*
* INPUTS
*
*      xhci                 Pointer to xHC driver control block.
*      slot_id              Slot ID of device for which command is directed.
*      ep_index             Index of endpoint effected by the command
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*      NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Queue_Reset_Endpoint_Command( NU_USBH_XHCI *xhci,
                                          UINT8        slot_id,
                                          UINT8        ep_index )
{
    STATUS status = NU_SUCCESS;

    /* These declarations are for the Sanity checks.*/
    NU_USBH_XHCI_DEVICE *xhci_dev = NU_NULL;
    NU_USBH_XHCI_EP_CTX *ep_ctx   = NU_NULL;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);

    xhci_dev = xhci->device[slot_id];

    /* Place Address Device Command on the Command ring. */
    status = XHCI_Queue_Command( xhci, 0, 0, 0,
                                 SET_TRB_SLOT_ID(slot_id) |
                                 SET_TRB_EP_ID(ep_index)  |
                                 SET_TRB_TYPE(XHCI_TRB_RESET_EP) );

   /* Sanity check for faulty HW. */
    if ( status == NU_SUCCESS )
    {
        status = XHCI_Get_Endpoint_Context( xhci_dev->dev_out_ctx,
                                        &ep_ctx, ep_index );

        if (status == NU_SUCCESS)
        {
            if ((ep_ctx->ep_info1 & EP_CTX_STATE_MASK) !=
                                        XHCI_EP_STATE_STOPPED)
            {
                /* Need to issue configure endpoint again.?*/
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Queue_Address_Device_Command
*
* DESCRIPTION
*
*       This function parses address device command TRB and queues the
*       command on the command ring.
*
* INPUTS
*
*       xhci                Pointer to xHC driver control block.
*       irp                 IRP submitted for address device command.
*       xhci_dev            Pointer xHCD device control block.
*       slot_id             Slot ID of device for which command is directed.
*       ep_index            Index of endpoint effected by the command.
*       func_addr           USB device address.
*
* OUTPUTS
*
*       NU_SUCCESS          Successful completion.
*       NU_USB_INVLD_ARG    Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Queue_Address_Device_Command(NU_USBH_XHCI     *xhci,
                                      NU_USB_IRP          *irp,
                                      NU_USBH_XHCI_DEVICE *xhci_dev,
                                      UINT8               slot_id,
                                      UINT8               ep_index,
                                      UINT8               func_addr)
{
    /* Progress status. */
    STATUS                 status   = NU_SUCCESS;

    /* Pointer to USB pipe - used as an intermediate variable for getting
     * pointer to USB device.
     */
    NU_USB_PIPE           *pipe     = NU_NULL;

    /* Pointer to USB device. */
    NU_USB_DEVICE         *usb_dev  = NU_NULL;

    /* Pointer to xHC device input context data structure.*/
    NU_USBH_XHCI_RAW_CTX  *in_ctx   = NU_NULL;

    /* Pointer to xHC device output context data structure.*/
    NU_USBH_XHCI_RAW_CTX  *out_ctx  = NU_NULL;

    /* Pointer to xHCI device slot context.*/
    NU_USBH_XHCI_SLOT_CTX *slot_ctx = NU_NULL;

    /* Pointer to input ctrl context data structure. */
    NU_USBH_XHCI_CTRL_CTX *ctrl_ctx = NU_NULL;

    /* EP context pointer for sanity chk.*/
    NU_USBH_XHCI_EP_CTX   *ep_ctx   = NU_NULL;

    /* Pointer to ctrl endpoint ring. */
    NU_USBH_XHCI_RING     *ep0_ring = NU_NULL;

    /* Device address assigned by the xHC.*/
    UINT8                 dev_addr  = 0;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(irp);
    NU_USB_PTRCHK(xhci_dev);

    in_ctx   = xhci_dev->dev_in_ctx;
    out_ctx  = xhci_dev->dev_out_ctx;
    ep0_ring = xhci_dev->ep_info[0].ring;


    /* Get the pipe handle from the IRP control block.*/
    status  = NU_USB_IRP_Get_Pipe( irp, &pipe );

    if ( status == NU_SUCCESS )
    {
        /* Get the USB device handle from the pipe control block.*/
        status = NU_USB_PIPE_Get_Device( pipe, &usb_dev );

        if ( (status == NU_SUCCESS )&& (usb_dev->state != USB_STATE_CONFIGURED) )
        {
            status = XHCI_Get_Slot_Context( out_ctx, &slot_ctx );
            if ( (status == NU_SUCCESS) &&
               ( (SLOT_CTX_GET_SLOT_STATE(LE32_2_HOST(slot_ctx->device_state)))
                  != XHCI_SLOT_STATE_ADDRESSED) )
            {
                /* Parse the slot ctx and ep ctx data structures for Address device
                 * command - xHCI specs,sections 6.2.3.1 & 6.2.2.1.
                 */
                status = XHCI_Parse_Address_Cmd_In_Ctx(xhci, xhci_dev, ep0_ring, usb_dev );

                if ( status == NU_SUCCESS )
                {
                    /* Place Address Device Command on the Command ring. */
                    status = XHCI_Queue_Command( xhci, (UINT32)in_ctx->ctx_buffer, 0, 0,
                                                 SET_TRB_SLOT_ID(slot_id) |
                                                 SET_TRB_TYPE(XHCI_TRB_ADDR_DEV) );
                    if ( status == NU_SUCCESS )
                    {
                        status = XHCI_Get_Slot_Context( out_ctx, &slot_ctx );

                        if ( status == NU_SUCCESS )
                        {
                            /* Get the address assigned by the xHC from the output slot context. */
                            dev_addr = (LE32_2_HOST(slot_ctx->device_state)) & SLOT_CTX_DEV_ADDR_MASK;
                            xhci_dev->device_addr = dev_addr;

                            /* Save the slot ID in the device table for later reference. */
                            xhci->device_table[func_addr] = slot_id;
                            status = XHCI_Get_Endpoint_Context(out_ctx,&ep_ctx, 0x00 );
                            status = XHCI_Get_Input_Control_Context( in_ctx, &ctrl_ctx );

                            if ( status == NU_SUCCESS )
                            {
                                ctrl_ctx->add_flags  = 0;
                                ctrl_ctx->drop_flags = 0;
                                status = XHCI_Handle_IRP_Completion(irp, NU_SUCCESS, 0);
                            }
                        }
                    }
                }
            }
            else
            {
               status = XHCI_Handle_IRP_Completion(irp, NU_SUCCESS, 0);
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Ring_Command_Doorbell
*
* DESCRIPTION
*
*       This function rings command doorbell to initiate command execution.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Ring_Command_Doorbell(NU_USBH_XHCI *xhci)
{
    STATUS                 status  = NU_SUCCESS;
    NU_USBH_XHCI_DB_ARRAY  *db_reg = NU_NULL;
    UINT32                  field  = 0;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);

    status = XHCI_Get_Doorbell_Regs_Handle( &xhci->reg_base, &db_reg );
    if ( status == NU_SUCCESS )
    {
        XHCI_HW_READ32( xhci, &db_reg->db_array[0], field);
        field |= XHCI_DB_TGT_HOST | XHCI_DB_STRM_ID_HOST;
        XHCI_HW_WRITE32( xhci, &db_reg->db_array[0], field);
    }

    return ( status );
}


/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Command_Completion
*
* DESCRIPTION
*
*       This function handles command completion and resumes the command
*       submitter context by releasing the command completion semaphore.
*
*
* INPUTS
*
*       xhci                Pointer to xHC driver control block.
*       event_trb           Pointer to event TRB which generated the event.
*
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Handle_Command_Completion( NU_USBH_XHCI     *xhci,
                                       NU_USBH_XHCI_TRB *event_trb )
{
    /* Command completion status. */
    STATUS            status      = NU_SUCCESS;
    UINT32            *cmd_status = NU_NULL;

    /* Command sync semaphore. */
    NU_SEMAPHORE      *cmd_sem    = NU_NULL;

    /* Command ring handle. */
    NU_USBH_XHCI_RING *cmd_ring   = NU_NULL;

    /* Completed command TRB. */
    NU_USBH_XHCI_TRB  *cmd_trb    = NU_NULL;

    /* Command completion code. */
    UINT8             cc_code     = 0;
    UINT32            addr;

    /* Get the command completion code from the event_trb .*/
    cc_code = GET_CMD_TRB_COMP_CODE(HOST_2_LE32(event_trb->status));

    /* Get the command completion status holding variable. */
    cmd_status = &xhci->cmd_ring_cb.cmd_status;

    /* Get the command completion semaphore. */
    cmd_sem = &xhci->cmd_ring_cb.cmd_comp_lock;
    *(cmd_status) = (UINT32)cc_code;

    status = XHCI_Get_Command_Ring_Handle(&xhci->cmd_ring_cb,&cmd_ring );

    XHCI_READ_ADDRESS(xhci , &event_trb->parameter[0], addr);

    cmd_trb = (NU_USBH_XHCI_TRB *)addr;

    if ( cmd_trb )
    {
        status = XHCI_Get_Command_Ring_Handle(&xhci->cmd_ring_cb, &cmd_ring );

        if (status == NU_SUCCESS)
        {
            if (GET_TRB_TYPE(HOST_2_LE32(cmd_trb->control)) == XHCI_TRB_ENABLE_SLOT )
            {
                /* Get the assigned slot ID from the event TRB. */
                if ( cc_code == XHCI_TRB_COMPL_SUCCESS )
                {
                    xhci->slot_id = GET_EVENT_TRB_SLOT_ID(HOST_2_LE32(event_trb->control));
                }
                else
                {
                    xhci->slot_id = NU_NULL;
                }
            }

            status = XHCI_Inc_Ring_Dequeue_Ptr( cmd_ring, NU_FALSE );

            if ( status == NU_SUCCESS )
            {
                status = NU_Release_Semaphore( cmd_sem );
            }
        }
    }
    else
    {
        status = XHCI_INVLD_RING_ADDR;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Parse_Address_Cmd_In_Ctx
*
* DESCRIPTION
*
*       This function parses input context for address device command.
*
* INPUTS
*
*       xhci                Pointer to xHC driver control block.
*       xhci_dev            xHC device control block
*                           parameters.
*       ring                Pointer to transfer ring.
*       usb_device          Pointer to USB device.
*
* OUTPUTS
*
*       NU_SUCCESS          Successful completion.
*       NU_USB_INVLD_ARG    Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Parse_Address_Cmd_In_Ctx(NU_USBH_XHCI         *xhci,
                                     NU_USBH_XHCI_DEVICE  *xhci_dev,
                                     NU_USBH_XHCI_RING    *ring,
                                     NU_USB_DEVICE        *usb_device)
{
    /* Progress status. */
    STATUS status                     = NU_SUCCESS;

    /* Pointer to xhci device slot context.*/
    NU_USBH_XHCI_SLOT_CTX *slot_ctx   = NU_NULL;

    /* Pointer to xhci device endpoint context.*/
    NU_USBH_XHCI_EP_CTX   *ep_ctx     = NU_NULL;

    /* Pointer to input control slot context.*/
    NU_USBH_XHCI_CTRL_CTX *ctrl_ctx   = NU_NULL;

    NU_USB_DEVICE         *tmp_device = NU_NULL;

    /* Pointer to xHC device input context data structure.*/
    NU_USBH_XHCI_RAW_CTX  *in_ctx   = NU_NULL;

    /* Depth of device parent hub.*/
    UINT8                 depth       = 0;

    /* Route string for issuing set address request in case of usb3.0
     * device.
     */
    UINT32                route_str   = 0;

    UINT8                 slot_id     = 0;

    UINT8                 port_num    = 0;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(xhci_dev);
    NU_USB_PTRCHK(ring);
    NU_USB_PTRCHK(usb_device);

    in_ctx  = xhci_dev->dev_in_ctx;
    /* Get the input control context from the xHCI device in context .*/
    status = XHCI_Get_Input_Control_Context( in_ctx, &ctrl_ctx );

    if (status == NU_SUCCESS)
    {
        /* Get the slot context. */
        status = XHCI_Get_Slot_Context( in_ctx, &slot_ctx );

        if ( status == NU_SUCCESS )
        {
          /* Get the endpoint context.*/
          status = XHCI_Get_Endpoint_Context( in_ctx, &ep_ctx , 0x00 );

            if ( status == NU_SUCCESS )
            {
                /* Since only slot context and ep 0 context are affected by the
                 * address device command therefore set them in the control ctx.
                 */
                ctrl_ctx->add_flags = SLOT_CTX_SLOT_FLAG | SLOT_CTX_EP0_FLAG;

                /* Set the device speed in the slot context device_info1 field.*/
                switch ( usb_device->speed )
                {
                    case USB_SPEED_HIGH:
                        slot_ctx->device_info1 = (UINT32)XHCI_SLOT_SPEED_HS;
                    break;

                    case USB_SPEED_FULL:
                        slot_ctx->device_info1 = XHCI_SLOT_SPEED_FS;
                    break;

                    case USB_SPEED_LOW:
                        slot_ctx->device_info1 = XHCI_SLOT_SPEED_LS;
                    break;

                    case USB_SPEED_SUPER:
                        slot_ctx->device_info1 = XHCI_SLOT_SPEED_SS;
                    break;

                    default:
                    /* Printf ("invalid speed");*/
                    break;
                }

                /* Last valid context is 1 which corresponds to endpoint 0.*/
                slot_ctx->device_info1 |= SLOT_CTX_LAST_CTX(1);

                /* Find the depth at which device parent hub is connected. This is
                 * required for finding the route string.
                 */
                tmp_device = usb_device;

                while ( !USBH_IS_ROOT_HUB(tmp_device->parent) )
                {
                     depth++;
                     tmp_device = tmp_device->parent ;
                }

                /* The root hub port number where device hierarchy is terminated.*/
                slot_ctx->device_info2 =
                                   SLOT_CTX_ROOT_HUB_PORT(tmp_device->port_number);

                /* Program the route string field in the slot context data structure
                 * that is necessary for address assignment.
                 */
                if ( (usb_device->speed == USB_SPEED_SUPER)
                     && (!USBH_IS_ROOT_HUB(usb_device->parent)) )
                {
                    route_str = usb_device->port_number << (4*(depth-1));
                    slot_ctx->device_info1 |=( (route_str |
                                                usb_device->parent->route_string) &
                                                SLOT_CTX_ROUTE_STR_MASK );

                    /* Save the route string in the USB device control block. */
                    usb_device->route_string = route_str;
                }

                /* There is only one interrupt target which is interrupter 0. */
                slot_ctx->tt_intr_info = SLOT_CTX_INTR_TGT(0);

                /* The slot context tt_intr_info field initialization for address device
                 * command when full/low speed device is connected via high speed hub.
                 * Refer to section 4.5.2. of xHCI specs.
                 */

                tmp_device = usb_device->parent;

                if ( tmp_device->function_address != USB_ROOT_HUB )
                {
                    if ( ( usb_device->speed < USB_SPEED_HIGH ) &&
                                 (tmp_device->speed == USB_SPEED_HIGH) )
                    {
                       /* Get the parent device slot id. */
                       slot_id = xhci->device_table[tmp_device->function_address];

                       /* Get the port number of parent high speed hub where low/full speed
                        * device is connected.
                        */
                       port_num = usb_device->port_number;

                       /* Populate the slot context fields. */
                       slot_ctx->tt_intr_info |= SLOT_CTX_TT_PORT(port_num);
                       slot_ctx->tt_intr_info |= (slot_id & SLOT_CTX_TT_SLOT);
                    }
                }

                /* Populate required fields of the ep0 context. For details refer
                 * to xHCI specs,section 4.6.5.
                 */
                ep_ctx->ep_info1  = EP_CTX_SET_MAX_PSTREAM(0);
                ep_ctx->ep_info1 |= EP_CTX_SET_STATE(0);
                ep_ctx->ep_info2  = EP_CTX_SET_MAX_BURST(0);
                ep_ctx->ep_info2 |= EP_CTX_ERROR_COUNT(3);
                ep_ctx->ep_info2 |= EP_CTX_EP_TYPE_SET(XHCI_CTRL_EP);

                /* Set the endpoint ring pointer and ring cycle state in the endpoint context data
                 * structure.
                 */
                XHCI_WRITE_ADDRESS(xhci, &ep_ctx->deq_ptr_lo,
                              (((UINT32)ring->enqueue_ptr | ring->cycle_state)));
                XHCI_WRITE_ADDRESS(xhci, &ep_ctx->deq_ptr_hi, 0);

                /* Populate the Max packet size for the EP 0,for superspeed and
                 * HS this is 512 and 64 respectively. xHCI specs, section 4.3.
                 */
                switch ( usb_device->speed )
                {
                    case USB_SPEED_HIGH:
                        ep_ctx->ep_info2 |= EP_CTX_SET_MAX_PACKET(64);
                    break;

                    case USB_SPEED_FULL:
                        ep_ctx->ep_info2 |= EP_CTX_SET_MAX_PACKET(8);
                    break;

                    case USB_SPEED_LOW:
                        ep_ctx->ep_info2 |= EP_CTX_SET_MAX_PACKET(8);
                    break;

                    case USB_SPEED_SUPER:
                        ep_ctx->ep_info2 |= EP_CTX_SET_MAX_PACKET(512);
                    break;

                    default:
                    /* Printf ("invalid speed");*/
                    break;
                }

                /* Convert to host endianess .*/
                XHCI_CONVERT_ENDIANESS_SLOT_CTX(slot_ctx);
                XHCI_CONVERT_ENDIANESS_CTRL_CTX(ctrl_ctx);
                XHCI_CONVERT_ENDIANESS_EP_CTX(ep_ctx);
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Normalize_Interval
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
UINT8 XHCI_Normalize_Interval(UINT32 interval)
{
    UINT8   i;

    for (i = 2; i <= 32; i <<= 1)
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
*       XHCI_Copy_Slot_Context
*
* DESCRIPTION
*
*       This function copies output slot context to input slot context,this
*       is useful when only one of the fields of slot context is to be changed.
*
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       in_ctx               xHCI device input context.
*       out_ctx              xHCI device output context.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Copy_Slot_Context(NU_USBH_XHCI         *xhci,
                              NU_USBH_XHCI_RAW_CTX *in_ctx,
                              NU_USBH_XHCI_RAW_CTX *out_ctx)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_XHCI_SLOT_CTX *slot_in_ctx;
    NU_USBH_XHCI_SLOT_CTX *slot_out_ctx;

    status =  XHCI_Get_Slot_Context( in_ctx, &slot_in_ctx );
    if ( status == NU_SUCCESS )
    {
        status =  XHCI_Get_Slot_Context( out_ctx, &slot_out_ctx );
        if ( status == NU_SUCCESS )
        {
            slot_in_ctx->device_info1 = slot_out_ctx->device_info1;
            slot_in_ctx->device_info1 = slot_out_ctx->device_info2;
            slot_in_ctx->tt_intr_info = slot_out_ctx->tt_intr_info;
            slot_in_ctx->device_state = slot_out_ctx->device_state;
        }
    }

    return ( status );
}
/**************************************************************************
*
* FUNCTION
*
*       XHCI_Copy_Endpoint_Context
*
* DESCRIPTION
*
*       This function copies output endpoint context to input endpoint context.
*
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       in_ctx               xHC device input context.
*       out_ctx              xHC device output context.
*       ep_index             Endpoint index.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Copy_Endpoint_Context(NU_USBH_XHCI         *xhci,
                                  NU_USBH_XHCI_RAW_CTX *in_ctx,
                                  NU_USBH_XHCI_RAW_CTX *out_ctx,
                                  UINT8                 ep_index)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_XHCI_EP_CTX *ep_in_ctx;
    NU_USBH_XHCI_EP_CTX *ep_out_ctx;

    status =  XHCI_Get_Endpoint_Context( in_ctx, &ep_in_ctx, ep_index);
    if ( status == NU_SUCCESS )
    {
        status = XHCI_Get_Endpoint_Context( out_ctx, &ep_out_ctx, ep_index );
        if ( status == NU_SUCCESS )
        {
            ep_in_ctx->ep_info1   = ep_out_ctx->ep_info1;
            ep_in_ctx->ep_info2   = ep_out_ctx->ep_info2;
            ep_in_ctx->deq_ptr_lo = ep_out_ctx->deq_ptr_lo;
            ep_in_ctx->deq_ptr_hi = ep_out_ctx->deq_ptr_hi;
            ep_in_ctx->ep_tx_info = ep_out_ctx->ep_tx_info;
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Parse_Config_Endpoint_In_Ctx
*
* DESCRIPTION
*
*       This function parses input context for configure endpoint command.
*
* INPUTS
*
*       xhci                                Pointer to xHC driver control block.
*       in_ctx                              xHC device input context used for passing
*                                           command parameters.
*       addr                                Pointer to ring or stream context array,
*                                           depends on calling context.
*       bEndpointAddress                    Identifies the endpoint which
*                                           owns the pipe.
*       bmAttributes                        Identifies the endpoint type.
*       speed                               USB device speed.
*       ep_max_size                         Maximum packet size the
*                                           endpoint should support.
*       interval                            Interval in micro seconds for
*                                           periodic endpoints.
*       bMaxBurst                           Burst size supported by the
*                                           endpoint.
*       SSEndpCompAttrib                    SS endpoint companion descrip-
*                                           tor attribute field.
*       bytes_per_interval                  Total bytes transferred by the
*                                           endpoint every service interval.
*
* OUTPUTS
*
*      NU_SUCCESS                          Successful completion.
*      NU_USB_INVLD_ARG                    Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Parse_Config_Endpoint_In_Ctx(NU_USBH_XHCI            *xhci,
                                         NU_USBH_XHCI_RAW_CTX    *in_ctx,
                                         VOID                    *addr,
                                         UINT8                   bEndpointAddress,
                                         UINT8                   bmEndpointAttributes,
                                         UINT8                   speed,
                                         UINT8                   bMaxBurst,
                                         UINT16                  wMaxPacketSize,
                                         UINT32                  interval,
                                         UINT8                   SSEndpCompAttrib,
                                         UINT16                  bytes_per_interval)
{
    /* Progress status. */
    STATUS status                   = NU_SUCCESS;

    /* xHCI endpoint context interval field. */
    UINT32 ep_interval              = 0;

    /* Max burst size supported by the endpoint. */
    UINT8  burst                    = 0;

    /* xHC driver defined endpoint index. */
    UINT8  ep_index                 = 0;

    /* Pointer to input context data structure. */
    NU_USBH_XHCI_CTRL_CTX *ctrl_ctx = NU_NULL;
    NU_USBH_XHCI_EP_CTX   *ep_ctx   = NU_NULL;
    NU_USBH_XHCI_SLOT_CTX *slot_ctx = NU_NULL;
    NU_USBH_XHCI_RING     *ring     = NU_NULL;

    /* Parameters validation .*/
    NU_USB_PTRCHK(in_ctx);
    NU_USB_PTRCHK(addr);

    /* Get the endpoint index. */
    ep_index = XHCI_EP_ADDR_TO_EP_INDEX(bEndpointAddress);

    /* Get the input control context from xhci device IN context. */
    status = XHCI_Get_Input_Control_Context(  in_ctx, &ctrl_ctx );
    if ( status == NU_SUCCESS )
    {
        /* Get the endpoint context from xhci device IN context. */
        status = XHCI_Get_Endpoint_Context( in_ctx, &ep_ctx, ep_index );

        if ( status == NU_SUCCESS )
        {
            /* Get the slot context from xhci device IN context. */
            status = XHCI_Get_Slot_Context( in_ctx, &slot_ctx );

            if ( status == NU_SUCCESS )
            {

                XHCI_CONVERT_ENDIANESS_SLOT_CTX(slot_ctx);
                XHCI_CONVERT_ENDIANESS_CTRL_CTX(ctrl_ctx);
                XHCI_CONVERT_ENDIANESS_EP_CTX(ep_ctx);

                /* Fill the endpoint context data structure interval field - xhci
                 * specs section 6.2.3.6.
                 */
                if ( ((bmEndpointAttributes & XHCI_EP_MASK) == USB_EP_ISO)
                    ||((bmEndpointAttributes & XHCI_EP_MASK) == USB_EP_INTR) )
                {
                    switch (speed)
                    {
                        case USB_SPEED_HIGH:
                        case USB_SPEED_SUPER:

                            /* In case of SuperSpeed and High Speed devices the EP
                             * CTX interval field is EP descriptor bInterval minus 1.
                             */
                            ep_interval = interval - 1;

                            if (ep_interval > 15 )
                            {
                                ep_interval = 15;
                            }

                        break;

                        case USB_SPEED_FULL:
                        case USB_SPEED_LOW:
                            /* need to round it to nearest base 2 multiple of
                             * 8*binterval .
                             */
                            ep_interval = XHCI_Normalize_Interval(interval);
                            if ( ep_interval > 10)
                                ep_interval = 10;

                            if ( ep_interval < 3 )
                                ep_interval = 3;

                        break;

                        default :
                           /*printf ("invalid speed");*/
                        break;
                    }
                }

                ep_ctx->ep_info1  = EP_CTX_SET_INTERVAL(ep_interval);

                /* Only primary stream arrays are supported.*/
                ep_ctx->ep_info1 |=  EP_CTX_HAS_LSA_MASK;

                if ( (speed == USB_SPEED_SUPER)
                      && ((bmEndpointAttributes & XHCI_EP_MASK) == USB_EP_ISO) )
                {
                     /* Maximum bursts supported with in the service interval. This
                      * field comes from SSEPCOMP descriptor,bmAttributes field
                      * bit [1:0]. For the time being we set it to moderate value.
                      */
                     ep_ctx->ep_info1 |= EP_CTX_SET_MULT(1);
                }

                /* Set the endpoint type as per xHCI requirements,refer to table 57
                 * of xHCI specs.
                 */
                if ( (bmEndpointAttributes & XHCI_EP_MASK) == USB_EP_ISO )
                {
                    if (bEndpointAddress & XHCI_EP_DIR_MASK)
                        ep_ctx->ep_info2 = EP_CTX_EP_TYPE_SET(XHCI_ISOC_IN_EP);
                    else
                        ep_ctx->ep_info2 = EP_CTX_EP_TYPE_SET(XHCI_ISOC_OUT_EP);
                }
                else if ((bmEndpointAttributes & XHCI_EP_MASK) == USB_EP_INTR )
                {
                    if (bEndpointAddress & XHCI_EP_DIR_MASK)
                        ep_ctx->ep_info2 = EP_CTX_EP_TYPE_SET(XHCI_INT_IN_EP);
                    else
                        ep_ctx->ep_info2 = EP_CTX_EP_TYPE_SET(XHCI_INT_OUT_EP);
                }
                else
                {
                    if (bEndpointAddress & XHCI_EP_DIR_MASK)
                        ep_ctx->ep_info2 = EP_CTX_EP_TYPE_SET(XHCI_BULK_IN_EP);
                    else
                        ep_ctx->ep_info2 = EP_CTX_EP_TYPE_SET(XHCI_BULK_OUT_EP);
                }

                /* Fill the EP CTX Max Packet Size field - xHCI specs, section
                 * 6.2.3.5
                 */
                switch ( speed )
                {
                    case USB_SPEED_SUPER:
                        ep_ctx->ep_info2 |= EP_CTX_SET_MAX_PACKET(wMaxPacketSize);
                        ep_ctx->ep_info2 |= EP_CTX_SET_MAX_BURST(bMaxBurst);
                        break;

                    case  USB_SPEED_HIGH:
                       ep_ctx->ep_info2 |= EP_CTX_SET_MAX_PACKET(wMaxPacketSize);
                       if ( ((bmEndpointAttributes & XHCI_EP_MASK) == USB_EP_ISO)
                          ||((bmEndpointAttributes & XHCI_EP_MASK) == USB_EP_INTR) )
                       {
                           burst = (UINT8)(wMaxPacketSize & 0x1800) >> 11;
                           ep_ctx->ep_info2 |= EP_CTX_SET_MAX_BURST(burst);
                       }
                    break;

                    case  USB_SPEED_FULL:
                    case  USB_SPEED_LOW:
                        ep_ctx->ep_info2 |= EP_CTX_SET_MAX_PACKET(wMaxPacketSize);
                        ep_ctx->ep_info2 |= EP_CTX_SET_MAX_BURST(0);
                    break;

                    default :
                    /*printf ("invalid speed");*/
                    break;
                }

                ep_ctx->ep_info2 |= EP_CTX_ERROR_COUNT(3);
                ep_ctx->ep_info2 &= ~(UINT32)EP_CTX_FORCE_EVENT;

                if (((bmEndpointAttributes & XHCI_EP_MASK) == USB_EP_BULK)
                     && (SSEndpCompAttrib) )
                {
                    /* Now initialize deq_ptr_lo with stream context array .*/
                    XHCI_WRITE_ADDRESS(xhci, &ep_ctx->deq_ptr_lo, (UINT32)addr);
                    XHCI_WRITE_ADDRESS(xhci, &ep_ctx->deq_ptr_hi, 0);
                    ep_ctx->ep_info1  |=  EP_CTX_SET_MAX_PSTREAM(SSEndpCompAttrib);
                }
                else
                {
                    ring = ( NU_USBH_XHCI_RING *) addr;

                    /* Now initialize deq_ptr_lo with endpoint ring address and cycle state .*/
                    XHCI_WRITE_ADDRESS(xhci, &ep_ctx->deq_ptr_lo,
                                             (((UINT32)ring->enqueue_ptr | ring->cycle_state)));
                    XHCI_WRITE_ADDRESS(xhci, &ep_ctx->deq_ptr_hi, 0);
                }

                /* Set the context enteries - which is the index of last valid endpoint. */
                if ( ((slot_ctx->device_info1 >> 27) & 0x1F) < (ep_index + 1) )
                {
                    slot_ctx->device_info1 &= ~SLOT_CTX_LAST_CTX_MASK;
                    slot_ctx->device_info1 |= SLOT_CTX_LAST_CTX(ep_index + 1);
                }

                /* We need to set the add and drop flags of the input ctrl ctx,
                 * ep0 add flag should be zero and slot flag and any other EP flag
                 * depending on the ep index must be set.
                 */
                ctrl_ctx->add_flags  =  SLOT_CTX_SLOT_FLAG;
                ctrl_ctx->add_flags  |= XHCI_CTRL_CTX_FLAG(ep_index);
                ctrl_ctx->drop_flags &= ~XHCI_CTRL_CTX_FLAG(ep_index);
                ctrl_ctx->drop_flags &= ~SLOT_CTX_SLOT_FLAG;

                XHCI_CONVERT_ENDIANESS_SLOT_CTX(slot_ctx);
                XHCI_CONVERT_ENDIANESS_CTRL_CTX(ctrl_ctx);
                XHCI_CONVERT_ENDIANESS_EP_CTX(ep_ctx);
            }
        }
    }

    return ( status );
}


/*==========================================================================
                        Root Hub Management Unit
===========================================================================*/

/* Array of Root Hub functions. */
STATUS (*XHCI_RH_Request[]) ( NU_USBH_XHCI *xhci, NU_USBH_CTRL_IRP *irp,
                              UINT8 speed) =
{
    XHCI_RH_Get_Status,                      /* GET_STATUS.        */
    XHCI_RH_Clear_Feature,                   /* CLEAR_FEATURE.     */
    XHCI_RH_Get_State,                       /* USBH_HUB_SPECIFIC. */
    XHCI_RH_Set_Feature,                     /* SET_FEATURE.       */
    XHCI_RH_Invalid_CMD,                     /* UN_SPECIFIED.      */
    XHCI_RH_Nothing_To_Do_CMD,               /* SET_ADDRESS.       */
    XHCI_RH_Get_Descriptor,                  /* GET_DESCRIPTOR.    */
    XHCI_RH_Invalid_CMD,                     /* SET_DESCRIPTOR.    */
    XHCI_RH_Nothing_To_Do_CMD,               /* GET_CONFIGURATION. */
    XHCI_RH_Nothing_To_Do_CMD,               /* SET_CONFIGURATION. */
    XHCI_RH_Invalid_CMD,                     /* GET_INTERFACE.     */
    XHCI_RH_Invalid_CMD,                     /* SET_INTERFACE.     */
    XHCI_RH_Invalid_CMD                      /* SYNC FRAME.        */
};

/* Root hub SuperSpeed device descriptor. */
static UINT8 xhci_rh_ss_dev_desc[] = {
    18,                                     /* bLength          */
    1,                                      /* DEVICE           */
    0x00,                                   /* USB 3.0          */
    0x03,                                   /* USB 3.0          */
    0x09,                                   /* HUB CLASS        */
    0x00,                                   /* Subclass         */
    0x03,                                   /* Protocol         */
    0x09,                                   /* bMaxPktSize0     */
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

/* Root hub BOS descriptors. */

static UINT8 xhci_rh_bos_desc[] = {

    /* BOS descriptor*/
    5,                                      /*[0],0:bLength        */
    USB_DT_BOS,                             /*[1],1:bDescriptorType */
    42,                                     /*[2],2(0):wTotalLength */
    0,                                      /*[3],2(1):wTotalLength */
    3,                                      /*[4],4:bNumDeviceCaps  */

    /*USB2 extension*/
    7,                                      /*[5],0:bLength*/
    USB_DT_DEVCAP,                          /*[6],1:bDescriptorType*/
    USB_DCT_USB2EXT,                        /*[7],2:bDevCapabilityType*/
    2,                                      /*[8],3(0):bmAttributes*/
    0,                                      /*[9],3(1):bmAttributes*/
    0,                                      /*[10],3(2):bmAttributes*/
    0,                                      /*[11],3(3):bmAttributes*/

    /*SuperSpeed USB Device Capability*/
    10,                                     /*[12],0:bLength*/
    USB_DT_DEVCAP,                          /*[13],1:bDescriptorType*/
    USB_DCT_USBSS,                          /*[14],2:bDevCapabilityType*/
    0,                                      /*[15],3:bmAttributes*/
    12,                                     /*[16],4(0):sSpeedsSupported*/
    0,                                      /*[17],4(1):sSpeedsSupported*/
    3,                                      /*[18],6:bFunctionalitySupport*/
    0,                                      /*[19],7:bU1DevExitLat*/
    0,                                      /*[20],8(0):wU2DevExitLat*/
    0,                                      /*[21],8(1):wU2DevExitLat*/

    /*Container ID*/
    20,                                     /*[22],0:bLength*/
    USB_DT_DEVCAP,                          /*[23],1:bDescriptorType*/
    USB_DCT_CONTID,                         /*[24],2:bDevCapabilityType*/
    0,                                      /*[25],3:bReserved*/
    0,                                      /*[26],4(0):ContainerID*/
    0,                                      /*[27],4(1):ContainerID*/
    0,                                      /*[28],4(2):ContainerID*/
    0,                                      /*[29],4(3):ContainerID*/
    0,                                      /*[30],4(4):ContainerID*/
    0,                                      /*[31],4(5):ContainerID*/
    0,                                      /*[21],4(6):ContainerID*/
    0,                                      /*[33],4(7):ContainerID*/
    0,                                      /*[34],4(8):ContainerID*/
    0,                                      /*[35],4(9):ContainerID*/
    0,                                      /*[36],4(10):ContainerID*/
    0,                                      /*[37],4(11):ContainerID*/
    0,                                      /*[38],4(12):ContainerID*/
    0,                                      /*[39],4(13):ContainerID*/
    0,                                      /*[40],4(14):ContainerID*/
    0                                       /*[41],4(15):ContainerID*/
};

/* Root hub SuperSpeed configuration descriptor.*/

static UINT8 xhci_rh_ss_cfg_desc[] = {

    /* Configuration Descriptor */

    9,                                      /* bLength              */
    2,                                      /* CONFIGURATION        */
    31,                                     /* length               */
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
    0x01,                                   /* iInterface           */

    /* Endpoint Descriptor */

    7,                                      /* bLength          */
    5,                                      /* ENDPOINT         */
    0x81,                                   /* bEndpointAddress */
    0x03,                                   /* bmAttributes     */
    0x10,                                   /* wMaxPacketSize   */
    0x00,                                   /* wMaxPacketSize   */
    0xf,                                    /* bInterval        */

    /* SS Endpoint Companion Descriptor. */

    6,                                      /* bLength           */
    48,                                     /* bDescriptorType   */
    0,                                      /* bMaxBurst         */
    0,                                      /* bmAttributes      */
    0x02,                                   /* wBytesPerInterval */
    0x00                                    /* wBytesPerInterval */
};

/* Root hub device descriptors. */

static UINT8 xhci_rh_dev_desc[] = {
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

/* Root hub configuration descriptor. */

static UINT8 xhci_rh_cfg_desc[] = {

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

/**************************************************************************
*
* FUNCTION
*
*       XHCI_RH_Fill_Descriptor
*
* DESCRIPTION
*
*       This function populates root hub descriptor according to given speed.
*
* INPUTS
*
*      xhci                 Pointer to xHC driver control block.
*      rh_desc_out          Pointer to the populated root hub descriptor.
*      speed                Speed for which descriptor is to be populated
*                           - high speed or super speed.
*
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*      NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_RH_Fill_Descriptor (NU_USBH_XHCI *xhci,
                                UINT8        *rh_desc_out,
                                UINT8        speed)
{
    /* Progress Status, */
    STATUS                status     = NU_SUCCESS;

    /* Capability register handle. */
    NU_USBH_XHCI_CAP_REGS *cap_regs  = NU_NULL;

    /* Register values. */
    UINT32                reg_value  = 0;
    UINT8                 reg_value2 = 0;

    /* Parameters validation. */
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(rh_desc_out);

    status = XHCI_Get_Capability_Regs_Handle( &xhci->reg_base, &cap_regs );
    if (status == NU_SUCCESS)
    {
        /* Hub descriptor for a roothub follows the standard format,
         * with the following fields fixed.
         *
         *  wHubCharacteristics[2] = 0 ; Its not a compound device.
         *  wHubCharacteristics[TTThinkTime] is given an arbitrary value.
         *  wHubCharacteristics[bPwrOn2PwrGood] is 0 ms as this is a soft
         *  hub
         *  Most of the devices on the ports are removable.
         */
        if ( speed ==  USB_SPEED_SUPER )
        {
            rh_desc_out[0] = 12;        /* bDescLength */
            rh_desc_out[1] = 0x2A;      /* bDescriptorType */
        }
        else
        {
            rh_desc_out[0] = 11;        /* bDescLength */
            rh_desc_out[1] = 0x29;      /* bDescriptorType */
        }

        /* For xhci, the Number of Ports are available in the HCSPARAMS1
         * register, bits 24-31
         */

        XHCI_HW_READ32(xhci, &cap_regs->xhci_hcs_params1, reg_value);

        rh_desc_out[2] = XHCI_HCS_MAX_DEV_PORTS(reg_value);

        /* Some portions of  the wHubCharacteristics is available in the
         * HCSPARAMS register */

        XHCI_HW_READ32(xhci, &cap_regs->xhci_hcc_params, reg_value);

        reg_value2 = 0;

        /* If this xhci supports per port power switching */
        if ( reg_value & 0x08 )
        {
            reg_value2 |= 0x01;
        }/* Otherwise, both d0 and d1 are zeros */

        /* Root hub is not part of any compound device so reg_value2[bit2] = 0.
         * Overcurrent protection mode is Global so reg_value2[bit2] = 0
         * No FS/LS device directly connected to this hub. so TTThinktime is
         * not to be bothered
         */

        /* If this xhci supports per port power indicators. */
        if (reg_value & 0x10)
        {
            reg_value2 |= (0x01 << 7);

        } /* Otherwise, d7 is zero */

        rh_desc_out[3] = reg_value2 ;
        rh_desc_out[4] = 0;

        /* Power-ON-2-Power-Good time is given as 2ms */
        rh_desc_out[5] = 1;

        /* Root Hub is powered by the System. Controller current requirements
         * are not to be bothered.
         */
        rh_desc_out[6] = 0;

        if ( speed == USB_SPEED_SUPER )
        {
            rh_desc_out[7] =  0;                  /* bHubHdrDecLat.*/
            rh_desc_out[8] =  0;                  /* wHubDelay. */
            rh_desc_out[9] =  0;

            /* All devices are removable to this Hub.So both the 'DeviceRemovable'
             * bytes are zero.
             */
            rh_desc_out[10] = 0 ;
            rh_desc_out[11] = 0 ;
        }
        else
        {
            rh_desc_out[7] = 0;
            rh_desc_out[8] = 0;

            /* PortPwrCtrlMask is now de-funct. We just initialize this to 0xFF
             * as required by the spec.
             */
            rh_desc_out[9] = 0xFF;
            rh_desc_out[10] = 0xFF;
        }

        if ( speed == USB_SPEED_SUPER )
        {
          /* Initialize U1 and U2 exit latencies for Root Hub in BOS descriptor. */
          XHCI_HW_READ32(xhci, &cap_regs->xhci_hcs_params3, reg_value);
          xhci_rh_bos_desc[19] = XHCI_HCS_U1_LATENCY(reg_value);
          xhci_rh_bos_desc[20] = (UINT8)(XHCI_HCS_U2_LATENCY(reg_value) & 0xFF);
          xhci_rh_bos_desc[21] = (UINT8)((XHCI_HCS_U2_LATENCY(reg_value) >> 8) & 0xFF);
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_RH_Handle_IRP
*
* DESCRIPTION
*
*       This function handles IRP directed for the root hub.
*
* INPUTS
*
*      xhci               Pointer to xHC driver control block.
*      irp                Data transfer description.
*      bEndpointAddress   Root hub endpoint address for which IRP
*                         is submitted.
*
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_RH_Handle_IRP(NU_USBH_XHCI *xhci,
                          NU_USB_IRP   *irp,
                          UINT8        bEndpointAddress)
{
    /* Progress Status. */
    STATUS              status      = NU_SUCCESS;

    /* Status of completed irp. */
    STATUS              irp_status  = NU_SUCCESS;

    /* Buffer for holding IRP data. */
    UINT32              *buffer     = NU_NULL;

    /* Control endpoint IRP. */
    NU_USBH_CTRL_IRP    *ctrl_irp   = NU_NULL;

    /* IRP call back function. */
    NU_USB_IRP_CALLBACK  callback   = NU_NULL;

    /* Pipe on which IRP is submitted. */
    NU_USB_PIPE         *pipe       = NU_NULL;

    /* Buffer for holding IRP data. */
    UINT8               *irp_buffer = NU_NULL;

    /* Pointer to USB device. */
    NU_USB_DEVICE       *usb_device = NU_NULL;

    UINT8                bRequest   = 0;

    /* Data transfer length. */
    UINT32              irp_length  = 0;

    UINT8               retire_irp  = NU_FALSE;

    /* Get the pipe on which irp is submitted. */
    status = NU_USB_IRP_Get_Pipe ( irp, &pipe );

    /* Get the root hub device. */
    if ( status == NU_SUCCESS )
    {
       status = NU_USB_PIPE_Get_Device ( pipe, &usb_device );
    }

    if ( status == NU_SUCCESS )
    {
        /* First handle the non-control endpoint requests. */
        if ( (bEndpointAddress & XHCI_EP_INDEX_MASK) != NU_NULL )
        {
            if ( usb_device->speed == USB_SPEED_SUPER )
            {
                /* Check if an updated status is available. */
                if ( xhci->usb3_hub.ss_rh_status.status_map == NU_NULL )
                {
                        xhci->usb3_hub.ss_rh_status.irp = irp;
                }
                else
                {
                    xhci->usb3_hub.ss_rh_status.irp = NU_NULL;

                    (VOID)NU_USB_IRP_Get_Data( irp, (UINT8 **) &buffer );

                    *buffer = HOST_2_LE32(xhci->usb3_hub.ss_rh_status.status_map);

                    (VOID)NU_USB_IRP_Set_Actual_Length( irp, sizeof(UINT32) );

                    xhci->usb3_hub.ss_rh_status.status_map = NU_NULL;

                    irp_status = NU_SUCCESS;

                    retire_irp = NU_TRUE;
                }

            }
            else if ( xhci->usb3_hub.hs_rh_status.status_map == NU_NULL )
            {
                xhci->usb3_hub.hs_rh_status.irp = irp;
            }
            else
            {
                xhci->usb3_hub.hs_rh_status.irp = NU_NULL;

                (VOID)NU_USB_IRP_Get_Data( irp, (UINT8 **) &buffer );

                *buffer = HOST_2_LE32(xhci->usb3_hub.hs_rh_status.status_map);

                (VOID)NU_USB_IRP_Set_Actual_Length( irp, sizeof(UINT32) );

                xhci->usb3_hub.hs_rh_status.status_map = NU_NULL;

                irp_status = NU_SUCCESS;

                retire_irp = NU_TRUE;
            }
        }
        else
        {
            /* If this is control transfer. */
            ctrl_irp = (NU_USBH_CTRL_IRP *) irp;

            (VOID)NU_USB_IRP_Get_Data( (NU_USB_IRP *)ctrl_irp, &irp_buffer );

            (VOID)NU_USB_IRP_Get_Length( (NU_USB_IRP *)ctrl_irp, &irp_length );

            /* Clear the buffer. */
            memset( (VOID *)irp_buffer, NU_NULL, irp_length );

            (VOID)NU_USB_IRP_Set_Actual_Length( (NU_USB_IRP *)ctrl_irp, NU_NULL );

            (VOID)NU_USBH_CTRL_IRP_Get_bRequest( ctrl_irp, &bRequest );

            if (bRequest >= (sizeof(XHCI_RH_Request) / sizeof(UINT32)))
            {
                irp_status =  NU_USB_INVLD_ARG;
                status =  NU_USB_INVLD_ARG;
            }
            else
            {
                /* Invoke the function appropriate for the request. */
                irp_status = XHCI_RH_Request[bRequest]( xhci, ctrl_irp, usb_device->speed );
            }

            retire_irp = NU_TRUE;
        }
    }

    if( retire_irp == NU_TRUE )
    {
        (VOID)NU_USB_IRP_Set_Status( irp, irp_status );

        (VOID)NU_USB_IRP_Get_Callback( irp, &callback );

        (VOID)NU_USB_IRP_Get_Pipe( irp, &pipe );

        if( callback != NU_NULL )
        {
            callback( pipe, irp );
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_RH_Event
*
* DESCRIPTION
*
*       This function handles root hub interrupt/event.
*
* INPUTS
*
*      xhci                 Pointer to xHC driver control block.
*      event_trb            Pointer to event TRB which generated
*                           interrupt.
*

* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*      NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
VOID XHCI_Handle_RH_Event( NU_USBH_XHCI     *xhci,
                           NU_USBH_XHCI_TRB *event_trb )
{
    /* Operational register handle. */
    NU_USBH_XHCI_OP_REGS *op_regs  = NU_NULL;

    /* Interrupt pipe on which irp was submitted. */
    NU_USB_PIPE          *pipe     = NU_NULL;

    /* IRP callback function. */
    NU_USB_IRP_CALLBACK  callback  = NU_NULL;

    /* Port ID for the port which generated the interrupt.*/
    UINT8                port_id   = 0;

    /* Variable to hold port address. */
    UINT32               addr      = 0;

    UINT32               reg_value = 0;

    /* Port status. */
    UINT32               report    = 0;

    UINT16               *buf      = NU_NULL;

    if ( (!xhci) || (!event_trb) )
    {
        /* Return from here to avoid crash. */
        return ;
    }

    /* Get the port ID from the event TRB. */
    port_id = GET_PORT_ID(HOST_2_LE32(event_trb->parameter[0]));

    if ( port_id > 15 )
    {
        return ;
    }

    (VOID)XHCI_Get_Operational_Regs_Handle( &xhci->reg_base,  &op_regs );

    /* PORTSC register address. */
    addr = (UINT32)&op_regs->xhci_port_status_base + XHCI_PORT_OFFSET(port_id);

    /* Retrieve the port status value. */
    XHCI_HW_READ32(xhci, addr, reg_value);

    /* This hook is added for Fresco logic xhci card based on xhci specs 0.95 .
     * Since it does not have extended capability structures therefore the
     * ports speed is not known in priori. SS devices connected through it
     * show correct speed even before reset.
     */
    if ( xhci->info_hub.xhci_specs_ver == XHCI_SPECS_0_95 )
    {
        if ( (reg_value & XHCI_PORT_CONNECT))
        { 
            if ((XHCI_DEVICE_SPEED(reg_value) == XHCI_DEVICE_SS) )
            {
                xhci->usb3_hub.port_array[port_id] = USB_SPEED_SUPER;
            }
            else
            {
                 xhci->usb3_hub.port_array[port_id] = USB_SPEED_HIGH;
            }
        }
    }

    /* A Status change event occurred in one of the ports owned by the
     * xhci controller.The relevant port status register contains the
     * specific information about the change.
     */
    if ( reg_value & XHCI_PORT_PLC)
    {
        /* If there is resume signaling on the port .*/
        if ( XHCI_GET_PORT_LS(reg_value) == XHCI_LINK_STATE_RESUME )
        {
            if (xhci->usb3_hub.port_array[port_id] == USB_SPEED_SUPER)
            {
                /* Ack the LS status change - Write 1 to clear.*/
                reg_value = (reg_value & XHCI_PORT_RO) | (reg_value & XHCI_PORT_RWS);
                reg_value |= XHCI_PORT_PLC;
                XHCI_HW_WRITE32(xhci, addr,reg_value);

                /* Set the PLS bits to U0(0) to complete the resume signaling.*/
                reg_value |= XHCI_SET_PORT_LS(0) | XHCI_PORT_LINK_STROBE;
                XHCI_HW_WRITE32(xhci, addr,reg_value);
            }
            else
            {
                /* wait for 20ms to complete the resume signaling for USB2.0 devices. */
                usb_wait_ms(20);
                reg_value = (reg_value & XHCI_PORT_RO) | (reg_value & XHCI_PORT_RWS);
                reg_value |= XHCI_SET_PORT_LS(0) | XHCI_PORT_LINK_STROBE;
                XHCI_HW_WRITE32(xhci, addr,reg_value);
            }
        }
    }

    /* Read the port status, */
    XHCI_HW_READ32(xhci, addr, reg_value);

    report |= (0x1 << port_id);

    if ( xhci->usb3_hub.port_array[port_id] == USB_SPEED_SUPER )
    {
        /* Now that we have found the ports that have changed, check
         * to see if there is a pending IRP. If so, update the data
         * buffer and invoke the callback.
         * If no IRP is pending, then make a note that there has been
         * an update in status change and return.
         */

        if (xhci->usb3_hub.ss_rh_status.irp != NU_NULL)
        {
            /* There is an IRP pending. Update the data */

            (VOID)NU_USB_IRP_Get_Data( xhci->usb3_hub.ss_rh_status.irp,
                                      (UINT8 **) &buf);

            *buf = HOST_2_LE16 (report);

            (VOID)NU_USB_IRP_Set_Actual_Length(xhci->usb3_hub.ss_rh_status.irp, 2);

            xhci->usb3_hub.ss_rh_status.status_map = 0;

            /* IRP is now complete. Update the status and invoke the
             * callback.
             */
            (VOID) NU_USB_IRP_Set_Status(
                                   (NU_USB_IRP *)(xhci->usb3_hub.ss_rh_status.irp),
                                    NU_SUCCESS);

            (VOID)NU_USB_IRP_Get_Callback(
                                   (NU_USB_IRP *)(xhci->usb3_hub.ss_rh_status.irp),
                                   &callback);

            (VOID)NU_USB_IRP_Get_Pipe(
                                   (NU_USB_IRP *)(xhci->usb3_hub.ss_rh_status.irp),
                                   &pipe);

            callback (pipe, (NU_USB_IRP *)xhci->usb3_hub.ss_rh_status.irp);

            xhci->usb3_hub.ss_rh_status.irp = NU_NULL;
        }
        else
        {
            /* This will be sent to the Hub Class driver when it polls next. */
            xhci->usb3_hub.ss_rh_status.status_map |= report;
        }
    }
    else
    {
        /* Now that we have found the ports that have changed, check
         * to see if there is a pending IRP. If so, update the data
         * buffer and invoke the callback.
         * If no IRP is pending, then make a note that there has been
         * an update in status change and return.
         */

        if ( xhci->usb3_hub.hs_rh_status.irp != NU_NULL )
        {
            /* There is an IRP pending. Update the data */

            (VOID)NU_USB_IRP_Get_Data( xhci->usb3_hub.hs_rh_status.irp,
                                      (UINT8 **) &buf);

            *buf = HOST_2_LE16 (report);

            (VOID)NU_USB_IRP_Set_Actual_Length(xhci->usb3_hub.hs_rh_status.irp, 2);

            xhci->usb3_hub.hs_rh_status.status_map = 0;

            /* IRP is now complete. Update the status and invoke the
             * callback.
             */
            (VOID) NU_USB_IRP_Set_Status(
                                   (NU_USB_IRP *)(xhci->usb3_hub.hs_rh_status.irp),
                                    NU_SUCCESS);

            (VOID)NU_USB_IRP_Get_Callback(
                                   (NU_USB_IRP *)(xhci->usb3_hub.hs_rh_status.irp),
                                   &callback);

            (VOID)NU_USB_IRP_Get_Pipe(
                                   (NU_USB_IRP *)(xhci->usb3_hub.hs_rh_status.irp),
                                   &pipe);

            callback (pipe, (NU_USB_IRP *)xhci->usb3_hub.hs_rh_status.irp);

            xhci->usb3_hub.hs_rh_status.irp = NU_NULL;
        }
        else
        {
            /* This will be sent to the Hub Class driver when it polls next. */
            xhci->usb3_hub.hs_rh_status.status_map |= report;
        }
    }

    return;
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_RH_Initialize
*
* DESCRIPTION
*
*       This function initializes SuperSpeed root hub data structures and power
*       up ports.
*
* INPUTS
*
*      xhci                Pointer to xHC driver control block.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_RH_Initialize (NU_USBH_XHCI  *xhci)
{
    STATUS               status    = NU_SUCCESS;

    /* Op register handle. */
    NU_USBH_XHCI_OP_REGS *op_regs  = NU_NULL;

    UINT32               index     = 0;

    UINT32               reg_value = 0;

    /* Port register address. */
    UINT32               addr      = 0;

    NU_USB_PTRCHK(xhci);

    /* This function call goes through extended capability list and assign speed to
     * ports depending on the revision number.
     */
    status = XHCI_Setup_RH_Ports( xhci, &xhci->usb3_hub);
    if ( status == NU_SUCCESS )
    {
        /* Populates the RH descriptors for USB2.0 portion of USB3.0 hub. */
        status = XHCI_RH_Fill_Descriptor( xhci, xhci->usb3_hub.hs_rh_hub_desc, USB_SPEED_HIGH );
        if ( status == NU_SUCCESS )
        {
            /* Populates the RH descriptors for SS portion of USB3.0 hub. */
            status = XHCI_RH_Fill_Descriptor( xhci, xhci->usb3_hub.ss_rh_hub_desc, USB_SPEED_SUPER );
            if ( status == NU_SUCCESS)
            {
                status = XHCI_Get_Operational_Regs_Handle( &xhci->reg_base, &op_regs );
                if ( status == NU_SUCCESS )
                {
                    /* Initialize the roothub status    */
                    xhci->usb3_hub.hs_rh_status.previous.hub_status = 0x0;
                    xhci->usb3_hub.ss_rh_status.previous.hub_status = 0x0;

                    /* Clear the status and initialize each port */
                    for (index = 1; index <= xhci->usb3_hub.hs_rh_hub_desc[2]; index++)
                    {
                        /* port status is reset */
                        xhci->usb3_hub.hs_rh_status.previous.port_status[index - 1] = 0;
                        xhci->usb3_hub.ss_rh_status.previous.port_status[index - 1] = 0;

                        /* For the port,
                         * 1. The Power is enabled.            b9 = 1
                         * 2. Port is given an Green Indicator b15:10 = 02b
                         * xHCI enables port power on every port after chip hardware reset
                         * whether port power switching is supported or not. Therefore just setup
                         * port indicators.
                         */

                        addr = (UINT32)&op_regs->xhci_port_status_base + XHCI_PORT_OFFSET(index);
                        XHCI_HW_READ32(xhci, addr, reg_value);
                        reg_value = (reg_value & XHCI_PORT_RO) | (reg_value & XHCI_PORT_RWS);
                        reg_value |=XHCI_PIC(2);
                        XHCI_HW_WRITE32(xhci, addr,reg_value);
                    }

                    /* status bitmap is reset */
                    xhci->usb3_hub.hs_rh_status.status_map = 0x0;
                }
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Setup_RH_Ports
*
* DESCRIPTION
*
*      This function goes through extended capability list and assign speed to
*      ports depending on the revision number.
* INPUTS
*
*      xhci                Pointer to xHC driver control block.
*      usb3_hub            Pointer to xHCI USB3 root hub.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Setup_RH_Ports(NU_USBH_XHCI *xhci,
                           XHCI_USB3_HUB *usb3_hub)
{
    /* Processing status. */
    STATUS                status = NU_SUCCESS;

    /* Register value. */
    UINT32                reg_value;

    /* Capability register set handle. */
    NU_USBH_XHCI_CAP_REGS *cap_regs;

    /* Extended Cap list pointer. */
    UINT32                exc_ptr;

    /* Port info i.e SS or HS.*/
    UINT32                port_info;

    UINT8                 port_count,port_offset;

    UINT32                addr;

    status = XHCI_Get_Capability_Regs_Handle(&xhci->reg_base, &cap_regs);
    if ( status == NU_SUCCESS )
    {
        /* Get the capability register pointer.*/
        XHCI_HW_READ32(xhci, &cap_regs->xhci_hcc_params, reg_value);
        exc_ptr = XHCI_HCC_EXT_CAPS_PTR(reg_value) << 2;

        /* Effective address of first capability data structure. */
        addr = (UINT32)cap_regs + exc_ptr;

        if ( exc_ptr )
        {
            /* Scan the xHCI supported protocol capability list and assign speed to ports
             * depending major revision number.
             */
            while ( exc_ptr )
            {
                XHCI_HW_READ32(xhci, addr ,reg_value);
                if (XHCI_EXTENDED_CAPS_ID(reg_value) == XHCI_EXTENDED_CAPS_PROTOCOL )
                {
                    XHCI_HW_READ32( xhci, (addr + XHCI_EXC_CAP_OFFSET), port_info);

                    /* Get the port offset and port count present in this protocol capability
                     *  structure.
                     */
                    port_offset = XHCI_ROOT_HUB_PRT_OFFSET(port_info);
                    port_count = XHCI_ROOT_HUB_PRT_COUNT(port_info);
                    port_offset--;
                    if ( XHCI_ROOT_HUB_PRT_MAJ_REV(reg_value) == XHCI_PORT_REV_SS )
                    {
                     /* This capability structure contains info relating to
                      * SS ports.Start from the port offset and increment until
                      * port count and assign speed to the ports.
                      */
                        while ( port_count )
                        {
                            if ( (port_offset + port_count) < XHCI_MAX_PORTS )
                            {
                                usb3_hub->port_array[port_offset + port_count] = USB_SPEED_SUPER;
                                port_count--;
                            }
                        }
                    }
                    /* Protocol capability structure contains info for USB2 protocol.*/
                    else
                    {
                        while ( port_count )
                        {
                            if ( (port_offset + port_count) < XHCI_MAX_PORTS )
                            {
                                usb3_hub->port_array[port_offset + port_count] = USB_SPEED_HIGH;
                                port_count--;
                            }
                        }
                    }
                }
                /* Go to next capability structure.*/
                exc_ptr = XHCI_EXTENDED_CAPS_NEXT(reg_value) << 2;
                addr = addr + exc_ptr;
            }
        }
        else
        {
            /* Fresco logic xHCI based on specs v0.95 does not have Extended
             * Capability Registers, but the port speed for SS devices is valid
             * before reset. This thing was later made part of specs v1.0 i.e for
             * SS devices the PORTSC register gives correct speed even before port
             * is reset.
             */
            if ( xhci->info_hub.xhci_specs_ver == XHCI_SPECS_0_95)
            {
              status = NU_SUCCESS;
            }
            else
            {
              status = XHCI_ERR_FATAL;
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_RH_Get_Descriptor
*
* DESCRIPTION
*
*       This function handles Get_Descriptor request for root hub.
*
* INPUTS
*
*      xhci                 Pointer to xHC driver control block.
*      irp                  Data transfer description.
*      speed                USB device speed for which descriptor is required.
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*      NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_RH_Get_Descriptor(NU_USBH_XHCI     *xhci,
                              NU_USBH_CTRL_IRP *irp,
                              UINT8            speed)
{
    /* Progress Status. */
    STATUS status        = NU_SUCCESS;

    /* Buffer for IRP data. */
    UINT8  *irp_buffer   = NU_NULL;

    /* Setup packet fields. */
    UINT16 wLength       = 0;
    UINT8  bmRequestType = 0;
    UINT16 wValue        = 0;

    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(irp);

    /* Depending on the descriptor required, the data is copied to the user
     * buffer. Length is set properly and the control is returned to the caller.
     */
    (VOID)NU_USBH_CTRL_IRP_Get_wLength (irp, &wLength);

    wLength = LE16_2_HOST(wLength);

    (VOID)NU_USB_IRP_Get_Data ((NU_USB_IRP *) irp, &irp_buffer);

    (VOID)NU_USBH_CTRL_IRP_Get_bmRequestType (irp, &bmRequestType);

    (VOID)NU_USBH_CTRL_IRP_Get_wValue (irp, &wValue);

    wValue = LE16_2_HOST(wValue);

    /* If this is a standard get descriptor request. */
    if ( (bmRequestType & 0x20) == 0 )
    {
        /* Device Descriptor  */
        if (USBH_XHCI_RH_MSB(wValue) == USB_DT_DEVICE)
        {
            if ( speed == USB_SPEED_SUPER )
            {
                if (wLength > xhci_rh_ss_dev_desc[0])
                    wLength = xhci_rh_ss_dev_desc[0];

                memcpy (irp_buffer, xhci_rh_ss_dev_desc, wLength);
            }
            else
            {
                if (wLength > xhci_rh_dev_desc[0])
                    wLength = xhci_rh_dev_desc[0];

                memcpy (irp_buffer, xhci_rh_dev_desc, wLength);
            }
        }
        else if (USBH_XHCI_RH_MSB(wValue) == USB_DT_CONFIG)
        {
            /* Configuration Descriptor */
            if ( speed == USB_SPEED_SUPER )
            {

                if (wLength >= xhci_rh_ss_cfg_desc[2])
                    wLength =  xhci_rh_ss_cfg_desc[2];

                memcpy (irp_buffer, xhci_rh_ss_cfg_desc, wLength);
            }
            else
            {
                if (wLength >= xhci_rh_cfg_desc[2])
                    wLength =  xhci_rh_cfg_desc[2];

                memcpy (irp_buffer, xhci_rh_cfg_desc, wLength);
            }
        }
        else if (USBH_XHCI_RH_MSB (wValue) == USB_DT_BOS)
        {
            /* BOS descriptor. */
            if ( wLength >= xhci_rh_bos_desc[2] )
                wLength =  xhci_rh_bos_desc[2];

            memcpy (irp_buffer, xhci_rh_bos_desc, wLength);
        }

        (VOID)NU_USB_IRP_Set_Actual_Length ((NU_USB_IRP *) irp, wLength);
    }
    else
    {
        /* GET_USB_HUB_DESCRIPTOR request. */
        if ( speed ==  USB_SPEED_SUPER )
        {
            if ( wLength >= xhci->usb3_hub.ss_rh_hub_desc[0] )
                 wLength = xhci->usb3_hub.ss_rh_hub_desc[0];

            memcpy( irp_buffer, xhci->usb3_hub.ss_rh_hub_desc, wLength );
        }
        else
        {
            if ( wLength >= xhci->usb3_hub.hs_rh_hub_desc[0] )
                 wLength = xhci->usb3_hub.hs_rh_hub_desc[0];

            memcpy( irp_buffer, xhci->usb3_hub.hs_rh_hub_desc, wLength );
        }

        (VOID)NU_USB_IRP_Set_Actual_Length ((NU_USB_IRP *) irp, wLength);
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_RH_Get_Status
*
* DESCRIPTION
*
*       This function handles Get_Port_Status request for the root hub.
*
* INPUTS
*
*      xhci                Pointer to xHC driver control block.
*      irp                 Pointer to IRP containing request description.
*      speed               USB device speed for which port status is required.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_RH_Get_Status (NU_USBH_XHCI     *xhci,
                           NU_USBH_CTRL_IRP *irp,
                           UINT8            speed)
{
    /* Operational register handle. */
    NU_USBH_XHCI_OP_REGS *op_regs      = NU_NULL;

    /* IRP buffer for data. */
    UINT8                *irp_buffer   = NU_NULL;

    /* Setup packet fields. */
    UINT8                bmRequestType = 0;
    UINT16               wIndex        = 0;
    UINT8                port          = 0;

    /* Non SS port status. */
    UINT32               port_sts      = 0;
    UINT32               reg_value     = 0;

    /* SS port status. */
    UINT32               ss_port_sts   = 0;

    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(irp);

    (VOID)NU_USB_IRP_Get_Data( (NU_USB_IRP *) irp, &irp_buffer );

    (VOID)NU_USBH_CTRL_IRP_Get_bmRequestType( irp, &bmRequestType );

    (VOID)NU_USBH_CTRL_IRP_Get_wIndex( irp, &wIndex );

    (VOID)XHCI_Get_Operational_Regs_Handle( &xhci->reg_base, &op_regs );

    wIndex = LE16_2_HOST(wIndex);

    /* Class specific GET STATUS.
     * At xhci rh, no hub level set features are supported.
     * only port level are supported. Hub status is always returned
     * success.
     */
    if ( USBH_XHCI_RH_GET_RECIPIENT(bmRequestType) !=
                                   USBH_XHCI_RH_RECIPIENT_PORT )
    {
        return ( NU_SUCCESS );
    }

    port = USBH_XHCI_RH_LSB(wIndex);

    /* Read the stored status for this port from the ports register */
    XHCI_HW_READ32(xhci, ((UINT32)&op_regs->xhci_port_status_base +
                   XHCI_PORT_OFFSET(port)), reg_value);

    port_sts = 0;

    /* We need to frame the status read from the POPRTSC register in a way
     * defined by the Hub Class spec for SuperSpeed and other speeds.
     */

    /* First, the change bits,Connect change, enable change,suspend change,
     * over current change and reset change.These bits occupy the Upper
     * 16-bit word of the status.
     */

    /* Connect change */
    if (reg_value & XHCI_PORT_CSC)
    {
        port_sts |= (0x1 << 16);
        ss_port_sts |= (0x1 << 16);
    }

    /* Enable change */
    if (reg_value & XHCI_PORT_PEC)
    {
        port_sts |= ((0x1 << 1) << 16);
    }

   /* Overcurrent change */
    if (reg_value & XHCI_PORT_OCC)
    {
        port_sts |= ((0x1 << 3) << 16);
        ss_port_sts |= ((0x1 << 3) << 16);
    }

    /* Reset Change */
    if (reg_value & XHCI_PORT_RC)
    {
        port_sts |= ((0x1 << 4) << 16);
        ss_port_sts |= ((0x1 << 4) << 16);
    }

    /* Port Link State Change */
    if (reg_value & XHCI_PORT_PLC)
    {
        port_sts  |= ((0x1 << 2) << 16);
        ss_port_sts |= ((0x1 << 6) << 16);
    }

    if ( reg_value & XHCI_PORT_WRC)
    {
        ss_port_sts |= ((0x1 << 5) << 16);
    }

    /* Now the status bits */

    /* Connect, Enable, Link State, OverCurrent, Reset, Power,Device speed
     * indication are supported.
     */

    /* Connect status */
    if (reg_value & XHCI_PORT_CONNECT)
    {
        port_sts  |= (0x1);
        ss_port_sts |= (0x1);
    }

    /* Enable status */
    if (reg_value & XHCI_PORT_PE)
    {
        port_sts  |= (0x1 << 1);
        ss_port_sts |= (0x1 << 1);
    }

    /* Overcurrent status */
    if (reg_value & XHCI_PORT_OC)
    {
        port_sts  |= (0x1 << 3);
        ss_port_sts |= (0x1 << 3);
    }

    /* Reset status */
    if (reg_value & XHCI_PORT_RESET)
    {
        port_sts  |= (0x1 << 4);
        ss_port_sts |= (0x1 << 4);
    }

    /* Suspend status */
    if (((reg_value & 0x01E0 ) >> 5) == 3)
    {
        port_sts  |= (0x1 << 2);
        ss_port_sts |= (reg_value & 0x01E0);
    }

    /* Power status */
    if (reg_value & XHCI_PORT_POWER)
    {
        port_sts  |= (0x1 << 8);
        ss_port_sts |= (0x1 << 9);
    }

    /* Superspeed device only */
    if (XHCI_DEVICE_SPEED(reg_value) == XHCI_DEVICE_SS)
    {
        ss_port_sts &= ~( 0x3 << 10 );
    }
    else if (XHCI_DEVICE_SPEED(reg_value) == XHCI_DEVICE_HS)
    {
        port_sts |= ( 0x1 << 10 );
    }
    else if(XHCI_DEVICE_SPEED(reg_value) == XHCI_DEVICE_FS)
    {
        port_sts &= ~( 0x1 << 10 );
        port_sts &= ~( 0x1 << 9 );
    }
    else
    {
        port_sts |= ( 0x1 << 9 );
        port_sts &= ~( 0x1 << 10 );
    }

    /* Copy this data to the irp buffer.*/
    if ( speed == USB_SPEED_SUPER )
    {
        reg_value = HOST_2_LE32 (ss_port_sts);
    }
    else
    {
        reg_value = HOST_2_LE32 (port_sts);
    }

    memcpy (irp_buffer, (UINT8 *) &reg_value, 4);

    (VOID)NU_USB_IRP_Set_Actual_Length((NU_USB_IRP *) irp, 4);

    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_RH_Clear_Feature
*
* DESCRIPTION
*
*       This function handles Clear_Port_Feature request for the root hub.
*
* INPUTS
*
*      xhci                  Pointer to xHC driver control block.
*      irp                   Pointer to IRP containing request description.
*      speed                 Unused parameter.
*
* OUTPUTS
*
*      NU_SUCCESS            Successful completion.
*      NU_USB_INVLD_ARG      Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_RH_Clear_Feature (NU_USBH_XHCI     *xhci,
                              NU_USBH_CTRL_IRP *irp,
                              UINT8            speed)
{
    /* Progress status. */
    STATUS               status         = NU_SUCCESS;

    /* OP register handle. */
    NU_USBH_XHCI_OP_REGS *op_regs       = NU_NULL;

    /* Port number for which feature is to be selected. */
    UINT8                port           = 0;

    /* Feature selector. */
    UINT8                feature        = 0;
    UINT32               temp           = 0;
    UINT32               reg_value      = 0;
    UINT32               addr           = 0;

    /* Setup packet fields. */
    UINT8                bmRequestType  = 0;
    UINT16               wIndex         = 0;
    UINT16               wValue         = 0;


    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(irp);

    NU_UNUSED_PARAM(speed);

    /* Get the operatioanl register base address pointer. */
    status = XHCI_Get_Operational_Regs_Handle( &xhci->reg_base, &op_regs );
    if ( status == NU_SUCCESS )
    {
        (VOID)NU_USBH_CTRL_IRP_Get_bmRequestType( irp, &bmRequestType );
        (VOID)NU_USBH_CTRL_IRP_Get_wValue( irp, &wValue );
        (VOID)NU_USBH_CTRL_IRP_Get_wIndex( irp, &wIndex );

        wValue = LE16_2_HOST(wValue);
        wIndex = LE16_2_HOST(wIndex);
        port = USBH_XHCI_RH_LSB (wIndex);
        feature = USBH_XHCI_RH_LSB (wValue);

        addr = (UINT32)&op_regs->xhci_port_status_base + XHCI_PORT_OFFSET(port);

        /* Class specific SET FEATURE.
         * At xhci rh, no hub level set features are supported.
         * Only port level are supported.
         */

        /* Acknowledges the port status change by clearing the changed bit-
         * Write 1 to clear.
         */
        switch (feature)
        {

            case USBH_HUB_FEATURE_C_PORT_CONNECTION:
                temp = XHCI_PORT_CSC;
            break;

            case USBH_HUB_FEATURE_C_PORT_ENABLE:
                temp = XHCI_PORT_PEC;
            break;

            case USBH_HUB_FEATURE_C_PORT_OVER_CURRENT:
                temp = XHCI_PORT_OCC;
            break;

            case USBH_HUB_FEATURE_C_PORT_RESET:
                temp = XHCI_PORT_RC;
            break;

            case USBH_HUB_FEATURE_C_PORT_LINK_STATE:
                temp = XHCI_PORT_PLC;
            break;

            case USBH_HUB_FEATURE_C_PORT_RESET_BH:
                temp = XHCI_PORT_WRC;
            break;

            /* Want to disable the port. SW cant enable the port */
            case USBH_HUB_FEATURE_PORT_ENABLE:
                temp = XHCI_PORT_PE;
            break;

            default:;
        }

        /* Read the PORTSC register. */
        XHCI_HW_READ32(xhci, addr, reg_value);
        reg_value = (reg_value & XHCI_PORT_RO) | (reg_value & XHCI_PORT_RWS);
        reg_value |= temp;
        XHCI_HW_WRITE32(xhci, addr, reg_value);
    }

    return ( status );
}


/**************************************************************************
*
* FUNCTION
*
*       XHCI_RH_Invalid_CMD
*
* DESCRIPTION
*
*       This function handles non-supported request for the root hub.
*
* INPUTS
*
*      xhci         Unused parameter.
*      ctrl_irp     Unused parameter.
*      speed        Unused parameter.
*
* OUTPUTS
*     NU_SUCCESS           Always
*
**************************************************************************/
STATUS XHCI_RH_Invalid_CMD(NU_USBH_XHCI     *xhci,
                           NU_USBH_CTRL_IRP *ctrl_irp,
                           UINT8            speed)
{
    NU_UNUSED_PARAM(xhci);
    NU_UNUSED_PARAM(ctrl_irp);
    NU_UNUSED_PARAM(speed);

    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_RH_Get_State
*
* DESCRIPTION
*
*       This function handles Get_State request for root.
*       ports.
*
* INPUTS
*
*       xhci         Unused parameter.
*       ctrl_irp     Unused parameter.
*       speed        Unused parameter.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*
**************************************************************************/
STATUS XHCI_RH_Get_State(NU_USBH_XHCI       *xhci,
                         NU_USBH_CTRL_IRP   *ctrl_irp,
                                            UINT8 speed)
{
    NU_UNUSED_PARAM(xhci);
    NU_UNUSED_PARAM(ctrl_irp);
    NU_UNUSED_PARAM(speed);

    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_RH_Nothing_To_Do_CMD
*
* DESCRIPTION
*
*       This function handles port Set_Feature request for root hub.
*
* INPUTS
*
*      xhci                 Unused parameter.
*      ctrl_irp             Unused parameter.
*      speed                Unused parameter.
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*      NU_USB_INVLD_ARG     Any of the input argument in invalid.
*                    
**************************************************************************/
STATUS XHCI_RH_Nothing_To_Do_CMD(NU_USBH_XHCI     *xhci,
                                 NU_USBH_CTRL_IRP *ctrl_irp,
                                 UINT8            speed)
{
    NU_UNUSED_PARAM(xhci);
    NU_UNUSED_PARAM(ctrl_irp);
    NU_UNUSED_PARAM(speed);

    return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_RH_Set_Feature
*
* DESCRIPTION
*
*       This function handles port Set_Port_Feature request for root hub.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       irp                  Pointer to IRP containing request description
*       speed                Unused parameter.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*                     
**************************************************************************/
STATUS XHCI_RH_Set_Feature (NU_USBH_XHCI     *xhci,
                            NU_USBH_CTRL_IRP *irp,
                            UINT8            speed)
{
    /* Progress status. */
    STATUS               status         = NU_SUCCESS;

    /* OP register handle. */
    NU_USBH_XHCI_OP_REGS *op_regs       = NU_NULL;

    /* Setup packet fields. */
    UINT8                bmRequestType = 0;
    UINT16               wIndex        = 0;
    UINT16               wValue        = 0;

    /* Port number for which feature is to be selected. */
    UINT8                port          = 0;

    /* Feature selector. */
    UINT8                feature       = 0;
    UINT8                selector      = 0;
    UINT32               reg_value     = 0;
    UINT32               addr          = 0;
    UINT32               reg_value2    = 0;

    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(irp);

    NU_UNUSED_PARAM(speed);

    status = XHCI_Get_Operational_Regs_Handle( &xhci->reg_base, &op_regs );

    if ( status ==  NU_SUCCESS )
    {
        (VOID)NU_USBH_CTRL_IRP_Get_bmRequestType( irp, &bmRequestType );
        (VOID)NU_USBH_CTRL_IRP_Get_wValue( irp, &wValue );
        (VOID)NU_USBH_CTRL_IRP_Get_wIndex( irp, &wIndex );

        wValue = LE16_2_HOST(wValue);
        wIndex = LE16_2_HOST(wIndex);

        /* Class specific SET FEATURE.
         * At xhci rh, no hub level set features are supported.
         * Only port level are supported.
         */

        port = USBH_XHCI_RH_LSB (wIndex);
        selector = USBH_XHCI_RH_MSB (wIndex);
        feature = USBH_XHCI_RH_LSB (wValue);

        addr = (UINT32)&op_regs->xhci_port_status_base + XHCI_PORT_OFFSET(port);
        XHCI_HW_READ32(xhci, addr, reg_value);

        addr = (UINT32)&op_regs->xhci_port_power_base + XHCI_PORT_OFFSET(port);
        XHCI_HW_READ32(xhci, addr, reg_value2);

        switch ( feature )
        {
            case USBH_HUB_FEATURE_PORT_LINK_STATE:

                /* Check the desired link state, if it is U3 then port must
                 * be enabled otherwise return error status - xHCI specs,
                 * sec 4.15.1.
                 */
                if ( selector == USB_LINK_STATE_U3 )
                {
                    if (!(reg_value & XHCI_PORT_PE) )
                    {
                        status = XHCI_ERR_PORT_DISABLED;
                        break;
                    }

                }
                reg_value = (reg_value & XHCI_PORT_RO)|(reg_value & XHCI_PORT_RWS);

                /* Update the PORTSC register PLS field .*/
                reg_value |= XHCI_SET_PORT_LS(selector) | XHCI_PORT_LINK_STROBE;
                addr = (UINT32)&op_regs->xhci_port_status_base + XHCI_PORT_OFFSET(port);
                XHCI_HW_WRITE32(xhci, addr, reg_value);

                /* Wait for the link state transition to complete.*/
                usb_wait_ms(10);
            break;

            case USBH_HUB_FEATURE_PORT_U1_TIMEOUT:

                /* Update the PORTPMSC register U1 timeout value. */
                reg_value2 &= ~0xFF;
                reg_value2 |= XHCI_PORT_U1_TIMEOUT(selector);
                addr = (UINT32)&op_regs->xhci_port_power_base + XHCI_PORT_OFFSET(port);
                XHCI_HW_WRITE32(xhci, addr, reg_value2);
            break;

            case USBH_HUB_FEATURE_PORT_U2_TIMEOUT:

                /* Update the PORTPMSC register U2 timeout value. */
                reg_value2 &= ~(0xFF00);
                reg_value2 |= XHCI_PORT_U2_TIMEOUT(selector);
                addr = (UINT32)&op_regs->xhci_port_power_base + XHCI_PORT_OFFSET(port);
                XHCI_HW_WRITE32(xhci, addr, reg_value2);
            break;

            /* PORTSC .*/
            case USBH_HUB_FEATURE_PORT_RESET:
                reg_value = (reg_value & XHCI_PORT_RO)|(reg_value & XHCI_PORT_RWS);
                reg_value |= XHCI_PORT_RESET;
                addr = (UINT32)&op_regs->xhci_port_status_base + XHCI_PORT_OFFSET(port);
                XHCI_HW_WRITE32(xhci, addr, reg_value);

                /* Wait for the port reset to complete. */
                usb_wait_ms(10);
                break;

            /* PORTSC .*/
            case USBH_HUB_FEATURE_PORT_POWER:
                reg_value = (reg_value & XHCI_PORT_RO)|(reg_value & XHCI_PORT_RWS);
                reg_value |= XHCI_PORT_POWER;
                addr = (UINT32)&op_regs->xhci_port_status_base + XHCI_PORT_OFFSET(port);
                XHCI_HW_WRITE32(xhci, addr, reg_value);

                /* Wait for the power to become stable.*/
                usb_wait_ms(20);
            break;

            default:;
        }
    }

    return ( status );
}


/**************************************************************************
*
* FUNCTION
*
*       XHCI_Allocate_TRB_Ring
*
* DESCRIPTION
*
*       This function allocates TRB ring according to given alignment
*       and number of segments.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       ring                 Pointer to allocated ring.
*       num_segments         Pointer xHCD device control block.
*       alignment            Index of endpoint used for data transfer.
*       link_trb             If link TRB is supported on RING.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Allocate_TRB_Ring (NU_USBH_XHCI      *xhci,
                               NU_USBH_XHCI_RING **ring,
                               UINT8             num_segments,
                               UINT32            alignment,
                               BOOLEAN           link_trb)
{
    /* Progress status .*/
    STATUS status                      = NU_SUCCESS;
    STATUS int_status                  = NU_SUCCESS;

    /* TRB ring pointer used for ring memory allocation. */
    NU_USBH_XHCI_RING    *loc_ring     = NU_NULL;

    /* Ring segments pointer - use for linking the segments. */
    NU_USBH_XHCI_SEGMENT *curr_segment = NU_NULL;
    NU_USBH_XHCI_SEGMENT *new_segment  = NU_NULL;

    /* Pointer to first TRB of ring. */
    NU_USBH_XHCI_TRB     *loc_trb      = NU_NULL;

    /* Number of segments in the ring. */
    UINT8                req_segments  = 0;

    /* Parameter validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(ring);

    req_segments = num_segments;

    /* Allocate memory for TRB ring control block from cache-able
     * memory pool.
     */
    status = NU_Allocate_Memory( xhci->cacheable_pool,
                                 (VOID **)&loc_ring,
                                 sizeof(NU_USBH_XHCI_RING),
                                 NU_SUSPEND );

    if ( status == NU_SUCCESS )
    {
        memset( (VOID *)loc_ring, 0x00 , sizeof(NU_USBH_XHCI_RING) );

        /* Allocate memory for TRB segment.*/
        status = XHCI_Allocate_TRB_Segment( xhci,
                                            &loc_ring->first_segment,
                                            alignment );
        if ( status == NU_SUCCESS)
        {
            --req_segments;
            curr_segment = loc_ring->first_segment;

            /* If required number of segments are greater than 1, then allocate
             * and link the required segments.
             */
            while ( (req_segments != 0) && (status == NU_SUCCESS) )
            {
                status = XHCI_Allocate_TRB_Segment( xhci,
                                                     &new_segment,
                                                     alignment );
                if (status == NU_SUCCESS)
                {
                    /* Link the TRB segments - Always returns SUCCESS. */
                    (VOID)XHCI_Link_TRB_Segments( xhci,
                                                  curr_segment,
                                                  new_segment,
                                                  link_trb );
                    --req_segments;
                    curr_segment = new_segment;
                }
                else
                {
                    break;
                }
            }
        }
    }

    if ( status == NU_SUCCESS )
    {
        /* Always returns success .*/
        (VOID)XHCI_Link_TRB_Segments( xhci,
                                     curr_segment,
                                     loc_ring->first_segment,
                                     link_trb );
        if ( link_trb )
        {
            /* Set the Toggle Cycle flag in the last TRB of last segment for Transfer and
             * Command rings.Link TRBs are not present on event ring.
             */
            curr_segment->trbs[XHCI_TRBS_PER_SEGMENT - 1].control |=
                                                       HOST_2_LE32(TRB_LINK_TOGGLE);
        }

        /* Initialize ring info. */
        loc_trb = loc_ring->first_segment->trbs;
        loc_ring->enqueue_ptr    = loc_trb;
        loc_ring->dequeue_ptr    = loc_ring->enqueue_ptr;
        loc_ring->last_segment   = curr_segment;
        loc_ring->enq_segment    = loc_ring->first_segment;
        loc_ring->deq_segment    = loc_ring->first_segment;
        loc_ring->num_segments   = num_segments;
        loc_ring->cycle_state    = 1;

        /* Link TRBs are not included in the available TRB count.*/
        loc_ring->available_trbs = num_segments *(XHCI_TRBS_PER_SEGMENT - 1) - 1;
        loc_ring->total_trbs = num_segments * XHCI_TRBS_PER_SEGMENT;
        *ring = loc_ring;
    }

    /* Cleanup in case of error. */
    if (status != NU_SUCCESS)
    {
        int_status = XHCI_Deallocate_TRB_Ring( loc_ring );
        NU_UNUSED_PARAM(int_status);
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Allocate_Event_Ring
*
* DESCRIPTION
*
*       This function is wrapper function and allocates event ring.
*
* INPUTS
*
*       xhci    Pointer to xHC driver control block.
*       ring    Pointer to allocated event ring.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Allocate_Event_Ring(NU_USBH_XHCI      *xhci,
                                NU_USBH_XHCI_RING **ring)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_XHCI_RING *event_ring = NU_NULL;

    NU_USB_PTRCHK(xhci);

    status = XHCI_Allocate_TRB_Ring ( xhci,
                                      &event_ring,
                                      XHCI_EVENT_RING_SEGMENTS,
                                      XHCI_TRANS_RING_ALIGNMENT,
                                      NU_FALSE );
    if ( status == NU_SUCCESS )
    {
        *ring = event_ring;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Allocate_Command_Ring
*
* DESCRIPTION
*
*       This is a wrapper function which allocates command ring.
*
* INPUTS
*
*       xhci           Pointer to xHC driver control block.
*       ring           Pointer to allocated command ring.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Allocate_Command_Ring(NU_USBH_XHCI      *xhci,
                                  NU_USBH_XHCI_RING **ring)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_XHCI_RING *cmd_ring = NU_NULL;

    NU_USB_PTRCHK(xhci);

    status = XHCI_Allocate_TRB_Ring ( xhci,
                                      &cmd_ring,
                                      XHCI_CMD_RING_SEGMENTS,
                                      XHCI_CMD_RING_ALIGNMENT,
                                      NU_TRUE );
    if ( status == NU_SUCCESS )
    {
        *ring = cmd_ring;
    }

    return ( status );

}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Allocate_TRB_Segment
*
* DESCRIPTION
*
*       This function allocates TRB segment.
*
* INPUTS
*
*       xhci               Pointer to xHC driver control block.
*       segment            Pointer to allocated segment.
*       alignment          Required alignment.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Allocate_TRB_Segment( NU_USBH_XHCI         *xhci,
                                  NU_USBH_XHCI_SEGMENT **segment,
                                  UINT32               alignment )
{
    /* Progress status. */
    STATUS               status       = NU_SUCCESS;
    STATUS               int_status   = NU_SUCCESS;

    /* Pointer to allocated ring segment. */
    NU_USBH_XHCI_SEGMENT *loc_segment = NU_NULL;

    /* Parameters validation.*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(segment);

    /* Allocate ring segment control block memory.*/
    status = NU_Allocate_Memory( xhci->cacheable_pool,
                                 (VOID **)&loc_segment,
                                 sizeof(NU_USBH_XHCI_SEGMENT),
                                 NU_SUSPEND );
    if( status == NU_SUCCESS )
    {
        memset( (VOID *)loc_segment , 0x00 , sizeof(NU_USBH_XHCI_SEGMENT) );

        /* Allocate ring segment memory from non cache-able memory pool. */
        status = NU_Allocate_Aligned_Memory( xhci->cb.pool,
                                             (VOID **)&loc_segment->trbs,
                                             XHCI_SEGMENT_SIZE,
                                             alignment,
                                             NU_SUSPEND );

        if  ( status == NU_SUCCESS )
        {
            memset( (VOID *)loc_segment->trbs , 0x00 , XHCI_SEGMENT_SIZE );
            loc_segment->next = NU_NULL;
            *segment          = loc_segment;
        }
        else
        {
            int_status = NU_Deallocate_Memory(loc_segment);
            NU_UNUSED_PARAM(int_status);
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*     XHCI_Deallocate_TRB_Segment
*
* DESCRIPTION
*
*     This function de-allocates TRB segment.
*
* INPUTS
*
*     segment              Pointer to segment to be de-allocated.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Deallocate_TRB_Segment(NU_USBH_XHCI_SEGMENT *segment)
{
    STATUS status = NU_USB_INVLD_ARG;

    if ( segment && (segment->trbs) )
    {
        status = NU_Deallocate_Memory( segment->trbs );
        segment->trbs = NU_NULL;
        status |= NU_Deallocate_Memory( segment );
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Link_TRB_Segments
*
* DESCRIPTION
*
*       This function links TRB segments.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       curr_segment         Pointer to segment already present in the ring.
*       new_segment          Pointer to new segment - segment to be linked.
*       link_trb             If the ring contains Link TRB, NU_TRUE for transfer
*                            and command ring and NU_FALSE for event ring.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Link_TRB_Segments(NU_USBH_XHCI         *xhci,
                             NU_USBH_XHCI_SEGMENT  *curr_segment,
                             NU_USBH_XHCI_SEGMENT  *new_segment,
                             BOOLEAN               link_trb)
{
    STATUS           status = NU_SUCCESS;
    NU_USBH_XHCI_TRB *trb   = NU_SUCCESS;
    UINT32           value  = 0;

    /* Parameters validation.*/
    NU_USB_PTRCHK(curr_segment);
    NU_USB_PTRCHK(new_segment);
    NU_UNUSED_PARAM(xhci);

    curr_segment->next = new_segment;

    /* For event ring there is no link TRB. */
    if ( link_trb )
    {
        trb = curr_segment->trbs;

        /* Chain bit is set in the link TRB as per xHCI specs, v0.95.*/
        value = SET_TRB_TYPE(XHCI_TRB_LINK);

        trb[XHCI_TRBS_PER_SEGMENT - 1].control = HOST_2_LE32(value);

        /* Load the link TRB parameter field with the address of first TRB of next segment. */
        XHCI_WRITE_ADDRESS(xhci, &trb[XHCI_TRBS_PER_SEGMENT - 1].parameter[0],
                                                                  (UINT32)new_segment->trbs);
        XHCI_WRITE_ADDRESS(xhci, &trb[XHCI_TRBS_PER_SEGMENT - 1].parameter[1], (UINT32)0x00);
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Deallocate_TRB_Ring
*
* DESCRIPTION
*
*       This function deallocates TRB ring.
*
* INPUTS
*
*       ring                 Pointer to the ring to be deallocated .
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Deallocate_TRB_Ring( NU_USBH_XHCI_RING *ring)
{
    STATUS               status        = NU_SUCCESS;
    NU_USBH_XHCI_SEGMENT *curr_segment = NU_NULL;
    NU_USBH_XHCI_SEGMENT *next_segment = NU_NULL;

    NU_USB_PTRCHK(ring);

    curr_segment = ring->first_segment;
    next_segment = ring->first_segment;

    while ( ring->num_segments )
    {
        if ( curr_segment )
        {
            next_segment = next_segment->next;
            status |= XHCI_Deallocate_TRB_Segment( curr_segment );
            curr_segment = next_segment;
            ring->num_segments--;
        }
    }

    status |= NU_Deallocate_Memory( ring );

    return ( status );
}
/**************************************************************************
*
* FUNCTION
*
*       XHCI_Room_On_Ring
*
* DESCRIPTION
*
*       This function checks if there are enough free TRBs on the ring
*       for data transfer.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       ring                 Pointer to the ring.
*       req_trbs             Required number of TRBs
*       alignment            Alignment of ring.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Room_On_Ring (NU_USBH_XHCI      *xhci,
                          NU_USBH_XHCI_RING *ring,
                          UINT32            req_trbs,
                          UINT32            alignment)
{
    /* Progress status.*/
    STATUS               status         = NU_SUCCESS;

    /* Number of segments required. */
    UINT8                req_segments   = 0;

    /* Loop index. */
    UINT8                index          = 0;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(ring);

    USBH_XHCI_PROTECT

    if ( ring->available_trbs < req_trbs )
    {
        /* Count the number of segments required,keeping in account the
         * link TRBs.
         */
        req_trbs = req_trbs - ring->available_trbs;

        for (index = 0;
               index < req_trbs ;
                 index += (XHCI_TRBS_PER_SEGMENT -1))
            ++req_segments;

        if ( ((ring->num_segments + req_segments)* XHCI_TRBS_PER_SEGMENT)
               < XHCI_RING_MAX_SIZE )
        {
            /* Queue the new segments in the ring.*/
            status = XHCI_Queue_TRB_Segments( xhci, ring, req_segments,
                                          NU_TRUE, alignment );
            if (status == NU_SUCCESS)
            {
                /* Increment the available TRB count. */
                ring->available_trbs +=
                      req_segments * (XHCI_TRBS_PER_SEGMENT -1);
                ring->total_trbs     +=
                      req_segments * XHCI_TRBS_PER_SEGMENT;
            }
        }
        else
        {
            status = XHCI_ERR_RING_FULL;
        }
    }

    USBH_XHCI_UNPROTECT

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Reinitialize_Ring
*
* DESCRIPTION
*
*       This fucntion reinitializes the ring .
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       ring                 Pointer to the ring.
*       ep_index             Endpoint index.
*       slot_id              Slot ID.
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Reinitialize_Ring(NU_USBH_XHCI *xhci,
                        NU_USBH_XHCI_RING *ring,
                        UINT8        ep_index,
                        UINT8        slot_id)
{

    STATUS status;
    NU_USBH_XHCI_SEGMENT  *tmp_segment = NU_NULL;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(ring);

    /* Traverse the ring and memset 0 all the TRBS in the segment
     * except the last TRB. This will avoid need of re-linking
     * the TRB segments.
     */
    tmp_segment = ring->first_segment;

    do
    {
        memset (tmp_segment->trbs , 0x00 , (XHCI_SEGMENT_SIZE - XHCI_TRB_SIZE) );

        tmp_segment->trbs[XHCI_TRBS_PER_SEGMENT-1].control &=
                              HOST_2_LE32(~(UINT32)TRB_CYCLE_BIT_MASK);
        tmp_segment = tmp_segment->next;

    } while ( tmp_segment != ring->first_segment );

    /* Reinitialize the ring info.*/
    ring->cycle_state    = 1;
    ring->enqueue_ptr    = ring->first_segment->trbs;
    ring->dequeue_ptr    = ring->enqueue_ptr;
    ring->enq_segment    = ring->first_segment;
    ring->deq_segment    = ring->enq_segment;
    ring->available_trbs =(XHCI_TRBS_PER_SEGMENT - 1)*(ring->num_segments) - 1;

    /* Point the ring dequeue pointer to the beginning of first TRB in the ring.*/
    status = XHCI_Queue_Set_TR_Dequeue_Command( xhci,
                                                ring->enqueue_ptr,
                                                ep_index,
                                                ring->cycle_state,
                                                0,
                                                slot_id );

    return ( status);
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Queue_TRB_Segments
*
* DESCRIPTION
*
*       This function inserts new segment in the ring.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       ring                 Pointer to the ring.
*       num_segments         Number of segments to be inserted.
*       req_trbs             Required number of TRBs
*       alignment            Alignment of ring.
*
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Queue_TRB_Segments(NU_USBH_XHCI      *xhci,
                               NU_USBH_XHCI_RING *ring,
                               UINT8             num_segments,
                               BOOLEAN           link_trb,
                               UINT32            alignment)
{
    /* Progress status. */
    STATUS               status        = NU_SUCCESS;

    /* Segment containing the enqueue pointer. */
    NU_USBH_XHCI_SEGMENT *enq_segment  = NU_NULL ;

    /* Enqueue segment. */
    NU_USBH_XHCI_SEGMENT *curr_segment = NU_NULL;

    /* New allocated segment.*/
    NU_USBH_XHCI_SEGMENT *new_segment  = NU_NULL;

    /* Working segment. */
    NU_USBH_XHCI_SEGMENT *tmp_segment  = NU_NULL;

     /* Num of segments required. */
     UINT8                num_seg       = 0;

    if ( num_segments == 0 )
    {
        return  ( status );
    }

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(ring);
    NU_USB_PTRCHK(ring->enq_segment);
    NU_USB_PTRCHK(ring->deq_segment);
    NU_USB_PTRCHK(ring->enqueue_ptr);
    NU_USB_PTRCHK(ring->dequeue_ptr);


    enq_segment  = ring->enq_segment;
    curr_segment = enq_segment;
    num_seg      = num_segments;
    tmp_segment  = enq_segment->next;

    /* New segment is only inserted if any of link TRBs is owned by the
     * producer of TRBs. TRBs between dequeue and enqueue pointer are owned
     * by the consumer and producer should not access them.
     */
    if ( ring->enq_segment == ring->deq_segment )
    {
        /* If ring enqueue and dequeue pointers lie in the same segment,
         * then check if enqueue pointer is greater than dequeue pointer.If
         * TRUE then producer owns the link TRB and new segment can be
         * inserted
         */
        if ( ring->enqueue_ptr < ring->dequeue_ptr )
        {
            status = XHCI_ERR_RING_FULL;
        }
    }

   /* Since room is available on the ring for queuing new segments, so
    * allocate new segments and link them previous ones.
    */
    while ( (num_seg != 0) && (status == NU_SUCCESS) )
    {
        status = XHCI_Allocate_TRB_Segment( xhci, &new_segment, alignment );

        if ( status == NU_SUCCESS )
        {
            status = XHCI_Link_TRB_Segments( xhci, curr_segment, new_segment,
                                             link_trb );
        }

        curr_segment = new_segment;
        --num_seg;
    }

    if ( status == NU_SUCCESS )
    {
        (VOID)XHCI_Link_TRB_Segments( xhci, curr_segment,
                                      tmp_segment, link_trb );
        if ( link_trb )
        {
            /* Set the link toggle flag in the link TRB of last segment.*/
            if ( tmp_segment == ring->first_segment )
            {
                curr_segment->trbs[XHCI_TRBS_PER_SEGMENT - 1].control |=
                                                          HOST_2_LE32(TRB_LINK_TOGGLE);
                tmp_segment->trbs[XHCI_TRBS_PER_SEGMENT - 1].control &=
                                                          HOST_2_LE32(~(UINT32)TRB_LINK_TOGGLE);
                ring->last_segment = curr_segment;
            }
        }

        /* Increment the ring segment count. */
        ring->num_segments += num_segments;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Inc_Ring_Enqueue_Ptr
*
* DESCRIPTION
*
*       This function increments ring enqueue pointer.
*
* INPUTS
*
*       ring                 Pointer to ring control block.
*       trb_producer         NU_TRUE if the function is called by the producer
*                            of TRBs like command and transfer ring otherwise false.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Inc_Ring_Enqueue_Ptr(NU_USBH_XHCI_RING *ring,
                                 BOOLEAN           trb_producer)
{
    STATUS                status = NU_SUCCESS;
    NU_USBH_XHCI_TRB     *trb;
    NU_USBH_XHCI_TRB     *temp_trb;
    NU_USBH_XHCI_SEGMENT *enq_segment;
    NU_USBH_XHCI_SEGMENT *last_segment;
    UINT32                trb_ctrl_field ;

    /* Parameters validation .*/
    NU_USB_PTRCHK(ring);
    NU_USB_PTRCHK(ring->last_segment);
    NU_USB_PTRCHK(ring->enq_segment);
    NU_USB_PTRCHK(ring->enqueue_ptr);

    enq_segment  = ring->enq_segment;
    last_segment = ring->last_segment;

    /* Increment the ring enqueue pointer. */
    trb = ++(ring->enqueue_ptr);
    trb_ctrl_field = LE32_2_HOST(trb->control);

    /* Decrement the available TRB count. */
    --(ring->available_trbs);

    /* Check if this is the last TRB(Link TRB) in the segment. */
    if ( (GET_TRB_TYPE(trb_ctrl_field) == XHCI_TRB_LINK )
        || (trb == &(last_segment->trbs[XHCI_TRBS_PER_SEGMENT -1])) )
    {
        if ( trb_ctrl_field & TRB_CYCLE_BIT_MASK )
        {
            trb_ctrl_field &= ~SET_NORM_TRB_CYCLE;
        }
        else
        {
            trb_ctrl_field |= SET_NORM_TRB_CYCLE;
        }

        temp_trb = &enq_segment->trbs[XHCI_TRBS_PER_SEGMENT - 2];

        if ( HOST_2_LE32(temp_trb->control) & SET_NORM_TRB_CHAIN )
        {
           trb_ctrl_field |= SET_NORM_TRB_CHAIN;
        }
        else
        {
          trb_ctrl_field &= ~SET_NORM_TRB_CHAIN;
        }

        /* If this is the last TRB in the last segment and increment
         * pointer is called by the producer of TRBs then toggle the ring
         * cycle state. This condition holds for Transfer and CMD rings.
         */
        if ( ( trb_producer ) &&
             ( trb == &(last_segment->trbs[XHCI_TRBS_PER_SEGMENT-1]) ) )
        {
            ring->cycle_state = ((ring->cycle_state)? 0 : 1);
        }

        trb->control = HOST_2_LE32(trb_ctrl_field);

        /* Update the enqueue segment pointer. */
        enq_segment = enq_segment->next;
        ring->enq_segment = enq_segment;
        ring->enqueue_ptr = enq_segment->trbs;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Inc_Ring_Dequeue_Ptr
*
* DESCRIPTION
*
*       This function increments ring dequeue pointer.
*
* INPUTS
*
*       ring                 Pointer to ring control block.
*       trb_consumer         NU_TRUE if the function is called by the consumer
*                            of TRBs like for event ring otherwise NU_FALSE.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Inc_Ring_Dequeue_Ptr(NU_USBH_XHCI_RING *ring,
                                 BOOLEAN           trb_consumer)
{
    STATUS                status = NU_SUCCESS;
    NU_USBH_XHCI_TRB     *trb;
    NU_USBH_XHCI_SEGMENT *deq_segment;
    NU_USBH_XHCI_SEGMENT *last_segment;
    UINT32                trb_ctrl_field;

    /* Parameters validation .*/
    NU_USB_PTRCHK(ring);
    NU_USB_PTRCHK(ring->last_segment);
    NU_USB_PTRCHK(ring->deq_segment);
    NU_USB_PTRCHK(ring->dequeue_ptr);

    deq_segment  = ring->deq_segment;
    last_segment = ring->last_segment;

    /* Increment the ring dequeue pointer. */
    trb = ++(ring->dequeue_ptr);
    trb_ctrl_field = LE32_2_HOST(trb->control);

    /* Increment the available TRB count. */
    ++(ring->available_trbs);

    if ( ring->available_trbs > (ring->total_trbs - 2) )
    {
        ring->available_trbs = ring->total_trbs - 2;
    }

    /* Check if this is the last TRB(Link TRB) in the segment. */
    if ( (GET_TRB_TYPE(trb_ctrl_field) == XHCI_TRB_LINK)
         || ( trb == &last_segment->trbs[XHCI_TRBS_PER_SEGMENT] ) )
    {
        if ( trb_consumer && (trb == &last_segment->trbs[XHCI_TRBS_PER_SEGMENT]) )
        {
            /* If this is the last TRB in the last segment and increment
             * pointer is called by the consumer of TRBs then toggle the ring
             * cycle state. This condition holds for the event ring.
             */
            ring->cycle_state = ((ring->cycle_state)? 0 : 1);
        }
        /* Update the dequeue segment pointer. */
        deq_segment = deq_segment->next;
        ring->deq_segment = deq_segment;
        ring->dequeue_ptr = deq_segment->trbs;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Allocate_Scratchpad_Memory
*
* DESCRIPTION
*
*       This function allocates scratchpad memory for host controller.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Allocate_Scratchpad_Memory (NU_USBH_XHCI *xhci)
{
    /* Progress status. */
    STATUS                  status      = NU_SUCCESS;
    STATUS                  int_status  = NU_SUCCESS;

    /* Number of buffers required for scratchpad. */
    UINT8                   num_buffers = 0;

    /* Loop index. */
    UINT8                   index       = 0;

    /*CAP register set handle. */
    NU_USBH_XHCI_CAP_REGS   *cap_regs   = NU_NULL;

    /* Allocated scratchpad memory. */
    NU_USBH_XHCI_SCRATCHPAD *sp_cb      = NU_NULL;
    UINT8                   roll_back   = 0;
    UINT32                  reg_value   = 0;
    UINT32                  sp_addr     = 0;

    /* Parameters validation .*/
    NU_USB_PTRCHK(xhci);

    status = XHCI_Get_Capability_Regs_Handle( &xhci->reg_base, &cap_regs );
    if ( status == NU_SUCCESS )
    {
        /* Get the number of scratch pad buffers required from the HCSPARAMS2 register. */
        XHCI_HW_READ32(xhci,&cap_regs->xhci_hcs_params2,reg_value);
        num_buffers = XHCI_HCS_MAX_SCRATCHPAD_BUFF(reg_value);

        if ( num_buffers > 0 )
        {
            /* Allocate memory for scratch pad control block. */
            status = NU_Allocate_Memory( xhci->cacheable_pool,
                                         (VOID **)&sp_cb,
                                         sizeof(NU_USBH_XHCI_SCRATCHPAD),
                                         NU_SUSPEND );

            if ( status == NU_SUCCESS )
            {
                /* Allocate memeory for scratchpad array. */
                status = NU_Allocate_Aligned_Memory( xhci->cb.pool,
                                                     (VOID **)&(sp_cb->sp_array),
                                                     (num_buffers * sizeof(NU_USBH_XHCI_SP_ARRAY)),
                                                     XHCI_SP_ARRAY_ALIGNMENT,
                                                     NU_SUSPEND );

                if ( status == NU_SUCCESS )
                {
                    memset( (VOID*)sp_cb->sp_array, 0x00,
                               (num_buffers * sizeof(NU_USBH_XHCI_SP_ARRAY)) );

                     /* Allocate memory for scratch pad buffers. */
                    status = NU_Allocate_Aligned_Memory( xhci->cb.pool,
                                                         &(sp_cb->sp_buffer),
                                                         (num_buffers * XHCI_SP_BUFF_SIZE),
                                                         (4096),
                                                         NU_SUSPEND );

                    if ( status == NU_SUCCESS )
                    {
                        /* Save the buffer pointers in the scratch pad array. */
                        for (index = 0 ; index  < num_buffers ; index++)
                        {
                            sp_addr = (UINT32)sp_cb->sp_buffer + index * XHCI_SP_BUFF_SIZE;
                            XHCI_WRITE_ADDRESS(xhci,&sp_cb->sp_array[index].sp_buffer_lo, sp_addr);
                            XHCI_WRITE_ADDRESS(xhci,&sp_cb->sp_array[index].sp_buffer_hi,(UINT32)0x00);
                        }

                        xhci->scratchpad_cb = sp_cb;
                        XHCI_WRITE_ADDRESS(xhci,&xhci->dcbaa[0].dev_ctx_ptrs_lo,(UINT32)sp_cb->sp_array );
                        XHCI_WRITE_ADDRESS(xhci,&xhci->dcbaa[0].dev_ctx_ptrs_hi,(UINT32)0x00 );
                    }
                    else
                    {
                        roll_back = 2;
                    }
                }
                else
                {
                    roll_back = 1;
                }
            }
        }
    }

    /* Cleanup in case of error. */
    switch ( roll_back )
    {
        case 2:
            int_status = NU_Deallocate_Memory(sp_cb->sp_array);
            sp_cb->sp_array = NU_NULL;

        case 1:
            int_status = NU_Deallocate_Memory(sp_cb);
            sp_cb = NU_NULL;

        case 0:
            NU_UNUSED_PARAM(int_status);

        default:
        break;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Deallocate_Scratchpad_Memory
*
* DESCRIPTION
*
*       This function frees memory allocated for scratchpad buffer.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Deallocate_Scratchpad_Memory (NU_USBH_XHCI *xhci)
{
    STATUS status = NU_USB_INVLD_ARG;
    NU_USBH_XHCI_SCRATCHPAD *sp_cb = NU_NULL;

    /*Parameters validation.*/
    NU_USB_PTRCHK(xhci);

    /* Get the scratchpad control block. */
    sp_cb = xhci->scratchpad_cb;

    if ( sp_cb )
    {
        if (sp_cb->sp_buffer)
        {
            status = NU_Deallocate_Memory(sp_cb->sp_buffer);
            sp_cb->sp_buffer = NU_NULL;
        }

        if(sp_cb->sp_array)
        {
            status |= NU_Deallocate_Memory(sp_cb->sp_array);
            sp_cb->sp_array = NU_NULL;
        }

        status |= NU_Deallocate_Memory(sp_cb);
        xhci->scratchpad_cb = NU_NULL;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Allocate_Device_Context
*
* DESCRIPTION
*
*       This function allocates memory for xHCD device context.
*
* INPUTS
*
*       xhci               Pointer to xHC driver control block.
*       ctx                Pointer to allocated device context.
*       ctx_type           Type of context IN or OUT.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Allocate_Device_Context (NU_USBH_XHCI         *xhci,
                                     NU_USBH_XHCI_RAW_CTX **ctx,
                                     BOOLEAN              ctx_type)
{
    /* Progress status. */
    STATUS                status     = NU_SUCCESS;
    STATUS                int_status = NU_SUCCESS;

    /* Allocated context. */
    NU_USBH_XHCI_RAW_CTX  *loc_ctx   = NU_NULL;

    /* Cap register set. */
    NU_USBH_XHCI_CAP_REGS *cap_regs  = NU_NULL;

    /* Context size, different for input/output contexts.*/
    UINT32                ctx_size   = 0;
    UINT32                offset     = 0;
    UINT32                base       = 0;
    UINT8                 index      = 0;
    UINT32                reg_value  = 0;

    /* Parameters validation. */
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(ctx);

    /* Get pointer to Capability register set. */
    status = XHCI_Get_Capability_Regs_Handle( &xhci->reg_base, &cap_regs );
    if ( status == NU_SUCCESS )
    {
        /* Memory allocation for xHCI device context control block. */
        status = NU_Allocate_Memory(xhci->cacheable_pool,
                                    (VOID **) &loc_ctx,
                                    sizeof(NU_USBH_XHCI_RAW_CTX),
                                    NU_SUSPEND);

        if ( (status == NU_SUCCESS) && (loc_ctx != NU_NULL) )
        {
            memset ((VOID *)loc_ctx, 0x00, sizeof(NU_USBH_XHCI_RAW_CTX));

            /* Get the size of supported data structures (32 or 64 bit) from
             * HCCPARAMS register,table 23,section 5.3.6 xhci specs.
             */
            XHCI_HW_READ32(xhci, &cap_regs->xhci_hcc_params, reg_value);
            if (XHCI_HCC_64BYTE_CNTXT( reg_value ))
            {
                ctx_size = 2048;
                base = 64;
            }
            else
            {
                ctx_size = 1024;
                base = 32;
            }

            if ( ctx_type == XHCI_DEVICE_IN_CTX )
            {
                ctx_size += base;
            }

            status = NU_Allocate_Aligned_Memory(xhci->cb.pool,
                                                (VOID **)&loc_ctx->ctx_buffer,
                                                ctx_size,
                                                64,
                                                NU_SUSPEND );

            if ( status == NU_SUCCESS )
            {
                memset ((VOID *)loc_ctx->ctx_buffer, 0x00 , ctx_size);

                /* Populate the fields of xhci device raw context data structure.*/
                if ( ctx_type == XHCI_DEVICE_IN_CTX )
                {
                    /* For xHCI device input context ,the first context data
                     * structure is input control context.
                     */
                    loc_ctx->ctrl_ctx = (NU_USBH_XHCI_CTRL_CTX *)
                                            ( loc_ctx->ctx_buffer );

                    /* Increment the offset variable by context size so that the
                     * slot context is assigned from input context at index 1.
                     */
                    offset += base;
                }

                loc_ctx->slot_ctx = (NU_USBH_XHCI_SLOT_CTX *)
                                               ( offset + loc_ctx->ctx_buffer );
                offset += base;

                for (index = 0; index < 31 ; ++index)
                {
                    loc_ctx->ep_ctx[index] = (NU_USBH_XHCI_EP_CTX *)
                                                   (offset + loc_ctx->ctx_buffer);
                    offset += base;
                }

                loc_ctx->ctx_size = ctx_size;
                loc_ctx->ctx_type = ctx_type;
                *ctx = loc_ctx;
            }
            else
            {
                /* In case of error cleanup the raw context control block memory.*/
                if ( loc_ctx )
                {
                    int_status = NU_Deallocate_Memory( loc_ctx );
                    NU_UNUSED_PARAM(int_status);
                }
            }
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Deallocate_Device_Context
*
* DESCRIPTION
*
*       This function frees memory allocated for device context.
*
* INPUTS
*
*       ctx   Pointer to context to be de-allocated.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Deallocate_Device_Context(NU_USBH_XHCI_RAW_CTX *ctx)
{
    STATUS status = NU_USB_INVLD_ARG;

    if (ctx)
    {
        if (ctx->ctx_buffer)
        {
            status = NU_Deallocate_Memory(ctx->ctx_buffer);
            status |= NU_Deallocate_Memory(ctx);
        }
    }

    return ( status );
}
/**************************************************************************
*
* FUNCTION
*
*       XHCI_Init_IRP
*
* DESCRIPTION
*
*       This function initializes xHCI defined IRP structure.
*
* INPUTS
*
*      xhci                Pointer to xHC driver control block.
*      td                  Pointer to transfer descriptor.
*      irp                 Pointer to IRP to be transferred.
*      max_packet          Maximum packet size for endpoint.
*      trbs_req            TRBs required for transfer.
*      direction           Direction of data transfer IN/OUT.
*      empty_packet        ZLP required or not.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*
**************************************************************************/
__inline STATUS XHCI_Init_IRP( NU_USBH_XHCI_IRP *xhci_irp,
                               NU_USB_IRP       *irp,
                               NU_USBH_XHCI_TD  **td,
                               UINT16           ep_max_pack,
                               UINT32           trbs_req,
                               BOOLEAN          direction,
                               BOOLEAN          empty_packet )
{
    xhci_irp->irp           = irp;
    xhci_irp->rem_length    = irp->length;
    xhci_irp->tx_length     = irp->length;
    xhci_irp->buffer        = (UINT8 *)irp->buffer;
    xhci_irp->trbs_req      = trbs_req;
    xhci_irp->rem_trbs      = trbs_req;
    xhci_irp->direction     = direction;
    xhci_irp->max_p         = ep_max_pack;
    xhci_irp->total_length  = 0;
    xhci_irp->empty_packet  = empty_packet;
    *td                     = &xhci_irp->td;

   return ( NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Input_Control_Context
*
* DESCRIPTION
*
*       This function returns pointer to input control context.
*
* INPUTS
*
*       ctx                  Pointer to device input context.
*       ctrl_ctx             Pointer to returned control context.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS XHCI_Get_Input_Control_Context (NU_USBH_XHCI_RAW_CTX  *ctx,
                                                NU_USBH_XHCI_CTRL_CTX **ctrl_ctx)
{
    STATUS status = XHCI_ERR_INVLD_CTX;

    /* Parameters validation .*/
    NU_USB_PTRCHK(ctx);
    NU_USB_PTRCHK(ctrl_ctx);

    if ( (ctx->ctx_type == XHCI_IN_CTX) &&
          (ctx->ctrl_ctx != NU_NULL) )
    {
        *ctrl_ctx = ctx->ctrl_ctx;
        status   = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Slot_Context
*
* DESCRIPTION
*
*       This function is used to get device slot context.
*
* INPUTS
*
*       ctx                  Pointer to device context.
*       slot_ctx             Pointer to hold returned slot context.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS XHCI_Get_Slot_Context(NU_USBH_XHCI_RAW_CTX  *ctx,
                                      NU_USBH_XHCI_SLOT_CTX **slot_ctx)
{
    STATUS status = XHCI_ERR_INVLD_CTX;

    /* Parameters validation .*/
    NU_USB_PTRCHK(ctx);
    NU_USB_PTRCHK(slot_ctx);

    if (ctx->slot_ctx)
    {
        *slot_ctx = ctx->slot_ctx;
        status   = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Endpoint_Context
*
* DESCRIPTION
*
*       This function is used to get device endpoint context.
*
* INPUTS
*
*       ctx                 Pointer to device context.
*       ep_ctx              Pointer to hold returned endpoint context.
*       ep_index            Index of endpoint whose endpoint context is
*                           required.
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*      NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS XHCI_Get_Endpoint_Context(NU_USBH_XHCI_RAW_CTX  *ctx,
                                          NU_USBH_XHCI_EP_CTX   **ep_ctx,
                                          UINT8                 ep_index)
{
    STATUS status    = XHCI_ERR_INVLD_CTX;

    /* Parameters validation .*/
    NU_USB_PTRCHK(ctx);
    NU_USB_PTRCHK(ep_ctx);

    if ( ctx->ep_ctx[ep_index] )
    {
       *ep_ctx = ctx->ep_ctx[ep_index];
       status   = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Capability_Regs_Handle
*
* DESCRIPTION
*
*       This function returns capability register set handle.
*
* INPUTS
*
*       reg_base            Pointer to xHCD register base.
*       cap_reg             Handle of capability registers set.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS XHCI_Get_Capability_Regs_Handle(NU_USBH_XHCI_REG_BASE *reg_base,
                                                NU_USBH_XHCI_CAP_REGS **cap_reg)
{
    STATUS status = NU_USB_INVLD_ARG;

    NU_USB_PTRCHK(reg_base);
    NU_USB_PTRCHK(cap_reg);

    if (reg_base->capability_regs)
    {
        *cap_reg = reg_base->capability_regs;
        status = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Operational_Regs_Handle
*
* DESCRIPTION
*
*       This function returns operational registers set handle.
*
* INPUTS
*
*       reg_base             Pointer to xHCD register base.
*       op_reg               Handle of operational registers set.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS XHCI_Get_Operational_Regs_Handle(NU_USBH_XHCI_REG_BASE *reg_base,
                                                 NU_USBH_XHCI_OP_REGS  **op_reg)
{
    STATUS status = NU_USB_INVLD_ARG;

    NU_USB_PTRCHK(reg_base);
    NU_USB_PTRCHK(op_reg);

    if (reg_base->operational_regs)
    {
        *op_reg = reg_base->operational_regs;
        status = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Run_Time_Regs_Handle
*
* DESCRIPTION
*
*       This function returns run time registers set handle.
*
* INPUTS
*
*       reg_base             Pointer to xHCD register base.
*       rt_reg               Handle of run time registers set.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS XHCI_Get_Run_Time_Regs_Handle(NU_USBH_XHCI_REG_BASE *reg_base,
                                              NU_USBH_XHCI_RUN_REGS **rt_reg)
{
    STATUS status = NU_USB_INVLD_ARG;

    NU_USB_PTRCHK(reg_base);
    NU_USB_PTRCHK(rt_reg);

    if (reg_base->run_time_regs)
    {
        *rt_reg = reg_base->run_time_regs;
        status = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Doorbell_Regs_Handle
*
* DESCRIPTION
*
*       This function returns capability registers set handle.
*
* INPUTS
*
*       reg_base             Pointer to xHCD register base.
*       cap_reg              Handle of capability registers set.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS XHCI_Get_Doorbell_Regs_Handle(NU_USBH_XHCI_REG_BASE *reg_base,
                                              NU_USBH_XHCI_DB_ARRAY **db_reg)
{
    STATUS status = NU_USB_INVLD_ARG;

    NU_USB_PTRCHK(reg_base);
    NU_USB_PTRCHK(db_reg);

    if (reg_base->doorbell_array)
    {
        *db_reg = reg_base->doorbell_array;
        status = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Device_Handle
*
* DESCRIPTION
*
*       This function returns xHCD device handle.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       slot_id              Slot ID of device.
*       xhci_device          Pointer to returned device control block.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS  XHCI_Get_Device_Handle(NU_USBH_XHCI        *xhci,
                                        UINT8               slot_id,
                                        NU_USBH_XHCI_DEVICE **xhci_device)
{
    STATUS status = NU_USB_INVLD_ARG;

    /* Parameters Validation. */
    NU_USB_PTRCHK(xhci);
    NU_USB_PTRCHK(xhci_device);

    if ( slot_id < XHCI_MAX_SLOTS )
    {
        if ( xhci->device[slot_id] )
        {
            *xhci_device = xhci->device[slot_id];
            status = NU_SUCCESS;
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Endpoint_Handle
*
* DESCRIPTION
*
*       This function is used to obtain endpoint info control block
*       of particular endpoint.
*
* INPUTS
*
*       xhci_dev             Pointer xHCD device control block.
*       ep_index             Index of endpoint.
*       ep_info              Returned endpoint info control block.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS  XHCI_Get_Endpoint_Handle(NU_USBH_XHCI_DEVICE  *xhci_device,
                                         UINT8                 ep_index,
                                         NU_USBH_XHCI_EP_INFO  **ep_info)
{
    STATUS status = NU_SUCCESS;

    /* Parameter Validation. */
    NU_USB_PTRCHK(ep_info);
    NU_USB_PTRCHK(xhci_device);

    *ep_info = &xhci_device->ep_info[ep_index];

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Endpoint_Ring_Handle
*
* DESCRIPTION
*
*       This function returns Ring associated with the endpoint.
*
* INPUTS
*
*       ep_info              Pointer endpoint info control block.
*       ep_ring              Returned ring control block.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS  XHCI_Get_Endpoint_Ring_Handle(NU_USBH_XHCI_EP_INFO *ep_info,
                                               NU_USBH_XHCI_RING    **ep_ring)
{
    STATUS status = NU_USB_INVLD_ARG;

    /* Parameter Validation. */
    NU_USB_PTRCHK(ep_info);
    NU_USB_PTRCHK(ep_ring);

    if ( ep_info->ring )
    {
        *ep_ring = ep_info->ring;
        status = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Event_Ring_Handle
*
* DESCRIPTION
*
*       This function returns pointer to event ring.
*
* INPUTS
*
*       cb                   Pointer to event ring info control block.
*       ring                 Pointer to the returned event ring.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS XHCI_Get_Event_Ring_Handle(NU_USBH_XHCI_EVENT_RING *cb,
                                           NU_USBH_XHCI_RING       **ring)
{
    STATUS status = NU_USB_INVLD_ARG;
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(ring);

    if ( cb->event_ring )
    {
        *ring = cb->event_ring;
        status = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Stream_Ring_handle
*
* DESCRIPTION
*
*       This function returns handle of ring associated with the stream.
*
* INPUTS
*
*       strm_info            Pointer to allocated stream info control block.
*       stream_id            Stream ID
*       ring                 Ring associated with stream.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS XHCI_Get_Stream_Ring_handle( NU_USBH_XHCI_STREAM_INFO *strm_info,
                                             UINT16                   stream_id,
                                             NU_USBH_XHCI_RING        **ring )
{
    STATUS status = NU_USB_INVLD_ARG;

    NU_USB_PTRCHK(strm_info);
    NU_USB_PTRCHK(ring);
    NU_USB_PTRCHK(stream_id);

    if ( strm_info->stream_ring[stream_id] )
    {
        *ring = strm_info->stream_ring[stream_id];
        status = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Get_Command_Ring_Handle
*
* DESCRIPTION
*
*       This function returns command ring handle.
*
* INPUTS
*
*       cb                   Pointer to command ring info control block.
*       ring                 Returned pointer.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
__inline STATUS XHCI_Get_Command_Ring_Handle(NU_USBH_XHCI_CMD_RING *cb,
                                             NU_USBH_XHCI_RING     **ring)
{
    STATUS status = NU_USB_INVLD_ARG;
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(ring);

    if ( cb->cmd_ring )
    {
        *ring  = cb->cmd_ring;
        status = NU_SUCCESS;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Allocate_Streams
*
* DESCRIPTION
*
*       This function allocates xHCD data structure required for
*       streams management.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       num_streams          Number of streams, define stream context array
*                            size.
*       strm_info            Pointer to allocated stream info control block.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Allocate_Streams( NU_USBH_XHCI             *xhci,
                              UINT16                   num_streams,
                              NU_USBH_XHCI_STREAM_INFO **strm_info)
{
    /* Progress status. */
    STATUS                   status = NU_SUCCESS;
    STATUS                   int_status = NU_SUCCESS;

    /* Cap register set. */
    NU_USBH_XHCI_CAP_REGS    *cap_regs = NU_NULL;

    /* Allocated stream context. */
    NU_USBH_XHCI_STREAM_INFO *loc_strm_info = NU_NULL;

    /* Stream ring. */
    NU_USBH_XHCI_RING        *ring;

    /* Number of streams required. */
    UINT16                   max_streams;
    UINT16                   num_strm_ctx = 1;
    UINT32                   reg_value;
    UINT16                   index;
    UINT8                    rollback = 0;

    status = XHCI_Get_Capability_Regs_Handle( &xhci->reg_base, &cap_regs );

    if ( status == NU_SUCCESS )
    {
        /* Convert to nearest power of 2 - size of stream context array must be power of 2. */
        while ( num_strm_ctx < num_streams )
        {
            num_strm_ctx <<= 1;
        }
        /* Get the number of maximum primary streams array entries supported by the Host
         * controller.
         */
        XHCI_HW_READ32(xhci, &cap_regs->xhci_hcc_params, reg_value);
        max_streams = XHCI_HCC_MAX_PRI_STREAM_ARR(reg_value);

        /* Convert the num of streams into nearest power of 2. */
        if ( num_strm_ctx > max_streams )
        {
            status = XHCI_LIM_STREAM_ERR;
        }

        if ( status == NU_SUCCESS )
        {
            /* Allocate memory for stream control block. */
            status = NU_Allocate_Memory(xhci->cacheable_pool,
                                        (VOID **)&loc_strm_info,
                                        sizeof( NU_USBH_XHCI_STREAM_INFO),
                                        NU_SUSPEND);
            if ( status == NU_SUCCESS )
            {
                /* Allocate stream context array. */
                status = NU_Allocate_Aligned_Memory(xhci->cb.pool,
                                                    (VOID **) &loc_strm_info->stream_ctx_array,
                                                    (num_strm_ctx *
                                                     sizeof(NU_USBH_XHCI_STREAM_CTX)),
                                                    16,
                                                    NU_SUSPEND );
               if ( status == NU_SUCCESS )
               {
                    /* Allocate memory for streams ring .*/
                    status = NU_Allocate_Memory( xhci->cacheable_pool,
                                                 (VOID **)loc_strm_info->stream_ring,
                                                 (num_streams *(sizeof(NU_USBH_XHCI_RING *))),
                                                 NU_SUSPEND);
                    if ( status == NU_SUCCESS )
                    {
                        for ( index = 1; index < num_streams; ++index )
                        {
                            status = XHCI_Allocate_TRB_Ring( xhci,
                                                             &loc_strm_info->stream_ring[index],
                                                             1,
                                                             XHCI_TRANS_RING_ALIGNMENT,
                                                             NU_TRUE);

                            if ( status == NU_SUCCESS )
                            {
                                ring = loc_strm_info->stream_ring[index];
                                reg_value = (UINT32)ring->enqueue_ptr | ring->cycle_state
                                                                | XHCI_STREAM_CTX_TYPE(SCT_PRIMARY_TR);
                                XHCI_WRITE_ADDRESS( xhci,
                                                    &loc_strm_info->stream_ctx_array[index].deq_ptr_lo,
                                                    reg_value );
                                XHCI_WRITE_ADDRESS( xhci,
                                                    &loc_strm_info->stream_ctx_array[index].deq_ptr_hi,
                                                    reg_value );
                            }
                            else
                            {
                                rollback = 3;
                                break;
                            }
                        }
                        if ( status == NU_SUCCESS )
                        {
                            loc_strm_info->num_streams = num_streams;
                            *strm_info = loc_strm_info;
                        }
                    }
                    else
                        rollback = 2;
               }
               else
                  rollback = 1;
            }
        }
    }

    /* Clean up in case of error .*/
    switch ( rollback)
    {
        case 3:
            /* Deallocate the rings. */
            while ( index != 1 )
            {
                int_status |= XHCI_Deallocate_TRB_Ring(loc_strm_info->stream_ring[index]);
                XHCI_WRITE_ADDRESS( xhci,
                                    &loc_strm_info->stream_ctx_array[index].deq_ptr_lo,
                                     0 );
                XHCI_WRITE_ADDRESS( xhci,
                                    &loc_strm_info->stream_ctx_array[index].deq_ptr_hi,
                                    0 );
                --index;
            }

        case 2:
            int_status = NU_Deallocate_Memory(loc_strm_info->stream_ring);

        case 1:
            int_status |= NU_Deallocate_Memory(loc_strm_info->stream_ctx_array);

        default:
            NU_UNUSED_PARAM(int_status);
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Deallocate_Streams
*
* DESCRIPTION
*
*       This function frees stream related data structures.
*
* INPUTS
*
*       xhci                 Pointer to xHC driver control block.
*       strm_info            Pointer to stream info control block.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful completion.
*       NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
STATUS XHCI_Deallocate_Streams( NU_USBH_XHCI             *xhci,
                                NU_USBH_XHCI_STREAM_INFO *strm_info )
{
    STATUS status = NU_SUCCESS;
    UINT16 index = 0;

    NU_USB_PTRCHK(strm_info);

    for ( index = 0; index < strm_info->num_streams; ++index )
    {
        if ( strm_info->stream_ring[index] )
        {
            status |= XHCI_Deallocate_TRB_Ring(strm_info->stream_ring[index]);
            XHCI_WRITE_ADDRESS( xhci, &strm_info->stream_ctx_array[index].deq_ptr_lo, 0 );
            XHCI_WRITE_ADDRESS( xhci, &strm_info->stream_ctx_array[index].deq_ptr_hi, 0 );
            strm_info->stream_ring[index] = NU_NULL;
        }
    }

    if ( strm_info->stream_ring )
    {
        status |= NU_Deallocate_Memory(strm_info->stream_ring);
        strm_info->stream_ring = NU_NULL;
    }

    if ( strm_info->stream_ctx_array )
    {
        status |= NU_Deallocate_Memory(strm_info->stream_ctx_array);
        strm_info->stream_ctx_array = NU_NULL;
    }

    status |= NU_Deallocate_Memory(strm_info);

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Initialize_HW_Controller
*
* DESCRIPTION
*
*       This function performs HW specific initialization.
*
* INPUTS
*
*       cb                                  xHCI Control block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful initialization of
*                                           the hw.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*
**************************************************************************/
STATUS XHCI_Initialize_HW_Controller(NU_USB_HW    *cb)
{
    /* Progress status. */
    STATUS                      status     = NU_SUCCESS;
    STATUS                      int_status = NU_SUCCESS;

    /* Pointer to xHCD control block. */
    NU_USBH_XHCI                *xhci     = NU_NULL;

    /* Capability registers base address pointer */
    NU_USBH_XHCI_CAP_REGS       *cap_regs = NU_NULL;

    /* Operational registers base address pointer */
    NU_USBH_XHCI_OP_REGS        *op_regs  = NU_NULL;

    /* Run Time registers base address pointer */
    NU_USBH_XHCI_RUN_REGS       *rt_regs  = NU_NULL;

    /* Doorbell registers base address pointer */
    NU_USBH_XHCI_DB_ARRAY       *db_array = NU_NULL;

    /* Pointer to current interrupter register set */
    NU_USBH_XHCI_INTR_REG       *ir_set   = NU_NULL;

    /* xHCD ring pointer.*/
    NU_USBH_XHCI_RING           *ring     = NU_NULL;

    /* Event ring segment table entry. */
    NU_USBH_XHCI_ERST           *entry    = NU_NULL;

    /* Pointer to xHCI segment address. */
    NU_USBH_XHCI_SEGMENT        *segment  = NU_NULL;

    /* Registers values.*/
    UINT32                      reg_value = 0;
    UINT32                      reg_val2  = 0;

    /* Loop index variable. */
    UINT8                       index     = 0;
    UINT8                       tmp_var   = 0;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);

    /* Setup the register base of xHCD.*/
    xhci     = ( NU_USBH_XHCI *) cb;

    /* Get the base address of capability register set. */
    cap_regs = (NU_USBH_XHCI_CAP_REGS *) cb->base_address;

    /* Get the base address of the operational registers set from the CAPLENGTH
     * register present in the capability registers set.
     */
    XHCI_HW_READ32(xhci, &cap_regs->xhci_hc_capbase, reg_value);

    xhci->info_hub.xhci_specs_ver = XHCI_HCI_VERSION(reg_value);

    reg_value &= 0x00FF;

    op_regs    =(NU_USBH_XHCI_OP_REGS *)( cb->base_address + reg_value);

    /* Get the Run Time registers base address from the RTSOFF register
     * present in the capability registers set.
     */
    XHCI_HW_READ32(xhci, &cap_regs->xhci_run_regs_off, reg_value);

    reg_value &= XHCI_RUN_REGS_OFF_MASK;

    rt_regs   = (NU_USBH_XHCI_RUN_REGS *)((UINT32)cap_regs + reg_value);

    /* Set the interrupt register set. */
    ir_set    = (NU_USBH_XHCI_INTR_REG *)rt_regs->ir_set;

    /* Get the doorbell register base address from the DBOFF register
     * present in the capability registers set.
     */
    XHCI_HW_READ32(xhci, &cap_regs->xhci_db_off, reg_value);

    reg_value &= XHCI_DB_OFF_MASK;

    db_array  = (NU_USBH_XHCI_DB_ARRAY *)((UINT32)cap_regs + reg_value);

    /* For resetting the HW controller, HCH bit in USBSTS register must be set.
     * HCH  bit  is  asserted  when  RUN/STOP  bit  of  USBCMD  register  is 1(xhci
     * spes,table 28). Therefore  first read the  USBSTS register and  check if HCH
     * bit is set,if not then clear RUN/ STOP bit in the USBCMD register.
     */

    XHCI_HW_READ32(xhci, &op_regs->xhci_usb_status, reg_value);

    if (!(reg_value & XHCI_STATUS_HALT))
    {
        XHCI_HW_READ32(xhci, &op_regs->xhci_usb_command, reg_value);

        reg_value &= ~XHCI_USB_CMD_RUN;

        XHCI_HW_WRITE32(xhci, &op_regs->xhci_usb_command, reg_value);

        XHCI_HW_READ32(xhci, &op_regs->xhci_usb_status, reg_value);

        /* Wait for halt to complete.*/
        while ( !(reg_value & XHCI_USB_CMD_RESET) )
        {
            usb_wait_ms(10);
            XHCI_HW_READ32(xhci, &op_regs->xhci_usb_status, reg_value);
        }
    }

    /* Reset the Host Controller. */
    XHCI_HW_READ32(xhci, &op_regs->xhci_usb_command, reg_value);

    reg_value |= XHCI_USB_CMD_RESET;

    XHCI_HW_WRITE32(xhci, &op_regs->xhci_usb_command, reg_value);

    XHCI_HW_READ32(xhci, &op_regs->xhci_usb_command, reg_value);

    /* Wait for reset to complete. */
    while ( reg_value & XHCI_USB_CMD_RESET )
    {
        usb_wait_ms(10);
        XHCI_HW_READ32(xhci, &op_regs->xhci_usb_command, reg_value);
    }

    /* Update the Host Controller state. */
    xhci->state = XHCI_STATE_RESET;

    /* Get the maximum number of slots available in the Host Controller
     * from the HCSPARAMS1 register.
     */
    XHCI_HW_READ32(xhci, &cap_regs->xhci_hcs_params1, reg_value);

    /* Save the host controller information. */
    xhci->info_hub.max_slots        = reg_value & XHCI_HCS_MAX_DEV_SLOTS_MASK;
    xhci->info_hub.max_interrupters = XHCI_HCS_MAX_DEV_INTRS(reg_value);
    xhci->info_hub.num_ports        = XHCI_HCS_MAX_DEV_PORTS(reg_value);

    reg_value &= XHCI_HCS_MAX_DEV_SLOTS_MASK;

    tmp_var    = reg_value;

    XHCI_HW_READ32(xhci, &op_regs->xhci_config_reg, reg_val2);

    reg_val2  &= ~XHCI_HCS_MAX_DEV_SLOTS_MASK;

    reg_val2  |= reg_value;

    /* Program the maximum available slots in the CONFIG register. Required by
     * the host controller initialization sequence xHCI specs, section 4.2.
     */
    XHCI_HW_WRITE32(xhci, &op_regs->xhci_config_reg, reg_val2);

    XHCI_HW_READ32(xhci, &op_regs->xhci_page_size, reg_value);
    xhci->info_hub.page_size = reg_value & 0xFFFF;

    XHCI_HW_READ32(xhci, &cap_regs->xhci_hcc_params, reg_value);

    /* Save the base addresses of register sets in the xHCD register base
     * control block.
     */
    xhci->reg_base.capability_regs    = cap_regs;
    xhci->reg_base.operational_regs   = op_regs;
    xhci->reg_base.run_time_regs      = rt_regs;
    xhci->reg_base.doorbell_array     = db_array;
    xhci->reg_base.interrupt_reg_set  = ir_set;


    if ( status == NU_SUCCESS )
    {
        /* Allocate Device Context Base Address Array (DCBAA) - 64 byte aligned. */
        status = NU_Allocate_Aligned_Memory( xhci->cb.pool,
                                            (VOID**)&xhci->dcbaa,
                                            ((tmp_var + 1) *
                                            sizeof(NU_USBH_XHCI_DEV_CTX_ARRAY)),
                                            XHCI_DCBAA_ALINMENT,
                                            NU_SUSPEND );
        if ( status == NU_SUCCESS )
        {
            memset( xhci->dcbaa, 0x00,
                ((tmp_var + 1) * sizeof(NU_USBH_XHCI_DEV_CTX_ARRAY)) );

            /* Save the base address of the DCBAA in the DCBAA Pointer register present in
             * the operational registers set.
             */
            XHCI_WRITE_ADDRESS(xhci, &op_regs->xhci_dcbaa_ptr_lo,(UINT32)xhci->dcbaa );
            XHCI_WRITE_ADDRESS(xhci, &op_regs->xhci_dcbaa_ptr_hi,0);

            if ( status == NU_SUCCESS )
            {
                status = NU_Create_Semaphore( &xhci->protect_sem, "XHCI_SEM", 1, NU_PRIORITY  );

                /* This code segment allocates and initializes data structures required by the command
                 * engine of xHC.
                 */

                /* Allocate Command Ring .*/
                status = XHCI_Allocate_Command_Ring( xhci,
                                                     &xhci->cmd_ring_cb.cmd_ring );
                if ( status == NU_SUCCESS )
                {
                    /* Program the Command Ring Control Register (CRCR) with the command ring
                     * dequeue pointer (address of first TRB in the command ring).
                     */
                    ring = xhci->cmd_ring_cb.cmd_ring;

                    XHCI_READ_ADDRESS(xhci, &op_regs->xhci_cmd_ring_lo, reg_value);

                    reg_value =(reg_value & XHCI_COMMAND_RING_RSVD_BITS)
                                             | (UINT32)ring->enqueue_ptr
                                                 | (UINT32)ring->cycle_state;

                    XHCI_WRITE_ADDRESS(xhci, &op_regs->xhci_cmd_ring_lo,reg_value);

                    XHCI_WRITE_ADDRESS(xhci, &op_regs->xhci_cmd_ring_hi,0);

                    /* Semaphore for synchronizing command completion.*/
                    status = NU_Create_Semaphore( &xhci->cmd_ring_cb.cmd_comp_lock,
                                                  "CMD_SEM",
                                                  0,
                                                  NU_PRIORITY );
                }
            }
        }
    }

    /* This code segment allocates and initializes data structures required by the interrupt
     * interface of xHC.Inintiliaztion of interrupt interface of host controller is carried
     * out according to steps mentioned in the xHCI specs,section 4.2.
     */
    if ( status == NU_SUCCESS )
    {
        /* Allocate event ring for interrupter 0 only .*/
        status = XHCI_Allocate_Event_Ring( xhci,
                                           &xhci->event_ring_cb.event_ring);
        if ( status == NU_SUCCESS )
        {
            xhci->event_ring_cb.num_entries = XHCI_EVENT_RING_SEGMENTS;
            ring = xhci->event_ring_cb.event_ring;

            /* Allocate memory for event ring segment Table .*/
            status = NU_Allocate_Aligned_Memory( xhci->cb.pool,
                                                 (VOID**)&xhci->event_ring_cb.entries,
                                                 (XHCI_EVENT_RING_SEGMENTS*
                                                 sizeof(NU_USBH_XHCI_ERST)),
                                                 XHCI_ERST_ALIGNMENT,
                                                 NU_SUSPEND );
           if ( status == NU_SUCCESS )
           {
               memset( xhci->event_ring_cb.entries, 0x00,
                 (XHCI_EVENT_RING_SEGMENTS * sizeof(NU_USBH_XHCI_ERST)) );

               segment = ring->first_segment;

                /* Initialize the event ring segment table. */
               for ( index = 0 ; index < XHCI_EVENT_RING_SEGMENTS; ++index )
               {
                   entry = &xhci->event_ring_cb.entries[index];
                   XHCI_WRITE_ADDRESS(xhci,&entry->seg_addr_lo,(UINT32)segment->trbs);
                   XHCI_WRITE_ADDRESS(xhci,&entry->seg_addr_hi,0);
                   entry->seg_size = HOST_2_LE32(XHCI_TRBS_PER_SEGMENT);
                   segment = segment->next;
               }

               /* Program the number of event ring segment table entries in the
                * ERTSZ register.
                */
               XHCI_HW_READ32(xhci, &ir_set->xhci_erst_size, reg_value);
               reg_value  &= (UINT32)XHCI_EVT_RING_SEG_SIZE_MASK;
               reg_value |= XHCI_EVENT_RING_SEGMENTS;
               XHCI_HW_WRITE32(xhci, &ir_set->xhci_erst_size,reg_value);

               /* Set the event ring dequeue pointer in the ERDP register. */
               XHCI_READ_ADDRESS(xhci, &ir_set->xhci_erst_dequeue_lo, reg_value);
               reg_value  &= XHCI_EVT_RING_SEG_PTR_MASK;

               /* Want to save the EHB. */
               XHCI_READ_ADDRESS(xhci, &xhci->event_ring_cb.entries[0].seg_addr_lo, reg_val2);
               reg_value |= reg_val2;
               XHCI_WRITE_ADDRESS(xhci, &ir_set->xhci_erst_dequeue_lo,reg_value);
               XHCI_WRITE_ADDRESS(xhci, &ir_set->xhci_erst_dequeue_hi,0);

               /* Write the start address of Event Ring Segment Table
                * in the ERSTBA register.
                */
               XHCI_READ_ADDRESS(xhci, &ir_set->xhci_erst_base_lo, reg_value);
               reg_value &= 0xF;
               reg_value |= (UINT32)xhci->event_ring_cb.entries;
               XHCI_WRITE_ADDRESS(xhci, &ir_set->xhci_erst_base_lo,reg_value);
               XHCI_WRITE_ADDRESS(xhci, &ir_set->xhci_erst_base_hi,0);
           }
       }
    }

    /* Allocate the scratchpad memory for host controller.*/
    if ( status == NU_SUCCESS )
    {
        status = XHCI_Allocate_Scratchpad_Memory( xhci );
    }

    /* Cleanup in case of error. */
    if ( status != NU_SUCCESS )
    {
        int_status = NU_USBH_XHCI_Cleanup( xhci );
        NU_UNUSED_PARAM(int_status);
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Uninitialize_HW_Controller
*
* DESCRIPTION
*
*       This function performs HW specific un-initializations.
*
* INPUTS
*
*       cb                                  xHCI controller to be
*                                           un-initialized.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always.
*       NU_USB_INVLD_ARG                    Indicates an invalid parameter.
*
**************************************************************************/
STATUS XHCI_Uninitialize_HW_Controller(NU_USBH_XHCI *xhci)
{
    STATUS                status = NU_SUCCESS;

    /* Operational registers base address pointer. */
    NU_USBH_XHCI_OP_REGS  *op_regs  = NU_NULL;

    /* Run Time registers base address pointer. */
    NU_USBH_XHCI_RUN_REGS *rt_regs  = NU_NULL;

    UINT32                reg_value = 0;

    /* Parameter validation. */
    NU_USB_PTRCHK(xhci);

    /* Get the operational register handle.*/
    status = XHCI_Get_Operational_Regs_Handle( &xhci->reg_base, &op_regs );

    /* Get the run time register handle.*/
    if ( status == NU_SUCCESS )
    {
        status = XHCI_Get_Run_Time_Regs_Handle( &xhci->reg_base, &rt_regs );

        if ( status == NU_SUCCESS )
        {
            /* Halt HW controller. */
            reg_value  = ~XHCI_USB_INT_ENABLE;

            reg_value &= ~XHCI_USB_CMD_RUN;

            XHCI_HW_WRITE32(xhci, &op_regs->xhci_usb_command, reg_value);

            /* Need to wait here for controller to halt - 16 uFrame Max.*/
            usb_wait_ms(10);

            /* Reset the HW controller - This will reset the operational registers too. */
            reg_value = ~XHCI_USB_CMD_RESET;

            XHCI_HW_WRITE32(xhci, &op_regs->xhci_usb_command, reg_value);

            /* Need to wait here for controller to reset - 50 ms Max.*/
            usb_wait_ms(100);

            xhci->state = XHCI_STATE_RESET;

            /* Disable the event ring interrupts.*/
            XHCI_HW_READ32(xhci, &rt_regs->ir_set[0].xhci_iman, reg_value);

            reg_value |= XHCI_IMAN_IRQ_PENDING | ~XHCI_IR_INTE;

            XHCI_HW_WRITE32(xhci, &rt_regs->ir_set[0].xhci_iman,reg_value);

            /* Reset the ERST,ERDP and ERTSBA registers. */
            XHCI_HW_WRITE32(xhci, &rt_regs->ir_set[0].xhci_erst_size,0);

            XHCI_HW_WRITE32(xhci, &rt_regs->ir_set[0].xhci_erst_base_lo,0);

            XHCI_HW_WRITE32(xhci, &rt_regs->ir_set[0].xhci_erst_dequeue_lo,0);
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_XHCI_Cleanup
*
* DESCRIPTION
*
*       This function frees the resources grabbed by the xHC driver.
*
* INPUTS
*
*       xhci                                Pointer to xHC driver
*                                           control block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*       NU_USB_INVLD_ARG                    Any of the input argument is
*                                           invalid.
*
**************************************************************************/
STATUS NU_USBH_XHCI_Cleanup(NU_USBH_XHCI *xhci)
{
    STATUS status = NU_SUCCESS;

    /* Parameters Validation. */
    NU_USB_PTRCHK(xhci);

    /* Deallocate the device context base address array. */
    if ( xhci->dcbaa )
    {
        status = NU_Deallocate_Memory( xhci->dcbaa );
        xhci->dcbaa = NU_NULL;
    }

    if ( xhci->cmd_ring_cb.cmd_ring )
    {
        status |= XHCI_Deallocate_TRB_Ring( xhci->cmd_ring_cb.cmd_ring );
        xhci->cmd_ring_cb.cmd_ring = NU_NULL;
    }

    if ( xhci->event_ring_cb.event_ring )
    {
        status |= XHCI_Deallocate_TRB_Ring( xhci->event_ring_cb.event_ring );
        xhci->event_ring_cb.event_ring  = NU_NULL;
    }

    if ( xhci->event_ring_cb.entries )
    {
        status |= NU_Deallocate_Memory( xhci->event_ring_cb.entries );
        xhci->event_ring_cb.entries = NU_NULL;
    }

    if ( xhci->scratchpad_cb )
    {
        status |= XHCI_Deallocate_Scratchpad_Memory( xhci );
        xhci->scratchpad_cb = NU_NULL;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Event_Handler
*
* DESCRIPTION
*
*       This function performs preliminary processing on the events
*       generated by the host controller and delegate then to appropriate.
*       event handler functions.
*
* INPUTS
*
*       xhci               Pointer to xHC driver control block.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*
**************************************************************************/
VOID XHCI_Event_Handler(NU_USBH_XHCI *xhci)
{
    /* Event TRB handle. */
    NU_USBH_XHCI_TRB      *event_trb  = NU_NULL;

    /* Event Ring handle. */
    NU_USBH_XHCI_RING     *event_ring = NU_NULL;

    /* Run Time register pointer. */
    NU_USBH_XHCI_RUN_REGS *rt_reg     = NU_NULL;

    /* Register value. */
    UINT32                reg_value   = 0;

    if (!xhci)
    {
        return;
    }

    /* Get the Event Ring handle. */
    (VOID)XHCI_Get_Event_Ring_Handle( &xhci->event_ring_cb,
                                      &event_ring );

    /* Get the Run Time registers handle. */
    (VOID)XHCI_Get_Run_Time_Regs_Handle( &xhci->reg_base,
                                         &rt_reg );
    /* Read the first event TRB. */
    event_trb = event_ring->dequeue_ptr;

    /* See if xHC driver or Host Controller owns the TRB.*/
    if ( (HOST_2_LE32((event_trb->control)) & GET_EVENT_TRB_CYCLE)
         != (event_ring->cycle_state) )
    {
        /* TRB is owned by the Host Controller - return. */
        return ;
    }

    /* Process the Event Ring TRBs until the Ring is empty. */
    do{
        /* Decode the event TRB type field .*/
        switch ( GET_TRB_TYPE(HOST_2_LE32(event_trb->control)) )
        {
            /* Data transfer event .*/
            case XHCI_TRB_TRANSFER:
                (VOID)XHCI_Handle_Rx_Event( xhci, event_trb );
            break;

            /* Command completion event. */
            case XHCI_TRB_COMPLETION:
                (VOID)XHCI_Handle_Command_Completion( xhci, event_trb );
            break;

            /* Port status change event. */
            case XHCI_TRB_PORT_STATUS:
                (VOID)XHCI_Handle_RH_Event( xhci, event_trb );
            break;

            case XHCI_TRB_BANDWIDTH_EVENT:
            break;

            case XHCI_TRB_HC_EVENT:/* host controller internal error.*/
            break;

            case XHCI_TRB_DEV_NOTE:/* Remote wakeup,LTM */
            break;

            case XHCI_TRB_MFINDEX_WRAP:
            break;

            default:
            break;
        }

        /* Increment the Event Ring dequeue pointer. */
        XHCI_Inc_Ring_Dequeue_Ptr( event_ring, NU_TRUE );

        /* Get the next event TRB.*/
        event_trb = event_ring->dequeue_ptr;

        /* Check if xHC driver own the TRB. */
    } while ( (HOST_2_LE32(event_trb->control) & GET_EVENT_TRB_CYCLE)
              == (event_ring->cycle_state) );

    /* Update the event ring dequeue pointer. */
    XHCI_READ_ADDRESS(xhci, &rt_reg->ir_set[0].xhci_erst_dequeue_lo, reg_value);
    reg_value &=   XHCI_EVT_RING_SEG_PTR_MASK;

    /* Clear the Event Handler Busy (EHB) bit - Write 1 to clear. */
    reg_value |=  XHCI_EVT_RING_SEG_EHB;
    reg_value |=  (UINT32)event_ring->dequeue_ptr;
    XHCI_WRITE_ADDRESS(xhci, &rt_reg->ir_set[0].xhci_erst_dequeue_lo,reg_value);
    XHCI_WRITE_ADDRESS(xhci, &rt_reg->ir_set[0].xhci_erst_dequeue_hi,0);
    XHCI_READ_ADDRESS(xhci, &rt_reg->ir_set[0].xhci_erst_dequeue_hi,reg_value);

    return;
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Disable_Interrupts
*
* DESCRIPTION
*
*       This function disables host controller interrupts.
*
* INPUTS
*
*       xhci           Pointer to xHC driver control block.
*
* OUTPUTS
*
*       None
**************************************************************************/
VOID XHCI_Disable_Interrupts(NU_USBH_XHCI *xhci)
{
    /* Operational registers base address pointer. */
    NU_USBH_XHCI_OP_REGS  *op_reg   = NU_NULL;

    /* Run Time registers base address pointer. */
    NU_USBH_XHCI_RUN_REGS *rt_reg   = NU_NULL;
    UINT32                 reg_value = 0;

    if ( !xhci )
    {
        return;
    }

    (VOID)XHCI_Get_Operational_Regs_Handle( &xhci->reg_base, &op_reg );
    (VOID)XHCI_Get_Run_Time_Regs_Handle( &xhci->reg_base, &rt_reg );

    /* Disable the interrupter 0 interrupts. NEC controller clears the IP flag
     * even with writing 0 which is not as per spes.
     */
    XHCI_HW_READ32(xhci, &rt_reg->ir_set[0].xhci_iman, reg_value);
    reg_value = reg_value & ((UINT32)(~0x3));
    XHCI_HW_WRITE32(xhci, &rt_reg->ir_set[0].xhci_iman,reg_value);

    /* Disable the Host System interrupts. */
    XHCI_HW_READ32(xhci, &op_reg->xhci_usb_command, reg_value);
    reg_value &= ~XHCI_USB_CMD_EIE;
    XHCI_HW_WRITE32(xhci, &op_reg->xhci_usb_command, reg_value);
    xhci->int_disable_count++;
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Enable_Interrupts
*
* DESCRIPTION
*
*       This function enables host controller interrupts.
*
* INPUTS
*
*       xhci           Pointer to xHC driver control block.
*
* OUTPUTS
*
*       None
**************************************************************************/
VOID XHCI_Enable_Interrupts(NU_USBH_XHCI *xhci)
{
    /* Operational registers base address pointer. */
    NU_USBH_XHCI_OP_REGS  *op_reg   = NU_NULL;

    /* Run Time registers base address pointer. */
    NU_USBH_XHCI_RUN_REGS *rt_reg   = NU_NULL;
    UINT32                 reg_value = 0;

    if ( !xhci )
    {
        return;
    }

    if ( xhci->int_disable_count)
    {
        xhci->int_disable_count--;
    }

    /* Enable interrupts only when this routine is called as many times as
     * disable interrupt routine is called.
     */
    if ( xhci->int_disable_count == NU_NULL)
    {
        (VOID)XHCI_Get_Operational_Regs_Handle( &xhci->reg_base, &op_reg );
        (VOID)XHCI_Get_Run_Time_Regs_Handle( &xhci->reg_base, &rt_reg );

        XHCI_HW_READ32(xhci, &rt_reg->ir_set[0].xhci_imod,reg_value);
        reg_value &= ~0xFFFF ;
        reg_value |= (UINT32) 160;
        XHCI_HW_WRITE32(xhci, &rt_reg->ir_set[0].xhci_imod,reg_value);

        /* Enable the Host system interrupts. */
        XHCI_HW_READ32(xhci, &op_reg->xhci_usb_command, reg_value);
        reg_value |= XHCI_USB_CMD_EIE;
        XHCI_HW_WRITE32(xhci, &op_reg->xhci_usb_command, reg_value);

        reg_value = 0;

        /* Enable the interrupter 0 interrupts. */
        XHCI_HW_READ32(xhci, &rt_reg->ir_set[0].xhci_iman, reg_value);
        reg_value = XHCI_IMAN_IRQ_ENABLE(reg_value);
        XHCI_HW_WRITE32(xhci, &rt_reg->ir_set[0].xhci_iman,reg_value);
        XHCI_HW_READ32(xhci, &rt_reg->ir_set[0].xhci_iman, reg_value);
    }
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Start_Controller
*
* DESCRIPTION
*
*       This function starts the host controller.
*
* INPUTS
*
*       xhci           Pointer to xHC driver control block.
*
* OUTPUTS
*
*       Status
**************************************************************************/
STATUS XHCI_Start_Controller(NU_USBH_XHCI *xhci)
{
    /* Progress status .*/
    STATUS                status    = NU_SUCCESS;

    /* Operational registers base address pointer. */
    NU_USBH_XHCI_OP_REGS  *op_reg   = NU_NULL;

    UINT32                reg_value = 0;

    status = XHCI_Get_Operational_Regs_Handle( &xhci->reg_base, &op_reg );

    if ( status == NU_SUCCESS )
    {
        /* Start the Host Controller by asserting RUN/STOP bit of USBCMD
         * register.
         */
        if ( (xhci->state == XHCI_STATE_RESET) || 
             (xhci->state == XHCI_STATE_STOPPED) )
        {
            XHCI_HW_READ32(xhci, &op_reg->xhci_usb_command, reg_value);
            reg_value |= XHCI_USB_CMD_RUN;
            XHCI_HW_WRITE32(xhci, &op_reg->xhci_usb_command, reg_value);
            xhci->state = XHCI_STATE_RUNNING;
            XHCI_HW_READ32(xhci, &op_reg->xhci_usb_command, reg_value);
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Stop_Controller
*
* DESCRIPTION
*
*       This function stops the host controller.
*
* INPUTS
*
*       xhci           Pointer to xHC driver control block.
*
* OUTPUTS
*
*       Status
**************************************************************************/
STATUS XHCI_Stop_Controller(NU_USBH_XHCI *xhci)
{
    /* Progress status .*/
    STATUS                status    = NU_SUCCESS;

    /* Operational registers base address pointer. */
    NU_USBH_XHCI_OP_REGS  *op_reg   = NU_NULL;

    UINT32                reg_value = 0;

    status = XHCI_Get_Operational_Regs_Handle( &xhci->reg_base, &op_reg );

    if ( status == NU_SUCCESS )
    {
        /* Start the Host Controller by asserting RUN/STOP bit of USBCMD
         * register.
         */
        if ( xhci->state ==XHCI_STATE_RUNNING )
        {
            XHCI_HW_READ32(xhci, &op_reg->xhci_usb_command, reg_value);
            reg_value &= ~XHCI_USB_CMD_RUN;
            XHCI_HW_WRITE32(xhci, &op_reg->xhci_usb_command, reg_value);
            xhci->state = XHCI_STATE_STOPPED;
            XHCI_HW_READ32(xhci, &op_reg->xhci_usb_command, reg_value);
        }
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       xhci_transfer_done
*
* DESCRIPTION
*
*       This function is called on completion of transfer if XHCI
*       allocated an Non-cacheable buffer for handling an IRP.
*
* INPUTS
*
*       xhci                                Pointer to the XHCI structure
*                                           containing root-hub data.
*       actual_buffer                       Actual buffer in which data
*                                           is to be copied.
*       non_cache_buffer                    Non-cacheable buffer containing
*                                           data.
*       raw_buffer                          Pointer to original Non-cacheable
*                                           buffer allocated earlier.
*       length                              Length of data to be copied.
*       sync                                Specifies if contents of actual
*                                           buffer and Non-cacheable buffer
*                                           should be synchronized or not.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
**************************************************************************/
STATUS xhci_transfer_done(NU_USBH_XHCI  *xhci,
                        UINT8           *actual_buffer,
                        UINT8           *non_cache_buffer,
                        UINT8           *raw_buffer,
                        UINT32          length,
                        UINT32          alignment,
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
*       xhci_normalize_buffer
*
* DESCRIPTION
*
*       This function converts "buffer" into a cache-coherent, aligned
*       memory buffer.
*
* INPUTS
*
*       xhci                                Pointer to the XHCI structure
*                                           containing root-hub data.
*       actual_buffer                       Actual buffer in which data
*                                           is to be copied.
*       non_cache_buffer                    Non-cacheable buffer containing
*                                           data.
*       raw_buffer                          Pointer to original Non-cacheable
*                                           buffer allocated earlier.
*       length                              Length of data to be copied.
*       sync                                Specifies if contents of actual
*                                           buffer and Non-cacheable buffer
*                                           should be synchronized or not.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed
*                                           successfully.
*
**************************************************************************/
STATUS xhci_normalize_buffer(NU_USBH_XHCI   *xhci,
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
