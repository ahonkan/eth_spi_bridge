/**************************************************************************
*
*           Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbf_acm_user_imp.h
*
*
* COMPONENT
*
*       Nucleus USB Function Software : Remote ACM User Driver.
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for ACM User Driver.
*
* DATA STRUCTURES
*
*       NU_USBF_ACM_USER_DISPATCH           ACM User Driver Dispatch Table.
*       NU_USBF_ACM_USER                    ACM User Driver Control Block.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_acm_user_dat.h              Dispatch Table Definitions.
*       nu_usbf_user_comm_ext.h             Communication user external
*                                           definitions.
*
**************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_ACM_USER_IMP_H_
#define _NU_USBF_ACM_USER_IMP_H_
/* ==================================================================== */

#ifdef __cplusplus
extern "C" {					/* C declarations in C++. */
#endif

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */
#include "connectivity/nu_usbf_user_comm_ext.h"

/*================================Constants============================ */

/* These definitions control which version of the user API should be
   compatible with. This should allow new versions of API to be shipped but
   remain compatible with applications for previous versions. */
#define ACMF_1_5         1       /* ACM 1.5 */
#define ACMF_2_0         2       /* ACM 2.0 */

/* The version for which compatibility is desired. */
#ifndef ACMF_VERSION_COMP
    #define ACMF_VERSION_COMP    ACMF_2_0
#endif

#define ACMF_SUBCLASS    0x2
#define ACMF_ZERO    0x0

/* Abstract control model specific command codes.*/
#define ACMF_SEND_ENCAPSULATED_COMMAND   0x0
#define ACMF_GET_ENCAPSULATED_RESPONSE   0x1
#define ACMF_SET_COMM_FEATURE        0x2
#define ACMF_GET_COMM_FEATURE        0x3
#define ACMF_CLEAR_COMM_FEATURE      0x4
#define ACMF_SET_LINE_CODING         0x20
#define ACMF_GET_LINE_CODING         0x21
#define ACMF_SET_CONTROL_LINE_STATE  0x22
#define ACMF_SEND_BREAK          0x23

/* Define modem state  */
#define ACMF_MODEM_ONLINE   0x01
#define ACMF_MODEM_COMMAND  0x02
#define ACMF_MODEM_DISCONNECT   0x03
#define ACMF_MODEM_TX_DONE   0x04
#define ACMF_MODEM_TX_ERROR   0x05

/* Abstract Control model Notifications */
#define ACM_RESPONSE_AVAILABLE     0x01
#define ACM_NETWORK_CONNECTION     0x00
#define ACM_SERIAL_STATE       0x20

/* Define COMM, Data Flags for  */
#define ACMF_COMM_FLAG    0x01
#define ACMF_DATA_FLAG    0x02

/* Parity modes */
#define ACMF_MARK   0x01
#define ACMF_EVEN   0x02
#define ACMF_ODD    0x03
#define ACMF_SPACE  0x04
#define ACMF_NONE   0x05

/* ACM data_send_event Flag */
#define ACMF_DATA_SENT  0x1

#define MAX_COMMAND_SIZE    0x4C

#define ACMF_CONFIG_BUFF_SIZE   8
#define ACMF_NOTIF_BUFF_SIZE    4

/* =============================Dispatch Table =========================*/
typedef struct _nu_usbf_acm_user_dispatch
{
    NU_USBF_USER_COMM_DISPATCH  dispatch;
    /* Extension to NU_USBF_USER services
    * should be declared here */
}
NU_USBF_ACM_USER_DISPATCH;

/* =====================================================================*/

/* Application dispatch function declarations. */

#if (ACMF_VERSION_COMP >= ACMF_2_0)

typedef STATUS (*ACMF_ENCAP_COMM_RCV_CALLBACK)(UINT8 *data, UINT16 len, VOID *handle);
typedef STATUS (*ACMF_NEW_TRANSFER_CALLBACK)  (CHAR *buffer,UINT32 length, VOID *handle);
typedef VOID   (*ACMF_APP_NOTIFY) (NU_USBF_USER *user,
                   UINT32 data_send_event, VOID *handle);
#else

typedef STATUS (*ACMF_ENCAP_COMM_RCV_CALLBACK)(UINT8 *data, UINT16 len);
typedef STATUS (*ACMF_NEW_TRANSFER_CALLBACK)  (CHAR *buffer,UINT32 length);
typedef VOID   (*ACMF_APP_NOTIFY) (NU_USBF_USER *user,
                   UINT32 data_send_event, void *handle);
#endif

/* =====================================================================*/

/* ========================== Control Block =========================== */

typedef struct _nu_usbf_acm_dev
{
	CS_NODE node;
	VOID *handle;
	NU_EVENT_GROUP data_send_event;

	/* This buffer is used to store response for host. This response
     * data is sent to host on arrival of GET_ENCAPSULATED_RESPONSE.
     */
    /* Pointer to  response  buffer (global). */
    UINT8  *acmf_resp_buff_ptr;
    UINT32 acmf_resp_length;	
	UINT8  acmf_config_buff[ACMF_CONFIG_BUFF_SIZE];
    UINT8  acmf_notif_buff[ACMF_NOTIF_BUFF_SIZE];
	
    UINT32 acmf_speed;
    UINT8  acmf_stop_bits;
    UINT8  acmf_data_bits;
    UINT8  acmf_parity;

    UINT8 acmf_curr_notif;

    UINT8 acmf_data_flag;
    UINT8 acmf_modem_state;

    /* RS-232 signal used to tell the DCE device that DTE device
     * is now present */
    BOOLEAN acmf_dte_present;

    /* RS-232,RTS */
    BOOLEAN commf_acm_carrier_activate;	
}
NU_USBF_ACM_DEV;

/* Redefine if your control block is any different from this*/
typedef struct _nu_usbf_acm_user
{
    NU_USBF_USER_COMM parent;
    NU_TIMER timer_send_break;		
    /* Structure sent to communication driver. */
    USBF_COMM_USER_NOTIFICATION user_notif;
	NU_USBF_ACM_DEV	*acm_list_head;
	USBF_FUNCTION   *acm_function;

	NU_MEMORY_POOL *pool;
    /* Callback functions */
    ACMF_ENCAP_COMM_RCV_CALLBACK  encap_comm_rcv_callback;
    ACMF_NEW_TRANSFER_CALLBACK    new_transfer_callback;
    ACMF_APP_NOTIFY       app_notify;
}

NU_USBF_ACM_USER;

/* ========= Function Prototypes =========== */

STATUS ACM_Send_Notification(
       NU_USBF_ACM_USER*     cb,
       NU_USBF_ACM_DEV* pdev,
       UINT8                 notif);

NU_USBF_ACM_DEV *ACM_Find_Device (NU_USBF_ACM_USER *cb,
                                  VOID *handle);

/* ==================================================================== */

#include "connectivity/nu_usbf_acm_user_dat.h"

/* ==================================================================== */

#ifdef      __cplusplus
}                                           /* End of C declarations     */
#endif

#endif                                      /* _NU_USBF_ACM_USER_IMP_H_ */

/* ======================  End Of File  =============================== */

