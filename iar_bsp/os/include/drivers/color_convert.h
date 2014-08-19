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
*       color_convert.h
*
*   DESCRIPTION
*   
*       This file contains defines for configuring the LCD.
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
#ifndef _COLOR_CONVERT_H_
#define _COLOR_CONVERT_H_

#include "drivers/display_config.h"


/************************************************************/
/* Include the files related to the supported color format. */
/************************************************************/

#if (DISPLAY_CUSTOM_INCLUDE == NU_FALSE)

/* Check if it is 32 bits per pixel mode. */
#if (BPP == 32)

    /* Check if 32 bits per pixel is supported by the driver and hardware. */
    #if ((DRIVER_SUPPORTED_HW_MODES & BPP32_ARGB_8888) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP32_ABGR_8888))

        /* Use 32 bits per pixel mode. */
        #define INCLUDE_32_BIT

    #else

        /* Check if 24 bits per pixel is supported. */
        #if (DRIVER_SUPPORTED_HW_MODES & BPP24_RGB_888)

            /* Use 24 bits per pixel mode. */
            #define INCLUDE_24_BIT

        /* Else check if 16 bits per pixel is supported. */
        #elif ((DRIVER_SUPPORTED_HW_MODES & BPP16_RGB_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_RGB_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGR_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_BGR_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_4444))

            /* Use 16 bits per pixel mode. */
            #define INCLUDE_16_BIT

        /* Else check if 8 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP8_GRAYSCALE)

            /* Use 8 bits per pixel mode. */
            #define INCLUDE_8_BIT */

        /* Else check if 4 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP4_GRAYSCALE)

            /* Use 4 bits per pixel mode. */
            #define INCLUDE_4_BIT

        /* Else check if 2 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP2_GRAYSCALE)

            /* Use 2 bits per pixel mode. */
            #define INCLUDE_2_BIT

        /* Else check if 1 Bbits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP1_GRAYSCALE)

            /* Use 1 BPP mode. */
            #define INCLUDE_1_BIT

        #endif

    #endif

#elif (BPP == 24)

    /* Check if 24 bits per pixel is supported by the driver and hardware. */
    #if ((DRIVER_SUPPORTED_HW_MODES & BPP24_RGB_888) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP24_BGR_888))

        /* Use 24 bits per pixel mode. */
        #define INCLUDE_24_BIT

    #else

        /* Check if 32 bits per pixel is supported. */
        #if (DRIVER_SUPPORTED_HW_MODES & BPP32_ARGB_8888)

            /* Use 32 bits per pixel mode. */
            #define INCLUDE_32_BIT

        /* Check if 16 bits per pixel is supported. */
        #elif ((DRIVER_SUPPORTED_HW_MODES & BPP16_RGB_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_RGB_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGR_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_BGR_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_4444))

            /* Use 16 bits per pixel mode. */
            #define INCLUDE_16_BIT

        /* Check if 8 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP8_GRAYSCALE)

            /* Use 8 bits per pixel mode. */
            #define INCLUDE_8_BIT

        /* Else check if 4 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP4_GRAYSCALE)

            /* Use 4 bits per pixel mode. */
            #define INCLUDE_4_BIT

        /* Else check if 2 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP2_GRAYSCALE)

            /* Use 2 bits per pixel mode. */
            #define INCLUDE_2_BIT

        /* Else check if 1 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP1_GRAYSCALE)

            /* Use 1 bits per pixel mode. */
            #define INCLUDE_1_BIT

        #endif

    #endif

#elif (BPP == 16)

    /* Check if 16 bits per pixel is supported by the driver and hardware. */
    #if ((DRIVER_SUPPORTED_HW_MODES & BPP16_RGB_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_RGB_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGR_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_BGR_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_4444))

        /* Use 16 bits per pixel mode. */
        #define INCLUDE_16_BIT

    #else

        /* Check if 32 bits per pixel is supported. */
        #if (DRIVER_SUPPORTED_HW_MODES & BPP32_ARGB_8888)

            /* Use 32 bits per pixel mode. */
            #define INCLUDE_32_BIT

        /* Check if 24 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP24_RGB_888)

            /* Use 24 bits per pixel mode. */
            #define INCLUDE_24_BIT

        /* Check if 8 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP8_GRAYSCALE)

            /* Use 8 bits per pixel mode. */
            #define INCLUDE_8_BIT

        /* Else check if 4 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP4_GRAYSCALE)

            /* Use 4 bits per pixel mode. */
            #define INCLUDE_4_BIT

        /* Else check if 2 BPP is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP2_GRAYSCALE)

            /* Use 2 bits per pixel mode. */
            #define INCLUDE_2_BIT

        /* Else check if 1 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP1_GRAYSCALE)

            /* Use 1 bits per pixel mode. */
            #define INCLUDE_1_BIT

        #endif

    #endif

#elif (BPP == 8)

    /* Check if 8 bits per pixel is supported by the driver and hardware. */
    #if (DRIVER_SUPPORTED_HW_MODES & BPP8_GRAYSCALE)

        /* Use 8 bits per pixel mode. */
        #define INCLUDE_8_BIT

    #else

        /* Else check if 4 bits per pixel is supported. */
        #if (DRIVER_SUPPORTED_HW_MODES & BPP4_GRAYSCALE)

            /* Use 4 bits per pixel mode. */
            #define INCLUDE_4_BIT

        /* Else check if 2 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP2_GRAYSCALE)

            /* Use 2 bits per pixel mode. */
            #define INCLUDE_2_BIT

        /* Else check if 1 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP1_GRAYSCALE)

            /* Use 1 bits per pixel mode. */
            #define INCLUDE_1_BIT

        /* Check if 16 bits per pixel is supported. */
        #elif ((DRIVER_SUPPORTED_HW_MODES & BPP16_RGB_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_RGB_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGR_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_BGR_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_4444))

            /* Use 16 bits per pixel mode. */
            #define INCLUDE_16_BIT

        /* Check if 24 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP24_RGB_888)

            /* Use 24 bits per pixel mode. */
            #define INCLUDE_24_BIT

        /* Check if 32 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP32_ARGB_8888)

            /* Use 32 bits per pixel mode. */
            #define INCLUDE_32_BIT

        #endif

    #endif

#elif (BPP == 4)

    /* Check if 4 bits per pixel is supported by the driver and hardware. */
    #if (DRIVER_SUPPORTED_HW_MODES & BPP4_GRAYSCALE)

        /* Use 4 bits per pixel mode. */
        #define INCLUDE_4_BIT

    #else

        /* Else check if 8 bits per pixel is supported. */
        #if (DRIVER_SUPPORTED_HW_MODES & BPP8_GRAYSCALE)

            /* Use 8 bits per pixel mode. */
            #define INCLUDE_8_BIT

        /* Else check if 2 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP2_GRAYSCALE)

            /* Use 2 bits per pixel mode. */
            #define INCLUDE_2_BIT

        /* Else check if 1 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP1_GRAYSCALE)

            /* Use 1 bits per pixel mode. */
            #define INCLUDE_1_BIT

        /* Check if 16 bits per pixel is supported. */
        #elif ((DRIVER_SUPPORTED_HW_MODES & BPP16_RGB_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_RGB_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGR_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_BGR_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_4444))

            /* Use 16 bits per pixel mode. */
            #define INCLUDE_16_BIT

        /* Check if 24 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP24_RGB_888)

            /* Use 24 bits per pixel mode. */
            #define INCLUDE_24_BIT

        /* Check if 32 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP32_ARGB_8888)

            /* Use 32 bits per pixel mode. */
            #define INCLUDE_32_BIT

        #endif

    #endif

#elif (BPP == 2)

    /* Check if 2 bits per pixel is supported by the driver and hardware. */
    #if (DRIVER_SUPPORTED_HW_MODES & BPP2_GRAYSCALE)

        /* Use 2 bits per pixel mode. */
        #define INCLUDE_2_BIT

    #else

        /* Else check if 4 bits per pixel is supported. */
        #if (DRIVER_SUPPORTED_HW_MODES & BPP4_GRAYSCALE)

            /* Use 4 bits per pixel mode. */
            #define INCLUDE_4_BIT

        /* Else check if 8 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP8_GRAYSCALE)

            /* Use 8 bits per pixel mode. */
            #define INCLUDE_8_BIT

        /* Else check if 1 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP1_GRAYSCALE)

            /* Use 1 bits per pixel mode. */
            #define INCLUDE_1_BIT

        /* Check if 16 bits per pixel is supported. */
        #elif ((DRIVER_SUPPORTED_HW_MODES & BPP16_RGB_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_RGB_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGR_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_BGR_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_4444))

            /* Use 16 bits per pixel mode. */
            #define INCLUDE_16_BIT

        /* Check if 24 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP24_RGB_888)

            /* Use 24 bits per pixel mode. */
            #define INCLUDE_24_BIT

        /* Check if 32 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP32_ARGB_8888)

            /* Use 32 bits per pixel mode. */
            #define INCLUDE_32_BIT

        #endif

    #endif

#elif (BPP == 1)

    /* Check if 1 bits per pixel is supported by the driver and hardware. */
    #if (DRIVER_SUPPORTED_HW_MODES & BPP1_GRAYSCALE)

        /* Use 1 bits per pixel mode. */
        #define INCLUDE_1_BIT

    #else

        /* Else check if 2 bits per pixel is supported. */
        #if (DRIVER_SUPPORTED_HW_MODES & BPP2_GRAYSCALE)

            /* Use 2 bits per pixel mode. */
            #define INCLUDE_2_BIT

        /* Else check if 4 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP4_GRAYSCALE)

            /* Use 4 bits per pixel mode. */
            #define INCLUDE_4_BIT

        /* Else check if 8 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP8_GRAYSCALE)

            /* Use 8 bits per pixel mode. */
            #define INCLUDE_8_BIT

        /* Check if 16 bits per pixel is supported. */
        #elif ((DRIVER_SUPPORTED_HW_MODES & BPP16_RGB_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_RGB_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGR_565) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP15_BGR_555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_1555) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_5551) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_RGBA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ARGB_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_BGRA_4444) || \
         (DRIVER_SUPPORTED_HW_MODES & BPP16_ABGR_4444))

            /* Use 16 bits per pixel mode. */
            #define INCLUDE_16_BIT

        /* Check if 24 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP24_RGB_888)

            /* Use 24 bits per pixel mode. */
            #define INCLUDE_24_BIT

        /* Check if 32 bits per pixel is supported. */
        #elif (DRIVER_SUPPORTED_HW_MODES & BPP32_ARGB_8888)

            /* Use 32 bits per pixel mode. */
            #define INCLUDE_32_BIT

        #endif

    #endif

#endif

#endif /* (DISPLAY_CUSTOM_INCLUDE == NU_FALSE) */

/* Check if it is bits per pixel 2 or bits per pixel 4 mode. */
#if (defined(INCLUDE_2_BIT) || defined(INCLUDE_4_BIT))

#define INCLUDE_2_4_BIT

#endif

/* Check if it is 565 mode for bits per pixel 16. */
#if (defined(INCLUDE_16_BIT))

#if ((DRIVER_SUPPORTED_HW_MODES & BPP16_BGR_565) && \
    !(DRIVER_SUPPORTED_HW_MODES & BPP15_BGR_555))

#define CM565

#endif

#if ((DRIVER_SUPPORTED_HW_MODES & BPP16_RGB_565) && \
    !(DRIVER_SUPPORTED_HW_MODES & BPP15_RGB_555))

#define CM565

#endif

#if ((DISPLAY_MODE == BPP16_RGB_565) && \
     (DRIVER_SUPPORTED_HW_MODES & BPP16_RGB_565))

#define CM565

#endif

#if ((DISPLAY_MODE == BPP16_BGR_565) && \
     (DRIVER_SUPPORTED_HW_MODES & BPP16_BGR_565))

#define CM565

#endif

#endif

/************************************************************/
/*    Initialize color conversion need related defines      */
/************************************************************/


/* Check if the display mode is supported by hardware. */
#if (DISPLAY_MODE & DRIVER_SUPPORTED_HW_MODES)

/* Color conversion is not needed. */
#define     COLOR_CONVERSION_NEEDED         NU_FALSE

#else

/* Color conversion is needed. */
#define     COLOR_CONVERSION_NEEDED         NU_TRUE

#endif

/* Check if color conversion is required. */
#if (COLOR_CONVERSION_NEEDED == NU_TRUE)

/************************************************************/
/* The color positions and lengths for all the supported    */
/* source color formats.                                    */
/************************************************************/

#if (DISPLAY_CUSTOM_SOURCE_COLOR == NU_FALSE)

#if (BPP == 24)

#if (DISPLAY_MODE == BPP24_RGB_888)

#define     SRC_RED_POSITION                16
#define     SRC_GREEN_POSITION              8
#define     SRC_BLUE_POSITION               0
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  8
#define     SRC_GREEN_LENGTH                8
#define     SRC_BLUE_LENGTH                 8
#define     SRC_ALPHA_LENGTH                0

#elif (DISPLAY_MODE == BPP24_BGR_888)

#define     SRC_RED_POSITION                0
#define     SRC_GREEN_POSITION              8
#define     SRC_BLUE_POSITION               16
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  8
#define     SRC_GREEN_LENGTH                8
#define     SRC_BLUE_LENGTH                 8
#define     SRC_ALPHA_LENGTH                0

#elif (DISPLAY_MODE == BPP18_RGB_666)

#define     SRC_RED_POSITION                12
#define     SRC_GREEN_POSITION              6
#define     SRC_BLUE_POSITION               0
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  6
#define     SRC_GREEN_LENGTH                6
#define     SRC_BLUE_LENGTH                 6
#define     SRC_ALPHA_LENGTH                0

#elif (DISPLAY_MODE == BPP18_BGR_666)

#define     SRC_RED_POSITION                0
#define     SRC_GREEN_POSITION              6
#define     SRC_BLUE_POSITION               12
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  6
#define     SRC_GREEN_LENGTH                6
#define     SRC_BLUE_LENGTH                 6
#define     SRC_ALPHA_LENGTH                0

#else

#error      "Select the mode compatible with the BPP."

#endif /* (DISPLAY_MODE == BPP24_RGB_888) */

#endif /* (BPP == 24) */



#if (BPP == 32)

#if (DISPLAY_MODE == BPP32_RGBA_8888)

#define     SRC_RED_POSITION                24
#define     SRC_GREEN_POSITION              16
#define     SRC_BLUE_POSITION               8
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  8
#define     SRC_GREEN_LENGTH                8
#define     SRC_BLUE_LENGTH                 8
#define     SRC_ALPHA_LENGTH                8

#elif (DISPLAY_MODE == BPP32_BGRA_8888)

#define     SRC_RED_POSITION                8
#define     SRC_GREEN_POSITION              16
#define     SRC_BLUE_POSITION               24
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  8
#define     SRC_GREEN_LENGTH                8
#define     SRC_BLUE_LENGTH                 8
#define     SRC_ALPHA_LENGTH                8

#elif (DISPLAY_MODE == BPP32_ARGB_8888)

#define     SRC_RED_POSITION                16
#define     SRC_GREEN_POSITION              8
#define     SRC_BLUE_POSITION               0
#define     SRC_ALPHA_POSITION              24

#define     SRC_RED_LENGTH                  8
#define     SRC_GREEN_LENGTH                8
#define     SRC_BLUE_LENGTH                 8
#define     SRC_ALPHA_LENGTH                8

#elif (DISPLAY_MODE == BPP32_ABGR_8888)

#define     SRC_RED_POSITION                0
#define     SRC_GREEN_POSITION              8
#define     SRC_BLUE_POSITION               16
#define     SRC_ALPHA_POSITION              24

#define     SRC_RED_LENGTH                  8
#define     SRC_GREEN_LENGTH                8
#define     SRC_BLUE_LENGTH                 8
#define     SRC_ALPHA_LENGTH                8

#else

#error      "Select a related mode."

#endif /* (DISPLAY_MODE == BPP32_RGBA_8888) */

#endif /* (BPP == 32) */



#if (BPP == 16)

#if (DISPLAY_MODE == BPP15_RGB_555)

#define     SRC_RED_POSITION                10
#define     SRC_GREEN_POSITION              5
#define     SRC_BLUE_POSITION               0
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  5
#define     SRC_GREEN_LENGTH                5
#define     SRC_BLUE_LENGTH                 5
#define     SRC_ALPHA_LENGTH                0

#elif (DISPLAY_MODE == BPP15_BGR_555)

#define     SRC_RED_POSITION                0
#define     SRC_GREEN_POSITION              5
#define     SRC_BLUE_POSITION               10
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  5
#define     SRC_GREEN_LENGTH                5
#define     SRC_BLUE_LENGTH                 5
#define     SRC_ALPHA_LENGTH                0

#elif (DISPLAY_MODE == BPP16_RGB_565)

#define     SRC_RED_POSITION                11
#define     SRC_GREEN_POSITION              5
#define     SRC_BLUE_POSITION               0
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  5
#define     SRC_GREEN_LENGTH                6
#define     SRC_BLUE_LENGTH                 5
#define     SRC_ALPHA_LENGTH                0

#elif (DISPLAY_MODE == BPP16_BGR_565)

#define     SRC_RED_POSITION                0
#define     SRC_GREEN_POSITION              5
#define     SRC_BLUE_POSITION               11
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  5
#define     SRC_GREEN_LENGTH                6
#define     SRC_BLUE_LENGTH                 5
#define     SRC_ALPHA_LENGTH                0

#elif (DISPLAY_MODE == BPP16_RGBA_5551)

#define     SRC_RED_POSITION                11
#define     SRC_GREEN_POSITION              6
#define     SRC_BLUE_POSITION               1
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  5
#define     SRC_GREEN_LENGTH                5
#define     SRC_BLUE_LENGTH                 5
#define     SRC_ALPHA_LENGTH                1

#elif (DISPLAY_MODE == BPP16_BGRA_5551)

#define     SRC_RED_POSITION                1
#define     SRC_GREEN_POSITION              6
#define     SRC_BLUE_POSITION               11
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  5
#define     SRC_GREEN_LENGTH                5
#define     SRC_BLUE_LENGTH                 5
#define     SRC_ALPHA_LENGTH                1

#elif (DISPLAY_MODE == BPP16_ARGB_1555)

#define     SRC_RED_POSITION                10
#define     SRC_GREEN_POSITION              5
#define     SRC_BLUE_POSITION               0
#define     SRC_ALPHA_POSITION              15

#define     SRC_RED_LENGTH                  5
#define     SRC_GREEN_LENGTH                5
#define     SRC_BLUE_LENGTH                 5
#define     SRC_ALPHA_LENGTH                1

#elif (DISPLAY_MODE == BPP16_ABGR_1555)

#define     SRC_RED_POSITION                0
#define     SRC_GREEN_POSITION              5
#define     SRC_BLUE_POSITION               10
#define     SRC_ALPHA_POSITION              15

#define     SRC_RED_LENGTH                  5
#define     SRC_GREEN_LENGTH                5
#define     SRC_BLUE_LENGTH                 5
#define     SRC_ALPHA_LENGTH                1

#elif (DISPLAY_MODE == BPP16_RGBA_4444)

#define     SRC_RED_POSITION                12
#define     SRC_GREEN_POSITION              8
#define     SRC_BLUE_POSITION               4
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  4
#define     SRC_GREEN_LENGTH                4
#define     SRC_BLUE_LENGTH                 4
#define     SRC_ALPHA_LENGTH                4

#elif (DISPLAY_MODE == BPP16_BGRA_4444)

#define     SRC_RED_POSITION                4
#define     SRC_GREEN_POSITION              8
#define     SRC_BLUE_POSITION               12
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  4
#define     SRC_GREEN_LENGTH                4
#define     SRC_BLUE_LENGTH                 4
#define     SRC_ALPHA_LENGTH                4

#elif (DISPLAY_MODE == BPP16_ARGB_4444)

#define     SRC_RED_POSITION                8
#define     SRC_GREEN_POSITION              4
#define     SRC_BLUE_POSITION               0
#define     SRC_ALPHA_POSITION              12

#define     SRC_RED_LENGTH                  4
#define     SRC_GREEN_LENGTH                4
#define     SRC_BLUE_LENGTH                 4
#define     SRC_ALPHA_LENGTH                4

#elif (DISPLAY_MODE == BPP16_ABGR_4444)

#define     SRC_RED_POSITION                0
#define     SRC_GREEN_POSITION              4
#define     SRC_BLUE_POSITION               8
#define     SRC_ALPHA_POSITION              12

#define     SRC_RED_LENGTH                  4
#define     SRC_GREEN_LENGTH                4
#define     SRC_BLUE_LENGTH                 4
#define     SRC_ALPHA_LENGTH                4

#elif (DISPLAY_MODE == BPP12_RGB_444)

#define     SRC_RED_POSITION                8
#define     SRC_GREEN_POSITION              4
#define     SRC_BLUE_POSITION               0
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  4
#define     SRC_GREEN_LENGTH                4
#define     SRC_BLUE_LENGTH                 4
#define     SRC_ALPHA_LENGTH                0

#elif (DISPLAY_MODE == BPP12_BGR_444)

#define     SRC_RED_POSITION                0
#define     SRC_GREEN_POSITION              4
#define     SRC_BLUE_POSITION               8
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  4
#define     SRC_GREEN_LENGTH                4
#define     SRC_BLUE_LENGTH                 4
#define     SRC_ALPHA_LENGTH                0

#else

#error      "Select a related mode."

#endif /* (DISPLAY_MODE == BPP15_RGB_555) */

#elif (BPP == 8)

#if (DISPLAY_MODE == BPP8_GRAYSCALE)

#define     SRC_RED_POSITION                0
#define     SRC_GREEN_POSITION              0
#define     SRC_BLUE_POSITION               0
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  8
#define     SRC_GREEN_LENGTH                8
#define     SRC_BLUE_LENGTH                 8
#define     SRC_ALPHA_LENGTH                0

#endif

#elif (BPP == 4)

#if (DISPLAY_MODE == BPP4_GRAYSCALE)

#define     SRC_RED_POSITION                0
#define     SRC_GREEN_POSITION              0
#define     SRC_BLUE_POSITION               0
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  4
#define     SRC_GREEN_LENGTH                4
#define     SRC_BLUE_LENGTH                 4
#define     SRC_ALPHA_LENGTH                0

#endif

#elif (BPP == 2)

#if (DISPLAY_MODE == BPP2_GRAYSCALE)

#define     SRC_RED_POSITION                0
#define     SRC_GREEN_POSITION              0
#define     SRC_BLUE_POSITION               0
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  2
#define     SRC_GREEN_LENGTH                2
#define     SRC_BLUE_LENGTH                 2
#define     SRC_ALPHA_LENGTH                0

#endif

#elif (BPP == 1)

#if (DISPLAY_MODE == BPP1_GRAYSCALE)

#define     SRC_RED_POSITION                0
#define     SRC_GREEN_POSITION              0
#define     SRC_BLUE_POSITION               0
#define     SRC_ALPHA_POSITION              0

#define     SRC_RED_LENGTH                  1
#define     SRC_GREEN_LENGTH                1
#define     SRC_BLUE_LENGTH                 1
#define     SRC_ALPHA_LENGTH                0

#endif /* (DISPLAY_MODE == BPP1_GRAYSCALE) */

#endif /* #elif (BPP == 1) */

#endif /* (DISPLAY_CUSTOM_SOURCE_COLOR == NU_FALSE) */

/******************************************************************/
/*   Driver supported color formats for different values.         */
/******************************************************************/

#if (DISPLAY_CUSTOM_TARGET_COLOR == NU_FALSE)

#if (defined(INCLUDE_32_BIT))

#define     DST_RED_POSITION                0
#define     DST_GREEN_POSITION              8
#define     DST_BLUE_POSITION               16
#define     DST_ALPHA_POSITION              24

#define     DST_RED_LENGTH                  8
#define     DST_GREEN_LENGTH                8
#define     DST_BLUE_LENGTH                 8
#define     DST_ALPHA_LENGTH                8

#elif (defined(INCLUDE_24_BIT))

#define     DST_RED_POSITION                16
#define     DST_GREEN_POSITION              8
#define     DST_BLUE_POSITION               0
#define     DST_ALPHA_POSITION              0

#define     DST_RED_LENGTH                  8
#define     DST_GREEN_LENGTH                8
#define     DST_BLUE_LENGTH                 8
#define     DST_ALPHA_LENGTH                0

#elif (defined(INCLUDE_16_BIT))

#define     DST_RED_POSITION                0
#define     DST_GREEN_POSITION              5
#define     DST_BLUE_POSITION               10
#define     DST_ALPHA_POSITION              0

#define     DST_RED_LENGTH                  5
#define     DST_GREEN_LENGTH                5
#define     DST_BLUE_LENGTH                 5
#define     DST_ALPHA_LENGTH                0

#elif (defined(INCLUDE_8_BIT))

#define     DST_RED_POSITION                0
#define     DST_GREEN_POSITION              0
#define     DST_BLUE_POSITION               0
#define     DST_ALPHA_POSITION              0

#define     DST_RED_LENGTH                  8
#define     DST_GREEN_LENGTH                8
#define     DST_BLUE_LENGTH                 8
#define     DST_ALPHA_LENGTH                0

#elif (defined(INCLUDE_4_BIT))

#define     DST_RED_POSITION                0
#define     DST_GREEN_POSITION              0
#define     DST_BLUE_POSITION               0
#define     DST_ALPHA_POSITION              0

#define     DST_RED_LENGTH                  4
#define     DST_GREEN_LENGTH                4
#define     DST_BLUE_LENGTH                 4
#define     DST_ALPHA_LENGTH                0

#elif (defined(INCLUDE_2_BIT))

#define     DST_RED_POSITION                0
#define     DST_GREEN_POSITION              0
#define     DST_BLUE_POSITION               0
#define     DST_ALPHA_POSITION              0

#define     DST_RED_LENGTH                  2
#define     DST_GREEN_LENGTH                2
#define     DST_BLUE_LENGTH                 2
#define     DST_ALPHA_LENGTH                0

#elif (defined(INCLUDE_1_BIT))

#define     DST_RED_POSITION                0
#define     DST_GREEN_POSITION              0
#define     DST_BLUE_POSITION               0
#define     DST_ALPHA_POSITION              0

#define     DST_RED_LENGTH                  1
#define     DST_GREEN_LENGTH                1
#define     DST_BLUE_LENGTH                 1
#define     DST_ALPHA_LENGTH                0

#endif /* #elif (defined(INCLUDE_1_BIT)) */

#endif /* (DISPLAY_CUSTOM_TARGET_COLOR == NU_FALSE) */


/******************************************************************/
/* Defines for conversion of colors considering the difference in */
/* number of bits per color.                                      */
/******************************************************************/


#if (SRC_RED_LENGTH > DST_RED_LENGTH)

#define     TGT_RED_LENGTH                  DST_RED_LENGTH
#define     RED_USHIFT_VALUE               (SRC_RED_LENGTH - DST_RED_LENGTH)
#define     RED_DSHIFT_VALUE                0

#else

#define     TGT_RED_LENGTH                  SRC_RED_LENGTH
#define     RED_USHIFT_VALUE                0
#define     RED_DSHIFT_VALUE               (DST_RED_LENGTH - SRC_RED_LENGTH)

#endif

#if (SRC_GREEN_LENGTH > DST_GREEN_LENGTH)

#define     TGT_GREEN_LENGTH                DST_GREEN_LENGTH
#define     GREEN_USHIFT_VALUE             (SRC_GREEN_LENGTH - DST_GREEN_LENGTH)
#define     GREEN_DSHIFT_VALUE              0

#else

#define     TGT_GREEN_LENGTH                SRC_GREEN_LENGTH
#define     GREEN_USHIFT_VALUE              0
#define     GREEN_DSHIFT_VALUE             (DST_GREEN_LENGTH - SRC_GREEN_LENGTH)

#endif

#if (SRC_BLUE_LENGTH > DST_BLUE_LENGTH)

#define     TGT_BLUE_LENGTH                 DST_BLUE_LENGTH
#define     BLUE_USHIFT_VALUE              (SRC_BLUE_LENGTH - DST_BLUE_LENGTH)
#define     BLUE_DSHIFT_VALUE               0

#else

#define     TGT_BLUE_LENGTH                 SRC_BLUE_LENGTH
#define     BLUE_USHIFT_VALUE               0
#define     BLUE_DSHIFT_VALUE              (DST_BLUE_LENGTH - SRC_BLUE_LENGTH)

#endif

#if (SRC_ALPHA_LENGTH > DST_ALPHA_LENGTH)

#define     TGT_ALPHA_LENGTH                DST_ALPHA_LENGTH
#define     ALPHA_USHIFT_VALUE             (SRC_ALPHA_LENGTH - DST_ALPHA_LENGTH)
#define     ALPHA_DSHIFT_VALUE              0

#else

#define     TGT_ALPHA_LENGTH                SRC_ALPHA_LENGTH
#define     ALPHA_USHIFT_VALUE              0
#define     ALPHA_DSHIFT_VALUE             (DST_ALPHA_LENGTH - SRC_ALPHA_LENGTH)

#endif


/******************************************************************/
/*          Color conversion macro defines                        */
/******************************************************************/


/* Define the macro to generate bit masks. */
#define     BIT_MASK(length)              ((0x1 << length) - 1)

#if (TGT_ALPHA_LENGTH == 0)

/* The color conversion macro. */

#define     COLOR_CONVERT(src_color, dst_color)                               \
    dst_color  = (((src_color >> (SRC_RED_POSITION + RED_USHIFT_VALUE)) &     \
                     BIT_MASK(TGT_RED_LENGTH))                                \
                         << (DST_RED_POSITION + RED_DSHIFT_VALUE))            \
               | (((src_color >> (SRC_GREEN_POSITION + GREEN_USHIFT_VALUE)) & \
                     BIT_MASK(TGT_GREEN_LENGTH))                              \
                         << (DST_GREEN_POSITION + GREEN_DSHIFT_VALUE))        \
               | (((src_color >> (SRC_BLUE_POSITION + BLUE_USHIFT_VALUE)) &   \
                     BIT_MASK(TGT_BLUE_LENGTH))                               \
                         << (DST_BLUE_POSITION + BLUE_DSHIFT_VALUE))

/* The color back conversion macro. */

#define     COLOR_BACK_CONVERT(src_color, dst_color)                          \
    dst_color  = (((src_color >> (DST_RED_POSITION + RED_DSHIFT_VALUE)) &     \
                     BIT_MASK(TGT_RED_LENGTH))                                \
                         << (SRC_RED_POSITION + RED_USHIFT_VALUE))            \
               | (((src_color >> (DST_GREEN_POSITION + GREEN_DSHIFT_VALUE)) & \
                     BIT_MASK(TGT_GREEN_LENGTH))                              \
                         << (SRC_GREEN_POSITION + GREEN_USHIFT_VALUE))        \
               | (((src_color >> (DST_BLUE_POSITION + BLUE_DSHIFT_VALUE)) &   \
                     BIT_MASK(TGT_BLUE_LENGTH))                               \
                         << (SRC_BLUE_POSITION + BLUE_USHIFT_VALUE))

#else

/* The color conversion macro. */

#define     COLOR_CONVERT(src_color, dst_color)                               \
    dst_color  = (((src_color >> (SRC_RED_POSITION + RED_USHIFT_VALUE)) &     \
                     BIT_MASK(TGT_RED_LENGTH))                                \
                         << (DST_RED_POSITION + RED_DSHIFT_VALUE))            \
               | (((src_color >> (SRC_GREEN_POSITION + GREEN_USHIFT_VALUE)) & \
                     BIT_MASK(TGT_GREEN_LENGTH))                              \
                         << (DST_GREEN_POSITION + GREEN_DSHIFT_VALUE))        \
               | (((src_color >> (SRC_BLUE_POSITION + BLUE_USHIFT_VALUE)) &   \
                     BIT_MASK(TGT_BLUE_LENGTH))                               \
                         << (DST_BLUE_POSITION + BLUE_DSHIFT_VALUE))          \
               | (((src_color >> (SRC_ALPHA_POSITION + ALPHA_USHIFT_VALUE)) & \
                     BIT_MASK(TGT_ALPHA_LENGTH))                              \
                         << (DST_ALPHA_POSITION + ALPHA_DSHIFT_VALUE))

/* The color back conversion macro. */

#define     COLOR_BACK_CONVERT(src_color, dst_color)                          \
    dst_color  = (((src_color >> (DST_RED_POSITION + RED_DSHIFT_VALUE)) &     \
                     BIT_MASK(TGT_RED_LENGTH))                                \
                         << (SRC_RED_POSITION + RED_USHIFT_VALUE))            \
               | (((src_color >> (DST_GREEN_POSITION + GREEN_DSHIFT_VALUE)) & \
                     BIT_MASK(TGT_GREEN_LENGTH))                              \
                         << (SRC_GREEN_POSITION + GREEN_USHIFT_VALUE))        \
               | (((src_color >> (DST_BLUE_POSITION + BLUE_DSHIFT_VALUE)) &   \
                     BIT_MASK(TGT_BLUE_LENGTH))                               \
                         << (SRC_BLUE_POSITION + BLUE_USHIFT_VALUE))          \
               | (((src_color >> (DST_ALPHA_POSITION + ALPHA_DSHIFT_VALUE)) & \
                     BIT_MASK(TGT_ALPHA_LENGTH))                              \
                         << (SRC_ALPHA_POSITION + ALPHA_USHIFT_VALUE))

#endif

#else

/* The color conversion is not required so simply assign the color values directly. */

#define     COLOR_CONVERT(src_color, dst_color)                               \
    dst_color  = src_color

#define     COLOR_BACK_CONVERT(src_color, dst_color)                          \
    dst_color  = src_color

#endif


#endif /* _COLOR_CONVERT_H_ */

