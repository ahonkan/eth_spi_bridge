/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

*************************************************************************
*
* FILE NAME
*
*        nu_usb_irp_ext.c
*
* COMPONENT
*
*        Nucleus USB Software
*
* DESCRIPTION
*        This file contains implementation of NU_USB_IRP services. IRP is
*        Input/Output Request Packet, which is used by stack to communicate
*        information to hardware.
*
*
* DATA STRUCTURES
*       None.
*
* FUNCTIONS
*       NU_USB_IRP_Create                   -creates an IRP
*       NU_USB_IRP_Delete                   -deletes an IRP
*       NU_USB_IRP_Get_Accept_Short_Packets -returns accept short packets
*                                           flag of an IRP
*       NU_USB_IRP_Get_Actual_Length        -return actual length of an IRP
*       NU_USB_IRP_Get_Callback             -return callback pointer of an IRP
*       NU_USB_IRP_Get_Context              -return context info of an IRP
*       NU_USB_IRP_Get_Data                 -returns data pointer of an IRP
*       NU_USB_IRP_Get_Interval             -returns interval of an IRP
*       NU_USB_IRP_Get_Length               -returns length of data
*                                           transfer associated on an IRP
*       NU_USB_IRP_Get_Pipe                 -returns pipe associated on IRP
*       NU_USB_IRP_Get_Status               -returns status of data
*                                           transfer of IRP
*       NU_USB_IRP_Get_Use_Empty_Pkt        -returns use empty packet
*                                           flag of an IRP
*       NU_USB_IRP_Set_Accept_Short_Pkt     -sets accept short packets
*                                           flag of an IRP
*       NU_USB_IRP_Set_Actual_Length        -sets actual length of an IRP
*       NU_USB_IRP_Set_Callback             -sets callback pointer of an IRP
*       NU_USB_IRP_Set_Context              -sets context info of an IRP
*       NU_USB_IRP_Set_Data                 -sets data pointer of an IRP
*       NU_USB_IRP_Set_Interval             -sets interval of an IRP
*       NU_USB_IRP_Set_Length               -sets length of data
*                                           transfer associated on an IRP
*       NU_USB_IRP_Set_Pipe                 -sets pipe associated on IRP
*       NU_USB_IRP_Set_Status               -sets status of data
*                                           transfer of IRP
*       NU_USB_IRP_Set_Buffer_Type_Cachable          
*											-This function sets type of
*                                            buffer for IRP either as .
*                                            cacheable or non cacheable
*       NU_USB_IRP_Get_Buffer_Type_Cachable          
*											-This function returns type of
*                                            buffer
*       NU_USB_IRP_Set_Use_Empty_Pkt        -sets use empty packet
*                                           flag of an IRP
*
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_IRP_EXT_C
#define	USB_IRP_EXT_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Create
*
* DESCRIPTION
*       This function Creates an instance of IRP with initial values.
*
* INPUTS
*       cb                      pointer to IRP control block.
*       length                  length of data transfer associated with IRP.
*       data                    pointer to a location which contains
*                               transmitted or received data.
*       accept_short_packet     flag specifying to accept short packets in
*                               this transmission
*       use_empty_packets       flag specifying to use empty packets as end
*                               of  transfer markers.
*       callback                pointer to a function invoked when an IRP
*                               is completed.
*       context                 pointer to a location to pass context
*                               information back and forth between IRP
*                               submitter and IRP server.
*       Interval                polling interval for this IRP.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Create (NU_USB_IRP * cb,
                          UINT32 length,
                          UINT8 *data,
                          BOOLEAN accept_short_packets,
                          BOOLEAN use_empty_packets,
                          NU_USB_IRP_CALLBACK callback,
                          VOID *context,
                          UINT32 interval)
{
    NU_USB_PTRCHK(cb);

    /* Clear the control block */
    memset (cb, 0, sizeof (NU_USB_IRP));

    cb->length = length;
    cb->buffer = data;

    cb->callback = callback;

    cb->callback_data = context;

    /* Cast to UINT16 to remove Lint warning. */
    cb->interval = (UINT16)interval;

    if (use_empty_packets)
        cb->flags |= USB_ZERO_PACKET;

    if (!accept_short_packets)
        cb->flags |= USB_SHORT_NOT_OK;

	/* By default buffer type is always cachable. */
	cb->buffer_type = NU_TRUE;

    return (NU_SUCCESS);

}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Delete
*
* DESCRIPTION
*       This function deletes the instance of an IRP. This doesn't frees
*       the memory associated with the IRP control block.
*
* INPUTS
*       cb          pointer to IRP control block
*
* OUTPUTS
*       NU_SUCCESS  indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Delete (NU_USB_IRP * cb)
{
    NU_USB_PTRCHK(cb);

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Get_Length
*
* DESCRIPTION
*       This function returns the Length of the Data transfer associated
*       with IRP in length_out
*
* INPUTS
*       cb          pointer to IRP control block
*       length_out  pointer to a variable to hold the length field of
*                   associated IRP.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Get_Length (NU_USB_IRP * cb,
                              UINT32 *length_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(length_out);

    *length_out = cb->length;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Set_Length
*
* DESCRIPTION
*       This function sets the length of data transfer associated with an
*       IRP.
*
* INPUTS
*       cb          pointer to IRP control block.
*       length      length to be set.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Set_Length (NU_USB_IRP * cb,
                              UINT32 length)
{
    NU_USB_PTRCHK(cb);

    cb->length = length;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Get_Data
*
* DESCRIPTION
*       This function returns the Data pointer associated
*       with IRP in data_out
*
* INPUTS
*       cb          pointer to IRP control block
*       data_out    Pointer to a memory location to hold the pointer
*                   to data of associated IRP.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Get_Data (NU_USB_IRP * cb,
                            UINT8 **data_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(data_out);

    *data_out = cb->buffer;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Set_Data
*
* DESCRIPTION
*       This function sets the data pointer of data transfer associated with
*       an  IRP.
*
* INPUTS
*       cb          pointer to IRP control block.
*       data        pointer to a location to store data.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Set_Data (NU_USB_IRP * cb,
                            UINT8 *data)
{

    NU_USB_PTRCHK(cb);

    cb->buffer = data;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Get_Actual_Length
*
* DESCRIPTION
*       This function returns the actual Length of the Data transferred
*       by IRP in length_out
*
* INPUTS
*       cb          pointer to IRP control block
*       length_out  pointer to a variable to hold the returned actual length
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Get_Actual_Length (NU_USB_IRP * cb,
                                     UINT32 *length_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(length_out);

    *length_out = cb->actual_length;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Set_Actual_Length
*
* DESCRIPTION
*       This function sets the actual length of data transfer associated with an
*       IRP.
*
* INPUTS
*       cb          pointer to IRP control block.
*       length      actual length to be set.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Set_Actual_Length (NU_USB_IRP * cb,
                                     UINT32 length)
{

    NU_USB_PTRCHK(cb);

    cb->actual_length = length;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Get_Status
*
* DESCRIPTION
*       This function returns the status of the Data transfer associated
*       with IRP in status_out
*
* INPUTS
*       cb          pointer to IRP control block
*       status_out  pointer to a variable to hold the returned status of
*                   IRP.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Get_Status (NU_USB_IRP * cb,
                              STATUS *status_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(status_out);

    *status_out = cb->status;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Set_Status
*
* DESCRIPTION
*       This function sets the status of data transfer associated with an
*       IRP.
*
* INPUTS
*       cb          pointer to IRP control block.
*       status      status to be set.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Set_Status (NU_USB_IRP * cb,
                              STATUS status)
{
    NU_USB_PTRCHK(cb);

    cb->status = status;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Get_Accept_Short_Packets
*
* DESCRIPTION
*       This function returns if the associated IRP accept short packets or
*       not.
*
* INPUTS
*       cb                          pointer to IRP control block
*       accept_short_packets_out    pointer to a variable to hold returned
*                                   value of accept short packets flag on IRP
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Get_Accept_Short_Packets (NU_USB_IRP * cb,
                                            BOOLEAN * accept_short_packets_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(accept_short_packets_out);

    if (cb->flags & USB_SHORT_NOT_OK)
        *accept_short_packets_out = NU_FALSE;
    else
        *accept_short_packets_out = NU_TRUE;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Set_Accept_Short_Pkt
*
* DESCRIPTION
*       This function sets the accept short packets flag of an IRP
*
* INPUTS
*       cb                      pointer to IRP control block.
*       accept_short_packets    flag to be set.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Set_Accept_Short_Pkt (NU_USB_IRP * cb,
                                        BOOLEAN accept_short_packets)
{
    NU_USB_PTRCHK(cb);

    if (accept_short_packets)
        cb->flags &= ~USB_SHORT_NOT_OK;
    else
        cb->flags |= USB_SHORT_NOT_OK;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Get_Use_Empty_Pkt
*
* DESCRIPTION
*       This function returns if the associated IRP use empty packet to
*       mark transmission end or not.
*
* INPUTS
*       cb                          pointer to IRP control block
*       use_empty_packet_out       pointer to a variable to hold returned
*                                   value of use empty packet flag on IRP
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Get_Use_Empty_Pkt (NU_USB_IRP * cb,
                                     BOOLEAN * use_empty_packet_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(use_empty_packet_out);

    if (cb->flags & USB_ZERO_PACKET)
        *use_empty_packet_out = NU_TRUE;
    else
        *use_empty_packet_out = NU_FALSE;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Set_Use_Empty_Pkt
*
* DESCRIPTION
*       This function sets the use empty packet flag of an IRP
*
* INPUTS
*       cb                      pointer to IRP control block.
*       use_empty_packet        flag to be set.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Set_Use_Empty_Pkt (NU_USB_IRP * cb,
                                     BOOLEAN use_empty_packet)
{
    NU_USB_PTRCHK(cb);

    if (use_empty_packet)
        cb->flags |= USB_ZERO_PACKET;
    else
        cb->flags &= ~USB_ZERO_PACKET;

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Get_Callback
*
* DESCRIPTION
*       This function returns the Callback function pointer associated
*       with IRP in callback_out
*
* INPUTS
*       cb              pointer to IRP control block
*       callback_out    Pointer to a memory location to hold the pointer
*                       to callback function of associated IRP.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Get_Callback (NU_USB_IRP * cb,
                                NU_USB_IRP_CALLBACK * callback_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(callback_out);

    *callback_out = cb->callback;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Set_Callback
*
* DESCRIPTION
*       This function sets the callback function pointer associated with
*       an  IRP.
*
* INPUTS
*       cb          pointer to IRP control block.
*       callback    pointer to callback function.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Set_Callback (NU_USB_IRP * cb,
                                NU_USB_IRP_CALLBACK callback)
{
    NU_USB_PTRCHK(cb);

    cb->callback = callback;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Get_Context
*
* DESCRIPTION
*       This function returns the context data pointer associated
*       with IRP in context_out
*
* INPUTS
*       cb              pointer to IRP control block
*       context_out     Pointer to a memory location to hold the pointer
*                       to context data of associated IRP.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Get_Context (NU_USB_IRP * cb,
                               VOID **context_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(context_out);

    *context_out = cb->callback_data;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Set_Context
*
* DESCRIPTION
*       This function sets the context data associated with an  IRP.
*
* INPUTS
*       cb          pointer to IRP control block.
*       data        pointer to a location to store context.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Set_Context (NU_USB_IRP * cb,
                               VOID *context)
{
    NU_USB_PTRCHK(cb);

    cb->callback_data = context;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Get_Interval
*
* DESCRIPTION
*       This function returns the polling Interval of the Data transfer
*       associated with IRP in interval_out
*
* INPUTS
*       cb              pointer to IRP control block
*       interval_out    pointer to a variable to hold the interval field of
*                       associated IRP.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Get_Interval (NU_USB_IRP * cb,
                                UINT32 *interval_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(interval_out);

    *interval_out = cb->interval;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Set_Interval
*
* DESCRIPTION
*       This function sets the interval associated with an IRP.
*
* INPUTS
*       cb          pointer to IRP control block.
*       interval    interval to be set.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Set_Interval (NU_USB_IRP * cb,
                                UINT32 interval)
{
    NU_USB_PTRCHK(cb);

    cb->interval = (UINT16)interval;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Get_Pipe
*
* DESCRIPTION
*       This function returns the NU_USB_PIPE associated
*       with IRP in pipe_out
*
* INPUTS
*       cb              pointer to IRP control block
*       pipe_out        Pointer to a memory location to hold the pointer
*                       to NU_USB_PIPE of associated IRP.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion
*
*************************************************************************/
STATUS NU_USB_IRP_Get_Pipe (NU_USB_IRP * cb,
                            NU_USB_PIPE ** pipe_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(pipe_out);

    *pipe_out = cb->pipe;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_IRP_Set_Pipe
*
* DESCRIPTION
*       This function sets the pipe of an IRP.
*
* INPUTS
*       cb          pointer to IRP control block.
*       pipe        pointer to PIPE control block to be set.
*
* OUTPUTS
*       NU_SUCCESS  indicates successful completion.
*
*************************************************************************/
STATUS NU_USB_IRP_Set_Pipe (NU_USB_IRP * cb,
                            NU_USB_PIPE * pipe)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(pipe);

    cb->pipe = pipe;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*		NU_USB_IRP_Set_Buffer_Type_Cachable
*
* DESCRIPTION
*       This function sets type of buffer for IRP either as cacheable
*       or non cacheable.
*
* INPUTS
*       cb           Pointer to IRP control block.
*       buffer_type  Type of buffer to be set.
*                    NU_TRUE for cacheable and
*                    NU_FALSE for non cacheable buffer.
*
* OUTPUTS
*       NU_SUCCESS       Indicates successful completion.
*       NU_USB_INVLD_ARG Invalid parameters.
*
*************************** end of file ********************************/
STATUS NU_USB_IRP_Set_Buffer_Type_Cachable (NU_USB_IRP	*cb,
											BOOLEAN		buffer_type)
{
    NU_USB_PTRCHK(cb);

    cb->buffer_type = buffer_type;

    return (NU_SUCCESS);
}
/*************************************************************************
* FUNCTION
*		NU_USB_IRP_Get_Buffer_Type_Cachable
*
* DESCRIPTION
*       This function returns type of buffer.
*
* INPUTS
*       cb           Pointer to IRP control block.
*       buffer_type  Parameter to hold Type of buffer.
*                    NU_TRUE for cacheable buffer and
*                    NU_FALSE for non cacheable buffer.
*
* OUTPUTS
*       NU_SUCCESS       Indicates successful completion.
*       NU_USB_INVLD_ARG Invalid parameters.
*
*************************** end of file ********************************/
STATUS NU_USB_IRP_Get_Buffer_Type_Cachable (NU_USB_IRP	*cb,
											BOOLEAN		*buffer_type)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(buffer_type);

    *buffer_type = cb->buffer_type;

    return (NU_SUCCESS);
}

/*************************************************************************/

#endif /* USB_IRP_EXT_C */
/*************************** end of file ********************************/
