/*************************************************************************
*
*               Copyright 2002 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
****************************************************************************
* FILE NAME                                         
*
*       ppe_defs.h                                
*
* COMPONENT
*
*       PPE - PPPoE Driver Definitions
*
* DESCRIPTION
*
*       Constants and data structure definitions for the PPPoE driver.
*
* DATA STRUCTURES
*
*       PPE_LAYER
*       PPE_FRAME
*       PPE_TAG
*
* DEPENDENCIES
*
*       None
*
***************************************************************************/
#ifndef PPE_DEFS_H
#define PPE_DEFS_H

#include "networking/externs.h"
#include "networking/arp.h"
#include "drivers/nu_ppp.h"

#include "drivers/ppe_cfg.h"


/**** PPPoE type specifications, as defined in RFC2516 ****/

/* Packet types */
#define PPE_PADI                0x09
#define PPE_PADO                0x07
#define PPE_PADR                0x19
#define PPE_PADS                0x65
#define PPE_PADT                0xa7

/* PPPoE type and version fields */
#define PPE_VERTYPE             0x11

/* Discovery tags */
#define PPE_TAG_END             0x0000
#define PPE_TAG_SERVICE         0x0101
#define PPE_TAG_ACNAME          0x0102
#define PPE_TAG_HOSTUNIQ        0x0103
#define PPE_TAG_ACCOOKIE        0x0104
#define PPE_TAG_VENDORSP        0x0105
#define PPE_TAG_RELAYID         0x0110
#define PPE_TAG_SERVICERR       0x0201
#define PPE_TAG_SYSTEMERR       0x0202
#define PPE_TAG_GENERR          0x0203

/* Maximum transfer unit */
#define PPE_MTU                 1492

/************* End of RFC2516 specification ***************/



/* Miscellaneous constants */
#define PPE_TAG_HEADER_SIZE     4
#define PPE_PADx_MAXSIZE        1484


/* These are special purpose session id constants. The empty session id
   signifies that the virtual device exists, but no user process is
   attached. The init id signifies that it is ready to establish a new 
   link with a peer. */
#define PPE_EMPTY_SESSION_ID    0xffff
#define PPE_INIT_SESSION_ID     0x0000


/* These flags are used when searching and assigning the two types
   of identifiers for the device. */
#define PPE_SESSION_ID          0x0001
#define PPE_VDEVICE_ID          0x0002


/* Sizes of the relay tag and peer cookie/hostuniq */
#define PPE_RELAY_TAG_SIZE      12
#define PPE_COOKIE_SIZE         8


/* This flag is given as an argument to PPE_Session_Terminate(),
   differentiating it as a call from within PPPoE, not from PPP.
   The value is arbitrary, but must be different from other flags. */
#define PPE_STOP_PPP            0xCE


/* Options for PPEH_Control_Timer() */
#define PPE_STOP_TIMER          1
#define PPE_RESET_TIMER         2
#define PPE_RESTART_TIMER       3


/* Timeout options for PADI and PADR packets. The initial timeout is given
   in ticks. The time is doubled with each restart. The default is 100ms
   initial timeout with 5 restarts. */
#define PPE_TIMEOUT_VALUE       100
#define PPE_MAX_RESTARTS        5


/* This is the timer event to be placed in the event queue. */
#define PPE_PADx_TIMEOUT        14


/* Discovery event flags for PPE_LAYER.ppe_events   */
#define PPE_CONNECT             1
#define PPE_TIMEOUT             2


/* Possible bitmaps for the PPE_LAYER.status word */
#define PPE_DISCOVERY_MASK      0x07L
#define PPE_PADI_SENT           0x00000001L
#define PPE_PADO_SENT           0x00000002L
#define PPE_PADR_SENT           0x00000003L
#define PPE_PADS_SENT           0x00000004L

#define PPE_AC_MODE             0x00000008L
#define PPE_HOST_MODE           0x00000010L
#define PPE_SESSION             0x00000020L
#define PPE_ACTIVE              0x00000040L

#define PPE_SERVICE_FOUND       0x00000080L
#define PPE_AC_NAME_FOUND       0x00000100L
#define PPE_RELAY_FOUND         0x00000200L
#define PPE_REQUEST_BY_NAME     0x00000400L
#define PPE_COOKIE_VERIFIED     0x00000800L

#define PPE_AC_ERROR            0x00001000L
#define PPE_SERVICE_ERROR       0x00002000L



/* Messages used in some response PADx packets */
#define PPE_SERVICE_ERROR_MSG   "Invalid service request."
#define PPE_BAD_ACNAME_MSG      "Invalid AC Name."



/* Information about the host or AC peer that is connected with
   the virtual device via PPPoE. Only information relevant to 
   the PPPoE layer is included here. */
typedef struct PPE_Node
{
    UINT8                    *ppe_fname;        /* Foreign name */
    UINT8                    ppe_cookie[8];
    UINT8                    ppe_mac_addr[6];
} PPE_NODE;



/* This is the main PPPoE data structure that is linked into the
   parent PPP layer structure. */
typedef struct PPE_Layer
{
    DV_DEVICE_ENTRY          *ppe_hdevice;
    UINT8                    *ppe_lname;        /* Local name */
    NET_BUFFER               *ppe_bufptr;
    UINT32                   ppe_status;
    UINT32                   ppe_countdown;
    PPE_SERVICE              ppe_service;
    PPE_NODE                 ppe_node;
    NU_EVENT_GROUP           ppe_event;
    UINT16                   ppe_session_id;
    UINT16                   ppe_vdevice_id;
    STATUS                   (*ppe_event_handler)(DV_DEVICE_ENTRY*, UNSIGNED);
} PPE_LAYER;



/* This structure combines the necessary data structures for use when
   calling Net_Ether_Input() during packet output. */
typedef union PPE_Addr
{
    struct ARP_MAC_HEADER_STRUCT mac;
    struct SCK_SOCKADDR_IP_STRUCT sck;
} PPE_ADDR;



/* The structure of a PPPoE tag. Using this structure, it is easier
   to pass tag information as function arguments. */
typedef struct PPE_Tag
{
    UINT16      ppe_type;
    UINT16      ppe_offset;
    UINT16      ppe_length;
    UINT8 HUGE  *ppe_string;
} PPE_TAG;



/* This is the main PPPoE header structure. Using this structure, 
   it is easier to pass tag information as function arguments. */
typedef struct PPE_Frame
{
    UINT8       ppe_vertype;
    UINT8       ppe_dcode;
    UINT16      ppe_sid;
    UINT16      ppe_length;
    UINT8       ppe_macaddr[6];
} PPE_FRAME;


/* These macros allow PPPoE to be backward compatible with PPP 2.5. */
#if (NET_VERSION_COMP > NET_4_5)
#define hwi_connect         hwi.connect
#define hwi_disconnect      hwi.disconnect
#define hwi_passive         hwi.passive
#define hwi_dev_ptr         hwi.dev_ptr
#define hwi_itype           hwi.itype
#else
#define hwi_connect         connect
#define hwi_disconnect      disconnect
#define hwi_passive         passive
#define hwi_dev_ptr         dev_ptr
#define hwi_itype           itype
#endif

#define IP_HDR_LEN          20
#define TCP_HDR_LEN         20

#undef link_layer
#endif

