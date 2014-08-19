/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       hdlc.h
*
*   COMPONENT
*
*       HDLC - High-level Data Link Control Protocol
*
*   DESCRIPTION
*
*       This file contains declarations and definitions used by hdlc.c.
*
*   DATA STRUCTURES
*
*       HDLC_TEMP_BUFFER
*
*   DEPENDENCIES
*
*       nerrs.h
*       tcp_errs.h
*       mem_defs.h
*       dev.h
*       urt_defs.h
*       mdm_defs.h
*
*************************************************************************/
#ifndef PPP_INC_HDLC_H
#define PPP_INC_HDLC_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

#include "networking/nerrs.h"
#include "networking/tcp_errs.h"
#include "networking/mem_defs.h"
#include "networking/dev.h"
#include "drivers/serial.h"
#include "drivers/mdm_defs.h"

/* Define the sizes for the max TX/RX unit over the PPP link
   and the sizes of the headers and Fast Checksum Sequence (FCS). Note
   that this value strictly defines our max RX unit since it is used to
   declare the size of internal buffers. This value, though, can change
   during PPP negotiation for the max TX unit. So it is possible to have
   different RX and TX max values. NOTE: the minimum MTU is 1500. */
#define HDLC_MTU                        1500

#define HDLC_MAX_PROTOCOL_SIZE           2
#define HDLC_MAX_ADDR_CONTROL_SIZE       2
#define HDLC_FCS_SIZE_BYTE2              2
#define HDLC_FCS_SIZE_BYTE4              4  /* Not supported. Left for
                                               SNMP */

#define HDLC_FCS_SIZE                    HDLC_FCS_SIZE_BYTE2

/* Sum the above up. This will be the size used to create the static
   arrays used by the PPP RX LISR to store the incoming bytes. */
#define PPP_RX_HEADER_SIZE              (HDLC_FCS_SIZE         +     \
                                        HDLC_MAX_PROTOCOL_SIZE +     \
                                        HDLC_MAX_ADDR_CONTROL_SIZE)

#define PPP_MAX_RX_SIZE                 (HDLC_MTU + PPP_RX_HEADER_SIZE)

/* Size of the HISR Stack. */
#define PPP_HISR_STACK_SIZE             2000

/* PPP HDLC frame numbers */
#define PPP_HDLC_FRAME                  0x7e
#define PPP_HDLC_ADDRESS                0xff
#define PPP_HDLC_CONTROL                0x03
#define PPP_HDLC_CONTROL_ESCAPE         0x7d
#define PPP_HDLC_TRANSPARENCY           (UINT8)0x20
#define PPP_INIT_FCS16                  0xffff  /* Initial FCS value */
#define PPP_GOOD_FCS16                  0xf0b8  /* Good final FCS value */

#define PPP_MAX_ACCM                    32


/* Structure used to hold the packet size and pointer so the LISR
   can give it to the HISR. */
typedef struct _hdlc_temp_buffer
{
    struct _DV_DEVICE_ENTRY *device;
    UINT16                  size;
    UINT8                   buffer[PPP_MAX_RX_SIZE];
} HDLC_TEMP_BUFFER;


/* Function declarations */
STATUS          HDLC_Initialize(DV_DEVICE_ENTRY*);
STATUS          HDLC_Init(DV_DEVICE_ENTRY *);
STATUS          HDLC_TX_Packet (DV_DEVICE_ENTRY*, NET_BUFFER*);
STATUS          HDLC_RX_Packet (DV_DEVICE_ENTRY* , INT);
VOID            HDLC_TX_HISR_Entry (VOID);
VOID            HDLC_RX_HISR_Entry (VOID);
UINT16          HDLC_Compute_TX_FCS (UINT16, NET_BUFFER*);
UINT16          HDLC_Compute_RX_FCS (UINT16, HDLC_TEMP_BUFFER*);

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_HDLC_H */
