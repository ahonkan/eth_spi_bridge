/**************************************************************************
*
*               Copyright Mentor Graphics Corporation 2012
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*   nu_usbf_dfu_imp.c
*
* COMPONENT
*
*   Nucleus USB Function Software : DFU Class Driver
*
* DESCRIPTION
*
*   This file contains the internal functions of the DFU Class Driver.
*
* DATA STRUCTURES
*
*   None.
*
* FUNCTIONS
*
*   _NU_USBF_DFU_Notify             This function notifies Driver of USB
*                                   events.
*   _NU_USBF_DFU_Initialize_Intf    Connect notification function invoked
*                                   by stack when driver is given an
*                                   opportunity to own an Interface.
*   _NU_USBF_DFU_Disconnect         Disconnect Callback function, invoked
*                                   by stack when a Device/Interface served
*                                   by DFU is removed from the BUS.
*   _NU_USBF_DFU_Set_Intf           Notifies driver of change in alternate
*                                   setting.
*   USBF_DFU_Handle_Disconnected    This routine processes the Disconnect
*                                   event
*   USBF_DFU_Handle_USB_Event_Rcvd  This routine processes the USB bus
*                                   event notification such as reset.
*   USBF_DFU_Handle_Initialized     This is a initialization routine for
*                                   the DFU class interface.
*   DFUF_Ctrl_Data_IRP_Cmplt        This is a callback function for
*                                   processing the control data IRP
*                                   completion.
*   _NU_USBF_DFU_New_Setup          Processes a new class specific SETUP
*                                   packet from the Host.
*   DFUF_Get_Poll_Tm_Out            This function is used to get the poll
*                                   time out from the firmware handler.
*   DFU_Detach_Tmr                  Detach timer expiration routine, which
*                                   will be run when the detach timer
*                                   expires.
*   DFU_Dnload_Bsy_Tmr              Download busy timer expiration routine.
*   DFU_Mnfst_Sync_Tmr              Manifestation sync timer expiration
*                                   routine.
*   USBF_DFU_Handle_Ctrl_Xfer_Cmplt This function submits an IRP to receive
*                                   the request from the host.
*   DFUF_State_App_Idle             This function handles the request
*                                   received by the DFU when the DFU is in
*                                   applIDLE state.
*   DFUF_State_App_Detach           This function handles the request
*                                   received by the DFU when the DFU is in
*                                   applnDETACH state.
*   DFUF_State_Dfu_Idle             This function handles the request
*                                   received by the DFU when the DFU is in
*                                   dfuIDLE state.
*   DFUF_State_Dfu_Dnload_Sync      This function handles the request
*                                   received by the DFU when the DFU is in
*                                   dfuDNLOAD-SYNC state.
*   DFUF_State_Dfu_Dnload_Busy      This function handles the request
*                                   received by the DFU when the DFU is in
*                                   dfuDNBUSY state.
*   DFUF_State_Dfu_Dnload_Idle      This function handles the request
*                                   received by the DFU when the DFU is in
*                                   dfuDNLOAD-IDLE state.
*   DFUF_State_Dfu_Mnfst_Sync       This function handles the request
*                                   received by the DFU when the DFU is in
*                                   dfuMANIFEST-SYNC state.
*   DFUF_State_Dfu_Mnfst            This function handles the request
*                                   received by the DFU when the DFU is in
*                                   dfuMANIFEST state.
*   DFUF_State_Dfu_Mnfst_wt_Reset   This function handles the request
*                                   received by the DFU when the DFU is in
*                                   dfuMANIFEST-WAIT-RESET state.
*   DFUF_State_Dfu_Upload_Idle      This function handles the request
*                                   received by the DFU when the DFU is in
*                                   dfuUPLOAD-IDLE state.
*   DFUF_State_Dfu_Error            This function handles the request
*                                   received by the DFU when the DFU is in
*                                   dfuERROR state.
*   DFUF_Send_Get_State             This function transfers the state of
*                                   the DFU device to the host.
*   DFUF_Data_Xfer_Status_Error     This function updates the user about
*                                   the detected error and stalls the
*                                   control end point.
*   DFUF_Stall_Ctrl_Pipe            This function is used to stall the
*                                   control endpoint whenever the error
*                                   is detected.
*   DFUF_Send_Get_Status_Payload    This function is used to transfer the
*                                   DFU status payload to the host.
*
* DEPENDENCIES
*
*   nu_usb.h
*
**************************************************************************/

/*======================  USB Include Files ============================*/
#include "connectivity/nu_usb.h"
#include "nu_usbf_dfu_imp.h"

/* During mode switching (either from Normal Run-Time to DFU mpde or from DFU to Normal Run-Time mode)
 * drivers are closed hence interfaces are unbound from stack resulting in a call to NU_USBF_Detach_Device
 * function. This function in turns disables the device and send a disconnect to class drivers.
 * This disconnect causes a malfunction in DFU driver state machine.
 * The idea is not to handle disconnect notification in DFU if a mode switch is in progress. Following
 * variables represents a short term state of device where mode is being switched.
 * - is_switching_dfu_mode will be NU_TRUE if device is being switched to DFU mode.
 * - is_switching_nrml_mode will be NU_TRUE if device is being switched to Normal Run-Time mode.
 * If either of these variables is TRUE then a disconnection event will not pre processed.
 */
static BOOLEAN is_switching_nrml_mode = NU_FALSE;
static BOOLEAN is_switching_dfu_mode = NU_FALSE;

/*===============================  Functions ===========================*/
/**************************************************************************
* FUNCTION
*
*   _NU_USBF_DFU_Notify
*
* DESCRIPTION
*
*   This function notifies Driver of USB Events i.e,. connect, disconnect
*   and reset.
*
* INPUTS
*
*   cb          Class driver for which this callback is meant for.
*   stack       Stack invoking this function.
*   device      Device on which this event has happened.
*   event       USB event that has occurred.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS  Indicates that the class driver could process the
*               event successfully.
*
**************************************************************************/
STATUS _NU_USBF_DFU_Notify( NU_USB_DRVR * cb,
                                      NU_USB_STACK * stack,
                                      NU_USB_DEVICE * device,
                                      UINT32 event)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_DFU *dfu = (NU_USBF_DFU *)cb;

    /* Save the received event in driver's data. */
    dfu->usb_event = event;

    if(USBF_EVENT_DISCONNECT == event)
    {
        USBF_DFU_Handle_Disconnected(dfu);
    }
    else
    {
        USBF_DFU_Handle_USB_Event_Rcvd(dfu);
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*   _NU_USBF_DFU_Initialize_Intf
*
* DESCRIPTION
*
*   Connect notification function invoked by stack when driver is given
*   an opportunity to own an Interface
*
* INPUTS
*
*   cb          Pointer to Class Driver Control Block.
*   stk         Pointer to Stack Control Block of the calling stack.
*   dev         Pointer to Device Control Block of the device found.
*   intf        Pointer to Interface control Block to be served by this
*               class driver.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS  Indicates successful completion of the service.
*
**************************************************************************/
STATUS _NU_USBF_DFU_Initialize_Intf (NU_USB_DRVR * cb,
                                              NU_USB_STACK * stk,
                                              NU_USB_DEVICE * dev,
                                              NU_USB_INTF * intf)
{
    STATUS status;
    BOOLEAN isclaimed;
    NU_USBF_DFU *dfu = (NU_USBF_DFU *)cb;
    NU_USB_DRVR *old;
    dfu->dfu_user = NU_NULL;

    /* Save the characteristic of USB device. */
    dfu->dfu_intf = intf;
    dfu->dfu_dev = dev;

    /* Release the interface if already claimed. */
    NU_USB_INTF_Get_Is_Claimed(intf, &isclaimed, &old);

    if(isclaimed)
    {
        NU_USB_INTF_Release(intf, old);
    }

    /* Call the Set_Intf routine of this driver with current interface and
     * default alternate setting.
     */
    status = _NU_USBF_DFU_Set_Intf (cb, stk, dev, intf,
                                          &intf->alt_settg[0]);

    if(NU_SUCCESS == status)
    {
        /* Claim the interface */
        status = NU_USB_INTF_Claim (intf, cb);
    }
    return status;

}

/**************************************************************************
* FUNCTION
*
*   _NU_USBF_DFU_Disconnect
*
* DESCRIPTION
*
*   Disconnect Callback function, invoked by stack when a
*   Device/Interface served by DFU is removed from the BUS.
*
* INPUTS
*
*   cb      Pointer to Class Driver Control Block claimed this
*           Interface.
*   stk     Pointer to Stack Control Block.
*   device  Pointer to NU_USB_DEVICE Control Block disconnected.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS      Indicates successful completion of the service.
*
**************************************************************************/
STATUS _NU_USBF_DFU_Disconnect(NU_USB_DRVR * cb,
                                            NU_USB_STACK * stack,
                                            NU_USB_DEVICE * device)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_DFU *dfu = (NU_USBF_DFU *)cb;

    USBF_DFU_Handle_Disconnected(dfu);

    return status;
}

/**************************************************************************
* FUNCTION
*
*   _NU_USBF_DFU_Set_Intf
*
* DESCRIPTION
*
*   Notifies driver of change in alternate setting.
*
* INPUTS
*
*   cb          Class driver for which this callback is meant for.
*   stack       Stack invoking this function.
*   device      Device on which this event has happened.
*   intf        Interface which is affected.
*   alt_settg   New alternate setting for the interface.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS  Indicates that the class driver could process the
*               event successfully.
*
**************************************************************************/
STATUS _NU_USBF_DFU_Set_Intf (NU_USB_DRVR * cb,
                                        NU_USB_STACK * stack,
                                        NU_USB_DEVICE * device,
                                        NU_USB_INTF * intf,
                                        NU_USB_ALT_SETTG * alt_settg)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_DFU *dfu = (NU_USBF_DFU *)cb;

    /* Save the current alt_setting in driver's data. */
    dfu->dfu_alt_set = alt_settg;

    USBF_DFU_Handle_Initialized(dfu);

    /* Get the MaxPacketSize for control endpoint 0. */
    status = NU_USB_DEVICE_Get_bMaxPacketSize0(dfu->dfu_dev,
                                             &dfu->max_packet_size);

    return status;
}

/**************************************************************************
* FUNCTION
*
*   USBF_DFU_Handle_Disconnected
*
* DESCRIPTION
*
*   This routine processes the Disconnect event, changes the descriptors to
*   the normal runtime descriptors and sets the DFU state as dfuAPPLN IDLE
*   state.
*
* INPUTS
*
*   dfu             Pointer to Driver control block.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   None.
*
**************************************************************************/
VOID USBF_DFU_Handle_Disconnected(NU_USBF_DFU *dfu)
{
	if((is_switching_nrml_mode == NU_TRUE) ||
	   (is_switching_dfu_mode == NU_TRUE)) return;

    /* Give a callback to the firmware handler for disconnect event
     * and it also request the descriptors to be changed to the runtime
     * descriptors.
     */
    if ((dfu->dfu_usr_callback->DFU_Event_Callbk) != NU_NULL)
    {
        if(dfu->dfu_state != DFU_SPEC_STATE_APPL_IDLE)
        {
            dfu->dfu_usr_callback->DFU_Event_Callbk(dfu->user_context,
                                                USBF_EVENT_DISCONNECT,
                                                DFUF_CHANGE_DESC_TO_RUNTM);
        }
        else
        {
            dfu->dfu_usr_callback->DFU_Event_Callbk(dfu->user_context,
                                                USBF_EVENT_DISCONNECT,
                                                DFUF_CHANGE_NO_DESC);
        }
    }

    /* Disable the timers */
    NU_Control_Timer(&dfu->dfu_dnload_busy_timer,
                                NU_DISABLE_TIMER);

    NU_Control_Timer(&dfu->dfu_mnfst_sync_timer,
                                    NU_DISABLE_TIMER);

    /* Reset the dfu state and status of the DFU */
    dfu->dfu_state = DFU_SPEC_STATE_APPL_IDLE;
    dfu->dfu_status = DFU_SPEC_STATUS_OK;
    dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX0;

    dfu->blk_num = DFU_SET;

    /* Release the interface */
    NU_USB_INTF_Release (dfu->dfu_intf, (NU_USB_DRVR *)dfu);

    /* Reset the values of driver data like interface, pipes and user. */
    dfu->dfu_user = NU_NULL;
    dfu->dfu_intf = NU_NULL;
    dfu->control_pipe = NU_NULL;
}

/**************************************************************************
* FUNCTION
*
*   USBF_DFU_Handle_USB_Event_Rcvd
*
* DESCRIPTION
*
*   This routine processes the USB bus event notification such as reset.
*
* INPUTS
*
*   dfu             Pointer to Driver control block.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   None.
*
**************************************************************************/
VOID USBF_DFU_Handle_USB_Event_Rcvd(NU_USBF_DFU *dfu)
{
    STATUS status;

    if(USBF_EVENT_RESET == dfu->usb_event)
    {
       /* Check if the dfu state is in DFU_DETACH and timer is still
        * running.
        */
        if(DFU_SPEC_STATE_APPL_DETACH == dfu->dfu_state)
        {
            if(DFU_DETACH_TMR_RUNNING == dfu->detach_tmr_status)
            {
                status = NU_Control_Timer(&dfu->dfu_detach_timer,
                                                NU_DISABLE_TIMER);
                if(status != NU_SUCCESS)
                {
                    while(0);
                }
                /* Update the Detach timer status as expired and exit. */
                dfu->detach_tmr_status = DFU_DETACH_TMR_EXPIRED;

                /* Give a call back to the DFU application to change the
                 * Runtime descriptors to the DFU descriptors.
                 */
                if ((dfu->dfu_usr_callback->DFU_Event_Callbk) != NU_NULL)                 
                {
                    dfu->dfu_usr_callback->
                                DFU_Event_Callbk(dfu->user_context,
                                                      USBF_EVENT_RESET,
                                                      DFUF_CHANGE_DESC_TO_DFU);
                }

                /* DFU changes its state to the dfuIDLE and change the
                 * detach timer status as IDLE.
                 */
                dfu->dfu_state = DFU_SPEC_STATE_DFU_IDLE;
                dfu->detach_tmr_status = DFU_DETACH_TMR_IDLE;
                is_switching_dfu_mode = NU_FALSE;
            }
            else
            {
                dfu->dfu_state = DFU_SPEC_STATE_APPL_IDLE;
                is_switching_dfu_mode = NU_FALSE;
            }
        }
        else if(DFU_SPEC_STATE_APPL_IDLE != dfu->dfu_state)
        {
            if(dfu->detach_rcvd == DFUF_DETACH_RCVD_FALSE)
            {
            	is_switching_nrml_mode = NU_TRUE;

                /* Received USB reset, when in the DFU state. Give a call
                 * back to the DFU application to change the descriptors to
                 * the runtime descriptors.
                 */
                if ((dfu->dfu_usr_callback->DFU_Event_Callbk) != NU_NULL)
                {
                    dfu->dfu_usr_callback->DFU_Event_Callbk(dfu->user_context,
                                                USBF_EVENT_RESET,
                                                DFUF_CHANGE_DESC_TO_RUNTM);
                }
                /* DFU changes its state to the applIDLE */
                dfu->dfu_state = DFU_SPEC_STATE_APPL_IDLE;
                dfu->dfu_status = DFU_SPEC_STATUS_OK;
                dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX0;
                dfu->blk_num = DFU_SET;
                is_switching_nrml_mode = NU_FALSE;

                /* Disable the timers */
                status = NU_Control_Timer(&dfu->dfu_dnload_busy_timer,
                                            NU_DISABLE_TIMER);

                status = NU_Control_Timer(&dfu->dfu_mnfst_sync_timer,
                                                NU_DISABLE_TIMER);
            }
            else
            {
                dfu->detach_rcvd = DFUF_DETACH_RCVD_FALSE;
                is_switching_nrml_mode = NU_FALSE;
            }
        }
        else
        {
            /* Received USB reset when in the DFU applIDLE state. In which
             * case just give a call back to the DFU application for
             * reception of the reset event with no change in the
             * descriptors. And the DFU state remains in the same applIDLE
             * state.
             */
            if ((dfu->dfu_usr_callback->DFU_Event_Callbk) != NU_NULL)
            {
                dfu->dfu_usr_callback->DFU_Event_Callbk(dfu->user_context,
                                                      USBF_EVENT_RESET,
                                                      DFUF_CHANGE_NO_DESC);
            }
        }

        /* Flush the control pipe. */
        if(NU_NULL != dfu->control_pipe)
        {
            NU_USB_PIPE_Flush(dfu->control_pipe);
        }
    }
    else if (USBF_EVENT_CONNECT != dfu->usb_event)
    {
        /* Give a call back to the user informing about the received event.
         */
        if ((dfu->dfu_usr_callback->DFU_Event_Callbk) != NU_NULL)
        {
            dfu->dfu_usr_callback->DFU_Event_Callbk(dfu->user_context,
                                                    dfu->usb_event,
                                                    DFUF_CHANGE_NO_DESC);
        }
    }
}

/**************************************************************************
* FUNCTION
*
*   USBF_DFU_Handle_Initialized
*
* DESCRIPTION
*
*   This is a initialization routine for the DFU class interface. After the
*   initialization is done it gives a connect call back to the user and
*   submits an IRP to receive request from the HOST.
*
* INPUTS
*
*   dfu             Pointer to Driver control block.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   None.
*
**************************************************************************/
VOID USBF_DFU_Handle_Initialized (NU_USBF_DFU *dfu)
{
    STATUS status;
    NU_USB_PIPE *pipe;
    NU_USB_ALT_SETTG *alt_settg = dfu->dfu_alt_set;
 
    NU_USB_USER *prev_user;

    /* Find the default pipe */
    status = NU_USB_ALT_SETTG_Find_Pipe (alt_settg, USB_MATCH_EP_ADDRESS,
                                             0, 0, 0, &pipe);
    dfu->control_pipe = pipe;

    if(NU_SUCCESS == status)
    {
        /* Give a disconnect callback to the user. */
        prev_user = dfu->dfu_user;

        /* Disconnect the old user, if connect is to be sent to new user.*/
        if (NU_NULL != prev_user)
        {
            /* Give a callback to the firmware handler for disconnect
             * event.
             */
            if ((dfu->dfu_usr_callback->DFU_Event_Callbk) != NU_NULL)
            {
                dfu->dfu_usr_callback->DFU_Event_Callbk(dfu->user_context,
                                                     USBF_EVENT_DISCONNECT,
                                                     DFUF_CHANGE_NO_DESC);
            }
        }

        if ((dfu->dfu_usr_callback->DFU_Event_Callbk) != NU_NULL)       
        {
            status = dfu->dfu_usr_callback->
                    DFU_Event_Callbk(dfu->user_context,
                                          USBF_EVENT_CONNECT,
                                          DFUF_CHANGE_NO_DESC);
        }

    }
}

/**************************************************************************
* FUNCTION
*
*   DFUF_Ctrl_Data_IRP_Cmplt
*
* DESCRIPTION
*
*  This is a callback function for processing the control data IRP
*  completion.
*
* INPUTS
*
*   pipe               Pipe on which data transfer takes place
*   irp                Pointer to the control block of the IRP.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   None.
*
**************************************************************************/
VOID DFUF_Ctrl_Data_IRP_Cmplt(NU_USB_PIPE * pipe,
                                                NU_USB_IRP  *irp)
{
    NU_USBF_DFU *dfu;

    /* Get the context of IRP. */
    NU_USB_IRP_Get_Context (irp, (VOID **)&dfu);

    USBF_DFU_Handle_Ctrl_Xfer_Cmplt(dfu);
}

/**************************************************************************
* FUNCTION
*
*      _NU_USBF_DFU_New_Setup
*
* DESCRIPTION
*
*   Processes a new class specific SETUP packet from the Host.
*
*   The format of the setup packet is defined by the corresponding
*   class specification / custom protocol. The setup packet is
*   validated by this function and processed further as per the class
*   specification.
*
*   If there is any data phase associated with the setup request,
*   then the NU_USB_Submit_Irp function can be used to submit the
*   transfer to the stack. Status phase is automatically handled by the
*   Stack. If there is no data transfer associated with the command,
*   then no transfer is submitted.
*
*   For unknown and unsupported command, this function returns
*   appropriate error status. If this function returns any status other
*   than NU_SUCCESS, then the default endpoint will be stalled.
*
* INPUTS
*
*   cb          Class driver for which this callback is meant for.
*   stack       Stack invoking this function.
*   device      Device on which this event has happened.
*   setup       the 8 byte setup packet originating from the Host.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS          Indicates that the class driver could process the
*                       event successfully.
*
**************************************************************************/
STATUS _NU_USBF_DFU_New_Setup( NU_USB_DRVR * cb,
                                             NU_USB_STACK * stack,
                                             NU_USB_DEVICE * device,
                                             NU_USB_SETUP_PKT * setup)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_DFU *dfu = (NU_USBF_DFU *)cb;

    /* This is a call back function received when ever there is a request
     * from the host. Copy the setUp packet to the control block of the DFU
     * and call the class request handler routine to handle the received
     * request.
     */

    /* Save the received setup packet in the driver's data */
    memcpy(&dfu->setup_pkt, setup, sizeof(NU_USB_SETUP_PKT));

    /* Update the firmware handler about the request received by
     * the DFU device, excluding the detach, getstate and getstatus
     * requests.
     */
    if(DFU_SPEC_DETACH != setup->bRequest &&
       DFU_SPEC_GETSTATE != setup->bRequest &&
       DFU_SPEC_GETSTATUS != setup->bRequest)
    {
        if (dfu->dfu_usr_callback->DFU_Request_Update_Callbk != NU_NULL)
        {
             dfu->dfu_usr_callback->
                DFU_Request_Update_Callbk(dfu->user_context,
                                                 setup->bRequest);
        }
    }

    /* 1) Initially the DFU is in a applIDLE state
     * 2) have a array of function pointers for each of the DFU state(like
     *    applIDLE, dfuError etc)
     * 3) Indexing of the function pointers is based on the respective DFU
     *    state value as defined in the spec.
     * When any request comes in, do the following steps:
     * 4) Based on the bRequest value, index into the corresponding
     *    function to handle the request received.
     *    (i) If any data phase involved in the received request then
     *        submit the IRP for data phase(and send or receive data based
     *        on the request)
     * 5) When an IRP is submitted to receive or send data, it
     *    registers a function which will be called once the data is
     *    transferred.This function then calls an appropriate handler which
     *    will process the post data transfer things and submits an IRP to
     *    receive further requests.
     * 6) Once the request is processed, return to this point and submit
     *    the IRP to be able to receive the next request.
     */

    /* Branch to the function indexed by the current state of the DFU */
    DFUF_Rqst_Handlers[dfu->dfu_state](dfu);

    return status;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_Get_Poll_Tm_Out
*
* DESCRIPTION
*
*   This function is used to get the poll time out from the firmware
*   handler. This poll time out can be dynamically adjusted by the firmware
*   handler depending on the time that may be needed by it for
*   manifestation phase or writing the received firmware data to the flash
*   or fetching the firmware data from the flash. Poll time out is sent to
*   host as part of get status payload structure
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   poll_tm_out         Poll time out value
*
* RETURNS
*
*   NU_SUCCESS          On successful return of the poll time out value to
*                       the caller
*
**************************************************************************/
STATUS DFUF_Get_Poll_Tm_Out(NU_USBF_DFU *dfu, UINT32 *poll_tm_out)
{
    UINT32 tm_out;

    /* Get the poll time out value from the Firmware handler. */
    if (dfu->dfu_usr_callback->DFU_Get_Poll_Tm_Out != NU_NULL)
    {
        dfu->dfu_usr_callback->DFU_Get_Poll_Tm_Out( poll_tm_out);
    }

    tm_out = *poll_tm_out;

    tm_out = tm_out * 10;

    /* Get the poll time out value received into the 3 byte array of
     * poll time out member variable in the get status pay load
     * structure.
     */
    dfu->dfu_get_status_payload.dfu_poll_time_out[0] =
                                            (UINT8)tm_out;
    dfu->dfu_get_status_payload.dfu_poll_time_out[1] =
                                            (UINT8)(tm_out >> 8);
    dfu->dfu_get_status_payload.dfu_poll_time_out[2] =
                                            (UINT8)(tm_out >> 16);

    return NU_SUCCESS;
}

/**************************************************************************
* FUNCTION
*
*   DFU_Detach_Tmr
*
* DESCRIPTION
*
*   Detach timer expiration routine, which will be run when the detach
*   timer expires.
*
* INPUTS
*
*   id                  An UNSIGNED data element supplied to the timer
*                       expiration routine. The parameter may be used to
*                       help identify timers that use the same expiration
*                       routine.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   None.
*
**************************************************************************/
VOID DFU_Detach_Tmr(UNSIGNED id)
{
    NU_USBF_DFU *dfu;

    dfu = (NU_USBF_DFU *)id;

    /* Update the Detach timer status as expired and exit. */
    dfu->detach_tmr_status = DFU_DETACH_TMR_EXPIRED;
}

/**************************************************************************
* FUNCTION
*
*   DFU_Dnload_Bsy_Tmr
*
* DESCRIPTION
*
*   Download busy timer expiration routine, which will be run when the
*   download busy timer expires.
*
* INPUTS
*
*   id                  An UNSIGNED data element supplied to the timer
*                       expiration routine. The parameter may be used to
*                       help identify timers that use the same expiration
*                       routine.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   None.
*
**************************************************************************/
VOID DFU_Dnload_Bsy_Tmr(UNSIGNED id)
{
    NU_USBF_DFU *dfu;

    dfu = (NU_USBF_DFU *)id;

    if(DFU_SPEC_STATE_DFU_DNBUSY == dfu->dfu_state)
    {
        dfu->dfu_state = DFU_SPEC_STATE_DFU_DNLOAD_SYNC;
    }

    /* Once the timer expires after poll time out period, it sets the
     * dfu_dnld_blk_xfer_status as DFUF_DNLOAD_COMPLETED.
     */
    dfu->dfu_dnld_blk_xfer_status = DFUF_DNLOAD_COMPLETED;
}

/**************************************************************************
* FUNCTION
*
*   DFU_Mnfst_Sync_Tmr
*
* DESCRIPTION
*
*   Manifestation sync timer expiration routine, which will be run when the
*   manifestation sync timer expires.
*
* INPUTS
*
*   id                  An UNSIGNED data element supplied to the timer
*                       expiration routine. The parameter may be used to
*                       help identify timers that use the same expiration
*                       routine.
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   None.
*
**************************************************************************/
VOID DFU_Mnfst_Sync_Tmr(UNSIGNED id)
{
    NU_USBF_DFU *dfu;

    dfu = (NU_USBF_DFU *)id;

    /* Once the timer expires after poll time out period, it sets the
     * dfu_mnfst_status as DFUF_MNFST_COMPLETED.
     */
    dfu->dfu_mnfst_status = DFUF_MNFST_COMPLETED;

    /* The Poll Timeout has happened for the manifestation phase and if
     * bitTolerant is set the DFU state enters the manifestation sync state,
     * in this state it waits for the GET_STATUS request from the host.
     */
    if(DFU_SPEC_STATE_DFU_MNFST == dfu->dfu_state)
    {
        if(DFUF_RTM_FUN_DES_BMATTRIBUTES & DFU_FUN_DES_BITMNFST_TLRNT_TRUE)
        {
            dfu->dfu_state = DFU_SPEC_STATE_DFU_MNFST_SYNC;
        }
        else
        {
            /* If bitTolerant is reset then DFU state enters the
             * dfuMANIFEST_WAIT-RESET state
             */
            dfu->dfu_state = DFU_SPEC_STATE_DFU_MNFST_WT_RST;
        }
    }
}

/**************************************************************************
* FUNCTION
*
*   USBF_DFU_Handle_Ctrl_Xfer_Cmplt
*
* DESCRIPTION
*
*   This function is called whenever there is a control transfer complete
*   call back is received. This function submits an IRP to receive the
*   request from the host.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   None.
*
**************************************************************************/
VOID USBF_DFU_Handle_Ctrl_Xfer_Cmplt(NU_USBF_DFU *dfu)
{
    NU_USB_IRP *irp = &dfu->ctrl_irp;
    UINT32 cmpltd_len;
    STATUS status;
    UINT32 bytes_read;
    UINT8 data_pending;
    UINT32 buf_offset;

    /* Get the transfer attributes. */
    NU_USB_IRP_Get_Actual_Length(irp, &cmpltd_len);

    bytes_read = DFU_RESET;
    buf_offset = DFU_RESET;

    /* If OUT transfer, then pass the firmware data received to the
     * firmware handler.
     *
     * Note: IS transfer done call back required for In transaction
     *       complete??
     */
    if(DFU_SPEC_DNLOAD == dfu->setup_pkt.bRequest)
    {
        /* Once the firmware data is received, DFU enters the
         * DNLOAD_SYNC state. The DFU status and status index is set by
         * the firmware handler from the DFU_Data_Xfer_Callbk using
         * NU_USBF_DFU_Set_Status.
         */

        /* wLength > 0, DFU enters the download SYNC state. */
        dfu->dfu_state = DFU_SPEC_STATE_DFU_DNLOAD_SYNC;

        /* Give a call back to the firmware handler providing it with
         * the received firmware.
         */
        do
        {
            status = dfu->dfu_usr_callback->
                                DFU_Data_Xfer_Callbk(dfu,
                                            dfu->user_context,
                                            (dfu->data_buffer + buf_offset),
                                            cmpltd_len,
                                            &bytes_read,
                                            &data_pending,
                                            DFUF_DATA_XFER_SUCCESS,
                                            DFUF_FW_DNLOAD);
            if(status != NU_SUCCESS)
            {
                break;
            }

            if(DFU_SPEC_STATUS_OK != dfu->dfu_status)
            {
                /* Update the user about the error detected in the
                 * firmware download.
                 */
                DFUF_Data_Xfer_Status_Error(dfu);
                return;
            }

            cmpltd_len = cmpltd_len - bytes_read;
            buf_offset = bytes_read;
            bytes_read = DFU_RESET;
        }while(DFUF_DATA_XFER_PENDING == data_pending &&
                                         cmpltd_len > 0);
    }
    else if(DFU_SPEC_UPLOAD == dfu->setup_pkt.bRequest)
    {
        /* Note: Check here if the actual transferred length is equal to
         * the expected transferred length. If not pipe needs to be
         * stalled??
         */
        if(cmpltd_len < dfu->data_len)
        {
            dfu->dfu_state = DFU_SPEC_STATE_DFU_IDLE;
        }
        else
        {
            dfu->dfu_state = DFU_SPEC_STATE_DFU_UPLOAD_IDLE;
        }

    }
}

/**************************************************************************
* FUNCTION
*
*   DFUF_State_App_Idle
*
* DESCRIPTION
*
*   This function is called from the _NU_USBF_DFU_New_Setup
*   function. This function handles the request received by the DFU when the
*   DFU is in applIDLE state.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS
*   NU_USB_INVLD_ARG
*
**************************************************************************/
STATUS DFUF_State_App_Idle(NU_USBF_DFU *dfu)
{
    UNSIGNED tmr_ticks;
    NU_USB_SETUP_PKT *setup;
    STATUS status;

    setup = &dfu->setup_pkt;

    /* Received DFU_DETACH request. */
    if(DFU_SPEC_DETACH == setup->bRequest)
    {
        /* Note: How does device perform attach_detach sequence when
         * received the detach request and bitWillDetach bit is set in the
         * bmAttributes of functional descriptor.??
         */

        /* Check if previous state was applIDLE? */
        if(DFU_SPEC_STATE_APPL_IDLE == dfu->dfu_state)
        {
            /* Check if the wTimeout field is less than the wDetachTimeout
             * field of the functional descriptors. If yes then Process the
             * Detach request by starting the timer. Change the DFU state as
             * appDETACH and detach timer status as timer running.
             */
            if(DFUF_WDETACH_TMOUT_IN_MS >= setup->wValue)
            {
                /* Convert the milliseconds to timer ticks */
                tmr_ticks = setup->wValue * DFUF_NUM_OF_TMR_TICKS_FOR_1_MS;

                /* Start the Detach timer */
                status = NU_Reset_Timer(&dfu->dfu_detach_timer,
                                        DFU_Detach_Tmr,
                                        tmr_ticks, DFU_RESET,
                                        NU_ENABLE_TIMER);
                if(NU_SUCCESS == status)
                {
                    dfu->dfu_state = DFU_SPEC_STATE_APPL_DETACH;
                    dfu->detach_rcvd = DFUF_DETACH_RCVD_TRUE;
                    dfu->detach_tmr_status = DFU_DETACH_TMR_RUNNING;
                    is_switching_dfu_mode = NU_TRUE;
                }
            }
            else
            {
                /* Update the firmware handler about the reception of the
                 * Detach request with invalid timeout value.
                 */
            if (dfu->dfu_usr_callback->DFU_Request_Update_Callbk != NU_NULL)
            {
                dfu->dfu_usr_callback->
                       DFU_Request_Update_Callbk(dfu->user_context,
                                                 DFU_DETACH_INVALID_TMOUT);
            }

                /* wTimeout value is larger than the wDetachTimeout value,
                 * stall the pipe.
                 * Note:
                 * 1) Specifications do not mention any specific action 
                 * for this condition, but here we are stalling the control
                 *  pipe. 
                 * 2) what should be the DFU state in this condition??
                 */
                NU_USB_PIPE_Stall(dfu->control_pipe);

                status = NU_INVALID_ENTRY;
                dfu->dfu_state = DFU_SPEC_STATE_APPL_IDLE;
                dfu->dfu_status = DFU_SPEC_STATUS_ERR_STALLEEDPKT;
                dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX15;

            }
        }
        else
        {
            /* Stall the control pipe, which then host issues clear request.
             */
            NU_USB_PIPE_Stall(dfu->control_pipe);

            /* Update the status indicating that DETACH request received
             * when the current state was not in the applnIdle state, and
             * make the DFU state as applnIDLE state.
             */
            status = NU_INVALID_ENTRY;
            dfu->dfu_state = DFU_SPEC_STATE_APPL_IDLE;
            dfu->dfu_status = DFU_SPEC_STATUS_ERR_STALLEEDPKT;
            dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX15;
        }
    }
    else if(DFU_SPEC_GETSTATUS == setup->bRequest)
    {
        /* call the callback to prepare the GetStatus payload to send
         * to the host.
         */
        status = DFUF_Send_Get_Status_Payload(dfu);
        if(NU_SUCCESS != status)
        {
            /* Stall the pipe as submission of the IRP failed */
            NU_USB_PIPE_Stall(dfu->control_pipe);

            dfu->dfu_state = DFU_SPEC_STATE_APPL_IDLE;
            dfu->dfu_status = DFU_SPEC_STATUS_ERR_STALLEEDPKT;
            dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX15;
        }
    }
    else if(DFU_SPEC_GETSTATE == setup->bRequest)
    {
        /* Return the DFU state by submitting an IRP. Here the PollTimeOut
         * is ignored.
         */
        status = DFUF_Send_Get_State(dfu);
        if(NU_SUCCESS != status)
        {
            /* Stall the pipe as submission of the IRP failed */
            NU_USB_PIPE_Stall(dfu->control_pipe);

            dfu->dfu_state = DFU_SPEC_STATE_APPL_IDLE;
            dfu->dfu_status = DFU_SPEC_STATUS_ERR_STALLEEDPKT;
            dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX15;
        }
    }
    else
    {
        /* Receipt of any other class specific request, stall the control
         * pipe.
         */
        NU_USB_PIPE_Stall(dfu->control_pipe);

        /* Update the status indicating that DETACH request received
         * when the current state was not in the applnIdle state, and
         * make the DFU state as applnIDLE state.
         */
        status = NU_INVALID_ENTRY;
        dfu->dfu_state = DFU_SPEC_STATE_APPL_IDLE;
        dfu->dfu_status = DFU_SPEC_STATUS_ERR_STALLEEDPKT;
        dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX15;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_State_App_Detach
*
* DESCRIPTION
*
*   This function is called from the _NU_USBF_DFU_New_Setup
*   function. This function handles the request received by the DFU when the
*   DFU is in applnDETACH state.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS
*   NU_USB_INVLD_ARG
*
**************************************************************************/
STATUS DFUF_State_App_Detach(NU_USBF_DFU *dfu)
{
    STATUS status;
    NU_USB_SETUP_PKT *setup;

    setup = &dfu->setup_pkt;

    if(DFU_SPEC_GETSTATUS == setup->bRequest)
    {
        /* call the callback to prepare the GetStatus payload to send
         * to the host.
         */
        status = DFUF_Send_Get_Status_Payload(dfu);
        if(NU_SUCCESS != status)
        {
            /* Stall the pipe as submission of the IRP failed */
            NU_USB_PIPE_Stall(dfu->control_pipe);

            dfu->dfu_state = DFU_SPEC_STATE_APPL_IDLE;
            dfu->dfu_status = DFU_SPEC_STATUS_ERR_STALLEEDPKT;
            dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX15;
        }
    }
    else if(DFU_SPEC_GETSTATE == setup->bRequest)
    {
        /* Return the status response, here PollTimeOut is ignored */
        status = DFUF_Send_Get_State(dfu);
        if(NU_SUCCESS != status)
        {
            /* Stall the pipe as submission of the IRP failed */
            NU_USB_PIPE_Stall(dfu->control_pipe);

            dfu->dfu_state = DFU_SPEC_STATE_APPL_IDLE;
            dfu->dfu_status = DFU_SPEC_STATUS_ERR_STALLEEDPKT;
            dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX15;
        }
    }
    else
    {
        /* Receipt of any other class specific request, stall the control
         * pipe, which then host issues clear request.
         */
        NU_USB_PIPE_Stall(dfu->control_pipe);

        /* Update the status indicating that DETACH request received
         * when the current state was not in the applnIdle state, and
         * make the DFU state as applnIDLE state.
         */
        status = NU_INVALID_ENTRY;

        /* The next state of the DFU changes to applIDLE. */
        dfu->dfu_state = DFU_SPEC_STATE_APPL_IDLE;
        dfu->dfu_status = DFU_SPEC_STATUS_ERR_STALLEEDPKT;
        dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX15;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_State_Dfu_Idle
*
* DESCRIPTION
*
*   This function is called from the _NU_USBF_DFU_New_Setup
*   function. This function handles the request received by the DFU when the
*   DFU is in dfuIDLE state.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS
*   NU_USB_INVLD_ARG
*
**************************************************************************/
STATUS DFUF_State_Dfu_Idle(NU_USBF_DFU *dfu)
{
    NU_USB_SETUP_PKT *setup;
    STATUS status;
    UINT32 bytes_read;
    UINT8  data_pending;
    UINT32 buf_len;
    UINT32 dnld_busy_tmout;
    UINT32 bytes_in_buf;
    UINT32 mod_buf_len;
    UINT32 bytes_to_be_sent;

    bytes_read = DFU_RESET;

    setup = &dfu->setup_pkt;

    if(DFU_SPEC_DNLOAD == setup->bRequest)
    {
        /* Stall the control pipe in following cases
         * 1) If the download length is zero, the data block is a first
         *    block, in which case download is not considered to be useful.
         * 2) If the download bit is reset.
         * 3) If the host specified length, wLength, is greater than the
         *    wTransferSize of the device functional descriptor.
         * 4) If the block number is not zero, as this is the first block
         *    of the transfer.
         */
        if((DFU_RESET == setup->wLength) ||
          (!(DFUF_RTM_FUN_DES_BMATTRIBUTES & DFU_FUN_DES_BIT_CANDNLD_TRUE))
          || (DFUF_WTRANSFER_SIZE < setup->wLength) ||
          (DFU_RESET != setup->wValue)
          )
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

            /* Update the status indicating that DNLOAD request received
             * wLength value as zero.
             */
            status = NU_INVALID_SIZE;
        }
        else if((DFU_RESET < setup->wLength) &&
           (DFUF_RTM_FUN_DES_BMATTRIBUTES & DFU_FUN_DES_BIT_CANDNLD_TRUE))
        {
            dfu->data_len = setup->wLength;

            /* wLength > 0, DFU enters the download SYNC state. */
            dfu->dfu_state = DFU_SPEC_STATE_DFU_DNLOAD_SYNC;

            /* Set the block xfer variable as download in progress,
             * indicating the device is writing to the non_volatile memory.
             */
            dfu->dfu_dnld_blk_xfer_status = DFUF_DNLOAD_IN_PROGRESS;

            /* Submit IRP request to receive the firmware data in the buffer
             * or to receive the next getstatus request of the wLength is
             * zero. wLength value zero indicates the end of firmware
             * download.
             * Note:
             * 1) what is the optimal value for the buffer length here?
             * 2) If the buffer length is set more than the current 256,
             *    then UINT8 array cmd_buffer should be made as pointer and
             *    memory should be allocated using the NU_Allocate.
             */
            status = NU_USB_IRP_Create(&dfu->ctrl_irp,
                                    dfu->data_len,
                                    dfu->data_buffer,
                                    NU_TRUE,
                                    NU_FALSE,
                                    DFUF_Ctrl_Data_IRP_Cmplt, (VOID *)dfu,
                                    DFU_RESET);

            if(NU_SUCCESS == status)
            {
                status = NU_USB_PIPE_Submit_IRP(dfu->control_pipe,
                                                        &dfu->ctrl_irp);
            }

            if(NU_SUCCESS != status)
            {
                DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
            }
        }
    }
    else if(DFU_SPEC_UPLOAD == setup->bRequest)
    {
        /* Check if the upload bit in the bmAttributes is set and the
         * block number is zero as its the first block of the transfer.
         */
        if((DFUF_RTM_FUN_DES_BMATTRIBUTES & DFU_FUN_DES_BIT_CANUPLD_TRUE)&&
            (DFU_RESET == setup->wValue))
        {
            /* Check if the wLength is greater than max data length that
             * the DFU can handle.
             */
            if(DFUF_WTRANSFER_SIZE >= setup->wLength &&
                                            0 < setup->wLength)
            {
                dfu->data_len = setup->wLength;
            }
            else
            {
                /* wLength is greater than the wTransferSize, so stall the
                 * control pipe.
                 */
                DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

                /* Set the wLength to Zero. This is done to make sure that
                 * the controller driver does not try to write any data into
                 * hardware buffer
                 */
                setup->wLength = 0;

                /* Update the status indicating that DNLOAD request received
                 * wLength value as zero.
                 */
                status = NU_SUCCESS;

                return status;
            }

            dfu->dfu_state = DFU_SPEC_STATE_DFU_UPLOAD_IDLE;

            /* Give a call back to the firmware handler to get the
             * firmware to be transferred to the host.
             */
            buf_len = dfu->data_len;
            bytes_in_buf = DFU_RESET;

            do
            {
                status = dfu->dfu_usr_callback->
                            DFU_Data_Xfer_Callbk(dfu,
                                          dfu->user_context,
                                          (dfu->data_buffer + bytes_in_buf),
                                           buf_len,
                                           &bytes_read,
                                           &data_pending,
                                           DFUF_DATA_XFER_IN_PROGRESS,
                                           DFUF_FW_UPLOAD);
                if(NU_SUCCESS != status)
                {
                    /* If the callback fails, break from the do while loop.
                     */
                    break;
                }
                else if(DFU_SPEC_STATUS_OK != dfu->dfu_status)
                {
                    /* Update the user about the error detected in the
                     * firmware upload.
                     */
                    DFUF_Data_Xfer_Status_Error(dfu);
                    return NU_NO_MEMORY;
                }
                else
                {
                    buf_len = buf_len - bytes_read;
                    bytes_in_buf = bytes_in_buf + bytes_read;
                    mod_buf_len = bytes_in_buf %
                                        dfu->max_packet_size;
                    if(mod_buf_len > 0)
                    {
                        if(bytes_in_buf <= dfu->max_packet_size)
                        {
                            bytes_to_be_sent = bytes_in_buf;
                        }
                        else
                        {
                            if(DFUF_DATA_XFER_DONE == data_pending ||
                                0 == buf_len)
                            {
                                bytes_to_be_sent = bytes_in_buf;
                            }
                            else
                            {
                                bytes_to_be_sent = bytes_in_buf -
                                                            mod_buf_len;
                            }
                        }
                    }
                    else if(mod_buf_len == 0)
                    {
                        bytes_to_be_sent = bytes_in_buf;
                    }

                    if((dfu->max_packet_size <= bytes_to_be_sent) ||
                        (dfu->max_packet_size > bytes_to_be_sent &&
                        data_pending == DFUF_DATA_XFER_DONE))
                    {
                        /* Submit IRP request to transfer the firmware data
                         * to the host.
                         */
                        status = NU_USB_IRP_Create(&dfu->ctrl_irp,
                                            bytes_to_be_sent,
                                            dfu->data_buffer,
                                            NU_TRUE,
                                            NU_FALSE,
                                            DFUF_Ctrl_Data_IRP_Cmplt,
                                            (VOID *)dfu,
                                            DFU_RESET);

                        if(NU_SUCCESS == status)
                        {
                            status = NU_USB_PIPE_Submit_IRP(
                                                        dfu->control_pipe,
                                                        &dfu->ctrl_irp);
                        }

                        if(NU_SUCCESS != status)
                        {
                            /* If the submission of the IRP fails then
                             * break from the do while loop and stall the
                             * control pipe.
                             */
                            break;
                        }
                    }
                    if(dfu->max_packet_size <= bytes_to_be_sent ||
                       data_pending == DFUF_DATA_XFER_DONE)
                    {
                        bytes_in_buf = bytes_in_buf - bytes_to_be_sent;

                        if(bytes_in_buf > 0)
                        {
                            memmove((VOID *)dfu->data_buffer,
                                    (const VOID *)(dfu->data_buffer +
                                                        bytes_to_be_sent),
                                    bytes_in_buf);
                        }
                    }
                    /* Reset the bytes_read variable. */
                    bytes_read = DFU_RESET;
                }
            }while(DFUF_DATA_XFER_PENDING == data_pending &&
                                                            buf_len > 0);
            if(NU_SUCCESS != status)
            {
                if (dfu->dfu_usr_callback->DFU_Data_Xfer_Callbk != NU_NULL)
                {
                      status = dfu->dfu_usr_callback->
                               DFU_Data_Xfer_Callbk(dfu,
                                      dfu->user_context,
                                      dfu->data_buffer,
                                      0,
                                      &bytes_read,
                                      &data_pending,
                                      DFUF_DATA_XFER_FAILED,
                                      DFUF_FW_UPLOAD);
                }

                /* If for some reason, the firmware handler fails to
                 * provide the firmware data then stall the pipe.
                 */
                DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_UNKNOWN);
            }
            else
            {
                if(DFUF_DATA_XFER_DONE == data_pending)
                {
                    dfu->dfu_state = DFU_SPEC_STATE_DFU_IDLE;

                    if (dfu->dfu_usr_callback->DFU_Data_Xfer_Callbk != NU_NULL)
                    {

                          status = dfu->dfu_usr_callback->
                                   DFU_Data_Xfer_Callbk(dfu,
                                          dfu->user_context,
                                          dfu->data_buffer,
                                          0,
                                          &bytes_read,
                                          &data_pending,
                                          DFUF_DATA_XFER_SUCCESS,
                                          DFUF_FW_UPLOAD);
                    }
                }
            }
        }
        else
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

            /* Return a valid status. */
            status = NU_SUCCESS;
        }
    }
    else if(DFU_SPEC_GETSTATUS == setup->bRequest)
    {
        /* Get the poll time out from the firmware handler and filled in
         * the 3 byte array of poll time out member of the get status pay
         * load structure.
         */
        DFUF_Get_Poll_Tm_Out(dfu, &dnld_busy_tmout);

        /* call the callback to prepare the GetStatus payload to send
         * to the host.
         */
        status = DFUF_Send_Get_Status_Payload(dfu);
        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
    }
    else if(DFU_SPEC_GETSTATE == setup->bRequest)
    {
        /* Return the DFU state by submitting an IRP. */
        status = DFUF_Send_Get_State(dfu);
        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
    }
    else if(DFU_SPEC_ABORT == setup->bRequest)
    {
        /* When Abort is received in the dfuIDLE state, do nothing. */
        status = NU_SUCCESS;
    }
    else
    {
        /* Receipt of any other class specific request, stall the control
         * pipe.
         */
        DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

        /* Update the status indicating that invalid request received */
        status = NU_INVALID_ENTRY;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_State_Dfu_Dnload_Sync
*
* DESCRIPTION
*
*   This function is called from the _NU_USBF_DFU_New_Setup
*   function. This function handles the request received by the DFU when the
*   DFU is in dfuDNLOAD-SYNC state.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS
*   NU_USB_INVLD_ARG
*
**************************************************************************/
STATUS DFUF_State_Dfu_Dnload_Sync(NU_USBF_DFU *dfu)
{
    NU_USB_SETUP_PKT *setup;
    UNSIGNED dnld_busy_tmout;
    STATUS status = NU_SUCCESS;

    setup = &dfu->setup_pkt;

    if(DFU_SPEC_GETSTATUS == setup->bRequest)
    {
        /* Check if the firmware block is being written to the non-volatile
         * memory by the device. If so then enter the dfu state as dfuDNBSY.
         * If the device is done with writing to the non-volatile memory
         * then enter the dfu state as dfuDNLOAD_IDLE.
         */
        if(DFUF_DNLOAD_IN_PROGRESS == dfu->dfu_dnld_blk_xfer_status)
        {
            dfu->dfu_state = DFU_SPEC_STATE_DFU_DNBUSY;
        }
        else if (DFUF_DNLOAD_COMPLETED == dfu->dfu_dnld_blk_xfer_status)
        {
            dfu->dfu_state = DFU_SPEC_STATE_DFU_DNLOAD_IDLE;
            dfu->dfu_dnld_blk_xfer_status = DFUF_DNLOAD_IDLE;
            status = NU_Control_Timer(&dfu->dfu_dnload_busy_timer,
                                                   NU_DISABLE_TIMER);
        }

        /* Get the poll time out from the firmware handler and filled in
         * the 3 byte array of poll time out member of the get status pay
         * load structure.
         */
        DFUF_Get_Poll_Tm_Out(dfu, &dnld_busy_tmout);

        /* call the callback to prepare the GetStatus payload to send
         * to the host.
         */
        status = DFUF_Send_Get_Status_Payload(dfu);
        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
        else
        {
            /* If the block is in progress then start the timer counting for
             * the PollTimeOut period. Once the timer expires, in the timer
             * routine it sets the dfu state as dfuDNLOAD SYNC and
             * dfu_dnld_blk_xfer_status as DFUF_DNLOAD_COMPLETED.
             */
             if(DFUF_DNLOAD_IN_PROGRESS == dfu->dfu_dnld_blk_xfer_status)
             {
                /* Start the download busy timer */
                status = NU_Reset_Timer(&dfu->dfu_dnload_busy_timer,
                                        DFU_Dnload_Bsy_Tmr,
#if (PLUS_VERSION_COMP > PLUS_1_15)
                                        ((dnld_busy_tmout * NU_PLUS_TICKS_PER_SEC)/1000),
#else
                                        ((dnld_busy_tmout * NU_PLUS_Ticks_Per_Second)/1000),
#endif
                                        0,
                                        NU_ENABLE_TIMER);
                if(NU_SUCCESS != status)
                {
                    while(0);
                }
            }
        }
    }
    else if(DFU_SPEC_GETSTATE == setup->bRequest)
    {
        /* Return the DFU state by submitting an IRP. */
        status = DFUF_Send_Get_State(dfu);
        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
    }
    else
    {
        /* Receipt of any other class specific request, stall the control
         * pipe.
         */
        DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

        /* Update the status indicating that invalid request received */
        status = NU_INVALID_ENTRY;

        /* Reset the block number variable to 1 as the some error is
         * detected.
         */
        dfu->blk_num = DFU_SET;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_State_Dfu_Dnload_Busy
*
* DESCRIPTION
*
*   This function is called from the _NU_USBF_DFU_New_Setup
*   function. This function handles the request received by the DFU when the
*   DFU is in dfuDNLOAD-BUSY state.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS
*   NU_USB_INVLD_ARG
*
**************************************************************************/
STATUS DFUF_State_Dfu_Dnload_Busy(NU_USBF_DFU *dfu)
{
    NU_USB_SETUP_PKT *setup;
    STATUS status;

    setup = &dfu->setup_pkt;

    if(DFUF_DNLOAD_IN_PROGRESS == dfu->dfu_dnld_blk_xfer_status ||
        DFU_SPEC_GETSTATUS != setup->bRequest)
    {
        /* Request is received while the device is busy writing the
         * volatile-memory, Stall the control pipe.
         */
        DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

        /* Update the status indicating that invalid request received */
        status = NU_INVALID_ENTRY;

        /* Reset the block number variable to 1 as some error is detected.*/
        dfu->blk_num = DFU_SET;
    }
    else if(DFU_SPEC_GETSTATUS == setup->bRequest)
    {
        status = NU_Control_Timer(&dfu->dfu_dnload_busy_timer,
                                              NU_DISABLE_TIMER);

        /* call the callback to prepare the GetStatus payload to send
         * to the host.
         */
        status = DFUF_Send_Get_Status_Payload(dfu);

        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
    }
    else
    {
        /* Receipt of any other class specific request, stall the control
         * pipe.
         */
        DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

        /* Update the status indicating that invalid request received */
        status = NU_INVALID_ENTRY;

        /* Reset the block number variable to 1 as the some error is
         * detected.
         */
        dfu->blk_num = DFU_SET;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_State_Dfu_Dnload_Idle
*
* DESCRIPTION
*
*   This function is called from the _NU_USBF_DFU_New_Setup
*   function. This function handles the request received by the DFU when the
*   DFU is in dfuDNLOAD-IDLE state.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS
*   NU_USB_INVLD_ARG
*
**************************************************************************/
STATUS DFUF_State_Dfu_Dnload_Idle(NU_USBF_DFU *dfu)
{
    NU_USB_SETUP_PKT *setup;
    STATUS status;
    UINT32 bytes_read;
    UINT8 data_pending;
    UINT32 dnld_busy_tmout;

    setup = &dfu->setup_pkt;

    if(DFU_SPEC_DNLOAD == setup->bRequest)
    {
        /* Check if the received block number is valid, if not then stall
         * the pipe.
         */
        if(setup->wValue != dfu->blk_num)
        {
            /* Device stalls the pipe if the block number invalid. */
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

            /* Update the status indicating that invalid request received */
            status = NU_INVALID_ENTRY;

            /* Reset the block number variable to 1 as the some error is
             * detected.
             */
            dfu->blk_num = DFU_SET;

            return status;
        }
        else if(DFU_RESET < setup->wLength)
        {
            if(DFUF_WTRANSFER_SIZE >= setup->wLength)
            {
                dfu->data_len = setup->wLength;
            }
            else
            {
                /* When the wLength is greater than wTransferSize of
                 * the functional descriptor, than device stalls the pipe.
                 */
                DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

                /* Update the status indicating that invalid request received */
                status = NU_INVALID_ENTRY;

                /* Reset the block number variable to 1 as the some error is
                 * detected.
                 */
                dfu->blk_num = DFU_SET;

                return status;
            }

            /* Increment the block number, which will become the next
             * expected block number for the transfer.
             */
            dfu->blk_num++;

            /* Set the block xfer variable as download in progress,
             * indicating the device is writing to the non_volatile memory.
             */
            dfu->dfu_dnld_blk_xfer_status = DFUF_DNLOAD_IN_PROGRESS;

            /* Submit IRP request to receive the firmware data in the buffer
             * or to receive the next getstatus request of the wLength is
             * zero. wLength value zero indicates the end of firmware
             * download.
             * Note:
             * 1) what is the optimal value for the buffer length here?
             * 2) If the buffer length is set more than the current 256,
             *    then UINT8 array cmd_buffer should be made as pointer and
             *    memory should be allocated using the NU_Allocate.
             */
            status = NU_USB_IRP_Create ( &dfu->ctrl_irp,
                                dfu->data_len,
                                dfu->data_buffer,
                                NU_TRUE,
                                NU_FALSE,
                                DFUF_Ctrl_Data_IRP_Cmplt, (VOID *)dfu,
                                DFU_RESET);

            if(NU_SUCCESS == status)
            {
                status = NU_USB_PIPE_Submit_IRP(dfu->control_pipe,
                                                        &dfu->ctrl_irp);
            }

            if(NU_SUCCESS != status)
            {
                DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
            }
        }
        else if(DFU_RESET == setup->wLength)
        {
            /* Reset the block number variable to 1 as the download is
             * complete.
             */
            dfu->blk_num = DFU_SET;

            /* Give a call back to the firmware handler indicating that
             * firmware download is complete.
             */
            status = dfu->dfu_usr_callback->
                                DFU_Data_Xfer_Callbk(dfu,
                                dfu->user_context,
                                dfu->data_buffer,
                                DFU_RESET,
                                &bytes_read,
                                &data_pending,
                                DFUF_DATA_XFER_SUCCESS,
                                DFUF_FW_DNLOAD);
            if(DFU_SPEC_STATUS_OK != dfu->dfu_status)
            {
                /* Update the user about the error detected in the
                 * firmware download.
                 */
                DFUF_Data_Xfer_Status_Error(dfu);
                return NU_NO_MEMORY;
            }

            if(NU_SUCCESS == status)
            {
                /* Host is informing the device that there is no more data
                 * to download. DFU status and status string table index is
                 * set by the firmware handler by calling the set_status of
                 *  the dfu.
                 */
                dfu->dfu_state = DFU_SPEC_STATE_DFU_MNFST_SYNC;
                dfu->dfu_mnfst_status = DFUF_MNFST_IN_PROGRESS;
            }
            else
            {
                dfu->dfu_state = DFU_SPEC_STATE_DFU_ERR;
            }
        }
    }
    else if(DFU_SPEC_ABORT == setup->bRequest)
    {
        /* Abort request is received, bring the dfu state to the dfu IDLE
         * state and set the status as OK.
         */
        dfu->dfu_state = DFU_SPEC_STATE_DFU_IDLE;
        dfu->dfu_status = DFU_SPEC_STATUS_OK;
        dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX0;

        /* Reset the block number variable to 1 as the download is aborted.
         */
        dfu->blk_num = DFU_SET;

        /* Return a valid status. */
        status = NU_SUCCESS;
    }
    else if(DFU_SPEC_GETSTATUS == setup->bRequest)
    {
        /* Get the poll time out from the firmware handler and filled in
         * the 3 byte array of poll time out member of the get status pay
         * load structure.
         */
        DFUF_Get_Poll_Tm_Out(dfu, &dnld_busy_tmout);

        /* call the callback to prepare the GetStatus payload to send
         * to the host.
         */
        status = DFUF_Send_Get_Status_Payload(dfu);
        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
    }
    else if(DFU_SPEC_GETSTATE == setup->bRequest)
    {
        /* Return the DFU state by submitting an IRP. */
        status = DFUF_Send_Get_State(dfu);
        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
    }
    else
    {
        /* Receipt of any other class specific request, stall the control
         * pipe.
         */
        DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

        /* Update the status indicating that invalid request received. */
        status = NU_INVALID_ENTRY;

        /* Reset the block number variable to 1 as the some error is
         * detected.
         */
        dfu->blk_num = DFU_SET;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_State_Dfu_Mnfst_Sync
*
* DESCRIPTION
*
*   This function is called from the _NU_USBF_DFU_New_Setup
*   function. This function handles the request received by the DFU when the
*   DFU is in dfuMANIFEST-SYNC state.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS
*   NU_INVALID_ENTRY
*
**************************************************************************/
STATUS DFUF_State_Dfu_Mnfst_Sync(NU_USBF_DFU *dfu)
{
    NU_USB_SETUP_PKT *setup;
    UNSIGNED dnld_busy_tmout;
    STATUS status;

    setup = &dfu->setup_pkt;

    if(DFU_SPEC_GETSTATUS == setup->bRequest)
    {
        if(DFUF_MNFST_IN_PROGRESS == dfu->dfu_mnfst_status)
        {
            dfu->dfu_state = DFU_SPEC_STATE_DFU_MNFST;
        }
        else if((DFUF_MNFST_COMPLETED == dfu->dfu_mnfst_status) &&
            (DFUF_RTM_FUN_DES_BMATTRIBUTES &
            DFU_FUN_DES_BITMNFST_TLRNT_TRUE) )
        {
            dfu->dfu_state = DFU_SPEC_STATE_DFU_IDLE;
            dfu->dfu_mnfst_status = DFUF_MNFST_IDLE;
            status = NU_Control_Timer(&dfu->dfu_mnfst_sync_timer,
                                                   NU_DISABLE_TIMER);
        }
        /* Get the poll time out from the firmware handler and filled in
         * the 3 byte array of poll time out member of the get status pay
         * load structure.
         */
        DFUF_Get_Poll_Tm_Out(dfu, &dnld_busy_tmout);

        /* call the callback to prepare the GetStatus payload to send
         * to the host.
         */
        status = DFUF_Send_Get_Status_Payload(dfu);
        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
        else
        {
            if(DFUF_MNFST_IN_PROGRESS == dfu->dfu_mnfst_status)
            {
                /* Start the manifestation sync timer */
                status = NU_Reset_Timer( &dfu->dfu_mnfst_sync_timer,
                                         DFU_Mnfst_Sync_Tmr,
#if (PLUS_VERSION_COMP > PLUS_1_15)
                                        ((dnld_busy_tmout * NU_PLUS_TICKS_PER_SEC)/1000),
#else
                                        ((dnld_busy_tmout * NU_PLUS_Ticks_Per_Second)/1000),
#endif
                                         0,
                                         NU_ENABLE_TIMER);
                if(NU_SUCCESS != status)
                {
                    while(0);
                }
            }
        }
    }
    else if(DFU_SPEC_GETSTATE == setup->bRequest)
    {
        /* Return the DFU state by submitting an IRP. */
        status = DFUF_Send_Get_State(dfu);
        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
    }
    else
    {
        /* Receipt of any other class specific request, stall the control
         * pipe.
         */
        DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

        /* Update the status indicating that invalid request received. */
        status = NU_INVALID_ENTRY;

    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_State_Dfu_Mnfst
*
* DESCRIPTION
*
*   This function is called from the _NU_USBF_DFU_New_Setup
*   function. This function handles the request received by the DFU when the
*   DFU is in dfuMANIFEST state.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_INVALID_ENTRY
*
**************************************************************************/
STATUS DFUF_State_Dfu_Mnfst(NU_USBF_DFU *dfu)
{
    /* If any request is received from the host when the dfu is in the
     * manifestation  state the stall the pipe.
     */
    DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

    return NU_INVALID_ENTRY;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_State_Dfu_Mnfst_wt_Reset
*
* DESCRIPTION
*
*   This function is called from the _NU_USBF_DFU_New_Setup
*   function. This function handles the request received by the DFU when the
*   DFU is in dfuMANIFEST-WAIT-RESET state.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS
*
**************************************************************************/
STATUS DFUF_State_Dfu_Mnfst_wt_Reset(NU_USBF_DFU *dfu)
{
    STATUS status = NU_SUCCESS;

    /* When in this state, if received any request from the host then do
     * nothing. When the device reaches this state, then it has to wait and
     * receive the RESET for re-enumeration. Other this device cannot do
     * anything.
     */
    return status;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_State_Dfu_Upload_Idle
*
* DESCRIPTION
*
*   This function is called from the _NU_USBF_DFU_New_Setup
*   function. This function handles the request received by the DFU when the
*   DFU is in dfuUPLOAD-IDLE state.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS
*   NU_USB_INVLD_ARG
*
**************************************************************************/
STATUS DFUF_State_Dfu_Upload_Idle(NU_USBF_DFU *dfu)
{
    NU_USB_SETUP_PKT *setup;
    STATUS status;
    UINT32 bytes_read;
    UINT8 data_pending;
    UINT32 buf_len;
    UINT32 dnld_busy_tmout;
    UINT32 bytes_in_buf;
    UINT32 mod_buf_len;
    UINT32 bytes_to_be_sent;

    bytes_read = DFU_RESET;

    setup = &dfu->setup_pkt;

    if(DFU_SPEC_UPLOAD == setup->bRequest)
    {
        /* Check if the received block number, wValue, is valid, if
         * not then stall the pipe.
         */
        if((setup->wValue != dfu->blk_num) ||
            (DFUF_WTRANSFER_SIZE < setup->wLength))
        {
            /* Device stalls the pipe if the block number/data length
             * is invalid.
             */
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

            /* Update the status indicating that invalid request received.*/
            status = NU_INVALID_ENTRY;

            /* Reset the block number variable to 1 as the some error is
             * detected.
             */
            dfu->blk_num = DFU_SET;

            return status;
        }

        dfu->data_len = setup->wLength;

        /* Increment the block number, which will be the next expected
         * block number for the transfer.
         */
        dfu->blk_num++;

        /* Give a call back to the firmware handler to get the firmware
         * to be transferred to the host.
         */
        buf_len = dfu->data_len;
        bytes_in_buf = DFU_RESET;
        do
        {
            status = dfu->dfu_usr_callback->
                                DFU_Data_Xfer_Callbk(dfu,
                                dfu->user_context,
                                (dfu->data_buffer + bytes_in_buf),
                                buf_len,
                                &bytes_read,
                                &data_pending,
                                DFUF_DATA_XFER_IN_PROGRESS,
                                DFUF_FW_UPLOAD);
            if(NU_SUCCESS != status)
            {
                /* If the callback fails, break from the do while loop.
                 */
                break;
            }
            else if(DFU_SPEC_STATUS_OK != dfu->dfu_status)
            {
                /* Update the user about the error detected in the
                 * firmware upload.
                 */
                DFUF_Data_Xfer_Status_Error(dfu);
                return NU_NO_MEMORY;
            }
            else
            {
                buf_len = buf_len - bytes_read;
                bytes_in_buf = bytes_in_buf + bytes_read;
                mod_buf_len = bytes_in_buf %
                                    dfu->max_packet_size;
                if(mod_buf_len > 0)
                {
                    if(bytes_in_buf <= dfu->max_packet_size)
                    {
                        bytes_to_be_sent = bytes_in_buf;
                    }
                    else
                    {
                        if(DFUF_DATA_XFER_DONE == data_pending ||
                                0 == buf_len)
                        {
                            bytes_to_be_sent = bytes_in_buf;
                        }
                        else
                        {
                            bytes_to_be_sent = bytes_in_buf -
                                                        mod_buf_len;
                        }
                    }
                }
                else if(mod_buf_len == 0)
                {
                    bytes_to_be_sent = bytes_in_buf;
                }

                if((dfu->max_packet_size <= bytes_to_be_sent) ||
                    (dfu->max_packet_size > bytes_to_be_sent &&
                    data_pending == DFUF_DATA_XFER_DONE))
                {
                    /* Submit IRP request to transfer the firmware data
                     * to the host.
                     */
                    status = NU_USB_IRP_Create ( &dfu->ctrl_irp,
                                        bytes_to_be_sent,
                                        dfu->data_buffer,
                                        NU_TRUE,
                                        NU_FALSE,
                                        DFUF_Ctrl_Data_IRP_Cmplt,
                                        (VOID *)dfu,
                                        DFU_RESET);

                    if(NU_SUCCESS == status)
                    {
                        status = NU_USB_PIPE_Submit_IRP(dfu->control_pipe,
                                                            &dfu->ctrl_irp);
                    }

                    if(NU_SUCCESS != status)
                    {
                        /* If the submission of the IRP fails then
                         * break from the do while loop and stall the
                         * control pipe.
                         */
                        break;
                    }
                }

                if(dfu->max_packet_size <= bytes_to_be_sent ||
                   data_pending == DFUF_DATA_XFER_DONE)
                {
                    bytes_in_buf = bytes_in_buf - bytes_to_be_sent;

                    if(bytes_in_buf > 0)
                    {
                        memmove((VOID *)dfu->data_buffer,
                     (const VOID *)(dfu->data_buffer + bytes_to_be_sent),
                         bytes_in_buf);
                    }
                }
                /* Reset the bytes_read variable. */
                bytes_read = DFU_RESET;
            }
        }while(DFUF_DATA_XFER_PENDING == data_pending &&
                                                        buf_len > 0);

        if(NU_SUCCESS != status)
        {
            if(dfu->dfu_usr_callback->DFU_Data_Xfer_Callbk != NU_NULL)
            {
                   status = dfu->dfu_usr_callback->
                                  DFU_Data_Xfer_Callbk(dfu,
                                  dfu->user_context,
                                  dfu->data_buffer,
                                  0,
                                  &bytes_read,
                                  &data_pending,
                                  DFUF_DATA_XFER_FAILED,
                                  DFUF_FW_UPLOAD);
            }

            /* If for some reason, the firmware handler fails to
             * provide the firmware data then stall the pipe.
             */
            NU_USB_PIPE_Stall(dfu->control_pipe);

            /* The next state of the DFU changes to dfuERROR. */
            dfu->dfu_state = DFU_SPEC_STATE_DFU_ERR;
            dfu->dfu_status = DFU_SPEC_STATUS_ERR_UNKNOWN;
            dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX14;

            /* Reset the block number variable to 1 as the some error is
             * detected.
             */
            dfu->blk_num = DFU_SET;
        }
        else
        {
            if(DFUF_DATA_XFER_DONE == data_pending)
            {
                dfu->dfu_state = DFU_SPEC_STATE_DFU_IDLE;

                /* Reset the block number variable to 1. */
                dfu->blk_num = DFU_SET;
                if (dfu->dfu_usr_callback->DFU_Data_Xfer_Callbk != NU_NULL)
                {
                       status = dfu->dfu_usr_callback->
                               DFU_Data_Xfer_Callbk(dfu,
                                      dfu->user_context,
                                      dfu->data_buffer,
                                      0,
                                      &bytes_read,
                                      &data_pending,
                                      DFUF_DATA_XFER_SUCCESS,
                                      DFUF_FW_UPLOAD);
                }
            }
        }
    }
    else if(DFU_SPEC_ABORT == setup->bRequest)
    {
        /* Abort request is received, bring the dfu state to the dfu IDLE
         * state and set the status as OK.
         */
        dfu->dfu_state = DFU_SPEC_STATE_DFU_IDLE;
        dfu->dfu_status = DFU_SPEC_STATUS_OK;
        dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX0;

        /* Reset the block number variable to 1 as the download is aborted.
         */
        dfu->blk_num = DFU_SET;

        /* Return a valid status. */
        status = NU_SUCCESS;
    }
    else if(DFU_SPEC_GETSTATUS == setup->bRequest)
    {
        /* Get the poll time out from the firmware handler and filled in
         * the 3 byte array of poll time out member of the get status pay
         * load structure.
         */
        DFUF_Get_Poll_Tm_Out(dfu, &dnld_busy_tmout);
        /* call the callback to prepare the GetStatus payload to send
         * to the host.
         */
        status = DFUF_Send_Get_Status_Payload(dfu);
        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
    }
    else if(DFU_SPEC_GETSTATE == setup->bRequest)
    {
        /* Return the DFU state by submitting an IRP. */
        status = DFUF_Send_Get_State(dfu);
        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
    }
    else
    {
        /* If bitCanUpload is false, then stall the control pipe. */
        DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

        /* Update the status indicating that invalid request received.*/
        status = NU_INVALID_ENTRY;

        /* Reset the block number variable to 1 as the some error is
         * detected.
         */
        dfu->blk_num = DFU_SET;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_State_Dfu_Error
*
* DESCRIPTION
*
*   This function is called from the _NU_USBF_DFU_New_Setup
*   function. This function handles the request received by the DFU when the
*   DFU is in dfuERROR state.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS
*   NU_USB_INVLD_ARG
*
**************************************************************************/
STATUS DFUF_State_Dfu_Error(NU_USBF_DFU *dfu)
{
    NU_USB_SETUP_PKT *setup;
    STATUS status;
    UINT32 dnld_busy_tmout;

    setup = &dfu->setup_pkt;
    status = NU_SUCCESS;

    if(DFU_SPEC_GETSTATUS == setup->bRequest)
    {
        /* Get the poll time out from the firmware handler and filled in
         * the 3 byte array of poll time out member of the get status pay
         * load structure.
         */
        DFUF_Get_Poll_Tm_Out(dfu, &dnld_busy_tmout);
        /* call the callback to prepare the GetStatus payload to send
         * to the host.
         */
        status = DFUF_Send_Get_Status_Payload(dfu);
        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
    }
    else if(DFU_SPEC_GETSTATE == setup->bRequest)
    {
        /* Return the DFU state by submitting an IRP. */
        status = DFUF_Send_Get_State(dfu);
        if(NU_SUCCESS != status)
        {
            DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);
        }
    }
    else if(DFU_SPEC_CLRSTATUS == setup->bRequest)
    {
        /* The next state of the DFU changes to dfuIDLE. */
        dfu->dfu_state = DFU_SPEC_STATE_DFU_IDLE;
        dfu->dfu_status = DFU_SPEC_STATUS_OK;
        dfu->dfu_status_str_index = DFU_STATUS_TABLE_STR_INDX0;

        /* Reset the block number variable to 1 as the some error is
         * detected.
         */
        dfu->blk_num = DFU_SET;
    }
    else
    {
        /* Receipt of any other request, stall the control pipe. */
        DFUF_Stall_Ctrl_Pipe(dfu, DFU_SPEC_STATUS_ERR_STALLEEDPKT);

        /* Reset the block number variable to 1 as the some error is
         * detected.
         */
        dfu->blk_num = DFU_SET;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_Send_Get_State
*
* DESCRIPTION
*
*   This function creates and submits an IRP to transfer the DFU state to
*   the host.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   None
*
**************************************************************************/
STATUS DFUF_Send_Get_State(NU_USBF_DFU *dfu)
{
    STATUS status;
    /* Return the DFU state by submitting an IRP. */
    status = NU_USB_IRP_Create ( &dfu->ctrl_irp,
            sizeof(dfu->dfu_state),
            &dfu->dfu_state,
            NU_TRUE,
            NU_FALSE,
            DFUF_Ctrl_Data_IRP_Cmplt,(VOID *)dfu,
            DFU_RESET);

    if(NU_SUCCESS == status)
    {
        status = NU_USB_PIPE_Submit_IRP (dfu->control_pipe, &dfu->ctrl_irp);
    }
    return status;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_Data_Xfer_Status_Error
*
* DESCRIPTION
*
*   This function updates the user about the detected error and stalls the
*   control end point.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   None
*
**************************************************************************/
VOID DFUF_Data_Xfer_Status_Error(NU_USBF_DFU *dfu)
{
    /* Update the user about the error detected in the
     * firmware upload.
     */
     if (dfu->dfu_usr_callback->DFU_Status_Update_Callbk != NU_NULL)
    {
        dfu->dfu_usr_callback->
            DFU_Status_Update_Callbk(dfu->user_context, dfu->dfu_status);
    }

    /* Stall the pipe. */
    NU_USB_PIPE_Stall(dfu->control_pipe);

    /* The next state of the DFU changes to dfuERROR. */
    dfu->dfu_state = DFU_SPEC_STATE_DFU_ERR;
}

/**************************************************************************
* FUNCTION
*
*   DFUF_Stall_Ctrl_Pipe
*
* DESCRIPTION
*
*   This function is used to stall the control endpoint when ever the error
*   is detected.
*
* INPUTS
*
*   dfu                  DFU control block
*   dfu_status           status of the device
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   none
*
**************************************************************************/
VOID DFUF_Stall_Ctrl_Pipe(NU_USBF_DFU *dfu, UINT8 dfu_status)
{

    /* The next state of the DFU changes to dfuERROR.
     *
     * Note: status table doesn't have a specific error for upload
     * or download not supported. So we are setting it to the
     * stalled packet error condition.
     */
    dfu->dfu_state = DFU_SPEC_STATE_DFU_ERR;
    dfu->dfu_status = dfu_status;
    dfu->dfu_status_str_index = dfu_status;
    /* If bitCanUpload is false, then stall the control pipe. */
    NU_USB_PIPE_Stall(dfu->control_pipe);
}

/**************************************************************************
* FUNCTION
*
*   DFUF_Send_Get_Status_Payload
*
* DESCRIPTION
*
*   This function is used to transfer the DFU status payload to the host
*   when GET_STATUS request is received by the device.
*
* INPUTS
*
*   dfu                  DFU control block
*
* OUTPUTS
*
*   None.
*
* RETURNS
*
*   NU_SUCCESS
*
**************************************************************************/
STATUS DFUF_Send_Get_Status_Payload(NU_USBF_DFU *dfu)
{
    STATUS status;

    /* Prepare the GetStatus payload to send to the host. */
    dfu->dfu_get_status_payload.dfu_status  = dfu->dfu_status;
    dfu->dfu_get_status_payload.dfu_state   = dfu->dfu_state;
    dfu->dfu_get_status_payload.dfu_istring = dfu->dfu_status_str_index;

    status = NU_USB_IRP_Create ( &dfu->ctrl_irp,
                        sizeof(DFU_GETSTATUS_PAYLOAD),
                        (UINT8 *)&dfu->dfu_get_status_payload,
                        NU_TRUE,
                        NU_FALSE,
                        DFUF_Ctrl_Data_IRP_Cmplt,(VOID *)dfu,
                        DFU_RESET);

    if(NU_SUCCESS == status)
    {
        status = NU_USB_PIPE_Submit_IRP (dfu->control_pipe, &dfu->ctrl_irp);
    }

    return status;
}
/*============================  End Of File  ===========================*/
