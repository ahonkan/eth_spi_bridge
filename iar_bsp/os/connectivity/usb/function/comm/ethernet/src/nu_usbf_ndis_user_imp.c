/**************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***********************************************************************
*
* FILE NAME
*
*       nu_usbf_ndis_user_imp.c
*
* COMPONENT
*
*       Nucleus USB Function Software : Remote NDIS User Driver
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for NDIS User Driver.
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       NDISF_Decode_Encap_Command
*       NDISF_Prep_Initialize_Msg_Cmplt
*       NDISF_Send_Notification
*       NDISF_Proc_Initialize_Msg
*       NDISF_Proc_Query_Msg
*       NDISF_Prep_NF_QUERY_MSG_Cmplt
*       NDISF_Proc_Set_Msg
*       NDISF_Prep_Set_Msg_Cmplt
*       NDISF_Proc_Reset_Msg
*       NDISF_Prep_NF_RESET_MSG_Cmplt
*       NDISF_Proc_Halt_Msg
*       NDISF_Proc_Keepalive_Msg
*       NDISF_Prep_Keepalive_Msg_Cmplt
*       NDISF_Proc_Query_Oid
*       NDISF_Proc_Set_Oid
*       NDISF_Hndl_Oid_Gen_Supp_List
*       NDISF_Hndl_Oid_Gen_Vndr_Dvr_Ver
*       NDISF_Hndl_Oid_Gen_Max_Frm_Size
*       NDISF_Hndl_Oid_802_3_List_Size
*       NDISF_Hndl_Oid_802_3_Curr_Addr
*       NDISF_Hndl_Oid_802_3_Pert_Addr
*       NDISF_Hndl_Oid_Gen_Total_Size
*       NDISF_Hndl_Oid_Gen_Conn_Status
*       NDISF_Hndl_Oid_Gen_Link_Speed
*       NDISF_Hndl_Oid_Gen_Xmit_Ok
*       NDISF_Hndl_Oid_Gen_Rcv_Ok
*       NDISF_Hndl_Oid_Gen_Xmit_Error
*       NDISF_Hndl_Oid_Gen_Rcv_Error
*       NDISF_Hndl_Oid_Gen_Vendor_Id
*       NDISF_Hndl_Oid_Gen_Statistics
*
* DEPENDENCIES
*
*       nu_usb.h                    All USB definitions
*
*
************************************************************************/
#include "connectivity/nu_usb.h"
/* ==================================================================== */

/*************************************************************************
* FUNCTION
*
*       NDISF_Decode_Encap_Command
*
* DESCRIPTION
*
*       Communication class Encapsulated command parsing routine.
*
* INPUTS
*       cb       Pointer to user control block.
*       pdevice  Handle to the logical device control block.
*       data     Pointer to command buffer.
*       len      Length of command buffer.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*************************************************************************/

STATUS NDISF_Decode_Encap_Command(NU_USBF_NDIS_USER *cb,
                                  NU_USBF_NDIS_DEVICE *pdevice,
                                  UINT8 *data,
                                  UINT16 len)
{
    STATUS status;
    UINT32 messageType = 0;
    UINT32 query_data_len;
    UINT32 response_status;

    NU_ASSERT(cb);
    NU_ASSERT(data);
    NU_ASSERT(pdevice);

    messageType |= data[NF_MSG_TYPE_OFF + 0];
    messageType |= (data[NF_MSG_TYPE_OFF + 1]<< 8 );
    messageType |= (data[NF_MSG_TYPE_OFF + 2]<< 16 );
    messageType |= (data[NF_MSG_TYPE_OFF + 3]<< 24 );

    /* This message is sent by the host to a Remote NDIS device to
     * initialize the network connection. It is sent through the control
     * channel and only when the device is not in the rndis-initialized
     * state.
     */
    if(messageType == NF_INIT_MSG)
    {
        /* Send NF_INITIALIZE_CMPLT response to host.*/
        NDISF_Proc_Initialize_Msg(pdevice, data, len);

        /* Prepare the completion message. */
        NDISF_Prep_Initialize_Msg_Cmplt(pdevice,
                                data,
                                len,
                                pdevice->response_array,
                                &(pdevice->response_length));

        /* Send RESPONSE_AVAILABLE notification to host.*/
        status = NDISF_Send_Notification(cb, NF_RESPONSE_AVAILABLE,pdevice);

    }

    /*
     * This message is sent to a Remote NDIS device from a host when it
     * needs to query the device for its characteristics, statistics
     * information, or status. The parameter or statistics counter being
     * queried for is identified by means of an NDIS Object Identifier
     * (OID). The host may send NF_QUERY_MSG to the device
     * through the control channel at any time that the device is in either
     * the rndis-initialized or rndis-data-initialized state. The
     * Remote NDIS device will respond to this message by sending a
     * NF_QUERY_CMPLT to the host.
     */
    else if(messageType == NF_QUERY_MSG)
    {
        /* Parse the incoming Query message. */
        NDISF_Proc_Query_Msg(pdevice, data, len);

        /* Process the received OID. */
        status = NDISF_Proc_Query_Oid(pdevice,&pdevice->input_oid,
                    &pdevice->response_array[NF_QUERY_MSG_CMPLT_LENGTH],
                    &query_data_len,
                    &response_status);
        if(status == NU_SUCCESS)
        {
            /* Prepare the completion message. */
            NDISF_Prep_NF_QUERY_MSG_Cmplt(cb,
                                pdevice->input_oid.request_id,
                                response_status,
                                query_data_len,
                                pdevice->response_array,
                                &pdevice->response_length);

            /* Send RESPONSE_AVAILABLE notification to host.*/
            status = NDISF_Send_Notification(cb, NF_RESPONSE_AVAILABLE, pdevice);

        }
    }

    /* This message is sent to a Remote NDIS device from a host, when it
     * requires to set the value of some operational parameter on the
     * device. The specific parameter being set is identified by means of
     * an Object Identifier (OID), and the value it is to be set to is
     * contained in an information buffer sent along with the message.
     * The host may send NF_SET_MSG to the device through the
     * control channel at any time that the device is in either the
     * rndis-initialized or rndis-data-initialized state. The Remote NDIS
     * device will respond to this message by sending a
     * NF_SET_CMPLT to the host.
     */
    else if(messageType == NF_SET_MSG)
    {
        /* Parse the incoming Query message. */
        NDISF_Proc_Set_Msg(pdevice, data, len);

        /* Process the received OID. */
        status = NDISF_Proc_Set_Oid(cb, &pdevice->input_oid,
                    &response_status);
        if(status == NU_SUCCESS)
        {
            /* Prepare the completion message. */
            NDISF_Prep_Set_Msg_Cmplt(cb,
                                pdevice->input_oid.request_id,
                                response_status,
                                pdevice->response_array,
                                &pdevice->response_length);

            /* Send RESPONSE_AVAILABLE notification to host.*/
            status = NDISF_Send_Notification(cb, NF_RESPONSE_AVAILABLE, pdevice);

        }
    }

    else if(messageType == NF_RESET_MSG)
    {
        /* Parse receive message. */
        status = NDISF_Proc_Reset_Msg(pdevice);
        if(status == NU_SUCCESS)
        {
            /* Prepare the completion message. */
            NDISF_Prep_NF_RESET_MSG_Cmplt(cb,
                                pdevice->input_oid.request_id,
                                NF_STATUS_SUCCESS,
                                pdevice->response_array,
                                &pdevice->response_length);

            /* Send RESPONSE_AVAILABLE notification to host.*/
            status = NDISF_Send_Notification(cb, NF_RESPONSE_AVAILABLE, pdevice);

        }
    }
    else if(messageType == NF_KEEPALIVE_MSG)
    {
        /* Parse receive message. */
        status = NDISF_Proc_Keepalive_Msg(pdevice, data, len);
        if(status == NU_SUCCESS)
        {
            /* Prepare the completion message. */
            NDISF_Prep_Keepalive_Msg_Cmplt(pdevice,
                                pdevice->input_oid.request_id,
                                NF_STATUS_SUCCESS,
                                pdevice->response_array,
                                &pdevice->response_length);

            /* Send RESPONSE_AVAILABLE notification to host.*/
            status = NDISF_Send_Notification(cb, NF_RESPONSE_AVAILABLE, pdevice);

        }
    }
    else if(messageType == NF_HALT_MSG)
    {
        /* Process the received message. */
        status = NDISF_Proc_Halt_Msg(cb);
    }
    else
    {
        status = NU_SUCCESS;
    }

    return status;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Prep_Initialize_Msg_Cmplt
*
* DESCRIPTION
*
*       Initialize completion message generation routine.
*
* INPUTS
*       cb           Pointer to user control block.
*       data         Pointer to command buffer.
*       len          Length of command buffer.
*       data_out     Pointer to buffer for completion message.
*       data_out_len Pointer for length of completed message.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*************************************************************************/
VOID NDISF_Prep_Initialize_Msg_Cmplt(
                                     NU_USBF_NDIS_DEVICE *pdevice,
                                     UINT8 *data,
                                     UINT16 len,
                                     UINT8 * data_out,
                                     UINT32 * data_out_len)
{

    /* Prepare the INITIALIZE_CMPLT message in provided buffer. */
    *(UINT32*)((UINT32)data_out+NF_MSG_TYPE_OFF) =
                        HOST_2_LE32(NF_INITIALIZE_CMPLT);
    *(UINT32*)((UINT32)data_out+NF_MSG_LENGTH_OFF) =
                        HOST_2_LE32(NF_INITIALIZE_CMPLT_LENGTH);
    *(UINT32*)((UINT32)data_out+NF_INIT_MSG_CMPLT_REQ_ID_OFF) =
                        HOST_2_LE32(pdevice->last_request_id);
    *(UINT32*)((UINT32)data_out+NF_INIT_MSG_CMPLT_STATUS_OFF) =
                        HOST_2_LE32(NF_STATUS_SUCCESS);
    *(UINT32*)((UINT32)data_out+NF_INIT_MSG_CMPLT_MAJ_VER_OFF) =
                        HOST_2_LE32(NF_MAJOR_VERSION);
    *(UINT32*)((UINT32)data_out+NF_INIT_MSG_CMPLT_MIN_VER_OFF) =
                        HOST_2_LE32(NF_MINOR_VERSION);
    *(UINT32*)((UINT32)data_out+NF_INIT_MSG_CMPLT_DEV_FLGS_OFF) =
                        HOST_2_LE32(NF_DF_CONNECTIONLESS);
    *(UINT32*)((UINT32)data_out+NF_INIT_MSG_CMPLT_MEDIUM_OFF) =
                        HOST_2_LE32(NF_MEDIUM_802_3);
    *(UINT32*)((UINT32)data_out+NF_INIT_MSG_CMPLT_PKT_MSGS_OFF) =
                        HOST_2_LE32(NF_MAXPKTPERMSG);
    *(UINT32*)((UINT32)data_out+NF_INIT_MSG_CMPLT_XFR_SIZE_OFF) =
                        HOST_2_LE32(NF_MAX_DATA_XFER_SIZE);
    *(UINT32*)((UINT32)data_out+NF_INIT_MSG_CMPLT_ALIN_FAC_OFF) =
                        HOST_2_LE32(NF_PKTALIGNFACTOR);
    *(UINT32*)((UINT32)data_out+NF_INIT_MSG_CMPLT_AFLIST_OFF) =
                        HOST_2_LE32(NF_ZERO);
    *(UINT32*)((UINT32)data_out+NF_INIT_MSG_CMPLT_AF_SIZE_OFF) =
                        HOST_2_LE32(NF_ZERO);

    /* Update the length of message. */
    *data_out_len = NF_INITIALIZE_CMPLT_LENGTH;
}
/*************************************************************************
* FUNCTION
*
*       NDISF_Send_Notification
*
* DESCRIPTION
*
*       Routine used to send notification to Host.
*
* INPUTS
*       cb          Pointer to user control block.
*       notif       Notification code for sending to Host.
*       pdevice     Pointer to the user device control block.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Send_Notification(NU_USBF_NDIS_USER *cb,
                               UINT8 notif,
                               NU_USBF_NDIS_DEVICE *pdevice)
{
    STATUS status = NU_USB_INVLD_ARG;

    USBF_COMM_USER_NOTIFICATION * user_notif;
    NU_ASSERT(cb);
    NU_ASSERT(pdevice);

    user_notif = &pdevice->user_notif;

    /* Only RESPONSE_AVAILABLE is supported by this driver. */
    if(notif == NF_RESPONSE_AVAILABLE)
    {

        pdevice->resp_in_progress = 1;

        /* Initialize the notification structure. */
        user_notif->notification = notif;
        user_notif->notif_value = NF_ZERO;
        user_notif->data = NU_NULL;
        user_notif->length = 0;

        /* Call the communication class driver's API to send notification.
        */
        status = NU_USBF_COMM_Send_Notification(
                                    ((NU_USBF_USER_COMM *)cb)->mng_drvr,
                                    user_notif,
                                    pdevice->handle);
    }
    return status;
}
/*************************************************************************
* FUNCTION
*
*       NDISF_Proc_Initialize_Msg
*
* DESCRIPTION
*
*       Routine for processing initialize message.
*
* INPUTS
*       cb          Pointer to user control block.
*       pdevice     Pointer to the logical device.
*       data        Pointer to command buffer.
*       len         Length of command buffer.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*************************************************************************/
STATUS NDISF_Proc_Initialize_Msg(NU_USBF_NDIS_DEVICE *pdevice,
                                 UINT8 *data,
                                 UINT16 len)
{
    UINT32 tempvar;
    NU_ASSERT(pdevice);
    NU_ASSERT(data);

    /* Retrieve information from NF_INIT_MSG control message.*/
    tempvar = *(UINT32*)((UINT32)data + NF_INIT_MSG_REQ_ID_OFF);
    pdevice->last_request_id = LE32_2_HOST(tempvar);

    tempvar = *(UINT32*)((UINT32)data +
                                    NF_INIT_MSG_MAJ_VER_OFF);
    pdevice->major_version = LE32_2_HOST(tempvar);

    tempvar = *(UINT32*)((UINT32)data +
                                    NF_INIT_MSG_MIN_VER_OFF);
    pdevice->minor_version = LE32_2_HOST(tempvar);

    tempvar = *(UINT32*)((UINT32)data +
                                    NF_INIT_MSG_MAX_XFER_SIZE_OFF);
    pdevice->max_transfer_size = LE32_2_HOST(tempvar);

    return NU_SUCCESS;
}
/*************************************************************************
* FUNCTION
*
*       NDISF_Proc_Query_Msg
*
* DESCRIPTION
*
*       Routine for processing Query message from Host.
*
* INPUTS
*       cb          Pointer to user device control block.
*       data        Pointer to command buffer.
*       len         Length of command buffer.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*************************************************************************/
STATUS NDISF_Proc_Query_Msg(NU_USBF_NDIS_DEVICE *cb,
                                UINT8 *data,
                                UINT16 len)
{
    UINT32 offset;
    UINT32 tempvar;

    /* Decode and retrieve fields from QUERY_MSG. */
    tempvar = *(UINT32*)((UINT32)data + NF_QUERY_MSG_REQ_ID_OFF);
    cb->input_oid.request_id = LE32_2_HOST(tempvar);

    tempvar = *(UINT32*)((UINT32)data+NF_QUERY_MSG_OID_OFF);
    cb->input_oid.ndis_oid = LE32_2_HOST(tempvar);

    tempvar = *(UINT32*)((UINT32)data + NF_QUERY_MSG_INF_BUF_LEN_OFF);
    cb->input_oid.input_buffer_length = LE32_2_HOST(tempvar);

    /* Read the data associated with Query in the local buffer. */
    if((cb->input_oid.input_buffer_length)>0x0L)
    {
        tempvar = *(UINT32*)((UINT32)data+NF_QUERY_MSG_INF_BUFOFFSET_OFF);
        offset = LE32_2_HOST(tempvar);

        if(cb->input_oid.input_buffer_length <= NF_QUERY_MSG_MAX_INPUT_BUF_LEN)
        {
            memcpy(cb->input_oid.input_buffer,
                &data[NF_QUERY_MSG_REQ_ID_OFF+offset],
                cb->input_oid.input_buffer_length);
        }
        else
        {
            return NU_INVALID_POINTER;
        }
    }
    return NU_SUCCESS;
}
/*************************************************************************
* FUNCTION
*
*       NDISF_Prep_NF_QUERY_MSG_Cmplt
*
* DESCRIPTION
*
*       Routine for generation of Query completion message.
*
* INPUTS
*       cb             Pointer to user control block.
*       reqid          Request ID for current Query.
*       status         Status for the current Query.
*       inf_buffer_len Length of information buffer.
*       data_out       Pointer to buffer for completion message.
*       data_out_len   Pointer for length of completed message.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
VOID NDISF_Prep_NF_QUERY_MSG_Cmplt(NU_USBF_NDIS_USER *cb,
                                        UINT32 reqid,
                                        UINT32 status,
                                        UINT32 inf_buffer_len,
                                        UINT8 * data_out,
                                        UINT32 * data_out_len)
{
    /* Initialize the QUERY_CMPLT message in provided buffer. */
    *(UINT32*)((UINT32)data_out+NF_MSG_TYPE_OFF) =
                    HOST_2_LE32(NF_QUERY_MSG_CMPLT);
    *(UINT32*)((UINT32)data_out+NF_MSG_LENGTH_OFF) =
        HOST_2_LE32(NF_QUERY_MSG_CMPLT_LENGTH + inf_buffer_len);
    *(UINT32*)((UINT32)data_out+NF_QUERY_MSG_CMPLT_REQ_ID_OFF) =
                        HOST_2_LE32(reqid);
    *(UINT32*)((UINT32)data_out+NF_QUERY_MSG_CMPLT_STATUS_OFF) =
                        HOST_2_LE32(status);
    *(UINT32*)((UINT32)data_out+NF_QUERY_MSG_INF_BUF_LEN_OFF) =
                HOST_2_LE32(inf_buffer_len);
    *(UINT32*)((UINT32)data_out+NF_QUERY_MSG_INF_BUFOFFSET_OFF) = 0;

    /* Update the query data length in header, data is already placed after
     * header.
     */
    if(inf_buffer_len != 0)
    {
        *(UINT32*)((UINT32)data_out+NF_QUERY_MSG_INF_BUFOFFSET_OFF) =
                          HOST_2_LE32(NF_QUERY_MSG_CMPLT_LENGTH-
                                            NF_QUERY_MSG_REQ_ID_OFF);
    }

    /* Update total length of message. */
    *data_out_len = NF_QUERY_MSG_CMPLT_LENGTH + inf_buffer_len;
}
/*************************************************************************
* FUNCTION
*
*       NDISF_Proc_Set_Msg
*
* DESCRIPTION
*
*       Routine for processing Set message from USB Host.
*
* INPUTS
*       cb          Pointer to user device control block.
*       data        Pointer to command buffer.
*       len         Length of command buffer.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Proc_Set_Msg(NU_USBF_NDIS_DEVICE *cb,
                                UINT8 *data,
                                UINT16 len)
{
    UINT32 offset;
    UINT32 tempvar;

    /* Decode and retrieve fields from Set_MSG. */
    tempvar = *(UINT32*)((UINT32)data+NF_QUERY_MSG_REQ_ID_OFF);
    cb->input_oid.request_id = LE32_2_HOST(tempvar);

    tempvar = *(UINT32*)((UINT32)data+NF_QUERY_MSG_OID_OFF);
    cb->input_oid.ndis_oid = LE32_2_HOST(tempvar);

    tempvar = *(UINT32*)((UINT32)data+NF_QUERY_MSG_INF_BUF_LEN_OFF);
    cb->input_oid.input_buffer_length = LE32_2_HOST(tempvar);

    tempvar = *(UINT32*)((UINT32)data+NF_QUERY_MSG_INF_BUFOFFSET_OFF);
    offset = LE32_2_HOST(tempvar);

    /* Copy the set message data in local buffer for further processing. */
    if((cb->input_oid.input_buffer_length > 0)&&
       (cb->input_oid.input_buffer_length < NF_QUERY_MSG_MAX_INPUT_BUF_LEN))
    {
        memcpy(cb->input_oid.input_buffer,
            (UINT8*)((UINT32)data+NF_QUERY_MSG_REQ_ID_OFF+offset),
            cb->input_oid.input_buffer_length);
    }

    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Prep_Set_Msg_Cmplt
*
* DESCRIPTION
*
*       Routine for generation of set message completion routine.
*
* INPUTS
*       cb             Pointer to user control block.
*       reqid          Request ID for current Query.
*       status         Status for the current Query.
*       data_out       Pointer to buffer for completion message.
*       data_out_len   Pointer for length of completed message.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
VOID NDISF_Prep_Set_Msg_Cmplt(NU_USBF_NDIS_USER *cb,
                              UINT32 reqid,
                              UINT32 status,
                              UINT8 * data_out,
                              UINT32 * data_out_len)
{

    /* Initialize the SET_CMPLT message in provided buffer. */
    *(UINT32*)((UINT32)data_out+NF_MSG_TYPE_OFF) =
                    HOST_2_LE32(NF_SET_MSG_CMPLT);
    *(UINT32*)((UINT32)data_out+NF_MSG_LENGTH_OFF) =
        HOST_2_LE32(NF_SET_MSG_CMPLT_LENGTH);
    *(UINT32*)((UINT32)data_out+NF_QUERY_MSG_CMPLT_REQ_ID_OFF) =
                        HOST_2_LE32(reqid);
    *(UINT32*)((UINT32)data_out+NF_QUERY_MSG_CMPLT_STATUS_OFF) =
                        HOST_2_LE32(status);

    /* Update the length of message. */
    *data_out_len = NF_SET_MSG_CMPLT_LENGTH;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Proc_Reset_Msg
*
* DESCRIPTION
*
*       Routine for processing of Reset message from USB Host.
*
* INPUTS
*       pdevice          Pointer to user device control block.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Proc_Reset_Msg(NU_USBF_NDIS_DEVICE *pdevice)
{
    /* Reset the statistics of driver. */

    pdevice->frame_tx_ok = 0;
    pdevice->frame_rx_ok = 0;
    pdevice->frame_tx_err = 0;
    pdevice->frame_rx_err = 0;

    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Prep_NF_RESET_MSG_Cmplt
*
* DESCRIPTION
*
*       Routine for generation of Reset completion message.
*
* INPUTS
*       cb             Pointer to user control block.
*       reqid          Request ID for current Query.
*       status         Status for the current Query.
*       data_out       Pointer to buffer for completion message.
*       data_out_len   Pointer for length of completed message.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/

VOID NDISF_Prep_NF_RESET_MSG_Cmplt(NU_USBF_NDIS_USER *cb,
                                  UINT32 reqid,
                                  UINT32 status,
                                  UINT8 * data_out,
                                  UINT32 * data_out_len)
{

    /* Initialize RESET_CMPLT message in provided buffer. */
    *(UINT32*)((UINT32)data_out+NF_MSG_TYPE_OFF) =
                    HOST_2_LE32(NF_RESET_CMPLT);
    *(UINT32*)((UINT32)data_out+NF_MSG_LENGTH_OFF) =
        HOST_2_LE32(NF_SET_RESET_CMPLT_LENGTH);
    *(UINT32*)((UINT32)data_out+NF_RESET_MSG_CMPLT_STATUS_OFF) =
                        HOST_2_LE32(status);
    *(UINT32*)((UINT32)data_out+NF_RESET_MSG_CMPLT_ADR_RES_OFF) =
                        HOST_2_LE32(NF_ZERO);

    /* Update total length. */
    *data_out_len = NF_SET_RESET_CMPLT_LENGTH;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Proc_Halt_Msg
*
* DESCRIPTION
*
*       Routine for processing Halt message from USB Host.
*
* INPUTS
*       cb          Pointer to user control block.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Proc_Halt_Msg(NU_USBF_NDIS_USER *cb)
{

    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Proc_Keepalive_Msg
*
* DESCRIPTION
*
*       Routine for processing Keep-alive message from USB Host.
*
* INPUTS
*       cb          Pointer to user control block.
*       data        Pointer to command buffer.
*       len         Length of command buffer.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Proc_Keepalive_Msg(NU_USBF_NDIS_DEVICE *cb,
                                UINT8 *data,
                                UINT16 len)
{
    UINT32 tempvar;
    /* Decode and retrieve fields from Set_MSG. */
    tempvar = *(UINT32*)((UINT32)data+NF_QUERY_MSG_REQ_ID_OFF);
    cb->input_oid.request_id = LE32_2_HOST(tempvar);
    return NU_SUCCESS;
}
/*************************************************************************
* FUNCTION
*
*       NDISF_Prep_Keepalive_Msg_Cmplt
*
* DESCRIPTION
*
*       Routine for generation of Keep-alive message completion message.
*
* INPUTS
*       cb             Pointer to user device control block.
*       reqid          Request ID for current Query.
*       status         Status for the current Query.
*       data_out       Pointer to buffer for completion message.
*       data_out_len   Pointer for length of completed message.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
VOID NDISF_Prep_Keepalive_Msg_Cmplt(NU_USBF_NDIS_DEVICE *cb,
                                    UINT32 reqid,
                                    UINT32 status,
                                    UINT8 * data_out,
                                    UINT32 * data_out_len)
{
    /* Prepare KEEPALIVE_CMPLT message in provided buffer. */
    *(UINT32*)((UINT32)data_out+NF_MSG_TYPE_OFF) =
                    HOST_2_LE32(NF_KEEPALIVE_CMPLT);
    *(UINT32*)((UINT32)data_out+NF_MSG_LENGTH_OFF) =
        HOST_2_LE32(NF_SET_KEEPALIVE_CMPLT_LENGTH);
    *(UINT32*)((UINT32)data_out+NF_QUERY_MSG_CMPLT_REQ_ID_OFF) =
                        HOST_2_LE32(reqid);
    *(UINT32*)((UINT32)data_out+NF_QUERY_MSG_CMPLT_STATUS_OFF) =
                        HOST_2_LE32(status);

    /* Update length of message. */
    *data_out_len = NF_SET_KEEPALIVE_CMPLT_LENGTH;
}

/*************************************************************************
* FUNCTION
*       NDISF_Proc_Query_Oid
*
* DESCRIPTION
*       Routine for processing Query OID of Query message from the Host.
*
* INPUTS
*
*       pdevice                 Pointer to the user device.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Proc_Query_Oid(
                    NU_USBF_NDIS_DEVICE *pdevice,
                    USBF_NDIS_USER_OID * input_oid,
                    UINT8 * buffer,
                    UINT32 * buffer_len,
                    UINT32 * query_response_status)
{
    STATUS status;

    NU_ASSERT(pdevice);
    NU_ASSERT(input_oid);
    NU_ASSERT(buffer);
    NU_ASSERT(buffer_len);

    /* Process the received query OID. */
    if(input_oid->ndis_oid == NF_OID_GEN_SUPPORTED_LIST)
    {
        status = NDISF_Hndl_Oid_Gen_Supp_List(pdevice,
                                            input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }
    else if(input_oid->ndis_oid == NF_OID_GEN_VENDOR_DVR_VERSION)
    {
        status = NDISF_Hndl_Oid_Gen_Vndr_Dvr_Ver(pdevice, input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }

    else if(input_oid->ndis_oid == NF_OID_GEN_MAXIMUM_FRAME_SIZE)
    {
        status = NDISF_Hndl_Oid_Gen_Max_Frm_Size(pdevice, input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }

    else if(input_oid->ndis_oid == NF_OID_802_3_MAXIMUM_LIST_SIZE)
    {
        status = NDISF_Hndl_Oid_802_3_List_Size(pdevice, input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }
    else if(input_oid->ndis_oid == NF_OID_802_3_CURRENT_ADDRESS)
    {
        status = NDISF_Hndl_Oid_802_3_Curr_Addr(pdevice, input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }
    else if(input_oid->ndis_oid == NF_OID_802_3_PERMANENT_ADDRESS)
    {
        status = NDISF_Hndl_Oid_802_3_Pert_Addr(pdevice, input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }
    else if(input_oid->ndis_oid == NF_OID_GEN_MAXIMUM_TOTAL_SIZE)
    {
        status = NDISF_Hndl_Oid_Gen_Total_Size(pdevice, input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }
    else if(input_oid->ndis_oid == NF_OID_GEN_MED_CONNECT_STATUS)
    {
        status = NDISF_Hndl_Oid_Gen_Conn_Status(pdevice, input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }
    else if(input_oid->ndis_oid == NF_OID_GEN_LINK_SPEED)
    {
        status = NDISF_Hndl_Oid_Gen_Link_Speed(pdevice, input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }
    else if(input_oid->ndis_oid == NF_OID_GEN_XMIT_OK)
    {
        status = NDISF_Hndl_Oid_Gen_Xmit_Ok(pdevice,input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }
    else if(input_oid->ndis_oid == NF_OID_GEN_RCV_OK)
    {
        status = NDISF_Hndl_Oid_Gen_Rcv_Ok(pdevice, input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }
    else if(input_oid->ndis_oid == NF_OID_GEN_XMIT_ERROR)
    {
        status = NDISF_Hndl_Oid_Gen_Xmit_Error(pdevice, input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }
    else if(input_oid->ndis_oid == NF_OID_GEN_RCV_ERROR)
    {
        status = NDISF_Hndl_Oid_Gen_Rcv_Error(pdevice, input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }
    else if(input_oid->ndis_oid == NF_OID_GEN_VENDOR_ID)
    {
        status = NDISF_Hndl_Oid_Gen_Vendor_Id(input_oid,
                                            buffer,
                                            buffer_len,
                                            query_response_status);
    }
    else if(input_oid->ndis_oid == NF_OID_GEN_RCV_NO_BUFFER)
    {

        buffer[0]=0;
        buffer[1]=0;
        buffer[2]=0;
        buffer[3]=0;
        *buffer_len=4;
        *query_response_status = NF_STATUS_SUCCESS;
        status = NU_SUCCESS;
    }
    else if(input_oid->ndis_oid == NF_OID_GEN_STATISTICS)
    {
        status = NDISF_Hndl_Oid_Gen_Statistics(pdevice,
                                               input_oid,
                                               buffer,
                                               buffer_len,
                                               query_response_status);
    }
    else
    {
        status = NU_USB_NOT_SUPPORTED;
    }

    return status;
}
/*************************************************************************
* FUNCTION
*
*       NDISF_Proc_Set_Oid
*
* DESCRIPTION
*
*       Routine for processing Set OID from the Set message from Host.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Set OID structure.
*       query_response_status   Pointer for Set response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/

STATUS NDISF_Proc_Set_Oid(NU_USBF_NDIS_USER *cb,
                       USBF_NDIS_USER_OID * input_oid,
                       UINT32 * query_response_status)
{
    NU_ASSERT(input_oid);

    /* We do not process the set OID internally, only pass it to
     * application.
     */
    if(cb->rndis_ioctl)
    {
       cb->rndis_ioctl((NU_USBF_USER*)cb, input_oid->ndis_oid,
                    input_oid->input_buffer,
                    input_oid->input_buffer_length);
    }

    *query_response_status = NF_STATUS_SUCCESS;
    return NU_SUCCESS;
}
/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_Gen_Supp_List
*
* DESCRIPTION
*
*       OID_GEN_SUPPORTED_LIST query processing routine.
*
* INPUTS
*       cb                      Pointer to user device control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_Gen_Supp_List(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{
    UINT32 * buff = (UINT32*)buffer;
    UINT16 index=0;
    /* Prepare a buffer of all the OIDs supported. */
    buff[index++] = HOST_2_LE32(NF_OID_GEN_SUPPORTED_LIST          );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_HARDWARE_STATUS         );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_MEDIA_SUPPORTED         );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_MEDIA_IN_USE            );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_MAXIMUM_FRAME_SIZE      );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_LINK_SPEED              );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_TRANSMIT_BLOCK_SIZE     );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_RECEIVE_BLOCK_SIZE      );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_VENDOR_ID               );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_VENDOR_DESCRIPTION      );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_VENDOR_DVR_VERSION      );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_CURR_PACKET_FILTER      );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_MAXIMUM_TOTAL_SIZE      );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_MED_CONNECT_STATUS      );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_XMIT_OK                 );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_RCV_OK                  );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_XMIT_ERROR              );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_RCV_ERROR               );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_RCV_NO_BUFFER           );
    buff[index++] = HOST_2_LE32(NF_OID_802_3_PERMANENT_ADDRESS     );
    buff[index++] = HOST_2_LE32(NF_OID_802_3_CURRENT_ADDRESS       );
    buff[index++] = HOST_2_LE32(NF_OID_802_3_MULTICAST_LIST        );
    buff[index++] = HOST_2_LE32(NF_OID_802_3_MAXIMUM_LIST_SIZE     );
    buff[index++] = HOST_2_LE32(NF_OID_802_3_RCV_ERROR_ALIGN       );
    buff[index++] = HOST_2_LE32(NF_OID_802_3_XMIT_ONE_COLL         );
    buff[index++] = HOST_2_LE32(NF_OID_802_3_XMIT_MORE_COLL        );
    buff[index++] = HOST_2_LE32(NF_OID_GEN_STATISTICS              );
    *buffer_len = index*sizeof(UINT32);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_Gen_Vndr_Dvr_Ver
*
* DESCRIPTION
*
*       OID_GEN_VENDOR_DRIVER_VERSION query processing routine.
*
* INPUTS
*       cb                      Pointer to user device control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_Gen_Vndr_Dvr_Ver(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{
    UINT32 * buff = (UINT32*)buffer;
    UINT16 index=0;

    /* Fill the required data in provided buffer. */
    buff[index++] = HOST_2_LE32(NF_VENDOR_DRIVER_VERSION);
    *buffer_len = index*sizeof(UINT32);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}
/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_Gen_Max_Frm_Size
*
* DESCRIPTION
*
*       OID_GEN_MAXIMUM_FRAME_SIZE query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_Gen_Max_Frm_Size(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{
    UINT32 * buff = (UINT32*)buffer;
    UINT16 index=0;

    /* Fill the required data in provided buffer. */
    buff[index++] = HOST_2_LE32(NF_MAXIMUM_FRAME_SIZE);
    *buffer_len = index*sizeof(UINT32);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_802_3_List_Size
*
* DESCRIPTION
*
*       OID_802_3_MAXIMUM_LIST_SIZE query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_802_3_List_Size(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{
    UINT32 * buff = (UINT32*)buffer;
    UINT16 index=0;

    /* Fill the required data in provided buffer. */
    buff[index++] = HOST_2_LE32(NF_MAXIMUM_LIST_SIZE);
    *buffer_len = index*sizeof(UINT32);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_802_3_Curr_Addr
*
* DESCRIPTION
*
*       OID_802_3_CURRENT_ADDRESS query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_802_3_Curr_Addr(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{
    /* Fill the required data in provided buffer. */
    memcpy(buffer, (UINT8 *)(cb->mac_address), NF_MAC_ADDR_LEN);
    *buffer_len = NF_MAC_ADDR_LEN;

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_802_3_Pert_Addr
*
* DESCRIPTION
*
*       OID_802_3_PERMANENT_ADDRESS query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_802_3_Pert_Addr(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{
    /* Fill the required data in provided buffer. */
    memcpy(buffer, (UINT8 *)(cb->mac_address), NF_MAC_ADDR_LEN);
    *buffer_len = NF_MAC_ADDR_LEN;

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_Gen_Total_Size
*
* DESCRIPTION
*
*       OID_GEN_MAXIMUM_TOTAL_SIZE query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_Gen_Total_Size(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{

    UINT32 * buff = (UINT32*)buffer;
    UINT16 index=0;

    /* Fill the required data in provided buffer. */
    buff[index++] = HOST_2_LE32(NF_MAXIMUM_TOTAL_SIZE);
    *buffer_len = index*sizeof(UINT32);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_Gen_Conn_Status
*
* DESCRIPTION
*
*       OID_GEN_MEDIA_CONNECT_STATUS query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_Gen_Conn_Status(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{

    UINT32 * buff = (UINT32*)buffer;
    UINT16 index=0;

    /* Fill the required data in provided buffer. */
    buff[index++] = HOST_2_LE32(cb->connect_status);
    *buffer_len = index*sizeof(UINT32);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_Gen_Link_Speed
*
* DESCRIPTION
*
*       OID_GEN_LINK_SPEED query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_Gen_Link_Speed(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{

    UINT32 * buff = (UINT32*)buffer;
    UINT16 index=0;

    /* Fill the required data in provided buffer. */
    buff[index++] = HOST_2_LE32(NF_LINK_SPEED);
    *buffer_len = index*sizeof(UINT32);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_Gen_Xmit_Ok
*
* DESCRIPTION
*
*       OID_GEN_XMIT_OK query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_Gen_Xmit_Ok(
                            NU_USBF_NDIS_DEVICE *pdevice,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{

    UINT32 * buff = (UINT32*)buffer;
    UINT16 index=0;

    /* Fill the required data in provided buffer. */
    buff[index++] = HOST_2_LE32(pdevice->frame_tx_ok);
    *buffer_len = index*sizeof(UINT32);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_Gen_Rcv_Ok
*
* DESCRIPTION
*
*       OID_GEN_RCV_OK query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_Gen_Rcv_Ok(
                            NU_USBF_NDIS_DEVICE *pdevice,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{

    UINT32 * buff = (UINT32*)buffer;
    UINT16 index=0;

    /* Fill the required data in provided buffer. */
    buff[index++] = HOST_2_LE32(pdevice->frame_rx_ok);
    *buffer_len = index*sizeof(UINT32);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_Gen_Xmit_Error
*
* DESCRIPTION
*
*       OID_GEN_XMIT_ERROR query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_Gen_Xmit_Error(
                            NU_USBF_NDIS_DEVICE *pdevice,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{

    UINT32 * buff = (UINT32*)buffer;
    UINT16 index=0;

    /* Fill the required data in provided buffer. */
    buff[index++] = HOST_2_LE32(pdevice->frame_tx_err);
    *buffer_len = index*sizeof(UINT32);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_Gen_Rcv_Error
*
* DESCRIPTION
*
*       OID_GEN_RCV_ERROR query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_Gen_Rcv_Error(
                            NU_USBF_NDIS_DEVICE *pdevice,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{

    UINT32 * buff = (UINT32*)buffer;
    UINT16 index=0;

    /* Fill the required data in provided buffer. */
    buff[index++] = HOST_2_LE32(pdevice->frame_rx_err);
    *buffer_len = index*sizeof(UINT32);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_Gen_Vendor_Id
*
* DESCRIPTION
*
*       OID_GEN_VENDOR_ID query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_Gen_Vendor_Id(
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status)
{

    UINT32 * buff = (UINT32*)buffer;
    UINT16 index=0;

    /* Return 0xFFFFFFFF, if Vendor is registered with IEEE. */
    buff[index++] = HOST_2_LE32(0xFFFFFFFF);
    *buffer_len = index*sizeof(UINT32);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*        NDISF_Find_Device
*
* DESCRIPTION
*
*        Routine used to find pointer to the user device corresponding
*        to the handle provided by the Communication driver.
*
* INPUTS
*
*       cb          Pointer to user control block.
*       handle      handle to search.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
NU_USBF_NDIS_DEVICE *NDISF_Find_Device (NU_USBF_NDIS_USER *cb,
                                        VOID *handle)
{

    NU_USBF_NDIS_DEVICE *next;
    NU_USBF_NDIS_DEVICE *ndis_device = cb->ndis_list_head;

    /* Search for handle in the circular list of NDIS user
     * instances.
     */
    while (ndis_device )
    {
        next = (NU_USBF_NDIS_DEVICE*)(ndis_device->node.cs_next);

        if (ndis_device->handle == handle)
            return (ndis_device);

        if ( (next == cb->ndis_list_head) ||
            (cb->ndis_list_head == NU_NULL))
            return (NU_NULL);
        else
            ndis_device = next;
    }

    return (NU_NULL);

}

/*************************************************************************
* FUNCTION
*
*       NDISF_Hndl_Oid_Gen_Statistics
*
* DESCRIPTION
*
*       NF_OID_GEN_STATISTICS query processing routine.
*
* INPUTS
*       cb                      Pointer to user control block.
*       input_oid               Pointer to current Query OID structure.
*       buffer                  Pointer to buffer for Query data.
*       buffer_len              Pointer for length of Query data.
*       query_response_status   Pointer for Query response.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion.
*
*
*************************************************************************/
STATUS NDISF_Hndl_Oid_Gen_Statistics(
                            NU_USBF_NDIS_DEVICE *pdevice,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 *buffer,
                            UINT32 *buffer_len,
                            UINT32 *query_response_status)
{
    UINT32  header;
    UINT32 *buff = (UINT32*)buffer;
    UINT8  *rev = (UINT8*)&header;
    UINT16 *size = (UINT16*)&header;
    UINT16  index = 0;

    /* Fill all the required fields. */
    rev[0]=(0x80);
    rev[1]=(1);
    size[1]=HOST_2_LE16(152);
   *buffer_len = 152;
    buff[index++] = HOST_2_LE32(header);
    buff[index++] = HOST_2_LE32(0x00000010|0x00000020);
    buff[index++] = HOST_2_LE32(pdevice->frame_rx_err);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(pdevice->frame_rx_err);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);
    buff[index++] = HOST_2_LE32(0);

    *query_response_status = (NF_STATUS_SUCCESS);
    return NU_SUCCESS;
}

/* ======================  End Of File  =============================== */
