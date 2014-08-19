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
*       can_osal.c
*
* COMPONENT
*
*       CAN OSAL - OS Abstraction Layer for Nucleus CAN
*
* DESCRIPTION
*
*       This file contains the functions used to abstract OS functionality
*       used by Nucleus CAN.
*
* DATA STRUCTURES
*
*       CAN_Tx_Handler                      Data structures for CAN
*                                           transmission handler.
*
*       CAN_Rx_Handler                      Data structure for CAN
*                                           reception handler.
*
* FUNCTIONS
*
*       CAN_OSAL_Allocate_Resources         Function to allocate resources
*                                           for Nucleus CAN.
*
*       CAN_OSAL_Deallocate_Resources       Function to deallocate
*                                           resources assigned to
*                                           Nucleus CAN.
*
* DEPENDENCIES
*
*       can_extr.h                          Function prototypes for
*                                           Nucleus CAN services.
*
*************************************************************************/
#define     NU_CAN_SOURCE_FILE

#include    "connectivity/can_extr.h"

/* Thread environment handlers for Nucleus CAN. */
NU_HISR   CAN_Tx_Handler;
NU_HISR   CAN_Rx_Handler;

/* Extern the entry points for CAN transmission/reception handlers. */
extern      VOID CAN_Tx_Handler_Entry(VOID);
extern      VOID CAN_Rx_Handler_Entry(VOID);

/*************************************************************************
* FUNCTION
*
*       CAN_OSAL_Allocate_Resources
*
* DESCRIPTION
*
*       This function is responsible for allocating the OS & system
*       resources needed by Nucleus CAN.
*
* INPUTS
*
*      *can_cb                              Nucleus CAN control block.
*
*      *mem                                 Pointer to the memory pool
*                                           available to Nucleus CAN.
*
* OUTPUTS
*
*       NU_SUCCESS                          Initialization Service
*                                           completed successfully.
*
*       Error returned by call to Nucleus API.
*
*************************************************************************/
STATUS  CAN_OSAL_Allocate_Resources  (CAN_CB         *can_cb,
                                      NU_MEMORY_POOL *mem)
{
    STATUS  status;

    /* Allocate memory for the Nucleus CAN output queue.  */
    status = NU_Allocate_Memory(mem,
        (VOID **)&can_cb->can_queue.can_buff_out,
        sizeof(CAN_PACKET)*(can_cb->can_queue.can_tx_queue.can_qsize),
        NU_NO_SUSPEND);

    /* Proceed if memory allocation was successful. */
    if (status == NU_SUCCESS)
    {
        /* Allocate memory for the Nucleus CAN input queue.  */
        status = NU_Allocate_Memory(mem,
            (VOID**)&can_cb->can_queue.can_buff_in,
            sizeof(CAN_PACKET)*(can_cb->can_queue.can_rx_queue.can_qsize),
            NU_NO_SUSPEND);
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       CAN_OSAL_Deallocate_Resources
*
* DESCRIPTION
*
*       This function de-allocates the resources assigned to Nucleus CAN.
*
* INPUTS
*
*      *can_cb                              Nucleus CAN control block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Initialization Service
*                                           completed successfully.
*
*       Error returned by call to Nucleus API.

*************************************************************************/
STATUS  CAN_OSAL_Deallocate_Resources(CAN_CB            *can_cb)
{
    STATUS  status;

    /* Deallocate the memory allocated to input queue. */
    status = NU_Deallocate_Memory((VOID *)can_cb->can_queue.can_buff_in);

    /* Check if the previous service completed successfully. */
    if (status == NU_SUCCESS)
    {
        /* Deallocate the memory allocated to output queue. */
        status = NU_Deallocate_Memory(
                (VOID *)can_cb->can_queue.can_buff_out);
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       CAN_OSAL_Create_Handler
*
* DESCRIPTION
*
*       This function is responsible for allocating the OS & system
*       resources needed by Nucleus CAN.
*
* INPUTS
*
*      *mem                                 Pointer to the memory pool
*                                           available to Nucleus CAN.
*
* OUTPUTS
*
*       NU_SUCCESS                          Initialization Service
*                                           completed successfully.
*
*       Error returned by call to Nucleus API.

*************************************************************************/
STATUS  CAN_OSAL_Create_Handler  (NU_MEMORY_POOL *mem)
{
    STATUS  status;
    VOID *next_avail_mem = NU_NULL;

    /* Allocate memory for CAN transmission handler. */
    status = NU_Allocate_Memory(mem, &next_avail_mem,
             CAN_HANDLER_MEM_STACK, NU_NO_SUSPEND);

    /* Check if memory has been allocated successfully. */
    if (status == NU_SUCCESS)
    {
        /* Create HISR for call back services. */
        status = NU_Create_HISR (&CAN_Tx_Handler, "CANTxH",
                 CAN_Tx_Handler_Entry, CAN_HANDLER_PRIORITY,
                 next_avail_mem, CAN_HANDLER_MEM_STACK);
    }

    /* Check if memory has been allocated successfully. */
    if (status == NU_SUCCESS)
    {
        /* Create HISR for call back services. */
        status = NU_Create_HISR (&CAN_Rx_Handler, "CANRxH",
                CAN_Rx_Handler_Entry, CAN_HANDLER_PRIORITY,
                next_avail_mem, CAN_HANDLER_MEM_STACK);
    }

    /* Return the completion status of the service. */
    return (status);
}

