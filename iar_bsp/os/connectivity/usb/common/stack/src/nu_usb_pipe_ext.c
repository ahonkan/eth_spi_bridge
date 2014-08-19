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
*        nu_usb_pipe_ext.c
*
* COMPONENT
*       Nucleus USB Software : Pipe Component.
*
* DESCRIPTION
*       This file contains the implementation of services exported by
*       NU_USB_PIPE.
*
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       NU_USB_PIPE_Flush           -Flushes a pipe.
*       NU_USB_PIPE_Get_Device      -Returns NU_USB_DEVICE pointer
*                                   associated with the pipe.
*       NU_USB_PIPE_Get_Endp        -Returns NU_USB_ENDPpointer
*                                   associated with the pipe.
*       NU_USB_PIPE_Get_Is_Active   -Returns true if endpoint is active.
*       NU_USB_PIPE_Get_Is_Stalled  -returns true if endpoint is stalled.
*       NU_USB_PIPE_Set_Device      -Sets NU_USB_DEVICE pointer of the pipe
*       NU_USB_PIPE_Set_Endp        -Sets NU_USB_ENDP pointer of the pipe
*       NU_USB_PIPE_Set_Is_Active   -Sets the active flag of pipe.
*       NU_USB_PIPE_Stall           -Puts the pipe in stalled state.
*       NU_USB_PIPE_Submit_IRP      -Submit an IRP on a pipe.
*       NU_USB_PIPE_Cancel_IRP      -Cancel an IRP on a pipe.
*       NU_USB_PIPE_Unstall         -Puts the pipe in unstalled state.
*
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_PIPE_EXT_C
#define	USB_PIPE_EXT_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*               NU_USB_PIPE_Get_Is_Active
*
* DESCRIPTION
*       This function returns if a given pipe is active or in stalled
*       state.
*
* INPUTS
*       cb              pointer to Pipe control block.
*       is_active_out   pointer to a variable to store returned active flag
*                       of pipe.
*
* OUTPUTS
*       NU_SUCCESS          indicates successful completion.
*       NU_USB_INVLD_ARG    indicates Pipe control is not valid.
*
*************************************************************************/
STATUS NU_USB_PIPE_Get_Is_Active (NU_USB_PIPE * cb,
                                  BOOLEAN * is_active_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(is_active_out);

    return NU_USB_PIPE_Get_Is_Stalled (cb, is_active_out);
}

/*************************************************************************
* FUNCTION
*               NU_USB_PIPE_Set_Is_Active
*
* DESCRIPTION
*       This function is used to set the pipe in active or stalled state.
*
* INPUTS
*       cb          pointer to pipe control block.
*       is_active   flag to be set.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    indicates Pipe control is not valid.
*
*************************************************************************/
STATUS NU_USB_PIPE_Set_Is_Active (NU_USB_PIPE * cb,
                                  BOOLEAN is_active)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(cb->endpoint);

    if (is_active)
        return NU_USB_PIPE_Unstall (cb);
    else
        return NU_USB_PIPE_Stall (cb);
}

/*************************************************************************
* FUNCTION
*               NU_USB_PIPE_Get_Is_Stalled
*
* DESCRIPTION
*       This function returns if a given pipe is active or in stalled
*       state.
*
* INPUTS
*       cb              pointer to Pipe control block.
*       is_stalled_out  pointer to a variable to store returned stalled flag
*                       of pipe.
*
* OUTPUTS
*       NU_SUCCESS          indicates successful completion.
*       NU_USB_INVLD_ARG    indicates Pipe control is not valid.
*
*************************************************************************/
STATUS NU_USB_PIPE_Get_Is_Stalled (NU_USB_PIPE * cb,
                                   BOOLEAN * is_stalled_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(is_stalled_out);
    NU_USB_PTRCHK(cb->device);

    return NU_USB_STACK_Is_Endpoint_Stalled (cb->device->stack, cb,
                                             is_stalled_out);
}

/*************************************************************************
* FUNCTION
*               NU_USB_PIPE_Stall
*
* DESCRIPTION
*       This function is used to set the endpoint associated with this pipe
*       in halted mode. Pipe will start sending stalls for all transmissions
*       after this call.
*
* INPUTS
*       cb          pointer to pipe control block.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    indicates Pipe control is not valid.
*
*************************************************************************/
STATUS NU_USB_PIPE_Stall (NU_USB_PIPE * cb)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(cb->device);

    return NU_USB_STACK_Stall_Endpoint (cb->device->stack, cb);
}

/*************************************************************************
* FUNCTION
*               NU_USB_PIPE_Unstall
*
* DESCRIPTION
*       This function is used to clear a endpoint associated with this pipe
*       from halted mode.
*
* INPUTS
*       cb          pointer to pipe control block.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    indicates Pipe control is not valid.
*
*************************************************************************/
STATUS NU_USB_PIPE_Unstall (NU_USB_PIPE * cb)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(cb->device);

    return NU_USB_STACK_Unstall (cb->device->stack, cb);
}

/*************************************************************************
* FUNCTION
*               NU_USB_PIPE_Flush
*
* DESCRIPTION
*       This function flushes the given pipe. After this function all
*       pending IRP's on the pipe are discarded and pipe is empty.
*
* INPUTS
*       cb          Pointer to PIPE control block.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    Indicates that pipe control block is invalid.
*
*************************************************************************/
STATUS NU_USB_PIPE_Flush (NU_USB_PIPE * cb)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(cb->device);

    return NU_USB_STACK_Flush_Pipe (cb->device->stack, cb);
}

/*************************************************************************
* FUNCTION
*               NU_USB_PIPE_Get_Endp
*
* DESCRIPTION
*       This function returns the NU_USB_ENDP control block pointer,
*       this pipe is associated with.
*
* INPUTS
*       cb              pointer to Pipe control block.
*       endp_out        pointer to a variable to hold pointer to
*                       endpoint control block.
*
* OUTPUTS
*       NU_SUCCESS          indicates successful completion.
*       NU_USB_INVLD_ARG    indicates Pipe control block is not valid.
*
*************************************************************************/
STATUS NU_USB_PIPE_Get_Endp (NU_USB_PIPE * cb,
                             NU_USB_ENDP ** endp_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(endp_out);

    *endp_out = cb->endpoint;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_PIPE_Set_Endp
*
* DESCRIPTION
*       This function sets the NU_USB_ENDP control block pointer of
*       this pipe.
*
* INPUTS
*       cb              pointer to Pipe control block.
*       endp            pointer to endpoint control block.
*
* OUTPUTS
*       NU_SUCCESS          indicates successful completion.
*       NU_USB_INVLD_ARG    indicates Pipe control block is not valid.
*
*************************************************************************/
STATUS NU_USB_PIPE_Set_Endp (NU_USB_PIPE * cb,
                             NU_USB_ENDP * endp)
{
    NU_USB_PTRCHK(cb);

    cb->endpoint = endp;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_PIPE_Get_Device
*
* DESCRIPTION
*       This function returns the NU_USB_DEVICE control block pointer,
*       this pipe is associated with.
*
* INPUTS
*       cb              pointer to Pipe control block.
*       device_out      pointer to a variable to hold pointer to
*                       device control block.
*
* OUTPUTS
*       NU_SUCCESS          indicates successful completion.
*       NU_USB_INVLD_ARG    indicates Pipe control block is not valid.
*
*************************************************************************/
STATUS NU_USB_PIPE_Get_Device (NU_USB_PIPE * cb,
                               NU_USB_DEVICE ** device_out)
{
    NU_USB_PTRCHK(cb);

    *device_out = cb->device;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_PIPE_Set_Device
*
* DESCRIPTION
*       This function sets the NU_USB_DEVICE control block pointer of
*       this pipe.
*
* INPUTS
*       cb              pointer to Pipe control block.
*       device          pointer to device control block.
*
* OUTPUTS
*       NU_SUCCESS          indicates successful completion.
*       NU_USB_INVLD_ARG    indicates Pipe control block is not valid.
*
*************************************************************************/
STATUS NU_USB_PIPE_Set_Device (NU_USB_PIPE * cb,
                               NU_USB_DEVICE * device)
{
    NU_USB_PTRCHK(cb);

    cb->device = device;
    return (NU_SUCCESS);
}

/*************************************************************************/

#endif /* USB_PIPE_EXT_C */
/*************************** end of file ********************************/

