/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2002              
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
*       nu_websr.h                                   
*
*   COMPONENT
*
*       Nucleus WebServ
*
*   DESCRIPTION
*
*       This file includes all required header files for
*       Nucleus WebServ.
*
*   DATA STRUCTURES
*
*       None
*
*   FILE DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef _NU_WEBSR_H
#define _NU_WEBSR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "nucleus.h"
#include "networking/nu_net.h"
#include "storage/pcdisk.h"
#include "networking/nerrs.h"
#include "networking/ws_cfg.h"
#include "networking/ws_defs.h"
#include "networking/ws_extrn.h"

#if INCLUDE_SSL
#include "openssl/ssl.h"
#endif

#endif /* _NU_WEBSR_H */
