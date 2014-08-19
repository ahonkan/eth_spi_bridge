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
*       ike_buf.h
*
* COMPONENT
*
*       IKE - Memory Buffers
*
* DESCRIPTION
*
*       This file contains constants, data structure and function
*       prototypes needed to implement IKE Memory Buffers.
*
* DATA STRUCTURES
*
*       IKE_MEM_BUFFER
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IKE_BUF_H
#define IKE_BUF_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** Buffer constants. ****/

/* Size of each memory buffer. */
#define IKE_MAX_BUFFER_LEN              IKE_MAX_OUTBOUND_PACKET_LEN

/* Size of the receive buffer. */
#define IKE_MAX_RECV_BUFFER_LEN         IKE_MAX_INBOUND_PACKET_LEN

/**** Data structures. ****/

/* Structure for storing a node of memory buffer. */
typedef struct ike_mem_buffer
{
    struct ike_mem_buffer *ike_flink;       /* Front link. */
    UINT8                 *ike_buffer;      /* Pointer to buffer of size
                                             * IKE_MAX_OUTBOUND_PACKET_LEN.
                                             */
} IKE_MEM_BUFFER;

/**** Global variables. ****/

/* Define the global list of memory buffers. */
extern IKE_MEM_BUFFER *IKE_Buffer_List;

/* Buffer for storing incoming packets. */
extern UINT8 *IKE_Receive_Buffer;

/**** Function prototypes. ****/

STATUS IKE_Initialize_Buffers(VOID);
STATUS IKE_Deinitialize_Buffers(VOID);
STATUS IKE_Allocate_Buffer(UINT8 **buffer);
STATUS IKE_Deallocate_Buffer(UINT8 *buffer);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_BUF_H */
