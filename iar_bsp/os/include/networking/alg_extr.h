/****************************************************************************
*
*            Copyright Mentor Graphics Corporation 2001-2006 
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
****************************************************************************/
/****************************************************************************
*                                                                            
*   FILENAME                                                                          
*                                                                                    
*       alg_extr.h                                                  
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file contains those function definitions necessary to support 
*       the Application Level Gateways.
*                                                           
*   DATA STRUCTURES                                                            
*
*       None.
*                                               
*   FUNCTIONS                                                                  
*              
*       None.
*                                             
*   DEPENDENCIES                                                               
*
*       alg_defs.h
*                                                                
******************************************************************************/

#ifndef _ALG_EXTR_
#define _ALG_EXTR_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* _cplusplus */

#include "networking/alg_defs.h"

VOID            ALG_Init(VOID);
STATUS          ALG_Modify_Payload(NET_BUFFER *buf_ptr, NAT_PACKET *nat_packet, 
                                   INT32 index, UINT8 *old_data, 
                                   DV_DEVICE_ENTRY *internal_device);
STATUS          ALG_FTP_Translate(NET_BUFFER *buf_ptr, const NAT_PACKET *nat_packet, INT32 index,
                                  UINT8 *old_data, DV_DEVICE_ENTRY *internal_device);
STATUS          ALG_ICMP_Translate(const NET_BUFFER *buf_ptr, const NAT_PACKET *nat_packet,
                                   UINT8 *old_data);
INT32           ALG_Add_FTP_Entry(INT32 index, INT32 sequence_delta);
INT32           ALG_Find_FTP_Entry(UINT8 device_type, UINT32 source_addr, UINT16 source_port, 
                                   UINT32 dest_addr, UINT16 dest_port);
VOID            ALG_Delete_FTP_Entry(INT32 index);
VOID            ALG_FTP_Adjust_Sequence(const NET_BUFFER *buf_ptr, UINT8 device_type,
                                        INT32 sequence_delta, UINT32 ip_check_offset);
UINT8           ALG_FTP_Compute_Address(UINT8 *alg_buffer, UINT8);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _ALG_EXTR_ */
