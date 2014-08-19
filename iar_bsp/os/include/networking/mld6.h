/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
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
*       mld6.h                                       
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for Multicast Listener Discovery for IPv6 nodes.
*                                                                          
*   DATA STRUCTURES                                                          
*                 
*       mld6_hdr                                                         
*
*   DEPENDENCIES                                                             
*                                                                          
*       No other file dependencies                                          
*                                                                          
*************************************************************************/
/* Portions of this program were written by: */
/*************************************************************************
*                                                                         
* Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000 and 2001 WIDE Project.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of the project nor the names of its contributors
*    may be used to endorse or promote products derived from this 
*    software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS 
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
* THE POSSIBILITY OF SUCH DAMAGE.
*                                                                         
*************************************************************************/

#ifndef MLD6_H
#define MLD6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* MLD Packet Types */
#define MLD6_LISTENER_QUERY         130 /* Multicast Listener Query */
#define MLDV1_LISTENER_REPORT       131 /* Multicast Listener Report */
#define MLD6_LISTENER_DONE          132 /* Multicast Listener Done */
#define MLDV2_LISTENER_REPORT       143 /* MLDv2 Listener Report */

/* MLD Interoperability */
#define MLDV1_COMPATIBILITY         1
#define MLDV2_COMPATIBILITY         2
#define MLDV1_COMPAT_TIMER_RUNNING  0x1
/* MLD Packets Offsets */
#define MLD6_TYPE_OFFSET                    0
#define MLD6_CODE_OFFSET                    1
#define MLD6_CHECKSUM_OFFSET                2
#define MLD6_MAX_RESP_OFFSET                4
#define MLD6_RES_OFFSET                     6
#define MLD6_NUMBER_OF_MCAST_ADDR_RECORDS   6
#define MLD6_MULTICAST_ADDRESS_RECORD       8
#define MLD6_MULTI_ADDR_OFFSET              8
#define MLD6_QUERIER_QUERY_INTERVAL_CODE    25
#define MLD6_NUMBER_OF_SOURCES              26
#define MLD6_SOURCE_ADDRESSES               28

    
/* MLD Report Offsets */
#define MLD6_REPORT_TYPE_OFFSET                     0
#define MLD6_REPORT_RESERVED_OFFSET                 1
#define MLD6_REPORT_CHECKSUM_OFFSET                 2
#define MLD6_REPORT_RESERVED2_OFFSET                4
#define MLD6_REPORT_NUM_MULTI_ADDR_RECORDS_OFFSET   6

#define MLD6_RECORD_TYPE_OFFSET                     0
#define MLD6_RECORD_AUX_DATA_LEN_OFFSET             1
#define MLD6_RECORD_NUM_OF_SOURCES                  2
#define MLD6_RECORD_MULTICAST_ADDR_OFFSET           4
#define MLD6_RECORD_SOURCE_ADDR_OFFSET              20

#define MLD6_ROUTER_SIDE_PROCESS_SUPPRESS(x)    (((UINT8)(*(x + 24)) >> 3) & 0x01)  
#define MLD6_QUERIER_ROBUST_VARIABLE(x)         (((UINT8)(*(x + 24)) & 0x07)  


/* MLD States */
#define MLD6_NON_LISTENER           0
#define MLD6_DELAYING_LISTENER      1
#define MLD6_IDLE_LISTENER          2

/* MLD Query Types */
#define MLD6_GENERAL_QUERY          0
#define MLD6_MULTI_ADDR_SPEC_QUERY  1

#define MLD6_MIN_PKT_LENGTH         24

#define MLDV1_HEADER_SIZE           24
#define MLDV2_HEADER_SIZE           8
#define MLDV2_RECORD_SIZE           20
#define MLD6_ROUTER_ALERT_LENGTH    8

#define MLD6_HOP_LIMIT              1


#define MLD6_MESSAGE_TYPE(index)                                    \
    ( ((index >= MLDV1_MIN_TMR_INDEX) &&                            \
       (index <= MLDV1_MAX_TMR_INDEX)) ?                            \
                                MLDV1_LISTENER_REPORT  :            \
      ((index >= MLDV2_MODE_INC_MIN_TMR_INDEX) &&                   \
       (index <= MLDV2_MODE_INC_MAX_TMR_INDEX)) ?                   \
                                MULTICAST_MODE_IS_INCLUDE  :        \
      ((index >= MLDV2_MODE_EXC_MIN_TMR_INDEX) &&                   \
       (index <= MLDV2_MODE_EXC_MAX_TMR_INDEX)) ?                   \
                                MULTICAST_MODE_IS_EXCLUDE  :        \
      ((index >= MLDV2_CHANGE_TO_INC_MIN_TMR_INDEX) &&              \
       (index <= MLDV2_CHANGE_TO_INC_MAX_TMR_INDEX)) ?              \
                                MULTICAST_CHANGE_TO_INCLUDE_MODE  : \
      ((index >= MLDV2_CHANGE_TO_EXC_MIN_TMR_INDEX) &&              \
       (index <= MLDV2_CHANGE_TO_EXC_MAX_TMR_INDEX)) ?              \
                                MULTICAST_CHANGE_TO_EXCLUDE_MODE  : \
      ((index >= MLDV2_ALLOW_NEW_SRCS_MIN_TMR_INDEX) &&             \
       (index <= MLDV2_ALLOW_NEW_SRCS_MAX_TMR_INDEX)) ?             \
                                MULTICAST_ALLOW_NEW_SOURCES  :      \
      ((index >= MLDV2_BLOCK_OLD_SRCS_MIN_TMR_INDEX) &&             \
       (index <= MLDV2_BLOCK_OLD_SRCS_MAX_TMR_INDEX)) ?             \
                                MULTICAST_BLOCK_OLD_SOURCES : NU_INVAL)
    

STATUS      MLD6_Input(const IP6LAYER *, DV_DEVICE_ENTRY *, 
                       const NET_BUFFER *);
VOID        MLD6_Start_Listening(IP6_MULTI *, UINT8);
VOID        MLD6_Send_Startup_Report(TQ_EVENT, UNSIGNED, UNSIGNED);
VOID        MLD6_Stop_Listening_Addr(const UINT8 *, const DV_DEVICE_ENTRY *);
VOID        MLD6_Stop_Listening_Multi(IP6_MULTI *);
VOID        MLD6_Timer_Expired(TQ_EVENT, UNSIGNED, UNSIGNED);
VOID        MLD6_Update_Entry(IP6_MULTI *, UINT16);
VOID        MLD6_Expire_Entry(IP6_MULTI *);
STATUS      MLD6_Output(IP6_MULTI *, UINT8, UINT8 *, UINT16);
NET_BUFFER  *MLD6_Build_Packet(const IP6_MULTI *, UINT8, UINT16, const UINT8 *);
UINT32      MLD6_Calculate_Max_Response_Delay(UINT16);
VOID        MLD6_Update_Host_Compatibility(const NET_BUFFER *, MULTI_IP *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif
