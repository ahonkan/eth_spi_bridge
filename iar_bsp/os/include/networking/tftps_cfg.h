/*************************************************************************
*
*             Copyright 1995-2007 Mentor Graphics Corporation
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
*       tftps_cfg.h                                    
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package  -  Nucleus TFTP Server
*
*   DESCRIPTION
*
*       This file contains the configurable parameters specific to the
*       Nucleus TFTP Server.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef TFTPS_CFG_H
#define TFTPS_CFG_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "nucleus_gen_cfg.h"

/* Set this macro to TRUE to accept and process block size, transmission
 * size and timeout options.  Set this macro to FALSE to ignore all options
 * in incoming TFTP requests
 */
#ifdef CFG_NU_OS_NET_PROT_TFTP_SERVER_RFC2347_COMPLIANT
#define TFTPS_RFC2347_COMPLIANT     CFG_NU_OS_NET_PROT_TFTP_SERVER_RFC2347_COMPLIANT
#else
#define TFTPS_RFC2347_COMPLIANT     NU_FALSE
#endif

/* Default drive for file system */
#ifdef CFG_NU_OS_NET_PROT_TFTP_SERVER_DEFAULT_DRIVE
#define TFTPS_DEFAULT_DRIVE         CFG_NU_OS_NET_PROT_TFTP_SERVER_DEFAULT_DRIVE
#else
#define TFTPS_DEFAULT_DRIVE         0
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* TFTPS_CFG_H */
