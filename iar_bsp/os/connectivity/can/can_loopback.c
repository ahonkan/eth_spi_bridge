/****************************************************************************
*                                                                            
*                  Copyright 2002 Mentor Graphics Corporation       
*                             All Rights Reserved.                           
*                                                                            
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE   
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT    
* TO LICENSE TERMS.                                                          
*                                                                            
****************************************************************************/

/**************************************************************************
*
* FILE NAME                                          
*
*       can_loopback.c                            
*
* COMPONENT
*
*       CAN Loopback - Loopback device for Nucleus CAN
*
* DESCRIPTION
*
*       This file contains the functions used to manage and manipulate
*       loopback device for Nucleus CAN.
*
* DATA STRUCTURES
*
*       CAN_Loopback_HW_Buffers             An array of message buffers
*                                           provided by loopback device.
*
*       CAN_Loopback_IF_Reg                 Interrupt flag register for
*                                           loopback device.
*
* FUNCTIONS
*
*       CAN_Loopback_Initialize             CAN loopback device
*                                           initialization function.
*
*       CAN_Loopback_Write_Driver           Packet transmission routine.
*
*       CAN_Loopback_Assign_RTR_Buf         RTR response assignment
*                                           service.
*
*       CAN_Loopback_Release_RTR_Buf        Routine to clear a previously
*                                           assigned RTR response.
*
*       CAN_Loopback_Set_BaudRate           Service for setting baud rate.
*
*       CAN_Loopback_Shutdown               Service for shutting down
*                                           loopback device.
*
*       CAN_Loopback_Sleep                  Service for making the
*                                           loopback device sleep.
*
*       CAN_Loopback_Wakeup                 Service for waking up the
*                                           loopback device.
*
*       CAN_Loopback_Set_Accept_Mask        Service for setting acceptance
*                                           mask for Rx. messages.
*
*       CAN_Loopback_ISR                    Interrupt service routine for
*                                           loopback device.
*
* DEPENDENCIES
*
*       can_extr.h                          Function prototypes for
*                                           Nucleus CAN services.
*
*       can_loopback.h                      Function prototypes for
*                                           Nucleus CAN loopback device.
*
*************************************************************************/
#define     NU_CAN_SOURCE_FILE

#include    "connectivity/can_extr.h"
#include    "connectivity/can_loopback.h"

#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)

/* Flag to implement sleep and wakeup services for CAN loopback device. */
static  INT                 CAN_Sleeping;

/* Hardware message buffer pool for Nucleus CAN loopback device. */
static  CAN_LOOPBACK_HWMB   CAN_Loopback_HW_Buffers[CAN_LOOPBACK_HWMB_COUNT];

/* Loopback device interrupt flag register. */
static  UINT16              CAN_Loopback_IF_Reg;

/* Function prototypes for CAN loopback device services. */
STATUS  CAN_Loopback_Write_Driver       (UINT8 can_dev);
STATUS  CAN_Loopback_Set_BaudRate       (UINT16 baud_rate,
                                                 UINT8 can_dev);
STATUS  CAN_Loopback_Shutdown           (UINT8 can_dev);
static  VOID        CAN_Loopback_ISR            (VOID);
STATUS  CAN_Loopback_Sleep              (UINT8 can_dev);
STATUS  CAN_Loopback_Wakeup             (UINT8 can_dev);
STATUS  CAN_Loopback_Set_Accept_Mask    (UINT8 can_dev,
                                                 UINT8 buffer_no,
                                                 UINT32 mask_value);

#if         (NU_CAN_AUTOMATIC_RTR_RESPONSE && NU_CAN_SUPPORTS_RTR)

STATUS  CAN_Loopback_Assign_RTR_Buf     (CAN_PACKET *can_msg);
STATUS  CAN_Loopback_Release_RTR_Buf    (CAN_PACKET *can_msg);

#endif      /* (NU_CAN_AUTOMATIC_RTR_RESPONSE && NU_CAN_SUPPORTS_RTR) */

STATUS          CAN_Loopback_Initialize(VOID);

extern  CAN_CB     *CAN_Devs[NU_CAN_MAX_DEV_COUNT];

/*************************************************************************
* FUNCTION
*
*       CAN_Loopback_Initialize
*
* DESCRIPTION
*
*       This function initializes the CAN module.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS CAN_Loopback_Initialize(VOID)
{
    CAN_CB         *can_cb;

    /* Get control block of the specified loopback device. */
    can_cb = CAN_Devs[CAN_LOOPBACK_DEVICE];

    /* Set CAN node state to be in Loopback mode. */
    can_cb->can_state                        = CAN_IN_LOOPBACK;

    /* Reset the loopback hardware message buffers
       interrupt flag register. */
    CAN_Loopback_IF_Reg = 0;

    /* Return completion status of the service. */
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       CAN_Loopback_Write_Driver
*
* DESCRIPTION
*
*       This function is responsible for message transmission.
*
* INPUTS
*
*       can_dev                             CAN device number. Unused for
*                                           loopback device.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_NO_FREE_MB                      No free message buffer is
*                                           available for writing.
*
*       CAN_LOOPBACK_NOT_INIT               Loopback device is not
*                                           initialized.
*
*************************************************************************/
STATUS  CAN_Loopback_Write_Driver   (UINT8 can_dev)
{
    CAN_CB             *can_cb;
    CAN_LOOPBACK_HWMB  *hwmb;
    CAN_PACKET          can_packet;
    STATUS              status = NU_SUCCESS;
    INT                 mb_no;

    /* Handle unused parameter. */
    NU_UNUSED_PARAM(can_dev);

    /* Get the control block for the loopback CAN device. */
    can_cb = CAN_Devs[CAN_LOOPBACK_DEVICE];

    /* Check if the control block is valid. */
    if (can_cb != NU_NULL)
    {
        /* Check if the loopback device is not in sleep mode. */
        if (CAN_LOOPBACK_GET_MODE() != CAN_LOOPBACK_IN_SLEEP_MODE)
        {
            /* Look for a free buffer in which message will be written.  */
            for (mb_no = 0; mb_no < CAN_LOOPBACK_HWMB_COUNT; mb_no++)
            {
                /* Check if the message buffer is free. */
                if (!(CAN_Loopback_HW_Buffers[mb_no].can_loopback_flags &
                    CAN_LOOPBACK_HWMB_IN_USE))
                {
                    /* Terminate the search as a free buffer is found. */
                    break;
                }
            }

            /* Check if a free message buffer is found. */
            if (mb_no < CAN_LOOPBACK_HWMB_COUNT)
            {
                /* Get data from transmit queue and copy to the buffer. */
                CQM_Get_Tx_Queue(can_cb, &can_packet, &status);

                /* Check if message is retrieved
                   successfully from queue. */
                if (status == NU_SUCCESS)
                {
                    /* Get the address of the specified buffer. */
                    hwmb = &CAN_Loopback_HW_Buffers[mb_no];

                    /* Set the message buffer flag to 'in-use'. */
                    hwmb->can_loopback_flags |= CAN_LOOPBACK_HWMB_IN_USE;

                    /* Write the message in loopback message buffer. */
                    CANS_Copy_Msg(&can_packet, &hwmb->can_loopback_packet);

                    /* Set the relevant interrupt type based upon
                    message type. */

                    if (can_packet.can_msg_type == CAN_DATA_MSG)
                    {
                        hwmb->can_loopback_ints |=
                        CAN_LOOPBACK_DATA_TX_OK_INT;
                    }

                    else
                    {
                        hwmb->can_loopback_ints |=
                        CAN_LOOPBACK_RTR_TX_OK_INT;
                    }

                    /* Set the interrupt for the message buffer. */
                    CAN_LOOPBACK_SET_INT(mb_no);
                }
            }

            else
            {
                /* Set the status to indicate that no free message buffer
                is available. */
                status = CAN_NO_FREE_MB;
            }

            /* Interrupt loopback device to receive the message. */
            CAN_Loopback_ISR();
        }

        else
        {
            /* Set the status to indicate the the loopback device is in
               sleep mode. */
            status = CAN_DEV_SLEEPING;
        }
    }

    else
    {
        /* Set the status to indicate that the loopback
           device is not initialized. */
        status = CAN_LOOPBACK_NOT_INIT;
    }

    /* Return the completion status of the service. */
    return (status);
}

#if         (NU_CAN_AUTOMATIC_RTR_RESPONSE && NU_CAN_SUPPORTS_RTR)

/*************************************************************************
* FUNCTION
*
*       CAN_Loopback_Assign_RTR_Buf
*
* DESCRIPTION
*
*       This function is responsible for assigning a loopback hardware
*       message buffer to respond to an incoming matching RTR.
*
* INPUTS
*
*      *can_msg                             Response message pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_NO_FREE_MB                      No free message buffer is
*                                           available.
*
*       CAN_LOOPBACK_NOT_INIT               Loopback device is not
*                                           initialized.
*
*************************************************************************/
STATUS  CAN_Loopback_Assign_RTR_Buf (CAN_PACKET *can_msg)
{
    CAN_CB             *can_cb;
    STATUS              status = NU_SUCCESS;
    INT                 mb_no;

    /* Get the control block for the loopback CAN device. */
    can_cb = CAN_Devs[CAN_LOOPBACK_DEVICE];

    /* Check if the control block is valid. */
    if (can_cb != NU_NULL)
    {
        /* Check if the ID has been assigned already. */
        for (mb_no = 0; mb_no < CAN_LOOPBACK_HWMB_COUNT; mb_no++)
        {
            /* Check if the message buffer is free. */
            if (CAN_Loopback_HW_Buffers[mb_no].can_loopback_packet.can_msg_id ==
                can_msg->can_msg_id)
            {
                /* Set the status to indicate that an RTR response
                    message with the same ID is already assigned. */
                status = CAN_RTR_ALREADY_ASSIGNED;

                /* Terminate the search as a free message
                buffer is found. */
                break;
            }
        }

        /* Check if everything is O.K. till this point. */
        if (status == NU_SUCCESS)
        {
            /* Look for a free buffer in which message will be written. */
            for (mb_no = 0; mb_no < CAN_LOOPBACK_HWMB_COUNT; mb_no++)
            {
                /* Check if the message buffer is free. */
                if (!(CAN_Loopback_HW_Buffers[mb_no].can_loopback_flags &
                    CAN_LOOPBACK_HWMB_IN_USE))
                {
                    /* Terminate the search as a free message
                    buffer is found. */
                    break;
                }
            }

            /* Check if a free message buffer is found. */
            if (mb_no < CAN_LOOPBACK_HWMB_COUNT)
            {
                /* Set the message buffer as 'assigned for RTR'
                and 'in-use'. */
                CAN_Loopback_HW_Buffers[mb_no].can_loopback_flags |=
                    (CAN_LOOPBACK_HWMB_RTR | CAN_LOOPBACK_HWMB_IN_USE);

                /* Copy the message to the loopback message buffer. */
                CANS_Copy_Msg(can_msg,
                    &CAN_Loopback_HW_Buffers[mb_no].can_loopback_packet);
            }

            else
            {
                /* Set the status to indicate that no free message
                buffer is available. */
                status = CAN_NO_FREE_MB;
            }
        }
    }

    else
    {
        /* Set the status to indicate that the loopback
           device is not initialized. */
        status = CAN_LOOPBACK_NOT_INIT;
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       CAN_Loopback_Release_RTR_Buf
*
* DESCRIPTION
*
*       This function can be used to clear a message buffer that was
*       previously assigned for RTR response.
*
* INPUTS
*
*      *can_msg                             Response message pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_NO_ASSIGNED_MB                  No buffer has been assigned
*                                           the specified RTR response.
*
*       CAN_LOOPBACK_NOT_INIT               Loopback device is not
*                                           initialized.
*
*************************************************************************/
STATUS  CAN_Loopback_Release_RTR_Buf(CAN_PACKET *can_msg)
{
    CAN_CB             *can_cb;
    STATUS              status  = NU_SUCCESS;
    INT                 mb_no;

    /* Get the control block for the loopback CAN device. */
    can_cb = CAN_Devs[CAN_LOOPBACK_DEVICE];

    /* Check if the control block is valid. */
    if (can_cb != NU_NULL)
    {
        /* Look for a buffer with a matching ID response. */
        for (mb_no = 0; mb_no < CAN_LOOPBACK_HWMB_COUNT; mb_no++)
        {
            /* Check if the buffer is assigned for RTR response with
            the specified ID. */
            if ((CAN_Loopback_HW_Buffers[mb_no].can_loopback_flags &
                CAN_LOOPBACK_HWMB_RTR) &&
                (CAN_Loopback_HW_Buffers[mb_no].can_loopback_packet
                .can_msg_id == can_msg->can_msg_id))
            {
                /* Terminate the search as the required loopback hardware
                buffer is found. */
                break;
            }
        }

        /* Check if the required message buffer is found. */
        if (mb_no < CAN_LOOPBACK_HWMB_COUNT)
        {
            /* Reset the ID of the message buffer. */
            CAN_Loopback_HW_Buffers[mb_no].can_loopback_packet.can_msg_id = 0;

            /* Clear the message buffer from RTR response. */
            CAN_Loopback_HW_Buffers[mb_no].can_loopback_flags &=
                ((UINT16)~(CAN_LOOPBACK_HWMB_RTR |
                CAN_LOOPBACK_HWMB_IN_USE));
        }

        else
        {
            /* Set the status to indicate that no RTR buffer has been
            assigned. */
            status = CAN_NO_ASSIGNED_MB;
        }
    }

    else
    {
        /* Set the status to indicate that the loopback
           device is not initialized. */
        status = CAN_LOOPBACK_NOT_INIT;
    }

    /* Return the completion status of the service. */
    return (status);
}

#endif      /* (NU_CAN_AUTOMATIC_RTR_RESPONSE && NU_CAN_SUPPORTS_RTR) */

/*************************************************************************
* FUNCTION
*
*       CAN_Loopback_Set_BaudRate
*
* DESCRIPTION
*
*       This function is used to set the baud rate for loopback device.
*       This is actually a dummy function just to fulfill the
*       requirements of driver services block of Nucleus CAN.
*
* INPUTS
*
*       baud_rate                           Baud rate to set.
*
*       can_dev                             CAN device number. Unused for
*                                           loopback device.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_LOOPBACK_NOT_INIT               Loopback device is not
*                                           initialized.
*
*************************************************************************/
STATUS  CAN_Loopback_Set_BaudRate   (UINT16 baud_rate,
                                                 UINT8 can_dev)
{
    CAN_CB         *can_cb;
    STATUS          status = NU_SUCCESS;

    /* Get the control block for the loopback CAN device. */
    can_cb = CAN_Devs[CAN_LOOPBACK_DEVICE];

    /* Check if the control block is valid. */
    if (can_cb != NU_NULL)
    {
        /* Check if the loopback device is not in sleep mode. */
        if (CAN_LOOPBACK_GET_MODE() != CAN_LOOPBACK_IN_SLEEP_MODE)
        {
            /* Set the baud rate. */
            can_cb->can_baud = baud_rate;
        }

        else
        {
            /* Set the status to indicate the the loopback device is in
               sleep mode. */
            status = CAN_DEV_SLEEPING;
        }
    }

    else
    {
        /* Set the status to indicate that the loopback
        device is not initialized. */
        status = CAN_LOOPBACK_NOT_INIT;
    }

    /* Handle unused parameter. */
    NU_UNUSED_PARAM(can_dev);

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       CAN_Loopback_Shutdown
*
* DESCRIPTION
*
*       This function is responsible for stopping all the functionality of
*       the loopback device and disabling it.
*
* INPUTS
*
*       can_dev                             CAN device number. Unused for
*                                           loopback device
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS  CAN_Loopback_Shutdown       (UINT8 can_dev)
{
    INT             mem_size;
    UNSIGNED_INT   *int_ptr;

    /* Mark the pointer as unused. */
    NU_UNUSED_PARAM(can_dev);

    /* Get the size of the memory used for loopback hardware buffers. */
    mem_size = sizeof(CAN_Loopback_HW_Buffers);

    mem_size /= sizeof(UNSIGNED_INT);

    int_ptr = (UNSIGNED_INT *)&CAN_Loopback_HW_Buffers[0];

    /* Clear the loopback hardware buffers. */
    while(mem_size--)
    {
        *int_ptr++ = 0;
    }

    /* Reset the CAN control block. */
    CAN_Devs[CAN_LOOPBACK_DEVICE] = NU_NULL;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       CAN_Loopback_Sleep
*
* DESCRIPTION
*
*       This function makes the loopback device go to sleep mode.
*
* INPUTS
*
*       can_dev                             CAN device number. Unused for
*                                           loopback device
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS  CAN_Loopback_Sleep          (UINT8 can_dev)
{
    /* Mark the pointer as unused. */
    NU_UNUSED_PARAM(can_dev);

    /* Set loopback device to go to sleep mode. */
    CAN_Sleeping = CAN_LOOPBACK_IN_SLEEP_MODE;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       CAN_Loopback_Wakeup
*
* DESCRIPTION
*
*       This function makes the loopback device go to sleep mode.
*
* INPUTS
*
*       can_dev                             CAN device number. Unused for
*                                           loopback device
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*************************************************************************/
STATUS  CAN_Loopback_Wakeup         (UINT8 can_dev)
{
    /* Mark the pointer as unused. */
    NU_UNUSED_PARAM(can_dev);

    /* Set loopback device to go to sleep mode. */
    CAN_Sleeping = CAN_LOOPBACK_IS_AWAKE;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       CAN_Loopback_Set_Accept_Mask
*
* DESCRIPTION
*
*       This function sets the acceptance mask for receiving the messages.
*
* INPUTS
*
*       can_dev                             Nucleus CAN device ID.
*
*       buffer_no                           Mask buffer number.
*
*       mask_value                          Mask value.
*
* OUTPUTS
*
*       CAN_SERVICE_NOT_SUPPORTED           The service is not supported
*                                           on this target.
*
*************************************************************************/
STATUS  CAN_Loopback_Set_Accept_Mask(UINT8 can_dev,
                                             UINT8 buffer_no,
                                             UINT32 mask_value)
{
    NU_UNUSED_PARAM(can_dev);
    NU_UNUSED_PARAM(buffer_no);
    NU_UNUSED_PARAM(mask_value);

    return (CAN_SERVICE_NOT_SUPPORTED);
}


/*************************************************************************
* FUNCTION
*
*       CAN_Loopback_ISR
*
* DESCRIPTION
*
*       This function is responsible for message reception from hardware
*       message buffers of loopback device.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID CAN_Loopback_ISR(VOID)
{
    CAN_CB              *can_cb;
    CAN_LOOPBACK_HWMB   *hwmb;
    CAN_PACKET           can_msg;
    INT                  mb_no;
    STATUS               status = NU_SUCCESS;

    /* Service all the pending interrupts. */
    while (CAN_Loopback_IF_Reg)
    {
        /* Check which message buffer has caused an interrupt. */
        for (mb_no = 0; mb_no < CAN_LOOPBACK_HWMB_COUNT; mb_no++)
        {
            /* Check if this message buffer has caused an interrupt. */
            if (CAN_LOOPBACK_CHECK_INT(mb_no))
            {
                /* Terminate the search on the first message buffer
                   causing interrupt. */
                break;
            }
        }

        /* Check if a valid loopback hardware message buffer has
           caused an interrupt. */
        if (mb_no < CAN_LOOPBACK_HWMB_COUNT)
        {
            /* Get the device control block for the relevant device. */
            can_cb = CAN_Devs[CAN_LOOPBACK_DEVICE];

            /* Get the specified message buffer address. */
            hwmb = &CAN_Loopback_HW_Buffers[mb_no];

            /* Clear the message buffer interrupt. */
            CAN_LOOPBACK_CLEAR_INT(mb_no);

            /* Check if it a transmit/receive error interrupt. */
            if ((hwmb->can_loopback_ints & CAN_LOOPBACK_TX_ERR_INT) ||
                (hwmb->can_loopback_ints & CAN_LOOPBACK_RX_ERR_INT) )
            {
                /* Reset the interrupt type. */
                hwmb->can_loopback_ints &= ((UINT16)~(
                                            CAN_LOOPBACK_TX_ERR_INT |
                                            CAN_LOOPBACK_RX_ERR_INT));

                can_cb->can_ucb.can_error((CAN_LOOPBACK_TX_ERROR |
                CAN_LOOPBACK_RX_ERROR), 0, CAN_LOOPBACK_DEVICE);
            }

            /* Check if it is a data transmit completion interrupt. */
            if (hwmb->can_loopback_ints & CAN_LOOPBACK_DATA_TX_OK_INT)
            {
                /* Get the message from the loopback device. */
                CANS_Copy_Msg(&hwmb->can_loopback_packet,
                             &can_cb->can_buffer);

                /* Set the type of the confirmed message. */
                can_cb->can_buffer.can_msg_type = CAN_DATA_MSG;

                /* Reset the interrupt type. */
                hwmb->can_loopback_ints &=
                                   ((UINT16)~CAN_LOOPBACK_DATA_TX_OK_INT);

                /* Set the type of operation to execute by the
                   CAN handler. */
                can_cb->can_handler_type |= CAN_DATA_TRANSMITTED;

                /* Request the CAN handler to handle the message. */
                CAN_Handler(can_cb);

                /* Now set the reception interrupt to indicate that
                   a message has been received. */
                hwmb->can_loopback_ints |= CAN_LOOPBACK_DATA_RX_INT;
            }

#if         NU_CAN_SUPPORTS_RTR

            /* Check if it is an RTR transmit completion interrupt. */
            if (hwmb->can_loopback_ints & CAN_LOOPBACK_RTR_TX_OK_INT)
            {
                /* Get the message from the loopback device. */
                CANS_Copy_Msg(&hwmb->can_loopback_packet,
                    &can_cb->can_buffer);

                /* Set the message type confirmed. */
                can_cb->can_buffer.can_msg_type = CAN_RTR_MSG;

                /* Reset the interrupt type. */
                hwmb->can_loopback_ints &=
                                     ((UINT16)~CAN_LOOPBACK_RTR_TX_OK_INT);

                /* Set the type of operation to execute by the
                CAN handler. */
                can_cb->can_handler_type |= CAN_RTR_TRANSMITTED;

                /* Request the CAN handler to handle the message. */
                CAN_Handler(can_cb);

                /* Now set the reception interrupt to indicate that
                   a message has been received. */
                hwmb->can_loopback_ints |= CAN_LOOPBACK_RTR_RX_INT;
            }

#endif      /* NU_CAN_SUPPORTS_RTR */

            /* Check if it is a data reception interrupt. */
            if (hwmb->can_loopback_ints & CAN_LOOPBACK_DATA_RX_INT)
            {
                /* Get the message from the loopback device. */
                CANS_Copy_Msg(&hwmb->can_loopback_packet, &can_msg);

                /* Set the message type received. */
                can_msg.can_msg_type = CAN_DATA_MSG;

                /* Reset the interrupt type. */
                hwmb->can_loopback_ints &=
                                      ((UINT16)~CAN_LOOPBACK_DATA_RX_INT);

                /* Free the message buffer. */
                hwmb->can_loopback_flags &=
                                      ((UINT16)~CAN_LOOPBACK_HWMB_IN_USE);

                /* Put the received message in receive queue. */
                CQM_Put_Rx_Queue(can_cb, &can_msg, &status);

                /* Set the type of operation to execute by the
                   CAN handler. */
                can_cb->can_handler_type |= CAN_DATA_RECEIVED;

                /* Request the CAN handler to handle the message. */
                CAN_Handler(can_cb);
            }

            /* Check if it is an RTR reception interrupt. */
            if (hwmb->can_loopback_ints & CAN_LOOPBACK_RTR_RX_INT)
            {
                /* Get the message from the loopback device. */
                CANS_Copy_Msg(&hwmb->can_loopback_packet,
                    &can_cb->can_buffer);

                /* Set the message type received. */
                can_cb->can_buffer.can_msg_type =
                    CAN_RTR_MSG;

                /* Reset the interrupt type. */
                hwmb->can_loopback_ints &=
                                       ((UINT16)~CAN_LOOPBACK_RTR_RX_INT);

                /* Free the message buffer. */
                hwmb->can_loopback_flags &=
                                      ((UINT16)~CAN_LOOPBACK_HWMB_IN_USE);

                /* Set the type of operation to execute by the
                   CAN handler. */
                can_cb->can_handler_type |= CAN_RTR_RECEIVED;

                /* Request the CAN handler to handle the message. */
                CAN_Handler(can_cb);

                /* The following segment simulates the automatic
                   transmission of the RTR response from a remote node. */

#if         (NU_CAN_AUTOMATIC_RTR_RESPONSE)

                /* Search for the RTR response message. */
                for (mb_no = 0; mb_no < CAN_LOOPBACK_HWMB_COUNT; mb_no++)
                {
                    /* Check if the device has a response message with
                       matching ID. */
                    if ((CAN_Loopback_HW_Buffers[mb_no].can_loopback_flags
                        & CAN_LOOPBACK_HWMB_RTR) &&
                        (CAN_Loopback_HW_Buffers[mb_no].can_loopback_packet
                                                       .can_msg_id ==
                        hwmb->can_loopback_packet.can_msg_id))
                    {
                        /* Set the buffer pointer to new buffer. */
                        hwmb = &CAN_Loopback_HW_Buffers[mb_no];

                        /* Finish the search as the required message is
                           found. */
                        break;
                    }
                }

                /* Check if a valid RTR response message exists. */
                if (mb_no < CAN_LOOPBACK_HWMB_COUNT)
                {

                    /* Transmit the response of the RTR requests.
                       Since the responding device is also the same
                       loopback device so, the message already exists in
                       loopback hardware buffer.
                       Indicate that the message has been transmitted. */

                    /* Get the message from the loopback device. */
                    CANS_Copy_Msg(&hwmb->can_loopback_packet,
                        &can_cb->can_buffer);

                    /* Set the message type confirmed. */
                    can_cb->can_buffer.can_msg_type = CAN_DATA_MSG;

                    /* Set the type of operation to execute by the
                       CAN handler. */
                    can_cb->can_handler_type |= CAN_DATA_TRANSMITTED;

                    /* Request the CAN handler to handle the message. */
                    CAN_Handler(can_cb);

                    /* *** Now provide the response data to the
                          application. *** */

                    /* Get the message from the loopback device. */
                    CANS_Copy_Msg(&hwmb->can_loopback_packet, &can_msg);

                    /* Set the message type received. */
                    can_msg.can_msg_type = CAN_DATA_MSG;

                    /* Put the received message in receive queue. */
                    CQM_Put_Rx_Queue(can_cb, &can_msg, &status);

                    /* Set the type of operation to execute by the
                       CAN handler. */
                    can_cb->can_handler_type |= CAN_DATA_RECEIVED;

                    /* Request the CAN handler to handle the message. */
                    CAN_Handler(can_cb);
                }

#endif      /* NU_CAN_AUTOMATIC_RTR_RESPONSE */

            }
        }
    }
}

#endif      /* (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK) */

