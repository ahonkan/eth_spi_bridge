/******************************************************************************
*                                                                             
*              Copyright Mentor Graphics Corporation 2006                     
*                        All Rights Reserved.                                 
*                                                                             
*  THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS       
*  THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS        
*  SUBJECT TO LICENSE TERMS.                                                  
*                                                                             
*                                                                             
*******************************************************************************
*******************************************************************************
*
* FILE NAME                                                       
*
*   img.h                                                      
*
* DESCRIPTION
*
*   Holds defines to handle images.
*
* DATA STRUCTURES
*
*   None
*
* DEPENDENCIES
*
*   None
*
******************************************************************************/
#ifndef IMG_H
#define IMG_H

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "nucleus_gen_cfg.h"

#if ((defined(CFG_NU_OS_UI_IMAGE_IMG_INCLUDED)) && \
    (CFG_NU_OS_UI_IMAGE_IMG_INCLUDED == 1))

#if (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_FALSE)
#error	Image support in GRAFIX RS should be enabled for Image Component
#endif /* (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_FALSE) */

#define IMG_INCLUDED
#endif /* CFG_NU_OS_UI_IMAGE_IMG_INCLUDED */


#if (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE)

/*  Max Number of Animated Frames allowed within an Image */
#define NUM_ANIMATED_FRAMES  50  

/*  Max Number animated GIFS */
#define NUM_ANIMATED_GIFS    10

#endif /* (CFG_NU_OS_UI_GRAFIXRS_IMAGE_SUPPORT == NU_TRUE) */
#endif
