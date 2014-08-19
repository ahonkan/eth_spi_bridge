/*************************************************************************
*
*               Copyright 2009 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       dbg_rsp.c
*
*   COMPONENT
*
*       Debug Agent - Remote Serial Protocol (RSP)
*
*   DESCRIPTION
*
*       This file contains the C functions source code for the RSP Support
*       Component. This component is responsible for handling and
*       responding to all RSP packet requests from client. It also
*       provides API funtion-ality to transmit asynchronous notification
*       events to the client.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       rsp_create_error_msg
*       rsp_create_response_msg
*       rsp_thread_timer_exp
*       rsp_create_stop_notify_packet
*       RSP_Initialize
*       RSP_Process_Packet
*       RSP_Com_Error_Notify
*       RSP_Com_Disconnect_Notify
*       RSP_Send_Notification
*       rsp_session_reset
*       rsp_memory_read_handler
*       rsp_memory_write_handler
*       rsp_register_group_read_handler
*       rsp_register_group_write_handler
*       rsp_register_read_handler
*       rsp_register_write_handler
*       rsp_resume_target_handler
*       rsp_single_step_handler
*       rsp_halt_target_handler
*       rsp_insert_breakpoint_handler
*       rsp_clear_breakpoint_handler
*       rsp_general_query_get_handler
*       rsp_reason_target_stopped_handler
*       rsp_disconnect_handler
*       rsp_qfirst_thread_handler
*       rsp_qsubsequent_thread_handler
*       rsp_set_thread_handler
*       rsp_set_exec_thread_id_handler
*       rsp_set_other_thread_id_handler
*       rsp_get_current_thread_handler
*       rsp_get_current_thread
*       rsp_v_packet_handler
*       rsp_vcont_handler
*       rsp_vcont_parse
*       rsp_vcont_continue_handler
*       rsp_vcont_step_handler
*       rsp_vcont_stop_handler
*       rsp_vstopped_handler
*       rsp_thread_alive_handler
*       rsp_q_packet_handler
*       rsp_extended_mode_handler
*       rsp_vrun_handler
*       rsp_vrun_get_argc
*       rsp_vrun_get_argv
*       rsp_vkill_handler
*       rsp_shutdown
*       rsp_thread_extra_info_handler
*       rsp_offsets_handler
*       rsp_default_handler
*       RSP_Check_Packet
*
*   DEPENDENCIES
*
*       dbg.h
*
*************************************************************************/

/* Include files */

#include    "services/dbg.h"

/* Global variables */

#ifdef CFG_NU_OS_SVCS_TRACE_ENABLE

UINT32 DBG_Trace_Mask;

#endif /* CFG_NU_OS_SVCS_TRACE_ENABLE */

/* RSP Command Table */

RSP_STATUS         (*RSP_Packet_Handler[MAX_RSP_CMDS])(CHAR *    p_rsp_cmd_buff, CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);

/* RSP Registration Callback Table */

RSP_STATUS         (*RSP_Registration_Handler[RSP_CALLBACK_END])(RSP_CALLBACK* pRspCallback);

/* RSP callback context array. */

VOID *             RspCallbackContextArray[RSP_CALLBACK_END];

/* RSP packet management control block */

RSP_CB             rsp_pkt_mgmt;

/* External global variables privately imported */

extern DBG_CB *     DBG_p_cb;

/* Local Function prototypes */

/* Prototype */
static  RSP_STATUS  rsp_create_error_msg(RSP_ERROR_TYPE   error_type,
                                         CHAR *           p_rsp_resp_buff,
                                         UINT *           p_rsp_resp_size);
static RSP_STATUS   rsp_create_response_msg(RSP_STATUS      resp_status,
                                            CHAR *          p_rsp_resp_buff,
                                            UINT *          p_rsp_resp_size);
static  VOID        rsp_thread_timer_exp(VOID *           context);
static  RSP_STATUS  rsp_create_stop_notify_packet(DBG_THREAD_ID       thread_id,
                                                  CHAR *              prefix_str,
                                                  CHAR *              p_rsp_pkt,
                                                  UINT *      p_rsp_pkt_size);
static  RSP_STATUS  rsp_session_reset(VOID);
static  RSP_STATUS  rsp_memory_read_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_memory_write_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_register_group_read_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_register_group_write_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_register_read_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_register_write_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_resume_target_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_single_step_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_halt_target_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_insert_breakpoint_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_clear_breakpoint_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_general_query_get_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_reason_target_stopped_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_disconnect_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_default_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_qfirst_thread_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_qsubsequent_thread_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_set_thread_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_set_exec_thread_id_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_set_other_thread_id_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_get_current_thread_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static  RSP_STATUS  rsp_get_current_thread(DBG_THREAD_ID*  cur_thread_id);

static  RSP_STATUS  rsp_v_packet_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR * p_rsp_resp_buff, UINT * rsp_resp_size);
static RSP_STATUS   rsp_vcont_handler(CHAR *     p_rsp_cmd_buff,
                                     CHAR *     p_rsp_resp_buff,
                                     UINT *     p_rsp_resp_size);
static RSP_STATUS   rsp_vcont_parse(CHAR *           p_rsp_cmd_buff,
                                    DBG_THREAD_ID    exec_thread_id);
static RSP_STATUS   rsp_vcont_continue_handler(DBG_THREAD_ID     thread_id);
static RSP_STATUS   rsp_vcont_step_handler(DBG_THREAD_ID     thread_id);
static RSP_STATUS   rsp_vcont_stop_handler(DBG_THREAD_ID     thread_id);
static  RSP_STATUS  rsp_vstopped_handler(CHAR *  p_rsp_cmd_buff, CHAR *   p_rsp_resp_buff,\
                        UINT * p_rsp_resp_size);
static  RSP_STATUS  rsp_thread_alive_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size);
static  RSP_STATUS  rsp_q_packet_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size);
static  RSP_STATUS  rsp_extended_mode_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size);
static  RSP_STATUS  rsp_vrun_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size);
static  RSP_STATUS  rsp_vrun_get_argc(CHAR *  p_rsp_pkt, INT * p_argc);

static  RSP_STATUS  rsp_vrun_get_argv(CHAR * p_rsp_pkt, CHAR * p_vrun_data, CHAR ** p_argv, INT argc);

static  RSP_STATUS  rsp_vkill_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size);
static  RSP_STATUS  rsp_shutdown(VOID);

static  RSP_STATUS  rsp_thread_extra_info_handler(CHAR *  p_rsp_cmd_buff,\
                        CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size);
static  RSP_STATUS  rsp_offsets_handler(CHAR *   p_rsp_resp_buff,\
                        UINT * p_rsp_resp_size);

/*************************************************************************
*
*   FUNCTION
*
*       rsp_create_error_msg
*
*   DESCRIPTION
*
*       This function creates an error message.
*
*   INPUTS
*
*       error_type - Indicates the type of error that occurred.
*
*       p_rsp_resp_buff - The response buffer.
*
*       p_rsp_resp_size - Return parameter that will be updated with the
*                         response buffer size if the operation is
*                         successful.  If the operation fails the value is
*                         undefined.
*
*   OUTPUTS
*
*       RSP_STATUS_ERROR_RESP - Indicates successful translation of the
*                               error type into an error message.
*
*       RSP_STATUS_FAILED - Indicates unable to translate error to a
*                           message successfully (failure).
*
*************************************************************************/
static RSP_STATUS rsp_create_error_msg(RSP_ERROR_TYPE   error_type,
                                       CHAR *           p_rsp_resp_buff,
                                       UINT *   p_rsp_resp_size)
{
    RSP_STATUS      rsp_status;

    CHAR *          rsp_error_str[] = {"E01",
                                       "E02",
                                       "E03",
                                       "E04",
                                       "E05",
                                       "E06",
                                       "E07"};

    /* Ensure a valid error type. */
    if (error_type <= RSP_ERROR_E07_APP)
    {
        /* Set initial function status. */
        rsp_status = RSP_STATUS_ERROR_RESP;

    }
    else
    {
        /* ERROR: Invalid error type. */
        rsp_status = RSP_STATUS_FAILED;

    }

    if (rsp_status == RSP_STATUS_ERROR_RESP)
    {
        /* Generate error message into buffer provided. */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt(rsp_error_str[error_type],
                                                     p_rsp_resp_buff);

    }

    return (rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_create_response
*
*   DESCRIPTION
*
*       This function creates a response message based on the RSP status
*       value passed in.
*
*   INPUTS
*
*       resp_status             - The (RSP) response status value.
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates (other) internal error.
*
*************************************************************************/
static RSP_STATUS   rsp_create_response_msg(RSP_STATUS      resp_status,
                                            CHAR *          p_rsp_resp_buff,
                                            UINT *          p_rsp_resp_size)
{
    RSP_STATUS      rsp_status = RSP_STATUS_OK;

    /* Create response based on response status value. */

    switch (resp_status)
    {
        case RSP_STATUS_OK :
        {
            *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",
                                                         p_rsp_resp_buff);

            break;

        }

        case RSP_STATUS_NOT_SUPPORTED :
        {
            *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("",
                                                         p_rsp_resp_buff);

            break;
        }

        case RSP_STATUS_DEBUG_CALL_FAILED :
        {
            rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);

            break;

        }

        case RSP_STATUS_INVALID_MODE :
        {
            rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);

            break;

        }

        default :
        {
            rsp_status = rsp_create_error_msg(RSP_ERROR_E01_MSG,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);

            break;

        }

    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_create_stop_notify_packet
*
*   DESCRIPTION
*
*       This function creates a stop notify packet for a specified thread.
*
*   INPUTS
*
*       thread_id - The ID of the thread to create the notify packet for.
*
*       prefix_str - The string that will prefix the body of the notify
*                    packet.
*
*       p_rsp_pkt - The location where the notify packet will be created.
*
*       p_rsp_pkt_size - Return parameter that will be updated to contain
*                        size (in bytes) of the created notify packet if
*                        the operation is successful.  If the operation
*                        fails the value is undefined.
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_ERROR_RESP - Indicates unable to get registers for
*                               thread (used to create packet).
*
*************************************************************************/
static RSP_STATUS rsp_create_stop_notify_packet(DBG_THREAD_ID       thread_id,
                                                CHAR *              prefix_str,
                                                CHAR *              p_rsp_pkt,
                                                UINT *      p_rsp_pkt_size)
{
    DBG_STATUS              dbg_status = DBG_STATUS_OK;
    RSP_STATUS              rsp_status = RSP_STATUS_OK;
    DBG_CMD                 dbg_cmd;
    UINT                    byts_read = 0;
    CHAR                    local_work_buff[RSP_MAX_PACKET_SIZE_SUPPORTED];

    /* Read expedited registers for stop reply. */
    dbg_cmd.op = DBG_CMD_OP_REGISTER_READ;
    dbg_cmd.op_param.reg_rd.thread_id = thread_id;
    dbg_cmd.op_param.reg_rd.register_id = DBG_REGISTER_ID_EXPEDITE;
    dbg_cmd.op_param.reg_rd.p_register_data = (VOID *)local_work_buff;
    dbg_cmd.op_param.reg_rd.p_actual_reg_data_size = &byts_read;

    /* Call Debug Server */
    dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id,
                                 &dbg_cmd);

    if (dbg_status == DBG_STATUS_OK)
    {
        if(rsp_pkt_mgmt.mode == RSP_MODE_ALL_STOP)
        {
            /* In All-Stop mode save the thread ID for stepping. */
            rsp_pkt_mgmt.wrk_tid_step = thread_id;

            /* Update working thread for register and memory reads and writes */
            rsp_pkt_mgmt.wrk_tid_other = thread_id;

        }

        /* Construct RSP Response */
        RSP_Expedited_Regs_2_Rsp_Stop_Reply_Pkt(prefix_str,
                                                thread_id,
                                                &local_work_buff[0],
                                                byts_read,
                                                p_rsp_pkt,
                                                p_rsp_pkt_size);

        rsp_status = RSP_STATUS_OK;

    }
    else
    {
        /* ERROR: Unable to retrieve register values for thread. */

        rsp_status = RSP_STATUS_ERROR_RESP;
    }

    return (rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_thread_timer_exp
*
*   DESCRIPTION
*
*       This function handles the expiration of the thread timer used to
*       resend thread stop notifications.
*
*   INPUTS
*
*       context - Context value set when the timer is initialized.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID rsp_thread_timer_exp(VOID *         context)
{
    RSP_STATUS              rsp_status = RSP_STATUS_OK;
    DBG_THREAD_ID           thread_id = 0;
    CHAR                    local_out_buff[RSP_MAX_PACKET_SIZE_SUPPORTED];
    UINT                    size;
    CHAR *                  p_response_str;

    /* Get the saved thread ID for the % notification packet */
    thread_id = rsp_pkt_mgmt.notify_tid;

    /* Check for a valid thread ID */
    if(thread_id > 0)
    {
        /* Stop the timer */
        rsp_status = RSP_Timer_Stop(&rsp_pkt_mgmt.thread_timer);

        if(rsp_status == RSP_STATUS_OK)
        {
            /* Reset the timer to the resend timeout. */
            rsp_status = RSP_Timer_Start(&rsp_pkt_mgmt.thread_timer, RSP_NOTIFY_PACKET_RESEND_TIMEOUT);
        }

        if(rsp_status == RSP_STATUS_OK)
        {
            /* Set the current thread. */
            rsp_pkt_mgmt.current_tid = thread_id;

            switch(rsp_pkt_mgmt.notify_sig_type)
            {
                case RSP_NOTIFY_SIGNAL_T00 :
                {
                    p_response_str = "%Stop:T00";

                    break;

                } /* case */

                case RSP_NOTIFY_SIGNAL_T05 :
                {
                    p_response_str = "%Stop:T05";

                    break;

                } /* case */

                default :
                {
                    p_response_str = "%Stop:T05";

                    break;

                } /* default */

            } /* switch */

            /* Add stop packet.  Note that threads stopped by 'vCont;t'
            packets always report 'signal 0' as reason for stop. */
            rsp_status = rsp_create_stop_notify_packet(thread_id,
                                                    p_response_str,
                                                    (CHAR *)local_out_buff,
                                                    &size);
        }

        if(rsp_status != RSP_STATUS_OK)
        {
            /* Handle Error condition */
            rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                              local_out_buff,
                                              &size);
        }

        /* Send the message. */
        DBG_COM_Send(&DBG_p_cb -> com, (VOID *)&local_out_buff[0], size);
    }

    return;
}


/*************************************************************************
*
*   FUNCTION
*
*       RSP_Initialize
*
*   DESCRIPTION
*
*       Initializes the RSP component - Initializes the global RSP control
*       block, and registers the RSP packet handlers.
*
*   INPUTS
*
*       pMemory                 - Pointer to memory allocated for the RSP Support
*                                 Component. This memory is dynamically allocated
*                                 by the calling component.
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
RSP_STATUS  RSP_Initialize(VOID *    pMemory)
{
    INT             i;
    RSP_STATUS      rsp_status = RSP_STATUS_OK;

    if(pMemory)
    {

        /* Initialize RSP output buffer pointer */
        rsp_pkt_mgmt.p_out_rsp_pkt_buff = (CHAR *)pMemory;

        /* Initialize RSP working buffer pointer */
        rsp_pkt_mgmt.p_work_rsp_pkt_buff = (CHAR *)(rsp_pkt_mgmt.p_out_rsp_pkt_buff + RSP_MAX_PACKET_SIZE_SUPPORTED);

        /* Initialize the callback handlers to NULL. */
        for(i = 0; i < RSP_CALLBACK_END; i++)
        {
            RSP_REGISTER_CALLBACK_HANDLER(i, NU_NULL);
        }

        /* Register default packet handlers for all GDB packets defined in the header file */
        for(i = 0; i < MAX_RSP_CMDS; i++)
        {
            RSP_REGISTER_PACKET_HANDLER(i, rsp_default_handler);
        }

        /* Register function handlers for all supported RSP packets */
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_READ_MEMORY),rsp_memory_read_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_WRITE_MEMORY),rsp_memory_write_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_READ_REGISTER_GROUP),rsp_register_group_read_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_WRITE_REGISTER_GROUP),rsp_register_group_write_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_READ_REGISTER),rsp_register_read_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_WRITE_REGISTER),rsp_register_write_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_RESUME_TARGET),rsp_resume_target_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_SINGLE_STEP),rsp_single_step_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)RSP_HALT_TARGET,rsp_halt_target_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_INSERT_BREAKPOINT),rsp_insert_breakpoint_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_CLEAR_BREAKPOINT),rsp_clear_breakpoint_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_DISCONNECT),rsp_disconnect_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_KILL),rsp_vkill_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_GENERAL_QUERY_GET),rsp_general_query_get_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_REASON_TARGET_STOPPED), rsp_reason_target_stopped_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_SET_THREAD), rsp_set_thread_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_V_PACKET), rsp_v_packet_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_THREAD_ALIVE), rsp_thread_alive_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_Q_PACKET), rsp_q_packet_handler);
        RSP_REGISTER_PACKET_HANDLER((UINT)*((CHAR *)RSP_EXTENDED_MODE), rsp_extended_mode_handler);

        /* Initialize the thread timer (used for notification resends). */
        rsp_status = RSP_Timer_Initialize(&rsp_pkt_mgmt.thread_timer,
                                         rsp_thread_timer_exp,
                                         NU_NULL,
                                         RSP_THREAD_TIMER_STACK_SIZE);

        if (rsp_status == RSP_STATUS_OK)
        {
            /* Initialize the thread queue (used for notification). */
            rsp_status = RSP_Thread_Queue_Initialize(&rsp_pkt_mgmt.thread_queue,
                                                    RSP_THREAD_QUEUE_SIZE);

        }

    }
    else
    {
        /* Bad memory passed in return error code */
        rsp_status = RSP_STATUS_RESOURCE_NOT_AVAIL;
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       RSP_Process_Packet
*
*   DESCRIPTION
*
*       This function evaluates the received RSP packet from the client,
*       dispatches the appropriate packet handling function, calls the
*       registered Debug Server service handler, and constructs an RSP
*       response based on the call to the Debug Server.
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response string
*       p_rsp_resp_size         - Pointer the RSP Response size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicated successful operation
*       <other>                 - Indicates the corresponding failure
*                                 condition
*
*************************************************************************/
RSP_STATUS  RSP_Process_Packet(CHAR *    p_rsp_cmd_buff, CHAR *  p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS              rsp_status;

    /* Dispatch RSP command handling */
    rsp_status = RSP_PACKET_HANDLER_EXECUTE((UINT)(*p_rsp_cmd_buff), p_rsp_cmd_buff, p_rsp_resp_buff, p_rsp_resp_size);

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       RSP_Com_Error_Notify
*
*   DESCRIPTION
*
*       This function notifies the RSP component of a communications
*       error.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicated successful operation
*
*       <other>                 - Indicates the corresponding failure
*                                 condition
*
*************************************************************************/
RSP_STATUS  RSP_Com_Error_Notify(VOID)
{
    RSP_STATUS              rsp_status = RSP_STATUS_OK;

    /* Shutdown debugging activities. */
    rsp_status = rsp_shutdown();

    if (rsp_status == RSP_STATUS_OK)
    {
        /* Reset the RSP session. */
        rsp_status = rsp_session_reset();

    }

    return (rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       RSP_Com_Disconnect_Notify
*
*   DESCRIPTION
*
*       This function notifies the RSP component of a communications
*       disconnect event.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicated successful operation
*
*       <other>                 - Indicates the corresponding failure
*                                 condition
*
*************************************************************************/
RSP_STATUS  RSP_Com_Disconnect_Notify(VOID)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;

    /* Handle common debug engine and rsp shutdown tasks */
    rsp_status = rsp_shutdown();

    if (rsp_status == RSP_STATUS_OK)
    {
        /* Reset the session. */
        rsp_status = rsp_session_reset();
    }

    return (rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       RSP_Send_Notification
*
*   DESCRIPTION
*
*       This function implements logic to return RSP strings for notification
*       events like breakpoint, and single step completion to the calling
*       function.
*
*   INPUTS
*
*       notificationId          - Notification ID
*       notificationSignal      - Notification Signal
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response size
*       thread_id               - Thread ID where stop event occurred
*
*   OUTPUTS
*
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
RSP_STATUS  RSP_Send_Notification(UINT  notifId, UINT notifSignal, \
                                  CHAR * p_rsp_resp_buff, UINT * p_rsp_resp_size, \
                                  DBG_THREAD_ID thread_id)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;

    switch(notifId)
    {
        case    RSP_ACK_NOTIFICATION:
        {
            /* Check for No Ack Mode */
            if(rsp_pkt_mgmt.q_start_no_ack_mode == NU_TRUE)
            {
                /* No response is required in No Ack Mode.*/
                *p_rsp_resp_size = 0;
            }
            else
            {
                /* ACK the client */
                *p_rsp_resp_buff = RSP_ACK_CHAR;
                *p_rsp_resp_size = 1;
            }

            break;
        }
        case    RSP_NO_ACK_NOTIFICATION:
        {
            if(rsp_pkt_mgmt.q_start_no_ack_mode == NU_TRUE)
            {
                /* No response is required in No Ack Mode.*/
                *p_rsp_resp_size = 0;
            }
            else
            {
                /* Send the No ACK to the client */
                *p_rsp_resp_buff = RSP_NO_ACK_CHAR;
                *p_rsp_resp_size = 1;
            }

            break;
        }

        case    RSP_BREAKPOINT_NOTIFICATION:
        {
            /* Trap notification for breakpoints */

            switch(rsp_pkt_mgmt.mode)
            {
                case RSP_MODE_ALL_STOP :
                {
                    /* Reset the queue in all-stop mode. */
                    rsp_status = RSP_Thread_Queue_Reset(&rsp_pkt_mgmt.thread_queue);

                    if(rsp_status == RSP_STATUS_OK)
                    {
                        /* Set the current thread. */
                        rsp_pkt_mgmt.current_tid = thread_id;

                        /* Build a stop reply packet. */
                        rsp_status = rsp_create_stop_notify_packet(thread_id,
                                                                    "$T05",
                                                                    (CHAR *)p_rsp_resp_buff,
                                                                    p_rsp_resp_size);
                    }

                    if(rsp_status != RSP_STATUS_OK)
                    {
                        /* Handle Error condition */
                        rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                                          p_rsp_resp_buff,
                                                          p_rsp_resp_size);
                    }

                    break;

                } /* case */

                case RSP_MODE_NON_STOP :
                {
                    /* No response required */
                    *p_rsp_resp_size = 0;

                    if((rsp_pkt_mgmt.thread_seq_cnt == 0) && (rsp_pkt_mgmt.notify_tid == 0))
                    {
                        /* Get the first thread from the queue. */
                        rsp_status = RSP_Thread_Queue_Receive(&rsp_pkt_mgmt.thread_queue,\
                                                                &rsp_pkt_mgmt.notify_tid);

                        /* Set the % notification signal type */
                        rsp_pkt_mgmt.notify_sig_type = RSP_NOTIFY_SIGNAL_T05;

                        /* Start a notification timer */
                        rsp_status = RSP_Timer_Start(&rsp_pkt_mgmt.thread_timer, RSP_NOTIFY_PACKET_INIT_TIMEOUT);
                    }

                    break;

                } /* case */

                default :
                {
                    /* ERROR. A mode should have been specified. */
                    rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                                      p_rsp_resp_buff,
                                                      p_rsp_resp_size);

                    break;

                } /* default */

            } /* switch */

            break;
        }

        case    RSP_SINGLE_STEP_COMPLETE_NOTIFICATION:
        {
            /* Trap notification for single stepping */

            switch(rsp_pkt_mgmt.mode)
            {
                case RSP_MODE_ALL_STOP :
                {
                    /* Reset the queue in all-stop mode. */
                    rsp_status = RSP_Thread_Queue_Reset(&rsp_pkt_mgmt.thread_queue);

                    if(rsp_status == RSP_STATUS_OK)
                    {
                        /* Set the current thread. */
                        rsp_pkt_mgmt.current_tid = thread_id;

                        /* Build a stop reply packet. */
                        rsp_status = rsp_create_stop_notify_packet(thread_id,
                                                                    "$T05",
                                                                    (CHAR *)p_rsp_resp_buff,
                                                                    p_rsp_resp_size);
                    }

                    if(rsp_status != RSP_STATUS_OK)
                    {
                        /* Handle Error condition */
                        rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                                          p_rsp_resp_buff,
                                                          p_rsp_resp_size);
                    }

                    break;

                } /* case */

                case RSP_MODE_NON_STOP :
                {
                    /* No response required */
                    *p_rsp_resp_size = 0;

                    if((rsp_pkt_mgmt.thread_seq_cnt == 0) && (rsp_pkt_mgmt.notify_tid == 0))
                    {
                        /* Get the first thread from the queue. */
                        rsp_status = RSP_Thread_Queue_Receive(&rsp_pkt_mgmt.thread_queue,\
                                                                &rsp_pkt_mgmt.notify_tid);

                        /* Set the % notification signal type */
                        rsp_pkt_mgmt.notify_sig_type = RSP_NOTIFY_SIGNAL_T05;

                        /* Start a notification timer */
                        rsp_status = RSP_Timer_Start(&rsp_pkt_mgmt.thread_timer, RSP_NOTIFY_PACKET_INIT_TIMEOUT);
                    }

                    break;

                } /* case */

                default :
                {
                    /* ERROR. A mode should have been specified. */
                    rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                                      p_rsp_resp_buff,
                                                      p_rsp_resp_size);

                    break;

                } /* default */

            } /* switch */

            break;

        }

        default :
        {
            /* Return error code */
            rsp_status = RSP_STATUS_RESOURCE_NOT_AVAIL;

            break;

        }

    }  /* switch */

    /* Return notification packet construction status */
    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_session_reset
*
*   DESCRIPTION
*
*       This function resets the RSP component debug session.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_session_reset(VOID)
{
    RSP_STATUS      rsp_status;
    DBG_STATUS      dbg_status;
    DBG_THREAD_ID   thread_id = 0;
    DBG_CMD         dbg_cmd;

    /* Set initial function status. */
    rsp_status = RSP_STATUS_OK;

    /* Stop the timer */
    rsp_status = RSP_Timer_Stop(&rsp_pkt_mgmt.thread_timer);

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Reset the queue. */
        rsp_status = RSP_Thread_Queue_Reset(&rsp_pkt_mgmt.thread_queue);

    }

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Reset the the debug engine session before checking for active threads. */
        dbg_cmd.op = DBG_CMD_OP_SESSION_RESET;

        /* Call Debug Server */
        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

        if (dbg_status == DBG_STATUS_OK)
        {
            rsp_status = RSP_STATUS_OK;
        }
        else
        {
            rsp_status = RSP_STATUS_FAILED;
        }
    }

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Check for active application threads in the system after the debug engine has been reset. */
        rsp_status = rsp_get_current_thread(&thread_id);

        switch (rsp_status)
        {
            case RSP_STATUS_OK :
            {
                /* A statically linked application is running */
                rsp_pkt_mgmt.static_app_mode = NU_TRUE;

                break;

            }

            case RSP_STATUS_RESOURCE_NOT_AVAIL :
            {
                /* No threads in system. */
                rsp_pkt_mgmt.static_app_mode = NU_FALSE;

                /* Reset status so initialization will complete */
                rsp_status = RSP_STATUS_OK;

                break;

            }

            default :
            {
                /* ERROR: Unable to determine if threads in the
                    system. */
                rsp_status = RSP_STATUS_FAILED;

                break;

            }

        } /* switch */

    }

    if (rsp_status == RSP_STATUS_OK)
    {
        /* Initialize no ack mode. */
        rsp_pkt_mgmt.q_start_no_ack_mode = NU_FALSE;

        /* Clear the application running count */
        rsp_pkt_mgmt.dle_app_cnt = 0;

        /* Initialize flag to track first pass in QSubsequent thread handler */
        rsp_pkt_mgmt.is_first_qsubseq = NU_TRUE;

        /* Initialize mode. */
        rsp_pkt_mgmt.mode = RSP_MODE_ALL_STOP;

        /* Initialize working thread IDs */
        rsp_pkt_mgmt.wrk_tid_exec = DBG_THREAD_ID_ALL;
        rsp_pkt_mgmt.wrk_tid_other = DBG_THREAD_ID_ANY;

        /* Initialize the current thread IDs */
        rsp_pkt_mgmt.current_tid = DBG_THREAD_ID_NONE;

        /* Initialize the application offset address. */
        rsp_pkt_mgmt.app_offset = 0x0;

        /* Set the notification thread ID to zero */
        rsp_pkt_mgmt.notify_tid = 0;

    }

    return(rsp_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_memory_read_handler
*
*   DESCRIPTION
*
*       This function implements logic to read memory to support the RSP
*       packet 'm' - memory read
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_memory_read_handler(CHAR *  p_rsp_cmd_buff, CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    UINT                address = 0;
    UINT                length = 0;
    UINT                byts_read = 0;
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status;
    DBG_CMD             dbg_cmd;

    /* Adjust pointer to RSP input buffer to start of command */
    p_rsp_pkt = p_rsp_pkt + Get_String_Size(RSP_READ_MEMORY);

    /* Get Address */
    rsp_status = RSP_Get_UINT_Data(p_rsp_pkt,&p_rsp_pkt,&address,RSP_COMMA_DELIMITER);

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Get Length */
        rsp_status = RSP_Get_UINT_Data(p_rsp_pkt,&p_rsp_pkt,&length,RSP_PKT_END_CHAR);
    }

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Setup Debug Command arguments */
        dbg_cmd.op = DBG_CMD_OP_MEMORY_READ;
        dbg_cmd.op_param.mem_rd.p_read = (VOID *)address;
        dbg_cmd.op_param.mem_rd.read_size = length;
        dbg_cmd.op_param.mem_rd.access_mode = DBG_MEM_ACCESS_MODE_ANY;
        dbg_cmd.op_param.mem_rd.p_read_buffer = (VOID *)rsp_pkt_mgmt.p_work_rsp_pkt_buff;
        dbg_cmd.op_param.mem_rd.p_actual_read_size = &byts_read;

        /* Call Debug Server */
        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

        if(dbg_status == DBG_STATUS_OK)
        {
            /* Construct RSP Response */
            RSP_Convert_Bin_Arry_2_Rsp_Pkt((CHAR *)rsp_pkt_mgmt.p_work_rsp_pkt_buff, byts_read, (CHAR *)p_rsp_resp_buff, p_rsp_resp_size);

            rsp_status = RSP_STATUS_OK;
        }
        else
        {
            /* Handle Error condition */
            rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);
        }
    }
    else
    {
        /* Handle Error condition */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E01_MSG,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_memory_write_handler
*
*   DESCRIPTION
*
*       This function implements logic to write memory to support the RSP
*       packet 'M' - memory write
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_memory_write_handler(CHAR *  p_rsp_cmd_buff, CHAR * p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    UINT                address = 0;
    UINT                length = 0;
    UINT                byts_written = 0;
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status;
    DBG_CMD             dbg_cmd;

    p_rsp_pkt = p_rsp_pkt + Get_String_Size(RSP_WRITE_MEMORY);

    /* Get Address */
    rsp_status = RSP_Get_UINT_Data(p_rsp_pkt,&p_rsp_pkt,&address, RSP_COMMA_DELIMITER);

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Get Length */
        rsp_status = RSP_Get_UINT_Data(p_rsp_pkt,&p_rsp_pkt,&length, RSP_COLON_DELIMITER);
    }

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Get data */
        rsp_status = RSP_Get_Data(&p_rsp_pkt, rsp_pkt_mgmt.p_work_rsp_pkt_buff, RSP_PKT_END_CHAR);
    }

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Setup write memory debug API command parameters */
        dbg_cmd.op = DBG_CMD_OP_MEMORY_WRITE;
        dbg_cmd.op_param.mem_wrt.p_write = (VOID *)address;
        dbg_cmd.op_param.mem_wrt.write_size = length;
        dbg_cmd.op_param.mem_wrt.access_mode = DBG_MEM_ACCESS_MODE_ANY;
        dbg_cmd.op_param.mem_wrt.p_write_buffer = (VOID *)rsp_pkt_mgmt.p_work_rsp_pkt_buff;
        dbg_cmd.op_param.mem_wrt.p_actual_write_size = &byts_written;

        /* Call Debug Server */
        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

        if(dbg_status == DBG_STATUS_OK)
        {
            *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);

            rsp_status = RSP_STATUS_OK;
        }
        else
        {
            /* Respond with debug operation error code */
            rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);
        }
    }
    else
    {
        /* Respond with packet error code */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E01_MSG,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_register_group_read_handler
*
*   DESCRIPTION
*
*       This function implements logic to architecture register set to
*       to support the RSP packet 'g' - register group read
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_register_group_read_handler(CHAR *  p_rsp_cmd_buff, CHAR *  p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    UINT                byts_read = 0;
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status;
    DBG_CMD             dbg_cmd;
    DBG_THREAD_ID       thread_id = 0;

    if(rsp_pkt_mgmt.wrk_tid_other == DBG_THREAD_ID_ANY)
    {
        /* The working thread ID = 0. Use the first thread for the register read. */

        /* Setup Debug Command arguments */
        dbg_cmd.op = DBG_CMD_OP_THREAD_GET_FIRST;
        dbg_cmd.op_param.thd_get_fst_nxt.p_thread_id = &thread_id;

        /* Call Debug Server */
        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);
    }
    else
    {
        /* Use the wrk_tid_other as set by the GDB client. */
        thread_id = rsp_pkt_mgmt.wrk_tid_other;

        dbg_status = DBG_STATUS_OK;
    }

    if(dbg_status == DBG_STATUS_OK)
    {
        /* Read all registers. */
        dbg_cmd.op = DBG_CMD_OP_REGISTER_READ;
        dbg_cmd.op_param.reg_rd.thread_id = thread_id;
        dbg_cmd.op_param.reg_rd.register_id = DBG_REGISTER_ID_ALL;
        dbg_cmd.op_param.reg_rd.p_register_data = (VOID *)rsp_pkt_mgmt.p_work_rsp_pkt_buff;
        dbg_cmd.op_param.reg_rd.p_actual_reg_data_size = &byts_read;

        /* Call Debug Server */
        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);
    }

    if(dbg_status == DBG_STATUS_OK)
    {
        /* Construct RSP Response */
        RSP_Convert_Bin_Arry_2_Rsp_Pkt((CHAR *)rsp_pkt_mgmt.p_work_rsp_pkt_buff, byts_read, (CHAR *)p_rsp_resp_buff, p_rsp_resp_size);

        rsp_status = RSP_STATUS_OK;
    }
    else
    {
        /* Handle Error condition */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_register_group_write_handler
*
*   DESCRIPTION
*
*       This function implements logic to write architecture register set
*       to support RSP packet - 'G' - register group write
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_register_group_write_handler(CHAR *  p_rsp_cmd_buff, CHAR * p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status;
    DBG_CMD             dbg_cmd;
    DBG_THREAD_ID       thread_id = 0;

    p_rsp_pkt = p_rsp_pkt + Get_String_Size(RSP_WRITE_REGISTER_GROUP);

    /* Get Register Data */
    rsp_status = RSP_Get_UINT_Data(p_rsp_pkt,&p_rsp_pkt,(UINT *)rsp_pkt_mgmt.p_work_rsp_pkt_buff,RSP_PKT_END_CHAR);

    if(rsp_status == RSP_STATUS_OK)
    {

        if(rsp_pkt_mgmt.wrk_tid_other == DBG_THREAD_ID_ANY)
        {
            /* The working thread ID = 0. Use the first thread for the register read. */

            /* Setup Debug Command arguments */
            dbg_cmd.op = DBG_CMD_OP_THREAD_GET_FIRST;
            dbg_cmd.op_param.thd_get_fst_nxt.p_thread_id = &thread_id;

            /* Call Debug Server */
            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);
        }
        else
        {
            /* Use the wrk_tid_other as set by the GDB client. */
            thread_id = rsp_pkt_mgmt.wrk_tid_other;

            dbg_status = DBG_STATUS_OK;
        }

        if(dbg_status == DBG_STATUS_OK)
        {
            /* Write all registers. */
            dbg_cmd.op = DBG_CMD_OP_REGISTER_WRITE;
            dbg_cmd.op_param.reg_rd.thread_id = thread_id;
            dbg_cmd.op_param.reg_wrt.register_id = DBG_REGISTER_ID_ALL;
            dbg_cmd.op_param.reg_wrt.p_register_data = (VOID *)rsp_pkt_mgmt.p_work_rsp_pkt_buff;

            /* Call Debug Server */
            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);
        }

        if(dbg_status == DBG_STATUS_OK)
        {
            *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);

            rsp_status = RSP_STATUS_OK;
        }
        else
        {
            /* Handle Error condition */
            rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);
        }
    }
    else
    {
        /* Handle Error condition */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E01_MSG,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_register_read_handler
*
*   DESCRIPTION
*
*       This function implements logic to read register
*       to support RSP packet - 'p' - read register
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_register_read_handler(CHAR *  p_rsp_cmd_buff, CHAR *    p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    UINT                reg_id;
    UINT                byts_read;
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status;
    DBG_CMD             dbg_cmd;
    DBG_THREAD_ID       thread_id = 0;

    p_rsp_pkt = p_rsp_pkt + Get_String_Size(RSP_READ_REGISTER);

    /* Get Register ID */
    rsp_status = RSP_Get_UINT_Data(p_rsp_pkt,&p_rsp_pkt,&reg_id,RSP_PKT_END_CHAR);

    if(rsp_status == RSP_STATUS_OK)
    {

        if(rsp_pkt_mgmt.wrk_tid_other == DBG_THREAD_ID_ANY)
        {
            /* The working thread ID = 0. Use the first thread for the register read. */

            /* Setup Debug Command arguments */
            dbg_cmd.op = DBG_CMD_OP_THREAD_GET_FIRST;
            dbg_cmd.op_param.thd_get_fst_nxt.p_thread_id = &thread_id;

            /* Call Debug Server */
            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);
        }
        else
        {
            /* Use the wrk_tid_other as set by the GDB client. */
            thread_id = rsp_pkt_mgmt.wrk_tid_other;

            dbg_status = DBG_STATUS_OK;
        }

        if(dbg_status == DBG_STATUS_OK)
        {
            /* Read specific register. */
            dbg_cmd.op = DBG_CMD_OP_REGISTER_READ;
            dbg_cmd.op_param.reg_rd.thread_id = thread_id;
            dbg_cmd.op_param.reg_rd.register_id = reg_id;
            dbg_cmd.op_param.reg_rd.p_register_data = (VOID *)rsp_pkt_mgmt.p_work_rsp_pkt_buff;
            dbg_cmd.op_param.reg_rd.p_actual_reg_data_size = &byts_read;

            /* Call Debug Server */
            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);
        }

        if(dbg_status == DBG_STATUS_OK)
        {
            /* Construct RSP Response */
            RSP_Convert_Bin_Arry_2_Rsp_Pkt((CHAR *)rsp_pkt_mgmt.p_work_rsp_pkt_buff, byts_read, (CHAR *)p_rsp_resp_buff, p_rsp_resp_size);

            rsp_status = RSP_STATUS_OK;
        }
        else
        {
            /* Handle Error condition */
            rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);
        }
    }
    else
    {
        rsp_status = rsp_create_error_msg(RSP_ERROR_E01_MSG,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }
    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_register_write_handler
*
*   DESCRIPTION
*
*       This function implements logic to write register
*       to support RSP packet - 'P' - write register
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_register_write_handler(CHAR *  p_rsp_cmd_buff, CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    UINT                reg_id;
    UINT                reg_val;
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status;
    DBG_CMD             dbg_cmd;
    DBG_THREAD_ID       thread_id = 0;

    p_rsp_pkt = p_rsp_pkt + Get_String_Size(RSP_WRITE_REGISTER);

    /* Get Register ID */
    rsp_status = RSP_Get_UINT_Data(p_rsp_pkt,&p_rsp_pkt,&reg_id,RSP_EQUAL_SIGN_DELIMITER);

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Get Register Value */
        rsp_status = RSP_Get_UINT_Data(p_rsp_pkt,&p_rsp_pkt,&reg_val,RSP_PKT_END_CHAR);
    }

    if(rsp_status == RSP_STATUS_OK)
    {
        reg_val = RSP_Reverse_Int(reg_val);

        if(rsp_pkt_mgmt.wrk_tid_other == DBG_THREAD_ID_ANY)
        {
            /* The working thread ID = 0. Use the first thread for the register read. */

            /* Setup Debug Command arguments */
            dbg_cmd.op = DBG_CMD_OP_THREAD_GET_FIRST;
            dbg_cmd.op_param.thd_get_fst_nxt.p_thread_id = &thread_id;

            /* Call Debug Server */
            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);
        }
        else
        {
            /* Use the wrk_tid_other as set by the GDB client. */
            thread_id = rsp_pkt_mgmt.wrk_tid_other;

            dbg_status = DBG_STATUS_OK;
        }

        if(dbg_status == DBG_STATUS_OK)
        {
            /* Write specific register. */
            dbg_cmd.op = DBG_CMD_OP_REGISTER_WRITE;
            dbg_cmd.op_param.reg_rd.thread_id = thread_id;
            dbg_cmd.op_param.reg_wrt.register_id = reg_id;
            dbg_cmd.op_param.reg_wrt.p_register_data = &reg_val;

            /* Call Debug Server */
            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);
        }

        if(dbg_status == DBG_STATUS_OK)
        {
             /* Construct RSP Response */
            *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);

            rsp_status = RSP_STATUS_OK;
        }
        else
        {
            /* Handle Error condition */
            rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);
        }
    }
    else
    {
        /* Handle Error condition */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E01_MSG,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_resume_target_handler
*
*   DESCRIPTION
*
*       This function implements logic to resume target - 'c' - resume target
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_resume_target_handler(CHAR *  p_rsp_cmd_buff, CHAR *    p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS              rsp_status;
    DBG_STATUS              dbg_status;
    DBG_CMD                 dbg_cmd;

#ifdef CFG_NU_OS_SVCS_TRACE_ENABLE

    NU_Trace_Arm(DBG_Trace_Mask);

#endif /* CFG_NU_OS_SVCS_TRACE_ENABLE */

    /* Set all threads running. */
    dbg_cmd.op = DBG_CMD_OP_THREAD_GO;

    if(rsp_pkt_mgmt.mode == RSP_MODE_NON_STOP)
    {
        dbg_cmd.op_param.thd_go.thread_id = rsp_pkt_mgmt.wrk_tid_exec;
    }
    else
    {
        dbg_cmd.op_param.thd_go.thread_id = DBG_THREAD_ID_ALL;
    }


    /* Call Debug Server */
    dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

    /* No response required */
    *p_rsp_resp_size = 0;

    if(dbg_status == DBG_STATUS_OK)
    {
        rsp_status = RSP_STATUS_OK;
    }
    else
    {
        rsp_status = RSP_STATUS_NO_RESP;
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_single_step_handler
*
*   DESCRIPTION
*
*       This function implements logic to single step target - 's' - single
*       step
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_single_step_handler(CHAR *  p_rsp_cmd_buff, CHAR *  p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS              rsp_status;
    DBG_STATUS              dbg_status;
    DBG_CMD                 dbg_cmd;

#ifdef CFG_NU_OS_SVCS_TRACE_ENABLE

    NU_Trace_Arm(DBG_Trace_Mask);

#endif /* CFG_NU_OS_SVCS_TRACE_ENABLE */

    /* Perform a single-step. */
    dbg_cmd.op = DBG_CMD_OP_THREAD_STEP;
    dbg_cmd.op_param.thd_step.step_thread_id = rsp_pkt_mgmt.wrk_tid_step;

    if(rsp_pkt_mgmt.mode == RSP_MODE_NON_STOP)
    {
        dbg_cmd.op_param.thd_step.go_thread_id = rsp_pkt_mgmt.wrk_tid_step;
        dbg_cmd.op_param.thd_step.stop_thread_id = rsp_pkt_mgmt.wrk_tid_step;
    }
    else
    {
        dbg_cmd.op_param.thd_step.go_thread_id = DBG_THREAD_ID_ALL;
        dbg_cmd.op_param.thd_step.stop_thread_id = DBG_THREAD_ID_ALL;
    }

    /* Call Debug Server */
    dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

    /* No response required */
    *p_rsp_resp_size = 0;

    if(dbg_status == DBG_STATUS_OK)
    {
        rsp_status = RSP_STATUS_OK;
    }
    else
    {
        rsp_status = RSP_STATUS_NO_RESP;
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_halt_target_handler
*
*   DESCRIPTION
*
*       This function implements logic to halt target - '.' - halt execution
*       Cntrl-c
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_halt_target_handler(CHAR *  p_rsp_cmd_buff, CHAR *  p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS              rsp_status = RSP_STATUS_OK;
    DBG_STATUS              dbg_status = DBG_STATUS_NONE;
    DBG_CMD                 dbg_cmd;
    DBG_THREAD_ID           thread_id;

#ifdef CFG_NU_OS_SVCS_TRACE_ENABLE

    DBG_Trace_Mask = NU_Trace_Get_Mask();
    NU_Trace_Disarm(DBG_Trace_Mask);

#endif /* CFG_NU_OS_SVCS_TRACE_ENABLE */

    /* Stop all threads. */
    dbg_cmd.op = DBG_CMD_OP_THREAD_STOP;

    if(rsp_pkt_mgmt.mode == RSP_MODE_NON_STOP)
    {
        /* In non-stop mode get the exec thread value */
        thread_id = rsp_pkt_mgmt.wrk_tid_exec;
        dbg_cmd.op_param.thd_stop.thread_id = thread_id;
    }
    else
    {
        /* In all-stop mode get the current thread ID. */
        rsp_status = rsp_get_current_thread(&thread_id);

        if(rsp_status == RSP_STATUS_OK)
        {
            /* Set the thread ID to all */
            dbg_cmd.op_param.thd_stop.thread_id = DBG_THREAD_ID_ALL;

            /* Reset the queue in all-stop mode. */
            rsp_status = RSP_Thread_Queue_Reset(&rsp_pkt_mgmt.thread_queue);

        }
    }

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Call Debug Server */
        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);
    }

    if(dbg_status == DBG_STATUS_OK)
    {
        /* Build a stop reply packet. */
        rsp_status = rsp_create_stop_notify_packet(thread_id,
                                                    "$T05",
                                                    (CHAR *)p_rsp_resp_buff,
                                                    p_rsp_resp_size);

    }
    else
    {
       /* Construct RSP Response */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_insert_breakpoint_handler
*
*   DESCRIPTION
*
*       This function implements logic to insert breakpoint -
*       'Z' - insert breakpoint
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_insert_breakpoint_handler(CHAR *  p_rsp_cmd_buff, CHAR *    p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    UINT                address;
    UINT                length;
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status = DBG_STATUS_OK;
    DBG_CMD             dbg_cmd;

    p_rsp_pkt = p_rsp_pkt + Get_String_Size(RSP_INSERT_BREAKPOINT);

    /* Get breakpoint address */
    rsp_status = RSP_Get_UINT_Data(p_rsp_pkt,&p_rsp_pkt,&address,RSP_COMMA_DELIMITER);

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Get breakpoint length */
        rsp_status = RSP_Get_UINT_Data(p_rsp_pkt,&p_rsp_pkt,&length,RSP_PKT_END_CHAR);
    }

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Setup of breakpoint set operation. */
        dbg_cmd.op = DBG_CMD_OP_BREAKPOINT_SET;

        dbg_cmd.op_param.bkpt_set.p_address = (VOID *)address;
        dbg_cmd.op_param.bkpt_set.pass_count = 0;
        dbg_cmd.op_param.bkpt_set.hit_count = 0;

        if(rsp_pkt_mgmt.mode == RSP_MODE_NON_STOP)
        {
            /* Non-Stop mode */
            dbg_cmd.op_param.bkpt_set.hit_thread_id = DBG_THREAD_ID_ALL;
            dbg_cmd.op_param.bkpt_set.stop_thread_id = DBG_THREAD_ID_ANY;
        }
        else
        {
            /* All-Stop mode */
            dbg_cmd.op_param.bkpt_set.hit_thread_id = DBG_THREAD_ID_ALL;
            dbg_cmd.op_param.bkpt_set.stop_thread_id = DBG_THREAD_ID_ALL;
        }

        /* Call Debug Server */
        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

        /* Check for success */
        if(dbg_status == DBG_STATUS_OK)
        {
           /* Construct RSP Response */
           *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);

           rsp_status = RSP_STATUS_OK;
        }
        else
        {
            /* Handle Error condition */
            rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);
        }
    }
    else
    {
        /* Handle Error condition */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E01_MSG,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_clear_breakpoint_handler
*
*   DESCRIPTION
*
*       This function implements logic to clear breakpoint -
*       'z' - insert breakpoint
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_clear_breakpoint_handler(CHAR *  p_rsp_cmd_buff, CHAR * p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    UINT                address;
    UINT                length;
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status;
    DBG_CMD             dbg_cmd;

    p_rsp_pkt = p_rsp_pkt + Get_String_Size(RSP_INSERT_BREAKPOINT);

    /* Get breakpoint address */
    rsp_status = RSP_Get_UINT_Data(p_rsp_pkt,&p_rsp_pkt,&address,RSP_COMMA_DELIMITER);

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Get breakpoint length */
        rsp_status = RSP_Get_UINT_Data(p_rsp_pkt,&p_rsp_pkt,&length,RSP_PKT_END_CHAR);
    }

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Remove all breakpoints at the specified address (no thread ID). */
        dbg_cmd.op = DBG_CMD_OP_BREAKPOINT_CLEAR;
        dbg_cmd.op_param.bkpt_clr.thread_id = DBG_THREAD_ID_NONE;
        dbg_cmd.op_param.bkpt_clr.p_address = (VOID *)address;

        /* Call Debug Server */
        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

         /* Check for success */
        if(dbg_status == DBG_STATUS_OK)
        {
           /* Construct RSP Response */
           *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);

           rsp_status = RSP_STATUS_OK;
        }
        else
        {
            /* Handle Error condition */
            rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);
        }
    }
    else
    {
        /* Handle Error condition */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E01_MSG,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_general_query_get_handler
*
*   DESCRIPTION
*
*       This function implements logic general query - "qxxx" - general query
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       RSP_STATUS_ERROR_RESP   - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_general_query_get_handler(CHAR *  p_rsp_cmd_buff, CHAR *    p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    DBG_THREAD_ID       thread_id;

    if(p_rsp_pkt[1] == 'S' && p_rsp_pkt[2] == 'u')
    {
        if(rsp_status == RSP_STATUS_OK)
        {
            /* Reset the RSP session as the qSupported packets marks the start
            of a new debugging session. */
            rsp_status = rsp_session_reset();
        }

        if (rsp_status == RSP_STATUS_OK)
        {
            /* Construct RSP Response for "qSupported" */
#if (CFG_NU_OS_SVCS_DBG_COM_PORT_TYPE != 1)
            /* TCP */
            *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("PacketSize=200;QStartNoAckMode+;QNonStop+",p_rsp_resp_buff);
#else
            /* SERIAL */
            *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("PacketSize=200;QNonStop+",p_rsp_resp_buff);
#endif

        }
        else
        {
            /* ERROR: Unable to reset debug session. */
            rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);
        }

    }
    else if(p_rsp_pkt[1] == 'f' && p_rsp_pkt[2] == 'T')
    {
        /* Call RSP Response handler for "qfThreadInfo" */
        rsp_status = rsp_qfirst_thread_handler(p_rsp_cmd_buff, p_rsp_resp_buff, p_rsp_resp_size);
    }
    else if(p_rsp_pkt[1] == 's' && p_rsp_pkt[2] == 'T')
    {
        /* Construct RSP Response for "qsThreadInfo" */
        rsp_status = rsp_qsubsequent_thread_handler(p_rsp_cmd_buff, p_rsp_resp_buff, p_rsp_resp_size);
    }
    else if(p_rsp_pkt[1] == 'C')
    {
        /* Construct RSP Response for "qC" */
        rsp_status = rsp_get_current_thread_handler(p_rsp_cmd_buff, p_rsp_resp_buff, p_rsp_resp_size);
    }
    else if(p_rsp_pkt[1] == 'A' && p_rsp_pkt[2] == 't')
    {
        /* Construct RSP Response for "qAttached" */

        /* Determine if there is an application running. */
        rsp_status = rsp_get_current_thread(&thread_id);

        switch (rsp_status)
        {
            case RSP_STATUS_OK :
            {
                /* Indicate attached to existing process (application). */
                *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("1",p_rsp_resp_buff);

                break;
            }

            case  RSP_STATUS_RESOURCE_NOT_AVAIL :
            {
                /* Indicate not attached. */
                *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("",p_rsp_resp_buff);

                rsp_status = RSP_STATUS_ERROR_RESP;

                break;
            }

            default :
            {
                /* ERROR */
                break;
            }
        }

    }
    else if((p_rsp_pkt[1] == 'T') && (p_rsp_pkt[7] == 'E'))
    {
        /* Construct RSP Response for "qThreadExtraInfo" */
        rsp_status = rsp_thread_extra_info_handler(p_rsp_cmd_buff, p_rsp_resp_buff, p_rsp_resp_size);
    }
    else if ((p_rsp_pkt[1] == 'S') && (p_rsp_pkt[8] == ':'))
    {
        /* Construct response for "qSymbol::".  Note that the
           server does not request any symbols from the client. */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);

    }
    else if (p_rsp_pkt[1] == 'O')
    {
        /* Construct response for "qOffsets" using the current application
           offset address. */

        rsp_status = rsp_offsets_handler(p_rsp_resp_buff, p_rsp_resp_size);

    }
    else
    {
        /* Construct RSP Error Response */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("",p_rsp_resp_buff);

        rsp_status = RSP_STATUS_ERROR_RESP;
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_reason_target_stopped_handler
*
*   DESCRIPTION
*
*       This function implements logic to probe the reason the target stopped
*       - "?" - reason target stopped
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_reason_target_stopped_handler(CHAR *  p_rsp_cmd_buff, CHAR *    p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS               rsp_status;
    DBG_THREAD_ID            thread_id;
    RSP_THREAD_QUEUE_INFO    thread_queue_info;

    /* Make sure a current thread ID is available. */
    rsp_status = rsp_get_current_thread(&thread_id);

    if(rsp_status == RSP_STATUS_OK)
    {
        if(rsp_pkt_mgmt.mode == RSP_MODE_NON_STOP)
        {
            /* Stop the timer */
            rsp_status = RSP_Timer_Stop(&rsp_pkt_mgmt.thread_timer);

            /* Set the notification thread ID to a non-zero value.
               This prevents a step or breakpoint interrupt from
               trying to remove a thread ID from the queue.
               This value will be cleared in the vStopped handler.*/
            rsp_pkt_mgmt.notify_tid = (DBG_THREAD_ID)1;

            if(rsp_status == RSP_STATUS_OK)
            {
                /* Reset the queue. */
                rsp_status = RSP_Thread_Queue_Reset(&rsp_pkt_mgmt.thread_queue);
            }

            if(rsp_status == RSP_STATUS_OK)
            {
                /* Add all stopped threads to the queue. */
                rsp_status = RSP_Thread_Queue_Send_All_Stopped(&rsp_pkt_mgmt.thread_queue,\
                                                            rsp_pkt_mgmt.dbg_session_id);
            }

            if(rsp_status == RSP_STATUS_OK)
            {
                rsp_status = RSP_Thread_Queue_Info(&rsp_pkt_mgmt.thread_queue,\
                                                   &thread_queue_info);
            }

            if(rsp_status == RSP_STATUS_OK)
            {
                /* Save the '?' sequence count of stopped threads.*/
                rsp_pkt_mgmt.thread_seq_cnt = thread_queue_info.count;

                rsp_status = RSP_Thread_Queue_Receive(&rsp_pkt_mgmt.thread_queue, &thread_id);
            }

            switch(rsp_status)
            {
                case RSP_STATUS_OK :
                {
                    /* Update the '?' sequence count of stopped threads.*/
                    rsp_pkt_mgmt.thread_seq_cnt--;

                    /* Set the current thread. */
                    rsp_pkt_mgmt.current_tid = thread_id;

                    /* Build stop reply packet for next thread in queue. */
                    rsp_status = rsp_create_stop_notify_packet(thread_id,
                                                                "$T05",
                                                                (CHAR *)p_rsp_resp_buff,
                                                                p_rsp_resp_size);

                    if (rsp_status != RSP_STATUS_OK)
                    {
                        /* ERROR: Unable to create stop notify packet. */
                        rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                                          p_rsp_resp_buff,
                                                          p_rsp_resp_size);
                    }

                    break;

                } /* case */

                case RSP_STATUS_RESOURCE_NOT_AVAIL :
                {
                    /* End of queue. */
                    *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);

                    rsp_pkt_mgmt.notify_tid = 0;

                    rsp_status = RSP_STATUS_OK;

                    break;

                } /* case */

                default :
                {
                    /* ERROR: Retrieving thread from queue failed. */
                    rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                                      p_rsp_resp_buff,
                                                      p_rsp_resp_size);

                    rsp_pkt_mgmt.notify_tid = 0;

                    break;

                } /* default */

            } /* switch */

        }
        else
        {
            /* All stop mode. */

            /* Get the current thread ID. */
            rsp_status = rsp_get_current_thread(&thread_id);

            if(rsp_status == RSP_STATUS_OK)
            {
                /* Set the current thread. */
                rsp_pkt_mgmt.current_tid = thread_id;

                /* Build stop reply packet for next thread in queue. */
                rsp_status = rsp_create_stop_notify_packet(thread_id,
                                                           "$T05",
                                                           (CHAR *)p_rsp_resp_buff,
                                                           p_rsp_resp_size);
            }

            if (rsp_status != RSP_STATUS_OK)
            {
                /* ERROR: Unable to create stop notify packet. */
                rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                                  p_rsp_resp_buff,
                                                  p_rsp_resp_size);
            }
        }
    }
    else
    {
        if(rsp_pkt_mgmt.mode == RSP_MODE_NON_STOP)
        {
            /* Current thread is not available. */
            *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);
        }
        else
        {
            /* Current thread is not available. */
            *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("W00",p_rsp_resp_buff);
        }

        rsp_status = RSP_STATUS_ERROR_RESP;
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_disconnect_handler
*
*   DESCRIPTION
*
*       This function implements logic to disconnect from target
*       - "D" - Disconnect from target
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_disconnect_handler(CHAR *  p_rsp_cmd_buff, CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;

    /* Handle common debug engine and rsp shutdown tasks */
    rsp_status = rsp_shutdown();

    if (rsp_status == RSP_STATUS_OK)
    {
        /* Reset the session. */
        rsp_status = rsp_session_reset();
    }

    if (rsp_status == RSP_STATUS_OK)
    {
       /* Construct RSP Response */
       *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);

       rsp_status = RSP_STATUS_OK;
    }
    else
    {
        /* ERROR: Unable to reset RSP session. */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_qfirst_thread_handler
*
*   DESCRIPTION
*
*       This function implements logic to support the RSP
*       packet 'qfThreadInfo' - first thread info query.
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_qfirst_thread_handler(CHAR *  p_rsp_cmd_buff, CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status;
    DBG_CMD             dbg_cmd;
    CHAR                str_array[30];

    /* Setup Debug Command arguments */
    dbg_cmd.op = DBG_CMD_OP_THREAD_GET_FIRST;
    dbg_cmd.op_param.thd_get_fst_nxt.p_thread_id = &(rsp_pkt_mgmt.first_next_tid);

    /* Call Debug Server */
    dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

    if(dbg_status == DBG_STATUS_OK)
    {
        /* Start the response with an 'm' */
        str_array[0] = 'm';

        /* Construct RSP Response */
         DBG_STR_String_From_UINT(&str_array[1],
                                  (UINT)rsp_pkt_mgmt.first_next_tid,
                                  DBG_STR_RADIX_HEXADECIMAL,
                                  DBG_STR_INC_PREFIX_DISABLE,
                                  DBG_STR_INC_LEADING_ZEROS_DISABLE);

        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt(str_array, p_rsp_resp_buff);

        rsp_status = RSP_STATUS_OK;
    }
    else
    {
        /* ERROR: Unable to get first thread from debug engine. */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_qsubsequent_thread_handler
*
*   DESCRIPTION
*
*       This function implements logic to support the RSP
*       packet 'qsThreadInfo' - subsequent thread info queries.
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_qsubsequent_thread_handler(CHAR *  p_rsp_cmd_buff, CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status;
    DBG_CMD             dbg_cmd;
    CHAR                str_array[30];

    /* Setup Debug Command arguments */
    dbg_cmd.op = DBG_CMD_OP_THREAD_GET_NEXT;
    dbg_cmd.op_param.thd_get_fst_nxt.p_thread_id = &(rsp_pkt_mgmt.first_next_tid);

    /* Call Debug Server */
    dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

    if(dbg_status == DBG_STATUS_OK)
    {

        /* Start the response with an 'm' */
        str_array[0] = 'm';

        /* Construct RSP Response */
        DBG_STR_String_From_UINT(&str_array[1],
                                (UINT)rsp_pkt_mgmt.first_next_tid,
                                DBG_STR_RADIX_HEXADECIMAL,
                                DBG_STR_INC_PREFIX_DISABLE,
                                DBG_STR_INC_LEADING_ZEROS_DISABLE);

        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt(str_array, p_rsp_resp_buff);

        rsp_status = RSP_STATUS_OK;
    }
    else
    {
        /* Handle end of thread list. */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("l",p_rsp_resp_buff);

        rsp_status = RSP_STATUS_OK;
    }

    return(rsp_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_set_thread_handler
*
*   DESCRIPTION
*
*       This function implements logic for the set thread - "Hx" - command
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_set_thread_handler(CHAR *  p_rsp_cmd_buff, CHAR *    p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status;
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;

    if(p_rsp_pkt[1] == 'c')
    {
        /* Call RSP Response handler for "Hc". Sets thread for
           step and continue operations. */
        rsp_status = rsp_set_exec_thread_id_handler(p_rsp_cmd_buff, p_rsp_resp_buff, p_rsp_resp_size);
    }
    else if(p_rsp_pkt[1] == 'g')
    {
        /* Construct RSP Response for "Hg". Sets thread for
           ('m','M','g','G') commands. */
        rsp_status = rsp_set_other_thread_id_handler(p_rsp_cmd_buff, p_rsp_resp_buff, p_rsp_resp_size);
    }
    else
    {
        /* Construct RSP Error Response */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("",p_rsp_resp_buff);

        rsp_status = RSP_STATUS_ERROR_RESP;
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_set_exec_thread_id_handler
*
*   DESCRIPTION
*
*       This function implements logic to support the RSP
*       packet 'Hc' - Set thread for step and continue operations.
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_set_exec_thread_id_handler(CHAR *  p_rsp_cmd_buff, CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;
    DBG_STATUS          dbg_status = DBG_STATUS_OK;
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    DBG_THREAD_ID       thread_id = 0;
    DBG_CMD             dbg_cmd;

    /* Make sure a current thread ID is available. */
    rsp_status = rsp_get_current_thread(&thread_id);

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Adjust pointer to RSP input buffer to start of command */
        p_rsp_pkt = p_rsp_pkt + Get_String_Size(RSP_SET_THREAD);

        /* Get TID */
        p_rsp_pkt = RSP_Get_TID_Data(p_rsp_pkt,&thread_id,RSP_PKT_END_CHAR);

        switch(thread_id)
        {
            case DBG_THREAD_ID_ANY :
            {
                /* Thread ID = 0 */

                /* Setup Debug Command arguments */
                dbg_cmd.op = DBG_CMD_OP_THREAD_GET_FIRST;
                dbg_cmd.op_param.thd_get_fst_nxt.p_thread_id = &thread_id;

                /* Call Debug Server */
                dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

                if(dbg_status == DBG_STATUS_OK)
                {
                    /* Update working thread for execution commands. */
                    rsp_pkt_mgmt.wrk_tid_exec = thread_id;

                    rsp_status = RSP_STATUS_OK;
                }
                else
                {
                    rsp_status = RSP_STATUS_FAILED;
                }

                break;

            } /* case */

            case DBG_THREAD_ID_ALL :
            {
                /* Thread ID = -1 */

                /* Update working thread for execution commands. */
                rsp_pkt_mgmt.wrk_tid_exec = thread_id;

                rsp_status = RSP_STATUS_OK;

                break;

            } /* case */

            default :
            {
                if(thread_id > 0)
                {
                    /* Update working thread for execution commands. */
                    rsp_pkt_mgmt.wrk_tid_exec = thread_id;
                    rsp_pkt_mgmt.wrk_tid_step = thread_id;
                }
                else
                {
                    rsp_status = RSP_STATUS_FAILED;
                }

                break;

            } /* default */

        } /* switch */
    }

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Construct RSP Response */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);
    }
    else
    {
        /* ERROR: Unknown internal error during processing. */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_set_other_thread_id_handler
*
*   DESCRIPTION
*
*       This function implements logic to support the RSP
*       packet 'Hg' - Set thread for ('m','M','g','G') commands.
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_set_other_thread_id_handler(CHAR *  p_rsp_cmd_buff, CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;
    DBG_STATUS          dbg_status = DBG_STATUS_OK;
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    DBG_THREAD_ID       thread_id = 0;
    DBG_CMD             dbg_cmd;

    /* Make sure a current thread ID is available. */
    rsp_status = rsp_get_current_thread(&thread_id);

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Adjust pointer to RSP input buffer to start of command */
        p_rsp_pkt = p_rsp_pkt + Get_String_Size("Hg");

        /* Get TID */
        p_rsp_pkt = RSP_Get_TID_Data(p_rsp_pkt,&thread_id,RSP_PKT_END_CHAR);

        switch(thread_id)
        {
            case DBG_THREAD_ID_ANY :
            {
                /* Thread ID = 0 */

                /* Setup Debug Command arguments */
                dbg_cmd.op = DBG_CMD_OP_THREAD_GET_FIRST;
                dbg_cmd.op_param.thd_get_fst_nxt.p_thread_id = &thread_id;

                /* Call Debug Server */
                dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

                if(dbg_status == DBG_STATUS_OK)
                {
                    /* Update working thread for other commands. */
                    rsp_pkt_mgmt.wrk_tid_other = thread_id;

                    rsp_status = RSP_STATUS_OK;
                }
                else
                {
                    rsp_status = RSP_STATUS_FAILED;
                }

                break;

            } /* case */

            case DBG_THREAD_ID_ALL :
            {

                /* Thread ID = -1 */

                /* DBG_THREAD_ID_ALL cannot be used for register read/write commands. */
                rsp_status = RSP_STATUS_FAILED;

                break;

            } /* case */

            default :
            {
                if(thread_id > 0)
                {
                    /* Update working thread for other commands. */
                    rsp_pkt_mgmt.wrk_tid_other = thread_id;

                    rsp_status = RSP_STATUS_OK;
                }
                else
                {
                    rsp_status = RSP_STATUS_FAILED;
                }

                break;

            } /* default */

        } /* switch */
    }

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Construct RSP Response */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);
    }
    else
    {
        /* ERROR: Unknown internal error during processing. */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_get_current_thread_handler
*
*   DESCRIPTION
*
*       This function implements logic to support the RSP
*       packet 'qC' - Return Current Thread.
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       RSP_STATUS_ERROR_RESP   - Indicates no current thread.
*
*************************************************************************/
static RSP_STATUS  rsp_get_current_thread_handler(CHAR *  p_rsp_cmd_buff, CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status;
    CHAR                str_array[30];
    DBG_THREAD_ID       cur_thread_id;

    /* Make sure a current thread ID is available. */
    rsp_status = rsp_get_current_thread(&cur_thread_id);

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Current thread found.  Ensure that the current thread ID is
           set correctly in the control block. */
        if (rsp_pkt_mgmt.current_tid == DBG_THREAD_ID_NONE)
        {
            rsp_pkt_mgmt.current_tid = cur_thread_id;

        }

        /* Start the response with an 'QC' */
        str_array[0] = 'Q';
        str_array[1] = 'C';

        /* Construct RSP Response use the debug current_tid */
         DBG_STR_String_From_UINT(&str_array[2],
                                  (UINT)rsp_pkt_mgmt.current_tid,
                                  DBG_STR_RADIX_HEXADECIMAL,
                                  DBG_STR_INC_PREFIX_DISABLE,
                                  DBG_STR_INC_LEADING_ZEROS_DISABLE);

        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt(str_array, p_rsp_resp_buff);
    }
    else
    {
        /* ERROR: Unable to get a current thread. */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_get_current_thread
*
*   DESCRIPTION
*
*       This function implements logic to get the current thread ID.
*       If a current thread is not available it returns the dbg_status
*       with the error.
*
*   INPUTS
*
*       cur_thread_id - Pointer to current thread parameter
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_RESOURCE_NOT_AVAIL - Indicates no currently running
*                                       thread found.
*
*       RSP_STATUS_FAILED - Indicates an internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_get_current_thread(DBG_THREAD_ID*  cur_thread_id)
{
    DBG_STATUS          dbg_status;
    RSP_STATUS          rsp_status;
    DBG_CMD             dbg_cmd;

    /* Setup Debug Command arguments */
    dbg_cmd.op = DBG_CMD_OP_THREAD_GET_CURRENT;
    dbg_cmd.op_param.thd_get_cur.p_thread_id = cur_thread_id;

    /* Call Debug Server */
    dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id,
                                 &dbg_cmd);

    switch (dbg_status)
    {
        case DBG_STATUS_OK :
        {
            /* Found a current thread that will be returned. */
            rsp_status = RSP_STATUS_OK;

            break;

        }

        case  DBG_STATUS_RESOURCE_UNAVAILABLE :
        {
            /* There is no current thread so check if there are any
               threads in the system at all. */
            dbg_cmd.op = DBG_CMD_OP_THREAD_GET_FIRST;
            dbg_cmd.op_param.thd_get_fst_nxt.p_thread_id = cur_thread_id;

            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id,
                                         &dbg_cmd);

            switch (dbg_status)
            {
                case DBG_STATUS_OK :
                {
                    /* Found a thread that will be returned. */
                    rsp_status = RSP_STATUS_OK;

                    break;

                }

                case DBG_STATUS_RESOURCE_UNAVAILABLE :
                {
                    /* No threads in system. */
                    rsp_status = RSP_STATUS_RESOURCE_NOT_AVAIL;

                    break;

                }

                default :
                {
                    /* ERROR: Unable to determine if threads in the
                       system. */
                    rsp_status = RSP_STATUS_FAILED;

                    break;

                }

            }

            break;

        }

        default :
        {
            /* ERROR: Unable to determine if there is a current thread. */
            rsp_status = RSP_STATUS_FAILED;

            break;

        }

    }

    return(rsp_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_v_packet_handler
*
*   DESCRIPTION
*
*       This function implements logic for the - "vXxxx" - command
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_v_packet_handler(CHAR *  p_rsp_cmd_buff, CHAR *    p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status;
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;

    if(p_rsp_pkt[1] == 'C')
    {
        if(p_rsp_pkt[5] == '?')
        {
            /* Build response to 'vCont?' packet.*/
            *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("vCont;c;s;t",p_rsp_resp_buff);

            rsp_status = RSP_STATUS_OK;
        }
        else
        {
            /* Call RSP Response handler for "vCont". */
            rsp_status = rsp_vcont_handler(p_rsp_cmd_buff, p_rsp_resp_buff, p_rsp_resp_size) ;
        }
    }
    else if(p_rsp_pkt[1] == 'S')
    {
        /* Construct RSP Response for "vStopped". */
        rsp_status = rsp_vstopped_handler(p_rsp_cmd_buff, p_rsp_resp_buff, p_rsp_resp_size);
    }
    else if(p_rsp_pkt[1] == 'K')
    {
        /* Construct RSP Response for "vKill". */
        rsp_status = rsp_vkill_handler(p_rsp_cmd_buff, p_rsp_resp_buff, p_rsp_resp_size);
    }
    else if(p_rsp_pkt[1] == 'R')
    {
        /* Construct RSP Response for "vRun". */
        rsp_status = rsp_vrun_handler(p_rsp_cmd_buff, p_rsp_resp_buff, p_rsp_resp_size);

    }
    else
    {
        /* Construct RSP Error Response */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("",p_rsp_resp_buff);

        rsp_status = RSP_STATUS_ERROR_RESP;
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_vcont_handler
*
*   DESCRIPTION
*
*       This function implements logic to support the RSP
*       packet '$vCont[;action[:tid]]...#'.  Currently supported
*       actions are 'c' continue, 's' step, and 't' stop.
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_vcont_handler(CHAR *     p_rsp_cmd_buff,
                                     CHAR *     p_rsp_resp_buff,
                                     UINT *     p_rsp_resp_size)
{
    RSP_STATUS               rsp_status = RSP_STATUS_OK;

    /* Parse vCont command string executing any default action only. */
    rsp_status = rsp_vcont_parse(p_rsp_cmd_buff,
                                 DBG_THREAD_ID_ALL);

    if (rsp_status == RSP_STATUS_OK)
    {
        /* Parse vCont command string executing any thread-specific
           actions only. */
        rsp_status = rsp_vcont_parse(p_rsp_cmd_buff,
                                     DBG_THREAD_ID_ANY);
    }

    /* Create response based on function status. */
    rsp_status = rsp_create_response_msg(rsp_status,
                                         p_rsp_resp_buff,
                                         p_rsp_resp_size);

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_vcont_parse
*
*   DESCRIPTION
*
*       This function parses a vCont command selectively performing
*       the actions.
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to vCont command string.
*
*       exec_thread_id          - Thread ID value that is used to
*                                 determine which actions are taken.
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_vcont_parse(CHAR *           p_rsp_cmd_buff,
                                   DBG_THREAD_ID    exec_thread_id)
{
    RSP_STATUS               rsp_status = RSP_STATUS_OK;
    CHAR *                   p_rsp_pkt = p_rsp_cmd_buff;
    CHAR *                   p_rsp_pkt2;
    DBG_THREAD_ID            thread_id;
    INT                      num_byts = 0;
    CHAR                     action;
    CHAR                     cur_delimiter = 0;

    /* Adjust pointer to the RSP input buffer to the first action. */
    p_rsp_pkt = p_rsp_pkt + Get_String_Size("vCont;");

    while ((rsp_status == RSP_STATUS_OK) &&
           (cur_delimiter != RSP_PKT_END_CHAR))
    {
        /* Set the target thread to be 'all threads' (default). */
        thread_id = DBG_THREAD_ID_ALL;

        /* Get the RSP vCont action 'c' continue 's' step 't' stop. */
        action = *p_rsp_pkt;

        /* Increment to the next character.*/
        p_rsp_pkt++;

        switch(*p_rsp_pkt)
        {
            case RSP_COLON_DELIMITER :
            {
                /* Get the thread ID */

                /* Adjust pointer to the start of the thread Id. */
                p_rsp_pkt++;
                p_rsp_pkt2 = p_rsp_pkt;

                /* Save the Character */
                cur_delimiter = *p_rsp_pkt2;

                /* Reset the buffer overflow byte count */
                num_byts = 0;

                /* Find the end of field Character for the thread ID. */
                while((cur_delimiter != RSP_PKT_END_CHAR) &&
                      (cur_delimiter != RSP_SEMI_COLON_DELIMITER))
                {
                    /* Go to the next character. */
                    p_rsp_pkt2++;

                    /* Save the character */
                    cur_delimiter = *p_rsp_pkt2;

                    /* Check for buffer overflow. */
                    if(num_byts++ > RSP_MAX_PACKET_SIZE_SUPPORTED)
                    {
                        /* If the '#' or ';' have not been found exit the loop */
                        rsp_status = RSP_STATUS_ERROR_RESP;

                        break;
                    }
                }

                if (cur_delimiter == RSP_PKT_END_CHAR)
                {
                    /* Get TID */
                    p_rsp_pkt = RSP_Get_TID_Data(p_rsp_pkt,
                                                 &thread_id,
                                                 RSP_PKT_END_CHAR);

                    rsp_status = RSP_STATUS_OK;
                }
                else
                {
                    /* Get TID and set the pointer to the next action command. */
                    p_rsp_pkt = RSP_Get_TID_Data(p_rsp_pkt,
                                                 &thread_id,
                                                 RSP_SEMI_COLON_DELIMITER);

                    rsp_status = RSP_STATUS_OK;
                }

                break;

            }

            case RSP_PKT_END_CHAR :
            {
                /* Packet end character found. */
                cur_delimiter = *p_rsp_pkt;

                rsp_status = RSP_STATUS_OK;

                break;

            }

            case RSP_SEMI_COLON_DELIMITER :
            {
                /* Update cur_delimiter to the current character. */
                cur_delimiter = *p_rsp_pkt;

                /* Increment to the next action .*/
                p_rsp_pkt++;

                rsp_status = RSP_STATUS_OK;

                break;

            }

            default :
            {
                /* ERROR: Invalid character in vCont message from
                   client. */

                rsp_status = RSP_STATUS_ERROR_RESP;

                break;

            }

        }

        /* Selectively execute only for default ('all') actions or for
           'any' (specific) thread actions or for only a single specific
           thread based on parameter value. */
        if ((rsp_status == RSP_STATUS_OK) &&
            (((exec_thread_id == DBG_THREAD_ID_ANY) &&
              (thread_id != DBG_THREAD_ID_ALL)) ||
             ((exec_thread_id == thread_id))))
        {
            switch(action)
            {
                case 'c' : /* Continue */
                {
                    /* Call the "vCont:c" resume or continue handler. */
                    rsp_status  = rsp_vcont_continue_handler(thread_id);

                    break;

                }

                case 's' :  /* Step */
                {
                    /* Call the "vCont:s" single step handler. */
                    rsp_status  = rsp_vcont_step_handler(thread_id);

                    break;

                }

                case 't' :  /* Stop */
                {
                    /* Call the "vCont:t" halt or stop handler. */
                    rsp_status  = rsp_vcont_stop_handler(thread_id);

                    break;

                }

                default :
                {
                    /* ERROR. Only 'c' 's' and 't' are supported. */

                    rsp_status = RSP_STATUS_ERROR_RESP;

                    break;

                }

            }

        }

    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_vcont_continue_handler
*
*   DESCRIPTION
*
*       This function implements logic to support the RSP
*       packet 'vCont;c' or continue.
*
*   INPUTS
*
*       thread_id               - Target thread ID
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_vcont_continue_handler(DBG_THREAD_ID     thread_id)
{
    RSP_STATUS               rsp_status = RSP_STATUS_OK;
    DBG_STATUS               dbg_status = DBG_STATUS_OK;
    DBG_CMD                  dbg_cmd;

    /* Determine how to proceed based on current debugging mode. */
    switch(rsp_pkt_mgmt.mode)
    {
        case RSP_MODE_ALL_STOP :
        case RSP_MODE_NON_STOP :
        {
            /* Continue thread(s). */
            dbg_cmd.op = DBG_CMD_OP_THREAD_GO;
            dbg_cmd.op_param.thd_go.thread_id = thread_id;

            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id,
                                         &dbg_cmd);

            if(dbg_status != DBG_STATUS_OK)
            {
                /* ERROR: Unable to resume execution of thread via the
                   debug engine. */
                rsp_status = RSP_STATUS_DEBUG_CALL_FAILED;

            }

            break;

        }

        default :
        {
            /* ERROR. A mode should have been specified. */
            rsp_status = RSP_STATUS_INVALID_MODE;

            break;

        }

    }

    return(rsp_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_vcont_step_handler
*
*   DESCRIPTION
*
*       This function implements logic to support the RSP
*       packet 'vCont;s' or step.
*
*   INPUTS
*
*       thread_id               - Target thread ID
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_vcont_step_handler(DBG_THREAD_ID     thread_id)
{
    RSP_STATUS               rsp_status = RSP_STATUS_OK;
    DBG_STATUS               dbg_status = DBG_STATUS_OK;
    DBG_CMD                  dbg_cmd;

    /* Determine how to proceed based on the current debugging mode. */
    switch(rsp_pkt_mgmt.mode)
    {
        case RSP_MODE_ALL_STOP :
        {
            /* Perform a single-step. */
            dbg_cmd.op = DBG_CMD_OP_THREAD_STEP;
            dbg_cmd.op_param.thd_step.step_thread_id = thread_id;
            dbg_cmd.op_param.thd_step.go_thread_id = DBG_THREAD_ID_ALL;
            dbg_cmd.op_param.thd_step.stop_thread_id = DBG_THREAD_ID_ALL;

            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

            if(dbg_status != DBG_STATUS_OK)
            {
                /* ERROR: Unable to step execution of thread via the
                   debug engine. */
                rsp_status = RSP_STATUS_DEBUG_CALL_FAILED;

            }

            break;

        }

        case RSP_MODE_NON_STOP :
        {
            /* Set the current thread. */
            rsp_pkt_mgmt.current_tid = thread_id;

            /* Perform a single-step. */
            dbg_cmd.op = DBG_CMD_OP_THREAD_STEP;
            dbg_cmd.op_param.thd_step.step_thread_id = thread_id;
            dbg_cmd.op_param.thd_step.stop_thread_id = thread_id;

            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

            if(dbg_status != DBG_STATUS_OK)
            {
                /* ERROR: Unable to step execution of thread via the
                   debug engine. */
                rsp_status = RSP_STATUS_DEBUG_CALL_FAILED;

            }
            break;

        }

        default :
        {
            /* ERROR. A mode should have been specified. */
            rsp_status = RSP_STATUS_INVALID_MODE;

            break;

        }

    }

    return(rsp_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_vcont_stop_handler
*
*   DESCRIPTION
*
*       This function implements logic to support the RSP
*       packet 'vCont;t' stop.
*
*   INPUTS
*
*       thread_id               - Target thread ID
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_vcont_stop_handler(DBG_THREAD_ID     thread_id)
{
    RSP_STATUS               rsp_status = RSP_STATUS_OK;
    DBG_STATUS               dbg_status = DBG_STATUS_OK;
    DBG_CMD                  dbg_cmd;

    /* Determine how to proceed based on the current debugging mode. */
    switch(rsp_pkt_mgmt.mode)
    {
        case RSP_MODE_ALL_STOP :
        {
            if ((thread_id != DBG_THREAD_ID_ANY) &&
                (thread_id != DBG_THREAD_ID_ALL))
            {
                /* Set the current thread. */
                rsp_pkt_mgmt.current_tid = thread_id;

            }

            /* Stop the thread. */
            dbg_cmd.op = DBG_CMD_OP_THREAD_STOP;
            dbg_cmd.op_param.thd_stop.thread_id = thread_id;

            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

            if (dbg_status != DBG_STATUS_OK)
            {
                /* ERROR: Unable to stop thread(s) via debug
                   engine. */
                rsp_status = RSP_STATUS_DEBUG_CALL_FAILED;

            }

            break;

        }

        case RSP_MODE_NON_STOP :
        {
            if ((thread_id != DBG_THREAD_ID_ANY) &&
                (thread_id != DBG_THREAD_ID_ALL))
            {
                /* Set the current thread. */
                rsp_pkt_mgmt.current_tid = thread_id;

            }

            /* Stop the thread. */
            dbg_cmd.op = DBG_CMD_OP_THREAD_STOP;
            dbg_cmd.op_param.thd_stop.thread_id = thread_id;

            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

            if (dbg_status != DBG_STATUS_OK)
            {
                /* ERROR: Unable to stop thread(s) via debug
                   engine. */
                rsp_status = RSP_STATUS_DEBUG_CALL_FAILED;

            }

            if (rsp_status == RSP_STATUS_OK)
            {
                /* Ensure that we are not in the middle of a thread
                   update sequence and that there is not already a
                   pending thread notification. */
                if ((rsp_pkt_mgmt.thread_seq_cnt == 0) &&
                    (rsp_pkt_mgmt.notify_tid == 0))
                {
                    /* Get the first thread from the queue. */
                    rsp_status = RSP_Thread_Queue_Receive(&rsp_pkt_mgmt.thread_queue,
                                                            &rsp_pkt_mgmt.notify_tid);

                    switch (rsp_status)
                    {
                        case RSP_STATUS_OK :
                        {
                            /* Set the % notification signal type */
                            rsp_pkt_mgmt.notify_sig_type = RSP_NOTIFY_SIGNAL_T00;

                            /* Start a notification timer */
                            rsp_status = RSP_Timer_Start(&rsp_pkt_mgmt.thread_timer,
                                                         RSP_NOTIFY_PACKET_INIT_TIMEOUT);

                            break;

                        }

                        case RSP_STATUS_RESOURCE_NOT_AVAIL :
                        {
                            /* Make sure notify_tid is set to zero */
                            rsp_pkt_mgmt.notify_tid = 0;

                            /* End of queue.  Update status. */
                            rsp_status = RSP_STATUS_OK;

                            break;

                        }

                        default :
                        {
                            /* Make sure notify_tid is set to zero */
                            rsp_pkt_mgmt.notify_tid = 0;

                            /* Let ERROR function status be returned
                               (no change to status). */

                            break;

                        }

                    }

                }

            }

            break;

        }

        default :
        {
            /* ERROR: A mode should have been specified. */
            rsp_status = RSP_STATUS_INVALID_MODE;

            break;

        }

    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_vstopped_handler
*
*   DESCRIPTION
*
*       This function implements logic to support the RSP
*       packet 'vStopped' .
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_vstopped_handler(CHAR *  p_rsp_cmd_buff, CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;
    DBG_THREAD_ID       thread_id;

    /* Stop the timer */
    rsp_status = RSP_Timer_Stop(&rsp_pkt_mgmt.thread_timer);

    if (rsp_status == RSP_STATUS_OK)
    {
        /* Attempt to get a thread out of the notification queue */
        rsp_status = RSP_Thread_Queue_Receive(&rsp_pkt_mgmt.thread_queue, &thread_id);
    }

    switch(rsp_status)
    {
        case RSP_STATUS_OK :
        {
            if (rsp_pkt_mgmt.thread_seq_cnt > 0)
            {
                /* Update the '?' sequence count of stopped threads.*/
                rsp_pkt_mgmt.thread_seq_cnt--;

            }

            /* Set the current thread. */
            rsp_pkt_mgmt.current_tid = thread_id;

            /* Build a stop reply packet. */
            rsp_status = rsp_create_stop_notify_packet(thread_id,
                                                        "$T05",
                                                        (CHAR *)p_rsp_resp_buff,
                                                        p_rsp_resp_size);

            if(rsp_status != RSP_STATUS_OK)
            {
                /* ERROR: Unable to create stop notify packet. */
                rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                                  p_rsp_resp_buff,
                                                  p_rsp_resp_size);
            }

            break;

        } /* case */

        case RSP_STATUS_RESOURCE_NOT_AVAIL :
        {
            /* End of queue. */
            *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);

            rsp_status = RSP_STATUS_OK;

            /* Set the notification thread ID to zero */
            rsp_pkt_mgmt.notify_tid = 0;

            break;

        } /* case */

        default :
        {
            /* ERROR: Unable to retrieve thread from queue. */
            rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);

            break;

        } /* default */

    } /* switch */


    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_thread_alive_handler
*
*   DESCRIPTION
*
*       This function implements logic for the - 'T' - command
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_thread_alive_handler(CHAR * p_rsp_cmd_buff, CHAR * p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    DBG_THREAD_ID       thread_id = 0;
    DBG_THREAD_STATUS   thread_status;
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status;
    DBG_CMD             dbg_cmd;

    /* Adjust pointer to RSP input buffer to start of command */
    p_rsp_pkt = p_rsp_pkt + Get_String_Size(RSP_THREAD_ALIVE);

    /* Get TID */
    p_rsp_pkt = RSP_Get_TID_Data(p_rsp_pkt,&thread_id,RSP_PKT_END_CHAR);

    /* Setup Debug Command arguments */
    dbg_cmd.op_param.thd_info.thread_id = thread_id;
    dbg_cmd.op_param.thd_info.p_thread_status = &thread_status;

    dbg_cmd.op = DBG_CMD_OP_THREAD_INFO;

    /* Call Debug Server */
    dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

    if(dbg_status == DBG_STATUS_OK)
    {
        /* Construct RSP Response */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);

        rsp_status = RSP_STATUS_OK;
    }
    else
    {
        /* ERROR: Unable to retrieve thread information from debug
           engine. */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);

}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_q_packet_handler
*
*   DESCRIPTION
*
*       This function implements logic for the - "QXxxx" - command
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_q_packet_handler(CHAR * p_rsp_cmd_buff, CHAR * p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;
    DBG_STATUS          dbg_status = DBG_STATUS_OK;
    DBG_CMD             dbg_cmd;
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    INT                 mode;

    /* Check for QStartNoAckMode */
    if((p_rsp_pkt[1] == 'S') && (p_rsp_pkt[6] == 'N'))
    {
        /* Set the no ack flag. */
        rsp_pkt_mgmt.q_start_no_ack_mode = NU_TRUE;

        /* The mode was successfully changed. */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);

        rsp_status = RSP_STATUS_OK;
    }
    else if((p_rsp_pkt[1] == 'N') && (p_rsp_pkt[4] == 'S'))
    {
        /* QNonStop mode packet. */

        /* Adjust pointer to RSP input buffer to end of "QNonStop". */
        p_rsp_pkt = p_rsp_pkt + Get_String_Size(RSP_Q_PACKET);

        /* Get the mode. */
        p_rsp_pkt = RSP_Get_Int_Data(p_rsp_pkt,&mode,RSP_PKT_END_CHAR);

        switch(mode)
        {
            case RSP_MODE_ALL_STOP :
            {
                /* Enter All-Stop mode */

                /* Set mode in RSP control block. */
                rsp_pkt_mgmt.mode = RSP_MODE_ALL_STOP;

                /* Stop all threads. */
                dbg_cmd.op = DBG_CMD_OP_THREAD_STOP;
                dbg_cmd.op_param.thd_stop.thread_id = DBG_THREAD_ID_ALL;

                dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id,&dbg_cmd);

                if(dbg_status != DBG_STATUS_OK)
                {
                    rsp_status = RSP_STATUS_FAILED;
                }

                break;

            } /* case */

            case RSP_MODE_NON_STOP :
            {
                /* Enter non-stop mode. */

                /* Set mode in RSP control block. */
                rsp_pkt_mgmt.mode = RSP_MODE_NON_STOP;

                /* Indicate successful mode change. */
                rsp_status = RSP_STATUS_OK;

                break;

            } /* case */

            default :
            {
                /* Error */
                rsp_status = RSP_STATUS_FAILED;

                break;

            } /* default */

        } /* switch */

        if (rsp_status == RSP_STATUS_OK)
        {
            /* The mode was successfully changed. */
            *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);
        }
        else
        {
            /* ERROR: Unable to retrieve new non-stop state from client
               message. */
            rsp_status = rsp_create_error_msg(RSP_ERROR_E01_MSG,
                                              p_rsp_resp_buff,
                                              p_rsp_resp_size);
        }
    }
    else
    {
        /* The message is not currently being handled.  Construct RSP Error Response */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("",p_rsp_resp_buff);

        rsp_status = RSP_STATUS_ERROR_RESP;
    }

    return(rsp_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_extended_mode_handler
*
*   DESCRIPTION
*
*       This function implements logic for the - '!' - command
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_extended_mode_handler(CHAR *  p_rsp_cmd_buff, CHAR *  p_rsp_resp_buff, UINT * p_rsp_resp_size)
{

    /* Don't allow extended mode when a static application is running. */
    if(rsp_pkt_mgmt.static_app_mode == NU_FALSE)
    {
        /* Construct RSP Response */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);
    }
    else
    {
        /* Send command not supported if a static application is running. */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("",p_rsp_resp_buff);
    }

    return(RSP_STATUS_OK);

}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_vrun_handler
*
*   DESCRIPTION
*
*       This function implements logic for the - "vRun" - command
*
*       Packet format: '$vRun;filename[;arguments]...#'
*       The square brackets indicate optional parameters.
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_vrun_handler(CHAR *  p_rsp_cmd_buff, CHAR *    p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;
    DBG_STATUS          dbg_status;
    DBG_THREAD_ID       thread_id;
    DBG_CMD             dbg_cmd;
    RSP_CALLBACK        rsp_callback;
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    CHAR                data[RSP_MAX_PACKET_SIZE_SUPPORTED];
    CHAR **             argv = (CHAR **)NU_NULL;
    INT                 argc = 0;
    NU_TASK *           p_os_thread;
    OPTION              prev_preempt;
    OPTION              prev_priority;


    /* Make sure the callback function is registered before processing the packet. */

    if(RSP_GET_CALLBACK_HANDLER_POINTER(RSP_CALLBACK_VRUN) != NU_NULL)
    {
        /* Get the thread that the vRun handler is executing on. */
        p_os_thread = NU_Current_Task_Pointer();

        /* Change preemption state of this thread so that any dynamic
           application threads created will not pre-empt it. */
        prev_preempt = NU_Change_Preemption(NU_NO_PREEMPT);

        /* Adjust the priority of this thread to prevent preemption. */
        prev_priority = NU_Change_Priority(p_os_thread,
                                        RSP_VRUN_THREAD_ADJUSTED_PRIORITY);

        if(rsp_status == RSP_STATUS_OK)
        {
            /* Get argc */

            /* Adjust pointer to RSP input buffer to end of "vRun;". */
            p_rsp_pkt = p_rsp_pkt + Get_String_Size("vRun;");

            /* Determine the number of arguments or argc value. */
            rsp_status = rsp_vrun_get_argc(p_rsp_pkt, &argc);

            if(rsp_status == RSP_STATUS_OK)
            {
                /* Add argc to the callback structure. */
                rsp_callback.op_parm.rsp_vrun.argc = argc;
            }
            else
            {
                /* ERROR: Bad message from client. */
                rsp_status = rsp_create_error_msg(RSP_ERROR_E01_MSG,
                                                p_rsp_resp_buff,
                                                p_rsp_resp_size);
            }
        }

        if(rsp_status == RSP_STATUS_OK)
        {
            /* Allocate memory for argv */

            argv = (CHAR **)DBG_System_Memory_Allocate((argc * sizeof(INT)),
                                                      DBG_SYSTEM_MEMORY_ALLOC_NO_ALIGN,
                                                      DBG_SYSTEM_MEMORY_ALLOC_CACHED,
                                                      DBG_SYSTEM_MEMORY_ALLOC_ZERO_INIT,
                                                      DBG_SYSTEM_MEMORY_ALLOC_NO_ALIGN);
            /* Check for a NULL pointer. */
            if (argv == NU_NULL)
            {
                /* Handle Error condition. Memory allocation failed. */
                rsp_status = rsp_create_error_msg(RSP_ERROR_E03_MEM,
                                                p_rsp_resp_buff,
                                                p_rsp_resp_size);

            }
        }

        if (rsp_status == RSP_STATUS_OK)
        {
            /* Build argv */

            rsp_status = rsp_vrun_get_argv(p_rsp_pkt, &data[0], argv, argc);

            if (rsp_status == RSP_STATUS_OK)
            {
                /* Add argv to the callback structure. */
                rsp_callback.op_parm.rsp_vrun.argv = argv;
            }
            else
            {
                /* ERROR: Bad message from client. */
                rsp_status = rsp_create_error_msg(RSP_ERROR_E01_MSG,
                                                p_rsp_resp_buff,
                                                p_rsp_resp_size);
            }

        }

        if (rsp_status == RSP_STATUS_OK)
        {
            /* Call the dle vRun service callback function. */

            /* Add the vRun context value to the callback structure */
            rsp_callback.p_context = RSP_GET_CALLBACK_CONTEXT(RSP_CALLBACK_VRUN);

            /* Call the dle vRun callback handler */
            rsp_status = RSP_CALLBACK_HANDLER_EXECUTE(RSP_CALLBACK_VRUN, &rsp_callback);

            if (rsp_status != RSP_STATUS_OK)
            {
                /* ERROR: DLE service call failed. */
                rsp_status = rsp_create_error_msg(RSP_ERROR_E04_DLE,
                                                  p_rsp_resp_buff,
                                                  p_rsp_resp_size);
            }

        }

        if (rsp_status == RSP_STATUS_OK)
        {
            /* Get the thread ID */

            dbg_cmd.op = DBG_CMD_OP_THREAD_ID;
            dbg_cmd.op_param.thd_id.p_thread_id = &thread_id;

            /* Get the task control block returned from the dynamic
               loading service. */
            dbg_cmd.op_param.thd_id.p_os_thread = (NU_TASK *)rsp_callback.op_parm.rsp_vrun.pTaskCb;

            /* Call Debug Engine to get the Agent assigned thread ID. */
            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

            if(dbg_status != DBG_STATUS_OK)
            {
                /* ERROR: Unable to get the TID via debug engine */
                rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                                p_rsp_resp_buff,
                                                p_rsp_resp_size);
            }
        }

        if (rsp_status == RSP_STATUS_OK)
        {
            /* Set the application main thread's OS group status. */
            dbg_cmd.op = DBG_CMD_OP_THREAD_GRP;
            dbg_cmd.op_param.thd_grp.thread_id = thread_id;
            dbg_cmd.op_param.thd_grp.thread_group = DBG_THREAD_GROUP_APPLICATION;

            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

            if (dbg_status != DBG_STATUS_OK)
            {
                /* Skip generating this message if rsp_status has already failed. */
                if (rsp_status == RSP_STATUS_OK)
                {
                    /* ERROR: Unable to restore system protection. */
                    rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                                    p_rsp_resp_buff,
                                                    p_rsp_resp_size);
                }
            }
        }

        if (rsp_status == RSP_STATUS_OK)
        {
            /* Stop the new thread */

            dbg_cmd.op = DBG_CMD_OP_THREAD_STOP;
            dbg_cmd.op_param.thd_stop.thread_id = thread_id;

            /* Call Debug Server */
            dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

            if(dbg_status != DBG_STATUS_OK)
            {
                /* ERROR: Unable to stop dynamic application via debug
                   engine. */
                rsp_status = rsp_create_error_msg(RSP_ERROR_E02_DBG_ENG,
                                                p_rsp_resp_buff,
                                                p_rsp_resp_size);
            }
        }

        if (rsp_status == RSP_STATUS_OK)
        {
            /* Update the RSP control block. */

            /* Set the current thread. */
            rsp_pkt_mgmt.current_tid = thread_id;

            /* Indicate an application is running */
            rsp_pkt_mgmt.dle_app_cnt++;

            /* Update the application offset value. */
            rsp_pkt_mgmt.app_offset = rsp_callback.op_parm.rsp_vrun.app_offset;

        }

        if (rsp_status == RSP_STATUS_OK)
        {
            /* Create the GDB client response. */

            /* The debug engine loads the thread ID into the queue.
               It needs to be removed from the queue to avoid sending
               the same stop reply again. */
            rsp_status = RSP_Thread_Queue_Receive(&rsp_pkt_mgmt.thread_queue,\
                                                    &thread_id);
            if(rsp_status == RSP_STATUS_OK)
            {
                /* Build a stop reply packet. */
                rsp_status = rsp_create_stop_notify_packet(thread_id,
                                                        "$T05",
                                                        (CHAR *)p_rsp_resp_buff,
                                                        p_rsp_resp_size);
            }
            else
            {
                /* ERROR: Failure to receive thread from queue. */
                rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                                    p_rsp_resp_buff,
                                                    p_rsp_resp_size);
            }
        }

        if (argv != NU_NULL)
        {
            /* Deallocate memory for argv */

            dbg_status = DBG_System_Memory_Deallocate((VOID *)argv);

            if (dbg_status != DBG_STATUS_OK)
            {
                /* Skip generating this message if rsp_status has already failed. */
                if (rsp_status == RSP_STATUS_OK)
                {
                    /* ERROR: Memory deallocation failed. */
                    rsp_status = rsp_create_error_msg(RSP_ERROR_E03_MEM,
                                                    p_rsp_resp_buff,
                                                    p_rsp_resp_size);
                }
            }
        }

        /* Restore the priority of the this thread. */
        (VOID)NU_Change_Priority(p_os_thread,
                                 prev_priority);

        /* Restore the preemption state of this thread. */
        (VOID)NU_Change_Preemption(prev_preempt);
    }
    else
    {
        /* The message is not currently being handled.  Construct RSP Error Response */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("",p_rsp_resp_buff);

        rsp_status = RSP_STATUS_ERROR_RESP;

    }

    return(rsp_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_vrun_get_argc
*
*   DESCRIPTION
*
*       This function implements logic for the - "vRun" - command. It counts
*       the number of arguments, or argc value, in the vRun command.
*
*   INPUTS
*
*       p_rsp_pkt               - Pointer to RSP packet
*       p_argc                  - Pointer to RSP response buffer
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_vrun_get_argc(CHAR *  p_rsp_pkt, INT * p_argc)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;
    CHAR *              p_chk_pkt = p_rsp_pkt;
    INT                 byts_chkd = 0;
    INT                 local_argc = 0;

    /* Check for an arguments list. '$vRun;filename[;arguments]...#'
       The square brackets indicate an optional parameter. */

    /* This loop is used to determine the number of arguments passed in from the client.
       The first argument will be the file name and then any parameters. A count of
       how many parameters have been passed is needed so memory can be allocated. */
    while(byts_chkd < RSP_MAX_PACKET_SIZE_SUPPORTED)
    {
        if (*p_chk_pkt == RSP_SEMI_COLON_DELIMITER)
        {
            /* Count the number of ';' separated arguments. */
            local_argc++;

            /* Increment the pointer and check the next byte. */
            p_chk_pkt++;
        }
        else if (*p_chk_pkt == RSP_PKT_END_CHAR)
        {
            /* The '#' character is the end of the last argument passed in from the client. */
            local_argc++;

            /* Exit the loop*/
            break;
        }
        else
        {
            /* Increment the pointer and check the next byte. */
            p_chk_pkt++;
        }

        /* Increment the bytes checked count. */
        byts_chkd++;
    }

    if(byts_chkd < RSP_MAX_PACKET_SIZE_SUPPORTED)
    {
        *p_argc = local_argc;

        rsp_status = RSP_STATUS_OK;
    }
    else
    {
        *p_argc = 0;

        rsp_status = RSP_STATUS_FAILED;
    }

    return(rsp_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_vrun_get_argv
*
*   DESCRIPTION
*
*       This function implements logic for the - "vRun" - command. It
*       creates argv from the rsp vRun command packet.
*
*   INPUTS
*
*       p_rsp_pkt               - Pointer to RSP packet
*       p_vrun_data             - Pointer to buffer for argv
*       p_argv                  - Pointer to argv
*       argc                    - Number of arguments to process
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*
*************************************************************************/
static RSP_STATUS  rsp_vrun_get_argv(CHAR * p_rsp_pkt, CHAR * p_vrun_data, CHAR ** p_argv, INT argc)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;
    CHAR *              p_data = p_vrun_data;
    CHAR *              p_local_pkt = p_rsp_pkt;
    INT                 size = 0;
    INT                 idx;

    /* There is always at least one argument, the path.  Get the path
       and any additional arguments from the RSP packet. */
    for(idx = 0; idx < argc; idx++)
    {
        if (idx < (argc-1))
        {
            /* Save the pointer to the argument. */
            p_argv[idx] = p_data;

            /* Get the argument. Convert the hex encoded string to a binary string. */
            size = RSP_Ascii_Hex_Array_2_Bin_Array(p_local_pkt, p_data, RSP_SEMI_COLON_DELIMITER, \
                                            RSP_MAX_PACKET_SIZE_SUPPORTED);

            /* Null terminate the string. */
            p_data[size] = '\0';

            /* Move the pointer to the start of the next string. */
            p_data += (size + 1);

            /* Increment the RSP packet to the next argument.  Use (size * 2) to move the
            * pointer since the RSP packet has ascii encoded hex.  Or two characters for each
            * ascii byte.
            * Example: '1' is 0x31 in ascii.
            * Ascii encode hex would be two bytes '3' 0x33 and '1' 0x31. */
            p_local_pkt += ((size*2) + 1);

        }
        else
        {
            /* Get the last argument. */

            /* Save the pointer to the argument. */
            p_argv[idx] = p_data;

            /* Get the argument. Convert the hex encoded string to a binary string. */
            size = RSP_Ascii_Hex_Array_2_Bin_Array(p_local_pkt, p_data, RSP_PKT_END_CHAR, \
                                            RSP_MAX_PACKET_SIZE_SUPPORTED);

            /* Null terminate the string. */
            p_data[size] = '\0';
        }

        /* Argument size should be greater then zero */
        if (size == 0)
        {
            rsp_status = RSP_STATUS_FAILED;

            break;
        }
    }


    return(rsp_status);
}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_vkill_handler
*
*   DESCRIPTION
*
*       This function implements logic for the 'kill' and 'vKill' - commands
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_vkill_handler(CHAR *  p_rsp_cmd_buff, CHAR *  p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;
    RSP_CALLBACK        rsp_callback;
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    INT                 pid;

    /* Handle common debug engine and rsp shutdown tasks */
    rsp_status = rsp_shutdown();

    if(rsp_status == RSP_STATUS_OK)
    {
        /* Make sure there is a registered callback handler. */
        if(RSP_GET_CALLBACK_HANDLER_POINTER(RSP_CALLBACK_VKILL) != NU_NULL)
        {
            /* Get the PID if this is the vKill command. */
            if((p_rsp_pkt[0] == 'v') && (p_rsp_pkt[1] == 'K'))
            {
                /* Adjust the pointer to RSP input buffer to the pid "vKill;pid#" */
                p_rsp_pkt = p_rsp_pkt + Get_String_Size("vKill;");

                /* Get PID */
                RSP_Get_Int_Data(p_rsp_pkt,&pid,RSP_PKT_END_CHAR);

                /* Setup callback arguments */
                rsp_callback.op_parm.rsp_vkill.pid = pid;
            }
            else
            {
                /* This is a kill command */
                rsp_callback.op_parm.rsp_vkill.pid = -1;
            }

            /* Get the vKill context. */
            rsp_callback.p_context = RSP_GET_CALLBACK_CONTEXT(RSP_CALLBACK_VKILL);

            /* Call the vKill service callback function. */
            rsp_status = RSP_CALLBACK_HANDLER_EXECUTE(RSP_CALLBACK_VKILL, &rsp_callback);

            if((rsp_status == RSP_STATUS_OK) && (rsp_pkt_mgmt.dle_app_cnt > 0))
            {
                /* Decrement the application running counter. */
                rsp_pkt_mgmt.dle_app_cnt--;

                /* Update the application offset value to indicate no
                    dynamic application. */
                rsp_pkt_mgmt.app_offset = 0x0;

            }
        }
    }

    if (rsp_status == RSP_STATUS_OK)
    {
        /* Construct RSP Response */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("OK",p_rsp_resp_buff);
    }
    else
    {
        /* ERROR: Failure to shutdown debug engine and RSP component. */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E05_INTERNAL,
                                        p_rsp_resp_buff,
                                        p_rsp_resp_size);
    }

    return(rsp_status);

}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_shutdown
*
*   DESCRIPTION
*
*       This function handles common shutdown tasks for rsp_vkill_handler
*       and rsp_disconnect_handler.
*
*   INPUTS
*
*       VOID
*
*   OUTPUTS
*
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       RSP_STATUS_FAILED       - Indicates operation failed.
*
*************************************************************************/
static RSP_STATUS  rsp_shutdown(VOID)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;
    DBG_STATUS          dbg_status = DBG_STATUS_OK;
    DBG_CMD             dbg_cmd;


    if (rsp_status == RSP_STATUS_OK)
    {
        /* Stop the timer */
        rsp_status = RSP_Timer_Stop(&rsp_pkt_mgmt.thread_timer);

    }

    if (rsp_status == RSP_STATUS_OK)
    {
        /* Reset the queue. */
        rsp_status = RSP_Thread_Queue_Reset(&rsp_pkt_mgmt.thread_queue);

    }

    if (rsp_status == RSP_STATUS_OK)
    {
        /* Clear all breakpoints. */
        dbg_cmd.op = DBG_CMD_OP_BREAKPOINT_CLEAR_ALL;

        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

        if (dbg_status != DBG_STATUS_OK)
        {
            /* ERROR: Unable to clear all breakpoints. */
            rsp_status = RSP_STATUS_FAILED;

        }

    }

    if (rsp_status == RSP_STATUS_OK)
    {
        /* Set all threads running. */
        dbg_cmd.op = DBG_CMD_OP_THREAD_GO;
        dbg_cmd.op_param.thd_go.thread_id = DBG_THREAD_ID_ALL;

        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id, &dbg_cmd);

        if (dbg_status != DBG_STATUS_OK)
        {
            /* ERROR: Unable to resume execution of all threads. */
            rsp_status = RSP_STATUS_FAILED;

        }

    }


    return(rsp_status);

}


/*************************************************************************
*
*   FUNCTION
*
*       rsp_thread_extra_info_handler
*
*   DESCRIPTION
*
*       This function implements logic to support the RSP
*       packet 'qThreadExtraInfo'.
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_thread_extra_info_handler(CHAR *  p_rsp_cmd_buff, CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    CHAR *              p_rsp_pkt = p_rsp_cmd_buff;
    DBG_THREAD_ID       thread_id = 0;
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status = DBG_STATUS_OK;
    DBG_CMD             dbg_cmd;
    DBG_THREAD_STATUS   thread_status;
    CHAR                thread_info_hex[DBG_THREAD_INFO_STR_SIZE * 2];

    /* Adjust pointer to RSP input buffer to start of command */
    p_rsp_pkt = p_rsp_pkt + Get_String_Size("qThreadExtraInfo,");

    /* Get TID */
    p_rsp_pkt = RSP_Get_TID_Data(p_rsp_pkt,&thread_id,RSP_PKT_END_CHAR);

    if(dbg_status == DBG_STATUS_OK)
    {
        /* Retrieve information about the thread. */
        dbg_cmd.op = DBG_CMD_OP_THREAD_INFO;
        dbg_cmd.op_param.thd_info.thread_id = thread_id;
        dbg_cmd.op_param.thd_info.p_thread_status = &thread_status;

        dbg_status = DBG_ENG_Command(rsp_pkt_mgmt.dbg_session_id,
                                     &dbg_cmd);

        /* Convert the thread information into an ASCII hex
           representation. */

        (VOID)RSP_Convert_Str_2_Hex_Str(&dbg_cmd.op_param.thd_info.thread_info_str[0],
                                       &thread_info_hex[0]);

        /* Construct RSP Response */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt(&thread_info_hex[0],
                                                 p_rsp_resp_buff);

        rsp_status = RSP_STATUS_OK;
    }
    else
    {
        /* ERROR: Error in message from client. */
        rsp_status = rsp_create_error_msg(RSP_ERROR_E01_MSG,
                                          p_rsp_resp_buff,
                                          p_rsp_resp_size);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_offsets_handler
*
*   DESCRIPTION
*
*       This function implements logic to support the RSP
*       packet 'qOffsets'.
*
*   INPUTS
*
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_offsets_handler(CHAR *   p_rsp_resp_buff, UINT * p_rsp_resp_size)
{
    RSP_STATUS          rsp_status = RSP_STATUS_OK;
    DBG_STATUS          dbg_status;
    CHAR                resp_str[17];
    size_t              resp_str_size;

    /* Determine how to proceed based on the current offset address. */

    if ((UINT)rsp_pkt_mgmt.app_offset == 0x0)
    {
        /* An offset address of zero indicates a static image, so no
          offset information is passed to the client. */

        strcpy(&resp_str[0],
               "");

    }
    else
    {
        /* A non-zero offset address indicates a dynamic image, so pass
           the information to the client. */

        strcpy(&resp_str[0],
               "TextSeg=");

        resp_str_size = strlen(&resp_str[0]);

        dbg_status = DBG_STR_String_From_UINT(&resp_str[resp_str_size],
                                              (UINT)rsp_pkt_mgmt.app_offset,
                                              DBG_STR_RADIX_HEXADECIMAL,
                                              DBG_STR_INC_PREFIX_DISABLE,
                                              DBG_STR_INC_LEADING_ZEROS_ENABLE);

        if (dbg_status != DBG_STATUS_OK)
        {
            /* ERROR: Unable to translate offset address to string. */

            rsp_status = RSP_STATUS_FAILED;

        }

    }

    if (rsp_status == RSP_STATUS_OK)
    {
        /* Convert string to RSP packet. */
        *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt(&resp_str[0],
                                                 p_rsp_resp_buff);
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       rsp_default_handler
*
*   DESCRIPTION
*
*       This function implements logic to handle unsupported RSP packets
*
*   INPUTS
*
*       p_rsp_cmd_buff          - Pointer to RSP packet
*       p_rsp_resp_buff         - Pointer to RSP response buffer
*       p_rsp_resp_size         - Pointer to RSP response buffer size
*
*   OUTPUTS
*
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
static RSP_STATUS  rsp_default_handler(CHAR *  p_rsp_cmd_buff, CHAR *  p_rsp_resp_buff, UINT * p_rsp_resp_size)
{

    /* Construct RSP Response */
    *p_rsp_resp_size = RSP_Convert_Str_2_Rsp_Pkt("",p_rsp_resp_buff);

    return(RSP_STATUS_OK);

}


/*************************************************************************
*
*   FUNCTION
*
*       RSP_Register_Debug_Server_Callback
*
*   DESCRIPTION
*
*       This is the registration function for services that need to
*       register a callback function to handle an RSP packet.
*
*   INPUTS
*
*       index                   - Index into the callback array.
*       pCallback               - Pointer to a callback function.
*
*   OUTPUTS
*
*
*       RSP_STATUS_OK           - Indicates successful operation.
*       <other>                 - Indicates other internal error.
*
*************************************************************************/
RSP_STATUS  RSP_Register_Debug_Server_Callback(RSP_CALLBACK_IDX_TYPE index, VOID * pCallback, VOID * p_context)
{
    RSP_STATUS          rsp_status = RSP_STATUS_FAILED;

    if (pCallback != NU_NULL)
    {
        /* Register callback handler function. */
        RSP_REGISTER_CALLBACK_HANDLER(index, pCallback);

        /* Register user defined callback context.
         * this value is returned to registered function
         * when a callback occurs. */
        RSP_REGISTER_CALLBACK_CONTEXT(index, p_context);

        rsp_status = RSP_STATUS_OK;
    }

    return(rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       RSP_Check_Packet
*
*   DESCRIPTION
*
*       Processes the received RSP data byte and ascertains if a
*       valid and complete RSP packet has been received. If a valid packet
*       is received returns the appropriate action to be taken by the
*       calling function.
*
*       Note that if the data contains multiple packets the first valid
*       packet will be identified and processed by this function.
*
*   INPUTS
*
*       p_data - Pointer to RSP packet.
*
*       data-Size - Pointer to RSP packet size.
*
*       p_rsp_pkt - Pointer to start of RSP packet after processing.
*
*       p_rsp_pkt_size - Pointer to RSP packet size after processing.
*
*       p_data_processed_size - Return parameter that will be updated to
*                               indicate the size (in bytes) of the data
*                               processed (regardless of whether a packet
*                               was found or not).
*
*   OUTPUTS
*
*       RSP_NO_AVAILABLE_PACKET  - No valid RSP packet available.
*
*       RSP_FULL_PACKET_RECEIVED - A valid RSP packet has been received.
*
*       RSP_HALT_EVENT_RECEIVED  - A halt event has been received.
*
*       <other>                  - Pointer to received data packet is
*                                  updated in the RMD communication
*                                  control blocks auxiliary buffer.
*
*************************************************************************/
UINT    RSP_Check_Packet(CHAR *             p_data,
                                 UINT       data_size,
                                 VOID *             p_rsp_pkt,
                                 UINT *     p_rsp_pkt_size,
                                 UINT *     p_data_processed_size)
{
    CHAR *  p_in_buff;
    UINT8   rsp_byt;
    UINT8   pkt_chk_sum = 0;
    UINT8   computed_chk_sum = 0;
    UINT    rsp_pkt_flag = RSP_PARSING_INITIAL;
    UINT    rsp_pkt_byt_cound = 0;
    UINT    ret_val = 0;
    UINT    data_processed_size = 0;

    /* Obtain local pointer to input data */
    p_in_buff  = p_data;

    while ((data_processed_size < data_size) &&
           (ret_val == 0))
    {
        /* Obtain current byte of RSP packet data*/
        rsp_byt = *p_in_buff & 0xff;

        /* If data size is one, check for servicing all possible one byte packets */
        if((data_size == 1)&&(rsp_pkt_flag == RSP_PARSING_INITIAL))
        {
            /* Check if the packet received is a halt 'BREAK' command */
            if(rsp_byt == RSP_HALT_PKT_CHAR)
            {
                /* Full packet received, return process packet */
                ret_val  = RSP_PROCESS_PACKET;

                *((UINT *)p_rsp_pkt) = (UINT)(p_in_buff);
            }

            /* Check if the packet received is a ACK */
            if(rsp_byt == RSP_ACK_CHAR)
            {
                /* Ack received for previous packet sent, no action */
                ret_val  = RSP_NO_ACTION;
            }

            /* Check if the packet received is a ACK */
            if(rsp_byt == RSP_NO_ACK_CHAR)
            {
                /* Transmission error with previous packet, request retransmission */
                ret_val  = RSP_REQ_SERVER_RETRANSMISSION;
            }
        }

        if (rsp_pkt_flag == RSP_VALIDATE_CHKSUM)
        {
            pkt_chk_sum |= Hex_2_Dec(rsp_byt);

            if(pkt_chk_sum != computed_chk_sum)
            {
                /* Transmission error with previous packet, request retransmission */
                ret_val  = RSP_REQ_CLIENT_RETRANSMISSION;
            }
            else
            {
                /* Full packet received, return process packet */
                ret_val  = RSP_ACK_AND_PROCESS_PACKET;
            }
        }

        if (rsp_pkt_flag == RSP_PARSE_PKT_CHKSUM)
        {
           pkt_chk_sum = Hex_2_Dec(rsp_byt);
           pkt_chk_sum = pkt_chk_sum << 4;

           rsp_pkt_flag = RSP_VALIDATE_CHKSUM;
        }

        if (rsp_byt == RSP_PKT_END_CHAR)
        {
           rsp_pkt_flag = RSP_PARSE_PKT_CHKSUM;

           /* Increment RSP packet byte count */
           rsp_pkt_byt_cound++;
        }

        if (rsp_pkt_flag == RSP_PARSE_PKT_BODY)
        {
           /* Compute packet checksum */
           computed_chk_sum = rsp_byt + computed_chk_sum;

           /* Increment RSP packet byte count */
           rsp_pkt_byt_cound++;
        }

        if (rsp_byt == RSP_PKT_START_CHAR)
        {
           /* Mark start of RSP packet */
           *((UINT *)p_rsp_pkt) = (UINT)(p_in_buff + 1);

           rsp_pkt_flag = RSP_PARSE_PKT_BODY;
        }

        /* Increment input buffer */
        p_in_buff++;

        /* Update data processed size. */
        data_processed_size++;
    }

    /* Load RSP packet count */
    *p_rsp_pkt_size = rsp_pkt_byt_cound;

    /* Load data processed size. */
    *p_data_processed_size = data_processed_size;

    /* Return no packet available status */
    return(ret_val);
}
