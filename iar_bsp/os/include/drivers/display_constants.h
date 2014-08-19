/*************************************************************************
*
*            Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*   display_constants.h
*
* DESCRIPTION
*
*   This file contains defines for configuring the LCD.
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
**************************************************************************/
#ifndef _DISPLAY_CONSTANTS_H_
#define _DISPLAY_CONSTANTS_H_

/* The possible modes. */

#define     BPP1_GRAYSCALE          0x00000001
#define     BPP2_GRAYSCALE          0x00000002
#define     BPP4_GRAYSCALE          0x00000004
#define     BPP8_GRAYSCALE          0x00000008
#define     BPP15_RGB_555           0x00000010
#define     BPP15_BGR_555           0x00000020
#define     BPP16_RGB_565           0x00000040
#define     BPP16_BGR_565           0x00000080
#define     BPP16_ARGB_1555         0x00000100
#define     BPP16_ABGR_1555         0x00000200
#define     BPP16_RGBA_5551         0x00000400
#define     BPP16_BGRA_5551         0x00000800
#define     BPP16_ARGB_4444         0x00001000
#define     BPP16_ABGR_4444         0x00002000
#define     BPP16_RGBA_4444         0x00004000
#define     BPP16_BGRA_4444         0x00008000
#define     BPP18_RGB_666           0x00010000
#define     BPP18_BGR_666           0x00020000
#define     BPP24_RGB_888           0x00040000
#define     BPP24_BGR_888           0x00080000
#define     BPP32_ARGB_8888         0x00100000
#define     BPP32_ABGR_8888         0x00200000
#define     BPP32_RGBA_8888         0x00400000
#define     BPP32_BGRA_8888         0x00800000
#define     BPP12_RGB_444           0x01000000
#define     BPP12_BGR_444           0x02000000

/* Defines for display control. */

#define     DISPLAY_BACKLIGHT       0x1
#define     DISPLAY_BRIGHTNESS      0x2

#endif /* _DISPLAY_CONSTANTS_H_ */
