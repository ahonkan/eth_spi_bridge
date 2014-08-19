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
*       ike_api.h
*
* COMPONENT
*
*       IKE - API
*
* DESCRIPTION
*
*       This file includes other IKE header files needed to use
*       the IKE APIs. This file is not included in any IKE
*       source files but should only be included by external
*       applications wanting to use the IKE API. Including this
*       single file defines all the API data and functions.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       nu_net.h
*       ips.h
*       ips_tim.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*       ike_doi.h
*       ike_pload.h
*       ike_ctrl.h
*       ike_xchg.h
*       ike_dh.h
*       ike_cert.h
*
*************************************************************************/
#ifndef IKE_API_H
#define IKE_API_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

#include "networking/nu_net.h"
#include "networking/ips.h"
#include "networking/ips_tim.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"
#include "networking/ike_doi.h"
#include "networking/ike_pload.h"
#include "networking/ike_ctrl.h"
#include "networking/ike_xchg.h"
#include "networking/ike_dh.h"
#include "networking/ike_cert.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

#include "networking/ike2_api.h"

#endif

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_API_H */
