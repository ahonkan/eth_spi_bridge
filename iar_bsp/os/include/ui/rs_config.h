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
*       rs_config.h                                                    
*
* DESCRIPTION
*
*       This file contains Grafix RS configuration options.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nucleus.h
*
***************************************************************************/
#ifndef _RS_CONFIG_H_
#define _RS_CONFIG_H_

#include "nucleus.h"

#if (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE)

#if (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_FALSE)
#error Image Support is required by Mouse Cursor to work properly
#endif /* (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_FALSE) */

/* Enable cursor handling. */
#define     USE_CURSOR
#endif /* (CFG_NU_OS_UI_GRAFIXRS_CURSOR_SUPPORT == NU_TRUE) */

#if (CFG_NU_OS_UI_GRAFIXRS_GLOBAL_ALPHA_SUPPORT == NU_TRUE)

/* Enable global alpha support. */
#define     GLOBAL_ALPHA_SUPPORT

#endif /* (CFG_NU_OS_UI_GRAFIXRS_GLOBAL_ALPHA_SUPPORT == NU_TRUE) */

#if (CFG_NU_OS_UI_GRAFIXRS_FILL_PATTERNS_SUPPORT == NU_TRUE)

/* Enable fill patterns support. */
#define     FILL_PATTERNS_SUPPORT

#endif /* (CFG_NU_OS_UI_GRAFIXRS_FILL_PATTERNS_SUPPORT == NU_TRUE) */

#if (CFG_NU_OS_UI_GRAFIXRS_DASHED_LINE_SUPPORT == NU_TRUE)

/* Enable dashed line support. */
#define     DASHED_LINE_SUPPORT

#endif /* (CFG_NU_OS_UI_GRAFIXRS_DASHED_LINE_SUPPORT == NU_TRUE) */

#if (CFG_NU_OS_UI_GRAFIXRS_INCLUDE_DEFAULT_FONT == NU_TRUE)

/* Enable default font inclusion. */
#define     INCLUDE_DEFAULT_FONT

#endif /* (CFG_NU_OS_UI_GRAFIXRS_INCLUDE_DEFAULT_FONT == NU_TRUE) */

/* The define specified the size of the workspace buffer. */
#define     WORKSPACE_BUFFER_SIZE       4096

/* Uncomment the following define to enable thin (single pixel) line 
   optimization. */
/* #define     THIN_LINE_OPTIMIZE */ 

/* Uncomment the following define to enable verbose error reporting. */
/* #define     GFX_VERBOSE_ERROR */

#endif /* _RS_CONFIG_H_ */

