/**
 * @file   gpio_tgt.h
 * @Author Avinash avinash@terabitradios.com
 * @date   July 23 2014
 * @brief  GPIO defines
 *
 */

#ifndef __INCLUDE_ONCE_GPIO_TGT_H__
#define __INCLUDE_ONCE_GPIO_TGT_H__


#include "gpio.h"

/*
 * IAR STM32F429II  board GPIO defines
 */
#define LED1_SET    GPIO_SET(A,4)
#define LED2_SET    GPIO_SET(G,3)
#define LED3_SET    GPIO_SET(E,2)
#define LED4_SET    GPIO_SET(E,3)

#define LED1_CLEAR    GPIO_CLEAR(A,4)
#define LED2_CLEAR    GPIO_CLEAR(G,3)
#define LED3_CLEAR    GPIO_CLEAR(E,2)
#define LED4_CLEAR    GPIO_CLEAR(E,3)

        


#endif // __INCLUDE_ONCE_GPIO_TGT_H__
/******************************* END OF FILE *********************************/
