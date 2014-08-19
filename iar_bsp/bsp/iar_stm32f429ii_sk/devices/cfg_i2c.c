/**
 * @file   i2c.c
 * @Author Avinash avinash@terabitradios.com
 * @date   July 31 2014
 * @brief  Configuration for I2C ports
 *
 * I combined all serial hardware definitions in one file to make 
 * managing the code a bit easier.
 *
 */

/* Include required header files */
#include            "nucleus.h"


#ifdef CFG_IAR_STM32F429II_SK_I2C0_ENABLE

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_i2c0_setup
*
*   DESCRIPTION
*
*       This function setup the i2c component driver.
*
*   CALLED BY
*
*
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_i2c0_setup (VOID))
{
  GPIO_InitTypeDef GPIO_InitStruct;

    __GPIOB_CLK_ENABLE();
  
    /* Peripheral clock enable */
    __I2C1_CLK_ENABLE();
  
  /**I2C1 GPIO Configuration  
  PB6   ------> I2C1_SCL
  PB7   ------> I2C1_SDA 
  */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_i2c0_cleanup
*
*   DESCRIPTION
*
*       This function setup the i2c component driver.
*
*   CALLED BY
*
*
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_i2c0_cleanup (VOID))
{
  GPIO_InitTypeDef GPIO_InitStruct;

    /* Peripheral clock enable */
    __I2C1_CLK_DISABLE();
  
  /**I2C1 GPIO Configuration  
  PB6   ------> I2C1_SCL
  PB7   ------> I2C1_SDA 
  */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

#endif 

#ifdef CFG_IAR_STM32F429II_SK_I2C1_ENABLE

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_i2c1_setup
*
*   DESCRIPTION
*
*       This function setup the i2c component driver.
*
*   CALLED BY
*
*
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_i2c1_setup (VOID))
{
  GPIO_InitTypeDef GPIO_InitStruct;

    __GPIOF_CLK_ENABLE();
  
    /* Peripheral clock enable */
    __I2C2_CLK_ENABLE();
  
  /**I2C2 GPIO Configuration  
  PF0   ------> I2C2_SDA
  PF1   ------> I2C2_SCL 
  */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

}

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_i2c1_cleanup
*
*   DESCRIPTION
*
*       This function setup the i2c component driver.
*
*   CALLED BY
*
*
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_i2c1_cleanup (VOID))
{
  GPIO_InitTypeDef GPIO_InitStruct;

    /* Peripheral clock enable */
    __I2C2_CLK_DISABLE();
  
  /**I2C2 GPIO Configuration  
  PF0   ------> I2C2_SDA
  PF1   ------> I2C2_SCL 
  */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

}

#endif 

//#ifdef CFG_IAR_STM32F429II_SK_I2C2_ENABLE



/*------------------------------------ EOF ----------------------------------*/

