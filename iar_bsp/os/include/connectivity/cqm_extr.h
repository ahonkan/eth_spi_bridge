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
*       cqm_extr.h
*
* COMPONENT
*
*       CQM - Nucleus CAN Queue Manager
*
* DESCRIPTION
*
*       This file contains function prototypes for Nucleus CAN queue
*       management.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       can.h                               Main definition and API file
*                                           for Nucleus CAN
*
*************************************************************************/
/* Check to see if the file has been included already. */
#ifndef     CQM_EXTR_H
#define     CQM_EXTR_H

#include    "can.h"

/* Perform speed optimization based on the optimization setting. */
#if         (!NU_CAN_OPTIMIZE_FOR_SPEED)

/* Function prototypes for CAN message I/O queue manipulation. */
VOID        CQM_Get_Rx_Queue(CAN_CB *can_cb, CAN_PACKET *can_packet,
                             STATUS *qstatus);
VOID        CQM_Get_Tx_Queue(CAN_CB *can_cb, CAN_PACKET *can_packet,
                             STATUS *qstatus);
VOID        CQM_Put_Rx_Queue(CAN_CB *can_cb, CAN_PACKET *can_packet,
                             STATUS *qstatus);
VOID        CQM_Put_Tx_Queue(CAN_CB *can_cb, CAN_PACKET *can_packet,
                             STATUS *qstatus);

#else

/* Macro for reading a received message from input queue. */

#define     CQM_Get_Rx_Queue(can_cb, can_packet, qstatus)               \
        {                                                               \
            CAN_QUEUE *can_queue = (CAN_QUEUE*)(&(can_cb->can_queue));  \
                                                                        \
            if (can_queue->can_rx_queue.can_qcount != 0)                \
            {                                                           \
                CANS_Copy_Msg(can_queue->can_rx_queue.can_qread,         \
                    can_packet);                           \
                can_queue->can_rx_queue.can_qread++;                    \
                can_queue->can_rx_queue.can_qcount--;                   \
                if (can_queue->can_rx_queue.can_qread >=                \
                    can_queue->can_rx_queue.can_qend)                   \
                {                                                       \
                    can_queue->can_rx_queue.can_qread =                 \
                        can_queue->can_buff_in;                         \
                }                                                       \
                *qstatus = can_queue->can_rx_queue.can_qerror;          \
                can_queue->can_rx_queue.can_qerror = 0;                 \
            }                                                           \
            else                                                        \
            {                                                           \
                *qstatus = CAN_QUEUE_EMPTY;                             \
            }                                                           \
        }

/* Macro for reading a transmit message from output queue. */

#define     CQM_Get_Tx_Queue(can_cb, can_packet, qstatus)               \
        {                                                               \
            CAN_QUEUE *can_queue = (CAN_QUEUE*)(&(can_cb->can_queue));  \
                                                                        \
            if (can_queue->can_tx_queue.can_qcount != 0)                \
            {                                                           \
                CANS_Copy_Msg(can_cb->                                   \
                can_queue.can_tx_queue.can_qread, can_packet);          \
                can_queue->can_tx_queue.can_qread++;                    \
                can_queue->can_tx_queue.can_qcount--;                   \
                if (can_queue->can_tx_queue.can_qread >=                \
                    can_queue->can_tx_queue.can_qend)                   \
                {                                                       \
                    can_queue->can_tx_queue.can_qread =                 \
                        can_queue->can_buff_out;                        \
                }                                                       \
            }                                                           \
            else                                                        \
            {                                                           \
                *qstatus = CAN_QUEUE_EMPTY;                             \
            }                                                           \
        }

/* Macro for writing a received message to input queue. */

#define     CQM_Put_Rx_Queue(can_cb, can_packet, qstatus)               \
        {                                                               \
            CAN_QUEUE *can_queue = (CAN_QUEUE*)(&(can_cb->can_queue));  \
                                                                        \
            if (can_queue->can_rx_queue.can_qcount >=                   \
            can_queue->can_rx_queue.can_qsize)                          \
            {                                                           \
                if (can_queue->can_rx_queue.can_qwrite <=               \
                    can_queue->can_buff_in)                             \
                {                                                       \
                    can_queue->can_rx_queue.can_qwrite =                \
                        can_queue->can_rx_queue.can_qend;               \
                }                                                       \
                CANS_Copy_Msg(can_packet,                                \
                    (can_queue->can_rx_queue.can_qwrite-1));            \
                if (can_queue->can_rx_queue.can_qwrite >=               \
                    can_queue->can_rx_queue.can_qend)                   \
                {                                                       \
                    can_queue->can_rx_queue.can_qwrite =                \
                        can_queue->can_buff_in;                         \
                }                                                       \
                can_queue->can_rx_queue.can_qerror = CAN_SW_OVERRUN;    \
                *qstatus = CAN_SW_OVERRUN;                              \
            }                                                           \
            if (*qstatus != CAN_SW_OVERRUN)                             \
            {                                                           \
                CANS_Copy_Msg(can_packet,                                \
                    can_queue->can_rx_queue.can_qwrite);                \
                can_queue->can_rx_queue.can_qwrite++;                   \
                can_queue->can_rx_queue.can_qcount++;                   \
                if (can_queue->can_rx_queue.can_qwrite >=               \
                    can_queue->can_rx_queue.can_qend)                   \
                {                                                       \
                    can_queue->can_rx_queue.can_qwrite =                \
                        can_queue->can_buff_in;                         \
                }                                                       \
            }                                                           \
        }

/* Macro for writing a transmit message to output queue. */

#define     CQM_Put_Tx_Queue(can_cb, can_packet, qstatus)               \
        {                                                               \
            CAN_QUEUE *can_queue = (CAN_QUEUE*)(&(can_cb->can_queue));  \
                                                                        \
            if (can_queue->can_tx_queue.can_qcount <                    \
                can_queue->can_tx_queue.can_qsize)                      \
            {                                                           \
                CANS_Copy_Msg(can_packet,                                \
                can_queue->can_tx_queue.can_qwrite);                    \
                can_queue->can_tx_queue.can_qwrite++;                   \
                can_queue->can_tx_queue.can_qcount++;                   \
                if (can_queue->can_tx_queue.can_qwrite >=               \
                    can_queue->can_tx_queue.can_qend)                   \
                {                                                       \
                    can_queue->can_tx_queue.can_qwrite =                \
                        can_queue->can_buff_out;                        \
                }                                                       \
            }                                                           \
            else                                                        \
            {                                                           \
                *qstatus = CAN_QUEUE_FULL;                              \
            }                                                           \
        }

#endif      /* NU_CAN_OPTIMIZE_FOR_SPEED */

#endif      /* !CQM_EXTR_H */

