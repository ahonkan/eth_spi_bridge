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
*       mdm_defs.h
*
*   COMPONENT
*
*       MDM - Modem Control
*
*   DESCRIPTION
*
*       This file contains constant definitions for the modem module
*       of PPP.
*
*   DATA STRUCTURES
*
*       MDM_BUFFER
*       MDM_LAYER
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_INC_MDM_DEFS_H
#define PPP_INC_MDM_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

/* Max size of the CLI number */
#define     MDM_CLI_MAX                  20

typedef struct _MDM_BUFFER
{
    CHAR    *mdm_head;
    CHAR    *mdm_tail;
    CHAR    *mdm_read;
    CHAR    *mdm_write;
    INT     mdm_buffer_status;
} MDM_BUFFER;

typedef struct _MDM_LAYER_STRUCT
{
    MDM_BUFFER  recv_buffer;
    CHAR        dial_prefix[41];
    UINT8       hangup_attempts;
    UINT8       num_rings;

    CHAR        local_num[MDM_CLI_MAX + 1];

#if(PPP_ENABLE_CLI == NU_TRUE)
    CHAR        remote_num[MDM_CLI_MAX];
#endif

    UINT32      baud_rate;

} MDM_LAYER;

/* The modem can be used in two different modes: a terminal mode and
   network communication mode.  The following definitions are used to
   control which mode is in use. */
#ifndef MDM_NETWORK_COMMUNICATION
#define MDM_NETWORK_COMMUNICATION       1
#endif
#ifndef MDM_TERMINAL_COMMUNICATION
#define MDM_TERMINAL_COMMUNICATION      2
#endif

/* This is the size of the receive buffer when in terminal mode. */
#define     MDM_RECV_BUFFER_SIZE        100

/* This is the size of the modem response buffer used for connecting
   the local modem to a remote modem. */
#define     MDM_RESPONSE_SIZE           81

/* These are the possible values for the mdm_buffer_status. */
#define     MDM_BUFFER_EMPTY            1  /* No data in the buffer. */
#define     MDM_BUFFER_FULL             2  /* The buffer is full. */
#define     MDM_BUFFER_DATA             3  /* There is data in buffer. */

#define     MDM_DIAL_PREFIX             "ATDT"
#define     MDM_HANGUP_STRING           "~~~+++~~~ATH0^M"
#define     MDM_IGNORE_CALLS            "ATS0=0^M"
#define     MDM_ATTENTION               "AT^M"

/* Defines for Caller Line Identification */
#define     MDM_NUM_PREFIX              "NMBR = "
#define     MDM_NUM_TERMINATOR           0x0d

/* How many rings before the phone is answered. To change this simply
   change the 2 to the number of rings that is required.
*/
#define     MDM_ACCEPT_CALL             "ATS0="  /* Number of rings will be
                                                    specified by user. */
#define     MDM_STRING_TERMINATOR       "^M"

/* How many times to try to hangup the modem up before giving up. */
#define     MDM_MAX_HANGUP_ATTEMPTS     5

/* How many times to loop waiting for the modem to respond. This
   pertains to the MDM_Modem_Connected function. Each loop
   we wait one second. */
#define     MDM_MAX_DETECTION_WAIT_LOOPS    3

/* API aliases for Nucleus PPP. */
#define NU_Modem_Rings_To_Answer            MDM_Rings_To_Answer

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_MDM_DEFS_H */
