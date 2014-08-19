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
*       cqm.c
*
* COMPONENT
*
*       CQM - Nucleus CAN Queue Manager
*
* DESCRIPTION
*
*       This file contains the functions used to manage and manipulate
*       Nucleus CAN I/O messages queues.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       CQM_Get_Rx_Queue                    Function for reading an
*                                           element from input queue.
*
*       CQM_Put_Rx_Queue                    Function for writing an
*                                           element to input queue.
*
*       CQM_Put_Tx_Queue                    Function for writing an
*                                           element to output queue.
*
*       CQM_Get_Tx_Queue                    Function for reading an
*                                           element from output queue.
*
* DEPENDENCIES
*
*       can_extr.h                          Function prototypes for
*                                           for Nucleus CAN services.
*
*       cqm_extr.h                          Function prototypes for
*                                           Nucleus CAN queue manager.
*
*************************************************************************/
#define     NU_CAN_SOURCE_FILE

#include    "connectivity/can_extr.h"
#include    "connectivity/cqm_extr.h"

#if         (!NU_CAN_OPTIMIZE_FOR_SPEED)

/*************************************************************************
* FUNCTION
*
*       CQM_Get_Rx_Queue
*
* DESCRIPTION
*
*       This function reads a message from input queue.
*
* INPUTS
*
*      *can_cb                              Nucleus CAN control block.
*
*      *can_packet                          The space where read message
*                                           will be placed.
*
*      *qstatus                             The status returned by the
*                                           routine. See OUTPUTS for
*                                           returned values.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_QUEUE_EMPTY                     Indicates that input queue
*                                           is empty.
*
*       CAN_SW_OVERRUN                      Overrun had occurred in the
*                                           input queue at the last
*                                           message write.
*
*************************************************************************/
VOID    CQM_Get_Rx_Queue(CAN_CB     *can_cb,
                         CAN_PACKET *can_packet,
                         STATUS *qstatus)
{
    /* Check if data is available. */
    if (can_cb->can_queue.can_rx_queue.can_qcount != 0)
    {
        /* Copy received CAN message to user buffer. */
        CANS_Copy_Msg(can_cb->can_queue.can_rx_queue.can_qread,
                     can_packet);

        /* Increment read pointer to point to the next element. */
        can_cb->can_queue.can_rx_queue.can_qread++;

        /* Decrement the counter for received messages in input queue. */
        can_cb->can_queue.can_rx_queue.can_qcount--;

        /* Check for end of queue. */
        if (can_cb->can_queue.can_rx_queue.can_qread >=
            can_cb->can_queue.can_rx_queue.can_qend)
        {
            /* Wrap around the queue read pointer. */
            can_cb->can_queue.can_rx_queue.can_qread =
                can_cb->can_queue.can_buff_in;
        }

        /* Get if a queue error had occurred after last read. */
        *qstatus = can_cb->can_queue.can_rx_queue.can_qerror;

        /* Reset the queue error field. */
        can_cb->can_queue.can_rx_queue.can_qerror = 0;
    }

    else
    {
        /* Set the status to indicate that the queue is empty. */
        *qstatus = CAN_QUEUE_EMPTY;
    }
}

/*************************************************************************
* FUNCTION
*
*       CQM_Put_Rx_Queue
*
* DESCRIPTION
*
*       This function writes a message to input queue.
*
* INPUTS
*
*
*      *can_cb                              Nucleus CAN control block.
*
*      *can_packet                          The message to put into queue.
*
*      *qstatus                             The status returned by the
*                                           routine. See OUTPUTS for
*                                           returned values.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_SW_OVERRUN                      Message corrupted due to
*                                           software overrun.
*
*************************************************************************/
VOID    CQM_Put_Rx_Queue(CAN_CB     *can_cb,
                         CAN_PACKET *can_packet,
                         STATUS *qstatus)
{
    /* Check for buffer full. */
    if (can_cb->can_queue.can_rx_queue.can_qcount >=
        can_cb->can_queue.can_rx_queue.can_qsize)
    {
        /* Yes, overwrite the last entry. */
        if (can_cb->can_queue.can_rx_queue.can_qwrite <=
           can_cb->can_queue.can_buff_in)
        {
            /* Buffer at start boundary. */
            can_cb->can_queue.can_rx_queue.can_qwrite =
                can_cb->can_queue.can_rx_queue.can_qend;
        }

        /* Put the CAN received object in queue. */
        CANS_Copy_Msg(can_packet,
                    (can_cb->can_queue.can_rx_queue.can_qwrite - 1));

        /* Check if queue is at end boundary. */
        if (can_cb->can_queue.can_rx_queue.can_qwrite >=
            can_cb->can_queue.can_rx_queue.can_qend)
        {
            /* Reset the write pointer to queue start. */
            can_cb->can_queue.can_rx_queue.can_qwrite =
                can_cb->can_queue.can_buff_in;
        }

        /* The message received overwrote the last entry in queue. */
        can_cb->can_queue.can_rx_queue.can_qerror = CAN_SW_OVERRUN;

        /* Set the status to indicate a software overrun error. */
        *qstatus = CAN_SW_OVERRUN;
    }

    /* Check if status is O.K. and then proceed with data handling. */
    if (*qstatus != CAN_SW_OVERRUN)
    {
        /* Put the object in the receive queue. */
        CANS_Copy_Msg(can_packet,
                     can_cb->can_queue.can_rx_queue.can_qwrite);

        /* Increment write pointer to point to the next element. */
        can_cb->can_queue.can_rx_queue.can_qwrite++;

        /* Increment the count of the messages in receive queue. */
        can_cb->can_queue.can_rx_queue.can_qcount++;

        /* Check if end of queue has been reached. */
        if (can_cb->can_queue.can_rx_queue.can_qwrite >=
            can_cb->can_queue.can_rx_queue.can_qend)
        {
            /* Wrap around to point to the start of the queue. */
            can_cb->can_queue.can_rx_queue.can_qwrite =
                can_cb->can_queue.can_buff_in;
        }
    }
}

/*************************************************************************
* FUNCTION
*
*       CQM_Put_Tx_Queue
*
* DESCRIPTION
*
*       This function writes a message to output queue.
*
* INPUTS
*
*      *can_cb                              Nucleus CAN control block.
*
*      *can_packet                          Pointer to the packet to be
*                                           read from input queue.
*
*      *qstatus                             The status returned by the
*                                           routine. See OUTPUTS for
*                                           returned values.
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_QUEUE_FULL                      Transmit queue is full.
*
*************************************************************************/
VOID    CQM_Put_Tx_Queue(CAN_CB     *can_cb,
                         CAN_PACKET *can_packet,
                         STATUS *qstatus)
{
    /* Put the message in the output buffer if it is not full. */
    if (can_cb->can_queue.can_tx_queue.can_qcount <
        can_cb->can_queue.can_tx_queue.can_qsize)
    {
        /* Put the message in the output queue. */
        CANS_Copy_Msg(can_packet,
                     can_cb->can_queue.can_tx_queue.can_qwrite);

        /* Increment the write queue pointer to point to the
           next location to write. */
        can_cb->can_queue.can_tx_queue.can_qwrite++;

        /* Increment the count of the messages in transmit queue. */
        can_cb->can_queue.can_tx_queue.can_qcount++;

        /* Check if end of the queue has been reached. */
        if (can_cb->can_queue.can_tx_queue.can_qwrite >=
            can_cb->can_queue.can_tx_queue.can_qend)
        {
            /* Wrap around to point to the start of the queue. */
            can_cb->can_queue.can_tx_queue.can_qwrite =
                can_cb->can_queue.can_buff_out;
        }
    }

    else
    {
        /* Set the status to indicate that transmit queue is full. */
        *qstatus = CAN_QUEUE_FULL;
    }
}

/*************************************************************************
* FUNCTION
*
*       CQM_Get_Tx_Queue
*
* DESCRIPTION
*
*       This function reads a message from output queue.
*
* INPUTS
*
*      *can_cb                              Nucleus CAN control block.
*
*      *can_packet                          Pointer to the packet to be
*                                           read from input queue.
*
*      *qstatus                             The status returned by the
*                                           routine. See OUTPUTS for
*                                           returned values.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_QUEUE_EMPTY                     Transmit queue is empty.
*
*************************************************************************/
VOID    CQM_Get_Tx_Queue(CAN_CB     *can_cb,
                         CAN_PACKET *can_packet,
                         STATUS *qstatus)
{
    /* Get the message from the output buffer if it is not empty. */
    if (can_cb->can_queue.can_tx_queue.can_qcount != 0)
    {
        /* Get the message from the queue for transmission. */
        CANS_Copy_Msg(can_cb->can_queue.can_tx_queue.can_qread,
                     can_packet);

        /* Increment the pointer to read from the next location. */
        can_cb->can_queue.can_tx_queue.can_qread++;

        /* Decrement the counter for transmit queue. */
        can_cb->can_queue.can_tx_queue.can_qcount--;

        /* Check if the end of the buffer has been reached. */
        if (can_cb->can_queue.can_tx_queue.can_qread >=
            can_cb->can_queue.can_tx_queue.can_qend)
        {
            /* Wrap around to the start of queue. */
            can_cb->can_queue.can_tx_queue.can_qread =
                can_cb->can_queue.can_buff_out;
        }
    }

    else
    {
        /* Mark the status to indicate that output buffer is empty. */
        *qstatus = CAN_QUEUE_EMPTY;
    }
}

#endif      /* (!NU_CAN_OPTIMIZE_FOR_SPEED) */

