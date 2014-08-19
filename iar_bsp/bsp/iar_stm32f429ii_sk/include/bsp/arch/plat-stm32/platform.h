/***********************************************************************
*
*             Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       platform.h
*
*   DESCRIPTION
*
*       This file is a wrapper header file that includes all the necessary
*       header files for the configured platform
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus_gen_cfg.h
*       stm32_defs.h
*       stm3210e_eval_defs.h
*       stm3220g_eval_defs.h
*       stm3240g_eval_defs.h
*       stm32429i_eval1_defs.h
*       iar_stm32f429ii_sk_defs.h
*
***********************************************************************/

#ifndef PLATFORM_H
#define PLATFORM_H

/* Include configuration header file */
#include    "nucleus_gen_cfg.h"

/* Include header file for processor */
//#include    "stm32_defs.h"

#ifdef CFG_NU_BSP_STM3210E_EVAL_ENABLE

/* Include header file for STM3210E-EVAL */
#include    "bsp/stm3210e_eval_defs.h"

#endif

#ifdef CFG_NU_BSP_STM3220G_EVAL_ENABLE

/* Include header file for STM3220x-EVAL */
#include    "bsp/stm3220g_eval_defs.h"

#endif

#ifdef CFG_NU_BSP_STM3240G_EVAL_ENABLE

/* Include header file for STM3220x-EVAL */
#include    "bsp/stm3240g_eval_defs.h"

#endif

#ifdef CFG_NU_BSP_STM32429I_EVAL1_ENABLE

/* Include header file for STM32429I-EVAL1 */
#include    "bsp/stm32429i_eval1_defs.h"

#endif

#ifdef CFG_NU_BSP_IAR_STM32F429II_SK_ENABLE

/* Include header file for IAR STM32F429II-SK */
#include    "bsp/iar_stm32f429ii_sk_defs.h"

/* Include header file for processor */
#include    "stm32_defs.h"

#endif

#endif  /* PLATFORM_H */
