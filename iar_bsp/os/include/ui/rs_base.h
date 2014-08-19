/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                                       
*
*  rs_base.h                                                    
*
* DESCRIPTION
*
*  Rendering Services base include paths.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _RS_BASE_H_
#define _RS_BASE_H_

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "ui/rs_config.h"
#include "drivers/display_config.h"
#include "ui/masterinc.h"
#include "ui/rsconst.h"    
#include "ui/rsfonts.h"
#include "ui/rsports.h"    
#include "ui/grafdata.h"
#include "nucleus_gen_cfg.h"

/* Define Supervisor and User mode functions */
#if (!defined(NU_SUPERV_USER_VARIABLES))
    #define NU_IS_SUPERVISOR_MODE() (NU_TRUE)
    #define NU_SUPERVISOR_MODE() ((VOID) 0)
    #define NU_USER_MODE() ((VOID) 0)

    /* Not a Supervisor/User kernel */
    #define NU_SUPERV_USER_VARIABLES    

#endif /* NU_SUPERV_USER_MODE */

#endif /* _RS_BASE_H_ */




