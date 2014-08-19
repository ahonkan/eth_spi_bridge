/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ips_api.h
*
* COMPONENT
*
*       IPSEC - APIs
*
* DESCRIPTION
*
*       This file contains APIs for the creation of policies and security
*       associations.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       ips_ip.h
*       ips_cfg.h
*       ips.h
*       ips_esn.h
*       ips_ar.h
*       ips_tim.h
*       ips_sadb.h
*       ips_spdb.h
*       ips_grp.h
*       ips_auth.h
*       ips_enc.h
*       ips_ah.h
*       ips_esp.h
*       ips_cmac_xcbc_aes.h
*       ips6_ip.h
*
*************************************************************************/
#ifndef IPS_API_H
#define IPS_API_H

#ifdef          __cplusplus
extern  "C" {                             /* C declarations in C++ */
#endif /* _cplusplus */

#include "services/reg_api.h"

/*** APIs for the creation of policies and security associations. ***/
#include "networking/ips_ip.h"
#include "networking/ips_cfg.h"
#include "networking/ips.h"
#include "networking/ips_esn.h"
#include "networking/ips_ar.h"
#include "networking/ips_tim.h"
#include "networking/ips_sadb.h"
#include "networking/ips_spdb.h"
#include "networking/ips_grp.h"
#include "networking/ips_auth.h"
#include "networking/ips_enc.h"
#include "networking/ips_ah.h"
#include "networking/ips_esp.h"
#include "networking/ips_cmac_xcbc_aes.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/ips6_ip.h"
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef IPS_API_H */
