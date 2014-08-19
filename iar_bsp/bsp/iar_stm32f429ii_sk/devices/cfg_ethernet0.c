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
*       ethernet0.c
*
*   DESCRIPTION
*
*       This file contains STM3220x-EVAL ETHERNET0 device specific code
*
*   FUNCTIONS
*
*       stm3240g_eval_ethernet0_setup
*
*   DEPENDENCIES
*
*       nucleus.h                       Nucleus System constants
*
***********************************************************************/

/* Include required header files */
#include            "nucleus.h"

#ifdef CFG_IAR_STM32F429II_SK_ETHERNET0_ENABLE

/***********************************************************************
*
*   FUNCTION
*
*       stm3240g_eval_ethernet0_setup
*
*   DESCRIPTION
*
*       This function sets-up the target for EMAC access.
*
*   CALLED BY
*
*       EMAC_Register
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_ethernet0_setup (VOID))
{
  GPIO_InitTypeDef GPIO_InitStruct;

    /* Peripheral clock enable */
    __ETH_CLK_ENABLE();
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOG_CLK_ENABLE();

  /**ETH GPIO Configuration  
  PA2   ------> ETH_MDIO
  PC1   ------> ETH_MDC
  PA3   ------> ETH_MDINT
  PB11   ------> ETH_TX_EN
  PG13   ------> ETH_TXD0
  PG14   ------> ETH_TXD1 

  PA1   ------> ETH_REF_CLK
  PA7   ------> ETH_CRS_DV
  PC4   ------> ETH_RXD0
  PC5   ------> ETH_RXD1
  */   
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

}

#endif

/*----------------------------------- EOF -----------------------------------*/


