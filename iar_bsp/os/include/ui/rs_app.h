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
*  rs_app.h                                                     
*
* DESCRIPTION
*
*  This file contains "Master" include file list.
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
#ifndef _RS_APP_H_
#define _RS_APP_H_


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "nucleus.h"
#include "kernel/nu_kernel.h"

/* NEW version */
#include "ui/rsconst.h"  

/* Font File data structure */
#include "ui/rsfonts.h"  

/* GrafPort & GrafMap structures */
#include "ui/rsports.h"

/* Misc function prototypes */
#include "ui/rsextern.h" 

/* API function prototypes */
#include "ui/rs_api.h"   

#include "ui/str_utils.h"

#include "ui/std_utils.h"

#include "ui/memrymgr.h"

#include "ui/rects.h"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _RS_APP_H_ */


