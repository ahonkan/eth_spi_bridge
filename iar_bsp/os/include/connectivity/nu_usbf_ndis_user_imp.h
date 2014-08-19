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

***************************************************************************
*
* FILE NAME
*
*       nu_usbf_ndis_user_imp.h
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
*       NU_USBF_NDIS_USER_DISPATCH NDIS User Driver Dispatch Table.
*       NU_USBF_NDIS_USER          NDIS User Driver Control Block.
*       USBF_NDIS_USER_OID         OID structure.
*
* FUNCTIONS
*       None
*
* DEPENDENCIES
*
*     nu_usbf_ndis_user_dat.h     Dispatch Table Definitions.
*     nu_usbf_user_comm_ext.h     Communication user external definitions.
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_NDIS_USER_IMP_H_
#define _NU_USBF_NDIS_USER_IMP_H_

#ifdef          __cplusplus
extern  "C"                                 /* C declarations in C++    */
{
#endif

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */
#include "connectivity/nu_usbf_user_comm_ext.h"

/* =============Constants ============= */

/* These definitions control which version of the user API should be
   compatible with. This should allow new versions of API to be shipped but
   remain compatible with applications for previous versions. */
#define NF_1_6         1       /* NF 1.6 */
#define NF_2_0         2       /* NF 2.0 */

/* The version for which compatibility is desired. */
#ifndef NF_VERSION_COMP
    #define NF_VERSION_COMP    NF_2_0
#endif

/* NDIS is support under Abstract control model of communication device
 * class.
 */
#define NF_SUBCLASS                 0x2

#define NF_MAC_ADDR_LEN             0x6

#define NF_ZERO                     0x0L
#define NF_MAJOR_VERSION            0x1L
#define NF_MINOR_VERSION            0x0L
#define NF_DF_CONNECTIONLESS        0x1L
#define NF_DF_CONNECTIONLESS        0x1L
#define NF_DF_CONNECTION_ORIENTED   0x2L
#define NF_MEDIUM_802_3             0x0L
#define NF_MAXPKTPERMSG             0x01L

/*
 * Alignment factor is set to 2.
 */
#define NF_PKTALIGNFACTOR           0x2L

/* Abstract control model specific notification codes.*/
#define NF_RESPONSE_AVAILABLE       0x1

/* Abstract control model specific command codes.*/
#define NF_SEND_ENCAPSULATED_COMMAND    0x0L
#define NF_GET_ENCAPSULATED_RESPONSE    0x1L

/* Remote NDIS control message codes */
#define NF_PACKET_MSG           0x00000001L

#define NF_INIT_MSG             0x00000002L
#define NF_INITIALIZE_CMPLT     0x80000002L
#define NF_HALT_MSG             0x00000003L
#define NF_QUERY_MSG            0x00000004L
#define NF_QUERY_MSG_CMPLT      0x80000004L
#define NF_SET_MSG              0x00000005L
#define NF_SET_MSG_CMPLT        0x80000005L
#define NF_RESET_MSG            0x00000006L
#define NF_RESET_CMPLT          0x80000006L
#define NF_IND_STATUS_MSG       0x00000007L
#define NF_KEEPALIVE_MSG        0x00000008L
#define NF_KEEPALIVE_CMPLT      0x80000008L

/* Remote NDIS control messages lengths */
#define NF_INITIALIZE_CMPLT_LENGTH      0x34
#define NF_QUERY_MSG_CMPLT_LENGTH       0x18
#define NF_SET_MSG_CMPLT_LENGTH         0x10
#define NF_SET_KEEPALIVE_CMPLT_LENGTH   0x10
#define NF_SET_RESET_CMPLT_LENGTH       0x10

/* Indicate status message constants. */
#define NF_IND_STATUS_MSG_LENGTH        0x14L
#define NF_IND_STATUS_MSG_STATUS_OFF    0x08L
#define NF_IND_STATUS_MSG_BUF_LEN_OFF   0x0CL
#define NF_IND_STATUS_MSG_BUF_OFF       0x10L

/* Remote NDIS status codes */
#define NF_STATUS_BUFFER_OVERFLOW       0x80000005L
#define NF_STATUS_FAILURE               0xC0000001L
#define NF_STATUS_INVALID_DATA          0xC0010015L
#define NF_STATUS_MEDIA_CONNECT         0x4001000BL
#define NF_STATUS_MEDIA_DISCONNECT      0x4001000CL
#define NF_STATUS_NOT_SUPPORTED         0xC00000BBL
#define NF_STATUS_RESOURCES             0xC000009AL
#define NF_STATUS_SUCCESS               0x00000000L

/* Vendor driver version.  Here we are assuming 1.0 */
#define NF_VENDOR_DRIVER_VERSION        0x00010000L

/* Maximum Frame Size. */
#define NF_MAXIMUM_FRAME_SIZE           CFG_NU_OS_CONN_USB_FUNC_COMM_ETH_MAX_SEG_SIZE

/* Maximum List size for the multicast addresses which host can send
 * We are assuming sixteen here.
 */
#define NF_MAXIMUM_LIST_SIZE            0x10L

/* Maximum total size of the NDIS data packet. */
#define NF_MAXIMUM_TOTAL_SIZE           (NF_MAXIMUM_FRAME_SIZE+14)

#define NF_MAX_DATA_XFER_SIZE           (NF_MAXIMUM_TOTAL_SIZE+44)

/* Connect status values. */
#define NF_STATE_CONNENCTED             0x0L
#define NF_STATE_DISCONNENCTED          0x1L

/* Link speed in 100 bps units. */
#define NF_LINK_SPEED                   0x2710L

/* Remote NDIS Message OFFSETS */
#define NF_MSG_TYPE_OFF                 0x00000000L
#define NF_MSG_LENGTH_OFF               0x00000004L

/* Remote NDIS INITIALIZE_MSG offsets */
#define NF_INIT_MSG_REQ_ID_OFF          0x00000008L
#define NF_INIT_MSG_MAJ_VER_OFF         0x0000000CL
#define NF_INIT_MSG_MIN_VER_OFF         0x00000010L
#define NF_INIT_MSG_MAX_XFER_SIZE_OFF   0x00000014L

/* Remote NDIS INITIALIZE_CMPLT offsets */
#define NF_INIT_MSG_CMPLT_REQ_ID_OFF            0x00000008L
#define NF_INIT_MSG_CMPLT_STATUS_OFF            0x0000000CL
#define NF_INIT_MSG_CMPLT_MAJ_VER_OFF           0x00000010L
#define NF_INIT_MSG_CMPLT_MIN_VER_OFF           0x00000014L
#define NF_INIT_MSG_CMPLT_DEV_FLGS_OFF          0x00000018L
#define NF_INIT_MSG_CMPLT_MEDIUM_OFF            0x0000001CL
#define NF_INIT_MSG_CMPLT_PKT_MSGS_OFF          0x00000020L
#define NF_INIT_MSG_CMPLT_XFR_SIZE_OFF          0x00000024L
#define NF_INIT_MSG_CMPLT_ALIN_FAC_OFF          0x00000028L
#define NF_INIT_MSG_CMPLT_AFLIST_OFF            0x0000002CL
#define NF_INIT_MSG_CMPLT_AF_SIZE_OFF           0x00000030L

/* Remote NDIS Query_Msg*/
#define NF_QUERY_MSG_REQ_ID_OFF                 0x08L
#define NF_QUERY_MSG_OID_OFF                    0x0CL
#define NF_QUERY_MSG_INF_BUF_LEN_OFF            0x10L
#define NF_QUERY_MSG_INF_BUFOFFSET_OFF          0x14L
#define NF_QUERY_MSG_MAX_INPUT_BUF_LEN          0x100L

/* Remote NDIS NF_QUERY_MSG_Cmplt*/
#define NF_QUERY_MSG_CMPLT_REQ_ID_OFF           0x08L
#define NF_QUERY_MSG_CMPLT_STATUS_OFF           0x0CL
#define NF_QUERY_MSG_INF_BUF_LEN_OFF            0x10L
#define NF_QUERY_MSG_INF_BUFOFFSET_OFF          0x14L
#define NF_QUERY_MSG_CMPLT_DEV_VCH_OFF          0x18L

/* Remote NDIS Reset complete. */
#define NF_RESET_MSG_CMPLT_STATUS_OFF           0x08L
#define NF_RESET_MSG_CMPLT_ADR_RES_OFF         0x0CL

/* Remote NDIS Data message. */
#define NF_PKT_MSG_LENGTH                  0x2CL
#define NF_PKT_MSG_DATA_OFF                0x08L
#define NF_PKT_MSG_DATA_LENGTH_OFF         0x0CL
#define NF_PKT_MSG_PER_PKT_INFO_OFF        0x1CL
#define NF_PKT_MSG_PKT_INFO_LEN_OFF        0x20L
#define NF_PKT_MSG_OOB_DATA_OFF            0x10L
#define NF_PKT_MSG_OOB_DATA_LENGTH_OFF     0x14L
#define NF_PKT_MSG_NUM_OOB_ELEMS_OFF       0x18L
#define NF_PKT_MSG_VCHANDLE_OFF            0x24L

#define NF_PKT_MSG_VCHANDLE_SIZE           0x08L

/* NDIS object identifiers */
#define NF_OID_GEN_SUPPORTED_LIST          0x00010101L
#define NF_OID_GEN_HARDWARE_STATUS         0x00010102L
#define NF_OID_GEN_MEDIA_SUPPORTED         0x00010103L
#define NF_OID_GEN_MEDIA_IN_USE            0x00010104L
#define NF_OID_GEN_MAXIMUM_FRAME_SIZE      0x00010106L
#define NF_OID_GEN_LINK_SPEED              0x00010107L
#define NF_OID_GEN_TRANSMIT_BLOCK_SIZE     0x0001010aL
#define NF_OID_GEN_RECEIVE_BLOCK_SIZE      0x0001010bL
#define NF_OID_GEN_VENDOR_ID               0x0001010cL
#define NF_OID_GEN_VENDOR_DESCRIPTION      0x0001010dL
#define NF_OID_GEN_VENDOR_DVR_VERSION      0x00010116L
#define NF_OID_GEN_CURR_PACKET_FILTER      0x0001010eL
#define NF_OID_GEN_MAXIMUM_TOTAL_SIZE      0x00010111L
#define NF_OID_GEN_MED_CONNECT_STATUS      0x00010114L
#define NF_OID_GEN_XMIT_OK                 0x00020101L
#define NF_OID_GEN_RCV_OK                  0x00020102L
#define NF_OID_GEN_XMIT_ERROR              0x00020103L
#define NF_OID_GEN_RCV_ERROR               0x00020104L
#define NF_OID_GEN_RCV_NO_BUFFER           0x00020105L
#define NF_OID_802_3_PERMANENT_ADDRESS     0x01010101L
#define NF_OID_802_3_CURRENT_ADDRESS       0x01010102L
#define NF_OID_802_3_MULTICAST_LIST        0x01010103L
#define NF_OID_802_3_MAXIMUM_LIST_SIZE     0x01010104L
#define NF_OID_802_3_RCV_ERROR_ALIGN       0x01020101L
#define NF_OID_802_3_XMIT_ONE_COLL         0x01020102L
#define NF_OID_802_3_XMIT_MORE_COLL        0x01020103L
#define NF_OID_GEN_STATISTICS              0x00020106L

/* Maximum control message size is of NF_INITIALIZE_CMPLT
 * (i.e. 52).
 */
#define NF_MAX_CTRL_MSG_SIZE                0x100L

#define NF_MAX_STATUS_LENGTH                0x40L

/* IOCTLS */
#define USBF_NDIS_USER_SET_MAC        (USB_RNDIS_IOCTL_BASE + 0)
#define USBF_NDIS_USER_GET_MAC        (USB_RNDIS_IOCTL_BASE + 1)

/* =========================== ============= */
/* This structure is initialized for each query or set message from Host.
*/
typedef struct _usbf_ndis_user_oid
{
        UINT32 request_id;
        UINT32 ndis_oid;
        UINT32 input_buffer_length;
        UINT8 input_buffer[NF_QUERY_MSG_MAX_INPUT_BUF_LEN];
}
USBF_NDIS_USER_OID;

/* =============Dispatch Table ============= */
typedef struct _nu_usbf_ndis_user_dispatch
{
    NU_USBF_USER_COMM_DISPATCH  dispatch;
    /* Extension to NU_USBF_USER services
     * should be declared here */
}
NU_USBF_NDIS_USER_DISPATCH;

/* ============= Control Block ============= */

typedef struct _nu_usbf_ndis_device
{
	CS_NODE  node;
		
	VOID     *handle;

	/* Structure sent to communication driver. */
    USBF_COMM_USER_NOTIFICATION user_notif;

    /* Object ID under-processing by user application. It stores object ID
     * of most recent QUERY_MSG. All user applications are notified of its
     * arrival using event list.
     */
    USBF_NDIS_USER_OID input_oid;
	
	/* Last request ID sent by host. This request ID is used in the
     * responses.
     */
    UINT32 last_request_id;

    /* Remote NDIS major version supported by host.
     */
    UINT32 major_version;

    /* Remote NDIS minor version supported by host.
     */
    UINT32 minor_version;

    /* Maximum data transfer size supported by host.
     */
    UINT32 max_transfer_size;

    /* Ethernet statistics of driver. */
    UINT32 frame_tx_ok;
    UINT32 frame_rx_ok;
    UINT32 frame_tx_err;
    UINT32 frame_rx_err;

    UINT32 connect_status;                  /* Media connect status. */

    /* Specifies the number of bytes available in response_array.
     */
    UINT32 response_length;
    UINT32 status_length;

    /* This buffer is used to store response for host. This response data
     * is sent to host on arrival of GET_ENCAPSULATED_RESPONSE.
     */
    UINT8 response_array[NF_MAX_CTRL_MSG_SIZE];
    UINT8 status_array[NF_MAX_STATUS_LENGTH];
    UINT8 mac_address[NF_MAC_ADDR_LEN];

    /* Transfer synchronization flags. */
    UINT8 resp_in_progress;
    UINT8 status_pending;

}NU_USBF_NDIS_DEVICE;

/* Redefine if your control block is any different  from this*/
typedef struct _nu_usbf_ndis_user
{
    NU_USBF_USER_COMM parent;

	NU_USBF_NDIS_DEVICE *ndis_list_head;
	USBF_FUNCTION       *ndis_function;

	NU_MEMORY_POOL *pool;

    /* Application notify callback*/
    COMMF_APP_NOTIFY    rndis_app_notify;

    /* Application Receive callback*/
    COMMF_APP_RX_CALLBACK   rndis_rcvd_cb;

    /* Application Tx_Done callback*/
    COMMF_APP_TX_DONE       rndis_tx_done;

    /* Application IOCTL callback.*/
    COMMF_APP_IOCTL         rndis_ioctl;
}
NU_USBF_NDIS_USER;

/* MAC Address Set IOCTL data */
typedef struct _nu_usbf_ndis_mac_data
{
    UINT8 mac_addr[NF_MAC_ADDR_LEN];
    VOID  *handle;
}
NU_USBF_NDIS_MAC_DATA;

/* ========= Function Prototypes =========== */

NU_USBF_NDIS_DEVICE *NDISF_Find_Device (NU_USBF_NDIS_USER *cb,
                                      VOID *handle);

STATUS NDISF_Proc_Halt_Msg(NU_USBF_NDIS_USER *cb);

STATUS NDISF_Decode_Encap_Command(NU_USBF_NDIS_USER *cb,
                                  NU_USBF_NDIS_DEVICE *pdevice,
                                  UINT8 *data,
                                  UINT16 len);

STATUS NDISF_Proc_Initialize_Msg(  NU_USBF_NDIS_DEVICE *pdev,
                                   UINT8 *data,
                                   UINT16 len);

VOID NDISF_Prep_Initialize_Msg_Cmplt(  NU_USBF_NDIS_DEVICE *pdevice,
                                       UINT8 *data,
                                       UINT16 len,
                                       UINT8 * data_out,
                                       UINT32 * data_out_len);

VOID NDISF_Prep_NF_QUERY_MSG_Cmplt(NU_USBF_NDIS_USER *cb,
                                       UINT32 reqid,
                                       UINT32 status,
                                       UINT32 inf_buffer_len,
                                       UINT8 * data_out,
                                       UINT32 * data_out_len);

STATUS NDISF_Send_Notification(NU_USBF_NDIS_USER *cb, UINT8 notif, NU_USBF_NDIS_DEVICE *pdev);

STATUS NDISF_Proc_Query_Msg(NU_USBF_NDIS_DEVICE *cb,
                                       UINT8 *data,
                                       UINT16 len);

STATUS NDISF_Proc_Set_Msg(NU_USBF_NDIS_DEVICE *cb,
                                       UINT8 *data,
                                       UINT16 len);

STATUS NDISF_Proc_Reset_Msg(NU_USBF_NDIS_DEVICE *pdevice);

VOID NDISF_Prep_Set_Msg_Cmplt(NU_USBF_NDIS_USER *cb,
                                        UINT32 reqid,
                                        UINT32 status,
                                        UINT8 * data_out,
                                        UINT32 * data_out_len);

VOID NDISF_Prep_Keepalive_Msg_Cmplt(NU_USBF_NDIS_DEVICE *cb,
                                        UINT32 reqid,
                                        UINT32 status,
                                        UINT8 * data_out,
                                        UINT32 * data_out_len);

VOID NDISF_Prep_NF_RESET_MSG_Cmplt(
                                        NU_USBF_NDIS_USER *cb,
                                        UINT32 reqid,
                                        UINT32 status,
                                        UINT8 * data_out,
                                        UINT32 * data_out_len);

STATUS NDISF_Proc_Query_Oid(
                    NU_USBF_NDIS_DEVICE *pdevice,
                    USBF_NDIS_USER_OID * input_oid,
                    UINT8 * buffer,
                    UINT32 * buffer_len,
                    UINT32 * query_response_status);
STATUS NDISF_Proc_Set_Oid(
                    NU_USBF_NDIS_USER *cb,
                    USBF_NDIS_USER_OID * input_oid,
                    UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_Gen_Supp_List(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_Gen_Vndr_Dvr_Ver(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_Gen_Max_Frm_Size(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_802_3_List_Size(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_802_3_Curr_Addr(
                           NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_Gen_Total_Size(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_Gen_Conn_Status(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_Gen_Link_Speed(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_Gen_Xmit_Ok(
                            NU_USBF_NDIS_DEVICE *pdev,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_Gen_Rcv_Ok(
                            NU_USBF_NDIS_DEVICE *pdev,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_Gen_Xmit_Error(
                            NU_USBF_NDIS_DEVICE *pdevice,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_Gen_Rcv_Error(
                            NU_USBF_NDIS_DEVICE *pdevice,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_802_3_Pert_Addr(
                            NU_USBF_NDIS_DEVICE *cb,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);

STATUS NDISF_Hndl_Oid_Gen_Vendor_Id(
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);
STATUS NDISF_Hndl_Oid_Gen_Statistics(
							NU_USBF_NDIS_DEVICE *pdevice,
                            USBF_NDIS_USER_OID *input_oid,
                            UINT8 * buffer,
                            UINT32 * buffer_len,
                            UINT32 * query_response_status);
STATUS NDISF_Proc_Keepalive_Msg(NU_USBF_NDIS_DEVICE *cb,
                                UINT8 *data,
                                UINT16 len);


/* ==================================================================== */

#include "connectivity/nu_usbf_ndis_user_dat.h"

/* ==================================================================== */
#ifdef          __cplusplus
}                                           /* End of C declarations    */
#endif

#endif                                      /* _NU_USBF_NDIS_USER_IMP_H_*/

/* ======================  End Of File  =============================== */

