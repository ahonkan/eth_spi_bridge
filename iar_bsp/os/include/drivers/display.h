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
*       display.h
*
*   DESCRIPTION
*
*       This file contains definitions and structures for the display 
*       driver.
*
*   DATA STRUCTURES
*
*       DISPLAY_DEVICE
*
*   DEPENDENCIES
*
*       nucleus.h
*
*************************************************************************/
#ifndef     DISPLAY_H
#define     DISPLAY_H

#include    "nucleus.h"
#include    "kernel/dev_mgr.h"

/* Standard GUID for display devices. */
#define     DISPLAY_LABEL    {0xa3,0xbb,0xb8,0x65,0xda,0xea,0x4e,0xc1,0x8e,0xfe,0xfd,0x59,0x7e,0x59,0x89,0xd9}

/* Maximum number of Display sessions/instances */
#define     DISPLAY_MAX_INSTANCES               1
#define     DISPLAY_MAX_SESSIONS                (DISPLAY_MAX_INSTANCES)


/* Display IOCTL commands base offset. */
#define     IOCTL_DISPLAY_BASE                  (DV_IOCTL0 + 1)

/* Display IOCTL commands. */
#define     DISPLAY_GET_FRAME_BUFFER            0
#define     DISPLAY_GET_MW_CONFIG_PATH          1
#define     DISPLAY_BACKLIGHT_CTRL              2
#define     DISPLAY_BACKLIGHT_STATUS            3
#define     DISPLAY_GET_CONTRAST                4
#define     DISPLAY_SET_CONTRAST                5
#define     DISPLAY_GET_READ_PALETTE_FUNC       6
#define     DISPLAY_GET_WRITE_PALETTE_FUNC      7
#define     DISPLAY_GET_PREPROCESS_HOOK         8
#define     DISPLAY_GET_POSTPROCESS_HOOK        9
#define     DISPLAY_GET_FILLRECT_HOOK           10
#define     DISPLAY_GET_SETPIXEL_HOOK           11
#define     DISPLAY_GET_GETPIXEL_HOOK           12
#define     DISPLAY_PWR_HIB_RESTORE             13

/* Total number of display IOCTL commands. */
#define     DISPLAY_IOCTL_COUNT                 14

/* Power IOCTL commands base offset. */
#define     DISPLAY_POWER_BASE                  (IOCTL_DISPLAY_BASE + DISPLAY_IOCTL_COUNT)

/* Definition of power states. */

#define     LCD_OFF                             0
#define     LCD_DIM                             1
#define     LCD_ON                              2
#define     LCD_BRIGHT                          3

#define     DISPLAY_TOTAL_STATE_COUNT           4

/* Data structure to represent display device. */
typedef struct _display_device_struct
{
    DV_DEV_HANDLE   display_device;
    VOID            *display_frame_buffer;
    
#if         (BPP < 16)
    
    VOID            (*display_read_palette)(VOID *palette_ptr);
    VOID            (*display_write_palette)(VOID *palette_ptr);

#endif      /* (BPP < 16) */

#if         (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE)

    VOID            (*display_pre_process_hook)(VOID);
    
#endif      /* (DISPLAY_SUPPORT_PRE_PROCESS == NU_TRUE) */

#if         (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE)

    VOID            (*display_post_process_hook)(VOID);
    
#endif      /* (DISPLAY_SUPPORT_POST_PROCESS == NU_TRUE) */

#ifdef SMART_LCD

    VOID            (*display_fill_rect_hook)(UINT16 x_min, UINT16 x_max, UINT16 y_min, UINT16 y_max, UINT32 fill_colour);
    VOID            (*display_set_pixel_hook)(UINT16 x_coordinate, UINT16 y_coordinate, UINT32 fill_colour);
    VOID            (*display_get_pixel_hook)(UINT16 x_coordinate, UINT16 y_coordinate, UINT32* fill_colour_ptr);

#endif      /* SMART_LCD */

    UINT32          display_screen_width;
    UINT32          display_screen_height;
    UINT16          display_horizontal_dpi;
    UINT16          display_vertical_dpi;
    UINT16          display_refresh_rate;
    UINT8           display_bits_per_pixel;
    UINT8           display_planes_per_pixel;
    
} DISPLAY_DEVICE;
 
#endif      /* DISPLAY_H */

