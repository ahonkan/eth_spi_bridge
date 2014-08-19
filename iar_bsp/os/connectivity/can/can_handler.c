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
*       can_handler.c
*
* COMPONENT
*
*       CAN Handler - CAN message handler for Nucleus CAN
*
* DESCRIPTION
*
*       This file contains the routines responsible handling the
*       reception or transmission confirmation of a CAN message.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       CAN_Handler                         Function for activating a
*                                           thread handling the respective
*                                           CAN message event.
*
*       CAN_Tx_Handler_Entry                Thread for handling the
*                                           confirmation of a transmitted
*                                           CAN message.
*
*       CAN_Rx_Handler_Entry                Thread for handling the
*                                           reception of a CAN message.
*
* DEPENDENCIES
*
*       can_extr.h                          Function prototypes for
*                                           Nucleus CAN services.
*
*************************************************************************/
#define     NU_CAN_SOURCE_FILE

#include    "connectivity/can_extr.h"

extern  CAN_CB  *CAN_Devs[NU_CAN_MAX_DEV_COUNT];

/* Entry points for CAN message transmission/reception handling
   routines. */

VOID CAN_Tx_Handler_Entry(VOID);
VOID CAN_Rx_Handler_Entry(VOID);

/*************************************************************************
* FUNCTION
*
*       CAN_Handler
*
* DESCRIPTION
*
*       This function is used to activate a specified CAN handler.
*
* INPUTS
*
*       *can_cb                             Nucleus CAN control block.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID CAN_Handler(CAN_CB *can_cb)
{
    /* Check if the request is for activating transmission
       confirmation handler for data. */
    if (can_cb->can_handler_type & CAN_TX_HANDLER_MASK)
    {
        /* Activate CAN message handler for application. */
        (VOID)NU_Activate_HISR (&CAN_Tx_Handler);
    }

    /* Check if the request is also for activating reception handler
       for data. */
    if (can_cb->can_handler_type & CAN_RX_HANDLER_MASK)
    {
        /* Activate CAN message handler for application. */
        (VOID)NU_Activate_HISR (&CAN_Rx_Handler);
    }
}

/*************************************************************************
* HANDLER
*
*       CAN_Tx_Handler_Entry
*
* DESCRIPTION
*
*       This CAN handler is responsible for handling the confirmation of
*       CAN transmission and reporting them to the user.
*
*************************************************************************/
VOID CAN_Tx_Handler_Entry(VOID)
{
    CAN_CB     *can_cb;
    INT         can_dev = 0;

    /* Loop until all the activated transmission handler requests are
       satisfied. */
    while (can_dev < NU_CAN_MAX_DEV_COUNT)
    {
        /* Get the CAN control block. */
        can_cb = CAN_Devs[can_dev];

        /* Check if the CAN device is initialized. */
        if (can_cb != NU_NULL)
        {
            /* Check if it is the confirmation for data transmitted. */
            if (can_cb->can_handler_type & CAN_DATA_TRANSMITTED)
            {
                /* Call the user call back function to know about the data
                   transmission status. */
                can_cb->can_ucb.can_data_confirm(&can_cb->can_buffer,
                                                  can_cb->can_status);

                /* Reset the handler activation flag. */
                can_cb->can_handler_type &= ((UINT8)~CAN_DATA_TRANSMITTED);
            }

            /* Check if it is the confirmation of RTR transmission. */
            if (can_cb->can_handler_type & CAN_RTR_TRANSMITTED)
            {
                /* Call the user call back function to know about the RTR
                   transmission status. */
                can_cb->can_ucb.can_rtr_confirm(&can_cb->can_buffer,
                                                 can_cb->can_status);

                /* Reset the handler activation flag. */
                can_cb->can_handler_type &= ((UINT8)~CAN_RTR_TRANSMITTED);
            }
        }

        /* Increment the CAN device ID. */
        can_dev++;
    }
}

/*************************************************************************
* HANDLER
*
*       CAN_Rx_Handler_Entry
*
* DESCRIPTION
*
*       This CAN handler is responsible for handling CAN reception
*       indications.
*
*************************************************************************/
VOID CAN_Rx_Handler_Entry(VOID)
{
    CAN_CB     *can_cb;
    UINT8       can_dev = 0;

    /* Loop until all the activated reception handler requests are
       satisfied. */
    while (can_dev < NU_CAN_MAX_DEV_COUNT)
    {
        /* Get the CAN control block. */
        can_cb = CAN_Devs[can_dev];

        /* Check if the CAN device is initialized. */
        if (can_cb != NU_NULL)
        {
            /* Check if it is the indication of data reception. */
            if (can_cb->can_handler_type & CAN_DATA_RECEIVED)
            {

#if         NU_CAN_MULTIPLE_PORTS_SUPPORT

                /* Call the user call back function to handle the data. */
                can_cb->can_ucb.can_data_indication(can_cb->can_port_id,
                                                    can_dev);

#else

                /* Call the user call back function to handle the data. */
                can_cb->can_ucb.can_data_indication(0, can_dev);

#endif      /* NU_CAN_MULTIPLE_PORTS_SUPPORT */

                /* Reset the handler activation flag. */
                can_cb->can_handler_type &= ((UINT8)~CAN_DATA_RECEIVED);
            }

            /* Check if it is the indication of RTR message. */
            if (can_cb->can_handler_type & CAN_RTR_RECEIVED)
            {
                /* Call the user call back function to handle the
                   RTR indication. */
                can_cb->can_ucb.can_rtr_indication(&can_cb->can_buffer);

                /* Reset the handler activation flag. */
                can_cb->can_handler_type &= ((UINT8)~CAN_RTR_RECEIVED);
            }
        }

        /* Increment the CAN device ID. */
        can_dev++;
    }
}

