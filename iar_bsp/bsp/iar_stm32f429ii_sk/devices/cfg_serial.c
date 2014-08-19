/**
 * @file   serial.c
 * @Author Avinash avinash@terabitradios.com
 * @date   July 31 2014
 * @brief  Configuration for serial ports
 *
 * I combined all serial hardware definitions in one file to make 
 * managing the code a bit easier.
 *
 */

/* Include required header files */
#include            "nucleus.h"

#ifdef CFG_IAR_STM32F429II_SK_SERIAL0_ENABLE

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_serial0_setup
*
*   DESCRIPTION
*
*       This function sets-up the target for USART1 Serial port
*
*   CALLED BY
*
*       STM32_USART_Register
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_serial0_setup (VOID))
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    
    /* Reset UART */
    __USART1_FORCE_RESET();
    __USART1_RELEASE_RESET();

    /* Peripheral clock enable */
    __USART1_CLK_ENABLE();
     
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();

    /**USART1 GPIO Configuration  
    PA10   ------> USART1_RX
    PB6   ------> USART1_TX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


}

#endif 

#ifdef CFG_IAR_STM32F429II_SK_SERIAL1_ENABLE

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_serial1_setup
*
*   DESCRIPTION
*
*       This function sets-up the target for USART2 Serial port
*
*   CALLED BY
*
*       STM32_USART_Register
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
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_serial1_setup (VOID))
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __GPIOB_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();
    
    /* Reset UART */
    __USART3_FORCE_RESET();
    __USART3_RELEASE_RESET();

    /* Peripheral clock enable */
    __USART3_CLK_ENABLE();

  /**USART3 GPIO Configuration
  PB11   ------> USART3_RX
  PD8   ------> USART3_TX
  */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}


#endif 

// #ifdef CFG_IAR_STM32F429II_SK_SERIAL1_ENABLE

/*--------------------------------- EOF -------------------------------------*/



