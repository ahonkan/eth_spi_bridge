/**
 * @file   spi.c
 * @Author Avinash avinash@terabitradios.com
 * @date   July 31 2014
 * @brief  Configuration for serial ports
 *
 * Combined all serial hardware definitions in one file to make 
 * managing the code a bit easier.
 *
 */

/* Include required header files */
#include            "nucleus.h"

#ifdef CFG_IAR_STM32F429II_SK_SPI0_ENABLE

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_spi0_setup
*
*   DESCRIPTION
*
*       This function sets-up the target for SPI driver access.
*
*   CALLED BY
*
*       SPI_Driver_Register
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_spi0_setup (VOID))
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* Peripheral clock enable */
  __SPI1_CLK_ENABLE();
  __GPIOA_CLK_ENABLE();
  
  /**SPI1 GPIO Configuration  
  PA4   ------> SPI1_NSS
  PA5   ------> SPI1_SCK
  PA6   ------> SPI1_MISO
  PA7   ------> SPI1_MOSI 
  */
    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_spi0_cleanup
*
*   DESCRIPTION
*
*       This function cleans up the target for SPI driver access.
*
*   CALLED BY
*
*       SPI_Driver_Register
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_spi0_cleanup (VOID))
{
  GPIO_InitTypeDef GPIO_InitStruct;

  __SPI1_CLK_DISABLE();

  /* convert pins to inputs */

  /**SPI1 GPIO Configuration  
  PA4   ------> SPI1_NSS
  PA5   ------> SPI1_SCK
  PA6   ------> SPI1_MISO
  PA7   ------> SPI1_MOSI 
  */
    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


}

#endif 

#ifdef CFG_IAR_STM32F429II_SK_SPI1_ENABLE

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_spi1_setup
*
*   DESCRIPTION
*
*       This function sets-up the target for SPI driver access.
*
*   CALLED BY
*
*       SPI_Driver_Register
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_spi1_setup (VOID))
{
  GPIO_InitTypeDef GPIO_InitStruct;

    /* Peripheral clock enable */
    __SPI2_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
  
  /**SPI2 GPIO Configuration  
  PC2   ------> SPI2_MISO
  PC3   ------> SPI2_MOSI
  PB10   ------> SPI2_SCK
  PB12   ------> SPI2_NSS 
  */
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_spi1_cleanup
*
*   DESCRIPTION
*
*       This function cleans up the target for SPI driver access.
*
*   CALLED BY
*
*       SPI_Driver_Register
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_spi1_cleanup (VOID))
{
  GPIO_InitTypeDef GPIO_InitStruct;

  __SPI2_CLK_DISABLE();

  /* convert pins to inputs */

  /**SPI2 GPIO Configuration  
  PC2   ------> SPI2_MISO
  PC3   ------> SPI2_MOSI
  PB10   ------> SPI2_SCK
  PB12   ------> SPI2_NSS 
  */
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

#endif 

#ifdef CFG_IAR_STM32F429II_SK_SPI4_ENABLE

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_spi4_setup
*
*   DESCRIPTION
*
*       This function sets-up the target for SPI driver access.
*
*   CALLED BY
*
*       SPI_Driver_Register
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_spi4_setup (VOID))
{
  GPIO_InitTypeDef GPIO_InitStruct;

    /* Peripheral clock enable */
    __SPI5_CLK_ENABLE();
    __GPIOF_CLK_ENABLE();
  
  /**SPI5 GPIO Configuration  
  PF8   ------> SPI5_MISO
  PF9   ------> SPI5_MOSI
  PF7   ------> SPI5_SCK
  PBF6  ------> SPI5_NSS 
  */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI5;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

}

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_spi4_cleanup
*
*   DESCRIPTION
*
*       This function cleans up the target for SPI driver access.
*
*   CALLED BY
*
*       SPI_Driver_Register
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_spi4_cleanup (VOID))
{
  GPIO_InitTypeDef GPIO_InitStruct;

  __SPI5_CLK_DISABLE();

  /* convert pins to inputs */
  /**SPI5 GPIO Configuration  
  PF8   ------> SPI5_MISO
  PF9   ------> SPI5_MOSI
  PF7   ------> SPI5_SCK
  PBF6  ------> SPI5_NSS 
  */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

}

#endif 


/*---------------------------------- EOF ------------------------------------*/
