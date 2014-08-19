/**
 * @file   gpio.c
 * @Author Avinash avinash@terabitradios.com
 * @date   July 23 2014
 * @brief  GPIO IO defines.
 *
 *
 */


#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "nucleus_gen_cfg.h"

#include "gpio.h"
#include "gpio_defs.h"

#ifdef CFG_IAR_STM32F429II_SK_GPIO_ENABLE

/*
 * IAR STM32F429II GPIO defines
 */

GPIO_CREATE(JOY_UP,    I, 11, INPUT, NOPULL, LOW)
GPIO_CREATE(JOY_DOWN,  I, 10, INPUT, NOPULL, LOW)
GPIO_CREATE(JOY_LEFT,  I,  9, INPUT, NOPULL, LOW)
GPIO_CREATE(JOY_RIGHT, I,  8, INPUT, NOPULL, LOW)
GPIO_CREATE(JOY_CENT,  B,  5, INPUT, NOPULL, LOW)

GPIO_CREATE(LED1, A, 4, OUTPUT_OD, NOPULL, LOW)
GPIO_CREATE(LED2, G, 3, OUTPUT_OD, NOPULL, LOW)
GPIO_CREATE(LED3, E, 2, OUTPUT_OD, NOPULL, LOW)
GPIO_CREATE(LED4, E, 3, OUTPUT_OD, NOPULL, LOW)


ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_gpio_setup (VOID))
{
  __GPIOA_CLK_ENABLE();
  __GPIOB_CLK_ENABLE();
  __GPIOE_CLK_ENABLE();
  __GPIOG_CLK_ENABLE();
  __GPIOI_CLK_ENABLE();

  LED1_INIT();
  LED2_INIT();
  LED3_INIT();
  LED4_INIT();

  LED1_SET;
  LED1_CLEAR;
  LED2_SET;
  LED2_CLEAR;
  LED3_SET;
  LED3_CLEAR;
  LED4_SET;
  LED4_CLEAR;

  LED1_SET;
  LED2_SET;
  LED3_SET;
  LED4_SET;

}

#endif
/******************************* END OF FILE *********************************/
