/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
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
*       ike_buf_dini.c
*
* COMPONENT
*
*       IKE - Memory Buffers
*
* DESCRIPTION
*
*       This file contains the implementation of the IKE Memory
*       Buffers de-initialization function.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Deinitialize_Buffers
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_cfg.h
*       ike.h
*       ike_buf.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_buf.h"

/*************************************************************************
*
* FUNCTION
*
*       IKE_Deinitialize_Buffers
*
* DESCRIPTION
*
*       This function de-initializes the Buffer component. It
*       deallocates all buffers used for storing packets. Note
*       that if any buffers are allocated from this component,
*       then they would be stored in the Phase 1 and 2 Handles
*       instead of the buffer list maintained here. Therefore
*       the Database component MUST be de-initialized before
*       calling this function.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*
*************************************************************************/
STATUS IKE_Deinitialize_Buffers(VOID)
{
    IKE_MEM_BUFFER  *node;
    IKE_MEM_BUFFER  *next_node;
    INT             buffer_count = 0;

    /* Log debug message. */
    IKE_DEBUG_LOG("De-initializing IKE buffers");

    /* If the receive buffer is allocated. */
    if(IKE_Receive_Buffer != NU_NULL)
    {
        /* Deallocate the receive buffer. */
        if(NU_Deallocate_Memory(IKE_Receive_Buffer) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate IKE memory",
                        NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set receive buffer pointer to NULL. */
        IKE_Receive_Buffer = NU_NULL;
    }

    /* Initialize node to head of buffer list. */
    node = IKE_Buffer_List;

    /* Loop for all nodes in the list. */
    while(node != NU_NULL)
    {
        /* Save next node pointer. */
        next_node = node->ike_flink;

        /* Deallocate memory for the current node. This block
         * of memory also includes the buffer itself.
         */
        if(NU_Deallocate_Memory(node) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate IKE memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Increment buffer count. */
        buffer_count++;

        /* Move to the next node. */
        node = next_node;
    }

    /* Set the head of the list to NULL. */
    IKE_Buffer_List = NU_NULL;

    /* If buffer count is not equal to the total buffers
     * allocated initially.
     */
    if(buffer_count != IKE_MAX_BUFFERS)
    {
        /* There has been a memory leak. */
        NLOG_Error_Log("Memory leak detected during buffer deallocation",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Deinitialize_Buffers */
