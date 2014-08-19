/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
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
*       display_config.h
*
*   DESCRIPTION
*
*       This file contains configuration defines which rarely require 
*       modification.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus_gen_cfg.h
*       display_constants.h   
*
*************************************************************************/
#ifndef _DISPLAY_CONFIG_H_
#define _DISPLAY_CONFIG_H_

#include "nucleus_gen_cfg.h"
#include "drivers/display_constants.h"

/* Set the interleaving for each pixel.  By default set to 1,0,0.*/
#define     INTERLEAVE              1
#define     INTERLEAVE_OFFSET       0
#define     INTERLEAVE_SEGMENT      0

/* Number of supported display devices. */
#define     numDisplays             1

/* Set the define to true to support aligned memory for grafMap. */
#define     GRAFMAP_MEM_ALIGN_SUPPORT       NU_FALSE

/* Uncomment one of the following defines for memory pool type. */

/* #define     GRAPHMAP_MEMORY_POOL            System_Memory */
#define     GRAFMAP_MEMORY_POOL             Uncached_System_Memory

/* Define the required alignment for buffer memory. */
#define     GRAFMAP_MEMORY_ALIGNMENT        4

/* Map FUSE generated defines to driver generic defines. */
#define     MAX_SCREEN_WIDTH_X              CFG_NU_OS_DRVR_DISPLAY_SCREEN_WIDTH
#define     MAX_SCREEN_HEIGHT_Y             CFG_NU_OS_DRVR_DISPLAY_SCREEN_HEIGHT
#define     BPP                             CFG_NU_OS_DRVR_DISPLAY_BITS_PER_PIXEL
#define     DRIVER_SUPPORTED_HW_MODES       CFG_NU_OS_DRVR_DISPLAY_HW_COLOR_FMT
#define     DISPLAY_MODE                    CFG_NU_OS_DRVR_DISPLAY_SW_COLOR_FMT
#define     DISPLAY_SUPPORT_PRE_PROCESS     CFG_NU_OS_DRVR_DISPLAY_PRE_PROC_HOOK
#define     DISPLAY_SUPPORT_POST_PROCESS    CFG_NU_OS_DRVR_DISPLAY_POST_PROC_HOOK
#define     HARDWARE_ALPHA_SUPPORTED        CFG_NU_OS_DRVR_DISPLAY_HW_ALPHA

/* Include color conversion defines. */
#include    "drivers/color_convert.h"

/* To define NO BLIT support  */
#if (CFG_NU_OS_DRVR_DISPLAY_NO_BLIT_SUPPORT == NU_TRUE)
#define     NO_BLIT_SUPPORT
#endif

/* No image support will be available in display driver
   if image support is not required in GRAFIX RS. 			*/
#if (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_FALSE)
/* Note: mouse requires image support */
#define     NO_IMAGE_SUPPORT
#endif /* (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_FALSE) */

/* Define NO_REGION_CLIP to remove region clipping from the code */

#if (CFG_NU_OS_DRVR_DISPLAY_NO_REGION_CLIP == NU_TRUE)
#define     NO_REGION_CLIP
#endif

/* Define SMART_LCD for asynchronous displays. Smart LCDs have onchip
 * display buffer. Usually we can't allocate framebuffer due to limited
 * memory available.  */
#if (CFG_NU_OS_DRVR_DISPLAY_SMART_LCD == NU_TRUE)
#define     SMART_LCD
#endif

/* Check if the current display is not smart lcd. */
#ifndef SMART_LCD


/* Uncomment the define to enable the copy optimizations in blit and fill
   operations. These defines enable 32-bit copy to the LCD buffer instead
   of 8-bit in the case of 24 bpp and 16-bit in the case of 16 bpp mode. */

#define     LCD_OPTIMIZE_BLIT
#define     LCD_OPTIMIZE_FILL 

#endif /* SMART_LCD */

#endif /* _DISPLAY_CONFIG_H_ */

