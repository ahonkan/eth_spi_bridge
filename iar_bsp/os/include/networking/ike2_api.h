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
*       ike2_api.h
*
* COMPONENT
*
*       IKEv2 - API
*
* DESCRIPTION
*
*       This file includes other IKEv2 header files needed to use
*       the IKEv2 APIs. This file is included in ike_api.h file for
*       IKEv2 to be used by applications.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       ike2_cfg.h
*       ike2.h
*       ike2_pload.h
*       ike2_db.h
*       ike2_xchg.h
*       ike2_db.h
*       ike2_ctrl.h
*       ike2_sadb.h
*       ike2_pkt.h
*       ike2_evt.h
*       ike2_keymat.h
*       ike2_info.h
*       ike2_ips.h
*
*************************************************************************/
#ifndef IKE2_API_H
#define IKE2_API_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

#include "networking/ike2.h"
#include "networking/ike2_pload.h"
#include "networking/ike2_db.h"
#include "networking/ike2_xchg.h"
#include "networking/ike2_db.h"
#include "networking/ike2_ctrl.h"
#include "networking/ike2_sadb.h"
#include "networking/ike2_pkt.h"
#include "networking/ike2_evt.h"
#include "networking/ike2_keymat.h"
#include "networking/ike2_info.h"
#include "networking/ike2_ips.h"

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif
