/*!
 * \file cpu0.c
 *
 * \author Avinash Honkan
 * \date   8 July 2014
 *
 * BSP for the IAR STM32F429II-SK board
 */
/***********************************************************************
*
*             Copyright 2013 Mentor Graphics Corporation
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
*       cpu0.c
*
*   DESCRIPTION
*
*       This file contains IAR_STM32F429II_SK CPU0 device specific code
*
*   FUNCTIONS
*
*       iar_stm32f429ii_sk_cpu0_setup
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nucleus_gen_cfg.h
*       cpu_tgt.h 
*
***********************************************************************/
/* Include configuration header file */
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "nucleus_gen_cfg.h"

#if defined(CFG_IAR_STM32F429II_SK_CPU0_ENABLE)

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_cpu0_setup
*
*   DESCRIPTION
*
*       This function sets-up the target for CPU
*
*   CALLED BY
*
*       CPU_Register
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
#include    "bsp/drivers/cpu/stm32f2x/cpu_tgt.h"

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern CPU_REGISTER CPU_register_info;
#endif 

/* Assuming core clock set to 180M, the following frequency settings are defined */

/* Clock, AHB divisor, APB1 divisor, APB2 divisor, AHB frequency, APB1 frequency, APB2 frequency */
CPU_DVFS_OP CPU0_OP_Set[] ={
                            {CPU_PLL, AHB_DIV_8, APB_DIV_1, APB_DIV_2, 0, 0, 0},   /* #0 AHB=22.5MHz, APB1=22.5MHz,  APB2=11.25MHz */
                            {CPU_PLL, AHB_DIV_4, APB_DIV_1, APB_DIV_2, 0, 0, 0},   /* #1 AHB=45MHz,   APB1=45MHz,  APB2=22.5MHz  */
                            {CPU_PLL, AHB_DIV_2, APB_DIV_2, APB_DIV_2, 0, 0, 0},   /* #2 AHB=90MHz,   APB1=45MHz,  APB2=45MHz    */
                            {CPU_PLL, AHB_DIV_1, APB_DIV_4, APB_DIV_2, 0, 0, 0}};  /* #3 AHB=180MHz,  APB1=45MHz,  APB2=90MHz (startup) */

ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_cpu0_setup(VOID))
{
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /* Setup the register control block */
    CPU_register_info.pm_count = sizeof(CPU0_OP_Set) / sizeof(CPU_DVFS_OP);
    CPU_register_info.pm_op_list = &CPU0_OP_Set[0];
#endif  /* CFG_NU_OS_SVCS_PWR_ENABLE */
}

#endif /* defined(CFG_IAR_STM32F429II_SK_CPU0_ENABLE) */
