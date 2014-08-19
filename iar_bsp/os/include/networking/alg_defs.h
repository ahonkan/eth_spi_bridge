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
*       alg_defs.h                                                  
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file contains those defines and data structures necessary to
*       support the Application Level Gateways.
*                                                           
*   DATA STRUCTURES                                                            
*
*       ALG_FTP_ENTRY
*       ALG_FTP_TABLE
*                                               
*   FUNCTIONS                                                                  
*              
*       None.
*                                             
*   DEPENDENCIES                                                               
*
*       None.
*                                                                
******************************************************************************/

#ifndef _ALG_DEFS_
#define _ALG_DEFS_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* _cplusplus */

#define ALG_MAX_FTP_CONNS           NAT_MAX_TCP_CONNS / 2

#define ALG_FTP_PORT_COMMAND        1
#define ALG_FTP_PASV_COMMAND        2
#define ALG_FTP_EPRT_COMMAND        3

#define ALG_CLIENT                  0
#define ALG_HOST                    1

#define ALG_FTP_IP_ADDR_SIZE        16
#define ALG_FTP_PORT_NUMBER_SIZE    7

typedef struct _ALG_FTP_ENTRY
{
    UINT8   alg_side;
    UINT8   alg_padding[3];
    UINT32  alg_timeout;
    INT32   alg_sequence_delta;
    INT32   alg_tcp_index;
} ALG_FTP_ENTRY;

typedef struct _ALG_FTP_TABLE
{
    struct  _ALG_FTP_ENTRY  alg_ftp_entry[ALG_MAX_FTP_CONNS];
    INT32                   alg_next_avail_ftp_entry;
} ALG_FTP_TABLE;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _ALG_DEFS_ */
