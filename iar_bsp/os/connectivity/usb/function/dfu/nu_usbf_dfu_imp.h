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
*   nu_usbf_dfu_imp.h
*
* COMPONENT
*
*   Nucleus USB Function Software : DFU Class Driver
*
* DESCRIPTION
*
*   This file contains the internal data structures and definitions for DFU
*   Class Driver.
*
* DATA STRUCTURES
*
*   NU_USBF_DFU_DISPATCH    DFU Class Driver Dispatch Table.
*
* FUNCTIONS
*
*   _NU_USBF_DFU_Notify             This function notifies Driver of USB
*                                   events.
*   _NU_USBF_DFU_Initialize_Intf    Connect notification function invoked by
*                                   stack when driver is given an
*                                   opportunity to own an Interface
*   _NU_USBF_DFU_Disconnect         Disconnect Callback function, invoked by
*                                   stack when a Device/Interface served by
*                                   DFU is removed from the BUS.
*   _NU_USBF_DFU_Set_Intf           Notifies driver of change in alternate
*                                   setting.
*   USBF_DFU_Handle_Disconnected    This routine processes the Disconnect
*                                   event
*   USBF_DFU_Handle_USB_Event_Rcvd  This routine processes the USB bus event
*                                   notification such as reset.
*   USBF_DFU_Handle_Initialized     This is a initialization routine for the
*                                   DFU class interface.
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
*   DFUF_Data_Xfer_Status_Error     This function updates the user about the
*                                   detected error and stalls the control
*                                   end point.
*   DFUF_Stall_Ctrl_Pipe            This function is used to stall the
*                                   control endpoint when ever the error
*                                   is detected.
*   DFUF_Send_Get_Status_Payload    This function is used to transfer the
*                                   DFU status payload to the host.
*
* DEPENDENCIES
*
*   nu_usbf_user_ext.h  USB function user driver
*   nu_usbf_dfu_cfg.h   DFU configuration file
*   nu_usbf_dfu_dat.h   Dispatch table
**************************************************************************/

/*======================================================================*/
#ifndef _NU_USBF_DFU_IMP_H_

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_DFU_IMP_H_

/*==================== Header files ====================================*/
#include "connectivity/nu_usbf_user_ext.h"
#include "nu_usbf_dfu_cfg.h"

/*=============================== Macros ===============================*/
/* General macros */
#define DFU_BYTES_3                             3
#define DFU_RESET                               0
#define DFU_SET                                 1

/* DFU class specific requests */
#define DFU_SPEC_DETACH                         0
#define DFU_SPEC_DNLOAD                         1
#define DFU_SPEC_UPLOAD                         2
#define DFU_SPEC_GETSTATUS                      3
#define DFU_SPEC_CLRSTATUS                      4
#define DFU_SPEC_GETSTATE                       5
#define DFU_SPEC_ABORT                          6

/* DFU device  status macros */
#define DFU_SPEC_STATUS_OK                      0x00
#define DFU_SPEC_STATUS_ERR_TRGT                0x01
#define DFU_SPEC_STATUS_ERR_FILE                0x02
#define DFU_SPEC_STATUS_ERR_WRITE               0x03
#define DFU_SPEC_STATUS_ERR_ERASE               0x04
#define DFU_SPEC_STATUS_ERR_CHK_ERASED          0x05
#define DFU_SPEC_STATUS_ERR_PROG                0x06
#define DFU_SPEC_STATUS_ERR_VRFY                0x07
#define DFU_SPEC_STATUS_ERR_ADDRS               0x08
#define DFU_SPEC_STATUS_ERR_NOTDONE             0x09
#define DFU_SPEC_STATUS_ERR_FIRMWARE            0x0A
#define DFU_SPEC_STATUS_ERR_VNDR                0x0B
#define DFU_SPEC_STATUS_ERR_USBR                0x0C
#define DFU_SPEC_STATUS_ERR_POR                 0x0D
#define DFU_SPEC_STATUS_ERR_UNKNOWN             0x0E
#define DFU_SPEC_STATUS_ERR_STALLEEDPKT         0x0F

/* DFU device state macros */
#define DFU_SPEC_STATE_APPL_IDLE                0
#define DFU_SPEC_STATE_APPL_DETACH              1
#define DFU_SPEC_STATE_DFU_IDLE                 2
#define DFU_SPEC_STATE_DFU_DNLOAD_SYNC          3
#define DFU_SPEC_STATE_DFU_DNBUSY               4
#define DFU_SPEC_STATE_DFU_DNLOAD_IDLE          5
#define DFU_SPEC_STATE_DFU_MNFST_SYNC           6
#define DFU_SPEC_STATE_DFU_MNFST                7
#define DFU_SPEC_STATE_DFU_MNFST_WT_RST         8
#define DFU_SPEC_STATE_DFU_UPLOAD_IDLE          9
#define DFU_SPEC_STATE_DFU_ERR                  10

/* DFU class specific RequestType macros */
#define DFU_SPEC_RQST_TYPE_DETACH               00100001b
#define DFU_SPEC_RQST_TYPE_DNLOAD               00100001b
#define DFU_SPEC_RQST_TYPE_UPLOAD               10100001b
#define DFU_SPEC_RQST_TYPE_GETSTATUS            10100001b
#define DFU_SPEC_RQST_TYPE_CLRSTATUS            00100001b
#define DFU_SPEC_RQST_TYPE_GETSTATE             10100001b
#define DFU_SPEC_RQST_TYPE_ABORT                00100001b

/* Runtime DFU Interface Descriptor macros */
#define DFU_SPEC_RTM_INT_DES_LEN                0x09
#define DFU_SPEC_RTM_INT_DES_ALTR_STNG          0x00
#define DFU_SPEC_RTM_INT_DES_NUM_EPS            0x00
#define DFU_SPEC_RTM_INT_DES_INT_CLS            0xFE
#define DFU_SPEC_RTM_INT_DES_INT_SUBCLS         0x01
#define DFU_SPEC_RTM_INT_DES_INT_PRTCL          0x01

/* Runtime DFU Functional Descriptors macros */
#define DFU_SPEC_RTM_FUN_DES_LEN                0x09
#define DFU_SPEC_RTM_FUN_DES_TYPE               0x21

/* Runtime DFU Functional Descriptor bmAttributes macros */

/* bitWillDetach macros */
#define DFU_FUN_DES_BITWIL_DETACH_TRUE          (1 << 3)
#define DFU_FUN_DES_BITWIL_DETACH_FALSE         (0 << 3)

/* bitManifestation Tolerant macros */
#define DFU_FUN_DES_BITMNFST_TLRNT_TRUE         (1 << 2)
#define DFU_FUN_DES_BITMNFST_TLRNT_FALSE        (0 << 2)

/* bitCanUpload macros */
#define DFU_FUN_DES_BIT_CANUPLD_TRUE            (1 << 1)
#define DFU_FUN_DES_BIT_CANUPLD_FLASE           (0 << 1)

/* bitCanDnload macros */
#define DFU_FUN_DES_BIT_CANDNLD_TRUE            1
#define DFU_FUN_DES_BIT_CANDNLD_FLASE           0

/* DFU Mode Device Descriptor  macros */
#define DFU_SPEC_DEV_DES_LEN                    0x12
#define DFU_SPEC_DEV_DES_TYPE                   0x01
#define DFU_SPEC_DEV_DES_BCDUSB                 0x0100
#define DFU_SPEC_DEV_DES_DEV_CLS                0x00
#define DFU_SPEC_DEV_DES_DEV_SUBCLS             0x00
#define DFU_SPEC_DEV_DES_DEV_PRTCL              0x00
#define DFU_SPEC_DEV_DES_NUM_CFGS               0x01

/* DFU Mode Configuration Descriptor  macros */
#define DFU_SPEC_CFG_DES_NUM_INTR               0x01

/* DFU Mode Interface Descriptor macros */
#define DFU_SPEC_INT_DES_LEN                    0x09
#define DFU_SPEC_INT_DES_INTR_NUM               0x00
#define DFU_SPEC_INT_DES_NUM_EPS                0x00
#define DFU_SPEC_INT_DES_INT_CLS                0xFE
#define DFU_SPEC_INT_DES_INT_SUBCLS             0x01
#define DFU_SPEC_INT_DES_INT_PRTCL              0x02

/* Macros defining the index of status description in string table. */
#define DFU_STATUS_TABLE_STR_INDX0              0
#define DFU_STATUS_TABLE_STR_INDX1              1
#define DFU_STATUS_TABLE_STR_INDX2              2
#define DFU_STATUS_TABLE_STR_INDX3              3
#define DFU_STATUS_TABLE_STR_INDX4              4
#define DFU_STATUS_TABLE_STR_INDX5              5
#define DFU_STATUS_TABLE_STR_INDX6              6
#define DFU_STATUS_TABLE_STR_INDX7              7
#define DFU_STATUS_TABLE_STR_INDX8              8
#define DFU_STATUS_TABLE_STR_INDX9              9
#define DFU_STATUS_TABLE_STR_INDX10             10
#define DFU_STATUS_TABLE_STR_INDX11             11
#define DFU_STATUS_TABLE_STR_INDX12             12
#define DFU_STATUS_TABLE_STR_INDX13             13
#define DFU_STATUS_TABLE_STR_INDX14             14
#define DFU_STATUS_TABLE_STR_INDX15             15

/*  Common Macros for runtime and DFU mode descriptor */
#define DFU_RELEASE_SPEC_BCDVER                 0x01

/*========================== Constants =================================*/
/* Following macros define the different values that detach timer may
 * attain.
 */
#define DFU_DETACH_TMR_IDLE                     0
#define DFU_DETACH_TMR_RUNNING                  1
#define DFU_DETACH_TMR_EXPIRED                  2

/* Macros used for handling the reset event sent by the host to assign the
 * address to the device. In this case device should not change the mode but
 * just inform the user about the reception of the reset.
 */
#define DFUF_DETACH_RCVD_TRUE                   1
#define DFUF_DETACH_RCVD_FALSE                  0

/* Following macros are used to indicate the status of the firmware download
 */
#define DFUF_DNLOAD_IDLE                        0
#define DFUF_DNLOAD_IN_PROGRESS                 1
#define DFUF_DNLOAD_COMPLETED                   2

/* Following macros indicate the status of the manifestation phase. */
#define DFUF_MNFST_IDLE                         0
#define DFUF_MNFST_IN_PROGRESS                  1
#define DFUF_MNFST_COMPLETED                    2

/* Number of DFU state handlers */
#define NUM_DFU_STATE_HANDLERS                  11

/* Index for the DFU workers counter array */
#define DFU_CTRL_XFER_CMPLT                     3

/*========================= Macros =====================================*/
#define DFUF_FW_DNLOAD                          1
#define DFUF_FW_UPLOAD                          2

/* Macros used for firmware transfer between the firmware handler and DFU
 * class driver.
 */
#define DFUF_DATA_XFER_DONE                     1
#define DFUF_DATA_XFER_PENDING                  2

#define DFU_DNLOAD_COMPLETE                     0xA
#define DFU_UPLOAD_COMPLETE                     0xB

#define DFUF_DATA_XFER_SUCCESS                  0
#define DFUF_DATA_XFER_FAILED                   1
#define DFUF_DATA_XFER_IN_PROGRESS              2

/* Macro used to indicate the application that Detach request, with invalid
 * timeout value, is received by the device.
 */
#define DFU_DETACH_INVALID_TMOUT                0xC

/* Descriptor macros, used to inform DFU application about the change of
 * descriptors to DFU or runtime.
 */
#define DFUF_CHANGE_NO_DESC                     0
#define DFUF_CHANGE_DESC_TO_DFU                 1
#define DFUF_CHANGE_DESC_TO_RUNTM               2

/* Macro defining the size of the request/command buffer to receive the
 * command from the host.
 */
#define DFUF_CMD_PKT_SIZE                       8

/*====================== Data structures ===============================*/
/*============================ typedefs ================================*/
/* DFU_GETSTATUS response structure */
typedef struct _dfu_getstatus_payload_struct
{
    /* This member holds the status of the resulting from
     * the execution of the most recent request.
     */
    UINT8   dfu_status;

    /* This member give the the time in milliseconds that the host should
     * wait between status phase of the next DFU_DNLOAD and the
     * subsequent solicitation of the device's status via DFU_GETSTATUS.
     */
    UINT8   dfu_poll_time_out[DFU_BYTES_3];

    /* This member holds the state that the the DFU device
     * is going to enter immediately after transmission of this
     * response.
     */
    UINT8   dfu_state;

    /* This member holds a index into the status description
     * in the string table.
     */
    UINT8   dfu_istring;
}DFU_GETSTATUS_PAYLOAD;

/* Dispatch Table */
typedef struct _nu_usbf_dfu_dispatch
{
    NU_USBF_DRVR_DISPATCH   dispatch;

    /* Extension to NU_USBF_DRVR services
      * should be declared here
      */

}NU_USBF_DFU_DISPATCH;

/*====================== Data structures ===============================*/

/*  DFU control block */
typedef struct _nu_usbf_dfu_struct
{
    /* Parent's control block. */
    NU_USBF_DRVR parent;

    /* IRP for control transfer. */
    NU_USB_IRP  ctrl_irp;

    /* Received setup packet buffer.*/
    NU_USB_SETUP_PKT setup_pkt;

    /* DFU user pointer. */
    NU_USBF_USER *dfu_user;

    /* DFU runtime function */
    USBF_FUNCTION   *dfu_runtime;

    /* DFU standalone function */
    USBF_FUNCTION   *dfu_standalone;

    /* This member holds the user call back function pointers. */
    struct _nu_usbf_dfu_usr_callbk_struct *dfu_usr_callback;

    /* DFU interface pointer. */
    NU_USB_INTF *dfu_intf;

    /* DFU device pointer. */
    NU_USB_DEVICE *dfu_dev;

    /* DFU alt_setting pointer. */
    NU_USB_ALT_SETTG *dfu_alt_set;

    /* Control pipe pointer. */
    NU_USB_PIPE *control_pipe;

    /* Define a timer control block for the below mentioned 3 timers
     * 1) Detach timer
     * 2) Download busy timer
     * 3) Manifestation sync timer
     */
    NU_TIMER dfu_detach_timer;
    NU_TIMER dfu_dnload_busy_timer;
    NU_TIMER dfu_mnfst_sync_timer;

    DFU_GETSTATUS_PAYLOAD dfu_get_status_payload;

    /* This member is used to store the user driver context. */
    VOID *user_context;

    /* Buffer for received bus event. */
    UINT32  usb_event;

    /* Control command data buffer */
    UINT8   cmd_buffer[DFUF_CMD_PKT_SIZE];

    /* Data buffer to receive the firmware from the host in case of
     * download and from the firmware handler in case of the upload request
     * processing by the device.
     */
    UINT8   *data_buffer;

    /* This member is used to maintain the current state of the DFU.
     * The DFU states are as below:
     * (1) DFUF_STATE_APPL_IDLE
     * (2) DFUF_STATE_APPL_DETACH
     * (3) DFUF_STATE_DFU_IDLE
     * (4) DFUF_STATE_DFU_DNLOAD_SYNC
     * (5) DFUF_STATE_DFU_DNBUSY
     * (6) DFUF_STATE_DFU_DNLOAD_IDLE
     * (7) DFUF_STATE_DFU_MNFST_SYNC
     * (8) DFUF_STATE_DFU_MNFST
     * (9) DFUF_STATE_DFU_MNFST_WAIT_RSET
     * (10) DFUF_STATE_DFU_UPLOAD_IDLE
     * (11) DFUF_STATE_DFU_ERR
     */
    UINT8   dfu_state;

    /* This member holds the device status of the DFU.
     * The DFU status values are as below:
     * (1) DFU_SPEC_STATUS_OK
     * (2) DFU_SPEC_STATUS_ERR_TRGT
     * (3) FU_SPEC_STATUS_ERR_FILE
     * (4) FU_SPEC_STATUS_ERR_WRITE
     * (5) FU_SPEC_STATUS_ERR_ERASE
     * (6) FU_SPEC_STATUS_ERR_CHK_ERASED
     * (7) FU_SPEC_STATUS_ERR_PROG
     * (8) FU_SPEC_STATUS_ERR_VRFY
     * (9) FU_SPEC_STATUS_ERR_ADDRS
     * (10) FU_SPEC_STATUS_ERR_NOTDONE
     * (11) FU_SPEC_STATUS_ERR_FIRMWARE
     * (12) FU_SPEC_STATUS_ERR_VNDR
     * (13) FU_SPEC_STATUS_ERR_USBR
     * (14) FU_SPEC_STATUS_ERR_POR
     * (15) FU_SPEC_STATUS_ERR_UNKNOWN
     * (16) FU_SPEC_STATUS_ERR_STALLEEDPKT
     */
    UINT8 dfu_status;

    /* This member holds the status of the detach timer and it can take
     * the following values:
     * 1) DFU_DETACH_TMR_IDLE
     * 2) DFU_DETACH_TMR_RUNNING
     * 3) DFU_DETACH_TMR_EXPIRED
     */
    UINT8 detach_tmr_status;

    /* This member holds the index into the DFU status string table */
    UINT8   dfu_status_str_index;

    /* This member indicates if the firmware data download is completed or
     * in progress or is in idle state.
     * It can take the following values:
     * 1) DFUF_DNLOAD_IDLE
     * 2) DFUF_DNLOAD_IN_PROGRESS: Indicates that the device is writing to
     *                             the non_volatile memory.
     * 3) DFUF_DNLOAD_COMPLETED  : Indicates that the device is done with
     *                             writing the firmware data block to the
     *                             non-volatile memory.
     */
    UINT8   dfu_dnld_blk_xfer_status;

    /* This member indicates if the manifestation is completed or is in
     * progress.
     * It can take the following values:
     * 1) DFUF_MNFST_IDLE
     * 2) DFUF_MNFST_IN_PROGRESS: Indicates that manifestation is in
     *                            progress and is waiting for the
     *                            DFU_GETSTATUS request.
     * 3) DFUF_MNFST_COMPLETED  : Indicates that manifestation is completed.
     */
    UINT8   dfu_mnfst_status;

    /* This member is used to track the block numbers for the
     * upload/download transfers. It always holds a current block number
     * expected from the host.
     */
    UINT16 blk_num;

    /* This member holds the length of the last firmware download/upload
     * request.
     */
    UINT32 data_len;

    /* Maximum packet size for endpoint0 */
    UINT8 max_packet_size;

    /* Macros used for handling the reset event sent by the host to assign
     * the address to the device. In this case device should not change the
     * mode but just inform the user about the reception of the reset.
     */
    UINT8 detach_rcvd;

    /* Do 2 bytes padding */
    DATA_ELEMENT    dfu_fwh_padding[2];
} NU_USBF_DFU;

/* DFU Application call back structure */
typedef struct _nu_usbf_dfu_usr_callbk_struct
{
    /* This call back is used to transfer the firmware data between the
     * firmware handler and DFU class driver.
     */
    STATUS (*DFU_Data_Xfer_Callbk)(NU_USBF_DFU *cb,
                                   VOID *context,
                                   UINT8 *buf, UINT32 buf_len,
                                   UINT32 *bytes_read, UINT8 *data_pending,
                                   UINT8 data_xfer_status, UINT8 data_dir);

    /* This call back is used to escalate the event received by the DFU
     * class driver to the firmware handler. Following are the events that
     * can be received:
     * (1) connect
     * (2) disconnect
     * (3) RESET
     */
    STATUS (*DFU_Event_Callbk)(VOID          *context,
                               UINT8         event,
                               UINT8         desc_change_mode);

    /* This call back is used by the DFU class driver to give update on
     * the DFU status to the DFU firmware handler. The DFU status can be
     * Abort, any error state.
     */
    STATUS (*DFU_Status_Update_Callbk)(VOID  *context,
                                       UINT8 dfu_state);

    /* This call back is used by the DFU class driver to update the DFU
     * firmware handler on the DFU requests received by the DFU device and
     * also the status update on upload/download progress, such as
     * download/upload complete.
     */
    STATUS (*DFU_Request_Update_Callbk)(VOID  *context,
                                        UINT8 dfu_request);

    /* This call back is called by the DFU class driver to get the poll time
     * out value. This is the value for which the device can ask the host to
     * wait for the Poll time out milliseconds period before issuing the
     * next Get_Status request to the device.
     */
    STATUS (*DFU_Get_Poll_Tm_Out)( UINT32 *poll_tm_out);
} NU_USBF_DFU_USR_CALLBK;

/* Structure for Application registration with DFU driver in IOCTL 
 * function 
 */
typedef struct _usbf_dfu_app_callback
{
    /* Contains the pointer to the callback functions of Application */
    NU_USBF_DFU_USR_CALLBK  dfu_user_callbk;
    
    /* Pointer to the user context data */ 
    VOID *dfu_data_context;

} USBF_DFU_APP_CALLBACK;

/* typedef for HISR functions */
typedef VOID (*DFU_WORKERS)(NU_USBF_DFU *dfu);

/* typedef for DFU state handle */
typedef int(*DFU_RQST_HANDLER)(NU_USBF_DFU *dfu);

/*============================ Header files ============================*/
/* The extern declarations in the nu_usbf_dfu_dat.h header file requires the
 * definitions, which are defined above. Due to this its included here.
 */
#include "nu_usbf_dfu_dat.h"

/*============================ Internal Functions =======================*/
STATUS _NU_USBF_DFU_Notify(
                                    NU_USB_DRVR    *cb,
                                    NU_USB_STACK   *stack,
                                    NU_USB_DEVICE  *device,
                                    UINT32         event);
STATUS _NU_USBF_DFU_Initialize_Intf(
                                    NU_USB_DRVR    *cb,
                                    NU_USB_STACK   *stk,
                                    NU_USB_DEVICE  *dev,
                                    NU_USB_INTF    *intf);
STATUS _NU_USBF_DFU_Disconnect(
                                    NU_USB_DRVR    *cb,
                                    NU_USB_STACK   *stack,
                                    NU_USB_DEVICE  *device);
STATUS _NU_USBF_DFU_Set_Intf(
                                    NU_USB_DRVR    *cb,
                                    NU_USB_STACK   *stack,
                                    NU_USB_DEVICE  *device,
                                    NU_USB_INTF    *intf,
                                    NU_USB_ALT_SETTG *alt_settg);
VOID USBF_DFU_Handle_Disconnected(
                                    NU_USBF_DFU     *dfu);
VOID USBF_DFU_Handle_USB_Event_Rcvd(
                                    NU_USBF_DFU     *dfu);
VOID USBF_DFU_Handle_Initialized(
                                    NU_USBF_DFU     *dfu);
VOID DFUF_Ctrl_Data_IRP_Cmplt(
                                    NU_USB_PIPE     * pipe,
                                    NU_USB_IRP      *irp);
STATUS _NU_USBF_DFU_New_Setup(
                                    NU_USB_DRVR     *cb,
                                    NU_USB_STACK    *stack,
                                    NU_USB_DEVICE   *device,
                                    NU_USB_SETUP_PKT *setup);
VOID DFU_Detach_Tmr(                UNSIGNED        id);
VOID DFU_Dnload_Bsy_Tmr(            UNSIGNED        id);
VOID DFU_Mnfst_Sync_Tmr(            UNSIGNED        id);
VOID USBF_DFU_Handle_Ctrl_Xfer_Cmplt(
                                    NU_USBF_DFU     *dfu);
STATUS DFUF_State_App_Idle(
                                    NU_USBF_DFU     *dfu);
STATUS DFUF_State_App_Detach(
                                    NU_USBF_DFU     *dfu);
STATUS DFUF_State_Dfu_Idle(
                                    NU_USBF_DFU     *dfu);
STATUS DFUF_State_Dfu_Dnload_Sync(
                                    NU_USBF_DFU     *dfu);
STATUS DFUF_State_Dfu_Dnload_Busy(
                                    NU_USBF_DFU     *dfu);
STATUS DFUF_State_Dfu_Dnload_Idle(
                                    NU_USBF_DFU     *dfu);
STATUS DFUF_State_Dfu_Mnfst_Sync(
                                    NU_USBF_DFU     *dfu);
STATUS DFUF_State_Dfu_Mnfst(
                                    NU_USBF_DFU     *dfu);
STATUS DFUF_State_Dfu_Mnfst_wt_Reset(
                                    NU_USBF_DFU     *dfu);
STATUS DFUF_State_Dfu_Upload_Idle(
                                    NU_USBF_DFU     *dfu);
STATUS DFUF_State_Dfu_Error(
                                    NU_USBF_DFU     *dfu);
STATUS DFUF_Get_Poll_Tm_Out(
                                    NU_USBF_DFU     *dfu,
                                    UINT32          *poll_tm_out);
STATUS DFUF_Send_Get_State(
                                    NU_USBF_DFU     *dfu);
VOID DFUF_Data_Xfer_Status_Error(
                                    NU_USBF_DFU     *dfu);
VOID DFUF_Stall_Ctrl_Pipe(
                                    NU_USBF_DFU     *dfu,
                                    UINT8           dfu_status);
STATUS DFUF_Send_Get_Status_Payload(
                                    NU_USBF_DFU     *dfu);

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif /* _NU_USBF_DFU_IMP_H_ */
/*============================  End Of File  ===========================*/
