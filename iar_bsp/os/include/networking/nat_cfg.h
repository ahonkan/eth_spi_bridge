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
*       nat_cfg.h                                                   
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file holds all defines that control the various configuration 
*       settings of Nucleus NAT.
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
*       None.
*                                                                
******************************************************************************/

#ifndef _NAT_CFG_
#define _NAT_CFG_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "nucleus_gen_cfg.h"

#if (CFG_NU_OS_NET_STACK_INCLUDE_IPV4 == NU_FALSE)
#error NAT requires IPv4. Please enable the "include_ipv4" option in NET stack to use NAT.
#endif

/* These definitions control which version of NAT the build should be
   compatible with. This should allow new versions of NAT to be shipped but
   remain compatible with applications or drivers designed for previous
   versions. */
#define     NAT_1_0         1       /* NAT 1.1 */
#define     NAT_1_2         2       /* NAT 1.2 */
#define     NAT_1_3         3       /* NAT 1.3 */
#define     NAT_1_4         4       /* NAT 1.4 */

#define NAT_VERSION_COMP    NAT_1_4        /* The version for which compatibility
                                              is desired. */

#define     NAT_INCLUDE_FTP_ALG     NU_TRUE
#define     NAT_INCLUDE_ICMP_ALG    NU_TRUE

#define     NAT_MIN_PORT        61000
#define     NAT_MAX_TCP_CONNS   50
#define     NAT_MAX_UDP_CONNS   50
#define     NAT_MAX_ICMP_CONNS  25

#define     NAT_CONN_TIMEOUT    (4 * 60 * TICKS_PER_SECOND)           /* 4 minutes */
#define     NAT_CLOSE_TIMEOUT   (4 * 60 * TICKS_PER_SECOND)           /* 4 minutes */

#define     NAT_TCP_TIMEOUT     (24 * 60 * 60 * TICKS_PER_SECOND)     /* 24 hours */
#define     NAT_UDP_TIMEOUT     (4 * 60 * TICKS_PER_SECOND)           /* 4 minutes */
#define     NAT_ICMP_TIMEOUT    (4 * 60 * TICKS_PER_SECOND)           /* 4 minutes */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* _NAT_CFG_ */
