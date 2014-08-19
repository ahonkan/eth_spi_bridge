/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
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
*       colors.h
*
*   DESCRIPTION
*
*       This file contains the color defines based off of bits per pixel.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       display_config.h
*
*************************************************************************/
#ifndef _COLORS_H_
#define _COLORS_H_

#include    "drivers/display_config.h"

#if (BPP == 16)

#if (DISPLAY_MODE == BPP15_RGB_555)

#define Black          0x0000
#define Blue           0x0010
#define Green          0x0200
#define Cyan           0x0210
#define Red            0x4000
#define Magenta        0x4010
#define Brown          0x4103
#define LtGray         0x6318
#define Gray           0x4210
#define LtBlue         0x001f
#define LtGreen        0x03e0
#define LtCyan         0x03ff
#define LtRed          0x7c00
#define LtMagenta      0x7c1f
#define Yellow         0x7fe0
#define White          0xffff

#elif (DISPLAY_MODE == BPP15_BGR_555)

#define Black          0x0000
#define Blue           0x4000
#define Green          0x0200
#define Cyan           0x4200
#define Red            0x0010
#define Magenta        0x4010
#define Brown          0x0D10
#define LtGray         0x6318
#define Gray           0x4210
#define LtBlue         0x7C00
#define LtGreen        0x03e0
#define LtCyan         0x7FE0
#define LtRed          0x001F
#define LtMagenta      0x7c1f
#define Yellow         0x03FF
#define White          0xFFFF

#elif (DISPLAY_MODE == BPP16_BGR_565)

#define Black          0x0000
#define Blue           0x8000
#define Green          0x0400
#define Cyan           0x8400
#define Red            0x0010
#define Magenta        0x8010
#define Brown          0x1A10
#define LtGray         0xC618
#define Gray           0x8410
#define LtBlue         0xF800
#define LtGreen        0x07E0
#define LtCyan         0xFFE0
#define LtRed          0x001F
#define LtMagenta      0xf81f
#define Yellow         0x07FF
#define White          0xFFFF

#elif (DISPLAY_MODE == BPP16_RGB_565)

#define Black          0x0000
#define Blue           0x0010
#define Green          0x0400
#define Cyan           0x0410
#define Red            0x8000
#define Magenta        0x8010
#define Brown          0x8203
#define LtGray         0xc618
#define Gray           0x8410
#define LtBlue         0x001f
#define LtGreen        0x07e0
#define LtCyan         0x07ff
#define LtRed          0xf800
#define LtMagenta      0xf81f
#define Yellow         0xffe0
#define White          0xffff

#elif (DISPLAY_MODE == BPP16_ARGB_1555)

#define Black          0x0000
#define Blue           0x0010
#define Green          0x0200
#define Cyan           0x0210
#define Red            0x4000
#define Magenta        0x4010
#define Brown          0x4103
#define LtGray         0x6318
#define Gray           0x4210
#define LtBlue         0x001f
#define LtGreen        0x03e0
#define LtCyan         0x03ff
#define LtRed          0x7c00
#define LtMagenta      0x7c1f
#define Yellow         0x7fe0
#define White          0xffff

#elif (DISPLAY_MODE == BPP16_ABGR_1555)

#define Black          0x0000
#define Blue           0x4000
#define Green          0x0200
#define Cyan           0x4200
#define Red            0x0010
#define Magenta        0x4010
#define Brown          0x0D10
#define LtGray         0x6318
#define Gray           0x4210
#define LtBlue         0x7C00
#define LtGreen        0x03e0
#define LtCyan         0x7FE0
#define LtRed          0x001F
#define LtMagenta      0x7c1f
#define Yellow         0x03FF
#define White          0xFFFF

#elif (DISPLAY_MODE == BPP16_RGBA_5551)

#define Black          0x0000
#define Blue           0x0020
#define Green          0x0400
#define Cyan           0x0420
#define Red            0x8000
#define Magenta        0x8020
#define Brown          0x8206
#define LtGray         0xc630
#define Gray           0x8420
#define LtBlue         0x003e
#define LtGreen        0x07c0
#define LtCyan         0x07fe
#define LtRed          0xf800
#define LtMagenta      0xf83e
#define Yellow         0xffc0
#define White          0xfffe

#elif (DISPLAY_MODE == BPP16_BGRA_5551)

#define Black          0x0000
#define Blue           0x8000
#define Green          0x0400
#define Cyan           0x8400
#define Red            0x0020
#define Magenta        0x8020
#define Brown          0x1A20
#define LtGray         0xc630
#define Gray           0x8420
#define LtBlue         0xf800
#define LtGreen        0x07c0
#define LtCyan         0xffc0
#define LtRed          0x003e
#define LtMagenta      0xf83e
#define Yellow         0x07fe
#define White          0xfffe

#elif (DISPLAY_MODE == BPP16_RGBA_4444)

#define Black          0x0000
#define Blue           0x0080
#define Green          0x0800
#define Cyan           0x0880
#define Red            0x8000
#define Magenta        0x8080
#define Brown          0x8820
#define LtGray         0xCCC0
#define Gray           0x8880
#define LtBlue         0x00F0
#define LtGreen        0x0F00
#define LtCyan         0x0FF0
#define LtRed          0xF000
#define LtMagenta      0xF0F0
#define Yellow         0xFF00
#define White          0xFFF0

#elif (DISPLAY_MODE == BPP16_BGRA_4444)

#define Black          0x0000
#define Blue           0x8000
#define Green          0x0800
#define Cyan           0x8800
#define Red            0x0080
#define Magenta        0x8080
#define Brown          0x2880
#define LtGray         0xCCC0
#define Gray           0x8880
#define LtBlue         0xF000
#define LtGreen        0x0F00
#define LtCyan         0xFF00
#define LtRed          0x00F0
#define LtMagenta      0xF0F0
#define Yellow         0x0FF0
#define White          0xFFF0

#elif (DISPLAY_MODE == BPP16_ARGB_4444)

#define Black          0x0000
#define Blue           0x0008
#define Green          0x0080
#define Cyan           0x0088
#define Red            0x0800
#define Magenta        0x0808
#define Brown          0x0882
#define LtGray         0x0CCC
#define Gray           0x0888
#define LtBlue         0x000F
#define LtGreen        0x00F0
#define LtCyan         0x00FF
#define LtRed          0x0F00
#define LtMagenta      0x0F0F
#define Yellow         0x0FF0
#define White          0x0FFF

#elif (DISPLAY_MODE == BPP16_ABGR_4444)

#define Black          0x0000
#define Blue           0x0800
#define Green          0x0080
#define Cyan           0x0880
#define Red            0x0008
#define Magenta        0x0808
#define Brown          0x0288
#define LtGray         0x0CCC
#define Gray           0x0888
#define LtBlue         0x0F00
#define LtGreen        0x00F0
#define LtCyan         0x0FF0
#define LtRed          0x000F
#define LtMagenta      0x0F0F
#define Yellow         0x00FF
#define White          0x0FFF

#elif (DISPLAY_MODE == BPP12_RGB_444)

#define Black          0x000
#define Blue           0x008
#define Green          0x080
#define Cyan           0x088
#define Red            0x800
#define Magenta        0x808
#define Brown          0x882
#define LtGray         0xCCC
#define Gray           0x888
#define LtBlue         0x00F
#define LtGreen        0x0F0
#define LtCyan         0x0FF
#define LtRed          0xF00
#define LtMagenta      0xF0F
#define Yellow         0xFF0
#define White          0xFFF

#elif (DISPLAY_MODE == BPP12_BGR_444)

#define Black          0x000
#define Blue           0x800
#define Green          0x080
#define Cyan           0x880
#define Red            0x008
#define Magenta        0x808
#define Brown          0x288
#define LtGray         0xCCC
#define Gray           0x888
#define LtBlue         0xF00
#define LtGreen        0x0F0
#define LtCyan         0xFF0
#define LtRed          0x00F
#define LtMagenta      0xF0F
#define Yellow         0x0FF
#define White          0xFFF

#endif

#elif (BPP == 32)

#if (DISPLAY_MODE == BPP32_ABGR_8888)

#define Black          0x000000
#define Blue           0x800000
#define Green          0x008000
#define Cyan           0x808000
#define Red            0x000080
#define Magenta        0x800080
#define Brown          0x208080
#define LtGray         0xb0b0b0
#define Gray           0x808080
#define LtBlue         0xff0000
#define LtGreen        0x00ff00
#define LtCyan         0xffff00
#define LtRed          0x0000ff
#define LtMagenta      0xff00ff
#define Yellow         0x00ffff
#define White          0xffffff

#elif (DISPLAY_MODE == BPP32_ARGB_8888)

#define Black          0x000000
#define Blue           0x000080
#define Green          0x008000
#define Cyan           0x008080
#define Red            0x800000
#define Magenta        0x800080
#define Brown          0x808020
#define LtGray         0xb0b0b0
#define Gray           0x808080
#define LtBlue         0x0000ff
#define LtGreen        0x00ff00
#define LtCyan         0x00ffff
#define LtRed          0xff0000
#define LtMagenta      0xff00ff
#define Yellow         0xffff00
#define White          0xffffff

#elif (DISPLAY_MODE == BPP32_BGRA_8888)

#define Black          0x00000000
#define Blue           0x80000000
#define Green          0x00800000
#define Cyan           0x80800000
#define Red            0x00008000
#define Magenta        0x80008000
#define Brown          0x20808000
#define LtGray         0xb0b0b000
#define Gray           0x80808000
#define LtBlue         0xff000000
#define LtGreen        0x00ff0000
#define LtCyan         0xffff0000
#define LtRed          0x0000ff00
#define LtMagenta      0xff00ff00
#define Yellow         0x00ffff00
#define White          0xffffff00

#elif (DISPLAY_MODE == BPP32_RGBA_8888)

#define Black          0x00000000
#define Blue           0x00008000
#define Green          0x00800000
#define Cyan           0x00808000
#define Red            0x80000000
#define Magenta        0x80008000
#define Brown          0x80802000
#define LtGray         0xb0b0b000
#define Gray           0x80808000
#define LtBlue         0x0000ff00
#define LtGreen        0x00ff0000
#define LtCyan         0x00ffff00
#define LtRed          0xff000000
#define LtMagenta      0xff00ff00
#define Yellow         0xffff0000
#define White          0xffffff00

#endif

#elif (BPP == 24)

#if (DISPLAY_MODE == BPP24_BGR_888)
	
#define Black          0x000000
#define Blue           0x800000
#define Green          0x008000
#define Cyan           0x808000
#define Red            0x000080
#define Magenta        0x800080
#define Brown          0x208080
#define LtGray         0xb0b0b0
#define Gray           0x808080
#define LtBlue         0xff0000
#define LtGreen        0x00ff00
#define LtCyan         0xffff00
#define LtRed          0x0000ff
#define LtMagenta      0xff00ff
#define Yellow         0x00ffff
#define White          0xffffff

#elif (DISPLAY_MODE == BPP24_RGB_888)

#define Black          0x000000
#define Blue           0x000080
#define Green          0x008000
#define Cyan           0x008080
#define Red            0x800000
#define Magenta        0x800080
#define Brown          0x808020
#define LtGray         0xb0b0b0
#define Gray           0x808080
#define LtBlue         0x0000ff
#define LtGreen        0x00ff00
#define LtCyan         0x00ffff
#define LtRed          0xff0000
#define LtMagenta      0xff00ff
#define Yellow         0xffff00
#define White          0xffffff

#elif (DISPLAY_MODE == BPP18_BGR_666)

#define Black          0x00000
#define Blue           0x20000
#define Green          0x00800
#define Cyan           0x20800
#define Red            0x00020
#define Magenta        0x20020
#define Brown          0x08820
#define LtGray         0x2cb2c
#define Gray           0x20820
#define LtBlue         0x3f000
#define LtGreen        0x00fc0
#define LtCyan         0x3ffc0
#define LtRed          0x0003f
#define LtMagenta      0x3f03f
#define Yellow         0x00fff
#define White          0x3ffff

#elif (DISPLAY_MODE == BPP18_RGB_666)

#define Black          0x00000
#define Blue           0x00020
#define Green          0x00800
#define Cyan           0x00820
#define Red            0x20000
#define Magenta        0x20020
#define Brown          0x20808
#define LtGray         0x2cb2c
#define Gray           0x20820
#define LtBlue         0x0003f
#define LtGreen        0x00fc0
#define LtCyan         0x00fff
#define LtRed          0x3f000
#define LtMagenta      0x3f03f
#define Yellow         0x3ffc0
#define White          0x3ffff

#endif

#elif (BPP == 1)
/* 1 Bit per Pixel Display */

/* For the Low Level 1 Bit per Pixel functions to work correctly, the Colors
   defined have to be 8 bits of All Zeroes or 8 bits of All Ones. */

#define Black          0x00
#define White          0xff

/* The following Colors are being defined so GrafixWT will build for the
   1 Bit per Pixel Display */

#define LtRed          White
#define LtGreen        White
#define LtBlue         White
#define Yellow         White
#define LtCyan         White
#define LtMagenta      White
#define LtGray         White
#define Red            White
#define Green          White
#define Blue           White
#define Brown          White
#define Cyan           White
#define Magenta        White
#define Gray           White
#else

#ifndef USING_DIRECT_X
/* any other color BPP */
#define Black          0
#define Red            1
#define Green          2
#define Brown          3
#define Blue           4
#define Magenta        5
#define Cyan           6
#define LtGray         7

#define Gray           248
#define LtRed          249
#define LtGreen        250
#define Yellow         251
#define LtBlue         252
#define LtMagenta      253
#define LtCyan         254
#define White          255
#else
/* any other color BPP */
#define Black          0
#define Blue           10
#define Green          9
#define Cyan           12
#define Red            8
#define Magenta        13
#define Brown          11
#define LtGray         7
#define Gray           14
#define LtBlue         3
#define LtGreen        2
#define LtCyan         5
#define LtRed          1
#define LtMagenta      6
#define Yellow         4
#define White          15
#endif
#endif

#endif
