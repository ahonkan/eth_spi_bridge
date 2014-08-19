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
*        nu_usb_strm_ext.c
*
* COMPONENT
*
*       Nucleus USB Software
*
* DESCRIPTION
*
*       This file provides the implementation of external interfaces of
*       using bulk streams.
*
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*   NU_USB_STRM_GRP_Create                  Creates a stream group.
*   NU_USB_STRM_GRP_Delete                  Deletes a stream group.
*   NU_USB_STRM_GRP_Acquire_Arbitrate       Searches and acquires a free 
*                                           stream in stream group.
*   NU_USB_STRM_GRP_Acquire_Forceful        Acquires a stream identified by
*                                           'stream_id' mentioned by caller 
*                                           of this function
*   NU_USB_STRM_GRP_Release_Strm            Releases a pre-acquired stream.
*   NU_USB_STRM_Create                      Creates a single USB stream.;
*   NU_USB_STRM_Set_State                   Sets current state of stream.
*   NU_USB_STRM_Set_ID                      Sets stream ID.
*   NU_USB_STRM_Get_State                   Gets current state of stream.
*   NU_USB_STRM_Get_ID                      Gets stream ID.
*
* DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_STRM_EXT_C
#define USB_STRM_EXT_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/*Only include this file if stack is configured for Super Speed USB 
  (USB 3.0). */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_STRM_GRP_Create
*
*   DESCRIPTION
*
*       This function creates a stream group. A stream group can be 
*       associated with one pipe at a time.
*
*   INPUTS
*
*       stream_grp          Pointer to NU_USB_STREAMM_GRP control block.
*       num_streams         Number of streams in this stream group.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates operation completed successfully.
*       NU_USB_INVLD_ARG    Indicates any of the input argument is 
*                           invalid.
*
*************************************************************************/
STATUS NU_USB_STRM_GRP_Create(NU_USB_STREAM_GRP *stream_grp,
                              UINT16            num_streams)
{
    NU_USB_STREAM   *stream;
    UINT16          index;
    UINT8           rollback;
    STATUS          status, internal_sts = NU_SUCCESS;
    
    /* Check valadity of arguments. */
    NU_USB_PTRCHK(stream_grp);
    
    /* Initialize rollback with default value. */
    rollback = 0;
   
    do
    {
        /* Create lock for this stream group. */
        status = NU_Create_Semaphore(&stream_grp->stream_grp_lock,
                                     "STRMLCK",
                                     1,
                                     NU_FIFO);
        if ( status != NU_SUCCESS )
        {
            rollback = 0;
            break;
        }
        
        /* Allocate memory for requested number of streams in stream 
         * group.
         */
        status = USB_Allocate_Object( (sizeof(NU_USB_STREAM) * num_streams),
                                       (void**)&(stream_grp->streams));
        if ( status != NU_SUCCESS )
        {
            rollback = 1;
            break;
        }
        
        /* Starting from first stream, loop through each stream and reset 
         * its contents.
         */
        for(index = 0; index < num_streams; index++ )
        {
            /* Get pointer to stream. */
            stream = (stream_grp->streams + index);
            
            /* Reset memory contents. */
            memset(&stream->irp, 0x00, sizeof(NU_USB_IRP));
            
            /* Save stream ID. */
            stream->stream_id = 0;
            
            /* Initially all streams are free. */
            stream->state = STREAM_FREE;
        }
        
        /* Save number of streams in stream group. */
        stream_grp->num_streams = num_streams;
    }while(0);
    
    /* Cleanup according to rollback value. */
    switch(rollback)
    {
        case 1:
            internal_sts = NU_Delete_Semaphore(&stream_grp->stream_grp_lock);
            stream_grp->streams = NU_NULL;
            break;
        default:
            break;
    }
    
    return ( status == NU_SUCCESS ? internal_sts : status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_STRM_GRP_Delete
*
*   DESCRIPTION
*
*       This API deletes an existing stream group.
*
*   INPUTS
*
*       stream_grp          Pointer to NU_USB_STREAMM_GRP control block.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates operation completed successfully.
*       NU_USB_INVLD_ARG    Indicates any of the input argument is 
*                           invalid.
*
*************************************************************************/
STATUS NU_USB_STRM_GRP_Delete(NU_USB_STREAM_GRP *stream_grp)
{
    STATUS  status;
    
    /* Check valadity of arguments. */
    NU_USB_PTRCHK(stream_grp);
    
    /* Delete stream group semaphore. */
    status = NU_Delete_Semaphore(&stream_grp->stream_grp_lock);
    NU_USB_ASSERT( status == NU_SUCCESS );
    
    /* Deallocate memory which was allocated for streams. */
    if( stream_grp->streams )
    {
        status = USB_Deallocate_Memory(stream_grp->streams);
        NU_USB_ASSERT( status == NU_SUCCESS )
        
        stream_grp->streams = NU_NULL;
    }
    
    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_STRM_GRP_Acquire_Arbitrate
*
*   DESCRIPTION
*
*       This function search for a free stream available in stream 
*       group, reserve it in hardware, mark it used in software data 
*       structures and give access to acquired stream to the caller.
*
*   INPUTS
*
*       stream_grp          Pointer to NU_USB_STREAMM_GRP control block.
*       stream_id_out       Pointer to Stream ID of acquired stream.
*       stream_out          Pointer to acquired stream.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates operation completed successfully.
*       NU_USB_INVLD_ARG    Indicates any of the input argument is 
*                           invalid.
*       NU_USB_NO_STRM_AVAILABLE;
*                           No free stream is available.
*
*************************************************************************/
STATUS NU_USB_STRM_GRP_Acquire_Arbitrate(
                                        NU_USB_STREAM_GRP   *stream_grp,
                                        UINT16              *stream_id_out,
                                        NU_USB_STREAM       **stream_out)
{
    STATUS          status, internal_sts;
    UINT16          index;
    NU_USB_STREAM   *stream;    
    
    /* Check valadity of arguments. */
    NU_USB_PTRCHK(stream_grp);
    NU_USB_PTRCHK(stream_id_out);
    NU_USB_PTRCHK(stream_out);

    /* Reset value of output arguments to a default one. */
    *stream_id_out  = 0;
    *stream_out     = NU_NULL;
    
    /* Set status to default error value. */
    status = NU_USB_INVLD_ARG;
    
    /* Check if streams in this stream group are already initialized. */
    if ( stream_grp->streams )
    {
        /* Acqurie stream group lock. */
        status = NU_Obtain_Semaphore(&stream_grp->stream_grp_lock,
                                    NU_SUSPEND);
        if ( status == NU_SUCCESS )
        {
            status = NU_USB_NO_STRM_AVAILABLE;
        
            /* Find a free stream and return it's pointer and ID. */
            for(index = 0; index < stream_grp->num_streams; index++ )
            {
                /* Get pointer to stream. */
                stream = (stream_grp->streams + index);
                
                if ( stream->state == STREAM_FREE )
                {
                    *stream_id_out      = index;
                    *stream_out         = stream;
                    stream->state       = STREAM_BUSY;
                    stream->stream_id   = index;
                    status              = NU_SUCCESS;
                }
            }
         
            /* Release stream group lock. */   
            internal_sts = NU_Release_Semaphore(&stream_grp->stream_grp_lock);
        }
    }

    return ( status == NU_SUCCESS ? internal_sts : status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_STRM_GRP_Acquire_Forceful
*
*   DESCRIPTION
*
*       This function gives access of the requested stream identified 
*       by ‘stream_id’ if it is already free, otherwise return error.
*
*   INPUTS
*
*       stream_grp          Pointer to NU_USB_STREAM_GRP control block.
*       stream_id           Stream ID to be acquired.
*       stream_out          Pointer to acquired stream.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates operation completed successfully.
*       NU_USB_INVLD_ARG    Indicates any of the input argument is 
*                           invalid.
*       NU_USB_STRM_BUSY    Requsted stream is alreayd busy.
*
*************************************************************************/
STATUS NU_USB_STRM_GRP_Acquire_Forceful(
                                        NU_USB_STREAM_GRP   *stream_grp,
                                        UINT16              stream_id,
                                        NU_USB_STREAM       **stream_out)
{
    STATUS          status, internal_sts;
    NU_USB_STREAM   *stream;    
    
    /* Check valadity of arguments. */
    NU_USB_PTRCHK(stream_grp);
    NU_USB_PTRCHK(stream_out);

    /* Reset value of output arguments to a default one. */
    *stream_out = NU_NULL;
    
    /* Set status to default error value. */
    status = NU_USB_INVLD_ARG;
    
    /* Check if streams in this stream group are already initialized 
     * and request stream ID is withing range. 
     */
    if ( stream_grp->streams && (stream_id < stream_grp->num_streams) )
    {
        /* Acqurie stream group lock. */
        status = NU_Obtain_Semaphore(&stream_grp->stream_grp_lock,
                                    NU_SUSPEND);
        if ( status == NU_SUCCESS )
        {
            status = NU_USB_STRM_BUSY;
            stream = (stream_grp->streams + stream_id);
            
            if ( stream->state == STREAM_FREE )
            {
                *stream_out         = stream;
                stream->state       = STREAM_BUSY;
                stream->stream_id   = stream_id;
                status              = NU_SUCCESS;
            }
            
            /* Release stream group lock. */
            internal_sts = NU_Release_Semaphore(&stream_grp->stream_grp_lock);
        }
    }

    return ( status == NU_SUCCESS ? internal_sts : status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_STRM_GRP_Release_Strm
*
*   DESCRIPTION
*
*       This releases an already acquired stream.
*
*   INPUTS
*
*       stream_grp          Pointer to NU_USB_STREAM_GRP control block.
*       stream_id           Stream ID to be freed.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates operation completed successfully.
*       NU_USB_INVLD_ARG    Indicates any of the input argument is 
*                           invalid.
*
*************************************************************************/
STATUS NU_USB_STRM_GRP_Release_Strm(    
                                    NU_USB_STREAM_GRP   *stream_grp,
                                    UINT16              stream_id)
{
    STATUS          status, internal_sts;
    NU_USB_STREAM   *stream;    
    
    /* Check valadity of arguments. */
    NU_USB_PTRCHK(stream_grp);

    status = NU_USB_INVLD_ARG;
    
    if ( stream_grp->streams != NU_NULL )
    {
        /* Acqurie stream group lock. */
        status = NU_Obtain_Semaphore(&stream_grp->stream_grp_lock,
                                    NU_SUSPEND);
        if ( status == NU_SUCCESS )
        {
            stream = ( stream_grp->streams + stream_id );
            
            stream->state   = STREAM_FREE;
            status          = NU_SUCCESS;
            
            /* Release stream group lock. */
            internal_sts = NU_Release_Semaphore(&stream_grp->stream_grp_lock);
        }
    }

    return ( status == NU_SUCCESS ? internal_sts : status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_STRM_Create
*
*   DESCRIPTION
*
*       This function creates and initializes a stream.
*
*   INPUTS
*
*       stream_grp          Pointer to NU_USB_STREAMM control block.
*       length              Length of data to be received / sent on this 
*                           stream.
*       data                Data buffer.
*       callback            Function to be called by controller driver 
*                           once operation on stream will be completed.
*       UINT16              Stream ID.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates operation completed successfully.
*       NU_USB_INVLD_ARG    Indicates any of the input argument is 
*                           invalid.
*
*************************************************************************/
STATUS NU_USB_STRM_Create(NU_USB_STREAM       *cb,
                          UINT32              length,
                          UINT8               *data,
                          NU_USB_IRP_CALLBACK callback)
{
    STATUS  status;
    
    /* Check valadity of arguments. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(data);
    NU_USB_PTRCHK(callback);
    
    /* Create IRP for this stream. */
    status = NU_USB_IRP_Create(&cb->irp,
                                length,
                                data,
                                NU_TRUE,
                                NU_TRUE,
                                callback,
                                NU_NULL,
                                0);

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_STRM_Set_State
*
*   DESCRIPTION
*
*       This function sets the current state of the stream. 
*
*   INPUTS
*
*       cb                  Pointer to NU_USB_STREAMM control block.
*       state               New state of the stream.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates operation completed successfully.
*       NU_USB_INVLD_ARG    Indicates any of the input argument is 
*                           invalid.
*
*************************************************************************/
STATUS NU_USB_STRM_Set_State(NU_USB_STREAM  *cb,
                            UINT8           state)    
{
    /* Check valadity of arguments. */
    NU_USB_PTRCHK(cb);
    
    /* Set current state of stream to 'state'. */
    cb->state = state;
    
    return ( NU_SUCCESS );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_STRM_Get_State
*
*   DESCRIPTION
*
*       This function gets the current state of the stream.
*
*   INPUTS
*
*       cb                  Pointer to NU_USB_STREAMM control block.
*       state_out           Pointer to 8-bit variable containing stream 
*                           state when function returns.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates operation completed successfully.
*       NU_USB_INVLD_ARG    Indicates any of the input argument is 
*                           invalid.
*
*************************************************************************/
STATUS NU_USB_STRM_Get_State(NU_USB_STREAM  *cb,
                            UINT8           *state_out)
{
    /* Check valadity of arguments. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(state_out);
    
    /* Return state of stream in output argument 'state_out'. */
    *state_out = cb->state;
    
    return ( NU_SUCCESS );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_STRM_Set_Stream_ID
*
*   DESCRIPTION
*
*       This function sets the stream ID of stream identified by ‘cb’. 
*
*   INPUTS
*
*       cb                  Pointer to NU_USB_STREAMM control block.
*       stream_id           Stream ID of the stream.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates operation completed successfully.
*       NU_USB_INVLD_ARG    Indicates any of the input argument is 
*                           invalid.
*
*************************************************************************/
STATUS NU_USB_STRM_Set_Stream_ID(NU_USB_STREAM  *cb,
                                UINT8           stream_id)    
{
    /* Check valadity of arguments. */
    NU_USB_PTRCHK(cb);
    
    /* Set ID of this stream to the value passed by caller. */
    cb->stream_id = stream_id;
    
    return ( NU_SUCCESS );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_STRM_Get_Stream_ID
*
*   DESCRIPTION
*
*       This function gets the stream ID of stream identified by ‘cb’. 
*
*   INPUTS
*
*       cb                  Pointer to NU_USB_STREAMM control block.
*       stream_id_out       16-bit variable containing stream ID when 
*                           function returns.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates operation completed successfully.
*       NU_USB_INVLD_ARG    Indicates any of the input argument is 
*                           invalid.
*
*************************************************************************/
STATUS NU_USB_STRM_Get_Stream_ID(   NU_USB_STREAM   *cb,
                                    UINT8           *stream_id_out)
{
    /* Check valadity of arguments. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(stream_id_out);
    
    /* Return state of stream in output argument 'stream_id_out'. */
    *stream_id_out = cb->stream_id;
    
    return ( NU_SUCCESS );
}

/***********************************************************************/

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
#endif /* USB_STRM_EXT_C */
/*************************** end of file ********************************/

