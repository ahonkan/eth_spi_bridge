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
*       ike_buf.c
*
* COMPONENT
*
*       IKE - Memory Buffers
*
* DESCRIPTION
*
*       This file contains the implementation of the IKE Memory
*       Buffers. IKE services can be requested by multiple clients,
*       so the buffer functions need semaphore protection.
*
*       Note that only a single buffer is used for receiving
*       incoming packets. The buffer list is only for outgoing
*       packets.
*
* DATA STRUCTURES
*
*       IKE_Buffer_List         List of memory buffers.
*       IKE_Receive_Buffer      Buffer for storing incoming
*                               packets.
*
* FUNCTIONS
*
*       IKE_Initialize_Buffers
*       IKE_Allocate_Buffer
*       IKE_Deallocate_Buffer
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_buf.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_buf.h"

/* The global list of memory buffers. */
IKE_MEM_BUFFER *IKE_Buffer_List = NU_NULL;

/* Buffer for storing incoming packets. */
UINT8 *IKE_Receive_Buffer = NU_NULL;

/*************************************************************************
*
* FUNCTION
*
*       IKE_Initialize_Buffers
*
* DESCRIPTION
*
*       This function initializes the Buffer component. It
*       allocates the buffers used for storing packets
*       being encoded before transmission.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*
*************************************************************************/
STATUS IKE_Initialize_Buffers(VOID)
{
    INT             i;
    IKE_MEM_BUFFER  *node;
    IKE_MEM_BUFFER  *prev_node = NU_NULL;
    STATUS          status = NU_SUCCESS;

    /* Log debug message. */
    IKE_DEBUG_LOG("Initializing IKE buffers");

    for(i = 0; i < IKE_MAX_BUFFERS; i++)
    {
        /* Allocate memory for the memory buffer and the list
         * node in a single memory request.
         */
        status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&node,
                                    sizeof(IKE_MEM_BUFFER) +
                                    IKE_MAX_BUFFER_LEN,
                                    NU_NO_SUSPEND);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to allocate IKE buffer",
                           NERR_FATAL, __FILE__, __LINE__);

            /* Abort initialization. */
            break;
        }

        /* Normalize the pointer. */
        node = TLS_Normalize_Ptr(node);

        /* Initialize the buffer pointer of the node. */
        node->ike_buffer = (UINT8*)(node + 1);

        /* Set next pointer of the node to NULL. */
        node->ike_flink = NU_NULL;

        /* If this is not the first node of the list. */
        if(prev_node != NU_NULL)
        {
            /* Update the next pointer of the previous node. */
            prev_node->ike_flink = node;
        }

        else
        {
            /* Set the head of the list. */
            IKE_Buffer_List = node;
        }

        /* Update the previous node pointer. */
        prev_node = node;
    }

    /* If all outgoing buffers allocated successfully. */
    if(status == NU_SUCCESS)
    {
        /* Allocate the incoming packet buffer. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID**)&IKE_Receive_Buffer,
                                    IKE_MAX_RECV_BUFFER_LEN,
                                    NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* Normalize the pointer. */
            IKE_Receive_Buffer = TLS_Normalize_Ptr(IKE_Receive_Buffer);
        }

        else
        {
            NLOG_Error_Log("Failed to allocate IKE receive buffer",
                           NERR_FATAL, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Initialize_Buffers */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Allocate_Buffer
*
* DESCRIPTION
*
*       This function allocates a single memory buffer. Size of
*       the buffer is always IKE_MAX_BUFFER_LEN.
*
*       The caller is responsible for obtaining the IKE
*       semaphore before calling this function.
*
* INPUTS
*
*       **buffer                On return, this contains a pointer
*                               to the allocated memory.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      The parameter is invalid.
*       IKE_NO_MEMORY           Not enough memory.
*
*************************************************************************/
STATUS IKE_Allocate_Buffer(UINT8 **buffer)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameter is valid. */
    if(buffer == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* If there is a buffer available. */
    if(IKE_Buffer_List != NU_NULL)
    {
        /* Set the return pointer to the buffer. */
        *buffer = IKE_Buffer_List->ike_buffer;

        /* Remove the first node from the list. */
        IKE_Buffer_List = IKE_Buffer_List->ike_flink;
    }

    else
    {
        /* Set error status. */
        status = IKE_NO_MEMORY;

        NLOG_Error_Log("IKE buffer allocation failed",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Allocate_Buffer */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Deallocate_Buffer
*
* DESCRIPTION
*
*       This function deallocates a memory buffer previously
*       allocated using IKE_Allocate_Buffer.
*
*       The caller is responsible for obtaining the IKE
*       semaphore before calling this function.
*
* INPUTS
*
*       *buffer                 Pointer to the memory buffer
*                               being deallocated.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      The parameter is invalid.
*
*************************************************************************/
STATUS IKE_Deallocate_Buffer(UINT8 *buffer)
{
    IKE_MEM_BUFFER  *node;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameter is valid. */
    if(buffer == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Get pointer to list node, which is allocated
     * right before the memory buffer.
     */
    node = (IKE_MEM_BUFFER*)buffer;
    node--;

    /* Point frontlink of node to list. */
    node->ike_flink = IKE_Buffer_List;

    /* Add node as first item in list. */
    IKE_Buffer_List = node;

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Deallocate_Buffer */
