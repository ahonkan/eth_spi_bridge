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
*       nu_ppp.h
*
*   COMPONENT
*
*       PPP - Core component of PPP
*
*   DESCRIPTION
*
*       This file includes all required header files to allow
*       access to the Nucleus PPP API. Include this file in applications
*       that use PPP.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nu_net.h
*       ppp_cfg.h
*       nu_net6.h
*       ppp_defs.h
*       ppp_extr.h
*       lcp_extr.h
*       ncp_extr.h
*       chp_extr.h
*       pap_extr.h
*       pp6_extr.h
*       pm_extr.h
*       ppp_l2tp.h
*       ppp_mp.h
*       ppp_dc.h
*       chapm_extr.h
*
*************************************************************************/
#ifndef PPP_INC_NU_PPP_H
#define PPP_INC_NU_PPP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

#include "networking/nu_net.h"
#include "drivers/ppp_cfg.h"

/******************** Begin configuration validation ********************/

#if (PPP_DEBUG_PRINT_OK + LCP_DEBUG_PRINT_OK + NCP_DEBUG_PRINT_OK\
    + CHAP_DEBUG_PRINT_OK + PAP_DEBUG_PRINT_OK + HDLC_DEBUG_PRINT_OK\
    + MDM_DEBUG_PRINT_OK + PPP_LOG_PACKET > 0)

/* Printing of debug messages is enabled. */
#define NU_PPP_DEBUG                    NU_TRUE

#if (PPP_LOG_PACKET == NU_TRUE)
extern VOID PPP_PrintPkt(UINT8*, UINT32, CHAR*);
#else
#define PPP_PrintPkt(a,b,c)
#endif

#else
/* Printing of debug messages is disabled. */
#define NU_PPP_DEBUG                    NU_FALSE
#define PPP_PrintPkt(a,b,c)

#endif

#if (INCLUDE_SEC_MIB == NU_TRUE)

#if (PPP_USE_CHAP == NU_FALSE && PPP_USE_PAP == NU_FALSE)
#error INCLUDE_SEC_MIB must be disabled if no PPP authentication is used.

#elif (PPP_ENABLE_UM_DATABASE == NU_FALSE)
#error INCLUDE_SEC_MIB depends on PPP_ENABLE_UM_DATABASE. Both must be true.
#endif

#endif

#if (PPP_USE_DNS2 == NU_TRUE && PPP_USE_DNS1 == NU_FALSE)
#error PPP_USE_DNS2 == NU_TRUE requires PPP_USE_DNS1 == NU_TRUE
#endif

#if (PPP_USE_DNS1 == NU_TRUE && INCLUDE_DNS == NU_FALSE)
#undef PPP_USE_DNS1
#undef PPP_USE_DNS2
#define PPP_USE_DNS1    NU_FALSE
#define PPP_USE_DNS2    NU_FALSE
#endif

#if (HDLC_POLLED_TX == NU_TRUE)
#define PPP_POLLED_TX
#endif

/********************* End configuration validation *********************/

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/nu_net6.h"
#endif

#include "drivers/ppp_defs.h"
#include "drivers/ppp_extr.h"
#include "drivers/lcp_extr.h"
#include "drivers/ncp_extr.h"
#include "drivers/ccp_extr.h"
#include "drivers/chp_extr.h"
#include "drivers/pap_extr.h"
#include "drivers/pp6_extr.h"
#include "drivers/pm_extr.h"
#include "drivers/mppe_extr.h"
#include "drivers/ppp_l2tp.h"
#include "drivers/ppp.h"

#if(INCLUDE_PPP_MP == NU_TRUE)
#include "drivers/ppp_mp.h"
#endif

#if(INCLUDE_PPP_DC_PROTOCOL == NU_TRUE)
#include "drivers/ppp_dc.h"
#endif

#if (HDLC_POLLED_TX == NU_FALSE)
#error interrupt driven transmission is not currently supported 
#endif

#if ((INCLUDE_PPP_MP  == NU_TRUE) && (INCLUDE_IF_STACK == NU_FALSE))
#error INCLUDE_PPP_MP == NU_TRUE requires INCLUDE_IF_STACK == NU_TRUE in net/net_cfg.h
#endif

#if((PPP_USE_CHAP_MS1 == NU_TRUE) || (PPP_USE_CHAP_MS2 == NU_TRUE))
/* Compile time warnings */
#if ((PPP_USE_CHAP_MS1 == NU_TRUE) && (PPP_USE_CHAP == NU_FALSE))
#error PPP_USE_CHAP_MS1 == NU_TRUE requires PPP_USE_CHAP == NU_TRUE in ppp_cfg.h
#endif

#if ((PPP_USE_CHAP_MS2 == NU_TRUE) && (PPP_USE_CHAP == NU_FALSE))
#error PPP_USE_CHAP_MS2 == NU_TRUE requires PPP_USE_CHAP == NU_TRUE in ppp_cfg.h
#endif

#if ((PPP_USE_CHAP_MS1 == NU_FALSE) && (PPP_USE_CHAP_MS2 == NU_FALSE) && (PPP_ENABLE_MPPE == NU_TRUE))
#error PPP_ENABLE_MPPE requires either PPP_USE_CHAP_MS1 == NU_TRUE or PPP_USE_CHAP_MS2 == NU_TRUE
#endif
    
#include "drivers/chapm_extr.h"
#endif /* (PPP_USE_CHAP_MS1 == NU_TRUE) || (PPP_USE_CHAP_MS2 == NU_TRUE) */


extern NU_MEMORY_POOL *PPP_Memory;


#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_NU_PPP_H */
