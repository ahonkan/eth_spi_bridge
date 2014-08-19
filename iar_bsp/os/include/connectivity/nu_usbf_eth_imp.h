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
* FILE NAME
*
*       nu_usbf_eth_imp.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Ethernet User Driver
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for ETH User Driver.
*
* DATA STRUCTURES
*
*       NU_USBF_ETH_DISPATCH                ETH User Driver Dispatch Table.
*       NU_USBF_ETH_DEV                     Ethernet device control block.
*       NU_USBF_ETH                         ETH User Driver Control Block.
*       ETHF_MULTICAST_FILTER               Multicast filter structure.
*       ETHF_POWER_MNG_FILTER               Power management pattern
*                                           filter.
*       ETHF_PKT_FILTER                     Ethernet packet filter.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_eth_dat.h                   Dispatch Table Definitions.
*       nu_usbf_user_comm_ext.h             Communication user external
*                                           definitions.
*
**************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_ETH_IMP_H_
#define _NU_USBF_ETH_IMP_H_

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

/* ============= Constants ============= */

/* These definitions control which version of the user API should be
   compatible with. This should allow new versions of API to be shipped but
   remain compatible with applications for previous versions. */
#define EF_1_6         1       /* EF 1.6 */
#define EF_2_0         2       /* EF 2.0 */

/* The version for which compatibility is desired. */
#ifndef EF_VERSION_COMP
    #define EF_VERSION_COMP    EF_2_0
#endif

#define EF_SUBCLASS       0x6

/* Length of response to be sent to host after the
 * EF_GET_ENCAPSULATED_RESPONSE. This length depends on the vendor. 16 is
 * being selected as demonstration purpose.
 */
#define EF_MAX_RESPONSE_LENGTH         16
#define EF_MAX_MULTICAST_FILTERS       16
#define EF_MAX_POWER_PATTERN_FILTERS   4
#define EF_MAX_NUM_STATS               32
#define EF_MAX_MASK_SIZE               8
#define EF_MAX_PATTERN_SIZE            8*EF_MAX_MASK_SIZE
#define EF_SPD_ARR_SIZE                8
#define EF_PWR_PTRN_STAT_SIZE          2
#define EF_ETHERNET_STAT_SIZE          4
#define EF_CONN_SPD_NOTIF_SIZE         8

/* Notifications sent to the class driver. */

#define EF_NETWORK_CONNECTION          0x00
#define EF_RESPONSE_AVAILABLE          0x01
#define EF_CONNECTION_SPEED_CHANGE     0x2A

/* conn_status values */
#define EF_CONNECTED       1
#define EF_DISCONNECTED    0

/* Command received from the Host. */
#define EF_SEND_ENCAPSULATED_COMMAND     0x00
#define EF_GET_ENCAPSULATED_RESPONSE     0x01
#define EF_SET_ETHF_MULTICAST_FILTERS    0x40
#define EF_SET_ETH_PWR_MGT_PTRN_FILTER   0x41
#define EF_GET_ETH_PWR_MGT_PTRN_FILTER   0x42
#define EF_SET_ETHERNET_PACKET_FILTER    0x43
#define EF_GET_ETHERNET_STATISTIC        0x44

/* Power management pattern filter status. */
#define    EF_TRUE     0x0001
#define    EF_FALSE    0x0000

/* Error messages */
#define NU_USB_ETH_NOT_CONNECTED        -3899

/* Miscellaneous */
#define EF_MASK_BIT4 (1<<4)
#define EF_MASK_BIT3 (1<<3)
#define EF_MASK_BIT2 (1<<2)
#define EF_MASK_BIT1 (1<<1)
#define EF_MASK_BIT0 (1<<0)

/* Ethernet statistics indices for particular entry in the array. */
#define EF_XMIT_OK                 0x01
#define EF_RCV_OK                  0x02
#define EF_XMIT_ERROR              0x03
#define EF_RCV_ERROR               0x04
#define EF_RCV_NO_BUFFER           0x05
#define EF_DIRECTED_BYTES_XMIT     0x06
#define EF_DIRECTED_FRAMES_XMIT    0x07
#define EF_MULTICAST_BYTES_XMIT    0x08
#define EF_MULTICAST_FRAMES_XMIT   0x09
#define EF_BROADCAST_BYTES_XMIT    0x0A
#define EF_BROADCAST_FRAMES_XMIT   0x0B
#define EF_DIRECTED_BYTES_RCV      0x0C
#define EF_DIRECTED_FRAMES_RCV     0x0D
#define EF_MULTICAST_BYTES_RCV     0x0E
#define EF_MULTICAST_FRAMES_RCV    0x0F
#define EF_BROADCAST_BYTES_RCV     0x10
#define EF_BROADCAST_FRAMES_RCV    0x11
#define EF_RCV_CRC_ERROR           0x12
#define EF_TRANSMIT_QUEUE_LENGTH   0x13
#define EF_RCV_ERROR_ALIGNMENT     0x14
#define EF_XMIT_ONE_COLLISION      0x15
#define EF_XMIT_MORE_COLLISIONS    0x16
#define EF_XMIT_DEFERRED           0x17
#define EF_XMIT_EF_MAX_COLLISIONS  0x18
#define EF_RCV_OVERRUN             0x19
#define EF_XMIT_UNDERRUN           0x1A
#define EF_XMIT_HEARTBEAT_FAILURE  0x1B
#define EF_XMIT_TIMES_CRS_LOST     0x1C
#define EF_XMIT_LATE_COLLISIONS    0x1D

/* =============Dispatch Table ============= */
typedef struct _nu_usbf_eth_dispatch
{
    NU_USBF_USER_COMM_DISPATCH  dispatch;
}
NU_USBF_ETH_DISPATCH;

/* ============= Control Block ============= */
/* Multicast filter structure. */
typedef struct _eth_multicast_filter
{
    UINT8 byte7;
    UINT8 byte6;
    UINT8 byte5;
    UINT8 byte4;
    UINT8 byte3;
    UINT8 byte2;
    UINT8 byte1;
    UINT8 byte0;
}
ETHF_MULTICAST_FILTER;

/* Power management pattern filter structure. */
typedef struct _eth_power_mng_filter
{
    UINT16 filter_num;
    UINT16 filter_status;
    UINT16 mask_size;
    UINT8  mask[EF_MAX_MASK_SIZE];
    UINT8  pattern[EF_MAX_PATTERN_SIZE];
    DATA_ELEMENT pwr_filter[2];
}
ETHF_POWER_MNG_FILTER;

/* Ethernet packet filter structure. */
/* Each of individual filter can be NU_TRUE or NU_FALSE. */
typedef struct _eth_pkt_filter
{
    BOOLEAN packet_type_multicast;
    BOOLEAN packet_type_broadcast;
    BOOLEAN packet_type_directed;
    BOOLEAN packet_type_all_multicast;
    BOOLEAN packet_type_promiscuous;
    DATA_ELEMENT eth_pad[3];
}
ETHF_PKT_FILTER;

/* Ethernet device */
typedef struct _nu_usbf_eth_dev
{
	CS_NODE	node;	
	VOID    *handle;
	
    /* Structure sent to communication driver. */
    USBF_COMM_USER_NOTIFICATION user_notif;

    /* Array of multicast filters having 8 bytes entry for each filter. */
    ETHF_MULTICAST_FILTER multicast_filter_array[EF_MAX_MULTICAST_FILTERS];

    /* Array of power management pattern filters. */
    ETHF_POWER_MNG_FILTER   power_pattern_filter_array
                                            [EF_MAX_POWER_PATTERN_FILTERS];

    /* Structure for ethernet packet filter. */
    ETHF_PKT_FILTER  packet_filter;

	UINT32  us_bit_rate;                    /* Upstream port bit rate. */
    UINT32  ds_bit_rate;                    /* Downstream port bit rate.*/

    /* Array to hold different ethernet statistics. */
    UINT32 ethernet_statistics[EF_MAX_NUM_STATS];

    /* Buffer to hold specific ethernet statistic. */
    UINT32 eth_stat_buffer;

    /* Buffer to hold RESPONSE of the last ENCAPSULATED_COMMAND. */
    UINT8   response_array[EF_MAX_RESPONSE_LENGTH];

    /* Buffer to hold 8 byte data for the CONN_SPEED_CHNG notification. */
    UINT8   conn_speed_array[EF_SPD_ARR_SIZE];

    UINT16  response_length;                /* Response length */

    /* Buffer to hold power management pattern filter status. */
    UINT16   power_pattern_status_buffer;

    /* Currently sent notification. This should be set to COMMF_NO_NOTIF
     * (0xFF) when no notification is sent.
     */
    UINT8   curr_notif;

    /* Next notification to be sent. Set this if a response is expected
     * for notification or 2 notification are to be sent one by one.
     */
    UINT8   next_notif;

    /* Current connection status
     * 1 - Connected
     * 0 - Disconnected
     */
    UINT8   conn_status;
    /* Number of multicast filters. */
    UINT8   num_multicast_filters;
	
}NU_USBF_ETH_DEV;

/* Ethernet user driver function structure. */
typedef struct _nu_usbf_eth
{
    NU_USBF_USER_COMM parent;

	NU_USBF_ETH_DEV *eth_dev_list_head;

	NU_MEMORY_POOL  *pool;
	USBF_FUNCTION   *eth_function;

    /* Application notify callback*/
    COMMF_APP_NOTIFY    eth_app_notify;

    /* Application Receive callback*/
    COMMF_APP_RX_CALLBACK   eth_rcvd_cb;

    /* Application Tx_Done callback*/
    COMMF_APP_TX_DONE       eth_tx_done;

    /* Application IOCTL callback.*/
    COMMF_APP_IOCTL         eth_ioctl;

}
NU_USBF_ETH;

/* ============= Function prototypes ============= */
STATUS EF_Set_Multi_Filters_Rcvd(NU_USBF_ETH *eth,
								 NU_USBF_ETH_DEV *pdev,
                                 USBF_COMM_USER_CMD *user_cmd);

STATUS EF_Set_Power_Filters_Rcvd(NU_USBF_ETH *eth,
								 NU_USBF_ETH_DEV *pdev,
                                 USBF_COMM_USER_CMD *user_cmd);

STATUS EF_Set_Pkt_Filters_Rcvd(NU_USBF_ETH *eth,
							   NU_USBF_ETH_DEV *pdev,
                               USBF_COMM_USER_CMD *user_cmd);

NU_USBF_ETH_DEV *EF_Find_Device (NU_USBF_ETH *cb,
                                 VOID *handle);

/* ==================================================================== */

#include "connectivity/nu_usbf_eth_dat.h"

/* ==================================================================== */
#ifdef          __cplusplus
}                                           /* End of C declarations    */
#endif

#endif                                      /* _NU_USBF_ETH_IMP_H_ */

/* ======================  End Of File  =============================== */
