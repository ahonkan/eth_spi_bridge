/**
 * @file   gpio.h
 * @Author Avinash avinash@terabitradios.com
 * @date   May 2 2014
 * @brief  GPIO setup and access abstraction defines
 *
 * This file defines GPIO access macros for system use. 
 *
 */
#include "stm32f4xx_hal_gpio.h"

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __gpio_H
#define __gpio_H
#ifdef __cplusplus
 extern "C" {
#endif

#define GPIO_CREATE(name, port, pin, mode, pull, speed)            \
                                                                   \
void name ## _WR(GPIO_PinState state);                             \
void name ## _WR(GPIO_PinState state)                              \
{ HAL_GPIO_WritePin(GPIO ## port, GPIO_PIN_ ## pin, state); }      \
                                                                   \
GPIO_PinState name ## _RD(void);                                   \
GPIO_PinState name ## _RD(void)                                    \
{ return HAL_GPIO_ReadPin(GPIO ## port, GPIO_PIN_ ## pin); }       \
                                                                   \
void name ## _INIT(void);                                          \
void name ## _INIT(void)                                           \
{                                                                  \
  GPIO_InitTypeDef GPIO_InitStruct;                                \
  __ ## GPIO ## port ## _CLK_ENABLE();                             \
                                                                   \
  GPIO_InitStruct.Pin = GPIO_PIN_ ## pin;                          \
  GPIO_InitStruct.Mode = GPIO_MODE_ ## mode;                       \
  GPIO_InitStruct.Pull = GPIO_ ## pull;                            \
  GPIO_InitStruct.Speed = GPIO_SPEED_ ## speed;                    \
  HAL_GPIO_Init(GPIO ## port, &GPIO_InitStruct);                   \
}

#define GPIO_SET(port, pin) GPIO ## port->BSRRL = GPIO_PIN_ ## pin
#define GPIO_CLEAR(port, pin) GPIO ## port->BSRRH = GPIO_PIN_ ## pin


#ifdef __cplusplus
}
#endif
#endif

/************************ Copyright Terabit Radios *****END OF FILE****/
