/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILENAME
*
*       sck_srab.c
*
*   DESCRIPTION
*
*       This file contains the routine for setting up the
*       s_rx_ancillary_data parameter of a socket to the most recent
*       packet received on the socket.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SCK_Set_RX_Anc_BufPtr
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/***********************************************************************
*
*   FUNCTION
*
*       SCK_Set_RX_Anc_BufPtr
*
*   DESCRIPTION
*
*       This function sets the s_rx_ancillary_data pointer of the
*       socket structure to the packet provided.  If s_rx_ancillary_data
*       is already pointing to a packet, the function checks whether the
*       application layer has received the packet, and if so, frees it.
*
*   INPUTS
*
*       *sockptr                A pointer to the socket for which to
*                               set the new pointer.
*       *buf_ptr                A pointer to the most recently received
*                               packet on the socket.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SCK_Set_RX_Anc_BufPtr(struct sock_struct *sockptr, NET_BUFFER *buf_ptr)
{
    NET_BUFFER  *saved_buf;

    /* If a pointer to a buffer is already saved off for ancillary
     * data, free that buffer.
     */
    if (sockptr->s_rx_ancillary_data)
        MEM_One_Buffer_Chain_Free(sockptr->s_rx_ancillary_data,
                                  &MEM_Buffer_Freelist);

    /* Save a pointer to the buffer to return to the application
     * layer as ancillary data.
     */
    sockptr->s_rx_ancillary_data = buf_ptr;

    /* Only the first buffer is needed.  Free the remaining buffers. */
    if (buf_ptr->next_buffer)
    {
        /* Save a pointer to the rest of the buffers after the first */
        saved_buf = buf_ptr->next_buffer;

        /* NULL terminate the first buffer */
        buf_ptr->next_buffer = NU_NULL;

        /* Free the rest of the buffers */
        MEM_One_Buffer_Chain_Free(saved_buf, &MEM_Buffer_Freelist);
    }

} /* SCK_Set_RX_Anc_BufPtr */
